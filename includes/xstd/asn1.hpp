#pragma once
#include <vector>
#include <optional>
#include <variant>
#include <string>
#include <memory>
#include <deque>
#include <string_view>
#include "type_helpers.hpp"
#include "utf.hpp"
#include "oid.hpp"
#include "formatting.hpp"
#include "hexdump.hpp"
#include "time.hpp"

// Implements a barebones ASN1 parser.
//
namespace xstd::asn1
{
	// Specification types.
	//
	enum class identifier_class : uint8_t
	{
		universal =                 0b00,
		application_specific =      0b01,
		context_specific =          0b10,
		private_id =                0b11,
	};
	enum type_tag : uint8_t
	{
		tag_eoc,
		tag_boolean,
		tag_integer,
		tag_bit_string,
		tag_octet_string,
		tag_null,
		tag_oid,
		tag_object_descriptor,
		tag_external,
		tag_real,
		tag_enum,
		tag_pdv,
		tag_utf8_string,
		tag_sequence = 0x10,
		tag_set,
		tag_numeric_string,
		tag_printable_string,
		tag_teletex_string,
		tag_videotex_string,
		tag_ia5_string,
		tag_utc_time,
		tag_generalized_time,
		tag_graphic_string,
		tag_visible_string,
		tag_general_string,
		tag_universal_string,
		tag_bmp_string = 0x1E,
	};
	struct identifier
	{
		uint8_t          tag            : 5;
		uint8_t          is_constructed : 1;
		identifier_class tag_class      : 2;
	};

	// Decoded type tag.
	//
	struct tag
	{
		bool primitive = true;
		size_t tag_number = tag_null;
		identifier_class tag_class = identifier_class::universal;

		// Properties.
		//
		constexpr bool is_universal() const { return tag_class == identifier_class::universal; }
		constexpr bool is_app_specific() const { return tag_class == identifier_class::application_specific; }
		constexpr bool is_context_specific() const { return tag_class == identifier_class::context_specific; }
		constexpr bool is_private() const { return tag_class == identifier_class::private_id; }

		// Decodes a tag.
		//
		static constexpr std::optional<tag> decode( std::string_view& range )
		{
			if ( range.empty() )
				return std::nullopt;

			tag result = {};
			auto id = bit_cast< identifier >( range.front() );
			range.remove_prefix( 1 );
			result.primitive = !id.is_constructed;
			result.tag_class = id.tag_class;
			if ( id.tag == 0x1F )
			{
				result.tag_number = 0;
				while ( true )
				{
					if ( range.empty() )
						return std::nullopt;
					result.tag_number <<= 7;

					uint8_t val = ( uint8_t ) range.front();
					range.remove_prefix( 1 );
					result.tag_number |= val & 0x7F;
					if ( !( val & 0x80 ) )
						break;
				}
			}
			else
			{
				result.tag_number = id.tag;
			}
			return result;
		}
	};

	// Object body.
	//
	struct object
	{
		// Link to the parent object.
		//
		object* parent = nullptr;

		// Reference to the source range described.
		//
		std::string_view source = {};
		size_t header_length = 0;

		// Tag of the object.
		//
		tag tag_value = {};

		// Child objects.
		//
		bool encapsulating = false;
		std::vector<object*> children = {};

		// Raw data if primitive.
		//
		std::vector<uint8_t> raw_data = {};

		// If root, arena in which all children get allocated.
		//
		std::unique_ptr<std::deque<object>> arena = {};

