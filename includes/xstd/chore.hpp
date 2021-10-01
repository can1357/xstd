#pragma once
#include "intrinsics.hpp"
#include "time.hpp"
#include <thread>

// [Configuration]
// XSTD_CHORE_SCHEDULER: If set, chore will pass OS a callback to help with the scheduling.
//
#ifdef XSTD_CHORE_SCHEDULER
	extern "C" void __cdecl XSTD_CHORE_SCHEDULER( void( __cdecl* callback )( void* ), void* cb_arg, int64_t priority, int64_t due_time );
#endif

namespace xstd
{
	// Registers a chore that will be invoked either at a set time, or if left empty immediately but in an async context.
	//
	template<typename T>
	inline void chore( T&& fn, timestamp due_time, int64_t priority = -1 )
	{
#ifdef XSTD_CHORE_SCHEDULER
		auto [func, arg, _] = flatten( std::forward<T>( fn ) );
		XSTD_CHORE_SCHEDULER( func, arg, priority, due_time.time_since_epoch().count() );
#else
		std::thread( [ fn = std::forward<T>( fn ), due_time ]()
		{
			std::this_thread::sleep_until( due_time );
			fn();
		} ).detach();
#endif
	}
	template<typename T>
	inline void chore( T&& fn, duration delay, int64_t priority = -1 )
	{
		return chore( std::forward<T>( fn ), time::now() + delay, priority );
	}
	template<typename T>
	inline void chore( T&& fn, int64_t priority = -1 )
	{
#ifdef XSTD_CHORE_SCHEDULER
		auto [func, arg, _] = flatten( std::forward<T>( fn ) );
		XSTD_CHORE_SCHEDULER( func, arg, priority, 0 );
#else
		std::thread( std::forward<T>( fn ) ).detach();
#endif
	}
};