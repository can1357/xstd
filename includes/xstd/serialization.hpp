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

namespace xstd
{
	namespace impl
	{
		template<typename T>
		concept SafeObj = !Void<T> && ( Trivial<T> || Final<T> );

		// Magic tables for encoding/decoding of the indices.
		//
		inline constexpr std::array<uint8_t, 65> length_table = [ ] ()
		{
			std::array<uint8_t, 65> len = {};
			for ( uint8_t n = 0; n <= 64; n++ )
				len[ n ] = ( uint8_t ) std::max<int>( 1, ( ( 64 - n + 6 ) / 7 ) );
			return len;
		}();
		inline constexpr std::array<uint16_t, 33> decoder_table = [ ] ()
		{
			std::array<uint16_t, 33> result = {};
			for ( uint8_t i = 0; i != 33; i++ )
			{
				uint8_t shift, length;

				// [# = 0-9]
				if ( i < 9 )
				{
					shift =  64 - ( 7 * ( i + 1 ) );
					length = i + 1;
				}
				// [# = 10]
				else if ( i == 16 )
				{
					shift =  0;
					length = 10;
				}
				// [# = 0]
				else
				{
					shift =  0;
					length = 0xFF; // -1
				}
				result[ i ] = uint16_t( shift | ( uint16_t( length ) << 8 ) );
			}
			return result;
		}();
	};

	// Format extension marker for accepting end of stream in tiable objects.
	//
	struct TRIVIAL_ABI version_bump_t {
		struct _Tag {};
		constexpr explicit version_bump_t( _Tag ) {}

		constexpr version_bump_t( version_bump_t&& ) noexcept {}
		constexpr version_bump_t( const version_bump_t& ) {}
		constexpr version_bump_t& operator=( version_bump_t&& ) noexcept { return *this; }
		constexpr version_bump_t& operator=( const version_bump_t& ) { return *this; }

		constexpr auto operator<=>( const version_bump_t& ) const noexcept = default;
	};
	static version_bump_t version_bump{version_bump_t::_Tag{}};

	// Maximum bytes an index serializes into.
	//
	inline constexpr size_t max_index_length = ( 64 + 6 ) / 7;

	// Given an index returns the number of bytes it will take up.
	//
	FORCE_INLINE inline constexpr uint8_t length_index( uint64_t value )
	{
		uint8_t n = impl::length_table[ value ? 63 - msb( value ) : 64 ];
		assume( n <= max_index_length );
		return n;
	}

	// Decodes the given u64 in index from the pair given.
	// - Returns the decoded integer and the length, length < 0 indicates error.
	//
	FORCE_INLINE inline constexpr std::pair<uint64_t, int8_t> decode_index( uint64_t e1, uint16_t e2 )
	{
#if XSTD_HW_PDEP_PEXT
		if ( !std::is_constant_evaluated() )
		{
			uint32_t m = ( uint32_t ) bit_pext<uint64_t>( e1, 0x8080808080808080 );
			m |= uint32_t( e2 & 0x8080 ) << 1; // 9th & 17th

			uint8_t w = m ? ( uint8_t ) xstd::lsb( m ) : 32;
			uint64_t value =   bit_pext<uint64_t>( e1, 0x7f7f7f7f7f7f7f7f );
			value |= uint64_t( bit_pext<uint32_t>( e2, 0x7f7f ) ) << ( 7 * 8 );

			uint16_t dec = impl::decoder_table[ w ];
			uint8_t len =  uint8_t( dec >> 8 );
			uint64_t mask = ~0ull >> uint8_t( dec & 63 );
			return std::pair{ value & mask, int8_t( len ) };
		}
#endif

		size_t idx = 0;
		for ( size_t i = 0; i != 8; i++ )
		{
			uint64_t seg = e1 >> ( i * 8 );
			idx |= ( seg & 0x7F ) << ( i * 7 );
			if ( seg & 0x80 )
				return { idx, int8_t( i ) + 1 };
		}
		for ( size_t i = 8; i != max_index_length; i++ )
		{
			uint64_t seg = e2 >> ( ( i - 8 ) * 8 );
			idx |= ( seg & 0x7F ) << ( i * 7 );
			if ( seg & 0x80 )
				return { idx, int8_t( i ) + 1 };
		}
		return { 0, -1 };
	}

