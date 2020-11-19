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
#include <stdlib.h>
#include <string>
#include <stdint.h>
#include <type_traits>
#include <typeinfo>

#ifndef __has_builtin
	#define __has_builtin(x) 0
#endif

// [Configuration]
// XSTD_STR, XSTD_ESTR: Easy to override macro to control any strings emitted into binary, 
// _E prefix means it should be returning a const char* instead.
//
#ifndef XSTD_STR
    #define XSTD_STR(x) std::string{x}
#endif
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
#ifndef DEBUG_BUILD
    #if NDEBUG
        #define DEBUG_BUILD    0
    #elif _DEBUG               
        #define DEBUG_BUILD    1
    #else                      
        #define DEBUG_BUILD    0
    #endif
#endif

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

// Determine the compiler.
//
#if defined(__GNUC__) || defined(__clang__) || defined(__EMSCRIPTEN__) || defined(__MINGW64__)
    #define MS_COMPILER    0
    #define GNU_COMPILER   1
    #define EX_MS_COMPILER 1
#elif defined(_MSC_VER)
    #define MS_COMPILER    1
    #define GNU_COMPILER   0
    #define EX_MS_COMPILER defined(_MSC_VER)
#else
    #error "Unknown compiler."
#endif

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
    #define is_constant_evaluated() true_type::value
#endif

// Declare simple kernel mode switch.
//
inline static constexpr bool is_kernel_mode()
{
#ifdef _KERNEL_MODE
	return true;
#else
	return false;
#endif
}

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

// Ignore some warnings if GNU.
//
#if GNU_COMPILER
    #pragma GCC diagnostic ignored "-Wunused-function"
#endif

#if MS_COMPILER
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

    // Declare rotlq | rotrq
    //
    __forceinline static constexpr uint64_t rotlq( uint64_t value, int count )
    {
        if ( !std::is_constant_evaluated() )
            return _rotl64( value, count );
        count %= 64;
        return ( value << count ) | ( value >> ( 64 - count ) );
    }
    __forceinline static constexpr uint64_t rotrq( uint64_t value, int count )
    {
        if ( !std::is_constant_evaluated() )
            return _rotr64( value, count );
        count %= 64;
        return ( value >> count ) | ( value << ( 64 - count ) );
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
    #if !EX_MS_COMPILER
        #define __forceinline             __attribute__((always_inline))
        #define _AddressOfReturnAddress() ((void*)__builtin_frame_address(0))
    #endif
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
    #if !EX_MS_COMPILER
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

    // Declare rotlq | rotrq
    //
    __forceinline static constexpr uint64_t rotlq( uint64_t value, int count )
    {
#if __has_builtin(__builtin_rotateleft64)
        if ( !std::is_constant_evaluated() )
            return __builtin_rotateleft64( value, ( uint64_t ) count );
#endif
        count %= 64;
        return ( value << count ) | ( value >> ( 64 - count ) );
    }
    __forceinline static constexpr uint64_t rotrq( uint64_t value, int count )
    {
#if __has_builtin(__builtin_rotateright64)
        if ( !std::is_constant_evaluated() )
            return __builtin_rotateright64( value, ( uint64_t ) count );
#endif
        count %= 64;
        return ( value >> count ) | ( value << ( 64 - count ) );
    }

    // Declare _?mul128
    //
    using int128_t =  __int128;
    using uint128_t = unsigned __int128;
    #if !EX_MS_COMPILER
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
    #endif

    // Declare demangling helper.
    //
    #if (GNU_COMPILER && !EX_MS_COMPILER)
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