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

namespace xstd
{
	// Constexpr implementations of cstring functions.
	//
	constexpr static uint32_t cxlower( uint32_t v )
	{
		if ( 'A' <= v && v <= 'Z' )
			return v - 'A' + 'a';
		return v;
	}

	template<String T>
	constexpr static size_t strlen( T&& str )
	{
		return string_view_t<T>{ str }.size();
	}

	// Case/Width insensitive hasher.
	//
	template<String T>
	struct ihash
	{
		constexpr hash_t operator()( const T& value ) const noexcept
		{
			string_view_t<T> view = { value };
			hash_t h = {};
			for ( auto c : view )
				h.add_bytes( cxlower( c ) );
			return h;
		}
	};
	template<String T> static constexpr hash_t make_ihash( const T& value ) noexcept { return ihash<T>{}( value ); }

	// Width insensitive hasher.
	//
	template<String T>
	struct xhash
	{
		constexpr hash_t operator()( const T& value ) const noexcept
		{
			string_view_t<T> view = { value };
			hash_t h = {};
			for ( auto c : view )
				h.add_bytes( ( int ) c );
			return h;
		}
	};
	template<String T> static constexpr hash_t make_xhash( const T& value ) noexcept { return xhash<T>{}( value ); }

	// Case insensitive string operations.
	//
	template<String S>
	static inline std::basic_string<string_unit_t<S>> to_lower( S&& str )
	{
		std::basic_string<string_unit_t<S>> result{ string_view_t<S>{str} };
		for ( auto& c : result )
			c = cxlower( c );
		return result;
	}
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
	static constexpr size_t ifind( S1&& in, S2&& v )
	{
		string_view_t<S1> inv = { in };
		string_view_t<S2> sv = { v };
		
		ptrdiff_t diff = inv.length() - sv.length();
		for ( ptrdiff_t n = 0; n <= diff; n++ )
			if ( !istrcmp( inv.substr( n, sv.length() ), sv ) )
				return ( size_t ) n;
		return std::string::npos;
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

	// Text splitting.
	//
	template<typename C = char, String R = std::basic_string_view<C>>
	static std::vector<R> split_string( std::basic_string_view<C> in, char by )
	{
		std::vector<R> result;
		while ( !in.empty() )
		{
			size_t s = in.find( by );
			if ( s == std::string::npos )
			{
				result.push_back( R{ in } );
				break;
			}
			result.push_back( R{ in.substr( 0, s ) } );
			in.remove_prefix( s + 1 );
		}
		return result;
	}
	template<typename C = char, String R = std::basic_string_view<C>>
	static std::vector<R> split_lines( std::basic_string_view<C> in )
	{
		std::vector<R> result = split_string<C, R>( in, '\n' );
		for ( auto& chunk : result )
		{
			if ( !chunk.empty() && chunk.back() == '\r' )
			{
				if constexpr ( CppStringView<R> )
					chunk.remove_suffix( 1 );
				else if constexpr ( CppString<R> )
					chunk.pop_back();
				else
					unreachable();
			}
		}
		return result;
	}

	// Simple number to string conversation, no input validation.
	//  - Format => {0o 0x <>} (+/-) + [0-9] + '.' + [0-9]
	//
	template<typename T = uint64_t, String S1 = std::string_view> requires ( Integral<T> || FloatingPoint<T> )
	static constexpr T parse_number( S1&& str, int default_base = 10 )
	{
		using I = std::conditional_t<Unsigned<T>, convert_int_t<T>, T>;
		
		string_view_t<S1> view{ str };
		if ( view.empty() ) return 0;
		
		// Parse the sign.
		//
		I sign = +1;
		if ( view.front() == '-' )
		{
			sign = -1;
			view.remove_prefix( 1 );
		}
		else if ( view.front() == '+' )
		{
			view.remove_prefix( 1 );
		}

		// Find out the mode.
		//
		int base = default_base;
		if ( view.size() >= 2 )
		{
			if ( view[ 0 ] == '0' && cxlower( view[ 1 ] ) == 'x' )
				base = 16, view.remove_prefix( 2 );
			else if ( view[ 0 ] == '0' && cxlower( view[ 1 ] ) == 'o' )
				base = 8, view.remove_prefix( 2 );
		}

		// Declare digit parser.
		//
		T value = 0;
		auto parse_digit = [ hex = ( base == 16 ) ] ( char c )
		{
			if ( hex )
			{
				if ( c >= 'a' ) return 10 + ( c - 'a' );
				if ( c >= 'A' ) return 10 + ( c - 'A' );
			}
			return c - '0';
		};

		// Parse the body.
		//
		while ( !view.empty() )
		{
			if ( view.front() == '.' )
			{
				if constexpr ( FloatingPoint<T> )
				{
					view.remove_prefix( 1 );

					T mantissa = 0;
					while ( !view.empty() )
					{
						mantissa += parse_digit( view.back() );
						mantissa /= base;
						view.remove_suffix( 1 );
					}
					value += mantissa;
				}
				break;
			}

			value *= base;
			value += parse_digit( view.front() );
			view.remove_prefix( 1 );
		}
		return value * sign;
	}
};



#if ( defined(__INTELLISENSE__) || !GNU_COMPILER )
	#define MAKE_HASHER( op, fn )                                         \
	constexpr xstd::hash_t operator"" op( const char* str, size_t n )     \
	{                                                                     \
		return fn<std::basic_string_view<char>>{}( { str, n } );          \
	}                                                                     \
	constexpr xstd::hash_t operator"" op( const wchar_t* str, size_t n )  \
	{                                                                     \
		return fn<std::basic_string_view<wchar_t>>{}( { str, n } );       \
	}                                                                    
#else
	#define MAKE_HASHER( op, fn )                                                                        \
	template<typename T, T... chars>                                                                     \
	constexpr xstd::hash_t operator"" op()                                                              \
	{                                                                                                    \
		constexpr T str[] = { chars... };                                                                \
		constexpr xstd::hash_t hash = fn<std::basic_string_view<T>>{}( { str, sizeof...( chars ) } );    \
		return xstd::hash_t{ xstd::const_tag<hash.as64()>::value };                                      \
	}
#endif
	MAKE_HASHER( _ihash, xstd::ihash );
	MAKE_HASHER( _xhash, xstd::xhash );
#undef MAKE_HASHER