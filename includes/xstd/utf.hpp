#pragma once
#include <cstddef>
#include "bitwise.hpp"
#include "type_helpers.hpp"

namespace xstd
{
    namespace impl
    {
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

        inline static size_t length( uint32_t cp )
        {
            // Can be encoded with a single character if MSB unset.
            //
            if ( cp <= 0x7F ) return 1;
    
            // Since the initial character encodes the length using 1*N 
            // and a leading zero, and the extension count is always 6,
            // characters are appended in increments of 5.
            //
            //    5 bits  6 bits
            // 110xxxxx 10xxxxxx
            //    4 bits  6 bits  6 bits
            // 1110xxxx 10xxxxxx 10xxxxxx
            //
            size_t n = 2;
            cp >>= 5 + 6;
            while ( cp )
                cp >>= 5, n++;
            return n;
        }
        inline static void encode( uint32_t cp, T*& out )
        {
            size_t n = length( cp );

            // Handle single character case.
            //
            if ( n == 1 )
            {
                *out++ = T( cp );
                return;
            }
    
            // Write the initial byte.
            //
            bitcnt_t i = 8 - ( n + 1 );
            *out++ = T( fill_bits( n, i + 1 ) | ( ( cp >> ( 6 * ( n - 1 ) ) ) & fill_bits( i ) ) );
    
            // Write the extended bytes.
            //
            while ( --n )
                *out++ = T( ( 0b10 << 6 ) | ( ( cp >> ( 6 * ( n - 1 ) ) ) & fill_bits( 6 ) ) );
        }
        inline static uint32_t decode( std::basic_string_view<T>& in )
        {
            // Erroneous cases return 0. 
            //
            if ( in.empty() )
                return 0;
            
            // Handle single character case.
            //
            char c = in.front();
            in.remove_prefix( 1 );
            if ( !( c & 0x80 ) )
                return c;
            
            // Determine the length.
            //
            size_t n = 7 - msb( ( uint8_t ) ~c );
            if ( in.size() < ( n - 1 ) )
                return 0;

            // Read the initial byte.
            //
            size_t i = 8 - ( n + 1 );
            uint32_t cp = uint8_t( c ) & fill_bits( i );

            // Read the extended bytes.
            //
            while ( --n )
            {
                uint8_t c = ( uint8_t ) in.front();
                if ( ( c >> 6 ) != 0b10 )
                    return 0;
                cp <<= 6;
                cp |= c & fill_bits( 6 );
                in.remove_prefix( 1 );
            }
            return cp;
        }
    };

    // UTF-16.
    //
    template<impl::Char16 T>
    struct codepoint_cvt<T>
    {
        static constexpr size_t max_out = 2;

        inline static size_t length( uint32_t cp )
        {
            return cp <= 0xFFFF ? 1 : 2;
        }
        inline static void encode( uint32_t cp, T*& out )
        {
            if ( cp <= 0xFFFF )
            {
                *out++ = ( T ) cp;
                *out = 0; // Hint to vectorization that this address is available.
            }
            else
            {
                cp -= 0x10000;
                uint32_t lo = 0xD800 + ( ( cp >> 10 ) & fill_bits( 10 ) );
                uint32_t hi = 0xDC00 + ( cp & fill_bits( 10 ) );
                *out++ = ( T ) lo;
                *out++ = ( T ) hi;
            }
        }
        inline static uint32_t decode( std::basic_string_view<T>& in )
        {
            // Erroneous cases return 0. 
            //
            if ( in.empty() )
                return 0;

            // Handle single character case.
            //
            uint16_t c = ( uint16_t ) in.front();
            in.remove_prefix( 1 );
            if ( ( c >> 10 ) != 0x36 )
                return c;

            // Read the high pair.
            //
            if ( in.empty() )
                return 0;
            uint16_t c2 = ( uint16_t ) in.front();
            in.remove_prefix( 1 );
            if ( ( c >> 10 ) != 0x37 )
                return 0;

            uint32_t lo = c - 0xD800;
            uint32_t hi = c2 - 0xDC00;
            return 0x10000 + ( hi | ( lo << 10 ) );
        }
    };

    // UTF-32.
    //
    template<>
    struct codepoint_cvt<char32_t>
    {
        static constexpr size_t max_out = 1;

        inline static size_t length( uint32_t cp )
        {
            return 1;
        }
        inline static void encode( uint32_t cp, char32_t*& out )
        {
            *out++ = cp;
        }
        inline static uint32_t decode( std::u32string_view& in )
        {
            if ( in.empty() )
                return 0;
            uint32_t c = ( uint32_t ) in.front();
            in.remove_prefix( 1 );
            return c;
        }
    };

    // Generic conversion.
    //
    template<typename C, String S>
    static std::basic_string<C> utf_convert( S&& in )
    {
        using D = string_unit_t<S>;

        string_view_t<S> view = { in };
        if constexpr ( Same<C, D> )
        {
            return std::basic_string<C>{ std::forward<S>( in ) };
        }
        else
        {
            // Reserve an estimate size.
            //
            std::basic_string<C> result = {};
            result.resize( view.size() * codepoint_cvt<C>::max_out );
            C* out = result.data();
            while ( !view.empty() )
                codepoint_cvt<C>::encode( codepoint_cvt<D>::decode( view ), out );
            result.resize( out - result.data() );
            return result;
        }
    }

    // UTF aware string-length calculation.
    //
    template<String S>
    static size_t utf_length( S&& in )
    {
        using D = string_unit_t<S>;

        string_view_t<S> view = { in };
        size_t n = 0;
        while ( !view.empty() )
            codepoint_cvt<D>::decode( view ), n++;
        return n;
    }
}