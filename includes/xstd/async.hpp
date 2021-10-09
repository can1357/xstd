#pragma once
#include "future.hpp"
#include "chore.hpp"
#include "time.hpp"
#include "future.hpp"
#include "result.hpp"

namespace xstd
{
	namespace impl
	{
		template<typename F, typename R>
		struct async_promise : promise<R, void>
		{
			F functor;
			promise<R, void> promise = make_promise<R, void>();

			async_promise( F&& functor ) : functor( std::forward<F>( functor ) ) {}
			void operator()() const noexcept { promise.resolve( functor() ); }
			unique_future<R, void> get_future() const { return promise; }
		};

		template<typename F>
		struct async_promise<F, void>
		{
			F functor;
			promise<void, void> promise = make_promise<void, void>();

			async_promise( F&& functor ) : functor( std::forward<F>( functor ) ) {}
			void operator()() const noexcept { functor(); promise.resolve(); }
			unique_future<void, void> get_future() const { return promise; }
		};

		template<typename F, typename T, typename S>
		struct async_promise<F, basic_result<T, S>>
		{
			F functor;
			promise<T, S> promise = make_promise<T, S>();

			async_promise( F&& functor ) : functor( std::forward<F>( functor ) ) {}
			void operator()() const noexcept { promise.emplace( functor() ); }
			unique_future<T, S> get_future() const { return promise; }
		};
	};

	// Invokes the function in an async manner.
	//
	template<typename F>
	auto async( F&& func )
	{
		using R = decltype( func() );

		impl::async_promise<F, R> promise{ std::forward<F>( func ) };
		auto future = promise.get_future();
		chore( std::move( promise ) );
		return future;
	}

	// Simple wrapper for a coroutine starting itself and destorying itself on finalization.
	//
	struct async_task
	{
		struct promise_type
		{
			async_task get_return_object() { return {}; }
			suspend_never initial_suspend() noexcept { return {}; }
			suspend_never final_suspend() noexcept { return {}; }
			void unhandled_exception() {}
			void return_void() {}
		};
		async_task() {}
	};

	namespace impl
	{
		struct time_awaitable
		{
			duration delay;

			template<Duration D>  time_awaitable( const D& delay ) : delay( std::chrono::duration_cast<duration>( delay ) ) {}
			template<Timestamp T> time_awaitable( const T& timestamp ) : time_awaitable( timestamp - xstd::time::now() ) {}

			inline bool await_ready() { return false; }
			inline void await_suspend( coroutine_handle<> h ) { chore( h, delay ); }
			inline void await_resume() {}
		};

		struct event_awaitable
		{
			event_handle evt;

			event_awaitable( event_handle evt ) : evt( evt ) {}
			event_awaitable( const event& evt ) : event_awaitable( evt->handle() ) {}
			event_awaitable( const event_base& evt ) : event_awaitable( evt.handle() ) {}
			event_awaitable( const event_primitive& evt ) : event_awaitable( evt.handle() ) {}

			inline bool await_ready() { return !evt; }
			inline void await_suspend( coroutine_handle<> h ) { chore( h, evt ); }
			inline void await_resume() {}
		};

		struct event_awaitable_timed
		{
			event_handle evt;
			duration timeout;

			template<Duration D> event_awaitable_timed( event_handle evt, const D& timeout ) : evt( evt ), timeout( std::chrono::duration_cast<duration>( timeout ) ) {}
			template<Duration D> event_awaitable_timed( const event& evt, const D& timeout ) : event_awaitable_timed( evt->handle(), timeout ) {}
			template<Duration D> event_awaitable_timed( const event_base& evt, const D& timeout ) : event_awaitable_timed( evt.handle(), timeout ) {}
			template<Duration D> event_awaitable_timed( const event_primitive& evt, const D& timeout ) : event_awaitable_timed( evt.handle(), timeout ) {}

			inline bool await_ready() { return !evt; }
			inline void await_suspend( coroutine_handle<> h ) { chore( h, evt, timeout ); }
			inline void await_resume() {}
		};

		template<typename T, typename S>
		struct promise_awaitable_timed
		{
			duration timeout;
			promise_ref<T, S, false> promise;
			std::unique_ptr<wait_block> wb = nullptr;

