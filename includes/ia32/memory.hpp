#pragma once
#include <ia32.hpp>
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

	// Identity mapping of physical memory, optional.
	//
#if __has_xcxx_builtin(__builtin_fetch_dynamic)
	FORCE_INLINE inline void set_phys_base( const void* value ) { __builtin_store_dynamic( "@.physidmap", value ); }
	FORCE_INLINE CONST_FN inline xstd::any_ptr get_phys_base() { return __builtin_fetch_dynamic( "@.physidmap" ); }
#else
	inline const void* __phys_id_map = nullptr;
	FORCE_INLINE inline void set_phys_base( const void* value ) { __phys_id_map = value; }
	FORCE_INLINE CONST_FN inline xstd::any_ptr get_phys_base() { return __phys_id_map; }
#endif

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
	FORCE_INLINE inline unique_phys_ptr<T> map_physical( uint64_t physical_address, size_t length = sizeof( T ), bool cached = false ) {
		void* ptr = map_physical_memory_range( physical_address, length, cached );
		if ( !ptr )
			return {};
		return { ( std::add_pointer_t<std::remove_extent_t<T>> ) ptr, phys_memory_deleter{ length } };
	}

	// phys_ptr that indexes the identity mapping base address.
	//
	template<typename T = void>
	struct TRIVIAL_ABI phys_ptr {
		uint64_t address;

		// Constructed from a physical address, default copy move.
		//
		constexpr phys_ptr( std::nullptr_t = {} ) : address( ( uint64_t ) INT64_MIN ) {}
		constexpr phys_ptr( uint64_t address ) : address( address ) {}
		constexpr phys_ptr( phys_ptr&& ) noexcept = default;
		constexpr phys_ptr( const phys_ptr& ) = default;
		constexpr phys_ptr& operator=( phys_ptr&& ) noexcept = default;
		constexpr phys_ptr& operator=( const phys_ptr& ) = default;

		// Casting construction.
		//
		template<typename Y> requires ( !xstd::Same<T, Y> )
		constexpr phys_ptr( phys_ptr<Y> other ) : address( other.address ) {}

		// Behaves like a C++ pointer.
		//
		constexpr bool valid() const { return int64_t( address ) >= 0; }
		constexpr xstd::any_ptr get() const { return valid() ? get_phys_base() + address : nullptr; }
		constexpr operator T*() const { return get(); }
		constexpr T* operator->() const { return get(); }
		explicit constexpr operator bool() const { return valid(); }

		// Increment / decrement.
		//
		using K = std::conditional_t<xstd::Void<T>, char, T>;
		constexpr phys_ptr& operator+=( ptrdiff_t p ) { address += p * sizeof( K ); return *this; };
		constexpr phys_ptr& operator-=( ptrdiff_t p ) { address -= p * sizeof( K ); return *this; };
		constexpr phys_ptr& operator++() { address += sizeof( K ); return *this; };
		constexpr phys_ptr& operator--() { address -= sizeof( K ); return *this; };

		constexpr phys_ptr operator+( ptrdiff_t d ) const { auto s = *this; s += d; return s; }
		constexpr phys_ptr operator-( ptrdiff_t d ) const { auto s = *this; s -= d; return s; }
		constexpr phys_ptr operator++( int ) { auto s = *this; operator++(); return s; }
		constexpr phys_ptr operator--( int ) { auto s = *this; operator--(); return s; }

		// Comparison.
		//
		constexpr auto operator<=>( const phys_ptr& o ) const noexcept = default;
		template<typename Y> requires ( !xstd::Same<T, Y> )
		constexpr auto operator<=>( const phys_ptr<Y>& o ) const noexcept { return address <=> o.address; }
	};
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

	// Index of the self referencing page table entry and the bases, physical memory information.
	//
