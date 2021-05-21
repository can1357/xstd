#pragma once
#include <stdint.h>
#include <string>
#include <tuple>
#include <xstd/bitwise.hpp>
#include "../ia32.hpp"

// This namespace implements partial support for the performance monitoring extensions 
// implemented by AMD and Intel and tries to unify the interface exposed.
//
namespace ia32::pmu
{
	// Common event identifiers that are automatically resolved based on the platform.
	//
	enum class event_id
	{
		none,

		// Supported by both architectures.
		//
		clock_core,
		br_retire,
		ins_retire,
		uop_retire,
		br_miss_retire,
		llc_miss,

		// Intel only.
		//
		clock_tsc,      // Fixed counter only.
		ins_execute,
		uop_execute,
		uop_dispatch,
		hw_interrupt_receive,
		hw_interrupt_masked,
		hw_interrupt_pending_masked,
		avx_to_sse,
		sse_to_avx,
		recovery_stall,

		// AMD only.
		//
		smi_received,

		max
	};

	// Event selector description.
	//
	struct evtsel_t
	{
		uint32_t event_select = 0;
		uint32_t unit_mask = 0;
		uint32_t count_mask = 0;
		bool invert = false;
		bool edge = false;
		inline explicit operator bool() const { return event_select != 0; }
	};

	// Fixed event counter information.
	//  - Maps the platform and event identifier to a fixed counter index where possible, otherwise maps to npos.
	//
	template<bool is_intel, event_id evt>
	struct fixed_counter { static constexpr size_t value = std::string::npos /*None*/; };
	// -- Intel exclusive fixed counters.
	template<> struct fixed_counter<true, event_id::ins_retire> { static constexpr size_t value = 0; };
	template<> struct fixed_counter<true, event_id::clock_core> { static constexpr size_t value = 1; };
	template<> struct fixed_counter<true, event_id::clock_tsc> { static constexpr size_t value = 2; };

	// Dynamic event selectors.
	//  - Maps the platform and event identifier to a event selector where possible, otherwise maps to the null selector.
	//
	template<bool is_intel, event_id evt>
	struct dynamic_selector { static constexpr evtsel_t value = {}; };
	// -- Common events.
	template<bool is_intel> struct dynamic_selector<is_intel, event_id::none> { static constexpr evtsel_t value = {}; };
	template<bool is_intel> struct dynamic_selector<is_intel, event_id::ins_retire> { static constexpr evtsel_t value = { 0xC0, 0x00 }; };
	template<> struct dynamic_selector<true, event_id::br_retire> { static constexpr evtsel_t value = { 0xC4, 0x00 }; };
	template<> struct dynamic_selector<false, event_id::br_retire> { static constexpr evtsel_t value = { 0xC2, 0x00 }; };
	template<> struct dynamic_selector<true, event_id::uop_retire> { static constexpr evtsel_t value = { 0xC2, 0x01 }; };
	template<> struct dynamic_selector<false, event_id::uop_retire> { static constexpr evtsel_t value = { 0xC1, 0x00 }; };
	template<> struct dynamic_selector<true, event_id::clock_core> { static constexpr evtsel_t value = { 0x3C, 0x00 }; };
	template<> struct dynamic_selector<false, event_id::clock_core> { static constexpr evtsel_t value = { 0x76, 0x00 }; };
	template<> struct dynamic_selector<true, event_id::br_miss_retire> { static constexpr evtsel_t value = { 0xC5, 0x00 }; };
	template<> struct dynamic_selector<false, event_id::br_miss_retire> { static constexpr evtsel_t value = { 0xC3, 0x00 }; };
	template<> struct dynamic_selector<true, event_id::llc_miss> { static constexpr evtsel_t value = { 0x2E, 0x4F }; };
	template<> struct dynamic_selector<false, event_id::llc_miss> { static constexpr evtsel_t value = { 0x43, 0x5B }; };
	// -- Intel exclusive events.
	template<> struct dynamic_selector<true, event_id::ins_execute> { static constexpr evtsel_t value = { 0x16, 0x00 }; };
	template<> struct dynamic_selector<true, event_id::uop_execute> { static constexpr evtsel_t value = { 0xB1, 0x00 }; };
	template<> struct dynamic_selector<true, event_id::uop_dispatch> { static constexpr evtsel_t value = { 0xA1, 0xFF }; };
	template<> struct dynamic_selector<true, event_id::hw_interrupt_receive> { static constexpr evtsel_t value = { 0xCB, 0x01 }; };
	template<> struct dynamic_selector<true, event_id::hw_interrupt_masked> { static constexpr evtsel_t value = { 0xCB, 0x02 }; };
	template<> struct dynamic_selector<true, event_id::hw_interrupt_pending_masked> { static constexpr evtsel_t value = { 0xCB, 0x04 }; };
	template<> struct dynamic_selector<true, event_id::avx_to_sse> { static constexpr evtsel_t value = { 0xC1, 0x10 }; };
	template<> struct dynamic_selector<true, event_id::sse_to_avx> { static constexpr evtsel_t value = { 0xC1, 0x20 }; };
	template<> struct dynamic_selector<true, event_id::recovery_stall> { static constexpr evtsel_t value = { 0x0D, 0x20, 1 }; };


