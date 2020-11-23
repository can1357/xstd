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
#include <memory>
#include <vector>
#include <tuple>
#include <optional>
#include <unordered_map>
#include "assert.hpp"
#include "type_helpers.hpp"
#include "logger.hpp"

namespace xstd
{
    // Declare serialization interface.
    //
    struct serialization;
    template<typename T> struct serializer;
    template<typename T> static inline T deserialize( serialization& ctx );
    template<typename T> static inline void serialize( serialization& ctx, const T& value );

    struct serialization
    {
        enum class pointer_type
        {
            shared = 0,
            unique = 1,
        };

        struct pointer_record
        {
            uint32_t index = 0;
            uint8_t is_shared = false;
            std::vector<uint8_t> raw_data = {};
            auto tie() { return std::tie( index, is_shared, raw_data ); }
        };

        struct rpointer_record
        {
            std::vector<uint8_t> raw_data = {};
            bool is_lifted = false;
            std::shared_ptr<void> deserialized = {};
        };

        // Shared raw data.
        //
        std::vector<uint8_t> raw_data;
        
        // Fields used during serialization.
        //
        size_t offset = 0;
        uint32_t next_pointer = 1;
        std::unordered_map<uintptr_t, pointer_record> pointers;
        
        // Fields used during deserialization.
        //
        std::unordered_map<uint32_t, rpointer_record> rpointers;

        // Pointer serialization helpers.
        //
        template<typename T>
        void serialize_pointer( const T* value )
        {
            if ( !value ) return serialize<uint32_t>( *this, 0 );

            auto& rec = pointers[ ( uintptr_t ) value ];
            if ( !rec.index )
            {
                rec.index = next_pointer++;
                rec.is_shared = true;
                rec.raw_data = std::exchange( raw_data, {} );
                serialize( *this, *value );
                std::swap( rec.raw_data, raw_data );
            }
            serialize<uint32_t>( *this, rec.index );
        }
        template<typename T>
        std::shared_ptr<T> deserialize_pointer( bool release )
        {

            uint32_t index = deserialize<uint32_t>( *this );
            if ( !index ) return nullptr;

            auto& rec = rpointers.at( index );
            if ( !rec.is_lifted )
            {
                rec.deserialized = std::shared_ptr<T>( std::allocator<T>{}.allocate( 1 ) );
                rec.is_lifted = true;
                std::swap( raw_data, rec.raw_data );

                size_t su = std::exchange( offset, 0 );
                new ( rec.deserialized.get() ) T( deserialize<T>( *this ) );
                raw_data = std::move( rec.raw_data );
                offset = su;
            }
            return release ? std::reinterpret_pointer_cast<T>( std::move( rec.deserialized ) ) : std::reinterpret_pointer_cast<T>( rec.deserialized );
        }

        // Swapping context type.
        //
        void swap_type()
        {
            for ( auto& rec : pointers )
                rpointers[ rec.second.index ] = { std::move( rec.second.raw_data ) };
            pointers.clear();
        }

        // Helpers to dump into a vector of bytes and load from it.
        //
        std::vector<uint8_t> dump( bool no_header = false ) const;
        static serialization load( const std::vector<uint8_t>&, bool no_header = false );
        static serialization load( const uint8_t* data, size_t length, bool no_header = false );
    };

