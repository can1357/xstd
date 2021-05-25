#pragma once
#include <algorithm>
#include <variant>
#include <optional>
#include "intrinsics.hpp"
#include "type_helpers.hpp"
#include "formatting.hpp"
#include "fnv64.hpp"
#include "shared.hpp"

// [Configuration]
// XSTD_DEFAULT_HASHER: If set, changes the type of default hash_t.
//
#ifndef XSTD_DEFAULT_HASHER
	#define XSTD_DEFAULT_HASHER xstd::fnv64
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

	namespace impl
	{
		static constexpr uint64_t hash_combination_keys[] =
		{
			0xf8f2f808dfa2a53d, 0x8c67174bea0d0e12,
			0xb49abcd9cdd750d7, 0x81f82267dd8343db,
			0x7381f6d4dc4fe661, 0xd2c067d60e9f4734,
			0x04ecc5fce7ce1e6f,	0xbc82997a673d1ca3,
		};
		static constexpr uint64_t uhash_combination_key = 0xa990854d1e718fa9;
	};

	// Used to combine two hashes of arbitrary size.
	//
	__forceinline static constexpr hash_t combine_hash( hash_t a, const hash_t& b )
	{
		a.value *= b.value ^ rotlq( impl::hash_combination_keys[ a.value % 7 ], b.value & 63 );
		return a;
	}
	__forceinline static constexpr hash_t combine_unordered_hash( hash_t a, const hash_t& b )
	{
		a.value += b.value + impl::uhash_combination_key;
		return a;
	}

	// Define basic hasher.
	//
	template<typename T = void>
	struct hasher
	{
		constexpr auto operator()( const T& value ) const noexcept
		{
			// If object is hashable via ::hash(), use as is.
			//
			if constexpr ( CustomHashable<const T&> )
			{
				if ( !std::is_constant_evaluated() || is_constexpr( [ &value ] () { value.hash(); } ) )
				{
					return hash_t{ value.hash() };
				}
			}
			// If STL container or array, hash each element and add container information.
			//
			else if constexpr ( Iterable<const T&> )
			{
				using value_type = std::decay_t<iterable_val_t<const T&>>;

				if constexpr ( !std::is_void_v<decltype( hasher<value_type>{}( std::declval<value_type&>() ) ) > )
				{
					if constexpr ( Trivial<value_type> )
					{
						hash_t hash = {};
						for ( const auto& entry : value )
							hash.add_bytes( entry );
						return hash;
					}
					else
					{
						hash_t hash = {};
						for ( const auto& entry : value )
							hash = combine_hash( hash, hasher<value_type>{}( entry ) );
						return hash;
					}
				}
			}
			// If hash, combine with default seed.
			//
			else if constexpr ( Same<T, hash_t> )
			{
				return combine_hash( value, {} );
			}
			// If pointer, use a special hasher.
			//
			else if constexpr ( Pointer<T> || Same<T, any_ptr> )
			{
				// Extract the identifiers, most systems use 48 or 57 bit address spaces in reality with rest being sign extension.
				//
				uint64_t v0 = uint64_t( value ) & 0xFFF;
				uint64_t v1 = ( uint64_t( value ) >> ( 12 + 9 * 0 ) ) & 0x3FFFF;

				// Combine the values with the key and return.
				//
				uint64_t res = 0x87c37b91114253d5 * rotr( uint64_t( value ), 41 );
				return hash_t{ ~( ( res ^ uint32_t( v0 * 0x1b873593 ) ) - v1 ) };
			}
			// If register sized integral type, use a special hasher.
			//
			else if constexpr ( Integral<T> && ( sizeof( T ) == 8 || sizeof( T ) == 4 ) )
			{
				// Pick a random key per size to prevent collisions accross different types.
				//
				uint64_t res = sizeof( T ) == 8 ? 0x350dfbdfde7d6d41 : 0xfe89825303c1cce3;

				// Combine the value with the key.
				//
				res *= ( impl::uhash_combination_key - int64_t( value ) );
				
				// Rotate, combine and return.
				//
				res -= rotl( value, res % 37 );
				return hash_t{ res };
			}
			// If trivial type, hash each byte.
			//
			else if constexpr ( Trivial<T> )
			{
				hash_t hash = {};
				hash.add_bytes( value );
				return hash;
			}
			// If hashable using std::hash<>, redirect.
			//
			else if constexpr ( StdHashable<T> )
			{
				return hash_t{ std::hash<T>{}( value ) };
			}
			// If tiable, redirect.
			//
			else if constexpr ( Tiable<T> )
			{
				auto tuple = make_mutable( value ).tie();
				return hasher<decltype( tuple )>{}( std::move( tuple ) );
			}

			// Fail.
			//
			unreachable();
		}
	};

	// Vararg hasher wrapper that should be used to create hashes from N values.
	//
	template<typename T>
	__forceinline static constexpr hash_t make_hash( const T& value ) { return hasher<T>{}( value ); }
	template<typename C, typename... T>
	__forceinline static constexpr hash_t make_hash( const C& current, T&&... rest )
	{
		hash_t hash = make_hash( current );
		( ( hash = combine_hash( hash, make_hash( rest ) ) ), ... );
		return hash;
	}

	// Vararg hasher wrapper that should be used to create hashes from N values, explicitly ignoring the order.
	//
	template<typename T>
	__forceinline static constexpr hash_t make_unordered_hash( const T& value ) { return hasher<T>{}( value ); }
	template<typename C, typename... T>
	__forceinline static constexpr hash_t make_unordered_hash( const C& current, T&&... rest )
	{
		hash_t hash = make_unordered_hash( current );
		( ( hash = combine_unordered_hash( hash, make_unordered_hash( rest ) ) ), ... );
		return hash;
	}

	// Overload for std::reference_wrapper.
	//
	template<typename T>
	struct hasher<std::reference_wrapper<T>>
	{
		__forceinline constexpr hash_t operator()( const std::reference_wrapper<T>& value ) const noexcept
		{
			if ( value ) return make_hash( value.get() );
			else         return type_tag<T>::hash() ^ 0xac07ef2ee5fcaa79;
		}
	};

	// Overload for std::optional.
	//
	template<typename T>
	struct hasher<std::optional<T>>
	{
		__forceinline constexpr hash_t operator()( const std::optional<T>& value ) const noexcept
		{
			if ( value ) return make_hash( *value );
			else         return type_tag<T>::hash() ^ 0x6a477a8b10f59225;
		}
	};

	// Overload for std::variant.
	//
	template<typename... T>
	struct hasher<std::variant<T...>>
	{
		__forceinline constexpr hash_t operator()( const std::variant<T...>& value ) const noexcept
		{
			hash_t res = std::visit( [ ] ( auto&& arg ) { return make_hash( arg ); }, value );
			res.add_bytes( value.index() );
			return res;
		}
	};

	// Overload for XSTD smart pointers.
	//
	template<typename T>
	struct hasher<shared<T>>
	{
		__forceinline constexpr hash_t operator()( const shared<T>& value ) const noexcept
		{
			return make_hash( &value->entry );
		}
	};
	template<typename T>
	struct hasher<weak<T>>
	{
		__forceinline constexpr hash_t operator()( const weak<T>& value ) const noexcept
		{
			return make_hash( &value->entry );
		}
	};

	// Overload for STL smart pointers.
	//
	template<typename T>
	struct hasher<std::shared_ptr<T>>
	{
		__forceinline constexpr hash_t operator()( const std::shared_ptr<T>& value ) const noexcept
		{
			return make_hash( value.get() );
		}
	};
	template<typename T>
	struct hasher<std::weak_ptr<T>>
	{
		__forceinline constexpr hash_t operator()( const std::weak_ptr<T>& value ) const noexcept
		{
			return make_hash( value.get() );
		}
	};
	template<typename T, typename Dx>
	struct hasher<std::unique_ptr<T, Dx>>
	{
		__forceinline constexpr hash_t operator()( const std::unique_ptr<T, Dx>& value ) const noexcept
		{
			return make_hash( value.get() );
		}
	};

	// Overload for std::pair / std::tuple.
	//
	template<Tuple T>
	struct hasher<T>
	{
		__forceinline constexpr hash_t operator()( const T& obj ) const noexcept
		{
			if constexpr ( std::tuple_size_v<T> != 0 )
				return std::apply( [ ] ( auto&&... params ) { return make_hash( params... ); }, obj );
			else 
				return type_tag<T>::hash();
		}
	};

	// Overload for std::monostate.
	//
	template<>
	struct hasher<std::monostate>
	{
		__forceinline constexpr hash_t operator()( const std::monostate& ) const noexcept
		{
			return type_tag<std::monostate>::hash();
		}
	};

	// Overload default instance.
	//
	template<>
	struct hasher<void>
	{
		template<typename T>
		__forceinline constexpr size_t operator()( const T& obj ) const noexcept
		{
			return ( size_t ) make_hash( obj ).as64();
		}
	};

	// Disjunction of all possible conversions.
	//
	template<typename T>
	concept Hashable = requires( T v ) { !std::is_void_v<decltype( hasher<>{}( v ) )>; };

	// Implement a hasher that hashes pointers by their contents rather than the pointer themselves.
	//
	struct dereferencing_hasher
	{
		template<typename T>
		__forceinline constexpr size_t operator()( const T& value ) const noexcept
		{
			if constexpr( Dereferencable<T> )
				return make_hash( *value ).as64();
			else
				return make_hash( value ).as64();
		}
	};
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