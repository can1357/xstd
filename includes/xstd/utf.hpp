#pragma once
#include <cstddef>
#include "bitwise.hpp"
#include "type_helpers.hpp"

namespace xstd
{
	namespace impl
	{
		template<typename In, typename Out>
		FORCE_INLINE constexpr void overlapping_move_qword( In src, size_t n, Out dst )
		{
			if ( std::is_constant_evaluated() )
			{
				std::copy_n( src, n, dst );
			}
			else
			{
				if ( n <= 4 ) [[likely]]
				{
					// [2-4]
					*( uint16_t* ) dst = *( uint16_t* ) src;
					*( uint16_t* ) ( dst + n - 2 ) = *( uint16_t* ) ( src + n - 2 );
				}
				else
				{
					// [5-7]
					*( uint32_t* ) dst = *( uint32_t* ) src;
					*( uint32_t* ) ( dst + n - 4 ) = *( uint32_t* ) ( src + n - 4 );
				}
			}
		}

		// Character types.
		//
		template<typename T> concept Char8 = Same<T, char> || Same<T, char8_t>;
		template<typename T> concept Char16 = Same<T, wchar_t> || Same<T, char16_t>;
	};

	template<typename C>
	struct codepoint_cvt;

	// UTF-8.
	//
	template<impl::Char8 T>
	struct codepoint_cvt<T>
	{
		static constexpr size_t max_out = 7;

		inline static constexpr size_t length( uint32_t cp )
		{
			// Can be encoded with a single character if MSB unset.
			//
			if ( cp <= 0x7F ) [[likely]]
				return 1;

			// Since the initial character encodes the length using 1*N 
			// and a leading zero, and the extension count is always 6,
			// characters are appended in increments of 5.
			//
			//    5 bits  6 bits
			// 110xxxxx 10xxxxxx
			//    4 bits  6 bits  6 bits
			// 1110xxxx 10xxxxxx 10xxxxxx
			//
			return 1 + ( msb( cp ) - 6 + 5 ) / 5;
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
			T* p = out;

			// Fill a qword-sized array with the extension header.
			//
			std::array<uint8_t, 8> tmp = {};
			tmp.fill( 0x80 );

			// While the codepoint overflows the remaining bits from the,
			// header length write 6 bit extensions.
			//
			auto* it = &tmp.back();
			uint8_t max = 1 << 6;
			while ( cp >= max )
			{
				*it-- |= ( cp & fill_bits( 6 ) );
				cp >>= 6;
				max >>= 1;
			}

			// Write the length header.
			//
			auto n = &tmp[ tmp.size() ] - it;
			*it = ( cp | ( 0xFF << ( 8 - n ) ) );

			// Write the data using overlapping moves.
			//
			impl::overlapping_move_qword( it, n, p );
			
			// Increment the iterator.
			//
			out = p + n;
		}
		inline static constexpr uint32_t decode( std::basic_string_view<T>& in )
		{
			// Handle single character case.
			//
			const T& front = in.front();
			in.remove_prefix( 1 );
			if ( !( front & 0x80 ) ) [[likely]]
				return front;

			// Validate the length.
			//
			int n = 7 - msb( ( uint8_t ) ~front );
			if ( n >= 8 || in.size() < uint32_t( n - 1 ) ) [[unlikely]]
				return 0;
			in.remove_prefix( n - 1 );

			// Read the data using overlapping moves.
			//
			std::array<uint8_t, 8> tmp = {};
			tmp.fill( 0x80 );
			impl::overlapping_move_qword( &front, n, tmp.data() );

			// Read the value, extract the validation bits and remove them.
			//
			uint64_t raw_value = std::bit_cast< uint64_t >( tmp );
			uint64_t mask = 0x8080808080808080 | fill_bits( n, 8 - n );
			if ( ( raw_value & mask ) != mask ) [[unlikely]]
				return 0;
			raw_value -= mask;

			// Read each individual byte.
			//
			uint32_t cp = 0;
			#pragma clang loop unroll(disable)
			while ( --n >= 0 )
			{
				cp <<= 6;
				cp |= ( uint8_t ) raw_value;
				raw_value >>= 8;
			}
			return cp;
		}
		inline static constexpr uint32_t decode( const T*& in )
		{
			std::basic_string_view<T> tmp{ in, in + max_out };
			uint32_t cp = decode( tmp );
			in = tmp.data();
			return cp;
		}
	};