    // Implement it for trivials, atomics, iterables, tuples, variants and optionals.
    //
    template<Trivial T>
    struct serializer<T>
    {
        static inline void apply( serialization& ctx, const T& value )
        {
            ctx.raw_data.insert( ctx.raw_data.end(), ( uint8_t* ) ( &value ), ( uint8_t* ) ( &value + 1 ) );
        }
        static inline T reflect( serialization& ctx )
        {
            fassert( ctx.raw_data.size() >= ( sizeof( T ) + ctx.offset ) );

            std::remove_const_t<T> value;
            memcpy( &value, ctx.raw_data.data() + ctx.offset, sizeof( T ) );
            ctx.offset += sizeof( T );
            return value;
        }
    };
    template<Atomic T>
    struct serializer<T>
    {
        static inline void apply( serialization& ctx, const T& value )
        {
            return serialize( ctx, value.load() );
        }
        static inline T reflect( serialization& ctx )
        {
            return T{ deserialize<typename T::value_type>( ctx ) };
        }
    };
    template<Iterable T>
    struct serializer<T>
    {
        static inline void apply( serialization& ctx, const T& value )
        {
            serialize<uint32_t>( ctx, std::size( value ) );
            for ( auto& entry : value )
                serialize( ctx, entry );
        }
        static inline T reflect( serialization& ctx )
        {
            fassert( ( ctx.raw_data.size() - ctx.offset ) >= sizeof( uint32_t ) );
            uint32_t cnt = deserialize<uint32_t>( ctx );

            std::vector<iterator_value_type_t<T>> entries;
            entries.reserve( cnt );
            for ( size_t n = 0; n != cnt; n++ )
                entries.emplace_back( deserialize<iterator_value_type_t<T>>( ctx ) );
            return T{ entries.begin(), entries.end() };
        }
    };
    template<Tuple T>
    struct serializer<T>
    {
        static inline void apply( serialization& ctx, const T& value )
        {
            std::apply( [ & ] ( const auto&... refs )
            {
                auto discard = [ ] ( ... ) {};
                discard( ( serialize( ctx, refs ), 1 )... );
            }, value );
        }
        static inline T reflect( serialization& ctx )
        {
            return [ & ] <size_t... N> ( std::index_sequence<N...> )
            {
                return T( deserialize<std::tuple_element_t<N, T>>( ctx )... );
            }( std::make_index_sequence<std::tuple_size_v<T>>{} );
        }
    };
    template<Variant T>
    struct serializer<T>
    {
        static inline void apply( serialization& ctx, const T& value )
        {
            serialize<uint32_t>( ctx, value.index() );
            std::visit( [ & ] ( const auto& ref )
            {
                serialize( ctx, ref );
            }, value );
        }
        static inline T reflect( serialization& ctx )
        {
            uint32_t idx = deserialize<uint32_t>( ctx );
            auto resolve = [&] <uint32_t N> ( auto&& self, const_tag<N> )
            {
                if constexpr ( N != std::variant_size_v<T> )
                {
                    if ( idx == N )
                        return T( deserialize<std::variant_alternative_t<N, T>>( ctx ) );
                    else
                        self( self, const_tag<N + >{} );
                }
                return T{};
            };
            return resolve( const_tag<0> );
        }
    };
    template<Optional T>
    struct serializer<T>
    {
        static inline void apply( serialization& ctx, const T& value )
        {
            if ( value.has_value() )
            {
                serialize<int8_t>( ctx, 1 );
                serialize( ctx, value.value() );
            }
            else
            {
                serialize<int8_t>( ctx, 0 );
            }
        }
        static inline T reflect( serialization& ctx )
        {
            if ( deserialize<int8_t>( ctx ) )
                return T{ deserialize<typename T::value_type>( ctx ) };
            else
                return std::nullopt;
        }
    };

    // Implement it for shared_ptr and weak_ptr where the type is final.
    //
    namespace impl
    {
        template<typename T>
        concept SafeObj = !std::is_void_v<T> && ( Trivial<T> || Final<T> );
    };
    template<impl::SafeObj T>
    struct serializer<std::shared_ptr<T>>
    {
        static inline void apply( serialization& ctx, const std::shared_ptr<T>& value )
        {
            ctx.serialize_pointer( value.get() );
        }
        static inline std::shared_ptr<T> reflect( serialization& ctx )
        {
            return ctx.deserialize_pointer<T>( false );
        }
    };
    template<impl::SafeObj T>
    struct serializer<std::weak_ptr<T>>
    {
        static inline void apply( serialization& ctx, const std::weak_ptr<T>& value )
        {
            ctx.serialize_pointer( value.lock().get() );
        }
        static inline std::weak_ptr<T> reflect( serialization& ctx )
        {
            return ctx.deserialize_pointer<T>( false );
        }
    };
    template<impl::SafeObj T>
    struct serializer<std::unique_ptr<T>>
    {
        static inline void apply( serialization& ctx, const std::unique_ptr<T>& value )
        {
            ctx.serialize_pointer( value.get() );
        }
        static inline std::unique_ptr<T> reflect( serialization& ctx )
        {
            return std::unique_ptr<T>{ ctx.deserialize_pointer<T>( true ).release() };
        }
    };

