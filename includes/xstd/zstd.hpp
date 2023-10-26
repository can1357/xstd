#pragma once
#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>
#include <vector>
#include <string>
#include <cstring>
#include "result.hpp"
#include "type_helpers.hpp"
#include "narrow_cast.hpp"
#include "vec_buffer.hpp"


// [[Configuration]]
// XSTD_ZSTD_HAS_THREADLOCAL: Set if we can allocate threadlocal contexts.
//
#ifndef XSTD_ZSTD_HAS_THREADLOCAL
	#define XSTD_ZSTD_HAS_THREADLOCAL 0
#endif

namespace xstd::zstd {
	// Aliases.
	//
	using reset_directive =    ZSTD_ResetDirective;
	using compressor_ =        ZSTD_CCtx*;
	using decompressor_ =      ZSTD_DCtx*;
	using compressor_param =   ZSTD_cParameter;
	using decompressor_param = ZSTD_dParameter;
	inline static constexpr size_t compress_bound( size_t len ) {
		return ZSTD_COMPRESSBOUND( len );
	}

	// Compression Levels.
	//
	inline const int min_level =         ZSTD_minCLevel();
	inline const int max_level =         ZSTD_maxCLevel();
	inline constexpr int default_level = ZSTD_CLEVEL_DEFAULT;

	// Wrapper around format.
	//
	using zformat = ZSTD_format_e;
	inline constexpr zformat v1_format =        ZSTD_f_zstd1;
	inline constexpr zformat magicless_format = ZSTD_f_zstd1_magicless;
	inline constexpr zformat default_format =   v1_format;

	template<typename T>
	struct context_traits;
	template<> struct context_traits<compressor_> { 
		using parameter_type = compressor_param; 

		inline static constexpr size_t in_size =  ZSTD_BLOCKSIZE_MAX;
		inline static constexpr size_t out_size = compress_bound( ZSTD_BLOCKSIZE_MAX ) + 8;

		inline static compressor_ create() { return ZSTD_createCCtx(); }
		inline static void free( compressor_ ctx ) { ZSTD_freeCCtx( ctx ); }
	};
	template<> struct context_traits<decompressor_> {
		using parameter_type = decompressor_param;

		inline static constexpr size_t in_size =  ZSTD_BLOCKSIZE_MAX + 4;
		inline static constexpr size_t out_size = ZSTD_BLOCKSIZE_MAX + 4;

		inline static decompressor_ create() { return ZSTD_createDCtx(); }
		inline static void free( decompressor_ ctx ) { ZSTD_freeDCtx( ctx ); }
	};

	// Status type.
	//
	struct status_traits {
		inline static constexpr size_t success_value = 0;
		inline static constexpr size_t failure_value = ZSTD_CONTENTSIZE_ERROR;
		inline static bool is_success( auto&& v ) { return int64_t( size_t( v ) ) >= 0; }
	};
	struct TRIVIAL_ABI status {
		using status_traits = status_traits;
		size_t value = status_traits::failure_value;
		FORCE_INLINE constexpr status() noexcept = default;
		FORCE_INLINE constexpr status( size_t v ) noexcept : value( v ) {}
		FORCE_INLINE constexpr status( status&& ) noexcept = default;
		FORCE_INLINE constexpr status( const status& ) noexcept = default;
		FORCE_INLINE constexpr status& operator=( status&& ) noexcept = default;
		FORCE_INLINE constexpr status& operator=( const status& ) noexcept = default;
		FORCE_INLINE constexpr operator size_t& ( ) { return value; }
		FORCE_INLINE constexpr operator const size_t& ( ) const noexcept { return value; }
		FORCE_INLINE constexpr bool is_success() const noexcept { return int64_t( size_t( value ) ) >= 0; }
		FORCE_INLINE constexpr bool is_error() const noexcept { return int64_t( size_t( value ) ) < 0; }
		std::string to_string() const {
			if ( int64_t( value ) >= 0 )
				return XSTD_ESTR( "Ok" );
			const char* name = ZSTD_getErrorName( value );
			if ( name ) {
				return xstd::fmt::str( XSTD_ESTR( "ZSTD Error: %s" ), name );
			} else {
				return xstd::fmt::str( XSTD_ESTR( "ZSTD Error: -%llu" ), uint64_t( -int64_t( value ) ) );
			}
		}
	};

	// Result type.
	//
	template<typename T = std::monostate>
	using result = xstd::basic_result<T, status>;

