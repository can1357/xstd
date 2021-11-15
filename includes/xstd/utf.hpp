#pragma once
#include <cstddef>
#include "bitwise.hpp"
#include "type_helpers.hpp"
#include "xvector.hpp"

namespace xstd
{
	struct foreign_endianness_t {};

	template<typename C, bool ForeignEndianness = false>
	struct codepoint_cvt;

	// UTF-8.
	//
	template<Char8 T, bool ForeignEndianness>
	struct codepoint_cvt<T, ForeignEndianness>
	{
		//    7 bits
		// 0xxxxxxx
		//    5 bits  6 bits
		// 110xxxxx 10xxxxxx
		//    4 bits  6 bits  6 bits
		// 1110xxxx 10xxxxxx 10xxxxxx
		//    3 bits  6 bits  6 bits  6 bits
		// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		//
		static constexpr size_t max_out = 4;

		FORCE_INLINE inline static constexpr uint8_t rlength( T _front )
		{
			uint8_t front = uint8_t( _front );
			uint8_t result = ( front >> 7 ) + 1;
			result += front >= 0b11100000;
			result += front >= 0b11110000;
			assume( result != 0 && result <= max_out );
			return result;
		}
		FORCE_INLINE inline static constexpr uint8_t length( uint32_t cp )
		{
			uint8_t result = 1;
			result += bool( cp >> 7 );
			result += bool( cp >> ( 5 + 6 ) );
			result += bool( cp >> ( 4 + 6 + 6 ) );
			assume( result != 0 && result <= max_out );
			return result;
		}
		FORCE_INLINE inline static constexpr void encode( uint32_t cp, T*& out )
		{
			// Handle single character case.
			//
			if ( cp <= 0x7F ) [[likely]]
			{
				*out++ = T( cp );
				return;
			}

			auto write = [ & ] <auto I> ( const_tag<I> ) FORCE_INLINE
			{
				auto p = out;
				
#if XSTD_HW_PDEP_PEXT
				if ( !std::is_constant_evaluated() )
				{
					uint32_t tmp = bit_pdep<uint32_t>( cp, 0b00000111'00111111'00111111'00111111 );
					tmp |= 0x80808080 | fill_bits( I, 8 - I + 8 * ( I - 1 ) );

					// BSWAP here since we do not depend on the result.
					//
					if constexpr ( I == 4 )
					{
						*( uint32_t* ) &out[ 0 ] = bswapd( tmp );
					}
					else if constexpr ( I == 3 )
					{
						out[ 0 ] = uint8_t( tmp >> 16 );
						*( uint16_t* ) &out[ 1 ] = bswapw( ( uint16_t ) tmp );
					}
					else
					{
						*( uint16_t* ) &out[ 0 ] = bswapw( ( uint16_t ) tmp );
					}

					out += I;
					return;
				}
#endif
				for ( size_t i = 0; i != I; i++ )
				{
					uint8_t flag = i ? 0x80 : fill_bits( I, 8 - I );
					uint32_t value = cp >> 6 * ( I - i - 1 );
					value &= fill_bits( i ? 6 : 7 - I );
					p[ i ] = ( T ) uint8_t( flag | value );
				}

				out += I;
			};

			if ( ( cp >> 6 ) <= fill_bits( 5 ) ) [[likely]]
				write( const_tag<2ull>{} );
			else if ( ( cp >> 12 ) <= fill_bits( 4 ) ) [[likely]]
				write( const_tag<3ull>{} );
			else
				write( const_tag<4ull>{} );
		}
		FORCE_INLINE inline static constexpr uint32_t decode( std::basic_string_view<T>& in )
		{
			T front = in[ 0 ];
			if ( int8_t( front ) >= 0 ) [[likely]]
			{
				 in.remove_prefix( 1 );
				 return front;
			}

			auto read = [ & ] <auto I> ( const_tag<I> ) FORCE_INLINE -> uint32_t
			{
				if ( in.size() < I ) [[unlikely]]
				{
					in.remove_prefix( in.size() );
					return 0;
				}
				
#if XSTD_HW_PDEP_PEXT
				if ( !std::is_constant_evaluated() )
				{
					// Do not use BSWAP here to avoid dependency on it via PEXT.
					//
					uint32_t pext_mask, tmp;
					if constexpr ( I == 4 )
					{
						pext_mask = 0b00000111'00111111'00111111'00111111;
						tmp =  uint32_t( ( uint8_t ) front )   << 24;
						tmp |= uint32_t( ( uint8_t ) in[ 1 ] ) << 16;
						tmp |= uint32_t( ( uint8_t ) in[ 2 ] ) << 8;
						tmp |= uint32_t( ( uint8_t ) in[ 3 ] ) << 0;
					}
					else if constexpr ( I == 3 )
					{
						pext_mask = 0b00000000'00001111'00111111'00111111;
						tmp =  uint32_t( ( uint8_t ) front )   << 16;
						tmp |= uint32_t( ( uint8_t ) in[ 1 ] ) << 8;
						tmp |= uint32_t( ( uint8_t ) in[ 2 ] ) << 0;
					}
					else
					{
						pext_mask = 0b00000000'00000000'00011111'00111111;
						tmp =  uint32_t( ( uint8_t ) front )   << 8;
						tmp |= uint32_t( ( uint8_t ) in[ 1 ] ) << 0;
					}
					in.remove_prefix( I );
					return bit_pext( tmp, pext_mask );
				}
#endif
				uint32_t cp = ( front & fill_bits( 7 - I ) ) << ( 6 * ( I - 1 ) );
				for ( size_t i = 1; i != I; i++ )
					cp |= uint32_t( in[ i ] & fill_bits( 6 ) ) << ( 6 * ( I - 1 - i ) );
				in.remove_prefix( I );
				return cp;
			};

			if ( uint8_t( front ) < 0b11100000 ) [[likely]]
				return read( const_tag<2ull>{} );
			if ( uint8_t( front ) < 0b11110000 ) [[likely]]
				return read( const_tag<3ull>{} );
			return read( const_tag<4ull>{} );
		}
		FORCE_INLINE inline static constexpr uint32_t decode( const T*& in, size_t limit = max_out )
		{
			std::basic_string_view<T> tmp{ in, in + limit };
			uint32_t cp = decode( tmp );
			in += ( limit - tmp.size() );
			return cp;
		}
	};

