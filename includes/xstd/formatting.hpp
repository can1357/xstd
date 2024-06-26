#pragma once
#include <string>
#include <cstring>
#include <ctype.h>
#include <cstdio>
#include <type_traits>
#include <exception>
#include <optional>
#include <tuple>
#include <variant>
#include <cwchar>
#include <memory>
#include <filesystem>
#include <numeric>
#include <string_view>
#include "type_helpers.hpp"
#include "time.hpp"
#include "intrinsics.hpp"
#include "bitwise.hpp"
#include "enum_name.hpp"
#include "utf.hpp"
#include "small_vector.hpp"
#include "hexdump.hpp"

// [[Configuration]]
// Macro wrapping ANSI escape codes, can be replaced by '#define ANSI_ESCAPE(...)' in legacy Windows to disable colors completely.
//
#ifndef ANSI_ESCAPE
	#define ANSI_ESCAPE(...) ("\x1B[" __VA_ARGS__)
#endif

// Ignore format warnings.
//
#if GNU_COMPILER
	 #pragma GCC diagnostic ignored "-Wformat"
#endif

namespace xstd::fmt
{
	namespace impl
	{
		// Types that will require no explicit conversion to string when being passed to *printf will pass.
		//
		template<typename T>
		concept CFormatStringArgument = 
			Fundamental<T> || Enum<T>  ||
			Pointer<T>     || Array<T>;
		template<typename T>
		concept ValidFormatStringArgument = 
			CFormatStringArgument<T> || 
			Same<std::string, T> ||
			Same<std::u8string, T>;

		template<typename... Tx> struct conv_count { static constexpr size_t value = ( ( ValidFormatStringArgument<Tx> ? 0 : 1 ) + ... ); };
		template<> struct conv_count<> { static constexpr size_t value = 0; };

		template<typename... Tx> 
		using format_buffer_for = std::conditional_t<(conv_count<Tx...>::value==0), std::monostate, small_vector<std::string, conv_count<Tx...>::value>>;
	};
	namespace impl
	{
		// Fixes the type name to be more friendly.
		//
		inline static std::string fix_type_name( std::string in )
		{
			static const std::string_view remove_list[] = {
				"struct ",
				"class ",
				"enum "
			};
			for ( auto& str : remove_list )
			{
				if ( in.starts_with( str ) )
					return fix_type_name( in.substr( str.length() ) );

				for ( size_t i = 0; i != in.size(); i++ )
					if ( in[ i ] == '<' && in.substr( i + 1 ).starts_with( str ) )
						in = in.substr( 0, i + 1 ) + in.substr( i + 1 + str.length() );
			}
			return in;
		}
	};

	// Prints an unsigned hexadecimal number with a prefix and optionally no leading zeroes.
	//
	template<bool LeadingZeroes, Unsigned I>
	FORCE_INLINE inline static std::string print_ux( I x, std::string_view pfx = {} )
	{
		// Calculate the digit count, if no leading zeroes requested, shift it so that BSWAP will not discard the result.
		//
		constexpr bitcnt_t max_digits = bitcnt_t( sizeof( I ) * 2 );
		bitcnt_t digits = max_digits;
		if constexpr ( !LeadingZeroes )
		{
			digits = 1 + ( msb( x | 0xF ) / 4 );
			x = rotr( x, digits * 4 );
		}

		// Allocate the string.
		//
		assume( digits <= max_digits );
		std::string result( digits + pfx.size(), '\0' );

		// Copy the prefix.
		//
		auto it = result.data();
		for ( size_t n = 0; n != pfx.size(); n++ )
			*it++ = pfx[ n ];

		// Dump to a char array.
		//
		auto chars = as_hex_array( bswap( x ) );

		// If leading zeroes requested, simply store the vector.
		//
		if constexpr ( LeadingZeroes )
		{
			store_misaligned( it, chars );
		}
		// Otherwise, copy the requested digit count.
		//
		else
		{
			assume( digits <= max_digits );
			for ( bitcnt_t i = 0; i != digits; i++ )
				*it++ = chars[ i ];
		}
		return result;
	}
	template<bool LeadingZeroes, Integral I>
	inline std::string print_ix( I value )
	{
		if constexpr ( Unsigned<I> )
		{
			return print_ux<LeadingZeroes, I>( value, "0x" );
		}
		else
		{
			using U = std::make_unsigned_t<I>;
			std::string_view pfx = "-0x";
			if ( Signed<I> && value < 0 ) value = -value;
			else                          pfx.remove_prefix( 1 );
			return print_ux<LeadingZeroes, U>( ( U ) value, pfx );
		}
	}

