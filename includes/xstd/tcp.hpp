#pragma once
#include <string>
#include <array>
#include <mutex>
#include <vector>
#include <deque>
#include <string_view>
#include "spinlock.hpp"
#include "scope_tpr.hpp"
#include "type_helpers.hpp"
#include "future.hpp"
#include "coro.hpp"

// [[Configuration]]
// XSTD_SOCKET_TASK_PRIORITY: Task priority set when acquiring TCP related locks.
//
#ifndef XSTD_SOCKET_TASK_PRIORITY
	#define XSTD_SOCKET_TASK_PRIORITY 2
#endif

namespace xstd::tcp
{
	struct packet_processor_state
	{
		size_t             skip_count = 0;
		size_t             size_requested = 0;
		bool               retry_on_timer = false;
		std::string_view   last_view = {};
		coroutine_handle<> continuation = {};
	};
	struct packet_awaitable
	{
		packet_processor_state* ref;
		size_t n;

		inline bool await_ready() { return !ref; }
		inline void await_suspend( coroutine_handle<> hnd ) { ref->size_requested = n; ref->continuation = hnd; }
		inline std::string_view await_resume() { return ref ? ref->last_view : std::string_view{}; }
	};

	struct client
	{
		// Promise fulfilled once the socket is closed.
		//
		future<exception, void> socket_closed = nullptr;

		// Receive buffer.
		//
		size_t rx_buffer_offset = 0;
		std::vector<uint8_t> rx_buffer = {};

		// Transmission queues.
		//
		spinlock tx_lock = {};
		size_t last_ack_id = 0;
		size_t last_tx_id = 0;
		std::deque<std::pair<std::vector<uint8_t>, size_t>> tx_queue;
		std::deque<std::pair<std::vector<uint8_t>, size_t>> ack_queue;

		// Packet processor state.
		//
		packet_processor_state pp = {};

		// Whether or not the client has acknowledgment control.
		//
		const bool has_ack;
		inline client( bool has_ack ) : has_ack( has_ack ) {}

		// Implemented by network layer, starts the connection.
		//
		virtual future<> connect( const std::array<uint8_t, 4>& ip, uint16_t port ) = 0;

		// Implemented by network layer, tries to send the data given over the socket, returns 
		// non-zero size if data is (partially?) sent, else returns zero indicating error.
		//
		virtual size_t socket_write( any_ptr data, size_t length, bool last_segment ) = 0;
		
		// Implemented by network layer, forces the socket to exhaust the internal output queue.
		//
		virtual void socket_writeback() = 0;

		// Implemented by network layer, closes the connection.
		//
		virtual bool socket_close() = 0;

		// Enables or disables the nagle's algorithm.
		//
		virtual bool socket_set_nagle( bool ) { return false; }

		// Internal function used to flush the queues.
		// - Caller must hold the lock.
		//
		void flush_queues()
		{
			// Write back the transaction queue.
			//
			bool wb_retry = true;
			while( !tx_queue.empty() )
			{
				// Try writing it to the socket.
				//
				auto& [buffer, written] = tx_queue.front();
				size_t count = socket_write( buffer.data() + written, buffer.size() - written, tx_queue.size() == 1 );
				written += count;
				last_tx_id += count;

				// If none were written, invoke writeback to flush.
				//
				if ( !count && wb_retry )
				{
					socket_writeback();
					wb_retry = false;
					continue;
				}

				// If we've not written the entire buffer, break.
				//
				if ( written != buffer.size() )
					break;

				// Move the data over to the acknowledgment queue and erase it from the current queue.
				//
				if ( has_ack )
					ack_queue.emplace_back( std::move( buffer ), last_tx_id );
				tx_queue.pop_front();
			}
		}

		// Invoked by network layer to do periodic operations.
		//
		virtual void on_timer()
		{
			// Handle pending packet processing.
			//
			if ( std::exchange( pp.retry_on_timer, false ) ) [[unlikely]]
				on_socket_receive( {} );

			// Flush the queues.
			//
			if ( tx_queue.empty() )
				return;
			xstd::task_lock g{ tx_lock, XSTD_SOCKET_TASK_PRIORITY };
			flush_queues();
		}

