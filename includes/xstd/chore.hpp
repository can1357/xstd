#pragma once
#include "intrinsics.hpp"
#include "time.hpp"
#include "event.hpp"
#include <thread>

// [Configuration]
// XSTD_CHORE_SCHEDULER: If set, chore will pass OS two callbacks to help with the scheduling.
//
#ifdef XSTD_CHORE_SCHEDULER
	extern "C" void __cdecl XSTD_CHORE_SCHEDULER( bool( __cdecl* ready /*can be null*/ )( void* ), void* flt_arg, void( __cdecl* callback )( void* ), void* cb_arg );
#endif

namespace xstd
{
	// Registers a chore that will be invoked either at a set time, or if left empty immediately but in an async context.
	//
	template<typename T> requires Invocable<T, void>
	inline void chore( T&& fn, xstd::timestamp due_time = {} )
	{
#ifdef XSTD_CHORE_SCHEDULER
		auto [func, arg, _] = xstd::flatten( std::forward<T>( fn ) );
		if ( due_time == xstd::timestamp{} )
		{
			XSTD_CHORE_SCHEDULER( nullptr, nullptr, func, arg );
		}
		else
		{
			auto [ffunc, farg, __] = xstd::flatten( [ due_time ] ()
			{
				return xstd::time::now() >= due_time;
			} );
			XSTD_CHORE_SCHEDULER( ffunc, farg, func, arg );
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
#ifdef XSTD_CHORE_SCHEDULER
		auto [func, arg, _] = xstd::flatten( std::forward<T>( fn ) );
		auto [ffunc, farg, __] = xstd::flatten( [ evt ] ()
		{
			return evt->signalled();
		} );
		XSTD_CHORE_SCHEDULER( ffunc, farg, func, arg );
#else
		chore( [ fn = std::forward<T>( fn ), evt ] ()
		{
			evt->wait();
			fn();
		} );
#endif
	}
	template<typename T> requires Invocable<T, void>
	inline void chore( const event& evt, T&& fn, xstd::duration timeout )
	{
#ifdef XSTD_CHORE_SCHEDULER
		auto [func, arg, _] = xstd::flatten( std::forward<T>( fn ) );
		auto [ffunc, farg, __] = xstd::flatten( [ evt, to = xstd::time::now() + timeout ] ()
		{
			return evt->signalled() || xstd::time::now() >= t0;
		} );
		XSTD_CHORE_SCHEDULER( ffunc, farg, func, arg );
#else
		chore( [ fn = std::forward<T>( fn ), evt, t = timeout / 1ms ] ()
		{
			evt->wait_for( t );
			fn();
		} );
#endif
	}
};