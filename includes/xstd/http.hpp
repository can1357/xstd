#pragma once
#include <string>
#include <vector>
#include <optional>
#include <string_view>
#include <unordered_map>
#include "generator.hpp"
#include "tcp.hpp"
#include "text.hpp"
#include "assert.hpp"

namespace xstd::http
{
	// Status categories.
	//
	enum class status_category : uint8_t
	{
		none =          0,
		informational = 1,
		success =       2,
		redirecting =   3,
		client_error =  4,
		server_error =  5,
	};
	// Traits of the status code.
	//
	static constexpr status_category get_status_category( uint32_t status_code ) 
	{ 
		return status_category( status_code / 100 ); 
	}
	static constexpr bool is_success( uint32_t status_code ) 
	{ 
		switch ( get_status_category( status_code ) )
		{
			case status_category::success:
			case status_category::informational: return true;
			default:                             return false;
		}
	}
	static constexpr bool is_failure( uint32_t status_code ) { return !is_success( status_code ); }

	// HTTP methods.
	//
	enum method_id
	{
		GET,
		HEAD,
		POST,
		PUT,
		FDELETE,
		CONNECT,
		OPTIONS,
		TRACE,
		PATCH,
		maximum,
	};
	static constexpr std::string_view method_map[] = {
		"GET",
		"HEAD",
		"POST",
		"PUT",
		"DELETE",
		"CONNECT",
		"OPTIONS",
		"TRACE",
		"PATCH"
	};
	static constexpr auto hash_method( std::string_view method )
	{
		uint64_t value = 0;
		for ( size_t n = 0; n != 8 && n != method.size(); n++ )
			value |= 0x20 | ( uint64_t( n ) << ( 8 * n ) );
		value ^= 0xfa7c0c5;
		return ~value;
	}
	static constexpr auto method_hashmap = make_constant_series<std::size( method_map )>( [ ] <size_t N> ( const_tag<N> )
	{
		return hash_method( method_map[ N ] );
	} );

	// Define the header map.
	//
	using header_map =      std::unordered_map<std::string, std::string, ahash<std::string>, icmp_equals>;
	using header_view_map = std::unordered_map<std::string_view, std::string_view, ahash<std::string_view>, icmp_equals>;
	using header_list =     std::initializer_list<std::pair<std::string_view, std::string_view>>;
	using header_span =     std::span<const std::pair<std::string_view, std::string_view>>;
	template<typename T> concept HeaderMap = Same<T, header_map> || Same<T, header_view_map>;

	// Declare the I/O helpers.
	//
	template<typename T> requires ( HeaderMap<T> || Same<header_list, T> || Same<header_span, T> )
	inline void write( std::vector<uint8_t>& output, const T& headers )
	{
		for ( auto& [key, value] : headers )
		{
			output.insert( output.end(), key.begin(), key.end() );
			output.insert( output.end(), { ':', ' ' } );
			output.insert( output.end(), value.begin(), value.end() );
			output.insert( output.end(), { '\r', '\n' } );
		}
	}
	template<HeaderMap T>
	inline bool read( std::string_view& input, T& headers )
	{
		while ( true )
		{
			// Read the line.
			//
			size_t i = input.find( "\r\n" );
			if ( i == std::string::npos )
				break;
			std::string_view line = input.substr( 0, i );

			// Find the ending of the key.
			//
			size_t key_end = line.find( ": " );
			if ( key_end == std::string::npos )
				break;

			// Insert the key-value pair and remove the line.
			//
			using K = typename T::key_type;
			using V = typename T::mapped_type;
			std::string_view key = line.substr( 0, key_end );
			std::string_view value = line.substr( key_end + 2 );
			if ( !headers.emplace( K{ key }, V{ value } ).second )
				return false;
			input.remove_prefix( i + 2 );
		}
		return true;
	}

	// Define the request header.
	//
	struct request_header
	{
		method_id method = GET;
		std::string path = { '/' };
	};
	struct request_header_view
	{
		method_id method;
		std::string_view path;
	};
	template<typename T> concept RequestHeader = Same<T, request_header> || Same<T, request_header_view>;

