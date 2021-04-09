#pragma once
#include "../ia32.hpp"
#include "iospace.hpp"
#include <vector>
#include <xstd/spinlock.hpp>
#include <xstd/type_helpers.hpp>
#include <xstd/formatting.hpp>
#include <xstd/bitwise.hpp>
#include <xstd/hashable.hpp>

// PCI class codes.
//
#define PCI_BASE_CLASS_NOT_DEFINED     0x00
#define PCI_SUB_CLASS_NOT_DEFINED_VGA        0x01

#define PCI_BASE_CLASS_STORAGE         0x01
#define PCI_SUB_CLASS_STORAGE_SCSI           0x00
#define PCI_SUB_CLASS_STORAGE_IDE            0x01
#define PCI_SUB_CLASS_STORAGE_FLOPPY         0x02
#define PCI_SUB_CLASS_STORAGE_IPI            0x03
#define PCI_SUB_CLASS_STORAGE_RAID           0x04
#define PCI_SUB_CLASS_STORAGE_ATA            0x05
#define PCI_SUB_CLASS_STORAGE_SATA           0x06
#define PCI_SUB_CLASS_STORAGE_SAS            0x07
#define PCI_SUB_CLASS_STORAGE_NVME           0x08
#define PCI_SUB_CLASS_STORAGE_OTHER          0x80

#define PCI_BASE_CLASS_NETWORK         0x02
#define PCI_SUB_CLASS_NETWORK_ETHERNET       0x00
#define PCI_SUB_CLASS_NETWORK_TOKEN_RING     0x01
#define PCI_SUB_CLASS_NETWORK_FDDI           0x02
#define PCI_SUB_CLASS_NETWORK_ATM            0x03
#define PCI_SUB_CLASS_NETWORK_ISDN           0x04
#define PCI_SUB_CLASS_NETWORK_OTHER          0x80

#define PCI_BASE_CLASS_DISPLAY         0x03
#define PCI_SUB_CLASS_DISPLAY_VGA            0x00
#define PCI_SUB_CLASS_DISPLAY_XGA            0x01
#define PCI_SUB_CLASS_DISPLAY_3D             0x02
#define PCI_SUB_CLASS_DISPLAY_OTHER          0x80

#define PCI_BASE_CLASS_MULTIMEDIA      0x04
#define PCI_SUB_CLASS_MULTIMEDIA_VIDEO       0x00
#define PCI_SUB_CLASS_MULTIMEDIA_AUDIO       0x01
#define PCI_SUB_CLASS_MULTIMEDIA_PHONE       0x02
#define PCI_SUB_CLASS_MULTIMEDIA_AUDIO_DEV   0x03
#define PCI_SUB_CLASS_MULTIMEDIA_OTHER       0x80

#define PCI_BASE_CLASS_MEMORY          0x05
#define PCI_SUB_CLASS_MEMORY_RAM             0x00
#define PCI_SUB_CLASS_MEMORY_FLASH           0x01
#define PCI_SUB_CLASS_MEMORY_OTHER           0x80

#define PCI_BASE_CLASS_BRIDGE          0x06
#define PCI_SUB_CLASS_BRIDGE_HOST            0x00
#define PCI_SUB_CLASS_BRIDGE_ISA             0x01
#define PCI_SUB_CLASS_BRIDGE_EISA            0x02
#define PCI_SUB_CLASS_BRIDGE_MC              0x03
#define PCI_SUB_CLASS_BRIDGE_PCI             0x04
#define PCI_SUB_CLASS_BRIDGE_PCMCIA          0x05
#define PCI_SUB_CLASS_BRIDGE_NUBUS           0x06
#define PCI_SUB_CLASS_BRIDGE_CARDBUS         0x07
#define PCI_SUB_CLASS_BRIDGE_RACEWAY         0x08
#define PCI_SUB_CLASS_BRIDGE_PCI_SEMI        0x09
#define PCI_SUB_CLASS_BRIDGE_IB_TO_PCI       0x0a
#define PCI_SUB_CLASS_BRIDGE_OTHER           0x80

#define PCI_BASE_CLASS_COMMUNICATION   0x07
#define PCI_SUB_CLASS_COMMUNICATION_SERIAL   0x00
#define PCI_SUB_CLASS_COMMUNICATION_PARALLEL 0x01
#define PCI_SUB_CLASS_COMMUNICATION_MSERIAL  0x02
#define PCI_SUB_CLASS_COMMUNICATION_MODEM    0x03
#define PCI_SUB_CLASS_COMMUNICATION_OTHER    0x80

