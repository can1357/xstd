#pragma once
#include "type_helpers.hpp"
#include "xvector.hpp"
#include "formatting.hpp"
#include <math.h>
#include <array>
#include <span>

// [[Configuration]]
// XSTD_MATH_FP64:          If set uses double as default.
// XSTD_MATH_USE_X86INTRIN: If set, enabled optimization using intrinsics if not constexpr.
// XSTD_MATH_USE_POLY_TRIG: If set, uses sin/cos approximations instead of libc.
//
#ifndef XSTD_MATH_USE_POLY_TRIG
	#define XSTD_MATH_USE_POLY_TRIG 1
#endif
#ifndef XSTD_MATH_FP64
	#define XSTD_MATH_FP64 0
#endif
#ifndef XSTD_MATH_USE_X86INTRIN
	#if AMD64_TARGET && CLANG_COMPILER
		#define XSTD_MATH_USE_X86INTRIN 1
	#else
		#define XSTD_MATH_USE_X86INTRIN 0
	#endif
#endif
#if XSTD_MATH_USE_X86INTRIN
	#include <immintrin.h>
	#include <fmaintrin.h>
	#include <smmintrin.h>
#endif

// Defines useful math primitives.
//
namespace xstd::math
{
	// Types.
	//
#if XSTD_MATH_FP64
#define fp_t double
#else
#define fp_t float
#endif
	template<typename F>
	concept Float = std::is_floating_point_v<F>;
	
	// Intrinsics helpers.
	//
#if XSTD_MATH_USE_X86INTRIN
	namespace impl
	{
		template<typename V>
		FORCE_INLINE static void load_matrix_f32( const V( &src )[ 4 ], __m256& v01, __m256& v23 )
		{
			v01 = _mm256_loadu_ps( &src[ 0 ][ 0 ] );
			v23 = _mm256_loadu_ps( &src[ 2 ][ 0 ] );
		}
		template<typename T>
		FORCE_INLINE static T store_matrix_f32( const __m256& v01, const __m256& v23 )
		{
			T result;
			_mm256_storeu_ps( &result[ 0 ][ 0 ], v01 );
			_mm256_storeu_ps( &result[ 2 ][ 0 ], v23 );
			return result;
		}
		FORCE_INLINE static void tranpose_inplace_f32( __m256& v01, __m256& v23 )
		{
			__v8si mask{ 0, 4, 2, 6, 1, 5, 3, 7 };
			__m256 q01 = _mm256_permutevar8x32_ps( v01, ( __m256i ) mask );
			__m256 q23 = _mm256_permutevar8x32_ps( v23, ( __m256i ) mask );
			v01 = _mm256_unpacklo_pd( ( __m256d )q01, ( __m256d ) q23 );
			v23 = _mm256_unpackhi_pd( ( __m256d )q01, ( __m256d )q23 );
		}
	};
#endif

	// Math constants.
	//
	inline constexpr fp_t pi =      ( fp_t ) 3.14159265358979323846;
	inline constexpr fp_t e =       ( fp_t ) 2.71828182845904523536;
	inline constexpr fp_t flt_min = (std::numeric_limits<fp_t>::min)();
	inline constexpr fp_t flt_max = (std::numeric_limits<fp_t>::max)();
	inline constexpr fp_t flt_eps = std::numeric_limits<fp_t>::epsilon();
	inline constexpr fp_t flt_nan = std::numeric_limits<fp_t>::quiet_NaN();
	inline constexpr fp_t flt_inf = std::numeric_limits<fp_t>::infinity();
	template<Float F = fp_t> inline constexpr F fp_min = (std::numeric_limits<F>::min)();
	template<Float F = fp_t> inline constexpr F fp_max = (std::numeric_limits<F>::max)();
	template<Float F = fp_t> inline constexpr F fp_eps = std::numeric_limits<F>::epsilon();
	template<Float F = fp_t> inline constexpr F fp_nan = std::numeric_limits<F>::quiet_NaN();
	template<Float F = fp_t> inline constexpr F fp_inf = std::numeric_limits<F>::infinity();

