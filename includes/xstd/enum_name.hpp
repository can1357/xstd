#pragma once
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include "intrinsics.hpp"
#include "type_helpers.hpp"

namespace xstd
{
	// Used to generate names for enum types.
	//
	template<typename T, typename = void> requires Enum<T>
	struct enum_name
	{
#ifdef __INTELLISENSE__
		static constexpr int64_t iteration_limit = 1;
#else
		static constexpr int64_t iteration_limit = 256;
#endif

		// Type characteristics.
		//
		using value_type = std::underlying_type_t<T>;
		static constexpr bool is_signed = Signed<value_type>;
		using ex_value_type = std::conditional_t<Signed<value_type>, int64_t, uint64_t>;

		// Generates the name for the given enum.
		//
		template<T Q>
		static constexpr std::string_view generate()
		{
			std::string_view name = const_tag<Q>::to_string();
			if ( name[ 0 ] == '(' || uint8_t( name[ 0 ] - '0' ) <= 9 || name[ 0 ] == '-' )
				return std::string_view{};
			size_t n = name.rfind( ':' );
			if ( n != std::string::npos )
				name.remove_prefix( n + 1 );
			return name;
		}

		static constexpr int64_t min_value = Signed<value_type> ? ( generate<T( -2 )>().empty() ? -1 : -127 ) : 0;
		static constexpr std::array<std::string_view, iteration_limit> linear_series = make_constant_series<iteration_limit>(
			[ ] <auto V> ( const_tag<V> ) { return generate<T( V + min_value )>(); }
		);

		// String conversion.
		//
		static constexpr std::string_view try_resolve( T v )
		{
			int64_t index = ( value_type( v ) - min_value );
			if ( 0 <= index && index < iteration_limit )
				return linear_series[ index ];
			else
				return {};
		}
		static std::string resolve( T v )
		{
			auto sv = try_resolve( v );
			if ( sv.empty() )
				return std::to_string( ( value_type ) v );
			else
				return std::string{ sv };
		}
	};

	// Simple wrapper around enum_name for convenience.
	//
	template<Enum T>
	static constexpr std::string_view try_name_enum( T value )
	{
		return enum_name<T>::try_resolve( value );
	}
	template<Enum T>
	static std::string name_enum( T value )
	{
		return enum_name<T>::resolve( value );
	}
};