#pragma once
#include <frozen/algorithm.h>
#include <frozen/set.h>
#include <frozen/unordered_set.h>
#include <frozen/map.h>
#include <frozen/unordered_map.h>
#include "hashable.hpp"

namespace xstd {
	namespace impl {
		struct std_hasher {
			template<typename T>
			FORCE_INLINE inline constexpr size_t operator()( const T& obj, size_t seed ) const noexcept {
				hash_t h{ seed };
				extend_hash( h, obj );
				return h.digest();
			}
		};
	};

	template <typename T, typename U, size_t N, typename Equal = std::equal_to<T>>
	inline constexpr auto freeze_umap( std::pair<T, U> const ( &items )[ N ], const Equal& equal = {} )
#ifndef __INTELLISENSE__
	-> frozen::unordered_map<T, U, N, impl::std_hasher, Equal>
	{ return {items, impl::std_hasher{}, equal}; }
#else
	-> frozen::unordered_map<T, U, 1, impl::std_hasher, Equal>
	{ return { {items[ 0 ]}, impl::std_hasher{}, equal }; }
#endif

	template <typename T, typename U, size_t N, typename Equal = std::equal_to<T>>
	inline constexpr auto freeze_umap( std::array<std::pair<T, U>, N> const& items, const Equal& equal = {} ) 
#ifndef __INTELLISENSE__
	-> frozen::unordered_map<T, U, N, impl::std_hasher, Equal>
	{ return {items, impl::std_hasher{}, equal}; }
#else
	-> frozen::unordered_map<T, U, 1, impl::std_hasher, Equal>
	{ return { {items[ 0 ]}, impl::std_hasher{}, equal }; }
#endif

	template <typename T, size_t N, typename Equal = std::equal_to<T>>
	inline constexpr auto freeze_uset( T const ( &items )[ N ], const Equal& equal = {} )
#ifndef __INTELLISENSE__
	-> frozen::unordered_set<T, N, impl::std_hasher, Equal> 
	{ return {items, impl::std_hasher{}, equal}; }
#else
	-> frozen::unordered_set<T, 1, impl::std_hasher, Equal>
	{ return { {items[ 0 ]}, impl::std_hasher{}, equal }; }
#endif

	template <typename T, size_t N, typename Equal = std::equal_to<T>>
	inline constexpr auto freeze_uset( std::array<T, N> const& items, const Equal& equal = {} )
#ifndef __INTELLISENSE__
	-> frozen::unordered_set<T, N, impl::std_hasher, Equal> 
	{ return {items, impl::std_hasher{}, equal}; }
#else
	-> frozen::unordered_set<T, 1, impl::std_hasher, Equal>
	{ return { {items[ 0 ]}, impl::std_hasher{}, equal }; }
#endif

	template <typename T, typename U, size_t N, typename Equal = std::equal_to<T>>
	inline constexpr auto freeze_map( std::pair<T, U> const ( &items )[ N ], const Equal& equal = {} ) -> frozen::map<T, U, N, Equal> { return {items, equal}; }
	template <typename T, typename U, size_t N, typename Equal = std::equal_to<T>>
	inline constexpr auto freeze_map( std::array<std::pair<T, U>, N> const& items, const Equal& equal = {} )  -> frozen::map<T, U, N, Equal> { return {items, equal}; }
	template <typename T, size_t N, typename Equal = std::equal_to<T>>
	inline constexpr auto freeze_set( T const ( &items )[ N ], const Equal& equal = {} ) -> frozen::set<T, N, Equal> { return {items, equal}; }
	template <typename T, size_t N, typename Equal = std::equal_to<T>>
	inline constexpr auto freeze_set( std::array<T, N> const& items, const Equal& equal = {} ) -> frozen::set<T, N, Equal> { return {items, equal}; }
};