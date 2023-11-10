#pragma once
#include "chore.hpp"
#include "coro.hpp"
#include "async.hpp"
#include "event.hpp"
#include "assert.hpp"
#include "ref_counted.hpp"
#include "wait_list.hpp"

namespace xstd {
	// Defines a concept similar to async_task but with the difference that the state can be observed.
	//
	struct fiber {
		struct control_block : ref_counted<control_block>, wait_list {};

		struct promise_type {
			ref<control_block> blk = make_refc<control_block>();


			fiber get_return_object() { return { blk }; }
			xstd::yield initial_suspend() noexcept { return {}; }
			suspend_never final_suspend() noexcept {
				blk->signal_async();
				return {};
			}
			xstd::yield yield_value( std::monostate ) { return {}; }
			void return_void() { blk->signal_async(); }
			XSTDC_UNHANDLED_RETHROW;
			~promise_type() { blk->signal_async(); }
		};
		
		ref<control_block> blk = nullptr;

		constexpr fiber() = default;
		constexpr fiber( std::nullptr_t ) {}
		fiber( ref<control_block> blk ) : blk{ std::move( blk ) } {}
		fiber( const fiber& ) = delete;
		fiber& operator=( const fiber& ) = delete;
		fiber( fiber&& o ) noexcept { swap( o ); }
		fiber& operator=( fiber&& o ) noexcept { swap( o ); return *this; }
		void swap( fiber& o ) {
			blk.swap( o.blk );
		}

		// Detach and join similar to std::thread.
		//
		void detach() { blk.reset(); }
		void join() { if ( blk ) blk->wait(); }

		// Awaiter mimicking join.
		//
		struct awaiter {
			control_block* blk;
			FORCE_INLINE inline bool await_ready() noexcept { return !blk; }
			FORCE_INLINE inline bool await_suspend( coroutine_handle<> handle ) noexcept { return blk->listen( handle ) >= 0; }
			FORCE_INLINE inline void await_resume() const noexcept {}
		};
		inline awaiter operator co_await() && noexcept { return { running() ? blk.get() : nullptr }; }

		// Returns true if not yet finished / detached.
		//
		bool detached() const { return !blk; }
		bool running() const { return blk && !blk->is_settled(); }
		explicit operator bool() const { return running(); }

		// Join on destruction if still remaining.
		//
		~fiber() {
			if ( blk ) join();
		}
	};
};