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

		string_result<std::vector<uint8_t>> res;
		std::vector<uint8_t>& buffer = res.result.emplace();
		do
		{
			size_t old_size = buffer.size();
			size_t new_size = old_size ? 256 + old_size + old_size / 2 : deflateBound( &stream, stream.avail_in );
			buffer.resize( new_size );
			stream.avail_out = narrow_cast<uInt>( new_size - old_size );
			stream.next_out = buffer.data() + old_size;

			int state = deflate( &stream, Z_FINISH );
			if ( state != Z_STREAM_END && state != Z_OK && state != Z_BUF_ERROR )
			{
				res.status.assign( stream.msg );
				if ( res ) res = XSTD_ESTR( "Unknown compression error." );
				deflateEnd( &stream );
				return res;
			}
		}
		while ( stream.avail_out == 0 );
		deflateEnd( &stream );

		buffer.resize( buffer.size() - stream.avail_out );
		res.status.clear();
		return res;
	}
	template<ContiguousIterable T> requires ( !Pointer<T> )
	inline static string_result<std::vector<uint8_t>> compress( T&& cont, int level = XSTD_GZIP_DEFAULT_LEVEL, int strategy = Z_DEFAULT_STRATEGY )
	{
		return compress( &*std::begin( cont ), std::size( cont ) * sizeof( iterable_val_t<T> ), level, strategy );
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

		string_result<std::vector<uint8_t>> res;
		std::vector<uint8_t>& buffer = res.result.emplace();
		do
		{
			size_t old_size = buffer.size();
			size_t new_size = old_size ? old_size * 2 : 64 + len * 2;
			buffer.resize( new_size );
			stream.avail_out = narrow_cast< uInt >( new_size - old_size );
			stream.next_out = buffer.data() + old_size;

			int state = inflate( &stream, Z_FINISH );
			if ( state != Z_STREAM_END && state != Z_OK && state != Z_BUF_ERROR )
			{
				res.status.assign( stream.msg );
				if ( res ) res = XSTD_ESTR( "Unknown decompression error." );
				inflateEnd( &stream );
				return res;
			}
		}
		while ( stream.avail_out == 0 );
		inflateEnd( &stream );

		buffer.resize( buffer.size() - stream.avail_out );
		res.status.clear();
		return res;
	}
	template<Iterable T>
	inline static string_result<std::vector<uint8_t>> decompress( T&& cont )
	{
		return decompress( &*std::begin( cont ), std::size( cont ) * sizeof( iterable_val_t<T> ) );
	}
};