		// Invoked by application to write data to the socket.
		//
		void write( std::vector<uint8_t> data )
		{
			xstd::task_lock g{ tx_lock, XSTD_SOCKET_TASK_PRIORITY };

			// Append the data onto tx queue, flush the queues.
			//
			tx_queue.emplace_back( std::move( data ), 0 );
			flush_queues();
		}

		// Invoked by network layer to indicate the target acknowledged a number of bytes from our output queue.
		// - if HasAck == 0, can be used to indicate an arbitrary amount of data can be written again.
		//
		void on_socket_ack( size_t n )
		{
			xstd::task_lock g{ tx_lock, XSTD_SOCKET_TASK_PRIORITY };

			// Clear the acknowledgment queue.
			//
			if ( n )
			{
				last_ack_id += n;
				while ( !ack_queue.empty() && ack_queue.front().second <= last_ack_id )
					ack_queue.pop_front();
			}

			// Flush the queues.
			//
			flush_queues();
		}

		// Receives the data, if given of an expected size.
		// -- Must be only called by a single consumer.
		//
		auto recv( size_t n = 0 ) 
		{
			if ( is_closed() ) return packet_awaitable{ nullptr, 0 };
			else               return packet_awaitable{ &pp, n };
		}

		// Marks data processed.
		//
		void forward( size_t n ) { pp.skip_count += n; }
		void forward_to( std::string_view iterated ) { pp.skip_count = pp.last_view.size() - iterated.size(); }

		// Invoked by the network layer on the close event to indicate that the socket has closed.
		//
		void on_close()
		{
			if ( pp.continuation )
			{
				pp.last_view = {};
				std::exchange( pp.continuation, nullptr )( );
			}
		}

		// Invoked by network layer to indicate the socket received data.
		//
		void on_socket_receive( std::string_view segment )
		{
			if ( this->is_closed() ) return;

			auto packet_parse = [ & ] ( std::string_view new_data ) -> size_t
			{
				// If no continuation set, return 0 and set retry flag.
				//
				if ( !pp.continuation )
				{
					pp.retry_on_timer = true;
					return 0;
				}

				if ( size_t req = pp.size_requested )
				{
					if ( new_data.size() < req )
						return 0;
					pp.last_view = new_data.substr( 0, req );
					std::exchange( pp.continuation, nullptr )( );
					pp.last_view = {};
					return req;
				}
				else
				{
					pp.last_view = new_data;
					std::exchange( pp.continuation, nullptr )( );
					pp.last_view = {};
					return std::exchange( pp.skip_count, 0 );
				}
			};

			// If receive buffer is empty, try parsing the segment without any copy.
			//
			if ( rx_buffer.empty() )
			{
				if ( segment.empty() )
					return;

				// While packets are parsed:
				//
				while ( size_t n = packet_parse( segment ) )
				{
					// Remove the consumed range and return if empty.
					//
					segment.remove_prefix( n );
					if ( segment.empty() )
						return;
				}

				// Append the rest to the receive buffer.
				//
				rx_buffer.insert( rx_buffer.end(), segment.begin(), segment.end() );
			}
			else
			{
				// Append the entire segment to the receive buffer.
				//
				rx_buffer.insert( rx_buffer.end(), segment.begin(), segment.end() );

				// While packets are parsed:
				//
				std::string_view it = { ( char* ) rx_buffer.data() + rx_buffer_offset, ( char* ) rx_buffer.data() + rx_buffer.size() };
				while ( size_t n = packet_parse( it ) )
				{
					// Remove the consumed.
					//
					it.remove_prefix( n );
					
					// If we finally consumed the entire buffer, reset it and return.
					//
					if ( it.empty() )
					{
						rx_buffer_offset = 0;
						rx_buffer.clear();
						return;
					}
				}

				// If rx buffer has leftover segments, adjust the offset, this is done to 
				// avoid unnecessary memory movement.
				//
				rx_buffer_offset = ( uint8_t* ) it.data() - rx_buffer.data();
			}
		}

		// Expose the socket state.
		//
		bool is_closed() const
		{
			return socket_closed.fulfilled();
		}
	};
};