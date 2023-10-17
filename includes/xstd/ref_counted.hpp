#pragma once
#include "type_helpers.hpp"
#include "assert.hpp"

namespace xstd
{
	struct ref_counted_base {
		mutable std::atomic<uint32_t> ref_count = { 1 };
	};

	// Simple implementation of shared_ptr with known destructor and simpler reference counting.
	//
	template<typename T>
	struct ref
	{
		struct wrapper : T, ref_counted_base { using T::T; };
		using store_type = std::conditional_t<HasBase<ref_counted_base, T>, T, wrapper>;

		store_type* ptr = nullptr;

		// Construction by pointer.
		//
		constexpr explicit ref( store_type* ptr, bool add_ref = true ) : ptr( ptr )
		{ 
			if ( ptr && add_ref ) 
				ptr->ref_count++; 
		}

		// Null reference.
		//
		constexpr ref() = default;
		constexpr ref( std::nullptr_t ) : ptr( nullptr ) {}

		// Move and copy.
		//
		constexpr ref( ref&& o ) noexcept : ptr( std::exchange( o.ptr, nullptr ) ) {}
		constexpr ref( const ref& o ) : ref( o.ptr, true ) {}
		constexpr ref& operator=( ref&& o ) noexcept {
			std::swap( ptr, o.ptr );
			return *this;
		}
		constexpr ref& operator=( const ref& o ) {
			reset( o.ptr );
			return *this;
		}

		// Type cast.
		//
		template<typename Ty> requires ( std::is_base_of_v<T, Ty> && std::has_virtual_destructor_v<T> )
		constexpr ref( const ref<Ty>& o ) : ref( o.ptr, true ) {}
		template<typename Ty> requires ( std::is_base_of_v<T, Ty> && std::has_virtual_destructor_v<T> )
		constexpr ref( ref<Ty>&& o ) : ref( o.release(), false ) {}
		template<typename Ty>
		explicit ref( const ref<Ty>& o ) : ref( (store_type*) o.ptr, true ) {}
		template<typename Ty>
		explicit ref( ref<Ty>&& o ) : ref( (store_type*) o.release(), false ) {}

		// Reset and release.
		//
		constexpr void reset( store_type* new_ptr = nullptr ) {
			if ( new_ptr )
				new_ptr->ref_count++;
			if ( auto p = std::exchange( ptr, new_ptr ) ) {
				if ( !--p->ref_count ) [[unlikely]]
					delete p;
			}
		}
		constexpr store_type* release() { return std::exchange( ptr, nullptr ); }

		// Validity check and pointer like semantics.
		//
		constexpr explicit operator bool() const { return ptr != nullptr; }
		constexpr T* operator->() const { return ptr; }
		constexpr T& operator*() const { return *ptr; }
		constexpr T* get() const { return ptr; }

		// Reference helpers.
		//
		size_t inc_ref() const { 
			return ++ptr->ref_count; 
		}
		size_t dec_ref() const { 
			size_t n = --ptr->ref_count;
			fassert( n != 0 );
			return n;
		}
		size_t dec_ref() {
			size_t n = --ptr->ref_count;
			if ( !n )
				delete std::exchange( ptr, nullptr );
			return n;
		}
		size_t ref_count() const { return ptr->ref_count.load( std::memory_order::relaxed ); }

		// Dec-ref on destroy.
		//
		constexpr ~ref() { reset(); }
	};

	// enable_shared_from_this equivalent.
	//
	template<typename T>
	struct ref_counted : ref_counted_base
	{
		template<typename Ty = T>
		ref<Ty> add_ref() {  
			return ref<Ty>{ static_cast<Ty*>( this ) };
		}
		template<typename Ty = T>
		ref<const Ty> add_ref() const {
			return ref<const Ty>{ static_cast<const Ty*>( this ) };
		}
	};

	// Creates a ref-counted object.
	//
	template<typename T, typename... Tx> 
	inline ref<T> make_refc( Tx&&... args ) { 
		using V = typename ref<T>::store_type;
		return ref<T>( new V( std::forward<Tx>( args )... ), false );
	}
};