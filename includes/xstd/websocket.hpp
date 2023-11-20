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
#include "vec_buffer.hpp"

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

		size_t header_length() const {
			size_t result = sizeof( net_header );
			result += length == length_extend_u16 ? 2 : 0;
			result += length == length_extend_u64 ? 8 : 0;
			result += masked                      ? 4 : 0;
			return result;
		}
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
	struct header {
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

		// Readers.
		//
		std::optional<exception> read( vec_buffer& buffer ) {
			// Wait until we can receive the full header.
			//
			if ( buffer.size() < ws::max_net_header_size ) [[unlikely]] {
				if ( buffer.size() < sizeof( net_header ) )
					return std::nullopt;
				size_t hlen = ( (net_header*) buffer.data() )->header_length();
				if ( buffer.size() < hlen )
					return std::nullopt;
			}

			// Read and skip the header.
			//
			net_header net = buffer.shift_as<net_header>();
			op = net.op;
			finished = net.fin;

			if ( net.length == length_extend_u16 ) {
				length = bswap( buffer.shift_as<uint16_t>() );
			} else if ( net.length == length_extend_u64 ) {
				length = bswap( buffer.shift_as<uint64_t>() );
			} else {
				length = net.length;
			}

			if ( net.masked ) {
				mask_key = buffer.shift_as<uint32_t>();
			} else {
				mask_key = 0;
			}

			// Validate according to the RFC.
			//
			if ( net.rsvd )
				return XSTD_ESTR( "RSVD bits set in WS header" );
			if ( is_control_frame() && !finished )
				return XSTD_ESTR( "Control frame is expecting continuation" );
			return nullptr;
		}
		auto read( stream_view stream ) {
			return stream.read_until( [&]( vec_buffer& buf ) {
				if ( auto err = this->read( buf ) ) {
					if ( *err ) stream.stop( std::move( *err ) );
					return true;
				} else {
					return false;
				}
			} );
		}

		// Writers.
		//
		void write( vec_buffer& buffer ) const {
			// Create the networked header and write it including extended size if relevant.
			//
			dassert( this->op < opcode::maximum );
			dassert( !this->is_control_frame() || this->finished );

			net_header net = { .op = this->op, .rsvd = 0, .fin = this->finished, .masked = this->mask_key != 0 };
			if ( this->length > UINT16_MAX ) {
				net.length = length_extend_u64;
				buffer.emplace_back_as<net_header>( net );
				buffer.emplace_back_as<uint64_t>( bswap( (uint64_t) this->length ) );
			} else if ( this->length >= length_extend_u16 ) {
				net.length = length_extend_u16;
				buffer.emplace_back_as<net_header>( net );
				buffer.emplace_back_as<uint16_t>( bswap( (uint16_t) this->length ) );
			} else {
				net.length = (uint8_t) this->length;
				buffer.emplace_back_as<net_header>( net );
			}

			// Write the mask key if relevant.
			//
			if ( this->mask_key )
				buffer.emplace_back_as<uint32_t>( this->mask_key );
		}
		auto write( stream_view stream ) const {
			return stream.write_using( [&]( vec_buffer& buf ) {
				this->write( buf );
			} );
		}
	};

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
	inline http::request upgrade_request( http::request::options&& opts = {} ) {
		http::request req{ std::move( opts ) };
		req.set_headers( {
			{ "Connection",            "Upgrade" },
			{ "Cache-Control",         "no-cache" },
			{ "Upgrade",               "websocket" },
			{ "Sec-WebSocket-Version", "13" },
			{ "Sec-WebSocket-Key",     generate_sec_key() }
		} );
		return req;
	}
	inline http::response upgrade_accept( const http::request& reply_to, http::response::options&& opts = {} ) {
		opts.status = 101;
		http::response res{ std::move( opts ) };
		res.set_headers( {
			{ "Connection",            "Upgrade" },
			{ "Cache-Control",         "no-cache" },
			{ "Upgrade",               "websocket" },
			{ "Sec-WebSocket-Version", "13" },
			{ "Sec-WebSocket-Accept",  accept_sec_key( reply_to.get_header( "Sec-WebSocket-Key" ) ) },
		} );
		return res;
	}
	inline exception upgrade_validate( const http::request& request ) {
		if ( request.connection() != http::connection::upgrade || !iequals( request.headers[ "upgrade" ], "websocket" ) ) {
			return XSTD_ESTR( "Unexpected request, upgrade is not websocket." );
		} else if ( request.get_header( "Sec-WebSocket-Key" ).empty() ) {
			return XSTD_ESTR( "Unexpected request, request key does not exist." );
		}
		return {};
	}
	inline exception upgrade_validate( const http::request& request, const http::response& response ) {
		if ( response.status != 101 ) {
			return XSTD_ESTR( "Unexpected response, failed upgrade." );
		} else if ( response.connection() != http::connection::upgrade || !iequals( response.headers[ "upgrade" ], "websocket" ) ) {
			return XSTD_ESTR( "Unexpected response, upgrade is not websocket." );
		} else if ( response.get_header( "Sec-WebSocket-Accept" ) != accept_sec_key( request.get_header( "Sec-WebSocket-Key" ) ) ) {
			return XSTD_ESTR( "Unexpected response, accept key is mismatching." );
		}
		return {};
	}
};