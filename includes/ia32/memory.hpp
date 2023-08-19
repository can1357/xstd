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
	extern void ipi_flush_tlb( any_ptr ptr, size_t length = 0x1000 );

	// Mapping of physical memory.
	//
	extern any_ptr map_physical_memory_range( uint64_t address, size_t length, bool cached = false );
	extern void unmap_physical_memory_range( any_ptr pointer, size_t length );

	// Identity mapping of physical memory, optional.
	//
	static constexpr size_t phys_base_padding_pages = 512;
#if __has_xcxx_builtin(__builtin_fetch_dynamic)
	FORCE_INLINE inline void set_phys_base( const void* value ) { __builtin_store_dynamic( "@.physidmap", value ); }
	FORCE_INLINE CONST_FN inline any_ptr get_phys_base() { return __builtin_fetch_dynamic( "@.physidmap" ); }
#else
	inline const void* __phys_id_map = nullptr;
	FORCE_INLINE inline void set_phys_base( const void* value ) { __phys_id_map = value; }
	FORCE_INLINE CONST_FN inline any_ptr get_phys_base() { return __phys_id_map; }
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
		constexpr any_ptr get() const { return valid() ? get_phys_base() + address : nullptr; }
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
	static constexpr bitcnt_t page_table_depth = ( XSTD_IA32_LA57 ? 5 : 4 );
	static constexpr bitcnt_t va_bits = page_table_depth * 9 + 12;
	static constexpr bitcnt_t sx_bits = 64 - va_bits;

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
	FORCE_INLINE inline void set_pxe_base( pt_entry_64* value ) { __builtin_store_dynamic( "@.pxe_base", ( void* ) value ); }
	FORCE_INLINE CONST_FN inline pt_entry_64* pxe_base() { return ( pt_entry_64* ) __builtin_fetch_dynamic( "@.pxe_base" ); }

	FORCE_INLINE inline void set_self_ref_index( uint64_t value ) { __builtin_store_dynamic( "@.self_ref_idx", ( void* ) value ); }
	FORCE_INLINE CONST_FN inline uint64_t self_ref_index() { return ( uint64_t ) __builtin_fetch_dynamic( "@.self_ref_idx" ); }

	FORCE_INLINE CONST_FN inline uint64_t pfn_mask() { return ( uint64_t ) __builtin_fetch_dynamic( "@.mmu_pfn_mask" ); }
	FORCE_INLINE CONST_FN inline bitcnt_t pa_bits() { return ( bitcnt_t ) ( uint32_t ) ( uint64_t ) __builtin_fetch_dynamic( "@.mmu_pa_bits" ); }
	FORCE_INLINE inline void set_pa_bits( bitcnt_t value ) {
		__builtin_store_dynamic( "@.mmu_pa_bits", ( void* ) ( uint64_t ) value );
		__builtin_store_dynamic( "@.mmu_pfn_mask", ( void* ) ( xstd::fill_bits( value ) >> 12 ) );
	}

	FORCE_INLINE CONST_FN inline size_t max_pfn() { return ( size_t ) __builtin_fetch_dynamic( "@.maxpfn" ); }
	FORCE_INLINE CONST_FN inline size_t max_valid_pfn() { return ( size_t ) __builtin_fetch_dynamic( "@.maxvalidpfn" ); }
	FORCE_INLINE inline size_t set_max_valid_pfn( size_t value ) {
		size_t max_pfn = xstd::align_up( value + phys_base_padding_pages, 512 );
		__builtin_store_dynamic( "@.maxpfn", ( void* ) max_pfn );
		__builtin_store_dynamic( "@.maxvalidpfn", ( void* ) value );
		return max_pfn;
	}
#else
	inline pt_entry_64* __pxe_base = nullptr;
	FORCE_INLINE inline void set_pxe_base( pt_entry_64* value ) { __pxe_base = value; }
	FORCE_INLINE CONST_FN inline pt_entry_64* pxe_base() { return __pxe_base; }

	inline uint64_t __self_ref_index = 0;
	FORCE_INLINE inline void set_self_ref_index( uint64_t value ) { __self_ref_index = value; }
	FORCE_INLINE CONST_FN inline uint64_t self_ref_index() { return __self_ref_index; }

	inline bitcnt_t ___pa_bits = 0;
	FORCE_INLINE CONST_FN inline uint64_t pfn_mask() { return xstd::fill_bits( ___pa_bits ) >> 12; }
	FORCE_INLINE CONST_FN inline bitcnt_t pa_bits() { return ___pa_bits; }
	FORCE_INLINE inline void set_pa_bits( bitcnt_t value ) { ___pa_bits = value; }

	inline size_t ___max_valid_pfn = 0;
	FORCE_INLINE CONST_FN inline size_t max_pfn() { return xstd::align_up( ___max_valid_pfn + phys_base_padding_pages, 512 ); }
	FORCE_INLINE CONST_FN inline size_t max_valid_pfn() { return ___max_valid_pfn; }
	FORCE_INLINE inline size_t set_max_valid_pfn( size_t value ) { 
		___max_valid_pfn = value; 
		return xstd::align_up( value + phys_base_padding_pages, 512 );
	}
