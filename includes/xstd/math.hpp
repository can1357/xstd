#pragma once
#include "type_helpers.hpp"
#include "xvector.hpp"
#include "formatting.hpp"
#include <math.h>
#include <array>

// Defines useful math primitives.
//
namespace xstd::math
{
	// Math constants.
	//
	static constexpr float pi =      ( float ) 3.14159265358979323846;
	static constexpr float flt_eps = FLT_EPSILON;
	static constexpr float flt_min = FLT_MIN;
	static constexpr float flt_max = FLT_MAX;

	// Fast floating point intrinsics.
	//
#if !defined(CLANG_COMPILER) || defined(__INTELLISENSE__)
	inline float fsqrt( float x ) { return sqrtf( x ); }
	inline float fsin( float x ) { return sinf( x ); }
	inline float fcos( float x ) { return cosf( x ); }
	inline float _fcopysign( float m, float s ) { return copysignf( m, s ); }
	inline float _fabs( float x ) { return fabsf( x ); }
	inline float _ffloor( float x ) { return floorf( x ); }
#else
	float fsqrt( float x ) asm( "llvm.sqrt.f32" );
	float fsin( float x ) asm( "llvm.sin.f32" );
	float fcos( float x ) asm( "llvm.cos.f32" );
	float _fcopysign( float m, float s ) asm( "llvm.copysign.f32" );
	float _fabs( float x ) asm( "llvm.fabs.f32" );
	float _ffloor( float x ) asm( "llvm.floor.f32" );
#endif
	inline constexpr float fabs( float a )
	{
		if ( !std::is_constant_evaluated() )
			return _fabs( a );

		if ( a < 0 ) a = -a;
		return a;
	}
	inline constexpr float fcopysign( float m, float s ) 
	{
		if ( !std::is_constant_evaluated() )
			return _fcopysign( m, s );
		m = fabs( m );
		if ( s < 0 ) m = -m;
		return m;
	}
	inline constexpr float ffloor( float a )
	{
		if ( !std::is_constant_evaluated() )
			return _ffloor( a );
		return float( int64_t( a ) );
	}
	inline constexpr float fmod( float x, float y )
	{
		float m = 1.0f / y;
		return x - ffloor( x * m ) * y;
	}

	// Implement sincos since NT CRT does not include it.
	//
	inline std::pair<float, float> fsincos( float x ) 
	{
		float c = fcos( x );
		float s = fsin( x );
		return { s, c };
	}

	// Implement non-referencing min/max/clamp.
	//
	inline constexpr float fmin( float a, float b )
	{
		if ( b < a ) a = b;
		return a;
	}
	inline constexpr float fmax( float a, float b )
	{
		if ( a < b ) a = b;
		return a;
	}
	inline constexpr float fclamp( float v, float vmin, float vmax ) 
	{
		if ( v < vmin ) v = vmin;
		if ( v > vmax ) v = vmax;
		return v;
	}

	// Define vector types.
	//
#define ADD_OPERATORS(type)																													       \
	inline constexpr type operator+( const type& o ) const noexcept { return from_xvec( to_xvec() + o.to_xvec() ); }		  \
	inline constexpr type operator+( float f ) const noexcept { return from_xvec( to_xvec() + f ); }							  \
	inline constexpr type& operator+=( const type& o ) noexcept { return *this = from_xvec( to_xvec() + o.to_xvec() ); }	  \
	inline constexpr type& operator+=( float f ) noexcept { return *this = from_xvec( to_xvec() + f ); }						  \
	inline friend constexpr type operator+( float f, const type& o ) noexcept { return from_xvec( o.to_xvec() + f ); }	  \
	inline constexpr type operator*( const type& o ) const noexcept { return from_xvec( to_xvec() * o.to_xvec() ); }		  \
	inline constexpr type operator*( float f ) const noexcept { return from_xvec( to_xvec() * f ); }							  \
	inline constexpr type& operator*=( const type& o ) noexcept { return *this = from_xvec( to_xvec() * o.to_xvec() ); }	  \
	inline constexpr type& operator*=( float f ) noexcept { return *this = from_xvec( to_xvec() * f ); }						  \
	inline friend constexpr type operator*( float f, const type& o ) noexcept { return from_xvec( o.to_xvec() * f ); }	  \
	inline constexpr type operator-( const type& o ) const noexcept { return from_xvec( to_xvec() - o.to_xvec() ); }		  \
	inline constexpr type operator-( float f ) const noexcept { return from_xvec( to_xvec() - f ); }							  \
	inline constexpr type& operator-=( const type& o ) noexcept { return *this = from_xvec( to_xvec() - o.to_xvec() ); }	  \
	inline constexpr type& operator-=( float f ) noexcept { return *this = from_xvec( to_xvec() - f ); }						  \
	inline constexpr type operator/( const type& o ) const noexcept { return from_xvec( to_xvec() / o.to_xvec() ); }		  \
	inline constexpr type operator/( float f ) const noexcept { return from_xvec( to_xvec() / f ); }							  \
	inline constexpr type& operator/=( const type& o ) noexcept { return *this = from_xvec( to_xvec() / o.to_xvec() ); }	  \
	inline constexpr type& operator/=( float f ) noexcept { return *this = from_xvec( to_xvec() / f ); }						  \
	inline constexpr type operator-() const noexcept { return from_xvec( -to_xvec() ); };											  \
	inline constexpr auto operator<=>( const type& ) const noexcept = default;															  \
	inline constexpr float length_sq() const noexcept { auto v = to_xvec(); return vec::reduce_add( v * v ); }		        \
	inline float length() const noexcept { return fsqrt( length_sq() ); }
				