	// Encodes the given u64 in index format into the buffer, caller must have
	// max_index_length reserved as the output.
	//  - Returns the actual length.
	//
	inline constexpr size_t encode_index( uint8_t* out, uint64_t value )
	{
		uint8_t w = length_index( value );

#if XSTD_HW_PDEP_PEXT
		if ( !std::is_constant_evaluated() )
		{
			uint64_t e1 = bit_pdep<uint64_t>( value,              0x7f7f7f7f7f7f7f7f );
			uint32_t e2 = bit_pdep<uint32_t>( value >> ( 7 * 8 ), 0x7f7f );
			*( uint64_t* ) &out[ 0 ] = e1;
			*( uint16_t* ) &out[ 8 ] = e2;
			out[ w - 1 ] |= 0x80;
			return w;
		}
#endif

		for ( size_t n = 0; n != max_index_length; n++ )
			out[ n ] = ( value >> ( 7 * n ) ) & 0x7F;
		out[ w - 1 ] |= 0x80;
		return w;
	}

	// Decodes the given u64 in index from from the buffer.
	// - Returns the decoded integer and the length, length < 0 indicates error.
	//
	inline constexpr std::pair<uint64_t, int8_t> decode_index( const uint8_t* in, size_t limit = max_index_length )
	{
		uint64_t e1 = 0; 
		uint16_t e2 = 0;
		auto read_as = [ & ] <size_t N> ( const_tag<N> ) FORCE_INLINE
		{
			e1 = trivial_read_n<uint64_t, N>( in );
			if constexpr( N > 8 )
				e2 = trivial_read_n<uint16_t, N - 8>( in + 8 );
		};
		if ( limit >= max_index_length ) [[likely]]
			read_as( const_tag<max_index_length>{} );
		else
			visit_index<max_index_length>( limit, read_as );
		return decode_index( e1, e2 );
	}

	// Encodes the given u64 in index format into a vector.
	//
	FORCE_INLINE inline void encode_index( std::vector<uint8_t>& out, uint64_t value )
	{
		size_t pos = out.size();
		out.resize( pos + max_index_length );

		pos += encode_index( out.data() + pos, value );
		shrink_resize( out, pos );
	}

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
		struct pointer_record {
			size_t index = 0;
			std::vector<uint8_t> output_stream = {};
			bool is_backed = false; // Not serialized, temporary.
		};

		struct rpointer_record {
			std::span<const uint8_t> range = {};
			std::shared_ptr<void> deserialized = {};
			bool is_lifted = false;
		};

		// Shared version counter.
		//
		int64_t version = 0;

		// Fields used during serialization.
		//
		std::vector<uint8_t> output_stream;
		std::optional<std::unordered_map<xstd::any_ptr, pointer_record, xstd::hasher<>>> pointers;

