#pragma once
#include <zstd.h>
#include <vector>
#include <string>
#include <cstring>
#include "result.hpp"
#include "type_helpers.hpp"
#include "narrow_cast.hpp"


// [[Configuration]]
// XSTD_ZSTD_DEFAULT_LEVEL: Sets the default ZSTD compression level.
//
#ifndef XSTD_ZSTD_DEFAULT_LEVEL
	#define XSTD_ZSTD_DEFAULT_LEVEL ZSTD_CLEVEL_DEFAULT
#endif

namespace xstd::zstd
{
	// Compression Levels.
	//
	inline const int min_level = ZSTD_minCLevel();
	inline constexpr int default_level = XSTD_ZSTD_DEFAULT_LEVEL;
	inline const int max_level = ZSTD_maxCLevel();

	// Status type.
	//
	struct status_traits
	{
		inline static constexpr size_t success_value = 0;
		inline static constexpr size_t failure_value = ZSTD_CONTENTSIZE_ERROR;
		inline static bool is_success( auto&& v ) { return int64_t( size_t( v ) ) >= 0; }
	};
	struct TRIVIAL_ABI status
	{
		using status_traits = status_traits;
		size_t value = status_traits::failure_value;
		FORCE_INLINE constexpr status() noexcept = default;
		FORCE_INLINE constexpr status( size_t v ) noexcept : value( v ) {}
		FORCE_INLINE constexpr status( status&& ) noexcept = default;
		FORCE_INLINE constexpr status( const status& ) noexcept = default;
		FORCE_INLINE constexpr status& operator=( status&& ) noexcept = default;
		FORCE_INLINE constexpr status& operator=( const status& ) noexcept = default;
		FORCE_INLINE constexpr operator size_t&() { return value; }
		FORCE_INLINE constexpr operator const size_t&() const noexcept { return value; }
		FORCE_INLINE constexpr bool is_success() const noexcept { return int64_t( size_t( value ) ) >= 0; }
		FORCE_INLINE constexpr bool is_error() const noexcept { return int64_t( size_t( value ) ) < 0; }
		std::string to_string() const { 
			if (int64_t(value) >= 0) 
				return XSTD_ESTR("Ok");
			const char* name = ZSTD_getErrorName(value);
			if (name) {
				return xstd::fmt::str(XSTD_ESTR("ZSTD Error: %s"), name);
			} else {
				return xstd::fmt::str(XSTD_ESTR("ZSTD Error: -%llu"), uint64_t(-int64_t(value)));
			}
		}
	};

	// Result type.
	//
	template<typename T = std::monostate>
	using result = xstd::basic_result<T, status>;

	// Simple compression wrapper.
	//
	inline static result<std::vector<uint8_t>> compress( const void* data, size_t len, int level = default_level )
	{
		result<std::vector<uint8_t>> res;
		auto& buffer = res.result.emplace( make_uninitialized_vector<uint8_t>( ZSTD_compressBound( len ) ) );
		res.status = ZSTD_compress( buffer.data(), buffer.size(), data, len, level );
		if ( res.status.is_success() )
			shrink_resize( *res.result, res.status );
		return res;
	}
	template<ContiguousIterable T> requires ( !Pointer<T> )
	inline static result<std::vector<uint8_t>> compress( T&& cont, int level = default_level )
	{
		return compress( &*std::begin( cont ), std::size( cont ) * sizeof( iterable_val_t<T> ), level );
	}

	// Simple decompression wrapper.
	//
	inline static status get_decompressed_length( const void* data, size_t len )
	{
		return { ZSTD_getFrameContentSize( data, len ) };
	}
	inline static result<> decompress_into( void* buffer, size_t buffer_len, const void* data, size_t len )
	{
		return status{ ZSTD_decompress( buffer, buffer_len, data, len ) };
	}
	inline static result<std::vector<uint8_t>> decompress( const void* data, size_t len )
	{
		result<std::vector<uint8_t>> res;
		res.status = get_decompressed_length( data, len );
		if ( res.status.is_success() )
		{
			auto& buffer = res.result.emplace( make_uninitialized_vector<uint8_t>( res.status ) );
			res.status = decompress_into( buffer.data(), buffer.size(), data, len ).status;
		}
		return res;
	}
	template<ContiguousIterable T>
	inline static result<> decompress_into( void* buffer, size_t buffer_len, T&& cont )
	{
		return decompress_into( buffer, buffer_len, &*std::begin( cont ), std::size( cont ) * sizeof( iterable_val_t<T> ) );
	}
	template<ContiguousIterable T>
	inline static result<std::vector<uint8_t>> decompress( T&& cont )
	{
		return decompress( &*std::begin( cont ), std::size( cont ) * sizeof( iterable_val_t<T> ) );
	}

