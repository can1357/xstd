#pragma once
#include "chore.hpp"
#include "coro.hpp"
#include "async.hpp"
#include "event.hpp"
#include "assert.hpp"
#include "ref_counted.hpp"
#include "wait_list.hpp"

namespace xstd {
	// Defines a concept similar to async_task but with the difference that the state can be observed and resumability.
	//
	struct fiber {
		enum action {
			pause,
			heartbeat
		};

		struct control_block : wait_list {
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

			void unref() {
				if ( !--ref_count )
					delete this;
			}
		};

		struct promise_type {
			control_block* blk = new control_block();

			struct pause_yield {
				control_block* cb = nullptr;
				action act = {};
				inline bool await_ready() { 
					if ( act == fiber::heartbeat ) {
						return cb->resume_address.load( std::memory_order::relaxed ) != control_block::resume_bad;
					} else {
						return false;
					}
				}
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
			pause_yield yield_value( action a ) { return { blk, a }; }
			void return_void() {}
			XSTDC_UNHANDLED_RETHROW;
			~promise_type() {
				blk->unref();
			}
		};
		
		control_block* blk = nullptr;

		constexpr fiber() = default;
		constexpr fiber( std::nullptr_t ) {}
		fiber( control_block* blk ) : blk{ std::move( blk ) } {}
		fiber( const fiber& o ) : blk( o.blk )  {
			if ( blk ) ++o.blk->ref_count;
		}
		fiber& operator=( const fiber& o ) {
			fiber copy{ o };
			swap( copy );
			copy.detach();
			return *this;
		}
		fiber( fiber&& o ) noexcept { swap( o ); }
		fiber& operator=( fiber&& o ) noexcept { swap( o ); return *this; }
		void swap( fiber& o ) {
			std::swap( blk, o.blk );
		}

		// Marks the fiber for death on its next pause.
		//
		void kill() {
			blk->try_kill();
		}

		// Signals the fiber to resume.
		//
		void resume_sync() {
			if ( auto h = blk->try_resume() )
				h();
		}
		bool resume() {
			if ( auto h = blk->try_resume() ) {
				chore( std::move( h ) );
				return true;
			}
			return false;
		}

		// Returns true if paused.
		//
		bool paused() const {
			return blk->resume_address.load( std::memory_order::relaxed ) > control_block::resume_pending;
		}

		// Detach and join similar to std::thread.
		//
		bool detach() {
			if ( !blk ) return false;
			kill();
			std::exchange( blk, nullptr )->unref();
			return true;
		}
		bool join() {
			if ( !blk ) return false;
			kill();
			blk->wait();
			return true;
		}

		// Returns true if not yet finished / detached.
		//
		bool detached() const { return !blk; }
		bool done() const { return !blk || blk->is_settled(); }
		explicit operator bool() const { return !done(); }

		// Join on destruction.
		//
		~fiber() {
			join();
		}
	};
};