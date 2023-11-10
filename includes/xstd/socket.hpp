#pragma once
#include "intrinsics.hpp"
#include "type_helpers.hpp"
#include "stream.hpp"
#include "vec_buffer.hpp"
#include "formatting.hpp"
#include "bitwise.hpp"
#include "time.hpp"
#include "spinlock.hpp"
#include "chore.hpp"
#include "text.hpp"
#include "task.hpp"
#include "fiber.hpp"
#include "async.hpp"

// Detect the underlying system libraries.
//
#ifndef XSTD_LWIP
	#if KERNEL_TARGET && __has_include(<lwip/sys.h>)
		#define XSTD_LWIP 1
	#else
		#define XSTD_LWIP 0
	#endif
#endif
#ifndef XSTD_BERKELEY
	#if __has_include(<sys/socket.h>)
		#define XSTD_BERKELEY 1
	#else
		#define XSTD_BERKELEY 0
	#endif
#endif
#ifndef XSTD_WINSOCK
	#if !KERNEL_TARGET && __has_include(<WinSock2.h>)
		#define XSTD_WINSOCK 1
	#else
		#define XSTD_WINSOCK 0
	#endif
#endif

// Include appropriately.
//
#if XSTD_LWIP
	#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS
	#include <lwip/sys.h>
	#include <lwip/tcp.h>
	#include <lwip/init.h>
	#include <lwip/dns.h>
	#include <lwip/timeouts.h>
	#undef XSTD_WINSOCK
	#undef XSTD_BERKELEY
#endif
#if XSTD_WINSOCK
	#define NOMINMAX
	#include <WinSock2.h>
	#include <WinDNS.h>
	#include <WS2tcpip.h>
	#pragma comment(lib, "ws2_32.lib")
	#include <thread>
	#undef XSTD_LWIP
	#undef XSTD_BERKELEY
#endif
#if XSTD_BERKELEY
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <poll.h>
	#include <netdb.h>
	#include <thread>
	#undef XSTD_LWIP
	#undef XSTD_WINSOCK
#endif


namespace xstd::net {
	// IPv4 address descriptor.
	//
	struct ipv4 {
		std::array<uint8_t, 4> value;

		// Null construction.
		//
		constexpr ipv4() : value{ 0, 0, 0, 0 } {}

		// Construction by value.
		//
		constexpr ipv4( std::array<uint8_t, 4> value ) : value( value ) {}
		constexpr ipv4( uint8_t a, uint8_t b, uint8_t c, uint8_t d ) : value{ a, b, c, d } {}
		constexpr ipv4( uint32_t value ) : value( xstd::bit_cast<std::array<uint8_t, 4>>( value ) ) {}
		constexpr ipv4( std::string_view str ) : ipv4( parse( str ) ) {}
		constexpr ipv4( const char* str ) : ipv4( parse( str ) ) {}

		// Copy, move and compare.
		//
		constexpr ipv4( ipv4&& ) noexcept = default;
		constexpr ipv4& operator=( ipv4&& ) noexcept = default;
		constexpr ipv4( const ipv4& ) noexcept = default;
		constexpr ipv4& operator=( const ipv4& ) noexcept = default;
		constexpr bool operator!=( const ipv4& o ) const { return to_integer() != o.to_integer(); }
		constexpr bool operator==( const ipv4& o ) const { return to_integer() == o.to_integer(); }
		constexpr bool operator<( const ipv4& o ) const { return to_integer() < o.to_integer(); }

		// String conversion.
		//
		constexpr uint32_t to_integer() const { return xstd::bit_cast<uint32_t>( value ); }
		constexpr std::array<uint8_t, 4> to_array() const { return value; }
		std::string to_string() const { return xstd::fmt::str( "%d.%d.%d.%d", value[ 0 ], value[ 1 ], value[ 2 ], value[ 3 ] ); }

		// Decay to bool for validation and to raw value.
		//
		constexpr operator uint32_t() const { return to_integer(); }
		constexpr operator std::array<uint8_t, 4>() const { return to_array(); }
		constexpr explicit operator bool() const { return to_integer() != 0; }

		// Parser from string.
		//
		static constexpr ipv4 parse( const char* str, size_t& count ) {
			// Result and the current segment.
			//
			uint32_t value = 0, part = 0;
			int shift = 32;
			
			// For each character:
			//
			const char* it = str;
			size_t limit = count;
			while ( size_t( it - str ) < limit ) {
				// If digit:
				//
				if ( '0' <= *it && *it <= '9' ) {
					// Check for overflow.
					//
					uint32_t chk1 = part << ( -shift & 31 );
					part *= 10;
					part += *it++ - '0';
					uint32_t chk2 = part << ( -shift & 31 );
					if ( chk2 < chk1 ) [[unlikely]] {
						value = part = 0;
						it = str;
						break;
					}
				}
				// If dot and we can add another segment:
				//
				else if ( *it == '.' && shift != 0 ) {
					// If accumulated value is more than a byte, fail.
					//
					if ( part > 0xff ) [[unlikely]] {
						value = part = 0;
						it = str;
						break;
					}
					value = part | ( value << 8 );
					part =  0;
					shift -= 8;
					it++;
				} 
				// Else break.
				//
				else {
					break;
				}
			}
			value <<= shift & 31;
			value |= part;
			count = it - str;
			return bswapd( value );
		}
		static constexpr ipv4 parse( const char* str, const size_t& count = std::dynamic_extent ) {
			size_t n = count;
			return parse( str, n );
		}
		static constexpr ipv4 parse( std::string_view& sv ) {
			size_t count = sv.size();
			ipv4 result = ipv4::parse( sv.data(), count );
			sv.remove_prefix( count );
			return result;
		}

#if XSTD_LWIP
		// Conversion to/from LWIP type.
		//
		constexpr ipv4( const ip_addr_t& lwip ) : ipv4( lwip.addr ) {}
		constexpr ipv4( const ip_addr_t* lwip ) : ipv4( lwip ? lwip->addr : 0 ) {}
		ip_addr_t* lwip() { return (ip_addr_t*) this; }
		const ip_addr_t* lwip() const { return (const ip_addr_t*) this; }
#endif
	};

