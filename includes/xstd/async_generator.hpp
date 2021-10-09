#pragma once
#include "coro.hpp"
#include "event.hpp"
#include "chore.hpp"
#include <optional>
#include <atomic>

namespace xstd
{
	// Simple async generator coroutine, must have a single consumer.
	//
	template<typename T>
	struct async_generator
	{
		struct promise_type
		{
			event_base receive_event = {};
			std::atomic<void*> recepient = nullptr;
			std::optional<T>* store;

			struct yield_awaitable
			{
				bool dropped;
				inline bool await_ready() { return false; }
				inline void await_suspend( coroutine_handle<promise_type> handle ) noexcept
				{ 
					if ( dropped )
					{
						handle.destroy();
					}
					else
					{
						auto recepient = coroutine_handle<>::from_address( handle.promise().recepient.exchange( nullptr ) );
						handle.promise().receive_event.reset();
						chore( handle );
						recepient();
					}
				}
				inline void await_resume() {}
			};

			struct final_awaitable
			{
				bool dropped;
				inline bool await_ready() noexcept { return false; }
				inline bool await_suspend( coroutine_handle<promise_type> handle ) noexcept
				{
					if ( !dropped )
						coroutine_handle<>::from_address( handle.promise().recepient.exchange( nullptr ) )( );
					return false;
				}
				inline void await_resume() const noexcept {}
			};

			yield_awaitable yield_value( T value )
			{
				receive_event.wait();
				if ( !store )
					return { true };
				store->emplace( std::move( value ) );
				return { false };
			}
			async_generator get_return_object() { return *this; }
			suspend_always initial_suspend() { return {}; }
			final_awaitable final_suspend() noexcept
			{
				receive_event.wait();
				if ( !store )
					return { true };
				store->reset();
				return { false };
			}
			void return_void() {}
			void unhandled_exception() {}
		};

		// Returns whether or not the generator is done producing values.
		//
		bool finished() const { return !handle || handle.done(); }

		// Returns the work that needs to be started for the generator to start producing values.
		//
		bool work_released = false;
		coroutine_handle<> worker() { return std::exchange( work_released, true ) ? nullptr : handle; }

		// Internal constructor.
		//
		coroutine_handle<promise_type> handle;
		async_generator( promise_type& pr ) : handle( coroutine_handle<promise_type>::from_promise( pr ) ) {}

		// No copy.
		//
		async_generator( const async_generator& ) = delete;
		async_generator& operator=( const async_generator& ) = delete;

		// Move via swap.
		//
		async_generator( async_generator&& o ) noexcept : work_released( std::exchange( o.work_released, false ) ), handle( std::exchange( o.handle, nullptr ) ) {}
		async_generator& operator=( async_generator&& ) noexcept
		{
			std::swap( handle, o.handle );
			std::swap( work_released, o.work_released );
			return *this;
		}

		// Implement await.
		//
		struct proxy_awaitable
		{
			promise_type* promise;
			std::optional<T> result = std::nullopt;

			inline bool await_ready() { return promise == nullptr; }
			inline void await_suspend( coroutine_handle<> h )
			{
				promise->store = &result;
				promise->recepient.store( h.address(), std::memory_order::relaxed );
				promise->receive_event.notify();
			}
			inline std::optional<T>&& await_resume() { return std::move( result ); }
		};
		inline proxy_awaitable operator co_await() const { return { handle ? &handle.promise() : nullptr }; }

		// Drops the producer and signals it to stop producing.
		//
		void terminate()
		{
			if ( !handle ) return;

			// If we have not started yet, destroy the coroutine.
			//
			if ( !work_released )
			{
				handle.destroy();
			}
			// Otherwise signal it to stop producing.
			//
			else
			{
				auto& pr = handle.promise();
				pr.store = nullptr;
				pr.receive_event.notify();
			}
			handle = nullptr;
		}
		~async_generator() { terminate(); }
	};
};