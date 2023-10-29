#pragma once
#include "result.hpp"
#include "vec_buffer.hpp"
#include "spinlock.hpp"
#include "ref_counted.hpp"

namespace xstd {
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
		virtual void on_drain( xstd::vec_buffer& data, size_t hint ) {}

		// Stream closed (e.g. TCP connect / reset).
		//
		virtual void on_close( const exception& ex ) {}

		// Stream is outputting data, if controller does not consume it by changing its range,
		// duplex will buffer any leftovers, where controller should manually consume again if the
		// intention was to defer its processing.
		//
		virtual void on_input( xstd::vec_buffer& data ) {}
	};

	// Stream state.
	//
	enum class duplex_state : uint8_t {
		opening = 0,
		ready =   1,
		closing = 2,
		closed =  3,
	};

	// Stream source.
	//
	struct duplex_user;
	struct duplex : ref_counted<duplex> {
		friend struct duplex_user;
		mutable xstd::recursive_xspinlock<2> mtx;

		// Interface implemented by the underlying source.
		//
	protected:
		virtual void terminate() {}
		virtual bool try_close() { return false; }
		virtual void try_output( std::span<const uint8_t>& data ) {}

		// Internal state.
		//
	private:
		duplex_state         state =    duplex_state::opening;
		duplex_consumer*     consumer = nullptr;
		xstd::exception      close_reason = {};
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
			if ( consumer ) {
				consumer->on_ready();
			}
		}
		size_t on_drain( size_t hint = std::dynamic_extent ) {
			std::unique_lock lock{ mtx };
			needs_drain = false;
			return flush_write( hint );
		}
		void on_close( const exception& ex ) {
			std::unique_lock lock{ mtx };
			if ( state != duplex_state::closed ) {
				state = duplex_state::closed;
				if ( consumer ) {
					consumer->on_close( ex );
				}
			}
		}
		void on_input( std::span<const uint8_t> data, bool force_buffer = false ) {
			std::unique_lock lock{ mtx };
			recv_buffer.append_range( data );
			if ( !force_buffer && consumer ) {
				if ( !consumer->paused() )
					consumer->on_input( recv_buffer );
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
				return close_reason;
			} else {
				return std::nullopt;
			}
		}

		// Closes the stream.
		//
		void close( xstd::exception reason = XSTD_ESTR( "stream closed" ) ) {
			std::unique_lock lock{ mtx };
			if ( state >= duplex_state::closing ) {
				return;
			}
			close_reason = std::move( reason );
			state = duplex_state::closing;
			if ( !try_close() ) {
				state = duplex_state::closed;
				auto ref = this->add_ref();
				if ( consumer ) {
					consumer->on_close( close_reason );
				}
				ref->terminate();
			}
		}

		// Destroys the stream.
		//
		void destroy( xstd::exception reason = XSTD_ESTR( "stream closed" ) ) {
			std::unique_lock lock{ mtx };
			if ( state == duplex_state::closed ) {
				return;
			} else if ( state != duplex_state::closing ) {
				close_reason = std::move( reason );
			}
			state = duplex_state::closed;
			auto ref = this->add_ref();
			if ( consumer ) {
				consumer->on_close( close_reason );
			}
			ref->terminate();
		}

		// Reads more data from the stream.
		//
		void flush_read() {
			std::unique_lock lock{ mtx };
			if ( consumer ) {
				if ( !consumer->paused() )
					consumer->on_input( recv_buffer );
			}
		}

		// Outputs more data to the stream.
		//
		bool flush_write( size_t watermark_hint = std::dynamic_extent, bool force = false ) {
			std::unique_lock lock{ mtx };

			// Read more from the consumer.
			//
			if ( watermark_hint >= send_buffer.size() ) {
				if ( consumer ) {
					consumer->on_drain( this->send_buffer, watermark_hint - send_buffer.size() );
				}
			}

			if ( state != duplex_state::ready || corked() || ( !force && needs_drain ) ) {
				return false;
			}

			std::span<const uint8_t> range{ send_buffer };
			try_output( range );
			needs_drain = !range.empty();
			if ( range.empty() ) {
				send_buffer.clear();
				return true;
			} else {
				send_buffer.shrink_resize_reverse( range.size() );
				return false;
			}
		}

		// Writes data to the stream, returns true if data did not require buffering.
		//
		bool write( std::span<const uint8_t> data, bool force_buffer = false ) {
			std::unique_lock lock{ mtx };
			if ( state == duplex_state::closed ) {
				return true;
			}
			force_buffer = force_buffer || corked() || state == duplex_state::opening || needs_drain;
			if ( send_buffer || force_buffer ) {
				send_buffer.append_range( data );
				if ( force_buffer ) return false;
				return flush_write( 0 );
			} else {
				try_output( data );
				needs_drain = !data.empty();
				if ( !needs_drain ) {
					return true;
				}
				send_buffer.append_range( data );
				return false;
			}
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
		xstd::recursive_xspinlock<2>& mtx;
	private:
		std::unique_lock<xstd::recursive_xspinlock<2>> _ctor_lock{ mtx };
		duplex_state _pending_state;
	protected:
		duplex_user( ref<duplex> dup ) : stream( std::move( dup ) ), mtx( stream->mtx ) {
			fassert( !stream->consumer );
			stream->consumer = this;
			_pending_state = stream->state;
		}
		void begin() {
			if ( _pending_state == duplex_state::closing || _pending_state == duplex_state::closed ) {
				this->on_close( stream->close_reason );
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
