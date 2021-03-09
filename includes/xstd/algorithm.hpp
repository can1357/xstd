#pragma once
#include <iterator>
#include <limits>
#include <optional>
#include "type_helpers.hpp"
#include "range.hpp"

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
		struct skip_range
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
				const F* predicate;
				constexpr iterator( It at, It end, const F* predicate ) : at( std::move( at ) ), end( std::move( end ) ), predicate( predicate )
				{ 
					if ( at != end ) 
						next(); 
				}

				// Default copy/move.
				//
				constexpr iterator( iterator&& ) noexcept = default;
				constexpr iterator( const iterator& ) = default;
				constexpr iterator& operator=( iterator&& ) noexcept = default;
				constexpr iterator& operator=( const iterator& ) = default;

				// Support forward iteration.
				//
				constexpr iterator& next()
				{
					while ( at != end && !(*predicate)( at ) )
						++at;
					return *this;
				}
				constexpr iterator& operator++() { ++at; next(); return *this; }
				constexpr iterator operator++( int ) { auto s = *this; operator++(); return s; }

				// Equality check against another iterator and difference.
				//
				constexpr bool operator==( const iterator& other ) const { return at == other.at; }
				constexpr bool operator!=( const iterator& other ) const { return at != other.at; }
				constexpr difference_type operator-( const iterator& other ) const requires Subtractable<It, It> { return at - other.at; }

				// Default accessors as is.
				//
				constexpr reference operator*() const { return *at; }
				constexpr pointer operator->() const requires MemberPointable<const It&> { return std::to_address( at ); }
			};
			using const_iterator = iterator;

			// Holds the predicate and the limits.
			//
			It ibegin;
			It iend;
			F predicate;

			// Construct by beginning, end, and predicate.
			//
			constexpr skip_range( It begin, It end, F predicate )
				: ibegin( std::move( begin ) ), iend( std::move( end ) ), predicate( std::move( predicate ) ) {}

			// Construct by container and predicate.
			//
			template<Iterable C>
			constexpr skip_range( const C& container, F predicate )
				: ibegin( std::begin( container ) ), iend( std::end( container ) ), predicate( std::move( predicate ) ) {}

			// Default copy and move.
			//
			constexpr skip_range( skip_range&& ) noexcept = default;
			constexpr skip_range( const skip_range& ) = default;
			constexpr skip_range& operator=( skip_range&& ) noexcept = default;
			constexpr skip_range& operator=( const skip_range& ) = default;

			// Declare basic container interface.
			//
			constexpr iterator begin() const { return { ibegin, iend, &predicate }; }
			constexpr iterator end() const   { return { iend, iend, &predicate }; }
			constexpr size_t size() const    { return ( size_t ) std::distance( begin(), end() ); } // O(N) warning!
			constexpr bool empty() const     { return begin() == end(); } // O(1)
			constexpr decltype( auto ) operator[]( size_t n ) const { return *std::next( begin(), n ); } // O(N) warning!
		};

		// Declare the deduction guides.
		//
		template<typename It1, typename It2, typename Fn>
		skip_range( It1, It2, Fn )->skip_range<It1, Fn>;
		template<typename C, typename Fn> requires Iterable<C>
		skip_range( C, Fn )->skip_range<iterator_type_t<C>, Fn>;
	};

	// Group by algorithm.
	//
	template<typename T, typename Qr>
	static std::vector<std::vector<T>> group_by( const T& begin, const T& end, Qr&& query )
	{
		size_t size = std::distance( begin, end );

		// Start the list with every iterator being in its own group.
		//
		std::vector<std::vector<T>> group_vec( size );
		std::vector<std::vector<T>*> group_map( size );

		size_t i = 0;
		for ( auto it = begin; it != end; ++it, ++i )
		{
			group_vec[ i ].emplace_back( it );
			group_map[ i ] = &group_vec[ i ];
		}
		auto it_to_group = [ & ] ( auto&& it ) -> std::vector<T>*& { return group_map[ std::distance( begin, it ) ]; };

		// Iterate every element:
		//
		for ( auto it = begin; it != end; ++it )
		{
			auto& group = it_to_group( it );

			// Query which elements we need to group this one with.
			//
			query( it, [ & ] ( auto&& it2 )
			{
				// Join groups together.
				//
				std::vector<T>* gr_big = group;
				std::vector<T>* gr_small = it_to_group( it2 );
				if ( gr_big == gr_small )
					return;
				if ( gr_small->size() > gr_big->size() )
					std::swap( gr_small, gr_big );
				for ( auto& entry : *gr_small )
				{
					it_to_group( entry ) = gr_big;
					gr_big->emplace_back( entry );
				}
				gr_small->clear();
			} );
		}
		std::erase_if( group_vec, [ ] ( auto& e ) { return e.empty(); } );
		return group_vec;
	}
	template<Iterable T, typename Qr>
	static auto group_by( T& container, Qr&& query )
	{
		return group_by( std::begin( container ), std::end( container ), std::forward<Qr>( query ) );
	}

	// Sort/min_element/max_element redirect taking container instead of iterator.
	//
	template<Iterable T, typename Pr = std::less<>>
	static constexpr void sort( T& container, Pr&& predicate = {} )
	{
		std::sort( std::begin( container ), std::end( container ), std::forward<Pr>( predicate ) );
	}
	template<Iterable T, typename Pr = std::less<>>
	static constexpr auto min_element( T&& container, Pr&& predicate = {} ) -> std::optional<iterator_value_type_t<T>>
	{
		if ( auto it = std::min_element( std::begin( container ), std::end( container ), std::forward<Pr>( predicate ) ); it != std::end( container ) )
			return { *it };
		else
			return std::nullopt;
	}
	template<Iterable T, typename Pr = std::less<>>
	static constexpr auto max_element( T&& container, Pr&& predicate = {} ) -> std::optional<iterator_value_type_t<T>>
	{
		if ( auto it = std::max_element( std::begin( container ), std::end( container ), std::forward<Pr>( predicate ) ); it != std::end( container ) )
			return { *it };
		else
			return std::nullopt;
	}

	// Find if variants taking containers instead of iterators and returning bool convertible iterators.
	//
	template<Iterable T, typename Pr>
	static constexpr auto find_if( T&& container, Pr&& predicate ) -> impl::result_iterator<iterator_type_t<T>>
	{
		return { std::find_if( std::begin( container ), std::end( container ), std::forward<Pr>( predicate ) ), std::end( container ) };
	}
	template<Iterable T, typename V>
	static constexpr auto find( T&& container, V&& value ) -> impl::result_iterator<iterator_type_t<T>>
	{
		return { std::find( std::begin( container ), std::end( container ), std::forward<V>( value ) ), std::end( container ) };
	}

	// Binary search implemented similary to find.
	//
	template<Iterable T, typename V>
	static constexpr auto bsearch( T&& container, V&& value ) -> impl::result_iterator<iterator_type_t<T>>
	{
		auto beg = std::begin( container );
		auto end = std::end( container );
		auto it = std::lower_bound( beg, end, value );
		if ( it != end && *it == value )
			return { it, end };
		else
			return { end, end };
	}

	// Invokes into custom searching logic implemented by container.
	//
	template<Iterable T, typename V>
	static constexpr auto find_spec( T&& container, V&& value ) -> impl::result_iterator<iterator_type_t<T>>
	{
		return { container.find( std::forward<V>( value ) ), std::end( container ) };
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
	template<Iterable T, typename Pr>
	static constexpr bool contains_if( T&& container, Pr&& predicate )
	{
		for ( auto&& other : container )
			if ( predicate( other ) )
				return true;
		return false;
	}
	template<Iterable T, typename V>
	static constexpr size_t count( T&& container, V&& value )
	{
		return count_if( std::forward<T>( container ), [ & ] ( const auto& v ) { return v == value; } );
	}
	template<Iterable T, typename V>
	static constexpr bool contains( T&& container, V&& value )
	{
		return std::find( std::begin( container ), std::end( container ), std::forward<V>( value ) ) != std::end( container );
	}

	// Returns a range containing only the values that pass the predicate. Collect does the 
	// same thing except it has copying behaviour whereas filter is returning a proxy. _i prefix
	// implies predicate takes an iterator instead.
	//
	template<Iterable T, typename Pr>
	static constexpr auto filter_i( T&& container, Pr&& predicate )
	{
		return impl::skip_range<iterator_type_t<T>, Pr>{
			std::begin( container ),
			std::end( container ),
			std::forward<Pr>( predicate )
		};
	}
	template<Iterable T, typename Pr>
	static constexpr auto filter( T&& container, Pr&& predicate )
	{
		return filter_i( std::forward<T>( container ), [ p = std::forward<Pr>( predicate ) ] ( const auto& at ) { return p( *at ); } );
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

	// Filters the stream into unique values.
	//
	template<Iterable T>
	static constexpr auto unique( T&& container )
	{
		return filter_i( std::forward<T>( container ), [ bg = std::begin( container ) ] ( const auto& cur )
		{
			for ( auto it = bg; it != cur; ++it )
				if ( *it == *cur )
					return false;
			return true;
		} );
	}
};