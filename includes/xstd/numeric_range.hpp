#pragma once
#include <iterator>
#include <limits>
#include <string>
#include <optional>
#include "formatting.hpp"
#include "type_helpers.hpp"

namespace xstd
{
	template<Integral T = size_t>
	struct numeric_range
	{
		// Declare the iterator type.
		//
		struct iterator
		{
			// Generic iterator typedefs.
			//
			using iterator_category = std::random_access_iterator_tag;
			using difference_type =   std::make_signed_t<T>;
			using value_type =        T;
			using reference =         T&;
			using pointer =           void*;

			value_type at;

			// Support random iteration.
			//
			constexpr iterator& operator++() { at++; return *this; }
			constexpr iterator& operator--() { at--; return *this; }
			constexpr iterator operator++( int ) { auto s = *this; operator++(); return s; }
			constexpr iterator operator--( int ) { auto s = *this; operator--(); return s; }
			constexpr iterator& operator+=( difference_type d ) { at += d; return *this; }
			constexpr iterator& operator-=( difference_type d ) { at -= d; return *this; }
			constexpr iterator operator+( difference_type d ) const { auto s = *this; operator+=( d ); return s; }
			constexpr iterator operator-( difference_type d ) const { auto s = *this; operator-=( d ); return s; }

			// Comparison and difference against another iterator.
			//
			constexpr difference_type operator-( const iterator& other ) const { return at - other.at; }
			constexpr bool operator<( const iterator& other ) const { return at < other.at; }
			constexpr bool operator==( const iterator& other ) const { return at == other.at; }
			constexpr bool operator!=( const iterator& other ) const { return at != other.at; }
			
			// Redirect dereferencing to container.
			//
			constexpr value_type operator*() const { return at; }

			// String conversion.
			//
			std::string to_string() const { return fmt::as_string( at ); }
		};
		using const_iterator = iterator;

		// Beginning and the end of the range.
		//
		T first;
		T limit;
		constexpr numeric_range() : first( 0 ), limit( 0 ) {}
		constexpr numeric_range( T first, T limit ) 
			: first( first ), limit( limit ) {}

		// Default copy/move.
		//
		constexpr numeric_range( numeric_range&& ) noexcept = default;
		constexpr numeric_range( const numeric_range& ) = default;
		constexpr numeric_range& operator=( numeric_range&& ) noexcept = default;
		constexpr numeric_range& operator=( const numeric_range& ) = default;

		// Generic container helpers.
		//
		constexpr bool empty() const { return limit == first; }
		constexpr size_t size() const { return limit - first; }
		constexpr iterator begin() const { return { first }; }
		constexpr iterator end() const   { return { limit }; }
		constexpr T operator[]( size_t n ) const { return first + n; }

		// Finds the overlapping region if relevant.
		//
		constexpr numeric_range overlap( const numeric_range& other ) const
		{
			// Completely seperate ranges:
			//
			if ( other.first >= limit )
				return {};
			if ( other.limit <= first  )
				return {};

			// Return the overlap:
			//
			T nfirst = std::max( first, other.first );
			T nlimit = std::min( limit, other.limit );
			return { nfirst, nlimit };
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

		// String conversion.
		//
		std::string to_string() const
		{
			return XSTD_CSTR( "[" ) + fmt::as_string( first ) + XSTD_CSTR( ", " ) + fmt::as_string( limit ) + XSTD_CSTR( ")" );
		}
	};
	template<typename T>               numeric_range( T )      -> numeric_range<integral_max_t<T, T>>;  // Max'd to enforce the concept, intellisense does not like concepts here.
	template<typename T1, typename T2> numeric_range( T1, T2 ) -> numeric_range<integral_max_t<T1, T2>>;

	// Simple range creation wrapper.
	//
	static constexpr numeric_range<> iindices = { 0ull, SIZE_MAX };

	template<typename T>
	static numeric_range<T> iiota( T x ) { return { x, SIZE_MAX }; }
};