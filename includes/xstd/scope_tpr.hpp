#pragma once
#include "intrinsics.hpp"
#include "type_helpers.hpp"
#include "assert.hpp"
#include "spinlock.hpp"
#include <mutex>

namespace xstd
{
	// Changes the TPR in the scope.
	//
	template<uint8_t TP>
	struct scope_tpr
	{
		task_priority_t prev = 0;
		bool locked = false;

		// No copy.
		//
		scope_tpr( const scope_tpr& ) = delete;
		scope_tpr& operator=( const scope_tpr& ) = delete;

		FORCE_INLINE scope_tpr( std::adopt_lock_t ) : prev( TP ), locked( true ) {}
		FORCE_INLINE scope_tpr( std::defer_lock_t, task_priority_t _prev ) { lock( _prev ); }
		FORCE_INLINE scope_tpr( task_priority_t _prev = get_task_priority() ) { lock( _prev ); }
		FORCE_INLINE void lock( task_priority_t _prev = get_task_priority() )
		{
			fassert( !locked );
			locked = true;

			prev = _prev;
			dassert( prev <= TP );
			set_task_priority( TP );
		}
		FORCE_INLINE void unlock()
		{
			fassert( locked );
			locked = false;

			set_task_priority( prev );
		}
		FORCE_INLINE ~scope_tpr()
		{
			if ( locked )
				unlock();
		}
	};

	// Lock guard that raises TPR as well.
	//
	template<typename Mutex>
	struct task_lock
	{
		Mutex& ref;
		task_priority_t prev_tp = 0;
		const task_priority_t mtx_tp;
		bool locked = false;

		// No copy.
		//
		task_lock( const task_lock& ) = delete;
		task_lock& operator=( const task_lock& ) = delete;

		FORCE_INLINE task_lock( Mutex& ref, task_priority_t mtx_tp ) : ref( ref ), mtx_tp( mtx_tp ) { lock(); }
		FORCE_INLINE task_lock( std::defer_lock_t, Mutex& ref, task_priority_t mtx_tp ) : ref( ref ), mtx_tp( mtx_tp ) { lock(); }
		FORCE_INLINE task_lock( std::adopt_lock_t, Mutex& ref, task_priority_t mtx_tp ) : ref( ref ), prev_tp( mtx_tp ), mtx_tp( mtx_tp ), locked( true ) {}
		FORCE_INLINE task_lock( std::adopt_lock_t, Mutex& ref, task_priority_t mtx_tp, task_priority_t prev_tp ) : ref( ref ), prev_tp( prev_tp ), mtx_tp( mtx_tp ), locked( true ) {}

		FORCE_INLINE void lock()
		{
			fassert( !locked );
			locked = true;

			prev_tp = get_task_priority();
			dassert( prev_tp <= mtx_tp );

			if constexpr ( Same<Mutex, spinlock> )
			{
				while ( true )
				{
					set_task_priority( mtx_tp );
					if ( ref.try_lock() ) [[likely]]
						return;
					set_task_priority( prev_tp );
					
					do
						yield_cpu();
					while ( ref.locked() );
				}
			}
			else
			{
				set_task_priority( mtx_tp );
				ref.lock();
			}
		}
		FORCE_INLINE void unlock()
		{
			fassert( locked );
			locked = false;

			ref.unlock();
			set_task_priority( prev_tp );
		}
		FORCE_INLINE ~task_lock()
		{
			if ( locked )
				unlock();
		}

	};
};