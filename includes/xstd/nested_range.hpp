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
#include <optional>
#include "range.hpp"
#include "type_helpers.hpp"
#include "intrinsics.hpp"

namespace xstd
{
	// Declare a proxying nested iterator.
	//
	template<typename It, MemberReference F>
	struct nested_iterator
	{
		using Sit =               decltype( std::begin( std::to_address( std::declval<It>() )->*std::declval<F>() ) );

		// Define iterator traits.
		//
		using iterator_category = std::conditional_t<BidirectionalIterable<typename Sit::iterator_category> && 
			                                         BidirectionalIterable<typename It::iterator_category>, std::bidirectional_iterator_tag, std::forward_iterator_tag>;
		using difference_type =   ptrdiff_t;
		using reference =         decltype( *std::declval<Sit>() );
		using value_type =        std::remove_reference_t<reference>;
		using pointer =           value_type*;

		// Holds the range, two levels of iterators and the field.
		//
		const range<It>* top_range;
		It top_iterator;
		std::optional<Sit> sub_iterator;
		F field;

		constexpr nested_iterator( const range<It>* top_range, F field, bool at_end )
			: top_range( top_range ), field( std::move( field ) )
		{
			if ( !at_end )
			{
				top_iterator = top_range->ibegin;
				if ( top_iterator != top_range->iend )
					sub_iterator = std::begin( container() );

				if ( sub_iterator && *sub_iterator == std::end( container() ) )
					operator++();
			}
			else
			{
				top_iterator = top_range->iend;
			}
		}

		// Default copy and move.
		//
		constexpr nested_iterator( nested_iterator&& ) noexcept = default;
		constexpr nested_iterator( const nested_iterator& ) = default;
		constexpr nested_iterator& operator=( nested_iterator&& ) noexcept = default;
		constexpr nested_iterator& operator=( const nested_iterator& ) = default;


		// Support forward/bidirectional iteration.
		//
		constexpr auto& container() { return std::to_address( top_iterator )->*field; }
		constexpr auto& container() const { return std::to_address( top_iterator )->*field; }

		constexpr nested_iterator& operator++()
		{ 
			do
			{
				if ( *sub_iterator == std::end( container() ) )
				{
					sub_iterator = std::nullopt;
					if ( ++top_iterator != top_range->iend )
						sub_iterator = std::begin( container() );
				}
				else
				{
					++*sub_iterator;
				}
			}
			while ( top_iterator != top_range->iend &&
					sub_iterator == std::end( container() ) );
			return *this; 
		}
		constexpr nested_iterator operator++( int ) { auto s = *this; operator++(); return s; }
		constexpr nested_iterator& operator--() requires BidirectionalIterable<iterator_category> 
		{ 
			if ( top_iterator != top_range->iend && 
				 *sub_iterator != std::begin( container() ) )
			{
				--*sub_iterator;
			}
			else
			{
				while( true )
				{
					auto& c = std::to_address( --top_iterator )->*field;
					sub_iterator = std::end( c );
					if ( *sub_iterator != std::begin( c ) )
					{
						--* sub_iterator;
						break;
					}
				}
			}
			return *this; 
		}
		constexpr nested_iterator operator--( int ) requires BidirectionalIterable<iterator_category> { auto s = *this; operator--(); return s; }

		// Equality check against another iterator.
		//
		constexpr bool operator==( const nested_iterator& other ) const { return top_iterator == other.top_iterator && sub_iterator == other.sub_iterator; }
		constexpr bool operator!=( const nested_iterator& other ) const { return top_iterator != other.top_iterator || sub_iterator != other.sub_iterator; }

		// Forward accessor to the sub iterator.
		//
		constexpr reference operator*() const { return **sub_iterator; }
		constexpr decltype( auto ) operator->() const requires MemberPointable<reference> { return std::to_address( *sub_iterator ); }
	};

	// Declare a proxying nested range container.
	//
	template<typename It, MemberReference F>
	struct nested_range
	{
		using iterator =       nested_iterator<It, F>;
		using const_iterator = nested_iterator<It, F>;
		using value_type =     typename iterator::value_type;

		// Constructed by the origin limits and the field.
		//
		range<It> range;
		F field;

		template<Iterable C>
		constexpr nested_range( const C& container, F field )
			: range{ std::begin( container ), std::end( container ) }, field( std::move( field ) ) {}

		// Default copy and move.
		//
		constexpr nested_range( nested_range&& ) noexcept = default;
		constexpr nested_range( const nested_range& ) = default;
		constexpr nested_range& operator=( nested_range&& ) noexcept = default;
		constexpr nested_range& operator=( const nested_range& ) = default;

		// Declare basic container interface.
		//
		constexpr iterator begin() const  { return { &range, field, false }; }
		constexpr iterator end() const { return { &range, field, true }; }
		constexpr size_t size() const { return ( size_t ) std::distance( begin(), end() ); }
		constexpr bool empty() const { return size() == 0; }
		constexpr decltype( auto ) operator[]( size_t n ) const { return *std::next( begin(), n ); }
	};

	// Declare the deduction guide.
	//
	template<typename C, typename F> requires (Iterable<C> && MemberReference<F>)
	nested_range( C, F )->nested_range<iterator_type_t<C>, F>;
};