	// Fast floating point intrinsics.
	//
#if !CLANG_COMPILER || defined(__INTELLISENSE__)
	template<Float F = fp_t> inline F fsqrt( F x ) { return sqrt( x ); }
	template<Float F = fp_t> inline F fpow( F x, F y ) { return pow( x, y ); }
	template<Float F = fp_t> inline F _fsin( F x ) { return sin( x ); }
	template<Float F = fp_t> inline F _fcos( F x ) { return cos( x ); }
	template<Float F = fp_t> inline F _fcopysign( F m, F s ) { return copysign( m, s ); }
	template<Float F = fp_t> inline F _fabs( F x ) { return fabs( x ); }
	template<Float F = fp_t> inline F _ffloor( F x ) { return floor( x ); }
	template<Float F = fp_t> inline F _fceil( F x ) { return ceil( x ); }
	template<Float F = fp_t> inline F _fround( F x ) { return round( x ); }
	template<Float F = fp_t> inline F _ftrunc( F x ) { return trunc( x ); }
#else
	float fsqrt( float x ) asm( "llvm.sqrt.f32" );
	float fpow( float x, float y ) asm( "llvm.pow.f32" );
	float _fsin( float x ) asm( "llvm.sin.f32" );
	float _fcos( float x ) asm( "llvm.cos.f32" );
	float _fcopysign( float m, float s ) asm( "llvm.copysign.f32" );
	float _fabs( float x ) asm( "llvm.fabs.f32" );
	float _ffloor( float x ) asm( "llvm.floor.f32" );
	float _fceil( float x ) asm( "llvm.ceil.f32" );
	float _fround( float x ) asm( "llvm.round.f32" );
	float _ftrunc( float x ) asm( "llvm.trunc.f32" );
	double fsqrt( double x ) asm( "llvm.sqrt.f64" );
	double fpow( double x, double y ) asm( "llvm.pow.f64" );
	double _fsin( double x ) asm( "llvm.sin.f64" );
	double _fcos( double x ) asm( "llvm.cos.f64" );
	double _fcopysign( double m, double s ) asm( "llvm.copysign.f64" );
	double _fabs( double x ) asm( "llvm.fabs.f64" );
	double _ffloor( double x ) asm( "llvm.floor.f64" );
	double _fceil( double x ) asm( "llvm.ceil.f64" );
	double _fround( double x ) asm( "llvm.round.f64" );
	double _ftrunc( double x ) asm( "llvm.trunc.f64" );
#endif
	template<typename V>
	CONST_FN FORCE_INLINE inline constexpr V rcp( V v )
	{
		return 1 / v;
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr F fabs( F a )
	{
		if ( !std::is_constant_evaluated() )
			return _fabs( a );
		if ( a < 0 ) a = -a;
		return a;
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr F fcopysign( F m, F s )
	{
		if ( !std::is_constant_evaluated() )
			return _fcopysign( m, s );
		m = fabs( m );
		if ( s < 0 ) m = -m;
		return m;
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr F ffloor( F x )
	{
		if ( !std::is_constant_evaluated() )
			return _ffloor( x );
		F d = x - int64_t( x );
		if ( d != 0 ) d = -1;
		return ( F ) int64_t( x + d );
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr F fceil( F x )
	{
		if ( !std::is_constant_evaluated() )
			return _fceil( x );
		F d = x - int64_t( x );
		if ( d != 0 ) d = +1;
		return int64_t( x + d );
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr F fround( F x )
	{
		if ( !std::is_constant_evaluated() )
			return _fround( x );
		return F( int64_t( x + fcopysign( F(0.5), x ) ) );
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr F ftrunc( F x )
	{
		if ( !std::is_constant_evaluated() )
			return _ftrunc( x );
		return F( int64_t( x ) );
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr F fmod( F x, F y )
	{
		return x - ftrunc( x * rcp( y ) ) * y;
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr F foddsgn( F x )
	{
		if ( std::is_constant_evaluated() )
			return ( int64_t( x ) % 2 ) ? -1 : 1;
		x *= F(0.5);
		return x - fceil( x );
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr F fxorsgn( F x, F y )
	{
		if ( std::is_constant_evaluated() )
			return ( ( x < 0 ) != ( y < 0 ) ) ? -1 : 1;
		else
			return bit_cast< F >( bit_cast< convert_uint_t<F> >( x ) ^ bit_cast< convert_uint_t<F> >( y ) );
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr F fifsgn( F x, F value )
	{
		return x < 0 ? value : 0;
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr F fmin( F a, F b )
	{
#if __has_builtin(__builtin_fmin)
		if ( !std::is_constant_evaluated() ) {
			if constexpr ( std::is_same_v<F, float> )
				return __builtin_fminf( a, b );
			else
				return __builtin_fmin( a, b );
		}
#endif
		if ( b < a ) a = b;
		return a;
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr F fmax( F a, F b )
	{
#if __has_builtin(__builtin_fmin)
		if ( !std::is_constant_evaluated() ) {
			if constexpr( std::is_same_v<F, float> )
				return __builtin_fmaxf( a, b );
			else
				return __builtin_fmax( a, b );
		}
#endif
		if ( a < b ) a = b;
		return a;
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr F fclamp( F v, F vmin, F vmax )
	{
		return fmin( fmax( v, vmin ), vmax );
	}
	template<Float F = fp_t, typename... Tx>
	CONST_FN FORCE_INLINE inline constexpr F fmin( F a, F b, F c, Tx... rest )
	{
		return fmin( fmin( a, b ), c, rest... );
	}
	template<Float F = fp_t, typename... Tx>
	CONST_FN FORCE_INLINE inline constexpr F fmax( F a, F b, F c, Tx... rest )
	{
		return fmax( fmax( a, b ), c, rest... );
	}
	
	// Common vector operations with accceleration.
	//
	template<typename V>
	CONST_FN FORCE_INLINE inline constexpr auto dot( const V& v1, const V& v2 )
	{
#if XSTD_MATH_USE_X86INTRIN && __XSTD_USE_DP
		if ( !std::is_constant_evaluated() ) {
			if constexpr ( Same<typename V::value_type, float> && V::Length == 4 ) {
				__m128 q1 = v1.to_xvec()._nat;
				__m128 q2 = v2.to_xvec()._nat;
				return ( _mm_dp_ps( q1, q2, 0b1111'0001 ) )[ 0 ];
			} else if constexpr ( Same<typename V::value_type, float> && V::Length == 3 ) {
				__m128 q1 = v1.to_xvec()._nat;
				__m128 q2 = v2.to_xvec()._nat;
				return ( _mm_dp_ps( q1, q2, 0b0111'0001 ) )[ 0 ];
			}
		}
#endif
		
		return ( v1 * v2 ).reduce_add();
	}
	template<typename V>
	PURE_FN FORCE_INLINE inline V normalize( V vec )
	{
		return vec * rcp( fsqrt( vec.length_sq() + fp_min<typename V::value_type> ) );
	}
	template<typename V, Float F>
	CONST_FN FORCE_INLINE inline constexpr V lerp( const V& v1, const V& v2, F s )
	{
		return v1 + ( v2 - v1 ) * s;
	}

#define VEC_MATRIX_CONTRACT																													 \
																																						 \
	static constexpr size_t Columns = 1;																									 \
	static constexpr size_t Rows = Length;																									 \
	using key_type = size_t;																													 \
	using unit_type = T;																															 \
	constexpr size_t cols() const { return Columns; }																					 \
	constexpr size_t rows() const { return Rows; }																						 \
	constexpr size_t llen() const { return cols() * rows(); }																		 \
	T* __restrict data() { return &x; }																										 \
	const T* __restrict data() const { return &x; }																						 \
	constexpr T& __restrict at( size_t row, size_t col ) { return operator[]( row* cols() + col ); }					 \
	constexpr const T& __restrict at( size_t row, size_t col ) const { return operator[]( row* cols() + col ); }	 \
	constexpr T& __restrict operator()( size_t row, size_t col ) { return at( row, col ); }								 \
	constexpr const T& __restrict operator()( size_t row, size_t col ) const { return at( row, col ); }				 \

	// Define vector types.
	//
	template<typename T>
	struct TRIVIAL_ABI vec2_t
	{
		using value_type = T;
		static constexpr size_t Length = 2;
		VEC_MATRIX_CONTRACT;

		T x = 0;
		T y = 0;

		template<typename R>
		FORCE_INLINE inline constexpr vec2_t<R> cast() const { return { R( x ), R( y ) }; }

		FORCE_INLINE inline static constexpr vec2_t<T> from_xvec( xvec<T, 4> o )
		{
			if ( std::is_constant_evaluated() )
				return vec2_t<T>{ o[ 0 ], o[ 1 ] };
			else
				return *( const vec2_t<T>* ) & o;
		}
		FORCE_INLINE inline constexpr xvec<T, 4> to_xvec() const noexcept
		{
			return { x, y, 0, 0 };
		}

		FORCE_INLINE inline constexpr T& operator[]( size_t n )
		{
			if ( std::is_constant_evaluated() )
			{
				switch ( n )
				{
					default:
					case 0:  return x;
					case 1:  return y;
				}
			}
			return ( &x )[ n & 1 ];
		}
		FORCE_INLINE inline constexpr T operator[]( size_t n ) const
		{
			if ( std::is_constant_evaluated() )
			{
				switch ( n )
				{
					default:
					case 0:  return x;
					case 1:  return y;
				}
			}
			return ( &x )[ n & 1 ]; 
		}
		FORCE_INLINE inline constexpr size_t count() const { return 2; }

		FORCE_INLINE inline static constexpr vec2_t<T> fill( T x ) { return { x, x }; }
		FORCE_INLINE inline auto tie() { return std::tie( x, y ); }
		FORCE_INLINE inline std::string to_string() const noexcept { return fmt::str( ( Integral<T> ? "{%d, %d}" : "{%f, %f}" ), x, y ); }

		FORCE_INLINE inline constexpr vec2_t operator+( T f ) const noexcept { return { x + f, y + f }; }
		FORCE_INLINE inline constexpr vec2_t operator-( T f ) const noexcept { return { x - f, y - f }; }
		FORCE_INLINE inline constexpr vec2_t operator*( T f ) const noexcept { return { x * f, y * f }; }
		FORCE_INLINE inline constexpr vec2_t operator/( T f ) const noexcept { return { x / f, y / f }; }
		FORCE_INLINE inline constexpr vec2_t operator+( const vec2_t& o ) const noexcept { return { x + o.x, y + o.y }; }
		FORCE_INLINE inline constexpr vec2_t operator-( const vec2_t& o ) const noexcept { return { x - o.x, y - o.y }; }
		FORCE_INLINE inline constexpr vec2_t operator*( const vec2_t& o ) const noexcept { return { x * o.x, y * o.y }; }
		FORCE_INLINE inline constexpr vec2_t operator/( const vec2_t& o ) const noexcept { return { x / o.x, y / o.y }; }
		FORCE_INLINE inline friend constexpr vec2_t operator+( T f, const vec2_t& o ) noexcept { return { f + o.x, f + o.y }; }
		FORCE_INLINE inline friend constexpr vec2_t operator-( T f, const vec2_t& o ) noexcept { return { f - o.x, f - o.y }; }
		FORCE_INLINE inline friend constexpr vec2_t operator*( T f, const vec2_t& o ) noexcept { return { f * o.x, f * o.y }; }
		FORCE_INLINE inline friend constexpr vec2_t operator/( T f, const vec2_t& o ) noexcept { return { f / o.x, f / o.y }; }
		FORCE_INLINE inline constexpr vec2_t operator-() const noexcept { return { -x, -y }; };
		FORCE_INLINE inline constexpr T reduce_add() const noexcept { return x + y; }
		FORCE_INLINE inline constexpr T reduce_mul() const noexcept { return x * y; }
		FORCE_INLINE inline constexpr T reduce_min() const noexcept { return x < y ? x : y; }
		FORCE_INLINE inline constexpr T reduce_max() const noexcept { return y < x ? x : y; }
		FORCE_INLINE inline constexpr vec2_t min_element( const vec2_t& o ) const noexcept { return { math::fmin( x, o.x ), math::fmin( y, o.y ) }; }
		FORCE_INLINE inline constexpr vec2_t max_element( const vec2_t& o ) const noexcept { return { math::fmax( x, o.x ), math::fmax( y, o.y ) }; }

		FORCE_INLINE inline constexpr vec2_t& operator+=( T f ) noexcept { *this = ( *this + f ); return *this; }
		FORCE_INLINE inline constexpr vec2_t& operator-=( T f ) noexcept { *this = ( *this - f ); return *this; }
		FORCE_INLINE inline constexpr vec2_t& operator*=( T f ) noexcept { *this = ( *this * f ); return *this; }
		FORCE_INLINE inline constexpr vec2_t& operator/=( T f ) noexcept { *this = ( *this / f ); return *this; }
		FORCE_INLINE inline constexpr vec2_t& operator+=( const vec2_t& o ) noexcept { *this = ( *this + o ); return *this; }
		FORCE_INLINE inline constexpr vec2_t& operator-=( const vec2_t& o ) noexcept { *this = ( *this - o ); return *this; }
		FORCE_INLINE inline constexpr vec2_t& operator*=( const vec2_t& o ) noexcept { *this = ( *this * o ); return *this; }
		FORCE_INLINE inline constexpr vec2_t& operator/=( const vec2_t& o ) noexcept { *this = ( *this / o ); return *this; }
		FORCE_INLINE inline constexpr vec2_t operator+() const noexcept { return *this; };
		FORCE_INLINE inline constexpr auto operator<=>( const vec2_t& ) const noexcept = default;
		FORCE_INLINE inline constexpr T length_sq() const noexcept { return dot( *this, *this ); }
		FORCE_INLINE inline fp_t length() const noexcept { return fsqrt( length_sq() ); }

		template<typename F>
		FORCE_INLINE inline constexpr operator vec2_t<F>() const noexcept { return this->template cast<F>(); }
	};
	template<typename T>
	struct TRIVIAL_ABI vec3_t
	{
		using value_type = T;
		static constexpr size_t Length = 3;
		VEC_MATRIX_CONTRACT;

		T x = 0;
		T y = 0;
		T z = 0;

		template<typename R>
		FORCE_INLINE inline constexpr vec3_t<R> cast() const 
		{
			if ( std::is_constant_evaluated() )
				return { R( x ), R( y ), R( z ) };
			return vec3_t<R>::from_xvec( to_xvec().template cast<R>() );
		}

		FORCE_INLINE inline static constexpr vec3_t<T> from( const vec2_t<T>& v, T z = 0 ) 
		{
			if ( std::is_constant_evaluated() )
				return { v.x, v.y, z };
			return from_xvec( v.to_xvec().template shuffle<0, 1, 4, -1>( xvec<T, 4>{ z } ) );
		}

		FORCE_INLINE inline static constexpr vec3_t<T> from_xvec( xvec<T, 4> o )
		{
			if ( std::is_constant_evaluated() )
				return vec3_t<T>{ o[ 0 ], o[ 1 ], o[ 2 ] };
			else
				return *( const vec3_t<T>* ) &o;
		}
		FORCE_INLINE inline constexpr xvec<T, 4> to_xvec( T pad = 0 ) const noexcept 
		{ 
			return { x, y, z, pad };
		}

		FORCE_INLINE inline constexpr T& operator[]( size_t n )
		{
			if ( std::is_constant_evaluated() )
			{
				switch ( n )
				{
					default:
					case 0:  return x;
					case 1:  return y;
					case 2:  return z;
				}
			}
			return ( &x )[ n % 3 ];
		}
		FORCE_INLINE inline constexpr const T& operator[]( size_t n ) const
		{
			if ( std::is_constant_evaluated() )
			{
				switch ( n )
				{
					default:
					case 0:  return x;
					case 1:  return y;
					case 2:  return z;
				}
			}
			return ( &x )[ n % 3 ];
		}
		FORCE_INLINE inline constexpr size_t count() const { return 3; }

		FORCE_INLINE inline static constexpr vec3_t<T> fill( T x ) { return { x, x, x }; }
		FORCE_INLINE inline constexpr vec2_t<T> xy() const noexcept { return { x, y }; }
		FORCE_INLINE inline auto tie() { return std::tie( x, y, z ); }
		FORCE_INLINE inline std::string to_string() const noexcept { return fmt::str( ( Integral<T> ? "{%d, %d, %d}" : "{%f, %f, %f}" ), x, y, z ); }
		
		FORCE_INLINE inline constexpr vec3_t operator+( T f ) const noexcept { return from_xvec( to_xvec() + f ); }
		FORCE_INLINE inline constexpr vec3_t operator-( T f ) const noexcept { return from_xvec( to_xvec() - f ); }
		FORCE_INLINE inline constexpr vec3_t operator*( T f ) const noexcept { return from_xvec( to_xvec() * f ); }
		FORCE_INLINE inline constexpr vec3_t operator/( T f ) const noexcept { return from_xvec( to_xvec() / f ); }
		FORCE_INLINE inline constexpr vec3_t operator+( const vec3_t& o ) const noexcept { return from_xvec( to_xvec() + o.to_xvec() ); }
		FORCE_INLINE inline constexpr vec3_t operator-( const vec3_t& o ) const noexcept { return from_xvec( to_xvec() - o.to_xvec() ); }
		FORCE_INLINE inline constexpr vec3_t operator*( const vec3_t& o ) const noexcept { return from_xvec( to_xvec() * o.to_xvec() ); }
		FORCE_INLINE inline constexpr vec3_t operator/( const vec3_t& o ) const noexcept { return from_xvec( to_xvec() / o.to_xvec() ); }
		FORCE_INLINE inline friend constexpr vec3_t operator+( T f, const vec3_t& o ) noexcept { return from_xvec( o.to_xvec() + f ); }
		FORCE_INLINE inline friend constexpr vec3_t operator-( T f, const vec3_t& o ) noexcept { return from_xvec( -o.to_xvec() + f ); }
		FORCE_INLINE inline friend constexpr vec3_t operator*( T f, const vec3_t& o ) noexcept { return from_xvec( o.to_xvec() * f ); }
		FORCE_INLINE inline friend constexpr vec3_t operator/( T f, const vec3_t& o ) noexcept { return from_xvec( fill( f ).to_xvec() / o.to_xvec() ); }
		FORCE_INLINE inline constexpr vec3_t operator-() const noexcept { return from_xvec( -to_xvec() ); }
		FORCE_INLINE inline constexpr T reduce_add() const noexcept { return vec::reduce_add( to_xvec( 0 ) ); }
		FORCE_INLINE inline constexpr T reduce_mul() const noexcept { return vec::reduce_mul( to_xvec( 1 ) ); }
		FORCE_INLINE inline constexpr T reduce_min() const noexcept { return vec::reduce_min( to_xvec( z ) ); }
		FORCE_INLINE inline constexpr T reduce_max() const noexcept { return vec::reduce_max( to_xvec( z ) ); }
		FORCE_INLINE inline constexpr vec3_t min_element( const vec3_t& o ) const noexcept { return from_xvec( (vec::min)( to_xvec(), o.to_xvec() ) ); }
		FORCE_INLINE inline constexpr vec3_t max_element( const vec3_t& o ) const noexcept { return from_xvec( (vec::max)( to_xvec(), o.to_xvec() ) ); }

		FORCE_INLINE inline constexpr vec3_t& operator+=( T f ) noexcept { *this = ( *this + f ); return *this; }
		FORCE_INLINE inline constexpr vec3_t& operator-=( T f ) noexcept { *this = ( *this - f ); return *this; }
		FORCE_INLINE inline constexpr vec3_t& operator*=( T f ) noexcept { *this = ( *this * f ); return *this; }
		FORCE_INLINE inline constexpr vec3_t& operator/=( T f ) noexcept { *this = ( *this / f ); return *this; }
		FORCE_INLINE inline constexpr vec3_t& operator+=( const vec3_t& o ) noexcept { *this = ( *this + o ); return *this; }
		FORCE_INLINE inline constexpr vec3_t& operator-=( const vec3_t& o ) noexcept { *this = ( *this - o ); return *this; }
		FORCE_INLINE inline constexpr vec3_t& operator*=( const vec3_t& o ) noexcept { *this = ( *this * o ); return *this; }
		FORCE_INLINE inline constexpr vec3_t& operator/=( const vec3_t& o ) noexcept { *this = ( *this / o ); return *this; }
		FORCE_INLINE inline constexpr vec3_t operator+() const noexcept { return *this; };
		FORCE_INLINE inline constexpr auto operator<=>( const vec3_t& ) const noexcept = default;
		FORCE_INLINE inline constexpr T length_sq() const noexcept { return dot( *this, *this ); }
		FORCE_INLINE inline fp_t length() const noexcept { return fsqrt( length_sq() ); }

		template<typename F>
		FORCE_INLINE inline constexpr operator vec3_t<F>() const noexcept { return this->template cast<F>(); }
	};
	template<typename T>
	struct TRIVIAL_ABI vec4_t
	{
		using value_type = T;
		static constexpr size_t Length = 4;
		VEC_MATRIX_CONTRACT;

		T x = 0;
		T y = 0;
		T z = 0;
		T w = 0;

		template<typename R>
		FORCE_INLINE inline constexpr vec4_t<R> cast() const 
		{
			if ( std::is_constant_evaluated() )
				return { R( x ), R( y ), R( z ), R( w ) }; 
			return vec4_t<R>::from_xvec( to_xvec().template cast<R>() );
		}

		FORCE_INLINE inline static constexpr vec4_t<T> from( const vec2_t<T>& v, T z = 0, T w = 1 ) 
		{
			if ( std::is_constant_evaluated() )
				return { v.x, v.y, z, w };
			return from_xvec( v.to_xvec().template shuffle<0, 1, 4, 5>( xvec<T, 4>{ z, w } ) );
		}
		FORCE_INLINE inline static constexpr vec4_t<T> from( const vec3_t<T>& v, T w = 1 ) 
		{
			if ( std::is_constant_evaluated() )
				return { v.x, v.y, v.z, w };
			return from_xvec( v.to_xvec().template shuffle<0, 1, 2, 4>( xvec<T, 4>{ w } ) );
		}
		FORCE_INLINE inline static constexpr vec4_t<T> from_xvec( xvec<T, 4> o )
		{ 
			if ( std::is_constant_evaluated() )
				return vec4_t<T>{ o[ 0 ], o[ 1 ], o[ 2 ], o[ 3 ] };
			vec4_t r;
			xstd::store_misaligned( &r, o );
			return r;
		}
		FORCE_INLINE inline constexpr xvec<T, 4> to_xvec() const noexcept 
		{
			if ( std::is_constant_evaluated() )
				return { x, y, z, w };
			return xstd::load_misaligned<xvec<T, 4>>( this );
		}

		FORCE_INLINE inline constexpr T& operator[]( size_t n )
		{
			if ( std::is_constant_evaluated() )
			{
				switch ( n )
				{
					default:
					case 0:  return x;
					case 1:  return y;
					case 2:  return z;
					case 3:  return w;
				}
			}
			return ( &x )[ n & 3 ];
		}
		FORCE_INLINE inline constexpr const T& operator[]( size_t n ) const
		{
			if ( std::is_constant_evaluated() )
			{
				switch ( n )
				{
					default:
					case 0:  return x;
					case 1:  return y;
					case 2:  return z;
					case 3:  return w;
				}
			}
			return ( &x )[ n & 3 ];
		}
		FORCE_INLINE inline constexpr size_t count() const { return 4; }

		FORCE_INLINE inline static constexpr vec4_t<T> fill( T x ) { return { x, x, x, x }; }
		FORCE_INLINE inline constexpr vec2_t<T> xy() const noexcept { return { x, y }; }
		FORCE_INLINE inline constexpr vec3_t<T> xyz() const noexcept 
		{ 
			if ( std::is_constant_evaluated() )
				return { x, y, z };
			return vec3_t<T>::from_xvec( to_xvec() );
		}
		FORCE_INLINE inline auto tie() { return std::tie( x, y, z, w ); }
		FORCE_INLINE inline std::string to_string() const noexcept { return fmt::str( ( Integral<T> ? "{%d, %d, %d, %f}" : "{%f, %f, %f, %f}" ), x, y, z, w ); }

		FORCE_INLINE inline constexpr vec4_t operator+( T f ) const noexcept { return from_xvec( to_xvec() + f ); }
		FORCE_INLINE inline constexpr vec4_t operator-( T f ) const noexcept { return from_xvec( to_xvec() - f ); }
		FORCE_INLINE inline constexpr vec4_t operator*( T f ) const noexcept { return from_xvec( to_xvec() * f ); }
		FORCE_INLINE inline constexpr vec4_t operator/( T f ) const noexcept { return from_xvec( to_xvec() / f ); }
		FORCE_INLINE inline constexpr vec4_t operator+( const vec4_t& o ) const noexcept { return from_xvec( to_xvec() + o.to_xvec() ); }
		FORCE_INLINE inline constexpr vec4_t operator-( const vec4_t& o ) const noexcept { return from_xvec( to_xvec() - o.to_xvec() ); }
		FORCE_INLINE inline constexpr vec4_t operator*( const vec4_t& o ) const noexcept { return from_xvec( to_xvec() * o.to_xvec() ); }
		FORCE_INLINE inline constexpr vec4_t operator/( const vec4_t& o ) const noexcept { return from_xvec( to_xvec() / o.to_xvec() ); }
		FORCE_INLINE inline friend constexpr vec4_t operator+( T f, const vec4_t& o ) noexcept { return from_xvec( o.to_xvec() + f ); }
		FORCE_INLINE inline friend constexpr vec4_t operator-( T f, const vec4_t& o ) noexcept { return from_xvec( -o.to_xvec() + f ); }
		FORCE_INLINE inline friend constexpr vec4_t operator*( T f, const vec4_t& o ) noexcept { return from_xvec( o.to_xvec() * f ); }
		FORCE_INLINE inline friend constexpr vec4_t operator/( T f, const vec4_t& o ) noexcept { return from_xvec( fill( f ).to_xvec() / o.to_xvec() ); }
		FORCE_INLINE inline constexpr vec4_t operator-() const noexcept { return from_xvec( -to_xvec() ); }
		FORCE_INLINE inline constexpr T reduce_add() const noexcept { return vec::reduce_add( to_xvec() ); }
		FORCE_INLINE inline constexpr T reduce_mul() const noexcept { return vec::reduce_mul( to_xvec() ); }
		FORCE_INLINE inline constexpr T reduce_min() const noexcept { return vec::reduce_min( to_xvec() ); }
		FORCE_INLINE inline constexpr T reduce_max() const noexcept { return vec::reduce_max( to_xvec() ); }
		FORCE_INLINE inline constexpr vec4_t min_element( const vec4_t& o ) const noexcept { return from_xvec( (vec::min)( to_xvec(), o.to_xvec() ) ); }
		FORCE_INLINE inline constexpr vec4_t max_element( const vec4_t& o ) const noexcept { return from_xvec( (vec::max)( to_xvec(), o.to_xvec() ) ); }

		FORCE_INLINE inline constexpr vec4_t& operator+=( T f ) noexcept { *this = ( *this + f ); return *this; }
		FORCE_INLINE inline constexpr vec4_t& operator-=( T f ) noexcept { *this = ( *this - f ); return *this; }
		FORCE_INLINE inline constexpr vec4_t& operator*=( T f ) noexcept { *this = ( *this * f ); return *this; }
		FORCE_INLINE inline constexpr vec4_t& operator/=( T f ) noexcept { *this = ( *this / f ); return *this; }
		FORCE_INLINE inline constexpr vec4_t& operator+=( const vec4_t& o ) noexcept { *this = ( *this + o ); return *this; }
		FORCE_INLINE inline constexpr vec4_t& operator-=( const vec4_t& o ) noexcept { *this = ( *this - o ); return *this; }
		FORCE_INLINE inline constexpr vec4_t& operator*=( const vec4_t& o ) noexcept { *this = ( *this * o ); return *this; }
		FORCE_INLINE inline constexpr vec4_t& operator/=( const vec4_t& o ) noexcept { *this = ( *this / o ); return *this; }
		FORCE_INLINE inline constexpr vec4_t operator+() const noexcept { return *this; };
		FORCE_INLINE inline constexpr auto operator<=>( const vec4_t& ) const noexcept = default;
		FORCE_INLINE inline constexpr T length_sq() const noexcept { return dot( *this, *this ); }
		FORCE_INLINE inline fp_t length() const noexcept { return fsqrt( length_sq() ); }

		template<int A, int B, int C, int D>
		FORCE_INLINE inline constexpr vec4_t shuffle( const vec4_t& o ) const noexcept { return from_xvec( to_xvec().template shuffle<A, B, C, D>( o.to_xvec() ) ); }
		template<int A, int B, int C, int D>
		FORCE_INLINE inline constexpr vec4_t shuffle( const vec3_t<T>& o ) const noexcept { return from_xvec( to_xvec().template shuffle<A, B, C, D>( o.to_xvec() ) ); }
		template<int A, int B, int C, int D>
		FORCE_INLINE inline constexpr vec4_t shuffle( const vec2_t<T>& o ) const noexcept { return from_xvec( to_xvec().template shuffle<A, B, C, D>( o.to_xvec() ) ); }

		template<typename F>
		FORCE_INLINE inline constexpr operator vec4_t<F>() const noexcept { return this->template cast<F>(); }
	};

	using vec2 =       vec2_t<fp_t>;
	using vec3 =       vec3_t<fp_t>;
	using vec4 =       vec4_t<fp_t>;
	using vec2f =      vec2_t<float>;
	using vec3f =      vec3_t<float>;
	using vec4f =      vec4_t<float>;
	using vec2d =      vec2_t<double>;
	using vec3d =      vec3_t<double>;
	using vec4d =      vec4_t<double>;
	using ivec2 =      vec2_t<int32_t>;
	using ivec3 =      vec3_t<int32_t>;
	using ivec4 =      vec4_t<int32_t>;
	using ivec2q =     vec2_t<int64_t>;
	using ivec3q =     vec3_t<int64_t>;
	using ivec4q =     vec4_t<int64_t>;
	template<Float F = fp_t>
	using quaternion_t = vec4_t<F>;
	using quaternion = quaternion_t<>;

	// Implement certain float funcs for vectors as well.
	//
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr vec4_t<F> vec_abs( const vec4_t<F>& vec )
	{
#if __has_vector_builtin(__builtin_elementwise_abs)
		if ( !std::is_constant_evaluated() )
			return vec4_t<F>::from_xvec( xvec<F, 4>{ std::in_place, __builtin_elementwise_abs( vec.to_xvec()._nat ) } );
#endif
		return { fabs( vec.x ), fabs( vec.y ), fabs( vec.z ), fabs( vec.w ) };
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr vec4_t<F> vec_ceil( const vec4_t<F>& vec )
	{
#if __has_vector_builtin(__builtin_elementwise_ceil)
		if ( !std::is_constant_evaluated() )
			return vec4_t<F>::from_xvec( xvec<F, 4>{ std::in_place, __builtin_elementwise_ceil( vec.to_xvec()._nat ) } );
#endif
		return { fceil( vec.x ), fceil( vec.y ), fceil( vec.z ), fceil( vec.w ) };
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr vec4_t<F> vec_floor( const vec4_t<F>& vec )
	{
#if __has_vector_builtin(__builtin_elementwise_floor)
		if ( !std::is_constant_evaluated() )
			return vec4_t<F>::from_xvec( xvec<F, 4>{ std::in_place, __builtin_elementwise_floor( vec.to_xvec()._nat ) } );
#endif
		return { ffloor( vec.x ), ffloor( vec.y ), ffloor( vec.z ), ffloor( vec.w ) };
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr vec4_t<F> vec_round( const vec4_t<F>& vec )
	{
#if __has_vector_builtin(__builtin_elementwise_roundeven)
		if ( !std::is_constant_evaluated() )
			return vec4_t<F>::from_xvec( xvec<F, 4>{ std::in_place, __builtin_elementwise_roundeven( vec.to_xvec()._nat ) } );
#endif
		return { fround( vec.x ), fround( vec.y ), fround( vec.z ), fround( vec.w ) };
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr vec4_t<F> vec_trunc( const vec4_t<F>& vec )
	{
#if __has_vector_builtin(__builtin_elementwise_trunc)
		if ( !std::is_constant_evaluated() )
			return vec4_t<F>::from_xvec( xvec<F, 4>{ std::in_place, __builtin_elementwise_trunc( vec.to_xvec()._nat ) } );
#endif
		return { ftrunc( vec.x ), ftrunc( vec.y ), ftrunc( vec.z ), ftrunc( vec.w ) };
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr vec4_t<F> vec_oddsgn( vec4_t<F> x )
	{
		x *= F(0.5);
		return x - vec_ceil( x );
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr vec4_t<F> vec_xorsgn( const vec4_t<F>& a, const vec4_t<F>& b )
	{
		using U = convert_uint_t<F>;
		if ( !std::is_constant_evaluated() )
			return vec4_t<F>::from_xvec( ( a.to_xvec().template reinterpret<U>() ^ b.to_xvec().template reinterpret<U>() ).template reinterpret<F>() );
		return { fxorsgn( a.x, b.x ), fxorsgn( a.y, b.y ), fxorsgn( a.z, b.z ), fxorsgn( a.w, b.w ) };
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr vec4_t<F> vec_copysign( const vec4_t<F>& a, const vec4_t<F>& b )
	{
		if ( !std::is_constant_evaluated() ) {
			if constexpr ( std::is_same_v<F, float> ) {
				auto va = a.to_xvec().template reinterpret<uint32_t>() & 0x7FFFFFFF;
				auto vb = b.to_xvec().template reinterpret<uint32_t>() & 0x80000000;
				return vec4_t<F>::from_xvec( ( va | vb ).template reinterpret<float>() );
			} else if constexpr ( std::is_same_v<F, double> ) {
				auto va = a.to_xvec().template reinterpret<uint64_t>() & 0x7FFFFFFFFFFFFFFF;
				auto vb = b.to_xvec().template reinterpret<uint64_t>() & 0x8000000000000000;
				return vec4_t<F>::from_xvec( ( va | vb ).template reinterpret<double>() );
			}
		}
		return { fcopysign( a.x, b.x ), fcopysign( a.y, b.y ), fcopysign( a.z, b.z ), fcopysign( a.w, b.w ) };
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr vec4_t<F> vec_ifsgn( const vec4_t<F>& a, const vec4_t<F>& b )
	{
		if ( !std::is_constant_evaluated() )
		{
			using U = convert_uint_t<F>;
			using I = convert_int_t<F>;
			auto va = a.to_xvec().template reinterpret<U>();
			auto vb = ( b.to_xvec() < 0 ).template cast<I>();
			return vec4_t<F>::from_xvec( ( va & vb ).template reinterpret<F>() );
		}
		return { fifsgn( a.x, b.x ), fifsgn( a.y, b.y ), fifsgn( a.z, b.z ), fifsgn( a.w, b.w ) };
	}
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline vec4_t<F> vec_sqrt( const vec4_t<F>& vec ) { return { fsqrt( vec.x ), fsqrt( vec.y ), fsqrt( vec.z ), fsqrt( vec.w ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline vec4_t<F> vec_pow( const vec4_t<F>& a, F x ) { return { fpow( a.x, x ), fpow( a.y, x ), fpow( a.z, x ), fpow( a.w, x ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline vec4_t<F> vec_pow( const vec4_t<F>& a, const vec4_t<F>& b ) { return { fpow( a.x, b.x ), fpow( a.y, b.y ), fpow( a.z, b.z ), fpow( a.w, b.w ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec4_t<F> vec_mod( const vec4_t<F>& x, F y ) { return x - vec_trunc( x * rcp( y ) ) * y; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec4_t<F> vec_mod( const vec4_t<F>& x, const vec4_t<F>& y ) { return x - vec_trunc( x * rcp( y ) ) * y; }

	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec3_t<F> vec_abs( const vec3_t<F>& vec ) { return vec_abs( vec4_t<F>::from( vec, 0 ) ).xyz(); }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec3_t<F> vec_ceil( const vec3_t<F>& vec ) { return vec_ceil( vec4_t<F>::from( vec, 0 ) ).xyz(); }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec3_t<F> vec_floor( const vec3_t<F>& vec ) { return vec_floor( vec4_t<F>::from( vec, 0 ) ).xyz(); }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec3_t<F> vec_round( const vec3_t<F>& vec ) { return vec_round( vec4_t<F>::from( vec, 0 ) ).xyz(); }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec3_t<F> vec_trunc( const vec3_t<F>& vec ) { return vec_trunc( vec4_t<F>::from( vec, 0 ) ).xyz(); }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline vec3_t<F> constexpr vec_oddsgn( const vec3_t<F>& vec ) { return vec_oddsgn( vec4_t<F>::from( vec, 0 ) ).xyz(); }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline vec3_t<F> constexpr vec_xorsgn( const vec3_t<F>& a, const vec3_t<F>& b ) { return vec_xorsgn( vec4_t<F>::from( a, 0 ), vec4_t<F>::from( b, 0 ) ).xyz(); }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline vec3_t<F> constexpr vec_copysign( const vec3_t<F>& a, const vec3_t<F>& b ) { return vec_copysign( vec4_t<F>::from( a, 0 ), vec4_t<F>::from( b, 0 ) ).xyz(); }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline vec3_t<F> constexpr vec_ifsgn( const vec3_t<F>& a, const vec3_t<F>& b ) { return vec_ifsgn( vec4_t<F>::from( a, 0 ), vec4_t<F>::from( b, 0 ) ).xyz(); }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline vec3_t<F> vec_sqrt( const vec3_t<F>& vec ) { return vec_sqrt( vec4_t<F>::from( vec, 0 ) ).xyz(); }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline vec3_t<F> vec_pow( const vec3_t<F>& a, F x ) { return { fpow( a.x, x ), fpow( a.y, x ), fpow( a.z, x ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline vec3_t<F> vec_pow( const vec3_t<F>& a, const vec3_t<F>& b ) { return { fpow( a.x, b.x ), fpow( a.y, b.y ), fpow( a.z, b.z ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec3_t<F> vec_mod( const vec3_t<F>& x, F y ) { return vec_mod( vec4_t<F>::from( x, 0 ), y ).xyz(); }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec3_t<F> vec_mod( const vec3_t<F>& x, const vec3_t<F>& y ) { return vec_mod( vec4_t<F>::from( x, 0 ), vec4_t<F>::from( y, 0 ) ).xyz(); }

	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec2_t<F> vec_abs( const vec2_t<F>& vec ) { return { fabs( vec.x ), fabs( vec.y ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec2_t<F> vec_ceil( const vec2_t<F>& vec ) { return { fceil( vec.x ), fceil( vec.y ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec2_t<F> vec_floor( const vec2_t<F>& vec ) { return { ffloor( vec.x ), ffloor( vec.y ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec2_t<F> vec_round( const vec2_t<F>& vec ) { return { fround( vec.x ), fround( vec.y ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec2_t<F> vec_trunc( const vec2_t<F>& vec ) { return { ftrunc( vec.x ), ftrunc( vec.y ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec2_t<F> vec_oddsgn( const vec2_t<F>& vec ) { return { foddsgn( vec.x ), foddsgn( vec.y ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec2_t<F> vec_xorsgn( const vec2_t<F>& a, const vec2_t<F>& b ) { return { fxorsgn( a.x, b.x ), fxorsgn( a.y, b.y ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec2_t<F> vec_copysign( const vec2_t<F>& a, const vec2_t<F>& b ) { return { fcopysign( a.x, b.x ), fcopysign( a.y, b.y ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec2_t<F> vec_ifsgn( const vec2_t<F>& a, const vec2_t<F>& b ) { return { fifsgn( a.x, b.x ), fifsgn( a.y, b.y ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline vec2_t<F> vec_sqrt( const vec2_t<F>& vec ) { return { fsqrt( vec.x ), fsqrt( vec.y ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline vec2_t<F> vec_pow( const vec2_t<F>& a, F x ) { return { fpow( a.x, x ), fpow( a.y, x ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline vec2_t<F> vec_pow( const vec2_t<F>& a, const vec2_t<F>& b ) { return { fpow( a.x, b.x ), fpow( a.y, b.y ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec2_t<F> vec_mod( const vec2_t<F>& x, F y ) { return { fmod( x.x, y, x.y, y ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec2_t<F> vec_mod( const vec2_t<F>& x, const vec2_t<F>& y ) { return { fmod( x.x, y.x, x.y, y.y ) }; }


	template<typename V>
	CONST_FN FORCE_INLINE inline constexpr V vec_max( const V& v1, const V& v2 )
	{
		return v1.max_element( v2 );
	}
	template<typename V>
	CONST_FN FORCE_INLINE inline constexpr V vec_min( const V& v1, const V& v2 )
	{
		return v1.min_element( v2 );
	}
	template<typename V, typename... Tx>
	CONST_FN FORCE_INLINE inline constexpr V vec_max( const V& v1, const V& v2, const V& v3, Tx&&... rest )
	{
		return vec_max( vec_max( v1, v2 ), v3, std::forward<Tx>( rest )... );
	}
	template<typename V, typename... Tx>
	CONST_FN FORCE_INLINE inline constexpr V vec_min( const V& v1, const V& v2, const V& v3, Tx&&... rest )
	{
		return vec_min( vec_min( v1, v2 ), v3, std::forward<Tx>( rest )... );
	}
	template<typename V>
	CONST_FN FORCE_INLINE inline constexpr V vec_clamp( const V& v, const V& vmin, const V& vmax )
	{
		return vec_min( vec_max( v, vmin ), vmax );
	}
	template<typename V>
	CONST_FN FORCE_INLINE inline constexpr V vec_clamp( const V& v, fp_t vmin, fp_t vmax )
	{
		return vec_clamp( v, V::fill( vmin ), V::fill( vmax ) );
	}

	// Extended vector helpers.
	//
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr vec3_t<F> cross( const vec3_t<F>& v1, const vec3_t<F>& v2 )
	{
		auto x1 = v1.to_xvec();
		auto x2 = v2.to_xvec();
		auto res = ( x1 * x2.template shuffle<1, 2, 0, 3>( x2 ) ) -
					  ( x2 * x1.template shuffle<1, 2, 0, 3>( x1 ) );
		return vec3_t<F>::from_xvec( res.template shuffle<1, 2, 0, 3>( res ) );
	}
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr vec4_t<F> cross( const vec4_t<F>& v1, const vec4_t<F>& v2, const vec4_t<F>& v3 )
	{
		return {
		     v1.y * ( v2.z * v3.w - v3.z * v2.w ) - v1.z * ( v2.y * v3.w - v3.y * v2.w ) + v1.w * ( v2.y * v3.z - v2.z * v3.y ),
		  -( v1.x * ( v2.z * v3.w - v3.z * v2.w ) - v1.z * ( v2.x * v3.w - v3.x * v2.w ) + v1.w * ( v2.x * v3.z - v3.x * v2.z ) ),
		     v1.x * ( v2.y * v3.w - v3.y * v2.w ) - v1.y * ( v2.x * v3.w - v3.x * v2.w ) + v1.w * ( v2.x * v3.y - v3.x * v2.y ),
		  -( v1.x * ( v2.y * v3.z - v3.y * v2.z ) - v1.y * ( v2.x * v3.z - v3.x * v2.z ) + v1.z * ( v2.x * v3.y - v3.x * v2.y ) )
		};
	}
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr quaternion_t<F> inverse( const quaternion_t<F>& q )
	{
		return q * vec4{ -1, -1, -1, +1 };
	}
	template<typename V>
	FORCE_INLINE inline constexpr V saturate( const V& v )
	{
		return vec_min( vec_max( v, V::fill( 0 ) ), V::fill( 1 ) );
	}

	// Define matrix type.
	//
	template<Float F = fp_t>
	struct TRIVIAL_ABI mat4x4_t
	{
		using key_type =   size_t;
		using unit_type =  F;
		using value_type = vec4_t<F>;
		
		using vec = vec4_t<F>;
		vec m[ 4 ] = { vec{} };

		// Implement matrix contract.
		//
		static constexpr size_t Columns = 4;
		static constexpr size_t Rows =    4;
		constexpr size_t cols() const { return Columns; }
		constexpr size_t rows() const { return Rows; }
		constexpr size_t llen() const { return cols() * rows(); }
		F* __restrict data() { return ( F* ) &m; }
		const F* __restrict data() const { return ( F* ) &m; }
		constexpr F& __restrict at( size_t row, size_t col ) { return operator()( row * cols() + col ); }
		constexpr const F& __restrict at( size_t row, size_t col ) const { return operator()( row * cols() + col ); }

		// Default construction.
		//
		constexpr mat4x4_t() = default;

		// Construction by 4 vectors.
		//
		FORCE_INLINE inline constexpr mat4x4_t( vec a, vec b, vec c, vec d ) : m{ a, b, c, d } {}
		
		// Construction by all values.
		//
		FORCE_INLINE inline constexpr mat4x4_t( F _00, F _01, F _02, F _03,
									F _10, F _11, F _12, F _13,
									F _20, F _21, F _22, F _23,
									F _30, F _31, F _32, F _33 ) : m{ 
			vec{ _00, _01, _02, _03 },
			vec{ _10, _11, _12, _13 },
			vec{ _20, _21, _22, _23 },
			vec{ _30, _31, _32, _33 }
		} {}

		// Default copy.
		//
		constexpr mat4x4_t( const mat4x4_t& ) = default;
		constexpr mat4x4_t& operator=( const mat4x4_t& ) = default;

		// Identity matrix.
		//
		inline static constexpr std::array<vec, 4> identity_value = {
			vec{ 1, 0, 0, 0 },
			vec{ 0, 1, 0, 0 },
			vec{ 0, 0, 1, 0 },
			vec{ 0, 0, 0, 1 }
		};
		FORCE_INLINE inline static constexpr mat4x4_t identity() noexcept { return { identity_value[ 0 ], identity_value[ 1 ], identity_value[ 2 ], identity_value[ 3 ] }; }

		// Matrix mulitplication.
		//
		FORCE_INLINE inline constexpr mat4x4_t operator*( const mat4x4_t& other ) const noexcept
		{

#if XSTD_MATH_USE_X86INTRIN
			if constexpr ( std::is_same_v<F, float> ) {
				if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) ) {
					__m256 v01, v23, q01, q23;
					impl::load_matrix_f32( m, v01, v23 );
					impl::load_matrix_f32( other.m, q01, q23 );
	#if __XSTD_USE_DP || !XSTD_VECTOR_EXT
					impl::tranpose_inplace_f32( q01, q23 );
					__m256 vxx = _mm256_permute2f128_ps( q01, q01, 0b0000'0000 );
					__m256 vyy = _mm256_permute2f128_ps( q01, q01, 0b0001'0001 );
					__m256 vzz = _mm256_permute2f128_ps( q23, q23, 0b0000'0000 );
					__m256 vww = _mm256_permute2f128_ps( q23, q23, 0b0001'0001 );

					__m256 r01 = _mm256_dp_ps( v01, vxx, 0b11110001 );
					__m256 r23 = _mm256_dp_ps( v23, vxx, 0b11110001 );
					r01 = _mm256_or_ps( r01, _mm256_dp_ps( v01, vyy, 0b11110010 ) );
					r23 = _mm256_or_ps( r23, _mm256_dp_ps( v23, vyy, 0b11110010 ) );
					r01 = _mm256_or_ps( r01, _mm256_dp_ps( v01, vzz, 0b11110100 ) );
					r23 = _mm256_or_ps( r23, _mm256_dp_ps( v23, vzz, 0b11110100 ) );
					r01 = _mm256_or_ps( r01, _mm256_dp_ps( v01, vww, 0b11111000 ) );
					r23 = _mm256_or_ps( r23, _mm256_dp_ps( v23, vww, 0b11111000 ) );
					return impl::store_matrix<mat4x4>( r01, r23 );
	#else
		#define L0 0, 1, 0, 1,  4, 5, 4, 5 
		#define L1 1, 0, 1, 0,  5, 4, 5, 4 
		#define L2 2, 3, 2, 3,  6, 7, 6, 7
		#define L3 3, 2, 3, 2,  7, 6, 7, 6
		#define K0 0, 5, 2, 7,  0, 5, 2, 7
		#define K1 4, 1, 6, 3,  4, 1, 6, 3
		#define K2 0, 5, 2, 7,  0, 5, 2, 7
		#define K3 4, 1, 6, 3,  4, 1, 6, 3
					__m256 t0 = __builtin_shufflevector( v01, v01, L0 );
					__m256 t1 = __builtin_shufflevector( v01, v01, L1 );
					t0 *=       __builtin_shufflevector( q01, q01, K0 );
					t0 += t1 *  __builtin_shufflevector( q01, q01, K1 );
					__m256 t2 = __builtin_shufflevector( v01, v01, L2 );
					__m256 t3 = __builtin_shufflevector( v01, v01, L3 );
					t0 += t2 *  __builtin_shufflevector( q23, q23, K2 );
					t0 += t3 *  __builtin_shufflevector( q23, q23, K3 );
					__m256 t4 = __builtin_shufflevector( v23, v23, L0 );
					__m256 t5 = __builtin_shufflevector( v23, v23, L1 );
					t4 *=       __builtin_shufflevector( q01, q01, K0 );
					t4 += t5 *  __builtin_shufflevector( q01, q01, K1 );
					__m256 t6 = __builtin_shufflevector( v23, v23, L2 );
					__m256 t7 = __builtin_shufflevector( v23, v23, L3 );
					t4 += t6 *  __builtin_shufflevector( q23, q23, K2 );
					t4 += t7 *  __builtin_shufflevector( q23, q23, K3 );
					return impl::store_matrix_f32<mat4x4_t<float>>( t0, t4 );
		#undef K0
		#undef K1
		#undef K2
		#undef K3
		#undef L0
		#undef L1
		#undef L2
		#undef L3
	#endif
				}
			}
#endif
			return { other * m[ 0 ], other * m[ 1 ], other * m[ 2 ], other * m[ 3 ] };
		}
		FORCE_INLINE inline constexpr mat4x4_t& operator*=( const mat4x4_t& other ) noexcept { *this = ( *this * other ); return *this; }

		// Vector transformation.
		//
		FORCE_INLINE inline constexpr vec operator*( const vec& v ) const noexcept
		{
#if XSTD_MATH_USE_X86INTRIN
			if constexpr ( std::is_same_v<F, float> ) {
				if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) ) {
					auto xyzw = v.to_xvec();
					__m256 vxy = xyzw.template shuffle<0, 0, 0, 0, 1, 1, 1, 1>( xyzw )._nat;
					__m256 vzw = xyzw.template shuffle<2, 2, 2, 2, 3, 3, 3, 3>( xyzw )._nat;

					__m256 vrlh = _mm256_mul_ps( vxy, _mm256_loadu_ps( ( const float* ) &m[ 0 ] ) );
					vrlh = _mm256_fmadd_ps( vzw, _mm256_loadu_ps( ( const float* ) &m[ 2 ] ), vrlh );

					auto vrl = _mm256_extractf128_ps( vrlh, 0 );
					auto vrh = _mm256_extractf128_ps( vrlh, 1 );
					return vec4f::from_xvec( xvec<float, 4>{ std::in_place_t{}, _mm_add_ps( vrl, vrh ) } );
				}
			}
#endif
			vec r = m[ 0 ] * v.x;
			r += m[ 1 ] * v.y;
			r += m[ 2 ] * v.z;
			r += m[ 3 ] * v.w;
			return r;
		}
		FORCE_INLINE inline constexpr vec apply_no_w( const vec& v ) const noexcept
		{
#if XSTD_MATH_USE_X86INTRIN
			if constexpr ( std::is_same_v<F, float> ) {
				if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) ) {
					xvec<float, 4> one{ 1.0f };
					auto xyzw = v.to_xvec();
					__m256 vxy = xyzw.template shuffle<0, 0, 0, 0, 1, 1, 1, 1>( xyzw )._nat;
					__m256 vzw = xyzw.template shuffle<2, 2, 2, 2, 4, 4, 4, 4>( one )._nat;

					__m256 vrlh = _mm256_mul_ps( vxy, _mm256_loadu_ps( ( const float* ) &m[ 0 ] ) );
					vrlh = _mm256_fmadd_ps( vzw, _mm256_loadu_ps( ( const float* ) &m[ 2 ] ), vrlh );

					auto vrl = _mm256_extractf128_ps( vrlh, 0 );
					auto vrh = _mm256_extractf128_ps( vrlh, 1 );
					return vec4f::from_xvec( xvec<float, 4>{ std::in_place_t{}, _mm_add_ps( vrl, vrh ) } );
				}
			}
#endif
			auto r = m[ 0 ] * v.x;
			r += m[ 1 ] * v.y;
			r += m[ 2 ] * v.z;
			r += m[ 3 ];
			return r;
		}
		FORCE_INLINE inline constexpr vec3_t<F> operator*( const vec3_t<F>& v ) const noexcept
		{
			return apply_no_w( vec::from_xvec( v.to_xvec() ) ).xyz();
		}

		// Offset by scalar.
		//
		FORCE_INLINE inline constexpr mat4x4_t operator+( F v ) const noexcept
		{
			mat4x4_t result;
			for ( size_t i = 0; i != 4; i++ )
				result[ i ] = m[ i ] + v;
			return result;
		}
		FORCE_INLINE inline constexpr mat4x4_t operator-( F v ) const noexcept { return *this + ( -v ); }
		FORCE_INLINE inline constexpr mat4x4_t& operator-=( F v ) noexcept { *this = ( *this - v ); return *this; }
		FORCE_INLINE inline constexpr mat4x4_t& operator+=( F v ) noexcept { *this = ( *this + v ); return *this; }

		// Scale by scalar.
		//
		FORCE_INLINE inline constexpr mat4x4_t operator*( F v ) const noexcept
		{
#if XSTD_MATH_USE_X86INTRIN
			if constexpr ( std::is_same_v<F, float> ) {
				if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) ) {
					__m256 f = _mm256_broadcast_ss( &v );

					__m256 a, b;
					impl::load_matrix_f32( m, a, b );
					a = _mm256_mul_ps( a, f );
					b = _mm256_mul_ps( b, f );
					return impl::store_matrix_f32<mat4x4_t<float>>( a, b );
				}
			}
#endif
			mat4x4_t result;
			for ( size_t i = 0; i != 4; i++ )
				result[ i ] = m[ i ] * v;
			return result;
		}
		FORCE_INLINE inline constexpr mat4x4_t operator/( F v ) const noexcept { return *this * rcp( v ); }
		FORCE_INLINE inline constexpr mat4x4_t& operator/=( F v ) noexcept { *this = ( *this / v ); return *this; }
		FORCE_INLINE inline constexpr mat4x4_t& operator*=( F v ) noexcept { *this = ( *this * v ); return *this; }

		// Unaries.
		//
		FORCE_INLINE inline constexpr mat4x4_t operator-() const noexcept 
		{ 
#if XSTD_MATH_USE_X86INTRIN
			if constexpr ( std::is_same_v<F, float> ) {
				if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) ) {
					__m256 a, b;
					impl::load_matrix_f32( m, a, b );
					__v8su c{ 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
					a = _mm256_xor_ps( a, ( __m256 ) c );
					b = _mm256_xor_ps( b, ( __m256 ) c );
					return impl::store_matrix_f32<mat4x4_t<float>>( a, b );
				}
			}
#endif
			return *this * -1.0; 
		}
		FORCE_INLINE inline constexpr mat4x4_t operator+() const noexcept { return *this; }

		// Forward indexing.
		//
		FORCE_INLINE inline constexpr vec& operator[]( size_t n ) noexcept { return m[ n & 3 ]; }
		FORCE_INLINE inline constexpr const vec& operator[]( size_t n ) const noexcept { return m[ n & 3 ]; }
		FORCE_INLINE inline constexpr F& operator()( size_t i, size_t j ) noexcept { return m[ i ][ j ]; }
		FORCE_INLINE inline constexpr const F& operator()( size_t i, size_t j ) const noexcept { return m[ i ][ j ]; }

		// Hashing, serialization, comparison and string conversion.
		//
		FORCE_INLINE inline constexpr auto operator<=>( const mat4x4_t& ) const noexcept = default;
		FORCE_INLINE inline auto tie() { return std::tie( m ); }
		FORCE_INLINE inline std::string to_string() const noexcept 
		{ 
			return fmt::str(
				"| %-10f %-10f %-10f %-10f |\n"
				"| %-10f %-10f %-10f %-10f |\n"
				"| %-10f %-10f %-10f %-10f |\n"
				"| %-10f %-10f %-10f %-10f |",
				m[ 0 ][ 0 ], m[ 0 ][ 1 ], m[ 0 ][ 2 ], m[ 0 ][ 3 ],
				m[ 1 ][ 0 ], m[ 1 ][ 1 ], m[ 1 ][ 2 ], m[ 1 ][ 3 ],
				m[ 2 ][ 0 ], m[ 2 ][ 1 ], m[ 2 ][ 2 ], m[ 2 ][ 3 ],
				m[ 3 ][ 0 ], m[ 3 ][ 1 ], m[ 3 ][ 2 ], m[ 3 ][ 3 ]
			);
		}

		// Transpose.
		//
		FORCE_INLINE inline constexpr mat4x4_t transpose() const noexcept
		{
#if XSTD_MATH_USE_X86INTRIN
			if constexpr ( std::is_same_v<F, float> ) {
				if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) ) {
					__m256 v01, v23;
					impl::load_matrix_f32( m, v01, v23 );
					impl::tranpose_inplace_f32( v01, v23 );
					return impl::store_matrix_f32<mat4x4_t<float>>( v01, v23 );
				}
			}
#endif
			mat4x4_t out;
			for ( size_t i = 0; i != 4; i++ )
				for ( size_t j = 0; j != 4; j++ )
					out[ i ][ j ] = m[ j ][ i ];
			return out;
		}

		// Determinant.
		//
		FORCE_INLINE inline constexpr F determinant() const noexcept
		{
			vec minor = cross(
				vec{ m[ 0 ][ 0 ], m[ 1 ][ 0 ], m[ 2 ][ 0 ], m[ 3 ][ 0 ] },
				vec{ m[ 0 ][ 1 ], m[ 1 ][ 1 ], m[ 2 ][ 1 ], m[ 3 ][ 1 ] },
				vec{ m[ 0 ][ 2 ], m[ 1 ][ 2 ], m[ 2 ][ 2 ], m[ 3 ][ 2 ] }
			);
			return -( m[ 0 ][ 3 ] * minor.x + m[ 1 ][ 3 ] * minor.y + m[ 2 ][ 3 ] * minor.z + m[ 3 ][ 3 ] * minor.w );
		}

		// Cast.
		//
		template<typename O>
		FORCE_INLINE inline constexpr mat4x4_t<O> cast() const 
		{ 
			return { 
				m[ 0 ].template cast<O>(),
				m[ 1 ].template cast<O>(),
				m[ 2 ].template cast<O>(),
				m[ 3 ].template cast<O>(),
			};
		}
		template<typename F2>
		FORCE_INLINE inline constexpr operator mat4x4_t<F2>() const noexcept { return this->template cast<F2>(); }
	};
	using mat4x4f = mat4x4_t<float>;
	using mat4x4d = mat4x4_t<double>;
	using mat4x4 =  mat4x4_t<fp_t>;
	
	// Implement fast polynomial approximations of sin and cos.
	//
	namespace impl
	{
		template<typename T>
		struct unit_t { using type = typename T::value_type; };
		template<Float F>
		struct unit_t<F> { using type = F; };
		template<typename T>
		using U = typename unit_t<T>::type;

		// [0, 1/2 pi]
		// - https://gist.github.com/publik-void/0677f2ef32dbe5c27d6e215824c91#file-sin-cos-approximations-gist-adoc
		template<typename V>
		CONST_FN FORCE_INLINE inline constexpr V fsin_poly_hpi_v( V x1 )
		{
			V x2 = x1 * x1;
			// Degree 11, E(X) = 1.92e-11
			return x1 * ( U<V>( 0.99999999997884898600402426033768998 ) +
				x2 * ( U<V>( -0.166666666088260696413164261885310067 ) +
				x2 * ( U<V>( 0.00833333072055773645376566203656709979 ) +
				x2 * ( U<V>( -0.000198408328232619552901560108010257242 ) +
				x2 * ( U<V>( 2.75239710746326498401791551303359689e-6 ) -
				x2 * U<V>( 2.3868346521031027639830001794722295e-8 ) ) ) ) ) );
		}
		template<typename V>
		CONST_FN FORCE_INLINE inline constexpr V fcos_poly_hpi_v( V x1 )
		{
			V x2 = x1 * x1;
			// Degree 12, E(X) = 3.35e-12
			return U<V>( 0.99999999999664497762294088303450344 ) +
				x2 * ( U<V>( -0.499999999904093446864749737540127153 ) +
				x2 * ( U<V>( 0.0416666661919898461055893453767336909 ) +
				x2 * ( U<V>( -0.00138888797032770920681384355560203468 ) +
				x2 * ( U<V>( 0.0000248007136556145113256051130495176344 ) +
				x2 * ( U<V>( -2.75135611164571371141959208910569516e-7 ) +
				x2 * U<V>( 1.97644182995841772799444848310451781e-9 ) ) ) ) ) );
		}
		
		template<typename V>
		CONST_FN FORCE_INLINE inline constexpr V facos_poly_pos_v( V x )
		{
			// Degree 7, E(X) = 2.33e-7
			return U<V>( 1.5707963050 ) +
				x * ( U<V>( -0.2145988016 ) +
				x * ( U<V>( +0.0889789874 ) +
				x * ( U<V>( -0.0501743046 ) +
				x * ( U<V>( +0.0308918810 ) +
				x * ( U<V>( -0.0170881256 ) +
				x * ( U<V>( +0.0066700901 ) +
				x * U<V>( -0.0012624911 ) ) ) ) ) ) );
		}

		// Corrected scalar versions.
		//
		template<Float F = fp_t>
		CONST_FN FORCE_INLINE inline constexpr F fsin_poly_hpi( F x1 )
		{
			if ( std::is_constant_evaluated() )
			{
				if ( ( x1 - F(1e-4) ) <= 0 ) return 0;
				if ( ( x1 + F(1e-4) ) >= F( pi / 2 ) ) return 1;
			}
			return fsin_poly_hpi_v( x1 );
		}
		template<Float F = fp_t>
		CONST_FN FORCE_INLINE inline constexpr F fcos_poly_hpi( F x1 )
		{
			if ( std::is_constant_evaluated() )
			{
				if ( ( x1 - F(1e-4) ) <= 0 ) return 1;
				if ( ( x1 + F(1e-4) ) >= ( pi / 2 ) ) return 0;
			}
			return fcos_poly_hpi_v( x1 );
		}
		template<Float F = fp_t>
		CONST_FN FORCE_INLINE inline constexpr F facos_poly_pos( F x1 )
		{
			if ( std::is_constant_evaluated() )
			{
				if ( ( x1 + F(1e-4) ) >= 1 ) return F( 1.5707963050 );
				if ( ( x1 - F(1e-4) ) <= 0 ) return F( 0.0 );
			}
			return facos_poly_pos_v( x1 );
		}

		// Vector asin-acos.
		//
		template<Float F = fp_t>
		CONST_FN FORCE_INLINE inline constexpr std::pair<vec4_t<F>, vec4_t<F>> asincos_poly( vec4_t<F> x )
		{
			// acos [0-1]
			vec4_t<F> xa = vec_abs( x );
			vec4_t<F> result = facos_poly_pos_v( xa ) * vec_sqrt( 1 - xa );

			// fix sign
			result = vec_copysign( result, x );
			result += vec_ifsgn( x, vec4_t<F>::fill( F( 1.5707963050 * 2 ) ) );
			return { -( result - F( 1.5707963050 ) ), result };
		}

		// Vector sin-cos.
		//
		template<Float F = fp_t>
		CONST_FN FORCE_INLINE inline constexpr std::pair<vec4_t<F>, vec4_t<F>> sincos_poly( vec4_t<F> x )
		{
			// [0, inf]
			vec4_t<F> a = vec_abs( x ) / F( pi );

			// [0, pi]
			vec4_t<F> n = vec_trunc( a );
			vec4_t<F> v = a - n;

			// [0, pi/2]
			v = vec_min( v, 1 - v ) * F( pi );
			return {
				vec_copysign( fsin_poly_hpi_v( v ), vec_xorsgn( x, vec_oddsgn( n ) ) ),
				vec_copysign( fcos_poly_hpi_v( v ), vec_oddsgn( vec_round( a ) ) )
			};
		}

		// Scalar asin-acos.
		//
		template<Float F = fp_t>
		CONST_FN FORCE_INLINE inline std::pair<F, F> asincos_poly( F x )
		{
			// acos [0-1]
			F xa = fabs( x );
			F result = facos_poly_pos( xa ) * sqrtf( 1 - xa );

			// fix sign
			result = fcopysign( result, x );
			result += fifsgn( x, F( 1.5707963050 * 2 ) );
			return { -( result - F( 1.5707963050 ) ), result };
		}

		// Scalar sin-cos.
		//
		template<Float F = fp_t>
		CONST_FN FORCE_INLINE inline constexpr std::pair<F, F> sincos_poly( F x )
		{
			// [0, inf]
			F a = fabs( x ) / F(pi);

			// [0, pi]
			F n = ftrunc( a );
			F v = a - n;

			// [0, pi/2]
			v = fmin( v, 1 - v ) * F( pi );
			return {
				fcopysign( fsin_poly_hpi( v ), fxorsgn( x, foddsgn( n ) ) ),
				fcopysign( fcos_poly_hpi( v ), foddsgn( fround( a ) ) )
			};
		}
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr F fsin( F x )
	{
		if ( std::is_constant_evaluated() || XSTD_MATH_USE_POLY_TRIG )
			return impl::sincos_poly( x ).first;
		return _fsin( x );
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr F fcos( F x )
	{
		if ( std::is_constant_evaluated() || XSTD_MATH_USE_POLY_TRIG )
			return impl::sincos_poly( x ).second;
		return _fcos( x );
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline F fasin( F x )
	{
		if ( std::is_constant_evaluated() || XSTD_MATH_USE_POLY_TRIG )
			return impl::asincos_poly( x ).first;
		return asinf( x );
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline F facos( F x )
	{
		if ( std::is_constant_evaluated() || XSTD_MATH_USE_POLY_TRIG )
			return impl::asincos_poly( x ).second;
		return acosf( x );
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline std::pair<F, F> fasincos( F x )
	{
		if ( std::is_constant_evaluated() || XSTD_MATH_USE_POLY_TRIG )
			return impl::asincos_poly( x );
		return { asinf( x ), acosf( x ) };
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr std::pair<F, F> fsincos( F x )
	{
		if ( std::is_constant_evaluated() || XSTD_MATH_USE_POLY_TRIG )
			return impl::sincos_poly( x );
		F s = fsin( x );
		F c = fcopysign( fsqrt( 1 - s * s ), foddsgn( fround( fabs( x ) / pi ) ) );
		return { s, c };
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr F ftan( F x )
	{
		if ( std::is_constant_evaluated() )
		{
			auto [s, c] = impl::sincos_poly( x );
			return s / c;
		}
		return std::tan( x );
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr vec4_t<F> vec_sin( const vec4_t<F>& x )
	{
		if ( std::is_constant_evaluated() || XSTD_MATH_USE_POLY_TRIG )
			return impl::sincos_poly( x ).first;
		return { _fsin( x.x ), _fsin( x.y ), _fsin( x.z ), _fsin( x.w ) };
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr vec4_t<F> vec_cos( const vec4_t<F>& x )
	{
		if ( std::is_constant_evaluated() || XSTD_MATH_USE_POLY_TRIG )
			return impl::sincos_poly( x ).second;
		return { _fcos( x.x ), _fcos( x.y ), _fcos( x.z ), _fcos( x.w ) };
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline vec4_t<F> vec_asin( const vec4_t<F>& x )
	{
		if ( std::is_constant_evaluated() || XSTD_MATH_USE_POLY_TRIG )
			return impl::asincos_poly( x ).first;
		return { fasin( x.x ), fasin( x.y ), fasin( x.z ), fasin( x.w ) };
	}
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline vec4_t<F> vec_acos( const vec4_t<F>& x )
	{
		if ( std::is_constant_evaluated() || XSTD_MATH_USE_POLY_TRIG )
			return impl::asincos_poly( x ).second;
		return { facos( x.x ), facos( x.y ), facos( x.z ), facos( x.w ) };
	}
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec3_t<F> vec_sin( const vec3_t<F>& vec ) { return vec_sin( vec4_t<F>::from( vec, 0 ) ).xyz(); }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec3_t<F> vec_cos( const vec3_t<F>& vec ) { return vec_cos( vec4_t<F>::from( vec, 0 ) ).xyz(); }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline vec3_t<F> vec_asin( const vec3_t<F>& vec ) { return vec_asin( vec4_t<F>::from( vec, 0 ) ).xyz(); }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline vec3_t<F> vec_acos( const vec3_t<F>& vec ) { return vec_acos( vec4_t<F>::from( vec, 0 ) ).xyz(); }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec2_t<F> vec_sin( const vec2_t<F>& vec ) { return { fsin( vec.x ), fsin( vec.y ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline constexpr vec2_t<F> vec_cos( const vec2_t<F>& vec ) { return { fcos( vec.x ), fcos( vec.y ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline vec2_t<F> vec_asin( const vec2_t<F>& vec ) { return { fasin( vec.x ), fasin( vec.y ) }; }
	template<Float F = fp_t> CONST_FN FORCE_INLINE inline vec2_t<F> vec_acos( const vec2_t<F>& vec ) { return { facos( vec.x ), facos( vec.y ) }; }

	// Matrix inversion.
	//
	template<Float F = fp_t>
	inline constexpr mat4x4_t<F> inverse( const mat4x4_t<F>& m, F& det )
	{
		F t0, t1, t2;
		mat4x4_t<F> out = {};
		t0 =              m[ 2 ][ 2 ] * m[ 3 ][ 3 ] - m[ 2 ][ 3 ] * m[ 3 ][ 2 ];
		t1 =              m[ 1 ][ 2 ] * m[ 3 ][ 3 ] - m[ 1 ][ 3 ] * m[ 3 ][ 2 ];
		t2 =              m[ 1 ][ 2 ] * m[ 2 ][ 3 ] - m[ 1 ][ 3 ] * m[ 2 ][ 2 ];
		out[ 0 ][ 0 ] =   m[ 1 ][ 1 ] * t0 - m[ 2 ][ 1 ] * t1 + m[ 3 ][ 1 ] * t2;
		out[ 1 ][ 0 ] =  -m[ 1 ][ 0 ] * t0 + m[ 2 ][ 0 ] * t1 - m[ 3 ][ 0 ] * t2;

		t0 =              m[ 1 ][ 0 ] * m[ 2 ][ 1 ] - m[ 2 ][ 0 ] * m[ 1 ][ 1 ];
		t1 =              m[ 1 ][ 0 ] * m[ 3 ][ 1 ] - m[ 3 ][ 0 ] * m[ 1 ][ 1 ];
		t2 =              m[ 2 ][ 0 ] * m[ 3 ][ 1 ] - m[ 3 ][ 0 ] * m[ 2 ][ 1 ];
		out[ 2 ][ 0 ] =   m[ 3 ][ 3 ] * t0 - m[ 2 ][ 3 ] * t1 + m[ 1 ][ 3 ] * t2;
		out[ 3 ][ 0 ] =  -m[ 3 ][ 2 ] * t0 + m[ 2 ][ 2 ] * t1 - m[ 1 ][ 2 ] * t2;

		t0 =              m[ 2 ][ 2 ] * m[ 3 ][ 3 ] - m[ 2 ][ 3 ] * m[ 3 ][ 2 ];
		t1 =              m[ 0 ][ 2 ] * m[ 3 ][ 3 ] - m[ 0 ][ 3 ] * m[ 3 ][ 2 ];
		t2 =              m[ 0 ][ 2 ] * m[ 2 ][ 3 ] - m[ 0 ][ 3 ] * m[ 2 ][ 2 ];
		out[ 0 ][ 1 ] =  -m[ 0 ][ 1 ] * t0 + m[ 2 ][ 1 ] * t1 - m[ 3 ][ 1 ] * t2;
		out[ 1 ][ 1 ] =   m[ 0 ][ 0 ] * t0 - m[ 2 ][ 0 ] * t1 + m[ 3 ][ 0 ] * t2;

		t0 =              m[ 0 ][ 0 ] * m[ 2 ][ 1 ] - m[ 2 ][ 0 ] * m[ 0 ][ 1 ];
		t1 =              m[ 3 ][ 0 ] * m[ 0 ][ 1 ] - m[ 0 ][ 0 ] * m[ 3 ][ 1 ];
		t2 =              m[ 2 ][ 0 ] * m[ 3 ][ 1 ] - m[ 3 ][ 0 ] * m[ 2 ][ 1 ];
		out[ 2 ][ 1 ] =  -m[ 3 ][ 3 ] * t0 - m[ 2 ][ 3 ] * t1 - m[ 0 ][ 3 ] * t2;
		out[ 3 ][ 1 ] =   m[ 3 ][ 2 ] * t0 + m[ 2 ][ 2 ] * t1 + m[ 0 ][ 2 ] * t2;

		t0 =              m[ 1 ][ 2 ] * m[ 3 ][ 3 ] - m[ 1 ][ 3 ] * m[ 3 ][ 2 ];
		t1 =              m[ 0 ][ 2 ] * m[ 3 ][ 3 ] - m[ 0 ][ 3 ] * m[ 3 ][ 2 ];
		t2 =              m[ 0 ][ 2 ] * m[ 1 ][ 3 ] - m[ 0 ][ 3 ] * m[ 1 ][ 2 ];
		out[ 0 ][ 2 ] =   m[ 0 ][ 1 ] * t0 - m[ 1 ][ 1 ] * t1 + m[ 3 ][ 1 ] * t2;
		out[ 1 ][ 2 ] =  -m[ 0 ][ 0 ] * t0 + m[ 1 ][ 0 ] * t1 - m[ 3 ][ 0 ] * t2;

		t0 =              m[ 0 ][ 0 ] * m[ 1 ][ 1 ] - m[ 1 ][ 0 ] * m[ 0 ][ 1 ];
		t1 =              m[ 3 ][ 0 ] * m[ 0 ][ 1 ] - m[ 0 ][ 0 ] * m[ 3 ][ 1 ];
		t2 =              m[ 1 ][ 0 ] * m[ 3 ][ 1 ] - m[ 3 ][ 0 ] * m[ 1 ][ 1 ];
		out[ 2 ][ 2 ] =   m[ 3 ][ 3 ] * t0 + m[ 1 ][ 3 ] * t1 + m[ 0 ][ 3 ] * t2;
		out[ 3 ][ 2 ] =  -m[ 3 ][ 2 ] * t0 - m[ 1 ][ 2 ] * t1 - m[ 0 ][ 2 ] * t2;

		t0 =              m[ 1 ][ 2 ] * m[ 2 ][ 3 ] - m[ 1 ][ 3 ] * m[ 2 ][ 2 ];
		t1 =              m[ 0 ][ 2 ] * m[ 2 ][ 3 ] - m[ 0 ][ 3 ] * m[ 2 ][ 2 ];
		t2 =              m[ 0 ][ 2 ] * m[ 1 ][ 3 ] - m[ 0 ][ 3 ] * m[ 1 ][ 2 ];
		out[ 0 ][ 3 ] =  -m[ 0 ][ 1 ] * t0 + m[ 1 ][ 1 ] * t1 - m[ 2 ][ 1 ] * t2;
		out[ 1 ][ 3 ] =   m[ 0 ][ 0 ] * t0 - m[ 1 ][ 0 ] * t1 + m[ 2 ][ 0 ] * t2;
		out[ 2 ][ 3 ] =  -m[ 0 ][ 0 ] * ( m[ 1 ][ 1 ] * m[ 2 ][ 3 ] - m[ 1 ][ 3 ] * m[ 2 ][ 1 ] ) +
								m[ 1 ][ 0 ] * ( m[ 0 ][ 1 ] * m[ 2 ][ 3 ] - m[ 0 ][ 3 ] * m[ 2 ][ 1 ] ) -
								m[ 2 ][ 0 ] * ( m[ 0 ][ 1 ] * m[ 1 ][ 3 ] - m[ 0 ][ 3 ] * m[ 1 ][ 1 ] );
		out[ 3 ][ 3 ] =   m[ 0 ][ 0 ] * ( m[ 1 ][ 1 ] * m[ 2 ][ 2 ] - m[ 1 ][ 2 ] * m[ 2 ][ 1 ] ) -
								m[ 1 ][ 0 ] * ( m[ 0 ][ 1 ] * m[ 2 ][ 2 ] - m[ 0 ][ 2 ] * m[ 2 ][ 1 ] ) +
								m[ 2 ][ 0 ] * ( m[ 0 ][ 1 ] * m[ 1 ][ 2 ] - m[ 0 ][ 2 ] * m[ 1 ][ 1 ] );

		det = ( m[ 0 ][ 0 ] * out[ 0 ][ 0 ] + m[ 0 ][ 1 ] * out[ 1 ][ 0 ] + m[ 0 ][ 2 ] * out[ 2 ][ 0 ] + m[ 0 ][ 3 ] * out[ 3 ][ 0 ] );
		return out / det;
	}
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr mat4x4_t<F> inverse( const mat4x4_t<F>& m ) { F tmp = 0; return inverse( m, tmp ); }

	// Matrix decomposition helpers.
	//
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr vec3_t<F> matrix_to_translation( const mat4x4_t<F>& mat )
	{
		return mat[ 3 ].xyz();
	}
	template<Float F = fp_t>
	FORCE_INLINE inline vec3_t<F> matrix_to_scale( const mat4x4_t<F>& mat )
	{
		return { mat[ 0 ].xyz().length(), mat[ 1 ].xyz().length(), mat[ 2 ].xyz().length() };
	}
	template<Float F = fp_t>
	FORCE_INLINE inline vec3_t<F> matrix_to_forward( const mat4x4_t<F>& mat )
	{
		return normalize( mat[ 0 ].xyz() );
	}
	template<Float F = fp_t>
	FORCE_INLINE inline vec3_t<F> matrix_to_right( const mat4x4_t<F>& mat )
	{
		return normalize( mat[ 1 ].xyz() );
	}
	template<Float F = fp_t>
	FORCE_INLINE inline vec3_t<F> matrix_to_up( const mat4x4_t<F>& mat )
	{
		return normalize( mat[ 2 ].xyz() );
	}

	// Normalizes a matrix to have no scale.
	//
	template<Float F = fp_t>
	FORCE_INLINE inline mat4x4_t<F> matrix_normalize( mat4x4_t<F> mat )
	{
		vec4 scales = rcp( vec_sqrt( vec4{ mat[ 0 ].length_sq(), mat[ 1 ].length_sq(), mat[ 2 ].length_sq(), 0 } ) );
		mat[ 0 ] *= scales[ 0 ];
		mat[ 1 ] *= scales[ 1 ];
		mat[ 2 ] *= scales[ 2 ];
		return mat;
	}

	// Combining rotations.
	//
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr quaternion_t<F> rotate_by( const quaternion_t<F>& q2, const quaternion_t<F>& q1 )
	{
		return {
			 q1.x * q2.w + q1.y * q2.z - q1.z * q2.y + q1.w * q2.x,
			-q1.x * q2.z + q1.y * q2.w + q1.z * q2.x + q1.w * q2.y,
			 q1.x * q2.y - q1.y * q2.x + q1.z * q2.w + q1.w * q2.z,
			-q1.x * q2.x - q1.y * q2.y - q1.z * q2.z + q1.w * q2.w
		};
	}
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr mat4x4_t<F> rotate_by( const mat4x4_t<F>& m1, const mat4x4_t<F>& m2 )
	{
		return m1 * m2;
	}

	// Rotating a vector.
	//
	template<Float F = fp_t> FORCE_INLINE inline constexpr vec3_t<F> rotate_by( const vec3_t<F>& v, const mat4x4_t<F>& m ) { return m * v; }
	template<Float F = fp_t> FORCE_INLINE inline constexpr vec4_t<F> rotate_by( const vec4_t<F>& v, const mat4x4_t<F>& m ) { return m * v; }
	template<Float F = fp_t> FORCE_INLINE inline constexpr vec3_t<F> rotate_by( const vec3_t<F>& v, const quaternion_t<F>& q )
	{
		vec3_t<F> t = cross( q.xyz(), v ) * 2;
		return v + t * q.w + cross( q.xyz(), t );
	}

	// Translation and scaling matrices.
	//
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr mat4x4_t<F> matrix_translation( const vec3_t<F>& p )
	{
		mat4x4_t<F> out = mat4x4_t<F>::identity();
		out[ 3 ][ 0 ] = p.x;
		out[ 3 ][ 1 ] = p.y;
		out[ 3 ][ 2 ] = p.z;
		return out;
	}
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr mat4x4_t<F> matrix_scaling( const vec3_t<F>& p )
	{
		mat4x4_t<F> out = mat4x4_t<F>::identity();
		out[ 0 ][ 0 ] = p.x;
		out[ 1 ][ 1 ] = p.y;
		out[ 2 ][ 2 ] = p.z;
		return out;
	}

	// Spherical quaternion interpolation.
	//
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr quaternion_t<F> quaternion_slerp( const quaternion_t<F>& q1, const quaternion_t<F>& q2, F t )
	{
		F t2 = 1 - t;

		// Compute dot, if negative flip the signs.
		//
		F dotv = dot( q1, q2 );
		t = fcopysign( t, dotv );
		dotv = fabs( dotv );

		// If theta is numerically significant and will not result in division by zero:
		//
		if ( dotv < F( 0.999 ) )
		{
			F theta = facos( dotv );
			F rstheta = rcp( fsin( theta ) );
			t2 = fsin( theta * t2 ) * rstheta;
			t =  fsin( theta * t )  * rstheta;
		}
		return t2 * q1 + t * q2;
	}
	// Define helpers for radian / degrees.
	//
	template<typename V>
	FORCE_INLINE inline constexpr auto to_rad( V deg ) { return deg * ( pi / 180.0 ); }
	template<typename V>
	FORCE_INLINE inline constexpr auto to_deg( V rad ) { return rad * ( 180.0 / pi ); }

	// Default axis used for euler world.
	//
	inline constexpr vec3 up =      { 0, 0, 1 };
	inline constexpr vec3 right =   { 0, 1, 0 };
	inline constexpr vec3 forward = { 1, 0, 0 };

	// Matrix to Euler/Quaternion/Direction.
	//
	template<Float F = fp_t>
	FORCE_INLINE inline vec3_t<F> matrix_to_euler( const mat4x4_t<F>& mat )
	{
		// Thanks to Eigen.
		//
		vec3 res = {};
		F rsum = fsqrt( ( mat( 0, 0 ) * mat( 0, 0 ) + mat( 0, 1 ) * mat( 0, 1 ) + mat( 1, 2 ) * mat( 1, 2 ) + mat( 2, 2 ) * mat( 2, 2 ) ) / 2 );
		res.x = atan2( -mat( 0, 2 ), rsum );
		if ( rsum > 4 * fp_eps<F> )
		{
			res.z = atan2( mat( 1, 2 ), mat( 2, 2 ) );
			res.y = atan2( mat( 0, 1 ), mat( 0, 0 ) );
		}
		else if ( -mat( 0, 2 ) > 0 )
		{
			F spos = mat( 1, 0 ) + -mat( 2, 1 );
			F cpos = mat( 1, 1 ) + mat( 2, 0 );
			res.z = atan2( spos, cpos );
			res.y = 0;
		}
		else
		{
			F sneg = -( mat( 2, 1 ) + mat( 1, 0 ) );
			F cneg = mat( 1, 1 ) + -mat( 2, 0 );
			res.z = atan2( sneg, cneg );
			res.y = 0;
		}
		return res;
	}
	template<Float F = fp_t>
	FORCE_INLINE inline quaternion_t<F> matrix_to_quaternion( const mat4x4_t<F>& umat )
	{
		mat4x4_t<F> mat = matrix_normalize( umat );
		F trace = mat( 0, 0 ) + mat( 1, 1 ) + mat( 2, 2 ) + 1;
		if ( trace > 1 )
		{
			F s = 2 * fsqrt( trace );
			return {
				( mat( 1, 2 ) - mat( 2, 1 ) ) / s,
				( mat( 2, 0 ) - mat( 0, 2 ) ) / s,
				( mat( 0, 1 ) - mat( 1, 0 ) ) / s,
				F(0.25) * s
			};
		}
		else if ( mat( 0, 0 ) > mat( 1, 1 ) && mat( 0, 0 ) > mat( 2, 2 ) )
		{
			F s = 2 * fsqrt( 1.0 + mat( 0, 0 ) - mat( 1, 1 ) - mat( 2, 2 ) );
			return {
				F(0.25) * s,
				( mat( 0, 1 ) + mat( 1, 0 ) ) / s,
				( mat( 0, 2 ) + mat( 2, 0 ) ) / s,
				( mat( 1, 2 ) - mat( 2, 1 ) ) / s,
			};
		}
		else if ( mat( 1, 1 ) > mat( 2, 2 ) )
		{
			F s = 2 * fsqrt( 1.0 + mat( 1, 1 ) - mat( 0, 0 ) - mat( 2, 2 ) );
			return {
				( mat( 0, 1 ) + mat( 1, 0 ) ) / s,
				F(0.25) * s,
				( mat( 1, 2 ) + mat( 2, 1 ) ) / s,
				( mat( 2, 0 ) - mat( 0, 2 ) ) / s,
			};
		}
		else
		{
			F s = 2 * fsqrt( 1.0 + mat( 2, 2 ) - mat( 0, 0 ) - mat( 1, 1 ) );
			return {
				( mat( 0, 2 ) + mat( 2, 0 ) ) / s,
				( mat( 1, 2 ) + mat( 2, 1 ) ) / s,
				F(0.25) * s,
				( mat( 0, 1 ) - mat( 1, 0 ) ) / s,
			};
		}
	}
	template<Float F = fp_t>
	FORCE_INLINE inline vec3_t<F> matrix_to_direction( const mat4x4_t<F>& mat )
	{
		return normalize( matrix_to_forward( mat ) );
	}

	// Direction to Euler/Quaternion/Matrix.
	//
	template<Float F = fp_t>
	FORCE_INLINE inline mat4x4_t<F> direction_to_matrix( const vec3_t<F>& direction )
	{
		vec4_t<F> up_vec =    {  0, 1, 0, 0 };
		vec4_t<F> right_vec = { -1, 0, 0, 0 };
		vec4_t<F> dir_vec =   normalize( vec4_t<F>::from( direction, 0 ) );

		if ( ( direction.x * direction.x + direction.y * direction.y ) > 1e-5 )
		{
			vec3_t<F> up_vec3 =    cross( up, direction );
			vec3_t<F> right_vec3 = cross( direction, up_vec3 );
			up_vec =    vec4_t<F>::from( up_vec3, 0 );
			right_vec = vec4_t<F>::from( right_vec3, 0 );
		}

		return {
			dir_vec,
			normalize( up_vec ),
			normalize( right_vec ),
			vec4_t<F>{ 0, 0, 0, 1 },
		};
	}
	template<Float F = fp_t>
	FORCE_INLINE inline vec3_t<F> direction_to_euler( const vec3_t<F>& direction )
	{
		return vec3{ -fasin( direction.z / direction.length() ), atan2( direction.y, direction.x ), 0 };
	}
	template<Float F = fp_t>
	FORCE_INLINE inline quaternion_t<F> direction_to_quaternion( const vec3_t<F>& direction )
	{
		return matrix_to_quaternion( direction_to_matrix( direction ) );
	}

	// Quaternion to Euler/Matrix/Direction.
	//
	template<Float F = fp_t>
	FORCE_INLINE inline vec3_t<F> quaternion_to_euler( const quaternion_t<F>& q )
	{
		F sinr_cosp = 2 * ( q.w * q.x + q.y * q.z );
		F cosr_cosp = 1 - 2 * ( q.x * q.x + q.y * q.y );
		F sinp = 2 * ( q.w * q.y - q.z * q.x );
		F siny_cosp = 2 * ( q.w * q.z + q.x * q.y );
		F cosy_cosp = 1 - 2 * ( q.y * q.y + q.z * q.z );
		return {
			fasin( fclamp<F>( sinp, -1, +1 ) ),
			atan2( siny_cosp, cosy_cosp ),
			atan2( sinr_cosp, cosr_cosp )
		};
	}
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr mat4x4_t<F> quaternion_to_matrix_weighted( const quaternion_t<F>& rot, F w = 2 )
	{
		mat4x4_t<F> out = {};
		out[ 0 ][ 0 ] = 1 - w * ( rot.y * rot.y + rot.z * rot.z );
		out[ 0 ][ 1 ] = w * ( rot.x * rot.y + rot.z * rot.w );
		out[ 0 ][ 2 ] = w * ( rot.x * rot.z - rot.y * rot.w );
		out[ 1 ][ 0 ] = w * ( rot.x * rot.y - rot.z * rot.w );
		out[ 1 ][ 1 ] = 1 - w * ( rot.x * rot.x + rot.z * rot.z );
		out[ 1 ][ 2 ] = w * ( rot.y * rot.z + rot.x * rot.w );
		out[ 2 ][ 0 ] = w * ( rot.x * rot.z + rot.y * rot.w );
		out[ 2 ][ 1 ] = w * ( rot.y * rot.z - rot.x * rot.w );
		out[ 2 ][ 2 ] = 1 - w * ( rot.x * rot.x + rot.y * rot.y );
		out[ 3 ][ 3 ] = 1;
		return out;
	}
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr mat4x4_t<F> quaternion_to_matrix( const quaternion_t<F>& rot )
	{
		return quaternion_to_matrix_weighted( rot );
	}
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr vec3_t<F> quaternion_to_direction( const quaternion_t<F>& q )
	{
		return rotate_by( forward, q );
	}

	// Euler normalization.
	//
	FORCE_INLINE CONST_FN static constexpr float normalize_euler( float in )
	{
		return fmod( in + pi, 2 * pi ) - pi; // [-pi, +pi]
	}
	template<Float F = fp_t>
	FORCE_INLINE CONST_FN static constexpr vec3_t<F> normalize_euler( const vec3_t<F>& in )
	{
		return ( vec_mod( vec4_t<F>::from( in, 0 ) + pi, 2 * pi ) - pi ).xyz();
	}

	// Euler to Matrix/Quaternion/Direction.
	//
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr mat4x4_t<F> euler_to_matrix( const vec3_t<F>& rot )
	{
		auto [spitch, syaw, sroll] = vec_sin( rot );
		auto [cpitch, cyaw, croll] = vec_cos( rot );

		mat4x4_t<F> out = {};
		out[ 0 ][ 0 ] = cpitch * cyaw;
		out[ 0 ][ 1 ] = cpitch * syaw;
		out[ 0 ][ 2 ] = -spitch;
		out[ 0 ][ 3 ] = 0;
		out[ 1 ][ 0 ] = sroll * spitch * cyaw - croll * syaw;
		out[ 1 ][ 1 ] = sroll * spitch * syaw + croll * cyaw;
		out[ 1 ][ 2 ] = sroll * cpitch;
		out[ 1 ][ 3 ] = 0;
		out[ 2 ][ 0 ] = croll * spitch * cyaw + sroll * syaw;
		out[ 2 ][ 1 ] = croll * spitch * syaw - sroll * cyaw;
		out[ 2 ][ 2 ] = croll * cpitch;
		out[ 2 ][ 3 ] = 0;
		out[ 3 ][ 0 ] = 0;
		out[ 3 ][ 1 ] = 0;
		out[ 3 ][ 2 ] = 0;
		out[ 3 ][ 3 ] = 1;
		return out;
	}
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr quaternion_t<F> euler_to_quaternion( vec3_t<F> rot )
	{
		rot *= 0.5;
		auto [spitch, syaw, sroll] = vec_sin( rot );
		auto [cpitch, cyaw, croll] = vec_cos( rot );

		return {
			cyaw * cpitch * sroll - syaw * spitch * croll,
			syaw * cpitch * sroll + cyaw * spitch * croll,
			syaw * cpitch * croll - cyaw * spitch * sroll,
			cyaw * cpitch * croll + syaw * spitch * sroll
		};
	}
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr vec3_t<F> euler_to_direction( const vec3_t<F>& rot )
	{
		auto [spitch, cpitch] = fsincos( rot.x );
		auto [syaw,   cyaw] =   fsincos( rot.y );
		return {
			cpitch * cyaw,
			cpitch * syaw,
			-spitch
		};
	}
	
	// Angle-Axis helpers.
	//
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr quaternion_t<F> angle_axis_to_quaternion( F theta, const vec3_t<F>& axis )
	{
		auto [s, c] = fsincos( theta * 0.5 );
		return vec4_t<F>::from( s * axis, c );
	}
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr mat4x4_t<F> angle_axis_to_matrix( F theta, const vec3_t<F>& axis )
	{
		auto [sangle, cangle] = fsincos( theta );
		F cdiff = 1.0 - cangle;

		mat4x4_t<F> out;
		out[ 0 ][ 0 ] = cdiff * axis.x * axis.x + cangle;
		out[ 0 ][ 1 ] = cdiff * axis.y * axis.x + sangle * axis.z;
		out[ 0 ][ 2 ] = cdiff * axis.z * axis.x - sangle * axis.y;
		out[ 0 ][ 3 ] = 0.0;

		out[ 1 ][ 0 ] = cdiff * axis.x * axis.y - sangle * axis.z;
		out[ 1 ][ 1 ] = cdiff * axis.y * axis.y + cangle;
		out[ 1 ][ 2 ] = cdiff * axis.z * axis.y + sangle * axis.x;
		out[ 1 ][ 3 ] = 0.0;

		out[ 2 ][ 0 ] = cdiff * axis.x * axis.z + sangle * axis.y;
		out[ 2 ][ 1 ] = cdiff * axis.y * axis.z - sangle * axis.x;
		out[ 2 ][ 2 ] = cdiff * axis.z * axis.z + cangle;
		out[ 2 ][ 3 ] = 0.0;

		out[ 3 ][ 0 ] = 0.0;
		out[ 3 ][ 1 ] = 0.0;
		out[ 3 ][ 2 ] = 0.0;
		out[ 3 ][ 3 ] = 1.0;
		return out;
	}

	// Given a rotation matrix, converts it to rotation around a point.
	//
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr void make_rotation_off_center( mat4x4_t<F>& mat, vec4_t<F> center )
	{
		mat[ 3 ] += ( mat * -center ) + center;
	}

	// Fast transformation matrix.
	//
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr mat4x4_t<F> matrix_transformation( const quaternion_t<F>& rot, const vec3_t<F>& scale, const vec3_t<F>& translation )
	{
		auto mat = quaternion_to_matrix( rot );
		mat[ 0 ] *= scale.x;
		mat[ 1 ] *= scale.y;
		mat[ 2 ] *= scale.z;
		mat[ 3 ] = vec4_t<F>::from( translation, 1 );
		return mat;
	}

	// 2D helpers.
	//
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr mat4x4_t<F> euler_to_matrix( F rot )
	{
		auto [s, c] = fsincos( rot );
		return {
			 vec4_t<F>{ c,  s, 0, 0 },
			 vec4_t<F>{ -s, c, 0, 0 },
			 vec4_t<F>{ 0,  0, 1, 0 },
			 vec4_t<F>{ 0,  0, 0, 1 }
		};
	}
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr mat4x4_t<F> matrix_transformation( F rot, const vec2_t<F>& scale, const vec2_t<F>& translation )
	{
		auto [s, c] = fsincos( rot );
		return {
			 vec4_t<F>{ c,  s, 0, 0 } * scale.x,
			 vec4_t<F>{ -s, c, 0, 0 } * scale.y,
			 vec4_t<F>{ 0,  0, 1, 0 },
			 vec4_t<F>::from( translation, 0, 1 )
		};
	}

	// Intersection helpers.
	//
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr std::optional<F> ix_ray_sphere( const vec3_t<F>& ray_origin, const vec3_t<F>& ray_direction, 
																							const vec3_t<F>& sphere_origin, F sphere_radius )
	{
		auto m = ray_origin - sphere_origin;
		F b = dot( m, ray_direction );
		F c = m.length_sq() - sphere_radius * sphere_radius;
		if ( c >= 0 && b >= 0 )
			return std::nullopt;
		F d = b * b - c;
		if ( d < 0 ) 
			return std::nullopt;
		return fmax<F>( 0, -b - fsqrt( d ) );
	}
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr std::optional<F> ix_ray_box( const vec3_t<F>& ray_origin, const vec3_t<F>& ray_direction, 
																						const vec3_t<F>& box_min, const vec3_t<F>& box_max )
	{
		auto rdir = rcp( ray_direction );
		auto t1 = ( box_min - ray_origin ) * rdir;
		auto t2 = ( box_max - ray_origin ) * rdir;

		auto tmin = fmax<F>( t1.min_element( t2 ).reduce_max(), 0 );
		auto tmax = t1.max_element( t2 ).reduce_min();

		if ( tmax <= tmin )
			return std::nullopt;
		else
			return tmin;
	}
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr std::optional<F> ix_ray_plane( const vec3_t<F>& ray_origin, const vec3_t<F>& ray_direction, 
																						  const vec3_t<F>& plane_point, const vec3_t<F>& plane_normal )
	{
		F d = dot( plane_normal, ray_direction );
		if ( fabs( d ) > fp_eps<F> )
		{
			F t = dot( plane_point - ray_origin, plane_normal ) / d;
			if ( t >= 0 )
				return t;
		}
		return std::nullopt;
	}
	template<Float F = fp_t>
	FORCE_INLINE inline constexpr bool ix_aa_bb( const vec3_t<F>& a_min, const vec3_t<F>& a_max,
																const vec3_t<F>& b_min, const vec3_t<F>& b_max )
	{
		auto a = vec4_t<F>::from( a_max - b_min, 0 ); // >= 0
		auto b = vec4_t<F>::from( b_max - a_min, 0 ); // >= 0
		return a.min_element( b ).reduce_min() >= 0;
	}

	// View helpers.
	//
	template<bool LeftHanded = true, Float F = fp_t>
	FORCE_INLINE inline mat4x4_t<F> look_at( vec3_t<F> eye, vec3_t<F> dir, vec3_t<F> up )
	{
		constexpr F m = LeftHanded ? 1 : -1;

		vec3_t<F> right = cross( up, dir );
		up = cross( dir, right );
		right = normalize( right );
		up = normalize( up );

		mat4x4_t<F> out = {};
		out[ 0 ][ 0 ] = m * right.x;
		out[ 1 ][ 0 ] = m * right.y;
		out[ 2 ][ 0 ] = m * right.z;
		out[ 3 ][ 0 ] = m * -dot( right, eye );
		out[ 0 ][ 1 ] = up.x;
		out[ 1 ][ 1 ] = up.y;
		out[ 2 ][ 1 ] = up.z;
		out[ 3 ][ 1 ] = -dot( up, eye );
		out[ 0 ][ 2 ] = m * dir.x;
		out[ 1 ][ 2 ] = m * dir.y;
		out[ 2 ][ 2 ] = m * dir.z;
		out[ 3 ][ 2 ] = m * -dot( dir, eye );
		out[ 0 ][ 3 ] = 0.0;
		out[ 1 ][ 3 ] = 0.0;
		out[ 2 ][ 3 ] = 0.0;
		out[ 3 ][ 3 ] = 1.0;
		return out;
	}


	// 1->0 Depth, infinitely far.
	//
	template<bool LeftHanded = true, Float F = fp_t>
	FORCE_INLINE inline constexpr mat4x4_t<F> perspective_projection_rz( F tfx, F tfy, F zn )
	{
		constexpr F m = LeftHanded ? 1 : -1;
		mat4x4_t<F> out = mat4x4_t<F>::identity();
		out[ 0 ][ 0 ] = rcp( tfx );
		out[ 1 ][ 1 ] = rcp( tfy );
		out[ 2 ][ 2 ] = 0.0;
		out[ 2 ][ 3 ] = m * 1.0;
		out[ 3 ][ 2 ] = zn;
		out[ 3 ][ 3 ] = 0.0;
		return out;
	}

	// 1->0 Depth, far clipped.
	//
	template<bool LeftHanded = true, Float F = fp_t>
	FORCE_INLINE inline constexpr mat4x4_t<F> perspective_projection_rz( F tfx, F tfy, F zn, F zf )
	{
		constexpr F m = LeftHanded ? 1 : -1;
		mat4x4_t<F> out = mat4x4_t<F>::identity();
		out[ 0 ][ 0 ] = rcp( tfx );
		out[ 1 ][ 1 ] = rcp( tfy );
		out[ 2 ][ 2 ] = m * -zn / ( zf - zn );
		out[ 2 ][ 3 ] = m * 1.0;
		out[ 3 ][ 2 ] = ( zf * zn ) / ( zf - zn );
		out[ 3 ][ 3 ] = 0.0;
		return out;
	}

	// 0->1 Depth, infinitely far.
	//
	template<bool LeftHanded = true, Float F = fp_t>
	FORCE_INLINE inline constexpr mat4x4_t<F> perspective_projection( F tfx, F tfy, F zn )
	{
		constexpr F m = LeftHanded ? 1 : -1;
		mat4x4_t<F> out = mat4x4_t<F>::identity();
		out[ 0 ][ 0 ] = rcp( tfx );
		out[ 1 ][ 1 ] = rcp( tfy );
		out[ 2 ][ 2 ] = m * 1.0;
		out[ 2 ][ 3 ] = m * 1.0;
		out[ 3 ][ 2 ] = -zn;
		out[ 3 ][ 3 ] = 0.0;
		return out;
	}

	// 0->1 Depth, far clipped.
	//
	template<bool LeftHanded = true, Float F = fp_t>
	FORCE_INLINE inline constexpr mat4x4_t<F> perspective_projection( F tfx, F tfy, F zn, F zf )
	{
		constexpr F m = LeftHanded ? 1 : -1;
		mat4x4_t<F> out = mat4x4_t<F>::identity();
		out[ 0 ][ 0 ] = rcp( tfx );
		out[ 1 ][ 1 ] = rcp( tfy );
		out[ 2 ][ 2 ] = m * zf / ( zf - zn );
		out[ 2 ][ 3 ] = m * 1.0;
		out[ 3 ][ 2 ] = -( zf * zn ) / ( zf - zn );
		out[ 3 ][ 3 ] = 0.0;
		return out;
	}

	// Factorial, fast power and binomial helpers for beizer implementation.
	//
	template<Float F = fp_t>
	CONST_FN FORCE_INLINE inline constexpr F ffactorial( int i )
	{
		if ( std::is_constant_evaluated() || is_consteval( i ) )
		{
			if ( i >= 0 )
			{
				int r = 1;
				for ( int it = 2; it <= i; it++ )
					r *= it;
				return F( r );
			}
		}
		return tgammaf( i + 1 );
	}
	template<size_t I, typename V>
	CONST_FN FORCE_INLINE inline constexpr V const_pow( V value )
	{
		if constexpr ( I == 0 )
			return 1;
		else if constexpr ( I == 1 )
			return value;
		else if constexpr ( ( I % 3 ) == 0 )
			return const_pow<I / 3>( value * value * value );
		else if constexpr ( ( I % 2 ) == 0 )
			return const_pow<I / 2>( value * value );
		else
			return const_pow<I - 1>( value ) * value;
	}
	template<size_t N, size_t I>
	inline constexpr fp_t binomial_coefficient_v = ffactorial( N ) / ( ffactorial( I ) * ffactorial( N - I ) );

	// Implement the beizer calculation.
	//
	namespace impl
	{
		template<size_t N, typename Vec, typename... Vx>
		CONST_FN FORCE_INLINE inline constexpr Vec beizer_helper( const Vec& ti, const Vec& ts, const Vec& pi, const Vx&... px )
		{
			constexpr size_t I = N - sizeof...( Vx );
			Vec acc = binomial_coefficient_v<N, I> * ti * pi;
			if constexpr ( I == N )
				return acc;
			else
				return acc + beizer_helper<N>( ti * ts, ts, px... );
		}
	};
	template<typename V, typename... Vx>
	CONST_FN FORCE_INLINE inline constexpr V beizer( Float auto t, const V& p1, const Vx&... px )
	{
		using F = typename V::value_type;
		F ti = const_pow<sizeof...( Vx )>( 1 - t );
		F ts = t / ( 1 - t );
		return impl::beizer_helper<sizeof...( Vx )>( V::fill( ti ), V::fill( ts ), p1, px... );
	}
	template<typename V, typename C, Float F = typename V::value_type>
	CONST_FN FORCE_INLINE inline constexpr V beizer_dyn( F t, C&& container )
	{
		V accumulator = {};
		F n = ( ( int ) std::size( container ) );
		F i = 0;
		F tx = 1;
		F ti = rcp( 1 - t );
		F ts = t * ti;
		for ( const V& point : container )
		{
			accumulator += ti * point;
			ti *= ts;
			ti /= ++i;
			ti *= n - i;
			tx *= ( 1 - t );
		}
		return accumulator * tx;
	}

	// Catmull rom.
	//
	template<typename V, Float F = typename V::value_type>
	CONST_FN FORCE_INLINE inline F catmull_rom_t( F t_prev, const V& p0, const V& p1, F a = F( 0.5 ) )
	{
		F length = ( p1 - p0 ).length_sq();
		if ( xstd::is_consteval( a == F( 0.5 ) ) )
			return t_prev + fsqrt( fsqrt( length ) );
		else
			return t_prev + fpow( length, a * F( 0.5 ) );
	}
	template<typename V, Float F = typename V::value_type>
	CONST_FN FORCE_INLINE inline V catmull_rom( F t, const V& p0, const V& p1, const V& p2, const V& p3, F a = F( 0.5 ) )
	{
		constexpr F t0 = 0;
		F t1 = catmull_rom_t( t0, p0, p1, a );
		F t2 = catmull_rom_t( t1, p1, p2, a );
		F t3 = catmull_rom_t( t2, p2, p3, a );
		t = lerp( t1, t2, t );
		V a1 = ( t1 - t ) / ( t1 - t0 ) * p0 + ( t - t0 ) / ( t1 - t0 ) * p1;
		V a2 = ( t2 - t ) / ( t2 - t1 ) * p1 + ( t - t1 ) / ( t2 - t1 ) * p2;
		V a3 = ( t3 - t ) / ( t3 - t2 ) * p2 + ( t - t2 ) / ( t3 - t2 ) * p3;
		V b1 = ( t2 - t ) / ( t2 - t0 ) * a1 + ( t - t0 ) / ( t2 - t0 ) * a2;
		V b2 = ( t3 - t ) / ( t3 - t1 ) * a2 + ( t - t1 ) / ( t3 - t1 ) * a3;
		return ( t2 - t ) / ( t2 - t1 ) * b1 + ( t - t1 ) / ( t2 - t1 ) * b2;
	}

	// Conversion to barycentric coordinates.
	//
	template<typename V, Float F = typename V::value_type>
	CONST_FN FORCE_INLINE inline constexpr vec3_t<F> to_barycentric( const V& a, const V& b, const V& c, const V& p )
	{
		V v0 = b - a;
		V v1 = c - a;
		V v2 = p - a;
		F d00 = dot( v0, v0 );
		F d01 = dot( v0, v1 );
		F d11 = dot( v1, v1 );
		F d20 = dot( v2, v0 );
		F d21 = dot( v2, v1 );
		F rdenom = 1 / ( d00 * d11 - d01 * d01 );
		F v = ( d11 * d20 - d01 * d21 ) * rdenom;
		F w = ( d00 * d21 - d01 * d20 ) * rdenom;
		return vec3_t<F>{ 1 - ( v + w ), v, w };
	}
};
inline constexpr fp_t operator""_deg( long double deg ) { return xstd::math::to_rad( deg ); }
inline constexpr fp_t operator""_deg( unsigned long long int deg ) { return xstd::math::to_rad( deg ); }
#undef fp_t
#undef VEC_MATRIX_CONTRACT