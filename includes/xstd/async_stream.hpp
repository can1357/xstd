#pragma once
#include "spinlock.hpp"
#include "coro.hpp"
#include "vec_buffer.hpp"
#include "result.hpp"
#include "event.hpp"
#include "function_view.hpp"

namespace xstd {
	// Stream stop code, low digits are reserved for the user.
	//
	enum stream_stop_code : int16_t {
		stream_stop_none =     0'000,
		
		stream_stop_fin =      1'000, // Flag indicating clean exit.
		
		stream_stop_killed =   2'000, // Flag indicating forceful termination.
		stream_stop_timeout =  2'001, //

		stream_stop_error =    3'000, // Flag indicating erroneous termination.
	};

	// Stream state.
	//
	struct async_stream_state {
		// Set if stream has ended.
		//
		std::atomic<stream_stop_code> stop_code = stream_stop_none;
		exception                     stop_reason = {};
		event                         stop_event = {};
		std::atomic<bool>             stop_written = false;
	};

	// Stream utility provider.
	//
	template<typename Self>
	struct async_stream_utils {
		// Producer utilities.
		//
		auto write( std::span<const uint8_t> data ) {
			return static_cast<Self*>( this )->write_and( [ data ]( vec_buffer& buf ) FORCE_INLINE {
				buf.append_range( data );
			} );
		}

		// Consumer utilities.
		//
		auto read( vec_buffer& result, size_t min, size_t max ) {
			return static_cast<Self*>( this )->read_and(
				[&result]( vec_buffer& buf, size_t count ) FORCE_INLINE {
					if ( buf.size() > count ) {
						result.append_range( buf.subspan( 0, count ) );
						buf.shift( count );
					} else if ( !result ) {
						std::swap( result, buf );
					} else {
						result.append_range( buf );
					}
				},
				min, max
			);
		}
		auto read( vec_buffer& result ) {
			return read( result, 0, std::dynamic_extent );
		}
		auto read( vec_buffer& result, size_t len ) {
			return read( result, len, len );
		}
		auto read_into( std::span<uint8_t>& dst, size_t min ) {
			return static_cast<Self*>( this )->read_and(
				[out = dst.data(), &dst]( vec_buffer& buf, size_t count ) FORCE_INLINE {
					buf.shift_range( { out, count } );
					dst = dst.subspan( 0, count );
				},
				min, dst.size()
			);
		}
		auto read_into( std::span<uint8_t> dst ) { return read_into( dst, dst.size() ); }

		// State utilities.
		//
		async_stream_state& state() { return *( ( Self* )static_cast<const Self*>( this ) )->ref_state(); }
		const async_stream_state& state() const { return *( ( Self* )static_cast<const Self*>( this ) )->ref_state(); }
		
		// Stops the stream.
		//
		bool stop( stream_stop_code code = stream_stop_killed, exception ex = {} ) {
			if ( state().stop_code.exchange( code ) != stream_stop_none ) {
				return false;
			}
			state().stop_reason = std::move( ex );
			state().stop_written.store( true, std::memory_order::release );
			stop_event().notify();
			return true;
		}

		// Stop details.
		//
		bool stopped() const { return stop_code() != stream_stop_none; }
		event& stop_event() { return state().stop_event; }
		const event& stop_event() const { return state().stop_event; }
		stream_stop_code stop_code() const { return state().stop_code.load( std::memory_order::relaxed ); }
		exception stop_reason() const {
			if ( stopped() ) {
				while ( !state().stop_written.load( std::memory_order::relaxed ) )
					yield_cpu();
				return state().stop_reason;
			}
			return {};
		}

		// Waits for the streams closure.
		//
		void wait() { stop_event().wait(); }
		bool wait( duration d ) { return stop_event().wait_for( d ); }
	};

	// Single producer, single consumer in-memory stream.
	//
	struct async_stream : async_stream_utils<async_stream> {
		using lock_type = xspinlock<>;
		async_stream_state   state_ = {};

		// Lock and the buffer associated with the stream.
		//
		mutable lock_type    lock;
		vec_buffer           buffer =   {};

		// Producer handle that signals "consumer wants more data" and the high watermark
		// which is the amount of bytes in the buffer after which the producer stops getting
		// repetitively called unless specifically requested.
		//
		coroutine_handle<>   producer = {};
		size_t               high_watermark = 256_kb;

		// Consumer handle that signals "producer has written more data" and the minimum length
		// to reach before calling it.
		//
		coroutine_handle<>   consumer = {};
		size_t               consumer_minimum = 0;