	// UTF-16.
	//
	template<Char16 T, bool ForeignEndianness>
	struct codepoint_cvt<T, ForeignEndianness>
	{
		static constexpr size_t max_out = 2;

		FORCE_INLINE inline static constexpr uint8_t rlength( T front )
		{
			if constexpr ( ForeignEndianness )
				front = ( T ) bswapw( ( uint16_t ) front );
			uint8_t result = 1 + ( ( uint16_t( front ) >> 10 ) == 0x36 );
			assume( result != 0 && result <= max_out );
			return result;
		}
		FORCE_INLINE inline static constexpr uint8_t length( uint32_t cp )
		{
			// Assuming valid codepoint outside the surrogates.
			uint8_t result = 1 + bool( cp >> 16 );
			assume( result != 0 && result <= max_out );
			return result;
		}
		FORCE_INLINE inline static constexpr void encode( uint32_t cp, T*& out )
		{
			T* p = out;

			// Save the old CP, determine length.
			//
			const uint16_t word_cp = uint16_t( cp );
			const bool has_high = cp != word_cp;
			out += has_high + 1;

			// Adjust the codepoint, encode as extended.
			//
			cp -= 0x10000;
			uint16_t lo = 0xD800 | uint16_t( cp >> 10 );
			uint16_t hi = 0xDC00 | uint16_t( cp );

			// Swap the beginning with 1-byte version if not extended.
			//
			if ( !has_high )
				lo = word_cp;

			// Write the data and return the size.
			//
			if constexpr ( ForeignEndianness )
				lo = bswapw( lo ), hi = bswapw( hi );
			p[ has_high ] = T( hi );
			p[ 0 ] = T( lo );
		}
		FORCE_INLINE inline static constexpr uint32_t decode( std::basic_string_view<T>& in )
		{
			// Read the low pair, rotate.
			//
			uint16_t lo = in[ 0 ];
			if constexpr ( ForeignEndianness )
				lo = bswapw( lo );
			uint16_t lo_flg = lo & fill_bits( 6, 10 );

			// Read the high pair.
			//
			const bool has_high = lo_flg == 0xD800 && in.size() != 1;
			uint16_t hi = in[ has_high ];
			if constexpr ( ForeignEndianness )
				hi = bswapw( hi );

			// Adjust the codepoint accordingly.
			//
			uint32_t cp = hi - 0xDC00 + 0x10000 - ( 0xD800 << 10 );
			cp += lo << 10;
			if ( !has_high )
				cp = hi;

			// Adjust the string view and return.
			//
			in.remove_prefix( has_high + 1 );
			return cp;
		}
		FORCE_INLINE inline static constexpr uint32_t decode( const T*& in, size_t limit = max_out )
		{
			std::basic_string_view<T> tmp{ in, in + limit };
			uint32_t cp = decode( tmp );
			in += ( limit - tmp.size() );
			return cp;
		}
	};

