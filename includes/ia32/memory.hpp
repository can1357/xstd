#pragma once
#include "../ia32.hpp"
#include <xstd/type_helpers.hpp>
#include <xstd/fetch_once.hpp>

// [[Configuration]]
// XSTD_IA32_LA57: Enables or disables LA57.
//
#ifndef XSTD_IA32_LA57
	#define XSTD_IA32_LA57 0
#endif

//
// --- OS specific details that are left to be externally initialized.
//
namespace ia32::mem
{
	// Flushes the TLB given a range for all processors.
	// -- If no arguments given, will flush the whole TLB, else a single range.
	//
	extern void ipi_flush_tlb();
	extern void ipi_flush_tlb( xstd::any_ptr ptr, size_t length = 0x1000 );

	// Mapping of physical memory.
	//
	extern xstd::any_ptr map_physical_memory_range( uint64_t address, size_t length, bool cached = false );
	extern void unmap_physical_memory_range( xstd::any_ptr pointer, size_t length );

	//
	// --- Wrappers around the OS interface.
	//

	// Maps the requested physical memory range and returns a smart pointer.
	//
	struct phys_memory_deleter
	{
		size_t length;
		template<typename T> void operator()( T* ptr ) const { unmap_physical_memory_range( ( void* ) ptr, length ); }
	};
	template<typename T = uint8_t>
	using unique_phys_ptr = std::unique_ptr<T, phys_memory_deleter>;
	
	template<typename T = uint8_t>
	FORCE_INLINE inline unique_phys_ptr<T> map_physical( uint64_t physical_address, size_t length = sizeof( T ), bool cached = false )
	{
		void* ptr = map_physical_memory_range( physical_address, length, cached );
		if ( !ptr )
			return {};
		return { ( std::add_pointer_t<std::remove_extent_t<T>> ) ptr, phys_memory_deleter{ length } };
	}
};

// Implement the virtual memory helpers.
//
namespace ia32::mem
{
	static constexpr size_t page_table_depth = ( XSTD_IA32_LA57 ? 5 : 4 );
	static constexpr size_t va_bits = page_table_depth * 9 + 12;
	static constexpr size_t sx_bits = 64 - va_bits;

	// Paging level enumerator.
	//
	enum pt_level : int8_t
	{
		pte_level =   0,
		pde_level =   1,
		pdpte_level = 2,
		pml4e_level = 3,
#if XSTD_IA32_LA57
		pml5e_level = 4,
#endif
		pxe_level =   page_table_depth - 1,
	};

	// Maximum large-page level.
	//
	static constexpr pt_level max_large_page_level = pdpte_level;

	//
	// --- OS specific details that are left to be externally initialized.
	//

	// Index of the self referencing page table entry and the bases.
	//
	inline uint32_t self_ref_index = 0;
	inline uint64_t pxe_base_div_8 = 0;

	// Virtual address details.
	//
	FORCE_INLINE CONST_FN inline constexpr bool is_cannonical( xstd::any_ptr ptr ) { return ( ptr >> ( va_bits - 1 ) ) == 0 || ( int64_t( ptr ) >> ( va_bits - 1 ) ) == -1; }
	FORCE_INLINE CONST_FN inline constexpr xstd::any_ptr make_cannonical( xstd::any_ptr ptr ) { return xstd::any_ptr( int64_t( ptr << sx_bits ) >> sx_bits ); }
	FORCE_INLINE CONST_FN inline constexpr uint64_t page_size( int8_t depth ) { return 1ull << ( 12 + ( 9 * depth ) ); }
	FORCE_INLINE CONST_FN inline constexpr uint64_t page_offset( xstd::any_ptr ptr ) { return ptr & 0xFFF; }
	FORCE_INLINE CONST_FN inline constexpr uint64_t pt_index( xstd::any_ptr ptr, int8_t level ) { return ( ptr >> ( 12 + 9 * level ) ) % 512; }
	FORCE_INLINE CONST_FN inline constexpr uint64_t pt_index( xstd::any_ptr ptr ) { return pt_index( ptr, pte_level ); }
	FORCE_INLINE CONST_FN inline constexpr uint64_t pd_index( xstd::any_ptr ptr ) { return pt_index( ptr, pde_level ); }
	FORCE_INLINE CONST_FN inline constexpr uint64_t pdpt_index( xstd::any_ptr ptr ) { return pt_index( ptr, pdpte_level ); }
	FORCE_INLINE CONST_FN inline constexpr uint64_t pml4_index( xstd::any_ptr ptr ) { return pt_index( ptr, pml4e_level ); }
#if XSTD_IA32_LA57
	FORCE_INLINE CONST_FN inline constexpr uint64_t pml5_index( xstd::any_ptr ptr ) { return pt_index( ptr, pml5e_level ); }
#endif
	FORCE_INLINE CONST_FN inline constexpr uint64_t px_index( xstd::any_ptr ptr ) { return pt_index( ptr, pxe_level ); }

