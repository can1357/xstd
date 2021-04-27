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
	struct alignas( 16 ) basic_recursive_spinlock
	{
		using cid_t = decltype( CidGetter{}() );
		
		// Current owner of the lock and the depth.
		//
		cid_t owner = {};
		std::atomic<uint32_t> depth = 0;
		
		// Dummy constructor for template deduction.
		//
		FORCE_INLINE basic_recursive_spinlock( CidGetter ) {}

		// Default constructor.
		//
		FORCE_INLINE basic_recursive_spinlock() {}
		FORCE_INLINE basic_recursive_spinlock( cid_t owner, int32_t depth ) : owner( owner ), depth( depth ) {}

		// Implement the standard mutex interface.
		//
		FORCE_INLINE bool try_lock()
		{
			cid_t cid = CidGetter{}();
			if ( owner == cid && depth )
			{
				++depth;
				return true;
			}

			basic_recursive_spinlock expected = { {}, 0 };
			return cmpxchg( *this, expected, { cid, 1 } );
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
			dassert( owner == CidGetter{}() && depth );
			if ( !--depth )
				owner = {};
		}
	};
	namespace impl { inline constexpr auto get_tid = [ ] () { return std::this_thread::get_id(); }; };
	using recursive_spinlock = basic_recursive_spinlock<decltype(impl::get_tid)>;

	struct shared_spinlock
	{
		std::atomic<uint16_t> counter = 0;

		FORCE_INLINE bool try_lock()
		{
			uint16_t expected = 0;
			return counter.compare_exchange_strong( expected, UINT16_MAX, std::memory_order::acquire );
		}
		FORCE_INLINE bool try_upgrade()
		{
			uint16_t expected = 1;
			return counter.compare_exchange_strong( expected, UINT16_MAX, std::memory_order::acquire );
		}
		FORCE_INLINE bool try_lock_shared()
		{
			uint16_t value = counter.load();
			while ( value != UINT16_MAX )
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
				while ( counter == UINT16_MAX )
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
			uint16_t expected = UINT16_MAX;
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