	// UTF-32.
	//
	template<Char32 T, bool ForeignEndianness>
	struct codepoint_cvt<T, ForeignEndianness>
	{
		static constexpr size_t max_out = 1;

		FORCE_INLINE inline static constexpr uint8_t rlength( T ) { return 1; }
		FORCE_INLINE inline static constexpr uint8_t length( uint32_t ) { return 1; }
		FORCE_INLINE inline static constexpr void encode( uint32_t cp, T*& out )
		{
			if constexpr ( ForeignEndianness )
				cp = bswapd( cp );
			*out++ = ( T ) cp;
		}
		FORCE_INLINE inline static constexpr uint32_t decode( std::basic_string_view<T>& in )
		{
			uint32_t cp = ( uint32_t ) in.front();
			in.remove_prefix( 1 );
			if constexpr ( ForeignEndianness )
				cp = bswapd( cp );
			return cp;
		}
		FORCE_INLINE inline static constexpr uint32_t decode( const T*& in, size_t limit = max_out )
		{
			std::basic_string_view<T> tmp{ in, in + limit };
			uint32_t cp = decode( tmp );
			in += ( limit - tmp.size() );
			return cp;
		}
	};

	namespace impl
	{
		static constexpr size_t MinSIMDWidth = 8;
		static constexpr size_t MaxSIMDWidth = XSTD_VECTOR_EXT ? XSTD_SIMD_WIDTH : 0;

