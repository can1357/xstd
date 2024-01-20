#pragma once
#include <string>
#include <vector>
#include <optional>
#include <string_view>
#include <unordered_map>
#include "spinlock.hpp"
#include "text.hpp"
#include "assert.hpp"
#include "url.hpp"
#include "stream.hpp"
#include "robin_hood.hpp"
#include "socket.hpp"
#include "job.hpp"

namespace xstd::http {
	// Byte utils.
	//
	namespace detail {
		template<typename T>
		static std::span<const uint8_t> to_span( const T& v ) {
			auto bytes = std::as_bytes( v );
			return { (const uint8_t*) bytes.data(), bytes.size() };
		}
		template<size_t N>
		static std::span<const uint8_t> to_span( const char( &ref )[ N ] ) {
			return { (const uint8_t*) &ref[ 0 ], N - 1 };
		}
		static std::span<const uint8_t> to_span( std::string_view v ) {
			return { (const uint8_t*) v.data(), v.size() };
		}
		static std::span<const uint8_t> to_span( const std::string& v ) {
			return { (const uint8_t*) v.data(), v.size() };
		}
		template<typename Output, typename... Inputs>
		static void insert_into( Output& out, size_t at, const Inputs&... in ) {
			std::span<const uint8_t> spans[] = { to_span( in )... };
			size_t total = 0;
			for ( auto& span : spans )
				total += span.size();

			auto pos = out.size();
			uninitialized_resize( out, pos + total );
			memmove( at + total + out.data(), at + out.data(), pos - at );

			auto iter = at + out.data();
			for ( auto& span : spans ) {
				memcpy( iter, span.data(), span.size() );
				iter += span.size();
			}
		}
		template<typename Output, typename... Inputs>
		static void append_into( Output& out, const Inputs&... in ) {
			std::span<const uint8_t> spans[] = { to_span( in )... };
			size_t total = 0;
			for ( auto& span : spans )
				total += span.size();

			auto pos = out.size();
			uninitialized_resize( out, pos + total );
			auto iter = pos + out.data();
			for ( auto& span : spans ) {
				memcpy( iter, span.data(), span.size() );
				iter += span.size();
			}
		}

		static bool fast_ieq( std::string_view input, std::string_view against ) {
			size_t count = against.size();
			if ( input.size() != count ) return false;

			uint32_t mismatch = 0;
			if ( count > 4 ) {
				__hint_nounroll()
				for ( size_t it = 0; ( it + 4 ) <= count; it += 4 ) {
					mismatch |= ( (uint32_t&) input[ it ] ^ (uint32_t&) against[ it ] );
				}
				mismatch |= ( (uint32_t&) input[ count - 4 ] ^ (uint32_t&) against[ count - 4 ] );
			} else {
				if ( count >= 2 ) mismatch |= ( (uint16_t&) input[ count & 1 ] ^ (uint16_t&) against[ count & 1 ] );
				if ( count >= 1 ) mismatch |= ( (uint8_t&) input[ 0 ] ^ (uint8_t&) against[ 0 ] );
			}
			return ( mismatch & 0xDFDFDFDF ) == 0;
		}
	};

	// Reads a single HTTP line terminated with \r\n, skips the content in the buffer, returns the line itself as a string view.
	// If line is not terminated yet returns nullopt.
	//
	static std::optional<std::string_view> readln( vec_buffer& buf ) {
		size_t pos = std::string_view{ (char*) buf.data(), buf.size() }.find( "\r\n" );
		if ( pos == std::string::npos )
			return std::nullopt;
		else
			return std::string_view{ (char*) buf.shift( pos + 2 ), pos };
	}
	static auto readln( stream_view stream ) {
		return stream.read_until( []( vec_buffer& buf ) -> std::optional<std::string> {
			if ( auto view = readln( buf ) ) {
				return std::string( *view );
			} else {
				return std::nullopt;
			}
		} );
	}