	// DNS resolver types and the global hook.
	//
	struct dns_query_awaitable {
		// Input.
		//
		const char* hostname;
		bool no_hook;

		// Output.
		//
		mutable xstd::result<ipv4> result = {};
		std::coroutine_handle<> continuation = nullptr;

		bool await_ready() const { return result.success(); }
		bool await_suspend( std::coroutine_handle<> hnd );
		xstd::result<ipv4> await_resume() const { return std::move( result ); }
	};
	using fn_dns_hook = bool(*)( dns_query_awaitable*, std::coroutine_handle<> );
	inline fn_dns_hook g_dns_hook = nullptr;
	inline dns_query_awaitable query_dns_a( const char* hostname, bool no_hook = false ) {
		return { hostname, no_hook };
	}

	// Hostname resolver with fallback to DNS.
	//
	inline dns_query_awaitable resolve_hostname( const char* hostname ) {
		if ( '0' <= hostname[ 0 ] && hostname[ 0 ] <= '9' ) {
			size_t count = std::dynamic_extent;
			if ( auto ip = ipv4::parse( hostname, count ); ip && !hostname[ count ] ) {
				return dns_query_awaitable{ .result = ip };
			}
		}
		return query_dns_a( hostname );
	}

	// Standard options.
	//
	enum class socket_protocol {
		none,
		tcp,
	};
	struct socket_options {
		xstd::duration                conn_timeout =   5s;
		int                           listen_backlog = 128;
		xstd::duration                max_stall =      250ms;
		xstd::duration                linger =         30s;
		uint32_t                      recvbuf =        ( uint32_t ) 512_kb;
		uint32_t                      sendbuf =        ( uint32_t ) 512_kb;
		bool                          nodelay =        true;
		bool                          timestamps =     false;
		bool                          reuse =          true;
		std::optional<xstd::duration> keepalive =      std::nullopt;
	};

	// Definition via Berkeley/Winsock:
	//
#if XSTD_WINSOCK || XSTD_BERKELEY
	using socket_error = int;
#if XSTD_WINSOCK
	#define SOL_TCP IPPROTO_TCP
	using socket_t = uintptr_t;
	static constexpr socket_t invalid_socket = ~0ull;
#else
	using socket_t = int;
	static constexpr socket_t invalid_socket = -1;
#endif

	namespace detail {
		// Wrappers around functions differing between WSA / Berkeley standards.
		//
		static socket_error init_networking() {
#if XSTD_WINSOCK
			static WSADATA wsa;
			static socket_error status = ::WSAStartup( 0x0202, &wsa );
			if ( status )
				return status;
#endif
			return 0;
		}
		static socket_error close_socket( socket_t fd ) {
			if ( fd != invalid_socket ) {
#if XSTD_WINSOCK
				return ::closesocket( fd );
#else
				return ::close( fd );
#endif
			}
			return 0;
		}
		static socket_error get_last_error( socket_error value_or = {} ) {
			socket_error err;
#if XSTD_WINSOCK
			err = (socket_error) __readgsdword( 0x68 );
#else
			err = errno;
#endif
			return err ? err : value_or;
		}
		static socket_error set_blocking( socket_t fd, bool blocking ) {
#if XSTD_WINSOCK
			unsigned long ionbio = blocking ? 0 : 1;
			if ( socket_error err = ioctlsocket( fd, FIONBIO, &ionbio ); !err ) {
				return 0;
			} else {
				return get_last_error( err );
			}
#else
			int flags = fcntl( fd, F_GETFL, 0 );
			if ( flags != -1 ) {
				if ( blocking ) flags &= ~O_NONBLOCK;
				else            flags |=  O_NONBLOCK;
				return fcntl( fd, F_SETFL, flags );
			} else {
				return get_last_error( -1 );
			}
#endif
		}
		static socket_error create_socket( socket_t* out, int af, int type, std::optional<int> protocol = std::nullopt ) {
			socket_error err = 0;
#if XSTD_WINSOCK
			if ( !protocol ) {
				if ( type == SOCK_STREAM ) {
					protocol = IPPROTO_TCP;
				} else if ( type == SOCK_DGRAM ) {
					protocol = IPPROTO_UDP;
				} else {
					protocol = 0;
				}
			}
			socket_t fd = ::WSASocketW( af, type, *protocol, nullptr, 0, 0 );
			if ( fd == invalid_socket ) {
				err = get_last_error( -1 );
			}
#else
			if ( !protocol ) {
				protocol = 0;
			}
			socket_t fd = ::socket( af, type, *protocol );
			if ( fd == invalid_socket ) {
				err = get_last_error( -1 );
			}
#endif
			if ( err ) {
				close_socket( fd );
				fd = invalid_socket;
			}
			*out = fd;
			return err;
		}
		static timeval to_timeval( xstd::duration dur ) {
			int64_t usec = dur / 1us;
			timeval tv = {};
			tv.tv_sec = (int32_t) std::clamp<int64_t>( usec / int64_t( 1s / 1us ), INT32_MIN, INT32_MAX );
			tv.tv_usec = (int32_t) std::clamp<int64_t>( usec % int64_t( 1s / 1us ), INT32_MIN, INT32_MAX );
			return tv;
		}
		static xstd::duration from_timeval( const timeval& dur ) {
			return xstd::duration( 1s ) * dur.tv_sec + xstd::duration( 1us ) * dur.tv_usec;
		}
	};

