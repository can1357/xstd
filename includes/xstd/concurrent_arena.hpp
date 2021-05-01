#pragma once
#include <atomic>
#include <memory>
#include <shared_mutex>
#include "spinlock.hpp"
#include "intrinsics.hpp"

namespace xstd
{
	template<TriviallyMoveConstructable T = uint8_t> requires TriviallyDestructable<T>
	struct concurrent_arena
	{
		// Brief container traits.
		//
		using value_type =       T;
		using reference =        T&;
		using pointer =          T*;
		using iterator =         T*;
		using const_iterator =   const T*;

		// Lock protecting from destructive operations.
		//
		mutable shared_spinlock lock = {};

		// Raw space allocated for the entries and the current counter.
		//
		T* space = nullptr;
		std::atomic<size_t> counter = 0;
		size_t limit = 0;

		// Constructed by size, no copy.
		//
		inline concurrent_arena( size_t limit = 0 ) { resize( limit ); }

		// Implement copy, note that this is not a fast operation.
		//
		concurrent_arena( const concurrent_arena& o )
		{
			std::shared_lock _g{ o.lock };
			assign( o.begin(), o.end() );
		}
		concurrent_arena& operator=( const concurrent_arena& o )
		{
			std::unique_lock _g{ lock };
			std::shared_lock _g2{ o.lock };
			assign( o.begin(), o.end(), true );
			return *this;
		}

		// Implement move via swap.
		//
		inline concurrent_arena( concurrent_arena&& other ) { swap( other ); }
		inline concurrent_arena& operator=( concurrent_arena&& other ) { swap( other ); return *this; }
		inline void swap( concurrent_arena& other )
		{
			std::unique_lock _g{ lock };
			std::unique_lock _g2{ other.lock };
			std::swap( space, other.space );
			std::swap( limit, other.limit );
			other.counter = counter.exchange( other.counter.load() );
		}

		// Observers.
		// - Note: Thread safety left to the caller!
		//
		T* begin() { return space; }
		const T* begin() const { return space; }
		T* end() { return space + counter; }
		const T* end() const { return space + counter; }
		size_t size() const { return counter; }
		bool empty() const { return size() == 0; }

		// Implement resize, unlike STL resize it mainly acts as a "reserve".
		//
		inline void resize( size_t new_limit, bool holds_lock = false )
		{
			std::unique_lock u{ lock, std::defer_lock_t{} };
			if ( !holds_lock ) u.lock();
			
			// If clearing:
			//
			if ( !new_limit )
			{
				// Swap out the buffer, set limit to zero.
				//
				T* pspace = std::exchange( space, nullptr );
				counter.fetch_and( 0 );
				limit = 0;
				
				// Unlock, destroy all instances and free the leftover buffer.
				//
				u.unlock();
				if ( pspace )
					free( pspace );
			}
			// If resizing:
			//
			else
			{
				// If new limit is less than the amount of elements we already have, destroy the leftovers.
				//
				if ( new_limit < counter )
					counter = new_limit;

				// Realloc the buffer and change the limit.
				//
				space = ( T* ) realloc( space, new_limit * sizeof( T ) );
				limit = new_limit;
			}
		}

		// Clears the arena by destroying all pushed elements. Buffer remains the same.
		//
		inline void clear( bool holds_lock = false ) 
		{
			std::unique_lock u{ lock, std::defer_lock_t{} };
			if ( !holds_lock ) u.lock();
			counter.fetch_and( 0 );
		}

		// Shrinks the buffer to match the size of available elements.
		//
		inline void shrink_to_fit( bool holds_lock = false )
		{
			std::unique_lock u{ lock, std::defer_lock_t{} };
			if ( !holds_lock ) u.lock();

			space = realloc( space, sizeof( T ) * counter );
			limit = counter;
		}

		// Slot allocator, returns uninitialized memory.
		//
		inline T* allocate_slot( size_t c = 1, bool holds_slock = false )
		{
			std::shared_lock u{ lock, std::defer_lock_t{} };
			if ( !holds_slock ) u.lock();

			// Allocate a slot.
			//
			size_t n = counter.load();
			while ( true )
			{
				if ( ( n + c ) > limit )
					return nullptr;
				if ( counter.compare_exchange_strong( n, n + 1 ) )
					break;
			}
			if ( !space )
				return nullptr;
			return space + n;
		}

		// Atomic push variants, returns nullptr if the arena is full.
		//
		template<typename... Tx>
		inline T* emplace( Tx&&... value )
		{
			std::shared_lock _g{ lock };
			T* buffer = allocate_slot( 1, true );
			if ( !buffer )
				return nullptr;
			return new ( buffer ) T( std::forward<Tx>( value )... );
		}
		inline T* push( const T& value ) { return emplace( value ); }

		// Range assignment / insertation.
		//
		template<typename It1, typename It2>
		inline void assign( It1&& first, It2&& last, bool holds_lock = false )
		{
			// Acquire a unique lock and clear & resize the buffer if necesssary.
			//
			if ( !holds_lock ) lock.lock();
			clear( true );
			if ( ( last - first ) > limit )
				resize( last - first, true );

			// Downgrade to a shared lock, that's all we need for space consistency.
			//
			counter = last - first;
			lock.downgrade();
			std::uninitialized_copy( std::forward<It1>( first ), std::forward<It2>( last ), space );
			if ( holds_lock )
				lock.upgrade();
			else
				lock.unlock_shared();
		}
		template<typename It1, typename It2>
		inline T* insert( It1&& first, It2&& last, bool holds_slock = false )
		{
			std::shared_lock u{ lock, std::defer_lock_t{} };
			if ( !holds_slock ) u.lock();

			// Allocate slots, report failure if it fails.
			//
			T* ptr = allocate_slot( last - first, true );
			if ( !ptr )
				return nullptr;

			// Copy the range and return.
			//
			std::uninitialized_copy( std::forward<It1>( first ), std::forward<It2>( last ), ptr );
			return ptr;
		}
		template<Iterable C>
		inline void assign( C&& container, bool holds_lock = false )
		{
			return assign( std::begin( container ), std::end( container ), holds_lock );
		}
		template<Iterable C>
		inline T* insert( C&& container, bool holds_slock = false )
		{
			return insert( std::begin( container ), std::end( container ), holds_slock );
		}

		// Pops all elements and returns a shared_ptr with the array and the item count.
		//
		inline std::pair<std::unique_ptr<T[]>, size_t> pop_all( bool preserve_size = false )
		{
			std::unique_lock u{ lock };
			size_t pcounter = counter.exchange( 0 );
			if ( !pcounter )
				return { nullptr,  0 };
			
			T* pspace = nullptr;
			if ( preserve_size )
				pspace = ( T* ) malloc( sizeof( T ) * limit );
			else
				limit = 0;
			std::swap( pspace, space );
			return std::pair{ std::unique_ptr<T[]>{ pspace }, pcounter };
		}

		// Simple destructor freeing the owned space.
		//
		~concurrent_arena()
		{
			if ( space )
				delete[] space;
		}
	};
};