		// Writes more data to the stream.
		//
		struct writer {
			async_stream& stream;
			mutable std::unique_lock<lock_type> lock;
			size_t num_written = 0;
			inline bool await_ready() noexcept {
				// If written size is 0, user is deliberately asking for a consumer to exist.
				//
				if ( !num_written ) {
					return !!stream.consumer;
				} else {
					return stream.buffer.size() < stream.high_watermark && !stream.consumer;
				}
			}
			inline coroutine_handle<> await_suspend( coroutine_handle<> h ) noexcept {
				coroutine_handle<> consumer = noop_coroutine();
				if ( stream.consumer ) {
					if ( stream.consumer_minimum > stream.buffer.size() ) {
						return h;
					} else {
						consumer = std::exchange( stream.consumer, nullptr );
					}
				}
				stream.producer = h;
				lock.unlock();
				return consumer;
			}
			inline void await_resume() const noexcept {}
		};
		template<typename F>
		writer write_and( F&& fn ) {
			std::unique_lock lock{ this->lock };
			size_t prev = buffer.size();
			( (F&&) fn )( buffer );
			return { *this, std::move( lock ), buffer.size() - prev };
		}

		// Consumes data from the stream.
		//
		template<typename F>
		struct reader {
			mutable F     fn;
			async_stream& stream;
			size_t        lower = 0;
			size_t        upper = 0;
			mutable std::unique_lock<lock_type> lock{ stream.lock };

			inline bool await_ready() noexcept {
				return stream.buffer.size() >= lower;
			}
			inline coroutine_handle<> await_suspend( coroutine_handle<> h ) noexcept {
				stream.consumer =         h;
				stream.consumer_minimum = lower;
				auto producer = std::exchange( stream.producer, nullptr );
				lock.unlock();
				if ( producer ) {
					return producer;
				} else {
					return noop_coroutine();
				}
			}
			inline void await_resume() const noexcept {
				if ( !lock ) lock.lock();
				fn( stream.buffer, std::clamp( stream.buffer.size(), lower, upper ) );
			}
		};
		template<typename F>
		auto read_and( F&& fn, size_t min = 1, size_t max = std::dynamic_extent ) {
			return reader<F>{ fn, *this, min, max };
		}

		// State reference.
		//
		async_stream_state* ref_state() { return &state_; }

		// Destroy all coroutines on destruction.
		//
		~async_stream() {
			stop( stream_stop_killed );
			for ( auto coro : { std::exchange( consumer, nullptr ), std::exchange( producer, nullptr ) } ) {
				if ( coro ) 
					coro.destroy();
			}
		}
	};

	// Composition of two streams into a duplex.
	//
	struct async_stream_composition : async_stream_utils<async_stream_composition> {
		async_stream& input;
		async_stream& output;

		async_stream_state* state_ = input.ref_state();
		async_stream_state* ref_state() { return state_; }

		// Forward read into input, write into output.
		//
		template<typename F>
		auto write_and( F&& fn ) {
			return output.write_and<F>( std::forward<F>( fn ) );
		}
		template<typename F>
		auto read_and( F&& fn, size_t min = 1, size_t max = std::dynamic_extent ) {
			return input.read_and<F>( std::forward<F>( fn ), min, max );
		}
	};
	struct async_duplex : async_stream_utils<async_duplex> {
		async_stream input =  {};
		async_stream output = {};

		// Swapped compositions for the implementation.
		//
		async_stream_composition get_controller() { 
			return { {}, output, input, input.ref_state() };
		}

		// Forward read/state into input, write into output.
		//
		template<typename F>
		auto write_and( F&& fn ) {
			return output.write_and<F>( std::forward<F>( fn ) );
		}
		template<typename F>
		auto read_and( F&& fn, size_t min = 1, size_t max = std::dynamic_extent ) {
			return input.read_and<F>( std::forward<F>( fn ), min, max );
		}
		async_stream_state* ref_state() {
			return input.ref_state();
		}
	};

	// Type erased async stream.
	//
	namespace detail {
		using cb_async_write = function_view<void( vec_buffer& )>;
		using cb_async_read =  function_view<void( vec_buffer&, size_t )>;
		template<typename R, typename... Ax>
		using cb_asfn_t = R(*)( void* ctx, Ax... args );

