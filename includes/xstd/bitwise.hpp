#pragma once
#include <cstdint>
#include <cmath>
#include <optional>
#include <type_traits>
#include <numeric>
#include "type_helpers.hpp"
#include "intrinsics.hpp"

// [[Configuration]]
// XSTD_HW_BITSCAN:   Determines the availability of hardware bit-scan.
// XSTD_HW_PDEP_PEXT: Determines the availability of hardware parallel bit deposit / extraction.
//
#ifndef XSTD_HW_BITSCAN
	#define XSTD_HW_BITSCAN   ( __has_builtin(__builtin_ctz) || ( AMD64_TARGET && MS_COMPILER ) )
#endif
#ifndef XSTD_HW_PDEP_PEXT
	#define XSTD_HW_PDEP_PEXT ( AMD64_TARGET && ( GNU_COMPILER || MS_COMPILER ) )
#endif

using bitcnt_t = int;
namespace xstd
{
	// Micro-optimized implementation details to trick MSVC into actually optimizing it, like Clang :).
	//
	namespace impl
	{
		template<typename To, typename T>
		FORCE_INLINE CONST_FN inline constexpr bool const_demote( T value )
		{
			return const_condition( To( value ) == value );
		}
		template<typename To, typename T1, typename T2>
		FORCE_INLINE CONST_FN inline constexpr bool const_demote_and( T1 a, T2 b )
		{
			return const_demote<To>( a & b ) || const_demote<To>( a ) || const_demote<To>( b );
		}
#if !CLANG_COMPILER
		inline constexpr auto bit_reverse_lookup_table = xstd::make_constant_series<0x100>( [] <int N> ( const_tag<N> )
		{
			uint8_t value = 0;
			for ( size_t n = 0; n != 8; n++ )
				if ( N & ( 1 << n ) )
					value |= 1 << ( 7 - n );
			return value;
		} );
#endif
	};

