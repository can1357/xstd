#pragma once
#include <type_traits>
#include <stdint.h>
#include "intrinsics.hpp"

namespace xstd
{
	// This class generates a unique 64-bit hash for each type at link time 
	// with no dependency on compiler features such as RTTI.
	//
	template<typename T>
	struct lt_typeid
	{
		// Invoked internally to calculate the final hash.
		//
		static size_t calculate()
		{
#ifndef HAS_RTTI
			if constexpr ( !std::is_same_v<T, void> )
			{
				// Calculate the distance between the reference point of this type
				// and of void and apply an aribtrary hash function over it. This 
				// should match for all identical binaries regardless of relocations.
				//
				intptr_t reloc_delta = ( intptr_t ) &value - ( intptr_t ) &lt_typeid<void>::value;
				return ( size_t ) ( ( 0x47C63F4156E0EA7F ^ reloc_delta ) * ( sizeof( T ) + reloc_delta | 3 ) );
			}
			return ( size_t ) -1;
#else
			return typeid( T ).hash_code();
#endif
		}
		// Stores the computed hash at process initialization time.
		//
		inline static const size_t value = calculate();
	};

	template<typename T>
	static const size_t& lt_typeid_v = lt_typeid<std::decay_t<T>>::value;
};