#pragma once
#include "spinlock.hpp"
#include "coro.hpp"
#include "vec_buffer.hpp"
#include "result.hpp"
#include "event.hpp"

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
		auto read( size_t min, size_t max ) {
			return static_cast<Self*>( this )->read_and(
				[]( vec_buffer& buf, size_t count ) FORCE_INLINE{
					vec_buffer result;
					if ( buf.size() > count ) {
						result = { buf.subspan( 0, count ) };
						buf.shift( count );
					} else {
						result.swap( buf );
					}
					return result;
				},
				min, max
			);
		}
		auto read() {
			return read( 0, std::dynamic_extent );
		}
		auto read( size_t len ) {
			return read( len, len );
		}
		auto read_into( std::span<uint8_t> dst, size_t min ) {
			return static_cast<Self*>( this )->read_and(
				[out = dst.data()]( vec_buffer& buf, size_t count ) FORCE_INLINE{
					buf.shift_range( { out, count } );
					return count;
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
			inline decltype( auto ) await_resume() const noexcept {
				if ( !lock ) lock.lock();
				return std::move( fn )( stream.buffer, std::clamp( stream.buffer.size(), lower, upper ) );
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
};