#define PCI_BASE_CLASS_SYSTEM          0x08
#define PCI_SUB_CLASS_SYSTEM_PIC             0x00
#define PCI_SUB_CLASS_SYSTEM_DMA             0x01
#define PCI_SUB_CLASS_SYSTEM_TIMER           0x02
#define PCI_SUB_CLASS_SYSTEM_RTC             0x03
#define PCI_SUB_CLASS_SYSTEM_PCI_HOTPLUG     0x04
#define PCI_SUB_CLASS_SYSTEM_OTHER           0x80

#define PCI_BASE_CLASS_INPUT           0x09
#define PCI_SUB_CLASS_INPUT_KEYBOARD         0x00
#define PCI_SUB_CLASS_INPUT_PEN              0x01
#define PCI_SUB_CLASS_INPUT_MOUSE            0x02
#define PCI_SUB_CLASS_INPUT_SCANNER          0x03
#define PCI_SUB_CLASS_INPUT_GAMEPORT         0x04
#define PCI_SUB_CLASS_INPUT_OTHER            0x80

#define PCI_BASE_CLASS_DOCKING         0x0a
#define PCI_SUB_CLASS_DOCKING_GENERIC        0x00
#define PCI_SUB_CLASS_DOCKING_OTHER          0x80

#define PCI_BASE_CLASS_PROCESSOR       0x0b
#define PCI_SUB_CLASS_PROCESSOR_386          0x00
#define PCI_SUB_CLASS_PROCESSOR_486          0x01
#define PCI_SUB_CLASS_PROCESSOR_PENTIUM      0x02
#define PCI_SUB_CLASS_PROCESSOR_ALPHA        0x10
#define PCI_SUB_CLASS_PROCESSOR_POWERPC      0x20
#define PCI_SUB_CLASS_PROCESSOR_MIPS         0x30
#define PCI_SUB_CLASS_PROCESSOR_CO           0x40

#define PCI_BASE_CLASS_SERIAL          0x0c
#define PCI_SUB_CLASS_SERIAL_FIREWIRE        0x00
#define PCI_SUB_CLASS_SERIAL_ACCESS          0x01
#define PCI_SUB_CLASS_SERIAL_SSA             0x02
#define PCI_SUB_CLASS_SERIAL_USB             0x03
#define PCI_SUB_CLASS_SERIAL_FIBER           0x04
#define PCI_SUB_CLASS_SERIAL_SMBUS           0x05
#define PCI_SUB_CLASS_SERIAL_INFINIBAND      0x06

#define PCI_BASE_CLASS_WIRELESS        0x0d
#define PCI_SUB_CLASS_WIRELESS_IRDA          0x00
#define PCI_SUB_CLASS_WIRELESS_CONSUMER_IR   0x01
#define PCI_SUB_CLASS_WIRELESS_RF            0x10
#define PCI_SUB_CLASS_WIRELESS_OTHER         0x80

#define PCI_BASE_CLASS_INTELLIGENT     0x0e
#define PCI_SUB_CLASS_INTELLIGENT_I2O        0x00

#define PCI_BASE_CLASS_SATELLITE       0x0f
#define PCI_SUB_CLASS_SATELLITE_TV           0x00
#define PCI_SUB_CLASS_SATELLITE_AUDIO        0x01
#define PCI_SUB_CLASS_SATELLITE_VOICE        0x03
#define PCI_SUB_CLASS_SATELLITE_DATA         0x04

#define PCI_BASE_CLASS_CRYPT           0x10
#define PCI_SUB_CLASS_CRYPT_NETWORK          0x00
#define PCI_SUB_CLASS_CRYPT_ENTERTAINMENT    0x10
#define PCI_SUB_CLASS_CRYPT_OTHER            0x80

#define PCI_BASE_CLASS_SIGNAL          0x11
#define PCI_SUB_CLASS_SIGNAL_DPIO           0x00
#define PCI_SUB_CLASS_SIGNAL_PERF_CTR       0x01
#define PCI_SUB_CLASS_SIGNAL_SYNCHRONIZER   0x10
#define PCI_SUB_CLASS_SIGNAL_OTHER          0x80

#define PCI_BASE_CLASS_OTHERS          0xff

// Implements a simple interface for the PCI config space.
//
namespace ia32::pci
{
	// Basic header shared by all devices.
	//
	struct config_header
	{
		uint16_t vendor_id;
		uint16_t device_id;
		
		uint16_t command;
		uint16_t status;
		
		uint8_t  revision_id;
		uint8_t  prog_if;
		uint8_t  sub_class_code;
		uint8_t  class_code;
		