		template<typename Char, typename Char2, bool CaseSensitive, bool ForEquality, size_t SIMDWidth> requires ( sizeof( Char ) <= sizeof( Char2 ) )
		PURE_FN FORCE_INLINE inline std::optional<int> utf_ascii_cmp( const Char* v1, const Char2* v2, size_t limit, size_t& iterator )
		{
			// SIMD traits.
			//
			using U1 =        convert_uint_t<Char>;
			using U2 =        convert_uint_t<Char2>;
			using Vector =    xvec<U1, SIMDWidth / sizeof(Char2)>;

			while ( true )
			{
				// Load both vectors.
				//
				Vector vec1 = Vector::load( v1 + iterator );
				Vector vec2 = vec::cast<U1>( xvec<U2, Vector::Length>::load( v2 + iterator ) );

				// If non-ASCII, break out.
				//
				auto cc = vec1 > 0x7F;
				if ( !cc.is_zero() ) [[unlikely]]
					return std::nullopt;

				// If case-insensitive, convert to lower case.
				//
				if constexpr ( !CaseSensitive )
				{
					vec1 ^= ( ( vec1 - 'A' ) <= ( 'z' - 'a' ) ) & 0x20;
					vec2 ^= ( ( vec2 - 'A' ) <= ( 'z' - 'a' ) ) & 0x20;
				}

				// If not equal, break out.
				//
				if ( !vec1.equals( vec2 ) ) [[unlikely]]
				{
					if constexpr ( !ForEquality )
					{
						auto pos = lsb( ( vec1 != vec2 ).mask() );
						return int( vec1[ pos ] ) - int( vec2[ pos ] );
					}
					else
					{
						return 1;
					}
				}
					
				// Increment by vector length.
				//
				iterator += Vector::Length;

				// If we're overflowing.
				//
				if ( ( iterator + Vector::Length ) > limit ) [[unlikely]]
				{
					// If we reached the end, return.
					//
					if ( iterator == limit ) [[likely]]
						return 0;

					// Adjust the pointer to overlap the last vector.
					//
					iterator = limit - Vector::Length;
				}
			}
		}
		template<typename Char, size_t SIMDWidth, typename F>
		FORCE_INLINE inline size_t utf_ascii_enum( const Char* data, size_t limit, F&& enumerator )
		{
			// SIMD traits.
			//
			using U =         convert_uint_t<Char>;
			using Vector =    xvec<U, SIMDWidth / sizeof(Char)>;

			size_t iterator = 0;
			while ( true )
			{
				// Load the value and check for non-ASCII, invoke the functor.
				//
				auto value = Vector::load( data + iterator );
				auto cc = value > 0x7F;
				enumerator( value, iterator );

				// If it was not all ASCII, determine the position and return early.
				//
				if ( !cc.is_zero() ) [[unlikely]]
					return iterator + lsb( cc.bmask() ) / sizeof( Char );

				// Increment by vector length.
				//
				iterator += Vector::Length;

				// If we're overflowing.
				//
				if ( ( iterator + Vector::Length ) > limit ) [[unlikely]]
				{
					// If we reached the end, return.
					//
					if ( iterator == limit ) [[likely]]
						return iterator;

					// Adjust the pointer to overlap the last vector.
					//
					iterator = limit - Vector::Length;
				}
			}
			return iterator;
		}
		template<typename To, typename From, bool NoOutputConstraints, bool CaseConversion, size_t SIMDWidth>
		FORCE_INLINE inline constexpr To* utf_convert( const From* in, const From* in_end, To* out, To* out_end, bool to_lower = false )
		{
			using UTo =   convert_uint_t<To>;
			using UFrom = convert_uint_t<From>;
			const char case_conversion_r = to_lower ? 'A' : 'a';

			while( in != in_end )
			{
				// Consume ASCII as SIMD.
				//
				if constexpr ( SIMDWidth >= MinSIMDWidth )
				{
					size_t limit = size_t( in_end - in );
					if constexpr ( !NoOutputConstraints )
						limit = std::min<size_t>( limit, size_t( out_end - out ) );

					if ( limit >= ( SIMDWidth / sizeof( From ) ) )
					{
						size_t count = utf_ascii_enum<From, SIMDWidth>( in, limit, [ & ] ( auto vector, size_t at )
						{
							auto result = vec::cast<UTo>( vector );
							if constexpr ( CaseConversion )
								result ^= ( ( result - case_conversion_r ) <= ( 'z' - 'a' ) ) & 0x20;
							store_misaligned( out + at, result );
						} );
						in += count;
						out += count;
						if ( in == in_end ) [[unlikely]]
							break;
					}
					else
					{
						return utf_convert<To, From, NoOutputConstraints, CaseConversion, SIMDWidth / 4>( in, in_end, out, out_end, to_lower );
					}
				}

				// Converting between same units:
				//
				if constexpr ( sizeof( From ) == sizeof( To ) )
				{
					// Get the length of the codepoint.
					//
					From front = *in;
					uint8_t length = codepoint_cvt<From>::rlength( front );

					// Do a range check, if overflowing break out.
					//
					size_t limit = size_t( in_end - in );
					if constexpr ( !NoOutputConstraints )
						limit = std::min<size_t>( limit, size_t( out_end - out ) );
					if ( limit < length ) [[unlikely]]
						break;

					// Convert cases where relevant.
					//
					if constexpr ( CaseConversion ) 
						front ^= ( UFrom( front - case_conversion_r ) <= ( 'z' - 'a' ) ) << 5;
					out[ 0 ] = front;

					// Copy rest as encoded.
					//
					visit_index<codepoint_cvt<From>::max_out>( length, [ & ] <size_t I> ( const_tag<I> ) FORCE_INLINE
					{
						for ( uint8_t i = 1; i != I; i++ )
							out[ i ] = in[ i ];
					} );

					// Increment the iterator.
					//
					in += length;
					out += length;
				}
				else
				{
					// Decode a single unit.
					//
					uint32_t cp = codepoint_cvt<From>::decode( in, size_t( in_end - in ) );

					// Convert cases where relevant.
					//
					if constexpr ( CaseConversion )
						cp ^= ( uint32_t( cp - case_conversion_r ) <= ( 'z' - 'a' ) ) << 5;

					// Do a range check, if overflowing break out.
					//
					if ( !NoOutputConstraints && size_t( out_end - out ) < codepoint_cvt<To>::max_out ) [[unlikely]]
					{
						if ( size_t( out_end - out ) < codepoint_cvt<To>::length( cp ) )
							break;
					}

					// Otherwise encode and continue.
					//
					codepoint_cvt<To>::encode( cp, out );
				}
			}
			return out;
		}
		template<typename To, typename From, size_t SIMDWidth>
		PURE_FN FORCE_INLINE inline constexpr size_t utf_calc_length( const From* in, const From* in_end )
		{
			size_t n = 0;
			while( in != in_end )
			{
				// Consume ASCII as SIMD.
				//
				if constexpr ( SIMDWidth >= MinSIMDWidth )
				{
					size_t limit = in_end - in;
					if ( limit >= ( SIMDWidth / sizeof( From ) ) )
					{
						size_t count = utf_ascii_enum<From, SIMDWidth>( in, limit, [ & ] ( auto&&... ) {} );
						in += count;
						n += count;
						if ( in == in_end ) [[unlikely]]
							break;
					}
					else
					{
						return n + utf_calc_length<To, From, SIMDWidth / 4>( in, in_end );
					}
				}
				n += codepoint_cvt<To>::length( codepoint_cvt<From>::decode( in, in_end - in ) );
			}
			return n;
		}
		template<typename Char, typename Char2, bool CaseSensitive, bool ForEquality, size_t SIMDWidth> requires ( sizeof( Char ) <= sizeof( Char2 ) )
		PURE_FN FORCE_INLINE inline constexpr int utf_compare( const Char* a, const Char* a_end, const Char2* b, const Char2* b_end )
		{
			using UChar =   convert_uint_t<Char>;
			using UChar2 =  convert_uint_t<Char2>;

			while( true )
			{
				// Break out if we've reached the end.
				//
				size_t limit = std::min<size_t>( size_t( b_end - b ), size_t( a_end - a ) );
				if ( !limit ) [[unlikely]]
					break;

				// Compare ASCII as SIMD.
				//
				if constexpr ( SIMDWidth >= MinSIMDWidth )
				{
					if ( limit >= ( SIMDWidth / sizeof( Char2 ) ) )
					{
						size_t count = 0;

						auto result = utf_ascii_cmp<Char, Char2, CaseSensitive, ForEquality, SIMDWidth>( a, b, limit, count );
						a += count;
						b += count;
						limit -= count;

						if ( result )
						{
							if ( *result )
								return *result;
							else
								break;
						}
					}
					else
					{
						return utf_compare<Char, Char2, CaseSensitive, ForEquality, SIMDWidth / 4>( a, a_end, b, b_end );
					}
				}

				// If ASCII:
				//
				Char c1 = *a;
				Char2 c2 = *b;
				if ( UChar( c1 ) <= 0x7F && ( ForEquality || UChar2( c2 ) <= 0x7F ) ) [[likely]]
				{
					++a;
					++b;
					if constexpr ( !CaseSensitive )
					{
						c1 ^= ( ( ( 'A' <= c1 ) & ( c1 <= 'Z' ) ) << 5 );
						c2 ^= ( ( ( 'A' <= c2 ) & ( c2 <= 'Z' ) ) << 5 );
					}
					if ( c1 != c2 ) [[unlikely]]
						return int( c1 ) - int( c2 );
					continue;
				}

				// Comparison between same units:
				//
				if constexpr ( sizeof( Char ) == sizeof( Char2 ) )
				{
					if ( c1 != c2 ) [[unlikely]]
						return int( c1 ) - int( c2 );

					// Compare the rest of the encoding.
					//
					uint8_t length = codepoint_cvt<Char>::rlength( c1 );
					if ( limit < length ) [[unlikely]]
						return int( a_end - a ) - int( b_end - b ); // Invalid limits.

					bool mismatch = visit_index<codepoint_cvt<Char>::max_out>( length, [ & ] <size_t I> ( const_tag<I> ) FORCE_INLINE
					{
						bool mismatch = false;
						for ( uint8_t i = 1; i != I; i++ )
							mismatch |= a[ i ] != b[ i ];
						return mismatch;
					} );

					if ( mismatch ) [[unlikely]]
					{
						if constexpr ( ForEquality )
						{
							return 1;
						}
						else
						{
							uint32_t c1 = codepoint_cvt<Char>::decode( a, size_t( a_end - a ) );
							uint32_t c2 = codepoint_cvt<Char2>::decode( b, size_t( b_end - b ) );
							return int( c1 ) - int( c2 );
						}
					}

					a += length;
					b += length;
				}
				else
				{
					uint32_t c1 = codepoint_cvt<Char>::decode( a, size_t( a_end - a ) );
					uint32_t c2 = codepoint_cvt<Char2>::decode( b, size_t( b_end - b ) );
					if ( c1 != c2 ) [[unlikely]]
						return int( c1 ) - int( c2 );
				}
			}

			if ( a != a_end ) [[unlikely]]
			{
				if constexpr ( ForEquality )
					return 1;

				uint32_t cp = codepoint_cvt<Char>::decode( a, size_t( a_end - a ) );
				if constexpr ( !CaseSensitive )
					cp ^= ( ( ( 'A' <= cp ) & ( cp <= 'Z' ) ) << 5 );
				return int( cp );
			}
			else if ( b != b_end ) [[unlikely]]
			{
				if constexpr ( ForEquality )
					return 1;

				uint32_t cp = codepoint_cvt<Char2>::decode( b, size_t( b_end - b ) );
				if constexpr ( !CaseSensitive )
					cp ^= ( ( ( 'A' <= cp ) & ( cp <= 'Z' ) ) << 5 );
				return -int( cp );
			}
			else
			{
				return 0;
			}
		}
	};

