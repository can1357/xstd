#pragma once
#include "type_helpers.hpp"
#include "hashable.hpp"
#include "utf.hpp"

namespace xstd
{
	// Constexpr implementations of cstring functions.
	//
	inline constexpr uint32_t cxlower( uint32_t cp ) { return cp ^ ( ( ( 'A' <= cp ) & ( cp <= 'Z' ) ) << 5 ); }
	inline constexpr uint32_t cxupper( uint32_t cp ) { return cp ^ ( ( ( 'a' <= cp ) & ( cp <= 'z' ) ) << 5 ); }
	template<String T> inline constexpr size_t strlen( T&& str ) { return string_view_t<T>{ str }.size(); }

	// Common helpers.
	//
	namespace impl
	{
		template<typename C, typename H, bool CaseInsensitive>
		inline constexpr H make_text_hash( std::basic_string_view<C> input )
		{
			using U = convert_uint_t<C>;

			H h = {};
			while ( !input.empty() )
			{
				C front = input.front();

				// If ASCII:
				//
				if ( U( front ) <= 0x7f ) [[likely]]
				{
					if constexpr ( CaseInsensitive )
						front = cxlower( U( front ) );
					h.template add_bytes<char>( front );
					input.remove_prefix( 1 );
				}
				// Otherwise add as code-point:
				//
				else
				{
					uint32_t cp = codepoint_cvt<C>::decode( input );
					h.add_bytes( cp );
				}
			}
			return h;
		}
	};

	// Default string hashers.
	//
	using xhash_t = fnv64;
	using ihash_t = fnv64;

	// Case/Width insensitive hasher.
	//
	template<typename H, typename T = void>
	struct basic_ihash;
	template<typename H, String T>
	struct basic_ihash<H, T>
	{
		inline constexpr H operator()( const T& value ) const noexcept
		{
			return impl::make_text_hash<string_unit_t<T>, H, true>( std::basic_string_view<string_unit_t<T>>{ value } );
		}
	};
	template<typename H>
	struct basic_ihash<H, void>
	{
		template<String S>
		FORCE_INLINE inline constexpr H operator()( const S& value ) const noexcept
		{
			return basic_ihash<H, S>{}( value );
		}
	};

	// Width insensitive hasher.
	//
	template<typename H, typename T = void>
	struct basic_xhash;
	template<typename H, String T>
	struct basic_xhash<H, T>
	{
		inline constexpr H operator()( const T& value ) const noexcept
		{
			return impl::make_text_hash<string_unit_t<T>, H, false>( std::basic_string_view<string_unit_t<T>>{ value } );
		}
	};
	template<typename H>
	struct basic_xhash<H, void>
	{
		template<String S>
		FORCE_INLINE inline constexpr H operator()( const S& value ) const noexcept
		{
			return basic_xhash<H, S>{}( value );
		}
	};
	template<typename T = void> using ihash = basic_ihash<ihash_t, T>;
	template<typename T = void> using xhash = basic_xhash<xhash_t, T>;
	template<typename H, String T> inline constexpr H make_ihash( const T& value ) noexcept { return basic_ihash<H, T>{}( value ); }
	template<typename H, String T> inline constexpr H make_xhash( const T& value ) noexcept { return basic_xhash<H, T>{}( value ); }
	template<String T> inline constexpr ihash_t make_ihash( const T& value ) noexcept { return make_ihash<ihash_t>( value ); }
	template<String T> inline constexpr xhash_t make_xhash( const T& value ) noexcept { return make_xhash<xhash_t>( value ); }

	// Width insensitive string operations.
	//
	template<String S1, String S2>
	inline constexpr int xstrcmp( S1&& a, S2&& b )
	{
		string_view_t<S1> va = { a };
		string_view_t<S2> vb = { b };
		return utf_compare( va, vb );
	}
	template<String S1, String S2>
	inline constexpr bool xequals( S1&& a, S2&& b )
	{
		string_view_t<S1> va = { a };
		string_view_t<S2> vb = { b };
		return utf_cmpeq( va, vb );
	}
	template<String S1, String S2>
	inline constexpr size_t xfind( S1&& in, S2&& v )
	{
		string_view_t<S1> inv = { in };
		string_view_t<S2> sv = { v };
		size_t len = utf_length<string_unit_t<S1>>( sv );
		ptrdiff_t diff = inv.length() - len;
		for ( ptrdiff_t n = 0; n <= diff; n++ )
			if ( xequals( inv.substr( n, len ), sv ) )
				return ( size_t ) n;
		return std::string::npos;
	}
	template<String S1, String S2>
	inline constexpr bool xstarts_with( S1&& a, S2&& b )
	{
		string_view_t<S1> av = { a };
		string_view_t<S2> bv = { b };
		size_t len = utf_length<string_unit_t<S1>>( bv );
		if ( av.length() < len ) [[unlikely]]
			return false;
		av.remove_suffix( av.length() - len );
		return xequals( av, bv );
	}
	template<String S1, String S2>
	inline constexpr bool xends_with( S1&& a, S2&& b )
	{
		string_view_t<S1> av = { a };
		string_view_t<S2> bv = { b };
		size_t len = utf_length<string_unit_t<S1>>( bv );
		if ( av.length() < len ) [[unlikely]]
			return false;
		av.remove_prefix( av.length() - len );
		return xequals( av, bv );
	}