#if __has_xcxx_builtin(__builtin_fetch_dynamic)
	FORCE_INLINE inline void set_pxe_base_div8( uint64_t value ) { __builtin_store_dynamic( "@.pxe_base", ( void* ) value ); }
	FORCE_INLINE CONST_FN inline uint64_t pxe_base_div8() { return ( uint64_t ) __builtin_fetch_dynamic( "@.pxe_base" ); }

	FORCE_INLINE inline void set_self_ref_index( uint64_t value ) { __builtin_store_dynamic( "@.self_ref_idx", ( void* ) value ); }
	FORCE_INLINE CONST_FN inline uint64_t self_ref_index() { return ( uint64_t ) __builtin_fetch_dynamic( "@.self_ref_idx" ); }

	FORCE_INLINE CONST_FN inline uint64_t pfn_mask() { return ( uint64_t ) __builtin_fetch_dynamic( "@.mmu_pfn_mask" ); }
	FORCE_INLINE CONST_FN inline bitcnt_t pa_bits() { return ( bitcnt_t ) ( uint32_t ) ( uint64_t ) __builtin_fetch_dynamic( "@.mmu_pa_bits" ); }
	FORCE_INLINE inline void set_pa_bits( bitcnt_t value ) {
		__builtin_store_dynamic( "@.mmu_pa_bits", ( void* ) ( uint64_t ) value );
		__builtin_store_dynamic( "@.mmu_pfn_mask", ( void* ) ( xstd::fill_bits( value ) >> 12 ) );
	}
#else
	inline uint64_t __pxe_base_div_8 = 0;
	FORCE_INLINE inline void set_pxe_base_div8( uint64_t value ) { __pxe_base_div_8 = value; }
	FORCE_INLINE CONST_FN inline uint64_t pxe_base_div8() { return __pxe_base_div_8; }

	inline uint64_t __self_ref_index = 0;
	FORCE_INLINE inline void set_self_ref_index( uint64_t value ) { __self_ref_index = value; }
	FORCE_INLINE CONST_FN inline uint64_t self_ref_index() { return __self_ref_index; }

	inline bitcnt_t ___pa_bits = 0;
	FORCE_INLINE CONST_FN inline uint64_t pfn_mask() { return xstd::fill_bits( ___pa_bits ) >> 12; }
	FORCE_INLINE CONST_FN inline bitcnt_t pa_bits() { return ___pa_bits; }
	FORCE_INLINE inline void set_pa_bits( bitcnt_t value ) { ___pa_bits = value; }
#endif
	FORCE_INLINE CONST_FN inline bool has_1gb_pages() { return static_cpuid_s<0x80000001, 0, cpuid_eax_80000001>.edx.pages_1gb_available; }

	// Virtual address details.
	//
	FORCE_INLINE CONST_FN inline constexpr bool is_cannonical( xstd::any_ptr ptr ) { return ( ptr >> ( va_bits - 1 ) ) == 0 || ( int64_t( ptr ) >> ( va_bits - 1 ) ) == -1; }
	FORCE_INLINE CONST_FN inline constexpr xstd::any_ptr make_cannonical( xstd::any_ptr ptr ) { return xstd::any_ptr( int64_t( ptr << sx_bits ) >> sx_bits ); }
	FORCE_INLINE CONST_FN inline constexpr uint64_t page_size( int8_t depth ) { return 1ull << ( 12 + ( 9 * depth ) ); }
	FORCE_INLINE CONST_FN inline constexpr uint64_t page_offset( xstd::any_ptr ptr ) { return ptr & 0xFFF; }
	FORCE_INLINE CONST_FN inline constexpr uint64_t pt_index( xstd::any_ptr ptr, int8_t level ) { return ( ptr >> ( 12 + 9 * level ) ) & 511; }
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
			if constexpr ( N == page_table_depth )
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
#if __has_xcxx_builtin(__builtin_fetch_dynamic)
		switch ( depth )
		{
#if XSTD_IA32_LA57
			case pml5e_level:	 return ( pt_entry_64* ) __builtin_fetch_dynamic( "@.tbl_pml5e" );
#endif
			case pml4e_level:	 return ( pt_entry_64* ) __builtin_fetch_dynamic( "@.tbl_pml4e" );
			case pdpte_level:	 return ( pt_entry_64* ) __builtin_fetch_dynamic( "@.tbl_pdpte" );
			case pde_level:	 return ( pt_entry_64* ) __builtin_fetch_dynamic( "@.tbl_pde" );
			case pte_level:    return ( pt_entry_64* ) __builtin_fetch_dynamic( "@.tbl_pte" );
			default:           unreachable();
		}
