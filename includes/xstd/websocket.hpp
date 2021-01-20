#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <optional>
#include "type_helpers.hpp"
#include "assert.hpp"

namespace xstd::ws
{
	static constexpr uint8_t length_extend_u16 = 126;
	static constexpr uint8_t length_extend_u64 = 127;

	// Websocket opcodes and the traits.
	//
	enum class opcode : uint8_t
	{
		continuation = 0,
		text =         1,
		binary =       2,
		close =        8,
		ping =         9,
		pong =        10,
		maximum =     16,
	};
	static constexpr bool is_control_opcode( opcode op )
	{
		return op >= opcode::close;
	}

	// Websocket status codes.
	//
	enum status_code : uint16_t
	{
		status_shutdown =          1000,
		status_going_away =        1001,
		status_protocol_error =    1002,
		status_unknown_operation = 1003,
		status_unknown =           1005, // Do not send.
		status_connection_reset =  1006, // Do not send.
		status_invalid_data =      1007,
		status_policy_violation =  1008,
		status_data_too_large =    1009,
		status_missing_extension = 1010,
		status_unexpected_error =  1011,
	};

	// Networked packet header.
	//
	struct net_header
	{
		opcode  op     : 4;
		uint8_t rsvd   : 3;
		uint8_t fin    : 1;

		uint8_t length : 7;
		uint8_t masked : 1;
	};
	static constexpr size_t max_net_header_size = sizeof( net_header ) + sizeof( uint64_t ) + sizeof( uint32_t );

	// Masking helper.
	//
	inline void mask_buffer( any_ptr ptr, size_t len, uint32_t key )
	{
		if ( !key ) return;
		uint8_t* data = ptr;
		auto* kb = ( const uint8_t* ) &key;
		for ( size_t n = 0; n != len; n++ )
			data[ n ] ^= kb[ n % 4 ];
	}

	// Parsed packet header.
	//
	struct header
	{
		// Opcode and packet length.
		//
		opcode op = opcode::maximum;
		size_t length = 0;
		bool finished = true;

		// Mask key, omitted if zero.
		//
		uint32_t mask_key = 0;

		// State helpers.
		//
		bool is_control_frame() const { return is_control_opcode( op ); }
	};

	// Reads a packet header from the given stream. 
	// - Return codes:
	//    0 = Success.
	//   <0 = Waiting for further data.
	//   >0 = Should terminate with status, protocol error.
	//
	inline int read( std::string_view& buffer, header& hdr )
	{
		std::string_view it = buffer;
		auto read = [ & ] <typename T> ( T& v ) -> bool
		{
			if ( it.length() < sizeof( T ) )
				return false;
			memcpy( &v, it.data(), sizeof( T ) );
			it.remove_prefix( sizeof( T ) );
			return true;
		};

		// Read the networked header.
		//
		net_header net;
		if ( !read( net ) )
			return -1;
		hdr.op = net.op;
		hdr.finished = net.fin;
		hdr.length = net.length;

		// Validate according to the RFC.
		//
		if ( net.rsvd )
			return status_protocol_error;
		if ( hdr.is_control_frame() && !hdr.finished )
			return status_protocol_error;

		// Read the extended length if relevant.
		//
		if ( hdr.length == length_extend_u16 )
		{
			if ( !read( ( uint16_t& ) hdr.length ) )
				return -1;
		}
		else if ( hdr.length == length_extend_u64 )
		{
			if ( !read( ( uint64_t& ) hdr.length ) )
				return -1;
			if ( hdr.length > INT64_MAX )
				return status_protocol_error;
		}

		// Read the masking key if relevant.
		//
		if ( net.masked )
		{
			if ( !read( hdr.mask_key ) )
				return -1;
		}
		else
		{
			hdr.mask_key = 0;
		}

		// Consume the input buffer and indicate success.
		//
		buffer = it;
		return 0;
	}
	inline void write( std::string& buffer, header& hdr )
	{
		auto write = [ & ] <typename T> ( const T& v )
		{
			buffer.insert( buffer.end(), ( char* ) &v, ( char* ) std::next( &v ) );
		};

		// Reserve enough space to hold the maximum header length and the data implied by the header.
		//
		buffer.reserve( buffer.size() + max_net_header_size + hdr.length );

		// Create the networked header and write it including extended size if relevant.
		//
		dassert( hdr.op < opcode::maximum );
		net_header net = { .op = hdr.op, .rsvd = 0, .fin = hdr.finished, .masked = hdr.mask_key != 0 };
		if ( hdr.length > UINT16_MAX )
		{
			net.length = length_extend_u64;
			write( net );
			write( ( size_t ) hdr.length );
		}
		else if ( hdr.length >= length_extend_u16 )
		{
			net.length = length_extend_u16;
			write( net );
			write( ( uint16_t ) hdr.length );
		}
		else
		{
			net.length = ( uint8_t ) hdr.length;
			write( net );
		}

		// Write the mask key if relevant.
		//
		if ( hdr.mask_key )
			write( hdr.mask_key );
	}

