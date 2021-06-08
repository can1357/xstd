#pragma once
#include <memory>
#include <vector>
#include <tuple>
#include <span>
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
			size_t index = 0;
			std::vector<uint8_t> output_stream = {};
			bool is_backed = false; // Not serialized, temporary.
		};

		struct rpointer_record
		{
			std::span<const uint8_t> range = {};
			std::shared_ptr<void> deserialized = {};
			void* xdeserialized = nullptr;
			std::function<void()> destroy_xptr = {};
			bool is_lifted = false;
			~rpointer_record() { if ( destroy_xptr ) destroy_xptr(); }
		};

		// Fields used during serialization.
		//
		std::vector<uint8_t> output_stream;
		std::unordered_map<xstd::any_ptr, pointer_record, xstd::hasher<>> pointers;

		// Fields used during deserialization.
		//
		const uint8_t* input_start = nullptr;
		std::span<const uint8_t> input_stream;
		std::unordered_map<size_t, rpointer_record, xstd::hasher<>> rpointers;

		// Constructed from an optional byte array.
		//
		serialization() {}
		serialization( std::span<const uint8_t> view, bool no_header = false ) { load( view.data(), view.size(), no_header ); }
		serialization( const std::vector<uint8_t>& container, bool no_header = false ) { load( container.data(), container.size(), no_header ); }
		serialization( std::vector<uint8_t>&& container, bool no_header = false ) : output_stream( std::move( container ) ) { load( output_stream, no_header ); }
		serialization( const void* data, size_t length, bool no_header = false ) { load( data, length, no_header ); }
		
		// Default move, no copy.
		//
		serialization( serialization&& ) noexcept = default;
		serialization& operator=( serialization&& ) noexcept = default;

		// Helpers to load from a byte vector and dump into one.
		//
		std::vector<uint8_t> dump( bool no_header = false ) const;
		std::vector<uint8_t> dump( bool no_header = false );
		serialization& load( std::span<const uint8_t> view, bool no_header = false ) { return load( view.data(), view.size(), no_header ); }
		serialization& load( const std::vector<uint8_t>& container, bool no_header = false ) { return load( container.data(), container.size(), no_header ); }
		serialization& load( std::vector<uint8_t>&& container, bool no_header = false ) { output_stream = std::move( container ); return load( output_stream, no_header ); }
		serialization& load( const void* data, size_t length, bool no_header = false );

		// Raw data read write.
		//
		serialization& read( void* dst, size_t length )
		{
			if ( input_stream.size() < length )
				throw_fmt( XSTD_ESTR( "Reading out of stream boundaries." ) );
			memcpy( dst, input_stream.data(), length );
			input_stream = input_stream.subspan( length );
			return *this;
		}
		serialization& skip( size_t length )
		{
			if ( input_stream.size() < length )
				throw_fmt( XSTD_ESTR( "Reading out of stream boundaries." ) );
			input_stream = input_stream.subspan( length );
			return *this;
		}
		serialization& write( const void* src, size_t length )
		{
			output_stream.insert( output_stream.end(), ( uint8_t* ) src, ( ( uint8_t* ) src ) + length );
			return *this;
		}

		// Index read/write.
		//
		void write_idx( size_t idx )
		{
			while( true )
			{
				uint8_t seg = idx & 0x7F;
				if ( ( idx >> 7 ) == 0 )
					seg |= 0x80;
				write( &seg, 1 );
				idx >>= 7;
				if ( seg & 0x80 ) break;
			}
		}
		size_t read_idx()
		{
			size_t idx = 0;
			size_t shift = 0;
			while ( true )
			{
				uint8_t seg;
				read( &seg, 1 );
				idx |= ( seg & 0x7F ) << shift;
				shift += 7;
				if ( ( seg & 0x80 ) )
					return idx;
			}
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
			if ( !value )
				return write_idx( 0 );

			auto& rec = pointers[ value ];
			rec.is_backed |= owning;
			if ( !rec.index )
			{
				rec.index = pointers.size();
				rec.output_stream = std::exchange( output_stream, {} );
				serialize( *this, *value );
				std::swap( rec.output_stream, output_stream );
			}
			write_idx( rec.index );
		}
		template<impl::SafeObj T>
		std::unique_ptr<T> deserialize_pointer_u()
		{
			size_t index = read_idx();
			if ( !index ) return nullptr;

			auto& rec = rpointers.at( index );
			if ( !rec.is_lifted )
			{
				auto result = std::unique_ptr<T>( std::allocator<T>{}.allocate( 1 ) );
				rec.is_lifted = true;
				std::swap( input_stream, rec.range );
				new ( result.get() ) T( deserialize<T>( *this ) );
				input_stream = std::move( rec.range );
				return result;
			}
			unreachable();
		}
		template<impl::SafeObj T>
		std::shared_ptr<T> deserialize_pointer()
		{
			size_t index = read_idx();
			if ( !index ) return nullptr;

			auto& rec = rpointers.at( index );
			if ( !rec.is_lifted )
			{
				rec.deserialized = std::shared_ptr<T>( std::allocator<T>{}.allocate( 1 ) );
				rec.is_lifted = true;

				std::swap( input_stream, rec.range );
				new ( rec.deserialized.get() ) T( deserialize<T>( *this ) );
				input_stream = std::move( rec.range );
			}
			return std::reinterpret_pointer_cast< T >( rec.deserialized );
		}
		template<impl::SafeObj T>
		shared<T> deserialize_xpointer()
		{
			size_t index = read_idx();
			if ( !index ) return nullptr;

			auto& rec = rpointers.at( index );
			if ( !rec.is_lifted )
			{
				auto* ref = std::allocator<impl::ref_store<T>>{}.allocate( 1 );
				rec.xdeserialized = ref;
				ref->strong_ref_count = 1;
				ref->weak_ref_count = 0;
				rec.is_lifted = true;

				std::swap( input_stream, rec.range );
				new ( ref->value ) T( deserialize<T>( *this ) );
				input_stream = std::move( rec.range );

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
		bool is_input() const { return !input_stream.empty(); }
		bool empty() const { return !length(); }
		size_t length() const { return is_input() ? input_stream.size() : output_stream.size(); }
		size_t offset() const { return is_input() ? input_stream.data() - input_start : output_stream.size(); }

		// Reads or writes the pointer table.
		//
		std::vector<uint8_t> write_pointer_table() const
		{
			serialization sx = {};
			for ( auto& [ptr, rec] : pointers )
			{
				if ( !rec.is_backed )
					throw_fmt( XSTD_ESTR( "Dangling pointer serialized!" ) );
				if ( !rec.index )
					throw_fmt( XSTD_ESTR( "Invalid pointer table." ) );
				sx.write_idx( rec.index );
				sx.write_idx( rec.output_stream.size() );
				sx.write( rec.output_stream.data(), rec.output_stream.size() );
			}
			sx.write_idx( 0 );
			return std::move( sx.output_stream );
		}
		serialization& read_pointer_table()
		{
			while( true )
			{
				size_t idx = read_idx();
				if ( !idx )
					break;

				auto [it, inserted] = rpointers.emplace( idx, rpointer_record{} );
				if ( !inserted )
					throw_fmt( XSTD_ESTR( "Invalid pointer table." ) );

				size_t sz = read_idx();
				if ( sz >= input_stream.size() )
					throw_fmt( XSTD_ESTR( "Referencing out of stream boundaries." ) );
				it->second.range = input_stream.subspan( 0, sz );
				input_stream = input_stream.subspan( sz );
			}
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
			ctx.write_idx( std::size( value ) );
			for ( auto& entry : value )
				serialize( ctx, entry );
		}
		static inline T reflect( serialization& ctx )
		{
			size_t cnt = ctx.read_idx();
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
			size_t idx = value.index() + 1;
			ctx.write_idx( idx );
			if ( !idx ) return;

			std::visit( [ & ] ( const auto& ref )
			{
				serialize( ctx, ref );
			}, value );
		}
		static inline T reflect( serialization& ctx )
		{
			size_t idx = ctx.read_idx();
			if ( idx == 0 ) return {};
			--idx;

			auto resolve = [ & ] <auto N> ( auto && self, const_tag<N> )
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
			ctx.write( &value, sizeof( value ) );
		}
		static inline hash_t reflect( serialization& ctx )
		{
			hash_t value;
			ctx.read( &value, sizeof( value ) );
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
			auto tied = const_cast< O& >( value ).tie();
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
	inline std::vector<uint8_t> serialization::dump( bool no_header ) const
	{
		auto result = output_stream;

		// Insert pointer table if headers requested, else make sure there are none.
		//
		if ( !no_header )
		{
			auto ptr_tbl = write_pointer_table();
			result.insert( output_stream.begin(), ptr_tbl.begin(), ptr_tbl.end() );
		}
		else
		{
			if ( !pointers.empty() )
				throw_fmt( XSTD_ESTR( "Writing a serialization with a pointer table without headers." ) );
		}
		return result;
	}
	inline std::vector<uint8_t> serialization::dump( bool no_header )
	{
		// Insert pointer table if headers requested, else make sure there are none.
		//
		if ( !no_header )
		{
			auto ptr_tbl = write_pointer_table();
			output_stream.insert( output_stream.begin(), ptr_tbl.begin(), ptr_tbl.end() );
		}
		else
		{
			if ( !pointers.empty() )
				throw_fmt( XSTD_ESTR( "Writing a serialization with a pointer table without headers." ) );
		}
		return std::move( output_stream );
	}
	inline serialization& serialization::load( const void* data, size_t length, bool no_header )
	{
		input_stream = { ( uint8_t* ) data, length };
		input_start = input_stream.data();
		if ( !no_header )
			read_pointer_table();
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