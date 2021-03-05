#pragma once
#include <mutex>
#include <shared_mutex>
#include "type_helpers.hpp"
#include "assert.hpp"

namespace xstd
{
	// Pairs a value with a mutex.
	//
	template<typename T, Lockable L>
	struct guarded
	{
		T value;
		mutable L mtx = {};

		// Constructor wraps around the value.
		//
		template<typename... Tx>
		inline constexpr guarded( Tx&&... args ) : value( std::forward<Tx>( args )... ) {}

		// No move/copy allowed.
		//
		guarded( guarded&& ) noexcept = delete;
		guarded( const guarded& ) = delete;
		guarded& operator=( guarded&& ) noexcept = delete;
		guarded& operator=( const guarded& ) = delete;

		// Implement a mutex so it can be usable with types like std::unique_lock as well.
		//
		inline void lock() const { mtx.lock(); }
		inline void unlock() const { mtx.unlock(); }
		inline void lock_shared() const
		{
			if constexpr ( SharedLockable<L> )
				mtx.lock_shared();
			else
				mtx.lock();
		}
		inline void unlock_shared() const
		{
			if constexpr ( SharedLockable<L> )
				mtx.unlock_shared();
			else
				mtx.unlock();
		}
	};

	// Locks for accessing guarded types.
	//
	template<typename T, Lockable L>
	struct unique_guard
	{
		T* value;
		L* mtx;
		bool locked;

		// Constructed by a pointer to value, pointer to lockable and optionally adopt/defer/try.
		//
		inline unique_guard( T* value, L* mtx ) : value( value ), mtx( mtx ), locked( false ) { lock(); }
		inline unique_guard( T* value, L* mtx, std::adopt_lock_t ) : value( value ), mtx( mtx ), locked( true ) {}
		inline unique_guard( T* value, L* mtx, std::defer_lock_t ) : value( value ), mtx( mtx ), locked( false ) {}
		
		// Constructed by a reference to a guarded value and the same optionals.
		//
		inline unique_guard( guarded<T, L>& guarded ) : unique_guard( &guarded.value, &guarded.mtx ) {}
		inline unique_guard( guarded<T, L>& guarded, std::adopt_lock_t opt ) : unique_guard( &guarded.value, &guarded.mtx, opt ) {}
		inline unique_guard( guarded<T, L>& guarded, std::defer_lock_t opt ) : unique_guard( &guarded.value, &guarded.mtx, opt ) {}

		// No move/copy allowed.
		//
		unique_guard( unique_guard&& ) noexcept = delete;
		unique_guard( const unique_guard& ) = delete;
		unique_guard& operator=( unique_guard&& ) noexcept = delete;
		unique_guard& operator=( const unique_guard& ) = delete;

		// Accessors to the guarded value, returns nullptr if not locked.
		//
		inline T* operator->() const { return locked ? value : nullptr; }
		inline T& operator*() const { return locked ? value : nullptr; }

		// Wrappers around lock state.
		//
		inline void unlock() { dassert( locked ); mtx->unlock(); locked = false; }
		inline void lock() { dassert( !locked ); mtx->lock(); locked = true; }
		inline void reset() { if ( locked ) unlock(); }
		inline ~unique_guard() { reset(); }
	};
	template<typename T, SharedLockable L>
	struct shared_guard
	{
		const T* value;
		L* mtx;
		bool locked;

		// Constructed by a pointer to value, pointer to lockable and optionally adopt/defer/try.
		//
		inline shared_guard( const T* value, L* mtx ) : value( value ), mtx( mtx ), locked( false ) { lock(); }
		inline shared_guard( const T* value, L* mtx, std::adopt_lock_t ) : value( value ), mtx( mtx ), locked( true ) {}
		inline shared_guard( const T* value, L* mtx, std::defer_lock_t ) : value( value ), mtx( mtx ), locked( false ) {}

		// Constructed by a reference to a guarded value and the same optionals.
		//
		inline shared_guard( const guarded<T, L>& guarded ) : shared_guard( &guarded.value, &guarded.mtx ) {}
		inline shared_guard( const guarded<T, L>& guarded, std::adopt_lock_t opt ) : shared_guard( &guarded.value, &guarded.mtx, opt ) {}
		inline shared_guard( const guarded<T, L>& guarded, std::defer_lock_t opt ) : shared_guard( &guarded.value, &guarded.mtx, opt ) {}

		// No move/copy allowed.
		//
		shared_guard( shared_guard&& ) noexcept = delete;
		shared_guard( const shared_guard& ) = delete;
		shared_guard& operator=( shared_guard&& ) noexcept = delete;
		shared_guard& operator=( const shared_guard& ) = delete;

		// Accessors to the guarded value, returns nullptr if not locked.
		//
		inline const T* operator->() const { return locked ? value : nullptr; }
		inline const T& operator*() const { return locked ? value : nullptr; }

		// Wrappers around lock state.
		//
		inline void unlock() { dassert( locked ); mtx->unlock_shared(); locked = false; }
		inline void lock() { dassert( !locked ); mtx->lock_shared(); locked = true; }
		inline void reset() { if ( locked ) unlock(); }
		inline ~shared_guard() { reset(); }
	};

	// Declare the deduction guides.
	//
	template<typename T, typename L> unique_guard( guarded<T, L>& )->unique_guard<T, L>;
	template<typename T, typename L> unique_guard( T*, L*, ... )->unique_guard<T, L>;
	template<typename T, typename L> shared_guard( const guarded<T, L>& )->shared_guard<T, L>;
	template<typename T, typename L> shared_guard( const T*, L*, ... )->shared_guard<T, L>;
};