#endif
	FORCE_INLINE CONST_FN inline bool has_1gb_pages() { return static_cpuid_s<0x80000001, 0, cpuid_eax_80000001>.edx.pages_1gb_available; }

	// Virtual address details.
	//
	FORCE_INLINE CONST_FN inline constexpr bool is_cannonical( any_ptr ptr ) { return ( ptr >> ( va_bits - 1 ) ) == 0 || ( int64_t( ptr ) >> ( va_bits - 1 ) ) == -1; }
	FORCE_INLINE CONST_FN inline constexpr any_ptr make_cannonical( any_ptr ptr ) { return any_ptr( int64_t( ptr << sx_bits ) >> sx_bits ); }
	FORCE_INLINE CONST_FN inline constexpr uint64_t page_size( int8_t depth ) { return 1ull << ( 12 + ( 9 * depth ) ); }
	FORCE_INLINE CONST_FN inline constexpr uint64_t page_offset( any_ptr ptr ) { return ptr & 0xFFF; }
	FORCE_INLINE CONST_FN inline constexpr uint64_t pt_index( any_ptr ptr, int8_t level ) { return ( ptr >> ( 12 + 9 * level ) ) & 511; }
	FORCE_INLINE CONST_FN inline constexpr uint64_t px_index( any_ptr ptr ) { return pt_index( ptr, pxe_level ); }

	// Utilities for self-referencing page-table entry.
	//
	FORCE_INLINE CONST_FN inline pt_entry_64* locate_page_table( int8_t depth, std::optional<uint32_t> self_ref_idx = std::nullopt )
	{
#if __has_xcxx_builtin(__builtin_fetch_dynamic)
		if ( !self_ref_idx.has_value() ) {
			switch ( depth ) {
				case pxe_level:    return pxe_base();
#if XSTD_IA32_LA57
				case pml4e_level:	 return ( pt_entry_64* ) __builtin_fetch_dynamic( "@.tbl_pml4e" );
#endif
				case pdpte_level:	 return ( pt_entry_64* ) __builtin_fetch_dynamic( "@.tbl_pdpte" );
				case pde_level:	 return ( pt_entry_64* ) __builtin_fetch_dynamic( "@.tbl_pde" );
				case pte_level:    return ( pt_entry_64* ) __builtin_fetch_dynamic( "@.tbl_pte" );
				default:           unreachable();
			}
		}
#endif
		if ( self_ref_idx.has_value() ) {
			constexpr uint64_t base_coeff =
#if XSTD_IA32_LA57
				+ ( 1ull << ( 12 + 9 * pml5e_level ) )
#endif
				+ ( 1ull << ( 12 + 9 * pml4e_level ) )
				+ ( 1ull << ( 12 + 9 * pdpte_level ) )
				+ ( 1ull << ( 12 + 9 * pde_level ) )
				+ ( 1ull << ( 12 + 9 * pte_level ) );
			uint64_t coeff = ( base_coeff << sx_bits ) & ~( ( 1ull << ( sx_bits + 12 + 9 * ( pxe_level - depth ) ) ) - 1 );
			return ( pt_entry_64* ) uint64_t( int64_t( *self_ref_idx * coeff ) >> sx_bits );
		} else {
			any_ptr ptr = pxe_base();
			if ( xstd::const_condition( depth == pxe_level ) )
				return ptr;
			auto shift = 12 + ( pxe_level - depth ) * 9;
			return any_ptr( ( ptr >> shift ) << shift );
		}
	}
	FORCE_INLINE CONST_FN inline pt_entry_64* get_pte( any_ptr ptr, int8_t depth = pte_level, std::optional<uint32_t> self_ref_idx = std::nullopt )
	{
		uintptr_t aligned = ptr.address;
		uintptr_t offset = ( aligned << sx_bits ) >> ( sx_bits + 9 + 9 * depth );
		offset &= ~7ull;
		return ( pt_entry_64* ) ( uintptr_t( locate_page_table( depth, self_ref_idx ) ) + offset );
	}
	FORCE_INLINE CONST_FN inline pt_entry_64* get_pxe_by_index( uint32_t index, std::optional<uint32_t> self_ref_idx = std::nullopt ) 
	{ 
		return locate_page_table( pxe_level, self_ref_idx ) + index; 
	}
	FORCE_INLINE CONST_FN inline std::array<pt_entry_64*, page_table_depth> get_pte_hierarchy( any_ptr ptr, std::optional<uint32_t> self_ref_idx = std::nullopt ) 
	{
		return xstd::make_constant_series<page_table_depth>( [ & ] ( auto c ) { return get_pte( ptr, c, self_ref_idx ); } );
	}
	FORCE_INLINE CONST_FN inline any_ptr pte_to_va( any_ptr pte, int8_t level = pte_level ) 
	{ 
		return ( ( int64_t( pte ) << ( sx_bits + 12 + ( 9 * level ) - 3 ) ) >> sx_bits ); 
	}
	FORCE_INLINE CONST_FN inline std::pair<any_ptr, int8_t> rlookup_pte( any_ptr pte, std::optional<uint32_t> self_ref_idx = std::nullopt )
	{
		// Xor with PXE base ignoring 8-byte offsets.
		//
		uintptr_t base = uintptr_t( pxe_base() );
		if ( self_ref_idx.has_value() ) {
			base = uintptr_t( locate_page_table( pxe_level, *self_ref_idx ) );
		}
		
		// Count the highest bit mismatching, set lowest bit to optimize against tzcnt with 0.
		//
		bitcnt_t mismatch = xstd::msb( ( ( base ^ pte ) >> 3 ) | 0x1 );

		// If first mismatch is below the PXE level:
		// 
		if ( mismatch < ( va_bits - 9 - 3 ) ) {
			// Determine the level, return the mapped VA.
			//
			bitcnt_t level = pxe_level - ( mismatch / 9 );
			return { pte_to_va( pte & ~3ull, ( int8_t ) level ), ( int8_t ) level };
		}
		return { nullptr, -1 };
	}

	// Propagates flags from higher-level directory entry into the current one to form "effective" protection flags for a page-walk.
	//
	static constexpr uint64_t pte_viral_and = PT_ENTRY_64_USER_FLAG | PT_ENTRY_64_WRITE_FLAG;
	static constexpr uint64_t pte_viral_or =  PT_ENTRY_64_EXECUTE_DISABLE_FLAG;
	FORCE_INLINE CONST_FN inline constexpr pt_entry_64 pte_propagate_viral( pt_entry_64 parent, pt_entry_64 current ) {
		current.flags |= parent.flags & pte_viral_or;
		current.flags ^= ( current.flags & ~parent.flags ) & pte_viral_and;
		return current;
	}
	
	// Flags and status for pagewalk.
	//
	enum walk_flags : uint16_t {
		// Emulation flags.
		//
		pw_mark_accessed =     1 << 0, // Sets accessed flag.
		pw_mark_dirty =        1 << 1, // Sets dirty flag. (implies accessed).
		pw_clamp_pfn =         1 << 2,

		// Assertation flags.
		//
		pw_assert_ex =         1 << 3, // Fails if execute disable is set.
		pw_assert_su =         1 << 4, // Fails if user page.
		pw_assert_us =         1 << 5, // Fails if supervisor page.
		pw_assert_wr =         1 << 6, // Fails if read-only page.
		pw_assert_valid =      1 << 7, // Fails if reserved flags are set.

		// CPU flags.
		//
		pw_cpu_nxe_off =       1 << 8, // Emulates EFER.NXE = 0.

		// Presets.
		//
		pw_preset_relaxed =    0,
		pw_preset_strict =     pw_assert_valid | pw_clamp_pfn,
		pw_preset_emulator =   pw_preset_strict | pw_mark_accessed | pw_mark_dirty
	};
	enum class walk_status : int8_t {
		pending = -1,    // At page directory, recurse deeper (Only valid as a walk step).

		// Page state.
		//
		ok = 0,          // Found the page.
		not_present,     // Not present.
		
		// Protection faults.
		//
		xd,              // Executed disable violation.
		wp,              // Write protect violation.
		cpl,             // US/SU violation.
		
		// Reserved flag violations.
		//
		rsvd,        // - Pseudo:Range start.
		rsvd_nx,     // - NX bit is not defined.
		rsvd_large,  // - Large page at invalid level.
		rsvd_pfn,    // - Physical address has more bits than CPU supports or is misaligned.
	};

	// CPU state emulated in the walk.
	// - Used to calculate appropriate walk flags and generating #PF exception codes.
	//
	struct walk_request {
		// Special registers.
		//
		cr4              cr4 =   { .flags = CR4_SMAP_ENABLE_FLAG | CR4_SMEP_ENABLE_FLAG };
		cr0              cr0 =   { .flags = CR0_WRITE_PROTECT_FLAG };
		efer_register    efer =  { .flags = IA32_EFER_EXECUTE_DISABLE_BIT_ENABLE_FLAG };
		rflags           efl =   { .flags = 0 };

		// Source exception for access flags.
		//
		page_fault_exception access = { .flags = 0 };

		// Calculates the final walker flags.
		//
		FORCE_INLINE CONST_FN uint16_t get_flags( uint16_t wf = pw_preset_emulator ) const {
			// If write access:
			//
			if ( access.write ) {
				// Assert WP if user-mode or CR0.WP != 0.
				//
				if ( access.user_mode_access || cr0.write_protect ) {
					wf |= pw_assert_wr;
				}
			}
			// If read or execute:
			//
			else {
				// Clear mark dirty.
				//
				wf &= ~pw_mark_dirty;

				// If execute:
				//
				if ( access.execute ) {
					// Assert executable.
					//
					wf |= pw_assert_ex;

					// If supervisor and SMEP is enabled, assert su.
					//
					if ( !access.user_mode_access && cr4.smep_enable ) {
						wf |= pw_assert_su;
					}
				}
			}

			// If supervisor and SMAP is enabled, assert su, otherwise assert us if usermode.
			//
			if ( access.user_mode_access ) {
				wf |= pw_assert_us;
			} else if ( cr4.smap_enable && !efl.alignment_check_flag ) {
				wf |= pw_assert_su;
			}

			// Mark NX as reserved EFER.NXE = 0.
			//
			if ( !efer.execute_disable_bit_enable ) {
				wf |= pw_cpu_nxe_off;
				wf &= ~pw_assert_ex;
			}
			return wf;
		}

		// Returns the raised page_fault exception given the status and the flags.
		//
		FORCE_INLINE CONST_FN static constexpr page_fault_exception make_exception( page_fault_exception access, walk_status state, uint16_t flags ) {
			( void ) flags;

			// Propagate access flags.
			//
			page_fault_exception pf = { .flags = 0 };
			pf.hlat =             access.hlat;
			pf.sgx =              access.sgx;
			pf.shadow_stack =     access.shadow_stack;
			pf.write =            access.write;
			pf.execute =          access.execute;
			pf.user_mode_access = access.user_mode_access;

			// Set page flags.
			//
			pf.present =                state != walk_status::not_present;
			pf.reserved_bit_violation = state >= walk_status::rsvd;

			// TODO: protection_key_violation
			return pf;
		}
		FORCE_INLINE CONST_FN constexpr page_fault_exception make_exception( walk_status state, uint16_t flags ) const {
			return make_exception( access, state, flags );
		}
	};

	// Pagewalk interface base structures.
	//
	struct walk_parameters {
		// Known constants.
		//
		uint64_t pfn_mask =      mem::pfn_mask();
		bool     has_1gb_pages = mem::has_1gb_pages();
	};

	// Page walker interface.
	//
	struct walk_base {
		FORCE_INLINE PURE_FN static pt_entry_64 read_pte( walk_base* s, const walk_parameters& param ) {
			( void ) param;
			return *s->ppte;
		}
		FORCE_INLINE static void set_pte_flags( walk_base* s, const walk_parameters& param, uint64_t fl ) {
			( void ) param;
			if ( xstd::const_condition( !fl ) || ( s->ppte->flags & fl ) == fl ) {
				return;
			} else if ( xstd::const_condition( xstd::is_pow2( fl ) ) ) {
				xstd::atomic_bit_set( s->ppte->flags, xstd::msb( fl ) );
			} else {
				std::atomic_ref{ s->ppte->flags }.fetch_or( fl );
			}
		}
		FORCE_INLINE static bool next_pte( walk_base* s, const walk_parameters& param, pt_level level ) {
			( void ) s;
			( void ) level;
			( void ) param;
			return false;
		}
		FORCE_INLINE static void phys_write( const walk_parameters& param, uint64_t dst_pa, any_ptr src_va, size_t n ) {
			( void ) param;
			memcpy( mem::get_phys_base() + dst_pa, src_va, n );
		}
		FORCE_INLINE static void phys_read( const walk_parameters& param, any_ptr dst_va, uint64_t src_pa, size_t n ) {
			( void ) param;
			memcpy( dst_va, mem::get_phys_base() + src_pa, n );
		}
	
		// Effective value of the resolved PTE.
		//
		pt_entry_64  value = { .flags = pte_viral_and };

		// Pointer to the lowest PTE and the level of the entry.
		//
		pt_entry_64* ppte = nullptr;
		pt_level     level = {};

		// Virtual address.
		//
		any_ptr      virtual_address = nullptr;

		// Pagewalk status.
		//
		walk_status  status = {};

		// Default ctor/move/copy.
		//
		constexpr walk_base() = default;
		constexpr walk_base( walk_base&& ) noexcept = default;
		constexpr walk_base( const walk_base& ) noexcept = default;
		constexpr walk_base& operator=( walk_base&& ) noexcept = default;
		constexpr walk_base& operator=( const walk_base& ) noexcept = default;

		// Parameterized construction.
		//
		constexpr walk_base( any_ptr virtual_address ) {
			this->virtual_address = virtual_address;
		}
		
		// Useful utilities for resolved addresses.
		//
		FORCE_INLINE constexpr any_ptr get_va_base() const {
			size_t psize = mem::page_size( level );
			return virtual_address & ~( psize - 1 );
		}
		FORCE_INLINE constexpr any_ptr get_va_end() const {
			size_t psize = mem::page_size( level );
			return ( virtual_address + psize ) & ~( psize - 1 );
		}
		FORCE_INLINE constexpr uintptr_t get_pa() const {
			size_t psize = mem::page_size( level );
			uintptr_t pa = ( value.page_frame_number << 12 );
			pa |= virtual_address & ( psize - 1 );
			return pa;
		}

		// OK check.
		//
		FORCE_INLINE constexpr explicit operator bool() const {
			return status == walk_status::ok;
		}

		// Feeds the next entry to the walker, returns the status.
		//
		FORCE_INLINE constexpr walk_status step( pt_entry_64 pte, pt_level level, const walk_parameters& params, uint16_t flags = pw_preset_relaxed ) {
			this->value = ( pte = pte_propagate_viral( this->value, pte ) );
			this->level = level;

			// Check if present.
			//
			if ( !pte.present ) {
				[[unlikely]];
				return walk_status::not_present;
			}

			// Check if reserved bits are set.
			//
			const bool into_directory = level != pte_level && !pte.large_page;
			if ( flags & pw_assert_valid ) {
				uint64_t pfn_mask_eff = params.pfn_mask;

				// If describing page:
				//
				if ( !into_directory ) {
					// Check if we can have a page of this size:
					//
					if ( level > ( params.has_1gb_pages ? pdpte_level : pde_level ) ) {
						return walk_status::rsvd_large;
					}

					// Enforce alignment.
					//
					pfn_mask_eff >>= ( level * 9 );
					pfn_mask_eff <<= ( level * 9 );
				}

				// Check if PFN is invalid.
				//
				if ( pte.page_frame_number & ~pfn_mask_eff ) {
					return walk_status::rsvd_pfn;
				}

				// Check if protection is valid.
				//
				if ( pte.execute_disable && ( flags & pw_cpu_nxe_off ) ) {
					return walk_status::rsvd_nx;
				}
			}

			// Clamp the PFN.
			//
			if ( flags & pw_clamp_pfn ) {
				pte.page_frame_number = std::min<uint64_t>( pte.page_frame_number, max_valid_pfn() );
			}

			// Continue if entry maps into a page table, ignoring permissions until last level.
			//
			if ( into_directory || level > max_large_page_level ) {
				return walk_status::pending;
			}

			// Check permissions.
			//
			if ( flags & pw_assert_su ) {
				if ( pte.user ) {
					return walk_status::cpl;
				}
			} else if ( flags & pw_assert_us ) {
				if ( !pte.user ) {
					return walk_status::cpl;
				}
			}
			if ( flags & pw_assert_ex ) {
				if ( pte.execute_disable ) {
					return walk_status::xd;
				}
			} else if ( flags & pw_assert_wr ) {
				if ( !pte.write ) {
					return walk_status::wp;
				}
			}
			return walk_status::ok;
		}
	};
	template<typename Walker>
	struct iwalker : walk_base {
		template<typename W = Walker, typename Pa = typename W::parameters>
		FORCE_INLINE walk_status forward( pt_level level, const Pa& params, uint16_t flags = pw_preset_relaxed ) {
			// Fetch the PTE.
			//
			if ( !W::next_pte( ( W* ) this, params, level ) ) {
				return walk_status::not_present;
			}
			// Yield if not done.
			//
			auto status = this->step( W::read_pte( ( W* ) this, params ), level, params, flags );
			if ( status == walk_status::pending ) {
				if ( level == pte_level ) {
					unreachable();
				} else {
					[[likely]];
				}

				// Set accessed if relevant.
				//
				if ( flags & pw_mark_accessed )
					W::set_pte_flags( ( W* ) this, params, PT_ENTRY_64_ACCESSED_FLAG );
				return status;
			}

			// Set accessed/dirty if relevant.
			//
			if ( flags & ( pw_mark_accessed | pw_mark_dirty ) ) {
				if ( status != walk_status::not_present ) {
					uint64_t pte_flags = PT_ENTRY_64_ACCESSED_FLAG;
					//if ( flags & pw_mark_accessed )
					//	pte_flags |= PT_ENTRY_64_ACCESSED_FLAG;
					if ( flags & pw_mark_dirty )
						if ( status == walk_status::ok )
							pte_flags |= PT_ENTRY_64_DIRTY_FLAG;
					W::set_pte_flags( ( W* ) this, params, pte_flags );
				}
			}
			return status;
		}

		// Perform wrapper.
		//
		template<typename W = Walker, typename Pa = typename W::parameters>
		FORCE_INLINE static W perform( any_ptr virtual_address, const Pa& params = {}, uint16_t flags = pw_preset_relaxed ) {
			const Pa local_params{ params };
			W walker{ virtual_address };
			__hint_unroll()
			for ( int8_t n = pxe_level; n >= pte_level; n-- ) {
				walker.status = walker.forward( pt_level( n ), local_params, flags );
				if ( walker.status != walk_status::pending ) {
					break;
				}
			}
			return walker;
		}
	};

	// Self-ref based page walking.
	//
	struct walk : iwalker<walk> {
		struct parameters : walk_parameters {
			// Self-reference index.
			//
			std::optional<uint32_t> self_ref_idx;
		};
		FORCE_INLINE static bool next_pte( walk* s, const parameters& param, pt_level level ) {
			s->ppte = get_pte( s->virtual_address, level, param.self_ref_idx );
			return true;
		}
	};
	struct prewalk : iwalker<prewalk> {
		struct parameters : walk_parameters {
			std::array<pt_entry_64*, page_table_depth> hierarchy;
		};
		FORCE_INLINE static bool next_pte( prewalk* s, const parameters& param, pt_level level ) {
			s->ppte = param.hierarchy[ ( int ) level ];
			return true;
		}
	};

	// Physical memory based page walking.
	//
	struct physwalk : iwalker<physwalk> {
		struct parameters : walk_parameters {
			// Base of physical memory.
			//
			any_ptr  phys_base = mem::get_phys_base();

			// Top level page table.
			//
			cr3      directory;
		};

		FORCE_INLINE static bool next_pte( physwalk* s, const parameters& param, pt_level level ) {
			pt_entry_64* table;
			if ( level == pxe_level ) {
				table = param.phys_base + ( param.directory.address_of_page_directory << 12 );
			} else {
				table = param.phys_base + ( s->value.page_frame_number << 12 );
			}
			s->ppte = &table[ pt_index( s->virtual_address, level ) ];
			return true;
		}
	};

	// Virtual memory operations on foreign page tables.
	//
		// Memory operation result.
	//
	struct vm_result {
		any_ptr     fault_address = {};
		walk_status status =        {};

		// OK check.
		//
		FORCE_INLINE constexpr explicit operator bool() const {
			return status == walk_status::ok;
		}
	};
	enum class vm_operator {
		read,
		write,
		// TODO: XCHG, CMPXCHG, etc.
	};
	namespace detail {
		// Determines if we can execute this virtual memory operation atomically as in, either all globally visible or none on fault.
		//
		FORCE_INLINE inline constexpr bool is_atomic_vop( any_ptr va, size_t n, vm_operator op ) {
			( void ) op;
			( void ) va;
			return n <= 0x1000;
		}

		// Applies the virtual memory operation atomically, assumes the well-behaved check passes.
		//
		template<vm_operator Op, typename W, typename Pa = typename W::parameters>
		FORCE_INLINE inline vm_result apply_vop_atomic( any_ptr buffer, any_ptr va, size_t n, const Pa& params, uint16_t flags ) {
			// Determine whether or not N can be reduced to a small constant, allocate a temporary buffer at the 
			// beginning of the function to avoid alloca if so.
			//
			const bool is_n_const =           xstd::is_consteval( n );
			const bool is_small_copy_viable = is_n_const && ( n <= 128 );
			uint8_t tmp[ ( is_small_copy_viable ? n : 1 ) * 3 - 2 ];

			// Resolve the initial pointer.
			//
			walk_base w1 = W::perform( va, params, flags | pw_clamp_pfn );
			if ( !w1 ) [[unlikely]] {
				return {
					.fault_address = w1.virtual_address,
					.status =        w1.status
				};
			}
			uint64_t p_base = w1.get_pa();

			// If VA is aligned to the boundary of N, cannot overflow anyway, otherwise check if within boundary, if any of the checks pass, issue a memcpy and move on.
			//
			if ( ( is_n_const && xstd::is_pow2( n ) && xstd::is_aligned( va, n ) ) || ( va + n ) <= w1.get_va_end() ) [[likely]] {
				if constexpr ( Op == vm_operator::read ) {
					W::phys_read( params, buffer, p_base, n );
				} else {
					W::phys_write( params, p_base, buffer, n );
				}
				return {};
			}

			// Do a second translation.
			//
			walk_base w2 =  W::perform( w1.get_va_end(), params, flags | pw_clamp_pfn );
			if ( !w2 ) [[unlikely]] {
				return {
					.fault_address = w2.virtual_address,
					.status =        w2.status
				};
			}
			uint64_t p_hi = w2.get_pa();
			uint64_t p_lo = p_hi - n;

			// Do parial copies.
			//
			const size_t shift = p_base - p_lo;
			if constexpr ( Op == vm_operator::read ) {
				if ( is_small_copy_viable ) {
					uint8_t* baseline = &tmp[ n ];
					W::phys_read( params, baseline - shift,     p_lo,         n );
					W::phys_read( params, baseline + n - shift, p_hi,         n );
					memcpy( buffer, baseline, n );
				} else {
					W::phys_read( params, buffer,               p_lo + shift, n - shift );
					W::phys_read( params, buffer + n - shift,   p_hi,         shift );
				}
			} else {
				if ( is_small_copy_viable ) {
					uint8_t* baseline = &tmp[ n ];
					W::phys_read( params, baseline - shift,     p_lo,                          n );
					W::phys_read( params, baseline + n - shift, p_hi,                          n );
					memcpy( baseline, buffer, n );
					W::phys_write( params, p_lo,                baseline - shift,              n );
					W::phys_write( params, p_hi,                baseline + n - shift,          n );
				} else {
					W::phys_write( params, p_lo + shift,        buffer,                        n - shift );
					W::phys_write( params, p_hi,                buffer + n - shift,            shift );
				}
			}
			return {};
		}

		// Applies the virtual memory operation in a resumable fashion with no atomicity guarantees.
		//
		template<vm_operator Op, typename W, typename Pa = typename W::parameters>
		COLD inline vm_result apply_vop_resumable( any_ptr buffer, any_ptr va, size_t n, const Pa& params, uint16_t flags ) {
			any_ptr va_limit = va + n;
			ptrdiff_t va_to_buffer = buffer - va;
			while ( va < va_limit ) {
				// Translate the address, bail if there was an error.
				//
				walk_base translation = W::perform( va, params, flags | pw_clamp_pfn );
				if ( !translation ) [[unlikely]] {
					return {
						.fault_address = translation.virtual_address,
						.status =        translation.status
					};
				}

				// Determine the operation limit.
				//
				uint64_t p = translation.get_pa();
				any_ptr page_limit = translation.get_va_end();
				any_ptr copy_limit = std::min( page_limit, va_limit );
				size_t  copy_count = copy_limit - va;

				// Apply the operation.
				//
				if constexpr ( Op == vm_operator::read ) {
					W::phys_read(  params, va + va_to_buffer, p,                 copy_count );
				} else {
					W::phys_write( params, p,                 va + va_to_buffer, copy_count );
				}

				// Forward the iterator.
				//
				va = copy_limit;
			}
			return {};
		}
	};

	// Virtual memory operations on foreign page table.
	//
	template<vm_operator Op, typename W, typename Pa = typename W::parameters>
	FORCE_INLINE inline vm_result vm_apply( any_ptr buffer, any_ptr va, size_t n, const Pa& params = {}, uint16_t flags = pw_preset_relaxed ) {
		if ( detail::is_atomic_vop( va, n, Op ) ) [[likely]] {
			return detail::apply_vop_atomic<Op, W, Pa>( buffer, va, n,  params, flags );
		} else {
			return detail::apply_vop_resumable<Op, W, Pa>( buffer, va, n, params, flags );
		}
	}

	// Reads foreign memory.
	//
	template<typename W, typename Pa = typename W::parameters>
	FORCE_INLINE inline vm_result vm_read( any_ptr dst, any_ptr src, size_t n, const Pa& params = {}, uint16_t flags = pw_preset_relaxed ) {
		return vm_apply<vm_operator::read, W, Pa>( dst, src, n, params, flags );
	}
	template<typename W, typename Pa = typename W::parameters>
	FORCE_INLINE inline vm_result vm_write( any_ptr dst, any_ptr src, size_t n, const Pa& params = {}, uint16_t flags = pw_preset_relaxed ) {
		return vm_apply<vm_operator::write, W, Pa>( src, dst, n, params, flags );
	}


	// Fast page table lookup, no validation.
	//
	FORCE_INLINE PURE_FN inline std::pair<pt_entry_64*, int8_t> lookup_pte( std::array<pt_entry_64*, page_table_depth> hierarchy, pt_entry_64* accu = nullptr )
	{
		auto walk = prewalk::perform( nullptr, { .hierarchy = hierarchy } );
		if ( accu )
			accu->flags = walk.value.flags;
		return { walk.ppte, walk.level };
	}
	FORCE_INLINE PURE_FN inline std::pair<pt_entry_64*, int8_t> lookup_pte( any_ptr ptr, pt_entry_64* accu = nullptr, std::optional<uint32_t> self_ref_idx = std::nullopt ) 
	{
		auto walk = walk::perform( ptr, { .self_ref_idx = self_ref_idx } );
		if ( accu )
			accu->flags = walk.value.flags;
		return { walk.ppte, walk.level };
	}
	FORCE_INLINE PURE_FN inline std::pair<pt_entry_64*, int8_t> lookup_pte( any_ptr ptr, std::optional<uint32_t> self_ref_idx ) 
	{ 
		return lookup_pte( ptr, nullptr, self_ref_idx ); 
	}

	// Virtual address validation and translation.
	//
	FORCE_INLINE inline bool is_address_valid( any_ptr ptr, std::optional<uint32_t> self_ref_idx = std::nullopt )
	{
		if ( !is_cannonical( ptr ) ) [[unlikely]]
			return false;
		auto [pte, _] = lookup_pte( ptr, self_ref_idx );
		return pte->present;
	}

	// Gets the physical address associated with the virtual address.
	//
	FORCE_INLINE inline uint64_t get_physical_address( any_ptr ptr, std::optional<uint32_t> self_ref_idx = std::nullopt )
	{
		auto [pte, depth] = lookup_pte( ptr, self_ref_idx );
		if ( !pte->present ) return 0;
		return ( pte->page_frame_number << 12 ) | ( ptr & ( page_size( depth ) - 1 ) );
	}
	FORCE_INLINE inline uint64_t get_pfn( any_ptr ptr, std::optional<uint32_t> self_ref_idx = std::nullopt )
	{
		return get_physical_address( ptr, self_ref_idx ) >> 12;
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
		FORCE_INLINE inline void change_protection( any_ptr ptr, size_t length, protection_mask mask, std::optional<uint32_t> self_ref_idx = std::nullopt )
		{
			static constexpr uint64_t all_flags = prot_write | prot_read | prot_no_execute;

			// Iterate page boundaries:
			//
			for ( auto it = ptr; it < ( ptr + length ); ) {
				auto [pte, depth] = lookup_pte( it, self_ref_idx );

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
	FORCE_INLINE inline void change_protection( any_ptr ptr, size_t length, protection_mask mask, std::optional<uint32_t> self_ref_idx = std::nullopt ) { impl::change_protection<true>( ptr, length, mask, self_ref_idx ); }
	FORCE_INLINE inline void change_protection_no_ipi( any_ptr ptr, size_t length, protection_mask mask, std::optional<uint32_t> self_ref_idx = std::nullopt ) { impl::change_protection<false>( ptr, length, mask, self_ref_idx ); }

	// Initialization helper.
	//
	inline void init( uint32_t idx )
	{
		set_self_ref_index( idx );
		set_pxe_base( locate_page_table( pxe_level, idx ) );
		set_pa_bits( std::max<bitcnt_t>( ( bitcnt_t ) static_cpuid_s<0x80000008, 0, cpuid_eax_80000008>.eax.number_of_physical_address_bits, 48 ) );

#if __has_xcxx_builtin(__builtin_fetch_dynamic)
#if XSTD_IA32_LA57
		__builtin_store_dynamic( "@.tbl_pml4e", locate_page_table( pml4e_level, idx ) );
#endif
		__builtin_store_dynamic( "@.tbl_pdpte", locate_page_table( pdpte_level, idx ) );
		__builtin_store_dynamic( "@.tbl_pde", locate_page_table( pde_level, idx ) );
		__builtin_store_dynamic( "@.tbl_pte", locate_page_table( pte_level, idx ) );
#endif
	}
};