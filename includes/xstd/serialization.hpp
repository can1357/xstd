#pragma once
#include <memory>
#include <vector>
#include <tuple>
#include <optional>
#include <unordered_map>
#include "assert.hpp"
#include "type_helpers.hpp"
#include "logger.hpp"
#include "bitwise.hpp"
#include "hashable.hpp"
#include "narrow_cast.hpp"
#include "shared.hpp"

namespace xstd
{
    namespace impl
    {
        template<typename T>
        concept SafeObj = !std::is_void_v<T> && ( Trivial<T> || Final<T> );
    };

    // Declare serialization interface.
    //
    struct serialization;
    template<typename T> struct serializer;

    // Declare the contract for custom serializables.
    // => void T::serialize( ctx ) const
    // => static T T::deserialize( ctx )
    //
    template<typename T> concept CustomSerializable = requires( const T & x ) { x.serialize( std::declval<serialization&>() ); };
    template<typename T> concept CustomDeserializable = requires { T::deserialize( std::declval<serialization&>() ); };

    // If none fits, type is considered default serialized.
    //
    template<typename T> concept DefaultSerialized = !( Tiable<T> || CustomSerializable<T> || CustomDeserializable<T> );

    // Serialization context.
    //
    struct serialization
    {
        struct pointer_record
        {
            uint32_t index = 0;
            uint8_t is_shared = false;
            std::vector<uint8_t> raw_data = {};
            bool is_backed = false; // Not serialized, temporary.
            auto tie() { return std::tie( index, is_shared, raw_data ); }
        };

        struct rpointer_record
        {
            std::vector<uint8_t> raw_data = {};
            std::shared_ptr<void> deserialized = {};
            void* xdeserialized = nullptr;
            std::function<void()> destroy_xptr = {};
            bool is_lifted = false;
            ~rpointer_record() { if ( destroy_xptr ) destroy_xptr(); }
        };

        // Shared raw data.
        //
        std::vector<uint8_t> raw_data;
        
        // Fields used during serialization.
        //
        size_t offset = 0;
        std::unordered_map<uintptr_t, pointer_record> pointers;
        
        // Fields used during deserialization.
        //
        std::unordered_map<uint32_t, rpointer_record> rpointers;

        // Constructed from an optional byte array.
        //
        serialization() {}
        template<ContiguousIterable T> requires Trivial<iterable_val_t<T>>
        serialization( T&& container, bool no_header = false ) { load( std::forward<T>( container ), no_header ); }
        serialization( const void* data, size_t length, bool no_header = false ) { load( data, length, no_header ); }
        
        // Default move, no copy.
        //
        serialization( serialization&& ) noexcept = default;
        serialization& operator=( serialization&& ) noexcept = default;

        // Helpers to load from a byte vector and dump into one.
        //
        std::vector<uint8_t> dump() const;
        std::vector<uint8_t> dump();
        serialization& load( const void* data, size_t length, bool no_header = false );
        template<ContiguousIterable T> requires Trivial<iterable_val_t<T>>
        serialization& load( T&& container, bool no_header = false ) 
        {
            return load( ( const uint8_t* ) std::to_address( std::begin( container ) ), std::size( container ) * sizeof( iterable_val_t<T> ), no_header );
        }

        // Raw data read write.
        //
        serialization& read( void* dst, size_t length )
        {
            if ( raw_data.size() < ( offset + length ) )
                throw_fmt( XSTD_ESTR( "Reading out of stream boundaries." ) );
            memcpy( dst, raw_data.data() + offset, length );
            offset += length;
            return *this;
        }
        serialization& skip( size_t n )
        {
            if ( raw_data.size() < ( offset + n ) )
                throw_fmt( XSTD_ESTR( "Skipping out of stream boundaries." ) );
            offset += n;
            return *this;
        }
        serialization& write( const void* src, size_t length )
        {
            raw_data.insert( raw_data.end(), ( uint8_t* ) src, ( ( uint8_t* ) src ) + length );
            return *this;
        }

        // Provide templated member helper for convenience.
        //
        template<typename T> T read();
        template<typename T> void write( const T& );

