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

	// Intermediate representation.
	//
	template<bool bbp32 = false, bool store_top_down = true>
	struct bmp_image
	{
		// Characteristics of the storage format.
		//
		static constexpr image_orientation orientation = store_top_down ? image_orientation::top_down : image_orientation::bottom_up;
		static constexpr color_model       element_color_model = bbp32 ? color_model::xrgb : color_model::rgb;
		using element_type = color<element_color_model>;
		using view_type =    basic_image_view<element_color_model, orientation>;
		
		// The dimensions and the image itself.
		//
		size_t width = 0;
		size_t height = 0;
		std::vector<element_type> data;

		// Default constructible, default copy/move.
		//
		bmp_image() = default;
		bmp_image( bmp_image&& ) noexcept = default;
		bmp_image( const bmp_image& ) = default;
		bmp_image& operator=( bmp_image&& ) noexcept = default;
		bmp_image& operator=( const bmp_image& ) = default;

		// Empty bitmap constructed from dimensions.
		//
		bmp_image( size_t width, size_t height ) : width( width ), height( height )
		{
			data.resize( width * height );
		}
		
		// Bitmap constructed from data source.
		//
		template<color_model C, image_orientation O2>
		bmp_image( const basic_image_view<C, O2>& view ) : width( view.width() ), height( view.height() )
		{
			data.resize( view.width() * view.height() );
			view.copy_to( to_view() );
		}

		// Creates a view.
		//
		view_type to_view() const { return view_type{ ( element_type* ) data.data(), width, height }; }

		// Indexing of the pixels.
		//
		element_type& at( size_t x, size_t y ) { return *address_pixel<orientation>( data.data(), width, height, x, y); }
		const element_type& at( size_t x, size_t y ) const
		{
			return ( const_cast< bmp_image* >( this ) )->at( x, y );
		}
		decltype( auto ) operator()( size_t x, size_t y ) { return at( x, y ); }
		decltype( auto ) operator()( size_t x, size_t y ) const { return at( x, y ); }

		// Deeserializes a bitmap image from the given data.
		//
		static result<bmp_image> deserialize( const std::vector<uint8_t>& stream )
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

			// Make sure the DIB header is in the format we've expected.
			// - RGB or ARGB.
			if ( header->dib.bits_per_pixel != 24 && header->dib.bits_per_pixel != 32 )
				return xstd::exception{ XSTD_ESTR( "BMP pixel format unrecognized." ) };
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
			size_t stream_size = header->dib.width * line_count * (header->dib.bits_per_pixel / 8);
			if ( ( header->offset_image + stream_size ) > header->file_size )
				return xstd::exception{ XSTD_ESTR( "BMP pixel stream underflow." ) };

			// Convert to our format.
			//
			auto* begin = stream.data() + header->offset_image;
			switch ( header->dib.bits_per_pixel )
			{
				case 24: 
				{
					if ( top_down )
						return bmp_image( basic_image_view<color_model::rgb, image_orientation::top_down>{ ( rgb_t* ) begin, header->dib.width, line_count } );
					else
						return bmp_image( basic_image_view<color_model::rgb, image_orientation::bottom_up>{ ( rgb_t* ) begin, header->dib.width, line_count } );
				}
				case 32: 
				{
					if ( top_down )
						return bmp_image( basic_image_view<color_model::argb, image_orientation::top_down>{ ( argb_t* ) begin, header->dib.width, line_count } );
					else
						return bmp_image( basic_image_view<color_model::argb, image_orientation::bottom_up>{ ( argb_t* ) begin, header->dib.width, line_count } );
				}
				default: 
					unreachable();
			}
		}
		static bmp_image deserialize( serialization& ss )
		{
			std::vector<uint8_t> buffer( sizeof( bmp_header_t ) );
			ss.read( buffer.data(), buffer.size() );
			buffer.resize( ( ( bmp_header_t* ) buffer.data() )->file_size );
			ss.read( buffer.data() + sizeof( bmp_header_t ), buffer.size() - sizeof( bmp_header_t ) );
			return *deserialize( buffer );
		}

		// Serializes the bitmap.
		//
		std::vector<uint8_t> serialize() const
		{
			// Calculate and reserve the memory for output.
			//
			std::vector<uint8_t> out;
			size_t raw_img_size = data.size() * sizeof( element_type );
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
			hdr->dib.width = narrow_cast<int32_t>(width);
			hdr->dib.height = store_top_down ? -narrow_cast<int32_t>(height) : narrow_cast<int32_t>(height);
			hdr->dib.planes = 1;
			hdr->dib.bits_per_pixel = sizeof( element_type ) * 8;
			hdr->dib.compression = 0;
			hdr->dib.size_image = raw_img_size;
			hdr->dib.len_color_table = 0;
			hdr->dib.num_color_table_important = 0;

			// Write the image itself.
			//
			memcpy( hdr + 1, data.data(), raw_img_size );
			return out;
		}
		void serialize( serialization& ss ) const
		{
			ss.write( serialize() );
		}
	};
};