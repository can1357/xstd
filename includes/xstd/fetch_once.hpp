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
	// Fetches the value from the given consteval pointer.
	//
	template<Trivial T> requires ( sizeof( T ) <= 8 )
	FORCE_INLINE inline T fetch_once( const T* ptr )
	{
		return *ptr;
	}
#else
	namespace impl
	{
		[[gnu::naked]] NO_INLINE inline void fetch_once_helper()
		{
			__asm 
			{
				// Create the stack frame.
				//
				pushfq
				push     rax
				push     r10
				sub      rsp,                      0x30
				vmovups  ymmword ptr [rsp+0x10],   ymm0

				// Read the return pointer into r10, rollback the callsite.
				//
				mov      r10,                      [rsp+0x30+0x18]
				lea      r10,                      [r10-0x10]
				mov      [rsp+0x30+0x18],          r10

				// Read the movabs instruction with its ModR/M from the callsite into the bottom of the stack frame.
				//
				mov      ax,                       [r10+1]
				mov      [rsp],                    ax

				// Read the value using the pointer extracted from movabs into rax.
				//
				mov      rax,                      [r10+3]
				mov      rax,                      [rax]

				// Create the patch on stack and load it into xmm0.
				//
				mov      qword ptr [rsp+2],        rax        // - Write the imm64
				mov      dword ptr [rsp+2+8],      0x441F0F66 // - Pad with a 6 byte nop | nop word ptr [rax+rax*1+0x0]
				mov      word ptr [rsp+2+8+4],     0x0000     //
				vmovups  xmm0,                     xmmword ptr [rsp]
				
				// Apply the patch.
				//
				vmovups  xmmword ptr [r10],        xmm0
				mfence

				// Destroy the stack frame and return.
				//
				vmovups  ymm0,                     ymmword ptr [rsp+0x10]
				add      rsp,                      0x30
				pop      r10
				pop      rax
				popfq
				ret
			}
		}
	};

	// Fetches the value from the given consteval pointer and self-rewrites to make sure 
	// second call does fetch anything.
	//
	template<Trivial T> requires ( sizeof( T ) <= 8 )
	FORCE_INLINE inline T fetch_once( const volatile T* ptr )
	{
		uint64_t tmp;
		asm volatile(
			".byte 0x2E;"        // 0x0: cs
			"movabs %1,     %0;" // 0x1: movabs r64, 0x?????
			"callq  %c2;"        // 0xb: call   rel32
			: "=r" ( tmp ) : "i" ( ptr ), "i" ( impl::fetch_once_helper ) :
		);
		return *( T* ) &tmp;
	}
#endif
};