	// Implement DNS resolution.
	//
	inline bool dns_query_awaitable::await_suspend( std::coroutine_handle<> hnd ) {
		if ( !no_hook && g_dns_hook ) {
			return g_dns_hook( this, hnd );
		}

		// Initialize network stack.
		//
		if ( auto err = detail::init_networking() ) {
			this->result.raise( xstd::exception{ XSTD_ESTR( "failed to init net: %d" ), err } );
			return false;
		}

		// Create the hints.
		//
		addrinfo hints = {};
		memset( &hints, 0, sizeof( addrinfo ) );
		hints.ai_family =   AF_INET;
		hints.ai_socktype = SOCK_STREAM;
#if XSTD_WINSOCK
		hints.ai_flags =   AI_DNS_ONLY | AI_BYPASS_DNS_CACHE;
#endif

		// Parse the result.
		//
		addrinfo* result = nullptr;
		if ( auto err = getaddrinfo( hostname, nullptr, &hints, &result ) ) {
			this->result.raise( xstd::exception{ XSTD_ESTR( "failed to query dns: %d" ), err } );
			return false;
		}
		ipv4 ip = {};
		for ( auto i = result; i; i = i->ai_next ) {
			if ( i->ai_addr->sa_family == AF_INET ) {
				struct sockaddr_in* p = (struct sockaddr_in*) i->ai_addr;
				ip = p->sin_addr.s_addr;
				if ( ip ) break;
			}
		}
		freeaddrinfo( result );
		this->result.emplace( ip );
		return false;
	}

	// Base of the stream.
	//
	struct socket : duplex {
	protected:
		// Options.
		//
		socket_options  opt = {};
		socket_protocol proto = {};

		// File descriptor and remote connection details.
		//
		socket_t        fd = invalid_socket;
		ipv4            address = {};
		uint16_t        port = 0;

		// Constructed by initial details.
		//
		socket( socket_protocol proto, ipv4 address, uint16_t port, socket_options opt )
			: opt( opt ), proto( proto ), address( address ), port( port ) {}

		// Wrappers.
		//
	public:
		socket_error set_socket_opt( int level, int name, const void* data, size_t len ) {
			socket_error status = ::setsockopt( fd, level, name, (const char*) data, (int) len );
#if XSTD_WINSOCK
			status = status == -1 ? detail::get_last_error( status ) : 0;
#endif
			return status;
		}
		socket_error get_socket_opt( int level, int name, void* data, size_t* len ) {
			int rlen = (int) *len;
			socket_error status = ::getsockopt( this->fd, level, name, (char*) data, &rlen );
#if XSTD_WINSOCK
			status = status == -1 ? detail::get_last_error( status ) : 0;
#endif
			if ( !status ) *len = (size_t) std::max( 0, rlen );
			return status;
		}
		template<typename T>
		socket_error set_socket_opt( int level, int name, const T& val ) {
			if constexpr ( std::is_same_v<T, xstd::duration> ) {
				timeval tv = detail::to_timeval( val );
				return set_socket_opt( level, name, &tv, sizeof( timeval ) );
			} else {
				return set_socket_opt( level, name, &val, sizeof( T ) );
			}
		}
		template<typename T>
		socket_error get_socket_opt( int level, int name, T& val ) {
			if constexpr ( std::is_same_v<T, xstd::duration> ) {
				timeval tv = detail::to_timeval( val );
				size_t  sz = sizeof( timeval );
				socket_error err = get_socket_opt( level, name, &tv, &sz );
				val = detail::from_timeval( tv );
				return err;
			} else {
				size_t  sz = sizeof( T );
				return get_socket_opt( level, name, &val, &sz );
			}
		}
		template<typename T>
		socket_error try_set_socket_opt( int level, int name, T& val ) {
			socket_error err = set_socket_opt<T>( level, name, val );
			if ( err != 0 ) {
				get_socket_opt<T>( level, name, val );
			}
			return err;
		}
		socket_error get_socket_error() {
			socket_error error = 0;
			socket_error opt_error = get_socket_opt( SOL_SOCKET, SO_ERROR, error );
			return error ? error : opt_error;
		}
		std::optional<socket_error> socket_connect() {
			sockaddr_in addr;
			memset( &addr, 0, sizeof( sockaddr_in ) );
			addr.sin_family = AF_INET;
			addr.sin_port = bswap( this->port );
			addr.sin_addr.s_addr = this->address;
#if XSTD_WINSOCK
			if ( ::WSAConnect( this->fd, (const sockaddr*) &addr, sizeof( addr ), nullptr, nullptr, nullptr, nullptr ) == -1 ) {
				if ( auto err = detail::get_last_error(); err == WSAEWOULDBLOCK ) {
					return std::nullopt;
				} else {
					return err;
				}
			}
#else
			if ( ::connect( this->fd, (const sockaddr*) &addr, sizeof( addr ) ) == -1 ) {
				if ( auto err = detail::get_last_error(); err == EINPROGRESS ) {
					return std::nullopt;
				} else {
					return err;
				}
			}
#endif
			return 0;
		}
		socket_error socket_bind() {
			sockaddr_in addr;
			memset( &addr, 0, sizeof( sockaddr_in ) );
			addr.sin_family = AF_INET;
			addr.sin_port = bswap( this->port );
			addr.sin_addr.s_addr = this->address;
#if XSTD_WINSOCK
			if ( ::bind( this->fd, (const sockaddr*) &addr, sizeof( addr ) ) == -1 ) {
				return detail::get_last_error( -1 );
			}
#else
			if ( ::bind( this->fd, (const sockaddr*) &addr, sizeof( addr ) ) == -1 ) {
				return detail::get_last_error( -1 );
			}
#endif
			return 0;
		}
		socket_error socket_poll( pollfd& desc, duration timeout ) {
#if XSTD_WINSOCK
			if ( ::WSAPoll( &desc, 1, timeout / 1ms ) == -1 ) {
				return detail::get_last_error( -1 );
			}
#else
			if ( ::poll( &desc, 1, timeout / 1ms ) == -1 ) {
				return detail::get_last_error( -1 );
			}
#endif
			return 0;
		}
		socket_error socket_listen() {
			if ( ::listen( this->fd, opt.listen_backlog ) == -1 ) {
				return detail::get_last_error( -1 );
			}
			return 0;
		}
		std::tuple<socket_t, ipv4, uint16_t> socket_accept() {
			sockaddr_in addr;
			memset( &addr, 0, sizeof( sockaddr_in ) );
			int len = sizeof( addr );
			socket_t sock;
#if XSTD_WINSOCK
			sock = ::WSAAccept( this->fd, (sockaddr*) &addr, &len, nullptr, 0 );
#else
			sock = ::accept( this->fd, (sockaddr*) &addr, &len );
#endif
			return { sock, addr.sin_addr.s_addr, bswap( addr.sin_port ) };
		}
		bool socket_receive( std::span<uint8_t>& buffer, int flags = 0 ) {
			bool need_poll = false;
#if XSTD_WINSOCK
			if ( buffer.size() > UINT32_MAX )
				buffer = buffer.subspan( 0, (size_t) UINT32_MAX );
			DWORD  count = 0;
			DWORD  ioflags = (DWORD) flags;
			WSABUF bufdesc = { (uint32_t) buffer.size(), (char*) buffer.data() };
			if ( socket_error result = ::WSARecv( this->fd, &bufdesc, 1, &count, &ioflags, nullptr, nullptr ); result == -1 ) [[unlikely]] {
				if ( auto e = detail::get_last_error( result ); e != WSAEINTR && e != WSAEINPROGRESS && e != WSAEWOULDBLOCK && e != WSAEMSGSIZE ) {
					this->raise_error( XSTD_ESTR( "socket error: %d" ), e );
				}
				need_poll = true;
			}
			buffer = buffer.subspan( 0, count );
#else
			if ( buffer.size() > INT32_MAX )
				buffer = buffer.subspan( 0, (size_t) INT32_MAX );
			if ( socket_error result = ::recv( this->fd, (char*) buffer.data(), (int) buffer.size(), flags ); result == -1 ) [[unlikely]] {
				if ( auto e = detail::get_last_error( result ); e != EINTR && e != EINPROGRESS && e != EAGAIN && e != EWOULDBLOCK ) {
					this->raise_error( XSTD_ESTR( "socket error: %d" ), e );
				}
				need_poll = true;
			} else {
				buffer = buffer.subspan( 0, (uint32_t) result );
			}
#endif
			return need_poll;
		}
		bool socket_send( std::span<const uint8_t>& buffer, int flags = 0 ) {
			uint32_t write_count = buffer.size() > opt.sendbuf ? opt.sendbuf : uint32_t( buffer.size() );
			uint32_t count = 0;
			bool need_poll = false;
#if XSTD_WINSOCK
			WSABUF bufdesc = { write_count, (char*) buffer.data() };
			if ( socket_error result = ::WSASend( this->fd, &bufdesc, 1, (DWORD*) &count, (DWORD) flags, nullptr, nullptr ); result == -1 ) [[unlikely]] {
				if ( auto e = detail::get_last_error( result ); e != WSAEINTR && e != WSAEINPROGRESS && e != WSAEWOULDBLOCK && e != WSAEMSGSIZE ) {
					this->raise_error( XSTD_ESTR( "socket error: %d" ), e );
				}
				need_poll = true;
			}
#else
			if ( socket_error result = ::send( this->fd, (char*) buffer.data(), (int)write_count, flags ); result == -1 ) [[unlikely]] {
				if ( auto e = detail::get_last_error( result ); e != EINTR && e != EINPROGRESS && e != EAGAIN && e != EWOULDBLOCK ) {
					this->raise_error( XSTD_ESTR( "socket error: %d" ), e );
				}
				if ( e == EWOULDBLOCK || e == EAGAIN )
					count = write_count;
				need_poll = true;
			} else {
				count = (uint32_t) result;
			}
#endif
			buffer = buffer.subspan( count );
			return need_poll;
		}

