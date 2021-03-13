#pragma once
#include <cmath>
#include <iterator>
#include "formatting.hpp"
#include "type_helpers.hpp"

namespace xstd
{
	// Simple constexpr ceil/float.
	//
	namespace impl
	{
		template<typename T>
		static constexpr float cxpr_ceil( float f )
		{
			if ( T( f ) < f )
				return T( f ) + 1;
			else
				return T( f );
		}
		template<typename T>
		static constexpr float cxpr_floor( float f )
		{
			return T( f );
		}
	}

	// Computes the precision-strenghtened N'th percentile value given a sorted range.
	// - n = [0, 1]
	//
	template<typename T, typename I1, typename I2>
	static constexpr T percentile( const I1& begin, const I2& end, double n )
	{
		// Determine the length of the range, if empty return default, if single element return as is.
		//
		size_t size = std::distance( begin, end );
		if ( !size ) return T{};
		if ( size == 1 ) return *begin;

		// Handle min/max queries.
		//
		if ( n >= 1 ) return *std::prev( end );
		if ( n <= 0 ) return *begin;

		// Determine the stop position.
		//
		double stop_p = ( size - 1 ) * n;
		size_t stop_f = impl::cxpr_floor<size_t>( stop_p );
		size_t stop_c = impl::cxpr_ceil<size_t>( stop_p );

		// If ceil and floor is equal, there is a middle element, return as is.
		//
		auto it_f = std::next( begin, stop_f );
		auto el_f = *it_f;
		if ( stop_f == stop_c ) return el_f;
		auto it_c = std::next( it_f );
		auto el_c = *it_c;

		// Interpolate the middle value based on the slope.
		//
		double x = el_c - el_f;
		double y = stop_p - stop_f;
		return T( el_f + x * y );
	}
	template<typename I1, typename I2>
	static constexpr auto percentile( const I1& begin, const I2& end, double n ) { return percentile<double, I1, I2>( begin, end, n ); }

	// computes the percentile of a value given a sorted range.
	// - r = [0, 1]
	//
	template<typename It1, typename It2, typename T>
	static constexpr double percentile_of( const It1& begin, const It2& end, T&& element )
	{
		auto [lit, git] = std::equal_range( begin, end, element );
		size_t gn = std::distance( git, end );
		size_t ln = std::distance( lit, end );
		size_t en = std::distance( lit, git );
		if ( gn == ln ) return 0.5;
		if ( !gn )      return 0.0;
		if ( !ln )      return 1.0;
		return ( ln + double( en / 2 ) ) / double( ln + gn + en );
	}

	// Computes the precision-strenghtened mean value given a sorted range.
	//
	template<typename T, typename I1, typename I2>
	static constexpr T mean( const I1& begin, const I2& end )
	{
		// If empty, return default, if single element return as is.
		//
		if ( begin == end ) return T{};
		if ( ( begin + 1 ) == end ) return *begin;
		
		// Accumulate the delta from minimum value and the count.
		//
		size_t count = 0;
		double accumulator = 0;
		auto min_val = *begin;
		for ( auto it = begin; it != end; ++it )
			accumulator += ( *it - min_val ), ++count;
		return T( min_val + accumulator / count );
	}
	template<typename I1, typename I2>
	static constexpr auto mean( const I1& begin, const I2& end ) { return mean<double, I1, I2>( begin, end ); }

	// Computes the mode value given the sorted range.
	//
	template<typename I1, typename I2>
	static constexpr auto mode( const I1& begin, const I2& end ) -> std::decay_t<decltype( *std::declval<I1>() )>
	{
		// If empty, return default, if single element return as is.
		//
		if ( begin == end ) return {};
		if ( ( begin + 1 ) == end ) return *begin;

		// Loop through the items:
		//
		auto prv_beg = begin;
		size_t prev_dup = 1;
		for ( auto it = std::next( begin ); it != end; )
		{
			// Calculate the number of duplicates.
			//
			auto beg = it;
			size_t dup = 1;
			while ( ++it != end && *it == *beg )
				++dup;

			// Replace the previous range if larger.
			//
			if ( prev_dup < dup )
			{
				prev_dup = dup;
				prv_beg = std::move( beg );
			}
		}
		return *prv_beg;
	}

