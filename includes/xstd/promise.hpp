#pragma once
#include <atomic>
#include <memory>
#include <functional>
#include "intrinsics.hpp"
#include "type_helpers.hpp"
#include "formatting.hpp"
#include "assert.hpp"
#include "result.hpp"
#include "event.hpp"
#include "time.hpp"
#include "spinlock.hpp"

// [[Configuration]]
// XSTD_NO_AWAIT: If set disables the macro-based await keyword.
//
#ifndef XSTD_NO_AWAIT
	#define XSTD_NO_AWAIT 0
#endif
namespace xstd
{
	// Declare the forwarded types.
	//
	template<typename T = std::monostate, template<typename> typename R = result>
	struct promise_base;
	template<typename T = std::monostate, template<typename> typename R = result>
	using promise = std::shared_ptr<promise_base<T, R>>;

	// Declare the base promise implementation.
	//
	template<typename T, template<typename> typename R>
	struct promise_base : std::enable_shared_from_this<promise_base<T, R>>
	{
		// Promise traits.
		//
		using value_type =     typename R<T>::value_type;
		using status_type =    typename R<T>::status_type;
		using store_type =     R<T>;
		using result_traits =  typename store_type::traits;
		using callback_type =  std::function<void( const store_type& )>;
		using waiter_type =    std::function<void( store_type&, duration )>;
		static constexpr bool has_value = !std::is_same_v<value_type, std::monostate>;
		
		// List of callbacks guarded by a spinlock.
		//
		spinlock cb_lock = {};
		std::vector<callback_type> cb_list = {};

		// Resulting value or the status, constant after flag is set.
		//
		store_type result = {};
		
		// Event for when the store gets set.
		//
		event_base evt = {};
		mutable std::atomic<bool> preevent_flag = false;
		mutable waiter_type waiter = {};
		mutable spinlock waiter_lock = {};

		// Default constructor makes a pending promise.
		//
		promise_base() {}

		// Constructs a promise with a custom on-wait event.
		//
		promise_base( waiter_type waiter ) : waiter( std::move( waiter ) ) {}
		
		// Constructs a promise with a fixed value.
		//
		promise_base( store_type result ) : result( std::move( result ) ) { evt.notify(); }

		// Constructs a promise that waits for another promise and then transforms the value.
		//
		template<typename T2, template<typename> typename R2, typename F>
		promise_base( const promise<T2, R2>& pr, F&& transform )
		{
			waiter = [ transform = std::forward<F>( transform ), pr ] ( auto& result, duration time )
			{
				if constexpr ( std::is_void_v<decltype( transform( pr->wait() ) )> )
				{
					const auto& result2 = pr->wait( time );
					transform( result2 );

					if ( result2.success() )
					{
						result.emplace( value_type{}, status_type{ result_traits::success_value } );
					}
					else
					{
						if constexpr ( std::is_same_v<status_type, std::string> )
							result.emplace( value_type{}, result2.message() );
						if ( !result.fail() )
							result.status = status_type{ result_traits::failure_value };
					}
				}
				else
				{
					result = transform( pr->wait( time ) );
				}
			};
		}
		template<typename T2, template<typename> typename R2>
		promise_base( const promise<T2, R2>& pr )
		{
			waiter = [ pr ] ( auto& result, duration time )
			{
				const auto& result2 = pr->wait( time );
				if constexpr ( std::is_same_v<typename R<T>::status_type, typename R2<T2>::status_type> )
				{
					result.emplace( value_type{}, result2.status );
				}
				else
				{
					if ( result2.success() )
					{
						result.emplace( value_type{}, status_type{ result_traits::success_value } );
					}
					else
					{
						if constexpr ( std::is_same_v<status_type, std::string> )
							result.emplace( value_type{}, result2.message() );
						if ( !result.fail() )
							result.status = status_type{ result_traits::failure_value };
					}
				}
			};
		}
		
		// No copy allowed, default move.
		//
		promise_base( promise_base&& ) noexcept = default;
		promise_base& operator=( promise_base&& ) noexcept = default;
		promise_base( const promise_base& ) = delete;
		promise_base& operator=( const promise_base& ) = delete;

		// Invokes all callbacks after the value is resolved, internal use.
		//
		void signal()
		{
			dassert_s( evt.notify() );

			// Swap the callback-list with an empty one within a lock.
			//
			cb_lock.lock();
			auto list = std::exchange( cb_list, {} );
			cb_lock.unlock();

			// Invoke all callbacks.
			//
			for ( auto& cb : list )
				cb( result );
		}
		
