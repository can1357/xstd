#pragma once
#include "result.hpp"
#include "vec_buffer.hpp"
#include "spinlock.hpp"
#include "ref_counted.hpp"
#include "time.hpp"
#include <mutex>

// [[Configuration]]
// XSTD_IO_TPR: Task priority for I/O streams.
//
#ifndef XSTD_IO_TPR
	#define XSTD_IO_TPR 2
#endif
namespace xstd {
#if KERNEL_TARGET
	using io_mutex = recursive_xspinlock<2>;
#else
	using io_mutex = std::recursive_mutex;
#endif

	// Stream consumer.
	//
	struct duplex;
	struct duplex_consumer {
		friend struct duplex;

		// Pause state.
		//
	protected:
		int16_t pause_count = 0;
		int16_t cork_count = 0;
	public:
		bool paused() const { return pause_count > 0; }
		bool corked() const { return cork_count > 0; }

		// Interface implemented by the consumer.
		//
	protected:
		// Stream became writable (e.g. Connected for TCP).
		//
		virtual void on_ready() {}

		// Stream has finished the written data, more data can be written directly. (e.g. TCP write buffer end).
		//
		virtual void on_drain( xstd::vec_buffer& data,  size_t hint ) {
			(void) data;
			(void) hint;
		}

		// Stream closed (e.g. TCP connect / reset).
		//
		virtual void on_close( const exception& ex ) {
			(void) ex;
		}

		// Stream is outputting data, if controller does not consume it by changing its range,
		// duplex will buffer any leftovers, where controller should manually consume again if the
		// intention was to defer its processing.
		//
		virtual void on_input( xstd::vec_buffer& data ) {
			(void) data;
		}
	};

	// Stream state.
	//
	enum class duplex_state : uint8_t {
		opening = 0,
		ready =   1,
		closing = 2,
		closed =  3,
	};

	// Stream statistics.
	//
	struct duplex_stats {
		size_t drain_count = 0;
		size_t drain_wait_count = 0;

		size_t send_count = 0;
		size_t write_count = 0;
		size_t bytes_sent = 0;
		size_t bytes_written = 0;

		size_t recv_count = 0;
		size_t read_count = 0;
		size_t bytes_recv = 0;
		size_t bytes_read = 0;

		xstd::timestamp create_time = xstd::time::now();
		xstd::timestamp ready_time = {};
		xstd::timestamp close_time = {};
		xstd::timestamp last_recv_time = create_time;
		xstd::timestamp last_send_time = create_time;
	};

	// Stream source.
	//
	struct duplex_user;
	struct duplex : ref_counted<duplex> {
		friend struct duplex_user;
		mutable io_mutex mtx;
		duplex_stats     stats = {};

		// Interface implemented by the underlying source.
		//
	protected:
		virtual void terminate() {}
		virtual bool try_close() { return false; }
		virtual bool try_output( vec_buffer& data ) { return false; }

		// Internal state.
		//
	protected:
		duplex_state         state =    duplex_state::opening;
		duplex_consumer*     consumer = nullptr;
		xstd::exception      error = {};
		std::atomic<int16_t> cork_count = 0;
		bool                 needs_drain = false;

		// Recv/Send buffers.
		//
		xstd::vec_buffer  recv_buffer = {};
		xstd::vec_buffer  send_buffer = {};

		// Implement an interface similar to that of consumer but also including buffering for the implementor's use.
		//
	public:
		void on_ready() {
			std::unique_lock lock{ mtx };
			dassert( state == duplex_state::opening );
			state = duplex_state::ready;
			stats.ready_time = xstd::time::now();
			if ( consumer ) {
				consumer->on_ready();
			}
		}
		size_t on_drain( size_t hint = std::dynamic_extent ) {
			std::unique_lock lock{ mtx };
			++stats.drain_count;
			needs_drain = false;
			return flush_write( hint );
		}
		void on_close( const exception& ex ) {
			std::unique_lock lock{ mtx };
			if ( state != duplex_state::closed ) {
				stats.close_time = xstd::time::now();
				state = duplex_state::closed;
				if ( consumer ) {
					consumer->on_close( ex );
				}
			}
		}
		void on_input( std::span<const uint8_t> data, bool force_buffer = false ) {
			std::unique_lock lock{ mtx };
			stats.recv_count++;
			stats.last_recv_time = xstd::time::now();
			stats.bytes_recv += data.size();
			recv_buffer.append_range( data );
			if ( !force_buffer && consumer ) {
				if ( !consumer->paused() ) {
					stats.read_count++;
					size_t count = recv_buffer.size();
					consumer->on_input( recv_buffer );
					stats.bytes_read += count - recv_buffer.size();
				}
			}
		}

		// Corking.
		//
		bool corked() const { 
			return cork_count.load( std::memory_order::relaxed ) > 0; 
		}
		bool cork() {
			return !cork_count++; 
		}
		bool uncork() {
			if ( --cork_count )
				return false;
			flush_write();
			return true;
		}

