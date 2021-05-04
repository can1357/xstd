#pragma once
#include <zlib.h>
#include <vector>
#include <string>
#include <cstring>
#include "result.hpp"
#include "type_helpers.hpp"
#include "narrow_cast.hpp"

// [[Configuration]]
// XSTD_GZIP_DEFAULT_LEVEL: Sets the default ZLIB compression level.
//
#ifndef XSTD_GZIP_DEFAULT_LEVEL
	#define XSTD_GZIP_DEFAULT_LEVEL Z_BEST_COMPRESSION
#endif

namespace xstd::gzip
{
	// Compression wrapper.
	//
	inline static string_result<std::vector<uint8_t>> compress( const void* data, size_t len, int level = XSTD_GZIP_DEFAULT_LEVEL, int strategy = Z_DEFAULT_STRATEGY )
	{
		z_stream stream;
		memset( &stream, 0, sizeof( z_stream ) );
		if ( deflateInit2( &stream, level, Z_DEFLATED, 16 + MAX_WBITS, 8, strategy ) != Z_OK )
			return {};
		stream.next_in = ( uint8_t* ) data;
		stream.avail_in = narrow_cast<uInt>( len );

		std::vector<uint8_t> result = {};
		do
		{
			size_t extension_space = result.empty() ? deflateBound( &stream, stream.avail_in ) : 256 + result.size() / 2;
			result.resize( result.size() + extension_space );
			stream.avail_out = narrow_cast<uInt>( extension_space );
			stream.next_out = result.data() + result.size() - extension_space;

			int state = deflate( &stream, Z_FINISH );
			if ( state != Z_STREAM_END && state != Z_OK && state != Z_BUF_ERROR )
			{
				std::string res = stream.msg;
				deflateEnd( &stream );
				return { std::move( res ) };
			}
		}
		while ( stream.avail_out == 0 );
		deflateEnd( &stream );

		result.resize( result.size() - stream.avail_out );
		return result;
	}
	
	template<Iterable T> requires ( is_contiguous_iterable_v<T> && !Pointer<T> )
	inline static string_result<std::vector<uint8_t>> compress( T&& cont, int level = XSTD_GZIP_DEFAULT_LEVEL, int strategy = Z_DEFAULT_STRATEGY )
	{
		return compress( &*std::begin( cont ), std::size( cont ) * sizeof( iterator_value_type_t<T> ), level, strategy );
	}

	// Decompression wrapper.
	//
	inline static string_result<std::vector<uint8_t>> decompress( const void* data, size_t len )
	{
		z_stream stream;
		memset( &stream, 0, sizeof( z_stream ) );
		if ( inflateInit2( &stream, 32 + MAX_WBITS ) != Z_OK )
			return {};
		stream.next_in = ( uint8_t* ) data;
		stream.avail_in = narrow_cast<uInt>( len );
		
		std::vector<uint8_t> result = {};
		do
		{
			size_t extension_space = result.empty() ? len * 2 : 256 + result.size() * 2;
			result.resize( result.size() + extension_space );
			stream.avail_out = narrow_cast<uInt>( extension_space );
			stream.next_out = result.data() + result.size() - extension_space;

			int state = inflate( &stream, Z_FINISH );
			if ( state != Z_STREAM_END && state != Z_OK && state != Z_BUF_ERROR )
			{
				std::string res = stream.msg;
				inflateEnd( &stream );
				return { std::move( res ) };
			}
		}
		while ( stream.avail_out == 0 );
		inflateEnd( &stream );

		result.resize( result.size() - stream.avail_out );
		return result;
	}
	template<Iterable T>
	inline static string_result<std::vector<uint8_t>> decompress( T&& cont )
	{
		return decompress( &*std::begin( cont ), std::size( cont ) * sizeof( iterator_value_type_t<T> ) );
	}
};