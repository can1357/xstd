#pragma once
#include "intrinsics.hpp"
#include <algorithm>
#include <bit>
#include <array>
#include <span>

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
#if XSTD_VECTOR_EXT
	template <typename T, size_t N>
	using native_vector [[gnu::vector_size(sizeof(T[N]))]] = T;
#else
	template <typename T, size_t N>
	using native_vector = std::array<T, std::bit_ceil( N )>;
#endif

	// Sequence helpers.
	//
	namespace impl
	{
		template<size_t I, size_t N1, size_t N2, int... Ix>
		FORCE_INLINE constexpr auto make_slice_sequence()
		{
			// If we reached the end, return it:
			if constexpr ( I == N2 )
				return std::integer_sequence<int, Ix...>{};
			// If we're at the current vector:
			else if constexpr ( I < N1 )
				return make_slice_sequence<I + 1, N1, N2, Ix..., int( I )>();
			// If we're at the undefined vector:
			else
				return make_slice_sequence<I + 1, N1, N2, Ix..., int( -1 )>();
		}
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
				return make_combination_sequence<I + 1, N1, N2, Ix..., int( NE + ( I - N1 ) )>();
		}
	};
	template<size_t N1, size_t N2> constexpr auto make_resize_sequence() { return impl::make_slice_sequence<0, N1, N2>(); }
	template<size_t N1, size_t N2> constexpr auto make_combination_sequence() { return impl::make_combination_sequence<0, N1, N2>(); }
	template<size_t N1, size_t Offset, size_t Count> constexpr auto make_slice_sequence() { return impl::make_slice_sequence<Offset, N1, (Offset+Count)>(); }

	template<Arithmetic T, size_t N>
	union alignas( native_vector<T, N> ) xvec
	{
		// Storage.
		//
		std::array<T, N> _data = {};
		native_vector<T, N> _nat;

		// Default copy.
		//
		constexpr xvec( const xvec& ) = default;
		constexpr xvec& operator=( const xvec& ) = default;

		// Construction by native vector.
		//
		FORCE_INLINE explicit constexpr xvec( std::in_place_t, native_vector<T, N> nat ) noexcept : _nat( nat ) {}
		
		// Construction by value.
		//
#if XSTD_VECTOR_EXT
		template<Arithmetic... Tx>
		FORCE_INLINE constexpr xvec( Tx... values ) noexcept
		{
			if ( !std::is_constant_evaluated() )
				_nat = native_vector<T, N>{ T( values )... };
			else
			{
				T arr[] = { values... };
				for ( size_t i = 0; i != sizeof...( values ); i++ )
					_data[ i ] = arr[ i ];
			}
		}
#else
		template<Arithmetic... Tx>
		FORCE_INLINE constexpr xvec( Tx... values ) noexcept : _data{ T(values)... } {}
#endif

		// Vector fill.
		//
		FORCE_INLINE void fill( T value ) noexcept
		{
#if XSTD_VECTOR_EXT
			if ( !std::is_constant_evaluated() )
				return ( void ) ( _nat = native_vector<T, N>( value ) );
#endif
			for ( size_t i = 0; i != N; i++ )
				_data[ i ] = value;
		}

		// Indexing.
		//
		FORCE_INLINE constexpr T at( size_t n ) const noexcept
		{
			if ( !std::is_constant_evaluated() ) return _nat[ n ];
			else                                 return _data[ n ];
		}
		FORCE_INLINE constexpr void set( size_t n, T value ) noexcept
		{
			if ( !std::is_constant_evaluated() ) _nat[ n ] = value;
			else                                 _data[ n ] = value;
		}

		struct mut_proxy
		{
			xvec& ref;
			const size_t n;
			constexpr operator T() const noexcept { return ref.at( n ); }
			constexpr mut_proxy& operator=( T v ) noexcept { ref.set( n, v ); return *this; }
		};
		FORCE_INLINE constexpr T operator[]( size_t n ) const noexcept { return at( n ); }
		FORCE_INLINE constexpr mut_proxy operator[]( size_t n ) noexcept { return { *this, n }; }

		// Traits.
		//
		FORCE_INLINE _CONSTEVAL size_t size() const noexcept { return N; }
		FORCE_INLINE _CONSTEVAL size_t capacity() const noexcept { return N; }
		
		// Implement each operator.
		//
#define __DECLARE_UNARY(Op, Req)												      				\
		FORCE_INLINE constexpr xvec operator Op() const Req						         \
		{																										\
			if constexpr ( XSTD_VECTOR_EXT )															\
				if ( !std::is_constant_evaluated() )												\
					return xvec( std::in_place_t{}, Op _nat );									\
			xvec result = {};																				\
			for ( size_t i = 0; i != N; i++ )														\
				result._data[ i ] = Op _data[ i ];													\
			return result;																					\
		}																										\
																												
#define __DECLARE_BINARY(Op, Opa, Req)												      		\
		FORCE_INLINE constexpr xvec operator Op( const xvec& other ) const Req	      \
		{																										\
			if constexpr ( XSTD_VECTOR_EXT )															\
				if ( !std::is_constant_evaluated() )												\
					return xvec( std::in_place_t{}, _nat Op other._nat );						\
			xvec result = {};																				\
			for ( size_t i = 0; i != N; i++ )														\
				result._data[ i ] = _data[ i ] Op other[ i ];									\
			return result;																					\
		}																										\
		FORCE_INLINE constexpr xvec operator Op( T other ) const Req					   \
		{																										\
			if constexpr ( XSTD_VECTOR_EXT )															\
				if ( !std::is_constant_evaluated() )												\
					return xvec( std::in_place_t{}, _nat Op other );							\
			xvec result = {};																				\
			for ( size_t i = 0; i != N; i++ )														\
				result._data[ i ] = _data[ i ] Op other;											\
			return result;																					\
		}																										\
		FORCE_INLINE constexpr xvec& operator Opa( const xvec& other ) Req			   \
		{																										\
			if constexpr ( XSTD_VECTOR_EXT )															\
			{																									\
				if ( !std::is_constant_evaluated() )												\
				{																								\
					_nat Opa other._nat;																	\
					return *this;																			\
				}																								\
			}																									\
			for ( size_t i = 0; i != N; i++ )														\
				_data[ i ] Opa other[ i ];																\
			return *this;																					\
		}																										\
		FORCE_INLINE constexpr xvec& operator Opa( T other ) Req						      \
		{																										\
			if constexpr ( XSTD_VECTOR_EXT )															\
			{																									\
				if ( !std::is_constant_evaluated() )												\
				{																								\
					_nat Opa other;     																	\
					return *this;																			\
				}																								\
			}																									\
			for ( size_t i = 0; i != N; i++ )														\
				_data[ i ] Opa other;																	\
			return *this;																					\
		}																										
#define __DECLARE_COMPARISON(Op, Req)												      		\
		FORCE_INLINE constexpr xvec operator Op( const xvec& other ) const Req	      \
		{																										\
			if constexpr ( XSTD_VECTOR_EXT )															\
				if ( !std::is_constant_evaluated() )												\
					return xvec( std::in_place_t{}, _nat Op other._nat );						\
			xvec result = {};																				\
			for ( size_t i = 0; i != N; i++ )														\
				result._data[ i ] = ( _data[ i ] Op other[ i ] ) ? T( -1 ) : T( 0 );		\
			return result;																					\
		}																										\
		FORCE_INLINE constexpr xvec operator Op( T other ) const Req					   \
		{																										\
			if constexpr ( XSTD_VECTOR_EXT )															\
				if ( !std::is_constant_evaluated() )												\
					return xvec( std::in_place_t{}, _nat Op other );							\
			xvec result = {};																				\
			for ( size_t i = 0; i != N; i++ )														\
				result._data[ i ] = ( _data[ i ] Op other ) ? T( -1 ) : T( 0 );			\
			return result;																					\
		}																										

		// Arithmetic.
		__DECLARE_BINARY( +, +=, noexcept );
		__DECLARE_BINARY( -, -=, noexcept );
		__DECLARE_BINARY( *, *=, noexcept );
		__DECLARE_BINARY( /, /=, noexcept );
		__DECLARE_BINARY( %, %=, noexcept );
		__DECLARE_UNARY( -, noexcept );

		// Binary.
		__DECLARE_BINARY( &, &=, noexcept requires Integral<T> );
		__DECLARE_BINARY( |, |=, noexcept requires Integral<T> );
		__DECLARE_BINARY( ^, ^=, noexcept requires Integral<T> );
		__DECLARE_BINARY( >>, >>=, noexcept requires Integral<T> );
		__DECLARE_BINARY( <<, <<=, noexcept requires Integral<T> );
		__DECLARE_UNARY( ~, noexcept requires Integral<T> );

		// Comparison.
		__DECLARE_COMPARISON( ==, noexcept );
		__DECLARE_COMPARISON( !=, noexcept );
		__DECLARE_COMPARISON( <=, noexcept );
		__DECLARE_COMPARISON( >=, noexcept );
		__DECLARE_COMPARISON( <, noexcept );
		__DECLARE_COMPARISON( >, noexcept );
#undef __DECLARE_UNARY
#undef __DECLARE_BINARY
#undef __DECLARE_COMPARISON

		// Cast to same-size vector.
		//
		template<typename Ty, size_t N2> requires ( !Same<T, Ty> && sizeof( Ty[ N2 ] ) == sizeof( T[ N ] ) )
		FORCE_INLINE constexpr operator xvec<Ty, N2>() const noexcept
		{
			if constexpr ( XSTD_VECTOR_EXT )
				if ( !std::is_constant_evaluated() )
					return xvec<Ty, N2>( std::in_place_t{}, ( native_vector<Ty, N2> ) _nat );
			return std::bit_cast<xvec<Ty, N2>>( *this );
		}

		// Cast to same-length vector.
		//
		template<typename Ty>
		[[nodiscard]] FORCE_INLINE constexpr xvec<Ty, N> cast() const noexcept
		{
			if constexpr ( Same<Ty, T> )
				return *this;

#if __has_vector_builtin(__builtin_convertvector)
			if ( !std::is_constant_evaluated() )
				return xvec<Ty, N>( std::in_place_t{}, __builtin_convertvector( _nat, native_vector<Ty, N> ) );
#endif
			xvec<Ty, N> result = {};
			for ( size_t i = 0; i != N; i++ )
				result._data[ i ] = ( Ty ) _data[ i ];
			return result;
		}

		// Array conversion.
		//
		FORCE_INLINE constexpr operator std::array<T, N>() const noexcept { return _data; }
		FORCE_INLINE constexpr std::array<T, N> to_array() const noexcept { return _data; }

		// Vector shuffle.
		//
		template<int... Ix>
		FORCE_INLINE constexpr xvec<T, sizeof...( Ix )> shuffle( const xvec& other, std::integer_sequence<int, Ix...> seq = {} ) const noexcept
		{
#if __has_vector_builtin(__builtin_shufflevector)
			if ( !std::is_constant_evaluated() )
				return xvec<T, sizeof...( Ix )>( std::in_place_t{}, __builtin_shufflevector( _nat, other._nat, Ix... ) );
#endif

			xvec<T, sizeof...( Ix )> out = {};
			auto emplace = [ & ] <int Current, int... Rest> ( auto&& self, std::integer_sequence<int, Current, Rest...> ) FORCE_INLINE
			{
				constexpr size_t I = sizeof...( Ix ) - ( sizeof...( Rest ) + 1 );
				if constexpr ( Current >= 0 )
				{
					if constexpr ( Current < N )
						out._data[ I ] = _data[ Current ];
					else
						out._data[ I ] = other._data[ Current - N ];
				}
				else
				{
					if ( std::is_constant_evaluated() )
						out._data[ I ] = 0;
				}

				if constexpr ( sizeof...( Rest ) != 0 )
					self( self, std::integer_sequence<int, Rest...>{} );
			};
			emplace( emplace, seq );
			return out;
		}

		// Vector slicing.
		//
		template<size_t Offset, size_t _Count = std::string::npos> requires ( Offset < N )
		[[nodiscard]] FORCE_INLINE constexpr auto slice() const noexcept
		{
			constexpr size_t Count = _Count == std::string::npos ? ( N - Offset ) : _Count;
			return shuffle( *this, make_slice_sequence<N, Offset, Count>() );

		}
		template<size_t N2>
		[[nodiscard]] FORCE_INLINE constexpr xvec<T, N2> resize() const noexcept
		{
			if constexpr ( N2 == N )
				return *this;
			else
				return shuffle( *this, make_resize_sequence<N, N2>() );
		}

		// Vector combine.
		//
		template<size_t N2>
		[[nodiscard]] FORCE_INLINE constexpr xvec<T, N+N2> combine( const xvec<T, N2>& other ) const noexcept
		{
			constexpr size_t NE = N >= N2 ? N : N2;
			auto a = resize<NE>();
			auto b = other.template resize<NE>();
			return a.shuffle( b, make_combination_sequence<N, N2>() );
		}
		[[nodiscard]] FORCE_INLINE constexpr xvec<T, N + 1> combine( T value ) const noexcept
		{
			auto result = resize<N+1>();
			result.set( N, value );
			return result;
		}

		// Vector bytes.
		//
		FORCE_INLINE constexpr xvec<char, sizeof(T[N])> bytes() const noexcept
		{
			return ( xvec<char, sizeof( T[ N ] )> ) *this;
		}

		// Vector zero comparison.
		//
		FORCE_INLINE constexpr bool is_zero() const noexcept
		{
			if ( !std::is_constant_evaluated() )
			{
#if __has_ia32_vector_builtin( __builtin_ia32_ptestz128 )
				if constexpr ( sizeof( _nat ) == 16 )
					return ( bool ) __builtin_ia32_ptestz128( ( native_vector<char, 16> ) _nat, ( native_vector<char, 16> ) _nat );
#endif
#if __has_ia32_vector_builtin( __builtin_ia32_ptestz256 )
				if constexpr ( sizeof( _nat ) == 32 )
					return ( bool ) __builtin_ia32_ptestz256( ( native_vector<char, 32> ) _nat, ( native_vector<char, 32> ) _nat );
#endif
			}

			auto b = bytes();
			bool result = true;
			for ( size_t i = 0; i != N; i++ )
				result &= b._data[ i ] == 0;
			return result;
		}

		// Vector equal comparison.
		//
		FORCE_INLINE constexpr bool equals( xvec<T, N> other ) const noexcept
		{
			return ( bytes() ^ other.bytes() ).is_zero();
		}

		// Tie for hashing and string conversion.
		//
		FORCE_INLINE constexpr auto tie() noexcept { return std::tie( _data ); }
	};

	// Deduction guide.
	//
	template<typename T, typename... Tx>
	xvec( T, Tx... )->xvec<T, sizeof...( Tx ) + 1>;

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
		using value_type =             T;
		static constexpr size_t size = N;
	};
	template<typename T> struct vector_traits<T&>      : vector_traits<T> {};
	template<typename T> struct vector_traits<const T> : vector_traits<T> {};

	template<typename T> 
	using vector_element_t = typename vector_traits<T>::value_type;
	template<typename T> 
	static constexpr size_t vector_size_v = vector_traits<T>::size;
	template<typename T> 
	concept Vector = requires{ typename vector_traits<T>::element_type; };

	// Vector operations.
	//
	namespace vec
	{
		// Syntax sugars.
		//
		template<typename Tn, typename T, size_t N>
		FORCE_INLINE constexpr xvec<Tn, N> cast( xvec<T, N> vec ) noexcept { return vec.template cast<Tn>(); }
		template<typename T, size_t N> 
		FORCE_INLINE constexpr bvec<sizeof( T[ N ] )> bytes( xvec<T, N> x ) noexcept { return x.bytes(); }

		// Vector from array.
		//
		template<typename T, size_t N>
		FORCE_INLINE constexpr xvec<T, N> from( std::array<T, N> arr )
		{
			if constexpr ( sizeof( xvec<T, N> ) == sizeof( std::array<T, N> ) )
				return std::bit_cast< xvec<T, N> >( arr );

			xvec<T, N> vec = {};
			vec._data = arr;
			return vec;
		}

		// Vector from constant span.
		//
		template<typename T, size_t N> requires ( N != std::dynamic_extent )
		FORCE_INLINE constexpr xvec<T, N> from( std::span<T, N> span )
		{
			xvec<T, N> vec = {};
			for( size_t n = 0; n != N; n++ )
				vec._data[ n ] = span[ n ];
			return vec;
		}

		// Element-wise operations.
		//
		template<typename T, size_t N>
		FORCE_INLINE constexpr xvec<T, N> (max)( xvec<T, N> x, xvec<T, N> y ) noexcept
		{
#if __has_vector_builtin(__builtin_elementwise_max)
			if ( !std::is_constant_evaluated() )
				return xvec<T, N>( std::in_place_t{}, __builtin_elementwise_max( x, y ) );
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
				return xvec<T, N>( std::in_place_t{}, __builtin_elementwise_min( x, y ) );
#else
			for ( size_t i = 0; i != N; i++ )
				x[ i ] = std::min<T>( x[ i ], y[ i ] );
			return x;
#endif
		}
	};
};