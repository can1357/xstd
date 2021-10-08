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

		// Small vector implementation.
		//
		static constexpr size_t in_place_continuation_count = 2;
		struct continuation_vector
		{
			// In-place buffer.
			//
			coroutine_handle<>  ibuffer[ in_place_continuation_count ] = {};

			// Vector descriptor.
			//
			uint32_t            length =   0;
			uint32_t            reserved = in_place_continuation_count;
			coroutine_handle<>* first =    &ibuffer[ 0 ];

			// No move/copy, default constructed.
			//
			constexpr continuation_vector() = default;
			continuation_vector( const continuation_vector& ) = delete;
			continuation_vector& operator=( const continuation_vector& ) = delete;

			// Checks whether or not it holds an externally allocated buffer.
			//
			FORCE_INLINE bool inplace() const { return first == &ibuffer[ 0 ]; }

			// Make iterable.
			//
			FORCE_INLINE size_t size() const { return length; }
			FORCE_INLINE size_t capacity() const { return reserved; }
			FORCE_INLINE bool empty() const { return size() == 0; }
			FORCE_INLINE coroutine_handle<>* begin() { return &first[ 0 ]; }
			FORCE_INLINE const coroutine_handle<>* begin() const { return &first[ 0 ]; }
			FORCE_INLINE coroutine_handle<>* end() { return &first[ length ]; }
			FORCE_INLINE const coroutine_handle<>* end() const { return &first[ length ]; }
			FORCE_INLINE coroutine_handle<> operator[]( size_t n ) const { return first[ n ]; }

			// Pushes back an entry.
			//
			FORCE_INLINE void push( coroutine_handle<> value )
			{
				// If it does not fit within the current buffer:
				//
				if ( size() == capacity() ) [[unlikely]]
				{
					// Allocate a new buffer.
					//
					coroutine_handle<>* buffer = new coroutine_handle<>[ capacity() * 4 ];
					std::copy( begin(), end(), buffer );

					// Free the previous one and assign the new one.
					//
					if ( !inplace() )
						delete[] first;
					first = buffer;
				}
				first[ length++ ] = value;
			}

			// Finds an entry by value and removes it.
			//
			FORCE_INLINE bool pop( coroutine_handle<> value )
			{
				auto it = std::find( begin(), end(), value );
				if ( it == end() ) [[unlikely]]
					return false;
				for ( ; ( it + 1 ) != end(); it++ )
					*it = *( it + 1 );
				--length;
				return true;
			}

			// Remove all entries.
			//
			FORCE_INLINE void clear()
			{
				if ( !inplace() )
					delete[] first;
				first = &ibuffer[ 0 ];
				length = 0;
				reserved = in_place_continuation_count;
			}

			// Free the buffer on destruction.
			//
			FORCE_INLINE ~continuation_vector()
			{
				if ( !inplace() ) 
					delete[] first;
			}
		};

		// Atomic integer with constexpr fallback and relaxed rules.
		//
		template<typename T>
		struct atomic_integral
		{
			T value;
			FORCE_INLINE constexpr atomic_integral( T value ) : value( value ) {}

			FORCE_INLINE constexpr bool bit_set( int idx )
			{
				return xstd::bit_set( value, idx );
			}
			FORCE_INLINE constexpr T load() const
			{
				if ( std::is_constant_evaluated() )
					return value;
				else
					return std::atomic_ref{ value }.load( std::memory_order::relaxed );
			}
			FORCE_INLINE constexpr void store( T new_value )
			{
				if ( std::is_constant_evaluated() )
					value = new_value;
				else
					std::atomic_ref{ value }.store( new_value, std::memory_order::release );
			}
			FORCE_INLINE constexpr T fetch_add( T increment )
			{
				if ( std::is_constant_evaluated() )
				{
					T result = value;
					value += increment;
					return result;
				}
				else
				{
					return std::atomic_ref{ value }.fetch_add( increment );
				}
			}
			FORCE_INLINE constexpr T fetch_sub( T decrement )
			{
				if ( std::is_constant_evaluated() )
				{
					T result = value;
					value -= decrement;
					return result;
				}
				else
				{
					return std::atomic_ref{ value }.fetch_sub( decrement );
				}
			}
		};

		// Promise ref-counting details.
		//  - Refs : [24 (Promise) - 8 (Future)]
		//
		static constexpr uint32_t future_bit =   0;
		static constexpr uint32_t promise_bit =  24;
		static constexpr uint32_t future_flag =  1 << future_bit;
		static constexpr uint32_t promise_flag = 1 << promise_bit;
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

		// Reference count.
		//
		mutable impl::atomic_integral<uint32_t> refs = { 0 };

		// Lock guarding continuation list and the event list.
		//
		mutable spinlock                        state_lock = {};

		// Promise state.
		//
		impl::atomic_integral<uint16_t>         state = { 0 };

		// Event and the continuation list.
		//
		mutable impl::wait_block*               events = nullptr;
		mutable impl::continuation_vector       continuation = {};

		// Result.
		//
		union
		{
			uint8_t                              dummy = 0;
			basic_result<T, S>                   result;
		};

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
			xstd::task_lock g{ state_lock, XSTD_PROMISE_TASK_PRIORITY };
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
			xstd::task_lock g{ state_lock, XSTD_PROMISE_TASK_PRIORITY };

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

			xstd::task_lock g{ state_lock, XSTD_PROMISE_TASK_PRIORITY };
			if ( finished() ) [[unlikely]]
				return false;
			continuation.push( h );
			return true;
		}
		bool unlisten( coroutine_handle<> h ) const
		{
			if ( finished() ) [[likely]]
				return false;

			xstd::task_lock g{ state_lock, XSTD_PROMISE_TASK_PRIORITY };
			if ( finished() ) [[unlikely]]
				return false;
			if ( !continuation.pop( h ) ) [[unlikely]]
				xstd::error( XSTD_ESTR( "Corrupt continuation list." ) );
			return true;
		}

		// Signals the event, runs all continuation entries.
		//
		void signal()
		{
			uint8_t prev_tp;
			{
				xstd::task_lock g{ state_lock, XSTD_PROMISE_TASK_PRIORITY };
				prev_tp = g.prev_tp;

				// Notify all events.
				//
				for ( auto it = events; it; it = it->next )
					it->event.notify( true );
				events = nullptr;
			}

			// If there are any continuation entries:
			//
			if ( !continuation.empty() )
			{
				// If task priority is raised, schedule every instance via ::chore, otherwise 
				// schedule every instance but the first via ::chore and continue from the first one.
				//
				coroutine_handle<> cnt = nullptr;
				if ( !prev_tp )  cnt = continuation[ 0 ];
				else             xstd::chore( continuation[ 0 ] );

				for ( uint32_t n = 1; n != continuation.size(); n++ )
					xstd::chore( continuation[ n ] );

				continuation.clear();
				if ( cnt ) cnt();
			}
		}

		// Resolution of the promise value.
		//
		template<typename... Args> 
		void reject_unchecked( Args&&... args ) 
		{ 
			std::construct_at( &result, in_place_failure_t{}, std::forward<Args>( args )... ); 
			state.store( state_finished | state_written ); 
		}
		template<typename... Args> 
		void resolve_unchecked( Args&&... args ) 
		{ 
			std::construct_at( &result, in_place_success_t{}, std::forward<Args>( args )... ); 
			state.store( state_finished | state_written ); 
		}
		template<typename... Args> 
		void emplace_unchecked( Args&&... args ) 
		{ 
			std::construct_at( &result, std::forward<Args>( args )... ); 
			state.store( state_finished | state_written );
		}
		template<typename... Args> 
		bool reject( Args&&... args )  
		{ 
			if ( state.bit_set( state_finished_bit ) )
				return false; 
			reject_unchecked( std::forward<Args>( args )... ); 
			signal(); 
			return true; 
		}
		template<typename... Args> 
		bool resolve( Args&&... args )
		{
			if ( state.bit_set( state_finished_bit ) )
				return false; 
			resolve_unchecked( std::forward<Args>( args )... ); 
			signal(); 
			return true; 
		}
		template<typename... Args> 
		bool emplace( Args&&... args ) 
		{
			if ( state.bit_set( state_finished_bit ) )
				return false; 
			emplace_unchecked( std::forward<Args>( args )... ); 
			signal(); 
			return true; 
		}

		// Exposes information on the promise state.
		//
		inline bool finished() const { return state.load() & state_finished; }
		inline bool pending() const { return !finished(); }
		bool fulfilled() const 
		{ 
			if constexpr ( Same<S, no_status> )
				return finished();
			else
				return finished() && unrace().success(); 
		}
		bool failed() const 
		{ 
			if constexpr ( Same<S, no_status> )
				return false;
			else
				return finished() && unrace().fail(); 
		}

		// Helper to reference the result indirectly without a signal and avoiding the race condition.
		//
		const basic_result<T, S>& unrace() const
		{
			while ( !( state.load() & state_written ) ) [[unlikely]]
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
	template<typename T, typename S> 
	struct promise_ref_tag_t {};
	
	template<typename V, typename T, typename S>
	inline constexpr bool is_promise_v = std::is_base_of_v<promise_ref_tag_t<T, S>, V>;
	template<typename V, typename T, typename S>
	concept Promise = is_promise_v<V, T, S>;

	template<typename T, typename S, bool Owner>
	struct promise_ref : promise_ref_tag_t<T, S>
	{
		promise_base<T, S>* ptr = nullptr;

		static constexpr uint32_t ref_flag = Owner ? impl::promise_flag : impl::future_flag;
		static void destroy( promise_base<T, S>* ptr ) { delete ptr; }
		static void break_promise( promise_base<T, S>* ptr ) 
		{
			if constexpr ( Same<S, xstd::exception> )
				ptr->reject( xstd::exception{ XSTD_ESTR( "Promise broken." ) } );
			else if constexpr ( Same<S, no_status> )
				xstd::error( XSTD_ESTR( "Promise broken." ) );
			else
				ptr->reject();
		}

		FORCE_INLINE static void inc_ref( promise_base<T, S>* ptr )
		{
			ptr->refs.fetch_add( ref_flag );
		}
		FORCE_INLINE static void dec_ref( promise_base<T, S>* ptr )
		{
			// Break the promise if this is the only one left.
			//
			if constexpr ( Owner )
			{
				if ( ptr->pending() ) [[unlikely]]
					if ( ( ptr->refs.load() & ( impl::promise_flag - 1 ) ) == 1 ) [[unlikely]]
						break_promise( ptr );
			}

			// Destroy the object if this is the only one left.
			//
			if ( ptr->refs.fetch_sub( ref_flag ) == ref_flag )
				destroy( ptr );
		}
		FORCE_INLINE static promise_base<T, S>* cvt_ref( promise_base<T, S>* ptr, bool was_owning )
		{
			// If we have to change the types:
			//
			if ( ptr && was_owning != Owner )
			{
				// If it was previously owning:
				//
				if ( was_owning )
				{
					// Downgrade the reference.
					//
					auto prev = ptr->refs.fetch_add( impl::future_flag - impl::promise_flag );

					// Break promise if it was the only one.
					//
					if ( ptr->pending() ) [[unlikely]]
						if ( ( prev & ( impl::promise_flag - 1 ) ) == 1 ) [[unlikely]]
							break_promise( ptr );
				}
				// If it was previously viewing:
				//
				else
				{
					// Upgrade the reference.
					//
					ptr->refs.fetch_add( impl::promise_flag - impl::future_flag );
				}
			}
			return ptr;
		}

		// Simple constructors.
		//
		inline constexpr promise_ref() {}
		inline constexpr promise_ref( std::nullptr_t ) {}
		inline constexpr promise_ref( std::in_place_t, promise_base<T, S>* ptr ) : ptr( ptr ) {}

		// Copy/Move within the same types.
		//
		inline constexpr promise_ref( promise_ref&& other ) noexcept : ptr( std::exchange( other.ptr, nullptr ) ) {}
		inline constexpr promise_ref& operator=( promise_ref&& other ) noexcept { std::swap( ptr, other.ptr ); return *this; }
		inline constexpr promise_ref( const promise_ref& other ) { reset( other.ptr ); }
		inline constexpr promise_ref& operator=( const promise_ref& other ) { reset( other.ptr ); return *this; }

		// Copy/Move from parents.
		//
		template<typename V> requires Promise<V, T, S>
		inline constexpr promise_ref( V&& other ) noexcept
		{
			adopt( std::exchange( other.ptr, nullptr ), other.is_owning() );
		}
		template<typename V> requires Promise<V, T, S>
		inline constexpr promise_ref( const V& other ) 
		{
			reset( other.ptr );
		}
		template<typename V> requires Promise<V, T, S>
		inline constexpr promise_ref& operator=( V&& other ) noexcept
		{
			adopt( std::exchange( other.ptr, nullptr ), other.is_owning() );
			return *this;
		}
		template<typename V> requires Promise<V, T, S>
		inline constexpr promise_ref& operator=( const V& other )
		{
			reset( other.ptr );
			return *this;
		}

		// Resets the reference.
		//
		inline constexpr void adopt( promise_base<T, S>* new_ptr, bool was_owning = Owner )
		{
			if ( auto* prev_ptr = std::exchange( ptr, cvt_ref( new_ptr, was_owning ) ) )
				dec_ref( prev_ptr );
		}
		inline constexpr void reset( promise_base<T, S>* new_ptr = nullptr )
		{ 
			if ( new_ptr ) 
				inc_ref( new_ptr );
			if ( auto* prev_ptr = std::exchange( ptr, new_ptr ) )
				dec_ref( prev_ptr );
		}
		inline constexpr ~promise_ref() { reset(); }

		// Reference observer.
		//
		inline constexpr explicit operator bool() const { return ptr != nullptr; }
		inline constexpr auto* address() const { return ptr; }
		inline constexpr size_t promise_count() const
		{
			auto flags = ptr->refs.load();
			return flags >> impl::promise_bit;
		}
		inline constexpr size_t future_count() const
		{
			auto flags = ptr->refs.load();
			return flags & ( impl::promise_flag - 1 );
		}
		inline constexpr size_t ref_count() const 
		{
			auto flags = ptr->refs.load();
			return ( flags >> impl::promise_bit ) + ( flags & ( impl::promise_flag - 1 ) );
		}

		// Wrap around the reference.
		//
		inline constexpr bool is_owning() const noexcept { return Owner; }
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
		using reference_type = promise_ref<T, S, false>;
		using reference_type::reference_type;
		using reference_type::operator=;

		// No copy.
		//
		unique_future( const unique_future& ) = delete;
		unique_future& operator=( const unique_future& ) = delete;

		// Default move.
		//
		constexpr unique_future( unique_future&& ) noexcept = default;
		constexpr unique_future& operator=( unique_future&& ) noexcept = default;
	};
	struct make_awaitable_tag_t {};

	// Free-standing promise type.
	//
	template<typename T = void, typename S = xstd::exception>
	struct promise : promise_ref<T, S, true>, make_awaitable_tag_t
	{
		using reference_type = promise_ref<T, S, true>;
		using reference_type::reference_type;
		using reference_type::operator=;

		// Traits for co_await.
		//
		using value_type =  T;
		using status_type = S;
		
		// Make awaitable via move.
		//
		unique_future<T, S> move() { return std::move( *this ); }
	};

	// Factories for the promises.
	//
	template<typename T = void, typename S = xstd::exception>
	inline promise<T, S> make_promise()
	{
		auto res = new promise_base<T, S>();
		res->refs.store( impl::promise_flag );
		return promise<T, S>{ std::in_place_t{}, res };
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
		using reference_type = promise_ref<T, S, false>;
		using reference_type::reference_type;
		using reference_type::operator=;

		// Traits for co_await.
		//
		using value_type =  T;
		using status_type = S;

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
		constexpr unique_future<T, S> move() noexcept { return std::move( *this ); }
	};
	template<typename S>
	struct future<void, S> : promise_ref<void, S, false>, make_awaitable_tag_t
	{
		using reference_type = promise_ref<void, S, false>;
		using reference_type::reference_type;
		using reference_type::operator=;

		// Traits for co_await.
		//
		using value_type =  void;
		using status_type = S;

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
		constexpr unique_future<void, S> move() noexcept { return std::move( *this ); }
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