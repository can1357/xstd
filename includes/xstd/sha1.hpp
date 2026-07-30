#pragma once
#include <string>
#include <array>
#include <functional>
#include <numeric>
#include <cstring>
#include "intrinsics.hpp"
#include "type_helpers.hpp"
#include "hexdump.hpp"

// Armv8 folds SHA-1 and SHA-256 into one feature. Older ACLE spells it CRYPTO,
// newer spells it SHA2; both mean the same instructions are present, and unlike
// x86 they are known at compile time, so neither needs a runtime probe.
//
#ifndef XSTD_ARM_SHA2
	#if ARM64_TARGET && ( defined( __ARM_FEATURE_SHA2 ) || defined( __ARM_FEATURE_CRYPTO ) )
		#define XSTD_ARM_SHA2 1
	#else
		#define XSTD_ARM_SHA2 0
	#endif
#endif

// [[Configuration]]
// XSTD_HW_SHA1: Determines the availability of hardware SHA1.
//
#ifndef XSTD_HW_SHA1
	#define XSTD_HW_SHA1 ( ( AMD64_TARGET && GNU_COMPILER ) || XSTD_ARM_SHA2 )
#endif

// Intel implementation.
//
#if XSTD_HW_SHA1 && AMD64_TARGET && GNU_COMPILER
	#include <ia32.hpp>
#endif
#if XSTD_ARM_SHA2
	#include <arm_neon.h>
#endif

