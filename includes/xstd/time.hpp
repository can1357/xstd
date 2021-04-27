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
	#define XSTD_DEFAULT_CLOCK std::chrono::high_resolution_clock
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
		using microseconds = std::chrono::microseconds;
		using nanoseconds =  std::chrono::nanoseconds;
		using basic_units =  std::tuple<nanoseconds, microseconds, milliseconds, seconds, minutes, hours>;

		// Declare prefered clock and units.
		//
		using base_clock =   XSTD_DEFAULT_CLOCK;
		using timestamp =    base_clock::time_point;
		using duration =     base_clock::duration;

		// Wrap around base clock.
		//
		inline static timestamp now() { return base_clock::now(); }

		// Declare conversion to string.
		//
		template<typename Rep, typename Period>
		static std::string to_string( std::chrono::duration<Rep, Period> duration )
		{
			bool sign = false;
			if ( duration < std::chrono::duration < Rep, Period>{ 0 } )
			{
				sign = true;
				duration = -duration;
			}

			using T = std::chrono::duration<double, Period>;
			T fduration = std::chrono::duration_cast<T>( duration );

			static constexpr std::array abbreviations = { "ns", "us", "ms", "sec", "min", "hrs" };
			static constexpr std::array durations = make_constant_series<std::tuple_size_v<basic_units>>( [ ] ( auto x )
			{
				return std::chrono::duration_cast<T>( 
					std::tuple_element_t<decltype( x )::value, basic_units>( 1 )
				);
			} );

			// Iterate duration list in descending order until we find an appropriate duration.
			//
			size_t n = abbreviations.size() - 1;
			while ( n != 0 )
			{
				if ( fduration >= durations[ n ] )
					break;
				--n;
			}

			// Convert float to string.
			//
			char buffer[ 32 ];
			snprintf( buffer, 32, "%s%.2lf%s", sign ? "-" : "", fduration / durations[ n ], abbreviations[ n ] );
			return buffer;
		}

		// Monotic counter returning a unique value every call.
		//
		namespace mimpl { inline std::atomic<uint64_t> tcounter = 0; };
		inline static uint64_t monotonic()
		{
			return ++mimpl::tcounter;
		}
	};
	using timestamp = time::timestamp;
	using duration =  time::duration;

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
			timestamp t0 = time::now();
			f( std::forward<Tx>( args )... );
			timestamp t1 = time::now();
			return t1 - t0;
		}
		else
		{
			timestamp t0 = time::now();
			std::pair<result_t, duration> result = { f( std::forward<Tx>( args )... ), duration{} };
			timestamp t1 = time::now();
			result.second = t1 - t0;
			return result;
		}
	}

	// Same as ::profile but ignores the return value and runs N times.
	//
	template<size_t N, typename T, typename... Tx> requires InvocableWith<T, Tx...>
	static duration profile_n( T&& f, Tx&&... args )
	{
		auto t0 = time::now();
		for ( size_t i = 0; i != N; i++ ) 
			f( args... ); // Not forwarded since we can't move N times.
		auto t1 = time::now();
		return t1 - t0;
	}
};