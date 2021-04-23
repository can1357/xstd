#pragma once
#include <array>
#include <string>
#include "text.hpp"
#include "bitwise.hpp"

namespace xstd
{
	// OID helper.
	//
	struct oid
	{
		// Traits.
		//
		static constexpr size_t max_length = 32;
		
		// Tag used when dispatching string with known length.
		//
		struct string_literal_t {};

		// Raw data.
		//
		size_t length = 0;
		std::array<uint8_t, max_length> data = { 0 };

		// Construction from a string.
		//
		inline constexpr oid( const char* str, size_t n, string_literal_t _ )
		{
			// Until we reach the end of the string:
			//
			uint32_t prev = 0;
			bool first = true;
			for ( size_t i = 0; i < n; i++ )
			{
				// Decode an integer.
				//
				uint32_t value = 0;
				while ( i != n && str[ i ] != '.' )
				{
					value *= 10;
					value += str[ i ] - '0';
					i++;
				}

				// Handle the special case with the first two digits.
				//
				if ( first )
				{
					// Validate root.
					//
					if ( value > 2 )
					{
						length = 0;
						break;
					}
					prev = value;
					first = false;
					continue;
				}
				else if ( !first && prev )
				{
					// Validate child.
					//
					if ( prev != 2 && value > 40 )
					{
						length = 0;
						break;
					}
					value += prev * 40;
					prev = 0;
				}

				// Append the value, construct null on failure.
				//
				if ( !push_back( value ) )
				{
					length = 0;
					break;
				}
			}
		}
		inline constexpr oid( const char* str ) : oid( str, xstd::strlen( str ), oid::string_literal_t{} ) {}

		// Constructon from bytes.
		//
		inline constexpr oid( const uint8_t* data, size_t length ) : length( length )
		{
			std::copy_n( data, std::min( length, max_length ), this->data.begin() );
			if ( length > max_length )
			{
				length = max_length;
				while ( length && ( this->data[ length - 1 ] & 0x80 ) )
					length--;
			}
		}
		inline constexpr oid( const std::initializer_list<uint8_t>& data ) : oid( data.begin(), data.end() - data.begin() ) {}

		// Default construction / copy.
		//
		inline constexpr oid() = default;
		inline constexpr oid( const oid& ) = default;

		// Appends a digit.
		//
		inline constexpr bool push_back( uint32_t value )
		{
			// Determine the required length.
			//
			uint8_t required_length = 0;
			for ( auto it = value; it != 0; it >>= 7 )
				required_length++;

			// Fail on overflow.
			//
			if ( ( length + required_length ) > max_length )
				return false;

			// Encode in a single byte if possible.
			//
			if ( required_length == 1 )
			{
				data[ length++ ] = value;
				return true;
			}

			// Encode all digits.
			//
			for ( auto j = 0; j != required_length; j++ )
			{
				uint8_t enc = ( j + 1 ) == required_length ? 0 : 1 << 7;
				enc |= ( value >> ( ( required_length - j - 1 ) * 7 ) ) & fill_bits( 7 );
				data[ length++ ] = enc;
			}
			return true;
		}

		// Formatting.
		//
		std::string to_string() const
		{
			std::string result = {};
			auto str_end = data.begin() + length;
			for( auto it = data.begin(); it != str_end; )
			{
				// Find the ending.
				//
				auto end = it + 1;
				if ( *it & 0x80 )
				{
					end = std::find_if( it, str_end, [ ] ( auto& val ) { return !( val & 0x80 ); } );
					if ( end != str_end )
						++end;
				}

				// Decode the integer.
				//
				uint32_t value = 0;
				size_t length = end - it;
				for ( size_t n = 0; n != length; n++ )
					value |= ( it[ n ] & fill_bits( 7 ) ) << ( ( length - n - 1 ) * 7 );

				// Handle the special case with the first two digits.
				//
				if ( it == data.begin() )
				{
					uint32_t root = value / 40;
					if ( root >= 2 ) root = 2;
					result += std::to_string( root );
					result += ".";
					result += std::to_string( value - ( root * 40 ) );
					result += ".";
				}
				else
				{
					result += std::to_string( value );
					result += ".";
				}

				// Skip to the next instance.
				//
				it = end;
			}
			if ( !result.empty() )
				result.pop_back();
			return result;
		}

		// Prefix checks.
		//
		inline constexpr bool starts_with( const oid& other )
		{
			if ( other.size() < size() )
				return false;
			for ( size_t n = 0; n != size(); n++ )
				if ( other.data[ n ] != data[ n ] )
					return false;
			return true;
		}

		// Comparison operators.
		//
		inline constexpr bool operator<( const oid& o ) const
		{
			if ( length < o.length )      return true;
			else if ( length > o.length ) return false;
			for ( size_t n = 0; n != length; n++ )
			{
				if ( data[ n ] < o.data[ n ] )
					return true;
				if ( data[ n ] > o.data[ n ] )
					return false;
			}
			return false;
		}
		inline constexpr bool operator==( const oid& o ) const { return length == o.length && std::equal( data.begin(), data.begin() + length, o.begin() ); }
		inline constexpr bool operator!=( const oid& o ) const { return length != o.length || !std::equal( data.begin(), data.begin() + length, o.begin() ); }

		// Range of the raw tag.
		//
		inline constexpr const uint8_t* begin() const { return &data[ 0 ]; }
		inline constexpr const uint8_t* end() const { return &data[ length ]; }
		inline constexpr size_t size() const { return length; }
	};
};

// OID literals.
//
static constexpr xstd::oid operator ""_oid( const char* str, size_t n ) { return { str, n, xstd::oid::string_literal_t{} }; }
