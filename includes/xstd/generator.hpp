#pragma once
#include <optional>
#include "coro.hpp"

namespace xstd
{
	// Simple generator coroutine.
	//
	template<typename T>
	struct generator
	{
		struct promise_type
		{
			std::optional<T> current = std::nullopt;
			coroutine_handle<> continuation = {};

			struct yield_awaitable
			{
				bool await_ready() noexcept { return false; }
				void await_suspend( coroutine_handle<promise_type> handle ) noexcept
				{
					if ( auto c = std::exchange( handle.promise().continuation, nullptr ) )
						c();
				}
				void await_resume() noexcept {}
			};
			template<typename V>
			yield_awaitable yield_value( V&& value )
			{
				current.emplace( std::forward<V>( value ) );
				return {};
			}
			generator get_return_object() { return *this; }
			suspend_always initial_suspend() { return {}; }
			yield_awaitable final_suspend() noexcept { return {}; }
			void return_void() {}
			void unhandled_exception() {}
		};

		struct iterator
		{
			coroutine_handle<promise_type> handle = {};
			
			iterator& operator++()
			{
				handle.resume();
				if ( handle.done() )
					handle = nullptr;
				return *this;
			}
			bool operator==( const iterator& rhs ) const { return handle == rhs.handle; }
			bool operator!=( const iterator& rhs ) const { return handle != rhs.handle; }
			T&& operator*() const { return *std::move( handle.promise().current ); }
		};

		iterator begin() { return finished() ? end() : ++iterator{ handle }; }
		iterator end() { return {}; }

		bool finished() const { return handle.done(); }
		std::optional<T> operator()()
		{ 
			if ( finished() )
				return std::nullopt;
			handle.resume(); 
			return std::move( handle.promise().current ); 
		}

		unique_coroutine<promise_type> handle;
		generator( promise_type& pr ) : handle( pr ) {}

		// Make awaitable for symetric generators.
		//
		bool await_ready() { return finished(); }
		coroutine_handle<> await_suspend( coroutine_handle<> h )
		{ 
			handle.promise().continuation = h;
			return handle.get();
		}
		std::optional<T> await_resume() 
		{
			if ( finished() )
				return std::nullopt;
			else
				return std::move( handle.promise().current );
		}
	};
};