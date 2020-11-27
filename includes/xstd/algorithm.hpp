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
#include <iterator>
#include <limits>
#include "type_helpers.hpp"

namespace xstd
{
	namespace impl
	{
		template<typename T>
		struct result_iterator
		{
			T it, end;
			
			constexpr operator bool() const { return it != end; }
			constexpr decltype( auto ) operator->() const requires MemberPointable<const T&> { return std::to_address( it ); }
			constexpr decltype( auto ) operator*() const requires Dereferencable<const T&> { return *it; }
			constexpr decltype( auto ) operator->() requires MemberPointable<T&> { return std::to_address( it ); }
			constexpr decltype( auto ) operator*() requires Dereferencable<T&> { return *it; }
		};
	};

	// Find if variants taking containers instead of iterators and returning bool convertible iterators.
	//
	template<Iterable T, typename V>
	static constexpr auto find( T&& c, V&& value ) -> impl::result_iterator<iterator_type_t<T>>
	{
		return { std::find( std::begin( c ), std::end( c ), std::forward<V>( value ) ), std::end( c ) };
	}
	template<Iterable T, typename Pr>
	static constexpr auto find_if( T&& c, Pr&& predicate ) -> impl::result_iterator<iterator_type_t<T>>
	{
		return { std::find_if( std::begin( c ), std::end( c ), std::forward<Pr>( predicate ) ), std::end( c ) };
	}
};