		// Remote/local address.
		//
		std::pair<ipv4, uint16_t> get_remote_address() {
			return { address, port };
		}
		std::pair<ipv4, uint16_t> get_local_address() {
			sockaddr_in addr;
			memset( &addr, 0, sizeof( sockaddr_in ) );
			addr.sin_family = AF_INET;
			int len = sizeof( sockaddr_in );
			getsockname( this->fd, (sockaddr*) &addr, &len );
			return { addr.sin_addr.s_addr, bswap( addr.sin_port ) };
		}

		// Configuration.
		//
		void socket_reconfig_tcp() {
			this->try_set_socket_opt( SOL_TCP, TCP_NODELAY, opt.nodelay );
			this->try_set_socket_opt( SOL_TCP, TCP_TIMESTAMPS, opt.timestamps );
			if ( opt.keepalive ) {
				this->set_socket_opt( SOL_TCP, TCP_KEEPIDLE, uint32_t( *opt.keepalive / 1s ) );
			}
		}
		void socket_reconfig() {
			this->try_set_socket_opt( SOL_SOCKET, SO_REUSEADDR, opt.reuse );
			this->try_set_socket_opt( SOL_SOCKET, SO_LINGER, opt.linger );
			this->try_set_socket_opt( SOL_SOCKET, SO_SNDBUF, opt.sendbuf );
			this->try_set_socket_opt( SOL_SOCKET, SO_RCVBUF, opt.recvbuf );
			if ( opt.keepalive ) {
				this->set_socket_opt( SOL_SOCKET, SO_KEEPALIVE, true );
			} else {
				this->set_socket_opt( SOL_SOCKET, SO_KEEPALIVE, false );
			}
			if ( proto == socket_protocol::tcp )
				socket_reconfig_tcp();
		}
		int socket_shutdown( bool r, bool w ) {
			if ( !r && !w ) return 0;
#if XSTD_WINSOCK
			int code = r ? SD_RECEIVE : ( w ? SD_SEND : SD_BOTH );
#else
			int code = r ? SHUT_RD : ( w ? SHUT_WR : SHUT_RDWR );
#endif
			if ( ::shutdown( this->fd, code ) == -1 ) {
				return detail::get_last_error( -1 );
			} else {
				return 0;
			}
		}
		void set_options( socket_options opt ) {
			this->opt = opt;
			if ( this->fd != invalid_socket ) {
				socket_reconfig();
			}
		}
		const socket_options& get_options() {
			return this->opt;
		}
		socket_t get_fd() const {
			return fd;
		}
		void set_fd( socket_t sock ) {
			detail::close_socket( std::exchange( fd, sock ) );
			if ( sock != invalid_socket )
				socket_reconfig();
		}
		void close() {
			set_fd( invalid_socket );
		}

