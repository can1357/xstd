#pragma once
#include <atomic>
#include "intrinsics.hpp"

namespace xstd
{
	namespace impl
	{
		template<typename T>
		concept SlistViable = requires( T* x ) { x = x->next; };
	};

	template<impl::SlistViable T, typename Dx = std::default_delete<T>>
	struct atomic_slist
	{
		struct alignas( 16 ) versioned_pointer
		{
			T* pointer = nullptr;
			uint32_t version = 0;
			uint32_t length = 0;
		};
		static_assert( sizeof( versioned_pointer ) == 16, "Unexpected padding." );

		// List head and the length.
		//
		versioned_pointer head = {};

		// Default empty construct, no copy/move construction/assign.
		//
		atomic_slist() {}
		atomic_slist( atomic_slist&& other ) { other.swap( *this ); }
		atomic_slist( const atomic_slist& ) = delete;
		atomic_slist& operator=( atomic_slist&& other ) { swap( *this ); return *this; }
		atomic_slist& operator=( const atomic_slist& ) = delete;

		// Compare exchange primitive.
		//
		FORCE_INLINE bool cmpxchg_head( versioned_pointer& expected, const versioned_pointer& desired )
		{
			return cmpxchg( head, expected, desired );
		}

		// Atomic add/remove element(s).
		//
		FORCE_INLINE void push( T* node )
		{
			versioned_pointer curr_head = head;
			while ( true )
			{
				versioned_pointer new_head = { node, curr_head.version + 1, curr_head.length + 1 };
				node->next = curr_head.pointer;
				if ( cmpxchg_head( curr_head, new_head ) )
					break;
				yield_cpu();
			}
		}
		FORCE_INLINE void push_all( T* first )
		{
			if ( !first ) return;
			T* last = first;
			size_t count = 1;
			while ( last && last->next )
				last = last->next, ++count;

			versioned_pointer curr_head = head;
			while ( true )
			{
				versioned_pointer new_head = { first, curr_head.version + 1, curr_head.length + count };
				last->next = curr_head.pointer;
				if ( cmpxchg_head( curr_head, new_head ) )
					break;
				yield_cpu();
			}
		}
		FORCE_INLINE T* pop()
		{
			versioned_pointer curr_head = head;
			while ( curr_head.pointer != nullptr )
			{
				versioned_pointer new_head = { curr_head.pointer->next, curr_head.version + 1, curr_head.length - 1 };
				if ( cmpxchg_head( curr_head, new_head ) )
					break;
				yield_cpu();
			}
			return curr_head.pointer;
		}

		// Non-atomic properties.
		//
		FORCE_INLINE T* front() const { return head.pointer; }
		FORCE_INLINE bool empty() const { return !length; }
		FORCE_INLINE size_t size() const { return head.length; }

		// Swapping.
		//
		FORCE_INLINE T* exchange( const atomic_slist& other )
		{
			versioned_pointer val = head;
			while ( !cmpxchg_head( val, {} ) )
				yield_cpu();
			return val.pointer;
		}
		FORCE_INLINE void swap( atomic_slist& other )
		{
			T* firstm = exchange( {} );
			T* firsto = other.exchange( {} );
			push_all( firsto );
			other.push_all( firstm );
		}
		FORCE_INLINE ~atomic_slist()
		{
			T* p = head.pointer;
			while ( p )
				Dx{}( std::exchange( p, p->next ) );
		}
	};
};