	// HTTP status.
	//
	namespace detail {
		inline constexpr std::pair<int, std::string_view> status_codes[] = {
			{ 100, "Continue" },
			{ 101, "Switching Protocols" },
			{ 102, "Processing" },
			{ 103, "Early Hints" },
			{ 200, "OK" },
			{ 201, "Created" },
			{ 202, "Accepted" },
			{ 203, "Non-Authoritative Information" },
			{ 204, "No Content" },
			{ 205, "Reset Content" },
			{ 206, "Partial Content" },
			{ 207, "Multi-Status" },
			{ 208, "Already Reported" },
			{ 226, "IM Used" },
			{ 300, "Multiple Choices" },
			{ 301, "Moved Permanently" },
			{ 302, "Found" },
			{ 303, "See Other" },
			{ 304, "Not Modified" },
			{ 305, "Use Proxy" },
			{ 307, "Temporary Redirect" },
			{ 308, "Permanent Redirect" },
			{ 400, "Bad Request" },
			{ 401, "Unauthorized" },
			{ 402, "Payment Required" },
			{ 403, "Forbidden" },
			{ 404, "Not Found" },
			{ 405, "Method Not Allowed" },
			{ 406, "Not Acceptable" },
			{ 407, "Proxy Authentication Required" },
			{ 408, "Request Timeout" },
			{ 409, "Conflict" },
			{ 410, "Gone" },
			{ 411, "Length Required" },
			{ 412, "Precondition Failed" },
			{ 413, "Payload Too Large" },
			{ 414, "URI Too Long" },
			{ 415, "Unsupported Media Type" },
			{ 416, "Range Not Satisfiable" },
			{ 417, "Expectation Failed" },
			{ 418, "I'm a Teapot" },
			{ 421, "Misdirected Request" },
			{ 422, "Unprocessable Entity" },
			{ 423, "Locked" },
			{ 424, "Failed Dependency" },
			{ 425, "Too Early" },
			{ 426, "Upgrade Required" },
			{ 428, "Precondition Required" },
			{ 429, "Too Many Requests" },
			{ 431, "Request Header Fields Too Large" },
			{ 451, "Unavailable For Legal Reasons" },
			{ 500, "Internal Server Error" },
			{ 501, "Not Implemented" },
			{ 502, "Bad Gateway" },
			{ 503, "Service Unavailable" },
			{ 504, "Gateway Timeout" },
			{ 505, "HTTP Version Not Supported" },
			{ 506, "Variant Also Negotiates" },
			{ 507, "Insufficient Storage" },
			{ 508, "Loop Detected" },
			{ 509, "Bandwidth Limit Exceeded" },
			{ 510, "Not Extended" },
			{ 511, "Network Authentication Required" },
		};
	};
	enum class status_category : int {
		invalid =       0,
		informational = 100,
		success =       200,
		redirecting =   300,
		client_error =  400,
		server_error =  500,
	};
	static constexpr status_category get_status_category( int status_code ) {
		if ( status_code <  100 ) return status_category::invalid;
		if ( status_code <= 199 ) return (status_category) 100;
		if ( status_code <= 299 ) return (status_category) 200;
		if ( status_code <= 399 ) return (status_category) 300;
		if ( status_code <= 499 ) return (status_category) 400;
		if ( status_code <= 599 ) return (status_category) 500;
		return status_category::invalid;
	}
	static constexpr std::string_view get_status_message( int status_code ) {
		if ( status_code < 100 || status_code > 599 )
			status_code = 500;
		auto it = std::lower_bound( std::begin( detail::status_codes ), std::end( detail::status_codes ), status_code, []( auto& pair, int val ) { return pair.first < val; } );
		if ( it != std::end( detail::status_codes ) && it->first == status_code ) {
			return it->second;
		} else {
			switch ( get_status_category( status_code ) ) {
				case status_category::informational:
				case status_category::success:       return "OK"sv;
				case status_category::redirecting:   return "Redirecting"sv;
				case status_category::client_error:  return "Bad Request"sv;
				default:
				case status_category::server_error:  return "Internal Server Error"sv;
			}
		}
	}
	static constexpr bool is_success( int status_code )  {
		switch ( get_status_category( status_code ) ) {
			case status_category::success:
			case status_category::informational: return true;
			default:                             return false;
		}
	}
	static constexpr bool is_failure( int status_code ) { 
		return !is_success( status_code ); 
	}

	// HTTP methods.
	//
	enum method_id : uint8_t {
		GET,
		HEAD,
		POST,
		PUT,
		FDELETE,
		CONNECT,
		OPTIONS,
		TRACE,
		PATCH,
		INVALID,
	};
	static constexpr std::pair<uint32_t, std::string_view> method_map[] = {
		{ "GET"_wh, "GET" },
		{ "HEAD"_wh, "HEAD" },
		{ "POST"_wh, "POST" },
		{ "PUT"_wh, "PUT" },
		{ "DELETE"_wh, "DELETE" },
		{ "CONNECT"_wh, "CONNECT" },
		{ "OPTIONS"_wh, "OPTIONS" },
		{ "TRACE"_wh, "TRACE" },
		{ "PATCH"_wh, "PATCH" }
	};
	static constexpr std::string_view name_method( method_id id ) {
		if ( id < INVALID )
			return method_map[ id ].second;
		return {};
	}
	static constexpr method_id find_method( std::string_view name ) {
		auto method_hash = web_hasher{}( name ).as32();
		uint8_t id = 0;
		for ( ; id != INVALID; id++ ) {
			if ( method_map[ id ].first == method_hash ) {
				break;
			}
		}
		return method_id( id );
	}

	// Connection options.
	//
	enum class connection : uint32_t {
		keep_alive = ( uint32_t ) "keep-alive"_wh,
		upgrade =    ( uint32_t ) "upgrade"_wh,
		close =      ( uint32_t ) "close"_wh,
	};

