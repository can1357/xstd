#pragma once
#include <string>
#include <array>
#include <functional>
#include <numeric>
#include <cstring>
#include "intrinsics.hpp"
#include "type_helpers.hpp"
#include "hexdump.hpp"

namespace xstd
{
	// Defines a 160-bit hash type based on SHA-256.
	//
	struct sha1
	{
		struct iv_tag {};

		// Digest / Block traits for SHA-1.
		//
		static constexpr size_t block_size = 64;
		static constexpr size_t digest_size = 160 / 8;
		using block_type = std::array<uint8_t, block_size>;
		using value_type = std::array<uint32_t, digest_size / sizeof( uint32_t )>;

		// Default initialization vector for SHA-1.
		//
		static constexpr value_type default_iv = {
			0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0,
		};

		// Constant words K for SHA-1.
		//
		static constexpr std::array<uint32_t, 4> k_const = {
			0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6
		};

		// Implement the compression functor mixing the data.
		//
		__forceinline static constexpr void compress( value_type& iv, const block_type& block )
		{
			constexpr auto f1 = [ ] ( uint32_t x, uint32_t y, uint32_t z ) { return z ^ ( x & ( y ^ z ) ); };
			constexpr auto f2 = [ ] ( uint32_t x, uint32_t y, uint32_t z ) { return x ^ y ^ z; };
			constexpr auto f3 = [ ] ( uint32_t x, uint32_t y, uint32_t z ) { return ( x & y ) | ( x & z ) | ( y & z ); };

			uint32_t workspace[ 80 ] = { 0 };
			for ( size_t i = 0; i != 16; i++ )
			{
				uint32_t n = 0;
				for ( size_t j = 0; j != 4; j++ )
					n |= uint32_t( block[ ( i * 4 ) + j ] ) << ( 24 - 8 * j );
				workspace[ i ] = n;
			}
			for ( size_t i = 16; i != 80; i++ )
				workspace[ i ] = rotl( workspace[ i - 3 ] ^ workspace[ i - 8 ] ^ workspace[ i - 14 ] ^ workspace[ i - 16 ], 1 );

			value_type ivd = iv;
			auto mix = [ & ] <auto N> ( const_tag<N>, auto&& f )
			{
				auto& [a, b, c, d, e] = ivd;

				for ( size_t i = 0; i != 4; i++ )
				{
					size_t offset = 5 * ( i + N * 4 );
					e += rotl( a, 5 ) + f( b, c, d ) + workspace[ offset + 0 ] + k_const[ N ]; b = rotl( b, 30 );
					d += rotl( e, 5 ) + f( a, b, c ) + workspace[ offset + 1 ] + k_const[ N ]; a = rotl( a, 30 );
					c += rotl( d, 5 ) + f( e, a, b ) + workspace[ offset + 2 ] + k_const[ N ]; e = rotl( e, 30 );
					b += rotl( c, 5 ) + f( d, e, a ) + workspace[ offset + 3 ] + k_const[ N ]; d = rotl( d, 30 );
					a += rotl( b, 5 ) + f( c, d, e ) + workspace[ offset + 4 ] + k_const[ N ]; c = rotl( c, 30 );
				}
			};
			mix( const_tag<0>{}, f1 );
			mix( const_tag<1>{}, f2 );
			mix( const_tag<2>{}, f3 );
			mix( const_tag<3>{}, f2 );

			for ( size_t n = 0; n != ivd.size(); n++ )
				iv[ n ] += ivd[ n ];
		}

		// Crypto state.
		//
		value_type iv;
		size_t bit_count = 0;
		block_type leftover = { 0 };
		size_t leftover_offset = 0;
		bool finalized = false;

		// Construct a new hash from an optional IV of 160-bit value.
		//
		constexpr sha1() noexcept
			: iv{ default_iv } {}
		constexpr sha1( value_type result ) noexcept
			: iv{ result }, finalized( true ) {}
		constexpr sha1( value_type iv160, iv_tag ) noexcept
			: iv{ iv160 } {}

		// Default copy/move.
		//
		constexpr sha1( sha1&& ) noexcept = default;
		constexpr sha1( const sha1& ) = default;
		constexpr sha1& operator=( sha1&& ) noexcept = default;
		constexpr sha1& operator=( const sha1& ) = default;

		// Appends the given array of bytes into the hash value.
		//
		__forceinline constexpr void add_bytes( const uint8_t* data, size_t n )
		{
			while ( n-- )
			{
				leftover[ leftover_offset++ ] = *data++;

				if ( leftover_offset >= block_size )
				{
					compress( iv, leftover );
					leftover_offset = 0;
					bit_count += block_size * 8;
				}
			}
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
		__forceinline constexpr void finalize() noexcept
		{
			if ( finalized ) [[likely]]
				return;
			
			// Apply the leftover block and terminate the stream.
			//
			bit_count += leftover_offset * 8;
			leftover[ leftover_offset++ ] = 0x80;
			
			// If padding does not fit in the current block, compress first.
			//
			if ( leftover_offset > ( block_size - 8 ) )
			{
				std::fill( leftover.begin() + leftover_offset, leftover.end(), 0 );
				compress( iv, leftover );
				leftover_offset = 0;
			}

			// Pad with zeros and append the big endian lenght at the end.
			//
			std::fill( leftover.begin() + leftover_offset, leftover.end() - 8, 0 );
			leftover[ block_size - 1 ] = ( uint8_t ) ( bit_count );
			leftover[ block_size - 2 ] = ( uint8_t ) ( bit_count >> 8 );
			leftover[ block_size - 3 ] = ( uint8_t ) ( bit_count >> 16 );
			leftover[ block_size - 4 ] = ( uint8_t ) ( bit_count >> 24 );
			leftover[ block_size - 5 ] = ( uint8_t ) ( bit_count >> 32 );
			leftover[ block_size - 6 ] = ( uint8_t ) ( bit_count >> 40 );
			leftover[ block_size - 7 ] = ( uint8_t ) ( bit_count >> 48 );
			leftover[ block_size - 8 ] = ( uint8_t ) ( bit_count >> 56 );

			// Compress again.
			//
			compress( iv, leftover );

			// Write the digest and set the flag.
			//
			for ( auto& v : iv )
				v = bswap( v );
			finalized = true;
		}
		__forceinline constexpr value_type digest() const noexcept
		{
			if ( finalized ) [[likely]]
				return iv;
			sha1 clone{ *this };
			clone.finalize();
			return clone.iv;
		}

		// Explicit conversions.
		//
		constexpr value_type as160() const noexcept { return digest(); }
		constexpr uint32_t as32() const noexcept { return as160()[ 0 ]; }
		constexpr uint64_t as64() const noexcept
		{
			auto v = as160();
			return ( uint64_t( v[ 1 ] ) << 32 ) | v[ 0 ];
		}

		// Implicit conversions.
		//
		constexpr operator value_type() const noexcept { return as160(); }
		constexpr operator uint64_t() const noexcept { return as64(); }

		// Conversion to human-readable format.
		//
		std::string to_string() const { return fmt::const_hex_dump( digest() ); }

		// Basic comparison operators.
		//
		constexpr bool operator<( const sha1& o ) const noexcept { return digest() < o.digest(); }
		constexpr bool operator==( const sha1& o ) const noexcept { return digest() == o.digest(); }
		constexpr bool operator!=( const sha1& o ) const noexcept { return digest() != o.digest(); }
	};
};

// Make it std::hashable.
//
namespace std
{
	template<>
	struct hash<xstd::sha1>
	{
		size_t operator()( const xstd::sha1& value ) const { return ( size_t ) value.as64(); }
	};
};