		// Close the socket on destruction.
		//
		~socket() {
			close();
		}

	protected:
		void raise_error( const char* fmt, socket_error e = detail::get_last_error( -1 ) ) {
			this->stop( stream_stop_error, exception{ fmt, e } );
		}
		bool assert_status( const char* fmt, socket_error e ) {
			if ( !e ) [[likely]] {
				return true;
			}
			raise_error( fmt, e );
			return false;
		}
	};

	// TCP implementation.
	//
	struct tcp : socket {
		tcp( ipv4 address, uint16_t port, socket_options opts = {}, socket_t client_socket = invalid_socket ) : socket( socket_protocol::tcp, address, port, opts ) {
			// Initialize networking and get a socket.
			//
			const bool is_connect = client_socket == invalid_socket;
			if ( is_connect ) {
				if ( !this->assert_status( XSTD_ESTR( "failed to initialize networking: %d" ), detail::init_networking() ) )
					return;
				if ( !this->assert_status( XSTD_ESTR( "failed to create a socket: %d" ), detail::create_socket( &client_socket, AF_INET, SOCK_STREAM ) ) )
					return;
			}
			set_fd( client_socket );

			// Start the initial fibers.
			//
			state().attach( std::mem_fn( &tcp::init_thread ), this, is_connect );
		}
		~tcp() {
			close();
			stop( stream_stop_killed, XSTD_ESTR( "dropped" ) );
		}

	protected:
		fiber send_more() {
			// Allocate the buffer and get the controller.
			//
			const size_t buffer_length = opt.sendbuf;
			const auto   buffer =        std::make_unique_for_overwrite<uint8_t[]>( buffer_length );
			auto ctrl =                  this->controller();

			// Enter the loop once socket is ready.
			//
			co_yield fiber::pause;
			while ( true ) {
				// Wait for the user to request a send.
				//
				size_t count = co_await ctrl.read_into( { &buffer[ 0 ], buffer_length }, 1 );
				if ( !count ) {
					co_yield fiber::heartbeat;
					if ( ctrl.is_shutting_down() ) {
						this->socket_shutdown( false, true );
					}
					co_return;
				}

				// Ask the socket to write the data.
				//
				std::span<const uint8_t> request{ &buffer[ 0 ], count };
				while ( !request.empty() ) {
					bool need_poll = this->socket_send( request );
					co_yield need_poll ? fiber::pause : fiber::heartbeat;
				}
			}
		}
		fiber recv_more() {
			// Allocate the buffer and get the controller.
			//
			const size_t buffer_length = opt.recvbuf;
			const auto   buffer =        std::make_unique_for_overwrite<uint8_t[]>( buffer_length );
			auto ctrl =                  this->controller();

			// Enter the loop.
			//
			bool need_poll = true;
			while ( true ) {
				co_yield need_poll ? fiber::pause : fiber::heartbeat;

				// Read from the socket, terminate on error.
				//
				std::span range{ &buffer[ 0 ], buffer_length };
				need_poll = this->socket_receive( range );
				co_yield fiber::heartbeat;

				// If we received a FIN, try shutting down.
				//
				if ( range.empty() ) {
					if ( !need_poll ) {
						ctrl.shutdown();
						co_return;
					}
				}
				// Propagate the data.
				//
				else {
					need_poll = false;
					co_await ctrl.write( range );
				}
			}
		}

		// Connection and writer thread. 
		//
		fiber init_thread( bool connect ) {
			fiber fib_sender;
			fiber fib_receiver;

			// First we have to wait for the connection, set the socket non-blocking.
			//
			if ( auto err = detail::set_blocking( fd, false ) ) {
				co_return this->raise_error( XSTD_ESTR( "failed to change socket mode: %d" ), err );
			}
			co_await yield{};

			// Create the fixed poll.
			//
			pollfd poll = { .fd = fd, .events = 0, .revents = 0 };
			exception exc;
			auto poll_for = [ & ]( short evt ) FORCE_INLINE{
				poll.events = evt;
				if ( auto err = socket_poll( poll, opt.max_stall ) ) {
					exc = { XSTD_ESTR( "poll error: %d" ), err };
					poll.revents |= POLLERR;
				}
				short ret = poll.revents & ( evt | POLLERR );
				poll.revents ^= ret;
				return ret;
			};

			// Initialization logic:
			//
			if ( connect ) {
				// Start the connection.
				//
				if ( auto err = this->socket_connect(); err.has_value() ) {
					co_return this->raise_error( XSTD_ESTR( "connection failed: %d" ), *err );
				}

				// Poll until socket is writable.
				//
				auto timeout = time::now() + opt.conn_timeout;
				while ( true ) {
					short events = poll_for( POLLOUT );
					co_yield fiber::heartbeat;
					if ( events & POLLERR ) {
						stop( exc ? exc : exception{ XSTD_ESTR( "socket error: %d" ), get_socket_error() } );
						co_return;
					} else if ( timeout < time::now() ) {
						stop( stream_stop_timeout, XSTD_ESTR( "connection timed out" ) );
						co_return;
					} else if ( events & POLLOUT ) {
						break;
					} else {
						co_await yield{};
					}
				}
			}
			poll.revents |= POLLOUT;

			// Create the data fibers.
			//
			fib_receiver = recv_more();
			fib_sender = send_more();

			// Enter long-poll.
			//
			while ( true ) {
				short events = poll_for( POLLOUT | POLLIN );
				co_yield fiber::heartbeat;
				if ( events & POLLERR ) {
					stop( exc ? exc : exception{ XSTD_ESTR( "socket error: %d" ), get_socket_error() } );
					co_return;
				} else if ( events & POLLIN ) {
					fib_receiver.resume();
				} else if ( events & POLLOUT ) {
					fib_sender.resume();
				} else {
					co_await yield{};
				}
			}

		}
	};
	struct tcp_listening : socket {
		tcp_listening( ipv4 address, uint16_t port, socket_options opts = {} ) : socket( socket_protocol::tcp, address, port, opts ) {
			// Initialize networking and get a socket.
			//
			if ( !this->assert_status( XSTD_ESTR( "failed to initialize networking: %d" ), detail::init_networking() ) )
				return;
			socket_t listen_socket = invalid_socket;
			if ( !this->assert_status( XSTD_ESTR( "failed to create a socket: %d" ), detail::create_socket( &listen_socket, AF_INET, SOCK_STREAM ) ) )
				return;
			set_fd( listen_socket );

			// Bind the socket.
			//
			if ( !this->assert_status( XSTD_ESTR( "failed to bind socket: %d" ), this->socket_bind() ) )
				return;
		}
		tcp_listening( uint16_t port, socket_options opts = {} ) : tcp_listening( ipv4{}, port, opts ) {}

