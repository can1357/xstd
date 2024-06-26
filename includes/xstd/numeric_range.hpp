#pragma once
#include <iterator>
#include <limits>
#include <string>
#include <optional>
#include "formatting.hpp"
#include "type_helpers.hpp"

namespace xstd
{
	// Define a pseudo-iterator type for integers.
	//
	template<typename T = size_t>
	struct numeric_iterator
	{
		// Generic iterator typedefs.
		//
		using iterator_category = std::random_access_iterator_tag;
		using difference_type =   decltype( std::declval<T>() - std::declval<T>() );
		using value_type =        T;
		using reference =         const T&;
		using pointer =           const T*;

		value_type at;

		// Support random iteration.
		//
		constexpr numeric_iterator& operator++() { at++; return *this; }
		constexpr numeric_iterator& operator--() { at--; return *this; }
		constexpr numeric_iterator operator++( int ) { auto s = *this; operator++(); return s; }
		constexpr numeric_iterator operator--( int ) { auto s = *this; operator--(); return s; }
		constexpr numeric_iterator& operator+=( difference_type d ) { at += d; return *this; }
		constexpr numeric_iterator& operator-=( difference_type d ) { at -= d; return *this; }
		constexpr numeric_iterator operator+( difference_type d ) const { auto s = *this; s += d; return s; }
		constexpr numeric_iterator operator-( difference_type d ) const { auto s = *this; s -= d; return s; }

		// Comparison and difference against another iterator.
		//
		constexpr difference_type operator-( const numeric_iterator& other ) const { return at - other.at; }
		constexpr bool operator<( const numeric_iterator& other ) const { return at < other.at; }
		constexpr bool operator<=( const numeric_iterator& other ) const { return at <= other.at; }
		constexpr bool operator>( const numeric_iterator& other ) const { return at > other.at; }
		constexpr bool operator>=( const numeric_iterator& other ) const { return at >= other.at; }
		constexpr bool operator==( const numeric_iterator& other ) const { return at == other.at; }
		constexpr bool operator!=( const numeric_iterator& other ) const { return at != other.at; }
		
		// Redirect dereferencing to the number, decay to it as well.
		//
		constexpr pointer operator->() const { return &at; }
		constexpr reference operator*() const { return at; }
		constexpr operator value_type() const { return at; }

		// String conversion.
		//
		std::string to_string() const { return fmt::as_string( at ); }
	};
	template<typename T> numeric_iterator( T )->numeric_iterator<T>;

	// Define a psueodo-container storing numeric ranges.
	//
	template<typename T = size_t>
	struct numeric_range
	{
		using iterator =        numeric_iterator<T>;
		using const_iterator =  numeric_iterator<T>;
		using value_type =      T;
		using difference_type = decltype( std::declval<T>() - std::declval<T>() );

		// Beginning and the end of the range.
		//
		T first;
		T limit;
		constexpr numeric_range() : first( T{} ), limit( T{} ) {}
		constexpr numeric_range( T first, T limit ) 
			: first( first ), limit( limit ) {}

		// Default copy/move.
		//
		constexpr numeric_range( numeric_range&& ) noexcept = default;
		constexpr numeric_range( const numeric_range& ) = default;
		constexpr numeric_range& operator=( numeric_range&& ) noexcept = default;
		constexpr numeric_range& operator=( const numeric_range& ) = default;

		// Implement comparison.
		//
		constexpr bool operator<( const numeric_range& o ) const { return first < o.first || ( first == o.first && limit < o.limit ); }
		constexpr bool operator==( const numeric_range& o ) const { return first == o.first && limit == o.limit; }
		constexpr bool operator!=( const numeric_range& o ) const { return first != o.first || limit != o.limit; }

		// Generic container helpers.
		//
		constexpr bool empty() const { return limit == first; }
		constexpr size_t size() const { return limit - first; }
		constexpr iterator begin() const { return { first }; }
		constexpr iterator end() const   { return { limit }; }
		template<typename Ty> requires Addable<T, Ty>
		constexpr auto operator[]( Ty n ) const { return first + n; }

		// Finds the overlapping region if relevant.
		//
		constexpr numeric_range overlaps( const numeric_range& other ) const
		{
			// Disjoint cases:
			//
			if ( other.first >= limit )
				return {};
			if ( other.limit <= first  )
				return {};

			// Return the overlap:
			//
			return { std::max( first, other.first ), std::min( limit, other.limit ) };
		}

		// Checks if the range contains the other, if so returns the offset.
		//
		constexpr std::optional<difference_type> contains( T value ) const
		{
			if ( first <= value && value < limit )
				return ( difference_type )( value - first );
			return std::nullopt;
		}
		constexpr std::optional<difference_type> contains( const numeric_range& other ) const
		{
			if ( first <= other.first && other.limit <= limit )
				return ( difference_type )( other.first - first );
			return std::nullopt;
		}

		// Checks if the range contains a certain value.
		//
		constexpr iterator find( T value ) const
		{
			if ( first <= value && value < limit )
				return iterator{ value };
			else
				return iterator{ limit };
		}

		// Slices the region.
		//
		constexpr numeric_range slice( size_t offset = 0, size_t count = std::string::npos ) const
		{
			T nfirst = first + offset;
			if ( nfirst >= limit )
				return {};
			if ( count == std::string::npos )
				count = limit - nfirst;
			return { nfirst, nfirst + count };
		}

		// Substracts/adds the ranges, returns a sorted list of result.
		//
		constexpr std::pair<numeric_range, numeric_range> operator+( const numeric_range& other ) const
		{
			// Disjoint cases:
			//
			if ( other.first >= limit )
				return { *this, other };
			if ( other.limit <= first )
				return { other, *this };

			// Overlapping:
			//
			return { { std::min( first, other.first ), std::max( limit, other.limit ) }, {} };
		}
		constexpr std::pair<numeric_range, numeric_range> operator-( const numeric_range& other ) const
		{
			// Disjoint cases:
			//
			if ( other.first >= limit )
				return { *this, {} };
			if ( other.limit <= first )
				return { *this, {} };

			// Cut the edges.
			//
			numeric_range lo = *this;
			numeric_range hi = *this;
			if ( lo.limit > other.first )
				lo.limit = other.first;
			if ( hi.first < other.limit )
				hi.first = other.limit;

			// Normalize and return.
			//
			if ( lo.limit < lo.first )
				lo = {};
			if ( hi.limit < hi.first )
				hi = {};
			return { lo, hi };
		}

		// String conversion.
		//
		std::string to_string() const
		{
			return "[" + fmt::as_string( first ) + ", " + fmt::as_string( limit ) + ")";
		}

		// Automatic serialization.
		//
		auto tie() { return std::tie( first, limit ); }
	};
	template<typename T>               numeric_range( T )      -> numeric_range<T>;
	template<typename T1, typename T2> numeric_range( T1, T2 ) -> numeric_range<integral_max_t<T1, T2>>;

	// Simple range creation wrapper.
	//
	static constexpr numeric_range<> iindices = { 0ull, SIZE_MAX };
	
	template<typename T> 
	inline constexpr numeric_range<T> iota( T x, T offset = T{} ) { return { offset, offset + x }; }
};