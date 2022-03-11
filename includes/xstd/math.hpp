#pragma once
#include "type_helpers.hpp"
#include "xvector.hpp"
#include "formatting.hpp"
#include <math.h>
#include <array>

// [[Configuration]]
// XSTD_MATH_USE_XVEC: If set, enables acceleration of vector math using compiler support for vector extensions.
// XSTD_MATH_USE_X86INTRIN: If set, enabled optimization using intrinsics if not constexpr.
//
#ifndef XSTD_MATH_USE_XVEC
	#if XSTD_VECTOR_EXT
		#define XSTD_MATH_USE_XVEC 1
	#else
		#define XSTD_MATH_USE_XVEC 0
	#endif
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
	// Intrinsics helpers.
	//
#if XSTD_MATH_USE_X86INTRIN
	namespace impl
	{
		FORCE_INLINE static void load_vector( const void* _src, __m128& q )
		{
			auto src = ( const float* ) _src;
			q = _mm_loadu_ps( src );
		}
		FORCE_INLINE static void store_vector( void* _dst, const __m128& q )
		{
			auto dst = ( float* ) _dst;
			_mm_storeu_ps( dst, q );
		}
		FORCE_INLINE static void load_matrix( const void* _src, __m256& v01, __m256& v23 )
		{
			auto src = ( const float* ) _src;
			v01 = _mm256_loadu_ps( src );
			v23 = _mm256_loadu_ps( src + 8 );
		}
		FORCE_INLINE static void store_matrix( void* _dst, const __m256& v01, const __m256& v23 )
		{
			auto dst = ( float* ) _dst;
			_mm256_storeu_ps( dst , v01 );
			_mm256_storeu_ps( dst + 8, v23 );
		}
		FORCE_INLINE static void tranpose_inplace( __m256& v01, __m256& v23 )
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
	static constexpr float pi =      ( float ) 3.14159265358979323846;
	static constexpr float e =       ( float ) 2.71828182845904523536;
	static constexpr float flt_eps = __FLT_EPSILON__;
	static constexpr float flt_min = __FLT_MIN__;
	static constexpr float flt_max = __FLT_MAX__;

	// Fast floating point intrinsics.
	//
#if !defined(CLANG_COMPILER) || defined(__INTELLISENSE__)
	inline float fsqrt( float x ) { return sqrtf( x ); }
	inline float fsin( float x ) { return sinf( x ); }
	inline float fcos( float x ) { return cosf( x ); }
	inline float _fcopysign( float m, float s ) { return copysignf( m, s ); }
	inline float _fabs( float x ) { return fabsf( x ); }
	inline float _ffloor( float x ) { return floorf( x ); }
	inline float _fceil( float x ) { return ceilf( x ); }
	inline float _fround( float x ) { return roundf( x ); }
	inline float _ftrunc( float x ) { return truncf( x ); }
#else
	float fsqrt( float x ) asm( "llvm.sqrt.f32" );
	float fsin( float x ) asm( "llvm.sin.f32" );
	float fcos( float x ) asm( "llvm.cos.f32" );
	float _fcopysign( float m, float s ) asm( "llvm.copysign.f32" );
	float _fabs( float x ) asm( "llvm.fabs.f32" );
	float _ffloor( float x ) asm( "llvm.floor.f32" );
	float _fceil( float x ) asm( "llvm.ceil.f32" );
	float _fround( float x ) asm( "llvm.round.f32" );
	float _ftrunc( float x ) asm( "llvm.trunc.f32" );
#endif
	FORCE_INLINE inline constexpr float fabs( float a )
	{
		if ( !std::is_constant_evaluated() )
			return _fabs( a );
		if ( a < 0 ) a = -a;
		return a;
	}
	FORCE_INLINE inline constexpr float fcopysign( float m, float s ) 
	{
		if ( !std::is_constant_evaluated() )
			return _fcopysign( m, s );
		m = fabs( m );
		if ( s < 0 ) m = -m;
		return m;
	}
	FORCE_INLINE inline constexpr float ffloor( float x )
	{
		if ( !std::is_constant_evaluated() )
			return _ffloor( x );
		float d = x - int64_t( x );
		if ( d != 0 ) d = -1;
		return int64_t( x + d );
	}
	FORCE_INLINE inline constexpr float fceil( float x )
	{
		if ( !std::is_constant_evaluated() )
			return _fceil( x );
		float d = x - int64_t( x );
		if ( d != 0 ) d = +1;
		return int64_t( x + d );
	}
	FORCE_INLINE inline constexpr float fround( float x )
	{
		if ( !std::is_constant_evaluated() )
			return _fround( x );
		return float( int64_t( x + fcopysign( 0.5f, x ) ) );
	}
	FORCE_INLINE inline constexpr float ftrunc( float x )
	{
		if ( !std::is_constant_evaluated() )
			return _ftrunc( x );
		return float( int64_t( x ) );
	}
	FORCE_INLINE inline constexpr float fmod( float x, float y )
	{
		float m = 1.0f / y;
		return x - ftrunc( x * m ) * y;
	}

	// Implement sincos since NT CRT does not include it.
	//
	FORCE_INLINE inline std::pair<float, float> fsincos( float x ) 
	{
		float c = fcos( x );

		float s = fsqrt( 1 - c * c );
		x *= ( 1.0f / ( 2 * pi ) );
		x = x - ftrunc( x );
		x = ( x * ( 0.5f - fabsf( x ) ) );
		s = fcopysign( s, x );
		return { s, c };
	}

	// Implement non-referencing min/max/clamp.
	//
	FORCE_INLINE inline constexpr float fmin( float a, float b )
	{
		if ( b < a ) a = b;
		return a;
	}
	FORCE_INLINE inline constexpr float fmax( float a, float b )
	{
		if ( a < b ) a = b;
		return a;
	}
	FORCE_INLINE inline constexpr float fclamp( float v, float vmin, float vmax ) 
	{
		if ( v < vmin ) v = vmin;
		if ( v > vmax ) v = vmax;
		return v;
	}
	template<typename... Tx>
	FORCE_INLINE inline constexpr float fmin( float a, float b, float c, Tx... rest )
	{
		return fmin( fmin( a, b ), c, rest... );
	}
	template<typename... Tx>
	FORCE_INLINE inline constexpr float fmax( float a, float b, float c, Tx... rest )
	{
		return fmax( fmax( a, b ), c, rest... );
	}

	// Common vector operations with accceleration.
	//
	template<typename V>
	FORCE_INLINE inline constexpr float dot( const V& v1, const V& v2 )
	{
#if XSTD_MATH_USE_X86INTRIN
		if ( !std::is_constant_evaluated() )
		{
			if constexpr ( Same<typename V::element_type, float> && V::Length == 4 )
			{
				__m128 q1, q2;
				impl::load_vector( &v1, q1 );
				impl::load_vector( &v2, q2 );
				q1 = _mm_dp_ps( q1, q2, 0b1111'0001 );
				return q1[ 0 ];
			}
			else if constexpr ( Same<typename V::element_type, float> && V::Length == 3 )
			{
				__m128 q1, q2;
				impl::load_vector( &v1, q1 ); // Assumes .w is readable!
				impl::load_vector( &v2, q2 ); // Assumes .w is readable!
				q1 = _mm_dp_ps( q1, q2, 0b0111'0001 );
				return q1[ 0 ];
			}
		}
#endif
		return ( v1 * v2 ).reduce_add();
	}
	template<typename V>
	FORCE_INLINE inline V normalize( const V& vec )
	{
		return vec / fsqrt( fmax( flt_eps, vec.length_sq() ) );
	}
	template<typename V>
	FORCE_INLINE inline constexpr V lerp( const V& v1, const V& v2, float s )
	{
		return v1 + ( v2 - v1 ) * s;
	}

	// Define vector types.
	//
	namespace impl
	{
		template<typename T>
		struct vec2_t
		{
			using element_type = T;
			static constexpr size_t Length = 2;

			T x = 0;
			T y = 0;

			template<typename R>
			FORCE_INLINE inline constexpr vec2_t<R> cast() const { return { R( x ), R( y ) }; }

			template<typename X>
			FORCE_INLINE inline static constexpr vec2_t<T> from_xvec( X o ) { return vec2_t<T>{ o[ 0 ], o[ 1 ] }; }
			FORCE_INLINE inline constexpr xvec<T, 2> to_xvec() const noexcept { return { x, y }; }

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
				return ( &x )[ n ]; 
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
				return ( &x )[ n ]; 
			}
			FORCE_INLINE inline constexpr size_t count() const { return 2; }

			FORCE_INLINE inline static constexpr vec2_t<T> fill( T x ) { return { x, x }; }
			FORCE_INLINE inline auto tie() { return std::tie( x, y ); }
			FORCE_INLINE inline std::string to_string() const noexcept { return fmt::as_string( x, y ); }

#if XSTD_MATH_USE_XVEC
			FORCE_INLINE inline constexpr vec2_t operator+( T f ) const noexcept { return from_xvec( to_xvec() + f ); }
			FORCE_INLINE inline constexpr vec2_t operator-( T f ) const noexcept { return from_xvec( to_xvec() - f ); }
			FORCE_INLINE inline constexpr vec2_t operator*( T f ) const noexcept { return from_xvec( to_xvec() * f ); }
			FORCE_INLINE inline constexpr vec2_t operator/( T f ) const noexcept { return from_xvec( to_xvec() / f ); }
			FORCE_INLINE inline constexpr vec2_t operator+( const vec2_t& o ) const noexcept { return from_xvec( to_xvec() + o.to_xvec() ); }
			FORCE_INLINE inline constexpr vec2_t operator-( const vec2_t& o ) const noexcept { return from_xvec( to_xvec() - o.to_xvec() ); }
			FORCE_INLINE inline constexpr vec2_t operator*( const vec2_t& o ) const noexcept { return from_xvec( to_xvec() * o.to_xvec() ); }
			FORCE_INLINE inline constexpr vec2_t operator/( const vec2_t& o ) const noexcept { return from_xvec( to_xvec() / o.to_xvec() ); }
			FORCE_INLINE inline friend constexpr vec2_t operator+( T f, const vec2_t& o ) noexcept { return from_xvec( o.to_xvec() + f ); }
			FORCE_INLINE inline friend constexpr vec2_t operator-( T f, const vec2_t& o ) noexcept { return from_xvec( -o.to_xvec() + f ); }
			FORCE_INLINE inline friend constexpr vec2_t operator*( T f, const vec2_t& o ) noexcept { return from_xvec( o.to_xvec() * f ); }
			FORCE_INLINE inline friend constexpr vec2_t operator/( T f, const vec2_t& o ) noexcept { return from_xvec( fill( f ).to_xvec() / o.to_xvec() ); }
			FORCE_INLINE inline constexpr vec2_t operator-() const noexcept { return from_xvec( -to_xvec() ); };
			FORCE_INLINE inline constexpr T reduce_add() const noexcept { return vec::reduce_add( to_xvec() ); }
			FORCE_INLINE inline constexpr vec2_t min_element( const vec2_t& o ) const noexcept { return from_xvec( vec::min( to_xvec(), o.to_xvec() ) ); }
			FORCE_INLINE inline constexpr vec2_t max_element( const vec2_t& o ) const noexcept { return from_xvec( vec::max( to_xvec(), o.to_xvec() ) ); }
#else
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
			FORCE_INLINE inline constexpr vec2_t min_element( const vec2_t& o ) const noexcept { return { math::fmin( x, o.x ), math::fmin( y, o.y ) }; }
			FORCE_INLINE inline constexpr vec2_t max_element( const vec2_t& o ) const noexcept { return { math::fmax( x, o.x ), math::fmax( y, o.y ) }; }
#endif

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
			FORCE_INLINE inline float length() const noexcept { return fsqrt( length_sq() ); }
		};
		template<typename T>
		struct vec3_t
		{
			using element_type = T;
			static constexpr size_t Length = 3;

			T x = 0;
			T y = 0;
			T z = 0;

			template<typename R>
			FORCE_INLINE inline constexpr vec3_t<R> cast() const { return { R( x ), R( y ), R( z ) }; }

			FORCE_INLINE inline static constexpr vec3_t<T> from( const vec2_t<T>& v, T z = 0 ) { return { v.x, v.y, z }; }

			template<typename X>
			FORCE_INLINE inline static constexpr vec3_t<T> from_xvec( X o ) { return vec3_t<T>{ o[ 0 ],o[ 1 ],o[ 2 ] }; }
			FORCE_INLINE inline constexpr xvec<T, 3> to_xvec() const noexcept { return { x, y, z }; }

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
				return ( &x )[ n ];
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
				return ( &x )[ n ];
			}
			FORCE_INLINE inline constexpr size_t count() const { return 3; }

			FORCE_INLINE inline static constexpr vec3_t<T> fill( T x ) { return { x, x, x }; }
			FORCE_INLINE inline constexpr vec2_t<T> xy() const noexcept { return { x, y }; }
			FORCE_INLINE inline auto tie() { return std::tie( x, y, z ); }
			FORCE_INLINE inline std::string to_string() const noexcept { return fmt::as_string( x, y, z ); }
			
#if XSTD_MATH_USE_XVEC
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
			FORCE_INLINE inline constexpr vec3_t operator-() const noexcept { return from_xvec( -to_xvec() ); };
			FORCE_INLINE inline constexpr T reduce_add() const noexcept { return vec::reduce_add( to_xvec() ); }
			FORCE_INLINE inline constexpr vec3_t min_element( const vec3_t& o ) const noexcept { return from_xvec( vec::min( to_xvec(), o.to_xvec() ) ); }
			FORCE_INLINE inline constexpr vec3_t max_element( const vec3_t& o ) const noexcept { return from_xvec( vec::max( to_xvec(), o.to_xvec() ) ); }
#else
			FORCE_INLINE inline constexpr vec3_t operator+( T f ) const noexcept { return { x + f, y + f, z + f }; }
			FORCE_INLINE inline constexpr vec3_t operator-( T f ) const noexcept { return { x - f, y - f, z - f }; }
			FORCE_INLINE inline constexpr vec3_t operator*( T f ) const noexcept { return { x * f, y * f, z * f }; }
			FORCE_INLINE inline constexpr vec3_t operator/( T f ) const noexcept { return { x / f, y / f, z / f }; }
			FORCE_INLINE inline constexpr vec3_t operator+( const vec3_t& o ) const noexcept { return { x + o.x, y + o.y, z + o.z }; }
			FORCE_INLINE inline constexpr vec3_t operator-( const vec3_t& o ) const noexcept { return { x - o.x, y - o.y, z - o.z }; }
			FORCE_INLINE inline constexpr vec3_t operator*( const vec3_t& o ) const noexcept { return { x * o.x, y * o.y, z * o.z }; }
			FORCE_INLINE inline constexpr vec3_t operator/( const vec3_t& o ) const noexcept { return { x / o.x, y / o.y, z / o.z }; }
			FORCE_INLINE inline friend constexpr vec3_t operator+( T f, const vec3_t& o ) noexcept { return { f + o.x, f + o.y, f + o.z }; }
			FORCE_INLINE inline friend constexpr vec3_t operator-( T f, const vec3_t& o ) noexcept { return { f - o.x, f - o.y, f - o.z }; }
			FORCE_INLINE inline friend constexpr vec3_t operator*( T f, const vec3_t& o ) noexcept { return { f * o.x, f * o.y, f * o.z }; }
			FORCE_INLINE inline friend constexpr vec3_t operator/( T f, const vec3_t& o ) noexcept { return { f / o.x, f / o.y, f / o.z }; }
			FORCE_INLINE inline constexpr vec3_t operator-() const noexcept { return { -x, -y, -z }; };
			FORCE_INLINE inline constexpr T reduce_add() const noexcept { return x + y + z; }
			FORCE_INLINE inline constexpr vec3_t min_element( const vec3_t& o ) const noexcept { return { math::fmin( x, o.x ), math::fmin( y, o.y ), math::fmin( z, o.z ) }; }
			FORCE_INLINE inline constexpr vec3_t max_element( const vec3_t& o ) const noexcept { return { math::fmax( x, o.x ), math::fmax( y, o.y ), math::fmax( z, o.z ) }; }
#endif

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
			FORCE_INLINE inline float length() const noexcept { return fsqrt( length_sq() ); }
		};
		template<typename T>
		struct vec4_t
		{
			using element_type = T;
			static constexpr size_t Length = 4;

			T x = 0;
			T y = 0;
			T z = 0;
			T w = 0;

			template<typename R>
			FORCE_INLINE inline constexpr vec4_t<R> cast() const { return { R( x ), R( y ), R( z ), R( w ) }; }

			FORCE_INLINE inline static constexpr vec4_t<T> from( const vec2_t<T>& v, T z = 0, T w = 1 ) { return { v.x, v.y, z, w }; }
			FORCE_INLINE inline static constexpr vec4_t<T> from( const vec3_t<T>& v, T w = 1 ) { return { v.x, v.y, v.z, w }; }

			template<typename X>
			FORCE_INLINE inline static constexpr vec4_t<T> from_xvec( X o ) { return vec4_t<T>{ o[ 0 ], o[ 1 ], o[ 2 ], o[ 3 ] }; }
			FORCE_INLINE inline constexpr xvec<T, 4> to_xvec() const noexcept { return { x, y, z, w }; }

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
				return ( &x )[ n ];
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
				return ( &x )[ n ];
			}
			FORCE_INLINE inline constexpr size_t count() const { return 4; }

			FORCE_INLINE inline static constexpr vec4_t<T> fill( T x ) { return { x, x, x, x }; }
			FORCE_INLINE inline constexpr vec2_t<T> xy() const noexcept { return { x, y }; }
			FORCE_INLINE inline constexpr vec3_t<T> xyz() const noexcept { return { x, y, z }; }
			FORCE_INLINE inline auto tie() { return std::tie( x, y, z, w ); }
			FORCE_INLINE inline std::string to_string() const noexcept { return fmt::as_string( x, y, z, w ); }

#if XSTD_MATH_USE_XVEC
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
			FORCE_INLINE inline constexpr vec4_t operator-() const noexcept { return from_xvec( -to_xvec() ); };
			FORCE_INLINE inline constexpr T reduce_add() const noexcept { return vec::reduce_add( to_xvec() ); }
			FORCE_INLINE inline constexpr vec4_t min_element( const vec4_t& o ) const noexcept { return from_xvec( vec::min( to_xvec(), o.to_xvec() ) ); }
			FORCE_INLINE inline constexpr vec4_t max_element( const vec4_t& o ) const noexcept { return from_xvec( vec::max( to_xvec(), o.to_xvec() ) ); }
#else
			FORCE_INLINE inline constexpr vec4_t operator+( T f ) const noexcept { return { x + f, y + f, z + f, w + f }; }
			FORCE_INLINE inline constexpr vec4_t operator-( T f ) const noexcept { return { x - f, y - f, z - f, w - f }; }
			FORCE_INLINE inline constexpr vec4_t operator*( T f ) const noexcept { return { x * f, y * f, z * f, w * f }; }
			FORCE_INLINE inline constexpr vec4_t operator/( T f ) const noexcept { return { x / f, y / f, z / f, w / f }; }
			FORCE_INLINE inline constexpr vec4_t operator+( const vec4_t& o ) const noexcept { return { x + o.x, y + o.y, z + o.z, w + o.w }; }
			FORCE_INLINE inline constexpr vec4_t operator-( const vec4_t& o ) const noexcept { return { x - o.x, y - o.y, z - o.z, w - o.w }; }
			FORCE_INLINE inline constexpr vec4_t operator*( const vec4_t& o ) const noexcept { return { x * o.x, y * o.y, z * o.z, w * o.w }; }
			FORCE_INLINE inline constexpr vec4_t operator/( const vec4_t& o ) const noexcept { return { x / o.x, y / o.y, z / o.z, w / o.w }; }
			FORCE_INLINE inline friend constexpr vec4_t operator+( T f, const vec4_t& o ) noexcept { return { f + o.x, f + o.y, f + o.z, f + o.w }; }
			FORCE_INLINE inline friend constexpr vec4_t operator-( T f, const vec4_t& o ) noexcept { return { f - o.x, f - o.y, f - o.z, f - o.w }; }
			FORCE_INLINE inline friend constexpr vec4_t operator*( T f, const vec4_t& o ) noexcept { return { f * o.x, f * o.y, f * o.z, f * o.w }; }
			FORCE_INLINE inline friend constexpr vec4_t operator/( T f, const vec4_t& o ) noexcept { return { f / o.x, f / o.y, f / o.z, f / o.w }; }
			FORCE_INLINE inline constexpr vec4_t operator-() const noexcept { return { -x, -y, -z, -w }; };
			FORCE_INLINE inline constexpr T reduce_add() const noexcept { return x + y + z + w; }
			FORCE_INLINE inline constexpr vec4_t min_element( const vec4_t& o ) const noexcept { return { math::fmin( x, o.x ), math::fmin( y, o.y ), math::fmin( z, o.z ), math::fmin( w, o.w ) }; }
			FORCE_INLINE inline constexpr vec4_t max_element( const vec4_t& o ) const noexcept { return { math::fmax( x, o.x ), math::fmax( y, o.y ), math::fmax( z, o.z ), math::fmax( w, o.w ) }; }
#endif

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
			FORCE_INLINE inline float length() const noexcept { return fsqrt( length_sq() ); }
		};
	};

	using ivec2 =      impl::vec2_t<int32_t>;
	using ivec3 =      impl::vec3_t<int32_t>;
	using ivec4 =      impl::vec4_t<int32_t>;
	using vec2 =       impl::vec2_t<float>;
	using vec3 =       impl::vec3_t<float>;
	using vec4 =       impl::vec4_t<float>;
	using quaternion = vec4;

	// Implement certain float funcs for vectors as well.
	//
	FORCE_INLINE inline constexpr vec4 vec_abs( const vec4& vec ) { return { fabs( vec.x ), fabs( vec.y ), fabs( vec.z ), fabs( vec.w ) }; }
	FORCE_INLINE inline constexpr vec4 vec_ceil( const vec4& vec ) { return { fceil( vec.x ), fceil( vec.y ), fceil( vec.z ), fceil( vec.w ) }; }
	FORCE_INLINE inline constexpr vec4 vec_floor( const vec4& vec ) { return { ffloor( vec.x ), ffloor( vec.y ), ffloor( vec.z ), ffloor( vec.w ) }; }
	FORCE_INLINE inline constexpr vec4 vec_round( const vec4& vec ) { return { fround( vec.x ), fround( vec.y ), fround( vec.z ), fround( vec.w ) }; }
	FORCE_INLINE inline constexpr vec4 vec_trunc( const vec4& vec ) { return { ftrunc( vec.x ), ftrunc( vec.y ), ftrunc( vec.z ), ftrunc( vec.w ) }; }
	FORCE_INLINE inline vec4 vec_sqrt( const vec4& vec ) { return { fsqrt( vec.x ), fsqrt( vec.y ), fsqrt( vec.z ), fsqrt( vec.w ) }; }

	FORCE_INLINE inline constexpr vec3 vec_abs( const vec3& vec ) { return { fabs( vec.x ), fabs( vec.y ), fabs( vec.z ) }; }
	FORCE_INLINE inline constexpr vec3 vec_ceil( const vec3& vec ) { return { fceil( vec.x ), fceil( vec.y ), fceil( vec.z ) }; }
	FORCE_INLINE inline constexpr vec3 vec_floor( const vec3& vec ) { return { ffloor( vec.x ), ffloor( vec.y ), ffloor( vec.z ) }; }
	FORCE_INLINE inline constexpr vec3 vec_round( const vec3& vec ) { return { fround( vec.x ), fround( vec.y ), fround( vec.z ) }; }
	FORCE_INLINE inline constexpr vec3 vec_trunc( const vec3& vec ) { return { ftrunc( vec.x ), ftrunc( vec.y ), ftrunc( vec.z ) }; }
	FORCE_INLINE inline vec3 vec_sqrt( const vec3& vec ) { return { fsqrt( vec.x ), fsqrt( vec.y ), fsqrt( vec.z ) }; }

	FORCE_INLINE inline constexpr vec2 vec_abs( const vec2& vec ) { return { fabs( vec.x ), fabs( vec.y ) }; }
	FORCE_INLINE inline constexpr vec2 vec_ceil( const vec2& vec ) { return { fceil( vec.x ), fceil( vec.y ) }; }
	FORCE_INLINE inline constexpr vec2 vec_floor( const vec2& vec ) { return { ffloor( vec.x ), ffloor( vec.y ) }; }
	FORCE_INLINE inline constexpr vec2 vec_round( const vec2& vec ) { return { fround( vec.x ), fround( vec.y ) }; }
	FORCE_INLINE inline constexpr vec2 vec_trunc( const vec2& vec ) { return { ftrunc( vec.x ), ftrunc( vec.y ) }; }
	FORCE_INLINE inline vec2 vec_sqrt( const vec2& vec ) { return { fsqrt( vec.x ), fsqrt( vec.y ) }; }

	// Extended vector helpers.
	//
	template<typename V>
	FORCE_INLINE inline constexpr V vec_max( const V& v1, const V& v2 )
	{
		return v1.max_element( v2 );
	}
	template<typename V>
	FORCE_INLINE inline constexpr V vec_min( const V& v1, const V& v2 )
	{
		return v1.min_element( v2 );
	}
	template<typename V, typename... Tx>
	FORCE_INLINE inline constexpr V vec_max( const V& v1, const V& v2, const V& v3, Tx&&... rest )
	{
		return vec_max( vec_max( v1, v2 ), v3, std::forward<Tx>( rest )... );
	}
	template<typename V, typename... Tx>
	FORCE_INLINE inline constexpr V vec_min( const V& v1, const V& v2, const V& v3, Tx&&... rest )
	{
		return vec_min( vec_min( v1, v2 ), v3, std::forward<Tx>( rest )... );
	}
	FORCE_INLINE inline constexpr vec3 cross( const vec3& v1, const vec3& v2 ) 
	{
		return {
			v1.y * v2.z - v1.z * v2.y,
			v1.z * v2.x - v1.x * v2.z,
			v1.x * v2.y - v1.y * v2.x
		};
	}
	FORCE_INLINE inline constexpr vec4 cross( const vec4& v1, const vec4& v2, const vec4& v3 )
	{
		return {
		     v1.y * ( v2.z * v3.w - v3.z * v2.w ) - v1.z * ( v2.y * v3.w - v3.y * v2.w ) + v1.w * ( v2.y * v3.z - v2.z * v3.y ),
		  -( v1.x * ( v2.z * v3.w - v3.z * v2.w ) - v1.z * ( v2.x * v3.w - v3.x * v2.w ) + v1.w * ( v2.x * v3.z - v3.x * v2.z ) ),
		     v1.x * ( v2.y * v3.w - v3.y * v2.w ) - v1.y * ( v2.x * v3.w - v3.x * v2.w ) + v1.w * ( v2.x * v3.y - v3.x * v2.y ),
		  -( v1.x * ( v2.y * v3.z - v3.y * v2.z ) - v1.y * ( v2.x * v3.z - v3.x * v2.z ) + v1.z * ( v2.x * v3.y - v3.x * v2.y ) )
		};
	}
	FORCE_INLINE inline constexpr quaternion inverse( const quaternion& q )
	{
#if XSTD_MATH_USE_X86INTRIN
		if ( !std::is_constant_evaluated() && !xstd::is_consteval( q ) )
		{
			__m128 a;
			impl::load_vector( &q, a );

			__v4su c{ 0x80000000, 0x80000000, 0x80000000, 0x00000000 };
			a = _mm_xor_ps( a, ( __m128 ) c );
			return impl::store_vector<vec4>( a );
		}
#endif
		return { -q.x, -q.y, -q.z, q.w };
	}

	// Define matrix type.
	//
	struct matrix4x4
	{
		std::array<vec4, 4> m = { vec4{} };

		// Default construction.
		//
		constexpr matrix4x4() = default;

		// Construction by 4 vectors.
		//
		FORCE_INLINE inline constexpr matrix4x4( vec4 a, vec4 b, vec4 c, vec4 d ) : m{ a, b, c, d } {}
		
		// Construction by all values.
		//
		FORCE_INLINE inline constexpr matrix4x4( float _00, float _01, float _02, float _03,
									float _10, float _11, float _12, float _13,
									float _20, float _21, float _22, float _23,
									float _30, float _31, float _32, float _33 ) : m{ 
			vec4{ _00, _01, _02, _03 },
			vec4{ _10, _11, _12, _13 },
			vec4{ _20, _21, _22, _23 },
			vec4{ _30, _31, _32, _33 }
		} {}

		// Default copy.
		//
		constexpr matrix4x4( const matrix4x4& ) = default;
		constexpr matrix4x4& operator=( const matrix4x4& ) = default;

		// Identity matrix.
		//
		inline static constexpr std::array<vec4, 4> identity_value = {
			vec4{ 1, 0, 0, 0 },
			vec4{ 0, 1, 0, 0 },
			vec4{ 0, 0, 1, 0 },
			vec4{ 0, 0, 0, 1 }
		};
		FORCE_INLINE inline static constexpr matrix4x4 identity() noexcept { return { identity_value[ 0 ], identity_value[ 1 ], identity_value[ 2 ], identity_value[ 3 ] }; }

		// Matrix mulitplication.
		//
		FORCE_INLINE inline constexpr matrix4x4 operator*( const matrix4x4& other ) const noexcept
		{
#if XSTD_MATH_USE_X86INTRIN
			if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) )
			{
				matrix4x4 result;

				__m256 v01, v23, q01, q23;
				impl::load_matrix( &m, v01, v23 );
				impl::load_matrix( &other.m, q01, q23 );
				impl::tranpose_inplace( q01, q23 );

				__m256 vxx = _mm256_permute2f128_ps( q01, q01, 0b0000'0000 );
				__m256 vyy = _mm256_permute2f128_ps( q01, q01, 0b0001'0001 );
				__m256 vzz = _mm256_permute2f128_ps( q23, q23, 0b0000'0000 );
				__m256 vww = _mm256_permute2f128_ps( q23, q23, 0b0001'0001 );

				__m256 r01 = _mm256_dp_ps( v01, vxx, 0b11110001 );
				__m256 r23 = _mm256_dp_ps( v23, vxx, 0b11110001 );
				r01 = _mm256_xor_ps( r01, _mm256_dp_ps( v01, vyy, 0b11110010 ) );
				r23 = _mm256_xor_ps( r23, _mm256_dp_ps( v23, vyy, 0b11110010 ) );
				r01 = _mm256_xor_ps( r01, _mm256_dp_ps( v01, vzz, 0b11110100 ) );
				r23 = _mm256_xor_ps( r23, _mm256_dp_ps( v23, vzz, 0b11110100 ) );
				r01 = _mm256_xor_ps( r01, _mm256_dp_ps( v01, vww, 0b11111000 ) );
				r23 = _mm256_xor_ps( r23, _mm256_dp_ps( v23, vww, 0b11111000 ) );

				impl::store_matrix( &result, r01, r23 );
				return result;
			}
#endif
			matrix4x4 result = *this;
			result.m[ 0 ] = other * result.m[ 0 ];
			result.m[ 1 ] = other * result.m[ 1 ];
			result.m[ 2 ] = other * result.m[ 2 ];
			result.m[ 3 ] = other * result.m[ 3 ];
			return result;
		}
		FORCE_INLINE inline constexpr matrix4x4& operator*=( const matrix4x4& other ) noexcept { m = ( *this * other ).m; return *this; }

		// Vector transformation.
		//
		FORCE_INLINE inline constexpr vec4 operator*( const vec4& v ) const noexcept
		{
#if XSTD_MATH_USE_X86INTRIN
			if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) )
			{
				vec4 result;

				__m128 vx = _mm_broadcast_ss( &v.x );
				__m128 vy = _mm_broadcast_ss( &v.y );
				__m128 vz = _mm_broadcast_ss( &v.z );
				__m128 vw = _mm_broadcast_ss( &v.w );

				__m256 vxy = _mm256_castps128_ps256( vx );
				vxy = _mm256_insertf128_ps( vxy, vy, 1 );
				__m256 vzw = _mm256_castps128_ps256( vz );
				vzw = _mm256_insertf128_ps( vzw, vw, 1 );

				__m256 vrlh = _mm256_mul_ps( vxy, _mm256_loadu_ps( ( const float* ) &m[ 0 ] ) );
				vrlh = _mm256_fmadd_ps( vzw, _mm256_loadu_ps( ( const float* ) &m[ 2 ] ), vrlh );

				auto vrl = _mm256_extractf128_ps( vrlh, 0 );
				auto vrh = _mm256_extractf128_ps( vrlh, 1 );
				impl::store_vector( &result, _mm_add_ps( vrl, vrh ) );
				return result;
			}
#endif
			auto vx = m[ 0 ] * v.x;
			auto vy = m[ 1 ] * v.y;
			auto vz = m[ 2 ] * v.z;
			auto vw = m[ 3 ] * v.w;
			return vx + vy + vz + vw;
		}
		FORCE_INLINE inline constexpr vec3 operator*( const vec3& v ) const noexcept
		{
			vec4 res = *this * vec4{ v.x, v.y, v.z, 1 };
			return { res.x, res.y, res.z };
		}

		// Offset by scalar.
		//
		FORCE_INLINE inline constexpr matrix4x4 operator+( float v ) const noexcept
		{
#if XSTD_MATH_USE_X86INTRIN
			if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) )
			{
				__m256 f = _mm256_broadcast_ss( &v );
				
				__m256 a, b;
				impl::load_matrix( &m, a, b );
				
				a = _mm256_add_ps( a, f );
				b = _mm256_add_ps( b, f );

				matrix4x4 result;
				impl::store_matrix( &result, a, b );
				return result;
			}