		// Fields used during deserialization.
		//
		const uint8_t* input_start = nullptr;
		std::span<const uint8_t> input_stream;
		std::optional<std::unordered_map<size_t, rpointer_record, xstd::hasher<>>> rpointers;

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
			output_stream.insert( output_stream.end(), ( const uint8_t* ) src, ( const uint8_t* ) src + length );
			return *this;
		}

		// Index read/write.
		//
		void write_idx( size_t idx ) {
			encode_index( output_stream, idx );
		}
		size_t read_idx() {
			auto [idx, len] = decode_index( &input_stream[ 0 ], input_stream.size() );
			if ( len >= 0 ) [[likely]] {
				input_stream = input_stream.subspan( len );
				return idx;
			}
			throw_fmt( XSTD_ESTR( "Invalid index value." ) );
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
			if ( !pointers ) pointers.emplace();

			auto& rec = ( *pointers )[ value ];
			rec.is_backed |= owning;
			if ( !rec.index )
			{
				rec.index = pointers->size();
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

			auto& rec = rpointers->at( index );
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

			auto& rec = rpointers->at( index );
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

		// Helpers for readers.
		//
		bool is_input() const { return !input_stream.empty(); }
		bool empty() const { return !length(); }
		size_t length() const { return is_input() ? input_stream.size() : output_stream.size(); }
		size_t offset() const { return is_input() ? input_stream.data() - input_start : output_stream.size(); }

		// Reads or writes the headers.
		//
		std::vector<uint8_t> write_header() const
		{
			serialization sx = {};

			// If version bumps are used or if the first pointer's index clashes with our magic number, write the version header.
			//
			if ( version || ( pointers && !pointers->empty() && ( pointers->begin()->second.index & 0xFF ) == 0xCA ) ) {
				sx.write_idx( 0xca | ( uint64_t( version ) << 8 ) );
			}

			// Write pointers.
			//
			if ( pointers ) {
				for ( auto& [ptr, rec] : *pointers ) {
					if ( !rec.is_backed )
						throw_fmt( XSTD_ESTR( "Dangling pointer serialized!" ) );
					if ( !rec.index )
						throw_fmt( XSTD_ESTR( "Invalid pointer table." ) );
					sx.write_idx( rec.index );
					sx.write_idx( rec.output_stream.size() );
					sx.write( rec.output_stream.data(), rec.output_stream.size() );
				}
			}
			sx.write_idx( 0 );
			return std::move( sx.output_stream );
		}
		serialization& read_header()
		{
			// Read version header.
			//
			size_t idx = read_idx();
			if ( ( idx & 0xFF ) == 0xca ) {
				version = idx >> 8;
				idx = read_idx();
			}

			// Read pointer table.
			//
			if ( idx ) {
				auto& rp = rpointers.emplace();
				do {
					auto [it, inserted] = rp.emplace( idx, rpointer_record{} );
					if ( !inserted )
						throw_fmt( XSTD_ESTR( "Invalid pointer table." ) );
					size_t sz = read_idx();
					if ( sz >= input_stream.size() )
						throw_fmt( XSTD_ESTR( "Referencing out of stream boundaries." ) );
					it->second.range = input_stream.subspan( 0, sz );
					input_stream = input_stream.subspan( sz );
				} while ( ( idx = read_idx() ) );
			}
			return *this;
		}
	};

	// Implement it for trivials, atomics, iterables, tuples, variants, hash type and optionals.
	//
	template<Trivial T> requires ( DefaultSerialized<T> && !Integral<T> )
	struct serializer<T>
	{
		static void apply( serialization& ctx, const T& value )
		{
			ctx.write( &value, sizeof( T ) );
		}
		static T reflect( serialization& ctx )
		{
			std::remove_const_t<T> value;
			ctx.read( &value, sizeof( T ) );
			return value;
		}
	};
	template<Integral T>
	struct serializer<T>
	{
		static void apply( serialization& ctx, T value )
		{
			if constexpr ( sizeof( T ) == 1 )
			{
				ctx.write( &value, sizeof( T ) );
			}
			else
			{
				size_t pvalue;
				if constexpr ( Signed<T> )
				{
					if ( value >= 0 ) pvalue = ( size_t( value )  << 1 );
					else              pvalue = ( size_t( -value ) << 1 ) | 1;
				}
				else
				{
					pvalue = ( size_t ) value;
				}
				ctx.write_idx( pvalue );
			}
		}
		static T reflect( serialization& ctx )
		{
			if constexpr ( sizeof( T ) == 1 )
			{
				std::remove_const_t<T> value;
				ctx.read( &value, sizeof( T ) );
				return value;
			}
			else
			{
				size_t pvalue = ctx.read_idx();
				if constexpr ( Signed<T> )
				{
					if ( pvalue & 1 )
						return -( std::remove_const_t<T> )( pvalue >> 1 );
					else
						return ( std::remove_const_t<T> )( pvalue >> 1 );
				}
				else
				{
					return ( std::remove_const_t<T> ) pvalue;
				}
			}
		}
	};
	template<Atomic T>
	struct serializer<T>
	{
		static void apply( serialization& ctx, const T& value )
		{
			return serialize( ctx, value.load( std::memory_order::relaxed ) );
		}
		static T reflect( serialization& ctx )
		{
			return T{ deserialize<typename T::value_type>( ctx ) };
		}
	};
	template<Iterable T> requires ( DefaultSerialized<T> && !Trivial<T> && !StdArray<T> )
	struct serializer<T>
	{
		static void apply( serialization& ctx, const T& value )
		{
			ctx.write_idx( std::size( value ) );
			for ( auto& entry : value )
				serialize( ctx, entry );
		}
		static T reflect( serialization& ctx )
		{
			size_t cnt = ctx.read_idx();

			std::vector<iterable_val_t<T>> entries;
			entries.reserve( cnt );
			for ( size_t n = 0; n != cnt; n++ )
				entries.emplace_back( deserialize<iterable_val_t<T>>( ctx ) );
	
			if constexpr ( Same<T, std::vector<iterable_val_t<T>>> )
				return entries;
			else
				return T{ std::make_move_iterator( entries.begin() ), std::make_move_iterator( entries.end() ) };
		}
	};
	template<Tuple T>
	struct serializer<T>
	{
		static void apply( serialization& ctx, const T& value )
		{
			if constexpr ( std::tuple_size_v<T> != 0 )
			{
				make_constant_series<std::tuple_size_v<T>>( [ & ] <auto N> ( const_tag<N> )
				{
					serialize( ctx, std::get<N>( value ) );
				} );
			}
		}
		static T reflect( serialization& ctx )
		{
			if constexpr ( std::tuple_size_v<T> != 0 )
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
		}
	};
	template<Variant T>
	struct serializer<T>
	{
		static void apply( serialization& ctx, const T& value )
		{
			size_t idx = value.index();
			ctx.write_idx( idx + 1 );
			if ( idx == std::variant_npos )
				return;
			std::visit( [ & ] ( const auto& ref )
			{
				serialize( ctx, ref );
			}, value );
		}
		static T reflect( serialization& ctx )
		{
			size_t idx = ctx.read_idx();
			if ( idx == 0 ) return {};
			--idx;

			return visit_index<std::variant_size_v<T>>( idx, [ & ] <size_t N> ( const_tag<N> ) -> T
			{
				return T{ std::in_place_index_t<N>{}, deserialize<std::variant_alternative_t<N, T>>( ctx ) };
			} );
		}
	};
	template<Optional T>
	struct serializer<T>
	{
		static void apply( serialization& ctx, const T& value )
		{
			if ( value.has_value() )
			{
				ctx.write<int8_t>( 1 );
				ctx.write<T>( value.value() );
			}
			else
			{
				ctx.write<int8_t>( 0 );
			}
		}
		static T reflect( serialization& ctx )
		{
			if ( ctx.read<int8_t>() )
				return T{ ctx.read<typename T::value_type>() };
			else
				return std::nullopt;
		}
	};
	template<>
	struct serializer<hash_t>
	{
		static void apply( serialization& ctx, const hash_t& value )
		{
			ctx.write( &value, sizeof( value ) );
		}
		static hash_t reflect( serialization& ctx )
		{
			hash_t value;
			ctx.read( &value, sizeof( value ) );
			return value;
		}
	};

	// Acceleration for std::basic_string, std::vector<trivial>, std::array<trivial>.
	//
	template<Trivial T> requires ( !Pointer<T> )
	struct serializer<std::vector<T>>
	{
		static void apply( serialization& ctx, const std::vector<T>& value )
		{
			ctx.write_idx( value.size() );
			ctx.write( value.data(), value.size() * sizeof( T ) );
		}
		static std::vector<T> reflect( serialization& ctx )
		{
			std::vector<T> result;
			result.resize( ctx.read_idx() );
			ctx.read( result.data(), result.size() * sizeof( T ) );
			return result;
		}
	};
	template<typename T, size_t N>
	struct serializer<std::array<T, N>>
	{
		static void apply( serialization& ctx, const std::array<T, N>& value )
		{
			if constexpr ( Trivial<T> && !Pointer<T> )
			{
				ctx.write( value.data(), N * sizeof( T ) );
			}
			else
			{
				for ( size_t i = 0; i != N; i++ )
					ctx.write<T>( value[ i ] );
			}
		}
		static std::array<T, N> reflect( serialization& ctx )
		{
			std::aligned_storage_t<sizeof( std::array<T, N> ), alignof( std::array<T, N> )> tmp;
			auto& result = ( std::array<T, N>& ) tmp;

			if constexpr ( Trivial<T> && !Pointer<T> )
			{
				ctx.read( result.data(), N * sizeof( T ) );
			}
			else
			{
				for ( size_t i = 0; i != N; i++ )
					new ( &result[ i ] ) T( ctx.read<T>() );
			}
			return std::move( result );
		}
	};
	template<typename T>
	struct serializer<std::basic_string<T>>
	{
		static void apply( serialization& ctx, const std::basic_string<T>& value )
		{
			ctx.write_idx( value.size() );
			ctx.write( value.data(), value.size() * sizeof( T ) );
		}
		static std::basic_string<T> reflect( serialization& ctx )
		{
			std::basic_string<T> result;
			result.resize( ctx.read_idx() );
			ctx.read( result.data(), result.size() * sizeof( T ) );
			return result;
		}
	};

	// Implement it for pointer types the type is final.
	//
	template<impl::SafeObj T>
	struct serializer<T*>
	{
		static void apply( serialization& ctx, T* value )
		{
			ctx.serialize_pointer( value, false );
		}
		static T* reflect( serialization& ctx )
		{
			return ctx.deserialize_pointer<T>().get();
		}
	};
	template<impl::SafeObj T>
	struct serializer<const T*>
	{
		static void apply( serialization& ctx, const T* value )
		{
			ctx.serialize_pointer( value, false );
		}
		static const T* reflect( serialization& ctx )
		{
			return ctx.deserialize_pointer<T>().get();
		}
	};
	template<impl::SafeObj T>
	struct serializer<std::shared_ptr<T>>
	{
		static void apply( serialization& ctx, const std::shared_ptr<T>& value )
		{
			ctx.serialize_pointer( value.get(), true );
		}
		static std::shared_ptr<T> reflect( serialization& ctx )
		{
			return ctx.deserialize_pointer<T>();
		}
	};
	template<impl::SafeObj T>
	struct serializer<std::weak_ptr<T>>
	{
		static void apply( serialization& ctx, const std::weak_ptr<T>& value )
		{
			ctx.serialize_pointer( value.lock().get(), false );
		}
		static std::weak_ptr<T> reflect( serialization& ctx )
		{
			return ctx.deserialize_pointer<T>();
		}
	};
	template<impl::SafeObj T>
	struct serializer<std::unique_ptr<T>>
	{
		static void apply( serialization& ctx, const std::unique_ptr<T>& value )
		{
			ctx.serialize_pointer( value.get(), true );
		}
		static std::unique_ptr<T> reflect( serialization& ctx )
		{
			return ctx.deserialize_pointer_u<T>();
		}
	};
	
	// Implement custom serializables.
	//
	template<typename T> requires ( CustomSerializable<T> || CustomDeserializable<T> )
	struct serializer<T> {
		static void apply( serialization& ctx, const T& value ) requires CustomSerializable<T>
		{
			value.serialize( ctx );
		}
		static T reflect( serialization& ctx ) requires CustomDeserializable<T>
		{
			return T::deserialize( ctx );
		}
	};

	// Implement automatically for tiables.
	//
	template<>
	struct serializer<version_bump_t> {};
	template<Tiable O>
	struct serializer<O>
	{
		using T = decltype( std::declval<O&>().tie() );
	
		static void apply( serialization& ctx, const O& value )
		{
			auto tied = const_cast< O& >( value ).tie();
			if constexpr ( std::tuple_size_v<T> != 0 ) {
				make_constant_series<std::tuple_size_v<T>>( [ & ] <auto N> ( const_tag<N> ) {
					if constexpr ( std::is_same_v<std::decay_t<std::tuple_element_t<N ,T>>, version_bump_t> ) {
						ctx.version++;
					} else {
						serialize( ctx, std::get<N>( tied ) );
					}
				} );
			}
		}
		static O reflect( serialization& ctx )
		{
			O value = {};
			auto tied = value.tie();
			if constexpr ( std::tuple_size_v<T> != 0 ) {
				make_constant_search<std::tuple_size_v<T>>( [ & ] <auto N> ( const_tag<N> ) {
					auto& value = std::get<N>( tied );
					if constexpr ( std::is_same_v<std::decay_t<std::tuple_element_t<N, T>>, version_bump_t> ) {
						return --ctx.version < 0;
					} else {
						value = deserialize<std::remove_reference_t<decltype( value )>>( ctx );
						return false;
					}
				} );
			}
			return value;
		}
	};
	
	// Implement monostate as no-op.
	//
	template<>
	struct serializer<std::monostate>
	{
		static void apply( serialization&, const std::monostate& ) {}
		static std::monostate reflect( serialization& ) { return {}; }
	};
	template<typename T>
	using serializer_t = serializer<std::remove_cvref_t<T>>;
	
	// Implement the simple interface.
	//
	template<typename T>
	static std::vector<uint8_t> serialize( const T& value, bool no_header = false )
	{
		serialization ctx = {};
		serializer_t<T>::apply( ctx, value );
		return ctx.dump( no_header );
	}
	template<typename T> static void serialize( serialization& ctx, const T& value ) { serializer_t<T>::apply( ctx, value ); }
	template<typename T> static T deserialize( serialization& ctx ) { return serializer_t<T>::reflect( ctx ); }
	template<typename T> static T deserialize( serialization&& ctx ) { return serializer_t<T>::reflect( ctx ); }
	template<typename T> static void deserialize( T& out, serialization& ctx ) { out = serializer_t<T>::reflect( ctx ); }
	template<typename T> static void deserialize( T& out, serialization&& ctx ) { out = serializer_t<T>::reflect( ctx ); }
	
	// Implement the final steps.
	//
	inline std::vector<uint8_t> serialization::dump( bool no_header ) const
	{
		auto result = output_stream;

		// Insert pointer table if headers requested, else make sure there are none.
		//
		if ( !no_header )
		{
			auto ptr_tbl = write_header();
			result.insert( output_stream.begin(), ptr_tbl.begin(), ptr_tbl.end() );
		}
		else
		{
			if ( pointers && !pointers->empty() )
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
			auto ptr_tbl = write_header();
			output_stream.insert( output_stream.begin(), ptr_tbl.begin(), ptr_tbl.end() );
		}
		else
		{
			if ( pointers && !pointers->empty() )
				throw_fmt( XSTD_ESTR( "Writing a serialization with a pointer table without headers." ) );
			if ( version )
				throw_fmt( XSTD_ESTR( "Writing versioned serialization without headers." ) );
		}
		return std::move( output_stream );
	}
	inline serialization& serialization::load( const void* data, size_t length, bool no_header )
	{
		input_stream = { ( uint8_t* ) data, length };
		input_start = input_stream.data();
		if ( !no_header )
			read_header();
		return *this;
	}
	template<typename T> inline T serialization::read() { return serializer_t<T>::reflect( *this ); }
	template<typename T> inline void serialization::write( const T& value ) { serializer_t<T>::apply( *this, value ); }

	// Helper type for index-like serialization.
	//
	struct index_t
	{
	    size_t value;
	    constexpr index_t( size_t value = {} ) : value( value ) {}
	    constexpr index_t( index_t&& ) noexcept = default;
	    constexpr index_t( const index_t& ) = default;
	    constexpr index_t& operator=( index_t&& ) noexcept = default;
	    constexpr index_t& operator=( const index_t& ) = default;
	    constexpr operator size_t&() { return value; }
	    constexpr operator const size_t&() const { return value; }
	    
	    template<typename Tn> requires Castable<size_t, Tn>
	    explicit constexpr operator Tn() const { return ( Tn ) value; }
	    template<typename Tn> requires Convertible<size_t, Tn>
	    constexpr operator Tn() const { return ( Tn ) value; }
	
	    // Serialization, deserialization, string conversion and hashing.
	    //
		 template<typename H> constexpr void hash( H& out ) const { out.add_bytes( value ); }
	    std::string to_string() const { return fmt::as_string( value ); }
	    void serialize( serialization& ctx ) const { ctx.write_idx( value ); }
	    static index_t deserialize( serialization& ctx ) { return ctx.read_idx(); }
	};
};

// Overload streaming operators.
//
template<typename T>
static xstd::serialization& operator>>( xstd::serialization& self, T& v )
{
	v = self.read<T>();
	return self;
}
template<typename T>
static xstd::serialization& operator<<( T& v, xstd::serialization& self )
{
	v = self.read<T>();
	return self;
}
template<typename T>
static xstd::serialization& operator>>( const T& v, xstd::serialization& self )
{
	self.write( v );
	return self;
}
template<typename T>
static xstd::serialization& operator<<( xstd::serialization& self, const T& v )
{
	self.write( v );
	return self;
}