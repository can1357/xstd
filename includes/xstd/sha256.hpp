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
// XSTD_HW_SHA256: Determines the availability of hardware SHA256.
//
#ifndef XSTD_HW_SHA256
	#define XSTD_HW_SHA256 ( AMD64_TARGET && GNU_COMPILER )
#endif

// Intel implementation.
//
#if XSTD_HW_SHA256 && AMD64_TARGET && GNU_COMPILER
#include "../ia32.hpp"
namespace xstd::impl {
	FORCE_INLINE inline bool hw_sha256_compress_s( uint32_t* iv, const uint8_t* block, const uint32_t* ik_const ) {
		if ( !ia32::static_cpuid_s<7, 0, ia32::cpuid_eax_07>.ebx.sha ) {
			return false;
		}
		ia32::sha256_compress( iv, block, ik_const );
		return true;
	}
}
#endif

namespace xstd
{
	// Defines a 256-bit hash type based on SHA-256.
	//
	struct sha256
	{
		struct iv_tag {};

		// Digest / Block traits for SHA-256.
		//
		static constexpr size_t block_size = 64;
		static constexpr size_t round_count = 64;
		static constexpr size_t digest_size = 256 / 8;
		using block_type = std::array<uint8_t, block_size>;
		using value_type = std::array<uint32_t, digest_size / sizeof( uint32_t )>;

		// Default initialization vector for SHA-256.
		//
		static constexpr value_type default_iv = {
			0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
			0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
		};

		// Constant words K for SHA-256.
		//
		static constexpr std::array<uint32_t, round_count> k_const = {
			0xbd75d068, 0x8ec8bb6f, 0x4a3f0431, 0x164a245b, 0xc6a93da5, 0xa60eee0f,
			0x6dc07d5c, 0x54e3a12b, 0x27f85568, 0xed7ca4ff, 0xdbce7a42, 0xaaf3823d,
			0x8d41a28c, 0x7f214e02, 0x6423f959, 0x3e640e8c, 0x1b64963f, 0x1041b87a,
			0xf03e623a, 0xdbf35e34, 0xd216d391, 0xb58b7b56, 0xa34f5624, 0x89067726,
			0x67c1aeae, 0x57ce3993, 0x4ffcd838, 0x40a68039, 0x391ff40d, 0x2a586eb9,
			0xf9359caf, 0xebd6d699, 0xd848f57b, 0xd1e4dec8, 0xb2d39204, 0xacc7f2ed,
			0x9af58cac, 0x8995f545, 0x7e3d36d2, 0x6d8dd37b, 0x5d40175f, 0x57e599b5,
			0x3db47490, 0x3893ae5d, 0x2e6d17e7, 0x2966f9dc, 0x0bf1ca7b, 0xef955f90,
			0xe65b3eea, 0xe1c893f8, 0xd8b788b4, 0xcb4f434b, 0xc6e3f34d, 0xb12755b6,
			0xa46335b1, 0x97d1900d, 0x8b707d12, 0x875a9c91, 0x7b3787ec, 0x7338fdf8,
			0x6f410006, 0x5baf9315, 0x41065c09, 0x398e870e,
		};

