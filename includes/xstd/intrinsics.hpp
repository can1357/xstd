#pragma once

#ifndef __has_include
	#define __has_include(...) 0
#endif

#if __has_include(<xstd/options.hpp>)
	#include <xstd/options.hpp>
#elif __has_include("xstd_options.hpp")
	#include "xstd_options.hpp"
#endif

#ifndef __has_builtin
	#define __has_builtin(...) 0
#endif
#ifndef __has_xcxx_builtin
	#define __has_xcxx_builtin(...) 0
#endif
#ifndef __has_attribute
	#define __has_attribute(...) 0
#endif
#ifndef __has_cpp_attribute
	#define __has_cpp_attribute(...) 0
#endif
#ifndef __has_feature
	#define __has_feature(...) 0
#endif

#include <cstdlib>
#include <string>
#include <cstdint>
#include <type_traits>
#include <typeinfo>
#include <atomic>
#include <array>
#include <tuple>
#include <numeric>

// [[Configuration]]
// XSTD_ESTR: Easy to override macro to control any error strings emitted into binary, must return a C string.
//
#ifndef XSTD_ESTR
	#define XSTD_ESTR(x) (x)
#endif

// Determine compiler support for C++20 constant evaluation.
//
#ifndef _CONSTEVAL
	#if defined(__cpp_consteval)
		#define _CONSTEVAL consteval
	#else // ^^^ supports consteval / no consteval vvv
		#define _CONSTEVAL constexpr
	#endif
#endif

#ifndef _CONSTINIT
	#if defined(__cpp_constinit)
		#define _CONSTINIT constinit
	#else // ^^^ supports constinit / no constinit vvv
		#define _CONSTINIT
	#endif
#endif

// Determine if we're compiling in debug mode.
//
#if defined(DEBUG_BUILD)
	#if defined(RELEASE_BUILD)
		static_assert( DEBUG_BUILD == !RELEASE_BUILD, "Invalid configuration." );
	#endif
	#define RELEASE_BUILD  !DEBUG_BUILD
#elif defined(RELEASE_BUILD)
	#if defined(DEBUG_BUILD)
		static_assert( DEBUG_BUILD == !RELEASE_BUILD, "Invalid configuration." );
	#endif
	#define DEBUG_BUILD    !RELEASE_BUILD
#elif NDEBUG
	#define DEBUG_BUILD    0
	#define RELEASE_BUILD  1
#elif _DEBUG               
	#define DEBUG_BUILD    1
	#define RELEASE_BUILD  0
#else                      
	#define DEBUG_BUILD    0
	#define RELEASE_BUILD  1
#endif
inline static constexpr bool is_debug_build() { return DEBUG_BUILD; }
inline static constexpr bool is_release_build() { return RELEASE_BUILD; }

// Determine STL type.
//
#if defined(_MSVC_STL_VERSION)
	#define USING_MS_STL   1
	#define USING_GNU_STL  0
	#define USING_LLVM_STL 0
#elif defined(_LIBCPP_VERSION)
	#define USING_MS_STL   0
	#define USING_GNU_STL  0
	#define USING_LLVM_STL 1
#elif defined(__GLIBCXX__)
	#define USING_MS_STL   0
	#define USING_GNU_STL  1
	#define USING_LLVM_STL 0
#endif
inline static constexpr bool is_using_ms_stl() { return USING_MS_STL; }
inline static constexpr bool is_using_gcc_stl() { return USING_GNU_STL; }
inline static constexpr bool is_using_clang_stl() { return USING_LLVM_STL; }

// Determine the target platform.
//
#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined (__AMD_64) || defined(_M_AMD64)
	#define AMD64_TARGET   1
	#define ARM64_TARGET   0
	#define WASM_TARGET    0
#elif defined(__aarch64__) || defined(_M_ARM64)
	#define AMD64_TARGET   0
	#define ARM64_TARGET   1
	#define WASM_TARGET    0
#elif defined(__EMSCRIPTEN__)
	#define AMD64_TARGET   0
	#define ARM64_TARGET   0
	#define WASM_TARGET    1
#else
	#error "Unknown target architecture."
#endif
inline static constexpr bool is_amd64_target() { return AMD64_TARGET; }
inline static constexpr bool is_arm64_target() { return ARM64_TARGET; }
inline static constexpr bool is_wasm_target() { return WASM_TARGET; }

// Determine the target operating system.
//
#if defined(_WIN64)
	#define WINDOWS_TARGET 1
	#define UNIX_TARGET    0
	#define OSX_TARGET     0
#elif defined(__APPLE__)
	#define WINDOWS_TARGET 0
	#define UNIX_TARGET    0
	#define OSX_TARGET     1
#elif defined(__EMSCRIPTEN__)
	#define WINDOWS_TARGET 0
	#define UNIX_TARGET    0
	#define OSX_TARGET     0
