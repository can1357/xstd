#pragma once
#include <atomic>
#include <xstd/bitwise.hpp>
#include "../ia32.hpp"
#include "memory.hpp"

namespace ia32::apic
{
	// Description of APIC registers.
	//
	enum class delivery_mode : uint32_t
	{
		normal =         0,
		low_priority =   1,
		smi =            2,
		nmi =            4,
		init =           5,
		sipi =           6
	};
	enum class shorthand : uint32_t
	{
		none =           0,
		self =           1,
		all =            2,
		all_but_this =   3,
	};
	struct command
	{
		uint32_t       vector      : 8 =  0;
		delivery_mode  mode        : 3 =  delivery_mode::normal;
		uint32_t       is_logical  : 1 =  false;
		uint32_t       is_pending  : 1 =  false;
		uint32_t       reserved1   : 1 =  0;
		uint32_t       level       : 1 =  0;
		uint32_t       trigger     : 1 =  0;
		uint32_t       reserved2   : 2 =  0;
		shorthand      sh_group    : 2 =  shorthand::none;
		uint32_t       reserved3   : 12 = 0;
	};

	// APIC register mappings.
	//
	constexpr uint64_t x2apic_msr =               0x800;
	constexpr uint64_t end_of_int_register =      0x0B0;
	constexpr uint64_t logical_dst_register =     0x0D0;
	constexpr uint64_t dest_fmt_register =        0x0E0;
	constexpr uint64_t in_service_register =      0x100;
	constexpr uint64_t trigger_mode_register =    0x180;
	constexpr uint64_t irequest_register =        0x200; 
	constexpr uint64_t cmd_register =             0x300;
	constexpr uint64_t lvt_timer_register =       0x320;
	constexpr uint64_t lvt_thermal_register =     0x330;
	constexpr uint64_t lvt_pmi_register =         0x340;
	constexpr uint64_t lvt_lint0_register =       0x350;
	constexpr uint64_t lvt_lint1_register =       0x360;
	constexpr uint64_t lvt_error_register =       0x370;
	constexpr uint64_t lvt_init_count_register =  0x380;
	constexpr uint64_t lvt_curr_count_register =  0x390;
	constexpr uint64_t self_ipi_register =        0x3F0; // x2APIC only.

	// Global APIC mapping if relevant.
	//
	inline volatile uint32_t* apic_base = nullptr;
#if __has_xcxx_builtin(__builtin_fetch_dynamic)
	FORCE_INLINE inline void __set_apic_base( volatile uint32_t* value ) { apic_base = value;  __builtin_store_dynamic( "@.apic_base", ( void* ) value ); }
	FORCE_INLINE CONST_FN inline volatile uint32_t* __get_apic_base() { return ( volatile uint32_t* ) __builtin_fetch_dynamic( "@.apic_base" ); }
#else
	FORCE_INLINE inline void __set_apic_base( volatile uint32_t* value ) { apic_base = value; }
	FORCE_INLINE CONST_FN inline volatile uint32_t* __get_apic_base() { return apic_base; }
#endif

	// APIC controller.
	//
	struct controller
	{
		volatile uint32_t* const base_address = __get_apic_base();

		// Check for the state.
		//
		inline bool is_x2apic() const noexcept { return base_address == nullptr; }

		// Read/write register.
		//
		template<typename T = uint32_t> requires ( sizeof( T ) == 4 )
		inline T read_register( size_t i ) const noexcept
		{
			uint32_t value;
			if ( is_x2apic() )
				value = read_msr<uint32_t>( x2apic_msr + i / 0x10 );
			else
				value = base_address[ i / 4 ];
			return xstd::bit_cast< T >( value );
		}
		template<typename T = uint32_t> requires ( sizeof( T ) == 4 )
		inline void write_register( size_t i, T _value ) const noexcept
		{
			uint32_t value = xstd::bit_cast< uint32_t >( _value );
			if ( is_x2apic() )
				write_msr( x2apic_msr + i / 0x10, value );
			else
				base_address[ i / 4 ] = value;
		}

		// Basic properties.
		//
		inline uint32_t read_timer_counter() const noexcept
		{
			return read_register( lvt_curr_count_register );
		}

		// Signals the EOI.
		//
		inline void end_of_interrupt() const noexcept
		{
			write_register( end_of_int_register, 0 );
		}

		// Checks if the given ISR index is in service.
		//
		inline bool in_service( uint8_t idx ) const noexcept
		{
			return ( read_register( in_service_register + ( 0x10 * ( idx / 32 ) ) ) >> ( idx % 32 ) ) & 1;
		}

		// Waits for a command to be finished.
		//
		inline void wait_for_delivery() const noexcept
		{
			if ( !is_x2apic() )
			{
				while ( read_register<command>( cmd_register ).is_pending )
					yield_cpu();
			}
			else
			{
				/*no-op on x2apic*/
			}
		}

		// Sends a command.
		//
		inline void send_command( command cmd, uint32_t dst = 0 ) const noexcept
		{
			if ( !is_x2apic() )
			{
				// Disable interrupts since it is not atomic.
				//
				scope_irql<HIGH_LEVEL> _g{};

				// Wait for the pending flag to clear.
				//
				wait_for_delivery();

				// If shorthand is not used, write the destination.
				//
				if ( cmd.sh_group == shorthand::none )
					write_register( cmd_register + 0x10, dst );

				// Write the command.
				//
				write_register( cmd_register, cmd );
			}
			else
			{
				// If self IPI, used the new self IPI MSR.
				//
				if ( cmd.sh_group == shorthand::self )
					write_msr( x2apic_msr + self_ipi_register / 0x10, cmd.vector );
				else
					write_msr( x2apic_msr + cmd_register / 0x10, ( *( uint32_t* ) &cmd ) | ( uint64_t( dst ) << 32 ) );
			}
		}

		// Simple interfaces to request interrupts.
		//
		inline void request_interrupt( uint8_t vector, shorthand group ) const noexcept
		{
			command cmd = {};
			cmd.vector = vector;
			cmd.sh_group = group;
			send_command( cmd );
		}
		inline void request_interrupt( uint8_t vector, uint32_t identifier ) const noexcept
		{
			command cmd = {};
			cmd.vector = vector;
			send_command( cmd, identifier );
		}
		inline void request_nmi( shorthand group ) const noexcept
		{
			command cmd = {};
			cmd.mode = delivery_mode::nmi;
			cmd.sh_group = group;
			send_command( cmd );
		}
	};

	// Initialization of the APIC base.
	//
	inline bool initialized = false;
	inline bool init()
	{
		// Fail if APIC is not enabled.
		//
		auto apic_info = read_msr<apic_base_register>( IA32_APIC_BASE );
		if ( !apic_info.apic_global_enable )
			return false;

		// If not in x2apic mode, map the memory and assign the APIC base.
		//
		if ( !apic_info.enable_x2apic_mode )
		{
			static mem::unique_phys_ptr<volatile uint32_t> base;
			base = mem::map_physical<volatile uint32_t>( apic_info.apic_base << 12, 0x1000 );
			__set_apic_base( base.get() );
		}
		else
		{
			__set_apic_base( nullptr );
		}
		initialized = true;
		return true;
	}
};