#pragma once
#include <numeric>
#include <string>
#include <string_view>
#include "xvector.hpp"
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
	FORCE_INLINE inline constexpr void print_hex_digit( C* out, uint8_t value, bool uppercase = false )
	{
		auto print = [ base = ( uppercase ? 'A' : 'a' ) ] (uint8_t digit) FORCE_INLINE
		{
			return digit >= 9 ? ( base + digit - 10 ) : digit + '0';
		};
		out[ 0 ] = print( value >> 4 );
		out[ 1 ] = print( value & 0xF );
	}

	// Optimized hexdump for constant size input with no configuration.
	//
	template<bool Uppercase, typename T>
	FORCE_INLINE inline constexpr std::array<char, 2 * sizeof( T )> print_hex( const T& data )
	{
		constexpr char K = ( Uppercase ? 'A' : 'a' ) - '0' - 10;

		auto x = vec::cast<uint16_t>( vec::from( as_bytes( data ) ) );
		x |= ( x << 12 );
		x >>= 4;
		x &= 0x0F0F;

		auto chars = vec::bytes( x );
		chars += '0';
		chars += ( chars > '9' ) & K;
		return chars.to_array();
	}
	template<typename T>
	FORCE_INLINE inline constexpr std::array<char, 2 * sizeof( T )> as_hex_array( const T& value )
	{
		return print_hex<true>( value );
	}
	template<typename T>
	FORCE_INLINE inline std::string as_hex_string( const T& value )
	{
		std::string out( sizeof( T ) * 2, '\x0' );
		store_misaligned( out.data(), print_hex<true>( value ) );
		return out;
	}

	// Returns the hexdump of the given range according to the configuration.
	//
	template<Iterable C> requires ( sizeof( iterable_val_t<C> ) == 1 )
	inline std::string hex_dump( C&& container, hex_dump_config cfg = {} )
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
		result.reserve( length * ( 2ull + ( cfg.delimiter != 0 ) + ( cfg.ascii ) ) );
		
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
	inline std::string hex_dump( xstd::any_ptr p, size_t n, hex_dump_config cfg = {} )
	{
		return hex_dump( std::string_view{ ( const char* ) p, n }, cfg );
	}
};