	// Header joining rules.
	//
	static constexpr std::pair<std::string_view, std::string_view> header_join_keys[] = {
		// RFC 9110 Section 5.3
		// `age`, `authorization`, `content-length`, `content-type`,`etag`, `expires`, `from`, `host`, `if-modified-since`, `if-unmodified-since`,`last-modified`, `location`,
		// `max-forwards`, `proxy-authorization`, `referer`,`retry-after`, `server`, or `user-agent` are discarded.
		{ "Age", {} },
		{ "Authorization", {} },
		{ "Content-Length", {} },
		{ "Content-Type", {} },
		{ "ETag", {} },
		{ "Expires", {} },
		{ "From", {} },
		{ "Host", {} },
		{ "If-Modified-Since", {} },
		{ "If-Unmodified-Since", {} },
		{ "Last-Modified", {} },
		{ "Location", {} },
		{ "Max-Forwards", {} },
		{ "Proxy-Authorization", {} },
		{ "Referer", {} },
		{ "Retry-After", {} },
		{ "Server", {} },
		{ "User-Agent", {} },
		// `set-cookie` is always an array. 
		{ "Set-Cookie", { "\0"sv } },
		// For duplicate `cookie` headers, the values are joined together with `; `.
		{ "Cookie", "; " },
		// For all other headers, the values are joined together with `, `.
	};
	static constexpr std::string_view get_header_join_seperator( std::string_view e ) {
		for ( auto& [k, v] : header_join_keys ) {
			if ( detail::fast_ieq( k, e ) )
				return v;
		}
		return ", "sv;
	}

	// HTTP headers structure.
	//
	using  headers_init = std::initializer_list<std::pair<std::string_view, std::string_view>>;
	struct headers {
		enum merge_kind {
			merge_discard,
			merge_discard_if,
			merge_overwrite,
			merge_overwrite_if,
		};
		struct header_entry {
			size_t     k_len =  0;
			vec_buffer line = {};
			header_entry( std::string_view key, std::string_view value ) : k_len( key.size() ) {
				detail::append_into( line, key, ": ", value, "\r\n" );
			}

			// String conversion.
			//
			std::string to_string() const {
				return std::string{ write().substr( 0, line.size() - 2 ) };
			}

			// Returns a view of the raw header line.
			//
			std::string_view write() const {
				return std::string_view{ (char*) line.data(), line.size() };
			}
			
			// Key, value.
			//
			std::string_view key() const {
				return write().substr(0, k_len);
			}
			std::string_view value() const {
				auto result = write().substr(k_len + 2);
				return result.substr( 0, result.size() - 2 );
			}

			// Replaces the value.
			//
			void assign( std::string_view value ) {
				line.resize( k_len + 2 );
				detail::append_into( line, value, "\r\n" );
			}

			// Inserts another value into the inline-array.
			//
			bool merge( std::string_view value, merge_kind kind ) {
				if ( kind == merge_discard ) {
					return true;
				} else if ( kind == merge_overwrite ) {
					assign( value );
					return true;
				}

				auto sep = get_header_join_seperator( key() );
				if ( sep == "\0"sv ) {
					return false;
				}
				if ( sep.empty() ) {
					// discard if gets discarded
					if ( kind == merge_overwrite_if ) {
						assign( value );
					}
				} else {
					detail::insert_into( line, line.size() - 2, sep, value );
				}
				return true;
			}
		};
		using iterator =       typename std::vector<header_entry>::iterator;
		using const_iterator = typename std::vector<header_entry>::const_iterator;
		
		// Underlying storage.
		//
		std::vector<header_entry> storage = {};
		iterator begin() { return storage.begin(); }
		const_iterator begin() const { return storage.begin(); }
		iterator end() { return storage.end(); }
		const_iterator end() const { return storage.end(); }

		// Default construction/move/copy.
		//
		headers() = default;
		headers( headers&& ) noexcept = default;
		headers( const headers& ) = default;
		headers& operator=( headers&& ) noexcept = default;
		headers& operator=( const headers& ) = default;

		// Construction by list of values.
		//
		headers( headers_init entries, bool overwrite = true ) {
			append_range( entries, overwrite );
		}

		// Searcher.
		//
		std::pair<iterator, bool> search( std::string_view key ) {
			auto beg = std::lower_bound( begin(), end(), key.size(), []( const header_entry& a, size_t b ) { return a.k_len < b; } );
			for ( auto it = beg; it != storage.end(); ++it ) {
				if ( it->k_len != key.size() ) break;
				if ( detail::fast_ieq( it->key(), key ) ) return { it, true };
			}
			return { beg, false };
		}
		std::pair<const_iterator, bool> search( std::string_view key ) const {
			return const_cast<headers*>( this )->search( key );
		}
		template<typename It>
		It search_upper( It it ) const {
			It prev = it;
			while ( it != end() && it->k_len == prev->k_len && detail::fast_ieq( it->key(), prev->key() ) ) {
				++it;
			}
			return it;
		}

		// Mutators.
		//
		header_entry& try_emplace( std::string_view key, std::string_view value, merge_kind directive = merge_overwrite ) {
			auto [it, found] = search( key );
			if ( found ) {
				if ( it->merge( value, directive ) )
					return *it;
				it = search_upper( it );
			}
			return *storage.insert( it, header_entry{ key, value } );
		}
		header_entry& try_insert( std::string_view key, std::string_view value ) {
			return try_emplace( key, value, merge_discard );
		}
		header_entry& set( std::string_view key, std::string_view value, bool overwrite = true ) {
			return try_emplace( key, value, overwrite ? merge_overwrite_if : merge_discard_if );
		}
		size_t remove( std::string_view key ) {
			auto [it, found] = search( key );
			if ( found ) {
				auto end = search_upper( it );
				size_t n = end - it;
				storage.erase( it, end );
				return n;
			}
			return 0;
		}
		template<typename T>
		void append_range( T&& range, bool overwrite = true ) {
			for ( auto& [k, v] : range ) {
				set( k, v, overwrite );
			}
		}

