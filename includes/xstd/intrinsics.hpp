#pragma once
#include <stdlib.h>
#include <string>
#include <stdint.h>
#include <type_traits>
#include <typeinfo>

#ifdef __has_include
    #if __has_include(<xstd/options.hpp>)
        #include <xstd/options.hpp>
    #elif __has_include("xstd_options.hpp")
        #include "xstd_options.hpp"
    #endif
#endif

#ifndef __has_builtin
	#define __has_builtin(x) 0
#endif

// [Configuration]
// XSTD_STR, XSTD_CSTR: Easy to override macro to control any strings emitted into binary, 
// _C prefix means it should be returning a C string instead.
// _E prefix means that this is an error string in the form of a C string.
//
#ifndef XSTD_STR
    #define XSTD_STR(x) std::string{x}
#endif
#ifndef XSTD_CSTR
    #define XSTD_CSTR(x) (x)
#endif
#ifndef XSTD_ESTR
    #define XSTD_ESTR(x) XSTD_CSTR(x)
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
    #undef _CONSTINIT
    #define _CONSTINIT
    #define register
    #define is_constant_evaluated() true_type::value
#endif

// Determine RTTI support.
//
#if defined(_CPPRTTI)
	#define HAS_RTTI	   _CPPRTTI
#elif defined(__GXX_RTTI)
	#define HAS_RTTI	   __GXX_RTTI
#elif defined(__has_feature)
	#define HAS_RTTI	   __has_feature(cxx_rtti)
#else
	#define HAS_RTTI	   0
#endif
inline static constexpr bool cxx_has_rtti() { return HAS_RTTI; }

// Declare inlining primitives.
//
#if GNU_COMPILER
    #define FORCE_INLINE __attribute__((always_inline))
    #define NO_INLINE    __attribute__((noinline))
#else
    #define FORCE_INLINE __forceinline
    #define NO_INLINE    __declspec(noinline)
#endif

#ifndef RINLINE
    #if RELEASE_BUILD
        #define RINLINE     FORCE_INLINE
    #else
        #define RINLINE     
    #endif
#endif

// Stringification helpers.
// - ix implies no macro expansion where as x does.
//
#define ixstringify(x) #x
#define xstringify(x)  ixstringify(x)
#define ixstrcat(x,y)  x##y
#define xstrcat(x,y)   ixstrcat(x,y)

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

#if HAS_MS_EXTENSIONS
    #include <intrin.h>

    // Declare demangled function name.
    //
    #define FUNCTION_NAME __FUNCSIG__

    // Declare unreachable.
    //
    #define unreachable() __assume(0)
    
    // Declare yield.
    //
    #define yield_cpu() _mm_pause()

    // Declare fastfail. (Not marked noreturn in standard declaration.)
    //
    __forceinline static void fastfail [[noreturn]] ( int status ) 
    {
        __fastfail( status );
        unreachable();
    }

    // No-op demangle.
    //
    __forceinline static std::string compiler_demangle_type_name( const std::type_info& info ) { return info.name(); }
#else
    // Declare demangled function name.
    //
    #define FUNCTION_NAME __PRETTY_FUNCTION__

    // Declare MS-style builtins we use.
    //
    #define __forceinline             __attribute__((always_inline))
    #define _AddressOfReturnAddress() ((void*)__builtin_frame_address(0))

    // Declare fastfail.
    //
    __forceinline static void fastfail [[noreturn]] ( int status ) { abort(); }

    // Declare debugbreak / unreachable.
    //
    #if __has_builtin(__builtin_unreachable)
        #define unreachable() __builtin_unreachable()
    #elif __has_builtin(__builtin_trap)
        #define unreachable() __builtin_trap();
    #elif AMD64_TARGET
        #define unreachable() { asm volatile ( "int $3" );  fastfail( 0 ); }
    #elif ARM64_TARGET                                           
        #define unreachable() { asm volatile ( "bkpt #0" ); fastfail( 0 ); }
    #else
        #define unreachable() { *(int*)0 = 0;               fastfail( 0 ); }
    #endif

    // Declare debugbreak.
    // - Some compilers apparently assume debugbreak is [[noreturn]] which is unwanted so use actual builtin as fallback.
    //
    #if AMD64_TARGET
        #define __debugbreak() asm volatile ( "int $3" )
    #elif ARM64_TARGET
        #define __debugbreak() asm volatile ( "bkpt #0" )
    #elif __has_builtin(__builtin_debugtrap)
        #define __debugbreak() __builtin_debugtrap()
    #elif __has_builtin(__builtin_trap)
        #define __debugbreak() __builtin_trap()
    #else
        #define __debugbreak() { *(int*) 1 = 1; }
    #endif
    
    // Declare yield.
    //
    #if AMD64_TARGET
        #include <emmintrin.h>
        #define yield_cpu() _mm_pause()
    #elif ARM64_TARGET
        #define yield_cpu() asm volatile ("yield")
    #else
        // Close enough...
        #include <atomic>
        #define yield_cpu() std::atomic_thread_fence( std::memory_order_release );
    #endif

    // Declare _?mul128
    //
    using int128_t =  __int128;
    using uint128_t = unsigned __int128;
    __forceinline static uint64_t _umul128( uint64_t _Multiplier, uint64_t _Multiplicand, uint64_t* _HighProduct )
    {
        uint128_t _Product = uint128_t( _Multiplicand ) * _Multiplier;
        *_HighProduct = uint64_t( _Product >> 64 );
        return uint64_t( _Product );
    }

    __forceinline static int64_t _mul128( int64_t _Multiplier, int64_t _Multiplicand, int64_t* _HighProduct )
    {
        int128_t _Product = int128_t( _Multiplier ) * _Multiplicand;
        *_HighProduct = int64_t( uint128_t( _Product ) >> 64 );
        return int64_t( _Product );
    }

    // Declare _?mulh
    //
    __forceinline static int64_t __mulh( int64_t _Multiplier, int64_t _Multiplicand )
    {
        int64_t HighProduct;
        _mul128( _Multiplier, _Multiplicand, &HighProduct );
        return HighProduct;
    }

    __forceinline static uint64_t __umulh( uint64_t _Multiplier, uint64_t _Multiplicand )
    {
        uint64_t HighProduct;
        _umul128( _Multiplier, _Multiplicand, &HighProduct );
        return HighProduct;
    }

    // Declare demangling helper.
    //
    #if GNU_COMPILER
	    #include <cxxabi.h>
        __forceinline static std::string compiler_demangle_type_name( const std::type_info& info ) 
        {
            int status;
            std::string result;
            char* demangled_name = abi::__cxa_demangle( info.name(), nullptr, nullptr, &status );
            result = status ? info.name() : demangled_name;
            free( demangled_name );
            return result;
        }
    #else
        __forceinline static std::string compiler_demangle_type_name( const std::type_info& info ) { return info.name(); }
    #endif