	// Context wrappers.
	//
	struct ccontext_deleter { void operator()( ZSTD_CCtx* p ) const noexcept { ZSTD_freeCCtx( p ); } };
	using unique_ccontext = std::unique_ptr<ZSTD_CCtx, ccontext_deleter>;
	struct dcontext_deleter { void operator()( ZSTD_DCtx* p ) const noexcept { ZSTD_freeDCtx( p ); } };
	using unique_dcontext = std::unique_ptr<ZSTD_DCtx, dcontext_deleter>;

	struct ccontext
	{
		unique_ccontext ctx{ ZSTD_createCCtx() };

		// Parameter control.
		//
		inline status set_param( ZSTD_cParameter param, int value ) {
			return status{ ZSTD_CCtx_setParameter( ctx.get(), param, value ) };
		}

		// Simple compression.
		//
		inline result<std::vector<uint8_t>> compress( const void* data, size_t len, int level = default_level )
		{
			result<std::vector<uint8_t>> res;
			auto& buffer = res.result.emplace( make_uninitialized_vector<uint8_t>( ZSTD_compressBound( len ) ) );
			res.status = ZSTD_compressCCtx( ctx.get(), buffer.data(), buffer.size(), data, len, level );
			if ( res.status.is_success() )
				shrink_resize( *res.result, res.status );
			return res;
		}
		template<ContiguousIterable T> requires ( !Pointer<T> )
		inline result<std::vector<uint8_t>> compress( T&& cont, int level = default_level )
		{
			return compress( &*std::begin( cont ), std::size( cont ) * sizeof( iterable_val_t<T> ), level );
		}
	};
	
	struct dcontext
	{
		unique_dcontext ctx{ ZSTD_createDCtx() };

		// Parameter control.
		//
		inline status set_param( ZSTD_dParameter param, int value ) {
			return status{ ZSTD_DCtx_setParameter( ctx.get(), param, value ) };
		}

		// Simple decompression.
		//
		inline result<> decompress_into( void* buffer, size_t buffer_len, const void* data, size_t len )
		{
			return status{ ZSTD_decompressDCtx( ctx.get(), buffer, buffer_len, data, len ) };
		}
		inline result<std::vector<uint8_t>> decompress( const void* data, size_t len )
		{
			result<std::vector<uint8_t>> res;
			res.status = get_decompressed_length( data, len );
			if ( res.status.is_success() )
			{
				auto& buffer = res.result.emplace( make_uninitialized_vector<uint8_t>( res.status ) );
				res.status = decompress_into( buffer.data(), buffer.size(), data, len ).status;
			}
			return res;
		}
		template<ContiguousIterable T>
		inline result<> decompress_into( void* buffer, size_t buffer_len, T&& cont )
		{
			return decompress_into( buffer, buffer_len, &*std::begin( cont ), std::size( cont ) * sizeof( iterable_val_t<T> ) );
		}
		template<ContiguousIterable T>
		inline result<std::vector<uint8_t>> decompress( T&& cont )
		{
			return decompress( &*std::begin( cont ), std::size( cont ) * sizeof( iterable_val_t<T> ) );
		}
	};

	// Streaming counterparts.
	//
	struct cstream : ccontext {
		std::vector<uint8_t> output;

		static constexpr size_t in_size =  ZSTD_BLOCKSIZE_MAX;
		static constexpr size_t out_size = ZSTD_COMPRESSBOUND( ZSTD_BLOCKSIZE_MAX ) + 8;

		cstream( int level = XSTD_ZSTD_DEFAULT_LEVEL ) { 
			ZSTD_initCStream( this->ctx.get(), level ); 
		}
		cstream( cstream&& ) noexcept = default;
		cstream& operator=( cstream&& ) noexcept = default;

