#pragma once

#if __has_include(<xstd/options.hpp>)
	#include <xstd/options.hpp>
#elif __has_include("xstd_options.hpp")
	#include "xstd_options.hpp"
#endif

#include <stdlib.h>
#include <string>
#include <stdint.h>
#include <type_traits>
#include <typeinfo>
#include <atomic>
#include <array>

#ifndef __has_builtin
	#define __has_builtin(...) 0
#endif
#ifndef __has_attribute
	#define __has_attribute(...) 0
#endif
#ifndef __has_cpp_attribute
	#define __has_cpp_attribute(...) 0
#endif
#ifndef __has_include
	#define __has_include(...) 0
#endif

// [Configuration]
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
#ifdef _MSC_VER
	#define HAS_MS_EXTENSIONS 1
#else
	#define HAS_MS_EXTENSIONS 0
#endif
#elif defined(_MSC_VER)
	#define MS_COMPILER       1
	#define GNU_COMPILER      0
	#define HAS_MS_EXTENSIONS 1
#else
	#error "Unknown compiler."
#endif
inline static constexpr bool is_msvc() { return MS_COMPILER; }
inline static constexpr bool is_gcc() { return GNU_COMPILER; }
inline static constexpr bool has_ms_extensions() { return HAS_MS_EXTENSIONS; }

// Determine bitcast support.
//
#if (defined(_MSC_VER) && _MSC_VER >= 1926)
	#define HAS_BIT_CAST   1
#elif __has_builtin(__builtin_bit_cast)
	#define HAS_BIT_CAST   __has_builtin(__builtin_bit_cast)
#else
	#define HAS_BIT_CAST   0
#endif

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

// Determine RTTI support.
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

// Declare function attributes.
//
#if GNU_COMPILER
	#define PURE_FN      __attribute__((pure))
	#define CONST_FN     __attribute__((const))
	#define FLATTEN      __attribute__((flatten))
	#define COLD         __attribute__((cold, noinline, disable_tail_calls))
	#define FORCE_INLINE __attribute__((always_inline))
	#define NO_INLINE    __attribute__((noinline))
	#define NO_DEBUG     __attribute__((nodebug))
#else
	#pragma warning( disable: 4141 )
	#define PURE_FN
	#define CONST_FN
	#define FLATTEN
	#define COLD         __declspec(noinline)     
	#define FORCE_INLINE __forceinline
	#define NO_INLINE    __declspec(noinline)
	#define NO_DEBUG
#endif

#if GNU_COMPILER
	#define LAMBDA_INLINE __attribute__((always_inline))
#elif MS_COMPILER
	#define LAMBDA_INLINE [[msvc::forceinline]]
#else
	#define LAMBDA_INLINE
#endif

#if GNU_COMPILER
	#define TRIVIAL_ABI    __attribute__((trivial_abi))
#else
	#define TRIVIAL_ABI
#endif

#if GNU_COMPILER
	#define __hint_unroll() _Pragma("unroll")
#elif MS_COMPILER
	#define __hint_unroll() __pragma(loop(ivdep))
#else
	#define __hint_unroll()
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

// Ignore some warnings if GNU.
//
#if GNU_COMPILER
	#pragma GCC diagnostic ignored "-Wunused-function"
	#pragma GCC diagnostic ignored "-Wmicrosoft-cast"
#endif

// Define MS-style builtins we're using if not available.
//
#if !MS_COMPILER
	#define __forceinline __attribute__((always_inline)) inline
	#define _ReturnAddress() ((xstd::any_ptr)__builtin_return_address(0))
	#ifndef HAS_MS_EXTENSIONS
		#define _AddressOfReturnAddress() ((xstd::any_ptr)nullptr) // No equivalent, __builtin_frame_address(0) is wrong.
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

// Define __is_consteval(x)
//
#if __has_builtin(__builtin_constant_p)
	#define __is_consteval(...) __builtin_constant_p(__VA_ARGS__)
#else
	#define __is_consteval(...) false