	struct vec2
	{
		float x = 0;
		float y = 0;

		// Define xvector conversion.
		//
		inline static constexpr vec2 from_xvec( xvec<float, 2> o ) { return vec2{ o[ 0 ],o[ 1 ] }; }
		inline constexpr xvec<float, 2> to_xvec() const noexcept { return { x, y }; }

		// Add operators.
		//
		ADD_OPERATORS( vec2 );

		// String conversion and serialization.
		//
		inline auto tie() { return std::tie( x, y ); }
		inline std::string to_string() const noexcept { return fmt::str( "(%f, %f)", x, y ); }
	};
	struct vec3
	{
		float x = 0;
		float y = 0;
		float z = 0;

		// Extension helper.
		//
		inline static constexpr vec3 from( const vec2& v, float z = 0 ) { return { v.x, v.y, z }; }
		inline constexpr vec2 xy() const noexcept { return { x, y }; }

		// Define xvector conversion.
		//
		inline static constexpr vec3 from_xvec( xvec<float, 3> o ) { return vec3{ o[ 0 ],o[ 1 ],o[ 2 ] }; }
		inline constexpr xvec<float, 3> to_xvec() const noexcept { return { x, y, z }; }

		// Add operators.
		//
		ADD_OPERATORS( vec3 );

		// String conversion and serialization.
		//
		inline auto tie() { return std::tie( x, y, z ); }
		inline std::string to_string() const noexcept { return fmt::str( "(%f, %f, %f)", x, y, z ); }
	};
	struct vec4
	{
		float x = 0;
		float y = 0;
		float z = 0;
		float w = 0;

		// Extension helper.
		//
		inline static constexpr vec4 from( const vec2& v, float z = 0, float w = 1 ) { return { v.x, v.y, z, w }; }
		inline static constexpr vec4 from( const vec3& v, float w = 1 ) { return { v.x, v.y, v.z, w }; }
		inline constexpr vec2 xy() const noexcept { return { x, y }; }
		inline constexpr vec3 xyz() const noexcept { return { x, y, z }; }

		// Define xvector conversion.
		//
		inline static constexpr vec4 from_xvec( xvec<float, 4> o ) { return vec4{ o[ 0 ],o[ 1 ],o[ 2 ],o[ 3 ] }; }
		inline constexpr xvec<float, 4> to_xvec() const noexcept { return { x, y, z, w }; }

		// Add operators.
		//
		ADD_OPERATORS( vec4 );

		// String conversion and serialization.
		//
		inline auto tie() { return std::tie( x, y, z, w ); }
		inline std::string to_string() const noexcept { return fmt::str( "(%f, %f, %f, %f)", x, y, z, w ); }
	};
	using quaternion = vec4;

	template<typename T>
	concept Vector = std::is_same_v<vec2, T> || std::is_same_v<vec3, T> || std::is_same_v<vec4, T>;
#undef ADD_OPERATORS

	// Extended vector helpers.
	//
	template<typename V>
	inline V normalize( const V& vec ) 
	{
		return vec / fsqrt( fmax( flt_eps, vec.length_sq() ) );
	}
	template<typename V>
	inline constexpr float dot( const V& v1, const V& v2 )
	{
		return vec::reduce_add( v1.to_xvec() * v2.to_xvec() );
	}
	template<typename V>
	inline constexpr V lerp( const V& v1, const V& v2, float s )
	{
		if constexpr ( Vector<V> )
		{
			auto v1x = v1.to_xvec();
			auto v2x = v2.to_xvec();
			return V::from_xvec( lerp( v1x, v2x, s ) );
		}
		else
		{
			return v1 + ( v2 - v1 ) * s;
		}
	}
	template<typename V>
	inline constexpr V vec_max( const V& v1, const V& v2 )
	{
		return V::from_xvec( vec::max( v1.to_xvec(), v2.to_xvec() ) );
	}
	template<typename V>
	inline constexpr V vec_min( const V& v1, const V& v2 )
	{
		return V::from_xvec( vec::min( v1.to_xvec(), v2.to_xvec() ) );
	}

