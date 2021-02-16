#pragma once
#include "intrinsics.hpp"
#include "time.hpp"
#include "event.hpp"
#include <thread>

// [Configuration]
// XSTD_CHORE_OS_DEFER: If set, defer/timeout will invoke the OS helper given instead of creating a thread and sleeping.
// XSTD_CHORE_OS_SCHEDULE: If set, chore with an event will invoke the OS helper given with the event and the function 
// to schedule an action to be done after the event is signalled.
//
#ifdef XSTD_CHORE_OS_DEFER
	extern "C" void __cdecl XSTD_CHORE_OS_DEFER( void( __cdecl* func )( void* ), void* argument, const xstd::timestamp* due_time );
#endif
#ifdef XSTD_CHORE_OS_SCHEDULE
	extern "C" void __cdecl XSTD_CHORE_OS_SCHEDULE( void( __cdecl* func )( void* ), void* argument, const event& evt );
#endif

namespace xstd
{
	// Registers a chore that will be invoked either at a set time, or if left empty immediately but in an async context.
	//
	template<typename T> requires Invocable<T, void>
	inline void chore( T&& fn, xstd::timestamp due_time = {} )
	{
#if XSTD_CHORE_OS_DEFER
		auto [func, arg, _] = xstd::flatten( std::forward<T>( fn ) );
		if ( due_time == xstd::timestamp{} )
		{
			XSTD_CHORE_OS_DEFER( func, arg, nullptr );
		}
		else
		{
			XSTD_CHORE_OS_DEFER( func, arg, &due_time );
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

	// Registers a chore to be invoked when an even occurs.
	//
	template<typename T> requires Invocable<T, void>
	inline void chore( const event& evt, T&& fn )
	{
#if XSTD_CHORE_OS_SCHEDULE
		auto [func, arg, _] = xstd::flatten( std::forward<T>( fn ) );
		XSTD_CHORE_OS_SCHEDULE( func, arg, evt );
#else
		chore( [ fn = std::forward<T>( fn ) ] ()
		{
			evt->wait();
			fn();
		} );
#endif
	}
};