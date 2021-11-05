#pragma once
#include <string>
#include <array>
#include <functional>
#include <cstring>
#include "intrinsics.hpp"
#include "type_helpers.hpp"
#include "hexdump.hpp"

namespace xstd
{
	// Defines a 64-bit hash type based on FNV-1.
	//
	struct fnv64
	{
		// Magic constants for 64-bit FNV-1.
		//
		using value_type = uint64_t;
		static constexpr uint64_t default_seed = { 0xCBF29CE484222325 };
		static constexpr uint64_t prime =        { 0x00000100000001B3 };

		// Current value of the hash.
		//
		uint64_t value;

		// Construct a new hash from an optional seed of 64-bit value.
		//
		constexpr fnv64( uint64_t seed64 = default_seed ) noexcept
			: value{ seed64 } {}

		// Appends the given array of bytes into the hash value.
		//
		FORCE_INLINE constexpr void add_bytes( const uint8_t* data, size_t n )
		{
			uint64_t tmp = value;
			while( n-- )
			{
				tmp ^= ( uint64_t ) *data++;
				tmp *= prime;
			}
			value = tmp;
		}

		// Appends the given trivial value as bytes into the hash value.
		//
		template<typename T>
		FORCE_INLINE constexpr void add_bytes( const T& data ) noexcept
		{
			if ( std::is_constant_evaluated() )
			{
				using array_t = std::array<uint8_t, sizeof( T )>;
				array_t arr = bit_cast< array_t >( data );
				add_bytes( arr.data(), arr.size() );
			}
			else
			{
				add_bytes( ( const uint8_t* ) &data, sizeof( T ) );
			}
		}

		// Finalization of the hash.
		//
		constexpr void finalize() noexcept {}
		constexpr uint64_t digest() const noexcept { return value; }

		// Explicit conversions.
		//
		constexpr uint64_t as64() const noexcept { return digest(); }
		constexpr uint32_t as32() const noexcept { return digest() & 0xFFFFFFFF; }

		// Implicit conversions.
		//
		constexpr operator uint64_t() const noexcept { return as64(); }

		// Conversion to human-readable format.
		//
		std::string to_string() const { return fmt::as_hex_string( bswap( digest() ) ); }

		// Basic comparison operators.
		//
		constexpr bool operator<( const fnv64& o ) const noexcept { return digest() < o.digest(); }
		constexpr bool operator==( const fnv64& o ) const noexcept { return digest() == o.digest(); }
		constexpr bool operator!=( const fnv64& o ) const noexcept { return digest() != o.digest(); }
	};
};

// Make it std::hashable.
//
namespace std
{
	template<>
	struct hash<xstd::fnv64>
	{
		size_t operator()( const xstd::fnv64& value ) const { return ( size_t ) value.as64(); }
	};
};