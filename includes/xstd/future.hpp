#pragma once
#include "result.hpp"
#include "event.hpp"
#include "coro.hpp"
#include "chore.hpp"
#include "spinlock.hpp"
#include "scope_tpr.hpp"
#include "hashable.hpp"
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

		template<typename T>
		inline const basic_result<T, xstd::exception> timeout_result_v{ in_place_failure_t{}, XSTD_ESTR( "Promise timed out." ) };

		template<typename T, typename S>
		inline const basic_result<T, S>& timeout_result()
		{
			if constexpr ( Same<S, xstd::exception> )
				return timeout_result_v<T>;
			else if constexpr ( !basic_result<T, S>::has_status )
				xstd::error( XSTD_ESTR( "Promise timed out." ) );
			else
				return xstd::static_default{};
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

			FORCE_INLINE constexpr bool bit_set( int idx, bool relax = false )
			{
				if ( relax || std::is_constant_evaluated() )
					return xstd::bit_set( value, idx );
				else
					return xstd::bit_set<volatile T>( value, idx );
			}
			FORCE_INLINE constexpr bool bit_reset( int idx, bool relax = false )
			{
				if ( relax || std::is_constant_evaluated() )
					return xstd::bit_reset( value, idx );
				else
					return xstd::bit_reset<volatile T>( value, idx );
			}

			FORCE_INLINE constexpr T load( bool relax = false ) const
			{
				if ( relax || std::is_constant_evaluated() )
					return value;
				else
					return std::atomic_ref{ value }.load( std::memory_order::relaxed );
			}
			FORCE_INLINE constexpr void store( T new_value, bool relax = false )
			{
				if ( relax || std::is_constant_evaluated() )
					value = new_value;
				else
					std::atomic_ref{ value }.store( new_value, std::memory_order::release );
			}
			FORCE_INLINE constexpr T fetch_add( T increment, bool relax = false )
			{
				if ( relax || std::is_constant_evaluated() )
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
			FORCE_INLINE constexpr T fetch_sub( T decrement, bool relax = false )
			{
				if ( relax || std::is_constant_evaluated() )
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
		//  - Refs : [24 (Viewing) - 8 (Owning)]
		//
		static constexpr uint32_t owner_flag =  1;
		static constexpr uint32_t viewer_flag = 1 << 8;
		FORCE_INLINE static constexpr uint32_t count_owners( uint32_t x ) { return x & 0xFF; }
		FORCE_INLINE static constexpr uint32_t count_viewers( uint32_t x ) { return x >> 8; }

		// State flags.
		//
		static constexpr uint8_t  state_finished_bit = 0;
		static constexpr uint8_t  state_written_bit =  1;
		static constexpr uint16_t state_finished =     1 << state_finished_bit;
		static constexpr uint16_t state_written =      1 << state_written_bit;
	};

	// Base of the promise type.
	//
	template<typename T, typename S>
	struct promise_base
	{
		// Reference count.
		//
		mutable impl::atomic_integral<uint32_t> refs = { impl::owner_flag };

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

		// Allocating co-routine if not manually allocated.
		//
		coroutine_handle<>                      coro = nullptr;

		// Default constructor/destructor.
		//
		promise_base() {}
		~promise_base()
		{
			if ( finished() )
				std::destroy_at( &result );
		}

		// Constructor for in-place constructed promises.
		//
		template<typename Promise>
		promise_base( std::in_place_t, Promise* promise, uint32_t initial_ref_count = impl::owner_flag )
			: refs( initial_ref_count ), coro( coroutine_handle<Promise>::from_promise( *promise ) ) {}

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

			impl::wait_block wb = {};
			if ( !register_wait_block( wb ) )
				return unrace();
			
			wb.event.wait();
			return result;
		}
		basic_result<T, S> wait_move()
		{
			if ( finished() ) [[likely]]
				return std::move( *( unrace(), &result ) );

			impl::wait_block wb = {};
			if ( !register_wait_block( wb ) )
				return std::move( *( unrace(), &result ) );

			wb.event.wait();
			return std::move( result );
		}
		const basic_result<T, S>& wait_for( duration time ) const
		{
			if ( finished() ) [[likely]]
				return unrace();

			impl::wait_block wb = {};
			if ( !register_wait_block( wb ) )
				return unrace();

			if ( wb.event.wait_for( time ) || !deregister_wait_block( wb ) )
				return result;
			else
				return impl::timeout_result<T, S>();
		}
		basic_result<T, S> wait_for_move( duration time ) const
		{
			if ( finished() ) [[likely]]
				return std::move( *( unrace(), &result ) );

			impl::wait_block wb = {};
			if ( !register_wait_block( wb ) )
				return std::move( *( unrace(), &result ) );

			if ( wb.event.wait_for( time ) || !deregister_wait_block( wb ) )
				return std::move( result );
			else
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
			state.store( impl::state_finished | impl::state_written ); 
		}
		template<typename... Args> 
		void resolve_unchecked( Args&&... args ) 
		{ 
			std::construct_at( &result, in_place_success_t{}, std::forward<Args>( args )... ); 
			state.store( impl::state_finished | impl::state_written ); 
		}
		template<typename... Args> 
		void emplace_unchecked( Args&&... args ) 
		{ 
			std::construct_at( &result, std::forward<Args>( args )... ); 
			state.store( impl::state_finished | impl::state_written );
		}
		template<typename... Args> 
		bool reject( Args&&... args )  
		{ 
			if ( state.bit_set( impl::state_finished_bit ) )
				return false; 
			reject_unchecked( std::forward<Args>( args )... ); 
			signal(); 
			return true; 
		}
		template<typename... Args> 
		bool resolve( Args&&... args )
		{
			if ( state.bit_set( impl::state_finished_bit ) )
				return false; 
			resolve_unchecked( std::forward<Args>( args )... ); 
			signal(); 
			return true; 
		}
		template<typename... Args> 
		bool emplace( Args&&... args ) 
		{
			if ( state.bit_set( impl::state_finished_bit ) )
				return false; 
			emplace_unchecked( std::forward<Args>( args )... ); 
			signal(); 
			return true; 
		}

		// Exposes information on the promise state.
		//
		inline bool finished() const { return state.load() & impl::state_finished; }
		inline bool pending() const { return !finished(); }
		bool fulfilled() const 
		{ 
			if constexpr ( !basic_result<T, S>::has_status )
				return finished();
			else
				return finished() && unrace().success(); 
		}
		bool failed() const 
		{ 
			if constexpr ( !basic_result<T, S>::has_status )
				return false;
			else
				return finished() && unrace().fail(); 
		}

		// Helper to reference the result indirectly without a signal and avoiding the race condition.
		//
		inline const basic_result<T, S>& unrace() const
		{
			while ( !( state.load() & impl::state_written ) ) [[unlikely]]
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

		// Helpers for the reference type.
		//
		COLD void break_for_deref_unchecked()
		{
			if constexpr ( Same<S, xstd::exception> )
				reject_unchecked( xstd::exception{ XSTD_ESTR( "Promise broken." ) } );
			else if constexpr ( !basic_result<T, S>::has_status )
				xstd::error( XSTD_ESTR( "Promise broken." ) );
			else
				reject_unchecked();
			signal();
		}
		void break_for_deref() 
		{
			if ( !state.bit_set( impl::state_finished_bit, true /*relaxed since caller makes sure we're the only reference.*/ ) )
				break_for_deref_unchecked();
		}
		void destroy()
		{
			void* base = coro ? coro.address() : this;
			std::destroy_at( this );
			operator delete( base );
		}
		FORCE_INLINE void inc_ref( bool owner )
		{
			refs.fetch_add( owner ? impl::owner_flag : impl::viewer_flag );
		}
		FORCE_INLINE void dec_ref( bool owner )
		{
			// Break the promise if this is the only one left.
			//
			if ( owner )
			{
				auto counter = refs.load();
				if ( impl::count_owners( counter ) == 1 ) [[unlikely]]
					break_for_deref();
			}

			// Destroy the object if this is the only one left.
			//
			auto flag = owner ? impl::owner_flag : impl::viewer_flag;
			if ( refs.fetch_sub( flag ) == flag ) [[unlikely]]
				destroy();
		}
		FORCE_INLINE void cvt_ref( bool from_owner, bool to_owner )
		{
			// If we have to change the types:
			//
			if ( from_owner != to_owner )
			{
				// If it was previously owning:
				//
				if ( from_owner )
				{
					// Downgrade the reference.
					//
					auto counter = refs.fetch_add( impl::viewer_flag - impl::owner_flag );

					// Break promise if it was the only one.
					//
					if ( impl::count_owners( counter ) == 1 ) [[unlikely]]
						break_for_deref();
				}
				// If it was previously viewing:
				//
				else
				{
					// Upgrade the reference.
					//
					refs.fetch_add( impl::owner_flag - impl::viewer_flag );
				}
			}
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

		// Ownership properties.
		//
		inline constexpr bool is_owner() const noexcept { return Owner; }
		inline constexpr bool is_viewer() const noexcept { return !Owner; }

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
			adopt( std::exchange( other.ptr, nullptr ), other.is_owner() );
		}
		template<typename V> requires Promise<V, T, S>
		inline constexpr promise_ref( const V& other ) 
		{
			reset( other.ptr );
		}
		template<typename V> requires Promise<V, T, S>
		inline constexpr promise_ref& operator=( V&& other ) noexcept
		{
			adopt( std::exchange( other.ptr, nullptr ), other.is_owner() );
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
		inline constexpr void adopt( promise_base<T, S>* new_ptr, bool from_owner = Owner )
		{
			if ( new_ptr )
				new_ptr->cvt_ref( from_owner, Owner );
			if ( auto* prev_ptr = std::exchange( ptr, new_ptr ) )
				prev_ptr->dec_ref( Owner );
		}
		inline constexpr void reset( promise_base<T, S>* new_ptr = nullptr )
		{ 
			if ( new_ptr )
				new_ptr->inc_ref( Owner );
			if ( auto* prev_ptr = std::exchange( ptr, new_ptr ) )
				prev_ptr->dec_ref( Owner );
		}
		inline constexpr ~promise_ref() { reset(); }

		// Reference observer.
		//
		inline constexpr explicit operator bool() const { return ptr != nullptr; }
		inline constexpr promise_ref* address() const { return ptr; }
		inline constexpr size_t promise_count() const
		{
			return impl::count_owners( ptr->refs.load() );
		}
		inline constexpr size_t future_count() const
		{
			return impl::count_viewers( ptr->refs.load() );
		}
		inline constexpr size_t ref_count() const
		{
			auto flags = ptr->refs.load();
			return impl::count_owners( flags ) + impl::count_viewers( flags );
		}
		inline constexpr bool unique() const
		{
			return ptr->refs.load() == ( Owner ? impl::owner_flag : impl::viewer_flag );
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
		inline const basic_result<T, S>& result() const { fassert( finished() ); return ptr->unrace(); }
		inline std::string to_string() const { return ptr->to_string(); }
		
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

		// Comparison.
		//
		template<typename V> requires Promise<V, T, S> inline constexpr bool operator==( const V& other ) const noexcept { return ptr == other.ptr; }
		template<typename V> requires Promise<V, T, S> inline constexpr bool operator!=( const V& other ) const noexcept { return ptr != other.ptr; }
		template<typename V> requires Promise<V, T, S> inline constexpr bool operator<( const V& other )  const noexcept { return ptr < other.ptr; }
	};

	// Moving awaitable wrapper for promise refences, might be unsafe if shared.
	//
	template<typename T = void, typename S = xstd::exception>
	struct unique_future : promise_ref<T, S, false>
	{
		using reference_type = promise_ref<T, S, false>;
		using reference_type::reference_type;
		using reference_type::operator=;
		using reference_type::operator==;
		using reference_type::operator!=;
		using reference_type::operator<;

		// No copy.
		//
		unique_future( const unique_future& ) = delete;
		unique_future& operator=( const unique_future& ) = delete;

		// Default move.
		//
		constexpr unique_future( unique_future&& ) noexcept = default;
		constexpr unique_future& operator=( unique_future&& ) noexcept = default;

		// Make wait/wait_for/result move.
		//
		inline basic_result<T, S> wait() const { return reference_type::ptr->wait_move(); }
		inline basic_result<T, S> wait_for( duration time ) const { return reference_type::ptr->wait_for_move( time ); }
		inline basic_result<T, S> result() const { fassert( this->finished() ); this->unrace(); return std::move( reference_type::ptr->result ); }
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
		using reference_type::operator==;
		using reference_type::operator!=;
		using reference_type::operator<;

		// Traits for co_await.
		//
		using result_type = basic_result<T, S>;

		// Make awaitable via move.
		//
		unique_future<T, S> move() { return std::move( *this ); }
	};

	// Factories for the promises.
	//
	template<typename T = void, typename S = xstd::exception>
	inline promise<T, S> make_promise()
	{
		return promise<T, S>{ std::in_place_t{}, new promise_base<T, S>() };
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
		using reference_type::operator==;
		using reference_type::operator!=;
		using reference_type::operator<;

		// Traits for co_await.
		//
		using result_type = basic_result<T, S>;

		// Allow coroutine transformation.
		//
		struct promise_type
		{
			union
			{
				promise_base<T, S> pr;
				uint8_t dummy = 0;
			};
			promise_type() : pr( std::in_place_t{}, this, impl::owner_flag + impl::viewer_flag ) {}
			
			// Do not destroy the promise on destruction.
			//
			~promise_type() {}

			// No delete, promise_base will do that.
			//
			void* operator new( size_t n ) { return ::operator new( n ); }
			void operator delete( void* ) {}

			struct final_awaitable
			{
				inline bool await_ready() noexcept { return false; }

				template<typename P = promise_type>
				inline void await_suspend( coroutine_handle<P> hnd ) noexcept
				{ 
					auto& pr = hnd.promise().pr;
					pr.signal();
					hnd.destroy(); // Will not destroy the promise or delete it.
					pr.dec_ref( true );
				}
				inline void await_resume() const noexcept { unreachable(); }
			};

			future<T, S> get_return_object() { return { std::in_place_t{}, &pr }; }
			suspend_never initial_suspend() noexcept { return {}; }
			final_awaitable final_suspend() noexcept { return {}; }
			void unhandled_exception() {}
			
			template<typename V> 
			void return_value( V&& v )
			{
				pr.resolve_unchecked( std::forward<V>( v ) );
			}

			template<typename V>
			final_awaitable yield_value( V&& v ) requires ( basic_result<T, S>::has_status )
			{
				pr.reject_unchecked( std::forward<V>( v ) );
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
		using reference_type::operator==;
		using reference_type::operator!=;
		using reference_type::operator<;

		// Traits for co_await.
		//
		using result_type = basic_result<void, S>;

		// Allow coroutine transformation.
		//
		struct promise_type
		{
			union
			{
				promise_base<void, S> pr;
				uint8_t dummy = 0;
			};
			promise_type() : pr( std::in_place_t{}, this, impl::owner_flag + impl::viewer_flag ) {}

			// Do not destroy the promise on destruction.
			//
			~promise_type() {}

			// No delete, promise_base will do that.
			//
			void* operator new( size_t n ) { return ::operator new( n ); }
			void operator delete( void* ) {}

			struct final_awaitable
			{
				inline bool await_ready() noexcept { return false; }

				template<typename P = promise_type>
				inline void await_suspend( coroutine_handle<P> hnd ) noexcept
				{
					auto& pr = hnd.promise().pr;
					pr.signal();
					hnd.destroy(); // Will not destroy the promise or delete it.
					pr.dec_ref( true );
				}
				inline void await_resume() const noexcept { unreachable(); }
			};

			future<void, S> get_return_object() { return { std::in_place_t{}, &pr }; }
			suspend_never initial_suspend() noexcept { return {}; }
			final_awaitable final_suspend() noexcept { return {}; }
			void unhandled_exception() {}
			
			void return_void() { pr.resolve_unchecked(); }

			template<typename V>
			final_awaitable yield_value( V&& v ) requires ( result_type::has_status )
			{
				pr.reject_unchecked( std::forward<V>( v ) );
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
	inline auto operator co_await( const P& ref )
	{
		using R = typename P::result_type;

		struct awaitable
		{
			const P& promise;
			mutable coroutine_handle<> awaiting_as = nullptr;

			inline bool await_ready()
			{
				if ( !promise.finished() )
					return false;
				promise.unrace();
				return true;
			}
			inline bool await_suspend( coroutine_handle<> hnd )
			{
				if ( promise.listen( hnd ) )
				{
					awaiting_as = hnd;
					return true;
				}
				return false;
			}
			inline decltype( auto ) await_resume() const
			{
				awaiting_as = nullptr;
				if constexpr ( R::has_status )
					return promise.result();
				else if constexpr( R::has_value )
					return promise.result().value();
			}
			inline ~awaitable()
			{
				if ( awaiting_as )
					promise.unlisten( awaiting_as );
			}
		};
		return awaitable{ ref };
	}
	template<typename P> requires std::is_base_of_v<make_awaitable_tag_t, P>
	inline auto operator co_await( P&& ref )
	{
		using R = typename P::result_type;

		struct awaitable
		{
			P promise;
			mutable coroutine_handle<> awaiting_as = nullptr;

			inline bool await_ready()
			{
				if ( !promise.finished() )
					return false;
				promise.unrace();
				return true;
			}
			inline bool await_suspend( coroutine_handle<> hnd )
			{
				if ( promise.listen( hnd ) )
				{
					awaiting_as = hnd;
					return true;
				}
				return false;
			}
			inline auto await_resume() const
			{
				awaiting_as = nullptr;
				if constexpr ( R::has_status )
					return promise.result();
				else if constexpr ( R::has_value )
					return promise.result().value();
			}
			inline ~awaitable()
			{
				if ( awaiting_as )
					promise.unlisten( awaiting_as );
			}
		};
		return awaitable{ std::move( ref ) };
	}
	template<typename T, typename S>
	inline auto operator co_await( unique_future<T, S> ref )
	{
		using R = basic_result<T, S>;

		struct awaitable
		{
			promise_ref<T, S, false> promise;
			mutable coroutine_handle<> awaiting_as = nullptr;

			inline bool await_ready()
			{
				if ( !promise.finished() )
					return false;
				promise.unrace();
				return true;
			}
			inline bool await_suspend( coroutine_handle<> hnd )
			{
				if ( promise.listen( hnd ) )
				{
					awaiting_as = hnd;
					return true;
				}
				return false;
			}
			inline auto await_resume() const
			{
				awaiting_as = nullptr;

				if constexpr ( R::has_status )
					return std::move( make_mutable( promise.result() ) );
				else if constexpr ( R::has_value )
					return std::move( make_mutable( promise.result() ) ).value();
			}
			inline ~awaitable()
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

	// Make promise types hashable.
	//
	template<typename H, typename T, typename S>
	struct basic_hasher<H, future<T, S>>
	{
		__forceinline constexpr hash_t operator()( const future<T, S>& value ) const noexcept
		{
			return make_hash<H>( value.address() );
		}
	};
	template<typename H, typename T, typename S>
	struct basic_hasher<H, unique_future<T, S>>
	{
		__forceinline constexpr hash_t operator()( const unique_future<T, S>& value ) const noexcept
		{
			return make_hash<H>( value.address() );
		}
	};
	template<typename H, typename T, typename S>
	struct basic_hasher<H, promise<T, S>>
	{
		__forceinline constexpr hash_t operator()( const promise<T, S>& value ) const noexcept
		{
			return make_hash<H>( value.address() );
		}
	};
};

// Override coroutine traits to allow unique_future.
//
namespace std
{
	template<typename T, typename S, typename... A>
	struct coroutine_traits<xstd::unique_future<T, S>, A...>
	{
		using promise_type = typename xstd::future<T, S>::promise_type;
	};
};