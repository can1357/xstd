#pragma once
#include <memory>
#include <functional>
#include <type_traits>
#include <atomic>
#include "assert.hpp"
#include "intrinsics.hpp"
#include "formatting.hpp"
#include "type_helpers.hpp"

namespace xstd
{
	// Implements fast and atomic shared objects with with no type erasure, also allows manual 
	// management of reference counters for more complex scenarios.
	//
	template<NonVoid T>
	struct shared
	{
		// Store that prefixes the type with reference counting headers.
		//
		struct store_type
		{
			using allocator = std::allocator<store_type>;

			std::atomic<uint32_t> strong_ref_count = 1;
			std::atomic<uint32_t> weak_ref_count =   0;
			T                     value;

			// Constructor redirects to type.
			//
			template<typename... Tx> 
			inline constexpr store_type( Tx&&... args ) : value( std::forward<Tx>( args )... ) {}
			
			// No copy/move.
			//
			store_type( store_type&& ) = delete;
			store_type( const store_type& ) = delete;
			store_type& operator=( store_type&& ) = delete;
			store_type& operator=( const store_type& ) = delete;

			// Reference management:
			//
			__forceinline bool inc_ref()
			{
				if ( strong_ref_count++ != 0 )
					return true;
				--strong_ref_count;
				return false;
			}
			__forceinline bool dec_ref()
			{
				if ( !--strong_ref_count )
				{
					std::destroy_at( &value );
					if ( !weak_ref_count )
					{
						allocator{}.deallocate( this, 1 );
						return true;
					}
				}
				return false;
			}
			__forceinline void inc_weak_ref()
			{
				++weak_ref_count;
			}
			__forceinline bool dec_weak_ref()
			{
				if ( !--weak_ref_count && !strong_ref_count )
				{
					allocator{}.deallocate( this, 1 );
					return true;
				}
				return false;
			}
		};
		using value_type = T;
		using allocator =  typename store_type::allocator;

		// Store the pointer.
		//
		store_type* entry;

		// Null construction.
		//
		inline constexpr shared() : entry( nullptr ) {}
		inline constexpr shared( std::nullptr_t ) : shared() {}
		inline constexpr shared( std::nullopt_t ) : shared() {}

		// Explicit construction by entry pointer, adopts the reference by default.
		//
		explicit inline constexpr shared( store_type* entry ) : entry( entry ) {}

		// Explicit construction by constructor arguments.
		//
		template<typename T1, typename... Tx> requires ( !Same<T1, store_type*> )
		explicit inline shared( T1&& a1, Tx&&... rest ) : shared( new ( allocator{}.allocate( 1 ) ) store_type( std::forward<T1>( a1 ), std::forward<Tx>( rest )... ) ) {}
		
		// Copy construction/assignment.
		//
		inline shared( const shared& ref )
		{
			if ( ref.entry && ref.entry->inc_ref() )
				entry = ref.entry;
		}
		inline shared& operator=( const shared& o )
		{ 
			// If object is null, reset and return.
			//
			if ( !o.entry )
				return reset();

			// Return as is if same entry.
			//
			store_type* prev = entry;
			if ( prev == o.entry )
				return *this;

			// Copy entry and increment reference.
			//
			if ( o.entry->inc_ref() )
				entry = o.entry;
			else
				entry = nullptr;

			// Dereference previous entry.
			//
			if ( prev )
				prev->dec_ref();
		}

		// Move construction/assignment.
		//
		inline shared( shared&& ref ) noexcept
			: entry( std::exchange( ref.entry, nullptr ) ) {}
		inline shared& operator=( shared&& o ) noexcept
		{
			store_type* value = std::exchange( o.entry, nullptr );
			reset().entry = value;
			return *this;
		}
		inline void swap( shared& o )
		{
			std::swap( o.entry, entry );
		}

		// Simple observers.
		//
		inline size_t use_count() const { return entry ? entry->strong_ref_count.load() : 0; }
		inline size_t ref_count() const
		{
			if ( !entry ) return 0;
			uint64_t combined_count = ( ( std::atomic<uint64_t>* ) &entry->strong_ref_count )->load();
			return uint32_t( combined_count >> 32 ) + uint32_t( combined_count );
		}
		inline constexpr bool alive() const { return entry; }
		inline constexpr explicit operator bool() const { return alive(); }

		// Decay to const type.
		//
		inline operator const shared<const T>&() const { return *( const shared<const T>* ) this; }

		// Basic comparison operators.
		//
		inline constexpr bool operator==( const shared& o ) const { return entry == o.entry; }
		inline constexpr bool operator<( const shared& o ) const { return entry < o.entry; }

		// Redirect pointer and dereferencing operator.
		//
		inline constexpr T* get() const { return &entry->value; }
		inline constexpr T* operator->() const { return get(); }
		inline constexpr T& operator*() const { return *get(); }

		// String conversion.
		//
		inline std::string to_string() const { return alive() ? fmt::as_string( *get() ) : XSTD_STR( "nullptr" ); }

		// Resets the pointer to nullptr.
		//
		inline store_type* release() { return std::exchange( entry, nullptr ); }
		inline shared& reset()
		{
			if ( auto prev = release() )
				prev->dec_ref();
			return *this;
		}

