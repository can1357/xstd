#pragma once
#include "intrinsics.hpp"
#include <algorithm>
#include <bit>
#include <array>

// [Configuration]
// XSTD_VECTOR_EXT: Enables compiler support for vector extensions, by default detects clang support.
// XSTD_SIMD_WIDTH: Defines the default SIMD length for xstd::max_vec_t.
//
#ifndef XSTD_VECTOR_EXT
	#ifndef __INTELLISENSE__
		#define XSTD_VECTOR_EXT __has_builtin(__builtin_convertvector)
	#else
		#define XSTD_VECTOR_EXT 0
	#endif
#endif
#ifndef XSTD_SIMD_WIDTH
	#if AMD64_TARGET
		#if __AVX512DQ__
			#define XSTD_SIMD_WIDTH (512/8)
		#elif __AVX__
			#define XSTD_SIMD_WIDTH (256/8)
		#else
			#define XSTD_SIMD_WIDTH (128/8)
		#endif
	#else
		#define XSTD_SIMD_WIDTH (128/8)
	#endif
#endif

// Declare helper to check vector builtins.
//
#if XSTD_VECTOR_EXT
	#define __has_vector_builtin( x )      __has_builtin( x )
	#define __has_ia32_vector_builtin( x ) AMD64_TARGET && __has_builtin( x )
#else
	#define __has_vector_builtin( x )      0
	#define __has_ia32_vector_builtin( x ) 0
#endif

// Implements wrappers around Clang vector extensions with fallbacks for non-Clang compilers.
//
namespace xstd
{
	// Declare the vector types.
	//
#if XSTD_VECTOR_EXT && __clang__
	template<typename T, size_t N>
	using xvec = T  __attribute__( ( ext_vector_type( N ) ) );
#elif XSTD_VECTOR_EXT
	template <typename T, size_t N>
	using xvec [[gnu::vector_size(sizeof(T[N]))]] = T;
#else
	template<typename T, size_t N>
	struct xvec
	{
		T data[ N ] = { T() };

		// Indexing.
		//
		FORCE_INLINE constexpr T& operator[]( size_t n ) noexcept { return data[ n ]; }
		FORCE_INLINE constexpr const T& operator[]( size_t n ) const noexcept { return data[ n ]; }

		// Implement each operator.
		//
#define __DECLARE_UNARY(Op)												      							\
		FORCE_INLINE constexpr xvec operator Op() const noexcept	                           \
		{																												\
			xvec result = {};																					   \
			for ( size_t i = 0; i != N; i++ )																\
				result[ i ] = Op data[ i ];														         \
			return result;																							\
		}
#define __DECLARE_BINARY(Op, Opa)												      					\
		FORCE_INLINE constexpr xvec operator Op( const xvec& other ) const noexcept	      \
		{																												\
			xvec result = {};																					   \
			for ( size_t i = 0; i != N; i++ )																\
				result[ i ] = data[ i ] Op other[ i ];														\
			return result;																							\
		}																												\
		FORCE_INLINE constexpr xvec operator Op( T el ) const noexcept							   \
		{																												\
			xvec result = {};																					   \
			for ( size_t i = 0; i != N; i++ )																\
				result[ i ] = data[ i ] Op el;																\
			return result;																							\
		}																												\
		FORCE_INLINE constexpr xvec& operator Opa( const xvec& other ) noexcept			      \
		{																												\
			for ( size_t i = 0; i != N; i++ )																\
				data[ i ] Opa other[ i ];																		\
			return *this;																							\
		}																												\
		FORCE_INLINE constexpr xvec& operator Opa( T el ) noexcept								   \
		{																												\
			for ( size_t i = 0; i != N; i++ )																\
				data[ i ] Opa el;																					\
			return *this;																							\
		}
		
#define __DECLARE_COMPARISON(Op)												      					   \
		FORCE_INLINE constexpr xvec operator Op( const xvec& other ) const noexcept	      \
		{																												\
			xvec result = {};																					   \
			for ( size_t i = 0; i != N; i++ )																\
				result[ i ] = ( data[ i ] Op other[ i ] ) ? T(-1) : T(0);							\
			return result;																							\
		}																												\
		FORCE_INLINE constexpr xvec operator Op( T el ) const noexcept							   \
		{																												\
			xvec result = {};																					   \
			for ( size_t i = 0; i != N; i++ )																\
				result[ i ] = ( data[ i ] Op el ) ? T(-1) : T(0);				          			\
			return result;																							\
		}																												\