		// Observers.
		//
		bool has( std::string_view key ) const {
			return search( key ).second;
		}
		std::optional<std::string_view> get_if( std::string_view key ) const {
			auto [it, found] = search( key );
			if ( found ) return it->value();
			return {};
		}
		std::string_view get( std::string_view key ) const {
			return get_if( key ).value_or( ""sv );
		}
		std::string_view operator[]( std::string_view key ) const {
			return get( key );
		}
		std::span<const header_entry> list( std::string_view key ) {
			auto [beg, found] = search( key );
			if ( !found ) return {};
			return { std::to_address( beg ), std::to_address( search_upper( beg ) ) };
		}

		// Writers.
		//
		void write( vec_buffer& buf ) const {
			for ( auto& pair : *this ) {
				buf.append_range( pair.line );
			}
		}
		auto write( stream_view stream ) const {
			return stream.write_using( [&](vec_buffer& buf) {
				this->write( buf );
			} );
		}

		// Readers.
		//
		std::optional<exception> read( vec_buffer& buf ) {
			while ( true ) {
				auto line = readln( buf );
				if ( !line ) return std::nullopt; // Read more.
				if ( line->empty() ) return exception{}; // Done reading.

				size_t key_end = line->find( ": " );
				if ( key_end == std::string::npos ) {
					// Error.
					return exception( XSTD_ESTR( "invalid header line" ) );
				}

				// Set the value.
				std::string_view key{ line->begin(),                 line->begin() + key_end };
				std::string_view value{ line->begin() + key_end + 2, line->end() };
				this->set( key, value, false );
			}
		}
		auto read( stream_view stream ) {
			return stream.read_until( [&]( vec_buffer& buf ) {
				return this->read( buf );
			} );
		}
	};

	// HTTP body helpers.
	//
	namespace body {
		enum encoding {
			raw,
			chunked, 
			unknown,  // not yet evaluated
			finished, // already read
			error,    // failure
		};
		struct props {
			size_t   length = 0;
			encoding code = unknown;
		};
		static bool is_always_empty( method_id method, int status ) {
			// Any response to a HEAD request and any response with a 1xx (Informational), 204 (No Content), or 304 (Not Modified) status code is always terminated 
			// by the first empty line after the header fields, regardless of the header fields present in the message, and thus cannot contain a message body.
			//
			if ( method == HEAD || status == 204 || status == 304 || ( 100 <= status && status <= 199 ) ) {
				return true;
			}

			// Any 2xx( Successful ) response to a CONNECT request implies that the connection will become a tunnel immediately after the empty line that concludes 
			// the header fields. A client MUST ignore any Content - Length or Transfer - Encoding header fields received in such a message.
			//
			if ( method == CONNECT && ( 200 <= status && status <= 299 ) ) {
				return true;
			}
			return false;
		}
		static props get_properties( method_id method, int status, const headers& hdr ) {
			if ( is_always_empty( method, status ) ) {
				return { 0, finished };
			}

			// If a message is received with both a Transfer-Encoding and a Content-Length header field, the Transfer-Encoding overrides the Content-Length.
			//
			if ( web_hasher{}( hdr[ "Transfer-Encoding" ] ) == "chunked"_wh ) {
				return { std::dynamic_extent, chunked };
			}
			// If a valid Content-Length header field is present without Transfer-Encoding, its decimal value defines the expected message body length.
			//
			else if ( size_t content_length = parse_number<size_t>( hdr[ "Content-Length" ], 10, std::dynamic_extent ); content_length != std::dynamic_extent ) {
				return { content_length, content_length ? raw : finished };
			}
			// If this is a request message and none of the above are true, then the message body length is zero.
			// 
			else if ( status == -1 ) {
				return { 0, finished };
			}
			// Otherwise, this is a response message without a declared message body length, so the message body length is determined by the number of octets received prior to the server closing the connection.
			//
			else {
				return { std::dynamic_extent, raw };
			}
		}

