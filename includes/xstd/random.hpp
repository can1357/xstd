#pragma once
#include <random>
#include <stdint.h>
#include <type_traits>
#include <array>
#include <iterator>
#include "bitwise.hpp"

// [[Configuration]]
// XSTD_RANDOM_FIXED_SEED: If set, uses it as a fixed seed for the random number generator
// XSTD_RANDOM_THREAD_LOCAL: If set, will use the thread local qualifier for the random number generator.
//
#ifndef XSTD_RANDOM_THREAD_LOCAL
#define XSTD_RANDOM_THREAD_LOCAL 0
#endif

#if GNU_COMPILER
#pragma GCC diagnostic ignored "-Wunused-value"
#endif
namespace xstd {
	struct pcg_u128 {
		uint64_t lo;
		uint64_t hi;
		constexpr auto operator<=>( const pcg_u128& ) const noexcept = default;
	};

	// All primitives.
	//
	FORCE_INLINE inline constexpr uint64_t lce_64( uint64_t& value ) {
		return ( value = 6364136223846793005 * value + 1442695040888963407 );
	}
	FORCE_INLINE inline constexpr uint32_t pce_32( uint64_t& value ) {
		// oneseq_xsh_rr_64_32
		uint64_t x = std::exchange( value, 6364136223846793005 * value + 1442695040888963407 );
		uint8_t shift = uint8_t( x >> 59 );
		x ^= x >> 18;
		return rotl( uint32_t( x >> 27 ), shift );
	}
	FORCE_INLINE inline constexpr uint64_t pce_64( pcg_u128& value ) {
		// oneseq_xsl_rr_128_64
		constexpr uint64_t mul_lo = 4865540595714422341ull;
		constexpr uint64_t mul_hi = 2549297995355413924ull;
		constexpr uint64_t inc_lo = 1442695040888963407ull;
		constexpr uint64_t inc_hi = 6364136223846793005ull;

		uint64_t hi;
		uint64_t lo0 = umul128( mul_lo, value.lo, &hi );
		hi += ( mul_lo * value.hi ) + ( mul_hi * value.lo );

		auto [lo, of] = add_checked<uint64_t>( inc_lo, lo0 );
		hi = inc_hi + hi + of;
		value = { lo, hi };

		bitcnt_t n = ( hi >> 58 ) & 63;
		return rotrq( hi ^ lo, n );
	}
	FORCE_INLINE inline constexpr uint64_t pce_64( uint64_t& value ) {
		// 2x oneseq_xsh_rr_64_32
		return uint64_t( pce_32( value ) ) | ( uint64_t( pce_32( value ) ) << 32 );
	}

	// Step count versions.
	//
	FORCE_INLINE CONST_FN inline constexpr uint64_t lce_64_n( uint64_t value, size_t offset = 0 ) {
		uint64_t result;
		for ( size_t i = 0; i <= offset; i++ ) result = lce_64( value );
		return result;
	}
	FORCE_INLINE CONST_FN inline constexpr uint32_t pce_32_n( uint64_t value, size_t offset = 0 ) {
		uint32_t result;
		for ( size_t i = 0; i <= offset; i++ ) result = pce_32( value );
		return result;
	}
	FORCE_INLINE CONST_FN inline constexpr uint64_t pce_64_n( pcg_u128 value, size_t offset = 0 ) {
		uint64_t result;
		for ( size_t i = 0; i <= offset; i++ ) result = pce_64( value );
		return result;
	}
	FORCE_INLINE CONST_FN inline constexpr uint64_t pce_64_n( uint64_t value, size_t offset = 0 ) {
		uint64_t result;
		for ( size_t i = 0; i <= offset; i++ ) result = pce_64( value );
		return result;
	}

