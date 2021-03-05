#pragma once
#include <tuple>
#include "formatting.hpp"
#include "hashable.hpp"

namespace xstd
{
	// Defines a globally unique identifier.
	//
	struct guid
	{
		static constexpr size_t string_length = 36;

		// Store the value.
		//
		uint64_t low;
		uint64_t high;

		// Default constructor produces null.
		//
		_CONSTEVAL guid() : low( 0 ), high( 0 ) {}

		// Constructed by any hashable type as a constant expression.
		//
		template<typename T> requires ( !Same<std::decay_t<T>, guid> )
		_CONSTEVAL guid( T&& obj ) : low( make_hash( 0x49c54a9166f5c01cull, obj ).as64() ), high( make_hash( 0x7b0b6b0f8933b6a5ull, obj ).as64() ) {}

		// Default copy/move.
		//
		constexpr guid( guid&& ) noexcept = default;
		constexpr guid( const guid& ) = default;
		constexpr guid& operator=( guid&& ) noexcept = default;
		constexpr guid& operator=( const guid& ) = default;

		// String conversion.
		//
		template<typename C>
		constexpr void to_string( C&& buffer ) const
		{
			constexpr auto tochr = [ ] ( auto digit )
			{
				digit &= 0xF;
				if ( digit <= 9 ) return ( char ) ( '0' + digit );
				else              return ( char ) ( 'a' + ( digit - 0xA ) );
			};
			uint16_t high_lo = bswap( ( uint16_t ) high );
			uint64_t high_hi = bswap( high >> 16 ) >> 16;
			size_t i = 0;
			for ( int n = 32-4; n >= 0; n -= 4 )
				buffer[ i++ ] = tochr( ( low >> n ) );
			buffer[ i++ ] = '-';
			for ( int n = 48-4; n >= 32; n -= 4 )
				buffer[ i++ ] = tochr( ( low >> n ) );
			buffer[ i++ ] = '-';
			for ( int n = 64-4; n >= 48; n -= 4 )
				buffer[ i++ ] = tochr( ( low >> n ) );
			buffer[ i++ ] = '-';
			for ( int n = 16-4; n >= 0; n -= 4 )
				buffer[ i++ ] = tochr( ( high_lo >> n ) );
			buffer[ i++ ] = '-';
			for ( int n = 48-4; n >= 0; n -= 4 )
				buffer[ i++ ] = tochr( ( high_hi >> n ) );
		}
		std::string to_string() const
		{
			std::string out( string_length, '\x0' );
			to_string( out );
			return out;
		}
		std::wstring to_wstring() const
		{
			std::wstring out( string_length, L'\x0' );
			to_string( out );
			return out;
		}

		// Make tiable for serialization.
		//
		constexpr auto tie() { return std::tie( low, high ); }

		// Basic comparison operators.
		//
		constexpr bool operator==( const guid& o ) const { return low == o.low && high == o.high; }
		constexpr bool operator!=( const guid& o ) const { return low != o.low || high != o.high; }
		constexpr bool operator<( const guid& o ) const { return  o.high > high || ( o.high == high && o.low > low ); }
	};
};