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
		// Boolean convertible iterator wrapper.
		//
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

		// Declare a proxying filter container.
		//
		template<typename It, typename F>
		struct filter_proxy
		{
			// Declare proxying iterator.
			//
			struct iterator
			{
				// Define iterator traits.
				//
				using iterator_category = typename std::forward_iterator_tag;
				using difference_type =   typename std::iterator_traits<std::remove_cvref_t<It>>::difference_type;
				using reference =         decltype( *std::declval<It>() );
				using value_type =        std::remove_reference_t<reference>;
				using pointer =           value_type*;
				
				// Constructed by the original iterator a limit and a reference to predicate.
				//
				It at;
				It end;
				const F& predicate;
				constexpr iterator( const It& at, const It& end, const F& predicate ) : at( at ), end( end ), predicate( predicate ) 
				{
					if ( at != end && !predicate( *at ) )
						operator++();
				}

				// Support bidirectional/random iteration.
				//
				constexpr iterator& operator++() 
				{ 
					++at;
					while ( at != end && !predicate( *at ) )
						++at;
					return *this; 
				}
				constexpr iterator operator++( int ) { auto s = *this; operator++(); return s; }

				// Equality check against another iterator.
				//
				constexpr bool operator==( const iterator& other ) const { return at == other.at; }
				constexpr bool operator!=( const iterator& other ) const { return at != other.at; }

				// Default accessors as is.
				//
				constexpr reference operator*() const { return *at; }
				constexpr pointer operator->() const requires MemberPointable<const It&> { return std::to_address( at ); }
			};
			using const_iterator = iterator;

			// Holds the predicate and the limits.
			//
			F predicate;
			It ibegin;
			It iend;

			// Declare basic container interface.
			//
			constexpr iterator begin() const { return { ibegin, iend, predicate }; }
			constexpr iterator end() const   { return { iend, iend, predicate }; }
			constexpr size_t size() const    { return ( size_t ) std::distance( begin(), end() ); } // O(N) warning!
			constexpr bool empty() const     { return begin() == end(); } // O(1)
			constexpr decltype( auto ) operator[]( size_t n ) const { return *std::next( begin(), n ); } // O(N) warning!
		};
	};

	// Find if variants taking containers instead of iterators and returning bool convertible iterators.
	//
	template<Iterable T, typename V>
	static constexpr auto find( T&& container, V&& value ) -> impl::result_iterator<iterator_type_t<T>>
	{
		return { std::find( std::begin( container ), std::end( container ), std::forward<V>( value ) ), std::end( container ) };
	}
	template<Iterable T, typename Pr>
	static constexpr auto find_if( T&& container, Pr&& predicate ) -> impl::result_iterator<iterator_type_t<T>>
	{
		return { std::find_if( std::begin( container ), std::end( container ), std::forward<Pr>( predicate ) ), std::end( container ) };
	}

	// Counts the number of times a matching value is in the container.
	//
	template<Iterable T, typename Pr>
	static constexpr size_t count_if( T&& container, Pr&& predicate )
	{
		size_t n = 0;
		for ( auto&& other : container )
			if ( predicate( other ) )
				++n;
		return n;
	}
	template<Iterable T, typename V>
	static constexpr size_t count( T&& container, V&& value )
	{
		return count_if( std::forward<T>( container ), [ & ] ( auto&& v ) { return v == value; } );
	}

	// Returns a range containing only the values that pass the predicate. Collect does the 
	// same thing except it has copying behaviour whereas filter is returning a proxy.
	//
	template<Iterable T, typename Pr>
	static constexpr auto filter( T&& container, Pr&& predicate )
	{
		return impl::filter_proxy<iterator_type_t<T>, Pr>{
			std::forward<Pr>( predicate ),
			std::begin( container ),
			std::end( container )
		};
	}
	template<Iterable T, typename Pr>
	static auto collect( T&& container, Pr&& predicate )
	{
		std::vector<iterator_value_type_t<T>> result;
		result.reserve( std::size( container ) );
		for ( auto&& other : container )
			if ( predicate( other ) )
				result.emplace_back( other );
		return result;
	}
};