		template<typename F>
		fiber listen( F&& callback ) {
			if ( !this->assert_status( XSTD_ESTR( "failed to listen socket: %d" ), this->socket_listen() ) )
				return nullptr;
			return accept_thread( std::forward<F>( callback ) );
		}

		~tcp_listening() {
			close();
			stop( stream_stop_killed, XSTD_ESTR( "dropped" ) );
		}

	protected:
		template<typename F>
		fiber accept_thread( F callback ) {
			// Create FD watch.
			//
			fd_set fd_er, fd_wr;
			while ( true ) {
				FD_ZERO( &fd_er );    FD_ZERO( &fd_wr );
				FD_SET( fd, &fd_er ); FD_SET( fd, &fd_wr );

				timeval stall_timeout = detail::to_timeval( opt.max_stall );
				if ( select( int( fd ) + 1, &fd_wr, &fd_wr, &fd_er, &stall_timeout ) < 0 ) {
					this->stop( stream_stop_timeout, XSTD_ESTR( "accept wait error" ) );
					co_return;
				}
				if ( stopped() ) {
					co_return;
				}
				
				if ( FD_ISSET( fd, &fd_er ) ) {
					co_return raise_error( XSTD_ESTR( "socket error: %d" ), get_socket_error() );
				}

				if ( FD_ISSET( fd, &fd_wr ) ) {
					auto [sock, ip, port] = this->socket_accept();
					if ( !sock ) {
						if ( auto err = get_socket_error() ) {
							co_return raise_error( XSTD_ESTR( "socket error: %d" ), get_socket_error() );
						}
					} else {
						callback( std::make_unique<tcp>( ip, port, opt, sock ) );
					}
				}
			}
		}
	};

#endif

	// Definition via LWIP:
	//
#if XSTD_LWIP
	using socket_t =     tcp_pcb*;
	using socket_error = err_t;
	static constexpr socket_t invalid_socket = nullptr;

	// Core lock.
	//
	alignas( 64 ) inline recursive_xspinlock<> core_lock = {};
	
	// Implement DNS resolution.
	//
	inline bool dns_query_awaitable::await_suspend( std::coroutine_handle<> hnd ) {
		if ( !no_hook && g_dns_hook ) {
			return g_dns_hook( this, hnd );
		}
		std::unique_lock lock{ core_lock };
		err_t err = dns_gethostbyname_addrtype( this->hostname, this->result.emplace().lwip(), []( const char*, const ip_addr_t* ipaddr, void* callback_arg ) {
			auto* ctx = ( (dns_query_awaitable*) callback_arg );
			ctx->result.emplace( ipaddr );
			if ( ctx->continuation )
				xstd::chore( ctx->continuation );
		}, this, LWIP_DNS_ADDRTYPE_IPV4 );

		if ( err == ERR_INPROGRESS ) {
			this->continuation = hnd;
			return true;
		} else if ( err ) {
			this->result.raise( xstd::exception{ XSTD_ESTR( "failed to query dns: %d" ), err } );
			return false;
		} else {
			return false;
		}
	}

	// Base of the stream.
	//
	struct socket : duplex {
	protected:
		// Options.
		//
		socket_options  opt = {};
		socket_protocol proto = {};

		// File descriptor and remote connection details.
		//
		socket_t        pcb = invalid_socket;
		ipv4            address = {};
		uint16_t        port = 0;

		// Constructed by initial details.
		//
		socket( socket_protocol proto, ipv4 address, uint16_t port, socket_options opt )
			: opt( opt ), proto( proto ), address( address ), port( port ) {}

