#pragma once
#include <string>
#include <array>
#include <functional>
#include <cstring>
#include "intrinsics.hpp"
#include "type_helpers.hpp"
#include "bitwise.hpp"
#include "hexdump.hpp"

namespace xstd
{
	// Defines a generic CRC type for reflect in = 1, reflect out = 1.
	//
	template<typename U, typename I, U rpoly, U seed>
	struct crc
	{
		static constexpr auto lookup_table = [ ] ()
		{
			std::array<U, 256> table = {};

			for ( size_t n = 0; n <= 0xFF; n++ )
			{
				U crc = U( n );
				for ( size_t i = 0; i != 8; i++ )
				{
					if ( crc & U( 1 ) )
						crc = ( crc >> U( 1 ) ) ^ rpoly;
					else
						crc >>= U( 1 );
				}
				table[ n ] = ~crc;
			}
			return table;
		}();

		// Magic constants for 32-bit CRC.
		//
		using value_type =   U;
		static constexpr value_type default_seed = seed;
		static constexpr value_type polynomial =   rpoly;

		// Current value of the hash.
		//
		value_type value;

		// Construct a new hash from an optional seed of 64-bit value.
		//
		constexpr crc( U seed_n = default_seed ) noexcept
			: value{ seed_n } {}

		// Appends the given array of bytes into the hash value.
		//
		__forceinline constexpr void add_bytes( const uint8_t* data, size_t n )
		{
#ifndef __INTELLISENSE__
			U crc = ~value;
			while( n-- )
				crc = ( ~lookup_table[ U( *data++ ) ^ ( crc & U( 0xFF ) ) ] ) ^ ( crc >> U( 8 ) );
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
		constexpr value_type digest() const noexcept { return value; }

		// Explicit conversions.
		//
		constexpr uint32_t as32() const noexcept { return ( uint32_t ) digest(); }
		constexpr uint64_t as64() const noexcept { return ( uint64_t ) digest(); }

		// Implicit conversions.
		//
		constexpr operator value_type() const noexcept { return digest(); }

		// Conversion to human-readable format.
		//
		std::string to_string() const { return fmt::const_hex_dump( bswap( digest() ) ); }

		// Basic comparison operators.
		//
		constexpr bool operator<( const crc& o ) const noexcept { return digest() < o.digest(); }
		constexpr bool operator==( const crc& o ) const noexcept { return digest() == o.digest(); }
		constexpr bool operator!=( const crc& o ) const noexcept { return digest() != o.digest(); }
	};

	// Define common implementations.
	//
	using crc8 =     crc<uint8_t,  int8_t,  0xAB,               0xFF>;
	using crc16 =    crc<uint16_t, int16_t, 0xA6BC,             0xFFFF>;
	using crc32 =    crc<uint32_t, int32_t, 0xEDB88320,         0>;
	using crc32k =   crc<uint32_t, int32_t, 0x992C1A4C,         0>;
	using crc64 =    crc<uint64_t, int64_t, 0xD800000000000000, 0>;
	using crc64xz =  crc<uint64_t, int64_t, 0xC96C5795D7870F42, 0>;

#if __clang__
	#ifndef __INTELLISENSE__
		#define _intn(n) unsigned _ExtInt(n), _ExtInt(n)
	#else
		#define _intn(n) uint32_t, int32_t
	#endif
	using crc12 = crc<_intn(12), 0xF01,      0>;
	using crc17 = crc<_intn(17), 0x1B42D,    0>;
	using crc21 = crc<_intn(21), 0x132281,   0>;
	using crc24 = crc<_intn(24), 0xC60001,   0>;
	using crc30 = crc<_intn(30), 0x38E74301, 0>;
	#undef _intn
#endif
};

// Make it std::hashable.
//
namespace std
{
	template<typename U, typename I, U rpoly, U seed>
	struct hash<xstd::crc<U, I, rpoly, seed>>
	{
		size_t operator()( const xstd::crc<U, I, rpoly, seed>& value ) const { return ( size_t ) value.as64(); }
	};
};