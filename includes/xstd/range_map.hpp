#pragma once
#include <map>
#include "numeric_range.hpp"
#include "type_helpers.hpp"

namespace xstd
{
	namespace impl
	{
		template<typename T>
		concept RangeViable = LessComparable<T, T> && Subtractable<T, T> && Addable<T, size_t>;

		struct range_key_compare
		{
			template<typename T>
			constexpr bool operator()( const numeric_range<T>& a, const numeric_range<T>& b ) const noexcept { return a.second < b.second || ( a.second == b.second && a.first >= a.first ); }
		};
	};

	// Range maps implement maps with the key value being a range rather than a value.
	// - If the ranges in the keys overlap, the one with the highest limit will always win the search.
	//
	template<typename K, typename V>
	struct range_map : std::map<numeric_range<K>, V, impl::range_key_compare>
	{
		// Declare the range traits.
		//
		using range_type = numeric_range<K>;
		using map_type =   std::map<range_type, V, impl::range_key_compare>;

		// Redirect construction.
		//
		using map_type::map_type;

		// Implement the range search.
		//
		typename map_type::iterator search( const range_type& range, bool overlap = false )
		{
			auto it = map_type::upper_bound( range );
			while ( true )
			{
				if ( it == map_type::begin() ) break;
				--it;
				if ( it->first.limit <= range.first ) break;
				if ( overlap ? !it->first.overlaps( range ).empty() : it->first.contains( range ) )
					return it;
			}
			return map_type::end();
		}
		typename map_type::const_iterator search( const range_type& range, bool overlap = false ) const
		{
			return const_cast< range_map* >( this )->search( range, overlap );
		}
		typename map_type::iterator find( const K& key ) { return search( { key, key + 1ull } ); }
		typename map_type::const_iterator find( const K& key ) const { return search( { key, key + 1ull } ); }
	};
};