		// Wrappers.
		//
	public:
		socket_error socket_connect() {
			std::lock_guard _g{ core_lock };
			return tcp_connect( pcb, address.lwip(), port, []( void* _arg, socket_t pcb, socket_error err ) -> socket_error {
				if ( auto arg = (socket*) _arg ) {
					arg->on_state( pcb, err );
					return arg->get_pcb() ? ERR_OK : ERR_ABRT;
				}
				return ERR_OK;
			} );
		}
		socket_error socket_bind() {
			std::lock_guard _g{ core_lock };
			return tcp_bind( pcb, address.lwip(), port );
		}
		socket_error socket_listen() {
			std::lock_guard _g{ core_lock };
			socket_error err = 0;
			if ( auto new_fd = tcp_listen_with_backlog_and_err( pcb, (uint8_t) std::min( opt.listen_backlog, 0xff ), &err ) ) {
				if ( err ) {
					tcp_abort( new_fd );
				} else {
					pcb = new_fd;
				}
			}
			return err;
		}
		bool socket_send( std::span<const uint8_t>& buffer, int flags = TCP_WRITE_FLAG_COPY ) {
			std::lock_guard _g{ core_lock };
			const uint8_t* data = buffer.data();
			size_t         length = buffer.size();

			size_t  offset = 0;
			bool    exhausted = false;
			while ( !exhausted ) {
				// Calculate the maximum fragment length.
				//
				size_t  frag = length - offset;
				frag = std::min<size_t>( frag, 0xFFFF );
				if ( !frag ) break;
				
				// Try writing in increasingly smaller sizes.
				//
				err_t status;
				while ( true ) {
					status = tcp_write( pcb, (char*) data + offset, (uint16_t) frag, flags );
					if ( status != ERR_MEM ) {
						break;
					} else {
						exhausted = true;
						if ( ( tcp_sndbuf( pcb ) == 0 ) || ( tcp_sndqueuelen( pcb ) >= TCP_SND_QUEUELEN ) ) {
							frag = 0;
							status = ERR_OK;
							break;
						} else {
							frag >>= 1;
							if ( frag < 256 ) {
								frag = 0;
								status = ERR_OK;
								break;
							}
						}
					}
				}

				// If error, fail.
				//
				if ( status != ERR_OK ) {
					raise_error( XSTD_ESTR( "socket write error: %d" ), status );
					break;
				} 
				// Else, increment offset.
				//
				else {
					offset += frag;
				}
			}
			
			tcp_output( pcb );
			buffer = buffer.subspan( offset );
			return false;
		}
		bool socket_send( vec_buffer& buffer, int flags = 0 ) {
			std::span<const uint8_t> span{ buffer };
			bool res = socket_send( span, flags );
			buffer.shrink_resize_reverse( span.size() );
			return res;
		}

		// Remote/local address.
		//
		std::pair<ipv4, uint16_t> get_remote_address() {
			return { pcb ? pcb->remote_ip : address, pcb ? pcb->remote_port : port };
		}
		std::pair<ipv4, uint16_t> get_local_address() {
			return { pcb ? pcb->local_ip : ipv4{}, pcb ? pcb->local_port : 0 };
		}

		// Callbacks.
		//
		virtual void on_sent( socket_t pcb, size_t len ) {}
		virtual void on_recv( socket_t pcb, pbuf* p ) {}
		virtual void on_poll( socket_t pcb ) {}
		virtual bool on_accept( socket_t pcb ) { return false; }
		virtual void on_state( socket_t pcb, socket_error e ) {}

		// Configuration.
		//
		void socket_reconfig() {
			std::lock_guard _g{ core_lock };
			if ( pcb != invalid_socket ) {
				if ( opt.reuse ) {
					ip_set_option( pcb, SOF_REUSEADDR );
				} else {
					ip_reset_option( pcb, SOF_REUSEADDR );
				}
				if ( opt.nodelay ) {
					tcp_set_flags( pcb, TF_NODELAY );
				} else {
					tcp_clear_flags( pcb, TF_NODELAY );
				}
				opt.timestamps = LWIP_TCP_TIMESTAMPS;

				if ( opt.keepalive ) {
					pcb->keep_idle = (uint32_t) std::clamp<int64_t>( *opt.keepalive / 1ms, 0, UINT32_MAX );
					ip_set_option( pcb, SOF_KEEPALIVE );
				} else {
					ip_reset_option( pcb, SOF_KEEPALIVE );
				}
				opt.sendbuf = TCP_SND_BUF;
				// TODO: linger
			
				// Bind the callbacks.
				//
				pcb->callback_arg = this;
				tcp_arg( pcb, this );
				if ( pcb->state == LISTEN ) {
					tcp_accept( pcb, []( void* _arg, socket_t pcb, socket_error err ) -> socket_error {
						if ( err ) return err;
						if ( auto arg = (socket*) _arg ) {
							if ( !arg->on_accept( pcb ) ) {
								return ERR_MEM;
							}
							return arg->get_pcb() ? ERR_OK : ERR_ABRT;
						}
						tcp_abort( pcb );
						return ERR_ABRT;
					} );
				} else {
					tcp_sent( pcb, []( void* _arg, socket_t pcb, uint16_t len ) -> socket_error {
						if ( auto arg = (socket*) _arg ) {
							arg->on_sent( pcb, len );
							return arg->get_pcb() ? ERR_OK : ERR_ABRT;
						}
						return ERR_OK;
					} );
					tcp_recv( pcb, []( void* _arg, socket_t pcb, pbuf* p, socket_error err ) -> socket_error {
						if ( err ) return err;
						if ( auto arg = (socket*) _arg ) {
							arg->on_recv( pcb, p );
							if ( !arg->get_pcb() ) {
								return ERR_ABRT;
							}
							tcp_recved( pcb, p->tot_len );
							pbuf_free( p );
							return ERR_OK;
						}

						if ( tcp_close( pcb ) ) {
							tcp_abort( pcb );
						}
						return ERR_ABRT;
					} );
					tcp_err( pcb, []( void* _arg, socket_error err ) {
						if ( auto arg = (socket*) _arg ) {
							err = err ? err : ERR_CLSD;
							arg->close( true );
							arg->raise_error( XSTD_ESTR( "socket error: %d" ), err );
							arg->on_state( nullptr, err );
						}
					} );
					tcp_poll( pcb, []( void* _arg, socket_t pcb ) -> socket_error {
						if ( auto arg = (socket*) _arg ) {
							arg->on_poll( pcb );
							return arg->get_pcb() ? ERR_OK : ERR_ABRT;
						}
						if ( tcp_close( pcb ) ) {
							tcp_abort( pcb );
						}
						return ERR_ABRT;
					}, 1 );
				}
			}
		}
		socket_error socket_shutdown( bool r, bool w ) {
			if ( !r && !w || pcb == invalid_socket ) return 0;
			std::lock_guard _g{ core_lock };
			socket_error err = tcp_shutdown( pcb, (int) r, (int) w );
			if ( !err && r && w )
				pcb = invalid_socket;
			return err;
		}
		void set_options( socket_options opt ) {
			std::lock_guard _g{ core_lock };
			this->opt = opt;
			socket_reconfig();
		}
		const socket_options& get_options() {
			return this->opt;
		}
		socket_t get_pcb() const {
			return pcb;
		}
		void set_pcb( socket_t sock, bool freed = false ) {
			std::lock_guard _g{ core_lock };
			if ( auto prev = std::exchange( pcb, sock ); prev != invalid_socket && !freed ) {
				tcp_arg( prev, nullptr );
				if ( tcp_close( prev ) )
					tcp_abort( prev );
			}
			if ( sock != invalid_socket )
				socket_reconfig();
		}
		void close( bool freed = false ) {
			set_pcb( invalid_socket, freed );
		}