        // Pointer serialization helpers.
        //
        template<impl::SafeObj T>
        void serialize_pointer( const T* value, bool owning = false )
        {
            if ( !value ) return serialize<uint32_t>( *this, 0 );

            auto& rec = pointers[ ( uintptr_t ) value ];
            if ( !rec.index )
            {
                rec.index = narrow_cast<uint32_t>( pointers.size() );
                rec.is_shared = true;
                rec.raw_data = std::exchange( raw_data, {} );
                serialize( *this, *value );
                std::swap( rec.raw_data, raw_data );
            }
            rec.is_backed |= owning;
            serialize<uint32_t>( *this, rec.index );
        }
        template<impl::SafeObj T>
        std::unique_ptr<T> deserialize_pointer_u()
        {
            uint32_t index = deserialize<uint32_t>( *this );
            if ( !index ) return nullptr;

            auto& rec = rpointers.at( index );
            if ( !rec.is_lifted )
            {
                auto result = std::unique_ptr<T>( std::allocator<T>{}.allocate( 1 ) );
                rec.is_lifted = true;
                std::swap( raw_data, rec.raw_data );

                size_t su = std::exchange( offset, 0 );
                new ( result.get() ) T( deserialize<T>( *this ) );
                raw_data = std::move( rec.raw_data );
                offset = su;
                return result;
            }
            unreachable();
        }
        template<impl::SafeObj T>
        std::shared_ptr<T> deserialize_pointer()
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
            return std::reinterpret_pointer_cast<T>( rec.deserialized );
        }
        template<impl::SafeObj T>
        shared<T> deserialize_xpointer()
        {
            uint32_t index = deserialize<uint32_t>( *this );
            if ( !index ) return nullptr;

            auto& rec = rpointers.at( index );
            if ( !rec.is_lifted )
            {
                auto* ref = std::allocator<impl::ref_store<T>>{}.allocate( 1 );
                rec.xdeserialized = ref;
                ref->strong_ref_count = 1;
                ref->weak_ref_count = 0;
                rec.is_lifted = true;
                std::swap( raw_data, rec.raw_data );
                size_t su = std::exchange( offset, 0 );
                new ( ref->value ) T( deserialize<T>( *this ) );
                raw_data = std::move( rec.raw_data );
                offset = su;
                rec.destroy_xptr = [ = ] () { ref->dec_ref(); };
            }
            if ( auto* p = ( impl::ref_store<T>* ) rec.xdeserialized )
            {
                shared<T> ret{ p };
                ret.entry->inc_ref();
                return ret;
            }
            return nullptr;
        }

        // Helpers for readers.
        //
        bool empty() const { return length() == 0; }
        size_t length() const { return raw_data.size() - offset; }

        // Swaps the pointer table type.
        //
        serialization& flush_pointers()
        {
            for ( auto& rec : pointers )
                rpointers[ rec.second.index ] = { std::move( rec.second.raw_data ) };
            pointers.clear();
            return *this;
        }

