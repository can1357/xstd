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
				list->push( this );
				return true;
			}
			void await_resume() const noexcept {}

			// Runs this task and the rest of the queue, returning the number of tasks executed.
			//
			size_t run() {
				size_t n = 0;
				auto it = this;
				while ( it ) {
					++n;
					auto next = std::exchange( it->next, nullptr );
					it->hnd();
					it = next;
				}
				return n;
			}
		};

		// List.
		//
		LockType lock;
		entry* tail = nullptr;
		entry* head = nullptr;

		// Pushes an entry.
		//
		void push( entry* w ) {
			std::lock_guard _g{ lock };
			if ( tail ) {
				tail->next = w;
			} else {
				head = w;
			}
			tail = w;
			w->next = nullptr;
		}

		// Pops a single entry.
		//
		entry* pop() {
			std::lock_guard _g{ lock };
			auto e = head;
			if ( !e ) return nullptr;

			if ( e == tail ) {
				head = tail = nullptr;
			} else {
				head = e->next;
			}
			e->next = nullptr;
			return e;
		}

		// Pops all entries.
		//
		entry* pop_all() {
			std::lock_guard _g{ lock };
			auto e = head;
			head = tail = nullptr;
			return e;
		}

		// Pops the next entry and executes it.
		//
		size_t step() {
			auto e = pop();
			return e ? e->run() : 0;
		}

		// Pops all entries and executes them.
		//
		size_t consume() {
			auto e = pop_all();
			return e ? e->run() : 0;
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