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
#include "enum_name.hpp"
#include "fields.hpp"
#include "utf.hpp"
#include "small_vector.hpp"
#include "hexdump.hpp"

// [Configuration]
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
		concept ValidFormatStringArgument = 
			std::is_fundamental_v<std::remove_cvref_t<T>> || std::is_enum_v<std::remove_cvref_t<T>>  ||
			std::is_pointer_v<std::remove_cvref_t<T>>     || std::is_array_v<std::remove_cvref_t<T>> || 
			std::is_same_v<std::string, std::remove_cvref_t<T>>;

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

		// Prints a pointer.
		//
		inline static std::string print_pointer( xstd::any_ptr ptr )
		{
			std::string buf( 16, '\x0' );
			for ( size_t n = 0; n != 8; n++ )
			{
				uint64_t digit = ptr >> ( ( 7 - n ) * 8 );
				print_hex_digit( &buf[ n * 2 ], uint8_t( digit ), true );
			}
			return buf;
		}

		// Prints a hexadecimal number.
		//
		inline static std::string print_u64x( uint64_t num, std::string_view pfx = "0x" )
		{
			std::string buf( 16, '\x0' );
			for ( size_t n = 0; n != 8; n++ )
			{
				uint64_t digit = num >> ( ( 7 - n ) * 8 );
				print_hex_digit( &buf[ n * 2 ], uint8_t( digit ), true );
			}
			size_t p = buf.find_first_not_of( '0' );
			if ( p == std::string::npos )
				buf.resize( 1 );
			else
				buf.erase( buf.begin(), buf.begin() + p );
			buf.insert( buf.begin(), pfx.begin(), pfx.end() );
			return buf;
		}
	};

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

	// XSTD string-convertable types implement [std::string T::to_string() const] or override the custom_string_converter.
	//
	template<typename T, typename = void> struct custom_string_converter;
	template<typename T> struct custom_string_converter<T> { inline constexpr void operator()( const T& ) const noexcept {} };
	template<typename T> concept CustomStringConvertibleMember = requires( T v ) { v.to_string(); };
	template<typename T> concept CustomStringConvertibleExternal = String<decltype( custom_string_converter<std::decay_t<T>>{}( std::declval<T>() ) )>;
	template<typename T> concept CustomStringConvertible = CustomStringConvertibleExternal<T> || CustomStringConvertibleMember<T>;

	// Checks if std::to_string is specialized to convert type into string.
	//
	template<typename T>
	concept StdStringConvertible = requires( T v ) { std::to_string( v ); };

	// Converts any given object to a string.
	//
	template<typename T>
	static auto as_string( const T& x );
	template<typename T>
	concept StringConvertible = requires( std::string res, T v ) { res = as_string( v ); };

	template<typename T>
	NO_INLINE static auto as_string( const T& x )
	{
		using base_type = std::decay_t<T>;

		// Custom conversion.
		//
		if constexpr ( CustomStringConvertibleMember<T> )
		{
			return x.to_string();
		}
		else if constexpr ( CustomStringConvertibleExternal<base_type> )
		{
			return custom_string_converter<base_type>{}( x );
		}
		else if constexpr ( FieldMappable<base_type> )
		{
			using field_list = typename base_type::field_list;

			std::string result = { '{', ' ' };
			make_constant_series<std::tuple_size_v<field_list>>( [ & ] <size_t N> ( const_tag<N> )
			{
				using E = std::tuple_element_t<N, field_list>;

				if constexpr ( !E::is_function )
				{
					auto&& value = E::get( x );
					if constexpr ( StringConvertible<decltype( value )> )
					{
						const char* volatile name = &E::name[ 0 ];
						result += name;
						result += ": ";
						result += as_string( value );
						result += ", ";
					}
				}
			} );
			if ( result.size() > 2 )
				result.erase( result.end() - 2, result.end() );
			result += " }";
			return result;
		}
		// String and langauge primitives:
		//
		else if constexpr ( std::is_same_v<std::string, base_type> )
		{
			return x;
		}
		else if constexpr ( CppString<base_type> || CppStringView<base_type> )
		{
			return utf_convert<char>( x );
		}
		else if constexpr ( CString<base_type> )
		{
			return utf_convert<char>( string_view_t<base_type>{ x } );
		}
		else if constexpr ( Enum<T> )
		{
			return enum_name<T>::resolve( x );
		}
		else if constexpr ( std::is_same_v<any_ptr, base_type> )
		{
			return impl::print_pointer( x );
		}
		else if constexpr ( std::is_same_v<base_type, int64_t> || std::is_same_v<base_type, uint64_t> )
		{
			if constexpr ( std::is_signed_v<base_type> )
			{
				if ( x < 0 )
					return impl::print_u64x( -x, "-0x" );
			}
			return impl::print_u64x( x, "0x" );
		}
		else if constexpr ( std::is_same_v<base_type, bool> )
		{
			return x ? std::string{ "true" } : std::string{ "false" };
		}
		else if constexpr ( std::is_same_v<base_type, char> )
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
		else if constexpr ( Atomic<base_type> )
		{
			if constexpr ( StringConvertible<typename base_type::value_type> )
				return as_string( x.load() );
			else return type_tag<T>{};
		}
		// Pointers:
		//
		else if constexpr ( is_specialization_v<std::shared_ptr, base_type> || is_specialization_v<std::unique_ptr, base_type> || std::is_pointer_v<base_type> )
		{
			if constexpr ( StringConvertible<typename std::pointer_traits<base_type>::element_type> )
			{
				if ( x ) return as_string( *x );
				else     return std::string{ "nullptr" };
			}
			else if constexpr ( std::is_pointer_v<base_type> )
			{
				return impl::print_pointer( x );
			}
			else return type_tag<T>{};
		}
		else if constexpr ( is_specialization_v<std::weak_ptr, base_type> )
		{
			return as_string( x.lock() );
		}
		// Misc types:
		//
		else if constexpr ( Duration<T> )
		{
			return time::to_string( x );
		}
		else if constexpr ( std::is_base_of_v<std::exception, T> )
		{
			return std::string{ x.what() };
		}
		else if constexpr ( std::is_same_v<base_type, std::filesystem::directory_entry> )
		{
			return x.path().string();
		}
		else if constexpr ( std::is_same_v<base_type, std::monostate> )
		{
			return std::string{ "{}" };
		}
		else if constexpr ( std::is_same_v<base_type, std::filesystem::path> )
		{
			return x.string();
		}
		// Containers:
		//
		else if constexpr ( is_specialization_v<std::pair, base_type> )
		{
			if constexpr ( StringConvertible<decltype( x.first )> && StringConvertible<decltype( x.second )> )
				return '(' + as_string( x.first ) + std::string{ ", " } + as_string( x.second ) + ')';
			else return type_tag<T>{};
		}
		else if constexpr ( is_specialization_v<std::tuple, base_type> )
		{
			constexpr bool is_tuple_str_cvtable = [ ] ()
			{
				bool cvtable = true;
				if constexpr ( std::tuple_size_v<base_type> > 0 )
				{
					make_constant_series<std::tuple_size_v<base_type>>( [ & ] ( auto tag )
					{
						if constexpr ( !StringConvertible<std::tuple_element_t<decltype( tag )::value, base_type>> )
							cvtable = false;
					} );
				}
				return cvtable;
			}( );

			if constexpr ( std::tuple_size_v<base_type> == 0 )
				return std::string{ "{}" };
			else if constexpr ( is_tuple_str_cvtable )
			{
				std::string res = std::apply( [ ] ( auto&&... args )
				{
					return ( ( as_string( args ) + ", " ) + ... );
				}, x );
				return '{' + res.substr( 0, res.length() - 2 ) + '}';
			}
			else return type_tag<T>{};
		}
		else if constexpr ( is_specialization_v<std::variant, base_type> )
		{
			constexpr bool is_variant_str_cvtable = [ ] ()
			{
				bool cvtable = true;
				if constexpr ( std::variant_size_v<base_type> > 0 )
				{
					make_constant_series<std::variant_size_v<base_type>>( [ & ] ( auto tag )
					{
						if constexpr ( !StringConvertible<std::variant_alternative_t<decltype( tag )::value, base_type>> )
							cvtable = false;
					} );
				}
				return cvtable;
			}();

			if constexpr ( std::variant_size_v<base_type> == 0 )
				return std::string{ "{}" };
			else if constexpr ( is_variant_str_cvtable )
			{
				return "{" + std::visit( [ ] ( auto&& arg )
				{
					return as_string( arg );
				}, x ) + "}";
			}
			else return type_tag<T>{};
		}
		else if constexpr ( is_specialization_v<std::optional, base_type> )
		{
			if constexpr ( StringConvertible<decltype( x.value() )> )
			{
				if ( x.has_value() )
					return as_string( x.value() );
				else
					return std::string{ "nullopt" };
			}
			else return type_tag<T>{};
		}
		else if constexpr ( is_specialization_v<std::reference_wrapper, base_type> )
		{
			if constexpr ( StringConvertible<decltype( x.get() )> )
				return as_string( x.get() );
			else return type_tag<T>{};
		}
		else if constexpr ( Iterable<T> )
		{
			if constexpr ( StringConvertible<iterator_value_type_t<T>> )
			{
				std::string items = {};
				for ( auto&& entry : x )
					items += as_string( entry ) + std::string{ ", " };
				if ( !items.empty() ) items.resize( items.size() - 2 );
				return '{' + items + '}';
			}
			else return type_tag<T>{};
		}
		// Finally check for tiable types.
		//
		else if constexpr ( Tiable<T> )
		{
			return as_string( make_mutable( x ).tie() );
		}
		else return type_tag<T>{};
	}
	template<typename T, typename... Tx>
	__forceinline static auto as_string( const T& f, const Tx&... r )
	{
		return as_string( f ) + ", " + as_string( r... );
	}

	// Used to allow use of any type in combination with "%(l)s".
	//
	template<typename B, typename T>
	__forceinline static auto fix_parameter( B& buffer, T&& x )
	{
		using base_type = std::decay_t<T>;
		
		// If custom string convertible:
		//
		if constexpr ( CustomStringConvertibleExternal<base_type> )
		{
			return buffer.emplace_back( as_string( std::forward<T>( x ) ) ).data();
		}
		// If fundamental type, return as is.
		//
		else if constexpr ( std::is_fundamental_v<base_type> || std::is_enum_v<base_type> || 
					   std::is_pointer_v<base_type> || std::is_array_v<base_type>  )
		{
			return x;
		}
		else if constexpr ( Atomic<base_type> )
		{
			return fix_parameter( buffer, x.load() );
		}
		else if constexpr ( std::is_same_v<any_ptr, base_type> )
		{
			return ( void* ) x.address;
		}
		// If it is a basic ASCII string:
		//
		else if constexpr ( std::is_same_v<std::string, base_type> )
		{
			return x.data();
		}
		// If string convertible:
		//
		else if constexpr ( StringConvertible<T> )
		{
			return buffer.emplace_back( as_string( std::forward<T>( x ) ) ).data();
		}
		// If none matched, forcefully convert into [type @ pointer].
		//
		else
		{
			return buffer.emplace_back( '[' + dynamic_type_name( x ) + '@' + as_string( &x ) + ']' ).data();
		}
	}

	// Returns a formatted string.
	//
	template<typename C, typename F, typename... Tx>
	__forceinline static std::basic_string<C> vstr( F&& provider, const C* fmt_str, Tx&&... ps )
	{
		if constexpr ( sizeof...( Tx ) > 0 )
		{
			auto print_to_buffer = [ & ] ( const C* fmt_str, auto&&... args )
			{
				std::basic_string<C> buffer( 15, C{} );
				buffer.resize( provider( buffer.data(), buffer.size() + 1, fmt_str, args... ) );
				if ( buffer.size() > 15 )
					provider( buffer.data(), buffer.size() + 1, fmt_str, args... );
				return buffer;
			};

			impl::format_buffer_for<Tx...> buf = {};
			return print_to_buffer( fmt_str, fix_parameter( buf, std::forward<Tx>( ps ) )... );
		}
		else
		{
			return fmt_str;
		}
	}
	template<typename... Tx>
	__forceinline static std::string str( const char* fmt_str, Tx&&... ps )
	{ 
		return vstr<char>( [ ] <typename... T> ( char* o, size_t n, const char* f, T... args ) { return snprintf( o, n, f, args... ); }, fmt_str, std::forward<Tx>( ps )... );
	}
	template<typename... Tx>
	__forceinline static std::wstring wstr( const wchar_t* fmt_str, Tx&&... ps )
	{ 
		return vstr<wchar_t>( [] <typename... T> ( wchar_t* o, size_t n, const wchar_t* f, T... args ) { return swprintf( o, n, f, args... ); }, fmt_str, std::forward<Tx>( ps )... );
	}

	// Formats the integer into a signed hexadecimal.
	//
	template<Integral T>
	static std::string hex( T value )
	{
		if constexpr ( !std::is_signed_v<T> )
		{
			return impl::print_u64x( value, "0x" );
		}
		else
		{
			if ( value >= 0 ) return impl::print_u64x( value, "0x" );
			else              return impl::print_u64x( -value, "-0x" );
		}
	}

	// Formats the integer into a signed hexadecimal with explicit + if positive.
	//
	inline static std::string offset( int64_t value )
	{
		if ( value >= 0 ) return impl::print_u64x( value, "+ 0x" );
		else              return impl::print_u64x( -value, "- 0x" );
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
