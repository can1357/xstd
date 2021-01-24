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
	namespace impl
	{
		// Wraps a callback so that it can:
		// - 1) Be invoked with arguments even if it accepts none.
		// - 2) Return a value (std::monostate) even if it originally returned void.
		//
		template<typename Fx, typename... Args>
		static constexpr auto wrap_callback( Fx&& fn )
		{
			if constexpr ( InvocableWith<Fx, Args...> )
			{
				if constexpr ( std::is_void_v<decltype( fn( std::declval<Args>()... ) )> )
				{
					return [ fn = std::forward<Fx>( fn ) ] <typename... Tx> ( Tx&&... args )
					{
						fn( std::forward<Tx>( args )... );
						return std::monostate{};
					};
				}
				else
				{
					return [ fn = std::forward<Fx>( fn ) ] <typename... Tx> ( Tx&&... args )
					{
						return fn( std::forward<Tx>( args )... );
					};
				}
			}
			else
			{
				if constexpr ( std::is_void_v<decltype( fn() )> )
				{
					return [ fn = std::forward<Fx>( fn ) ] ( auto&&... )
					{
						fn();
						return std::monostate{};
					};
				}
				else
				{
					return [ fn = std::forward<Fx>( fn ) ] ( auto&&... )
					{
						return fn();
					};
				}
			}
		}
	};

	// Declare the forwarded types.
	//
	template<typename T = void>
	struct promise_store;
	template<typename T = void>
	using promise = std::shared_ptr<promise_store<T>>;
	template<typename T = void>
	using promise_trigger = std::function<void( promise_store<T>* )>;

	// Declare the storage of promises.
	//
	template<typename T>
	struct promise_store : std::enable_shared_from_this<promise_store<T>>
	{
		// Promise traits.
		//
		static constexpr bool has_value = !std::is_void_v<T>;
		using value_type =   std::conditional_t<has_value, T, std::monostate>;
		using store_type =   std::variant<value_type, std::exception>;
		using fn_success =   std::function<void( const value_type& )>;
		using fn_failure =   std::function<void( const std::exception& )>;
		using cb_pair_type = std::pair<fn_success, fn_failure>;

		// If relevant, reference to parent.
		//
		std::weak_ptr<void> parent;

		// List of callbacks guarded by another mutex.
		//
		std::mutex callback_lock;
		std::vector<cb_pair_type> callbacks;

		// Resulting value or the exception, constant after flag is set.
		//
		std::atomic<bool> value_set_flag = false;
		std::atomic<bool> value_flag = false;
		std::optional<store_type> value;

		// A special trigger event guarded by an atomic flag, gets invoked upon any await/::wait use.
		//
		mutable std::atomic<bool> trigger_flag = true;
		mutable promise_trigger<T> trigger = {};

		// Construction of pending or deferred values.
		//
		promise_store( promise_trigger<T> trigger = {} ) : trigger( std::move( trigger ) ) {}

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
		
		// Reject the promise on deconstruction if not completed.
		//
		~promise_store()
		{
			signal();
			if ( !value.has_value() )
				reject();
		}

		// Resolution of the promise value.
		//
		template<typename... Tx>
		void resolve( Tx&&... result )
		{
			// Ignore if value is already decided.
			//
			if ( value_set_flag.exchange( true ) )
				return;

			// Emplace the value and set the flag.
			//
			value.emplace( store_type( std::in_place_index_t<0>{}, std::forward<Tx>( result )... ) );
			const value_type& ref = std::get<0>( value.value() );
			if( value_flag.exchange( true ) )
				xstd::error( XSTD_ESTR( "Promise resolved multiple times." ) );

			// Swap the callback-list with an empty one within a lock.
			//
			callback_lock.lock();
			auto list = std::exchange( callbacks, {} );
			callback_lock.unlock();
			
			// Invoke all callbacks.
			//
			for ( auto& [cb, _] : list )
				cb( ref );
		}
		void reject( std::exception ex )
		{
			// Ignore if value is already decided.
			//
			if ( value_set_flag.exchange( true ) )
				return;

			// Emplace the exception and set the flag.
			//
			value.emplace( store_type( std::in_place_index_t<1>{}, std::move( ex ) ) );
			const std::exception& ref = std::get<1>( value.value() );
			if ( value_flag.exchange( true ) )
				xstd::error( XSTD_ESTR( "Promise resolved multiple times." ) );

			// Swap the callback-list with an empty one within a lock.
			//
			callback_lock.lock();
			auto list = std::exchange( callbacks, {} );
			callback_lock.unlock();
			
			// Invoke all callbacks.
			//
			for ( auto& [_, cb] : list )
				cb( ref );
		}
		void reject( const std::string& str = XSTD_ESTR( "Dropped promise." ) ) { reject( std::runtime_error{ str } ); }
		template<typename... Tx> void reject( const char* fmt, Tx&&... params ) { reject( fmt::str( fmt, std::forward<Tx>( params )... ) ); }

		// Waits for the value flag to be lifted assuming that value is already not std::nullopt, waiting for the full construction.
		//
		const store_type& poll() const
		{
			while ( !value_flag.load() )
				yield_cpu();
			return *value;
		}

		// When a user needs to query the state of the promise, trigger gets called to invoke the deferred routine.
		//
		void signal() const
		{
			if ( trigger_flag.exchange( false ) )
			{
				if ( auto fn = std::move( trigger ) )
					fn( make_mutable( this ) );
			}
		}

		// Exposes information on the promise state.
		//
		bool waiting() const { return !value_flag.load(); }
		bool finished() const { return value_flag.load(); }
		bool fulfilled() const { return finished() && poll().index() == 0; }
		bool failed() const { return finished() && poll().index() == 1; }

		// Waits for the completion of the promise.
		//
		auto wait() const
		{
			signal();
			poll();
			return this;
		}

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
			// Declare helpers.
			//
			using ref_type = std::conditional_t<S, const value_type&, const std::exception&>;
			constexpr size_t store_index = S ? 0 : 1;
			auto add_cb = [ & ] <typename Ts, typename Tf> ( Ts&& s, Tf&& f )
			{
				callback_lock.lock();
				if ( value_flag.load() )
				{
					callback_lock.unlock();

					auto& ref = *value;
					if ( const auto* ptr = std::get_if<store_index>( &ref ) )
						s( *ptr );
					else
						f( std::get<1 - store_index>( ref ) );
				}
				else
				{
					if constexpr ( S )
						callbacks.emplace_back( std::forward<Ts>( s ), std::forward<Tf>( f ) );
					else
						callbacks.emplace_back( std::forward<Tf>( f ), std::forward<Ts>( s ) );
					callback_lock.unlock();
				}
			};

			// Create another promise.
			//
			auto wrapped = impl::wrap_callback<F, ref_type>( std::forward<F>( functor ) );
			using ret_t = decltype( wrapped( std::declval<ref_type>() ) );
			auto chain = std::make_shared<promise_store<std::conditional_t<std::is_same_v<std::monostate, ret_t>, void, ret_t>>>();
			chain->parent = this->weak_from_this();

			// Declare the callback types and add the pair to the list (or immediately invoke picked instance).
			//
			auto cb = [ =, fn = std::move( wrapped ) ]( ref_type val )
			{
				chain->resolve( fn( val ) );
			};
			auto rcb = [ chain = chain.get() ] ( auto&& ex ) // Holds raw pointer, cb will reference for us anyway.
			{
				if constexpr ( S )
					chain->reject( ex );
				else
					chain->reject();
			};
			add_cb( std::move( cb ), std::move( rcb ) );

			// Return the chain promise.
			//
			return chain;
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
	// - All rejects the whole chain if one fails or returns const T&.
	// - Any resolves as much as possible and returns const T*, possibly null if an exception was involved.
	//
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
	template<typename... Tp>
	inline auto any( const promise<Tp>&... promises )
	{
		// Declare the resulting tuple's type.
		//
		using Tr = std::tuple<std::add_const_t<typename promise_store<Tp>::value_type>*...>;
		
		// Create a promise.
		//
		return make_promise<Tr>( [ = ] ( promise_store<Tr>* result )
		{
			auto wait_for = [ & ] ( auto& pr )
			{
				pr->wait();
				return pr->fulfilled() ? &pr->get_value() : nullptr;
			};
			result->resolve( Tr{ wait_for( promises )... } );
		} );
	}

	// If void promise, can chain using operator+= / operator+.
	//
	inline promise<> operator+( promise<> a, promise<> b )
	{
		return make_promise( [ a = std::move( a ), b = std::move( b ) ] ( promise_store<>* result )
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
			if ( wait_for( a ) && wait_for( b ) )
				result->resolve();
		} );
	}
	inline promise<>& operator+=( promise<>& a, promise<> b )
	{
		return a = operator+( std::move( a ), std::move( b ) );
	}
};
#if !XSTD_NO_AWAIT
	struct __async_await_t
	{
		template<typename T>
		auto operator<<( const xstd::promise<T>& pr ) const ->
			std::add_const_t<typename xstd::promise_store<T>::value_type>&
		{
			pr->wait();
			return pr->get_value();
		}
		template<typename T>
		auto operator<=( const xstd::promise<T>& pr ) const ->
			std::optional<typename xstd::promise_store<T>::value_type>
		{
			pr->wait();
			if ( pr->fulfilled() )
				return pr->get_value();
			else
				return std::nullopt;
		}
	};
	#define await      __async_await_t{} <<  // const T&         || => exception
	#define opt_await  __async_await_t{} <=  // std::optional<T> || std::nullopt
#endif