		// Arithmetic.
		__DECLARE_BINARY( +, += );
		__DECLARE_BINARY( -, -= );
		__DECLARE_BINARY( *, *= );
		__DECLARE_BINARY( /, /= );
		__DECLARE_BINARY( %, %= );
		__DECLARE_UNARY( - );

		// Binary.
		__DECLARE_BINARY( &, &= );
		__DECLARE_BINARY( |, |= );
		__DECLARE_BINARY( ^, ^= );
		__DECLARE_BINARY( >>, >>= );
		__DECLARE_BINARY( <<, <<= );
		__DECLARE_UNARY( ~ );

		// Comparison.
		__DECLARE_COMPARISON( == );
		__DECLARE_COMPARISON( != );
		__DECLARE_COMPARISON( <= );
		__DECLARE_COMPARISON( >= );
		__DECLARE_COMPARISON( < );
		__DECLARE_COMPARISON( > );

		// Cast to same-size vector.
		template<typename T2, size_t N2> requires ( sizeof( T2[ N2 ] ) == sizeof( T[ N ] ) )
		FORCE_INLINE constexpr operator xvec<T2, N2>() const noexcept
		{
			return std::bit_cast<xvec<T2, N2>>( *this );
		}

#undef __DECLARE_UNARY
#undef __DECLARE_BINARY
#undef __DECLARE_COMPARISON
	};
#endif

	// Byte vector.
	//
	template<size_t N>
	using bvec = xvec<char, N>;

	// Common SIMD sizes.
	//
	template<typename T = char> 
	using vec128_t =  xvec<T, 16 / sizeof( T )>;
	template<typename T = char>
	using vec256_t =  xvec<T, 32 / sizeof( T )>;
	template<typename T = char>
	using vec512_t =  xvec<T, 64 / sizeof( T )>;
	template<typename T = char>
	using max_vec_t = xvec<T, XSTD_SIMD_WIDTH / sizeof( T )>;

	// Vector traits.
	//
	template<typename T>
	struct vector_traits;
	template<typename T, size_t N>
	struct vector_traits<xvec<T, N>>
	{
		using element_type = T;
		static constexpr size_t size = N;
	};
	template<typename T> struct vector_traits<T&>      : vector_traits<T> {};
	template<typename T> struct vector_traits<const T> : vector_traits<T> {};

	template<typename T> 
	using vector_element_t = typename vector_traits<T>::element_type;
	template<typename T> 
	static constexpr size_t vector_size_v = vector_traits<T>::size;
	template<typename T> 
	concept XVector = requires{ typename vector_traits<T>::element_type; };

	// Shuffle traits.
	//
	template<typename V, int... Ix>
	struct shuffle_traits;
	template<typename T, size_t N, int... Ix> requires ( sizeof...( Ix ) != 0 )
	struct shuffle_traits<xvec<T, N>, Ix...>
	{
		using result_type = xvec<T, sizeof...( Ix )>;
		static constexpr bool viable = ( ( Ix < ( N * 2 ) ) && ... );
	};
	template<typename V, int... Ix> struct shuffle_traits<V&, Ix...>      : shuffle_traits<V, Ix...> {};
	template<typename V, int... Ix> struct shuffle_traits<const V, Ix...> : shuffle_traits<V, Ix...> {};

	template<typename V, int... Ix>
	using shuffle_result_t = typename shuffle_traits<V, Ix...>::result_type;
	template<typename V, int... Ix>
	concept ShuffleViable = requires{ typename shuffle_traits<V, Ix...>::result_type; };

	// Vector operations.
	//
	namespace vec
	{
		// Vector construction.
		// - V[N] = { values..., 0... }
		template<size_t N, typename T>
		FORCE_INLINE constexpr xvec<T, N> make()
		{
			return xvec<T, N>{};
		}
		template<size_t N, typename T, typename... Tx> requires ( ( Convertible<Tx, T> && ... ) && ( ( sizeof...( Tx ) + 1 ) <= N ) )
		FORCE_INLINE constexpr xvec<T, N> make( T v, Tx... vx )
		{
			return xvec<T, N>{ v, T( vx )... };
		}
		// - V[] = { values... }
		template<typename T, typename... Tx> requires ( Convertible<Tx, T> && ... )
		FORCE_INLINE constexpr xvec<T, sizeof...( Tx ) + 1> make( T v, Tx... vx )
		{
			return xvec<T, sizeof...( Tx ) + 1>{ v, T( vx )... };
		}
		// - V[N] = { v, v, v... }
		template<size_t N, typename T>
		FORCE_INLINE constexpr xvec<T, N> fill( T v )
		{
#if XSTD_VECTOR_EXT
			// Clang syntax.
			return xvec<T, N>( v );
#else
			xvec<T, N> result = {};
			for ( size_t i = 0; i != N; i++ )
				result[ i ] = v;
			return result;
#endif
		}