	// Standalone functions.
	//
	inline static status frame_size( const void* data, size_t len, zformat format = default_format ) {
		ZSTD_frameHeader zfh;
		if ( ZSTD_getFrameHeader_advanced( &zfh, data, len, format ) != 0 )
			return status{ ZSTD_CONTENTSIZE_ERROR };
		if ( zfh.frameType == ZSTD_skippableFrame ) {
			return status{ ZSTD_CONTENTSIZE_UNKNOWN };
		} else {
			return status{ zfh.frameContentSize };
		}
	}

	// Context wrappers.
	//
	inline static result<> set_paramater( compressor_ ctx, compressor_param param, int value ) {
		return { status{ ZSTD_CCtx_setParameter( ctx, param, value ) } };
	}
	inline static result<> set_paramater( decompressor_ ctx, decompressor_param param, int value ) {
		return { status{ ZSTD_DCtx_setParameter( ctx, param, value ) } };
	}
	inline static result<int> get_paramater( compressor_ ctx, compressor_param param ) {
		result<int> res;
		res.status = { ZSTD_CCtx_getParameter( ctx, param, &res.result.emplace() ) };
		return res;
	}
	inline static result<int> get_paramater( decompressor_ ctx, decompressor_param param ) {
		result<int> res;
		res.status = { ZSTD_DCtx_getParameter( ctx, param, &res.result.emplace() ) };
		return res;
	}
	inline static result<> reset_context( compressor_ ctx, reset_directive directive ) {
		return { status{ ZSTD_CCtx_reset( ctx, directive ) } };
	}
	inline static result<> reset_context( decompressor_ ctx, reset_directive directive ) {
		return { status{ ZSTD_DCtx_reset( ctx, directive ) } };
	}
	inline static zformat get_format( compressor_ ctx ) {
		return (zformat) get_paramater( ctx, ZSTD_c_format ).value_or( default_format );
	}
	inline static zformat get_format( decompressor_ ctx ) {
		return (zformat) get_paramater( ctx, ZSTD_d_format ).value_or( default_format );
	}
	inline static result<> set_format( compressor_ ctx, zformat fmt ) {
		return set_paramater( ctx, ZSTD_c_format, fmt );
	}
	inline static result<> set_format( decompressor_ ctx, zformat fmt ) {
		return set_paramater( ctx, ZSTD_d_format, fmt );
	}
	inline static int get_level( compressor_ ctx ) {
		return get_paramater( ctx, ZSTD_c_compressionLevel ).value_or( default_level );
	}
	inline static constexpr int get_level( decompressor_ ) {
		return default_level;
	}
	inline static int set_level( compressor_ ctx, int level ) {
		return set_paramater( ctx, ZSTD_c_compressionLevel, default_level ).fail() ? get_level( ctx ) : level;
	}
	inline static constexpr int set_level( decompressor_, int ) {
		return default_level;
	}
	inline static status apply( compressor_ c, void* buffer, size_t buffer_len, const void* data, size_t len ) {
		return status{ ZSTD_compress2( c, buffer, buffer_len, data, len ) };
	}
	inline static status apply( decompressor_ c, void* buffer, size_t buffer_len, const void* data, size_t len ) {
		return status{ ZSTD_decompressDCtx( c, buffer, buffer_len, data, len ) };
	}
	inline static status frame_upperbound( decompressor_ c, const void* data, size_t len ) {
		return frame_size( data, len, get_format( c ) );
	}
	inline static status frame_upperbound( compressor_, const void*, size_t len ) {
		return compress_bound( len );
	}
	inline static status stream( compressor_ c, std::span<uint8_t>& dst, std::span<const uint8_t>& src, bool end = false, size_t min_step = 0 ) {
		ZSTD_inBuffer in{ src.data(), src.size(), 0 };
		ZSTD_outBuffer out{ dst.data(), dst.size(), 0 };
		size_t status = ZSTD_compressStream2( c, &out, &in, src.empty() ? ( end ? ZSTD_e_end : ZSTD_e_flush ) : ZSTD_e_continue );
		src = src.subspan( in.pos );
		dst = dst.subspan( out.pos );
		return int64_t( status ) > 0 ? std::max( min_step, status ) : status;
	}
	inline static status stream( decompressor_ c, std::span<uint8_t>& dst, std::span<const uint8_t>& src, bool end = false, size_t min_step = 0 ) {
		ZSTD_inBuffer in{ src.data(), src.size(), 0 };
		ZSTD_outBuffer out{ dst.data(), dst.size(), 0 };
		size_t retval = ZSTD_decompressStream( c, &out, &in );
		if ( status{ retval }.is_success() ) {
			// `input.pos < input.size`, some input remaining and caller should provide remaining input
			if ( in.pos < in.size )
				retval = retval ? ZSTD_COMPRESSBOUND( retval ) : min_step;
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

	// Define the context wrapper.
	//
	template<typename T>
	struct TRIVIAL_ABI context_view {
		// Static traits.
		//
		using traits =    context_traits<T>;
		using pointer =   T;
		using parameter = typename traits::parameter_type;
		static constexpr size_t in_size =  traits::in_size;
		static constexpr size_t out_size = traits::out_size;

		// No construction.
		//
		context_view() = delete;
		static inline context_view* create() {
			return (context_view*) traits::create();
		}
		static inline context_view* create( int level ) {
			return create()->set_level( level );
		}
		static inline context_view* create( int level, zformat fmt ) {
			return create( level )->set_format( fmt );
		}

		// Trivially destructed, override delete.
		//
		~context_view() {}
		static void operator delete( void* p ) noexcept {
			traits::free( (pointer) p );
		}
		
		// No copy.
		//
		context_view( const context_view& ) = delete;
		context_view& operator=( const context_view& ) = delete;

		// Get returns this.
		//
		pointer get() const noexcept { return ( pointer ) this; }
		constexpr bool has_value() const noexcept { return true; }
		constexpr explicit operator bool() const noexcept { return true; }


		// Parameter configuration.
		//
		inline result<int> try_get( compressor_param param, int fallback = 0 ) const {
			return xstd::zstd::get_paramater( this->get(), param );
		}
		inline int get( parameter param ) const {
			return *try_get( param );
		}
		inline int get( parameter param, int fallback ) const {
			return try_get( param ).value_or( fallback );
		}
		inline result<> try_set( parameter param, int value ) {
			return xstd::zstd::set_paramater( this->get(), param, value );
		}
		inline context_view* set( parameter param, int value ) {
			try_set( param, value ).assert();
			return this;
		}

		// Context reset.
		//
		inline result<> try_reset( reset_directive directive ) {
			return xstd::zstd::reset_context( this->get(), directive );
		}
		inline context_view* reset_params() {
			try_reset( ZSTD_reset_parameters ).assert();
			return this;
		}
		inline context_view* reset_session() {
			try_reset( ZSTD_reset_session_only ).assert();
			return this;
		}
		inline context_view* reset() {
			try_reset( ZSTD_reset_session_and_parameters ).assert();
			return this;
		}

		// Changing of known parameters.
		//
		inline context_view* set_format( zformat f ) {
			xstd::zstd::set_format( this->get(), f ).assert();
			return this;
		}
		inline zformat get_format() const {
			return xstd::zstd::get_format( this->get() );
		}
		inline int get_level() const {
			return xstd::zstd::get_level( this->get() );
		}
		inline context_view* set_level( int f ) {
			xstd::zstd::set_level( this->get(), f );
			return this;
		}

		// Size calculation.
		//
		inline status frame_upperbound( const void* buffer, size_t len ) const {
			return xstd::zstd::frame_upperbound( this->get(), buffer, len );
		}
		inline status frame_upperbound( std::span<const uint8_t> buf ) const {
			return frame_upperbound( buf.data(), buf.size_bytes() );
		}

		// Simple compression / decompression.
		//
		inline result<> into( void* buffer, size_t buffer_len, const void* data, size_t len ) {
			return xstd::zstd::apply( this->get(), buffer, buffer_len, data, len );
		}
		inline result<> into( std::span<uint8_t> output, std::span<const uint8_t> input ) {
			return into( output.data(), output.size(), input.data(), input.size() );
		}
		template<typename D = vec_buffer>
		inline result<> append_into( D& output, std::span<const uint8_t> input ) {
			size_t skip = std::size( output );
			size_t alloc = frame_upperbound( input );
			uninitialized_resize( output, alloc + skip );
			result<> result = into( std::span{ output }, input );
			if ( result.fail() ) {
				shrink_resize( output, skip );
			} else {
				shrink_resize( output, skip + result.status );
			}
			return result;
		}
		template<typename D = vec_buffer>
		inline result<D> apply( std::span<const uint8_t> input ) {
			result<D> output;
			output.status = this->template append_into<D>( output.result.emplace(), input ).status;
			return output;
		}
		template<typename D = vec_buffer>
		inline result<D> apply( const void* input, size_t len ) {
			result<D> output;
			output.status = this->template append_into<D>( output.result.emplace(), { (const uint8_t*) input, len } ).status;
			return output;
		}

		// Streaming compression / decompression.
		//
		inline status stream( std::span<uint8_t>& dst, std::span<const uint8_t>& src, bool end = false, size_t step = out_size ) {
			return xstd::zstd::stream( this->get(), dst, src, end, step );
		}
		template<typename D = vec_buffer>
		inline result<> stream_into( D& dst, std::span<const uint8_t> src, bool flush = true, bool end = false, size_t step = out_size ) {
			end = end && flush;
			size_t skip = std::size( dst );
			size_t result = step;
			while ( true ) {
				bool flushed = src.empty() || !flush;
				uninitialized_resize( dst, skip + result );

				std::span<uint8_t> buffer{ std::data( dst ) + skip, result };
				result = stream( buffer, src, end, step );
				skip = buffer.data() - std::data( dst );

				if ( status{ result }.is_error() ) 
					return status{ result };
				if ( !result && flushed ) break;
			}
			shrink_resize( dst, skip );
			return status{ 0 };
		}
	};

	// Pointer types.
	//
	using compressor =          context_view<compressor_>;
	using decompressor =        context_view<decompressor_>;

	template<typename View>
	using unique_context =      std::unique_ptr<View>;
	using unique_compressor =   unique_context<compressor>;
	using unique_decompressor = unique_context<decompressor>;

	// Unique allocator.
	//
	template<typename View>
	inline static unique_context<View> make_unique( int level = default_level, zformat fmt = default_format ) {
		return unique_context<View>{ View::create( level, fmt ) };
	}

	// Temporary allocator.
	//
#if XSTD_ZSTD_HAS_THREADLOCAL
	template<typename View>
	inline thread_local unique_context<View> thread_context;

	template<typename View>
	inline static View* get_temporal( int level = default_level, zformat fmt = default_format ) {
		auto& tls = thread_context<View>;
		if ( !tls ) {
			tls = make_unique<View>();
		} else {
			tls->reset();
		}
		return tls->set_format( fmt )->set_level( level );
	}
#else
	template<typename View>
	inline static unique_context<View> get_temporal( int level = default_level, zformat fmt = default_format ) {
		return make_unique<View>( level, fmt );
	}
#endif

	// Simple compression / decompression.
	//
	inline result<> compress_into( std::span<uint8_t> output, std::span<const uint8_t> input, int level = default_level, zformat fmt = default_format ) {
		return get_temporal<compressor>( level, fmt )->into( output, input );
	}
	template<typename T = vec_buffer>
	inline result<> compress_into( T& output, std::span<const uint8_t> input, int level = default_level, zformat fmt = default_format ) {
		return get_temporal<compressor>( level, fmt )->append_into( output, input );
	}
	template<typename T = vec_buffer>
	inline result<T> compress( std::span<const uint8_t> input, int level = default_level, zformat fmt = default_format ) {
		return get_temporal<compressor>( level, fmt )->template apply<T>( input );
	}
	template<typename T = vec_buffer>
	inline result<T> compress( const void* input, size_t len, int level = default_level, zformat fmt = default_format ) {
		return get_temporal<compressor>( level, fmt )->template apply<T>( input, len );
	}
	inline result<> decompress_from( std::span<uint8_t> output, std::span<const uint8_t> input, int level = default_level, zformat fmt = default_format ) {
		return get_temporal<decompressor>( level, fmt )->into( output, input );
	}
	template<typename T = vec_buffer>
	inline result<> decompress_from( T& output, std::span<const uint8_t> input, int level = default_level, zformat fmt = default_format ) {
		return get_temporal<decompressor>( level, fmt )->append_into( output, input );
	}
	template<typename T = vec_buffer>
	inline result<T> decompress( std::span<const uint8_t> input, int level = default_level, zformat fmt = default_format ) {
		return get_temporal<decompressor>( level, fmt )->template apply<T>( input );
	}
	template<typename T = vec_buffer>
	inline result<T> decompress( const void* input, size_t len, int level = default_level, zformat fmt = default_format ) {
		return get_temporal<decompressor>( level, fmt )->template apply<T>( input, len );
	}
};