		// Readers.
		//
		static job<bool> read_chunked( vec_buffer& output, stream_view input ) {
			while ( true ) {
				// Read the length.
				//
				auto line = co_await readln( input );
				if ( !line ) [[unlikely]] {
					co_return false;
				}

				// Read the chunk length, if terminator chunk, break.
				//
				size_t n = parse_number<size_t>( *line, 16 );
				if ( !n ) {
					if ( line->empty() ) {
						input.stop( XSTD_ESTR( "invalid chunked message" ) );
						co_return false;
					}
					co_return !!( co_await readln( input ) );
				}

				// Read the chunk + the footer.
				//
				n += 2;
				uint8_t* out = output.push( n );
				co_await input.read_into( out, n );
				if ( memcmp( out + n - 2, "\r\n", 2 ) ) [[unlikely]] {
					input.stop( XSTD_ESTR( "invalid chunked message" ) );
					co_return false;
				} else {
					output.pop( 2 );
				}
			}
		}
		static job<bool> read_raw( vec_buffer& output, stream_view input, size_t content_length = std::dynamic_extent ) {
			if ( content_length ) {
				if ( !co_await input.read_into( output.push( content_length ), content_length ) ) {
					co_return false;
				}
			} else if ( content_length == std::dynamic_extent ) {
				co_await input.read_until( [&]( auto& ) { return false; } );
				output = co_await input.read( 0, content_length );
				if ( input.stop_code() != stream_stop_fin ) {
					co_return false;
				}
			}
			co_return true;
		}
		static job<bool> read( vec_buffer& output, stream_view input, props& prop ) {
			bool ok = false;
			switch ( prop.code ) {
				case chunked:  ok = co_await read_chunked( output, input ); break;
				case raw:      ok = co_await read_raw( output, input, prop.length ); break;
				case finished: co_return true;
				case error:    co_return false;
				default:       break;
			}
			if ( ok ) prop = { output.size(), finished };
			else      prop = { 0,             error };
			co_return ok;
		}

		// Writer, writes headers as well as it may need to be modified.
		// - TODO: chunked & streaming.
		//
		static void write( vec_buffer& output, std::span<const uint8_t> input, method_id method, int status ) {
			const bool is_request = status < 100;

			if ( input.empty() || is_always_empty( method, status ) ) {
				if ( !is_request )
					detail::append_into( output, "Content-Length: 0\r\n\r\n" );
				else
					detail::append_into( output, "\r\n" );
			} else {
				char buffer[ 64 ];
				auto last_header = xstd::fmt::into( buffer, "Content-Length: %llu\r\n\r\n", input.size() );
				output.append_range( std::span{ (uint8_t*) last_header.data(), last_header.size() } );
				output.append_range( input );
			}
		}
	}

	// Base message type.
	//
	struct message {
		struct options {
			vec_buffer    body = {};
			http::headers headers = {};
		};

		// Data.
		//
		vec_buffer      body = {};
		http::headers   headers = {};
		body::props     body_props = { 0, body::unknown };
		exception       connection_error = {};

		// Error state.
		//
		bool ok() const {
			return !connection_error;
		}
		exception error() const {
			return connection_error;
		}

		// Default, copy, move.
		//
		message() = default;
		message( message&& ) noexcept = default;
		message( const message& ) noexcept = default;
		message& operator=( message&& ) noexcept = default;
		message& operator=( const message& ) noexcept = default;

		// By value.
		//
		explicit message( exception error ) : connection_error( std::move( error ) ) {}
		message( vec_buffer body, http::headers headers = {} ) : body( std::move( body ) ), headers( std::move( headers ) ) { this->body_props = { this->body.size(), body::finished }; }
		message( options&& opt ) : message( std::move( opt.body ), std::move( opt.headers ) ) {}

		// Header modifiers.
		//
		std::string_view get_header( std::string_view key ) const {
			return headers.get( key );
		}
		void set_header( std::string_view key, std::string_view value, bool overwrite = true ) {
			headers.set( key, value, overwrite );
		}
		void set_headers( std::initializer_list<std::pair<std::string_view, std::string_view>> entries, bool overwrite = true ) {
			headers.append_range( entries, overwrite );
		}
		size_t remove_header( std::string_view key ) {
			return headers.remove( key );
		}

		// Readers.
		//
		job<bool> read_body( stream_view stream ) {
			return body::read( body, stream, body_props );
		}
		auto read_headers( stream_view stream ) {
			return headers.read( stream );
		}

		// Properties.
		//
		body::props get_body_properties( method_id req_method, int status ) const {
			return body::get_properties( req_method, status, headers );
		}
		http::connection connection() const {
			std::string_view header = headers[ "Connection" ];
			for ( auto& e : split_fwd<4>( header, ", " ) ) {
				auto res = (http::connection) web_hasher{}( e ).as32();
				if ( res == http::connection::keep_alive ) return res;
				if ( res == http::connection::close ) return res;
				if ( res == http::connection::upgrade ) return res;
			}
			return http::connection::keep_alive;
		}
		bool keep_alive() const { 
			return connection() == connection::keep_alive || connection() == connection::upgrade;
		}
		bool has_body() const {
			return body_props.length != 0;
		}
		bool is_body_read() const {
			return body_props.code == body::finished;
		}
	};

	// HTTP response and request as data types.
	//
	struct response : message {
		struct options {
			int status = 200;
			std::string_view message;
			vec_buffer body = {};
			http::headers headers = {};
		};

		// Status line.
		//
		int             status =         -1;
		std::string     status_message = {};
		
		// Default, copy, move.
		//
		response() = default;
		response( response&& ) noexcept = default;
		response( const response& ) noexcept = default;
		response& operator=( response&& ) noexcept = default;
		response& operator=( const response& ) noexcept = default;