		// Traits list.
		//
		struct async_stream_traits {
			cb_asfn_t<async_stream_state*>                                                ref_state = nullptr;
			cb_asfn_t<async_stream::writer,                cb_async_write>                write_and = nullptr;
			cb_asfn_t<async_stream::reader<cb_async_read>, cb_async_read, size_t, size_t> read_and = nullptr;
			cb_asfn_t<void>                                                               dtor = nullptr;
		};
		template<typename U>
		struct async_stream_traits_for : async_stream_traits {
			constexpr async_stream_traits_for() {
				this->ref_state = &_ref_state;
				this->write_and = &_write_and;
				this->read_and =  &_read_and;
				this->dtor =      &_dtor;
			}
			static async_stream_state* _ref_state( void* ptr ) {
				return ( (U*) ptr )->ref_state();
			}
			static async_stream::writer _write_and( void* ptr, cb_async_write fn ) {
				return ( (U*) ptr )->write_and( std::move( fn ) );
			}
			static async_stream::reader<cb_async_read> _read_and( void* ptr, cb_async_read fn, size_t min, size_t max ) {
				return ( (U*) ptr )->read_and( std::move( fn ), min, max );
			}
			static void _dtor( void* ptr ) {
				delete (U*) ptr;
			}
		};
		template<typename U>
		inline constexpr async_stream_traits async_stream_traits_v = async_stream_traits_for<U>{};
	};

	// Type erased viewing type.
	//
	struct async_stream_view : async_stream_utils<async_stream_view> {
		// Constructors and comparison.
		//
		async_stream_view() = default;
		async_stream_view( std::nullptr_t ) {}
		async_stream_view( async_stream_view&& ) noexcept = default;
		async_stream_view( const async_stream_view& ) noexcept = default;
		async_stream_view& operator=( async_stream_view&& ) noexcept = default;
		async_stream_view& operator=( const async_stream_view& ) noexcept = default;
		bool operator==( const async_stream_view& o ) const { return ptr == o.ptr; };
		bool operator<( const async_stream_view& o ) const { return ptr <= o.ptr; };

		// By typed pointer.
		//
		template<typename U>
		async_stream_view( U* ptr ) { this->reset( ptr ); }
		void reset() {
			this->ptr =    nullptr;
			this->traits = nullptr;
			this->state =  nullptr;
		}
		template<typename U>
		void reset( U* ptr ) {
			this->ptr =    ptr;
			this->traits = &detail::async_stream_traits_v<U>;
			this->state =  this->traits->ref_state( ptr );
		}

		// Observers.
		//
		template<typename U>
		bool is() const { return traits == &detail::async_stream_traits_v<U>; }
		bool has_value() const { return ptr != nullptr; }
		explicit operator bool() const { return has_value(); }

		void* address() { return ptr; }
		template<typename U>
		U& get() { return *(U*) ptr; }
		template<typename U>
		U* get_if() { return this->template is<U>() ? (U*) ptr : nullptr; }
		
		const void* address() const { return ptr; }
		template<typename U>
		const U& get() const { return const_cast<async_stream_view*>( this )->template get<U>(); }
		template<typename U> 
		const U* get_if() const { return const_cast<async_stream_view*>( this )->template get_if<U>(); }
		
		
		// Implement the interface.
		//
		async_stream::writer write_and( detail::cb_async_write&& fn ) {
			return traits->write_and( ptr, fn );
		}
		async_stream::reader<detail::cb_async_read> read_and( detail::cb_async_read&& fn, size_t min = 1, size_t max = std::dynamic_extent ) {
			return traits->read_and( ptr, fn, min, max );
		}
		async_stream_state* ref_state() {
			return state;
		}
	protected:
		void* ptr = nullptr;
		const detail::async_stream_traits* traits = nullptr;
		async_stream_state* state = nullptr;
	};

	// Type erased owning type.
	//
	struct unique_async_stream : async_stream_view {
		// Construction by pointer.
		//
		unique_async_stream() = default;
		unique_async_stream( std::nullptr_t ) {}
		template<typename U> 
		unique_async_stream( U* ptr ) : async_stream_view( ptr ) {}
		
		// Move construction, no copy.
		//
		unique_async_stream( unique_async_stream&& o ) noexcept {
			swap( o );
		}
		unique_async_stream& operator=( unique_async_stream&& o ) noexcept {
			swap( o );
			return *this;
		}
		unique_async_stream( const unique_async_stream& ) = delete;
		unique_async_stream& operator=( const unique_async_stream& ) = delete;
		void swap( unique_async_stream& o ) {
			std::swap( ptr,    o.ptr );
			std::swap( traits, o.traits );
			std::swap( state,  o.state );
		}

		// Comparison.
		//
		explicit operator bool() const { return async_stream_view::has_value(); }
		bool operator==( const unique_async_stream& o ) const { return ptr == o.ptr; };
		bool operator<( const unique_async_stream& o ) const { return ptr <= o.ptr; };

		// Override reset to delete.
		//
		void reset() {
			unique_async_stream tmp{};
			swap( tmp );
		}
		template<typename U>
		void reset( U* ptr ) {
			unique_async_stream tmp{ ptr };
			swap( tmp );
		}
		~unique_async_stream() {
			if ( ptr ) traits->dtor( ptr );
		}
	};
};