	// Prints a pointer.
	//
	inline static std::string print_pointer( any_ptr ptr )
	{
		return print_ux<true>( ptr.address );
	}

	// Formats the integer into a hexadecimal.
	//
	template<Integral I> inline std::string hex( I value ) { return print_ix<false>( value ); }
	template<Integral I> inline std::string hex_lz( I value ) { return print_ix<true>( value ); }

	// Formats the integer into a signed hexadecimal with explicit + if positive.
	//
	inline std::string offset( int64_t value )
	{
		char pfx[ 4 ] = { 0, ' ', '0', 'x' };
		pfx[ 0 ] = value < 0 ? '-' : '+';
		return print_ux<false>( ( uint64_t ) value, std::string_view{ std::begin( pfx ), std::end( pfx ) } );
	}

	// Returns the type name of the object passed, dynamic type name will
	// redirect to static type name if RTTI is not supported.
	//
	template<typename T>
	static std::string_view static_type_name()
	{
#if HAS_RTTI
		static const std::string value = impl::fix_type_name( compiler_demangle_type_name( typeid( T ) ) );
#else
		static constexpr auto str = type_tag<T>::to_string();
		static const std::string value = std::string{ str };
#endif
		return value;
	}
	template<typename T>
	static std::string dynamic_type_name( const T& o )
	{
#if HAS_RTTI
		return impl::fix_type_name( compiler_demangle_type_name( typeid( o ) ) );
#else
		return std::string{ "???" };
#endif
	}

	// XSTD string-convertable types implement [std::string T::to_string() const] or override the string_formatter.
	//
	namespace impl
	{
		template<typename T> 
		concept FormattableByMember = requires( const T& v ) { v.to_string(); };
	};
	template<typename T> 
	struct string_formatter;
	template<impl::FormattableByMember T>
	struct string_formatter<T>
	{
		FORCE_INLINE inline std::string operator()( const T& value ) const { return value.to_string(); }
	};
	template<typename T> 
	concept CustomStringConvertible = requires( const T& v ) { string_formatter<T>{}( v ); };
	template<typename T>
	concept StdStringConvertible = requires( T v ) { std::to_string( v ); };

	// Converts any given object to a string.
	//
	template<typename T>
	inline auto as_string( T&& x );
	template<typename T>
	concept StringConvertible = NonVoid<decltype( as_string( std::declval<T>() ) )>;
	
	template<StringConvertible... Tx> requires ( sizeof...( Tx ) != 1 )
	FORCE_INLINE inline std::string as_string( Tx&&... args )
	{
		if constexpr ( sizeof...( Tx ) != 0 )
		{
			std::string result = ( ( ' ' + as_string<Tx>( std::forward<Tx>( args ) ) + ',' ) + ... );
			result.front() = '{';
			result.back() =  '}';
			return result;
		}
		return "{}";
	}

