#pragma once
#include <random>
#include <stdint.h>
#include <type_traits>
#include <array>
#include <iterator>
#include "bitwise.hpp"

// [Configuration]
// XSTD_RANDOM_FIXED_SEED: If set, uses it as a fixed seed for the random number generator
// XSTD_RANDOM_THREAD_LOCAL: If set, will use the thread local qualifier for the random number generator.
//
#ifndef XSTD_RANDOM_THREAD_LOCAL
	#define XSTD_RANDOM_THREAD_LOCAL 0
#endif

#if GNU_COMPILER
    #pragma GCC diagnostic ignored "-Wunused-value"
#endif
namespace xstd
{
	// Linear congruential generator.
	//
	static constexpr uint64_t lce_64( uint64_t& value )
	{
		return ( value = 6364136223846793005 * value + 1442695040888963407 );
	}
	[[nodiscard]] static constexpr uint64_t lce_64_n( uint64_t value, size_t offset = 0 )
	{
		while ( true )
		{
			uint64_t result = lce_64( value );
			if ( !offset-- )
				return result;
		}
	}

	// Permuted congruential generator.
	//
	static constexpr uint32_t pce_32( uint64_t& value )
	{
		uint64_t x = std::exchange( value, value * 6364136223846793005 + 1 );
		uint8_t shift = uint8_t( x >> 59 );
		x ^= x >> 18;
		return rotl( uint32_t( x >> 27 ), shift );
	}
	[[nodiscard]] static constexpr uint32_t pce_32_n( uint64_t value, size_t offset = 1 )
	{
		while ( true )
		{
			uint32_t result = pce_32( value );
			if ( !offset-- )
				return result;
		}
	}
	static constexpr uint64_t pce_64( uint64_t& value )
	{
		return piecewise( pce_32( value ), pce_32( value ) );
	}
	[[nodiscard]] static constexpr uint64_t pce_64_n( uint64_t value, size_t offset = 1 )
	{
		while ( true )
		{
			uint64_t result = pce_64( value );
			if ( !offset-- )
				return result;
		}
	}

	// Implement an PCG and its atomic variant.
	//
	template<xstd::Unsigned T = uint64_t>
	struct basic_pcg
	{
		using result_type = T;
		static constexpr uint64_t multiplier = 6364136223846793005;
		static constexpr uint64_t increment =  1;

		uint64_t state;
		inline constexpr basic_pcg( uint64_t s = 0 ) : state( ( pce_32( ++s ), s ) ) {}
		inline constexpr void seed( uint64_t s ) { state = basic_pcg<>{ s }.state; }

		constexpr basic_pcg( const basic_pcg& o ) = default;
		constexpr basic_pcg& operator=( const basic_pcg& o ) = default;

		static inline constexpr result_type min() { return std::numeric_limits<result_type>::min(); }
		static inline constexpr result_type max() { return std::numeric_limits<result_type>::max(); }
		inline constexpr double entropy() const noexcept { return sizeof( T ) * 8; }

		[[nodiscard]] inline constexpr result_type operator()()
		{
			std::array<uint32_t, ( sizeof( T ) + 3 ) / 4> results = {};
			for ( auto& result : results )
				result = pce_32( state );
			if constexpr ( sizeof( T ) == 4 ) return results[ 0 ];
			else if constexpr ( sizeof( T ) == 8 ) return bit_cast<T>( results );
			else return *( result_type* ) &results;
		}
	};
	template<xstd::Unsigned T = uint64_t>
	struct basic_atomic_pcg
	{
		using result_type = T;
		static constexpr uint64_t multiplier = 6364136223846793005;
		static constexpr uint64_t increment =  1;

		std::atomic<uint64_t> state;
		inline constexpr basic_atomic_pcg( uint64_t s = 0 ) : state( basic_pcg<>{ s }.state ) {}
		inline void seed( uint64_t s ) { state.store( basic_pcg<>{ s }.state, std::memory_order::relaxed ); }
		