	// Declare the prototype of the type passed to the client defined below.
	//
	//struct client_prototype
	//{
	//	// Writes the given buffer into the TCP socket.
	//	// - Returns true if successful.
	//	//
	//	bool output( std::string );
	//
	//	// Gets invoked when the socket is closing.
	//	//
	//	void on_close( status_code status, bool server_close );
	//
	//	// Gets invoked when a websocket packet is received.
	//	// - Returns true if handled.
	//	//
	//	bool on_receive( opcode op, std::string_view data );
	//	
	//	// Closes the TCP socket.
	//	//
	//	void close_connection();
	//};

	// Define the websocket client.
	//
	template<typename T, size_t length_limit = 64_mb>
	struct client
	{
		// Client state.
		//
		bool terminated = false;

		// Receive buffer and the state of the fragmentation handlers.
		//
		size_t rx_buffer_offset = 0;
		std::vector<uint8_t> rx_buffer;
		std::optional<std::pair<header, std::vector<uint8_t>>> fragmented_packet;

		// Handles an incoming websocket packet.
		//
		bool accept_packet( const header& hdr, std::string_view data )
		{
			if ( terminated ) return false;
			if ( hdr.op == opcode::close ) terminated = true;

			// Handle known control frames.
			//
			if ( hdr.op == opcode::close )
			{
				// Read the status code and invoke the callback.
				//
				status_code status = status_unknown;
				if ( data.size() == 2 )
					memcpy( &status, data.data(), sizeof( status_code ) );
				( ( T* ) this )->on_close( status, true );
				( ( T* ) this )->close_connection();
				return false;
			}
			else if ( hdr.op == opcode::ping )
			{
				// Send a pong and continue.
				//
				send_packet( opcode::pong, data.data(), data.size() );
				return true;
			}
			else if ( hdr.op == opcode::pong )
			{
				// No operation.
				return true;
			}
			// Call into application.
			//
			else if ( ( ( T* ) this )->on_receive( hdr.op, data ) )
			{
				return true;
			}

			// Close due to unknown opcode.
			//
			close( status_unknown_operation );
			return false;
		}

		// Sends a websocket packet.
		//
		bool send_packet( opcode op, any_ptr data = {}, size_t length = 0 )
		{
			if ( terminated ) return false;
			if ( op == opcode::close ) terminated = true;

			// Create the header.
			//
			header hdr = {
				.op = op,
				.length = length,
				.finished = true,
				.mask_key = make_random<uint32_t>( 1, UINT32_MAX )
			};
			dassert( hdr.mask_key );

			// Serialize the header into the transmit buffer.
			//
			std::string tx_buffer = {};
			write( tx_buffer, hdr );

			// Append the data as well and mask it.
			//
			size_t p = tx_buffer.size();
			tx_buffer.insert( tx_buffer.end(), ( char* ) data, ( char* ) data + length );
			mask_buffer( tx_buffer.data() + p, length, hdr.mask_key );
			
			// Write to the TCP socket, close connection if it was opcode close, return the status.
			//
			bool state = ( ( T* ) this )->output( std::move( tx_buffer ) );
			if ( op == opcode::close ) ( ( T* ) this )->close_connection();
			return state;
		}
		bool send_packet( opcode op, std::string_view str )
		{
			return send_packet( op, str.data(), str.size() );
		}

