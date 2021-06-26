#pragma once
#include <stdint.h>
#include <math.h>
#include <optional>
#include <type_traits>
#include <numeric>
#include "type_helpers.hpp"
#include "intrinsics.hpp"

using bitcnt_t = int;

namespace xstd
{
	// Alignment helper.
	//
	template<typename T>
	__forceinline static constexpr T align_up( T value, size_t alignment )
	{
		using I = convert_int_t<T>;
		using U = convert_uint_t<T>;

		if ( std::is_constant_evaluated() )
		{
			U uval = bit_cast<U>( value );
			uval += U( ( -I( uval ) ) % alignment );
			return bit_cast<T>( uval  );
		}
		else
		{
			U uval = ( U ) ( value );
			uval += U( ( -I( uval ) ) % alignment );
			return ( T ) ( uval );
		}
	}
	template<typename T>
	__forceinline static constexpr T align_down( T value, size_t alignment )
	{
		using U = convert_uint_t<T>;
		if ( std::is_constant_evaluated() )
		{
			U uval = bit_cast<U>( value );
			uval -= uval % alignment;
			return bit_cast<T>( uval  );
		}
		else
		{
			U uval = ( U ) ( value );
			uval -= uval % alignment;
			return ( T ) ( uval );
		}
	}
	template<typename T>
	__forceinline static constexpr bool is_aligned( T value, size_t alignment )
	{		
		using U = convert_uint_t<T>;
		if ( std::is_constant_evaluated() )
			return !( bit_cast<U>( value ) % alignment );
		else
			return !( U( value ) % alignment );
	}

	// Extracts the sign bit from the given value.
	//
	template<Integral T>
	__forceinline static constexpr bool sgn( T type ) { return bool( type >> ( ( sizeof( T ) * 8 ) - 1 ) ); }

	// Micro-optimized implementation to trick MSVC into actually optimizing it, like Clang :).
	//
	namespace impl
	{
		__forceinline static constexpr bitcnt_t rshiftcnt( bitcnt_t n )
		{
			// If MSVC x86-64:
			//
#if MS_COMPILER && AMD64_TARGET && !defined(__INTELLISENSE__)
			// SAR/SHR/SHL will ignore anything besides [x % 64], which lets us 
			// optimize (64 - n) into [-n] by substracting modulo size {64}.
			//
			if ( !std::is_constant_evaluated() )
				return -n & 63;
#endif
			return 64 - n;
		}

		static constexpr auto bit_reverse_lookup_table = xstd::make_constant_series<0x100>( [] <int N> ( const_tag<N> )
		{
			uint8_t value = 0;
			for ( size_t n = 0; n != 8; n++ )
				if ( N & ( 1 << n ) )
					value |= 1 << ( 7 - n );
			return value;
		} );
	};

	// Implement platform-indepdenent bitwise operations.
	//
	__forceinline static constexpr bitcnt_t popcnt( uint64_t x )
	{
		// Optimized using intrinsics if not const evaluated.
		//
		if ( !std::is_constant_evaluated() )
		{
#if MS_COMPILER && AMD64_TARGET
			return ( bitcnt_t ) __popcnt64( x );
#elif __has_builtin(__builtin_popcountll)
			return ( bitcnt_t ) __builtin_popcountll( x );
#endif
		}
		bitcnt_t count = 0;
		for ( bitcnt_t i = 0; i < 64; i++, x >>= 1 )
			count += ( bitcnt_t ) ( x & 1 );
		return count;
	}
	__forceinline static constexpr bitcnt_t msb( uint64_t x )
	{
		// Optimized using intrinsics if not const evaluated.
		//
		if ( !std::is_constant_evaluated() )
		{
#if __has_builtin(__builtin_ctzll)
			return 63 - __builtin_clzll( x );
#elif MS_COMPILER && AMD64_TARGET
			unsigned long idx;
			return _BitScanReverse64( &idx, x ) ? idx : -1;
#endif
		}

		// Start scan loop, return idx if found, else -1.
		//
		for ( bitcnt_t i = 63; i >= 0; i-- )
			if ( x & ( 1ull << i ) )
				return i;
		return -1;
	}
	__forceinline static constexpr bitcnt_t lsb( uint64_t x )
	{
		// Optimized using intrinsics if not const evaluated.
		//
		if ( !std::is_constant_evaluated() )
		{
#if MS_COMPILER && AMD64_TARGET
			unsigned long idx;
			return _BitScanForward64( &idx, x ) ? idx : -1;
#elif __has_builtin(__builtin_ctzll)
			return x ? __builtin_ctzll( x ) : -1;
#endif
		}

		// Start scan loop, return idx if found, else -1.
		//
		for ( bitcnt_t i = 0; i <= 63; i++ )
			if ( x & ( 1ull << i ) )
				return i;
		return -1;
	}