#endif

			matrix4x4 result;
			for ( size_t i = 0; i != 4; i++ )
				result[ i ] = m[ i ] + v;
			return result;
		}
		FORCE_INLINE inline constexpr matrix4x4 operator-( float v ) const noexcept { return *this + ( -v ); }
		FORCE_INLINE inline constexpr matrix4x4& operator-=( float v ) noexcept { m = ( *this - v ).m; return *this; }
		FORCE_INLINE inline constexpr matrix4x4& operator+=( float v ) noexcept { m = ( *this + v ).m; return *this; }

		// Scale by scalar.
		//
		FORCE_INLINE inline constexpr matrix4x4 operator*( float v ) const noexcept
		{
#if XSTD_MATH_USE_X86INTRIN
			if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) )
			{
				__m256 f = _mm256_broadcast_ss( &v );
				
				__m256 a, b;
				impl::load_matrix( &m, a, b );
				
				a = _mm256_mul_ps( a, f );
				b = _mm256_mul_ps( b, f );

				matrix4x4 result;
				impl::store_matrix( &result, a, b );
				return result;
			}
#endif

			matrix4x4 result;
			for ( size_t i = 0; i != 4; i++ )
				result[ i ] = m[ i ] * v;
			return result;
		}
		FORCE_INLINE inline constexpr matrix4x4 operator/( float v ) const noexcept { return *this * ( 1.0f / v ); }
		FORCE_INLINE inline constexpr matrix4x4& operator/=( float v ) noexcept { m = ( *this / v ).m; return *this; }
		FORCE_INLINE inline constexpr matrix4x4& operator*=( float v ) noexcept { m = ( *this * v ).m; return *this; }

		// Unaries.
		//
		FORCE_INLINE inline constexpr matrix4x4 operator-() const noexcept 
		{
#if XSTD_MATH_USE_X86INTRIN
			if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) )
			{
				__m256 a, b;
				impl::load_matrix( &m, a, b );

				
				__v8su c{ 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
				a = _mm256_xor_ps( a, ( __m256 ) c );
				b = _mm256_xor_ps( b, ( __m256 ) c );
				return impl::store_matrix<matrix4x4>( a, b );
			}
#endif
			return *this * -1.0f; 
		}
		FORCE_INLINE inline constexpr matrix4x4 operator+() const noexcept { return *this; }

		// Forward indexing.
		//
		FORCE_INLINE inline constexpr vec4& operator[]( size_t n ) noexcept { return m[ n ]; }
		FORCE_INLINE inline constexpr const vec4& operator[]( size_t n ) const noexcept { return m[ n ]; }
		FORCE_INLINE inline constexpr float& operator()( size_t i, size_t j ) noexcept { return m[ i ][ j ]; }
		FORCE_INLINE inline constexpr const float& operator()( size_t i, size_t j ) const noexcept { return m[ i ][ j ]; }

		// Hashing, serialization, comparison and string conversion.
		//
		FORCE_INLINE inline constexpr auto operator<=>( const matrix4x4& ) const noexcept = default;
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
		FORCE_INLINE inline constexpr matrix4x4 transpose() const noexcept
		{
			matrix4x4 out;
#if XSTD_MATH_USE_X86INTRIN
			if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) )
			{
				__m256 v01, v23;
				impl::load_matrix( &m, v01, v23 );
				impl::tranpose_inplace( v01, v23 );
				impl::store_matrix( &out, v01, v23 );
				return out;
			}
