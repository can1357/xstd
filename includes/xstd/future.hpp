#pragma once
#include "result.hpp"
#include "event.hpp"
#include "coro.hpp"
#include "spinlock.hpp"
#include "scope_tpr.hpp"
#include "formatting.hpp"
#include "time.hpp"
#include <vector>

// [[Configuration]]
// XSTD_PROMISE_TASK_PRIORITY: Task priority set when acquiring promise related locks.
//
#ifndef XSTD_PROMISE_TASK_PRIORITY
	#define XSTD_PROMISE_TASK_PRIORITY 2
#endif

namespace xstd
{
	namespace impl
	{
		struct wait_block
		{
			wait_block* volatile next = nullptr;
			xstd::event_base     event = {};
		};

		template<typename T, typename S>
		inline const basic_result<T, S>& timeout_result()
		{
			if constexpr ( Same<S, xstd::exception> )
			{
				static const basic_result<T, S> timeout = xstd::exception{ XSTD_ESTR( "Promise timed out." ) };
				return timeout;
			}
			else if constexpr ( Same<S, no_status> )
			{
				xstd::error( XSTD_ESTR( "Promise timed out." ) );
			}
			else
			{
				return xstd::static_default{};
			}
		}
	};

	// Base of the promise type.
	//
	template<typename T, typename S>
	struct promise_base
	{
		// State flags.
		//
		static constexpr uint8_t  state_finished_bit = 0;
		static constexpr uint8_t  state_written_bit =  1;
		static constexpr uint16_t state_finished =     1 << state_finished_bit;
		static constexpr uint16_t state_written =      1 << state_written_bit;

		// Event and the continuation list.
		//
		mutable impl::wait_block*               events = nullptr;
		mutable std::vector<coroutine_handle<>> continuation = {};

		// Result.
		//
		union
		{
			uint8_t                         dummy = 0;
			basic_result<T, S>              result;
		};

		// Lock guarding continuation list and the event list.
		//
		mutable spinlock                state_lock = {};

		// Promise state.
		//
		std::atomic<uint16_t>           state = { 0 };

		// Reference count.
		//
		mutable std::atomic<uint32_t>   refs = { 0 };

		// Default constructor/destructor.
		//
		promise_base() {}
		~promise_base()
		{
			if ( finished() )
				std::destroy_at( &result );
		}

		// Wait block helpers.
		//
		bool register_wait_block( impl::wait_block& wb ) const
		{
			// Acquire the state lock, if already finished fail.
			//
			scope_tpr<XSTD_PROMISE_TASK_PRIORITY> _t{};
			std::lock_guard _g{ state_lock };
			if ( finished() ) [[unlikely]]
				return false;

			// Link the wait block and return.
			//
			wb.next = events;
			events = &wb;
			return true;
		}
		bool deregister_wait_block( impl::wait_block& wb ) const
		{
			// Acquire the state lock.
			//
			scope_tpr<XSTD_PROMISE_TASK_PRIORITY> _t{};
			std::lock_guard _g{ state_lock };

			// If event is already signalled, return.
			//
			if ( wb.event.signalled() )
				return false;

			// Find the entry before our entry, unlink and return.
			//
			for ( auto it = ( impl::wait_block* ) &events; it; it = it->next )
			{
				if ( it->next == &wb )
				{
					it->next = wb.next;
					break;
				}
			}
			return true;
		}

		// Waits for the event to be complete.
		//
		const basic_result<T, S>& wait() const
		{
			if ( finished() ) [[likely]]
				return unrace();

			// Register a wait block.
			//
			impl::wait_block wb = {};
			if ( !register_wait_block( wb ) )
				return unrace();
			
			// Wait for completion and return the result.
			//
			wb.event.wait();
			return result;
		}
		const basic_result<T, S>& wait_for( duration time ) const
		{
			if ( finished() ) [[likely]]
				return unrace();

			// Register a wait block.
			//
			impl::wait_block wb = {};
			if ( !register_wait_block( wb ) )
				return unrace();

			// Wait on it and then return on success.
			//
			if ( wb.event.wait_for( time ) )
				return result;

			// Deregister the wait block, if we fail return the result.
			//
			if ( !deregister_wait_block( wb ) )
				return result;

			// Return the timeout.
			//
			return impl::timeout_result<T, S>();
		}

