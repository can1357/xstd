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
	template<typename T>
	concept XMutex = requires{ typename T::underlying_lock; task_priority_t( T::task_priority ); };

	// Basic spinlock.
	//
	struct spinlock
	{
		std::atomic<uint16_t> value = 0;

		FORCE_INLINE bool try_lock()
		{
#if AMD64_TARGET
			return bit_set( value, 0 ) == 0;
#else
			return value.exchange( 1, std::memory_order::acquire ) == 0;
#endif
		}
		FORCE_INLINE void unlock()
		{
			dassert( value );
			value.store( 0, std::memory_order::release );
		}
		FORCE_INLINE bool locked() const
		{
#if AMD64_TARGET
			return bit_test( value, 0 );
#else
			return value.load( std::memory_order::relaxed ) != 0;
#endif
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

	// - Task priority wrapper.
	//
	template<task_priority_t Tpr>
	struct xspinlock : protected spinlock {
		using underlying_lock = spinlock;
		static constexpr task_priority_t task_priority = Tpr;

		FORCE_INLINE underlying_lock& unwrap() { return *this; }
		FORCE_INLINE const underlying_lock& unwrap() const { return *this; }

		FORCE_INLINE bool try_lock( task_priority_t prev ) {
			set_task_priority( Tpr );
			if ( underlying_lock::try_lock() ) return true;
			set_task_priority( prev );
			return false;
		}
		FORCE_INLINE void unlock( task_priority_t prev ) {
			underlying_lock::unlock();
			set_task_priority( prev );
		}
		FORCE_INLINE bool locked() const {
			return underlying_lock::locked();
		}

		FORCE_INLINE void lock( task_priority_t prev ) {
			while ( true ) {
				set_task_priority( Tpr );
				if ( underlying_lock::try_lock() ) [[likely]]
					return;
				set_task_priority( prev );

				do
					yield_cpu();
				while ( underlying_lock::locked() );
			}
		}
	};

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
		FORCE_INLINE bool locked_unique() const
		{
			return counter.load( std::memory_order::relaxed ) == UINT16_MAX;
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

	// - Task priority wrapper.
	//
	template<task_priority_t Tpr>
	struct shared_xspinlock : protected shared_spinlock {
		using underlying_lock = shared_spinlock;
		static constexpr task_priority_t task_priority = Tpr;

		FORCE_INLINE underlying_lock& unwrap() { return *this; }
		FORCE_INLINE const underlying_lock& unwrap() const { return *this; }

		FORCE_INLINE bool try_lock( task_priority_t prev ) {
			set_task_priority( Tpr );
			if ( underlying_lock::try_lock() ) return true;
			set_task_priority( prev );
			return false;
		}
		FORCE_INLINE bool try_lock_shared( task_priority_t prev ) {
			set_task_priority( Tpr );
			if ( underlying_lock::try_lock_shared() ) return true;
			set_task_priority( prev );
			return false;
		}

		FORCE_INLINE void unlock( task_priority_t prev ) {
			underlying_lock::unlock();
			set_task_priority( prev );
		}
		FORCE_INLINE void unlock_shared( task_priority_t prev ) {
			underlying_lock::unlock_shared();
			set_task_priority( prev );
		}

		FORCE_INLINE bool locked() const {
			return underlying_lock::locked();
		}
		FORCE_INLINE bool locked_unique() const {
			return underlying_lock::locked_unique();
		}
		FORCE_INLINE bool try_upgrade() {
			return underlying_lock::try_upgrade();
		}

		FORCE_INLINE void lock( task_priority_t prev ) {
			while ( !try_lock( prev ) ) [[unlikely]] {
				do
					yield_cpu();
				while ( underlying_lock::locked() );
			}
		}
		FORCE_INLINE void lock_shared( task_priority_t prev ) {
			while ( true ) {
				// If acquirable:
				//
				uint16_t value = underlying_lock::counter.load( std::memory_order::relaxed );
				if ( value < ( UINT16_MAX - 1 ) ) [[likely]] {
					// Raise TPR.
					//
					set_task_priority( Tpr );
					
					// Try to acquire.
					//
					while ( true ) {
						if ( underlying_lock::counter.compare_exchange_strong( value, value + 1, std::memory_order::acquire ) ) [[likely]]
							return;
						if ( value >= ( UINT16_MAX - 1 ) ) [[unlikely]]
							break;
					}

					// If we failed, lower again, yield.
					//
					set_task_priority( prev );
				}
				yield_cpu();
			}
		}
		FORCE_INLINE void upgrade() {
			if ( !try_upgrade() ) {
				underlying_lock::unlock_shared();
				underlying_lock::lock();
			}
		}
		FORCE_INLINE void upgrade( task_priority_t prev ) {
			if ( !try_upgrade() ) {
				unlock_shared( prev );
				lock( prev );
			}
		}
		FORCE_INLINE void downgrade() {
			underlying_lock::downgrade();
		}
	};

	// Recursive spinlock.
	//
	namespace impl { inline constexpr auto get_tid = [ ] () { return std::this_thread::get_id(); }; };
	template<DefaultConstructible CidGetter = decltype( impl::get_tid )>
	struct recursive_spinlock
	{
		FORCE_INLINE static uint64_t get_cid()
		{ 
			auto cid = CidGetter{}();
			if constexpr ( sizeof( cid ) == 8 )
				return 1 + ( uint64_t ) bit_cast< uint64_t >( cid );
			else if constexpr ( sizeof( cid ) == 4 )
				return 1 + ( uint64_t ) bit_cast< uint32_t >( cid );
			else if constexpr ( sizeof( cid ) == 2 )
				return 1 + ( uint64_t ) bit_cast< uint16_t >( cid );
			else if constexpr ( sizeof( cid ) == 1 )
				return 1 + ( uint64_t ) bit_cast< uint8_t >( cid );
			else
				static_assert( sizeof( cid ) == -1, "Invalid CID type." );
		}

		// Current owner of the lock and the depth.
		//
		std::atomic<uint64_t> owner = 0;
		uint32_t depth = 0;
		
		// Manually defined constructors for template deduction.
		//
		FORCE_INLINE recursive_spinlock( CidGetter ) {}
		FORCE_INLINE recursive_spinlock() = default;

		// Implement the standard mutex interface.
		//
		FORCE_INLINE bool try_lock()
		{
			uint64_t cid = get_cid();
			uint64_t expected = 0;
			if ( !owner.compare_exchange_strong( expected, cid ) && expected != cid )
				return false;
			++depth;
			return true;
		}
		FORCE_INLINE void lock()
		{
			uint64_t this_cid = get_cid();

			// If we're not recursing:
			//
			uint64_t expected = owner.load( std::memory_order::relaxed );
			if ( expected != this_cid )
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
					if ( owner.compare_exchange_strong( expected, this_cid, std::memory_order::acquire ) )
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
				owner.store( 0, std::memory_order::release );
		}
		FORCE_INLINE bool locked() const
		{
			return owner.load( std::memory_order::relaxed ) != 0;
		}
		FORCE_INLINE uint64_t owner_cid() const
		{
			return owner.load( std::memory_order::relaxed );
		}
	};
	template<typename T>
	recursive_spinlock( T )->recursive_spinlock<T>;

	// - Task priority wrapper.
	//
	template<task_priority_t Tpr, DefaultConstructible CidGetter = decltype( impl::get_tid )>
	struct recursive_xspinlock : protected recursive_spinlock<CidGetter> {
		using underlying_lock = recursive_spinlock<CidGetter>;
		static constexpr task_priority_t task_priority = Tpr;

		FORCE_INLINE underlying_lock& unwrap() { return *this; }
		FORCE_INLINE const underlying_lock& unwrap() const { return *this; }

		FORCE_INLINE static uint64_t get_cid() {
			return underlying_lock::get_cid();
		}

		// Manually defined constructors for template deduction.
		//
		FORCE_INLINE recursive_xspinlock( CidGetter ) {}
		FORCE_INLINE recursive_xspinlock() = default;

		FORCE_INLINE bool locked() const {
			return underlying_lock::locked();
		}
		FORCE_INLINE uint64_t owner_cid() const {
			return underlying_lock::owner_cid();
		}

		FORCE_INLINE bool try_lock( task_priority_t prev ) {
			set_task_priority( Tpr );
			if ( underlying_lock::try_lock() ) return true;
			set_task_priority( prev );
			return false;
		}
		FORCE_INLINE void unlock( task_priority_t prev ) {
			// Decrement depth, if we are finally releasing, clear owner, reset TPR.
			//
			if ( !--underlying_lock::depth ) {
				underlying_lock::owner.store( 0, std::memory_order::release );
				set_task_priority( prev );
			}
		}
		FORCE_INLINE void lock( task_priority_t prev ) {
			uint64_t this_cid = get_cid();

			// If we're not recursing:
			//
			uint64_t expected = owner_cid();
			if ( expected != this_cid ) {
				while ( true ) {
					// Wait until there's no owner.
					//
					while ( expected != 0 ) {
						yield_cpu();
						expected = underlying_lock::owner.load( std::memory_order::relaxed );
					}

					// If we could acquire, break.
					//
					set_task_priority( Tpr );
					if ( underlying_lock::owner.compare_exchange_strong( expected, this_cid, std::memory_order::acquire ) )
						break;
					set_task_priority( prev );
				}
			}

			// Increment depth and return.
			//
			++depth;
		}
	};

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

// Override STD guards.
//
namespace std {
	template<xstd::XMutex Mutex>
	class shared_lock<Mutex> {
		Mutex* pmtx     = nullptr;
		bool owns            = false;
		task_priority_t prev = 0;

		public:
		using mutex_type = Mutex;
		shared_lock() noexcept = default;
		shared_lock( const shared_lock& ) = delete;
		shared_lock& operator=( const shared_lock& ) = delete;

		explicit shared_lock( mutex_type& mtx ) : pmtx( &mtx ) { lock(); }
		explicit shared_lock( mutex_type& mtx, defer_lock_t ) : pmtx( &mtx ) {}
		explicit shared_lock( mutex_type& mtx, try_to_lock_t ) : pmtx( &mtx ), owns( mtx.try_lock_shared( prev ) ) {}
		explicit shared_lock( mutex_type& mtx, task_priority_t tpr, adopt_lock_t ) : pmtx( &mtx ), owns( true ), prev( tpr ) {}

		~shared_lock() noexcept {
			if ( owns ) {
				pmtx->unlock_shared( prev );
			}
		}
		shared_lock( shared_lock&& o ) noexcept : pmtx( o.pmtx ), owns( o.owns ), prev( o.prev ) {
			o.pmtx = nullptr;
			o.owns = false;
		}
		shared_lock& operator=( shared_lock&& o ) noexcept {
			if ( owns ) {
				pmtx->unlock_shared( prev );
			}
			pmtx = o.pmtx;
			owns = o.owns;
			prev = o.prev;
			o.pmtx = nullptr;
			o.owns = nullptr;
			return *this;
		}

		void lock() {
			prev = get_task_priority();
			dassert( this->pmtx && !this->owns );
			dassert( this->prev <= Mutex::task_priority );
			pmtx->lock_shared( prev );
			owns = true;
		}
		bool try_lock() {
			prev = get_task_priority();
			dassert( this->pmtx && !this->owns );
			dassert( this->prev <= Mutex::task_priority );
			owns = pmtx->try_lock_shared( prev );
			return owns;
		}
		void unlock() {
			dassert( this->pmtx && this->owns );
			pmtx->unlock_shared( prev );
			owns = false;
		}

		void swap( shared_lock& o ) noexcept {
			std::swap( o.pmtx, pmtx );
			std::swap( o.owns, owns );
			std::swap( o.prev, prev );
		}
		bool owns_lock() const noexcept {
			return owns;
		}
		explicit operator bool() const noexcept {
			return owns;
		}
		mutex_type* mutex() const noexcept {
			return pmtx;
		}
		task_priority_t priority() const noexcept {
			return prev;
		}
	};

	template<xstd::XMutex Mutex>
	class unique_lock<Mutex> {
		Mutex* pmtx = nullptr;
		bool owns = false;
		task_priority_t prev = 0;

		public:
		using mutex_type = Mutex;
		unique_lock() noexcept = default;
		unique_lock( const unique_lock& ) = delete;
		unique_lock& operator=( const unique_lock& ) = delete;

		explicit unique_lock( mutex_type& mtx ) : pmtx( &mtx ) { lock(); }
		explicit unique_lock( mutex_type& mtx, defer_lock_t ) : pmtx( &mtx ) {}
		explicit unique_lock( mutex_type& mtx, try_to_lock_t ) : pmtx( &mtx ), owns( mtx.try_lock( prev ) ) {}
		explicit unique_lock( mutex_type& mtx, task_priority_t tpr, adopt_lock_t ) : pmtx( &mtx ), owns( true ), prev( tpr ) {}

		~unique_lock() noexcept {
			if ( owns ) {
				pmtx->unlock( prev );
			}
		}
		unique_lock( unique_lock&& o ) noexcept : pmtx( o.pmtx ), owns( o.owns ), prev( o.prev ) {
			o.pmtx = nullptr;
			o.owns = false;
		}
		unique_lock& operator=( unique_lock&& o ) noexcept {
			if ( owns ) {
				pmtx->unlock( prev );
			}
			pmtx = o.pmtx;
			owns = o.owns;
			prev = o.prev;
			o.pmtx = nullptr;
			o.owns = nullptr;
			return *this;
		}

		void lock() {
			prev = get_task_priority();
			dassert( this->pmtx && !this->owns );
			dassert( this->prev <= Mutex::task_priority );
			pmtx->lock( prev );
			owns = true;
		}
		bool try_lock() {
			prev = get_task_priority();
			dassert( this->pmtx && !this->owns );
			dassert( this->prev <= Mutex::task_priority );
			owns = pmtx->try_lock( prev );
			return owns;
		}
		void unlock() {
			dassert( this->pmtx && this->owns );
			pmtx->unlock( prev );
			owns = false;
		}

		void swap( unique_lock& o ) noexcept {
			std::swap( o.pmtx, pmtx );
			std::swap( o.owns, owns );
			std::swap( o.prev, prev );
		}
		bool owns_lock() const noexcept {
			return owns;
		}
		explicit operator bool() const noexcept {
			return owns;
		}
		mutex_type* mutex() const noexcept {
			return pmtx;
		}
		task_priority_t priority() const noexcept {
			return prev;
		}
	};
	
	template<xstd::XMutex Mutex>
	class lock_guard<Mutex> { // class with destructor that unlocks a mutex
		Mutex& mtx;
		const task_priority_t prev = get_task_priority();
	public:
		 using mutex_type = Mutex;

		 explicit lock_guard( Mutex& mtx ) : mtx( mtx ) { 
			 dassert( this->prev <= Mutex::task_priority ); 
			 mtx.lock( prev ); 
		 }
		 lock_guard( Mutex& _Mtx, task_priority_t tpr, adopt_lock_t ) noexcept : mtx( mtx ), prev( tpr ) {}
		 ~lock_guard() noexcept { mtx.unlock( prev ); }

		 task_priority_t priority() const noexcept { return prev; }
	};
};