        // Flushes the stream by shifting the buffer clearing the read partition.
        //
        serialization& flush() 
        { 
            raw_data.erase( raw_data.begin(), raw_data.begin() + offset ); 
            offset = 0;
            return *this;
        }
    };

    // Implement it for trivials, atomics, iterables, tuples, variants, hash type and optionals.
    //
    template<Trivial T> requires DefaultSerialized<T>
    struct serializer<T>
    {
        static inline void apply( serialization& ctx, const T& value )
        {
            ctx.write( &value, sizeof( T ) );
        }
        static inline T reflect( serialization& ctx )
        {
            std::remove_const_t<T> value;
            ctx.read( &value, sizeof( T ) );
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
    template<Iterable T> requires ( DefaultSerialized<T> && !Trivial<T> )
    struct serializer<T>
    {
        static inline void apply( serialization& ctx, const T& value )
        {
            serialize<uint32_t>( ctx, narrow_cast<uint32_t>( std::size( value ) ) );
            for ( auto& entry : value )
                serialize( ctx, entry );
        }
        static inline T reflect( serialization& ctx )
        {
            fassert( ( ctx.raw_data.size() - ctx.offset ) >= sizeof( uint32_t ) );
            uint32_t cnt = deserialize<uint32_t>( ctx );

            if constexpr ( is_std_array_v<T> )
            {
                return make_constant_series<std::tuple_size_v<T>>( [ & ] <auto V> ( const_tag<V> _ )
                {
                    return deserialize<iterable_val_t<T>>( ctx );
                } );
            }
            else
            {
                std::vector<iterable_val_t<T>> entries;
                entries.reserve( cnt );
                for ( size_t n = 0; n != cnt; n++ )
                    entries.emplace_back( deserialize<iterable_val_t<T>>( ctx ) );

                return T{
                    std::make_move_iterator( entries.begin() ),
                    std::make_move_iterator( entries.end() )
                };
            }
        }
    };
    template<Tuple T>
    struct serializer<T>
    {
        static inline void apply( serialization& ctx, const T& value )
        {
            make_constant_series<std::tuple_size_v<T>>( [ & ] <auto N> ( const_tag<N> )
            {
                serialize( ctx, std::get<N>( value ) );
            } );
        }
        static inline T reflect( serialization& ctx )
        {
            if constexpr ( is_specialization_v<std::pair, T> )
            {
                return make_tuple_series<std::tuple_size_v<T>, std::pair>( [ & ] <auto N> ( const_tag<N> )
                {
                    return deserialize<std::tuple_element_t<N, T>>( ctx );
                } );
            }
            else
            {
                return make_tuple_series<std::tuple_size_v<T>>( [ & ] <auto N> ( const_tag<N> )
                {
                    return deserialize<std::tuple_element_t<N, T>>( ctx );
                } );
            }
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
            auto resolve = [ & ] <auto N> ( auto&& self, const_tag<N> )
            {
                if constexpr ( N != std::variant_size_v<T> )
                {
                    if ( idx == N )
                        return T( deserialize<std::variant_alternative_t<N, T>>( ctx ) );
                    else
                        return self( self, const_tag<N + 1>{} );
                }
                return T{};
            };
            return resolve( resolve, const_tag<0>{} );
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
    template<>
    struct serializer<hash_t>
    {
        static inline void apply( serialization& ctx, const hash_t& value )
        {
            ctx.raw_data.insert( ctx.raw_data.end(), ( uint8_t* ) ( &value ), ( uint8_t* ) ( &value + 1 ) );
        }
        static inline hash_t reflect( serialization& ctx )
        {
            fassert( ctx.raw_data.size() >= ( sizeof( hash_t ) + ctx.offset ) );

            hash_t value;
            memcpy( &value, ctx.raw_data.data() + ctx.offset, sizeof( hash_t ) );
            ctx.offset += sizeof( hash_t );
            return value;
        }
    };

    // Implement it for pointer types the type is final.
    //
    template<impl::SafeObj T>
    struct serializer<T*>
    {
        static inline void apply( serialization& ctx, T* value )
        {
            ctx.serialize_pointer( value, false );
        }
        static inline T* reflect( serialization& ctx )
        {
            return ctx.deserialize_pointer<T>().get();
        }
    };
    template<impl::SafeObj T>
    struct serializer<const T*>
    {
        static inline void apply( serialization& ctx, const T* value )
        {
            ctx.serialize_pointer( value, false );
        }
        static inline const T* reflect( serialization& ctx )
        {
            return ctx.deserialize_pointer<T>().get();
        }
    };
    template<impl::SafeObj T>
    struct serializer<std::shared_ptr<T>>
    {
        static inline void apply( serialization& ctx, const std::shared_ptr<T>& value )
        {
            ctx.serialize_pointer( value.get(), true );
        }
        static inline std::shared_ptr<T> reflect( serialization& ctx )
        {
            return ctx.deserialize_pointer<T>();
        }
    };
    template<impl::SafeObj T>
    struct serializer<shared<T>>
    {
        static inline void apply( serialization& ctx, const shared<T>& value )
        {
            ctx.serialize_pointer( value.get(), true );
        }
        static inline shared<T> reflect( serialization& ctx )
        {
            return ctx.deserialize_xpointer<T>();
        }
    };
    template<impl::SafeObj T>
    struct serializer<std::weak_ptr<T>>
    {
        static inline void apply( serialization& ctx, const std::weak_ptr<T>& value )
        {
            ctx.serialize_pointer( value.lock().get(), false );
        }
        static inline std::weak_ptr<T> reflect( serialization& ctx )
        {
            return ctx.deserialize_pointer<T>();
        }
    };
    template<impl::SafeObj T>
    struct serializer<weak<T>>
    {
        static inline void apply( serialization& ctx, const weak<T>& value )
        {
            ctx.serialize_pointer( value.lock().get(), false );
        }
        static inline weak<T> reflect( serialization& ctx )
        {
            return ctx.deserialize_xpointer<T>();
        }
    };
    template<impl::SafeObj T>
    struct serializer<std::unique_ptr<T>>
    {
        static inline void apply( serialization& ctx, const std::unique_ptr<T>& value )
        {
            ctx.serialize_pointer( value.get(), true );
        }
        static inline std::unique_ptr<T> reflect( serialization& ctx )
        {
            return ctx.deserialize_pointer_u<T>();
        }
    };

    // Implement custom serializables.
    //
    template<typename T> requires ( CustomSerializable<T> || CustomDeserializable<T> )
    struct serializer<T>
    {
        static inline void apply( serialization& ctx, const T& value ) requires CustomSerializable<T>
        {
            value.serialize( ctx );
        }
        static inline T reflect( serialization& ctx ) requires CustomDeserializable<T>
        {
            return T::deserialize( ctx );
        }
    };

    // Implement automatically for tiables.
    //
    template<Tiable O>
    struct serializer<O>
    {
        using T = decltype( std::declval<O&>().tie() );

        static inline void apply( serialization& ctx, const O& value )
        {
            auto tied = const_cast<O&>( value ).tie();
            make_constant_series<std::tuple_size_v<T>>( [ & ] <auto N> ( const_tag<N> )
            {
                serialize( ctx, std::get<N>( tied ) );
            } );
        }
        static inline O reflect( serialization& ctx )
        {
            O value = {};
            auto tied = value.tie();
            make_constant_series<std::tuple_size_v<T>>( [ & ] <auto N> ( const_tag<N> )
            {
                auto& value = std::get<N>( tied );
                value = deserialize<std::remove_reference_t<decltype( value )>>( ctx );
            } );
            return value;
        }
    };

    // Implement monostate as no-op.
    //
    template<>
    struct serializer<std::monostate>
    {
        static inline void apply( serialization&, const std::monostate& ) {}
        static inline std::monostate reflect( serialization& ) { return {}; }
    };
    template<typename T>
    using serializer_t = serializer<std::remove_cvref_t<T>>;

    // Implement the simple interface.
    //
    template<typename T>
    static inline std::vector<uint8_t> serialize( const T& value )
    {
        serialization ctx = {};
        serializer_t<T>::apply( ctx, value );
        return ctx.dump();
    }
    template<typename T> static inline void serialize( serialization& ctx, const T& value ) { serializer_t<T>::apply( ctx, value ); }
    template<typename T> static inline T deserialize( serialization& ctx ) { return serializer_t<T>::reflect( ctx ); }
    template<typename T> static inline T deserialize( serialization&& ctx ) { return serializer_t<T>::reflect( ctx ); }
    template<typename T> static inline void deserialize( T& out, serialization& ctx ) { out = serializer_t<T>::reflect( ctx ); }
    template<typename T> static inline void deserialize( T& out, serialization&& ctx ) { out = serializer_t<T>::reflect( ctx ); }

    // Implement the final steps.
    //
    inline std::vector<uint8_t> serialization::dump() const
    {
        // Validate pointer ownership.
        //
        for ( auto& ptr : pointers )
            if ( ptr.second.index && !ptr.second.is_backed )
                throw_fmt( XSTD_ESTR( "Dangling pointer serialized!" ) );
        
        // Emit the serialization flag declaring whether or not there is a pointer table following.
        //
        std::vector<uint8_t> result = { ( uint8_t ) !pointers.empty() };

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
    inline std::vector<uint8_t> serialization::dump()
    {
        // Validate pointer ownership.
        //
        for ( auto& ptr : pointers )
            if ( ptr.second.index && !ptr.second.is_backed )
                throw_fmt( XSTD_ESTR( "Dangling pointer serialized!" ) );
        auto result = std::move( raw_data );

        // Emit the serialization flag and the pointer table.
        //
        if ( !pointers.empty() )
        {
            serialization subctx;
            subctx.raw_data = { ( uint8_t ) true };
            serialize( subctx, pointers );
            fassert( subctx.pointers.empty() );
            result.insert( result.begin(), subctx.raw_data.begin(), subctx.raw_data.end() );
        }
        else
        {
            result.insert( result.begin(), ( uint8_t ) false );
        }
        
        // Insert the raw data and return.
        //
        return result;
    }
    inline serialization& serialization::load( const void* data, size_t length, bool no_header )
    {
        // Assign over the vector type.
        //
        raw_data.assign( ( const uint8_t* ) data, ( const uint8_t* ) data + length );

        // If using pointer tables, read the mappings and flush the pointers.
        //
        if ( !no_header && read<uint8_t>() )
        {
            deserialize( pointers, *this );
            flush_pointers();
        }
        return *this;
    }
    template<typename T> inline T serialization::read() { return serializer_t<T>::reflect( *this ); }
    template<typename T> inline void serialization::write( const T& value ) { serializer_t<T>::apply( *this, value ); }
};

// Overload streaming operators.
//
template<typename T>
inline static xstd::serialization& operator>>( xstd::serialization& self, T& v )
{
    v = self.read<T>();
    return self;
}
template<typename T>
inline static xstd::serialization& operator<<( T& v, xstd::serialization& self )
{
    v = self.read<T>();
    return self;
}
template<typename T>
inline static xstd::serialization& operator>>( const T& v, xstd::serialization& self )
{
    self.write( v );
    return self;
}
template<typename T>
inline static xstd::serialization& operator<<( xstd::serialization& self, const T& v )
{
    self.write( v );
    return self;
}