#pragma once
#include <memory>
#include <functional>
#include <atomic>
#include "time.hpp"
#include "intrinsics.hpp"
#include "type_helpers.hpp"

// [Configuration]
// XSTD_OS_EVENT_PRIMITIVE: If set, events will use the given OS primitive (wrapped by a class) instead of the std::future<> waits.
//
#ifndef XSTD_OS_EVENT_PRIMITIVE
#include <future>
namespace xstd
{
	struct event_primitive
	{
		std::promise<void> promise;
		std::future<void> future;
		inline event_primitive() { reset(); }

		// Implement the interface.
		// - void wait() const
		// - bool wait_for( ms ) const
		// - void reset()
		// - void notify()
		// - auto handle() const
		// - static void wait_from_handle( handle_t )
		// - static bool wait_from_handle( handle_t, ms )
		//
		inline void wait() const
		{
			future.wait();
		}
		inline bool wait_for( long long milliseconds ) const
		{
			return future.wait_for( time::milliseconds{ milliseconds } ) == std::future_status::ready;
		}
		inline void reset()
		{
			promise = {};
			future = promise.get_future();
		}
		inline void notify() 
		{ 
			promise.set_value(); 
		}

		inline auto handle() const { return this; }
		static void wait_from_handle( const event_primitive* handle ) { return handle->wait(); }
		static bool wait_from_handle( const event_primitive* handle, long long milliseconds ) { return handle->wait_for( milliseconds ); }
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
	struct event_base
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

		// Wrap all functions with a flag to reduce latency and to prevent races in the primitive.
		// & 1 = signalled
		// & 2 = resetting
		//
		std::atomic<uint16_t> flag = 0;
		
		// Checks whether or nor the event is signalled.
		//
		inline bool signalled() const { return flag.load( std::memory_order::relaxed ) & 1; }

		// Tries to reset the event flag, fails if already reset.
		//
		inline bool reset()
		{
			// Expect signalled flag and try to acquire the reset ownership.
			//
			uint16_t expected = 1;
			if ( flag.compare_exchange_strong( expected, 1 | 2, std::memory_order::seq_cst ) )
			{
				// Reset the primitive, then reset the flag.
				//
				primitive.reset();
				std::atomic_thread_fence( std::memory_order::seq_cst );
				flag.store( 0, std::memory_order::release );
				return true;
			}
			return false;
		}
		
		// Tries to set the event flag, fails if already set.
		// - If relaxed parameter is set, it is assumed that this event will be never reset and a simpler approach will be taken.
		//
		inline bool notify( bool relaxed = false )
		{
			if ( relaxed )
			{
				if ( bit_set( flag, 0 ) ) [[unlikely]]
					return false;
				primitive.notify();
				return true;
			}
			else
			{
				uint16_t expected = flag.load( std::memory_order::relaxed );
				while ( true )
				{
					// Spin while in-between states.
					//
					if ( expected & 2 )
					{
						yield_cpu();
						expected = flag.load( std::memory_order::relaxed );
						continue;
					}

					// Fail if already notified.
					//
					if ( expected & 1 )
						return false;

					// If we exchange succesfully, notify and indicate success.
					//
					if ( flag.compare_exchange_strong( expected, 1, std::memory_order::seq_cst ) )
					{
						primitive.notify();
						return true;
					}
				}
			}
		}

		// Waits for the event to be set.
		//
		inline void wait() const
		{
			if ( signalled() ) 
				return;
			primitive.wait();
		}
		inline bool wait_for( long long milliseconds ) const
		{
			if ( signalled() ) 
				return true;
			return primitive.wait_for( milliseconds );
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