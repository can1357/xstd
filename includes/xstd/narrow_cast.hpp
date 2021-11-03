#pragma once
#include <stdint.h>
#include <type_traits>
#include <numeric>
#include <optional>
#include "assert.hpp"
#include "type_helpers.hpp"

namespace xstd
{
	// Checks whether or not a value can be narrowed down to a given type.
	//
	template<Integral Dst, Integral Src>
	FORCE_INLINE inline constexpr bool narrow_viable( Src o )
	{
		// Src signed == Dst signed:
		//
		if constexpr ( Signed<Dst> == Signed<Src> )
		{
			return ( ( Dst ) o ) == o;
		}
		// Src signed, Dst unsigned:
		//
		else if constexpr ( Signed<Src> )
		{
			return o >= 0 && narrow_viable<Dst>( ( std::make_unsigned_t<Src> ) o );
		}
		// Dst signed, Src unsigned:
		//
		else
		{
			return o <= ( ( uint64_t ) std::numeric_limits<Dst>::max() );
		}
	}
	template<Integral Dst, Integral Src>
	FORCE_INLINE inline constexpr bool narrow_viable( Src o, int bits /*for bitfields.*/ )
	{
		bits = std::min<int>( bits, sizeof( Dst ) * 8 );

		// Validate sign.
		//
		if constexpr ( std::is_signed_v<Src> && !std::is_signed_v<Dst> )
		{
			if ( o < 0 )
				return false;
		}
		
		// If signed, [-2^(x-1), (2^(x-1))-1]:
		//
		if constexpr ( std::is_signed_v<Dst> )
		{
			const int64_t signed_max = ( 1ll << ( bits - 1 ) ) - 1;
			const int64_t signed_min = -( 1ll << ( bits - 1 ) );
			return signed_min <= o && o <= signed_max;
		}
		// If unsigned, [0, 2^x-1]:
		//
		else
		{
			const uint64_t unsigned_max = ( 1ull << bits ) - 1;
			return o <= unsigned_max;
		}
	}

	// Narrows the given type in a safe manner.
	//
	template<Integral Dst, Integral Src>
	FORCE_INLINE inline constexpr Dst narrow_cast( Src o )
	{
		dassert( narrow_viable<Dst>( o ) );
		return ( Dst ) o;
	}
	template<Integral Dst, Integral Src>
	FORCE_INLINE inline constexpr std::optional<Dst> narrow_cast_s( Src o )
	{
		std::optional<Dst> result = {};
		if ( narrow_viable<Dst>( o ) )
			result.emplace( ( Dst ) o );
		return result;
	}
};