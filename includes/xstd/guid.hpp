#pragma once
#include <tuple>
#include <string>
#include "intrinsics.hpp"
#include "hexdump.hpp"
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

		// Manual construction from the GUID value.
		//
		constexpr guid( uint64_t low, uint64_t high ) : low( low ), high( high ) {}
		constexpr guid( const uint8_t( &value )[ 16 ] ) : low( 0 ), high( 0 )
		{
			for ( size_t n = 0; n != 8; n++ )
				low |= uint64_t( value[ n ] ) << ( 8 * n );
			for ( size_t n = 0; n != 8; n++ )
				high |= uint64_t( value[ n + 8 ] ) << ( 8 * n );
		}

		// Constructed by any hashable type as a constant expression.
		//
		template<typename T> requires ( !Same<std::decay_t<T>, guid> )
		constexpr guid( const T& obj ) : low( make_hash<fnv64>( 0x49c54a9166f5c01cull, obj ).as64() ), high( make_hash<fnv64>( 0x7b0b6b0f8933b6a5ull, obj ).as64() ) {}

		// Static helper that guarantees const evaluation.
		//
		template<typename T>
		static _CONSTEVAL guid constant( const T& obj ) { return guid{ obj }; }

		// Constructs a GUID from string.
		// - Must be string_length.
		//
		template<typename C>
		static constexpr guid from( C&& str )
		{
			auto read = [ & ] <typename T> ( type_tag<T>, size_t offset ) FORCE_INLINE -> T
			{
				constexpr size_t nibbles = sizeof( T ) * 2;
				T value = 0;
				for ( size_t i = 0; i != nibbles; i++ )
				{
					char c = ( char ) str[ offset + nibbles - 1 - i ];
					if ( c < 'A' ) c = ( c - '0' );
					else           c = ( ( c | 0x20 ) - 'a' ) + 0xA;
					value |= T( uint8_t( c ) ) << ( i * 4 );
				}
				return value;
			};

			uint64_t low = read( type_tag<uint32_t>{}, 0 );
			low |= uint64_t( read( type_tag<uint16_t>{}, 9 ) ) << 32;
			low |= uint64_t( read( type_tag<uint16_t>{}, 14 ) ) << 48;

			uint64_t high = bswap( read( type_tag<uint16_t>{}, 19 ) );
			high |= uint64_t( bswap( read( type_tag<uint16_t>{}, 24 ) ) ) << 16;
			high |= uint64_t( bswap( read( type_tag<uint32_t>{}, 28 ) ) ) << 32;
			return { low, high };
		}

		// Validates a guid string.
		//
		template<Iterator C>
		static constexpr bool validate( C str )
		{
			auto take_hex = [ & ] ( size_t n ) FORCE_INLINE
			{
				for ( ; n; n-- )
				{
					auto character = *str++;
					if ( '0' <= character && character <= '9' )
						continue;
					if ( 'a' <= character && character <= 'f' )
						continue;
					if ( 'A' <= character && character <= 'F' )
						continue;
					return false;
				}
				return true;
			};
			return take_hex( 8 )
				&& ( *str++ == '-' )
				&& take_hex( 4 )
				&& ( *str++ == '-' )
				&& take_hex( 4 )
				&& ( *str++ == '-' )
				&& take_hex( 4 )
				&& ( *str++ == '-' )
				&& take_hex( 12 );
		}
		template<typename C>
		static constexpr bool validate( const std::basic_string<C>& cstr ) {
			if ( cstr.size() != string_length ) {
				return false;
			}
			return validate( cstr.data() );
		}
		template<typename C>
		static constexpr bool validate( const std::basic_string_view<C>& cstr ) {
			if ( cstr.size() != string_length ) {
				return false;
			}
			return validate( cstr.data() );
		}

		// Default copy/move.
		//
		constexpr guid( guid&& ) noexcept = default;
		constexpr guid( const guid& ) = default;
		constexpr guid& operator=( guid&& ) noexcept = default;
		constexpr guid& operator=( const guid& ) = default;

		// String conversion.
		//
		template<typename C>
		constexpr void to_string( C&& iterator ) const
		{
			auto write_hex = [ & ] <typename I> ( I integer ) FORCE_INLINE {
				if constexpr ( std::is_pointer_v<decltype( iterator )> ) {
					if ( !std::is_constant_evaluated() ) {
						if constexpr ( sizeof( *iterator ) == 1 ) {
							std::array res = fmt::print_hex<false>( integer );
							*( ( decltype( res )*& ) iterator )++ = res;
							return;
						} else if constexpr ( sizeof( *iterator ) == 2 ) {
							std::array res = fmt::print_hex16<false>( integer );
							*( ( decltype( res )*& ) iterator )++ = res;
							return;
						}
					}
				}
				for ( auto c : fmt::print_hex<false>( integer ) )
					*iterator++ = c;
			};
			write_hex( bswap( uint32_t( low ) ) );
			*iterator++ = '-';
			write_hex( bswap( uint16_t( low >> 32 ) ) );
			*iterator++ = '-';
			write_hex( bswap( uint16_t( low >> 48 ) ) );
			*iterator++ = '-';
			write_hex( uint16_t( high ) );
			*iterator++ = '-';
			write_hex( uint16_t( high >> 16 ) );
			write_hex( uint32_t( high >> 32 ) );
		}
		std::string to_string() const
		{
			std::string out( string_length, '\0' );
			to_string( out.begin() );
			return out;
		}
		std::wstring to_wstring() const
		{
			std::wstring out( string_length, L'\0' );
			to_string( out.begin() );
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
// GUID literals.
// 
#if !GNU_COMPILER || defined(__INTELLISENSE__)
inline constexpr xstd::guid operator ""_guid( const char* str, size_t n )
{
	return xstd::guid::from( std::string_view{ str, str + n } );
}
#else
namespace xstd::impl
{
	template<char... chars>
	struct const_guid
	{
		inline static constexpr guid value = [ ] ()
		{
			constexpr char str[] = { chars... };
			return guid::from( std::string_view{ str, str + sizeof...( chars ) } );
		}( );
	};
};
template<typename T, T... chars>
constexpr const xstd::guid& operator""_guid()
{
	return xstd::impl::const_guid<( ( char ) chars )...>::value;
}
#endif