	// Unpack / pack helpers.
	//
	FORCE_INLINE inline std::array<uint16_t, page_table_depth + 1> unpack( xstd::any_ptr ptr )
	{
		return xstd::make_constant_series<page_table_depth + 1>( [ & ] <size_t N> ( xstd::const_tag<N> c ) -> uint16_t
		{ 
			if constexpr ( c == page_table_depth )
				return page_offset( ptr );
			else
				return pt_index( ptr, page_table_depth - ( c + 1 ) ); 
		} );
	}
	FORCE_INLINE inline xstd::any_ptr pack( const std::array<uint16_t, page_table_depth + 1>& array )
	{
		uint64_t result = 0;
		for ( size_t n = 0; n != page_table_depth; n++ )
			result = ( result << 9 ) | array[ n ];
		result = ( result << 12 ) | array.back();
		return make_cannonical( result );
	}
	
	// Recursive page table indexing.
	//
	FORCE_INLINE CONST_FN inline pt_entry_64* locate_page_table( int8_t depth, uint32_t self_ref_idx )
	{
		uint64_t result = 0;
		for ( int8_t j = 0; j <= depth; j++ )
			result = ( result << 9 ) | self_ref_idx;
		result <<= sx_bits + 12 + ( pxe_level - depth ) * 9;
		return ( pt_entry_64* ) ( int64_t( result ) >> sx_bits );
	}
	FORCE_INLINE CONST_FN inline pt_entry_64* locate_page_table( int8_t depth )
	{
		xstd::any_ptr ptr = xstd::fetch_once<&pxe_base_div_8>();
		if ( xstd::const_condition( depth == pxe_level ) )
			return ( pt_entry_64* ) ( ptr << 3 );

		auto shift = 12 + ( pxe_level - depth ) * 9;
		return ( pt_entry_64* ) ( ( ptr >> ( shift - 3 ) ) << shift );
	}

	// Recursive page table lookup.
	//
	FORCE_INLINE CONST_FN inline pt_entry_64* get_pte( xstd::any_ptr ptr, int8_t depth )
	{
		uint64_t base = xstd::fetch_once<&pxe_base_div_8>();
		auto important_bits = ( page_table_depth - depth ) * 9;

		uint64_t tmp = shrd( base, ptr >> ( 12 + 9 * depth ), important_bits );
		return ( pt_entry_64* ) rotlq( tmp, important_bits + 3 );
	}
	FORCE_INLINE CONST_FN inline pt_entry_64* get_pte( xstd::any_ptr ptr ) { return get_pte( ptr, pte_level ); }
	FORCE_INLINE CONST_FN inline pt_entry_64* get_pde( xstd::any_ptr ptr ) { return get_pte( ptr, pde_level ); }
	FORCE_INLINE CONST_FN inline pt_entry_64* get_pdpte( xstd::any_ptr ptr ) { return get_pte( ptr, pdpte_level ); }
	FORCE_INLINE CONST_FN inline pt_entry_64* get_pml4e( xstd::any_ptr ptr ) { return get_pte( ptr, pml4e_level ); }
#if XSTD_IA32_LA57
	FORCE_INLINE CONST_FN inline pt_entry_64* get_pml5e( xstd::any_ptr ptr ) { return get_pte( ptr, pml5e_level ); }
#endif
	FORCE_INLINE CONST_FN inline pt_entry_64* get_pxe( xstd::any_ptr ptr ) { return get_pte( ptr, pxe_level ); }
	FORCE_INLINE CONST_FN inline pt_entry_64* get_pxe_by_index( uint32_t index ) 
	{
		uint64_t base = xstd::fetch_once<&pxe_base_div_8>();
		return ( pt_entry_64* ) rotlq( shrd( base, index, 9 ), 12 );
	}

