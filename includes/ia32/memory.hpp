#pragma once
#include "../ia32.hpp"
#include <xstd/type_helpers.hpp>

// Implements virtual memory helpers.
//
namespace ia32::mem
{
	//
	// --- OS specific details that are left to be externally initialized.
	//

	// Index of the self referencing page table entry and the bases.
	// - Grouping them together in an array lets us ensure that the linker
	//   puts this data in a single cache line to lower the subsequent read costs.
	//
	inline uint16_t self_ref_index = 0;
	alignas( 64 ) inline std::array<pt_entry_64*, 4> pt_bases = {};
	static constexpr pt_entry_64*& pte_base =   pt_bases[ 3 ];
	static constexpr pt_entry_64*& pde_base =   pt_bases[ 2 ];
	static constexpr pt_entry_64*& pdpte_base = pt_bases[ 1 ];
	static constexpr pt_entry_64*& pml4e_base = pt_bases[ 0 ];

#if __clang__
	// Clang splits this array.....
	[[gnu::used]] inline pt_entry_64** volatile __arr = &pt_bases[ 0 ];
#endif

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
	using phys_ptr = std::unique_ptr<T, phys_memory_deleter>;
	
	template<typename T = uint8_t>
	FORCE_INLINE inline phys_ptr<T> map_physical( uint64_t physical_address, size_t length = sizeof( T ), bool cached = false )
	{
		void* ptr = map_physical_memory_range( physical_address, length, cached );
		if ( !ptr )
			return {};
		return { ( std::add_pointer_t<std::remove_extent_t<T>> ) ptr, phys_memory_deleter{ length } };
	}

	//
	// --- Architectural details.
	//

	// Virtual address helpers.
	//
	union virtual_address_t
	{
		xstd::any_ptr pointer;
		struct
		{
			uint64_t page_offset : 12;
			uint64_t pte         : 9;
			uint64_t pde         : 9;
			uint64_t pdpte       : 9;
			uint64_t pml4e       : 9;
			uint64_t padding     : 16;
		};
	};
	FORCE_INLINE inline constexpr uint64_t page_offset( xstd::any_ptr ptr ) { return ptr & 0xFFF; }
	FORCE_INLINE inline constexpr uint64_t pt_index( xstd::any_ptr ptr ) { return ( ptr >> ( 12 + 9 * 0 ) ) % 512; }
	FORCE_INLINE inline constexpr uint64_t pd_index( xstd::any_ptr ptr ) { return ( ptr >> ( 12 + 9 * 1 ) ) % 512; }
	FORCE_INLINE inline constexpr uint64_t pdpt_index( xstd::any_ptr ptr ) { return ( ptr >> ( 12 + 9 * 2 ) ) % 512; }
	FORCE_INLINE inline constexpr uint64_t pml4_index( xstd::any_ptr ptr ) { return ( ptr >> ( 12 + 9 * 3 ) ) % 512; }
	FORCE_INLINE inline constexpr bool is_cannonical( xstd::any_ptr ptr ) { return ( ptr >> 47 ) == 0 || ( int64_t( ptr ) >> 47 ) == -1; }
	FORCE_INLINE inline constexpr auto unpack( xstd::any_ptr ptr )
	{
		return std::make_tuple( pml4_index( ptr ), pdpt_index( ptr ), pd_index( ptr ), pt_index( ptr ), page_offset( ptr ) );
	}
	FORCE_INLINE inline constexpr xstd::any_ptr pack( uint64_t pml4i, uint64_t pdpti, uint64_t pdi, uint64_t pti, uint64_t page_offset = 0 )
	{
		int64_t result = pml4i;
		result = ( result << 9 ) | pdpti;
		result = ( result << 9 ) | pdi;
		result = ( result << 9 ) | pti;
		result = ( result << 12 ) | page_offset;
		return ( result << 16 ) >> 16;
	}

