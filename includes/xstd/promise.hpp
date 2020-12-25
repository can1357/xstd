// Copyright (c) 2020, Can Boluk
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
#pragma once
#include "formatting.hpp"
#include "type_helpers.hpp"
#include "assert.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <exception>
#include <functional>
#include <list>

namespace xstd
{
	namespace impl
	{
#if XSTD_NO_EXCEPTIONS
		using default_exception_type = std::monostate;
#else
		using default_exception_type = std::exception;
#endif
	};

	// Declare the forwarded types.
	//
	template<typename T = void, typename E = impl::default_exception_type>
	struct promise_base;
	template<typename T = void, typename E = impl::default_exception_type>
	using promise = std::shared_ptr<promise_base<T, E>>;

	// Declare the storage of promises.
	//
	template<typename T, typename E>
	struct promise_base : std::enable_shared_from_this<promise_base<T, E>>
	{
		// Value traits.
		//
		static constexpr bool has_value =     !std::is_void_v<T>;
		static constexpr bool has_exception = !std::is_void_v<E>;

		using value_type =     std::conditional_t<has_value,     T, std::monostate>;
		using exception_type = std::conditional_t<has_exception, E, std::monostate>;
		using store_type =     std::variant<value_type, exception_type>;

		// Lock guarding the value, stays locked until the promise is resolved/rejected.
		//
		mutable std::shared_mutex value_lock;

		// Resulting value or the exception.
		//
		std::optional<store_type> value;

		// List of callbacks guarded by another mutex.
		//
		std::mutex callback_lock;
		std::list<std::function<void( const value_type& )>> success_callbacks;
		std::list<std::function<void( const exception_type& )>> fail_callbacks;

		// Lock the promise on pending value construction.
		//
		promise_base() 
		{ 
			value_lock.lock(); 
		}

		// Construction of immediate values.
		//
		template<typename... Tx>
		promise_base( Tx&&... params, std::true_type = {} )
		{
			value.emplace( store_type( std::in_place_index_t<0>, std::forward<Tx>( result )... ) );
		}
		template<typename... Tx>
		promise_base( Tx&&... params, std::false_type = {} )
		{
			value.emplace( store_type( std::in_place_index_t<1>, std::forward<Tx>( result )... ) );
		}

		// Unlock the promise on deconstruction if not completed.
		//
		~promise_base()
		{
			if ( !value.has_value() )
				value_lock.unlock();
		}

		// Resolution of the promise value.
		//
		template<typename... Tx>
		void resolve( Tx&&... result )
		{
			// Make sure the value is not set.
			//
			fassert( !value.has_value() );

			// Emplace the value and unlock the mutex.
			//
			value.emplace( store_type( std::in_place_index_t<0>{}, std::forward<Tx>( result )... ) );
			const value_type& ref = std::get<0>( value.value() );
			value_lock.unlock();

			// Swap the chain of interest with empty list and clear the other.
			//
			callback_lock.lock();
			auto list = std::exchange( success_callbacks, {} );
			fail_callbacks.clear();
			callback_lock.unlock();
			
			// Invoke all callbacks.
			//
			for ( auto& cb : list )
				cb( ref );
		}
		template<typename... Tx>
		void reject( Tx&&... result )
		{
			// Make sure the value is not set.
			//
			fassert( !value.has_value() );

			// Emplace the exception and unlock the mutex.
			//
			value.emplace( store_type( std::in_place_index_t<1>{}, std::forward<Tx>( result )... ) );
			const exception_type& ref = std::get<1>( value.value() );
			value_lock.unlock();

			// Swap the chain of interest with empty list and clear the other.
			//
			callback_lock.lock();
			auto list = std::exchange( fail_callbacks, {} );
			success_callbacks.clear();
			callback_lock.unlock();
			
			// Invoke all callbacks.
			//
			for ( auto& cb : list )
				cb( ref );
		}

		// Exposes information on the promise state.
		//
		bool waiting() const { return !value.has_value(); }
		bool finished() const { return value.has_value(); }
		bool fullfilled() const
		{
			if ( finished() )
			{
				value_lock.lock_shared();
				value_lock.unlock_shared();
				return value->index() == 0;
			}
			return false;
		}
		bool failed() const
		{
			if ( finished() )
			{
				value_lock.lock_shared();
				value_lock.unlock_shared();
				return value->index() == 1;
			}
			return false;
		}
		const value_type& get_value() const
		{
			fassert_s( fullfilled() );
			return std::get<0>( *ref );
		}
		const exception_type& get_exception() const
		{
			fassert_s( failed() );
			return std::get<1>( *ref );
		}

