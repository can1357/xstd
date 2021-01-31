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
		FORCE_INLINE constexpr B operator()( const A& a, const B& b ) const noexcept
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

		// Drawing primitives.
		//
		template<typename F>
		FORCE_INLINE constexpr void line( size_t x1, size_t y1, size_t x2, size_t y2, element_type color, F&& blender ) const
		{
			// Calculcate the delta for the slope.
			//
			int64_t dx = ( int64_t ) ( x2 - x1 );
			int64_t dy = ( int64_t ) ( y2 - y1 );
			
			// Constraint the points within the image boundaries.
			//
			size_t w = width();
			size_t h = height();
			if ( x1 >= w ) x1 = w - 1;
			if ( x2 >= w ) x2 = w - 1;
			if ( y1 >= h ) y1 = h - 1;
			if ( y2 >= h ) y2 = h - 1;

			// Calculate the constrained delta.
			//
			int64_t cdx = ( int64_t ) ( x2 - x1 );
			int64_t cdy = ( int64_t ) ( y2 - y1 );
			if ( !cdx && !cdy ) return;

			// Fast paths for veritcal / horizontal:
			//
			auto it = &at( x1, y1 );
			if ( !dy )
			{
				// If going backwards, normalize so that this ends up effectively being memset.
				//
				if ( cdx < 0 )
				{
					it += cdx;
					cdx = -cdx;
				}

				// Loop through every point.
				//
				for ( int64_t n = 0; n <= cdx; n++ )
				{
					auto p = it + n;
					dassert( begin_ptr() <= p && p < end_ptr() );
					*p = blender( *p, color );
				}
			}
			else if ( !dx )
			{
				// Loop through every point.
				//
				int64_t step = cdy > 0 ? spitch() : -spitch();
				int64_t end = cdy * spitch() + step;
				for ( int64_t n = 0; n != end; n += step )
				{
					auto p = it + n;
					dassert( begin_ptr() <= p && p < end_ptr() );
					*p = blender( *p, color );
				}
			}
			// Slow path with proper slope calculation:
			//
			else
			{
				size_t cx = std::abs( cdx );
				size_t cy = std::abs( cdy );
				int64_t sy = spitch();

				// X used as step counter:
				//
				if ( cx >= cy )
				{
					// Calculate the slope and counter direction.
					//
					float m = float( dy ) / float( dx );
					int64_t d = cdx > 0 ? +1 : -1;

					// Loop through the counter.
					//
					for ( int64_t xd = 0; xd <= cdx; xd += d )
					{
						auto p = it + xd + ( int64_t( m * xd ) * sy );
						dassert( begin_ptr() <= p && p < end_ptr() );
						*p = blender( *p, color );
					}
				}
				// Y used as step counter:
				//
				else
				{
					// Calculate the slope and counter direction.
					//
					float m = float( dx ) / float( dy );
					int64_t d = cdy > 0 ? +1 : -1;

					// Loop through the counter.
					//
					for ( int64_t yd = 0; yd <= cdy; yd += d )
					{
						auto p = it + ( yd * sy ) + ( int64_t( m * yd ) );
						dassert( begin_ptr() <= p && p < end_ptr() );
						*p = blender( *p, color );
					}
				}
			}
		}
		FORCE_INLINE constexpr void line( size_t x1, size_t y1, size_t x2, size_t y2, element_type color ) const
		{
			return line( x1, y1, x2, y2, color, no_blend{} );
		}

		// TODO: Fill rect.
		// TODO: Draw rect.
		// TODO: Draw circle.
	};
	
	// Default view type.
	//
	using image_view = basic_image_view<color_model::argb, image_orientation::top_down>;
};
