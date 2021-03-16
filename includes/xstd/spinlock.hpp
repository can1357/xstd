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
			{
				while ( value )
					yield_cpu();
			}
		}
		FORCE_INLINE void unlock()
		{
			value.store( false, std::memory_order::release );
		}
	};

	template<DefaultConstructable CidGetter>
	struct basic_recursive_spinlock
	{
		using cid_t = decltype( CidGetter{}() );

		cid_t owner = {};
		int32_t depth = 0;

		FORCE_INLINE bool try_lock()
		{
			basic_recursive_spinlock expected = { {}, 0 };
			basic_recursive_spinlock desired = { CidGetter{}(), 1 };
			if ( cmpxchg( *this, expected, desired ) )
			{
				return true;
			}
			else if ( desired.owner == expected.owner )
			{
				++depth;
				return true;
			}
			return false;
		}
		FORCE_INLINE void lock()
		{
			while ( !try_lock() )
			{
				while( depth != 0 )
					yield_cpu();
			}
		}
		FORCE_INLINE void unlock()
		{
			if ( depth != 1 )
			{
				--depth;
			}
			else
			{
				auto thrd = CidGetter{}();
				basic_recursive_spinlock expected = { thrd, 1 };
				bool success = cmpxchg( *this, expected, { {}, 0 } );
				dassert( success );
			}
		}
	};
	namespace impl
	{
		inline constexpr auto get_tid = [ ] () { return std::this_thread::get_id(); };
	};
	using recursive_spinlock = basic_recursive_spinlock<decltype(impl::get_tid)>;

	struct shared_spinlock
	{
		std::atomic<int32_t> counter = 0;

		FORCE_INLINE bool try_lock()
		{
			int32_t expected = 0;
			return counter.compare_exchange_strong( expected, -1, std::memory_order::acquire );
		}
		FORCE_INLINE bool try_upgrade()
		{
			int32_t expected = 1;
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
			{
				while ( counter != 0 )
					yield_cpu();
			}
		}
		FORCE_INLINE void lock_shared()
		{
			while ( !try_lock_shared() )
			{
				while ( counter == -1 )
					yield_cpu();
			}
		}
		FORCE_INLINE void upgrade()
		{
			if ( !try_upgrade() )
			{
				unlock_shared();
				lock();
			}
		}

		FORCE_INLINE void downgrade()
		{
			int32_t expected = -1;
			dassert_s( counter.compare_exchange_strong( expected, 1, std::memory_order::release ) );
		}
		FORCE_INLINE void unlock()
		{
			counter = 0;
		}
		FORCE_INLINE void unlock_shared()
		{
			--counter;
		}
	};
};