#endif
			for ( size_t i = 0; i != 4; i++ )
				for ( size_t j = 0; j != 4; j++ )
					out[ i ][ j ] = m[ j ][ i ];
			return out;
		}

		// Determinant.
		//
		FORCE_INLINE inline constexpr float determinant() const noexcept
		{
			vec4 minor = cross(
				vec4{ m[ 0 ][ 0 ], m[ 1 ][ 0 ], m[ 2 ][ 0 ], m[ 3 ][ 0 ] },
				vec4{ m[ 0 ][ 1 ], m[ 1 ][ 1 ], m[ 2 ][ 1 ], m[ 3 ][ 1 ] },
				vec4{ m[ 0 ][ 2 ], m[ 1 ][ 2 ], m[ 2 ][ 2 ], m[ 3 ][ 2 ] }
			);
			return -( m[ 0 ][ 3 ] * minor.x + m[ 1 ][ 3 ] * minor.y + m[ 2 ][ 3 ] * minor.z + m[ 3 ][ 3 ] * minor.w );
		}
	};

	// Matrix inversion.
	//
	inline constexpr matrix4x4 inverse( const matrix4x4& m, float& det )
	{
		float t[ 3 ] = {};
		matrix4x4 out = {};

		t[ 0 ] =          m[ 2 ][ 2 ] * m[ 3 ][ 3 ] - m[ 2 ][ 3 ] * m[ 3 ][ 2 ];
		t[ 1 ] =          m[ 1 ][ 2 ] * m[ 3 ][ 3 ] - m[ 1 ][ 3 ] * m[ 3 ][ 2 ];
		t[ 2 ] =          m[ 1 ][ 2 ] * m[ 2 ][ 3 ] - m[ 1 ][ 3 ] * m[ 2 ][ 2 ];
		out[ 0 ][ 0 ] =   m[ 1 ][ 1 ] * t[ 0 ] - m[ 2 ][ 1 ] * t[ 1 ] + m[ 3 ][ 1 ] * t[ 2 ];
		out[ 1 ][ 0 ] =  -m[ 1 ][ 0 ] * t[ 0 ] + m[ 2 ][ 0 ] * t[ 1 ] - m[ 3 ][ 0 ] * t[ 2 ];

		t[ 0 ] =          m[ 1 ][ 0 ] * m[ 2 ][ 1 ] - m[ 2 ][ 0 ] * m[ 1 ][ 1 ];
		t[ 1 ] =          m[ 1 ][ 0 ] * m[ 3 ][ 1 ] - m[ 3 ][ 0 ] * m[ 1 ][ 1 ];
		t[ 2 ] =          m[ 2 ][ 0 ] * m[ 3 ][ 1 ] - m[ 3 ][ 0 ] * m[ 2 ][ 1 ];
		out[ 2 ][ 0 ] =   m[ 3 ][ 3 ] * t[ 0 ] - m[ 2 ][ 3 ] * t[ 1 ] + m[ 1 ][ 3 ] * t[ 2 ];
		out[ 3 ][ 0 ] =  -m[ 3 ][ 2 ] * t[ 0 ] + m[ 2 ][ 2 ] * t[ 1 ] - m[ 1 ][ 2 ] * t[ 2 ];

		t[ 0 ] =          m[ 2 ][ 2 ] * m[ 3 ][ 3 ] - m[ 2 ][ 3 ] * m[ 3 ][ 2 ];
		t[ 1 ] =          m[ 0 ][ 2 ] * m[ 3 ][ 3 ] - m[ 0 ][ 3 ] * m[ 3 ][ 2 ];
		t[ 2 ] =          m[ 0 ][ 2 ] * m[ 2 ][ 3 ] - m[ 0 ][ 3 ] * m[ 2 ][ 2 ];
		out[ 0 ][ 1 ] =  -m[ 0 ][ 1 ] * t[ 0 ] + m[ 2 ][ 1 ] * t[ 1 ] - m[ 3 ][ 1 ] * t[ 2 ];
		out[ 1 ][ 1 ] =   m[ 0 ][ 0 ] * t[ 0 ] - m[ 2 ][ 0 ] * t[ 1 ] + m[ 3 ][ 0 ] * t[ 2 ];

		t[ 0 ] =          m[ 0 ][ 0 ] * m[ 2 ][ 1 ] - m[ 2 ][ 0 ] * m[ 0 ][ 1 ];
		t[ 1 ] =          m[ 3 ][ 0 ] * m[ 0 ][ 1 ] - m[ 0 ][ 0 ] * m[ 3 ][ 1 ];
		t[ 2 ] =          m[ 2 ][ 0 ] * m[ 3 ][ 1 ] - m[ 3 ][ 0 ] * m[ 2 ][ 1 ];
		out[ 2 ][ 1 ] =  -m[ 3 ][ 3 ] * t[ 0 ] - m[ 2 ][ 3 ] * t[ 1 ] - m[ 0 ][ 3 ] * t[ 2 ];
		out[ 3 ][ 1 ] =   m[ 3 ][ 2 ] * t[ 0 ] + m[ 2 ][ 2 ] * t[ 1 ] + m[ 0 ][ 2 ] * t[ 2 ];

		t[ 0 ] =          m[ 1 ][ 2 ] * m[ 3 ][ 3 ] - m[ 1 ][ 3 ] * m[ 3 ][ 2 ];
		t[ 1 ] =          m[ 0 ][ 2 ] * m[ 3 ][ 3 ] - m[ 0 ][ 3 ] * m[ 3 ][ 2 ];
		t[ 2 ] =          m[ 0 ][ 2 ] * m[ 1 ][ 3 ] - m[ 0 ][ 3 ] * m[ 1 ][ 2 ];
		out[ 0 ][ 2 ] =   m[ 0 ][ 1 ] * t[ 0 ] - m[ 1 ][ 1 ] * t[ 1 ] + m[ 3 ][ 1 ] * t[ 2 ];
		out[ 1 ][ 2 ] =  -m[ 0 ][ 0 ] * t[ 0 ] + m[ 1 ][ 0 ] * t[ 1 ] - m[ 3 ][ 0 ] * t[ 2 ];

		t[ 0 ] =          m[ 0 ][ 0 ] * m[ 1 ][ 1 ] - m[ 1 ][ 0 ] * m[ 0 ][ 1 ];
		t[ 1 ] =          m[ 3 ][ 0 ] * m[ 0 ][ 1 ] - m[ 0 ][ 0 ] * m[ 3 ][ 1 ];
		t[ 2 ] =          m[ 1 ][ 0 ] * m[ 3 ][ 1 ] - m[ 3 ][ 0 ] * m[ 1 ][ 1 ];
		out[ 2 ][ 2 ] =   m[ 3 ][ 3 ] * t[ 0 ] + m[ 1 ][ 3 ] * t[ 1 ] + m[ 0 ][ 3 ] * t[ 2 ];
		out[ 3 ][ 2 ] =  -m[ 3 ][ 2 ] * t[ 0 ] - m[ 1 ][ 2 ] * t[ 1 ] - m[ 0 ][ 2 ] * t[ 2 ];

		t[ 0 ] =          m[ 1 ][ 2 ] * m[ 2 ][ 3 ] - m[ 1 ][ 3 ] * m[ 2 ][ 2 ];
		t[ 1 ] =          m[ 0 ][ 2 ] * m[ 2 ][ 3 ] - m[ 0 ][ 3 ] * m[ 2 ][ 2 ];
		t[ 2 ] =          m[ 0 ][ 2 ] * m[ 1 ][ 3 ] - m[ 0 ][ 3 ] * m[ 1 ][ 2 ];
		out[ 0 ][ 3 ] =  -m[ 0 ][ 1 ] * t[ 0 ] + m[ 1 ][ 1 ] * t[ 1 ] - m[ 2 ][ 1 ] * t[ 2 ];
		out[ 1 ][ 3 ] =   m[ 0 ][ 0 ] * t[ 0 ] - m[ 1 ][ 0 ] * t[ 1 ] + m[ 2 ][ 0 ] * t[ 2 ];
		out[ 2 ][ 3 ] =  -m[ 0 ][ 0 ] * ( m[ 1 ][ 1 ] * m[ 2 ][ 3 ] - m[ 1 ][ 3 ] * m[ 2 ][ 1 ] ) +
								m[ 1 ][ 0 ] * ( m[ 0 ][ 1 ] * m[ 2 ][ 3 ] - m[ 0 ][ 3 ] * m[ 2 ][ 1 ] ) -
								m[ 2 ][ 0 ] * ( m[ 0 ][ 1 ] * m[ 1 ][ 3 ] - m[ 0 ][ 3 ] * m[ 1 ][ 1 ] );
		out[ 3 ][ 3 ] =   m[ 0 ][ 0 ] * ( m[ 1 ][ 1 ] * m[ 2 ][ 2 ] - m[ 1 ][ 2 ] * m[ 2 ][ 1 ] ) -
								m[ 1 ][ 0 ] * ( m[ 0 ][ 1 ] * m[ 2 ][ 2 ] - m[ 0 ][ 2 ] * m[ 2 ][ 1 ] ) +
								m[ 2 ][ 0 ] * ( m[ 0 ][ 1 ] * m[ 1 ][ 2 ] - m[ 0 ][ 2 ] * m[ 1 ][ 1 ] );

		det = ( m[ 0 ][ 0 ] * out[ 0 ][ 0 ] + m[ 0 ][ 1 ] * out[ 1 ][ 0 ] + m[ 0 ][ 2 ] * out[ 2 ][ 0 ] + m[ 0 ][ 3 ] * out[ 3 ][ 0 ] );
		return out / det;
	}
	FORCE_INLINE inline constexpr matrix4x4 inverse( const matrix4x4& m ) { float tmp = 0; return inverse( m, tmp ); }

	// Matrix decomposition helpers.
	//
	FORCE_INLINE inline constexpr vec3 matrix_to_translation( const matrix4x4& mat )
	{
		return mat[ 3 ].xyz();
	}
	FORCE_INLINE inline vec3 matrix_to_scale( const matrix4x4& mat )
	{
		return { mat[ 0 ].xyz().length(), mat[ 1 ].xyz().length(), mat[ 2 ].xyz().length() };
	}
	FORCE_INLINE inline vec3 matrix_to_forward( const matrix4x4& mat )
	{
		return normalize( mat[ 0 ].xyz() );
	}
	FORCE_INLINE inline vec3 matrix_to_right( const matrix4x4& mat )
	{
		return normalize( mat[ 1 ].xyz() );
	}
	FORCE_INLINE inline vec3 matrix_to_up( const matrix4x4& mat )
	{
		return normalize( mat[ 2 ].xyz() );
	}

	// Normalizes a matrix to have no scale.
	//
	FORCE_INLINE inline matrix4x4 matrix_normalize( matrix4x4 mat )
	{
		vec4 scales = 1.0f / vec_sqrt( vec4{ mat[ 0 ].length_sq(), mat[ 1 ].length_sq(), mat[ 2 ].length_sq(), 0.0f } );
		mat[ 0 ] *= scales[ 0 ];
		mat[ 1 ] *= scales[ 1 ];
		mat[ 2 ] *= scales[ 2 ];
		return mat;
	}

	// Rotation around an axis.
	// - Axis must be normalized.
	//
	FORCE_INLINE inline quaternion rotate_q( float theta, const vec3& axis )
	{
		auto [s, c] = fsincos( theta / 2 );
		return {
			s * axis.x,
			s * axis.y,
			s * axis.z,
			c
		};
	}
	FORCE_INLINE inline matrix4x4 rotate_v( float theta, const vec3& axis )
	{
		auto [sangle, cangle] = fsincos( theta );
		float cdiff = 1.0f - cangle;

		matrix4x4 out;
		out[ 0 ][ 0 ] = cdiff * axis.x * axis.x + cangle;
		out[ 1 ][ 0 ] = cdiff * axis.x * axis.y - sangle * axis.z;
		out[ 2 ][ 0 ] = cdiff * axis.x * axis.z + sangle * axis.y;
		out[ 3 ][ 0 ] = 0.0f;
		out[ 0 ][ 1 ] = cdiff * axis.y * axis.x + sangle * axis.z;
		out[ 1 ][ 1 ] = cdiff * axis.y * axis.y + cangle;
		out[ 2 ][ 1 ] = cdiff * axis.y * axis.z - sangle * axis.x;
		out[ 3 ][ 1 ] = 0.0f;
		out[ 0 ][ 2 ] = cdiff * axis.z * axis.x - sangle * axis.y;
		out[ 1 ][ 2 ] = cdiff * axis.z * axis.y + sangle * axis.x;
		out[ 2 ][ 2 ] = cdiff * axis.z * axis.z + cangle;
		out[ 3 ][ 2 ] = 0.0f;
		out[ 0 ][ 3 ] = 0.0f;
		out[ 1 ][ 3 ] = 0.0f;
		out[ 2 ][ 3 ] = 0.0f;
		out[ 3 ][ 3 ] = 1.0f;
		return out;
	}

	// Combining rotations.
	//
	FORCE_INLINE inline constexpr quaternion rotate_by( const quaternion& q2, const quaternion& q1 ) 
	{
		return {
			 q1.x * q2.w + q1.y * q2.z - q1.z * q2.y + q1.w * q2.x,
			-q1.x * q2.z + q1.y * q2.w + q1.z * q2.x + q1.w * q2.y,
			 q1.x * q2.y - q1.y * q2.x + q1.z * q2.w + q1.w * q2.z,
			-q1.x * q2.x - q1.y * q2.y - q1.z * q2.z + q1.w * q2.w
		};
	}
	FORCE_INLINE inline constexpr matrix4x4 rotate_by( const matrix4x4& m1, const matrix4x4& m2 )
	{
		return m1 * m2;
	}

	// Rotating a vector.
	//
	FORCE_INLINE inline constexpr vec3 rotate_by( const vec3& v, const matrix4x4& m ) { return m * v; }
	FORCE_INLINE inline constexpr vec4 rotate_by( const vec4& v, const matrix4x4& m ) { return m * v; }
	FORCE_INLINE inline constexpr vec3 rotate_by( const vec3& v, const quaternion& q )
	{
		vec3 u{ q.x, q.y, q.z };
		return 
			u * 2.0f * dot( u, v ) + 
			v * ( q.w * q.w - dot( u, u ) ) + 
			cross( u, v ) * 2.0f * q.w;
	}

	// Translation and scaling matrices.
	//
	FORCE_INLINE inline constexpr matrix4x4 matrix_translation( const vec3& p )
	{
		matrix4x4 out = matrix4x4::identity();
		out[ 3 ][ 0 ] = p.x;
		out[ 3 ][ 1 ] = p.y;
		out[ 3 ][ 2 ] = p.z;
		return out;
	}
	FORCE_INLINE inline constexpr matrix4x4 matrix_scaling( const vec3& p )
	{
		matrix4x4 out = matrix4x4::identity();
		out[ 0 ][ 0 ] = p.x;
		out[ 1 ][ 1 ] = p.y;
		out[ 2 ][ 2 ] = p.z;
		return out;
	}

	// Spherical quaternion interpolation.
	//
	FORCE_INLINE inline constexpr quaternion quaternion_slerp( const quaternion& pq1, const quaternion& pq2, float t )
	{
		float epsilon = fcopysign( +1.0f, dot( pq1, pq2 ) );
		return {
			( 1.0f - t ) * pq1.x + epsilon * t * pq2.x,
			( 1.0f - t ) * pq1.y + epsilon * t * pq2.y,
			( 1.0f - t ) * pq1.z + epsilon * t * pq2.z,
			( 1.0f - t ) * pq1.w + epsilon * t * pq2.w,
		};
	}
	// Define helpers for radian / degrees.
	//
	template<typename T = float>
	FORCE_INLINE inline constexpr auto to_rad( T deg ) { return deg * float( pi / 180.0f ); }
	template<typename T = float>
	FORCE_INLINE inline constexpr auto to_deg( T rad ) { return rad * float( 180.0f / pi ); }

	// Default axis used for euler world.
	//
	inline constexpr vec3 up =      { 0, 0, 1 };
	inline constexpr vec3 right =   { 0, 1, 0 };
	inline constexpr vec3 forward = { 1, 0, 0 };

	// Matrix to Euler/Quaternion/Direction.
	//
	FORCE_INLINE inline vec3 matrix_to_euler( const matrix4x4& mat )
	{
		// Thanks to Eigen.
		//
		vec3 res = {};
		float rsum = fsqrt( ( mat( 0, 0 ) * mat( 0, 0 ) + mat( 0, 1 ) * mat( 0, 1 ) + mat( 1, 2 ) * mat( 1, 2 ) + mat( 2, 2 ) * mat( 2, 2 ) ) / 2 );
		res.x = atan2f( -mat( 0, 2 ), rsum );
		if ( rsum > 4 * flt_eps )
		{
			res.z = atan2f( mat( 1, 2 ), mat( 2, 2 ) );
			res.y = atan2f( mat( 0, 1 ), mat( 0, 0 ) );
		}
		else if ( -mat( 0, 2 ) > 0 )
		{
			float spos = mat( 1, 0 ) + -mat( 2, 1 );
			float cpos = mat( 1, 1 ) + mat( 2, 0 );
			res.z = atan2f( spos, cpos );
			res.y = 0;
		}
		else
		{
			float sneg = -( mat( 2, 1 ) + mat( 1, 0 ) );
			float cneg = mat( 1, 1 ) + -mat( 2, 0 );
			res.z = atan2f( sneg, cneg );
			res.y = 0;
		}
		return res;
	}
	FORCE_INLINE inline quaternion matrix_to_quaternion( const matrix4x4& mat )
	{
		float trace = mat( 0, 0 ) + mat( 1, 1 ) + mat( 2, 2 ) + 1.0f;
		if ( trace > 1.0f )
		{
			float s = 2.0f * fsqrt( trace );
			return {
				( mat( 1, 2 ) - mat( 2, 1 ) ) / s,
				( mat( 2, 0 ) - mat( 0, 2 ) ) / s,
				( mat( 0, 1 ) - mat( 1, 0 ) ) / s,
				0.25f * s
			};
		}
		else if ( mat( 0, 0 ) > mat( 1, 1 ) && mat( 0, 0 ) > mat( 2, 2 ) )
		{
			float s = 2.0f * fsqrt( 1.0f + mat( 0, 0 ) - mat( 1, 1 ) - mat( 2, 2 ) );
			return {
				0.25f * s,
				( mat( 0, 1 ) + mat( 1, 0 ) ) / s,
				( mat( 0, 2 ) + mat( 2, 0 ) ) / s,
				( mat( 1, 2 ) - mat( 2, 1 ) ) / s,
			};
		}
		else if ( mat( 1, 1 ) > mat( 2, 2 ) )
		{
			float s = 2.0f * fsqrt( 1.0f + mat( 1, 1 ) - mat( 0, 0 ) - mat( 2, 2 ) );
			return {
				( mat( 0, 1 ) + mat( 1, 0 ) ) / s,
				0.25f * s,
				( mat( 1, 2 ) + mat( 2, 1 ) ) / s,
				( mat( 2, 0 ) - mat( 0, 2 ) ) / s,
			};
		}
		else
		{
			float s = 2.0f * fsqrt( 1.0f + mat( 2, 2 ) - mat( 0, 0 ) - mat( 1, 1 ) );
			return {
				( mat( 0, 2 ) + mat( 2, 0 ) ) / s,
				( mat( 1, 2 ) + mat( 2, 1 ) ) / s,
				0.25f * s,
				( mat( 0, 1 ) - mat( 1, 0 ) ) / s,
			};
		}
	}
	FORCE_INLINE inline vec3 matrix_to_direction( const matrix4x4& mat )
	{
		return matrix_to_forward( mat );
	}

	// Direction to Euler/Quaternion/Matrix.
	//
	FORCE_INLINE inline matrix4x4 direction_to_matrix( const vec3& direction )
	{
		vec3 up_vec, right_vec;

		if ( ( direction.x * direction.x + direction.y * direction.y ) > 2 * flt_eps )
		{
			up_vec =    cross( up, direction );
			right_vec = cross( direction, up_vec );
		}
		else
		{
			up_vec =    {  0, 1, 0 };
			right_vec = { -1, 0, 0 };
		}

		matrix4x4 result = {
			vec4::from( direction, 0 ),
			vec4::from( up_vec,    0 ),
			vec4::from( right_vec, 0 ),
			vec4{ 0, 0, 0,         1 },
		};
		return matrix_normalize( result );
	}
	FORCE_INLINE inline vec3 direction_to_euler( const vec3& direction )
	{
		return vec3{ -asinf( direction.z / direction.length() ), atan2f( direction.y, direction.x ), 0 };
	}
	FORCE_INLINE inline quaternion direction_to_quaternion( const vec3& direction )
	{
		return matrix_to_quaternion( direction_to_matrix( direction ) );
	}

	// Quaternion to Euler/Matrix/Direction.
	//
	FORCE_INLINE inline vec3 quaternion_to_euler( const quaternion& q )
	{
		float sinr_cosp = 2.0f * ( q.w * q.x + q.y * q.z );
		float cosr_cosp = 1.0f - 2.0f * ( q.x * q.x + q.y * q.y );
		float sinp = 2.0f * ( q.w * q.y - q.z * q.x );
		float siny_cosp = 2.0f * ( q.w * q.z + q.x * q.y );
		float cosy_cosp = 1.0f - 2.0f * ( q.y * q.y + q.z * q.z );
		return {
			asinf( fclamp( sinp, -1.0f, +1.0f ) ),
			atan2f( siny_cosp, cosy_cosp ),
			atan2f( sinr_cosp, cosr_cosp )
		};
	}
	FORCE_INLINE inline constexpr matrix4x4 quaternion_to_matrix( const quaternion& rot )
	{
		matrix4x4 out = {};
		out[ 0 ][ 0 ] = 1.0f - 2.0f * ( rot.y * rot.y + rot.z * rot.z );
		out[ 0 ][ 1 ] = 2.0f * ( rot.x * rot.y + rot.z * rot.w );
		out[ 0 ][ 2 ] = 2.0f * ( rot.x * rot.z - rot.y * rot.w );
		out[ 1 ][ 0 ] = 2.0f * ( rot.x * rot.y - rot.z * rot.w );
		out[ 1 ][ 1 ] = 1.0f - 2.0f * ( rot.x * rot.x + rot.z * rot.z );
		out[ 1 ][ 2 ] = 2.0f * ( rot.y * rot.z + rot.x * rot.w );
		out[ 2 ][ 0 ] = 2.0f * ( rot.x * rot.z + rot.y * rot.w );
		out[ 2 ][ 1 ] = 2.0f * ( rot.y * rot.z - rot.x * rot.w );
		out[ 2 ][ 2 ] = 1.0f - 2.0f * ( rot.x * rot.x + rot.y * rot.y );
		out[ 3 ][ 3 ] = 1.0f;
		return out;
	}
	FORCE_INLINE inline vec3 quaternion_to_direction( const quaternion& q )
	{
		return rotate_by( forward, q );
	}


	// Euler to Matrix/Quaternion/Direction.
	//
	FORCE_INLINE inline matrix4x4 euler_to_matrix( const vec3& rot )
	{
		auto [spitch, cpitch] = fsincos( rot.x );
		auto [syaw,   cyaw] =   fsincos( rot.y );
		auto [sroll,  croll] =  fsincos( rot.z );

		matrix4x4 out = {};
		out[ 0 ][ 0 ] = cpitch * cyaw;
		out[ 0 ][ 1 ] = cpitch * syaw;
		out[ 0 ][ 2 ] = -spitch;
		out[ 0 ][ 3 ] = 0.0f;
		out[ 1 ][ 0 ] = sroll * spitch * cyaw - croll * syaw;
		out[ 1 ][ 1 ] = sroll * spitch * syaw + croll * cyaw;
		out[ 1 ][ 2 ] = sroll * cpitch;
		out[ 1 ][ 3 ] = 0.0f;
		out[ 2 ][ 0 ] = croll * spitch * cyaw + sroll * syaw;
		out[ 2 ][ 1 ] = croll * spitch * syaw - sroll * cyaw;
		out[ 2 ][ 2 ] = croll * cpitch;
		out[ 2 ][ 3 ] = 0.0f;
		out[ 3 ][ 0 ] = 0.0f;
		out[ 3 ][ 1 ] = 0.0f;
		out[ 3 ][ 2 ] = 0.0f;
		out[ 3 ][ 3 ] = 1.0f;
		return out;
	}
	FORCE_INLINE inline quaternion euler_to_quaternion( const vec3& rot )
	{
		auto [spitch, cpitch] = fsincos( rot.x * 0.5f );
		auto [syaw,   cyaw] =   fsincos( rot.y * 0.5f );
		auto [sroll,  croll] =  fsincos( rot.z * 0.5f );

		return {
			cyaw * cpitch * sroll - syaw * spitch * croll,
			syaw * cpitch * sroll + cyaw * spitch * croll,
			syaw * cpitch * croll - cyaw * spitch * sroll,
			cyaw * cpitch * croll + syaw * spitch * sroll
		};
	}
	FORCE_INLINE inline vec3 euler_to_direction( const vec3& rot )
	{
		auto [spitch, cpitch] = fsincos( rot.x );
		auto [syaw,   cyaw] =   fsincos( rot.y );
		return {
			cpitch * cyaw,
			cpitch * syaw,
			-spitch
		};
	}

	// View helpers.
	//
	template<bool LeftHanded = true>
	FORCE_INLINE inline matrix4x4 look_at( vec3 eye, vec3 dir, vec3 up )
	{
		constexpr float m = LeftHanded ? 1 : -1;

		vec3 right = cross( up, dir );
		up = cross( dir, right );
		right = normalize( right );
		up = normalize( up );

		matrix4x4 out = {};
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
		out[ 0 ][ 3 ] = 0.0f;
		out[ 1 ][ 3 ] = 0.0f;
		out[ 2 ][ 3 ] = 0.0f;
		out[ 3 ][ 3 ] = 1.0f;
		return out;
	}
	template<bool LeftHanded = true>
	FORCE_INLINE inline constexpr matrix4x4 perspective_tan_fov_xy( float tfx, float tfy, float zn, float zf )
	{
		constexpr float m = LeftHanded ? 1 : -1;
		matrix4x4 out = matrix4x4::identity();
		out[ 0 ][ 0 ] = 1.0f / tfx;
		out[ 1 ][ 1 ] = 1.0f / tfy;
		out[ 2 ][ 2 ] = m * zf / ( zf - zn );
		out[ 2 ][ 3 ] = m * 1.0f;
		out[ 3 ][ 2 ] = ( zf * zn ) / ( zn - zf );
		out[ 3 ][ 3 ] = 0.0f;
		return out;
	}
	template<bool LeftHanded = true>
	FORCE_INLINE inline matrix4x4 perspective_fov_x( float fov, float aspect, float zn, float zf )
	{
		float t = tanf( fov / 2.0f );
		return perspective_tan_fov_xy<LeftHanded>( t, aspect * t, zn, zf );
	}
	template<bool LeftHanded = true>
	FORCE_INLINE inline matrix4x4 perspective_fov_y( float fov, float aspect, float zn, float zf )
	{
		float t = tanf( fov / 2.0f );
		return perspective_tan_fov_xy<LeftHanded>( aspect * t, t, zn, zf );
	}
};
inline constexpr float operator""_deg( long double deg ) { return xstd::math::to_rad( deg ); }
inline constexpr float operator""_deg( unsigned long long int deg ) { return xstd::math::to_rad( deg ); }