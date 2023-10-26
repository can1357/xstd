#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <optional>
#include "random.hpp"
#include "type_helpers.hpp"
#include "assert.hpp"
#include "result.hpp"
#include "sha1.hpp"
#include "base_n.hpp"
#include "random.hpp"
#include "http.hpp"

#if XSTD_VECTOR_EXT
	#include "xvector.hpp"
#endif

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
		inline constexpr status_code parser_status( uint16_t i ) { return status_code( 4500 + i ); }
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
	static constexpr size_t header_length( size_t packet_size, bool masked ) {
		size_t result = sizeof( net_header );
		result += packet_size >= length_extend_u16 ? 2 : 0;
		result += packet_size > UINT16_MAX         ? 6 : 0;
		result += masked                           ? 4 : 0;
		return result;
	}

	// Masking helper.
	//
	static void masked_copy( void* __restrict _dst, const void* __restrict _src, size_t len, uint32_t key ) {
		uint8_t* __restrict       dst = ( uint8_t * __restrict ) _dst;
		const uint8_t* __restrict src = ( const uint8_t * __restrict ) _src;

		if ( !key ) {
			if ( dst != src )
				memcpy( dst, src, len );
			return;
		}

#if CLANG_COMPILER
#pragma clang loop vectorize(enable)
#endif
		for ( size_t it = 0; it != len; it++ ) {
			dst[ it ] = src[ it ] ^ uint8_t( key >> 8 * ( it & 3 ) );
		}
	}
	static void masked_copy_bwd( void* __restrict _dst, const void* __restrict _src, size_t len, uint32_t key ) {
		uint8_t* __restrict       dst = ( uint8_t * __restrict ) _dst;
		const uint8_t* __restrict src = ( const uint8_t * __restrict ) _src;

		if ( !key ) {
			if ( dst != src )
				memmove( dst, src, len );
			return;
		}

#if CLANG_COMPILER
#pragma clang loop vectorize(enable)
#endif
		for ( size_t it = len - 1; ptrdiff_t( it ) >= 0; it-- ) {
			dst[ it ] = src[ it ] ^ uint8_t( key >> 8 * ( it & 3 ) );
		}
	}
	static void masked_move( void* __restrict _dst, const void* __restrict _src, size_t len, uint32_t key ) {
		if ( _dst > _src )
			return masked_copy_bwd( _dst, _src, len, key );
		else
			return masked_copy( _dst, _src, len, key );
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
		size_t header_length() const { return ws::header_length( length, mask_key != 0 ); }
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
			return parser_status( 1 );
		if ( hdr.is_control_frame() && !hdr.finished )
			return parser_status( 2 );

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
	inline size_t write( uint8_t* buffer, const header& hdr )
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
		assume( length <= max_net_header_size );
		return length;
	}

	// Sec-Websocket-Key / Accept handling.
	//
	static constexpr std::string_view sec_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	inline std::string accept_sec_key( std::string_view key ) {
		sha1 sha{};
		sha.add_bytes( (uint8_t*) key.data(), key.size() );
		sha.add_bytes( (uint8_t*) sec_guid.data(), sec_guid.size() );
		return encode::base64( sha.digest() );
	}
	inline std::string generate_sec_key() {
		auto bytes = make_random_n<uint8_t, 16>();
		return encode::base64( bytes );
	}

	// Upgrade logic.
	//
	inline std::vector<uint8_t> request_upgrade( std::string& sec_key, http::header_list user_headers, http::request_header_view req = {} ) {
		// Generate Sec key if not done so.
		//
		if ( sec_key.empty() ) {
			sec_key = generate_sec_key();
		}

		// Write the request header, common headers and the user headers.
		//
		std::vector<uint8_t> output = {};
		http::write( output, req );
		http::write( output, http::header_list{
			{ "Connection",            "Upgrade" },
			{ "Cache-Control",         "no-cache" },
			{ "Upgrade",               "websocket" },
			{ "Sec-WebSocket-Version", "13" },
			{ "Sec-WebSocket-Key",     sec_key },
		} );
		http::write( output, user_headers );
		http::end( output );
		return output;
	}
	static constexpr std::string_view accept_header = "HTTP/1.1 101 Switching Protocols\r\n";
	inline std::vector<uint8_t> accept_upgrade( std::string_view sec_key, http::header_list user_headers ) {
		// Write the response header, common headers and the user headers.
		//
		std::vector<uint8_t> output{ (const uint8_t*) accept_header.data(), (const uint8_t*) accept_header.data() + accept_header.size() };
		std::string accept_key = accept_sec_key( sec_key );
		http::write( output, http::header_list{
			{ "Connection",            "Upgrade" },
			{ "Cache-Control",         "no-cache" },
			{ "Upgrade",               "websocket" },
			{ "Sec-WebSocket-Version", "13" },
			{ "Sec-WebSocket-Accept",  accept_key },
		} );
		http::write( output, user_headers );
		http::end( output );
		return output;
	}
};