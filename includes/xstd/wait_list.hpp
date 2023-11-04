#pragma once
#include "type_helpers.hpp"
#include "coro.hpp"
#include "spinlock.hpp"
#include "event.hpp"
#include "chore.hpp"

namespace xstd {
	// Structure representing a union of primitives that can receive signals.
	//
	struct TRIVIAL_ABI awakable {
		union {
			uint64_t value = 0;
			struct {
				uint64_t sync : 1;
				int64_t  ptr : 63;
			};
		};

		// Default and move ctors.
		//
		constexpr awakable() = default;
		constexpr awakable( std::nullptr_t ) {}
		constexpr awakable( awakable&& o ) noexcept = default;
		constexpr awakable& operator=( awakable&& o ) noexcept = default;
		constexpr awakable( const awakable& ) = default;
		constexpr awakable& operator=( const awakable& ) = default;

		// Comparison.
		//
		constexpr bool operator<( const awakable& o ) const { return value < o.value; }
		constexpr bool operator==( const awakable& o ) const { return value == o.value; }

		// Construction from the primitives.
		//
		awakable( event_handle evt ) { bind( evt ); }
		awakable( const event& evt ) { bind( evt ); }
		awakable( const event_primitive& evt ) { bind( evt ); }
		awakable( coroutine_handle<> hnd ) { bind( hnd ); }

		// Binding.
		//
		void reset() { value = 0; }
		void bind( event_handle evt ) { sync = true; ptr = (int64_t) (uint64_t) evt; }
		void bind( const event& evt ) { return bind( evt.handle() ); }
		void bind( const event_primitive& evt ) { return bind( evt.handle() ); }
		void bind( coroutine_handle<> hnd ) { sync = false; ptr = (int64_t) (uint64_t) hnd.address(); }
		constexpr void swap( awakable& o ) { std::swap( value, o.value ); }

		// Observers.
		//
		bool is_event() const { return ptr && sync; }
		bool is_coroutine() const { return ptr && !sync; }
		void* address() const { return (void*) (uint64_t) ptr; }
		event_handle get_event() const { return sync ? (event_handle) this->address() : nullptr; }
		coroutine_handle<> get_coroutine() const { return sync ? nullptr : coroutine_handle<>::from_address( this->address() ); }
		constexpr bool pending() const { return value != 0; }
		constexpr explicit operator bool() const { return pending(); }

		// Signalling.
		//
		void signal( coroutine_handle<>* sync_hnd ) {
			awakable target{ std::move( *this ) };
			if ( auto evt = target.get_event() ) {
				event_primitive::from_handle( evt ).notify();
			} else if ( auto coro = target.get_coroutine() ) {
				if ( sync_hnd && !*sync_hnd )
					*sync_hnd = coro;
				else
					chore( std::move( coro ) );
			}
		}
	};

	// Wait list implementation.
	//
	template<typename LockType>
	struct basic_wait_list {
		static constexpr int32_t I = 4;

		basic_wait_list() = default;
		basic_wait_list( const basic_wait_list& ) = delete;
		basic_wait_list& operator=( const basic_wait_list& ) = delete;
		~basic_wait_list() { signal_and_reset(); }

		// Container details.
		//
	protected:
		// Array of entries, reserved count is implicit from geometric allocation.
		// Indices are immutable.
		//
		std::array<awakable, I>  inline_list = { nullptr };
		awakable*                extern_list = nullptr;
		LockType                   lock;
		uint32_t                   next_index : 31 = 0;
		uint32_t                   settled    : 1 = 0;

		static constexpr int32_t get_capacity_from_size( int32_t size ) {
			if ( size <= I ) {
				return 0;
			} else {
				return (int32_t) ( std::bit_ceil( (uint32_t) size ) - I );
			}
		}
		awakable& ref_at( int32_t n ) {
			if ( n < I ) {
				return inline_list[ n ];
			} else {
				return extern_list[ n - I ];
			}
		}
		awakable& alloc_at( int32_t n ) {
			if ( n < I ) {
				return inline_list[ n ];
			} else {
				size_t old_capacity = get_capacity_from_size( n );
				size_t new_capacity = get_capacity_from_size( n + 1 );
				if ( old_capacity != new_capacity ) {
					extern_list = (awakable*) realloc( extern_list, sizeof( awakable ) * new_capacity );
					std::fill( extern_list + old_capacity, extern_list + new_capacity, nullptr );
				}
				return extern_list[ n - I ];
			}
		}
		template<typename F>
		void for_each( F&& fn, awakable* ext, int32_t count ) {
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
			this->for_each( [&]( awakable& e ) {
				e.signal( hnd );
				e = nullptr;
			}, alloc, count );
			if ( alloc )
				free( alloc );
		}
	public:
		// - Registrar interface.
		// Adds a new listener, returns the handle assigned for it or < 0 on failure.
		//
		int32_t listen( awakable a ) {
			if ( is_settled() ) return -1;
			std::lock_guard _g{ lock };
			if ( is_settled() ) return -1;
			int32_t idx = (int32_t) next_index++;
			alloc_at( idx ) = a;
			return idx;
		}
		// Removes a listener by its previously assigned handle, returns false on failure.
		//
		bool unlisten( int32_t idx ) {
			if ( is_settled() ) return false;
			std::lock_guard _g{ lock };
			if ( !( 0 <= idx && idx < next_index ) )
				return false;
			return std::exchange( ref_at( idx ), nullptr ).pending();
		}
		// Removes a listener by its entry type, returns false on failure.
		//
		bool unlisten( awakable a ) {
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
		// Registers a wait block and starts waiting inline.
		//
		void wait() {
			event_primitive evt = {};
			if ( listen( evt ) >= 0 ) {
				evt.wait();
			}
		}
		bool wait_for( duration time ) {
			event_primitive evt = {};
			if ( int32_t idx = listen( evt ); idx >= 0 ) {
				if ( !evt.wait_for( time / 1ms ) ) {
					if ( unlisten( idx ) ) {
						return false;
					}
					evt.wait();
				}
			}
			return true;
		}
		// Awaitable.
		//
		struct awaiter {
			basic_wait_list& list;
			bool ok = false;

			bool await_ready() const {
				return list.is_settled();
			}
			bool await_suspend( coroutine_handle<> hnd ) {
				ok = list.listen( hnd ) >= 0;
				return ok;
			}
			bool await_resume() const {
				return ok;
			}
		};
		inline awaiter operator co_await() noexcept { return { *this }; }

		// - Provider interface.
		// Clears all entries and the sealed flag.
		//
		void clear() {
			std::lock_guard _g{ lock };
			signal_and_reset();
			settled = 0;
		}
		// Signals all listeners associated, returns a coro handle to symetric transfer to.
		//
		[[nodiscard]] coroutine_handle<> signal() {
			// If task priority is raised, we cannot sync. call any coroutines.
			//
			std::lock_guard g{ lock };
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
				xstd::chore( std::move( h ) );
		}
	};
	using wait_list =            basic_wait_list<noop_lock>;
	using concurrent_wait_list = basic_wait_list<xspinlock<XSTD_SYNC_TPR>>;
};