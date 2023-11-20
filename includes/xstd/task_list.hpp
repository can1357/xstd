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
				this->hnd = hnd;
				if ( list->push( this ) ) {
					return false;
				}
				return true;
			}
			void await_resume() const noexcept {}
		};

		// List.
		//
		LockType lock;
		entry* tail = nullptr;
		entry* head = nullptr;

		// Pushes an entry, returns true if master entry.
		//
		bool push( entry* w ) {
			std::lock_guard _g{ lock };
			if ( tail ) {
				tail->next = w;
			} else {
				head = w;
			}
			tail = w;
		}

		// Pops the next entry.
		//
		entry* pop( bool all = false ) {
			std::lock_guard _g{ lock };
			if ( !head ) return nullptr;
			auto r = head;
			if ( all || head == tail ) {
				head = tail = nullptr;
			} else {
				head = head->next;
			}
			return r;
		}
		entry* pop_all() { return pop( true ); }

		// Consumes all pending work.
		//
		void consume() {
			entry* r = pop_all();
			while ( r ) {
				entry* next = r->next;
				r->hnd();
				r = next;
			}
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