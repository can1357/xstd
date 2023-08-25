#pragma once
#include "sha1.hpp"

// [[Configuration]]
// XSTD_HW_SHA256: Determines the availability of hardware SHA256.
//
#ifndef XSTD_HW_SHA256
	#define XSTD_HW_SHA256 ( AMD64_TARGET && GNU_COMPILER )
#endif

#if XSTD_HW_SHA256 && AMD64_TARGET && GNU_COMPILER
#include <ia32.hpp>
#endif

namespace xstd
{
	// Define SHA-256.
	//
	template<bool HwAcccel>
	struct sha256_traits
	{
		using block_type = std::array<uint8_t,  64>;
		using value_type = std::array<uint32_t, 256 / ( 8 * sizeof( uint32_t ) )>;
		
		static constexpr value_type default_iv = {
			0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
			0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
		};
		static constexpr std::array<uint32_t, 64> k_const = {
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
			if ( HwAcccel && !std::is_constant_evaluated() ) {
#if AMD64_TARGET && GNU_COMPILER
				if ( ia32::static_cpuid_s<7, 0, ia32::cpuid_eax_07>.ebx.sha ) {
					ia32::sha256_compress( iv.data(), block, k_const.data() );
					return;
				}
#endif
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
			for ( size_t i = 16; i != 64; i++ )
			{
				workspace[ i & 0xF ] += s0( workspace[ ( i + 1 ) & 0xF ] ) + s1( workspace[ ( i + 14 ) & 0xF ] ) + workspace[ ( i + 9 ) & 0xF ];
				shuffle( workspace[ i & 0xF ], i );
			}

			for ( size_t n = 0; n != ivd.size(); n++ )
				iv[ n ] += ivd[ n ];
		}
	};
	using sha256 =   basic_sha<sha256_traits<true>>;
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