namespace xstd::impl {
#if XSTD_HW_SHA1 && XSTD_ARM_SHA2
	// Compresses one 64-byte block into the five SHA-1 words.
	//
	// The instructions consume four rounds at a time: sha1h rotates the working
	// `a` into the next `e`, sha1c/p/m apply the round function for the quarter
	// it is named after, and sha1su0/su1 extend the message schedule two steps
	// ahead of where it is consumed.
	//
	FORCE_INLINE inline void arm_sha1_compress( uint32_t* state, const uint8_t* block ) {
		uint32x4_t abcd = vld1q_u32( state );
		uint32_t e0     = state[ 4 ];

		const uint32x4_t abcd_saved = abcd;
		const uint32_t e0_saved     = e0;

		uint32x4_t msg0 = vreinterpretq_u32_u8( vrev32q_u8( vld1q_u8( block + 0 ) ) );
		uint32x4_t msg1 = vreinterpretq_u32_u8( vrev32q_u8( vld1q_u8( block + 16 ) ) );
		uint32x4_t msg2 = vreinterpretq_u32_u8( vrev32q_u8( vld1q_u8( block + 32 ) ) );
		uint32x4_t msg3 = vreinterpretq_u32_u8( vrev32q_u8( vld1q_u8( block + 48 ) ) );

		const uint32x4_t k0 = vdupq_n_u32( 0x5a827999 );
		const uint32x4_t k1 = vdupq_n_u32( 0x6ed9eba1 );
		const uint32x4_t k2 = vdupq_n_u32( 0x8f1bbcdc );
		const uint32x4_t k3 = vdupq_n_u32( 0xca62c1d6 );

		uint32x4_t tmp0 = vaddq_u32( msg0, k0 );
		uint32x4_t tmp1 = vaddq_u32( msg1, k0 );
		uint32_t e1;

		// Rounds 0-19, choice.
		//
		e1   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1cq_u32( abcd, e0, tmp0 );
		tmp0 = vaddq_u32( msg2, k0 );
		msg0 = vsha1su0q_u32( msg0, msg1, msg2 );

		e0   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1cq_u32( abcd, e1, tmp1 );
		tmp1 = vaddq_u32( msg3, k0 );
		msg0 = vsha1su1q_u32( msg0, msg3 );
		msg1 = vsha1su0q_u32( msg1, msg2, msg3 );

		e1   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1cq_u32( abcd, e0, tmp0 );
		tmp0 = vaddq_u32( msg0, k0 );
		msg1 = vsha1su1q_u32( msg1, msg0 );
		msg2 = vsha1su0q_u32( msg2, msg3, msg0 );

		e0   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1cq_u32( abcd, e1, tmp1 );
		tmp1 = vaddq_u32( msg1, k1 );
		msg2 = vsha1su1q_u32( msg2, msg1 );
		msg3 = vsha1su0q_u32( msg3, msg0, msg1 );

		e1   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1cq_u32( abcd, e0, tmp0 );
		tmp0 = vaddq_u32( msg2, k1 );
		msg3 = vsha1su1q_u32( msg3, msg2 );
		msg0 = vsha1su0q_u32( msg0, msg1, msg2 );

		// Rounds 20-39, parity.
		//
		e0   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1pq_u32( abcd, e1, tmp1 );
		tmp1 = vaddq_u32( msg3, k1 );
		msg0 = vsha1su1q_u32( msg0, msg3 );
		msg1 = vsha1su0q_u32( msg1, msg2, msg3 );

		e1   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1pq_u32( abcd, e0, tmp0 );
		tmp0 = vaddq_u32( msg0, k1 );
		msg1 = vsha1su1q_u32( msg1, msg0 );
		msg2 = vsha1su0q_u32( msg2, msg3, msg0 );

		e0   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1pq_u32( abcd, e1, tmp1 );
		tmp1 = vaddq_u32( msg1, k1 );
		msg2 = vsha1su1q_u32( msg2, msg1 );
		msg3 = vsha1su0q_u32( msg3, msg0, msg1 );

		e1   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1pq_u32( abcd, e0, tmp0 );
		tmp0 = vaddq_u32( msg2, k2 );
		msg3 = vsha1su1q_u32( msg3, msg2 );
		msg0 = vsha1su0q_u32( msg0, msg1, msg2 );

		e0   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1pq_u32( abcd, e1, tmp1 );
		tmp1 = vaddq_u32( msg3, k2 );
		msg0 = vsha1su1q_u32( msg0, msg3 );
		msg1 = vsha1su0q_u32( msg1, msg2, msg3 );

		// Rounds 40-59, majority.
		//
		e1   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1mq_u32( abcd, e0, tmp0 );
		tmp0 = vaddq_u32( msg0, k2 );
		msg1 = vsha1su1q_u32( msg1, msg0 );
		msg2 = vsha1su0q_u32( msg2, msg3, msg0 );

		e0   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1mq_u32( abcd, e1, tmp1 );
		tmp1 = vaddq_u32( msg1, k2 );
		msg2 = vsha1su1q_u32( msg2, msg1 );
		msg3 = vsha1su0q_u32( msg3, msg0, msg1 );

		e1   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1mq_u32( abcd, e0, tmp0 );
		tmp0 = vaddq_u32( msg2, k2 );
		msg3 = vsha1su1q_u32( msg3, msg2 );
		msg0 = vsha1su0q_u32( msg0, msg1, msg2 );

		e0   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1mq_u32( abcd, e1, tmp1 );
		tmp1 = vaddq_u32( msg3, k3 );
		msg0 = vsha1su1q_u32( msg0, msg3 );
		msg1 = vsha1su0q_u32( msg1, msg2, msg3 );

		e1   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1mq_u32( abcd, e0, tmp0 );
		tmp0 = vaddq_u32( msg0, k3 );
		msg1 = vsha1su1q_u32( msg1, msg0 );
		msg2 = vsha1su0q_u32( msg2, msg3, msg0 );

		// Rounds 60-79, parity again.
		//
		e0   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1pq_u32( abcd, e1, tmp1 );
		tmp1 = vaddq_u32( msg1, k3 );
		msg2 = vsha1su1q_u32( msg2, msg1 );
		msg3 = vsha1su0q_u32( msg3, msg0, msg1 );

		e1   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1pq_u32( abcd, e0, tmp0 );
		tmp0 = vaddq_u32( msg2, k3 );
		msg3 = vsha1su1q_u32( msg3, msg2 );

		e0   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1pq_u32( abcd, e1, tmp1 );
		tmp1 = vaddq_u32( msg3, k3 );

		e1   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1pq_u32( abcd, e0, tmp0 );

		e0   = vsha1h_u32( vgetq_lane_u32( abcd, 0 ) );
		abcd = vsha1pq_u32( abcd, e1, tmp1 );

		vst1q_u32( state, vaddq_u32( abcd_saved, abcd ) );
		state[ 4 ] = e0 + e0_saved;
	}
#endif
};

