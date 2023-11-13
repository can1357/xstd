#pragma once
#include "intrinsics.hpp"
#include "type_helpers.hpp"
#include "spinlock.hpp"
#include "event.hpp"
#include "time.hpp"
#include <list>
#include <deque>
#include <thread>

namespace xstd {
	// Work item.
	//
	struct work_item {
		void( __cdecl*cb )( void* );
		void* arg;

		FORCE_INLINE void operator()() { cb( arg ); }
		FORCE_INLINE constexpr bool is_ready( int64_t ) const { return true; }
	};
	struct deferred_work_item : work_item {
		mutable event_handle evt = {};
		int64_t              timeout = 0;

		FORCE_INLINE bool is_ready( int64_t now ) const {
			if ( timeout < now ) return true;
			return evt && event_primitive::from_handle( evt ).peek();
		}
	};

	// Worker type, allowing thread-pool to be customizable.
	//
	struct worker_state {
		worker_state*   next_idle = nullptr;
		event_primitive signal =    {};
		bool            idle =      false;
	};
	struct default_worker : worker_state {
		// Traits of the thread-pool.
		//
		static constexpr bool is_lazy = true;
		static size_t get_ideal_worker_count() {
			return std::clamp<size_t>( 2 * std::thread::hardware_concurrency(), 8, 32 );
		}
		static void create_thread( void( __cdecl* cb )( void* ), void* arg ) {
			std::thread{ cb, arg }.detach();
		}
		FORCE_INLINE static int64_t timestamp( int64_t delta_ns ) {
			return ( time::now().time_since_epoch() / 1ns ) + delta_ns;
		}

		// Worker hooks.
		//
		FORCE_INLINE void sleep( uint32_t milliseconds ) { std::this_thread::sleep_for( milliseconds * 1ms ); }
		FORCE_INLINE void wait( event_primitive& evt ) { evt.wait(); }
		FORCE_INLINE void execute( work_item item ) { item(); }
		FORCE_INLINE void after_drain() {}
	};

	// Immediate queue type.
	//
	template<typename W>
	struct work_queue;
	template<>
	struct work_queue<work_item> {
#if WINDOWS_TARGET
		static constexpr int32_t max_yield_per_acquire = 1024; // ~ 10ms re-scheduling hit, spin for ~1ms to avoid it.
#else
		static constexpr int32_t max_yield_per_acquire = 64;   // < 1ms re-scheduling hit
#endif

		xspinlock<>            lock = {};
		std::atomic<uint16_t>  pressure = 0;
		std::deque<work_item>  list =      {};
		worker_state*          idle_list = nullptr;
		std::atomic<bool>      is_empty = false;

		// Custom locking logic for worker-side to collect more tasks before committing a sleep.
		//
		FORCE_INLINE bool lock_as_worker( bool was_idle ) {
			// Increment pressure.
			//
			if ( !was_idle ) ++pressure;

			// Enter acquisition loop:
			//
			int32_t yields_left = max_yield_per_acquire;
			while ( true ) {
				// If list is empty yield.
				//
				if ( is_empty.load( std::memory_order::relaxed ) ) {
					if ( --yields_left <= 0 ) {
						yield_cpu();
						continue;
					}
				}

				// Increment task priority and try to lock..
				//
				set_task_priority( lock.task_priority );
				if ( lock.unwrap().try_lock() ) {
					// If list is not empty or if we tried too many times, decrement pressure and break.
					//
					bool idle = list.empty();
					if ( !idle || yields_left < 0 ) {
						--pressure;
						return idle;
					}

					// Unlock.
					//
					lock.unwrap().unlock();
				}

				// Lower TPR and yield.
				//
				set_task_priority( 0 );
				yield_cpu();
			}
		}
		FORCE_INLINE void unlock_as_worker() {
			lock.unwrap().unlock();
			set_task_priority( 0 );
		}

		// Gets the next worker to signal to increase pressure if 0.
		//
		FORCE_INLINE worker_state* locked_wakeup_one() {
			worker_state* w = idle_list;
			uint16_t expected = pressure.load( std::memory_order::relaxed );
			if ( w && !expected ) [[unlikely]] {
				++pressure; // Weakened, can be > 1 after execution, don't care.
				idle_list = w->next_idle;
				return w;
			}
			return nullptr;
		}

