// Copyright (c) 2020, Can Boluk
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
#pragma once
#include <stdint.h>
#include <type_traits>
#include <numeric>
#include "type_helpers.hpp"

namespace xstd
{
	// Narrows the given type in a safe manner.
	//
	template<Integral T, Integral T2> requires( sizeof( T ) <= sizeof( T2 ) || std::is_signed_v<T> != std::is_signed_v<T2> )
	__forceinline static constexpr T narrow_cast( T2 o )
	{
		if constexpr ( std::is_signed_v<T2> ^ std::is_signed_v<T> )
			dassert( 0 <= o && o <= std::numeric_limits<T>::max() );
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