	// Declare the I/O helpers.
	//
	template<RequestHeader T>
	inline void write( std::vector<uint8_t>& output, const T& header )
	{
		dassert( header.method < method_id::maximum );
		auto& method = method_map[ size_t( header.method ) ];
		output.insert( output.end(), method.begin(), method.end() );
		output.insert( output.end(), ' ' );
		output.insert( output.end(), header.path.begin(), header.path.end() );
		output.insert( output.end(), ' ' );

		constexpr std::string_view version = "HTTP/1.1\r\n";
		output.insert( output.end(), version.begin(), version.end() );
	}
	template<RequestHeader T>
	inline bool read( std::string_view& input, T& header )
	{
		// Read the line.
		//
		size_t i = input.find( "\r\n" );
		if ( i == std::string::npos ) return false;
		std::string_view line = input.substr( 0, i );

		// Check the HTTP version and erase the suffix.
		//
		if ( !line.ends_with( " HTTP/1.1\r\n" ) )
			return false;
		line.remove_suffix( strlen( " HTTP/1.1\r\n" ) );

		// Read the method and hash the name.
		//
		size_t method_end = line.find( ' ' );
		if ( method_end == std::string::npos )
			return false;
		hash_t method_hash = hash_method( line.substr( 0, method_end ) );
		auto it = std::find( method_hashmap.begin(), method_hashmap.end(), method_hash );
		if ( it == method_hashmap.end() )
			return false;
		header.method = method_id( it - method_hashmap.begin() );
		line.remove_prefix( method_end + 1 );

		// Finally read the path and remove the line from the input.
		//
		using S = decltype( T::path );
		header.path = S{ line };
		input.remove_prefix( i + 2 );
		return true;
	}

	// Define the response header.
	//
	// HTTP/1.1 200 OK
	struct response_header
	{
		uint32_t status = 200;
		std::string message = {};
	};
	struct response_header_view
	{
		uint32_t status;
		std::string_view message;
	};
	template<typename T> concept ResponseHeader = Same<T, response_header> || Same<T, response_header_view>;

	// Declare the I/O helpers.
	//
	template<ResponseHeader T>
	inline void write( std::vector<uint8_t>& output, const T& header )
	{
		dassert( header.method < method_id::maximum );
		auto& method = method_map[ size_t( header.method ) ];

		constexpr std::string_view version = "HTTP/1.1 ";
		output.insert( output.end(), version.begin(), version.end() );

		std::array<char, 4> status = {
			'0' + char( int( header.status / 100 ) % 10 ),
			'0' + char( int( header.status / 10 ) % 10 ),
			'0' + char( int( header.status / 1 ) % 10 ),
			' '
		};
		output.insert( output.end(), status.begin(), status.end() );
		output.insert( output.end(), header.message.begin(), header.message.end() );
		output.insert( output.end(), { '\r', '\n' } );
	}
	template<ResponseHeader T>
	inline bool read( std::string_view& input, T& header )
	{
		// Read the line.
		//
		size_t i = input.find( "\r\n" );
		if ( i == std::string::npos ) return false;
		std::string_view line = input.substr( 0, i );

		// Check the HTTP version and erase the prefix.
		//
		if ( !line.starts_with( "HTTP/1.1 " ) )
			return false;
		line.remove_prefix( strlen( "HTTP/1.1 " ) );

		// Read the status and convert to an integer.
		//
		size_t status_end = line.find( ' ' );
		if ( status_end == std::string::npos )
			return false;
		header.status = parse_number<uint32_t>( line.substr( 0, status_end ) );
		line.remove_prefix( status_end + 1 );

		// Finally read the path and remove the line from the input.
		//
		using S = decltype( T::message );
		header.message = S{ line };
		input.remove_prefix( i + 2 );
		return true;
	}

	// Implement the decoding for chunked input. Reads as many chunks as possible from rx-buffer
	// into the output, returns false on failure and true on success. If rx buffer is not empty 
	// after being indicated success, it means more data is required.
	//
	template<typename T> 
	concept Buffer = Same<T, std::string> || Same<T, std::string_view> || Same<T, std::vector<uint8_t>> || Same<T, std::vector<char>>;
	template<Buffer T1, Buffer T2>
	inline bool decode_chunked( T1& output, T2& rx_buffer )
	{
		output.reserve( rx_buffer.size() );

		bool status = true;
		std::string_view buffer_view = { ( char* ) rx_buffer.data(), ( char* ) rx_buffer.data() + rx_buffer.size() };
		while ( true )
		{
			// If buffer is empty, we're done.
			//
			if ( buffer_view.empty() )
				break;

			// Find the line end and get the integer.
			//
			size_t i = buffer_view.find( "\r\n" );
			std::string_view integer = buffer_view.substr( i );

			// Make sure it does not contain anything besides 0-9, a-f, A-F.
			//
			static constexpr auto hex_validation = make_constant_series<0x100>( [ ] ( auto v )
			{
				uint8_t character = ( uint8_t ) v.value;
				if ( '0' <= character && character <= '9' ) return true;
				if ( 'A' <= character && character <= 'F' ) return true;
				if ( 'a' <= character && character <= 'f' ) return true;
				return false;
			} );
			auto invit = std::accumulate( integer.begin(), integer.end(), 0ull, [ ] ( size_t ac, auto& ch ) { return ac + hex_validation[ ch ]; } );
			if ( invit != integer.size() )
			{
				status = false;
				break;
			}

			// Parse the size, if there is not enough data in the buffer to read the size, break.
			//
			size_t chunk_size = parse_number<size_t>( integer, 16 );
			if ( buffer_view.size() < ( chunk_size + i + 4 ) )
				break;

			// Otherwise append data to the real buffer, remove the header and the data.
			//
			buffer_view.remove_prefix( i + 2 );
			output.insert( output.end(), buffer_view.begin(), buffer_view.begin() + chunk_size );
			buffer_view.remove_prefix( chunk_size + 2 );
		}

		// Remove the read parts from the buffer and return the status.
		//
		if ( buffer_view.empty() )
		{
			rx_buffer = {};
		}
		else
		{
			if constexpr ( Same<T2, std::string_view> )
				rx_buffer.remove_prefix( buffer_view.data() - rx_buffer.data() );
			else
				rx_buffer.erase( rx_buffer.begin(), rx_buffer.begin() + ( buffer_view.data() - ( char* ) rx_buffer.data() ) );
		}
		return status;
	}