		// Constructor invokes reset.
		//
		inline ~shared() { reset(); }
	};

	// Equivalent of std::weak_ptr for shared<T>.
	//
	template<NonVoid T>
	struct weak
	{
		using value_type = T;
		using store_type = typename shared<T>::store_type;
		
		// Store the pointer.
		//
		store_type* entry = nullptr;

		// Default null constructor.
		//
		inline constexpr weak() : entry( nullptr ) {}
		inline constexpr weak( std::nullptr_t ) : entry( nullptr ) {}
		
		// Borrowing constructor/assignment.
		//
		inline constexpr weak( const shared<T>& ref )
			: entry( ref.entry )
		{
			if ( entry ) 
				entry->inc_weak_ref();
		}
		inline constexpr weak& operator=( const shared<T>& ref ) 
		{
			reset().entry = ref.entry;
			if ( entry )
				entry->inc_weak_ref();
			return *this;
		}

		// Copy constructor/assignment.
		//
		inline constexpr weak( const weak& ref )
			: entry( ref.entry )
		{
			if ( entry ) 
				entry->inc_weak_ref();
		}
		inline constexpr weak& operator=( const weak& ref )
		{ 
			reset().entry = ref.entry;
			if ( entry )
				entry->inc_weak_ref();
			return *this;
		}
		
		// Move construction/assignment.
		//
		inline weak( weak&& ref ) noexcept
			: entry( std::exchange( ref.entry, nullptr ) ) {}
		inline weak& operator=( weak&& o ) noexcept
		{
			store_type* value = std::exchange( o.entry, nullptr );
			reset().entry = value;
			return *this;
		}
		inline void swap( weak& o )
		{
			std::swap( o.entry, entry );
		}

		// Basic comparison operators are redirected to the pointer type.
		//
		inline constexpr bool operator<( const weak& o ) const { return entry < o.entry; }
		inline constexpr bool operator==( const weak& o ) const { return entry == o.entry; }
		inline constexpr bool operator<( const shared<T>& o ) const { return entry < o.entry; }
		inline constexpr bool operator==( const shared<T>& o ) const { return entry == o.entry; }

		// Decay to const type.
		//
		inline operator const weak<const T>&() const { return *( const weak<const T>* ) this; }

		// Redirect pointer and dereferencing operator.
		//
		inline constexpr T* get() const { return &entry->value; }
		inline constexpr T* operator->() const { return get(); }
		inline constexpr T& operator*() const { return *get(); }

		// Simple observers.
		//
		inline size_t use_count() const { return entry ? entry->strong_ref_count.load() : 0; }
		inline size_t ref_count() const
		{
			if ( !entry ) return 0;
			uint64_t combined_count = ( ( std::atomic<uint64_t>* ) & entry->strong_ref_count )->load();
			return uint32_t( combined_count >> 32 ) + uint32_t( combined_count );
		}
		inline constexpr bool alive() const { return entry && entry->strong_ref_count.load() != 0; }
		inline constexpr explicit operator bool() const { return alive(); }

		// Upgrading to shared reference.
		//
		inline shared<T> lock() const
		{
			if ( entry && entry->inc_ref() )
				return shared<T>{ entry };
			return nullptr;
		}

		// String conversion.
		//
		inline std::string to_string() const { return lock()->to_string(); }

		// Resets the pointer to nullptr.
		//
		inline store_type* release() { return std::exchange( entry, nullptr ); }
		inline weak& reset()
		{
			if ( auto prev = release() )
				prev->dec_weak_ref();
			return *this;
		}

		// Constructor invokes reset.
		//
		inline ~weak() { reset(); }
	};

	// Equivalent of std::enable_shared_from_this for shared<T>.
	//
	template<typename T>
	struct reference_counted
	{
		using store_type = typename shared<T>::store_type;

		// Creation of shared / weak from this.
		//
		shared<T> shared_from_this()
		{
			if ( inc_ref() )
				return shared<T>{ get_store() };
			return nullptr;
		}
		weak<T> weak_from_this()
		{
			if ( inc_weak_ref() )
				return weak<T>{ get_store() };
			return nullptr;
		}
		shared<const T> shared_from_this() const
		{
			if ( inc_ref() )
				return shared<T>{ get_store() };
			return nullptr;
		}
		weak<const T> weak_from_this() const
		{
			if ( inc_weak_ref() )
				return weak<T>{ get_store() };
			return nullptr;
		}

		// Manual reference management.
		//
		inline store_type* get_store() const { return ( store_type* ) this; }
		inline bool inc_ref() const { return get_store()->inc_ref(); }
		inline void inc_weak_ref() const { get_store()->inc_weak_ref(); }
		inline bool dec_ref() const { return get_store()->dec_ref(); }
		inline bool dec_weak_ref() const { return get_store()->dec_weak_ref(); }
	};

	// Creates a reference counted object.
	//
	template<typename T, typename... Tx>
	__forceinline static constexpr shared<T> make_shared( Tx&&... args ) 
	{
		using allocator = typename shared<T>::allocator;
		using store_type = typename shared<T>::store_type;
		return shared<T>{ new ( allocator{}.allocate( 1 ) ) store_type( std::forward<Tx>( args )... ) };
	}
};