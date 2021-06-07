#pragma once
#include <atomic>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include "intrinsics.hpp"
#include "bitwise.hpp"
#include "assert.hpp"

namespace xstd
{
	template<typename T, size_t N>
	struct basic_spinlock
	{
		std::atomic<T> value = 0;

		FORCE_INLINE bool try_lock()
		{
			return bit_set( value, N ) == 0;
		}
		FORCE_INLINE void unlock()
		{
			dassert( value );
			value.store( 0, std::memory_order::release );
		}
		FORCE_INLINE bool locked() const
		{
			return bit_test( value, N );
		}
		FORCE_INLINE void lock()
		{
			while ( !try_lock() ) [[unlikely]]
			{
				do
					yield_cpu();
				while ( locked() );
			}
		}
	};
	using spinlock = basic_spinlock<uint16_t, 0>;

	template<DefaultConstructable CidGetter>
	struct basic_recursive_spinlock
	{
		FORCE_INLINE static uint32_t get_cid() { return 1 + bit_cast<uint32_t>( CidGetter{}() ); }
		FORCE_INLINE static uint64_t combine( uint32_t owner, uint32_t depth ) { return owner | ( uint64_t( depth ) << 32 ); }
		FORCE_INLINE static std::pair<uint32_t, uint32_t> split( uint64_t value ) { return { uint32_t( value ), uint32_t( value >> 32 ) }; }

		// Current owner of the lock and the depth.
		//
		std::atomic<uint64_t> value = 0;
		
		// Dummy constructor for template deduction.
		//
		FORCE_INLINE basic_recursive_spinlock( CidGetter ) {}

		// Default constructor.
		//
		FORCE_INLINE basic_recursive_spinlock() {}

		// Implement the standard mutex interface.
		//
		FORCE_INLINE bool try_lock()
		{
			uint32_t cid = get_cid();
			uint64_t expected = value.load();
			while ( true )
			{
				auto [owner, depth] = split( expected );
				if ( depth )
				{
					if ( owner != cid )
						return false;
					value.store( combine( cid, depth + 1 ), std::memory_order::relaxed );
					return true;
				}

				expected = 0;
				if ( value.compare_exchange_strong( expected, combine( cid, 1 ), std::memory_order::acquire ) )
					return true;
			}
		}
		FORCE_INLINE void lock()
		{
			uint32_t cid = get_cid();
			uint64_t expected = value.load();
			auto [owner, depth] = split( expected );
			if ( owner == cid && depth )
			{
				value.store( combine( cid, depth + 1 ) );
				return;
			}

			while ( true )
			{
				while ( expected != 0 )
				{
					yield_cpu();
					expected = value.load( std::memory_order::relaxed );
				}
				if ( value.compare_exchange_strong( expected, combine( cid, 1 ), std::memory_order::acquire ) )
					return;
			}
		}
		FORCE_INLINE void unlock()
		{
			auto [owner, depth] = split( value.load() );
			dassert( depth && owner == get_cid() );
			--depth;
			value.store( combine( depth ? owner : 0, depth ), std::memory_order::release );
		}
		FORCE_INLINE bool locked() const
		{
			return value.load() != 0;
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
			while ( value < ( UINT16_MAX - 1 ) )
			{
				if ( counter.compare_exchange_strong( value, value + 1, std::memory_order::acquire ) )
					return true;
			}
			return false;
		}
		FORCE_INLINE void downgrade()
		{
			counter = 1;
		}
		FORCE_INLINE void unlock()
		{
			dassert( counter == UINT16_MAX );
			counter.store( 0, std::memory_order::release );
		}
		FORCE_INLINE void unlock_shared()
		{
			--counter;
		}
		FORCE_INLINE bool locked() const
		{
			return counter.load( std::memory_order::relaxed ) != 0;
		}
		FORCE_INLINE void lock()
		{
			while ( !try_lock() ) [[unlikely]]
			{
				do
					yield_cpu();
				while ( locked() );
			}
		}
		FORCE_INLINE void lock_shared()
		{
			uint16_t value = counter.load();

			__hint_unroll_n( 2 )
			while ( true )
			{
				// If not exclusively owned, try incrementing the lock count.
				//
				if ( value < ( UINT16_MAX - 1 ) ) [[likely]]
				{
					if ( counter.compare_exchange_strong( value, value + 1, std::memory_order::acquire ) )
						return;
				}
				// Otherwise, yield the CPU until we can lock again.
				//
				else
				{
					do
						yield_cpu();
					while ( ( value = counter.load() ) >= ( UINT16_MAX - 1 ) );
				}
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
	};
};