	// Describes a HTTP request.
	//
	struct request_view
	{
		// Headers of the request.
		//
		request_header_view meta = {};
		header_span headers = {};

		// Body of the request.
		//
		std::span<const uint8_t> body = {};

		// Formats the header and the body.
		//
		std::vector<uint8_t> write( bool content_length_set = false ) const
		{
			// Write the request header and the common headers.
			//
			std::vector<uint8_t> output = {};
			output.reserve( body.size() + 512 );
			http::write( output, meta );
			http::write( output, headers );

			// If there is a body and Content-Length is not set, do so.
			//
			if ( !body.empty() && !content_length_set )
			{
				char buffer[ 128 ];
				int length = sprintf_s( buffer, "Content-Length: %llu\r\n", body.size() );
				output.insert( output.end(), &buffer[ 0 ], &buffer[ length ] );
			}

			// Finalize header list, write the body.
			//
			output.insert( output.end(), { '\r', '\n' } );
			output.insert( output.end(), body.begin(), body.end() );
			return output;
		}
	};

	// Describes a HTTP response.
	//
	struct response
	{
		// Headers of the response.
		//
		response_header_view meta = {};
		header_view_map headers = {};
		
		// Body of the request.
		//
		size_t content_length = 0;
		std::vector<uint8_t> body = {};

		// Parses a response given the received message.
		//
		static std::optional<response> parse( std::string_view& input, bool header_only = false )
		{
			// Parse the base headers.
			//
			std::optional<response> retval = {};
			response& result = retval.emplace();
			if ( !http::read( input, result.meta ) )
				return std::nullopt;
			if ( !http::read( input, result.headers ) )
				return std::nullopt;

			// Stop parsing if the parser is header only.
			//
			if ( header_only )
				return retval;

			// Skip the newline.
			//
			if ( !input.starts_with( "\r\n" ) )
				return std::nullopt;
			input.remove_prefix( 2 );

			// Get content length.
			//
			auto clen_it = result.headers.find( "Content-Length" );
			result.content_length = 0;
			if ( clen_it != result.headers.end() )
				result.content_length = parse_number<size_t>( clen_it->second );

			// Parse the body.
			//
			if ( result.content_length )
			{
				if ( input.empty() )
					return std::nullopt;

				// Handle the transfer encodings and read the body. If not fully consumed or if size does not match, will fail. 
				// This is a oneshot decoder, for more specialized use cases user should use the underlying details.
				//
				if ( auto it = result.headers.find( "Transfer-Encoding" ); it != result.headers.end() && !istrcmp( it->second, "chunked" ) )
				{
					
					if ( !decode_chunked( result.body, input ) || input.empty() )
						return std::nullopt;
					if ( result.body.size() != result.content_length )
						return std::nullopt;
				}
				else
				{

					if ( result.content_length >= input.size() )
						result.body.assign( input.begin(), input.begin() + result.content_length ), input.remove_prefix( result.content_length );
					else
						return std::nullopt;
				}
			}
			return retval;
		}
	};

	// Sends a request through the TCP socket.
	//
	inline void send( tcp::client& socket, method_id method, std::string_view path, header_list headers, std::span<const uint8_t> body = {} )
	{
		socket.write( request_view{ .meta = { method, path }, .headers = headers, .body = body }.write() );
	}

	// Wrappers for the most common methods.
	//
	inline void get( tcp::client& socket, std::string_view path, header_list headers, std::span<const uint8_t> body = {} )
	{
		send( socket, method_id::GET, path, headers, body );
	}
	inline void post( tcp::client& socket, std::string_view path, header_list headers, std::span<const uint8_t> body = {} )
	{
		send( socket, method_id::POST, path, headers, body );
	}

	// Parses HTTP responses.
	//
	inline generator<response> parser( tcp::client& socket, bool header_only = false )
	{
		while ( true )
		{
			auto view = co_await socket.recv();
			if ( view.empty() )
				co_return;

			if ( auto result = response::parse( view, header_only ) )
			{
				socket.forward_to( view );
				co_yield *result;
			}
		}
	}
};