	template<typename T>
	__forceinline static constexpr bool bit_set( T& value, bitcnt_t n )
	{
		using U = convert_uint_t<T>;

		if constexpr ( std::is_volatile_v<T> || xstd::Atomic<T> )
		{
#if AMD64_TARGET
			if constexpr ( sizeof( T ) == 8 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "lock btsq %2, %1" : "=@ccc" ( out ), "+m" ( *( uint64_t* ) &value ) : "Jr" ( uint64_t( n ) ) );
				return out;
#elif HAS_MS_EXTENSIONS
				return _interlockedbittestandset64( ( volatile long long* ) &value, n );
#endif
			}
			else if constexpr ( sizeof( T ) == 4 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "lock btsl %2, %1" : "=@ccc" ( out ), "+m" ( *( uint32_t* ) &value ) : "Jr" ( uint32_t( n ) ) );
				return out;
#elif HAS_MS_EXTENSIONS
				return _interlockedbittestandset( ( volatile long* ) &value, n );
#endif
			}
			else if constexpr ( sizeof( T ) == 2 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "lock btsw %2, %1" : "=@ccc" ( out ), "+m" ( *( uint16_t* ) &value ) : "Jr" ( uint16_t( n ) ) );
				return out;
#endif
			}
#endif
			auto& ref = *( std::atomic<U>* ) &value;
			U value = ref.load();
			while ( !ref.compare_exchange_strong( value, value | U( 1ull << n ) ) );
			return value & U( 1ull << n );
		}
		else if constexpr( xstd::Integral<T> )
		{
			if ( !std::is_constant_evaluated() && !__is_consteval( n ) )
			{
#if AMD64_TARGET
				if constexpr ( sizeof( T ) == 8 )
				{
#if GNU_COMPILER
					int out;
					asm volatile( "btsq %2, %1" : "=@ccc" ( out ), "+rm" ( *( uint64_t* ) &value ) : "Jr" ( uint64_t( n ) ) );
					return out;
#elif HAS_MS_EXTENSIONS
					return _bittestandset64( ( long long* ) &value, n );
#endif
				}
				else if constexpr ( sizeof( T ) == 4 )
				{
#if GNU_COMPILER
					int out;
					asm volatile( "btsl %2, %1" : "=@ccc" ( out ), "+rm" ( *( uint32_t* ) &value ) : "Jr" ( uint32_t( n ) ) );
					return out;
#elif HAS_MS_EXTENSIONS
					return _bittestandset( ( long* ) &value, n );
#endif
				}
				else if constexpr ( sizeof( T ) == 2 )
				{
#if GNU_COMPILER
					int out;
					asm volatile( "btsw %2, %1" : "=@ccc" ( out ), "+rm" ( *( uint16_t* ) &value ) : "Jr" ( uint16_t( n ) ) );
					return out;
#endif
				}
#endif
			}

			bool is_set = value & U( 1ull << n );
			value |= U( 1ull << n );
			return is_set;
		}
		else
		{
			static_assert( sizeof( T ) == -1, "Invalid type for bitwise op." );
			unreachable();
		}
	}

	template<typename T>
	__forceinline static constexpr bool bit_reset( T& value, bitcnt_t n )
	{
		using U = convert_uint_t<T>;

		if constexpr ( std::is_volatile_v<T> || xstd::Atomic<T> )
		{
#if AMD64_TARGET
			if constexpr ( sizeof( T ) == 8 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "lock btrq %2, %1" : "=@ccc" ( out ), "+m" ( *( uint64_t* ) &value ) : "Jr" ( uint64_t( n ) ) );
				return out;
#elif HAS_MS_EXTENSIONS
				return _interlockedbittestandreset64( ( volatile long long* ) &value, n );
#endif
			}
			else if constexpr ( sizeof( T ) == 4 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "lock btrl %2, %1" : "=@ccc" ( out ), "+m" ( *( uint32_t* ) &value ) : "Jr" ( uint32_t( n ) ) );
				return out;
#elif HAS_MS_EXTENSIONS
				return _interlockedbittestandreset( ( volatile long* ) &value, n );
#endif
			}
			else if constexpr ( sizeof( T ) == 2 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "lock btrw %2, %1" : "=@ccc" ( out ), "+m" ( *( uint16_t* ) &value ) : "Jr" ( uint16_t( n ) ) );
				return out;
#endif
			}
