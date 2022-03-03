#pragma once
#include "type_helpers.hpp"
#include "xvector.hpp"
#include "formatting.hpp"
#include <math.h>
#include <array>

// [[Configuration]]
// XSTD_MATH_USE_XVEC: If set, enables acceleration of vector math using compiler support for vector extensions.
//
#ifndef XSTD_MATH_USE_XVEC
	#if XSTD_VECTOR_EXT
		#define XSTD_MATH_USE_XVEC 1
	#else
		#define XSTD_MATH_USE_XVEC 0
	#endif
#endif

// Defines useful math primitives.
//
namespace xstd::math
{
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
#else
	float fsqrt( float x ) asm( "llvm.sqrt.f32" );
	float fsin( float x ) asm( "llvm.sin.f32" );
	float fcos( float x ) asm( "llvm.cos.f32" );
	float _fcopysign( float m, float s ) asm( "llvm.copysign.f32" );
	float _fabs( float x ) asm( "llvm.fabs.f32" );
	float _ffloor( float x ) asm( "llvm.floor.f32" );
	float _fceil( float x ) asm( "llvm.ceil.f32" );
	float _fround( float x ) asm( "llvm.round.f32" );
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
	FORCE_INLINE inline constexpr float fmod( float x, float y )
	{
		float m = 1.0f / y;
		return x - float( int64_t( x * m ) ) * y;
	}