		// Vector from array.
		//
		template<typename T, size_t N>
		FORCE_INLINE constexpr xvec<T, N> from_arr( std::array<T, N> arr )
		{
			if constexpr ( sizeof( xvec<T, N> ) == sizeof( std::array<T, N> ) )
				return std::bit_cast< xvec<T, N> >( arr );
		
			xvec<T, N> vec = {};
			for ( size_t i = 0; i != N; i++ )
				vec[ i ] = arr[ i ];
			return vec;
		}
		template<typename T, size_t N>
		FORCE_INLINE constexpr std::array<T, N> to_arr( xvec<T, N> vec )
		{
			if constexpr ( sizeof( xvec<T, N> ) == sizeof( std::array<T, N> ) )
				return std::bit_cast< std::array<T, N> >( vec );
		
			std::array<T, N> arr = {};
			for ( size_t i = 0; i != N; i++ )
				arr[ i ] = vec[ i ];
			return arr;
		}

		// std::size equivalent for vector types.
		//
		template<typename T, size_t N>
		FORCE_INLINE _CONSTEVAL size_t size( const xvec<T, N>& ) noexcept { return N; }

		// Vector cast.
		//
		template<typename To, typename From, size_t N> requires std::is_convertible_v<From, To>
		FORCE_INLINE constexpr xvec<To, N> cast( xvec<From, N> x ) noexcept
		{
#if __has_vector_builtin(__builtin_convertvector)
			if ( !std::is_constant_evaluated() )
				return __builtin_convertvector( x, xvec<To, N> );
#else
			xvec<To, N> result = {};
			for ( size_t i = 0; i != N; i++ )
				result[ i ] = ( To ) x[ i ];
			return result;
#endif
		}

		// Vector shuffling.
		//
		template<int... Ix> requires ( sizeof...( Ix ) != 0 )
		struct shuffle_t
		{
			template<size_t I, typename O, typename V, int Current, int... Rest>
			FORCE_INLINE static constexpr void emplace( O& out, const V& a, const V& b, std::integer_sequence<int, Current, Rest...> ) noexcept
			{
				if constexpr ( Current >= 0 )
				{
					if constexpr ( Current < vector_size_v<V> )
						out[ I ] = a[ Current ];
					else
						out[ I ] = b[ Current - vector_size_v<V> ];
				}
				else
				{
					if ( std::is_constant_evaluated() )
						out[ I ] = 0;
				}

				if constexpr ( sizeof...( Rest ) != 0 )
					emplace<I + 1, O, V, Rest...>( out, a, b, std::integer_sequence<int, Rest...>{} );
			}

			template<typename V>
			FORCE_INLINE static constexpr shuffle_result_t<V, Ix...> apply( V v1, V v2 ) noexcept
			{
				using R = shuffle_result_t<V, Ix...>;
				
				R result = {};
				emplace<0>( result, v1, v2, std::integer_sequence<int, Ix...>{} );
				return result;
			}

			template<typename V> requires ShuffleViable<V, Ix...>
			FORCE_INLINE constexpr shuffle_result_t<V, Ix...> operator()( V v1, V v2 ) const noexcept
			{
#if __has_vector_builtin(__builtin_shufflevector)
				if ( !std::is_constant_evaluated() )
					return __builtin_shufflevector( v1, v2, Ix... );
#endif
				return shuffle_t::apply( v1, v2 );

			}
		};
		template<int... Ix>
		static constexpr shuffle_t<Ix...> shuffle{};

		template<typename V, typename Ti, auto... Ix> requires ShuffleViable<V, int( Ix )...>
		FORCE_INLINE constexpr shuffle_result_t<V, int(Ix)...> shuffle_seq( V v1, V v2, std::integer_sequence<Ti, Ix...> ) noexcept
		{
			return shuffle<int( Ix )...>( v1, v2 );
		}