	// Implement converters for STL wrappers.
	//
	template<>
	struct string_formatter<std::monostate> {
		FORCE_INLINE inline std::string operator()( std::monostate ) const { return as_string(); }
	};
	template<>
	struct string_formatter<std::nullopt_t> {
		FORCE_INLINE inline std::string operator()( std::nullopt_t ) const { return "nullopt"; }
	};
	template<StringConvertible... Tx>
	struct string_formatter<std::variant<Tx...>>
	{
		FORCE_INLINE inline std::string operator()( const std::variant<Tx...>& value ) const
		{ 
			if constexpr ( sizeof...( Tx ) == 0 )
			{
				return as_string();
			}
			else
			{
				return std::visit( [ ] ( const auto& arg )
				{
					return as_string( arg );
				}, value );
			}
		}
	};
	template<StringConvertible T1, StringConvertible T2>
	struct string_formatter<std::pair<T1, T2>>
	{
		FORCE_INLINE inline std::string operator()( const std::pair<T1, T2>& value ) const
		{
			return as_string( value.first, value.second );
		}
	};
	template<StringConvertible... Tx>
	struct string_formatter<std::tuple<Tx...>>
	{
		FORCE_INLINE inline std::string operator()( const std::tuple<Tx...>& value ) const
		{
			return std::apply( [ ] ( const auto&... args ) { return as_string( args... ); }, value );
		}
	};
	template<StringConvertible T>
	struct string_formatter<std::reference_wrapper<T>>
	{
		FORCE_INLINE inline std::string operator()( const std::reference_wrapper<T>& value ) const
		{
			return as_string( value.get() );
		}
	};
	template<StringConvertible T>
	struct string_formatter<std::optional<T>>
	{
		FORCE_INLINE inline std::string operator()( const std::optional<T>& value ) const
		{
			if ( value ) return as_string( value.value() );
			else         return as_string( std::nullopt );
		}
	};
	template<StringConvertible T>
	struct string_formatter<std::atomic<T>>
	{
		FORCE_INLINE inline std::string operator()( const std::atomic<T>& value ) const
		{
			return as_string( value.load( std::memory_order::relaxed ) );
		}
	};
	template<StringConvertible T>
	struct string_formatter<std::atomic_ref<T>>
	{
		FORCE_INLINE inline std::string operator()( const std::atomic_ref<T>& value ) const
		{
			return as_string( value.load( std::memory_order::relaxed ) );
		}
	};
	template<StringConvertible T>
	struct string_formatter<std::shared_ptr<T>>
	{
		FORCE_INLINE inline std::string operator()( const std::shared_ptr<T>& value ) const
		{
			return as_string( value.get() );
		}
	};
	template<StringConvertible T>
	struct string_formatter<std::unique_ptr<T>>
	{
		FORCE_INLINE inline std::string operator()( const std::unique_ptr<T>& value ) const
		{
			return as_string( value.get() );
		}
	};
	template<StringConvertible T>
	struct string_formatter<std::weak_ptr<T>>
	{
		FORCE_INLINE inline std::string operator()( const std::weak_ptr<T>& value ) const
		{
			return as_string( value.lock() );
		}
	};
	template<Duration D>
	struct string_formatter<D>
	{
		FORCE_INLINE inline std::string operator()( D value ) const
		{
			return time::to_string( value );
		}
	};
	template<>
	struct string_formatter<std::filesystem::directory_entry>
	{
		FORCE_INLINE inline std::string operator()( const std::filesystem::directory_entry& value ) const
		{
			return value.path().string();
		}
	};
	template<>
	struct string_formatter<std::filesystem::path>
	{
		FORCE_INLINE inline std::string operator()( const std::filesystem::path& value ) const
		{
			return value.string();
		}
	};
	template<typename T> requires HasBase<T, std::exception>
	struct string_formatter<T>
	{
		FORCE_INLINE inline std::string operator()( const T& value ) const
		{
			return value.what();
		}
	};
	template<typename T, size_t N>
	struct string_formatter<xvec<T, N>>
	{
		FORCE_INLINE inline std::string operator()( xvec<T, N> value ) const
		{
			return as_string( value.to_array() );
		}
	};

