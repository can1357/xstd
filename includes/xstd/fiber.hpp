#pragma once
#include "chore.hpp"
#include "coro.hpp"
#include "async.hpp"
#include "event.hpp"
#include "assert.hpp"
#include "ref_counted.hpp"
#include "wait_list.hpp"

namespace xstd {
	// Defines a concept similar to async_task but with the difference that the state can be observed and externally resumed/paused.
	//
	struct fiber_control_block : wait_list {
		static constexpr uintptr_t resume_none =    0;
		static constexpr uintptr_t resume_bad =     1;
		static constexpr uintptr_t resume_pending = 2;

		std::atomic<uintptr_t> resume_address = resume_none;
		std::atomic<char>      ref_count      = 2;
		
		coroutine_handle<> try_suspend( coroutine_handle<> coro ) {
			uintptr_t address = resume_address.load( std::memory_order::relaxed );
			while ( true ) {
				switch ( address ) {
					case resume_none: {
						if ( resume_address.compare_exchange_strong( address, (uintptr_t) coro.address() ) ) {
							return noop_coroutine();
						}
						continue;
					}
					case resume_bad: {
						auto result = this->signal();
#if MS_COMPILER
						// coroutine is not fully suspended at await_suspend, non std behaviour, non std solutions.
						//
						chore( [coro] { coro.destroy(); }, 1s );
#else
						coro.destroy();
#endif
						return result;
					}
					default:
					case resume_pending: {
						if ( resume_address.compare_exchange_strong( address, resume_none ) ) {
							return coro;
						}
						continue;
					}

				}
			}
		}
		coroutine_handle<> try_resume() {
			uintptr_t address = resume_address.load( std::memory_order::relaxed );
			while ( true ) {
				switch ( address ) {
					case resume_none: {
						if ( resume_address.compare_exchange_strong( address, resume_pending ) ) {
							return nullptr;
						}
						continue;
					}
					case resume_bad:
						return nullptr;
					case resume_pending:
						return nullptr;
					default: {
						if ( resume_address.compare_exchange_strong( address, resume_pending ) ) {
							return coroutine_handle<>::from_address( (void*) address );
						}
						continue;
					}
				}
			}
		}
		void try_kill() {
			uintptr_t address = resume_address.load( std::memory_order::relaxed );
			while ( true ) {
				switch ( address ) {
					case resume_pending:
					case resume_none: {
						if ( resume_address.compare_exchange_strong( address, resume_bad ) ) {
							return;
						}
						continue;
					}
					case resume_bad:
						return;
					default: {
						if ( resume_address.compare_exchange_strong( address, resume_pending ) ) {
							this->signal_async();
							return coroutine_handle<>::from_address( (void*) address ).destroy();
						}
						continue;
					}
				}
			}
		}

		fiber_control_block* ref() {
			++ref_count;
			if ( is_settled() ) {
				unref();
				return nullptr;
			}
			return this;
		}
		void unref() {
			if ( !--ref_count )
				delete this;
		}
	};
	struct fiber_view {
		fiber_control_block* blk = nullptr;

		// Default, by pointer and null constructors.
		//
		constexpr fiber_view() = default;
		constexpr fiber_view( std::nullptr_t ) {}
		explicit fiber_view( fiber_control_block* blk ) : blk{ blk } {}

		// Make copyable and movable.
		//
		fiber_view( const fiber_view& o ) : blk{ o.blk ? o.blk->ref() : nullptr } {}
		fiber_view& operator=( const fiber_view& o ) {
			fiber_view copy{ o };
			swap( copy );
			return *this;
		}
		fiber_view( fiber_view&& o ) noexcept { swap( o ); }
		fiber_view& operator=( fiber_view&& o ) noexcept { swap( o ); return *this; }
		void swap( fiber_view& o ) {
			std::swap( blk, o.blk );
		}

		// Signals the fiber to resume.
		//
		template<Scheduler S>
		bool resume( S&& sched ) {
			if ( auto h = blk->try_resume() ) {
				sched(h)();
				return true;
			}
			return false;
		}
		bool resume_sync() {
			return resume( noop_scheduler{} );
		}
		bool resume() {
			return resume( chore_scheduler{} );
		}

		// Returns true if the fiber is currently executing.
		//
		bool running() const {
			return !blk || blk->resume_address.load( std::memory_order::relaxed ) <= fiber_control_block::resume_pending;
		}

		// Returns true if not yet complete.
		//
		bool pending() const { 
			return blk && !blk->is_settled(); 
		}
		explicit operator bool() const { 
			return pending(); 
		}

		// Marks the fiber for death next time it runs.
		//
		void kill() {
			if ( blk ) blk->try_kill();
		}

		// Waits until the fiber completes.
		//
		void join() {
			if ( blk ) blk->wait();
		}

		// Marks the fiber for death next time and waits until the fiber completes.
		//
		void destroy() {
			if ( blk ) {
				blk->try_kill();
				blk->wait();
			}
		}

		// Unref on destruction.
		//
		~fiber_view() {
			if ( blk ) blk->unref();
		}
	};
	struct fiber : fiber_view {
		struct promise_type {
			fiber_control_block* blk = new fiber_control_block();

			struct pause_yield {
				fiber_control_block* cb = nullptr;
				inline bool await_ready() { return false; }
				inline coroutine_handle<> await_suspend( coroutine_handle<> h ) {
					return cb->try_suspend( h );
				}
				inline void await_resume() {}
			};

			fiber get_return_object() { return { blk }; }
			suspend_never initial_suspend() noexcept { return {}; }
			suspend_never final_suspend() noexcept {
				blk->signal_async();
				return {};
			}
			pause_yield yield_value( std::monostate ) { return { blk }; }
			void return_void() {}
			XSTDC_UNHANDLED_RETHROW;
			~promise_type() {
				blk->unref();
			}
		};
		
		// Default, by pointer and null constructors.
		//
		constexpr fiber() = default;
		constexpr fiber( std::nullptr_t ) {}
		fiber( fiber_control_block* blk ) : fiber_view{ blk } {}

		// No copy, move by swap.
		//
		fiber( const fiber& ) = delete;
		fiber& operator=( const fiber& ) = delete;
		fiber( fiber&& o ) noexcept { swap( o ); }
		fiber& operator=( fiber&& o ) noexcept { swap( o ); return *this; }

		// Destroy on destruction.
		//
		~fiber() { destroy(); }
	};
};