	// Generic conversion.
	//
	template<typename To, typename From, bool NoOutputConstraints = false, bool ToUpper = false, bool ToLower = false>
	inline constexpr size_t utf_convert( std::basic_string_view<From> view, std::span<To> output )
	{
		// Converting between same units with no case conversion:
		//
		if constexpr ( sizeof( From ) == sizeof( To ) && !ToUpper && !ToLower )
		{
			size_t limit = NoOutputConstraints ? view.size() : std::min<size_t>( view.size(), output.size() );
			if ( std::is_constant_evaluated() )
				std::copy_n( view.data(), limit, output.data() );
			else
				memcpy( output.data(), view.data(), limit * sizeof( From ) );
			return limit;
		}
		else
		{
			To* result;
			if ( !std::is_constant_evaluated() )
				result = impl::utf_convert<To, From, NoOutputConstraints, ToUpper || ToLower, impl::MaxSIMDWidth>( view.data(), view.data() + view.size(), output.data(), output.data() + output.size(), ToLower );
			else
				result = impl::utf_convert<To, From, NoOutputConstraints, ToUpper || ToLower, 0>( view.data(), view.data() + view.size(), output.data(), output.data() + output.size(), ToLower );
			return result - output.data();
		}
	}

	// UTF aware string comparison.
	//
	template<String S1, String S2, bool CaseSensitive = true, bool ForEquality = false>
	PURE_FN inline constexpr int utf_compare( S1&& a, S2&& b )
	{
		using Char =  string_unit_t<S1>;
		using Char2 = string_unit_t<S2>;
		if constexpr ( sizeof( Char ) > sizeof( Char2 ) )
		{
			return utf_compare<S2, S1, CaseSensitive>( std::forward<S2>( b ), std::forward<S1>( a ) );
		}
		else
		{
			std::basic_string_view<Char> v1{ a };
			std::basic_string_view<Char2> v2{ b };
			if ( !std::is_constant_evaluated() )
				return impl::utf_compare<Char, Char2, CaseSensitive, ForEquality, impl::MaxSIMDWidth>( v1.data(), v1.data() + v1.size(), v2.data(), v2.data() + v2.size() );
			else
				return impl::utf_compare<Char, Char2, CaseSensitive, ForEquality, 0>( v1.data(), v1.data() + v1.size(), v2.data(), v2.data() + v2.size() );
		}
	}
	template<String S1, String S2>
	PURE_FN inline constexpr int utf_icompare( S1&& a, S2&& b )
	{
		return utf_compare<S1, S2, false>( std::forward<S1>( a ), std::forward<S2>( b ) );
	}
	template<String S1, String S2> 
	PURE_FN inline constexpr bool utf_cmpeq( S1&& a, S2&& b )  { return utf_compare<S1, S2, true, true>( std::forward<S1>( a ), std::forward<S2>( b ) ) == 0; }
	template<String S1, String S2> 
	PURE_FN inline constexpr bool utf_icmpeq( S1&& a, S2&& b ) { return utf_compare<S1, S2, false, true>( std::forward<S1>( a ), std::forward<S2>( b ) ) == 0; }

