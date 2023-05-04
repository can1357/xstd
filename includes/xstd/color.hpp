#pragma once
#include <cmath>
#include <algorithm>
#include <bit>
#include "formatting.hpp"
#include "type_helpers.hpp"

namespace xstd
{
	namespace impl
	{
		static constexpr float pi = ( float ) 3.14159265358979323846;
		inline constexpr float fmodcx( float a, float b ) 
		{
			float m = 1.0f / b;
			return a - int32_t( a * m ) * b;
		}
		inline constexpr float normalize_angle_pos( float rad ) 
		{
			float x = fmodcx( rad, 2 * pi );
			if ( x < 0 )
				return ( 2 * pi ) + x;
			else
				return x;
		}
	};

	// Define the color models.
	//
	enum class color_model
	{
		// Grayscale
		//
		monochrome,
		grayscale,
		grayscale_alpha,

		// RGB
		//
		rgb,
		argb,
		xrgb,

		// HSV
		//
		hsv,
		ahsv,
	};
	template<color_model>
	struct color;

	// Declare the cast helper.
	//
	template<color_model dst, color_model src>
	inline constexpr color<dst> cast_color( const color<src>& s );

	// Define the color data types.
	//
#pragma pack(push, 1)
	template<>
	struct color<color_model::monochrome>
	{
		bool white;

		template<color_model dst>
		inline constexpr operator color<dst>() const { return cast_color<dst>( *this ); }
	};
	using monochrome_t = color<color_model::monochrome>;

	template<>
	struct color<color_model::grayscale>
	{
		uint8_t lightness;

		template<color_model dst>
		inline constexpr operator color<dst>() const { return cast_color<dst>( *this ); }
	};
	using grayscale_t = color<color_model::grayscale>;
	
	template<>
	struct color<color_model::grayscale_alpha>
	{
		uint8_t lightness;
		uint8_t alpha;

		template<color_model dst>
		inline constexpr operator color<dst>() const { return cast_color<dst>( *this ); }
	};
	using grayscale_alpha_t = color<color_model::grayscale_alpha>;

	template<>
	struct color<color_model::rgb>
	{
		uint8_t b;
		uint8_t g;
		uint8_t r;

		template<color_model dst>
		inline constexpr operator color<dst>() const { return cast_color<dst>( *this ); }

		std::string to_string() const { return fmt::str( "#%02x%02x%02x", r, g, b ); }
	};
	using rgb_t = color<color_model::rgb>;

	template<>
	struct color<color_model::argb>
	{
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t a;

		template<color_model dst>
		inline constexpr operator color<dst>() const { return cast_color<dst>( *this ); }

		std::string to_string() const { return fmt::str( "#%02x%02x%02x%02x", r, g, b, a ); }
	};
	using argb_t = color<color_model::argb>;

	template<>
	struct color<color_model::xrgb>
	{
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t x;

		template<color_model dst>
		inline constexpr operator color<dst>() const { return cast_color<dst>( *this ); }
	};
	using xrgb_t = color<color_model::xrgb>;

	template<>
	struct color<color_model::hsv>
	{
		float h;
		float s;
		float v;

		template<color_model dst>
		inline constexpr operator color<dst>() const { return cast_color<dst>( *this ); }
	};
	using hsv_t = color<color_model::hsv>;

	template<>
	struct color<color_model::ahsv>
	{
		float h;
		float s;
		float v;
		float a;

		template<color_model dst>
		inline constexpr operator color<dst>() const { return cast_color<dst>( *this ); }
	};
	using ahsv_t = color<color_model::ahsv>;
#pragma pack(pop)

	// Conversion to ARGB.
	//
	template<color_model M>
	static constexpr argb_t to_argb( const color<M>& src );

	template<>
	FORCE_INLINE constexpr argb_t to_argb<color_model::monochrome>( const monochrome_t& src )
	{
		uint8_t x = src.white ? 255 : 0;
		return { x, x, x, 255 };
	}
	template<>
	FORCE_INLINE constexpr argb_t to_argb<color_model::grayscale>( const grayscale_t& src )
	{
		return { src.lightness, src.lightness, src.lightness, 255 };
	}
	template<>
	FORCE_INLINE constexpr argb_t to_argb<color_model::grayscale_alpha>( const grayscale_alpha_t& src )
	{
		return { src.lightness, src.lightness, src.lightness, src.alpha };
	}
	template<>
	FORCE_INLINE constexpr argb_t to_argb<color_model::rgb>( const rgb_t& src )
	{
		return { src.b, src.g, src.r, 255 };
	}
	template<>
	FORCE_INLINE constexpr argb_t to_argb<color_model::argb>( const argb_t& src )
	{
		return src;
	}
	template<>
	FORCE_INLINE constexpr argb_t to_argb<color_model::xrgb>( const xrgb_t& src )
	{
		return { .b = src.b, .g = src.g, .r = src.r, .a = src.x };
	}
	template<>
	constexpr argb_t to_argb<color_model::ahsv>( const ahsv_t& src )
	{
		float hh = impl::normalize_angle_pos( src.h ) / ( impl::pi * 60 / 180 );
		uint32_t i = ( uint32_t ) hh;
		float ff = hh - i;
		float p = src.v * ( 1 - src.s );
		float q = src.v * ( 1 - ( src.s * ff ) );
		float t = src.v * ( 1 - ( src.s * ( 1 - ff ) ) );

		std::tuple<float, float, float> res;
		switch ( i )
		{
			case 0: res = { src.v, t, p }; break;
			case 1: res = { q, src.v, p }; break;
			case 2: res = { p, src.v, t }; break;
			case 3: res = { p, q, src.v }; break;
			case 4: res = { t, p, src.v }; break;
			default:
			case 5: res = { src.v, p, q }; break;
		}

		argb_t out = { 0, 0, 0, 0 };
		out.b = ( uint8_t ) std::clamp<float>( std::get<2>( res ) * 255.0f, 0, 255 );
		out.g = ( uint8_t ) std::clamp<float>( std::get<1>( res ) * 255.0f, 0, 255 );
		out.r = ( uint8_t ) std::clamp<float>( std::get<0>( res ) * 255.0f, 0, 255 );
		out.a = ( uint8_t ) std::clamp<float>( src.a * 255.0f, 0, 255 );
		return out;
	}
	template<>
	FORCE_INLINE constexpr argb_t to_argb<color_model::hsv>( const hsv_t& src )
	{
		return to_argb<color_model::ahsv>( { src.h, src.s, src.v, 1.0 } );
	}