		// Vector extension/shrinking.
		//
		namespace impl
		{
			template<size_t I, size_t N1, size_t N2, int... Ix>
			FORCE_INLINE constexpr auto make_resize_sequence()
			{
				// If we reached the end, return it:
				if constexpr ( I == N2 )
					return std::integer_sequence<int, Ix...>{};
				// If we're at the current vector:
				else if constexpr( I < N1 )
					return make_resize_sequence<I + 1, N1, N2, Ix..., int( I )>();
				// If we're at the undefined vector:
				else
					return make_resize_sequence<I + 1, N1, N2, Ix..., int( -1 )>();
			}
		};
		template<size_t N1, size_t N2>
		constexpr auto make_resize_sequence() { return impl::make_resize_sequence<0, N1, N2>(); }

		template<size_t N2, typename T, size_t N1>
		FORCE_INLINE constexpr xvec<T, N2> resize( xvec<T, N1> vec ) noexcept
		{
			if constexpr ( N2 == N1 )
				return vec;
			else
				return shuffle_seq( vec, vec, make_resize_sequence<N1, N2>() );
		}

		// Vector combination.
		//
		namespace impl
		{
			template<size_t I, size_t N1, size_t N2, int... Ix>
			FORCE_INLINE constexpr auto make_combination_sequence()
			{
				// Extended max index.
				constexpr size_t NE = N1 >= N2 ? N1 : N2;

				// If we reached the end, return it:
				if constexpr ( I == ( N1 + N2 ) )
					return std::integer_sequence<int, Ix...>{};
				// If we're at the first vector:
				else if constexpr ( I < N1 )
					return make_combination_sequence<I + 1, N1, N2, Ix..., int( I )>();
				// If we're at the second vector:
				else
					return make_combination_sequence<I + 1, N1, N2, Ix..., int( NE + (I - N1) )>();
			}
		};
		template<size_t N1, size_t N2>
		constexpr auto make_combination_sequence() { return impl::make_combination_sequence<0, N1, N2>(); }

		template<typename T, size_t N1, size_t N2>
		FORCE_INLINE constexpr xvec<T, N1 + N2> combine( xvec<T, N1> a, xvec<T, N2> b ) noexcept
		{
			constexpr size_t NE = N1 >= N2 ? N1 : N2;
			return shuffle_seq( resize<NE>( a ), resize<NE>( b ), make_combination_sequence<N1, N2>() );
		}


		template<typename T, size_t N>
		FORCE_INLINE constexpr xvec<T, N + 1> push( xvec<T, N> a, T value ) noexcept
		{
			auto result = resize<N + 1>( a );
			result[ N ] = value;
			return result;
		}

		// Vector bytes.
		//
		template<typename T, size_t N>
		FORCE_INLINE constexpr bvec<sizeof( T[ N ] )> as_bytes( xvec<T, N> x ) noexcept
		{
			return ( bvec<sizeof( T[ N ] )> ) x;
		}

		// Vector test.
		//
		template<typename T, size_t N>
		FORCE_INLINE constexpr bool testz( xvec<T, N> x ) noexcept
		{
			auto bytes = as_bytes( x );

			if ( !std::is_constant_evaluated() )
			{
#if __has_ia32_vector_builtin( __builtin_ia32_ptestz128 )
				if constexpr ( sizeof( x ) == sizeof( vec128_t<> ) )
					return ( bool ) __builtin_ia32_ptestz128( bytes, bytes );
#endif
#if __has_ia32_vector_builtin( __builtin_ia32_ptestz256 )
				if constexpr ( sizeof( x ) == sizeof( vec256_t<> ) )
					return ( bool ) __builtin_ia32_ptestz256( bytes, bytes );
#endif
			}

			bool result = true;
			for ( size_t i = 0; i != N; i++ )
				result &= bytes[ i ] == 0;
			return result;
		}

		// Element-wise operations.
		//
		template<typename T, size_t N>
		FORCE_INLINE constexpr xvec<T, N> (max)( xvec<T, N> x, xvec<T, N> y ) noexcept
		{
#if __has_vector_builtin(__builtin_elementwise_max)
			if ( !std::is_constant_evaluated() )
				return __builtin_elementwise_max( x, y );
#else
			for ( size_t i = 0; i != N; i++ )
				x[ i ] = std::max<T>( x[ i ], y[ i ] );
			return x;
#endif
		}
		template<typename T, size_t N>
		FORCE_INLINE constexpr xvec<T, N> (min)( xvec<T, N> x, xvec<T, N> y ) noexcept
		{
#if __has_vector_builtin(__builtin_elementwise_min)
			if ( !std::is_constant_evaluated() )
				return __builtin_elementwise_min( x, y );
#else
			for ( size_t i = 0; i != N; i++ )
				x[ i ] = std::min<T>( x[ i ], y[ i ] );
			return x;
#endif
		}
	};
};