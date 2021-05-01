#pragma once
#include <mutex>
#include <shared_mutex> // For shared_lock.
#include "assert.hpp"

namespace xstd
{
	// Implements an upgradable shared mutex.
	//
	struct shared_mutex
	{
		// The mutex we're playing around.
		//
		std::mutex mutex = {};

		// Share counter, if negative exclusive.
		//
		std::atomic<int32_t> share_count = 0;

		// Replicate the STL interface.
		//
		FORCE_INLINE bool try_lock()
		{
			while ( !share_count )
			{
				if ( mutex.try_lock() )
				{
					share_count.fetch_or( -1 );
					return true;
				}
			}
			return false;
		}
		FORCE_INLINE void lock()
		{
			mutex.lock();
			share_count.fetch_or( -1 );
		}
		FORCE_INLINE bool try_lock_shared()
		{
			int32_t expected = share_count.load();

			#if GNU_COMPILER
				#pragma unroll(2)
			#endif
			while ( true )
			{
				// While shared, try incrementing the counter.
				//
				while ( expected > 0 )
					if ( share_count.compare_exchange_strong( expected, expected + 1 ) )
						return true;

				// If exclusive, fail.
				//
				if ( expected < 0 )
					return false;

				// If not owned, try locking, if successful break.
				//
				if ( mutex.try_lock() )
					break;

				// On transaction failure, yield the cpu and load the share count again.
				//
				yield_cpu();
				expected = share_count.load();
			}

			// Start the counter at one, indicate success.
			//
			dassert_s( ++share_count == 1 );
			return true;
		}
		FORCE_INLINE void lock_shared()
		{
			int32_t expected = share_count.load();

			#if GNU_COMPILER
				#pragma unroll(2)
			#endif
			while ( true )
			{
				// While shared, try incrementing the counter.
				//
				while ( expected > 0 )
					if ( share_count.compare_exchange_strong( expected, expected + 1 ) )
						return;

				// If exclusive, lock and break.
				//
				if ( expected < 0 )
				{
					mutex.lock();
					break;
				}

				// If not owned, try locking, if successful break.
				//
				if ( mutex.try_lock() )
					break;

				// On transaction failure, yield the cpu and load the share count again.
				//
				yield_cpu();
				expected = share_count.load();
			}

			// Start the counter at one.
			//
			dassert_s( ++share_count == 1 );
		}
		FORCE_INLINE void unlock()
		{
			share_count.fetch_and( 0 );
			mutex.unlock();
		}
		FORCE_INLINE void unlock_shared()
		{
			// Decrement share count.
			//
			int32_t value = --share_count;
			
			// Validate the result, if we were the sole owner unlock the mutex.
			//
			dassert( value >= 0 );
			if ( !value )
				mutex.unlock();
		}

		// Implement the upgrade.
		//
		FORCE_INLINE void upgrade()
		{
			int32_t expected = share_count.load();
			dassert( expected >= 1 );
			while ( true )
			{
				// If sole owner, try changing the state.
				//
				if ( expected == 1 )
				{
					if ( share_count.compare_exchange_strong( expected, -1 ) )
						return;
				}
				// Otherwise, try unlocking and re-locking.
				//
				else
				{
					if ( share_count.compare_exchange_strong( expected, expected - 1 ) )
						return lock();
				}
			}
		}
	};
};