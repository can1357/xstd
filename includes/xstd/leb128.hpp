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
#include <vector>
#include "result.hpp"
#include "bitwise.hpp"
#include "type_helpers.hpp"
#include "formatting.hpp"
#include "serialization.hpp"

namespace xstd::encode
{
    template<typename T>
    static constexpr size_t leb_max_size = ( ( sizeof( T ) * 8 ) + 7 ) / 7;

    // Define generic encoder and decoder.
    //
    template<Integral T>
    inline static void leb128( std::vector<uint8_t>& out, T value )
    {
        while ( 1 )
        {
            // Push 7 bits of value onto the stream.
            //
            auto& segment = out.emplace_back( ( uint8_t ) ( value & 0x7F ) );
    
            // Shift the number, if we've reached the ending condition break.
            //
            if constexpr ( Signed<T> )
            {
                T ext = value >>= 6;
                if ( ( value >>= 1 ) == ext )
                    break;
            }
            else
            {
                value >>= 7;
                if ( !value ) break;
            }
            
            // Mark the byte to indicate stream is not terminated yet.
            segment |= 0x80;
        }
    }
    
    template<Integral T, typename It1, typename It2>
    inline static result<T> rleb128( It1&& it, It2&& end )
    {
        T value = 0;
        for ( size_t bitcnt = 0; it != end; bitcnt += 7 )
        {
            // Read 7 bits from the stream and reflect onto the value.
            //
            uint8_t segment = *it++;
            value |= T( segment & 0x7F ) << bitcnt;
    
            // Make sure we did not overflow out of the range.
            //
            if ( ( value >> bitcnt ) != ( segment & 0x7F ) )
                return std::nullopt;
    
            // If stream is terminated, return the value.
            //
            if ( !( segment & 0x80 ) )
            {
                if constexpr ( Signed<T> )
                    return ( T ) sign_extend( uint64_t( value ), bitcnt + 7 );
                return { value };
            }
        }
        return std::nullopt;
    }

    // Declare array encoder and decoders.
    //
    template<Integral T>
    inline std::vector<uint8_t> leb128s( const std::vector<T>& values )
    {
        // Allocate a vector for the result and reserve the maximum result size.
        //
        std::vector<uint8_t> result;
        result.reserve( values.size() * leb_max_size<T> );

        // Encode every value and return.
        //
        for ( T value : values )
            leb128<T>( result, value );
        return result;
    }
    template<Integral T, typename It1, typename It2>
    inline std::vector<T> rleb128s( It1&& begin, It2&& end )
    {
        // Allocate a vector for the result and reserve the maximum result size.
        //
        std::vector<T> result;
        result.reserve( std::distance( begin, end ) );

        // Read until we reach the end, indicate failure by returning null.
        //
        auto it = begin;
        while ( it != end )
        {
            if ( auto val = rleb128<T>( it, end ) )
                result.emplace_back( val );
            else
                return {};
        }
        return result;
    }

    // Declare friendlier interface for singular encoding / decoding.
    //
    template<Integral T>
    inline static std::vector<uint8_t> leb128( T value )
    {
        std::vector<uint8_t> result;
        leb128( result, value );
        return result;
    }
    template<Integral T, Iterable C>
    inline static result<T> rleb128( C&& container )
    {
        return rleb128<T>( std::begin( container ), std::end( container ) );
    }

    // Wrapper for serialization helper.
    //
    template<typename T> requires ( Integral<T> || Enum<T> )
    struct leb128_t
    {
        template<typename Ty> struct _underlying_type;
        template<Integral Ty> struct _underlying_type<Ty> { using type = Ty; };
        template<Enum Ty>     struct _underlying_type<Ty> { using type = std::underlying_type_t<Ty>; };
        using underlying_type = typename _underlying_type<T>::type;

        // Thin wrapping around an integer type.
        //
        T value;
        constexpr leb128_t( T value = {} ) : value( value ) {}
        constexpr leb128_t( leb128_t&& ) noexcept = default;
        constexpr leb128_t( const leb128_t& ) = default;
        constexpr leb128_t& operator=( leb128_t&& ) noexcept = default;
        constexpr leb128_t& operator=( const leb128_t& ) = default;
        constexpr operator T&() { return value; }
        constexpr operator const T&() const { return value; }

        // Serialization, deserialization and string conversion.
        //
        std::string to_string() const
        {
            return fmt::as_string( value );
        }
        void serialize( serialization& ctx ) const
        {
            leb128<underlying_type>( ctx.raw_data, ( underlying_type ) value );
        }
        static leb128_t deserialize( serialization& ctx )
        {
            const uint8_t* beg = ctx.raw_data.data() + ctx.offset;
            const uint8_t* it = beg;
            leb128_t value = { ( T ) *rleb128<underlying_type>( it, ctx.raw_data.data() + ctx.raw_data.size() ) };
            ctx.offset += it - beg;
            return value;
        }
    };

    using sleb128_t = leb128_t<int64_t>;
    using uleb128_t = leb128_t<uint64_t>;
};
