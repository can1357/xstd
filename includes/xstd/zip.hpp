#pragma once
#include <tuple>
#include <utility>
#include <stdint.h>
#include "type_helpers.hpp"

namespace xstd
{
	template<Iterable... Tx>
	struct joint_container
	{
		using iterator_array =  std::tuple<decltype( std::begin( std::declval<const Tx&>() ) )...>;
		using reference_array = std::tuple<decltype( *std::begin( std::declval<const Tx&>() ) )...>;

		// Declare the iterator type.
		//
		struct base_iterator
		{
			// Generic iterator typedefs.
			//
			using iterator_category = std::bidirectional_iterator_tag;
			using difference_type =   int64_t;
			using reference =         reference_array;
			using value_type =        reference_array;
			using pointer =           reference_array*;

			// Iterators.
			//
			iterator_array iterators;

			// Support bidirectional iteration.
			//
			constexpr base_iterator& operator++() { std::apply( [ ] ( auto&... it ) { ( ( ++it ), ... ); }, iterators ); return *this; }
			constexpr base_iterator& operator--() { std::apply( [ ] ( auto&... it ) { ( ( --it ), ... ); }, iterators ); return *this; }
			constexpr base_iterator operator++( int ) { auto s = *this; operator++(); return s; }
			constexpr base_iterator operator--( int ) { auto s = *this; operator--(); return s; }

			// Equality check against another iterator.
			//
			constexpr bool operator==( const base_iterator& other ) const { return std::get<0>( iterators ) == std::get<0>( other.iterators ); }
			constexpr bool operator!=( const base_iterator& other ) const { return std::get<0>( iterators ) != std::get<0>( other.iterators ); }

			// Redirect dereferencing to container.
			//
			constexpr reference_array operator*() const { return std::apply( [ ] ( auto&... it ) { return reference_array{ *it... }; }, iterators ); }
		};
		using iterator =       base_iterator;
		using const_iterator = base_iterator;

		// Tuple containing data sources, length of iteration range, pre-computed begin and end.
		//
		const std::tuple<Tx...> sources;
		const size_t length;
		const iterator begin_p;
		const iterator end_p;

		template<typename... Tv>
		constexpr joint_container( Tv&&... sc )
			: sources( std::forward<Tv>( sc )... ),
			  length(  std::size( std::get<0>( sources ) ) ),
			  begin_p( std::apply( [ & ] ( auto&... src ) { return iterator{ { std::begin( src )... }                      }; }, sources ) ),
			  end_p(   std::apply( [ & ] ( auto&... src ) { return iterator{ { std::next( std::begin( src ), length )... } }; }, sources ) ) {}

		// Generic container interface.
		//
		constexpr size_t size() const     { return length; }
		constexpr iterator begin() const  { return begin_p; }
		constexpr iterator end() const    { return end_p; }
		constexpr decltype( auto ) operator[]( size_t n ) const { return *std::next( begin(), n ); }
	};

	template <Iterable... Tx>
	static constexpr joint_container<Tx...> zip( Tx&&... args ) { return { std::forward<Tx>( args )... }; }
};