		// Close the socket on destruction.
		//
		void raise_error( const char* fmt, socket_error e ) {
			this->stop( stream_stop_error, exception{ fmt, e } );
		}
		bool assert_status( const char* fmt, socket_error e ) {
			if ( !e ) [[likely]] {
				return true;
			}
			raise_error( fmt, e );
			return false;
		}
		~socket() {
			close();
		}
	};


	// TCP implementation.
	//
	struct tcp : socket {
		// Time it was opened or the timeout limit.
		//
		timestamp open_time = {};

		tcp( ipv4 address, uint16_t port, socket_options opts = {}, socket_t client_socket = invalid_socket ) : socket( socket_protocol::tcp, address, port, opts ) {
			std::unique_lock lock{ core_lock };

			// Initialize networking and get a socket.
			//
			const bool is_connect = client_socket == invalid_socket;
			if ( is_connect ) {
				client_socket = tcp_new();
				if ( !client_socket ) {
					lock.unlock();
					this->stop( XSTD_ESTR( "failed to create a socket" ) );
					return;
				}
			}
			set_pcb( client_socket );

			// Start the initial fibers.
			//
			thr_sender = init_thread( is_connect );
		}
		~tcp() {
			close();
			stop( stream_stop_killed, XSTD_ESTR( "dropped" ) );
		}

		void on_sent( socket_t pcb, size_t len ) override {
			tcp_output( pcb ); // needed?
			sender_retry.signal_async();
		}
		void on_recv( socket_t pcb, pbuf* p ) override {
			// If we received a FIN, stop.
			//
			if ( !p ) {
				req_shutdown = true;
			}
			// Copy the pbuf into the buffer.
			//
			else {
				async_buffer_locked buffer{ controller().writable() };
				auto* dst = buffer->push( p->tot_len );
				for ( auto it = p; it; it = it->next ) {
					memcpy( dst, it->payload, it->len );
					dst += it->len;
				}
			}


			// Signal recv.
			//
			if ( !req_recv++ ) {
				thr_recv = recv_thread();
			}
		}
		void on_poll( socket_t pcb ) override {
			if ( pcb->state == SYN_SENT && open_time > time::now() ) {
				stop( XSTD_ESTR( "connection timed out" ) );
			} else if ( tcp_sndbuf( pcb ) ) {
				sender_retry.signal_async();
			}
		}
		bool on_accept( socket_t pcb ) override { return false; }
		void on_state( socket_t pcb, socket_error e ) override {
			if ( pcb ) {
				open_time = time::now();
			}
			if ( auto s = sender_retry.signal() ) {
				s();
			}
		}


		std::atomic<uint32_t> req_recv = 0;
		std::atomic<bool> req_shutdown = false;
		fiber thr_recv;
		fiber recv_thread() {
			auto ctrl = controller();
			uint32_t n = req_recv.load( std::memory_order::relaxed );
			while ( n ) {
				co_await ctrl.flush();
				n = ( req_recv -= n );
			}

			if ( req_shutdown ) {
				ctrl.shutdown();
			}
		}

		struct send_retry_t {
			void* continuation = nullptr;
			
			FORCE_INLINE bool await_ready() noexcept { return false; }
			FORCE_INLINE void await_suspend( coroutine_handle<> handle ) noexcept {  continuation = handle.address(); }
			FORCE_INLINE void await_resume() const noexcept {}

			coroutine_handle<> signal() {
				if ( continuation ) {
					if ( auto c = std::atomic_ref{ continuation }.exchange( nullptr ) ) {
						return coroutine_handle<>::from_address( c );
					}
				}
				return {};
			}
			void signal_async() {
				if ( auto h = signal() )
					chore( std::move( h ) );
			}
			~send_retry_t() {
				if ( auto h = signal() )
					h();
			}
		} sender_retry;


	protected:
		fiber thr_sender;
		fiber thr_receiver;

		// Connection and writer thread. 
		//
		fiber init_thread( bool connect ) {
			// Initialization logic:
			//
			if ( connect ) {
				// Start the connection.
				//
				if ( auto err = this->socket_connect() ) {
					co_return this->raise_error( XSTD_ESTR( "connection failed: %d" ), err );
				}
			}

			// Start the receiver thread.
			//
			thr_receiver = recv_thread();

			// Sender logic:
			//
			{
				auto ctrl = this->controller();
				while ( pcb ) {
					// Wait for the user to request a send.
					//
					auto request = co_await ctrl.read();
					if ( !request ) {
						if ( ctrl.is_shutting_down() ) {
							this->socket_shutdown( false, true );
						}
						co_return;
					}

					// Wait for connection.
					//
					while ( pcb && pcb->state < ESTABLISHED ) {
						co_await this->sender_retry;
						if ( this->stopped() )
							co_return;
					}

					// Write the data.
					//
					while ( !request.empty() ) {
						this->socket_send( request );
						if ( this->stopped() )
							co_return;
						if ( request )
							co_await this->sender_retry;
						if ( !pcb ) 
							co_return;
					}
				}
			}
		}
	};
#endif
};