	// Implement platform-indepdenent bitwise operations.
	//
	template<Integral T = uint64_t>
	FORCE_INLINE CONST_FN inline constexpr bitcnt_t popcnt( T v ) {
		// Optimized using intrinsics if not const evaluated.
		//
		if ( !std::is_constant_evaluated() ) {
#if MS_COMPILER && AMD64_TARGET
			if constexpr ( sizeof( T ) <= 4 )
				return (bitcnt_t) __popcnt( v );
			else
				return (bitcnt_t) __popcnt64( v );
#elif __has_builtin(__builtin_popcount)
			if constexpr ( sizeof( T ) <= 4 )
				return (bitcnt_t) __builtin_popcount( v );
			else
				return (bitcnt_t) __builtin_popcountll( v );
#endif
		}

		if ( sizeof( T ) <= 4 ) {
			uint32_t x = uint32_t( v );
			x = x - ( ( x >> 1 ) & 0x55555555u );
			x = ( x & 0x33333333u ) + ( ( x >> 2 ) & 0x33333333u );
			x = ( ( ( x + ( x >> 4u ) ) & 0xF0F0F0Fu ) * 0x1010101u ) >> 24u;
			return bitcnt_t( x );
		} else {
			uint64_t x = uint64_t( v );
			x = x - ( ( x >> 1 ) & 0x5555555555555555ull );
			x = ( x & 0x3333333333333333ull ) + ( ( x >> 2u ) & 0x3333333333333333ull );
			x = ( ( ( x + ( x >> 4 ) ) & 0x0F0F0F0F0F0F0F0Full ) * 0x0101010101010101ull ) >> 56u;
			return bitcnt_t( x );
		}
	}
	template<Integral T = uint64_t>
	FORCE_INLINE CONST_FN inline constexpr bool bit_parity( T v ) {
		// Optimized using intrinsics if not const evaluated.
		//
		if ( !std::is_constant_evaluated() ) {
#if __has_builtin(__builtin_parity)
			if constexpr ( sizeof( T ) <= 4 )
				return ( bool ) __builtin_parity( v );
			else
				return ( bool ) __builtin_parityll( v );
#endif
		}
		return ( popcnt( v ) & 1 ) == 1;
	}
	template<Integral I>
	FORCE_INLINE CONST_FN inline constexpr I bit_reverse( I value ) {
#if CLANG_COMPILER
		switch ( sizeof( I ) ) {
			case 1:  return __builtin_bitreverse8( value );
			case 2:  return __builtin_bitreverse16( value );
			case 4:  return __builtin_bitreverse32( value );
			case 8:  return __builtin_bitreverse64( value );
			default: return 0;
		}
#else
		if constexpr ( sizeof( I ) == 1 ) {
			return ( I ) impl::bit_reverse_lookup_table[ uint8_t( value ) ];
		} else {
			std::make_unsigned_t<I> u = 0;
			for ( size_t n = 0; n != sizeof( I ); n++ ) {
				u <<= 8;
				u |= impl::bit_reverse_lookup_table[ uint8_t( value & 0xFF ) ];
				value >>= 8;
			}
			return ( I ) u;
		}
#endif
	}
	template<Integral T = uint64_t>
	FORCE_INLINE CONST_FN inline constexpr bitcnt_t lsb( T x ) {
		// Optimized using intrinsics if not const evaluated.
		//
#if XSTD_HW_BITSCAN
		if ( !std::is_constant_evaluated() ) {
#if MS_COMPILER && AMD64_TARGET
			if constexpr ( sizeof( T ) <= 4 ) {
				unsigned long idx;
				return _BitScanForward( &idx, x ) ? idx : -1;
			} else {
				unsigned long idx;
				return _BitScanForward64( &idx, x ) ? idx : -1;
			}
#elif __has_builtin(__builtin_ctz)
#if __has_builtin(__builtin_ctzs)
			if constexpr ( sizeof( T ) <= 2 )
				return x ? __builtin_ctzs( x ) : -1;
#endif
			if constexpr ( sizeof( T ) <= 4 )
				return x ? __builtin_ctz( x ) : -1;
			else
				return x ? __builtin_ctzll( x ) : -1;
#endif
		}
#endif

		if ( x ) {
			uint64_t y = uint64_t( x ) & uint64_t( -int64_t( x ) );
			bitcnt_t r = bool( y >> 32 );
			y |= y >> 32;
			r = ( r << 1 ) + bool( y & 0xffff0000 );
			r = ( r << 1 ) + bool( y & 0xff00ff00 );
			r = ( r << 1 ) + bool( y & 0xf0f0f0f0 );
			r = ( r << 1 ) + bool( y & 0xcccccccc );
			r = ( r << 1 ) + bool( y & 0xaaaaaaaa );
			return r;
		}
		return -1;
	}
	template<Integral T = uint64_t>
	FORCE_INLINE CONST_FN inline constexpr bitcnt_t msb( T x ) {
		// Optimized using intrinsics if not const evaluated.
		//
#if XSTD_HW_BITSCAN
		if ( !std::is_constant_evaluated() ) {
#if __has_builtin(__builtin_clz)
#if __has_builtin(__builtin_clzs)
			if constexpr ( sizeof( T ) <= 2 )
				return x ? 15 - __builtin_clzs( x ) : -1;
#endif
			if constexpr ( sizeof( T ) <= 4 )
				return x ? 31 - __builtin_clz( x ) : -1;
			else
				return x ? 63 - __builtin_clzll( x ) : -1;
#elif MS_COMPILER && AMD64_TARGET
			if constexpr ( sizeof( T ) <= 4 ) {
				unsigned long idx;
				return _BitScanReverse( &idx, x ) ? idx : -1;
			} else {
				unsigned long idx;
				return _BitScanReverse64( &idx, x ) ? idx : -1;
			}
#endif
		}
#endif

		if ( x ) {
			switch ( sizeof( T ) ) {
				case 1: return  7 - lsb( bit_reverse<uint8_t>( uint8_t( x ) ) );
				case 2: return 15 - lsb( bit_reverse<uint16_t>( uint16_t( x ) ) );
				case 4: return 31 - lsb( bit_reverse<uint32_t>( uint32_t( x ) ) );
				case 8: return 63 - lsb( bit_reverse<uint64_t>( uint64_t( x ) ) );
				default: return 0;
			}
		}
		return -1;
	}
	template<Unsigned T = uint64_t>
	FORCE_INLINE CONST_FN inline constexpr bool is_pow2( T x ) {
		return ( x & ( x - 1 ) ) == 0;
	}
	template<Integral T = uint64_t>
	FORCE_INLINE CONST_FN inline constexpr T bit_floor( T x ) {
		if ( x != 0 )
			x = T{ 1 } << ( msb( x ) - 1 );
		return x;
	}
	template<Integral T = uint64_t>
	FORCE_INLINE CONST_FN inline constexpr T bit_ceil( T x ) {
		T f = bit_floor( x );
		if ( f != x )
			x = f << 1;
		return x;
	}
	template<Signed T>
	FORCE_INLINE CONST_FN inline constexpr bool sgn( T type ) {
		return type < 0;
	}

