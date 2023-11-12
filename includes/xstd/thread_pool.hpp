#pragma once
#include "time.hpp"
#include "event.hpp"
#include "spinlock.hpp"
#include "intrinsics.hpp"
#include <list>

// [[Configuration]]
// XSTD_TPOOL_START_THREAD: If set, calls the function to start the thread instead of std::thread.
//
#ifdef XSTD_TPOOL_START_THREAD
	extern "C" void __cdecl XSTD_TPOOL_START_THREAD( void( __cdecl* callback )( void* ), void* cb_arg );
#else
	#include <thread>
	#define XSTD_TPOOL_START_THREAD(cb, arg) std::thread{cb, arg}.detach()
#endif

namespace xstd {
	// Work item.
	//
	struct work_item {
		void( __cdecl*cb )( void* );
		void* arg;

		FORCE_INLINE void operator()() { cb( arg ); }
		constexpr bool is_ready( int64_t ) const { return true; }
	};
	struct deferred_work_item : work_item {
		static constexpr int64_t timeout_never = INT64_MAX;
		static constexpr int64_t timeout_now =   0;
		static int64_t get_time() { return time::now().time_since_epoch() / 1ns; }

		mutable event_handle evt = {};
		int64_t              timeout = 0;

		bool is_ready( int64_t now ) const {
			if ( timeout < now ) return true;
			if ( evt && event_primitive::from_handle( evt ).peek() ) return true;
			return false;
		}
	};

	// Queue type.
	//
	template<typename W>
	struct work_queue {
		xspinlock<>  lock =   {};
		event        signal = {};
		std::list<W> list =   {};
		bool         signal_busy = false;
		
		void on_pop() {
			if ( list.empty() )
				signal.reset();
		}
		void process( uint32_t& last_sleep, uint32_t max_sleep, work_queue<work_item>* sister_queue ) {
			// If nothing to do:
			//
			std::unique_lock g{ lock };
			if ( list.empty() ) {
				// If we can register a signal:
				//
				if ( !signal_busy ) {
					signal_busy = true;
					g.unlock();
					signal.wait();
					signal_busy = false;
					return;
				}
			} else {
				// If deferred: try to find ready entry, if found, push to immediate queue.
				//
				if constexpr ( std::is_same_v<W, deferred_work_item> ) {
					constexpr size_t max_traverse = 4;
					int64_t time = deferred_work_item::get_time();

					auto it = list.begin();
					for ( size_t i = 0; i != max_traverse; i++ ) {
						if ( it == list.end() ) {
							break;
						} 
						if ( it->is_ready( time ) ) {
							work_item rw{ *it };
							list.erase( it++ );
							on_pop();
							g.unlock();

							// Push to immediate queue or run in-place.
							//
							last_sleep = 0; // We did something!
							if ( sister_queue )
								return sister_queue->push( rw );
							else
								return rw();
						} else {
							list.splice( list.end(), list, it++ );
						}
					}
				}
				// Otherwise pop first entry, run.
				//
				else {
					last_sleep = 0; // We did something!
					work_item rw{ list.front() };
					list.pop_front();
					on_pop();
					g.unlock();
					return rw();
				}
			}
			g.unlock();

			// Stall.
			//
			if ( sister_queue ) {
				std::unique_lock g2{ sister_queue->lock, std::try_to_lock };
				if ( g2 && !sister_queue->list.empty() ) {
					work_item rw{ sister_queue->list.front() };
					sister_queue->list.pop_front();
					sister_queue->on_pop();
					g2.unlock();
					return rw();
				}
			}

			uint32_t new_sleep = std::min( max_sleep, ( last_sleep + 1 ) * 2 );
			if ( new_sleep < 7 ) {
				std::this_thread::yield();
			} else {
				std::this_thread::sleep_for( new_sleep * 1ms );
			}
			last_sleep = new_sleep;
		}
		void push( W work ) {
			std::unique_lock g{ lock };
			bool was_empty = list.empty();
			list.emplace_back( work );
			if ( was_empty ) {
				g.unlock();
				signal.notify();
			}
		}
	};

	// Thread-pool.
	//
	struct thread_pool {
		// One immediate queue and one deferred queue.
		//
		work_queue<work_item>          q_immediate = {};
		work_queue<deferred_work_item> q_deferred  = {};

		// Thread counts.
		//
#if DEBUG_BUILD
		size_t              num_threads_target = 7;
#else
		size_t              num_threads_target = std::clamp<size_t>( std::thread::hardware_concurrency(), 4, 16 );
#endif
		std::atomic<size_t> num_threads =        0;
		std::atomic<bool>   running =            false;

		// Thread logic.
		//
		static void thread_entry_point( void* ctx ) {
			auto& tp = *(thread_pool*) ctx;
			uint32_t id = ( uint32_t ) tp.num_threads++;
			bool is_master = id == 0;
			uint32_t last_sleep = 0;
			uint32_t max_sleep = 10 + ( 5 << ( std::min<uint32_t>( id, 15 ) >> 1 ) ); // 15ms-650ms sleep duration, preferring localized work.
			if ( is_master ) {
				while ( tp.running.load( std::memory_order::relaxed ) ) [[likely]]
					tp.q_deferred.process( last_sleep, max_sleep, &tp.q_immediate );
			} else {
				while ( tp.running.load( std::memory_order::relaxed ) ) [[likely]]
					tp.q_immediate.process( last_sleep, max_sleep, nullptr );
			}
			tp.num_threads--;
		}

		// Queues new work, starts the thread-pool if necessary.
		//
		void queue( void( __cdecl* cb )( void* ), void* arg, size_t delay = 0, event_handle event_handle = nullptr ) {
			work_item iw{ cb, arg };
			if ( event_handle || delay ) {
				deferred_work_item dw{ iw, event_handle };
				if ( delay )
					dw.timeout = deferred_work_item::get_time() + delay;
				else
					dw.timeout = deferred_work_item::timeout_never;
				q_deferred.push( dw );
			} else {
				q_immediate.push( iw );
			}
		}
		void queue_lazy( void( __cdecl* cb )( void* ), void* arg, size_t delay = 0, event_handle event_handle = nullptr ) {
			if ( !running.load( std::memory_order::relaxed ) ) [[unlikely]] {
				start();
			}
			return queue( cb, arg, delay, event_handle );
		}

		// Starts/stops the threads.
		//
		COLD void start() {
			if ( running.exchange( true ) )
				return;
			for ( size_t n = 0; n <= num_threads_target; n++ ) {
				XSTD_TPOOL_START_THREAD( &thread_entry_point, this );
			}
		}
		void wait_and_stop() {
			running = false;
			while ( num_threads )
				yield_cpu();
		}
	};
};