namespace xstd {
	// Common SHA implementation.
	//
	template<typename Traits>
	struct basic_sha {
		using value_type = typename Traits::value_type;
		using block_type = typename Traits::block_type;
		using unit_type  = typename value_type::value_type;

		static constexpr size_t block_size  = std::tuple_size_v<block_type>;
		static constexpr size_t digest_size = std::tuple_size_v<value_type> * sizeof( unit_type );

		// Crypto state.
		//
		value_type iv;
		size_t input_length = 0;
		block_type leftover = { 0 };

		// Default IV connstruction.
		//
		constexpr basic_sha() noexcept : iv{ Traits::default_iv } {}

		// Custom IV construction.
		//
		constexpr basic_sha( value_type iv ) noexcept : iv{ iv } {}

		// Default copy/move.
		//
		constexpr basic_sha( basic_sha&& ) noexcept            = default;
		constexpr basic_sha( const basic_sha& )                = default;
		constexpr basic_sha& operator=( basic_sha&& ) noexcept = default;
		constexpr basic_sha& operator=( const basic_sha& )     = default;

		// Returns whether or not hash is finalized.
		//
		FORCE_INLINE constexpr bool finalized() const { return input_length == std::string::npos; }

		// Skips to next block.
		//
		FORCE_INLINE constexpr void next_block() {
			Traits::compress( iv, leftover.data() );
			leftover = { 0 };
		}