	// Implement sincos since NT CRT does not include it.
	//
	FORCE_INLINE inline std::pair<float, float> fsincos( float x ) 
	{
		float c = fcos( x );
		float s = fsin( x );
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
	// Define vector types.
	//
	namespace impl
	{
		template<typename T>
		struct vec2_t
		{
			T x = 0;
			T y = 0;

			template<typename R>
			FORCE_INLINE inline constexpr vec2_t<R> cast() const { return { R( x ), R( y ) }; }

			FORCE_INLINE inline static constexpr vec2_t<T> from_xvec( xvec<T, 3> o ) { return vec2_t<T>{ o[ 0 ], o[ 1 ] }; }
			FORCE_INLINE inline constexpr xvec<T, 3> to_xvec() const noexcept { return { x, y }; }

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
			FORCE_INLINE inline constexpr T length_sq() const noexcept { return ( *this * *this ).reduce_add(); }
			FORCE_INLINE inline float length() const noexcept { return fsqrt( length_sq() ); }
		};
		template<typename T>
		struct vec3_t
		{
			T x = 0;
			T y = 0;
			T z = 0;

			template<typename R>
			FORCE_INLINE inline constexpr vec3_t<R> cast() const { return { R( x ), R( y ), R( z ) }; }

			FORCE_INLINE inline static constexpr vec3_t<T> from( const vec2_t<T>& v, T z = 0 ) { return { v.x, v.y, z }; }

			FORCE_INLINE inline static constexpr vec3_t<T> from_xvec( xvec<T, 3> o ) { return vec3_t<T>{ o[ 0 ],o[ 1 ],o[ 2 ] }; }
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
			FORCE_INLINE inline constexpr T operator[]( size_t n ) const
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
			FORCE_INLINE inline constexpr T length_sq() const noexcept { return ( *this * *this ).reduce_add(); }
			FORCE_INLINE inline float length() const noexcept { return fsqrt( length_sq() ); }
		};
		template<typename T>
		struct vec4_t
		{
			T x = 0;
			T y = 0;
			T z = 0;
			T w = 0;

			template<typename R>
			FORCE_INLINE inline constexpr vec4_t<R> cast() const { return { R( x ), R( y ), R( z ), R( w ) }; }

			FORCE_INLINE inline static constexpr vec4_t<T> from( const vec2_t<T>& v, T z = 0, T w = 1 ) { return { v.x, v.y, z, w }; }
			FORCE_INLINE inline static constexpr vec4_t<T> from( const vec3_t<T>& v, T w = 1 ) { return { v.x, v.y, v.z, w }; }

			FORCE_INLINE inline static constexpr vec4_t<T> from_xvec( xvec<T, 4> o ) { return vec4_t<T>{ o[ 0 ],o[ 1 ],o[ 2 ],o[ 3 ] }; }
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
			FORCE_INLINE inline constexpr T operator[]( size_t n ) const
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
			FORCE_INLINE inline constexpr T length_sq() const noexcept { return ( *this * *this ).reduce_add(); }
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

	FORCE_INLINE inline constexpr vec3 vec_abs( const vec3& vec ) { return { fabs( vec.x ), fabs( vec.y ), fabs( vec.z ) }; }
	FORCE_INLINE inline constexpr vec3 vec_ceil( const vec3& vec ) { return { fceil( vec.x ), fceil( vec.y ), fceil( vec.z ) }; }
	FORCE_INLINE inline constexpr vec3 vec_floor( const vec3& vec ) { return { ffloor( vec.x ), ffloor( vec.y ), ffloor( vec.z ) }; }
	FORCE_INLINE inline constexpr vec3 vec_round( const vec3& vec ) { return { fround( vec.x ), fround( vec.y ), fround( vec.z ) }; }

	FORCE_INLINE inline constexpr vec2 vec_abs( const vec2& vec ) { return { fabs( vec.x ), fabs( vec.y ) }; }
	FORCE_INLINE inline constexpr vec2 vec_ceil( const vec2& vec ) { return { fceil( vec.x ), fceil( vec.y ) }; }
	FORCE_INLINE inline constexpr vec2 vec_floor( const vec2& vec ) { return { ffloor( vec.x ), ffloor( vec.y ) }; }
	FORCE_INLINE inline constexpr vec2 vec_round( const vec2& vec ) { return { fround( vec.x ), fround( vec.y ) }; }

	// Extended vector helpers.
	//
	template<typename V>
	FORCE_INLINE inline V normalize( const V& vec ) 
	{
		return vec / fsqrt( fmax( flt_eps, vec.length_sq() ) );
	}
	template<typename V>
	FORCE_INLINE inline constexpr float dot( const V& v1, const V& v2 )
	{
		return ( v1 * v2 ).reduce_add();
	}
	template<typename V>
	FORCE_INLINE inline constexpr V lerp( const V& v1, const V& v2, float s )
	{
		return v1 + ( v2 - v1 ) * s;
	}
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
			const vec4 o0 = other.m[ 0 ];
			const vec4 o1 = other.m[ 1 ];
			const vec4 o2 = other.m[ 2 ];
			const vec4 o3 = other.m[ 3 ];

			vec4 s0 = m[ 0 ];
			vec4 s1 = m[ 1 ];
			vec4 s2 = m[ 2 ];
			vec4 s3 = m[ 3 ];

			s0 = s0[ 0 ] * o0 + s0[ 1 ] * o1 + s0[ 2 ] * o2 + s0[ 3 ] * o3;
			s1 = s1[ 0 ] * o0 + s1[ 1 ] * o1 + s1[ 2 ] * o2 + s1[ 3 ] * o3;
			s2 = s2[ 0 ] * o0 + s2[ 1 ] * o1 + s2[ 2 ] * o2 + s2[ 3 ] * o3;
			s3 = s3[ 0 ] * o0 + s3[ 1 ] * o1 + s3[ 2 ] * o2 + s3[ 3 ] * o3;
			
			return { s0, s1, s2, s3 };
		}
		FORCE_INLINE inline constexpr matrix4x4& operator*=( const matrix4x4& other ) noexcept { m = ( *this * other ).m; return *this; }

		// Vector transformation.
		//
		FORCE_INLINE inline constexpr vec4 operator*( const vec4& v ) const noexcept
		{
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

		// Forward indexing.
		//
		FORCE_INLINE inline constexpr vec4& operator[]( size_t n ) noexcept { return m[ n ]; }
		FORCE_INLINE inline constexpr const vec4& operator[]( size_t n ) const noexcept { return m[ n ]; }

		// Default comparison.
		//
		FORCE_INLINE inline constexpr auto operator<=>( const matrix4x4& ) const noexcept = default;
	};

	// Extended matrix helpers.
	//
	FORCE_INLINE inline constexpr matrix4x4 transpose( const matrix4x4& m ) 
	{
		matrix4x4 out;
		for ( size_t i = 0; i != 4; i++ )
			for ( size_t j = 0; j != 4; j++ )
				out[ i ][ j ] = m[ j ][ i ];
		return out;
	}
	FORCE_INLINE inline constexpr float determinant( const matrix4x4& m ) 
	{
		vec4 minor = cross( 
			vec4{ m[ 0 ][ 0 ], m[ 1 ][ 0 ], m[ 2 ][ 0 ], m[ 3 ][ 0 ] }, 
			vec4{ m[ 0 ][ 1 ], m[ 1 ][ 1 ], m[ 2 ][ 1 ], m[ 3 ][ 1 ] },
			vec4{ m[ 0 ][ 2 ], m[ 1 ][ 2 ], m[ 2 ][ 2 ], m[ 3 ][ 2 ] }
		);
		return -( m[ 0 ][ 3 ] * minor.x + m[ 1 ][ 3 ] * minor.y + m[ 2 ][ 3 ] * minor.z + m[ 3 ][ 3 ] * minor.w );
	}
	inline constexpr matrix4x4 inverse( const matrix4x4& m, float& det )
	{
		float t[ 3 ] = {};
		float v[ 16 ] = {};
		t[ 0 ] =   m[ 2 ][ 2 ] * m[ 3 ][ 3 ] - m[ 2 ][ 3 ] * m[ 3 ][ 2 ];
		t[ 1 ] =   m[ 1 ][ 2 ] * m[ 3 ][ 3 ] - m[ 1 ][ 3 ] * m[ 3 ][ 2 ];
		t[ 2 ] =   m[ 1 ][ 2 ] * m[ 2 ][ 3 ] - m[ 1 ][ 3 ] * m[ 2 ][ 2 ];
		v[ 0 ] =   m[ 1 ][ 1 ] * t[ 0 ] - m[ 2 ][ 1 ] * t[ 1 ] + m[ 3 ][ 1 ] * t[ 2 ];
		v[ 4 ] =  -m[ 1 ][ 0 ] * t[ 0 ] + m[ 2 ][ 0 ] * t[ 1 ] - m[ 3 ][ 0 ] * t[ 2 ];
		t[ 0 ] =   m[ 1 ][ 0 ] * m[ 2 ][ 1 ] - m[ 2 ][ 0 ] * m[ 1 ][ 1 ];
		t[ 1 ] =   m[ 1 ][ 0 ] * m[ 3 ][ 1 ] - m[ 3 ][ 0 ] * m[ 1 ][ 1 ];
		t[ 2 ] =   m[ 2 ][ 0 ] * m[ 3 ][ 1 ] - m[ 3 ][ 0 ] * m[ 2 ][ 1 ];
		v[ 8 ] =   m[ 3 ][ 3 ] * t[ 0 ] - m[ 2 ][ 3 ] * t[ 1 ] + m[ 1 ][ 3 ] * t[ 2 ];
		v[ 12 ] = -m[ 3 ][ 2 ] * t[ 0 ] + m[ 2 ][ 2 ] * t[ 1 ] - m[ 1 ][ 2 ] * t[ 2 ];
		t[ 0 ] =   m[ 2 ][ 2 ] * m[ 3 ][ 3 ] - m[ 2 ][ 3 ] * m[ 3 ][ 2 ];
		t[ 1 ] =   m[ 0 ][ 2 ] * m[ 3 ][ 3 ] - m[ 0 ][ 3 ] * m[ 3 ][ 2 ];
		t[ 2 ] =   m[ 0 ][ 2 ] * m[ 2 ][ 3 ] - m[ 0 ][ 3 ] * m[ 2 ][ 2 ];
		v[ 1 ] =  -m[ 0 ][ 1 ] * t[ 0 ] + m[ 2 ][ 1 ] * t[ 1 ] - m[ 3 ][ 1 ] * t[ 2 ];
		v[ 5 ] =   m[ 0 ][ 0 ] * t[ 0 ] - m[ 2 ][ 0 ] * t[ 1 ] + m[ 3 ][ 0 ] * t[ 2 ];
		t[ 0 ] =   m[ 0 ][ 0 ] * m[ 2 ][ 1 ] - m[ 2 ][ 0 ] * m[ 0 ][ 1 ];
		t[ 1 ] =   m[ 3 ][ 0 ] * m[ 0 ][ 1 ] - m[ 0 ][ 0 ] * m[ 3 ][ 1 ];
		t[ 2 ] =   m[ 2 ][ 0 ] * m[ 3 ][ 1 ] - m[ 3 ][ 0 ] * m[ 2 ][ 1 ];
		v[ 9 ] =  -m[ 3 ][ 3 ] * t[ 0 ] - m[ 2 ][ 3 ] * t[ 1 ] - m[ 0 ][ 3 ] * t[ 2 ];
		v[ 13 ] =  m[ 3 ][ 2 ] * t[ 0 ] + m[ 2 ][ 2 ] * t[ 1 ] + m[ 0 ][ 2 ] * t[ 2 ];
		t[ 0 ] =   m[ 1 ][ 2 ] * m[ 3 ][ 3 ] - m[ 1 ][ 3 ] * m[ 3 ][ 2 ];
		t[ 1 ] =   m[ 0 ][ 2 ] * m[ 3 ][ 3 ] - m[ 0 ][ 3 ] * m[ 3 ][ 2 ];
		t[ 2 ] =   m[ 0 ][ 2 ] * m[ 1 ][ 3 ] - m[ 0 ][ 3 ] * m[ 1 ][ 2 ];
		v[ 2 ] =   m[ 0 ][ 1 ] * t[ 0 ] - m[ 1 ][ 1 ] * t[ 1 ] + m[ 3 ][ 1 ] * t[ 2 ];
		v[ 6 ] =  -m[ 0 ][ 0 ] * t[ 0 ] + m[ 1 ][ 0 ] * t[ 1 ] - m[ 3 ][ 0 ] * t[ 2 ];
		t[ 0 ] =   m[ 0 ][ 0 ] * m[ 1 ][ 1 ] - m[ 1 ][ 0 ] * m[ 0 ][ 1 ];
		t[ 1 ] =   m[ 3 ][ 0 ] * m[ 0 ][ 1 ] - m[ 0 ][ 0 ] * m[ 3 ][ 1 ];
		t[ 2 ] =   m[ 1 ][ 0 ] * m[ 3 ][ 1 ] - m[ 3 ][ 0 ] * m[ 1 ][ 1 ];
		v[ 10 ] =  m[ 3 ][ 3 ] * t[ 0 ] + m[ 1 ][ 3 ] * t[ 1 ] + m[ 0 ][ 3 ] * t[ 2 ];
		v[ 14 ] = -m[ 3 ][ 2 ] * t[ 0 ] - m[ 1 ][ 2 ] * t[ 1 ] - m[ 0 ][ 2 ] * t[ 2 ];
		t[ 0 ] =   m[ 1 ][ 2 ] * m[ 2 ][ 3 ] - m[ 1 ][ 3 ] * m[ 2 ][ 2 ];
		t[ 1 ] =   m[ 0 ][ 2 ] * m[ 2 ][ 3 ] - m[ 0 ][ 3 ] * m[ 2 ][ 2 ];
		t[ 2 ] =   m[ 0 ][ 2 ] * m[ 1 ][ 3 ] - m[ 0 ][ 3 ] * m[ 1 ][ 2 ];
		v[ 3 ] =  -m[ 0 ][ 1 ] * t[ 0 ] + m[ 1 ][ 1 ] * t[ 1 ] - m[ 2 ][ 1 ] * t[ 2 ];
		v[ 7 ] =   m[ 0 ][ 0 ] * t[ 0 ] - m[ 1 ][ 0 ] * t[ 1 ] + m[ 2 ][ 0 ] * t[ 2 ];
		v[ 11 ] = -m[ 0 ][ 0 ] * ( m[ 1 ][ 1 ] * m[ 2 ][ 3 ] - m[ 1 ][ 3 ] * m[ 2 ][ 1 ] ) +
			        m[ 1 ][ 0 ] * ( m[ 0 ][ 1 ] * m[ 2 ][ 3 ] - m[ 0 ][ 3 ] * m[ 2 ][ 1 ] ) -
			        m[ 2 ][ 0 ] * ( m[ 0 ][ 1 ] * m[ 1 ][ 3 ] - m[ 0 ][ 3 ] * m[ 1 ][ 1 ] );
		v[ 15 ] =  m[ 0 ][ 0 ] * ( m[ 1 ][ 1 ] * m[ 2 ][ 2 ] - m[ 1 ][ 2 ] * m[ 2 ][ 1 ] ) -
			        m[ 1 ][ 0 ] * ( m[ 0 ][ 1 ] * m[ 2 ][ 2 ] - m[ 0 ][ 2 ] * m[ 2 ][ 1 ] ) +
			        m[ 2 ][ 0 ] * ( m[ 0 ][ 1 ] * m[ 1 ][ 2 ] - m[ 0 ][ 2 ] * m[ 1 ][ 1 ] );
		det = ( m[ 0 ][ 0 ] * v[ 0 ] + m[ 0 ][ 1 ] * v[ 4 ] + m[ 0 ][ 2 ] * v[ 8 ] + m[ 0 ][ 3 ] * v[ 12 ] );
		float idet = 1.0f / det;
		matrix4x4 out;
		for ( size_t i = 0; i != 4; i++ )
			for ( size_t j = 0; j != 4; j++ )
				out[ i ][ j ] = v[ 4 * i + j ] * idet;
		return out;
	}
	FORCE_INLINE inline constexpr matrix4x4 inverse( const matrix4x4& m ) { float tmp = 0; return inverse( m, tmp ); }

	// Define helpers for radian / degrees.
	//
	template<typename T = float>
	FORCE_INLINE inline constexpr auto to_rad( T deg ) { return deg * float( pi / 180.0f ); }
	template<typename T = float>
	FORCE_INLINE inline constexpr auto to_deg( T rad ) { return rad * float( 180.0f / pi ); }

	// Rotation -> (Pitch, Yaw, Roll).
	//
	FORCE_INLINE inline vec3 to_euler( const matrix4x4& m )
	{
		return {
			asinf( -m[ 2 ][ 1 ] ),
			atan2f( m[ 2 ][ 0 ], m[ 2 ][ 2 ] ),
			atan2f( m[ 0 ][ 1 ], m[ 1 ][ 1 ] )
		};
	}
	FORCE_INLINE inline vec3 to_euler( const quaternion& q )
	{
		float sinr_cosp = 2.0f * ( q.w * q.x + q.y * q.z );
		float cosr_cosp = 1.0f - 2.0f * ( q.x * q.x + q.y * q.y );
		float sinp =      2.0f * ( q.w * q.y - q.z * q.x );
		float siny_cosp = 2.0f * ( q.w * q.z + q.x * q.y );
		float cosy_cosp = 1.0f - 2.0f * ( q.y * q.y + q.z * q.z );
		return {
			atan2f( sinr_cosp, cosr_cosp ),
			asinf( fclamp( sinp, -1.0f, +1.0f ) ),
			atan2f( siny_cosp, cosy_cosp ),
		};
	}

	// Rotation around an axis.
	//
	FORCE_INLINE inline quaternion rotate_q( float theta, const vec3& axis )
	{
		vec3 a = normalize( axis );
		auto [s, c] = fsincos( theta / 2 );
		return {
			s * a.x,
			s * a.y,
			s * a.z,
			c
		};
	}
	FORCE_INLINE inline matrix4x4 rotate_v( float theta, const vec3& axis )
	{
		vec3 a = normalize( axis );
		auto [sangle, cangle] = fsincos( theta );
		float cdiff = 1.0f - cangle;

		matrix4x4 out;
		out[ 0 ][ 0 ] = cdiff * a.x * a.x + cangle;
		out[ 1 ][ 0 ] = cdiff * a.x * a.y - sangle * a.z;
		out[ 2 ][ 0 ] = cdiff * a.x * a.z + sangle * a.y;
		out[ 3 ][ 0 ] = 0.0f;
		out[ 0 ][ 1 ] = cdiff * a.y * a.x + sangle * a.z;
		out[ 1 ][ 1 ] = cdiff * a.y * a.y + cangle;
		out[ 2 ][ 1 ] = cdiff * a.y * a.z - sangle * a.x;
		out[ 3 ][ 1 ] = 0.0f;
		out[ 0 ][ 2 ] = cdiff * a.z * a.x - sangle * a.y;
		out[ 1 ][ 2 ] = cdiff * a.z * a.y + sangle * a.x;
		out[ 2 ][ 2 ] = cdiff * a.z * a.z + cangle;
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

	// Changing the rotation types.
	//
	FORCE_INLINE inline quaternion quaternion_rotation( const matrix4x4& rot )
	{
		//return quaternion_rotation( to_euler( rot ) );

		float trace = rot[ 0 ][ 0 ] + rot[ 1 ][ 1 ] + rot[ 2 ][ 2 ] + 1.0f;
		if ( trace > 1.0f )
		{
			float s = 2.0f * fsqrt( trace );
			return {
				( rot[ 1 ][ 2 ] - rot[ 2 ][ 1 ] ) / s,
				( rot[ 2 ][ 0 ] - rot[ 0 ][ 2 ] ) / s,
				( rot[ 0 ][ 1 ] - rot[ 1 ][ 0 ] ) / s,
				0.25f * s
			};
		}
		else if ( rot[ 0 ][ 0 ] > rot[ 1 ][ 1 ] && rot[ 0 ][ 0 ] > rot[ 2 ][ 2 ] )
		{
			float s = 2.0f * fsqrt( 1.0f + rot[ 0 ][ 0 ] - rot[ 1 ][ 1 ] - rot[ 2 ][ 2 ] );
			return {
				0.25f * s,
				( rot[ 0 ][ 1 ] + rot[ 1 ][ 0 ] ) / s,
				( rot[ 0 ][ 2 ] + rot[ 2 ][ 0 ] ) / s,
				( rot[ 1 ][ 2 ] - rot[ 2 ][ 1 ] ) / s,
			};
		}
		else if ( rot[ 1 ][ 1 ] > rot[ 2 ][ 2 ] )
		{
			float s = 2.0f * fsqrt( 1.0f + rot[ 1 ][ 1 ] - rot[ 0 ][ 0 ] - rot[ 2 ][ 2 ] );
			return {
				( rot[ 0 ][ 1 ] + rot[ 1 ][ 0 ] ) / s,
				0.25f * s,
				( rot[ 1 ][ 2 ] + rot[ 2 ][ 1 ] ) / s,
				( rot[ 2 ][ 0 ] - rot[ 0 ][ 2 ] ) / s,
			};
		}
		else
		{
			float s = 2.0f * fsqrt( 1.0f + rot[ 2 ][ 2 ] - rot[ 0 ][ 0 ] - rot[ 1 ][ 1 ] );
			return {
				( rot[ 0 ][ 2 ] + rot[ 2 ][ 0 ] ) / s,
				( rot[ 1 ][ 2 ] + rot[ 2 ][ 1 ] ) / s,
				0.25f * s,
				( rot[ 0 ][ 1 ] - rot[ 1 ][ 0 ] ) / s,
			};
		}
	}
	FORCE_INLINE inline constexpr matrix4x4 matrix_rotation( const quaternion& rot )
	{
		matrix4x4 out = matrix4x4::identity();
		out[ 0 ][ 0 ] = 1.0f - 2.0f * ( rot.y * rot.y + rot.z * rot.z );
		out[ 0 ][ 1 ] = 2.0f * ( rot.x * rot.y + rot.z * rot.w );
		out[ 0 ][ 2 ] = 2.0f * ( rot.x * rot.z - rot.y * rot.w );
		out[ 1 ][ 0 ] = 2.0f * ( rot.x * rot.y - rot.z * rot.w );
		out[ 1 ][ 1 ] = 1.0f - 2.0f * ( rot.x * rot.x + rot.z * rot.z );
		out[ 1 ][ 2 ] = 2.0f * ( rot.y * rot.z + rot.x * rot.w );
		out[ 2 ][ 0 ] = 2.0f * ( rot.x * rot.z + rot.y * rot.w );
		out[ 2 ][ 1 ] = 2.0f * ( rot.y * rot.z - rot.x * rot.w );
		out[ 2 ][ 2 ] = 1.0f - 2.0f * ( rot.x * rot.x + rot.y * rot.y );
		return out;
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
	FORCE_INLINE inline matrix4x4 perspective_fov_y( float fov, float aspect, float zn, float zf )
	{
		constexpr float m = LeftHanded ? 1 : -1;
		matrix4x4 out = matrix4x4::identity();

		float t = tanf( fov / 2.0f );
		out[ 0 ][ 0 ] = 1.0f / ( aspect * t );
		out[ 1 ][ 1 ] = 1.0f / t;
		out[ 2 ][ 2 ] = m * zf / ( zf - zn );
		out[ 2 ][ 3 ] = m * 1.0f;
		out[ 3 ][ 2 ] = ( zf * zn ) / ( zn - zf );
		out[ 3 ][ 3 ] = 0.0f;
		return out;
	}
	template<bool LeftHanded = true>
	FORCE_INLINE inline matrix4x4 perspective_fov_x( float fov, float aspect, float zn, float zf )
	{
		constexpr float m = LeftHanded ? 1 : -1;
		matrix4x4 out = matrix4x4::identity();

		float t = tanf( fov / 2.0f );
		out[ 0 ][ 0 ] = 1.0f / t;
		out[ 1 ][ 1 ] = 1.0f / ( aspect * t );
		out[ 2 ][ 2 ] = m * zf / ( zf - zn );
		out[ 2 ][ 3 ] = m * 1.0f;
		out[ 3 ][ 2 ] = ( zf * zn ) / ( zn - zf );
		out[ 3 ][ 3 ] = 0.0f;
		return out;
	}
};
inline constexpr float operator""_deg( long double deg ) { return xstd::math::to_rad( deg ); }
inline constexpr float operator""_deg( unsigned long long int deg ) { return xstd::math::to_rad( deg ); }