	// Page table lookup.
	//
	FORCE_INLINE inline pt_entry_64* get_pte_base( uint16_t self_ref_idx ) { return pack( self_ref_idx, 0, 0, 0 ); }
	FORCE_INLINE inline pt_entry_64* get_pde_base( uint16_t self_ref_idx ) { return pack( self_ref_idx, self_ref_idx, 0, 0 ); }
	FORCE_INLINE inline pt_entry_64* get_pdpte_base( uint16_t self_ref_idx ) { return pack( self_ref_idx, self_ref_idx, self_ref_idx, 0 ); }
	FORCE_INLINE inline pt_entry_64* get_pml4e_base( uint16_t self_ref_idx ) { return pack( self_ref_idx, self_ref_idx, self_ref_idx, self_ref_idx ); }
	FORCE_INLINE inline pt_entry_64* get_pte( xstd::any_ptr ptr ) { return &pte_base[ ( ptr << 16 ) >> ( 16 + 12 + 9 * 0 ) ]; }
	FORCE_INLINE inline pt_entry_64* get_pde( xstd::any_ptr ptr ) { return &pde_base[ ( ptr << 16 ) >> ( 16 + 12 + 9 * 1 ) ]; }
	FORCE_INLINE inline pt_entry_64* get_pdpte( xstd::any_ptr ptr ) { return &pdpte_base[ ( ptr << 16 ) >> ( 16 + 12 + 9 * 2 ) ]; }
	FORCE_INLINE inline pt_entry_64* get_pml4e( xstd::any_ptr ptr ) { return &pml4e_base[ ( ptr << 16 ) >> ( 16 + 12 + 9 * 3 ) ]; }
	FORCE_INLINE inline xstd::any_ptr pte_to_va( const void* pte ) { return ( ( int64_t( pte ) << ( 16 + 12 + ( 9 * 0 ) - 3 ) ) >> 16 ); }
	FORCE_INLINE inline xstd::any_ptr pde_to_va( const void* pde ) { return ( ( int64_t( pde ) << ( 16 + 12 + ( 9 * 1 ) - 3 ) ) >> 16 ); }
	FORCE_INLINE inline xstd::any_ptr pdpte_to_va( const void* pdpte ) { return ( ( int64_t( pdpte ) << ( 16 + 12 + ( 9 * 2 ) - 3 ) ) >> 16 ); }
	FORCE_INLINE inline xstd::any_ptr pml4e_to_va( const void* pml4e ) { return ( ( int64_t( pml4e ) << ( 16 + 12 + ( 9 * 3 ) - 3 ) ) >> 16 ); }
	FORCE_INLINE inline std::array<pt_entry_64*, 4> get_pte_hierarchy( xstd::any_ptr ptr ) { return { get_pml4e( ptr ), get_pdpte( ptr ), get_pde( ptr ), get_pte( ptr ) }; }

	// Gets the bottom level PTE for the virtual address and the page size.
	//
	FORCE_INLINE inline std::pair<pt_entry_64*, size_t> get_bottom_pte( xstd::any_ptr ptr )
	{
		std::array hierarchy = get_pte_hierarchy( ptr );
		size_t size = 1ull << ( 12 + 9 * 3 );
		for ( size_t n = 0;; n++, size >>= 9 )
		{
			if ( &hierarchy[ n ] == &hierarchy.back() ||  // Reached PTE level.
				 !hierarchy[ n ]->present ||              // Page is not present.
				 hierarchy[ n ]->large_page )             // Large page.
				return { hierarchy[ n ], size };
		}
	}

	// Virtual address validation and translation.
	//
	FORCE_INLINE inline bool is_address_valid( xstd::any_ptr ptr )
	{
		auto [pte, _] = get_bottom_pte( ptr );
		return pte->present;
	}

	// Gets the physical address associated with the virtual address.
	//
	FORCE_INLINE inline uint64_t get_physical_address( xstd::any_ptr ptr )
	{
		auto [pte, page_size] = get_bottom_pte( ptr );
		if ( !pte->present ) return 0;
		return ( pte->page_frame_number << 12 ) | ( ptr & ( page_size - 1 ) );
	}

	// Changes the protection of the given range.
	//
	enum protection_mask : uint64_t
	{
		prot_write =      PT_ENTRY_64_WRITE_FLAG,
		prot_read =       PT_ENTRY_64_PRESENT_FLAG,
		prot_no_execute = PT_ENTRY_64_EXECUTE_DISABLE_FLAG,
		prot_execute = 0,

		prot_rwx = prot_read | prot_write | prot_execute,
		prot_rw =  prot_read | prot_write | prot_no_execute,
		prot_rx =  prot_read | prot_execute,
		prot_ro =  prot_read,
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
				// Get the PTE, if not present, touch and retry.
				//
				auto [pte, sz] = get_bottom_pte( it );
				dassert( pte->present );
				pte->flags &= ~all_flags;
				pte->flags |= mask;

				if constexpr ( !IpiFlush )
					invlpg( it );
				
				it += sz;
			}
			
			if constexpr( IpiFlush )
				ipi_flush_tlb( ptr, length );
		}
	};
	FORCE_INLINE inline void change_protection( xstd::any_ptr ptr, size_t length, protection_mask mask ) { impl::change_protection<true>( ptr, length, mask ); }
	FORCE_INLINE inline void change_protection_no_ipi( xstd::any_ptr ptr, size_t length, protection_mask mask ) { impl::change_protection<false>( ptr, length, mask ); }
};