	// UTF-16.
	//
	template<impl::Char16 T>
	struct codepoint_cvt<T>
	{
		static constexpr size_t max_out = 2;

		inline static constexpr size_t length( uint32_t cp )
		{
			return cp <= 0xFFFF ? 1 : 2;
		}
		inline static constexpr void encode( uint32_t cp, T*& out )
		{
			T* p = out;
			if ( cp <= 0xFFFF ) [[likely]]
			{
				*p++ = ( T ) cp;
			}
			else
			{
				cp -= 0x10000;
				uint32_t lo = 0xD800 + ( ( cp >> 10 ) & fill_bits( 10 ) );
				uint32_t hi = 0xDC00 + ( cp & fill_bits( 10 ) );
				*p++ = ( T ) lo;
				*p++ = ( T ) hi;
			}
			out = p;
		}
		inline static constexpr uint32_t decode( std::basic_string_view<T>& in )
		{
			// Handle single character case.
			//
			uint16_t c = ( uint16_t ) in.front();
			in.remove_prefix( 1 );
			if ( ( c >> 10 ) != 0x36 ) [[likely]]
				return c;

			// Read the high pair.
			//
			if ( in.empty() )
				return 0;
			uint16_t c2 = ( uint16_t ) in.front();
			in.remove_prefix( 1 );
			if ( ( c2 >> 10 ) != 0x37 )
				return 0;

			uint32_t lo = c - 0xD800;
			uint32_t hi = c2 - 0xDC00;
			return 0x10000 + ( hi | ( lo << 10 ) );
		}
		inline static constexpr uint32_t decode( const T*& in )
		{
			std::basic_string_view<T> tmp{ in, in + max_out };
			uint32_t cp = decode( tmp );
			in = tmp.data();
			return cp;
		}
	};

	// UTF-32.
	//
	template<>
	struct codepoint_cvt<char32_t>
	{
		static constexpr size_t max_out = 1;

		inline static constexpr size_t length( [[maybe_unused]] uint32_t cp )
		{
			return 1;
		}
		inline static constexpr void encode( uint32_t cp, char32_t*& out )
		{
			*out++ = cp;
		}
		inline static constexpr uint32_t decode( std::u32string_view& in )
		{
			uint32_t c = ( uint32_t ) in.front();
			in.remove_prefix( 1 );
			return c;
		}
		inline static constexpr uint32_t decode( const char32_t*& in )
		{
			std::basic_string_view<char32_t> tmp{ in, in + max_out };
			uint32_t cp = decode( tmp );
			in = tmp.data();
			return cp;
		}
	};

	// Generic conversion.
	//
	template<typename C, String S>
	inline static std::basic_string<C> utf_convert( S&& in )
	{
		using D = string_unit_t<S>;

		if constexpr ( Same<C, D> )
		{
			return std::basic_string<C>{ std::forward<S>( in ) };
		}
		else
		{
			string_view_t<S> view = { in };

			// Reserve an estimate size.
			//
			std::basic_string<C> result = {};
			result.resize( view.size() * codepoint_cvt<C>::max_out );
			C* out = result.data();
			while ( !view.empty() )
			{
				if ( C front = view.front(); front <= 0x7F ) [[likely]]
				{
					*out++ = front;
					continue;
				}
				codepoint_cvt<C>::encode( codepoint_cvt<D>::decode( view ), out );
			}
			result.resize( out - result.data() );
			return result;
		}
	}

	// UTF aware string-length calculation.
	//
	template<String S>
	inline static constexpr size_t utf_length( S&& in )
	{
		using D = string_unit_t<S>;
		string_view_t<S> view = { in };
		size_t n = 0;
		while ( !view.empty() )
			codepoint_cvt<D>::decode( view ), n++;
		return n;
	}
}