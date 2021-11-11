#pragma once
#include <stdint.h>
#include <optional>
#include <string_view>
#include <xstd/small_vector.hpp>
#include <xstd/guid.hpp>
#include <xstd/result.hpp>
#include <xstd/text.hpp>

// Implements an SMBIOS parser.
//
namespace ia32::smbios
{
	// Magic numbers used during the anchor discovery.
	//
	static constexpr std::string_view anchor_v2 = "_SM_";
	static constexpr std::string_view anchor_v3 = "_SM3_";
	static constexpr std::string_view int_anchor = "_DMI_";

	// Specification types.
	//
#pragma pack(push, 1)
	// Entry points.
	//
	struct entry_point_v2
	{
		char     anchor[ anchor_v2.size() ];
		uint8_t  ep_checksum;
		uint8_t  ep_length;
		uint8_t  major_version;
		uint8_t  minor_version;
		uint16_t maximum_structure_size;
		uint8_t  ep_revision;
		uint8_t  ep_revision_reserved[ 5 ];
		char     intermediate_anchor[ int_anchor.size() ];
		uint8_t  intermediate_checksum;
		uint16_t total_length;
		uint32_t address;
		uint16_t num_structures;
		uint8_t  bcd_revision;
	};
	static_assert( sizeof( entry_point_v2 ) == 0x1F, "Entry point does not match the specification." );
	struct entry_point_v3
	{
		char     anchor[ anchor_v3.size() ];
		uint8_t  ep_checksum;
		uint8_t  ep_length;
		uint8_t  major_version;
		uint8_t  minor_version;
		uint8_t  docrev;
		uint8_t  ep_revision;
		uint8_t  ep_revision_reserved[ 1 ];
		uint32_t total_length;
		uint64_t address;
	};
	static_assert( sizeof( entry_point_v3 ) == 0x18, "Entry point does not match the specification." );

	// Entry header.
	//
	struct entry_header
	{
		uint8_t  type;
		uint8_t  length;
		uint16_t handle;
	};

	// Entry types.
	//
	struct string_t { uint8_t raw; };
	struct bios_entry
	{
		static constexpr uint8_t type_id = 0;

		string_t vendor;
		string_t version;
		uint16_t starting_segment;
		string_t release_data;
		uint8_t  rom_size;
		uint8_t  characteristics[ 8 ];
		uint8_t  extension_bytes[ 2 ];
		uint8_t  major_release;
		uint8_t  minor_release;
		uint8_t  firmware_major_release;
		uint8_t  firmware_minor_release;
	};
	struct sysinfo_entry
	{
		static constexpr uint8_t type_id = 1;
		
		// 2.0+
		string_t   manufacturer;
		string_t   product_name;
		string_t   version;
		string_t   serial_number;
		
		// 2.1+
		xstd::guid uuid;
		uint8_t    wakeup_type;
		
		// 2.4+
		string_t   sku_number;
		string_t   family;
	};
	struct baseboard_entry
	{
		static constexpr uint8_t type_id = 2;

		// 2.0+
		string_t manufacturer;
		string_t product;
		string_t version;
		string_t serial_number;
		string_t asset_tag;
		uint8_t  feature_flags;
		string_t location_in_chassis;
		uint16_t chassis_handle;
		uint8_t  board_type;
	};
	struct system_enclosure_entry
	{
		static constexpr uint8_t type_id = 3;

		// 2.0+
		string_t manufacturer;
		uint8_t  type;
		string_t version;
		string_t serial_number;
		string_t asset_tag;

		// 2.1+
		uint8_t  bootup_state;
		uint8_t  psu_state;
		uint8_t  thermal_state;
		uint8_t  security_state;

		// 2.3+
		uint32_t oem_defined;
		uint8_t  height;
		uint8_t  num_pow_cords;
	};
	struct processor_entry
	{
		static constexpr uint8_t type_id = 4;

		// 2.0+
		string_t socket_designation;
		uint8_t  processor_type;
		uint8_t  processor_family;
		string_t processor_manufacturer;
		uint8_t  processor_id[8];
		string_t processor_version;
		uint8_t  voltage;
		uint16_t external_clock;
		uint16_t max_speed;
		uint16_t current_speed;
		uint8_t  status;
		uint8_t  processor_upgrade;

		// 2.1+
		uint16_t l1_cache_handle;
		uint16_t l2_cache_handle;
		uint16_t l3_cache_handle;

