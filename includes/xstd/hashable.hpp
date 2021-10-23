#pragma once
#include <algorithm>
#include <variant>
#include <optional>
#include "intrinsics.hpp"
#include "type_helpers.hpp"
#include "formatting.hpp"
#include "fnv64.hpp"
#include "crc.hpp"
#include "shared.hpp"
#include "ref_counted.hpp"

// [Configuration]
// XSTD_DEFAULT_HASHER: If set, changes the type of default hash_t.
//
#ifndef XSTD_DEFAULT_HASHER
	#if XSTD_HW_CRC32C
		namespace xstd { using xcrc = typename xstd::crc32c::rebind<0x7b1faccd>::type; };
		#define XSTD_DEFAULT_HASHER xstd::xcrc;
	#else
		#define XSTD_DEFAULT_HASHER xstd::fnv64
	#endif
#endif

namespace xstd
{
	using hash_t = XSTD_DEFAULT_HASHER;

	// XSTD hashable types implement [H T::hash() const] / [void T::hash( H& ) const].
	//
	template<typename T>
	concept CustomHashable = requires( const T& v ) { v.hash(); };
	template<typename T, typename H>
	concept CustomHashableAs = requires( const T& v ) { H{ v.hash() }; };
	template<typename T, typename H>
	concept CustomHashExtendable = requires( const T& v, H& hasher ) { v.hash( hasher ); };

	// XSTD hash-combinable hash types implement [void H::combine(const H2&)].
	//
	template<typename H1, typename H2>
	concept CustomHashCombinable = requires( H1& out, const H2& in ) { out.combine( in ); };

	// Checks if std::hash is specialized to hash the type.
	//
	template<typename T>
	concept StdHashable = requires( T v ) { std::hash<T>{}( v ); };

	// Forward definition of hasher type.
	//
	template<typename H, typename T = void>
	struct basic_hasher;

	// Hash combination helper.
	//
	template<typename H1, typename H2>
	__forceinline static constexpr void combine_hash( H1& out, const H2& in )
	{
		// If custom combinable:
		//
		if constexpr ( CustomHashCombinable<H1, H2> )
			out.combine( in );
		// Otherwise append as bytes:
		//
		else
			out.add_bytes( in.digest() );
	}

	// Define basic hasher.
	//
	template<typename H, typename T>
	struct basic_hasher
	{
		__forceinline constexpr static void extend( H& out, const T& value ) noexcept
		{
			// If tiable, redirect.
			//
			if constexpr ( Tiable<T> )
			{
				std::apply( [ & ] ( const auto&... elements ) { return extend_hash<H>( out, elements... ); }, make_mutable( value ).tie() );
			}
			// If STL container or array, hash each element and add container information.
			//
			else if constexpr ( Iterable<const T&> )
			{
				using value_type = std::decay_t<iterable_val_t<const T&>>;
				if constexpr ( Trivial<value_type> )
				{
					for ( const auto& entry : value )
						out.add_bytes( entry );
				}
				else
				{
					for ( const auto& entry : value )
						extend_hash<H>( out, entry );
				}
			}
			// If hash, combine.
			//
			else if constexpr ( Same<T, H> || Same<T, hash_t> )
			{
				combine_hash( out, value );
			}
			// If trivial type:
			//
			else if constexpr ( Trivial<T> || Pointer<T> || Same<T, any_ptr> || ( Integral<T> && ( sizeof( T ) == 8 || sizeof( T ) == 4 ) ) || Enum<T> )
			{
				out.add_bytes( value );
			}
			// If hashable using std::hash<>, redirect.
			//
			else if constexpr ( StdHashable<T> )
			{
				combine_hash( out, std::hash<T>{}( value ) );
			}
			else
			{
				static_assert( sizeof( T ) == -1, "Type is not hashable." );
			}
		}
		__forceinline constexpr H operator()( const T& value ) const noexcept
		{
			H out = {};
			extend( out, value );
			return out;
		}
	};

	// Vararg hasher wrapper that should be used to create hashes from N values.
	//
	template<typename H, typename... Tx>
	__forceinline static constexpr void extend_hash( H& out, const Tx&... values )
	{
		visit_tuple( [ & ] <typename T> ( const T& el )
		{
			if constexpr ( requires { basic_hasher<H, T>::extend( out, el ); } )
				basic_hasher<H, T>::extend( out, el );
			else
				combine_hash( out, basic_hasher<H, T>{}( el ) );
		}, std::tie( values... ) );
	}
	template<typename H, typename... Tx>
	__forceinline static constexpr H make_hash( const Tx&... values )
	{
		if constexpr ( sizeof...( Tx ) == 1 )
		{
			using T = first_of_t<Tx...>;
			return basic_hasher<H, T>{}( values... );
		}
		else
		{
			H result = {};
			extend_hash<H, Tx...>( result, values... );
			return result;
		}
	}
	template<typename... Tx>
	__forceinline static constexpr hash_t make_hash( const Tx&... el )
	{
		return make_hash<hash_t, Tx...>( el... );
	}
	
