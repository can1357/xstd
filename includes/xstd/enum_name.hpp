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
		static constexpr size_t iteration_limit = 1;
#else
		static constexpr size_t iteration_limit = 256;
#endif

		// Type characteristics.
		//
		using value_type = std::underlying_type_t<T>;
		static constexpr bool is_signed = Signed<value_type>;
		using ex_value_type = std::conditional_t<is_signed, int64_t, uint64_t>;

		static constexpr int min_value = is_signed ? -(int)( iteration_limit / 2 ) : 0;
		static constexpr int max_value = is_signed ? +(int)( iteration_limit / 2 ) : (int)( iteration_limit );
		
		// Generates the name for the given enum.
		//
		template<T Q>
		static constexpr std::pair<std::string_view, bool> generate()
		{
			std::string_view name = const_tag<Q>::to_string();
			if ( name[ 0 ] == '(' || uint8_t( name[ 0 ] - '0' ) <= 9 )
				return { "", false };
			size_t n = name.rfind( ':' );
			if ( n != std::string::npos )
				name.remove_prefix( n + 1 );
			return { name, true };
		}

		// String conversion at runtime.
		//
		static std::string resolve( T v )
		{
			ex_value_type value = ( ex_value_type ) v;

			// If value within iteration range, try the linear list:
			//
			if ( min_value <= value && value < max_value )
			{
				static constexpr auto linear_series = make_constant_series<iteration_limit>(
					[ ] ( auto tag ) { return generate<T( decltype( tag )::value + min_value )>(); }
				);
				ex_value_type adjusted = value - min_value;
				if ( ex_value_type( 0 ) <= adjusted && adjusted < ex_value_type( iteration_limit ) )
				{
					auto& [str, valid] = linear_series[ adjusted ];
					if ( valid ) return std::string{ str.begin(), str.end() };
				}
			}
			// If not and type is not signed, try interpreting it as a flag combination:
			//
			if constexpr ( !is_signed )
			{
				static constexpr auto flag_series = make_constant_series<sizeof( value_type ) * 8>(
					[ ] ( auto tag ) { return generate<T( 1ull << decltype( tag )::value )>(); }
				);

				std::string name;
				for ( size_t i = 0; value; i++, value >>= 1 )
				{
					if ( value & 1 )
					{
						if ( auto& [str, valid] = flag_series[ i ]; valid ) 
							name += std::string{ str.begin(), str.end() } + "|";
						else                           
							return std::to_string( ( value_type ) v );
					}
				}
				if ( !name.empty() ) return name.substr( 0, name.length() - 1 );
			}
			return std::to_string( value );
		}
	};

	// Simple wrapper around enum_name for convenience.
	//
	template<Enum T>
	static std::string name_enum( T value )
	{
		return enum_name<T>::resolve( value );
	}
};