	// Converts any given object to a string.
	//
	template<typename T>
	inline auto as_string( T&& x )
	{
		using base_type = std::decay_t<T>;

		// Custom conversion.
		//
		if constexpr ( CustomStringConvertible<base_type> )
			return string_formatter<base_type>{}( std::forward<T>( x ) );
		// String and langauge primitives:
		//
		else if constexpr ( Same<std::string, base_type> )
			return std::string{ std::forward<T>( x ) };
		else if constexpr ( CppString<base_type> || CppStringView<base_type> )
			return utf_convert<char>( std::forward<T>( x ) );
		else if constexpr ( CString<base_type> )
			return utf_convert<char>( string_view_t<base_type>{ x } );
		else if constexpr ( Enum<base_type> )
			return enum_name<base_type>::resolve( x );
		else if constexpr ( Same<base_type, any_ptr> )
			return print_pointer( x );
		else if constexpr ( Same<base_type, int64_t> || Same<base_type, uint64_t> )
			return hex( x );
		else if constexpr ( Same<base_type, bool> )
			return x ? std::string{ "true" } : std::string{ "false" };
		else if constexpr ( Same<base_type, uint8_t> )
		{
			std::string buf( 4, '\x0' );
			buf[ 0 ] = '0';
			buf[ 1 ] = 'x';
			print_hex_digit( &buf[ 2 ], x, true );
			return buf;
		}
		else if constexpr ( Same<base_type, char> )
		{
			if ( isgraph( x ) )
			{
				std::string s( 2 + 1, '\'' );
				s[ 1 ] = x;
				return s;
			}
			else
			{
				std::string s( 2 + 3, '\'' );
				s[ 1 ] = '\\';
				print_hex_digit( &s[ 2 ], x, false );
				return s;
			}
		}
		else if constexpr ( StdStringConvertible<T> )
		{
			return std::to_string( x );
		}
		// Pointers:
		//
		else if constexpr ( Pointer<base_type> )
		{
			using E = typename std::pointer_traits<base_type>::element_type;
			if constexpr ( !Pointer<std::decay_t<E>> && StringConvertible<E> )
			{
				if ( x ) return as_string( *x );
				else     return std::string{ "nullptr" };
			}
			else
			{
				return print_pointer( x );
			}
		}
		// Containers:
		//
		else if constexpr ( Iterable<T> )
		{
			if constexpr ( StringConvertible<iterable_val_t<T>> )
			{
				std::string items = {};
				for ( auto&& entry : std::forward<T>( x ) )
					items += as_string( std::forward<decltype(entry)>( entry ) ) + ", ";
				if ( !items.empty() ) items.resize( items.size() - 2 );
				return '{' + items + '}';
			}
			else
			{
				return; // void
			}
		}
		// Finally check for tiable types.
		//
		else if constexpr ( Tiable<base_type> )
		{
			return as_string( make_mutable( x ).tie() );
		}
		else
		{
			return; // void
		}
	}

	// Used to allow use of any type in combination with "%(l)s".
	//
	template<typename B, typename T>
	inline auto fix_parameter( B& buffer, T&& x ) {
		using base_type = std::decay_t<T>;
		
		if constexpr ( CustomStringConvertible<base_type> )
			return buffer.emplace_back( string_formatter<base_type>{}( std::forward<T>( x ) ) ).data();
		else if constexpr ( impl::CFormatStringArgument<base_type> )
			return x;
		else if constexpr ( Same<any_ptr, base_type> )
			return ( void* ) x.address;
		else if constexpr ( Same<std::string, base_type> || Same<std::u8string, base_type> )
			return x.data();
		else if constexpr ( StringConvertible<base_type> )
			return buffer.emplace_back( as_string( std::forward<T>( x ) ) ).data();
		else
			return type_tag<T>{}; // Error.
	}

