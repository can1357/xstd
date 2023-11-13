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
			~promise_type() {
				dassert( !continuation ); // Should not leak continuation!
				if ( done )
					std::destroy_at( &value );
			}

			struct final_awaitable
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
				inline void await_resume() const noexcept {}
			};

			task<T, S> get_return_object() { return *this; }
			suspend_always initial_suspend() noexcept { return {}; }
			final_awaitable final_suspend() noexcept { return {}; }
			XSTDC_UNHANDLED_RETHROW;
			
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
				return {};
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
		
		auto& status() & { fassert( finished() ); return handle.promise().value.status; }
		auto&& status() && { fassert( finished() ); return std::move( handle.promise().value.status ); }
		const auto& status() const & { fassert( finished() ); return handle.promise().value.status; }

		// Resumes the coroutine and returns the status.
		//
		bool operator()() { if ( !finished() ) handle.resume(); return finished(); }
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

			inline promise_type() {}
			inline ~promise_type()
			{
				dassert( !continuation ); // Should not leak continuation!
				if ( done )
					std::destroy_at( &value );
			}
			
			struct final_awaitable
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
				inline void await_resume() const noexcept {}
			};

			inline task<void, S> get_return_object() { return *this; }
			inline suspend_always initial_suspend() noexcept { return {}; }
			inline final_awaitable final_suspend() noexcept { return {}; }
			XSTDC_UNHANDLED_RETHROW;
			
			inline void return_void() 
			{
				std::construct_at( &value, in_place_success_t{}, std::monostate{} );
				done = true;
			}
			template<typename V> 
			inline final_awaitable yield_value( V&& v )
			{
				std::construct_at( &value, in_place_failure_t{}, std::forward<V>( v ) );
				done = true;
				return {};
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
		
		auto& status() & { fassert( finished() ); return handle.promise().value.status; }
		auto&& status() && { fassert( finished() ); return std::move( handle.promise().value.status ); }
		const auto& status() const & { fassert( finished() ); return handle.promise().value.status; }

		// Resumes the coroutine and returns the status.
		//
		bool operator()() { if ( !finished() ) handle.resume(); return finished(); }
	};

	// Make task references co awaitable.
	//
	template<typename T, typename S>
	inline auto operator co_await( const task<T, S>& ref ) {
		using promise_type = typename task<T, S>::promise_type;
		struct awaitable {
			coroutine_handle<promise_type> handle;
			inline bool await_ready() { return handle.promise().done; }
			inline coroutine_handle<> await_suspend( coroutine_handle<> hnd ) {
				auto& cont = handle.promise().continuation;
				dassert( cont == nullptr );
				cont = hnd;
				return handle;
			}
			inline const auto& await_resume() const	{
				if constexpr ( !basic_result<T, S>::has_status && basic_result<T, S>::has_value )
					return handle.promise().value.value();
				else
					return handle.promise().value;
			}
		};
		return awaitable{ ref.handle };
	}
	template<typename T, typename S>
	inline auto operator co_await( task<T, S>&& ref ) {
		using promise_type = typename task<T, S>::promise_type;
		struct awaitable {
			coroutine_handle<promise_type> handle;
			inline bool await_ready() { return handle.promise().done; }
			inline coroutine_handle<> await_suspend( coroutine_handle<> hnd ) {
				auto& cont = handle.promise().continuation;
				dassert( cont == nullptr );
				cont = hnd;
				return handle;
			}
			inline auto&& await_resume() const {
				if constexpr ( !basic_result<T, S>::has_status && basic_result<T, S>::has_value )
					return std::move( handle.promise().value.value() );
				else
					return std::move( handle.promise().value );
			}
		};
		return awaitable{ ref.handle };
	}
};