	// Alignment helper.
	//
	template<typename T>
	FORCE_INLINE CONST_FN inline constexpr T align_up( T value, size_t alignment, bool safe = false )
	{
		using U = convert_uint_t<T>;
		if ( !safe || is_pow2( alignment ) ) {
#if __has_builtin(__builtin_align_up)
			if constexpr ( Same<T, any_ptr> )
				return __builtin_align_up( ( void* ) value, alignment );
			else
				return __builtin_align_up( value, alignment );
#else
#endif
			U x = U( value );
			x += alignment - 1;
			x &= ~( alignment - 1 );
			return T( x );
		} else {
			U x = U( value );
			x += alignment - 1;
			x -= x % alignment;
			return T( x );
		}
	}
	template<typename T>
	FORCE_INLINE CONST_FN inline constexpr T align_down( T value, size_t alignment, bool safe = false )
	{
		using U = convert_uint_t<T>;
		if ( !safe || is_pow2( alignment ) ) {
#if __has_builtin(__builtin_align_down)
			if constexpr ( Same<T, any_ptr> )
				return __builtin_align_down( ( void* ) value, alignment );
			else
				return __builtin_align_down( value, alignment );
#else
#endif
			U x = U( value );
			x &= ~( alignment - 1 );
			return T( x );
		} else {
			U x = U( value );
			x -= x % alignment;
			return T( x );
		}
	}
	template<typename T>
	FORCE_INLINE CONST_FN inline constexpr bool is_aligned( T value, size_t alignment, bool safe = false )
	{
		using U = convert_uint_t<T>;
		if ( !safe || is_pow2( alignment ) ) {
#if __has_builtin(__builtin_is_aligned)
			if constexpr ( Same<T, any_ptr> )
				return __builtin_is_aligned( ( void* ) value, alignment );
			else
				return __builtin_is_aligned( value, alignment );
#else
#endif
			U x = U( value );
			return ( x & ( alignment - 1 ) ) == 0;
		} else {
			U x = U( value );
			return ( x % alignment ) == 0;
		}
	}

	// Bit set / reset / complement and test.
	//
	template<typename T>
	FORCE_INLINE inline constexpr bool bit_set( T& value, bitcnt_t n )
	{
		using U = convert_uint_t<T>;

		if constexpr ( Volatile<T> || Atomic<T> )
		{
#if AMD64_TARGET
			if constexpr ( sizeof( T ) == 8 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "lock btsq %2, %1" : "=@ccc" ( out ), "+m" ( *( uint64_t* ) &value ) : "Jr" ( uint64_t( n ) ) );
				return bool( out );
#elif HAS_MS_EXTENSIONS
				return _interlockedbittestandset64( ( volatile long long* ) &value, n );
#endif
			}
			else if constexpr ( sizeof( T ) == 4 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "lock btsl %2, %1" : "=@ccc" ( out ), "+m" ( *( uint32_t* ) &value ) : "Jr" ( uint32_t( n ) ) );
				return bool( out );
#elif HAS_MS_EXTENSIONS
				return _interlockedbittestandset( ( volatile long* ) &value, n );
#endif
			}
			else if constexpr ( sizeof( T ) == 2 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "lock btsw %2, %1" : "=@ccc" ( out ), "+m" ( *( uint16_t* ) &value ) : "Jr" ( uint16_t( n ) ) );
				return bool( out );
#endif
			}
			else if constexpr ( sizeof( T ) == 1 )
			{
#if GNU_COMPILER
				auto ptr = ( uint64_t ) &value;
				if ( ptr & 1 )
					n += 8, ptr -= 1;
				return bit_set( *( volatile uint16_t* ) ptr, n );
#endif
			}
