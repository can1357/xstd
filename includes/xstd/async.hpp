#pragma once
#include "future.hpp"
#include "chore.hpp"
#include "time.hpp"
#include "result.hpp"

namespace xstd
{
	namespace impl
	{
		template<typename F, typename R>
		struct async_promise : promise<R, no_status>
		{
			F functor;
			promise<R, no_status> promise = make_promise<R, no_status>();

			async_promise( F&& functor ) : functor( std::forward<F>( functor ) ) {}
			void operator()() const noexcept { promise.resolve( functor() ); }
			unique_future<R, no_status> get_future() const { return promise; }
		};

		template<typename F>
		struct async_promise<F, void>
		{
			F functor;
			promise<void, no_status> promise = make_promise<void, no_status>();

			async_promise( F&& functor ) : functor( std::forward<F>( functor ) ) {}
			void operator()() const noexcept { functor(); promise.resolve(); }
			unique_future<void, no_status> get_future() const { return promise; }
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

	// Switches to an async context.
	//
	struct yield
	{
		bool await_ready() { return false; }
		void await_suspend( coroutine_handle<> h ) { chore( h ); }
		void await_resume() {}
	};

	// Yields the coroutine for a given time.
	//
	struct yield_for
	{
		duration delay;

		bool await_ready() { return false; }
		void await_suspend( coroutine_handle<> h ) { chore( h, delay ); }
		void await_resume() {}
	};

	// Yields the coroutine until an event is signalled.
	//
	struct yield_until
	{
		event_handle evt;

		yield_until( const event& evt ) : evt( evt->handle() ) {}
		yield_until( const event_base& evt ) : evt( evt.handle() ) {}
		yield_until( const event_primitive& evt ) : evt( evt.handle() ) {}

		bool await_ready() { return false; }
		void await_suspend( coroutine_handle<> h ) { chore( h, evt ); }
		void await_resume() {}
	};
};