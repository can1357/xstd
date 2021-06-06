#pragma once
#include <stdint.h>
#include <type_traits>
#include <numeric>
#include <optional>
#include "assert.hpp"
#include "type_helpers.hpp"

namespace xstd
{
	// Checks whether or not a value can be clamped to a given type.
	//
	template<Integral T, Integral T2>
	__forceinline static constexpr bool range_check( T2 o )
	{
		// If signs mismatch:
		//
		if constexpr ( std::is_signed_v<T2> ^ std::is_signed_v<T> )
			return 0 <= o && size_t( o ) <= size_t( std::numeric_limits<T>::max() );
		else
			return std::numeric_limits<T>::min() <= o && o <= std::numeric_limits<T>::max();
	}

	// Narrows the given type in a safe manner.
	//
	template<Integral T, Integral T2> requires( sizeof( T ) <= sizeof( T2 ) || std::is_signed_v<T> != std::is_signed_v<T2> )
	__forceinline static constexpr T narrow_cast( T2 o )
	{
		dassert( range_check<T>( o ) );
		return ( T ) o;
	}
	template<Integral T, Integral T2> requires( sizeof( T ) <= sizeof( T2 ) || std::is_signed_v<T> != std::is_signed_v<T2> )
	__forceinline static constexpr std::optional<T> narrow_cast_s( T2 o )
	{
		std::optional<T> result = {};
		if ( range_check<T>( o ) )
			result.emplace( ( T ) o );
		return result;
	}
};