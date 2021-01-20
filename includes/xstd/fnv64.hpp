#pragma once
#include <string>
#include <array>
#include <functional>
#include <cstring>
#include "intrinsics.hpp"
#include "type_helpers.hpp"

namespace xstd
{
	// Defines a 64-bit hash type based on FNV-1.
	//
	struct fnv64_hash_t
	{
		// Magic constants for 64-bit FNV-1 .
		//
		using value_t = uint64_t;
		static constexpr value_t default_seed = { 0xCBF29CE484222325 };
		static constexpr value_t prime =        { 0x00000100000001B3 };

		// Current value of the hash.
		//
		value_t value;

		// Construct a new hash from an optional seed of 64-bit value.
		//
		constexpr fnv64_hash_t( uint64_t seed64 = default_seed ) noexcept
			: value{ seed64 } {}

		// Appends the given array of bytes into the hash value.
		//
		template<typename T>
		constexpr void add_bytes( const T& data ) noexcept
		{
			using array_t = std::array<uint8_t, sizeof( T )>;

			if ( std::is_constant_evaluated() && !std::is_same_v<array_t, T> )
			{
				if constexpr ( Bitcastable<T> )
				{
					auto byte_view = bit_cast<array_t>( data );

					for ( uint8_t byte : byte_view )
					{
						value ^= byte;
						value *= prime;
					}
					return;
				}
				unreachable();
			}
			for ( size_t n = 0; n != sizeof( T ); n++ )
			{
				value ^= ( ( const uint8_t* ) &data )[ n ];
				value *= prime;
			}
		}

		// Implicit conversion to 64-bit values.
		//
		constexpr uint64_t as64() const noexcept { return value; }
		constexpr operator uint64_t() const noexcept { return as64(); }

		// Conversion to human-readable format.
		//
		std::string to_string() const
		{
			char str[ 16 + 3 ] = {};
			snprintf( str, std::size( str ), XSTD_CSTR( "0x%llx" ), value );
			return str;
		}

		// Basic comparison operators.
		//
		constexpr bool operator<( const fnv64_hash_t& o ) const noexcept { return value < o.value; }
		constexpr bool operator==( const fnv64_hash_t& o ) const noexcept { return value == o.value; }
		constexpr bool operator!=( const fnv64_hash_t& o ) const noexcept { return value != o.value; }
	};
};

// Make it std::hashable.
//
namespace std
{
	template<>
	struct hash<xstd::fnv64_hash_t>
	{
		size_t operator()( const xstd::fnv64_hash_t& value ) const { return ( size_t ) value.as64(); }
	};
};