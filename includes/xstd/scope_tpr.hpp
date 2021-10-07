#pragma once
#include "intrinsics.hpp"
#include "type_helpers.hpp"
#include "assert.hpp"
#include <mutex>

namespace xstd
{
	// Changes the TPR in the scope.
	//
	template<uint8_t TP>
	struct scope_tpr
	{
		uint8_t prev;
		bool locked = false;

		FORCE_INLINE scope_tpr( std::adopt_lock_t ) : prev( TP ), locked( true ) {}
		FORCE_INLINE scope_tpr( std::defer_lock_t, uint8_t prev = get_task_priority() ) : prev( prev ) {}
		FORCE_INLINE scope_tpr( uint8_t prev = get_task_priority() ) : prev( prev ) { lock(); }
		FORCE_INLINE void lock()
		{
			locked = true;
			dassert( prev <= TP );
			set_task_priority( TP );
		}
		FORCE_INLINE void unlock()
		{
			set_task_priority( prev );
			locked = false;
		}
		FORCE_INLINE ~scope_tpr()
		{
			if ( locked )
				unlock();
		}
	};
};