#endif

// Declare rotation.
//
__forceinline static constexpr uint64_t rotlq( uint64_t value, int count ) noexcept
{
#if __has_builtin(__builtin_rotateleft64)
    if ( !std::is_constant_evaluated() )
        return __builtin_rotateleft64( value, ( uint64_t ) count );
#endif
    count %= 64;
    return ( value << count ) | ( value >> ( 64 - count ) );
}
__forceinline static constexpr uint64_t rotrq( uint64_t value, int count ) noexcept
{
#if __has_builtin(__builtin_rotateright64)
    if ( !std::is_constant_evaluated() )
        return __builtin_rotateright64( value, ( uint64_t ) count );
#endif
    count %= 64;
    return ( value >> count ) | ( value << ( 64 - count ) );
}
__forceinline static constexpr uint32_t rotld( uint32_t value, int count ) noexcept
{
#if __has_builtin(__builtin_rotateleft32)
    if ( !std::is_constant_evaluated() )
        return __builtin_rotateleft32( value, ( uint32_t ) count );
#endif
    count %= 32;
    return ( value << count ) | ( value >> ( 32 - count ) );
}
__forceinline static constexpr uint32_t rotrd( uint32_t value, int count ) noexcept
{
#if __has_builtin(__builtin_rotateright32)
    if ( !std::is_constant_evaluated() )
        return __builtin_rotateright32( value, ( uint32_t ) count );
#endif
    count %= 32;
    return ( value >> count ) | ( value << ( 32 - count ) );
}
__forceinline static constexpr uint16_t rotlw( uint16_t value, int count ) noexcept
{
#if __has_builtin(__builtin_rotateleft16)
    if ( !std::is_constant_evaluated() )
        return __builtin_rotateleft16( value, ( uint16_t ) count );
#endif
    count %= 16;
    return ( value << count ) | ( value >> ( 16 - count ) );
}
__forceinline static constexpr uint16_t rotrw( uint16_t value, int count ) noexcept
{
#if __has_builtin(__builtin_rotateright16)
    if ( !std::is_constant_evaluated() )
        return __builtin_rotateright16( value, ( uint16_t ) count );
#endif
    count %= 16;
    return ( value >> count ) | ( value << ( 16 - count ) );
}
__forceinline static constexpr uint8_t rotlb( uint8_t value, int count ) noexcept
{
#if __has_builtin(__builtin_rotateleft8)
    if ( !std::is_constant_evaluated() )
        return __builtin_rotateleft8( value, ( uint8_t ) count );
#endif
    count %= 8;
    return ( value << count ) | ( value >> ( 8 - count ) );
}
__forceinline static constexpr uint8_t rotrb( uint8_t value, int count ) noexcept
{
#if __has_builtin(__builtin_rotateright8)
    if ( !std::is_constant_evaluated() )
        return __builtin_rotateright8( value, ( uint8_t ) count );
#endif
    count %= 8;
    return ( value >> count ) | ( value << ( 8 - count ) );
}
template<typename T> requires ( std::is_integral_v<T> || std::is_enum_v<T> )
__forceinline static constexpr T rotl( T value, int count ) noexcept
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
__forceinline static constexpr T rotr( T value, int count ) noexcept
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

// Declare bswap.
//
__forceinline static constexpr uint16_t bswapw( uint16_t value ) noexcept
{
#if __has_builtin(__builtin_bswap16)
    if ( !std::is_constant_evaluated() )
        return __builtin_bswap16( value );
#endif
    return ( ( value & 0xFF ) << 8 ) | ( ( value & 0xFF00 ) >> 8 );
}
__forceinline static constexpr uint32_t bswapd( uint32_t value ) noexcept
{
#if __has_builtin(__builtin_bswap32)
    if ( !std::is_constant_evaluated() )
        return __builtin_bswap32( value );
#endif
    return
        ( uint32_t( bswapw( uint16_t( ( value << 16 ) >> 16 ) ) ) << 16 ) |
        ( uint32_t( bswapw( uint16_t( ( value >> 16 ) ) ) ) );
}
__forceinline static constexpr uint64_t bswapq( uint64_t value ) noexcept
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
__forceinline static constexpr T bswap( T value ) noexcept
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