	// UTF aware (converted) string-length calculation.
	//
	template<typename To, String S>
	PURE_FN inline constexpr size_t utf_length( S&& in )
	{
		using From = string_unit_t<S>;
		string_view_t<S> view = { in };
		if constexpr ( sizeof( To ) == sizeof( From ) )
		{
			return view.size();
		}
		else
		{
			if ( !std::is_constant_evaluated() )
				return impl::utf_calc_length<To, From, impl::MaxSIMDWidth>( view.data(), view.data() + view.size() );
			else
				return impl::utf_calc_length<To, From, 0>( view.data(), view.data() + view.size() );
		}
	}

	// Simple conversion wrapper.
	//
	template<typename To, String S>
	inline std::basic_string<To> utf_convert( S&& in )
	{
		// If same unit, forward.
		//
		using From = string_unit_t<S>;
		if constexpr ( Same<From, To> )
			return std::basic_string<To>{ std::forward<S>( in ) };

		// Construct a view and compute the maximum length.
		//
		std::basic_string_view<From> view{ in };
		size_t max_out = codepoint_cvt<To>::max_out * view.size();

		// Reserve the maximum length and invoke the ranged helper.
		//
		std::basic_string<To> result( max_out, '\0' );
		size_t length = utf_convert<To, From, true>( view, result );
		size_t clength = result.size();
		assume( length <= clength );
		result.resize( length );
		return result;
	}
	template<typename To, String S>
	inline std::basic_string<To> utf_convert( S&& in, foreign_endianness_t )
	{
		// If same unit, just byteswap the data.
		//
		using From = string_unit_t<S>;
		if constexpr ( Same<From, To> )
		{
			using U = convert_uint_t<To>;
			std::basic_string<To> result{ std::forward<S>( in ) };
			for ( To& ch : result )
				ch = ( To ) bswap( ( U ) ch );
			return result;
		}

		// Construct a view and compute the maximum length.
		//
		std::basic_string_view<From> view{ in };
		size_t max_out = codepoint_cvt<To>::max_out * view.size();

		// Reserve the maximum length, execute the encoder loop until view is empty.
		//
		std::basic_string<To> result( max_out, '\0' );
		To* iterator = result.data();
		while ( !view.empty() )
			codepoint_cvt<To>::encode( codepoint_cvt<From, true>::decode( view ), iterator );

		// Resize and return the result.
		//
		size_t length = iterator - result.data();
		size_t clength = result.size();
		assume( length <= clength );
		result.resize( length );
		return result;
	}