		// Observers.
		//
		duplex_state current_state() const { return state; }
		bool opening() const { return current_state() == duplex_state::opening; }
		bool ready() const { return current_state() == duplex_state::ready; }
		bool closed() const { return current_state() == duplex_state::closed; }
		bool closing() const { return current_state() == duplex_state::closing; }
		bool ended() const { return current_state() >= duplex_state::closing; }
		bool is_recv_buffering() const {
			std::unique_lock lock{ mtx };
			return !recv_buffer.empty();
		}
		bool is_send_buffering() const {
			std::unique_lock lock{ mtx };
			return !send_buffer.empty();
		}
		xstd::exception get_error() const {
			std::unique_lock lock{ mtx };
			if ( state >= duplex_state::closing ) {
				return error;
			} else {
				return std::nullopt;
			}
		}
		duplex_stats get_stats() const {
			std::unique_lock lock{ mtx };
			return stats;
		}

		// Closes the stream.
		//
		void close() {
			std::unique_lock lock{ mtx };
			if ( state >= duplex_state::closing ) {
				return;
			}
			state = duplex_state::closing;
			if ( !try_close() ) {
				state = duplex_state::closed;
				auto ref = this->add_ref();
				if ( consumer ) {
					consumer->on_close( error );
				}
				ref->terminate();
			}
		}

		// Destroys the stream.
		//
		void destroy( xstd::exception reason = XSTD_ESTR( "stream destroyed" ) ) {
			std::unique_lock lock{ mtx };
			if ( state == duplex_state::closed ) {
				return;
			} else if ( state != duplex_state::closing ) {
				error = std::move( reason );
			}
			state = duplex_state::closed;
			auto ref = this->add_ref();
			if ( consumer ) {
				consumer->on_close( error );
			}
			ref->terminate();
		}

		// Reads more data from the stream.
		//
		void flush_read() {
			std::unique_lock lock{ mtx };
			if ( consumer ) {
				if ( !consumer->paused() ) {
					stats.read_count++;
					size_t count = recv_buffer.size();
					consumer->on_input( recv_buffer );
					stats.bytes_read += count - recv_buffer.size();
				}
			}
		}

		// Outputs more data to the stream, returns true if data did not require buffering.
		//
		bool flush_write( size_t watermark_hint = std::dynamic_extent ) {
			std::unique_lock lock{ mtx };
			if ( state == duplex_state::closed ) {
				return true;
			}

			// Read more from the consumer.
			//
			if ( watermark_hint >= send_buffer.size() ) {
				if ( consumer ) {
					++stats.write_count;
					size_t count = send_buffer.size();
					consumer->on_drain( this->send_buffer, watermark_hint - send_buffer.size() );
					count = send_buffer.size() - count;
					stats.bytes_written += count;
				}
			}

			// If not open yet, corked, or needs drain skip.
			//
			if ( state == duplex_state::opening || corked() || needs_drain ) {
				return false;
			}

			++stats.send_count;
			size_t count = send_buffer.size();
			bool needs_drain_explicit = try_output( send_buffer );
			stats.last_send_time = xstd::time::now();
			stats.bytes_sent += count - send_buffer.size();
			if ( send_buffer.empty() ) {
				needs_drain = needs_drain_explicit;
				return true;
			}

			needs_drain = true;
			++stats.drain_wait_count;
			return false;
		}

		// Writes data to the stream, returns true if data did not require buffering.
		//
		bool write( std::span<const uint8_t> data, bool force_buffer = false ) {
			std::unique_lock lock{ mtx };
			if ( state == duplex_state::closed ) {
				return true;
			}
			++stats.write_count;
			stats.bytes_written += data.size();
			send_buffer.append_range( data );
			if ( force_buffer ) return false;
			return flush_write( 0 );
		}

		// Virtual destructor.
		//
		virtual ~duplex() = default;
	};

	// Stream user.
	//
	struct duplex_user : duplex_consumer {
	public:
		const ref<duplex> stream;
		io_mutex& mtx;
	private:
		std::unique_lock<io_mutex> _ctor_lock{ mtx };
		duplex_state _pending_state;
	protected:
		duplex_user( ref<duplex> dup ) : stream( std::move( dup ) ), mtx( stream->mtx ) {
			fassert( !stream->consumer );
			stream->consumer = this;
			_pending_state = stream->state;
		}
		void begin() {
			if ( _pending_state == duplex_state::closing || _pending_state == duplex_state::closed ) {
				this->on_close( stream->error );
			} else if ( _pending_state == duplex_state::ready ) {
				this->on_ready();
			}
			_ctor_lock.unlock();
		}
		void end() {
			stream->consumer = nullptr;
			if ( stream.ref_count() == 1 )
				stream->destroy();
		}

	public:
		bool cork() {
			if ( cork_count++ )
				return false;
			stream->cork();
			return true;
		}
		bool uncork() {
			if ( --cork_count )
				return false;
			stream->uncork();
			return true;
		}
		bool pause() {
			return !pause_count++;
		}
		bool unpause() {
			if ( --pause_count )
				return false;
			stream->flush_read();
			return true;
		}

	public:
		~duplex_user() {
			this->end();
		}
	};
};
