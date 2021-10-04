#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <optional>
#include "generator.hpp"
#include "spinlock.hpp"
#include "time.hpp"
#include "random.hpp"
#include "tcp.hpp"
#include "task.hpp"
#include "type_helpers.hpp"
#include "assert.hpp"

// Define status codes and the traits.
//
namespace xstd
{
	namespace ws
	{
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
		inline constexpr status_code application_status( uint16_t i ) { return status_code( 4000 + i ); }
	};

	template<>
	struct status_traits<ws::status_code>
	{
		static constexpr ws::status_code success_value = ws::status_none;
		static constexpr ws::status_code failure_value = ws::status_unknown;
		constexpr static inline bool is_success( uint16_t s ) { return s == ws::status_none; }
	};
};

// Start the websocket implementation.
//
namespace xstd::ws
{
	static constexpr uint8_t length_extend_u16 = 126;
	static constexpr uint8_t length_extend_u64 = 127;
	static constexpr size_t default_length_limit = 64_mb;
	static constexpr size_t max_packet_size = 63_kb; // 65535 - max_net_header_size - 34;

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
	inline void masked_copy( uint8_t* out, any_ptr ptr, size_t len, uint32_t key )
	{
		if ( !key ) return;
		uint8_t* data = ptr;
		auto* kb = ( const uint8_t* ) &key;
		for ( size_t n = 0; n != len; n++ )
			out[ n ] = data[ n ] ^ kb[ n % 4 ];
	}

	// Parsed packet header.
	//
	struct header
	{
		// Opcode and packet length.
		//
		size_t length = 0;
		opcode op = opcode::maximum;
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
	inline size_t write( uint8_t* buffer, header& hdr )
	{
		size_t length = 0;
		auto write = [ & ] <typename T> ( const T& v )
		{
			*( T* ) &buffer[ length ] = v;
			length += sizeof( T );
		};

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
		
		// Return the length of the header.
		//
		return length;
	}
	
	// Generic send implementation.
	//
	inline void send( tcp::client& socket, opcode op, const void* src, size_t length )
	{
		auto submit = [ & ] ( header hdr, std::span<const uint8_t> body )
		{
			// Update the length in the header, pick a mask key.
			//
			hdr.length = body.size();
			hdr.mask_key = xstd::make_random<uint32_t>( 1, UINT32_MAX );
			
			// Write the header.
			//
			std::vector<uint8_t> buffer( max_net_header_size + body.size() );
			auto it = write( buffer.data(), hdr );
			
			// Write the data.
			//
			masked_copy( buffer.data() + it, body.data(), body.size(), hdr.mask_key );
			
			// Resize the buffer and send it.
			//
			buffer.resize( it + body.size() );
			socket.tx_queue.emplace_back( std::move( buffer ), 0 );
		};

		// Acquire the TX queue lock and reserve the slots.
		//
		std::lock_guard _g{ socket.tx_lock };
		std::span data{ ( const uint8_t* ) src, ( const uint8_t* ) src + length };

		// Split the packets into continuation lengths.
		//
		while ( data.size() > max_packet_size )
		{
			submit( { .op = op, .finished = false }, data.subspan( 0, max_packet_size ) );
			data = data.subspan( max_packet_size );
			op = opcode::continuation;
		}
		submit( { .op = op, .finished = true }, data );

		// Flush the queues.
		//
		socket.flush_queues( true );
	}

	// Helpers for specific opcodes.
	//
	inline void send( tcp::client& socket, std::string_view text )
	{
		send( socket, opcode::text, text.data(), text.size() );
	}
	inline void send( tcp::client& socket, std::span<const uint8_t> data )
	{
		send( socket, opcode::binary, data.data(), data.size() );
	}
	inline void close( tcp::client& socket, status_code status )
	{
		if ( socket.is_closed() ) return;
		status = bswap( status );
		send( socket, opcode::close, &status, sizeof( status ) );
		socket.socket_close();
	}
	inline void ping( tcp::client& socket, std::span<const uint8_t> data )
	{
		send( socket, opcode::ping, data.data(), data.size() );
	}
	inline void pong( tcp::client& socket, std::span<const uint8_t> data )
	{
		send( socket, opcode::pong, data.data(), data.size() );
	}

	// Packet parser.
	//
	inline generator<std::pair<opcode, std::span<uint8_t>>> parser( tcp::client& socket, status_code& status, size_t length_limit = default_length_limit )
	{
		auto accept_packet = [ & ] () -> xstd::task<std::pair<header, std::span<uint8_t>>, status_code>
		{
			// Read the header.
			//
			header hdr = {};
			while ( true )
			{
				auto data = co_await socket.recv();
				if ( data.empty() )
					co_yield status_connection_reset;

				int result = read( data, hdr );
				if ( result < 0 )
					continue;

				if ( result > 0 )
					co_yield ( status_code ) result;
				socket.forward_to( data );
				break;
			}

			// Validate the header.
			//
			if ( hdr.length > length_limit )
				co_yield status_data_too_large;

			// Wait for the body.
			//
			auto sstr = co_await socket.recv( hdr.length );
			if ( sstr.empty() )
				co_yield status_connection_reset;

			std::span<uint8_t> result_data = { ( uint8_t* ) sstr.data(), sstr.size() };
			mask_buffer( result_data.data(), result_data.size(), hdr.mask_key );
			co_return std::pair{ hdr, result_data };
		};

		status = status_code::status_none;
		while ( true )
		{
			// Accept a packet.
			//
			auto res = co_await accept_packet();
			if ( res.fail() )
			{
				close( socket, ( status = res.status ) );
				co_return;
			}
			auto [hdr, data] = *res;

			// Fail if continuation packet.
			//
			if ( hdr.op == opcode::continuation )
			{
				close( socket, ( status = status_protocol_error ) ); // Did not expect a continuation.
				co_return;
			}

			// If control frame, handle seperately and continue.
			//
			if ( hdr.is_control_frame() || hdr.finished )
			{
				switch ( hdr.op )
				{
					case opcode::ping:
						pong( socket, data );
						break;
					case opcode::close:
						status = data.size() == 2 ? bswap( *( status_code* ) data.data() ) : status_unknown;
						socket.socket_close();
						co_return;
					default:
						co_yield std::pair{ hdr.op, data };
						break;
				}
				continue;
			}

			// Start fragmented packet.
			//
			std::vector<uint8_t> fragmented_packet{ data.begin(), data.end() };
			while ( true )
			{
				// Accept a packet.
				//
				auto res = co_await accept_packet();
				if ( res.fail() )
				{
					close( socket, ( status = res.status ) );
					co_return;
				}
				auto [hdr, data] = *res;

				// If control frame, handle seperately and continue.
				//
				if ( hdr.is_control_frame() )
				{
					switch ( hdr.op )
					{
						case opcode::ping: 
							pong( socket, data ); 
							break;
						case opcode::close:
							status = data.size() == 2 ? bswap( *( status_code* ) data.data() ) : status_unknown;
							co_return;
						default:
							co_yield std::pair{ hdr.op, data };
							break;
					}
					continue;
				}

				// Add fragment.
				//
				if ( hdr.op != opcode::continuation )
				{
					close( socket, ( status = status_protocol_error ) ); // Unfinished fragmented packet.
					co_return;
				}
				fragmented_packet.insert( fragmented_packet.end(), data.begin(), data.end() );

				// If we finished the continuation stream, break.
				//
				if ( hdr.finished )
					break;
			}

			// Yield the final packet.
			//
			co_yield std::pair{ hdr.op, std::span<uint8_t>{ fragmented_packet } };
		}
	}
};