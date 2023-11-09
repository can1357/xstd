#pragma once
#include "text.hpp"
#include "hashable.hpp"
#include "socket.hpp"

namespace xstd {
	using web_hasher = basic_ahash<crc32c, void>;
	XSTD_MAKE_TEXT_SHADER( _wh, xstd::web_hasher );

	namespace detail {
		FORCE_INLINE static constexpr auto split( std::string_view haystack, std::string_view needle, bool bwd = false, bool skip = true ) {
			std::string_view a;
			std::string_view b = haystack;
			if ( bwd ) std::swap( a, b );
			if ( size_t p = haystack.find( needle ); p != std::string::npos ) {
				a = haystack.substr( 0, p );
				b = haystack.substr( p + ( skip ? needle.size() : 0 ) );
			}
			haystack = b;
			return std::tuple{ a, b };
		}
		static constexpr std::pair<uint32_t, uint16_t> schema_to_port[] = {
			{ "http"_wh,  80 },
			{ "https"_wh, 443 },
			{ "ws"_wh,    80 },
			{ "wss"_wh,   443 },
		};
	};

	template<typename T>
	struct basic_uri {
		T schema;
		T username;
		T password;
		T hostname;
		T pathname;
		T search;
		T fragment;
		uint16_t port;

		// Default move, ctor.
		//
		constexpr basic_uri() noexcept = default;
		constexpr basic_uri( basic_uri&& ) noexcept = default;
		constexpr basic_uri& operator=( basic_uri&& ) noexcept = default;
		
		// Copy from self and others.
		//
		constexpr basic_uri( const basic_uri& ) noexcept = default;
		constexpr basic_uri& operator=( const basic_uri& ) noexcept = default;
		template<typename O>
		constexpr basic_uri( const basic_uri<O>& o ) noexcept { this->template assign<O>( o ); }
		template<typename O>
		constexpr basic_uri& operator=( const basic_uri<O>& o ) noexcept { this->template assign<O>( o ); return *this; }
		template<typename O>
		constexpr void assign( const basic_uri<O>& o ) {
			schema = T{ o.schema };
			username = T{ o.username };
			password = T{ o.password };
			hostname = T{ o.hostname };
			pathname = T{ o.pathname };
			search = T{ o.search };
			fragment = T{ o.fragment };
			port = o.port;
		}

		// Construction by string view.
		//
		constexpr basic_uri( std::string_view sv ) {
			if ( sv.starts_with( '/' ) ) {
				set_path( sv );
				return;
			}
			auto [s, uhppqf] = detail::split( sv, ":" );
			schema = s;
			if ( uhppqf.starts_with( "//" ) ) uhppqf.remove_prefix( 2 );

			auto [auth, hppqf] = detail::split( uhppqf, "@" );
			set_auth( auth );
			auto [hp, pqf] = detail::split( hppqf, "/", false, false );
			set_host( hp );
			set_path( pqf );
		}

		constexpr basic_uri( const char* ptr ) : basic_uri( std::string_view{ ptr } ) {}

		// Assigning compound fields.
		//
		constexpr void set_auth( std::string_view auth ) {
			auto [u, p] = detail::split( auth, ":", true );
			username = u; 
			password = p;
		}
		constexpr void set_host( std::string_view host ) {
			auto [h, p] = detail::split( host, ":", true );
			hostname = h;
			port = parse_number<uint16_t>( p, 10 );
		}
		constexpr void set_path( std::string_view path ) {
			if ( path.empty() ) {
				pathname = "/";
			} else {
				auto [p, qf] = detail::split( path, "?", true, false );
				auto [q, f] = detail::split( qf, "#", true, false );
				pathname = p;
				search = q;
				fragment = f;
			}
		}

		// Observers.
		//
		constexpr uint16_t port_or_default() const {
			if ( !port ) {
				uint32_t hash = web_hasher{}( schema );
				for ( auto& [s, p] : detail::schema_to_port ) {
					if ( hash == s ) {
						return p;
					}
				}
				return 0;
			} else {
				return port;
			}
		}
		std::string host() const {
			std::string result{ hostname };
			if ( port ) {
				result += ':';
				result += std::to_string( port );
			}
			return result;
		}
		std::string protocol() const {
			std::string result{ schema };
			if ( !result.empty() ) {
				result += ':';
			}
			return result;
		}
		std::string origin() const {
			return protocol() + "//" + host();
		}
		std::string auth() const {
			std::string result{ username };
			if ( !password.empty() ) {
				result += ':';
				result += password;
			}
			return result;
		}
		std::string path() const {
			std::string result{ pathname };
			result += search;
			result += fragment;
			return result;
		}
		std::string href() const {
			std::string result{ protocol() += "//" };
			if ( auto a = auth(); !a.empty() ) {
				result += a;
				result += "@";
			}
			result += host();
			result += path();
			return result;
		}

		// String conversion.
		//
		std::string to_string() const {
			return href();
		}
	};

	using uri_view = basic_uri<std::string_view>;
	using uri =      basic_uri<std::string>;
};