	// -- AMD exclusive events.
	template<> struct dynamic_selector<false, event_id::smi_received> { static constexpr evtsel_t value = { 0x2B, 0x00 }; };

	// Shortcuts.
	//
	template<bool is_intel, event_id evt> inline constexpr size_t   fixed_counter_v =    fixed_counter<is_intel, evt>::value;
	template<bool is_intel, event_id evt> inline constexpr evtsel_t dynamic_selector_v = dynamic_selector<is_intel, evt>::value;

	// Configuration traits for each platform.
	//
	template<bool _is_intel>
	struct traits_of
	{
		static constexpr bool is_intel = _is_intel;

		// Shortcuts.
		//
		template<event_id evt> static constexpr size_t fixed_counter() { return fixed_counter_v<is_intel, evt>; }
		template<event_id evt> static constexpr evtsel_t dynamic_selector() { return dynamic_selector_v<is_intel, evt>; }

		// Status MSRs.
		//
		static constexpr uint32_t status_inuse = is_intel ? IA32_PERF_GLOBAL_INUSE : UINT32_MAX;

		// Counter MSRs.
		//
		static constexpr uint32_t config_base = is_intel ? IA32_PERFEVTSEL0 : IA32_PERFEVTSEL0_AMD;
		static constexpr uint32_t counter_base = is_intel ? IA32_PMC0 : IA32_PMC0_AMD;
		static constexpr uint32_t counter_limit = is_intel ? 8 : 4;
		static constexpr uint32_t counter_stride = 1;

		// Aliasing counter MSRs.
		// - Intel uses these to extend the numeric range, AMD uses these to implement extension counters.
		//
		static constexpr uint32_t aliasing_config_base = is_intel ? IA32_PERFEVTSEL0 : IA32_A_PERFEVTSEL0_AMD;
		static constexpr uint32_t aliasing_counter_base = is_intel ? IA32_A_PMC0 : IA32_A_PMC0_AMD;
		static constexpr uint32_t aliasing_counter_limit = is_intel ? 8 : 6;
		static constexpr uint32_t aliasing_counter_stride = is_intel ? 1 : 2;

		// Control MSRs.
		// - AMD does not use a global config space nor have fixed counters.
		//
		static constexpr uint32_t global_control = is_intel ? IA32_PERF_GLOBAL_CTRL : UINT32_MAX;
		static constexpr uint32_t fixed_control = is_intel ? IA32_FIXED_CTR_CTRL : UINT32_MAX;
		static constexpr uint32_t fixed_counter_base = is_intel ? IA32_FIXED_CTR0 : UINT32_MAX;
		static constexpr uint32_t fixed_counter_limit = is_intel ? 3 : UINT32_MAX;
		static constexpr uint32_t fixed_counter_stride = is_intel ? 1 : UINT32_MAX;

