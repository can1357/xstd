#pragma once
#include "coro.hpp"
#include <optional>

namespace xstd {
	// Simple generator coroutine.
	//
	template<typename T>
	struct generator {
		struct promise_type {
			coroutine_handle<> continuation = {};
			std::optional<T>*  placement;

			struct yield_awaitable {
				inline bool await_ready() { return false; }
				inline coroutine_handle<> await_suspend( coroutine_handle<promise_type> handle ) noexcept {
					return handle.promise().continuation;
				}
				inline void await_resume() {}
			};
			template<typename V>
			yield_awaitable yield_value( V&& value ) {
				std::exchange( placement, nullptr )->emplace( std::forward<V>( value ) );
				return {};
			}
			generator get_return_object() { return *this; }
			suspend_always initial_suspend() { return {}; }
			suspend_always final_suspend() noexcept {}
			void return_void() {}
			XSTDC_UNHANDLED_RETHROW;
		};

		// Returns whether or not the generator is done producing values.
		//
		bool finished() const { return handle.done(); }
		explicit operator bool() const { return !finished; }

		// Returns the work that needs to be started for the generator to start producing values.
		//
		bool work_released = false;
		coroutine_handle<> worker() { return std::exchange( work_released, true ) ? nullptr : handle; }

		// Internal constructor.
		//
		unique_coroutine<promise_type> handle;
		generator( promise_type& pr ) : handle( pr ) {}

		// Implement await.
		//
		struct awaiter {
			coroutine_handle<promise_type> hnd;
			std::optional<T> result = std::nullopt;

			inline bool await_ready() { return hnd.done(); }
			inline coroutine_handle<> await_suspend( coroutine_handle<> h ) {
				auto& promise = hnd.promise();
				promise.placement =    &result;
				promise.continuation = h;
				return hnd;
			}
			inline std::optional<T> await_resume() { return std::move( result ); }
		};
		inline awaiter operator co_await() { return { handle.hnd }; }
	};
};