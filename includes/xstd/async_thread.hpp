#pragma once
#include "chore.hpp"
#include "coro.hpp"
#include "event.hpp"
#include "assert.hpp"

namespace xstd {
	// Defines a thread that holds a coroutine that can pause and resume it via co_await / co_yield.
	//
	struct async_thread {
		struct promise_type {
			uint64_t             tid = 0;
			event                resume_event = {};
			event*               exit_event = {};
			bool                 detached = false;

			struct resume_thread {
				inline bool await_ready() noexcept { return false; }
				inline bool await_suspend( coroutine_handle<promise_type> handle ) noexcept {
					auto* promise = &handle.promise();
					if ( promise->tid == get_thread_uid() )
						return false;
					promise->resume_event.notify();
					return true;
				}
				inline void await_resume() noexcept {}
			};
			resume_thread yield_value( std::monostate ) { return {}; }
			async_thread get_return_object() { return *this; }
			suspend_always initial_suspend() { return {}; }
			suspend_always final_suspend() noexcept { return {}; }
			void return_void() {}
			XSTDC_UNHANDLED_RETHROW;
		};

		unique_coroutine<promise_type> handle;
		
		// Default and move construction.
		//
		async_thread() = default;
		async_thread( std::nullptr_t ) {}
		async_thread( async_thread&& ) noexcept = default;
		async_thread& operator=( async_thread&& ) noexcept = default;

		// Construction by underlying promise, thread starts immediately.
		//
		async_thread( promise_type& pr ) : handle( pr ) {
			start();
		}

		// Detach and join similar to std::thread.
		//
		void detach() {
			handle.promise().detached = true;
			if ( handle.done() ) {
				handle.reset();
			} else {
				handle.release();
			}
		}
		void join() {
			auto&& evt = get_temporary_event();
			handle.promise().exit_event = &evt;
			if ( !handle.done() ) {
				evt.wait();
			}
			handle.reset();
		}

		// Forward operator() to detach.
		//
		void operator()() { return detach(); }

		// Returns true if not yet finished / detached.
		//
		bool detached() const { return !handle; }
		bool running() const { return handle && !handle.done(); }
		explicit operator bool() const { return running(); }

		// Join on destruction if still remaining.
		//
		~async_thread() {
			if ( handle ) join();
		}

	private:
		void start() {
			dassert( handle );
			xstd::chore( [handle = handle.get()]() {
				auto& promise = handle.promise();
				promise.tid = get_thread_uid();
				while ( true ) {
					handle.resume();
					if ( handle.done() ) {
						if ( promise.exit_event ) {
							promise.exit_event->notify();
						} else if ( promise.detached ) {
							handle.destroy();
						}
						return;
					}
					promise.resume_event.wait();
					promise.resume_event.reset();
				}
			} );
		}
	};
};