// Copyright (c) 2020, Can Boluk
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
#pragma once
#include <random>
#include <stdint.h>
#include <type_traits>
#include <array>
#include <iterator>
#include "type_helpers.hpp"

// [Configuration]
// XSTD_RANDOM_THREAD_LOCAL: If set, will use the thread local qualifier for the random number generator.
//
#ifndef XSTD_RANDOM_THREAD_LOCAL
	#define XSTD_RANDOM_THREAD_LOCAL 0
#endif

// [Configuration]
// XSTD_RANDOM_FIXED_SEED: If set, uses it as a fixed seed for the random number generator
//
//-- #undef XSTD_RANDOM_FIXED_SEED


#if GNU_COMPILER
    #pragma GCC diagnostic ignored "-Wunused-value"
#endif
namespace xstd
{
#if XSTD_RANDOM_THREAD_LOCAL
	#define _XSTD_RANDOM_RNG_QUALIFIERS thread_local
#else
	#define _XSTD_RANDOM_RNG_QUALIFIERS 
#endif

	namespace impl
	{
#ifndef XSTD_RANDOM_FIXED_SEED
		// Declare the constexpr random seed.
		//
		static constexpr uint64_t crandom_default_seed = ([]()
		{
			uint64_t value = 0xa0d82d3adc00b109;
			for ( char c : __TIME__ )
				value = ( value ^ c ) * 0x100000001B3;
			return value;
		} )();

		// Declare a random engine state per thread.
		//
		inline auto& get_runtime_rng()
		{
			static _XSTD_RANDOM_RNG_QUALIFIERS std::mt19937_64 local_rng( std::random_device{}( ) );
			return local_rng;
		}
#else
		// Declare both random generators with a fixed seed.
		//
		static constexpr uint64_t crandom_default_seed = XSTD_RANDOM_FIXED_SEED ^ 0xC0EC0E00;
		inline auto& get_runtime_rng()
		{
			static _XSTD_RANDOM_RNG_QUALIFIERS std::default_random_engine local_rng( XSTD_RANDOM_FIXED_SEED );
			return local_rng;
		}
#endif
		// Constexpr uniform integer distribution.
		//
		template<Integral T>
		static constexpr T uniform_eval( uint64_t value, T min, T max )
		{
			if ( uint64_t range = ( uint64_t( max - min ) + 1 ) )
				return ( T ) ( convert_uint_t<T> ) ( min + ( value % range ) );
			else
				return ( T ) ( convert_uint_t<T> ) value;
		}
	};

	// Linear congruential generator using the constants from Numerical Recipes.
	//
	static constexpr uint64_t lce_64( uint64_t& value )
	{
		return ( value = 1664525 * value + 1013904223 );
	}
	[[nodiscard]] static constexpr uint64_t lce_64_n( uint64_t value, size_t offset = 1 )
	{
		while ( offset-- )
			lce_64( value );
		return value;
	}

	// Changes the random seed.
	// - If XSTD_RANDOM_THREAD_LOCAL is set, will be a thread-local change, else global.
	//
	static void seed_rng( size_t n ) 
	{ 
		impl::get_runtime_rng().seed( n ); 
	}

	// Generates a single random number.
	//
	template<Integral T = uint64_t>
	static T make_random( T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		using V = std::conditional_t<sizeof( T ) < 4, int32_t, T>;
		return ( T ) std::uniform_int_distribution<V>{ ( V ) min, ( V ) max }( impl::get_runtime_rng() );
	}
	template<FloatingPoint T = float>
	static T make_random( T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		return std::uniform_real_distribution<T>{ min, max }( impl::get_runtime_rng() );
	}
	template<Integral T = uint64_t>
	static constexpr T make_crandom( size_t offset = 1, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		uint64_t value = impl::crandom_default_seed;
		while ( offset-- != 0 ) lce_64( value );
		return impl::uniform_eval( lce_64( value ), min, max );
	}

	// Enum equivalents.
	//
	template<Enum T>
	static T make_random( T min, T max )
	{
		using V = std::underlying_type_t<T>;
		return ( T ) make_random<V>( V( min ), V( max ) );
	}
	template<Enum T>
	static constexpr T make_crandom( size_t offset, T min, T max )
	{
		using V = std::underlying_type_t<T>;
		return ( T ) make_crandom<V>( offset, V( min ), V( max ) );
	}

	// Generates an array of random numbers.
	//
	template<typename T, size_t... I>
	static std::array<T, sizeof...( I )> make_random_n( T min, T max, std::index_sequence<I...> )
	{
		return { ( I, make_random<T>( min ,max ) )... };
	}
	template<typename T, size_t... I>
	static constexpr std::array<T, sizeof...( I )> make_crandom_n( size_t offset, T min, T max, std::index_sequence<I...> )
	{
		uint64_t value = impl::crandom_default_seed;
		while ( offset-- != 0 ) lce_64( value );
		return { impl::uniform_eval( lce_64( ( I, value ) ), min, max )... };
	}
	template<typename T, size_t N>
	static std::array<T, N> make_random_n( T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		return make_random_n<T>( min, max, std::make_index_sequence<N>{} );
	}
	template<typename T, size_t N>
	static constexpr std::array<T, N> make_crandom_n( size_t offset = 1, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		return make_crandom_n<T>( offset, min, max, std::make_index_sequence<N>{} );
	}

	// Picks a random item from the initializer list / argument pack.
	//
	template<typename T>
	static auto pick_random( std::initializer_list<T> list )
	{
		return *( list.begin() + make_random<size_t>( 0, list.size() - 1 ) );
	}
	template<Iterable T>
	static decltype( auto ) pick_randomi( T&& source )
	{
		auto size = std::size( source );
		return *std::next( std::begin( source ), make_random<size_t>( 0, size - 1 ) );
	}

	template<size_t offset, typename... Tx>
	static constexpr decltype( auto ) pick_crandom( Tx&&... args )
	{
		return std::get<make_crandom<size_t>( offset, 0, sizeof...( args ) - 1 )>( std::tuple<Tx&&...>{ std::forward<Tx>( args )... } );
	}
	template<size_t offset, Iterable T>
	static constexpr decltype( auto ) pick_crandomi( T& source )
	{
		auto size = std::size( source );
		return *std::next( std::begin( source ), make_crandom<size_t>( offset, 0, size - 1 ) );
	}

	// Shuffles a container in a random manner.
	//
	template<Iterable T>
	static void shuffle_random( T& source )
	{
		if ( std::size( source ) <= 1 )
			return;

		size_t n = 1;
		auto beg = std::begin( source );
		auto end = std::end( source );
		for ( auto it = std::next( beg ); it != end; ++n, ++it )
		{
			size_t off = make_random<size_t>( 0, n );
			if ( off != n )
				std::iter_swap( it, std::next( beg, off ) );
		}
	}
	template<Iterable T>
	static constexpr void shuffle_crandom( size_t offset, T& source )
	{
		if ( std::size( source ) <= 1 )
			return;

		uint64_t value = impl::crandom_default_seed;
		while ( offset-- != 0 ) lce_64( value );

		size_t n = 1;
		auto beg = std::begin( source );
		auto end = std::end( source );
		for ( auto it = std::next( beg ); it != end; ++n, ++it )
		{
			size_t off = lce_64( value ) % ( n + 1 );
			if ( off != n )
				std::iter_swap( it, std::next( beg, off ) );
		}
	}
};