		// CPU capability information.
		//
		inline static size_t get_dynamic_counter_count()
		{
			if constexpr ( is_intel )
			{
				auto& caps = static_cpuid<0xA, 0, cpuid_eax_0a>::result;
				return caps.eax.number_of_performance_monitoring_counter_per_logical_processor;
			}
			else
			{
				auto& caps = static_cpuid<0x80000001, 0, cpuid_eax_80000001>::result;
				if ( caps.ecx.flags & ( 1ull << 23 ) /*[PerfCtrExtCore]*/ )
					return 6;
				else
					return 4;
			}
		}
		inline static size_t get_fixed_counter_count()
		{
			if constexpr ( is_intel )
			{
				auto& caps = static_cpuid<0xA, 0, cpuid_eax_0a>::result;
				return caps.edx.number_of_fixed_function_performance_counters;
			}
			return 0;
		}

		// Converts a given fixed/dynamic counter index to a pair of MSRs mapping the respective
		// configuration register and the counter register.
		// - Null MSRs indicate failure.
		// - Note: Fixed counter control MSR is a bitset.
		//
		inline static std::pair<uint32_t, uint32_t> resolve_dynamic( size_t index, bool alias = false )
		{
			// Validate the counter index.
			//
			if ( index >= aliasing_counter_limit )
				return { UINT32_MAX, UINT32_MAX };

			// Validate against the index limit of the current CPU.
			//
			if ( index >= get_dynamic_counter_count() )
				return { UINT32_MAX, UINT32_MAX };

			// Return the mapping MSRs.
			//
			if ( !alias && index < counter_limit )
			{
				uint32_t offset = counter_stride * index;
				return { config_base + offset, counter_base + offset };
			}
			else
			{
				uint32_t offset = aliasing_counter_stride * index;
				return { aliasing_config_base + offset, aliasing_counter_base + offset };
			}
		}
		inline static std::pair<uint32_t, uint32_t> resolve_fixed( size_t index )
		{
			// Validate the counter index.
			//
			if constexpr ( fixed_control == UINT32_MAX || fixed_counter_base == UINT32_MAX )
				return { UINT32_MAX, UINT32_MAX };
			if ( index >= fixed_counter_limit )
				return { UINT32_MAX, UINT32_MAX };

			// Validate against the index limit of the current CPU.
			//
			if ( index >= get_fixed_counter_count() )
				return { UINT32_MAX, UINT32_MAX };

			// Return the mapping MSRs.
			//
			uint32_t offset = fixed_counter_stride * index;
			return { fixed_control, fixed_counter_base + offset };
		}
	};
	using intel_traits = traits_of<true>;
	using amd_traits =   traits_of<false>;

	// Implement a simple visitor.
	//
	template<typename T>
	FORCE_INLINE inline decltype( auto ) visit_traits( T&& fn )
	{
		if ( is_intel() )
			return fn( intel_traits{} );
		else
			return fn( amd_traits{} );
	}

	// Counter flags.
	// - Maps to perfevtsel bits 1:1 for convenience.
	//
	enum counter_flags : uint32_t
	{
		ctr_enable =     IA32_PERFEVTSEL_EN_FLAG,
		ctr_user =       IA32_PERFEVTSEL_USR_FLAG,
		ctr_supervisor = IA32_PERFEVTSEL_OS_FLAG,
		ctr_any_thread = IA32_PERFEVTSEL_ANY_THREAD_FLAG,
		ctr_interrupt =  IA32_PERFEVTSEL_INTR_FLAG,
	};

