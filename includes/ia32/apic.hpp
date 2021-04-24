#pragma once
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
	constexpr uint64_t x2apic_msr =             0x800;
	constexpr uint64_t x2apic_self_ipi_msr =    0x83F;
	constexpr uint64_t cmd_register =           0x300;
	constexpr uint64_t timer_register =         0x390;
	constexpr uint64_t end_of_int_register =    0x0B0;
	constexpr uint64_t logical_dst_register =   0x0D0;
	constexpr uint64_t dest_fmt_register =      0x0E0;

	// Global APIC mapping if relevant.
	//
	inline volatile uint32_t* apic_base = nullptr;

	// Read/write register.
	//
	inline uint32_t read_register( size_t i )
	{
		if ( apic_base )
			return apic_base[ i / 4 ];
		else
			return read_msr<uint32_t>( x2apic_msr + i / 0x10 );
	}
	inline void write_register( size_t i, uint32_t v )
	{
		if ( apic_base )
			apic_base[ i / 4 ] = v;
		else
			write_msr( x2apic_msr + i / 0x10, v );
	}

	// Basic properties.
	//
	inline uint32_t read_timer()
	{
		return read_register( timer_register );
	}
	inline bool is_x2apic()
	{
		return apic_base == nullptr;
	}

	// Signals the EOI.
	//
	inline void end_of_interrupt()
	{
		write_register( end_of_int_register, 0 );
	}

	// Waits for a command to be finished.
	//
	inline void wait_for_command()
	{
		if ( apic_base )
		{
			volatile command& icr = ( volatile command& ) apic_base[ cmd_register / 4 ];
			while ( icr.is_pending )
				yield_cpu();
		}
		else
		{
			/*no-op on x2apic*/
		}
	}

	// Sends a command.
	//
	inline void send_command( command cmd, uint32_t dst = 0, bool wait_finish = true )
	{
		if ( apic_base )
		{
			// Disable interrupts since it is not atomic.
			//
			scope_irql<NO_INTERRUPTS> _g{};

			// Wait for the pending flag to clear.
			//
			volatile command& icr = ( volatile command& ) apic_base[ cmd_register / 4 ];
			while ( icr.is_pending )
				yield_cpu();

			// If shorthand is not used, write the destination.
			//
			if ( cmd.sh_group == shorthand::none )
				apic_base[ ( cmd_register + 0x10 ) / 4 ] = dst;

			// Write the command.
			//
			*( volatile uint32_t* ) &icr = *( uint32_t* ) &cmd;

			// Wait for it to be finished.
			//
			if ( wait_finish )
				wait_for_command();
		}
		else
		{
			// If self IPI, used the new self IPI MSR.
			//
			if ( cmd.sh_group == shorthand::self )
				write_msr( x2apic_self_ipi_msr, cmd.vector );
			else
				write_msr( x2apic_msr + cmd_register / 0x10, ( *( uint32_t* ) &cmd ) | ( uint64_t( dst ) << 32 ) );
		}
	}

	// Simple interfaces to request interrupts.
	//
	inline void request_interrupt( uint8_t vector, shorthand group )
	{
		command cmd = {};
		cmd.vector = vector;
		cmd.sh_group = group;
		send_command( cmd );
	}
	inline void request_interrupt( uint8_t vector, uint32_t identifier )
	{
		command cmd = {};
		cmd.vector = vector;
		send_command( cmd, identifier );
	}
	inline void request_nmi( shorthand group )
	{
		command cmd = {};
		cmd.mode = delivery_mode::nmi;
		cmd.sh_group = group;
		send_command( cmd );
	}
	inline void request_nmi( uint8_t vector, uint32_t identifier )
	{
		command cmd = {};
		cmd.mode = delivery_mode::nmi;
		send_command( cmd, identifier );
	}

	// Initialization of the APIC base.
	//
	inline bool detect_lapic()
	{
		// Fail if APIC is not enabled.
		//
		auto apic_info = ia32::read_msr<ia32::apic_base_register>( IA32_APIC_BASE );
		if ( !apic_info.apic_global_enable )
			return false;

		// If not in x2apic mode, map the memory and assign the APIC base.
		//
		if ( !apic_info.enable_x2apic_mode )
		{
			static std::shared_ptr<volatile uint32_t> base;
			base = ia32::mem::map_physical<volatile uint32_t>( apic_info.apic_base, 0x1000 );
			apic_base = base.get();
		}
		return true;
	}
};