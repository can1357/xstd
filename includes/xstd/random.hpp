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
	#define _XSTD_RANDOM_RNG_QUALIFIERS static thread_local
#else
	#define _XSTD_RANDOM_RNG_QUALIFIERS inline
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
		_XSTD_RANDOM_RNG_QUALIFIERS std::default_random_engine local_rng( std::random_device{}( ) );
#else
		// Declare both random generators with a fixed seed.
		//
		static constexpr uint64_t crandom_default_seed = XSTD_RANDOM_FIXED_SEED;
		_XSTD_RANDOM_RNG_QUALIFIERS std::default_random_engine local_rng( XSTD_RANDOM_FIXED_SEED );
#endif

		// Linear congruential generator using the constants from Numerical Recipes.
		//
		static constexpr uint64_t lce_64( uint64_t& value )
		{
			return ( value = 1664525 * value + 1013904223 );
		}

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

	// Generates a single random number.
	//
	template<Integral T>
	static T make_random( T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		using V = std::conditional_t<sizeof( T ) < 4, int32_t, T>;
		return ( T ) std::uniform_int_distribution<V>{ ( V ) min, ( V ) max }( impl::local_rng );
	}
	template<FloatingPoint T>
	static T make_random( T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		return std::uniform_real_distribution<T>{ min, max }( impl::local_rng );
	}
	template<Integral T>
	static constexpr T make_crandom( size_t offset, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		uint64_t value = impl::crandom_default_seed;
		while ( offset-- != 0 ) impl::lce_64( value );
		return impl::uniform_eval( impl::lce_64( value ), min, max );
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
		while ( offset-- != 0 ) impl::lce_64( value );
		return { impl::uniform_eval( impl::lce_64( ( I, value ) ), min, max )... };
	}
	template<typename T, size_t N>
	static std::array<T, N> make_random_n( T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		return make_random_n<T>( min, max, std::make_index_sequence<N>{} );
	}
	template<typename T, size_t N>
	static constexpr std::array<T, N> make_crandom_n( size_t offset, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
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
};