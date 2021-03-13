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
		constexpr fnv64_hash_t( uint64_t seed64 = default_seed ) noexcept
			: value{ seed64 } {}

		// Appends the given array of bytes into the hash value.
		//
		constexpr void add_bytes( const uint8_t* data, size_t n )
		{
#ifndef __INTELLISENSE__
			uint64_t tmp = value;
			while( n-- )
			{
				tmp ^= ( uint64_t ) *data++;
				tmp *= prime;
			}
			value = tmp;
#endif
		}

		// Appends the given trivial value as bytes into the hash value.
		//
		template<typename T>
		constexpr void add_bytes( const T& data ) noexcept
		{
			if ( std::is_constant_evaluated() )
			{
				using array_t = std::array<uint8_t, sizeof( T )>;
				if constexpr ( std::is_same_v<array_t, T> )
					add_bytes( data.data(), data.size() );
				else if constexpr ( Bitcastable<T> )
					add_bytes( bit_cast<array_t>( data ) );
				else
					unreachable();
			}
			else
			{
				add_bytes( ( const uint8_t* ) &data, sizeof( T ) );
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
			snprintf( str, std::size( str ), "0x%llx", value );
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