#endif
			U mask = U{ 1 } << n;
			auto& ref = *( std::atomic<U>* ) &value;
			return ( ref.fetch_or( mask ) & mask ) != 0;
		}
		else if constexpr( Integral<T> )
		{
			// If shift is constant and can be encoded as imm32/imm8, OR will be faster.
			//
			if ( !std::is_constant_evaluated() && ( !is_consteval( n ) || n >= 32 ) )
			{
#if AMD64_TARGET
				if constexpr ( sizeof( T ) == 8 )
				{
#if GNU_COMPILER
					int out;
					asm volatile( "btsq %2, %1" : "=@ccc" ( out ), "+r" ( *( uint64_t* ) &value ) : "Jr" ( uint64_t( n ) ) );
					return bool( out );
#elif HAS_MS_EXTENSIONS
					return _bittestandset64( ( volatile long long* ) &value, n );
#endif
				}
				else if constexpr ( sizeof( T ) == 4 )
				{
#if GNU_COMPILER
					int out;
					asm volatile( "btsl %2, %1" : "=@ccc" ( out ), "+r" ( *( uint32_t* ) &value ) : "Jr" ( uint32_t( n ) ) );
					return bool( out );
#elif HAS_MS_EXTENSIONS
					return _bittestandset( ( volatile long* ) &value, n );
#endif
				}
				else if constexpr ( sizeof( T ) == 2 )
				{
#if GNU_COMPILER
					int out;
					asm volatile( "btsw %2, %1" : "=@ccc" ( out ), "+r" ( *( uint16_t* ) &value ) : "Jr" ( uint16_t( n ) ) );
					return bool( out );
#endif
				}
#endif
			}

			bool is_set = U( value ) & ( U( 1 ) << n );
			value |= ( U(1) << n );
			return is_set;
		}
		else
		{
			static_assert( sizeof( T ) == -1, "Invalid type for bitwise op." );
			return false;
		}
	}
	template<typename T>
	FORCE_INLINE inline constexpr bool bit_reset( T& value, bitcnt_t n )
	{
		using U = convert_uint_t<T>;

		if constexpr ( Volatile<T> || Atomic<T> )
		{
#if AMD64_TARGET
			if constexpr ( sizeof( T ) == 8 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "lock btrq %2, %1" : "=@ccc" ( out ), "+m" ( *( uint64_t* ) &value ) : "Jr" ( uint64_t( n ) ) );
				return bool( out );
#elif HAS_MS_EXTENSIONS
				return _interlockedbittestandreset64( ( volatile long long* ) &value, n );
#endif
			}
			else if constexpr ( sizeof( T ) == 4 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "lock btrl %2, %1" : "=@ccc" ( out ), "+m" ( *( uint32_t* ) &value ) : "Jr" ( uint32_t( n ) ) );
				return bool( out );
#elif HAS_MS_EXTENSIONS
				return _interlockedbittestandreset( ( volatile long* ) &value, n );
#endif
			}
			else if constexpr ( sizeof( T ) == 2 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "lock btrw %2, %1" : "=@ccc" ( out ), "+m" ( *( uint16_t* ) &value ) : "Jr" ( uint16_t( n ) ) );
				return bool( out );
#endif
			}
			else if constexpr ( sizeof( T ) == 1 )
			{
#if GNU_COMPILER
				auto ptr = ( uint64_t ) &value;
				if ( ptr & 1 )
					n += 8, ptr -= 1;
				return bit_reset( *( volatile uint16_t* ) ptr, n );
#endif
			}