		// Chain/unchain a coroutine.
		//
		bool listen( coroutine_handle<> h ) const
		{
			if ( finished() ) [[likely]]
				return false;

			scope_tpr<XSTD_PROMISE_TASK_PRIORITY> _t{};
			std::lock_guard _g{ state_lock };
			if ( finished() ) [[unlikely]]
				return false;

			continuation.emplace_back( std::move( h ) );
			return true;
		}
		bool unlisten( coroutine_handle<> h ) const
		{
			if ( finished() ) [[likely]]
				return false;

			scope_tpr<XSTD_PROMISE_TASK_PRIORITY> _t{};
			std::lock_guard _g{ state_lock };
			if ( finished() ) [[unlikely]]
				return false;

			auto it = std::find( continuation.begin(), continuation.end(), h );
			if ( it != continuation.end() )
			{
				std::swap( *it, continuation.back() );
				continuation.resize( continuation.size() - 1 );
			}
			return true;
		}

		// Signals the event, runs all continuation entries.
		//
		void signal()
		{
			std::vector<coroutine_handle<>> list = {};

			uint8_t prev = get_task_priority();
			{
				scope_tpr<XSTD_PROMISE_TASK_PRIORITY> _t{ prev };
				std::lock_guard _g{ state_lock };

				// Swap the list.
				//
				continuation.swap( list );

				// Notify all events.
				//
				for ( auto it = events; it; it = it->next )
					it->event.notify();
				events = nullptr;
			}

			// Invoke all continuation handles.
			//
			if ( prev )
			{
				for ( auto& cb : list )
					xstd::chore( std::move( cb ) );
			}
			else
			{
				for ( auto& cb : list )
					cb();
			}
		}

		// Resolution of the promise value.
		//
		template<typename... Args> 
		void reject_unchecked( Args&&... args ) 
		{ 
			std::construct_at( &result, in_place_failure_t{}, std::forward<Args>( args )... ); 
			state.store( state_finished | state_written, std::memory_order::relaxed ); 
		}
		template<typename... Args> 
		void resolve_unchecked( Args&&... args ) 
		{ 
			std::construct_at( &result, in_place_success_t{}, std::forward<Args>( args )... ); 
			state.store( state_finished | state_written, std::memory_order::relaxed ); 
		}
		template<typename... Args> 
		void emplace_unchecked( Args&&... args ) 
		{ 
			std::construct_at( &result, std::forward<Args>( args )... ); 
			state.store( state_finished | state_written, std::memory_order::relaxed );
		}
		template<typename... Args> 
		bool reject( Args&&... args )  
		{ 
			if ( xstd::bit_set( state, state_finished_bit ) ) 
				return false; 
			reject_unchecked( std::forward<Args>( args )... ); 
			signal(); 
			return true; 
		}
		template<typename... Args> 
		bool resolve( Args&&... args )
		{ 
			if ( xstd::bit_set( state, state_finished_bit ) ) 
				return false; 
			resolve_unchecked( std::forward<Args>( args )... ); 
			signal(); 
			return true; 
		}
		template<typename... Args> 
		bool emplace( Args&&... args ) 
		{ 
			if ( xstd::bit_set( state, state_finished_bit ) ) 
				return false; 
			emplace_unchecked( std::forward<Args>( args )... ); 
			signal(); 
			return true; 
		}

		// Exposes information on the promise state.
		//
		inline bool finished() const { return state.load( std::memory_order::relaxed ) & state_finished; }
		inline bool pending() const { return !finished(); }
		bool fulfilled() const { return finished() && unrace().success(); }
		bool failed() const { return finished() && unrace().fail(); }

		// Helper to reference the result indirectly without a signal and avoiding the race condition.
		//
		const basic_result<T, S>& unrace() const
		{
			while ( !( state.load( std::memory_order::relaxed ) & state_written ) ) [[unlikely]]
				yield_cpu();
			return result;
		}

		// String conversion.
		//
		std::string to_string() const
		{
			if ( pending() )
				return std::string{ "(Pending)" };

			auto& res = unrace();
			if ( res.success() )
				return fmt::str( "(Fulfilled='%s')", fmt::as_string( res.value() ) );
			else
				return fmt::str( "(Rejected='%s')", res.message() );
		}
	};

	// Promise reference type.
	//
	template<typename T, typename S, bool Owner>
	struct promise_ref
	{
		promise_base<T, S>* ptr = nullptr;