	// Case conversion.
	//
	template<bool ToLower, typename To, typename From>
	inline std::basic_string<To> convert_case( std::basic_string_view<From> src )
	{
		size_t max_out = Same<To, From> ? src.size() + codepoint_cvt<To>::max_out : codepoint_cvt<To>::max_out * src.size();
		std::basic_string<To> result( max_out, '\0' );
		size_t length = utf_convert<To, From, true, !ToLower, ToLower>( src, result );

		size_t clength = result.size();
		assume( length < clength );
		result.resize( length );
		return result;
	}
	template<typename Encoding, String S>
	inline std::basic_string<Encoding> to_lower( S&& str )
	{
		using From = string_unit_t<S>;
		return convert_case<true, Encoding, From>( str );
	}
	template<typename Encoding, String S>
	inline std::basic_string<Encoding> to_upper( S&& str )
	{
		using From = string_unit_t<S>;
		return convert_case<false, Encoding, From>( str );
	}
	template<String S> inline auto to_lower( S&& str ) { return to_lower<string_unit_t<S>, S>( std::forward<S>( str ) ); }
	template<String S> inline auto to_upper( S&& str ) { return to_upper<string_unit_t<S>, S>( std::forward<S>( str ) ); }

	// Case insensitive string operations.
	//
	template<String S1, String S2>
	inline constexpr int istrcmp( S1&& a, S2&& b )
	{
		string_view_t<S1> va = { a };
		string_view_t<S2> vb = { b };
		return utf_icompare( va, vb );
	}
	template<String S1, String S2>
	inline constexpr bool iequals( S1&& a, S2&& b )
	{
		string_view_t<S1> va = { a };
		string_view_t<S2> vb = { b };
		return utf_icmpeq( va, vb );
	}
	template<String S1, String S2>
	inline constexpr size_t ifind( S1&& in, S2&& v )
	{
		string_view_t<S1> inv = { in };
		string_view_t<S2> sv = { v };
		size_t len = utf_length<string_unit_t<S1>>( sv );
		ptrdiff_t diff = inv.length() - len;
		for ( ptrdiff_t n = 0; n <= diff; n++ )
			if ( iequals( inv.substr( n, len ), sv ) )
				return ( size_t ) n;
		return std::string::npos;
	}
	template<String S1, String S2>
	inline constexpr bool istarts_with( S1&& a, S2&& b )
	{
		string_view_t<S1> av = { a };
		string_view_t<S2> bv = { b };
		size_t len = utf_length<string_unit_t<S1>>( bv );
		if ( av.length() < len ) [[unlikely]]
			return false;
		av.remove_suffix( av.length() - len );
		return iequals( av, bv );
	}
	template<String S1, String S2>
	inline constexpr bool iends_with( S1&& a, S2&& b )
	{
		string_view_t<S1> av = { a };
		string_view_t<S2> bv = { b };
		size_t len = utf_length<string_unit_t<S1>>( bv );
		if ( av.length() < len ) [[unlikely]]
			return false;
		av.remove_prefix( av.length() - len );
		return iequals( av, bv );
	}

	// Text splitting.
	//
	template<typename C = char, String R = std::basic_string_view<C>>
	static std::vector<R> split_string( std::basic_string_view<C> in, C by )
	{
		std::vector<R> result;
		while ( !in.empty() )
		{
			size_t s = in.find( by );
			if ( s == std::string::npos )
			{
				result.push_back( R{ in } );
				break;
			}
			result.push_back( R{ in.substr( 0, s ) } );
			in.remove_prefix( s + 1 );
		}
		return result;
	}

	template<typename C = char, String R = std::basic_string_view<C>>
	static std::vector<R> split_lines( std::basic_string_view<C> in )
	{
		std::vector<R> result = split_string<C, R>( in, '\n' );
		for ( auto& chunk : result )
		{
			if ( !chunk.empty() && chunk.back() == '\r' )
			{
				if constexpr ( CppStringView<R> )
					chunk.remove_suffix( 1 );
				else if constexpr ( CppString<R> )
					chunk.pop_back();
				else
					unreachable();
			}
		}
		return result;
	}

