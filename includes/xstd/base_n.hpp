// Copyright (c) 2020, Can Boluk
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
#pragma once
#include <string_view>
#include <vector>
#include "bitwise.hpp"
#include "type_helpers.hpp"

namespace xstd::encode
{
    namespace impl
    {
        // Helper to create dictionaries.
        //
        template<size_t N>
        static constexpr std::array<int, 0x100> reverse_dictionary( const char( &v )[ N ] )
        {
            std::array<int, 0x100> res = { -1 };
            for ( size_t n = 0; n != N; n++ )
                res[ v[ n ] ] = ( int ) n;
            return res;
        }
        template<size_t N>
        static constexpr std::array<int, 0x100> forward_dictionary( const char( &v )[ N ] )
        {
            std::array<int, 0x100> res = { -1 };
            for ( size_t n = 0; n != N; n++ )
                res[ n ] = v[ n ];
            return res;
        }
    };

    // Declare a basic dictionary with encoding and decoding traits.
    //
    template<size_t N> requires ( N < 0x100 && popcnt( N ) == 1 )
    struct dictionary
    {
        // Number of bits encoded per character, e.g. base64 => 6.
        //
        static constexpr bitcnt_t _bits_per_char = lsb( N );

        // Calculates the group size we encode on each step, e.g. base64 => { 4 out, 3 in }.
        //
        static constexpr bitcnt_t _group_size_out = [ ] ()
        {
            for ( bitcnt_t n = 1; n <= 8; n++ )
                if ( !( ( n * _bits_per_char ) % 8 ) )
                    return n;
            unreachable();
        }();
        static constexpr bitcnt_t _group_size_in = ( _group_size_out * _bits_per_char ) / 8;

        // Bit mask.
        //
        static constexpr uint64_t _mask = fill_bits( _bits_per_char );

        // Forward and reverse dictionaries.
        //
        std::array<int, 0x100> lookup;
        std::array<int, 0x100> rlookup;
        constexpr dictionary( const char( &v )[ N + 2 ] ) : lookup( impl::forward_dictionary( v ) ), rlookup( impl::reverse_dictionary( v ) ) {}

        // Helper methods.
        //
        constexpr char encode( char n ) const
        {
            dassert( n < N );
            return ( char ) lookup[ n ];
        }
        constexpr char decode( char n ) const
        {
            char r = ( char ) rlookup[ n ];
            dassert( 0 <= r && r <= N );
            return r;
        }
        constexpr char fill() const { return lookup[ N ]; }
        constexpr size_t size() const { return N; }
        constexpr size_t group_size_in() const { return _group_size_in; }
        constexpr size_t group_size_out() const { return _group_size_out; }
        constexpr size_t bits_per_char() const { return _bits_per_char; }
        constexpr size_t mask() const { return _mask; }
    };
    template<size_t N>
    dictionary( const char( &v )[ N ] )->dictionary<N-2>;

    // Decodes data using a dictionary.
    //
    namespace impl
    {
        template<typename C = char, bool bit_rev = true, typename Dc = dictionary<64>>
        static std::vector<uint8_t> rbase_n( std::basic_string_view<C> str, const Dc& dictionary )
        {
            if ( ( str.length() % dictionary.group_size_out() ) != 0 )
                return {};

            // Calculate the group count and reserve the output.
            //
            std::vector<uint8_t> result;
            size_t group_count = str.size() / dictionary.group_size_out();
            result.resize( group_count * dictionary.group_size_in() + 1 );
            uint8_t* rit = result.data();

            // Read each group:
            //
            size_t offset = 0;
            for ( auto& c : str )
            {
                if ( c != dictionary.fill() )
                {
                    uint16_t v = dictionary.decode( ( char ) c );
                    if constexpr ( bit_rev )
                    {
                        v <<= 8 - dictionary.bits_per_char();
                        v = bit_reverse<uint8_t>( v );
                        v &= dictionary.mask();

                        rit[ 0 ] |= bit_reverse<uint8_t>( v << offset );
                        rit[ 1 ] |= bit_reverse<uint8_t>( v >> ( 8 - offset ) );
                    }
                    else
                    {
                        rit[ 0 ] |= v << offset;
                        rit[ 1 ] |= v >> ( 8 - offset );
                    }

                    offset += dictionary.bits_per_char();
                    rit += offset / 8;
                    offset %= 8;
                }
                else
                {
                    rit += *rit != 0;
                    break;
                }
            }
            result.resize( rit - result.data() );
            return result;
        }
    };
    template<String S, bool bit_rev = true, typename Dc = dictionary<64>>
    static std::vector<uint8_t> rbase_n( S&& str, const Dc& dictionary )
    {
        return impl::rbase_n( string_view_t<S>{ str }, dictionary );
    }

    // Encodes data using a dictionary.
    //
    template<typename C = char, bool bit_rev = true, typename Dc = dictionary<64>>
    static std::basic_string<C> base_n( const void* data, size_t length, const Dc& dictionary )
    {
        // Calculate the group count and reserve the output.
        //
        std::basic_string<C> result;
        size_t group_count = ( length + dictionary.group_size_in() - 1 ) / dictionary.group_size_in();
        result.resize( group_count * dictionary.group_size_out() );
        C* rit = result.data();

        // Write each group:
        //
        const uint8_t* beg = ( const uint8_t* ) data;
        const uint8_t* end = beg + length;
        for ( auto it = beg; it < end; it += dictionary.group_size_in() )
        {
            // For each output:
            //
            for ( size_t offset = 0; offset != ( dictionary.group_size_in() * 8 ); offset += dictionary.bits_per_char() )
            {
                // Insert filler if out of boundaries.
                //
                auto in = it + ( offset / 8 );
                if ( in >= end )
                {
                    *rit++ = dictionary.fill();
                }
                // Otherwise read the value and encode.
                //
                else
                {
                    if constexpr ( bit_rev )
                    {
                        uint16_t value = uint16_t( bit_reverse( in[ 0 ] ) );
                        if ( ( in + 1 ) != end )
                            value |= uint16_t( bit_reverse( in[ 1 ] ) ) << 8;
                        value >>= offset % 8;
                        value = bit_reverse( uint8_t( value & dictionary.mask() ) ) >> ( 8 - dictionary.bits_per_char() );
                        *rit++ = dictionary.encode( ( char ) value );
                    }
                    else
                    {
                        uint16_t value = uint16_t( in[ 0 ] );
                        if ( ( in + 1 ) != end )
                            value |= uint16_t( in[ 1 ] ) << 8;
                        value >>= offset % 8;
                        value &= dictionary.mask();
                        *rit++ = dictionary.encode( ( char ) value );
                    }
                }
            }
        }
        result.resize( rit - result.data() );
        return result;
    }

    template<typename C = char, bool bit_rev = true, typename Dc = dictionary<64>, Iterable T = const std::initializer_list<uint8_t>&> 
    requires is_contiguous_iterable_v<T>
    static std::basic_string<C> base_n( T&& c, const Dc& dictionary )
    {
        return base_n<C, bit_rev, Dc>( &*std::begin( c ), std::size( c ) * sizeof( iterator_value_type_t<T> ), dictionary );
    }

    // Dictionary defined for base64.
    //
    static constexpr dictionary base64_dictionary = {
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "+/="
    };
};