#endif
			auto& ref = *( std::atomic<U>* ) &value;
			U value = ref.load();
			while ( !ref.compare_exchange_strong( value, value & ~U( 1ull << n ) ) );
			return value & U( 1ull << n );
		}
		else if constexpr( xstd::Integral<T> )
		{
			bool is_set = value & U( 1ull << n );
			value &= ~U( 1ull << n );
			return is_set;
		}
		else
		{
			static_assert( sizeof( T ) == -1, "Invalid type for bitwise op." );
			unreachable();
		}
	}

	template<typename T>
	__forceinline static constexpr bool bit_complement( T& value, bitcnt_t n )
	{
		using U = convert_uint_t<T>;

		if constexpr ( std::is_volatile_v<T> || xstd::Atomic<T> )
		{
#if AMD64_TARGET
			if constexpr ( sizeof( T ) == 8 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "lock btcq %2, %1" : "=@ccc" ( out ), "+m" ( *( uint64_t* ) &value ) : "Jr" ( uint64_t( n ) ) );
				return out;
#endif
			}
			else if constexpr ( sizeof( T ) == 4 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "lock btcl %2, %1" : "=@ccc" ( out ), "+m" ( *( uint32_t* ) &value ) : "Jr" ( uint32_t( n ) ) );
				return out;
#endif
			}
			else if constexpr ( sizeof( T ) == 2 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "lock btcw %2, %1" : "=@ccc" ( out ), "+m" ( *( uint16_t* ) &value ) : "Jr" ( uint16_t( n ) ) );
				return out;
#endif
			}
#endif
			auto& ref = *( std::atomic<U>* ) &value;
			U value = ref.load();
			while ( !ref.compare_exchange_strong( value, value ^ U( 1ull << n ) ) );
			return value & U( 1ull << n );
		}
		else if constexpr ( xstd::Integral<T> )
		{
			bool is_set = value & U( 1ull << n );
			value ^= U( 1ull << n );
			return is_set;
		}
		else
		{
			static_assert( sizeof( T ) == -1, "Invalid type for bitwise op." );
			unreachable();
		}
	}

	template<typename T>
	__forceinline static constexpr bool bit_test( const T& value, bitcnt_t n )
	{
		using U = convert_uint_t<T>;
		if constexpr ( std::is_volatile_v<T> || xstd::Atomic<T> )
		{
			// If shift is constant and can be encoded as imm32/imm16/imm8, TEST will be faster.
			//
			if ( __is_consteval( n ) && n <= 31 )
			{
				U flag = 1ul << n;
				if constexpr ( sizeof( T ) == 8 )
					return ( *( volatile uint64_t* ) &value ) & flag;
				else if constexpr ( sizeof( T ) == 4 )
					return ( *( volatile uint32_t* ) &value ) & flag;
				else if constexpr ( sizeof( T ) == 2 )
					return ( *( volatile uint16_t* ) &value ) & flag;
				else if constexpr ( sizeof( T ) == 1 )
					return ( *( volatile uint8_t* ) &value ) & flag;
			}

#if AMD64_TARGET
			if constexpr ( sizeof( T ) == 8 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "btq %2, %1" : "=@ccc" ( out ) : "m" ( *( uint64_t* ) &value ), "Jr" ( uint64_t( n ) ) );
				return out;
#elif HAS_MS_EXTENSIONS
				return _bittest64( ( long long* ) &value, n );
#endif
			}
			else if constexpr ( sizeof( T ) == 4 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "btl %2, %1" : "=@ccc" ( out ) : "m" ( *( uint32_t* ) &value ), "Jr" ( uint32_t( n ) ) );
				return out;
#elif HAS_MS_EXTENSIONS
				return _bittest( ( long* ) &value, n );
#endif
			}
			else if constexpr ( sizeof( T ) == 2 )
			{
#if GNU_COMPILER
				int out;
				asm volatile( "btw %2, %1" : "=@ccc" ( out ) : "m" ( *( uint16_t* ) &value ), "Jr" ( uint16_t( n ) ) );
				return out;
#endif
			}
