#pragma once
#include "type_helpers.hpp"
#include "coro.hpp"
#include "spinlock.hpp"
#include "event.hpp"
#include "async.hpp"

namespace xstd {
	// Wait list implementation.
	//
	template<typename LockType>
	struct basic_wait_list {
		static constexpr int32_t I = 2;
		LockType lock;

		~basic_wait_list() { 
			this->signal_and_reset( chore_scheduler{}, true )();
		}

		// Container details.
		//
	protected:
		// Array of entries, reserved count is implicit from geometric allocation.
		// Indices are immutable.
		//
		std::array<coroutine_handle<>, I>  inline_list = { nullptr };
		coroutine_handle<>*                extern_list = nullptr;
		int32_t                            next_index = 0;
		std::optional<event>               associated_event = std::nullopt;

		static constexpr int32_t get_capacity_from_size( int32_t size ) {
			if ( size <= I ) {
				return 0;
			} else {
				return (int32_t) ( std::bit_ceil( (uint32_t) size ) - I );
			}
		}
		coroutine_handle<>& ref_at( int32_t n ) {
			if ( n < I ) {
				return inline_list[ n ];
			} else {
				return extern_list[ n - I ];
			}
		}
		coroutine_handle<>& alloc_at( int32_t n ) {
			if ( n < I ) {
				return inline_list[ n ];
			} else {
				size_t old_capacity = get_capacity_from_size( n );
				size_t new_capacity = get_capacity_from_size( n + 1 );
				if ( old_capacity != new_capacity ) {
					extern_list = (coroutine_handle<>*) realloc( extern_list, sizeof( coroutine_handle<> ) * new_capacity );
					std::fill( extern_list + old_capacity, extern_list + new_capacity, nullptr );
				}
				return extern_list[ n - I ];
			}
		}
		template<Scheduler S>
		coroutine_handle<> signal_and_reset( S&& sched, bool transfer_disable ) {
			int32_t count = 0;
			coroutine_handle<>* alloc = nullptr;
			{
				if ( is_settled() ) return noop_coroutine();
				std::unique_lock g{ lock };
				if ( is_settled() ) return noop_coroutine();
				count = std::exchange( next_index, -1 );
				alloc = std::exchange( extern_list, nullptr );
				if ( associated_event ) associated_event->notify();
				if ( !count ) return noop_coroutine();
				if constexpr ( XMutex<LockType> ) {
					transfer_disable = transfer_disable || g.priority() > 0;
				} else {
					transfer_disable = transfer_disable || get_task_priority() > 0;
				}
			}

			coroutine_handle<> transfer = nullptr;
			for ( auto& e : inline_list ) {
				if ( transfer ) sched( transfer )();
				transfer =      e;
			}
			for ( int32_t i = I; i < count; i++ ) {
				if ( transfer ) sched( transfer )();
				transfer =      alloc[ i - I ];
			}
			if ( alloc )
				free( alloc );

			if ( transfer ) {
				if ( transfer_disable ) 
					sched( transfer )();
				else 
					return transfer;
			}
			return noop_coroutine();
		}
	public:
		// - Registrar interface.
		// 
		// Associates an event with the list if not yet done, returns the pointer or nullptr if already settled.
		// 
		event* listen() {
			if ( is_settled() ) return nullptr;
			std::unique_lock g{ lock };
			if ( is_settled() ) return nullptr;
			return &associated_event.emplace();
		}
		// 
		// Adds a new listener, returns the handle assigned for it or < 0 on failure.
		//
		int32_t listen( coroutine_handle<> a ) {
			if ( is_settled() ) return -1;
			std::lock_guard _g{ lock };
			if ( is_settled() ) return -1;
			int32_t idx = next_index++;
			alloc_at( idx ) = a;
			return idx;
		}
		// Adds a callback listener.
		// 
		template<typename F, typename... Args>
		void then( F&& fn, Args&&... args ) {
			auto fn_bound = std::bind_front( std::forward<F>( fn ), std::forward<Args>( args )... );
			if ( is_settled() ) return (void) fn_bound();
			std::unique_lock g{ lock };
			if ( is_settled() ) {
				g.unlock();
				return (void) fn_bound();
			}
			int32_t idx = next_index++;
			constexpr auto runner = []( auto func ) -> deferred_task {
				( void ) func();
				co_return;
			};
			alloc_at( idx ) = runner( std::move( fn_bound ) ).release();
		}
		// Removes a listener by its previously assigned handle, returns false on failure.
		//
		bool unlisten( int32_t idx ) {
			if ( is_settled() ) return false;
			std::lock_guard _g{ lock };
			if ( !( 0 <= idx && idx < next_index ) )
				return false;
			ref_at( idx ) = nullptr;
			return true;
		}
		// Removes a listener by its entry type, returns false on failure.
		//
		bool unlisten( coroutine_handle<> a ) {
			if ( !a || is_settled() ) return false;
			std::lock_guard _g{ lock };
			for ( int32_t i = 0; i != next_index; i++ ) {
				if ( auto& e = ref_at( i ); e == a ) {
					e = nullptr;
					return true;
				}
			}
			return false;
		}
		// Returns true if wait-list is finalized, no more entries are accepted and entries cannot be removed.
		//
		bool is_settled() const {
			return next_index < 0;
		}
		// Registers an event and starts waiting inline.
		//
		void wait() {
			if ( auto* e = listen() )
				return e->wait();
		}
		bool wait_for( duration time ) {
			if ( time <= 0ms ) 
				return is_settled();
			if ( auto* e = listen() )
				return e->wait_for( time );
			else
				return true;
		}
		// Awaitable.
		//
		struct awaiter {
			basic_wait_list& list;
			bool ok = false;

			bool await_ready() const { return false; }
			bool await_suspend( coroutine_handle<> hnd ) {
				ok = list.listen( hnd ) >= 0;
				return ok;
			}
			bool await_resume() const {
				return ok;
			}
		};
		inline awaiter operator co_await() noexcept { return { *this }; }

		// Signals all listeners associated, returns a coro handle to symmetric transfer to.
		//
		template<Scheduler S>
		[[nodiscard]] coroutine_handle<> signal( S&& sched ) {
			return this->signal_and_reset( sched, false );
		}
		[[nodiscard]] coroutine_handle<> signal() {
			return this->signal_and_reset( chore_scheduler{}, false );
		}
		void signal_sync() {
			this->signal_and_reset( noop_scheduler{}, true )();
		}
		void signal_async() {
			this->signal_and_reset( chore_scheduler{}, true )();
		}
	};
	using wait_list = basic_wait_list<xspinlock<>>;
};