#pragma once
#include <math.h>
#include <string>
#include "zip.hpp"
#include "algorithm.hpp"
#include "formatting.hpp"
#include "type_helpers.hpp"

// Trivial types with useful explict formatting wrappers.
//
namespace xstd::fmt
{
	// Explicit integer formatting.
	//
	template<size_t N = 0, Integral T = uint64_t>
	struct binary
	{
		static constexpr size_t _N = N ? N : sizeof( T ) * 8;

		T value = 0;
		constexpr binary() {}
		constexpr binary( T value ) : value( value ) {}
		std::string to_string() const
		{
			std::string result( _N, '0' );
			for ( size_t i = 0; i != _N; i++ )
				if ( ( uint64_t( value ) >> i ) & 1 )
					result[ _N - ( i + 1 ) ] = '1';
			return result;
		}
	};
	template<Integral T>
	struct decimal
	{
		T value = 0;
		constexpr decimal() {}
		constexpr decimal( T value ) : value( value ) {}
		std::string to_string() const
		{
			return std::to_string( value );
		}
	};
	template<Integral T>
	struct hexadecimal
	{
		T value = 0;
		constexpr hexadecimal() {}
		constexpr hexadecimal( T value ) : value( value ) {}

		std::string to_string() const
		{
			return fmt::hex( value );
		}
	};

	// Explicit memory/file size formatting.
	//
	template<Integral T = size_t>
	struct byte_count
	{
		inline static const std::array unit_abbrv = {
			"b",  "kb",
			"mb", "gb",
			"tb", "pb"
		};
		inline static const std::array unit_size = xstd::make_constant_series<std::size( unit_abbrv )>( [ ] ( auto n )
		{
			size_t r = 1;
			for( size_t i = 0; i != n.value; i++ )
				r *= 1024;
			return r;
		} );


		T value = 0;
		constexpr byte_count() {}
		constexpr byte_count( T value ) : value( value ) {}
		
		std::string to_string() const
		{
			size_t limit = 1ull << ( 10 * ( unit_abbrv.size() - 1 ) );
			for ( size_t n = unit_abbrv.size() - 1; n != 0; n--, limit >>= 10 )
			{
				if ( value >= limit )
					return fmt::str( "%.1lf", double( value ) / limit ) + unit_abbrv[ n ];
			}
			return fmt::str( "%llub", value );
		}
	};

	// Explicit character formatting.
	//
	template<Integral T = char>
	struct character
	{
		T value = '\x0';
		constexpr character() {}
		constexpr character( T value ) : value( value ) {}

		std::string to_string() const
		{
			if ( !value ) return {};
			else          return std::string( 1, ( char ) value );
		}
	};

	// Explicit percentage formatting.
	//
	template<FloatingPoint T = double>
	struct percentage
	{
		T value = 0.0f;
		constexpr percentage() {}
		constexpr percentage( T value ) : value( value ) {}
		
		// Additional constructor for ratio.
		//
		template<Integral I1, Integral I2>
		constexpr percentage( I1 a, I2 b ) : value( T(a)/T(b) ) {}

		// Negatable.
		//
		constexpr percentage operator-() const { return { 1.0f - value }; }

		std::string to_string() const
		{
			return fmt::str( "%.2lf%%", double( value * 100 ) );
		}
	};

	// Explicit enum naming.
	//
	template<Enum T>
	struct named_enum
	{
		T value = {};
		constexpr named_enum() {}
		constexpr named_enum( T value ) : value( value ) {}

		std::string to_string() const
		{
			return enum_name<T>::resolve( value );
		}
	};
};