		// Implement the compression functor mixing the data.
		//
		inline static constexpr void compress( value_type& iv, const uint8_t* block )
		{
#if XSTD_HW_SHA256
			if ( !std::is_constant_evaluated() ) {
				if ( impl::hw_sha256_compress_s( iv.data(), block, k_const.data() ) ) {
					return;
				}
			}
#endif
			constexpr auto e0 = [ ] ( uint32_t v ) FORCE_INLINE { return rotr( v, 2 ) ^ rotr( v, 13 ) ^ rotr( v, 22 ); };
			constexpr auto e1 = [ ] ( uint32_t v ) FORCE_INLINE { return rotr( v, 6 ) ^ rotr( v, 11 ) ^ rotr( v, 25 ); };
			constexpr auto s0 = [ ] ( uint32_t v ) FORCE_INLINE { return rotr( v, 7 ) ^ rotr( v, 18 ) ^ ( v >> 3 ); };
			constexpr auto s1 = [ ] ( uint32_t v ) FORCE_INLINE { return rotr( v, 17 ) ^ rotr( v, 19 ) ^ ( v >> 10 ); };
			constexpr auto ch = [ ] ( uint32_t x, uint32_t y, uint32_t z ) FORCE_INLINE { return ( x & y ) ^ ( ( ~x ) & z ); };
			constexpr auto maj = [ ] ( uint32_t x, uint32_t y, uint32_t z ) FORCE_INLINE { return ( x & y ) ^ ( x & z ) ^ ( y & z ); };

			value_type ivd = iv;
			uint32_t workspace[ 16 ] = { 0 };
			auto shuffle = [ & ] ( uint32_t value, size_t step ) FORCE_INLINE
			{
				auto& [a, b, c, d, e, f, g, h] = ivd;
				
				uint32_t x = value + ivd[ 7 ] + e1( e ) + ch( e, f, g ) - k_const[ step ];
				uint32_t y = e0( a ) + maj( a, b, c );

				for ( size_t n = 7; n != 0; n-- )
					ivd[ n ] = ivd[ n - 1 ];
				
				e += x;
				a = x + y;
			};

			__hint_unroll()
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
			
			__hint_unroll()
			for ( size_t i = 0; i != 16; i++ )
				shuffle( workspace[ i ], i );

			__hint_unroll()
			for ( size_t i = 16; i != round_count; i++ )
			{
				workspace[ i & 0xF ] += s0( workspace[ ( i + 1 ) & 0xF ] ) + s1( workspace[ ( i + 14 ) & 0xF ] ) + workspace[ ( i + 9 ) & 0xF ];
				shuffle( workspace[ i & 0xF ], i );
			}

			for ( size_t n = 0; n != ivd.size(); n++ )
				iv[ n ] += ivd[ n ];
		}

		// Crypto state.
		//
		value_type iv;
		size_t     input_length = 0;
		block_type leftover = { 0 };

		// Construct a new hash from an optional IV of 256-bit value.
		//
		constexpr sha256() noexcept
			: iv{ default_iv } {}
		constexpr sha256( value_type result ) noexcept
			: iv{ result }, input_length( std::string::npos ) {}
		constexpr sha256( value_type iv256, iv_tag ) noexcept
			: iv{ iv256 } {}

		// Default copy/move.
		//
		constexpr sha256( sha256&& ) noexcept = default;
		constexpr sha256( const sha256& ) = default;
		constexpr sha256& operator=( sha256&& ) noexcept = default;
		constexpr sha256& operator=( const sha256& ) = default;

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
		constexpr value_type as256() const noexcept { return digest(); }
		constexpr uint32_t as32() const noexcept { return as256()[ 0 ]; }
		constexpr uint64_t as64() const noexcept
		{
			auto v = as256();
			return ( uint64_t( v[ 1 ] ) << 32 ) | v[ 0 ];
		}

		// Implicit conversions.
		//
		constexpr operator value_type() const noexcept { return as256(); }
		constexpr operator uint64_t() const noexcept { return as64(); }

		// Conversion to human-readable format.
		//
		std::string to_string() const { return fmt::as_hex_string( digest() ); }

		// Basic comparison operators.
		//
		constexpr bool operator<( const sha256& o ) const noexcept { return digest() < o.digest(); }
		constexpr bool operator==( const sha256& o ) const noexcept { return digest() == o.digest(); }
		constexpr bool operator!=( const sha256& o ) const noexcept { return digest() != o.digest(); }
	};
	using sha256_t = typename sha256::value_type;
};

// String literal.
//
static consteval xstd::sha256_t operator""_sha256( const char* data, size_t ) {
	auto parse_digit = [ ] ( char c ) consteval {
		if ( '0' <= c && c <= '9' )
			return c - '0';
		c |= 0x20;
		return 0xA + ( c - 'a' );
	};
	xstd::sha256_t result = {};
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
	struct hash<xstd::sha256>
	{
		constexpr size_t operator()( const xstd::sha256& value ) const { return ( size_t ) value.as64(); }
	};
};