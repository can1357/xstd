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
#include <stdexcept>
#include <functional>
#include <list>

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
	template<typename T = void>
	struct promise_store;
	template<typename T = void>
	using promise = std::shared_ptr<promise_store<T>>;

	// Declare the storage of promises.
	//
	template<typename T>
	struct promise_store : std::enable_shared_from_this<promise_store<T>>
	{
		// Value traits.
		//
		static constexpr bool has_value = !std::is_void_v<T>;
		using value_type =                std::conditional_t<has_value, T, std::monostate>;
		using store_type =                std::variant<value_type, std::exception>;

		// If relevant, reference to parent.
		//
		std::weak_ptr<void> parent;

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
		std::list<std::function<void( const std::exception& )>> fail_callbacks;

		// A special trigger event guarded by an atomic flag, gets invoked upon any await/::wait use.
		//
		std::atomic<bool> trigger_flag = false;
		std::function<void( promise_store<T>* )> trigger = {};

		// Lock the promise on pending value construction.
		//
		promise_store( std::function<void( promise_store<T>* )> trigger = {} ) : trigger( std::move( trigger ) )
		{ 
			if ( this->trigger )
				trigger_flag = true;
			value_lock.lock(); 
		}

		// Construction of immediate values.
		//
		template<typename... Tx>
		promise_store( Tx&&... params, std::true_type = {} )
		{
			value.emplace( store_type( std::in_place_index_t<0>, std::forward<Tx>( result )... ) );
		}
		template<typename... Tx>
		promise_store( Tx&&... params, std::false_type = {} )
		{
			value.emplace( store_type( std::in_place_index_t<1>, std::forward<Tx>( result )... ) );
		}

		// No copy allowed, default move.
		//
		promise_store( promise_store&& ) noexcept = default;
		promise_store& operator=( promise_store&& ) noexcept = default;
		promise_store( const promise_store& ) = delete;
		promise_store& operator=( const promise_store& ) = delete;
		
		// Unlock the promise on deconstruction if not completed.
		//
		~promise_store()
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
		void reject( std::exception ex )
		{
			// Make sure the value is not set.
			//
			fassert( !value.has_value() );

			// Emplace the exception and unlock the mutex.
			//
			value.emplace( store_type( std::in_place_index_t<1>{}, std::move( ex ) ) );
			const std::exception& ref = std::get<1>( value.value() );
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
		void reject( const std::string& str = XSTD_ESTR( "Dropped promise." ) ) { reject( std::runtime_error{ str } ); }
		template<typename... Tx> void reject( const char* fmt, Tx&&... params ) { reject( fmt::str( fmt, std::forward<Tx>( params )... ) ); }

		// Exposes information on the promise state.
		//
		bool waiting() const { return !value.has_value(); }
		bool finished() const { return value.has_value(); }
		bool fulfilled() const
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

		// Waits for the completion of the promise.
		//
		auto wait()
		{
			if ( trigger_flag.exchange( false ) )
			{
				auto&& fn = std::move( trigger );
				dassert( fn );
				fn( this );
			}
			value_lock.lock_shared();
			value_lock.unlock_shared();
			return this;
		}
		decltype( auto ) wait() const { return make_mutable( this )->wait(); }

		// Waits for the value / exception and returns the result, throws if an unexpected result is found.
		//
		const value_type& get_value() const
		{
			wait();
#if !XSTD_NO_EXCEPTIONS
			if ( value->index() == 1 )
				throw std::get<1>( *value );
#endif
			dassert( fulfilled() );
			return std::get<0>( *value );
		}
		const std::exception& get_exception() const
		{
			wait();
			dassert( failed() );
			return std::get<1>( *value );
		}

		// Chaining of promise types.
		//
		template<typename F, bool S>
		auto chain_promise( F&& functor )
		{
			auto&& [cb_list, rcb_list] = [ & ] () {
				if constexpr ( S )
					return std::pair{ &success_callbacks, &fail_callbacks };
				else
					return std::pair{ &fail_callbacks, &success_callbacks };
			}();

			using ref_type = std::conditional_t<S, const value_type&, const std::exception&>;
			constexpr size_t store_index = S ? 0 : 1;

			// If function can be invoked with void type:
			//
			if constexpr ( InvocableWith<F> )
			{
				using ret_t = decltype( functor() );

				// Create another promise.
				//
				auto chain = std::make_shared<promise_store<ret_t>>();
				chain->parent = this->weak_from_this();

				// Declare the callback types.
				//
				auto cb = [ =, fn = std::move( functor ) ]( ref_type val )
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
				auto rcb = [ = ]( auto&& ex )
				{
					if constexpr ( S )
						chain->reject( ex );
					else
						chain->reject();
				};

				// If value is already resolved, immediately invoke or discard the callback.
				//
				if ( value.has_value() )
				{
					value_lock.lock_shared();
					value_lock.unlock_shared();
					auto& ref = *value;
					if ( const auto* ptr = std::get_if<store_index>( &ref ) )
						cb( *ptr );
					else
						rcb( std::get<1 - store_index>( ref ) );
				}
				// Otherwise insert into the callbacks list.
				//
				else
				{
					callback_lock.lock();
					cb_list->emplace_back( std::move( cb ) );
					rcb_list->emplace_back( std::move( rcb ) );
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
				auto chain = std::make_shared<promise_store<ret_t>>();
				chain->parent = this->weak_from_this();

				// Declare the callback types.
				//
				auto cb = [ =, fn = std::move( functor ) ]( ref_type val )
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
				auto rcb = [ = ]( auto&& ex )
				{
					if constexpr ( S )
						chain->reject( ex );
					else
						chain->reject();
				};

				// If value is already resolved, immediately invoke or discard the callback.
				//
				if ( value.has_value() )
				{
					value_lock.lock_shared();
					value_lock.unlock_shared();
					auto& ref = *value;
					if ( const auto* ptr = std::get_if<store_index>( &ref ) )
						cb( *ptr );
					else
						rcb( std::get<1 - store_index>( ref ) );
				}
				// Otherwise insert into the callbacks list.
				//
				else
				{
					callback_lock.lock();
					cb_list->emplace_back( std::move( cb ) );
					rcb_list->emplace_back( std::move( rcb ) );
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
				return XSTD_STR( "(Pending)" );
			else if ( fulfilled() )
				return fmt::str( XSTD_CSTR( "(Fulfilled='%s')" ), get_value() );
			else
				return fmt::str( XSTD_CSTR( "(Rejected='%s')" ), get_exception() );
		}
	};

	// Declare the promise creation wrappers.
	//
	template<typename T = void, typename... Tx>
	inline promise<T> make_promise( Tx&&... trigger )
	{
		return std::make_shared<promise_store<T>>( std::forward<Tx>( trigger )... );
	}
	template<typename T = void, typename... Tx>
	inline promise<T> make_rejected_promise( Tx&&... exception )
	{
		promise<T> pr = make_promise<T>();
		pr->reject( std::forward<Tx>( exception )... );
		return pr;
	}

	// Promise combination:
	// - Note: Unlike chained ->then | ->catch promises, these instances will be storing dangling values as soon 
	//         as the parent is freed since value is propagated by reference.
	//
	template<typename... Tp>
	inline auto all( const promise<Tp>&... promises )
	{
		// Declare the resulting tuple's type.
		//
		using Tr = std::tuple<std::add_const_t<typename promise_store<Tp>::value_type>&...>;
		
		// Create a promise.
		//
		return make_promise<Tr>( [ = ] ( promise_store<Tr>* result )
		{
			auto wait_for = [ & ] ( auto& pr )
			{
				pr->wait();
				if ( pr->failed() )
				{
					result->reject( pr->get_exception() );
					return false;
				}
				return true;
			};

			bool has_result = ( wait_for( promises ) && ... );
			if ( has_result )
				result->resolve( Tr{ promises->get_value()... } );
		} );
	}
};
#if !XSTD_NO_AWAIT
	struct __async_await_t
	{
		template<typename T>
		decltype( auto ) operator<<( const xstd::promise<T>& pr ) const
		{
			pr->wait();
			return pr->get_value();
		}
	};
	#define await __async_await_t{} <<
#endif