		// 2.3+
		string_t serial_number;
		string_t asset_tag;
		string_t part_number;

		// 2.5+
		uint8_t  core_count;
		uint8_t  core_enabled;
		uint8_t  thread_count;
		uint16_t characteristics;
	};
	struct sysslot_entry
	{
		static constexpr uint8_t type_id = 9;

		// 2.0+
		string_t designation;
		uint8_t  type;
		uint8_t  data_bus_width;
		uint8_t  current_usage;
		uint8_t  slot_length;
		uint16_t slot_id;
		uint8_t  characteristics1;

		// 2.1+
		uint8_t  characteristics2;

		// 2.6+
		uint16_t segment_group_number;
		uint8_t  bus_number;
		uint8_t  device_or_function_number;
	};
	struct physical_memory_entry
	{
		static constexpr uint8_t type_id = 16;

		// 2.1+
		uint8_t  location;
		uint8_t  use;
		uint8_t  error_correction;
		uint32_t maximum_capacity;
		uint16_t error_information_handle;
		uint16_t num_devices;

		// 2.7+
		uint64_t maximum_capacity_ex;
	};
	struct memory_device_entry
	{
		static constexpr uint8_t type_id = 17;

		// 2.1+
		uint16_t physical_array_handle;
		uint16_t error_info_handle;
		uint16_t total_width;
		uint16_t data_width;
		uint16_t size;
		uint8_t  form_factor;
		uint8_t  device_set;
		string_t device_locator;
		string_t bank_locator;
		uint8_t  memory_type;
		uint16_t type_detail;

		// 2.3+
		uint16_t speed;
		string_t manufacturer;
		string_t serial_number;
		string_t asset_tag;
		string_t part_number;
	};
#pragma pack(pop)

	// Checksum helper.
	//
	template<typename T>
	inline bool checksum( const T* tbl )
	{
		uint8_t chk = 0;
		for ( size_t n = 0; n != tbl->ep_length; n++ )
			chk += xstd::ref_at<uint8_t>( tbl, n );
		return chk == 0;
	}

	// Parsed types.
	//
	struct entry
	{
		xstd::range<const char*> data = { nullptr, nullptr };
		xstd::small_vector<std::string_view, 8> strings = {};

		// Helpers to read type specific information.
		//
		template<typename T>
		T as() const
		{
			if ( data.size() >= sizeof( T ) )
				return *( T* ) data.begin();

			char raw[ sizeof( T ) ];
			auto it = std::copy( data.begin(), data.end(), std::begin( raw ) );
			std::fill( it, std::end( raw ), 0 );
			return *( T* ) &raw[ 0 ];
		}
		std::string_view resolve( string_t str ) const
		{
			if ( !str.raw || str.raw > strings.size() )
				return {};
			else
				return strings[ str.raw - 1 ];
		}
	};
	struct table
	{
		std::vector<std::pair<uint8_t, entry>> entries = {};
	};

