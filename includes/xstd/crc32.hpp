#pragma once
#include <string>
#include <array>
#include <functional>
#include <cstring>
#include "intrinsics.hpp"
#include "type_helpers.hpp"

namespace xstd
{
	// Defines a 32-bit hash type based on CRC.
	//
	struct crc32_hash_t
	{
		// Magic constants for 32-bit CRC.
		//
		using value_t = uint32_t;
		static constexpr value_t default_seed = { 0 };
		static constexpr value_t polynomial =   { 0xEDB88320 };

		// Current value of the hash.
		//
		value_t value;

		// Construct a new hash from an optional seed of 64-bit value.
		//
		constexpr crc32_hash_t( uint64_t seed64 = default_seed ) noexcept
			: value{ seed64 } {}

		// Appends the given array of bytes into the hash value.
		//
		constexpr void add_bytes( const uint8_t* data, size_t n )
		{
			value_t crc = ~value;
			while( n-- )
			{
				crc ^= *data++;
				for ( size_t j = 0; j != 8; j++ )
				{
					value_t mask = -( crc & 1 );
					crc = ( crc >> 1 ) ^ ( polynomial & mask );
				}
			}
			value = ~crc;
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
					add_bytes( bit_cast< array_t >( data ) );
				else
					unreachable();
			}
			else
			{
				add_bytes( ( const uint8_t* ) &data, sizeof( T ) );
			}
		}

		// Implicit conversion to 32-bit values.
		//
		constexpr uint32_t as32() const noexcept { return value; }
		constexpr operator uint32_t() const noexcept { return as32(); }

		// Conversion to human-readable format.
		//
		std::string to_string() const
		{
			char str[ 8 + 3 ] = {};
			snprintf( str, std::size( str ), XSTD_CSTR( "0x%x" ), value );
			return str;
		}

		// Basic comparison operators.
		//
		constexpr bool operator<( const crc32_hash_t& o ) const noexcept { return value < o.value; }
		constexpr bool operator==( const crc32_hash_t& o ) const noexcept { return value == o.value; }
		constexpr bool operator!=( const crc32_hash_t& o ) const noexcept { return value != o.value; }
	};
};

// Make it std::hashable.
//
namespace std
{
	template<>
	struct hash<xstd::crc32_hash_t>
	{
		size_t operator()( const xstd::crc32_hash_t& value ) const { return ( size_t ) value.as32(); }
	};
};