	// Fast uniform real distribution.
	//
	template<FloatingPoint F, Unsigned S> requires ( sizeof( S ) >= sizeof( F ) )
	FORCE_INLINE CONST_FN inline constexpr F uniform_real( S v )
	{
		using U = std::conditional_t<Same<F, float>, uint32_t, uint64_t>;
		constexpr bitcnt_t mantissa_bits = Same<F, float> ? 23 : 52;
		constexpr bitcnt_t exponent_bits = Same<F, float> ? 8 : 11;
		constexpr uint32_t exponent_0 = uint32_t( fill_bits( exponent_bits - 1 ) - 2 ); // 2^-2, max at 0.499999.

		// Find the lowest bit set, 0 requires 1 bit to be set to '0', 1 requires 
		// 2 bits to be set to '0', each increment has half the chance of being 
		// returned compared to the previous one which leads to equal distribution 
		// between exponential levels.
		//
		constexpr bitcnt_t exponent_seed_bits = std::min<size_t>( 31, sizeof( S ) * 8 - ( mantissa_bits + 1 ) );
		uint32_t exponent = exponent_0 - lsb( uint32_t( v ) | ( 1u << exponent_seed_bits ) );

		// Clear and replace the exponent bits.
		//
		v &= ~fill_bits( exponent_bits );
		v |= exponent;

		// Rotate until mantissa is at the bottom, add 0.5 to shift from [-0.5, +0.5] to [0.0, 1.0].
		//
		v = rotl( v, mantissa_bits );
		return bit_cast< F >( U( v ) ) + 0.5;
	}
	template<Unsigned S> requires ( sizeof( S ) >= sizeof( double ) )
	FORCE_INLINE CONST_FN inline constexpr double uniform_real( S v, double min, double max )
	{
		return min + uniform_real<double, S>( v ) * ( max - min );
	}
	template<Unsigned S> requires ( sizeof( S ) >= sizeof( float ) )
	FORCE_INLINE CONST_FN inline constexpr float uniform_real( S v, float min, float max )
	{
		return min + uniform_real<float, S>( v ) * ( max - min );
	}

	// Uniform integer distribution.
	//
	template<Unsigned U>
	FORCE_INLINE CONST_FN inline constexpr U uniform_integer( U seed, U max ) {
		// Fixes both the issue with overflow/zero and speeds up handling of pow2-1.
		//
		if ( !( max & ( max + 1 ) ) )
			return seed & max;
		else
			return seed % ( max + 1 );
	}
	template<Integral I, Unsigned S>
	FORCE_INLINE CONST_FN inline constexpr I uniform_integer( S seed, I min, I max )
	{
		if constexpr ( Same<I, bool> ) {
			return ( seed & 1 ) ? min : max;
		} else {
			return min + uniform_integer( seed, S( max - min ) );
		}
	}

	// Implement the engine-like wrapper.
	//
	struct pcg {
		using result_type = uint32_t;

		uint64_t state = { 0 };
		inline constexpr pcg( uint64_t s = 0xcafef00dd15ea5e5 ) { seed( s ); }
		inline constexpr void seed( uint64_t s ) {
			s += 1442695040888963407;
			pce_32( s );
			state = s;
		}

		constexpr pcg( pcg&& o ) noexcept = default;
		constexpr pcg( const pcg& o ) = default;
		constexpr pcg& operator=( pcg&& o ) noexcept = default;
		constexpr pcg& operator=( const pcg& o ) = default;

		static inline constexpr result_type min() { return ( std::numeric_limits<result_type>::min )( ); }
		static inline constexpr result_type max() { return ( std::numeric_limits<result_type>::max )( ); }
		inline constexpr double entropy() const noexcept { return sizeof( result_type ) * 8; }
		inline constexpr result_type operator()() { return ( result_type ) pce_32( state ); }
	};
	struct pcg64 {
		using result_type = uint64_t;

		pcg_u128 state = { 0, 0 };
		inline constexpr pcg64( uint64_t s = 0xcafef00dd15ea5e5 ) { seed( s ); }
		inline constexpr void seed( uint64_t s ) {
			pcg_u128 tmp = { s, 6364136223846793005 };
			pce_64( tmp );
			state = tmp;
		}

		constexpr pcg64( pcg64&& o ) noexcept = default;
		constexpr pcg64( const pcg64& o ) = default;
		constexpr pcg64& operator=( pcg64&& o ) noexcept = default;
		constexpr pcg64& operator=( const pcg64& o ) = default;

		static inline constexpr result_type min() { return ( std::numeric_limits<result_type>::min )( ); }
		static inline constexpr result_type max() { return ( std::numeric_limits<result_type>::max )( ); }
		inline constexpr double entropy() const noexcept { return sizeof( result_type ) * 8; }
		inline constexpr result_type operator()() { return ( result_type ) pce_64( state ); }
	};
	struct atomic_pcg {
		using result_type = uint32_t;

		std::atomic<uint64_t> state = { 0 };
		inline atomic_pcg( uint64_t s = 0xcafef00dd15ea5e5 ) { seed( s ); }
		inline void seed( uint64_t s ) {
			s += 1442695040888963407;
			pce_32( s );
			state.store( s, std::memory_order::release );
		}

