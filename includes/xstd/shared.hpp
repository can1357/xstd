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
	namespace impl
	{
		// Store that prefixes the type with reference counting headers.
		//
		template<NonVoid T>
		struct ref_store
		{
			std::atomic<uint32_t> strong_ref_count = 1;
			std::atomic<uint32_t> weak_ref_count =   0;
		
			// -- Prevents call to T's destructor on our destructor.
			union
			{
				T                 value;
				char              raw[ 1 ];
			};
			inline ~ref_store() {}

			// Constructor redirects to type.
			//
			template<typename... Tx> 
			inline ref_store( Tx&&... args ) : value( std::forward<Tx>( args )... ) {}

			// No copy/move.
			//
			ref_store( ref_store&& ) = delete;
			ref_store( const ref_store& ) = delete;
			ref_store& operator=( ref_store&& ) = delete;
			ref_store& operator=( const ref_store& ) = delete;

			// Reference management:
			//
			inline bool inc_ref()
			{
				dassert( strong_ref_count.load() >= 0 );
				if ( strong_ref_count++ != 0 )
					return true;
				--strong_ref_count;
				return false;
			}
			inline bool dec_ref()
			{
				dassert( strong_ref_count.load() > 0 );
				if ( !--strong_ref_count )
				{
					std::destroy_at( &value );
					if ( !weak_ref_count )
					{
						delete this;
						return true;
					}
				}
				return false;
			}
			inline void inc_weak_ref()
			{
				dassert( ( weak_ref_count.load() + strong_ref_count.load() ) >= 0 );
				++weak_ref_count;
			}
			inline bool dec_weak_ref()
			{
				dassert( weak_ref_count.load() > 0 );
				if ( !--weak_ref_count && !strong_ref_count )
				{
					delete this;
					return true;
				}
				return false;
			}
		};
	};

	// Implements fast and atomic shared objects with with no type erasure, also allows manual 
	// management of reference counters for more complex scenarios.
	//
	template<NonVoid T>
	struct shared
	{
		using value_type = T;
		using store_type = impl::ref_store<std::remove_cv_t<T>>;

		// Store the pointer.
		//
		store_type* entry;

		// Null construction.
		//
		inline constexpr shared() : entry( nullptr ) {}
		inline constexpr shared( std::nullptr_t ) : shared() {}

		// Explicit construction by entry pointer, adopts the reference by default.
		//
		explicit inline constexpr shared( store_type* entry ) : entry( entry ) {}

		// Copy construction/assignment.
		//
		inline shared( const shared& o )
		{
			if ( o.entry && o.entry->inc_ref() )
				entry = o.entry;
			else
				entry = nullptr;
		}
		inline shared& operator=( const shared& o )
		{
			// Return as is if same entry.
			//
			store_type* prev = entry;
			if ( prev == o.entry )
				return *this;

			// If object is null, reset and return.
			//
			if ( !o.entry )
				return reset();

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
			return *this;
		}

		// Move construction/assignment.
		//
		template<typename Ty> requires Same<std::remove_cvref_t<T>, Ty>
		inline shared( shared<Ty>&& ref ) noexcept : entry( std::exchange( ref.entry, nullptr ) ) {}
		inline shared& operator=( shared&& o ) noexcept { swap( o ); return *this; }
		inline void swap( shared& o ) { std::swap( o.entry, entry ); }

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

		// Basic comparison operators.
		//
		inline constexpr bool operator==( const shared& o ) const { return entry == o.entry; }
		inline constexpr bool operator<( const shared& o ) const { return entry < o.entry; }
		
		// Redirect pointer and dereferencing operator.
		//
		inline constexpr T* get() const { return entry ? &entry->value : nullptr; }
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
		using store_type = impl::ref_store<std::remove_cv_t<T>>;
		
		// Store the pointer.
		//
		store_type* entry = nullptr;

		// Default null constructor.
		//
		inline constexpr weak() : entry( nullptr ) {}
		inline constexpr weak( std::nullptr_t ) : entry( nullptr ) {}

		// Explicit construction by entry pointer, adopts the reference by default.
		//
		explicit inline constexpr weak( store_type* entry ) : entry( entry ) {}

		// Move construction/assignment.
		//
		template<typename Ty> requires Same<std::remove_cvref_t<T>, Ty>
		inline weak( weak<Ty>&& ref ) noexcept : entry( std::exchange( ref.entry, nullptr ) ) {}
		inline weak& operator=( weak&& o ) noexcept { swap( o ); return *this; }
		inline void swap( weak& o ) { std::swap( o.entry, entry ); }

		// Copy constructor/assignment.
		//
		inline weak( const weak& ref ) : entry( ref.entry ) { if ( entry )  entry->inc_weak_ref(); }
		inline weak( const shared<T>& o ) : entry( o.entry ) { if ( entry ) entry->inc_weak_ref(); }
		inline weak& operator=( const weak& o )
		{ 
			// Return as is if same entry.
			//
			store_type* prev = entry;
			if ( prev == o.entry )
				return *this;

			// If object is null, reset and return.
			//
			if ( !o.entry )
				return reset();

			// Copy entry and increment reference.
			//
			o.entry->inc_weak_ref();
			entry = o.entry;

			// Dereference previous entry.
			//
			if ( prev )
				prev->dec_weak_ref();
			return *this;
		}
		inline constexpr weak& operator=( const shared<T>& o ) 
		{
			// Return as is if same entry.
			//
			store_type* prev = entry;
			if ( prev == o.entry )
				return *this;

			// If object is null, reset and return.
			//
			if ( !o.entry )
				return reset();

			// Copy entry and increment reference.
			//
			o.entry->inc_weak_ref();
			entry = o.entry;

			// Dereference previous entry.
			//
			if ( prev )
				prev->dec_weak_ref();
			return *this;
		}

		// Basic comparison operators are redirected to the pointer type.
		//
		inline constexpr bool operator<( const weak& o ) const { return entry < o.entry; }
		inline constexpr bool operator==( const weak& o ) const { return entry == o.entry; }
		inline constexpr bool operator<( const shared<T>& o ) const { return entry < o.entry; }
		inline constexpr bool operator==( const shared<T>& o ) const { return entry == o.entry; }

		// Redirect pointer and dereferencing operator.
		//
		inline constexpr T* get() const { return entry ? &entry->value : nullptr; }
		inline constexpr T* operator->() const { return get(); }
		inline constexpr T& operator*() const { return *get(); }

		// Simple observers.
		//
		inline size_t use_count() const { return entry ? entry->strong_ref_count.load() : 0; }
		inline size_t ref_count() const
		{
			if ( !entry ) return 0;
			uint64_t combined_count = ( ( std::atomic<uint64_t>* ) &entry->strong_ref_count )->load();
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
		using value_type = T;
		using store_type = impl::ref_store<std::remove_cv_t<T>>;

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
			inc_weak_ref();
			return weak<T>{ get_store() };
		}
		shared<const T> shared_from_this() const
		{
			if ( inc_ref() )
				return shared<const T>{ get_store() };
			return nullptr;
		}
		weak<const T> weak_from_this() const
		{
			inc_weak_ref();
			return weak<const T>{ get_store() };
		}

		// Manual reference management.
		//
		inline store_type* get_store() const { return ( store_type* ) this; }
		inline bool inc_ref() const { return get_store()->inc_ref(); }
		inline void inc_weak_ref() const { get_store()->inc_weak_ref(); }
		inline bool dec_ref() const { return get_store()->dec_ref(); }
		inline bool dec_weak_ref() const { return get_store()->dec_weak_ref(); }
	};

	// Inline storage of shared objects, will throw an error if there are references at destruction.
	//
	template<NonVoid T>
	struct inline_shared
	{
		using value_type = T;
		using store_type = impl::ref_store<std::remove_cv_t<T>>;

		shared<T> base;
		alignas( store_type ) uint8_t storage[ sizeof( store_type ) ];

		// Construct arguments are passed to the stored type.
		//
		template<typename... Tx>
		inline inline_shared( Tx&&... args ) : base( new ( &storage[ 0 ] ) store_type( std::forward<Tx>( args )... ) ) {}
		
		// No move/copy assignment/construction allowed.
		//
		inline_shared( inline_shared&& ) = delete;
		inline_shared( const inline_shared& ) = delete;
		inline_shared& operator=( inline_shared&& ) = delete;
		inline_shared& operator=( const inline_shared& ) = delete;

		// Decay to shared.
		//
		inline operator const shared<T>&() { return *( const shared<T>* ) &base; }
		inline operator const shared<const T>&() const { return *( const shared<const T>* ) &base; }
		inline store_type* get_store() const { return ( store_type* ) &storage[ 0 ]; }
		inline T* get() { return &get_store()->value; }
		inline const T* get() const { return &get_store()->value; }
		inline T* operator->() { return get(); }
		inline const T* operator->() const { return get(); }
		inline T& operator*() { return *get(); }
		inline const T& operator*() const { return *get(); }

		// String conversion.
		//
		inline std::string to_string() const { return fmt::as_string( *get() ); }

		// Destruction ensures there are no refences, skips deallocation and destroys the object.
		//
		inline ~inline_shared()
		{
			fassert( base.entry->strong_ref_count == 1 && base.entry->weak_ref_count == 0 );
			std::destroy_at( &base.release()->value );
		}
	};

	// Creates a reference counted object.
	//
	template<typename T, typename... Tx>
	inline static shared<T> make_shared( Tx&&... args ) 
	{
		return shared<T>{ new impl::ref_store<std::remove_cv_t<T>>( std::forward<Tx>( args )... ) };
	}
};