		// Returns next allocation hint, or 0 if done. If negative, parse as ZStd error.
		// Empty input will flush the stream.
		//
		size_t stream( std::span<uint8_t>& dst, std::span<const uint8_t>& src, bool end = false ) {
			ZSTD_inBuffer in{ src.data(), src.size(), 0 };
			ZSTD_outBuffer out{ dst.data(), dst.size(), 0 };
			size_t status = ZSTD_compressStream2( this->ctx.get(), &out, &in, src.empty() ? ( end ? ZSTD_e_end : ZSTD_e_flush ) : ZSTD_e_continue );
			src = src.subspan( in.pos );
			dst = dst.subspan( out.pos );
			return status ? status + out_size : status;
		}
		
		// Feeds the entire chunk to the stream.
		//
		result<> stream( std::vector<uint8_t>& dst, std::span<const uint8_t> src, bool end = false ) {
			size_t skip =   dst.size();
			size_t result = out_size;
			while ( true ) {
				bool flushed = src.empty();
				uninitialized_resize( dst, dst.size() + result );

				std::span buffer = std::span{ dst }.subspan( skip );
				result = stream( buffer, src, end );
				skip =   buffer.data() - dst.data();
				
				if ( status{ result }.is_error() ) return status{ result };
				if ( !result && flushed ) break;
			}
			shrink_resize( dst, skip );
			return status{ 0 };
		}

		// Feeds the entire chunk to the stream using a temporary buffer.
		//
		result<std::span<uint8_t>> stream( std::span<const uint8_t> data, bool end = false ) {
			output.clear();
			if ( auto st = stream( output, data, end ); st.fail() )
				return st.status;
			if ( output.capacity() > std::max<size_t>( 4 * out_size, output.size() * 2 ) ) {
				output.shrink_to_fit();
			}
			return output;
		}
	};
	struct dstream : dcontext {
		std::vector<uint8_t> output;

		static constexpr size_t in_size =  ZSTD_BLOCKSIZE_MAX + 4;
		static constexpr size_t out_size = ZSTD_BLOCKSIZE_MAX;

		dstream() { ZSTD_initDStream( this->ctx.get() ); }
		dstream( dstream&& ) noexcept = default;
		dstream& operator=( dstream&& ) noexcept = default;
		
		// Returns next allocation hint, or 0 if done. If negative, parse as ZStd error.
		// Empty input will flush the stream.
		//
		size_t stream( std::span<uint8_t>& dst, std::span<const uint8_t>& src ) {
			ZSTD_inBuffer in{ src.data(), src.size(), 0 };
			ZSTD_outBuffer out{ dst.data(), dst.size(), 0 };
			size_t retval = ZSTD_decompressStream(this->ctx.get(), &out, &in);

			if ( status{ retval }.is_success() ) {
				// `input.pos < input.size`, some input remaining and caller should provide remaining input
				if ( in.pos < in.size )
					retval = retval ? ZSTD_COMPRESSBOUND( retval ) : out_size;
				// `output.pos < output.size`, decoder finished and flushed all remaining buffers.
				else if ( out.pos < out.size )
					retval = 0;
				// `output.pos == output.size`, potentially unflushed data present in the internal buffers
				else
					retval = retval ? ZSTD_COMPRESSBOUND( retval ) : 0;
			}

			src = src.subspan( in.pos );
			dst = dst.subspan( out.pos );
			return retval;
		}

		// Feeds the entire chunk to the stream.
		//
		result<> stream( std::vector<uint8_t>& dst, std::span<const uint8_t> src ) {
			size_t skip =   dst.size();
			size_t result = out_size;
			while ( true ) {
				bool flushed = src.empty();
				uninitialized_resize( dst, dst.size() + result );

				std::span buffer = std::span{ dst }.subspan( skip );
				result = stream( buffer, src );
				skip =   buffer.data() - dst.data();
				
				if ( status{ result }.is_error() ) return status{ result };
				if ( !result && flushed ) break;
			}
			shrink_resize( dst, skip );
			return status{ 0 };
		}

		// Feeds the entire chunk to the stream using a temporary buffer.
		//
		result<std::span<uint8_t>> stream( std::span<const uint8_t> data ) {
			output.clear();
			if ( auto st = stream( output, data ); st.fail() )
				return st.status;
			if ( output.capacity() > std::max<size_t>( 4 * out_size, output.size() * 2 ) ) {
				output.shrink_to_fit();
			}
			return output;
		}
	};
};