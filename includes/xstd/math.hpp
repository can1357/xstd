#pragma once
#include "type_helpers.hpp"
#include "xvector.hpp"
#include "formatting.hpp"
#include <math.h>
#include <array>

// [[Configuration]]
// XSTD_MATH_USE_X86INTRIN: If set, enabled optimization using intrinsics if not constexpr.
// XSTD_MATH_USE_POLY_TRIG: If set, uses sin/cos approximations instead of libc.
//
#ifndef XSTD_MATH_USE_POLY_TRIG
	#define XSTD_MATH_USE_POLY_TRIG 1
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
		template<typename V>
		FORCE_INLINE static void load_matrix( const std::array<V, 4>& src, __m256& v01, __m256& v23 )
		{
			v01 = _mm256_loadu_ps( &src[ 0 ][ 0 ] );
			v23 = _mm256_loadu_ps( &src[ 2 ][ 0 ] );
		}
		template<typename T>
		FORCE_INLINE static T store_matrix( const __m256& v01, const __m256& v23 )
		{
			T result;
			_mm256_storeu_ps( &result[ 0 ][ 0 ], v01 );
			_mm256_storeu_ps( &result[ 2 ][ 0 ], v23 );
			return result;
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
	inline float _fsin( float x ) { return sinf( x ); }
	inline float _fcos( float x ) { return cosf( x ); }
	inline float _fcopysign( float m, float s ) { return copysignf( m, s ); }
	inline float _fabs( float x ) { return fabsf( x ); }
	inline float _ffloor( float x ) { return floorf( x ); }
	inline float _fceil( float x ) { return ceilf( x ); }
	inline float _fround( float x ) { return roundf( x ); }
	inline float _ftrunc( float x ) { return truncf( x ); }
#else
	float fsqrt( float x ) asm( "llvm.sqrt.f32" );
	float _fsin( float x ) asm( "llvm.sin.f32" );
	float _fcos( float x ) asm( "llvm.cos.f32" );
	float _fcopysign( float m, float s ) asm( "llvm.copysign.f32" );
	float _fabs( float x ) asm( "llvm.fabs.f32" );
	float _ffloor( float x ) asm( "llvm.floor.f32" );
	float _fceil( float x ) asm( "llvm.ceil.f32" );
	float _fround( float x ) asm( "llvm.round.f32" );
	float _ftrunc( float x ) asm( "llvm.trunc.f32" );
#endif
	CONST_FN FORCE_INLINE inline constexpr float fabs( float a )
	{
		if ( !std::is_constant_evaluated() )
			return _fabs( a );
		if ( a < 0 ) a = -a;
		return a;
	}
	CONST_FN FORCE_INLINE inline constexpr float fcopysign( float m, float s )
	{
		if ( !std::is_constant_evaluated() )
			return _fcopysign( m, s );
		m = fabs( m );
		if ( s < 0 ) m = -m;
		return m;
	}
	CONST_FN FORCE_INLINE inline constexpr float ffloor( float x )
	{
		if ( !std::is_constant_evaluated() )
			return _ffloor( x );
		float d = x - int64_t( x );
		if ( d != 0 ) d = -1;
		return int64_t( x + d );
	}
	CONST_FN FORCE_INLINE inline constexpr float fceil( float x )
	{
		if ( !std::is_constant_evaluated() )
			return _fceil( x );
		float d = x - int64_t( x );
		if ( d != 0 ) d = +1;
		return int64_t( x + d );
	}
	CONST_FN FORCE_INLINE inline constexpr float fround( float x )
	{
		if ( !std::is_constant_evaluated() )
			return _fround( x );
		return float( int64_t( x + fcopysign( 0.5f, x ) ) );
	}
	CONST_FN FORCE_INLINE inline constexpr float ftrunc( float x )
	{
		if ( !std::is_constant_evaluated() )
			return _ftrunc( x );
		return float( int64_t( x ) );
	}
	CONST_FN FORCE_INLINE inline constexpr float fmod( float x, float y )
	{
		float m = 1.0f / y;
		return x - ftrunc( x * m ) * y;
	}
	CONST_FN FORCE_INLINE inline constexpr float foddsgn( float x )
	{
		if ( std::is_constant_evaluated() )
			return ( int64_t( x ) % 2 ) ? -1.0f : 1.0f;
		x *= 0.5f;
		return x - fceil( x );
	}
	CONST_FN FORCE_INLINE inline constexpr float fxorsgn( float x, float y )
	{
		if ( std::is_constant_evaluated() )
			return ( ( x < 0 ) != ( y < 0 ) ) ? -1.0f : 1.0f;
		else
			return bit_cast< float >( bit_cast< int32_t >( x ) ^ bit_cast< int32_t >( y ) );
	}
	CONST_FN FORCE_INLINE inline constexpr float fmin( float a, float b )
	{
#if __has_builtin(__builtin_fminf)
		if ( !std::is_constant_evaluated() )
			return __builtin_fminf( a, b );
#endif
		if ( b < a ) a = b;
		return a;
	}
	CONST_FN FORCE_INLINE inline constexpr float fmax( float a, float b )
	{
#if __has_builtin(__builtin_fminf)
		if ( !std::is_constant_evaluated() )
			return __builtin_fmaxf( a, b );
#endif
		if ( a < b ) a = b;
		return a;
	}
	CONST_FN FORCE_INLINE inline constexpr float fclamp( float v, float vmin, float vmax )
	{
		return fmin( fmax( v, vmin ), vmax );
	}
	template<typename... Tx>
	CONST_FN FORCE_INLINE inline constexpr float fmin( float a, float b, float c, Tx... rest )
	{
		return fmin( fmin( a, b ), c, rest... );
	}
	template<typename... Tx>
	CONST_FN FORCE_INLINE inline constexpr float fmax( float a, float b, float c, Tx... rest )
	{
		return fmax( fmax( a, b ), c, rest... );
	}
	
	// Implement fast polynomial approximations of sin and cos.
	//
	namespace impl
	{
		// [0, 1/2 pi]
		// - https://gist.github.com/publik-void/067f7f2fef32dbe5c27d6e215f824c91#file-sin-cos-approximations-gist-adoc
		CONST_FN FORCE_INLINE inline constexpr float fsin_poly_hpi( float x1 )
		{
			if ( std::is_constant_evaluated() )
			{
				if ( ( x1 - 1e-4 ) <= 0 ) return 0.0f;
				if ( ( x1 + 1e-4 ) >= ( pi / 2 ) ) return 1.0f;
			}

			float x2 = x1 * x1;
			// Degree 11, E(X) = 1.92e-11
			return x1 * ( 0.99999999997884898600402426033768998f + x2 * ( -0.166666666088260696413164261885310067f + x2 * ( 0.00833333072055773645376566203656709979f + x2 * ( -0.000198408328232619552901560108010257242f + x2 * ( 2.75239710746326498401791551303359689e-6f - 2.3868346521031027639830001794722295e-8f * x2 ) ) ) ) );
		}
		CONST_FN FORCE_INLINE inline constexpr float fcos_poly_hpi( float x1 )
		{
			if ( std::is_constant_evaluated() )
			{
				if ( ( x1 - 1e-4 ) <= 0 ) return 1.0f;
				if ( ( x1 + 1e-4 ) >= ( pi / 2 ) ) return 0.0f;
			}

			float x2 = x1 * x1;
			// Degree 12, E(X) = 3.35e-12
			return 0.99999999999664497762294088303450344f + x2 * ( -0.499999999904093446864749737540127153f + x2 * ( 0.0416666661919898461055893453767336909f + x2 * ( -0.00138888797032770920681384355560203468f + x2 * ( 0.0000248007136556145113256051130495176344f + x2 * ( -2.75135611164571371141959208910569516e-7f + 1.97644182995841772799444848310451781e-9f * x2 ) ) ) ) );
		}
		CONST_FN FORCE_INLINE inline constexpr std::pair<float, float> fsincos_poly( float x )
		{
			// [0, inf]
			float a = fabs( x ) / pi;

			// [0, pi]
			float n = ftrunc( a );
			float v = a - n;

			// [0, pi/2]
			if ( v > 0.5f )
				v = 1.0f - v;
			return {
				fcopysign( fsin_poly_hpi( v * pi ), fxorsgn( x, foddsgn( n ) ) ),
				fcopysign( fcos_poly_hpi( v * pi ), foddsgn( fround( a ) ) )
			};
		}
	};

	CONST_FN FORCE_INLINE inline constexpr float fsin( float x )
	{
		if ( std::is_constant_evaluated() || XSTD_MATH_USE_POLY_TRIG )
			return impl::fsincos_poly( x ).first;
		return _fsin( x );
	}
	CONST_FN FORCE_INLINE inline constexpr float fcos( float x )
	{
		if ( std::is_constant_evaluated() || XSTD_MATH_USE_POLY_TRIG )
			return impl::fsincos_poly( x ).second;
		return _fcos( x );
	}
	CONST_FN FORCE_INLINE inline constexpr float ftan( float x )
	{
		if ( std::is_constant_evaluated() )
		{
			auto [s, c] = impl::fsincos_poly( x );
			return s / c;
		}
		return tanf( x );
	}
	CONST_FN FORCE_INLINE inline constexpr std::pair<float, float> fsincos( float x )
	{
		if ( std::is_constant_evaluated() || XSTD_MATH_USE_POLY_TRIG )
			return impl::fsincos_poly( x );

		float s = fsin( x );
		float c = fcopysign( fsqrt( 1 - s * s ), foddsgn( fround( fabs( x ) / pi ) ) );
		return { s, c };
	}

	// Common vector operations with accceleration.
	//
	template<typename V>
	CONST_FN FORCE_INLINE inline constexpr float dot( const V& v1, const V& v2 )
	{
#if XSTD_MATH_USE_X86INTRIN
		if ( !std::is_constant_evaluated() )
		{
			if constexpr ( Same<typename V::value_type, float> && V::Length == 4 )
			{
				__m128 q1 = v1.to_xvec()._nat;
				__m128 q2 = v2.to_xvec()._nat;
				return (_mm_dp_ps(q1, q2, 0b1111'0001))[0];
			}
			else if constexpr ( Same<typename V::value_type, float> && V::Length == 3 )
			{
				__m128 q1 = v1.to_xvec()._nat;
				__m128 q2 = v2.to_xvec()._nat;
				return (_mm_dp_ps(q1, q2, 0b0111'0001))[0];
			}
		}
#endif
		return ( v1 * v2 ).reduce_add();
	}
	template<typename V>
	PURE_FN FORCE_INLINE inline V normalize( const V& vec )
	{
		return vec / fsqrt( fmax( flt_eps, vec.length_sq() ) );
	}
	template<typename V>
	CONST_FN FORCE_INLINE inline constexpr V lerp( const V& v1, const V& v2, float s )
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
			using key_type = size_t;
			using value_type = T;
			static constexpr size_t Length = 2;

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
			FORCE_INLINE inline std::string to_string() const noexcept { return fmt::as_string( x, y ); }

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
			using key_type = size_t;
			using value_type = T;
			static constexpr size_t Length = 3;

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
			FORCE_INLINE inline constexpr xvec<T, 4> to_xvec() const noexcept 
			{ 
				return { x, y, z, 0 };
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
			FORCE_INLINE inline std::string to_string() const noexcept { return fmt::as_string( x, y, z ); }
			
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
			FORCE_INLINE inline constexpr T reduce_add() const noexcept { return vec::reduce_add( to_xvec() ); }
			FORCE_INLINE inline constexpr vec3_t min_element( const vec3_t& o ) const noexcept { return from_xvec( vec::min( to_xvec(), o.to_xvec() ) ); }
			FORCE_INLINE inline constexpr vec3_t max_element( const vec3_t& o ) const noexcept { return from_xvec( vec::max( to_xvec(), o.to_xvec() ) ); }

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
			using key_type = size_t;
			using value_type = T;
			static constexpr size_t Length = 4;

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
			FORCE_INLINE inline std::string to_string() const noexcept { return fmt::as_string( x, y, z, w ); }

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
			FORCE_INLINE inline constexpr vec4_t min_element( const vec4_t& o ) const noexcept { return from_xvec( vec::min( to_xvec(), o.to_xvec() ) ); }
			FORCE_INLINE inline constexpr vec4_t max_element( const vec4_t& o ) const noexcept { return from_xvec( vec::max( to_xvec(), o.to_xvec() ) ); }

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

			template<int A, int B, int C, int D>
			FORCE_INLINE inline constexpr vec4_t shuffle( const vec4_t& o ) const noexcept { return from_xvec( to_xvec().template shuffle<A, B, C, D>( o.to_xvec() ) ); }
			template<int A, int B, int C, int D>
			FORCE_INLINE inline constexpr vec4_t shuffle( const vec3_t<T>& o ) const noexcept { return from_xvec( to_xvec().template shuffle<A, B, C, D>( o.to_xvec() ) ); }
			template<int A, int B, int C, int D>
			FORCE_INLINE inline constexpr vec4_t shuffle( const vec2_t<T>& o ) const noexcept { return from_xvec( to_xvec().template shuffle<A, B, C, D>( o.to_xvec() ) ); }
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

	FORCE_INLINE inline constexpr vec3 vec_abs( const vec3& vec ) { return vec_abs( vec4::from( vec, 0 ) ).xyz(); }
	FORCE_INLINE inline constexpr vec3 vec_ceil( const vec3& vec ) { return vec_ceil( vec4::from( vec, 0 ) ).xyz(); }
	FORCE_INLINE inline constexpr vec3 vec_floor( const vec3& vec ) { return vec_floor( vec4::from( vec, 0 ) ).xyz(); }
	FORCE_INLINE inline constexpr vec3 vec_round( const vec3& vec ) { return vec_round( vec4::from( vec, 0 ) ).xyz(); }
	FORCE_INLINE inline constexpr vec3 vec_trunc( const vec3& vec ) { return vec_trunc( vec4::from( vec, 0 ) ).xyz(); }
	FORCE_INLINE inline vec3 vec_sqrt( const vec3& vec ) { return vec_sqrt( vec4::from( vec, 0 ) ).xyz(); }

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
		auto x1 = v1.to_xvec();
		auto x2 = v2.to_xvec();
		auto res = ( x1 * x2.template shuffle<1, 2, 0, 3>( x2 ) ) -
					  ( x2 * x1.template shuffle<1, 2, 0, 3>( x1 ) );
		return vec3::from_xvec( res.template shuffle<1, 2, 0, 3>( res ) );
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
		return q * vec4{ -1, -1, -1, +1 };
	}

	// Define matrix type.
	//
	struct mat4x4
	{
		using key_type =   size_t;
		using value_type = vec4;

		std::array<vec4, 4> m = { vec4{} };

		// Default construction.
		//
		constexpr mat4x4() = default;

		// Construction by 4 vectors.
		//
		FORCE_INLINE inline constexpr mat4x4( vec4 a, vec4 b, vec4 c, vec4 d ) : m{ a, b, c, d } {}
		
		// Construction by all values.
		//
		FORCE_INLINE inline constexpr mat4x4( float _00, float _01, float _02, float _03,
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
		constexpr mat4x4( const mat4x4& ) = default;
		constexpr mat4x4& operator=( const mat4x4& ) = default;

		// Identity matrix.
		//
		inline static constexpr std::array<vec4, 4> identity_value = {
			vec4{ 1, 0, 0, 0 },
			vec4{ 0, 1, 0, 0 },
			vec4{ 0, 0, 1, 0 },
			vec4{ 0, 0, 0, 1 }
		};
		FORCE_INLINE inline static constexpr mat4x4 identity() noexcept { return { identity_value[ 0 ], identity_value[ 1 ], identity_value[ 2 ], identity_value[ 3 ] }; }

		// Matrix mulitplication.
		//
		FORCE_INLINE inline constexpr mat4x4 operator*( const mat4x4& other ) const noexcept
		{
#if XSTD_MATH_USE_X86INTRIN
			if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) )
			{
				__m256 v01, v23, q01, q23;
				impl::load_matrix( m, v01, v23 );
				impl::load_matrix( other.m, q01, q23 );
#if __XSTD_USE_DPPS || !XSTD_VECTOR_EXT
				impl::tranpose_inplace( q01, q23 );
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
				return impl::store_matrix<mat4x4>( t0, t4 );
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
#endif
			return {
				other * m[ 0 ],
				other * m[ 1 ],
				other * m[ 2 ],
				other * m[ 3 ]
			};
		}
		FORCE_INLINE inline constexpr mat4x4& operator*=( const mat4x4& other ) noexcept { *this = ( *this * other ); return *this; }

		// Vector transformation.
		//
		FORCE_INLINE inline constexpr vec4 operator*( const vec4& v ) const noexcept
		{
#if XSTD_MATH_USE_X86INTRIN
			if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) )
			{
				auto xyzw = v.to_xvec();
				__m256 vxy = xyzw.template shuffle<0, 0, 0, 0, 1, 1, 1, 1>( xyzw )._nat;
				__m256 vzw = xyzw.template shuffle<2, 2, 2, 2, 3, 3, 3, 3>( xyzw )._nat;

				__m256 vrlh = _mm256_mul_ps( vxy, _mm256_loadu_ps( ( const float* ) &m[ 0 ] ) );
				vrlh = _mm256_fmadd_ps( vzw, _mm256_loadu_ps( ( const float* ) &m[ 2 ] ), vrlh );

				auto vrl = _mm256_extractf128_ps( vrlh, 0 );
				auto vrh = _mm256_extractf128_ps( vrlh, 1 );
				return vec4::from_xvec( xvec<float, 4>{ std::in_place_t{}, _mm_add_ps( vrl, vrh ) } );
			}
#endif
			auto r = m[ 0 ] * v.x;
			r += m[ 1 ] * v.y;
			r += m[ 2 ] * v.z;
			r += m[ 3 ] * v.w;
			return r;
		}
		FORCE_INLINE inline constexpr vec4 apply_no_w( const vec4& v ) const noexcept
		{
#if XSTD_MATH_USE_X86INTRIN
			if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) )
			{
				xvec<float, 4> one{ 1.0f };
				auto xyzw = v.to_xvec();
				__m256 vxy = xyzw.template shuffle<0, 0, 0, 0, 1, 1, 1, 1>( xyzw )._nat;
				__m256 vzw = xyzw.template shuffle<2, 2, 2, 2, 4, 4, 4, 4>( one )._nat;

				__m256 vrlh = _mm256_mul_ps( vxy, _mm256_loadu_ps( ( const float* ) &m[ 0 ] ) );
				vrlh = _mm256_fmadd_ps( vzw, _mm256_loadu_ps( ( const float* ) &m[ 2 ] ), vrlh );

				auto vrl = _mm256_extractf128_ps( vrlh, 0 );
				auto vrh = _mm256_extractf128_ps( vrlh, 1 );
				return vec4::from_xvec( xvec<float, 4>{ std::in_place_t{}, _mm_add_ps( vrl, vrh ) } );
			}
#endif
			auto r = m[ 0 ] * v.x;
			r += m[ 1 ] * v.y;
			r += m[ 2 ] * v.z;
			r += m[ 3 ];
			return r;
		}
		FORCE_INLINE inline constexpr vec3 operator*( const vec3& v ) const noexcept
		{
			return apply_no_w( vec4::from_xvec( v.to_xvec() ) ).xyz();
		}

		// Offset by scalar.
		//
		FORCE_INLINE inline constexpr mat4x4 operator+( float v ) const noexcept
		{
#if XSTD_MATH_USE_X86INTRIN
			if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) )
			{
				__m256 f = _mm256_broadcast_ss( &v );
				
				__m256 a, b;
				impl::load_matrix( m, a, b );
				
				a = _mm256_add_ps( a, f );
				b = _mm256_add_ps( b, f );
				return impl::store_matrix<mat4x4>( a, b );
			}
#endif

			mat4x4 result;
			for ( size_t i = 0; i != 4; i++ )
				result[ i ] = m[ i ] + v;
			return result;
		}
		FORCE_INLINE inline constexpr mat4x4 operator-( float v ) const noexcept { return *this + ( -v ); }
		FORCE_INLINE inline constexpr mat4x4& operator-=( float v ) noexcept { *this = ( *this - v ); return *this; }
		FORCE_INLINE inline constexpr mat4x4& operator+=( float v ) noexcept { *this = ( *this + v ); return *this; }

		// Scale by scalar.
		//
		FORCE_INLINE inline constexpr mat4x4 operator*( float v ) const noexcept
		{
#if XSTD_MATH_USE_X86INTRIN
			if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) )
			{
				__m256 f = _mm256_broadcast_ss( &v );
				
				__m256 a, b;
				impl::load_matrix( m, a, b );
				
				a = _mm256_mul_ps( a, f );
				b = _mm256_mul_ps( b, f );
				return impl::store_matrix<mat4x4>( a, b );
			}
