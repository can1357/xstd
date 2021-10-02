#pragma once
#include "coro.hpp"
#include "result.hpp"
#include "formatting.hpp"

namespace xstd
{
	// Unique task coroutine.
	//
	template<typename T = void, typename S = xstd::exception>
	struct task
	{
		struct promise_type
		{
			// Continuation coroutine.
			//
			coroutine_handle<> continuation = {};

			// Result value.
			//
			union
			{
				basic_result<T, S> value;
				uint8_t dummy = 0;
			};
			bool done = false;

			promise_type() {}
			~promise_type() 
			{
				if ( done ) 
					std::destroy_at( &value ); 
			}

			struct final_awaitable
			{
				bool has_continuation = false;
				bool await_ready() noexcept { return !has_continuation; }
				coroutine_handle<> await_suspend( coroutine_handle<promise_type> hnd ) noexcept { return hnd.promise().continuation; }
				void await_suspend( coroutine_handle<> ) noexcept {};
				void await_resume() const noexcept {}
			};

			task<T, S> get_return_object() { return *this; }
			suspend_always initial_suspend() noexcept { return {}; }
			final_awaitable final_suspend() noexcept { return { continuation != nullptr }; }
			void unhandled_exception() {}
			
			template<typename V> 
			void return_value( V&& v ) 
			{
				std::construct_at( &value, in_place_success_t{}, std::forward<V>( v ) );
				done = true;
			}
			template<typename V> 
			final_awaitable yield_value( V&& v )
			{
				std::construct_at( &value, in_place_failure_t{}, std::forward<V>( v ) );
				done = true;
				return { continuation != nullptr };
			}
		};

		// Coroutine handle and the internal constructor.
		//
		unique_coroutine<promise_type> handle;
		task( promise_type& pr ) : handle( pr ) {}

		// Null constructor and validity check.
		//
		task() : handle( nullptr ) {}
		task( std::nullptr_t ) : handle( nullptr ) {}
		bool valid() const { return handle.get() != nullptr; }
		explicit operator bool() const { return valid(); }

		// No copy, default move.
		//
		task( task&& ) noexcept = default;
		task( const task& ) = delete;
		task& operator=( task&& ) noexcept = default;
		task& operator=( const task& ) = delete;

		// State.
		//
		bool finished() const { return handle.promise().done; }
		bool pending() const { return !finished(); }
		bool failed() const { return finished() && handle.promise().value.fail(); }
		bool fulfilled() const { return finished() && handle.promise().value.success(); }
		
		basic_result<T, S>& result() & { fassert( finished() ); return handle.promise().value; }
		const basic_result<T, S>& result() const & { fassert( finished() ); return handle.promise().value; }
		basic_result<T, S>&& result() && { fassert( finished() ); return std::move( handle.promise().value ); }

		T& value() & { fassert( finished() ); return *handle.promise().value; }
		T&& value() && { fassert( finished() ); return std::move( *handle.promise().value ); }
		const T& value() const & { fassert( finished() ); return *handle.promise().value; }
		
		S& status() & { fassert( finished() ); return handle.promise().value.status; }
		S&& status() && { fassert( finished() ); return std::move( handle.promise().value.status ); }
		const S& status() const & { fassert( finished() ); return handle.promise().value.status; }

		// Resumes the coroutine and returns the status.
		//
		bool operator()() const { if ( !finished() ) handle.resume(); return finished(); }
	};
	template<typename S>
	struct task<void, S>
	{
		struct promise_type
		{
			// Continuation coroutine.
			//
			coroutine_handle<> continuation = {};

			// Result value.
			//
			union
			{
				basic_result<void, S> value;
				uint8_t dummy = 0;
			};
			bool done = false;

			promise_type() {}
			~promise_type() { if ( done ) std::destroy_at( &value ); }
			
			struct final_awaitable
			{
				bool has_continuation = false;
				bool await_ready() noexcept { return !has_continuation; }
				coroutine_handle<> await_suspend( coroutine_handle<promise_type> hnd ) noexcept { return hnd.promise().continuation; }
				void await_suspend( coroutine_handle<> ) noexcept {};
				void await_resume() const noexcept {}
			};

			task<void, S> get_return_object() { return *this; }
			suspend_always initial_suspend() noexcept { return {}; }
			final_awaitable final_suspend() noexcept { return { continuation != nullptr }; }
			void unhandled_exception() {}
			
			void return_void() 
			{
				std::construct_at( &value, in_place_success_t{}, std::monostate{} );
				done = true;
			}
			template<typename V> 
			final_awaitable yield_value( V&& v )
			{
				std::construct_at( &value, in_place_failure_t{}, std::forward<V>( v ) );
				done = true;
				return { continuation != nullptr };
			}
		};

		// Coroutine handle and the internal constructor.
		//
		unique_coroutine<promise_type> handle;
		task( promise_type& pr ) : handle( pr ) {}

		// Null constructor and validity check.
		//
		task() : handle( nullptr ) {}
		task( std::nullptr_t ) : handle( nullptr ) {}
		bool valid() const { return handle.get() != nullptr; }
		explicit operator bool() const { return valid(); }

		// No copy, default move.
		//
		task( task&& ) noexcept = default;
		task( const task& ) = delete;
		task& operator=( task&& ) noexcept = default;
		task& operator=( const task& ) = delete;

		// State.
		//
		bool finished() const { return handle.promise().done; }
		bool pending() const { return !finished(); }
		bool failed() const { return finished() && handle.promise().value.fail(); }
		bool fulfilled() const { return finished() && handle.promise().value.success(); }
		
		basic_result<void, S>& result() & { fassert( finished() ); return handle.promise().value; }
		const basic_result<void, S>& result() const & { fassert( finished() ); return handle.promise().value; }
		basic_result<void, S>&& result() && { fassert( finished() ); return std::move( handle.promise().value ); }

		std::monostate value() { fassert( finished() ); return {}; }
		
		S& status() & { fassert( finished() ); return handle.promise().value.status; }
		S&& status() && { fassert( finished() ); return std::move( handle.promise().value.status ); }
		const S& status() const & { fassert( finished() ); return handle.promise().value.status; }

		// Resumes the coroutine and returns the status.
		//
		bool operator()() const { if ( !finished() ) handle.resume(); return finished(); }
	};

	// Make task references co awaitable.
	//
	template<typename T, typename S>
	auto operator co_await( const task<T, S>& ref )
	{
		struct awaitable
		{
			const xstd::task<T, S>& ref;

			bool await_ready()
			{
				return ref.finished();
			}
			coroutine_handle<> await_suspend( coroutine_handle<> hnd )
			{
				dassert_s( std::exchange( ref.handle.promise().continuation, hnd ) == nullptr );
				return ref.handle;
			}
			const auto& await_resume() const
			{
				if constexpr ( Same<S, no_status> )
					return ref.result().value();
				else
					return ref.result();
			}
		};
		return awaitable{ ref };
	}
	template<typename T, typename S>
	auto operator co_await( task<T, S>&& ref )
	{
		struct awaitable
		{
			xstd::task<T, S> ref;

			bool await_ready()
			{
				return ref.finished();
			}
			coroutine_handle<> await_suspend( coroutine_handle<> hnd )
			{
				dassert_s( std::exchange( ref.handle.promise().continuation, hnd ) == nullptr );
				return ref.handle;
			}
			const auto& await_resume() const
			{
				if constexpr ( Same<S, no_status> )
					return ref.result().value();
				else
					return ref.result();
			}
		};
		return awaitable{ std::move( ref ) };
	}
};