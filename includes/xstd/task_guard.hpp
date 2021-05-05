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
		FORCE_INLINE scope_tpr( uint8_t prev = get_task_priority() ) : prev( prev )
		{
			lock();
		}
		FORCE_INLINE void lock()
		{
			dassert( prev <= TP );
			set_task_priority( TP );
		}
		FORCE_INLINE void unlock()
		{
			dassert( get_task_priority() == TP );
			set_task_priority( prev );
		}
		FORCE_INLINE ~scope_tpr()
		{
			unlock();
		}
#else
		scope_tpr( uint8_t prev = 0 ) {}
#endif
	};

	// Raises caller to a specific task priority upon lock and lowers on unlock. Ignored for shared lockers.
	//
	template<Lockable Mutex, uint8_t TaskPriority>
	struct task_guard
	{
		Mutex mutex;
		uint16_t depth = 0;
		uint8_t prev_prio = 0;

		template<typename... Tx>
		FORCE_INLINE constexpr task_guard( Tx&&... args ) : mutex( std::forward<Tx>()... ) {}

		// Common helpers for raising/lowering of the task priority.
		//
		FORCE_INLINE static uint8_t raise( bool raised = false )
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
		FORCE_INLINE static void lower( uint8_t prev )
		{
#if XSTD_HAS_TASK_PRIORITY
			set_task_priority( prev );
#endif
		}

		// Wrap the mutex/recursive_mutex interface.
		//
		FORCE_INLINE void lock( bool raised = false )
		{
#if !XSTD_HAS_TASK_PRIORITY
			mutex.lock();
#else
			// Raise the task priority.
			//
			auto prev = raise( raised );

			// If mutex is capable:
			//
			bool locked = false;
			if constexpr ( LockCheckable<Mutex> )
			{
				// If optimization is viable:
				//
				if ( prev < TaskPriority )
				{
					while ( 1 )
					{
						// Try to lock.
						//
						locked = mutex.try_lock();
						if ( locked ) break;

						// Lower the task priority.
						//
						set_task_priority( prev );

						// Yield the CPU.
						//
						while ( mutex.locked() )
							yield_cpu();

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
			if ( !depth++ )
				prev_prio = prev;
			else
				dassert( prev == TaskPriority );
#endif
		}
		FORCE_INLINE bool try_lock( bool raised = false ) requires TryLockable<Mutex>
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
				if ( !depth++ )
					prev_prio = prev;
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
		FORCE_INLINE bool try_lock_for( const T& dur, bool raised = false ) requires TimeLockable<Mutex>
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
				if ( !depth++ )
					prev_prio = prev;
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
		FORCE_INLINE bool try_lock_until( const T& st, bool raised = false ) requires TimeLockable<Mutex>
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
				if ( !depth++ )
					prev_prio = prev;
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
		FORCE_INLINE void unlock()
		{
			// Load the previous task priority and unlock.
			//
#if XSTD_HAS_TASK_PRIORITY
			auto prev = prev_prio;
			auto ndepth = --depth;
#endif
			std::atomic_thread_fence( std::memory_order::seq_cst );
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
		FORCE_INLINE auto lock_shared() requires SharedLockable<Mutex>
		{
			return mutex.lock_shared();
		}
		FORCE_INLINE auto try_lock_shared() requires SharedTryLockable<Mutex>
		{
			return mutex.lock_shared();
		}
		template<Duration T>
		FORCE_INLINE auto try_lock_shared_for( const T& dur ) requires SharedTimeLockable<Mutex>
		{
			return mutex.try_lock_shared_for( dur );
		}
		template<Timestamp T>
		FORCE_INLINE auto try_lock_shared_until( const T& st ) requires SharedTimeLockable<Mutex>
		{
			return mutex.try_lock_shared_until( st );
		}
		FORCE_INLINE auto unlock_shared() requires SharedLockable<Mutex>
		{
			return mutex.unlock_shared();
		}
	};
};