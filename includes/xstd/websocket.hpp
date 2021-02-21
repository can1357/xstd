#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include "time.hpp"
#include "random.hpp"
#include "tcp.hpp"
#include "type_helpers.hpp"
#include "assert.hpp"

namespace xstd::ws
{
	static constexpr uint8_t length_extend_u16 = 126;
	static constexpr uint8_t length_extend_u64 = 127;
	static constexpr size_t length_limit = 64_mb;

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
		status_none =              0,
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
		if ( hdr.is_control_frame() && hdr.length >= length_extend_u16 )
			return status_protocol_error;

		// Read the extended length if relevant.
		//
		if ( hdr.length == length_extend_u16 )
		{
			if ( !read( ( uint16_t& ) hdr.length ) )
				return -1;
			hdr.length = bswap( ( uint16_t& ) hdr.length );
		}
		else if ( hdr.length == length_extend_u64 )
		{
			if ( !read( ( uint64_t& ) hdr.length ) )
				return -1;
			hdr.length = bswap( ( uint64_t& ) hdr.length );
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
		dassert( !hdr.is_control_frame() || hdr.length < length_extend_u16 );
		dassert( !hdr.is_control_frame() || hdr.finished );

		net_header net = { .op = hdr.op, .rsvd = 0, .fin = hdr.finished, .masked = hdr.mask_key != 0 };
		if ( hdr.length > UINT16_MAX )
		{
			net.length = length_extend_u64;
			write( net );
			write( bswap( ( size_t ) hdr.length ) );
		}
		else if ( hdr.length >= length_extend_u16 )
		{
			net.length = length_extend_u16;
			write( net );
			write( bswap( ( uint16_t ) hdr.length ) );
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

	// Define the websocket client.
	//
	template<typename transport_layer>
	struct client : transport_layer
	{
		// Status of the WebSocket.
		//
		std::atomic<status_code> status = status_none;

		// Status of our ping-pong requests.
		//
		uint32_t ping_key = make_random<uint32_t>();
		timestamp last_ping = {};
		timestamp last_pong = {};

		// Receive buffer and the state of the fragmentation handlers.
		//
		std::recursive_mutex receive_mutex;
		std::optional<std::pair<header, std::vector<uint8_t>>> fragmented_packet;

		// Implemented by the application layer.
		//
		virtual bool on_receive( opcode op, std::string_view data ) = 0;

		// Handles an incoming websocket packet.
		//
		void handle_packet( const header& hdr, std::string_view data )
		{
			if ( transport_layer::closed ) return;

			// Handle known control frames.
			//
			if ( hdr.op == opcode::close )
			{
				// Read the status code and invoke the callback.
				//
				auto status_ex = status_none;
				if ( data.size() >= 2 )
					status.compare_exchange_strong( status_ex, bswap( *( status_code* ) data.data() ) );
				else
					status.compare_exchange_strong( status_ex, status_shutdown );
				transport_layer::socket_close();
			}
			else if ( hdr.op == opcode::ping )
			{
				// Send a pong and continue.
				//
				send_packet( opcode::pong, data.data(), data.size() );
			}
			else if ( hdr.op == opcode::pong )
			{
				// No operation, just update the timer if same key.
				//
				if ( last_ping > last_pong && ping_key && data.size() == 4 && ping_key == *( uint32_t* ) data.data() )
				{
					last_pong = time::now();
					ping_key = 0;
				}
			}
			// Call into application.
			//
			else if ( !on_receive( hdr.op, data ) )
			{
				close( status_unknown_operation );
			}
		}

		// Sends a websocket packet.
		//
		void send_packet( opcode op, any_ptr data, size_t length )
		{
			if ( transport_layer::closed ) return;

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
			ws::write( tx_buffer, hdr );

			// Append the data as well and mask it.
			//
			size_t p = tx_buffer.size();
			tx_buffer.insert( tx_buffer.end(), ( char* ) data, ( char* ) data + length );
			mask_buffer( tx_buffer.data() + p, length, hdr.mask_key );
			
			// Write to the TCP socket.
			//
			transport_layer::write( std::move( tx_buffer ) );
		}
		void send_packet( opcode op, std::string_view str = {} ) { send_packet( op, str.data(), str.size() ); }

		// Ping-pong helper, returns latest measured latency or zero.
		//
		duration ping()
		{
			auto t = last_pong - last_ping;
			ping_key = make_random<uint32_t>( 1 );
			last_ping = time::now();
			send_packet( opcode::ping, &ping_key, sizeof( ping_key ) );
			return t;
		}

		// Terminates the connection.
		//
		void close( status_code st = status_shutdown )
		{
			if ( transport_layer::closed ) return;
			if ( st != status_none )
			{
				status_code status_ex = status_none;
				if ( status.compare_exchange_strong( status_ex, st ) )
				{
					st = bswap( st );
					send_packet( opcode::close, &st, sizeof( st ) );
					transport_layer::socket_writeback();
				}
			}
			transport_layer::socket_close();
		}

		// Parses packets from the TCP stream.
		//
		size_t packet_parse( std::string_view data ) override
		{
			std::lock_guard _g{ receive_mutex };
			const char* it_original = data.data();

			// Read the headers:
			//
			header hdr = {};
			int state = read( data, hdr );
			// Propagate error.
			if ( state > 0 )
			{
				close( ( status_code ) state );
				return 0;
			}
			// Pending header, break.
			else if ( state < 0 )
				return 0;
			// Check for the length limit.
			else if ( hdr.length > length_limit )
			{
				close( status_data_too_large );
				return 0;
			}
			// Pending body, break.
			else if ( data.size() < hdr.length )
				return 0;;

			// Get the data buffer and mask it if relevant.
			//
			std::string_view data_buffer = data.substr( 0, hdr.length );
			mask_buffer( data_buffer.data(), data_buffer.size(), hdr.mask_key );
			data.remove_prefix( hdr.length );

			// If control frame, handle ignoring continuation state:
			//
			if ( hdr.is_control_frame() )
			{
				handle_packet( hdr, data_buffer );
			}
			// If continuation frame:
			//
			else if ( hdr.op == opcode::continuation )
			{
				// If no unfinished packet exists, terminate due to corrupt stream.
				//
				if ( !fragmented_packet )
				{
					close( status_protocol_error );
					return 0;
				}

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
					handle_packet( fragmented_packet->first, full_data );
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
				{
					close( status_protocol_error );
					return 0;
				}

				// Save the fragmented packet.
				//
				auto& buf = fragmented_packet.emplace( std::pair{ hdr, std::vector<uint8_t>{} } ).second;
				buf.insert( buf.end(), data_buffer.begin(), data_buffer.end() );
			}
			// If any other packet fitting in one frame, handle it.
			//
			else
			{
				handle_packet( hdr, data_buffer );
			}

			// Return the consumed size.
			//
			return data.data() - it_original;
		}

		// Close the stream on destruction.
		//
		~client() { close( status_none ); }
	};
};