		// Type checks.
		//
		bool is_boolean() const { return tag_value.is_universal() && tag_value.tag_number == tag_boolean; }
		bool is_integer() const { return tag_value.is_universal() && tag_value.tag_number == tag_integer; }
		bool is_enum() const { return tag_value.is_universal() && tag_value.tag_number == tag_enum; }
		bool is_oid() const { return tag_value.is_universal() && tag_value.tag_number == tag_oid; }
		bool is_null() const { return tag_value.is_universal() && tag_value.tag_number == tag_null; }
		bool is_set() const { return tag_value.is_universal() && tag_value.tag_number == tag_set; }
		bool is_sequence() const { return tag_value.is_universal() && tag_value.tag_number == tag_sequence; }
		bool is_timepoint() const { return tag_value.is_universal() && ( tag_value.tag_number == tag_generalized_time || tag_value.tag_number == tag_utc_time ); }
		bool is_string() const 
		{ 
			if ( !tag_value.is_universal() )
				return false;
			switch ( tag_value.tag_number )
			{
				case tag_bit_string:
				case tag_octet_string:
				case tag_utf8_string:
				case tag_numeric_string:
				case tag_printable_string:
				case tag_teletex_string:
				case tag_videotex_string:
				case tag_ia5_string:
				case tag_visible_string:
				case tag_general_string:
				case tag_bmp_string:
					return true;
				default:
					return false;
			}
		}

		// Readers for primitive types.
		//
		template<typename T>
		T as() const
		{
			// Boolean.
			//
			if constexpr ( Same<T, bool> )
			{
				return raw_data.size() && raw_data[ 0 ] != 0;
			}
			// Integer.
			//
			else if constexpr ( Integral<T> || Enum<T> )
			{
				uint64_t value = 0;
				size_t length = std::min( sizeof( value ), raw_data.size() );
				std::copy_n( raw_data.data(), length, std::reverse_iterator( ( ( uint8_t* ) &value ) + length ) );

				if constexpr ( Unsigned<T> )
					return T( value );
				else
					return T( sign_extend( value, 8 * length ) );
			}
			// String.
			//
			else if constexpr ( CppString<T> )
			{
				if ( tag_value.tag_number == tag_bmp_string )
				{
					std::wstring swapped = { ( wchar_t* ) raw_data.data(), raw_data.size() / sizeof( wchar_t ) };
					for ( auto& res : swapped )
						res = bswap( res );
					return utf_convert<string_unit_t<T>>( std::move( swapped ) );
				}
				else
					return utf_convert<string_unit_t<T>>( std::string_view{ ( char* ) raw_data.data(), raw_data.size() } );
			}
			// OID.
			//
			else if constexpr ( Same<oid, T> )
			{
				return oid{ raw_data.data(), raw_data.size() };
			}
			// Time/date.
			//
			else if constexpr ( Timestamp<T> || Duration<T> )
			{
				auto get_digit = [ &, i = 0u ] () mutable
				{
					if ( raw_data.size() < ( i + 2 ) )
						return 0;
					if ( raw_data[ i ] == 'Z' || raw_data[ i + 1 ] == 'Z' ||
						 raw_data[ i ] == '.' || raw_data[ i + 1 ] == '.' )
						return 0;
					auto val = ( raw_data[ i + 1 ] - '0' ) + ( raw_data[ i ] - '0' ) * 10;
					i += 2;
					return val;
				};

				int y = 0;
				if ( tag_value.tag_number == tag_generalized_time )
					y = get_digit() * 100 + get_digit();
				else
					y = 2000 + get_digit();
				int m = get_digit();
				int d = get_digit();

				y -= m <= 2;
				uint32_t era = y / 400;
				uint32_t yoe = uint32_t( y - era * 400 );
				uint32_t doy = ( 153 * ( m + ( m > 2 ? -3 : 9 ) ) + 2 ) / 5 + d - 1;
				uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;

				time::seconds result = ( era * 146097 + int32_t( doe ) - 719468 ) * 24h;
				result += get_digit() * 1h;
				result += get_digit() * 1min;
				result += get_digit() * 1s;

				if constexpr ( Duration<T> )
					return std::chrono::duration_cast< T >( result );
				else
					return T( std::chrono::duration_cast< typename T::clock::duration >( result ) );
			}
			else
			{
				static_assert( sizeof( T ) == -1, "Unrecognized type." );
			}
		}