		// Refs : [24 (Future) - 8 (Promise)]
		//
		static constexpr uint32_t future_flag =  0x100;
		static constexpr uint32_t promise_flag = 0x1;
		static constexpr uint32_t ref_flag = Owner ? promise_flag : future_flag;
		COLD static void destroy( promise_base<T, S>* ptr ) { delete ptr; }
		COLD static void break_promise( promise_base<T, S>* ptr ) 
		{
			if constexpr ( Same<S, xstd::exception> )
				ptr->reject( xstd::exception{ XSTD_ESTR( "Promise broken." ) } );
			else if constexpr ( Same<S, no_status> )
				xstd::error( XSTD_ESTR( "Promise broken." ) );
			else
				ptr->reject();
		}
		static void inc_ref( promise_base<T, S>* ptr )
		{
			ptr->refs += ref_flag;
		}
		static void dec_ref( promise_base<T, S>* ptr )
		{
			if ( ptr )
			{
				if constexpr ( Owner )
				{
					// Remove the promise reference and add a future reference.
					//
					uint32_t prev = ptr->refs.fetch_add( future_flag - promise_flag );
					
					// Delete if we were the only reference.
					//
					if ( prev == ref_flag )
						return destroy( ptr );

					// Break promise if we were the only promise.
					//
					if ( ( prev & ( future_flag - 1 ) ) == 1 && !ptr->finished() )
						break_promise( ptr );

					// Remove the temporary future reference.
					//
					if ( ptr->refs.fetch_sub( future_flag ) == future_flag )
						destroy( ptr );
				}
				else
				{
					if ( ptr->refs.fetch_sub( ref_flag ) == ref_flag )
						destroy( ptr );
				}
			}
		}

		// Simple constructors.
		//
		promise_ref() {}
		promise_ref( std::nullptr_t ) {}
		explicit promise_ref( promise_base<T, S>* ptr ) : ptr( ptr ) {}

		// Copy via ref.
		//
		promise_ref( const promise_ref<T, S, Owner>& other )
		{
			ptr = other.ptr;
			if ( ptr )
				inc_ref( ptr );
		}
		promise_ref& operator=( const promise_ref<T, S, Owner>& other )
		{
			auto prev = std::exchange( ptr, other.ptr );
			if ( ptr )
				inc_ref( ptr );
			if ( prev )
				dec_ref( prev );
			return *this;
		}
		promise_ref( const promise_ref<T, S, !Owner>& other )
		{
			ptr = other.ptr;
			if ( ptr )
				inc_ref( ptr );
		}
		promise_ref& operator=( const promise_ref<T, S, !Owner>& other )
		{
			auto prev = std::exchange( ptr, other.ptr );
			if ( ptr )
				inc_ref( ptr );
			if ( prev )
				dec_ref( prev );
			return *this;
		}

		// Move via swap.
		//
		promise_ref( promise_ref<T, S, Owner>&& other ) noexcept
		{
			std::swap( ptr, other.ptr );
		}
		promise_ref& operator=( promise_ref<T, S, Owner>&& other ) noexcept
		{
			std::swap( ptr, other.ptr );
			return *this;
		}

		// Resets the reference.
		//
		inline explicit operator bool() const { return ptr != nullptr; }
		inline void reset() { dec_ref( std::exchange( ptr, nullptr ) ); }
		inline ~promise_ref() { reset(); }

		// Reference observer.
		//
		inline auto* address() const { return ptr; }
		inline size_t ref_count() const 
		{
			if ( !ptr ) return 0;
			auto val = ptr->refs.load();
			return ( val >> 8 ) + ( val & 0xFF );
		}

		// Wrap around the reference.
		//
		inline void unrace() const { ptr->unrace(); }
		inline bool finished() const { return ptr->finished(); }
		inline bool pending() const { return ptr->pending(); }
		inline bool fulfilled() const { return ptr->fulfilled(); }
		inline bool failed() const { return ptr->failed(); }
		inline bool listen( coroutine_handle<> h ) const { return ptr->listen( h ); }
		inline bool unlisten( coroutine_handle<> h ) const { return ptr->unlisten( h ); }
		inline const basic_result<T, S>& wait() const { return ptr->wait(); }
		inline const basic_result<T, S>& wait_for( duration time ) const { return ptr->wait_for( time ); }
		inline std::string to_string() const { return ptr->to_string(); }
		inline const basic_result<T, S>& result() const { fassert( finished() ); return ptr->unrace(); }
		
