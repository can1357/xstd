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
	private:
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
		virtual void on_drain( size_t hint ) {}

		// Stream closed (e.g. TCP connect / reset).
		//
		virtual void on_close( const exception& ex ) {}

		// Stream is outputting data, if controller does not consume it by changing its range,
		// duplex will buffer any leftovers, where controller should manually consume again if the
		// intention was to defer its processing.
		//
		virtual void on_input( std::span<const uint8_t>& data ) {}
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
	struct duplex : ref_counted<duplex> {
		// Interface implemented by the underlying source.
		//
	protected:
		virtual void terminate() {}
		virtual bool try_close() { return false; }
		virtual void try_output( std::span<const uint8_t>& data ) {}

		// Internal state.
		//
	private:
		mutable xstd::recursive_xspinlock<2> mtx;
		duplex_state         state =    duplex_state::opening;
		duplex_consumer*     consumer = nullptr;
		xstd::exception      close_reason = {};
		std::atomic<int16_t> cork_count = 0;

		// Recv/Send buffers.
		//
		xstd::vec_buffer  recv_buffer = {};
		xstd::vec_buffer  send_buffer = {};

		// Implement an interface similar to that of consumer but also including buffering.
		//
	protected:
		void on_ready() {
			std::unique_lock lock{ mtx };
			dassert( state == duplex_state::opening );
			state = duplex_state::ready;
			if ( consumer )
				consumer->on_ready();
		}
		void on_drain( size_t hint = std::dynamic_extent ) {
			std::unique_lock lock{ mtx };
			hint -= flush_write( hint );
			if ( hint && consumer ) {
				consumer->on_drain( hint );
			}
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
		void on_input( std::span<const uint8_t> data ) {
			std::unique_lock lock{ mtx };
			if ( paused() ) {
				recv_buffer.append_range( data );
				return;
			}
			if ( recv_buffer ) {
				recv_buffer.append_range( data );
				flush_read();
			} else {
				consumer->on_input( data );
				recv_buffer.append_range( data );
			}
		}
	public:
		// Corking.
		//
		bool corked() const { 
			return cork_count.load( std::memory_order::relaxed ) > 0; 
		}
		bool paused() const {
			auto c = std::atomic_ref{ consumer }.load( std::memory_order::relaxed );
			return !c || c->paused(); 
		}
		bool cork() {
			return !cork_count++; 
		}
		bool uncork() {
			if ( --cork_count )
				return false;
			std::unique_lock lock{ mtx };
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
				if ( consumer ) consumer->on_close( close_reason );
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
			if ( consumer ) consumer->on_close( close_reason );
			ref->terminate();
		}

		// Reads more data from the stream.
		//
		void flush_read( size_t hint = std::dynamic_extent ) {
			std::unique_lock lock{ mtx };
			if ( paused() ) return;

			hint = std::min( recv_buffer.size(), hint );
			std::span<const uint8_t> range{ recv_buffer.subspan( 0, hint ) };
			consumer->on_input( range );
			hint -= range.size();
			recv_buffer.shift( hint );
			if ( recv_buffer.empty() ) {
				recv_buffer.clear();
			}
		}

		// Outputs more data to the stream, returns the number of bytes removed from the internal buffer.
		//
		size_t flush_write( size_t hint = std::dynamic_extent ) {
			std::unique_lock lock{ mtx };
			if ( state != duplex_state::ready || corked() )
				return 0;
			
			hint = std::min( send_buffer.size(), hint );
			if ( hint ) {
				std::span<const uint8_t> range{ send_buffer.subspan( 0, hint ) };
				try_output( range );
				hint -= range.size();
				send_buffer.shift( hint );
				if ( send_buffer.empty() ) {
					send_buffer.clear();
				}
			}
			return hint;
		}

		// Writes data to the stream, returns true if data did not require buffering.
		//
		bool write( std::span<const uint8_t> data ) {
			std::unique_lock lock{ mtx };
			if ( state >= duplex_state::closing ) {
				return true;
			}
			if ( state != duplex_state::opening && !send_buffer && !corked() ) {
				try_output( data );
			}
			if ( !data.empty() ) {
				send_buffer.append_range( data );
				return false;
			}
			return true;
		}
		bool write( xstd::vec_buffer data ) {
			std::unique_lock lock{ mtx };
			if ( state >= duplex_state::closing ) {
				return true;
			}
			if ( state != duplex_state::opening && !send_buffer && !corked() ) {
				std::span<const uint8_t> range{ data };
				try_output( range );
				data.shrink_resize_reverse( range.size() );
			}
			if ( !data.empty() ) {
				if ( !send_buffer )
					send_buffer = std::move( data );
				else
					send_buffer.append_range( data );
				return false;
			}
			return true;
		}

		// Acquires the stream for a new consumer / detaches from the current one.
		//
		xstd::exception set_consumer( duplex_consumer* new_consumer ) {
			std::unique_lock lock{ mtx };
			if ( !new_consumer ) {
				auto prev = std::exchange( consumer, nullptr );
				if ( prev && prev->cork_count > 0 )
					this->uncork();
				return {};
			} else if ( state > duplex_state::closing ) {
				return close_reason;
			} else if ( consumer ) {
				return XSTD_ESTR( "stream is already being consumed" );
			}

			consumer = new_consumer;
			if ( new_consumer->cork_count > 0 )
				this->cork();
			if ( state == duplex_state::ready ) {
				new_consumer->on_ready();
				this->flush_read();
			}
			return std::nullopt;
		}

		// Virtual destructor.
		//
		virtual ~duplex() = default;
	};

	// Stream user.
	//
	template<typename T = duplex> requires std::is_base_of_v<duplex, T>
	struct duplex_user : duplex_consumer {
	protected:
		ref<T> stream = nullptr;
		
	public:
		bool cork() {
			if ( cork_count++ )
				return false;
			if ( stream )
				stream->cork();
			return true;
		}
		bool uncork() {
			if ( --cork_count )
				return false;
			if ( stream )
				stream->uncork();
			return true;
		}
		bool pause() {
			return !pause_count++;
		}
		bool unpause() {
			if ( --pause_count )
				return false;
			if ( stream )
				stream->flush_read();
			return true;
		}

		// Changes the bound stream.
		//
		xstd::exception bind( ref<T> new_stream ) {
			if ( stream == new_stream )
				return {};
			if ( auto err = new_stream->set_consumer( this ) )
				return err;
			if ( stream && stream.ref_count() == 1 ) {
				stream->destroy();
			}
			stream = std::move( new_stream );
			return {};
		}
		void unbind() {
			if ( stream && stream.ref_count() == 1 ) {
				stream->destroy();
				stream = {};
			}
		}

	public:
		~duplex_user() {
			this->unbind();
		}
	};
};
