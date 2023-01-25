#pragma once
#include <stdint.h>
#include <stdexcept>
#include <string_view>
#include "intrinsics.hpp"
#include "logger.hpp"

// [[Configuration]]
// XSTD_ASSERT_MESSAGE: Sets the assert failure message.
// XSTD_ASSERT_LEVEL: Declares assert level, valid values are:
//  - 2: Both dassert and fassert will be evaluated. Debug mode default.
//  - 1: Only fassert will be evaluated. Release mode default. 
//  - 0: All asserts are ignored.
//
#ifndef XSTD_ASSERT_MESSAGE
	#define XSTD_ASSERT_MESSAGE( cc, file, line ) XSTD_ESTR( "Assertion failure [" cc "] at " file ":" line )
#endif
#ifndef XSTD_ASSERT_NO_TRACE
	#define XSTD_ASSERT_NO_TRACE !DEBUG_BUILD
#endif
#ifndef XSTD_ASSERT_LEVEL
	#if DEBUG_BUILD
		#define XSTD_ASSERT_LEVEL 2
	#else
		#define XSTD_ASSERT_LEVEL 1
	#endif
#endif

namespace xstd
{
	// A helper to throw formatted error messages.
	//
	template<typename... params>
	FORCE_INLINE inline void throw_fmt [[noreturn]] ( const char* fmt_str, params&&... ps )
	{
		// Format error message.
		//
#if XSTD_NO_EXCEPTIONS
		error( fmt_str, std::forward<params>( ps )... );
#else
		if constexpr ( sizeof...( params ) != 0 )
		{
			throw std::runtime_error( fmt::str(
				fmt_str,
				std::forward<params>( ps )...
			) );
		}
		else
		{
			throw std::runtime_error( fmt_str );
		}
#endif
	}

	// Aborts if the given condition is not met.
	//
	template<typename... Tx>
	FORCE_INLINE inline constexpr void assert_that( bool condition, const char* fmt_str, Tx&&... ps )
	{
		if ( !condition ) [[unlikely]]
		{
			if ( std::is_constant_evaluated() ) unreachable();
			else                                throw_fmt( fmt_str, std::forward<Tx>( ps )... );
		}
	}
	FORCE_INLINE inline constexpr void assert_that( bool condition )
	{
		if ( !condition ) [[unlikely]]
		{
			if ( std::is_constant_evaluated() ) unreachable();
			else                                throw_fmt( XSTD_ESTR( "Assertation failed." ) );
		}
	}

	// Same as the one above but takes the format string as a lambda to avoid failure where the the string
	// cannot be converted to const char* during constexpr evaluation.
	//
	template<typename F>
	FORCE_INLINE inline constexpr void xassert_helper( bool condition, F&& getter )
	{
		if ( std::is_constant_evaluated() ) assert_that( condition );
		else                                assert_that( condition, getter() );
	}
};

// Declare main assert macro.
//
#define xassert(...) xstd::xassert_helper((bool)(__VA_ARGS__), []{ return XSTD_ASSERT_MESSAGE( xstringify( __VA_ARGS__ ), __FILE__, xstringify( __LINE__ ) ); } )

// Declare assertions, dassert is debug mode only, fassert is demo mode only, _s helpers 
// have the same functionality but still evaluate the statement.
//
#if ( XSTD_ASSERT_LEVEL >= 2 || defined(__INTELLISENSE__) )
	#define dassert(...)     xassert( __VA_ARGS__ )
	#define dassert_s( ... ) xassert( __VA_ARGS__ )
	#define fassert(...)     xassert( __VA_ARGS__ )
	#define fassert_s( ... ) xassert( __VA_ARGS__ )
#elif XSTD_ASSERT_LEVEL >= 1
	#define dassert(...)     do{}while(false)
	#define dassert_s( ... ) ( ( void ) (__VA_ARGS__ ) )
	#define fassert(...)     xassert( __VA_ARGS__ )
	#define fassert_s( ... ) xassert( __VA_ARGS__ )
#else
	#define dassert(...)     do{}while(false)
	#define dassert_s( ... ) ( ( void ) (__VA_ARGS__ ) )
	#define fassert(...)     do{}while(false)
	#define fassert_s( ... ) ( ( void ) (__VA_ARGS__ ) )
#endif