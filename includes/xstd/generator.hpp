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
				inline bool await_ready() noexcept { return false; }

				template<typename P = promise_type>
				inline coroutine_handle<> await_suspend( coroutine_handle<P> handle ) noexcept
				{
					if ( auto c = std::exchange( handle.promise().continuation, nullptr ) )
						return c;
					else
						return noop_coroutine();
				}
				inline void await_resume() noexcept {}
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
			XSTDC_UNHANDLED_RETHROW;
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
		iterator end() const { return {}; }

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
		inline bool await_ready() { return finished(); }
		inline coroutine_handle<> await_suspend( coroutine_handle<> h )
		{ 
			handle.promise().continuation = h;
			return handle.get();
		}
		inline std::optional<T> await_resume()
		{
			if ( finished() )
				return std::nullopt;
			else
				return std::move( handle.promise().current );
		}
	};
};