#endif
			U mask = U{ 1 } << n;
			auto& ref = *( std::atomic<U>* ) & value;
			return ( ref.fetch_and( ~mask ) & mask ) != 0;
		}
		else if constexpr( Integral<T> )
		{
			// If shift is constant and can be encoded as imm32/imm8, AND will be faster.
			//
			if ( !std::is_constant_evaluated() && ( !is_consteval( n ) || n >= 32 ) )
			{
#if AMD64_TARGET
				if constexpr ( sizeof( T ) == 8 )
				{
#if GNU_COMPILER
					int out;
					asm volatile( "btrq %2, %1" : "=@ccc" ( out ), "+r" ( *( uint64_t* ) &value ) : "Jr" ( uint64_t( n ) ) );
					return bool( out );
#elif HAS_MS_EXTENSIONS
					return _bittestandreset64( ( long long* ) &value, n );
#endif
				}
				else if constexpr ( sizeof( T ) == 4 )
				{
#if GNU_COMPILER
					int out;
					asm volatile( "btrl %2, %1" : "=@ccc" ( out ), "+r" ( *( uint32_t* ) &value ) : "Jr" ( uint32_t( n ) ) );
					return bool( out );
#elif HAS_MS_EXTENSIONS
					return _bittestandreset( ( long* ) &value, n );
#endif
				}
				else if constexpr ( sizeof( T ) == 2 )
				{
#if GNU_COMPILER
					int out;
					asm volatile( "btrw %2, %1" : "=@ccc" ( out ), "+r" ( *( uint16_t* ) &value ) : "Jr" ( uint16_t( n ) ) );
					return bool( out );
#endif
				}
#endif
			}

			bool is_set = U( value ) & ( U(1) << n );
			value &= ~( U(1) << n );
			return is_set;
		}
		else
		{
			static_assert( sizeof( T ) == -1, "Invalid type for bitwise op." );
			return false;
		}
	}
	template<typename T>
	FORCE_INLINE inline constexpr bool bit_complement( T& value, bitcnt_t n )
	{
		using U = convert_uint_t<T>;

		if constexpr ( Volatile<T> || Atomic<T> )
		{
#if AMD64_TARGET
			if constexpr ( sizeof( T ) == 8 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "lock btcq %2, %1" : "=@ccc" ( out ), "+m" ( *( uint64_t* ) &value ) : "Jr" ( uint64_t( n ) ) );
				return bool( out );
#endif
			}
			else if constexpr ( sizeof( T ) == 4 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "lock btcl %2, %1" : "=@ccc" ( out ), "+m" ( *( uint32_t* ) &value ) : "Jr" ( uint32_t( n ) ) );
				return bool( out );
#endif
			}
			else if constexpr ( sizeof( T ) == 2 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "lock btcw %2, %1" : "=@ccc" ( out ), "+m" ( *( uint16_t* ) &value ) : "Jr" ( uint16_t( n ) ) );
				return bool( out );
#endif
			}
			else if constexpr ( sizeof( T ) == 1 )
			{
#if GNU_COMPILER
				auto ptr = ( uint64_t ) &value;
				if ( ptr & 1 )
					n += 8, ptr -= 1;
				return bit_complement( *( volatile uint16_t* ) ptr, n );
#endif
			}