	inline constexpr vec3 cross( const vec3& v1, const vec3& v2 ) 
	{
		return {
			v1.y * v2.z - v1.z * v2.y,
			v1.z * v2.x - v1.x * v2.z,
			v1.x * v2.y - v1.y * v2.x
		};
	}
	inline constexpr vec4 cross( const vec4& v1, const vec4& v2, const vec4& v3 )
	{
		return {
		     v1.y * ( v2.z * v3.w - v3.z * v2.w ) - v1.z * ( v2.y * v3.w - v3.y * v2.w ) + v1.w * ( v2.y * v3.z - v2.z * v3.y ),
		  -( v1.x * ( v2.z * v3.w - v3.z * v2.w ) - v1.z * ( v2.x * v3.w - v3.x * v2.w ) + v1.w * ( v2.x * v3.z - v3.x * v2.z ) ),
		     v1.x * ( v2.y * v3.w - v3.y * v2.w ) - v1.y * ( v2.x * v3.w - v3.x * v2.w ) + v1.w * ( v2.x * v3.y - v3.x * v2.y ),
		  -( v1.x * ( v2.y * v3.z - v3.y * v2.z ) - v1.y * ( v2.x * v3.z - v3.x * v2.z ) + v1.z * ( v2.x * v3.y - v3.x * v2.y ) )
		};
	}
	inline constexpr quaternion inverse( const quaternion& q )
	{
		float norm = fmax( flt_min, q.length_sq() );
		return { -q.x / norm, -q.y / norm, -q.z / norm, q.w / norm, };
	}

	// Define matrix type.
	//
	struct matrix4x4
	{
		std::array<std::array<float, 4>, 4> m = {};

		// Identity matrix.
		//
		inline static constexpr matrix4x4 identity() noexcept 
		{
			matrix4x4 result = {};
			result[ 0 ][ 0 ] = 1.0f;
			result[ 1 ][ 1 ] = 1.0f;
			result[ 2 ][ 2 ] = 1.0f;
			result[ 3 ][ 3 ] = 1.0f;
			return result;
		}

		// Matrix mulitplication.
		//
		FORCE_INLINE inline constexpr matrix4x4 operator*( const matrix4x4& other ) const noexcept
		{
			matrix4x4 out;
			#pragma unroll
			for ( size_t i = 0; i != 4; i++ )
				#pragma unroll
				for ( size_t j = 0; j != 4; j++ )
					out.m[ i ][ j ] = m[ i ][ 0 ] * other.m[ 0 ][ j ] + m[ i ][ 1 ] * other.m[ 1 ][ j ] + m[ i ][ 2 ] * other.m[ 2 ][ j ] + m[ i ][ 3 ] * other.m[ 3 ][ j ];
			return out;
		}
		FORCE_INLINE inline constexpr matrix4x4& operator*=( const matrix4x4& other ) noexcept { m = ( *this * other ).m; return *this; }

		// Vector transformation.
		//
		FORCE_INLINE inline constexpr vec4 operator*( const vec4& v ) const noexcept
		{
			return {
				m[ 0 ][ 0 ] * v.x + m[ 1 ][ 0 ] * v.y + m[ 2 ][ 0 ] * v.z + m[ 3 ][ 0 ] * v.w,
				m[ 0 ][ 1 ] * v.x + m[ 1 ][ 1 ] * v.y + m[ 2 ][ 1 ] * v.z + m[ 3 ][ 1 ] * v.w,
				m[ 0 ][ 2 ] * v.x + m[ 1 ][ 2 ] * v.y + m[ 2 ][ 2 ] * v.z + m[ 3 ][ 2 ] * v.w,
				m[ 0 ][ 3 ] * v.x + m[ 1 ][ 3 ] * v.y + m[ 2 ][ 3 ] * v.z + m[ 3 ][ 3 ] * v.w
			};
		}
		FORCE_INLINE inline constexpr vec3 operator*( const vec3& v ) const noexcept
		{
			vec4 res = *this * vec4{ v.x, v.y, v.z, 1 };
			return { res.x, res.y, res.z };
		}