	// Overload for custom-hashables.
	//
	template<typename H, typename T> requires ( CustomHashable<T> || CustomHashExtendable<T, H> )
	struct basic_hasher<H, T>
	{
		__forceinline constexpr static void extend( H& out, const T& value ) noexcept
		{
			if constexpr ( CustomHashExtendable<T, H> )
				value.hash( out );
			else
				combine_hash( out, value.hash() );
		}
		__forceinline constexpr H operator()( const T& obj ) const noexcept
		{
			if constexpr ( CustomHashableAs<T, H> )
				return H{ obj.hash() };

			H hasher = {};
			extend( hasher, obj );
			return hasher;
		}
	};

	// Overload for std::reference_wrapper.
	//
	template<typename H, typename T>
	struct basic_hasher<H, std::reference_wrapper<T>>
	{
		__forceinline constexpr static void extend( H& out, const std::reference_wrapper<T>& value ) noexcept
		{
			if ( value )
				extend_hash<H>( out, value.get() );
			else
				out.template add_bytes<int8_t>( 0 );
		}
		__forceinline constexpr H operator()( const std::reference_wrapper<T>& value ) const noexcept
		{
			H out = {};
			extend( out, value );
			return out;
		}
	};

	// Overload for std::optional.
	//
	template<typename H, typename T>
	struct basic_hasher<H, std::optional<T>>
	{
		__forceinline constexpr static void extend( H& out, const std::optional<T>& value ) noexcept
		{
			if ( value )
				extend_hash<H>( out, *value );
			else
				out.template add_bytes<int8_t>( 0 );
		}
		__forceinline constexpr H operator()( const std::optional<T>& value ) const noexcept
		{
			H out = {};
			extend( out, value );
			return out;
		}
	};

	// Overload for std::variant.
	//
	template<typename H, typename... T>
	struct basic_hasher<H, std::variant<T...>>
	{
		__forceinline constexpr static void extend( H& out, const std::variant<T...>& value ) noexcept
		{
			size_t idx = value.index();
			if ( idx == std::variant_npos )
				return;

			if constexpr ( sizeof...( T ) <= UINT8_MAX )
				out.add_bytes( ( uint8_t ) idx );
			else if constexpr ( sizeof...( T ) <= UINT16_MAX )
				out.add_bytes( ( uint16_t ) idx );
			else
				out.add_bytes( ( uint32_t ) idx );
			
			std::visit( [ & ] ( const auto& arg ) { extend_hash<H>( out, arg ); }, value );
		}
		__forceinline constexpr H operator()( const std::variant<T...>& value ) const noexcept
		{
			H out = {};
			extend( out, value );
			return out;
		}
	};

	// Overload for XSTD smart pointers.
	//
	template<typename H, typename T>
	struct basic_hasher<H, shared<T>>
	{
		__forceinline constexpr static void extend( H& out, const shared<T>& value ) noexcept
		{
			extend_hash<H>( out, value.get() );
		}
		__forceinline constexpr H operator()( const shared<T>& value ) const noexcept
		{
			H out = {};
			extend( out, value );
			return out;
		}
	};
	template<typename H, typename T>
	struct basic_hasher<H, weak<T>>
	{
		__forceinline constexpr static void extend( H& out, const weak<T>& value ) noexcept
		{
			extend_hash<H>( out, value.get() );
		}
		__forceinline constexpr H operator()( const weak<T>& value ) const noexcept
		{
			H out = {};
			extend( out, value );
			return out;
		}
	};
	template<typename H, typename T>
	struct basic_hasher<H, ref<T>>
	{
		__forceinline constexpr static void extend( H& out, const ref<T>& value ) noexcept
		{
			extend_hash<H>( out, value.get() );
		}
		__forceinline constexpr H operator()( const ref<T>& value ) const noexcept
		{
			H out = {};
			extend( out, value );
			return out;
		}
	};

