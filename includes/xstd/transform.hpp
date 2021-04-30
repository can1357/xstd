#pragma once
#include <iterator>
#include <vector>
#include "type_helpers.hpp"
#include "intrinsics.hpp"

// [Configuration]
// XSTD_NO_PARALLEL: If set, parallel_transform will redirect into transform.
//
#ifndef XSTD_NO_PARALLEL
	#define XSTD_NO_PARALLEL 0
#endif

#if !XSTD_NO_PARALLEL
	#include <atomic>
	#include "chore.hpp"
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
		{
			auto end = std::end( container );
			for ( auto it = std::begin( container ); it != end; ++it )
				worker( *it );
		}
		// Otherwise, use xstd::chore for each entry.
		//
		else
		{
#if !XSTD_NO_PARALLEL
			event_base event = {};
			std::atomic<size_t> completion_counter = container_size;

			for ( auto it = std::begin( container ); it != std::end( container ); ++it )
			{
				chore( [ &, it ]
				{
					worker( *it );
					if ( !--completion_counter )
						event.notify();
				} );
			}
			event.wait();
#endif
		}
	}
};