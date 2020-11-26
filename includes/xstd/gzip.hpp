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
#include <zlib.h>
#include <vector>
#include <string>
#include <cstring>
#include "result.hpp"
#include "type_helpers.hpp"

namespace xstd::gzip
{
	// Compression wrapper.
	//
	inline static string_result<std::vector<uint8_t>> compress( const void* data, size_t len,
												                int level = Z_DEFAULT_COMPRESSION, int strategy = Z_DEFAULT_STRATEGY )
	{
		z_stream stream;
		memset( &stream, 0, sizeof( z_stream ) );
		if ( deflateInit2( &stream, level, Z_DEFLATED, 16 + MAX_WBITS, 8, strategy ) != Z_OK )
			return {};
		stream.next_in = ( uint8_t* ) data;
		stream.avail_in = len;

		std::vector<uint8_t> result = {};
		do
		{
			size_t extension_space = 256 + result.size() / 2;
			result.resize( result.size() + extension_space );
			stream.avail_out = extension_space;
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
	
	template<Iterable T> requires is_contiguous_iterable_v<T>
	inline static string_result<std::vector<uint8_t>> compress( T&& cont, int level = Z_DEFAULT_COMPRESSION, int strategy = Z_DEFAULT_STRATEGY )
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
		stream.avail_in = len;
		
		std::vector<uint8_t> result = {};
		do
		{
			size_t extension_space = 256 + result.size() * 2;
			result.resize( result.size() + extension_space );
			stream.avail_out = extension_space;
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
};