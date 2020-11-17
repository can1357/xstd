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
#include <stdexcept>
#include <string_view>
#include "intrinsics.hpp"
#include "logger.hpp"

// [Configuration]
// XSTD_ASSERT_NO_TRACE: If set, will not leak the file name / line information in string format into the final binary.
//
#ifndef XSTD_ASSERT_NO_TRACE
	#define XSTD_ASSERT_NO_TRACE !DEBUG_BUILD
#endif

// [Configuration]
// XSTD_NO_EXCEPTIONS: If set, exceptions will resolve into error instead.
//
#ifndef XSTD_NO_EXCEPTIONS
	#define XSTD_NO_EXCEPTIONS 0
#endif

// [Configuration]
// XSTD_ASSERT_LEVEL: Declares assert level, valid values are:
//  - 2: Both dassert and fassert will be evaluated. Debug mode default.
//  - 1: Only fassert will be evaluated. Release mode default. 
//  - 0: All asserts are ignored.
// 
#ifndef XSTD_ASSERT_LEVEL
	#if DEBUG_BUILD
		#define XSTD_ASSERT_LEVEL 2
	#else
		#define XSTD_ASSERT_LEVEL 1
	#endif
#endif

namespace xstd
{
	// Aborts if the given condition is met.
	//
	__forceinline static constexpr void abort_if( bool condition, const char* string )
	{
		// If condition met:
		//
		if ( condition )
		{
			// Throw exception if consteval, else invoke logger error.
			//
			if ( std::is_constant_evaluated() ) throw std::logic_error{ string };
			else                                error( XSTD_ESTR( "%s" ), string );
		}
	}

	// A helper to throw formatted error messages.
	//
	template<typename... params>
	__forceinline static void throw_fmt [[noreturn]] ( const char* fmt, params&&... ps )
	{
		// Format error message.
		//
#if XSTD_NO_EXCEPTIONS
		error( fmt, std::forward<params>( ps )... );
#else
		throw std::runtime_error( format::str(
			fmt,
			std::forward<params>( ps )...
		) );
#endif
	}
};

// Declare main assert macro.
//
#define xassert__stringify(x) #x
#define xassert__istringify(x) xassert__stringify(x)

#if XSTD_ASSERT_NO_TRACE
	#define xassert(...) xstd::abort_if(!bool(__VA_ARGS__), "" )
#else
	#define xassert(...) xstd::abort_if(!bool(__VA_ARGS__), "Assertion failure, " xassert__stringify(__VA_ARGS__) " at " __FILE__ ":" xassert__istringify(__LINE__) )
#endif

// Declare assertions, dassert is debug mode only, fassert is demo mode only, _s helpers 
// have the same functionality but still evaluate the statement.
//
#if XSTD_ASSERT_LEVEL >= 2
	#define dassert(...)     xassert( __VA_ARGS__ )
	#define dassert_s( ... ) xassert( __VA_ARGS__ )
	#define fassert(...)     xassert( __VA_ARGS__ )
	#define fassert_s( ... ) xassert( __VA_ARGS__ )
#elif XSTD_ASSERT_LEVEL >= 1
	#define dassert(...)     
	#define dassert_s( ... ) ( __VA_ARGS__ )
	#define fassert(...)     xassert( __VA_ARGS__ )
	#define fassert_s( ... ) xassert( __VA_ARGS__ )
#else
	#define dassert(...)     
	#define dassert_s( ... ) ( __VA_ARGS__ )
	#define fassert(...)     
	#define fassert_s( ... ) ( __VA_ARGS__ )
#endif