#pragma once
#include "intrinsics.hpp"
#include "coro.hpp"
#include "spinlock.hpp"

namespace xstd {
	// Task list for implementing a scheduler.
	//
	template<typename LockType>
	struct basic_task_list {
		// Entry type.
		//
		struct entry {
			union {
				basic_task_list* list;
				entry*           next;
			};
			coroutine_handle<>   hnd;

			bool await_ready() const noexcept { return false; }
			bool await_suspend( coroutine_handle<> hnd ) noexcept {
				auto* list = this->list;
				if ( list->push( this, hnd ) ) {
					return false;
				}
				return true;
			}
			bool await_resume() const noexcept {
				return hnd == nullptr;
			}
		};

		// List.
		//
		LockType lock;
		entry*   tail = nullptr;
		entry    head = {};

		// Pushes an entry, returns true if master entry.
		//
		bool push( entry* i, coroutine_handle<> hnd ) {
			std::lock_guard _g{ lock };
			if ( !tail ) {
				// i =    { nullptr, hnd }
				// tail = i
				// pop into head before resume
				// i =    { discarded }
				// head = { nullptr, hnd }
				// tail = &head
				// 
				head = { nullptr, hnd };
				tail = &head;
				return true;
			} else {
				tail->next = i;
				tail =       i;
				i->hnd =     hnd;
				i->next =    nullptr;
				return false;
			}
		}

		// Pops the next entry.
		//
		coroutine_handle<> pop() {
			std::unique_lock g{ lock };
			while ( true ) {
				// End condition.
				//
				auto* next = head.next;
				if ( !next ) {
					dassert( &head == tail );
					head = { nullptr, nullptr };
					tail = nullptr;
					return nullptr;
				}

				// Pop next item.
				//
				head = *next;
				if ( !head.next ) {
					dassert( tail == next );
					tail = &head;
				}
				dassert( head.hnd );
				return head.hnd;
			}
		}

		// Called after task completion to yield to next entry.
		//
		void yield( bool flag = true ) {
			if ( flag )
				while ( auto hnd = pop() )
					hnd();
		}

		// Make co-awaitable.
		//
		entry operator co_await() {
			return { this };
		}
	};
	using task_list =            basic_task_list<noop_lock>;
	using concurrent_task_list = basic_task_list<xspinlock<>>;
};