		// Drains the queue.
		//
		template<typename Worker>
		FORCE_INLINE void drain( Worker& worker, [[maybe_unused]] work_queue<work_item>& ) {
			// Acquire the lock, if there's nothing to do:
			//
			worker.idle = lock_as_worker( worker.idle );
			if ( worker.idle ) [[unlikely]] {
				// Insert into idle list and unlock.
				//
				worker.next_idle = idle_list;
				idle_list = &worker;
				unlock_as_worker();

				// Halt until signalled.
				//
				worker.wait( worker.signal );
				return worker.signal.reset();
			}

			// Pop the first entry.
			//
			work_item rw{ list.front() };
			list.pop_front();

			// If list is not empty wake up the next worker.
			//
			worker_state* w = nullptr;
			if ( list.empty() ) {
				is_empty.store( true, std::memory_order::relaxed );
			} else {
				w = locked_wakeup_one();
			}

			unlock_as_worker();
			if ( w ) w->signal.notify();

			// Execute the work.
			//
			return worker.execute( rw );
		}

		// Appends work to the queue.
		//
		NO_INLINE void push( void( __cdecl* cb )( void* ), void* arg ) {
			is_empty.store( false, std::memory_order::relaxed );
			std::unique_lock g{ lock };
			list.emplace_back( work_item{ cb, arg } );

			// If there is no pressure, wakeup next idle worker.
			//
			if ( worker_state* w = locked_wakeup_one() ) {
				g.unlock();
				w->signal.notify();
			}
		}

		// Wakes up all threads, used for stopping the thread pool.
		//
		void wakeup_all() {
			worker_state* w;
			{
				std::lock_guard _g{ lock };
				w = idle_list;
				idle_list = nullptr;
			}

			while ( w ) {
				++pressure;
				std::exchange( w, w->next_idle )->signal.notify();
			}
		}
	};

	// Deferred queue type (assumes only one worker is using it).
	//
	template<>
	struct work_queue<deferred_work_item> {
		static constexpr size_t max_locked_traversal = 32;
		
		xspinlock<>                   lock =      {};
		std::list<deferred_work_item> list =      {};
		worker_state*                 idle =      nullptr;
		uint32_t                      sleep_hint = 0;

		// Locking logic assuming proper TPR.
		//
		FORCE_INLINE void lock_as_worker() {
			while ( true ) {
				set_task_priority( lock.task_priority );
				if ( lock.unwrap().try_lock() ) {
					break;
				}
				set_task_priority( 0 );
				yield_cpu();
			}
		}
		FORCE_INLINE void unlock_as_worker() {
			lock.unwrap().unlock();
			set_task_priority( 0 );
		}

		// Drains the queue.
		//
		template<typename Worker>
		FORCE_INLINE void drain( Worker& worker, work_queue<work_item>& immediate ) {
			// Acquire the lock.
			//
			lock_as_worker();

			// If there is no work to do:
			//
			if ( list.empty() ) [[unlikely]] {
				// Set as idle entry and unlock.
				//
				idle = &worker;
				unlock_as_worker();

				// Halt until signalled.
				//
				worker.wait( worker.signal );
				return worker.signal.reset();
			}

			std::list<deferred_work_item> ready_list = {};
			auto split_ready_from = [ & ]( std::list<deferred_work_item>& other ) FORCE_INLINE {
				int64_t current_time = Worker::timestamp( 0 );
				for ( auto it = other.begin(); it != other.end(); ) {
					if ( auto entry = it++; entry->is_ready( current_time ) ) {
						ready_list.splice( ready_list.end(), other, entry );
					}
				}
			};

			// Separate the ready list, either via two-phase merge or full-scan.
			//
			if ( list.size() > max_locked_traversal ) [[unlikely]] {
				// Swap the list with an empty one.
				//
				std::list<deferred_work_item> backlog = {};
				list.swap( backlog );

				// Filter out the ready list while unlocked.
				//
				unlock_as_worker();
				split_ready_from( backlog );
				lock_as_worker();

				// If list has new entries, scan it, insert backlog at the end.
				//
				if ( !list.empty() ) {
					split_ready_from( list );
					list.splice( list.end(), backlog, backlog.begin(), backlog.end() );
				} 
				// Otherwise, swap the lists.
				//
				else {
					list.swap( backlog );
				}
			} else {
				// Filter out the ready list inline.
				//
				split_ready_from( list );
			}

			// Unlock and commit sleep if there's nothing to do.
			//
			unlock_as_worker();
			if ( ready_list.empty() ) {
				sleep_hint |= 15;
				sleep_hint <<= 1;
				return worker.sleep( sleep_hint & 255 );
			}

			// Insert all entries from the ready list into the immediate queue and return.
			//
			for ( work_item e : ready_list ) {
				immediate.push( e.cb, e.arg );
			}
			sleep_hint = 0;
		}