#elif defined(__linux__) || defined(__unix__) || defined(__unix)
	#define WINDOWS_TARGET 0
	#define UNIX_TARGET    1
	#define OSX_TARGET     0
#else
	#error "Unknown target OS."
#endif
inline static constexpr bool is_windows_target() { return WINDOWS_TARGET; }
inline static constexpr bool is_unix_target() { return UNIX_TARGET; }
inline static constexpr bool is_osx_target() { return OSX_TARGET; }

#if (defined(_KERNEL_MODE) || defined(XSTD_KERNEL_MODE))
	#define USER_TARGET   0
	#define KERNEL_TARGET 1
#else
	#define USER_TARGET   1
	#define KERNEL_TARGET 0
#endif
inline static constexpr bool is_user_mode() { return USER_TARGET; }
inline static constexpr bool is_kernel_mode() { return KERNEL_TARGET; }

// Determine the compiler.
//
#if defined(__GNUC__) || defined(__clang__) || defined(__EMSCRIPTEN__) || defined(__MINGW64__)
	#pragma GCC diagnostic ignored "-Wgnu-string-literal-operator-template"
	#define MS_COMPILER       0
	#define GNU_COMPILER      1
#ifdef __clang__
	#define CLANG_COMPILER    1
#else
	#define CLANG_COMPILER    0
#endif
#ifdef _MSC_VER
	#define HAS_MS_EXTENSIONS 1
#else
	#define HAS_MS_EXTENSIONS 0
#endif
#elif defined(_MSC_VER)
	#define MS_COMPILER       1
	#define GNU_COMPILER      0
	#define CLANG_COMPILER    0
	#define HAS_MS_EXTENSIONS 1
#else
	#error "Unknown compiler."
#endif
inline static constexpr bool is_msvc() { return MS_COMPILER; }
inline static constexpr bool is_gcc() { return GNU_COMPILER; }
inline static constexpr bool is_clang() { return CLANG_COMPILER; }
inline static constexpr bool has_ms_extensions() { return HAS_MS_EXTENSIONS; }

// Stop intellisense from dying.
//
#ifdef __INTELLISENSE__
	#define register
#endif

// Stringification helpers.
// - ix implies no macro expansion where as x does.
//
#define ixstringify(x) #x
#define xstringify(x)  ixstringify(x)
#define ixstrcat(x,y)  x##y
#define xstrcat(x,y)   ixstrcat(x,y)

// Determine RTTI and exception support.
//
#if defined(_CPPRTTI)
	#define HAS_RTTI	_CPPRTTI
#elif defined(__GXX_RTTI)
	#define HAS_RTTI	__GXX_RTTI
#elif defined(__has_feature)
	#define HAS_RTTI	__has_feature(cxx_rtti)
#else
	#define HAS_RTTI	0
#endif
inline static constexpr bool cxx_has_rtti() { return HAS_RTTI; }

#if (defined(_HAS_EXCEPTIONS)&&!_HAS_EXCEPTIONS) || (defined(__cpp_exceptions)&&!__cpp_exceptions)
	#define HAS_CXX_EH 0
#else
	#define HAS_CXX_EH 1
#endif
inline static constexpr bool cxx_has_eh() { return HAS_CXX_EH; }

// [[Configuration]]
// XSTD_NO_EXCEPTIONS: If set, exceptions will resolve into error instead.
//
#ifndef XSTD_NO_EXCEPTIONS
	#if HAS_CXX_EH
		#define XSTD_NO_EXCEPTIONS 0
	#else
		#define XSTD_NO_EXCEPTIONS 1
	#endif
#endif

// Declare common attributes.
//
#if GNU_COMPILER
	#define PURE_FN      __attribute__((pure))
	#define CONST_FN     __attribute__((const))
	#define FLATTEN      __attribute__((flatten))
	#define COLD         __attribute__((cold, noinline, disable_tail_calls))
	#define FORCE_INLINE __attribute__((always_inline))
	#define NO_INLINE    __attribute__((noinline))
	#define NO_DEBUG     __attribute__((nodebug))
	#define TRIVIAL_ABI  __attribute__((trivial_abi))
#elif MS_COMPILER
	#pragma warning( disable: 4141 )
	#define PURE_FN
	#define CONST_FN
	#define FLATTEN
	#define COLD         __declspec(noinline)     
	#define FORCE_INLINE [[msvc::forceinline]]
	#define NO_INLINE    __declspec(noinline)
	#define NO_DEBUG
	#define TRIVIAL_ABI
#else
	#define PURE_FN
	#define CONST_FN
	#define FLATTEN
	#define COLD 
	#define FORCE_INLINE
	#define NO_INLINE
	#define NO_DEBUG
	#define TRIVIAL_ABI
#endif

