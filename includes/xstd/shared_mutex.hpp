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
		bool try_lock()
		{
			if ( !mutex.try_lock() )
				return false;
			int32_t expected = 0;
			dassert_s( share_count.compare_exchange_strong( expected, -1 ) );
			return true;
		}
		void lock()
		{
			mutex.lock();
			int32_t expected = 0;
			dassert_s( share_count.compare_exchange_strong( expected, -1 ) );
		}
		bool try_lock_shared()
		{
			int32_t expected = share_count;
			while ( true )
			{
				// Try incrementing the share count of a shared instance.
				//
				while ( expected > 0 )
					if ( share_count.compare_exchange_strong( expected, expected + 1 ) )
						return true;

				// Try locking, if it fails check expected one more time, if not exclusively owned loop.
				//
				if ( !mutex.try_lock() )
				{
					expected = share_count;
					if ( expected >= 0 )
					{
						yield_cpu();
						continue;
					}
					return false;
				}
				// Start the counter at one, indicate success.
				//
				else
				{
					expected = 0;
					dassert_s( share_count.compare_exchange_strong( expected, 1 ) );
					return true;
				}
			}
		}
		void lock_shared()
		{
			int32_t expected = share_count;
			while ( true )
			{
				// Try incrementing the share count of a shared instance.
				//
				while ( expected > 0 )
					if ( share_count.compare_exchange_strong( expected, expected + 1 ) )
						return;

				// Try locking, if it fails check expected one more time, if not exclusively owned loop, otherwise lock mutex and break.
				//
				if ( !mutex.try_lock() )
				{
					expected = share_count;
					if ( expected >= 0 )
					{
						yield_cpu();
						continue;
					}
					mutex.lock();
				}
				break;
			}

			// Start the counter at one.
			//
			expected = 0;
			dassert_s( share_count.compare_exchange_strong( expected, 1 ) );
		}
		void unlock()
		{
			int32_t expected = -1;
			dassert_s( share_count.compare_exchange_strong( expected, 0 ) );
			mutex.unlock();
		}
		void unlock_shared()
		{
			int32_t expected = share_count;
			dassert_s( expected >= 1 );
			while ( true )
			{
				if ( share_count.compare_exchange_strong( expected, expected - 1 ) )
				{
					if ( expected == 1 )
						mutex.unlock();
					return;
				}
				yield_cpu();
			}
		}

		// Implement the upgrade.
		//
		void upgrade()
		{
			int32_t expected = share_count;
			dassert_s( expected >= 1 );
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
				yield_cpu();
			}
		}
	};
};