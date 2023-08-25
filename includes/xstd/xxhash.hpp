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
	// Common traits.
	//
	template<typename T>
	struct xxhash_traits;
	template<>
	struct xxhash_traits<uint32_t> {
		static constexpr uint32_t prime_1 = 0x9E3779B1U;
		static constexpr uint32_t prime_2 = 0x85EBCA77U;
		static constexpr uint32_t prime_3 = 0xC2B2AE3DU;
		static constexpr uint32_t prime_4 = 0x27D4EB2FU;
		static constexpr uint32_t prime_5 = 0x165667B1U;

		FORCE_INLINE static constexpr uint32_t round( uint32_t acc, uint32_t input ) {
			acc += input * prime_2;
			acc = rotl( acc, 13 );
			acc *= prime_1;
			return acc;
		}
		FORCE_INLINE static constexpr std::array<uint32_t, 4> vec_round( std::array<uint32_t, 4> acc, std::array<uint32_t, 4> input ) {
#if XSTD_VECTOR_EXT && CLANG_COMPILER
			using vec = native_vector<uint32_t, 4>;
			vec vacc = xstd::bit_cast<vec>( acc );
			vec vinp = xstd::bit_cast<vec>( input );
			vinp *= prime_2;
			vacc += vinp;
			vacc = ( vacc << 13 ) | ( vacc >> ( 32 - 13 ) );
			vacc *= prime_1;
			return xstd::bit_cast<std::array<uint32_t, 4>>( vacc );
#else
			for ( size_t i = 0; i != 4; i++ ) input[ i ] *= prime_2;
			for ( size_t i = 0; i != 4; i++ ) acc[ i ] += input[ i ];
			for ( size_t i = 0; i != 4; i++ ) acc[ i ] = rotl( acc[ i ], 13 );
			for ( size_t i = 0; i != 4; i++ ) acc[ i ] *= prime_1;
			return acc;
#endif
		}
		FORCE_INLINE static constexpr uint32_t avalanche( uint32_t hash ) {
			hash ^= hash >> 15;
			hash *= prime_2;
			hash ^= hash >> 13;
			hash *= prime_3;
			hash ^= hash >> 16;
			return hash;
		}
		FORCE_INLINE static constexpr uint32_t digest( std::array<uint32_t, 4> iv, size_t len, const uint8_t* leftover ) {
			uint32_t hash;
			if ( len >= 16 ) {
				hash = rotl( iv[ 0 ], 1 )
					+ rotl( iv[ 1 ], 7 )
					+ rotl( iv[ 2 ], 12 )
					+ rotl( iv[ 3 ], 18 );
			} else {
				hash = iv[ 2 ] /* == seed */ + prime_5;
			}
			hash += (uint32_t) len;

			len &= 15;
			const uint8_t* p = leftover;
			while ( len >= 4 ) {
				uint32_t v = *(const uint32_t*) p;
				hash += v * prime_3;
				hash = rotl( hash, 17 ) * prime_4;
				p += 4;
				len -= 4;
			}
			while ( len > 0 ) {
				hash += ( *p++ ) * prime_5;
				hash = rotl( hash, 11 ) * prime_1;
				--len;
			}
			return avalanche( hash );
		}
	};
	template<>
	struct xxhash_traits<uint64_t> {
		static constexpr uint64_t prime_1 = 0x9E3779B185EBCA87ULL;
		static constexpr uint64_t prime_2 = 0xC2B2AE3D27D4EB4FULL;
		static constexpr uint64_t prime_3 = 0x165667B19E3779F9ULL;
		static constexpr uint64_t prime_4 = 0x85EBCA77C2B2AE63ULL;
		static constexpr uint64_t prime_5 = 0x27D4EB2F165667C5ULL;

		FORCE_INLINE static constexpr uint64_t round( uint64_t acc, uint64_t input ) {
			acc += input * prime_2;
			acc = rotl( acc, 31 );
			acc *= prime_1;
			return acc;
		}
		FORCE_INLINE static constexpr std::array<uint64_t, 4> vec_round( std::array<uint64_t, 4> acc, std::array<uint64_t, 4> input ) {
#if XSTD_VECTOR_EXT && CLANG_COMPILER
			using vec = native_vector<uint64_t, 4>;
			vec vacc = xstd::bit_cast<vec>( acc );
			vec vinp = xstd::bit_cast<vec>( input );
			vinp *= prime_2;
			vacc += vinp;
			vacc = ( vacc << 31 ) | ( vacc >> ( 64 - 31 ) );
			vacc *= prime_1;
			return xstd::bit_cast<std::array<uint64_t, 4>>( vacc );
#else
			for ( size_t i = 0; i != 4; i++ ) input[ i ] *= prime_2;
			for ( size_t i = 0; i != 4; i++ ) acc[ i ] += input[ i ];
			for ( size_t i = 0; i != 4; i++ ) acc[ i ] = rotl( acc[ i ], 31 );
			for ( size_t i = 0; i != 4; i++ ) acc[ i ] *= prime_1;
			return acc;
#endif
		}
		FORCE_INLINE static constexpr uint64_t avalanche( uint64_t hash ) {
			hash ^= hash >> 33;
			hash *= prime_2;
			hash ^= hash >> 29;
			hash *= prime_3;
			hash ^= hash >> 32;
			return hash;
		}
		FORCE_INLINE static constexpr uint64_t digest( std::array<uint64_t, 4> iv, size_t len, const uint8_t* leftover ) {
			uint64_t hash;
			if ( len >= 32 ) {
				hash = rotl( iv[ 0 ], 1 ) + rotl( iv[ 1 ], 7 ) + rotl( iv[ 2 ], 12 ) + rotl( iv[ 3 ], 18 );
				iv = vec_round( { 0, 0, 0, 0 }, iv );
				hash = ( hash ^ iv[ 0 ] ) * prime_1 + prime_4;
				hash = ( hash ^ iv[ 1 ] ) * prime_1 + prime_4;
				hash = ( hash ^ iv[ 2 ] ) * prime_1 + prime_4;
				hash = ( hash ^ iv[ 3 ] ) * prime_1 + prime_4;
			} else {
				hash = iv[ 2 ] /* == seed */ + prime_5;
			}
			hash += (uint64_t) len;

			len &= 31;
			const uint8_t* p = leftover;
			while ( len >= 8 ) {
				uint64_t v = round( 0, *(const uint64_t*) p );
				hash ^= v;
				hash = rotl( hash, 27 ) * prime_1 + prime_4;
				p += 8;
				len -= 8;
			}
			if ( len >= 4 ) {
				hash ^= uint64_t( *(const uint32_t*) p ) * prime_1;
				hash = rotl( hash, 23 ) * prime_2 + prime_3;
				p += 4;
				len -= 4;
			}
			while ( len > 0 ) {
				hash ^= ( *p++ ) * prime_5;
				hash = rotl( hash, 11 ) * prime_1;
				--len;
			}
			return avalanche( hash );
		}
	};

	// Common XXHASH implementation.
	//
	template<typename U>
	struct basic_xxhash
	{
		using traits =     xxhash_traits<U>;
		using value_type = U;
		using block_type = std::array<U, 4>;
		static constexpr size_t block_size =  sizeof( U ) * 4;
		static constexpr size_t digest_size = sizeof( U );
		
		// Crypto state.
		//
		size_t                               input_length = 0;
		std::array<U, 4>                     iv = {};
		std::array<uint8_t, sizeof( U ) * 4> leftover = {};

		// Default seed connstruction.
		//
		constexpr basic_xxhash() noexcept
			: basic_xxhash{ value_type(0) } { }

		// Custom seed construction.
		//
		constexpr basic_xxhash( value_type seed ) noexcept {
			iv = {
				seed + traits::prime_1 + traits::prime_2,
				seed + traits::prime_2,
				seed + 0,
				seed - traits::prime_1,
			};
		}

		// Default copy/move.
		//
		constexpr basic_xxhash( basic_xxhash&& ) noexcept = default;
		constexpr basic_xxhash( const basic_xxhash& ) = default;
		constexpr basic_xxhash& operator=( basic_xxhash&& ) noexcept = default;
		constexpr basic_xxhash& operator=( const basic_xxhash& ) = default;

		// Returns whether or not hash is finalized.
		//
		FORCE_INLINE constexpr bool finalized() const { return input_length == std::string::npos; }

		// Skips to next block.
		//
		FORCE_INLINE constexpr void compress( const uint8_t* block ) {
			std::array<U, 4> data;
			if ( std::is_constant_evaluated() ) {
				for ( U& v : data ) {
					U n = 0;
					for ( size_t j = 0; j != sizeof( U ); j++ )
						n |= U( *block++ ) << ( 8 * j );
					v = n;
				}
			} else {
				data = *( const std::array<U, 4>* ) block;
			}
			iv = traits::vec_round( iv, data );
		}
		FORCE_INLINE constexpr void next_block() {
			compress( leftover.data() );
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
			if ( size_t offset = prev_length % leftover.size() ) {
				// Pad with new data.
				//
				size_t space_available = leftover.size() - offset;
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
			while ( n >= leftover.size() ) {
				compress( data );
				n -=    leftover.size();
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
		FORCE_INLINE constexpr void add_bytes( const T& data ) noexcept
		{
			if ( std::is_constant_evaluated() )
			{
				using array_t = std::array<uint8_t, sizeof( T )>;
				array_t arr = xstd::bit_cast< array_t >( data );
				add_bytes( arr.data(), arr.size() );
			}
			else
			{
				add_bytes( ( const uint8_t* ) &data, sizeof( T ) );
			}
		}

		// Update wrapper.
		//
		template<typename T>
		FORCE_INLINE constexpr basic_xxhash& update(const T& data) noexcept { this->add_bytes<T>(data); return *this; };
		FORCE_INLINE constexpr basic_xxhash& update(const uint8_t* data, size_t n) { this->add_bytes(data, n); return *this; }

		// Finalization of the hash.
		//
		FORCE_INLINE constexpr basic_xxhash& finalize() noexcept
		{
			if ( finalized() ) return *this;
			iv[ 0 ] =      traits::digest( iv, input_length, &leftover[ 0 ] );
			input_length = std::string::npos;
			return *this;
		}
		FORCE_INLINE constexpr value_type digest() noexcept { return finalize().iv[ 0 ]; }
		FORCE_INLINE constexpr value_type digest() const noexcept
		{
			if ( finalized() ) [[likely]]
				return iv;
			auto clone{ *this };
			return clone.digest();
		}

		// Explicit conversions.
		//
		constexpr uint32_t as32() const noexcept { return uint32_t( digest()[ 0 ] ); }
		constexpr uint64_t as64() const noexcept { return uint64_t( digest()[ 0 ] ); }

		// Implicit conversions.
		//
		constexpr operator uint64_t() const noexcept { return as64(); }
		constexpr operator uint32_t() const noexcept { return as32(); }

		// Conversion to human-readable format.
		//
		std::string to_string() const { return fmt::as_hex_string( digest() ); }

		// Basic comparison operators.
		//
		constexpr bool operator<( const basic_xxhash& o ) const noexcept { return digest() < o.digest(); }
		constexpr bool operator==( const basic_xxhash& o ) const noexcept { return digest() == o.digest(); }
		constexpr bool operator!=( const basic_xxhash& o ) const noexcept { return digest() != o.digest(); }
	};

	// Define XXH32 and XXH64.
	//
	using xxhash32 = basic_xxhash<uint32_t>;
	using xxhash64 = basic_xxhash<uint64_t>;
};

// Make it std::hashable.
//
namespace std
{
	template<typename T>
	struct hash<xstd::basic_xxhash<T>>
	{
		constexpr size_t operator()( const xstd::basic_xxhash<T>& value ) const { 
			return ( size_t ) value.as64(); 
		}
	};
};