#if GNU_COMPILER
	#define __hint_unroll()   _Pragma("unroll")
	#define __hint_nounroll() _Pragma("nounroll")
#elif MS_COMPILER
	#define __hint_unroll()
	#define __hint_nounroll()
#else
	#define __hint_unroll()
	#define __hint_nounroll()
#endif

#if GNU_COMPILER && HAS_MS_EXTENSIONS
	#define __hint_unroll_n(N) __pragma(unroll(N))
#else
	#define __hint_unroll_n(N)
#endif

#ifndef RINLINE
	#if RELEASE_BUILD
		#define RINLINE     FORCE_INLINE
	#else
		#define RINLINE     
	#endif
#endif

// Make sure all users link to the same version.
//
#ifndef MUST_MATCH
	#if HAS_MS_EXTENSIONS
		#define MUST_MATCH(x) __pragma(comment(linker, "/failifmismatch:\"" #x "=" xstringify(x) "\""))
	#else
		#define MUST_MATCH(x)
	#endif
#endif
MUST_MATCH( DEBUG_BUILD );

// Ignore some warnings if GNU/Clang.
//
#if GNU_COMPILER
	#pragma GCC diagnostic ignored "-Wunused-function"
	#pragma GCC diagnostic ignored "-Wmicrosoft-cast"
#endif
#if CLANG_COMPILER
	#pragma clang diagnostic ignored "-Wassume"
#endif

// Define MS-style builtins if not available.
//
#if !MS_COMPILER
	#define __forceinline __attribute__((always_inline)) inline
	#define _ReturnAddress() (__builtin_return_address(0))
	#ifndef HAS_MS_EXTENSIONS
		#define _AddressOfReturnAddress() ((any_ptr)nullptr) // No equivalent, __builtin_frame_address(0) is wrong.
	#endif
#endif

// Include intrin.h if available.
//
#if HAS_MS_EXTENSIONS
	#include <intrin.h>
	#if MS_COMPILER
		#include <xmmintrin.h>
	#endif
#endif

// Declare xstd::is_consteval(x) and xstd::const_condition(x).
//
namespace xstd
{
	template<typename T>
	FORCE_INLINE static constexpr bool is_consteval( T value ) noexcept
	{
#if __has_builtin(__builtin_constant_p)
		return __builtin_constant_p( value );
#else
		return std::is_constant_evaluated();
#endif
	}
	FORCE_INLINE static constexpr bool const_condition( bool value ) noexcept
	{
		return is_consteval( value ) && value;
	}
};

// Define assume() / unreachable() / debugbreak() / fastfail(x).
//
#if MS_COMPILER
	#define assume(...) __assume(__VA_ARGS__)
	#define unreachable() __assume(0)
	#define debugbreak() __debugbreak()
	FORCE_INLINE static void fastfail [[noreturn]] ( int status )
	{
		__fastfail( status );
		unreachable();
	}
	FORCE_INLINE static void __trap() { fastfail( -1 ); }
#else
	#if __has_builtin(__builtin_assume)
		#define assume(...) __builtin_assume(__VA_ARGS__)
	#else
		#define assume(...) if constexpr( false ) { ( void ) (__VA_ARGS__); }
	#endif


	#if __has_builtin(__builtin_unreachable)
		#define unreachable() __builtin_unreachable()
	#elif __has_builtin(__builtin_trap)
		#define unreachable() __builtin_trap();
	#else
		#define unreachable() { *(int*)0 = 0; }
	#endif

	#if __has_builtin(__builtin_debugtrap)
		#define debugbreak() __builtin_debugtrap()
	#elif __has_builtin(__builtin_trap)
		#define debugbreak() __builtin_trap()
	#else
		#define debugbreak() { *(volatile int*)0 = 0; }
	#endif

	FORCE_INLINE static void __trap() 
	{
		#if __has_builtin(__builtin_trap)
			__builtin_trap();
		#else
			debugbreak();
		#endif
	}

	FORCE_INLINE static void fastfail [[noreturn]] ( int status )
	{
		#if AMD64_TARGET
			asm volatile ( "int $0x29" :: "c" ( status ) );
		#else
			__trap();
		#endif
		unreachable();
	}
#endif

// Define yield for busy loops.
//
FORCE_INLINE constexpr static void yield_cpu()
{
	if ( std::is_constant_evaluated() )
		return;

#if AMD64_TARGET && CLANG_COMPILER
	__builtin_ia32_pause();
#elif AMD64_TARGET && GNU_COMPILER
	asm volatile( "pause" ::: "memory" );
#elif AMD64_TARGET && MS_COMPILER
	_mm_pause();
#elif ARM64_TARGET
	asm volatile ( "isb" ::: "memory" );
#else
	std::atomic_thread_fence( std::memory_order::release ); // Close enough...
#endif
}

