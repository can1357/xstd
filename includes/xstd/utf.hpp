#pragma once
#include <cstddef>
#include "bitwise.hpp"
#include "type_helpers.hpp"

namespace xstd
{
    template<typename C>
    struct codepoint_cvt;
    
    // UTF-8.
    //
    template<>
    struct codepoint_cvt<char>
    {
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
        inline static void encode( uint32_t cp, std::string& out )
        {
            size_t n = length( cp );
            out.resize( out.size() + n );
            char* pout = out.data() + out.size() - n;

            // Handle single character case.
            //
            if ( n == 1 )
            {
                *pout = char( cp );
                return;
            }
    
            // Write the initial byte.
            //
            bitcnt_t i = 8 - ( n + 1 );
            *pout++ = char( fill_bits( n, i + 1 ) | ( ( cp >> ( 6 * ( n - 1 ) ) ) & fill_bits( i ) ) );
    
            // Write the extended bytes.
            //
            while ( --n )
                *pout++ = char( ( 0b10 << 6 ) | ( ( cp >> ( 6 * ( n - 1 ) ) ) & fill_bits( 6 ) ) );
        }
        inline static uint32_t decode( std::string_view& in )
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
    template<>
    struct codepoint_cvt<wchar_t>
    {
        inline static size_t length( uint32_t cp )
        {
            return cp <= 0xFFFF ? 1 : 2;
        }
        inline static void encode( uint32_t cp, std::wstring& out )
        {
            if ( cp <= 0xFFFF )
            {
                out.push_back( ( wchar_t ) cp );
            }
            else
            {
                cp -= 0x10000;
                uint32_t lo = 0xD800 + ( ( cp >> 10 ) & fill_bits( 10 ) );
                uint32_t hi = 0xDC00 + ( cp & fill_bits( 10 ) );
                out.push_back( ( wchar_t ) lo );
                out.push_back( ( wchar_t ) hi );
            }
        }
        inline static uint32_t decode( std::wstring_view& in )
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
        inline static size_t length( uint32_t cp )
        {
            return 1;
        }
        inline static void encode( uint32_t cp, std::u32string& out )
        {
            out.push_back( cp );
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
        if constexpr ( sizeof( C ) == sizeof( D ) )
        {
            return std::basic_string<C>{ std::forward<S>( in ) };
        }
        else
        {
            // Reserve an estimate size.
            //
            std::basic_string<C> result = {};
            result.reserve( ( view.size() * sizeof( C ) ) / sizeof( D ) );
            while ( !view.empty() )
                codepoint_cvt<C>::encode( codepoint_cvt<D>::decode( view ), result );
            return result;
        }
    }
}