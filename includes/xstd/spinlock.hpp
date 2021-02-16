#pragma once
#include <atomic>
#include <thread>
#include "intrinsics.hpp"

namespace xstd
{
	template<bool Yield = true>
	struct spinlock
	{
		std::atomic<bool> value = false;

		void lock()
		{
			while ( value.exchange( true, std::memory_order::acquire ) )
			{
				if constexpr ( Yield )
					std::this_thread::yield();
				else
					yield_cpu();
			}
		}

		void unlock()
		{
			value.store( false, std::memory_order::release );
		}
	};
};