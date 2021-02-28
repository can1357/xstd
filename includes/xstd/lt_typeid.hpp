#pragma once
#include <type_traits>
#include <stdint.h>
#include "random.hpp"
#include "intrinsics.hpp"

namespace xstd
{
	namespace impl
	{
		// Weak random generated based on storage traits.
		//
		template<typename T>
		static constexpr size_t get_rng_base( uint64_t key )
		{
			uint64_t base = key - sizeof( T );
			return lce_64_n( base, ( base / alignof( T ) ) & 3 );
		}
	};

	// This class generates a unique 64-bit hash for each type at link time 
	// with no dependency on compiler features such as RTTI abusing relocations.
	//
	template<typename T>
	struct lt_typeid
	{
		using I = std::conditional_t<std::is_void_v<T>, char, std::decay_t<T>>;
		inline static volatile char padding[ 17 + ( impl::get_rng_base<I>( 0x6a477a8b10f59225 ) % 30 ) ];

		// Returns the unique relocation based identifier.
		//
		__forceinline static intptr_t get_reloc()
		{
			return ( intptr_t ) &padding[ sizeof( padding ) ] - ( intptr_t ) &lt_typeid<void>::padding[ 0 ];
		}

		// Invoked internally to calculate the final hash.
		//
		__forceinline static size_t hash( uint64_t key = 0x47C63F4156E0EA7F )
		{
			if constexpr ( !std::is_same_v<T, void> )
			{
				// Calculate the distance between the reference point of this type
				// and of void and apply an aribtrary hash function over it. This 
				// should match for all identical binaries regardless of relocations.
				//
				intptr_t reloc_delta = get_reloc();
				return ( size_t ) impl::get_rng_base<I>( ~key ) ^ ( ( key ^ reloc_delta ) * ( ( sizeof( T ) + reloc_delta ) | 3 ) );
			}
			return key - 0x9efabe91b381ba30;
		}

		// Weaker "hash" but likely will be 1-2 instructions due to the handled relocations.
		//
		__forceinline static size_t weak()
		{
			if constexpr ( !std::is_same_v<T, void> )
				return get_reloc() >> 5;
			else
				return 0xc304dc3397d80fb0;
		}
	};
};