#endif
			U mask = U{ 1 } << n;
			auto& ref = *( std::atomic<U>* ) & value;
			return ( ref.fetch_xor( mask ) & mask ) != 0;
		}
		else if constexpr ( Integral<T> )
		{
			// If shift is constant and can be encoded as imm32/imm8, XOR will be faster.
			//
			if ( !std::is_constant_evaluated() && ( !is_consteval( n ) || n >= 32 ) )
			{
#if AMD64_TARGET
				if constexpr ( sizeof( T ) == 8 )
				{
#if GNU_COMPILER
					int out;
					asm volatile( "btcq %2, %1" : "=@ccc" ( out ), "+r" ( *( uint64_t* ) &value ) : "Jr" ( uint64_t( n ) ) );
					return bool( out );
#elif HAS_MS_EXTENSIONS
					return _bittestandcomplement64( ( volatile long long* ) &value, n );
#endif
				}
				else if constexpr ( sizeof( T ) == 4 )
				{
#if GNU_COMPILER
					int out;
					asm volatile( "btcl %2, %1" : "=@ccc" ( out ), "+r" ( *( uint32_t* ) &value ) : "Jr" ( uint32_t( n ) ) );
					return bool( out );
#elif HAS_MS_EXTENSIONS
					return _bittestandcomplement( ( volatile long* ) &value, n );
#endif
				}
				else if constexpr ( sizeof( T ) == 2 )
				{
#if GNU_COMPILER
					int out;
					asm volatile( "btcw %2, %1" : "=@ccc" ( out ), "+r" ( *( uint16_t* ) &value ) : "Jr" ( uint16_t( n ) ) );
					return bool( out );
#endif
				}
#endif
			}

			bool is_set = U( value ) & ( U(1) << n );
			value ^= ( U(1) << n );
			return is_set;
		}
		else
		{
			static_assert( sizeof( T ) == -1, "Invalid type for bitwise op." );
			return false;
		}
	}
	template<typename T>
	FORCE_INLINE inline constexpr bool bit_test( const T& value, bitcnt_t n )
	{
		using U = convert_uint_t<T>;
		if constexpr ( Volatile<T> || Atomic<T> )
		{
			// If shift is constant and can be encoded as imm32/imm8, TEST will be faster.
			//
			if ( is_consteval( n ) && n <= 31 )
			{
				U flag = U( 1 ) << n;
				return ( *( volatile U* ) &value ) & flag;
			}

#if AMD64_TARGET
			if constexpr ( sizeof( T ) == 8 )
			{
#if GNU_COMPILER
				int out;
				asm( "btq %2, %1" : "=@ccc" ( out ) : "m" ( *( uint64_t* ) &value ), "Jr" ( uint64_t( n ) ) );
				return bool( out );
#elif HAS_MS_EXTENSIONS
				return _bittest64( ( long long* ) &value, n );
#endif
			}
			else if constexpr ( sizeof( T ) == 4 )
			{
#if GNU_COMPILER
				int out;
				asm( "btl %2, %1" : "=@ccc" ( out ) : "m" ( *( uint32_t* ) &value ), "Jr" ( uint32_t( n ) ) );
				return bool( out );
#elif HAS_MS_EXTENSIONS
				return _bittest( ( long* ) &value, n );
#endif
			}
			else if constexpr ( sizeof( T ) == 2 )
			{
#if GNU_COMPILER
				int out;
				asm( "btw %2, %1" : "=@ccc" ( out ) : "m" ( *( uint16_t* ) &value ), "Jr" ( uint16_t( n ) ) );
				return bool( out );
#endif
			}
#endif
			return ( *( volatile U* ) &value ) & ( U(1) << n );
		}
		else if constexpr ( Integral<T> )
		{
			// If shift is constant and can be encoded as imm32/imm8, TEST will be faster.
			//
			if ( !std::is_constant_evaluated() && ( !is_consteval( n ) || n >= 32 ) )
			{
#if AMD64_TARGET
				if constexpr ( sizeof( T ) == 8 )
				{
#if GNU_COMPILER
					int out;
					asm( "btq %2, %1" : "=@ccc" ( out ) : "r" ( *( const uint64_t* ) &value ), "Jr" ( uint64_t( n ) ) );
					return bool( out );
#elif HAS_MS_EXTENSIONS
					return _bittest64( ( long long* ) &value, n );
#endif
				}
				else if constexpr ( sizeof( T ) == 4 )
				{
#if GNU_COMPILER
					int out;
					asm( "btl %2, %1" : "=@ccc" ( out ) : "r" ( *( const uint32_t* ) &value ), "Jr" ( uint32_t( n ) ) );
					return bool( out );
#elif HAS_MS_EXTENSIONS
					return _bittest( ( long* ) &value, n );
#endif
				}
				else if constexpr ( sizeof( T ) == 2 )
				{
#if GNU_COMPILER
					int out;
					asm( "btw %2, %1" : "=@ccc" ( out ) : "r" ( *( const uint16_t* ) &value ), "Jr" ( uint16_t( n ) ) );
					return bool( out );
#endif
				}
#endif
			}

			return U( value ) & ( U( 1 ) << n );
		}
		else
		{
			static_assert( sizeof( T ) == -1, "Invalid type for bitwise op." );
			return false;
		}
	}
	template<typename T>
	FORCE_INLINE inline bool atomic_bit_reset( T& value, bitcnt_t n ) 
	{
		if constexpr ( Integral<T> )
			return bit_reset( ( volatile T& ) value, n );
		else
			return bit_reset( value, n );
	}
	template<typename T>
	FORCE_INLINE inline bool atomic_bit_set( T& value, bitcnt_t n ) 
	{
		if constexpr ( Integral<T> )
			return bit_set( ( volatile T& ) value, n );
		else
			return bit_set( value, n );
	}
	template<typename T>
	FORCE_INLINE inline bool atomic_bit_complement( T& value, bitcnt_t n ) 
	{
		if constexpr ( Integral<T> )
			return bit_complement( ( volatile T& ) value, n );
		else
			return bit_complement( value, n );
	}

	// Parallel extraction/deposit.
	//
	template<Integral I = uint64_t>
	FORCE_INLINE CONST_FN inline constexpr I bit_pext( I value, I mask )
	{
		// Constant operand size demotion.
		//
		if constexpr ( sizeof( I ) > 1 )
			if ( impl::const_demote_and<uint8_t>( value, mask ) )
				return ( I ) bit_pext<uint8_t>( ( uint8_t ) value, ( uint8_t ) mask );
		if constexpr ( sizeof( I ) > 2 )
			if ( impl::const_demote_and<uint16_t>( value, mask ) )
				return ( I ) bit_pext<uint16_t>( ( uint16_t ) value, ( uint16_t ) mask );
		if constexpr ( sizeof( I ) > 4 )
			if ( impl::const_demote_and<uint32_t>( value, mask ) )
				return ( I ) bit_pext<uint32_t>( ( uint32_t ) value, ( uint32_t ) mask );

#if XSTD_HW_PDEP_PEXT && !defined(__INTELLISENSE__)
		if ( !std::is_constant_evaluated() )
		{
#if GNU_COMPILER
			if constexpr ( sizeof( I ) <= 4 )
				return ( I ) __builtin_ia32_pext_si( ( uint32_t ) value, ( uint32_t ) mask );
			else
				return ( I ) __builtin_ia32_pext_di( ( uint64_t ) value, ( uint64_t ) mask );
#elif MS_COMPILER
			if constexpr ( sizeof( I ) <= 4 )
				return ( I ) _pext_u32( ( uint32_t ) value, ( uint32_t ) mask );
			else
				return ( I ) _pext_u64( ( uint64_t ) value, ( uint64_t ) mask );
#endif
		}
#endif

		using U = convert_uint_t<I>;

		U result = 0;
		bitcnt_t k = 0;
		for ( bitcnt_t m = 0; m != ( sizeof( I ) * 8 ); m++ )
			if ( ( U( mask ) >> m ) & 1 )
				result |= ( ( U( value ) >> m ) & 1 ) << ( k++ );
		return ( I ) result;
	}

	template<Integral I = uint64_t>
	FORCE_INLINE CONST_FN inline constexpr I bit_pdep( I value, I mask )
	{
		// Constant operand size demotion.
		//
		if constexpr ( sizeof( I ) > 1 )
			if ( impl::const_demote<uint8_t>( mask ) )
				return ( I ) bit_pdep<uint8_t>( ( uint8_t ) value, ( uint8_t ) mask );
		if constexpr ( sizeof( I ) > 2 )
			if ( impl::const_demote<uint16_t>( mask ) )
				return ( I ) bit_pdep<uint16_t>( ( uint16_t ) value, ( uint16_t ) mask );
		if constexpr ( sizeof( I ) > 4 )
			if ( impl::const_demote<uint32_t>( mask ) )
				return ( I ) bit_pdep<uint32_t>( ( uint32_t ) value, ( uint32_t ) mask );

#if XSTD_HW_PDEP_PEXT && !defined(__INTELLISENSE__)
		if ( !std::is_constant_evaluated() )
		{
#if GNU_COMPILER
			if constexpr ( sizeof( I ) <= 4 )
				return ( I ) __builtin_ia32_pdep_si( ( uint32_t ) value, ( uint32_t ) mask );
			else
				return ( I ) __builtin_ia32_pdep_di( ( uint64_t ) value, ( uint64_t ) mask );
#elif MS_COMPILER
			if constexpr ( sizeof( I ) <= 4 )
				return ( I ) _pdep_u32( ( uint32_t ) value, ( uint32_t ) mask );
			else
				return ( I ) _pdep_u64( ( uint64_t ) value, ( uint64_t ) mask );
#endif
		}
#endif

		using U = convert_uint_t<I>;

		U result = 0;
		bitcnt_t k = 0;
		for ( bitcnt_t m = 0; m != ( sizeof( I ) * 8 ); m++ )
			if ( ( U( mask ) >> m ) & 1 )
				result |= ( ( U( value ) >> ( k++ ) ) & 1 ) << m;
		return ( I ) result;
	}

	// Used to find a bit with a specific value in a linear memory region.
	//
	inline constexpr int64_t bit_npos = -1ll;
	template<typename T>
	inline constexpr int64_t bit_find( const T* begin, const T* end, bool value, bool reverse = false )
	{
		constexpr bitcnt_t bit_size = sizeof( T ) * 8;
		using U = std::make_unsigned_t<T>;

		// Generate the xor mask.
		//
		const U xor_mask = value ? 0 : ~U( 0 );

		// Loop each block:
		//
		int64_t n = 0;
		for ( auto it = begin; it != end; it++, n += bit_size )
		{
			// Return if we could find the bit in the block:
			//
			if ( bitcnt_t i = reverse ? msb( *it ^ xor_mask ) : lsb( *it ^ xor_mask ); i >= 0 )
				return n + i;
		}

		// Return invalid index.
		//
		return bit_npos;
	}

	// Used to enumerate each set bit in the integer.
	//
	template<typename V, typename T>
	inline constexpr void bit_enum( V mask, T&& fn, bool reverse = false )
	{
		while ( mask )
		{
			bitcnt_t idx = reverse ? msb( mask ) : lsb( mask );
			bit_reset( mask, idx );
			fn( idx );
		}
	}

	// Generate a mask for the given variable size and offset.
	// - Undefined if [ bit count < 0 || bit count > 64 || bit offset < 0 ].
	//
	FORCE_INLINE CONST_FN inline constexpr uint64_t fill_bits( bitcnt_t bit_count, bitcnt_t bit_offset = 0 )
	{
		// Substract with borrow to handle bit count == 0.
		//
		uint64_t value = uint64_t( 0ll - ( bit_count != 0 ) );

		// Shift to right to remove bits, shift to left for adjustment.
		//
		value >>= ( 64 - bit_count );
		value <<= bit_offset;
		return value;
	}

	// Fills the bits of the uint64_t type after the given offset with the sign bit.
	// - We accept an [uint64_t] as the sign "bit" instead of a for 
	//   the sake of a further trick we use to avoid branches.
	//
	FORCE_INLINE CONST_FN inline constexpr uint64_t fill_sign( uint64_t sign, bitcnt_t bit_offset = 0 )
	{
		// The XOR operation with 0b1 flips the sign bit, after which when we subtract
		// one to create 0xFF... for (1) and 0x00... for (0).
		//
		return ( ( sign ^ 1 ) - 1 ) << bit_offset;
	}

	// Extends the given integral type into uint64_t or int64_t.
	//
	template<Integral T>
	FORCE_INLINE CONST_FN inline constexpr auto imm_extend( T imm )
	{
		if constexpr ( Signed<T> ) return ( int64_t ) imm;
		else                       return ( uint64_t ) imm;
	}

	// Signed/Unsigned bit-cast.
	//
	template<typename T>
	FORCE_INLINE CONST_FN inline constexpr std::make_signed_t<T> as_signed( T value ) {
		return xstd::bit_cast<std::make_signed_t<T>>( value );
	}
	template<typename T>
	FORCE_INLINE CONST_FN inline constexpr std::make_unsigned_t<T> as_unsigned( T value ) {
		return xstd::bit_cast<std::make_unsigned_t<T>>( value );
	}

	// Zero extends the given integer.
	//
	FORCE_INLINE CONST_FN inline constexpr uint64_t zero_extend( uint64_t value, bitcnt_t bcnt_src )
	{
		if ( std::is_constant_evaluated() || is_consteval( bcnt_src ) )
		{
			return value & fill_bits( bcnt_src );
		}
		else
		{
			value <<= ( 64 - bcnt_src );
			value >>= ( 64 - bcnt_src );
			return value;
		}
	}

	// Sign extends the given integer.
	//
	template<xstd::Integral I>
	FORCE_INLINE CONST_FN inline constexpr int64_t sign_extend( I _value, bitcnt_t bcnt_src )
	{
		// Constexpr implementation, the VM does not like signed/overflowing shifts very much.
		//
		if ( std::is_constant_evaluated() )
		{
			uint64_t value = ( std::make_unsigned_t<I> ) _value;

			// Handle edge cases.
			//
			if ( bcnt_src == 64 ) return ( int64_t ) value;
			if ( bcnt_src <= 1 )  return value & 1;

			// Save the sign bit and mask the value.
			//
			bool sgn = bit_test( value, bcnt_src - 1 );
			value &= fill_bits( bcnt_src - 1 );

			// Handle the sign bit.
			//
			if ( sgn )
			{
				value = ( ~value );
				value &= fill_bits( bcnt_src - 1 );
				return -( int64_t ) ( value + 1 );
			}
			else
			{
				return ( int64_t ) value;
			}
		}
		else
		{
			// Interprete as signed, shift left matching the MSB and shift right.
			//
			int64_t value = ( int64_t ) ( std::make_unsigned_t<I> ) _value;
			value <<= ( 64 - bcnt_src );
			value >>= ( 64 - bcnt_src );
			return value;
		}
	}
};
