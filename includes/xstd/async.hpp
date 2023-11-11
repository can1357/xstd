#pragma once
#include "coro.hpp"
#include "chore.hpp"
#include "time.hpp"

namespace xstd {
	// Define the concept of a scheduler and the default instances.
	//
	template<typename T>
	concept Scheduler = requires( T& sched, coroutine_handle<> hnd ) {
		hnd = sched( hnd );
	};
	struct noop_scheduler {
		constexpr coroutine_handle<> operator()( coroutine_handle<> handle ) const noexcept {
			return handle;
		}
	};
	struct chore_scheduler {
		noop_coroutine_handle operator()( coroutine_handle<> handle ) const noexcept {
			chore( handle );
			return noop_coroutine();
		}
	};
	struct scheduler_reference {
		coroutine_handle<>( *fn )( void*, coroutine_handle<> ) = nullptr;
		void* ctx = nullptr;

		template<Scheduler Sched> requires Empty<Sched>
		constexpr scheduler_reference( Sched&& ) {
			fn = []( void*, coroutine_handle<> hnd ) -> coroutine_handle<> {
				return Sched{}( hnd );
			};
		}
		template<Scheduler Sched>
		constexpr scheduler_reference( Sched& sched ) {
			ctx = &sched;
			fn = []( void* ctx, coroutine_handle<> hnd ) -> coroutine_handle<> {
				return ( *(Sched*) ctx )( hnd );
			};
		}
		coroutine_handle<> operator()( coroutine_handle<> handle ) const {
			return fn( ctx, handle );
		}
	};

	// Switches to an async context.
	//
	template<Scheduler Sched = chore_scheduler>
	struct yield {
		Sched schedule;

		constexpr yield() requires Empty<Sched> : schedule{} {}
		constexpr yield( Sched sched ) : schedule{ std::move( sched ) } {}

		inline bool await_ready() { return Same<Sched, noop_scheduler>; }
		inline auto await_suspend( coroutine_handle<> h ) { 
			auto result = schedule( h );
			if constexpr ( !Same<decltype( result ), noop_coroutine_handle> ) {
				return result;
			}
		}
		inline void await_resume() {}
	};

	// Simple wrapper for a coroutine starting itself and destroying itself on finalization.
	//
	struct async_task {
		struct promise_type {
			async_task get_return_object() { return {}; }
			suspend_never initial_suspend() noexcept { return {}; }
			suspend_never final_suspend() noexcept { return {}; }
			xstd::yield<> yield_value( std::monostate ) { return {}; }
			XSTDC_UNHANDLED_RETHROW;
			void return_void() {}
		};
		async_task() {}
	};

	// Deferred task wrapper for a packed but not yet ran function.
	//
	struct deferred_task {
		struct promise_type {
			deferred_task get_return_object() { return { *this }; }
			suspend_always initial_suspend() noexcept { return {}; }
			suspend_never final_suspend() noexcept { return {}; }
			XSTDC_UNHANDLED_RETHROW;
			void return_void() {}
		};
		unique_coroutine<promise_type> handle;
		deferred_task( promise_type& pr ) : handle( pr ) {}

		inline void operator()() { return handle.resume(); }
		inline coroutine_handle<> release() { return handle.release(); }
	};

	// Declare yield types and their deduction guides.
	//
	template<typename... Tx> struct yield_for;
	template<typename... Tx> yield_for( const Tx&... )->yield_for<Tx...>;
	template<typename... Tx> struct yield_until;
	template<typename... Tx> yield_until( const Tx&... )->yield_until<Tx...>;

	// Yields the coroutine for a given time.
	//
	template<>
	struct yield_for<> {
		duration delay;
		template<Duration D>  yield_for( const D& delay ) : delay( std::chrono::duration_cast<duration>( delay ) ) {}
		template<Timestamp T> yield_for( const T& timestamp ) : yield_for( timestamp.time_since_epoch() - time::now().time_since_epoch() ) {}
		inline bool await_ready() { return false; }
		inline void await_suspend( coroutine_handle<> h ) { chore( h, delay ); }
		inline void await_resume() {}
	};
	template<Duration D>  struct yield_for<D>    : yield_for<> { using yield_for<>::yield_for; };
	template<Timestamp T> struct yield_until<T>  : yield_for<> { using yield_for<>::yield_for; };

	// Yields the coroutine until an event is signalled.
	//
	template<> 
	struct yield_until<event_handle> {
		event_handle evt;
		yield_until( event_handle evt ) : evt( evt ) {}
		yield_until( const event& evt ) : evt( evt.handle() ) {}
		yield_until( const event_primitive& evt ) : evt( evt.handle() ) {}

		inline bool await_ready() { return !evt; }
		inline void await_suspend( coroutine_handle<> h ) { chore( h, evt ); }
		inline void await_resume() {}
	};
	template<> struct yield_until<event>           : yield_until<event_handle> { using yield_until<event_handle>::yield_until; };
	template<> struct yield_until<event_primitive> : yield_until<event_handle> { using yield_until<event_handle>::yield_until; };

	// Yields the coroutine until an event is signalled or the given timeout period passes.
	//
	template<Duration D>
	struct yield_until<event_handle, D> {
		event_handle evt;
		duration timeout;

		yield_until( event_handle evt, const D& timeout ) : evt( evt ), timeout( std::chrono::duration_cast<duration>( timeout ) ) {}
		yield_until( const event& evt, const D& timeout ) : yield_until( evt.handle(), timeout ) {}
		yield_until( const event_primitive& evt, const D& timeout ) : yield_until( evt.handle(), timeout ) {}

		inline bool await_ready() { return !evt; }
		inline void await_suspend( coroutine_handle<> h ) { chore( h, evt, timeout ); }
		inline void await_resume() {}
	};
	template<Duration D> struct yield_until<event, D>           : yield_until<event_handle, D> { using yield_until<event_handle, D>::yield_until; };
	template<Duration D> struct yield_until<event_primitive, D> : yield_until<event_handle, D> { using yield_until<event_handle, D>::yield_until; };
};