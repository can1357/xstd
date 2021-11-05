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
		concept CFormatStringArgument = 
			std::is_fundamental_v<T> || std::is_enum_v<T>  ||
			std::is_pointer_v<T>     || std::is_array_v<T>;
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
			digits = ( ( x ? msb( x ) : 1 ) + 3 ) / 4;
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
		auto chars = cexpr_hex_dump( bswap( x ) );

		// If leading zeroes requested, simply store the vector.
		//
		if constexpr ( !LeadingZeroes )
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
		using U = std::make_unsigned_t<I>;
		std::string_view pfx = "-0x";
		if ( Signed<I> && value < 0 ) value = -value;
		else                          pfx.remove_prefix( 1 );
		return print_ux<LeadingZeroes, U>( ( U ) value, pfx );
	}

	// Prints a pointer.
	//
	inline static std::string print_pointer( xstd::any_ptr ptr )
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
	concept StringConvertible = NonVoid<decltype(as_string( std::declval<T>() ))>;

	// Implement converters for STL wrappers.
	//
	template<>
	struct string_formatter<std::monostate>
	{
		FORCE_INLINE inline std::string operator()( std::monostate ) const { return "{}"; }
	};
	template<StringConvertible... Tx>
	struct string_formatter<std::variant<Tx...>>
	{
		FORCE_INLINE inline std::string operator()( const std::variant<Tx...>& value ) const
		{ 
			if constexpr ( sizeof...( Tx ) == 0 )
			{
				return as_string( std::monostate{} );
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
			return as_string( value.first ) + ", " as_string( value.second );
		}
	};
	template<StringConvertible... Tx>
	struct string_formatter<std::tuple<Tx...>>
	{
		FORCE_INLINE inline std::string operator()( const std::tuple<Tx...>& value ) const
		{
			if constexpr ( sizeof...( Tx ) == 0 )
			{
				return as_string( std::monostate{} );
			}
			else
			{
				std::string res = std::apply( [ ] ( auto&&... args )
				{
					return ( ( as_string( args ) + ", " ) + ... );
				}, tuple );

				std::copy( res.begin(), res.end() - 1, res.begin() + 1 );
				res.front() = '{';
				res.back() = '}';
				return res;
			}
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
			else         return "nullopt";
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
			return time::to_string( x );
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
	template<typename T> requires std::is_base_of_v<T, std::exception>
	struct string_formatter<T>
	{
		FORCE_INLINE inline std::string operator()( const T& value ) const
		{
			return value.what();
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
		{
			return string_formatter<base_type>{}( std::forward<T>( x ) );
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

			auto it = result.data() + result.size();
			if ( result.size() != 2 )
				*--it = ' ';
			*--it = '}';
			return result;
		}
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
			if constexpr ( !Pointer<E> && StringConvertible<E> )
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
					items += as_string( std::forward<decltype(entry)>( entry ) ) += ", ";
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
	template<typename T, typename... Tx>
	FORCE_INLINE inline auto as_string( T&& f, Tx&&... r )
	{
		std::string result = as_string( std::forward<T>( f ) );
		result += ", ";
		result += as_string( std::forward<Tx>( r )... );
		return result;
	}

	// Used to allow use of any type in combination with "%(l)s".
	//
	template<typename B, typename T>
	inline auto fix_parameter( B& buffer, T&& x )
	{
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

	// Returns a formatted string.
	//
	template<typename C, typename F, typename... Tx>
	FORCE_INLINE inline std::basic_string<C> vstr( F&& provider, const C* fmt_str, Tx&&... ps )
	{
		if constexpr ( sizeof...( Tx ) > 0 )
		{
			auto print_to_buffer = [ & ] ( const C* fmt_str, auto&&... args )
			{
				static constexpr size_t small_capacity = 15 / sizeof( C );
				std::basic_string<C> buffer( small_capacity, C{} );
				buffer.resize( provider( buffer.data(), buffer.size() + 1, fmt_str, args... ) );
				if ( buffer.size() > small_capacity )
					provider( buffer.data(), buffer.size() + 1, fmt_str, args... );
				return buffer;
			};

			impl::format_buffer_for<std::decay_t<Tx>...> buf = {};
			return print_to_buffer( fmt_str, fix_parameter( buf, std::forward<Tx>( ps ) )... );
		}
		else
		{
			return fmt_str;
		}
	}
	template<typename... Tx>
	inline std::string str( const char* fmt_str, Tx&&... ps )
	{ 
		return vstr<char>( [ ] <typename... T> ( char* o, size_t n, const char* f, T... args ) { return snprintf( o, n, f, args... ); }, fmt_str, std::forward<Tx>( ps )... );
	}
	template<typename... Tx>
	inline std::wstring wstr( const wchar_t* fmt_str, Tx&&... ps )
	{ 
		return vstr<wchar_t>( [] <typename... T> ( wchar_t* o, size_t n, const wchar_t* f, T... args ) { return swprintf( o, n, f, args... ); }, fmt_str, std::forward<Tx>( ps )... );
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
