#pragma once
#include <atomic>
#include <thread>
#include <mutex>
#include "intrinsics.hpp"
#include "assert.hpp"

namespace xstd
{
	struct spinlock
	{
		std::atomic<bool> value = false;

		FORCE_INLINE bool try_lock()
		{
			return !value.exchange( true, std::memory_order::acquire );
		}
		FORCE_INLINE void lock()
		{
			while ( !try_lock() )
				yield_cpu();
		}
		FORCE_INLINE void unlock()
		{
			value.store( false, std::memory_order::release );
		}
	};

	struct recursive_spinlock
	{
		std::atomic<std::thread::id> owner = {};
		int32_t depth = 0;

		FORCE_INLINE bool try_lock()
		{
			std::thread::id expected = {};
			std::thread::id desired = std::this_thread::get_id();
			if ( !owner.compare_exchange_strong( expected, desired, std::memory_order::acquire ) )
			{
				if ( desired != expected )
					return false;
			}
			++depth;
			return true;
		}
		FORCE_INLINE void lock()
		{
			while ( !try_lock() )
				yield_cpu();
		}
		FORCE_INLINE void unlock()
		{
			if( !--depth )
				owner.store( std::thread::id{}, std::memory_order::release );
		}
	};

	struct shared_spinlock
	{
		std::atomic<int32_t> counter = 0;

		FORCE_INLINE bool try_lock()
		{
			int32_t expected = 0;
			return counter.compare_exchange_strong( expected, -1, std::memory_order::acquire );
		}
		FORCE_INLINE bool try_lock_shared()
		{
			int32_t value = counter.load();
			while ( value >= 0 )
			{
				if ( counter.compare_exchange_strong( value, value + 1, std::memory_order::acquire ) )
					return true;
				yield_cpu();
			}
			return false;
		}

		FORCE_INLINE void lock()
		{
			while ( !try_lock() )
				yield_cpu();
		}
		FORCE_INLINE void lock_shared()
		{
			while ( !try_lock_shared() )
				yield_cpu();
		}

		FORCE_INLINE void unlock()
		{
			int32_t expected = -1;
			dassert_s( counter.compare_exchange_strong( expected, 0, std::memory_order::release ) );
		}
		FORCE_INLINE void unlock_shared()
		{
			dassert_s( counter-- >= 0 );
		}
	};
};