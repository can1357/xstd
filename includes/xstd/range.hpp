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
#include <type_traits>
#include "type_helpers.hpp"
#include "intrinsics.hpp"

namespace xstd
{
	namespace impl
	{
		// Dummy transformation.
		//
		struct no_transform
		{
			template<typename T> __forceinline T operator()( T&& x ) const noexcept { return x; }
		};
	};

	// Declare a proxying range iterator.
	//
	template<typename It, typename F = impl::no_transform>
	struct range_iterator
	{
		// Define iterator traits.
		//
		using iterator_category = typename std::iterator_traits<std::remove_cvref_t<It>>::iterator_category;
		using difference_type =   typename std::iterator_traits<std::remove_cvref_t<It>>::difference_type;
		using reference =         decltype( std::declval<F>()( *std::declval<It>() ) );
		using value_type =        std::remove_reference_t<reference>;
		using pointer =           value_type*;
		
		// Constructed by the original iterator and a reference to transformation function.
		//
		It at;
		const F* transform;
		constexpr range_iterator( It i, const F* transform ) : at( std::move( i ) ), transform( transform ) {}

		// Default copy/move.
		//
		constexpr range_iterator( range_iterator&& ) noexcept = default;
		constexpr range_iterator( const range_iterator& ) = default;

		// Support bidirectional/random iteration.
		//
		constexpr range_iterator& operator++() { ++at; return *this; }
		constexpr range_iterator operator++( int ) { auto s = *this; operator++(); return s; }
		constexpr range_iterator& operator--() requires BidirectionalIterable<iterator_category> { --at; return *this; }
		constexpr range_iterator operator--( int ) requires BidirectionalIterable<iterator_category> { auto s = *this; operator--(); return s; }
		constexpr range_iterator& operator+=( difference_type d ) requires RandomIterable<iterator_category> { at += d; return *this; }
		constexpr range_iterator& operator-=( difference_type d ) requires RandomIterable<iterator_category> { at -= d; return *this; }
		constexpr range_iterator operator+( difference_type d ) const requires RandomIterable<iterator_category> { auto s = *this; operator+=( d ); return s; }
		constexpr range_iterator operator-( difference_type d ) const requires RandomIterable<iterator_category> { auto s = *this; operator-=( d ); return s; }

		// Equality check against another iterator and difference.
		//
		constexpr bool operator==( const range_iterator& other ) const { return at == other.at; }
		constexpr bool operator!=( const range_iterator& other ) const { return at != other.at; }
		constexpr difference_type operator-( const range_iterator& other ) const requires Subtractable<It, It> { return at - other.at; }

		// Override accessor to apply transformations where relevant.
		//
		constexpr reference operator*() const { return (*transform)( *at ); }
		constexpr decltype( auto ) operator->() const requires MemberPointable<reference> { return (*transform)( *at ); }

		// Getter of the origin without transformations.
		//
		constexpr decltype( auto ) origin() const { return *at; }
	};

	// Declare a proxying range container.
	//
	template<typename It, typename F = impl::no_transform>
	struct range
	{
		using iterator =       range_iterator<It, F>;
		using const_iterator = range_iterator<It, F>;
		using value_type =     typename iterator::value_type;

		// Holds transformation and the limits.
		//
		It ibegin;
		It iend;
		F transform;

		// Construct by beginning, end, and transformation.
		//
		constexpr range( It begin, It end, F fn ) 
			: ibegin( std::move( begin ) ), iend( std::move( end ) ), transform( std::move( fn ) ) {}

		// Construct by container and transformation.
		//
		template<Iterable C>
		constexpr range( const C& container, F fn ) 
			: ibegin( std::begin( container ) ), iend( std::end( container ) ), transform( std::move( fn ) ) {}

		// Construct by beginning and end.
		//
		constexpr range( It begin, It end ) 
			: ibegin( std::move( begin ) ), iend( std::move( end ) ) {}

		// Default copy and move.
		//
		constexpr range( range&& ) noexcept = default;
		constexpr range( const range& ) = default;
		constexpr range& operator=( range&& ) noexcept = default;
		constexpr range& operator=( const range& ) = default;

		// Declare basic container interface.
		//
		constexpr iterator begin() const  { return { ibegin, &transform }; }
		constexpr iterator end() const { return { iend, &transform }; }
		constexpr size_t size() const { return ( size_t ) std::distance( ibegin, iend ); }
		constexpr bool empty() const { return ibegin == iend; }
		constexpr decltype( auto ) operator[]( size_t n ) const { return transform( *std::next( ibegin, n ) ); }
	};

	// Declare the deduction guides.
	//
	template<typename It1, typename It2, typename Fn>
	range( It1, It2, Fn )->range<It1, Fn>;
	template<typename C, typename Fn> requires Iterable<C>
	range( C, Fn )->range<iterator_type_t<C>, Fn>;
	template<typename It1, typename It2> requires ( !Iterable<It1> )
	range( It1, It2 )->range<It1>;

	// Old style functors.
	//
	template<typename It1, typename It2, typename Fn>
	static constexpr auto make_range( It1&& begin, It2&& end, Fn&& f )
	{
		using It = std::conditional_t<Convertible<It1, It2>, It2, It1>;

		return range<It, Fn>{
				std::forward<It1>( begin ),
				std::forward<It2>( end ),
				std::forward<Fn>( f )
		};
	}
	template<typename It1, typename It2>
	static constexpr auto make_range( It1&& begin, It2&& end )
	{
		using It = std::conditional_t<Convertible<It1, It2>, It2, It1>;

		return range<It, impl::no_transform>{
				std::forward<It1>( begin ),
				std::forward<It2>( end ),
				impl::no_transform{}
		};
	}
	template<Iterable C, typename Fn>
	static constexpr auto make_view( C&& container, Fn&& f )
	{
		return range<iterator_type_t<C>, Fn>{
				std::begin( container ),
				std::end( container ),
				std::forward<Fn>( f )
		};
	}
	template<Iterable C, typename B, typename F>
	static constexpr auto make_view( C&& container, member_reference_t<B, F> ref )
	{
		auto fn = [ ref = std::move( ref ) ] <typename T> ( T && v ) -> decltype( auto )
		{
			if constexpr ( std::is_base_of_v<B, T> )
				return v.*ref;
			else
				return std::to_address( v )->*ref;
		};
		return range<iterator_type_t<C>, decltype( fn )>{
				std::begin( container ),
				std::end( container ),
				std::move( fn )
		};
	}

	// Returns a reversed container.
	//
	template<Iterable C>
	static constexpr auto backwards( C&& container )
	{
		return range<std::reverse_iterator<iterator_type_t<C>>, impl::no_transform>{
			std::reverse_iterator( std::end( container ) ),
			std::reverse_iterator( std::begin( container ) ),
			impl::no_transform{}
		};
	}
};