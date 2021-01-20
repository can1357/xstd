#pragma once
#include <iterator>
#include <limits>
#include <string>
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
		};
		using const_iterator = iterator;

		// Beginning and the end of the range.
		//
		T min_value;
		T max_value;
		constexpr numeric_range( T min_value = std::numeric_limits<T>::min(),
					             T max_value = std::numeric_limits<T>::max() ) 
			: min_value( min_value ), max_value( max_value ) {}

		// Default copy/move.
		//
		constexpr numeric_range( numeric_range&& ) noexcept = default;
		constexpr numeric_range( const numeric_range& ) = default;
		constexpr numeric_range& operator=( numeric_range&& ) noexcept = default;
		constexpr numeric_range& operator=( const numeric_range& ) = default;

		// Generic container helpers.
		//
		constexpr size_t size() const { return max_value - min_value; }
		constexpr iterator begin() const { return { min_value }; }
		constexpr iterator end() const   { return { max_value }; }
		constexpr T operator[]( size_t n ) const { return min_value + n; }

		// String conversion.
		//
		std::string to_string() const
		{
			return XSTD_CSTR( "[" ) + std::to_string( min_value ) + XSTD_CSTR( ", " ) + std::to_string( max_value ) + XSTD_CSTR( ")" );
		}
	};
	template<typename T>               numeric_range( T )      -> numeric_range<integral_max_t<T, T>>;  // Max'd to enforce the concept, intellisense does not like concepts here.
	template<typename T1, typename T2> numeric_range( T1, T2 ) -> numeric_range<integral_max_t<T1, T2>>;

	// Simple range creation wrapper.
	//
	static constexpr numeric_range<> iindices = {};

	template<typename T>
	static numeric_range<T> iiota( T x ) { return { x }; }
};