#pragma once
#include "intrinsics.hpp"
#include "type_helpers.hpp"
#include "assert.hpp"
#include <atomic>

namespace xstd
{
	// Standalone way to raise task priority for the scope.
	//
	template<uint8_t TP>
	struct scope_tpr
	{
#if XSTD_HAS_TASK_PRIORITY
		uint8_t prev;
		__forceinline scope_tpr( uint8_t prev = get_task_priority() ) : prev( prev )
		{
			lock();
		}
		__forceinline void lock()
		{
			dassert( prev <= TP );
			set_task_priority( TP );
		}
		__forceinline void unlock()
		{
			dassert( get_task_priority() == TP );
			set_task_priority( prev );
		}
		__forceinline ~scope_tpr()
		{
			unlock();
		}
#else
		scope_tpr( uint8_t prev = 0 ) {}
#endif
	};

	// Raises caller to a specific task priority upon lock and lowers on unlock. Ignored for shared lockers.
	//
	template<Lockable Mutex, uint8_t TaskPriority, bool Optimized = false>
	struct task_guard
	{
		Mutex mutex;
		std::atomic<uint16_t> ex_depth = 0;
		std::atomic<uint8_t> ex_tp = 0;

		template<typename... Tx>
		__forceinline constexpr task_guard( Tx&&... args ) : mutex( std::forward<Tx>()... ) {}

		// Common helpers for raising/lowering of the task priority.
		//
		__forceinline static uint8_t raise( bool raised = false )
		{
#if XSTD_HAS_TASK_PRIORITY
			if ( raised )
			{
				dassert( get_task_priority() == TaskPriority );
				return TaskPriority;
			}
			else
			{
				uint8_t prio = get_task_priority();
				dassert( prio <= TaskPriority );
				set_task_priority( TaskPriority );
				return prio;
			}
#else
			return 0;
#endif
		}
		__forceinline static void lower( uint8_t prev )
		{
#if XSTD_HAS_TASK_PRIORITY
			set_task_priority( prev );
#endif
		}

		// Wrap the mutex/recursive_mutex interface.
		//
		__forceinline void lock( bool raised = false )
		{
#if !XSTD_HAS_TASK_PRIORITY
			mutex.lock();
#else
			// Raise the task priority.
			//
			auto prev = raise( raised );

			// If optimization requested and mutex is capable:
			//
			bool locked = false;
			if constexpr ( TryLockable<Mutex> && TimeLockable<Mutex> && Optimized )
			{
				// If optimization is viable:
				//
				if ( prev == 0 )
				{
					while ( 1 )
					{
						// Try to lock.
						//
						locked = mutex.try_lock_for( 10ms );
						if ( locked ) break;

						// Lower the task priority and yield the thread.
						//
						set_task_priority( 0 );
						std::this_thread::sleep_for( 5ms );

						// Raise the task priority again.
						//
						set_task_priority( TaskPriority );
					}
				}
			}
			if ( !locked )
				mutex.lock();

			// If not recursive, store the previous priority.
			//
			if ( !ex_depth++ )
				ex_tp.store( prev, std::memory_order::acquire );
			else
				dassert( prev == TaskPriority );
#endif
		}
		__forceinline bool try_lock( bool raised = false ) requires TryLockable<Mutex>
		{
			// Raise the prioriy and attempt at locking.
			//
			auto prev = raise( raised );
			bool state = mutex.try_lock();

#if XSTD_HAS_TASK_PRIORITY
			// If successful, store the priority if not recursive.
			//
			if ( state )
			{
				if ( !ex_depth++ )
					ex_tp.store( prev, std::memory_order::acquire );
				else
					dassert( prev == TaskPriority );
			}
			// Otherwise, lower the priority back.
			//
			else if ( !raised )
			{
				lower( prev );
			}
#endif
			return state;
		}

		// Wrap the timed_mutex/timed_recursive_mutex interface.
		//
		template<Duration T>
		__forceinline bool try_lock_for( const T& dur, bool raised = false ) requires TimeLockable<Mutex>
		{
			// Raise the prioriy and attempt at locking.
			//
			auto prev = raise( raised );
			bool state = mutex.try_lock_for( dur );

#if XSTD_HAS_TASK_PRIORITY
			// If successful, store the priority if not recursive.
			//
			if ( state )
			{
				if ( !ex_depth++ )
					ex_tp.store( prev, std::memory_order::release );
				else
					dassert( prev == TaskPriority );
			}
			// Otherwise, lower the priority back.
			//
			else if ( !raised )
			{
				lower( prev );
			}
#endif
			return state;
		}
		template<Timestamp T>
		__forceinline bool try_lock_until( const T& st, bool raised = false ) requires TimeLockable<Mutex>
		{
			// Raise the prioriy and attempt at locking.
			//
			auto prev = raise( raised );
			bool state = mutex.try_lock_until( st );

#if XSTD_HAS_TASK_PRIORITY
			// If successful, store the priority if not recursive.
			//
			if ( state )
			{
				if ( !ex_depth++ )
					ex_tp.store( prev, std::memory_order::release );
				else
					dassert( prev == TaskPriority );
			}
			// Otherwise, lower the priority back.
			//
			else if ( !raised )
			{
				lower( prev );
			}
#endif
			return state;
		}
		__forceinline void unlock()
		{
			// Load the previous task priority and unlock.
			//
#if XSTD_HAS_TASK_PRIORITY
			auto prev = ex_tp.load( std::memory_order::acquire );
			auto ndepth = --ex_depth;
#endif
			
			mutex.unlock();

			// If we're last in the recursive chain (where relevant), lower to the previous task priority.
			//
#if XSTD_HAS_TASK_PRIORITY
			if ( !ndepth )
				lower( prev );
#endif
		}

		// Shared redirects ignore task priority checks.
		//
		__forceinline auto lock_shared() requires SharedLockable<Mutex>
		{
			return mutex.lock_shared();
		}
		__forceinline auto try_lock_shared() requires SharedTryLockable<Mutex>
		{
			return mutex.lock_shared();
		}
		template<Duration T>
		__forceinline auto try_lock_shared_for( const T& dur ) requires SharedTimeLockable<Mutex>
		{
			return mutex.try_lock_shared_for( dur );
		}
		template<Timestamp T>
		__forceinline auto try_lock_shared_until( const T& st ) requires SharedTimeLockable<Mutex>
		{
			return mutex.try_lock_shared_until( st );
		}
		__forceinline auto unlock_shared() requires SharedLockable<Mutex>
		{
			return mutex.unlock_shared();
		}
	};
};