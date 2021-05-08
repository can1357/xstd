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
	// Defines a 32-bit hash type based on CRC.
	//
	struct crc32
	{
		// Magic constants for 32-bit CRC.
		//
		using value_type = uint32_t;
		static constexpr uint32_t default_seed = { 0 };
		static constexpr uint32_t polynomial =   { 0xEDB88320 };

		// Current value of the hash.
		//
		uint32_t value;

		// Construct a new hash from an optional seed of 64-bit value.
		//
		constexpr crc32( uint32_t seed32 = default_seed ) noexcept
			: value{ seed32 } {}

		// Appends the given array of bytes into the hash value.
		//
		constexpr void add_bytes( const uint8_t* data, size_t n )
		{
#ifndef __INTELLISENSE__
			uint32_t crc = ~value;
			while( n-- )
			{
				crc ^= *data++;
				for ( size_t j = 0; j != 8; j++ )
				{
					uint32_t mask = ( uint32_t ) ( -int32_t( crc & 1 ) );
					crc = ( crc >> 1 ) ^ ( polynomial & mask );
				}
			}
			value = ~crc;
#endif
		}

		// Appends the given trivial value as bytes into the hash value.
		//
		template<typename T>
		__forceinline constexpr void add_bytes( const T& data ) noexcept
		{
			if ( std::is_constant_evaluated() )
			{
				using array_t = std::array<uint8_t, sizeof( T )>;
#ifndef __INTELLISENSE__
				array_t arr = bit_cast< array_t >( data );
				add_bytes( arr.data(), arr.size() );
#endif
			}
			else
			{
				add_bytes( ( const uint8_t* ) &data, sizeof( T ) );
			}
		}

		// Finalization of the hash.
		//
		constexpr void finalize() noexcept {}
		constexpr uint32_t digest() const noexcept { return value; }

		// Explicit conversions.
		//
		constexpr uint32_t as32() const noexcept { return digest(); }
		constexpr uint64_t as64() const noexcept { return digest(); }

		// Implicit conversions.
		//
		constexpr operator uint32_t() const noexcept { return as32(); }

		// Conversion to human-readable format.
		//
		std::string to_string() const { return fmt::const_hex_dump( bswap( digest() ) ); }

		// Basic comparison operators.
		//
		constexpr bool operator<( const crc32& o ) const noexcept { return digest() < o.digest(); }
		constexpr bool operator==( const crc32& o ) const noexcept { return digest() == o.digest(); }
		constexpr bool operator!=( const crc32& o ) const noexcept { return digest() != o.digest(); }
	};
};

// Make it std::hashable.
//
namespace std
{
	template<>
	struct hash<xstd::crc32>
	{
		size_t operator()( const xstd::crc32& value ) const { return ( size_t ) value.as32(); }
	};
};