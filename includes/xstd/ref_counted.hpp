#pragma once
#include "type_helpers.hpp"
#include "assert.hpp"

namespace xstd
{
	struct ref_counted_tag_t {};

	// Simple implementation of shared_ptr with known destructor and simpler reference counting.
	//
	template<typename T>
	struct ref
	{
		struct ref_base { mutable std::atomic<uint32_t> ref_count = { 1 }; };
		struct wrapper : T, ref_base { using T::T; };
		using store_type = std::conditional_t<HasBase<ref_counted_tag_t, T>, T, wrapper>;

		store_type* ptr = nullptr;

		// Construction by pointer.
		//
		explicit ref( store_type* ptr, bool add_ref = true ) : ptr( ptr )
		{ 
			if ( ptr && add_ref ) 
				ptr->ref_count++; 
		}

		// Null reference.
		//
		ref() : ptr( nullptr ) {}
		ref( std::nullptr_t ) : ptr( nullptr ) {}

		// Move and copy.
		//
		ref( ref&& o ) noexcept : ptr( std::exchange( o.ptr, nullptr ) ) {}
		ref( const ref& o ) : ref( o.ptr, true ) {}
		ref& operator=( ref&& o ) noexcept 
		{ 
			std::swap( ptr, o.ptr ); 
			return *this; 
		}
		ref& operator=( const ref& o ) 
		{ 
			reset( o.ptr ); 
			return *this; 
		}

		// Reset and release.
		//
		void reset( store_type* new_ptr = nullptr )
		{
			if ( new_ptr )
				new_ptr->ref_count++;
			if ( auto p = std::exchange( ptr, new_ptr ) )
			{
				if ( !--p->ref_count ) [[unlikely]]
					cold_call( [ = ] { std::destroy_at( p ); } );
			}
		}
		store_type* release() { return std::exchange( ptr, nullptr ); }

		// Validity check and pointer like semantics.
		//
		explicit operator bool() const { return ptr != nullptr; }
		T* operator->() const { return ptr; }
		T& operator*() const { return *ptr; }
		T* get() const { return ptr; }

		// Reference helpers.
		//
		size_t inc_ref() const 
		{ 
			return ++ptr->ref_count; 
		}
		size_t dec_ref() const
		{ 
			size_t n = --ptr->ref_count;
			fassert( n != 0 );
			return n;
		}
		size_t dec_ref()
		{
			size_t n = --ptr->ref_count;
			if ( !n )
				std::destroy_at( std::exchange( ptr, nullptr ) );
			return n;
		}
		size_t ref_count() const { return ptr->ref_count.load( std::memory_order::relaxed ); }

		// Dec-ref on destroy.
		//
		~ref() { reset(); }
	};

	// enable_shared_from_this equivalent.
	//
	template<typename T>
	struct ref_counted : ref_counted_tag_t
	{
		mutable std::atomic<uint32_t> ref_count = { 1 };
		T* _wptr = nullptr;

		ref<T> add_ref() 
		{  
			fassert( _wptr );
			return ref<T>{ _wptr }; 
		}
		ref<const T> add_ref() const 
		{ 
			fassert( _wptr );
			return ref<const T>{ _wptr }; 
		}
	};

	// Creates a ref-counted object.
	//
	template<typename T, typename... Tx> 
	inline ref<T> make_refc( Tx&&... args ) 
	{ 
		using V = typename ref<T>::store_type;
		V* ptr = new V( std::forward<Tx>( args )... );
		if constexpr ( HasBase<ref_counted_tag_t, T> )
			ptr->_wptr = ( T* ) ptr;
		return ref<T>( ptr, false ); 
	}
};