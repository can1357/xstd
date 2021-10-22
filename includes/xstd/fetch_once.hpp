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
				// Load the callsite into RAX and pad the stack 16 bytes.
				//
				"popq     %%rax;"
				"pushq    %%rax;"
				"pushq    %%rax;"

				// Create the desired callsite on stack.
				//
				"pushq    (%%rax);"
				"pushq    %0;"
				"pushw    $0xb848;"

				// Temporarily load into XMM0 and store back to callsite.
				//
				"movups   %%xmm0,       0x12(%%rsp);"
				"movups   (%%rsp),      %%xmm0;"
				"movups   %%xmm0,       -10(%%rax);"
				"movups   0x12(%%rsp),  %%xmm0;"

				// Push the return address, read the result into RAX.
				//
				"pushq    %%rax;"
				"movq     10(%%rsp),    %%rax;"

				// Destroy the stack frame and return.
				//
				"ret      $0x22;"
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