		static inline constexpr result_type min() { return ( std::numeric_limits<result_type>::min )( ); }
		static inline constexpr result_type max() { return ( std::numeric_limits<result_type>::max )( ); }
		inline constexpr double entropy() const noexcept { return sizeof( result_type ) * 8; }

		inline result_type operator()() {
			uint64_t s0 = state.load( std::memory_order::relaxed );
			while ( true ) {
				uint64_t s1 = s0;
				result_type r = pce_32( s1 );
				if ( state.compare_exchange_strong( s0, s1, std::memory_order::acquire ) ) {
					return r;
				}
			}
		}
	};
	struct atomic_pcg64 {
		using result_type = uint64_t;

		volatile pcg_u128 state = pcg_u128{ 0, 0 };
		inline atomic_pcg64( uint64_t s = 0xcafef00dd15ea5e5 ) { seed( s ); }
		inline void seed( uint64_t s ) { 
			pcg_u128 tmp = { s, 6364136223846793005 };
			pce_64( tmp );
			( pcg_u128& ) state = tmp;
		}

		static inline constexpr result_type min() { return ( std::numeric_limits<result_type>::min )( ); }
		static inline constexpr result_type max() { return ( std::numeric_limits<result_type>::max )( ); }
		inline constexpr double entropy() const noexcept { return sizeof( result_type ) * 8; }

		inline result_type operator()() {
			pcg_u128 s0 = ( pcg_u128& ) state;
			std::atomic_thread_fence( std::memory_order::release );
			while ( true ) {
				pcg_u128 s1 = s0;
				result_type r = pce_64( s1 );
				if ( cmpxchg( state, s0, s1 ) ) {
					return r;
				}
			}
		}
	};

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
		static constexpr uint64_t crandom_default_seed = [ ](){
			uint64_t value = 0xa0d82d3adc00b109;
			for ( char c : __DATE__ )
				value = ( value ^ c ) * 0x100000001B3;
			return value;
		}();
		inline __xstd_rng global_rng{ uint64_t( std::random_device{}( ) ) | ( uint64_t( std::random_device{}( ) ) << 32 ) };
#else
		static constexpr uint64_t crandom_default_seed = XSTD_RANDOM_FIXED_SEED ^ 0xC0EC0E00;
		inline __xstd_rng global_rng{ XSTD_RANDOM_FIXED_SEED };
#endif
#undef __xstd_rng
	};

	// Changes the random seed.
	// - If XSTD_RANDOM_THREAD_LOCAL is set, will be a thread-local change, else global.
	//
	FORCE_INLINE static void seed_rng( uint64_t n )
	{
		impl::global_rng.seed( n );
	}

	// Generates a secure random number.
	//
	template<Integral T = uint64_t>
	FORCE_INLINE static T make_srandom( T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		using U = std::random_device::result_type;
		U seed[ ( sizeof( T ) + sizeof( U ) - 1 ) / sizeof( U ) ];
		for ( auto& v : seed )
			v = std::random_device{}( );
		return uniform_integer( *( convert_uint_t<T>* ) &seed[ 0 ], min, max );
	}
	template<FloatingPoint T>
	FORCE_INLINE static T make_srandom( T min = 0, T max = 1 )
	{
		return uniform_real( make_srandom<convert_uint_t<T>>(), min, max );
	}

	// Generates a single random number.
	//
	template<Integral T = uint64_t>
	FORCE_INLINE static T make_random( T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		return uniform_integer( impl::global_rng(), min, max );
	}
	template<FloatingPoint T>
	FORCE_INLINE static T make_random( T min = 0, T max = 1 )
	{
		return uniform_real( make_random<convert_uint_t<T>>(), min, max );
	}
	template<Integral T = uint64_t>
	FORCE_INLINE static constexpr T make_crandom( uint64_t key = 0, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max() )
	{
		key = pce_64_n( impl::crandom_default_seed ^ key, 1 + ( key & 3 ) );
		return uniform_integer( key, min, max );
	}
	template<FloatingPoint T>
	FORCE_INLINE static constexpr T make_crandom( uint64_t key = 0, T min = 0, T max = 1 )
	{
		return uniform_real( make_crandom<convert_uint_t<T>>( key ), min, max );
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
			v = uniform_integer( lce_64( key ), min, max );
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
		return { uniform_integer( lce_64( ( I, key ) ), min, max )... };
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