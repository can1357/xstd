#pragma once
#include <cstddef>
#include "bitwise.hpp"
#include "type_helpers.hpp"
#include "xvector.hpp"

namespace xstd
{
	template<typename C>
	struct codepoint_cvt;

	// UTF-8.
	//
	template<Char8 T>
	struct codepoint_cvt<T>
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

		inline static constexpr uint8_t rlength( T _front )
		{
			uint8_t front = uint8_t( _front );
			uint8_t result = ( front >> 7 ) + 1;
			result += front >= 0b11100000;
			result += front >= 0b11110000;
			assume( result != 0 && result <= max_out );
			return result;
		}
		inline static constexpr uint8_t length( uint32_t cp )
		{
			uint8_t result = 1;
			result += bool( cp >> 7 );
			result += bool( cp >> ( 5 + 6 ) );
			result += bool( cp >> ( 4 + 6 + 6 ) );
			assume( result != 0 && result <= max_out );
			return result;
		}
		inline static constexpr void encode( uint32_t cp, T*& out )
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
		inline static constexpr uint32_t decode( std::basic_string_view<T>& in )
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
		inline static constexpr uint32_t decode( const T*& in, size_t limit = max_out )
		{
			std::basic_string_view<T> tmp{ in, in + limit };
			uint32_t cp = decode( tmp );
			in += ( limit - tmp.size() );
			return cp;
		}
	};

	// UTF-16.
	//
	template<Char16 T>
	struct codepoint_cvt<T>
	{
		static constexpr size_t max_out = 2;

		inline static constexpr uint8_t rlength( T front )
		{
			uint8_t result = 1 + ( ( uint16_t( front ) >> 10 ) == 0x36 );
			assume( result != 0 && result <= max_out );
			return result;
		}
		inline static constexpr uint8_t length( uint32_t cp )
		{
			// Assuming valid codepoint outside the surrogates.
			uint8_t result = 1 + bool( cp >> 16 );
			assume( result != 0 && result <= max_out );
			return result;
		}
		inline static constexpr void encode( uint32_t cp, T*& out )
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
			p[ has_high ] = T( hi );
			p[ 0 ] = T( lo );
		}
		inline static constexpr uint32_t decode( std::basic_string_view<T>& in )
		{
			// Read the low pair, rotate.
			//
			uint16_t lo = in[ 0 ];
			uint16_t lo_flg = lo & fill_bits( 6, 10 );

			// Read the high pair.
			//
			const bool has_high = lo_flg == 0xD800 && in.size() != 1;
			uint16_t hi = in[ has_high ];

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
		inline static constexpr uint32_t decode( const T*& in, size_t limit = max_out )
		{
			std::basic_string_view<T> tmp{ in, in + limit };
			uint32_t cp = decode( tmp );
			in += ( limit - tmp.size() );
			return cp;
		}
	};

	// UTF-32.
	//
	template<Char32 T>
	struct codepoint_cvt<T>
	{
		static constexpr size_t max_out = 1;

		inline static constexpr uint8_t rlength( T ) { return 1; }
		inline static constexpr uint8_t length( uint32_t ) { return 1; }
		inline static constexpr void encode( uint32_t cp, T*& out )
		{
			*out++ = ( T ) cp;
		}
		inline static constexpr uint32_t decode( std::basic_string_view<T>& in )
		{
			uint32_t c = ( uint32_t ) in.front();
			in.remove_prefix( 1 );
			return c;
		}
		inline static constexpr uint32_t decode( const T*& in, size_t limit = max_out )
		{
			std::basic_string_view<T> tmp{ in, in + limit };
			uint32_t cp = decode( tmp );
			in += ( limit - tmp.size() );
			return cp;
		}
	};


	namespace impl
	{
		static constexpr size_t MaxSIMDWidth = XSTD_SIMD_WIDTH * 2;
		static constexpr size_t MinSIMDWidth = 8;

		template<typename Char, typename F, size_t SIMDWidth = MaxSIMDWidth>
		FORCE_INLINE inline size_t utf_ascii_enum( const Char* data, size_t limit, F&& enumerator, size_t iterator = 0 )
		{
			// SIMD traits.
			//
			using U =         convert_uint_t<Char>;
			using Vector =    xvec<U, SIMDWidth / sizeof(Char)>;

			// Create the mask.
			//
			constexpr auto mask_bytes = Vector::broadcast( ~Char( 0x7F ) ).bytes();

			// If we can load a vector:
			//
			if ( limit >= ( iterator + Vector::Length ) ) [[likely]]
			{
				while ( true )
				{
					// Load the value and check for non-ASCII, invoke the functor.
					//
					auto value = load_misaligned<Vector>( data + iterator );
					auto cc = value.bytes() & mask_bytes;
					enumerator( value, iterator );

					// If it was not all ASCII, determine the position and return early.
					//
					if ( !cc.is_zero() ) [[unlikely]]
					{
						return iterator + ( lsb( ( cc != 0 ).bmask() ) / sizeof( Char ) );
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
						if ( iterator == limit ) [[unlikely]]
							return iterator;

						// Adjust the pointer to overlap the last vector (if next vector size) or break.
						//
						if constexpr ( SIMDWidth <= ( MaxSIMDWidth / 2 ) )
							iterator = limit - Vector::Length;
						else
							break;
					}
				}
			}

			// Continue onto a smaller size.
			//
			if constexpr ( SIMDWidth > MinSIMDWidth )
				return utf_ascii_enum<Char, F, SIMDWidth / 2>( data, limit, std::forward<F>( enumerator ), iterator );
			else
				return iterator;
		}
	};

	// Generic conversion.
	//
	template<typename To, typename From, bool NoOutputConstraints = false>
	inline constexpr size_t utf_convert( std::basic_string_view<From> view, std::span<To> output )
	{
		// Ranges.
		//
		To* out = output.data();
		To* const limit = out + output.size();

		// Until the view is consumed completely:
		//
		while ( !view.empty() )
		{
			size_t out_limit = NoOutputConstraints ? std::string::npos : size_t( limit - out );
			if ( !std::is_constant_evaluated() )
			{
				// Consume ASCII as SIMD.
				//
				size_t limit = NoOutputConstraints ? view.size() : std::min<size_t>( view.size(), out_limit );
				size_t iterator = impl::utf_ascii_enum<From>( view.data(), limit, [ & ] ( auto vector, size_t at )
				{
					store_misaligned( out + at, vec::cast<convert_uint_t<To>>( vector ) );
				} );
				out += iterator;
				out_limit -= iterator;
				view.remove_prefix( iterator );
				if ( view.empty() ) break;
			}

			// Decode a single unit.
			//
			uint32_t cp = codepoint_cvt<From>::decode( view );

			// Do a range check, if overflowing break out.
			//
			if ( !NoOutputConstraints && out_limit < codepoint_cvt<To>::max_out ) [[unlikely]]
			{
				if ( out_limit < codepoint_cvt<To>::length( cp ) )
					break;
			}

			// Otherwise encode and continue.
			//
			codepoint_cvt<To>::encode( cp, out );
		}
		return out - output.data();
	}

	// UTF aware (converted) string-length calculation.
	//
	template<typename To, String S>
	inline constexpr size_t utf_length( S&& in )
	{
		using From = string_unit_t<S>;
		string_view_t<S> view = { in };
		if constexpr ( sizeof( To ) == sizeof( From ) )
			return view.size();

		size_t n = 0;
		while ( !view.empty() )
		{
			if ( !std::is_constant_evaluated() )
			{
				// Consume ASCII as SIMD.
				//
				size_t iterator = impl::utf_ascii_enum<From>( view.data(), view.size(), [ & ] ( auto&&... ) {} );
				n += iterator;
				view.remove_prefix( iterator );
				if ( view.empty() ) break;
			}
			
			uint32_t cp = codepoint_cvt<From>::decode( view );
			n += codepoint_cvt<To>::length( cp );
		}
		return n;
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
		assume( length < clength );
		result.resize( length );
		return result;
	}
}