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
#include <math.h>
#include <string>
#include "zip.hpp"
#include "reverse_iterator.hpp"
#include "formatting.hpp"
#include "type_helpers.hpp"

// Trivial types with useful explict formatting wrappers.
//
namespace xstd::fmt
{
	// Explicit integer formatting.
	//
	template<Integral T>
	struct decimal
	{
		T value = 0;
		constexpr decimal() {}
		constexpr decimal( T value ) : value( value ) {}
		constexpr operator T& ( ) { return value; }
		constexpr operator const T& ( ) const { return value; }
		std::string to_string() const
		{
			return std::to_string( value );
		}
	};
	template<Integral T>
	struct hexadecimal
	{
		T value = 0;
		constexpr hexadecimal() {}
		constexpr hexadecimal( T value ) : value( value ) {}
		constexpr operator T& ( ) { return value; }
		constexpr operator const T& ( ) const { return value; }

		std::string to_string() const
		{
			return format::hex( value );
		}
	};

	// Explicit memory/file size formatting.
	//
	template<Integral T = size_t>
	struct byte_count
	{
		inline static const std::array unit_abbrv = { XSTD_STR( "b" ), XSTD_STR( "kb" ), XSTD_STR( "mb" ), XSTD_STR( "gb" ), XSTD_STR( "tb" ) };

		T value = 0;
		constexpr byte_count() {}
		constexpr byte_count( T value ) : value( value ) {}
		constexpr operator T& ( ) { return value; }
		constexpr operator const T& ( ) const { return value; }

		std::string to_string() const
		{
			// Convert to double.
			//
			double fvalue = ( double ) value;

			// Iterate unit list in descending order.
			//
			for ( auto [abbrv, i] : backwards( zip( unit_abbrv, iindices ) ) )
			{
				double limit = pow( 1024.0, i );

				// If value is larger than the unit given or if we're at the last unit:
				//
				if ( std::abs( fvalue ) >= limit || abbrv == *std::begin( unit_abbrv ) )
				{
					// Convert float to string.
					//
					char buffer[ 32 ];
					snprintf( buffer, 32, XSTD_CSTR( "%.1lf%s" ), fvalue / limit, abbrv );
					return buffer;
				}
			}
			unreachable();
		}
	};

	// Explicit character formatting.
	//
	template<Integral T = char>
	struct character
	{
		T value = '\x0';
		constexpr character() {}
		constexpr character( T value ) : value( value ) {}
		constexpr operator T& ( ) { return value; }
		constexpr operator const T& ( ) const { return value; }

		std::string to_string() const
		{
			if ( !value ) return "";
			else          return std::string( 1, ( char ) value );
		}
	};

	// Explicit percentage formatting.
	//
	template<FloatingPoint T = double>
	struct percentage
	{
		T value = 0.0f;
		constexpr percentage() {}
		constexpr percentage( T value ) : value( value ) {}
		constexpr operator T& ( ) { return value; }
		constexpr operator const T& ( ) const { return value; }
		
		// Additional constructor for ratio.
		//
		template<Integral I>
		constexpr percentage( I a, I b ) : value( T(a)/T(b) ) {}

		std::string to_string() const
		{
			char buffer[ 32 ];
			snprintf( buffer, 32, XSTD_CSTR( "%.2lf%%" ), double( value * 100 ) );
			return buffer;
		}
	};

	// Explicit enum naming.
	//
	template<Enum T>
	struct named_enum
	{
		T value = {};
		constexpr named_enum() {}
		constexpr named_enum( T value ) : value( value ) {}
		constexpr operator T& ( ) { return value; }
		constexpr operator const T& ( ) const { return value; }

		std::string to_string() const
		{
			return enum_name<T>::resolve( value );
		}
	};
};
