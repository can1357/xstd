#pragma once
#include "intrinsics.hpp"
#include "time.hpp"
#include "event.hpp"
#include <thread>

// [Configuration]
// XSTD_CHORE_SCHEDULER: If set, chore will pass OS two callbacks to help with the scheduling.
//
#ifdef XSTD_CHORE_SCHEDULER
	extern "C" void __cdecl XSTD_CHORE_SCHEDULER( bool( __cdecl* ready )( void* ), void* flt_arg, void( __cdecl* callback )( void* ), void* cb_arg, int64_t priority );
#endif

namespace xstd
{
	// Registers a chore that will be invoked either at a set time, or if left empty immediately but in an async context.
	//
	template<typename T> requires Invocable<T, void>
	inline void chore( T&& fn, timestamp due_time, int64_t priority = -1 )
	{
#ifdef XSTD_CHORE_SCHEDULER
		auto [func, arg, _] = flatten( std::forward<T>( fn ) );
		auto [ffunc, farg, __] = flatten( [ due_time ] ()
		{
			return time::now() >= due_time;
		} );
		XSTD_CHORE_SCHEDULER( ffunc, farg, func, arg, priority );
#else
		std::thread( [ fn = std::forward<T>( fn ), due_time ]()
		{
			std::this_thread::sleep_until( due_time );
			fn();
		} ).detach();
#endif
	}
	template<typename T> requires Invocable<T, void>
	inline void chore( T&& fn, duration delay, int64_t priority = -1 )
	{
		return chore( std::forward<T>( fn ), time::now() + delay, priority );
	}
	template<typename T> requires Invocable<T, void>
	inline void chore( T&& fn, int64_t priority = -1 )
	{
#ifdef XSTD_CHORE_SCHEDULER
		auto [func, arg, _] = flatten( std::forward<T>( fn ) );
		XSTD_CHORE_SCHEDULER( [ ] ( void* ) { return true; }, nullptr, func, arg, priority );
#else
		std::thread( std::forward<T>( fn ) ).detach();
#endif
	}

	// Registers a chore to be invoked when an even occurs.
	//
	template<typename T> requires Invocable<T, void>
	inline void chore( T&& fn, const event& evt, int64_t priority = -1 )
	{
#ifdef XSTD_CHORE_SCHEDULER
		auto [func, arg, _] = flatten( std::forward<T>( fn ) );
		auto [ffunc, farg, __] = flatten( [ evt ] ()
		{
			return evt->signalled();
		} );
		XSTD_CHORE_SCHEDULER( ffunc, farg, func, arg, priority );
#else
		chore( [ fn = std::forward<T>( fn ), evt ] ()
		{
			evt->wait();
			fn();
		} );
#endif
	}
	template<typename T> requires Invocable<T, void>
	inline void chore( T&& fn, const event& evt, duration timeout, int64_t priority = -1 )
	{
#ifdef XSTD_CHORE_SCHEDULER
		auto [func, arg, _] = flatten( std::forward<T>( fn ) );
		auto [ffunc, farg, __] = flatten( [ evt, to = time::now() + timeout ] ()
		{
			return evt->signalled() || time::now() >= t0;
		} );
		XSTD_CHORE_SCHEDULER( ffunc, farg, func, arg, priority );
#else
		chore( [ fn = std::forward<T>( fn ), evt, t = timeout / 1ms ] ()
		{
			evt->wait_for( t );
			fn();
		} );
#endif
	}
};