#endif

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
		#define assume(...)
	#endif

	#if __has_builtin(__builtin_unreachable)
		#define unreachable() __builtin_unreachable()
	#elif __has_builtin(__builtin_trap)
		#define unreachable() __builtin_trap();
	#else
		#define unreachable() { *(volatile int*)0 = 0; }
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
			asm volatile ( "int $0x2c" :: "c" ( status ) );
		#else
			__trap();
		#endif
		unreachable();
	}
#endif

// Define yield for busy loops.
//
FORCE_INLINE static void yield_cpu()
{
	#if MS_COMPILER
		_mm_pause();
	#elif AMD64_TARGET
		asm volatile ( "pause" ::: "memory" );
	#elif ARM64_TARGET
		asm volatile ( "yield" ::: "memory" );
	#else
		std::atomic_thread_fence( std::memory_order_release ); // Close enough...
	#endif
}

// Define task priority.
// [Configuration] 
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
		FORCE_INLINE static uint64_t umul128( uint64_t _Multiplier, uint64_t _Multiplicand, uint64_t* _HighProduct )
		{
			uint128_t _Product = uint128_t( _Multiplicand ) * _Multiplier;
			*_HighProduct = uint64_t( _Product >> 64 );
			return uint64_t( _Product );
		}
		FORCE_INLINE static int64_t mul128( int64_t _Multiplier, int64_t _Multiplicand, int64_t* _HighProduct )
		{
			int128_t _Product = int128_t( _Multiplier ) * _Multiplicand;
			*_HighProduct = int64_t( uint128_t( _Product ) >> 64 );
			return int64_t( _Product );
		}
		FORCE_INLINE CONST_FN static int64_t mulh( int64_t _Multiplier, int64_t _Multiplicand )
		{
			int64_t HighProduct;
			mul128( _Multiplier, _Multiplicand, &HighProduct );
			return HighProduct;
		}
		FORCE_INLINE CONST_FN static uint64_t umulh( uint64_t _Multiplier, uint64_t _Multiplicand )
		{
			uint64_t HighProduct;
			umul128( _Multiplier, _Multiplicand, &HighProduct );
			return HighProduct;
		}
#else
		FORCE_INLINE static uint64_t umul128( uint64_t _Multiplier, uint64_t _Multiplicand, uint64_t* _HighProduct )
		{
			return _umul128( _Multiplier, _Multiplicand, _HighProduct );
		}
		FORCE_INLINE static int64_t mul128( int64_t _Multiplier, int64_t _Multiplicand, int64_t* _HighProduct )
		{
			return _mul128( _Multiplier, _Multiplicand, _HighProduct );
		}
		FORCE_INLINE static int64_t mulh( int64_t _Multiplier, int64_t _Multiplicand )
		{
			return __mulh( _Multiplier, _Multiplicand );
		}

		FORCE_INLINE static uint64_t umulh( uint64_t _Multiplier, uint64_t _Multiplicand )
		{
			return __umulh( _Multiplier, _Multiplicand );
		}
#endif

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
		__int64 _ExchangeHigh,
		__int64 _ExchangeLow,
		__int64* _ComparandResult
	);
	#pragma intrinsic(_InterlockedCompareExchange128)
#endif
template<typename T>
FORCE_INLINE static bool cmpxchg( volatile T& data, T& expected, const T& desired )
{
#if !MS_COMPILER
	using Y = std::array<uint8_t, sizeof( T )>;
	//#if __has_feature(c_atomic)
		static_assert( __c11_atomic_is_lock_free( sizeof( Y ) ), "Compare exchange of this size cannot be lock-free." );
		return __c11_atomic_compare_exchange_strong(
				( _Atomic( Y ) * ) &data,
				( Y* ) &expected,
				*( Y* ) &desired,
				__ATOMIC_SEQ_CST, 
				__ATOMIC_SEQ_CST
		);
	//#else
	//    return ( ( std::atomic<Y>* ) &data )->compare_exchange_strong( *( Y* ) &expected, *( const Y* ) &desired );
	//#endif
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