	// Parser for the string sets following entries.
	// - Returns nullopt on failure.
	//
	inline static std::optional<xstd::small_vector<std::string_view, 16>> parse_strings( std::string_view& range )
	{
		xstd::small_vector<std::string_view, 16> result = {};
		while ( !range.empty() )
		{
			// Find the next terminator, if no match invalid.
			//
			auto it = range.find( '\x0' );
			if ( it == std::string::npos )
				return std::nullopt;

			// If at the front and word-sized or not the first entry, terminator.
			//
			if ( it == 0 )
			{
				if ( !result.empty() )
				{
					range.remove_prefix( 1 );
					return result;
				}
				else if ( range.size() >= 2 && range[ 1 ] == 0 )
				{
					range.remove_prefix( 2 );
					return result;
				}
			}

			// Overflow.
			//
			if ( result.capacity() == result.size() )
				return result;

			// Add to the result list and skip.
			//
			auto& str = result.emplace_back( range.substr( 0, it ) );
			range.remove_prefix( it + 1 );

			// Null it out if it looks like a default string.
			//
			if ( str.find_first_not_of( "x0 " ) == std::string::npos )
				str = {};
			if ( xstd::ifind( str, "asset" ) != std::string::npos ||
				 xstd::ifind( str, "serial" ) != std::string::npos ||
				 xstd::ifind( str, "sernum" ) != std::string::npos )
				str = {};

			// Normalize the string.
			//
			while ( !str.empty() && str.back() == ' ' )
				str.remove_suffix( 1 );

			switch ( xstd::make_ahash( str ) )
			{
				case "Default string"_ahash:
					
				// Attempts at sanity.
				//
				// - Void group
				case "Unknown"_ahash:
				case "Undefined"_ahash:
				case "Empty"_ahash:
				case "[Empty]"_ahash:
				// - No group
				case "No DIMM"_ahash:
				case "No Module Installed"_ahash:
				// - Not group
				case "N/A"_ahash:
				case "Not Settable"_ahash:
				case "Not Provided"_ahash:
				case "Not Specified"_ahash:
				case "Not Available"_ahash:
				case "None"_ahash:
				case "NULL"_ahash:

				// All hail the OEM.
				//
				case "To Be Filled By O.E.M."_ahash:
				case "To Be Filled By OEM"_ahash:
				case "Fill By OEM"_ahash:
				case "OEM"_ahash:
				case "OEM_Define0"_ahash:
				case "OEM_Define1"_ahash:
				case "OEM_Define2"_ahash:
				case "OEM_Define3"_ahash:
				case "OEM_Define4"_ahash:
				case "OEM_Define5"_ahash:
				case "OEM_Define6"_ahash:
				case "OEM_Define7"_ahash:
				case "OEM_Define8"_ahash:
				case "OEM_Define9"_ahash:
				case "OEM String"_ahash:
				case "OEM Define 0"_ahash:
				case "OEM Define 1"_ahash:
				case "OEM Define 2"_ahash:
				case "OEM Define 3"_ahash:
				case "OEM Define 4"_ahash:
				case "OEM Define 5"_ahash:
				case "OEM Define 6"_ahash:
				case "OEM Define 7"_ahash:
				case "OEM Define 8"_ahash:
				case "OEM Define 9"_ahash:
				case "OEM-specific"_ahash:
				case "<OUT OF SPEC>"_ahash:

				// Autism friendly placeholders.
				//
				case "System Product Name"_ahash:
				case "System Version"_ahash:
				case "Base Board"_ahash:
				case "SKU Number"_ahash:
				case "SKU"_ahash:

				// Credit card numbers.
				//
				case "0"_ahash:
				case "1.0"_ahash:
				case "1234567"_ahash:
				case "12345678"_ahash:
				case "0123456789"_ahash:
				case "1234567890"_ahash:
				case "9876543210"_ahash:
				case "0987654321"_ahash:
				case "03142563"_ahash:
				case "FFFF"_ahash:
				case "FFFFFFFF"_ahash:
				case "FFFFFFFFFFFFFFFF"_ahash:
				
				// Brain damage.
				//
				case "*"_ahash:
				case "BSN12345678901234567"_ahash:
				case "SQUARE"_ahash:
					str = {};
					break;
				default:
					break;
			}
		}

		// Improperly terminated descriptor.
		//
		return std::nullopt;
	}

	// Given the SMBIOS data range, returns the parsed table.
	//
	inline static xstd::result<table> parse( std::string_view range, bool lenient = true )
	{
		table result = {};
		while ( !range.empty() )
		{
			// Read the entry header.
			//
			if ( range.size() < sizeof( entry_header ) )
				return result;
			const entry_header* hdr = ( entry_header* ) range.data();
			
			// If end of table, break.
			//
			if ( hdr->type == 127 )
				break;

			// Validate the entry, forward the iterator.
			//
			if ( hdr->length < sizeof( entry_header ) )
				return xstd::exception{ XSTD_ESTR( "Invalid SMBIOS entry header." ) };
			if ( range.size() < hdr->length )
				return xstd::exception{ XSTD_ESTR( "SMBIOS entry overflows the range." ) };

			// Parse the strings, save the entry and forward the iterator.
			//
			auto& tbl_entry = result.entries.emplace_back( hdr->type, entry{} ).second;
			tbl_entry.data = { range.data() + sizeof( entry_header ), range.data() + hdr->length };
			range.remove_prefix( hdr->length );
			auto strings = parse_strings( range );
			if ( !strings && !lenient )
				return xstd::exception{ XSTD_ESTR( "Failed parsing SMBIOS entry strings." ) };
			tbl_entry.strings = std::move( strings ).value();
		}
		return result;
	}
};