// Define task priority.
// [[Configuration]] 
//  - XSTD_HAS_TASK_PRIORITY: If set enables task priority, by default set if kernel target.
using task_priority_t = uintptr_t;
#ifndef XSTD_HAS_TASK_PRIORITY
	#define XSTD_HAS_TASK_PRIORITY KERNEL_TARGET
#endif
FORCE_INLINE static void set_task_priority( [[maybe_unused]] task_priority_t value )
{
#if MS_COMPILER && AMD64_TARGET && XSTD_HAS_TASK_PRIORITY
	__writecr8( value );
#elif AMD64_TARGET && XSTD_HAS_TASK_PRIORITY
	asm volatile ( "mov %0, %%cr8" :: "r" ( value ) );
#else
	// Assuming not relevant.
#endif
}
FORCE_INLINE static task_priority_t get_task_priority()
{
	task_priority_t value;
#if MS_COMPILER && AMD64_TARGET && XSTD_HAS_TASK_PRIORITY
	value = __readcr8();
#elif AMD64_TARGET && XSTD_HAS_TASK_PRIORITY
	asm volatile ( "mov %%cr8, %0" : "=r" ( value ) );
#else
	// Assuming not relevant.
	value = 0;
#endif
	assume( value <= 0xF );
	return value;
}

// Declare some compiler dependant features.
//
#if HAS_MS_EXTENSIONS
	// Declare demangled function name.
	//
	#define FUNCTION_NAME __FUNCSIG__

	// No-op demangle.
	//
	FORCE_INLINE static std::string compiler_demangle_type_name( const std::type_info& info ) { return info.name(); }
#else
	// Declare demangled function name.
	//
	#define FUNCTION_NAME __PRETTY_FUNCTION__

	// Declare demangling helper.
	//
	#if GNU_COMPILER
		#include <cxxabi.h>
		FORCE_INLINE static std::string compiler_demangle_type_name( const std::type_info& info ) 
		{
			int status;
			std::string result;
			char* demangled_name = abi::__cxa_demangle( info.name(), nullptr, nullptr, &status );
			result = status ? info.name() : demangled_name;
			free( demangled_name );
			return result;
		}
	#else
		FORCE_INLINE static std::string compiler_demangle_type_name( const std::type_info& info ) { return info.name(); }
	#endif
#endif