		// By value.
		//
		explicit response( exception error ) : message( std::move( error ) ) {}
		response( int status, std::string_view msg = {}, vec_buffer body = {}, http::headers headers = {} ) : message( std::move( body ), std::move( headers ) ), status( status ), status_message( msg ) {
			if ( this->status_message.empty() )
				this->status_message = get_status_message( this->status );
		}
		response( vec_buffer body, http::headers headers = {} ) : message( std::move( body ), std::move( headers ) ), status( 200 ), status_message( "OK" ) {}
		response( options&& opt ) : response( opt.status, opt.message, std::move( opt.body ), std::move( opt.headers ) ) {}

		// Error state including status.
		//
		bool ok() const {
			return message::ok() && is_success( status );
		}
		exception error() const {
			if ( !message::ok() )
				return message::error();
			if ( !is_success( status ) )
				return status_message;
			return {};
		}

		// Writers.
		//
		void write( vec_buffer& buf, method_id req_method = INVALID ) const {
			uint8_t status_buffer[ 3 ] = {};
			
			int sit = status <= 99 || status > 999 ? 500 : status;
			for ( int i = 2; i >= 0; i-- ) {
				status_buffer[ i ] = uint8_t( '0' + sit % 10 );
				sit /= 10;
			}

			detail::append_into( buf, "HTTP/1.1 ", std::span{ status_buffer }, " ", status_message, "\r\n" );
			headers.write( buf );
			body::write( buf, body, req_method, -1 );
		}
		auto write( stream_view stream, method_id req_method = INVALID ) const {
			return stream.write_using( [&]( vec_buffer& buf ) {
				this->write( buf, req_method );
			} );
		}
		std::string to_string() const {
			vec_buffer buf{};
			write( buf );
			return std::string{ (char*) buf.data(), buf.size() };
		}
		vec_buffer write() const {
			vec_buffer result = {};
			write( result );
			return result;
		}

		// Readers.
		//
		job<bool> read_head( stream_view stream, method_id req_method = INVALID ) {
			// Read the status line.
			//
			auto meta = co_await readln( stream );
			if ( !meta ) {
				co_return false;
			}
			if ( !meta->starts_with( "HTTP/1.1 " ) ) {
				stream.stop( XSTD_ESTR( "invalid response line: http version" ) );
				co_return false;
			}
			std::string_view status_and_msg{ meta->data() + xstd::strlen( "HTTP/1.1 " ), meta->size() };
			if ( status_and_msg.size() < 4 || status_and_msg[ 3 ] != ' ' ) {
				stream.stop( XSTD_ESTR( "invalid response line: status code" ) );
				co_return false;
			}
			status = 0;
			for ( size_t i = 0; i != 3; i++ ) {
				if ( '0' <= status_and_msg[ i ] && status_and_msg[ i ] <= '9' ) {
					status *= 10;
					status += ( status_and_msg[ i ] - '0' );
				} else {
					stream.stop( XSTD_ESTR( "invalid response line: status code" ) );
					co_return false;
				}
			}
			status_and_msg.remove_prefix( 4 );
			meta->erase( meta->begin(), meta->begin() + xstd::strlen( "HTTP/1.1 " ) + 4 );
			status_message = std::move( meta ).value();
			if ( !is_success( status ) && status_message.empty() )
				status_message = get_status_message( status );

			// Read the headers, set body properties.
			//
			bool ok = true;
			if ( auto err = co_await this->read_headers( stream ); err == std::nullopt || err->has_value() ) {
				ok = false;
				if ( err ) stream.stop( std::move( *err ) );
			}
			this->body_props = this->get_body_properties( req_method );
			co_return ok;
		}
		job<bool> read( stream_view stream, method_id req_method = INVALID ) {
			co_return co_await read_head( stream, req_method ) && co_await read_body( stream );
		}

		// Sync parser.
		//
		static response parse( vec_buffer& io, method_id req_method = INVALID ) {
			stream mem{ stream::memory( io ) };
			response retval = {};
			if ( !retval.read( &mem, req_method ).run() ) {
				exception ex = {};
				if ( mem.errored() )
					ex = mem.stop_reason();
				retval.connection_error = std::move( ex ).value_or( XSTD_ESTR( "unfinished stream" ) );
			}
			io = std::move( mem.buffer_ );
			return retval;
		}

		// Properties.
		//
		body::props get_body_properties( method_id req_method = INVALID ) const {
			return message::get_body_properties( req_method, status );
		}

		// Receives a response from the stream.
		//
		static job<response> receive( stream_view stream, method_id req_method = INVALID ) {
			response res = {};
			co_await res.read( stream, req_method );
			if ( stream.errored() )
				res.connection_error = stream.stop_reason();
			else if ( !res.keep_alive() )
				stream.stop();
			co_return std::move( res );
		}
	};
	struct request : message {
		struct options {
			method_id method = INVALID;
			std::string_view path = {};
			vec_buffer body = {};
			http::headers headers = {};
		};

		// Request line.
		//
		method_id       method = INVALID;
		std::string     path =   {};

