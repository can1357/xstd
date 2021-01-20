#pragma once
#include <mutex>
#include <shared_mutex>

namespace xstd
{
	// This type behaves exactly the same as the original lock type, with the 
	// exception of constructing it empty if the condition bool is false.
	//
	template<typename lock_type, typename mutex_type>
	struct cnd_variant_lock : lock_type
	{
		// Default constructors
		//
		cnd_variant_lock() {}
		cnd_variant_lock( mutex_type& mtx ) : lock_type{ mtx } {}

		// Construct from mutex and condition.
		//
		cnd_variant_lock( mutex_type& mtx, bool condition )
		{
			// If condition is passed:
			//
			if ( condition )
			{
				// Create a secondary lock with the mutex being actually 
				// acquired and swap states with the current empty instance.
				//
				lock_type lock{ mtx };
				this->swap( lock );
			}
		}
	};

	// Declare shortcuts for unique_lock, shared_lock and lock_guard and their deduction guides.
	//
	template<typename T>
	struct cnd_unique_lock : cnd_variant_lock<std::unique_lock<T>, T> { using cnd_variant_lock<std::unique_lock<T>, T>::cnd_variant_lock; };
	template<typename T> 
	cnd_unique_lock( T&, bool )->cnd_unique_lock<T>;

	template<typename T>
	struct cnd_shared_lock : cnd_variant_lock<std::shared_lock<T>, T> { using cnd_variant_lock<std::shared_lock<T>, T>::cnd_variant_lock; };
	template<typename T> 
	cnd_shared_lock( T&, bool )->cnd_shared_lock<T>;

	template<typename T>
	struct cnd_lock_guard  : cnd_variant_lock<std::lock_guard<T>,  T> { using cnd_variant_lock<std::lock_guard<T>,  T>::cnd_variant_lock; };
	template<typename T> 
	cnd_lock_guard( T&, bool )->cnd_lock_guard<T>;
};