	// Given a span of raw-data, identifies the encoding using the byte order mark 
	// if relevant and re-encodes into the requested codepoint.
	//
	template<typename To>
	inline std::basic_string<To> utf_convert( std::span<const uint8_t> data )
	{
		// If stream does not start with UTF8 BOM:
		//
		if ( data.size() < 3 || !trivial_equals_n<3>( data.data(), "\xEF\xBB\xBF" ) )
		{
			// Try matching against UTF-32 LE/BE:
			//
			if ( std::u32string_view view{ ( const char32_t* ) data.data(), data.size() / sizeof( char32_t ) }; !view.empty() )
			{
				if ( view.front() == 0xFEFF ) [[unlikely]]
					return utf_convert<To>( view.substr( 1 ) );
				if ( view.front() == bswap<char32_t>( 0xFEFF ) ) [[unlikely]]
					return utf_convert<To>( view.substr( 1 ), foreign_endianness_t{} );
			}
			// Try matching against UTF-16 LE/BE:
			//
			if ( std::u16string_view view{ ( const char16_t* ) data.data(), data.size() / sizeof( char16_t ) }; !view.empty() )
			{
				if ( view.front() == 0xFEFF ) 
					return utf_convert<To>( view.substr( 1 ) );
				if ( view.front() == bswap<char16_t>( 0xFEFF ) ) [[unlikely]]
					return utf_convert<To>( view.substr( 1 ), foreign_endianness_t{} );
			}
		}
		// Otherwise remove the header and continue with the default.
		//
		else
		{
			data = data.subspan( 3 );
		}

		// Decode as UTF-8 and return.
		//
		return utf_convert<To>( std::u8string_view{ ( const char8_t* ) data.data(), data.size() / sizeof( char8_t ) } );
	}
}