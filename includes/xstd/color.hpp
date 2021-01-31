#pragma once
#include <cmath>
#include <algorithm>
#include "type_helpers.hpp"

namespace xstd
{
	// Define the color models.
	//
	enum class color_model
	{
		// Grayscale
		//
		monochrome,
		grayscale,

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
	struct color<color_model::rgb>
	{
		uint8_t b;
		uint8_t g;
		uint8_t r;

		template<color_model dst>
		inline constexpr operator color<dst>() const { return cast_color<dst>( *this ); }
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
		return std::bit_cast<argb_t>( src );
	}
	template<>
	constexpr argb_t to_argb<color_model::ahsv>( const ahsv_t& src )
	{
		if ( !std::is_constant_evaluated() )
		{
			argb_t res;
			float rs = 1 + src.s * ( cos( src.h ) - 1 );
			float gs = 1 + src.s * ( cos( src.h - 2.09439 ) - 1 );
			float bs = 1 + src.s * ( cos( src.h + 2.09439 ) - 1 );
			res.b = ( uint8_t ) std::clamp<int>( bs * src.v * 256.0f, 0, 255 );
			res.g = ( uint8_t ) std::clamp<int>( gs * src.v * 256.0f, 0, 255 );
			res.r = ( uint8_t ) std::clamp<int>( rs * src.v * 256.0f, 0, 255 );
			res.a = ( uint8_t ) std::clamp<int>( src.a * 256.0f, 0, 255 );
			return res;
		}
		unreachable();
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
		return std::bit_cast<xrgb_t>( src );
	}
	template<>
	constexpr ahsv_t from_argb<color_model::ahsv>( const argb_t& src )
	{
		constexpr float sqrt3 = 1.732050807570f;

		if ( !std::is_constant_evaluated() )
		{
			ahsv_t res;
			float rs = src.r / 256.0f;
			float gs = src.g / 256.0f;
			float bs = src.b / 256.0f;
			float as = src.a / 256.0f;

			float c = rs + gs + bs;
			float p = 2 * sqrtf( bs * bs + gs * gs + rs * rs - gs * rs - bs * gs - bs * rs );
			res.h = atan2( bs - gs, ( 2 * rs - bs - gs ) / sqrt3 );
			res.s = p / ( c + p );
			res.v = ( c + p ) / 3;
			res.a = as;
			return res;
		}
		unreachable();
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
	FORCE_INLINE static constexpr color<dst> cast_color( const color<src>& s ) { return from_argb<dst>( to_argb<src>( s ) ); }
};