	// Changes the state configuration given the dynamic/fixed counter.
	// - Returns false/npos on failure, else true/counter index.
	//
	FORCE_INLINE inline bool dynamic_set_state( size_t index, evtsel_t selector, uint32_t flags, bool update_global = false )
	{
		// Visit the platform.
		//
		bool success = false;
		visit_traits( [ & ] <typename traits> ( traits )
		{
			// Resolve the counter.
			//
			auto [cfg, cnt] = traits::resolve_dynamic( index );
			if ( cfg == UINT32_MAX )
				return;

			// Create the event selector register and write it.
			// - User-passed flags are bitwise compatible.
			//
			perfevtsel_register sel = { .flags = flags };
			sel.event_select = selector.event_select;
			sel.u_mask = selector.unit_mask;
			sel.edge = selector.edge;
			sel.inv = selector.invert;
			sel.cmask = selector.count_mask;
			write_msr( cfg, sel );

			// Update the global control register if requested.
			//
			if ( update_global && traits::global_control != UINT32_MAX )
			{
				auto global_ctrl = read_msr<perf_global_ctrl_register>( traits::global_control );
				if ( flags & ctr_enable )
					global_ctrl.en_pmcn |= 1ull << index;
				else
					global_ctrl.en_pmcn &= ~( 1ull << index );
				write_msr( traits::global_control, global_ctrl );
			}
			success = true;
		} );
		return success;
	}
	FORCE_INLINE inline bool dynamic_set_state( size_t index, event_id event, uint32_t flags, bool update_global = false )
	{
		// Visit the event identifier.
		//
		bool success = false;
		visit_traits( [ & ] <typename traits> ( traits )
		{
			xstd::visit_range<event_id::none, event_id::max>( event, [ & ] <event_id evt> ( xstd::const_tag<evt> )
			{
				// Map and validate the selector.
				//
				evtsel_t selector = traits::template dynamic_selector<evt>();
				if ( !selector && evt != event_id::none )
					return;
				success = dynamic_set_state( index, selector, flags, update_global );
			} );
		} );
		return success;
	}
	FORCE_INLINE inline size_t fixed_set_state( event_id event, uint32_t flags, bool update_global = false )
	{
		// Visit the platform.
		//
		size_t index = std::string::npos;
		visit_traits( [ & ] <typename traits> ( traits )
		{
			// Validate the fixed control register.
			//
			if ( traits::fixed_control == UINT32_MAX )
				return;

			// Visit the event identifier.
			//
			xstd::visit_range<event_id::none, event_id::max>( event, [ & ] <event_id evt> ( xstd::const_tag<evt> )
			{
				// Map and validate the selector.
				//
				size_t iindex = traits::template fixed_counter<evt>();
				if ( iindex == std::string::npos )
					return;

				// Update the fixed control register.
				//
				auto fixed_ctrl = read_msr( traits::fixed_control );
				fixed_ctrl &= ~xstd::fill_bits( 4, iindex * 4 );
				fixed_ctr_ctrl_register ctrl = { .flags = 0 };
				if ( flags & ctr_enable )
				{
					ctrl.en0_os = ( flags & ctr_supervisor ) != 0;
					ctrl.en0_usr = ( flags & ctr_user ) != 0;
					ctrl.en0_pmi = ( flags & ctr_interrupt ) != 0;
					ctrl.any_thread0 = ( flags & ctr_any_thread ) != 0;
					fixed_ctrl |= ctrl.flags << ( iindex * 4 );
				}
				write_msr( traits::fixed_control, fixed_ctrl );

				// Update the global control register if requested.
				//
				if ( update_global && traits::global_control != UINT32_MAX )
				{
					auto global_ctrl = read_msr<perf_global_ctrl_register>( traits::global_control );
					if ( flags & ctr_enable )
						global_ctrl.en_fixed_ctrn |= 1ull << iindex;
					else
						global_ctrl.en_fixed_ctrn &= ~( 1ull << iindex );
					write_msr( traits::global_control, global_ctrl );
				}

				// Indicate success by setting the index.
				//
				index = iindex;
			} );
		} );
		return index;
	}
	
