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
	// Defines a 512-bit hash type based on SHA-512.
	//
	struct sha512
	{
		struct iv_tag {};

		// Digest / Block traits for SHA-512.
		//
		static constexpr size_t block_size = 128;
		static constexpr size_t round_count = 80;
		static constexpr size_t digest_size = 512 / 8;
		using block_type = std::array<uint8_t, block_size>;
		using value_type = std::array<uint64_t, digest_size / sizeof( uint64_t )>;

		// Default initialization vector for SHA-512.
		//
		static constexpr value_type default_iv = {
			0x6A09E667F3BCC908, 0xBB67AE8584CAA73B,
			0x3C6EF372FE94F82B, 0xA54FF53A5F1D36F1,
			0x510E527FADE682D1, 0x9B05688C2B3E6C1F,
			0x1F83D9ABFB41BD6B, 0x5BE0CD19137E2179,
		};

		// Constant words K for SHA-512.
		//
		static constexpr std::array<uint64_t, round_count> k_const = {
			0xbd75d06728d751de, 0x8ec8bb6edc109a33, 0x4a3f043013b2c4d1, 0x164a245a7e762444, 
			0xc6a93da40cb74ac8, 0xa60eee0e49fa2fe7, 0x6dc07d5b50e6b065, 0x54e3a12a25927ee8,
			0x27f855675cfcfdbe, 0xed7ca4feba8f9042, 0xdbce7a41b11b4d74, 0xaaf3823c2a004b1e,
			0x8d41a28b0d847691, 0x7f214e01c4e9694f, 0x6423f958da38edcb, 0x3e640e8b3096d96c,
			0x1b64963e610eb52e, 0x1041b879c7b0da1d, 0xf03e623974732a4b, 0xdbf35e338853639b,
			0xd216d390a6d4fd8b, 0xb58b7b5591591b7d, 0xa34f562342be042c, 0x890677257ceeac4b,
			0x67c1aead11992055, 0x57ce3992d24bcdf0, 0x4ffcd8376704dec1, 0x40a680384110f11c,
			0x391ff40cc257703e, 0x2a586eb86cf558db, 0xf9359cae1ffc7d91, 0xebd6d698f5f19190,
			0xd848f57ab92dd004, 0xd1e4dec7a3d936da, 0xb2d39203a53bd513, 0xacc7f2ec626a4c21,
			0x9af58cab74509c22, 0x8995f544c3884d58, 0x7e3d36d1b812511a, 0x6d8dd37aeb7dcac5,
			0x5d40175eb30efc9c, 0x57e599b443bdcfff, 0x3db4748f2f07686f, 0x3893ae5cf9ab41d0,
			0x2e6d17e62910ade8, 0x2966f9dbaa9a56f0, 0xbf1ca7aa88edfd6,  0xef955f8fcd442e48,
			0xe65b3ee9472d2f38, 0xe1c893f7aebe54ad, 0xd8b788b320711467, 0xcb4f434a1e64b758,
			0xc6e3f34c3a36a59d, 0xb12755b51cbe7535, 0xa46335b0889c1c8d, 0x97d1900c294d475d,
			0x8b707d11a2104d04, 0x875a9c90bce8d0a0, 0x7b3787eb5e0f548e, 0x7338fdf7e59bc614,
			0x6f410005dc9ce1d8, 0x5baf9314217d4217, 0x41065c084d3986eb, 0x398e870d1c8dacd5,
			0x35d8c13115d99e64, 0x2e794738de3f3df9, 0x15258229321f14e2, 0xa82b08011912e88, 
			0xf90f98558de89046, 0xf59c823a5d37675a, 0xeec067fb4106f252, 0xe48ef4caece3b8e5,
			0xd724880adcfb827c, 0xcd355484bf38db6d, 0xc36141f5ea364144, 0xbce2983b63eff2b4,
			0xb33a2b4134c1bd4a, 0xa680d663039a81d6, 0xa0349054c5290514, 0x93bbe673b5b8a7e9
		};

		// Implement the compression functor mixing the data.
		//
		FORCE_INLINE static constexpr void compress( value_type& iv, const block_type& block )
		{
			constexpr auto e0 = [ ] ( uint64_t v ) FORCE_INLINE { return rotr( v, 28 ) ^ rotr( v, 34 ) ^ rotr( v, 39 ); };
			constexpr auto e1 = [ ] ( uint64_t v ) FORCE_INLINE { return rotr( v, 14 ) ^ rotr( v, 18 ) ^ rotr( v, 41 ); };
			constexpr auto s0 = [ ] ( uint64_t v ) FORCE_INLINE { return rotr( v, 1 ) ^ rotr( v, 8 ) ^ ( v >> 7 ); };
			constexpr auto s1 = [ ] ( uint64_t v ) FORCE_INLINE { return rotr( v, 19 ) ^ rotr( v, 61 ) ^ ( v >> 6 ); };
			constexpr auto ch = [ ] ( uint64_t x, uint64_t y, uint64_t z ) FORCE_INLINE { return ( x & y ) ^ ( ( ~x ) & z ); };
			constexpr auto maj = [ ] ( uint64_t x, uint64_t y, uint64_t z ) FORCE_INLINE { return ( x & y ) ^ ( x & z ) ^ ( y & z ); };

			value_type ivd = iv;
			uint64_t workspace[ 16 ] = { 0 };
			auto shuffle = [ & ] ( uint64_t value, size_t step ) FORCE_INLINE
			{
				auto& [a, b, c, d, e, f, g, h] = ivd;
				
				uint64_t x = value + ivd[ 7 ] + e1( e ) + ch( e, f, g ) - k_const[ step ];
				uint64_t y = e0( a ) + maj( a, b, c );

				for ( size_t n = 7; n != 0; n-- )
					ivd[ n ] = ivd[ n - 1 ];
				
				e += x;
				a = x + y;
			};

			__hint_unroll()
			for ( size_t i = 0; i != 16; i++ )
			{
				uint64_t n = 0;
				if ( std::is_constant_evaluated() )
				{
					for ( size_t j = 0; j != 8; j++ )
						n |= uint64_t( block[ ( i * 8 ) + j ] ) << ( 56 - 8 * j );
				}
				else
				{
					n = bswapq( *( const uint64_t* ) &block[ ( i * 8 ) ] );
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
		size_t bit_count = 0;
		block_type leftover = { 0 };
		size_t leftover_offset = 0;
		bool finalized = false;

		// Construct a new hash from an optional IV of 512-bit value.
		//
		constexpr sha512() noexcept
			: iv{ default_iv } {}
		constexpr sha512( value_type result ) noexcept
			: iv{ result }, finalized( true ) {}
		constexpr sha512( value_type iv512, iv_tag ) noexcept
			: iv{ iv512 } {}

		// Default copy/move.
		//
		constexpr sha512( sha512&& ) noexcept = default;
		constexpr sha512( const sha512& ) = default;
		constexpr sha512& operator=( sha512&& ) noexcept = default;
		constexpr sha512& operator=( const sha512& ) = default;

		// Appends the given array of bytes into the hash value.
		//
		inline constexpr void add_bytes( const uint8_t* data, size_t n )
		{
			while ( n-- )
			{
				leftover[ leftover_offset++ ] = *data++;

				if ( leftover_offset == block_size )
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
		constexpr void finalize() noexcept
		{
			if ( finalized )
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

			// Pad with zeros and append the big endian length at the end.
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
		FORCE_INLINE constexpr value_type digest() const noexcept
		{
			if ( finalized ) [[likely]]
				return iv;
			sha512 clone{ *this };
			clone.finalize();
			return clone.iv;
		}

		// Explicit conversions.
		//
		constexpr value_type as512() const noexcept { return digest(); }
		constexpr uint64_t as64() const noexcept { return as512()[ 0 ]; }
		constexpr uint64_t as32() const noexcept { return uint32_t( as64() ); }

		// Implicit conversions.
		//
		constexpr operator value_type() const noexcept { return as512(); }
		constexpr operator uint64_t() const noexcept { return as64(); }

		// Conversion to human-readable format.
		//
		std::string to_string() const { return fmt::as_hex_string( digest() ); }

		// Basic comparison operators.
		//
		constexpr bool operator<( const sha512& o ) const noexcept { return digest() < o.digest(); }
		constexpr bool operator==( const sha512& o ) const noexcept { return digest() == o.digest(); }
		constexpr bool operator!=( const sha512& o ) const noexcept { return digest() != o.digest(); }
	};
	using sha512_t = typename sha512::value_type;
};

// Make it std::hashable.
//
namespace std
{
	template<>
	struct hash<xstd::sha512>
	{
		constexpr size_t operator()( const xstd::sha512& value ) const { return ( size_t ) value.as64(); }
	};
};