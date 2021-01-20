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
	template<typename It, typename F>
	struct nested_iterator
	{
		using Sit =               decltype( std::begin( std::declval<F>()( *std::declval<It>() ) ) );

		// Define iterator traits.
		//
		using iterator_category = std::conditional_t<BidirectionalIterable<typename Sit::iterator_category> && 
			                                         BidirectionalIterable<typename It::iterator_category>, std::bidirectional_iterator_tag, std::forward_iterator_tag>;
		using difference_type =   ptrdiff_t;
		using reference =         decltype( *std::declval<Sit>() );
		using value_type =        std::remove_reference_t<reference>;
		using pointer =           value_type*;

		// Holds the range, two levels of iterators and the accessor.
		//
		const range<It>* top_range;
		It top_iterator;
		std::optional<Sit> sub_iterator;
		const F* accessor;

		constexpr nested_iterator( const range<It>* top_range, const F* accessor, bool at_end )
			: top_range( top_range ), accessor( accessor )
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
		constexpr auto& container() { return ( *accessor )( *top_iterator ); }
		constexpr auto& container() const { return ( *accessor )( *top_iterator ); }

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
					auto& c = ( *accessor )( *--top_iterator );
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
	template<typename It, typename F>
	struct nested_range
	{
		using iterator =       nested_iterator<It, F>;
		using const_iterator = nested_iterator<It, F>;
		using value_type =     typename iterator::value_type;

		// Constructed by the origin limits and the accessor.
		//
		range<It> range;
		F accessor;

		template<Iterable C>
		constexpr nested_range( const C& container, F accessor )
			: range{ std::begin( container ), std::end( container ) }, accessor( std::move( accessor ) ) {}

		// Default copy and move.
		//
		constexpr nested_range( nested_range&& ) noexcept = default;
		constexpr nested_range( const nested_range& ) = default;
		constexpr nested_range& operator=( nested_range&& ) noexcept = default;
		constexpr nested_range& operator=( const nested_range& ) = default;

		// Declare basic container interface.
		//
		constexpr iterator begin() const  { return { &range, &accessor, false }; }
		constexpr iterator end() const { return { &range, &accessor, true }; }
		constexpr size_t size() const { return ( size_t ) std::distance( begin(), end() ); }
		constexpr bool empty() const { return size() == 0; }
		constexpr decltype( auto ) operator[]( size_t n ) const { return *std::next( begin(), n ); }
	};

	// Declare the deduction guide.
	//
	template<typename C, typename F> requires Iterable<C>
	nested_range( C, F )->nested_range<iterator_type_t<C>, F>;
};