#pragma once
#include "type_helpers.hpp"
#include <vector>
#include <string>
#include <memory>

// Transmute implements a STL-specific way to transmute between contigious containers storing trivial types, will fallback to copy/reinterprete 
// when an unrecognized STL is being used.
//
namespace xstd
{
#ifdef _MSVC_STL_VERSION
	#define _XSTD_IS_MS_STL 1
#else
	#define _XSTD_IS_MS_STL 0
#endif

	// Vector to unique_ptr transmute.
	//
	template<Trivial T, Trivial T2, template<typename> typename A> requires ( sizeof( T ) == sizeof( T2 ) && Empty<A<T>> && Empty<A<T2>> )
	__forceinline std::unique_ptr<T[], allocator_delete<A<T>>> transmute_unq( std::vector<T2, A<T2>>&& o )
	{
		T* result;
		if constexpr ( _XSTD_IS_MS_STL && sizeof( std::vector<T2> ) == ( 3 * sizeof( T* ) ) )
		{
			T* result = ( T* ) o.data();
			new ( &o ) std::vector<T2>();
			return std::unique_ptr<T[], allocator_delete<A<T>>>{ result };
		}
		else
		{
			T* result = std::allocator<T>::allocate( o.size() );
			memcpy( result, o.data(), o.size() * sizeof( T ) );
			std::copy_n( ( T* ) o.data(), o.size(), result );
			return std::unique_ptr<T[], allocator_delete<A<T>>>{ result };
		}
	}
	template<Trivial T, template<typename> typename A> requires Empty<A<T>>
	__forceinline std::unique_ptr<T[], allocator_delete<A<T>>> transmute_unq( std::vector<T, A<T>>&& o ) { return transmute_unq<T, T, A>( std::move( o ) ); }

	// Vector to vector transmute.
	//
	template<Trivial T, Trivial T2, template<typename> typename A> requires ( sizeof( T ) == sizeof( T2 ) && Empty<A<T>> && Empty<A<T2>> )
	__forceinline std::vector<T, A<T>> transmute_vec( std::vector<T2, A<T2>>&& o )
	{
		if constexpr ( sizeof( std::vector<T> ) == sizeof( std::vector<T2> ) )
			return std::move( *( std::vector<T, A<T>>* ) &o );
		else
			return std::vector<T, A<T>>{ ( T* ) o.data(), ( T* ) ( o.data() + o.size() ) };
	}

	// String to vector transmute.
	//
	template<Trivial T, Trivial T2, template<typename> typename A, typename Tr> requires ( sizeof( T ) == sizeof( T2 ) && Empty<A<T>> && Empty<A<T2>> )
	__forceinline std::vector<T, A<T>> transmute_vec( std::basic_string<T2, Tr, A<T2>>&& o )
	{
		if constexpr ( _XSTD_IS_MS_STL && sizeof( std::vector<T2, A<T2>> ) == ( 3 * sizeof( T* ) ) )
		{
			if ( o.data() >= ( void* ) ( &o + 1 ) || o.data() < ( void* ) ( &o ) )
			{
				std::vector<T> vec;
				auto& [beg, end, rsv] = *( std::array<T*, 3>* ) &vec;
				beg = ( T* ) o.data();
				end = ( T* ) o.data() + o.size();
				rsv = ( T* ) o.data() + o.capacity();
				new ( &o ) std::basic_string<T2, Tr, A<T2>>();
				return vec;
			}
		}
		return std::vector<T, A<T>>{ ( T* ) o.data(), ( T* ) ( o.data() + o.size() ) };
	}
	
	template<Trivial T, template<typename> typename A, typename Tr> requires Empty<A<T>>
	__forceinline std::vector<T, A<T>> transmute_vec( std::basic_string<T, Tr, A<T>>&& o ) { return transmute_vec<T, T, A, Tr>( std::move( o ) ); }
};