#pragma once
#include "intrinsics.hpp"
#include "type_helpers.hpp"
#include <atomic>

namespace xstd
{
	// Raises caller to a specific task priority upon lock and lowers on unlock. Ignored for shared lockers.
	//
	template<Lockable Mutex, uintptr_t TP>
	struct task_guard
	{
		Mutex mutex;

		size_t ex_depth = 0;
		std::atomic<uintptr_t> ex_tp = 0;

		template<typename... Tx>
		__forceinline constexpr task_guard( Tx&&... args ) : mutex( std::forward<Tx>()... ) {}

		// Common helpers for raising/lowering of the task priority.
		//
		__forceinline static uintptr_t raise()
		{
			uintptr_t prio = get_task_priority();
			if ( prio < TP )
				set_task_priority( TP );
			return prio;
		}
		__forceinline static void lower( uintptr_t prev )
		{
			if ( prev < TP )
				set_task_priority( prev );
		}

		// Wrap the mutex/recursive_mutex interface.
		//
		__forceinline void lock( bool raised = false )
		{
			// Raise the priority and lock.
			//
			auto prev = raised ? TP : raise();
			mutex.lock();

			// If not recursive, store the previous priority.
			//
			if ( !ex_depth++ )
				ex_tp.store( prev, std::memory_order::release );
		}
		__forceinline bool try_lock( bool raised = false ) requires TryLockable<Mutex>
		{
			// Raise the prioriy and attempt at locking.
			//
			auto prev = raised ? TP : raise();
			bool state = mutex.try_lock();

			// If successful, store the priority if not recursive.
			//
			if ( state )
			{
				if ( !ex_depth++ )
					ex_tp.store( prev, std::memory_order::release );
			}
			// Otherwise, lower the priority back.
			//
			else
			{
				lower( prev );
			}
			return state;
		}

		// Wrap the timed_mutex/timed_recursive_mutex interface.
		//
		template<Duration T>
		__forceinline bool try_lock_for( const T& dur, bool raised = false ) requires TimeLockable<Mutex>
		{
			// Raise the prioriy and attempt at locking.
			//
			auto prev = raised ? TP : raise();
			bool state = mutex.try_lock_for( dur );

			// If successful, store the priority if not recursive.
			//
			if ( state )
			{
				if ( !ex_depth++ )
					ex_tp.store( prev, std::memory_order::release );
			}
			// Otherwise, lower the priority back.
			//
			else
			{
				lower( prev );
			}
			return state;
		}
		template<Timestamp T>
		__forceinline bool try_lock_until( const T& st, bool raised = false ) requires TimeLockable<Mutex>
		{
			// Raise the prioriy and attempt at locking.
			//
			auto prev = raised ? TP : raise();
			bool state = mutex.try_lock_for( st );

			// If successful, store the priority if not recursive.
			//
			if ( state )
			{
				if ( !ex_depth++ )
					ex_tp.store( prev, std::memory_order::release );
			}
			// Otherwise, lower the priority back.
			//
			else
			{
				lower( prev );
			}
			return state;
		}
		__forceinline void unlock()
		{
			// Load the previous task priority, if we're last in the recursive chain (if relevant at all), 
			// lower to the previous task priority.
			//
			auto prev = ex_tp.load( std::memory_order::acquire );
			if ( !--ex_depth )
				lower( prev );
			mutex.unlock();
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