		// Forward indexing.
		//
		inline constexpr std::array<float, 4>& operator[]( size_t n ) noexcept { return m[ n ]; }
		inline constexpr const std::array<float, 4>& operator[]( size_t n ) const noexcept { return m[ n ]; }

		// Default comparison.
		//
		inline constexpr auto operator<=>( const matrix4x4& ) const noexcept = default;
	};

	// Extended matrix helpers.
	//
	inline constexpr matrix4x4 transpose( const matrix4x4& m ) 
	{
		matrix4x4 out;
		for ( size_t i = 0; i != 4; i++ )
			for ( size_t j = 0; j != 4; j++ )
				out[ i ][ j ] = m[ j ][ i ];
		return out;
	}
	inline constexpr float determinant( const matrix4x4& m ) 
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
	inline constexpr matrix4x4 inverse( const matrix4x4& m ) { float tmp = 0; return inverse( m, tmp ); }

	// Define helpers for radian / degrees.
	//
	template<typename T = float>
	inline constexpr auto to_rad( T deg ) { return deg * float( pi / 180.0f ); }
	template<typename T = float>
	inline constexpr auto to_deg( T rad ) { return rad * float( 180.0f / pi ); }

	// Rotation -> (Pitch, Yaw, Roll).
	//
	inline vec3 to_euler( const matrix4x4& m )
	{
		return {
			asinf( -m[ 2 ][ 1 ] ),
			atan2f( m[ 2 ][ 0 ], m[ 2 ][ 2 ] ),
			atan2f( m[ 0 ][ 1 ], m[ 1 ][ 1 ] )
		};
	}
	inline vec3 to_euler( const quaternion& q )
	{
		float sinr_cosp = 2.0f * ( q.w * q.x + q.y * q.z );
		float cosr_cosp = 1.0f - 2.0f * ( q.x * q.x + q.y * q.y );
		float sinp = 2.0f * ( q.w * q.y - q.z * q.x );
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
	inline quaternion rotate_q( float theta, const vec3& axis )
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
	inline matrix4x4 rotate_v( float theta, const vec3& axis )
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

	// Rotating a vector.
	//
	inline vec3 rotate_by( const vec3& v, const matrix4x4& m ) { return m * v; }
	inline vec4 rotate_by( const vec4& v, const matrix4x4& m ) { return m * v; }
	inline vec3 rotate_by( const vec3& v, const quaternion& q )
	{
		vec3 u{ q.x, q.y, q.z };
		return 
			u * 2.0f * dot( u, v ) + 
			v * ( q.w * q.w - dot( u, u ) ) + 
			cross( u, v ) * 2.0f * q.w;
	}

	// Changing the rotation types.
	//
	inline quaternion quaternion_rotation( const matrix4x4& rot )
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
	inline constexpr matrix4x4 matrix_rotation( const quaternion& rot )
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
	inline constexpr matrix4x4 matrix_translation( const vec3& p )
	{
		matrix4x4 out = matrix4x4::identity();
		out[ 3 ][ 0 ] = p.x;
		out[ 3 ][ 1 ] = p.y;
		out[ 3 ][ 2 ] = p.z;
		return out;
	}
	inline constexpr matrix4x4 matrix_scaling( const vec3& p )
	{
		matrix4x4 out = matrix4x4::identity();
		out[ 0 ][ 0 ] = p.x;
		out[ 1 ][ 1 ] = p.y;
		out[ 2 ][ 2 ] = p.z;
		return out;
	}

	// Spherical quaternion interpolation.
	//
	inline constexpr quaternion quaternion_slerp( const quaternion& pq1, const quaternion& pq2, float t )
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
	inline matrix4x4 look_at( const vec3& eye, const vec3& at, vec3 up )
	{
		constexpr float m = LeftHanded ? 1 : -1;

		vec3 vec = normalize( at - eye );
		vec3 right = cross( up, vec );
		up = cross( vec, right );
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
		out[ 0 ][ 2 ] = m * vec.x;
		out[ 1 ][ 2 ] = m * vec.y;
		out[ 2 ][ 2 ] = m * vec.z;
		out[ 3 ][ 2 ] = m * -dot( vec, eye );
		out[ 0 ][ 3 ] = 0.0f;
		out[ 1 ][ 3 ] = 0.0f;
		out[ 2 ][ 3 ] = 0.0f;
		out[ 3 ][ 3 ] = 1.0f;
		return out;
	}
	template<bool LeftHanded = true>
	inline matrix4x4 perspective_fov_y( float fov, float aspect, float zn, float zf )
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
	inline matrix4x4 perspective_fov_x( float fov, float aspect, float zn, float zf )
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