#pragma once
#include "coro.hpp"
#include "assert.hpp"

namespace xstd {
	namespace detail {
		template<typename T>
		struct job_awaitable {
			FORCE_INLINE inline bool await_ready() noexcept { return false; }
			FORCE_INLINE inline coroutine_handle<> await_suspend( coroutine_handle<T> handle ) noexcept {
				coroutine_handle<> onto = handle.promise().continuation;
				return onto ? onto : handle;
			}
			FORCE_INLINE inline void await_resume() const noexcept {}
		};
	};

	// Simple job coroutine with no exceptions or sync interface.
	//
	template<typename T = void>
	struct job {
		struct promise_type {
			coroutine_handle<> continuation = {};
			std::optional<T>* placement = nullptr;

			FORCE_INLINE inline job get_return_object() { return *this; }
			FORCE_INLINE inline suspend_always initial_suspend() noexcept { return {}; }
			FORCE_INLINE inline detail::job_awaitable<promise_type> final_suspend() noexcept { return {}; }
			XSTDC_UNHANDLED_RETHROW;
			template<typename V>
			FORCE_INLINE inline void return_value( V&& v ) {
				if ( placement )
					placement->emplace( std::forward<V>( v ) );
			}
		};
		struct awaiter {
			unique_coroutine<promise_type> handle;
			std::optional<T> placement;

			FORCE_INLINE inline bool await_ready() noexcept { return false; }
			FORCE_INLINE inline coroutine_handle<> await_suspend( coroutine_handle<> hnd ) noexcept {
				auto& pr = handle.promise();
				pr.continuation = hnd;
				pr.placement = &placement;
				return handle.hnd;
			}
			FORCE_INLINE inline T await_resume() noexcept {
				strong_assume( placement.has_value() );
				return std::move( placement ).value();
			}
		};
		inline awaiter operator co_await() && noexcept { return { std::move( handle ) }; }

		// Coroutine handle and the internal constructor.
		//
		unique_coroutine<promise_type> handle;
		FORCE_INLINE job( promise_type& pr ) : handle( pr ) {}

		// Move/Default constructor.
		//
		constexpr job() = default;
		constexpr job( std::nullptr_t ) : handle( nullptr ) {}
		constexpr job( job&& ) noexcept = default;
		constexpr job& operator=( job&& ) noexcept = default;

		// Observers.
		//
		FORCE_INLINE inline bool has_value() const { return handle.get() != nullptr; }
		FORCE_INLINE inline explicit operator bool() const { return has_value(); }

		// Launches the job synchronously, lets go of the ownership.
		//
		FORCE_INLINE coroutine_handle<promise_type> release() { return handle.release(); }
		FORCE_INLINE void launch() { release().resume(); }
		FORCE_INLINE void operator()() { launch(); }

		// Tails into a job from another coroutine discarding the result.
		//
		FORCE_INLINE coroutine_handle<> chain( coroutine_handle<> hnd ) {
			if ( !handle ) return hnd;
			auto& pr = handle.promise();
			pr.continuation = hnd;
			return handle.hnd;
		}
	};

	// Void variant.
	//
	template<>
	struct job<void> {
		struct promise_type {
			coroutine_handle<> continuation = {};

			FORCE_INLINE inline job get_return_object() { return *this; }
			FORCE_INLINE inline suspend_always initial_suspend() noexcept { return {}; }
			FORCE_INLINE inline detail::job_awaitable<promise_type> final_suspend() noexcept { return {}; }
			XSTDC_UNHANDLED_RETHROW;
			FORCE_INLINE inline void return_void() {}
		};
		struct awaiter {
			unique_coroutine<promise_type> handle;
			FORCE_INLINE inline bool await_ready() noexcept { return false; }
			FORCE_INLINE inline coroutine_handle<> await_suspend( coroutine_handle<> hnd ) noexcept {
				handle.promise().continuation = hnd;
				return handle.hnd;
			}
			FORCE_INLINE inline void await_resume() const noexcept {}
		};
		inline awaiter operator co_await() && noexcept { return { std::move( handle ) }; }

		// Coroutine handle and the internal constructor.
		//
		unique_coroutine<promise_type> handle;
		FORCE_INLINE job( promise_type& pr ) : handle( pr ) {}

		// Move/Default constructor.
		//
		constexpr job() = default;
		constexpr job( std::nullptr_t ) : handle( nullptr ) {}
		job( job&& ) noexcept = default;
		job& operator=( job&& ) noexcept = default;

		// Observers.
		//
		FORCE_INLINE inline bool has_value() const { return bool( handle ); }
		FORCE_INLINE inline explicit operator bool() const { return has_value(); }

		// Launches the job synchronously, lets go of the ownership.
		//
		FORCE_INLINE coroutine_handle<promise_type> release() { return handle.release(); }
		FORCE_INLINE void launch() { release().resume(); }
		FORCE_INLINE void operator()() { launch(); }

		// Tails into a job from another coroutine discarding the result.
		//
		FORCE_INLINE coroutine_handle<> chain( coroutine_handle<> hnd ) {
			if ( !handle ) return hnd;
			auto& pr = handle.promise();
			pr.continuation = hnd;
			return handle.hnd;
		}
	};
};