#else
		xstd::any_ptr ptr = pxe_base_div8();
		if ( xstd::const_condition( depth == pxe_level ) )
			return ( pt_entry_64* ) ( ptr << 3 );
		auto shift = 12 + ( pxe_level - depth ) * 9;
		return ( pt_entry_64* ) ( ( ptr >> ( shift - 3 ) ) << shift );
#endif
	}

	// Recursive page table lookup.
	//
	FORCE_INLINE CONST_FN inline pt_entry_64* get_pte( xstd::any_ptr ptr, int8_t depth )
	{
#if __has_xcxx_builtin(__builtin_fetch_dynamic)
		pt_entry_64* tbl = locate_page_table( depth );
		return &tbl[ ( ptr << sx_bits ) >> ( sx_bits + 12 + 9 * depth ) ];
#else
		uint64_t base = pxe_base_div8();
		auto important_bits = ( page_table_depth - depth ) * 9;
		uint64_t tmp = shrd( base, ptr >> ( 12 + 9 * depth ), important_bits );
		return ( pt_entry_64* ) rotlq( tmp, important_bits + 3 );
#endif
	}
	FORCE_INLINE CONST_FN inline pt_entry_64* get_pte( xstd::any_ptr ptr ) { return get_pte( ptr, pte_level ); }
	FORCE_INLINE CONST_FN inline pt_entry_64* get_pde( xstd::any_ptr ptr ) { return get_pte( ptr, pde_level ); }
	FORCE_INLINE CONST_FN inline pt_entry_64* get_pdpte( xstd::any_ptr ptr ) { return get_pte( ptr, pdpte_level ); }
	FORCE_INLINE CONST_FN inline pt_entry_64* get_pml4e( xstd::any_ptr ptr ) { return get_pte( ptr, pml4e_level ); }
#if XSTD_IA32_LA57
	FORCE_INLINE CONST_FN inline pt_entry_64* get_pml5e( xstd::any_ptr ptr ) { return get_pte( ptr, pml5e_level ); }
#endif
	FORCE_INLINE CONST_FN inline pt_entry_64* get_pxe( xstd::any_ptr ptr ) { return get_pte( ptr, pxe_level ); }
	FORCE_INLINE CONST_FN inline pt_entry_64* get_pxe_by_index( uint32_t index ) { return &locate_page_table( pxe_level )[ index ]; }

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
			if ( !entry->present || ( n <= max_large_page_level && entry->large_page ) ) [[unlikely]]
				return { entry, n };
		}

		// Return the PTE.
		//
		return { get_pte( ptr ), pte_level };
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
		if ( hierarchy[ 0 ] != self_ref_index() )
			return { nullptr, -1 };

		size_t n = 1;
		while ( n != page_table_depth && hierarchy[ n ] == self_ref_index() )
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
			for ( auto it = ptr; it < ( ptr + length ); ) {
				auto [pte, depth] = lookup_pte( it );

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
		set_self_ref_index( idx );
		set_pxe_base_div8( uint64_t( locate_page_table( pxe_level, idx ) ) >> 3 );
		set_pa_bits( std::max<bitcnt_t>( ( bitcnt_t ) static_cpuid_s<0x80000008, 0, cpuid_eax_80000008>.eax.number_of_physical_address_bits, 48 ) );

#if __has_xcxx_builtin(__builtin_fetch_dynamic)
#if XSTD_IA32_LA57
		__builtin_store_dynamic( "@.tbl_pml5e", locate_page_table( pml5e_level, idx ) );
#endif
		__builtin_store_dynamic( "@.tbl_pml4e", locate_page_table( pml4e_level, idx ) );
		__builtin_store_dynamic( "@.tbl_pdpte", locate_page_table( pdpte_level, idx ) );
		__builtin_store_dynamic( "@.tbl_pde", locate_page_table( pde_level, idx ) );
		__builtin_store_dynamic( "@.tbl_pte", locate_page_table( pte_level, idx ) );
#endif
	}
};