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
	#define XSTD_VECTOR_EXT __has_builtin(__builtin_convertvector)
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
		T data[ N ];

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
	
	// Vector construction.
	// - V[N] = { values..., 0 }
	template<size_t N, typename T>
	FORCE_INLINE constexpr xvec<T, N> make_vec_n()
	{
		return xvec<T, N>{};
	}
	template<size_t N, typename T, typename... Tx> requires ( ( std::is_convertible_v<Tx, T> && ... ) && ( ( sizeof...( Tx ) + 1 ) <= N ) )
	FORCE_INLINE constexpr xvec<T, N> make_vec_n( T v, Tx... vx )
	{
		return xvec<T, N>{ v, T( vx )... };
	}
	// - V[] = { values... }
	template<typename T, typename... Tx> requires ( std::is_convertible_v<Tx, T> && ... )
	FORCE_INLINE constexpr xvec<T, sizeof...( Tx ) + 1> make_vec( T v, Tx... vx )
	{
		return xvec<T, sizeof...( Tx ) + 1>{ v, T( vx )... };
	}
	// - V[N] = { v, v, v... }
	template<size_t N, typename T>
	FORCE_INLINE constexpr xvec<T, N> fill_vec( T v )
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
	template<typename V1, typename V2, size_t... Ix>
	struct shuffle_traits;
	template<typename E, size_t N1, size_t N2, size_t... Ix> requires ( sizeof...( Ix ) != 0 )
	struct shuffle_traits<xvec<E, N1>, xvec<E, N2>, Ix...>
	{
		using result_type = xvec<E, sizeof...( Ix )>;
		static constexpr bool viable = ( ( Ix < ( N1 + N2 ) ) && ... );
	};
	template<typename V1, typename V2, size_t... Ix> struct shuffle_traits<V1&, V2&, Ix...>           : shuffle_traits<V1, V2, Ix...> {};
	template<typename V1, typename V2, size_t... Ix> struct shuffle_traits<const V1, const V2, Ix...> : shuffle_traits<V1, V2, Ix...> {};

	template<typename V1, typename V2, size_t... Ix> 
	using shuffle_result_t = typename shuffle_traits<V1, V2, Ix...>::result_type;
	template<typename V1, typename V2, size_t... Ix> 
	concept ShuffleViable = requires{ typename shuffle_traits<V1, V2, Ix...>::result_type; };

	// Vector reference type.
	// - If there is compiler support, pass by register.
	//
#if XSTD_VECTOR_EXT
	template<typename T, size_t N>
	using xvec_ref =  xvec<T, N>;
	template<typename T, size_t N>
	using xvec_cref = xvec<T, N>;
#else
	template<typename T, size_t N>
	using xvec_ref =  xvec<T, N>&;
	template<typename T, size_t N>
	using xvec_cref = const xvec<T, N>&;
#endif

	// Vector operations.
	//
	namespace vec
	{
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
		FORCE_INLINE constexpr xvec<To, N> cast( xvec_cref<From, N> x ) noexcept
		{
#if __has_vector_builtin(__builtin_convertvector)
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
		template<size_t... Ix> requires ( sizeof...( Ix ) != 0 )
		struct shuffle_t
		{
#if !__has_vector_builtin(__builtin_shufflevector)
			static constexpr size_t index_list[] = { Ix... };
			template<size_t I, typename O, typename V1, typename V2>
			FORCE_INLINE static constexpr void emplace( O& out, const V1& a, const V2& b ) noexcept
			{
				if constexpr ( I < sizeof...( Ix ) )
				{
					size_t Iv = index_list[ I ];
					if ( Iv < vector_size_v<V1> )
						out[ I ] = a[ Iv ];
					else
						out[ I ] = b[ Iv - vector_size_v<V1> ];
					emplace<I + 1, O, V1, V2>( out, a, b );
				}
			}

			template<typename V1, typename V2>
			FORCE_INLINE static constexpr shuffle_result_t<V1, V2, Ix...> apply( const V1& v1, const V2& v2 ) noexcept
			{
				shuffle_result_t<V1, V2, Ix...> result = {};
				emplace<0>( result, v1, v2 );
				return result;
			}
#else
			template<typename V1, typename V2>
			FORCE_INLINE static constexpr shuffle_result_t<V1, V2, Ix...> apply( V1 v1, V2 v2 ) noexcept
			{
				return __builtin_shufflevector( v1, v2, Ix... );
			}
#endif
			template<typename V1, typename V2> requires ShuffleViable<V1, V2, Ix...>
			FORCE_INLINE constexpr shuffle_result_t<V1, V2, Ix...> operator()( V1&& v1, V2&& v2 ) const noexcept
			{
				return shuffle_t::apply( std::forward<V1>( v1 ), std::forward<V2>( v2 ) );
			}
		};
		template<size_t... Ix>
		static constexpr shuffle_t<Ix...> shuffle{};

		template<typename V1, typename V2, size_t... Ix> requires ShuffleViable<V1, V2, Ix...>
		FORCE_INLINE constexpr shuffle_result_t<V1, V2, Ix...> shuffle_seq( V1&& v1, V2&& v2, std::index_sequence<Ix...> ) noexcept
		{
			return shuffle_t<Ix...>::apply( v1, v2 );
		}

		// Vector combination.
		//
		template<typename T, size_t N1, size_t N2>
		FORCE_INLINE constexpr xvec<T, N1 + N2> combine( xvec_cref<T, N1> a, xvec_cref<T, N2> b ) noexcept
		{
			return shuffle_seq( a, b, std::make_index_sequence<N1 + N2>{} );
		}

		// Vector bytes.
		//
		template<typename T, size_t N>
		FORCE_INLINE constexpr bvec<sizeof( T[ N ] )> as_bytes( xvec_cref<T, N> x ) noexcept
		{
			return ( bvec<sizeof( T[ N ] )> ) x;
		}

		// Vector test.
		//
		template<typename T, size_t N>
		FORCE_INLINE constexpr bool testz( xvec_cref<T, N> x ) noexcept
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
		FORCE_INLINE constexpr xvec<T, N> (max)( xvec<T, N> x, xvec_cref<T, N> y ) noexcept
		{
#if __has_vector_builtin(__builtin_elementwise_max)
			return __builtin_elementwise_max( x, y );
#else
			for ( size_t i = 0; i != N; i++ )
				x[ i ] = std::max<T>( x[ i ], y[ i ] );
			return x;
#endif
		}
		template<typename T, size_t N>
		FORCE_INLINE constexpr xvec<T, N> (min)( xvec<T, N> x, xvec_cref<T, N> y ) noexcept
		{
#if __has_vector_builtin(__builtin_elementwise_min)
			return __builtin_elementwise_min( x, y );
#else
			for ( size_t i = 0; i != N; i++ )
				x[ i ] = std::min<T>( x[ i ], y[ i ] );
			return x;
#endif
		}
	};
};