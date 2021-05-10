#pragma once
#include <string>
#include <vector>
#include <optional>
#include <string_view>
#include <unordered_map>
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
	static constexpr auto method_hashmap = make_constant_series<std::size( method_map )>( [ ] <size_t N> ( const_tag<N> )
	{
		return make_xhash( method_map[ N ] );
	} );

	// Define case insensitive string comperator for the header map.
	//
	namespace impl
	{
		struct icase_eq
		{
			bool operator()( const std::string& a, const std::string& b ) const noexcept
			{
				return istrcmp( a, b ) == 0;
			}
			bool operator()( const std::string_view& a, const std::string_view& b ) const noexcept
			{
				return istrcmp( a, b ) == 0;
			}
		};
	};

	// Define the header map.
	//
	using header_map =      std::unordered_map<std::string, std::string, ihash<std::string>, impl::icase_eq>;
	using header_view_map = std::unordered_map<std::string_view, std::string_view, ihash<std::string_view>, impl::icase_eq>;
	template<typename T> concept HeaderMap = std::is_same_v<T, header_map> || std::is_same_v<T, header_view_map>;

	// Declare the I/O helpers.
	//
	template<HeaderMap T>
	inline void write( std::string& output, const T& headers )
	{
		// Reserve the space we need to write the headers.
		//
		output.reserve( output.size() + std::accumulate( headers.begin(), headers.end(), 0ull, [ ] ( size_t n, auto& pair )
		{
			return n + pair.first.size() + 2 + pair.second.size() + 2;
		} ) );

		// Write all headers.
		//
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
	template<typename T> concept RequestHeader = std::is_same_v<T, request_header> || std::is_same_v<T, request_header_view>;

	// Declare the I/O helpers.
	//
	template<RequestHeader T>
	inline void write( std::string& output, const T& header )
	{
		// Reserve the size required and then write it.
		//
		dassert( header.method < method_id::maximum );
		auto& method = method_map[ size_t( header.method ) ];
		output.reserve( output.size() + method.size() + 1 + header.path.size() + 1 + strlen( "HTTP/1.1\r\n" ) );
		output += method;
		output += ' ';
		output += header.path;
		output += ' ';
		output += "HTTP/1.1\r\n";
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
		hash_t method_hash = make_xhash( line.substr( 0, method_end ) );
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
	template<typename T> concept ResponseHeader = std::is_same_v<T, response_header> || std::is_same_v<T, response_header_view>;

	// Declare the I/O helpers.
	//
	template<ResponseHeader T>
	inline void write( std::string& output, const T& header )
	{
		// Reserve the size required and then write it.
		//
		dassert( header.method < method_id::maximum );
		auto& method = method_map[ size_t( header.method ) ];
		output.reserve( output.size() + strlen( "HTTP/1.1 " ) + 5 + header.message.size() + 2 );
		output += "HTTP/1.1 ";
		output += std::to_string( header.status );
		output += ' ';
		output += header.message;
		output += "\r\n";
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
	concept Buffer = std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view> || std::is_same_v<T, std::vector<uint8_t>> || std::is_same_v<T, std::vector<char>>;
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
	struct request
	{
		// Headers of the request.
		//
		request_header meta = {};
		header_map headers = {};
		
		// Body of the request.
		//
		std::vector<uint8_t> body = {};

		// Formats the header.
		//
		std::string write_headers() const
		{
			// Write the request header and the common headers.
			//
			std::string output = {};
			http::write( output, meta );
			http::write( output, headers );

			// If there is a body and Content-Length is not set, do so.
			//
			if ( !body.empty() && !headers.contains( "Content-Length" ) )
			{
				output += "Content-Length: ";
				output += std::to_string( body.size() );
				output += "\r\n";
			}
			return output;
		}

		// Formats the header and the body.
		//
		std::string write() const
		{
			std::string result = write_headers();
			result += "\r\n";
			result.insert( result.end(), body.begin(), body.end() );
			return result;
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
			response result = {};
			if ( !http::read( input, result.meta ) )
				return std::nullopt;
			if ( !http::read( input, result.headers ) )
				return std::nullopt;

			// Stop parsing if the parser is header only.
			//
			if ( header_only )
				return result;

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

					if ( result.content_length > input.size() )
						result.body.assign( input.begin(), input.begin() + result.content_length ), input.remove_prefix( result.content_length );
					else
						return std::nullopt;
				}
			}
			return result;
		}
	};
};