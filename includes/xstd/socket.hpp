#pragma once
#include "intrinsics.hpp"
#include "type_helpers.hpp"
#include "duplex.hpp"
#include "vec_buffer.hpp"
#include "formatting.hpp"
#include "bitwise.hpp"
#include "time.hpp"
#include "spinlock.hpp"
#include "chore.hpp"
#include "task.hpp"

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

		// Default copy and compare.
		//
		constexpr ipv4( const ipv4& ) = default;
		constexpr ipv4& operator=( const ipv4& ) = default;
		constexpr auto operator<=>( const ipv4& ) const = default;

		// String conversion.
		//
		std::string to_string() const { return xstd::fmt::str( "%d.%d.%d.%d", value[ 0 ], value[ 1 ], value[ 2 ], value[ 3 ] ); }

		// Decay to bool for validation and to raw value.
		//
		constexpr operator std::array<uint8_t, 4>() const { return value; }
		constexpr operator uint32_t() const { return xstd::bit_cast<uint32_t>( value ); }
		constexpr explicit operator bool() const { return xstd::bit_cast<uint32_t>( value ) != 0; }

#if XSTD_LWIP
		// Conversion to/from LWIP type.
		//
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

		bool await_ready() const { return false; }
		bool await_suspend( std::coroutine_handle<> hnd );
		xstd::result<ipv4> await_resume() const { return std::move( result ); }
	};
	using fn_dns_hook = bool(*)( dns_query_awaitable* );
	inline fn_dns_hook g_dns_hook = nullptr;

	// Standard options.
	//
	enum class socket_protocol {
		none,
		tcp,
	};
	struct socket_options {
		xstd::duration                conn_timeout = 5s;
		xstd::duration                linger =       30s;
		size_t                        recvbuf =      512_kb;
		size_t                        sendbuf =      256_kb;
		bool                          nodelay =      true;
		bool                          timestamps =   false;
		bool                          reuse =        true;
		std::optional<xstd::duration> keepalive =    std::nullopt;
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

	// Global net lock, only protects the FDD list in Berkeley mode.
	//
	inline xstd::spinlock g_lock = {};
	namespace detail {
		// Wrappers around functions differring between WSA / Berkeley standards.
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
			if ( fd != invalid_socket ) {
				unsigned long ionbio = 1;
				err = ioctlsocket( fd, FIONBIO, &ionbio );
				if ( socket_error err = ioctlsocket( fd, FIONBIO, &ionbio ) ) {
					err = get_last_error( err );
				}
			} else {
				err = get_last_error( -1 );
			}
#else
			if ( !protocol ) {
				protocol = 0;
			}
			socket_t fd = ::socket( af, type, *protocol );
			if ( fd != invalid_socket ) {
				int flags = fcntl( fd, F_GETFL, 0 );
				if ( flags != -1 ) {
					err = fcntl( fd, F_SETFL, flags | O_NONBLOCK );
				} else {
					err = get_last_error( -1 );
				}
			} else {
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

		// Base of the stream.
		//
		struct fd_thread;
		struct fd_duplex : xstd::duplex {
			friend struct fd_thread;
		protected:
			// Options.
			//
			socket_options  opt = {};
			socket_protocol proto = {};

			// File descriptor.
			//
			socket_t        fd = invalid_socket;

			// Time it was opened or the timeout limit.
			//
			xstd::timestamp open_time = {};

			// Event flags.
			//
			std::atomic<bool> evt_wr = true;

			// Constructed by protocol.
			//
			fd_duplex( socket_protocol proto ) : proto( proto ) {}

			// Wrappers.
			//
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
					timeval tv = to_timeval( val );
					return set_socket_opt( level, name, &tv, sizeof( timeval ) );
				} else {
					return set_socket_opt( level, name, &val, sizeof( T ) );
				}
			}
			template<typename T>
			socket_error get_socket_opt( int level, int name, T& val ) {
				if constexpr ( std::is_same_v<T, xstd::duration> ) {
					timeval tv = to_timeval( val );
					size_t  sz = sizeof( timeval );
					socket_error err = get_socket_opt( level, name, &tv, &sz );
					val = from_timeval( tv );
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
			std::optional<socket_error> socket_connect( ipv4 adr, uint16_t port ) {
				sockaddr_in addr;
				memset( &addr, 0, sizeof( sockaddr_in ) );
				addr.sin_family = AF_INET;
				addr.sin_port = bswap( port );
				addr.sin_addr.s_addr = adr;
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
			void socket_receive( std::span<uint8_t>& buffer, int flags = 0 ) {
#if XSTD_WINSOCK
				flags |= MSG_PUSH_IMMEDIATE;
				if ( buffer.size() > UINT32_MAX )
					buffer = buffer.subspan( 0, (size_t) UINT32_MAX );
				DWORD  count = 0;
				DWORD  ioflags = (DWORD) flags;
				WSABUF bufdesc = { (uint32_t) buffer.size(), (char*) buffer.data() };
				if ( socket_error result = ::WSARecv( this->fd, &bufdesc, 1, &count, &ioflags, nullptr, nullptr ); result == -1 ) [[unlikely]] {
					if ( auto e = detail::get_last_error( result ); e != WSAEINTR && e != WSAEINPROGRESS && e != WSAEWOULDBLOCK && e != WSAEMSGSIZE ) {
						this->raise_errno( XSTD_ESTR( "socket error: %d" ), e );
						return;
					}
				}
				buffer = buffer.subspan( 0, count );
#else
				if ( buffer.size() > INT32_MAX )
					buffer = buffer.subspan( 0, (size_t) INT32_MAX );
				if ( socket_error result = ::recv( this->fd, (char*) buffer.data(), (int) buffer.size(), flags ); result == -1 ) [[unlikely]] {
					if ( auto e = detail::get_last_error( result ); e != EAGAIN && e != EWOULDBLOCK ) {
						this->raise_errno( XSTD_ESTR( "socket error: %d" ), e );
						return;
					}
					buffer = {};
				} else {
					buffer = buffer.subspan( 0, (uint32_t) result );
				}
#endif
			}
			void socket_send( xstd::vec_buffer& buffer, int flags = 0 ) {
#if XSTD_WINSOCK
				uint32_t write_count = buffer.size() > 0xffffffffu ? 0xffffffffu : uint32_t( buffer.size() );
				DWORD  count = 0;
				WSABUF bufdesc = { write_count, (char*) buffer.data() };
				if ( socket_error result = ::WSASend( this->fd, &bufdesc, 1, &count, (DWORD) flags, nullptr, nullptr ); result == -1 ) [[unlikely]] {
					if ( auto e = detail::get_last_error( result ); e != WSAEINTR && e != WSAEINPROGRESS && e != WSAEWOULDBLOCK && e != WSAEMSGSIZE ) {
						this->raise_errno( XSTD_ESTR( "socket error: %d" ), e );
						return;
					}
				}
				buffer.shift( count );
#else
				uint32_t write_count = buffer.size() > 0x7fffffffu ? 0x7fffffffu : uint32_t( buffer.size() );
				if ( socket_error result = ::send( this->fd, (char*) buffer.data(), (int)write_count, flags ); result == -1 ) [[unlikely]] {
					if ( auto e = detail::get_last_error( result ); e != EAGAIN && e != EWOULDBLOCK ) {
						this->raise_errno( XSTD_ESTR( "socket error: %d" ), e );
						return;
					}
				} else {
					buffer.shift( (uint32_t) result );
				}
#endif
				evt_wr = true;
			}

			// Sets options.
			//
			void reconfigure_tcp() {
				this->try_set_socket_opt( SOL_TCP, TCP_NODELAY, opt.nodelay );
				this->try_set_socket_opt( SOL_TCP, TCP_TIMESTAMPS, opt.timestamps );
				if ( opt.keepalive ) {
					this->set_socket_opt( SOL_TCP, TCP_KEEPIDLE, uint32_t( *opt.keepalive / 1s ) );
				}
			}
			void reconfigure_sock() {
				this->try_set_socket_opt( SOL_SOCKET, SO_REUSEADDR, opt.reuse );
				this->try_set_socket_opt( SOL_SOCKET, SO_LINGER, opt.linger );
				this->try_set_socket_opt( SOL_SOCKET, SO_SNDBUF, opt.sendbuf );
				this->try_set_socket_opt( SOL_SOCKET, SO_RCVBUF, opt.recvbuf );
				if ( opt.keepalive ) {
					this->set_socket_opt( SOL_SOCKET, SO_KEEPALIVE, true );
				} else {
					this->set_socket_opt( SOL_SOCKET, SO_KEEPALIVE, false );
				}
			}

			// Close the socket on destruction.
			//
			~fd_duplex() {
				close_socket( fd );
			}

		protected:
			void raise_errno( const char* fmt, socket_error e = detail::get_last_error( -1 ) ) {
				destroy( { fmt, e } );
			}
			bool assert_errno( const char* fmt, socket_error e ) {
				if ( !e ) [[likely]] {
					return true;
				}
				raise_errno( fmt, e );
				return false;
			}
		};

		// fd thread list.
		//
#ifndef FD_SETSIZE
		static constexpr size_t fd_per_thread = std::min<size_t>( 16, FD_SETSIZE );
#else
		static constexpr size_t fd_per_thread = 16;
#endif
		struct fd_thread;
		inline fd_thread*     g_current_fdd = nullptr;
		struct fd_thread {
			xstd::spinlock                        lock = {};
			std::array<fd_duplex*, fd_per_thread> list = {};
			size_t                                list_pos = 0;
			uint8_t                               buffer[ 8_mb ];

			void runner_thread() {
				socket_t   fd[ fd_per_thread ];
				fd_set     rd_watch, wr_watch, er_watch;

				struct timeval timeout = {};
				timeout.tv_sec =  1;
				timeout.tv_usec = 0;

				while ( true ) {
					FD_ZERO( &rd_watch );
					FD_ZERO( &wr_watch );
					FD_ZERO( &er_watch );

					socket_t   fd_max = 0;
					size_t     count = 0;
					{
						std::unique_lock ll{ this->lock };

						// Create a list of all sockets, decrement references of the dead ones and re-arrange.
						//
						for ( auto& e : std::span{ this->list }.subspan( 0, this->list_pos ) ) {
							if ( e ) {
								if ( auto efd = e->fd; efd != invalid_socket ) {
									this->list[ count ] = e;
									fd[ count ] = efd;
									fd_max = std::max( fd_max, efd );
									count++;
									FD_SET( efd, &rd_watch );
									FD_SET( efd, &er_watch );
									if ( e->evt_wr.exchange( false ) )
										FD_SET( efd, &wr_watch );
								} else {
									e->dec_ref();
									e = nullptr;
								}
							}
						}
						this->list_pos = count;

						// If empty:
						//
						if ( !count ) {
							// Reaquire the locks in the correct order.
							//
							ll.unlock();
							std::unique_lock gl{ g_lock };
							ll.lock();

							// If an entry was added while we were reacquiring, retry.
							//
							if ( this->list_pos != count )
								continue;

							// If we're not the current FDD, delete and terminate.
							//
							if ( g_current_fdd != this ) {
								ll.unlock();
								delete this;
							}

							// Otherwise just stop the thread until it becomes active again.
							//
							return;
						}
					}

					// Issue the select call, ignore errors since it may be caused by socket being closed.
					//
					int res = select( ( int ) fd_max, &rd_watch, &wr_watch, &er_watch, &timeout );
					auto time = xstd::time::now();

					// For each socket we were watching:
					//
					for ( size_t n = 0; n != count; n++ ) {
						auto f = fd[ n ];
						auto s = this->list[ n ];

						// If socked closed while we're selecting, delete.
						//
						if ( s->fd != f ) [[unlikely]] {
							s->dec_ref();
							this->list[ n ] = nullptr;
							continue;
						}

						// If it errored, raise the error, and delete.
						//
						if ( FD_ISSET( f, &er_watch ) ) {
							socket_error error = 0;
							size_t len = sizeof( socket_error );
							socket_error opt_error = s->get_socket_opt( SOL_SOCKET, SO_ERROR, &error, &len );
							s->raise_errno( XSTD_ESTR( "socket error: %d" ), error ? error : opt_error );
							s->dec_ref();
							this->list[ n ] = nullptr;
							continue;
						}

						// If still opening, use wr_watch for connection, else for drain.
						//
						if ( s->opening() ) {
							if ( FD_ISSET( f, &wr_watch ) ) {
								s->open_time = time;
								s->on_ready();
							} else if ( time > s->open_time ) {
								s->destroy( XSTD_ESTR( "connection timeout" ) );
							}
						} else {
							if ( FD_ISSET( f, &wr_watch ) ) {
								s->on_drain( s->opt.sendbuf );
							}
						}

						// Receive data.
						//
						if ( FD_ISSET( f, &rd_watch ) ) {
							std::span<uint8_t> range{ this->buffer };
							s->socket_receive( range );
							if ( !range.empty() ) {
								s->on_input( range );
							}
						}
					}
				}
			}
			bool watch( fd_duplex* fdd ) {
				std::unique_lock lg{ lock };
				if ( list_pos == list.size() )
					return false;

				// Set the entry.
				//
				list[list_pos] = fdd;
				fdd->add_ref();

				// If it was not running, start.
				//
				if ( !list_pos++ ) [[unlikely]] {
					lg.unlock();
					std::thread( [p = this] { p->runner_thread(); } ).detach();
				}
				return true;
			}
		};

		// Starts watching an FDD.
		//
		inline void watch_fd( fd_duplex* dp ) {
			std::lock_guard gl{ g_lock };
			auto* fdd = g_current_fdd;
			bool watched = fdd && fdd->watch( dp );
			if ( !watched ) {
				fdd = g_current_fdd = new fd_thread();
				watched = fdd->watch( dp );
			}
			dassert( watched );
		}
	};

	// Implement DNS resolution.
	//
	inline bool dns_query_awaitable::await_suspend( std::coroutine_handle<> hnd ) {
		if ( !no_hook && g_dns_hook ) {
			return g_dns_hook( this );
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

	// TCP implementation.
	//
	struct tcp : detail::fd_duplex {
		// Remote connection.
		//
		const ipv4    remote_ip;
		const int16_t remote_port;

		// Initializer.
		//
		tcp( ipv4 adr, uint16_t port, socket_options opts = {} ) : fd_duplex( socket_protocol::tcp ), remote_ip(adr), remote_port(port) {
			// Initialize networking.
			//
			if ( !this->assert_errno( XSTD_ESTR( "failed to initialize networking: %d" ), detail::init_networking() ) )
				return;

			// Create the socket.
			//
			if ( !this->assert_errno( XSTD_ESTR( "failed to create a socket: %d" ), detail::create_socket( &this->fd, AF_INET, SOCK_STREAM ) ) )
				return;

			// Set socket options.
			//
			this->configure( opts );

			// Try to connect.
			//
			if ( auto err = this->socket_connect( adr, port ); err.has_value() ) {
				this->raise_errno( XSTD_ESTR( "connection failed: %d" ), *err );
			}

			// Start the timeout timer, create the thread.
			//
			open_time = xstd::time::now() + opts.conn_timeout;
			detail::watch_fd( this );
		}

		// Configuration.
		//
		void configure( socket_options opt ) {
			this->opt = opt;
			if ( this->fd != invalid_socket ) {
				reconfigure_sock();
				if ( proto == socket_protocol::tcp )
					reconfigure_tcp();
			}
		}

		// Implement the interface.
		//
	protected:
		void terminate() override {
			if ( auto prev_sock = std::exchange( this->fd, invalid_socket ) ) {
				detail::close_socket( prev_sock );
			}
		}
		bool try_close() override {
			return false;
		}
		bool try_output( xstd::vec_buffer& data ) override {
			this->socket_send( data );
			return false;
		}
	};
#endif

	// Definition via LWIP:
	//
#if XSTD_LWIP
	// Global net lock, replaces LWIP core lock.
	//
	inline static io_mutex g_lock = {};
	
	// Implement DNS resolution.
	//
	inline bool dns_query_awaitable::await_suspend( std::coroutine_handle<> hnd ) {
		if ( !no_hook && g_dns_hook ) {
			return g_dns_hook( this );
		}
		this->continuation = hnd;

		std::unique_lock lock{ g_lock };
		err_t err = dns_gethostbyname_addrtype( this->hostname, this->result.emplace().lwip(), []( const char* hostname, const ip_addr_t* ipaddr, void* callback_arg ) {
			auto* ctx = ( (dns_query_awaitable*) callback_arg );
			ctx->result.emplace( ipaddr );
			xstd::chore( ctx->continuation );
		}, this, LWIP_DNS_ADDRTYPE_IPV4 );

		if ( err == ERR_INPROGRESS ) {
			return true;
		} else if ( err ) {
			this->result.raise( xstd::exception{ XSTD_ESTR( "failed to query dns: %d" ), err } );
			return false;
		} else {
			return false;
		}
	}

	// TCP implementation.
	//
	struct tcp : xstd::duplex {
		// Socket options.
		//
		socket_options opt = {};

		// Remote connection.
		//
		const ipv4    remote_ip;
		const int16_t remote_port;

		// PCB.
		//
		tcp_pcb* pcb = nullptr;

		// Time it was opened or the timeout limit.
		//
		xstd::timestamp open_time = {};

		// Initializer.
		//
		tcp( ipv4 adr, uint16_t port, socket_options opts = {} ) : remote_ip(adr), remote_port(port) {
			// Initialize networking.
			//
			//if ( !this->assert_errno( XSTD_ESTR( "failed to initialize networking: %d" ), detail::init_networking() ) )
			//	return;

			// Create the socket.
			//
			std::unique_lock lock{ g_lock };
			pcb = tcp_new();
			if ( !pcb ) {
				lock.unlock();
				this->destroy( XSTD_ESTR( "failed to create a socket" ) );
				return;
			}

			// Bind the calllbacks.
			//
			tcp_arg( pcb, this );
			tcp_err( pcb, (tcp_err_fn) sock_err );
			tcp_poll( pcb, (tcp_poll_fn) sock_poll, 1 );
			tcp_recv( pcb, (tcp_recv_fn) sock_recv );
			tcp_sent( pcb, (tcp_sent_fn) sock_sent );

			// Set socket options.
			//
			this->configure( opts );

			// Try to connect.
			//
			if ( auto err = tcp_connect( pcb, adr.lwip(), port, (tcp_connected_fn) sock_connected ) ) {
				this->raise_errno( XSTD_ESTR( "connection failed: %d" ), err );
			}

			// Start the timeout timer.
			//
			open_time = xstd::time::now() + opts.conn_timeout;
		}

		// Close the socket on destruction.
		//
		~tcp() {
			terminate();
		}

		// Configuration.
		//
		void configure( socket_options opt ) {
			this->opt = opt;
			std::unique_lock lock{ g_lock };
			if ( auto* p = pcb ) {
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
#if LWIP_TCP_TIMESTAMPS
				if ( opt.timestamps ) {
					tcp_set_flags( pcb, TF_TIMESTAMP );
				} else {
					tcp_clear_flags( pcb, TF_TIMESTAMP );
				}
#else
				opt.timestamps = false;
#endif
				if ( opt.keepalive ) {
					pcb->keep_idle = ( uint32_t ) std::clamp<int64_t>( *opt.keepalive / 1ms, 0, UINT32_MAX );
					ip_set_option( pcb, SOF_KEEPALIVE );
				} else {
					ip_reset_option( pcb, SOF_KEEPALIVE );
				}
				opt.sendbuf = TCP_SND_BUF;

				// TODO: linger
				// TODO: make buffer configurable with circular inner buffer & no copy to LWIP
			}
		}

		// Implement the interface.
		//
	protected:
		void terminate() override {
			std::unique_lock lock{ g_lock };
			if ( auto prev_pcb = std::exchange( this->pcb, nullptr ) ) {
				tcp_arg( prev_pcb, nullptr ); // Do not recurse.
				if ( tcp_close( prev_pcb ) != ERR_OK )
					tcp_abort( prev_pcb );
			}
		}
		bool try_close() override {
			// TODO: Flush remaining data.
			return false;
		}
		bool try_output( xstd::vec_buffer& data ) override {
			std::unique_lock lock{ g_lock };
			size_t offset = tcpi_write( data.data(), data.size(), 0 );
			data.shift( offset );
			return false;
		}

	private:
		// Error type handlers.
		//
		void raise_errno( const char* fmt, err_t e ) {
			close( { fmt, (int) e } );
		}
		bool assert_errno( const char* fmt, err_t e ) {
			if ( !e ) [[likely]] {
				return true;
			}
			raise_errno( fmt, e );
			return false;
		}

		// TCP internals, always called with lock held.
		//
		size_t tcpi_write( const uint8_t* data, size_t length, bool last ) {
			size_t  offset = 0;
			bool    exhaused = false;
			while ( !exhaused ) {
				// Calculate the maximum fragment length.
				//
				size_t  frag = length - offset;
				frag = std::min<size_t>( frag, 0xFFFF );
				frag = std::min<size_t>( frag, tcp_sndbuf( pcb ) );
				if ( !frag ) break;
				
				// Try writing in increasingly smaller sizes.
				//
				err_t status;
				do {
					status = tcp_write( pcb, (char*) data + offset, (uint16_t) frag, TCP_WRITE_FLAG_COPY | ( ( last && ( offset + frag ) == length ) ? 0 : TCP_WRITE_FLAG_MORE ) );
					if ( status != ERR_MEM ) {
						break;
					} else {
						exhaused = true;
						if ( ( tcp_sndbuf( pcb ) == 0 ) || ( tcp_sndqueuelen( pcb ) >= TCP_SND_QUEUELEN ) ) {
							frag = 0;
							status = ERR_OK;
							break;
						} else {
							frag >>= 1;
						}
					}
				} while ( false );

				// If error, fail.
				//
				if ( status != ERR_OK ) {
					raise_errno( XSTD_ESTR( "socket write error: %d" ), status );
					break;
				} 
				// Else, increment offset.
				//
				else {
					offset += frag;
				}
			}
			return offset;
		}
		err_t tcpi_close( xstd::exception ex, tcp_pcb* tpcb ) {
			this->pcb = nullptr;
			if ( tpcb ) {
				tcp_arg( tpcb, nullptr ); // Do not recurse.
				if ( tcp_close( tpcb ) != ERR_OK )
					tcp_abort( tpcb );
			}
			if ( !this->closed() ) {
				this->inc_ref();
				xstd::chore( [self = this, ex = std::move(ex)]() mutable {
					self->on_close( std::move( ex ) );
					self->dec_ref();
				} );
			}
			return ERR_ABRT;
		}

		// Individual handlers as static functions.
		//
		static void sock_err( tcp* cli, err_t err ) {
			if ( cli ) {
				cli->pcb = nullptr;
				cli->tcpi_close( { XSTD_ESTR( "socket error: %d" ), (int)err }, nullptr );
			}
		}
		static err_t sock_poll( tcp* cli, tcp_pcb* tpcb ) {
			if ( cli ) {
				if ( cli->opening() && cli->open_time > xstd::time::now() ) {
					return cli->tcpi_close( XSTD_ESTR( "connection timed out" ), tpcb );
				} else if ( cli->is_send_buffering() && tcp_sndbuf( tpcb ) ) {
					cli->on_drain( tcp_sndbuf( tpcb ) );
				}
			}
			return ERR_OK;
		}
		static err_t sock_recv( tcp* cli, tcp_pcb* tpcb, pbuf* p, err_t err ) {
			// Handle connection reset.
			//
			if ( !p ) {
				if ( cli )
					return cli->tcpi_close( XSTD_ESTR( "connection reset" ), tpcb );
				return ERR_OK;
			}

			if ( cli ) {
				int32_t n = p->tot_len;
				for ( auto it = p; it != nullptr && n > 0; n -= it->len, it = it->next ) {
					cli->on_input( { (uint8_t*) it->payload, std::min<size_t>( it->len, (uint32_t) n ) } );
					if ( !cli->pcb ) {
						pbuf_free( p );
						return ERR_ABRT;
					}
				}
				tcp_recved( tpcb, p->tot_len );
			}
			pbuf_free( p );
			return ERR_OK;
		}
		static err_t sock_sent( tcp* cli, tcp_pcb* tpcb, u16_t len ) {
			if ( cli ) {
				cli->on_drain( tcp_sndbuf( tpcb ) );
				return cli->pcb ? ERR_OK : ERR_ABRT;
			}
			return ERR_OK;
		}
		static err_t sock_connected( tcp* cli, tcp_pcb* tpcb, err_t er ) {
			if ( !cli ) [[unlikely]] {
				tcp_abort( tpcb );
				return ERR_ABRT;
			}
			cli->open_time = xstd::time::now();
			cli->on_ready();
			return cli->pcb ? ERR_OK : ERR_ABRT;
		}
	};
#endif

	// Functional wrappers.
	//
	namespace connect {
		inline ref<net::tcp> tcp( ipv4 adr, uint16_t port, socket_options opts = {} ) {
			return make_refc<net::tcp>( adr, port, std::move( opts ) );
		}
	};
	namespace dns {
		inline net::dns_query_awaitable query_a( const char* hostname, bool no_hook = false ) {
			return { hostname, no_hook };
		}
	};
};