#endif
			auto& ref = *( std::atomic<U>* ) &value;
			return ref & U( 1ull << n );
		}
		else if constexpr ( xstd::Integral<T> )
		{
			return value & U( 1ull << n );
		}
		else
		{
			static_assert( sizeof( T ) == -1, "Invalid type for bitwise op." );
		}
	}
	template<Integral I>
	__forceinline static constexpr I bit_reverse( I value )
	{
		if constexpr ( sizeof( I ) == 1 )
		{
			return ( I ) impl::bit_reverse_lookup_table[ uint8_t( value ) ];
		}
		else
		{
			std::make_unsigned_t<I> u = 0;
			for ( size_t n = 0; n != sizeof( I ); n++ )
			{
				u <<= 8;
				u |= impl::bit_reverse_lookup_table[ uint8_t( value & 0xFF ) ];
				value >>= 8;
			}
			return ( I ) u;
		}

	}

	// Used to find a bit with a specific value in a linear memory region.
	//
	static constexpr size_t bit_npos = ( size_t ) -1;
	template<typename T>
	static constexpr size_t bit_find( const T* begin, const T* end, bool value, bool reverse = false )
	{
		constexpr size_t bit_size = sizeof( T ) * 8;
		using uint_t = std::make_unsigned_t<T>;
		using int_t =  std::make_signed_t<T>;
		const auto scanner = reverse ? msb : lsb;

		// Generate the xor mask, if we're looking for 1, -!1 will evaluate to 0,
		// otherwise -!0 will evaluate to 0xFF.. in order to flip all bits.
		//
		uint_t xor_mask = ( uint_t ) ( -( ( int_t ) !value ) );

		// Loop each block:
		//
		size_t n = 0;
		for ( auto it = begin; it != end; it++, n += bit_size )
		{
			// Return if we could find the bit in the block:
			//
			if ( bitcnt_t i = scanner( *it ^ xor_mask ); i >= 0 )
				return n + i;
		}

		// Return invalid index.
		//
		return bit_npos;
	}

	// Used to enumerate each set bit in the integer.
	//
	template<typename T>
	static constexpr void bit_enum( uint64_t mask, T&& fn, bool reverse = false )
	{
		const auto scanner = reverse ? msb : lsb;
		while ( true )
		{
			// If scanner returns negative, break.
			//
			bitcnt_t idx = scanner( mask );
			if ( idx < 0 ) return;

			// Reset the bit and invoke the callback.
			//
			bit_reset( mask, idx );
			fn( idx );
		}
	}

	// Generate a mask for the given variable size and offset.
	//
	__forceinline static constexpr uint64_t fill_bits( bitcnt_t bit_count, bitcnt_t bit_offset = 0 )
	{
		// If bit offset is negative, substract from bit count 
		// and zero it out.
		//
		bit_count += bit_offset < 0 ? bit_offset : 0; // CMOV
		bit_offset = bit_offset < 0 ? 0 : bit_offset; // CMOV

		// Provide constexpr safety.
		//
		if ( std::is_constant_evaluated() )
			if( bit_count <= 0 || bit_offset >= 64 )
				return 0;
		
		// Create the value by two shifts.
		//
		uint64_t value = bit_count > 0 ? ~0ull : 0; // CMOV
		value >>= impl::rshiftcnt( bit_count );
		value <<= bit_offset;

		// If offset overflows, zero out, else return.
		//
		return bit_offset >= 64 ? 0 : value; // CMOV
	}

	// Fills the bits of the uint64_t type after the given offset with the sign bit.
	// - We accept an [uint64_t] as the sign "bit" instead of a for 
	//   the sake of a further trick we use to avoid branches.
	//
	__forceinline static constexpr uint64_t fill_sign( uint64_t sign, bitcnt_t bit_offset = 0 )
	{
		// The XOR operation with 0b1 flips the sign bit, after which when we subtract
		// one to create 0xFF... for (1) and 0x00... for (0).
		//
		return ( ( sign ^ 1 ) - 1 ) << bit_offset;
	}

	// Extends the given integral type into uint64_t or int64_t.
	//
	template<Integral T>
	__forceinline static constexpr auto imm_extend( T imm )
	{
		if constexpr ( std::is_signed_v<T> ) return ( int64_t ) imm;
		else                                 return ( uint64_t ) imm;
	}

	// Zero extends the given integer.
	//
	__forceinline static constexpr uint64_t zero_extend( uint64_t value, bitcnt_t bcnt_src )
	{
		// Constexpr implementation, the VM does not like signed/overflowing shifts very much.
		//
		if ( std::is_constant_evaluated() )
		{
			return value & fill_bits( bcnt_src );
		}
		else
		{
			// Shift left matching the MSB and shift right.
			//
			value <<= impl::rshiftcnt( bcnt_src );
			value >>= impl::rshiftcnt( bcnt_src );
			return value;
		}
	}

	// Sign extends the given integer.
	//
	template<xstd::Integral I>
	__forceinline static constexpr int64_t sign_extend( I _value, bitcnt_t bcnt_src )
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
			value <<= impl::rshiftcnt( bcnt_src );
			value >>= impl::rshiftcnt( bcnt_src );
			return value;
		}
	}
};