		template<typename... Tx>
		inline bool resolve( Tx&&... value ) const requires Owner
		{
			return ptr->resolve( std::forward<Tx>( value )... );
		}
		template<typename... Tx>
		inline bool reject( Tx&&... value ) const requires Owner
		{
			return ptr->reject( std::forward<Tx>( value )... );
		}
		template<typename... Tx>
		inline bool emplace( Tx&&... value ) const requires Owner
		{
			return ptr->emplace( std::forward<Tx>( value )... );
		}
		template<typename... Tx>
		inline void resolve_unchecked( Tx&&... value ) const requires Owner
		{
			ptr->resolve_unchecked( std::forward<Tx>( value )... );
		}
		template<typename... Tx>
		inline void reject_unchecked( Tx&&... value ) const requires Owner
		{
			ptr->reject_unchecked( std::forward<Tx>( value )... );
		}
		template<typename... Tx>
		inline void emplace_unchecked( Tx&&... value ) const requires Owner
		{
			ptr->emplace_unchecked( std::forward<Tx>( value )... );
		}
		inline void signal() const requires Owner 
		{ 
			return ptr->signal(); 
		}
	};

	// Moving awaitable wrapper for promise refences, might be unsafe if shared.
	//
	template<typename T = void, typename S = xstd::exception>
	struct unique_future : promise_ref<T, S, false>
	{
		using promise_ref<T, S, false>::promise_ref;

		// No copy, default move.
		//
		unique_future( const unique_future& ) = delete;
		unique_future& operator=( const unique_future& ) = delete;
		unique_future( unique_future&& ) noexcept = default;
		unique_future& operator=( unique_future&& ) noexcept = default;
	};

	struct make_awaitable_tag_t {};

	// Free-standing promise type.
	//
	template<typename T = void, typename S = xstd::exception>
	struct promise : promise_ref<T, S, true>, make_awaitable_tag_t
	{
		// Traits for co_await.
		//
		using value_type =  T;
		using status_type = S;

		// Redirect constructor to reference type.
		//
		using promise_ref<T, S, true>::promise_ref;

		// Make awaitable via move.
		//
		unique_future<T, S> move() { return { std::move( *this ) }; }
	};

	// Factories for the promises.
	//
	template<typename T = void, typename S = xstd::exception>
	inline promise<T, S> make_promise()
	{
		auto res = new promise_base<T, S>();
		res->refs.store( 1 /*promise flag*/, std::memory_order::relaxed );
		return promise<T, S>{ res };
	}
	template<typename T = void, typename S = xstd::exception>
	inline promise<T, S> make_rejected_promise( S&& status )
	{
		auto pr = make_promise<T, S>();
		pr.reject( std::move( status ) );
		return pr;
	}

	// Observer for the promise type.
	//
	template<typename T = void, typename S = xstd::exception>
	struct future : promise_ref<T, S, false>, make_awaitable_tag_t
	{
		// Traits for co_await.
		//
		using value_type =  T;
		using status_type = S;

		// Redirect constructor to reference type.
		//
		using promise_ref<T, S, false>::promise_ref;
		
		// Allow coroutine transformation.
		//
		struct promise_type
		{
			promise<T, S> promise = make_promise<T, S>();

			struct final_awaitable
			{
				bool await_ready() noexcept { return false; }
				void await_suspend( coroutine_handle<promise_type> hnd ) noexcept 
				{ 
					hnd.promise().promise.signal();
					hnd.destroy();
				}
				void await_resume() const noexcept { unreachable(); }
			};

			future<T, S> get_return_object() { return promise; }
			suspend_never initial_suspend() noexcept { return {}; }
			final_awaitable final_suspend() noexcept { return {}; }
			void unhandled_exception() {}
			
			template<typename V> 
			void return_value( V&& v )
			{
				promise.resolve_unchecked( std::forward<V>( v ) );
			}

			template<typename V>
			final_awaitable yield_value( V&& v ) requires ( !Same<S, no_status> )
			{
				promise.reject_unchecked( std::forward<V>( v ) );
				return {};
			}
		};

		// Make awaitable via move.
		//
		unique_future<T, S> move() { return { std::move( *this ) }; }
	};
	template<typename S>
	struct future<void, S> : promise_ref<void, S, false>, make_awaitable_tag_t
	{
		// Traits for co_await.
		//
		using value_type =  void;
		using status_type = S;

