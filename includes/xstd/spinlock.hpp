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
	// Basic spinlock.
	//
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

	// Shared spinlock.
	//
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
			uint16_t value = counter.load( std::memory_order::relaxed );
			while ( value < ( UINT16_MAX - 1 ) )
			{
				if ( counter.compare_exchange_strong( value, value + 1, std::memory_order::acquire ) )
					return true;
			}
			return false;
		}
		FORCE_INLINE void downgrade()
		{
			counter.store( 1, std::memory_order::release );
		}
		FORCE_INLINE void unlock()
		{
			dassert( counter.load( std::memory_order::relaxed ) == UINT16_MAX );
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
			while ( true )
			{
				// Yield the CPU until the exclusive lock is gone.
				//
				uint16_t value;
				while ( ( value = counter.load( std::memory_order::relaxed ) ) >= ( UINT16_MAX - 1 ) ) [[unlikely]]
					yield_cpu();

				// Try incrementing share count.
				//
				if ( counter.compare_exchange_strong( value, value + 1, std::memory_order::acquire ) )
					return;
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

	// Recursive spinlock.
	//
	namespace impl { inline constexpr auto get_tid = [ ] () { return std::this_thread::get_id(); }; };
	template<DefaultConstructible CidGetter = decltype( impl::get_tid )>
	struct recursive_spinlock
	{
		FORCE_INLINE static uint32_t get_cid() 
		{ 
			auto cid = CidGetter{}();
			if constexpr ( sizeof( cid ) == 4 )
				return 1 + ( uint32_t ) bit_cast<uint32_t>( cid );
			else if constexpr ( sizeof( cid ) == 2 )
				return 1 + ( uint32_t ) bit_cast<uint16_t>( cid );
			else if constexpr ( sizeof( cid ) == 1 )
				return 1 + ( uint32_t ) bit_cast<uint8_t>( cid );
			else
				return 1 + *( const uint32_t* ) &cid;
		}

		// Current owner of the lock and the depth.
		//
		std::atomic<uint32_t> owner = 0;
		uint32_t depth = 0;
		
		// Dummy constructor for template deduction.
		//
		FORCE_INLINE recursive_spinlock( CidGetter ) {}

		// Default constructor.
		//
		FORCE_INLINE recursive_spinlock() {}

		// Implement the standard mutex interface.
		//
		FORCE_INLINE bool try_lock()
		{
			uint32_t cid = get_cid();
			uint32_t expected = 0;
			if ( !owner.compare_exchange_strong( expected, cid ) && expected != cid )
				return false;
			++depth;
			return true;
		}
		FORCE_INLINE void lock()
		{
			uint32_t cid = get_cid();

			// If we're not recursing:
			//
			uint32_t expected = owner.load( std::memory_order::relaxed );
			if ( expected != cid )
			{
				while ( true )
				{
					// Wait until there's no owner.
					//
					while ( expected != 0 )
					{
						yield_cpu();
						expected = owner.load( std::memory_order::relaxed );
					}

					// If we could acquire, break.
					//
					if ( owner.compare_exchange_strong( expected, cid, std::memory_order::acquire ) )
						break;
				}
			}

			// Increment depth and return.
			//
			++depth;
		}
		FORCE_INLINE void unlock()
		{
			// Decrement depth, if we are finally releasing, clear owner.
			//
			if ( !--depth )
				owner.store( 0, std::memory_order::acquire );
		}
		FORCE_INLINE bool locked() const
		{
			return owner.load( std::memory_order::relaxed ) != 0;
		}
	};
	template<typename T>
	recursive_spinlock( T )->recursive_spinlock<T>;

	// Upgrade guard.
	//
	template<typename T>
	struct upgrade_guard
	{
		T* pmutex;
		bool owns;
		
		// All constructors in STL style and the destructor releasing the lock where relevant.
		//
		upgrade_guard()                             : pmutex( nullptr ), owns( false ) {}
		upgrade_guard( T& ref )                     : pmutex( &ref ),    owns( false ) { lock(); }
		upgrade_guard( T& ref, std::adopt_lock_t )  : pmutex( &ref ),    owns( true )  {}
		upgrade_guard( T& ref, std::defer_lock_t )  : pmutex( &ref ),    owns( false ) {}
		upgrade_guard( T& ref, std::try_to_lock_t ) : pmutex( &ref ),    owns( false ) { try_lock(); }
		~upgrade_guard() { if ( owns ) unlock(); }

		// No copy.
		//
		upgrade_guard( const upgrade_guard& ) = delete;
		upgrade_guard& operator=( const upgrade_guard& ) = delete;

		// Move by exchange.
		//
		upgrade_guard( upgrade_guard&& o ) noexcept : pmutex( std::exchange( o.pmutex, nullptr ) ), owns( std::exchange( o.owns, false ) ) {}
		upgrade_guard& operator=( upgrade_guard&& o )
		{
			unlock();
			pmutex = std::exchange( o.pmutex, nullptr );
			owns = std::exchange( o.owns, false );
			return *this;
		}

		// STL interface.
		//
		void lock()
		{
			dassert( !owns && pmutex );
			pmutex->upgrade();
			owns = true;
		}
		bool try_lock()
		{
			dassert( !owns && pmutex );
			owns = pmutex->try_upgrade();
			return owns;
		}
		void unlock()
		{
			dassert( owns && pmutex );
			pmutex->downgrade();
			owns = false;
		}
		void swap( upgrade_guard& o ) noexcept
		{
			std::swap( pmutex, o.pmutex );
			std::swap( owns, o.owns );
		}
		T* release() noexcept
		{
			owns = false;
			return std::exchange( pmutex, nullptr );
		}
		bool owns_lock() const noexcept
		{
			return owns;
		}
		explicit operator bool() const noexcept
		{
			return owns;
		}
		T* mutex() const noexcept
		{
			return pmutex;
		}
	};
};