		inline constexpr basic_atomic_pcg( const basic_atomic_pcg& o ) = delete;
		inline constexpr basic_atomic_pcg& operator=( const basic_atomic_pcg& o ) = delete;

		static inline constexpr result_type min() { return std::numeric_limits<result_type>::min(); }
		static inline constexpr result_type max() { return std::numeric_limits<result_type>::max(); }
		inline constexpr double entropy() const noexcept { return sizeof( T ) * 8; }

		[[nodiscard]] inline result_type operator()()
		{
			static constexpr size_t step_count = ( sizeof( T ) + 3 ) / 4;

			// Generate all keys in one step.
			//
			std::array<uint64_t, step_count> keys = {};
			uint64_t expected = state.load( std::memory_order::relaxed );
			while ( true )
			{
				uint64_t desired = expected;
				for ( auto& key : keys )
					key = std::exchange( desired, desired * multiplier + increment );
				if ( state.compare_exchange_strong( expected, desired ) )
					break;
			}

			// Mask and return.
			//
			std::array<uint32_t, step_count> results = {};
			for ( size_t n = 0; n != step_count; n++ )
				results[ n ] = pce_32( keys[ n ] );
			return *( result_type* ) &results;
		}
	};
	using pcg =          basic_pcg<uint32_t>;
	using pcg64 =        basic_pcg<uint64_t>;
	using atomic_pcg =   basic_atomic_pcg<uint32_t>;
	using atomic_pcg64 = basic_atomic_pcg<uint64_t>;

	namespace impl
	{
#if XSTD_RANDOM_THREAD_LOCAL
	#define __xstd_rng thread_local pcg64
#else
	#define __xstd_rng pcg64
#endif

#ifndef XSTD_RANDOM_FIXED_SEED
		// Declare the constexpr random seed.
		//
		static constexpr uint64_t crandom_default_seed = ([]()
		{
			uint64_t value = 0xa0d82d3adc00b109;
			for ( char c : __DATE__ )
				value = ( value ^ c ) * 0x100000001B3;
			return value;
		} )();
		inline __xstd_rng global_rng{ piecewise<uint32_t>( std::random_device{}(), std::random_device{}() ) };
#else
		static constexpr uint64_t crandom_default_seed = XSTD_RANDOM_FIXED_SEED ^ 0xC0EC0E00;
		inline __xstd_rng global_rng{ XSTD_RANDOM_FIXED_SEED };
#endif

		// Constexpr uniform integer distribution.
		//
		template<Integral T>
		static constexpr T uniform_eval( uint64_t value, T min, T max )
		{
			using U = std::make_unsigned_t<T>;
			
			if ( min == std::numeric_limits<T>::min() && max == std::numeric_limits<T>::max() )
			{
				value &= std::numeric_limits<U>::max();

				if constexpr ( Signed<T> )
					return ( T ) ( int64_t( value ) + min );
				else
					return ( T ) value;
			}

			if constexpr ( Signed<T> )
			{
				if ( min < 0 && max < 0 )
					return -( T ) uniform_eval<U>( value, ( U ) -min, ( U ) -max );
				else if ( min < 0 )
					return T( uniform_eval<U>( value, 0, ( U ) max + ( U ) -min ) ) - min;
				else
					return ( T ) uniform_eval<U>( value, ( U ) min, ( U ) max );
			}
			else
			{
				uint64_t range = ( uint64_t( max - min ) + 1 );
				return ( T ) ( convert_uint_t<T> ) ( min + ( value % range ) );
			}
		}
#undef __xstd_rng
	};

	// Changes the random seed.
	// - If XSTD_RANDOM_THREAD_LOCAL is set, will be a thread-local change, else global.
	//
	FORCE_INLINE static void seed_rng( size_t n )
	{
		impl::global_rng.seed( n );
	}

