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
#include "type_helpers.hpp"
#include "hashable.hpp"

namespace xstd::text
{
	constexpr static uint32_t cxlower( uint32_t v )
	{
		if ( 'A' <= v && v <= 'Z' )
			return v - 'A' + 'a';
		return v;
	}

	// Case/Width insensitive hasher.
	//
	template<String T>
	struct ihash
	{
		constexpr auto operator()( const T& value ) const noexcept
		{
			string_view_t<T> view = { value };
			hash_t h = {};
			for ( auto c : view )
				h.add_bytes( cxlower( c ) );
			return h;
		}
	};

	// Width insensitive hasher.
	//
	template<String T>
	struct xhash
	{
		constexpr auto operator()( const T& value ) const noexcept
		{
			string_view_t<T> view = { value };
			hash_t h = {};
			for ( auto c : view )
				h.add_bytes( ( int ) c );
			return h;
		}
	};

#if ( defined(__INTELLISENSE__) || !GNU_COMPILER )
	#define MAKE_HASHER( op, fn )                                        \
	_CONSTEVAL hash_t operator"" op( const char* str, size_t n )         \
	{                                                                    \
		return fn<std::basic_string_view<char>>{}( { str, n } );         \
	}                                                                    \
	_CONSTEVAL hash_t operator"" op( const wchar_t* str, size_t n )      \
	{                                                                    \
		return fn<std::basic_string_view<wchar_t>>{}( { str, n } );      \
	}                                                                    
#else
	#define MAKE_HASHER( op, fn )                                                                  \
	template<typename T, T... chars>                                                               \
	_CONSTEVAL auto operator"" op()                                                                \
	{                                                                                              \
		constexpr T str[] = { chars... };                                                          \
		constexpr hash_t hash = fn<std::basic_string_view<T>>{}( { str, sizeof...( chars ) } );    \
		return hash_t{ const_tag<hash.as64()>::value };                                            \
	}
#endif
	MAKE_HASHER( _ihash, ihash );
	MAKE_HASHER( _xhash, xhash );
#undef MAKE_HASHER

	// Case insensitive string comparison.
	//
	template<String S1, String S2>
	static constexpr int istrcmp( S1&& a, S2&& b )
	{
		string_view_t<S1> av = { a };
		string_view_t<S2> bv = { b };
		if ( int d = ( int ) av.length() - ( int ) bv.length(); d != 0 )
			return d;
		for ( size_t n = 0; n != av.length(); n++ )
		{
			if ( int d = cxlower( av[ n ] ) - cxlower( bv[ n ] ); d != 0 )
				return d;
		}
		return 0;
	}
	template<String S1, String S2> 
	static constexpr bool iequals( S1&& a, S2&& b ) 
	{ 
		return istrcmp<S1, S2>( std::forward<S1>( a ), std::forward<S2>( b ) ) == 0; 
	}
	template<String S1, String S2>
	static constexpr bool istarts_with( S1&& a, S2&& b )
	{
		string_view_t<S1> av = { a };
		string_view_t<S2> bv = { b };
		if ( av.length() < bv.length() )
			return false;
		av.remove_suffix( av.length() - bv.length() );
		return istrcmp( av, bv ) == 0;
	}
	template<String S1, String S2>
	static constexpr bool iends_with( S1&& a, S2&& b )
	{
		string_view_t<S1> av = { a };
		string_view_t<S2> bv = { b };
		if ( av.length() < bv.length() )
			return false;
		av.remove_prefix( av.length() - bv.length() );
		return istrcmp( av, bv ) == 0;
	}
};