		uint8_t  cache_line;
		uint8_t  latency_timer;
		uint8_t  header_type    : 7;
		uint8_t  multi_function : 1;
		uint8_t  bist;
	};

	// Device entry.
	//
	struct device
	{
		// Device configuration.
		//
		config_header    config;

		// Extended information.
		//
		union
		{
			uint32_t     subsystem = 0;
			struct
			{
				uint16_t subsystem_id;
				uint16_t subsystem_vendor_id;
			};
		};

		// Saved address.
		//
		pci_address address = {};

		// Config space read/write.
		// -- Register address is in units.
		//
		template<typename T> requires ( !( sizeof( T ) % sizeof( uint32_t ) ) )
		T read_cfg( uint8_t reg ) const
		{
			T result = {};
			pci_config_space.read_range( &result, address + reg, sizeof( T ) / sizeof( uint32_t ) );
			return result;
		}
		template<typename T> requires ( !( sizeof( T ) % sizeof( uint32_t ) ) )
		void write_cfg( uint8_t reg, T value ) const
		{
			pci_config_space.write_range( address + reg, &value, sizeof( T ) / sizeof( uint32_t ) );
		}

		// String conversion.
		//
		std::string to_string() const
		{
			if ( config.vendor_id == 0xFFFF )
				return "[Null]";
			else
				return xstd::fmt::str( "[Device Id = 0x%04x.0x%04x, Vendor Id = 0x%04x.0x%04x, Class %02x:%02x:%02x]", ( uint32_t ) config.device_id, ( uint32_t ) subsystem_id, ( uint32_t ) config.vendor_id, ( uint32_t ) subsystem_vendor_id, ( uint32_t ) config.class_code, ( uint32_t ) config.sub_class_code, (uint32_t) config.prog_if );
		}
	};
	
	// Extended header fields we are interested in:
	//
	struct header_detail
	{
		xstd::member_reference_t<device, uint32_t> field;
		uint16_t                                   offset;
	};
	inline constexpr std::initializer_list<header_detail> header_extensions[] =
	{
		// Type 0 - Host bridge.
		{ { &device::subsystem, 0x2C / 4 } },
		
		// Type 1 - PCI-to-PCI bridge.
		{},

		// Type 2 - PCI-to-CardBus bridge.
		{ { &device::subsystem, 0x40 / 4 } },
	};

	// Device list enumeration.
	//
	FORCE_INLINE inline std::vector<device> enumerate()
	{
		std::vector<device> result = {};

		// Iterate every Bus:Device:Function combination possible:
		//
		for ( uint32_t bus = 0; bus < 256; bus++ )
		{
			result.reserve( result.size() + 32 * 8 );
			for ( uint32_t device = 0; device < 32; device++ )
			{
				for ( uint32_t function = 0; function < 8; function++ )
				{
					// Read the vendor/device information.
					//
					pci_address adr{ bus, device, function };
					uint32_t device_id = pci_config_space.read( adr );

					// If device is present:
					//
					if ( uint16_t( device_id & 0xFFFF ) != 0xFFFF )
					{
						// Emplace the device entry.
						//
						auto& dev = result.emplace_back();
						dev.address = adr;
						dev.config.vendor_id = device_id & 0xFFFF;
						dev.config.device_id = ( device_id >> 16 ) & 0xFFFF;

						// Read the rest of the header.
						//
						pci_config_space.read_range(
							std::next( &dev.config.device_id ),
							adr + 1,
							( sizeof( config_header ) / sizeof( uint32_t ) ) - 1
						);

						// If valid header type:
						//
						if ( dev.config.header_type < std::size( header_extensions ) )
						{
							// Read the extended details of interest:
							//
							for ( auto& entry : header_extensions[ dev.config.header_type ] )
								dev.*entry.field = pci_config_space.read( adr + entry.offset );
						}

						// If first function and not marked multi-function, break out of function iteration.
						//
						if ( function == 0 && !dev.config.multi_function )
							break;
					}
					// Otherwise, if first function, skip to the next device.
					//
					else if ( function == 0 )
					{
						break;
					}
				}
			}
		}

		// Return the device list.
		//
		return result;
	}

	// Cached device list.
	//
	inline xstd::spinlock device_list_lock = {};
	inline std::vector<device> device_list = {};
	RINLINE inline const std::vector<device>& get_device_list( bool force_update = false )
	{
		std::lock_guard _g{ device_list_lock };
		if ( force_update || device_list.empty() )
			device_list = enumerate();
		return device_list;
	}
};