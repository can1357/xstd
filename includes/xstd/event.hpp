#pragma once
#include "intrinsics.hpp"
#include "type_helpers.hpp"
#include <mutex>
#include <memory>
#include <functional>

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
		std::mutex mtx = {};

		// No copy/move.
		//
		event_primitive_default() {};
		event_primitive_default( event_primitive_default&& ) noexcept = delete;
		event_primitive_default( const event_primitive_default& ) = delete;
		event_primitive_default& operator=( event_primitive_default&& ) noexcept = delete;
		event_primitive_default& operator=( const event_primitive_default& ) = delete;
		~event_primitive_default() {}

		bool signalled() const { return flag; }
		bool wait_for( long long milliseconds ); /*Not implemented.*/
		void wait() 
		{ 
			if ( flag )
				return;
			std::lock_guard _g{ mtx };
			if ( flag )
				return;
			flag.wait( false, std::memory_order::acquire ); 
		}
		void notify() 
		{ 
			flag.store( true, std::memory_order::relaxed );
			flag.notify_one(); 
		}
	};
	using event_primitive = XSTD_OS_EVENT_PRIMITIVE;
	struct event_wrapper : event_primitive
	{
		using event_primitive::event_primitive;
		template<Duration T> bool wait_for( T duration ) { return event_primitive::wait_for( duration / 1ms ); }
	};
	using event = std::shared_ptr<event_wrapper>;
	inline event make_event() { return std::make_shared<event_wrapper>(); }
};