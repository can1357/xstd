#pragma once
#include <chrono>
#include <type_traits>
#include <array>
#include <string>
#include <thread>
#include <atomic>
#include "type_helpers.hpp"
#include "zip.hpp"
#include "algorithm.hpp"

// [Configuration]
// XSTD_DEFAULT_CLOCK: Changes the default high-accuracy clock.
#ifndef XSTD_DEFAULT_CLOCK
	#define XSTD_DEFAULT_CLOCK std::chrono::steady_clock
#endif

// No-bloat chrono interface with some helpers and a profiler.
//
namespace xstd
{
	using namespace std::literals::chrono_literals;

	namespace time
	{
		// Declare basic units.
		//
		using hours =        std::chrono::hours;
		using minutes =      std::chrono::minutes;
		using seconds =      std::chrono::seconds;
		using milliseconds = std::chrono::milliseconds;
		using nanoseconds =  std::chrono::nanoseconds;
		using unit_t =       nanoseconds;
		using basic_units =  std::tuple<nanoseconds, milliseconds, seconds, minutes, hours>;

		// Declare prefered clock and units.
		//
		using base_clock = XSTD_DEFAULT_CLOCK;
		using stamp_t =    base_clock::time_point;

		// Wrap around base clock.
		//
		inline static stamp_t now() { return base_clock::now(); }

		// Declare conversion to string.
		//
		template<Duration T>
		static std::string to_string( T duration )
		{
			static const std::array basic_unit_abbreviations = { XSTD_STR( "ns" ),          XSTD_STR( "ms" ),           XSTD_STR( "sec" ),     XSTD_STR( "min" ),     XSTD_STR( "hrs" ) };
			static const std::array basic_unit_names =         { XSTD_STR( "nanoseconds" ), XSTD_STR( "milliseconds" ), XSTD_STR( "seconds" ), XSTD_STR( "minutes" ), XSTD_STR( "hours" ) };
			static const std::array basic_unit_durations = make_constant_series<std::tuple_size_v<basic_units>>( [ ] ( auto x )
			{
				return std::chrono::duration_cast< unit_t >( std::tuple_element_t<decltype( x )::value, basic_units>( 1 ) );
			} );

			// Convert to unit time.
			//
			unit_t t = std::chrono::duration_cast<unit_t>( duration );
			
			// Iterate duration list in descending order.
			//
			for ( auto [duration, abbrv] : backwards( zip( basic_unit_durations, basic_unit_abbreviations ) ) )
			{
				// If time is larger than the duration given or if we're at the last duration:
				//
				if ( t >= duration || duration == *std::begin( basic_unit_durations ) )
				{
					// Convert float to string.
					//
					char buffer[ 32 ];
					snprintf( buffer, 32, XSTD_CSTR( "%.2lf" ), t.count() / double( duration.count() ) );
					return buffer + abbrv;
				}
			}
			unreachable();
		}

		// Platform specific fast monotic counter.
		//
		namespace mimpl { inline std::atomic<uint64_t> tcounter = 0; };
		inline static uint64_t monotonic()
		{
#if WINDOWS_TARGET
			if constexpr ( is_kernel_mode() )
				return *( volatile uint64_t* ) 0xFFFFF78000000008;
			else
				return *( volatile uint64_t* ) 0x7FFE0008;
#else
			return ++mimpl::tcounter;
#endif
		}
	};
	using timestamp_t = time::stamp_t;
	using timeunit_t =  time::unit_t;

	// Wrappers around std::this_thread::sleep_*.
	//
	template<Duration T>
	static void sleep_for( T&& d ) { std::this_thread::sleep_for( std::forward<T>( d ) ); }
	template<Timestamp T>
	static void sleep_until( T&& d ) { std::this_thread::sleep_until( std::forward<T>( d ) ); }

	// Times the callable given and returns pair [result, duration] if it has 
	// a return value or just [duration].
	//
	template<typename T, typename... Tx> requires InvocableWith<T, Tx...>
	static auto profile( T&& f, Tx&&... args )
	{
		using result_t = decltype( std::declval<T>()( std::forward<Tx>( args )... ) );

		if constexpr ( std::is_same_v<result_t, void> )
		{
			timestamp_t t0 = time::now();
			f( std::forward<Tx>( args )... );
			timestamp_t t1 = time::now();
			return t1 - t0;
		}
		else
		{

			timestamp_t t0 = time::now();
			result_t res = f( std::forward<Tx>( args )... );
			timestamp_t t1 = time::now();
			return std::make_pair( res, t1 - t0 );
		}
	}

	// Same as ::profile but ignores the return value and runs N times.
	//
	template<size_t N, typename T, typename... Tx> requires InvocableWith<T, Tx...>
	static timeunit_t profile_n( T&& f, Tx&&... args )
	{
		auto t0 = time::now();
		for ( size_t i = 0; i != N; i++ ) 
			f( args... ); // Not forwarded since we can't move N times.
		auto t1 = time::now();
		return t1 - t0;
	}
};