#pragma once
#include <atomic>
#include <thread>
#include "intrinsics.hpp"

namespace xstd
{
	template<bool Busy = false>
	struct spinlock
	{
		std::atomic<bool> value = false;

		spinlock() {}
		spinlock( spinlock&& ) noexcept = default;
		spinlock( const spinlock& ) = delete;
		spinlock& operator=( spinlock&& ) noexcept = default;
		spinlock& operator=( const spinlock& ) = delete;

		FORCE_INLINE bool try_lock()
		{
			return !value.exchange( true, std::memory_order::acquire );
		}
		FORCE_INLINE void lock()
		{
			while ( !try_lock() )
			{
				if constexpr ( Busy )
					std::this_thread::yield();
				else
					yield_cpu();
			}
		}
		FORCE_INLINE void unlock()
		{
			value.store( false, std::memory_order::release );
		}
	};
};