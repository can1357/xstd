#pragma once
#include <stdint.h>
#include <type_traits>
#include <numeric>
#include "assert.hpp"
#include "type_helpers.hpp"

namespace xstd
{
	// Narrows the given type in a safe manner.
	//
	template<Integral T, Integral T2> requires( sizeof( T ) <= sizeof( T2 ) || std::is_signed_v<T> != std::is_signed_v<T2> )
	__forceinline static constexpr T narrow_cast( T2 o )
	{
		if constexpr ( std::is_signed_v<T2> ^ std::is_signed_v<T> )
			dassert( 0 <= o && T( o ) <= std::numeric_limits<T>::max() );
		else
			dassert( std::numeric_limits<T>::min() <= o && o <= std::numeric_limits<T>::max() );

		return ( T ) o;
	}
	template<Integral T, Integral T2> requires( sizeof( T ) <= sizeof( T2 ) || std::is_signed_v<T> != std::is_signed_v<T2> )
	__forceinline static constexpr T narrow_cast_s( T2 o )
	{
		return ( T ) std::clamp( 
			o, 
			std::min( ( T2 ) std::numeric_limits<T>::min(), std::numeric_limits<T2>::min() ), 
			( T2 ) std::numeric_limits<T>::max() 
		);
	}
};