#pragma once
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <functional>
#include <atomic>
#include "time.hpp"
#include "intrinsics.hpp"
#include "type_helpers.hpp"

// [Configuration]
// XSTD_OS_EVENT_PRIMITIVE: If set, events will use the given OS primitive (wrapped by a class) instead of the atomic waits.
//
#ifndef XSTD_OS_EVENT_PRIMITIVE
namespace xstd
{
	struct event_primitive
	{
		mutable std::shared_timed_mutex mtx = {};
		inline default_event_primitive() { mtx.lock(); };
		inline ~default_event_primitive() { ( void ) mtx.try_lock(); mtx.unlock(); }

		// Implement the interface.
		// - void wait() const
		// - bool wait_for(ms) const
		// - void reset()
		// - void notify()
		// - auto handle() const
		// - static void wait_from_handle( handle_t )
		//
		inline void wait() const
		{
			mtx.lock_shared();
			mtx.unlock_shared();
		}
		inline bool wait_for( long long milliseconds ) const
		{
			if ( !mtx.try_lock_shared_for( time::milliseconds{ milliseconds } ) )
				return false;
			mtx.unlock_shared();
			return true;
		}
		inline void reset() { mtx.lock(); }
		inline void notify() { mtx.unlock(); }
		inline auto handle() const { return this; }

		static void wait_from_handle( const event_primitive* handle ) { return handle->wait(); }
	};
};
#else
namespace xstd { using event_primitive = XSTD_OS_EVENT_PRIMITIVE; };
#endif

namespace xstd
{
	// Event handle type.
	//
	using event_handle =    std::decay_t<decltype( std::declval<event_primitive>().handle() )>;

	// Wrap the event primitive with a easy to check flag.
	//
	struct event_base : event_primitive
	{
		event_primitive primitive = {};

		// Default constructable.
		//
		inline event_base() {};

		// No copy/move.
		//
		event_base( event_base&& ) noexcept = delete;
		event_base( const event_base& ) = delete;
		event_base& operator=( event_base&& ) noexcept = delete;
		event_base& operator=( const event_base& ) = delete;

		// Handle resolves into the event primitive.
		//
		inline event_handle handle() const { return primitive.handle(); }

		// Wrap all functions with a flag to reduce latency.
		//
		std::atomic<uint16_t> flag = 0;
		inline bool signalled() const
		{
			return flag.load( std::memory_order::relaxed );
		}
		inline void reset()
		{
			if ( bit_reset( flag, 0 ) )
				primitive.reset();
		}
		inline void wait() const
		{
			if ( signalled() ) return;
			primitive.wait();
		}
		inline bool wait_for( long long milliseconds ) const
		{
			if ( signalled() ) return true;
			return primitive.wait_for( milliseconds );
		}
		inline bool notify()
		{
			if ( bit_set( flag, 0 ) )
				return false;
			primitive.notify();
			return true;
		}

		// Wait for wrapper.
		//
		template<Duration T> 
		bool wait_for( T duration ) const { return wait_for( duration / 1ms ); }
	};
	using event = std::shared_ptr<event_base>;

	// Creates an event object.
	//
	inline event make_event() { return std::make_shared<event_base>(); }
};