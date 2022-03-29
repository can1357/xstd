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
	// Defines a generic FNV-1A hasher.
	//
	template<typename V, V Seed, V Prime>
	struct fnv1a
	{
		using value_type = V;
		static constexpr V default_seed = Seed;
		static constexpr V prime =        Prime;

		// Current value of the hash.
		//
		V value;

		// Construct a new hash from an optional seed of 64-bit value.
		//
		constexpr fnv1a( uint64_t seed64 = default_seed ) noexcept
			: value{ V( seed64 ) } {}

		// Appends the given array of bytes into the hash value.
		//
		FORCE_INLINE constexpr void add_bytes( const uint8_t* data, size_t n )
		{
			V tmp = value;
			while( n-- )
			{
				tmp ^= ( V ) *data++;
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
		constexpr V digest() const noexcept { return value; }

		// Explicit conversions.
		//
		constexpr uint64_t as64() const noexcept { return ( uint64_t ) digest(); }
		constexpr uint32_t as32() const noexcept { return ( uint32_t ) digest(); }

		// Implicit conversions.
		//
		constexpr operator value_type() const noexcept { return digest(); }

		// Conversion to human-readable format.
		//
		std::string to_string() const { return fmt::as_hex_string( bswap( digest() ) ); }

		// Basic comparison operators.
		//
		constexpr bool operator<( const fnv1a& o ) const noexcept { return digest() < o.digest(); }
		constexpr bool operator==( const fnv1a& o ) const noexcept { return digest() == o.digest(); }
		constexpr bool operator!=( const fnv1a& o ) const noexcept { return digest() != o.digest(); }
	};

	// Define default FNV64 and FNV32 types.
	//
	using fnv64 = fnv1a<uint64_t, 0xCBF29CE484222325, 0x00000100000001B3>;
	using fnv32 = fnv1a<uint32_t, 0x811C9DC5,         0x01000193>;
};

// Make it std::hashable.
//
namespace std
{
	template<typename V, V Seed, V Prime>
	struct hash<xstd::fnv1a<V, Seed, Prime>>
	{
		constexpr size_t operator()( const xstd::fnv1a<V, Seed, Prime>& value ) const { return ( size_t ) value.as64(); }
	};
};