		// Appends the given array of bytes into the hash value.
		//
		FORCE_INLINE constexpr void add_bytes( const uint8_t* data, size_t n ) {
			size_t prev_length = input_length;
			input_length += n;
			assume( input_length != std::string::npos );

			// If there was a leftover:
			//
			if ( size_t offset = prev_length % leftover.size() ) {
				// Pad with new data.
				//
				size_t space_available = leftover.size() - offset;
				size_t copy_count      = std::min( n, space_available );
				if ( std::is_constant_evaluated() )
					std::copy_n( data, copy_count, leftover.data() + offset );
				else
					memcpy( leftover.data() + offset, data, copy_count );
				n -= copy_count;
				data += copy_count;

				// Finish the block.
				//
				if ( space_available == copy_count ) {
					next_block();
				}

				// Break if done.
				//
				if ( !n )
					return;
			}

			// Add full blocks.
			//
			while ( n >= leftover.size() ) {
				Traits::compress( iv, data );
				n -= leftover.size();
				data += leftover.size();
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
		FORCE_INLINE constexpr void add_bytes( const T& data ) noexcept {
			if ( std::is_constant_evaluated() ) {
				using array_t = std::array<uint8_t, sizeof( T )>;
				array_t arr   = xstd::bit_cast<array_t>( data );
				add_bytes( arr.data(), arr.size() );
			} else {
				add_bytes( ( const uint8_t* ) &data, sizeof( T ) );
			}
		}

		// Update wrapper.
		//
		template<typename T>
		FORCE_INLINE constexpr basic_sha& update( const T& data ) noexcept {
			this->add_bytes<T>( data );
			return *this;
		};
		FORCE_INLINE constexpr basic_sha& update( const uint8_t* data, size_t n ) {
			this->add_bytes( data, n );
			return *this;
		}

		// Finalization of the hash.
		//
		FORCE_INLINE constexpr basic_sha& finalize() noexcept {
			if ( finalized() )
				return *this;

			// Apply the leftover block and terminate the stream.
			//
			size_t leftover_offset        = input_length % leftover.size();
			leftover[ leftover_offset++ ] = 0x80;

			// If padding does not fit in the current block, compress first.
			//
			// The length field is an eighth of the block: 64 bits for the hashes
			// on a 64-byte block, 128 for SHA-512's 128-byte one. Reserving a
			// flat eight bytes would let a message ending 112 to 119 bytes into
			// a SHA-512 block keep its length in that same block, where the
			// standard says it no longer fits.
			//
			constexpr size_t length_field = block_size / 8;
			if ( leftover_offset > ( leftover.size() - length_field ) ) {
				next_block();
			}

			// Append the big endian length at the end.
			//
			size_t bit_count = input_length * 8;
			if ( std::is_constant_evaluated() ) {
				leftover[ leftover.size() - 1 ] = ( uint8_t ) ( bit_count );
				leftover[ leftover.size() - 2 ] = ( uint8_t ) ( bit_count >> 8 );
				leftover[ leftover.size() - 3 ] = ( uint8_t ) ( bit_count >> 16 );
				leftover[ leftover.size() - 4 ] = ( uint8_t ) ( bit_count >> 24 );
				leftover[ leftover.size() - 5 ] = ( uint8_t ) ( bit_count >> 32 );
				leftover[ leftover.size() - 6 ] = ( uint8_t ) ( bit_count >> 40 );
				leftover[ leftover.size() - 7 ] = ( uint8_t ) ( bit_count >> 48 );
				leftover[ leftover.size() - 8 ] = ( uint8_t ) ( bit_count >> 56 );
			} else {
				*( uint64_t* ) &leftover[ leftover.size() - 8 ] = bswapq( bit_count );
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
		FORCE_INLINE constexpr value_type digest() const noexcept {
			if ( finalized() ) [[likely]]
				return iv;
			auto clone{ *this };
			return clone.digest();
		}

		// Explicit conversions.
		//
		constexpr uint32_t as32() const noexcept { return uint32_t( digest()[ 0 ] ); }
		constexpr uint64_t as64() const noexcept {
			if constexpr ( std::is_same_v<unit_type, uint64_t> ) {
				return digest()[ 0 ];
			} else if constexpr ( std::is_same_v<unit_type, uint32_t> ) {
				auto v = digest();
				return ( uint64_t( v[ 1 ] ) << 32 ) | v[ 0 ];
			} else {
				static_assert( sizeof( unit_type ) == -1, "Invalid unit." );
			}
		}

		// Implicit conversions.
		//
		constexpr operator value_type() const noexcept { return digest(); }
		constexpr operator uint64_t() const noexcept { return as64(); }
		constexpr operator uint32_t() const noexcept { return as32(); }

		// Conversion to human-readable format.
		//
		std::string to_string() const { return fmt::as_hex_string( digest() ); }

		// Basic comparison operators.
		//
		constexpr bool operator<( const basic_sha& o ) const noexcept { return digest() < o.digest(); }
		constexpr bool operator==( const basic_sha& o ) const noexcept { return digest() == o.digest(); }
		constexpr bool operator!=( const basic_sha& o ) const noexcept { return digest() != o.digest(); }
	};

	// Define SHA-1.
	//
	template<bool HwAcccel>
	struct sha1_traits {
		using block_type = std::array<uint8_t, 64>;
		using value_type = std::array<uint32_t, 160 / ( 8 * sizeof( uint32_t ) )>;

		static constexpr value_type default_iv = {
			0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0,
		};
		static constexpr std::array<uint32_t, 4> k_const = { 0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6 };

		// Implement the compression functor mixing the data.
		//
		inline static constexpr void compress( value_type& iv, const uint8_t* block ) {
#if XSTD_HW_SHA1
			if ( HwAcccel && !std::is_constant_evaluated() ) {
	#if AMD64_TARGET && GNU_COMPILER
				if ( ia32::static_cpuid_s<7, 0, ia32::cpuid_eax_07>.ebx.sha ) {
					ia32::sha1_compress( iv.data(), block );
					return;
				}
	#elif XSTD_ARM_SHA2
				impl::arm_sha1_compress( iv.data(), block );
				return;
	#endif
			}
#endif
			constexpr auto f1 = []( uint32_t x, uint32_t y, uint32_t z ) FORCE_INLINE { return z ^ ( x & ( y ^ z ) ); };
			constexpr auto f2 = []( uint32_t x, uint32_t y, uint32_t z ) FORCE_INLINE { return x ^ y ^ z; };
			constexpr auto f3 = []( uint32_t x, uint32_t y, uint32_t z ) FORCE_INLINE { return ( x & y ) | ( x & z ) | ( y & z ); };

			uint32_t workspace[ 80 ] = { 0 };
			for ( size_t i = 0; i != 16; i++ ) {
				uint32_t n = 0;
				if ( std::is_constant_evaluated() ) {
					for ( size_t j = 0; j != 4; j++ )
						n |= uint32_t( block[ ( i * 4 ) + j ] ) << ( 24 - 8 * j );
				} else {
					n = bswapd( *( const uint32_t* ) &block[ ( i * 4 ) ] );
				}
				workspace[ i ] = n;
			}
			for ( size_t i = 16; i != 32; i++ )
				workspace[ i ] = rotl( workspace[ i - 3 ] ^ workspace[ i - 8 ] ^ workspace[ i - 14 ] ^ workspace[ i - 16 ], 1 );

			__hint_unroll() for ( size_t i = 32; i != 80; i++ ) workspace[ i ] = rotl( workspace[ i - 6 ] ^ workspace[ i - 16 ] ^ workspace[ i - 28 ] ^ workspace[ i - 32 ], 2 );

			value_type ivd = iv;
			auto mix       = [ & ]<auto N>( const_tag<N>, auto&& f ) FORCE_INLINE {
				auto& [ a, b, c, d, e ] = ivd;

				for ( size_t i = 0; i != 4; i++ ) {
					size_t offset = 5 * ( i + N * 4 );
					e += rotl( a, 5 ) + f( b, c, d ) + workspace[ offset + 0 ] + k_const[ N ];
					b = rotl( b, 30 );
					d += rotl( e, 5 ) + f( a, b, c ) + workspace[ offset + 1 ] + k_const[ N ];
					a = rotl( a, 30 );
					c += rotl( d, 5 ) + f( e, a, b ) + workspace[ offset + 2 ] + k_const[ N ];
					e = rotl( e, 30 );
					b += rotl( c, 5 ) + f( d, e, a ) + workspace[ offset + 3 ] + k_const[ N ];
					d = rotl( d, 30 );
					a += rotl( b, 5 ) + f( c, d, e ) + workspace[ offset + 4 ] + k_const[ N ];
					c = rotl( c, 30 );
				}
			};
			mix( const_tag<0>{}, f1 );
			mix( const_tag<1>{}, f2 );
			mix( const_tag<2>{}, f3 );
			mix( const_tag<3>{}, f2 );

			for ( size_t n = 0; n != ivd.size(); n++ )
				iv[ n ] += ivd[ n ];
		}
	};
	using sha1   = basic_sha<sha1_traits<true>>;
	using sha1_t = typename sha1::value_type;
};

// String literal.
//
static consteval xstd::sha1_t operator""_sha1( const char* data, size_t ) {
	auto parse_digit = []( char c ) consteval {
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
namespace std {
	template<typename T>
	struct hash<xstd::basic_sha<T>> {
		constexpr size_t operator()( const xstd::basic_sha<T>& value ) const { return ( size_t ) value.as64(); }
	};
};