// Declare 128-bit multiplication.
//
#if !MS_COMPILER
using int128_t = __int128;
using uint128_t = unsigned __int128;
FORCE_INLINE static constexpr uint64_t umul128( uint64_t x, uint64_t y, uint64_t* hi ) {
	uint128_t r = uint128_t( y ) * x;
	*hi = uint64_t( r >> 64 );
	return uint64_t( r );
}
FORCE_INLINE static constexpr int64_t mul128( int64_t x, int64_t y, int64_t* hi ) {
	int128_t r = int128_t( x ) * y;
	*hi = int64_t( uint128_t( r ) >> 64 );
	return int64_t( r );
}
FORCE_INLINE CONST_FN static constexpr int64_t mulh( int64_t x, int64_t y ) {
	int64_t h;
	mul128( x, y, &h );
	return h;
}
FORCE_INLINE CONST_FN static constexpr uint64_t umulh( uint64_t x, uint64_t y ) {
	uint64_t h;
	umul128( x, y, &h );
	return h;
}
FORCE_INLINE static uint64_t udiv128( uint64_t dividend_hi, uint64_t dividend_lo, uint64_t divisor, uint64_t* rem ) {
#if AMD64_TARGET
	uint64_t a = dividend_lo, d = dividend_hi;
	asm( "div %[o]" : "+a"( a ), "+d"( d ) : [o] "c"( divisor ) );
	*rem = d;
	return a;
#else
	uint128_t dividend = ( uint128_t( dividend_hi ) << 64 ) + dividend_lo;
	*rem = uint64_t( dividend % divisor );
	return uint64_t( dividend / divisor );
#endif
}
FORCE_INLINE static int64_t div128( int64_t dividend_hi, int64_t dividend_lo, int64_t divisor, int64_t* rem ) {
#if AMD64_TARGET
	int64_t a = dividend_lo, d = dividend_hi;
	asm( "idiv %[o]" : "+a"( a ), "+d"( d ) : [o] "c"( divisor ) );
	*rem = d;
	return a;
#else
	int128_t dividend = int128_t( ( uint128_t( (uint64_t) dividend_hi ) << 64 ) + uint64_t( dividend_lo ) );
	*rem = int64_t( dividend % divisor );
	return int64_t( dividend / divisor );
#endif
}
#else
FORCE_INLINE static constexpr uint64_t umul128( uint64_t x, uint64_t y, uint64_t* hi ) {
	if ( std::is_constant_evaluated() ) {
		uint32_t a_lo = ( uint32_t ) x;
		uint32_t a_hi = ( uint32_t ) ( x >> 32 );
		uint32_t b_lo = ( uint32_t ) y;
		uint32_t b_hi = ( uint32_t ) ( y >> 32 );
		uint64_t b00 = ( uint64_t ) a_lo * b_lo;
		uint64_t b01 = ( uint64_t ) a_lo * b_hi;
		uint64_t b10 = ( uint64_t ) a_hi * b_lo;
		uint64_t b11 = ( uint64_t ) a_hi * b_hi;
		uint32_t b00_lo = ( uint32_t ) b00;
		uint32_t b00_hi = ( uint32_t ) ( b00 >> 32 );
		uint64_t mid1 = b10 + b00_hi;
		uint32_t mid1_lo = ( uint32_t ) ( mid1 );
		uint32_t mid1_hi = ( uint32_t ) ( mid1 >> 32 );
		uint64_t mid2 = b01 + mid1_lo;
		uint32_t mid2_lo = ( uint32_t ) ( mid2 );
		uint32_t mid2_hi = ( uint32_t ) ( mid2 >> 32 );
		*hi = b11 + mid1_hi + mid2_hi;
		return ( ( uint64_t ) mid2_lo << 32 ) + b00_lo;
	}
	return _umul128( x, y, hi );
}
FORCE_INLINE static constexpr int64_t mul128( int64_t x, int64_t y, int64_t* hi ) {
	if ( std::is_constant_evaluated() ) {
		if ( x == -1 ) {
			y = -y;
			*hi = y < 0 ? -1 : 0;
			return y;
		} else if ( y == -1 ) {
			x = -x;
			*hi = x < 0 ? -1 : 0;
			return x;
		} else {
			uint64_t ahi;
			uint64_t alo = umul128( x < 0 ? -x : x, y < 0 ? -y : y, &ahi );
			if ( ( x < 0 ) == ( y < 0 ) ) {
				*hi = int64_t( ahi );
				return int64_t( alo );
			} else {
				*hi = int64_t( ~ahi );
				return -int64_t( alo );
			}
		}
	}
	return _mul128( x, y, hi );
}
FORCE_INLINE static constexpr int64_t mulh( int64_t x, int64_t y ) {
	if ( std::is_constant_evaluated() ) {
		int64_t hi;
		mul128( x, y, &hi );
		return hi;
	}
	return __mulh( x, y );
}
FORCE_INLINE static constexpr uint64_t umulh( uint64_t x, uint64_t y ) {
	if ( std::is_constant_evaluated() ) {
		uint64_t hi;
		umul128( x, y, &hi );
		return hi;
	}
	return __umulh( x, y );
}
FORCE_INLINE static uint64_t udiv128( uint64_t dividend_hi, uint64_t dividend_lo, uint64_t divisor, uint64_t* rem ) {
	return _udiv128( dividend_hi, dividend_lo, divisor, rem );
}
FORCE_INLINE static int64_t div128( int64_t dividend_hi, int64_t dividend_lo, int64_t divisor, int64_t* rem ) {
	return _div128( dividend_hi, dividend_lo, divisor, rem );
}
#endif