	// Generates a secure random number.
	//
	template<Integral T = uint64_t>
	FORCE_INLINE static T make_srandom()
	{
		using U = std::random_device::result_type;
		U seed[ ( sizeof( T ) + sizeof( U ) - 1 ) / sizeof( U ) ];
		for( auto& v : seed )
			v = std::random_device{}();
		return *( T* ) &seed[ 0 ];
	}
	template<Integral T = uint64_t>
	FORCE_INLINE static T make_srandom( T min, T max = std::numeric_limits<T>::max() )
	{
		if ( const_condition( min == std::numeric_limits<T>::min() && max == std::numeric_limits<T>::max() ) )
			return make_srandom<T>();

		using V = std::conditional_t < sizeof( T ) < 4, int32_t, T > ;
		return ( T ) std::uniform_int_distribution<V>{ ( V ) min, ( V ) max }( std::random_device{} );
	}
	template<FloatingPoint T>
	FORCE_INLINE static T make_srandom( T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		return std::uniform_real_distribution<T>{ min, max }( std::random_device{} );
	}

	// Generates a single random number.
	//
	template<Integral T = uint64_t>
	FORCE_INLINE static T make_random()
	{
		uint64_t result = impl::global_rng();
		return ( T& ) result;
	}
	template<Integral T = uint64_t>
	FORCE_INLINE static T make_random( T min, T max = std::numeric_limits<T>::max() )
	{
		if ( const_condition( min == std::numeric_limits<T>::min() && max == std::numeric_limits<T>::max() ) )
			return make_random<T>();
		using V = std::conditional_t < sizeof( T ) < 4, int32_t, T > ;
		return ( T ) std::uniform_int_distribution<V>{ ( V ) min, ( V ) max }( impl::global_rng );
	}
	template<FloatingPoint T>
	FORCE_INLINE static T make_random( T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		return std::uniform_real_distribution<T>{ min, max }( impl::global_rng );
	}
	template<Integral T = uint64_t>
	FORCE_INLINE static constexpr T make_crandom( uint64_t key = 0, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		key = pce_64_n( impl::crandom_default_seed ^ key, 1 + ( key & 3 ) );
		return impl::uniform_eval( key, min, max );
	}

	// Fills the given range with randoms.
	//
	template<Iterable It, Integral T = iterable_val_t<It>>
	FORCE_INLINE static void fill_random( It&& cnt, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		for ( auto& v : cnt )
			v = make_random<T>( min, max );
	}
	template<Iterable It, Integral T = iterable_val_t<It>>
	FORCE_INLINE static void fill_srandom( It&& cnt, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		for ( auto& v : cnt )
			v = make_srandom<T>( min, max );
	}
	template<Iterable It, Integral T = iterable_val_t<It>>
	FORCE_INLINE static constexpr void fill_crandom( It&& cnt, uint64_t key = 0, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		key = pce_64_n( impl::crandom_default_seed ^ key, 1 + ( key & 3 ) );
		for ( auto& v : cnt )
			v = impl::uniform_eval<T>( lce_64( key ), min, max );
	}

	// Enum equivalents.
	//
	template<Enum T>
	FORCE_INLINE static T make_random( T min, T max )
	{
		using V = std::underlying_type_t<T>;
		return ( T ) make_random<V>( V( min ), V( max ) );
	}
	template<Enum T>
	FORCE_INLINE static T make_srandom( T min, T max )
	{
		using V = std::underlying_type_t<T>;
		return ( T ) make_srandom<V>( V( min ), V( max ) );
	}
	template<Enum T>
	FORCE_INLINE static constexpr T make_crandom( uint64_t key, T min, T max )
	{
		using V = std::underlying_type_t<T>;
		return ( T ) make_crandom<V>( key, V( min ), V( max ) );
	}

