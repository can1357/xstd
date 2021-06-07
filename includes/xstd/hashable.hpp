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

	// XSTD hashable types implement [hash_t T::hash() const];
	//
	template<typename T>
	concept CustomHashable = requires( T v ) { v.hash(); };

	// Checks if std::hash is specialized to hash the type.
	//
	template<typename T>
	concept StdHashable = requires( T v ) { std::hash<T>{}( v ); };

	// Implement hash combination details.
	//
	template<typename H, typename T = void>
	struct basic_hasher;
	namespace impl
	{
		static constexpr uint64_t hash_combination_keys[] =
		{
			0x113769B23C5F1, 0x81BF0FFFFED, 0x426DE69929, 0x2540BE3DF
		};
		static constexpr uint64_t uhash_combination_key = 0xF91A2E471B43FFBF;

		template<typename H, typename F, typename... Tx>
		static constexpr H smart_combine( F&& combine, const Tx&... values )
		{
			if constexpr ( sizeof...( Tx ) > 1 )
			{
				constexpr bool has_trivial = ( Trivial<Tx> || ... );
				constexpr bool has_complex = ( (!Trivial<Tx>) || ... );

				H hc = {}, ht = {};
				xstd::visit_tuple( [ & ] <typename T> ( const T & el )
				{
					if constexpr ( Trivial<T> ) ht.add_bytes( el );
					else                        hc = H{ combine( hc, basic_hasher<H, T>{}( el ) ) };
				}, std::tie( values... ) );

				if constexpr ( !has_trivial )
					return hc;
				else if constexpr ( !has_complex )
					return ht;
				else
					return combine( ht, hc );
			}
			else if constexpr ( sizeof...( Tx ) )
			{
				return basic_hasher<H, Tx...>{}( values... );
			}
			else
			{
				return H{};
			}
		}
	};

	// Used to combine two hashes of arbitrary size.
	//
	__forceinline static constexpr uint64_t combine_hash( uint64_t a, uint64_t b )
	{
		uint64_t key = impl::hash_combination_keys[ a & 3 ];
		uint64_t x = key * ( impl::uhash_combination_key + ( a ^ ~b ) );
		return rotlq( x, ( key + x ) & 63 );
	}
	__forceinline static constexpr uint64_t combine_unordered_hash( uint64_t a, uint64_t b )
	{
		return a + b + impl::uhash_combination_key;
	}

	// Define basic hasher.
	//
	template<typename H, typename T>
	struct basic_hasher
	{
		constexpr auto operator()( const T& value ) const noexcept
		{
			// If object is hashable via ::hash(), use as is.
			//
			if constexpr ( CustomHashable<const T&> )
			{
				if ( !std::is_constant_evaluated() || is_constexpr( [ &value ] () { value.hash(); } ) )
				{
					return H{ value.hash() };
				}
			}
			// If STL container or array, hash each element and add container information.
			//
			else if constexpr ( Iterable<const T&> )
			{
				using value_type = std::decay_t<iterable_val_t<const T&>>;

				if constexpr ( !std::is_void_v<decltype( basic_hasher<H, value_type>{}( std::declval<value_type&>() ) ) > )
				{
					if constexpr ( Trivial<value_type> )
					{
						H hash = {};
						for ( const auto& entry : value )
							hash.add_bytes( entry );
						return hash;
					}
					else
					{
						H hash = {};
						for ( const auto& entry : value )
							hash = H{ combine_hash( hash, basic_hasher<H, value_type>{}( entry ) ) };
						return hash;
					}
				}
			}
			// If hash, combine with default seed.
			//
			else if constexpr ( Same<T, H> )
			{
				return H{ combine_hash( value, {} ) };
			}
			// If register sized integral/pointer type, use a special hasher.
			//
			else if constexpr ( Pointer<T> || Same<T, any_ptr> || ( Integral<T> && ( sizeof( T ) == 8 || sizeof( T ) == 4 ) ) || Enum<T> )
			{
#if XSTD_HW_CRC32C
				H res{};
				res.add_bytes( value );
				return res;
#else
				constexpr uint64_t prime = sizeof( T ) == 8 ? 0x613b62c597707bb : 0x277a4fb3943fef;
				constexpr uint64_t type_key = type_tag<T>::hash();

				// Mix the value and return.
				//
				uint64_t x = ( type_key ^ uint64_t( value ) );
				return H{ rotl( prime * x, 27 ) };
#endif
			}
			// If trivial type, hash each byte.
			//
			else if constexpr ( Trivial<T> )
			{
				H hash = {};
				hash.add_bytes( value );
				return hash;
			}
			// If hashable using std::hash<>, redirect.
			//
			else if constexpr ( StdHashable<T> )
			{
				return H{ std::hash<T>{}( value ) };
			}
			// If tiable, redirect.
			//
			else if constexpr ( Tiable<T> )
			{
				auto tuple = make_mutable( value ).tie();
				return basic_hasher<H, decltype( tuple )>{}( std::move( tuple ) );
			}

			// Fail.
			//
			unreachable();
		}
	};

	// Vararg hasher wrapper that should be used to create hashes from N values.
	//
	template<typename H, typename... Tx>
	__forceinline static constexpr H make_hash( const Tx&... values )
	{
		return impl::smart_combine<H>( [ ] ( auto&& a, auto&& b ) -> H { return combine_hash( a, b ); }, values... );
	}
	template<typename H, typename... Tx>
	__forceinline static constexpr H make_unordered_hash( const Tx&... values )
	{
		return impl::smart_combine<H>( [ ] ( auto&& a, auto&& b ) -> H { return combine_unordered_hash( a, b ); }, values... );
	}
	template<typename... Tx>
	__forceinline static constexpr hash_t make_hash( const Tx&... el )
	{
		return make_hash<hash_t, Tx...>( el... );
	}
	template<typename... Tx>
	__forceinline static constexpr hash_t make_unordered_hash( const Tx&... el )
	{
		return make_unordered_hash<hash_t, Tx...>( el... );
	}

	// Overload for std::reference_wrapper.
	//
	template<typename H, typename T>
	struct basic_hasher<H, std::reference_wrapper<T>>
	{
		__forceinline constexpr hash_t operator()( const std::reference_wrapper<T>& value ) const noexcept
		{
			if ( value ) return make_hash<H>( value.get() );
			else         return type_tag<T>::hash() ^ 0xac07ef2ee5fcaa79;
		}
	};

	// Overload for std::optional.
	//
	template<typename H, typename T>
	struct basic_hasher<H, std::optional<T>>
	{
		__forceinline constexpr hash_t operator()( const std::optional<T>& value ) const noexcept
		{
			if ( value ) return make_hash<H>( *value );
			else         return type_tag<T>::hash() ^ 0x6a477a8b10f59225;
		}
	};

	// Overload for std::variant.
	//
	template<typename H, typename... T>
	struct basic_hasher<H, std::variant<T...>>
	{
		__forceinline constexpr hash_t operator()( const std::variant<T...>& value ) const noexcept
		{
			hash_t res = std::visit( [ ] ( auto&& arg ) { return make_hash<H>( arg ); }, value );
			res.add_bytes( value.index() );
			return res;
		}
	};

	// Overload for XSTD smart pointers.
	//
	template<typename H, typename T>
	struct basic_hasher<H, shared<T>>
	{
		__forceinline constexpr hash_t operator()( const shared<T>& value ) const noexcept
		{
			return make_hash<H>( &value->entry );
		}
	};
	template<typename H, typename T>
	struct basic_hasher<H, weak<T>>
	{
		__forceinline constexpr hash_t operator()( const weak<T>& value ) const noexcept
		{
			return make_hash<H>( &value->entry );
		}
	};

	// Overload for STL smart pointers.
	//
	template<typename H, typename T>
	struct basic_hasher<H, std::shared_ptr<T>>
	{
		__forceinline constexpr hash_t operator()( const std::shared_ptr<T>& value ) const noexcept
		{
			return make_hash<H>( value.get() );
		}
	};
	template<typename H, typename T>
	struct basic_hasher<H, std::weak_ptr<T>>
	{
		__forceinline constexpr hash_t operator()( const std::weak_ptr<T>& value ) const noexcept
		{
			return make_hash<H>( value.get() );
		}
	};
	template<typename H, typename T, typename Dx>
	struct basic_hasher<H, std::unique_ptr<T, Dx>>
	{
		__forceinline constexpr hash_t operator()( const std::unique_ptr<T, Dx>& value ) const noexcept
		{
			return make_hash<H>( value.get() );
		}
	};

	// Overload for contigious trivial iterables.
	//
	template<typename H, ContiguousIterable T> requires Trivial<iterable_val_t<T>>
	struct basic_hasher<H, T>
	{
		using U = iterable_val_t<T>;

		__forceinline constexpr hash_t operator()( const T& value ) const noexcept
		{
			// If C array with char/wchar element, we'll skip last one despite the lameness.
			//
			if constexpr ( std::is_array_v<T> && ( Same<U, char> || Same<U, char8_t> || Same<U, wchar_t> || Same<U, char16_t> ) )
			{
				return make_hash<H>( std::span{ std::begin( value ), std::end( value ) - 1 } );
			}
			else
			{
				if ( std::is_constant_evaluated() )
				{
					H hash = {};
					for ( const auto& entry : value )
						hash.add_bytes( entry );
					return hash;
				}
				else
				{
					auto* first = &*std::begin( value );
					H hash = {};
					hash.add_bytes( ( const uint8_t* ) first, sizeof( U ) * std::size( value ) );
					return hash;
				}
			}
		}
	};

	// Overload for C strings.
	//
	template<typename H, typename T> requires ( Same<T, char> || Same<T, char8_t> || Same<T, wchar_t> || Same<T, char16_t> )
	struct basic_hasher<H, const T*>
	{
		__forceinline constexpr hash_t operator()( const T& value ) const noexcept
		{
			return make_hash<H>( std::basic_string_view<T>{ value } );
		}
	};
	template<typename H, typename T> requires ( Same<T, char> || Same<T, char8_t> || Same<T, wchar_t> || Same<T, char16_t> )
	struct basic_hasher<H, T*>
	{
		__forceinline constexpr hash_t operator()( const T& value ) const noexcept
		{
			return make_hash<H>( std::basic_string_view<T>{ value } );
		}
	};

	// Overload for std::pair / std::tuple.
	//
	template<typename H, Tuple T>
	struct basic_hasher<H, T>
	{
		__forceinline constexpr hash_t operator()( const T& obj ) const noexcept
		{
			if constexpr ( std::tuple_size_v<T> != 0 )
				return std::apply( [ ] ( auto&&... params ) { return make_hash<H>( params... ); }, obj );
			else 
				return type_tag<T>::hash();
		}
	};

	// Overload for std::monostate.
	//
	template<typename H>
	struct basic_hasher<H, std::monostate>
	{
		__forceinline constexpr hash_t operator()( const std::monostate& ) const noexcept
		{
			return type_tag<std::monostate>::hash();
		}
	};

	// Overload default instance.
	//
	template<typename H>
	struct basic_hasher<H, void>
	{
		template<typename T>
		__forceinline constexpr size_t operator()( const T& obj ) const noexcept
		{
			return ( size_t ) make_hash<H>( obj ).as64();
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
	template<xstd::CustomHashable T>
	struct hash<T>
	{
		__forceinline constexpr size_t operator()( const T& value ) const noexcept
		{
			return xstd::hash_t{ value.hash() }.as64();
		}
	};
};