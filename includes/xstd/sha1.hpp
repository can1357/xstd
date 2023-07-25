#pragma once
#include <string>
#include <array>
#include <functional>
#include <numeric>
#include <cstring>
#include "intrinsics.hpp"
#include "type_helpers.hpp"
#include "hexdump.hpp"

// [[Configuration]]
// XSTD_HW_SHA1: Determines the availability of hardware SHA1.
//
#ifndef XSTD_HW_SHA1
	#define XSTD_HW_SHA1 ( AMD64_TARGET && GNU_COMPILER )
#endif

// Intel implementation.
//
#if XSTD_HW_SHA1 && AMD64_TARGET && GNU_COMPILER
#include "../ia32.hpp"
namespace xstd::impl {
	FORCE_INLINE inline bool hw_sha1_compress_s( uint32_t* iv, const uint8_t* block ) {
		return ia32::sha1_compress_s( iv, block );
	}
}
#endif

namespace xstd
{
	// Defines a 160-bit hash type based on SHA-1.
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
		inline static constexpr void compress( value_type& iv, const uint8_t* block )
		{
#if XSTD_HW_SHA1
			if ( !std::is_constant_evaluated() ) {
				if ( impl::hw_sha1_compress_s( iv.data(), block ) ) {
					return;
				}
			}
#endif
			constexpr auto f1 = [ ] ( uint32_t x, uint32_t y, uint32_t z ) FORCE_INLINE { return z ^ ( x & ( y ^ z ) ); };
			constexpr auto f2 = [ ] ( uint32_t x, uint32_t y, uint32_t z ) FORCE_INLINE { return x ^ y ^ z; };
			constexpr auto f3 = [ ] ( uint32_t x, uint32_t y, uint32_t z ) FORCE_INLINE { return ( x & y ) | ( x & z ) | ( y & z ); };

			uint32_t workspace[ 80 ] = { 0 };
			for ( size_t i = 0; i != 16; i++ )
			{
				uint32_t n = 0;
				if ( std::is_constant_evaluated() )
				{
					for ( size_t j = 0; j != 4; j++ )
						n |= uint32_t( block[ ( i * 4 ) + j ] ) << ( 24 - 8 * j );
				}
				else
				{
					n = bswapd( *( const uint32_t* ) &block[ ( i * 4 ) ] );
				}
				workspace[ i ] = n;
			}
			for ( size_t i = 16; i != 32; i++ )
				workspace[ i ] = rotl( workspace[ i - 3 ] ^ workspace[ i - 8 ] ^ workspace[ i - 14 ] ^ workspace[ i - 16 ], 1 );

			__hint_unroll()
			for ( size_t i = 32; i != 80; i++ )
				workspace[ i ] = rotl( workspace[ i - 6 ] ^ workspace[ i - 16 ] ^ workspace[ i - 28 ] ^ workspace[ i - 32 ], 2 );

			value_type ivd = iv;
			auto mix = [ & ] <auto N> ( const_tag<N>, auto&& f ) FORCE_INLINE
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
		size_t     input_length = 0;
		block_type leftover = { 0 };

		// Construct a new hash from an optional IV of 160-bit value.
		//
		constexpr sha1() noexcept
			: iv{ default_iv } {}
		constexpr sha1( value_type result ) noexcept
			: iv{ result }, input_length( std::string::npos ) {}
		constexpr sha1( value_type iv160, iv_tag ) noexcept
			: iv{ iv160 } {}

		// Default copy/move.
		//
		constexpr sha1( sha1&& ) noexcept = default;
		constexpr sha1( const sha1& ) = default;
		constexpr sha1& operator=( sha1&& ) noexcept = default;
		constexpr sha1& operator=( const sha1& ) = default;

		// Returns whether or not hash is finalized.
		//
		FORCE_INLINE constexpr bool finalized() const { return input_length == std::string::npos; }

		// Skips to next block.
		//
		FORCE_INLINE constexpr void next_block() 
		{
			compress( iv, leftover.data() );
			leftover = { 0 };
		}

		// Appends the given array of bytes into the hash value.
		//
		FORCE_INLINE constexpr void add_bytes( const uint8_t* data, size_t n )
		{
			size_t prev_length = input_length;
			input_length += n;
			assume( input_length != std::string::npos );

			// If there was a leftover:
			//
			if ( size_t offset = prev_length % block_size ) {
				// Pad with new data.
				//
				size_t space_available = block_size - offset;
				size_t copy_count =      std::min( n, space_available );
				if ( std::is_constant_evaluated() )
					std::copy_n( data, copy_count, leftover.data() + offset );
				else
					memcpy( leftover.data() + offset, data, copy_count );
				n -=    copy_count;
				data += copy_count;

				// Finish the block.
				//
				if ( space_available == copy_count ) {
					next_block();
				}

				// Break if done.
				//
				if ( !n ) return;
			}

			// Add full blocks.
			//
			while ( n >= block_size ) {
				compress( iv, data );
				n -=     block_size;
				data +=  block_size;
			}

			// Add the remainder.
			//
			if ( n ) {
				if ( std::is_constant_evaluated() )
					std::copy_n( data, n, leftover.begin() );
				else
					memcpy( leftover.data(), data, n );
			}
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
		FORCE_INLINE constexpr auto& finalize() noexcept
		{
			if ( finalized() ) return *this;
			
			// Apply the leftover block and terminate the stream.
			//
			size_t leftover_offset = input_length % block_size;
			leftover[ leftover_offset++ ] = 0x80;
			
			// If padding does not fit in the current block, compress first.
			//
			if ( leftover_offset > ( block_size - 8 ) ) {
				next_block();
			}

			// Append the big endian length at the end.
			//
			size_t bit_count = input_length * 8;
			if ( std::is_constant_evaluated() ) {
				leftover[ block_size - 1 ] = ( uint8_t ) ( bit_count );
				leftover[ block_size - 2 ] = ( uint8_t ) ( bit_count >> 8 );
				leftover[ block_size - 3 ] = ( uint8_t ) ( bit_count >> 16 );
				leftover[ block_size - 4 ] = ( uint8_t ) ( bit_count >> 24 );
				leftover[ block_size - 5 ] = ( uint8_t ) ( bit_count >> 32 );
				leftover[ block_size - 6 ] = ( uint8_t ) ( bit_count >> 40 );
				leftover[ block_size - 7 ] = ( uint8_t ) ( bit_count >> 48 );
				leftover[ block_size - 8 ] = ( uint8_t ) ( bit_count >> 56 );
			} else {
				*( uint64_t* ) &leftover[ block_size - 8 ] = bswapq( bit_count );
			}

			// Compress again.
			//
			next_block();

			// Write the digest and set the flag.
			//
			for ( auto& v : iv )
				v = bswap( v );
			input_length = std::string::npos;
			return *this;
		}
		FORCE_INLINE constexpr value_type digest() noexcept { return finalize().iv; }
		FORCE_INLINE constexpr value_type digest() const noexcept
		{
			if ( finalized() ) [[likely]]
				return iv;
			auto clone{ *this };
			return clone.digest();
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
		std::string to_string() const { return fmt::as_hex_string( digest() ); }

		// Basic comparison operators.
		//
		constexpr bool operator<( const sha1& o ) const noexcept { return digest() < o.digest(); }
		constexpr bool operator==( const sha1& o ) const noexcept { return digest() == o.digest(); }
		constexpr bool operator!=( const sha1& o ) const noexcept { return digest() != o.digest(); }
	};
	using sha1_t = typename sha1::value_type;
};

// String literal.
//
static consteval xstd::sha1_t operator""_sha1( const char* data, size_t ) {
	auto parse_digit = [ ] ( char c ) consteval {
		if ( '0' <= c && c <= '9' )
			return c - '0';
		c |= 0x20;
		return 0xA + ( c - 'a' );
	};
	xstd::sha1_t result = {};
	for ( auto& dw : result ) {
		for ( size_t i = 0; i != 4; i++ ) {
			uint8_t byte = parse_digit( *data++ ) << 4;
			byte |= parse_digit( *data++ );
			dw |= uint32_t( byte ) << ( i * 8 );
		}
	}
	return result;
}

// Make it std::hashable.
//
namespace std
{
	template<>
	struct hash<xstd::sha1>
	{
		constexpr size_t operator()( const xstd::sha1& value ) const { return ( size_t ) value.as64(); }
	};
};