		// Waits for the value and returns the result, throws if an error occurred.
		//
		const value_type& await() const
		{
			value_lock.lock_shared();
			value_lock.unlock_shared();
			const auto& ref = *value;

			if ( ref.index() == 0 )
			{
				return std::get<0>( ref );
			}
			else
			{
#if XSTD_NO_EXCEPTIONS
				error( XSTD_ESTR( "Awaiting on rejected promise: %s" ), std::get<1>( ref ) );
#else
				throw std::get<1>( ref );
#endif
			}
			unreachable();
		}

		// Chaining of promise types.
		//
		template<typename F, bool S>
		auto chain_promise( F&& functor )
		{
			auto& cb_list = [ & ] () -> auto& {
				if constexpr ( S )
					return success_callbacks;
				else
					return fail_callbacks;
			}();
			using ref_type = std::conditional_t<S, const value_type&, const exception_type&>;
			constexpr size_t store_index = S ? 0 : 1;

			// If function can be invoked with void type:
			//
			if constexpr ( InvocableWith<F> )
			{
				using ret_t = decltype( functor() );

				// Create another promise.
				//
				auto chain = std::make_shared<promise_base<ret_t>>();

				// Declare the callback type.
				//
				auto cb = [ chain, fn = std::move( functor ), ref = this->shared_from_this() ]( ref_type val )
				{
					auto resolver = [ & ] ()
					{
						if constexpr ( std::is_void_v<ret_t> )
						{
							fn();
							chain->resolve();
						}
						else
						{
							chain->resolve( fn() );
						}
					};

#if XSTD_NO_EXCEPTIONS
					resolver();
#else
					try
					{
						resolver();
					}
					catch ( std::exception ex )
					{
						chain->reject( ex );
					}
#endif
				};

				// If value is already resolved, immediately invoke or discard the callback.
				//
				if ( value.has_value() )
				{
					value_lock.lock_shared();
					value_lock.unlock_shared();
					if ( const auto* ptr = std::get_if<store_index>( &*value ) )
						cb( *ptr );
				}
				// Otherwise insert into the callbacks list.
				//
				else
				{
					callback_lock.lock();
					cb_list.emplace_back( std::move( cb ) );
					callback_lock.unlock();
				}

				// Return the chain promise.
				//
				return chain;
			}
			// If function takes the result as an argument:
			//
			else
			{
				using ret_t = decltype( functor( std::declval<ref_type>() ) );

				// Create another promise.
				//
				auto chain = std::make_shared<promise_base<ret_t>>();

				// Declare the callback type.
				//
				auto cb = [ chain, fn = std::move( functor ), ref = this->shared_from_this() ]( ref_type val )
				{
					auto resolver = [ & ] ()
					{
						if constexpr ( std::is_void_v<ret_t> )
						{
							fn( val );
							chain->resolve();
						}
						else
						{
							chain->resolve( fn( val ) );
						}
					};

#if XSTD_NO_EXCEPTIONS
					resolver();
#else
					try
					{
						resolver();
					}
					catch ( std::exception ex )
					{
						chain->reject( ex );
					}
#endif
				};

				// If value is already resolved, immediately invoke or discard the callback.
				//
				if ( value.has_value() )
				{
					value_lock.lock_shared();
					value_lock.unlock_shared();
					if( const auto* ptr = std::get_if<store_index>( &*value ) )
						cb( *ptr );
				}
				// Otherwise insert into the callbacks list.
				//
				else
				{
					callback_lock.lock();
					cb_list.emplace_back( std::move( cb ) );
					callback_lock.unlock();
				}

				// Return the chain promise.
				//
				return chain;
			}
		}
		template<typename F> auto then( F&& functor ) { return chain_promise<F, true>( std::forward<F>( functor ) ); }
		template<typename F> auto except( F&& functor ) { return std::pair{ this->shared_from_this(), chain_promise<F, false>( std::forward<F>( functor ) ) }; }

		// String conversion.
		//
		std::string to_string() const
		{
			if ( waiting() )
				return XSTD_STR( "Pending" );
			else if ( fullfilled() )
				return xstd::fmt::as_string( get_value() );
			else
				return xstd::fmt::str( XSTD_STR( "Failed: %s" ), get_exception() );
		}
	};

	// Declare the promise creation wrapper.
	//
	template<typename T = void, typename E = impl::default_exception_type, typename... Tx>
	inline promise<T, E> make_promise( Tx&&... args )
	{
		return std::make_shared<promise_base<T, E>>( std::forward<Tx>( args )... );
	}
};