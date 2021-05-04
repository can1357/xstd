#pragma once
#include <numeric>
#include <string>
#include <string_view>
#include "type_helpers.hpp"

// Implements a simple hex dump.
//
namespace xstd::fmt
{
	struct hex_dump_config
	{
		char delimiter = ' ';
		bool ascii = false;
		size_t row_length = std::numeric_limits<size_t>::max();
		bool uppercase = true;
	};

	// Prints a singular hexadecimal digit to the buffer given.
	//
	template<typename C>
	inline static constexpr void print_hex_digit( C* out, uint8_t value, bool uppercase = false )
	{
		for ( size_t n = 0; n != 2; n++ )
		{
			uint8_t r = ( value >> ( 4 - ( n * 4 ) ) ) & 0xF;
			if ( r <= 9 )
				out[ n ] = '0' + r;
			else
				out[ n ] = ( r - 0xA ) + ( uppercase ? 'A' : 'a' );
		}
	}

	// Optimized hexdump for constant size input with no configuration.
	//
	template<typename T, typename C = char>
	inline static std::string const_hex_dump( const T& value )
	{
		std::basic_string<C> out( sizeof( T ) * 2, '\x0' );
		C* p = out.data();
		auto* i = ( const uint8_t* ) &value;
		for ( size_t n = 0; n != sizeof( T ); n++ )
			print_hex_digit( p + n * 2, i[ n ] );
		return out;
	}
	template<typename T, typename C = char>
	inline static constexpr std::array<C, 2*sizeof(T)> cexpr_hex_dump( const T& value )
	{
		std::array<C, sizeof( T ) * 2> result = {};
		auto bytes = bit_cast<std::array<uint8_t, sizeof(T)>>( value );
		for ( size_t n = 0; n != sizeof( T ); n++ )
			print_hex_digit( &result[ n * 2 ], bytes[ n ] );
		return result;
	}

	// Returns the hexdump of the given range according to the configuration.
	//
	template<Iterable C> requires ( sizeof( iterator_value_type_t<C> ) == 1 )
	inline static std::string hex_dump( C&& container, hex_dump_config cfg = {} )
	{
		// Calculate container limits and normalize row length.
		//
		auto it = std::begin( container );
		auto end = std::end( container );
		size_t length = end - it;
		cfg.row_length = std::min( length, cfg.row_length );

		// Allocate storage and reserve an approximate size.
		//
		std::string result = {};
		result.reserve( length * ( 2 + bool( cfg.delimiter != 0 ) + bool( cfg.ascii ) ) );
		
		// Print row by low:
		//
		auto add_delimiter = [ & ] ( char c, size_t n ) { if ( c ) result.insert( result.end(), n, c ); };
		for ( size_t n = 0; n < length; n += cfg.row_length )
		{
			auto it2 = it;
			for ( size_t j = 0; j != cfg.row_length; j++ )
			{
				if ( it2 == end )
				{
					add_delimiter( cfg.delimiter, 2 );
				}
				else
				{
					size_t t = result.size();
					result.resize( t + 2 );
					print_hex_digit( &result[ t ], *it2++, cfg.uppercase );
				}
				if ( j != ( cfg.row_length - 1 ) )
					add_delimiter( cfg.delimiter, 1 );
			}

			if ( cfg.ascii )
			{
				add_delimiter( cfg.delimiter, 4 );

				it2 = it;
				for ( size_t j = 0; j != cfg.row_length; j++ )
				{
					if ( it2 == end )
					{
						add_delimiter( cfg.delimiter, 1 );
					}
					else
					{
						char c = char( *it2++ );
						result += isprint( c ) ? c : '.';
					}
					if ( j != ( cfg.row_length - 1 ) )
						add_delimiter( cfg.delimiter, 1 );
				}
			}
			it = it2;

			if ( ( n + cfg.row_length ) < length )
				add_delimiter( '\n', 1 );
		}
		return result;
	}
	inline static std::string hex_dump( xstd::any_ptr p, size_t n, hex_dump_config cfg = {} )
	{
		return hex_dump( std::string_view{ ( const char* ) p, n }, cfg );
	}
};