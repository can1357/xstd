#pragma once
#include "type_helpers.hpp"
#include "color.hpp"
#include "assert.hpp"

namespace xstd
{
	// Declare a dummy no-op blender.
	//
	struct no_blend
	{
		template<typename A, typename B>
		FORCE_INLINE constexpr B operator()( const A&, const B& b ) const noexcept
		{
			return b;
		}
	};

	// Define the image orientations.
	//
	enum class image_orientation
	{
		top_down,
		bottom_up,
	};

	template<image_orientation orientation, typename T>
	FORCE_INLINE static constexpr decltype( auto ) address_pixel( T&& begin, size_t width, size_t height, size_t x, size_t y )
	{
		switch ( orientation )
		{
			case image_orientation::top_down:
				return begin + ( x + ( y * width ) );
			case image_orientation::bottom_up:
				return begin + ( x + ( ( height - ( y + 1 ) ) * width ) );
			default:
				unreachable();
		}
	}

	// Declare a generic image view, not const correct on purpose to allow implementation of generic graphics.
	//
	template<color_model C, image_orientation O>
	struct basic_image_view
	{
		// Characteristics of the image viewed.
		//
		using element_type = color<C>;
		static constexpr image_orientation orientation = O;
		static constexpr color_model       element_color_model = C;

		// Runtime characteristics of the image viewed, left trivial to construct/move/copy.
		//
		element_type* source = nullptr;
		size_t source_width = 0;
		size_t source_height = 0;
		size_t offset_x = 0;
		size_t offset_y = 0;
		size_t cutoff_x = 0;
		size_t cutoff_y = 0;

		// Basic properties.
		//
		FORCE_INLINE constexpr size_t width() const { return source_width - ( offset_x + cutoff_x ); }
		FORCE_INLINE constexpr size_t height() const { return source_height - ( offset_y + cutoff_y ); }
		FORCE_INLINE constexpr size_t pitch() const { return source_width; }
		FORCE_INLINE constexpr size_t pitch_bytes() const { return pitch() * sizeof( element_type ); }
		FORCE_INLINE constexpr int64_t spitch() const
		{
			if constexpr ( O == image_orientation::bottom_up )
				return -( int64_t ) pitch();
			else
				return ( int64_t ) pitch();
		}
		FORCE_INLINE constexpr size_t size() const { return width() * height(); }
		FORCE_INLINE constexpr size_t source_size() const { return source_width * source_height; }
		FORCE_INLINE constexpr element_type& at( size_t x, size_t y ) const { return *address_pixel<orientation>( source, source_width, source_height, x + offset_x, y + offset_y ); }
		FORCE_INLINE constexpr element_type& operator()( size_t x, size_t y ) const { return at( x, y ); }
		FORCE_INLINE constexpr element_type* begin_ptr() const 
		{
			if constexpr ( O == image_orientation::bottom_up )
				return &at( 0, height() - 1 );
			else
				return &at( 0, 0 );
		}
		FORCE_INLINE constexpr element_type* end_ptr() const 
		{
			if constexpr ( O == image_orientation::bottom_up )
				return &at( width(), 0 );
			else
				return &at( width(), height() - 1 );
		}

		// Subviews.
		//
		FORCE_INLINE constexpr basic_image_view get_source() const
		{
			return { source, source_width, source_height };
		}
		FORCE_INLINE constexpr basic_image_view subrect( size_t off_x, size_t off_y, size_t cnt_x = std::string::npos, size_t cnt_y = std::string::npos ) const
		{
			// Calculate the cutoff.
			//
			size_t excutoff_x = 0, excutoff_y = 0;
			if ( cnt_x != std::string::npos )
				excutoff_x = ( width() - off_x ) - cnt_x;
			if ( cnt_y != std::string::npos )
				excutoff_y = ( height() - off_y ) - cnt_y;
			
			// Create and return the new view.
			//
			return { source, source_width, source_height, offset_x + off_x, offset_y + off_y, cutoff_x + excutoff_x, cutoff_y + excutoff_y };
		}

		// Blend/Copy.
		//
		template<color_model C2, image_orientation O2, typename F>
		FORCE_INLINE constexpr void blend_to( const basic_image_view<C2, O2>& other, F&& blender, size_t dst_x = 0, size_t dst_y = 0 ) const
		{
			// Calculate the copy count.
			//
			size_t xcnt = other.width();
			if ( xcnt > dst_x ) xcnt = std::min( xcnt - dst_x, width() );
			else                return;
			size_t ycnt = other.height();
			if ( ycnt > dst_y ) ycnt = std::min( ycnt - dst_y, height() );
			else                return;

			// Begin the iteration optimized for vectorization.
			//
			auto* src = &at( 0, 0 );
			int64_t dsrc = spitch();
			auto* dst = &other.at( dst_x, dst_y );
			int64_t ddst = other.spitch();
			
			for ( size_t i = 0; i != ycnt; i++ )
			{
				for ( size_t j = 0; j != xcnt; j++ )
					dst[ j ] = blender( dst[ j ], src[ j ] );
				dst += ddst;
				src += dsrc;
			}
		}
		template<color_model C2, image_orientation O2>
		FORCE_INLINE constexpr void copy_to( const basic_image_view<C2, O2>& other, size_t dst_x = 0, size_t dst_y = 0 ) const
		{
			blend_to( other, no_blend{}, dst_x, dst_y );
		}
	};
	
	// Default view type.
	//
	using image_view = basic_image_view<color_model::argb, image_orientation::top_down>;
};