		// Terminates the connection.
		//
		void close( status_code status = status_unknown )
		{
			if ( terminated ) return;

			send_packet( opcode::close, &status, status == status_unknown ? 0 : sizeof( status_code ) );
			( ( T* ) this )->on_close( status, false ); 
			( ( T* ) this )->close_connection();
		}

		// Accepts raw input from the TCP stream.
		//
		void input( any_ptr data, size_t length )
		{
			if ( terminated ) return;

			// Append to the receive buffer.
			//
			rx_buffer.insert( rx_buffer.end(), ( char* ) data, ( char* ) data + length );

			// Enter the consumption buffer.
			//
			size_t consumed = rx_buffer_offset;
			while ( true )
			{
				std::string_view it = { ( char* ) rx_buffer.data() + consumed, ( char* ) rx_buffer.data() + rx_buffer.size() };

				// Read the headers:
				//
				header hdr = {};
				int state = read( it, hdr );
				// Propagate error.
				if ( state > 0 )
					return close( ( status_code ) state );
				// Pending header, break.
				else if ( state < 0 )
					break;
				// Check for the length limit.
				else if ( hdr.length > length_limit )
					return close( status_data_too_large );
				// Pending body, break.
				else if ( it.size() < hdr.length )
					break;

				// Get the data buffer and mask it if relevant.
				//
				std::string_view data_buffer = it.substr( 0, hdr.length );
				mask_buffer( data_buffer.data(), data_buffer.size(), hdr.mask_key );
				it.remove_prefix( hdr.length );

				// If control frame, handle ignoring continuation state:
				//
				if ( hdr.is_control_frame() )
				{
					if ( !accept_packet( hdr, data_buffer ) )
						return;
				}
				// If continuation frame:
				//
				else if ( hdr.op == opcode::continuation )
				{
					// If no unfinished packet exists, terminate due to corrupt stream.
					//
					if ( !fragmented_packet )
						return close( status_protocol_error );

					// Append the data onto the streamed packet.
					//
					fragmented_packet->second.insert(
						fragmented_packet->second.end(),
						data_buffer.begin(),
						data_buffer.end()
					);

					// If packet finishes on this frame, handle the packet and reset fragmented packet.
					//
					if ( hdr.finished )
					{
						std::string_view full_data = { ( char* ) fragmented_packet->second.data(), fragmented_packet->second.size() };
						if ( !accept_packet( fragmented_packet->first, full_data ) )
							return;
						fragmented_packet.reset();
					}
				}
				// If fragmented packet:
				//
				else if ( !hdr.finished )
				{
					// If there is already another fragmented packet, terminate due to corrupt stream.
					//
					if ( fragmented_packet )
						return close();
					
					// Save the fragmented packet.
					//
					auto& buf = fragmented_packet.emplace( std::pair{ hdr, std::vector<uint8_t>{} } ).second;
					buf.insert( buf.end(), data_buffer.begin(), data_buffer.end() );
				}
				// If any other packet fitting in one frame, handle it.
				//
				else
				{
					if ( !accept_packet( hdr, data_buffer ) )
						return;
				}
				
				// Update the consumed size.
				//
				consumed += it.data() - ( char* ) rx_buffer.data();
			}
			
			// If we finally consumed the entire buffer, reset it, otherwise add it 
			// to the offset. This is done to avoid unnecessarily moving data.
			//
			if ( rx_buffer.size() == consumed )
				rx_buffer.clear();
			else
				rx_buffer_offset = consumed;
		}

		// Close the stream on destruction.
		//
		~client() { close( status_going_away ); }
	};
};