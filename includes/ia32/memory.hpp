#pragma once
#include "../ia32.hpp"
#include <xstd/type_helpers.hpp>

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
	using phys_ptr = std::unique_ptr<T, phys_memory_deleter>;
	
	template<typename T = uint8_t>
	FORCE_INLINE inline phys_ptr<T> map_physical( uint64_t physical_address, size_t length = sizeof( T ), bool cached = false )
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

	//
	// --- OS specific details that are left to be externally initialized.
	//

	// Index of the self referencing page table entry and the bases.
	// - Grouping them together in an array lets us ensure that the linker
	//   puts this data in a single cache line to lower the subsequent read costs.
	//
	inline uint16_t self_ref_index = 0;
	alignas( 64 ) inline std::array<pt_entry_64*, page_table_depth> pt_bases = {};
	static constexpr pt_entry_64*& pte_base =   pt_bases[ 0 ];
	static constexpr pt_entry_64*& pde_base =   pt_bases[ 1 ];
	static constexpr pt_entry_64*& pdpte_base = pt_bases[ 2 ];
	static constexpr pt_entry_64*& pml4e_base = pt_bases[ 3 ];
#if XSTD_IA32_LA57
	static constexpr pt_entry_64*& pml5e_base = pt_bases[ 4 ];
#endif
	static constexpr pt_entry_64*& pxe_base = pt_bases[ pt_bases.size() - 1 ];

#if __clang__
	// Clang splits this array.....
	namespace impl
	{
		[[gnu::used]] inline pt_entry_64** volatile __arr = &pt_bases[ 0 ];
	};
#endif

	//
	// --- Architectural details.
	//

	// Virtual address details.
	//
	FORCE_INLINE inline constexpr bool is_cannonical( xstd::any_ptr ptr ) { return ( ptr >> ( va_bits - 1 ) ) == 0 || ( int64_t( ptr ) >> ( va_bits - 1 ) ) == -1; }
	FORCE_INLINE inline constexpr uint64_t page_size( int8_t depth ) { return 1ull << ( 12 + ( 9 * depth ) ); }
	FORCE_INLINE inline constexpr uint64_t page_offset( xstd::any_ptr ptr ) { return ptr & 0xFFF; }
	FORCE_INLINE inline constexpr uint64_t pt_index( xstd::any_ptr ptr, size_t depth ) { return ( ptr >> ( 12 + 9 * depth ) ) % 512; }
	FORCE_INLINE inline constexpr uint64_t pt_index( xstd::any_ptr ptr ) { return pt_index( ptr, 0 ); }
	FORCE_INLINE inline constexpr uint64_t pd_index( xstd::any_ptr ptr ) { return pt_index( ptr, 1 ); }
	FORCE_INLINE inline constexpr uint64_t pdpt_index( xstd::any_ptr ptr ) { return pt_index( ptr, 2 ); }
	FORCE_INLINE inline constexpr uint64_t pml4_index( xstd::any_ptr ptr ) { return pt_index( ptr, 3 ); }
#if XSTD_IA32_LA57
	FORCE_INLINE inline constexpr uint64_t pml5_index( xstd::any_ptr ptr ) { return pt_index( ptr, 4 ); }
#endif
	FORCE_INLINE inline constexpr uint64_t px_index( xstd::any_ptr ptr ) { return pt_index( ptr, page_table_depth - 1 ); }

	// Unpack / pack helpers.
	//
	FORCE_INLINE inline std::array<uint16_t, page_table_depth + 1> unpack( xstd::any_ptr ptr )
	{
		return xstd::make_constant_series<page_table_depth + 1>( [ & ] <size_t N> ( xstd::const_tag<N> c ) -> uint16_t
		{ 
			if constexpr ( c == page_table_depth )
				return page_offset( ptr );
			else
				return pt_index( ptr, page_table_depth - c ); 
		} );
	}
	FORCE_INLINE inline xstd::any_ptr pack( const std::array<uint16_t, page_table_depth + 1>& array )
	{
		int64_t result = 0;
		for ( size_t n = 0; n != page_table_depth; n++ )
			result = ( result << 9 ) | array[ n ];
		result = ( result << 12 ) | array.back();
		return uint64_t( ( result << sx_bits ) >> sx_bits );
	}
	
	// Recursive page table indexing.
	//
	FORCE_INLINE inline pt_entry_64* get_pte_base( uint16_t self_ref_idx, size_t depth )
	{
		int64_t result = 0;
		for ( size_t j = 0; j <= depth; j++ )
			result = ( result << 9 ) | self_ref_idx;
		result <<= 12 + ( page_table_depth - depth - 1 ) * 9;
		return ( pt_entry_64* ) ( ( result << sx_bits ) >> sx_bits );
	}

	// Recursive page table lookup.
	//
	FORCE_INLINE inline pt_entry_64* get_pte( xstd::any_ptr ptr, size_t depth ) { return &pt_bases[ depth ][ ( ptr << sx_bits ) >> ( sx_bits + 12 + 9 * depth ) ]; }
	FORCE_INLINE inline pt_entry_64* get_pte( xstd::any_ptr ptr ) { return get_pte( ptr, 0 ); }
	FORCE_INLINE inline pt_entry_64* get_pde( xstd::any_ptr ptr ) { return get_pte( ptr, 1 ); }
	FORCE_INLINE inline pt_entry_64* get_pdpte( xstd::any_ptr ptr ) { return get_pte( ptr, 2 ); }
	FORCE_INLINE inline pt_entry_64* get_pml4e( xstd::any_ptr ptr ) { return get_pte( ptr, 3 ); }