		// Waits for the event to be complete.
		//
		const store_type& wait( duration time = {} ) const
		{
			if ( waiter )
			{
				if ( waiter_lock.try_lock() )
				{
					if ( !preevent_flag.exchange( true ) )
					{
						std::move( waiter )( const_cast< store_type& >( result ), time );
						const_cast< promise_base* >( this )->signal();
						return result;
					}
				}
			}
			if ( time != duration{} )
			{
				if ( !evt.wait_for( time ) )
				{
					if constexpr ( std::is_same_v<status_type, xstd::exception> )
					{
						static constexpr auto store = xstd::exception{ XSTD_ESTR( "Promise timed out." ) };
						return store;
					}
					else
					{
						return xstd::static_default{};
					}
				}
			}
			evt.wait();
			return result;
		}
		const store_type& wait_for( duration time ) const { return wait( time ); }

		// Resolution of the promise value.
		//
		template<typename... Tx>
		bool resolve( store_type value )
		{
			if ( preevent_flag.exchange( true ) )
				return false;
			result = std::move( value );
			signal();
			return true;
		}
		template<typename... Tx>
		bool resolve( Tx&&... value )
		{
			if ( preevent_flag.exchange( true ) )
				return false;
			result.emplace(
				value_type{ std::forward<Tx>( value )... },
				status_type{ result_traits::success_value }
			);
			signal();
			return true;
		}
		template<typename... Tx>
		bool reject( Tx&&... value )
		{
			if ( preevent_flag.exchange( true ) )
				return false;
			status_type error{ std::forward<Tx>( value )... };
			if ( result_traits::is_success( error ) )
				error = result_traits::failure_value;
			result.status = std::move( error );
			signal();
			return true;
		}

		// Exposes information on the promise state.
		//
		bool finished() const { return evt.signalled(); }
		bool pending() const { return !finished(); }
		bool fulfilled() const { return finished() && result.success(); }
		bool failed() const { return finished() && result.fail(); }

		// Waits for the value / exception and returns the result, throws if an unexpected result is found.
		//
		const value_type& value() const
		{
			return *wait();
		}
		const status_type& status() const
		{
			return wait().status;
		}

		// Promise callback chaining.
		//
		template<typename F>
		void chain( F&& cb )
		{
			if ( !finished() )
			{
				std::lock_guard _g{ cb_lock };
				if ( !finished() )
				{
					cb_list.emplace_back( std::forward<F>( cb ) );
					return;
				}
			}
			cb( result );
		}
		template<typename F> requires ( Invocable<F, void, const value_type&> )
		void then( F&& cb )
		{
			chain( [ cb = std::forward<F>( cb ) ]( const store_type& result ) mutable
			{
				if ( result.success() )
					cb( result.value() );
			} );
		}
		template<typename F> requires ( Invocable<F, void> && !Invocable<F, void, const value_type&> )
		void then( F&& cb )
		{
			chain( [ cb = std::forward<F>( cb ) ]( const store_type& result ) mutable
			{
				if ( result.success() )
					cb();
			} );
		}
		template<typename F>
		void except( F&& cb )
		{
			chain( [ cb = std::forward<F>( cb ) ]( const store_type& result ) mutable
			{
				if ( result.fail() )
					cb( result.status );
			} );
		}

		// String conversion.
		//
		std::string to_string() const
		{
			if ( pending() )
				return std::string{ "(Pending)" };
			else if ( fulfilled() )
				return fmt::str( "(Fulfilled='%s')", fmt::as_string( result.value() ) );
			else
				return fmt::str( "(Rejected='%s')", result.message() );
		}

		// Reject the promise on deconstruction if not completed.
		//
		~promise_base() { reject(); }
	};

	// Declare the promise creation wrappers.
	//
	template<typename T = std::monostate, template<typename> typename R = result, typename... Tx>
	inline promise<T, R> make_promise( Tx&&... args )
	{
		return std::make_shared<promise_base<T, R>>( std::forward<Tx>( args )... );
	}
	template<typename T = std::monostate, template<typename> typename R = result, typename... Tx>
	inline promise<T, R> make_rejected_promise( Tx&&... status )
	{
		promise<T, R> pr = make_promise<T, R>();
		pr->reject( std::forward<Tx>( status )... );
		return pr;
	}
	template<typename T = std::monostate, template<typename> typename R = result, typename... Tx>
	inline promise<T, R> make_resolved_promise( Tx&&... value )
	{
		promise<T, R> pr = make_promise<T, R>();
		pr->resolve( std::forward<Tx>( value )... );
		return pr;
	}
};
#if !XSTD_NO_AWAIT
	struct __async_await_t
	{
		template<typename T>
		__forceinline const auto& operator<=( const xstd::promise<T>& pr ) const
		{
			return pr->wait();
		}
		__forceinline void operator<=( const xstd::event& evt ) const
		{
			return evt->wait();
		}
	};
	struct __async_await_for_t
	{
		xstd::duration duration;

		template<typename T>
		__forceinline const auto& operator<=( const xstd::promise<T>& pr ) const
		{
			return pr->wait_for( duration );
		}
		__forceinline bool operator<=( const xstd::event& evt ) const
		{
			return evt->wait_for( duration );
		}
	};
	#define await         __async_await_t{} <=       // const R<T>&
	#define await_for(x)  __async_await_for_t{x} <=  // const R<T>&
#endif