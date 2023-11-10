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
	enum class status_category : uint8_t {
		none =          0,
		informational = 1,
		success =       2,
		redirecting =   3,
		client_error =  4,
		server_error =  5,
	};
	static constexpr status_category get_status_category( int status_code ) {
		return status_category( status_code / 100 ); 
	}
	static constexpr bool is_success( int status_code )  { 
		switch ( get_status_category( status_code ) ) {
			case status_category::success:
			case status_category::informational: return true;
			default:                             return false;
		}
	}
	static constexpr bool is_failure( int status_code ) { return !is_success( status_code ); }

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
	static constexpr std::tuple<uint32_t, std::string_view, std::string_view> header_join_keys[] = {
		// RFC 9110 Section 5.3
		// `age`, `authorization`, `content-length`, `content-type`,`etag`, `expires`, `from`, `host`, `if-modified-since`, `if-unmodified-since`,`last-modified`, `location`,
		// `max-forwards`, `proxy-authorization`, `referer`,`retry-after`, `server`, or `user-agent` are discarded.
		{ "Age"_wh, "Age", {} },
		{ "Authorization"_wh, "Authorization", {} },
		{ "Content-Length"_wh, "Content-Length", {} },
		{ "Content-Type"_wh, "Content-Type", {} },
		{ "ETag"_wh, "ETag", {} },
		{ "Expires"_wh, "Expires", {} },
		{ "From"_wh, "From", {} },
		{ "Host"_wh, "Host", {} },
		{ "If-Modified-Since"_wh, "If-Modified-Since", {} },
		{ "If-Unmodified-Since"_wh, "If-Unmodified-Since", {} },
		{ "Last-Modified"_wh, "Last-Modified", {} },
		{ "Location"_wh, "Location", {} },
		{ "Max-Forwards"_wh, "Max-Forwards", {} },
		{ "Proxy-Authorization"_wh, "Proxy-Authorization", {} },
		{ "Referer"_wh, "Referer", {} },
		{ "Retry-After"_wh, "Retry-After", {} },
		{ "Server"_wh, "Server", {} },
		{ "User-Agent"_wh, "User-Agent", {} },
		// `set-cookie` is always an array. 
		{ "Set-Cookie"_wh, "Set-Cookie", { "\0"sv } },
		// For duplicate `cookie` headers, the values are joined together with `; `.
		{ "Cookie"_wh, "Cookie", "; " },
		// For all other headers, the values are joined together with `, `.
	};
	static constexpr std::string_view get_header_join_seperator( std::string_view e, std::optional<uint32_t> hash_hint = {} ) {
		uint32_t hash = hash_hint.value_or( web_hasher{}( e ) );
		for ( auto& [h, k, v] : header_join_keys ) {
			if ( h == hash && k == e ) 
				return v;
		}
		return ", "sv;
	}

	// HTTP headers structure.
	//
	struct headers {
		enum merge_kind {
			merge_discard,
			merge_discard_if,
			merge_overwrite,
			merge_overwrite_if,
		};
		struct header_entry {
			size_t     k_len =  0;
			uint32_t   k_hash = 0;
			vec_buffer line = {};
			header_entry( uint32_t hash, std::string_view key, std::string_view value ) : k_len( key.size() ), k_hash( hash ) {
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

				auto sep = get_header_join_seperator( key(), k_hash );
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
		headers( std::initializer_list<std::pair<std::string_view, std::string_view>> entries, bool overwrite = true ) {
			append_range( entries, overwrite );
		}

		// Searcher.
		//
		std::pair<iterator, bool> search( std::string_view key, std::optional<uint32_t> hash_hint = {} ) {
			uint32_t hash = hash_hint.value_or( web_hasher{}( key ) );
			auto beg = std::lower_bound( begin(), end(), hash, []( const header_entry& a, uint32_t b ) { return a.k_hash < b; } );
			for ( auto it = beg; it != storage.end(); ++it ) {
				if ( it->k_hash != hash ) break;
				if ( xstd::iequals( it->key(), key ) ) return { it, true };
			}
			return { beg, false };
		}
		std::pair<const_iterator, bool> search( std::string_view key, std::optional<uint32_t> hash_hint = {} ) const {
			return const_cast<headers*>( this )->search( key, hash_hint );
		}
		template<typename It>
		It search_upper( It it ) const {
			It prev = it;
			while ( it != end() && it->k_hash == prev->k_hash && xstd::iequals( it->key(), prev->key() ) ) {
				++it;
			}
			return it;
		}

		// Mutators.
		//
		header_entry& try_emplace( std::string_view key, std::string_view value, merge_kind directive = merge_overwrite ) {
			uint32_t hash = web_hasher{}( key );
			auto [it, found] = search( key, hash );
			if ( found ) {
				if ( it->merge( value, directive ) )
					return *it;
				it = search_upper( it );
			}
			return *storage.insert( it, header_entry{ hash, key, value } );
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
				if ( auto err = this->read( buf ) ) {
					if ( *err ) stream.stop( std::move( *err ) );
					return true;
				} else {
					return false;
				}
			} );
		}
	};

	// HTTP agent structure.
	// - Currently only manages the socket pool, TODO: Caching, cookies.
	//
	struct agent {
		// Releases a connection back to the pool after a request is complete with keep-alive.
		//
		virtual void release( unique_stream& sock ) {
			(void) sock;
		}

		// Creates a connection using the pool.
		//
		virtual unique_stream connect_by_ip( net::ipv4 ip, uint16_t port, net::socket_options opts = {} ) {
			return { new net::tcp( ip, port, std::move( opts ) ) };
		}
		virtual job<result<unique_stream>> connect_by_name( const char* hostname, uint16_t port, net::socket_options opts = {} ) {
			auto ip = co_await net::resolve_hostname( hostname );
			if ( !ip ) co_return std::move( ip.status );
			co_return connect_by_ip( *ip, port, opts );
		}

		// Virtual dtor.
		//
		virtual ~agent() = default;
		
		// Global agent (no-op by default).
		//
		inline static std::atomic<agent*> g_agent = nullptr;
		template<typename Type, typename... Tx>
		inline static void set_global( Tx&&... args ) {
			// Will never be destroyed.
			g_agent = new Type( std::forward<Tx>( args )... );
		}
		inline static agent& global() {
			auto* result = g_agent.load( std::memory_order::relaxed );
			if ( !result ) {
				result = (agent*) &xstd::make_default<agent>();
			}
			return *result;
		}
	};
	struct shared_agent : agent {
		xstd::xspinlock<> mtx;

		// Connection pool.
		//
		robin_hood::unordered_flat_map<uint64_t, unique_stream> connection_pool;
		static constexpr uint64_t remote_uid( net::ipv4 ip, uint16_t port ) {
			uint64_t uid = { ip.to_integer() };
			uid = ( uid << 16 ) | port;
			return uid;
		}
		void release( unique_stream& sock ) override {
			if ( auto* tcp = sock.get_if<net::tcp>() ) {
				auto [ip, port] = tcp->get_remote_address();
				auto uid = remote_uid( ip, port );
				std::lock_guard _g{ mtx };
				connection_pool.try_emplace( uid, std::move( sock ) );
			}
		}
		unique_stream connect_by_ip( net::ipv4 ip, uint16_t port, net::socket_options opts = {} ) override {
			unique_stream result;
			{
				std::lock_guard _g{ mtx };
				auto it = connection_pool.find( remote_uid( ip, port ) );
				if ( it != connection_pool.end() ) {
					if ( !it->second.stopped() ) {
						result = std::move( it->second );
					}
					connection_pool.erase( it );
				}
			}
			if ( !result ) {
				result = agent::connect_by_ip( ip, port, std::move( opts ) );
			}
			return result;
		}
	};

	// HTTP body helpers.
	//
	namespace body {
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
				co_await input.read_until( [&]( vec_buffer& v ) { return false; } );
				output = co_await input.read( 0, content_length );
				if ( input.stop_code() != stream_stop_fin ) {
					co_return false;
				}
			}
			co_return true;
		}
		static job<bool> read( vec_buffer& output, stream_view input, method_id method, int status, const headers& hdr ) {
			const bool is_request = status < 100;

			if ( is_always_empty( method, status ) ) {
				return read_raw( output, input, 0 );
			} 
			// If a message is received with both a Transfer-Encoding and a Content-Length header field, the Transfer-Encoding overrides the Content-Length.
			//
			else if ( web_hasher{}( hdr[ "Transfer-Encoding" ] ) == "chunked"_wh ) {
				return read_chunked( output, input );
			}
			// If a valid Content-Length header field is present without Transfer-Encoding, its decimal value defines the expected message body length.
			//
			else if ( size_t content_length = parse_number<size_t>( hdr[ "Content-Length" ], 10, std::dynamic_extent ); content_length != std::dynamic_extent ) {
				return read_raw( output, input, content_length );
			} 
			// If this is a request message and none of the above are true, then the message body length is zero.
			// Otherwise, this is a response message without a declared message body length, so the message body length is determined by the number of octets received prior to the server closing the connection.
			//
			else {
				return read_raw( output, input, is_request ? 0 : content_length );
			}
		}

		// Writer, writes headers as well as it may need to be modified.
		// - TODO: chunked & streaming.
		//
		static void write( vec_buffer& output, vec_buffer input, method_id method, int status ) {
			const bool is_request = status < 100;

			if ( input.empty() || is_always_empty( method, status ) ) {
				if ( !is_request )
					detail::append_into( output, "Content-Length: 0\r\n\r\n" );
				else
					detail::append_into( output, "\r\n" );
			} else {
				char buffer[ 64 ];
				auto last_header = xstd::fmt::into( buffer, "Content-Length: %llu\r\n\r\n", input.size() );
				input.prepend_range( std::span{ (uint8_t*) last_header.data(), last_header.size() } );
				output.append_range( std::move( input ) );
			}
		}
	}

	// Base message type.
	//
	struct message {
		// Data.
		//
		vec_buffer      body = {};
		http::headers   headers = {};

		// Header modifiers.
		//
		std::string_view get( std::string_view key ) const {
			return headers.get( key );
		}
		void set( std::string_view key, std::string_view value, bool overwrite = true ) {
			headers.set( key, value, overwrite );
		}
		void set( std::initializer_list<std::pair<std::string_view, std::string_view>> entries, bool overwrite = true ) {
			headers.append_range( entries, overwrite );
		}
		size_t remove( std::string_view key ) {
			return headers.remove( key );
		}

		// Properties.
		//
		bool has_body( method_id req_method, int status ) const {
			return !body::is_always_empty( req_method, status );
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
		bool keep_alive() const { return connection() == connection::keep_alive; }
	};

	// HTTP response and request as data types.
	//
	struct response : message {
		// Status line.
		//
		int             status =         200;
		std::string     status_message = "OK";

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
			body::write( buf, std::move( body ), req_method, -1 );
		}
		auto write( stream_view stream, method_id req_method = INVALID ) const {
			return stream.write_using( [&]( vec_buffer& buf ) {
				this->write( buf, req_method );
			} );
		}

		// Readers.
		//
		job<bool> read_head( stream_view stream ) {
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

			// Read the headers.
			//
			co_return co_await headers.read( stream );
		}
		job<bool> read_body( stream_view stream, method_id req_method = INVALID ) {
			return body::read( body, stream, req_method, status, headers );
		}
		job<bool> read( stream_view stream, method_id req_method = INVALID ) {
			bool ok = co_await read_head( stream );
			if ( ok && has_body( req_method ) )
				ok = co_await read_body( stream, req_method );
			co_return ok;
		}

		// Properties.
		//
		bool has_body( method_id req_method = INVALID ) const {
			return message::has_body( req_method, status );
		}
	};
	struct request : message {
		// Request line.
		//
		method_id       method = INVALID;
		std::string     path = {};

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

			// Read the headers.
			//
			co_return co_await headers.read( stream );
		}
		job<bool> read_body( stream_view stream ) {
			return body::read( body, stream, method, -1, headers );
		}
		job<bool> read( stream_view stream ) {
			bool ok = co_await read_head( stream );
			if ( ok && has_body() )
				ok = co_await read_body( stream );
			co_return ok;
		}

		// Properties.
		//
		bool has_body() const {
			return message::has_body( method, -1 );
		}
	};

	// Incoming versions with saved sockets and deferred body readers.
	//
	struct incoming_response : response {
		// State passed over.
		//
		exception      external_error =  {};
		unique_stream  socket =          nullptr;
		http::agent*   agent =           nullptr;
		bool           body_read =       false;

		incoming_response() = default;
		incoming_response( unique_stream socket, http::agent* agent = nullptr ) : socket( std::move( socket ) ), agent( agent ) {}
		incoming_response( exception error ) : external_error( std::move( error ).value_or( XSTD_ESTR( "unexpected error" ) ) ) {}
		incoming_response( response&& res ) : response( std::move( res ) ) {}
		incoming_response( incoming_response&& ) noexcept = default;
		incoming_response& operator=( incoming_response&& ) noexcept = default;

		// Read body.
		//
		task<std::span<const uint8_t>, exception> body() {
			if ( external_error )
				co_yield external_error;
			if ( !body_read ) {
				body_read = true;
				co_await response::read_body( socket );
			}
			if ( socket.errored() )
				co_yield socket.stop_reason();
			co_return response::body;
		}

		// Error state.
		//
		bool ok() const { 
			return is_success( status ) && socket && !socket.errored(); 
		}
		exception error() const {
			if ( external_error ) return external_error;
			return socket.errored() ? socket.stop_reason() : std::nullopt;
		}

		// Starts receiving a response.
		//
		static job<incoming_response> receive( unique_stream socket, http::agent* agent = nullptr, method_id req_method = INVALID, vec_buffer&& req_buf = {} ) {
			incoming_response res = { std::move( socket ), agent };
			res.response::body = std::move( req_buf );
			res.response::body.clear();
			res.status = -1;
			res.status_message = {};
			co_await res.read_head( res.socket );
			if ( !res.has_body( req_method ) )
				res.body_read = true;
			co_return std::move( res );
		}

		// Release socket on destruction for keep-alive.
		//
		~incoming_response() {
			if ( keep_alive() && body_read ) {
				agent->release( socket );
			}
		}
	};
	struct incoming_request : request {
		stream_view socket = nullptr;
		bool        body_read = false;

		incoming_request() = default;
		incoming_request( stream_view socket ) : socket( socket ) {}
		incoming_request( request&& res ) : request( std::move( res ) ) {}
		incoming_request( incoming_request&& ) noexcept = default;
		incoming_request& operator=( incoming_request&& ) noexcept = default;

		// Read body.
		//
		task<std::span<const uint8_t>, exception> body() {
			if ( !body_read ) {
				body_read = true;
				co_await request::read_body( socket );
			}
			if ( socket.errored() )
				co_yield socket.stop_reason();
			co_return request::body;
		}

		// Error state.
		//
		exception error() const {
			return socket.errored() ? socket.stop_reason() : std::nullopt;
		}

		// Starts receiving a request.
		//
		static job<incoming_request> receive( stream_view socket, vec_buffer&& req_buf = {} ) {
			incoming_request res = { std::move( socket ) };
			res.request::body = std::move( req_buf );
			res.request::body.clear();
			co_await res.read_head( res.socket );
			if ( !res.has_body() )
				res.body_read = true;
			co_return std::move( res );
		}
	};

	// Fetch API.
	//
	struct fetch_options {
		vec_buffer               body = {};
		http::headers            headers = {};
		agent&                   agent =  agent::global();
		unique_stream            socket = nullptr;
		net::socket_options      socket_options = {};
	};
	inline job<incoming_response> fetch( url_view url, method_id method = GET, fetch_options&& opt = {} ) {
		// Set host header, normalize path into URI.
		//
		if ( url.schema.empty() ) {
			url.schema = "http";
		}
		if ( url.hostname.empty() ) {
			if ( auto* tcp = opt.socket.get_if<net::tcp>() ) {
				auto [ip, port] = tcp->get_remote_address();
				url.hostname = ip.to_string();
				if ( !url.port && port != url.port_or_default() )
					url.port = port;
			}
		}
		if ( !url.hostname.empty() ) {
			opt.headers.try_insert( "Host", url.host() );
		}

		// Establish the socket.
		//
		if ( !opt.socket ) {
			auto sock = co_await opt.agent.connect_by_name( std::string{ url.hostname }.c_str(), url.port_or_default(), opt.socket_options );
			if ( !sock ) {
				co_return incoming_response{ std::move( sock.status ) };
			}
			opt.socket = *std::move( sock );
		}

		// Write the request.
		//
		request req{ message{ std::move( opt.body ), std::move( opt.headers ) }, method, std::string{url.pathname} };
		co_await req.write( opt.socket );

		// Read the response and return.
		//
		co_return co_await incoming_response::receive( std::move( opt.socket ), &opt.agent, req.method, std::move( req.body ) );
	}

	// Wrappers for the most common methods.
	//
	inline job<incoming_response> get( url_view url, fetch_options&& opt = {} ) { return fetch( url, GET, std::move( opt ) ); }
	inline job<incoming_response> put( url_view url, fetch_options&& opt = {} ) { return fetch( url, PUT, std::move( opt ) ); }
	inline job<incoming_response> post( url_view url, fetch_options&& opt = {} ) { return fetch( url, POST, std::move( opt ) ); }
	inline job<incoming_response> head( url_view url, fetch_options&& opt = {} ) { return fetch( url, HEAD, std::move( opt ) ); }
};