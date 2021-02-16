#pragma once
#include "intrinsics.hpp"
#include "time.hpp"
#include <thread>

// [Configuration]
// XSTD_CHORE_OS_REDIRECT: If set, defer/timeout will invoke OS helper with the given name instead of creating a thread.
//
#ifdef XSTD_CHORE_OS_REDIRECT
	extern "C" void __cdecl XSTD_CHORE_OS_REDIRECT( void( __cdecl* func )( void* ), void* argument, const xstd::timestamp* due_time );
#endif

namespace xstd
{
	// Registers a chore that will be invoked either at a set time, or if left empty immediately but in an async context.
	//
	template<typename T> requires Invocable<T, void>
	inline void chore( T&& fn, xstd::timestamp due_time = {} )
	{
#if XSTD_CHORE_OS_REDIRECT
		auto [func, arg, _] = xstd::flatten( std::forward<T>( fn ) );
		if ( due_time == xstd::timestamp{} )
		{
			XSTD_CHORE_OS_REDIRECT( func, arg, nullptr );
		}
		else
		{
			XSTD_CHORE_OS_REDIRECT( func, arg, &due_time );
		}
#else
		if ( due_time == xstd::timestamp{} )
		{
			std::thread( std::forward<T>( fn ) ).detach();
		}
		else
		{
			std::thread( [ fn = std::forward<T>( fn ), due_time ]()
			{
				std::this_thread::sleep_until( due_time );
				fn();
			} ).detach();
		}
#endif
	}
	template<typename T> requires Invocable<T, void>
	inline void chore( T&& fn, xstd::duration delay )
	{
		return chore( std::forward<T>( fn ), xstd::time::now() + delay );
	}
};