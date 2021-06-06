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
	__forceinline static constexpr bool narrow_viable( Src o )
	{
		return ( ( Dst ) o ) == o;
	}

	// Narrows the given type in a safe manner.
	//
	template<Integral Dst, Integral Src>
	__forceinline static constexpr Dst narrow_cast( Src o )
	{
		dassert( narrow_viable<Dst>( o ) );
		return ( Dst ) o;
	}
	template<Integral Dst, Integral Src>
	__forceinline static constexpr std::optional<Dst> narrow_cast_s( Src o )
	{
		std::optional<Dst> result = {};
		if ( narrow_viable<Dst>( o ) )
			result.emplace( ( Dst ) o );
		return result;
	}
};