// Checked integer operations.
//
template<typename T>
FORCE_INLINE CONST_FN constexpr std::pair<T, bool> add_checked( T x, T y ) {
	T r; 
	bool f;
#if __has_builtin(__builtin_add_overflow)
	f = __builtin_add_overflow( x, y, &r );
#else
	if constexpr ( std::is_signed_v<T> ) {
		using U = std::make_unsigned_t<T>;
		r = T( U( x ) + U( y ) );
		f = ( ( x < 0 ) == ( y < 0 ) ) && ( ( x < 0 ) != ( r < 0 ) );
	} else {
		r = x + y;
		f = r < y;
	}
#endif
	return { r, f };
}
template<typename T>
FORCE_INLINE CONST_FN constexpr std::pair<T, bool> sub_checked( T x, T y ) {
	T r; bool f;
#if __has_builtin(__builtin_sub_overflow)
	f = __builtin_sub_overflow( x, y, &r );
#else
	if constexpr ( std::is_signed_v<T> ) {
		using U = std::make_unsigned_t<T>;
		r = T( U( x ) - U( y ) );
		f = ( ( x < 0 ) == ( y >= 0 ) ) && ( ( x < 0 ) != ( r < 0 ) );
	} else {
		r = x - y;
		f = x < y;
	}
#endif
	return { r, f };
}
template<typename T>
FORCE_INLINE CONST_FN constexpr std::pair<T, bool> mul_checked( T x, T y ) {
	T r; bool f;
#if __has_builtin(__builtin_mul_overflow)
	f = __builtin_mul_overflow( x, y, &r );
#else
	if constexpr ( std::is_signed_v<T> ) {
		using U = std::make_unsigned_t<T>;
		r = T( U( x ) * U( y ) );
		if ( r == ( std::numeric_limits<T>::min )( ) && x == -1 ) {
			f = true;
		} else {
			f = x != 0 && T( r / x ) != y;
		}
	} else {
		r = x * y;
		f = x != 0 && T( r / x ) != y;
	}
#endif
	return { r, f };
}
template<typename T>
FORCE_INLINE CONST_FN constexpr std::tuple<T/*quot*/, T/*rem*/, bool> div_checked( T dividend_hi, T dividend_lo, T divisor ) {
	
	// Divisor is 0.
	//
	if ( divisor == 0 ) 
		return { T(), T(), true };

	constexpr T minv = ( std::numeric_limits<T>::min )( );
	constexpr T maxv = ( std::numeric_limits<T>::max )( );
	
	// If unsigned:
	//
	if constexpr ( !std::is_signed_v<T> ) {
		// Extend to U64.
		//
		if constexpr ( sizeof( T ) != 8 ) {
			uint64_t dividend = dividend_lo;
			dividend += uint64_t( dividend_hi ) << ( sizeof( T ) * 8 );
			uint64_t q = dividend / divisor;
			uint64_t r = dividend % divisor;
			return { T( q ), T( r ), q > maxv };
		} else {
			if ( dividend_hi >= divisor ) {
				return { T(), T(), true };
			} else {
				uint64_t rem;
				uint64_t quot = udiv128( dividend_hi, dividend_lo, divisor, &rem );
				return { quot, rem, false };
			}
		}
	} else {
		// Extend to I64.
		//
		if constexpr ( sizeof( T ) != 8 ) {
			int64_t dividend = int64_t( dividend_hi ) << ( sizeof( T ) * 8 );
			dividend += uint64_t( ( std::make_unsigned_t<T> ) dividend_lo );
			int64_t q = dividend / divisor;
			int64_t r = dividend % divisor;
			return { T( q ), T( r ), !( minv <= q && q <= maxv ) };
		} else {
			int64_t da = divisor < 0 ? -divisor : +divisor;
			if ( dividend_hi < 0 ) {
				int64_t min_hi = ( ( -( da + ( divisor > 0 ) ) >> 1 ) );
				int64_t min_lo = da & 1 ? INT64_MIN : 0;
				if ( divisor > 0 ) min_lo -= da;
				auto htest = dividend_hi + ( uint64_t( dividend_lo ) > uint64_t( min_lo ) );
				if ( htest <= min_hi )
					return { T(), T(), true };
			} else {
				int64_t max_hi = ( ( da >> 1 ) );
				int64_t max_lo = da & 1 ? INT64_MIN : 0;
				if ( divisor < 0 ) max_lo += da;
				auto htest = dividend_hi - ( uint64_t( dividend_lo ) < uint64_t( max_lo ) );
				if ( htest >= max_hi )
					return { T(), T(), true };
			}
			int64_t rem;
			int64_t quot = div128( dividend_hi, dividend_lo, divisor, &rem );
			return { quot, rem, false };
		}
	}
}