    // Implement custom serializable types.
    // => void T::serialize( ctx ) const
    // => static T T::deserialize( ctx )
    //
    template<typename T>
    concept CustomSerializable = requires( const T& x )
    {
        x.serialize( std::declval<serialization&>() );
        T::deserialize( std::declval<serialization&>() );
    };
    template<CustomSerializable T>
    struct serializer<T>
    {
        static inline void apply( serialization& ctx, const T& value )
        {
            value.serialize( ctx );
        }
        static inline T reflect( serialization& ctx )
        {
            return T::deserialize( ctx );
        }
    };

    // Implement auto serializable types.
    // => std::tuple<&...> T::tie()
    //
    template<typename T>
    concept AutoSerializable = requires( T& x ) { x.tie(); };
    template<AutoSerializable O>
    struct serializer<O>
    {
        using T = decltype( std::declval<O&>().tie() );

        static inline void apply( serialization& ctx, const O& value )
        {
            std::apply( [ & ] ( const auto&... refs )
            {
                auto discard = [ ] ( ... ) {};
                discard( ( serialize( ctx, refs ), 1 )... );
            }, const_cast<O&>( value ).tie() );
        }
        static inline O reflect( serialization& ctx )
        {
            O value = {};
            auto&& tied = value.tie();
            [ & ] <size_t... N> ( std::index_sequence<N...> )
            {
                auto discard = [ ] ( ... ) {};
                discard( ( std::get<N>( tied ) = deserialize<std::remove_reference_t<decltype( std::get<N>( tied ) )>>( ctx ), 1 )... );
            }( std::make_index_sequence<std::tuple_size_v<T>>{} );
            return value;
        }
    };

    // Implement the simple interface.
    //
    template<typename T>
    static inline void serialize( serialization& ctx, const T& value )
    {
        serializer<T>::apply( ctx, value );
    }
    template<typename T>
    static inline T deserialize( serialization& ctx )
    {
        return serializer<T>::reflect( ctx );
    }
    template<typename T>
    static inline serialization serialize( const T& value )
    {
        serialization ctx = {};
        serialize( ctx, value );
        return ctx;
    }
    template<typename T>
    static inline void deserialize( T& out, serialization& ctx )
    {
        out = deserialize<T>( ctx );
    }

    // Implement the final steps.
    //
    inline std::vector<uint8_t> serialization::dump( bool no_header ) const
    {
        // Emit the serialization header.
        //
        // struct xstd_hdr_t {
        //   uint8_t magic[4] = { 'X', 'S', 'T', 'D' };
        //   uint8_t ptr_table_used;
        // }
        //
        std::vector<uint8_t> result;
        if ( !no_header )
            result = { 'X', 'S', 'T', 'D', ( uint8_t ) !pointers.empty() };
        else
            fassert( pointers.empty() );

        // Emit the pointer table if relevant.
        //
        if ( !pointers.empty() )
        {
            serialization subctx;
            serialize( subctx, pointers );
            fassert( subctx.pointers.empty() );
            result.insert( result.end(), subctx.raw_data.begin(), subctx.raw_data.end() );
        }
        
        // Insert the raw data and return.
        //
        result.insert( result.end(), raw_data.begin(), raw_data.end() );
        return result;
    }
    inline serialization serialization::load( const uint8_t* data, size_t length, bool no_header )
    {
        // Read the header if requested.
        //
        serialization ctx = {};
        if ( !no_header )
        {
            fassert( length > 5 && !memcmp( data, XSTD_CSTR( "XSTD" ), 4 ) );
            data += 4;
            length -= 4;
            if ( bool ptr_table_used = *data++ )
            {
                serialization subctx = load( data, length, true );
                deserialize( ctx.pointers, subctx );
                data += subctx.offset;
                length -= subctx.offset;
            }
        }
        ctx.raw_data = { data, data + length };
        ctx.swap_type();
        return ctx;
    }
    inline serialization serialization::load( const std::vector<uint8_t>& data, bool no_header ) { return load( data.data(), data.size(), no_header ); }
};