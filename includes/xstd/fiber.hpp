#pragma once
#include "chore.hpp"
#include "coro.hpp"
#include "event.hpp"
#include "assert.hpp"
#include "ref_counted.hpp"

namespace xstd {
	// Defines a concept similar to async_task but with the difference that the state can be observed.
	//
	struct fiber {
		struct control_block : ref_counted<control_block> {
			std::atomic<bool> flag = false;
			event             signal = {};
		};

		struct promise_type {
			ref<control_block> blk = make_refc<control_block>();

			fiber get_return_object() { return { blk }; }
			suspend_never initial_suspend() noexcept { return {}; }
			suspend_never final_suspend() noexcept {
				blk->flag = true; 
				return {}; 
			}
			void return_void() {
				blk->flag = true;
			}
			XSTDC_UNHANDLED_RETHROW;

			~promise_type() {
				blk->flag = true;
				blk->signal.notify();
			}
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
		void join() {
			if ( !blk->flag.load( std::memory_order::relaxed ) ) {
				blk->signal.wait();
			}
		}

		// Returns true if not yet finished / detached.
		//
		bool detached() const { return !blk; }
		bool running() const { return blk && !blk->flag.load( std::memory_order::relaxed ); }
		explicit operator bool() const { return running(); }

		// Join on destruction if still remaining.
		//
		~fiber() {
			if ( blk ) join();
		}
	};
};