	// Generates an array of random numbers.
	//
	template<typename T, size_t... I>
	FORCE_INLINE static std::array<T, sizeof...( I )> make_random_n( T min, T max, std::index_sequence<I...> )
	{
		return { ( I, make_random<T>( min ,max ) )... };
	}
	template<typename T, size_t... I>
	FORCE_INLINE static std::array<T, sizeof...( I )> make_srandom_n( T min, T max, std::index_sequence<I...> )
	{
		return { ( I, make_srandom<T>( min ,max ) )... };
	}
	template<typename T, size_t... I>
	FORCE_INLINE static constexpr std::array<T, sizeof...( I )> make_crandom_n( uint64_t key, T min, T max, std::index_sequence<I...> )
	{
		key = pce_64_n( impl::crandom_default_seed ^ key, 1 + ( key & 3 ) );
		return { impl::uniform_eval( lce_64( ( I, key ) ), min, max )... };
	}
	template<typename T, size_t N>
	FORCE_INLINE static std::array<T, N> make_random_n( T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		return make_random_n<T>( min, max, std::make_index_sequence<N>{} );
	}
	template<typename T, size_t N>
	FORCE_INLINE static std::array<T, N> make_srandom_n( T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		return make_srandom_n<T>( min, max, std::make_index_sequence<N>{} );
	}
	template<typename T, size_t N>
	FORCE_INLINE static constexpr std::array<T, N> make_crandom_n( uint64_t key = 0, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		return make_crandom_n<T>( key, min, max, std::make_index_sequence<N>{} );
	}

	// Picks a random item from the initializer list / argument pack.
	//
	template<typename T>
	FORCE_INLINE static auto pick_random( std::initializer_list<T> list )
	{
		return *( list.begin() + make_random<size_t>( 0, list.size() - 1 ) );
	}
	template<Iterable T>
	FORCE_INLINE static decltype( auto ) pick_randomi( T&& source )
	{
		auto size = std::size( source );
		return *std::next( std::begin( source ), make_random<size_t>( 0, size - 1 ) );
	}
	template<typename T>
	FORCE_INLINE static auto pick_srandom( std::initializer_list<T> list )
	{
		return *( list.begin() + make_srandom<size_t>( 0, list.size() - 1 ) );
	}
	template<Iterable T>
	FORCE_INLINE static decltype( auto ) pick_srandomi( T&& source )
	{
		auto size = std::size( source );
		return *std::next( std::begin( source ), make_srandom<size_t>( 0, size - 1 ) );
	}
	template<uint64_t key, typename... Tx>
	FORCE_INLINE static constexpr decltype( auto ) pick_crandom( Tx&&... args )
	{
		return std::get<make_crandom<size_t>( key, 0, sizeof...( args ) - 1 )>( std::tuple<Tx&&...>{ std::forward<Tx>( args )... } );
	}
	template<uint64_t key, Iterable T>
	FORCE_INLINE static constexpr decltype( auto ) pick_crandomi( T& source )
	{
		auto size = std::size( source );
		return *std::next( std::begin( source ), make_crandom<size_t>( key, 0, size - 1 ) );
	}

	// Shuffles a container in a random manner.
	//
	template<Iterable T>
	FORCE_INLINE static void shuffle_random( T& source )
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
	FORCE_INLINE static void shuffle_srandom( T& source )
	{
		if ( std::size( source ) <= 1 )
			return;

		size_t n = 1;
		auto beg = std::begin( source );
		auto end = std::end( source );
		for ( auto it = std::next( beg ); it != end; ++n, ++it )
		{
			size_t off = make_srandom<size_t>( 0, n );
			if ( off != n )
				std::iter_swap( it, std::next( beg, off ) );
		}
	}
	template<Iterable T>
	FORCE_INLINE static constexpr void shuffle_crandom( uint64_t key, T& source )
	{
		if ( std::size( source ) <= 1 )
			return;
		key = pce_64_n( impl::crandom_default_seed ^ key, key & 3 );

		size_t n = 1;
		auto beg = std::begin( source );
		auto end = std::end( source );
		for ( auto it = std::next( beg ); it != end; ++n, ++it )
		{
			size_t off = lce_64( key ) % ( n + 1 );
			if ( off != n )
				std::iter_swap( it, std::next( beg, off ) );
		}
	}
};