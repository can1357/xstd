#pragma once
#include <list>
#include <string>
#include <array>
#include <mutex>
#include <string_view>
#include "spinlock.hpp"
#include "type_helpers.hpp"
#include "promise.hpp"

namespace xstd::tcp
{
	template<bool has_ack = true>
	struct client
	{
		// Socket state.
		// -- Should be resolved by the network layer.
		//
		xstd::promise<> socket_closed = xstd::make_promise();
		xstd::promise<> socket_connected = xstd::make_promise();

		// Receive buffer.
		//
		size_t rx_buffer_offset = 0;
		std::vector<uint8_t> rx_buffer;
		xstd::spinlock rx_lock;

		// Transmission queues.
		//
		xstd::spinlock tx_lock;
		std::atomic<size_t> last_ack_id = 0;
		size_t last_tx_id = 0;
		std::list<std::pair<std::vector<uint8_t>, size_t>> tx_queue;
		std::list<std::pair<std::vector<uint8_t>, size_t>> ack_queue;

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

		// Internal function used to flush the queues, must be called with the lock.
		//
		void flush_queues()
		{
			// Clear the acknowledgment queue.
			//
			for ( auto it = ack_queue.begin(); it != ack_queue.end(); )
			{
				if ( it->second > last_ack_id )
					break;
				it = ack_queue.erase( it );
			}

			// Write back the transaction queue.
			//
			bool wb_retry = true;
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
				if constexpr ( has_ack )
					ack_queue.emplace_back( std::move( buffer ), last_tx_id );
				it = tx_queue.erase( it );
			}
		}

		// Invoked by network layer to do periodic operations.
		//
		virtual void on_timer()
		{
			if ( this->is_closed() ) return;

			// Flush all queues.
			//
			std::lock_guard _g{ tx_lock };
			flush_queues();
		}

		// Invoked by application to write data to the socket.
		//
		void write( std::vector<uint8_t> data )
		{
			if ( this->is_closed() ) return;

			// Append the data onto tx queue and try submitting to the network layer.
			//
			std::lock_guard _g{ tx_lock };
			tx_queue.emplace_back( std::move( data ), 0 );
			flush_queues();
		}

		// Invoked by network layer to indicate the target acknowledged a number of bytes from our output queue.
		// - if has_ack == 0, can be used to indicate an arbitrary amount of data can be written again.
		//
		void on_socket_ack( size_t n )
		{
			if ( this->is_closed() ) return;
			last_ack_id += n;

			std::lock_guard _g{ tx_lock };
			flush_queues();
		}

		// Invoked by network layer to indicate the socket received data.
		//
		void on_socket_receive( std::string_view segment )
		{
			if ( this->is_closed() ) return;

			// If receive buffer is empty, try parsing the segment without any copy.
			//
			std::unique_lock lock{ rx_lock };
			if ( rx_buffer.empty() )
			{
				// While packets are parsed:
				//
				lock.unlock();
				while ( size_t n = packet_parse( segment ) )
				{
					// Remove the consumed range and return if empty.
					//
					segment.remove_prefix( n );
					if ( segment.empty() )
						return;
				}
				lock.lock();

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
		bool is_connected() const
		{
			return !socket_closed->fulfilled() && socket_connected->fulfilled();
		}
		bool is_closed() const
		{
			return socket_closed->fulfilled() || socket_connected->failed();
		}
	};
};