	// Case insensitive comparison predicate.
	//
	struct icmp_less
	{
		template<String T1, String T2>
		constexpr bool operator()( const T1& s1, const T2& s2 ) const noexcept
		{
			return istrcmp( s1, s2 ) < 0;
		}
	};
	struct icmp_equals
	{
		template<String T1, String T2>
		constexpr bool operator()( const T1& s1, const T2& s2 ) const noexcept
		{
			return iequals( s1, s2 );
		}
	};
	struct icmp_greater
	{
		template<String T1, String T2>
		constexpr bool operator()( const T1& s1, const T2& s2 ) const noexcept
		{
			return istrcmp( s1, s2 ) > 0;
		}
	};

	// Simple number to string conversation, no input validation.
	//  - Format => {0o 0x <>} (+/-) + [0-9] + '.' + [0-9]
	//  - _v version takes a reference to a string view and erases the relevant part.
	//
	template<typename T = uint64_t, typename C> requires ( Integral<T> || FloatingPoint<T> )
	static constexpr T parse_number_v( std::basic_string_view<C>& view, int default_base = 10 )
	{
		using I = std::conditional_t<Unsigned<T>, convert_int_t<T>, T>;
		if ( view.empty() ) return 0;
		
		// Parse the sign.
		//
		I sign = +1;
		if ( view.front() == '-' )
		{
			sign = -1;
			view.remove_prefix( 1 );
		}
		else if ( view.front() == '+' )
		{
			view.remove_prefix( 1 );
		}

		// Find out the mode.
		//
		int base = default_base;
		if ( view.size() >= 2 )
		{
			if ( view[ 0 ] == '0' && cxlower( view[ 1 ] ) == 'x' )
				base = 16, view.remove_prefix( 2 );
			else if ( view[ 0 ] == '0' && cxlower( view[ 1 ] ) == 'o' )
				base = 8, view.remove_prefix( 2 );
		}

		// Declare digit parser.
		//
		T value = 0;
		auto parse_digit = [ hex = ( base == 16 ) ] ( char c )
		{
			if ( hex )
			{
				if ( c >= 'a' ) return 10 + ( c - 'a' );
				if ( c >= 'A' ) return 10 + ( c - 'A' );
			}
			return c - '0';
		};

		// Parse the body.
		//
		while ( !view.empty() )
		{
			if ( view.front() == '.' )
			{
				if constexpr ( FloatingPoint<T> )
				{
					view.remove_prefix( 1 );

					T mantissa = 0;
					while ( !view.empty() )
					{
						mantissa += parse_digit( view.back() );
						mantissa /= base;
						view.remove_suffix( 1 );
					}
					value += mantissa;
				}
				break;
			}
			else if ( view.front() == ' ' )
				break;

			value *= base;
			value += parse_digit( view.front() );
			view.remove_prefix( 1 );
		}
		return value * sign;
	}
	template<typename T = uint64_t, String S1 = std::string_view> requires ( Integral<T> || FloatingPoint<T> )
	static constexpr T parse_number( S1&& str, int default_base = 10 )
	{
		string_view_t<S1> view{ str };
		return parse_number_v<T, string_unit_t<S1>>( view, default_base );
	}
};

// Hex decoding string literal.
//
#if !GNU_COMPILER
std::array<uint8_t, 1> operator""_hex( const char* str, size_t n );
#else
template<typename T, T... chars>
constexpr auto operator""_hex()
{
	constexpr char str[] = { char(chars)... };
	static_assert( !( sizeof...( chars ) & 1 ), "Invalid hex digest." );

	constexpr auto read_digit = [ ] ( char c ) -> uint8_t
	{
		if ( c >= 'a' ) return 10 + ( c - 'a' );
		if ( c >= 'A' ) return 10 + ( c - 'A' );
		return c - '0';
	};

	std::array<uint8_t, sizeof( str ) / 2> result = {};
	for ( size_t n = 0; n != result.size(); n++ )
		result[ n ] = ( read_digit( str[ n * 2 ] ) << 4 ) | read_digit( str[ 1 + n * 2 ] );
	return result;
}
#endif

// Hash literals.
//
#if !GNU_COMPILER
	#define MAKE_HASHER( op, fn )                                                \
	inline constexpr auto operator"" op( const char* str, size_t n )             \
	{                                                                            \
		return fn<std::basic_string_view<char>>{}( { str, n } );                  \
	}                                                                            \
	inline constexpr auto operator"" op( const wchar_t* str, size_t n )          \
	{                                                                            \
		return fn<std::basic_string_view<wchar_t>>{}( { str, n } );               \
	}                                                                    
#else
	#define MAKE_HASHER( op, fn )                                                               \
	template<typename T, T... chars>                                                            \
	constexpr auto operator"" op()                                                              \
	{                                                                                           \
		constexpr T str[] = { chars... };                                                        \
		constexpr auto hash = fn<std::basic_string_view<T>>{}( { str, sizeof...( chars ) } );    \
		return (decltype(hash)){ xstd::const_tag<hash.as64()>::value };                          \
	}
#endif
	MAKE_HASHER( _ihash, xstd::ihash );
	MAKE_HASHER( _xhash, xstd::xhash );
#undef MAKE_HASHER