	FORCE_INLINE CONST_FN inline std::array<pt_entry_64*, page_table_depth> get_pte_hierarchy( xstd::any_ptr ptr )
	{
		return xstd::make_constant_series<page_table_depth>( [ & ] ( auto c ) { return get_pte( ptr, pxe_level - c ); } );
	}
	FORCE_INLINE PURE_FN inline std::pair<pt_entry_64*, int8_t> lookup_pte( xstd::any_ptr ptr )
	{
		// Iterate the page tables until the PTE.
		//
		for ( int8_t n = pxe_level; n != pte_level; n-- )
		{
			// If we reached an entry representing data pages or if the
			// entry is not present, return.
			//
			auto* entry = get_pte( ptr, n );
			if ( !entry->present || ( n <= max_large_page_level && entry->large_page ) )
				return { entry, n };
		}

		// Return the PTE.
		//
		return { get_pte( ptr ), pte_level };
	}

	// Looks up a PTE with specific access rights, on failure returns the lookup result
	// and an additional "fake" page fault exception with the bits set as the following:
	//
	//  - .present:          Page was NOT present.  // <--- Additional emphasis, since it behaves differently compared to a real #PF.
	//  - .write:            Page was not writable.
	//  - .execute:          Page was not executable.
	//  - .user_mode_access: Supervisor page accessed by user mode or VICE VERSA. // ^
	//
	FORCE_INLINE inline std::tuple<pt_entry_64*, int8_t, page_fault_exception> lookup_pte_as( xstd::any_ptr ptr, bool user, bool write, bool execute, bool smap = false, bool smep = false )
	{
		// Iterate the page tables until the PTE.
		//
		bool puser = true;
		for ( int8_t n = pxe_level; n >= pte_level; n-- )
		{
			auto* entry = get_pte( ptr, n );

			// Fail if not present / violates user-bit.
			//
			if ( !entry->present ) [[unlikely]]
				return { entry, n, { .present = true } };
			if ( user && !entry->user ) [[unlikely]]
				return { entry, n, { .user_mode_access = true } };
			puser &= entry->user;

			// If we've reached the final data page, validate access.
			//
			if ( n == 0 || entry->large_page )
			{
				if ( puser && !user && ( execute ? smep : smap ) ) [[unlikely]]
					return { entry, n, { .user_mode_access = true } };
				else if ( write && !entry->write ) [[unlikely]]
					return { entry, n, { .write = true } };
				else if ( execute && entry->execute_disable ) [[unlikely]]
					return { entry, n, { .execute = true } };
				else
					return { entry, n, { .flags = 0 } };
			}
		}
		unreachable();
	}

