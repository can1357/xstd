#pragma once
#include <vector>
#include "type_helpers.hpp"
#include "color.hpp"
#include "serialization.hpp"
#include "result.hpp"
#include "narrow_cast.hpp"
#include "image_view.hpp"

namespace xstd
{
	// Magic numbers.
	//
	static constexpr uint16_t bmp_signature = 0x4D42; // 'BM'

	// File headers.
	//
#pragma pack(push, 1)
	struct dib_header_t
	{
		// Size of the header itself.
		//
		uint32_t     header_size;

		// Image dimensions.
		//
		int32_t      width;
		int32_t      height;
		uint16_t     planes;
		uint16_t     bits_per_pixel;

		// Storage details.
		//
		uint32_t     compression;
		uint32_t     size_image;

		// Measurement details.
		//
		uint32_t     x_ppm;
		uint32_t     y_ppm;

		// Color table.
		//
		uint32_t     len_color_table;
		uint32_t     num_color_table_important;
	};
	struct bmp_header_t
	{
		uint16_t     signature;
		uint32_t     file_size;
		uint32_t     reserved;
		uint32_t     offset_image;
		dib_header_t dib;
	};
#pragma pack(pop)

	// Given a span view of a BMP image, invokes the visitor with a xstd::basic_image_view, returns an exception if relevant.
	//
	template<typename F>
	inline xstd::exception visit_bmp( std::span<const uint8_t> stream, F&& func ) 
	{
		// Get the header.
		//
		if ( stream.size() < sizeof( bmp_header_t ) )
			return xstd::exception{ XSTD_ESTR( "Invalid stream length." ) };
		bmp_header_t* header = ( bmp_header_t* ) stream.data();

		// Validate the header.
		//
		if ( header->signature != bmp_signature )
			return xstd::exception{ XSTD_ESTR( "Invalid BMP header." ) };
		if ( header->reserved )
			return xstd::exception{ XSTD_ESTR( "Invalid BMP header." ) };
		if ( header->file_size > stream.size() )
			return xstd::exception{ XSTD_ESTR( "BMP stream underflow." ) };
		// - No compression.
		if ( header->dib.compression != 0 && header->dib.compression != 3 )
			return xstd::exception{ XSTD_ESTR( "BMP compression unrecognized." ) };
		// - Single plane, no color table.
		if ( header->dib.planes != 1 || header->dib.len_color_table )
			return xstd::exception{ XSTD_ESTR( "BMP color format unrecognized." ) };
		if ( header->dib.width <= 0 || header->dib.height == 0 )
			return xstd::exception{ XSTD_ESTR( "Invalid BMP dimensions." ) };

		// Validate the size and read the image.
		//
		bool top_down = header->dib.height < 0;
		size_t line_count = top_down ? -header->dib.height : header->dib.height;
		size_t stream_size = header->dib.width * line_count * ( header->dib.bits_per_pixel / 8 );
		if ( ( header->offset_image + stream_size ) > header->file_size )
			return xstd::exception{ XSTD_ESTR( "BMP pixel stream underflow." ) };

		auto* begin = stream.data() + header->offset_image;
		switch ( header->dib.bits_per_pixel )
		{
			case 24:
			{
				if ( top_down )
					func( basic_image_view<color_model::rgb, image_orientation::top_down>{ ( rgb_t* ) begin, ( uint32_t ) header->dib.width, line_count } );
				else
					func( basic_image_view<color_model::rgb, image_orientation::bottom_up>{ ( rgb_t* ) begin, ( uint32_t ) header->dib.width, line_count } );
				return xstd::exception{};
			}
			case 32:
			{
				if ( top_down )
					func( basic_image_view<color_model::argb, image_orientation::top_down>{ ( argb_t* ) begin, ( uint32_t ) header->dib.width, line_count } );
				else
					func( basic_image_view<color_model::argb, image_orientation::bottom_up>{ ( argb_t* ) begin, ( uint32_t ) header->dib.width, line_count } );
				return xstd::exception{};
			}
			default:
				return xstd::exception{ XSTD_ESTR( "BMP pixel format unrecognized." ) };
		}
	}

	// Given an image view, writes it as a BMP file.
	//
	template<color_model C, image_orientation O>
	inline std::vector<uint8_t> write_bmp( const basic_image_view<C, O>& img ) 
	{
		// Calculate and reserve the memory for output.
		//
		std::vector<uint8_t> out;
		size_t raw_img_size = img.size() * sizeof( color<C> );
		out.resize( sizeof( bmp_header_t ) + raw_img_size );

		// Write the base BMP header.
		//
		bmp_header_t* hdr = ( bmp_header_t* ) out.data();
		hdr->signature = bmp_signature;
		hdr->file_size = out.size();
		hdr->reserved = 0;
		hdr->offset_image = sizeof( bmp_header_t );

		// Write the DIB header.
		//
		hdr->dib.header_size = sizeof( dib_header_t );
		hdr->dib.width = narrow_cast< int32_t >( img.width() );
		hdr->dib.height = O == xstd::image_orientation::top_down ? -narrow_cast< int32_t >( img.height() ) : narrow_cast< int32_t >( img.height() );
		hdr->dib.planes = 1;
		hdr->dib.bits_per_pixel = sizeof( color<C> ) * 8;
		hdr->dib.compression = 0;
		hdr->dib.size_image = raw_img_size;
		hdr->dib.len_color_table = 0;
		hdr->dib.num_color_table_important = 0;

		// Write the image itself.
		//
		memcpy( hdr + 1, img.source, raw_img_size );
		return out;
	}
};