		// Default, copy, move.
		//
		request() = default;
		request( request&& ) noexcept = default;
		request( const request& ) noexcept = default;
		request& operator=( request&& ) noexcept = default;
		request& operator=( const request& ) noexcept = default;

		// By value.
		//
		explicit request( exception error ) : message( std::move( error ) ) {}
		request( method_id method, std::string_view path, vec_buffer body = {}, http::headers headers = {} ) : message( std::move( body ), std::move( headers ) ), method( method ), path( path ) {
			if ( this->method == INVALID )
				this->method = this->body ? POST : GET;
			if ( this->path.empty() )
				this->path = "/";
		}
		request( std::string_view path, vec_buffer body = {}, http::headers headers = {} ) : request( INVALID, path, std::move( body ), std::move( headers ) ) {}
		request( options&& opt ) : request( opt.method, opt.path, std::move( opt.body ), std::move( opt.headers ) ) {}

		// Writers.
		//
		void write( vec_buffer& buf ) const {
			detail::append_into( buf, method_map[ size_t( method ) ].second, " ", path, " HTTP/1.1\r\n" );
			headers.write( buf );
			body::write( buf, body, method, -1 );
		}
		auto write( stream_view stream ) const {
			return stream.write_using( [&]( vec_buffer& buf ) {
				this->write( buf );
			} );
		}
		std::string to_string() const {
			vec_buffer buf{};
			write( buf );
			return std::string{ (char*) buf.data(), buf.size() };
		}
		vec_buffer write() const {
			vec_buffer result = {};
			write( result );
			return result;
		}

		// Readers.
		//
		job<bool> read_head( stream_view stream ) {
			// Read the request line.
			//
			auto meta = co_await readln( stream );
			if ( !meta ) {
				co_return false;
			}
			auto [method_name, pathname, version] = split_fwd<3>( *meta, " " );
			method = find_method( method_name );
			path = pathname;
			if ( version != "HTTP/1.1" ) {
				stream.stop( XSTD_ESTR( "invalid request line: http version" ) );
				co_return false;
			}
			if ( method == INVALID ) {
				stream.stop( XSTD_ESTR( "invalid request line: method" ) );
				co_return false;
			}

			// Read the headers, set body properties.
			//
			bool ok = true;
			if ( auto err = co_await this->read_headers( stream ); err == std::nullopt || err->has_value() ) {
				ok = false;
				if ( err ) stream.stop( std::move( *err ) );
			}
			this->body_props = this->get_body_properties();
			co_return ok;
		}
		job<bool> read( stream_view stream ) {
			co_return co_await read_head( stream ) && co_await read_body( stream );
		}

		// Sync parser.
		//
		static request parse( vec_buffer& io ) {
			stream mem{ stream::memory( io ) };
			request retval = {};
			if ( !retval.read( &mem ).run() ) {
				exception ex = {};
				if ( mem.errored() )
					ex = mem.stop_reason();
				retval.connection_error = std::move( ex ).value_or( XSTD_ESTR( "unfinished stream" ) );
			}
			io = std::move( mem.buffer_ );
			return retval;
		}

		// Properties.
		//
		body::props get_body_properties() const {
			return message::get_body_properties( method, -1 );
		}

		// Receives a request from the stream.
		//
		static job<request> receive( stream_view stream ) {
			request req = {};
			co_await req.read( stream );
			if ( stream.errored() )
				req.connection_error = stream.stop_reason();
			else if ( !req.keep_alive() )
				stream.stop();
			co_return std::move( req );
		}
	};
	
	// HTTP agent structure.
	// - Currently only manages the socket pool, TODO: Caching, cookies.
	//
	struct agent : std::enable_shared_from_this<agent> {
		// Creates a connection given the hostname or IP.
		//
		virtual job<result<unique_stream>> connect( const char* hostname, uint16_t port ) = 0;
		virtual job<result<unique_stream>> connect( net::ipv4 ip, uint16_t port ) {
			char buf[ 24 ];
			sprintf( buf, "%u", bswap( ip.to_integer() ) );
			co_return co_await connect( buf, port );
		}


		// Utilities.
		//
		void set_global() {
			agent::set_global( shared_from_this() );
		}

		// Virtual dtor.
		//
		virtual ~agent() = default;

		// Global agent.
		//
		inline static xstd::xspinlock<>      g_agent_mtx = {};
		inline static std::shared_ptr<agent> g_agent = nullptr;
		inline static void set_global( std::shared_ptr<agent> agent ) {
			std::lock_guard _g{ g_agent_mtx };
			g_agent = std::move( agent );
		}
		template<typename T, typename... Args>
		inline static void make_global(Args&&... args) {
			set_global( std::make_shared<T>( std::forward<Args>( args )... ) );
		}
		inline static std::shared_ptr<agent> global() {
			std::lock_guard _g{ g_agent_mtx };
			return g_agent;
		}
	};
#if XSTD_HAS_TCP
	struct basic_agent : agent {
		// Lock guarding the overall structure and the connection pool.
		//
		xstd::xspinlock<>                                       mtx;
		robin_hood::unordered_flat_map<uint64_t, unique_stream> connection_pool;

