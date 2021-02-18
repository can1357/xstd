#pragma once
#include <list>
#include <string>
#include <array>
#include <mutex>
#include <string_view>
#include "type_helpers.hpp"
#include "promise.hpp"

namespace xstd::tcp
{
	template<bool has_ack = true>
	struct client
	{
		// Socket state.
		//
		bool closed = true;

		// Receive buffer.
		//
		size_t rx_buffer_offset = 0;
		std::string rx_buffer;

		// Transmission queues.
		//
		std::recursive_mutex queue_lock;
		size_t last_ack_id = 0;
		size_t last_tx_id = 0;
		std::list<std::pair<std::string, size_t>> tx_queue;
		std::list<std::pair<std::string, size_t>> ack_queue;

		// Implemented by application, tries parsing the given data range as a singular 
		// packet returns non-zero size if data is (partially?) consumed, else returns zero 
		// indicating more data is needed.
		//
		virtual size_t packet_parse( std::string_view data ) = 0;

		// Implemented by network layer, tries to send the data given over the socket, returns 
		// non-zero size if data is (partially?) sent, else returns zero indicating error.
		//
		virtual size_t socket_write( any_ptr data, size_t length, bool last_segment ) = 0;
		
		// Implemented by network layer, forces the socket to exhaust the internal output queue.
		//
		virtual void socket_writeback() = 0;

		// Implemented by network layer, closes the connection.
		//
		virtual void socket_close() = 0;

		// Implemented by network layer, starts the connection.
		//
		virtual promise<> socket_connect( const std::array<uint8_t, 4>& ip, uint16_t port ) = 0;

		// Enables or disables the nagle's algorithm.
		//
		virtual bool socket_set_nagle( bool state ) { return false; }

		// Invoked by network layer to do periodic operations.
		//
		virtual void on_timer()
		{
			if ( closed ) return;

			std::lock_guard _g{ queue_lock };
			for ( auto it = tx_queue.begin(); it != tx_queue.end(); )
			{
				// Try writing it to the socket.
				//
				auto& [buffer, written] = *it;
				size_t count = socket_write( buffer.data() + written, buffer.size() - written, std::next( it ) == tx_queue.end() );
				written += count;
				last_tx_id += count;

				// If none were written, invoke writeback to flush.
				//
				if ( !count )
					socket_writeback();

				// If we've not written the entire buffer, break.
				//
				if ( written != buffer.size() )
					break;

				// Move the data over to the acknowledgment queue and erase it from the current queue.
				//
				if constexpr ( has_ack )
					ack_queue.emplace_back( std::move( buffer ), last_tx_id );
				it = tx_queue.erase( it );
			}
		}

		// Invoked by application to write data to the socket.
		//
		void write( std::string data )
		{
			if ( closed ) return;

			// Append the data onto tx queue.
			//
			queue_lock.lock();
			tx_queue.emplace_back( std::move( data ), 0 );
			queue_lock.unlock();

			// Invoke socket timer to exhaust the queue.
			//
			client::on_timer();
		}

		// Invoked by network layer to indicate a connection was started.
		//
		void on_socket_connect()
		{
			closed = false;
		}

		// Invoked by network layer to indicate the target acknowledged a number of bytes from our output queue.
		//
		void on_socket_ack( size_t n )
		{
			if ( closed ) return;

			// Delete every entry from acknowledgment queue where the id is below the new one.
			//
			std::lock_guard _g{ queue_lock };
			size_t new_id = ( last_ack_id += n );
			for ( auto it = ack_queue.begin(); it != ack_queue.end(); )
			{
				if ( it->second > new_id )
					break;
				it = ack_queue.erase( it );
			}
		}

		// Invoked by network layer to indicate the socket was closed.
		// - Can be overriden to hook the event.
		//
		virtual void on_socket_close()
		{
			// Reset the buffers.
			//
			closed = true;
			std::lock_guard _g{ queue_lock };
			tx_queue.clear();
			ack_queue.clear();
			rx_buffer.clear();
			rx_buffer_offset = 0;
			last_tx_id = 0;
			last_ack_id = 0;
		}

		// Invoked by network layer to indicate the socket received data.
		//
		void on_socket_receive( std::string_view segment )
		{
			if ( closed ) return;

			// If receive buffer is empty, try parsing the segment without any copy.
			//
			std::lock_guard _g{ queue_lock };
			if ( rx_buffer.empty() )
			{
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
				std::string_view it = { rx_buffer.begin() + rx_buffer_offset, rx_buffer.end() };
				while ( size_t n = packet_parse( it ) )
				{
					// Remove the consumed.
					//
					it.remove_prefix( n );
					
					// If we finally consumed the entire buffer, reset it and return.
					//
					if ( it.empty() )
					{
						rx_buffer.clear();
						return;
					}
				}

				// If rx buffer has leftover segments, adjust the offset, this is done to 
				// avoid unnecessary memory movement.
				//
				rx_buffer_offset = it.data() - rx_buffer.data();
			}
		}
	};
};