			template<Duration D>
			promise_awaitable_timed( promise_ref<T, S, false> _pr, const D& timeout ) : timeout( std::chrono::duration_cast<duration>( timeout ) ), promise( std::move( _pr ) )
			{
				auto& pr = *promise.ptr;

				// If promise is finished already, do not wait at all.
				//
				if ( pr.finished() )
					return;

				// Create a wait block and register it, if we can't due to a race, do not wait, we have the result.
				//
				wb = std::make_unique<wait_block>();
				if ( !pr.register_wait_block( *wb ) )
					wb.reset();
			}

			// Default move.
			//
			promise_awaitable_timed( promise_awaitable_timed&& ) noexcept = default;
			promise_awaitable_timed& operator=( promise_awaitable_timed&& ) noexcept = default;

			// If no waitblock registered, we already have our result.
			//
			inline bool await_ready() { return !wb; }

			// Register the time constrained wait.
			//
			inline void await_suspend( coroutine_handle<> h ) { chore( h, wb->event.handle(), timeout ); }

			// Check the status on resume and deregister.
			//
			basic_result<T, S> await_resume()
			{
				auto p = std::exchange( promise, {} );
				if ( !wb || !p.ptr->deregister_wait_block( *wb ) )
					return p.ptr->result;
				else
					return timeout_result<T, S>();
			}

			// If await_resume is not called and we have a registered wait-block on destruction, deregister it.
			//
			~promise_awaitable_timed()
			{
				if ( promise && wb )
					promise.ptr->deregister_wait_block( *wb );
			}
		};
	};

	// Switches to an async context.
	//
	struct yield
	{
		inline bool await_ready() { return false; }
		inline void await_suspend( coroutine_handle<> h ) { chore( h ); }
		inline void await_resume() {}
	};

	// Declare yield types and their deduction guides.
	//
	template<typename... Tx> struct yield_for;
	template<typename... Tx> yield_for( const Tx&... )->yield_for<Tx...>;
	template<typename... Tx> struct yield_until;
	template<typename... Tx> yield_until( const Tx&... )->yield_until<Tx...>;

	// Yields the coroutine for a given time.
	//
	template<Duration D>  struct yield_for<D>    : impl::time_awaitable { using impl::time_awaitable::time_awaitable; };
	template<Timestamp T> struct yield_until<T>  : impl::time_awaitable { using impl::time_awaitable::time_awaitable; };

	// Yields the coroutine until an event is signalled.
	//
	template<> struct yield_until<event>           : impl::event_awaitable { using impl::event_awaitable::event_awaitable; };
	template<> struct yield_until<event_handle>    : impl::event_awaitable { using impl::event_awaitable::event_awaitable; };
	template<> struct yield_until<event_base>      : impl::event_awaitable { using impl::event_awaitable::event_awaitable; };
	template<> struct yield_until<event_primitive> : impl::event_awaitable { using impl::event_awaitable::event_awaitable; };

	// Yields the coroutine until an event is signalled or the given timeout period passes.
	//
	template<Duration D> struct yield_until<event, D>           : impl::event_awaitable_timed { using impl::event_awaitable_timed::event_awaitable_timed; };
	template<Duration D> struct yield_until<event_handle, D>    : impl::event_awaitable_timed { using impl::event_awaitable_timed::event_awaitable_timed; };
	template<Duration D> struct yield_until<event_base, D>      : impl::event_awaitable_timed { using impl::event_awaitable_timed::event_awaitable_timed; };
	template<Duration D> struct yield_until<event_primitive, D> : impl::event_awaitable_timed { using impl::event_awaitable_timed::event_awaitable_timed; };

	// Yields the coroutine until a promise is signalled or the given timeout period passes.
	//
	template<typename T, typename S, bool O, Duration D> struct yield_for<promise_ref<T, S, O>, D> : impl::promise_awaitable_timed<T, S> { using impl::promise_awaitable_timed<T, S>::promise_awaitable_timed; };
	template<typename T, typename S, Duration D> struct yield_for<promise<T, S>, D>                : impl::promise_awaitable_timed<T, S> { using impl::promise_awaitable_timed<T, S>::promise_awaitable_timed; };
	template<typename T, typename S, Duration D> struct yield_for<future<T, S>, D>                 : impl::promise_awaitable_timed<T, S> { using impl::promise_awaitable_timed<T, S>::promise_awaitable_timed; };
	template<typename T, typename S, Duration D> struct yield_for<unique_future<T, S>, D>          : impl::promise_awaitable_timed<T, S> { using impl::promise_awaitable_timed<T, S>::promise_awaitable_timed; };
};