// Declare rotation.
//
FORCE_INLINE CONST_FN static constexpr uint64_t rotlq( uint64_t value, int count ) noexcept
{
#if __has_builtin(__builtin_rotateleft64)
	if ( !std::is_constant_evaluated() )
		return __builtin_rotateleft64( value, ( uint64_t ) count );
#endif
	count %= 64;
	if ( !count ) return value;
	return uint64_t( value << count ) | uint64_t( value >> ( 64 - count ) );
}
FORCE_INLINE CONST_FN static constexpr uint64_t rotrq( uint64_t value, int count ) noexcept
{
#if __has_builtin(__builtin_rotateright64)
	if ( !std::is_constant_evaluated() )
		return __builtin_rotateright64( value, ( uint64_t ) count );
#endif
	count %= 64;
	if ( !count ) return value;
	return uint64_t( value >> count ) | uint64_t( value << ( 64 - count ) );
}
FORCE_INLINE CONST_FN static constexpr uint32_t rotld( uint32_t value, int count ) noexcept
{
#if __has_builtin(__builtin_rotateleft32)
	if ( !std::is_constant_evaluated() )
		return __builtin_rotateleft32( value, ( uint32_t ) count );
#endif
	count %= 32;
	if ( !count ) return value;
	return uint32_t( value << count ) | uint32_t( value >> ( 32 - count ) );
}
FORCE_INLINE CONST_FN static constexpr uint32_t rotrd( uint32_t value, int count ) noexcept
{
#if __has_builtin(__builtin_rotateright32)
	if ( !std::is_constant_evaluated() )
		return __builtin_rotateright32( value, ( uint32_t ) count );
#endif
	count %= 32;
	if ( !count ) return value;
	return uint32_t( value >> count ) | uint32_t( value << ( 32 - count ) );
}
FORCE_INLINE CONST_FN static constexpr uint16_t rotlw( uint16_t value, int count ) noexcept
{
#if __has_builtin(__builtin_rotateleft16)
	if ( !std::is_constant_evaluated() )
		return __builtin_rotateleft16( value, ( uint16_t ) count );
#endif
	count %= 16;
	if ( !count ) return value;
	return uint16_t( value << count ) | uint16_t( value >> ( 16 - count ) );
}
FORCE_INLINE CONST_FN static constexpr uint16_t rotrw( uint16_t value, int count ) noexcept
{
#if __has_builtin(__builtin_rotateright16)
	if ( !std::is_constant_evaluated() )
		return __builtin_rotateright16( value, ( uint16_t ) count );
#endif
	count %= 16;
	if ( !count ) return value;
	return uint16_t( value >> count ) | uint16_t( value << ( 16 - count ) );
}
FORCE_INLINE CONST_FN static constexpr uint8_t rotlb( uint8_t value, int count ) noexcept
{
#if __has_builtin(__builtin_rotateleft8)
	if ( !std::is_constant_evaluated() )
		return __builtin_rotateleft8( value, ( uint8_t ) count );
#endif
	count %= 8;
	if ( !count ) return value;
	return uint8_t( value << count ) | uint8_t( value >> ( 8 - count ) );
}
FORCE_INLINE CONST_FN static constexpr uint8_t rotrb( uint8_t value, int count ) noexcept
{
#if __has_builtin(__builtin_rotateright8)
	if ( !std::is_constant_evaluated() )
		return __builtin_rotateright8( value, ( uint8_t ) count );
#endif
	count %= 8;
	if ( !count ) return value;
	return uint8_t( value >> count ) | uint8_t( value << ( 8 - count ) );
}
template<typename T> requires ( std::is_integral_v<T> || std::is_enum_v<T> )
FORCE_INLINE static constexpr T rotl( T value, int count ) noexcept
{
	if constexpr ( sizeof( T ) == 8 )
		return ( T ) rotlq( ( uint64_t ) value, count );
	else if constexpr ( sizeof( T ) == 4 )
		return ( T ) rotld( ( uint32_t ) value, count );
	else if constexpr ( sizeof( T ) == 2 )
		return ( T ) rotlw( ( uint16_t ) value, count );
	else if constexpr ( sizeof( T ) == 1 )
		return ( T ) rotlb( ( uint8_t ) value, count );
	else
		unreachable();
}
template<typename T> requires ( std::is_integral_v<T> || std::is_enum_v<T> )
FORCE_INLINE static constexpr T rotr( T value, int count ) noexcept
{
	if constexpr ( sizeof( T ) == 8 )
		return ( T ) rotrq( ( uint64_t ) value, count );
	else if constexpr ( sizeof( T ) == 4 )
		return ( T ) rotrd( ( uint32_t ) value, count );
	else if constexpr ( sizeof( T ) == 2 )
		return ( T ) rotrw( ( uint16_t ) value, count );
	else if constexpr ( sizeof( T ) == 1 )
		return ( T ) rotrb( ( uint8_t ) value, count );
	else
		unreachable();
}


// Double precision shift.
//
FORCE_INLINE CONST_FN static constexpr uint64_t shrd( uint64_t x, uint64_t y, int count )
{
#if GNU_COMPILER
	if ( !std::is_constant_evaluated() )
	{
		uint128_t tmp = x;
		tmp |= uint128_t( y ) << 64;
		tmp >>= count;
		return ( uint64_t ) tmp;
	}
#endif

	count %= 64;
	if ( !count ) return x;
	x >>= count;
	y <<= ( 64 - count );
	return x | y;
}
FORCE_INLINE CONST_FN static constexpr uint64_t shld( uint64_t x, uint64_t y, int count )
{
#if GNU_COMPILER
	if ( !std::is_constant_evaluated() )
	{
		uint128_t tmp = y;
		tmp |= uint128_t( x ) << 64;
		tmp <<= count;
		return ( uint64_t ) ( tmp >> 64 );
	}
#endif
	count %= 64;
	if ( !count ) return x;
	x <<= count;
	y >>= ( 64 - count );
	return x | y;
}


