#pragma once
#include <iterator>
#include <type_traits>
#include "type_helpers.hpp"
#include "intrinsics.hpp"

namespace xstd
{
	template<typename It, typename F = void>
	struct range;

	// Declare a proxying range iterator.
	//
	template<typename It, typename F>
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
		constexpr range_iterator& operator=( range_iterator&& ) noexcept = default;
		constexpr range_iterator& operator=( const range_iterator& ) = default;

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
	template<typename It, NonVoid F>
	struct range<It, F>
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

	// Declare a trivial range where a transformation cannot be used.
	//
	template<typename It>
	struct range<It, void>
	{
		using iterator =       It;
		using const_iterator = It;
		using value_type =     std::remove_cvref_t<decltype( *std::declval<It>() )>;

		// Holds the limits.
		//
		It ibegin;
		It iend;

		// Construct by beginning and end.
		//
		constexpr range( It begin, It end )
			: ibegin( std::move( begin ) ), iend( std::move( end ) ) {}

		// Construct by container.
		//
		template<Iterable C>
		constexpr range( C&& container )
			: ibegin( std::begin( container ) ), iend( std::end( container ) ) {}

		// Default copy and move.
		//
		constexpr range( range&& ) noexcept = default;
		constexpr range( const range& ) = default;
		constexpr range& operator=( range&& ) noexcept = default;
		constexpr range& operator=( const range& ) = default;

		// Declare basic container interface.
		//
		constexpr iterator begin() const  { return ibegin; }
		constexpr iterator end() const { return iend; }
		constexpr size_t size() const { return ( size_t ) std::distance( ibegin, iend ); }
		constexpr bool empty() const { return ibegin == iend; }
		constexpr decltype( auto ) operator[]( size_t n ) const { return  *std::next( ibegin, n ); }
	};

	// Declare the deduction guides.
	//
	template<typename C> requires Iterable<C>
	range( C )->range<iterator_type_t<C>, void>;
	template<typename C, typename Fn> requires Iterable<C>
	range( C, Fn )->range<iterator_type_t<C>, Fn>;
	template<typename It1, typename It2> requires ( !Iterable<It1> )
	range( It1, It2 )->range<std::conditional_t<Convertible<It1, It2>, It2, It1>, void>;
	template<typename It1, typename It2, typename Fn>
	range( It1, It2, Fn )->range<std::conditional_t<Convertible<It1, It2>, It2, It1>, Fn>;

	// Old style functors.
	//
	template<typename It1, typename It2, typename Fn>
	static constexpr auto make_range( It1&& begin, It2&& end, Fn&& f )
	{
		using It = std::conditional_t<Convertible<It1, It2>, It2, It1>;

		return range<It, Fn>(
				std::forward<It1>( begin ),
				std::forward<It2>( end ),
				std::forward<Fn>( f )
		);
	}
	template<typename It1, typename It2>
	static constexpr auto make_range( It1&& begin, It2&& end )
	{
		using It = std::conditional_t<Convertible<It1, It2>, It2, It1>;

		return range<It, void>( std::forward<It1>( begin ), std::forward<It2>( end ) );
	}
	template<Iterable C, typename Fn>
	static constexpr auto map( C& container, Fn&& f )
	{
		return range<iterator_type_t<C>, Fn>(
			std::begin( container ),
			std::end( container ),
			std::forward<Fn>( f )
		);
	}
	template<Iterable C, typename B, typename F>
	static constexpr auto map( C& container, member_reference_t<B, F> ref )
	{
		auto fn = [ ref = std::move( ref ) ] <typename T> ( T && v ) -> decltype( auto )
		{
			if constexpr ( std::is_base_of_v<B, T> )
				return v.*ref;
			else
				return std::to_address( v )->*ref;
		};
		return range<iterator_type_t<C>, decltype( fn )>(
			std::begin( container ),
			std::end( container ),
			std::move( fn )
		);
	}

	// Returns a reversed container.
	//
	template<Iterable C>
	static constexpr auto backwards( C&& container )
	{
		return range<std::reverse_iterator<iterator_type_t<C>>, void>{
			std::reverse_iterator( std::end( container ) ),
			std::reverse_iterator( std::begin( container ) )
		};
	}
	template<typename It1, typename It2>
	static constexpr auto backwards( It1&& first, It2&& last )
	{
		using It = std::reverse_iterator<std::conditional_t<Convertible<It1, It2>, It2, It1>>;
		return range<It, void>( It( std::forward<It2>( last ) ), It( std::forward<It1>( first ) ) );
	}
};