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
	#define XSTD_ZSTD_DEFAULT_LEVEL 14
#endif

namespace xstd::zstd
{
	struct status_traits
	{
		inline static constexpr size_t success_value = 0;
		inline static constexpr size_t failure_value = ZSTD_CONTENTSIZE_ERROR;
		inline static bool is_success( auto&& v ) { return int64_t( size_t( v ) ) >= 0; }
	};

	// Status type.
	//
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
		std::string to_string() const { return int64_t( value ) >= 0 ? XSTD_ESTR( "Ok" ) : xstd::fmt::str( XSTD_ESTR( "ZSTD Error: -%llu" ), uint64_t( -int64_t( value ) ) ); }
	};

	// Result type.
	//
	template<typename T = std::monostate>
	using result = xstd::result<T, status>;

	// Simple compression wrapper.
	//
	inline static result<std::vector<uint8_t>> compress( const void* data, size_t len, int level = XSTD_ZSTD_DEFAULT_LEVEL )
	{
		result<std::vector<uint8_t>> res;
		auto& buffer = res.result.emplace( std::vector<uint8_t>( ZSTD_compressBound( len ), 0 ) );
		res.status = ZSTD_compress( buffer.data(), buffer.size(), data, len, level );
		if ( res.status.is_success() )
			res.result->resize( res.status );
		return res;
	}
	template<ContiguousIterable T> requires ( !Pointer<T> )
	inline static result<std::vector<uint8_t>> compress( T&& cont, int level = XSTD_ZSTD_DEFAULT_LEVEL )
	{
		return compress( &*std::begin( cont ), std::size( cont ) * sizeof( iterable_val_t<T> ), level );
	}

	// Simple decompression wrapper.
	//
	inline static result<std::vector<uint8_t>> decompress( const void* data, size_t len )
	{
		result<std::vector<uint8_t>> res;
		res.status = ZSTD_getFrameContentSize( data, len );
		if ( res.status.is_error() )
			return res;
		auto& buffer = res.result.emplace( std::vector<uint8_t>( res.status, 0 ) );
		res.status = ZSTD_decompress( buffer.data(), buffer.size(), data, len );
		return res;
	}
	template<Iterable T>
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

		// Simple compression.
		//
		inline result<std::vector<uint8_t>> compress( const void* data, size_t len, int level = XSTD_ZSTD_DEFAULT_LEVEL )
		{
			result<std::vector<uint8_t>> res;
			auto& buffer = res.result.emplace( std::vector<uint8_t>( ZSTD_compressBound( len ), 0 ) );
			res.status = ZSTD_compressCCtx( ctx.get(), buffer.data(), buffer.size(), data, len, level );
			if ( res.status.is_success() )
				res.result->resize( res.status );
			return res;
		}
		template<ContiguousIterable T> requires ( !Pointer<T> )
		inline result<std::vector<uint8_t>> compress( T&& cont, int level = XSTD_ZSTD_DEFAULT_LEVEL )
		{
			return compress( &*std::begin( cont ), std::size( cont ) * sizeof( iterable_val_t<T> ), level );
		}
	};
	
	struct dcontext
	{
		unique_dcontext ctx{ ZSTD_createDCtx() };

		// Simple decompression.
		//
		inline result<std::vector<uint8_t>> decompress( const void* data, size_t len )
		{
			result<std::vector<uint8_t>> res;
			res.status = ZSTD_getFrameContentSize( data, len );
			if ( res.status.is_error() )
				return res;
			auto& buffer = res.result.emplace( std::vector<uint8_t>( res.status, 0 ) );
			res.status = ZSTD_decompressDCtx( ctx.get(), buffer.data(), buffer.size(), data, len );

			return res;
		}
		template<Iterable T>
		inline result<std::vector<uint8_t>> decompress( T&& cont )
		{
			return decompress( &*std::begin( cont ), std::size( cont ) * sizeof( iterable_val_t<T> ) );
		}
	};
};