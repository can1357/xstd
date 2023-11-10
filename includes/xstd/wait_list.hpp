#pragma once
#include "type_helpers.hpp"
#include "coro.hpp"
#include "spinlock.hpp"
#include "event.hpp"
#include "chore.hpp"
#include "async.hpp"

namespace xstd {
	// Wait list implementation.
	//
	template<typename LockType, Scheduler S>
	struct basic_wait_list {
		static constexpr int32_t I = 2;
		LockType lock;
		S schedule;
		
		basic_wait_list() = default;
		basic_wait_list( S schedule ) : schedule( std::move( schedule ) ) {}
		basic_wait_list( const basic_wait_list& ) = delete;
		basic_wait_list& operator=( const basic_wait_list& ) = delete;
		~basic_wait_list() { signal_and_reset(); }

		// Container details.
		//
	protected:
		// Array of entries, reserved count is implicit from geometric allocation.
		// Indices are immutable.
		//
		std::array<coroutine_handle<>, I>  inline_list = { nullptr };
		coroutine_handle<>*                extern_list = nullptr;
		uint32_t                           next_index : 31 = 0;
		uint32_t                           settled    : 1 = 0;
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
		template<typename F>
		void for_each( F&& fn, coroutine_handle<>* ext, int32_t count ) {
			for ( auto& e : this->inline_list )
				if ( e ) fn( e );
			for ( int32_t i = I; i < count; i++ )
				if ( auto& e = ext[ i - I ] )
					fn( e );
		}
		void signal_and_reset( coroutine_handle<>* hnd = nullptr ) {
			auto count = next_index;
			auto alloc = std::exchange( extern_list, nullptr );
			this->settled = true;
			this->next_index = 0;
			if ( this->associated_event )
				this->associated_event->notify();
			this->for_each( [&]( coroutine_handle<>& e ) {
				if ( hnd && !*hnd )
					*hnd = e;
				else
					this->schedule( e )( );
				e = nullptr;
			}, alloc, count );
			if ( alloc )
				free( alloc );
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
			int32_t idx = (int32_t) next_index++;
			alloc_at( idx ) = a;
			return idx;
		}
		// Adds a callback listener.
		// 
		template<typename F, typename... Args>
		void then( F&& fn, Args&&... args ) {
			if ( is_settled() ) return fn();
			std::unique_lock g{ lock };
			if ( is_settled() ) {
				g.unlock();
				return fn();
			}
			int32_t idx = (int32_t) next_index++;
			constexpr auto runner = []( auto func ) -> deferred_task {
				co_return func();
			};
			alloc_at( idx ) = runner( std::bind_front( std::forward<F>( fn ), std::forward<Args>( args )... ) ).release();
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
			return this->settled;
		}
		// Registers an event and starts waiting inline.
		//
		void wait() {
			if ( auto* e = listen() )
				return e->wait();
		}
		bool wait_for( duration time ) {
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

		// Signals all listeners associated, returns a coro handle to symetric transfer to.
		//
		[[nodiscard]] coroutine_handle<> signal() {
			// If task priority is raised, we cannot sync. call any coroutines.
			//
			std::lock_guard g{ lock };
			if ( settled ) return noop_coroutine();
			coroutine_handle<> continuation = {};
			if constexpr ( XMutex<LockType> ) {
				if ( g.priority() >= XSTD_SYNC_TPR ) {
					continuation = noop_coroutine();
				}
			} else {
				if ( get_task_priority() >= XSTD_SYNC_TPR ) {
					continuation = noop_coroutine();
				}
			}

			// Signal all awakables and reset allocation, return the continuation handle.
			//
			signal_and_reset( &continuation );
			return continuation ? continuation : noop_coroutine();
		}
		void signal_async() {
			if ( auto h = signal(); h != noop_coroutine() )
				this->schedule( h );
		}
	};
	using wait_list = basic_wait_list<xspinlock<>, threadpool_scheduler>;
};