#if XSTD_IA32_LA57
	FORCE_INLINE inline pt_entry_64* get_pml5e( xstd::any_ptr ptr ) { return get_pte( ptr, 4 ); }
#endif
	FORCE_INLINE inline pt_entry_64* get_pxe( xstd::any_ptr ptr ) { return get_pte( ptr, page_table_depth - 1 ); }
	FORCE_INLINE inline std::array<pt_entry_64*, page_table_depth> get_pte_hierarchy( xstd::any_ptr ptr )
	{
		return xstd::make_constant_series<page_table_depth>( [ & ] ( auto c ) { return get_pte( ptr, page_table_depth - c - 1 ); } );
	}
	FORCE_INLINE inline std::pair<pt_entry_64*, int8_t> lookup_pte( xstd::any_ptr ptr )
	{
		// Iterate the page tables until the PTE.
		//
		for ( size_t n = page_table_depth - 1; n != 0; n-- )
		{
			// If we reached an entry representing data pages or if the
			// entry is not present, return.
			//
			auto* entry = get_pte( ptr, n );
			if ( !entry->present || entry->large_page )
				return { entry, n };
		}

		// Return the PTE.
		//
		return { get_pte( ptr ), 0 };
	}

	// Reverse recursive page table lookup.
	//
	FORCE_INLINE inline xstd::any_ptr pte_to_va( const void* pte, size_t depth ) { return ( ( int64_t( pte ) << ( sx_bits + 12 + ( 9 * depth ) - 3 ) ) >> sx_bits ); }
	FORCE_INLINE inline xstd::any_ptr pte_to_va( const void* pte ) { return pte_to_va( pte, 0 ); }
	FORCE_INLINE inline xstd::any_ptr pde_to_va( const void* pte ) { return pte_to_va( pte, 1 ); }
	FORCE_INLINE inline xstd::any_ptr pdpte_to_va( const void* pte ) { return pte_to_va( pte, 2 ); }
	FORCE_INLINE inline xstd::any_ptr pml4e_to_va( const void* pte ) { return pte_to_va( pte, 3 ); }
#if XSTD_IA32_LA57
	FORCE_INLINE inline xstd::any_ptr pml5e_to_va( const void* pte ) { return pte_to_va( pte, 4 ); }
#endif
	FORCE_INLINE inline xstd::any_ptr pxe_to_va( const void* pte ) { return pte_to_va( pte, page_table_depth - 1 ); }
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
		auto [pte, _] = lookup_pte( ptr );
		return pte->present;
	}

	// Gets the physical address associated with the virtual address.
	//
	FORCE_INLINE inline uint64_t get_physical_address( xstd::any_ptr ptr )
	{
		auto [pte, depth] = lookup_pte( ptr );
		if ( !pte->present ) return 0;
		return ( pte->page_frame_number << 12 ) | ( ptr & ( mem::page_size( depth ) - 1 ) );
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
				auto [pte, depth] = lookup_pte( it );
				dassert( pte->present );
				pte->flags &= ~all_flags;
				pte->flags |= mask;

				if constexpr ( !IpiFlush )
					invlpg( it );
				
				it += mem::page_size( depth );
			}
			
			if constexpr( IpiFlush )
				ipi_flush_tlb( ptr, length );
		}
	};
	FORCE_INLINE inline void change_protection( xstd::any_ptr ptr, size_t length, protection_mask mask ) { impl::change_protection<true>( ptr, length, mask ); }
	FORCE_INLINE inline void change_protection_no_ipi( xstd::any_ptr ptr, size_t length, protection_mask mask ) { impl::change_protection<false>( ptr, length, mask ); }

	// Initialization helper.
	//
	inline void init( uint16_t idx )
	{
		self_ref_index = idx;
		for ( size_t n = 0; n != pt_bases.size(); n++ )
			pt_bases[ n ] = get_pte_base( self_ref_index, n );
	}
};