#endif

			mat4x4 result;
			for ( size_t i = 0; i != 4; i++ )
				result[ i ] = m[ i ] * v;
			return result;
		}
		FORCE_INLINE inline constexpr mat4x4 operator/( float v ) const noexcept { return *this * ( 1.0f / v ); }
		FORCE_INLINE inline constexpr mat4x4& operator/=( float v ) noexcept { *this = ( *this / v ); return *this; }
		FORCE_INLINE inline constexpr mat4x4& operator*=( float v ) noexcept { *this = ( *this * v ); return *this; }

		// Unaries.
		//
		FORCE_INLINE inline constexpr mat4x4 operator-() const noexcept 
		{
#if XSTD_MATH_USE_X86INTRIN
			if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) )
			{
				__m256 a, b;
				impl::load_matrix( m, a, b );
				
				__v8su c{ 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
				a = _mm256_xor_ps( a, ( __m256 ) c );
				b = _mm256_xor_ps( b, ( __m256 ) c );
				return impl::store_matrix<mat4x4>( a, b );
			}
#endif
			return *this * -1.0f; 
		}
		FORCE_INLINE inline constexpr mat4x4 operator+() const noexcept { return *this; }

		// Forward indexing.
		//
		FORCE_INLINE inline constexpr vec4& operator[]( size_t n ) noexcept { return m[ n & 3 ]; }
		FORCE_INLINE inline constexpr const vec4& operator[]( size_t n ) const noexcept { return m[ n & 3 ]; }
		FORCE_INLINE inline constexpr float& operator()( size_t i, size_t j ) noexcept { return m[ i & 3 ][ j & 3 ]; }
		FORCE_INLINE inline constexpr const float& operator()( size_t i, size_t j ) const noexcept { return m[ i & 3 ][ j & 3 ]; }

		// Hashing, serialization, comparison and string conversion.
		//
		FORCE_INLINE inline constexpr auto operator<=>( const mat4x4& ) const noexcept = default;
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
		FORCE_INLINE inline constexpr mat4x4 transpose() const noexcept
		{
#if XSTD_MATH_USE_X86INTRIN
			if ( !std::is_constant_evaluated() && !xstd::is_consteval( *this ) )
			{
				__m256 v01, v23;
				impl::load_matrix( m, v01, v23 );
				impl::tranpose_inplace( v01, v23 );
				return impl::store_matrix<mat4x4>( v01, v23 );
			}
#endif
			mat4x4 out;
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
	inline constexpr mat4x4 inverse( const mat4x4& m, float& det )
	{
		float t[ 3 ] = {};
		mat4x4 out = {};

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
	FORCE_INLINE inline constexpr mat4x4 inverse( const mat4x4& m ) { float tmp = 0; return inverse( m, tmp ); }

	// Matrix decomposition helpers.
	//
	FORCE_INLINE inline constexpr vec3 matrix_to_translation( const mat4x4& mat )
	{
		return mat[ 3 ].xyz();
	}
	FORCE_INLINE inline vec3 matrix_to_scale( const mat4x4& mat )
	{
		return { mat[ 0 ].xyz().length(), mat[ 1 ].xyz().length(), mat[ 2 ].xyz().length() };
	}
	FORCE_INLINE inline vec3 matrix_to_forward( const mat4x4& mat )
	{
		return normalize( mat[ 0 ].xyz() );
	}
	FORCE_INLINE inline vec3 matrix_to_right( const mat4x4& mat )
	{
		return normalize( mat[ 1 ].xyz() );
	}
	FORCE_INLINE inline vec3 matrix_to_up( const mat4x4& mat )
	{
		return normalize( mat[ 2 ].xyz() );
	}

	// Normalizes a matrix to have no scale.
	//
	FORCE_INLINE inline mat4x4 matrix_normalize( mat4x4 mat )
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
	FORCE_INLINE inline constexpr quaternion rotate_q( float theta, const vec3& axis )
	{
		auto [s, c] = fsincos( theta / 2 );
		return {
			s * axis.x,
			s * axis.y,
			s * axis.z,
			c
		};
	}
	FORCE_INLINE inline constexpr mat4x4 rotate_v( float theta, const vec3& axis )
	{
		auto [sangle, cangle] = fsincos( theta );
		float cdiff = 1.0f - cangle;

		mat4x4 out;
		out[ 0 ][ 0 ] = cdiff * axis.x * axis.x + cangle;
		out[ 0 ][ 1 ] = cdiff * axis.y * axis.x + sangle * axis.z;
		out[ 0 ][ 2 ] = cdiff * axis.z * axis.x - sangle * axis.y;
		out[ 0 ][ 3 ] = 0.0f;

		out[ 1 ][ 0 ] = cdiff * axis.x * axis.y - sangle * axis.z;
		out[ 1 ][ 1 ] = cdiff * axis.y * axis.y + cangle;
		out[ 1 ][ 2 ] = cdiff * axis.z * axis.y + sangle * axis.x;
		out[ 1 ][ 3 ] = 0.0f;

		out[ 2 ][ 0 ] = cdiff * axis.x * axis.z + sangle * axis.y;
		out[ 2 ][ 1 ] = cdiff * axis.y * axis.z - sangle * axis.x;
		out[ 2 ][ 2 ] = cdiff * axis.z * axis.z + cangle;
		out[ 2 ][ 3 ] = 0.0f;

		out[ 3 ][ 0 ] = 0.0f;
		out[ 3 ][ 1 ] = 0.0f;
		out[ 3 ][ 2 ] = 0.0f;
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
	FORCE_INLINE inline constexpr mat4x4 rotate_by( const mat4x4& m1, const mat4x4& m2 )
	{
		return m1 * m2;
	}

	// Rotating a vector.
	//
	FORCE_INLINE inline constexpr vec3 rotate_by( const vec3& v, const mat4x4& m ) { return m * v; }
	FORCE_INLINE inline constexpr vec4 rotate_by( const vec4& v, const mat4x4& m ) { return m * v; }
	FORCE_INLINE inline constexpr vec3 rotate_by( const vec3& v, const quaternion& q )
	{
		vec3 t = cross( q.xyz(), v ) * 2;
		return v + t * q.w + cross( q.xyz(), t );
	}

	// Translation and scaling matrices.
	//
	FORCE_INLINE inline constexpr mat4x4 matrix_translation( const vec3& p )
	{
		mat4x4 out = mat4x4::identity();
		out[ 3 ][ 0 ] = p.x;
		out[ 3 ][ 1 ] = p.y;
		out[ 3 ][ 2 ] = p.z;
		return out;
	}
	FORCE_INLINE inline constexpr mat4x4 matrix_scaling( const vec3& p )
	{
		mat4x4 out = mat4x4::identity();
		out[ 0 ][ 0 ] = p.x;
		out[ 1 ][ 1 ] = p.y;
		out[ 2 ][ 2 ] = p.z;
		return out;
	}

	// Spherical quaternion interpolation.
	//
	FORCE_INLINE inline constexpr quaternion quaternion_slerp( const quaternion& q1, const quaternion& q2, float t )
	{
		float t2 = 1.0f - t;

		// Compute dot, if negative flip the signs.
		//
		float dotv = dot( q1, q2 );
		t = fcopysign( t, dotv );
		dotv = fabs( dotv );

		// If theta is numerically significant and will not result in division by zero:
		//
		if ( dotv < 0.999f )
		{
			float theta = acosf( dotv );
			float rstheta = 1.0f / fsin( theta );
			t2 = fsin( theta * t2 ) * rstheta;
			t =  fsin( theta * t )  * rstheta;
		}
		return t2 * q1 + t * q2;
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
	FORCE_INLINE inline vec3 matrix_to_euler( const mat4x4& mat )
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
	FORCE_INLINE inline quaternion matrix_to_quaternion( const mat4x4& umat )
	{
		mat4x4 mat = matrix_normalize( umat );
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
	FORCE_INLINE inline vec3 matrix_to_direction( const mat4x4& mat )
	{
		return normalize( matrix_to_forward( mat ) );
	}

	// Direction to Euler/Quaternion/Matrix.
	//
	FORCE_INLINE inline mat4x4 direction_to_matrix( const vec3& direction )
	{
		vec4 up_vec =    {  0, 1, 0, 0 };
		vec4 right_vec = { -1, 0, 0, 0 };
		vec4 dir_vec =   normalize( vec4::from( direction, 0 ) );

		if ( ( direction.x * direction.x + direction.y * direction.y ) > 1e-5f )
		{
			vec3 up_vec3 =    cross( up, direction );
			vec3 right_vec3 = cross( direction, up_vec3 );
			up_vec =    vec4::from( up_vec3, 0 );
			right_vec = vec4::from( right_vec3, 0 );
		}

		return {
			dir_vec,
			normalize( up_vec ),
			normalize( right_vec ),
			vec4{ 0, 0, 0, 1 },
		};
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
	FORCE_INLINE inline constexpr mat4x4 quaternion_to_matrix( const quaternion& rot )
	{
		mat4x4 out = {};
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
	FORCE_INLINE inline constexpr vec3 quaternion_to_direction( const quaternion& q )
	{
		return rotate_by( forward, q );
	}


	// Euler to Matrix/Quaternion/Direction.
	//
	FORCE_INLINE inline constexpr mat4x4 euler_to_matrix( const vec3& rot )
	{
		auto [spitch, cpitch] = fsincos( rot.x );
		auto [syaw,   cyaw] =   fsincos( rot.y );
		auto [sroll,  croll] =  fsincos( rot.z );

		mat4x4 out = {};
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
	FORCE_INLINE inline constexpr quaternion euler_to_quaternion( const vec3& rot )
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
	FORCE_INLINE inline constexpr vec3 euler_to_direction( const vec3& rot )
	{
		auto [spitch, cpitch] = fsincos( rot.x );
		auto [syaw,   cyaw] =   fsincos( rot.y );
		return {
			cpitch * cyaw,
			cpitch * syaw,
			-spitch
		};
	}

	// Fast transformation matrix.
	//
	FORCE_INLINE inline constexpr mat4x4 matrix_transformation( const quaternion& rot, const vec3& scale, const vec3& translation )
	{
		auto mat = quaternion_to_matrix( rot );
		mat[ 0 ] *= scale.x;
		mat[ 1 ] *= scale.y;
		mat[ 2 ] *= scale.z;
		mat[ 3 ] = vec4::from( translation, 1 );
		return mat;
	}

	// View helpers.
	//
	template<bool LeftHanded = true>
	FORCE_INLINE inline mat4x4 look_at( vec3 eye, vec3 dir, vec3 up )
	{
		constexpr float m = LeftHanded ? 1 : -1;

		vec3 right = cross( up, dir );
		up = cross( dir, right );
		right = normalize( right );
		up = normalize( up );

		mat4x4 out = {};
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


	// 1->0 Depth, infinitely far.
	//
	template<bool LeftHanded = true>
	FORCE_INLINE inline constexpr mat4x4 perspective_projection_rz( float tfx, float tfy, float zn )
	{
		constexpr float m = LeftHanded ? 1 : -1;
		mat4x4 out = mat4x4::identity();
		out[ 0 ][ 0 ] = 1.0f / tfx;
		out[ 1 ][ 1 ] = 1.0f / tfy;
		out[ 2 ][ 2 ] = 0.0f;
		out[ 2 ][ 3 ] = m * 1.0f;
		out[ 3 ][ 2 ] = zn;
		out[ 3 ][ 3 ] = 0.0f;
		return out;
	}

	// 1->0 Depth, far clipped.
	//
	template<bool LeftHanded = true>
	FORCE_INLINE inline constexpr mat4x4 perspective_projection_rz( float tfx, float tfy, float zn, float zf )
	{
		constexpr float m = LeftHanded ? 1 : -1;
		mat4x4 out = mat4x4::identity();
		out[ 0 ][ 0 ] = 1.0f / tfx;
		out[ 1 ][ 1 ] = 1.0f / tfy;
		out[ 2 ][ 2 ] = m * -zn / ( zf - zn );
		out[ 2 ][ 3 ] = m * 1.0f;
		out[ 3 ][ 2 ] = ( zf * zn ) / ( zf - zn );
		out[ 3 ][ 3 ] = 0.0f;
		return out;
	}

	// 0->1 Depth, infinitely far.
	//
	template<bool LeftHanded = true>
	FORCE_INLINE inline constexpr mat4x4 perspective_projection( float tfx, float tfy, float zn )
	{
		constexpr float m = LeftHanded ? 1 : -1;
		mat4x4 out = mat4x4::identity();
		out[ 0 ][ 0 ] = 1.0f / tfx;
		out[ 1 ][ 1 ] = 1.0f / tfy;
		out[ 2 ][ 2 ] = m * 1.0f;
		out[ 2 ][ 3 ] = m * 1.0f;
		out[ 3 ][ 2 ] = -zn;
		out[ 3 ][ 3 ] = 0.0f;
		return out;
	}

	// 0->1 Depth, far clipped.
	//
	template<bool LeftHanded = true>
	FORCE_INLINE inline constexpr mat4x4 perspective_projection( float tfx, float tfy, float zn, float zf )
	{
		constexpr float m = LeftHanded ? 1 : -1;
		mat4x4 out = mat4x4::identity();
		out[ 0 ][ 0 ] = 1.0f / tfx;
		out[ 1 ][ 1 ] = 1.0f / tfy;
		out[ 2 ][ 2 ] = m * zf / ( zf - zn );
		out[ 2 ][ 3 ] = m * 1.0f;
		out[ 3 ][ 2 ] = -( zf * zn ) / ( zf - zn );
		out[ 3 ][ 3 ] = 0.0f;
		return out;
	}

	// Factorial, fast power and binomial helpers for beizer implementation.
	//
	CONST_FN FORCE_INLINE inline constexpr float ffactorial( int i )
	{
		if ( std::is_constant_evaluated() || is_consteval( i ) )
		{
			if ( i >= 0 )
			{
				int r = 1;
				for ( int it = 2; it <= i; it++ )
					r *= it;
				return float( r );
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
	inline constexpr float binomial_coefficient_v = ffactorial( N ) / ( ffactorial( I ) * ffactorial( N - I ) );

	// Implement the beizer calculation.
	//
	namespace impl
	{
		template<size_t N, typename Vec, typename... Vx>
		FORCE_INLINE inline constexpr Vec beizer_helper( const Vec& ti, const Vec& ts, const Vec& pi, const Vx&... px )
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
	FORCE_INLINE inline constexpr V beizer( float t, const V& p1, const Vx&... px )
	{
		float ti = const_pow<sizeof...( Vx )>( 1 - t );
		float ts = t / ( 1 - t );
		return impl::beizer_helper<sizeof...( Vx )>( V::fill( ti ), V::fill( ts ), p1, px... );
	}
	template<typename V, typename C>
	FORCE_INLINE inline constexpr V beizer_dyn( float t, C&& container )
	{
		V accumulator = {};
		float n = ( ( int ) std::size( container ) );
		float i = 0;
		float tx = 1;
		float ti = 1 / ( 1 - t );
		float ts = t * ti;
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
};
inline constexpr float operator""_deg( long double deg ) { return xstd::math::to_rad( deg ); }
inline constexpr float operator""_deg( unsigned long long int deg ) { return xstd::math::to_rad( deg ); }