	// Queries the state configuration given the dynamic/fixed counter.
	// - Returns zero on failure.
	//
	FORCE_INLINE inline counter_flags dynamic_query_state( size_t index, bool query_global = true )
	{
		// Visit the platform.
		//
		return visit_traits( [ & ] <typename traits> ( traits ) -> counter_flags
		{
			// Returns zero (disabled) if the dynamic counter index is invalid.
			//
			auto [cfg, cnt] = traits::resolve_dynamic( index );
			if ( cfg == UINT32_MAX )
				return counter_flags( 0 );

			// Query global state if requested and relevant.
			//
			if ( query_global && traits::global_control != UINT32_MAX )
			{
				// If performance counter is disabled at the global level, return zero as well.
				//
				auto global_ctrl = read_msr<perf_global_ctrl_register>( traits::global_control );
				if ( !( global_ctrl.en_pmcn & ( 1ull << index ) ) )
					return counter_flags( 0 );
			}

			// Finally read the MSR and return it as is.
			//
			return counter_flags( read_msr( cfg ) );
		} );
	}
	FORCE_INLINE inline counter_flags fixed_query_state( size_t index, bool query_global = true )
	{
		return visit_traits( [ & ] <typename traits> ( traits ) -> counter_flags
		{
			// Returns zero (disabled) if the fixed counter index is invalid.
			//
			auto [cfg, cnt] = traits::resolve_fixed( index );
			if ( cfg == UINT32_MAX )
				return counter_flags( 0 );

			// Query global state if requested and relevant.
			//
			if ( query_global && traits::global_control != UINT32_MAX )
			{
				// If performance counter is disabled at the global level, return zero as well.
				//
				auto global_ctrl = read_msr<perf_global_ctrl_register>( traits::global_control );
				if ( !( global_ctrl.en_fixed_ctrn & ( 1ull << index ) ) )
					return counter_flags( 0 );
			}

			// Read the fixed control register, return zero if disabled for all CPLs.
			//
			fixed_ctr_ctrl_register ctrl{ .flags = ( read_msr( cfg ) >> ( index * 4 ) ) };
			if ( !ctrl.en0_os && !ctrl.en0_usr )
				return counter_flags( 0 );

			// Form the counter flags and return it.
			//
			uint32_t result = ctr_enable;
			if ( ctrl.en0_os ) result |= ctr_supervisor;
			if ( ctrl.en0_usr ) result |= ctr_user;
			if ( ctrl.en0_pmi ) result |= ctr_interrupt;
			if ( ctrl.any_thread0 ) result |= ctr_any_thread;
			return counter_flags( result );
		} );
	}

	// Sets the counter value given the dynamic/fixed counter.
	// - Returns false on failure.
	//
	FORCE_INLINE inline bool dynamic_set_value( size_t index, uint64_t value )
	{
		// Visit the platform.
		//
		return visit_traits( [ & ] <typename traits> ( traits ) -> bool
		{
			// Return false if the dynamic counter index is invalid.
			//
			auto [cfg, cnt] = traits::resolve_dynamic( index );
			if ( cnt == UINT32_MAX )
				return false;

			// Write the value and indicate success.
			//
			write_msr( cnt, value );
			return true;
		} );
	}
	FORCE_INLINE inline bool fixed_set_value( size_t index, uint64_t value )
	{
		// Visit the platform.
		//
		return visit_traits( [ & ] <typename traits> ( traits ) -> bool
		{
			// Return false if the fixed counter index is invalid.
			//
			auto [cfg, cnt] = traits::resolve_fixed( index );
			if ( cnt == UINT32_MAX )
				return false;

			// Write the value and indicate success.
			//
			write_msr( cnt, value );
			return true;
		} );
	}

	// Queries the counter value given the dynamic/fixed counter.
	// - This funtion (as opposed to directly calling read_pmc) will do validation and return 0 on an illegal call,
	//   so it is slower than the alternative. It will also read using the MSR.
	//
	FORCE_INLINE inline uint64_t dynamic_query_value( size_t index )
	{
		return visit_traits( [ & ] <typename traits> ( traits ) -> uint64_t
		{
			auto [cfg, cnt] = traits::resolve_dynamic( index );
			return ( cnt && cfg ) ? read_msr( cnt ) : 0;
		} );
	}
	FORCE_INLINE inline uint64_t fixed_query_value( size_t index )
	{
		return visit_traits( [ & ] <typename traits> ( traits ) -> uint64_t
		{
			auto [cfg, cnt] = traits::resolve_fixed( index );
			return ( cnt && cfg ) ? read_msr( cnt ) : 0;
		} );
	}

	// Quick disable shortcuts.
	// - Returns false/npos on failure, else true/counter index.
	//
	FORCE_INLINE inline bool dynamic_disable( size_t index, bool update_global = false ) { return dynamic_set_state( index, event_id::none, 0, update_global ); }
	FORCE_INLINE inline size_t fixed_disable( event_id event, bool update_global = false ) { return fixed_set_state( event, 0, update_global ); }
};