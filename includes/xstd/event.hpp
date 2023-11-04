#pragma once
#include <memory>
#include <functional>
#include <atomic>
#include "time.hpp"
#include "intrinsics.hpp"
#include "type_helpers.hpp"
#include "bitwise.hpp"

// [[Configuration]]
// XSTD_OS_EVENT_PRIMITIVE: If set, events will use the given OS primitive (wrapped by a class) instead of the std::future<> waits.
//
#ifndef XSTD_OS_EVENT_PRIMITIVE

#if WINDOWS_TARGET
#pragma comment(lib, "ntdll.lib")
extern "C" {
	__declspec( dllimport ) int32_t NtCreateEvent( void** EventHandle, unsigned int DesiredAccess, void* ObjectAttributes, unsigned int EventType, unsigned char InitialState );
	__declspec( dllimport ) int32_t NtWaitForSingleObject( void* Handle, unsigned char Alertable, long long* Timeout );
	__declspec( dllimport ) int32_t NtSetEvent( void* EventHandle, long* PreviousState );
	__declspec( dllimport ) int32_t NtClearEvent( void* EventHandle );
	__declspec( dllimport ) int32_t NtClose( void* Handle );
};

namespace xstd {
	struct event_primitive {
		using handle_type = void*;

		handle_type hnd;

		event_primitive() { NtCreateEvent( &hnd, 0x2000000, nullptr, 0, false ); }
		~event_primitive() { NtClose( hnd ); }
		inline void wait() const { NtWaitForSingleObject( hnd, false, nullptr ); }
		inline bool wait_for( long long milliseconds ) const {
			auto ticks = milliseconds * -10000;
			return !NtWaitForSingleObject( hnd, false, &ticks );
		}
		inline void reset() { NtClearEvent( hnd ); }
		inline void notify() { NtSetEvent( hnd, nullptr ); }
		inline bool peek() const { return wait_for( 0 ); }
		inline handle_type handle() const { return hnd; }
		static event_primitive& from_handle( handle_type& evt ) { return *(event_primitive*) &evt; }
	};
};
#else
#include <mutex>
#include <condition_variable>
namespace xstd {
	struct event_primitive {
		using handle_type = event_primitive*;

		mutable std::condition_variable cv;
		mutable std::mutex mtx;
		bool state = false;

		event_primitive() {}
		~event_primitive() {}

		inline void wait() const {
			std::unique_lock lock{ mtx };
			if ( state ) return;
			cv.wait( lock );
		}
		inline bool wait_for( long long milliseconds ) const {
			std::unique_lock lock{ mtx };
			if ( state ) return true;
			return cv.wait_for( lock, time::milliseconds{ milliseconds } ) == std::cv_status::no_timeout;
		}
		inline void reset() {
			state = false;
		}
		inline void notify() {
			std::unique_lock lock{ mtx };
			if ( !state ) {
				state = true;
				cv.notify_all();
			}
		}
		inline bool peek() const {
			return state;
		}
		inline handle_type handle() const { return (handle_type) this; }
		static event_primitive& from_handle( handle_type& evt ) { return *evt; }
	};
};
#endif

#else
namespace xstd { using event_primitive = XSTD_OS_EVENT_PRIMITIVE; };
#endif

namespace xstd
{
	using event_handle = typename event_primitive::handle_type;

	// Wrap the event primitive with a easy to check flag.
	//
	struct event
	{
		event_primitive primitive = {};

		// Default constructible.
		//
		inline event() {}

		// No copy/move.
		//
		event( event&& ) noexcept = delete;
		event( const event& ) = delete;
		event& operator=( event&& ) noexcept = delete;
		event& operator=( const event& ) = delete;

		// Re-export the real interface.
		//
		inline event_handle handle() const { return primitive.handle(); }
		inline bool peek() const { return primitive.peek(); }
		inline void reset() { primitive.reset(); }
		inline void notify() { primitive.notify(); }
		inline void wait() const { primitive.wait(); }
		inline bool wait_for( long long milliseconds ) const { return primitive.wait_for( milliseconds ); }
		template<Duration T> 
		inline bool wait_for( T duration ) const { return wait_for( duration / 1ms ); }
	};
};