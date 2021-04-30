#pragma once
#include <iterator>
#include <vector>
#include <atomic>
#include "chore.hpp"
#include "type_helpers.hpp"
#include "intrinsics.hpp"

// [Configuration]
// XSTD_NO_PARALLEL: If set, parallel_transform will redirect into transform.
//
#ifndef XSTD_NO_PARALLEL
	#define XSTD_NO_PARALLEL 0
#endif

namespace xstd
{
	// Generic transformation wrapper.
	//
	template<Iterable C, typename F> requires Invocable<F, void, iterator_reference_type_t<C>>
	static void transform( C&& container, const F& worker )
	{
		auto end = std::end( container );
		for ( auto it = std::begin( container ); it != end; ++it )
			worker( *it );
	}

	// Generic parallel transformation wrapper.
	//
	template<Iterable C, typename F> requires Invocable<F, void, iterator_reference_type_t<C>>
	static void transform_parallel( C&& container, const F& worker )
	{
		size_t container_size = std::size( container );

		// If parallel transformation is disabled or if the container only has one entry, 
		// fallback to serial transformation.
		//
		if ( XSTD_NO_PARALLEL || container_size == 1 )
			return transform( std::forward<C>( container ), worker );
		
		// Otherwise, use xstd::chore for each entry.
		//
		event_base event = {};
		std::atomic<size_t> completion_counter = container_size;
		std::atomic<size_t> init_counter = 0;

		for ( size_t n = 0; n != container_size; n++ )
		{
			chore( [ & ]
			{
				worker( *std::next( std::begin( container ), init_counter++ ) );
				if ( !--completion_counter )
					event.notify();
			} );
		}
		event.wait();
	}
};