		// Linear iteration.
		//
		auto begin() const { return children.begin(); }
		auto end() const { return children.end(); }
		size_t size() const { return children.size(); }

		// Non-recursive enumeration helper.
		//
		template<typename F> requires Invocable<F, void, object*>
		void enumerate( const F& func ) const
		{
			std::vector<object*> stack;
			for ( auto it = children.rbegin(); it != children.rend(); ++it )
				stack.emplace_back( *it );
			while( !stack.empty() )
			{
				// Pop the top item.
				//
				auto top = stack.back();
				stack.pop_back();

				// Push children.
				//
				for ( auto it = top->children.rbegin(); it != top->children.rend(); ++it )
					stack.emplace_back( *it );

				// Invoke the callback.
				//
				func( top );
			}
		}

		// Dumps to a string.
		//
		std::string dump() const
		{
			std::string result = {};

			auto name_tag = [ & ] ()
			{
				switch ( tag_value.tag_class )
				{
					case identifier_class::universal:            return fmt::str( "Universal(0x%llx)", tag_value.tag_number );
					case identifier_class::application_specific: return fmt::str( "Application Specific(0x%llx)", tag_value.tag_number );
					case identifier_class::context_specific:     return fmt::str( "Context Specific(0x%llx)", tag_value.tag_number );
					case identifier_class::private_id:           return fmt::str( "Private(0x%llx)", tag_value.tag_number );
					default: unreachable();
				}
			};

			if ( tag_value.primitive && !encapsulating )
			{
				if ( is_boolean() )
					result += as<bool>() ? "true\n" : "false\n";
				else if ( is_null() )
					result += "null\n";
				else if ( is_enum() )
					result += fmt::str( "Enum(0x%llx)\n", as<size_t>() );
				else if ( is_integer() )
				{
					if ( raw_data.size() > 8 )
					{
						result += "0x";
						for( auto& b : raw_data )
							result += fmt::str( "%02x", b );
						result += '\n';
					}
					else
					{
						result += fmt::str( "0x%llx\n", as<size_t>() );
					}
				}
				else if ( is_timepoint() )
					result += fmt::str( "{ Epoch + %llu }\n", as<time::seconds>().count() );
				else if ( is_oid() )
					result += as<oid>().to_string() + "\n";
				else if ( is_string() )
				{
					std::string str = as<std::string>();
					bool is_graphic = true;
					for ( auto it = str.begin(); it != str.end() && is_graphic; it++ )
						is_graphic &= isprint( *it ) || isspace( *it );
					if ( str.size() > 64 )
						str.resize( 65 );

					if ( !is_graphic )
						str = fmt::hex_dump( str );

					if ( is_graphic )
					{
						if ( str.size() > 64 )
							result += fmt::str( "'%s...'\n", str );
						else
							result += fmt::str( "'%s'\n", str );
					}
					else
					{
						if ( str.size() > 64 )
							result += fmt::str( "[%s...]\n", str );
						else
							result += fmt::str( "[%s]\n", str );
					}
				}
				else
				{
					std::string_view view{ ( char* ) raw_data.data(), raw_data.size() };
					if ( view.size() > 64 )
						result += name_tag() + " [" + fmt::hex_dump( view.substr( 0, 33 ) ) + "...]\n";
					else
						result += name_tag() + " [" + fmt::hex_dump( view ) + "]\n";
				}
			}
			else
			{
				if ( encapsulating )
					result += "Encapsulated<>:\n";
				else if ( is_set() )
					result += "Set[]:\n";
				else if ( is_sequence() )
					result += "Sequence<>:\n";
				else
					result += name_tag() + ":\n";
				for ( auto& child : children )
				{
					const char pfx[] = " |   ";
					result += " |--> ";
					auto cres = child->dump();
					for ( auto it = cres.end() - 2; it != cres.begin(); --it )
					{
						if ( *it == '\n' )
						{
							it = cres.insert( it + 1, std::begin( pfx ), std::end( pfx ) - 1 );
							--it;
						}
					}
					result += cres;
				}
			}
			return result;
		}
	};