		// Redirect constructor to reference type.
		//
		using promise_ref<void, S, false>::promise_ref;

		// Allow coroutine transformation.
		//
		struct promise_type
		{
			promise<void, S> promise = make_promise<void, S>();

			struct final_awaitable
			{
				bool await_ready() noexcept { return false; }
				void await_suspend( coroutine_handle<promise_type> hnd ) noexcept 
				{ 
					hnd.promise().promise.signal();
					hnd.destroy();
				}
				void await_resume() const noexcept { unreachable(); }
			};

			future<void, S> get_return_object() { return promise; }
			suspend_never initial_suspend() noexcept { return {}; }
			final_awaitable final_suspend() noexcept { return {}; }
			void unhandled_exception() {}
			
			void return_void() { promise.resolve_unchecked(); }

			template<typename V>
			final_awaitable yield_value( V&& v ) requires ( !Same<S, no_status> )
			{
				promise.reject_unchecked( std::forward<V>( v ) );
				return {};
			}
		};

		// Make awaitable via move.
		//
		unique_future<void, S> move() { return { std::move( *this ) }; }
	};

	// Make promise references co awaitable.
	//
	template<typename P> requires std::is_base_of_v<make_awaitable_tag_t, P>
	auto operator co_await( const P& ref )
	{
		using S = typename P::status_type;

		struct awaitable
		{
			const P& promise;
			mutable coroutine_handle<> awaiting_as = nullptr;

			bool await_ready()
			{
				if ( !promise.finished() )
					return false;
				promise.unrace();
				return true;
			}
			bool await_suspend( coroutine_handle<> hnd )
			{
				if ( promise.listen( hnd ) )
				{
					awaiting_as = hnd;
					return true;
				}
				return false;
			}
			const auto& await_resume() const
			{
				awaiting_as = nullptr;
				if constexpr ( Same<S, no_status> )
					return promise.result().value();
				else
					return promise.result();
			}
			~awaitable()
			{
				if ( awaiting_as )
					promise.unlisten( awaiting_as );
			}
		};
		return awaitable{ ref };
	}
	template<typename P> requires std::is_base_of_v<make_awaitable_tag_t, P>
	auto operator co_await( P&& ref )
	{
		using S = typename P::status_type;

		struct awaitable
		{
			P promise;
			mutable coroutine_handle<> awaiting_as = nullptr;

			bool await_ready()
			{
				if ( !promise.finished() )
					return false;
				promise.unrace();
				return true;
			}
			bool await_suspend( coroutine_handle<> hnd )
			{
				if ( promise.listen( hnd ) )
				{
					awaiting_as = hnd;
					return true;
				}
				return false;
			}
			auto await_resume() const
			{
				awaiting_as = nullptr;
				if constexpr ( Same<S, no_status> )
					return promise.result().value();
				else
					return promise.result();
			}
			~awaitable()
			{
				if ( awaiting_as )
					promise.unlisten( awaiting_as );
			}
		};
		return awaitable{ std::move( ref ) };
	}
	template<typename T, typename S>
	auto operator co_await( unique_future<T, S> ref )
	{
		struct awaitable
		{
			promise_ref<T, S, false> promise;
			mutable coroutine_handle<> awaiting_as = nullptr;

			bool await_ready()
			{
				if ( !promise.finished() )
					return false;
				promise.unrace();
				return true;
			}
			bool await_suspend( coroutine_handle<> hnd )
			{
				if ( promise.listen( hnd ) )
				{
					awaiting_as = hnd;
					return true;
				}
				return false;
			}
			auto await_resume() const
			{
				awaiting_as = nullptr;
				if constexpr ( Same<S, no_status> )
				{
					if constexpr ( !Same<T, void> )
						return std::move( make_mutable( promise.result() ) ).value();
				}
				else
				{
					return std::move( make_mutable( promise.result() ) );
				}
			}
			~awaitable()
			{
				if ( awaiting_as )
					promise.unlisten( awaiting_as );
			}
		};
		return awaitable{ std::move( ref ) };
	}

	// Deduction guides for future decay.
	//
	template<typename T, typename S> future( promise<T, S> )->future<T, S>;
	template<typename T, typename S> future( unique_future<T, S>&& )->future<T, S>;
	template<typename T, typename S> unique_future( promise<T, S> )->unique_future<T, S>;
};