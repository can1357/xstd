#pragma once
#include <iterator>
#include "bitwise.hpp"

namespace xstd
{
	// Declares a bitmap of size N with fast search helpers.
	//
	template<size_t N>
	struct bitmap
	{
		// Declare invalid iterator and block count.
		//
		static constexpr size_t npos = bit_npos;
		static constexpr size_t block_count = ( N + 63 ) / 64;

		// Store the bits, initialized to zero.
		//
		uint64_t blocks[ block_count ] = { 0 };

		// Default construction / copy / move.
		//
		constexpr bitmap() = default;
		constexpr bitmap( bitmap&& ) noexcept = default;
		constexpr bitmap( const bitmap& ) = default;
		constexpr bitmap& operator=( bitmap&& ) noexcept = default;
		constexpr bitmap& operator=( const bitmap& ) = default;

		// Find any bit with the given value in the array.
		//
		constexpr size_t find( bool value ) const
		{
			// Invoke find bit.
			//
			size_t idx = bit_find( std::begin( blocks ), std::end( blocks ), value );
			
			// If block has leftovers, adjust for overflow.
			//
			if constexpr ( ( block_count * 64 ) != N )
			{
				if ( idx > N )
					idx = bit_npos;
			}

			// Return the index.
			//
			return idx;
		}

		// Gets the value of the Nth bit.
		//
		constexpr bool get( size_t n ) const
		{
			dassert( n < N );
			return bit_test( blocks[ n / 64 ], n & 63 );
		}

		// Sets the value of the Nth bit.
		//
		constexpr bool set( size_t n, bool v )
		{
			dassert( n < N );
			if ( v ) return bit_set( blocks[ n / 64 ], n & 63 );
			else     return bit_reset( blocks[ n / 64 ], n & 63 );
		}
	};
};