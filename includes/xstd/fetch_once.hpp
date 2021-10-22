#pragma once
#include "type_helpers.hpp"

// [[Configuration]]
// XSTD_NO_RWX: Disables self patching.
//
#ifndef XSTD_NO_RWX
	#define XSTD_NO_RWX 0
#endif

// Implements self-patching fetch-once, following conditions must be met for proper functioning:
// - Target must be X64.
// - Compiler must be clang.
// - RWX code must not be disabled.
// - Pointer must be const evaluted.
// - [ptr, ptr+8) must be readable.
//
namespace xstd
{
#if !defined(__clang__) || !AMD64_TARGET || XSTD_NO_RWX
	// Fetches the value from the given consteval reference.
	//
	template<auto* Value> requires ( sizeof( *Value ) == 8 && TriviallyCopyable<std::decay_t<decltype( *Value )>> )
	FORCE_INLINE CONST_FN inline auto fetch_once()
	{
		return *Value;
	}
#else
	namespace impl
	{
		template<auto* Value>
		[[gnu::naked, gnu::noinline]] inline void fetch_once_helper()
		{
			asm volatile(
				// Create the stack frame.
				"pushfq;"
				"pushq    %%rsi;"
				"pushq    %%rdi;"
				"sub      $0x20,       %%rsp;"
				"movups   %%xmm0,      0x10(%%rsp);"

				// Backtrack and reference the callsite into RSI.
				// 
				"subq     $10,         0x38(%%rsp);"
				"movq     0x38(%%rsp), %%rsi;"

				// Read the callsite into XMM0, store it on stack.
				//
				"movups   (%%rsi),     %%xmm0;"
				"movups   %%xmm0,      (%%rsp);"

				// If unexpected, simply retry.
				//
				"cmpb     $0x2E,       (%%rsp);"
				"jne      1f;"

				// Read the value and create the desired callsite on stack.
				//
				"movq     %0,          %%rdi;"
				"movw     $0xb848,     (%%rsp);"
				"movq     %%rdi,       2(%%rsp);"

				// Reload into XMM0 and store back to callsite.
				//
				"movups   (%%rsp),     %%xmm0;"
				"movups   %%xmm0,      (%%rsi);"

				// Destroy the stack frame and try again.
				"1:"
				"movups   0x10(%%rsp), %%xmm0;"
				"add      $0x20,       %%rsp;"
				"popq     %%rdi;"
				"popq     %%rsi;"
				"popfq;"
				"ret;"
			:: "m" ( *Value ) );
		}
	};

	// Fetches the value from the given consteval reference and self-rewrites to make sure 
	// second call does not issue a memory read.
	//
	template<auto* Value> requires ( sizeof( *Value ) == 8 && TriviallyCopyable<std::decay_t<decltype( *Value )>> )
	FORCE_INLINE CONST_FN inline auto fetch_once()
	{
		uint64_t tmp;
		asm(
			".byte 0x2E, 0x2E, 0x2E, 0x2E, 0x2E;" // 0x0: cs * 5
			"call   %c1;"                         // 0x5: call   rel32
			: "=a" ( tmp ) : "i" ( &impl::fetch_once_helper<Value> ) :
		);
		return *( decltype( Value ) ) &tmp;
	}
#endif
};