// Declare bswap.
//
FORCE_INLINE static constexpr uint16_t bswapw( uint16_t value ) noexcept
{
#if __has_builtin(__builtin_bswap16)
	if ( !std::is_constant_evaluated() )
		return __builtin_bswap16( value );
#endif
	return uint16_t( ( value & 0xFF ) << 8 ) | uint16_t( ( value & 0xFF00 ) >> 8 );
}
FORCE_INLINE static constexpr uint32_t bswapd( uint32_t value ) noexcept
{
#if __has_builtin(__builtin_bswap32)
	if ( !std::is_constant_evaluated() )
		return __builtin_bswap32( value );
#endif
	return
		( uint32_t( bswapw( uint16_t( ( value << 16 ) >> 16 ) ) ) << 16 ) |
		( uint32_t( bswapw( uint16_t( ( value >> 16 ) ) ) ) );
}
FORCE_INLINE static constexpr uint64_t bswapq( uint64_t value ) noexcept
{
#if __has_builtin(__builtin_bswap64)
	if ( !std::is_constant_evaluated() )
		return __builtin_bswap64( value );
#endif
	return
		( uint64_t( bswapd( uint32_t( ( value << 32 ) >> 32 ) ) ) << 32 ) |
		( uint64_t( bswapd( uint32_t( ( value >> 32 ) ) ) ) );
}
template<typename T> requires ( std::is_integral_v<T> || std::is_enum_v<T> )
FORCE_INLINE static constexpr T bswap( T value ) noexcept
{
	if constexpr ( sizeof( T ) == 8 )
		return ( T ) bswapq( ( uint64_t ) value );
	else if constexpr ( sizeof( T ) == 4 )
		return ( T ) bswapd( ( uint32_t ) value );
	else if constexpr ( sizeof( T ) == 2 )
		return ( T ) bswapw( ( uint16_t ) value );
	else if constexpr ( sizeof( T ) == 1 )
		return value;
	else
		unreachable();
}

// Declare cmpxchg primitive.
//
#if MS_COMPILER
	char _InterlockedCompareExchange8(
		char volatile* Destination,
		char Exchange,
		char Comparand
	);
	#pragma intrinsic(_InterlockedCompareExchange8)
	short _InterlockedCompareExchange16(
		short volatile* Destination,
		short Exchange,
		short Comparand
	);
	#pragma intrinsic(_InterlockedCompareExchange16)
	long _InterlockedCompareExchange(
		long volatile* Destination,
		long Exchange,
		long Comparand
	);
	#pragma intrinsic(_InterlockedCompareExchange)
	__int64 _InterlockedCompareExchange64(
		__int64 volatile* Destination,
		__int64 Exchange,
		__int64 Comparand
	);
	#pragma intrinsic(_InterlockedCompareExchange64)
	unsigned char _InterlockedCompareExchange128(
		__int64 volatile* _Destination,
		__int64 _Exchange_high,
		__int64 _ExchangeLow,
		__int64* _ComparandResult
	);
	#pragma intrinsic(_InterlockedCompareExchange128)
#endif
template<typename T>
FORCE_INLINE static bool cmpxchg( volatile T& data, T& expected, const T& desired )
{

#if CLANG_COMPILER
	using Y = std::array<uint8_t, sizeof( T )>;
	static_assert( __c11_atomic_is_lock_free( sizeof( Y ) ), "Compare exchange of this size cannot be lock-free." );
	return __c11_atomic_compare_exchange_strong(
			( _Atomic( Y ) * ) &data,
			( Y* ) &expected,
			*( Y* ) &desired,
			__ATOMIC_SEQ_CST, 
			__ATOMIC_SEQ_CST
	);
#elif GNU_COMPILER
	using Y = std::array<uint8_t, sizeof( T )>;
	std::atomic_ref<Y> ref_data{ ( Y& ) data };
	return ref_data.compare_exchange_strong( *( Y* ) &expected, *( Y* ) &desired );
#else

	#define __CMPXCHG_BASE(fn , type)          \
	auto iexpected = *( type* ) &expected;     \
	auto value = fn (                          \
	   ( volatile type* ) &data,               \
	   *( type* ) &desired,                    \
	   iexpected                               \
	);                                         \
	*( type* ) &expected = value;              \
	return value == iexpected;

	if constexpr ( sizeof( T ) == 1 )
	{
		__CMPXCHG_BASE( _InterlockedCompareExchange8, char );
	}
	else if constexpr ( sizeof( T ) == 2 )
	{
		__CMPXCHG_BASE( _InterlockedCompareExchange16, short );
	}
	else if constexpr ( sizeof( T ) == 4 )
	{
		__CMPXCHG_BASE( _InterlockedCompareExchange, long );
	}
	else if constexpr ( sizeof( T ) == 8 )
	{
		__CMPXCHG_BASE( _InterlockedCompareExchange64, __int64 );
	}
	else if constexpr ( sizeof( T ) == 16 )
	{
		return _InterlockedCompareExchange128(
				( volatile long long* ) &data,
				( ( long long* ) &desired )[ 1 ],
				( ( long long* ) &desired )[ 0 ],
				( long long* ) &expected
		);
	}
	else
	{
		static_assert( sizeof( T ) == 1, "Compare exchange of this size is not supported." );
	}
#undef __CMPXCHG_BASE

#endif
}
template<typename T>
FORCE_INLINE static bool cmpxchg( std::atomic<T>& data, T& expected, const T& desired )
{
	return cmpxchg( *( volatile T* ) &data, expected, desired );
}