	// Calculates the precision-strenghtened variance of a sorted range.
	//
	template<typename I1, typename I2>
	static constexpr double variance( const I1& begin, const I2& end )
	{
		// If empty/single element, return 0.
		//
		if ( begin == end || ( begin + 1 ) == end ) 
			return 0.0f;
		
		// Accumulate the delta from minimum value and the count.
		//
		size_t count = 0;
		double delta_acc = 0;
		auto min_val = *begin;
		for ( auto it = begin; it != end; ++it )
			delta_acc += ( *it - min_val ), ++count;
		delta_acc /= count;

		// Iterate once again to calculate the variance.
		//
		double sq_delta_acc = 0;
		for ( auto it = begin; it != end; ++it )
		{
			double dt = ( *it - min_val ) - delta_acc;
			sq_delta_acc += dt * dt;
		}
		return sq_delta_acc / ( count - 1 );
	}

	// Calculates the standard variation using the variance algorithm above.
	//
	template<typename I1, typename I2>
	static double stdev( const I1& begin, const I2& end ) { return sqrt( variance<I1, I2>( begin, end ) ); }

	// Creates a sorted clone of the range in the form of a vector.
	//
	template<typename I1, typename I2, typename... Tx>
	static auto sorted_clone( const I1& begin, const I2& end, Tx&&... srt )
	{
		std::vector vec( begin, end );
		std::sort( vec.begin(), vec.end(), std::forward<Tx>( srt )... );
		return vec;
	}

	// Write container wrappers for all functions above.
	//
	template<Iterable C> static constexpr auto percentile( C&& c, double n ) { return percentile<double, iterator_type_t<C>, iterator_type_t<C>>( std::begin( c ), std::end( c ), n ); }
	template<typename T, Iterable C> static constexpr auto percentile( C&& c, double n ) { return percentile<T, iterator_type_t<C>, iterator_type_t<C>>( std::begin( c ), std::end( c ), n ); }
	template<Iterable C> static constexpr auto mean( C&& c ) { return mean<double, iterator_type_t<C>, iterator_type_t<C>>( std::begin( c ), std::end( c ) ); }
	template<typename T, Iterable C> static constexpr auto mean( C&& c ) { return mean<T, iterator_type_t<C>, iterator_type_t<C>>( std::begin( c ), std::end( c ) ); }
	template<Iterable C> static constexpr auto mode( C&& c ) { return mode( std::begin( c ), std::end( c ) ); }
	template<Iterable C> static constexpr auto variance( C&& c ) { return variance( std::begin( c ), std::end( c ) ); }
	template<Iterable C> static auto stdev( C&& c ) { return stdev( std::begin( c ), std::end( c ) ); }
	template<Iterable C, typename T> static constexpr auto percentile_of( C&& c, T&& value ) { return percentile_of<iterator_type_t<C>, iterator_type_t<C>>( std::begin( c ), std::end( c ), std::forward<T>( value ) ); }

	template<Iterable C> requires ( !std::is_array_v<std::remove_cvref_t<C>> && !is_std_array_v<std::remove_cvref_t<C>> )
	static auto sorted_clone( C&& c ) 
	{ 
		return sorted_clone( std::begin( c ), std::end( c ) ); 
	}
	template<typename T, size_t N>
	static auto sorted_clone( T( &arr )[ N ] )
	{
		std::array<T, N> clone;
		std::copy( arr, arr + N, clone.begin() );
		std::sort( clone.begin(), clone.end() );
		return clone;
	}
	template<typename T, size_t N>
	static auto sorted_clone( const std::array<T, N>& arr )
	{
		std::array<T, N> clone = arr;
		std::sort( clone.begin(), clone.end() );
		return clone;
	}

	// Formats statistics about the given container:
	//
	namespace fmt
	{
		template<Iterable C>
		inline std::string stats_sorted( C&& c )
		{
			return str(
				"{'%.2f, [%.2f], %.2f, [%.2f], %.2f' | E(x)=%.2f | var(x)=%.2f | mode(x)=%.2f}",
				percentile<double>( c, 0 ),
				percentile<double>( c, 0.25 ),
				percentile<double>( c, 0.5 ),
				percentile<double>( c, 0.75 ),
				percentile<double>( c, 1.0 ),
				mean<double>( c ),
				variance( c ),
				( double ) mode( c )
			);
		}
		template<Iterable C> inline std::string stats( C&& c ) { return stats_sorted( sorted_clone( std::forward<C>( c ) ) ); }
	};
};