	// Internal decoder.
	//
	namespace impl {
		static object* decode_into( object* root, object* result, std::string_view& range ) {
			// Decode the tag.
			//
			result->source = range;
			if ( auto tag = tag::decode( range ); !tag )
				return nullptr;
			else
				result->tag_value = tag.value();

			// Decode the length.
			//
			size_t length;
			if ( range.empty() )
				return nullptr;
			uint8_t val = ( uint8_t ) range.front();
			range.remove_prefix( 1 );

			// Definite short form:
			//
			if ( !( val & 0x80 ) )
			{
				length = val;
			}
			// Indefinite long form:
			//
			else if ( !( val & 0x7F ) )
			{
				length = std::string::npos;
			}
			// Definite long form:
			//
			else
			{
				size_t bytes = val & 0x7F;
				if ( range.size() < bytes )
					return nullptr;

				length = 0;
				for ( size_t n = 0; n != bytes; n++ )
				{
					length <<= 8;
					length |= ( uint8_t ) range[ n ];
				}
				range.remove_prefix( bytes );
			}
			result->header_length = result->source.size() - range.size();

			// Handle primitive data:
			//
			if ( result->tag_value.primitive )
			{
				// Can't be indefinite length.
				//
				if ( length == std::string::npos )
					return nullptr;

				// Read the whole body.
				//
				if ( range.size() < length )
					return nullptr;
				result->raw_data = { ( uint8_t* ) range.data(), ( uint8_t* ) range.data() + length };
				range.remove_prefix( length );

				// Bitstring / OctetString might be used to encapsulate further data.
				//
				if ( result->tag_value.is_universal() && ( result->tag_value.tag_number == tag_octet_string || result->tag_value.tag_number == tag_bit_string ) )
				{
					std::string_view sub_range = { ( char* ) result->raw_data.data(), result->raw_data.size() };
					while ( !sub_range.empty() )
					{
						if ( auto res = decode_into( root, &root->arena->emplace_back(), sub_range ); !res || res->tag_value.tag_number == tag_eoc )
						{
							result->children.clear();
							break;
						}
						else
						{
							result->children.emplace_back( res );
						}
					}
					result->encapsulating = !result->children.empty();
				}
			}
			else
			{
				// If definite:
				//
				if ( length != std::string::npos )
				{
					if ( range.size() < length )
						return nullptr;
					auto sub_range = range.substr( 0, length );
					range.remove_prefix( length );
					while ( !sub_range.empty() )
					{
						auto child = decode_into( root, &root->arena->emplace_back(), sub_range );
						if ( !child ) return nullptr;
						child->parent = result;
						result->children.emplace_back( child );
					}
				}
				// If indefinite:
				//
				else
				{
					while ( !range.empty() )
					{
						// Must have enough space to decode tag + length.
						//
						if ( range.size() < 2 )
							return nullptr;

						// If end of content:
						//
						if ( range[ 0 ] == 0 && range[ 1 ] == 0 )
						{
							range.remove_prefix( 2 );
							break;
						}

						// Otherwise, decode the element.
						//
						auto child = decode_into( root, &root->arena->emplace_back(), range );
						if ( !child ) return nullptr;
						child->parent = result;
						result->children.emplace_back( child );
					}
				}
			}
			result->source.remove_suffix( range.size() );
			return result;
		}
	};

	// Decodes an object instance, returns null on failure.
	//
	static std::unique_ptr<object> decode( std::string_view& range )
	{
		auto root = std::make_unique<object>();
		root->arena = std::make_unique<std::deque<object>>();
		if ( impl::decode_into( root.get(), root.get(), range ) ) {
			return root;
		}
		return nullptr;
	}
	static std::unique_ptr<object> decode( xstd::any_ptr ptr, size_t len )
	{
		std::string_view rng{ ( char* ) ptr, ( char* ) ptr + len };
		return decode( rng );
	}
};