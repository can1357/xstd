#pragma once
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <functional>
#include "intrinsics.hpp"
#include "type_helpers.hpp"

// [Configuration]
// XSTD_OS_EVENT_PRIMITIVE: If set, events will use the given OS primitive (wrapped by a class) instead of the atomic waits.
//
#ifndef XSTD_OS_EVENT_PRIMITIVE
	#define XSTD_OS_EVENT_PRIMITIVE xstd::event_primitive_default
#endif

namespace xstd
{
	// The event primitive.
	//
	struct event_primitive_default
	{
		std::atomic<bool> flag = false;
		mutable std::shared_timed_mutex mtx = {};

		// No copy/move.
		//
		event_primitive_default() { mtx.lock(); };
		event_primitive_default( event_primitive_default&& ) noexcept = delete;
		event_primitive_default( const event_primitive_default& ) = delete;
		event_primitive_default& operator=( event_primitive_default&& ) noexcept = delete;
		event_primitive_default& operator=( const event_primitive_default& ) = delete;
		~event_primitive_default() { if( !flag ) mtx.unlock(); }

		bool signalled() const { return flag; }
		bool wait_for( long long milliseconds ) const
		{
			if ( flag )
				return true;
			if ( mtx.try_lock_shared_for( time::milliseconds{ milliseconds } ) )
			{
				mtx.unlock_shared();
				return true;
			}
			return false;
		}
		void wait() const
		{ 
			if ( flag ) return;
			mtx.lock_shared();
			mtx.unlock_shared();
		}
		void reset()
		{
			mtx.lock();
			flag = false;
		}
		bool notify()
		{ 
			if ( flag )
				return false;
			flag.store( true, std::memory_order::relaxed );
			mtx.unlock();
			return true;
		}
	};
	using event_primitive = XSTD_OS_EVENT_PRIMITIVE;
	struct event_base : event_primitive
	{
		using event_primitive::event_primitive;
		
		template<Duration T> 
		bool wait_for( T duration ) const { return event_primitive::wait_for( duration / 1ms ); }

		// No copy/move.
		//
		event_base( const event_base& ) = delete;
		event_base& operator=( const event_base& ) = delete;
	};
	using event = std::shared_ptr<event_base>;

	// Creates an event object.
	//
	inline event make_event() { return std::make_shared<event_base>(); }
};