	// Reverse recursive page table lookup.
	//
	FORCE_INLINE CONST_FN inline xstd::any_ptr pte_to_va( const void* pte, int8_t level ) { return ( ( int64_t( pte ) << ( sx_bits + 12 + ( 9 * level ) - 3 ) ) >> sx_bits ); }
	FORCE_INLINE CONST_FN inline xstd::any_ptr pte_to_va( const void* pte ) { return pte_to_va( pte, pte_level ); }
	FORCE_INLINE CONST_FN inline xstd::any_ptr pde_to_va( const void* pte ) { return pte_to_va( pte, pde_level ); }
	FORCE_INLINE CONST_FN inline xstd::any_ptr pdpte_to_va( const void* pte ) { return pte_to_va( pte, pdpte_level ); }
	FORCE_INLINE CONST_FN inline xstd::any_ptr pml4e_to_va( const void* pte ) { return pte_to_va( pte, pml4e_level ); }
#if XSTD_IA32_LA57
	FORCE_INLINE CONST_FN inline xstd::any_ptr pml5e_to_va( const void* pte ) { return pte_to_va( pte, pml5e_level ); }
#endif
	FORCE_INLINE CONST_FN inline xstd::any_ptr pxe_to_va( const void* pte ) { return pte_to_va( pte, pxe_level ); }
	FORCE_INLINE inline std::pair<xstd::any_ptr, int8_t> rlookup_pte( const pt_entry_64* pte )
	{
		std::array hierarchy = unpack( pte );
		if ( hierarchy[ 0 ] != self_ref_index )
			return { nullptr, -1 };

		size_t n = 1;
		while ( n != page_table_depth && hierarchy[ n ] == self_ref_index )
			n++;
		return { pte_to_va( pte, n - 1 ), n - 1 };
	}

	// Virtual address validation and translation.
	//
	FORCE_INLINE inline bool is_address_valid( xstd::any_ptr ptr )
	{
		if ( !is_cannonical( ptr ) ) [[unlikely]]
			return false;
		auto [pte, _] = lookup_pte( ptr );
		return pte->present;
	}

	// Gets the physical address associated with the virtual address.
	//
	FORCE_INLINE inline uint64_t get_physical_address( xstd::any_ptr ptr )
	{
		auto [pte, depth] = lookup_pte( ptr );
		if ( !pte->present ) return 0;
		return ( pte->page_frame_number << 12 ) | ( ptr & ( page_size( depth ) - 1 ) );
	}
	FORCE_INLINE inline uint64_t get_pfn( xstd::any_ptr ptr )
	{
		return get_physical_address( ptr ) >> 12;
	}

	// Changes the protection of the given range.
	//
	enum protection_mask : uint64_t
	{
		prot_write =      PT_ENTRY_64_WRITE_FLAG,
		prot_read =       PT_ENTRY_64_PRESENT_FLAG,
		prot_no_execute = PT_ENTRY_64_EXECUTE_DISABLE_FLAG,
		prot_execute = 0,

		prot_rwx =  prot_read | prot_write | prot_execute,
		prot_rw =   prot_read | prot_write | prot_no_execute,
		prot_rx =   prot_read | prot_execute,
		prot_ro =   prot_read,
		prot_none = 0,
	};
	namespace impl
	{
		template<bool IpiFlush>
		FORCE_INLINE inline void change_protection( xstd::any_ptr ptr, size_t length, protection_mask mask )
		{
			static constexpr uint64_t all_flags = prot_write | prot_read | prot_no_execute;

			// Iterate page boundaries:
			//
			for ( auto it = ptr; it < ( ptr + length ); )
			{
				auto [pte, depth] = lookup_pte( it );
				dassert( pte->present );

				auto& atomic = xstd::make_atomic( pte->flags );
				atomic.fetch_or( mask );
				atomic.fetch_and( mask | ~all_flags );

				if constexpr ( !IpiFlush )
					invlpg( it );
				
				it += page_size( depth );
			}
			
			if constexpr( IpiFlush )
				ipi_flush_tlb( ptr, length );
		}
	};
	FORCE_INLINE inline void change_protection( xstd::any_ptr ptr, size_t length, protection_mask mask ) { impl::change_protection<true>( ptr, length, mask ); }
	FORCE_INLINE inline void change_protection_no_ipi( xstd::any_ptr ptr, size_t length, protection_mask mask ) { impl::change_protection<false>( ptr, length, mask ); }

	// Initialization helper.
	//
	inline void init( uint32_t idx )
	{
		self_ref_index = idx;
		pxe_base_div_8 = uint64_t( locate_page_table( pxe_level, idx ) ) >> 3;
	}
};