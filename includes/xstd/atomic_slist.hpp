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

	template<impl::SlistViable T>
	struct atomic_slist
	{
		struct alignas( 16 ) versioned_pointer
		{
			T* pointer = nullptr;
			uint64_t version = 0;
		};

		// List head and the length.
		//
		versioned_pointer head = {};
		std::atomic<size_t> length = 0;

		// Default empty construct, no copy/move construction/assign.
		//
		atomic_slist() {}
		atomic_slist( atomic_slist&& ) noexcept = delete;
		atomic_slist( const atomic_slist& ) = delete;
		atomic_slist& operator=( atomic_slist&& ) noexcept = delete;
		atomic_slist& operator=( const atomic_slist& ) = delete;

		// Compare exchange primitive.
		//
		FORCE_INLINE bool cmpxchg( versioned_pointer& expected, const versioned_pointer& desired )
		{
			return _InterlockedCompareExchange128(
				( volatile long long* ) &head,
				( ( long long* ) &expected )[ 1 ],
				( ( long long* ) &expected )[ 0 ],
				( long long* ) &desired
			);
		}

		// Atomic add/remove element.
		//
		FORCE_INLINE void push( T* node )
		{
			versioned_pointer curr_head = head;
			while ( true )
			{
				versioned_pointer new_head = { node, curr_head.version + 1 };
				node->next = curr_head.pointer;
				if ( cmpxchg( curr_head, new_head ) )
				{
					++length;
					break;
				}
				yield_cpu();
			}
		}
		FORCE_INLINE T* pop()
		{
			versioned_pointer curr_head = head;
			while ( curr_head.pointer != nullptr )
			{
				versioned_pointer new_head = { curr_head.pointer->next, curr_head.version + 1 };
				if ( cmpxchg( curr_head, new_head ) )
				{
					--length;
					break;
				}
				yield_cpu();
			}
			return curr_head.pointer;
		}

		// Non-atomic properties.
		//
		FORCE_INLINE T* front() const { return head.pointer; }
		FORCE_INLINE bool empty() const { return !length; }
		FORCE_INLINE size_t size() const { return length; }
	};
};