	// Conversion from ARGB.
	//
	template<color_model M>
	static constexpr color<M> from_argb( const argb_t& src );

	template<>
	FORCE_INLINE constexpr monochrome_t from_argb<color_model::monochrome>( const argb_t& src )
	{
		uint8_t x = ( ( uint32_t ) ( src.r ) + uint32_t( src.g ) + uint32_t( src.b ) ) / 3;
		return { x >= 128 };
	}
	template<>
	FORCE_INLINE constexpr grayscale_t from_argb<color_model::grayscale>( const argb_t& src )
	{
		uint8_t x = ( ( uint32_t ) ( src.r ) + uint32_t( src.g ) + uint32_t( src.b ) ) / 3;
		return { x };
	}
	template<>
	FORCE_INLINE constexpr grayscale_alpha_t from_argb<color_model::grayscale_alpha>( const argb_t& src )
	{
		uint8_t x = ( ( uint32_t ) ( src.r ) + uint32_t( src.g ) + uint32_t( src.b ) ) / 3;
		return { x, src.a };
	}
	template<>
	FORCE_INLINE constexpr rgb_t from_argb<color_model::rgb>( const argb_t& src )
	{
		return { src.b, src.g, src.r };
	}
	template<>
	FORCE_INLINE constexpr argb_t from_argb<color_model::argb>( const argb_t& src )
	{
		return src;
	}
	template<>
	FORCE_INLINE constexpr xrgb_t from_argb<color_model::xrgb>( const argb_t& src )
	{
		return { .b = src.b, .g = src.g, .r = src.r, .x = src.a };
	}
	template<>
	constexpr ahsv_t from_argb<color_model::ahsv>( const argb_t& src )
	{
		float fr = src.r / 255.0f;
		float fg = src.g / 255.0f;
		float fb = src.b / 255.0f;
		float fa = src.a / 255.0f;

		float rmin = std::min<float>( { fr, fg, fb } );
		float rmax = std::max<float>( { fr, fg, fb } );

		ahsv_t out = { 0, 0, 0, 0 };
		out.a = fa;
		out.v = rmax;
		
		float delta = rmax - rmin;
		if ( delta < 0.001 )
			return out;

		out.s = delta / rmax;

		if ( fr == rmax )
			out.h = ( fg - fb ) / delta;
		else if ( fg == rmax )
			out.h = 2.0f + ( fb - fr ) / delta;
		else
			out.h = 4.0f + ( fr - fg ) / delta;
		out.h = impl::normalize_angle_pos( out.h * ( impl::pi * 60 / 180 ) );
		return out;
	}
	template<>
	FORCE_INLINE constexpr hsv_t from_argb<color_model::hsv>( const argb_t& src )
	{
		ahsv_t v = from_argb<color_model::ahsv>( src );
		return { v.h, v.s, v.v };
	}

	// Declare conversions between color models.
	//
	template<color_model dst, color_model src>
	FORCE_INLINE inline constexpr color<dst> cast_color( const color<src>& s ) { return from_argb<dst>( to_argb<src>( s ) ); }
	
	// Helpers to create darker / lighter colors.
	//
	template<typename T>
	FORCE_INLINE inline constexpr T lighten_color( T color, float perc )
	{
		argb_t rcol = argb_t( color );

		float fr = ( float ) rcol.r;
		float fg = ( float ) rcol.g;
		float fb = ( float ) rcol.b;

		// Effectively lower HSV.S.
		//
		float fmax = std::max<float>( { fr, fg, fb } );
		fr = fr + ( fmax - fr ) * perc;
		fg = fg + ( fmax - fg ) * perc;
		fb = fb + ( fmax - fb ) * perc;

		rcol.r = ( uint8_t ) fr;
		rcol.g = ( uint8_t ) fg;
		rcol.b = ( uint8_t ) fb;
		return T( rcol );
	}
	template<typename T>
	FORCE_INLINE inline constexpr T darken_color( T color, float perc )
	{
		argb_t rcol = argb_t( color );

		// Effectively lower HSV.V.
		//
		rcol.r = uint8_t( rcol.r * ( 1.0f - perc ) );
		rcol.g = uint8_t( rcol.g * ( 1.0f - perc ) );
		rcol.b = uint8_t( rcol.b * ( 1.0f - perc ) );
		return T( rcol );
	}
};
