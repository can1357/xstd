#pragma once
#include "type_helpers.hpp"
#include "hashable.hpp"
#include "utf.hpp"
#include "xvector.hpp"
#include "fnv.hpp"

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
					h.template add_bytes<char>( ( char ) front );
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

		template<typename T>
		struct ceval_ascii_helpers
		{
			FORCE_INLINE static constexpr size_t length( const T& ) noexcept { return std::string::npos; }
			template<bool CaseSensitive, typename O> FORCE_INLINE static bool equals( const T&, const O* ) { unreachable(); }
		};
		template<typename T, size_t N> requires ( 2 <= N && N <= 32 )
		struct ceval_ascii_helpers<T[N]>
		{
			FORCE_INLINE static constexpr size_t length( const T( &value )[ N ] ) noexcept
			{
				bool is_ascii = true;
				for ( size_t i = 0; i != ( N - 1 ); i++ )
					is_ascii &= value[ i ] <= 0x7f;
				if ( is_ascii )
					return N - 1;
				return std::string::npos;
			}
			template<bool CaseSensitive, typename O>
			FORCE_INLINE static constexpr bool equals( const T( &value )[ N ], const O* other )
			{
				using U = convert_uint_t<O>;

				U mismatch = 0;
				for ( size_t i = 0; i != ( N - 1 ); i++ )
				{
					if constexpr ( CaseSensitive )
					{
						mismatch |= U( value[ i ] ) ^ U( other[ i ] );
					}
					else
					{
						bool alpha = ( 'a' <= value[ i ] && value[ i ] <= 'z' ) || ( 'A' <= value[ i ] && value[ i ] <= 'Z' );
						U mask = ~U( alpha ? 0x20 : 0x00 );
						mismatch |= ( U( value[ i ] ) ^ U( other[ i ] ) ) & mask;
					}
				}
				return mismatch == 0;
			}
		};
		template<typename T>
		FORCE_INLINE inline constexpr size_t ceval_ascii_length( const T& data )
		{
			return ceval_ascii_helpers<T>::length( data );
		}
		template<bool CaseSensitive, typename T, typename O>
		FORCE_INLINE inline constexpr bool ceval_ascii_equals( const T& data, const O* y )
		{
			return ceval_ascii_helpers<T>::template equals<CaseSensitive>( data, y );
		}
	};

	// Default string hashers.
	//
	using xhash_t = fnv64;
	using ihash_t = fnv64;
	using ahash_t = fnv64;

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

	// Fast case-insensitive ASCII hasher.
	//
	template<typename H, typename T = void>
	struct basic_ahash;
	template<typename H, String T>
	struct basic_ahash<H, T>
	{
		using U = string_unit_t<T>;
		using I = convert_uint_t<U>;

		inline constexpr H operator()( const T& value ) const noexcept
		{
			H h = {};
			if constexpr ( CString<T> )
			{
				const U* iterator = value;
				while ( true )
				{
					U c = *iterator++;
					if ( c )
						h.add_bytes( uint8_t( c & 0xDF ) );
					else
						break;
				}
			}
			else
			{
				std::basic_string_view<U> sv{ value };

				if constexpr ( Same<H, crc32c> || Same<H, xcrc> )
				{
					if ( !std::is_constant_evaluated() )
					{
						while ( sv.size() >= 8 )
						{
							uint64_t value;
							if constexpr ( sizeof( U ) == 1 )
							{
								value = *( const uint64_t* ) sv.data();
							}
							else
							{
								auto vec = xvec<I, 8>::load( ( const I* ) sv.data() );
								value = vec::cast<char>( vec ).template reinterpret<uint64_t>()[ 0 ];
							}
							h.add_bytes( value & 0xDFDFDFDFDFDFDFDF );
							sv.remove_prefix( 8 );
						}
					}
				}

				for ( U c : sv )
					h.add_bytes( uint8_t( c & 0xDF ) );
			}
			return h;
		}
	};
	template<typename H>
	struct basic_ahash<H, void>
	{
		template<String S>
		FORCE_INLINE inline constexpr H operator()( const S& value ) const noexcept
		{
			return basic_ahash<H, S>{}( value );
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
	template<typename T = void> using ahash = basic_ahash<ahash_t, T>;
	template<typename H, String T> inline constexpr H make_ihash( const T& value ) noexcept { return basic_ihash<H, T>{}( value ); }
	template<typename H, String T> inline constexpr H make_xhash( const T& value ) noexcept { return basic_xhash<H, T>{}( value ); }
	template<typename H, String T> inline constexpr H make_ahash( const T& value ) noexcept { return basic_ahash<H, T>{}( value ); }
	template<String T> inline constexpr ihash_t make_ihash( const T& value ) noexcept { return make_ihash<ihash_t>( value ); }
	template<String T> inline constexpr xhash_t make_xhash( const T& value ) noexcept { return make_xhash<xhash_t>( value ); }
	template<String T> inline constexpr ahash_t make_ahash( const T& value ) noexcept { return make_ahash<ahash_t>( value ); }

	// Width insensitive string operations.
	//
	template<String S1, String S2>
	FORCE_INLINE inline constexpr int xstrcmp( S1&& a, S2&& b )
	{
		// Take optimized path at libc, if same unit.
		//
		string_view_t<S1> av = { a };
		if constexpr ( Same<string_unit_t<S1>, string_unit_t<S2>> )
			return av.compare( b );

		string_view_t<S2> bv = { b };
		return utf_compare( av, bv );
	}
	template<String S1, String S2>
	FORCE_INLINE inline constexpr bool xequals( S1&& a, S2&& b )
	{
		// Take optimized path at libc, if same unit.
		//
		string_view_t<S1> av = { a };
		if constexpr ( Same<string_unit_t<S1>, string_unit_t<S2>> )
			return av.compare( b ) == 0;
	
		// Take ascii path if relevant.
		//
		if ( size_t clen = impl::ceval_ascii_length( b ); clen != std::string::npos )
			return av.size() == clen && impl::ceval_ascii_equals<true>( b, av.data() );

		string_view_t<S2> bv = { b };
		return utf_cmpeq( av, bv );
	}
	template<String S1, String S2>
	FORCE_INLINE inline constexpr size_t xfind( S1&& a, S2&& b )
	{
		// Take optimized path at libc, if same unit.
		//
		string_view_t<S1> av = { a };
		if constexpr ( Same<string_unit_t<S1>, string_unit_t<S2>> )
			return av.find( b );

		// Take ascii path if relevant.
		//
		if ( size_t clen = impl::ceval_ascii_length( b ); clen != std::string::npos )
		{
			ptrdiff_t diff = av.length() - clen;
			for ( ptrdiff_t n = 0; n <= diff; n++ )
				if ( impl::ceval_ascii_equals<true>( b, av.data() + n ) )
					return ( size_t ) n;
		}
		else
		{
			string_view_t<S2> bv = { b };
			size_t len = utf_length<string_unit_t<S1>>( bv );
			ptrdiff_t diff = av.length() - len;
			for ( ptrdiff_t n = 0; n <= diff; n++ )
				if ( xequals( av.substr( n, len ), bv ) )
					return ( size_t ) n;
		}
		return std::string::npos;
	}
	template<String S1, String S2>
	FORCE_INLINE inline constexpr bool xstarts_with( S1&& a, S2&& b )
	{
		// Take optimized path at libc, if same unit.
		//
		string_view_t<S1> av = { a };
		if constexpr ( Same<string_unit_t<S1>, string_unit_t<S2>> )
			return av.starts_with( b ) == 0;

		// Take ascii path if relevant.
		//
		if ( size_t clen = impl::ceval_ascii_length( b ); clen != std::string::npos )
		{
			if ( av.length() < clen ) [[unlikely]]
				return false;
			return impl::ceval_ascii_equals<true>( b, av.data() );
		}
		else
		{
			string_view_t<S2> bv = { b };
			size_t len = utf_length<string_unit_t<S1>>( bv );
			if ( av.length() < len ) [[unlikely]]
				return false;
			av.remove_suffix( av.length() - len );
			return xequals( av, bv );
		}
	}
	template<String S1, String S2>
	FORCE_INLINE inline constexpr bool xends_with( S1&& a, S2&& b )
	{
		// Take optimized path at libc, if same unit.
		//
		string_view_t<S1> av = { a };
		if constexpr ( Same<string_unit_t<S1>, string_unit_t<S2>> )
			return av.ends_with( b ) == 0;

		// Take ascii path if relevant.
		//
		if ( size_t clen = impl::ceval_ascii_length( b ); clen != std::string::npos )
		{
			if ( av.length() < clen ) [[unlikely]]
				return false;
			return impl::ceval_ascii_equals<true>( b, av.data() + av.size() - clen );
		}
		else
		{
			string_view_t<S2> bv = { b };
			size_t len = utf_length<string_unit_t<S1>>( bv );
			if ( av.length() < len ) [[unlikely]]
				return false;
			av.remove_prefix( av.length() - len );
			return xequals( av, bv );
		}
	}

	// Case conversion.
	//
	template<bool ToLower, typename To, typename From>
	inline std::basic_string<To> convert_case( std::basic_string_view<From> src )
	{
		size_t max_out = Same<To, From> ? src.size() + codepoint_cvt<To>::max_out : codepoint_cvt<To>::max_out * src.size();
		std::basic_string<To> result( max_out, '\0' );
		size_t length = utf_convert<To, From, true, !ToLower, ToLower>( src, result );
		shrink_resize( result, length );
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
	FORCE_INLINE inline constexpr int istrcmp( S1&& a, S2&& b )
	{
		string_view_t<S1> av = { a };
		string_view_t<S2> bv = { b };
		return utf_icompare( av, bv );
	}
	template<String S1, String S2>
	FORCE_INLINE inline constexpr bool iequals( S1&& a, S2&& b )
	{
		string_view_t<S1> av = { a };
		if ( size_t clen = impl::ceval_ascii_length( b ); clen != std::string::npos )
			return av.size() == clen && impl::ceval_ascii_equals<false>( b, av.data() );
		string_view_t<S2> bv = { b };
		return utf_icmpeq( av, bv );
	}
	template<String S1, String S2>
	FORCE_INLINE inline constexpr size_t ifind( S1&& a, S2&& b )
	{
		string_view_t<S1> av = { a };
		if ( size_t clen = impl::ceval_ascii_length( b ); clen != std::string::npos )
		{
			ptrdiff_t diff = av.length() - clen;
			for ( ptrdiff_t n = 0; n <= diff; n++ )
				if ( impl::ceval_ascii_equals<false>( b, av.data() + n ) )
					return ( size_t ) n;
		}
		else
		{
			string_view_t<S2> bv = { b };
			size_t len = utf_length<string_unit_t<S1>>( bv );
			ptrdiff_t diff = av.length() - len;
			for ( ptrdiff_t n = 0; n <= diff; n++ )
				if ( iequals( av.substr( n, len ), bv ) )
					return ( size_t ) n;
		}
		return std::string::npos;
	}
	template<String S1, String S2>
	FORCE_INLINE inline constexpr bool istarts_with( S1&& a, S2&& b )
	{
		string_view_t<S1> av = { a };
		if ( size_t clen = impl::ceval_ascii_length( b ); clen != std::string::npos )
		{
			if ( av.length() < clen ) [[unlikely]]
				return false;
			return impl::ceval_ascii_equals<false>( b, av.data() );
		}
		else
		{
			string_view_t<S2> bv = { b };
			size_t len = utf_length<string_unit_t<S1>>( bv );
			if ( av.length() < len ) [[unlikely]]
				return false;
			av.remove_suffix( av.length() - len );
			return iequals( av, bv );
		}
	}
	template<String S1, String S2>
	FORCE_INLINE inline constexpr bool iends_with( S1&& a, S2&& b )
	{
		string_view_t<S1> av = { a };
		if ( size_t clen = impl::ceval_ascii_length( b ); clen != std::string::npos )
		{
			if ( av.length() < clen ) [[unlikely]]
				return false;
			return impl::ceval_ascii_equals<false>( b, av.data() + av.size() - clen );
		}
		else
		{
			string_view_t<S2> bv = { b };
			size_t len = utf_length<string_unit_t<S1>>( bv );
			if ( av.length() < len ) [[unlikely]]
				return false;
			av.remove_prefix( av.length() - len );
			return iequals( av, bv );
		}
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
	inline constexpr T parse_number_v( std::basic_string_view<C>& view, int default_base = 10, T default_value = {} )
	{
		if ( view.empty() ) return default_value;
		
		// Parse the sign.
		//
		bool sign = false;
		if ( view.front() == '-' )
		{
			sign = true;
			view.remove_prefix( 1 );
		}
		else if ( view.front() == '+' )
		{
			view.remove_prefix( 1 );
		}

		// Find out the mode.
		//
		int base = default_base;
		if ( view.size() >= 2 && Integral<T> )
		{
			if ( view[ 0 ] == '0' && cxlower( view[ 1 ] ) == 'x' )
				base = 16, view.remove_prefix( 2 );
			else if ( view[ 0 ] == '0' && cxlower( view[ 1 ] ) == 'o' )
				base = 8, view.remove_prefix( 2 );
		}

		// Declare digit parser.
		//
		T value = 0;
		auto parse_digit = [ hex = ( base == 16 ) ] ( char c ) FORCE_INLINE -> std::optional<int>
		{
			if ( '0' <= c && c <= '9' )
				return c - '0';

			if ( hex )
			{
				c |= 0x20;
				if ( 'a' <= c && c <= 'f' )
					return 0xA + ( c - 'a' );
			}

			return std::nullopt;
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
					T mbase = 0.1f;
					while ( !view.empty() )
					{
						auto v = parse_digit( view.front() );
						if ( !v ) [[unlikely]]
							break;
						mantissa += mbase * *v;
						mbase *= 0.1f;
						view.remove_prefix( 1 );
					}
					value += mantissa;
				}
				break;
			}

			auto v = parse_digit( view.front() );
			if ( !v ) [[unlikely]]
				break;

			value *= base;
			value += *v;
			view.remove_prefix( 1 );
		}
		return sign ? -value : value;
	}
	template<typename T = uint64_t, String S1 = std::string_view> requires ( Integral<T> || FloatingPoint<T> )
	inline constexpr T parse_number( S1&& str, int default_base = 10, T default_value = {} )
	{
		string_view_t<S1> view{ str };
		return parse_number_v<T, string_unit_t<S1>>( view, default_base, default_value );
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
	MAKE_HASHER( _ahash, xstd::ahash );
#undef MAKE_HASHER