	// Overload for STL smart pointers.
	//
	template<typename H, typename T>
	struct basic_hasher<H, std::shared_ptr<T>>
	{
		__forceinline constexpr static void extend( H& out, const std::shared_ptr<T>& value ) noexcept
		{
			extend_hash<H>( out, value.get() );
		}
		__forceinline constexpr H operator()( const std::shared_ptr<T>& value ) const noexcept
		{
			H hasher = {};
			extend( hasher, value );
			return hasher;
		}
	};
	template<typename H, typename T>
	struct basic_hasher<H, std::weak_ptr<T>>
	{
		__forceinline constexpr static void extend( H& out, const std::weak_ptr<T>& value ) noexcept
		{
			extend_hash<H>( out, value.get() );
		}
		__forceinline constexpr H operator()( const std::weak_ptr<T>& value ) const noexcept
		{
			H hasher = {};
			extend( hasher, value );
			return hasher;
		}
	};
	template<typename H, typename T, typename Dx>
	struct basic_hasher<H, std::unique_ptr<T, Dx>>
	{
		__forceinline constexpr static void extend( H& out, const std::unique_ptr<T, Dx>& value ) noexcept
		{
			extend_hash<H>( out, value.get() );
		}
		__forceinline constexpr H operator()( const std::unique_ptr<T, Dx>& value ) const noexcept
		{
			H hasher = {};
			extend( hasher, value );
			return hasher;
		}
	};

	// Overload for contigious trivial iterables.
	//
	template<typename H, ContiguousIterable T> requires Trivial<iterable_val_t<T>>
	struct basic_hasher<H, T>
	{
		using U = iterable_val_t<T>;

		__forceinline constexpr static void extend( H& out, const T& value ) noexcept
		{
			// If C array with char/wchar element, we'll skip last one despite the lameness.
			//
			if constexpr ( std::is_array_v<T> && ( Same<U, char> || Same<U, char8_t> || Same<U, wchar_t> || Same<U, char16_t> ) )
			{
				extend_hash<H>( out, std::span{ std::begin( value ), std::end( value ) - 1 } );
			}
			else
			{
				if ( std::is_constant_evaluated() )
				{
					for ( const auto& entry : value )
						out.add_bytes( entry );
				}
				else
				{
					auto* first = &*std::begin( value );
					out.add_bytes( ( const uint8_t* ) first, sizeof( U ) * std::size( value ) );
				}
			}
		}
		__forceinline constexpr H operator()( const T& value ) const noexcept
		{
			H hasher = {};
			extend( hasher, value );
			return hasher;
		}
	};

	// Overload for C strings.
	//
	template<typename H, typename T> requires ( Same<T, char> || Same<T, char8_t> || Same<T, wchar_t> || Same<T, char16_t> ||
															  Same<T, const char> || Same<T, const char8_t> || Same<T, const wchar_t> || Same<T, const char16_t> )
	struct basic_hasher<H, T*>
	{
		__forceinline constexpr static void extend( H& out, T value ) noexcept
		{
			extend_hash<H>( out, std::basic_string_view<T>{ value } );
		}
		__forceinline constexpr H operator()( T value ) const noexcept
		{
			H hasher = {};
			extend( hasher, value );
			return hasher;
		}
	};

	// Overload for std::pair / std::tuple.
	//
	template<typename H, Tuple T>
	struct basic_hasher<H, T>
	{
		__forceinline constexpr static void extend( H& out, const T& value ) noexcept
		{
			std::apply( [ & ] ( const auto&... elements ) { return extend_hash<H>( out, elements... ); }, value );
		}
		__forceinline constexpr H operator()( const T& value ) const noexcept
		{
			return std::apply( [ & ] ( const auto&... elements ) { return make_hash<H>( elements... ); }, value );
		}
	};

	// Overload for std::monostate.
	//
	template<typename H>
	struct basic_hasher<H, std::monostate>
	{
		__forceinline constexpr static void extend( H&, std::monostate ) noexcept {}
		__forceinline constexpr H operator()( std::monostate ) const noexcept { return H{}; }
	};

	// Overload default instance.
	//
	template<typename H>
	struct basic_hasher<H, void>
	{
		template<typename T>
		__forceinline constexpr static void extend( H& hasher, const T& obj ) noexcept
		{
			extend_hash<H, T>( hasher, obj );
		}
		template<typename T>
		__forceinline constexpr H operator()( const T& obj ) const noexcept
		{
			return make_hash<H>( obj );
		}
	};


	// Hasher redirect with the default.
	//
	template<typename T = void> 
	using hasher = basic_hasher<hash_t, T>;

	// Disjunction of all possible conversions.
	//
	template<typename T>
	concept Hashable = requires( T v ) { !std::is_void_v<decltype( hasher<>{}( v ) )>; };
};

// Redirect from std::hash.
//
namespace std
{
	template<typename T> requires ( xstd::CustomHashable<T> || xstd::CustomHashExtendable<T, xstd::hash_t> )
	struct hash<T>
	{
		__forceinline constexpr size_t operator()( const T& value ) const noexcept
		{
			return xstd::make_hash( value ).as64();
		}
	};
};