		// Socket wrapper with connection metadata.
		//
		struct shared_socket {
			unique_stream              socket;
			uint64_t                   cache_uid;
			std::weak_ptr<basic_agent> source;

			stream_state& state() { return socket.state(); }
			async_buffer& readable() { return socket.readable(); }
			async_buffer& writable() { return socket.writable(); }

			~shared_socket() {
				if ( !socket || socket.stopped() ) return;
				if ( auto agent = source.lock() ) {
					std::lock_guard _g{ agent->mtx };
					agent->connection_pool.try_emplace( cache_uid, std::move( socket ) );
				}
			}
		};

		// Creates a connection given the hostname or IP.
		//
		job<result<unique_stream>> connect( net::ipv4 ip, uint16_t port ) override {
			uint64_t uid = ( uint64_t( ip.to_integer() ) << 16 ) | port;

			unique_stream result;
			{
				std::lock_guard _g{ mtx };
				auto it = connection_pool.find( uid );
				if ( it != connection_pool.end() ) {
					if ( !it->second.stopped() ) {
						result = std::move( it->second );
					}
					connection_pool.erase( it );
				}
			}
			co_return new shared_socket{
				result ? std::move( result ) : new net::tcp( ip, port ),
				uid,
				std::static_pointer_cast<basic_agent>( shared_from_this() )
			};
		}
		job<result<unique_stream>> connect( const char* hostname, uint16_t port ) override {
			auto ip = co_await net::resolve_hostname( hostname );
			if ( !ip ) co_return std::move( ip.status );
			co_return co_await connect( *ip, port );
		}
	};
#endif

	// Fetch API.
	//
	struct fetch_options {
		// Message.
		//
		vec_buffer    body = {};
		http::headers headers = {};

		// Agent and optional explicit socket.
		//
		std::shared_ptr<agent> agent =  agent::global();
		stream_view            socket = nullptr;

		// Redirection handling.
		// - Once the limit is exhaused, response code is returned as is.
		//
		int32_t                max_redirects = 8;
	};
	inline job<response> fetch( xstd::url url, method_id method = GET, fetch_options&& opt = {} ) {
		auto        agent = std::move( opt.agent );
		int32_t     redirects_left = opt.max_redirects;
		stream_view stream = opt.socket;

		// Create the request.
		//
		request req{ method, {}, std::move( opt.body ), std::move( opt.headers ) };
		response res = {};
		while ( true ) {
			// Determine the protocol.
			//
			if ( url.schema.empty() ) {
				url.schema = "http";
			} else if ( !detail::fast_ieq( url.schema, "http" ) ) {
				res.connection_error = exception{ XSTD_ESTR( "protocol '%s' not supported" ), url.schema };
				break;
			}

			// Set host header.
			//
			req.path = url.path();
			if ( !url.hostname.empty() ) {
				req.headers.try_emplace( "Host", url.host(), http::headers::merge_overwrite );
			}

			// Establish the socket.
			//
			unique_stream tmp;
			if ( !stream ) {
				if ( !agent ) {
					res.connection_error = XSTD_ESTR( "neither socket nor agent was specified" );
					break;
				}
				auto sock = co_await agent->connect( url.hostname.c_str(), url.port_or_default() );
				if ( !sock ) {
					res.connection_error = std::move( sock.status );
					break;
				}
				tmp = std::move( sock.value() );
				stream = tmp;
			}

			// Send the request.
			//
			co_await req.write( stream );

			// Read the response.
			//
			co_await res.read( stream, req.method );
			if ( stream.errored() ) {
				req.connection_error = stream.stop_reason();
				break;
			}
			else if ( !req.keep_alive() ) {
				stream.stop();
				stream = {};
			}

			// Follow redirects where relevant.
			//
			if ( get_status_category( res.status ) == status_category::redirecting && --redirects_left > 0 ) {
				std::string_view target_loc = res.get_header( "Location" );
				if ( target_loc.empty() ) break;

				if ( res.status == 303 ) {
					req.method = GET;
					req.body.clear();
				}
				if ( target_loc.starts_with( "/" ) ) {
					url.pathname = target_loc;
				} else {
					xstd::url new_url = target_loc;
					if ( new_url.hostname != url.hostname ) {
						stream = {};
					}
					auto prev_q = std::move( url.search );
					url = std::move( new_url );
					url.search = std::move( prev_q );
				}
				res = {};
				continue;
			}
			break;
		}
		co_return res;
	}

	// Wrappers for the most common methods.
	//
	inline auto get( xstd::url url, fetch_options&& opt = {} ) { return fetch( std::move( url ), GET, std::move( opt ) ); }
	inline auto put( xstd::url url, fetch_options&& opt = {} ) { return fetch( std::move( url ), PUT, std::move( opt ) ); }
	inline auto post( xstd::url url, fetch_options&& opt = {} ) { return fetch( std::move( url ), POST, std::move( opt ) ); }
	inline auto head( xstd::url url, fetch_options&& opt = {} ) { return fetch( std::move( url ), HEAD, std::move( opt ) ); }
};