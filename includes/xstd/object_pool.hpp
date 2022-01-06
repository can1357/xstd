#pragma once
#include "type_helpers.hpp"
#include "spinlock.hpp"
#include "plf_colony.hpp"
#include "assert.hpp"
#include <bit>

namespace xstd
{
	// Implements an object pool that can be used to allocate fixed-size elements in a fast manner.
	//
	template<size_t Length, size_t Align, bool ThreadSafe = false>
	struct basic_object_pool
	{
		// Traits.
		//
		using element_type = std::aligned_storage_t<Length, Align>;
		static constexpr size_t alloc_size =    Length;
		static constexpr size_t alloc_align =   Align;
		static constexpr bool is_thread_safe =  ThreadSafe;
		
		// Colony used as an allocator, lock and the allocation counter.
		//
		plf::colony<element_type> colony;
		spinlock                  lock;
		size_t                    num_allocations;

		// Basic interface.
		//
		void* allocate()
		{
			if constexpr ( ThreadSafe ) lock.lock();
			void* result = &*colony.emplace();
			num_allocations++;
			if constexpr ( ThreadSafe ) lock.unlock();
			return result;
		}
		void deallocate( void* pointer )
		{
			if constexpr ( ThreadSafe ) lock.lock();
			colony.erase( colony.get_iterator( ( element_type* ) pointer ) );
			num_allocations--;
			if constexpr ( ThreadSafe ) lock.unlock();
		}

		// Typed interface.
		//
		template<NonVoid T, typename... Tx>
		T* emplace( Tx&&... args )
		{
			static_assert( sizeof( T ) <= alloc_size && alignof( T ) <= alloc_align, "Invalid allocation." );
			return new ( allocate() ) T( std::forward<Tx>( args )... );
		}
		template<NonVoid T>
		void erase( T* pointer )
		{
			static_assert( sizeof( T ) <= alloc_size && alignof( T ) <= alloc_align, "Invalid allocation." );
			
			std::destroy_at( pointer );
			
			deallocate( pointer );
		}
	};
	template<typename T> using object_pool =            basic_object_pool<sizeof( T ), alignof( T ), false>;
	template<typename T> using threadsafe_object_pool = basic_object_pool<sizeof( T ), alignof( T ), true>;

	// Implement an allocator type that can use a given object pool.
	//
	template<typename T, typename Pool>
	struct pool_allocator
	{
		static_assert( sizeof( T ) <= Pool::alloc_size && alignof( T ) <= Pool::alloc_align, "Invalid pool allocator." );

		// Allocator traits.
		//
		using value_type =         T;
		using pointer =            T*;
		using const_pointer =      const T*;
		using void_pointer =       void*;
		using const_void_pointer = const void*;
		using size_type =          size_t;
		using difference_type =    int64_t;
		using is_always_equal =    std::false_type;

		template<typename U>
		struct rebind { using other = pool_allocator<T, Pool>; };

		// Reference to the pool.
		//
		Pool& pool;

		// Construct from a pool reference, implement copy and comparison.
		//
		constexpr pool_allocator( Pool& pool ) : pool( pool ) {}
		template <typename T2>
		constexpr pool_allocator( const pool_allocator<T2, Pool>& o ) : pool( o.pool ) {}
		template<typename T2>
		constexpr bool operator==( const pool_allocator<T2, Pool>& o ) const { return &pool == &o.pool; }

		// Allocation routine.
		//
		T* allocate( size_t n, void* hint = 0 )
		{
			dassert( n == 1 );
			return ( T* ) pool.allocate();
		}

		// Deallocation routine.
		//
		void deallocate( T* ptr, size_t n )
		{
			dassert( n == 1 );
			return pool.deallocate( ptr );
		}
	};

	// Implement the default object pool.
	//
	namespace impl
	{
		template<size_t Length, size_t Align>
		inline basic_object_pool<Length, Align, true> global_pool_instance;
	};
	template<typename T>
	inline constexpr auto& default_pool = impl::global_pool_instance<
		std::bit_ceil<size_t>( std::max<size_t>( sizeof( T ), 32 ) ),
		std::bit_ceil<size_t>( std::max<size_t>( alignof( T ), alignof( void* ) ) )
	>;
	template<typename T, auto& Pool = &default_pool<T>>
	struct default_pool_allocator
	{
		static_assert( sizeof( T ) <= decltype( Pool )::alloc_size && 
							alignof( T ) <= decltype( Pool )::alloc_align, "Invalid pool allocator." );

		// Allocator traits.
		//
		using value_type =         T;
		using pointer =            T*;
		using const_pointer =      const T*;
		using void_pointer =       void*;
		using const_void_pointer = const void*;
		using size_type =          size_t;
		using difference_type =    int64_t;
		using is_always_equal =    std::true_type;

		template<typename U>
		struct rebind 
		{ 
			using other = default_pool_allocator<U, Pool>;
		};

		// Default constructed.
		//
		constexpr default_pool_allocator() {};
		template <typename T2> constexpr default_pool_allocator( const default_pool_allocator<T2, Pool>& ) {}
		template<typename T2>  constexpr bool operator==( const default_pool_allocator<T2, Pool>& o ) const { return true; }

		// Allocation routine.
		//
		T* allocate( size_t n, void* hint = 0 )
		{
			dassert( n == 1 );
			return ( T* ) Pool.allocate();
		}

		// Deallocation routine.
		//
		void deallocate( T* ptr, size_t n )
		{
			dassert( n == 1 );
			return Pool.deallocate( ptr );
		}
	};

	// Unique-ptr interface for pool allocators.
	//
	struct default_pool_deleter
	{
		template<typename T> void operator()( T* pointer ) const { default_pool<T>.erase( pointer ); }
	};
	template<typename Pool>
	struct pool_deleter
	{
		Pool& ref;
		template<typename T> void operator()( T* pointer ) const { ref.erase( pointer ); }
	};
	template<typename T, typename... Tx>
	inline std::unique_ptr<T, default_pool_deleter> make_unique_from_pool( Tx&&... args )
	{
		return { default_pool<T>.template emplace<T>( std::forward<Tx>( args )... ), default_pool_deleter{} };
	}
	template<typename T, typename... Tx>
	inline std::unique_ptr<T, pool_deleter<object_pool<T>>> make_unique_from_pool( object_pool<T>& pool, Tx&&... args )
	{
		return { pool.emplace<T>( std::forward<Tx>( args )... ), pool_deleter<object_pool<T>>{ pool } };
	}
	template<typename T, typename... Tx>
	inline std::unique_ptr<T, default_pool_deleter> make_unique_from_pool_for_overwrite( Tx&&... args )
	{
		return { ( T* ) default_pool<T>.allocate(), default_pool_deleter{} };
	}
	template<typename T, typename... Tx>
	inline std::unique_ptr<T, pool_deleter<object_pool<T>>> make_unique_from_pool_for_overwrite( object_pool<T>& pool, Tx&&... args )
	{
		return { ( T* ) pool.allocate(), pool_deleter<object_pool<T>>{ pool } };
	}
};