	// nprintf wrapper.
	//
	template<typename... T>
	FORCE_INLINE static int nsprintf( char* o, size_t n, const char* f, T... args ) {
		return snprintf( o, n, f, args... );
	}
	template<typename... T>
	FORCE_INLINE static int nsprintf( wchar_t* o, size_t n, const wchar_t* f, T... args ) {
		return swprintf( o, n, f, args... );
	}

	// Returns a formatted string.
	//
	template<typename C, typename... Tx>
	FORCE_INLINE static std::basic_string_view<C> into( std::span<C> buffer, const C* fmt_str, Tx&&... ps ) {
		using char_traits = std::char_traits<C>;

		if constexpr ( sizeof...( Tx ) > 0 ) {
			auto print_to_buffer = [&]( const C* fmt_str, auto&&... args ) FORCE_INLINE {
				int lim = (int) (intptr_t) ( buffer.size() - 1 );
				int n =   nsprintf( buffer.data(), buffer.size(), fmt_str, args... );
				n = std::clamp( n, 0, lim );
				return std::basic_string_view<C>{ buffer.data(), buffer.data() + n };
			};
			impl::format_buffer_for<std::decay_t<Tx>...> buf = {};
			return print_to_buffer( fmt_str, fix_parameter( buf, std::forward<Tx>( ps ) )... );
		} else {
			size_t n = char_traits::length( fmt_str );
			n = std::min( n, buffer.size() );
			char_traits::copy( buffer.data(), fmt_str, n );
			return { buffer.data(), buffer.data() + n };
		}
	}
	template<typename C, size_t N, typename... Tx>
	FORCE_INLINE static std::basic_string_view<C> into( C( &buffer )[ N ], const C* fmt_str, Tx&&... ps ) {
		return into( std::span{ &buffer[ 0 ], N }, fmt_str, std::forward<Tx>( ps )... );
	}
	template<typename C, typename F, typename... Tx> requires Invocable<F, C*, size_t>
	FORCE_INLINE static std::basic_string_view<C> into( F&& allocator, const C* fmt_str, Tx&&... ps ) {
		using char_traits = std::char_traits<C>;

		if constexpr ( sizeof...( Tx ) > 0 ) {
			auto print_to_buffer = [&]( const C* fmt_str, auto&&... args ) FORCE_INLINE {
				C small_buffer[ 32 ];
				int n = nsprintf( &small_buffer[ 0 ], 32, fmt_str, args... );
				if ( n < 0 ) n = 0;
				
				C* buffer = allocator( size_t( n ) );
				std::basic_string_view<C> result = { buffer, buffer + n };
				if ( n <= 31 ) {
					char_traits::copy( buffer, small_buffer, size_t( n + 1 ) );
				} else {
					nsprintf( buffer, n + 1, fmt_str, args... );
				}
				return result;
			};
			impl::format_buffer_for<std::decay_t<Tx>...> buf = {};
			return print_to_buffer( fmt_str, fix_parameter( buf, std::forward<Tx>( ps ) )... );
		} else {
			size_t n = char_traits::length( fmt_str );
			C* buffer = allocator( n );
			char_traits::copy( buffer, fmt_str, n );
			return { buffer, buffer + n };
		}
	}
	template<typename... Tx>
	inline std::string str( const char* fmt_str, Tx&&... ps ) {
		std::string result;
		into( [ & ]( size_t n ) FORCE_INLINE { result.resize( n ); return result.data(); }, fmt_str, std::forward<Tx>( ps )... );
		return result;
	}
	template<typename... Tx>
	inline std::wstring wstr( const wchar_t* fmt_str, Tx&&... ps ) { 
		std::wstring result;
		into( [ & ]( size_t n ) FORCE_INLINE { result.resize( n ); return result.data(); }, fmt_str, std::forward<Tx>( ps )... );
		return result;
	}
};
#undef HAS_RTTI

// Export the concepts.
//
namespace xstd
{
	using fmt::CustomStringConvertible;
	using fmt::StdStringConvertible;
	using fmt::StringConvertible;
};