		// Appends work to the queue.
		//
		NO_INLINE void push( void( __cdecl* cb )( void* ), void* arg, int64_t timeout, event_handle evt ) {
			std::unique_lock g{ lock };
			list.emplace_front( deferred_work_item{ work_item{cb, arg}, evt, timeout } );

			// Wake up if idle.
			//
			if ( worker_state* w = idle ) {
				idle = nullptr;
				g.unlock();
				w->signal.notify();
			}
		}
		
		// Wakes up all threads, used for stopping the thread pool.
		//
		void wakeup_all() {
			worker_state* w;
			{
				std::lock_guard _g{ lock };
				w = idle;
				idle = nullptr;
			}
			if ( w ) w->signal.notify();
		}
	};

	// Thread-pool.
	//
	template<typename Worker = default_worker>
	struct thread_pool {
		// Immediate and the deferred queue.
		//
		work_queue<work_item>          queue = {};
		work_queue<deferred_work_item> deferred_queue  = {};

		// Thread pool state.
		//
		std::atomic<size_t> num_threads = 0;
		spinlock            state_lock = {};
		bool                running = false;

		// Thread logic.
		//
		template<typename Queue>
		FORCE_INLINE void work( Queue& queue ) {
			Worker worker = {};
			this->num_threads++;
			while ( running ) [[likely]] {
				queue.drain( worker, this->queue );
				worker.after_drain();
			}
			this->num_threads--;
		}
		NO_INLINE static void aux_entry_point( void* ctx ) {
			auto& tp = *(thread_pool*) ctx;
			return tp.work( tp.queue );
		}
		NO_INLINE static void master_entry_point( void* ctx ) {
			auto& tp = *(thread_pool*) ctx;
			return tp.work( tp.deferred_queue );
		}

		// Queues new work, starts the thread-pool if lazily started.
		//
		FORCE_INLINE void push( void( __cdecl* cb )( void* ), void* arg, int64_t delay_ns = 0, event_handle evt = nullptr ) {
			if ( evt || delay_ns > ( 1ms / 1ns ) ) {
				int64_t timeout = INT64_MAX;
				if ( delay_ns > 0 ) timeout = Worker::timestamp( delay_ns );
				deferred_queue.push( cb, arg, timeout, evt );
			} else {
				queue.push( cb, arg );
			}

			if constexpr ( Worker::is_lazy ) {
				if ( !running ) [[unlikely]] {
					start_cold();
				}
			}
		}
		FORCE_INLINE void operator()( void( __cdecl* cb )( void* ), void* arg, int64_t delay_ns = 0, event_handle evt = nullptr ) { 
			return push( cb, arg, delay_ns, evt );
		}

		// Starts/stops the threads.
		//
		COLD void start_cold() {
			// Second protection needed since start may require arguments if not lazily started.
			//
			if constexpr ( Worker::is_lazy ) {
				start();
			}
		}

		template<typename... Tx>
		void start( Tx&&... args ) {
			std::lock_guard _g{ state_lock };
			if ( running ) return;
			running = true;

			size_t target_count = std::max<size_t>( 2, Worker::get_ideal_worker_count() );
			Worker::create_thread( &master_entry_point, this, args... );
			for ( size_t n = 1; n != target_count; n++ ) {
				Worker::create_thread( &aux_entry_point, this, args... );
			}
			while ( num_threads != target_count )
				yield_cpu();
		}
		void stop() {
			std::lock_guard _g{ state_lock };
			if ( !running ) return;
			running = false;

			while ( num_threads ) {
				queue.wakeup_all();
				deferred_queue.wakeup_all();
				yield_cpu();
			}
		}
	};
};