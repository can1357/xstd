#pragma once
#include <stdint.h>
#include <tuple>
#include <array>
#include <xstd/intrinsics.hpp>
#include <xstd/type_helpers.hpp>
#include <xstd/assert.hpp>

namespace ia32 {

/**
 * @defgroup intel_manual \
 *           Intel Manual
 *
 * @remarks All references are based on <b>Intel(R) 64 and IA-32 architectures software developer's manual combined volumes:
 *          1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4</b> (May 2018).
 * @{
 */
/**
 * @defgroup control_registers \
 *           Control registers
 *
 * Control registers (CR0, CR1, CR2, CR3, and CR4) determine operating mode of the processor and the characteristics of the
 * currently executing task. These registers are 32 bits in all 32-bit modes and compatibility mode.
 * In 64-bit mode, control registers are expanded to 64 bits. The MOV CRn instructions are used to manipulate the register
 * bits. Operand-size prefixes for these instructions are ignored. The following is also true:
 * - The control registers can be read and loaded (or modified) using the move-to-or-from-control-registers forms of the
 * MOV instruction. In protected mode, the MOV instructions allow the control registers to be read or loaded (at privilege
 * level 0 only). This restriction means that application programs or operating-system procedures (running at privilege
 * levels 1, 2, or 3) are prevented from reading or loading the control registers.
 * - Bits 63:32 of CR0 and CR4 are reserved and must be written with zeros. Writing a nonzero value to any of the upper 32
 * bits results in a general-protection exception, \#GP(0).
 * - All 64 bits of CR2 are writable by software.
 * - Bits 51:40 of CR3 are reserved and must be 0.
 * - The MOV CRn instructions do not check that addresses written to CR2 and CR3 are within the linear-address or
 * physical-address limitations of the implementation.
 * - Register CR8 is available in 64-bit mode only. The control registers are summarized below, and each architecturally
 * defined control field in these control registers is described individually.
 * - CR0 - Contains system control flags that control operating mode and states of the processor.
 * - CR1 - Reserved.
 * - CR2 - Contains the page-fault linear address (the linear address that caused a page fault).
 * - CR3 - Contains the physical address of the base of the paging-structure hierarchy and two flags (PCD and PWT). Only
 * the most-significant bits (less the lower 12 bits) of the base address are specified; the lower 12 bits of the address
 * are assumed to be 0. The first paging structure must thus be aligned to a page (4-KByte) boundary. The PCD and PWT flags
 * control caching of that paging structure in the processor's internal data caches (they do not control TLB caching of
 * page-directory information). When using the physical address extension, the CR3 register contains the base address of
 * the page-directorypointer table. In IA-32e mode, the CR3 register contains the base address of the PML4 table.
 * - CR4 - Contains a group of flags that enable several architectural extensions, and indicate operating system or
 * executive support for specific processor capabilities.
 * - CR8 - Provides read and write access to the Task Priority Register (TPR). It specifies the priority threshold value
 * that operating systems use to control the priority class of external interrupts allowed to interrupt the processor. This
 * register is available only in 64-bit mode. However, interrupt filtering continues to apply in compatibility mode.
 *
 * @see Vol3A[2.5(CONTROL REGISTERS)] (reference)
 * @{
 */
typedef union
{
  struct
  {
	/**
	* @brief Protection Enable
	*
	* [Bit 0] Enables protected mode when set; enables real-address mode when clear. This flag does not enable paging
	* directly. It only enables segment-level protection. To enable paging, both the PE and PG flags must be set.
	*
	* @see Vol3A[9.9(Mode Switching)]
	*/
	uint64_t protection_enable                                       : 1;
#define CR0_PROTECTION_ENABLE_BIT                                    0
#define CR0_PROTECTION_ENABLE_FLAG                                   0x01
#define CR0_PROTECTION_ENABLE_MASK                                   0x01
#define CR0_PROTECTION_ENABLE(_)                                     (((_) >> 0) & 0x01)

	/**
	* @brief Monitor Coprocessor
	*
	* [Bit 1] Controls the interaction of the WAIT (or FWAIT) instruction with the TS flag (bit 3 of CR0). If the MP flag is
	* set, a WAIT instruction generates a device-not-available exception (\#NM) if the TS flag is also set. If the MP flag is
	* clear, the WAIT instruction ignores the setting of the TS flag.
	*/
	uint64_t monitor_coprocessor                                     : 1;
#define CR0_MONITOR_COPROCESSOR_BIT                                  1
#define CR0_MONITOR_COPROCESSOR_FLAG                                 0x02
#define CR0_MONITOR_COPROCESSOR_MASK                                 0x01
#define CR0_MONITOR_COPROCESSOR(_)                                   (((_) >> 1) & 0x01)

	/**
	* @brief FPU Emulation
	*
	* [Bit 2] Indicates that the processor does not have an internal or external x87 FPU when set; indicates an x87 FPU is
	* present when clear. This flag also affects the execution of MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instructions.
	* When the EM flag is set, execution of an x87 FPU instruction generates a device-not-available exception (\#NM). This
	* flag must be set when the processor does not have an internal x87 FPU or is not connected to an external math
	* coprocessor. Setting this flag forces all floating-point instructions to be handled by software emulation.
	* Also, when the EM flag is set, execution of an MMX instruction causes an invalid-opcode exception (\#UD) to be
	* generated. Thus, if an IA-32 or Intel 64 processor incorporates MMX technology, the EM flag must be set to 0 to enable
	* execution of MMX instructions. Similarly for SSE/SSE2/SSE3/SSSE3/SSE4 extensions, when the EM flag is set, execution of
	* most SSE/SSE2/SSE3/SSSE3/SSE4 instructions causes an invalid opcode exception (\#UD) to be generated. If an IA-32 or
	* Intel 64 processor incorporates the SSE/SSE2/SSE3/SSSE3/SSE4 extensions, the EM flag must be set to 0 to enable
	* execution of these extensions. SSE/SSE2/SSE3/SSSE3/SSE4 instructions not affected by the EM flag include: PAUSE,
	* PREFETCHh, SFENCE, LFENCE, MFENCE, MOVNTI, CLFLUSH, CRC32, and POPCNT.
	*/
	uint64_t emulate_fpu                                             : 1;
#define CR0_EMULATE_FPU_BIT                                          2
#define CR0_EMULATE_FPU_FLAG                                         0x04
#define CR0_EMULATE_FPU_MASK                                         0x01
#define CR0_EMULATE_FPU(_)                                           (((_) >> 2) & 0x01)

	/**
	* @brief Task Switched
	*
	* [Bit 3] Allows the saving of the x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 context on a task switch to be delayed until an
	* x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instruction is actually executed by the new task. The processor sets this flag on
	* every task switch and tests it when executing x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instructions.
	* - If the TS flag is set and the EM flag (bit 2 of CR0) is clear, a device-not-available exception (\#NM) is raised prior
	* to the execution of any x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instruction; with the exception of PAUSE, PREFETCHh,
	* SFENCE, LFENCE, MFENCE, MOVNTI, CLFLUSH, CRC32, and POPCNT.
	* - If the TS flag is set and the MP flag (bit 1 of CR0) and EM flag are clear, an \#NM exception is not raised prior to
	* the execution of an x87 FPU WAIT/FWAIT instruction.
	* - If the EM flag is set, the setting of the TS flag has no effect on the execution of x87
	* FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instructions.
	*   The processor does not automatically save the context of the x87 FPU, XMM, and MXCSR registers on a task switch.
	*   Instead, it sets the TS flag, which causes the processor to raise an \#NM exception whenever it encounters an x87
	*   FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instruction in the instruction stream for the new task (with the exception of the
	*   instructions listed above).
	*   The fault handler for the \#NM exception can then be used to clear the TS flag (with the CLTS instruction) and save
	*   the context of the x87 FPU, XMM, and MXCSR registers. If the task never encounters an x87
	*   FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instruction, the x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 context is never saved.
	*/
	uint64_t task_switched                                           : 1;
#define CR0_TASK_SWITCHED_BIT                                        3
#define CR0_TASK_SWITCHED_FLAG                                       0x08
#define CR0_TASK_SWITCHED_MASK                                       0x01
#define CR0_TASK_SWITCHED(_)                                         (((_) >> 3) & 0x01)

	/**
	* @brief Extension Type
	*
	* [Bit 4] Reserved in the Pentium 4, Intel Xeon, P6 family, and Pentium processors. In the Pentium 4, Intel Xeon, and P6
	* family processors, this flag is hardcoded to 1. In the Intel386 and Intel486 processors, this flag indicates support of
	* Intel 387 DX math coprocessor instructions when set.
	*/
	uint64_t extension_type                                          : 1;
#define CR0_EXTENSION_TYPE_BIT                                       4
#define CR0_EXTENSION_TYPE_FLAG                                      0x10
#define CR0_EXTENSION_TYPE_MASK                                      0x01
#define CR0_EXTENSION_TYPE(_)                                        (((_) >> 4) & 0x01)

	/**
	* @brief Numeric Error
	*
	* [Bit 5] Enables the native (internal) mechanism for reporting x87 FPU errors when set; enables the PC-style x87 FPU
	* error reporting mechanism when clear. When the NE flag is clear and the IGNNE\# input is asserted, x87 FPU errors are
	* ignored. When the NE flag is clear and the IGNNE\# input is deasserted, an unmasked x87 FPU error causes the processor
	* to assert the FERR\# pin to generate an external interrupt and to stop instruction execution immediately before
	* executing the next waiting floating-point instruction or WAIT/FWAIT instruction.
	* The FERR\# pin is intended to drive an input to an external interrupt controller (the FERR\# pin emulates the ERROR\#
	* pin of the Intel 287 and Intel 387 DX math coprocessors). The NE flag, IGNNE\# pin, and FERR\# pin are used with
	* external logic to implement PC-style error reporting. Using FERR\# and IGNNE\# to handle floating-point exceptions is
	* deprecated by modern operating systems; this non-native approach also limits newer processors to operate with one
	* logical processor active.
	*
	* @see Vol1[8.7(Handling x87 FPU Exceptions in Software)]
	* @see Vol1[A.1(APPENDIX A | EFLAGS Cross-Reference)]
	*/
	uint64_t numeric_error                                           : 1;
#define CR0_NUMERIC_ERROR_BIT                                        5
#define CR0_NUMERIC_ERROR_FLAG                                       0x20
#define CR0_NUMERIC_ERROR_MASK                                       0x01
#define CR0_NUMERIC_ERROR(_)                                         (((_) >> 5) & 0x01)
	uint64_t reserved1                                               : 10;

	/**
	* @brief Write Protect
	*
	* [Bit 16] When set, inhibits supervisor-level procedures from writing into readonly pages; when clear, allows
	* supervisor-level procedures to write into read-only pages (regardless of the U/S bit setting). This flag facilitates
	* implementation of the copy-onwrite method of creating a new process (forking) used by operating systems such as UNIX.
	*
	* @see Vol3A[4.1.3(Paging-Mode Modifiers)]
	* @see Vol3A[4.6(ACCESS RIGHTS)]
	*/
	uint64_t write_protect                                           : 1;
#define CR0_WRITE_PROTECT_BIT                                        16
#define CR0_WRITE_PROTECT_FLAG                                       0x10000
#define CR0_WRITE_PROTECT_MASK                                       0x01
#define CR0_WRITE_PROTECT(_)                                         (((_) >> 16) & 0x01)
	uint64_t reserved2                                               : 1;

	/**
	* @brief Alignment Mask
	*
	* [Bit 18] Enables automatic alignment checking when set; disables alignment checking when clear. Alignment checking is
	* performed only when the AM flag is set, the AC flag in the EFLAGS register is set, CPL is 3, and the processor is
	* operating in either protected or virtual-8086 mode.
	*/
	uint64_t alignment_mask                                          : 1;
#define CR0_ALIGNMENT_MASK_BIT                                       18
#define CR0_ALIGNMENT_MASK_FLAG                                      0x40000
#define CR0_ALIGNMENT_MASK_MASK                                      0x01
#define CR0_ALIGNMENT_MASK(_)                                        (((_) >> 18) & 0x01)
	uint64_t reserved3                                               : 10;

	/**
	* @brief Not Write-through
	*
	* [Bit 29] When the NW and CD flags are clear, write-back (for Pentium 4, Intel Xeon, P6 family, and Pentium processors)
	* or write-through (for Intel486 processors) is enabled for writes that hit the cache and invalidation cycles are enabled.
	*/
	uint64_t not_write_through                                       : 1;
#define CR0_NOT_WRITE_THROUGH_BIT                                    29
#define CR0_NOT_WRITE_THROUGH_FLAG                                   0x20000000
#define CR0_NOT_WRITE_THROUGH_MASK                                   0x01
#define CR0_NOT_WRITE_THROUGH(_)                                     (((_) >> 29) & 0x01)

	/**
	* @brief Cache Disable
	*
	* [Bit 30] When the CD and NW flags are clear, caching of memory locations for the whole of physical memory in the
	* processor's internal (and external) caches is enabled. When the CD flag is set, caching is restricted. To prevent the
	* processor from accessing and updating its caches, the CD flag must be set and the caches must be invalidated so that no
	* cache hits can occur.
	*
	* @see Vol3A[11.5.3(Preventing Caching)]
	* @see Vol3A[11.5(CACHE CONTROL)]
	*/
	uint64_t cache_disable                                           : 1;
#define CR0_CACHE_DISABLE_BIT                                        30
#define CR0_CACHE_DISABLE_FLAG                                       0x40000000
#define CR0_CACHE_DISABLE_MASK                                       0x01
#define CR0_CACHE_DISABLE(_)                                         (((_) >> 30) & 0x01)

	/**
	* @brief Paging Enable
	*
	* [Bit 31] Enables paging when set; disables paging when clear. When paging is disabled, all linear addresses are treated
	* as physical addresses. The PG flag has no effect if the PE flag (bit 0 of register CR0) is not also set; setting the PG
	* flag when the PE flag is clear causes a general-protection exception (\#GP).
	* On Intel 64 processors, enabling and disabling IA-32e mode operation also requires modifying CR0.PG.
	*
	* @see Vol3A[4(PAGING)]
	*/
	uint64_t paging_enable                                           : 1;
#define CR0_PAGING_ENABLE_BIT                                        31
#define CR0_PAGING_ENABLE_FLAG                                       0x80000000
#define CR0_PAGING_ENABLE_MASK                                       0x01
#define CR0_PAGING_ENABLE(_)                                         (((_) >> 31) & 0x01)
	uint64_t reserved4                                               : 32;
  };

  uint64_t flags;
} cr0;

typedef union
{
  struct
  {
		uint64_t pcid         : 12;
		uint64_t padding      : 51;
		uint64_t preserve_tlb : 1;
  };
  struct
  {
	uint64_t reserved1                                               : 3;

	/**
	* @brief Page-level Write-Through
	*
	* [Bit 3] Controls the memory type used to access the first paging structure of the current paging-structure hierarchy.
	* This bit is not used if paging is disabled, with PAE paging, or with 4-level paging if CR4.PCIDE=1.
	*
	* @see Vol3A[4.9(PAGING AND MEMORY TYPING)]
	*/
	uint64_t page_level_write_through                                : 1;
#define CR3_PAGE_LEVEL_WRITE_THROUGH_BIT                             3
#define CR3_PAGE_LEVEL_WRITE_THROUGH_FLAG                            0x08
#define CR3_PAGE_LEVEL_WRITE_THROUGH_MASK                            0x01
#define CR3_PAGE_LEVEL_WRITE_THROUGH(_)                              (((_) >> 3) & 0x01)

	/**
	* @brief Page-level Cache Disable
	*
	* [Bit 4] Controls the memory type used to access the first paging structure of the current paging-structure hierarchy.
	* This bit is not used if paging is disabled, with PAE paging, or with 4-level paging2 if CR4.PCIDE=1.
	*
	* @see Vol3A[4.9(PAGING AND MEMORY TYPING)]
	*/
	uint64_t page_level_cache_disable                                : 1;
#define CR3_PAGE_LEVEL_CACHE_DISABLE_BIT                             4
#define CR3_PAGE_LEVEL_CACHE_DISABLE_FLAG                            0x10
#define CR3_PAGE_LEVEL_CACHE_DISABLE_MASK                            0x01
#define CR3_PAGE_LEVEL_CACHE_DISABLE(_)                              (((_) >> 4) & 0x01)
	uint64_t reserved2                                               : 7;

	/**
	* @brief Address of page directory
	*
	* [Bits 47:12] Physical address of the 4-KByte aligned page directory (32-bit paging) or PML4 table (64-bit paging) used
	* for linear-address translation.
	*
	* @see Vol3A[4.3(32-BIT PAGING)]
	* @see Vol3A[4.5(4-LEVEL PAGING)]
	*/
	uint64_t address_of_page_directory                               : 36;
#define CR3_ADDRESS_OF_PAGE_DIRECTORY_BIT                            12
#define CR3_ADDRESS_OF_PAGE_DIRECTORY_FLAG                           0xFFFFFFFFF000
#define CR3_ADDRESS_OF_PAGE_DIRECTORY_MASK                           0xFFFFFFFFF
#define CR3_ADDRESS_OF_PAGE_DIRECTORY(_)                             (((_) >> 12) & 0xFFFFFFFFF)
	uint64_t reserved3                                               : 16;
  };

  uint64_t flags;
} cr3;

typedef union
{
  struct
  {
	/**
	* @brief Virtual-8086 Mode Extensions
	*
	* [Bit 0] Enables interrupt- and exception-handling extensions in virtual-8086 mode when set; disables the extensions when
	* clear. Use of the virtual mode extensions can improve the performance of virtual-8086 applications by eliminating the
	* overhead of calling the virtual- 8086 monitor to handle interrupts and exceptions that occur while executing an 8086
	* program and, instead, redirecting the interrupts and exceptions back to the 8086 program's handlers. It also provides
	* hardware support for a virtual interrupt flag (VIF) to improve reliability of running 8086 programs in multitasking and
	* multiple-processor environments.
	*
	* @see Vol3B[20.3(INTERRUPT AND EXCEPTION HANDLING IN VIRTUAL-8086 MODE)]
	*/
	uint64_t virtual_mode_extensions                                 : 1;
#define CR4_VIRTUAL_MODE_EXTENSIONS_BIT                              0
#define CR4_VIRTUAL_MODE_EXTENSIONS_FLAG                             0x01
#define CR4_VIRTUAL_MODE_EXTENSIONS_MASK                             0x01
#define CR4_VIRTUAL_MODE_EXTENSIONS(_)                               (((_) >> 0) & 0x01)

	/**
	* @brief Protected-Mode Virtual Interrupts
	*
	* [Bit 1] Enables hardware support for a virtual interrupt flag (VIF) in protected mode when set; disables the VIF flag in
	* protected mode when clear.
	*
	* @see Vol3B[20.4(PROTECTED-MODE VIRTUAL INTERRUPTS)]
	*/
	uint64_t protected_mode_virtual_interrupts                       : 1;
#define CR4_PROTECTED_MODE_VIRTUAL_INTERRUPTS_BIT                    1
#define CR4_PROTECTED_MODE_VIRTUAL_INTERRUPTS_FLAG                   0x02
#define CR4_PROTECTED_MODE_VIRTUAL_INTERRUPTS_MASK                   0x01
#define CR4_PROTECTED_MODE_VIRTUAL_INTERRUPTS(_)                     (((_) >> 1) & 0x01)

	/**
	* @brief Time Stamp Disable
	*
	* [Bit 2] Restricts the execution of the RDTSC instruction to procedures running at privilege level 0 when set; allows
	* RDTSC instruction to be executed at any privilege level when clear. This bit also applies to the RDTSCP instruction if
	* supported (if CPUID.80000001H:EDX[27] = 1).
	*/
	uint64_t timestamp_disable                                       : 1;
#define CR4_TIMESTAMP_DISABLE_BIT                                    2
#define CR4_TIMESTAMP_DISABLE_FLAG                                   0x04
#define CR4_TIMESTAMP_DISABLE_MASK                                   0x01
#define CR4_TIMESTAMP_DISABLE(_)                                     (((_) >> 2) & 0x01)

	/**
	* @brief Debugging Extensions
	*
	* [Bit 3] References to debug registers DR4 and DR5 cause an undefined opcode (\#UD) exception to be generated when set;
	* when clear, processor aliases references to registers DR4 and DR5 for compatibility with software written to run on
	* earlier IA-32 processors.
	*
	* @see Vol3B[17.2.2(Debug Registers DR4 and DR5)]
	*/
	uint64_t debugging_extensions                                    : 1;
#define CR4_DEBUGGING_EXTENSIONS_BIT                                 3
#define CR4_DEBUGGING_EXTENSIONS_FLAG                                0x08
#define CR4_DEBUGGING_EXTENSIONS_MASK                                0x01
#define CR4_DEBUGGING_EXTENSIONS(_)                                  (((_) >> 3) & 0x01)

	/**
	* @brief Page Size Extensions
	*
	* [Bit 4] Enables 4-MByte pages with 32-bit paging when set; restricts 32-bit paging to pages of 4 KBytes when clear.
	*
	* @see Vol3A[4.3(32-BIT PAGING)]
	*/
	uint64_t page_size_extensions                                    : 1;
#define CR4_PAGE_SIZE_EXTENSIONS_BIT                                 4
#define CR4_PAGE_SIZE_EXTENSIONS_FLAG                                0x10
#define CR4_PAGE_SIZE_EXTENSIONS_MASK                                0x01
#define CR4_PAGE_SIZE_EXTENSIONS(_)                                  (((_) >> 4) & 0x01)

	/**
	* @brief Physical Address Extension
	*
	* [Bit 5] When set, enables paging to produce physical addresses with more than 32 bits. When clear, restricts physical
	* addresses to 32 bits. PAE must be set before entering IA-32e mode.
	*
	* @see Vol3A[4(PAGING)]
	*/
	uint64_t physical_address_extension                              : 1;
#define CR4_PHYSICAL_ADDRESS_EXTENSION_BIT                           5
#define CR4_PHYSICAL_ADDRESS_EXTENSION_FLAG                          0x20
#define CR4_PHYSICAL_ADDRESS_EXTENSION_MASK                          0x01
#define CR4_PHYSICAL_ADDRESS_EXTENSION(_)                            (((_) >> 5) & 0x01)

	/**
	* @brief Machine-Check Enable
	*
	* [Bit 6] Enables the machine-check exception when set; disables the machine-check exception when clear.
	*
	* @see Vol3B[15(MACHINE-CHECK ARCHITECTURE)]
	*/
	uint64_t machine_check_enable                                    : 1;
#define CR4_MACHINE_CHECK_ENABLE_BIT                                 6
#define CR4_MACHINE_CHECK_ENABLE_FLAG                                0x40
#define CR4_MACHINE_CHECK_ENABLE_MASK                                0x01
#define CR4_MACHINE_CHECK_ENABLE(_)                                  (((_) >> 6) & 0x01)

	/**
	* @brief Page Global Enable
	*
	* [Bit 7] (Introduced in the P6 family processors.) Enables the global page feature when set; disables the global page
	* feature when clear. The global page feature allows frequently used or shared pages to be marked as global to all users
	* (done with the global flag, bit 8, in a page-directory or page-table entry). Global pages are not flushed from the
	* translation-lookaside buffer (TLB) on a task switch or a write to register CR3. When enabling the global page feature,
	* paging must be enabled (by setting the PG flag in control register CR0) before the PGE flag is set. Reversing this
	* sequence may affect program correctness, and processor performance will be impacted.
	*
	* @see Vol3A[4.10(CACHING TRANSLATION INFORMATION)]
	*/
	uint64_t page_global_enable                                      : 1;
#define CR4_PAGE_GLOBAL_ENABLE_BIT                                   7
#define CR4_PAGE_GLOBAL_ENABLE_FLAG                                  0x80
#define CR4_PAGE_GLOBAL_ENABLE_MASK                                  0x01
#define CR4_PAGE_GLOBAL_ENABLE(_)                                    (((_) >> 7) & 0x01)

	/**
	* @brief Performance-Monitoring Counter Enable
	*
	* [Bit 8] Enables execution of the RDPMC instruction for programs or procedures running at any protection level when set;
	* RDPMC instruction can be executed only at protection level 0 when clear.
	*/
	uint64_t performance_monitoring_counter_enable                   : 1;
#define CR4_PERFORMANCE_MONITORING_COUNTER_ENABLE_BIT                8
#define CR4_PERFORMANCE_MONITORING_COUNTER_ENABLE_FLAG               0x100
#define CR4_PERFORMANCE_MONITORING_COUNTER_ENABLE_MASK               0x01
#define CR4_PERFORMANCE_MONITORING_COUNTER_ENABLE(_)                 (((_) >> 8) & 0x01)

	/**
	* @brief Operating System Support for FXSAVE and FXRSTOR instructions
	*
	* [Bit 9] When set, this flag:
	* -# indicates to software that the operating system supports the use of the FXSAVE and FXRSTOR instructions,
	* -# enables the FXSAVE and FXRSTOR instructions to save and restore the contents of the XMM and MXCSR registers along
	* with the contents of the x87 FPU and MMX registers, and
	* -# enables the processor to execute SSE/SSE2/SSE3/SSSE3/SSE4 instructions, with the exception of the PAUSE, PREFETCHh,
	* SFENCE, LFENCE, MFENCE, MOVNTI, CLFLUSH, CRC32, and POPCNT.
	* If this flag is clear, the FXSAVE and FXRSTOR instructions will save and restore the contents of the x87 FPU and MMX
	* registers, but they may not save and restore the contents of the XMM and MXCSR registers. Also, the processor will
	* generate an invalid opcode exception (\#UD) if it attempts to execute any SSE/SSE2/SSE3 instruction, with the exception
	* of PAUSE, PREFETCHh, SFENCE, LFENCE, MFENCE, MOVNTI, CLFLUSH, CRC32, and POPCNT. The operating system or executive must
	* explicitly set this flag.
	*
	* @remarks CPUID feature flag FXSR indicates availability of the FXSAVE/FXRSTOR instructions. The OSFXSR bit provides
	*          operating system software with a means of enabling FXSAVE/FXRSTOR to save/restore the contents of the X87 FPU, XMM and
	*          MXCSR registers. Consequently OSFXSR bit indicates that the operating system provides context switch support for
	*          SSE/SSE2/SSE3/SSSE3/SSE4.
	*/
	uint64_t os_fxsave_fxrstor_support                               : 1;
#define CR4_OS_FXSAVE_FXRSTOR_SUPPORT_BIT                            9
#define CR4_OS_FXSAVE_FXRSTOR_SUPPORT_FLAG                           0x200
#define CR4_OS_FXSAVE_FXRSTOR_SUPPORT_MASK                           0x01
#define CR4_OS_FXSAVE_FXRSTOR_SUPPORT(_)                             (((_) >> 9) & 0x01)

	/**
	* @brief Operating System Support for Unmasked SIMD Floating-Point Exceptions
	*
	* [Bit 10] Operating System Support for Unmasked SIMD Floating-Point Exceptions - When set, indicates that the operating
	* system supports the handling of unmasked SIMD floating-point exceptions through an exception handler that is invoked
	* when a SIMD floating-point exception (\#XM) is generated. SIMD floating-point exceptions are only generated by
	* SSE/SSE2/SSE3/SSE4.1 SIMD floatingpoint instructions.
	* The operating system or executive must explicitly set this flag. If this flag is not set, the processor will generate an
	* invalid opcode exception (\#UD) whenever it detects an unmasked SIMD floating-point exception.
	*/
	uint64_t os_xmm_exception_support                                : 1;
#define CR4_OS_XMM_EXCEPTION_SUPPORT_BIT                             10
#define CR4_OS_XMM_EXCEPTION_SUPPORT_FLAG                            0x400
#define CR4_OS_XMM_EXCEPTION_SUPPORT_MASK                            0x01
#define CR4_OS_XMM_EXCEPTION_SUPPORT(_)                              (((_) >> 10) & 0x01)

	/**
	* @brief User-Mode Instruction Prevention
	*
	* [Bit 11] When set, the following instructions cannot be executed if CPL > 0: SGDT, SIDT, SLDT, SMSW, and STR. An attempt
	* at such execution causes a generalprotection exception (\#GP).
	*/
	uint64_t usermode_instruction_prevention                         : 1;
#define CR4_USERMODE_INSTRUCTION_PREVENTION_BIT                      11
#define CR4_USERMODE_INSTRUCTION_PREVENTION_FLAG                     0x800
#define CR4_USERMODE_INSTRUCTION_PREVENTION_MASK                     0x01
#define CR4_USERMODE_INSTRUCTION_PREVENTION(_)                       (((_) >> 11) & 0x01)
	
	/**
	* @brief 5-level paging.
	*
	* [Bit 12]
	*/
	uint64_t la57_enable                                             : 1;
#define CR4_LA57_ENABLE_BIT                                          12
#define CR4_LA57_ENABLE_FLAG                                         0x1000
#define CR4_LA57_ENABLE_MASK                                         0x01
#define CR4_LA57_ENABLE(_)                                           (((_) >> 12) & 0x01)

	/**
	* @brief VMX-Enable
	*
	* [Bit 13] Enables VMX operation when set.
	*
	* @see Vol3C[23(INTRODUCTION TO VIRTUAL MACHINE EXTENSIONS)]
	*/
	uint64_t vmx_enable                                              : 1;
#define CR4_VMX_ENABLE_BIT                                           13
#define CR4_VMX_ENABLE_FLAG                                          0x2000
#define CR4_VMX_ENABLE_MASK                                          0x01
#define CR4_VMX_ENABLE(_)                                            (((_) >> 13) & 0x01)

	/**
	* @brief SMX-Enable
	*
	* [Bit 14] Enables SMX operation when set.
	*
	* @see Vol2[6(SAFER MODE EXTENSIONS REFERENCE)]
	*/
	uint64_t smx_enable                                              : 1;
#define CR4_SMX_ENABLE_BIT                                           14
#define CR4_SMX_ENABLE_FLAG                                          0x4000
#define CR4_SMX_ENABLE_MASK                                          0x01
#define CR4_SMX_ENABLE(_)                                            (((_) >> 14) & 0x01)
	uint64_t reserved2                                               : 1;

	/**
	* @brief FSGSBASE-Enable
	*
	* [Bit 16] Enables the instructions RDFSBASE, RDGSBASE, WRFSBASE, and WRGSBASE.
	*/
	uint64_t fsgsbase_enable                                         : 1;
#define CR4_FSGSBASE_ENABLE_BIT                                      16
#define CR4_FSGSBASE_ENABLE_FLAG                                     0x10000
#define CR4_FSGSBASE_ENABLE_MASK                                     0x01
#define CR4_FSGSBASE_ENABLE(_)                                       (((_) >> 16) & 0x01)

	/**
	* @brief PCID-Enable
	*
	* [Bit 17] Enables process-context identifiers (PCIDs) when set. Can be set only in IA-32e mode (if IA32_EFER.LMA = 1).
	*
	* @see Vol3A[4.10.1(Process-Context Identifiers (PCIDs))]
	*/
	uint64_t pcid_enable                                             : 1;
#define CR4_PCID_ENABLE_BIT                                          17
#define CR4_PCID_ENABLE_FLAG                                         0x20000
#define CR4_PCID_ENABLE_MASK                                         0x01
#define CR4_PCID_ENABLE(_)                                           (((_) >> 17) & 0x01)

	/**
	* @brief XSAVE and Processor Extended States-Enable
	*
	* [Bit 18] When set, this flag:
	* -# indicates (via CPUID.01H:ECX.OSXSAVE[bit 27]) that the operating system supports the use of the XGETBV, XSAVE and
	* XRSTOR instructions by general software;
	* -# enables the XSAVE and XRSTOR instructions to save and restore the x87 FPU state (including MMX registers), the SSE
	* state (XMM registers and MXCSR), along with other processor extended states enabled in XCR0;
	* -# enables the processor to execute XGETBV and XSETBV instructions in order to read and write XCR0.
	*
	* @see Vol3A[2.6(EXTENDED CONTROL REGISTERS (INCLUDING XCR0))]
	* @see Vol3A[13(SYSTEM PROGRAMMING FOR INSTRUCTION SET EXTENSIONS AND PROCESSOR EXTENDED)]
	*/
	uint64_t os_xsave                                                : 1;
#define CR4_OS_XSAVE_BIT                                             18
#define CR4_OS_XSAVE_FLAG                                            0x40000
#define CR4_OS_XSAVE_MASK                                            0x01
#define CR4_OS_XSAVE(_)                                              (((_) >> 18) & 0x01)
	uint64_t reserved3                                               : 1;

	/**
	* @brief SMEP-Enable
	*
	* [Bit 20] Enables supervisor-mode execution prevention (SMEP) when set.
	*
	* @see Vol3A[4.6(ACCESS RIGHTS)]
	*/
	uint64_t smep_enable                                             : 1;
#define CR4_SMEP_ENABLE_BIT                                          20
#define CR4_SMEP_ENABLE_FLAG                                         0x100000
#define CR4_SMEP_ENABLE_MASK                                         0x01
#define CR4_SMEP_ENABLE(_)                                           (((_) >> 20) & 0x01)

	/**
	* @brief SMAP-Enable
	*
	* [Bit 21] Enables supervisor-mode access prevention (SMAP) when set.
	*
	* @see Vol3A[4.6(ACCESS RIGHTS)]
	*/
	uint64_t smap_enable                                             : 1;
#define CR4_SMAP_ENABLE_BIT                                          21
#define CR4_SMAP_ENABLE_FLAG                                         0x200000
#define CR4_SMAP_ENABLE_MASK                                         0x01
#define CR4_SMAP_ENABLE(_)                                           (((_) >> 21) & 0x01)

	/**
	* @brief Protection-Key-Enable
	*
	* [Bit 22] Enables 4-level paging to associate each linear address with a protection key. The PKRU register specifies, for
	* each protection key, whether user-mode linear addresses with that protection key can be read or written. This bit also
	* enables access to the PKRU register using the RDPKRU and WRPKRU instructions.
	*/
	uint64_t protection_key_enable                                   : 1;
#define CR4_PROTECTION_KEY_ENABLE_BIT                                22
#define CR4_PROTECTION_KEY_ENABLE_FLAG                               0x400000
#define CR4_PROTECTION_KEY_ENABLE_MASK                               0x01
#define CR4_PROTECTION_KEY_ENABLE(_)                                 (((_) >> 22) & 0x01)
	uint64_t reserved4                                               : 41;
  };

  uint64_t flags;
} cr4;

typedef union
{
  struct
  {
	/**
	* @brief Task Priority Level
	*
	* [Bits 3:0] This sets the threshold value corresponding to the highestpriority interrupt to be blocked. A value of 0
	* means all interrupts are enabled. This field is available in 64- bit mode. A value of 15 means all interrupts will be
	* disabled.
	*/
	uint64_t task_priority_level                                     : 4;
#define CR8_TASK_PRIORITY_LEVEL_BIT                                  0
#define CR8_TASK_PRIORITY_LEVEL_FLAG                                 0x0F
#define CR8_TASK_PRIORITY_LEVEL_MASK                                 0x0F
#define CR8_TASK_PRIORITY_LEVEL(_)                                   (((_) >> 0) & 0x0F)

	/**
	* @brief Reserved
	*
	* [Bits 63:4] Reserved and must be written with zeros. Failure to do this causes a general-protection exception.
	*/
	uint64_t reserved                                                : 60;
#define CR8_RESERVED_BIT                                             4
#define CR8_RESERVED_FLAG                                            0xFFFFFFFFFFFFFFF0
#define CR8_RESERVED_MASK                                            0xFFFFFFFFFFFFFFF
#define CR8_RESERVED(_)                                              (((_) >> 4) & 0xFFFFFFFFFFFFFFF)
  };

  uint64_t flags;
} cr8;

/**
 * @}
 */

/**
 * @defgroup debug_registers \
 *           Debug registers
 *
 * Eight debug registers control the debug operation of the processor. These registers can be written to and read using the
 * move to/from debug register form of the MOV instruction. A debug register may be the source or destination operand for
 * one of these instructions.
 * Debug registers are privileged resources; a MOV instruction that accesses these registers can only be executed in
 * real-address mode, in SMM or in protected mode at a CPL of 0. An attempt to read or write the debug registers from any
 * other privilege level generates a general-protection exception (\#GP). The primary function of the debug registers is to
 * set up and monitor from 1 to 4 breakpoints, numbered 0 though 3. For each breakpoint, the following information can be
 * specified:
 * - The linear address where the breakpoint is to occur.
 * - The length of the breakpoint location: 1, 2, 4, or 8 bytes.
 * - The operation that must be performed at the address for a debug exception to be generated.
 * - Whether the breakpoint is enabled.
 * - Whether the breakpoint condition was present when the debug exception was generated.
 *
 * @see Vol3B[17.2.4(Debug Control Register (DR7))]
 * @see Vol3B[17.2(DEBUG REGISTERS)] (reference)
 * @{
 */
typedef union
{
  struct
  {
	/**
	* @brief B0 through B3 (breakpoint condition detected) flags
	*
	* [Bits 3:0] Indicates (when set) that its associated breakpoint condition was met when a debug exception was generated.
	* These flags are set if the condition described for each breakpoint by the LENn, and R/Wn flags in debug control register
	* DR7 is true. They may or may not be set if the breakpoint is not enabled by the Ln or the Gn flags in register DR7.
	* Therefore on a \#DB, a debug handler should check only those B0-B3 bits which correspond to an enabled breakpoint.
	*/
	uint64_t breakpoint_condition                                    : 4;
#define DR6_BREAKPOINT_CONDITION_BIT                                 0
#define DR6_BREAKPOINT_CONDITION_FLAG                                0x0F
#define DR6_BREAKPOINT_CONDITION_MASK                                0x0F
#define DR6_BREAKPOINT_CONDITION(_)                                  (((_) >> 0) & 0x0F)
	uint64_t reserved1                                               : 9;

	/**
	* @brief BD (debug register access detected) flag
	*
	* [Bit 13] Indicates that the next instruction in the instruction stream accesses one of the debug registers (DR0 through
	* DR7). This flag is enabled when the GD (general detect) flag in debug control register DR7 is set.
	*
	* @see Vol3B[17.2.4(Debug Control Register (DR7))]
	*/
	uint64_t debug_register_access_detected                          : 1;
#define DR6_DEBUG_REGISTER_ACCESS_DETECTED_BIT                       13
#define DR6_DEBUG_REGISTER_ACCESS_DETECTED_FLAG                      0x2000
#define DR6_DEBUG_REGISTER_ACCESS_DETECTED_MASK                      0x01
#define DR6_DEBUG_REGISTER_ACCESS_DETECTED(_)                        (((_) >> 13) & 0x01)

	/**
	* @brief BS (single step) flag
	*
	* [Bit 14] Indicates (when set) that the debug exception was triggered by the singlestep execution mode (enabled with the
	* TF flag in the EFLAGS register). The single-step mode is the highestpriority debug exception. When the BS flag is set,
	* any of the other debug status bits also may be set.
	*/
	uint64_t single_instruction                                      : 1;
#define DR6_SINGLE_INSTRUCTION_BIT                                   14
#define DR6_SINGLE_INSTRUCTION_FLAG                                  0x4000
#define DR6_SINGLE_INSTRUCTION_MASK                                  0x01
#define DR6_SINGLE_INSTRUCTION(_)                                    (((_) >> 14) & 0x01)

	/**
	* @brief BT (task switch) flag
	*
	* [Bit 15] Indicates (when set) that the debug exception was triggered by the singlestep execution mode (enabled with the
	* TF flag in the EFLAGS register). The single-step mode is the highestpriority debug exception. When the BS flag is set,
	* any of the other debug status bits also may be set.
	*/
	uint64_t task_switch                                             : 1;
#define DR6_TASK_SWITCH_BIT                                          15
#define DR6_TASK_SWITCH_FLAG                                         0x8000
#define DR6_TASK_SWITCH_MASK                                         0x01
#define DR6_TASK_SWITCH(_)                                           (((_) >> 15) & 0x01)

	/**
	* @brief RTM (restricted transactional memory) flag
	*
	* [Bit 16] Indicates (when clear) that a debug exception (\#DB) or breakpoint exception (\#BP) occurred inside an RTM
	* region while advanced debugging of RTM transactional regions was enabled. This bit is set for any other debug exception
	* (including all those that occur when advanced debugging of RTM transactional regions is not enabled). This bit is always
	* 1 if the processor does not support RTM.
	*
	* @see Vol3B[17.3.3(Debug Exceptions, Breakpoint Exceptions, and Restricted Transactional Memory (RTM))]
	*/
	uint64_t restricted_transactional_memory                         : 1;
#define DR6_RESTRICTED_TRANSACTIONAL_MEMORY_BIT                      16
#define DR6_RESTRICTED_TRANSACTIONAL_MEMORY_FLAG                     0x10000
#define DR6_RESTRICTED_TRANSACTIONAL_MEMORY_MASK                     0x01
#define DR6_RESTRICTED_TRANSACTIONAL_MEMORY(_)                       (((_) >> 16) & 0x01)
	uint64_t reserved2                                               : 47;
  };

  uint64_t flags;
} dr6;

typedef union
{
  struct
  {
	/**
	* @brief L0 through L3 (local breakpoint enable) flags (bits 0, 2, 4, and 6)
	*
	* [Bit 0] Enables (when set) the breakpoint condition for the associated breakpoint for the current task. When a
	* breakpoint condition is detected and its associated Ln flag is set, a debug exception is generated. The processor
	* automatically clears these flags on every task switch to avoid unwanted breakpoint conditions in the new task.
	*/
	uint64_t local_breakpoint_0                                      : 1;
#define DR7_LOCAL_BREAKPOINT_0_BIT                                   0
#define DR7_LOCAL_BREAKPOINT_0_FLAG                                  0x01
#define DR7_LOCAL_BREAKPOINT_0_MASK                                  0x01
#define DR7_LOCAL_BREAKPOINT_0(_)                                    (((_) >> 0) & 0x01)

	/**
	* @brief G0 through G3 (global breakpoint enable) flags (bits 1, 3, 5, and 7)
	*
	* [Bit 1] Enables (when set) the breakpoint condition for the associated breakpoint for all tasks. When a breakpoint
	* condition is detected and its associated Gn flag is set, a debug exception is generated. The processor does not clear
	* these flags on a task switch, allowing a breakpoint to be enabled for all tasks.
	*/
	uint64_t global_breakpoint_0                                     : 1;
#define DR7_GLOBAL_BREAKPOINT_0_BIT                                  1
#define DR7_GLOBAL_BREAKPOINT_0_FLAG                                 0x02
#define DR7_GLOBAL_BREAKPOINT_0_MASK                                 0x01
#define DR7_GLOBAL_BREAKPOINT_0(_)                                   (((_) >> 1) & 0x01)
	uint64_t local_breakpoint_1                                      : 1;
#define DR7_LOCAL_BREAKPOINT_1_BIT                                   2
#define DR7_LOCAL_BREAKPOINT_1_FLAG                                  0x04
#define DR7_LOCAL_BREAKPOINT_1_MASK                                  0x01
#define DR7_LOCAL_BREAKPOINT_1(_)                                    (((_) >> 2) & 0x01)
	uint64_t global_breakpoint_1                                     : 1;
#define DR7_GLOBAL_BREAKPOINT_1_BIT                                  3
#define DR7_GLOBAL_BREAKPOINT_1_FLAG                                 0x08
#define DR7_GLOBAL_BREAKPOINT_1_MASK                                 0x01
#define DR7_GLOBAL_BREAKPOINT_1(_)                                   (((_) >> 3) & 0x01)
	uint64_t local_breakpoint_2                                      : 1;
#define DR7_LOCAL_BREAKPOINT_2_BIT                                   4
#define DR7_LOCAL_BREAKPOINT_2_FLAG                                  0x10
#define DR7_LOCAL_BREAKPOINT_2_MASK                                  0x01
#define DR7_LOCAL_BREAKPOINT_2(_)                                    (((_) >> 4) & 0x01)
	uint64_t global_breakpoint_2                                     : 1;
#define DR7_GLOBAL_BREAKPOINT_2_BIT                                  5
#define DR7_GLOBAL_BREAKPOINT_2_FLAG                                 0x20
#define DR7_GLOBAL_BREAKPOINT_2_MASK                                 0x01
#define DR7_GLOBAL_BREAKPOINT_2(_)                                   (((_) >> 5) & 0x01)
	uint64_t local_breakpoint_3                                      : 1;
#define DR7_LOCAL_BREAKPOINT_3_BIT                                   6
#define DR7_LOCAL_BREAKPOINT_3_FLAG                                  0x40
#define DR7_LOCAL_BREAKPOINT_3_MASK                                  0x01
#define DR7_LOCAL_BREAKPOINT_3(_)                                    (((_) >> 6) & 0x01)
	uint64_t global_breakpoint_3                                     : 1;
#define DR7_GLOBAL_BREAKPOINT_3_BIT                                  7
#define DR7_GLOBAL_BREAKPOINT_3_FLAG                                 0x80
#define DR7_GLOBAL_BREAKPOINT_3_MASK                                 0x01
#define DR7_GLOBAL_BREAKPOINT_3(_)                                   (((_) >> 7) & 0x01)

	/**
	* @brief LE (local exact breakpoint enable)
	*
	* [Bit 8] This feature is not supported in the P6 family processors, later IA-32 processors, and Intel 64 processors. When
	* set, these flags cause the processor to detect the exact instruction that caused a data breakpoint condition. For
	* backward and forward compatibility with other Intel processors, we recommend that the LE and GE flags be set to 1 if
	* exact breakpoints are required.
	*/
	uint64_t local_exact_breakpoint                                  : 1;
#define DR7_LOCAL_EXACT_BREAKPOINT_BIT                               8
#define DR7_LOCAL_EXACT_BREAKPOINT_FLAG                              0x100
#define DR7_LOCAL_EXACT_BREAKPOINT_MASK                              0x01
#define DR7_LOCAL_EXACT_BREAKPOINT(_)                                (((_) >> 8) & 0x01)
	uint64_t global_exact_breakpoint                                 : 1;
#define DR7_GLOBAL_EXACT_BREAKPOINT_BIT                              9
#define DR7_GLOBAL_EXACT_BREAKPOINT_FLAG                             0x200
#define DR7_GLOBAL_EXACT_BREAKPOINT_MASK                             0x01
#define DR7_GLOBAL_EXACT_BREAKPOINT(_)                               (((_) >> 9) & 0x01)
	uint64_t reserved1                                               : 1;

	/**
	* @brief RTM (restricted transactional memory) flag
	*
	* [Bit 11] Enables (when set) advanced debugging of RTM transactional regions. This advanced debugging is enabled only if
	* IA32_DEBUGCTL.RTM is also set.
	*
	* @see Vol3B[17.3.3(Debug Exceptions, Breakpoint Exceptions, and Restricted Transactional Memory (RTM))]
	*/
	uint64_t restricted_transactional_memory                         : 1;
#define DR7_RESTRICTED_TRANSACTIONAL_MEMORY_BIT                      11
#define DR7_RESTRICTED_TRANSACTIONAL_MEMORY_FLAG                     0x800
#define DR7_RESTRICTED_TRANSACTIONAL_MEMORY_MASK                     0x01
#define DR7_RESTRICTED_TRANSACTIONAL_MEMORY(_)                       (((_) >> 11) & 0x01)
	uint64_t reserved2                                               : 1;

	/**
	* @brief GD (general detect enable) flag
	*
	* [Bit 13] Enables (when set) debug-register protection, which causes a debug exception to be generated prior to any MOV
	* instruction that accesses a debug register. When such a condition is detected, the BD flag in debug status register DR6
	* is set prior to generating the exception. This condition is provided to support in-circuit emulators.
	* When the emulator needs to access the debug registers, emulator software can set the GD flag to prevent interference
	* from the program currently executing on the processor.
	* The processor clears the GD flag upon entering to the debug exception handler, to allow the handler access to the debug
	* registers.
	*/
	uint64_t general_detect                                          : 1;
#define DR7_GENERAL_DETECT_BIT                                       13
#define DR7_GENERAL_DETECT_FLAG                                      0x2000
#define DR7_GENERAL_DETECT_MASK                                      0x01
#define DR7_GENERAL_DETECT(_)                                        (((_) >> 13) & 0x01)
	uint64_t reserved3                                               : 2;

	/**
	* @brief R/W0 through R/W3 (read/write) fields (bits 16, 17, 20, 21, 24, 25, 28, and 29)
	*
	* [Bits 17:16] Specifies the breakpoint condition for the corresponding breakpoint. The DE (debug extensions) flag in
	* control register CR4 determines how the bits in the R/Wn fields are interpreted. When the DE flag is set, the processor
	* interprets bits as follows:
	* - 00 - Break on instruction execution only.
	* - 01 - Break on data writes only.
	* - 10 - Break on I/O reads or writes.
	* - 11 - Break on data reads or writes but not instruction fetches.
	* When the DE flag is clear, the processor interprets the R/Wn bits the same as for the Intel386(TM) and Intel486(TM)
	* processors, which is as follows:
	* - 00 - Break on instruction execution only.
	* - 01 - Break on data writes only.
	* - 10 - Undefined.
	* - 11 - Break on data reads or writes but not instruction fetches.
	*/
	uint64_t read_write_0                                            : 2;
#define DR7_READ_WRITE_0_BIT                                         16
#define DR7_READ_WRITE_0_FLAG                                        0x30000
#define DR7_READ_WRITE_0_MASK                                        0x03
#define DR7_READ_WRITE_0(_)                                          (((_) >> 16) & 0x03)

	/**
	* @brief LEN0 through LEN3 (Length) fields (bits 18, 19, 22, 23, 26, 27, 30, and 31)
	*
	* [Bits 19:18] Specify the size of the memory location at the address specified in the corresponding breakpoint address
	* register (DR0 through DR3). These fields are interpreted as follows:
	* - 00 - 1-byte length.
	* - 01 - 2-byte length.
	* - 10 - Undefined (or 8 byte length, see note below).
	* - 11 - 4-byte length.
	* If the corresponding RWn field in register DR7 is 00 (instruction execution), then the LENn field should also be 00. The
	* effect of using other lengths is undefined.
	*
	* @see Vol3B[17.2.5(Breakpoint Field Recognition)]
	*/
	uint64_t length_0                                                : 2;
#define DR7_LENGTH_0_BIT                                             18
#define DR7_LENGTH_0_FLAG                                            0xC0000
#define DR7_LENGTH_0_MASK                                            0x03
#define DR7_LENGTH_0(_)                                              (((_) >> 18) & 0x03)
	uint64_t read_write_1                                            : 2;
#define DR7_READ_WRITE_1_BIT                                         20
#define DR7_READ_WRITE_1_FLAG                                        0x300000
#define DR7_READ_WRITE_1_MASK                                        0x03
#define DR7_READ_WRITE_1(_)                                          (((_) >> 20) & 0x03)
	uint64_t length_1                                                : 2;
#define DR7_LENGTH_1_BIT                                             22
#define DR7_LENGTH_1_FLAG                                            0xC00000
#define DR7_LENGTH_1_MASK                                            0x03
#define DR7_LENGTH_1(_)                                              (((_) >> 22) & 0x03)
	uint64_t read_write_2                                            : 2;
#define DR7_READ_WRITE_2_BIT                                         24
#define DR7_READ_WRITE_2_FLAG                                        0x3000000
#define DR7_READ_WRITE_2_MASK                                        0x03
#define DR7_READ_WRITE_2(_)                                          (((_) >> 24) & 0x03)
	uint64_t length_2                                                : 2;
#define DR7_LENGTH_2_BIT                                             26
#define DR7_LENGTH_2_FLAG                                            0xC000000
#define DR7_LENGTH_2_MASK                                            0x03
#define DR7_LENGTH_2(_)                                              (((_) >> 26) & 0x03)
	uint64_t read_write_3                                            : 2;
#define DR7_READ_WRITE_3_BIT                                         28
#define DR7_READ_WRITE_3_FLAG                                        0x30000000
#define DR7_READ_WRITE_3_MASK                                        0x03
#define DR7_READ_WRITE_3(_)                                          (((_) >> 28) & 0x03)
	uint64_t length_3                                                : 2;
#define DR7_LENGTH_3_BIT                                             30
#define DR7_LENGTH_3_FLAG                                            0xC0000000
#define DR7_LENGTH_3_MASK                                            0x03
#define DR7_LENGTH_3(_)                                              (((_) >> 30) & 0x03)
	uint64_t reserved4                                               : 32;
  };

  uint64_t flags;
} dr7;

/**
 * @}
 */

/**
 * @defgroup cpuid \
 *           CPUID
 *
 * @see Vol2A[3.2(CPUID)] (reference)
 * @{
 */
/**
 * @brief Returns CPUID's Highest Value for Basic Processor Information and the Vendor Identification String
 *
 * When CPUID executes with EAX set to 0, the processor returns the highest value the CPUID recognizes for returning basic
 * processor information. The value is returned in the EAX register and is processor specific.
 * A vendor identification string is also returned in EBX, EDX, and ECX. For Intel processors, the string is "GenuineIntel"
 * and is expressed:
 * - EBX <- 756e6547h (* "Genu", with G in the low eight bits of BL *)
 * - EDX <- 49656e69h (* "ineI", with i in the low eight bits of DL *)
 * - ECX <- 6c65746eh (* "ntel", with n in the low eight bits of CL *)
 */
#define CPUID_SIGNATURE                                              0x00000000
typedef struct
{
  /**
	* @brief EAX
	*
	* Maximum Input Value for Basic CPUID Information.
	*/
  uint32_t max_cpuid_input_value;

  /**
	* @brief EBX
	*
	* "Genu"
	*/
  uint32_t ebx_value_genu;

  /**
	* @brief ECX
	*
	* "ntel"
	*/
  uint32_t ecx_value_ntel;

  /**
	* @brief EDX
	*
	* "ineI"
	*/
  uint32_t edx_value_inei;
} cpuid_eax_00;


/**
 * @brief Returns Model, Family, Stepping Information, Additional Information and Feature Information
 *
 * Returns:
 *   * Model, Family, Stepping Information in EAX
 *   * Additional Information in EBX
 *   * Feature Information in ECX and EDX
 */
#define CPUID_VERSION_INFORMATION                                    0x00000001
typedef struct
{
  /**
	* @brief When CPUID executes with EAX set to 01H, version information is returned in EAX
	*/
  union
  {
	struct
	{
		uint32_t stepping_id                                           : 4;
#define CPUID_VERSION_INFORMATION_STEPPING_ID_BIT                    0
#define CPUID_VERSION_INFORMATION_STEPPING_ID_FLAG                   0x0F
#define CPUID_VERSION_INFORMATION_STEPPING_ID_MASK                   0x0F
#define CPUID_VERSION_INFORMATION_STEPPING_ID(_)                     (((_) >> 0) & 0x0F)
		uint32_t model                                                 : 4;
#define CPUID_VERSION_INFORMATION_MODEL_BIT                          4
#define CPUID_VERSION_INFORMATION_MODEL_FLAG                         0xF0
#define CPUID_VERSION_INFORMATION_MODEL_MASK                         0x0F
#define CPUID_VERSION_INFORMATION_MODEL(_)                           (((_) >> 4) & 0x0F)
		uint32_t family_id                                             : 4;
#define CPUID_VERSION_INFORMATION_FAMILY_ID_BIT                      8
#define CPUID_VERSION_INFORMATION_FAMILY_ID_FLAG                     0xF00
#define CPUID_VERSION_INFORMATION_FAMILY_ID_MASK                     0x0F
#define CPUID_VERSION_INFORMATION_FAMILY_ID(_)                       (((_) >> 8) & 0x0F)

		/**
		* [Bits 13:12] - 0 - Original OEM Processor
		* - 1 - Intel OverDrive(R) Processor
		* - 2 - Dual processor (not applicable to Intel486 processors)
		* - 3 - Intel reserved
		*/
		uint32_t processor_type                                        : 2;
#define CPUID_VERSION_INFORMATION_PROCESSOR_TYPE_BIT                 12
#define CPUID_VERSION_INFORMATION_PROCESSOR_TYPE_FLAG                0x3000
#define CPUID_VERSION_INFORMATION_PROCESSOR_TYPE_MASK                0x03
#define CPUID_VERSION_INFORMATION_PROCESSOR_TYPE(_)                  (((_) >> 12) & 0x03)
		uint32_t reserved1                                             : 2;

		/**
		* [Bits 19:16] The Extended Model ID needs to be examined only when the Family ID is 06H or 0FH.
		*/
		uint32_t extended_model_id                                     : 4;
#define CPUID_VERSION_INFORMATION_EXTENDED_MODEL_ID_BIT              16
#define CPUID_VERSION_INFORMATION_EXTENDED_MODEL_ID_FLAG             0xF0000
#define CPUID_VERSION_INFORMATION_EXTENDED_MODEL_ID_MASK             0x0F
#define CPUID_VERSION_INFORMATION_EXTENDED_MODEL_ID(_)               (((_) >> 16) & 0x0F)

		/**
		* [Bits 27:20] The Extended Family ID needs to be examined only when the Family ID is 0FH.
		*/
		uint32_t extended_family_id                                    : 8;
#define CPUID_VERSION_INFORMATION_EXTENDED_FAMILY_ID_BIT             20
#define CPUID_VERSION_INFORMATION_EXTENDED_FAMILY_ID_FLAG            0xFF00000
#define CPUID_VERSION_INFORMATION_EXTENDED_FAMILY_ID_MASK            0xFF
#define CPUID_VERSION_INFORMATION_EXTENDED_FAMILY_ID(_)              (((_) >> 20) & 0xFF)
		uint32_t reserved2                                             : 4;
	};

	uint32_t flags;
  } cpuid_version_information;

  /**
	* @brief When CPUID executes with EAX set to 01H, additional information is returned to the EBX register
	*/
  union
  {
	struct
	{
		/**
		* [Bits 7:0] This number provides an entry into a brand string table that contains brand strings for IA-32 processors.
		* More information about this field is provided later in this section.
		*/
		uint32_t brand_index                                           : 8;
#define CPUID_ADDITIONAL_INFORMATION_BRAND_INDEX_BIT                 0
#define CPUID_ADDITIONAL_INFORMATION_BRAND_INDEX_FLAG                0xFF
#define CPUID_ADDITIONAL_INFORMATION_BRAND_INDEX_MASK                0xFF
#define CPUID_ADDITIONAL_INFORMATION_BRAND_INDEX(_)                  (((_) >> 0) & 0xFF)

		/**
		* @brief Value * 8 = cache line size in bytes; used also by CLFLUSHOPT
		*
		* [Bits 15:8] This number indicates the size of the cache line flushed by the CLFLUSH and CLFLUSHOPT instructions in
		* 8-byte increments. This field was introduced in the Pentium 4 processor.
		*/
		uint32_t clflush_line_size                                     : 8;
#define CPUID_ADDITIONAL_INFORMATION_CLFLUSH_LINE_SIZE_BIT           8
#define CPUID_ADDITIONAL_INFORMATION_CLFLUSH_LINE_SIZE_FLAG          0xFF00
#define CPUID_ADDITIONAL_INFORMATION_CLFLUSH_LINE_SIZE_MASK          0xFF
#define CPUID_ADDITIONAL_INFORMATION_CLFLUSH_LINE_SIZE(_)            (((_) >> 8) & 0xFF)

		/**
		* [Bits 23:16] Maximum number of addressable IDs for logical processors in this physical package.
		*
		* @remarks The nearest power-of-2 integer that is not smaller than EBX[23:16] is the number of unique initial APIC IDs
		*          reserved for addressing different logical processors in a physical package. This field is only valid if
		*          CPUID.1.EDX.HTT[bit 28] = 1.
		*/
		uint32_t max_addressable_ids                                   : 8;
#define CPUID_ADDITIONAL_INFORMATION_MAX_ADDRESSABLE_IDS_BIT         16
#define CPUID_ADDITIONAL_INFORMATION_MAX_ADDRESSABLE_IDS_FLAG        0xFF0000
#define CPUID_ADDITIONAL_INFORMATION_MAX_ADDRESSABLE_IDS_MASK        0xFF
#define CPUID_ADDITIONAL_INFORMATION_MAX_ADDRESSABLE_IDS(_)          (((_) >> 16) & 0xFF)

		/**
		* [Bits 31:24] This number is the 8-bit ID that is assigned to the local APIC on the processor during power up. This field
		* was introduced in the Pentium 4 processor.
		*/
		uint32_t initial_apic_id                                       : 8;
#define CPUID_ADDITIONAL_INFORMATION_INITIAL_APIC_ID_BIT             24
#define CPUID_ADDITIONAL_INFORMATION_INITIAL_APIC_ID_FLAG            0xFF000000
#define CPUID_ADDITIONAL_INFORMATION_INITIAL_APIC_ID_MASK            0xFF
#define CPUID_ADDITIONAL_INFORMATION_INITIAL_APIC_ID(_)              (((_) >> 24) & 0xFF)
	};

	uint32_t flags;
  } cpuid_additional_information;

  /**
	* @brief When CPUID executes with EAX set to 01H, feature information is returned in ECX and EDX
	*/
  union
  {
	struct
	{
		/**
		* @brief Streaming SIMD Extensions 3 (SSE3)
		*
		* [Bit 0] A value of 1 indicates the processor supports this technology.
		*/
		uint32_t streaming_simd_extensions_3                           : 1;
#define CPUID_FEATURE_INFORMATION_ECX_STREAMING_SIMD_EXTENSIONS_3_BIT 0
#define CPUID_FEATURE_INFORMATION_ECX_STREAMING_SIMD_EXTENSIONS_3_FLAG 0x01
#define CPUID_FEATURE_INFORMATION_ECX_STREAMING_SIMD_EXTENSIONS_3_MASK 0x01
#define CPUID_FEATURE_INFORMATION_ECX_STREAMING_SIMD_EXTENSIONS_3(_) (((_) >> 0) & 0x01)

		/**
		* @brief PCLMULQDQ instruction
		*
		* [Bit 1] A value of 1 indicates the processor supports the PCLMULQDQ instruction.
		*/
		uint32_t pclmulqdq_instruction                                 : 1;
#define CPUID_FEATURE_INFORMATION_ECX_PCLMULQDQ_INSTRUCTION_BIT      1
#define CPUID_FEATURE_INFORMATION_ECX_PCLMULQDQ_INSTRUCTION_FLAG     0x02
#define CPUID_FEATURE_INFORMATION_ECX_PCLMULQDQ_INSTRUCTION_MASK     0x01
#define CPUID_FEATURE_INFORMATION_ECX_PCLMULQDQ_INSTRUCTION(_)       (((_) >> 1) & 0x01)

		/**
		* @brief 64-bit DS Area
		*
		* [Bit 2] A value of 1 indicates the processor supports DS area using 64-bit layout.
		*/
		uint32_t ds_area_64bit_layout                                  : 1;
#define CPUID_FEATURE_INFORMATION_ECX_DS_AREA_64BIT_LAYOUT_BIT       2
#define CPUID_FEATURE_INFORMATION_ECX_DS_AREA_64BIT_LAYOUT_FLAG      0x04
#define CPUID_FEATURE_INFORMATION_ECX_DS_AREA_64BIT_LAYOUT_MASK      0x01
#define CPUID_FEATURE_INFORMATION_ECX_DS_AREA_64BIT_LAYOUT(_)        (((_) >> 2) & 0x01)

		/**
		* @brief MONITOR/MWAIT instruction
		*
		* [Bit 3] A value of 1 indicates the processor supports this feature.
		*/
		uint32_t monitor_mwait_instruction                             : 1;
#define CPUID_FEATURE_INFORMATION_ECX_MONITOR_MWAIT_INSTRUCTION_BIT  3
#define CPUID_FEATURE_INFORMATION_ECX_MONITOR_MWAIT_INSTRUCTION_FLAG 0x08
#define CPUID_FEATURE_INFORMATION_ECX_MONITOR_MWAIT_INSTRUCTION_MASK 0x01
#define CPUID_FEATURE_INFORMATION_ECX_MONITOR_MWAIT_INSTRUCTION(_)   (((_) >> 3) & 0x01)

		/**
		* @brief CPL Qualified Debug Store
		*
		* [Bit 4] A value of 1 indicates the processor supports the extensions to the Debug Store feature to allow for branch
		* message storage qualified by CPL.
		*/
		uint32_t cpl_qualified_debug_store                             : 1;
#define CPUID_FEATURE_INFORMATION_ECX_CPL_QUALIFIED_DEBUG_STORE_BIT  4
#define CPUID_FEATURE_INFORMATION_ECX_CPL_QUALIFIED_DEBUG_STORE_FLAG 0x10
#define CPUID_FEATURE_INFORMATION_ECX_CPL_QUALIFIED_DEBUG_STORE_MASK 0x01
#define CPUID_FEATURE_INFORMATION_ECX_CPL_QUALIFIED_DEBUG_STORE(_)   (((_) >> 4) & 0x01)

		/**
		* @brief Virtual Machine Extensions
		*
		* [Bit 5] A value of 1 indicates that the processor supports this technology.
		*/
		uint32_t virtual_machine_extensions                            : 1;
#define CPUID_FEATURE_INFORMATION_ECX_VIRTUAL_MACHINE_EXTENSIONS_BIT 5
#define CPUID_FEATURE_INFORMATION_ECX_VIRTUAL_MACHINE_EXTENSIONS_FLAG 0x20
#define CPUID_FEATURE_INFORMATION_ECX_VIRTUAL_MACHINE_EXTENSIONS_MASK 0x01
#define CPUID_FEATURE_INFORMATION_ECX_VIRTUAL_MACHINE_EXTENSIONS(_)  (((_) >> 5) & 0x01)

		/**
		* @brief Safer Mode Extensions
		*
		* [Bit 6] A value of 1 indicates that the processor supports this technology.
		*
		* @see Vol2[6(SAFER MODE EXTENSIONS REFERENCE)]
		*/
		uint32_t safer_mode_extensions                                 : 1;
#define CPUID_FEATURE_INFORMATION_ECX_SAFER_MODE_EXTENSIONS_BIT      6
#define CPUID_FEATURE_INFORMATION_ECX_SAFER_MODE_EXTENSIONS_FLAG     0x40
#define CPUID_FEATURE_INFORMATION_ECX_SAFER_MODE_EXTENSIONS_MASK     0x01
#define CPUID_FEATURE_INFORMATION_ECX_SAFER_MODE_EXTENSIONS(_)       (((_) >> 6) & 0x01)

		/**
		* @brief Enhanced Intel SpeedStep(R) technology
		*
		* [Bit 7] A value of 1 indicates that the processor supports this technology.
		*/
		uint32_t enhanced_intel_speedstep_technology                   : 1;
#define CPUID_FEATURE_INFORMATION_ECX_ENHANCED_INTEL_SPEEDSTEP_TECHNOLOGY_BIT 7
#define CPUID_FEATURE_INFORMATION_ECX_ENHANCED_INTEL_SPEEDSTEP_TECHNOLOGY_FLAG 0x80
#define CPUID_FEATURE_INFORMATION_ECX_ENHANCED_INTEL_SPEEDSTEP_TECHNOLOGY_MASK 0x01
#define CPUID_FEATURE_INFORMATION_ECX_ENHANCED_INTEL_SPEEDSTEP_TECHNOLOGY(_) (((_) >> 7) & 0x01)

		/**
		* @brief Thermal Monitor 2
		*
		* [Bit 8] A value of 1 indicates whether the processor supports this technology.
		*/
		uint32_t thermal_monitor_2                                     : 1;
#define CPUID_FEATURE_INFORMATION_ECX_THERMAL_MONITOR_2_BIT          8
#define CPUID_FEATURE_INFORMATION_ECX_THERMAL_MONITOR_2_FLAG         0x100
#define CPUID_FEATURE_INFORMATION_ECX_THERMAL_MONITOR_2_MASK         0x01
#define CPUID_FEATURE_INFORMATION_ECX_THERMAL_MONITOR_2(_)           (((_) >> 8) & 0x01)

		/**
		* @brief Supplemental Streaming SIMD Extensions 3 (SSSE3)
		*
		* [Bit 9] A value of 1 indicates the presence of the Supplemental Streaming SIMD Extensions 3 (SSSE3). A value of 0
		* indicates the instruction extensions are not present in the processor.
		*/
		uint32_t supplemental_streaming_simd_extensions_3              : 1;
#define CPUID_FEATURE_INFORMATION_ECX_SUPPLEMENTAL_STREAMING_SIMD_EXTENSIONS_3_BIT 9
#define CPUID_FEATURE_INFORMATION_ECX_SUPPLEMENTAL_STREAMING_SIMD_EXTENSIONS_3_FLAG 0x200
#define CPUID_FEATURE_INFORMATION_ECX_SUPPLEMENTAL_STREAMING_SIMD_EXTENSIONS_3_MASK 0x01
#define CPUID_FEATURE_INFORMATION_ECX_SUPPLEMENTAL_STREAMING_SIMD_EXTENSIONS_3(_) (((_) >> 9) & 0x01)

		/**
		* @brief L1 Context ID
		*
		* [Bit 10] A value of 1 indicates the L1 data cache mode can be set to either adaptive mode or shared mode. A value of 0
		* indicates this feature is not supported. See definition of the IA32_MISC_ENABLE MSR Bit 24 (L1 Data Cache Context Mode)
		* for details.
		*/
		uint32_t l1_context_id                                         : 1;
#define CPUID_FEATURE_INFORMATION_ECX_L1_CONTEXT_ID_BIT              10
#define CPUID_FEATURE_INFORMATION_ECX_L1_CONTEXT_ID_FLAG             0x400
#define CPUID_FEATURE_INFORMATION_ECX_L1_CONTEXT_ID_MASK             0x01
#define CPUID_FEATURE_INFORMATION_ECX_L1_CONTEXT_ID(_)               (((_) >> 10) & 0x01)

		/**
		* @brief IA32_DEBUG_INTERFACE MSR for silicon debug
		*
		* [Bit 11] A value of 1 indicates the processor supports IA32_DEBUG_INTERFACE MSR for silicon debug.
		*/
		uint32_t silicon_debug                                         : 1;
#define CPUID_FEATURE_INFORMATION_ECX_SILICON_DEBUG_BIT              11
#define CPUID_FEATURE_INFORMATION_ECX_SILICON_DEBUG_FLAG             0x800
#define CPUID_FEATURE_INFORMATION_ECX_SILICON_DEBUG_MASK             0x01
#define CPUID_FEATURE_INFORMATION_ECX_SILICON_DEBUG(_)               (((_) >> 11) & 0x01)

		/**
		* @brief FMA extensions using YMM state
		*
		* [Bit 12] A value of 1 indicates the processor supports FMA (Fused Multiple Add) extensions using YMM state.
		*/
		uint32_t fma_extensions                                        : 1;
#define CPUID_FEATURE_INFORMATION_ECX_FMA_EXTENSIONS_BIT             12
#define CPUID_FEATURE_INFORMATION_ECX_FMA_EXTENSIONS_FLAG            0x1000
#define CPUID_FEATURE_INFORMATION_ECX_FMA_EXTENSIONS_MASK            0x01
#define CPUID_FEATURE_INFORMATION_ECX_FMA_EXTENSIONS(_)              (((_) >> 12) & 0x01)

		/**
		* @brief CMPXCHG16B instruction
		*
		* [Bit 13] A value of 1 indicates that the feature is available.
		*/
		uint32_t cmpxchg16b_instruction                                : 1;
#define CPUID_FEATURE_INFORMATION_ECX_CMPXCHG16B_INSTRUCTION_BIT     13
#define CPUID_FEATURE_INFORMATION_ECX_CMPXCHG16B_INSTRUCTION_FLAG    0x2000
#define CPUID_FEATURE_INFORMATION_ECX_CMPXCHG16B_INSTRUCTION_MASK    0x01
#define CPUID_FEATURE_INFORMATION_ECX_CMPXCHG16B_INSTRUCTION(_)      (((_) >> 13) & 0x01)

		/**
		* @brief xTPR Update Control
		*
		* [Bit 14] A value of 1 indicates that the processor supports changing IA32_MISC_ENABLE[bit 23].
		*/
		uint32_t xtpr_update_control                                   : 1;
#define CPUID_FEATURE_INFORMATION_ECX_XTPR_UPDATE_CONTROL_BIT        14
#define CPUID_FEATURE_INFORMATION_ECX_XTPR_UPDATE_CONTROL_FLAG       0x4000
#define CPUID_FEATURE_INFORMATION_ECX_XTPR_UPDATE_CONTROL_MASK       0x01
#define CPUID_FEATURE_INFORMATION_ECX_XTPR_UPDATE_CONTROL(_)         (((_) >> 14) & 0x01)

		/**
		* @brief Perfmon and Debug Capability
		*
		* [Bit 15] A value of 1 indicates the processor supports the performance and debug feature indication MSR
		* IA32_PERF_CAPABILITIES.
		*/
		uint32_t perfmon_and_debug_capability                          : 1;
#define CPUID_FEATURE_INFORMATION_ECX_PERFMON_AND_DEBUG_CAPABILITY_BIT 15
#define CPUID_FEATURE_INFORMATION_ECX_PERFMON_AND_DEBUG_CAPABILITY_FLAG 0x8000
#define CPUID_FEATURE_INFORMATION_ECX_PERFMON_AND_DEBUG_CAPABILITY_MASK 0x01
#define CPUID_FEATURE_INFORMATION_ECX_PERFMON_AND_DEBUG_CAPABILITY(_) (((_) >> 15) & 0x01)
		uint32_t reserved1                                             : 1;

		/**
		* @brief Process-context identifiers
		*
		* [Bit 17] A value of 1 indicates that the processor supports PCIDs and that software may set CR4.PCIDE to 1.
		*/
		uint32_t process_context_identifiers                           : 1;
#define CPUID_FEATURE_INFORMATION_ECX_PROCESS_CONTEXT_IDENTIFIERS_BIT 17
#define CPUID_FEATURE_INFORMATION_ECX_PROCESS_CONTEXT_IDENTIFIERS_FLAG 0x20000
#define CPUID_FEATURE_INFORMATION_ECX_PROCESS_CONTEXT_IDENTIFIERS_MASK 0x01
#define CPUID_FEATURE_INFORMATION_ECX_PROCESS_CONTEXT_IDENTIFIERS(_) (((_) >> 17) & 0x01)

		/**
		* @brief Direct Cache Access
		*
		* [Bit 18] A value of 1 indicates the processor supports the ability to prefetch data from a memory mapped device (Direct
		* Cache Access).
		*/
		uint32_t direct_cache_access                                   : 1;
#define CPUID_FEATURE_INFORMATION_ECX_DIRECT_CACHE_ACCESS_BIT        18
#define CPUID_FEATURE_INFORMATION_ECX_DIRECT_CACHE_ACCESS_FLAG       0x40000
#define CPUID_FEATURE_INFORMATION_ECX_DIRECT_CACHE_ACCESS_MASK       0x01
#define CPUID_FEATURE_INFORMATION_ECX_DIRECT_CACHE_ACCESS(_)         (((_) >> 18) & 0x01)

		/**
		* @brief SSE4.1 support
		*
		* [Bit 19] A value of 1 indicates that the processor supports SSE4.1.
		*/
		uint32_t sse41_support                                         : 1;
#define CPUID_FEATURE_INFORMATION_ECX_SSE41_SUPPORT_BIT              19
#define CPUID_FEATURE_INFORMATION_ECX_SSE41_SUPPORT_FLAG             0x80000
#define CPUID_FEATURE_INFORMATION_ECX_SSE41_SUPPORT_MASK             0x01
#define CPUID_FEATURE_INFORMATION_ECX_SSE41_SUPPORT(_)               (((_) >> 19) & 0x01)

		/**
		* @brief SSE4.2 support
		*
		* [Bit 20] A value of 1 indicates that the processor supports SSE4.2.
		*/
		uint32_t sse42_support                                         : 1;
#define CPUID_FEATURE_INFORMATION_ECX_SSE42_SUPPORT_BIT              20
#define CPUID_FEATURE_INFORMATION_ECX_SSE42_SUPPORT_FLAG             0x100000
#define CPUID_FEATURE_INFORMATION_ECX_SSE42_SUPPORT_MASK             0x01
#define CPUID_FEATURE_INFORMATION_ECX_SSE42_SUPPORT(_)               (((_) >> 20) & 0x01)

		/**
		* @brief x2APIC support
		*
		* [Bit 21] A value of 1 indicates that the processor supports x2APIC feature.
		*/
		uint32_t x2apic_support                                        : 1;
#define CPUID_FEATURE_INFORMATION_ECX_X2APIC_SUPPORT_BIT             21
#define CPUID_FEATURE_INFORMATION_ECX_X2APIC_SUPPORT_FLAG            0x200000
#define CPUID_FEATURE_INFORMATION_ECX_X2APIC_SUPPORT_MASK            0x01
#define CPUID_FEATURE_INFORMATION_ECX_X2APIC_SUPPORT(_)              (((_) >> 21) & 0x01)

		/**
		* @brief MOVBE instruction
		*
		* [Bit 22] A value of 1 indicates that the processor supports MOVBE instruction.
		*/
		uint32_t movbe_instruction                                     : 1;
#define CPUID_FEATURE_INFORMATION_ECX_MOVBE_INSTRUCTION_BIT          22
#define CPUID_FEATURE_INFORMATION_ECX_MOVBE_INSTRUCTION_FLAG         0x400000
#define CPUID_FEATURE_INFORMATION_ECX_MOVBE_INSTRUCTION_MASK         0x01
#define CPUID_FEATURE_INFORMATION_ECX_MOVBE_INSTRUCTION(_)           (((_) >> 22) & 0x01)

		/**
		* @brief POPCNT instruction
		*
		* [Bit 23] A value of 1 indicates that the processor supports the POPCNT instruction.
		*/
		uint32_t popcnt_instruction                                    : 1;
#define CPUID_FEATURE_INFORMATION_ECX_POPCNT_INSTRUCTION_BIT         23
#define CPUID_FEATURE_INFORMATION_ECX_POPCNT_INSTRUCTION_FLAG        0x800000
#define CPUID_FEATURE_INFORMATION_ECX_POPCNT_INSTRUCTION_MASK        0x01
#define CPUID_FEATURE_INFORMATION_ECX_POPCNT_INSTRUCTION(_)          (((_) >> 23) & 0x01)

		/**
		* @brief TSC Deadline
		*
		* [Bit 24] A value of 1 indicates that the processor's local APIC timer supports one-shot operation using a TSC deadline
		* value.
		*/
		uint32_t tsc_deadline                                          : 1;
#define CPUID_FEATURE_INFORMATION_ECX_TSC_DEADLINE_BIT               24
#define CPUID_FEATURE_INFORMATION_ECX_TSC_DEADLINE_FLAG              0x1000000
#define CPUID_FEATURE_INFORMATION_ECX_TSC_DEADLINE_MASK              0x01
#define CPUID_FEATURE_INFORMATION_ECX_TSC_DEADLINE(_)                (((_) >> 24) & 0x01)

		/**
		* @brief AESNI instruction extensions
		*
		* [Bit 25] A value of 1 indicates that the processor supports the AESNI instruction extensions.
		*/
		uint32_t aesni_instruction_extensions                          : 1;
#define CPUID_FEATURE_INFORMATION_ECX_AESNI_INSTRUCTION_EXTENSIONS_BIT 25
#define CPUID_FEATURE_INFORMATION_ECX_AESNI_INSTRUCTION_EXTENSIONS_FLAG 0x2000000
#define CPUID_FEATURE_INFORMATION_ECX_AESNI_INSTRUCTION_EXTENSIONS_MASK 0x01
#define CPUID_FEATURE_INFORMATION_ECX_AESNI_INSTRUCTION_EXTENSIONS(_) (((_) >> 25) & 0x01)

		/**
		* @brief XSAVE/XRSTOR instruction extensions
		*
		* [Bit 26] A value of 1 indicates that the processor supports the XSAVE/XRSTOR processor extended states feature, the
		* XSETBV/XGETBV instructions, and XCR0.
		*/
		uint32_t xsave_xrstor_instruction                              : 1;
#define CPUID_FEATURE_INFORMATION_ECX_XSAVE_XRSTOR_INSTRUCTION_BIT   26
#define CPUID_FEATURE_INFORMATION_ECX_XSAVE_XRSTOR_INSTRUCTION_FLAG  0x4000000
#define CPUID_FEATURE_INFORMATION_ECX_XSAVE_XRSTOR_INSTRUCTION_MASK  0x01
#define CPUID_FEATURE_INFORMATION_ECX_XSAVE_XRSTOR_INSTRUCTION(_)    (((_) >> 26) & 0x01)

		/**
		* @brief CR4.OSXSAVE[bit 18] set
		*
		* [Bit 27] A value of 1 indicates that the OS has set CR4.OSXSAVE[bit 18] to enable XSETBV/XGETBV instructions to access
		* XCR0 and to support processor extended state management using XSAVE/XRSTOR.
		*/
		uint32_t osx_save                                              : 1;
#define CPUID_FEATURE_INFORMATION_ECX_OSX_SAVE_BIT                   27
#define CPUID_FEATURE_INFORMATION_ECX_OSX_SAVE_FLAG                  0x8000000
#define CPUID_FEATURE_INFORMATION_ECX_OSX_SAVE_MASK                  0x01
#define CPUID_FEATURE_INFORMATION_ECX_OSX_SAVE(_)                    (((_) >> 27) & 0x01)

		/**
		* @brief AVX instruction extensions support
		*
		* [Bit 28] A value of 1 indicates the processor supports the AVX instruction extensions.
		*/
		uint32_t avx_support                                           : 1;
#define CPUID_FEATURE_INFORMATION_ECX_AVX_SUPPORT_BIT                28
#define CPUID_FEATURE_INFORMATION_ECX_AVX_SUPPORT_FLAG               0x10000000
#define CPUID_FEATURE_INFORMATION_ECX_AVX_SUPPORT_MASK               0x01
#define CPUID_FEATURE_INFORMATION_ECX_AVX_SUPPORT(_)                 (((_) >> 28) & 0x01)

		/**
		* @brief 16-bit floating-point conversion instructions support
		*
		* [Bit 29] A value of 1 indicates that processor supports 16-bit floating-point conversion instructions.
		*/
		uint32_t half_precision_conversion_instructions                : 1;
#define CPUID_FEATURE_INFORMATION_ECX_HALF_PRECISION_CONVERSION_INSTRUCTIONS_BIT 29
#define CPUID_FEATURE_INFORMATION_ECX_HALF_PRECISION_CONVERSION_INSTRUCTIONS_FLAG 0x20000000
#define CPUID_FEATURE_INFORMATION_ECX_HALF_PRECISION_CONVERSION_INSTRUCTIONS_MASK 0x01
#define CPUID_FEATURE_INFORMATION_ECX_HALF_PRECISION_CONVERSION_INSTRUCTIONS(_) (((_) >> 29) & 0x01)

		/**
		* @brief RDRAND instruction support
		*
		* [Bit 30] A value of 1 indicates that processor supports RDRAND instruction.
		*/
		uint32_t rdrand_instruction                                    : 1;
#define CPUID_FEATURE_INFORMATION_ECX_RDRAND_INSTRUCTION_BIT         30
#define CPUID_FEATURE_INFORMATION_ECX_RDRAND_INSTRUCTION_FLAG        0x40000000
#define CPUID_FEATURE_INFORMATION_ECX_RDRAND_INSTRUCTION_MASK        0x01
#define CPUID_FEATURE_INFORMATION_ECX_RDRAND_INSTRUCTION(_)          (((_) >> 30) & 0x01)
		
		uint32_t hypervisor_present                                    : 1;
	};

	uint32_t flags;
  } cpuid_feature_information_ecx;

  /**
	* @brief When CPUID executes with EAX set to 01H, feature information is returned in ECX and EDX
	*/
  union
  {
	struct
	{
		/**
		* @brief Floating Point Unit On-Chip
		*
		* [Bit 0] The processor contains an x87 FPU.
		*/
		uint32_t floating_point_unit_on_chip                           : 1;
#define CPUID_FEATURE_INFORMATION_EDX_FLOATING_POINT_UNIT_ON_CHIP_BIT 0
#define CPUID_FEATURE_INFORMATION_EDX_FLOATING_POINT_UNIT_ON_CHIP_FLAG 0x01
#define CPUID_FEATURE_INFORMATION_EDX_FLOATING_POINT_UNIT_ON_CHIP_MASK 0x01
#define CPUID_FEATURE_INFORMATION_EDX_FLOATING_POINT_UNIT_ON_CHIP(_) (((_) >> 0) & 0x01)

		/**
		* @brief Virtual 8086 Mode Enhancements
		*
		* [Bit 1] Virtual 8086 mode enhancements, including CR4.VME for controlling the feature, CR4.PVI for protected mode
		* virtual interrupts, software interrupt indirection, expansion of the TSS with the software indirection bitmap, and
		* EFLAGS.VIF and EFLAGS.VIP flags.
		*/
		uint32_t virtual_8086_mode_enhancements                        : 1;
#define CPUID_FEATURE_INFORMATION_EDX_VIRTUAL_8086_MODE_ENHANCEMENTS_BIT 1
#define CPUID_FEATURE_INFORMATION_EDX_VIRTUAL_8086_MODE_ENHANCEMENTS_FLAG 0x02
#define CPUID_FEATURE_INFORMATION_EDX_VIRTUAL_8086_MODE_ENHANCEMENTS_MASK 0x01
#define CPUID_FEATURE_INFORMATION_EDX_VIRTUAL_8086_MODE_ENHANCEMENTS(_) (((_) >> 1) & 0x01)

		/**
		* @brief Debugging Extensions
		*
		* [Bit 2] Support for I/O breakpoints, including CR4.DE for controlling the feature, and optional trapping of accesses to
		* DR4 and DR5.
		*/
		uint32_t debugging_extensions                                  : 1;
#define CPUID_FEATURE_INFORMATION_EDX_DEBUGGING_EXTENSIONS_BIT       2
#define CPUID_FEATURE_INFORMATION_EDX_DEBUGGING_EXTENSIONS_FLAG      0x04
#define CPUID_FEATURE_INFORMATION_EDX_DEBUGGING_EXTENSIONS_MASK      0x01
#define CPUID_FEATURE_INFORMATION_EDX_DEBUGGING_EXTENSIONS(_)        (((_) >> 2) & 0x01)

		/**
		* @brief Page Size Extension
		*
		* [Bit 3] Large pages of size 4 MByte are supported, including CR4.PSE for controlling the feature, the defined dirty bit
		* in PDE (Page Directory Entries), optional reserved bit trapping in CR3, PDEs, and PTEs.
		*/
		uint32_t page_size_extension                                   : 1;
#define CPUID_FEATURE_INFORMATION_EDX_PAGE_SIZE_EXTENSION_BIT        3
#define CPUID_FEATURE_INFORMATION_EDX_PAGE_SIZE_EXTENSION_FLAG       0x08
#define CPUID_FEATURE_INFORMATION_EDX_PAGE_SIZE_EXTENSION_MASK       0x01
#define CPUID_FEATURE_INFORMATION_EDX_PAGE_SIZE_EXTENSION(_)         (((_) >> 3) & 0x01)

		/**
		* @brief Time Stamp Counter
		*
		* [Bit 4] The RDTSC instruction is supported, including CR4.TSD for controlling privilege.
		*/
		uint32_t timestamp_counter                                     : 1;
#define CPUID_FEATURE_INFORMATION_EDX_TIMESTAMP_COUNTER_BIT          4
#define CPUID_FEATURE_INFORMATION_EDX_TIMESTAMP_COUNTER_FLAG         0x10
#define CPUID_FEATURE_INFORMATION_EDX_TIMESTAMP_COUNTER_MASK         0x01
#define CPUID_FEATURE_INFORMATION_EDX_TIMESTAMP_COUNTER(_)           (((_) >> 4) & 0x01)

		/**
		* @brief Model Specific Registers RDMSR and WRMSR Instructions
		*
		* [Bit 5] The RDMSR and WRMSR instructions are supported. Some of the MSRs are implementation dependent.
		*/
		uint32_t rdmsr_wrmsr_instructions                              : 1;
#define CPUID_FEATURE_INFORMATION_EDX_RDMSR_WRMSR_INSTRUCTIONS_BIT   5
#define CPUID_FEATURE_INFORMATION_EDX_RDMSR_WRMSR_INSTRUCTIONS_FLAG  0x20
#define CPUID_FEATURE_INFORMATION_EDX_RDMSR_WRMSR_INSTRUCTIONS_MASK  0x01
#define CPUID_FEATURE_INFORMATION_EDX_RDMSR_WRMSR_INSTRUCTIONS(_)    (((_) >> 5) & 0x01)

		/**
		* @brief Physical Address Extension
		*
		* [Bit 6] Physical addresses greater than 32 bits are supported: extended page table entry formats, an extra level in the
		* page translation tables is defined, 2-MByte pages are supported instead of 4 Mbyte pages if PAE bit is 1.
		*/
		uint32_t physical_address_extension                            : 1;
#define CPUID_FEATURE_INFORMATION_EDX_PHYSICAL_ADDRESS_EXTENSION_BIT 6
#define CPUID_FEATURE_INFORMATION_EDX_PHYSICAL_ADDRESS_EXTENSION_FLAG 0x40
#define CPUID_FEATURE_INFORMATION_EDX_PHYSICAL_ADDRESS_EXTENSION_MASK 0x01
#define CPUID_FEATURE_INFORMATION_EDX_PHYSICAL_ADDRESS_EXTENSION(_)  (((_) >> 6) & 0x01)

		/**
		* @brief Machine Check Exception
		*
		* [Bit 7] Exception 18 is defined for Machine Checks, including CR4.MCE for controlling the feature. This feature does not
		* define the model-specific implementations of machine-check error logging, reporting, and processor shutdowns. Machine
		* Check exception handlers may have to depend on processor version to do model specific processing of the exception, or
		* test for the presence of the Machine Check feature.
		*/
		uint32_t machine_check_exception                               : 1;
#define CPUID_FEATURE_INFORMATION_EDX_MACHINE_CHECK_EXCEPTION_BIT    7
#define CPUID_FEATURE_INFORMATION_EDX_MACHINE_CHECK_EXCEPTION_FLAG   0x80
#define CPUID_FEATURE_INFORMATION_EDX_MACHINE_CHECK_EXCEPTION_MASK   0x01
#define CPUID_FEATURE_INFORMATION_EDX_MACHINE_CHECK_EXCEPTION(_)     (((_) >> 7) & 0x01)

		/**
		* @brief CMPXCHG8B Instruction
		*
		* [Bit 8] The compare-and-exchange 8 bytes (64 bits) instruction is supported (implicitly locked and atomic).
		*/
		uint32_t cmpxchg8b                                             : 1;
#define CPUID_FEATURE_INFORMATION_EDX_CMPXCHG8B_BIT                  8
#define CPUID_FEATURE_INFORMATION_EDX_CMPXCHG8B_FLAG                 0x100
#define CPUID_FEATURE_INFORMATION_EDX_CMPXCHG8B_MASK                 0x01
#define CPUID_FEATURE_INFORMATION_EDX_CMPXCHG8B(_)                   (((_) >> 8) & 0x01)

		/**
		* @brief APIC On-Chip
		*
		* [Bit 9] The processor contains an Advanced Programmable Interrupt Controller (APIC), responding to memory mapped
		* commands in the physical address range FFFE0000H to FFFE0FFFH (by default - some processors permit the APIC to be
		* relocated).
		*/
		uint32_t apic_on_chip                                          : 1;
#define CPUID_FEATURE_INFORMATION_EDX_APIC_ON_CHIP_BIT               9
#define CPUID_FEATURE_INFORMATION_EDX_APIC_ON_CHIP_FLAG              0x200
#define CPUID_FEATURE_INFORMATION_EDX_APIC_ON_CHIP_MASK              0x01
#define CPUID_FEATURE_INFORMATION_EDX_APIC_ON_CHIP(_)                (((_) >> 9) & 0x01)
		uint32_t reserved1                                             : 1;

		/**
		* @brief SYSENTER and SYSEXIT Instructions
		*
		* [Bit 11] The SYSENTER and SYSEXIT and associated MSRs are supported.
		*/
		uint32_t sysenter_sysexit_instructions                         : 1;
#define CPUID_FEATURE_INFORMATION_EDX_SYSENTER_SYSEXIT_INSTRUCTIONS_BIT 11
#define CPUID_FEATURE_INFORMATION_EDX_SYSENTER_SYSEXIT_INSTRUCTIONS_FLAG 0x800
#define CPUID_FEATURE_INFORMATION_EDX_SYSENTER_SYSEXIT_INSTRUCTIONS_MASK 0x01
#define CPUID_FEATURE_INFORMATION_EDX_SYSENTER_SYSEXIT_INSTRUCTIONS(_) (((_) >> 11) & 0x01)

		/**
		* @brief Memory Type Range Registers
		*
		* [Bit 12] MTRRs are supported. The MTRRcap MSR contains feature bits that describe what memory types are supported, how
		* many variable MTRRs are supported, and whether fixed MTRRs are supported.
		*/
		uint32_t memory_type_range_registers                           : 1;
#define CPUID_FEATURE_INFORMATION_EDX_MEMORY_TYPE_RANGE_REGISTERS_BIT 12
#define CPUID_FEATURE_INFORMATION_EDX_MEMORY_TYPE_RANGE_REGISTERS_FLAG 0x1000
#define CPUID_FEATURE_INFORMATION_EDX_MEMORY_TYPE_RANGE_REGISTERS_MASK 0x01
#define CPUID_FEATURE_INFORMATION_EDX_MEMORY_TYPE_RANGE_REGISTERS(_) (((_) >> 12) & 0x01)

		/**
		* @brief Page Global Bit
		*
		* [Bit 13] The global bit is supported in paging-structure entries that map a page, indicating TLB entries that are common
		* to different processes and need not be flushed. The CR4.PGE bit controls this feature.
		*/
		uint32_t page_global_bit                                       : 1;
#define CPUID_FEATURE_INFORMATION_EDX_PAGE_GLOBAL_BIT_BIT            13
#define CPUID_FEATURE_INFORMATION_EDX_PAGE_GLOBAL_BIT_FLAG           0x2000
#define CPUID_FEATURE_INFORMATION_EDX_PAGE_GLOBAL_BIT_MASK           0x01
#define CPUID_FEATURE_INFORMATION_EDX_PAGE_GLOBAL_BIT(_)             (((_) >> 13) & 0x01)

		/**
		* @brief Machine Check Architecture
		*
		* [Bit 14] A value of 1 indicates the Machine Check Architecture of reporting machine errors is supported. The MCG_CAP MSR
		* contains feature bits describing how many banks of error reporting MSRs are supported.
		*/
		uint32_t machine_check_architecture                            : 1;
#define CPUID_FEATURE_INFORMATION_EDX_MACHINE_CHECK_ARCHITECTURE_BIT 14
#define CPUID_FEATURE_INFORMATION_EDX_MACHINE_CHECK_ARCHITECTURE_FLAG 0x4000
#define CPUID_FEATURE_INFORMATION_EDX_MACHINE_CHECK_ARCHITECTURE_MASK 0x01
#define CPUID_FEATURE_INFORMATION_EDX_MACHINE_CHECK_ARCHITECTURE(_)  (((_) >> 14) & 0x01)

		/**
		* @brief Conditional Move Instructions
		*
		* [Bit 15] The conditional move instruction CMOV is supported. In addition, if x87 FPU is present as indicated by the
		* CPUID.FPU feature bit, then the FCOMI and FCMOV instructions are supported
		*/
		uint32_t conditional_move_instructions                         : 1;
#define CPUID_FEATURE_INFORMATION_EDX_CONDITIONAL_MOVE_INSTRUCTIONS_BIT 15
#define CPUID_FEATURE_INFORMATION_EDX_CONDITIONAL_MOVE_INSTRUCTIONS_FLAG 0x8000
#define CPUID_FEATURE_INFORMATION_EDX_CONDITIONAL_MOVE_INSTRUCTIONS_MASK 0x01
#define CPUID_FEATURE_INFORMATION_EDX_CONDITIONAL_MOVE_INSTRUCTIONS(_) (((_) >> 15) & 0x01)

		/**
		* @brief Page Attribute Table
		*
		* [Bit 16] Page Attribute Table is supported. This feature augments the Memory Type Range Registers (MTRRs), allowing an
		* operating system to specify attributes of memory accessed through a linear address on a 4KB granularity.
		*/
		uint32_t page_attribute_table                                  : 1;
#define CPUID_FEATURE_INFORMATION_EDX_PAGE_ATTRIBUTE_TABLE_BIT       16
#define CPUID_FEATURE_INFORMATION_EDX_PAGE_ATTRIBUTE_TABLE_FLAG      0x10000
#define CPUID_FEATURE_INFORMATION_EDX_PAGE_ATTRIBUTE_TABLE_MASK      0x01
#define CPUID_FEATURE_INFORMATION_EDX_PAGE_ATTRIBUTE_TABLE(_)        (((_) >> 16) & 0x01)

		/**
		* @brief 36-Bit Page Size Extension
		*
		* [Bit 17] 4-MByte pages addressing physical memory beyond 4 GBytes are supported with 32-bit paging. This feature
		* indicates that upper bits of the physical address of a 4-MByte page are encoded in bits 20:13 of the page-directory
		* entry. Such physical addresses are limited by MAXPHYADDR and may be up to 40 bits in size.
		*/
		uint32_t page_size_extension_36bit                             : 1;
#define CPUID_FEATURE_INFORMATION_EDX_PAGE_SIZE_EXTENSION_36BIT_BIT  17
#define CPUID_FEATURE_INFORMATION_EDX_PAGE_SIZE_EXTENSION_36BIT_FLAG 0x20000
#define CPUID_FEATURE_INFORMATION_EDX_PAGE_SIZE_EXTENSION_36BIT_MASK 0x01
#define CPUID_FEATURE_INFORMATION_EDX_PAGE_SIZE_EXTENSION_36BIT(_)   (((_) >> 17) & 0x01)

		/**
		* @brief Processor Serial Number
		*
		* [Bit 18] The processor supports the 96-bit processor identification number feature and the feature is enabled.
		*/
		uint32_t processor_serial_number                               : 1;
#define CPUID_FEATURE_INFORMATION_EDX_PROCESSOR_SERIAL_NUMBER_BIT    18
#define CPUID_FEATURE_INFORMATION_EDX_PROCESSOR_SERIAL_NUMBER_FLAG   0x40000
#define CPUID_FEATURE_INFORMATION_EDX_PROCESSOR_SERIAL_NUMBER_MASK   0x01
#define CPUID_FEATURE_INFORMATION_EDX_PROCESSOR_SERIAL_NUMBER(_)     (((_) >> 18) & 0x01)

		/**
		* @brief CLFLUSH Instruction
		*
		* [Bit 19] CLFLUSH Instruction is supported.
		*/
		uint32_t clflush                                               : 1;
#define CPUID_FEATURE_INFORMATION_EDX_CLFLUSH_BIT                    19
#define CPUID_FEATURE_INFORMATION_EDX_CLFLUSH_FLAG                   0x80000
#define CPUID_FEATURE_INFORMATION_EDX_CLFLUSH_MASK                   0x01
#define CPUID_FEATURE_INFORMATION_EDX_CLFLUSH(_)                     (((_) >> 19) & 0x01)
		uint32_t reserved2                                             : 1;

		/**
		* @brief Debug Store
		*
		* [Bit 21] The processor supports the ability to write debug information into a memory resident buffer. This feature is
		* used by the branch trace store (BTS) and processor event-based sampling (PEBS) facilities.
		*
		* @see Vol3C[23(INTRODUCTION TO VIRTUAL MACHINE EXTENSIONS)]
		*/
		uint32_t debug_store                                           : 1;
#define CPUID_FEATURE_INFORMATION_EDX_DEBUG_STORE_BIT                21
#define CPUID_FEATURE_INFORMATION_EDX_DEBUG_STORE_FLAG               0x200000
#define CPUID_FEATURE_INFORMATION_EDX_DEBUG_STORE_MASK               0x01
#define CPUID_FEATURE_INFORMATION_EDX_DEBUG_STORE(_)                 (((_) >> 21) & 0x01)

		/**
		* @brief Thermal Monitor and Software Controlled Clock Facilities
		*
		* [Bit 22] The processor implements internal MSRs that allow processor temperature to be monitored and processor
		* performance to be modulated in predefined duty cycles under software control.
		*/
		uint32_t thermal_control_msrs_for_acpi                         : 1;
#define CPUID_FEATURE_INFORMATION_EDX_THERMAL_CONTROL_MSRS_FOR_ACPI_BIT 22
#define CPUID_FEATURE_INFORMATION_EDX_THERMAL_CONTROL_MSRS_FOR_ACPI_FLAG 0x400000
#define CPUID_FEATURE_INFORMATION_EDX_THERMAL_CONTROL_MSRS_FOR_ACPI_MASK 0x01
#define CPUID_FEATURE_INFORMATION_EDX_THERMAL_CONTROL_MSRS_FOR_ACPI(_) (((_) >> 22) & 0x01)

		/**
		* @brief Intel MMX Technology
		*
		* [Bit 23] The processor supports the Intel MMX technology.
		*/
		uint32_t mmx_support                                           : 1;
#define CPUID_FEATURE_INFORMATION_EDX_MMX_SUPPORT_BIT                23
#define CPUID_FEATURE_INFORMATION_EDX_MMX_SUPPORT_FLAG               0x800000
#define CPUID_FEATURE_INFORMATION_EDX_MMX_SUPPORT_MASK               0x01
#define CPUID_FEATURE_INFORMATION_EDX_MMX_SUPPORT(_)                 (((_) >> 23) & 0x01)

		/**
		* @brief FXSAVE and FXRSTOR Instructions
		*
		* [Bit 24] The FXSAVE and FXRSTOR instructions are supported for fast save and restore of the floating point context.
		* Presence of this bit also indicates that CR4.OSFXSR is available for an operating system to indicate that it supports
		* the FXSAVE and FXRSTOR instructions.
		*/
		uint32_t fxsave_fxrstor_instructions                           : 1;
#define CPUID_FEATURE_INFORMATION_EDX_FXSAVE_FXRSTOR_INSTRUCTIONS_BIT 24
#define CPUID_FEATURE_INFORMATION_EDX_FXSAVE_FXRSTOR_INSTRUCTIONS_FLAG 0x1000000
#define CPUID_FEATURE_INFORMATION_EDX_FXSAVE_FXRSTOR_INSTRUCTIONS_MASK 0x01
#define CPUID_FEATURE_INFORMATION_EDX_FXSAVE_FXRSTOR_INSTRUCTIONS(_) (((_) >> 24) & 0x01)

		/**
		* @brief SSE extensions support
		*
		* [Bit 25] The processor supports the SSE extensions.
		*/
		uint32_t sse_support                                           : 1;
#define CPUID_FEATURE_INFORMATION_EDX_SSE_SUPPORT_BIT                25
#define CPUID_FEATURE_INFORMATION_EDX_SSE_SUPPORT_FLAG               0x2000000
#define CPUID_FEATURE_INFORMATION_EDX_SSE_SUPPORT_MASK               0x01
#define CPUID_FEATURE_INFORMATION_EDX_SSE_SUPPORT(_)                 (((_) >> 25) & 0x01)

		/**
		* @brief SSE2 extensions support
		*
		* [Bit 26] The processor supports the SSE2 extensions.
		*/
		uint32_t sse2_support                                          : 1;
#define CPUID_FEATURE_INFORMATION_EDX_SSE2_SUPPORT_BIT               26
#define CPUID_FEATURE_INFORMATION_EDX_SSE2_SUPPORT_FLAG              0x4000000
#define CPUID_FEATURE_INFORMATION_EDX_SSE2_SUPPORT_MASK              0x01
#define CPUID_FEATURE_INFORMATION_EDX_SSE2_SUPPORT(_)                (((_) >> 26) & 0x01)

		/**
		* @brief Self Snoop
		*
		* [Bit 27] The processor supports the management of conflicting memory types by performing a snoop of its own cache
		* structure for transactions issued to the bus.
		*/
		uint32_t self_snoop                                            : 1;
#define CPUID_FEATURE_INFORMATION_EDX_SELF_SNOOP_BIT                 27
#define CPUID_FEATURE_INFORMATION_EDX_SELF_SNOOP_FLAG                0x8000000
#define CPUID_FEATURE_INFORMATION_EDX_SELF_SNOOP_MASK                0x01
#define CPUID_FEATURE_INFORMATION_EDX_SELF_SNOOP(_)                  (((_) >> 27) & 0x01)

		/**
		* @brief Max APIC IDs reserved field is Valid
		*
		* [Bit 28] A value of 0 for HTT indicates there is only a single logical processor in the package and software should
		* assume only a single APIC ID is reserved. A value of 1 for HTT indicates the value in CPUID.1.EBX[23:16] (the Maximum
		* number of addressable IDs for logical processors in this package) is valid for the package.
		*/
		uint32_t hyper_threading_technology                            : 1;
#define CPUID_FEATURE_INFORMATION_EDX_HYPER_THREADING_TECHNOLOGY_BIT 28
#define CPUID_FEATURE_INFORMATION_EDX_HYPER_THREADING_TECHNOLOGY_FLAG 0x10000000
#define CPUID_FEATURE_INFORMATION_EDX_HYPER_THREADING_TECHNOLOGY_MASK 0x01
#define CPUID_FEATURE_INFORMATION_EDX_HYPER_THREADING_TECHNOLOGY(_)  (((_) >> 28) & 0x01)

		/**
		* @brief Thermal Monitor
		*
		* [Bit 29] The processor implements the thermal monitor automatic thermal control circuitry (TCC).
		*/
		uint32_t thermal_monitor                                       : 1;
#define CPUID_FEATURE_INFORMATION_EDX_THERMAL_MONITOR_BIT            29
#define CPUID_FEATURE_INFORMATION_EDX_THERMAL_MONITOR_FLAG           0x20000000
#define CPUID_FEATURE_INFORMATION_EDX_THERMAL_MONITOR_MASK           0x01
#define CPUID_FEATURE_INFORMATION_EDX_THERMAL_MONITOR(_)             (((_) >> 29) & 0x01)
		uint32_t reserved3                                             : 1;

		/**
		* @brief Pending Break Enable
		*
		* [Bit 31] The processor supports the use of the FERR\#/PBE\# pin when the processor is in the stop-clock state (STPCLK\#
		* is asserted) to signal the processor that an interrupt is pending and that the processor should return to normal
		* operation to handle the interrupt. Bit 10 (PBE enable) in the IA32_MISC_ENABLE MSR enables this capability.
		*/
		uint32_t pending_break_enable                                  : 1;
#define CPUID_FEATURE_INFORMATION_EDX_PENDING_BREAK_ENABLE_BIT       31
#define CPUID_FEATURE_INFORMATION_EDX_PENDING_BREAK_ENABLE_FLAG      0x80000000
#define CPUID_FEATURE_INFORMATION_EDX_PENDING_BREAK_ENABLE_MASK      0x01
#define CPUID_FEATURE_INFORMATION_EDX_PENDING_BREAK_ENABLE(_)        (((_) >> 31) & 0x01)
	};

	uint32_t flags;
  } cpuid_feature_information_edx;

} cpuid_eax_01;


/**
 * @brief Deterministic Cache Parameters Leaf
 *
 * When CPUID executes with EAX set to 04H and ECX contains an index value, the processor returns encoded data that
 * describe a set of deterministic cache parameters (for the cache level associated with the input in ECX). Valid index
 * values start from 0.
 * Software can enumerate the deterministic cache parameters for each level of the cache hierarchy starting with an index
 * value of 0, until the parameters report the value associated with the cache type field is 0. The architecturally defined
 * fields reported by deterministic cache parameters are documented in Table 3-8.
 * This Cache Size in Bytes
 * - = (Ways + 1) * (Partitions + 1) * (Line_Size + 1) * (Sets + 1)
 * - = (EBX[31:22] + 1) * (EBX[21:12] + 1) * (EBX[11:0] + 1) * (ECX + 1)
 * The CPUID leaf 04H also reports data that can be used to derive the topology of processor cores in a physical package.
 * This information is constant for all valid index values. Software can query the raw data reported by executing CPUID
 * with EAX=04H and ECX=0 and use it as part of the topology enumeration algorithm.
 *
 * @see Vol3A[8(Multiple-Processor Management)]
 */
#define CPUID_CACHE_PARAMETERS                                       0x00000004
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 4:0] - 0 = Null - No more caches.
		* - 1 = Data Cache.
		* - 2 = Instruction Cache.
		* - 3 = Unified Cache.
		* - 4-31 = Reserved.
		*/
		uint32_t cache_type_field                                      : 5;
#define CPUID_EAX_CACHE_TYPE_FIELD_BIT                               0
#define CPUID_EAX_CACHE_TYPE_FIELD_FLAG                              0x1F
#define CPUID_EAX_CACHE_TYPE_FIELD_MASK                              0x1F
#define CPUID_EAX_CACHE_TYPE_FIELD(_)                                (((_) >> 0) & 0x1F)

		/**
		* [Bits 7:5] Cache Level (starts at 1).
		*/
		uint32_t cache_level                                           : 3;
#define CPUID_EAX_CACHE_LEVEL_BIT                                    5
#define CPUID_EAX_CACHE_LEVEL_FLAG                                   0xE0
#define CPUID_EAX_CACHE_LEVEL_MASK                                   0x07
#define CPUID_EAX_CACHE_LEVEL(_)                                     (((_) >> 5) & 0x07)

		/**
		* [Bit 8] Self Initializing cache level (does not need SW initialization).
		*/
		uint32_t self_initializing_cache_level                         : 1;
#define CPUID_EAX_SELF_INITIALIZING_CACHE_LEVEL_BIT                  8
#define CPUID_EAX_SELF_INITIALIZING_CACHE_LEVEL_FLAG                 0x100
#define CPUID_EAX_SELF_INITIALIZING_CACHE_LEVEL_MASK                 0x01
#define CPUID_EAX_SELF_INITIALIZING_CACHE_LEVEL(_)                   (((_) >> 8) & 0x01)

		/**
		* [Bit 9] Fully Associative cache.
		*/
		uint32_t fully_associative_cache                               : 1;
#define CPUID_EAX_FULLY_ASSOCIATIVE_CACHE_BIT                        9
#define CPUID_EAX_FULLY_ASSOCIATIVE_CACHE_FLAG                       0x200
#define CPUID_EAX_FULLY_ASSOCIATIVE_CACHE_MASK                       0x01
#define CPUID_EAX_FULLY_ASSOCIATIVE_CACHE(_)                         (((_) >> 9) & 0x01)
		uint32_t reserved1                                             : 4;

		/**
		* [Bits 25:14] Maximum number of addressable IDs for logical processors sharing this cache.
		*
		* @note Add one to the return value to get the result.
		*       The nearest power-of-2 integer that is not smaller than (1 + EAX[25:14]) is the number of unique initial APIC IDs
		*       reserved for addressing different logical processors sharing this cache.
		*/
		uint32_t max_addressable_ids_for_logical_processors_sharing_this_cache: 12;
#define CPUID_EAX_MAX_ADDRESSABLE_IDS_FOR_LOGICAL_PROCESSORS_SHARING_THIS_CACHE_BIT 14
#define CPUID_EAX_MAX_ADDRESSABLE_IDS_FOR_LOGICAL_PROCESSORS_SHARING_THIS_CACHE_FLAG 0x3FFC000
#define CPUID_EAX_MAX_ADDRESSABLE_IDS_FOR_LOGICAL_PROCESSORS_SHARING_THIS_CACHE_MASK 0xFFF
#define CPUID_EAX_MAX_ADDRESSABLE_IDS_FOR_LOGICAL_PROCESSORS_SHARING_THIS_CACHE(_) (((_) >> 14) & 0xFFF)

		/**
		* [Bits 31:26] Maximum number of addressable IDs for processor cores in the physical package.
		*
		* @note Add one to the return value to get the result.
		*       The nearest power-of-2 integer that is not smaller than (1 + EAX[31:26]) is the number of unique Core_IDs reserved for
		*       addressing different processor cores in a physical package. Core ID is a subset of bits of the initial APIC ID.
		*       The returned value is constant for valid initial values in ECX. Valid ECX values start from 0.
		*/
		uint32_t max_addressable_ids_for_processor_cores_in_physical_package: 6;
#define CPUID_EAX_MAX_ADDRESSABLE_IDS_FOR_PROCESSOR_CORES_IN_PHYSICAL_PACKAGE_BIT 26
#define CPUID_EAX_MAX_ADDRESSABLE_IDS_FOR_PROCESSOR_CORES_IN_PHYSICAL_PACKAGE_FLAG 0xFC000000
#define CPUID_EAX_MAX_ADDRESSABLE_IDS_FOR_PROCESSOR_CORES_IN_PHYSICAL_PACKAGE_MASK 0x3F
#define CPUID_EAX_MAX_ADDRESSABLE_IDS_FOR_PROCESSOR_CORES_IN_PHYSICAL_PACKAGE(_) (((_) >> 26) & 0x3F)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 11:0] System Coherency Line Size.
		*
		* @note Add one to the return value to get the result.
		*/
		uint32_t system_coherency_line_size                            : 12;
#define CPUID_EBX_SYSTEM_COHERENCY_LINE_SIZE_BIT                     0
#define CPUID_EBX_SYSTEM_COHERENCY_LINE_SIZE_FLAG                    0xFFF
#define CPUID_EBX_SYSTEM_COHERENCY_LINE_SIZE_MASK                    0xFFF
#define CPUID_EBX_SYSTEM_COHERENCY_LINE_SIZE(_)                      (((_) >> 0) & 0xFFF)

		/**
		* [Bits 21:12] Physical Line partitions.
		*
		* @note Add one to the return value to get the result.
		*/
		uint32_t physical_line_partitions                              : 10;
#define CPUID_EBX_PHYSICAL_LINE_PARTITIONS_BIT                       12
#define CPUID_EBX_PHYSICAL_LINE_PARTITIONS_FLAG                      0x3FF000
#define CPUID_EBX_PHYSICAL_LINE_PARTITIONS_MASK                      0x3FF
#define CPUID_EBX_PHYSICAL_LINE_PARTITIONS(_)                        (((_) >> 12) & 0x3FF)

		/**
		* [Bits 31:22] Ways of associativity.
		*
		* @note Add one to the return value to get the result.
		*/
		uint32_t ways_of_associativity                                 : 10;
#define CPUID_EBX_WAYS_OF_ASSOCIATIVITY_BIT                          22
#define CPUID_EBX_WAYS_OF_ASSOCIATIVITY_FLAG                         0xFFC00000
#define CPUID_EBX_WAYS_OF_ASSOCIATIVITY_MASK                         0x3FF
#define CPUID_EBX_WAYS_OF_ASSOCIATIVITY(_)                           (((_) >> 22) & 0x3FF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Number of Sets.
		*
		* @note Add one to the return value to get the result.
		*/
		uint32_t number_of_sets                                        : 32;
#define CPUID_ECX_NUMBER_OF_SETS_BIT                                 0
#define CPUID_ECX_NUMBER_OF_SETS_FLAG                                0xFFFFFFFF
#define CPUID_ECX_NUMBER_OF_SETS_MASK                                0xFFFFFFFF
#define CPUID_ECX_NUMBER_OF_SETS(_)                                  (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* @brief Write-Back Invalidate/Invalidate
		*
		* [Bit 0] - 0 = WBINVD/INVD from threads sharing this cache acts upon lower level caches for threads sharing this cache.
		* - 1 = WBINVD/INVD is not guaranteed to act upon lower level caches of non-originating threads sharing this cache.
		*/
		uint32_t write_back_invalidate                                 : 1;
#define CPUID_EDX_WRITE_BACK_INVALIDATE_BIT                          0
#define CPUID_EDX_WRITE_BACK_INVALIDATE_FLAG                         0x01
#define CPUID_EDX_WRITE_BACK_INVALIDATE_MASK                         0x01
#define CPUID_EDX_WRITE_BACK_INVALIDATE(_)                           (((_) >> 0) & 0x01)

		/**
		* @brief Cache Inclusiveness
		*
		* [Bit 1] - 0 = Cache is not inclusive of lower cache levels.
		* - 1 = Cache is inclusive of lower cache levels.
		*/
		uint32_t cache_inclusiveness                                   : 1;
#define CPUID_EDX_CACHE_INCLUSIVENESS_BIT                            1
#define CPUID_EDX_CACHE_INCLUSIVENESS_FLAG                           0x02
#define CPUID_EDX_CACHE_INCLUSIVENESS_MASK                           0x01
#define CPUID_EDX_CACHE_INCLUSIVENESS(_)                             (((_) >> 1) & 0x01)

		/**
		* @brief Complex Cache Indexing
		*
		* [Bit 2] - 0 = Direct mapped cache.
		* - 1 = A complex function is used to index the cache, potentially using all address bits.
		*/
		uint32_t complex_cache_indexing                                : 1;
#define CPUID_EDX_COMPLEX_CACHE_INDEXING_BIT                         2
#define CPUID_EDX_COMPLEX_CACHE_INDEXING_FLAG                        0x04
#define CPUID_EDX_COMPLEX_CACHE_INDEXING_MASK                        0x01
#define CPUID_EDX_COMPLEX_CACHE_INDEXING(_)                          (((_) >> 2) & 0x01)
		uint32_t reserved1                                             : 29;
	};

	uint32_t flags;
  } edx;

} cpuid_eax_04;


/**
 * @brief MONITOR/MWAIT Leaf
 *
 * When CPUID executes with EAX set to 05H, the processor returns information about features available to MONITOR/MWAIT
 * instructions. The MONITOR instruction is used for address-range monitoring in conjunction with MWAIT instruction. The
 * MWAIT instruction optionally provides additional extensions for advanced power management.
 */
#define CPUID_MONITOR_MWAIT                                          0x00000005
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 15:0] Smallest monitor-line size in bytes (default is processor's monitor granularity).
		*/
		uint32_t smallest_monitor_line_size                            : 16;
#define CPUID_EAX_SMALLEST_MONITOR_LINE_SIZE_BIT                     0
#define CPUID_EAX_SMALLEST_MONITOR_LINE_SIZE_FLAG                    0xFFFF
#define CPUID_EAX_SMALLEST_MONITOR_LINE_SIZE_MASK                    0xFFFF
#define CPUID_EAX_SMALLEST_MONITOR_LINE_SIZE(_)                      (((_) >> 0) & 0xFFFF)
		uint32_t reserved1                                             : 16;
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 15:0] Largest monitor-line size in bytes (default is processor's monitor granularity).
		*/
		uint32_t largest_monitor_line_size                             : 16;
#define CPUID_EBX_LARGEST_MONITOR_LINE_SIZE_BIT                      0
#define CPUID_EBX_LARGEST_MONITOR_LINE_SIZE_FLAG                     0xFFFF
#define CPUID_EBX_LARGEST_MONITOR_LINE_SIZE_MASK                     0xFFFF
#define CPUID_EBX_LARGEST_MONITOR_LINE_SIZE(_)                       (((_) >> 0) & 0xFFFF)
		uint32_t reserved1                                             : 16;
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bit 0] Enumeration of Monitor-Mwait extensions (beyond EAX and EBX registers) supported.
		*/
		uint32_t enumeration_of_monitor_mwait_extensions               : 1;
#define CPUID_ECX_ENUMERATION_OF_MONITOR_MWAIT_EXTENSIONS_BIT        0
#define CPUID_ECX_ENUMERATION_OF_MONITOR_MWAIT_EXTENSIONS_FLAG       0x01
#define CPUID_ECX_ENUMERATION_OF_MONITOR_MWAIT_EXTENSIONS_MASK       0x01
#define CPUID_ECX_ENUMERATION_OF_MONITOR_MWAIT_EXTENSIONS(_)         (((_) >> 0) & 0x01)

		/**
		* [Bit 1] Supports treating interrupts as break-event for MWAIT, even when interrupts disabled.
		*/
		uint32_t supports_treating_interrupts_as_break_event_for_mwait : 1;
#define CPUID_ECX_SUPPORTS_TREATING_INTERRUPTS_AS_BREAK_EVENT_FOR_MWAIT_BIT 1
#define CPUID_ECX_SUPPORTS_TREATING_INTERRUPTS_AS_BREAK_EVENT_FOR_MWAIT_FLAG 0x02
#define CPUID_ECX_SUPPORTS_TREATING_INTERRUPTS_AS_BREAK_EVENT_FOR_MWAIT_MASK 0x01
#define CPUID_ECX_SUPPORTS_TREATING_INTERRUPTS_AS_BREAK_EVENT_FOR_MWAIT(_) (((_) >> 1) & 0x01)
		uint32_t reserved1                                             : 30;
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 3:0] Number of C0 sub C-states supported using MWAIT.
		*/
		uint32_t number_of_c0_sub_c_states                             : 4;
#define CPUID_EDX_NUMBER_OF_C0_SUB_C_STATES_BIT                      0
#define CPUID_EDX_NUMBER_OF_C0_SUB_C_STATES_FLAG                     0x0F
#define CPUID_EDX_NUMBER_OF_C0_SUB_C_STATES_MASK                     0x0F
#define CPUID_EDX_NUMBER_OF_C0_SUB_C_STATES(_)                       (((_) >> 0) & 0x0F)

		/**
		* [Bits 7:4] Number of C1 sub C-states supported using MWAIT.
		*/
		uint32_t number_of_c1_sub_c_states                             : 4;
#define CPUID_EDX_NUMBER_OF_C1_SUB_C_STATES_BIT                      4
#define CPUID_EDX_NUMBER_OF_C1_SUB_C_STATES_FLAG                     0xF0
#define CPUID_EDX_NUMBER_OF_C1_SUB_C_STATES_MASK                     0x0F
#define CPUID_EDX_NUMBER_OF_C1_SUB_C_STATES(_)                       (((_) >> 4) & 0x0F)

		/**
		* [Bits 11:8] Number of C2 sub C-states supported using MWAIT.
		*/
		uint32_t number_of_c2_sub_c_states                             : 4;
#define CPUID_EDX_NUMBER_OF_C2_SUB_C_STATES_BIT                      8
#define CPUID_EDX_NUMBER_OF_C2_SUB_C_STATES_FLAG                     0xF00
#define CPUID_EDX_NUMBER_OF_C2_SUB_C_STATES_MASK                     0x0F
#define CPUID_EDX_NUMBER_OF_C2_SUB_C_STATES(_)                       (((_) >> 8) & 0x0F)

		/**
		* [Bits 15:12] Number of C3 sub C-states supported using MWAIT.
		*/
		uint32_t number_of_c3_sub_c_states                             : 4;
#define CPUID_EDX_NUMBER_OF_C3_SUB_C_STATES_BIT                      12
#define CPUID_EDX_NUMBER_OF_C3_SUB_C_STATES_FLAG                     0xF000
#define CPUID_EDX_NUMBER_OF_C3_SUB_C_STATES_MASK                     0x0F
#define CPUID_EDX_NUMBER_OF_C3_SUB_C_STATES(_)                       (((_) >> 12) & 0x0F)

		/**
		* [Bits 19:16] Number of C4 sub C-states supported using MWAIT.
		*/
		uint32_t number_of_c4_sub_c_states                             : 4;
#define CPUID_EDX_NUMBER_OF_C4_SUB_C_STATES_BIT                      16
#define CPUID_EDX_NUMBER_OF_C4_SUB_C_STATES_FLAG                     0xF0000
#define CPUID_EDX_NUMBER_OF_C4_SUB_C_STATES_MASK                     0x0F
#define CPUID_EDX_NUMBER_OF_C4_SUB_C_STATES(_)                       (((_) >> 16) & 0x0F)

		/**
		* [Bits 23:20] Number of C5 sub C-states supported using MWAIT.
		*/
		uint32_t number_of_c5_sub_c_states                             : 4;
#define CPUID_EDX_NUMBER_OF_C5_SUB_C_STATES_BIT                      20
#define CPUID_EDX_NUMBER_OF_C5_SUB_C_STATES_FLAG                     0xF00000
#define CPUID_EDX_NUMBER_OF_C5_SUB_C_STATES_MASK                     0x0F
#define CPUID_EDX_NUMBER_OF_C5_SUB_C_STATES(_)                       (((_) >> 20) & 0x0F)

		/**
		* [Bits 27:24] Number of C6 sub C-states supported using MWAIT.
		*/
		uint32_t number_of_c6_sub_c_states                             : 4;
#define CPUID_EDX_NUMBER_OF_C6_SUB_C_STATES_BIT                      24
#define CPUID_EDX_NUMBER_OF_C6_SUB_C_STATES_FLAG                     0xF000000
#define CPUID_EDX_NUMBER_OF_C6_SUB_C_STATES_MASK                     0x0F
#define CPUID_EDX_NUMBER_OF_C6_SUB_C_STATES(_)                       (((_) >> 24) & 0x0F)

		/**
		* [Bits 31:28] Number of C7 sub C-states supported using MWAIT.
		*/
		uint32_t number_of_c7_sub_c_states                             : 4;
#define CPUID_EDX_NUMBER_OF_C7_SUB_C_STATES_BIT                      28
#define CPUID_EDX_NUMBER_OF_C7_SUB_C_STATES_FLAG                     0xF0000000
#define CPUID_EDX_NUMBER_OF_C7_SUB_C_STATES_MASK                     0x0F
#define CPUID_EDX_NUMBER_OF_C7_SUB_C_STATES(_)                       (((_) >> 28) & 0x0F)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_05;


/**
 * @brief Thermal and Power Management Leaf
 *
 * When CPUID executes with EAX set to 06H, the processor returns information about thermal and power management features.
 */
#define CPUID_THERMAL_AND_POWER_MANAGEMENT                           0x00000006
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bit 0] Digital temperature sensor is supported if set.
		*/
		uint32_t temperature_sensor_supported                          : 1;
#define CPUID_EAX_TEMPERATURE_SENSOR_SUPPORTED_BIT                   0
#define CPUID_EAX_TEMPERATURE_SENSOR_SUPPORTED_FLAG                  0x01
#define CPUID_EAX_TEMPERATURE_SENSOR_SUPPORTED_MASK                  0x01
#define CPUID_EAX_TEMPERATURE_SENSOR_SUPPORTED(_)                    (((_) >> 0) & 0x01)

		/**
		* [Bit 1] Intel Turbo Boost Technology available (see description of IA32_MISC_ENABLE[38]).
		*/
		uint32_t intel_turbo_boost_technology_available                : 1;
#define CPUID_EAX_INTEL_TURBO_BOOST_TECHNOLOGY_AVAILABLE_BIT         1
#define CPUID_EAX_INTEL_TURBO_BOOST_TECHNOLOGY_AVAILABLE_FLAG        0x02
#define CPUID_EAX_INTEL_TURBO_BOOST_TECHNOLOGY_AVAILABLE_MASK        0x01
#define CPUID_EAX_INTEL_TURBO_BOOST_TECHNOLOGY_AVAILABLE(_)          (((_) >> 1) & 0x01)

		/**
		* [Bit 2] ARAT. APIC-Timer-always-running feature is supported if set.
		*/
		uint32_t apic_timer_always_running                             : 1;
#define CPUID_EAX_APIC_TIMER_ALWAYS_RUNNING_BIT                      2
#define CPUID_EAX_APIC_TIMER_ALWAYS_RUNNING_FLAG                     0x04
#define CPUID_EAX_APIC_TIMER_ALWAYS_RUNNING_MASK                     0x01
#define CPUID_EAX_APIC_TIMER_ALWAYS_RUNNING(_)                       (((_) >> 2) & 0x01)
		uint32_t reserved1                                             : 1;

		/**
		* [Bit 4] PLN. Power limit notification controls are supported if set.
		*/
		uint32_t power_limit_notification                              : 1;
#define CPUID_EAX_POWER_LIMIT_NOTIFICATION_BIT                       4
#define CPUID_EAX_POWER_LIMIT_NOTIFICATION_FLAG                      0x10
#define CPUID_EAX_POWER_LIMIT_NOTIFICATION_MASK                      0x01
#define CPUID_EAX_POWER_LIMIT_NOTIFICATION(_)                        (((_) >> 4) & 0x01)

		/**
		* [Bit 5] ECMD. Clock modulation duty cycle extension is supported if set.
		*/
		uint32_t clock_modulation_duty                                 : 1;
#define CPUID_EAX_CLOCK_MODULATION_DUTY_BIT                          5
#define CPUID_EAX_CLOCK_MODULATION_DUTY_FLAG                         0x20
#define CPUID_EAX_CLOCK_MODULATION_DUTY_MASK                         0x01
#define CPUID_EAX_CLOCK_MODULATION_DUTY(_)                           (((_) >> 5) & 0x01)

		/**
		* [Bit 6] PTM. Package thermal management is supported if set.
		*/
		uint32_t package_thermal_management                            : 1;
#define CPUID_EAX_PACKAGE_THERMAL_MANAGEMENT_BIT                     6
#define CPUID_EAX_PACKAGE_THERMAL_MANAGEMENT_FLAG                    0x40
#define CPUID_EAX_PACKAGE_THERMAL_MANAGEMENT_MASK                    0x01
#define CPUID_EAX_PACKAGE_THERMAL_MANAGEMENT(_)                      (((_) >> 6) & 0x01)

		/**
		* [Bit 7] HWP. HWP base registers (IA32_PM_ENABLE[bit 0], IA32_HWP_CAPABILITIES, IA32_HWP_REQUEST, IA32_HWP_STATUS) are
		* supported if set.
		*/
		uint32_t hwp_base_registers                                    : 1;
#define CPUID_EAX_HWP_BASE_REGISTERS_BIT                             7
#define CPUID_EAX_HWP_BASE_REGISTERS_FLAG                            0x80
#define CPUID_EAX_HWP_BASE_REGISTERS_MASK                            0x01
#define CPUID_EAX_HWP_BASE_REGISTERS(_)                              (((_) >> 7) & 0x01)

		/**
		* [Bit 8] HWP_Notification. IA32_HWP_INTERRUPT MSR is supported if set.
		*/
		uint32_t hwp_notification                                      : 1;
#define CPUID_EAX_HWP_NOTIFICATION_BIT                               8
#define CPUID_EAX_HWP_NOTIFICATION_FLAG                              0x100
#define CPUID_EAX_HWP_NOTIFICATION_MASK                              0x01
#define CPUID_EAX_HWP_NOTIFICATION(_)                                (((_) >> 8) & 0x01)

		/**
		* [Bit 9] HWP_Activity_Window. IA32_HWP_REQUEST[bits 41:32] is supported if set.
		*/
		uint32_t hwp_activity_window                                   : 1;
#define CPUID_EAX_HWP_ACTIVITY_WINDOW_BIT                            9
#define CPUID_EAX_HWP_ACTIVITY_WINDOW_FLAG                           0x200
#define CPUID_EAX_HWP_ACTIVITY_WINDOW_MASK                           0x01
#define CPUID_EAX_HWP_ACTIVITY_WINDOW(_)                             (((_) >> 9) & 0x01)

		/**
		* [Bit 10] HWP_Energy_Performance_Preference. IA32_HWP_REQUEST[bits 31:24] is supported if set.
		*/
		uint32_t hwp_energy_performance_preference                     : 1;
#define CPUID_EAX_HWP_ENERGY_PERFORMANCE_PREFERENCE_BIT              10
#define CPUID_EAX_HWP_ENERGY_PERFORMANCE_PREFERENCE_FLAG             0x400
#define CPUID_EAX_HWP_ENERGY_PERFORMANCE_PREFERENCE_MASK             0x01
#define CPUID_EAX_HWP_ENERGY_PERFORMANCE_PREFERENCE(_)               (((_) >> 10) & 0x01)

		/**
		* [Bit 11] HWP_Package_Level_Request. IA32_HWP_REQUEST_PKG MSR is supported if set.
		*/
		uint32_t hwp_package_level_request                             : 1;
#define CPUID_EAX_HWP_PACKAGE_LEVEL_REQUEST_BIT                      11
#define CPUID_EAX_HWP_PACKAGE_LEVEL_REQUEST_FLAG                     0x800
#define CPUID_EAX_HWP_PACKAGE_LEVEL_REQUEST_MASK                     0x01
#define CPUID_EAX_HWP_PACKAGE_LEVEL_REQUEST(_)                       (((_) >> 11) & 0x01)
		uint32_t reserved2                                             : 1;

		/**
		* [Bit 13] HDC. HDC base registers IA32_PKG_HDC_CTL, IA32_PM_CTL1, IA32_THREAD_STALL MSRs are supported if set.
		*/
		uint32_t hdc                                                   : 1;
#define CPUID_EAX_HDC_BIT                                            13
#define CPUID_EAX_HDC_FLAG                                           0x2000
#define CPUID_EAX_HDC_MASK                                           0x01
#define CPUID_EAX_HDC(_)                                             (((_) >> 13) & 0x01)

		/**
		* [Bit 14] Intel(R) Turbo Boost Max Technology 3.0 available.
		*/
		uint32_t intel_turbo_boost_max_technology_3_available          : 1;
#define CPUID_EAX_INTEL_TURBO_BOOST_MAX_TECHNOLOGY_3_AVAILABLE_BIT   14
#define CPUID_EAX_INTEL_TURBO_BOOST_MAX_TECHNOLOGY_3_AVAILABLE_FLAG  0x4000
#define CPUID_EAX_INTEL_TURBO_BOOST_MAX_TECHNOLOGY_3_AVAILABLE_MASK  0x01
#define CPUID_EAX_INTEL_TURBO_BOOST_MAX_TECHNOLOGY_3_AVAILABLE(_)    (((_) >> 14) & 0x01)

		/**
		* [Bit 15] HWP Capabilities. Highest Performance change is supported if set.
		*/
		uint32_t hwp_capabilities                                      : 1;
#define CPUID_EAX_HWP_CAPABILITIES_BIT                               15
#define CPUID_EAX_HWP_CAPABILITIES_FLAG                              0x8000
#define CPUID_EAX_HWP_CAPABILITIES_MASK                              0x01
#define CPUID_EAX_HWP_CAPABILITIES(_)                                (((_) >> 15) & 0x01)

		/**
		* [Bit 16] HWP PECI override is supported if set.
		*/
		uint32_t hwp_peci_override                                     : 1;
#define CPUID_EAX_HWP_PECI_OVERRIDE_BIT                              16
#define CPUID_EAX_HWP_PECI_OVERRIDE_FLAG                             0x10000
#define CPUID_EAX_HWP_PECI_OVERRIDE_MASK                             0x01
#define CPUID_EAX_HWP_PECI_OVERRIDE(_)                               (((_) >> 16) & 0x01)

		/**
		* [Bit 17] Flexible HWP is supported if set.
		*/
		uint32_t flexible_hwp                                          : 1;
#define CPUID_EAX_FLEXIBLE_HWP_BIT                                   17
#define CPUID_EAX_FLEXIBLE_HWP_FLAG                                  0x20000
#define CPUID_EAX_FLEXIBLE_HWP_MASK                                  0x01
#define CPUID_EAX_FLEXIBLE_HWP(_)                                    (((_) >> 17) & 0x01)

		/**
		* [Bit 18] Fast access mode for the IA32_HWP_REQUEST MSR is supported if set.
		*/
		uint32_t fast_access_mode_for_hwp_request_msr                  : 1;
#define CPUID_EAX_FAST_ACCESS_MODE_FOR_HWP_REQUEST_MSR_BIT           18
#define CPUID_EAX_FAST_ACCESS_MODE_FOR_HWP_REQUEST_MSR_FLAG          0x40000
#define CPUID_EAX_FAST_ACCESS_MODE_FOR_HWP_REQUEST_MSR_MASK          0x01
#define CPUID_EAX_FAST_ACCESS_MODE_FOR_HWP_REQUEST_MSR(_)            (((_) >> 18) & 0x01)
		uint32_t reserved3                                             : 1;

		/**
		* [Bit 20] Ignoring Idle Logical Processor HWP request is supported if set.
		*/
		uint32_t ignoring_idle_logical_processor_hwp_request           : 1;
#define CPUID_EAX_IGNORING_IDLE_LOGICAL_PROCESSOR_HWP_REQUEST_BIT    20
#define CPUID_EAX_IGNORING_IDLE_LOGICAL_PROCESSOR_HWP_REQUEST_FLAG   0x100000
#define CPUID_EAX_IGNORING_IDLE_LOGICAL_PROCESSOR_HWP_REQUEST_MASK   0x01
#define CPUID_EAX_IGNORING_IDLE_LOGICAL_PROCESSOR_HWP_REQUEST(_)     (((_) >> 20) & 0x01)
		uint32_t reserved4                                             : 11;
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 3:0] Number of Interrupt Thresholds in Digital Thermal Sensor.
		*/
		uint32_t number_of_interrupt_thresholds_in_thermal_sensor      : 4;
#define CPUID_EBX_NUMBER_OF_INTERRUPT_THRESHOLDS_IN_THERMAL_SENSOR_BIT 0
#define CPUID_EBX_NUMBER_OF_INTERRUPT_THRESHOLDS_IN_THERMAL_SENSOR_FLAG 0x0F
#define CPUID_EBX_NUMBER_OF_INTERRUPT_THRESHOLDS_IN_THERMAL_SENSOR_MASK 0x0F
#define CPUID_EBX_NUMBER_OF_INTERRUPT_THRESHOLDS_IN_THERMAL_SENSOR(_) (((_) >> 0) & 0x0F)
		uint32_t reserved1                                             : 28;
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bit 0] Hardware Coordination Feedback Capability (Presence of IA32_MPERF and IA32_APERF). The capability to provide a
		* measure of delivered processor performance (since last reset of the counters), as a percentage of the expected processor
		* performance when running at the TSC frequency.
		*/
		uint32_t hardware_coordination_feedback_capability             : 1;
#define CPUID_ECX_HARDWARE_COORDINATION_FEEDBACK_CAPABILITY_BIT      0
#define CPUID_ECX_HARDWARE_COORDINATION_FEEDBACK_CAPABILITY_FLAG     0x01
#define CPUID_ECX_HARDWARE_COORDINATION_FEEDBACK_CAPABILITY_MASK     0x01
#define CPUID_ECX_HARDWARE_COORDINATION_FEEDBACK_CAPABILITY(_)       (((_) >> 0) & 0x01)
		uint32_t reserved1                                             : 2;

		/**
		* [Bit 3] The processor supports performance-energy bias preference if CPUID.06H:ECX.SETBH[bit 3] is set and it also
		* implies the presence of a new architectural MSR called IA32_ENERGY_PERF_BIAS (1B0H).
		*/
		uint32_t performance_energy_bias_preference                    : 1;
#define CPUID_ECX_PERFORMANCE_ENERGY_BIAS_PREFERENCE_BIT             3
#define CPUID_ECX_PERFORMANCE_ENERGY_BIAS_PREFERENCE_FLAG            0x08
#define CPUID_ECX_PERFORMANCE_ENERGY_BIAS_PREFERENCE_MASK            0x01
#define CPUID_ECX_PERFORMANCE_ENERGY_BIAS_PREFERENCE(_)              (((_) >> 3) & 0x01)
		uint32_t reserved2                                             : 28;
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EDX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EDX_RESERVED_BIT                                       0
#define CPUID_EDX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_06;


/**
 * @brief Structured Extended Feature Flags Enumeration Leaf (Output depends on ECX input value)
 *
 * When CPUID executes with EAX set to 07H and ECX = 0, the processor returns information about the maximum input value for
 * sub-leaves that contain extended feature flags.
 * When CPUID executes with EAX set to 07H and the input value of ECX is invalid (see leaf 07H entry in Table 3-8), the
 * processor returns 0 in EAX/EBX/ECX/EDX. In subleaf 0, EAX returns the maximum input value of the highest leaf 7
 * sub-leaf, and EBX, ECX & EDX contain information of extended feature flags.
 */
#define CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS                      0x00000007
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] Reports the maximum input value for supported leaf 7 sub-leaves.
		*/
		uint32_t number_of_sub_leaves                                  : 32;
#define CPUID_EAX_NUMBER_OF_SUB_LEAVES_BIT                           0
#define CPUID_EAX_NUMBER_OF_SUB_LEAVES_FLAG                          0xFFFFFFFF
#define CPUID_EAX_NUMBER_OF_SUB_LEAVES_MASK                          0xFFFFFFFF
#define CPUID_EAX_NUMBER_OF_SUB_LEAVES(_)                            (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bit 0] Supports RDFSBASE/RDGSBASE/WRFSBASE/WRGSBASE if 1.
		*/
		uint32_t fsgsbase                                              : 1;
#define CPUID_EBX_FSGSBASE_BIT                                       0
#define CPUID_EBX_FSGSBASE_FLAG                                      0x01
#define CPUID_EBX_FSGSBASE_MASK                                      0x01
#define CPUID_EBX_FSGSBASE(_)                                        (((_) >> 0) & 0x01)

		/**
		* [Bit 1] IA32_TSC_ADJUST MSR is supported if 1.
		*/
		uint32_t ia32_tsc_adjust_msr                                   : 1;
#define CPUID_EBX_IA32_TSC_ADJUST_MSR_BIT                            1
#define CPUID_EBX_IA32_TSC_ADJUST_MSR_FLAG                           0x02
#define CPUID_EBX_IA32_TSC_ADJUST_MSR_MASK                           0x01
#define CPUID_EBX_IA32_TSC_ADJUST_MSR(_)                             (((_) >> 1) & 0x01)

		/**
		* [Bit 2] Supports Intel(R) Software Guard Extensions (Intel(R) SGX Extensions) if 1.
		*/
		uint32_t sgx                                                   : 1;
#define CPUID_EBX_SGX_BIT                                            2
#define CPUID_EBX_SGX_FLAG                                           0x04
#define CPUID_EBX_SGX_MASK                                           0x01
#define CPUID_EBX_SGX(_)                                             (((_) >> 2) & 0x01)

		/**
		* [Bit 3] BMI1.
		*/
		uint32_t bmi1                                                  : 1;
#define CPUID_EBX_BMI1_BIT                                           3
#define CPUID_EBX_BMI1_FLAG                                          0x08
#define CPUID_EBX_BMI1_MASK                                          0x01
#define CPUID_EBX_BMI1(_)                                            (((_) >> 3) & 0x01)

		/**
		* [Bit 4] HLE.
		*/
		uint32_t hle                                                   : 1;
#define CPUID_EBX_HLE_BIT                                            4
#define CPUID_EBX_HLE_FLAG                                           0x10
#define CPUID_EBX_HLE_MASK                                           0x01
#define CPUID_EBX_HLE(_)                                             (((_) >> 4) & 0x01)

		/**
		* [Bit 5] AVX2.
		*/
		uint32_t avx2                                                  : 1;
#define CPUID_EBX_AVX2_BIT                                           5
#define CPUID_EBX_AVX2_FLAG                                          0x20
#define CPUID_EBX_AVX2_MASK                                          0x01
#define CPUID_EBX_AVX2(_)                                            (((_) >> 5) & 0x01)

		/**
		* [Bit 6] x87 FPU Data Pointer updated only on x87 exceptions if 1.
		*/
		uint32_t fdp_excptn_only                                       : 1;
#define CPUID_EBX_FDP_EXCPTN_ONLY_BIT                                6
#define CPUID_EBX_FDP_EXCPTN_ONLY_FLAG                               0x40
#define CPUID_EBX_FDP_EXCPTN_ONLY_MASK                               0x01
#define CPUID_EBX_FDP_EXCPTN_ONLY(_)                                 (((_) >> 6) & 0x01)

		/**
		* [Bit 7] Supports Supervisor-Mode Execution Prevention if 1.
		*/
		uint32_t smep                                                  : 1;
#define CPUID_EBX_SMEP_BIT                                           7
#define CPUID_EBX_SMEP_FLAG                                          0x80
#define CPUID_EBX_SMEP_MASK                                          0x01
#define CPUID_EBX_SMEP(_)                                            (((_) >> 7) & 0x01)

		/**
		* [Bit 8] BMI2.
		*/
		uint32_t bmi2                                                  : 1;
#define CPUID_EBX_BMI2_BIT                                           8
#define CPUID_EBX_BMI2_FLAG                                          0x100
#define CPUID_EBX_BMI2_MASK                                          0x01
#define CPUID_EBX_BMI2(_)                                            (((_) >> 8) & 0x01)

		/**
		* [Bit 9] Supports Enhanced REP MOVSB/STOSB if 1.
		*/
		uint32_t enhanced_rep_movsb_stosb                              : 1;
#define CPUID_EBX_ENHANCED_REP_MOVSB_STOSB_BIT                       9
#define CPUID_EBX_ENHANCED_REP_MOVSB_STOSB_FLAG                      0x200
#define CPUID_EBX_ENHANCED_REP_MOVSB_STOSB_MASK                      0x01
#define CPUID_EBX_ENHANCED_REP_MOVSB_STOSB(_)                        (((_) >> 9) & 0x01)

		/**
		* [Bit 10] If 1, supports INVPCID instruction for system software that manages process-context identifiers.
		*/
		uint32_t invpcid                                               : 1;
#define CPUID_EBX_INVPCID_BIT                                        10
#define CPUID_EBX_INVPCID_FLAG                                       0x400
#define CPUID_EBX_INVPCID_MASK                                       0x01
#define CPUID_EBX_INVPCID(_)                                         (((_) >> 10) & 0x01)

		/**
		* [Bit 11] RTM.
		*/
		uint32_t rtm                                                   : 1;
#define CPUID_EBX_RTM_BIT                                            11
#define CPUID_EBX_RTM_FLAG                                           0x800
#define CPUID_EBX_RTM_MASK                                           0x01
#define CPUID_EBX_RTM(_)                                             (((_) >> 11) & 0x01)

		/**
		* [Bit 12] Supports Intel(R) Resource Director Technology (Intel(R) RDT) Monitoring capability if 1.
		*/
		uint32_t rdt_m                                                 : 1;
#define CPUID_EBX_RDT_M_BIT                                          12
#define CPUID_EBX_RDT_M_FLAG                                         0x1000
#define CPUID_EBX_RDT_M_MASK                                         0x01
#define CPUID_EBX_RDT_M(_)                                           (((_) >> 12) & 0x01)

		/**
		* [Bit 13] Deprecates FPU CS and FPU DS values if 1.
		*/
		uint32_t deprecates                                            : 1;
#define CPUID_EBX_DEPRECATES_BIT                                     13
#define CPUID_EBX_DEPRECATES_FLAG                                    0x2000
#define CPUID_EBX_DEPRECATES_MASK                                    0x01
#define CPUID_EBX_DEPRECATES(_)                                      (((_) >> 13) & 0x01)

		/**
		* [Bit 14] Supports Intel(R) Memory Protection Extensions if 1.
		*/
		uint32_t mpx                                                   : 1;
#define CPUID_EBX_MPX_BIT                                            14
#define CPUID_EBX_MPX_FLAG                                           0x4000
#define CPUID_EBX_MPX_MASK                                           0x01
#define CPUID_EBX_MPX(_)                                             (((_) >> 14) & 0x01)

		/**
		* [Bit 15] Supports Intel(R) Resource Director Technology (Intel(R) RDT) Allocation capability if 1.
		*/
		uint32_t rdt                                                   : 1;
#define CPUID_EBX_RDT_BIT                                            15
#define CPUID_EBX_RDT_FLAG                                           0x8000
#define CPUID_EBX_RDT_MASK                                           0x01
#define CPUID_EBX_RDT(_)                                             (((_) >> 15) & 0x01)

		/**
		* [Bit 16] AVX512F.
		*/
		uint32_t avx512f                                               : 1;
#define CPUID_EBX_AVX512F_BIT                                        16
#define CPUID_EBX_AVX512F_FLAG                                       0x10000
#define CPUID_EBX_AVX512F_MASK                                       0x01
#define CPUID_EBX_AVX512F(_)                                         (((_) >> 16) & 0x01)

		/**
		* [Bit 17] AVX512DQ.
		*/
		uint32_t avx512dq                                              : 1;
#define CPUID_EBX_AVX512DQ_BIT                                       17
#define CPUID_EBX_AVX512DQ_FLAG                                      0x20000
#define CPUID_EBX_AVX512DQ_MASK                                      0x01
#define CPUID_EBX_AVX512DQ(_)                                        (((_) >> 17) & 0x01)

		/**
		* [Bit 18] RDSEED.
		*/
		uint32_t rdseed                                                : 1;
#define CPUID_EBX_RDSEED_BIT                                         18
#define CPUID_EBX_RDSEED_FLAG                                        0x40000
#define CPUID_EBX_RDSEED_MASK                                        0x01
#define CPUID_EBX_RDSEED(_)                                          (((_) >> 18) & 0x01)

		/**
		* [Bit 19] ADX.
		*/
		uint32_t adx                                                   : 1;
#define CPUID_EBX_ADX_BIT                                            19
#define CPUID_EBX_ADX_FLAG                                           0x80000
#define CPUID_EBX_ADX_MASK                                           0x01
#define CPUID_EBX_ADX(_)                                             (((_) >> 19) & 0x01)

		/**
		* [Bit 20] Supports Supervisor-Mode Access Prevention (and the CLAC/STAC instructions) if 1.
		*/
		uint32_t smap                                                  : 1;
#define CPUID_EBX_SMAP_BIT                                           20
#define CPUID_EBX_SMAP_FLAG                                          0x100000
#define CPUID_EBX_SMAP_MASK                                          0x01
#define CPUID_EBX_SMAP(_)                                            (((_) >> 20) & 0x01)

		/**
		* [Bit 21] AVX512_IFMA.
		*/
		uint32_t avx512_ifma                                           : 1;
#define CPUID_EBX_AVX512_IFMA_BIT                                    21
#define CPUID_EBX_AVX512_IFMA_FLAG                                   0x200000
#define CPUID_EBX_AVX512_IFMA_MASK                                   0x01
#define CPUID_EBX_AVX512_IFMA(_)                                     (((_) >> 21) & 0x01)
		uint32_t reserved1                                             : 1;

		/**
		* [Bit 23] CLFLUSHOPT.
		*/
		uint32_t clflushopt                                            : 1;
#define CPUID_EBX_CLFLUSHOPT_BIT                                     23
#define CPUID_EBX_CLFLUSHOPT_FLAG                                    0x800000
#define CPUID_EBX_CLFLUSHOPT_MASK                                    0x01
#define CPUID_EBX_CLFLUSHOPT(_)                                      (((_) >> 23) & 0x01)

		/**
		* [Bit 24] CLWB.
		*/
		uint32_t clwb                                                  : 1;
#define CPUID_EBX_CLWB_BIT                                           24
#define CPUID_EBX_CLWB_FLAG                                          0x1000000
#define CPUID_EBX_CLWB_MASK                                          0x01
#define CPUID_EBX_CLWB(_)                                            (((_) >> 24) & 0x01)

		/**
		* [Bit 25] Intel Processor Trace.
		*/
		uint32_t intel                                                 : 1;
#define CPUID_EBX_INTEL_BIT                                          25
#define CPUID_EBX_INTEL_FLAG                                         0x2000000
#define CPUID_EBX_INTEL_MASK                                         0x01
#define CPUID_EBX_INTEL(_)                                           (((_) >> 25) & 0x01)

		/**
		* [Bit 26] (Intel(R) Xeon Phi(TM) only).
		*/
		uint32_t avx512pf                                              : 1;
#define CPUID_EBX_AVX512PF_BIT                                       26
#define CPUID_EBX_AVX512PF_FLAG                                      0x4000000
#define CPUID_EBX_AVX512PF_MASK                                      0x01
#define CPUID_EBX_AVX512PF(_)                                        (((_) >> 26) & 0x01)

		/**
		* [Bit 27] (Intel(R) Xeon Phi(TM) only).
		*/
		uint32_t avx512er                                              : 1;
#define CPUID_EBX_AVX512ER_BIT                                       27
#define CPUID_EBX_AVX512ER_FLAG                                      0x8000000
#define CPUID_EBX_AVX512ER_MASK                                      0x01
#define CPUID_EBX_AVX512ER(_)                                        (((_) >> 27) & 0x01)

		/**
		* [Bit 28] AVX512CD.
		*/
		uint32_t avx512cd                                              : 1;
#define CPUID_EBX_AVX512CD_BIT                                       28
#define CPUID_EBX_AVX512CD_FLAG                                      0x10000000
#define CPUID_EBX_AVX512CD_MASK                                      0x01
#define CPUID_EBX_AVX512CD(_)                                        (((_) >> 28) & 0x01)

		/**
		* [Bit 29] Supports Intel(R) Secure Hash Algorithm Extensions (Intel(R) SHA Extensions) if 1.
		*/
		uint32_t sha                                                   : 1;
#define CPUID_EBX_SHA_BIT                                            29
#define CPUID_EBX_SHA_FLAG                                           0x20000000
#define CPUID_EBX_SHA_MASK                                           0x01
#define CPUID_EBX_SHA(_)                                             (((_) >> 29) & 0x01)

		/**
		* [Bit 30] AVX512BW.
		*/
		uint32_t avx512bw                                              : 1;
#define CPUID_EBX_AVX512BW_BIT                                       30
#define CPUID_EBX_AVX512BW_FLAG                                      0x40000000
#define CPUID_EBX_AVX512BW_MASK                                      0x01
#define CPUID_EBX_AVX512BW(_)                                        (((_) >> 30) & 0x01)

		/**
		* [Bit 31] AVX512VL.
		*/
		uint32_t avx512vl                                              : 1;
#define CPUID_EBX_AVX512VL_BIT                                       31
#define CPUID_EBX_AVX512VL_FLAG                                      0x80000000
#define CPUID_EBX_AVX512VL_MASK                                      0x01
#define CPUID_EBX_AVX512VL(_)                                        (((_) >> 31) & 0x01)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bit 0] (Intel(R) Xeon Phi(TM) only).
		*/
		uint32_t prefetchwt1                                           : 1;
#define CPUID_ECX_PREFETCHWT1_BIT                                    0
#define CPUID_ECX_PREFETCHWT1_FLAG                                   0x01
#define CPUID_ECX_PREFETCHWT1_MASK                                   0x01
#define CPUID_ECX_PREFETCHWT1(_)                                     (((_) >> 0) & 0x01)

		/**
		* [Bit 1] AVX512_VBMI.
		*/
		uint32_t avx512_vbmi                                           : 1;
#define CPUID_ECX_AVX512_VBMI_BIT                                    1
#define CPUID_ECX_AVX512_VBMI_FLAG                                   0x02
#define CPUID_ECX_AVX512_VBMI_MASK                                   0x01
#define CPUID_ECX_AVX512_VBMI(_)                                     (((_) >> 1) & 0x01)

		/**
		* [Bit 2] Supports user-mode instruction prevention if 1.
		*/
		uint32_t umip                                                  : 1;
#define CPUID_ECX_UMIP_BIT                                           2
#define CPUID_ECX_UMIP_FLAG                                          0x04
#define CPUID_ECX_UMIP_MASK                                          0x01
#define CPUID_ECX_UMIP(_)                                            (((_) >> 2) & 0x01)

		/**
		* [Bit 3] Supports protection keys for user-mode pages if 1.
		*/
		uint32_t pku                                                   : 1;
#define CPUID_ECX_PKU_BIT                                            3
#define CPUID_ECX_PKU_FLAG                                           0x08
#define CPUID_ECX_PKU_MASK                                           0x01
#define CPUID_ECX_PKU(_)                                             (((_) >> 3) & 0x01)

		/**
		* [Bit 4] If 1, OS has set CR4.PKE to enable protection keys (and the RDPKRU/WRPKRU instructions).
		*/
		uint32_t ospke                                                 : 1;
#define CPUID_ECX_OSPKE_BIT                                          4
#define CPUID_ECX_OSPKE_FLAG                                         0x10
#define CPUID_ECX_OSPKE_MASK                                         0x01
#define CPUID_ECX_OSPKE(_)                                           (((_) >> 4) & 0x01)
		uint32_t reserved1                                             : 12;

		/**
		* [Bits 21:17] The value of MAWAU used by the BNDLDX and BNDSTX instructions in 64-bit mode.
		*/
		uint32_t mawau                                                 : 5;
#define CPUID_ECX_MAWAU_BIT                                          17
#define CPUID_ECX_MAWAU_FLAG                                         0x3E0000
#define CPUID_ECX_MAWAU_MASK                                         0x1F
#define CPUID_ECX_MAWAU(_)                                           (((_) >> 17) & 0x1F)

		/**
		* [Bit 22] RDPID and IA32_TSC_AUX are available if 1.
		*/
		uint32_t rdpid                                                 : 1;
#define CPUID_ECX_RDPID_BIT                                          22
#define CPUID_ECX_RDPID_FLAG                                         0x400000
#define CPUID_ECX_RDPID_MASK                                         0x01
#define CPUID_ECX_RDPID(_)                                           (((_) >> 22) & 0x01)
		uint32_t reserved2                                             : 7;

		/**
		* [Bit 30] Supports SGX Launch Configuration if 1.
		*/
		uint32_t sgx_lc                                                : 1;
#define CPUID_ECX_SGX_LC_BIT                                         30
#define CPUID_ECX_SGX_LC_FLAG                                        0x40000000
#define CPUID_ECX_SGX_LC_MASK                                        0x01
#define CPUID_ECX_SGX_LC(_)                                          (((_) >> 30) & 0x01)
		uint32_t reserved3                                             : 1;
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EDX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EDX_RESERVED_BIT                                       0
#define CPUID_EDX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_07;


/**
 * @brief Direct Cache Access Information Leaf
 *
 * When CPUID executes with EAX set to 09H, the processor returns information about Direct Cache Access capabilities.
 */
#define CPUID_DIRECT_CACHE_ACCESS_INFORMATION                        0x00000009
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] Value of bits [31:0] of IA32_PLATFORM_DCA_CAP MSR (address 1F8H).
		*/
		uint32_t ia32_platform_dca_cap                                 : 32;
#define CPUID_EAX_IA32_PLATFORM_DCA_CAP_BIT                          0
#define CPUID_EAX_IA32_PLATFORM_DCA_CAP_FLAG                         0xFFFFFFFF
#define CPUID_EAX_IA32_PLATFORM_DCA_CAP_MASK                         0xFFFFFFFF
#define CPUID_EAX_IA32_PLATFORM_DCA_CAP(_)                           (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EBX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EBX_RESERVED_BIT                                       0
#define CPUID_EBX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EBX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EBX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] ECX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_ECX_RESERVED_BIT                                       0
#define CPUID_ECX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EDX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EDX_RESERVED_BIT                                       0
#define CPUID_EDX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_09;


/**
 * @brief Architectural Performance Monitoring Leaf
 *
 * When CPUID executes with EAX set to 0AH, the processor returns information about support for architectural performance
 * monitoring capabilities. Architectural performance monitoring is supported if the version ID is greater than Pn 0. For
 * each version of architectural performance monitoring capability, software must enumerate this leaf to discover the
 * programming facilities and the architectural performance events available in the processor.
 *
 * @see Vol3C[23(Introduction to Virtual-Machine Extensions)]
 */
#define CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING                   0x0000000A
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 7:0] Version ID of architectural performance monitoring.
		*/
		uint32_t version_id_of_architectural_performance_monitoring    : 8;
#define CPUID_EAX_VERSION_ID_OF_ARCHITECTURAL_PERFORMANCE_MONITORING_BIT 0
#define CPUID_EAX_VERSION_ID_OF_ARCHITECTURAL_PERFORMANCE_MONITORING_FLAG 0xFF
#define CPUID_EAX_VERSION_ID_OF_ARCHITECTURAL_PERFORMANCE_MONITORING_MASK 0xFF
#define CPUID_EAX_VERSION_ID_OF_ARCHITECTURAL_PERFORMANCE_MONITORING(_) (((_) >> 0) & 0xFF)

		/**
		* [Bits 15:8] Number of general-purpose performance monitoring counter per logical processor.
		*/
		uint32_t number_of_performance_monitoring_counter_per_logical_processor: 8;
#define CPUID_EAX_NUMBER_OF_PERFORMANCE_MONITORING_COUNTER_PER_LOGICAL_PROCESSOR_BIT 8
#define CPUID_EAX_NUMBER_OF_PERFORMANCE_MONITORING_COUNTER_PER_LOGICAL_PROCESSOR_FLAG 0xFF00
#define CPUID_EAX_NUMBER_OF_PERFORMANCE_MONITORING_COUNTER_PER_LOGICAL_PROCESSOR_MASK 0xFF
#define CPUID_EAX_NUMBER_OF_PERFORMANCE_MONITORING_COUNTER_PER_LOGICAL_PROCESSOR(_) (((_) >> 8) & 0xFF)

		/**
		* [Bits 23:16] Bit width of general-purpose, performance monitoring counter.
		*/
		uint32_t bit_width_of_performance_monitoring_counter           : 8;
#define CPUID_EAX_BIT_WIDTH_OF_PERFORMANCE_MONITORING_COUNTER_BIT    16
#define CPUID_EAX_BIT_WIDTH_OF_PERFORMANCE_MONITORING_COUNTER_FLAG   0xFF0000
#define CPUID_EAX_BIT_WIDTH_OF_PERFORMANCE_MONITORING_COUNTER_MASK   0xFF
#define CPUID_EAX_BIT_WIDTH_OF_PERFORMANCE_MONITORING_COUNTER(_)     (((_) >> 16) & 0xFF)

		/**
		* [Bits 31:24] Length of EBX bit vector to enumerate architectural performance monitoring events.
		*/
		uint32_t ebx_bit_vector_length                                 : 8;
#define CPUID_EAX_EBX_BIT_VECTOR_LENGTH_BIT                          24
#define CPUID_EAX_EBX_BIT_VECTOR_LENGTH_FLAG                         0xFF000000
#define CPUID_EAX_EBX_BIT_VECTOR_LENGTH_MASK                         0xFF
#define CPUID_EAX_EBX_BIT_VECTOR_LENGTH(_)                           (((_) >> 24) & 0xFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bit 0] Core cycle event not available if 1.
		*/
		uint32_t core_cycle_event_not_available                        : 1;
#define CPUID_EBX_CORE_CYCLE_EVENT_NOT_AVAILABLE_BIT                 0
#define CPUID_EBX_CORE_CYCLE_EVENT_NOT_AVAILABLE_FLAG                0x01
#define CPUID_EBX_CORE_CYCLE_EVENT_NOT_AVAILABLE_MASK                0x01
#define CPUID_EBX_CORE_CYCLE_EVENT_NOT_AVAILABLE(_)                  (((_) >> 0) & 0x01)

		/**
		* [Bit 1] Instruction retired event not available if 1.
		*/
		uint32_t instruction_retired_event_not_available               : 1;
#define CPUID_EBX_INSTRUCTION_RETIRED_EVENT_NOT_AVAILABLE_BIT        1
#define CPUID_EBX_INSTRUCTION_RETIRED_EVENT_NOT_AVAILABLE_FLAG       0x02
#define CPUID_EBX_INSTRUCTION_RETIRED_EVENT_NOT_AVAILABLE_MASK       0x01
#define CPUID_EBX_INSTRUCTION_RETIRED_EVENT_NOT_AVAILABLE(_)         (((_) >> 1) & 0x01)

		/**
		* [Bit 2] Reference cycles event not available if 1.
		*/
		uint32_t reference_cycles_event_not_available                  : 1;
#define CPUID_EBX_REFERENCE_CYCLES_EVENT_NOT_AVAILABLE_BIT           2
#define CPUID_EBX_REFERENCE_CYCLES_EVENT_NOT_AVAILABLE_FLAG          0x04
#define CPUID_EBX_REFERENCE_CYCLES_EVENT_NOT_AVAILABLE_MASK          0x01
#define CPUID_EBX_REFERENCE_CYCLES_EVENT_NOT_AVAILABLE(_)            (((_) >> 2) & 0x01)

		/**
		* [Bit 3] Last-level cache reference event not available if 1.
		*/
		uint32_t last_level_cache_reference_event_not_available        : 1;
#define CPUID_EBX_LAST_LEVEL_CACHE_REFERENCE_EVENT_NOT_AVAILABLE_BIT 3
#define CPUID_EBX_LAST_LEVEL_CACHE_REFERENCE_EVENT_NOT_AVAILABLE_FLAG 0x08
#define CPUID_EBX_LAST_LEVEL_CACHE_REFERENCE_EVENT_NOT_AVAILABLE_MASK 0x01
#define CPUID_EBX_LAST_LEVEL_CACHE_REFERENCE_EVENT_NOT_AVAILABLE(_)  (((_) >> 3) & 0x01)

		/**
		* [Bit 4] Last-level cache misses event not available if 1.
		*/
		uint32_t last_level_cache_misses_event_not_available           : 1;
#define CPUID_EBX_LAST_LEVEL_CACHE_MISSES_EVENT_NOT_AVAILABLE_BIT    4
#define CPUID_EBX_LAST_LEVEL_CACHE_MISSES_EVENT_NOT_AVAILABLE_FLAG   0x10
#define CPUID_EBX_LAST_LEVEL_CACHE_MISSES_EVENT_NOT_AVAILABLE_MASK   0x01
#define CPUID_EBX_LAST_LEVEL_CACHE_MISSES_EVENT_NOT_AVAILABLE(_)     (((_) >> 4) & 0x01)

		/**
		* [Bit 5] Branch instruction retired event not available if 1.
		*/
		uint32_t branch_instruction_retired_event_not_available        : 1;
#define CPUID_EBX_BRANCH_INSTRUCTION_RETIRED_EVENT_NOT_AVAILABLE_BIT 5
#define CPUID_EBX_BRANCH_INSTRUCTION_RETIRED_EVENT_NOT_AVAILABLE_FLAG 0x20
#define CPUID_EBX_BRANCH_INSTRUCTION_RETIRED_EVENT_NOT_AVAILABLE_MASK 0x01
#define CPUID_EBX_BRANCH_INSTRUCTION_RETIRED_EVENT_NOT_AVAILABLE(_)  (((_) >> 5) & 0x01)

		/**
		* [Bit 6] Branch mispredict retired event not available if 1.
		*/
		uint32_t branch_mispredict_retired_event_not_available         : 1;
#define CPUID_EBX_BRANCH_MISPREDICT_RETIRED_EVENT_NOT_AVAILABLE_BIT  6
#define CPUID_EBX_BRANCH_MISPREDICT_RETIRED_EVENT_NOT_AVAILABLE_FLAG 0x40
#define CPUID_EBX_BRANCH_MISPREDICT_RETIRED_EVENT_NOT_AVAILABLE_MASK 0x01
#define CPUID_EBX_BRANCH_MISPREDICT_RETIRED_EVENT_NOT_AVAILABLE(_)   (((_) >> 6) & 0x01)
		uint32_t reserved1                                             : 25;
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] ECX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_ECX_RESERVED_BIT                                       0
#define CPUID_ECX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 4:0] Number of fixed-function performance counters (if Version ID > 1).
		*/
		uint32_t number_of_fixed_function_performance_counters         : 5;
#define CPUID_EDX_NUMBER_OF_FIXED_FUNCTION_PERFORMANCE_COUNTERS_BIT  0
#define CPUID_EDX_NUMBER_OF_FIXED_FUNCTION_PERFORMANCE_COUNTERS_FLAG 0x1F
#define CPUID_EDX_NUMBER_OF_FIXED_FUNCTION_PERFORMANCE_COUNTERS_MASK 0x1F
#define CPUID_EDX_NUMBER_OF_FIXED_FUNCTION_PERFORMANCE_COUNTERS(_)   (((_) >> 0) & 0x1F)

		/**
		* [Bits 12:5] Bit width of fixed-function performance counters (if Version ID > 1).
		*/
		uint32_t bit_width_of_fixed_function_performance_counters      : 8;
#define CPUID_EDX_BIT_WIDTH_OF_FIXED_FUNCTION_PERFORMANCE_COUNTERS_BIT 5
#define CPUID_EDX_BIT_WIDTH_OF_FIXED_FUNCTION_PERFORMANCE_COUNTERS_FLAG 0x1FE0
#define CPUID_EDX_BIT_WIDTH_OF_FIXED_FUNCTION_PERFORMANCE_COUNTERS_MASK 0xFF
#define CPUID_EDX_BIT_WIDTH_OF_FIXED_FUNCTION_PERFORMANCE_COUNTERS(_) (((_) >> 5) & 0xFF)
		uint32_t reserved1                                             : 2;

		/**
		* [Bit 15] AnyThread deprecation.
		*/
		uint32_t any_thread_deprecation                                : 1;
#define CPUID_EDX_ANY_THREAD_DEPRECATION_BIT                         15
#define CPUID_EDX_ANY_THREAD_DEPRECATION_FLAG                        0x8000
#define CPUID_EDX_ANY_THREAD_DEPRECATION_MASK                        0x01
#define CPUID_EDX_ANY_THREAD_DEPRECATION(_)                          (((_) >> 15) & 0x01)
		uint32_t reserved2                                             : 16;
	};

	uint32_t flags;
  } edx;

} cpuid_eax_0a;


/**
 * @brief Extended Topology Enumeration Leaf
 *
 * When CPUID executes with EAX set to 0BH, the processor returns information about extended topology enumeration data.
 * Software must detect the presence of CPUID leaf 0BH by verifying
 * - the highest leaf index supported by CPUID is >= 0BH, and
 * - CPUID.0BH:EBX[15:0] reports a non-zero value.
 *
 * @note Most of Leaf 0BH output depends on the initial value in ECX. The EDX output of leaf 0BH is always valid and does
 *       not vary with input value in ECX. Output value in ECX[7:0] always equals input value in ECX[7:0]. Sub-leaf index 0
 *       enumerates SMT level. Each subsequent higher sub-leaf index enumerates a higherlevel topological entity in hierarchical
 *       order. For sub-leaves that return an invalid level-type of 0 in ECX[15:8]; EAX and EBX will return 0. If an input value
 *       n in ECX returns the invalid level-type of 0 in ECX[15:8], other input values with ECX > n also return 0 in ECX[15:8].
 */
#define CPUID_EXTENDED_TOPOLOGY                                      0x0000000B
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 4:0] Number of bits to shift right on x2APIC ID to get a unique topology ID of the next level type. All logical
		* processors with the same next level ID share current level.
		*
		* @note Software should use this field (EAX[4:0]) to enumerate processor topology of the system.
		*/
		uint32_t x2apic_id_to_unique_topology_id_shift                 : 5;
#define CPUID_EAX_X2APIC_ID_TO_UNIQUE_TOPOLOGY_ID_SHIFT_BIT          0
#define CPUID_EAX_X2APIC_ID_TO_UNIQUE_TOPOLOGY_ID_SHIFT_FLAG         0x1F
#define CPUID_EAX_X2APIC_ID_TO_UNIQUE_TOPOLOGY_ID_SHIFT_MASK         0x1F
#define CPUID_EAX_X2APIC_ID_TO_UNIQUE_TOPOLOGY_ID_SHIFT(_)           (((_) >> 0) & 0x1F)
		uint32_t reserved1                                             : 27;
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 15:0] Number of logical processors at this level type. The number reflects configuration as shipped by Intel.
		*
		* @note Software must not use EBX[15:0] to enumerate processor topology of the system. This value in this field
		*       (EBX[15:0]) is only intended for display/diagnostic purposes. The actual number of logical processors available to
		*       BIOS/OS/Applications may be different from the value of EBX[15:0], depending on software and platform hardware
		*       configurations.
		*/
		uint32_t number_of_logical_processors_at_this_level_type       : 16;
#define CPUID_EBX_NUMBER_OF_LOGICAL_PROCESSORS_AT_THIS_LEVEL_TYPE_BIT 0
#define CPUID_EBX_NUMBER_OF_LOGICAL_PROCESSORS_AT_THIS_LEVEL_TYPE_FLAG 0xFFFF
#define CPUID_EBX_NUMBER_OF_LOGICAL_PROCESSORS_AT_THIS_LEVEL_TYPE_MASK 0xFFFF
#define CPUID_EBX_NUMBER_OF_LOGICAL_PROCESSORS_AT_THIS_LEVEL_TYPE(_) (((_) >> 0) & 0xFFFF)
		uint32_t reserved1                                             : 16;
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 7:0] Level number. Same value in ECX input.
		*/
		uint32_t level_number                                          : 8;
#define CPUID_ECX_LEVEL_NUMBER_BIT                                   0
#define CPUID_ECX_LEVEL_NUMBER_FLAG                                  0xFF
#define CPUID_ECX_LEVEL_NUMBER_MASK                                  0xFF
#define CPUID_ECX_LEVEL_NUMBER(_)                                    (((_) >> 0) & 0xFF)

		/**
		* [Bits 15:8] Level type.
		*
		* @note The value of the "level type" field is not related to level numbers in any way, higher "level type" values do not
		*       mean higher levels. Level type field has the following encoding:
		*       - 0: Invalid.
		*       - 1: SMT.
		*       - 2: Core.
		*       - 3-255: Reserved.
		*/
		uint32_t level_type                                            : 8;
#define CPUID_ECX_LEVEL_TYPE_BIT                                     8
#define CPUID_ECX_LEVEL_TYPE_FLAG                                    0xFF00
#define CPUID_ECX_LEVEL_TYPE_MASK                                    0xFF
#define CPUID_ECX_LEVEL_TYPE(_)                                      (((_) >> 8) & 0xFF)
		uint32_t reserved1                                             : 16;
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] x2APIC ID the current logical processor.
		*/
		uint32_t x2apic_id                                             : 32;
#define CPUID_EDX_X2APIC_ID_BIT                                      0
#define CPUID_EDX_X2APIC_ID_FLAG                                     0xFFFFFFFF
#define CPUID_EDX_X2APIC_ID_MASK                                     0xFFFFFFFF
#define CPUID_EDX_X2APIC_ID(_)                                       (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_0b;

/**
 * @defgroup cpuid_eax_0d \
 *           EAX = 0x0D
 *
 * When CPUID executes with EAX set to 0DH and ECX = 0, the processor returns information about the bit-vector
 * representation of all processor state extensions that are supported in the processor and storage size requirements of
 * the XSAVE/XRSTOR area.
 * When CPUID executes with EAX set to 0DH and ECX = n (n > 1, and is a valid sub-leaf index), the processor returns
 * information about the size and offset of each processor extended state save area within the XSAVE/XRSTOR area. Software
 * can use the forward-extendable technique depicted below to query the valid sub-leaves and obtain size and offset
 * information for each processor extended state save area:
 * <pre> For i = 2 to 62 // sub-leaf 1 is reserved IF (CPUID.(EAX=0DH, ECX=0):VECTOR[i] = 1) // VECTOR is the 64-bit value
 * of EDX:EAX Execute CPUID.(EAX=0DH, ECX = i) to examine size and offset for sub-leaf i; FI; </pre>
 * @{
 */
#define CPUID_EXTENDED_STATE_INFORMATION                             0x0000000D
/**
 * @brief Processor Extended State Enumeration Main Leaf (EAX = 0DH, ECX = 0)
 */
typedef struct
{
  /**
	* @brief Reports the supported bits of the lower 32 bits of XCR0. XCR0[n] can be set to 1 only if EAX[n] is 1
	*/
  union
  {
	struct
	{
		/**
		* [Bit 0] x87 state.
		*/
		uint32_t x87_state                                             : 1;
#define CPUID_EAX_X87_STATE_BIT                                      0
#define CPUID_EAX_X87_STATE_FLAG                                     0x01
#define CPUID_EAX_X87_STATE_MASK                                     0x01
#define CPUID_EAX_X87_STATE(_)                                       (((_) >> 0) & 0x01)

		/**
		* [Bit 1] SSE state.
		*/
		uint32_t sse_state                                             : 1;
#define CPUID_EAX_SSE_STATE_BIT                                      1
#define CPUID_EAX_SSE_STATE_FLAG                                     0x02
#define CPUID_EAX_SSE_STATE_MASK                                     0x01
#define CPUID_EAX_SSE_STATE(_)                                       (((_) >> 1) & 0x01)

		/**
		* [Bit 2] AVX state.
		*/
		uint32_t avx_state                                             : 1;
#define CPUID_EAX_AVX_STATE_BIT                                      2
#define CPUID_EAX_AVX_STATE_FLAG                                     0x04
#define CPUID_EAX_AVX_STATE_MASK                                     0x01
#define CPUID_EAX_AVX_STATE(_)                                       (((_) >> 2) & 0x01)

		/**
		* [Bits 4:3] MPX state.
		*/
		uint32_t mpx_state                                             : 2;
#define CPUID_EAX_MPX_STATE_BIT                                      3
#define CPUID_EAX_MPX_STATE_FLAG                                     0x18
#define CPUID_EAX_MPX_STATE_MASK                                     0x03
#define CPUID_EAX_MPX_STATE(_)                                       (((_) >> 3) & 0x03)

		/**
		* [Bits 7:5] AVX-512 state.
		*/
		uint32_t avx_512_state                                         : 3;
#define CPUID_EAX_AVX_512_STATE_BIT                                  5
#define CPUID_EAX_AVX_512_STATE_FLAG                                 0xE0
#define CPUID_EAX_AVX_512_STATE_MASK                                 0x07
#define CPUID_EAX_AVX_512_STATE(_)                                   (((_) >> 5) & 0x07)

		/**
		* [Bit 8] Used for IA32_XSS.
		*/
		uint32_t used_for_ia32_xss_1                                   : 1;
#define CPUID_EAX_USED_FOR_IA32_XSS_1_BIT                            8
#define CPUID_EAX_USED_FOR_IA32_XSS_1_FLAG                           0x100
#define CPUID_EAX_USED_FOR_IA32_XSS_1_MASK                           0x01
#define CPUID_EAX_USED_FOR_IA32_XSS_1(_)                             (((_) >> 8) & 0x01)

		/**
		* [Bit 9] PKRU state.
		*/
		uint32_t pkru_state                                            : 1;
#define CPUID_EAX_PKRU_STATE_BIT                                     9
#define CPUID_EAX_PKRU_STATE_FLAG                                    0x200
#define CPUID_EAX_PKRU_STATE_MASK                                    0x01
#define CPUID_EAX_PKRU_STATE(_)                                      (((_) >> 9) & 0x01)
		uint32_t reserved1                                             : 3;

		/**
		* [Bit 13] Used for IA32_XSS.
		*/
		uint32_t used_for_ia32_xss_2                                   : 1;
#define CPUID_EAX_USED_FOR_IA32_XSS_2_BIT                            13
#define CPUID_EAX_USED_FOR_IA32_XSS_2_FLAG                           0x2000
#define CPUID_EAX_USED_FOR_IA32_XSS_2_MASK                           0x01
#define CPUID_EAX_USED_FOR_IA32_XSS_2(_)                             (((_) >> 13) & 0x01)
		uint32_t reserved2                                             : 18;
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Maximum size (bytes, from the beginning of the XSAVE/XRSTOR save area) required by enabled features in XCR0.
		* May be different than ECX if some features at the end of the XSAVE save area are not enabled.
		*/
		uint32_t max_size_required_by_enabled_features_in_xcr0         : 32;
#define CPUID_EBX_MAX_SIZE_REQUIRED_BY_ENABLED_FEATURES_IN_XCR0_BIT  0
#define CPUID_EBX_MAX_SIZE_REQUIRED_BY_ENABLED_FEATURES_IN_XCR0_FLAG 0xFFFFFFFF
#define CPUID_EBX_MAX_SIZE_REQUIRED_BY_ENABLED_FEATURES_IN_XCR0_MASK 0xFFFFFFFF
#define CPUID_EBX_MAX_SIZE_REQUIRED_BY_ENABLED_FEATURES_IN_XCR0(_)   (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Maximum size (bytes, from the beginning of the XSAVE/XRSTOR save area) of the XSAVE/XRSTOR save area
		* required by all supported features in the processor, i.e., all the valid bit fields in XCR0.
		*/
		uint32_t max_size_of_xsave_xrstor_save_area                    : 32;
#define CPUID_ECX_MAX_SIZE_OF_XSAVE_XRSTOR_SAVE_AREA_BIT             0
#define CPUID_ECX_MAX_SIZE_OF_XSAVE_XRSTOR_SAVE_AREA_FLAG            0xFFFFFFFF
#define CPUID_ECX_MAX_SIZE_OF_XSAVE_XRSTOR_SAVE_AREA_MASK            0xFFFFFFFF
#define CPUID_ECX_MAX_SIZE_OF_XSAVE_XRSTOR_SAVE_AREA(_)              (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Reports the supported bits of the upper 32 bits of XCR0. XCR0[n+32] can be set to 1 only if EDX[n] is 1.
		*/
		uint32_t xcr0_supported_bits                                   : 32;
#define CPUID_EDX_XCR0_SUPPORTED_BITS_BIT                            0
#define CPUID_EDX_XCR0_SUPPORTED_BITS_FLAG                           0xFFFFFFFF
#define CPUID_EDX_XCR0_SUPPORTED_BITS_MASK                           0xFFFFFFFF
#define CPUID_EDX_XCR0_SUPPORTED_BITS(_)                             (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_0d_ecx_00;

/**
 * @brief Direct Cache Access Information Leaf
 */
typedef struct
{
  union
  {
	struct
	{
		uint32_t reserved1                                             : 1;

		/**
		* [Bit 1] Supports XSAVEC and the compacted form of XRSTOR if set.
		*/
		uint32_t supports_xsavec_and_compacted_xrstor                  : 1;
#define CPUID_EAX_SUPPORTS_XSAVEC_AND_COMPACTED_XRSTOR_BIT           1
#define CPUID_EAX_SUPPORTS_XSAVEC_AND_COMPACTED_XRSTOR_FLAG          0x02
#define CPUID_EAX_SUPPORTS_XSAVEC_AND_COMPACTED_XRSTOR_MASK          0x01
#define CPUID_EAX_SUPPORTS_XSAVEC_AND_COMPACTED_XRSTOR(_)            (((_) >> 1) & 0x01)

		/**
		* [Bit 2] Supports XGETBV with ECX = 1 if set.
		*/
		uint32_t supports_xgetbv_with_ecx_1                            : 1;
#define CPUID_EAX_SUPPORTS_XGETBV_WITH_ECX_1_BIT                     2
#define CPUID_EAX_SUPPORTS_XGETBV_WITH_ECX_1_FLAG                    0x04
#define CPUID_EAX_SUPPORTS_XGETBV_WITH_ECX_1_MASK                    0x01
#define CPUID_EAX_SUPPORTS_XGETBV_WITH_ECX_1(_)                      (((_) >> 2) & 0x01)

		/**
		* [Bit 3] Supports XSAVES/XRSTORS and IA32_XSS if set.
		*/
		uint32_t supports_xsave_xrstor_and_ia32_xss                    : 1;
#define CPUID_EAX_SUPPORTS_XSAVE_XRSTOR_AND_IA32_XSS_BIT             3
#define CPUID_EAX_SUPPORTS_XSAVE_XRSTOR_AND_IA32_XSS_FLAG            0x08
#define CPUID_EAX_SUPPORTS_XSAVE_XRSTOR_AND_IA32_XSS_MASK            0x01
#define CPUID_EAX_SUPPORTS_XSAVE_XRSTOR_AND_IA32_XSS(_)              (((_) >> 3) & 0x01)
		uint32_t reserved2                                             : 28;
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] The size in bytes of the XSAVE area containing all states enabled by XCRO | IA32_XSS.
		*/
		uint32_t size_of_xsave_aread                                   : 32;
#define CPUID_EBX_SIZE_OF_XSAVE_AREAD_BIT                            0
#define CPUID_EBX_SIZE_OF_XSAVE_AREAD_FLAG                           0xFFFFFFFF
#define CPUID_EBX_SIZE_OF_XSAVE_AREAD_MASK                           0xFFFFFFFF
#define CPUID_EBX_SIZE_OF_XSAVE_AREAD(_)                             (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 7:0] Used for XCR0.
		*/
		uint32_t used_for_xcr0_1                                       : 8;
#define CPUID_ECX_USED_FOR_XCR0_1_BIT                                0
#define CPUID_ECX_USED_FOR_XCR0_1_FLAG                               0xFF
#define CPUID_ECX_USED_FOR_XCR0_1_MASK                               0xFF
#define CPUID_ECX_USED_FOR_XCR0_1(_)                                 (((_) >> 0) & 0xFF)

		/**
		* [Bit 8] PT state.
		*/
		uint32_t pt_state                                              : 1;
#define CPUID_ECX_PT_STATE_BIT                                       8
#define CPUID_ECX_PT_STATE_FLAG                                      0x100
#define CPUID_ECX_PT_STATE_MASK                                      0x01
#define CPUID_ECX_PT_STATE(_)                                        (((_) >> 8) & 0x01)

		/**
		* [Bit 9] Used for XCR0.
		*/
		uint32_t used_for_xcr0_2                                       : 1;
#define CPUID_ECX_USED_FOR_XCR0_2_BIT                                9
#define CPUID_ECX_USED_FOR_XCR0_2_FLAG                               0x200
#define CPUID_ECX_USED_FOR_XCR0_2_MASK                               0x01
#define CPUID_ECX_USED_FOR_XCR0_2(_)                                 (((_) >> 9) & 0x01)
		uint32_t reserved1                                             : 3;

		/**
		* [Bit 13] HWP state.
		*/
		uint32_t hwp_state                                             : 1;
#define CPUID_ECX_HWP_STATE_BIT                                      13
#define CPUID_ECX_HWP_STATE_FLAG                                     0x2000
#define CPUID_ECX_HWP_STATE_MASK                                     0x01
#define CPUID_ECX_HWP_STATE(_)                                       (((_) >> 13) & 0x01)
		uint32_t reserved2                                             : 18;
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EDX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EDX_RESERVED_BIT                                       0
#define CPUID_EDX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_0d_ecx_01;

/**
 * @brief Processor Extended State Enumeration Sub-leaves (EAX = 0DH, ECX = n, n > 1)
 *
 * @note Leaf 0DH output depends on the initial value in ECX. Each sub-leaf index (starting at position 2) is supported if
 *       it corresponds to a supported bit in either the XCR0 register or the IA32_XSS MSR.
 *       If ECX contains an invalid sub-leaf index, EAX/EBX/ECX/EDX return 0. Sub-leaf n (0 <= n <= 31) is invalid if sub-leaf 0
 *       returns 0 in EAX[n] and sub-leaf 1 returns 0 in ECX[n]. Sub-leaf n (32 <= n <= 63) is invalid if sub-leaf 0 returns 0 in
 *       EDX[n-32] and sub-leaf 1 returns 0 in EDX[n-32].
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] The size in bytes (from the offset specified in EBX) of the save area for an extended state feature
		* associated with a valid sub-leaf index, n.
		*/
		uint32_t ia32_platform_dca_cap                                 : 32;
#define CPUID_EAX_IA32_PLATFORM_DCA_CAP_BIT                          0
#define CPUID_EAX_IA32_PLATFORM_DCA_CAP_FLAG                         0xFFFFFFFF
#define CPUID_EAX_IA32_PLATFORM_DCA_CAP_MASK                         0xFFFFFFFF
#define CPUID_EAX_IA32_PLATFORM_DCA_CAP(_)                           (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] The offset in bytes of this extended state component's save area from the beginning of the XSAVE/XRSTOR
		* area.
		* This field reports 0 if the sub-leaf index, n, does not map to a valid bit in the XCR0 register.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EBX_RESERVED_BIT                                       0
#define CPUID_EBX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EBX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EBX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bit 0] Is set if the bit n (corresponding to the sub-leaf index) is supported in the IA32_XSS MSR; it is clear if bit n
		* is instead supported in XCR0.
		*/
		uint32_t ecx_2                                                 : 1;
#define CPUID_ECX_ECX_2_BIT                                          0
#define CPUID_ECX_ECX_2_FLAG                                         0x01
#define CPUID_ECX_ECX_2_MASK                                         0x01
#define CPUID_ECX_ECX_2(_)                                           (((_) >> 0) & 0x01)

		/**
		* [Bit 1] Is set if, when the compacted format of an XSAVE area is used, this extended state component located on the next
		* 64-byte boundary following the preceding state component (otherwise, it is located immediately following the preceding
		* state component).
		*/
		uint32_t ecx_1                                                 : 1;
#define CPUID_ECX_ECX_1_BIT                                          1
#define CPUID_ECX_ECX_1_FLAG                                         0x02
#define CPUID_ECX_ECX_1_MASK                                         0x01
#define CPUID_ECX_ECX_1(_)                                           (((_) >> 1) & 0x01)
		uint32_t reserved1                                             : 30;
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] This field reports 0 if the sub-leaf index, n, is invalid; otherwise it is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EDX_RESERVED_BIT                                       0
#define CPUID_EDX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_0d_ecx_n;

/**
 * @}
 */

/**
 * @defgroup cpuid_eax_0f \
 *           EAX = 0x0F
 *
 * When CPUID executes with EAX set to 0FH and ECX = 0, the processor returns information about the bit-vector
 * representation of QoS monitoring resource types that are supported in the processor and maximum range of RMID values the
 * processor can use to monitor of any supported resource types. Each bit, starting from bit 1, corresponds to a specific
 * resource type if the bit is set. The bit position corresponds to the sub-leaf index (or ResID) that software must use to
 * query QoS monitoring capability available for that type. See Table 3-8.
 * When CPUID executes with EAX set to 0FH and ECX = n (n >= 1, and is a valid ResID), the processor returns information
 * software can use to program IA32_PQR_ASSOC, IA32_QM_EVTSEL MSRs before reading QoS data from the IA32_QM_CTR MSR.
 * @{
 */
#define CPUID_INTEL_RESOURCE_DIRECTOR_TECHNOLOGY_MONITORING_INFORMATION 0x0000000F
/**
 * @brief Intel Resource Director Technology (Intel RDT) Monitoring Enumeration Sub-leaf (EAX = 0FH, ECX = 0)
 *
 * @note Leaf 0FH output depends on the initial value in ECX. Sub-leaf index 0 reports valid resource type starting at bit
 *       position 1 of EDX.
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] EAX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EAX_RESERVED_BIT                                       0
#define CPUID_EAX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EAX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EAX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Maximum range (zero-based) of RMID within this physical processor of all types.
		*/
		uint32_t rmid_max_range                                        : 32;
#define CPUID_EBX_RMID_MAX_RANGE_BIT                                 0
#define CPUID_EBX_RMID_MAX_RANGE_FLAG                                0xFFFFFFFF
#define CPUID_EBX_RMID_MAX_RANGE_MASK                                0xFFFFFFFF
#define CPUID_EBX_RMID_MAX_RANGE(_)                                  (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] ECX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_ECX_RESERVED_BIT                                       0
#define CPUID_ECX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		uint32_t reserved1                                             : 1;

		/**
		* [Bit 1] Supports L3 Cache Intel RDT Monitoring if 1.
		*/
		uint32_t supports_l3_cache_intel_rdt_monitoring                : 1;
#define CPUID_EDX_SUPPORTS_L3_CACHE_INTEL_RDT_MONITORING_BIT         1
#define CPUID_EDX_SUPPORTS_L3_CACHE_INTEL_RDT_MONITORING_FLAG        0x02
#define CPUID_EDX_SUPPORTS_L3_CACHE_INTEL_RDT_MONITORING_MASK        0x01
#define CPUID_EDX_SUPPORTS_L3_CACHE_INTEL_RDT_MONITORING(_)          (((_) >> 1) & 0x01)
		uint32_t reserved2                                             : 30;
	};

	uint32_t flags;
  } edx;

} cpuid_eax_0f_ecx_00;

/**
 * @brief L3 Cache Intel RDT Monitoring Capability Enumeration Sub-leaf (EAX = 0FH, ECX = 1)
 *
 * @note Leaf 0FH output depends on the initial value in ECX.
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] EAX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EAX_RESERVED_BIT                                       0
#define CPUID_EAX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EAX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EAX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Conversion factor from reported IA32_QM_CTR value to occupancy metric (bytes).
		*/
		uint32_t conversion_factor                                     : 32;
#define CPUID_EBX_CONVERSION_FACTOR_BIT                              0
#define CPUID_EBX_CONVERSION_FACTOR_FLAG                             0xFFFFFFFF
#define CPUID_EBX_CONVERSION_FACTOR_MASK                             0xFFFFFFFF
#define CPUID_EBX_CONVERSION_FACTOR(_)                               (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Maximum range (zero-based) of RMID within this physical processor of all types.
		*/
		uint32_t rmid_max_range                                        : 32;
#define CPUID_ECX_RMID_MAX_RANGE_BIT                                 0
#define CPUID_ECX_RMID_MAX_RANGE_FLAG                                0xFFFFFFFF
#define CPUID_ECX_RMID_MAX_RANGE_MASK                                0xFFFFFFFF
#define CPUID_ECX_RMID_MAX_RANGE(_)                                  (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bit 0] Supports L3 occupancy monitoring if 1.
		*/
		uint32_t supports_l3_occupancy_monitoring                      : 1;
#define CPUID_EDX_SUPPORTS_L3_OCCUPANCY_MONITORING_BIT               0
#define CPUID_EDX_SUPPORTS_L3_OCCUPANCY_MONITORING_FLAG              0x01
#define CPUID_EDX_SUPPORTS_L3_OCCUPANCY_MONITORING_MASK              0x01
#define CPUID_EDX_SUPPORTS_L3_OCCUPANCY_MONITORING(_)                (((_) >> 0) & 0x01)

		/**
		* [Bit 1] Supports L3 Total Bandwidth monitoring if 1.
		*/
		uint32_t supports_l3_total_bandwidth_monitoring                : 1;
#define CPUID_EDX_SUPPORTS_L3_TOTAL_BANDWIDTH_MONITORING_BIT         1
#define CPUID_EDX_SUPPORTS_L3_TOTAL_BANDWIDTH_MONITORING_FLAG        0x02
#define CPUID_EDX_SUPPORTS_L3_TOTAL_BANDWIDTH_MONITORING_MASK        0x01
#define CPUID_EDX_SUPPORTS_L3_TOTAL_BANDWIDTH_MONITORING(_)          (((_) >> 1) & 0x01)

		/**
		* [Bit 2] Supports L3 Local Bandwidth monitoring if 1.
		*/
		uint32_t supports_l3_local_bandwidth_monitoring                : 1;
#define CPUID_EDX_SUPPORTS_L3_LOCAL_BANDWIDTH_MONITORING_BIT         2
#define CPUID_EDX_SUPPORTS_L3_LOCAL_BANDWIDTH_MONITORING_FLAG        0x04
#define CPUID_EDX_SUPPORTS_L3_LOCAL_BANDWIDTH_MONITORING_MASK        0x01
#define CPUID_EDX_SUPPORTS_L3_LOCAL_BANDWIDTH_MONITORING(_)          (((_) >> 2) & 0x01)
		uint32_t reserved1                                             : 29;
	};

	uint32_t flags;
  } edx;

} cpuid_eax_0f_ecx_01;

/**
 * @}
 */

/**
 * @defgroup cpuid_eax_10 \
 *           EAX = 0x10
 *
 * When CPUID executes with EAX set to 10H and ECX = 0, the processor returns information about the bit-vector
 * representation of QoS Enforcement resource types that are supported in the processor. Each bit, starting from bit 1,
 * corresponds to a specific resource type if the bit is set. The bit position corresponds to the sub-leaf index (or ResID)
 * that software must use to query QoS enforcement capability available for that type.
 * When CPUID executes with EAX set to 10H and ECX = n (n >= 1, and is a valid ResID), the processor returns information
 * about available classes of service and range of QoS mask MSRs that software can use to configure each class of services
 * using capability bit masks in the QoS Mask registers, IA32_resourceType_Mask_n.
 * @{
 */
#define CPUID_INTEL_RESOURCE_DIRECTOR_TECHNOLOGY_ALLOCATION_INFORMATION 0x00000010
/**
 * @brief Intel Resource Director Technology (Intel RDT) Allocation Enumeration Sub-leaf (EAX = 10H, ECX = 0)
 *
 * @note Leaf 10H output depends on the initial value in ECX. Sub-leaf index 0 reports valid resource identification
 *       (ResID) starting at bit position 1 of EBX.
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] Value of bits [31:0] of IA32_PLATFORM_DCA_CAP MSR (address 1F8H).
		*/
		uint32_t ia32_platform_dca_cap                                 : 32;
#define CPUID_EAX_IA32_PLATFORM_DCA_CAP_BIT                          0
#define CPUID_EAX_IA32_PLATFORM_DCA_CAP_FLAG                         0xFFFFFFFF
#define CPUID_EAX_IA32_PLATFORM_DCA_CAP_MASK                         0xFFFFFFFF
#define CPUID_EAX_IA32_PLATFORM_DCA_CAP(_)                           (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		uint32_t reserved1                                             : 1;

		/**
		* [Bit 1] Supports L3 Cache Allocation Technology if 1.
		*/
		uint32_t supports_l3_cache_allocation_technology               : 1;
#define CPUID_EBX_SUPPORTS_L3_CACHE_ALLOCATION_TECHNOLOGY_BIT        1
#define CPUID_EBX_SUPPORTS_L3_CACHE_ALLOCATION_TECHNOLOGY_FLAG       0x02
#define CPUID_EBX_SUPPORTS_L3_CACHE_ALLOCATION_TECHNOLOGY_MASK       0x01
#define CPUID_EBX_SUPPORTS_L3_CACHE_ALLOCATION_TECHNOLOGY(_)         (((_) >> 1) & 0x01)

		/**
		* [Bit 2] Supports L2 Cache Allocation Technology if 1.
		*/
		uint32_t supports_l2_cache_allocation_technology               : 1;
#define CPUID_EBX_SUPPORTS_L2_CACHE_ALLOCATION_TECHNOLOGY_BIT        2
#define CPUID_EBX_SUPPORTS_L2_CACHE_ALLOCATION_TECHNOLOGY_FLAG       0x04
#define CPUID_EBX_SUPPORTS_L2_CACHE_ALLOCATION_TECHNOLOGY_MASK       0x01
#define CPUID_EBX_SUPPORTS_L2_CACHE_ALLOCATION_TECHNOLOGY(_)         (((_) >> 2) & 0x01)

		/**
		* [Bit 3] Supports Memory Bandwidth Allocation if 1.
		*/
		uint32_t supports_memory_bandwidth_allocation                  : 1;
#define CPUID_EBX_SUPPORTS_MEMORY_BANDWIDTH_ALLOCATION_BIT           3
#define CPUID_EBX_SUPPORTS_MEMORY_BANDWIDTH_ALLOCATION_FLAG          0x08
#define CPUID_EBX_SUPPORTS_MEMORY_BANDWIDTH_ALLOCATION_MASK          0x01
#define CPUID_EBX_SUPPORTS_MEMORY_BANDWIDTH_ALLOCATION(_)            (((_) >> 3) & 0x01)
		uint32_t reserved2                                             : 28;
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] ECX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_ECX_RESERVED_BIT                                       0
#define CPUID_ECX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EDX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EDX_RESERVED_BIT                                       0
#define CPUID_EDX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_10_ecx_00;

/**
 * @brief L3 Cache Allocation Technology Enumeration Sub-leaf (EAX = 10H, ECX = ResID = 1)
 *
 * @note Leaf 10H output depends on the initial value in ECX.
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 4:0] Length of the capacity bit mask for the corresponding ResID using minus-one notation.
		*/
		uint32_t length_of_capacity_bit_mask                           : 5;
#define CPUID_EAX_LENGTH_OF_CAPACITY_BIT_MASK_BIT                    0
#define CPUID_EAX_LENGTH_OF_CAPACITY_BIT_MASK_FLAG                   0x1F
#define CPUID_EAX_LENGTH_OF_CAPACITY_BIT_MASK_MASK                   0x1F
#define CPUID_EAX_LENGTH_OF_CAPACITY_BIT_MASK(_)                     (((_) >> 0) & 0x1F)
		uint32_t reserved1                                             : 27;
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Bit-granular map of isolation/contention of allocation units.
		*/
		uint32_t ebx_0                                                 : 32;
#define CPUID_EBX_EBX_0_BIT                                          0
#define CPUID_EBX_EBX_0_FLAG                                         0xFFFFFFFF
#define CPUID_EBX_EBX_0_MASK                                         0xFFFFFFFF
#define CPUID_EBX_EBX_0(_)                                           (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		uint32_t reserved1                                             : 2;

		/**
		* [Bit 2] Code and Data Prioritization Technology supported if 1.
		*/
		uint32_t code_and_data_priorization_technology_supported       : 1;
#define CPUID_ECX_CODE_AND_DATA_PRIORIZATION_TECHNOLOGY_SUPPORTED_BIT 2
#define CPUID_ECX_CODE_AND_DATA_PRIORIZATION_TECHNOLOGY_SUPPORTED_FLAG 0x04
#define CPUID_ECX_CODE_AND_DATA_PRIORIZATION_TECHNOLOGY_SUPPORTED_MASK 0x01
#define CPUID_ECX_CODE_AND_DATA_PRIORIZATION_TECHNOLOGY_SUPPORTED(_) (((_) >> 2) & 0x01)
		uint32_t reserved2                                             : 29;
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 15:0] Highest COS number supported for this ResID.
		*/
		uint32_t highest_cos_number_supported                          : 16;
#define CPUID_EDX_HIGHEST_COS_NUMBER_SUPPORTED_BIT                   0
#define CPUID_EDX_HIGHEST_COS_NUMBER_SUPPORTED_FLAG                  0xFFFF
#define CPUID_EDX_HIGHEST_COS_NUMBER_SUPPORTED_MASK                  0xFFFF
#define CPUID_EDX_HIGHEST_COS_NUMBER_SUPPORTED(_)                    (((_) >> 0) & 0xFFFF)
		uint32_t reserved1                                             : 16;
	};

	uint32_t flags;
  } edx;

} cpuid_eax_10_ecx_01;

/**
 * @brief L2 Cache Allocation Technology Enumeration Sub-leaf (EAX = 10H, ECX = ResID = 2)
 *
 * @note Leaf 10H output depends on the initial value in ECX.
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 4:0] Length of the capacity bit mask for the corresponding ResID using minus-one notation.
		*/
		uint32_t length_of_capacity_bit_mask                           : 5;
#define CPUID_EAX_LENGTH_OF_CAPACITY_BIT_MASK_BIT                    0
#define CPUID_EAX_LENGTH_OF_CAPACITY_BIT_MASK_FLAG                   0x1F
#define CPUID_EAX_LENGTH_OF_CAPACITY_BIT_MASK_MASK                   0x1F
#define CPUID_EAX_LENGTH_OF_CAPACITY_BIT_MASK(_)                     (((_) >> 0) & 0x1F)
		uint32_t reserved1                                             : 27;
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Bit-granular map of isolation/contention of allocation units.
		*/
		uint32_t ebx_0                                                 : 32;
#define CPUID_EBX_EBX_0_BIT                                          0
#define CPUID_EBX_EBX_0_FLAG                                         0xFFFFFFFF
#define CPUID_EBX_EBX_0_MASK                                         0xFFFFFFFF
#define CPUID_EBX_EBX_0(_)                                           (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] ECX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_ECX_RESERVED_BIT                                       0
#define CPUID_ECX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 15:0] Highest COS number supported for this ResID.
		*/
		uint32_t highest_cos_number_supported                          : 16;
#define CPUID_EDX_HIGHEST_COS_NUMBER_SUPPORTED_BIT                   0
#define CPUID_EDX_HIGHEST_COS_NUMBER_SUPPORTED_FLAG                  0xFFFF
#define CPUID_EDX_HIGHEST_COS_NUMBER_SUPPORTED_MASK                  0xFFFF
#define CPUID_EDX_HIGHEST_COS_NUMBER_SUPPORTED(_)                    (((_) >> 0) & 0xFFFF)
		uint32_t reserved1                                             : 16;
	};

	uint32_t flags;
  } edx;

} cpuid_eax_10_ecx_02;

/**
 * @brief Memory Bandwidth Allocation Enumeration Sub-leaf (EAX = 10H, ECX = ResID = 3)
 *
 * @note Leaf 10H output depends on the initial value in ECX.
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 11:0] Reports the maximum MBA throttling value supported for the corresponding ResID using minus-one notation.
		*/
		uint32_t max_mba_throttling_value                              : 12;
#define CPUID_EAX_MAX_MBA_THROTTLING_VALUE_BIT                       0
#define CPUID_EAX_MAX_MBA_THROTTLING_VALUE_FLAG                      0xFFF
#define CPUID_EAX_MAX_MBA_THROTTLING_VALUE_MASK                      0xFFF
#define CPUID_EAX_MAX_MBA_THROTTLING_VALUE(_)                        (((_) >> 0) & 0xFFF)
		uint32_t reserved1                                             : 20;
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EBX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EBX_RESERVED_BIT                                       0
#define CPUID_EBX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EBX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EBX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		uint32_t reserved1                                             : 2;

		/**
		* [Bit 2] Reports whether the response of the delay values is linear.
		*/
		uint32_t response_of_delay_is_linear                           : 1;
#define CPUID_ECX_RESPONSE_OF_DELAY_IS_LINEAR_BIT                    2
#define CPUID_ECX_RESPONSE_OF_DELAY_IS_LINEAR_FLAG                   0x04
#define CPUID_ECX_RESPONSE_OF_DELAY_IS_LINEAR_MASK                   0x01
#define CPUID_ECX_RESPONSE_OF_DELAY_IS_LINEAR(_)                     (((_) >> 2) & 0x01)
		uint32_t reserved2                                             : 29;
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 15:0] Highest COS number supported for this ResID.
		*/
		uint32_t highest_cos_number_supported                          : 16;
#define CPUID_EDX_HIGHEST_COS_NUMBER_SUPPORTED_BIT                   0
#define CPUID_EDX_HIGHEST_COS_NUMBER_SUPPORTED_FLAG                  0xFFFF
#define CPUID_EDX_HIGHEST_COS_NUMBER_SUPPORTED_MASK                  0xFFFF
#define CPUID_EDX_HIGHEST_COS_NUMBER_SUPPORTED(_)                    (((_) >> 0) & 0xFFFF)
		uint32_t reserved1                                             : 16;
	};

	uint32_t flags;
  } edx;

} cpuid_eax_10_ecx_03;

/**
 * @}
 */

/**
 * @defgroup cpuid_eax_12 \
 *           EAX = 0x12
 *
 * When CPUID executes with EAX set to 12H and ECX = 0H, the processor returns information about Intel SGX capabilities.
 * When CPUID executes with EAX set to 12H and ECX = 1H, the processor returns information about Intel SGX attributes.
 * When CPUID executes with EAX set to 12H and ECX = n (n > 1), the processor returns information about Intel SGX Enclave
 * Page Cache.
 * @{
 */
#define CPUID_INTEL_SGX                                              0x00000012
/**
 * @brief Intel SGX Capability Enumeration Leaf, sub-leaf 0 (EAX = 12H, ECX = 0)
 *
 * @note Leaf 12H sub-leaf 0 (ECX = 0) is supported if CPUID.(EAX=07H, ECX=0H):EBX[SGX] = 1.
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bit 0] If 1, Indicates Intel SGX supports the collection of SGX1 leaf functions.
		*/
		uint32_t sgx1                                                  : 1;
#define CPUID_EAX_SGX1_BIT                                           0
#define CPUID_EAX_SGX1_FLAG                                          0x01
#define CPUID_EAX_SGX1_MASK                                          0x01
#define CPUID_EAX_SGX1(_)                                            (((_) >> 0) & 0x01)

		/**
		* [Bit 1] If 1, Indicates Intel SGX supports the collection of SGX2 leaf functions.
		*/
		uint32_t sgx2                                                  : 1;
#define CPUID_EAX_SGX2_BIT                                           1
#define CPUID_EAX_SGX2_FLAG                                          0x02
#define CPUID_EAX_SGX2_MASK                                          0x01
#define CPUID_EAX_SGX2(_)                                            (((_) >> 1) & 0x01)
		uint32_t reserved1                                             : 3;

		/**
		* [Bit 5] If 1, indicates Intel SGX supports ENCLV instruction leaves EINCVIRTCHILD, EDECVIRTCHILD, and ESETCONTEXT.
		*/
		uint32_t sgx_enclv_advanced                                    : 1;
#define CPUID_EAX_SGX_ENCLV_ADVANCED_BIT                             5
#define CPUID_EAX_SGX_ENCLV_ADVANCED_FLAG                            0x20
#define CPUID_EAX_SGX_ENCLV_ADVANCED_MASK                            0x01
#define CPUID_EAX_SGX_ENCLV_ADVANCED(_)                              (((_) >> 5) & 0x01)

		/**
		* [Bit 6] If 1, indicates Intel SGX supports ENCLS instruction leaves ETRACKC, ERDINFO, ELDBC, and ELDUC.
		*/
		uint32_t sgx_encls_advanced                                    : 1;
#define CPUID_EAX_SGX_ENCLS_ADVANCED_BIT                             6
#define CPUID_EAX_SGX_ENCLS_ADVANCED_FLAG                            0x40
#define CPUID_EAX_SGX_ENCLS_ADVANCED_MASK                            0x01
#define CPUID_EAX_SGX_ENCLS_ADVANCED(_)                              (((_) >> 6) & 0x01)
		uint32_t reserved2                                             : 25;
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Bit vector of supported extended SGX features.
		*/
		uint32_t miscselect                                            : 32;
#define CPUID_EBX_MISCSELECT_BIT                                     0
#define CPUID_EBX_MISCSELECT_FLAG                                    0xFFFFFFFF
#define CPUID_EBX_MISCSELECT_MASK                                    0xFFFFFFFF
#define CPUID_EBX_MISCSELECT(_)                                      (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] ECX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_ECX_RESERVED_BIT                                       0
#define CPUID_ECX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 7:0] The maximum supported enclave size in non-64-bit mode is 2^(EDX[7:0]).
		*/
		uint32_t max_enclave_size_not64                                : 8;
#define CPUID_EDX_MAX_ENCLAVE_SIZE_NOT64_BIT                         0
#define CPUID_EDX_MAX_ENCLAVE_SIZE_NOT64_FLAG                        0xFF
#define CPUID_EDX_MAX_ENCLAVE_SIZE_NOT64_MASK                        0xFF
#define CPUID_EDX_MAX_ENCLAVE_SIZE_NOT64(_)                          (((_) >> 0) & 0xFF)

		/**
		* [Bits 15:8] The maximum supported enclave size in 64-bit mode is 2^(EDX[15:8]).
		*/
		uint32_t max_enclave_size_64                                   : 8;
#define CPUID_EDX_MAX_ENCLAVE_SIZE_64_BIT                            8
#define CPUID_EDX_MAX_ENCLAVE_SIZE_64_FLAG                           0xFF00
#define CPUID_EDX_MAX_ENCLAVE_SIZE_64_MASK                           0xFF
#define CPUID_EDX_MAX_ENCLAVE_SIZE_64(_)                             (((_) >> 8) & 0xFF)
		uint32_t reserved1                                             : 16;
	};

	uint32_t flags;
  } edx;

} cpuid_eax_12_ecx_00;

/**
 * @brief Intel SGX Attributes Enumeration Leaf, sub-leaf 1 (EAX = 12H, ECX = 1)
 *
 * @note Leaf 12H sub-leaf 1 (ECX = 1) is supported if CPUID.(EAX=07H, ECX=0H):EBX[SGX] = 1.
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] Reports the valid bits of SECS.ATTRIBUTES[31:0] that software can set with ECREATE.
		*/
		uint32_t valid_secs_attributes_0                               : 32;
#define CPUID_EAX_VALID_SECS_ATTRIBUTES_0_BIT                        0
#define CPUID_EAX_VALID_SECS_ATTRIBUTES_0_FLAG                       0xFFFFFFFF
#define CPUID_EAX_VALID_SECS_ATTRIBUTES_0_MASK                       0xFFFFFFFF
#define CPUID_EAX_VALID_SECS_ATTRIBUTES_0(_)                         (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Reports the valid bits of SECS.ATTRIBUTES[63:32] that software can set with ECREATE.
		*/
		uint32_t valid_secs_attributes_1                               : 32;
#define CPUID_EBX_VALID_SECS_ATTRIBUTES_1_BIT                        0
#define CPUID_EBX_VALID_SECS_ATTRIBUTES_1_FLAG                       0xFFFFFFFF
#define CPUID_EBX_VALID_SECS_ATTRIBUTES_1_MASK                       0xFFFFFFFF
#define CPUID_EBX_VALID_SECS_ATTRIBUTES_1(_)                         (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Reports the valid bits of SECS.ATTRIBUTES[95:64] that software can set with ECREATE.
		*/
		uint32_t valid_secs_attributes_2                               : 32;
#define CPUID_ECX_VALID_SECS_ATTRIBUTES_2_BIT                        0
#define CPUID_ECX_VALID_SECS_ATTRIBUTES_2_FLAG                       0xFFFFFFFF
#define CPUID_ECX_VALID_SECS_ATTRIBUTES_2_MASK                       0xFFFFFFFF
#define CPUID_ECX_VALID_SECS_ATTRIBUTES_2(_)                         (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Reports the valid bits of SECS.ATTRIBUTES[127:96] that software can set with ECREATE.
		*/
		uint32_t valid_secs_attributes_3                               : 32;
#define CPUID_EDX_VALID_SECS_ATTRIBUTES_3_BIT                        0
#define CPUID_EDX_VALID_SECS_ATTRIBUTES_3_FLAG                       0xFFFFFFFF
#define CPUID_EDX_VALID_SECS_ATTRIBUTES_3_MASK                       0xFFFFFFFF
#define CPUID_EDX_VALID_SECS_ATTRIBUTES_3(_)                         (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_12_ecx_01;

/**
 * @brief Intel SGX EPC Enumeration Leaf, sub-leaves (EAX = 12H, ECX = 2 or higher)
 *
 * @note Leaf 12H sub-leaf 2 or higher (ECX >= 2) is supported if CPUID.(EAX=07H, ECX=0H):EBX[SGX] = 1.
 *       This structure describes sub-leaf type 0.
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 3:0] Sub-leaf Type 0. Indicates this sub-leaf is invalid.
		*/
		uint32_t sub_leaf_type                                         : 4;
#define CPUID_EAX_SUB_LEAF_TYPE_BIT                                  0
#define CPUID_EAX_SUB_LEAF_TYPE_FLAG                                 0x0F
#define CPUID_EAX_SUB_LEAF_TYPE_MASK                                 0x0F
#define CPUID_EAX_SUB_LEAF_TYPE(_)                                   (((_) >> 0) & 0x0F)
		uint32_t reserved1                                             : 28;
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EBX is zero.
		*/
		uint32_t zero                                                  : 32;
#define CPUID_EBX_ZERO_BIT                                           0
#define CPUID_EBX_ZERO_FLAG                                          0xFFFFFFFF
#define CPUID_EBX_ZERO_MASK                                          0xFFFFFFFF
#define CPUID_EBX_ZERO(_)                                            (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] ECX is zero.
		*/
		uint32_t zero                                                  : 32;
#define CPUID_ECX_ZERO_BIT                                           0
#define CPUID_ECX_ZERO_FLAG                                          0xFFFFFFFF
#define CPUID_ECX_ZERO_MASK                                          0xFFFFFFFF
#define CPUID_ECX_ZERO(_)                                            (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EDX is zero.
		*/
		uint32_t zero                                                  : 32;
#define CPUID_EDX_ZERO_BIT                                           0
#define CPUID_EDX_ZERO_FLAG                                          0xFFFFFFFF
#define CPUID_EDX_ZERO_MASK                                          0xFFFFFFFF
#define CPUID_EDX_ZERO(_)                                            (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_12_ecx_02p_slt_0;

/**
 * @brief Intel SGX EPC Enumeration Leaf, sub-leaves (EAX = 12H, ECX = 2 or higher)
 *
 * @note Leaf 12H sub-leaf 2 or higher (ECX >= 2) is supported if CPUID.(EAX=07H, ECX=0H):EBX[SGX] = 1.
 *       This structure describes sub-leaf type 1.
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 3:0] Sub-leaf Type 1. This sub-leaf enumerates an EPC section. EBX:EAX and EDX:ECX provide information on the
		* Enclave Page Cache (EPC) section.
		*/
		uint32_t sub_leaf_type                                         : 4;
#define CPUID_EAX_SUB_LEAF_TYPE_BIT                                  0
#define CPUID_EAX_SUB_LEAF_TYPE_FLAG                                 0x0F
#define CPUID_EAX_SUB_LEAF_TYPE_MASK                                 0x0F
#define CPUID_EAX_SUB_LEAF_TYPE(_)                                   (((_) >> 0) & 0x0F)
		uint32_t reserved1                                             : 8;

		/**
		* [Bits 31:12] Bits 31:12 of the physical address of the base of the EPC section.
		*/
		uint32_t epc_base_physical_address_1                           : 20;
#define CPUID_EAX_EPC_BASE_PHYSICAL_ADDRESS_1_BIT                    12
#define CPUID_EAX_EPC_BASE_PHYSICAL_ADDRESS_1_FLAG                   0xFFFFF000
#define CPUID_EAX_EPC_BASE_PHYSICAL_ADDRESS_1_MASK                   0xFFFFF
#define CPUID_EAX_EPC_BASE_PHYSICAL_ADDRESS_1(_)                     (((_) >> 12) & 0xFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 19:0] Bits 51:32 of the physical address of the base of the EPC section.
		*/
		uint32_t epc_base_physical_address_2                           : 20;
#define CPUID_EBX_EPC_BASE_PHYSICAL_ADDRESS_2_BIT                    0
#define CPUID_EBX_EPC_BASE_PHYSICAL_ADDRESS_2_FLAG                   0xFFFFF
#define CPUID_EBX_EPC_BASE_PHYSICAL_ADDRESS_2_MASK                   0xFFFFF
#define CPUID_EBX_EPC_BASE_PHYSICAL_ADDRESS_2(_)                     (((_) >> 0) & 0xFFFFF)
		uint32_t reserved1                                             : 12;
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 3:0] EPC section property encoding defined as follows:
		* - If EAX[3:0] 0000b, then all bits of the EDX:ECX pair are enumerated as 0.
		* - If EAX[3:0] 0001b, then this section has confidentiality and integrity protection.
		* All other encodings are reserved.
		*/
		uint32_t epc_section_property                                  : 4;
#define CPUID_ECX_EPC_SECTION_PROPERTY_BIT                           0
#define CPUID_ECX_EPC_SECTION_PROPERTY_FLAG                          0x0F
#define CPUID_ECX_EPC_SECTION_PROPERTY_MASK                          0x0F
#define CPUID_ECX_EPC_SECTION_PROPERTY(_)                            (((_) >> 0) & 0x0F)
		uint32_t reserved1                                             : 8;

		/**
		* [Bits 31:12] Bits 31:12 of the size of the corresponding EPC section within the Processor Reserved Memory.
		*/
		uint32_t epc_size_1                                            : 20;
#define CPUID_ECX_EPC_SIZE_1_BIT                                     12
#define CPUID_ECX_EPC_SIZE_1_FLAG                                    0xFFFFF000
#define CPUID_ECX_EPC_SIZE_1_MASK                                    0xFFFFF
#define CPUID_ECX_EPC_SIZE_1(_)                                      (((_) >> 12) & 0xFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 19:0] Bits 51:32 of the size of the corresponding EPC section within the Processor Reserved Memory.
		*/
		uint32_t epc_size_2                                            : 20;
#define CPUID_EDX_EPC_SIZE_2_BIT                                     0
#define CPUID_EDX_EPC_SIZE_2_FLAG                                    0xFFFFF
#define CPUID_EDX_EPC_SIZE_2_MASK                                    0xFFFFF
#define CPUID_EDX_EPC_SIZE_2(_)                                      (((_) >> 0) & 0xFFFFF)
		uint32_t reserved1                                             : 12;
	};

	uint32_t flags;
  } edx;

} cpuid_eax_12_ecx_02p_slt_1;

/**
 * @}
 */

/**
 * @defgroup cpuid_eax_14 \
 *           EAX = 0x14
 *
 * When CPUID executes with EAX set to 14H and ECX = 0H, the processor returns information about Intel Processor Trace
 * extensions.
 * When CPUID executes with EAX set to 14H and ECX = n (n > 0 and less than the number of non-zero bits in CPUID.(EAX=14H,
 * ECX= 0H).EAX), the processor returns information about packet generation in Intel Processor Trace.
 * @{
 */
#define CPUID_INTEL_PROCESSOR_TRACE_INFORMATION                      0x00000014
/**
 * @brief Intel Processor Trace Enumeration Main Leaf (EAX = 14H, ECX = 0)
 *
 * @note Leaf 14H main leaf (ECX = 0).
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] Reports the maximum sub-leaf supported in leaf 14H.
		*/
		uint32_t max_sub_leaf                                          : 32;
#define CPUID_EAX_MAX_SUB_LEAF_BIT                                   0
#define CPUID_EAX_MAX_SUB_LEAF_FLAG                                  0xFFFFFFFF
#define CPUID_EAX_MAX_SUB_LEAF_MASK                                  0xFFFFFFFF
#define CPUID_EAX_MAX_SUB_LEAF(_)                                    (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bit 0] If 1, indicates that IA32_RTIT_CTL.CR3Filter can be set to 1, and that IA32_RTIT_CR3_MATCH MSR can be accessed.
		*/
		uint32_t flag0                                                 : 1;
#define CPUID_EBX_FLAG0_BIT                                          0
#define CPUID_EBX_FLAG0_FLAG                                         0x01
#define CPUID_EBX_FLAG0_MASK                                         0x01
#define CPUID_EBX_FLAG0(_)                                           (((_) >> 0) & 0x01)

		/**
		* [Bit 1] If 1, indicates support of Configurable PSB and Cycle-Accurate Mode.
		*/
		uint32_t flag1                                                 : 1;
#define CPUID_EBX_FLAG1_BIT                                          1
#define CPUID_EBX_FLAG1_FLAG                                         0x02
#define CPUID_EBX_FLAG1_MASK                                         0x01
#define CPUID_EBX_FLAG1(_)                                           (((_) >> 1) & 0x01)

		/**
		* [Bit 2] If 1, indicates support of IP Filtering, TraceStop filtering, and preservation of Intel PT MSRs across warm
		* reset.
		*/
		uint32_t flag2                                                 : 1;
#define CPUID_EBX_FLAG2_BIT                                          2
#define CPUID_EBX_FLAG2_FLAG                                         0x04
#define CPUID_EBX_FLAG2_MASK                                         0x01
#define CPUID_EBX_FLAG2(_)                                           (((_) >> 2) & 0x01)

		/**
		* [Bit 3] If 1, indicates support of MTC timing packet and suppression of COFI-based packets.
		*/
		uint32_t flag3                                                 : 1;
#define CPUID_EBX_FLAG3_BIT                                          3
#define CPUID_EBX_FLAG3_FLAG                                         0x08
#define CPUID_EBX_FLAG3_MASK                                         0x01
#define CPUID_EBX_FLAG3(_)                                           (((_) >> 3) & 0x01)

		/**
		* [Bit 4] If 1, indicates support of PTWRITE. Writes can set IA32_RTIT_CTL[12] (PTWEn) and IA32_RTIT_CTL[5] (FUPonPTW),
		* and PTWRITE can generate packets.
		*/
		uint32_t flag4                                                 : 1;
#define CPUID_EBX_FLAG4_BIT                                          4
#define CPUID_EBX_FLAG4_FLAG                                         0x10
#define CPUID_EBX_FLAG4_MASK                                         0x01
#define CPUID_EBX_FLAG4(_)                                           (((_) >> 4) & 0x01)

		/**
		* [Bit 5] If 1, indicates support of Power Event Trace. Writes can set IA32_RTIT_CTL[4] (PwrEvtEn), enabling Power Event
		* Trace packet generation.
		*/
		uint32_t flag5                                                 : 1;
#define CPUID_EBX_FLAG5_BIT                                          5
#define CPUID_EBX_FLAG5_FLAG                                         0x20
#define CPUID_EBX_FLAG5_MASK                                         0x01
#define CPUID_EBX_FLAG5(_)                                           (((_) >> 5) & 0x01)
		uint32_t reserved1                                             : 26;
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bit 0] If 1, Tracing can be enabled with IA32_RTIT_CTL.ToPA = 1, hence utilizing the ToPA output scheme;
		* IA32_RTIT_OUTPUT_BASE and IA32_RTIT_OUTPUT_MASK_PTRS MSRs can be accessed.
		*/
		uint32_t flag0                                                 : 1;
#define CPUID_ECX_FLAG0_BIT                                          0
#define CPUID_ECX_FLAG0_FLAG                                         0x01
#define CPUID_ECX_FLAG0_MASK                                         0x01
#define CPUID_ECX_FLAG0(_)                                           (((_) >> 0) & 0x01)

		/**
		* [Bit 1] If 1, ToPA tables can hold any number of output entries, up to the maximum allowed by the MaskOrTableOffset
		* field of IA32_RTIT_OUTPUT_MASK_PTRS.
		*/
		uint32_t flag1                                                 : 1;
#define CPUID_ECX_FLAG1_BIT                                          1
#define CPUID_ECX_FLAG1_FLAG                                         0x02
#define CPUID_ECX_FLAG1_MASK                                         0x01
#define CPUID_ECX_FLAG1(_)                                           (((_) >> 1) & 0x01)

		/**
		* [Bit 2] If 1, indicates support of Single-Range Output scheme.
		*/
		uint32_t flag2                                                 : 1;
#define CPUID_ECX_FLAG2_BIT                                          2
#define CPUID_ECX_FLAG2_FLAG                                         0x04
#define CPUID_ECX_FLAG2_MASK                                         0x01
#define CPUID_ECX_FLAG2(_)                                           (((_) >> 2) & 0x01)

		/**
		* [Bit 3] If 1, indicates support of output to Trace Transport subsystem.
		*/
		uint32_t flag3                                                 : 1;
#define CPUID_ECX_FLAG3_BIT                                          3
#define CPUID_ECX_FLAG3_FLAG                                         0x08
#define CPUID_ECX_FLAG3_MASK                                         0x01
#define CPUID_ECX_FLAG3(_)                                           (((_) >> 3) & 0x01)
		uint32_t reserved1                                             : 27;

		/**
		* [Bit 31] If 1, generated packets which contain IP payloads have LIP values, which include the CS base component.
		*/
		uint32_t flag31                                                : 1;
#define CPUID_ECX_FLAG31_BIT                                         31
#define CPUID_ECX_FLAG31_FLAG                                        0x80000000
#define CPUID_ECX_FLAG31_MASK                                        0x01
#define CPUID_ECX_FLAG31(_)                                          (((_) >> 31) & 0x01)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EDX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EDX_RESERVED_BIT                                       0
#define CPUID_EDX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_14_ecx_00;

/**
 * @brief Intel Processor Trace Enumeration Sub-leaf (EAX = 14H, ECX = 1)
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 2:0] Number of configurable Address Ranges for filtering.
		*/
		uint32_t number_of_configurable_address_ranges_for_filtering   : 3;
#define CPUID_EAX_NUMBER_OF_CONFIGURABLE_ADDRESS_RANGES_FOR_FILTERING_BIT 0
#define CPUID_EAX_NUMBER_OF_CONFIGURABLE_ADDRESS_RANGES_FOR_FILTERING_FLAG 0x07
#define CPUID_EAX_NUMBER_OF_CONFIGURABLE_ADDRESS_RANGES_FOR_FILTERING_MASK 0x07
#define CPUID_EAX_NUMBER_OF_CONFIGURABLE_ADDRESS_RANGES_FOR_FILTERING(_) (((_) >> 0) & 0x07)
		uint32_t reserved1                                             : 13;

		/**
		* [Bits 31:16] Bitmap of supported MTC period encodings.
		*/
		uint32_t bitmap_of_supported_mtc_period_encodings              : 16;
#define CPUID_EAX_BITMAP_OF_SUPPORTED_MTC_PERIOD_ENCODINGS_BIT       16
#define CPUID_EAX_BITMAP_OF_SUPPORTED_MTC_PERIOD_ENCODINGS_FLAG      0xFFFF0000
#define CPUID_EAX_BITMAP_OF_SUPPORTED_MTC_PERIOD_ENCODINGS_MASK      0xFFFF
#define CPUID_EAX_BITMAP_OF_SUPPORTED_MTC_PERIOD_ENCODINGS(_)        (((_) >> 16) & 0xFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 15:0] Bitmap of supported Cycle Threshold value encodings.
		*/
		uint32_t bitmap_of_supported_cycle_threshold_value_encodings   : 16;
#define CPUID_EBX_BITMAP_OF_SUPPORTED_CYCLE_THRESHOLD_VALUE_ENCODINGS_BIT 0
#define CPUID_EBX_BITMAP_OF_SUPPORTED_CYCLE_THRESHOLD_VALUE_ENCODINGS_FLAG 0xFFFF
#define CPUID_EBX_BITMAP_OF_SUPPORTED_CYCLE_THRESHOLD_VALUE_ENCODINGS_MASK 0xFFFF
#define CPUID_EBX_BITMAP_OF_SUPPORTED_CYCLE_THRESHOLD_VALUE_ENCODINGS(_) (((_) >> 0) & 0xFFFF)

		/**
		* [Bits 31:16] Bitmap of supported Configurable PSB frequency encodings.
		*/
		uint32_t bitmap_of_supported_configurable_psb_frequency_encodings: 16;
#define CPUID_EBX_BITMAP_OF_SUPPORTED_CONFIGURABLE_PSB_FREQUENCY_ENCODINGS_BIT 16
#define CPUID_EBX_BITMAP_OF_SUPPORTED_CONFIGURABLE_PSB_FREQUENCY_ENCODINGS_FLAG 0xFFFF0000
#define CPUID_EBX_BITMAP_OF_SUPPORTED_CONFIGURABLE_PSB_FREQUENCY_ENCODINGS_MASK 0xFFFF
#define CPUID_EBX_BITMAP_OF_SUPPORTED_CONFIGURABLE_PSB_FREQUENCY_ENCODINGS(_) (((_) >> 16) & 0xFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] ECX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_ECX_RESERVED_BIT                                       0
#define CPUID_ECX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EDX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EDX_RESERVED_BIT                                       0
#define CPUID_EDX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_14_ecx_01;

/**
 * @}
 */


/**
 * @brief Stamp Counter and Nominal Core Crystal Clock Information Leaf
 *
 * When CPUID executes with EAX set to 15H and ECX = 0H, the processor returns information about Time Stamp Counter and
 * Core Crystal Clock.
 *
 * @note If EBX[31:0] is 0, the TSC/"core crystal clock" ratio is not enumerated. EBX[31:0]/EAX[31:0] indicates the ratio
 *       of the TSC frequency and the core crystal clock frequency.
 *       If ECX is 0, the nominal core crystal clock frequency is not enumerated. "TSC frequency" = "core crystal clock
 *       frequency" * EBX/EAX.
 */
#define CPUID_TIME_STAMP_COUNTER_INFORMATION                         0x00000015
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] An unsigned integer which is the denominator of the TSC/"core crystal clock" ratio.
		*/
		uint32_t denominator                                           : 32;
#define CPUID_EAX_DENOMINATOR_BIT                                    0
#define CPUID_EAX_DENOMINATOR_FLAG                                   0xFFFFFFFF
#define CPUID_EAX_DENOMINATOR_MASK                                   0xFFFFFFFF
#define CPUID_EAX_DENOMINATOR(_)                                     (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] An unsigned integer which is the numerator of the TSC/"core crystal clock" ratio.
		*/
		uint32_t numerator                                             : 32;
#define CPUID_EBX_NUMERATOR_BIT                                      0
#define CPUID_EBX_NUMERATOR_FLAG                                     0xFFFFFFFF
#define CPUID_EBX_NUMERATOR_MASK                                     0xFFFFFFFF
#define CPUID_EBX_NUMERATOR(_)                                       (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] An unsigned integer which is the nominal frequency of the core crystal clock in Hz.
		*/
		uint32_t nominal_frequency                                     : 32;
#define CPUID_ECX_NOMINAL_FREQUENCY_BIT                              0
#define CPUID_ECX_NOMINAL_FREQUENCY_FLAG                             0xFFFFFFFF
#define CPUID_ECX_NOMINAL_FREQUENCY_MASK                             0xFFFFFFFF
#define CPUID_ECX_NOMINAL_FREQUENCY(_)                               (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EDX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EDX_RESERVED_BIT                                       0
#define CPUID_EDX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_15;


/**
 * @brief Processor Frequency Information Leaf
 *
 * When CPUID executes with EAX set to 16H, the processor returns information about Processor Frequency Information.
 *
 * @note Data is returned from this interface in accordance with the processor's specification and does not reflect actual
 *       values. Suitable use of this data includes the display of processor information in like manner to the processor brand
 *       string and for determining the appropriate range to use when displaying processor information e.g. frequency history
 *       graphs. The returned information should not be used for any other purpose as the returned information does not
 *       accurately correlate to information / counters returned by other processor interfaces.
 *       While a processor may support the Processor Frequency Information leaf, fields that return a value of zero are not
 *       supported.
 */
#define CPUID_PROCESSOR_FREQUENCY_INFORMATION                        0x00000016
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 15:0] Processor Base Frequency (in MHz).
		*/
		uint32_t procesor_base_frequency_mhz                           : 16;
#define CPUID_EAX_PROCESOR_BASE_FREQUENCY_MHZ_BIT                    0
#define CPUID_EAX_PROCESOR_BASE_FREQUENCY_MHZ_FLAG                   0xFFFF
#define CPUID_EAX_PROCESOR_BASE_FREQUENCY_MHZ_MASK                   0xFFFF
#define CPUID_EAX_PROCESOR_BASE_FREQUENCY_MHZ(_)                     (((_) >> 0) & 0xFFFF)
		uint32_t reserved1                                             : 16;
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 15:0] Maximum Frequency (in MHz).
		*/
		uint32_t processor_maximum_frequency_mhz                       : 16;
#define CPUID_EBX_PROCESSOR_MAXIMUM_FREQUENCY_MHZ_BIT                0
#define CPUID_EBX_PROCESSOR_MAXIMUM_FREQUENCY_MHZ_FLAG               0xFFFF
#define CPUID_EBX_PROCESSOR_MAXIMUM_FREQUENCY_MHZ_MASK               0xFFFF
#define CPUID_EBX_PROCESSOR_MAXIMUM_FREQUENCY_MHZ(_)                 (((_) >> 0) & 0xFFFF)
		uint32_t reserved1                                             : 16;
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 15:0] Bus (Reference) Frequency (in MHz).
		*/
		uint32_t bus_frequency_mhz                                     : 16;
#define CPUID_ECX_BUS_FREQUENCY_MHZ_BIT                              0
#define CPUID_ECX_BUS_FREQUENCY_MHZ_FLAG                             0xFFFF
#define CPUID_ECX_BUS_FREQUENCY_MHZ_MASK                             0xFFFF
#define CPUID_ECX_BUS_FREQUENCY_MHZ(_)                               (((_) >> 0) & 0xFFFF)
		uint32_t reserved1                                             : 16;
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EDX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EDX_RESERVED_BIT                                       0
#define CPUID_EDX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_16;

/**
 * @defgroup cpuid_eax_17 \
 *           EAX = 0x17
 *
 * When CPUID executes with EAX set to 17H, the processor returns information about the System-On-Chip Vendor Attribute
 * Enumeration.
 * @{
 */
#define CPUID_SOC_VENDOR_INFORMATION                                 0x00000017
/**
 * @brief System-On-Chip Vendor Attribute Enumeration Main Leaf (EAX = 17H, ECX = 0)
 *
 * @note Leaf 17H main leaf (ECX = 0). Leaf 17H output depends on the initial value in ECX. Leaf 17H sub-leaves 1 through 3
 *       reports SOC Vendor Brand String. Leaf 17H is valid if MaxSOCID_Index >= 3. Leaf 17H sub-leaves 4 and above are reserved.
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] Reports the maximum input value of supported sub-leaf in leaf 17H.
		*/
		uint32_t max_soc_id_index                                      : 32;
#define CPUID_EAX_MAX_SOC_ID_INDEX_BIT                               0
#define CPUID_EAX_MAX_SOC_ID_INDEX_FLAG                              0xFFFFFFFF
#define CPUID_EAX_MAX_SOC_ID_INDEX_MASK                              0xFFFFFFFF
#define CPUID_EAX_MAX_SOC_ID_INDEX(_)                                (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 15:0] SOC Vendor ID.
		*/
		uint32_t soc_vendor_id                                         : 16;
#define CPUID_EBX_SOC_VENDOR_ID_BIT                                  0
#define CPUID_EBX_SOC_VENDOR_ID_FLAG                                 0xFFFF
#define CPUID_EBX_SOC_VENDOR_ID_MASK                                 0xFFFF
#define CPUID_EBX_SOC_VENDOR_ID(_)                                   (((_) >> 0) & 0xFFFF)

		/**
		* [Bit 16] If 1, the SOC Vendor ID field is assigned via an industry standard enumeration scheme. Otherwise, the SOC
		* Vendor ID field is assigned by Intel.
		*/
		uint32_t is_vendor_scheme                                      : 1;
#define CPUID_EBX_IS_VENDOR_SCHEME_BIT                               16
#define CPUID_EBX_IS_VENDOR_SCHEME_FLAG                              0x10000
#define CPUID_EBX_IS_VENDOR_SCHEME_MASK                              0x01
#define CPUID_EBX_IS_VENDOR_SCHEME(_)                                (((_) >> 16) & 0x01)
		uint32_t reserved1                                             : 15;
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] A unique number an SOC vendor assigns to its SOC projects.
		*/
		uint32_t project_id                                            : 32;
#define CPUID_ECX_PROJECT_ID_BIT                                     0
#define CPUID_ECX_PROJECT_ID_FLAG                                    0xFFFFFFFF
#define CPUID_ECX_PROJECT_ID_MASK                                    0xFFFFFFFF
#define CPUID_ECX_PROJECT_ID(_)                                      (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] A unique number within an SOC project that an SOC vendor assigns.
		*/
		uint32_t stepping_id                                           : 32;
#define CPUID_EDX_STEPPING_ID_BIT                                    0
#define CPUID_EDX_STEPPING_ID_FLAG                                   0xFFFFFFFF
#define CPUID_EDX_STEPPING_ID_MASK                                   0xFFFFFFFF
#define CPUID_EDX_STEPPING_ID(_)                                     (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_17_ecx_00;

/**
 * @brief System-On-Chip Vendor Attribute Enumeration Sub-leaf (EAX = 17H, ECX = 1..3)
 *
 * @note Leaf 17H output depends on the initial value in ECX. SOC Vendor Brand String is a UTF-8 encoded string padded with
 *       trailing bytes of 00H. The complete SOC Vendor Brand String is constructed by concatenating in ascending order of
 *       EAX:EBX:ECX:EDX and from the sub-leaf 1 fragment towards sub-leaf 3.
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] SOC Vendor Brand String. UTF-8 encoded string.
		*/
		uint32_t soc_vendor_brand_string                               : 32;
#define CPUID_EAX_SOC_VENDOR_BRAND_STRING_BIT                        0
#define CPUID_EAX_SOC_VENDOR_BRAND_STRING_FLAG                       0xFFFFFFFF
#define CPUID_EAX_SOC_VENDOR_BRAND_STRING_MASK                       0xFFFFFFFF
#define CPUID_EAX_SOC_VENDOR_BRAND_STRING(_)                         (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] SOC Vendor Brand String. UTF-8 encoded string.
		*/
		uint32_t soc_vendor_brand_string                               : 32;
#define CPUID_EBX_SOC_VENDOR_BRAND_STRING_BIT                        0
#define CPUID_EBX_SOC_VENDOR_BRAND_STRING_FLAG                       0xFFFFFFFF
#define CPUID_EBX_SOC_VENDOR_BRAND_STRING_MASK                       0xFFFFFFFF
#define CPUID_EBX_SOC_VENDOR_BRAND_STRING(_)                         (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] SOC Vendor Brand String. UTF-8 encoded string.
		*/
		uint32_t soc_vendor_brand_string                               : 32;
#define CPUID_ECX_SOC_VENDOR_BRAND_STRING_BIT                        0
#define CPUID_ECX_SOC_VENDOR_BRAND_STRING_FLAG                       0xFFFFFFFF
#define CPUID_ECX_SOC_VENDOR_BRAND_STRING_MASK                       0xFFFFFFFF
#define CPUID_ECX_SOC_VENDOR_BRAND_STRING(_)                         (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] SOC Vendor Brand String. UTF-8 encoded string.
		*/
		uint32_t soc_vendor_brand_string                               : 32;
#define CPUID_EDX_SOC_VENDOR_BRAND_STRING_BIT                        0
#define CPUID_EDX_SOC_VENDOR_BRAND_STRING_FLAG                       0xFFFFFFFF
#define CPUID_EDX_SOC_VENDOR_BRAND_STRING_MASK                       0xFFFFFFFF
#define CPUID_EDX_SOC_VENDOR_BRAND_STRING(_)                         (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_17_ecx_01_03;

/**
 * @brief System-On-Chip Vendor Attribute Enumeration Sub-leaves (EAX = 17H, ECX > MaxSOCID_Index)
 *
 * @note Leaf 17H output depends on the initial value in ECX.
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] Reserved = 0.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EAX_RESERVED_BIT                                       0
#define CPUID_EAX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EAX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EAX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Reserved = 0.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EBX_RESERVED_BIT                                       0
#define CPUID_EBX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EBX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EBX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Reserved = 0.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_ECX_RESERVED_BIT                                       0
#define CPUID_ECX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Reserved = 0.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EDX_RESERVED_BIT                                       0
#define CPUID_EDX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_17_ecx_n;

/**
 * @}
 */

/**
 * @defgroup cpuid_eax_18 \
 *           EAX = 0x18
 *
 * When CPUID executes with EAX set to 18H, the processor returns information about the Deterministic Address Translation
 * Parameters.
 * @{
 */
#define CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS           0x00000018
/**
 * @brief Deterministic Address Translation Parameters Main Leaf (EAX = 18H, ECX = 0)
 *
 * @note Each sub-leaf enumerates a different address translation structure.
 *       If ECX contains an invalid sub-leaf index, EAX/EBX/ECX/EDX return 0. Sub-leaf index n is invalid if n exceeds the value
 *       that sub-leaf 0 returns in EAX. A sub-leaf index is also invalid if EDX[4:0] returns 0. Valid sub-leaves do not need to
 *       be contiguous or in any particular order. A valid sub-leaf may be in a higher input ECX value than an invalid sub-leaf
 *       or than a valid sub-leaf of a higher or lower-level structure.
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] Reports the maximum input value of supported sub-leaf in leaf 18H.
		*/
		uint32_t max_sub_leaf                                          : 32;
#define CPUID_EAX_MAX_SUB_LEAF_BIT                                   0
#define CPUID_EAX_MAX_SUB_LEAF_FLAG                                  0xFFFFFFFF
#define CPUID_EAX_MAX_SUB_LEAF_MASK                                  0xFFFFFFFF
#define CPUID_EAX_MAX_SUB_LEAF(_)                                    (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bit 0] 4K page size entries supported by this structure.
		*/
		uint32_t page_entries_4kb_supported                            : 1;
#define CPUID_EBX_PAGE_ENTRIES_4KB_SUPPORTED_BIT                     0
#define CPUID_EBX_PAGE_ENTRIES_4KB_SUPPORTED_FLAG                    0x01
#define CPUID_EBX_PAGE_ENTRIES_4KB_SUPPORTED_MASK                    0x01
#define CPUID_EBX_PAGE_ENTRIES_4KB_SUPPORTED(_)                      (((_) >> 0) & 0x01)

		/**
		* [Bit 1] 2MB page size entries supported by this structure.
		*/
		uint32_t page_entries_2mb_supported                            : 1;
#define CPUID_EBX_PAGE_ENTRIES_2MB_SUPPORTED_BIT                     1
#define CPUID_EBX_PAGE_ENTRIES_2MB_SUPPORTED_FLAG                    0x02
#define CPUID_EBX_PAGE_ENTRIES_2MB_SUPPORTED_MASK                    0x01
#define CPUID_EBX_PAGE_ENTRIES_2MB_SUPPORTED(_)                      (((_) >> 1) & 0x01)

		/**
		* [Bit 2] 4MB page size entries supported by this structure.
		*/
		uint32_t page_entries_4mb_supported                            : 1;
#define CPUID_EBX_PAGE_ENTRIES_4MB_SUPPORTED_BIT                     2
#define CPUID_EBX_PAGE_ENTRIES_4MB_SUPPORTED_FLAG                    0x04
#define CPUID_EBX_PAGE_ENTRIES_4MB_SUPPORTED_MASK                    0x01
#define CPUID_EBX_PAGE_ENTRIES_4MB_SUPPORTED(_)                      (((_) >> 2) & 0x01)

		/**
		* [Bit 3] 1 GB page size entries supported by this structure.
		*/
		uint32_t page_entries_1gb_supported                            : 1;
#define CPUID_EBX_PAGE_ENTRIES_1GB_SUPPORTED_BIT                     3
#define CPUID_EBX_PAGE_ENTRIES_1GB_SUPPORTED_FLAG                    0x08
#define CPUID_EBX_PAGE_ENTRIES_1GB_SUPPORTED_MASK                    0x01
#define CPUID_EBX_PAGE_ENTRIES_1GB_SUPPORTED(_)                      (((_) >> 3) & 0x01)
		uint32_t reserved1                                             : 4;

		/**
		* [Bits 10:8] Partitioning (0: Soft partitioning between the logical processors sharing this structure).
		*/
		uint32_t partitioning                                          : 3;
#define CPUID_EBX_PARTITIONING_BIT                                   8
#define CPUID_EBX_PARTITIONING_FLAG                                  0x700
#define CPUID_EBX_PARTITIONING_MASK                                  0x07
#define CPUID_EBX_PARTITIONING(_)                                    (((_) >> 8) & 0x07)
		uint32_t reserved2                                             : 5;

		/**
		* [Bits 31:16] W = Ways of associativity.
		*/
		uint32_t ways_of_associativity_00                              : 16;
#define CPUID_EBX_WAYS_OF_ASSOCIATIVITY_00_BIT                       16
#define CPUID_EBX_WAYS_OF_ASSOCIATIVITY_00_FLAG                      0xFFFF0000
#define CPUID_EBX_WAYS_OF_ASSOCIATIVITY_00_MASK                      0xFFFF
#define CPUID_EBX_WAYS_OF_ASSOCIATIVITY_00(_)                        (((_) >> 16) & 0xFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Number of Sets.
		*/
		uint32_t number_of_sets                                        : 32;
#define CPUID_ECX_NUMBER_OF_SETS_BIT                                 0
#define CPUID_ECX_NUMBER_OF_SETS_FLAG                                0xFFFFFFFF
#define CPUID_ECX_NUMBER_OF_SETS_MASK                                0xFFFFFFFF
#define CPUID_ECX_NUMBER_OF_SETS(_)                                  (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 4:0] Translation cache type field.
		* - 00000b: Null (indicates this sub-leaf is not valid).
		* - 00001b: Data TLB.
		* - 00010b: Instruction TLB.
		* - 00011b: Unified TLB.
		* All other encodings are reserved.
		*
		* @note Some unified TLBs will allow a single TLB entry to satisfy data read/write and instruction fetches. Others will
		*       require separate entries (e.g., one loaded on data read/write and another loaded on an instruction fetch) . Please see
		*       the Intel(R) 64 and IA-32 Architectures Optimization Reference Manual for details of a particular product.
		*/
		uint32_t translation_cache_type_field                          : 5;
#define CPUID_EDX_TRANSLATION_CACHE_TYPE_FIELD_BIT                   0
#define CPUID_EDX_TRANSLATION_CACHE_TYPE_FIELD_FLAG                  0x1F
#define CPUID_EDX_TRANSLATION_CACHE_TYPE_FIELD_MASK                  0x1F
#define CPUID_EDX_TRANSLATION_CACHE_TYPE_FIELD(_)                    (((_) >> 0) & 0x1F)

		/**
		* [Bits 7:5] Translation cache level (starts at 1).
		*/
		uint32_t translation_cache_level                               : 3;
#define CPUID_EDX_TRANSLATION_CACHE_LEVEL_BIT                        5
#define CPUID_EDX_TRANSLATION_CACHE_LEVEL_FLAG                       0xE0
#define CPUID_EDX_TRANSLATION_CACHE_LEVEL_MASK                       0x07
#define CPUID_EDX_TRANSLATION_CACHE_LEVEL(_)                         (((_) >> 5) & 0x07)

		/**
		* [Bit 8] Fully associative structure.
		*/
		uint32_t fully_associative_structure                           : 1;
#define CPUID_EDX_FULLY_ASSOCIATIVE_STRUCTURE_BIT                    8
#define CPUID_EDX_FULLY_ASSOCIATIVE_STRUCTURE_FLAG                   0x100
#define CPUID_EDX_FULLY_ASSOCIATIVE_STRUCTURE_MASK                   0x01
#define CPUID_EDX_FULLY_ASSOCIATIVE_STRUCTURE(_)                     (((_) >> 8) & 0x01)
		uint32_t reserved1                                             : 5;

		/**
		* [Bits 25:14] Maximum number of addressable IDs for logical processors sharing this translation cache.
		*
		* @note Add one to the return value to get the result.
		*/
		uint32_t max_addressable_ids_for_logical_processors            : 12;
#define CPUID_EDX_MAX_ADDRESSABLE_IDS_FOR_LOGICAL_PROCESSORS_BIT     14
#define CPUID_EDX_MAX_ADDRESSABLE_IDS_FOR_LOGICAL_PROCESSORS_FLAG    0x3FFC000
#define CPUID_EDX_MAX_ADDRESSABLE_IDS_FOR_LOGICAL_PROCESSORS_MASK    0xFFF
#define CPUID_EDX_MAX_ADDRESSABLE_IDS_FOR_LOGICAL_PROCESSORS(_)      (((_) >> 14) & 0xFFF)
		uint32_t reserved2                                             : 6;
	};

	uint32_t flags;
  } edx;

} cpuid_eax_18_ecx_00;

/**
 * @brief Deterministic Address Translation Parameters Sub-leaf (EAX = 18H, ECX >= 1)
 *
 * @note Each sub-leaf enumerates a different address translation structure.
 *       If ECX contains an invalid sub-leaf index, EAX/EBX/ECX/EDX return 0. Sub-leaf index n is invalid if n exceeds the value
 *       that sub-leaf 0 returns in EAX. A sub-leaf index is also invalid if EDX[4:0] returns 0. Valid sub-leaves do not need to
 *       be contiguous or in any particular order. A valid sub-leaf may be in a higher input ECX value than an invalid sub-leaf
 *       or than a valid sub-leaf of a higher or lower-level structure.
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] EAX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EAX_RESERVED_BIT                                       0
#define CPUID_EAX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EAX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EAX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bit 0] 4K page size entries supported by this structure.
		*/
		uint32_t page_entries_4kb_supported                            : 1;
#define CPUID_EBX_PAGE_ENTRIES_4KB_SUPPORTED_BIT                     0
#define CPUID_EBX_PAGE_ENTRIES_4KB_SUPPORTED_FLAG                    0x01
#define CPUID_EBX_PAGE_ENTRIES_4KB_SUPPORTED_MASK                    0x01
#define CPUID_EBX_PAGE_ENTRIES_4KB_SUPPORTED(_)                      (((_) >> 0) & 0x01)

		/**
		* [Bit 1] 2MB page size entries supported by this structure.
		*/
		uint32_t page_entries_2mb_supported                            : 1;
#define CPUID_EBX_PAGE_ENTRIES_2MB_SUPPORTED_BIT                     1
#define CPUID_EBX_PAGE_ENTRIES_2MB_SUPPORTED_FLAG                    0x02
#define CPUID_EBX_PAGE_ENTRIES_2MB_SUPPORTED_MASK                    0x01
#define CPUID_EBX_PAGE_ENTRIES_2MB_SUPPORTED(_)                      (((_) >> 1) & 0x01)

		/**
		* [Bit 2] 4MB page size entries supported by this structure.
		*/
		uint32_t page_entries_4mb_supported                            : 1;
#define CPUID_EBX_PAGE_ENTRIES_4MB_SUPPORTED_BIT                     2
#define CPUID_EBX_PAGE_ENTRIES_4MB_SUPPORTED_FLAG                    0x04
#define CPUID_EBX_PAGE_ENTRIES_4MB_SUPPORTED_MASK                    0x01
#define CPUID_EBX_PAGE_ENTRIES_4MB_SUPPORTED(_)                      (((_) >> 2) & 0x01)

		/**
		* [Bit 3] 1 GB page size entries supported by this structure.
		*/
		uint32_t page_entries_1gb_supported                            : 1;
#define CPUID_EBX_PAGE_ENTRIES_1GB_SUPPORTED_BIT                     3
#define CPUID_EBX_PAGE_ENTRIES_1GB_SUPPORTED_FLAG                    0x08
#define CPUID_EBX_PAGE_ENTRIES_1GB_SUPPORTED_MASK                    0x01
#define CPUID_EBX_PAGE_ENTRIES_1GB_SUPPORTED(_)                      (((_) >> 3) & 0x01)
		uint32_t reserved1                                             : 4;

		/**
		* [Bits 10:8] Partitioning (0: Soft partitioning between the logical processors sharing this structure).
		*/
		uint32_t partitioning                                          : 3;
#define CPUID_EBX_PARTITIONING_BIT                                   8
#define CPUID_EBX_PARTITIONING_FLAG                                  0x700
#define CPUID_EBX_PARTITIONING_MASK                                  0x07
#define CPUID_EBX_PARTITIONING(_)                                    (((_) >> 8) & 0x07)
		uint32_t reserved2                                             : 5;

		/**
		* [Bits 31:16] W = Ways of associativity.
		*/
		uint32_t ways_of_associativity_01                              : 16;
#define CPUID_EBX_WAYS_OF_ASSOCIATIVITY_01_BIT                       16
#define CPUID_EBX_WAYS_OF_ASSOCIATIVITY_01_FLAG                      0xFFFF0000
#define CPUID_EBX_WAYS_OF_ASSOCIATIVITY_01_MASK                      0xFFFF
#define CPUID_EBX_WAYS_OF_ASSOCIATIVITY_01(_)                        (((_) >> 16) & 0xFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Number of Sets.
		*/
		uint32_t number_of_sets                                        : 32;
#define CPUID_ECX_NUMBER_OF_SETS_BIT                                 0
#define CPUID_ECX_NUMBER_OF_SETS_FLAG                                0xFFFFFFFF
#define CPUID_ECX_NUMBER_OF_SETS_MASK                                0xFFFFFFFF
#define CPUID_ECX_NUMBER_OF_SETS(_)                                  (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 4:0] Translation cache type field.
		* - 00000b: Null (indicates this sub-leaf is not valid).
		* - 00001b: Data TLB.
		* - 00010b: Instruction TLB.
		* - 00011b: Unified TLB.
		* All other encodings are reserved.
		*
		* @note Some unified TLBs will allow a single TLB entry to satisfy data read/write and instruction fetches. Others will
		*       require separate entries (e.g., one loaded on data read/write and another loaded on an instruction fetch) . Please see
		*       the Intel(R) 64 and IA-32 Architectures Optimization Reference Manual for details of a particular product.
		*/
		uint32_t translation_cache_type_field                          : 5;
#define CPUID_EDX_TRANSLATION_CACHE_TYPE_FIELD_BIT                   0
#define CPUID_EDX_TRANSLATION_CACHE_TYPE_FIELD_FLAG                  0x1F
#define CPUID_EDX_TRANSLATION_CACHE_TYPE_FIELD_MASK                  0x1F
#define CPUID_EDX_TRANSLATION_CACHE_TYPE_FIELD(_)                    (((_) >> 0) & 0x1F)

		/**
		* [Bits 7:5] Translation cache level (starts at 1).
		*/
		uint32_t translation_cache_level                               : 3;
#define CPUID_EDX_TRANSLATION_CACHE_LEVEL_BIT                        5
#define CPUID_EDX_TRANSLATION_CACHE_LEVEL_FLAG                       0xE0
#define CPUID_EDX_TRANSLATION_CACHE_LEVEL_MASK                       0x07
#define CPUID_EDX_TRANSLATION_CACHE_LEVEL(_)                         (((_) >> 5) & 0x07)

		/**
		* [Bit 8] Fully associative structure.
		*/
		uint32_t fully_associative_structure                           : 1;
#define CPUID_EDX_FULLY_ASSOCIATIVE_STRUCTURE_BIT                    8
#define CPUID_EDX_FULLY_ASSOCIATIVE_STRUCTURE_FLAG                   0x100
#define CPUID_EDX_FULLY_ASSOCIATIVE_STRUCTURE_MASK                   0x01
#define CPUID_EDX_FULLY_ASSOCIATIVE_STRUCTURE(_)                     (((_) >> 8) & 0x01)
		uint32_t reserved1                                             : 5;

		/**
		* [Bits 25:14] Maximum number of addressable IDs for logical processors sharing this translation cache.
		*
		* @note Add one to the return value to get the result.
		*/
		uint32_t max_addressable_ids_for_logical_processors            : 12;
#define CPUID_EDX_MAX_ADDRESSABLE_IDS_FOR_LOGICAL_PROCESSORS_BIT     14
#define CPUID_EDX_MAX_ADDRESSABLE_IDS_FOR_LOGICAL_PROCESSORS_FLAG    0x3FFC000
#define CPUID_EDX_MAX_ADDRESSABLE_IDS_FOR_LOGICAL_PROCESSORS_MASK    0xFFF
#define CPUID_EDX_MAX_ADDRESSABLE_IDS_FOR_LOGICAL_PROCESSORS(_)      (((_) >> 14) & 0xFFF)
		uint32_t reserved2                                             : 6;
	};

	uint32_t flags;
  } edx;

} cpuid_eax_18_ecx_01p;

/**
 * @}
 */


/**
 * @brief Extended Function CPUID Information
 *
 * When CPUID executes with EAX set to 80000000H, the processor returns the highest value the processor recognizes for
 * returning extended processor information. The value is returned in the EAX register and is processor specific.
 */
#define CPUID_EXTENDED_FUNCTION_INFORMATION                          0x80000000
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] Maximum Input Value for Extended Function CPUID Information.
		*/
		uint32_t max_extended_functions                                : 32;
#define CPUID_EAX_MAX_EXTENDED_FUNCTIONS_BIT                         0
#define CPUID_EAX_MAX_EXTENDED_FUNCTIONS_FLAG                        0xFFFFFFFF
#define CPUID_EAX_MAX_EXTENDED_FUNCTIONS_MASK                        0xFFFFFFFF
#define CPUID_EAX_MAX_EXTENDED_FUNCTIONS(_)                          (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EBX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EBX_RESERVED_BIT                                       0
#define CPUID_EBX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EBX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EBX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] ECX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_ECX_RESERVED_BIT                                       0
#define CPUID_ECX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EDX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EDX_RESERVED_BIT                                       0
#define CPUID_EDX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_80000000;


/**
 * Extended Function CPUID Information.
 */
#define CPUID_EXTENDED_CPU_SIGNATURE                                 0x80000001
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] EAX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EAX_RESERVED_BIT                                       0
#define CPUID_EAX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EAX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EAX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EBX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EBX_RESERVED_BIT                                       0
#define CPUID_EBX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EBX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EBX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bit 0] LAHF/SAHF available in 64-bit mode.
		*/
		uint32_t lahf_sahf_available_in_64_bit_mode                    : 1;
#define CPUID_ECX_LAHF_SAHF_AVAILABLE_IN_64_BIT_MODE_BIT             0
#define CPUID_ECX_LAHF_SAHF_AVAILABLE_IN_64_BIT_MODE_FLAG            0x01
#define CPUID_ECX_LAHF_SAHF_AVAILABLE_IN_64_BIT_MODE_MASK            0x01
#define CPUID_ECX_LAHF_SAHF_AVAILABLE_IN_64_BIT_MODE(_)              (((_) >> 0) & 0x01)
		uint32_t reserved1                                             : 4;

		/**
		* [Bit 5] LZCNT.
		*/
		uint32_t lzcnt                                                 : 1;
#define CPUID_ECX_LZCNT_BIT                                          5
#define CPUID_ECX_LZCNT_FLAG                                         0x20
#define CPUID_ECX_LZCNT_MASK                                         0x01
#define CPUID_ECX_LZCNT(_)                                           (((_) >> 5) & 0x01)
		uint32_t reserved2                                             : 2;

		/**
		* [Bit 8] PREFETCHW.
		*/
		uint32_t prefetchw                                             : 1;
#define CPUID_ECX_PREFETCHW_BIT                                      8
#define CPUID_ECX_PREFETCHW_FLAG                                     0x100
#define CPUID_ECX_PREFETCHW_MASK                                     0x01
#define CPUID_ECX_PREFETCHW(_)                                       (((_) >> 8) & 0x01)
		uint32_t reserved3                                             : 23;
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		uint32_t reserved1                                             : 11;

		/**
		* [Bit 11] SYSCALL/SYSRET available in 64-bit mode.
		*/
		uint32_t syscall_sysret_available_in_64_bit_mode               : 1;
#define CPUID_EDX_SYSCALL_SYSRET_AVAILABLE_IN_64_BIT_MODE_BIT        11
#define CPUID_EDX_SYSCALL_SYSRET_AVAILABLE_IN_64_BIT_MODE_FLAG       0x800
#define CPUID_EDX_SYSCALL_SYSRET_AVAILABLE_IN_64_BIT_MODE_MASK       0x01
#define CPUID_EDX_SYSCALL_SYSRET_AVAILABLE_IN_64_BIT_MODE(_)         (((_) >> 11) & 0x01)
		uint32_t reserved2                                             : 8;

		/**
		* [Bit 20] Execute Disable Bit available.
		*/
		uint32_t execute_disable_bit_available                         : 1;
#define CPUID_EDX_EXECUTE_DISABLE_BIT_AVAILABLE_BIT                  20
#define CPUID_EDX_EXECUTE_DISABLE_BIT_AVAILABLE_FLAG                 0x100000
#define CPUID_EDX_EXECUTE_DISABLE_BIT_AVAILABLE_MASK                 0x01
#define CPUID_EDX_EXECUTE_DISABLE_BIT_AVAILABLE(_)                   (((_) >> 20) & 0x01)
		uint32_t reserved3                                             : 5;

		/**
		* [Bit 26] 1-GByte pages are available if 1.
		*/
		uint32_t pages_1gb_available                                   : 1;
#define CPUID_EDX_PAGES_1GB_AVAILABLE_BIT                            26
#define CPUID_EDX_PAGES_1GB_AVAILABLE_FLAG                           0x4000000
#define CPUID_EDX_PAGES_1GB_AVAILABLE_MASK                           0x01
#define CPUID_EDX_PAGES_1GB_AVAILABLE(_)                             (((_) >> 26) & 0x01)

		/**
		* [Bit 27] RDTSCP and IA32_TSC_AUX are available if 1.
		*/
		uint32_t rdtscp_available                                      : 1;
#define CPUID_EDX_RDTSCP_AVAILABLE_BIT                               27
#define CPUID_EDX_RDTSCP_AVAILABLE_FLAG                              0x8000000
#define CPUID_EDX_RDTSCP_AVAILABLE_MASK                              0x01
#define CPUID_EDX_RDTSCP_AVAILABLE(_)                                (((_) >> 27) & 0x01)
		uint32_t reserved4                                             : 1;

		/**
		* [Bit 29] Intel(R) 64 Architecture available if 1.
		*/
		uint32_t ia64_available                                        : 1;
#define CPUID_EDX_IA64_AVAILABLE_BIT                                 29
#define CPUID_EDX_IA64_AVAILABLE_FLAG                                0x20000000
#define CPUID_EDX_IA64_AVAILABLE_MASK                                0x01
#define CPUID_EDX_IA64_AVAILABLE(_)                                  (((_) >> 29) & 0x01)
		uint32_t reserved5                                             : 2;
	};

	uint32_t flags;
  } edx;

} cpuid_eax_80000001;


/**
 * Extended Function CPUID Information.
 */
#define CPUID_BRAND_STRING1                                          0x80000002

/**
 * Extended Function CPUID Information.
 */
#define CPUID_BRAND_STRING2                                          0x80000003

/**
 * Extended Function CPUID Information.
 */
#define CPUID_BRAND_STRING3                                          0x80000004
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] Processor Brand String.
		*/
		uint32_t processor_brand_string_1                              : 32;
#define CPUID_EAX_PROCESSOR_BRAND_STRING_1_BIT                       0
#define CPUID_EAX_PROCESSOR_BRAND_STRING_1_FLAG                      0xFFFFFFFF
#define CPUID_EAX_PROCESSOR_BRAND_STRING_1_MASK                      0xFFFFFFFF
#define CPUID_EAX_PROCESSOR_BRAND_STRING_1(_)                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Processor Brand String Continued.
		*/
		uint32_t processor_brand_string_2                              : 32;
#define CPUID_EBX_PROCESSOR_BRAND_STRING_2_BIT                       0
#define CPUID_EBX_PROCESSOR_BRAND_STRING_2_FLAG                      0xFFFFFFFF
#define CPUID_EBX_PROCESSOR_BRAND_STRING_2_MASK                      0xFFFFFFFF
#define CPUID_EBX_PROCESSOR_BRAND_STRING_2(_)                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Processor Brand String Continued.
		*/
		uint32_t processor_brand_string_3                              : 32;
#define CPUID_ECX_PROCESSOR_BRAND_STRING_3_BIT                       0
#define CPUID_ECX_PROCESSOR_BRAND_STRING_3_FLAG                      0xFFFFFFFF
#define CPUID_ECX_PROCESSOR_BRAND_STRING_3_MASK                      0xFFFFFFFF
#define CPUID_ECX_PROCESSOR_BRAND_STRING_3(_)                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Processor Brand String Continued.
		*/
		uint32_t processor_brand_string_4                              : 32;
#define CPUID_EDX_PROCESSOR_BRAND_STRING_4_BIT                       0
#define CPUID_EDX_PROCESSOR_BRAND_STRING_4_FLAG                      0xFFFFFFFF
#define CPUID_EDX_PROCESSOR_BRAND_STRING_4_MASK                      0xFFFFFFFF
#define CPUID_EDX_PROCESSOR_BRAND_STRING_4(_)                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_80000002;

/**
 * @brief Extended Function CPUID Information
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] Processor Brand String Continued.
		*/
		uint32_t processor_brand_string_5                              : 32;
#define CPUID_EAX_PROCESSOR_BRAND_STRING_5_BIT                       0
#define CPUID_EAX_PROCESSOR_BRAND_STRING_5_FLAG                      0xFFFFFFFF
#define CPUID_EAX_PROCESSOR_BRAND_STRING_5_MASK                      0xFFFFFFFF
#define CPUID_EAX_PROCESSOR_BRAND_STRING_5(_)                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Processor Brand String Continued.
		*/
		uint32_t processor_brand_string_6                              : 32;
#define CPUID_EBX_PROCESSOR_BRAND_STRING_6_BIT                       0
#define CPUID_EBX_PROCESSOR_BRAND_STRING_6_FLAG                      0xFFFFFFFF
#define CPUID_EBX_PROCESSOR_BRAND_STRING_6_MASK                      0xFFFFFFFF
#define CPUID_EBX_PROCESSOR_BRAND_STRING_6(_)                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Processor Brand String Continued.
		*/
		uint32_t processor_brand_string_7                              : 32;
#define CPUID_ECX_PROCESSOR_BRAND_STRING_7_BIT                       0
#define CPUID_ECX_PROCESSOR_BRAND_STRING_7_FLAG                      0xFFFFFFFF
#define CPUID_ECX_PROCESSOR_BRAND_STRING_7_MASK                      0xFFFFFFFF
#define CPUID_ECX_PROCESSOR_BRAND_STRING_7(_)                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Processor Brand String Continued.
		*/
		uint32_t processor_brand_string_8                              : 32;
#define CPUID_EDX_PROCESSOR_BRAND_STRING_8_BIT                       0
#define CPUID_EDX_PROCESSOR_BRAND_STRING_8_FLAG                      0xFFFFFFFF
#define CPUID_EDX_PROCESSOR_BRAND_STRING_8_MASK                      0xFFFFFFFF
#define CPUID_EDX_PROCESSOR_BRAND_STRING_8(_)                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_80000003;

/**
 * @brief Extended Function CPUID Information
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] Processor Brand String Continued.
		*/
		uint32_t processor_brand_string_9                              : 32;
#define CPUID_EAX_PROCESSOR_BRAND_STRING_9_BIT                       0
#define CPUID_EAX_PROCESSOR_BRAND_STRING_9_FLAG                      0xFFFFFFFF
#define CPUID_EAX_PROCESSOR_BRAND_STRING_9_MASK                      0xFFFFFFFF
#define CPUID_EAX_PROCESSOR_BRAND_STRING_9(_)                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Processor Brand String Continued.
		*/
		uint32_t processor_brand_string_10                             : 32;
#define CPUID_EBX_PROCESSOR_BRAND_STRING_10_BIT                      0
#define CPUID_EBX_PROCESSOR_BRAND_STRING_10_FLAG                     0xFFFFFFFF
#define CPUID_EBX_PROCESSOR_BRAND_STRING_10_MASK                     0xFFFFFFFF
#define CPUID_EBX_PROCESSOR_BRAND_STRING_10(_)                       (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Processor Brand String Continued.
		*/
		uint32_t processor_brand_string_11                             : 32;
#define CPUID_ECX_PROCESSOR_BRAND_STRING_11_BIT                      0
#define CPUID_ECX_PROCESSOR_BRAND_STRING_11_FLAG                     0xFFFFFFFF
#define CPUID_ECX_PROCESSOR_BRAND_STRING_11_MASK                     0xFFFFFFFF
#define CPUID_ECX_PROCESSOR_BRAND_STRING_11(_)                       (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] Processor Brand String Continued.
		*/
		uint32_t processor_brand_string_12                             : 32;
#define CPUID_EDX_PROCESSOR_BRAND_STRING_12_BIT                      0
#define CPUID_EDX_PROCESSOR_BRAND_STRING_12_FLAG                     0xFFFFFFFF
#define CPUID_EDX_PROCESSOR_BRAND_STRING_12_MASK                     0xFFFFFFFF
#define CPUID_EDX_PROCESSOR_BRAND_STRING_12(_)                       (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_80000004;

/**
 * @brief Extended Function CPUID Information
 */
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] EAX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EAX_RESERVED_BIT                                       0
#define CPUID_EAX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EAX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EAX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EBX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EBX_RESERVED_BIT                                       0
#define CPUID_EBX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EBX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EBX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] ECX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_ECX_RESERVED_BIT                                       0
#define CPUID_ECX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EDX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EDX_RESERVED_BIT                                       0
#define CPUID_EDX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_80000005;


/**
 * Extended Function CPUID Information.
 */
#define CPUID_EXTENDED_CACHE_INFO                                    0x80000006
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] EAX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EAX_RESERVED_BIT                                       0
#define CPUID_EAX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EAX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EAX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EBX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EBX_RESERVED_BIT                                       0
#define CPUID_EBX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EBX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EBX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 7:0] Cache Line size in bytes.
		*/
		uint32_t cache_line_size_in_bytes                              : 8;
#define CPUID_ECX_CACHE_LINE_SIZE_IN_BYTES_BIT                       0
#define CPUID_ECX_CACHE_LINE_SIZE_IN_BYTES_FLAG                      0xFF
#define CPUID_ECX_CACHE_LINE_SIZE_IN_BYTES_MASK                      0xFF
#define CPUID_ECX_CACHE_LINE_SIZE_IN_BYTES(_)                        (((_) >> 0) & 0xFF)
		uint32_t reserved1                                             : 4;

		/**
		* [Bits 15:12] L2 Associativity field.
		* L2 associativity field encodings:
		* - 00H - Disabled.
		* - 01H - Direct mapped.
		* - 02H - 2-way.
		* - 04H - 4-way.
		* - 06H - 8-way.
		* - 08H - 16-way.
		* - 0FH - Fully associative.
		*/
		uint32_t l2_associativity_field                                : 4;
#define CPUID_ECX_L2_ASSOCIATIVITY_FIELD_BIT                         12
#define CPUID_ECX_L2_ASSOCIATIVITY_FIELD_FLAG                        0xF000
#define CPUID_ECX_L2_ASSOCIATIVITY_FIELD_MASK                        0x0F
#define CPUID_ECX_L2_ASSOCIATIVITY_FIELD(_)                          (((_) >> 12) & 0x0F)

		/**
		* [Bits 31:16] Cache size in 1K units.
		*/
		uint32_t cache_size_in_1k_units                                : 16;
#define CPUID_ECX_CACHE_SIZE_IN_1K_UNITS_BIT                         16
#define CPUID_ECX_CACHE_SIZE_IN_1K_UNITS_FLAG                        0xFFFF0000
#define CPUID_ECX_CACHE_SIZE_IN_1K_UNITS_MASK                        0xFFFF
#define CPUID_ECX_CACHE_SIZE_IN_1K_UNITS(_)                          (((_) >> 16) & 0xFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EDX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EDX_RESERVED_BIT                                       0
#define CPUID_EDX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EDX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } edx;

} cpuid_eax_80000006;


/**
 * Extended Function CPUID Information.
 */
#define CPUID_EXTENDED_TIME_STAMP_COUNTER                            0x80000007
typedef struct
{
  union
  {
	struct
	{
		/**
		* [Bits 31:0] EAX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EAX_RESERVED_BIT                                       0
#define CPUID_EAX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EAX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EAX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } eax;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] EBX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_EBX_RESERVED_BIT                                       0
#define CPUID_EBX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_EBX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_EBX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ebx;

  union
  {
	struct
	{
		/**
		* [Bits 31:0] ECX is reserved.
		*/
		uint32_t reserved                                              : 32;
#define CPUID_ECX_RESERVED_BIT                                       0
#define CPUID_ECX_RESERVED_FLAG                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED_MASK                                      0xFFFFFFFF
#define CPUID_ECX_RESERVED(_)                                        (((_) >> 0) & 0xFFFFFFFF)
	};

	uint32_t flags;
  } ecx;

  union
  {
	struct
	{
		uint32_t reserved1                                             : 8;

		/**
		* [Bit 8] Invariant TSC available if 1.
		*/
		uint32_t invariant_tsc_available                               : 1;
#define CPUID_EDX_INVARIANT_TSC_AVAILABLE_BIT                        8
#define CPUID_EDX_INVARIANT_TSC_AVAILABLE_FLAG                       0x100
#define CPUID_EDX_INVARIANT_TSC_AVAILABLE_MASK                       0x01
#define CPUID_EDX_INVARIANT_TSC_AVAILABLE(_)                         (((_) >> 8) & 0x01)
		uint32_t reserved2                                             : 23;
	};

	uint32_t flags;
  } edx;

} cpuid_eax_80000007;

/**
 * @}
 */

/**
 * @defgroup model_specific_registers \
 *           Model Specific Registers
 *
 * @see Vol2A[3.2(CPUID)] (reference)
 * @{
 */
/**
 * @defgroup ia32_p5_mc \
 *           IA32_P5_MC_(x)
 *
 * When machine-check exceptions are enabled for the Pentium processor (MCE flag is set in control register CR4), the
 * machine-check exception handler uses the RDMSR instruction to read the error type from the P5_MC_TYPE register and the
 * machine check address from the P5_MC_ADDR register. The handler then normally reports these register values to the
 * system console before aborting execution.
 *
 * @see Vol3B[15.10.2(Pentium Processor Machine-Check Exception Handling)] (reference)
 * @{
 */
/**
 * Machine-check exception address.
 *
 * @remarks 05_01H
 * @see Vol4[2.22(MSRS IN PENTIUM PROCESSORS)]
 */
#define IA32_P5_MC_ADDR                                              0x00000000

/**
 * Machine-check exception type.
 *
 * @remarks 05_01H
 * @see Vol4[2.22(MSRS IN PENTIUM PROCESSORS)]
 */
#define IA32_P5_MC_TYPE                                              0x00000001
/**
 * @}
 */

/**
 * System coherence line size.
 *
 * @remarks 0F_03H
 * @see Vol3A[8.10.5(Monitor/Mwait Address Range Determination)]
 * @see Vol3A[8.10.5(Monitor/Mwait Address Range Determination)] (reference)
 */
#define IA32_MONITOR_FILTER_LINE_SIZE                                0x00000006

/**
 * Value as returned by instruction RDTSC.
 *
 * @remarks 05_01H
 * @see Vol3B[17.17(TIME-STAMP COUNTER)]
 */
#define IA32_TIME_STAMP_COUNTER                                      0x00000010

/**
 * The operating system can use this MSR to determine "slot" information for the processor and the proper microcode update
 * to load.
 *
 * @remarks 06_01H
 */
#define IA32_PLATFORM_ID                                             0x00000017
typedef union
{
  struct
  {
	uint64_t reserved1                                               : 50;

	/**
	* @brief Platform Id <b>(RO)</b>
	*
	* [Bits 52:50] Contains information concerning the intended platform for the processor.
	* 
	* 
	* 52 | 51 | 50 | _
	* --:|:--:|:---|-----------------
	* 0  | 0  | 0  | Processor Flag 0
	* 0  | 0  | 1  | Processor Flag 1
	* 0  | 1  | 0  | Processor Flag 2
	* 0  | 1  | 1  | Processor Flag 3
	* 1  | 0  | 0  | Processor Flag 4
	* 1  | 0  | 1  | Processor Flag 5
	* 1  | 1  | 0  | Processor Flag 6
	* 1  | 1  | 1  | Processor Flag 7
	*/
	uint64_t platform_id                                             : 3;
#define IA32_PLATFORM_ID_PLATFORM_ID_BIT                             50
#define IA32_PLATFORM_ID_PLATFORM_ID_FLAG                            0x1C000000000000
#define IA32_PLATFORM_ID_PLATFORM_ID_MASK                            0x07
#define IA32_PLATFORM_ID_PLATFORM_ID(_)                              (((_) >> 50) & 0x07)
	uint64_t reserved2                                               : 11;
  };

  uint64_t flags;
} platform_id_register;


/**
 * This register holds the APIC base address, permitting the relocation of the APIC memory map.
 *
 * @remarks 06_01H
 * @see Vol3A[10.4.4(Local APIC Status and Location)]
 * @see Vol3A[10.4.5(Relocating the Local APIC Registers)]
 */
#define IA32_APIC_BASE                                               0x0000001B
typedef union
{
  struct
  {
	uint64_t reserved1                                               : 8;

	/**
	* [Bit 8] BSP flag.
	*/
	uint64_t bsp_flag                                                : 1;
#define IA32_APIC_BASE_BSP_FLAG_BIT                                  8
#define IA32_APIC_BASE_BSP_FLAG_FLAG                                 0x100
#define IA32_APIC_BASE_BSP_FLAG_MASK                                 0x01
#define IA32_APIC_BASE_BSP_FLAG(_)                                   (((_) >> 8) & 0x01)
	uint64_t reserved2                                               : 1;

	/**
	* [Bit 10] Enable x2APIC mode.
	*/
	uint64_t enable_x2apic_mode                                      : 1;
#define IA32_APIC_BASE_ENABLE_X2APIC_MODE_BIT                        10
#define IA32_APIC_BASE_ENABLE_X2APIC_MODE_FLAG                       0x400
#define IA32_APIC_BASE_ENABLE_X2APIC_MODE_MASK                       0x01
#define IA32_APIC_BASE_ENABLE_X2APIC_MODE(_)                         (((_) >> 10) & 0x01)

	/**
	* [Bit 11] APIC Global Enable.
	*/
	uint64_t apic_global_enable                                      : 1;
#define IA32_APIC_BASE_APIC_GLOBAL_ENABLE_BIT                        11
#define IA32_APIC_BASE_APIC_GLOBAL_ENABLE_FLAG                       0x800
#define IA32_APIC_BASE_APIC_GLOBAL_ENABLE_MASK                       0x01
#define IA32_APIC_BASE_APIC_GLOBAL_ENABLE(_)                         (((_) >> 11) & 0x01)

	/**
	* [Bits 47:12] APIC Base.
	*/
	uint64_t apic_base                                               : 36;
#define IA32_APIC_BASE_APIC_BASE_BIT                                 12
#define IA32_APIC_BASE_APIC_BASE_FLAG                                0xFFFFFFFFF000
#define IA32_APIC_BASE_APIC_BASE_MASK                                0xFFFFFFFFF
#define IA32_APIC_BASE_APIC_BASE(_)                                  (((_) >> 12) & 0xFFFFFFFFF)
	uint64_t reserved3                                               : 16;
  };

  uint64_t flags;
} apic_base_register;


/**
 * Control Features in Intel 64 Processor.
 *
 * @remarks If any one enumeration condition for defined bit field holds.
 */
#define IA32_FEATURE_CONTROL                                         0x0000003A
typedef union
{
  struct
  {
	/**
	* @brief Lock bit <b>(R/WO)</b>
	*
	* [Bit 0] When set, locks this MSR from being written; writes to this bit will result in GP(0).
	*
	* @note Once the Lock bit is set, the contents of this register cannot be modified. Therefore the lock bit must be set
	*       after configuring support for Intel Virtualization Technology and prior to transferring control to an option ROM or the
	*       OS. Hence, once the Lock bit is set, the entire IA32_FEATURE_CONTROL contents are preserved across RESET when PWRGOOD is
	*       not deasserted.
	* @remarks If any one enumeration condition for defined bit field position greater than bit 0 holds.
	*/
	uint64_t lock_bit                                                : 1;
#define IA32_FEATURE_CONTROL_LOCK_BIT_BIT                            0
#define IA32_FEATURE_CONTROL_LOCK_BIT_FLAG                           0x01
#define IA32_FEATURE_CONTROL_LOCK_BIT_MASK                           0x01
#define IA32_FEATURE_CONTROL_LOCK_BIT(_)                             (((_) >> 0) & 0x01)

	/**
	* @brief Enable VMX inside SMX operation <b>(R/WL)</b>
	*
	* [Bit 1] This bit enables a system executive to use VMX in conjunction with SMX to support Intel(R) Trusted Execution
	* Technology. BIOS must set this bit only when the CPUID function 1 returns VMX feature flag and SMX feature flag set (ECX
	* bits 5 and 6 respectively).
	*
	* @remarks If CPUID.01H:ECX[5] = 1 && CPUID.01H:ECX[6] = 1
	*/
	uint64_t enable_vmx_inside_smx                                   : 1;
#define IA32_FEATURE_CONTROL_ENABLE_VMX_INSIDE_SMX_BIT               1
#define IA32_FEATURE_CONTROL_ENABLE_VMX_INSIDE_SMX_FLAG              0x02
#define IA32_FEATURE_CONTROL_ENABLE_VMX_INSIDE_SMX_MASK              0x01
#define IA32_FEATURE_CONTROL_ENABLE_VMX_INSIDE_SMX(_)                (((_) >> 1) & 0x01)

	/**
	* @brief Enable VMX outside SMX operation <b>(R/WL)</b>
	*
	* [Bit 2] This bit enables VMX for a system executive that does not require SMX. BIOS must set this bit only when the
	* CPUID function 1 returns the VMX feature flag set (ECX bit 5).
	*
	* @remarks If CPUID.01H:ECX[5] = 1
	*/
	uint64_t enable_vmx_outside_smx                                  : 1;
#define IA32_FEATURE_CONTROL_ENABLE_VMX_OUTSIDE_SMX_BIT              2
#define IA32_FEATURE_CONTROL_ENABLE_VMX_OUTSIDE_SMX_FLAG             0x04
#define IA32_FEATURE_CONTROL_ENABLE_VMX_OUTSIDE_SMX_MASK             0x01
#define IA32_FEATURE_CONTROL_ENABLE_VMX_OUTSIDE_SMX(_)               (((_) >> 2) & 0x01)
	uint64_t reserved1                                               : 5;

	/**
	* @brief SENTER Local Function Enable <b>(R/WL)</b>
	*
	* [Bits 14:8] When set, each bit in the field represents an enable control for a corresponding SENTER function. This field
	* is supported only if CPUID.1:ECX.[bit 6] is set.
	*
	* @remarks If CPUID.01H:ECX[6] = 1
	*/
	uint64_t senter_local_function_enables                           : 7;
#define IA32_FEATURE_CONTROL_SENTER_LOCAL_FUNCTION_ENABLES_BIT       8
#define IA32_FEATURE_CONTROL_SENTER_LOCAL_FUNCTION_ENABLES_FLAG      0x7F00
#define IA32_FEATURE_CONTROL_SENTER_LOCAL_FUNCTION_ENABLES_MASK      0x7F
#define IA32_FEATURE_CONTROL_SENTER_LOCAL_FUNCTION_ENABLES(_)        (((_) >> 8) & 0x7F)

	/**
	* @brief SENTER Global Enable <b>(R/WL)</b>
	*
	* [Bit 15] This bit must be set to enable SENTER leaf functions. This bit is supported only if CPUID.1:ECX.[bit 6] is set.
	*
	* @remarks If CPUID.01H:ECX[6] = 1
	*/
	uint64_t senter_global_enable                                    : 1;
#define IA32_FEATURE_CONTROL_SENTER_GLOBAL_ENABLE_BIT                15
#define IA32_FEATURE_CONTROL_SENTER_GLOBAL_ENABLE_FLAG               0x8000
#define IA32_FEATURE_CONTROL_SENTER_GLOBAL_ENABLE_MASK               0x01
#define IA32_FEATURE_CONTROL_SENTER_GLOBAL_ENABLE(_)                 (((_) >> 15) & 0x01)
	uint64_t reserved2                                               : 1;

	/**
	* @brief SGX Launch Control Enable <b>(R/WL)</b>
	*
	* [Bit 17] This bit must be set to enable runtime reconfiguration of SGX Launch Control via the IA32_SGXLEPUBKEYHASHn MSR.
	*
	* @remarks If CPUID.(EAX=07H, ECX=0H): ECX[30] = 1
	*/
	uint64_t sgx_launch_control_enable                               : 1;
#define IA32_FEATURE_CONTROL_SGX_LAUNCH_CONTROL_ENABLE_BIT           17
#define IA32_FEATURE_CONTROL_SGX_LAUNCH_CONTROL_ENABLE_FLAG          0x20000
#define IA32_FEATURE_CONTROL_SGX_LAUNCH_CONTROL_ENABLE_MASK          0x01
#define IA32_FEATURE_CONTROL_SGX_LAUNCH_CONTROL_ENABLE(_)            (((_) >> 17) & 0x01)

	/**
	* @brief SGX Global Enable <b>(R/WL)</b>
	*
	* [Bit 18] This bit must be set to enable SGX leaf functions.
	*
	* @remarks If CPUID.(EAX=07H, ECX=0H): EBX[2] = 1
	*/
	uint64_t sgx_global_enable                                       : 1;
#define IA32_FEATURE_CONTROL_SGX_GLOBAL_ENABLE_BIT                   18
#define IA32_FEATURE_CONTROL_SGX_GLOBAL_ENABLE_FLAG                  0x40000
#define IA32_FEATURE_CONTROL_SGX_GLOBAL_ENABLE_MASK                  0x01
#define IA32_FEATURE_CONTROL_SGX_GLOBAL_ENABLE(_)                    (((_) >> 18) & 0x01)
	uint64_t reserved3                                               : 1;

	/**
	* @brief LMCE On <b>(R/WL)</b>
	*
	* [Bit 20] When set, system software can program the MSRs associated with LMCE to configure delivery of some machine check
	* exceptions to a single logical processor.
	*
	* @remarks If IA32_MCG_CAP[27] = 1
	*/
	uint64_t lmce_on                                                 : 1;
#define IA32_FEATURE_CONTROL_LMCE_ON_BIT                             20
#define IA32_FEATURE_CONTROL_LMCE_ON_FLAG                            0x100000
#define IA32_FEATURE_CONTROL_LMCE_ON_MASK                            0x01
#define IA32_FEATURE_CONTROL_LMCE_ON(_)                              (((_) >> 20) & 0x01)
	uint64_t reserved4                                               : 43;
  };

  uint64_t flags;
} feature_control_register;


/**
 * Per Logical Processor TSC Adjust.
 *
 * @remarks If CPUID.(EAX=07H, ECX=0H): EBX[1] = 1
 */
#define IA32_TSC_ADJUST                                              0x0000003B
typedef struct
{
  /**
	* Local offset value of the IA32_TSC for a logical processor. Reset value is zero. A write to IA32_TSC will modify the
	* local offset in IA32_TSC_ADJUST and the content of IA32_TSC, but does not affect the internal invariant TSC hardware.
	*/
  uint64_t thread_adjust;
} tsc_adjust_register;


/**
 * @brief BIOS Update Trigger <b>(W)</b>
 *
 * Executing a WRMSR instruction to this MSR causes a microcode update to be loaded into the processor. A processor may
 * prevent writing to this MSR when loading guest states on VM entries or saving guest states on VM exits.
 *
 * @remarks 06_01H
 * @see Vol3A[9.11.6(Microcode Update Loader)]
 */
#define IA32_BIOS_UPDATE_TRIGGER                                     0x00000079

/**
 * @brief BIOS Update Signature <b>(RO)</b>
 *
 * Returns the microcode update signature following the execution of CPUID.01H. A processor may prevent writing to this MSR
 * when loading guest states on VM entries or saving guest states on VM exits.
 *
 * @remarks 06_01H
 */
#define IA32_BIOS_UPDATE_SIGNATURE                                   0x0000008B
typedef union
{
  struct
  {
	/**
	* [Bits 31:0] Reserved.
	*/
	uint64_t reserved                                                : 32;
#define IA32_BIOS_UPDATE_SIGNATURE_RESERVED_BIT                      0
#define IA32_BIOS_UPDATE_SIGNATURE_RESERVED_FLAG                     0xFFFFFFFF
#define IA32_BIOS_UPDATE_SIGNATURE_RESERVED_MASK                     0xFFFFFFFF
#define IA32_BIOS_UPDATE_SIGNATURE_RESERVED(_)                       (((_) >> 0) & 0xFFFFFFFF)

	/**
	* @brief Microcode update signature
	*
	* [Bits 63:32] This field contains the signature of the currently loaded microcode update when read following the
	* execution of the CPUID instruction, function 1. It is required that this register field be pre-loaded with zero prior to
	* executing the CPUID, function 1. If the field remains equal to zero, then there is no microcode update loaded. Another
	* nonzero value will be the signature.
	*
	* @see Vol3A[9.11.7.1(Determining the Signature)] (reference)
	*/
	uint64_t microcode_update_signature                              : 32;
#define IA32_BIOS_UPDATE_SIGNATURE_MICROCODE_UPDATE_SIGNATURE_BIT    32
#define IA32_BIOS_UPDATE_SIGNATURE_MICROCODE_UPDATE_SIGNATURE_FLAG   0xFFFFFFFF00000000
#define IA32_BIOS_UPDATE_SIGNATURE_MICROCODE_UPDATE_SIGNATURE_MASK   0xFFFFFFFF
#define IA32_BIOS_UPDATE_SIGNATURE_MICROCODE_UPDATE_SIGNATURE(_)     (((_) >> 32) & 0xFFFFFFFF)
  };

  uint64_t flags;
} bios_update_signature_register;

/**
 * @defgroup ia32_sgxlepubkeyhash \
 *           IA32_SGXLEPUBKEYHASH[(64*n+63):(64*n)]
 *
 * Bits (64*n+63):(64*n) of the SHA256 digest of the SIGSTRUCT.MODULUS for SGX Launch Enclave. On reset, the default value
 * is the digest of Intel's signing key.
 *
 * @remarks Read permitted If CPUID.(EAX=12H,ECX=0H): EAX[0]=1 && CPUID.(EAX=07H,ECX=0H):ECX[30]=1. Write permitted if
 *          CPUID.(EAX=12H,ECX=0H): EAX[0]=1 && IA32_FEATURE_CONTROL[17] = 1 && IA32_FEATURE_CONTROL[0] = 1.
 * @{
 */
#define IA32_SGXLEPUBKEYHASH0                                        0x0000008C
#define IA32_SGXLEPUBKEYHASH1                                        0x0000008D
#define IA32_SGXLEPUBKEYHASH2                                        0x0000008E
#define IA32_SGXLEPUBKEYHASH3                                        0x0000008F
/**
 * @}
 */


/**
 * SMM Monitor Configuration.
 *
 * @remarks If CPUID.01H: ECX[5]=1 || CPUID.01H: ECX[6] = 1
 */
#define IA32_SMM_MONITOR_CTL                                         0x0000009B
typedef union
{
  struct
  {
	/**
	* @brief Valid <b>(R/W)</b>
	*
	* [Bit 0] The STM may be invoked using VMCALL only if this bit is 1. Because VMCALL is used to activate the dual-monitor
	* treatment, the dual-monitor treatment cannot be activated if the bit is 0. This bit is cleared when the logical
	* processor is reset.
	*
	* @see Vol3C[34.15.6(Activating the Dual-Monitor Treatment)]
	* @see Vol3C[34.15.5(Enabling the Dual-Monitor Treatment)] (reference)
	*/
	uint64_t valid                                                   : 1;
#define IA32_SMM_MONITOR_CTL_VALID_BIT                               0
#define IA32_SMM_MONITOR_CTL_VALID_FLAG                              0x01
#define IA32_SMM_MONITOR_CTL_VALID_MASK                              0x01
#define IA32_SMM_MONITOR_CTL_VALID(_)                                (((_) >> 0) & 0x01)
	uint64_t reserved1                                               : 1;

	/**
	* @brief Controls SMI unblocking by VMXOFF
	*
	* [Bit 2] Determines whether executions of VMXOFF unblock SMIs under the default treatment of SMIs and SMM. Executions of
	* VMXOFF unblock SMIs unless bit 2 is 1 (the value of bit 0 is irrelevant).
	*
	* @remarks If IA32_VMX_MISC[28]
	* @see Vol3C[34.14.4(VMXOFF and SMI Unblocking)]
	* @see Vol3C[34.15.5(Enabling the Dual-Monitor Treatment)] (reference)
	*/
	uint64_t smi_unblocking_by_vmxoff                                : 1;
#define IA32_SMM_MONITOR_CTL_SMI_UNBLOCKING_BY_VMXOFF_BIT            2
#define IA32_SMM_MONITOR_CTL_SMI_UNBLOCKING_BY_VMXOFF_FLAG           0x04
#define IA32_SMM_MONITOR_CTL_SMI_UNBLOCKING_BY_VMXOFF_MASK           0x01
#define IA32_SMM_MONITOR_CTL_SMI_UNBLOCKING_BY_VMXOFF(_)             (((_) >> 2) & 0x01)
	uint64_t reserved2                                               : 9;

	/**
	* @brief MSEG Base <b>(R/W)</b>
	*
	* [Bits 31:12] Value that, when shifted left 12 bits, is the physical address of MSEG (the MSEG base address).
	*
	* @see Vol3C[34.15.5(Enabling the Dual-Monitor Treatment)] (reference)
	*/
	uint64_t mseg_base                                               : 20;
#define IA32_SMM_MONITOR_CTL_MSEG_BASE_BIT                           12
#define IA32_SMM_MONITOR_CTL_MSEG_BASE_FLAG                          0xFFFFF000
#define IA32_SMM_MONITOR_CTL_MSEG_BASE_MASK                          0xFFFFF
#define IA32_SMM_MONITOR_CTL_MSEG_BASE(_)                            (((_) >> 12) & 0xFFFFF)
	uint64_t reserved3                                               : 32;
  };

  uint64_t flags;
} smm_monitor_ctl_register;

typedef struct
{
  /**
	* @brief MSEG revision identifier
	*
	* Different processors may use different MSEG revision identifiers. These identifiers enable software to avoid using an
	* MSEG header formatted for one processor on a processor that uses a different format. Software can discover the MSEG
	* revision identifier that a processor uses by reading the VMX capability MSR IA32_VMX_MISC.
	*
	* @see Vol3D[A.6(MISCELLANEOUS DATA)]
	*/
  uint32_t mseg_header_revision;

  /**
	* @brief SMM-transfer monitor features field
	*
	* Bits 31:1 of this field are reserved and must be zero. Bit 0 of the field is the IA-32e mode SMM feature bit. It
	* indicates whether the logical processor will be in IA-32e mode after the STM is activated.
	*
	* @see Vol3C[34.15.6(Activating the Dual-Monitor Treatment)]
	*/
  uint32_t monitor_features;

  /**
	* Define values for the MonitorFeatures field of MSEG_HEADER.
	*/
#define IA32_STM_FEATURES_IA32E                                      0x00000001

  /**
	* Fields that determine how processor state is loaded when the STM is activated. SMM code should establish these fields so
	* that activating of the STM invokes the STM's initialization code.
	*
	* @see Vol3C[34.15.6.5(Loading Host State)]
	*/
  uint32_t gdtr_limit;
  uint32_t gdtr_base_offset;
  uint32_t cs_selector;
  uint32_t eip_offset;
  uint32_t esp_offset;
  uint32_t cr3_offset;
} mseg_header;


/**
 * Base address of the logical processor's SMRAM image.
 *
 * @remarks If IA32_VMX_MISC[15]
 */
#define IA32_SMBASE                                                  0x0000009E
/**
 * @defgroup ia32_pmc \
 *           IA32_PMC(n)
 *
 * General Performance Counters.
 *
 * @remarks If CPUID.0AH: EAX[15:8] > n
 * @{
 */
#define IA32_PMC0                                                    0x000000C1
#define IA32_PMC1                                                    0x000000C2
#define IA32_PMC2                                                    0x000000C3
#define IA32_PMC3                                                    0x000000C4
#define IA32_PMC4                                                    0x000000C5
#define IA32_PMC5                                                    0x000000C6
#define IA32_PMC6                                                    0x000000C7
#define IA32_PMC7                                                    0x000000C8
/**
 * @}
 */


/**
 * TSC Frequency Clock Counter.
 *
 * @remarks If CPUID.06H: ECX[0] = 1
 */
#define IA32_MPERF                                                   0x000000E7
typedef struct
{
  /**
	* @brief C0 TSC Frequency Clock Count
	*
	* Increments at fixed interval (relative to TSC freq.) when the logical processor is in C0. Cleared upon overflow /
	* wrap-around of IA32_APERF.
	*/
  uint64_t c0_mcnt;
} mperf_register;


/**
 * Actual Performance Clock Counter
 *
 * @remarks If CPUID.06H: ECX[0] = 1
 */
#define IA32_APERF                                                   0x000000E8
typedef struct
{
  /**
	* @brief C0 Actual Frequency Clock Count
	*
	* Accumulates core clock counts at the coordinated clock frequency, when the logical processor is in C0. Cleared upon
	* overflow / wrap-around of IA32_MPERF.
	*/
  uint64_t c0_acnt;
} aperf_register;


/**
 * MTRR Capability.
 *
 * @see Vol3A[11.11.2.1(IA32_MTRR_DEF_TYPE MSR)]
 * @see Vol3A[11.11.1(MTRR Feature Identification)] (reference)
 */
#define IA32_MTRR_CAPABILITIES                                       0x000000FE
typedef union
{
  struct
  {
	/**
	* @brief VCNT (variable range registers count) field
	*
	* [Bits 7:0] Indicates the number of variable ranges implemented on the processor.
	*/
	uint64_t variable_range_count                                    : 8;
#define IA32_MTRR_CAPABILITIES_VARIABLE_RANGE_COUNT_BIT              0
#define IA32_MTRR_CAPABILITIES_VARIABLE_RANGE_COUNT_FLAG             0xFF
#define IA32_MTRR_CAPABILITIES_VARIABLE_RANGE_COUNT_MASK             0xFF
#define IA32_MTRR_CAPABILITIES_VARIABLE_RANGE_COUNT(_)               (((_) >> 0) & 0xFF)

	/**
	* @brief FIX (fixed range registers supported) flag
	*
	* [Bit 8] Fixed range MTRRs (IA32_MTRR_FIX64K_00000 through IA32_MTRR_FIX4K_0F8000) are supported when set; no fixed range
	* registers are supported when clear.
	*/
	uint64_t fixed_range_supported                                   : 1;
#define IA32_MTRR_CAPABILITIES_FIXED_RANGE_SUPPORTED_BIT             8
#define IA32_MTRR_CAPABILITIES_FIXED_RANGE_SUPPORTED_FLAG            0x100
#define IA32_MTRR_CAPABILITIES_FIXED_RANGE_SUPPORTED_MASK            0x01
#define IA32_MTRR_CAPABILITIES_FIXED_RANGE_SUPPORTED(_)              (((_) >> 8) & 0x01)
	uint64_t reserved1                                               : 1;

	/**
	* @brief WC (write combining) flag
	*
	* [Bit 10] The write-combining (WC) memory type is supported when set; the WC type is not supported when clear.
	*/
	uint64_t wc_supported                                            : 1;
#define IA32_MTRR_CAPABILITIES_WC_SUPPORTED_BIT                      10
#define IA32_MTRR_CAPABILITIES_WC_SUPPORTED_FLAG                     0x400
#define IA32_MTRR_CAPABILITIES_WC_SUPPORTED_MASK                     0x01
#define IA32_MTRR_CAPABILITIES_WC_SUPPORTED(_)                       (((_) >> 10) & 0x01)

	/**
	* @brief SMRR (System-Management Range Register) flag
	*
	* [Bit 11] The system-management range register (SMRR) interface is supported when bit 11 is set; the SMRR interface is
	* not supported when clear.
	*/
	uint64_t smrr_supported                                          : 1;
#define IA32_MTRR_CAPABILITIES_SMRR_SUPPORTED_BIT                    11
#define IA32_MTRR_CAPABILITIES_SMRR_SUPPORTED_FLAG                   0x800
#define IA32_MTRR_CAPABILITIES_SMRR_SUPPORTED_MASK                   0x01
#define IA32_MTRR_CAPABILITIES_SMRR_SUPPORTED(_)                     (((_) >> 11) & 0x01)
	uint64_t reserved2                                               : 52;
  };

  uint64_t flags;
} mtrr_capabilities_register;


/**
 * @brief SYSENTER_CS_MSR <b>(R/W)</b>
 *
 * The lower 16 bits of this MSR are the segment selector for the privilege level 0 code segment. This value is also used
 * to determine the segment selector of the privilege level 0 stack segment. This value cannot indicate a null selector.
 *
 * @remarks 06_01H
 * @see Vol2B[4.3(Instructions (M-U) | SYSCALL - Fast System Call)] (reference)
 */
#define IA32_SYSENTER_CS                                             0x00000174
typedef union
{
  struct
  {
	/**
	* [Bits 15:0] CS Selector.
	*/
	uint64_t cs_selector                                             : 16;
#define IA32_SYSENTER_CS_CS_SELECTOR_BIT                             0
#define IA32_SYSENTER_CS_CS_SELECTOR_FLAG                            0xFFFF
#define IA32_SYSENTER_CS_CS_SELECTOR_MASK                            0xFFFF
#define IA32_SYSENTER_CS_CS_SELECTOR(_)                              (((_) >> 0) & 0xFFFF)

	/**
	* [Bits 31:16] Not used.
	*
	* @remarks Can be read and written.
	*/
	uint64_t not_used_1                                              : 16;
#define IA32_SYSENTER_CS_NOT_USED_1_BIT                              16
#define IA32_SYSENTER_CS_NOT_USED_1_FLAG                             0xFFFF0000
#define IA32_SYSENTER_CS_NOT_USED_1_MASK                             0xFFFF
#define IA32_SYSENTER_CS_NOT_USED_1(_)                               (((_) >> 16) & 0xFFFF)

	/**
	* [Bits 63:32] Not used.
	*
	* @remarks Writes ignored; reads return zero.
	*/
	uint64_t not_used_2                                              : 32;
#define IA32_SYSENTER_CS_NOT_USED_2_BIT                              32
#define IA32_SYSENTER_CS_NOT_USED_2_FLAG                             0xFFFFFFFF00000000
#define IA32_SYSENTER_CS_NOT_USED_2_MASK                             0xFFFFFFFF
#define IA32_SYSENTER_CS_NOT_USED_2(_)                               (((_) >> 32) & 0xFFFFFFFF)
  };

  uint64_t flags;
} sysenter_cs_register;


/**
 * @brief SYSENTER_ESP_MSR <b>(R/W)</b>
 *
 * The value of this MSR is loaded into RSP (thus, this value contains the stack pointer for the privilege level 0 stack).
 * This value cannot represent a non-canonical address. In protected mode, only bits 31:0 are loaded.
 *
 * @remarks 06_01H
 * @see Vol2B[4.3(Instructions (M-U) | SYSCALL - Fast System Call)] (reference)
 */
#define IA32_SYSENTER_ESP                                            0x00000175

/**
 * @brief SYSENTER_EIP_MSR <b>(R/W)</b>
 *
 * The value of this MSR is loaded into RIP (thus, this value references the first instruction of the selected operating
 * procedure or routine). In protected mode, only bits 31:0 are loaded.
 *
 * @remarks 06_01H
 * @see Vol2B[4.3(Instructions (M-U) | SYSCALL - Fast System Call)] (reference)
 */
#define IA32_SYSENTER_EIP                                            0x00000176

/**
 * Global Machine Check Capability.
 *
 * @remarks 06_01H
 */
#define IA32_MCG_CAP                                                 0x00000179
typedef union
{
  struct
  {
	/**
	* [Bits 7:0] Number of reporting banks.
	*/
	uint64_t count                                                   : 8;
#define IA32_MCG_CAP_COUNT_BIT                                       0
#define IA32_MCG_CAP_COUNT_FLAG                                      0xFF
#define IA32_MCG_CAP_COUNT_MASK                                      0xFF
#define IA32_MCG_CAP_COUNT(_)                                        (((_) >> 0) & 0xFF)

	/**
	* [Bit 8] IA32_MCG_CTL is present if this bit is set.
	*/
	uint64_t mcg_ctl_p                                               : 1;
#define IA32_MCG_CAP_MCG_CTL_P_BIT                                   8
#define IA32_MCG_CAP_MCG_CTL_P_FLAG                                  0x100
#define IA32_MCG_CAP_MCG_CTL_P_MASK                                  0x01
#define IA32_MCG_CAP_MCG_CTL_P(_)                                    (((_) >> 8) & 0x01)

	/**
	* [Bit 9] Extended machine check state registers are present if this bit is set.
	*/
	uint64_t mcg_ext_p                                               : 1;
#define IA32_MCG_CAP_MCG_EXT_P_BIT                                   9
#define IA32_MCG_CAP_MCG_EXT_P_FLAG                                  0x200
#define IA32_MCG_CAP_MCG_EXT_P_MASK                                  0x01
#define IA32_MCG_CAP_MCG_EXT_P(_)                                    (((_) >> 9) & 0x01)

	/**
	* [Bit 10] Support for corrected MC error event is present.
	*
	* @remarks 06_01H
	*/
	uint64_t mcp_cmci_p                                              : 1;
#define IA32_MCG_CAP_MCP_CMCI_P_BIT                                  10
#define IA32_MCG_CAP_MCP_CMCI_P_FLAG                                 0x400
#define IA32_MCG_CAP_MCP_CMCI_P_MASK                                 0x01
#define IA32_MCG_CAP_MCP_CMCI_P(_)                                   (((_) >> 10) & 0x01)

	/**
	* [Bit 11] Threshold-based error status register are present if this bit is set.
	*/
	uint64_t mcg_tes_p                                               : 1;
#define IA32_MCG_CAP_MCG_TES_P_BIT                                   11
#define IA32_MCG_CAP_MCG_TES_P_FLAG                                  0x800
#define IA32_MCG_CAP_MCG_TES_P_MASK                                  0x01
#define IA32_MCG_CAP_MCG_TES_P(_)                                    (((_) >> 11) & 0x01)
	uint64_t reserved1                                               : 4;

	/**
	* [Bits 23:16] Number of extended machine check state registers present.
	*/
	uint64_t mcg_ext_cnt                                             : 8;
#define IA32_MCG_CAP_MCG_EXT_CNT_BIT                                 16
#define IA32_MCG_CAP_MCG_EXT_CNT_FLAG                                0xFF0000
#define IA32_MCG_CAP_MCG_EXT_CNT_MASK                                0xFF
#define IA32_MCG_CAP_MCG_EXT_CNT(_)                                  (((_) >> 16) & 0xFF)

	/**
	* [Bit 24] The processor supports software error recovery if this bit is set.
	*/
	uint64_t mcg_ser_p                                               : 1;
#define IA32_MCG_CAP_MCG_SER_P_BIT                                   24
#define IA32_MCG_CAP_MCG_SER_P_FLAG                                  0x1000000
#define IA32_MCG_CAP_MCG_SER_P_MASK                                  0x01
#define IA32_MCG_CAP_MCG_SER_P(_)                                    (((_) >> 24) & 0x01)
	uint64_t reserved2                                               : 1;

	/**
	* [Bit 26] Indicates that the processor allows platform firmware to be invoked when an error is detected so that it may
	* provide additional platform specific information in an ACPI format "Generic Error Data Entry" that augments the data
	* included in machine check bank registers.
	*
	* @remarks 06_3EH
	*/
	uint64_t mcg_elog_p                                              : 1;
#define IA32_MCG_CAP_MCG_ELOG_P_BIT                                  26
#define IA32_MCG_CAP_MCG_ELOG_P_FLAG                                 0x4000000
#define IA32_MCG_CAP_MCG_ELOG_P_MASK                                 0x01
#define IA32_MCG_CAP_MCG_ELOG_P(_)                                   (((_) >> 26) & 0x01)

	/**
	* [Bit 27] Indicates that the processor supports extended state in IA32_MCG_STATUS and associated MSR necessary to
	* configure Local Machine Check Exception (LMCE).
	*
	* @remarks 06_3EH
	*/
	uint64_t mcg_lmce_p                                              : 1;
#define IA32_MCG_CAP_MCG_LMCE_P_BIT                                  27
#define IA32_MCG_CAP_MCG_LMCE_P_FLAG                                 0x8000000
#define IA32_MCG_CAP_MCG_LMCE_P_MASK                                 0x01
#define IA32_MCG_CAP_MCG_LMCE_P(_)                                   (((_) >> 27) & 0x01)
	uint64_t reserved3                                               : 36;
  };

  uint64_t flags;
} mcg_cap_register;


/**
 * Global Machine Check Status.
 *
 * @remarks 06_01H
 */
#define IA32_MCG_STATUS                                              0x0000017A
typedef union
{
  struct
  {
	/**
	* [Bit 0] Restart IP valid.
	*
	* @remarks 06_01H
	*/
	uint64_t ripv                                                    : 1;
#define IA32_MCG_STATUS_RIPV_BIT                                     0
#define IA32_MCG_STATUS_RIPV_FLAG                                    0x01
#define IA32_MCG_STATUS_RIPV_MASK                                    0x01
#define IA32_MCG_STATUS_RIPV(_)                                      (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Error IP valid.
	*
	* @remarks 06_01H
	*/
	uint64_t eipv                                                    : 1;
#define IA32_MCG_STATUS_EIPV_BIT                                     1
#define IA32_MCG_STATUS_EIPV_FLAG                                    0x02
#define IA32_MCG_STATUS_EIPV_MASK                                    0x01
#define IA32_MCG_STATUS_EIPV(_)                                      (((_) >> 1) & 0x01)

	/**
	* [Bit 2] Machine check in progress.
	*
	* @remarks 06_01H
	*/
	uint64_t mcip                                                    : 1;
#define IA32_MCG_STATUS_MCIP_BIT                                     2
#define IA32_MCG_STATUS_MCIP_FLAG                                    0x04
#define IA32_MCG_STATUS_MCIP_MASK                                    0x01
#define IA32_MCG_STATUS_MCIP(_)                                      (((_) >> 2) & 0x01)

	/**
	* [Bit 3] If IA32_MCG_CAP.LMCE_P[27] = 1.
	*/
	uint64_t lmce_s                                                  : 1;
#define IA32_MCG_STATUS_LMCE_S_BIT                                   3
#define IA32_MCG_STATUS_LMCE_S_FLAG                                  0x08
#define IA32_MCG_STATUS_LMCE_S_MASK                                  0x01
#define IA32_MCG_STATUS_LMCE_S(_)                                    (((_) >> 3) & 0x01)
	uint64_t reserved1                                               : 60;
  };

  uint64_t flags;
} mcg_status_register;


/**
 * Global Machine Check Control.
 *
 * @remarks If IA32_MCG_CAP.CTL_P[8] = 1
 */
#define IA32_MCG_CTL                                                 0x0000017B
/**
 * @defgroup ia32_perfevtsel \
 *           IA32_PERFEVTSEL(n)
 *
 * Performance Event Select Register n.
 *
 * @remarks If CPUID.0AH: EAX[15:8] > n
 * @{
 */
#define IA32_PERFEVTSEL0                                             0x00000186
#define IA32_PERFEVTSEL1                                             0x00000187
#define IA32_PERFEVTSEL2                                             0x00000188
#define IA32_PERFEVTSEL3                                             0x00000189
typedef union
{
  struct
  {
	/**
	* [Bits 7:0] Selects a performance event logic unit.
	*/
	uint64_t event_select                                            : 8;
#define IA32_PERFEVTSEL_EVENT_SELECT_BIT                             0
#define IA32_PERFEVTSEL_EVENT_SELECT_FLAG                            0xFF
#define IA32_PERFEVTSEL_EVENT_SELECT_MASK                            0xFF
#define IA32_PERFEVTSEL_EVENT_SELECT(_)                              (((_) >> 0) & 0xFF)

	/**
	* [Bits 15:8] Qualifies the microarchitectural condition to detect on the selected event logic.
	*/
	uint64_t u_mask                                                  : 8;
#define IA32_PERFEVTSEL_U_MASK_BIT                                   8
#define IA32_PERFEVTSEL_U_MASK_FLAG                                  0xFF00
#define IA32_PERFEVTSEL_U_MASK_MASK                                  0xFF
#define IA32_PERFEVTSEL_U_MASK(_)                                    (((_) >> 8) & 0xFF)

	/**
	* [Bit 16] Counts while in privilege level is not ring 0.
	*/
	uint64_t usr                                                     : 1;
#define IA32_PERFEVTSEL_USR_BIT                                      16
#define IA32_PERFEVTSEL_USR_FLAG                                     0x10000
#define IA32_PERFEVTSEL_USR_MASK                                     0x01
#define IA32_PERFEVTSEL_USR(_)                                       (((_) >> 16) & 0x01)

	/**
	* [Bit 17] Counts while in privilege level is ring 0.
	*/
	uint64_t os                                                      : 1;
#define IA32_PERFEVTSEL_OS_BIT                                       17
#define IA32_PERFEVTSEL_OS_FLAG                                      0x20000
#define IA32_PERFEVTSEL_OS_MASK                                      0x01
#define IA32_PERFEVTSEL_OS(_)                                        (((_) >> 17) & 0x01)

	/**
	* [Bit 18] Enables edge detection if set.
	*/
	uint64_t edge                                                    : 1;
#define IA32_PERFEVTSEL_EDGE_BIT                                     18
#define IA32_PERFEVTSEL_EDGE_FLAG                                    0x40000
#define IA32_PERFEVTSEL_EDGE_MASK                                    0x01
#define IA32_PERFEVTSEL_EDGE(_)                                      (((_) >> 18) & 0x01)

	/**
	* [Bit 19] Enables pin control.
	*/
	uint64_t pc                                                      : 1;
#define IA32_PERFEVTSEL_PC_BIT                                       19
#define IA32_PERFEVTSEL_PC_FLAG                                      0x80000
#define IA32_PERFEVTSEL_PC_MASK                                      0x01
#define IA32_PERFEVTSEL_PC(_)                                        (((_) >> 19) & 0x01)

	/**
	* [Bit 20] Enables interrupt on counter overflow.
	*/
	uint64_t intr                                                    : 1;
#define IA32_PERFEVTSEL_INTR_BIT                                     20
#define IA32_PERFEVTSEL_INTR_FLAG                                    0x100000
#define IA32_PERFEVTSEL_INTR_MASK                                    0x01
#define IA32_PERFEVTSEL_INTR(_)                                      (((_) >> 20) & 0x01)

	/**
	* [Bit 21] When set to 1, it enables counting the associated event conditions occurring across all logical processors
	* sharing a processor core. When set to 0, the counter only increments the associated event conditions occurring in the
	* logical processor which programmed the MSR.
	*/
	uint64_t any_thread                                              : 1;
#define IA32_PERFEVTSEL_ANY_THREAD_BIT                               21
#define IA32_PERFEVTSEL_ANY_THREAD_FLAG                              0x200000
#define IA32_PERFEVTSEL_ANY_THREAD_MASK                              0x01
#define IA32_PERFEVTSEL_ANY_THREAD(_)                                (((_) >> 21) & 0x01)

	/**
	* [Bit 22] Enables the corresponding performance counter to commence counting when this bit is set.
	*/
	uint64_t en                                                      : 1;
#define IA32_PERFEVTSEL_EN_BIT                                       22
#define IA32_PERFEVTSEL_EN_FLAG                                      0x400000
#define IA32_PERFEVTSEL_EN_MASK                                      0x01
#define IA32_PERFEVTSEL_EN(_)                                        (((_) >> 22) & 0x01)

	/**
	* [Bit 23] Invert the CMASK.
	*/
	uint64_t inv                                                     : 1;
#define IA32_PERFEVTSEL_INV_BIT                                      23
#define IA32_PERFEVTSEL_INV_FLAG                                     0x800000
#define IA32_PERFEVTSEL_INV_MASK                                     0x01
#define IA32_PERFEVTSEL_INV(_)                                       (((_) >> 23) & 0x01)

	/**
	* [Bits 31:24] When CMASK is not zero, the corresponding performance counter increments each cycle if the event count is
	* greater than or equal to the CMASK.
	*/
	uint64_t cmask                                                   : 8;
#define IA32_PERFEVTSEL_CMASK_BIT                                    24
#define IA32_PERFEVTSEL_CMASK_FLAG                                   0xFF000000
#define IA32_PERFEVTSEL_CMASK_MASK                                   0xFF
#define IA32_PERFEVTSEL_CMASK(_)                                     (((_) >> 24) & 0xFF)
	uint64_t reserved1                                               : 32;
  };

  uint64_t flags;
} perfevtsel_register;

/**
 * @}
 */


/**
 * Current Performance Status.
 *
 * @remarks 0F_03H
 * @see Vol3B[14.1.1(Software Interface For Initiating Performance State Transitions)]
 */
#define IA32_PERF_STATUS                                             0x00000198
typedef union
{
  struct
  {
	/**
	* [Bits 15:0] Current performance State Value.
	*/
	uint64_t state_value                                             : 16;
#define IA32_PERF_STATUS_STATE_VALUE_BIT                             0
#define IA32_PERF_STATUS_STATE_VALUE_FLAG                            0xFFFF
#define IA32_PERF_STATUS_STATE_VALUE_MASK                            0xFFFF
#define IA32_PERF_STATUS_STATE_VALUE(_)                              (((_) >> 0) & 0xFFFF)
	uint64_t reserved1                                               : 48;
  };

  uint64_t flags;
} perf_status_register;


/**
 * @brief Performance Control <b>(R/W)</b>
 *
 * Performance Control. Software makes a request for a new Performance state (P-State) by writing this MSR.
 *
 * @remarks 0F_03H
 * @see Vol3B[14.1.1(Software Interface For Initiating Performance State Transitions)]
 */
#define IA32_PERF_CTL                                                0x00000199
typedef union
{
  struct
  {
	/**
	* [Bits 15:0] Target performance State Value.
	*/
	uint64_t target_state_value                                      : 16;
#define IA32_PERF_CTL_TARGET_STATE_VALUE_BIT                         0
#define IA32_PERF_CTL_TARGET_STATE_VALUE_FLAG                        0xFFFF
#define IA32_PERF_CTL_TARGET_STATE_VALUE_MASK                        0xFFFF
#define IA32_PERF_CTL_TARGET_STATE_VALUE(_)                          (((_) >> 0) & 0xFFFF)
	uint64_t reserved1                                               : 16;

	/**
	* [Bit 32] IDA Engage.
	*
	* @remarks 06_0FH (Mobile only)
	*/
	uint64_t ida_engage                                              : 1;
#define IA32_PERF_CTL_IDA_ENGAGE_BIT                                 32
#define IA32_PERF_CTL_IDA_ENGAGE_FLAG                                0x100000000
#define IA32_PERF_CTL_IDA_ENGAGE_MASK                                0x01
#define IA32_PERF_CTL_IDA_ENGAGE(_)                                  (((_) >> 32) & 0x01)
	uint64_t reserved2                                               : 31;
  };

  uint64_t flags;
} perf_ctl_register;


/**
 * Clock Modulation Control.
 *
 * @remarks If CPUID.01H:EDX[22] = 1
 * @see Vol3B[14.7.3(Software Controlled Clock Modulation)]
 */
#define IA32_CLOCK_MODULATION                                        0x0000019A
typedef union
{
  struct
  {
	/**
	* [Bit 0] Extended On-Demand Clock Modulation Duty Cycle.
	*
	* @remarks If CPUID.06H:EAX[5] = 1
	*/
	uint64_t extended_on_demand_clock_modulation_duty_cycle          : 1;
#define IA32_CLOCK_MODULATION_EXTENDED_ON_DEMAND_CLOCK_MODULATION_DUTY_CYCLE_BIT 0
#define IA32_CLOCK_MODULATION_EXTENDED_ON_DEMAND_CLOCK_MODULATION_DUTY_CYCLE_FLAG 0x01
#define IA32_CLOCK_MODULATION_EXTENDED_ON_DEMAND_CLOCK_MODULATION_DUTY_CYCLE_MASK 0x01
#define IA32_CLOCK_MODULATION_EXTENDED_ON_DEMAND_CLOCK_MODULATION_DUTY_CYCLE(_) (((_) >> 0) & 0x01)

	/**
	* @brief On-Demand Clock Modulation Duty Cycle
	*
	* [Bits 3:1] On-Demand Clock Modulation Duty Cycle: Specific encoded values for target duty cycle modulation.
	*
	* @remarks If CPUID.01H:EDX[22] = 1
	*/
	uint64_t on_demand_clock_modulation_duty_cycle                   : 3;
#define IA32_CLOCK_MODULATION_ON_DEMAND_CLOCK_MODULATION_DUTY_CYCLE_BIT 1
#define IA32_CLOCK_MODULATION_ON_DEMAND_CLOCK_MODULATION_DUTY_CYCLE_FLAG 0x0E
#define IA32_CLOCK_MODULATION_ON_DEMAND_CLOCK_MODULATION_DUTY_CYCLE_MASK 0x07
#define IA32_CLOCK_MODULATION_ON_DEMAND_CLOCK_MODULATION_DUTY_CYCLE(_) (((_) >> 1) & 0x07)

	/**
	* @brief On-Demand Clock Modulation Enable
	*
	* [Bit 4] On-Demand Clock Modulation Enable: Set 1 to enable modulation.
	*
	* @remarks If CPUID.01H:EDX[22] = 1
	*/
	uint64_t on_demand_clock_modulation_enable                       : 1;
#define IA32_CLOCK_MODULATION_ON_DEMAND_CLOCK_MODULATION_ENABLE_BIT  4
#define IA32_CLOCK_MODULATION_ON_DEMAND_CLOCK_MODULATION_ENABLE_FLAG 0x10
#define IA32_CLOCK_MODULATION_ON_DEMAND_CLOCK_MODULATION_ENABLE_MASK 0x01
#define IA32_CLOCK_MODULATION_ON_DEMAND_CLOCK_MODULATION_ENABLE(_)   (((_) >> 4) & 0x01)
	uint64_t reserved1                                               : 59;
  };

  uint64_t flags;
} clock_modulation_register;


/**
 * @brief Thermal Interrupt Control <b>(R/W)</b>
 *
 * Thermal Interrupt Control. Enables and disables the generation of an interrupt on temperature transitions detected with
 * the processor's thermal sensors and thermal monitor.
 *
 * @remarks If CPUID.01H:EDX[22] = 1
 * @see Vol3B[14.7.2(Thermal Monitor)]
 */
#define IA32_THERM_INTERRUPT                                         0x0000019B
typedef union
{
  struct
  {
	/**
	* [Bit 0] High-Temperature Interrupt Enable.
	*
	* @remarks If CPUID.01H:EDX[22] = 1
	*/
	uint64_t high_temperature_interrupt_enable                       : 1;
#define IA32_THERM_INTERRUPT_HIGH_TEMPERATURE_INTERRUPT_ENABLE_BIT   0
#define IA32_THERM_INTERRUPT_HIGH_TEMPERATURE_INTERRUPT_ENABLE_FLAG  0x01
#define IA32_THERM_INTERRUPT_HIGH_TEMPERATURE_INTERRUPT_ENABLE_MASK  0x01
#define IA32_THERM_INTERRUPT_HIGH_TEMPERATURE_INTERRUPT_ENABLE(_)    (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Low-Temperature Interrupt Enable.
	*
	* @remarks If CPUID.01H:EDX[22] = 1
	*/
	uint64_t low_temperature_interrupt_enable                        : 1;
#define IA32_THERM_INTERRUPT_LOW_TEMPERATURE_INTERRUPT_ENABLE_BIT    1
#define IA32_THERM_INTERRUPT_LOW_TEMPERATURE_INTERRUPT_ENABLE_FLAG   0x02
#define IA32_THERM_INTERRUPT_LOW_TEMPERATURE_INTERRUPT_ENABLE_MASK   0x01
#define IA32_THERM_INTERRUPT_LOW_TEMPERATURE_INTERRUPT_ENABLE(_)     (((_) >> 1) & 0x01)

	/**
	* [Bit 2] PROCHOT\# Interrupt Enable.
	*
	* @remarks If CPUID.01H:EDX[22] = 1
	*/
	uint64_t prochot_interrupt_enable                                : 1;
#define IA32_THERM_INTERRUPT_PROCHOT_INTERRUPT_ENABLE_BIT            2
#define IA32_THERM_INTERRUPT_PROCHOT_INTERRUPT_ENABLE_FLAG           0x04
#define IA32_THERM_INTERRUPT_PROCHOT_INTERRUPT_ENABLE_MASK           0x01
#define IA32_THERM_INTERRUPT_PROCHOT_INTERRUPT_ENABLE(_)             (((_) >> 2) & 0x01)

	/**
	* [Bit 3] FORCEPR\# Interrupt Enable.
	*
	* @remarks If CPUID.01H:EDX[22] = 1
	*/
	uint64_t forcepr_interrupt_enable                                : 1;
#define IA32_THERM_INTERRUPT_FORCEPR_INTERRUPT_ENABLE_BIT            3
#define IA32_THERM_INTERRUPT_FORCEPR_INTERRUPT_ENABLE_FLAG           0x08
#define IA32_THERM_INTERRUPT_FORCEPR_INTERRUPT_ENABLE_MASK           0x01
#define IA32_THERM_INTERRUPT_FORCEPR_INTERRUPT_ENABLE(_)             (((_) >> 3) & 0x01)

	/**
	* [Bit 4] Critical Temperature Interrupt Enable.
	*
	* @remarks If CPUID.01H:EDX[22] = 1
	*/
	uint64_t critical_temperature_interrupt_enable                   : 1;
#define IA32_THERM_INTERRUPT_CRITICAL_TEMPERATURE_INTERRUPT_ENABLE_BIT 4
#define IA32_THERM_INTERRUPT_CRITICAL_TEMPERATURE_INTERRUPT_ENABLE_FLAG 0x10
#define IA32_THERM_INTERRUPT_CRITICAL_TEMPERATURE_INTERRUPT_ENABLE_MASK 0x01
#define IA32_THERM_INTERRUPT_CRITICAL_TEMPERATURE_INTERRUPT_ENABLE(_) (((_) >> 4) & 0x01)
	uint64_t reserved1                                               : 3;

	/**
	* [Bits 14:8] Threshold \#1 Value
	*
	* @remarks If CPUID.01H:EDX[22] = 1
	*/
	uint64_t threshold1_value                                        : 7;
#define IA32_THERM_INTERRUPT_THRESHOLD1_VALUE_BIT                    8
#define IA32_THERM_INTERRUPT_THRESHOLD1_VALUE_FLAG                   0x7F00
#define IA32_THERM_INTERRUPT_THRESHOLD1_VALUE_MASK                   0x7F
#define IA32_THERM_INTERRUPT_THRESHOLD1_VALUE(_)                     (((_) >> 8) & 0x7F)

	/**
	* [Bit 15] Threshold \#1 Interrupt Enable.
	*
	* @remarks If CPUID.01H:EDX[22] = 1
	*/
	uint64_t threshold1_interrupt_enable                             : 1;
#define IA32_THERM_INTERRUPT_THRESHOLD1_INTERRUPT_ENABLE_BIT         15
#define IA32_THERM_INTERRUPT_THRESHOLD1_INTERRUPT_ENABLE_FLAG        0x8000
#define IA32_THERM_INTERRUPT_THRESHOLD1_INTERRUPT_ENABLE_MASK        0x01
#define IA32_THERM_INTERRUPT_THRESHOLD1_INTERRUPT_ENABLE(_)          (((_) >> 15) & 0x01)

	/**
	* [Bits 22:16] Threshold \#2 Value.
	*
	* @remarks If CPUID.01H:EDX[22] = 1
	*/
	uint64_t threshold2_value                                        : 7;
#define IA32_THERM_INTERRUPT_THRESHOLD2_VALUE_BIT                    16
#define IA32_THERM_INTERRUPT_THRESHOLD2_VALUE_FLAG                   0x7F0000
#define IA32_THERM_INTERRUPT_THRESHOLD2_VALUE_MASK                   0x7F
#define IA32_THERM_INTERRUPT_THRESHOLD2_VALUE(_)                     (((_) >> 16) & 0x7F)

	/**
	* [Bit 23] Threshold \#2 Interrupt Enable.
	*
	* @remarks If CPUID.01H:EDX[22] = 1
	*/
	uint64_t threshold2_interrupt_enable                             : 1;
#define IA32_THERM_INTERRUPT_THRESHOLD2_INTERRUPT_ENABLE_BIT         23
#define IA32_THERM_INTERRUPT_THRESHOLD2_INTERRUPT_ENABLE_FLAG        0x800000
#define IA32_THERM_INTERRUPT_THRESHOLD2_INTERRUPT_ENABLE_MASK        0x01
#define IA32_THERM_INTERRUPT_THRESHOLD2_INTERRUPT_ENABLE(_)          (((_) >> 23) & 0x01)

	/**
	* [Bit 24] Power Limit Notification Enable.
	*
	* @remarks If CPUID.06H:EAX[4] = 1
	*/
	uint64_t power_limit_notification_enable                         : 1;
#define IA32_THERM_INTERRUPT_POWER_LIMIT_NOTIFICATION_ENABLE_BIT     24
#define IA32_THERM_INTERRUPT_POWER_LIMIT_NOTIFICATION_ENABLE_FLAG    0x1000000
#define IA32_THERM_INTERRUPT_POWER_LIMIT_NOTIFICATION_ENABLE_MASK    0x01
#define IA32_THERM_INTERRUPT_POWER_LIMIT_NOTIFICATION_ENABLE(_)      (((_) >> 24) & 0x01)
	uint64_t reserved2                                               : 39;
  };

  uint64_t flags;
} therm_interrupt_register;


/**
 * @brief Thermal Status Information <b>(RO)</b>
 *
 * Thermal Status Information. Contains status information about the processor's thermal sensor and automatic thermal
 * monitoring facilities.
 *
 * @remarks If CPUID.01H:EDX[22] = 1
 * @see Vol3B[14.7.2(Thermal Monitor)]
 */
#define IA32_THERM_STATUS                                            0x0000019C
typedef union
{
  struct
  {
	/**
	* [Bit 0] Thermal Status
	*
	* @remarks If CPUID.01H:EDX[22] = 1
	*/
	uint64_t thermal_status                                          : 1;
#define IA32_THERM_STATUS_THERMAL_STATUS_BIT                         0
#define IA32_THERM_STATUS_THERMAL_STATUS_FLAG                        0x01
#define IA32_THERM_STATUS_THERMAL_STATUS_MASK                        0x01
#define IA32_THERM_STATUS_THERMAL_STATUS(_)                          (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Thermal Status Log
	*
	* @remarks If CPUID.01H:EDX[22] = 1
	*/
	uint64_t thermal_status_log                                      : 1;
#define IA32_THERM_STATUS_THERMAL_STATUS_LOG_BIT                     1
#define IA32_THERM_STATUS_THERMAL_STATUS_LOG_FLAG                    0x02
#define IA32_THERM_STATUS_THERMAL_STATUS_LOG_MASK                    0x01
#define IA32_THERM_STATUS_THERMAL_STATUS_LOG(_)                      (((_) >> 1) & 0x01)

	/**
	* [Bit 2] PROCHOT \# or FORCEPR\# event
	*
	* @remarks If CPUID.01H:EDX[22] = 1
	*/
	uint64_t prochot_forcepr_event                                   : 1;
#define IA32_THERM_STATUS_PROCHOT_FORCEPR_EVENT_BIT                  2
#define IA32_THERM_STATUS_PROCHOT_FORCEPR_EVENT_FLAG                 0x04
#define IA32_THERM_STATUS_PROCHOT_FORCEPR_EVENT_MASK                 0x01
#define IA32_THERM_STATUS_PROCHOT_FORCEPR_EVENT(_)                   (((_) >> 2) & 0x01)

	/**
	* [Bit 3] PROCHOT \# or FORCEPR\# log
	*
	* @remarks If CPUID.01H:EDX[22] = 1
	*/
	uint64_t prochot_forcepr_log                                     : 1;
#define IA32_THERM_STATUS_PROCHOT_FORCEPR_LOG_BIT                    3
#define IA32_THERM_STATUS_PROCHOT_FORCEPR_LOG_FLAG                   0x08
#define IA32_THERM_STATUS_PROCHOT_FORCEPR_LOG_MASK                   0x01
#define IA32_THERM_STATUS_PROCHOT_FORCEPR_LOG(_)                     (((_) >> 3) & 0x01)

	/**
	* [Bit 4] Critical Temperature Status
	*
	* @remarks If CPUID.01H:EDX[22] = 1
	*/
	uint64_t critical_temperature_status                             : 1;
#define IA32_THERM_STATUS_CRITICAL_TEMPERATURE_STATUS_BIT            4
#define IA32_THERM_STATUS_CRITICAL_TEMPERATURE_STATUS_FLAG           0x10
#define IA32_THERM_STATUS_CRITICAL_TEMPERATURE_STATUS_MASK           0x01
#define IA32_THERM_STATUS_CRITICAL_TEMPERATURE_STATUS(_)             (((_) >> 4) & 0x01)

	/**
	* [Bit 5] Critical Temperature Status log
	*
	* @remarks If CPUID.01H:EDX[22] = 1
	*/
	uint64_t critical_temperature_status_log                         : 1;
#define IA32_THERM_STATUS_CRITICAL_TEMPERATURE_STATUS_LOG_BIT        5
#define IA32_THERM_STATUS_CRITICAL_TEMPERATURE_STATUS_LOG_FLAG       0x20
#define IA32_THERM_STATUS_CRITICAL_TEMPERATURE_STATUS_LOG_MASK       0x01
#define IA32_THERM_STATUS_CRITICAL_TEMPERATURE_STATUS_LOG(_)         (((_) >> 5) & 0x01)

	/**
	* [Bit 6] Thermal Threshold \#1 Status
	*
	* @remarks If CPUID.01H:ECX[8] = 1
	*/
	uint64_t thermal_threshold1_status                               : 1;
#define IA32_THERM_STATUS_THERMAL_THRESHOLD1_STATUS_BIT              6
#define IA32_THERM_STATUS_THERMAL_THRESHOLD1_STATUS_FLAG             0x40
#define IA32_THERM_STATUS_THERMAL_THRESHOLD1_STATUS_MASK             0x01
#define IA32_THERM_STATUS_THERMAL_THRESHOLD1_STATUS(_)               (((_) >> 6) & 0x01)

	/**
	* [Bit 7] Thermal Threshold \#1 log
	*
	* @remarks If CPUID.01H:ECX[8] = 1
	*/
	uint64_t thermal_threshold1_log                                  : 1;
#define IA32_THERM_STATUS_THERMAL_THRESHOLD1_LOG_BIT                 7
#define IA32_THERM_STATUS_THERMAL_THRESHOLD1_LOG_FLAG                0x80
#define IA32_THERM_STATUS_THERMAL_THRESHOLD1_LOG_MASK                0x01
#define IA32_THERM_STATUS_THERMAL_THRESHOLD1_LOG(_)                  (((_) >> 7) & 0x01)

	/**
	* [Bit 8] Thermal Threshold \#2 Status
	*
	* @remarks If CPUID.01H:ECX[8] = 1
	*/
	uint64_t thermal_threshold2_status                               : 1;
#define IA32_THERM_STATUS_THERMAL_THRESHOLD2_STATUS_BIT              8
#define IA32_THERM_STATUS_THERMAL_THRESHOLD2_STATUS_FLAG             0x100
#define IA32_THERM_STATUS_THERMAL_THRESHOLD2_STATUS_MASK             0x01
#define IA32_THERM_STATUS_THERMAL_THRESHOLD2_STATUS(_)               (((_) >> 8) & 0x01)

	/**
	* [Bit 9] Thermal Threshold \#2 log
	*
	* @remarks If CPUID.01H:ECX[8] = 1
	*/
	uint64_t thermal_threshold2_log                                  : 1;
#define IA32_THERM_STATUS_THERMAL_THRESHOLD2_LOG_BIT                 9
#define IA32_THERM_STATUS_THERMAL_THRESHOLD2_LOG_FLAG                0x200
#define IA32_THERM_STATUS_THERMAL_THRESHOLD2_LOG_MASK                0x01
#define IA32_THERM_STATUS_THERMAL_THRESHOLD2_LOG(_)                  (((_) >> 9) & 0x01)

	/**
	* [Bit 10] Power Limitation Status
	*
	* @remarks If CPUID.06H:EAX[4] = 1
	*/
	uint64_t power_limitation_status                                 : 1;
#define IA32_THERM_STATUS_POWER_LIMITATION_STATUS_BIT                10
#define IA32_THERM_STATUS_POWER_LIMITATION_STATUS_FLAG               0x400
#define IA32_THERM_STATUS_POWER_LIMITATION_STATUS_MASK               0x01
#define IA32_THERM_STATUS_POWER_LIMITATION_STATUS(_)                 (((_) >> 10) & 0x01)

	/**
	* [Bit 11] Power Limitation log
	*
	* @remarks If CPUID.06H:EAX[4] = 1
	*/
	uint64_t power_limitation_log                                    : 1;
#define IA32_THERM_STATUS_POWER_LIMITATION_LOG_BIT                   11
#define IA32_THERM_STATUS_POWER_LIMITATION_LOG_FLAG                  0x800
#define IA32_THERM_STATUS_POWER_LIMITATION_LOG_MASK                  0x01
#define IA32_THERM_STATUS_POWER_LIMITATION_LOG(_)                    (((_) >> 11) & 0x01)

	/**
	* [Bit 12] Current Limit Status
	*
	* @remarks If CPUID.06H:EAX[7] = 1
	*/
	uint64_t current_limit_status                                    : 1;
#define IA32_THERM_STATUS_CURRENT_LIMIT_STATUS_BIT                   12
#define IA32_THERM_STATUS_CURRENT_LIMIT_STATUS_FLAG                  0x1000
#define IA32_THERM_STATUS_CURRENT_LIMIT_STATUS_MASK                  0x01
#define IA32_THERM_STATUS_CURRENT_LIMIT_STATUS(_)                    (((_) >> 12) & 0x01)

	/**
	* [Bit 13] Current Limit log
	*
	* @remarks If CPUID.06H:EAX[7] = 1
	*/
	uint64_t current_limit_log                                       : 1;
#define IA32_THERM_STATUS_CURRENT_LIMIT_LOG_BIT                      13
#define IA32_THERM_STATUS_CURRENT_LIMIT_LOG_FLAG                     0x2000
#define IA32_THERM_STATUS_CURRENT_LIMIT_LOG_MASK                     0x01
#define IA32_THERM_STATUS_CURRENT_LIMIT_LOG(_)                       (((_) >> 13) & 0x01)

	/**
	* [Bit 14] Cross Domain Limit Status
	*
	* @remarks If CPUID.06H:EAX[7] = 1
	*/
	uint64_t cross_domain_limit_status                               : 1;
#define IA32_THERM_STATUS_CROSS_DOMAIN_LIMIT_STATUS_BIT              14
#define IA32_THERM_STATUS_CROSS_DOMAIN_LIMIT_STATUS_FLAG             0x4000
#define IA32_THERM_STATUS_CROSS_DOMAIN_LIMIT_STATUS_MASK             0x01
#define IA32_THERM_STATUS_CROSS_DOMAIN_LIMIT_STATUS(_)               (((_) >> 14) & 0x01)

	/**
	* [Bit 15] Cross Domain Limit log
	*
	* @remarks If CPUID.06H:EAX[7] = 1
	*/
	uint64_t cross_domain_limit_log                                  : 1;
#define IA32_THERM_STATUS_CROSS_DOMAIN_LIMIT_LOG_BIT                 15
#define IA32_THERM_STATUS_CROSS_DOMAIN_LIMIT_LOG_FLAG                0x8000
#define IA32_THERM_STATUS_CROSS_DOMAIN_LIMIT_LOG_MASK                0x01
#define IA32_THERM_STATUS_CROSS_DOMAIN_LIMIT_LOG(_)                  (((_) >> 15) & 0x01)

	/**
	* [Bits 22:16] Digital Readout
	*
	* @remarks If CPUID.06H:EAX[0] = 1
	*/
	uint64_t digital_readout                                         : 7;
#define IA32_THERM_STATUS_DIGITAL_READOUT_BIT                        16
#define IA32_THERM_STATUS_DIGITAL_READOUT_FLAG                       0x7F0000
#define IA32_THERM_STATUS_DIGITAL_READOUT_MASK                       0x7F
#define IA32_THERM_STATUS_DIGITAL_READOUT(_)                         (((_) >> 16) & 0x7F)
	uint64_t reserved1                                               : 4;

	/**
	* [Bits 30:27] Resolution in Degrees Celsius
	*
	* @remarks If CPUID.06H:EAX[0] = 1
	*/
	uint64_t resolution_in_degrees_celsius                           : 4;
#define IA32_THERM_STATUS_RESOLUTION_IN_DEGREES_CELSIUS_BIT          27
#define IA32_THERM_STATUS_RESOLUTION_IN_DEGREES_CELSIUS_FLAG         0x78000000
#define IA32_THERM_STATUS_RESOLUTION_IN_DEGREES_CELSIUS_MASK         0x0F
#define IA32_THERM_STATUS_RESOLUTION_IN_DEGREES_CELSIUS(_)           (((_) >> 27) & 0x0F)

	/**
	* [Bit 31] Reading Valid
	*
	* @remarks If CPUID.06H:EAX[0] = 1
	*/
	uint64_t reading_valid                                           : 1;
#define IA32_THERM_STATUS_READING_VALID_BIT                          31
#define IA32_THERM_STATUS_READING_VALID_FLAG                         0x80000000
#define IA32_THERM_STATUS_READING_VALID_MASK                         0x01
#define IA32_THERM_STATUS_READING_VALID(_)                           (((_) >> 31) & 0x01)
	uint64_t reserved2                                               : 32;
  };

  uint64_t flags;
} therm_status_register;


/**
 * @brief Enable Misc. Processor Features <b>(R/W)</b>
 *
 * Allows a variety of processor functions to be enabled and disabled.
 */
#define IA32_MISC_ENABLE                                             0x000001A0
typedef union
{
  struct
  {
	/**
	* @brief Fast-Strings Enable
	*
	* [Bit 0] When set, the fast-strings feature (for REP MOVS and REP STORS) is enabled (default). When clear, fast-strings
	* are disabled.
	*
	* @remarks 0F_0H
	*/
	uint64_t fast_strings_enable                                     : 1;
#define IA32_MISC_ENABLE_FAST_STRINGS_ENABLE_BIT                     0
#define IA32_MISC_ENABLE_FAST_STRINGS_ENABLE_FLAG                    0x01
#define IA32_MISC_ENABLE_FAST_STRINGS_ENABLE_MASK                    0x01
#define IA32_MISC_ENABLE_FAST_STRINGS_ENABLE(_)                      (((_) >> 0) & 0x01)
	uint64_t reserved1                                               : 2;

	/**
	* @brief Automatic Thermal Control Circuit Enable <b>(R/W)</b>
	*
	* [Bit 3] - 1 = Setting this bit enables the thermal control circuit (TCC) portion of the Intel Thermal Monitor feature.
	* This allows the processor to automatically reduce power consumption in response to TCC activation.
	* - 0 = Disabled.
	*
	* @note In some products clearing this bit might be ignored in critical thermal conditions, and TM1, TM2 and adaptive
	*       thermal throttling will still be activated. The default value of this field varies with product.
	* @remarks 0F_0H
	*/
	uint64_t automatic_thermal_control_circuit_enable                : 1;
#define IA32_MISC_ENABLE_AUTOMATIC_THERMAL_CONTROL_CIRCUIT_ENABLE_BIT 3
#define IA32_MISC_ENABLE_AUTOMATIC_THERMAL_CONTROL_CIRCUIT_ENABLE_FLAG 0x08
#define IA32_MISC_ENABLE_AUTOMATIC_THERMAL_CONTROL_CIRCUIT_ENABLE_MASK 0x01
#define IA32_MISC_ENABLE_AUTOMATIC_THERMAL_CONTROL_CIRCUIT_ENABLE(_) (((_) >> 3) & 0x01)
	uint64_t reserved2                                               : 3;

	/**
	* @brief Performance Monitoring Available <b>(R)</b>
	*
	* [Bit 7] - 1 = Performance monitoring enabled.
	* - 0 = Performance monitoring disabled.
	*
	* @remarks 0F_0H
	*/
	uint64_t performance_monitoring_available                        : 1;
#define IA32_MISC_ENABLE_PERFORMANCE_MONITORING_AVAILABLE_BIT        7
#define IA32_MISC_ENABLE_PERFORMANCE_MONITORING_AVAILABLE_FLAG       0x80
#define IA32_MISC_ENABLE_PERFORMANCE_MONITORING_AVAILABLE_MASK       0x01
#define IA32_MISC_ENABLE_PERFORMANCE_MONITORING_AVAILABLE(_)         (((_) >> 7) & 0x01)
	uint64_t reserved3                                               : 3;

	/**
	* @brief Branch Trace Storage Unavailable <b>(RO)</b>
	*
	* [Bit 11] - 1 = Processor doesn't support branch trace storage (BTS).
	* - 0 = BTS is supported.
	*
	* @remarks 0F_0H
	*/
	uint64_t branch_trace_storage_unavailable                        : 1;
#define IA32_MISC_ENABLE_BRANCH_TRACE_STORAGE_UNAVAILABLE_BIT        11
#define IA32_MISC_ENABLE_BRANCH_TRACE_STORAGE_UNAVAILABLE_FLAG       0x800
#define IA32_MISC_ENABLE_BRANCH_TRACE_STORAGE_UNAVAILABLE_MASK       0x01
#define IA32_MISC_ENABLE_BRANCH_TRACE_STORAGE_UNAVAILABLE(_)         (((_) >> 11) & 0x01)

	/**
	* @brief Processor Event Based Sampling (PEBS) Unavailable <b>(RO)</b>
	*
	* [Bit 12] - 1 = PEBS is not supported.
	* - 0 = PEBS is supported.
	*
	* @remarks 06_0FH
	*/
	uint64_t processor_event_based_sampling_unavailable              : 1;
#define IA32_MISC_ENABLE_PROCESSOR_EVENT_BASED_SAMPLING_UNAVAILABLE_BIT 12
#define IA32_MISC_ENABLE_PROCESSOR_EVENT_BASED_SAMPLING_UNAVAILABLE_FLAG 0x1000
#define IA32_MISC_ENABLE_PROCESSOR_EVENT_BASED_SAMPLING_UNAVAILABLE_MASK 0x01
#define IA32_MISC_ENABLE_PROCESSOR_EVENT_BASED_SAMPLING_UNAVAILABLE(_) (((_) >> 12) & 0x01)
	uint64_t reserved4                                               : 3;

	/**
	* @brief Enhanced Intel SpeedStep Technology Enable <b>(R/W)</b>
	*
	* [Bit 16] - 0 = Enhanced Intel SpeedStep Technology disabled.
	* - 1 = Enhanced Intel SpeedStep Technology enabled.
	*
	* @remarks If CPUID.01H: ECX[7] = 1
	*/
	uint64_t enhanced_intel_speedstep_technology_enable              : 1;
#define IA32_MISC_ENABLE_ENHANCED_INTEL_SPEEDSTEP_TECHNOLOGY_ENABLE_BIT 16
#define IA32_MISC_ENABLE_ENHANCED_INTEL_SPEEDSTEP_TECHNOLOGY_ENABLE_FLAG 0x10000
#define IA32_MISC_ENABLE_ENHANCED_INTEL_SPEEDSTEP_TECHNOLOGY_ENABLE_MASK 0x01
#define IA32_MISC_ENABLE_ENHANCED_INTEL_SPEEDSTEP_TECHNOLOGY_ENABLE(_) (((_) >> 16) & 0x01)
	uint64_t reserved5                                               : 1;

	/**
	* @brief ENABLE MONITOR FSM <b>(R/W)</b>
	*
	* [Bit 18] When this bit is set to 0, the MONITOR feature flag is not set (CPUID.01H:ECX[bit3] = 0). This indicates that
	* MONITOR/MWAIT are not supported. Software attempts to execute MONITOR/MWAIT will cause \#UD when this bit is 0.
	* When this bit is set to 1 (default), MONITOR/MWAIT are supported (CPUID.01H:ECX[bit 3] = 1). If the SSE3 feature flag
	* ECX[0] is not set (CPUID.01H:ECX[bit 0] = 0), the OS must not attempt to alter this bit. BIOS must leave it in the
	* default state. Writing this bit when the SSE3 feature flag is set to 0 may generate a \#GP exception.
	*
	* @remarks 0F_03H
	*/
	uint64_t enable_monitor_fsm                                      : 1;
#define IA32_MISC_ENABLE_ENABLE_MONITOR_FSM_BIT                      18
#define IA32_MISC_ENABLE_ENABLE_MONITOR_FSM_FLAG                     0x40000
#define IA32_MISC_ENABLE_ENABLE_MONITOR_FSM_MASK                     0x01
#define IA32_MISC_ENABLE_ENABLE_MONITOR_FSM(_)                       (((_) >> 18) & 0x01)
	uint64_t reserved6                                               : 3;

	/**
	* @brief Limit CPUID Maxval <b>(R/W)</b>
	*
	* [Bit 22] When this bit is set to 1, CPUID.00H returns a maximum value in EAX[7:0] of 2. BIOS should contain a setup
	* question that allows users to specify when the installed OS does not support CPUID functions greater than 2.
	* Before setting this bit, BIOS must execute the CPUID.0H and examine the maximum value returned in EAX[7:0]. If the
	* maximum value is greater than 2, this bit is supported.
	* Otherwise, this bit is not supported. Setting this bit when the maximum value is not greater than 2 may generate a \#GP
	* exception. Setting this bit may cause unexpected behavior in software that depends on the availability of CPUID leaves
	* greater than 2.
	*
	* @remarks 0F_03H
	*/
	uint64_t limit_cpuid_maxval                                      : 1;
#define IA32_MISC_ENABLE_LIMIT_CPUID_MAXVAL_BIT                      22
#define IA32_MISC_ENABLE_LIMIT_CPUID_MAXVAL_FLAG                     0x400000
#define IA32_MISC_ENABLE_LIMIT_CPUID_MAXVAL_MASK                     0x01
#define IA32_MISC_ENABLE_LIMIT_CPUID_MAXVAL(_)                       (((_) >> 22) & 0x01)

	/**
	* @brief xTPR Message Disable <b>(R/W)</b>
	*
	* [Bit 23] When set to 1, xTPR messages are disabled. xTPR messages are optional messages that allow the processor to
	* inform the chipset of its priority.
	*
	* @remarks If CPUID.01H:ECX[14] = 1
	*/
	uint64_t xtpr_message_disable                                    : 1;
#define IA32_MISC_ENABLE_XTPR_MESSAGE_DISABLE_BIT                    23
#define IA32_MISC_ENABLE_XTPR_MESSAGE_DISABLE_FLAG                   0x800000
#define IA32_MISC_ENABLE_XTPR_MESSAGE_DISABLE_MASK                   0x01
#define IA32_MISC_ENABLE_XTPR_MESSAGE_DISABLE(_)                     (((_) >> 23) & 0x01)
	uint64_t reserved7                                               : 10;

	/**
	* @brief XD Bit Disable <b>(R/W)</b>
	*
	* [Bit 34] When set to 1, the Execute Disable Bit feature (XD Bit) is disabled and the XD Bit extended feature flag will
	* be clear (CPUID.80000001H: EDX[20]=0).
	* When set to a 0 (default), the Execute Disable Bit feature (if available) allows the OS to enable PAE paging and take
	* advantage of data only pages.
	* BIOS must not alter the contents of this bit location, if XD bit is not supported. Writing this bit to 1 when the XD Bit
	* extended feature flag is set to 0 may generate a \#GP exception.
	*
	* @remarks If CPUID.80000001H:EDX[20] = 1
	*/
	uint64_t xd_bit_disable                                          : 1;
#define IA32_MISC_ENABLE_XD_BIT_DISABLE_BIT                          34
#define IA32_MISC_ENABLE_XD_BIT_DISABLE_FLAG                         0x400000000
#define IA32_MISC_ENABLE_XD_BIT_DISABLE_MASK                         0x01
#define IA32_MISC_ENABLE_XD_BIT_DISABLE(_)                           (((_) >> 34) & 0x01)
	uint64_t reserved8                                               : 29;
  };

  uint64_t flags;
} misc_enable_register;


/**
 * Performance Energy Bias Hint.
 *
 * @remarks If CPUID.6H:ECX[3] = 1
 */
#define IA32_ENERGY_PERF_BIAS                                        0x000001B0
typedef union
{
  struct
  {
	/**
	* @brief Power Policy Preference
	*
	* [Bits 3:0] - 0 indicates preference to highest performance.
	* - 15 indicates preference to maximize energy saving.
	*/
	uint64_t power_policy_preference                                 : 4;
#define IA32_ENERGY_PERF_BIAS_POWER_POLICY_PREFERENCE_BIT            0
#define IA32_ENERGY_PERF_BIAS_POWER_POLICY_PREFERENCE_FLAG           0x0F
#define IA32_ENERGY_PERF_BIAS_POWER_POLICY_PREFERENCE_MASK           0x0F
#define IA32_ENERGY_PERF_BIAS_POWER_POLICY_PREFERENCE(_)             (((_) >> 0) & 0x0F)
	uint64_t reserved1                                               : 60;
  };

  uint64_t flags;
} energy_perf_bias_register;


/**
 * @brief Package Thermal Status Information <b>(RO)</b>
 *
 * Package Thermal Status Information. Contains status information about the package's thermal sensor.
 *
 * @remarks If CPUID.06H: EAX[6] = 1
 * @see Vol3B[14.8(PACKAGE LEVEL THERMAL MANAGEMENT)]
 */
#define IA32_PACKAGE_THERM_STATUS                                    0x000001B1
typedef union
{
  struct
  {
	/**
	* [Bit 0] Pkg Thermal Status
	*/
	uint64_t thermal_status                                          : 1;
#define IA32_PACKAGE_THERM_STATUS_THERMAL_STATUS_BIT                 0
#define IA32_PACKAGE_THERM_STATUS_THERMAL_STATUS_FLAG                0x01
#define IA32_PACKAGE_THERM_STATUS_THERMAL_STATUS_MASK                0x01
#define IA32_PACKAGE_THERM_STATUS_THERMAL_STATUS(_)                  (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Pkg Thermal Status Log
	*/
	uint64_t thermal_status_log                                      : 1;
#define IA32_PACKAGE_THERM_STATUS_THERMAL_STATUS_LOG_BIT             1
#define IA32_PACKAGE_THERM_STATUS_THERMAL_STATUS_LOG_FLAG            0x02
#define IA32_PACKAGE_THERM_STATUS_THERMAL_STATUS_LOG_MASK            0x01
#define IA32_PACKAGE_THERM_STATUS_THERMAL_STATUS_LOG(_)              (((_) >> 1) & 0x01)

	/**
	* [Bit 2] Pkg PROCHOT \# event
	*/
	uint64_t prochot_event                                           : 1;
#define IA32_PACKAGE_THERM_STATUS_PROCHOT_EVENT_BIT                  2
#define IA32_PACKAGE_THERM_STATUS_PROCHOT_EVENT_FLAG                 0x04
#define IA32_PACKAGE_THERM_STATUS_PROCHOT_EVENT_MASK                 0x01
#define IA32_PACKAGE_THERM_STATUS_PROCHOT_EVENT(_)                   (((_) >> 2) & 0x01)

	/**
	* [Bit 3] Pkg PROCHOT \# log
	*/
	uint64_t prochot_log                                             : 1;
#define IA32_PACKAGE_THERM_STATUS_PROCHOT_LOG_BIT                    3
#define IA32_PACKAGE_THERM_STATUS_PROCHOT_LOG_FLAG                   0x08
#define IA32_PACKAGE_THERM_STATUS_PROCHOT_LOG_MASK                   0x01
#define IA32_PACKAGE_THERM_STATUS_PROCHOT_LOG(_)                     (((_) >> 3) & 0x01)

	/**
	* [Bit 4] Pkg Critical Temperature Status
	*/
	uint64_t critical_temperature_status                             : 1;
#define IA32_PACKAGE_THERM_STATUS_CRITICAL_TEMPERATURE_STATUS_BIT    4
#define IA32_PACKAGE_THERM_STATUS_CRITICAL_TEMPERATURE_STATUS_FLAG   0x10
#define IA32_PACKAGE_THERM_STATUS_CRITICAL_TEMPERATURE_STATUS_MASK   0x01
#define IA32_PACKAGE_THERM_STATUS_CRITICAL_TEMPERATURE_STATUS(_)     (((_) >> 4) & 0x01)

	/**
	* [Bit 5] Pkg Critical Temperature Status Log
	*/
	uint64_t critical_temperature_status_log                         : 1;
#define IA32_PACKAGE_THERM_STATUS_CRITICAL_TEMPERATURE_STATUS_LOG_BIT 5
#define IA32_PACKAGE_THERM_STATUS_CRITICAL_TEMPERATURE_STATUS_LOG_FLAG 0x20
#define IA32_PACKAGE_THERM_STATUS_CRITICAL_TEMPERATURE_STATUS_LOG_MASK 0x01
#define IA32_PACKAGE_THERM_STATUS_CRITICAL_TEMPERATURE_STATUS_LOG(_) (((_) >> 5) & 0x01)

	/**
	* [Bit 6] Pkg Thermal Threshold \#1 Status
	*/
	uint64_t thermal_threshold1_status                               : 1;
#define IA32_PACKAGE_THERM_STATUS_THERMAL_THRESHOLD1_STATUS_BIT      6
#define IA32_PACKAGE_THERM_STATUS_THERMAL_THRESHOLD1_STATUS_FLAG     0x40
#define IA32_PACKAGE_THERM_STATUS_THERMAL_THRESHOLD1_STATUS_MASK     0x01
#define IA32_PACKAGE_THERM_STATUS_THERMAL_THRESHOLD1_STATUS(_)       (((_) >> 6) & 0x01)

	/**
	* [Bit 7] Pkg Thermal Threshold \#1 log
	*/
	uint64_t thermal_threshold1_log                                  : 1;
#define IA32_PACKAGE_THERM_STATUS_THERMAL_THRESHOLD1_LOG_BIT         7
#define IA32_PACKAGE_THERM_STATUS_THERMAL_THRESHOLD1_LOG_FLAG        0x80
#define IA32_PACKAGE_THERM_STATUS_THERMAL_THRESHOLD1_LOG_MASK        0x01
#define IA32_PACKAGE_THERM_STATUS_THERMAL_THRESHOLD1_LOG(_)          (((_) >> 7) & 0x01)

	/**
	* [Bit 8] Pkg Thermal Threshold \#2 Status
	*/
	uint64_t thermal_threshold2_status                               : 1;
#define IA32_PACKAGE_THERM_STATUS_THERMAL_THRESHOLD2_STATUS_BIT      8
#define IA32_PACKAGE_THERM_STATUS_THERMAL_THRESHOLD2_STATUS_FLAG     0x100
#define IA32_PACKAGE_THERM_STATUS_THERMAL_THRESHOLD2_STATUS_MASK     0x01
#define IA32_PACKAGE_THERM_STATUS_THERMAL_THRESHOLD2_STATUS(_)       (((_) >> 8) & 0x01)

	/**
	* [Bit 9] Pkg Thermal Threshold \#2 log
	*/
	uint64_t thermal_threshold2_log                                  : 1;
#define IA32_PACKAGE_THERM_STATUS_THERMAL_THRESHOLD2_LOG_BIT         9
#define IA32_PACKAGE_THERM_STATUS_THERMAL_THRESHOLD2_LOG_FLAG        0x200
#define IA32_PACKAGE_THERM_STATUS_THERMAL_THRESHOLD2_LOG_MASK        0x01
#define IA32_PACKAGE_THERM_STATUS_THERMAL_THRESHOLD2_LOG(_)          (((_) >> 9) & 0x01)

	/**
	* [Bit 10] Pkg Power Limitation Status
	*/
	uint64_t power_limitation_status                                 : 1;
#define IA32_PACKAGE_THERM_STATUS_POWER_LIMITATION_STATUS_BIT        10
#define IA32_PACKAGE_THERM_STATUS_POWER_LIMITATION_STATUS_FLAG       0x400
#define IA32_PACKAGE_THERM_STATUS_POWER_LIMITATION_STATUS_MASK       0x01
#define IA32_PACKAGE_THERM_STATUS_POWER_LIMITATION_STATUS(_)         (((_) >> 10) & 0x01)

	/**
	* [Bit 11] Pkg Power Limitation log
	*/
	uint64_t power_limitation_log                                    : 1;
#define IA32_PACKAGE_THERM_STATUS_POWER_LIMITATION_LOG_BIT           11
#define IA32_PACKAGE_THERM_STATUS_POWER_LIMITATION_LOG_FLAG          0x800
#define IA32_PACKAGE_THERM_STATUS_POWER_LIMITATION_LOG_MASK          0x01
#define IA32_PACKAGE_THERM_STATUS_POWER_LIMITATION_LOG(_)            (((_) >> 11) & 0x01)
	uint64_t reserved1                                               : 4;

	/**
	* [Bits 22:16] Pkg Digital Readout
	*/
	uint64_t digital_readout                                         : 7;
#define IA32_PACKAGE_THERM_STATUS_DIGITAL_READOUT_BIT                16
#define IA32_PACKAGE_THERM_STATUS_DIGITAL_READOUT_FLAG               0x7F0000
#define IA32_PACKAGE_THERM_STATUS_DIGITAL_READOUT_MASK               0x7F
#define IA32_PACKAGE_THERM_STATUS_DIGITAL_READOUT(_)                 (((_) >> 16) & 0x7F)
	uint64_t reserved2                                               : 41;
  };

  uint64_t flags;
} package_therm_status_register;


/**
 * @brief Package Thermal Interrupt Control <b>(RO)</b>
 *
 * Enables and disables the generation of an interrupt on temperature transitions detected with the package's thermal
 * sensor.
 *
 * @remarks If CPUID.06H: EAX[6] = 1
 * @see Vol3B[14.8(PACKAGE LEVEL THERMAL MANAGEMENT)]
 */
#define IA32_PACKAGE_THERM_INTERRUPT                                 0x000001B2
typedef union
{
  struct
  {
	/**
	* [Bit 0] Pkg High-Temperature Interrupt Enable.
	*/
	uint64_t high_temperature_interrupt_enable                       : 1;
#define IA32_PACKAGE_THERM_INTERRUPT_HIGH_TEMPERATURE_INTERRUPT_ENABLE_BIT 0
#define IA32_PACKAGE_THERM_INTERRUPT_HIGH_TEMPERATURE_INTERRUPT_ENABLE_FLAG 0x01
#define IA32_PACKAGE_THERM_INTERRUPT_HIGH_TEMPERATURE_INTERRUPT_ENABLE_MASK 0x01
#define IA32_PACKAGE_THERM_INTERRUPT_HIGH_TEMPERATURE_INTERRUPT_ENABLE(_) (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Pkg Low-Temperature Interrupt Enable.
	*/
	uint64_t low_temperature_interrupt_enable                        : 1;
#define IA32_PACKAGE_THERM_INTERRUPT_LOW_TEMPERATURE_INTERRUPT_ENABLE_BIT 1
#define IA32_PACKAGE_THERM_INTERRUPT_LOW_TEMPERATURE_INTERRUPT_ENABLE_FLAG 0x02
#define IA32_PACKAGE_THERM_INTERRUPT_LOW_TEMPERATURE_INTERRUPT_ENABLE_MASK 0x01
#define IA32_PACKAGE_THERM_INTERRUPT_LOW_TEMPERATURE_INTERRUPT_ENABLE(_) (((_) >> 1) & 0x01)

	/**
	* [Bit 2] Pkg PROCHOT\# Interrupt Enable.
	*/
	uint64_t prochot_interrupt_enable                                : 1;
#define IA32_PACKAGE_THERM_INTERRUPT_PROCHOT_INTERRUPT_ENABLE_BIT    2
#define IA32_PACKAGE_THERM_INTERRUPT_PROCHOT_INTERRUPT_ENABLE_FLAG   0x04
#define IA32_PACKAGE_THERM_INTERRUPT_PROCHOT_INTERRUPT_ENABLE_MASK   0x01
#define IA32_PACKAGE_THERM_INTERRUPT_PROCHOT_INTERRUPT_ENABLE(_)     (((_) >> 2) & 0x01)
	uint64_t reserved1                                               : 1;

	/**
	* [Bit 4] Pkg Overheat Interrupt Enable.
	*/
	uint64_t overheat_interrupt_enable                               : 1;
#define IA32_PACKAGE_THERM_INTERRUPT_OVERHEAT_INTERRUPT_ENABLE_BIT   4
#define IA32_PACKAGE_THERM_INTERRUPT_OVERHEAT_INTERRUPT_ENABLE_FLAG  0x10
#define IA32_PACKAGE_THERM_INTERRUPT_OVERHEAT_INTERRUPT_ENABLE_MASK  0x01
#define IA32_PACKAGE_THERM_INTERRUPT_OVERHEAT_INTERRUPT_ENABLE(_)    (((_) >> 4) & 0x01)
	uint64_t reserved2                                               : 3;

	/**
	* [Bits 14:8] Pkg Threshold \#1 Value
	*/
	uint64_t threshold1_value                                        : 7;
#define IA32_PACKAGE_THERM_INTERRUPT_THRESHOLD1_VALUE_BIT            8
#define IA32_PACKAGE_THERM_INTERRUPT_THRESHOLD1_VALUE_FLAG           0x7F00
#define IA32_PACKAGE_THERM_INTERRUPT_THRESHOLD1_VALUE_MASK           0x7F
#define IA32_PACKAGE_THERM_INTERRUPT_THRESHOLD1_VALUE(_)             (((_) >> 8) & 0x7F)

	/**
	* [Bit 15] Pkg Threshold \#1 Interrupt Enable.
	*/
	uint64_t threshold1_interrupt_enable                             : 1;
#define IA32_PACKAGE_THERM_INTERRUPT_THRESHOLD1_INTERRUPT_ENABLE_BIT 15
#define IA32_PACKAGE_THERM_INTERRUPT_THRESHOLD1_INTERRUPT_ENABLE_FLAG 0x8000
#define IA32_PACKAGE_THERM_INTERRUPT_THRESHOLD1_INTERRUPT_ENABLE_MASK 0x01
#define IA32_PACKAGE_THERM_INTERRUPT_THRESHOLD1_INTERRUPT_ENABLE(_)  (((_) >> 15) & 0x01)

	/**
	* [Bits 22:16] Pkg Threshold \#2 Value.
	*/
	uint64_t threshold2_value                                        : 7;
#define IA32_PACKAGE_THERM_INTERRUPT_THRESHOLD2_VALUE_BIT            16
#define IA32_PACKAGE_THERM_INTERRUPT_THRESHOLD2_VALUE_FLAG           0x7F0000
#define IA32_PACKAGE_THERM_INTERRUPT_THRESHOLD2_VALUE_MASK           0x7F
#define IA32_PACKAGE_THERM_INTERRUPT_THRESHOLD2_VALUE(_)             (((_) >> 16) & 0x7F)

	/**
	* [Bit 23] Pkg Threshold \#2 Interrupt Enable.
	*/
	uint64_t threshold2_interrupt_enable                             : 1;
#define IA32_PACKAGE_THERM_INTERRUPT_THRESHOLD2_INTERRUPT_ENABLE_BIT 23
#define IA32_PACKAGE_THERM_INTERRUPT_THRESHOLD2_INTERRUPT_ENABLE_FLAG 0x800000
#define IA32_PACKAGE_THERM_INTERRUPT_THRESHOLD2_INTERRUPT_ENABLE_MASK 0x01
#define IA32_PACKAGE_THERM_INTERRUPT_THRESHOLD2_INTERRUPT_ENABLE(_)  (((_) >> 23) & 0x01)

	/**
	* [Bit 24] Pkg Power Limit Notification Enable.
	*/
	uint64_t power_limit_notification_enable                         : 1;
#define IA32_PACKAGE_THERM_INTERRUPT_POWER_LIMIT_NOTIFICATION_ENABLE_BIT 24
#define IA32_PACKAGE_THERM_INTERRUPT_POWER_LIMIT_NOTIFICATION_ENABLE_FLAG 0x1000000
#define IA32_PACKAGE_THERM_INTERRUPT_POWER_LIMIT_NOTIFICATION_ENABLE_MASK 0x01
#define IA32_PACKAGE_THERM_INTERRUPT_POWER_LIMIT_NOTIFICATION_ENABLE(_) (((_) >> 24) & 0x01)
	uint64_t reserved3                                               : 39;
  };

  uint64_t flags;
} package_therm_interrupt_register;


/**
 * Trace/Profile Resource Control.
 *
 * @remarks 06_0EH
 */
#define IA32_DEBUGCTL                                                0x000001D9
typedef union
{
  struct
  {
	/**
	* [Bit 0] Setting this bit to 1 enables the processor to record a running trace of the most recent branches taken by the
	* processor in the LBR stack.
	*
	* @remarks 06_01H
	*/
	uint64_t lbr                                                     : 1;
#define IA32_DEBUGCTL_LBR_BIT                                        0
#define IA32_DEBUGCTL_LBR_FLAG                                       0x01
#define IA32_DEBUGCTL_LBR_MASK                                       0x01
#define IA32_DEBUGCTL_LBR(_)                                         (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Setting this bit to 1 enables the processor to treat EFLAGS.TF as single-step on branches instead of single-step
	* on instructions.
	*
	* @remarks 06_01H
	*/
	uint64_t btf                                                     : 1;
#define IA32_DEBUGCTL_BTF_BIT                                        1
#define IA32_DEBUGCTL_BTF_FLAG                                       0x02
#define IA32_DEBUGCTL_BTF_MASK                                       0x01
#define IA32_DEBUGCTL_BTF(_)                                         (((_) >> 1) & 0x01)
	uint64_t reserved1                                               : 4;

	/**
	* [Bit 6] Setting this bit to 1 enables branch trace messages to be sent.
	*
	* @remarks 06_0EH
	*/
	uint64_t tr                                                      : 1;
#define IA32_DEBUGCTL_TR_BIT                                         6
#define IA32_DEBUGCTL_TR_FLAG                                        0x40
#define IA32_DEBUGCTL_TR_MASK                                        0x01
#define IA32_DEBUGCTL_TR(_)                                          (((_) >> 6) & 0x01)

	/**
	* [Bit 7] Setting this bit enables branch trace messages (BTMs) to be logged in a BTS buffer.
	*
	* @remarks 06_0EH
	*/
	uint64_t bts                                                     : 1;
#define IA32_DEBUGCTL_BTS_BIT                                        7
#define IA32_DEBUGCTL_BTS_FLAG                                       0x80
#define IA32_DEBUGCTL_BTS_MASK                                       0x01
#define IA32_DEBUGCTL_BTS(_)                                         (((_) >> 7) & 0x01)

	/**
	* [Bit 8] When clear, BTMs are logged in a BTS buffer in circular fashion. When this bit is set, an interrupt is generated
	* by the BTS facility when the BTS buffer is full.
	*
	* @remarks 06_0EH
	*/
	uint64_t btint                                                   : 1;
#define IA32_DEBUGCTL_BTINT_BIT                                      8
#define IA32_DEBUGCTL_BTINT_FLAG                                     0x100
#define IA32_DEBUGCTL_BTINT_MASK                                     0x01
#define IA32_DEBUGCTL_BTINT(_)                                       (((_) >> 8) & 0x01)

	/**
	* [Bit 9] When set, BTS or BTM is skipped if CPL = 0.
	*
	* @remarks 06_0FH
	*/
	uint64_t bts_off_os                                              : 1;
#define IA32_DEBUGCTL_BTS_OFF_OS_BIT                                 9
#define IA32_DEBUGCTL_BTS_OFF_OS_FLAG                                0x200
#define IA32_DEBUGCTL_BTS_OFF_OS_MASK                                0x01
#define IA32_DEBUGCTL_BTS_OFF_OS(_)                                  (((_) >> 9) & 0x01)

	/**
	* [Bit 10] When set, BTS or BTM is skipped if CPL > 0.
	*
	* @remarks 06_0FH
	*/
	uint64_t bts_off_usr                                             : 1;
#define IA32_DEBUGCTL_BTS_OFF_USR_BIT                                10
#define IA32_DEBUGCTL_BTS_OFF_USR_FLAG                               0x400
#define IA32_DEBUGCTL_BTS_OFF_USR_MASK                               0x01
#define IA32_DEBUGCTL_BTS_OFF_USR(_)                                 (((_) >> 10) & 0x01)

	/**
	* [Bit 11] When set, the LBR stack is frozen on a PMI request.
	*
	* @remarks If CPUID.01H: ECX[15] = 1 && CPUID.0AH: EAX[7:0] > 1
	*/
	uint64_t freeze_lbrs_on_pmi                                      : 1;
#define IA32_DEBUGCTL_FREEZE_LBRS_ON_PMI_BIT                         11
#define IA32_DEBUGCTL_FREEZE_LBRS_ON_PMI_FLAG                        0x800
#define IA32_DEBUGCTL_FREEZE_LBRS_ON_PMI_MASK                        0x01
#define IA32_DEBUGCTL_FREEZE_LBRS_ON_PMI(_)                          (((_) >> 11) & 0x01)

	/**
	* [Bit 12] When set, each ENABLE bit of the global counter control MSR are frozen (address 38FH) on a PMI request.
	*
	* @remarks If CPUID.01H: ECX[15] = 1 && CPUID.0AH: EAX[7:0] > 1
	*/
	uint64_t freeze_perfmon_on_pmi                                   : 1;
#define IA32_DEBUGCTL_FREEZE_PERFMON_ON_PMI_BIT                      12
#define IA32_DEBUGCTL_FREEZE_PERFMON_ON_PMI_FLAG                     0x1000
#define IA32_DEBUGCTL_FREEZE_PERFMON_ON_PMI_MASK                     0x01
#define IA32_DEBUGCTL_FREEZE_PERFMON_ON_PMI(_)                       (((_) >> 12) & 0x01)

	/**
	* [Bit 13] When set, enables the logical processor to receive and generate PMI on behalf of the uncore.
	*
	* @remarks 06_1AH
	*/
	uint64_t enable_uncore_pmi                                       : 1;
#define IA32_DEBUGCTL_ENABLE_UNCORE_PMI_BIT                          13
#define IA32_DEBUGCTL_ENABLE_UNCORE_PMI_FLAG                         0x2000
#define IA32_DEBUGCTL_ENABLE_UNCORE_PMI_MASK                         0x01
#define IA32_DEBUGCTL_ENABLE_UNCORE_PMI(_)                           (((_) >> 13) & 0x01)

	/**
	* [Bit 14] When set, freezes perfmon and trace messages while in SMM.
	*
	* @remarks If IA32_PERF_CAPABILITIES[12] = 1
	*/
	uint64_t freeze_while_smm                                        : 1;
#define IA32_DEBUGCTL_FREEZE_WHILE_SMM_BIT                           14
#define IA32_DEBUGCTL_FREEZE_WHILE_SMM_FLAG                          0x4000
#define IA32_DEBUGCTL_FREEZE_WHILE_SMM_MASK                          0x01
#define IA32_DEBUGCTL_FREEZE_WHILE_SMM(_)                            (((_) >> 14) & 0x01)

	/**
	* [Bit 15] When set, enables DR7 debug bit on XBEGIN.
	*
	* @remarks If (CPUID.(EAX=07H, ECX=0):EBX[11] = 1)
	*/
	uint64_t rtm_debug                                               : 1;
#define IA32_DEBUGCTL_RTM_DEBUG_BIT                                  15
#define IA32_DEBUGCTL_RTM_DEBUG_FLAG                                 0x8000
#define IA32_DEBUGCTL_RTM_DEBUG_MASK                                 0x01
#define IA32_DEBUGCTL_RTM_DEBUG(_)                                   (((_) >> 15) & 0x01)
	uint64_t reserved2                                               : 48;
  };

  uint64_t flags;
} debugctl_register;


/**
 * @brief SMRR Base Address <b>(Writeable only in SMM)</b>
 *
 * SMRR Base Address. Base address of SMM memory range.
 *
 * @remarks If IA32_MTRRCAP.SMRR[11] = 1
 */
#define IA32_SMRR_PHYSBASE                                           0x000001F2
typedef union
{
  struct
  {
	/**
	* @brief Type
	*
	* [Bits 7:0] Type. Specifies memory type of the range.
	*/
	uint64_t type                                                    : 8;
#define IA32_SMRR_PHYSBASE_TYPE_BIT                                  0
#define IA32_SMRR_PHYSBASE_TYPE_FLAG                                 0xFF
#define IA32_SMRR_PHYSBASE_TYPE_MASK                                 0xFF
#define IA32_SMRR_PHYSBASE_TYPE(_)                                   (((_) >> 0) & 0xFF)
	uint64_t reserved1                                               : 4;

	/**
	* [Bits 31:12] SMRR physical Base Address.
	*/
	uint64_t smrr_physical_base_address                              : 20;
#define IA32_SMRR_PHYSBASE_SMRR_PHYSICAL_BASE_ADDRESS_BIT            12
#define IA32_SMRR_PHYSBASE_SMRR_PHYSICAL_BASE_ADDRESS_FLAG           0xFFFFF000
#define IA32_SMRR_PHYSBASE_SMRR_PHYSICAL_BASE_ADDRESS_MASK           0xFFFFF
#define IA32_SMRR_PHYSBASE_SMRR_PHYSICAL_BASE_ADDRESS(_)             (((_) >> 12) & 0xFFFFF)
	uint64_t reserved2                                               : 32;
  };

  uint64_t flags;
} smrr_physbase_register;


/**
 * @brief SMRR Range Mask <b>(Writeable only in SMM)</b>
 *
 * Range Mask of SMM memory range.
 *
 * @remarks If IA32_MTRRCAP[SMRR] = 1
 */
#define IA32_SMRR_PHYSMASK                                           0x000001F3
typedef union
{
  struct
  {
	uint64_t reserved1                                               : 11;

	/**
	* [Bit 11] Enable range mask.
	*/
	uint64_t enable_range_mask                                       : 1;
#define IA32_SMRR_PHYSMASK_ENABLE_RANGE_MASK_BIT                     11
#define IA32_SMRR_PHYSMASK_ENABLE_RANGE_MASK_FLAG                    0x800
#define IA32_SMRR_PHYSMASK_ENABLE_RANGE_MASK_MASK                    0x01
#define IA32_SMRR_PHYSMASK_ENABLE_RANGE_MASK(_)                      (((_) >> 11) & 0x01)

	/**
	* [Bits 31:12] SMRR address range mask.
	*/
	uint64_t smrr_address_range_mask                                 : 20;
#define IA32_SMRR_PHYSMASK_SMRR_ADDRESS_RANGE_MASK_BIT               12
#define IA32_SMRR_PHYSMASK_SMRR_ADDRESS_RANGE_MASK_FLAG              0xFFFFF000
#define IA32_SMRR_PHYSMASK_SMRR_ADDRESS_RANGE_MASK_MASK              0xFFFFF
#define IA32_SMRR_PHYSMASK_SMRR_ADDRESS_RANGE_MASK(_)                (((_) >> 12) & 0xFFFFF)
	uint64_t reserved2                                               : 32;
  };

  uint64_t flags;
} smrr_physmask_register;


/**
 * DCA Capability.
 *
 * @remarks If CPUID.01H: ECX[18] = 1
 */
#define IA32_PLATFORM_DCA_CAP                                        0x000001F8

/**
 * If set, CPU supports Prefetch-Hint type.
 *
 * @remarks If CPUID.01H: ECX[18] = 1
 */
#define IA32_CPU_DCA_CAP                                             0x000001F9

/**
 * DCA type 0 Status and Control register.
 *
 * @remarks If CPUID.01H: ECX[18] = 1
 */
#define IA32_DCA_0_CAP                                               0x000001FA
typedef union
{
  struct
  {
	/**
	* [Bit 0] Set by HW when DCA is fuseenabled and no defeatures are set.
	*/
	uint64_t dca_active                                              : 1;
#define IA32_DCA_0_CAP_DCA_ACTIVE_BIT                                0
#define IA32_DCA_0_CAP_DCA_ACTIVE_FLAG                               0x01
#define IA32_DCA_0_CAP_DCA_ACTIVE_MASK                               0x01
#define IA32_DCA_0_CAP_DCA_ACTIVE(_)                                 (((_) >> 0) & 0x01)

	/**
	* [Bits 2:1] TRANSACTION.
	*/
	uint64_t transaction                                             : 2;
#define IA32_DCA_0_CAP_TRANSACTION_BIT                               1
#define IA32_DCA_0_CAP_TRANSACTION_FLAG                              0x06
#define IA32_DCA_0_CAP_TRANSACTION_MASK                              0x03
#define IA32_DCA_0_CAP_TRANSACTION(_)                                (((_) >> 1) & 0x03)

	/**
	* [Bits 6:3] DCA_TYPE.
	*/
	uint64_t dca_type                                                : 4;
#define IA32_DCA_0_CAP_DCA_TYPE_BIT                                  3
#define IA32_DCA_0_CAP_DCA_TYPE_FLAG                                 0x78
#define IA32_DCA_0_CAP_DCA_TYPE_MASK                                 0x0F
#define IA32_DCA_0_CAP_DCA_TYPE(_)                                   (((_) >> 3) & 0x0F)

	/**
	* [Bits 10:7] DCA_QUEUE_SIZE.
	*/
	uint64_t dca_queue_size                                          : 4;
#define IA32_DCA_0_CAP_DCA_QUEUE_SIZE_BIT                            7
#define IA32_DCA_0_CAP_DCA_QUEUE_SIZE_FLAG                           0x780
#define IA32_DCA_0_CAP_DCA_QUEUE_SIZE_MASK                           0x0F
#define IA32_DCA_0_CAP_DCA_QUEUE_SIZE(_)                             (((_) >> 7) & 0x0F)
	uint64_t reserved1                                               : 2;

	/**
	* [Bits 16:13] Writes will update the register but have no HW side-effect.
	*/
	uint64_t dca_delay                                               : 4;
#define IA32_DCA_0_CAP_DCA_DELAY_BIT                                 13
#define IA32_DCA_0_CAP_DCA_DELAY_FLAG                                0x1E000
#define IA32_DCA_0_CAP_DCA_DELAY_MASK                                0x0F
#define IA32_DCA_0_CAP_DCA_DELAY(_)                                  (((_) >> 13) & 0x0F)
	uint64_t reserved2                                               : 7;

	/**
	* [Bit 24] SW can request DCA block by setting this bit.
	*/
	uint64_t sw_block                                                : 1;
#define IA32_DCA_0_CAP_SW_BLOCK_BIT                                  24
#define IA32_DCA_0_CAP_SW_BLOCK_FLAG                                 0x1000000
#define IA32_DCA_0_CAP_SW_BLOCK_MASK                                 0x01
#define IA32_DCA_0_CAP_SW_BLOCK(_)                                   (((_) >> 24) & 0x01)
	uint64_t reserved3                                               : 1;

	/**
	* [Bit 26] Set when DCA is blocked by HW (e.g. CR0.CD = 1).
	*/
	uint64_t hw_block                                                : 1;
#define IA32_DCA_0_CAP_HW_BLOCK_BIT                                  26
#define IA32_DCA_0_CAP_HW_BLOCK_FLAG                                 0x4000000
#define IA32_DCA_0_CAP_HW_BLOCK_MASK                                 0x01
#define IA32_DCA_0_CAP_HW_BLOCK(_)                                   (((_) >> 26) & 0x01)
	uint64_t reserved4                                               : 37;
  };

  uint64_t flags;
} dca_0_cap_register;

/**
 * @defgroup ia32_mtrr_physbase \
 *           IA32_MTRR_PHYSBASE(n)
 *
 * IA32_MTRR_PHYSBASE(0-9).
 *
 * @remarks If CPUID.01H: EDX.MTRR[12] = 1
 * @see Vol3A[11.11.2.3(Variable Range MTRRs)]
 * @{
 */
typedef union
{
  struct
  {
	/**
	* [Bits 7:0] Specifies the memory type for the range.
	*/
	uint64_t type                                                    : 8;
#define IA32_MTRR_PHYSBASE_TYPE_BIT                                  0
#define IA32_MTRR_PHYSBASE_TYPE_FLAG                                 0xFF
#define IA32_MTRR_PHYSBASE_TYPE_MASK                                 0xFF
#define IA32_MTRR_PHYSBASE_TYPE(_)                                   (((_) >> 0) & 0xFF)
	uint64_t reserved1                                               : 4;

	/**
	* [Bits 47:12] Specifies the base address of the address range. This 24-bit value, in the case where MAXPHYADDR is 36
	* bits, is extended by 12 bits at the low end to form the base address (this automatically aligns the address on a 4-KByte
	* boundary).
	*/
	uint64_t page_frame_number                                       : 36;
#define IA32_MTRR_PHYSBASE_PAGE_FRAME_NUMBER_BIT                     12
#define IA32_MTRR_PHYSBASE_PAGE_FRAME_NUMBER_FLAG                    0xFFFFFFFFF000
#define IA32_MTRR_PHYSBASE_PAGE_FRAME_NUMBER_MASK                    0xFFFFFFFFF
#define IA32_MTRR_PHYSBASE_PAGE_FRAME_NUMBER(_)                      (((_) >> 12) & 0xFFFFFFFFF)
	uint64_t reserved2                                               : 16;
  };

  uint64_t flags;
} mtrr_physbase_register;

#define IA32_MTRR_PHYSBASE0                                          0x00000200
#define IA32_MTRR_PHYSBASE1                                          0x00000202
#define IA32_MTRR_PHYSBASE2                                          0x00000204
#define IA32_MTRR_PHYSBASE3                                          0x00000206
#define IA32_MTRR_PHYSBASE4                                          0x00000208
#define IA32_MTRR_PHYSBASE5                                          0x0000020A
#define IA32_MTRR_PHYSBASE6                                          0x0000020C
#define IA32_MTRR_PHYSBASE7                                          0x0000020E
#define IA32_MTRR_PHYSBASE8                                          0x00000210
#define IA32_MTRR_PHYSBASE9                                          0x00000212
/**
 * @}
 */

/**
 * @defgroup ia32_mtrr_physmask \
 *           IA32_MTRR_PHYSMASK(n)
 *
 * IA32_MTRR_PHYSMASK(0-9).
 *
 * @remarks If CPUID.01H: EDX.MTRR[12] = 1
 * @see Vol3A[11.11.2.3(Variable Range MTRRs)]
 * @{
 */
typedef union
{
  struct
  {
	/**
	* [Bits 7:0] Specifies the memory type for the range.
	*/
	uint64_t type                                                    : 8;
#define IA32_MTRR_PHYSMASK_TYPE_BIT                                  0
#define IA32_MTRR_PHYSMASK_TYPE_FLAG                                 0xFF
#define IA32_MTRR_PHYSMASK_TYPE_MASK                                 0xFF
#define IA32_MTRR_PHYSMASK_TYPE(_)                                   (((_) >> 0) & 0xFF)
	uint64_t reserved1                                               : 3;

	/**
	* [Bit 11] Enables the register pair when set; disables register pair when clear.
	*/
	uint64_t valid                                                   : 1;
#define IA32_MTRR_PHYSMASK_VALID_BIT                                 11
#define IA32_MTRR_PHYSMASK_VALID_FLAG                                0x800
#define IA32_MTRR_PHYSMASK_VALID_MASK                                0x01
#define IA32_MTRR_PHYSMASK_VALID(_)                                  (((_) >> 11) & 0x01)

	/**
	* [Bits 47:12] Specifies a mask (24 bits if the maximum physical address size is 36 bits, 28 bits if the maximum physical
	* address size is 40 bits). The mask determines the range of the region being mapped, according to the following
	* relationships:
	* - Address_Within_Range AND PhysMask = PhysBase AND PhysMask
	* - This value is extended by 12 bits at the low end to form the mask value.
	* - The width of the PhysMask field depends on the maximum physical address size supported by the processor.
	* CPUID.80000008H reports the maximum physical address size supported by the processor. If CPUID.80000008H is not
	* available, software may assume that the processor supports a 36-bit physical address size.
	*
	* @see Vol3A[11.11.3(Example Base and Mask Calculations)]
	*/
	uint64_t page_frame_number                                       : 36;
#define IA32_MTRR_PHYSMASK_PAGE_FRAME_NUMBER_BIT                     12
#define IA32_MTRR_PHYSMASK_PAGE_FRAME_NUMBER_FLAG                    0xFFFFFFFFF000
#define IA32_MTRR_PHYSMASK_PAGE_FRAME_NUMBER_MASK                    0xFFFFFFFFF
#define IA32_MTRR_PHYSMASK_PAGE_FRAME_NUMBER(_)                      (((_) >> 12) & 0xFFFFFFFFF)
	uint64_t reserved2                                               : 16;
  };

  uint64_t flags;
} mtrr_physmask_register;

#define IA32_MTRR_PHYSMASK0                                          0x00000201
#define IA32_MTRR_PHYSMASK1                                          0x00000203
#define IA32_MTRR_PHYSMASK2                                          0x00000205
#define IA32_MTRR_PHYSMASK3                                          0x00000207
#define IA32_MTRR_PHYSMASK4                                          0x00000209
#define IA32_MTRR_PHYSMASK5                                          0x0000020B
#define IA32_MTRR_PHYSMASK6                                          0x0000020D
#define IA32_MTRR_PHYSMASK7                                          0x0000020F
#define IA32_MTRR_PHYSMASK8                                          0x00000211
#define IA32_MTRR_PHYSMASK9                                          0x00000213
/**
 * @}
 */

/**
 * @defgroup ia32_mtrr_fix \
 *           IA32_MTRR_FIX(x)
 *
 * IA32_MTRR_FIX(x).
 *
 * @remarks If CPUID.01H: EDX.MTRR[12] = 1
 * @see Vol3A[11.11.2.2(Fixed Range MTRRs)]
 * @{
 */
/**
 * @defgroup ia32_mtrr_fix64k \
 *           IA32_MTRR_FIX64K(x)
 *
 * IA32_MTRR_FIX64K(x).
 * @{
 */
#define IA32_MTRR_FIX64K_BASE                                        0x00000000
#define IA32_MTRR_FIX64K_SIZE                                        0x00010000
#define IA32_MTRR_FIX64K_00000                                       0x00000250
/**
 * @}
 */

/**
 * @defgroup ia32_mtrr_fix16k \
 *           IA32_MTRR_FIX16K(x)
 *
 * IA32_MTRR_FIX16K(x).
 * @{
 */
#define IA32_MTRR_FIX16K_BASE                                        0x00080000
#define IA32_MTRR_FIX16K_SIZE                                        0x00004000
#define IA32_MTRR_FIX16K_80000                                       0x00000258
#define IA32_MTRR_FIX16K_A0000                                       0x00000259
/**
 * @}
 */

/**
 * @defgroup ia32_mtrr_fix4k \
 *           IA32_MTRR_FIX4K(x)
 *
 * IA32_MTRR_FIX4K(x).
 * @{
 */
#define IA32_MTRR_FIX4K_BASE                                         0x000C0000
#define IA32_MTRR_FIX4K_SIZE                                         0x00001000
#define IA32_MTRR_FIX4K_C0000                                        0x00000268
#define IA32_MTRR_FIX4K_C8000                                        0x00000269
#define IA32_MTRR_FIX4K_D0000                                        0x0000026A
#define IA32_MTRR_FIX4K_D8000                                        0x0000026B
#define IA32_MTRR_FIX4K_E0000                                        0x0000026C
#define IA32_MTRR_FIX4K_E8000                                        0x0000026D
#define IA32_MTRR_FIX4K_F0000                                        0x0000026E
#define IA32_MTRR_FIX4K_F8000                                        0x0000026F
/**
 * @}
 */

/**
 * Architecture defined number of fixed range MTRRs (1 for 64k, 2 for 16k, 8 for 4k).
 */
#define IA32_MTRR_FIX_COUNT                                          ((1 + 2 + 8) * 8)

/**
 * Architecture defined number of variable range MTRRs.
 */
#define IA32_MTRR_VARIABLE_COUNT                                     0x000000FF

/**
 * A size of array to store all possible MTRRs.
 */
#define IA32_MTRR_COUNT                                              (IA32_MTRR_FIX_COUNT + IA32_MTRR_VARIABLE_COUNT)
/**
 * @}
 */


/**
 * IA32_PAT.
 *
 * @remarks If CPUID.01H: EDX.MTRR[16] = 1
 */
#define IA32_PAT                                                     0x00000277
typedef union
{
  struct
  {
	/**
	* [Bits 2:0] PA0.
	*/
	uint64_t pa0                                                     : 3;
#define IA32_PAT_PA0_BIT                                             0
#define IA32_PAT_PA0_FLAG                                            0x07
#define IA32_PAT_PA0_MASK                                            0x07
#define IA32_PAT_PA0(_)                                              (((_) >> 0) & 0x07)
	uint64_t reserved1                                               : 5;

	/**
	* [Bits 10:8] PA1.
	*/
	uint64_t pa1                                                     : 3;
#define IA32_PAT_PA1_BIT                                             8
#define IA32_PAT_PA1_FLAG                                            0x700
#define IA32_PAT_PA1_MASK                                            0x07
#define IA32_PAT_PA1(_)                                              (((_) >> 8) & 0x07)
	uint64_t reserved2                                               : 5;

	/**
	* [Bits 18:16] PA2.
	*/
	uint64_t pa2                                                     : 3;
#define IA32_PAT_PA2_BIT                                             16
#define IA32_PAT_PA2_FLAG                                            0x70000
#define IA32_PAT_PA2_MASK                                            0x07
#define IA32_PAT_PA2(_)                                              (((_) >> 16) & 0x07)
	uint64_t reserved3                                               : 5;

	/**
	* [Bits 26:24] PA3.
	*/
	uint64_t pa3                                                     : 3;
#define IA32_PAT_PA3_BIT                                             24
#define IA32_PAT_PA3_FLAG                                            0x7000000
#define IA32_PAT_PA3_MASK                                            0x07
#define IA32_PAT_PA3(_)                                              (((_) >> 24) & 0x07)
	uint64_t reserved4                                               : 5;

	/**
	* [Bits 34:32] PA4.
	*/
	uint64_t pa4                                                     : 3;
#define IA32_PAT_PA4_BIT                                             32
#define IA32_PAT_PA4_FLAG                                            0x700000000
#define IA32_PAT_PA4_MASK                                            0x07
#define IA32_PAT_PA4(_)                                              (((_) >> 32) & 0x07)
	uint64_t reserved5                                               : 5;

	/**
	* [Bits 42:40] PA5.
	*/
	uint64_t pa5                                                     : 3;
#define IA32_PAT_PA5_BIT                                             40
#define IA32_PAT_PA5_FLAG                                            0x70000000000
#define IA32_PAT_PA5_MASK                                            0x07
#define IA32_PAT_PA5(_)                                              (((_) >> 40) & 0x07)
	uint64_t reserved6                                               : 5;

	/**
	* [Bits 50:48] PA6.
	*/
	uint64_t pa6                                                     : 3;
#define IA32_PAT_PA6_BIT                                             48
#define IA32_PAT_PA6_FLAG                                            0x7000000000000
#define IA32_PAT_PA6_MASK                                            0x07
#define IA32_PAT_PA6(_)                                              (((_) >> 48) & 0x07)
	uint64_t reserved7                                               : 5;

	/**
	* [Bits 58:56] PA7.
	*/
	uint64_t pa7                                                     : 3;
#define IA32_PAT_PA7_BIT                                             56
#define IA32_PAT_PA7_FLAG                                            0x700000000000000
#define IA32_PAT_PA7_MASK                                            0x07
#define IA32_PAT_PA7(_)                                              (((_) >> 56) & 0x07)
	uint64_t reserved8                                               : 5;
  };

  uint64_t flags;
} pat_register;

/**
 * @defgroup ia32_mc_ctl2 \
 *           IA32_MC(i)_CTL2
 *
 * MSR to enable/disable CMCI capability for bank n.
 *
 * @remarks If IA32_MCG_CAP[10] = 1 && IA32_MCG_CAP[7:0] > n
 * @see Vol3B[15.3.2.5(IA32_MCi_CTL2 MSRs)]
 * @{
 */
#define IA32_MC0_CTL2                                                0x00000280
#define IA32_MC1_CTL2                                                0x00000281
#define IA32_MC2_CTL2                                                0x00000282
#define IA32_MC3_CTL2                                                0x00000283
#define IA32_MC4_CTL2                                                0x00000284
#define IA32_MC5_CTL2                                                0x00000285
#define IA32_MC6_CTL2                                                0x00000286
#define IA32_MC7_CTL2                                                0x00000287
#define IA32_MC8_CTL2                                                0x00000288
#define IA32_MC9_CTL2                                                0x00000289
#define IA32_MC10_CTL2                                               0x0000028A
#define IA32_MC11_CTL2                                               0x0000028B
#define IA32_MC12_CTL2                                               0x0000028C
#define IA32_MC13_CTL2                                               0x0000028D
#define IA32_MC14_CTL2                                               0x0000028E
#define IA32_MC15_CTL2                                               0x0000028F
#define IA32_MC16_CTL2                                               0x00000290
#define IA32_MC17_CTL2                                               0x00000291
#define IA32_MC18_CTL2                                               0x00000292
#define IA32_MC19_CTL2                                               0x00000293
#define IA32_MC20_CTL2                                               0x00000294
#define IA32_MC21_CTL2                                               0x00000295
#define IA32_MC22_CTL2                                               0x00000296
#define IA32_MC23_CTL2                                               0x00000297
#define IA32_MC24_CTL2                                               0x00000298
#define IA32_MC25_CTL2                                               0x00000299
#define IA32_MC26_CTL2                                               0x0000029A
#define IA32_MC27_CTL2                                               0x0000029B
#define IA32_MC28_CTL2                                               0x0000029C
#define IA32_MC29_CTL2                                               0x0000029D
#define IA32_MC30_CTL2                                               0x0000029E
#define IA32_MC31_CTL2                                               0x0000029F
typedef union
{
  struct
  {
	/**
	* [Bits 14:0] Corrected error count threshold.
	*/
	uint64_t corrected_error_count_threshold                         : 15;
#define IA32_MC_CTL2_CORRECTED_ERROR_COUNT_THRESHOLD_BIT             0
#define IA32_MC_CTL2_CORRECTED_ERROR_COUNT_THRESHOLD_FLAG            0x7FFF
#define IA32_MC_CTL2_CORRECTED_ERROR_COUNT_THRESHOLD_MASK            0x7FFF
#define IA32_MC_CTL2_CORRECTED_ERROR_COUNT_THRESHOLD(_)              (((_) >> 0) & 0x7FFF)
	uint64_t reserved1                                               : 15;

	/**
	* [Bit 30] CMCI_EN.
	*/
	uint64_t cmci_en                                                 : 1;
#define IA32_MC_CTL2_CMCI_EN_BIT                                     30
#define IA32_MC_CTL2_CMCI_EN_FLAG                                    0x40000000
#define IA32_MC_CTL2_CMCI_EN_MASK                                    0x01
#define IA32_MC_CTL2_CMCI_EN(_)                                      (((_) >> 30) & 0x01)
	uint64_t reserved2                                               : 33;
  };

  uint64_t flags;
} mc_ctl2_register;

/**
 * @}
 */


/**
 * IA32_MTRR_DEF_TYPE.
 *
 * @remarks If CPUID.01H: EDX.MTRR[12] = 1
 */
#define IA32_MTRR_DEF_TYPE                                           0x000002FF
typedef union
{
  struct
  {
	/**
	* [Bits 2:0] Default Memory Type.
	*/
	uint64_t default_memory_type                                     : 3;
#define IA32_MTRR_DEF_TYPE_DEFAULT_MEMORY_TYPE_BIT                   0
#define IA32_MTRR_DEF_TYPE_DEFAULT_MEMORY_TYPE_FLAG                  0x07
#define IA32_MTRR_DEF_TYPE_DEFAULT_MEMORY_TYPE_MASK                  0x07
#define IA32_MTRR_DEF_TYPE_DEFAULT_MEMORY_TYPE(_)                    (((_) >> 0) & 0x07)
	uint64_t reserved1                                               : 7;

	/**
	* [Bit 10] Fixed Range MTRR Enable.
	*/
	uint64_t fixed_range_mtrr_enable                                 : 1;
#define IA32_MTRR_DEF_TYPE_FIXED_RANGE_MTRR_ENABLE_BIT               10
#define IA32_MTRR_DEF_TYPE_FIXED_RANGE_MTRR_ENABLE_FLAG              0x400
#define IA32_MTRR_DEF_TYPE_FIXED_RANGE_MTRR_ENABLE_MASK              0x01
#define IA32_MTRR_DEF_TYPE_FIXED_RANGE_MTRR_ENABLE(_)                (((_) >> 10) & 0x01)

	/**
	* [Bit 11] MTRR Enable.
	*/
	uint64_t mtrr_enable                                             : 1;
#define IA32_MTRR_DEF_TYPE_MTRR_ENABLE_BIT                           11
#define IA32_MTRR_DEF_TYPE_MTRR_ENABLE_FLAG                          0x800
#define IA32_MTRR_DEF_TYPE_MTRR_ENABLE_MASK                          0x01
#define IA32_MTRR_DEF_TYPE_MTRR_ENABLE(_)                            (((_) >> 11) & 0x01)
	uint64_t reserved2                                               : 52;
  };

  uint64_t flags;
} mtrr_def_type_register;

/**
 * @defgroup ia32_fixed_ctr \
 *           IA32_FIXED_CTR(n)
 *
 * Fixed-Function Performance Counter n.
 *
 * @remarks If CPUID.0AH: EDX[4:0] > n
 * @{
 */
/**
 * Counts Instr_Retired.Any.
 */
#define IA32_FIXED_CTR0                                              0x00000309

/**
 * Counts CPU_CLK_Unhalted.Core
 */
#define IA32_FIXED_CTR1                                              0x0000030A

/**
 * Counts CPU_CLK_Unhalted.Ref
 */
#define IA32_FIXED_CTR2                                              0x0000030B
/**
 * @}
 */


/**
 * Read Only MSR that enumerates the existence of performance monitoring features.
 *
 * @remarks If CPUID.01H: ECX[15] = 1
 */
#define IA32_PERF_CAPABILITIES                                       0x00000345
typedef union
{
  struct
  {
	/**
	* [Bits 5:0] LBR format.
	*/
	uint64_t lbr_format                                              : 6;
#define IA32_PERF_CAPABILITIES_LBR_FORMAT_BIT                        0
#define IA32_PERF_CAPABILITIES_LBR_FORMAT_FLAG                       0x3F
#define IA32_PERF_CAPABILITIES_LBR_FORMAT_MASK                       0x3F
#define IA32_PERF_CAPABILITIES_LBR_FORMAT(_)                         (((_) >> 0) & 0x3F)

	/**
	* [Bit 6] PEBS Trap.
	*/
	uint64_t pebs_trap                                               : 1;
#define IA32_PERF_CAPABILITIES_PEBS_TRAP_BIT                         6
#define IA32_PERF_CAPABILITIES_PEBS_TRAP_FLAG                        0x40
#define IA32_PERF_CAPABILITIES_PEBS_TRAP_MASK                        0x01
#define IA32_PERF_CAPABILITIES_PEBS_TRAP(_)                          (((_) >> 6) & 0x01)

	/**
	* [Bit 7] PEBSSaveArchRegs.
	*/
	uint64_t pebs_save_arch_regs                                     : 1;
#define IA32_PERF_CAPABILITIES_PEBS_SAVE_ARCH_REGS_BIT               7
#define IA32_PERF_CAPABILITIES_PEBS_SAVE_ARCH_REGS_FLAG              0x80
#define IA32_PERF_CAPABILITIES_PEBS_SAVE_ARCH_REGS_MASK              0x01
#define IA32_PERF_CAPABILITIES_PEBS_SAVE_ARCH_REGS(_)                (((_) >> 7) & 0x01)

	/**
	* [Bits 11:8] PEBS Record Format.
	*/
	uint64_t pebs_record_format                                      : 4;
#define IA32_PERF_CAPABILITIES_PEBS_RECORD_FORMAT_BIT                8
#define IA32_PERF_CAPABILITIES_PEBS_RECORD_FORMAT_FLAG               0xF00
#define IA32_PERF_CAPABILITIES_PEBS_RECORD_FORMAT_MASK               0x0F
#define IA32_PERF_CAPABILITIES_PEBS_RECORD_FORMAT(_)                 (((_) >> 8) & 0x0F)

	/**
	* [Bit 12] Freeze while SMM is supported.
	*/
	uint64_t freeze_while_smm_is_supported                           : 1;
#define IA32_PERF_CAPABILITIES_FREEZE_WHILE_SMM_IS_SUPPORTED_BIT     12
#define IA32_PERF_CAPABILITIES_FREEZE_WHILE_SMM_IS_SUPPORTED_FLAG    0x1000
#define IA32_PERF_CAPABILITIES_FREEZE_WHILE_SMM_IS_SUPPORTED_MASK    0x01
#define IA32_PERF_CAPABILITIES_FREEZE_WHILE_SMM_IS_SUPPORTED(_)      (((_) >> 12) & 0x01)

	/**
	* [Bit 13] Full width of counter writable via IA32_A_PMCx.
	*/
	uint64_t full_width_counter_write                                : 1;
#define IA32_PERF_CAPABILITIES_FULL_WIDTH_COUNTER_WRITE_BIT          13
#define IA32_PERF_CAPABILITIES_FULL_WIDTH_COUNTER_WRITE_FLAG         0x2000
#define IA32_PERF_CAPABILITIES_FULL_WIDTH_COUNTER_WRITE_MASK         0x01
#define IA32_PERF_CAPABILITIES_FULL_WIDTH_COUNTER_WRITE(_)           (((_) >> 13) & 0x01)
	uint64_t reserved1                                               : 50;
  };

  uint64_t flags;
} perf_capabilities_register;


/**
 * @brief Fixed-Function Performance Counter Control <b>(R/W)</b>
 *
 * Fixed-Function Performance Counter Control. Counter increments while the results of ANDing respective enable bit in
 * IA32_PERF_GLOBAL_CTRL with the corresponding OS or USR bits in this MSR is true.
 *
 * @remarks If CPUID.0AH: EAX[7:0] > 1
 */
#define IA32_FIXED_CTR_CTRL                                          0x0000038D
typedef union
{
  struct
  {
	/**
	* [Bit 0] EN0_OS: Enable Fixed Counter 0 to count while CPL = 0.
	*/
	uint64_t en0_os                                                  : 1;
#define IA32_FIXED_CTR_CTRL_EN0_OS_BIT                               0
#define IA32_FIXED_CTR_CTRL_EN0_OS_FLAG                              0x01
#define IA32_FIXED_CTR_CTRL_EN0_OS_MASK                              0x01
#define IA32_FIXED_CTR_CTRL_EN0_OS(_)                                (((_) >> 0) & 0x01)

	/**
	* [Bit 1] EN0_Usr: Enable Fixed Counter 0 to count while CPL > 0.
	*/
	uint64_t en0_usr                                                 : 1;
#define IA32_FIXED_CTR_CTRL_EN0_USR_BIT                              1
#define IA32_FIXED_CTR_CTRL_EN0_USR_FLAG                             0x02
#define IA32_FIXED_CTR_CTRL_EN0_USR_MASK                             0x01
#define IA32_FIXED_CTR_CTRL_EN0_USR(_)                               (((_) >> 1) & 0x01)

	/**
	* [Bit 2] AnyThread: When set to 1, it enables counting the associated event conditions occurring across all logical
	* processors sharing a processor core. When set to 0, the counter only increments the associated event conditions
	* occurring in the logical processor which programmed the MSR.
	*/
	uint64_t any_thread0                                             : 1;
#define IA32_FIXED_CTR_CTRL_ANY_THREAD0_BIT                          2
#define IA32_FIXED_CTR_CTRL_ANY_THREAD0_FLAG                         0x04
#define IA32_FIXED_CTR_CTRL_ANY_THREAD0_MASK                         0x01
#define IA32_FIXED_CTR_CTRL_ANY_THREAD0(_)                           (((_) >> 2) & 0x01)

	/**
	* [Bit 3] EN0_PMI: Enable PMI when fixed counter 0 overflows.
	*/
	uint64_t en0_pmi                                                 : 1;
#define IA32_FIXED_CTR_CTRL_EN0_PMI_BIT                              3
#define IA32_FIXED_CTR_CTRL_EN0_PMI_FLAG                             0x08
#define IA32_FIXED_CTR_CTRL_EN0_PMI_MASK                             0x01
#define IA32_FIXED_CTR_CTRL_EN0_PMI(_)                               (((_) >> 3) & 0x01)

	/**
	* [Bit 4] EN1_OS: Enable Fixed Counter 1 to count while CPL = 0.
	*/
	uint64_t en1_os                                                  : 1;
#define IA32_FIXED_CTR_CTRL_EN1_OS_BIT                               4
#define IA32_FIXED_CTR_CTRL_EN1_OS_FLAG                              0x10
#define IA32_FIXED_CTR_CTRL_EN1_OS_MASK                              0x01
#define IA32_FIXED_CTR_CTRL_EN1_OS(_)                                (((_) >> 4) & 0x01)

	/**
	* [Bit 5] EN1_Usr: Enable Fixed Counter 1 to count while CPL > 0.
	*/
	uint64_t en1_usr                                                 : 1;
#define IA32_FIXED_CTR_CTRL_EN1_USR_BIT                              5
#define IA32_FIXED_CTR_CTRL_EN1_USR_FLAG                             0x20
#define IA32_FIXED_CTR_CTRL_EN1_USR_MASK                             0x01
#define IA32_FIXED_CTR_CTRL_EN1_USR(_)                               (((_) >> 5) & 0x01)

	/**
	* [Bit 6] AnyThread: When set to 1, it enables counting the associated event conditions occurring across all logical
	* processors sharing a processor core. When set to 0, the counter only increments the associated event conditions
	* occurring in the logical processor which programmed the MSR.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 2
	*/
	uint64_t any_thread1                                             : 1;
#define IA32_FIXED_CTR_CTRL_ANY_THREAD1_BIT                          6
#define IA32_FIXED_CTR_CTRL_ANY_THREAD1_FLAG                         0x40
#define IA32_FIXED_CTR_CTRL_ANY_THREAD1_MASK                         0x01
#define IA32_FIXED_CTR_CTRL_ANY_THREAD1(_)                           (((_) >> 6) & 0x01)

	/**
	* [Bit 7] EN1_PMI: Enable PMI when fixed counter 1 overflows.
	*/
	uint64_t en1_pmi                                                 : 1;
#define IA32_FIXED_CTR_CTRL_EN1_PMI_BIT                              7
#define IA32_FIXED_CTR_CTRL_EN1_PMI_FLAG                             0x80
#define IA32_FIXED_CTR_CTRL_EN1_PMI_MASK                             0x01
#define IA32_FIXED_CTR_CTRL_EN1_PMI(_)                               (((_) >> 7) & 0x01)

	/**
	* [Bit 8] EN2_OS: Enable Fixed Counter 2 to count while CPL = 0.
	*/
	uint64_t en2_os                                                  : 1;
#define IA32_FIXED_CTR_CTRL_EN2_OS_BIT                               8
#define IA32_FIXED_CTR_CTRL_EN2_OS_FLAG                              0x100
#define IA32_FIXED_CTR_CTRL_EN2_OS_MASK                              0x01
#define IA32_FIXED_CTR_CTRL_EN2_OS(_)                                (((_) >> 8) & 0x01)

	/**
	* [Bit 9] EN2_Usr: Enable Fixed Counter 2 to count while CPL > 0.
	*/
	uint64_t en2_usr                                                 : 1;
#define IA32_FIXED_CTR_CTRL_EN2_USR_BIT                              9
#define IA32_FIXED_CTR_CTRL_EN2_USR_FLAG                             0x200
#define IA32_FIXED_CTR_CTRL_EN2_USR_MASK                             0x01
#define IA32_FIXED_CTR_CTRL_EN2_USR(_)                               (((_) >> 9) & 0x01)

	/**
	* [Bit 10] AnyThread: When set to 1, it enables counting the associated event conditions occurring across all logical
	* processors sharing a processor core. When set to 0, the counter only increments the associated event conditions
	* occurring in the logical processor which programmed the MSR.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 2
	*/
	uint64_t any_thread2                                             : 1;
#define IA32_FIXED_CTR_CTRL_ANY_THREAD2_BIT                          10
#define IA32_FIXED_CTR_CTRL_ANY_THREAD2_FLAG                         0x400
#define IA32_FIXED_CTR_CTRL_ANY_THREAD2_MASK                         0x01
#define IA32_FIXED_CTR_CTRL_ANY_THREAD2(_)                           (((_) >> 10) & 0x01)

	/**
	* [Bit 11] EN2_PMI: Enable PMI when fixed counter 2 overflows.
	*/
	uint64_t en2_pmi                                                 : 1;
#define IA32_FIXED_CTR_CTRL_EN2_PMI_BIT                              11
#define IA32_FIXED_CTR_CTRL_EN2_PMI_FLAG                             0x800
#define IA32_FIXED_CTR_CTRL_EN2_PMI_MASK                             0x01
#define IA32_FIXED_CTR_CTRL_EN2_PMI(_)                               (((_) >> 11) & 0x01)
	uint64_t reserved1                                               : 52;
  };

  uint64_t flags;
} fixed_ctr_ctrl_register;


/**
 * Global Performance Counter Status.
 *
 * @remarks If CPUID.0AH: EAX[7:0] > 0
 */
#define IA32_PERF_GLOBAL_STATUS                                      0x0000038E
typedef union
{
  struct
  {
	/**
	* [Bit 0] Ovf_PMC0: Overflow status of IA32_PMC0.
	*
	* @remarks If CPUID.0AH: EAX[15:8] > 0
	*/
	uint64_t ovf_pmc0                                                : 1;
#define IA32_PERF_GLOBAL_STATUS_OVF_PMC0_BIT                         0
#define IA32_PERF_GLOBAL_STATUS_OVF_PMC0_FLAG                        0x01
#define IA32_PERF_GLOBAL_STATUS_OVF_PMC0_MASK                        0x01
#define IA32_PERF_GLOBAL_STATUS_OVF_PMC0(_)                          (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Ovf_PMC1: Overflow status of IA32_PMC1.
	*
	* @remarks If CPUID.0AH: EAX[15:8] > 1
	*/
	uint64_t ovf_pmc1                                                : 1;
#define IA32_PERF_GLOBAL_STATUS_OVF_PMC1_BIT                         1
#define IA32_PERF_GLOBAL_STATUS_OVF_PMC1_FLAG                        0x02
#define IA32_PERF_GLOBAL_STATUS_OVF_PMC1_MASK                        0x01
#define IA32_PERF_GLOBAL_STATUS_OVF_PMC1(_)                          (((_) >> 1) & 0x01)

	/**
	* [Bit 2] Ovf_PMC2: Overflow status of IA32_PMC2.
	*
	* @remarks If CPUID.0AH: EAX[15:8] > 2
	*/
	uint64_t ovf_pmc2                                                : 1;
#define IA32_PERF_GLOBAL_STATUS_OVF_PMC2_BIT                         2
#define IA32_PERF_GLOBAL_STATUS_OVF_PMC2_FLAG                        0x04
#define IA32_PERF_GLOBAL_STATUS_OVF_PMC2_MASK                        0x01
#define IA32_PERF_GLOBAL_STATUS_OVF_PMC2(_)                          (((_) >> 2) & 0x01)

	/**
	* [Bit 3] Ovf_PMC3: Overflow status of IA32_PMC3.
	*
	* @remarks If CPUID.0AH: EAX[15:8] > 3
	*/
	uint64_t ovf_pmc3                                                : 1;
#define IA32_PERF_GLOBAL_STATUS_OVF_PMC3_BIT                         3
#define IA32_PERF_GLOBAL_STATUS_OVF_PMC3_FLAG                        0x08
#define IA32_PERF_GLOBAL_STATUS_OVF_PMC3_MASK                        0x01
#define IA32_PERF_GLOBAL_STATUS_OVF_PMC3(_)                          (((_) >> 3) & 0x01)
	uint64_t reserved1                                               : 28;

	/**
	* [Bit 32] Ovf_FixedCtr0: Overflow status of IA32_FIXED_CTR0.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 1
	*/
	uint64_t ovf_fixedctr0                                           : 1;
#define IA32_PERF_GLOBAL_STATUS_OVF_FIXEDCTR0_BIT                    32
#define IA32_PERF_GLOBAL_STATUS_OVF_FIXEDCTR0_FLAG                   0x100000000
#define IA32_PERF_GLOBAL_STATUS_OVF_FIXEDCTR0_MASK                   0x01
#define IA32_PERF_GLOBAL_STATUS_OVF_FIXEDCTR0(_)                     (((_) >> 32) & 0x01)

	/**
	* [Bit 33] Ovf_FixedCtr1: Overflow status of IA32_FIXED_CTR1.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 1
	*/
	uint64_t ovf_fixedctr1                                           : 1;
#define IA32_PERF_GLOBAL_STATUS_OVF_FIXEDCTR1_BIT                    33
#define IA32_PERF_GLOBAL_STATUS_OVF_FIXEDCTR1_FLAG                   0x200000000
#define IA32_PERF_GLOBAL_STATUS_OVF_FIXEDCTR1_MASK                   0x01
#define IA32_PERF_GLOBAL_STATUS_OVF_FIXEDCTR1(_)                     (((_) >> 33) & 0x01)

	/**
	* [Bit 34] Ovf_FixedCtr2: Overflow status of IA32_FIXED_CTR2.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 1
	*/
	uint64_t ovf_fixedctr2                                           : 1;
#define IA32_PERF_GLOBAL_STATUS_OVF_FIXEDCTR2_BIT                    34
#define IA32_PERF_GLOBAL_STATUS_OVF_FIXEDCTR2_FLAG                   0x400000000
#define IA32_PERF_GLOBAL_STATUS_OVF_FIXEDCTR2_MASK                   0x01
#define IA32_PERF_GLOBAL_STATUS_OVF_FIXEDCTR2(_)                     (((_) >> 34) & 0x01)
	uint64_t reserved2                                               : 20;

	/**
	* [Bit 55] Trace_ToPA_PMI: A PMI occurred due to a ToPA entry memory buffer that was completely filled.
	*
	* @remarks If (CPUID.(EAX=07H, ECX=0):EBX[25] = 1) && IA32_RTIT_CTL.ToPA = 1
	*/
	uint64_t trace_topa_pmi                                          : 1;
#define IA32_PERF_GLOBAL_STATUS_TRACE_TOPA_PMI_BIT                   55
#define IA32_PERF_GLOBAL_STATUS_TRACE_TOPA_PMI_FLAG                  0x80000000000000
#define IA32_PERF_GLOBAL_STATUS_TRACE_TOPA_PMI_MASK                  0x01
#define IA32_PERF_GLOBAL_STATUS_TRACE_TOPA_PMI(_)                    (((_) >> 55) & 0x01)
	uint64_t reserved3                                               : 2;

	/**
	* [Bit 58] LBR_Frz. LBRs are frozen due to:
	* * IA32_DEBUGCTL.FREEZE_LBR_ON_PMI=1.
	* * The LBR stack overflowed.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 3
	*/
	uint64_t lbr_frz                                                 : 1;
#define IA32_PERF_GLOBAL_STATUS_LBR_FRZ_BIT                          58
#define IA32_PERF_GLOBAL_STATUS_LBR_FRZ_FLAG                         0x400000000000000
#define IA32_PERF_GLOBAL_STATUS_LBR_FRZ_MASK                         0x01
#define IA32_PERF_GLOBAL_STATUS_LBR_FRZ(_)                           (((_) >> 58) & 0x01)

	/**
	* [Bit 59] CTR_Frz. Performance counters in the core PMU are frozen due to:
	* * IA32_DEBUGCTL.FREEZE_PERFMON_ON_PMI=1.
	* * One or more core PMU counters overflowed.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 3
	*/
	uint64_t ctr_frz                                                 : 1;
#define IA32_PERF_GLOBAL_STATUS_CTR_FRZ_BIT                          59
#define IA32_PERF_GLOBAL_STATUS_CTR_FRZ_FLAG                         0x800000000000000
#define IA32_PERF_GLOBAL_STATUS_CTR_FRZ_MASK                         0x01
#define IA32_PERF_GLOBAL_STATUS_CTR_FRZ(_)                           (((_) >> 59) & 0x01)

	/**
	* [Bit 60] ASCI: Data in the performance counters in the core PMU may include contributions from the direct or indirect
	* operation Intel SGX to protect an enclave.
	*
	* @remarks If CPUID.(EAX=07H, ECX=0):EBX[2] = 1
	*/
	uint64_t asci                                                    : 1;
#define IA32_PERF_GLOBAL_STATUS_ASCI_BIT                             60
#define IA32_PERF_GLOBAL_STATUS_ASCI_FLAG                            0x1000000000000000
#define IA32_PERF_GLOBAL_STATUS_ASCI_MASK                            0x01
#define IA32_PERF_GLOBAL_STATUS_ASCI(_)                              (((_) >> 60) & 0x01)

	/**
	* [Bit 61] Uncore counter overflow status.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 2
	*/
	uint64_t ovf_uncore                                              : 1;
#define IA32_PERF_GLOBAL_STATUS_OVF_UNCORE_BIT                       61
#define IA32_PERF_GLOBAL_STATUS_OVF_UNCORE_FLAG                      0x2000000000000000
#define IA32_PERF_GLOBAL_STATUS_OVF_UNCORE_MASK                      0x01
#define IA32_PERF_GLOBAL_STATUS_OVF_UNCORE(_)                        (((_) >> 61) & 0x01)

	/**
	* [Bit 62] OvfBuf: DS SAVE area Buffer overflow status.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 0
	*/
	uint64_t ovf_buf                                                 : 1;
#define IA32_PERF_GLOBAL_STATUS_OVF_BUF_BIT                          62
#define IA32_PERF_GLOBAL_STATUS_OVF_BUF_FLAG                         0x4000000000000000
#define IA32_PERF_GLOBAL_STATUS_OVF_BUF_MASK                         0x01
#define IA32_PERF_GLOBAL_STATUS_OVF_BUF(_)                           (((_) >> 62) & 0x01)

	/**
	* [Bit 63] CondChgd: Status bits of this register have changed.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 0
	*/
	uint64_t cond_chgd                                               : 1;
#define IA32_PERF_GLOBAL_STATUS_COND_CHGD_BIT                        63
#define IA32_PERF_GLOBAL_STATUS_COND_CHGD_FLAG                       0x8000000000000000
#define IA32_PERF_GLOBAL_STATUS_COND_CHGD_MASK                       0x01
#define IA32_PERF_GLOBAL_STATUS_COND_CHGD(_)                         (((_) >> 63) & 0x01)
  };

  uint64_t flags;
} perf_global_status_register;


/**
 * @brief Global Performance Counter Control <b>(R/W)</b>
 *
 * Global Performance Counter Control. Counter increments while the result of ANDing the respective enable bit in this MSR
 * with the corresponding OS or USR bits in the general-purpose or fixed counter control MSR is true.
 *
 * @remarks If CPUID.0AH: EAX[7:0] > 0
 */
#define IA32_PERF_GLOBAL_CTRL                                        0x0000038F
typedef struct
{
  /**
	* EN_PMC(n). Enable bitmask. Only the first n-1 bits are valid. Bits 31:n are reserved.
	*
	* @remarks If CPUID.0AH: EAX[15:8] > n
	*/
  uint32_t en_pmcn;

  /**
	* EN_FIXED_CTR(n). Enable bitmask. Only the first n-1 bits are valid. Bits 31:n are reserved.
	*
	* @remarks If CPUID.0AH: EDX[4:0] > n
	*/
  uint32_t en_fixed_ctrn;
} perf_global_ctrl_register;


/**
 * Global Performance Counter Overflow Reset Control.
 *
 * @remarks If CPUID.0AH: EAX[7:0] > 3
 */
#define IA32_PERF_GLOBAL_STATUS_RESET                                0x00000390
typedef union
{
  struct
  {
	/**
	* [Bits 31:0] Set 1 to clear Ovf_PMC(n) bit. Clear bitmask. Only the first n-1 bits are valid. Bits 31:n are reserved.
	*
	* @remarks If CPUID.0AH: EAX[15:8] > n
	*/
	uint64_t clear_ovf_pmcn                                          : 32;
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_OVF_PMCN_BIT             0
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_OVF_PMCN_FLAG            0xFFFFFFFF
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_OVF_PMCN_MASK            0xFFFFFFFF
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_OVF_PMCN(_)              (((_) >> 0) & 0xFFFFFFFF)

	/**
	* [Bits 34:32] Set 1 to clear Ovf_FIXED_CTR(n) bit. Clear bitmask. Only the first n-1 bits are valid. Bits 31:n are
	* reserved.
	*
	* @remarks If CPUID.0AH: EDX[4:0] > n
	*/
	uint64_t clear_ovf_fixed_ctrn                                    : 3;
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_OVF_FIXED_CTRN_BIT       32
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_OVF_FIXED_CTRN_FLAG      0x700000000
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_OVF_FIXED_CTRN_MASK      0x07
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_OVF_FIXED_CTRN(_)        (((_) >> 32) & 0x07)
	uint64_t reserved1                                               : 20;

	/**
	* [Bit 55] Set 1 to clear Trace_ToPA_PMI bit.
	*
	* @remarks If (CPUID.(EAX=07H, ECX=0):EBX[25] = 1) && IA32_RTIT_CTL.ToPA = 1
	*/
	uint64_t clear_trace_topa_pmi                                    : 1;
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_TRACE_TOPA_PMI_BIT       55
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_TRACE_TOPA_PMI_FLAG      0x80000000000000
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_TRACE_TOPA_PMI_MASK      0x01
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_TRACE_TOPA_PMI(_)        (((_) >> 55) & 0x01)
	uint64_t reserved2                                               : 2;

	/**
	* [Bit 58] Set 1 to clear LBR_Frz bit.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 3
	*/
	uint64_t clear_lbr_frz                                           : 1;
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_LBR_FRZ_BIT              58
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_LBR_FRZ_FLAG             0x400000000000000
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_LBR_FRZ_MASK             0x01
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_LBR_FRZ(_)               (((_) >> 58) & 0x01)

	/**
	* [Bit 59] Set 1 to clear CTR_Frz bit.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 3
	*/
	uint64_t clear_ctr_frz                                           : 1;
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_CTR_FRZ_BIT              59
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_CTR_FRZ_FLAG             0x800000000000000
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_CTR_FRZ_MASK             0x01
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_CTR_FRZ(_)               (((_) >> 59) & 0x01)

	/**
	* [Bit 60] Set 1 to clear ASCI bit.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 3
	*/
	uint64_t clear_asci                                              : 1;
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_ASCI_BIT                 60
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_ASCI_FLAG                0x1000000000000000
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_ASCI_MASK                0x01
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_ASCI(_)                  (((_) >> 60) & 0x01)

	/**
	* [Bit 61] Set 1 to clear Ovf_Uncore bit.
	*
	* @remarks 06_2EH
	*/
	uint64_t clear_ovf_uncore                                        : 1;
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_OVF_UNCORE_BIT           61
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_OVF_UNCORE_FLAG          0x2000000000000000
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_OVF_UNCORE_MASK          0x01
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_OVF_UNCORE(_)            (((_) >> 61) & 0x01)

	/**
	* [Bit 62] Set 1 to clear OvfBuf bit.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 0
	*/
	uint64_t clear_ovf_buf                                           : 1;
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_OVF_BUF_BIT              62
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_OVF_BUF_FLAG             0x4000000000000000
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_OVF_BUF_MASK             0x01
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_OVF_BUF(_)               (((_) >> 62) & 0x01)

	/**
	* [Bit 63] Set 1 to clear CondChgd bit.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 0
	*/
	uint64_t clear_cond_chgd                                         : 1;
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_COND_CHGD_BIT            63
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_COND_CHGD_FLAG           0x8000000000000000
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_COND_CHGD_MASK           0x01
#define IA32_PERF_GLOBAL_STATUS_RESET_CLEAR_COND_CHGD(_)             (((_) >> 63) & 0x01)
  };

  uint64_t flags;
} perf_global_status_reset_register;


/**
 * Global Performance Counter Overflow Set Control.
 *
 * @remarks If CPUID.0AH: EAX[7:0] > 3
 */
#define IA32_PERF_GLOBAL_STATUS_SET                                  0x00000391
typedef union
{
  struct
  {
	/**
	* [Bits 31:0] Set 1 to cause Ovf_PMC(n) = 1. Set bitmask. Only the first n-1 bits are valid. Bits 31:n are reserved.
	*
	* @remarks If CPUID.0AH: EAX[15:8] > n
	*/
	uint64_t ovf_pmcn                                                : 32;
#define IA32_PERF_GLOBAL_STATUS_SET_OVF_PMCN_BIT                     0
#define IA32_PERF_GLOBAL_STATUS_SET_OVF_PMCN_FLAG                    0xFFFFFFFF
#define IA32_PERF_GLOBAL_STATUS_SET_OVF_PMCN_MASK                    0xFFFFFFFF
#define IA32_PERF_GLOBAL_STATUS_SET_OVF_PMCN(_)                      (((_) >> 0) & 0xFFFFFFFF)

	/**
	* [Bits 34:32] Set 1 to cause Ovf_FIXED_CTR(n) = 1. Set bitmask. Only the first n-1 bits are valid. Bits 31:n are
	* reserved.
	*
	* @remarks If CPUID.0AH: EDX[4:0] > n
	*/
	uint64_t ovf_fixed_ctrn                                          : 3;
#define IA32_PERF_GLOBAL_STATUS_SET_OVF_FIXED_CTRN_BIT               32
#define IA32_PERF_GLOBAL_STATUS_SET_OVF_FIXED_CTRN_FLAG              0x700000000
#define IA32_PERF_GLOBAL_STATUS_SET_OVF_FIXED_CTRN_MASK              0x07
#define IA32_PERF_GLOBAL_STATUS_SET_OVF_FIXED_CTRN(_)                (((_) >> 32) & 0x07)
	uint64_t reserved1                                               : 20;

	/**
	* [Bit 55] Set 1 to cause Trace_ToPA_PMI = 1.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 3
	*/
	uint64_t trace_topa_pmi                                          : 1;
#define IA32_PERF_GLOBAL_STATUS_SET_TRACE_TOPA_PMI_BIT               55
#define IA32_PERF_GLOBAL_STATUS_SET_TRACE_TOPA_PMI_FLAG              0x80000000000000
#define IA32_PERF_GLOBAL_STATUS_SET_TRACE_TOPA_PMI_MASK              0x01
#define IA32_PERF_GLOBAL_STATUS_SET_TRACE_TOPA_PMI(_)                (((_) >> 55) & 0x01)
	uint64_t reserved2                                               : 2;

	/**
	* [Bit 58] Set 1 to cause LBR_Frz = 1.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 3
	*/
	uint64_t lbr_frz                                                 : 1;
#define IA32_PERF_GLOBAL_STATUS_SET_LBR_FRZ_BIT                      58
#define IA32_PERF_GLOBAL_STATUS_SET_LBR_FRZ_FLAG                     0x400000000000000
#define IA32_PERF_GLOBAL_STATUS_SET_LBR_FRZ_MASK                     0x01
#define IA32_PERF_GLOBAL_STATUS_SET_LBR_FRZ(_)                       (((_) >> 58) & 0x01)

	/**
	* [Bit 59] Set 1 to cause CTR_Frz = 1.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 3
	*/
	uint64_t ctr_frz                                                 : 1;
#define IA32_PERF_GLOBAL_STATUS_SET_CTR_FRZ_BIT                      59
#define IA32_PERF_GLOBAL_STATUS_SET_CTR_FRZ_FLAG                     0x800000000000000
#define IA32_PERF_GLOBAL_STATUS_SET_CTR_FRZ_MASK                     0x01
#define IA32_PERF_GLOBAL_STATUS_SET_CTR_FRZ(_)                       (((_) >> 59) & 0x01)

	/**
	* [Bit 60] Set 1 to cause ASCI = 1.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 3
	*/
	uint64_t asci                                                    : 1;
#define IA32_PERF_GLOBAL_STATUS_SET_ASCI_BIT                         60
#define IA32_PERF_GLOBAL_STATUS_SET_ASCI_FLAG                        0x1000000000000000
#define IA32_PERF_GLOBAL_STATUS_SET_ASCI_MASK                        0x01
#define IA32_PERF_GLOBAL_STATUS_SET_ASCI(_)                          (((_) >> 60) & 0x01)

	/**
	* [Bit 61] Set 1 to cause Ovf_Uncore = 1.
	*
	* @remarks 06_2EH
	*/
	uint64_t ovf_uncore                                              : 1;
#define IA32_PERF_GLOBAL_STATUS_SET_OVF_UNCORE_BIT                   61
#define IA32_PERF_GLOBAL_STATUS_SET_OVF_UNCORE_FLAG                  0x2000000000000000
#define IA32_PERF_GLOBAL_STATUS_SET_OVF_UNCORE_MASK                  0x01
#define IA32_PERF_GLOBAL_STATUS_SET_OVF_UNCORE(_)                    (((_) >> 61) & 0x01)

	/**
	* [Bit 62] Set 1 to cause OvfBuf = 1.
	*
	* @remarks If CPUID.0AH: EAX[7:0] > 3
	*/
	uint64_t ovf_buf                                                 : 1;
#define IA32_PERF_GLOBAL_STATUS_SET_OVF_BUF_BIT                      62
#define IA32_PERF_GLOBAL_STATUS_SET_OVF_BUF_FLAG                     0x4000000000000000
#define IA32_PERF_GLOBAL_STATUS_SET_OVF_BUF_MASK                     0x01
#define IA32_PERF_GLOBAL_STATUS_SET_OVF_BUF(_)                       (((_) >> 62) & 0x01)
	uint64_t reserved3                                               : 1;
  };

  uint64_t flags;
} perf_global_status_set_register;


/**
 * Indicator that core perfmon interface is in use.
 *
 * @remarks If CPUID.0AH: EAX[7:0] > 3
 */
#define IA32_PERF_GLOBAL_INUSE                                       0x00000392
typedef union
{
  struct
  {
	/**
	* [Bits 31:0] IA32_PERFEVTSEL(n) in use. Status bitmask. Only the first n-1 bits are valid. Bits 31:n are reserved.
	*
	* @remarks If CPUID.0AH: EAX[15:8] > n
	*/
	uint64_t ia32_perfevtseln_in_use                                 : 32;
#define IA32_PERF_GLOBAL_INUSE_IA32_PERFEVTSELN_IN_USE_BIT           0
#define IA32_PERF_GLOBAL_INUSE_IA32_PERFEVTSELN_IN_USE_FLAG          0xFFFFFFFF
#define IA32_PERF_GLOBAL_INUSE_IA32_PERFEVTSELN_IN_USE_MASK          0xFFFFFFFF
#define IA32_PERF_GLOBAL_INUSE_IA32_PERFEVTSELN_IN_USE(_)            (((_) >> 0) & 0xFFFFFFFF)

	/**
	* [Bits 34:32] IA32_FIXED_CTR(n) in use. Status bitmask. Only the first n-1 bits are valid. Bits 31:n are reserved.
	*/
	uint64_t ia32_fixed_ctrn_in_use                                  : 3;
#define IA32_PERF_GLOBAL_INUSE_IA32_FIXED_CTRN_IN_USE_BIT            32
#define IA32_PERF_GLOBAL_INUSE_IA32_FIXED_CTRN_IN_USE_FLAG           0x700000000
#define IA32_PERF_GLOBAL_INUSE_IA32_FIXED_CTRN_IN_USE_MASK           0x07
#define IA32_PERF_GLOBAL_INUSE_IA32_FIXED_CTRN_IN_USE(_)             (((_) >> 32) & 0x07)
	uint64_t reserved1                                               : 28;

	/**
	* [Bit 63] PMI in use.
	*/
	uint64_t pmi_in_use                                              : 1;
#define IA32_PERF_GLOBAL_INUSE_PMI_IN_USE_BIT                        63
#define IA32_PERF_GLOBAL_INUSE_PMI_IN_USE_FLAG                       0x8000000000000000
#define IA32_PERF_GLOBAL_INUSE_PMI_IN_USE_MASK                       0x01
#define IA32_PERF_GLOBAL_INUSE_PMI_IN_USE(_)                         (((_) >> 63) & 0x01)
  };

  uint64_t flags;
} perf_global_inuse_register;


/**
 * PEBS Control.
 *
 * @remarks If CPUID.0AH: EAX[7:0] > 3
 */
#define IA32_PEBS_ENABLE                                             0x000003F1
typedef union
{
  struct
  {
	/**
	* [Bit 0] Enable PEBS on IA32_PMC0.
	*
	* @remarks 06_0FH
	*/
	uint64_t enable_pebs                                             : 1;
#define IA32_PEBS_ENABLE_ENABLE_PEBS_BIT                             0
#define IA32_PEBS_ENABLE_ENABLE_PEBS_FLAG                            0x01
#define IA32_PEBS_ENABLE_ENABLE_PEBS_MASK                            0x01
#define IA32_PEBS_ENABLE_ENABLE_PEBS(_)                              (((_) >> 0) & 0x01)

	/**
	* [Bits 3:1] Reserved or model specific.
	*/
	uint64_t reservedormodelspecific1                                : 3;
#define IA32_PEBS_ENABLE_RESERVEDORMODELSPECIFIC1_BIT                1
#define IA32_PEBS_ENABLE_RESERVEDORMODELSPECIFIC1_FLAG               0x0E
#define IA32_PEBS_ENABLE_RESERVEDORMODELSPECIFIC1_MASK               0x07
#define IA32_PEBS_ENABLE_RESERVEDORMODELSPECIFIC1(_)                 (((_) >> 1) & 0x07)
	uint64_t reserved1                                               : 28;

	/**
	* [Bits 35:32] Reserved or model specific.
	*/
	uint64_t reservedormodelspecific2                                : 4;
#define IA32_PEBS_ENABLE_RESERVEDORMODELSPECIFIC2_BIT                32
#define IA32_PEBS_ENABLE_RESERVEDORMODELSPECIFIC2_FLAG               0xF00000000
#define IA32_PEBS_ENABLE_RESERVEDORMODELSPECIFIC2_MASK               0x0F
#define IA32_PEBS_ENABLE_RESERVEDORMODELSPECIFIC2(_)                 (((_) >> 32) & 0x0F)
	uint64_t reserved2                                               : 28;
  };

  uint64_t flags;
} pebs_enable_register;

/**
 * @defgroup ia32_mc_ctl \
 *           IA32_MC(i)_CTL
 *
 * IA32_MC(0-28)_CTL.
 *
 * @remarks If IA32_MCG_CAP.CNT > n
 * @{
 */
#define IA32_MC0_CTL                                                 0x00000400
#define IA32_MC1_CTL                                                 0x00000404
#define IA32_MC2_CTL                                                 0x00000408
#define IA32_MC3_CTL                                                 0x0000040C
#define IA32_MC4_CTL                                                 0x00000410
#define IA32_MC5_CTL                                                 0x00000414
#define IA32_MC6_CTL                                                 0x00000418
#define IA32_MC7_CTL                                                 0x0000041C
#define IA32_MC8_CTL                                                 0x00000420
#define IA32_MC9_CTL                                                 0x00000424
#define IA32_MC10_CTL                                                0x00000428
#define IA32_MC11_CTL                                                0x0000042C
#define IA32_MC12_CTL                                                0x00000430
#define IA32_MC13_CTL                                                0x00000434
#define IA32_MC14_CTL                                                0x00000438
#define IA32_MC15_CTL                                                0x0000043C
#define IA32_MC16_CTL                                                0x00000440
#define IA32_MC17_CTL                                                0x00000444
#define IA32_MC18_CTL                                                0x00000448
#define IA32_MC19_CTL                                                0x0000044C
#define IA32_MC20_CTL                                                0x00000450
#define IA32_MC21_CTL                                                0x00000454
#define IA32_MC22_CTL                                                0x00000458
#define IA32_MC23_CTL                                                0x0000045C
#define IA32_MC24_CTL                                                0x00000460
#define IA32_MC25_CTL                                                0x00000464
#define IA32_MC26_CTL                                                0x00000468
#define IA32_MC27_CTL                                                0x0000046C
#define IA32_MC28_CTL                                                0x00000470
/**
 * @}
 */

/**
 * @defgroup ia32_mc_status \
 *           IA32_MC(i)_STATUS
 *
 * IA32_MC(0-28)_STATUS.
 *
 * @remarks If IA32_MCG_CAP.CNT > n
 * @{
 */
#define IA32_MC0_STATUS                                              0x00000401
#define IA32_MC1_STATUS                                              0x00000405
#define IA32_MC2_STATUS                                              0x00000409
#define IA32_MC3_STATUS                                              0x0000040D
#define IA32_MC4_STATUS                                              0x00000411
#define IA32_MC5_STATUS                                              0x00000415
#define IA32_MC6_STATUS                                              0x00000419
#define IA32_MC7_STATUS                                              0x0000041D
#define IA32_MC8_STATUS                                              0x00000421
#define IA32_MC9_STATUS                                              0x00000425
#define IA32_MC10_STATUS                                             0x00000429
#define IA32_MC11_STATUS                                             0x0000042D
#define IA32_MC12_STATUS                                             0x00000431
#define IA32_MC13_STATUS                                             0x00000435
#define IA32_MC14_STATUS                                             0x00000439
#define IA32_MC15_STATUS                                             0x0000043D
#define IA32_MC16_STATUS                                             0x00000441
#define IA32_MC17_STATUS                                             0x00000445
#define IA32_MC18_STATUS                                             0x00000449
#define IA32_MC19_STATUS                                             0x0000044D
#define IA32_MC20_STATUS                                             0x00000451
#define IA32_MC21_STATUS                                             0x00000455
#define IA32_MC22_STATUS                                             0x00000459
#define IA32_MC23_STATUS                                             0x0000045D
#define IA32_MC24_STATUS                                             0x00000461
#define IA32_MC25_STATUS                                             0x00000465
#define IA32_MC26_STATUS                                             0x00000469
#define IA32_MC27_STATUS                                             0x0000046D
#define IA32_MC28_STATUS                                             0x00000471
/**
 * @}
 */

/**
 * @defgroup ia32_mc_addr \
 *           IA32_MC(i)_ADDR
 *
 * IA32_MC(0-28)_ADDR.
 *
 * @remarks If IA32_MCG_CAP.CNT > n
 * @{
 */
#define IA32_MC0_ADDR                                                0x00000402
#define IA32_MC1_ADDR                                                0x00000406
#define IA32_MC2_ADDR                                                0x0000040A
#define IA32_MC3_ADDR                                                0x0000040E
#define IA32_MC4_ADDR                                                0x00000412
#define IA32_MC5_ADDR                                                0x00000416
#define IA32_MC6_ADDR                                                0x0000041A
#define IA32_MC7_ADDR                                                0x0000041E
#define IA32_MC8_ADDR                                                0x00000422
#define IA32_MC9_ADDR                                                0x00000426
#define IA32_MC10_ADDR                                               0x0000042A
#define IA32_MC11_ADDR                                               0x0000042E
#define IA32_MC12_ADDR                                               0x00000432
#define IA32_MC13_ADDR                                               0x00000436
#define IA32_MC14_ADDR                                               0x0000043A
#define IA32_MC15_ADDR                                               0x0000043E
#define IA32_MC16_ADDR                                               0x00000442
#define IA32_MC17_ADDR                                               0x00000446
#define IA32_MC18_ADDR                                               0x0000044A
#define IA32_MC19_ADDR                                               0x0000044E
#define IA32_MC20_ADDR                                               0x00000452
#define IA32_MC21_ADDR                                               0x00000456
#define IA32_MC22_ADDR                                               0x0000045A
#define IA32_MC23_ADDR                                               0x0000045E
#define IA32_MC24_ADDR                                               0x00000462
#define IA32_MC25_ADDR                                               0x00000466
#define IA32_MC26_ADDR                                               0x0000046A
#define IA32_MC27_ADDR                                               0x0000046E
#define IA32_MC28_ADDR                                               0x00000472
/**
 * @}
 */

/**
 * @defgroup ia32_mc_misc \
 *           IA32_MC(i)_MISC
 *
 * IA32_MC(0-28)_MISC.
 *
 * @remarks If IA32_MCG_CAP.CNT > n
 * @{
 */
#define IA32_MC0_MISC                                                0x00000403
#define IA32_MC1_MISC                                                0x00000407
#define IA32_MC2_MISC                                                0x0000040B
#define IA32_MC3_MISC                                                0x0000040F
#define IA32_MC4_MISC                                                0x00000413
#define IA32_MC5_MISC                                                0x00000417
#define IA32_MC6_MISC                                                0x0000041B
#define IA32_MC7_MISC                                                0x0000041F
#define IA32_MC8_MISC                                                0x00000423
#define IA32_MC9_MISC                                                0x00000427
#define IA32_MC10_MISC                                               0x0000042B
#define IA32_MC11_MISC                                               0x0000042F
#define IA32_MC12_MISC                                               0x00000433
#define IA32_MC13_MISC                                               0x00000437
#define IA32_MC14_MISC                                               0x0000043B
#define IA32_MC15_MISC                                               0x0000043F
#define IA32_MC16_MISC                                               0x00000443
#define IA32_MC17_MISC                                               0x00000447
#define IA32_MC18_MISC                                               0x0000044B
#define IA32_MC19_MISC                                               0x0000044F
#define IA32_MC20_MISC                                               0x00000453
#define IA32_MC21_MISC                                               0x00000457
#define IA32_MC22_MISC                                               0x0000045B
#define IA32_MC23_MISC                                               0x0000045F
#define IA32_MC24_MISC                                               0x00000463
#define IA32_MC25_MISC                                               0x00000467
#define IA32_MC26_MISC                                               0x0000046B
#define IA32_MC27_MISC                                               0x0000046F
#define IA32_MC28_MISC                                               0x00000473
/**
 * @}
 */


/**
 * Reporting Register of Basic VMX Capabilities.
 *
 * @remarks If CPUID.01H:ECX.[5] = 1
 * @see Vol3D[A.1(BASIC VMX INFORMATION)]
 * @see Vol3D[A.1(Basic VMX Information)] (reference)
 */
#define IA32_VMX_BASIC                                               0x00000480
typedef union
{
  struct
  {
	/**
	* @brief VMCS revision identifier used by the processor
	*
	* [Bits 30:0] 31-bit VMCS revision identifier used by the processor. Processors that use the same VMCS revision identifier
	* use the same size for VMCS regions.
	*/
	uint64_t vmcs_revision_id                                        : 31;
#define IA32_VMX_BASIC_VMCS_REVISION_ID_BIT                          0
#define IA32_VMX_BASIC_VMCS_REVISION_ID_FLAG                         0x7FFFFFFF
#define IA32_VMX_BASIC_VMCS_REVISION_ID_MASK                         0x7FFFFFFF
#define IA32_VMX_BASIC_VMCS_REVISION_ID(_)                           (((_) >> 0) & 0x7FFFFFFF)

	/**
	* [Bit 31] Bit 31 is always 0.
	*/
	uint64_t must_be_zero                                            : 1;
#define IA32_VMX_BASIC_MUST_BE_ZERO_BIT                              31
#define IA32_VMX_BASIC_MUST_BE_ZERO_FLAG                             0x80000000
#define IA32_VMX_BASIC_MUST_BE_ZERO_MASK                             0x01
#define IA32_VMX_BASIC_MUST_BE_ZERO(_)                               (((_) >> 31) & 0x01)

	/**
	* @brief Size of the VMCS
	*
	* [Bits 44:32] Report the number of bytes that software should allocate for the VMXON region and any VMCS region. It is a
	* value greater than 0 and at most 4096 (bit 44 is set if and only if bits 43:32 are clear).
	*/
	uint64_t vmcs_size_in_bytes                                      : 13;
#define IA32_VMX_BASIC_VMCS_SIZE_IN_BYTES_BIT                        32
#define IA32_VMX_BASIC_VMCS_SIZE_IN_BYTES_FLAG                       0x1FFF00000000
#define IA32_VMX_BASIC_VMCS_SIZE_IN_BYTES_MASK                       0x1FFF
#define IA32_VMX_BASIC_VMCS_SIZE_IN_BYTES(_)                         (((_) >> 32) & 0x1FFF)
	uint64_t reserved1                                               : 3;

	/**
	* @brief Width of physical address used for the VMCS
	*        - 0 -> limited to the available amount of physical RAM
	*        - 1 -> within the first 4 GB
	*
	* [Bit 48] Indicates the width of the physical addresses that may be used for the VMXON region, each VMCS, and data
	* structures referenced by pointers in a VMCS (I/O bitmaps, virtual-APIC page, MSR areas for VMX transitions). If the bit
	* is 0, these addresses are limited to the processor's physical-address width.2 If the bit is 1, these addresses are
	* limited to 32 bits. This bit is always 0 for processors that support Intel 64 architecture.
	*/
	uint64_t vmcs_physical_address_width                             : 1;
#define IA32_VMX_BASIC_VMCS_PHYSICAL_ADDRESS_WIDTH_BIT               48
#define IA32_VMX_BASIC_VMCS_PHYSICAL_ADDRESS_WIDTH_FLAG              0x1000000000000
#define IA32_VMX_BASIC_VMCS_PHYSICAL_ADDRESS_WIDTH_MASK              0x01
#define IA32_VMX_BASIC_VMCS_PHYSICAL_ADDRESS_WIDTH(_)                (((_) >> 48) & 0x01)

	/**
	* @brief Whether the processor supports the dual-monitor treatment of system-management interrupts and system-management
	*        code (always 1)
	*
	* [Bit 49] Read as 1, the logical processor supports the dual-monitor treatment of system-management interrupts and
	* system-management mode.
	*
	* @see Vol3C[34.15(DUAL-MONITOR TREATMENT OF SMIs AND SMM)]
	*/
	uint64_t dual_monitor_support                                    : 1;
#define IA32_VMX_BASIC_DUAL_MONITOR_SUPPORT_BIT                      49
#define IA32_VMX_BASIC_DUAL_MONITOR_SUPPORT_FLAG                     0x2000000000000
#define IA32_VMX_BASIC_DUAL_MONITOR_SUPPORT_MASK                     0x01
#define IA32_VMX_BASIC_DUAL_MONITOR_SUPPORT(_)                       (((_) >> 49) & 0x01)

	/**
	* @brief Memory type that must be used for the VMCS
	*
	* [Bits 53:50] Report the memory type that should be used for the VMCS, for data structures referenced by pointers in the
	* VMCS (I/O bitmaps, virtual-APIC page, MSR areas for VMX transitions), and for the MSEG header. If software needs to
	* access these data structures (e.g., to modify the contents of the MSR bitmaps), it can configure the paging structures
	* to map them into the linear-address space. If it does so, it should establish mappings that use the memory type reported
	* bits 53:50 in this MSR.
	* As of this writing, all processors that support VMX operation indicate the write-back type.
	*/
	uint64_t memory_type                                             : 4;
#define IA32_VMX_BASIC_MEMORY_TYPE_BIT                               50
#define IA32_VMX_BASIC_MEMORY_TYPE_FLAG                              0x3C000000000000
#define IA32_VMX_BASIC_MEMORY_TYPE_MASK                              0x0F
#define IA32_VMX_BASIC_MEMORY_TYPE(_)                                (((_) >> 50) & 0x0F)

	/**
	* @brief Whether the processor provides additional information for exits due to INS/OUTS
	*
	* [Bit 54] When set to 1, the processor reports information in the VM-exit instruction-information field on VM exits due
	* to execution of the INS and OUTS instructions. This reporting is done only if this bit is read as 1.
	*
	* @see Vol3C[27.2.4(Information for VM Exits Due to Instruction Execution)]
	*/
	uint64_t ins_outs_reporting                                      : 1;
#define IA32_VMX_BASIC_INS_OUTS_REPORTING_BIT                        54
#define IA32_VMX_BASIC_INS_OUTS_REPORTING_FLAG                       0x40000000000000
#define IA32_VMX_BASIC_INS_OUTS_REPORTING_MASK                       0x01
#define IA32_VMX_BASIC_INS_OUTS_REPORTING(_)                         (((_) >> 54) & 0x01)

	/**
	* @brief Whether default 1 bits in control MSRs (pin/proc/exit/entry) may be cleared to 0 and that 'true' control MSRs are
	*        supported
	*
	* [Bit 55] Is read as 1 if any VMX controls that default to 1 may be cleared to 0. It also reports support for the VMX
	* capability MSRs IA32_VMX_TRUE_PINBASED_CTLS, IA32_VMX_TRUE_PROCBASED_CTLS, IA32_VMX_TRUE_EXIT_CTLS, and
	* IA32_VMX_TRUE_ENTRY_CTLS.
	*
	* @see Vol3D[A.2(RESERVED CONTROLS AND DEFAULT SETTINGS)]
	* @see Vol3D[A.3.1(Pin-Based VM-Execution Controls)]
	* @see Vol3D[A.3.2(Primary Processor-Based VM-Execution Controls)]
	* @see Vol3D[A.4(VM-EXIT CONTROLS)]
	* @see Vol3D[A.5(VM-ENTRY CONTROLS)]
	*/
	uint64_t vmx_controls                                            : 1;
#define IA32_VMX_BASIC_VMX_CONTROLS_BIT                              55
#define IA32_VMX_BASIC_VMX_CONTROLS_FLAG                             0x80000000000000
#define IA32_VMX_BASIC_VMX_CONTROLS_MASK                             0x01
#define IA32_VMX_BASIC_VMX_CONTROLS(_)                               (((_) >> 55) & 0x01)
	uint64_t reserved2                                               : 8;
  };

  uint64_t flags;
} vmx_basic_register;


/**
 * Capability Reporting Register of Pin-Based VM-Execution Controls.
 *
 * @remarks If CPUID.01H:ECX.[5] = 1
 * @see Vol3D[A.3.1(Pin-Based VM-Execution Controls)]
 * @see Vol3C[24.6.1(Pin-Based VM-Execution Controls)] (reference)
 */
#define IA32_VMX_PINBASED_CTLS                                       0x00000481
typedef union
{
  struct
  {
	/**
	* @brief External interrupts cause VM-exits if set; otherwise dispatched through the guest's IDT
	*
	* [Bit 0] If this control is 1, external interrupts cause VM exits. Otherwise, they are delivered normally through the
	* guest interrupt-descriptor table (IDT). If this control is 1, the value of RFLAGS.IF does not affect interrupt blocking.
	*/
	uint64_t external_interrupt_exiting                              : 1;
#define IA32_VMX_PINBASED_CTLS_EXTERNAL_INTERRUPT_EXITING_BIT        0
#define IA32_VMX_PINBASED_CTLS_EXTERNAL_INTERRUPT_EXITING_FLAG       0x01
#define IA32_VMX_PINBASED_CTLS_EXTERNAL_INTERRUPT_EXITING_MASK       0x01
#define IA32_VMX_PINBASED_CTLS_EXTERNAL_INTERRUPT_EXITING(_)         (((_) >> 0) & 0x01)
	uint64_t reserved1                                               : 2;

	/**
	* @brief Non-maskable interrupts cause VM-exits if set; otherwise dispatched through the guest's IDT
	*
	* [Bit 3] If this control is 1, non-maskable interrupts (NMIs) cause VM exits. Otherwise, they are delivered normally
	* using descriptor 2 of the IDT. This control also determines interactions between IRET and blocking by NMI.
	*
	* @see Vol3C[25.3(CHANGES TO INSTRUCTION BEHAVIOR IN VMX NON-ROOT OPERATION)]
	*/
	uint64_t nmi_exiting                                             : 1;
#define IA32_VMX_PINBASED_CTLS_NMI_EXITING_BIT                       3
#define IA32_VMX_PINBASED_CTLS_NMI_EXITING_FLAG                      0x08
#define IA32_VMX_PINBASED_CTLS_NMI_EXITING_MASK                      0x01
#define IA32_VMX_PINBASED_CTLS_NMI_EXITING(_)                        (((_) >> 3) & 0x01)
	uint64_t reserved2                                               : 1;

	/**
	* @brief Virtual NMIs
	*
	* [Bit 5] If this control is 1, NMIs are never blocked and the "blocking by NMI" bit (bit 3) in the interruptibility-state
	* field indicates "virtual-NMI blocking". This control also interacts with the "NMI-window exiting" VM-execution control.
	*
	* @see Vol3C[24.6.2(Processor-Based VM-Execution Controls)]
	*/
	uint64_t virtual_nmi                                             : 1;
#define IA32_VMX_PINBASED_CTLS_VIRTUAL_NMI_BIT                       5
#define IA32_VMX_PINBASED_CTLS_VIRTUAL_NMI_FLAG                      0x20
#define IA32_VMX_PINBASED_CTLS_VIRTUAL_NMI_MASK                      0x01
#define IA32_VMX_PINBASED_CTLS_VIRTUAL_NMI(_)                        (((_) >> 5) & 0x01)

	/**
	* @brief Activate VMX preemption timer
	*
	* [Bit 6] If this control is 1, the VMX-preemption timer counts down in VMX non-root operation. A VM exit occurs when the
	* timer counts down to zero.
	*
	* @see Vol3C[25.5.1(VMX-Preemption Timer)]
	* @see Vol3C[25.2(OTHER CAUSES OF VM EXITS)]
	*/
	uint64_t activate_vmx_preemption_timer                           : 1;
#define IA32_VMX_PINBASED_CTLS_ACTIVATE_VMX_PREEMPTION_TIMER_BIT     6
#define IA32_VMX_PINBASED_CTLS_ACTIVATE_VMX_PREEMPTION_TIMER_FLAG    0x40
#define IA32_VMX_PINBASED_CTLS_ACTIVATE_VMX_PREEMPTION_TIMER_MASK    0x01
#define IA32_VMX_PINBASED_CTLS_ACTIVATE_VMX_PREEMPTION_TIMER(_)      (((_) >> 6) & 0x01)

	/**
	* @brief Process interrupts with the posted-interrupt notification vector
	*
	* [Bit 7] If this control is 1, the processor treats interrupts with the posted-interrupt notification vector specially,
	* updating the virtual-APIC page with posted-interrupt requests.
	*
	* @see Vol3C[24.6.8(Controls for APIC Virtualization)]
	* @see Vol3C[29.6(POSTED-INTERRUPT PROCESSING)]
	*/
	uint64_t process_posted_interrupts                               : 1;
#define IA32_VMX_PINBASED_CTLS_PROCESS_POSTED_INTERRUPTS_BIT         7
#define IA32_VMX_PINBASED_CTLS_PROCESS_POSTED_INTERRUPTS_FLAG        0x80
#define IA32_VMX_PINBASED_CTLS_PROCESS_POSTED_INTERRUPTS_MASK        0x01
#define IA32_VMX_PINBASED_CTLS_PROCESS_POSTED_INTERRUPTS(_)          (((_) >> 7) & 0x01)
	uint64_t reserved3                                               : 56;
  };

  uint64_t flags;
} vmx_pinbased_ctls_register;


/**
 * Capability Reporting Register of Primary Processor-Based VM-Execution Controls.
 *
 * @remarks If CPUID.01H:ECX.[5] = 1
 * @see Vol3D[A.3.2(Primary Processor-Based VM-Execution Controls)]
 * @see Vol3C[24.6.2(Processor-Based VM-Execution Controls)] (reference)
 */
#define IA32_VMX_PROCBASED_CTLS                                      0x00000482
typedef union
{
  struct
  {
	uint64_t reserved1                                               : 2;

	/**
	* @brief VM-exit as soon as RFLAGS.IF=1 and no blocking is active
	*
	* [Bit 2] If this control is 1, a VM exit occurs at the beginning of any instruction if RFLAGS.IF = 1 and there are no
	* other blocking of interrupts.
	*
	* @see Vol3C[24.4.2(Guest Non-Register State)]
	*/
	uint64_t interrupt_window_exiting                                : 1;
#define IA32_VMX_PROCBASED_CTLS_INTERRUPT_WINDOW_EXITING_BIT         2
#define IA32_VMX_PROCBASED_CTLS_INTERRUPT_WINDOW_EXITING_FLAG        0x04
#define IA32_VMX_PROCBASED_CTLS_INTERRUPT_WINDOW_EXITING_MASK        0x01
#define IA32_VMX_PROCBASED_CTLS_INTERRUPT_WINDOW_EXITING(_)          (((_) >> 2) & 0x01)

	/**
	* @brief Use timestamp counter offset
	*
	* [Bit 3] This control determines whether executions of RDTSC, executions of RDTSCP, and executions of RDMSR that read
	* from the IA32_TIME_STAMP_COUNTER MSR return a value modified by the TSC offset field.
	*
	* @see Vol3C[24.6.5(Time-Stamp Counter Offset and Multiplier)]
	* @see Vol3C[25.3(CHANGES TO INSTRUCTION BEHAVIOR IN VMX NON-ROOT OPERATION)]
	*/
	uint64_t use_tsc_offsetting                                      : 1;
#define IA32_VMX_PROCBASED_CTLS_USE_TSC_OFFSETTING_BIT               3
#define IA32_VMX_PROCBASED_CTLS_USE_TSC_OFFSETTING_FLAG              0x08
#define IA32_VMX_PROCBASED_CTLS_USE_TSC_OFFSETTING_MASK              0x01
#define IA32_VMX_PROCBASED_CTLS_USE_TSC_OFFSETTING(_)                (((_) >> 3) & 0x01)
	uint64_t reserved2                                               : 3;

	/**
	* @brief VM-exit when executing the HLT instruction
	*
	* [Bit 7] This control determines whether executions of HLT cause VM exits.
	*/
	uint64_t hlt_exiting                                             : 1;
#define IA32_VMX_PROCBASED_CTLS_HLT_EXITING_BIT                      7
#define IA32_VMX_PROCBASED_CTLS_HLT_EXITING_FLAG                     0x80
#define IA32_VMX_PROCBASED_CTLS_HLT_EXITING_MASK                     0x01
#define IA32_VMX_PROCBASED_CTLS_HLT_EXITING(_)                       (((_) >> 7) & 0x01)
	uint64_t reserved3                                               : 1;

	/**
	* @brief VM-exit when executing the INVLPG instruction
	*
	* [Bit 9] This control determines whether executions of INVLPG cause VM exits.
	*/
	uint64_t invlpg_exiting                                          : 1;
#define IA32_VMX_PROCBASED_CTLS_INVLPG_EXITING_BIT                   9
#define IA32_VMX_PROCBASED_CTLS_INVLPG_EXITING_FLAG                  0x200
#define IA32_VMX_PROCBASED_CTLS_INVLPG_EXITING_MASK                  0x01
#define IA32_VMX_PROCBASED_CTLS_INVLPG_EXITING(_)                    (((_) >> 9) & 0x01)

	/**
	* @brief VM-exit when executing the MWAIT instruction
	*
	* [Bit 10] This control determines whether executions of MWAIT cause VM exits.
	*/
	uint64_t mwait_exiting                                           : 1;
#define IA32_VMX_PROCBASED_CTLS_MWAIT_EXITING_BIT                    10
#define IA32_VMX_PROCBASED_CTLS_MWAIT_EXITING_FLAG                   0x400
#define IA32_VMX_PROCBASED_CTLS_MWAIT_EXITING_MASK                   0x01
#define IA32_VMX_PROCBASED_CTLS_MWAIT_EXITING(_)                     (((_) >> 10) & 0x01)

	/**
	* @brief VM-exit when executing the RDPMC instruction
	*
	* [Bit 11] This control determines whether executions of RDPMC cause VM exits.
	*/
	uint64_t rdpmc_exiting                                           : 1;
#define IA32_VMX_PROCBASED_CTLS_RDPMC_EXITING_BIT                    11
#define IA32_VMX_PROCBASED_CTLS_RDPMC_EXITING_FLAG                   0x800
#define IA32_VMX_PROCBASED_CTLS_RDPMC_EXITING_MASK                   0x01
#define IA32_VMX_PROCBASED_CTLS_RDPMC_EXITING(_)                     (((_) >> 11) & 0x01)

	/**
	* @brief VM-exit when executing the RDTSC/RDTSCP instruction
	*
	* [Bit 12] This control determines whether executions of RDTSC and RDTSCP cause VM exits.
	*/
	uint64_t rdtsc_exiting                                           : 1;
#define IA32_VMX_PROCBASED_CTLS_RDTSC_EXITING_BIT                    12
#define IA32_VMX_PROCBASED_CTLS_RDTSC_EXITING_FLAG                   0x1000
#define IA32_VMX_PROCBASED_CTLS_RDTSC_EXITING_MASK                   0x01
#define IA32_VMX_PROCBASED_CTLS_RDTSC_EXITING(_)                     (((_) >> 12) & 0x01)
	uint64_t reserved4                                               : 2;

	/**
	* @brief VM-exit when executing the MOV to CR3 instruction (forced to 1 on the 'first' VT-x capable CPUs; this actually
	*        includes the newest Nehalem CPUs)
	*
	* [Bit 15] In conjunction with the CR3-target controls, this control determines whether executions of MOV to CR3 cause VM
	* exits. The first processors to support the virtual-machine extensions supported only the 1-setting of this control.
	*
	* @see Vol3C[24.6.7(CR3-Target Controls)]
	* @see Vol3C[25.1.3(Instructions That Cause VM Exits Conditionally)]
	*/
	uint64_t cr3_load_exiting                                        : 1;
#define IA32_VMX_PROCBASED_CTLS_CR3_LOAD_EXITING_BIT                 15
#define IA32_VMX_PROCBASED_CTLS_CR3_LOAD_EXITING_FLAG                0x8000
#define IA32_VMX_PROCBASED_CTLS_CR3_LOAD_EXITING_MASK                0x01
#define IA32_VMX_PROCBASED_CTLS_CR3_LOAD_EXITING(_)                  (((_) >> 15) & 0x01)

	/**
	* @brief VM-exit when executing the MOV from CR3 instruction (forced to 1 on the 'first' VT-x capable CPUs; this actually
	*        includes the newest Nehalem CPUs)
	*
	* [Bit 16] This control determines whether executions of MOV from CR3 cause VM exits. The first processors to support the
	* virtual-machine extensions supported only the 1-setting of this control.
	*/
	uint64_t cr3_store_exiting                                       : 1;
#define IA32_VMX_PROCBASED_CTLS_CR3_STORE_EXITING_BIT                16
#define IA32_VMX_PROCBASED_CTLS_CR3_STORE_EXITING_FLAG               0x10000
#define IA32_VMX_PROCBASED_CTLS_CR3_STORE_EXITING_MASK               0x01
#define IA32_VMX_PROCBASED_CTLS_CR3_STORE_EXITING(_)                 (((_) >> 16) & 0x01)
	uint64_t reserved5                                               : 2;

	/**
	* @brief VM-exit on CR8 loads
	*
	* [Bit 19] This control determines whether executions of MOV to CR8 cause VM exits.
	*/
	uint64_t cr8_load_exiting                                        : 1;
#define IA32_VMX_PROCBASED_CTLS_CR8_LOAD_EXITING_BIT                 19
#define IA32_VMX_PROCBASED_CTLS_CR8_LOAD_EXITING_FLAG                0x80000
#define IA32_VMX_PROCBASED_CTLS_CR8_LOAD_EXITING_MASK                0x01
#define IA32_VMX_PROCBASED_CTLS_CR8_LOAD_EXITING(_)                  (((_) >> 19) & 0x01)

	/**
	* @brief VM-exit on CR8 stores
	*
	* [Bit 20] This control determines whether executions of MOV from CR8 cause VM exits.
	*/
	uint64_t cr8_store_exiting                                       : 1;
#define IA32_VMX_PROCBASED_CTLS_CR8_STORE_EXITING_BIT                20
#define IA32_VMX_PROCBASED_CTLS_CR8_STORE_EXITING_FLAG               0x100000
#define IA32_VMX_PROCBASED_CTLS_CR8_STORE_EXITING_MASK               0x01
#define IA32_VMX_PROCBASED_CTLS_CR8_STORE_EXITING(_)                 (((_) >> 20) & 0x01)

	/**
	* @brief Use TPR shadow
	*
	* [Bit 21] Setting this control to 1 enables TPR virtualization and other APIC-virtualization features.
	*
	* @see Vol3C[29(APIC VIRTUALIZATION AND VIRTUAL INTERRUPTS)]
	*/
	uint64_t use_tpr_shadow                                          : 1;
#define IA32_VMX_PROCBASED_CTLS_USE_TPR_SHADOW_BIT                   21
#define IA32_VMX_PROCBASED_CTLS_USE_TPR_SHADOW_FLAG                  0x200000
#define IA32_VMX_PROCBASED_CTLS_USE_TPR_SHADOW_MASK                  0x01
#define IA32_VMX_PROCBASED_CTLS_USE_TPR_SHADOW(_)                    (((_) >> 21) & 0x01)

	/**
	* @brief VM-exit when virtual NMI blocking is disabled
	*
	* [Bit 22] If this control is 1, a VM exit occurs at the beginning of any instruction if there is no virtual-NMI blocking.
	*
	* @see Vol3C[24.4.2(Guest Non-Register State)]
	*/
	uint64_t nmi_window_exiting                                      : 1;
#define IA32_VMX_PROCBASED_CTLS_NMI_WINDOW_EXITING_BIT               22
#define IA32_VMX_PROCBASED_CTLS_NMI_WINDOW_EXITING_FLAG              0x400000
#define IA32_VMX_PROCBASED_CTLS_NMI_WINDOW_EXITING_MASK              0x01
#define IA32_VMX_PROCBASED_CTLS_NMI_WINDOW_EXITING(_)                (((_) >> 22) & 0x01)

	/**
	* @brief VM-exit when executing a MOV DRx instruction
	*
	* [Bit 23] This control determines whether executions of MOV DR cause VM exits.
	*/
	uint64_t mov_dr_exiting                                          : 1;
#define IA32_VMX_PROCBASED_CTLS_MOV_DR_EXITING_BIT                   23
#define IA32_VMX_PROCBASED_CTLS_MOV_DR_EXITING_FLAG                  0x800000
#define IA32_VMX_PROCBASED_CTLS_MOV_DR_EXITING_MASK                  0x01
#define IA32_VMX_PROCBASED_CTLS_MOV_DR_EXITING(_)                    (((_) >> 23) & 0x01)

	/**
	* @brief VM-exit when executing IO instructions
	*
	* [Bit 24] This control determines whether executions of I/O instructions (IN, INS/INSB/INSW/INSD, OUT, and
	* OUTS/OUTSB/OUTSW/OUTSD) cause VM exits.
	*/
	uint64_t unconditional_io_exiting                                : 1;
#define IA32_VMX_PROCBASED_CTLS_UNCONDITIONAL_IO_EXITING_BIT         24
#define IA32_VMX_PROCBASED_CTLS_UNCONDITIONAL_IO_EXITING_FLAG        0x1000000
#define IA32_VMX_PROCBASED_CTLS_UNCONDITIONAL_IO_EXITING_MASK        0x01
#define IA32_VMX_PROCBASED_CTLS_UNCONDITIONAL_IO_EXITING(_)          (((_) >> 24) & 0x01)

	/**
	* @brief Use IO bitmaps
	*
	* [Bit 25] This control determines whether I/O bitmaps are used to restrict executions of I/O instructions For this
	* control, "0" means "do not use I/O bitmaps" and "1" means "use I/O bitmaps." If the I/O bitmaps are used, the setting of
	* the "unconditional I/O exiting" control is ignored.
	*
	* @see Vol3C[24.6.4(I/O-Bitmap Addresses)]
	* @see Vol3C[25.1.3(Instructions That Cause VM Exits Conditionally)]
	*/
	uint64_t use_io_bitmaps                                          : 1;
#define IA32_VMX_PROCBASED_CTLS_USE_IO_BITMAPS_BIT                   25
#define IA32_VMX_PROCBASED_CTLS_USE_IO_BITMAPS_FLAG                  0x2000000
#define IA32_VMX_PROCBASED_CTLS_USE_IO_BITMAPS_MASK                  0x01
#define IA32_VMX_PROCBASED_CTLS_USE_IO_BITMAPS(_)                    (((_) >> 25) & 0x01)
	uint64_t reserved6                                               : 1;

	/**
	* @brief Monitor trap flag
	*
	* [Bit 27] If this control is 1, the monitor trap flag debugging feature is enabled.
	*
	* @see Vol3C[25.5.2(Monitor Trap Flag)]
	*/
	uint64_t monitor_trap_flag                                       : 1;
#define IA32_VMX_PROCBASED_CTLS_MONITOR_TRAP_FLAG_BIT                27
#define IA32_VMX_PROCBASED_CTLS_MONITOR_TRAP_FLAG_FLAG               0x8000000
#define IA32_VMX_PROCBASED_CTLS_MONITOR_TRAP_FLAG_MASK               0x01
#define IA32_VMX_PROCBASED_CTLS_MONITOR_TRAP_FLAG(_)                 (((_) >> 27) & 0x01)

	/**
	* @brief Use MSR bitmaps
	*
	* [Bit 28] This control determines whether MSR bitmaps are used to control execution of the RDMSR and WRMSR instructions.
	* For this control, "0" means "do not use MSR bitmaps" and "1" means "use MSR bitmaps." If the MSR bitmaps are not used,
	* all executions of the RDMSR and WRMSR instructions cause VM exits.
	*
	* @see Vol3C[24.6.9(MSR-Bitmap Address)]
	* @see Vol3C[25.1.3(Instructions That Cause VM Exits Conditionally)]
	*/
	uint64_t use_msr_bitmaps                                         : 1;
#define IA32_VMX_PROCBASED_CTLS_USE_MSR_BITMAPS_BIT                  28
#define IA32_VMX_PROCBASED_CTLS_USE_MSR_BITMAPS_FLAG                 0x10000000
#define IA32_VMX_PROCBASED_CTLS_USE_MSR_BITMAPS_MASK                 0x01
#define IA32_VMX_PROCBASED_CTLS_USE_MSR_BITMAPS(_)                   (((_) >> 28) & 0x01)

	/**
	* @brief VM-exit when executing the MONITOR instruction
	*
	* [Bit 29] This control determines whether executions of MONITOR cause VM exits.
	*/
	uint64_t monitor_exiting                                         : 1;
#define IA32_VMX_PROCBASED_CTLS_MONITOR_EXITING_BIT                  29
#define IA32_VMX_PROCBASED_CTLS_MONITOR_EXITING_FLAG                 0x20000000
#define IA32_VMX_PROCBASED_CTLS_MONITOR_EXITING_MASK                 0x01
#define IA32_VMX_PROCBASED_CTLS_MONITOR_EXITING(_)                   (((_) >> 29) & 0x01)

	/**
	* @brief VM-exit when executing the PAUSE instruction
	*
	* [Bit 30] This control determines whether executions of PAUSE cause VM exits.
	*/
	uint64_t pause_exiting                                           : 1;
#define IA32_VMX_PROCBASED_CTLS_PAUSE_EXITING_BIT                    30
#define IA32_VMX_PROCBASED_CTLS_PAUSE_EXITING_FLAG                   0x40000000
#define IA32_VMX_PROCBASED_CTLS_PAUSE_EXITING_MASK                   0x01
#define IA32_VMX_PROCBASED_CTLS_PAUSE_EXITING(_)                     (((_) >> 30) & 0x01)

	/**
	* @brief Determines whether the secondary processor based VM-execution controls are used
	*
	* [Bit 31] This control determines whether the secondary processor-based VM-execution controls are used. If this control
	* is 0, the logical processor operates as if all the secondary processor-based VM-execution controls were also 0.
	*/
	uint64_t activate_secondary_controls                             : 1;
#define IA32_VMX_PROCBASED_CTLS_ACTIVATE_SECONDARY_CONTROLS_BIT      31
#define IA32_VMX_PROCBASED_CTLS_ACTIVATE_SECONDARY_CONTROLS_FLAG     0x80000000
#define IA32_VMX_PROCBASED_CTLS_ACTIVATE_SECONDARY_CONTROLS_MASK     0x01
#define IA32_VMX_PROCBASED_CTLS_ACTIVATE_SECONDARY_CONTROLS(_)       (((_) >> 31) & 0x01)
	uint64_t reserved7                                               : 32;
  };

  uint64_t flags;
} vmx_procbased_ctls_register;


/**
 * Capability Reporting Register of VM-Exit Controls.
 *
 * @remarks If CPUID.01H:ECX.[5] = 1
 * @see Vol3D[A.4(VM-EXIT CONTROLS)]
 * @see Vol3C[24.7.1(VM-Exit Controls)] (reference)
 */
#define IA32_VMX_EXIT_CTLS                                           0x00000483
typedef union
{
  struct
  {
	uint64_t reserved1                                               : 2;

	/**
	* @brief Save guest debug controls (dr7 & IA32_DEBUGCTL_MSR) (forced to 1 on the 'first' VT-x capable CPUs; this actually
	*        includes the newest Nehalem CPUs)
	*
	* [Bit 2] This control determines whether DR7 and the IA32_DEBUGCTL MSR are saved on VM exit. The first processors to
	* support the virtual-machine extensions supported only the 1-setting of this control.
	*/
	uint64_t save_debug_controls                                     : 1;
#define IA32_VMX_EXIT_CTLS_SAVE_DEBUG_CONTROLS_BIT                   2
#define IA32_VMX_EXIT_CTLS_SAVE_DEBUG_CONTROLS_FLAG                  0x04
#define IA32_VMX_EXIT_CTLS_SAVE_DEBUG_CONTROLS_MASK                  0x01
#define IA32_VMX_EXIT_CTLS_SAVE_DEBUG_CONTROLS(_)                    (((_) >> 2) & 0x01)
	uint64_t reserved2                                               : 6;

	/**
	* @brief Return to long mode after a VM-exit
	*
	* [Bit 9] On processors that support Intel 64 architecture, this control determines whether a logical processor is in
	* 64-bit mode after the next VM exit. Its value is loaded into CS.L, IA32_EFER.LME, and IA32_EFER.LMA on every VM exit.1
	* This control must be 0 on processors that do not support Intel 64 architecture.
	*/
	uint64_t host_address_space_size                                 : 1;
#define IA32_VMX_EXIT_CTLS_HOST_ADDRESS_SPACE_SIZE_BIT               9
#define IA32_VMX_EXIT_CTLS_HOST_ADDRESS_SPACE_SIZE_FLAG              0x200
#define IA32_VMX_EXIT_CTLS_HOST_ADDRESS_SPACE_SIZE_MASK              0x01
#define IA32_VMX_EXIT_CTLS_HOST_ADDRESS_SPACE_SIZE(_)                (((_) >> 9) & 0x01)
	uint64_t reserved3                                               : 2;

	/**
	* @brief Whether the IA32_PERF_GLOBAL_CTRL MSR is loaded on VM-exit
	*
	* [Bit 12] This control determines whether the IA32_PERF_GLOBAL_CTRL MSR is loaded on VM exit.
	*/
	uint64_t load_ia32_perf_global_ctrl                              : 1;
#define IA32_VMX_EXIT_CTLS_LOAD_IA32_PERF_GLOBAL_CTRL_BIT            12
#define IA32_VMX_EXIT_CTLS_LOAD_IA32_PERF_GLOBAL_CTRL_FLAG           0x1000
#define IA32_VMX_EXIT_CTLS_LOAD_IA32_PERF_GLOBAL_CTRL_MASK           0x01
#define IA32_VMX_EXIT_CTLS_LOAD_IA32_PERF_GLOBAL_CTRL(_)             (((_) >> 12) & 0x01)
	uint64_t reserved4                                               : 2;

	/**
	* @brief Acknowledge external interrupts with the irq controller if one caused a VM-exit
	*
	* [Bit 15] This control affects VM exits due to external interrupts:
	* - If such a VM exit occurs and this control is 1, the logical processor acknowledges the interrupt controller, acquiring
	* the interrupt's vector. The vector is stored in the VM-exit interruption-information field, which is marked valid.
	* - If such a VM exit occurs and this control is 0, the interrupt is not acknowledged and the VM-exit
	* interruption-information field is marked invalid.
	*/
	uint64_t acknowledge_interrupt_on_exit                           : 1;
#define IA32_VMX_EXIT_CTLS_ACKNOWLEDGE_INTERRUPT_ON_EXIT_BIT         15
#define IA32_VMX_EXIT_CTLS_ACKNOWLEDGE_INTERRUPT_ON_EXIT_FLAG        0x8000
#define IA32_VMX_EXIT_CTLS_ACKNOWLEDGE_INTERRUPT_ON_EXIT_MASK        0x01
#define IA32_VMX_EXIT_CTLS_ACKNOWLEDGE_INTERRUPT_ON_EXIT(_)          (((_) >> 15) & 0x01)
	uint64_t reserved5                                               : 2;

	/**
	* @brief Whether the guest IA32_PAT MSR is saved on VM-exit
	*
	* [Bit 18] This control determines whether the IA32_PAT MSR is saved on VM exit.
	*/
	uint64_t save_ia32_pat                                           : 1;
#define IA32_VMX_EXIT_CTLS_SAVE_IA32_PAT_BIT                         18
#define IA32_VMX_EXIT_CTLS_SAVE_IA32_PAT_FLAG                        0x40000
#define IA32_VMX_EXIT_CTLS_SAVE_IA32_PAT_MASK                        0x01
#define IA32_VMX_EXIT_CTLS_SAVE_IA32_PAT(_)                          (((_) >> 18) & 0x01)

	/**
	* @brief Whether the host IA32_PAT MSR is loaded on VM-exit
	*
	* [Bit 19] This control determines whether the IA32_PAT MSR is loaded on VM exit.
	*/
	uint64_t load_ia32_pat                                           : 1;
#define IA32_VMX_EXIT_CTLS_LOAD_IA32_PAT_BIT                         19
#define IA32_VMX_EXIT_CTLS_LOAD_IA32_PAT_FLAG                        0x80000
#define IA32_VMX_EXIT_CTLS_LOAD_IA32_PAT_MASK                        0x01
#define IA32_VMX_EXIT_CTLS_LOAD_IA32_PAT(_)                          (((_) >> 19) & 0x01)

	/**
	* @brief Whether the guest IA32_EFER MSR is saved on VM-exit
	*
	* [Bit 20] This control determines whether the IA32_EFER MSR is saved on VM exit.
	*/
	uint64_t save_ia32_efer                                          : 1;
#define IA32_VMX_EXIT_CTLS_SAVE_IA32_EFER_BIT                        20
#define IA32_VMX_EXIT_CTLS_SAVE_IA32_EFER_FLAG                       0x100000
#define IA32_VMX_EXIT_CTLS_SAVE_IA32_EFER_MASK                       0x01
#define IA32_VMX_EXIT_CTLS_SAVE_IA32_EFER(_)                         (((_) >> 20) & 0x01)

	/**
	* @brief Whether the host IA32_EFER MSR is loaded on VM-exit
	*
	* [Bit 21] This control determines whether the IA32_EFER MSR is loaded on VM exit.
	*/
	uint64_t load_ia32_efer                                          : 1;
#define IA32_VMX_EXIT_CTLS_LOAD_IA32_EFER_BIT                        21
#define IA32_VMX_EXIT_CTLS_LOAD_IA32_EFER_FLAG                       0x200000
#define IA32_VMX_EXIT_CTLS_LOAD_IA32_EFER_MASK                       0x01
#define IA32_VMX_EXIT_CTLS_LOAD_IA32_EFER(_)                         (((_) >> 21) & 0x01)

	/**
	* @brief Whether the value of the VMX preemption timer is saved on every VM-exit
	*
	* [Bit 22] This control determines whether the value of the VMX-preemption timer is saved on VM exit.
	*/
	uint64_t save_vmx_preemption_timer_value                         : 1;
#define IA32_VMX_EXIT_CTLS_SAVE_VMX_PREEMPTION_TIMER_VALUE_BIT       22
#define IA32_VMX_EXIT_CTLS_SAVE_VMX_PREEMPTION_TIMER_VALUE_FLAG      0x400000
#define IA32_VMX_EXIT_CTLS_SAVE_VMX_PREEMPTION_TIMER_VALUE_MASK      0x01
#define IA32_VMX_EXIT_CTLS_SAVE_VMX_PREEMPTION_TIMER_VALUE(_)        (((_) >> 22) & 0x01)

	/**
	* [Bit 23] This control determines whether the IA32_BNDCFGS MSR is cleared on VM exit.
	*/
	uint64_t clear_ia32_bndcfgs                                      : 1;
#define IA32_VMX_EXIT_CTLS_CLEAR_IA32_BNDCFGS_BIT                    23
#define IA32_VMX_EXIT_CTLS_CLEAR_IA32_BNDCFGS_FLAG                   0x800000
#define IA32_VMX_EXIT_CTLS_CLEAR_IA32_BNDCFGS_MASK                   0x01
#define IA32_VMX_EXIT_CTLS_CLEAR_IA32_BNDCFGS(_)                     (((_) >> 23) & 0x01)

	/**
	* [Bit 24] If this control is 1, Intel Processor Trace does not produce a paging information packet (PIP) on a VM exit or
	* a VMCS packet on an SMM VM exit.
	*
	* @see Vol3C[35(INTEL(R) PROCESSOR TRACE)]
	*/
	uint64_t conceal_vmx_from_pt                                     : 1;
#define IA32_VMX_EXIT_CTLS_CONCEAL_VMX_FROM_PT_BIT                   24
#define IA32_VMX_EXIT_CTLS_CONCEAL_VMX_FROM_PT_FLAG                  0x1000000
#define IA32_VMX_EXIT_CTLS_CONCEAL_VMX_FROM_PT_MASK                  0x01
#define IA32_VMX_EXIT_CTLS_CONCEAL_VMX_FROM_PT(_)                    (((_) >> 24) & 0x01)
	uint64_t reserved6                                               : 39;
  };

  uint64_t flags;
} vmx_exit_ctls_register;


/**
 * Capability Reporting Register of VM-Entry Controls.
 *
 * @remarks If CPUID.01H:ECX.[5] = 1
 * @see Vol3D[A.5(VM-ENTRY CONTROLS)]
 * @see Vol3D[24.8.1(VM-Entry Controls)] (reference)
 */
#define IA32_VMX_ENTRY_CTLS                                          0x00000484
typedef union
{
  struct
  {
	uint64_t reserved1                                               : 2;

	/**
	* @brief Load guest debug controls (dr7 & IA32_DEBUGCTL_MSR) (forced to 1 on the 'first' VT-x capable CPUs; this actually
	*        includes the newest Nehalem CPUs)
	*
	* [Bit 2] This control determines whether DR7 and the IA32_DEBUGCTL MSR are loaded on VM entry. The first processors to
	* support the virtual-machine extensions supported only the 1-setting of this control.
	*/
	uint64_t load_debug_controls                                     : 1;
#define IA32_VMX_ENTRY_CTLS_LOAD_DEBUG_CONTROLS_BIT                  2
#define IA32_VMX_ENTRY_CTLS_LOAD_DEBUG_CONTROLS_FLAG                 0x04
#define IA32_VMX_ENTRY_CTLS_LOAD_DEBUG_CONTROLS_MASK                 0x01
#define IA32_VMX_ENTRY_CTLS_LOAD_DEBUG_CONTROLS(_)                   (((_) >> 2) & 0x01)
	uint64_t reserved2                                               : 6;

	/**
	* @brief 64 bits guest mode. Must be 0 for CPUs that don't support AMD64
	*
	* [Bit 9] On processors that support Intel 64 architecture, this control determines whether the logical processor is in
	* IA-32e mode after VM entry. Its value is loaded into IA32_EFER.LMA as part of VM entry. This control must be 0 on
	* processors that do not support Intel 64 architecture.
	*/
	uint64_t ia32e_mode_guest                                        : 1;
#define IA32_VMX_ENTRY_CTLS_IA32E_MODE_GUEST_BIT                     9
#define IA32_VMX_ENTRY_CTLS_IA32E_MODE_GUEST_FLAG                    0x200
#define IA32_VMX_ENTRY_CTLS_IA32E_MODE_GUEST_MASK                    0x01
#define IA32_VMX_ENTRY_CTLS_IA32E_MODE_GUEST(_)                      (((_) >> 9) & 0x01)

	/**
	* @brief In SMM mode after VM-entry
	*
	* [Bit 10] This control determines whether the logical processor is in system-management mode (SMM) after VM entry. This
	* control must be 0 for any VM entry from outside SMM.
	*/
	uint64_t entry_to_smm                                            : 1;
#define IA32_VMX_ENTRY_CTLS_ENTRY_TO_SMM_BIT                         10
#define IA32_VMX_ENTRY_CTLS_ENTRY_TO_SMM_FLAG                        0x400
#define IA32_VMX_ENTRY_CTLS_ENTRY_TO_SMM_MASK                        0x01
#define IA32_VMX_ENTRY_CTLS_ENTRY_TO_SMM(_)                          (((_) >> 10) & 0x01)

	/**
	* @brief Disable dual treatment of SMI and SMM; must be zero for VM-entry outside of SMM
	*
	* [Bit 11] If set to 1, the default treatment of SMIs and SMM is in effect after the VM entry. This control must be 0 for
	* any VM entry from outside SMM
	*
	* @see Vol3C[34.15.7(Deactivating the Dual-Monitor Treatment)]
	*/
	uint64_t deactivate_dual_monitor_treatment                       : 1;
#define IA32_VMX_ENTRY_CTLS_DEACTIVATE_DUAL_MONITOR_TREATMENT_BIT    11
#define IA32_VMX_ENTRY_CTLS_DEACTIVATE_DUAL_MONITOR_TREATMENT_FLAG   0x800
#define IA32_VMX_ENTRY_CTLS_DEACTIVATE_DUAL_MONITOR_TREATMENT_MASK   0x01
#define IA32_VMX_ENTRY_CTLS_DEACTIVATE_DUAL_MONITOR_TREATMENT(_)     (((_) >> 11) & 0x01)
	uint64_t reserved3                                               : 1;

	/**
	* @brief Whether the guest IA32_PERF_GLOBAL_CTRL MSR is loaded on VM-entry
	*
	* [Bit 13] This control determines whether the IA32_PERF_GLOBAL_CTRL MSR is loaded on VM entry.
	*/
	uint64_t load_ia32_perf_global_ctrl                              : 1;
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_PERF_GLOBAL_CTRL_BIT           13
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_PERF_GLOBAL_CTRL_FLAG          0x2000
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_PERF_GLOBAL_CTRL_MASK          0x01
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_PERF_GLOBAL_CTRL(_)            (((_) >> 13) & 0x01)

	/**
	* @brief Whether the guest IA32_PAT MSR is loaded on VM-entry
	*
	* [Bit 14] This control determines whether the IA32_PAT MSR is loaded on VM entry.
	*/
	uint64_t load_ia32_pat                                           : 1;
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_PAT_BIT                        14
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_PAT_FLAG                       0x4000
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_PAT_MASK                       0x01
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_PAT(_)                         (((_) >> 14) & 0x01)

	/**
	* @brief Whether the guest IA32_EFER MSR is loaded on VM-entry
	*
	* [Bit 15] This control determines whether the IA32_EFER MSR is loaded on VM entry.
	*/
	uint64_t load_ia32_efer                                          : 1;
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_EFER_BIT                       15
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_EFER_FLAG                      0x8000
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_EFER_MASK                      0x01
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_EFER(_)                        (((_) >> 15) & 0x01)

	/**
	* [Bit 16] This control determines whether the IA32_BNDCFGS MSR is loaded on VM entry.
	*/
	uint64_t load_ia32_bndcfgs                                       : 1;
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_BNDCFGS_BIT                    16
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_BNDCFGS_FLAG                   0x10000
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_BNDCFGS_MASK                   0x01
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_BNDCFGS(_)                     (((_) >> 16) & 0x01)

	/**
	* [Bit 17] If this control is 1, Intel Processor Trace does not produce a paging information packet (PIP) on a VM entry or
	* a VMCS packet on a VM entry that returns from SMM.
	*
	* @see Vol3C[35(INTEL(R) PROCESSOR TRACE)]
	*/
	uint64_t conceal_vmx_from_pt                                     : 1;
#define IA32_VMX_ENTRY_CTLS_CONCEAL_VMX_FROM_PT_BIT                  17
#define IA32_VMX_ENTRY_CTLS_CONCEAL_VMX_FROM_PT_FLAG                 0x20000
#define IA32_VMX_ENTRY_CTLS_CONCEAL_VMX_FROM_PT_MASK                 0x01
#define IA32_VMX_ENTRY_CTLS_CONCEAL_VMX_FROM_PT(_)                   (((_) >> 17) & 0x01)

	/**
	* [Bit 18] This control determines whether the IA32_RTIT_CTL MSR is loaded on VM entry.
	*/
	uint64_t load_ia32_rtit_ctl                                      : 1;
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_RTIT_CTL_BIT                   18
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_RTIT_CTL_FLAG                  0x40000
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_RTIT_CTL_MASK                  0x01
#define IA32_VMX_ENTRY_CTLS_LOAD_IA32_RTIT_CTL(_)                    (((_) >> 18) & 0x01)
	uint64_t reserved4                                               : 1;

	/**
	* [Bit 20] This control determines whether CET-related MSRs and SPP are loaded on VM entry.
	*/
	uint64_t load_cet_state                                          : 1;
#define IA32_VMX_ENTRY_CTLS_LOAD_CET_STATE_BIT                       20
#define IA32_VMX_ENTRY_CTLS_LOAD_CET_STATE_FLAG                      0x100000
#define IA32_VMX_ENTRY_CTLS_LOAD_CET_STATE_MASK                      0x01
#define IA32_VMX_ENTRY_CTLS_LOAD_CET_STATE(_)                        (((_) >> 20) & 0x01)
	uint64_t reserved5                                               : 43;
  };

  uint64_t flags;
} vmx_entry_ctls_register;


/**
 * Reporting Register of Miscellaneous VMX Capabilities.
 *
 * @remarks If CPUID.01H:ECX.[5] = 1
 * @see Vol3D[A.6(MISCELLANEOUS DATA)]
 * @see Vol3D[A.6(Miscellaneous Data)] (reference)
 */
#define IA32_VMX_MISC                                                0x00000485
typedef union
{
  struct
  {
	/**
	* @brief Relationship between the preemption timer and tsc; count down every time bit x of the tsc changes
	*
	* [Bits 4:0] Report a value X that specifies the relationship between the rate of the VMX-preemption timer and that of the
	* timestamp counter (TSC). Specifically, the VMX-preemption timer (if it is active) counts down by 1 every time bit X in
	* the TSC changes due to a TSC increment.
	*/
	uint64_t preemption_timer_tsc_relationship                       : 5;
#define IA32_VMX_MISC_PREEMPTION_TIMER_TSC_RELATIONSHIP_BIT          0
#define IA32_VMX_MISC_PREEMPTION_TIMER_TSC_RELATIONSHIP_FLAG         0x1F
#define IA32_VMX_MISC_PREEMPTION_TIMER_TSC_RELATIONSHIP_MASK         0x1F
#define IA32_VMX_MISC_PREEMPTION_TIMER_TSC_RELATIONSHIP(_)           (((_) >> 0) & 0x1F)

	/**
	* @brief Whether VM-exit stores EFER.LMA into the "IA32e mode guest" field
	*
	* [Bit 5] When set to 1, VM exits store the value of IA32_EFER.LMA into the "IA-32e mode guest" VM-entry control. This bit
	* is read as 1 on any logical processor that supports the 1-setting of the "unrestricted guest" VM-execution control.
	*
	* @see Vol3C[27.2(RECORDING VM-EXIT INFORMATION AND UPDATING VM-ENTRY CONTROL FIELDS)]
	*/
	uint64_t store_efer_lma_on_vmexit                                : 1;
#define IA32_VMX_MISC_STORE_EFER_LMA_ON_VMEXIT_BIT                   5
#define IA32_VMX_MISC_STORE_EFER_LMA_ON_VMEXIT_FLAG                  0x20
#define IA32_VMX_MISC_STORE_EFER_LMA_ON_VMEXIT_MASK                  0x01
#define IA32_VMX_MISC_STORE_EFER_LMA_ON_VMEXIT(_)                    (((_) >> 5) & 0x01)

	/**
	* @brief Activity states supported by the implementation
	*
	* [Bits 8:6] Report, as a bitmap, the activity states supported by the implementation:
	* - Bit 6 reports (if set) the support for activity state 1 (HLT).
	* - Bit 7 reports (if set) the support for activity state 2 (shutdown).
	* - Bit 8 reports (if set) the support for activity state 3 (wait-for-SIPI).
	* If an activity state is not supported, the implementation causes a VM entry to fail if it attempts to establish that
	* activity state. All implementations support VM entry to activity state 0 (active).
	*/
	uint64_t activity_states                                         : 3;
#define IA32_VMX_MISC_ACTIVITY_STATES_BIT                            6
#define IA32_VMX_MISC_ACTIVITY_STATES_FLAG                           0x1C0
#define IA32_VMX_MISC_ACTIVITY_STATES_MASK                           0x07
#define IA32_VMX_MISC_ACTIVITY_STATES(_)                             (((_) >> 6) & 0x07)
	uint64_t reserved1                                               : 5;

	/**
	* @brief Intel Processor Trace (Intel PT) can be used in VMX operation
	*
	* [Bit 14] When set to 1, Intel(R) Processor Trace (Intel PT) can be used in VMX operation. If the processor supports Intel
	* PT but does not allow it to be used in VMX operation, execution of VMXON clears IA32_RTIT_CTL.TraceEn; any attempt to
	* write IA32_RTIT_CTL while in VMX operation (including VMX root operation) causes a general-protection exception.
	*
	* @see Vol3C[30.3(VMX INSTRUCTIONS | VMXON-Enter VMX Operation)]
	*/
	uint64_t intel_pt_available_in_vmx                               : 1;
#define IA32_VMX_MISC_INTEL_PT_AVAILABLE_IN_VMX_BIT                  14
#define IA32_VMX_MISC_INTEL_PT_AVAILABLE_IN_VMX_FLAG                 0x4000
#define IA32_VMX_MISC_INTEL_PT_AVAILABLE_IN_VMX_MASK                 0x01
#define IA32_VMX_MISC_INTEL_PT_AVAILABLE_IN_VMX(_)                   (((_) >> 14) & 0x01)

	/**
	* @brief Whether RDMSR can be used to read IA32_SMBASE_MSR in SMM
	*
	* [Bit 15] When set to 1, the RDMSR instruction can be used in system-management mode (SMM) to read the IA32_SMBASE MSR
	* (MSR address 9EH).
	*
	* @see Vol3C[34.15.6.3(Saving Guest State)]
	*/
	uint64_t rdmsr_can_read_ia32_smbase_msr_in_smm                   : 1;
#define IA32_VMX_MISC_RDMSR_CAN_READ_IA32_SMBASE_MSR_IN_SMM_BIT      15
#define IA32_VMX_MISC_RDMSR_CAN_READ_IA32_SMBASE_MSR_IN_SMM_FLAG     0x8000
#define IA32_VMX_MISC_RDMSR_CAN_READ_IA32_SMBASE_MSR_IN_SMM_MASK     0x01
#define IA32_VMX_MISC_RDMSR_CAN_READ_IA32_SMBASE_MSR_IN_SMM(_)       (((_) >> 15) & 0x01)

	/**
	* @brief Number of CR3 target values supported by the processor (0-256)
	*
	* [Bits 24:16] Indicate the number of CR3-target values supported by the processor. This number is a value between 0 and
	* 256, inclusive (bit 24 is set if and only if bits 23:16 are clear).
	*/
	uint64_t cr3_target_count                                        : 9;
#define IA32_VMX_MISC_CR3_TARGET_COUNT_BIT                           16
#define IA32_VMX_MISC_CR3_TARGET_COUNT_FLAG                          0x1FF0000
#define IA32_VMX_MISC_CR3_TARGET_COUNT_MASK                          0x1FF
#define IA32_VMX_MISC_CR3_TARGET_COUNT(_)                            (((_) >> 16) & 0x1FF)

	/**
	* @brief Maximum number of MSRs in the VMCS. (N+1)*512
	*
	* [Bits 27:25] Used to compute the recommended maximum number of MSRs that should appear in the VM-exit MSR-store list,
	* the VM-exit MSR-load list, or the VM-entry MSR-load list. Specifically, if the value bits 27:25 of IA32_VMX_MISC is N,
	* then 512 * (N + 1) is the recommended maximum number of MSRs to be included in each list. If the limit is exceeded,
	* undefined processor behavior may result (including a machine check during the VMX transition).
	*/
	uint64_t max_number_of_msr                                       : 3;
#define IA32_VMX_MISC_MAX_NUMBER_OF_MSR_BIT                          25
#define IA32_VMX_MISC_MAX_NUMBER_OF_MSR_FLAG                         0xE000000
#define IA32_VMX_MISC_MAX_NUMBER_OF_MSR_MASK                         0x07
#define IA32_VMX_MISC_MAX_NUMBER_OF_MSR(_)                           (((_) >> 25) & 0x07)

	/**
	* @brief Whether bit 2 of IA32_SMM_MONITOR_CTL can be set to 1
	*
	* [Bit 28] When set to 1, bit 2 of the IA32_SMM_MONITOR_CTL can be set to 1. VMXOFF unblocks SMIs unless
	* IA32_SMM_MONITOR_CTL[bit 2] is 1.
	*
	* @see Vol3C[34.14.4(VMXOFF and SMI Unblocking)]
	*/
	uint64_t smm_monitor_ctl_b2                                      : 1;
#define IA32_VMX_MISC_SMM_MONITOR_CTL_B2_BIT                         28
#define IA32_VMX_MISC_SMM_MONITOR_CTL_B2_FLAG                        0x10000000
#define IA32_VMX_MISC_SMM_MONITOR_CTL_B2_MASK                        0x01
#define IA32_VMX_MISC_SMM_MONITOR_CTL_B2(_)                          (((_) >> 28) & 0x01)

	/**
	* @brief Whether VMWRITE can be used to write VM-exit information fields
	*
	* [Bit 29] When set to 1, software can use VMWRITE to write to any supported field in the VMCS; otherwise, VMWRITE cannot
	* be used to modify VM-exit information fields.
	*/
	uint64_t vmwrite_vmexit_info                                     : 1;
#define IA32_VMX_MISC_VMWRITE_VMEXIT_INFO_BIT                        29
#define IA32_VMX_MISC_VMWRITE_VMEXIT_INFO_FLAG                       0x20000000
#define IA32_VMX_MISC_VMWRITE_VMEXIT_INFO_MASK                       0x01
#define IA32_VMX_MISC_VMWRITE_VMEXIT_INFO(_)                         (((_) >> 29) & 0x01)

	/**
	* [Bit 30] When set to 1, VM entry allows injection of a software interrupt, software exception, or privileged software
	* exception with an instruction length of 0.
	*/
	uint64_t zero_length_instruction_vmentry_injection               : 1;
#define IA32_VMX_MISC_ZERO_LENGTH_INSTRUCTION_VMENTRY_INJECTION_BIT  30
#define IA32_VMX_MISC_ZERO_LENGTH_INSTRUCTION_VMENTRY_INJECTION_FLAG 0x40000000
#define IA32_VMX_MISC_ZERO_LENGTH_INSTRUCTION_VMENTRY_INJECTION_MASK 0x01
#define IA32_VMX_MISC_ZERO_LENGTH_INSTRUCTION_VMENTRY_INJECTION(_)   (((_) >> 30) & 0x01)
	uint64_t reserved2                                               : 1;

	/**
	* @brief MSEG revision identifier used by the processor
	*
	* [Bits 63:32] Report the 32-bit MSEG revision identifier used by the processor.
	*/
	uint64_t mseg_id                                                 : 32;
#define IA32_VMX_MISC_MSEG_ID_BIT                                    32
#define IA32_VMX_MISC_MSEG_ID_FLAG                                   0xFFFFFFFF00000000
#define IA32_VMX_MISC_MSEG_ID_MASK                                   0xFFFFFFFF
#define IA32_VMX_MISC_MSEG_ID(_)                                     (((_) >> 32) & 0xFFFFFFFF)
  };

  uint64_t flags;
} vmx_misc_register;


/**
 * Capability Reporting Register of CR0 Bits Fixed to 0.
 *
 * @remarks If CPUID.01H:ECX.[5] = 1
 * @see Vol3D[A.7(VMX-FIXED BITS IN CR0)]
 * @see Vol3D[A.7(VMX-Fixed Bits in CR0)] (reference)
 */
#define IA32_VMX_CR0_FIXED0                                          0x00000486

/**
 * Capability Reporting Register of CR0 Bits Fixed to 1.
 *
 * @remarks If CPUID.01H:ECX.[5] = 1
 * @see Vol3D[A.7(VMX-FIXED BITS IN CR0)]
 * @see Vol3D[A.7(VMX-Fixed Bits in CR0)] (reference)
 */
#define IA32_VMX_CR0_FIXED1                                          0x00000487

/**
 * Capability Reporting Register of CR4 Bits Fixed to 0.
 *
 * @remarks If CPUID.01H:ECX.[5] = 1
 * @see Vol3D[A.8(VMX-FIXED BITS IN CR4)]
 * @see Vol3D[A.8(VMX-Fixed Bits in CR4)] (reference)
 */
#define IA32_VMX_CR4_FIXED0                                          0x00000488

/**
 * Capability Reporting Register of CR4 Bits Fixed to 1.
 *
 * @remarks If CPUID.01H:ECX.[5] = 1
 * @see Vol3D[A.8(VMX-FIXED BITS IN CR4)]
 * @see Vol3D[A.8(VMX-Fixed Bits in CR4)] (reference)
 */
#define IA32_VMX_CR4_FIXED1                                          0x00000489

/**
 * Capability Reporting Register of VMCS Field Enumeration.
 *
 * @remarks If CPUID.01H:ECX.[5] = 1
 * @see Vol3D[A.9(VMCS ENUMERATION)]
 * @see Vol3D[A.9(VMCS Enumeration)] (reference)
 */
#define IA32_VMX_VMCS_ENUM                                           0x0000048A
typedef union
{
  struct
  {
	/**
	* [Bit 0] Indicates access type.
	*/
	uint64_t access_type                                             : 1;
#define IA32_VMX_VMCS_ENUM_ACCESS_TYPE_BIT                           0
#define IA32_VMX_VMCS_ENUM_ACCESS_TYPE_FLAG                          0x01
#define IA32_VMX_VMCS_ENUM_ACCESS_TYPE_MASK                          0x01
#define IA32_VMX_VMCS_ENUM_ACCESS_TYPE(_)                            (((_) >> 0) & 0x01)

	/**
	* [Bits 9:1] Highest index value used for any VMCS encoding.
	*/
	uint64_t highest_index_value                                     : 9;
#define IA32_VMX_VMCS_ENUM_HIGHEST_INDEX_VALUE_BIT                   1
#define IA32_VMX_VMCS_ENUM_HIGHEST_INDEX_VALUE_FLAG                  0x3FE
#define IA32_VMX_VMCS_ENUM_HIGHEST_INDEX_VALUE_MASK                  0x1FF
#define IA32_VMX_VMCS_ENUM_HIGHEST_INDEX_VALUE(_)                    (((_) >> 1) & 0x1FF)

	/**
	* [Bits 11:10] Indicate the field's type.
	*/
	uint64_t field_type                                              : 2;
#define IA32_VMX_VMCS_ENUM_FIELD_TYPE_BIT                            10
#define IA32_VMX_VMCS_ENUM_FIELD_TYPE_FLAG                           0xC00
#define IA32_VMX_VMCS_ENUM_FIELD_TYPE_MASK                           0x03
#define IA32_VMX_VMCS_ENUM_FIELD_TYPE(_)                             (((_) >> 10) & 0x03)
	uint64_t reserved1                                               : 1;

	/**
	* [Bits 14:13] Indicate the field's width.
	*/
	uint64_t field_width                                             : 2;
#define IA32_VMX_VMCS_ENUM_FIELD_WIDTH_BIT                           13
#define IA32_VMX_VMCS_ENUM_FIELD_WIDTH_FLAG                          0x6000
#define IA32_VMX_VMCS_ENUM_FIELD_WIDTH_MASK                          0x03
#define IA32_VMX_VMCS_ENUM_FIELD_WIDTH(_)                            (((_) >> 13) & 0x03)
	uint64_t reserved2                                               : 49;
  };

  uint64_t flags;
} vmx_vmcs_enum_register;


/**
 * Capability Reporting Register of Secondary Processor-Based VM-Execution Controls.
 *
 * @remarks If ( CPUID.01H:ECX.[5] && IA32_VMX_PROCBASED_CTLS[63] )
 * @see Vol3D[A.3.3(Secondary Processor-Based VM-Execution Controls)]
 * @see Vol3D[24.6.2(Processor-Based VM-Execution Controls)] (reference)
 */
#define IA32_VMX_PROCBASED_CTLS2                                     0x0000048B
typedef union
{
  struct
  {
	/**
	* @brief Virtualize APIC access
	*
	* [Bit 0] If this control is 1, the logical processor treats specially accesses to the page with the APICaccess address.
	*
	* @see Vol3C[29.4(VIRTUALIZING MEMORY-MAPPED APIC ACCESSES)]
	*/
	uint64_t virtualize_apic_accesses                                : 1;
#define IA32_VMX_PROCBASED_CTLS2_VIRTUALIZE_APIC_ACCESSES_BIT        0
#define IA32_VMX_PROCBASED_CTLS2_VIRTUALIZE_APIC_ACCESSES_FLAG       0x01
#define IA32_VMX_PROCBASED_CTLS2_VIRTUALIZE_APIC_ACCESSES_MASK       0x01
#define IA32_VMX_PROCBASED_CTLS2_VIRTUALIZE_APIC_ACCESSES(_)         (((_) >> 0) & 0x01)

	/**
	* @brief EPT supported/enabled
	*
	* [Bit 1] If this control is 1, extended page tables (EPT) are enabled.
	*
	* @see Vol3C[28.2(THE EXTENDED PAGE TABLE MECHANISM (EPT))]
	*/
	uint64_t enable_ept                                              : 1;
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_EPT_BIT                      1
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_EPT_FLAG                     0x02
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_EPT_MASK                     0x01
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_EPT(_)                       (((_) >> 1) & 0x01)

	/**
	* @brief Descriptor table instructions cause VM-exits
	*
	* [Bit 2] This control determines whether executions of LGDT, LIDT, LLDT, LTR, SGDT, SIDT, SLDT, and STR cause VM exits.
	*/
	uint64_t descriptor_table_exiting                                : 1;
#define IA32_VMX_PROCBASED_CTLS2_DESCRIPTOR_TABLE_EXITING_BIT        2
#define IA32_VMX_PROCBASED_CTLS2_DESCRIPTOR_TABLE_EXITING_FLAG       0x04
#define IA32_VMX_PROCBASED_CTLS2_DESCRIPTOR_TABLE_EXITING_MASK       0x01
#define IA32_VMX_PROCBASED_CTLS2_DESCRIPTOR_TABLE_EXITING(_)         (((_) >> 2) & 0x01)

	/**
	* @brief RDTSCP supported/enabled
	*
	* [Bit 3] If this control is 0, any execution of RDTSCP causes an invalid-opcode exception (\#UD).
	*/
	uint64_t enable_rdtscp                                           : 1;
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_RDTSCP_BIT                   3
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_RDTSCP_FLAG                  0x08
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_RDTSCP_MASK                  0x01
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_RDTSCP(_)                    (((_) >> 3) & 0x01)

	/**
	* @brief Virtualize x2APIC mode
	*
	* [Bit 4] If this control is 1, the logical processor treats specially RDMSR and WRMSR to APIC MSRs (in the range
	* 800H-8FFH).
	*
	* @see Vol3C[29.5(VIRTUALIZING MSR-BASED APIC ACCESSES)]
	*/
	uint64_t virtualize_x2apic_mode                                  : 1;
#define IA32_VMX_PROCBASED_CTLS2_VIRTUALIZE_X2APIC_MODE_BIT          4
#define IA32_VMX_PROCBASED_CTLS2_VIRTUALIZE_X2APIC_MODE_FLAG         0x10
#define IA32_VMX_PROCBASED_CTLS2_VIRTUALIZE_X2APIC_MODE_MASK         0x01
#define IA32_VMX_PROCBASED_CTLS2_VIRTUALIZE_X2APIC_MODE(_)           (((_) >> 4) & 0x01)

	/**
	* @brief VPID supported/enabled
	*
	* [Bit 5] If this control is 1, cached translations of linear addresses are associated with a virtualprocessor identifier
	* (VPID).
	*
	* @see Vol3C[28.1(VIRTUAL PROCESSOR IDENTIFIERS (VPIDS))]
	*/
	uint64_t enable_vpid                                             : 1;
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_VPID_BIT                     5
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_VPID_FLAG                    0x20
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_VPID_MASK                    0x01
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_VPID(_)                      (((_) >> 5) & 0x01)

	/**
	* @brief VM-exit when executing the WBINVD instruction
	*
	* [Bit 6] This control determines whether executions of WBINVD cause VM exits.
	*/
	uint64_t wbinvd_exiting                                          : 1;
#define IA32_VMX_PROCBASED_CTLS2_WBINVD_EXITING_BIT                  6
#define IA32_VMX_PROCBASED_CTLS2_WBINVD_EXITING_FLAG                 0x40
#define IA32_VMX_PROCBASED_CTLS2_WBINVD_EXITING_MASK                 0x01
#define IA32_VMX_PROCBASED_CTLS2_WBINVD_EXITING(_)                   (((_) >> 6) & 0x01)

	/**
	* @brief Unrestricted guest execution
	*
	* [Bit 7] This control determines whether guest software may run in unpaged protected mode or in realaddress mode.
	*/
	uint64_t unrestricted_guest                                      : 1;
#define IA32_VMX_PROCBASED_CTLS2_UNRESTRICTED_GUEST_BIT              7
#define IA32_VMX_PROCBASED_CTLS2_UNRESTRICTED_GUEST_FLAG             0x80
#define IA32_VMX_PROCBASED_CTLS2_UNRESTRICTED_GUEST_MASK             0x01
#define IA32_VMX_PROCBASED_CTLS2_UNRESTRICTED_GUEST(_)               (((_) >> 7) & 0x01)

	/**
	* @brief APIC register virtualization
	*
	* [Bit 8] If this control is 1, the logical processor virtualizes certain APIC accesses.
	*
	* @see Vol3C[29.4(VIRTUALIZING MEMORY-MAPPED APIC ACCESSES)]
	* @see Vol3C[29.5(VIRTUALIZING MSR-BASED APIC ACCESSES)]
	*/
	uint64_t apic_register_virtualization                            : 1;
#define IA32_VMX_PROCBASED_CTLS2_APIC_REGISTER_VIRTUALIZATION_BIT    8
#define IA32_VMX_PROCBASED_CTLS2_APIC_REGISTER_VIRTUALIZATION_FLAG   0x100
#define IA32_VMX_PROCBASED_CTLS2_APIC_REGISTER_VIRTUALIZATION_MASK   0x01
#define IA32_VMX_PROCBASED_CTLS2_APIC_REGISTER_VIRTUALIZATION(_)     (((_) >> 8) & 0x01)

	/**
	* @brief Virtual-interrupt delivery
	*
	* [Bit 9] This controls enables the evaluation and delivery of pending virtual interrupts as well as the emulation of
	* writes to the APIC registers that control interrupt prioritization.
	*/
	uint64_t virtual_interrupt_delivery                              : 1;
#define IA32_VMX_PROCBASED_CTLS2_VIRTUAL_INTERRUPT_DELIVERY_BIT      9
#define IA32_VMX_PROCBASED_CTLS2_VIRTUAL_INTERRUPT_DELIVERY_FLAG     0x200
#define IA32_VMX_PROCBASED_CTLS2_VIRTUAL_INTERRUPT_DELIVERY_MASK     0x01
#define IA32_VMX_PROCBASED_CTLS2_VIRTUAL_INTERRUPT_DELIVERY(_)       (((_) >> 9) & 0x01)

	/**
	* @brief A specified number of pause loops cause a VM-exit
	*
	* [Bit 10] This control determines whether a series of executions of PAUSE can cause a VM exit.
	*
	* @see Vol3C[24.6.13(Controls for PAUSE-Loop Exiting)]
	* @see Vol3C[25.1.3(Instructions That Cause VM Exits Conditionally)]
	*/
	uint64_t pause_loop_exiting                                      : 1;
#define IA32_VMX_PROCBASED_CTLS2_PAUSE_LOOP_EXITING_BIT              10
#define IA32_VMX_PROCBASED_CTLS2_PAUSE_LOOP_EXITING_FLAG             0x400
#define IA32_VMX_PROCBASED_CTLS2_PAUSE_LOOP_EXITING_MASK             0x01
#define IA32_VMX_PROCBASED_CTLS2_PAUSE_LOOP_EXITING(_)               (((_) >> 10) & 0x01)

	/**
	* @brief VM-exit when executing RDRAND instructions
	*
	* [Bit 11] This control determines whether executions of RDRAND cause VM exits.
	*/
	uint64_t rdrand_exiting                                          : 1;
#define IA32_VMX_PROCBASED_CTLS2_RDRAND_EXITING_BIT                  11
#define IA32_VMX_PROCBASED_CTLS2_RDRAND_EXITING_FLAG                 0x800
#define IA32_VMX_PROCBASED_CTLS2_RDRAND_EXITING_MASK                 0x01
#define IA32_VMX_PROCBASED_CTLS2_RDRAND_EXITING(_)                   (((_) >> 11) & 0x01)

	/**
	* @brief Enables INVPCID instructions
	*
	* [Bit 12] If this control is 0, any execution of INVPCID causes a \#UD.
	*/
	uint64_t enable_invpcid                                          : 1;
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_INVPCID_BIT                  12
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_INVPCID_FLAG                 0x1000
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_INVPCID_MASK                 0x01
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_INVPCID(_)                   (((_) >> 12) & 0x01)

	/**
	* @brief Enables VMFUNC instructions
	*
	* [Bit 13] Setting this control to 1 enables use of the VMFUNC instruction in VMX non-root operation.
	*
	* @see Vol3C[25.5.5(VM Functions)]
	*/
	uint64_t enable_vm_functions                                     : 1;
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_VM_FUNCTIONS_BIT             13
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_VM_FUNCTIONS_FLAG            0x2000
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_VM_FUNCTIONS_MASK            0x01
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_VM_FUNCTIONS(_)              (((_) >> 13) & 0x01)

	/**
	* @brief Enables VMCS shadowing
	*
	* [Bit 14] If this control is 1, executions of VMREAD and VMWRITE in VMX non-root operation may access a shadow VMCS
	* (instead of causing VM exits).
	*
	* @see {'Vol3C[24.10(VMCS TYPES': 'ORDINARY AND SHADOW)]'}
	* @see Vol3C[30.3(VMX INSTRUCTIONS)]
	*/
	uint64_t vmcs_shadowing                                          : 1;
#define IA32_VMX_PROCBASED_CTLS2_VMCS_SHADOWING_BIT                  14
#define IA32_VMX_PROCBASED_CTLS2_VMCS_SHADOWING_FLAG                 0x4000
#define IA32_VMX_PROCBASED_CTLS2_VMCS_SHADOWING_MASK                 0x01
#define IA32_VMX_PROCBASED_CTLS2_VMCS_SHADOWING(_)                   (((_) >> 14) & 0x01)

	/**
	* @brief Enables ENCLS VM-exits
	*
	* [Bit 15] If this control is 1, executions of ENCLS consult the ENCLS-exiting bitmap to determine whether the instruction
	* causes a VM exit.
	*
	* @see Vol3C[24.6.16(ENCLS-Exiting Bitmap)]
	* @see Vol3C[25.1.3(Instructions That Cause VM Exits Conditionally)]
	*/
	uint64_t enable_encls_exiting                                    : 1;
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_ENCLS_EXITING_BIT            15
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_ENCLS_EXITING_FLAG           0x8000
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_ENCLS_EXITING_MASK           0x01
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_ENCLS_EXITING(_)             (((_) >> 15) & 0x01)

	/**
	* @brief VM-exit when executing RDSEED
	*
	* [Bit 16] This control determines whether executions of RDSEED cause VM exits.
	*/
	uint64_t rdseed_exiting                                          : 1;
#define IA32_VMX_PROCBASED_CTLS2_RDSEED_EXITING_BIT                  16
#define IA32_VMX_PROCBASED_CTLS2_RDSEED_EXITING_FLAG                 0x10000
#define IA32_VMX_PROCBASED_CTLS2_RDSEED_EXITING_MASK                 0x01
#define IA32_VMX_PROCBASED_CTLS2_RDSEED_EXITING(_)                   (((_) >> 16) & 0x01)

	/**
	* @brief Enables page-modification logging
	*
	* [Bit 17] If this control is 1, an access to a guest-physical address that sets an EPT dirty bit first adds an entry to
	* the page-modification log.
	*
	* @see Vol3C[28.2.5(Page-Modification Logging)]
	*/
	uint64_t enable_pml                                              : 1;
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_PML_BIT                      17
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_PML_FLAG                     0x20000
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_PML_MASK                     0x01
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_PML(_)                       (((_) >> 17) & 0x01)

	/**
	* @brief Controls whether EPT-violations may cause
	*
	* [Bit 18] If this control is 1, EPT violations may cause virtualization exceptions (\#VE) instead of VM exits.
	*
	* @see Vol3C[25.5.6(Virtualization Exceptions)]
	*/
	uint64_t ept_violation                                           : 1;
#define IA32_VMX_PROCBASED_CTLS2_EPT_VIOLATION_BIT                   18
#define IA32_VMX_PROCBASED_CTLS2_EPT_VIOLATION_FLAG                  0x40000
#define IA32_VMX_PROCBASED_CTLS2_EPT_VIOLATION_MASK                  0x01
#define IA32_VMX_PROCBASED_CTLS2_EPT_VIOLATION(_)                    (((_) >> 18) & 0x01)

	/**
	* @brief Conceal VMX non-root operation from Intel processor trace (PT)
	*
	* [Bit 19] If this control is 1, Intel Processor Trace suppresses from PIPs an indication that the processor was in VMX
	* non-root operation and omits a VMCS packet from any PSB+ produced in VMX nonroot operation.
	*
	* @see Vol3C[35(INTEL(R) PROCESSOR TRACE)]
	*/
	uint64_t conceal_vmx_from_pt                                     : 1;
#define IA32_VMX_PROCBASED_CTLS2_CONCEAL_VMX_FROM_PT_BIT             19
#define IA32_VMX_PROCBASED_CTLS2_CONCEAL_VMX_FROM_PT_FLAG            0x80000
#define IA32_VMX_PROCBASED_CTLS2_CONCEAL_VMX_FROM_PT_MASK            0x01
#define IA32_VMX_PROCBASED_CTLS2_CONCEAL_VMX_FROM_PT(_)              (((_) >> 19) & 0x01)

	/**
	* @brief Enables XSAVES/XRSTORS instructions
	*
	* [Bit 20] If this control is 0, any execution of XSAVES or XRSTORS causes a \#UD.
	*/
	uint64_t enable_xsaves                                           : 1;
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_XSAVES_BIT                   20
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_XSAVES_FLAG                  0x100000
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_XSAVES_MASK                  0x01
#define IA32_VMX_PROCBASED_CTLS2_ENABLE_XSAVES(_)                    (((_) >> 20) & 0x01)
	uint64_t reserved1                                               : 1;

	/**
	* [Bit 22] If this control is 1, EPT execute permissions are based on whether the linear address being accessed is
	* supervisor mode or user mode.
	*
	* @see Vol3C[28(VMX SUPPORT FOR ADDRESS TRANSLATION)]
	*/
	uint64_t mode_based_execute_control_for_ept                      : 1;
#define IA32_VMX_PROCBASED_CTLS2_MODE_BASED_EXECUTE_CONTROL_FOR_EPT_BIT 22
#define IA32_VMX_PROCBASED_CTLS2_MODE_BASED_EXECUTE_CONTROL_FOR_EPT_FLAG 0x400000
#define IA32_VMX_PROCBASED_CTLS2_MODE_BASED_EXECUTE_CONTROL_FOR_EPT_MASK 0x01
#define IA32_VMX_PROCBASED_CTLS2_MODE_BASED_EXECUTE_CONTROL_FOR_EPT(_) (((_) >> 22) & 0x01)
	uint64_t reserved2                                               : 2;

	/**
	* @brief Use TSC scaling
	*
	* [Bit 25] This control determines whether executions of RDTSC, executions of RDTSCP, and executions of RDMSR that read
	* from the IA32_TIME_STAMP_COUNTER MSR return a value modified by the TSC multiplier field.
	*
	* @see Vol3C[24.6.5(Time-Stamp Counter Offset and Multiplier)]
	* @see Vol3C[25.3(CHANGES TO INSTRUCTION BEHAVIOR IN VMX NON-ROOT OPERATION)]
	*/
	uint64_t use_tsc_scaling                                         : 1;
#define IA32_VMX_PROCBASED_CTLS2_USE_TSC_SCALING_BIT                 25
#define IA32_VMX_PROCBASED_CTLS2_USE_TSC_SCALING_FLAG                0x2000000
#define IA32_VMX_PROCBASED_CTLS2_USE_TSC_SCALING_MASK                0x01
#define IA32_VMX_PROCBASED_CTLS2_USE_TSC_SCALING(_)                  (((_) >> 25) & 0x01)
	uint64_t reserved3                                               : 38;
  };

  uint64_t flags;
} vmx_procbased_ctls2_register;


/**
 * Capability Reporting Register of EPT and VPID.
 *
 * @remarks If ( CPUID.01H:ECX.[5] && IA32_VMX_PROCBASED_CTLS[63] && (IA32_VMX_PROCBASED_CTLS2[33] ||
 *          IA32_VMX_PROCBASED_CTLS2[37]) )
 * @see Vol3D[A.10(VPID AND EPT CAPABILITIES)]
 * @see Vol3D[A.10(VPID and EPT Capabilities)] (reference)
 */
#define IA32_VMX_EPT_VPID_CAP                                        0x0000048C
typedef union
{
  struct
  {
	/**
	* [Bit 0] When set to 1, the processor supports execute-only translations by EPT. This support allows software to
	* configure EPT paging-structure entries in which bits 1:0 are clear (indicating that data accesses are not allowed) and
	* bit 2 is set (indicating that instruction fetches are allowed).
	*/
	uint64_t execute_only_pages                                      : 1;
#define IA32_VMX_EPT_VPID_CAP_EXECUTE_ONLY_PAGES_BIT                 0
#define IA32_VMX_EPT_VPID_CAP_EXECUTE_ONLY_PAGES_FLAG                0x01
#define IA32_VMX_EPT_VPID_CAP_EXECUTE_ONLY_PAGES_MASK                0x01
#define IA32_VMX_EPT_VPID_CAP_EXECUTE_ONLY_PAGES(_)                  (((_) >> 0) & 0x01)
	uint64_t reserved1                                               : 5;

	/**
	* [Bit 6] Indicates support for a page-walk length of 4.
	*/
	uint64_t page_walk_length_4                                      : 1;
#define IA32_VMX_EPT_VPID_CAP_PAGE_WALK_LENGTH_4_BIT                 6
#define IA32_VMX_EPT_VPID_CAP_PAGE_WALK_LENGTH_4_FLAG                0x40
#define IA32_VMX_EPT_VPID_CAP_PAGE_WALK_LENGTH_4_MASK                0x01
#define IA32_VMX_EPT_VPID_CAP_PAGE_WALK_LENGTH_4(_)                  (((_) >> 6) & 0x01)
	uint64_t reserved2                                               : 1;

	/**
	* [Bit 8] When set to 1, the logical processor allows software to configure the EPT paging-structure memory type to be
	* uncacheable (UC).
	*
	* @see Vol3C[24.6.11(Extended-Page-Table Pointer (EPTP))]
	*/
	uint64_t memory_type_uncacheable                                 : 1;
#define IA32_VMX_EPT_VPID_CAP_MEMORY_TYPE_UNCACHEABLE_BIT            8
#define IA32_VMX_EPT_VPID_CAP_MEMORY_TYPE_UNCACHEABLE_FLAG           0x100
#define IA32_VMX_EPT_VPID_CAP_MEMORY_TYPE_UNCACHEABLE_MASK           0x01
#define IA32_VMX_EPT_VPID_CAP_MEMORY_TYPE_UNCACHEABLE(_)             (((_) >> 8) & 0x01)
	uint64_t reserved3                                               : 5;

	/**
	* [Bit 14] When set to 1, the logical processor allows software to configure the EPT paging-structure memory type to be
	* write-back (WB).
	*/
	uint64_t memory_type_write_back                                  : 1;
#define IA32_VMX_EPT_VPID_CAP_MEMORY_TYPE_WRITE_BACK_BIT             14
#define IA32_VMX_EPT_VPID_CAP_MEMORY_TYPE_WRITE_BACK_FLAG            0x4000
#define IA32_VMX_EPT_VPID_CAP_MEMORY_TYPE_WRITE_BACK_MASK            0x01
#define IA32_VMX_EPT_VPID_CAP_MEMORY_TYPE_WRITE_BACK(_)              (((_) >> 14) & 0x01)
	uint64_t reserved4                                               : 1;

	/**
	* [Bit 16] When set to 1, the logical processor allows software to configure a EPT PDE to map a 2-Mbyte page (by setting
	* bit 7 in the EPT PDE).
	*/
	uint64_t pde_2mb_pages                                           : 1;
#define IA32_VMX_EPT_VPID_CAP_PDE_2MB_PAGES_BIT                      16
#define IA32_VMX_EPT_VPID_CAP_PDE_2MB_PAGES_FLAG                     0x10000
#define IA32_VMX_EPT_VPID_CAP_PDE_2MB_PAGES_MASK                     0x01
#define IA32_VMX_EPT_VPID_CAP_PDE_2MB_PAGES(_)                       (((_) >> 16) & 0x01)

	/**
	* [Bit 17] When set to 1, the logical processor allows software to configure a EPT PDPTE to map a 1-Gbyte page (by setting
	* bit 7 in the EPT PDPTE).
	*/
	uint64_t pdpte_1gb_pages                                         : 1;
#define IA32_VMX_EPT_VPID_CAP_PDPTE_1GB_PAGES_BIT                    17
#define IA32_VMX_EPT_VPID_CAP_PDPTE_1GB_PAGES_FLAG                   0x20000
#define IA32_VMX_EPT_VPID_CAP_PDPTE_1GB_PAGES_MASK                   0x01
#define IA32_VMX_EPT_VPID_CAP_PDPTE_1GB_PAGES(_)                     (((_) >> 17) & 0x01)
	uint64_t reserved5                                               : 2;

	/**
	* [Bit 20] If bit 20 is read as 1, the INVEPT instruction is supported.
	*
	* @see Vol3C[30(VMX INSTRUCTION REFERENCE)]
	* @see Vol3C[28.3.3.1(Operations that Invalidate Cached Mappings)]
	*/
	uint64_t invept                                                  : 1;
#define IA32_VMX_EPT_VPID_CAP_INVEPT_BIT                             20
#define IA32_VMX_EPT_VPID_CAP_INVEPT_FLAG                            0x100000
#define IA32_VMX_EPT_VPID_CAP_INVEPT_MASK                            0x01
#define IA32_VMX_EPT_VPID_CAP_INVEPT(_)                              (((_) >> 20) & 0x01)

	/**
	* [Bit 21] When set to 1, accessed and dirty flags for EPT are supported.
	*
	* @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
	*/
	uint64_t ept_accessed_and_dirty_flags                            : 1;
#define IA32_VMX_EPT_VPID_CAP_EPT_ACCESSED_AND_DIRTY_FLAGS_BIT       21
#define IA32_VMX_EPT_VPID_CAP_EPT_ACCESSED_AND_DIRTY_FLAGS_FLAG      0x200000
#define IA32_VMX_EPT_VPID_CAP_EPT_ACCESSED_AND_DIRTY_FLAGS_MASK      0x01
#define IA32_VMX_EPT_VPID_CAP_EPT_ACCESSED_AND_DIRTY_FLAGS(_)        (((_) >> 21) & 0x01)

	/**
	* [Bit 22] When set to 1, the processor reports advanced VM-exit information for EPT violations. This reporting is done
	* only if this bit is read as 1.
	*
	* @see Vol3C[27.2.1(Basic VM-Exit Information)]
	*/
	uint64_t advanced_vmexit_ept_violations_information              : 1;
#define IA32_VMX_EPT_VPID_CAP_ADVANCED_VMEXIT_EPT_VIOLATIONS_INFORMATION_BIT 22
#define IA32_VMX_EPT_VPID_CAP_ADVANCED_VMEXIT_EPT_VIOLATIONS_INFORMATION_FLAG 0x400000
#define IA32_VMX_EPT_VPID_CAP_ADVANCED_VMEXIT_EPT_VIOLATIONS_INFORMATION_MASK 0x01
#define IA32_VMX_EPT_VPID_CAP_ADVANCED_VMEXIT_EPT_VIOLATIONS_INFORMATION(_) (((_) >> 22) & 0x01)
	uint64_t reserved6                                               : 2;

	/**
	* [Bit 25] When set to 1, the single-context INVEPT type is supported.
	*
	* @see Vol3C[30(VMX INSTRUCTION REFERENCE)]
	* @see Vol3C[28.3.3.1(Operations that Invalidate Cached Mappings)]
	*/
	uint64_t invept_single_context                                   : 1;
#define IA32_VMX_EPT_VPID_CAP_INVEPT_SINGLE_CONTEXT_BIT              25
#define IA32_VMX_EPT_VPID_CAP_INVEPT_SINGLE_CONTEXT_FLAG             0x2000000
#define IA32_VMX_EPT_VPID_CAP_INVEPT_SINGLE_CONTEXT_MASK             0x01
#define IA32_VMX_EPT_VPID_CAP_INVEPT_SINGLE_CONTEXT(_)               (((_) >> 25) & 0x01)

	/**
	* [Bit 26] When set to 1, the all-context INVEPT type is supported.
	*
	* @see Vol3C[30(VMX INSTRUCTION REFERENCE)]
	* @see Vol3C[28.3.3.1(Operations that Invalidate Cached Mappings)]
	*/
	uint64_t invept_all_contexts                                     : 1;
#define IA32_VMX_EPT_VPID_CAP_INVEPT_ALL_CONTEXTS_BIT                26
#define IA32_VMX_EPT_VPID_CAP_INVEPT_ALL_CONTEXTS_FLAG               0x4000000
#define IA32_VMX_EPT_VPID_CAP_INVEPT_ALL_CONTEXTS_MASK               0x01
#define IA32_VMX_EPT_VPID_CAP_INVEPT_ALL_CONTEXTS(_)                 (((_) >> 26) & 0x01)
	uint64_t reserved7                                               : 5;

	/**
	* [Bit 32] When set to 1, the INVVPID instruction is supported.
	*/
	uint64_t invvpid                                                 : 1;
#define IA32_VMX_EPT_VPID_CAP_INVVPID_BIT                            32
#define IA32_VMX_EPT_VPID_CAP_INVVPID_FLAG                           0x100000000
#define IA32_VMX_EPT_VPID_CAP_INVVPID_MASK                           0x01
#define IA32_VMX_EPT_VPID_CAP_INVVPID(_)                             (((_) >> 32) & 0x01)
	uint64_t reserved8                                               : 7;

	/**
	* [Bit 40] When set to 1, the individual-address INVVPID type is supported.
	*/
	uint64_t invvpid_individual_address                              : 1;
#define IA32_VMX_EPT_VPID_CAP_INVVPID_INDIVIDUAL_ADDRESS_BIT         40
#define IA32_VMX_EPT_VPID_CAP_INVVPID_INDIVIDUAL_ADDRESS_FLAG        0x10000000000
#define IA32_VMX_EPT_VPID_CAP_INVVPID_INDIVIDUAL_ADDRESS_MASK        0x01
#define IA32_VMX_EPT_VPID_CAP_INVVPID_INDIVIDUAL_ADDRESS(_)          (((_) >> 40) & 0x01)

	/**
	* [Bit 41] When set to 1, the single-context INVVPID type is supported.
	*/
	uint64_t invvpid_single_context                                  : 1;
#define IA32_VMX_EPT_VPID_CAP_INVVPID_SINGLE_CONTEXT_BIT             41
#define IA32_VMX_EPT_VPID_CAP_INVVPID_SINGLE_CONTEXT_FLAG            0x20000000000
#define IA32_VMX_EPT_VPID_CAP_INVVPID_SINGLE_CONTEXT_MASK            0x01
#define IA32_VMX_EPT_VPID_CAP_INVVPID_SINGLE_CONTEXT(_)              (((_) >> 41) & 0x01)

	/**
	* [Bit 42] When set to 1, the all-context INVVPID type is supported.
	*/
	uint64_t invvpid_all_contexts                                    : 1;
#define IA32_VMX_EPT_VPID_CAP_INVVPID_ALL_CONTEXTS_BIT               42
#define IA32_VMX_EPT_VPID_CAP_INVVPID_ALL_CONTEXTS_FLAG              0x40000000000
#define IA32_VMX_EPT_VPID_CAP_INVVPID_ALL_CONTEXTS_MASK              0x01
#define IA32_VMX_EPT_VPID_CAP_INVVPID_ALL_CONTEXTS(_)                (((_) >> 42) & 0x01)

	/**
	* [Bit 43] When set to 1, the single-context-retaining-globals INVVPID type is supported.
	*/
	uint64_t invvpid_single_context_retain_globals                   : 1;
#define IA32_VMX_EPT_VPID_CAP_INVVPID_SINGLE_CONTEXT_RETAIN_GLOBALS_BIT 43
#define IA32_VMX_EPT_VPID_CAP_INVVPID_SINGLE_CONTEXT_RETAIN_GLOBALS_FLAG 0x80000000000
#define IA32_VMX_EPT_VPID_CAP_INVVPID_SINGLE_CONTEXT_RETAIN_GLOBALS_MASK 0x01
#define IA32_VMX_EPT_VPID_CAP_INVVPID_SINGLE_CONTEXT_RETAIN_GLOBALS(_) (((_) >> 43) & 0x01)
	uint64_t reserved9                                               : 20;
  };

  uint64_t flags;
} vmx_ept_vpid_cap_register;

/**
 * @defgroup ia32_vmx_true_ctls \
 *           IA32_VMX_TRUE_(x)_CTLS
 *
 * Capability Reporting Register of Pin-Based VM-Execution Flex Controls, Primary Processor-Based VM-Execution Flex
 * Controls, VM-Exit Flex Controls and VM-Entry Flex Controls.
 *
 * @remarks If ( CPUID.01H:ECX.[5] = 1 && IA32_VMX_BASIC[55] )
 * @see Vol3D[A.3.1(Pin-Based VM-Execution Controls)]
 * @see Vol3D[A.3.2(Primary Processor-Based VM-Execution Controls)]
 * @see Vol3D[A.4(VM-EXIT CONTROLS)]
 * @see Vol3D[A.5(VM-ENTRY CONTROLS)]
 * @see Vol3D[A.3.1(Pin-Based VMExecution Controls)] (reference)
 * @see Vol3D[A.3.2(Primary Processor-Based VM-Execution Controls)] (reference)
 * @see Vol3D[A.4(VM-Exit Controls)] (reference)
 * @see Vol3D[A.5(VM-Entry Controls)] (reference)
 * @{
 */
#define IA32_VMX_TRUE_PINBASED_CTLS                                  0x0000048D
#define IA32_VMX_TRUE_PROCBASED_CTLS                                 0x0000048E
#define IA32_VMX_TRUE_EXIT_CTLS                                      0x0000048F
#define IA32_VMX_TRUE_ENTRY_CTLS                                     0x00000490
typedef union
{
  struct
  {
	/**
	* [Bits 31:0] Indicate the allowed 0-settings of these controls. VM entry allows control X to be 0 if bit X in the MSR is
	* cleared to 0; if bit X in the MSR is set to 1, VM entry fails if control X is 0.
	*/
	uint64_t allowed_0_settings                                      : 32;
#define IA32_VMX_TRUE_CTLS_ALLOWED_0_SETTINGS_BIT                    0
#define IA32_VMX_TRUE_CTLS_ALLOWED_0_SETTINGS_FLAG                   0xFFFFFFFF
#define IA32_VMX_TRUE_CTLS_ALLOWED_0_SETTINGS_MASK                   0xFFFFFFFF
#define IA32_VMX_TRUE_CTLS_ALLOWED_0_SETTINGS(_)                     (((_) >> 0) & 0xFFFFFFFF)

	/**
	* [Bits 63:32] Indicate the allowed 1-settings of these controls. VM entry allows control X to be 1 if bit 32+X in the MSR
	* is set to 1; if bit 32+X in the MSR is cleared to 0, VM entry fails if control X is 1.
	*/
	uint64_t allowed_1_settings                                      : 32;
#define IA32_VMX_TRUE_CTLS_ALLOWED_1_SETTINGS_BIT                    32
#define IA32_VMX_TRUE_CTLS_ALLOWED_1_SETTINGS_FLAG                   0xFFFFFFFF00000000
#define IA32_VMX_TRUE_CTLS_ALLOWED_1_SETTINGS_MASK                   0xFFFFFFFF
#define IA32_VMX_TRUE_CTLS_ALLOWED_1_SETTINGS(_)                     (((_) >> 32) & 0xFFFFFFFF)
  };

  uint64_t flags;
} vmx_true_ctls_register;

/**
 * @}
 */


/**
 * Capability Reporting Register of VMFunction Controls.
 *
 * @remarks If ( CPUID.01H:ECX.[5] = 1 && IA32_VMX_BASIC[55] )
 * @see Vol3D[A.11(VM FUNCTIONS)]
 * @see Vol3D[24.6.14(VM-Function Controls)] (reference)
 */
#define IA32_VMX_VMFUNC                                              0x00000491
typedef union
{
  struct
  {
	/**
	* [Bit 0] The EPTP-switching VM function changes the EPT pointer to a value chosen from the EPTP list.
	*
	* @see Vol3C[25.5.5.3(EPTP Switching)]
	*/
	uint64_t eptp_switching                                          : 1;
#define IA32_VMX_VMFUNC_EPTP_SWITCHING_BIT                           0
#define IA32_VMX_VMFUNC_EPTP_SWITCHING_FLAG                          0x01
#define IA32_VMX_VMFUNC_EPTP_SWITCHING_MASK                          0x01
#define IA32_VMX_VMFUNC_EPTP_SWITCHING(_)                            (((_) >> 0) & 0x01)
	uint64_t reserved1                                               : 63;
  };

  uint64_t flags;
} vmx_vmfunc_register;

/**
 * @defgroup ia32_a_pmc \
 *           IA32_A_PMC(n)
 *
 * Full Width Writable IA32_PMC(n) Alias.
 *
 * @remarks (If CPUID.0AH: EAX[15:8] > 0) && IA32_PERF_CAPABILITIES[13] = 1
 * @{
 */
#define IA32_A_PMC0                                                  0x000004C1
#define IA32_A_PMC1                                                  0x000004C2
#define IA32_A_PMC2                                                  0x000004C3
#define IA32_A_PMC3                                                  0x000004C4
#define IA32_A_PMC4                                                  0x000004C5
#define IA32_A_PMC5                                                  0x000004C6
#define IA32_A_PMC6                                                  0x000004C7
#define IA32_A_PMC7                                                  0x000004C8
/**
 * @}
 */


/**
 * Allows software to signal some MCEs to only a single logical processor in the system.
 *
 * @remarks If IA32_MCG_CAP.LMCE_P = 1
 * @see Vol3B[15.3.1.4(IA32_MCG_EXT_CTL MSR)]
 */
#define IA32_MCG_EXT_CTL                                             0x000004D0
typedef union
{
  struct
  {
	uint64_t lmce_en                                                 : 1;
#define IA32_MCG_EXT_CTL_LMCE_EN_BIT                                 0
#define IA32_MCG_EXT_CTL_LMCE_EN_FLAG                                0x01
#define IA32_MCG_EXT_CTL_LMCE_EN_MASK                                0x01
#define IA32_MCG_EXT_CTL_LMCE_EN(_)                                  (((_) >> 0) & 0x01)
	uint64_t reserved1                                               : 63;
  };

  uint64_t flags;
} mcg_ext_ctl_register;


/**
 * @brief Status and SVN Threshold of SGX Support for ACM <b>(RO)</b>
 *
 * Intel SGX only allows launching ACMs with an Intel SGX SVN that is at the same level or higher than the expected Intel
 * SGX SVN. The expected Intel SGX SVN is specified by BIOS and locked down by the processor on the first successful
 * execution of an Intel SGX instruction that doesn't return an error code. Intel SGX provides interfaces for system
 * software to discover whether a non faulting Intel SGX instruction has been executed, and evaluate the suitability of the
 * Intel SGX SVN value of any ACM that is expected to be launched by the OS or the VMM.
 *
 * @remarks If CPUID.(EAX=07H, ECX=0H): EBX[2] = 1
 * @see Vol3D[41.11.3(Interactions with Authenticated Code Modules (ACMs))] (reference)
 */
#define IA32_SGX_SVN_STATUS                                          0x00000500
typedef union
{
  struct
  {
	/**
	* [Bit 0] - If 1, indicates that a non-faulting Intel SGX instruction has been executed, consequently, launching a
	* properly signed ACM but with Intel SGX SVN value less than the BIOS specified Intel SGX SVN threshold would lead to an
	* TXT shutdown.
	* - If 0, indicates that the processor will allow a properly signed ACM to launch irrespective of the Intel SGX SVN value
	* of the ACM.
	*
	* @see Vol3D[41.11.3(Interactions with Authenticated Code Modules (ACMs))]
	*/
	uint64_t lock                                                    : 1;
#define IA32_SGX_SVN_STATUS_LOCK_BIT                                 0
#define IA32_SGX_SVN_STATUS_LOCK_FLAG                                0x01
#define IA32_SGX_SVN_STATUS_LOCK_MASK                                0x01
#define IA32_SGX_SVN_STATUS_LOCK(_)                                  (((_) >> 0) & 0x01)
	uint64_t reserved1                                               : 15;

	/**
	* @brief Reflects the expected threshold of Intel SGX SVN for the SINIT ACM
	*
	* [Bits 23:16] - If CPUID.01H:ECX.SMX = 1, this field reflects the expected threshold of Intel SGX SVN for the SINIT ACM.
	* - If CPUID.01H:ECX.SMX = 0, this field is reserved (0).
	*
	* @see Vol3D[41.11.3(Interactions with Authenticated Code Modules (ACMs))]
	*/
	uint64_t sgx_svn_sinit                                           : 8;
#define IA32_SGX_SVN_STATUS_SGX_SVN_SINIT_BIT                        16
#define IA32_SGX_SVN_STATUS_SGX_SVN_SINIT_FLAG                       0xFF0000
#define IA32_SGX_SVN_STATUS_SGX_SVN_SINIT_MASK                       0xFF
#define IA32_SGX_SVN_STATUS_SGX_SVN_SINIT(_)                         (((_) >> 16) & 0xFF)
	uint64_t reserved2                                               : 40;
  };

  uint64_t flags;
} sgx_svn_status_register;


/**
 * Trace Output Base Register.
 *
 * @remarks If ( (CPUID.(EAX=07H, ECX=0):EBX[25] = 1) && ( (CPUID.(EAX=14H,ECX=0): ECX[0] = 1) ||
 *          (CPUID.(EAX=14H,ECX=0):ECX[2] = 1) ) )
 * @see Vol3C[35.2.7.7(IA32_RTIT_OUTPUT_BASE MSR)] (reference)
 */
#define IA32_RTIT_OUTPUT_BASE                                        0x00000560
typedef union
{
  struct
  {
	uint64_t reserved1                                               : 7;

	/**
	* @brief Base physical address
	*
	* [Bits 47:7] The base physical address. How this address is used depends on the value of IA32_RTIT_CTL.ToPA:
	* - 0: This is the base physical address of a single, contiguous physical output region. This could be mapped to DRAM or
	* to MMIO, depending on the value. The base address should be aligned with the size of the region, such that none of the
	* 1s in the mask value overlap with 1s in the base address. If the base is not aligned, an operational error will result.
	* - 1: The base physical address of the current ToPA table. The address must be 4K aligned. Writing an address in which
	* bits 11:7 are non-zero will not cause a \#GP, but an operational error will be signaled once TraceEn is set.
	*
	* @see Vol3C[35.2.7.8(IA32_RTIT_OUTPUT_MASK_PTRS MSR)]
	* @see Vol3C[35.3.9(Operational Errors)]
	* @see Vol3C[35.2.6.2(Table of Physical Addresses (ToPA))]
	*/
	uint64_t base_physical_address                                   : 41;
#define IA32_RTIT_OUTPUT_BASE_BASE_PHYSICAL_ADDRESS_BIT              7
#define IA32_RTIT_OUTPUT_BASE_BASE_PHYSICAL_ADDRESS_FLAG             0xFFFFFFFFFF80
#define IA32_RTIT_OUTPUT_BASE_BASE_PHYSICAL_ADDRESS_MASK             0x1FFFFFFFFFF
#define IA32_RTIT_OUTPUT_BASE_BASE_PHYSICAL_ADDRESS(_)               (((_) >> 7) & 0x1FFFFFFFFFF)
	uint64_t reserved2                                               : 16;
  };

  uint64_t flags;
} rtit_output_base_register;


/**
 * Trace Output Mask Pointers Register.
 *
 * @remarks If ( (CPUID.(EAX=07H, ECX=0):EBX[25] = 1) && ( (CPUID.(EAX=14H,ECX=0):ECX[0] = 1) ||
 *          (CPUID.(EAX=14H,ECX=0):ECX[2] = 1) ) )
 * @see Vol3C[35.2.7.8(IA32_RTIT_OUTPUT_MASK_PTRS MSR)] (reference)
 */
#define IA32_RTIT_OUTPUT_MASK_PTRS                                   0x00000561
typedef union
{
  struct
  {
	/**
	* [Bits 6:0] Forced to 1, writes are ignored.
	*/
	uint64_t lower_mask                                              : 7;
#define IA32_RTIT_OUTPUT_MASK_PTRS_LOWER_MASK_BIT                    0
#define IA32_RTIT_OUTPUT_MASK_PTRS_LOWER_MASK_FLAG                   0x7F
#define IA32_RTIT_OUTPUT_MASK_PTRS_LOWER_MASK_MASK                   0x7F
#define IA32_RTIT_OUTPUT_MASK_PTRS_LOWER_MASK(_)                     (((_) >> 0) & 0x7F)

	/**
	* @brief MaskOrTableOffset
	*
	* [Bits 31:7] The use of this field depends on the value of IA32_RTIT_CTL.ToPA:
	* - 0: This field holds bits 31:7 of the mask value for the single, contiguous physical output region. The size of this
	* field indicates that regions can be of size 128B up to 4GB. This value (combined with the lower 7 bits, which are
	* reserved to 1) will be ANDed with the OutputOffset field to determine the next write address. All 1s in this field
	* should be consecutive and starting at bit 7, otherwise the region will not be contiguous, and an operational error will
	* be signaled when TraceEn is set.
	* - 1: This field holds bits 27:3 of the offset pointer into the current ToPA table. This value can be added to the
	* IA32_RTIT_OUTPUT_BASE value to produce a pointer to the current ToPA table entry, which itself is a pointer to the
	* current output region. In this scenario, the lower 7 reserved bits are ignored. This field supports tables up to 256
	* MBytes in size.
	*
	* @see Vol3C[35.3.9(Operational Errors)]
	*/
	uint64_t mask_or_table_offset                                    : 25;
#define IA32_RTIT_OUTPUT_MASK_PTRS_MASK_OR_TABLE_OFFSET_BIT          7
#define IA32_RTIT_OUTPUT_MASK_PTRS_MASK_OR_TABLE_OFFSET_FLAG         0xFFFFFF80
#define IA32_RTIT_OUTPUT_MASK_PTRS_MASK_OR_TABLE_OFFSET_MASK         0x1FFFFFF
#define IA32_RTIT_OUTPUT_MASK_PTRS_MASK_OR_TABLE_OFFSET(_)           (((_) >> 7) & 0x1FFFFFF)

	/**
	* @brief Output Offset
	*
	* [Bits 63:32] The use of this field depends on the value of IA32_RTIT_CTL.ToPA:
	* - 0: This is bits 31:0 of the offset pointer into the single, contiguous physical output region. This value will be
	* added to the IA32_RTIT_OUTPUT_BASE value to form the physical address at which the next byte of packet output data will
	* be written. This value must be less than or equal to the MaskOrTableOffset field, otherwise an operational error will be
	* signaled when TraceEn is set.
	* - 1: This field holds bits 31:0 of the offset pointer into the current ToPA output region. This value will be added to
	* the output region base field, found in the current ToPA table entry, to form the physical address at which the next byte
	* of trace output data will be written. This value must be less than the ToPA entry size, otherwise an operational error
	* will be signaled when TraceEn is set.
	*
	* @see Vol3C[35.3.9(Operational Errors)]
	*/
	uint64_t output_offset                                           : 32;
#define IA32_RTIT_OUTPUT_MASK_PTRS_OUTPUT_OFFSET_BIT                 32
#define IA32_RTIT_OUTPUT_MASK_PTRS_OUTPUT_OFFSET_FLAG                0xFFFFFFFF00000000
#define IA32_RTIT_OUTPUT_MASK_PTRS_OUTPUT_OFFSET_MASK                0xFFFFFFFF
#define IA32_RTIT_OUTPUT_MASK_PTRS_OUTPUT_OFFSET(_)                  (((_) >> 32) & 0xFFFFFFFF)
  };

  uint64_t flags;
} rtit_output_mask_ptrs_register;


/**
 * Trace Control Register.
 *
 * @remarks If (CPUID.(EAX=07H, ECX=0):EBX[25] = 1)
 * @see Vol3C[35.2.7.2(IA32_RTIT_CTL MSR)] (reference)
 */
#define IA32_RTIT_CTL                                                0x00000570
typedef union
{
  struct
  {
	/**
	* @brief TraceEn
	*
	* [Bit 0] If 1, enables tracing; else tracing is disabled.
	* When this bit transitions from 1 to 0, all buffered packets are flushed out of internal buffers. A further store, fence,
	* or architecturally serializing instruction may be required to ensure that packet data can be observed at the trace
	* endpoint.
	* Note that the processor will clear this bit on \#SMI (Section) and warm reset. Other MSR bits of IA32_RTIT_CTL (and
	* other trace configuration MSRs) are not impacted by these events.
	*
	* @see Vol3C[35.2.7.3(Enabling and Disabling Packet Generation with TraceEn)]
	*/
	uint64_t trace_enabled                                           : 1;
#define IA32_RTIT_CTL_TRACE_ENABLED_BIT                              0
#define IA32_RTIT_CTL_TRACE_ENABLED_FLAG                             0x01
#define IA32_RTIT_CTL_TRACE_ENABLED_MASK                             0x01
#define IA32_RTIT_CTL_TRACE_ENABLED(_)                               (((_) >> 0) & 0x01)

	/**
	* @brief CYCEn
	*
	* [Bit 1] - 0: Disables CYC Packet.
	* - 1: Enables CYC Packet.
	*
	* @remarks If CPUID.(EAX=14H, ECX=0):EBX.CPSB_CAM[bit 1] = 0
	* @see Vol3C[35.4.2.14(Cycle Count (CYC) Packet)]
	*/
	uint64_t cyc_enabled                                             : 1;
#define IA32_RTIT_CTL_CYC_ENABLED_BIT                                1
#define IA32_RTIT_CTL_CYC_ENABLED_FLAG                               0x02
#define IA32_RTIT_CTL_CYC_ENABLED_MASK                               0x01
#define IA32_RTIT_CTL_CYC_ENABLED(_)                                 (((_) >> 1) & 0x01)

	/**
	* @brief OS
	*
	* [Bit 2] - 0: Packet generation is disabled when CPL = 0.
	* - 1: Packet generation may be enabled when CPL = 0.
	*/
	uint64_t os                                                      : 1;
#define IA32_RTIT_CTL_OS_BIT                                         2
#define IA32_RTIT_CTL_OS_FLAG                                        0x04
#define IA32_RTIT_CTL_OS_MASK                                        0x01
#define IA32_RTIT_CTL_OS(_)                                          (((_) >> 2) & 0x01)

	/**
	* @brief User
	*
	* [Bit 3] - 0: Packet generation is disabled when CPL > 0.
	* - 1: Packet generation may be enabled when CPL > 0.
	*/
	uint64_t user                                                    : 1;
#define IA32_RTIT_CTL_USER_BIT                                       3
#define IA32_RTIT_CTL_USER_FLAG                                      0x08
#define IA32_RTIT_CTL_USER_MASK                                      0x01
#define IA32_RTIT_CTL_USER(_)                                        (((_) >> 3) & 0x01)

	/**
	* @brief PwrEvtEn
	*
	* [Bit 4] - 0: Power Event Trace packets are disabled.
	* - 1: Power Event Trace packets are enabled.
	*
	* @see Vol3C[35.2.3(Power Event Tracing)]
	*/
	uint64_t power_event_trace_enabled                               : 1;
#define IA32_RTIT_CTL_POWER_EVENT_TRACE_ENABLED_BIT                  4
#define IA32_RTIT_CTL_POWER_EVENT_TRACE_ENABLED_FLAG                 0x10
#define IA32_RTIT_CTL_POWER_EVENT_TRACE_ENABLED_MASK                 0x01
#define IA32_RTIT_CTL_POWER_EVENT_TRACE_ENABLED(_)                   (((_) >> 4) & 0x01)

	/**
	* @brief FUPonPTW
	*
	* [Bit 5] - 0: PTW packets are not followed by FUPs.
	* - 1: PTW packets are followed by FUPs.
	*/
	uint64_t fup_on_ptw                                              : 1;
#define IA32_RTIT_CTL_FUP_ON_PTW_BIT                                 5
#define IA32_RTIT_CTL_FUP_ON_PTW_FLAG                                0x20
#define IA32_RTIT_CTL_FUP_ON_PTW_MASK                                0x01
#define IA32_RTIT_CTL_FUP_ON_PTW(_)                                  (((_) >> 5) & 0x01)

	/**
	* @brief FabricEn
	*
	* [Bit 6] - 0: Trace output is directed to the memory subsystem, mechanism depends on IA32_RTIT_CTL.ToPA.
	* - 1: Trace output is directed to the trace transport subsystem, IA32_RTIT_CTL.ToPA is ignored.
	*
	* @remarks If (CPUID.(EAX=07H, ECX=0):ECX[3] = 1) Reserved if CPUID.(EAX=14H, ECX=0):ECX[bit 3] = 0
	*/
	uint64_t fabric_enabled                                          : 1;
#define IA32_RTIT_CTL_FABRIC_ENABLED_BIT                             6
#define IA32_RTIT_CTL_FABRIC_ENABLED_FLAG                            0x40
#define IA32_RTIT_CTL_FABRIC_ENABLED_MASK                            0x01
#define IA32_RTIT_CTL_FABRIC_ENABLED(_)                              (((_) >> 6) & 0x01)

	/**
	* @brief CR3 filter
	*
	* [Bit 7] - 0: Disables CR3 filtering.
	* - 1: Enables CR3 filtering.
	*/
	uint64_t cr3_filter                                              : 1;
#define IA32_RTIT_CTL_CR3_FILTER_BIT                                 7
#define IA32_RTIT_CTL_CR3_FILTER_FLAG                                0x80
#define IA32_RTIT_CTL_CR3_FILTER_MASK                                0x01
#define IA32_RTIT_CTL_CR3_FILTER(_)                                  (((_) >> 7) & 0x01)

	/**
	* @brief ToPA
	*
	* [Bit 8] - 0: Single-range output scheme enabled.
	* - 1: ToPA output scheme enabled.
	*
	* @remarks 0: If CPUID.(EAX=14H, ECX=0):ECX.SNGLRGNOUT[bit 2] = 1 and IA32_RTIT_CTL.FabricEn=0 1: If CPUID.(EAX=14H,
	*          ECX=0):ECX.TOPA[bit 0] = 1, and IA32_RTIT_CTL.FabricEn=0
	*          WRMSR to IA32_RTIT_CTL that sets TraceEn but clears this bit and FabricEn would cause \#GP: If CPUID.(EAX=14H,
	*          ECX=0):ECX.SNGLRGNOUT[bit 2] = 0 WRMSR to IA32_RTIT_CTL that sets this bit causes \#GP: If CPUID.(EAX=14H,
	*          ECX=0):ECX.TOPA[bit 0] = 0
	* @see Vol3C[35.2.6.2(Table of Physical Addresses (ToPA))]
	*/
	uint64_t topa                                                    : 1;
#define IA32_RTIT_CTL_TOPA_BIT                                       8
#define IA32_RTIT_CTL_TOPA_FLAG                                      0x100
#define IA32_RTIT_CTL_TOPA_MASK                                      0x01
#define IA32_RTIT_CTL_TOPA(_)                                        (((_) >> 8) & 0x01)

	/**
	* @brief MTCEn
	*
	* [Bit 9] - 0: Disables MTC Packet.
	* - 1: Enables MTC Packet.
	*
	* @remarks If (CPUID.(EAX=07H, ECX=0):EBX[3] = 1) Reserved if CPUID.(EAX=14H, ECX=0):EBX.MTC[bit 3] = 0
	* @see Vol3C[35.4.2.16(Overflow (OVF) Packet)]
	*/
	uint64_t mtc_enabled                                             : 1;
#define IA32_RTIT_CTL_MTC_ENABLED_BIT                                9
#define IA32_RTIT_CTL_MTC_ENABLED_FLAG                               0x200
#define IA32_RTIT_CTL_MTC_ENABLED_MASK                               0x01
#define IA32_RTIT_CTL_MTC_ENABLED(_)                                 (((_) >> 9) & 0x01)

	/**
	* @brief TSCEn
	*
	* [Bit 10] - 0: Disable TSC packets.
	* - 1: Enable TSC packets.
	*
	* @see Vol3C[35.4.2.11(Timestamp Counter (TSC) Packet)]
	*/
	uint64_t tsc_enabled                                             : 1;
#define IA32_RTIT_CTL_TSC_ENABLED_BIT                                10
#define IA32_RTIT_CTL_TSC_ENABLED_FLAG                               0x400
#define IA32_RTIT_CTL_TSC_ENABLED_MASK                               0x01
#define IA32_RTIT_CTL_TSC_ENABLED(_)                                 (((_) >> 10) & 0x01)

	/**
	* @brief DisRETC
	*
	* [Bit 11] - 0: Enable RET compression.
	* - 1: Disable RET compression.
	*
	* @see Vol3C[35.2.1.2(Indirect Transfer COFI)]
	*/
	uint64_t ret_compression_disabled                                : 1;
#define IA32_RTIT_CTL_RET_COMPRESSION_DISABLED_BIT                   11
#define IA32_RTIT_CTL_RET_COMPRESSION_DISABLED_FLAG                  0x800
#define IA32_RTIT_CTL_RET_COMPRESSION_DISABLED_MASK                  0x01
#define IA32_RTIT_CTL_RET_COMPRESSION_DISABLED(_)                    (((_) >> 11) & 0x01)

	/**
	* @brief PTWEn
	*
	* [Bit 12] - 0: PTWRITE packet generation disabled.
	* - 1: PTWRITE packet generation enabled.
	*/
	uint64_t ptw_enabled                                             : 1;
#define IA32_RTIT_CTL_PTW_ENABLED_BIT                                12
#define IA32_RTIT_CTL_PTW_ENABLED_FLAG                               0x1000
#define IA32_RTIT_CTL_PTW_ENABLED_MASK                               0x01
#define IA32_RTIT_CTL_PTW_ENABLED(_)                                 (((_) >> 12) & 0x01)

	/**
	* @brief BranchEn
	*
	* [Bit 13] - 0: Disable COFI-based packets.
	* - 1: Enable COFI-based packets: FUP, TIP, TIP.PGE, TIP.PGD, TNT, MODE.Exec, MODE.TSX.
	*
	* @see Vol3C[35.2.5.4(Branch Enable (BranchEn))]
	*/
	uint64_t branch_enabled                                          : 1;
#define IA32_RTIT_CTL_BRANCH_ENABLED_BIT                             13
#define IA32_RTIT_CTL_BRANCH_ENABLED_FLAG                            0x2000
#define IA32_RTIT_CTL_BRANCH_ENABLED_MASK                            0x01
#define IA32_RTIT_CTL_BRANCH_ENABLED(_)                              (((_) >> 13) & 0x01)

	/**
	* @brief MTCFreq
	*
	* [Bits 17:14] Defines MTC packet Frequency, which is based on the core crystal clock, or Always Running Timer (ART). MTC
	* will be sent each time the selected ART bit toggles. The following Encodings are defined:
	* 0: ART(0), 1: ART(1), 2: ART(2), 3: ART(3), 4: ART(4), 5: ART(5), 6: ART(6), 7: ART(7), 8: ART(8), 9: ART(9), 10:
	* ART(10), 11: ART(11), 12: ART(12), 13: ART(13), 14: ART(14), 15: ART(15)
	*
	* @remarks If (CPUID.(EAX=07H, ECX=0):EBX[3] = 1) Reserved if CPUID.(EAX=14H, ECX=0):EBX.MTC[bit 3] = 0
	* @see Vol3C[35.3.1(Detection of Intel Processor Trace and Capability Enumeration)]
	*/
	uint64_t mtc_frequency                                           : 4;
#define IA32_RTIT_CTL_MTC_FREQUENCY_BIT                              14
#define IA32_RTIT_CTL_MTC_FREQUENCY_FLAG                             0x3C000
#define IA32_RTIT_CTL_MTC_FREQUENCY_MASK                             0x0F
#define IA32_RTIT_CTL_MTC_FREQUENCY(_)                               (((_) >> 14) & 0x0F)
	uint64_t reserved1                                               : 1;

	/**
	* @brief CYCThresh
	*
	* [Bits 22:19] CYC packet threshold. CYC packets will be sent with the first eligible packet after N cycles have passed
	* since the last CYC packet. If CycThresh is 0 then N=0, otherwise N is defined as 2(CycThresh-1). The following Encodings
	* are defined:
	* 0: 0, 1: 1, 2: 2, 3: 4, 4: 8, 5: 16, 6: 32, 7: 64, 8: 128, 9: 256, 10: 512, 11: 1024, 12: 2048, 13: 4096, 14: 8192, 15:
	* 16384
	*
	* @remarks If (CPUID.(EAX=07H, ECX=0):EBX[1] = 1) Reserved if CPUID.(EAX=14H, ECX=0):EBX.CPSB_CAM[bit 1] = 0
	* @see Vol3C[35.3.6(Cycle-Accurate Mode)]
	* @see Vol3C[35.3.1(Detection of Intel Processor Trace and Capability Enumeration)]
	*/
	uint64_t cyc_threshold                                           : 4;
#define IA32_RTIT_CTL_CYC_THRESHOLD_BIT                              19
#define IA32_RTIT_CTL_CYC_THRESHOLD_FLAG                             0x780000
#define IA32_RTIT_CTL_CYC_THRESHOLD_MASK                             0x0F
#define IA32_RTIT_CTL_CYC_THRESHOLD(_)                               (((_) >> 19) & 0x0F)
	uint64_t reserved2                                               : 1;

	/**
	* @brief PSBFreq
	*
	* [Bits 27:24] Indicates the frequency of PSB packets. PSB packet frequency is based on the number of Intel PT packet
	* bytes output, so this field allows the user to determine the increment of IA32_IA32_RTIT_STATUS.PacketByteCnt that
	* should cause a PSB to be generated. Note that PSB insertion is not precise, but the average output bytes per PSB should
	* approximate the SW selected period. The following Encodings are defined:
	* 0: 2K, 1: 4K, 2: 8K, 3: 16K, 4: 32K, 5: 64K, 6: 128K, 7: 256K, 8: 512K, 9: 1M, 10: 2M, 11: 4M, 12: 8M, 13: 16M, 14: 32M,
	* 15: 64M
	*
	* @remarks If (CPUID.(EAX=07H, ECX=0):EBX[1] = 1) Reserved if CPUID.(EAX=14H, ECX=0):EBX.CPSB_CAM[bit 1] = 0
	* @see Vol3C[35.3.1(Detection of Intel Processor Trace and Capability Enumeration)]
	*/
	uint64_t psb_frequency                                           : 4;
#define IA32_RTIT_CTL_PSB_FREQUENCY_BIT                              24
#define IA32_RTIT_CTL_PSB_FREQUENCY_FLAG                             0xF000000
#define IA32_RTIT_CTL_PSB_FREQUENCY_MASK                             0x0F
#define IA32_RTIT_CTL_PSB_FREQUENCY(_)                               (((_) >> 24) & 0x0F)
	uint64_t reserved3                                               : 4;

	/**
	* @brief ADDR0_CFG
	*
	* [Bits 35:32] Configures the base/limit register pair IA32_RTIT_ADDR0_A/B based on the following encodings:
	* - 0: ADDR0 range unused.
	* - 1: The [IA32_RTIT_ADDR0_A..IA32_RTIT_ADDR0_B] range defines a FilterEn range. FilterEn will only be set when the IP is
	* within this range, though other FilterEn ranges can additionally be used.
	* - 2: The [IA32_RTIT_ADDR0_A..IA32_RTIT_ADDR0_B] range defines a TraceStop range. TraceStop will be asserted if code
	* branches into this range.
	* - 3..15: Reserved (\#GP).
	*
	* @remarks If (CPUID.(EAX=07H, ECX=1):EAX[2:0] > 0) Reserved if CPUID.(EAX=14H, ECX=1):EBX.RANGECNT[2:0] >= 0
	* @see Vol3C[35.2.4.3(Filtering by IP)]
	* @see Vol3C[35.4.2.10(Core:Bus Ratio (CBR) Packet)]
	*/
	uint64_t addr0_cfg                                               : 4;
#define IA32_RTIT_CTL_ADDR0_CFG_BIT                                  32
#define IA32_RTIT_CTL_ADDR0_CFG_FLAG                                 0xF00000000
#define IA32_RTIT_CTL_ADDR0_CFG_MASK                                 0x0F
#define IA32_RTIT_CTL_ADDR0_CFG(_)                                   (((_) >> 32) & 0x0F)

	/**
	* @brief ADDR1_CFG
	*
	* [Bits 39:36] Configures the base/limit register pair IA32_RTIT_ADDR1_A/B based on the following encodings:
	* - 0: ADDR1 range unused.
	* - 1: The [IA32_RTIT_ADDR1_A..IA32_RTIT_ADDR1_B] range defines a FilterEn range. FilterEn will only be set when the IP is
	* within this range, though other FilterEn ranges can additionally be used.
	* - 2: The [IA32_RTIT_ADDR1_A..IA32_RTIT_ADDR1_B] range defines a TraceStop range. TraceStop will be asserted if code
	* branches into this range.
	* - 3..15: Reserved (\#GP).
	*
	* @remarks If (CPUID.(EAX=07H, ECX=1):EAX[2:0] > 1) Reserved if CPUID.(EAX=14H, ECX=1):EBX.RANGECNT[2:0] < 2
	* @see Vol3C[35.2.4.3(Filtering by IP)]
	* @see Vol3C[35.4.2.10(Core:Bus Ratio (CBR) Packet)]
	*/
	uint64_t addr1_cfg                                               : 4;
#define IA32_RTIT_CTL_ADDR1_CFG_BIT                                  36
#define IA32_RTIT_CTL_ADDR1_CFG_FLAG                                 0xF000000000
#define IA32_RTIT_CTL_ADDR1_CFG_MASK                                 0x0F
#define IA32_RTIT_CTL_ADDR1_CFG(_)                                   (((_) >> 36) & 0x0F)

	/**
	* @brief ADDR2_CFG
	*
	* [Bits 43:40] Configures the base/limit register pair IA32_RTIT_ADDR2_A/B based on the following encodings:
	* - 0: ADDR2 range unused.
	* - 1: The [IA32_RTIT_ADDR2_A..IA32_RTIT_ADDR2_B] range defines a FilterEn range. FilterEn will only be set when the IP is
	* within this range, though other FilterEn ranges can additionally be used.
	* - 2: The [IA32_RTIT_ADDR2_A..IA32_RTIT_ADDR2_B] range defines a TraceStop range. TraceStop will be asserted if code
	* branches into this range.
	* - 3..15: Reserved (\#GP).
	*
	* @remarks If (CPUID.(EAX=07H, ECX=1):EAX[2:0] > 2) Reserved if CPUID.(EAX=14H, ECX=1):EBX.RANGECNT[2:0] < 3
	* @see Vol3C[35.2.4.3(Filtering by IP)]
	* @see Vol3C[35.4.2.10(Core:Bus Ratio (CBR) Packet)]
	*/
	uint64_t addr2_cfg                                               : 4;
#define IA32_RTIT_CTL_ADDR2_CFG_BIT                                  40
#define IA32_RTIT_CTL_ADDR2_CFG_FLAG                                 0xF0000000000
#define IA32_RTIT_CTL_ADDR2_CFG_MASK                                 0x0F
#define IA32_RTIT_CTL_ADDR2_CFG(_)                                   (((_) >> 40) & 0x0F)

	/**
	* @brief ADDR3_CFG
	*
	* [Bits 47:44] Configures the base/limit register pair IA32_RTIT_ADDR3_A/B based on the following encodings:
	* - 0: ADDR3 range unused.
	* - 1: The [IA32_RTIT_ADDR3_A..IA32_RTIT_ADDR3_B] range defines a FilterEn range. FilterEn will only be set when the IP is
	* within this range, though other FilterEn ranges can additionally be used.
	* - 2: The [IA32_RTIT_ADDR3_A..IA32_RTIT_ADDR3_B] range defines a TraceStop range. TraceStop will be asserted if code
	* branches into this range.
	* - 3..15: Reserved (\#GP).
	*
	* @remarks If (CPUID.(EAX=07H, ECX=1):EAX[2:0] > 3) Reserved if CPUID.(EAX=14H, ECX=1):EBX.RANGECNT[2:0] < 4
	* @see Vol3C[35.2.4.3(Filtering by IP)]
	* @see Vol3C[35.4.2.10(Core:Bus Ratio (CBR) Packet)]
	*/
	uint64_t addr3_cfg                                               : 4;
#define IA32_RTIT_CTL_ADDR3_CFG_BIT                                  44
#define IA32_RTIT_CTL_ADDR3_CFG_FLAG                                 0xF00000000000
#define IA32_RTIT_CTL_ADDR3_CFG_MASK                                 0x0F
#define IA32_RTIT_CTL_ADDR3_CFG(_)                                   (((_) >> 44) & 0x0F)
	uint64_t reserved4                                               : 8;

	/**
	* @brief InjectPsbPmiOnEnable
	*
	* [Bit 56] - 1: Enables use of IA32_RTIT_STATUS bits PendPSB[6] and PendTopaPMI[7].
	* - 0: IA32_RTIT_STATUS bits 6 and 7 are ignored.
	*
	* @remarks Reserved if CPUID.(EAX=14H, ECX=0):EBX.INJECTPSBPMI[6] = 0
	* @see Vol3C[35.2.7.4(IA32_RTIT_STATUS MSR)]
	*/
	uint64_t inject_psb_pmi_on_enable                                : 1;
#define IA32_RTIT_CTL_INJECT_PSB_PMI_ON_ENABLE_BIT                   56
#define IA32_RTIT_CTL_INJECT_PSB_PMI_ON_ENABLE_FLAG                  0x100000000000000
#define IA32_RTIT_CTL_INJECT_PSB_PMI_ON_ENABLE_MASK                  0x01
#define IA32_RTIT_CTL_INJECT_PSB_PMI_ON_ENABLE(_)                    (((_) >> 56) & 0x01)
	uint64_t reserved5                                               : 7;
  };

  uint64_t flags;
} rtit_ctl_register;


/**
 * Tracing Status Register.
 *
 * @remarks If (CPUID.(EAX=07H, ECX=0):EBX[25] = 1)
 */
#define IA32_RTIT_STATUS                                             0x00000571
typedef union
{
  struct
  {
	/**
	* @brief FilterEn (writes ignored)
	*
	* [Bit 0] This bit is written by the processor, and indicates that tracing is allowed for the current IP. Writes are
	* ignored.
	*
	* @remarks If (CPUID.(EAX=07H, ECX=0):EBX[2] = 1)
	* @see Vol3C[35.2.5.5(Filter Enable (FilterEn))]
	*/
	uint64_t filter_enabled                                          : 1;
#define IA32_RTIT_STATUS_FILTER_ENABLED_BIT                          0
#define IA32_RTIT_STATUS_FILTER_ENABLED_FLAG                         0x01
#define IA32_RTIT_STATUS_FILTER_ENABLED_MASK                         0x01
#define IA32_RTIT_STATUS_FILTER_ENABLED(_)                           (((_) >> 0) & 0x01)

	/**
	* @brief ContexEn (writes ignored)
	*
	* [Bit 1] The processor sets this bit to indicate that tracing is allowed for the current context. Writes are ignored.
	*
	* @see Vol3C[35.2.5.3(Context Enable (ContextEn))]
	*/
	uint64_t context_enabled                                         : 1;
#define IA32_RTIT_STATUS_CONTEXT_ENABLED_BIT                         1
#define IA32_RTIT_STATUS_CONTEXT_ENABLED_FLAG                        0x02
#define IA32_RTIT_STATUS_CONTEXT_ENABLED_MASK                        0x01
#define IA32_RTIT_STATUS_CONTEXT_ENABLED(_)                          (((_) >> 1) & 0x01)

	/**
	* @brief TriggerEn (writes ignored)
	*
	* [Bit 2] The processor sets this bit to indicate that tracing is enabled. Writes are ignored.
	*
	* @see Vol3C[35.2.5.2(Trigger Enable (TriggerEn))]
	*/
	uint64_t trigger_enabled                                         : 1;
#define IA32_RTIT_STATUS_TRIGGER_ENABLED_BIT                         2
#define IA32_RTIT_STATUS_TRIGGER_ENABLED_FLAG                        0x04
#define IA32_RTIT_STATUS_TRIGGER_ENABLED_MASK                        0x01
#define IA32_RTIT_STATUS_TRIGGER_ENABLED(_)                          (((_) >> 2) & 0x01)
	uint64_t reserved1                                               : 1;

	/**
	* @brief Error
	*
	* [Bit 4] The processor sets this bit to indicate that an operational error has been encountered. When this bit is set,
	* TriggerEn is cleared to 0 and packet generation is disabled.
	* When TraceEn is cleared, software can write this bit. Once it is set, only software can clear it. It is not recommended
	* that software ever set this bit, except in cases where it is restoring a prior saved state.
	*
	* @see Vol3C[35.2.6.2(Table of Physical Addresses (ToPA) | ToPA Errors)]
	*/
	uint64_t error                                                   : 1;
#define IA32_RTIT_STATUS_ERROR_BIT                                   4
#define IA32_RTIT_STATUS_ERROR_FLAG                                  0x10
#define IA32_RTIT_STATUS_ERROR_MASK                                  0x01
#define IA32_RTIT_STATUS_ERROR(_)                                    (((_) >> 4) & 0x01)

	/**
	* @brief Stopped
	*
	* [Bit 5] The processor sets this bit to indicate that a ToPA Stop condition has been encountered. When this bit is set,
	* TriggerEn is cleared to 0 and packet generation is disabled.
	* When TraceEn is cleared, software can write this bit. Once it is set, only software can clear it. It is not recommended
	* that software ever set this bit, except in cases where it is restoring a prior saved state.
	*
	* @see Vol3C[35.2.6.2(Table of Physical Addresses (ToPA) | ToPA STOP)]
	*/
	uint64_t stopped                                                 : 1;
#define IA32_RTIT_STATUS_STOPPED_BIT                                 5
#define IA32_RTIT_STATUS_STOPPED_FLAG                                0x20
#define IA32_RTIT_STATUS_STOPPED_MASK                                0x01
#define IA32_RTIT_STATUS_STOPPED(_)                                  (((_) >> 5) & 0x01)

	/**
	* @brief Pend PSB
	*
	* [Bit 6] If IA32_RTIT_CTL.InjectPsbPmiOnEnable[56] = 1, the processor sets this bit when the threshold for a PSB+ to be
	* inserted has been reached. The processor will clear this bit when the PSB+ has been inserted into the trace. If PendPSB
	* = 1 and InjectPsbPmiOnEnable = 1 when IA32_RTIT_CTL.TraceEn[0] transitions from 0 to 1, a PSB+ will be inserted into the
	* trace.
	*
	* @remarks If CPUID.(EAX=14H, ECX=0):EBX.INJECTPSBPMI[6] = 1
	*/
	uint64_t pend_psb                                                : 1;
#define IA32_RTIT_STATUS_PEND_PSB_BIT                                6
#define IA32_RTIT_STATUS_PEND_PSB_FLAG                               0x40
#define IA32_RTIT_STATUS_PEND_PSB_MASK                               0x01
#define IA32_RTIT_STATUS_PEND_PSB(_)                                 (((_) >> 6) & 0x01)

	/**
	* @brief Pend ToPA PMI
	*
	* [Bit 7] If IA32_RTIT_CTL.InjectPsbPmiOnEnable[56] = 1, the processor sets this bit when the threshold for a ToPA PMI to
	* be inserted has been reached. Software should clear this bit once the ToPA PMI has been handled. If PendTopaPMI = 1 and
	* InjectPsbPmiOnEnable = 1 when IA32_RTIT_CTL.TraceEn[0] transitions from 0 to 1, a PMI will be pended.
	*
	* @remarks If CPUID.(EAX=14H, ECX=0):EBX.INJECTPSBPMI[6] = 1
	* @see Vol3C[35.2.6.2(Table of Physical Addresses (ToPA) | ToPA PMI)]
	*/
	uint64_t pend_topa_pmi                                           : 1;
#define IA32_RTIT_STATUS_PEND_TOPA_PMI_BIT                           7
#define IA32_RTIT_STATUS_PEND_TOPA_PMI_FLAG                          0x80
#define IA32_RTIT_STATUS_PEND_TOPA_PMI_MASK                          0x01
#define IA32_RTIT_STATUS_PEND_TOPA_PMI(_)                            (((_) >> 7) & 0x01)
	uint64_t reserved2                                               : 24;

	/**
	* @brief PacketByteCnt
	*
	* [Bits 48:32] This field is written by the processor, and holds a count of packet bytes that have been sent out. The
	* processor also uses this field to determine when the next PSB packet should be inserted. Note that the processor may
	* clear or modify this field at any time while IA32_RTIT_CTL.TraceEn=1. It will have a stable value when
	* IA32_RTIT_CTL.TraceEn=0.
	*
	* @remarks If (CPUID.(EAX=07H, ECX=0):EBX[1] > 3)
	* @see Vol3C[35.4.2.17(Packet Stream Boundary (PSB) Packet)]
	*/
	uint64_t packet_byte_count                                       : 17;
#define IA32_RTIT_STATUS_PACKET_BYTE_COUNT_BIT                       32
#define IA32_RTIT_STATUS_PACKET_BYTE_COUNT_FLAG                      0x1FFFF00000000
#define IA32_RTIT_STATUS_PACKET_BYTE_COUNT_MASK                      0x1FFFF
#define IA32_RTIT_STATUS_PACKET_BYTE_COUNT(_)                        (((_) >> 32) & 0x1FFFF)
	uint64_t reserved3                                               : 15;
  };

  uint64_t flags;
} rtit_status_register;


/**
 * @brief Trace Filter CR3 Match Register <b>(R/W)</b>
 *
 * The IA32_RTIT_CR3_MATCH register is compared against CR3 when IA32_RTIT_CTL.CR3Filter is 1. Bits 63:5 hold the CR3
 * address value to match, bits 4:0 are reserved to 0.
 *
 * @remarks If (CPUID.(EAX=07H, ECX=0):EBX[25] = 1)
 * @see Vol3C[35.2.4.2(Filtering by CR3)]
 * @see Vol3C[35.2.7.6(IA32_RTIT_CR3_MATCH MSR)] (reference)
 */
#define IA32_RTIT_CR3_MATCH                                          0x00000572
typedef union
{
  struct
  {
	uint64_t reserved1                                               : 5;

	/**
	* [Bits 63:5] CR3[63:5] value to match.
	*/
	uint64_t cr3_value_to_match                                      : 59;
#define IA32_RTIT_CR3_MATCH_CR3_VALUE_TO_MATCH_BIT                   5
#define IA32_RTIT_CR3_MATCH_CR3_VALUE_TO_MATCH_FLAG                  0xFFFFFFFFFFFFFFE0
#define IA32_RTIT_CR3_MATCH_CR3_VALUE_TO_MATCH_MASK                  0x7FFFFFFFFFFFFFF
#define IA32_RTIT_CR3_MATCH_CR3_VALUE_TO_MATCH(_)                    (((_) >> 5) & 0x7FFFFFFFFFFFFFF)
  };

  uint64_t flags;
} rtit_cr3_match_register;

/**
 * @defgroup ia32_rtit_addr \
 *           IA32_RTIT_ADDR(x)
 *
 * The role of the IA32_RTIT_ADDRn_A/B register pairs, for each n, is determined by the corresponding ADDRn_CFG fields in
 * IA32_RTIT_CTL. The number of these register pairs is enumerated by CPUID.(EAX=14H, ECX=1):EAX.RANGECNT[2:0].
 *
 * @remarks If (CPUID.(EAX=07H, ECX=1):EAX[2:0] > n)
 * @see Vol3C[35.2.7.2(IA32_RTIT_CTL MSR)]
 * @see Vol3C[35.2.7.5(IA32_RTIT_ADDRn_A and IA32_RTIT_ADDRn_B MSRs)] (reference)
 * @{
 */
/**
 * @defgroup ia32_rtit_addr_a \
 *           IA32_RTIT_ADDR(n)_A
 *
 * Region n Start Address.
 *
 * @remarks If (CPUID.(EAX=07H, ECX=1):EAX[2:0] > n)
 * @{
 */
#define IA32_RTIT_ADDR0_A                                            0x00000580
#define IA32_RTIT_ADDR1_A                                            0x00000582
#define IA32_RTIT_ADDR2_A                                            0x00000584
#define IA32_RTIT_ADDR3_A                                            0x00000586
/**
 * @}
 */

/**
 * @defgroup ia32_rtit_addr_b \
 *           IA32_RTIT_ADDR(n)_B
 *
 * Region n End Address.
 *
 * @remarks If (CPUID.(EAX=07H, ECX=1):EAX[2:0] > n)
 * @{
 */
#define IA32_RTIT_ADDR0_B                                            0x00000581
#define IA32_RTIT_ADDR1_B                                            0x00000583
#define IA32_RTIT_ADDR2_B                                            0x00000585
#define IA32_RTIT_ADDR3_B                                            0x00000587
/**
 * @}
 */

typedef union
{
  struct
  {
	/**
	* [Bits 47:0] Virtual Address.
	*/
	uint64_t virtual_address                                         : 48;
#define IA32_RTIT_ADDR_VIRTUAL_ADDRESS_BIT                           0
#define IA32_RTIT_ADDR_VIRTUAL_ADDRESS_FLAG                          0xFFFFFFFFFFFF
#define IA32_RTIT_ADDR_VIRTUAL_ADDRESS_MASK                          0xFFFFFFFFFFFF
#define IA32_RTIT_ADDR_VIRTUAL_ADDRESS(_)                            (((_) >> 0) & 0xFFFFFFFFFFFF)

	/**
	* [Bits 63:48] SignExt_VA.
	*/
	uint64_t sign_ext_va                                             : 16;
#define IA32_RTIT_ADDR_SIGN_EXT_VA_BIT                               48
#define IA32_RTIT_ADDR_SIGN_EXT_VA_FLAG                              0xFFFF000000000000
#define IA32_RTIT_ADDR_SIGN_EXT_VA_MASK                              0xFFFF
#define IA32_RTIT_ADDR_SIGN_EXT_VA(_)                                (((_) >> 48) & 0xFFFF)
  };

  uint64_t flags;
} rtit_addr_register;

/**
 * @}
 */


/**
 * DS Save Area. Points to the linear address of the first byte of the DS buffer management area, which is used to manage
 * the BTS and PEBS buffers.
 * Returns:
 * - [63:0] The linear address of the first byte of the DS buffer management area, if IA-32e mode is active.
 * - [31:0] The linear address of the first byte of the DS buffer management area, if not in IA-32e mode.
 * - [63:32] Reserved if not in IA-32e mode.
 *
 * @remarks If CPUID.01H:EDX.DS[21] = 1
 * @see Vol3B[18.6.3.4(Debug Store (DS) Mechanism)]
 */
#define IA32_DS_AREA                                                 0x00000600

/**
 * TSC Target of Local APIC's TSC Deadline Mode.
 *
 * @remarks If CPUID.01H:ECX.[24] = 1
 */
#define IA32_TSC_DEADLINE                                            0x000006E0

/**
 * Enable/disable HWP.
 *
 * @remarks If CPUID.06H:EAX.[7] = 1
 */
#define IA32_PM_ENABLE                                               0x00000770
typedef union
{
  struct
  {
	/**
	* [Bit 0] HWP_ENABLE.
	*
	* @remarks If CPUID.06H:EAX.[7] = 1
	* @see Vol3B[14.4.2(Enabling HWP)]
	*/
	uint64_t hwp_enable                                              : 1;
#define IA32_PM_ENABLE_HWP_ENABLE_BIT                                0
#define IA32_PM_ENABLE_HWP_ENABLE_FLAG                               0x01
#define IA32_PM_ENABLE_HWP_ENABLE_MASK                               0x01
#define IA32_PM_ENABLE_HWP_ENABLE(_)                                 (((_) >> 0) & 0x01)
	uint64_t reserved1                                               : 63;
  };

  uint64_t flags;
} pm_enable_register;


/**
 * HWP Performance Range Enumeration.
 *
 * @remarks If CPUID.06H:EAX.[7] = 1
 */
#define IA32_HWP_CAPABILITIES                                        0x00000771
typedef union
{
  struct
  {
	/**
	* [Bits 7:0] Highest_Performance.
	*
	* @remarks If CPUID.06H:EAX.[7] = 1
	* @see Vol3B[14.4.3(HWP Performance Range and Dynamic Capabilities)]
	*/
	uint64_t highest_performance                                     : 8;
#define IA32_HWP_CAPABILITIES_HIGHEST_PERFORMANCE_BIT                0
#define IA32_HWP_CAPABILITIES_HIGHEST_PERFORMANCE_FLAG               0xFF
#define IA32_HWP_CAPABILITIES_HIGHEST_PERFORMANCE_MASK               0xFF
#define IA32_HWP_CAPABILITIES_HIGHEST_PERFORMANCE(_)                 (((_) >> 0) & 0xFF)

	/**
	* [Bits 15:8] Guaranteed_Performance.
	*
	* @remarks If CPUID.06H:EAX.[7] = 1
	* @see Vol3B[14.4.3(HWP Performance Range and Dynamic Capabilities)]
	*/
	uint64_t guaranteed_performance                                  : 8;
#define IA32_HWP_CAPABILITIES_GUARANTEED_PERFORMANCE_BIT             8
#define IA32_HWP_CAPABILITIES_GUARANTEED_PERFORMANCE_FLAG            0xFF00
#define IA32_HWP_CAPABILITIES_GUARANTEED_PERFORMANCE_MASK            0xFF
#define IA32_HWP_CAPABILITIES_GUARANTEED_PERFORMANCE(_)              (((_) >> 8) & 0xFF)

	/**
	* [Bits 23:16] Most_Efficient_Performance.
	*
	* @remarks If CPUID.06H:EAX.[7] = 1
	* @see Vol3B[14.4.3(HWP Performance Range and Dynamic Capabilities)]
	*/
	uint64_t most_efficient_performance                              : 8;
#define IA32_HWP_CAPABILITIES_MOST_EFFICIENT_PERFORMANCE_BIT         16
#define IA32_HWP_CAPABILITIES_MOST_EFFICIENT_PERFORMANCE_FLAG        0xFF0000
#define IA32_HWP_CAPABILITIES_MOST_EFFICIENT_PERFORMANCE_MASK        0xFF
#define IA32_HWP_CAPABILITIES_MOST_EFFICIENT_PERFORMANCE(_)          (((_) >> 16) & 0xFF)

	/**
	* [Bits 31:24] Lowest_Performance.
	*
	* @remarks If CPUID.06H:EAX.[7] = 1
	* @see Vol3B[14.4.3(HWP Performance Range and Dynamic Capabilities)]
	*/
	uint64_t lowest_performance                                      : 8;
#define IA32_HWP_CAPABILITIES_LOWEST_PERFORMANCE_BIT                 24
#define IA32_HWP_CAPABILITIES_LOWEST_PERFORMANCE_FLAG                0xFF000000
#define IA32_HWP_CAPABILITIES_LOWEST_PERFORMANCE_MASK                0xFF
#define IA32_HWP_CAPABILITIES_LOWEST_PERFORMANCE(_)                  (((_) >> 24) & 0xFF)
	uint64_t reserved1                                               : 32;
  };

  uint64_t flags;
} hwp_capabilities_register;


/**
 * Power Management Control Hints for All Logical Processors in a Package.
 *
 * @remarks If CPUID.06H:EAX.[11] = 1
 */
#define IA32_HWP_REQUEST_PKG                                         0x00000772
typedef union
{
  struct
  {
	/**
	* [Bits 7:0] Minimum_Performance.
	*
	* @remarks If CPUID.06H:EAX.[11] = 1
	* @see Vol3B[14.4.4(Managing HWP)]
	*/
	uint64_t minimum_performance                                     : 8;
#define IA32_HWP_REQUEST_PKG_MINIMUM_PERFORMANCE_BIT                 0
#define IA32_HWP_REQUEST_PKG_MINIMUM_PERFORMANCE_FLAG                0xFF
#define IA32_HWP_REQUEST_PKG_MINIMUM_PERFORMANCE_MASK                0xFF
#define IA32_HWP_REQUEST_PKG_MINIMUM_PERFORMANCE(_)                  (((_) >> 0) & 0xFF)

	/**
	* [Bits 15:8] Maximum_Performance.
	*
	* @remarks If CPUID.06H:EAX.[11] = 1
	* @see Vol3B[14.4.4(Managing HWP)]
	*/
	uint64_t maximum_performance                                     : 8;
#define IA32_HWP_REQUEST_PKG_MAXIMUM_PERFORMANCE_BIT                 8
#define IA32_HWP_REQUEST_PKG_MAXIMUM_PERFORMANCE_FLAG                0xFF00
#define IA32_HWP_REQUEST_PKG_MAXIMUM_PERFORMANCE_MASK                0xFF
#define IA32_HWP_REQUEST_PKG_MAXIMUM_PERFORMANCE(_)                  (((_) >> 8) & 0xFF)

	/**
	* [Bits 23:16] Desired_Performance.
	*
	* @remarks If CPUID.06H:EAX.[11] = 1
	* @see Vol3B[14.4.4(Managing HWP)]
	*/
	uint64_t desired_performance                                     : 8;
#define IA32_HWP_REQUEST_PKG_DESIRED_PERFORMANCE_BIT                 16
#define IA32_HWP_REQUEST_PKG_DESIRED_PERFORMANCE_FLAG                0xFF0000
#define IA32_HWP_REQUEST_PKG_DESIRED_PERFORMANCE_MASK                0xFF
#define IA32_HWP_REQUEST_PKG_DESIRED_PERFORMANCE(_)                  (((_) >> 16) & 0xFF)

	/**
	* [Bits 31:24] Energy_Performance_Preference.
	*
	* @remarks If CPUID.06H:EAX.[11] = 1 && CPUID.06H:EAX.[10] = 1
	* @see Vol3B[14.4.4(Managing HWP)]
	*/
	uint64_t energy_performance_preference                           : 8;
#define IA32_HWP_REQUEST_PKG_ENERGY_PERFORMANCE_PREFERENCE_BIT       24
#define IA32_HWP_REQUEST_PKG_ENERGY_PERFORMANCE_PREFERENCE_FLAG      0xFF000000
#define IA32_HWP_REQUEST_PKG_ENERGY_PERFORMANCE_PREFERENCE_MASK      0xFF
#define IA32_HWP_REQUEST_PKG_ENERGY_PERFORMANCE_PREFERENCE(_)        (((_) >> 24) & 0xFF)

	/**
	* [Bits 41:32] Activity_Window.
	*
	* @remarks If CPUID.06H:EAX.[11] = 1 && CPUID.06H:EAX.[9] = 1
	* @see Vol3B[14.4.4(Managing HWP)]
	*/
	uint64_t activity_window                                         : 10;
#define IA32_HWP_REQUEST_PKG_ACTIVITY_WINDOW_BIT                     32
#define IA32_HWP_REQUEST_PKG_ACTIVITY_WINDOW_FLAG                    0x3FF00000000
#define IA32_HWP_REQUEST_PKG_ACTIVITY_WINDOW_MASK                    0x3FF
#define IA32_HWP_REQUEST_PKG_ACTIVITY_WINDOW(_)                      (((_) >> 32) & 0x3FF)
	uint64_t reserved1                                               : 22;
  };

  uint64_t flags;
} hwp_request_pkg_register;


/**
 * Control HWP Native Interrupts.
 *
 * @remarks If CPUID.06H:EAX.[8] = 1
 */
#define IA32_HWP_INTERRUPT                                           0x00000773
typedef union
{
  struct
  {
	/**
	* [Bit 0] EN_Guaranteed_Performance_Change.
	*
	* @remarks If CPUID.06H:EAX.[8] = 1
	* @see Vol3B[14.4.6(HWP Notifications)]
	*/
	uint64_t en_guaranteed_performance_change                        : 1;
#define IA32_HWP_INTERRUPT_EN_GUARANTEED_PERFORMANCE_CHANGE_BIT      0
#define IA32_HWP_INTERRUPT_EN_GUARANTEED_PERFORMANCE_CHANGE_FLAG     0x01
#define IA32_HWP_INTERRUPT_EN_GUARANTEED_PERFORMANCE_CHANGE_MASK     0x01
#define IA32_HWP_INTERRUPT_EN_GUARANTEED_PERFORMANCE_CHANGE(_)       (((_) >> 0) & 0x01)

	/**
	* [Bit 1] EN_Excursion_Minimum.
	*
	* @remarks If CPUID.06H:EAX.[8] = 1
	* @see Vol3B[14.4.6(HWP Notifications)]
	*/
	uint64_t en_excursion_minimum                                    : 1;
#define IA32_HWP_INTERRUPT_EN_EXCURSION_MINIMUM_BIT                  1
#define IA32_HWP_INTERRUPT_EN_EXCURSION_MINIMUM_FLAG                 0x02
#define IA32_HWP_INTERRUPT_EN_EXCURSION_MINIMUM_MASK                 0x01
#define IA32_HWP_INTERRUPT_EN_EXCURSION_MINIMUM(_)                   (((_) >> 1) & 0x01)
	uint64_t reserved1                                               : 62;
  };

  uint64_t flags;
} hwp_interrupt_register;


/**
 * Power Management Control Hints to a Logical Processor.
 *
 * @remarks If CPUID.06H:EAX.[7] = 1
 */
#define IA32_HWP_REQUEST                                             0x00000774
typedef union
{
  struct
  {
	/**
	* [Bits 7:0] Minimum_Performance.
	*
	* @remarks If CPUID.06H:EAX.[7] = 1
	* @see Vol3B[14.4.4(Managing HWP)]
	*/
	uint64_t minimum_performance                                     : 8;
#define IA32_HWP_REQUEST_MINIMUM_PERFORMANCE_BIT                     0
#define IA32_HWP_REQUEST_MINIMUM_PERFORMANCE_FLAG                    0xFF
#define IA32_HWP_REQUEST_MINIMUM_PERFORMANCE_MASK                    0xFF
#define IA32_HWP_REQUEST_MINIMUM_PERFORMANCE(_)                      (((_) >> 0) & 0xFF)

	/**
	* [Bits 15:8] Maximum_Performance.
	*
	* @remarks If CPUID.06H:EAX.[7] = 1
	* @see Vol3B[14.4.4(Managing HWP)]
	*/
	uint64_t maximum_performance                                     : 8;
#define IA32_HWP_REQUEST_MAXIMUM_PERFORMANCE_BIT                     8
#define IA32_HWP_REQUEST_MAXIMUM_PERFORMANCE_FLAG                    0xFF00
#define IA32_HWP_REQUEST_MAXIMUM_PERFORMANCE_MASK                    0xFF
#define IA32_HWP_REQUEST_MAXIMUM_PERFORMANCE(_)                      (((_) >> 8) & 0xFF)

	/**
	* [Bits 23:16] Desired_Performance.
	*
	* @remarks If CPUID.06H:EAX.[7] = 1
	* @see Vol3B[14.4.4(Managing HWP)]
	*/
	uint64_t desired_performance                                     : 8;
#define IA32_HWP_REQUEST_DESIRED_PERFORMANCE_BIT                     16
#define IA32_HWP_REQUEST_DESIRED_PERFORMANCE_FLAG                    0xFF0000
#define IA32_HWP_REQUEST_DESIRED_PERFORMANCE_MASK                    0xFF
#define IA32_HWP_REQUEST_DESIRED_PERFORMANCE(_)                      (((_) >> 16) & 0xFF)

	/**
	* [Bits 31:24] Energy_Performance_Preference.
	*
	* @remarks If CPUID.06H:EAX.[7] = 1 && CPUID.06H:EAX.[10] = 1
	* @see Vol3B[14.4.4(Managing HWP)]
	*/
	uint64_t energy_performance_preference                           : 8;
#define IA32_HWP_REQUEST_ENERGY_PERFORMANCE_PREFERENCE_BIT           24
#define IA32_HWP_REQUEST_ENERGY_PERFORMANCE_PREFERENCE_FLAG          0xFF000000
#define IA32_HWP_REQUEST_ENERGY_PERFORMANCE_PREFERENCE_MASK          0xFF
#define IA32_HWP_REQUEST_ENERGY_PERFORMANCE_PREFERENCE(_)            (((_) >> 24) & 0xFF)

	/**
	* [Bits 41:32] Activity_Window.
	*
	* @remarks If CPUID.06H:EAX.[7] = 1 && CPUID.06H:EAX.[9] = 1
	* @see Vol3B[14.4.4(Managing HWP)]
	*/
	uint64_t activity_window                                         : 10;
#define IA32_HWP_REQUEST_ACTIVITY_WINDOW_BIT                         32
#define IA32_HWP_REQUEST_ACTIVITY_WINDOW_FLAG                        0x3FF00000000
#define IA32_HWP_REQUEST_ACTIVITY_WINDOW_MASK                        0x3FF
#define IA32_HWP_REQUEST_ACTIVITY_WINDOW(_)                          (((_) >> 32) & 0x3FF)

	/**
	* [Bit 42] Package_Control.
	*
	* @remarks If CPUID.06H:EAX.[7] = 1 && CPUID.06H:EAX.[11] = 1
	* @see Vol3B[14.4.4(Managing HWP)]
	*/
	uint64_t package_control                                         : 1;
#define IA32_HWP_REQUEST_PACKAGE_CONTROL_BIT                         42
#define IA32_HWP_REQUEST_PACKAGE_CONTROL_FLAG                        0x40000000000
#define IA32_HWP_REQUEST_PACKAGE_CONTROL_MASK                        0x01
#define IA32_HWP_REQUEST_PACKAGE_CONTROL(_)                          (((_) >> 42) & 0x01)
	uint64_t reserved1                                               : 21;
  };

  uint64_t flags;
} hwp_request_register;


/**
 * Log bits indicating changes to Guaranteed & excursions to Minimum.
 *
 * @remarks If CPUID.06H:EAX.[7] = 1
 */
#define IA32_HWP_STATUS                                              0x00000777
typedef union
{
  struct
  {
	/**
	* [Bit 0] Guaranteed_Performance_Change.
	*
	* @remarks If CPUID.06H:EAX.[7] = 1
	* @see Vol3B[14.4.5(HWP Feedback)]
	*/
	uint64_t guaranteed_performance_change                           : 1;
#define IA32_HWP_STATUS_GUARANTEED_PERFORMANCE_CHANGE_BIT            0
#define IA32_HWP_STATUS_GUARANTEED_PERFORMANCE_CHANGE_FLAG           0x01
#define IA32_HWP_STATUS_GUARANTEED_PERFORMANCE_CHANGE_MASK           0x01
#define IA32_HWP_STATUS_GUARANTEED_PERFORMANCE_CHANGE(_)             (((_) >> 0) & 0x01)
	uint64_t reserved1                                               : 1;

	/**
	* [Bit 2] Excursion_To_Minimum.
	*
	* @remarks If CPUID.06H:EAX.[7] = 1
	* @see Vol3B[14.4.5(HWP Feedback)]
	*/
	uint64_t excursion_to_minimum                                    : 1;
#define IA32_HWP_STATUS_EXCURSION_TO_MINIMUM_BIT                     2
#define IA32_HWP_STATUS_EXCURSION_TO_MINIMUM_FLAG                    0x04
#define IA32_HWP_STATUS_EXCURSION_TO_MINIMUM_MASK                    0x01
#define IA32_HWP_STATUS_EXCURSION_TO_MINIMUM(_)                      (((_) >> 2) & 0x01)
	uint64_t reserved2                                               : 61;
  };

  uint64_t flags;
} hwp_status_register;


/**
 * x2APIC ID Register.
 *
 * @remarks If CPUID.01H:ECX[21] = 1 && IA32_APIC_BASE.[10] = 1
 * @see Vol3A[10.12(EXTENDED XAPIC (X2APIC))]
 */
#define IA32_X2APIC_APICID                                           0x00000802

/**
 * x2APIC Version Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_VERSION                                          0x00000803

/**
 * x2APIC Task Priority Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_TPR                                              0x00000808

/**
 * x2APIC Processor Priority Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_PPR                                              0x0000080A

/**
 * x2APIC EOI Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_EOI                                              0x0000080B

/**
 * x2APIC Logical Destination Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_LDR                                              0x0000080D

/**
 * x2APIC Spurious Interrupt Vector Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_SIVR                                             0x0000080F
/**
 * @defgroup ia32_x2apic_isr \
 *           IA32_X2APIC_ISR(n)
 *
 * x2APIC In-Service Register Bits (n * 32 + 31):(n * 32).
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 * @{
 */
#define IA32_X2APIC_ISR0                                             0x00000810
#define IA32_X2APIC_ISR1                                             0x00000811
#define IA32_X2APIC_ISR2                                             0x00000812
#define IA32_X2APIC_ISR3                                             0x00000813
#define IA32_X2APIC_ISR4                                             0x00000814
#define IA32_X2APIC_ISR5                                             0x00000815
#define IA32_X2APIC_ISR6                                             0x00000816
#define IA32_X2APIC_ISR7                                             0x00000817
/**
 * @}
 */

/**
 * @defgroup ia32_x2apic_tmr \
 *           IA32_X2APIC_TMR(n)
 *
 * x2APIC Trigger Mode Register Bits (n * 32 + 31):(n * 32).
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 * @{
 */
#define IA32_X2APIC_TMR0                                             0x00000818
#define IA32_X2APIC_TMR1                                             0x00000819
#define IA32_X2APIC_TMR2                                             0x0000081A
#define IA32_X2APIC_TMR3                                             0x0000081B
#define IA32_X2APIC_TMR4                                             0x0000081C
#define IA32_X2APIC_TMR5                                             0x0000081D
#define IA32_X2APIC_TMR6                                             0x0000081E
#define IA32_X2APIC_TMR7                                             0x0000081F
/**
 * @}
 */

/**
 * @defgroup ia32_x2apic_irr \
 *           IA32_X2APIC_IRR(n)
 *
 * x2APIC Interrupt Request Register Bits (n * 32 + 31):(n * 32).
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 * @{
 */
#define IA32_X2APIC_IRR0                                             0x00000820
#define IA32_X2APIC_IRR1                                             0x00000821
#define IA32_X2APIC_IRR2                                             0x00000822
#define IA32_X2APIC_IRR3                                             0x00000823
#define IA32_X2APIC_IRR4                                             0x00000824
#define IA32_X2APIC_IRR5                                             0x00000825
#define IA32_X2APIC_IRR6                                             0x00000826
#define IA32_X2APIC_IRR7                                             0x00000827
/**
 * @}
 */


/**
 * x2APIC Error Status Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_ESR                                              0x00000828

/**
 * x2APIC LVT Corrected Machine Check Interrupt Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_LVT_CMCI                                         0x0000082F

/**
 * x2APIC Interrupt Command Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_ICR                                              0x00000830

/**
 * x2APIC LVT Timer Interrupt Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_LVT_TIMER                                        0x00000832

/**
 * x2APIC LVT Thermal Sensor Interrupt Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_LVT_THERMAL                                      0x00000833

/**
 * x2APIC LVT Performance Monitor Interrupt Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_LVT_PMI                                          0x00000834

/**
 * x2APIC LVT LINT0 Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_LVT_LINT0                                        0x00000835

/**
 * x2APIC LVT LINT1 Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_LVT_LINT1                                        0x00000836

/**
 * x2APIC LVT Error Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_LVT_ERROR                                        0x00000837

/**
 * x2APIC Initial Count Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_INIT_COUNT                                       0x00000838

/**
 * x2APIC Current Count Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_CUR_COUNT                                        0x00000839

/**
 * x2APIC Divide Configuration Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_DIV_CONF                                         0x0000083E

/**
 * x2APIC Self IPI Register.
 *
 * @remarks If CPUID.01H:ECX.[21] = 1 && IA32_APIC_BASE.[10] = 1
 */
#define IA32_X2APIC_SELF_IPI                                         0x0000083F

/**
 * Silicon Debug Feature Control.
 *
 * @remarks If CPUID.01H:ECX.[11] = 1
 */
#define IA32_DEBUG_INTERFACE                                         0x00000C80
typedef union
{
  struct
  {
	/**
	* @brief Enable <b>(R/W)</b>
	*
	* [Bit 0] BIOS set 1 to enable Silicon debug features. Default is 0.
	*
	* @remarks If CPUID.01H:ECX.[11] = 1
	*/
	uint64_t enable                                                  : 1;
#define IA32_DEBUG_INTERFACE_ENABLE_BIT                              0
#define IA32_DEBUG_INTERFACE_ENABLE_FLAG                             0x01
#define IA32_DEBUG_INTERFACE_ENABLE_MASK                             0x01
#define IA32_DEBUG_INTERFACE_ENABLE(_)                               (((_) >> 0) & 0x01)
	uint64_t reserved1                                               : 29;

	/**
	* @brief Lock <b>(R/W)</b>
	*
	* [Bit 30] If 1, locks any further change to the MSR. The lock bit is set automatically on the first SMI assertion even if
	* not explicitly set by BIOS. Default is 0.
	*
	* @remarks If CPUID.01H:ECX.[11] = 1
	*/
	uint64_t lock                                                    : 1;
#define IA32_DEBUG_INTERFACE_LOCK_BIT                                30
#define IA32_DEBUG_INTERFACE_LOCK_FLAG                               0x40000000
#define IA32_DEBUG_INTERFACE_LOCK_MASK                               0x01
#define IA32_DEBUG_INTERFACE_LOCK(_)                                 (((_) >> 30) & 0x01)

	/**
	* @brief Debug Occurred <b>(R/O)</b>
	*
	* [Bit 31] This "sticky bit" is set by hardware to indicate the status of bit 0. Default is 0.
	*
	* @remarks If CPUID.01H:ECX.[11] = 1
	*/
	uint64_t debug_occurred                                          : 1;
#define IA32_DEBUG_INTERFACE_DEBUG_OCCURRED_BIT                      31
#define IA32_DEBUG_INTERFACE_DEBUG_OCCURRED_FLAG                     0x80000000
#define IA32_DEBUG_INTERFACE_DEBUG_OCCURRED_MASK                     0x01
#define IA32_DEBUG_INTERFACE_DEBUG_OCCURRED(_)                       (((_) >> 31) & 0x01)
	uint64_t reserved2                                               : 32;
  };

  uint64_t flags;
} debug_interface_register;


/**
 * L3 QOS Configuration.
 *
 * @remarks If ( CPUID.(EAX=10H, ECX=1):ECX.[2] = 1 )
 */
#define IA32_L3_QOS_CFG                                              0x00000C81
typedef union
{
  struct
  {
	/**
	* @brief Enable <b>(R/W)</b>
	*
	* [Bit 0] Set 1 to enable L3 CAT masks and COS to operate in Code and Data Prioritization (CDP) mode.
	*/
	uint64_t enable                                                  : 1;
#define IA32_L3_QOS_CFG_ENABLE_BIT                                   0
#define IA32_L3_QOS_CFG_ENABLE_FLAG                                  0x01
#define IA32_L3_QOS_CFG_ENABLE_MASK                                  0x01
#define IA32_L3_QOS_CFG_ENABLE(_)                                    (((_) >> 0) & 0x01)
	uint64_t reserved1                                               : 63;
  };

  uint64_t flags;
} l3_qos_cfg_register;


/**
 * L2 QOS Configuration.
 *
 * @remarks If ( CPUID.(EAX=10H, ECX=2):ECX.[2] = 1 )
 */
#define IA32_L2_QOS_CFG                                              0x00000C82
typedef union
{
  struct
  {
	/**
	* @brief Enable <b>(R/W)</b>
	*
	* [Bit 0] Set 1 to enable L2 CAT masks and COS to operate in Code and Data Prioritization (CDP) mode.
	*/
	uint64_t enable                                                  : 1;
#define IA32_L2_QOS_CFG_ENABLE_BIT                                   0
#define IA32_L2_QOS_CFG_ENABLE_FLAG                                  0x01
#define IA32_L2_QOS_CFG_ENABLE_MASK                                  0x01
#define IA32_L2_QOS_CFG_ENABLE(_)                                    (((_) >> 0) & 0x01)
	uint64_t reserved1                                               : 63;
  };

  uint64_t flags;
} l2_qos_cfg_register;


/**
 * Monitoring Event Select Register.
 *
 * @remarks If ( CPUID.(EAX=07H, ECX=0):EBX.[12] = 1 )
 */
#define IA32_QM_EVTSEL                                               0x00000C8D
typedef union
{
  struct
  {
	/**
	* @brief Event ID
	*
	* [Bits 7:0] ID of a supported monitoring event to report via IA32_QM_CTR.
	*/
	uint64_t event_id                                                : 8;
#define IA32_QM_EVTSEL_EVENT_ID_BIT                                  0
#define IA32_QM_EVTSEL_EVENT_ID_FLAG                                 0xFF
#define IA32_QM_EVTSEL_EVENT_ID_MASK                                 0xFF
#define IA32_QM_EVTSEL_EVENT_ID(_)                                   (((_) >> 0) & 0xFF)
	uint64_t reserved1                                               : 24;

	/**
	* @brief Resource Monitoring ID
	*
	* [Bits 63:32] ID for monitoring hardware to report monitored data via IA32_QM_CTR.
	*
	* @remarks Bits [N+31:32] N = Ceil (Log2 (CPUID.(EAX= 0FH,ECX=0H).EBX[31:0] + 1))
	*/
	uint64_t resource_monitoring_id                                  : 32;
#define IA32_QM_EVTSEL_RESOURCE_MONITORING_ID_BIT                    32
#define IA32_QM_EVTSEL_RESOURCE_MONITORING_ID_FLAG                   0xFFFFFFFF00000000
#define IA32_QM_EVTSEL_RESOURCE_MONITORING_ID_MASK                   0xFFFFFFFF
#define IA32_QM_EVTSEL_RESOURCE_MONITORING_ID(_)                     (((_) >> 32) & 0xFFFFFFFF)
  };

  uint64_t flags;
} qm_evtsel_register;


/**
 * Monitoring Counter Register.
 *
 * @remarks If ( CPUID.(EAX=07H, ECX=0):EBX.[12] = 1 )
 */
#define IA32_QM_CTR                                                  0x00000C8E
typedef union
{
  struct
  {
	/**
	* [Bits 61:0] Resource Monitored Data.
	*/
	uint64_t resource_monitored_data                                 : 62;
#define IA32_QM_CTR_RESOURCE_MONITORED_DATA_BIT                      0
#define IA32_QM_CTR_RESOURCE_MONITORED_DATA_FLAG                     0x3FFFFFFFFFFFFFFF
#define IA32_QM_CTR_RESOURCE_MONITORED_DATA_MASK                     0x3FFFFFFFFFFFFFFF
#define IA32_QM_CTR_RESOURCE_MONITORED_DATA(_)                       (((_) >> 0) & 0x3FFFFFFFFFFFFFFF)

	/**
	* @brief Unavailable
	*
	* [Bit 62] If 1, indicates data for this RMID is not available or not monitored for this resource or RMID.
	*/
	uint64_t unavailable                                             : 1;
#define IA32_QM_CTR_UNAVAILABLE_BIT                                  62
#define IA32_QM_CTR_UNAVAILABLE_FLAG                                 0x4000000000000000
#define IA32_QM_CTR_UNAVAILABLE_MASK                                 0x01
#define IA32_QM_CTR_UNAVAILABLE(_)                                   (((_) >> 62) & 0x01)

	/**
	* @brief Error
	*
	* [Bit 63] If 1, indicates an unsupported RMID or event type was written to IA32_PQR_QM_EVTSEL.
	*/
	uint64_t error                                                   : 1;
#define IA32_QM_CTR_ERROR_BIT                                        63
#define IA32_QM_CTR_ERROR_FLAG                                       0x8000000000000000
#define IA32_QM_CTR_ERROR_MASK                                       0x01
#define IA32_QM_CTR_ERROR(_)                                         (((_) >> 63) & 0x01)
  };

  uint64_t flags;
} qm_ctr_register;


/**
 * Resource Association Register.
 *
 * @remarks If ( (CPUID.(EAX=07H, ECX=0):EBX[12] = 1) or (CPUID.(EAX=07H, ECX=0):EBX[15] = 1 ) )
 */
#define IA32_PQR_ASSOC                                               0x00000C8F
typedef union
{
  struct
  {
	/**
	* @brief Resource Monitoring ID <b>(R/W)</b>
	*
	* [Bits 31:0] ID for monitoring hardware to track internal operation, e.g., memory access.
	*
	* @remarks Bits [N-1:0] N = Ceil (Log2 (CPUID.(EAX= 0FH, ECX=0H).EBX[31:0] +1)) 31:N Reserved
	*/
	uint64_t resource_monitoring_id                                  : 32;
#define IA32_PQR_ASSOC_RESOURCE_MONITORING_ID_BIT                    0
#define IA32_PQR_ASSOC_RESOURCE_MONITORING_ID_FLAG                   0xFFFFFFFF
#define IA32_PQR_ASSOC_RESOURCE_MONITORING_ID_MASK                   0xFFFFFFFF
#define IA32_PQR_ASSOC_RESOURCE_MONITORING_ID(_)                     (((_) >> 0) & 0xFFFFFFFF)

	/**
	* @brief COS <b>(R/W)</b>
	*
	* [Bits 63:32] The class of service (COS) to enforce (on writes); returns the current COS when read.
	*
	* @remarks If ( CPUID.(EAX=07H, ECX=0):EBX.[15] = 1 )
	*/
	uint64_t cos                                                     : 32;
#define IA32_PQR_ASSOC_COS_BIT                                       32
#define IA32_PQR_ASSOC_COS_FLAG                                      0xFFFFFFFF00000000
#define IA32_PQR_ASSOC_COS_MASK                                      0xFFFFFFFF
#define IA32_PQR_ASSOC_COS(_)                                        (((_) >> 32) & 0xFFFFFFFF)
  };

  uint64_t flags;
} pqr_assoc_register;


/**
 * Supervisor State of MPX Configuration.
 *
 * @remarks If (CPUID.(EAX=07H, ECX=0H):EBX[14] = 1)
 */
#define IA32_BNDCFGS                                                 0x00000D90
typedef union
{
  struct
  {
	/**
	* [Bit 0] Enable Intel MPX in supervisor mode.
	*/
	uint64_t enable                                                  : 1;
#define IA32_BNDCFGS_ENABLE_BIT                                      0
#define IA32_BNDCFGS_ENABLE_FLAG                                     0x01
#define IA32_BNDCFGS_ENABLE_MASK                                     0x01
#define IA32_BNDCFGS_ENABLE(_)                                       (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Preserve the bounds registers for near branch instructions in the absence of the BND prefix.
	*/
	uint64_t bnd_preserve                                            : 1;
#define IA32_BNDCFGS_BND_PRESERVE_BIT                                1
#define IA32_BNDCFGS_BND_PRESERVE_FLAG                               0x02
#define IA32_BNDCFGS_BND_PRESERVE_MASK                               0x01
#define IA32_BNDCFGS_BND_PRESERVE(_)                                 (((_) >> 1) & 0x01)
	uint64_t reserved1                                               : 10;

	/**
	* [Bits 63:12] Base Address of Bound Directory.
	*/
	uint64_t bound_directory_base_address                            : 52;
#define IA32_BNDCFGS_BOUND_DIRECTORY_BASE_ADDRESS_BIT                12
#define IA32_BNDCFGS_BOUND_DIRECTORY_BASE_ADDRESS_FLAG               0xFFFFFFFFFFFFF000
#define IA32_BNDCFGS_BOUND_DIRECTORY_BASE_ADDRESS_MASK               0xFFFFFFFFFFFFF
#define IA32_BNDCFGS_BOUND_DIRECTORY_BASE_ADDRESS(_)                 (((_) >> 12) & 0xFFFFFFFFFFFFF)
  };

  uint64_t flags;
} bndcfgs_register;


/**
 * Extended Supervisor State Mask.
 *
 * @remarks If ( CPUID.(0DH, 1):EAX.[3] = 1
 */
#define IA32_XSS                                                     0x00000DA0
typedef union
{
  struct
  {
	uint64_t reserved1                                               : 8;

	/**
	* [Bit 8] Trace Packet Configuration State.
	*/
	uint64_t trace_packet_configuration_state                        : 1;
#define IA32_XSS_TRACE_PACKET_CONFIGURATION_STATE_BIT                8
#define IA32_XSS_TRACE_PACKET_CONFIGURATION_STATE_FLAG               0x100
#define IA32_XSS_TRACE_PACKET_CONFIGURATION_STATE_MASK               0x01
#define IA32_XSS_TRACE_PACKET_CONFIGURATION_STATE(_)                 (((_) >> 8) & 0x01)
	uint64_t reserved2                                               : 55;
  };

  uint64_t flags;
} xss_register;


/**
 * Package Level Enable/disable HDC.
 *
 * @remarks If CPUID.06H:EAX.[13] = 1
 */
#define IA32_PKG_HDC_CTL                                             0x00000DB0
typedef union
{
  struct
  {
	/**
	* @brief HDC_Pkg_Enable <b>(R/W)</b>
	*
	* [Bit 0] Force HDC idling or wake up HDC-idled logical processors in the package.
	*
	* @remarks If CPUID.06H:EAX.[13] = 1
	* @see Vol3B[14.5.2(Package level Enabling HDC)]
	*/
	uint64_t hdc_pkg_enable                                          : 1;
#define IA32_PKG_HDC_CTL_HDC_PKG_ENABLE_BIT                          0
#define IA32_PKG_HDC_CTL_HDC_PKG_ENABLE_FLAG                         0x01
#define IA32_PKG_HDC_CTL_HDC_PKG_ENABLE_MASK                         0x01
#define IA32_PKG_HDC_CTL_HDC_PKG_ENABLE(_)                           (((_) >> 0) & 0x01)
	uint64_t reserved1                                               : 63;
  };

  uint64_t flags;
} pkg_hdc_ctl_register;


/**
 * Enable/disable HWP.
 *
 * @remarks If CPUID.06H:EAX.[13] = 1
 */
#define IA32_PM_CTL1                                                 0x00000DB1
typedef union
{
  struct
  {
	/**
	* @brief HDC_Allow_Block <b>(R/W)</b>
	*
	* [Bit 0] Allow/Block this logical processor for package level HDC control.
	*
	* @remarks If CPUID.06H:EAX.[13] = 1
	* @see Vol3B[14.5.3(Logical-Processor Level HDC Control)]
	*/
	uint64_t hdc_allow_block                                         : 1;
#define IA32_PM_CTL1_HDC_ALLOW_BLOCK_BIT                             0
#define IA32_PM_CTL1_HDC_ALLOW_BLOCK_FLAG                            0x01
#define IA32_PM_CTL1_HDC_ALLOW_BLOCK_MASK                            0x01
#define IA32_PM_CTL1_HDC_ALLOW_BLOCK(_)                              (((_) >> 0) & 0x01)
	uint64_t reserved1                                               : 63;
  };

  uint64_t flags;
} pm_ctl1_register;


/**
 * Per-Logical_Processor HDC Idle Residency.
 *
 * @remarks If CPUID.06H:EAX.[13] = 1
 */
#define IA32_THREAD_STALL                                            0x00000DB2
typedef struct
{
  /**
	* @brief Stall_Cycle_Cnt <b>(R/W)</b>
	*
	* Stalled cycles due to HDC forced idle on this logical processor.
	*
	* @remarks If CPUID.06H:EAX.[13] = 1
	* @see Vol3B[14.5.4.1(IA32_THREAD_STALL)]
	*/
  uint64_t stall_cycle_count;
} thread_stall_register;


/**
 * Extended Feature Enables.
 *
 * @remarks If CPUID.06H:EAX.[13] = 1
 */
#define IA32_EFER                                                    0xC0000080
typedef union
{
  struct
  {
	/**
	* @brief SYSCALL Enable <b>(R/W)</b>
	*
	* [Bit 0] Enables SYSCALL/SYSRET instructions in 64-bit mode.
	*/
	uint64_t syscall_enable                                          : 1;
#define IA32_EFER_SYSCALL_ENABLE_BIT                                 0
#define IA32_EFER_SYSCALL_ENABLE_FLAG                                0x01
#define IA32_EFER_SYSCALL_ENABLE_MASK                                0x01
#define IA32_EFER_SYSCALL_ENABLE(_)                                  (((_) >> 0) & 0x01)
	uint64_t reserved1                                               : 7;

	/**
	* @brief IA-32e Mode Enable <b>(R/W)</b>
	*
	* [Bit 8] Enables IA-32e mode operation.
	*/
	uint64_t ia32e_mode_enable                                       : 1;
#define IA32_EFER_IA32E_MODE_ENABLE_BIT                              8
#define IA32_EFER_IA32E_MODE_ENABLE_FLAG                             0x100
#define IA32_EFER_IA32E_MODE_ENABLE_MASK                             0x01
#define IA32_EFER_IA32E_MODE_ENABLE(_)                               (((_) >> 8) & 0x01)
	uint64_t reserved2                                               : 1;

	/**
	* @brief IA-32e Mode Active <b>(R)</b>
	*
	* [Bit 10] Indicates IA-32e mode is active when set.
	*/
	uint64_t ia32e_mode_active                                       : 1;
#define IA32_EFER_IA32E_MODE_ACTIVE_BIT                              10
#define IA32_EFER_IA32E_MODE_ACTIVE_FLAG                             0x400
#define IA32_EFER_IA32E_MODE_ACTIVE_MASK                             0x01
#define IA32_EFER_IA32E_MODE_ACTIVE(_)                               (((_) >> 10) & 0x01)

	/**
	* [Bit 11] Execute Disable Bit Enable.
	*/
	uint64_t execute_disable_bit_enable                              : 1;
#define IA32_EFER_EXECUTE_DISABLE_BIT_ENABLE_BIT                     11
#define IA32_EFER_EXECUTE_DISABLE_BIT_ENABLE_FLAG                    0x800
#define IA32_EFER_EXECUTE_DISABLE_BIT_ENABLE_MASK                    0x01
#define IA32_EFER_EXECUTE_DISABLE_BIT_ENABLE(_)                      (((_) >> 11) & 0x01)
	uint64_t reserved3                                               : 52;
  };

  uint64_t flags;
} efer_register;


/**
 * System Call Target Address.
 *
 * @remarks If CPUID.80000001:EDX.[29] = 1
 */
#define IA32_STAR                                                    0xC0000081

/**
 * @brief IA-32e Mode System Call Target Address <b>(R/W)</b>
 *
 * Target RIP for the called procedure when SYSCALL is executed in 64-bit mode.
 *
 * @remarks If CPUID.80000001:EDX.[29] = 1
 */
#define IA32_LSTAR                                                   0xC0000082

/**
 * @brief IA-32e Mode System Call Target Address <b>(R/W)</b>
 *
 * Not used, as the SYSCALL instruction is not recognized in compatibility mode.
 *
 * @remarks If CPUID.80000001:EDX.[29] = 1
 */
#define IA32_CSTAR                                                   0xC0000083

/**
 * System Call Flag Mask.
 *
 * @remarks If CPUID.80000001:EDX.[29] = 1
 */
#define IA32_FMASK                                                   0xC0000084

/**
 * Map of BASE Address of FS.
 *
 * @remarks If CPUID.80000001:EDX.[29] = 1
 */
#define IA32_FS_BASE                                                 0xC0000100

/**
 * Map of BASE Address of GS.
 *
 * @remarks If CPUID.80000001:EDX.[29] = 1
 */
#define IA32_GS_BASE                                                 0xC0000101

/**
 * Swap Target of BASE Address of GS.
 *
 * @remarks If CPUID.80000001:EDX.[29] = 1
 */
#define IA32_KERNEL_GS_BASE                                          0xC0000102

/**
 * Auxiliary TSC.
 *
 * @remarks If CPUID.80000001H: EDX[27] = 1 or CPUID.(EAX=7,ECX=0):ECX[bit 22] = 1
 */
#define IA32_TSC_AUX                                                 0xC0000103
typedef union
{
  struct
  {
	/**
	* [Bits 31:0] AUX. Auxiliary signature of TSC.
	*/
	uint64_t tsc_auxiliary_signature                                 : 32;
#define IA32_TSC_AUX_TSC_AUXILIARY_SIGNATURE_BIT                     0
#define IA32_TSC_AUX_TSC_AUXILIARY_SIGNATURE_FLAG                    0xFFFFFFFF
#define IA32_TSC_AUX_TSC_AUXILIARY_SIGNATURE_MASK                    0xFFFFFFFF
#define IA32_TSC_AUX_TSC_AUXILIARY_SIGNATURE(_)                      (((_) >> 0) & 0xFFFFFFFF)
	uint64_t reserved1                                               : 32;
  };

  uint64_t flags;
} tsc_aux_register;

/**
 * @}
 */

/**
 * @defgroup paging \
 *           Paging
 * @{
 */
/**
 * @defgroup paging_32 \
 *           32-Bit Paging
 *
 * A logical processor uses 32-bit paging if CR0.PG = 1 and CR4.PAE = 0. 32-bit paging translates 32-bit linear addresses
 * to 40-bit physical addresses. Although 40 bits corresponds to 1 TByte, linear addresses are limited to 32 bits; at most
 * 4 GBytes of linear-address space may be accessed at any given time.
 * 32-bit paging uses a hierarchy of paging structures to produce a translation for a linear address. CR3 is used to locate
 * the first paging-structure, the page directory. 32-bit paging may map linear addresses to either 4-KByte pages or
 * 4-MByte pages.
 *
 * @see Vol3A[4.5(4-LEVEL PAGING)] (reference)
 * @{
 */
/**
 * @brief Format of a 32-Bit Page-Directory Entry that Maps a 4-MByte Page
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] Present; must be 1 to map a 4-MByte page.
	*/
	uint32_t present                                                 : 1;
#define PDE_4MB_32_PRESENT_BIT                                       0
#define PDE_4MB_32_PRESENT_FLAG                                      0x01
#define PDE_4MB_32_PRESENT_MASK                                      0x01
#define PDE_4MB_32_PRESENT(_)                                        (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Read/write; if 0, writes may not be allowed to the 4-MByte page referenced by this entry.
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint32_t write                                                   : 1;
#define PDE_4MB_32_WRITE_BIT                                         1
#define PDE_4MB_32_WRITE_FLAG                                        0x02
#define PDE_4MB_32_WRITE_MASK                                        0x01
#define PDE_4MB_32_WRITE(_)                                          (((_) >> 1) & 0x01)

	/**
	* [Bit 2] User/supervisor; if 0, user-mode accesses are not allowed to the 4-MByte page referenced by this entry.
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint32_t user                                                    : 1;
#define PDE_4MB_32_USER_BIT                                    2
#define PDE_4MB_32_USER_FLAG                                   0x04
#define PDE_4MB_32_USER_MASK                                   0x01
#define PDE_4MB_32_USER(_)                                     (((_) >> 2) & 0x01)

	/**
	* [Bit 3] Page-level write-through; indirectly determines the memory type used to access the 4-MByte page referenced by
	* this entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint32_t page_level_write_through                                : 1;
#define PDE_4MB_32_PAGE_LEVEL_WRITE_THROUGH_BIT                      3
#define PDE_4MB_32_PAGE_LEVEL_WRITE_THROUGH_FLAG                     0x08
#define PDE_4MB_32_PAGE_LEVEL_WRITE_THROUGH_MASK                     0x01
#define PDE_4MB_32_PAGE_LEVEL_WRITE_THROUGH(_)                       (((_) >> 3) & 0x01)

	/**
	* [Bit 4] Page-level cache disable; indirectly determines the memory type used to access the 4-MByte page referenced by
	* this entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint32_t page_level_cache_disable                                : 1;
#define PDE_4MB_32_PAGE_LEVEL_CACHE_DISABLE_BIT                      4
#define PDE_4MB_32_PAGE_LEVEL_CACHE_DISABLE_FLAG                     0x10
#define PDE_4MB_32_PAGE_LEVEL_CACHE_DISABLE_MASK                     0x01
#define PDE_4MB_32_PAGE_LEVEL_CACHE_DISABLE(_)                       (((_) >> 4) & 0x01)

	/**
	* [Bit 5] Accessed; indicates whether software has accessed the 4-MByte page referenced by this entry.
	*
	* @see Vol3A[4.8(Accessed and Dirty Flags)]
	*/
	uint32_t accessed                                                : 1;
#define PDE_4MB_32_ACCESSED_BIT                                      5
#define PDE_4MB_32_ACCESSED_FLAG                                     0x20
#define PDE_4MB_32_ACCESSED_MASK                                     0x01
#define PDE_4MB_32_ACCESSED(_)                                       (((_) >> 5) & 0x01)

	/**
	* [Bit 6] Dirty; indicates whether software has written to the 4-MByte page referenced by this entry.
	*
	* @see Vol3A[4.8(Accessed and Dirty Flags)]
	*/
	uint32_t dirty                                                   : 1;
#define PDE_4MB_32_DIRTY_BIT                                         6
#define PDE_4MB_32_DIRTY_FLAG                                        0x40
#define PDE_4MB_32_DIRTY_MASK                                        0x01
#define PDE_4MB_32_DIRTY(_)                                          (((_) >> 6) & 0x01)

	/**
	* [Bit 7] Page size; must be 1 (otherwise, this entry references a page table).
	*/
	uint32_t large_page                                              : 1;
#define PDE_4MB_32_LARGE_PAGE_BIT                                    7
#define PDE_4MB_32_LARGE_PAGE_FLAG                                   0x80
#define PDE_4MB_32_LARGE_PAGE_MASK                                   0x01
#define PDE_4MB_32_LARGE_PAGE(_)                                     (((_) >> 7) & 0x01)

	/**
	* [Bit 8] Global; if CR4.PGE = 1, determines whether the translation is global; ignored otherwise.
	*
	* @see Vol3A[4.10(Caching Translation Information)]
	*/
	uint32_t global                                                  : 1;
#define PDE_4MB_32_GLOBAL_BIT                                        8
#define PDE_4MB_32_GLOBAL_FLAG                                       0x100
#define PDE_4MB_32_GLOBAL_MASK                                       0x01
#define PDE_4MB_32_GLOBAL(_)                                         (((_) >> 8) & 0x01)

	/**
	* [Bits 11:9] Ignored.
	*/
	uint32_t ignored_1                                               : 3;
#define PDE_4MB_32_IGNORED_1_BIT                                     9
#define PDE_4MB_32_IGNORED_1_FLAG                                    0xE00
#define PDE_4MB_32_IGNORED_1_MASK                                    0x07
#define PDE_4MB_32_IGNORED_1(_)                                      (((_) >> 9) & 0x07)

	/**
	* [Bit 12] Indirectly determines the memory type used to access the 4-MByte page referenced by this entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint32_t pat                                                     : 1;
#define PDE_4MB_32_PAT_BIT                                           12
#define PDE_4MB_32_PAT_FLAG                                          0x1000
#define PDE_4MB_32_PAT_MASK                                          0x01
#define PDE_4MB_32_PAT(_)                                            (((_) >> 12) & 0x01)

	/**
	* [Bits 20:13] Bits (M-1):32 of physical address of the 4-MByte page referenced by this entry.
	*/
	uint32_t page_frame_number_low                                   : 8;
#define PDE_4MB_32_PAGE_FRAME_NUMBER_LOW_BIT                         13
#define PDE_4MB_32_PAGE_FRAME_NUMBER_LOW_FLAG                        0x1FE000
#define PDE_4MB_32_PAGE_FRAME_NUMBER_LOW_MASK                        0xFF
#define PDE_4MB_32_PAGE_FRAME_NUMBER_LOW(_)                          (((_) >> 13) & 0xFF)
	uint32_t reserved1                                               : 1;

	/**
	* [Bits 31:22] Bits 31:22 of physical address of the 4-MByte page referenced by this entry.
	*/
	uint32_t page_frame_number_high                                  : 10;
#define PDE_4MB_32_PAGE_FRAME_NUMBER_HIGH_BIT                        22
#define PDE_4MB_32_PAGE_FRAME_NUMBER_HIGH_FLAG                       0xFFC00000
#define PDE_4MB_32_PAGE_FRAME_NUMBER_HIGH_MASK                       0x3FF
#define PDE_4MB_32_PAGE_FRAME_NUMBER_HIGH(_)                         (((_) >> 22) & 0x3FF)
  };

  uint32_t flags;
} pde_4mb_32;

/**
 * @brief Format of a 32-Bit Page-Directory Entry that References a Page Table
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] Present; must be 1 to reference a page table.
	*/
	uint32_t present                                                 : 1;
#define PDE_32_PRESENT_BIT                                           0
#define PDE_32_PRESENT_FLAG                                          0x01
#define PDE_32_PRESENT_MASK                                          0x01
#define PDE_32_PRESENT(_)                                            (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Read/write; if 0, writes may not be allowed to the 4-MByte region controlled by this entry.
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint32_t write                                                   : 1;
#define PDE_32_WRITE_BIT                                             1
#define PDE_32_WRITE_FLAG                                            0x02
#define PDE_32_WRITE_MASK                                            0x01
#define PDE_32_WRITE(_)                                              (((_) >> 1) & 0x01)

	/**
	* [Bit 2] User/supervisor; if 0, user-mode accesses are not allowed to the 4-MByte region controlled by this entry.
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint32_t user                                                    : 1;
#define PDE_32_USER_BIT                                        2
#define PDE_32_USER_FLAG                                       0x04
#define PDE_32_USER_MASK                                       0x01
#define PDE_32_USER(_)                                         (((_) >> 2) & 0x01)

	/**
	* [Bit 3] Page-level write-through; indirectly determines the memory type used to access the page table referenced by this
	* entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint32_t page_level_write_through                                : 1;
#define PDE_32_PAGE_LEVEL_WRITE_THROUGH_BIT                          3
#define PDE_32_PAGE_LEVEL_WRITE_THROUGH_FLAG                         0x08
#define PDE_32_PAGE_LEVEL_WRITE_THROUGH_MASK                         0x01
#define PDE_32_PAGE_LEVEL_WRITE_THROUGH(_)                           (((_) >> 3) & 0x01)

	/**
	* [Bit 4] Page-level cache disable; indirectly determines the memory type used to access the page table referenced by this
	* entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint32_t page_level_cache_disable                                : 1;
#define PDE_32_PAGE_LEVEL_CACHE_DISABLE_BIT                          4
#define PDE_32_PAGE_LEVEL_CACHE_DISABLE_FLAG                         0x10
#define PDE_32_PAGE_LEVEL_CACHE_DISABLE_MASK                         0x01
#define PDE_32_PAGE_LEVEL_CACHE_DISABLE(_)                           (((_) >> 4) & 0x01)

	/**
	* [Bit 5] Accessed; indicates whether this entry has been used for linear-address translation.
	*
	* @see Vol3A[4.8(Accessed and Dirty Flags)]
	*/
	uint32_t accessed                                                : 1;
#define PDE_32_ACCESSED_BIT                                          5
#define PDE_32_ACCESSED_FLAG                                         0x20
#define PDE_32_ACCESSED_MASK                                         0x01
#define PDE_32_ACCESSED(_)                                           (((_) >> 5) & 0x01)

	/**
	* [Bit 6] Ignored.
	*/
	uint32_t ignored_1                                               : 1;
#define PDE_32_IGNORED_1_BIT                                         6
#define PDE_32_IGNORED_1_FLAG                                        0x40
#define PDE_32_IGNORED_1_MASK                                        0x01
#define PDE_32_IGNORED_1(_)                                          (((_) >> 6) & 0x01)

	/**
	* [Bit 7] If CR4.PSE = 1, must be 0 (otherwise, this entry maps a 4-MByte page); otherwise, ignored.
	*/
	uint32_t large_page                                              : 1;
#define PDE_32_LARGE_PAGE_BIT                                        7
#define PDE_32_LARGE_PAGE_FLAG                                       0x80
#define PDE_32_LARGE_PAGE_MASK                                       0x01
#define PDE_32_LARGE_PAGE(_)                                         (((_) >> 7) & 0x01)

	/**
	* [Bits 11:8] Ignored.
	*/
	uint32_t ignored_2                                               : 4;
#define PDE_32_IGNORED_2_BIT                                         8
#define PDE_32_IGNORED_2_FLAG                                        0xF00
#define PDE_32_IGNORED_2_MASK                                        0x0F
#define PDE_32_IGNORED_2(_)                                          (((_) >> 8) & 0x0F)

	/**
	* [Bits 31:12] Physical address of 4-KByte aligned page table referenced by this entry.
	*/
	uint32_t page_frame_number                                       : 20;
#define PDE_32_PAGE_FRAME_NUMBER_BIT                                 12
#define PDE_32_PAGE_FRAME_NUMBER_FLAG                                0xFFFFF000
#define PDE_32_PAGE_FRAME_NUMBER_MASK                                0xFFFFF
#define PDE_32_PAGE_FRAME_NUMBER(_)                                  (((_) >> 12) & 0xFFFFF)
  };

  uint32_t flags;
} pde_32;

/**
 * @brief Format of a 32-Bit Page-Table Entry that Maps a 4-KByte Page
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] Present; must be 1 to map a 4-KByte page.
	*/
	uint32_t present                                                 : 1;
#define PTE_32_PRESENT_BIT                                           0
#define PTE_32_PRESENT_FLAG                                          0x01
#define PTE_32_PRESENT_MASK                                          0x01
#define PTE_32_PRESENT(_)                                            (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Read/write; if 0, writes may not be allowed to the 4-KByte page referenced by this entry.
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint32_t write                                                   : 1;
#define PTE_32_WRITE_BIT                                             1
#define PTE_32_WRITE_FLAG                                            0x02
#define PTE_32_WRITE_MASK                                            0x01
#define PTE_32_WRITE(_)                                              (((_) >> 1) & 0x01)

	/**
	* [Bit 2] User/supervisor; if 0, user-mode accesses are not allowed to the 4-KByte page referenced by this entry.
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint32_t user                                                    : 1;
#define PTE_32_USER_BIT                                        2
#define PTE_32_USER_FLAG                                       0x04
#define PTE_32_USER_MASK                                       0x01
#define PTE_32_USER(_)                                         (((_) >> 2) & 0x01)

	/**
	* [Bit 3] Page-level write-through; indirectly determines the memory type used to access the 4-KByte page referenced by
	* this entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint32_t page_level_write_through                                : 1;
#define PTE_32_PAGE_LEVEL_WRITE_THROUGH_BIT                          3
#define PTE_32_PAGE_LEVEL_WRITE_THROUGH_FLAG                         0x08
#define PTE_32_PAGE_LEVEL_WRITE_THROUGH_MASK                         0x01
#define PTE_32_PAGE_LEVEL_WRITE_THROUGH(_)                           (((_) >> 3) & 0x01)

	/**
	* [Bit 4] Page-level cache disable; indirectly determines the memory type used to access the 4-KByte page referenced by
	* this entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint32_t page_level_cache_disable                                : 1;
#define PTE_32_PAGE_LEVEL_CACHE_DISABLE_BIT                          4
#define PTE_32_PAGE_LEVEL_CACHE_DISABLE_FLAG                         0x10
#define PTE_32_PAGE_LEVEL_CACHE_DISABLE_MASK                         0x01
#define PTE_32_PAGE_LEVEL_CACHE_DISABLE(_)                           (((_) >> 4) & 0x01)

	/**
	* [Bit 5] Accessed; indicates whether software has accessed the 4-KByte page referenced by this entry.
	*
	* @see Vol3A[4.8(Accessed and Dirty Flags)]
	*/
	uint32_t accessed                                                : 1;
#define PTE_32_ACCESSED_BIT                                          5
#define PTE_32_ACCESSED_FLAG                                         0x20
#define PTE_32_ACCESSED_MASK                                         0x01
#define PTE_32_ACCESSED(_)                                           (((_) >> 5) & 0x01)

	/**
	* [Bit 6] Dirty; indicates whether software has written to the 4-KByte page referenced by this entry.
	*
	* @see Vol3A[4.8(Accessed and Dirty Flags)]
	*/
	uint32_t dirty                                                   : 1;
#define PTE_32_DIRTY_BIT                                             6
#define PTE_32_DIRTY_FLAG                                            0x40
#define PTE_32_DIRTY_MASK                                            0x01
#define PTE_32_DIRTY(_)                                              (((_) >> 6) & 0x01)

	/**
	* [Bit 7] Indirectly determines the memory type used to access the 4-KByte page referenced by this entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint32_t pat                                                     : 1;
#define PTE_32_PAT_BIT                                               7
#define PTE_32_PAT_FLAG                                              0x80
#define PTE_32_PAT_MASK                                              0x01
#define PTE_32_PAT(_)                                                (((_) >> 7) & 0x01)

	/**
	* [Bit 8] Global; if CR4.PGE = 1, determines whether the translation is global; ignored otherwise.
	*
	* @see Vol3A[4.10(Caching Translation Information)]
	*/
	uint32_t global                                                  : 1;
#define PTE_32_GLOBAL_BIT                                            8
#define PTE_32_GLOBAL_FLAG                                           0x100
#define PTE_32_GLOBAL_MASK                                           0x01
#define PTE_32_GLOBAL(_)                                             (((_) >> 8) & 0x01)

	/**
	* [Bits 11:9] Ignored.
	*/
	uint32_t ignored_1                                               : 3;
#define PTE_32_IGNORED_1_BIT                                         9
#define PTE_32_IGNORED_1_FLAG                                        0xE00
#define PTE_32_IGNORED_1_MASK                                        0x07
#define PTE_32_IGNORED_1(_)                                          (((_) >> 9) & 0x07)

	/**
	* [Bits 31:12] Physical address of 4-KByte aligned page table referenced by this entry.
	*/
	uint32_t page_frame_number                                       : 20;
#define PTE_32_PAGE_FRAME_NUMBER_BIT                                 12
#define PTE_32_PAGE_FRAME_NUMBER_FLAG                                0xFFFFF000
#define PTE_32_PAGE_FRAME_NUMBER_MASK                                0xFFFFF
#define PTE_32_PAGE_FRAME_NUMBER(_)                                  (((_) >> 12) & 0xFFFFF)
  };

  uint32_t flags;
} pte_32;

/**
 * @brief Format of a common Page-Table Entry
 */
typedef union
{
  struct
  {
	uint32_t present                                                 : 1;
#define PT_ENTRY_32_PRESENT_BIT                                      0
#define PT_ENTRY_32_PRESENT_FLAG                                     0x01
#define PT_ENTRY_32_PRESENT_MASK                                     0x01
#define PT_ENTRY_32_PRESENT(_)                                       (((_) >> 0) & 0x01)
	uint32_t write                                                   : 1;
#define PT_ENTRY_32_WRITE_BIT                                        1
#define PT_ENTRY_32_WRITE_FLAG                                       0x02
#define PT_ENTRY_32_WRITE_MASK                                       0x01
#define PT_ENTRY_32_WRITE(_)                                         (((_) >> 1) & 0x01)
	uint32_t user                                                    : 1;
#define PT_ENTRY_32_USER_BIT                                   2
#define PT_ENTRY_32_USER_FLAG                                  0x04
#define PT_ENTRY_32_USER_MASK                                  0x01
#define PT_ENTRY_32_USER(_)                                    (((_) >> 2) & 0x01)
	uint32_t page_level_write_through                                : 1;
#define PT_ENTRY_32_PAGE_LEVEL_WRITE_THROUGH_BIT                     3
#define PT_ENTRY_32_PAGE_LEVEL_WRITE_THROUGH_FLAG                    0x08
#define PT_ENTRY_32_PAGE_LEVEL_WRITE_THROUGH_MASK                    0x01
#define PT_ENTRY_32_PAGE_LEVEL_WRITE_THROUGH(_)                      (((_) >> 3) & 0x01)
	uint32_t page_level_cache_disable                                : 1;
#define PT_ENTRY_32_PAGE_LEVEL_CACHE_DISABLE_BIT                     4
#define PT_ENTRY_32_PAGE_LEVEL_CACHE_DISABLE_FLAG                    0x10
#define PT_ENTRY_32_PAGE_LEVEL_CACHE_DISABLE_MASK                    0x01
#define PT_ENTRY_32_PAGE_LEVEL_CACHE_DISABLE(_)                      (((_) >> 4) & 0x01)
	uint32_t accessed                                                : 1;
#define PT_ENTRY_32_ACCESSED_BIT                                     5
#define PT_ENTRY_32_ACCESSED_FLAG                                    0x20
#define PT_ENTRY_32_ACCESSED_MASK                                    0x01
#define PT_ENTRY_32_ACCESSED(_)                                      (((_) >> 5) & 0x01)
	uint32_t dirty                                                   : 1;
#define PT_ENTRY_32_DIRTY_BIT                                        6
#define PT_ENTRY_32_DIRTY_FLAG                                       0x40
#define PT_ENTRY_32_DIRTY_MASK                                       0x01
#define PT_ENTRY_32_DIRTY(_)                                         (((_) >> 6) & 0x01)
	uint32_t large_page                                              : 1;
#define PT_ENTRY_32_LARGE_PAGE_BIT                                   7
#define PT_ENTRY_32_LARGE_PAGE_FLAG                                  0x80
#define PT_ENTRY_32_LARGE_PAGE_MASK                                  0x01
#define PT_ENTRY_32_LARGE_PAGE(_)                                    (((_) >> 7) & 0x01)
	uint32_t global                                                  : 1;
#define PT_ENTRY_32_GLOBAL_BIT                                       8
#define PT_ENTRY_32_GLOBAL_FLAG                                      0x100
#define PT_ENTRY_32_GLOBAL_MASK                                      0x01
#define PT_ENTRY_32_GLOBAL(_)                                        (((_) >> 8) & 0x01)

	/**
	* [Bits 11:9] Ignored.
	*/
	uint32_t ignored_1                                               : 3;
#define PT_ENTRY_32_IGNORED_1_BIT                                    9
#define PT_ENTRY_32_IGNORED_1_FLAG                                   0xE00
#define PT_ENTRY_32_IGNORED_1_MASK                                   0x07
#define PT_ENTRY_32_IGNORED_1(_)                                     (((_) >> 9) & 0x07)

	/**
	* [Bits 31:12] Physical address of the 4-KByte page referenced by this entry.
	*/
	uint32_t page_frame_number                                       : 20;
#define PT_ENTRY_32_PAGE_FRAME_NUMBER_BIT                            12
#define PT_ENTRY_32_PAGE_FRAME_NUMBER_FLAG                           0xFFFFF000
#define PT_ENTRY_32_PAGE_FRAME_NUMBER_MASK                           0xFFFFF
#define PT_ENTRY_32_PAGE_FRAME_NUMBER(_)                             (((_) >> 12) & 0xFFFFF)
  };

  uint32_t flags;
} pt_entry_32;

/**
 * @defgroup paging_structures_entry_count_32 \
 *           Paging structures entry counts
 *
 * Paging structures entry counts.
 * @{
 */
#define PDE_ENTRY_COUNT_32                                           0x00000400
#define PTE_ENTRY_COUNT_32                                           0x00000400
/**
 * @}
 */

/**
 * @}
 */

/**
 * @defgroup paging_64 \
 *           64-Bit (4-Level) Paging
 *
 * A logical processor uses 4-level paging if CR0.PG = 1, CR4.PAE = 1, and IA32_EFER.LME = 1. With 4-level paging, linear
 * address are translated using a hierarchy of in-memory paging structures located using the contents of CR3. 4-level
 * paging translates 48-bit linear addresses to 52-bit physical addresses. Although 52 bits corresponds to 4 PBytes, linear
 * addresses are limited to 48 bits; at most 256 TBytes of linear-address space may be accessed at any given time.
 * 4-level paging uses a hierarchy of paging structures to produce a translation for a linear address. CR3 is used to
 * locate the first paging-structure, the PML4 table. Use of CR3 with 4-level paging depends on whether processcontext
 * identifiers (PCIDs) have been enabled by setting CR4.PCIDE.
 *
 * @see Vol3A[4.5(4-LEVEL PAGING)] (reference)
 * @{
 */
/**
 * @brief Format of a 4-Level PML4 Entry (PML4E) that References a Page-Directory-Pointer Table
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] Present; must be 1 to reference a page-directory-pointer table.
	*/
	uint64_t present                                                 : 1;
#define PML4E_64_PRESENT_BIT                                         0
#define PML4E_64_PRESENT_FLAG                                        0x01
#define PML4E_64_PRESENT_MASK                                        0x01
#define PML4E_64_PRESENT(_)                                          (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Read/write; if 0, writes may not be allowed to the 512-GByte region controlled by this entry.
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint64_t write                                                   : 1;
#define PML4E_64_WRITE_BIT                                           1
#define PML4E_64_WRITE_FLAG                                          0x02
#define PML4E_64_WRITE_MASK                                          0x01
#define PML4E_64_WRITE(_)                                            (((_) >> 1) & 0x01)

	/**
	* [Bit 2] User/supervisor; if 0, user-mode accesses are not allowed to the 512-GByte region controlled by this entry.
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint64_t user                                                    : 1;
#define PML4E_64_USER_BIT                                      2
#define PML4E_64_USER_FLAG                                     0x04
#define PML4E_64_USER_MASK                                     0x01
#define PML4E_64_USER(_)                                       (((_) >> 2) & 0x01)

	/**
	* [Bit 3] Page-level write-through; indirectly determines the memory type used to access the page-directory-pointer table
	* referenced by this entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint64_t page_level_write_through                                : 1;
#define PML4E_64_PAGE_LEVEL_WRITE_THROUGH_BIT                        3
#define PML4E_64_PAGE_LEVEL_WRITE_THROUGH_FLAG                       0x08
#define PML4E_64_PAGE_LEVEL_WRITE_THROUGH_MASK                       0x01
#define PML4E_64_PAGE_LEVEL_WRITE_THROUGH(_)                         (((_) >> 3) & 0x01)

	/**
	* [Bit 4] Page-level cache disable; indirectly determines the memory type used to access the page-directory-pointer table
	* referenced by this entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint64_t page_level_cache_disable                                : 1;
#define PML4E_64_PAGE_LEVEL_CACHE_DISABLE_BIT                        4
#define PML4E_64_PAGE_LEVEL_CACHE_DISABLE_FLAG                       0x10
#define PML4E_64_PAGE_LEVEL_CACHE_DISABLE_MASK                       0x01
#define PML4E_64_PAGE_LEVEL_CACHE_DISABLE(_)                         (((_) >> 4) & 0x01)

	/**
	* [Bit 5] Accessed; indicates whether this entry has been used for linear-address translation.
	*
	* @see Vol3A[4.8(Accessed and Dirty Flags)]
	*/
	uint64_t accessed                                                : 1;
#define PML4E_64_ACCESSED_BIT                                        5
#define PML4E_64_ACCESSED_FLAG                                       0x20
#define PML4E_64_ACCESSED_MASK                                       0x01
#define PML4E_64_ACCESSED(_)                                         (((_) >> 5) & 0x01)
	uint64_t reserved1                                               : 1;

	/**
	* [Bit 7] Reserved (must be 0).
	*/
	uint64_t must_be_zero                                            : 1;
#define PML4E_64_MUST_BE_ZERO_BIT                                    7
#define PML4E_64_MUST_BE_ZERO_FLAG                                   0x80
#define PML4E_64_MUST_BE_ZERO_MASK                                   0x01
#define PML4E_64_MUST_BE_ZERO(_)                                     (((_) >> 7) & 0x01)

	/**
	* [Bits 11:8] Ignored.
	*/
	uint64_t ignored_1                                               : 4;
#define PML4E_64_IGNORED_1_BIT                                       8
#define PML4E_64_IGNORED_1_FLAG                                      0xF00
#define PML4E_64_IGNORED_1_MASK                                      0x0F
#define PML4E_64_IGNORED_1(_)                                        (((_) >> 8) & 0x0F)

	/**
	* [Bits 47:12] Physical address of 4-KByte aligned page-directory-pointer table referenced by this entry.
	*/
	uint64_t page_frame_number                                       : 36;
#define PML4E_64_PAGE_FRAME_NUMBER_BIT                               12
#define PML4E_64_PAGE_FRAME_NUMBER_FLAG                              0xFFFFFFFFF000
#define PML4E_64_PAGE_FRAME_NUMBER_MASK                              0xFFFFFFFFF
#define PML4E_64_PAGE_FRAME_NUMBER(_)                                (((_) >> 12) & 0xFFFFFFFFF)
	uint64_t reserved2                                               : 4;

	/**
	* [Bits 62:52] Ignored.
	*/
	uint64_t ignored_2                                               : 11;
#define PML4E_64_IGNORED_2_BIT                                       52
#define PML4E_64_IGNORED_2_FLAG                                      0x7FF0000000000000
#define PML4E_64_IGNORED_2_MASK                                      0x7FF
#define PML4E_64_IGNORED_2(_)                                        (((_) >> 52) & 0x7FF)

	/**
	* [Bit 63] If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 512-GByte region
	* controlled by this entry); otherwise, reserved (must be 0).
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint64_t execute_disable                                         : 1;
#define PML4E_64_EXECUTE_DISABLE_BIT                                 63
#define PML4E_64_EXECUTE_DISABLE_FLAG                                0x8000000000000000
#define PML4E_64_EXECUTE_DISABLE_MASK                                0x01
#define PML4E_64_EXECUTE_DISABLE(_)                                  (((_) >> 63) & 0x01)
  };

  uint64_t flags;
} pml4e_64;

/**
 * @brief Format of a 4-Level Page-Directory-Pointer-Table Entry (PDPTE) that Maps a 1-GByte Page
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] Present; must be 1 to map a 1-GByte page.
	*/
	uint64_t present                                                 : 1;
#define PDPTE_1GB_64_PRESENT_BIT                                     0
#define PDPTE_1GB_64_PRESENT_FLAG                                    0x01
#define PDPTE_1GB_64_PRESENT_MASK                                    0x01
#define PDPTE_1GB_64_PRESENT(_)                                      (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Read/write; if 0, writes may not be allowed to the 1-GByte page referenced by this entry.
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint64_t write                                                   : 1;
#define PDPTE_1GB_64_WRITE_BIT                                       1
#define PDPTE_1GB_64_WRITE_FLAG                                      0x02
#define PDPTE_1GB_64_WRITE_MASK                                      0x01
#define PDPTE_1GB_64_WRITE(_)                                        (((_) >> 1) & 0x01)

	/**
	* [Bit 2] User/supervisor; if 0, user-mode accesses are not allowed to the 1-GByte page referenced by this entry.
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint64_t user                                                    : 1;
#define PDPTE_1GB_64_USER_BIT                                  2
#define PDPTE_1GB_64_USER_FLAG                                 0x04
#define PDPTE_1GB_64_USER_MASK                                 0x01
#define PDPTE_1GB_64_USER(_)                                   (((_) >> 2) & 0x01)

	/**
	* [Bit 3] Page-level write-through; indirectly determines the memory type used to access the 1-GByte page referenced by
	* this entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint64_t page_level_write_through                                : 1;
#define PDPTE_1GB_64_PAGE_LEVEL_WRITE_THROUGH_BIT                    3
#define PDPTE_1GB_64_PAGE_LEVEL_WRITE_THROUGH_FLAG                   0x08
#define PDPTE_1GB_64_PAGE_LEVEL_WRITE_THROUGH_MASK                   0x01
#define PDPTE_1GB_64_PAGE_LEVEL_WRITE_THROUGH(_)                     (((_) >> 3) & 0x01)

	/**
	* [Bit 4] Page-level cache disable; indirectly determines the memory type used to access the 1-GByte page referenced by
	* this entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint64_t page_level_cache_disable                                : 1;
#define PDPTE_1GB_64_PAGE_LEVEL_CACHE_DISABLE_BIT                    4
#define PDPTE_1GB_64_PAGE_LEVEL_CACHE_DISABLE_FLAG                   0x10
#define PDPTE_1GB_64_PAGE_LEVEL_CACHE_DISABLE_MASK                   0x01
#define PDPTE_1GB_64_PAGE_LEVEL_CACHE_DISABLE(_)                     (((_) >> 4) & 0x01)

	/**
	* [Bit 5] Accessed; indicates whether software has accessed the 1-GByte page referenced by this entry.
	*
	* @see Vol3A[4.8(Accessed and Dirty Flags)]
	*/
	uint64_t accessed                                                : 1;
#define PDPTE_1GB_64_ACCESSED_BIT                                    5
#define PDPTE_1GB_64_ACCESSED_FLAG                                   0x20
#define PDPTE_1GB_64_ACCESSED_MASK                                   0x01
#define PDPTE_1GB_64_ACCESSED(_)                                     (((_) >> 5) & 0x01)

	/**
	* [Bit 6] Dirty; indicates whether software has written to the 1-GByte page referenced by this entry.
	*
	* @see Vol3A[4.8(Accessed and Dirty Flags)]
	*/
	uint64_t dirty                                                   : 1;
#define PDPTE_1GB_64_DIRTY_BIT                                       6
#define PDPTE_1GB_64_DIRTY_FLAG                                      0x40
#define PDPTE_1GB_64_DIRTY_MASK                                      0x01
#define PDPTE_1GB_64_DIRTY(_)                                        (((_) >> 6) & 0x01)

	/**
	* [Bit 7] Page size; must be 1 (otherwise, this entry references a page directory).
	*/
	uint64_t large_page                                              : 1;
#define PDPTE_1GB_64_LARGE_PAGE_BIT                                  7
#define PDPTE_1GB_64_LARGE_PAGE_FLAG                                 0x80
#define PDPTE_1GB_64_LARGE_PAGE_MASK                                 0x01
#define PDPTE_1GB_64_LARGE_PAGE(_)                                   (((_) >> 7) & 0x01)

	/**
	* [Bit 8] Global; if CR4.PGE = 1, determines whether the translation is global; ignored otherwise.
	*
	* @see Vol3A[4.10(Caching Translation Information)]
	*/
	uint64_t global                                                  : 1;
#define PDPTE_1GB_64_GLOBAL_BIT                                      8
#define PDPTE_1GB_64_GLOBAL_FLAG                                     0x100
#define PDPTE_1GB_64_GLOBAL_MASK                                     0x01
#define PDPTE_1GB_64_GLOBAL(_)                                       (((_) >> 8) & 0x01)

	/**
	* [Bits 11:9] Ignored.
	*/
	uint64_t ignored_1                                               : 3;
#define PDPTE_1GB_64_IGNORED_1_BIT                                   9
#define PDPTE_1GB_64_IGNORED_1_FLAG                                  0xE00
#define PDPTE_1GB_64_IGNORED_1_MASK                                  0x07
#define PDPTE_1GB_64_IGNORED_1(_)                                    (((_) >> 9) & 0x07)

	/**
	* [Bit 12] Indirectly determines the memory type used to access the 1-GByte page referenced by this entry.
	*
	* @note The PAT is supported on all processors that support 4-level paging.
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint64_t pat                                                     : 1;
#define PDPTE_1GB_64_PAT_BIT                                         12
#define PDPTE_1GB_64_PAT_FLAG                                        0x1000
#define PDPTE_1GB_64_PAT_MASK                                        0x01
#define PDPTE_1GB_64_PAT(_)                                          (((_) >> 12) & 0x01)
	uint64_t reserved1                                               : 17;

	/**
	* [Bits 47:30] Physical address of the 1-GByte page referenced by this entry.
	*/
	uint64_t page_frame_number                                       : 18;
#define PDPTE_1GB_64_PAGE_FRAME_NUMBER_BIT                           30
#define PDPTE_1GB_64_PAGE_FRAME_NUMBER_FLAG                          0xFFFFC0000000
#define PDPTE_1GB_64_PAGE_FRAME_NUMBER_MASK                          0x3FFFF
#define PDPTE_1GB_64_PAGE_FRAME_NUMBER(_)                            (((_) >> 30) & 0x3FFFF)
	uint64_t reserved2                                               : 4;

	/**
	* [Bits 58:52] Ignored.
	*/
	uint64_t ignored_2                                               : 7;
#define PDPTE_1GB_64_IGNORED_2_BIT                                   52
#define PDPTE_1GB_64_IGNORED_2_FLAG                                  0x7F0000000000000
#define PDPTE_1GB_64_IGNORED_2_MASK                                  0x7F
#define PDPTE_1GB_64_IGNORED_2(_)                                    (((_) >> 52) & 0x7F)

	/**
	* [Bits 62:59] Protection key; if CR4.PKE = 1, determines the protection key of the page; ignored otherwise.
	*
	* @see Vol3A[4.6.2(Protection Keys)]
	*/
	uint64_t protection_key                                          : 4;
#define PDPTE_1GB_64_PROTECTION_KEY_BIT                              59
#define PDPTE_1GB_64_PROTECTION_KEY_FLAG                             0x7800000000000000
#define PDPTE_1GB_64_PROTECTION_KEY_MASK                             0x0F
#define PDPTE_1GB_64_PROTECTION_KEY(_)                               (((_) >> 59) & 0x0F)

	/**
	* [Bit 63] If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 1-GByte page
	* controlled by this entry); otherwise, reserved (must be 0).
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint64_t execute_disable                                         : 1;
#define PDPTE_1GB_64_EXECUTE_DISABLE_BIT                             63
#define PDPTE_1GB_64_EXECUTE_DISABLE_FLAG                            0x8000000000000000
#define PDPTE_1GB_64_EXECUTE_DISABLE_MASK                            0x01
#define PDPTE_1GB_64_EXECUTE_DISABLE(_)                              (((_) >> 63) & 0x01)
  };

  uint64_t flags;
} pdpte_1gb_64;

/**
 * @brief Format of a 4-Level Page-Directory-Pointer-Table Entry (PDPTE) that References a Page Directory
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] Present; must be 1 to reference a page directory.
	*/
	uint64_t present                                                 : 1;
#define PDPTE_64_PRESENT_BIT                                         0
#define PDPTE_64_PRESENT_FLAG                                        0x01
#define PDPTE_64_PRESENT_MASK                                        0x01
#define PDPTE_64_PRESENT(_)                                          (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Read/write; if 0, writes may not be allowed to the 1-GByte region controlled by this entry.
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint64_t write                                                   : 1;
#define PDPTE_64_WRITE_BIT                                           1
#define PDPTE_64_WRITE_FLAG                                          0x02
#define PDPTE_64_WRITE_MASK                                          0x01
#define PDPTE_64_WRITE(_)                                            (((_) >> 1) & 0x01)

	/**
	* [Bit 2] User/supervisor; if 0, user-mode accesses are not allowed to the 1-GByte region controlled by this entry.
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint64_t user                                                    : 1;
#define PDPTE_64_USER_BIT                                      2
#define PDPTE_64_USER_FLAG                                     0x04
#define PDPTE_64_USER_MASK                                     0x01
#define PDPTE_64_USER(_)                                       (((_) >> 2) & 0x01)

	/**
	* [Bit 3] Page-level write-through; indirectly determines the memory type used to access the page directory referenced by
	* this entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint64_t page_level_write_through                                : 1;
#define PDPTE_64_PAGE_LEVEL_WRITE_THROUGH_BIT                        3
#define PDPTE_64_PAGE_LEVEL_WRITE_THROUGH_FLAG                       0x08
#define PDPTE_64_PAGE_LEVEL_WRITE_THROUGH_MASK                       0x01
#define PDPTE_64_PAGE_LEVEL_WRITE_THROUGH(_)                         (((_) >> 3) & 0x01)

	/**
	* [Bit 4] Page-level cache disable; indirectly determines the memory type used to access the page directory referenced by
	* this entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint64_t page_level_cache_disable                                : 1;
#define PDPTE_64_PAGE_LEVEL_CACHE_DISABLE_BIT                        4
#define PDPTE_64_PAGE_LEVEL_CACHE_DISABLE_FLAG                       0x10
#define PDPTE_64_PAGE_LEVEL_CACHE_DISABLE_MASK                       0x01
#define PDPTE_64_PAGE_LEVEL_CACHE_DISABLE(_)                         (((_) >> 4) & 0x01)

	/**
	* [Bit 5] Accessed; indicates whether this entry has been used for linear-address translation.
	*
	* @see Vol3A[4.8(Accessed and Dirty Flags)]
	*/
	uint64_t accessed                                                : 1;
#define PDPTE_64_ACCESSED_BIT                                        5
#define PDPTE_64_ACCESSED_FLAG                                       0x20
#define PDPTE_64_ACCESSED_MASK                                       0x01
#define PDPTE_64_ACCESSED(_)                                         (((_) >> 5) & 0x01)
	uint64_t reserved1                                               : 1;

	/**
	* [Bit 7] Page size; must be 0 (otherwise, this entry maps a 1-GByte page).
	*/
	uint64_t large_page                                              : 1;
#define PDPTE_64_LARGE_PAGE_BIT                                      7
#define PDPTE_64_LARGE_PAGE_FLAG                                     0x80
#define PDPTE_64_LARGE_PAGE_MASK                                     0x01
#define PDPTE_64_LARGE_PAGE(_)                                       (((_) >> 7) & 0x01)

	/**
	* [Bits 11:8] Ignored.
	*/
	uint64_t ignored_1                                               : 4;
#define PDPTE_64_IGNORED_1_BIT                                       8
#define PDPTE_64_IGNORED_1_FLAG                                      0xF00
#define PDPTE_64_IGNORED_1_MASK                                      0x0F
#define PDPTE_64_IGNORED_1(_)                                        (((_) >> 8) & 0x0F)

	/**
	* [Bits 47:12] Physical address of 4-KByte aligned page directory referenced by this entry.
	*/
	uint64_t page_frame_number                                       : 36;
#define PDPTE_64_PAGE_FRAME_NUMBER_BIT                               12
#define PDPTE_64_PAGE_FRAME_NUMBER_FLAG                              0xFFFFFFFFF000
#define PDPTE_64_PAGE_FRAME_NUMBER_MASK                              0xFFFFFFFFF
#define PDPTE_64_PAGE_FRAME_NUMBER(_)                                (((_) >> 12) & 0xFFFFFFFFF)
	uint64_t reserved2                                               : 4;

	/**
	* [Bits 62:52] Ignored.
	*/
	uint64_t ignored_2                                               : 11;
#define PDPTE_64_IGNORED_2_BIT                                       52
#define PDPTE_64_IGNORED_2_FLAG                                      0x7FF0000000000000
#define PDPTE_64_IGNORED_2_MASK                                      0x7FF
#define PDPTE_64_IGNORED_2(_)                                        (((_) >> 52) & 0x7FF)

	/**
	* [Bit 63] If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 1-GByte region
	* controlled by this entry); otherwise, reserved (must be 0).
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint64_t execute_disable                                         : 1;
#define PDPTE_64_EXECUTE_DISABLE_BIT                                 63
#define PDPTE_64_EXECUTE_DISABLE_FLAG                                0x8000000000000000
#define PDPTE_64_EXECUTE_DISABLE_MASK                                0x01
#define PDPTE_64_EXECUTE_DISABLE(_)                                  (((_) >> 63) & 0x01)
  };

  uint64_t flags;
} pdpte_64;

/**
 * @brief Format of a 4-Level Page-Directory Entry that Maps a 2-MByte Page
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] Present; must be 1 to map a 2-MByte page.
	*/
	uint64_t present                                                 : 1;
#define PDE_2MB_64_PRESENT_BIT                                       0
#define PDE_2MB_64_PRESENT_FLAG                                      0x01
#define PDE_2MB_64_PRESENT_MASK                                      0x01
#define PDE_2MB_64_PRESENT(_)                                        (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Read/write; if 0, writes may not be allowed to the 2-MByte page referenced by this entry.
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint64_t write                                                   : 1;
#define PDE_2MB_64_WRITE_BIT                                         1
#define PDE_2MB_64_WRITE_FLAG                                        0x02
#define PDE_2MB_64_WRITE_MASK                                        0x01
#define PDE_2MB_64_WRITE(_)                                          (((_) >> 1) & 0x01)

	/**
	* [Bit 2] User/supervisor; if 0, user-mode accesses are not allowed to the 2-MByte page referenced by this entry.
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint64_t user                                                    : 1;
#define PDE_2MB_64_USER_BIT                                    2
#define PDE_2MB_64_USER_FLAG                                   0x04
#define PDE_2MB_64_USER_MASK                                   0x01
#define PDE_2MB_64_USER(_)                                     (((_) >> 2) & 0x01)

	/**
	* [Bit 3] Page-level write-through; indirectly determines the memory type used to access the 2-MByte page referenced by
	* this entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint64_t page_level_write_through                                : 1;
#define PDE_2MB_64_PAGE_LEVEL_WRITE_THROUGH_BIT                      3
#define PDE_2MB_64_PAGE_LEVEL_WRITE_THROUGH_FLAG                     0x08
#define PDE_2MB_64_PAGE_LEVEL_WRITE_THROUGH_MASK                     0x01
#define PDE_2MB_64_PAGE_LEVEL_WRITE_THROUGH(_)                       (((_) >> 3) & 0x01)

	/**
	* [Bit 4] Page-level cache disable; indirectly determines the memory type used to access the 2-MByte page referenced by
	* this entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint64_t page_level_cache_disable                                : 1;
#define PDE_2MB_64_PAGE_LEVEL_CACHE_DISABLE_BIT                      4
#define PDE_2MB_64_PAGE_LEVEL_CACHE_DISABLE_FLAG                     0x10
#define PDE_2MB_64_PAGE_LEVEL_CACHE_DISABLE_MASK                     0x01
#define PDE_2MB_64_PAGE_LEVEL_CACHE_DISABLE(_)                       (((_) >> 4) & 0x01)

	/**
	* [Bit 5] Accessed; indicates whether software has accessed the 2-MByte page referenced by this entry.
	*
	* @see Vol3A[4.8(Accessed and Dirty Flags)]
	*/
	uint64_t accessed                                                : 1;
#define PDE_2MB_64_ACCESSED_BIT                                      5
#define PDE_2MB_64_ACCESSED_FLAG                                     0x20
#define PDE_2MB_64_ACCESSED_MASK                                     0x01
#define PDE_2MB_64_ACCESSED(_)                                       (((_) >> 5) & 0x01)

	/**
	* [Bit 6] Dirty; indicates whether software has written to the 2-MByte page referenced by this entry.
	*
	* @see Vol3A[4.8(Accessed and Dirty Flags)]
	*/
	uint64_t dirty                                                   : 1;
#define PDE_2MB_64_DIRTY_BIT                                         6
#define PDE_2MB_64_DIRTY_FLAG                                        0x40
#define PDE_2MB_64_DIRTY_MASK                                        0x01
#define PDE_2MB_64_DIRTY(_)                                          (((_) >> 6) & 0x01)

	/**
	* [Bit 7] Page size; must be 1 (otherwise, this entry references a page directory).
	*/
	uint64_t large_page                                              : 1;
#define PDE_2MB_64_LARGE_PAGE_BIT                                    7
#define PDE_2MB_64_LARGE_PAGE_FLAG                                   0x80
#define PDE_2MB_64_LARGE_PAGE_MASK                                   0x01
#define PDE_2MB_64_LARGE_PAGE(_)                                     (((_) >> 7) & 0x01)

	/**
	* [Bit 8] Global; if CR4.PGE = 1, determines whether the translation is global; ignored otherwise.
	*
	* @see Vol3A[4.10(Caching Translation Information)]
	*/
	uint64_t global                                                  : 1;
#define PDE_2MB_64_GLOBAL_BIT                                        8
#define PDE_2MB_64_GLOBAL_FLAG                                       0x100
#define PDE_2MB_64_GLOBAL_MASK                                       0x01
#define PDE_2MB_64_GLOBAL(_)                                         (((_) >> 8) & 0x01)

	/**
	* [Bits 11:9] Ignored.
	*/
	uint64_t ignored_1                                               : 3;
#define PDE_2MB_64_IGNORED_1_BIT                                     9
#define PDE_2MB_64_IGNORED_1_FLAG                                    0xE00
#define PDE_2MB_64_IGNORED_1_MASK                                    0x07
#define PDE_2MB_64_IGNORED_1(_)                                      (((_) >> 9) & 0x07)

	/**
	* [Bit 12] Indirectly determines the memory type used to access the 2-MByte page referenced by this entry.
	*
	* @note The PAT is supported on all processors that support 4-level paging.
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint64_t pat                                                     : 1;
#define PDE_2MB_64_PAT_BIT                                           12
#define PDE_2MB_64_PAT_FLAG                                          0x1000
#define PDE_2MB_64_PAT_MASK                                          0x01
#define PDE_2MB_64_PAT(_)                                            (((_) >> 12) & 0x01)
	uint64_t reserved1                                               : 8;

	/**
	* [Bits 47:21] Physical address of the 2-MByte page referenced by this entry.
	*/
	uint64_t page_frame_number                                       : 27;
#define PDE_2MB_64_PAGE_FRAME_NUMBER_BIT                             21
#define PDE_2MB_64_PAGE_FRAME_NUMBER_FLAG                            0xFFFFFFE00000
#define PDE_2MB_64_PAGE_FRAME_NUMBER_MASK                            0x7FFFFFF
#define PDE_2MB_64_PAGE_FRAME_NUMBER(_)                              (((_) >> 21) & 0x7FFFFFF)
	uint64_t reserved2                                               : 4;

	/**
	* [Bits 58:52] Ignored.
	*/
	uint64_t ignored_2                                               : 7;
#define PDE_2MB_64_IGNORED_2_BIT                                     52
#define PDE_2MB_64_IGNORED_2_FLAG                                    0x7F0000000000000
#define PDE_2MB_64_IGNORED_2_MASK                                    0x7F
#define PDE_2MB_64_IGNORED_2(_)                                      (((_) >> 52) & 0x7F)

	/**
	* [Bits 62:59] Protection key; if CR4.PKE = 1, determines the protection key of the page; ignored otherwise.
	*
	* @see Vol3A[4.6.2(Protection Keys)]
	*/
	uint64_t protection_key                                          : 4;
#define PDE_2MB_64_PROTECTION_KEY_BIT                                59
#define PDE_2MB_64_PROTECTION_KEY_FLAG                               0x7800000000000000
#define PDE_2MB_64_PROTECTION_KEY_MASK                               0x0F
#define PDE_2MB_64_PROTECTION_KEY(_)                                 (((_) >> 59) & 0x0F)

	/**
	* [Bit 63] If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 2-MByte page
	* controlled by this entry); otherwise, reserved (must be 0).
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint64_t execute_disable                                         : 1;
#define PDE_2MB_64_EXECUTE_DISABLE_BIT                               63
#define PDE_2MB_64_EXECUTE_DISABLE_FLAG                              0x8000000000000000
#define PDE_2MB_64_EXECUTE_DISABLE_MASK                              0x01
#define PDE_2MB_64_EXECUTE_DISABLE(_)                                (((_) >> 63) & 0x01)
  };

  uint64_t flags;
} pde_2mb_64;

/**
 * @brief Format of a 4-Level Page-Directory Entry that References a Page Table
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] Present; must be 1 to reference a page table.
	*/
	uint64_t present                                                 : 1;
#define PDE_64_PRESENT_BIT                                           0
#define PDE_64_PRESENT_FLAG                                          0x01
#define PDE_64_PRESENT_MASK                                          0x01
#define PDE_64_PRESENT(_)                                            (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Read/write; if 0, writes may not be allowed to the 2-MByte region controlled by this entry.
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint64_t write                                                   : 1;
#define PDE_64_WRITE_BIT                                             1
#define PDE_64_WRITE_FLAG                                            0x02
#define PDE_64_WRITE_MASK                                            0x01
#define PDE_64_WRITE(_)                                              (((_) >> 1) & 0x01)

	/**
	* [Bit 2] User/supervisor; if 0, user-mode accesses are not allowed to the 2-MByte region controlled by this entry.
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint64_t user                                                    : 1;
#define PDE_64_USER_BIT                                        2
#define PDE_64_USER_FLAG                                       0x04
#define PDE_64_USER_MASK                                       0x01
#define PDE_64_USER(_)                                         (((_) >> 2) & 0x01)

	/**
	* [Bit 3] Page-level write-through; indirectly determines the memory type used to access the page table referenced by this
	* entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint64_t page_level_write_through                                : 1;
#define PDE_64_PAGE_LEVEL_WRITE_THROUGH_BIT                          3
#define PDE_64_PAGE_LEVEL_WRITE_THROUGH_FLAG                         0x08
#define PDE_64_PAGE_LEVEL_WRITE_THROUGH_MASK                         0x01
#define PDE_64_PAGE_LEVEL_WRITE_THROUGH(_)                           (((_) >> 3) & 0x01)

	/**
	* [Bit 4] Page-level cache disable; indirectly determines the memory type used to access the page table referenced by this
	* entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint64_t page_level_cache_disable                                : 1;
#define PDE_64_PAGE_LEVEL_CACHE_DISABLE_BIT                          4
#define PDE_64_PAGE_LEVEL_CACHE_DISABLE_FLAG                         0x10
#define PDE_64_PAGE_LEVEL_CACHE_DISABLE_MASK                         0x01
#define PDE_64_PAGE_LEVEL_CACHE_DISABLE(_)                           (((_) >> 4) & 0x01)

	/**
	* [Bit 5] Accessed; indicates whether this entry has been used for linear-address translation.
	*
	* @see Vol3A[4.8(Accessed and Dirty Flags)]
	*/
	uint64_t accessed                                                : 1;
#define PDE_64_ACCESSED_BIT                                          5
#define PDE_64_ACCESSED_FLAG                                         0x20
#define PDE_64_ACCESSED_MASK                                         0x01
#define PDE_64_ACCESSED(_)                                           (((_) >> 5) & 0x01)
	uint64_t reserved1                                               : 1;

	/**
	* [Bit 7] Page size; must be 0 (otherwise, this entry maps a 2-MByte page).
	*/
	uint64_t large_page                                              : 1;
#define PDE_64_LARGE_PAGE_BIT                                        7
#define PDE_64_LARGE_PAGE_FLAG                                       0x80
#define PDE_64_LARGE_PAGE_MASK                                       0x01
#define PDE_64_LARGE_PAGE(_)                                         (((_) >> 7) & 0x01)

	/**
	* [Bits 11:8] Ignored.
	*/
	uint64_t ignored_1                                               : 4;
#define PDE_64_IGNORED_1_BIT                                         8
#define PDE_64_IGNORED_1_FLAG                                        0xF00
#define PDE_64_IGNORED_1_MASK                                        0x0F
#define PDE_64_IGNORED_1(_)                                          (((_) >> 8) & 0x0F)

	/**
	* [Bits 47:12] Physical address of 4-KByte aligned page table referenced by this entry.
	*/
	uint64_t page_frame_number                                       : 36;
#define PDE_64_PAGE_FRAME_NUMBER_BIT                                 12
#define PDE_64_PAGE_FRAME_NUMBER_FLAG                                0xFFFFFFFFF000
#define PDE_64_PAGE_FRAME_NUMBER_MASK                                0xFFFFFFFFF
#define PDE_64_PAGE_FRAME_NUMBER(_)                                  (((_) >> 12) & 0xFFFFFFFFF)
	uint64_t reserved2                                               : 4;

	/**
	* [Bits 62:52] Ignored.
	*/
	uint64_t ignored_2                                               : 11;
#define PDE_64_IGNORED_2_BIT                                         52
#define PDE_64_IGNORED_2_FLAG                                        0x7FF0000000000000
#define PDE_64_IGNORED_2_MASK                                        0x7FF
#define PDE_64_IGNORED_2(_)                                          (((_) >> 52) & 0x7FF)

	/**
	* [Bit 63] If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 2-MByte region
	* controlled by this entry); otherwise, reserved (must be 0).
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint64_t execute_disable                                         : 1;
#define PDE_64_EXECUTE_DISABLE_BIT                                   63
#define PDE_64_EXECUTE_DISABLE_FLAG                                  0x8000000000000000
#define PDE_64_EXECUTE_DISABLE_MASK                                  0x01
#define PDE_64_EXECUTE_DISABLE(_)                                    (((_) >> 63) & 0x01)
  };

  uint64_t flags;
} pde_64;

/**
 * @brief Format of a 4-Level Page-Table Entry that Maps a 4-KByte Page
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] Present; must be 1 to map a 4-KByte page.
	*/
	uint64_t present                                                 : 1;
#define PTE_64_PRESENT_BIT                                           0
#define PTE_64_PRESENT_FLAG                                          0x01
#define PTE_64_PRESENT_MASK                                          0x01
#define PTE_64_PRESENT(_)                                            (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Read/write; if 0, writes may not be allowed to the 4-KByte page referenced by this entry.
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint64_t write                                                   : 1;
#define PTE_64_WRITE_BIT                                             1
#define PTE_64_WRITE_FLAG                                            0x02
#define PTE_64_WRITE_MASK                                            0x01
#define PTE_64_WRITE(_)                                              (((_) >> 1) & 0x01)

	/**
	* [Bit 2] User/supervisor; if 0, user-mode accesses are not allowed to the 4-KByte page referenced by this entry.
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint64_t user                                                    : 1;
#define PTE_64_USER_BIT                                        2
#define PTE_64_USER_FLAG                                       0x04
#define PTE_64_USER_MASK                                       0x01
#define PTE_64_USER(_)                                         (((_) >> 2) & 0x01)

	/**
	* [Bit 3] Page-level write-through; indirectly determines the memory type used to access the 4-KByte page referenced by
	* this entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint64_t page_level_write_through                                : 1;
#define PTE_64_PAGE_LEVEL_WRITE_THROUGH_BIT                          3
#define PTE_64_PAGE_LEVEL_WRITE_THROUGH_FLAG                         0x08
#define PTE_64_PAGE_LEVEL_WRITE_THROUGH_MASK                         0x01
#define PTE_64_PAGE_LEVEL_WRITE_THROUGH(_)                           (((_) >> 3) & 0x01)

	/**
	* [Bit 4] Page-level cache disable; indirectly determines the memory type used to access the 4-KByte page referenced by
	* this entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint64_t page_level_cache_disable                                : 1;
#define PTE_64_PAGE_LEVEL_CACHE_DISABLE_BIT                          4
#define PTE_64_PAGE_LEVEL_CACHE_DISABLE_FLAG                         0x10
#define PTE_64_PAGE_LEVEL_CACHE_DISABLE_MASK                         0x01
#define PTE_64_PAGE_LEVEL_CACHE_DISABLE(_)                           (((_) >> 4) & 0x01)

	/**
	* [Bit 5] Accessed; indicates whether software has accessed the 4-KByte page referenced by this entry.
	*
	* @see Vol3A[4.8(Accessed and Dirty Flags)]
	*/
	uint64_t accessed                                                : 1;
#define PTE_64_ACCESSED_BIT                                          5
#define PTE_64_ACCESSED_FLAG                                         0x20
#define PTE_64_ACCESSED_MASK                                         0x01
#define PTE_64_ACCESSED(_)                                           (((_) >> 5) & 0x01)

	/**
	* [Bit 6] Dirty; indicates whether software has written to the 4-KByte page referenced by this entry.
	*
	* @see Vol3A[4.8(Accessed and Dirty Flags)]
	*/
	uint64_t dirty                                                   : 1;
#define PTE_64_DIRTY_BIT                                             6
#define PTE_64_DIRTY_FLAG                                            0x40
#define PTE_64_DIRTY_MASK                                            0x01
#define PTE_64_DIRTY(_)                                              (((_) >> 6) & 0x01)

	/**
	* [Bit 7] Indirectly determines the memory type used to access the 4-KByte page referenced by this entry.
	*
	* @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
	*/
	uint64_t pat                                                     : 1;
#define PTE_64_PAT_BIT                                               7
#define PTE_64_PAT_FLAG                                              0x80
#define PTE_64_PAT_MASK                                              0x01
#define PTE_64_PAT(_)                                                (((_) >> 7) & 0x01)

	/**
	* [Bit 8] Global; if CR4.PGE = 1, determines whether the translation is global; ignored otherwise.
	*
	* @see Vol3A[4.10(Caching Translation Information)]
	*/
	uint64_t global                                                  : 1;
#define PTE_64_GLOBAL_BIT                                            8
#define PTE_64_GLOBAL_FLAG                                           0x100
#define PTE_64_GLOBAL_MASK                                           0x01
#define PTE_64_GLOBAL(_)                                             (((_) >> 8) & 0x01)

	/**
	* [Bits 11:9] Ignored.
	*/
	uint64_t ignored_1                                               : 3;
#define PTE_64_IGNORED_1_BIT                                         9
#define PTE_64_IGNORED_1_FLAG                                        0xE00
#define PTE_64_IGNORED_1_MASK                                        0x07
#define PTE_64_IGNORED_1(_)                                          (((_) >> 9) & 0x07)

	/**
	* [Bits 47:12] Physical address of the 4-KByte page referenced by this entry.
	*/
	uint64_t page_frame_number                                       : 36;
#define PTE_64_PAGE_FRAME_NUMBER_BIT                                 12
#define PTE_64_PAGE_FRAME_NUMBER_FLAG                                0xFFFFFFFFF000
#define PTE_64_PAGE_FRAME_NUMBER_MASK                                0xFFFFFFFFF
#define PTE_64_PAGE_FRAME_NUMBER(_)                                  (((_) >> 12) & 0xFFFFFFFFF)
	uint64_t reserved1                                               : 4;

	/**
	* [Bits 58:52] Ignored.
	*/
	uint64_t ignored_2                                               : 7;
#define PTE_64_IGNORED_2_BIT                                         52
#define PTE_64_IGNORED_2_FLAG                                        0x7F0000000000000
#define PTE_64_IGNORED_2_MASK                                        0x7F
#define PTE_64_IGNORED_2(_)                                          (((_) >> 52) & 0x7F)

	/**
	* [Bits 62:59] Protection key; if CR4.PKE = 1, determines the protection key of the page; ignored otherwise.
	*
	* @see Vol3A[4.6.2(Protection Keys)]
	*/
	uint64_t protection_key                                          : 4;
#define PTE_64_PROTECTION_KEY_BIT                                    59
#define PTE_64_PROTECTION_KEY_FLAG                                   0x7800000000000000
#define PTE_64_PROTECTION_KEY_MASK                                   0x0F
#define PTE_64_PROTECTION_KEY(_)                                     (((_) >> 59) & 0x0F)

	/**
	* [Bit 63] If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 1-GByte page
	* controlled by this entry); otherwise, reserved (must be 0).
	*
	* @see Vol3A[4.6(Access Rights)]
	*/
	uint64_t execute_disable                                         : 1;
#define PTE_64_EXECUTE_DISABLE_BIT                                   63
#define PTE_64_EXECUTE_DISABLE_FLAG                                  0x8000000000000000
#define PTE_64_EXECUTE_DISABLE_MASK                                  0x01
#define PTE_64_EXECUTE_DISABLE(_)                                    (((_) >> 63) & 0x01)
  };

  uint64_t flags;
} pte_64;

/**
 * @brief Format of a common Page-Table Entry
 */
typedef union
{
  struct
  {
	uint64_t present                                                 : 1;
#define PT_ENTRY_64_PRESENT_BIT                                      0
#define PT_ENTRY_64_PRESENT_FLAG                                     0x01
#define PT_ENTRY_64_PRESENT_MASK                                     0x01
#define PT_ENTRY_64_PRESENT(_)                                       (((_) >> 0) & 0x01)
	uint64_t write                                                   : 1;
#define PT_ENTRY_64_WRITE_BIT                                        1
#define PT_ENTRY_64_WRITE_FLAG                                       0x02
#define PT_ENTRY_64_WRITE_MASK                                       0x01
#define PT_ENTRY_64_WRITE(_)                                         (((_) >> 1) & 0x01)
	uint64_t user                                                    : 1;
#define PT_ENTRY_64_USER_BIT                                         2
#define PT_ENTRY_64_USER_FLAG                                        0x04
#define PT_ENTRY_64_USER_MASK                                        0x01
#define PT_ENTRY_64_USER(_)                                          (((_) >> 2) & 0x01)
	uint64_t page_level_write_through                                : 1;
#define PT_ENTRY_64_PAGE_LEVEL_WRITE_THROUGH_BIT                     3
#define PT_ENTRY_64_PAGE_LEVEL_WRITE_THROUGH_FLAG                    0x08
#define PT_ENTRY_64_PAGE_LEVEL_WRITE_THROUGH_MASK                    0x01
#define PT_ENTRY_64_PAGE_LEVEL_WRITE_THROUGH(_)                      (((_) >> 3) & 0x01)
	uint64_t page_level_cache_disable                                : 1;
#define PT_ENTRY_64_PAGE_LEVEL_CACHE_DISABLE_BIT                     4
#define PT_ENTRY_64_PAGE_LEVEL_CACHE_DISABLE_FLAG                    0x10
#define PT_ENTRY_64_PAGE_LEVEL_CACHE_DISABLE_MASK                    0x01
#define PT_ENTRY_64_PAGE_LEVEL_CACHE_DISABLE(_)                      (((_) >> 4) & 0x01)
	uint64_t accessed                                                : 1;
#define PT_ENTRY_64_ACCESSED_BIT                                     5
#define PT_ENTRY_64_ACCESSED_FLAG                                    0x20
#define PT_ENTRY_64_ACCESSED_MASK                                    0x01
#define PT_ENTRY_64_ACCESSED(_)                                      (((_) >> 5) & 0x01)
	uint64_t dirty                                                   : 1;
#define PT_ENTRY_64_DIRTY_BIT                                        6
#define PT_ENTRY_64_DIRTY_FLAG                                       0x40
#define PT_ENTRY_64_DIRTY_MASK                                       0x01
#define PT_ENTRY_64_DIRTY(_)                                         (((_) >> 6) & 0x01)
	uint64_t large_page                                              : 1;
#define PT_ENTRY_64_LARGE_PAGE_BIT                                   7
#define PT_ENTRY_64_LARGE_PAGE_FLAG                                  0x80
#define PT_ENTRY_64_LARGE_PAGE_MASK                                  0x01
#define PT_ENTRY_64_LARGE_PAGE(_)                                    (((_) >> 7) & 0x01)
	uint64_t global                                                  : 1;
#define PT_ENTRY_64_GLOBAL_BIT                                       8
#define PT_ENTRY_64_GLOBAL_FLAG                                      0x100
#define PT_ENTRY_64_GLOBAL_MASK                                      0x01
#define PT_ENTRY_64_GLOBAL(_)                                        (((_) >> 8) & 0x01)

	/**
	* [Bits 11:9] Ignored.
	*/
	uint64_t ignored_1                                               : 3;
#define PT_ENTRY_64_IGNORED_1_BIT                                    9
#define PT_ENTRY_64_IGNORED_1_FLAG                                   0xE00
#define PT_ENTRY_64_IGNORED_1_MASK                                   0x07
#define PT_ENTRY_64_IGNORED_1(_)                                     (((_) >> 9) & 0x07)

	/**
	* [Bits 47:12] Physical address of the 4-KByte page referenced by this entry.
	*/
	uint64_t page_frame_number                                       : 36;
#define PT_ENTRY_64_PAGE_FRAME_NUMBER_BIT                            12
#define PT_ENTRY_64_PAGE_FRAME_NUMBER_FLAG                           0xFFFFFFFFF000
#define PT_ENTRY_64_PAGE_FRAME_NUMBER_MASK                           0xFFFFFFFFF
#define PT_ENTRY_64_PAGE_FRAME_NUMBER(_)                             (((_) >> 12) & 0xFFFFFFFFF)
	uint64_t reserved1                                               : 4;

	/**
	* [Bits 58:52] Ignored.
	*/
	uint64_t ignored_2                                               : 7;
#define PT_ENTRY_64_IGNORED_2_BIT                                    52
#define PT_ENTRY_64_IGNORED_2_FLAG                                   0x7F0000000000000
#define PT_ENTRY_64_IGNORED_2_MASK                                   0x7F
#define PT_ENTRY_64_IGNORED_2(_)                                     (((_) >> 52) & 0x7F)
	uint64_t protection_key                                          : 4;
#define PT_ENTRY_64_PROTECTION_KEY_BIT                               59
#define PT_ENTRY_64_PROTECTION_KEY_FLAG                              0x7800000000000000
#define PT_ENTRY_64_PROTECTION_KEY_MASK                              0x0F
#define PT_ENTRY_64_PROTECTION_KEY(_)                                (((_) >> 59) & 0x0F)
	uint64_t execute_disable                                         : 1;
#define PT_ENTRY_64_EXECUTE_DISABLE_BIT                              63
#define PT_ENTRY_64_EXECUTE_DISABLE_FLAG                             0x8000000000000000
#define PT_ENTRY_64_EXECUTE_DISABLE_MASK                             0x01
#define PT_ENTRY_64_EXECUTE_DISABLE(_)                               (((_) >> 63) & 0x01)
  };

  uint64_t flags;
} pt_entry_64;

/**
 * @defgroup paging_structures_entry_count_64 \
 *           Paging structures entry counts
 *
 * Paging structures entry counts.
 * @{
 */
#define PML4E_ENTRY_COUNT_64                                         0x00000200
#define PDPTE_ENTRY_COUNT_64                                         0x00000200
#define PDE_ENTRY_COUNT_64                                           0x00000200
#define PTE_ENTRY_COUNT_64                                           0x00000200
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @defgroup segment_descriptors \
 *           Segment descriptors
 * @{
 */
/**
 * @brief Pseudo-Descriptor Format (32-bit)
 *
 * @see Vol3A[3.5.1(Segment Descriptor Tables)] (reference)
 */
#pragma pack(push, 1)
typedef struct
{
  /**
	* Limit.
	*/
  uint16_t limit;

  /**
	* Base Address.
	*/
  uint32_t base_address;
} segment_descriptor_register_32;
#pragma pack(pop)

/**
 * @brief Pseudo-Descriptor Format (64-bit)
 *
 * @see Vol3A[3.5.1(Segment Descriptor Tables)] (reference)
 */
#pragma pack(push, 1)
typedef struct
{
  /**
	* Limit.
	*/
  uint16_t limit;

  /**
	* Base Address.
	*/
  uint64_t base_address;
} segment_descriptor_register_64;
#pragma pack(pop)

/**
 * @brief Segment access rights
 *
 * @see Vol2A[3.2(Instructions (A-L) | LAR-Load Access Rights Byte)] (reference)
 */
typedef union
{
  struct
  {
	uint32_t reserved1                                               : 8;

	/**
	* @brief Type field
	*
	* [Bits 11:8] Indicates the segment or gate type and specifies the kinds of access that can be made to the segment and the
	* direction of growth. The interpretation of this field depends on whether the descriptor type flag specifies an
	* application (code or data) descriptor or a system descriptor. The encoding of the type field is different for code,
	* data, and system descriptors.
	*
	* @see Vol3A[3.4.5.1(Code- and Data-Segment Descriptor Types)]
	*/
	uint32_t type                                                    : 4;
#define SEGMENT_ACCESS_RIGHTS_TYPE_BIT                               8
#define SEGMENT_ACCESS_RIGHTS_TYPE_FLAG                              0xF00
#define SEGMENT_ACCESS_RIGHTS_TYPE_MASK                              0x0F
#define SEGMENT_ACCESS_RIGHTS_TYPE(_)                                (((_) >> 8) & 0x0F)

	/**
	* @brief S (descriptor type) flag
	*
	* [Bit 12] Specifies whether the segment descriptor is for a system segment (S flag is clear) or a code or data segment (S
	* flag is set).
	*/
	uint32_t descriptor_type                                         : 1;
#define SEGMENT_ACCESS_RIGHTS_DESCRIPTOR_TYPE_BIT                    12
#define SEGMENT_ACCESS_RIGHTS_DESCRIPTOR_TYPE_FLAG                   0x1000
#define SEGMENT_ACCESS_RIGHTS_DESCRIPTOR_TYPE_MASK                   0x01
#define SEGMENT_ACCESS_RIGHTS_DESCRIPTOR_TYPE(_)                     (((_) >> 12) & 0x01)

	/**
	* @brief DPL (descriptor privilege level) field
	*
	* [Bits 14:13] Specifies the privilege level of the segment. The privilege level can range from 0 to 3, with 0 being the
	* most privileged level. The DPL is used to control access to the segment. See Section 5.5, "Privilege Levels", for a
	* description of the relationship of the DPL to the CPL of the executing code segment and the RPL of a segment selector.
	*/
	uint32_t descriptor_privilege_level                              : 2;
#define SEGMENT_ACCESS_RIGHTS_DESCRIPTOR_PRIVILEGE_LEVEL_BIT         13
#define SEGMENT_ACCESS_RIGHTS_DESCRIPTOR_PRIVILEGE_LEVEL_FLAG        0x6000
#define SEGMENT_ACCESS_RIGHTS_DESCRIPTOR_PRIVILEGE_LEVEL_MASK        0x03
#define SEGMENT_ACCESS_RIGHTS_DESCRIPTOR_PRIVILEGE_LEVEL(_)          (((_) >> 13) & 0x03)

	/**
	* @brief P (segment-present) flag
	*
	* [Bit 15] Indicates whether the segment is present in memory (set) or not present (clear). If this flag is clear, the
	* processor generates a segment-not-present exception (\#NP) when a segment selector that points to the segment descriptor
	* is loaded into a segment register. Memory management software can use this flag to control which segments are actually
	* loaded into physical memory at a given time. It offers a control in addition to paging for managing virtual memory.
	*/
	uint32_t present                                                 : 1;
#define SEGMENT_ACCESS_RIGHTS_PRESENT_BIT                            15
#define SEGMENT_ACCESS_RIGHTS_PRESENT_FLAG                           0x8000
#define SEGMENT_ACCESS_RIGHTS_PRESENT_MASK                           0x01
#define SEGMENT_ACCESS_RIGHTS_PRESENT(_)                             (((_) >> 15) & 0x01)
	uint32_t reserved2                                               : 4;

	/**
	* @brief Available bit
	*
	* [Bit 20] Bit 20 of the second doubleword of the segment descriptor is available for use by system software.
	*/
	uint32_t system                                                  : 1;
#define SEGMENT_ACCESS_RIGHTS_SYSTEM_BIT                             20
#define SEGMENT_ACCESS_RIGHTS_SYSTEM_FLAG                            0x100000
#define SEGMENT_ACCESS_RIGHTS_SYSTEM_MASK                            0x01
#define SEGMENT_ACCESS_RIGHTS_SYSTEM(_)                              (((_) >> 20) & 0x01)

	/**
	* @brief L (64-bit code segment) flag
	*
	* [Bit 21] In IA-32e mode, bit 21 of the second doubleword of the segment descriptor indicates whether a code segment
	* contains native 64-bit code. A value of 1 indicates instructions in this code segment are executed in 64-bit mode. A
	* value of 0 indicates the instructions in this code segment are executed in compatibility mode. If L-bit is set, then
	* D-bit must be cleared. When not in IA-32e mode or for non-code segments, bit 21 is reserved and should always be set to
	* 0.
	*/
	uint32_t long_mode                                               : 1;
#define SEGMENT_ACCESS_RIGHTS_LONG_MODE_BIT                          21
#define SEGMENT_ACCESS_RIGHTS_LONG_MODE_FLAG                         0x200000
#define SEGMENT_ACCESS_RIGHTS_LONG_MODE_MASK                         0x01
#define SEGMENT_ACCESS_RIGHTS_LONG_MODE(_)                           (((_) >> 21) & 0x01)

	/**
	* @brief D/B (default operation size/default stack pointer size and/or upper bound) flag
	*
	* [Bit 22] Performs different functions depending on whether the segment descriptor is an executable code segment, an
	* expand-down data segment, or a stack segment. (This flag should always be set to 1 for 32-bit code and data segments and
	* to 0 for 16-bit code and data segments.)
	* - Executable code segment. The flag is called the D flag and it indicates the default length for effective addresses and
	* operands referenced by instructions in the segment. If the flag is set, 32-bit addresses and 32-bit or 8-bit operands
	* are assumed; if it is clear, 16-bit addresses and 16-bit or 8-bit operands are assumed. The instruction prefix 66H can
	* be used to select an operand size other than the default, and the prefix 67H can be used select an address size other
	* than the default.
	* - Stack segment (data segment pointed to by the SS register). The flag is called the B (big) flag and it specifies the
	* size of the stack pointer used for implicit stack operations (such as pushes, pops, and calls). If the flag is set, a
	* 32-bit stack pointer is used, which is stored in the 32-bit ESP register; if the flag is clear, a 16-bit stack pointer
	* is used, which is stored in the 16- bit SP register. If the stack segment is set up to be an expand-down data segment
	* (described in the next paragraph), the B flag also specifies the upper bound of the stack segment.
	* - Expand-down data segment. The flag is called the B flag and it specifies the upper bound of the segment. If the flag
	* is set, the upper bound is FFFFFFFFH (4 GBytes); if the flag is clear, the upper bound is FFFFH (64 KBytes).
	*/
	uint32_t default_big                                             : 1;
#define SEGMENT_ACCESS_RIGHTS_DEFAULT_BIG_BIT                        22
#define SEGMENT_ACCESS_RIGHTS_DEFAULT_BIG_FLAG                       0x400000
#define SEGMENT_ACCESS_RIGHTS_DEFAULT_BIG_MASK                       0x01
#define SEGMENT_ACCESS_RIGHTS_DEFAULT_BIG(_)                         (((_) >> 22) & 0x01)

	/**
	* @brief G (granularity) flag
	*
	* [Bit 23] Determines the scaling of the segment limit field. When the granularity flag is clear, the segment limit is
	* interpreted in byte units; when flag is set, the segment limit is interpreted in 4-KByte units. (This flag does not
	* affect the granularity of the base address; it is always byte granular.) When the granularity flag is set, the twelve
	* least significant bits of an offset are not tested when checking the offset against the segment limit. For example, when
	* the granularity flag is set, a limit of 0 results in valid offsets from 0 to 4095.
	*/
	uint32_t granularity                                             : 1;
#define SEGMENT_ACCESS_RIGHTS_GRANULARITY_BIT                        23
#define SEGMENT_ACCESS_RIGHTS_GRANULARITY_FLAG                       0x800000
#define SEGMENT_ACCESS_RIGHTS_GRANULARITY_MASK                       0x01
#define SEGMENT_ACCESS_RIGHTS_GRANULARITY(_)                         (((_) >> 23) & 0x01)
	uint32_t reserved3                                               : 8;
  };

  uint32_t flags;
} segment_access_rights;

/**
 * @brief General Segment Descriptor (32-bit)
 *
 * A segment descriptor is a data structure in a GDT or LDT that provides the processor with the size and location of a
 * segment, as well as access control and status information. Segment descriptors are typically created by compilers,
 * linkers, loaders, or the operating system or executive, but not application programs.
 *
 * @see Vol3A[5.2(FIELDS AND FLAGS USED FOR SEGMENT-LEVEL AND PAGE-LEVEL PROTECTION)]
 * @see Vol3A[5.2.1(Code-Segment Descriptor in 64-bit Mode)]
 * @see Vol3A[5.8.3(Call Gates)]
 * @see Vol3A[6.11(IDT DESCRIPTORS)]
 * @see Vol3A[6.14.1(64-Bit Mode IDT)]
 * @see Vol3A[7.2.2(TSS Descriptor)]
 * @see Vol3A[7.2.3(TSS Descriptor in 64-bit mode)]
 * @see Vol3A[7.2.5(Task-Gate Descriptor)]
 * @see Vol3A[3.4.5(Segment Descriptors)] (reference)
 */
typedef struct
{
  /**
	* @brief Segment limit field (15:00)
	*
	* Specifies the size of the segment. The processor puts together the two segment limit fields to form a 20-bit value. The
	* processor interprets the segment limit in one of two ways, depending on the setting of the G (granularity) flag:
	* - If the granularity flag is clear, the segment size can range from 1 byte to 1 MByte, in byte increments.
	* - If the granularity flag is set, the segment size can range from 4 KBytes to 4 GBytes, in 4-KByte increments.
	* The processor uses the segment limit in two different ways, depending on whether the segment is an expand-up or an
	* expand-down segment. For expand-up segments, the offset in a logical address can range from 0 to the segment limit.
	* Offsets greater than the segment limit generate general-protection exceptions (\#GP, for all segments other than SS) or
	* stack-fault exceptions (\#SS for the SS segment). For expand-down segments, the segment limit has the reverse function;
	* the offset can range from the segment limit plus 1 to FFFFFFFFH or FFFFH, depending on the setting of the B flag.
	* Offsets less than or equal to the segment limit generate general-protection exceptions or stack-fault exceptions.
	* Decreasing the value in the segment limit field for an expanddown segment allocates new memory at the bottom of the
	* segment's address space, rather than at the top. IA-32 architecture stacks always grow downwards, making this mechanism
	* convenient for expandable stacks.
	*
	* @see Vol3A[3.4.5.1(Code- and Data-Segment Descriptor Types)]
	*/
  uint16_t segment_limit_low;

  /**
	* @brief Base address field (15:00)
	*
	* Defines the location of byte 0 of the segment within the 4-GByte linear address space. The processor puts together the
	* three base address fields to form a single 32-bit value. Segment base addresses should be aligned to 16-byte boundaries.
	* Although 16-byte alignment is not required, this alignment allows programs to maximize performance by aligning code and
	* data on 16-byte boundaries.
	*/
  uint16_t base_address_low;
  /**
	* @brief Segment descriptor fields
	*/
  union
  {
	struct
	{
		/**
		* [Bits 7:0] Base address field (23:16); see description of $BASE_LOW for more details.
		*/
		uint32_t base_address_middle                                   : 8;
#define SEGMENT__BASE_ADDRESS_MIDDLE_BIT                             0
#define SEGMENT__BASE_ADDRESS_MIDDLE_FLAG                            0xFF
#define SEGMENT__BASE_ADDRESS_MIDDLE_MASK                            0xFF
#define SEGMENT__BASE_ADDRESS_MIDDLE(_)                              (((_) >> 0) & 0xFF)

		/**
		* @brief Type field
		*
		* [Bits 11:8] Indicates the segment or gate type and specifies the kinds of access that can be made to the segment and the
		* direction of growth. The interpretation of this field depends on whether the descriptor type flag specifies an
		* application (code or data) descriptor or a system descriptor. The encoding of the type field is different for code,
		* data, and system descriptors.
		*
		* @see Vol3A[3.4.5.1(Code- and Data-Segment Descriptor Types)]
		*/
		uint32_t type                                                  : 4;
#define SEGMENT__TYPE_BIT                                            8
#define SEGMENT__TYPE_FLAG                                           0xF00
#define SEGMENT__TYPE_MASK                                           0x0F
#define SEGMENT__TYPE(_)                                             (((_) >> 8) & 0x0F)

		/**
		* @brief S (descriptor type) flag
		*
		* [Bit 12] Specifies whether the segment descriptor is for a system segment (S flag is clear) or a code or data segment (S
		* flag is set).
		*/
		uint32_t descriptor_type                                       : 1;
#define SEGMENT__DESCRIPTOR_TYPE_BIT                                 12
#define SEGMENT__DESCRIPTOR_TYPE_FLAG                                0x1000
#define SEGMENT__DESCRIPTOR_TYPE_MASK                                0x01
#define SEGMENT__DESCRIPTOR_TYPE(_)                                  (((_) >> 12) & 0x01)

		/**
		* @brief DPL (descriptor privilege level) field
		*
		* [Bits 14:13] Specifies the privilege level of the segment. The privilege level can range from 0 to 3, with 0 being the
		* most privileged level. The DPL is used to control access to the segment. See Section 5.5, "Privilege Levels", for a
		* description of the relationship of the DPL to the CPL of the executing code segment and the RPL of a segment selector.
		*/
		uint32_t descriptor_privilege_level                            : 2;
#define SEGMENT__DESCRIPTOR_PRIVILEGE_LEVEL_BIT                      13
#define SEGMENT__DESCRIPTOR_PRIVILEGE_LEVEL_FLAG                     0x6000
#define SEGMENT__DESCRIPTOR_PRIVILEGE_LEVEL_MASK                     0x03
#define SEGMENT__DESCRIPTOR_PRIVILEGE_LEVEL(_)                       (((_) >> 13) & 0x03)

		/**
		* @brief P (segment-present) flag
		*
		* [Bit 15] Indicates whether the segment is present in memory (set) or not present (clear). If this flag is clear, the
		* processor generates a segment-not-present exception (\#NP) when a segment selector that points to the segment descriptor
		* is loaded into a segment register. Memory management software can use this flag to control which segments are actually
		* loaded into physical memory at a given time. It offers a control in addition to paging for managing virtual memory.
		*/
		uint32_t present                                               : 1;
#define SEGMENT__PRESENT_BIT                                         15
#define SEGMENT__PRESENT_FLAG                                        0x8000
#define SEGMENT__PRESENT_MASK                                        0x01
#define SEGMENT__PRESENT(_)                                          (((_) >> 15) & 0x01)

		/**
		* [Bits 19:16] Segment limit field (19:16); see description of $LIMIT_LOW for more details.
		*/
		uint32_t segment_limit_high                                    : 4;
#define SEGMENT__SEGMENT_LIMIT_HIGH_BIT                              16
#define SEGMENT__SEGMENT_LIMIT_HIGH_FLAG                             0xF0000
#define SEGMENT__SEGMENT_LIMIT_HIGH_MASK                             0x0F
#define SEGMENT__SEGMENT_LIMIT_HIGH(_)                               (((_) >> 16) & 0x0F)

		/**
		* @brief Available bit
		*
		* [Bit 20] Bit 20 of the second doubleword of the segment descriptor is available for use by system software.
		*/
		uint32_t system                                                : 1;
#define SEGMENT__SYSTEM_BIT                                          20
#define SEGMENT__SYSTEM_FLAG                                         0x100000
#define SEGMENT__SYSTEM_MASK                                         0x01
#define SEGMENT__SYSTEM(_)                                           (((_) >> 20) & 0x01)

		/**
		* @brief L (64-bit code segment) flag
		*
		* [Bit 21] In IA-32e mode, bit 21 of the second doubleword of the segment descriptor indicates whether a code segment
		* contains native 64-bit code. A value of 1 indicates instructions in this code segment are executed in 64-bit mode. A
		* value of 0 indicates the instructions in this code segment are executed in compatibility mode. If L-bit is set, then
		* D-bit must be cleared. When not in IA-32e mode or for non-code segments, bit 21 is reserved and should always be set to
		* 0.
		*/
		uint32_t long_mode                                             : 1;
#define SEGMENT__LONG_MODE_BIT                                       21
#define SEGMENT__LONG_MODE_FLAG                                      0x200000
#define SEGMENT__LONG_MODE_MASK                                      0x01
#define SEGMENT__LONG_MODE(_)                                        (((_) >> 21) & 0x01)

		/**
		* @brief D/B (default operation size/default stack pointer size and/or upper bound) flag
		*
		* [Bit 22] Performs different functions depending on whether the segment descriptor is an executable code segment, an
		* expand-down data segment, or a stack segment. (This flag should always be set to 1 for 32-bit code and data segments and
		* to 0 for 16-bit code and data segments.)
		* - Executable code segment. The flag is called the D flag and it indicates the default length for effective addresses and
		* operands referenced by instructions in the segment. If the flag is set, 32-bit addresses and 32-bit or 8-bit operands
		* are assumed; if it is clear, 16-bit addresses and 16-bit or 8-bit operands are assumed. The instruction prefix 66H can
		* be used to select an operand size other than the default, and the prefix 67H can be used select an address size other
		* than the default.
		* - Stack segment (data segment pointed to by the SS register). The flag is called the B (big) flag and it specifies the
		* size of the stack pointer used for implicit stack operations (such as pushes, pops, and calls). If the flag is set, a
		* 32-bit stack pointer is used, which is stored in the 32-bit ESP register; if the flag is clear, a 16-bit stack pointer
		* is used, which is stored in the 16- bit SP register. If the stack segment is set up to be an expand-down data segment
		* (described in the next paragraph), the B flag also specifies the upper bound of the stack segment.
		* - Expand-down data segment. The flag is called the B flag and it specifies the upper bound of the segment. If the flag
		* is set, the upper bound is FFFFFFFFH (4 GBytes); if the flag is clear, the upper bound is FFFFH (64 KBytes).
		*/
		uint32_t default_big                                           : 1;
#define SEGMENT__DEFAULT_BIG_BIT                                     22
#define SEGMENT__DEFAULT_BIG_FLAG                                    0x400000
#define SEGMENT__DEFAULT_BIG_MASK                                    0x01
#define SEGMENT__DEFAULT_BIG(_)                                      (((_) >> 22) & 0x01)

		/**
		* @brief G (granularity) flag
		*
		* [Bit 23] Determines the scaling of the segment limit field. When the granularity flag is clear, the segment limit is
		* interpreted in byte units; when flag is set, the segment limit is interpreted in 4-KByte units. (This flag does not
		* affect the granularity of the base address; it is always byte granular.) When the granularity flag is set, the twelve
		* least significant bits of an offset are not tested when checking the offset against the segment limit. For example, when
		* the granularity flag is set, a limit of 0 results in valid offsets from 0 to 4095.
		*/
		uint32_t granularity                                           : 1;
#define SEGMENT__GRANULARITY_BIT                                     23
#define SEGMENT__GRANULARITY_FLAG                                    0x800000
#define SEGMENT__GRANULARITY_MASK                                    0x01
#define SEGMENT__GRANULARITY(_)                                      (((_) >> 23) & 0x01)

		/**
		* [Bits 31:24] Base address field (31:24); see description of $BASE_LOW for more details.
		*/
		uint32_t base_address_high                                     : 8;
#define SEGMENT__BASE_ADDRESS_HIGH_BIT                               24
#define SEGMENT__BASE_ADDRESS_HIGH_FLAG                              0xFF000000
#define SEGMENT__BASE_ADDRESS_HIGH_MASK                              0xFF
#define SEGMENT__BASE_ADDRESS_HIGH(_)                                (((_) >> 24) & 0xFF)
	};

	uint32_t flags;
  } ;

} segment_descriptor_32;

/**
 * @brief General Segment Descriptor (64-bit)
 *
 * A segment descriptor is a data structure in a GDT or LDT that provides the processor with the size and location of a
 * segment, as well as access control and status information. Segment descriptors are typically created by compilers,
 * linkers, loaders, or the operating system or executive, but not application programs.
 *
 * @see Vol3A[3.4.5(Segment Descriptors)] (reference)
 */
typedef struct
{
  /**
	* @brief Segment limit field (15:00)
	*
	* Specifies the size of the segment. The processor puts together the two segment limit fields to form a 20-bit value. The
	* processor interprets the segment limit in one of two ways, depending on the setting of the G (granularity) flag:
	* - If the granularity flag is clear, the segment size can range from 1 byte to 1 MByte, in byte increments.
	* - If the granularity flag is set, the segment size can range from 4 KBytes to 4 GBytes, in 4-KByte increments.
	* The processor uses the segment limit in two different ways, depending on whether the segment is an expand-up or an
	* expand-down segment. For expand-up segments, the offset in a logical address can range from 0 to the segment limit.
	* Offsets greater than the segment limit generate general-protection exceptions (\#GP, for all segments other than SS) or
	* stack-fault exceptions (\#SS for the SS segment). For expand-down segments, the segment limit has the reverse function;
	* the offset can range from the segment limit plus 1 to FFFFFFFFH or FFFFH, depending on the setting of the B flag.
	* Offsets less than or equal to the segment limit generate general-protection exceptions or stack-fault exceptions.
	* Decreasing the value in the segment limit field for an expanddown segment allocates new memory at the bottom of the
	* segment's address space, rather than at the top. IA-32 architecture stacks always grow downwards, making this mechanism
	* convenient for expandable stacks.
	*
	* @see Vol3A[3.4.5.1(Code- and Data-Segment Descriptor Types)]
	*/
  uint16_t segment_limit_low;

  /**
	* @brief Base address field (15:00)
	*
	* Defines the location of byte 0 of the segment within the 4-GByte linear address space. The processor puts together the
	* three base address fields to form a single 32-bit value. Segment base addresses should be aligned to 16-byte boundaries.
	* Although 16-byte alignment is not required, this alignment allows programs to maximize performance by aligning code and
	* data on 16-byte boundaries.
	*/
  uint16_t base_address_low;
  /**
	* @brief Segment descriptor fields
	*/
  union
  {
	struct
	{
		/**
		* [Bits 7:0] Base address field (23:16); see description of $BASE_LOW for more details.
		*/
		uint32_t base_address_middle                                   : 8;
#define SEGMENT__BASE_ADDRESS_MIDDLE_BIT                             0
#define SEGMENT__BASE_ADDRESS_MIDDLE_FLAG                            0xFF
#define SEGMENT__BASE_ADDRESS_MIDDLE_MASK                            0xFF
#define SEGMENT__BASE_ADDRESS_MIDDLE(_)                              (((_) >> 0) & 0xFF)

		/**
		* @brief Type field
		*
		* [Bits 11:8] Indicates the segment or gate type and specifies the kinds of access that can be made to the segment and the
		* direction of growth. The interpretation of this field depends on whether the descriptor type flag specifies an
		* application (code or data) descriptor or a system descriptor. The encoding of the type field is different for code,
		* data, and system descriptors.
		*
		* @see Vol3A[3.4.5.1(Code- and Data-Segment Descriptor Types)]
		*/
		uint32_t type                                                  : 4;
#define SEGMENT__TYPE_BIT                                            8
#define SEGMENT__TYPE_FLAG                                           0xF00
#define SEGMENT__TYPE_MASK                                           0x0F
#define SEGMENT__TYPE(_)                                             (((_) >> 8) & 0x0F)

		/**
		* @brief S (descriptor type) flag
		*
		* [Bit 12] Specifies whether the segment descriptor is for a system segment (S flag is clear) or a code or data segment (S
		* flag is set).
		*/
		uint32_t descriptor_type                                       : 1;
#define SEGMENT__DESCRIPTOR_TYPE_BIT                                 12
#define SEGMENT__DESCRIPTOR_TYPE_FLAG                                0x1000
#define SEGMENT__DESCRIPTOR_TYPE_MASK                                0x01
#define SEGMENT__DESCRIPTOR_TYPE(_)                                  (((_) >> 12) & 0x01)

		/**
		* @brief DPL (descriptor privilege level) field
		*
		* [Bits 14:13] Specifies the privilege level of the segment. The privilege level can range from 0 to 3, with 0 being the
		* most privileged level. The DPL is used to control access to the segment. See Section 5.5, "Privilege Levels", for a
		* description of the relationship of the DPL to the CPL of the executing code segment and the RPL of a segment selector.
		*/
		uint32_t descriptor_privilege_level                            : 2;
#define SEGMENT__DESCRIPTOR_PRIVILEGE_LEVEL_BIT                      13
#define SEGMENT__DESCRIPTOR_PRIVILEGE_LEVEL_FLAG                     0x6000
#define SEGMENT__DESCRIPTOR_PRIVILEGE_LEVEL_MASK                     0x03
#define SEGMENT__DESCRIPTOR_PRIVILEGE_LEVEL(_)                       (((_) >> 13) & 0x03)

		/**
		* @brief P (segment-present) flag
		*
		* [Bit 15] Indicates whether the segment is present in memory (set) or not present (clear). If this flag is clear, the
		* processor generates a segment-not-present exception (\#NP) when a segment selector that points to the segment descriptor
		* is loaded into a segment register. Memory management software can use this flag to control which segments are actually
		* loaded into physical memory at a given time. It offers a control in addition to paging for managing virtual memory.
		*/
		uint32_t present                                               : 1;
#define SEGMENT__PRESENT_BIT                                         15
#define SEGMENT__PRESENT_FLAG                                        0x8000
#define SEGMENT__PRESENT_MASK                                        0x01
#define SEGMENT__PRESENT(_)                                          (((_) >> 15) & 0x01)

		/**
		* [Bits 19:16] Segment limit field (19:16); see description of $LIMIT_LOW for more details.
		*/
		uint32_t segment_limit_high                                    : 4;
#define SEGMENT__SEGMENT_LIMIT_HIGH_BIT                              16
#define SEGMENT__SEGMENT_LIMIT_HIGH_FLAG                             0xF0000
#define SEGMENT__SEGMENT_LIMIT_HIGH_MASK                             0x0F
#define SEGMENT__SEGMENT_LIMIT_HIGH(_)                               (((_) >> 16) & 0x0F)

		/**
		* @brief Available bit
		*
		* [Bit 20] Bit 20 of the second doubleword of the segment descriptor is available for use by system software.
		*/
		uint32_t system                                                : 1;
#define SEGMENT__SYSTEM_BIT                                          20
#define SEGMENT__SYSTEM_FLAG                                         0x100000
#define SEGMENT__SYSTEM_MASK                                         0x01
#define SEGMENT__SYSTEM(_)                                           (((_) >> 20) & 0x01)

		/**
		* @brief L (64-bit code segment) flag
		*
		* [Bit 21] In IA-32e mode, bit 21 of the second doubleword of the segment descriptor indicates whether a code segment
		* contains native 64-bit code. A value of 1 indicates instructions in this code segment are executed in 64-bit mode. A
		* value of 0 indicates the instructions in this code segment are executed in compatibility mode. If L-bit is set, then
		* D-bit must be cleared. When not in IA-32e mode or for non-code segments, bit 21 is reserved and should always be set to
		* 0.
		*/
		uint32_t long_mode                                             : 1;
#define SEGMENT__LONG_MODE_BIT                                       21
#define SEGMENT__LONG_MODE_FLAG                                      0x200000
#define SEGMENT__LONG_MODE_MASK                                      0x01
#define SEGMENT__LONG_MODE(_)                                        (((_) >> 21) & 0x01)

		/**
		* @brief D/B (default operation size/default stack pointer size and/or upper bound) flag
		*
		* [Bit 22] Performs different functions depending on whether the segment descriptor is an executable code segment, an
		* expand-down data segment, or a stack segment. (This flag should always be set to 1 for 32-bit code and data segments and
		* to 0 for 16-bit code and data segments.)
		* - Executable code segment. The flag is called the D flag and it indicates the default length for effective addresses and
		* operands referenced by instructions in the segment. If the flag is set, 32-bit addresses and 32-bit or 8-bit operands
		* are assumed; if it is clear, 16-bit addresses and 16-bit or 8-bit operands are assumed. The instruction prefix 66H can
		* be used to select an operand size other than the default, and the prefix 67H can be used select an address size other
		* than the default.
		* - Stack segment (data segment pointed to by the SS register). The flag is called the B (big) flag and it specifies the
		* size of the stack pointer used for implicit stack operations (such as pushes, pops, and calls). If the flag is set, a
		* 32-bit stack pointer is used, which is stored in the 32-bit ESP register; if the flag is clear, a 16-bit stack pointer
		* is used, which is stored in the 16- bit SP register. If the stack segment is set up to be an expand-down data segment
		* (described in the next paragraph), the B flag also specifies the upper bound of the stack segment.
		* - Expand-down data segment. The flag is called the B flag and it specifies the upper bound of the segment. If the flag
		* is set, the upper bound is FFFFFFFFH (4 GBytes); if the flag is clear, the upper bound is FFFFH (64 KBytes).
		*/
		uint32_t default_big                                           : 1;
#define SEGMENT__DEFAULT_BIG_BIT                                     22
#define SEGMENT__DEFAULT_BIG_FLAG                                    0x400000
#define SEGMENT__DEFAULT_BIG_MASK                                    0x01
#define SEGMENT__DEFAULT_BIG(_)                                      (((_) >> 22) & 0x01)

		/**
		* @brief G (granularity) flag
		*
		* [Bit 23] Determines the scaling of the segment limit field. When the granularity flag is clear, the segment limit is
		* interpreted in byte units; when flag is set, the segment limit is interpreted in 4-KByte units. (This flag does not
		* affect the granularity of the base address; it is always byte granular.) When the granularity flag is set, the twelve
		* least significant bits of an offset are not tested when checking the offset against the segment limit. For example, when
		* the granularity flag is set, a limit of 0 results in valid offsets from 0 to 4095.
		*/
		uint32_t granularity                                           : 1;
#define SEGMENT__GRANULARITY_BIT                                     23
#define SEGMENT__GRANULARITY_FLAG                                    0x800000
#define SEGMENT__GRANULARITY_MASK                                    0x01
#define SEGMENT__GRANULARITY(_)                                      (((_) >> 23) & 0x01)

		/**
		* [Bits 31:24] Base address field (31:24); see description of $BASE_LOW for more details.
		*/
		uint32_t base_address_high                                     : 8;
#define SEGMENT__BASE_ADDRESS_HIGH_BIT                               24
#define SEGMENT__BASE_ADDRESS_HIGH_FLAG                              0xFF000000
#define SEGMENT__BASE_ADDRESS_HIGH_MASK                              0xFF
#define SEGMENT__BASE_ADDRESS_HIGH(_)                                (((_) >> 24) & 0xFF)
	};

	uint32_t flags;
  } ;


  /**
	* Base address field (32:63); see description of $BASE_LOW for more details.
	*/
  uint32_t base_address_upper;
} segment_descriptor_64;

#define SEGMENT_DESCRIPTOR_TYPE_SYSTEM                               0x00000000
#define SEGMENT_DESCRIPTOR_TYPE_CODE_OR_DATA                         0x00000001
/**
 * @defgroup segment_descriptor_code_and_data_type \
 *           Code- and Data-Segment Descriptor Types
 *
 * When the S (descriptor type) flag in a segment descriptor is set, the descriptor is for either a code or a data segment.
 * The highest order bit of the type field (bit 11 of the second double word of the segment descriptor) then determines
 * whether the descriptor is for a data segment (clear) or a code segment (set). For data segments, the three low-order
 * bits of the type field (bits 8, 9, and 10) are interpreted as accessed (A), write-enable (W), and expansion-direction
 * (E). See Table 3-1 for a description of the encoding of the bits in the type field for code and data segments. Data
 * segments can be read-only or read/write segments, depending on the setting of the write-enable bit.
 *
 * @see Vol3A[3.4.5.1(Code- and Data-Segment Descriptor Types)] (reference)
 * @{
 */
/**
 * Read-Only.
 */
#define SEGMENT_DESCRIPTOR_TYPE_DATA_READ_ONLY                       0x00000000

/**
 * Data Read-Only, accessed.
 */
#define SEGMENT_DESCRIPTOR_TYPE_DATA_READ_ONLY_ACCESSED              0x00000001

/**
 * Data Read/Write.
 */
#define SEGMENT_DESCRIPTOR_TYPE_DATA_READ_WRITE                      0x00000002

/**
 * Data Read/Write, accessed.
 */
#define SEGMENT_DESCRIPTOR_TYPE_DATA_READ_WRITE_ACCESSED             0x00000003

/**
 * Data Read-Only, expand-down.
 */
#define SEGMENT_DESCRIPTOR_TYPE_DATA_READ_ONLY_EXPAND_DOWN           0x00000004

/**
 * Data Read-Only, expand-down, accessed.
 */
#define SEGMENT_DESCRIPTOR_TYPE_DATA_READ_ONLY_EXPAND_DOWN_ACCESSED  0x00000005

/**
 * Data Read/Write, expand-down.
 */
#define SEGMENT_DESCRIPTOR_TYPE_DATA_READ_WRITE_EXPAND_DOWN          0x00000006

/**
 * Data Read/Write, expand-down, accessed.
 */
#define SEGMENT_DESCRIPTOR_TYPE_DATA_READ_WRITE_EXPAND_DOWN_ACCESSED 0x00000007

/**
 * Code Execute-Only.
 */
#define SEGMENT_DESCRIPTOR_TYPE_CODE_EXECUTE_ONLY                    0x00000008

/**
 * Code Execute-Only, accessed.
 */
#define SEGMENT_DESCRIPTOR_TYPE_CODE_EXECUTE_ONLY_ACCESSED           0x00000009

/**
 * Code Execute/Read.
 */
#define SEGMENT_DESCRIPTOR_TYPE_CODE_EXECUTE_READ                    0x0000000A

/**
 * Code Execute/Read, accessed.
 */
#define SEGMENT_DESCRIPTOR_TYPE_CODE_EXECUTE_READ_ACCESSED           0x0000000B

/**
 * Code Execute-Only, conforming.
 */
#define SEGMENT_DESCRIPTOR_TYPE_CODE_EXECUTE_ONLY_CONFORMING         0x0000000C

/**
 * Code Execute-Only, conforming, accessed.
 */
#define SEGMENT_DESCRIPTOR_TYPE_CODE_EXECUTE_ONLY_CONFORMING_ACCESSED 0x0000000D

/**
 * Code Execute/Read, conforming.
 */
#define SEGMENT_DESCRIPTOR_TYPE_CODE_EXECUTE_READ_CONFORMING         0x0000000E

/**
 * Code Execute/Read, conforming, accessed.
 */
#define SEGMENT_DESCRIPTOR_TYPE_CODE_EXECUTE_READ_CONFORMING_ACCESSED 0x0000000F
/**
 * @}
 */

/**
 * @defgroup segment_descriptor_system_type \
 *           System Descriptor Types
 *
 * When the S (descriptor type) flag in a segment descriptor is clear, the descriptor type is a system descriptor. The
 * processor recognizes the following types of system descriptors:
 * - Local descriptor-table (LDT) segment descriptor.
 * - Task-state segment (TSS) descriptor.
 * - Call-gate descriptor.
 * - Interrupt-gate descriptor.
 * - Trap-gate descriptor.
 * - Task-gate descriptor.
 * These descriptor types fall into two categories: system-segment descriptors and gate descriptors. Systemsegment
 * descriptors point to system segments (LDT and TSS segments). Gate descriptors are in themselves "gates," which hold
 * pointers to procedure entry points in code segments (call, interrupt, and trap gates) or which hold segment selectors
 * for TSS's (task gates).
 *
 * @see Vol3A[3.5(SYSTEM DESCRIPTOR TYPES)] (reference)
 * @{
 */
/**
 * - 32-Bit Mode: Reserved
 * - IA-32e Mode: Reserved
 */
#define SEGMENT_DESCRIPTOR_TYPE_RESERVED_1                           0x00000000

/**
 * - 32-Bit Mode: 16-bit TSS (Available)
 * - IA-32e Mode: Reserved
 */
#define SEGMENT_DESCRIPTOR_TYPE_TSS_16_AVAILABLE                     0x00000001

/**
 * - 32-Bit Mode: LDT
 * - IA-32e Mode: LDT
 */
#define SEGMENT_DESCRIPTOR_TYPE_LDT                                  0x00000002

/**
 * - 32-Bit Mode: 16-bit TSS (Busy)
 * - IA-32e Mode: Reserved
 */
#define SEGMENT_DESCRIPTOR_TYPE_TSS_16_BUSY                          0x00000003

/**
 * - 32-Bit Mode: 16-bit Call Gate
 * - IA-32e Mode: Reserved
 */
#define SEGMENT_DESCRIPTOR_TYPE_CALL_GATE_16                         0x00000004

/**
 * - 32-Bit Mode: Task Gate
 * - IA-32e Mode: Reserved
 */
#define SEGMENT_DESCRIPTOR_TYPE_TASK_GATE                            0x00000005

/**
 * - 32-Bit Mode: 16-bit Interrupt Gate
 * - IA-32e Mode: Reserved
 */
#define SEGMENT_DESCRIPTOR_TYPE_INTERRUPT_GATE_16                    0x00000006

/**
 * - 32-Bit Mode: 16-bit Trap Gate
 * - IA-32e Mode: Reserved
 */
#define SEGMENT_DESCRIPTOR_TYPE_TRAP_GATE_16                         0x00000007

/**
 * - 32-Bit Mode: Reserved
 * - IA-32e Mode: Reserved
 */
#define SEGMENT_DESCRIPTOR_TYPE_RESERVED_2                           0x00000008

/**
 * - 32-Bit Mode: 32-bit TSS (Available)
 * - IA-32e Mode: 64-bit TSS (Available)
 */
#define SEGMENT_DESCRIPTOR_TYPE_TSS_AVAILABLE                        0x00000009

/**
 * - 32-Bit Mode: Reserved
 * - IA-32e Mode: Reserved
 */
#define SEGMENT_DESCRIPTOR_TYPE_RESERVED_3                           0x0000000A

/**
 * - 32-Bit Mode: 32-bit TSS (Busy)
 * - IA-32e Mode: 64-bit TSS (Busy)
 */
#define SEGMENT_DESCRIPTOR_TYPE_TSS_BUSY                             0x0000000B

/**
 * - 32-Bit Mode: 32-bit Call Gate
 * - IA-32e Mode: 64-bit Call Gate
 */
#define SEGMENT_DESCRIPTOR_TYPE_CALL_GATE                            0x0000000C

/**
 * - 32-Bit Mode: Reserved
 * - IA-32e Mode: Reserved
 */
#define SEGMENT_DESCRIPTOR_TYPE_RESERVED_4                           0x0000000D

/**
 * - 32-Bit Mode: 32-bit Interrupt Gate
 * - IA-32e Mode: 64-bit Interrupt Gate
 */
#define SEGMENT_DESCRIPTOR_TYPE_INTERRUPT_GATE                       0x0000000E

/**
 * - 32-Bit Mode: 32-bit Trap Gate
 * - IA-32e Mode: 64-bit Trap Gate
 */
#define SEGMENT_DESCRIPTOR_TYPE_TRAP_GATE                            0x0000000F
/**
 * @}
 */

/**
 * @brief A segment selector is a 16-bit identifier for a segment. It does not point directly to the segment, but instead
 *        points to the segment descriptor that defines the segment
 *
 * @see Vol3A[3.4.2(Segment Selectors)] (reference)
 */
typedef union
{
  struct
  {
	/**
	* [Bits 1:0] Specifies the privilege level of the selector. The privilege level can range from 0 to 3, with 0 being the
	* most privileged level.
	*
	* @see Vol3A[5.5(Privilege Levels)]
	*/
	uint16_t request_privilege_level                                 : 2;
#define SEGMENT_SELECTOR_REQUEST_PRIVILEGE_LEVEL_BIT                 0
#define SEGMENT_SELECTOR_REQUEST_PRIVILEGE_LEVEL_FLAG                0x03
#define SEGMENT_SELECTOR_REQUEST_PRIVILEGE_LEVEL_MASK                0x03
#define SEGMENT_SELECTOR_REQUEST_PRIVILEGE_LEVEL(_)                  (((_) >> 0) & 0x03)

	/**
	* [Bit 2] Specifies the descriptor table to use: clearing this flag selects the GDT; setting this flag selects the current
	* LDT.
	*/
	uint16_t table                                                   : 1;
#define SEGMENT_SELECTOR_TABLE_BIT                                   2
#define SEGMENT_SELECTOR_TABLE_FLAG                                  0x04
#define SEGMENT_SELECTOR_TABLE_MASK                                  0x01
#define SEGMENT_SELECTOR_TABLE(_)                                    (((_) >> 2) & 0x01)

	/**
	* [Bits 15:3] Selects one of 8192 descriptors in the GDT or LDT. The processor multiplies the index value by 8 (the number
	* of bytes in a segment descriptor) and adds the result to the base address of the GDT or LDT (from the GDTR or LDTR
	* register, respectively).
	*/
	uint16_t index                                                   : 13;
#define SEGMENT_SELECTOR_INDEX_BIT                                   3
#define SEGMENT_SELECTOR_INDEX_FLAG                                  0xFFF8
#define SEGMENT_SELECTOR_INDEX_MASK                                  0x1FFF
#define SEGMENT_SELECTOR_INDEX(_)                                    (((_) >> 3) & 0x1FFF)
  };

  uint16_t flags;
} segment_selector;

/**
 * @}
 */

/**
 * @defgroup vmx \
 *           VMX
 * @{
 */
/**
 * @{
 */
/**
 * @defgroup vmx_basic_exit_reasons \
 *           VMX Basic Exit Reasons
 *
 * VMX Basic Exit Reasons.
 *
 * @see Vol3D[C(VMX BASIC EXIT REASONS)] (reference)
 * @{
 */
/**
 * @brief Exception or non-maskable interrupt (NMI)
 *
 * Either:
 * -# Guest software caused an exception and the bit in the exception bitmap associated with exception's vector was 1. This
 * case includes executions of BOUND that cause \#BR, executions of INT1 (they cause \#DB), executions of INT3 (they cause
 * \#BP), executions of INTO that cause \#OF, and executions of UD0, UD1, and UD2 (they cause \#UD).
 * -# An NMI was delivered to the logical processor and the "NMI exiting" VM-execution control was 1.
 */
#define VMX_EXIT_REASON_EXCEPTION_OR_NMI                             0x00000000

/**
 * @brief External interrupt
 *
 * An external interrupt arrived and the "external-interrupt exiting" VM-execution control was 1.
 */
#define VMX_EXIT_REASON_EXTERNAL_INTERRUPT                           0x00000001

/**
 * @brief Triple fault
 *
 * The logical processor encountered an exception while attempting to call the double-fault handler and that exception did
 * not itself cause a VM exit due to the exception bitmap.
 */
#define VMX_EXIT_REASON_TRIPLE_FAULT                                 0x00000002

/**
 * @brief INIT signal
 *
 * An INIT signal arrived.
 */
#define VMX_EXIT_REASON_INIT_SIGNAL                                  0x00000003

/**
 * @brief Start-up IPI (SIPI)
 *
 * A SIPI arrived while the logical processor was in the "wait-for-SIPI" state.
 */
#define VMX_EXIT_REASON_STARTUP_IPI                                  0x00000004

/**
 * @brief I/O system-management interrupt (SMI)
 *
 * An SMI arrived immediately after retirement of an I/O instruction and caused an SMM VM exit.
 *
 * @see Vol3C[34.15.2(SMM VM Exits)]
 */
#define VMX_EXIT_REASON_IO_SMI                                       0x00000005

/**
 * @brief Other SMI
 *
 * An SMI arrived and caused an SMM VM exit but not immediately after retirement of an I/O instruction.
 *
 * @see Vol3C[34.15.2(SMM VM Exits)]
 */
#define VMX_EXIT_REASON_SMI                                          0x00000006

/**
 * @brief Interrupt window exiting
 *
 * At the beginning of an instruction, RFLAGS.IF was 1; events were not blocked by STI or by MOV SS; and the
 * "interrupt-window exiting" VM-execution control was 1.
 */
#define VMX_EXIT_REASON_INTERRUPT_WINDOW                             0x00000007

/**
 * @brief NMI window exiting
 *
 * At the beginning of an instruction, there was no virtual-NMI blocking; events were not blocked by MOV SS; and the
 * "NMI-window exiting" VM-execution control was 1.
 */
#define VMX_EXIT_REASON_NMI_WINDOW                                   0x00000008

/**
 * @brief Task switch
 *
 * Guest software attempted a task switch.
 */
#define VMX_EXIT_REASON_TASK_SWITCH                                  0x00000009

/**
 * @brief CPUID
 *
 * Guest software attempted to execute CPUID.
 */
#define VMX_EXIT_REASON_EXECUTE_CPUID                                0x0000000A

/**
 * @brief GETSEC
 *
 * Guest software attempted to execute GETSEC.
 */
#define VMX_EXIT_REASON_EXECUTE_GETSEC                               0x0000000B

/**
 * @brief HLT
 *
 * Guest software attempted to execute HLT and the "HLT exiting" VM-execution control was 1.
 */
#define VMX_EXIT_REASON_EXECUTE_HLT                                  0x0000000C

/**
 * @brief INVD
 *
 * Guest software attempted to execute INVD.
 */
#define VMX_EXIT_REASON_EXECUTE_INVD                                 0x0000000D

/**
 * @brief INVLPG
 *
 * Guest software attempted to execute INVLPG and the "INVLPG exiting" VM-execution control was 1.
 */
#define VMX_EXIT_REASON_EXECUTE_INVLPG                               0x0000000E

/**
 * @brief RDPMC
 *
 * Guest software attempted to execute RDPMC and the "RDPMC exiting" VM-execution control was 1.
 */
#define VMX_EXIT_REASON_EXECUTE_RDPMC                                0x0000000F

/**
 * @brief RDTSC
 *
 * Guest software attempted to execute RDTSC and the "RDTSC exiting" VM-execution control was 1.
 */
#define VMX_EXIT_REASON_EXECUTE_RDTSC                                0x00000010

/**
 * @brief RSM in SMM
 *
 * Guest software attempted to execute RSM in SMM.
 */
#define VMX_EXIT_REASON_EXECUTE_RSM_IN_SMM                           0x00000011

/**
 * @brief VMCALL
 *
 * VMCALL was executed either by guest software (causing an ordinary VM exit) or by the executive monitor (causing an SMM
 * VM exit).
 *
 * @see Vol3C[34.15.2(SMM VM Exits)]
 */
#define VMX_EXIT_REASON_EXECUTE_VMCALL                               0x00000012

/**
 * @brief VMCLEAR
 *
 * Guest software attempted to execute VMCLEAR.
 */
#define VMX_EXIT_REASON_EXECUTE_VMCLEAR                              0x00000013

/**
 * @brief VMLAUNCH
 *
 * Guest software attempted to execute VMLAUNCH.
 */
#define VMX_EXIT_REASON_EXECUTE_VMLAUNCH                             0x00000014

/**
 * @brief VMPTRLD
 *
 * Guest software attempted to execute VMPTRLD.
 */
#define VMX_EXIT_REASON_EXECUTE_VMPTRLD                              0x00000015

/**
 * @brief VMPTRST
 *
 * Guest software attempted to execute VMPTRST.
 */
#define VMX_EXIT_REASON_EXECUTE_VMPTRST                              0x00000016

/**
 * @brief VMREAD
 *
 * Guest software attempted to execute VMREAD.
 */
#define VMX_EXIT_REASON_EXECUTE_VMREAD                               0x00000017

/**
 * @brief VMRESUME
 *
 * Guest software attempted to execute VMRESUME.
 */
#define VMX_EXIT_REASON_EXECUTE_VMRESUME                             0x00000018

/**
 * @brief VMWRITE
 *
 * Guest software attempted to execute VMWRITE.
 */
#define VMX_EXIT_REASON_EXECUTE_VMWRITE                              0x00000019

/**
 * @brief VMXOFF
 *
 * Guest software attempted to execute VMXOFF.
 */
#define VMX_EXIT_REASON_EXECUTE_VMXOFF                               0x0000001A

/**
 * @brief VMXON
 *
 * Guest software attempted to execute VMXON.
 */
#define VMX_EXIT_REASON_EXECUTE_VMXON                                0x0000001B

/**
 * @brief Control-register accesses
 *
 * Guest software attempted to access CR0, CR3, CR4, or CR8 using CLTS, LMSW, or MOV CR and the VM-execution control fields
 * indicate that a VM exit should occur. This basic exit reason is not used for trap-like VM exits following executions of
 * the MOV to CR8 instruction when the "use TPR shadow" VM-execution control is 1. Such VM exits instead use basic exit
 * reason 43.
 *
 * @see Vol3C[25.1(INSTRUCTIONS THAT CAUSE VM EXITS)]
 */
#define VMX_EXIT_REASON_MOV_CR                                       0x0000001C

/**
 * @brief Debug-register accesses
 *
 * Guest software attempted a MOV to or from a debug register and the "MOV-DR exiting" VM-execution control was 1.
 */
#define VMX_EXIT_REASON_MOV_DR                                       0x0000001D

/**
 * @brief I/O instruction
 *
 * Guest software attempted to execute an I/O instruction and either:
 * -# The "use I/O bitmaps" VM-execution control was 0 and the "unconditional I/O exiting" VM-execution control was 1.
 * -# The "use I/O bitmaps" VM-execution control was 1 and a bit in the I/O bitmap associated with one of the ports
 * accessed by the I/O instruction was 1.
 */
#define VMX_EXIT_REASON_EXECUTE_IO_INSTRUCTION                       0x0000001E

/**
 * @brief RDMSR
 *
 * Guest software attempted to execute RDMSR and either:
 * -# The "use MSR bitmaps" VM-execution control was 0.
 * -# The value of RCX is neither in the range 00000000H - 00001FFFH nor in the range C0000000H - C0001FFFH.
 * -# The value of RCX was in the range 00000000H - 00001FFFH and the nth bit in read bitmap for low MSRs is 1, where n was
 * the value of RCX.
 * -# The value of RCX is in the range C0000000H - C0001FFFH and the nth bit in read bitmap for high MSRs is 1, where n is
 * the value of RCX & 00001FFFH.
 */
#define VMX_EXIT_REASON_EXECUTE_RDMSR                                0x0000001F

/**
 * @brief WRMSR
 *
 * Guest software attempted to execute WRMSR and either:
 * -# The "use MSR bitmaps" VM-execution control was 0.
 * -# The value of RCX is neither in the range 00000000H - 00001FFFH nor in the range C0000000H - C0001FFFH.
 * -# The value of RCX was in the range 00000000H - 00001FFFH and the nth bit in write bitmap for low MSRs is 1, where n
 * was the value of RCX.
 * -# The value of RCX is in the range C0000000H - C0001FFFH and the nth bit in write bitmap for high MSRs is 1, where n is
 * the value of RCX & 00001FFFH.
 */
#define VMX_EXIT_REASON_EXECUTE_WRMSR                                0x00000020

/**
 * @brief VM-entry failure due to invalid guest state
 *
 * A VM entry failed one of the checks identified in Section 26.3.1.
 */
#define VMX_EXIT_REASON_ERROR_INVALID_GUEST_STATE                    0x00000021

/**
 * @brief VM-entry failure due to MSR loading
 *
 * A VM entry failed in an attempt to load MSRs. See Section 26.4.
 */
#define VMX_EXIT_REASON_ERROR_MSR_LOAD                               0x00000022

/**
 * @brief Guest software executed MWAIT
 *
 * Guest software attempted to execute MWAIT and the "MWAIT exiting" VM-execution control was 1.
 */
#define VMX_EXIT_REASON_EXECUTE_MWAIT                                0x00000024

/**
 * @brief VM-exit due to monitor trap flag
 *
 * A VM entry occurred due to the 1-setting of the "monitor trap flag" VM-execution control and injection of an MTF VM exit
 * as part of VM entry.
 *
 * @see Vol3C[25.5.2(Monitor Trap Flag)]
 */
#define VMX_EXIT_REASON_MONITOR_TRAP_FLAG                            0x00000025

/**
 * @brief Guest software attempted to execute MONITOR
 *
 * Guest software attempted to execute MONITOR and the "MONITOR exiting" VM-execution control was 1.
 */
#define VMX_EXIT_REASON_EXECUTE_MONITOR                              0x00000027

/**
 * @brief Guest software attempted to execute PAUSE
 *
 * Either guest software attempted to execute PAUSE and the "PAUSE exiting" VM-execution control was 1 or the "PAUSE-loop
 * exiting" VM-execution control was 1 and guest software executed a PAUSE loop with execution time exceeding PLE_Window.
 *
 * @see Vol3C[25.1.3(Instructions That Cause VM Exits Conditionally)]
 */
#define VMX_EXIT_REASON_EXECUTE_PAUSE                                0x00000028

/**
 * @brief VM-entry failure due to machine-check
 *
 * A machine-check event occurred during VM entry.
 *
 * @see Vol3C[26.8(MACHINE-CHECK EVENTS DURING VM ENTRY)]
 */
#define VMX_EXIT_REASON_ERROR_MACHINE_CHECK                          0x00000029

/**
 * @brief TPR below threshold
 *
 * The logical processor determined that the value of bits 7:4 of the byte at offset 080H on the virtual-APIC page was
 * below that of the TPR threshold VM-execution control field while the "use TPR shadow" VMexecution control was 1 either
 * as part of TPR virtualization or VM entry.
 *
 * @see Vol3C[29.1.2(TPR Virtualization)]
 * @see Vol3C[26.6.7(VM Exits Induced by the TPR Threshold)]
 */
#define VMX_EXIT_REASON_TPR_BELOW_THRESHOLD                          0x0000002B

/**
 * @brief APIC access
 *
 * Guest software attempted to access memory at a physical address on the APIC-access page and the "virtualize APIC
 * accesses" VM-execution control was 1.
 *
 * @see Vol3C[29.4(VIRTUALIZING MEMORY-MAPPED APIC ACCESSES)]
 */
#define VMX_EXIT_REASON_APIC_ACCESS                                  0x0000002C

/**
 * @brief Virtualized EOI
 *
 * EOI virtualization was performed for a virtual interrupt whose vector indexed a bit set in the EOIexit bitmap.
 */
#define VMX_EXIT_REASON_VIRTUALIZED_EOI                              0x0000002D

/**
 * @brief Access to GDTR or IDTR
 *
 * Guest software attempted to execute LGDT, LIDT, SGDT, or SIDT and the "descriptor-table exiting" VM-execution control
 * was 1.
 */
#define VMX_EXIT_REASON_GDTR_IDTR_ACCESS                             0x0000002E

/**
 * @brief Access to LDTR or TR
 *
 * Guest software attempted to execute LLDT, LTR, SLDT, or STR and the "descriptor-table exiting" VM-execution control was
 * 1.
 */
#define VMX_EXIT_REASON_LDTR_TR_ACCESS                               0x0000002F

/**
 * @brief EPT violation
 *
 * An attempt to access memory with a guest-physical address was disallowed by the configuration of the EPT paging
 * structures.
 */
#define VMX_EXIT_REASON_EPT_VIOLATION                                0x00000030

/**
 * @brief EPT misconfiguration
 *
 * An attempt to access memory with a guest-physical address encountered a misconfigured EPT paging-structure entry.
 */
#define VMX_EXIT_REASON_EPT_MISCONFIGURATION                         0x00000031

/**
 * @brief INVEPT
 *
 * Guest software attempted to execute INVEPT.
 */
#define VMX_EXIT_REASON_EXECUTE_INVEPT                               0x00000032

/**
 * @brief RDTSCP
 *
 * Guest software attempted to execute RDTSCP and the "enable RDTSCP" and "RDTSC exiting" VM-execution controls were both
 * 1.
 */
#define VMX_EXIT_REASON_EXECUTE_RDTSCP                               0x00000033

/**
 * @brief VMX-preemption timer expired
 *
 * The preemption timer counted down to zero.
 */
#define VMX_EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED                 0x00000034

/**
 * @brief INVVPID
 *
 * Guest software attempted to execute INVVPID.
 */
#define VMX_EXIT_REASON_EXECUTE_INVVPID                              0x00000035

/**
 * @brief WBINVD
 *
 * Guest software attempted to execute WBINVD and the "WBINVD exiting" VM-execution control was 1.
 */
#define VMX_EXIT_REASON_EXECUTE_WBINVD                               0x00000036

/**
 * @brief XSETBV - Guest software attempted to execute XSETBV
 *
 * Guest software attempted to execute XSETBV.
 */
#define VMX_EXIT_REASON_EXECUTE_XSETBV                               0x00000037

/**
 * @brief APIC write
 *
 * Guest software completed a write to the virtual-APIC page that must be virtualized by VMM software.
 *
 * @see Vol3C[29.4.3.3(APIC-Write VM Exits)]
 */
#define VMX_EXIT_REASON_APIC_WRITE                                   0x00000038

/**
 * @brief RDRAND
 *
 * Guest software attempted to execute RDRAND and the "RDRAND exiting" VM-execution control was 1.
 */
#define VMX_EXIT_REASON_EXECUTE_RDRAND                               0x00000039

/**
 * @brief INVPCID
 *
 * Guest software attempted to execute INVPCID and the "enable INVPCID" and "INVLPG exiting" VM-execution controls were
 * both 1.
 */
#define VMX_EXIT_REASON_EXECUTE_INVPCID                              0x0000003A

/**
 * @brief VMFUNC
 *
 * Guest software invoked a VM function with the VMFUNC instruction and the VM function either was not enabled or generated
 * a function-specific condition causing a VM exit.
 */
#define VMX_EXIT_REASON_EXECUTE_VMFUNC                               0x0000003B

/**
 * @brief ENCLS
 *
 * Guest software attempted to execute ENCLS and "enable ENCLS exiting" VM-execution control was 1 and either:
 * -# EAX < 63 and the corresponding bit in the ENCLS-exiting bitmap is 1; or
 * -# EAX >= 63 and bit 63 in the ENCLS-exiting bitmap is 1.
 */
#define VMX_EXIT_REASON_EXECUTE_ENCLS                                0x0000003C

/**
 * @brief RDSEED
 *
 * Guest software attempted to execute RDSEED and the "RDSEED exiting" VM-execution control was 1.
 */
#define VMX_EXIT_REASON_EXECUTE_RDSEED                               0x0000003D

/**
 * @brief Page-modification log full
 *
 * The processor attempted to create a page-modification log entry and the value of the PML index was not in the range
 * 0-511.
 */
#define VMX_EXIT_REASON_PAGE_MODIFICATION_LOG_FULL                   0x0000003E

/**
 * @brief XSAVES
 *
 * Guest software attempted to execute XSAVES, the "enable XSAVES/XRSTORS" was 1, and a bit was set in the logical-AND of
 * the following three values: EDX:EAX, the IA32_XSS MSR, and the XSS-exiting bitmap.
 */
#define VMX_EXIT_REASON_EXECUTE_XSAVES                               0x0000003F

/**
 * @brief XRSTORS
 *
 * Guest software attempted to execute XRSTORS, the "enable XSAVES/XRSTORS" was 1, and a bit was set in the logical-AND of
 * the following three values: EDX:EAX, the IA32_XSS MSR, and the XSS-exiting bitmap.
 */
#define VMX_EXIT_REASON_EXECUTE_XRSTORS                              0x00000040
/**
 * @}
 */

/**
 * @defgroup vmx_instruction_error_numbers \
 *           VM-Instruction Error Numbers
 *
 * VM-Instruction Error Numbers.
 *
 * @see Vol3C[30.4(VM INSTRUCTION ERROR NUMBERS)] (reference)
 * @{
 */
/**
 * VMCALL executed in VMX root operation.
 */
#define VMX_ERROR_VMCALL_IN_VMX_ROOT_OPERATION                       0x00000001

/**
 * VMCLEAR with invalid physical address.
 */
#define VMX_ERROR_VMCLEAR_INVALID_PHYSICAL_ADDRESS                   0x00000002

/**
 * VMCLEAR with VMXON pointer.
 */
#define VMX_ERROR_VMCLEAR_INVALID_VMXON_POINTER                      0x00000003

/**
 * VMLAUNCH with non-clear VMCS.
 */
#define VMX_ERROR_VMLAUCH_NON_CLEAR_VMCS                             0x00000004

/**
 * VMRESUME with non-launched VMCS.
 */
#define VMX_ERROR_VMRESUME_NON_LAUNCHED_VMCS                         0x00000005

/**
 * VMRESUME after VMXOFF (VMXOFF and VMXON between VMLAUNCH and VMRESUME).
 */
#define VMX_ERROR_VMRESUME_AFTER_VMXOFF                              0x00000006

/**
 * VM entry with invalid control field(s).
 */
#define VMX_ERROR_VMENTRY_INVALID_CONTROL_FIELDS                     0x00000007

/**
 * VM entry with invalid host-state field(s).
 */
#define VMX_ERROR_VMENTRY_INVALID_HOST_STATE                         0x00000008

/**
 * VMPTRLD with invalid physical address.
 */
#define VMX_ERROR_VMPTRLD_INVALID_PHYSICAL_ADDRESS                   0x00000009

/**
 * VMPTRLD with VMXON pointer.
 */
#define VMX_ERROR_VMPTRLD_VMXON_POINTER                              0x0000000A

/**
 * VMPTRLD with incorrect VMCS revision identifier.
 */
#define VMX_ERROR_VMPTRLD_INCORRECT_VMCS_REVISION_ID                 0x0000000B

/**
 * VMREAD/VMWRITE from/to unsupported VMCS component.
 */
#define VMX_ERROR_VMREAD_VMWRITE_INVALID_COMPONENT                   0x0000000C

/**
 * VMWRITE to read-only VMCS component.
 */
#define VMX_ERROR_VMWRITE_READONLY_COMPONENT                         0x0000000D

/**
 * VMXON executed in VMX root operation.
 */
#define VMX_ERROR_VMXON_IN_VMX_ROOT_OP                               0x0000000F

/**
 * VM entry with invalid executive-VMCS pointer.
 */
#define VMX_ERROR_VMENTRY_INVALID_VMCS_EXECUTIVE_POINTER             0x00000010

/**
 * VM entry with non-launched executive VMCS.
 */
#define VMX_ERROR_VMENTRY_NON_LAUNCHED_EXECUTIVE_VMCS                0x00000011

/**
 * VM entry with executive-VMCS pointer not VMXON pointer (when attempting to deactivate the dual-monitor treatment of SMIs
 * and SMM).
 */
#define VMX_ERROR_VMENTRY_EXECUTIVE_VMCS_PTR                         0x00000012

/**
 * VMCALL with non-clear VMCS (when attempting to activate the dual-monitor treatment of SMIs and SMM).
 */
#define VMX_ERROR_VMCALL_NON_CLEAR_VMCS                              0x00000013

/**
 * VMCALL with invalid VM-exit control fields.
 */
#define VMX_ERROR_VMCALL_INVALID_VMEXIT_FIELDS                       0x00000014

/**
 * VMCALL with incorrect MSEG revision identifier (when attempting to activate the dual-monitor treatment of SMIs and SMM).
 */
#define VMX_ERROR_VMCALL_INVALID_MSEG_REVISION_ID                    0x00000016

/**
 * VMXOFF under dual-monitor treatment of SMIs and SMM.
 */
#define VMX_ERROR_VMXOFF_DUAL_MONITOR                                0x00000017

/**
 * VMCALL with invalid SMM-monitor features (when attempting to activate the dual-monitor treatment of SMIs and SMM).
 */
#define VMX_ERROR_VMCALL_INVALID_SMM_MONITOR                         0x00000018

/**
 * VM entry with invalid VM-execution control fields in executive VMCS (when attempting to return from SMM).
 */
#define VMX_ERROR_VMENTRY_INVALID_VM_EXECUTION_CONTROL               0x00000019

/**
 * VM entry with events blocked by MOV SS.
 */
#define VMX_ERROR_VMENTRY_MOV_SS                                     0x0000001A

/**
 * Invalid operand to INVEPT/INVVPID.
 */
#define VMX_ERROR_INVEPT_INVVPID_INVALID_OPERAND                     0x0000001C
/**
 * @}
 */

/**
 * @defgroup vmx_exceptions \
 *           Virtualization Exceptions
 *
 * Virtualization Exceptions.
 *
 * @see Vol3C[25.5.6(Virtualization Exceptions)] (reference)
 * @{
 */
typedef struct
{
  /**
	* The 32-bit value that would have been saved into the VMCS as an exit reason had a VM exit occurred instead of the
	* virtualization exception. For EPT violations, this value is 48 (00000030H).
	*/
  uint32_t reason;

  /**
	* FFFFFFFFH
	*/
  uint32_t exception_mask;

  /**
	* The 64-bit value that would have been saved into the VMCS as an exit qualification had a VM exit occurred instead of the
	* virtualization exception.
	*/
  uint64_t exit;

  /**
	* The 64-bit value that would have been saved into the VMCS as a guest-linear address had a VM exit occurred instead of
	* the virtualization exception.
	*/
  uint64_t guest_linear_address;

  /**
	* The 64-bit value that would have been saved into the VMCS as a guest-physical address had a VM exit occurred instead of
	* the virtualization exception.
	*/
  uint64_t guest_physical_address;

  /**
	* The current 16-bit value of the EPTP index VM-execution control.
	*
	* @see Vol3C[24.6.18(Controls for Virtualization Exceptions)]
	* @see Vol3C[25.5.5.3(EPTP Switching)]
	*/
  uint16_t current_eptp_index;
} vmx_virtualization_exception_information;

/**
 * @}
 */

/**
 * @defgroup vmx_basic_exit_information \
 *           Basic VM-Exit Information
 *
 * Basic VM-Exit Information.
 *
 * @see Vol3C[27.2.1(Basic VM-Exit Information)] (reference)
 * @{
 */
/**
 * @brief Exit Qualification for Debug Exceptions
 */
typedef union
{
  struct
  {
	/**
	* @brief B0 - B3
	*
	* [Bits 3:0] When set, each of these bits indicates that the corresponding breakpoint condition was met. Any of these bits
	* may be set even if its corresponding enabling bit in DR7 is not set.
	*/
	uint64_t breakpoint_condition                                    : 4;
#define VMX_EXIT_QUALIFICATION_DEBUG_EXCEPTION_BREAKPOINT_CONDITION_BIT 0
#define VMX_EXIT_QUALIFICATION_DEBUG_EXCEPTION_BREAKPOINT_CONDITION_FLAG 0x0F
#define VMX_EXIT_QUALIFICATION_DEBUG_EXCEPTION_BREAKPOINT_CONDITION_MASK 0x0F
#define VMX_EXIT_QUALIFICATION_DEBUG_EXCEPTION_BREAKPOINT_CONDITION(_) (((_) >> 0) & 0x0F)
	uint64_t reserved1                                               : 9;

	/**
	* @brief BD
	*
	* [Bit 13] When set, this bit indicates that the cause of the debug exception is "debug register access detected."
	*/
	uint64_t debug_register_access_detected                          : 1;
#define VMX_EXIT_QUALIFICATION_DEBUG_EXCEPTION_DEBUG_REGISTER_ACCESS_DETECTED_BIT 13
#define VMX_EXIT_QUALIFICATION_DEBUG_EXCEPTION_DEBUG_REGISTER_ACCESS_DETECTED_FLAG 0x2000
#define VMX_EXIT_QUALIFICATION_DEBUG_EXCEPTION_DEBUG_REGISTER_ACCESS_DETECTED_MASK 0x01
#define VMX_EXIT_QUALIFICATION_DEBUG_EXCEPTION_DEBUG_REGISTER_ACCESS_DETECTED(_) (((_) >> 13) & 0x01)

	/**
	* @brief BS
	*
	* [Bit 14] When set, this bit indicates that the cause of the debug exception is either the execution of a single
	* instruction (if RFLAGS.TF = 1 and IA32_DEBUGCTL.BTF = 0) or a taken branch (if RFLAGS.TF = DEBUGCTL.BTF = 1).
	*/
	uint64_t single_instruction                                      : 1;
#define VMX_EXIT_QUALIFICATION_DEBUG_EXCEPTION_SINGLE_INSTRUCTION_BIT 14
#define VMX_EXIT_QUALIFICATION_DEBUG_EXCEPTION_SINGLE_INSTRUCTION_FLAG 0x4000
#define VMX_EXIT_QUALIFICATION_DEBUG_EXCEPTION_SINGLE_INSTRUCTION_MASK 0x01
#define VMX_EXIT_QUALIFICATION_DEBUG_EXCEPTION_SINGLE_INSTRUCTION(_) (((_) >> 14) & 0x01)
	uint64_t reserved2                                               : 49;
  };

  uint64_t flags;
} vmx_exit_qualification_debug_exception;

/**
 * @brief Exit Qualification for Task Switch
 */
typedef union
{
  struct
  {
	/**
	* [Bits 15:0] Selector of task-state segment (TSS) to which the guest attempted to switch.
	*/
	uint64_t selector                                                : 16;
#define VMX_EXIT_QUALIFICATION_TASK_SWITCH_SELECTOR_BIT              0
#define VMX_EXIT_QUALIFICATION_TASK_SWITCH_SELECTOR_FLAG             0xFFFF
#define VMX_EXIT_QUALIFICATION_TASK_SWITCH_SELECTOR_MASK             0xFFFF
#define VMX_EXIT_QUALIFICATION_TASK_SWITCH_SELECTOR(_)               (((_) >> 0) & 0xFFFF)
	uint64_t reserved1                                               : 14;

	/**
	* [Bits 31:30] Source of task switch initiation.
	*/
	uint64_t source                                                  : 2;
#define VMX_EXIT_QUALIFICATION_TASK_SWITCH_SOURCE_BIT                30
#define VMX_EXIT_QUALIFICATION_TASK_SWITCH_SOURCE_FLAG               0xC0000000
#define VMX_EXIT_QUALIFICATION_TASK_SWITCH_SOURCE_MASK               0x03
#define VMX_EXIT_QUALIFICATION_TASK_SWITCH_SOURCE(_)                 (((_) >> 30) & 0x03)
#define VMX_EXIT_QUALIFICATION_TYPE_CALL_INSTRUCTION                 0x00000000
#define VMX_EXIT_QUALIFICATION_TYPE_IRET_INSTRUCTION                 0x00000001
#define VMX_EXIT_QUALIFICATION_TYPE_JMP_INSTRUCTION                  0x00000002
#define VMX_EXIT_QUALIFICATION_TYPE_TASK_GATE_IN_IDT                 0x00000003
	uint64_t reserved2                                               : 32;
  };

  uint64_t flags;
} vmx_exit_qualification_task_switch;

/**
 * @brief Exit Qualification for Control-Register Accesses
 */
typedef union
{
  struct
  {
	/**
	* [Bits 3:0] Number of control register (0 for CLTS and LMSW). Bit 3 is always 0 on processors that do not support Intel
	* 64 architecture as they do not support CR8.
	*/
	uint64_t control_register                                        : 4;
#define VMX_EXIT_QUALIFICATION_MOV_CR_CONTROL_REGISTER_BIT           0
#define VMX_EXIT_QUALIFICATION_MOV_CR_CONTROL_REGISTER_FLAG          0x0F
#define VMX_EXIT_QUALIFICATION_MOV_CR_CONTROL_REGISTER_MASK          0x0F
#define VMX_EXIT_QUALIFICATION_MOV_CR_CONTROL_REGISTER(_)            (((_) >> 0) & 0x0F)
#define VMX_EXIT_QUALIFICATION_REGISTER_CR0                          0x00000000
#define VMX_EXIT_QUALIFICATION_REGISTER_CR2                          0x00000002
#define VMX_EXIT_QUALIFICATION_REGISTER_CR3                          0x00000003
#define VMX_EXIT_QUALIFICATION_REGISTER_CR4                          0x00000004
#define VMX_EXIT_QUALIFICATION_REGISTER_CR8                          0x00000008

	/**
	* [Bits 5:4] Access type.
	*/
	uint64_t access_type                                             : 2;
#define VMX_EXIT_QUALIFICATION_MOV_CR_ACCESS_TYPE_BIT                4
#define VMX_EXIT_QUALIFICATION_MOV_CR_ACCESS_TYPE_FLAG               0x30
#define VMX_EXIT_QUALIFICATION_MOV_CR_ACCESS_TYPE_MASK               0x03
#define VMX_EXIT_QUALIFICATION_MOV_CR_ACCESS_TYPE(_)                 (((_) >> 4) & 0x03)
#define VMX_EXIT_QUALIFICATION_ACCESS_MOV_TO_CR                      0x00000000
#define VMX_EXIT_QUALIFICATION_ACCESS_MOV_FROM_CR                    0x00000001
#define VMX_EXIT_QUALIFICATION_ACCESS_CLTS                           0x00000002
#define VMX_EXIT_QUALIFICATION_ACCESS_LMSW                           0x00000003

	/**
	* [Bit 6] LMSW operand type. For CLTS and MOV CR, cleared to 0.
	*/
	uint64_t lmsw_operand_type                                       : 1;
#define VMX_EXIT_QUALIFICATION_MOV_CR_LMSW_OPERAND_TYPE_BIT          6
#define VMX_EXIT_QUALIFICATION_MOV_CR_LMSW_OPERAND_TYPE_FLAG         0x40
#define VMX_EXIT_QUALIFICATION_MOV_CR_LMSW_OPERAND_TYPE_MASK         0x01
#define VMX_EXIT_QUALIFICATION_MOV_CR_LMSW_OPERAND_TYPE(_)           (((_) >> 6) & 0x01)
#define VMX_EXIT_QUALIFICATION_LMSW_OP_REGISTER                      0x00000000
#define VMX_EXIT_QUALIFICATION_LMSW_OP_MEMORY                        0x00000001
	uint64_t reserved1                                               : 1;

	/**
	* [Bits 11:8] For MOV CR, the general-purpose register.
	*/
	uint64_t general_purpose_register                                : 4;
#define VMX_EXIT_QUALIFICATION_MOV_CR_GENERAL_PURPOSE_REGISTER_BIT   8
#define VMX_EXIT_QUALIFICATION_MOV_CR_GENERAL_PURPOSE_REGISTER_FLAG  0xF00
#define VMX_EXIT_QUALIFICATION_MOV_CR_GENERAL_PURPOSE_REGISTER_MASK  0x0F
#define VMX_EXIT_QUALIFICATION_MOV_CR_GENERAL_PURPOSE_REGISTER(_)    (((_) >> 8) & 0x0F)
#define VMX_EXIT_QUALIFICATION_GENREG_RAX                            0x00000000
#define VMX_EXIT_QUALIFICATION_GENREG_RCX                            0x00000001
#define VMX_EXIT_QUALIFICATION_GENREG_RDX                            0x00000002
#define VMX_EXIT_QUALIFICATION_GENREG_RBX                            0x00000003
#define VMX_EXIT_QUALIFICATION_GENREG_RSP                            0x00000004
#define VMX_EXIT_QUALIFICATION_GENREG_RBP                            0x00000005
#define VMX_EXIT_QUALIFICATION_GENREG_RSI                            0x00000006
#define VMX_EXIT_QUALIFICATION_GENREG_RDI                            0x00000007
#define VMX_EXIT_QUALIFICATION_GENREG_R8                             0x00000008
#define VMX_EXIT_QUALIFICATION_GENREG_R9                             0x00000009
#define VMX_EXIT_QUALIFICATION_GENREG_R10                            0x0000000A
#define VMX_EXIT_QUALIFICATION_GENREG_R11                            0x0000000B
#define VMX_EXIT_QUALIFICATION_GENREG_R12                            0x0000000C
#define VMX_EXIT_QUALIFICATION_GENREG_R13                            0x0000000D
#define VMX_EXIT_QUALIFICATION_GENREG_R14                            0x0000000E
#define VMX_EXIT_QUALIFICATION_GENREG_R15                            0x0000000F
	uint64_t reserved2                                               : 4;

	/**
	* [Bits 31:16] For LMSW, the LMSW source data. For CLTS and MOV CR, cleared to 0.
	*/
	uint64_t lmsw_source_data                                        : 16;
#define VMX_EXIT_QUALIFICATION_MOV_CR_LMSW_SOURCE_DATA_BIT           16
#define VMX_EXIT_QUALIFICATION_MOV_CR_LMSW_SOURCE_DATA_FLAG          0xFFFF0000
#define VMX_EXIT_QUALIFICATION_MOV_CR_LMSW_SOURCE_DATA_MASK          0xFFFF
#define VMX_EXIT_QUALIFICATION_MOV_CR_LMSW_SOURCE_DATA(_)            (((_) >> 16) & 0xFFFF)
	uint64_t reserved3                                               : 32;
  };

  uint64_t flags;
} vmx_exit_qualification_mov_cr;

/**
 * @brief Exit Qualification for MOV DR
 */
typedef union
{
  struct
  {
	/**
	* [Bits 2:0] Number of debug register.
	*/
	uint64_t debug_register                                          : 3;
#define VMX_EXIT_QUALIFICATION_MOV_DR_DEBUG_REGISTER_BIT             0
#define VMX_EXIT_QUALIFICATION_MOV_DR_DEBUG_REGISTER_FLAG            0x07
#define VMX_EXIT_QUALIFICATION_MOV_DR_DEBUG_REGISTER_MASK            0x07
#define VMX_EXIT_QUALIFICATION_MOV_DR_DEBUG_REGISTER(_)              (((_) >> 0) & 0x07)
#define VMX_EXIT_QUALIFICATION_REGISTER_DR0                          0x00000000
#define VMX_EXIT_QUALIFICATION_REGISTER_DR1                          0x00000001
#define VMX_EXIT_QUALIFICATION_REGISTER_DR2                          0x00000002
#define VMX_EXIT_QUALIFICATION_REGISTER_DR3                          0x00000003
#define VMX_EXIT_QUALIFICATION_REGISTER_DR6                          0x00000006
#define VMX_EXIT_QUALIFICATION_REGISTER_DR7                          0x00000007
	uint64_t reserved1                                               : 1;

	/**
	* [Bit 4] Direction of access (0 = MOV to DR; 1 = MOV from DR).
	*/
	uint64_t direction_of_access                                     : 1;
#define VMX_EXIT_QUALIFICATION_MOV_DR_DIRECTION_OF_ACCESS_BIT        4
#define VMX_EXIT_QUALIFICATION_MOV_DR_DIRECTION_OF_ACCESS_FLAG       0x10
#define VMX_EXIT_QUALIFICATION_MOV_DR_DIRECTION_OF_ACCESS_MASK       0x01
#define VMX_EXIT_QUALIFICATION_MOV_DR_DIRECTION_OF_ACCESS(_)         (((_) >> 4) & 0x01)
#define VMX_EXIT_QUALIFICATION_DIRECTION_MOV_TO_DR                   0x00000000
#define VMX_EXIT_QUALIFICATION_DIRECTION_MOV_FROM_DR                 0x00000001
	uint64_t reserved2                                               : 3;

	/**
	* [Bits 11:8] General-purpose register.
	*/
	uint64_t general_purpose_register                                : 4;
#define VMX_EXIT_QUALIFICATION_MOV_DR_GENERAL_PURPOSE_REGISTER_BIT   8
#define VMX_EXIT_QUALIFICATION_MOV_DR_GENERAL_PURPOSE_REGISTER_FLAG  0xF00
#define VMX_EXIT_QUALIFICATION_MOV_DR_GENERAL_PURPOSE_REGISTER_MASK  0x0F
#define VMX_EXIT_QUALIFICATION_MOV_DR_GENERAL_PURPOSE_REGISTER(_)    (((_) >> 8) & 0x0F)
	uint64_t reserved3                                               : 52;
  };

  uint64_t flags;
} vmx_exit_qualification_mov_dr;

/**
 * @brief Exit Qualification for I/O Instructions
 */
typedef union
{
  struct
  {
	/**
	* [Bits 2:0] Size of access.
	*/
	uint64_t size_of_access                                          : 3;
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_SIZE_OF_ACCESS_BIT     0
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_SIZE_OF_ACCESS_FLAG    0x07
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_SIZE_OF_ACCESS_MASK    0x07
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_SIZE_OF_ACCESS(_)      (((_) >> 0) & 0x07)
#define VMX_EXIT_QUALIFICATION_WIDTH_1_BYTE                          0x00000000
#define VMX_EXIT_QUALIFICATION_WIDTH_2_BYTE                          0x00000001
#define VMX_EXIT_QUALIFICATION_WIDTH_4_BYTE                          0x00000003

	/**
	* [Bit 3] Direction of the attempted access (0 = OUT, 1 = IN).
	*/
	uint64_t direction_of_access                                     : 1;
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_DIRECTION_OF_ACCESS_BIT 3
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_DIRECTION_OF_ACCESS_FLAG 0x08
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_DIRECTION_OF_ACCESS_MASK 0x01
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_DIRECTION_OF_ACCESS(_) (((_) >> 3) & 0x01)
#define VMX_EXIT_QUALIFICATION_DIRECTION_OUT                         0x00000000
#define VMX_EXIT_QUALIFICATION_DIRECTION_IN                          0x00000001

	/**
	* [Bit 4] String instruction (0 = not string; 1 = string).
	*/
	uint64_t string_instruction                                      : 1;
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_STRING_INSTRUCTION_BIT 4
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_STRING_INSTRUCTION_FLAG 0x10
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_STRING_INSTRUCTION_MASK 0x01
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_STRING_INSTRUCTION(_)  (((_) >> 4) & 0x01)
#define VMX_EXIT_QUALIFICATION_IS_STRING_NOT_STRING                  0x00000000
#define VMX_EXIT_QUALIFICATION_IS_STRING_STRING                      0x00000001

	/**
	* [Bit 5] REP prefixed (0 = not REP; 1 = REP).
	*/
	uint64_t rep_prefixed                                            : 1;
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_REP_PREFIXED_BIT       5
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_REP_PREFIXED_FLAG      0x20
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_REP_PREFIXED_MASK      0x01
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_REP_PREFIXED(_)        (((_) >> 5) & 0x01)
#define VMX_EXIT_QUALIFICATION_IS_REP_NOT_REP                        0x00000000
#define VMX_EXIT_QUALIFICATION_IS_REP_REP                            0x00000001

	/**
	* [Bit 6] Operand encoding (0 = DX, 1 = immediate).
	*/
	uint64_t operand_encoding                                        : 1;
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_OPERAND_ENCODING_BIT   6
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_OPERAND_ENCODING_FLAG  0x40
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_OPERAND_ENCODING_MASK  0x01
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_OPERAND_ENCODING(_)    (((_) >> 6) & 0x01)
#define VMX_EXIT_QUALIFICATION_ENCODING_DX                           0x00000000
#define VMX_EXIT_QUALIFICATION_ENCODING_IMMEDIATE                    0x00000001
	uint64_t reserved1                                               : 9;

	/**
	* [Bits 31:16] Port number (as specified in DX or in an immediate operand).
	*/
	uint64_t port_number                                             : 16;
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_PORT_NUMBER_BIT        16
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_PORT_NUMBER_FLAG       0xFFFF0000
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_PORT_NUMBER_MASK       0xFFFF
#define VMX_EXIT_QUALIFICATION_IO_INSTRUCTION_PORT_NUMBER(_)         (((_) >> 16) & 0xFFFF)
	uint64_t reserved2                                               : 32;
  };

  uint64_t flags;
} vmx_exit_qualification_io_instruction;

/**
 * @brief Exit Qualification for APIC-Access VM Exits from Linear Accesses and Guest-Physical Accesses
 */
typedef union
{
  struct
  {
	/**
	* [Bits 11:0] - If the APIC-access VM exit is due to a linear access, the offset of access within the APIC page.
	* - Undefined if the APIC-access VM exit is due a guest-physical access.
	*/
	uint64_t page_offset                                             : 12;
#define VMX_EXIT_QUALIFICATION_APIC_ACCESS_PAGE_OFFSET_BIT           0
#define VMX_EXIT_QUALIFICATION_APIC_ACCESS_PAGE_OFFSET_FLAG          0xFFF
#define VMX_EXIT_QUALIFICATION_APIC_ACCESS_PAGE_OFFSET_MASK          0xFFF
#define VMX_EXIT_QUALIFICATION_APIC_ACCESS_PAGE_OFFSET(_)            (((_) >> 0) & 0xFFF)

	/**
	* [Bits 15:12] Access type.
	*/
	uint64_t access_type                                             : 4;
#define VMX_EXIT_QUALIFICATION_APIC_ACCESS_ACCESS_TYPE_BIT           12
#define VMX_EXIT_QUALIFICATION_APIC_ACCESS_ACCESS_TYPE_FLAG          0xF000
#define VMX_EXIT_QUALIFICATION_APIC_ACCESS_ACCESS_TYPE_MASK          0x0F
#define VMX_EXIT_QUALIFICATION_APIC_ACCESS_ACCESS_TYPE(_)            (((_) >> 12) & 0x0F)
	/**
	* Linear access for a data read during instruction execution.
	*/
#define VMX_EXIT_QUALIFICATION_TYPE_LINEAR_READ                      0x00000000

	/**
	* Linear access for a data write during instruction execution.
	*/
#define VMX_EXIT_QUALIFICATION_TYPE_LINEAR_WRITE                     0x00000001

	/**
	* Linear access for an instruction fetch.
	*/
#define VMX_EXIT_QUALIFICATION_TYPE_LINEAR_INSTRUCTION_FETCH         0x00000002

	/**
	* Linear access (read or write) during event delivery.
	*/
#define VMX_EXIT_QUALIFICATION_TYPE_LINEAR_EVENT_DELIVERY            0x00000003

	/**
	* Guest-physical access during event delivery.
	*/
#define VMX_EXIT_QUALIFICATION_TYPE_PHYSICAL_EVENT_DELIVERY          0x0000000A

	/**
	* Guest-physical access for an instruction fetch or during instruction execution.
	*/
#define VMX_EXIT_QUALIFICATION_TYPE_PHYSICAL_INSTRUCTION_FETCH       0x0000000F
	uint64_t reserved1                                               : 48;
  };

  uint64_t flags;
} vmx_exit_qualification_apic_access;

/**
 * @brief Exit Qualification for EPT Violations
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] Set if the access causing the EPT violation was a data read.
	*/
	uint64_t read_access                                             : 1;
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_READ_ACCESS_BIT         0
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_READ_ACCESS_FLAG        0x01
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_READ_ACCESS_MASK        0x01
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_READ_ACCESS(_)          (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Set if the access causing the EPT violation was a data write.
	*/
	uint64_t write_access                                            : 1;
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_WRITE_ACCESS_BIT        1
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_WRITE_ACCESS_FLAG       0x02
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_WRITE_ACCESS_MASK       0x01
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_WRITE_ACCESS(_)         (((_) >> 1) & 0x01)

	/**
	* [Bit 2] Set if the access causing the EPT violation was an instruction fetch.
	*/
	uint64_t execute_access                                          : 1;
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EXECUTE_ACCESS_BIT      2
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EXECUTE_ACCESS_FLAG     0x04
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EXECUTE_ACCESS_MASK     0x01
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EXECUTE_ACCESS(_)       (((_) >> 2) & 0x01)

	/**
	* [Bit 3] The logical-AND of bit 0 in the EPT paging-structure entries used to translate the guest-physical address of the
	* access causing the EPT violation (indicates whether the guest-physical address was readable).
	*/
	uint64_t ept_readable                                            : 1;
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EPT_READABLE_BIT        3
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EPT_READABLE_FLAG       0x08
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EPT_READABLE_MASK       0x01
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EPT_READABLE(_)         (((_) >> 3) & 0x01)

	/**
	* [Bit 4] The logical-AND of bit 1 in the EPT paging-structure entries used to translate the guest-physical address of the
	* access causing the EPT violation (indicates whether the guest-physical address was writeable).
	*/
	uint64_t ept_writeable                                           : 1;
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EPT_WRITEABLE_BIT       4
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EPT_WRITEABLE_FLAG      0x10
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EPT_WRITEABLE_MASK      0x01
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EPT_WRITEABLE(_)        (((_) >> 4) & 0x01)

	/**
	* [Bit 5] The logical-AND of bit 2 in the EPT paging-structure entries used to translate the guest-physical address of the
	* access causing the EPT violation.
	* If the "mode-based execute control for EPT" VM-execution control is 0, this indicates whether the guest-physical address
	* was executable. If that control is 1, this indicates whether the guest-physical address was executable for
	* supervisor-mode linear addresses.
	*/
	uint64_t ept_executable                                          : 1;
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EPT_EXECUTABLE_BIT      5
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EPT_EXECUTABLE_FLAG     0x20
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EPT_EXECUTABLE_MASK     0x01
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EPT_EXECUTABLE(_)       (((_) >> 5) & 0x01)

	/**
	* [Bit 6] If the "mode-based execute control" VM-execution control is 0, the value of this bit is undefined. If that
	* control is 1, this bit is the logical-AND of bit 10 in the EPT paging-structures entries used to translate the
	* guest-physical address of the access causing the EPT violation. In this case, it indicates whether the guest-physical
	* address was executable for user-mode linear addresses.
	*/
	uint64_t ept_executable_for_user_mode                            : 1;
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EPT_EXECUTABLE_FOR_USER_MODE_BIT 6
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EPT_EXECUTABLE_FOR_USER_MODE_FLAG 0x40
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EPT_EXECUTABLE_FOR_USER_MODE_MASK 0x01
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EPT_EXECUTABLE_FOR_USER_MODE(_) (((_) >> 6) & 0x01)

	/**
	* [Bit 7] Set if the guest linear-address field is valid. The guest linear-address field is valid for all EPT violations
	* except those resulting from an attempt to load the guest PDPTEs as part of the execution of the MOV CR instruction.
	*/
	uint64_t valid_guest_linear_address                              : 1;
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_VALID_GUEST_LINEAR_ADDRESS_BIT 7
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_VALID_GUEST_LINEAR_ADDRESS_FLAG 0x80
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_VALID_GUEST_LINEAR_ADDRESS_MASK 0x01
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_VALID_GUEST_LINEAR_ADDRESS(_) (((_) >> 7) & 0x01)

	/**
	* [Bit 8] If bit 7 is 1:
	* - Set if the access causing the EPT violation is to a guest-physical address that is the translation of a linear
	* address.
	* - Clear if the access causing the EPT violation is to a paging-structure entry as part of a page walk or the update of
	* an accessed or dirty bit.
	* Reserved if bit 7 is 0 (cleared to 0).
	*/
	uint64_t caused_by_translation                                   : 1;
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_CAUSED_BY_TRANSLATION_BIT 8
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_CAUSED_BY_TRANSLATION_FLAG 0x100
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_CAUSED_BY_TRANSLATION_MASK 0x01
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_CAUSED_BY_TRANSLATION(_) (((_) >> 8) & 0x01)

	/**
	* [Bit 9] This bit is 0 if the linear address is a supervisor-mode linear address and 1 if it is a user-mode linear
	* address. Otherwise, this bit is undefined.
	*
	* @remarks If bit 7 is 1, bit 8 is 1, and the processor supports advanced VM-exit information for EPT violations. (If
	*          CR0.PG = 0, the translation of every linear address is a user-mode linear address and thus this bit will be 1.)
	*/
	uint64_t user_mode_linear_address                                : 1;
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_USER_MODE_LINEAR_ADDRESS_BIT 9
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_USER_MODE_LINEAR_ADDRESS_FLAG 0x200
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_USER_MODE_LINEAR_ADDRESS_MASK 0x01
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_USER_MODE_LINEAR_ADDRESS(_) (((_) >> 9) & 0x01)

	/**
	* [Bit 10] This bit is 0 if paging translates the linear address to a read-only page and 1 if it translates to a
	* read/write page. Otherwise, this bit is undefined
	*
	* @remarks If bit 7 is 1, bit 8 is 1, and the processor supports advanced VM-exit information for EPT violations. (If
	*          CR0.PG = 0, every linear address is read/write and thus this bit will be 1.)
	*/
	uint64_t readable_writable_page                                  : 1;
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_READABLE_WRITABLE_PAGE_BIT 10
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_READABLE_WRITABLE_PAGE_FLAG 0x400
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_READABLE_WRITABLE_PAGE_MASK 0x01
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_READABLE_WRITABLE_PAGE(_) (((_) >> 10) & 0x01)

	/**
	* [Bit 11] This bit is 0 if paging translates the linear address to an executable page and 1 if it translates to an
	* execute-disable page. Otherwise, this bit is undefined.
	*
	* @remarks If bit 7 is 1, bit 8 is 1, and the processor supports advanced VM-exit information for EPT violations. (If
	*          CR0.PG = 0, CR4.PAE = 0, or IA32_EFER.NXE = 0, every linear address is executable and thus this bit will be 0.)
	*/
	uint64_t execute_disable_page                                    : 1;
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EXECUTE_DISABLE_PAGE_BIT 11
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EXECUTE_DISABLE_PAGE_FLAG 0x800
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EXECUTE_DISABLE_PAGE_MASK 0x01
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_EXECUTE_DISABLE_PAGE(_) (((_) >> 11) & 0x01)

	/**
	* [Bit 12] NMI unblocking due to IRET.
	*/
	uint64_t nmi_unblocking                                          : 1;
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_NMI_UNBLOCKING_BIT      12
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_NMI_UNBLOCKING_FLAG     0x1000
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_NMI_UNBLOCKING_MASK     0x01
#define VMX_EXIT_QUALIFICATION_EPT_VIOLATION_NMI_UNBLOCKING(_)       (((_) >> 12) & 0x01)
	uint64_t reserved1                                               : 51;
  };

  uint64_t flags;
} vmx_exit_qualification_ept_violation;

/**
 * @}
 */

/**
 * @defgroup vmx_vmexit_instruction_information \
 *           Information for VM Exits Due to Instruction Execution
 *
 * Information for VM Exits Due to Instruction Execution.
 *
 * @see Vol3C[27.2.4(Information for VM Exits Due to Instruction Execution)] (reference)
 * @{
 */
/**
 * @brief VM-Exit Instruction-Information Field as Used for INS and OUTS
 */
typedef union
{
  struct
  {
	uint64_t reserved1                                               : 7;

	/**
	* @brief Address size
	*
	* [Bits 9:7] 0: 16-bit
	* 1: 32-bit
	* 2: 64-bit (used only on processors that support Intel 64 architecture)
	* Other values not used.
	*/
	uint64_t address_size                                            : 3;
#define VMX_VMEXIT_INSTRUCTION_INFO_INS_OUTS_ADDRESS_SIZE_BIT        7
#define VMX_VMEXIT_INSTRUCTION_INFO_INS_OUTS_ADDRESS_SIZE_FLAG       0x380
#define VMX_VMEXIT_INSTRUCTION_INFO_INS_OUTS_ADDRESS_SIZE_MASK       0x07
#define VMX_VMEXIT_INSTRUCTION_INFO_INS_OUTS_ADDRESS_SIZE(_)         (((_) >> 7) & 0x07)
	uint64_t reserved2                                               : 5;

	/**
	* @brief Segment register
	*
	* [Bits 17:15] 0: ES
	* 1: CS
	* 2: SS
	* 3: DS
	* 4: FS
	* 5: GS
	* Other values not used. Undefined for VM exits due to execution of INS.
	*/
	uint64_t segment_register                                        : 3;
#define VMX_VMEXIT_INSTRUCTION_INFO_INS_OUTS_SEGMENT_REGISTER_BIT    15
#define VMX_VMEXIT_INSTRUCTION_INFO_INS_OUTS_SEGMENT_REGISTER_FLAG   0x38000
#define VMX_VMEXIT_INSTRUCTION_INFO_INS_OUTS_SEGMENT_REGISTER_MASK   0x07
#define VMX_VMEXIT_INSTRUCTION_INFO_INS_OUTS_SEGMENT_REGISTER(_)     (((_) >> 15) & 0x07)
	uint64_t reserved3                                               : 46;
  };

  uint64_t flags;
} vmx_vmexit_instruction_info_ins_outs;

/**
 * @brief VM-Exit Instruction-Information Field as Used for INVEPT, INVPCID, and INVVPID
 */
typedef union
{
  struct
  {
	/**
	* @brief Scaling
	*
	* [Bits 1:0] 0: no scaling
	* 1: scale by 2
	* 2: scale by 4
	* 3: scale by 8 (used only on processors that support Intel 64 architecture)
	* Undefined for instructions with no index register (bit 22 is set).
	*/
	uint64_t scaling                                                 : 2;
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_SCALING_BIT           0
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_SCALING_FLAG          0x03
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_SCALING_MASK          0x03
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_SCALING(_)            (((_) >> 0) & 0x03)
	uint64_t reserved1                                               : 5;

	/**
	* @brief Address size
	*
	* [Bits 9:7] 0: 16-bit
	* 1: 32-bit
	* 2: 64-bit (used only on processors that support Intel 64 architecture)
	* Other values not used.
	*/
	uint64_t address_size                                            : 3;
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_ADDRESS_SIZE_BIT      7
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_ADDRESS_SIZE_FLAG     0x380
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_ADDRESS_SIZE_MASK     0x07
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_ADDRESS_SIZE(_)       (((_) >> 7) & 0x07)
	uint64_t reserved2                                               : 5;

	/**
	* @brief Segment register
	*
	* [Bits 17:15] 0: ES
	* 1: CS
	* 2: SS
	* 3: DS
	* 4: FS
	* 5: GS
	* Other values not used. Undefined for VM exits due to execution of INS.
	*/
	uint64_t segment_register                                        : 3;
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_SEGMENT_REGISTER_BIT  15
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_SEGMENT_REGISTER_FLAG 0x38000
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_SEGMENT_REGISTER_MASK 0x07
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_SEGMENT_REGISTER(_)   (((_) >> 15) & 0x07)

	/**
	* [Bits 21:18] General-purpose register. Undefined for instructions with no index register (bit 22 is set).
	*/
	uint64_t general_purpose_register                                : 4;
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_GENERAL_PURPOSE_REGISTER_BIT 18
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_GENERAL_PURPOSE_REGISTER_FLAG 0x3C0000
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_GENERAL_PURPOSE_REGISTER_MASK 0x0F
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_GENERAL_PURPOSE_REGISTER(_) (((_) >> 18) & 0x0F)

	/**
	* [Bit 22] IndexReg invalid (0 = valid; 1 = invalid).
	*/
	uint64_t general_purpose_register_invalid                        : 1;
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_GENERAL_PURPOSE_REGISTER_INVALID_BIT 22
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_GENERAL_PURPOSE_REGISTER_INVALID_FLAG 0x400000
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_GENERAL_PURPOSE_REGISTER_INVALID_MASK 0x01
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_GENERAL_PURPOSE_REGISTER_INVALID(_) (((_) >> 22) & 0x01)

	/**
	* [Bits 26:23] BaseReg (encoded as IndexReg above). Undefined for memory instructions with no base register (bit 27 is
	* set).
	*/
	uint64_t base_register                                           : 4;
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_BASE_REGISTER_BIT     23
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_BASE_REGISTER_FLAG    0x7800000
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_BASE_REGISTER_MASK    0x0F
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_BASE_REGISTER(_)      (((_) >> 23) & 0x0F)

	/**
	* [Bit 27] BaseReg invalid (0 = valid; 1 = invalid).
	*/
	uint64_t base_register_invalid                                   : 1;
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_BASE_REGISTER_INVALID_BIT 27
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_BASE_REGISTER_INVALID_FLAG 0x8000000
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_BASE_REGISTER_INVALID_MASK 0x01
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_BASE_REGISTER_INVALID(_) (((_) >> 27) & 0x01)

	/**
	* [Bits 31:28] Reg2 (same encoding as IndexReg above).
	*/
	uint64_t register_2                                              : 4;
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_REGISTER_2_BIT        28
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_REGISTER_2_FLAG       0xF0000000
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_REGISTER_2_MASK       0x0F
#define VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE_REGISTER_2(_)         (((_) >> 28) & 0x0F)
	uint64_t reserved3                                               : 32;
  };

  uint64_t flags;
} vmx_vmexit_instruction_info_invalidate;

/**
 * @brief VM-Exit Instruction-Information Field as Used for LIDT, LGDT, SIDT, or SGDT
 */
typedef union
{
  struct
  {
	/**
	* @brief Scaling
	*
	* [Bits 1:0] 0: no scaling
	* 1: scale by 2
	* 2: scale by 4
	* 3: scale by 8 (used only on processors that support Intel 64 architecture)
	* Undefined for instructions with no index register (bit 22 is set).
	*/
	uint64_t scaling                                                 : 2;
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_SCALING_BIT     0
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_SCALING_FLAG    0x03
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_SCALING_MASK    0x03
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_SCALING(_)      (((_) >> 0) & 0x03)
	uint64_t reserved1                                               : 5;

	/**
	* @brief Address size
	*
	* [Bits 9:7] 0: 16-bit
	* 1: 32-bit
	* 2: 64-bit (used only on processors that support Intel 64 architecture)
	* Other values not used.
	*/
	uint64_t address_size                                            : 3;
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_ADDRESS_SIZE_BIT 7
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_ADDRESS_SIZE_FLAG 0x380
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_ADDRESS_SIZE_MASK 0x07
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_ADDRESS_SIZE(_) (((_) >> 7) & 0x07)
	uint64_t reserved2                                               : 1;

	/**
	* @brief Operand size
	*
	* [Bit 11] 0: 16-bit
	* 1: 32-bit
	* Undefined for VM exits from 64-bit mode.
	*/
	uint64_t operand_size                                            : 1;
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_OPERAND_SIZE_BIT 11
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_OPERAND_SIZE_FLAG 0x800
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_OPERAND_SIZE_MASK 0x01
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_OPERAND_SIZE(_) (((_) >> 11) & 0x01)
	uint64_t reserved3                                               : 3;

	/**
	* @brief Segment register
	*
	* [Bits 17:15] 0: ES
	* 1: CS
	* 2: SS
	* 3: DS
	* 4: FS
	* 5: GS
	* Other values not used.
	*/
	uint64_t segment_register                                        : 3;
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_SEGMENT_REGISTER_BIT 15
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_SEGMENT_REGISTER_FLAG 0x38000
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_SEGMENT_REGISTER_MASK 0x07
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_SEGMENT_REGISTER(_) (((_) >> 15) & 0x07)

	/**
	* [Bits 21:18] General-purpose register. Undefined for instructions with no index register (bit 22 is set).
	*/
	uint64_t general_purpose_register                                : 4;
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_GENERAL_PURPOSE_REGISTER_BIT 18
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_GENERAL_PURPOSE_REGISTER_FLAG 0x3C0000
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_GENERAL_PURPOSE_REGISTER_MASK 0x0F
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_GENERAL_PURPOSE_REGISTER(_) (((_) >> 18) & 0x0F)

	/**
	* [Bit 22] IndexReg invalid (0 = valid; 1 = invalid).
	*/
	uint64_t general_purpose_register_invalid                        : 1;
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_GENERAL_PURPOSE_REGISTER_INVALID_BIT 22
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_GENERAL_PURPOSE_REGISTER_INVALID_FLAG 0x400000
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_GENERAL_PURPOSE_REGISTER_INVALID_MASK 0x01
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_GENERAL_PURPOSE_REGISTER_INVALID(_) (((_) >> 22) & 0x01)

	/**
	* [Bits 26:23] BaseReg (encoded as IndexReg above). Undefined for memory instructions with no base register (bit 27 is
	* set).
	*/
	uint64_t base_register                                           : 4;
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_BASE_REGISTER_BIT 23
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_BASE_REGISTER_FLAG 0x7800000
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_BASE_REGISTER_MASK 0x0F
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_BASE_REGISTER(_) (((_) >> 23) & 0x0F)

	/**
	* [Bit 27] BaseReg invalid (0 = valid; 1 = invalid).
	*/
	uint64_t base_register_invalid                                   : 1;
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_BASE_REGISTER_INVALID_BIT 27
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_BASE_REGISTER_INVALID_FLAG 0x8000000
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_BASE_REGISTER_INVALID_MASK 0x01
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_BASE_REGISTER_INVALID(_) (((_) >> 27) & 0x01)

	/**
	* @brief Instruction identity
	*
	* [Bits 29:28] 0: SGDT
	* 1: SIDT
	* 2: LGDT
	* 3: LIDT
	*/
	uint64_t instruction                                             : 2;
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_INSTRUCTION_BIT 28
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_INSTRUCTION_FLAG 0x30000000
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_INSTRUCTION_MASK 0x03
#define VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS_INSTRUCTION(_)  (((_) >> 28) & 0x03)
	uint64_t reserved4                                               : 34;
  };

  uint64_t flags;
} vmx_vmexit_instruction_info_gdtr_idtr_access;

/**
 * @brief VM-Exit Instruction-Information Field as Used for LLDT, LTR, SLDT, and STR
 */
typedef union
{
  struct
  {
	/**
	* @brief Scaling
	*
	* [Bits 1:0] 0: no scaling
	* 1: scale by 2
	* 2: scale by 4
	* 3: scale by 8 (used only on processors that support Intel 64 architecture)
	* Undefined for instructions with no index register (bit 22 is set).
	*/
	uint64_t scaling                                                 : 2;
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_SCALING_BIT       0
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_SCALING_FLAG      0x03
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_SCALING_MASK      0x03
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_SCALING(_)        (((_) >> 0) & 0x03)
	uint64_t reserved1                                               : 1;

	/**
	* [Bits 6:3] Reg1. Undefined for memory instructions (bit 10 is clear).
	*/
	uint64_t reg_1                                                   : 4;
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_REG_1_BIT         3
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_REG_1_FLAG        0x78
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_REG_1_MASK        0x0F
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_REG_1(_)          (((_) >> 3) & 0x0F)

	/**
	* @brief Address size
	*
	* [Bits 9:7] 0: 16-bit
	* 1: 32-bit
	* 2: 64-bit (used only on processors that support Intel 64 architecture)
	* Other values not used. Undefined for register instructions (bit 10 is set).
	*/
	uint64_t address_size                                            : 3;
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_ADDRESS_SIZE_BIT  7
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_ADDRESS_SIZE_FLAG 0x380
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_ADDRESS_SIZE_MASK 0x07
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_ADDRESS_SIZE(_)   (((_) >> 7) & 0x07)

	/**
	* [Bit 10] Mem/Reg (0 = memory; 1 = register).
	*/
	uint64_t memory_register                                         : 1;
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_MEMORY_REGISTER_BIT 10
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_MEMORY_REGISTER_FLAG 0x400
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_MEMORY_REGISTER_MASK 0x01
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_MEMORY_REGISTER(_) (((_) >> 10) & 0x01)
	uint64_t reserved2                                               : 4;

	/**
	* @brief Segment register
	*
	* [Bits 17:15] 0: ES
	* 1: CS
	* 2: SS
	* 3: DS
	* 4: FS
	* 5: GS
	* Other values not used. Undefined for register instructions (bit 10 is set).
	*/
	uint64_t segment_register                                        : 3;
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_SEGMENT_REGISTER_BIT 15
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_SEGMENT_REGISTER_FLAG 0x38000
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_SEGMENT_REGISTER_MASK 0x07
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_SEGMENT_REGISTER(_) (((_) >> 15) & 0x07)

	/**
	* [Bits 21:18] General-purpose register. Undefined for register instructions (bit 10 is set) and for memory instructions
	* with no index register (bit 10 is clear and bit 22 is set).
	*/
	uint64_t general_purpose_register                                : 4;
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_GENERAL_PURPOSE_REGISTER_BIT 18
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_GENERAL_PURPOSE_REGISTER_FLAG 0x3C0000
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_GENERAL_PURPOSE_REGISTER_MASK 0x0F
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_GENERAL_PURPOSE_REGISTER(_) (((_) >> 18) & 0x0F)

	/**
	* [Bit 22] IndexReg invalid (0 = valid; 1 = invalid). Undefined for register instructions (bit 10 is set).
	*/
	uint64_t general_purpose_register_invalid                        : 1;
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_GENERAL_PURPOSE_REGISTER_INVALID_BIT 22
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_GENERAL_PURPOSE_REGISTER_INVALID_FLAG 0x400000
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_GENERAL_PURPOSE_REGISTER_INVALID_MASK 0x01
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_GENERAL_PURPOSE_REGISTER_INVALID(_) (((_) >> 22) & 0x01)

	/**
	* [Bits 26:23] BaseReg (encoded as IndexReg above). Undefined for register instructions (bit 10 is set) and for memory
	* instructions with no base register (bit 10 is clear and bit 27 is set).
	*/
	uint64_t base_register                                           : 4;
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_BASE_REGISTER_BIT 23
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_BASE_REGISTER_FLAG 0x7800000
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_BASE_REGISTER_MASK 0x0F
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_BASE_REGISTER(_)  (((_) >> 23) & 0x0F)

	/**
	* [Bit 27] BaseReg invalid (0 = valid; 1 = invalid). Undefined for register instructions (bit 10 is set).
	*/
	uint64_t base_register_invalid                                   : 1;
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_BASE_REGISTER_INVALID_BIT 27
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_BASE_REGISTER_INVALID_FLAG 0x8000000
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_BASE_REGISTER_INVALID_MASK 0x01
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_BASE_REGISTER_INVALID(_) (((_) >> 27) & 0x01)

	/**
	* @brief Instruction identity
	*
	* [Bits 29:28] 0: SLDT
	* 1: STR
	* 2: LLDT
	* 3: LTR
	*/
	uint64_t instruction                                             : 2;
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_INSTRUCTION_BIT   28
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_INSTRUCTION_FLAG  0x30000000
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_INSTRUCTION_MASK  0x03
#define VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS_INSTRUCTION(_)    (((_) >> 28) & 0x03)
	uint64_t reserved3                                               : 34;
  };

  uint64_t flags;
} vmx_vmexit_instruction_info_ldtr_tr_access;

/**
 * @brief VM-Exit Instruction-Information Field as Used for RDRAND and RDSEED
 */
typedef union
{
  struct
  {
	uint64_t reserved1                                               : 3;

	/**
	* [Bits 6:3] Destination register.
	*/
	uint64_t destination_register                                    : 4;
#define VMX_VMEXIT_INSTRUCTION_INFO_RDRAND_RDSEED_DESTINATION_REGISTER_BIT 3
#define VMX_VMEXIT_INSTRUCTION_INFO_RDRAND_RDSEED_DESTINATION_REGISTER_FLAG 0x78
#define VMX_VMEXIT_INSTRUCTION_INFO_RDRAND_RDSEED_DESTINATION_REGISTER_MASK 0x0F
#define VMX_VMEXIT_INSTRUCTION_INFO_RDRAND_RDSEED_DESTINATION_REGISTER(_) (((_) >> 3) & 0x0F)
	uint64_t reserved2                                               : 4;

	/**
	* @brief Operand size
	*
	* [Bits 12:11] 0: 16-bit
	* 1: 32-bit
	* 2: 64-bit
	* The value 3 is not used.
	*/
	uint64_t operand_size                                            : 2;
#define VMX_VMEXIT_INSTRUCTION_INFO_RDRAND_RDSEED_OPERAND_SIZE_BIT   11
#define VMX_VMEXIT_INSTRUCTION_INFO_RDRAND_RDSEED_OPERAND_SIZE_FLAG  0x1800
#define VMX_VMEXIT_INSTRUCTION_INFO_RDRAND_RDSEED_OPERAND_SIZE_MASK  0x03
#define VMX_VMEXIT_INSTRUCTION_INFO_RDRAND_RDSEED_OPERAND_SIZE(_)    (((_) >> 11) & 0x03)
	uint64_t reserved3                                               : 51;
  };

  uint64_t flags;
} vmx_vmexit_instruction_info_rdrand_rdseed;

/**
 * @brief VM-Exit Instruction-Information Field as Used for VMCLEAR, VMPTRLD, VMPTRST, VMXON, XRSTORS, and XSAVES
 */
typedef union
{
  struct
  {
	/**
	* @brief Scaling
	*
	* [Bits 1:0] 0: no scaling
	* 1: scale by 2
	* 2: scale by 4
	* 3: scale by 8 (used only on processors that support Intel 64 architecture)
	* Undefined for instructions with no index register (bit 22 is set).
	*/
	uint64_t scaling                                                 : 2;
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_SCALING_BIT       0
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_SCALING_FLAG      0x03
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_SCALING_MASK      0x03
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_SCALING(_)        (((_) >> 0) & 0x03)
	uint64_t reserved1                                               : 5;

	/**
	* @brief Address size
	*
	* [Bits 9:7] 0: 16-bit
	* 1: 32-bit
	* 2: 64-bit (used only on processors that support Intel 64 architecture)
	* Other values not used.
	*/
	uint64_t address_size                                            : 3;
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_ADDRESS_SIZE_BIT  7
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_ADDRESS_SIZE_FLAG 0x380
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_ADDRESS_SIZE_MASK 0x07
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_ADDRESS_SIZE(_)   (((_) >> 7) & 0x07)
	uint64_t reserved2                                               : 5;

	/**
	* @brief Segment register
	*
	* [Bits 17:15] 0: ES
	* 1: CS
	* 2: SS
	* 3: DS
	* 4: FS
	* 5: GS
	* Other values not used.
	*/
	uint64_t segment_register                                        : 3;
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_SEGMENT_REGISTER_BIT 15
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_SEGMENT_REGISTER_FLAG 0x38000
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_SEGMENT_REGISTER_MASK 0x07
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_SEGMENT_REGISTER(_) (((_) >> 15) & 0x07)

	/**
	* [Bits 21:18] General-purpose register. Undefined for instructions with no index register (bit 22 is set).
	*/
	uint64_t general_purpose_register                                : 4;
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_GENERAL_PURPOSE_REGISTER_BIT 18
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_GENERAL_PURPOSE_REGISTER_FLAG 0x3C0000
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_GENERAL_PURPOSE_REGISTER_MASK 0x0F
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_GENERAL_PURPOSE_REGISTER(_) (((_) >> 18) & 0x0F)

	/**
	* [Bit 22] IndexReg invalid (0 = valid; 1 = invalid).
	*/
	uint64_t general_purpose_register_invalid                        : 1;
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_GENERAL_PURPOSE_REGISTER_INVALID_BIT 22
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_GENERAL_PURPOSE_REGISTER_INVALID_FLAG 0x400000
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_GENERAL_PURPOSE_REGISTER_INVALID_MASK 0x01
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_GENERAL_PURPOSE_REGISTER_INVALID(_) (((_) >> 22) & 0x01)

	/**
	* [Bits 26:23] BaseReg (encoded as IndexReg above). Undefined for memory instructions with no base register (bit 27 is
	* set).
	*/
	uint64_t base_register                                           : 4;
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_BASE_REGISTER_BIT 23
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_BASE_REGISTER_FLAG 0x7800000
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_BASE_REGISTER_MASK 0x0F
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_BASE_REGISTER(_)  (((_) >> 23) & 0x0F)

	/**
	* [Bit 27] BaseReg invalid (0 = valid; 1 = invalid).
	*/
	uint64_t base_register_invalid                                   : 1;
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_BASE_REGISTER_INVALID_BIT 27
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_BASE_REGISTER_INVALID_FLAG 0x8000000
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_BASE_REGISTER_INVALID_MASK 0x01
#define VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES_BASE_REGISTER_INVALID(_) (((_) >> 27) & 0x01)
	uint64_t reserved3                                               : 36;
  };

  uint64_t flags;
} vmx_vmexit_instruction_info_vmx_and_xsaves;

/**
 * @brief VM-Exit Instruction-Information Field as Used for VMREAD and VMWRITE
 */
typedef union
{
  struct
  {
	/**
	* @brief Scaling
	*
	* [Bits 1:0] 0: no scaling
	* 1: scale by 2
	* 2: scale by 4
	* 3: scale by 8 (used only on processors that support Intel 64 architecture)
	* Undefined for register instructions (bit 10 is set) and for memory instructions with no index register (bit 10 is clear
	* and bit 22 is set).
	*/
	uint64_t scaling                                                 : 2;
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_SCALING_BIT       0
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_SCALING_FLAG      0x03
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_SCALING_MASK      0x03
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_SCALING(_)        (((_) >> 0) & 0x03)
	uint64_t reserved1                                               : 1;

	/**
	* [Bits 6:3] Reg1. Undefined for memory instructions (bit 10 is clear).
	*/
	uint64_t register_1                                              : 4;
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_REGISTER_1_BIT    3
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_REGISTER_1_FLAG   0x78
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_REGISTER_1_MASK   0x0F
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_REGISTER_1(_)     (((_) >> 3) & 0x0F)

	/**
	* @brief Address size
	*
	* [Bits 9:7] 0: 16-bit
	* 1: 32-bit
	* 2: 64-bit (used only on processors that support Intel 64 architecture)
	* Other values not used. Undefined for register instructions (bit 10 is set).
	*/
	uint64_t address_size                                            : 3;
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_ADDRESS_SIZE_BIT  7
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_ADDRESS_SIZE_FLAG 0x380
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_ADDRESS_SIZE_MASK 0x07
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_ADDRESS_SIZE(_)   (((_) >> 7) & 0x07)

	/**
	* [Bit 10] Mem/Reg (0 = memory; 1 = register).
	*/
	uint64_t memory_register                                         : 1;
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_MEMORY_REGISTER_BIT 10
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_MEMORY_REGISTER_FLAG 0x400
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_MEMORY_REGISTER_MASK 0x01
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_MEMORY_REGISTER(_) (((_) >> 10) & 0x01)
	uint64_t reserved2                                               : 4;

	/**
	* @brief Segment register
	*
	* [Bits 17:15] 0: ES
	* 1: CS
	* 2: SS
	* 3: DS
	* 4: FS
	* 5: GS
	* Other values not used. Undefined for register instructions (bit 10 is set).
	*/
	uint64_t segment_register                                        : 3;
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_SEGMENT_REGISTER_BIT 15
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_SEGMENT_REGISTER_FLAG 0x38000
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_SEGMENT_REGISTER_MASK 0x07
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_SEGMENT_REGISTER(_) (((_) >> 15) & 0x07)

	/**
	* [Bits 21:18] General-purpose register. Undefined for register instructions (bit 10 is set) and for memory instructions
	* with no index register (bit 10 is clear and bit 22 is set).
	*/
	uint64_t general_purpose_register                                : 4;
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_GENERAL_PURPOSE_REGISTER_BIT 18
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_GENERAL_PURPOSE_REGISTER_FLAG 0x3C0000
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_GENERAL_PURPOSE_REGISTER_MASK 0x0F
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_GENERAL_PURPOSE_REGISTER(_) (((_) >> 18) & 0x0F)

	/**
	* [Bit 22] IndexReg invalid (0 = valid; 1 = invalid). Undefined for register instructions (bit 10 is set).
	*/
	uint64_t general_purpose_register_invalid                        : 1;
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_GENERAL_PURPOSE_REGISTER_INVALID_BIT 22
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_GENERAL_PURPOSE_REGISTER_INVALID_FLAG 0x400000
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_GENERAL_PURPOSE_REGISTER_INVALID_MASK 0x01
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_GENERAL_PURPOSE_REGISTER_INVALID(_) (((_) >> 22) & 0x01)

	/**
	* [Bits 26:23] BaseReg (encoded as Reg1 above). Undefined for register instructions (bit 10 is set) and for memory
	* instructions with no base register (bit 10 is clear and bit 27 is set).
	*/
	uint64_t base_register                                           : 4;
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_BASE_REGISTER_BIT 23
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_BASE_REGISTER_FLAG 0x7800000
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_BASE_REGISTER_MASK 0x0F
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_BASE_REGISTER(_)  (((_) >> 23) & 0x0F)

	/**
	* [Bit 27] BaseReg invalid (0 = valid; 1 = invalid).
	*/
	uint64_t base_register_invalid                                   : 1;
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_BASE_REGISTER_INVALID_BIT 27
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_BASE_REGISTER_INVALID_FLAG 0x8000000
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_BASE_REGISTER_INVALID_MASK 0x01
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_BASE_REGISTER_INVALID(_) (((_) >> 27) & 0x01)

	/**
	* [Bits 31:28] Reg2 (same encoding as IndexReg above).
	*/
	uint64_t register_2                                              : 4;
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_REGISTER_2_BIT    28
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_REGISTER_2_FLAG   0xF0000000
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_REGISTER_2_MASK   0x0F
#define VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE_REGISTER_2(_)     (((_) >> 28) & 0x0F)
	uint64_t reserved3                                               : 32;
  };

  uint64_t flags;
} vmx_vmexit_instruction_info_vmread_vmwrite;

/**
 * @}
 */

/**
 * @brief - The low 16 bits correspond to bits 23:8 of the upper 32 bits of a 64-bit segment descriptor. While bits 19:16
 *        of code-segment and data-segment descriptors correspond to the upper 4 bits of the segment limit, the corresponding bits
 *        (bits 11:8) are reserved in this VMCS field.
 *        - Bit 16 indicates an unusable segment. Attempts to use such a segment fault except in 64-bit mode. In general, a
 *        segment register is unusable if it has been loaded with a null selector.
 *        - Bits 31:17 are reserved
 *
 * @note There are a few exceptions to this statement. For example, a segment with a non-null selector may be unusable
 *       following a task switch that fails after its commit point. In contrast, the TR register is usable after processor reset
 *       despite having a null selector
 * @see SEGMENT_DESCRIPTOR_32
 * @see SEGMENT_DESCRIPTOR_64
 * @see XXX_ACCESS_RIGHTS fields of 32_BIT_GUEST_STATE_FIELDS
 * @see Vol3C[24.4.2(Guest Non-Register State)] (reference)
 */
typedef union
{
  struct
  {
	/**
	* [Bits 3:0] Segment type.
	*/
	uint32_t type                                                    : 4;
#define VMX_SEGMENT_ACCESS_RIGHTS_TYPE_BIT                           0
#define VMX_SEGMENT_ACCESS_RIGHTS_TYPE_FLAG                          0x0F
#define VMX_SEGMENT_ACCESS_RIGHTS_TYPE_MASK                          0x0F
#define VMX_SEGMENT_ACCESS_RIGHTS_TYPE(_)                            (((_) >> 0) & 0x0F)

	/**
	* [Bit 4] S - Descriptor type (0 = system; 1 = code or data).
	*/
	uint32_t descriptor_type                                         : 1;
#define VMX_SEGMENT_ACCESS_RIGHTS_DESCRIPTOR_TYPE_BIT                4
#define VMX_SEGMENT_ACCESS_RIGHTS_DESCRIPTOR_TYPE_FLAG               0x10
#define VMX_SEGMENT_ACCESS_RIGHTS_DESCRIPTOR_TYPE_MASK               0x01
#define VMX_SEGMENT_ACCESS_RIGHTS_DESCRIPTOR_TYPE(_)                 (((_) >> 4) & 0x01)

	/**
	* [Bits 6:5] DPL - Descriptor privilege level.
	*/
	uint32_t descriptor_privilege_level                              : 2;
#define VMX_SEGMENT_ACCESS_RIGHTS_DESCRIPTOR_PRIVILEGE_LEVEL_BIT     5
#define VMX_SEGMENT_ACCESS_RIGHTS_DESCRIPTOR_PRIVILEGE_LEVEL_FLAG    0x60
#define VMX_SEGMENT_ACCESS_RIGHTS_DESCRIPTOR_PRIVILEGE_LEVEL_MASK    0x03
#define VMX_SEGMENT_ACCESS_RIGHTS_DESCRIPTOR_PRIVILEGE_LEVEL(_)      (((_) >> 5) & 0x03)

	/**
	* [Bit 7] P - Segment present.
	*/
	uint32_t present                                                 : 1;
#define VMX_SEGMENT_ACCESS_RIGHTS_PRESENT_BIT                        7
#define VMX_SEGMENT_ACCESS_RIGHTS_PRESENT_FLAG                       0x80
#define VMX_SEGMENT_ACCESS_RIGHTS_PRESENT_MASK                       0x01
#define VMX_SEGMENT_ACCESS_RIGHTS_PRESENT(_)                         (((_) >> 7) & 0x01)
	uint32_t reserved1                                               : 4;

	/**
	* [Bit 12] AVL - Available for use by system software.
	*/
	uint32_t available_bit                                           : 1;
#define VMX_SEGMENT_ACCESS_RIGHTS_AVAILABLE_BIT_BIT                  12
#define VMX_SEGMENT_ACCESS_RIGHTS_AVAILABLE_BIT_FLAG                 0x1000
#define VMX_SEGMENT_ACCESS_RIGHTS_AVAILABLE_BIT_MASK                 0x01
#define VMX_SEGMENT_ACCESS_RIGHTS_AVAILABLE_BIT(_)                   (((_) >> 12) & 0x01)

	/**
	* [Bit 13] Reserved (except for CS). L - 64-bit mode active (for CS only).
	*/
	uint32_t long_mode                                               : 1;
#define VMX_SEGMENT_ACCESS_RIGHTS_LONG_MODE_BIT                      13
#define VMX_SEGMENT_ACCESS_RIGHTS_LONG_MODE_FLAG                     0x2000
#define VMX_SEGMENT_ACCESS_RIGHTS_LONG_MODE_MASK                     0x01
#define VMX_SEGMENT_ACCESS_RIGHTS_LONG_MODE(_)                       (((_) >> 13) & 0x01)

	/**
	* [Bit 14] D/B - Default operation size (0 = 16-bit segment; 1 = 32-bit segment).
	*/
	uint32_t default_big                                             : 1;
#define VMX_SEGMENT_ACCESS_RIGHTS_DEFAULT_BIG_BIT                    14
#define VMX_SEGMENT_ACCESS_RIGHTS_DEFAULT_BIG_FLAG                   0x4000
#define VMX_SEGMENT_ACCESS_RIGHTS_DEFAULT_BIG_MASK                   0x01
#define VMX_SEGMENT_ACCESS_RIGHTS_DEFAULT_BIG(_)                     (((_) >> 14) & 0x01)

	/**
	* [Bit 15] G - Granularity.
	*/
	uint32_t granularity                                             : 1;
#define VMX_SEGMENT_ACCESS_RIGHTS_GRANULARITY_BIT                    15
#define VMX_SEGMENT_ACCESS_RIGHTS_GRANULARITY_FLAG                   0x8000
#define VMX_SEGMENT_ACCESS_RIGHTS_GRANULARITY_MASK                   0x01
#define VMX_SEGMENT_ACCESS_RIGHTS_GRANULARITY(_)                     (((_) >> 15) & 0x01)

	/**
	* [Bit 16] Segment unusable (0 = usable; 1 = unusable).
	*/
	uint32_t unusable                                                : 1;
#define VMX_SEGMENT_ACCESS_RIGHTS_UNUSABLE_BIT                       16
#define VMX_SEGMENT_ACCESS_RIGHTS_UNUSABLE_FLAG                      0x10000
#define VMX_SEGMENT_ACCESS_RIGHTS_UNUSABLE_MASK                      0x01
#define VMX_SEGMENT_ACCESS_RIGHTS_UNUSABLE(_)                        (((_) >> 16) & 0x01)
	uint32_t reserved2                                               : 15;
  };

  uint32_t flags;
} vmx_segment_access_rights;

/**
 * @brief The IA-32 architecture includes features that permit certain events to be blocked for a period of time. This
 *        field contains information about such blocking
 *
 * @see INTERRUPTIBILITY_STATE of 32_BIT_GUEST_STATE_FIELDS
 * @see Vol3C[24.4.2(Guest Non-Register State)] (reference)
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] Execution of STI with RFLAGS.IF = 0 blocks maskable interrupts on the instruction boundary following its
	* execution.1 Setting this bit indicates that this blocking is in effect.
	*
	* @see Vol2B[4(STI-Set Interrupt Flag)]
	*/
	uint32_t blocking_by_sti                                         : 1;
#define VMX_INTERRUPTIBILITY_STATE_BLOCKING_BY_STI_BIT               0
#define VMX_INTERRUPTIBILITY_STATE_BLOCKING_BY_STI_FLAG              0x01
#define VMX_INTERRUPTIBILITY_STATE_BLOCKING_BY_STI_MASK              0x01
#define VMX_INTERRUPTIBILITY_STATE_BLOCKING_BY_STI(_)                (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Execution of a MOV to SS or a POP to SS blocks or suppresses certain debug exceptions as well as interrupts
	* (maskable and nonmaskable) on the instruction boundary following its execution. Setting this bit indicates that this
	* blocking is in effect. This document uses the term "blocking by MOV SS," but it applies equally to POP SS.
	*
	* @see Vol3A[6.8.3(Masking Exceptions and Interrupts When Switching Stacks)]
	*/
	uint32_t blocking_by_mov_ss                                      : 1;
#define VMX_INTERRUPTIBILITY_STATE_BLOCKING_BY_MOV_SS_BIT            1
#define VMX_INTERRUPTIBILITY_STATE_BLOCKING_BY_MOV_SS_FLAG           0x02
#define VMX_INTERRUPTIBILITY_STATE_BLOCKING_BY_MOV_SS_MASK           0x01
#define VMX_INTERRUPTIBILITY_STATE_BLOCKING_BY_MOV_SS(_)             (((_) >> 1) & 0x01)

	/**
	* [Bit 2] System-management interrupts (SMIs) are disabled while the processor is in system-management mode (SMM). Setting
	* this bit indicates that blocking of SMIs is in effect.
	*
	* @see Vol3C[34.2(System Management Interrupt (SMI))]
	*/
	uint32_t blocking_by_smi                                         : 1;
#define VMX_INTERRUPTIBILITY_STATE_BLOCKING_BY_SMI_BIT               2
#define VMX_INTERRUPTIBILITY_STATE_BLOCKING_BY_SMI_FLAG              0x04
#define VMX_INTERRUPTIBILITY_STATE_BLOCKING_BY_SMI_MASK              0x01
#define VMX_INTERRUPTIBILITY_STATE_BLOCKING_BY_SMI(_)                (((_) >> 2) & 0x01)

	/**
	* [Bit 3] Delivery of a non-maskable interrupt (NMI) or a system-management interrupt (SMI) blocks subsequent NMIs until
	* the next execution of IRET. Setting this bit indicates that blocking of NMIs is in effect. Clearing this bit does not
	* imply that NMIs are not (temporarily) blocked for other reasons. If the "virtual NMIs" VM-execution control is 1, this
	* bit does not control the blocking of NMIs. Instead, it refers to "virtual-NMI blocking" (the fact that guest software is
	* not ready for an NMI).
	*
	* @see Vol3C[6.7.1(Handling Multiple NMIs)]
	* @see Vol3C[25.3(CHANGES TO INSTRUCTION BEHAVIOR IN VMX NON-ROOT OPERATION)]
	* @see Vol3C[24.6.1(Pin-Based VM-Execution Controls)]
	*/
	uint32_t blocking_by_nmi                                         : 1;
#define VMX_INTERRUPTIBILITY_STATE_BLOCKING_BY_NMI_BIT               3
#define VMX_INTERRUPTIBILITY_STATE_BLOCKING_BY_NMI_FLAG              0x08
#define VMX_INTERRUPTIBILITY_STATE_BLOCKING_BY_NMI_MASK              0x01
#define VMX_INTERRUPTIBILITY_STATE_BLOCKING_BY_NMI(_)                (((_) >> 3) & 0x01)

	/**
	* [Bit 4] A VM exit saves this bit as 1 to indicate that the VM exit was incident to enclave mode.
	*/
	uint32_t enclave_interruption                                    : 1;
#define VMX_INTERRUPTIBILITY_STATE_ENCLAVE_INTERRUPTION_BIT          4
#define VMX_INTERRUPTIBILITY_STATE_ENCLAVE_INTERRUPTION_FLAG         0x10
#define VMX_INTERRUPTIBILITY_STATE_ENCLAVE_INTERRUPTION_MASK         0x01
#define VMX_INTERRUPTIBILITY_STATE_ENCLAVE_INTERRUPTION(_)           (((_) >> 4) & 0x01)
	uint32_t reserved1                                               : 27;
  };

  uint32_t flags;
} vmx_interruptibility_state;

typedef enum
{
  /**
	* The logical processor is executing instructions normally.
	*/
  vmx_active                                                   = 0x00000000,

  /**
	* The logical processor is inactive because it executed the HLT instruction.
	*/
  vmx_hlt                                                      = 0x00000001,

  /**
	* The logical processor is inactive because it incurred a triple fault1 or some other serious error.
	*/
  vmx_shutdown                                                 = 0x00000002,

  /**
	* The logical processor is inactive because it is waiting for a startup-IPI (SIPI).
	*/
  vmx_wait_for_sipi                                            = 0x00000003,
} vmx_guest_activity_state;

/**
 * @}
 */

/**
 * @brief Format of Exit Reason
 *
 * Exit reason (32 bits). This field encodes the reason for the VM exit and has the structure.
 *
 * @see Vol3C[24.9.1(Basic VM-Exit Information)] (reference)
 */
typedef union
{
  struct
  {
	/**
	* [Bits 15:0] Provides basic information about the cause of the VM exit (if bit 31 is clear) or of the VM-entry failure
	* (if bit 31 is set).
	*/
	uint32_t basic_exit_reason                                       : 16;
#define VMX_VMEXIT_REASON_BASIC_EXIT_REASON_BIT                      0
#define VMX_VMEXIT_REASON_BASIC_EXIT_REASON_FLAG                     0xFFFF
#define VMX_VMEXIT_REASON_BASIC_EXIT_REASON_MASK                     0xFFFF
#define VMX_VMEXIT_REASON_BASIC_EXIT_REASON(_)                       (((_) >> 0) & 0xFFFF)

	/**
	* [Bit 16] Always cleared to 0.
	*/
	uint32_t always0                                                 : 1;
#define VMX_VMEXIT_REASON_ALWAYS0_BIT                                16
#define VMX_VMEXIT_REASON_ALWAYS0_FLAG                               0x10000
#define VMX_VMEXIT_REASON_ALWAYS0_MASK                               0x01
#define VMX_VMEXIT_REASON_ALWAYS0(_)                                 (((_) >> 16) & 0x01)
	uint32_t reserved1                                               : 10;
#define VMX_VMEXIT_REASON_RESERVED1_BIT                              17
#define VMX_VMEXIT_REASON_RESERVED1_FLAG                             0x7FE0000
#define VMX_VMEXIT_REASON_RESERVED1_MASK                             0x3FF
#define VMX_VMEXIT_REASON_RESERVED1(_)                               (((_) >> 17) & 0x3FF)

	/**
	* [Bit 27] A VM exit saves this bit as 1 to indicate that the VM exit was incident to enclave mode.
	*/
	uint32_t enclave_mode                                            : 1;
#define VMX_VMEXIT_REASON_ENCLAVE_MODE_BIT                           27
#define VMX_VMEXIT_REASON_ENCLAVE_MODE_FLAG                          0x8000000
#define VMX_VMEXIT_REASON_ENCLAVE_MODE_MASK                          0x01
#define VMX_VMEXIT_REASON_ENCLAVE_MODE(_)                            (((_) >> 27) & 0x01)

	/**
	* [Bit 28] Pending MTF VM exit.
	*/
	uint32_t pending_mtf_vm_exit                                     : 1;
#define VMX_VMEXIT_REASON_PENDING_MTF_VM_EXIT_BIT                    28
#define VMX_VMEXIT_REASON_PENDING_MTF_VM_EXIT_FLAG                   0x10000000
#define VMX_VMEXIT_REASON_PENDING_MTF_VM_EXIT_MASK                   0x01
#define VMX_VMEXIT_REASON_PENDING_MTF_VM_EXIT(_)                     (((_) >> 28) & 0x01)

	/**
	* [Bit 29] VM exit from VMX root operation.
	*/
	uint32_t vm_exit_from_vmx_roor                                   : 1;
#define VMX_VMEXIT_REASON_VM_EXIT_FROM_VMX_ROOR_BIT                  29
#define VMX_VMEXIT_REASON_VM_EXIT_FROM_VMX_ROOR_FLAG                 0x20000000
#define VMX_VMEXIT_REASON_VM_EXIT_FROM_VMX_ROOR_MASK                 0x01
#define VMX_VMEXIT_REASON_VM_EXIT_FROM_VMX_ROOR(_)                   (((_) >> 29) & 0x01)
	uint32_t reserved2                                               : 1;
#define VMX_VMEXIT_REASON_RESERVED2_BIT                              30
#define VMX_VMEXIT_REASON_RESERVED2_FLAG                             0x40000000
#define VMX_VMEXIT_REASON_RESERVED2_MASK                             0x01
#define VMX_VMEXIT_REASON_RESERVED2(_)                               (((_) >> 30) & 0x01)

	/**
	* [Bit 31] VM-entry failure:
	*   - 0 = true VM exit
	*   - 1 = VM-entry failure
	*/
	uint32_t vm_entry_failure                                        : 1;
#define VMX_VMEXIT_REASON_VM_ENTRY_FAILURE_BIT                       31
#define VMX_VMEXIT_REASON_VM_ENTRY_FAILURE_FLAG                      0x80000000
#define VMX_VMEXIT_REASON_VM_ENTRY_FAILURE_MASK                      0x01
#define VMX_VMEXIT_REASON_VM_ENTRY_FAILURE(_)                        (((_) >> 31) & 0x01)
  };

  uint32_t flags;
} vmx_vmexit_reason;

typedef struct
{
#define IO_BITMAP_A_MIN                                              0x00000000
#define IO_BITMAP_A_MAX                                              0x00007FFF
#define IO_BITMAP_B_MIN                                              0x00008000
#define IO_BITMAP_B_MAX                                              0x0000FFFF
  uint8_t io_a[4096];
  uint8_t io_b[4096];
} vmx_io_bitmap;

typedef struct
{
#define MSR_ID_LOW_MIN                                               0x00000000
#define MSR_ID_LOW_MAX                                               0x00001FFF
#define MSR_ID_HIGH_MIN                                              0xC0000000
#define MSR_ID_HIGH_MAX                                              0xC0001FFF
  uint8_t rdmsr_low[1024];
  uint8_t rdmsr_high[1024];
  uint8_t wrmsr_low[1024];
  uint8_t wrmsr_high[1024];
} vmx_msr_bitmap;

/**
 * @defgroup ept \
 *           The extended page-table mechanism
 *
 * The extended page-table mechanism (EPT) is a feature that can be used to support the virtualization of physical memory.
 * When EPT is in use, certain addresses that would normally be treated as physical addresses (and used to access memory)
 * are instead treated as guest-physical addresses. Guest-physical addresses are translated by traversing a set of EPT
 * paging structures to produce physical addresses that are used to access memory.
 *
 * @see Vol3C[28.2(THE EXTENDED PAGE TABLE MECHANISM (EPT))] (reference)
 * @{
 */
/**
 * @brief Extended-Page-Table Pointer (EPTP)
 *
 * The extended-page-table pointer (EPTP) contains the address of the base of EPT PML4 table, as well as other EPT
 * configuration information.
 *
 * @see Vol3C[28.2.2(EPT Translation Mechanism]
 * @see Vol3C[24.6.11(Extended-Page-Table Pointer (EPTP)] (reference)
 */
typedef union
{
  struct
  {
	/**
	* [Bits 2:0] EPT paging-structure memory type:
	* - 0 = Uncacheable (UC)
	* - 6 = Write-back (WB)
	* Other values are reserved.
	*
	* @see Vol3C[28.2.6(EPT and memory Typing)]
	*/
	uint64_t memory_type                                             : 3;
#define EPT_POINTER_MEMORY_TYPE_BIT                                  0
#define EPT_POINTER_MEMORY_TYPE_FLAG                                 0x07
#define EPT_POINTER_MEMORY_TYPE_MASK                                 0x07
#define EPT_POINTER_MEMORY_TYPE(_)                                   (((_) >> 0) & 0x07)

	/**
	* [Bits 5:3] This value is 1 less than the EPT page-walk length.
	*
	* @see Vol3C[28.2.6(EPT and memory Typing)]
	*/
	uint64_t page_walk_length                                        : 3;
#define EPT_POINTER_PAGE_WALK_LENGTH_BIT                             3
#define EPT_POINTER_PAGE_WALK_LENGTH_FLAG                            0x38
#define EPT_POINTER_PAGE_WALK_LENGTH_MASK                            0x07
#define EPT_POINTER_PAGE_WALK_LENGTH(_)                              (((_) >> 3) & 0x07)
#define EPT_PAGE_WALK_LENGTH_4                                       0x00000003

	/**
	* [Bit 6] Setting this control to 1 enables accessed and dirty flags for EPT.
	*
	* @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
	*/
	uint64_t enable_access_and_dirty_flags                           : 1;
#define EPT_POINTER_ENABLE_ACCESS_AND_DIRTY_FLAGS_BIT                6
#define EPT_POINTER_ENABLE_ACCESS_AND_DIRTY_FLAGS_FLAG               0x40
#define EPT_POINTER_ENABLE_ACCESS_AND_DIRTY_FLAGS_MASK               0x01
#define EPT_POINTER_ENABLE_ACCESS_AND_DIRTY_FLAGS(_)                 (((_) >> 6) & 0x01)
	uint64_t reserved1                                               : 5;

	/**
	* [Bits 47:12] Bits N-1:12 of the physical address of the 4-KByte aligned EPT PML4 table.
	*/
	uint64_t page_frame_number                                       : 36;
#define EPT_POINTER_PAGE_FRAME_NUMBER_BIT                            12
#define EPT_POINTER_PAGE_FRAME_NUMBER_FLAG                           0xFFFFFFFFF000
#define EPT_POINTER_PAGE_FRAME_NUMBER_MASK                           0xFFFFFFFFF
#define EPT_POINTER_PAGE_FRAME_NUMBER(_)                             (((_) >> 12) & 0xFFFFFFFFF)
	uint64_t reserved2                                               : 16;
  };

  uint64_t flags;
} ept_pointer;

/**
 * @brief Format of an EPT PML4 Entry (PML4E) that References an EPT Page-Directory-Pointer Table
 *
 * A 4-KByte naturally aligned EPT PML4 table is located at the physical address specified in bits 51:12 of the
 * extended-page-table pointer (EPTP), a VM-execution control field. An EPT PML4 table comprises 512 64-bit entries (EPT
 * PML4Es). An EPT PML4E is selected using the physical address defined as follows:
 * - Bits 63:52 are all 0.
 * - Bits 51:12 are from the EPTP.
 * - Bits 11:3 are bits 47:39 of the guest-physical address.
 * - Bits 2:0 are all 0.
 * Because an EPT PML4E is identified using bits 47:39 of the guest-physical address, it controls access to a 512- GByte
 * region of the guest-physical-address space.
 *
 * @see Vol3C[24.6.11(Extended-Page-Table Pointer (EPTP)]
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] Read access; indicates whether reads are allowed from the 512-GByte region controlled by this entry.
	*/
	uint64_t read_access                                             : 1;
#define EPT_PML4_READ_ACCESS_BIT                                     0
#define EPT_PML4_READ_ACCESS_FLAG                                    0x01
#define EPT_PML4_READ_ACCESS_MASK                                    0x01
#define EPT_PML4_READ_ACCESS(_)                                      (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Write access; indicates whether writes are allowed from the 512-GByte region controlled by this entry.
	*/
	uint64_t write_access                                            : 1;
#define EPT_PML4_WRITE_ACCESS_BIT                                    1
#define EPT_PML4_WRITE_ACCESS_FLAG                                   0x02
#define EPT_PML4_WRITE_ACCESS_MASK                                   0x01
#define EPT_PML4_WRITE_ACCESS(_)                                     (((_) >> 1) & 0x01)

	/**
	* [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
	* instruction fetches are allowed from the 512-GByte region controlled by this entry.
	* If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
	* allowed from supervisor-mode linear addresses in the 512-GByte region controlled by this entry.
	*/
	uint64_t execute_access                                          : 1;
#define EPT_PML4_EXECUTE_ACCESS_BIT                                  2
#define EPT_PML4_EXECUTE_ACCESS_FLAG                                 0x04
#define EPT_PML4_EXECUTE_ACCESS_MASK                                 0x01
#define EPT_PML4_EXECUTE_ACCESS(_)                                   (((_) >> 2) & 0x01)
	uint64_t reserved1                                               : 5;

	/**
	* [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 512-GByte region
	* controlled by this entry. Ignored if bit 6 of EPTP is 0.
	*
	* @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
	*/
	uint64_t accessed                                                : 1;
#define EPT_PML4_ACCESSED_BIT                                        8
#define EPT_PML4_ACCESSED_FLAG                                       0x100
#define EPT_PML4_ACCESSED_MASK                                       0x01
#define EPT_PML4_ACCESSED(_)                                         (((_) >> 8) & 0x01)
	uint64_t reserved2                                               : 1;

	/**
	* [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
	* is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 512-GByte region
	* controlled by this entry. If that control is 0, this bit is ignored.
	*/
	uint64_t user_mode_execute                                       : 1;
#define EPT_PML4_USER_MODE_EXECUTE_BIT                               10
#define EPT_PML4_USER_MODE_EXECUTE_FLAG                              0x400
#define EPT_PML4_USER_MODE_EXECUTE_MASK                              0x01
#define EPT_PML4_USER_MODE_EXECUTE(_)                                (((_) >> 10) & 0x01)
	uint64_t reserved3                                               : 1;

	/**
	* [Bits 47:12] Physical address of 4-KByte aligned EPT page-directory-pointer table referenced by this entry.
	*/
	uint64_t page_frame_number                                       : 36;
#define EPT_PML4_PAGE_FRAME_NUMBER_BIT                               12
#define EPT_PML4_PAGE_FRAME_NUMBER_FLAG                              0xFFFFFFFFF000
#define EPT_PML4_PAGE_FRAME_NUMBER_MASK                              0xFFFFFFFFF
#define EPT_PML4_PAGE_FRAME_NUMBER(_)                                (((_) >> 12) & 0xFFFFFFFFF)
	uint64_t reserved4                                               : 16;
  };

  uint64_t flags;
} ept_pml4;

/**
 * @brief Format of an EPT Page-Directory-Pointer-Table Entry (PDPTE) that Maps a 1-GByte Page
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] Read access; indicates whether reads are allowed from the 1-GByte page referenced by this entry.
	*/
	uint64_t read_access                                             : 1;
#define EPDPTE_1GB_READ_ACCESS_BIT                                   0
#define EPDPTE_1GB_READ_ACCESS_FLAG                                  0x01
#define EPDPTE_1GB_READ_ACCESS_MASK                                  0x01
#define EPDPTE_1GB_READ_ACCESS(_)                                    (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Write access; indicates whether writes are allowed from the 1-GByte page referenced by this entry.
	*/
	uint64_t write_access                                            : 1;
#define EPDPTE_1GB_WRITE_ACCESS_BIT                                  1
#define EPDPTE_1GB_WRITE_ACCESS_FLAG                                 0x02
#define EPDPTE_1GB_WRITE_ACCESS_MASK                                 0x01
#define EPDPTE_1GB_WRITE_ACCESS(_)                                   (((_) >> 1) & 0x01)

	/**
	* [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
	* instruction fetches are allowed from the 1-GByte page controlled by this entry.
	* If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
	* allowed from supervisor-mode linear addresses in the 1-GByte page controlled by this entry.
	*/
	uint64_t execute_access                                          : 1;
#define EPDPTE_1GB_EXECUTE_ACCESS_BIT                                2
#define EPDPTE_1GB_EXECUTE_ACCESS_FLAG                               0x04
#define EPDPTE_1GB_EXECUTE_ACCESS_MASK                               0x01
#define EPDPTE_1GB_EXECUTE_ACCESS(_)                                 (((_) >> 2) & 0x01)

	/**
	* [Bits 5:3] EPT memory type for this 1-GByte page.
	*
	* @see Vol3C[28.2.6(EPT and memory Typing)]
	*/
	uint64_t memory_type                                             : 3;
#define EPDPTE_1GB_MEMORY_TYPE_BIT                                   3
#define EPDPTE_1GB_MEMORY_TYPE_FLAG                                  0x38
#define EPDPTE_1GB_MEMORY_TYPE_MASK                                  0x07
#define EPDPTE_1GB_MEMORY_TYPE(_)                                    (((_) >> 3) & 0x07)

	/**
	* [Bit 6] Ignore PAT memory type for this 1-GByte page.
	*
	* @see Vol3C[28.2.6(EPT and memory Typing)]
	*/
	uint64_t ignore_pat                                              : 1;
#define EPDPTE_1GB_IGNORE_PAT_BIT                                    6
#define EPDPTE_1GB_IGNORE_PAT_FLAG                                   0x40
#define EPDPTE_1GB_IGNORE_PAT_MASK                                   0x01
#define EPDPTE_1GB_IGNORE_PAT(_)                                     (((_) >> 6) & 0x01)

	/**
	* [Bit 7] Must be 1 (otherwise, this entry references an EPT page directory).
	*/
	uint64_t large_page                                              : 1;
#define EPDPTE_1GB_LARGE_PAGE_BIT                                    7
#define EPDPTE_1GB_LARGE_PAGE_FLAG                                   0x80
#define EPDPTE_1GB_LARGE_PAGE_MASK                                   0x01
#define EPDPTE_1GB_LARGE_PAGE(_)                                     (((_) >> 7) & 0x01)

	/**
	* [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 1-GByte page
	* referenced by this entry. Ignored if bit 6 of EPTP is 0.
	*
	* @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
	*/
	uint64_t accessed                                                : 1;
#define EPDPTE_1GB_ACCESSED_BIT                                      8
#define EPDPTE_1GB_ACCESSED_FLAG                                     0x100
#define EPDPTE_1GB_ACCESSED_MASK                                     0x01
#define EPDPTE_1GB_ACCESSED(_)                                       (((_) >> 8) & 0x01)

	/**
	* [Bit 9] If bit 6 of EPTP is 1, dirty flag for EPT; indicates whether software has written to the 1-GByte page referenced
	* by this entry. Ignored if bit 6 of EPTP is 0.
	*
	* @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
	*/
	uint64_t dirty                                                   : 1;
#define EPDPTE_1GB_DIRTY_BIT                                         9
#define EPDPTE_1GB_DIRTY_FLAG                                        0x200
#define EPDPTE_1GB_DIRTY_MASK                                        0x01
#define EPDPTE_1GB_DIRTY(_)                                          (((_) >> 9) & 0x01)

	/**
	* [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
	* is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 1-GByte page controlled
	* by this entry. If that control is 0, this bit is ignored.
	*/
	uint64_t user_mode_execute                                       : 1;
#define EPDPTE_1GB_USER_MODE_EXECUTE_BIT                             10
#define EPDPTE_1GB_USER_MODE_EXECUTE_FLAG                            0x400
#define EPDPTE_1GB_USER_MODE_EXECUTE_MASK                            0x01
#define EPDPTE_1GB_USER_MODE_EXECUTE(_)                              (((_) >> 10) & 0x01)
	uint64_t reserved1                                               : 19;

	/**
	* [Bits 47:30] Physical address of 4-KByte aligned EPT page-directory-pointer table referenced by this entry.
	*/
	uint64_t page_frame_number                                       : 18;
#define EPDPTE_1GB_PAGE_FRAME_NUMBER_BIT                             30
#define EPDPTE_1GB_PAGE_FRAME_NUMBER_FLAG                            0xFFFFC0000000
#define EPDPTE_1GB_PAGE_FRAME_NUMBER_MASK                            0x3FFFF
#define EPDPTE_1GB_PAGE_FRAME_NUMBER(_)                              (((_) >> 30) & 0x3FFFF)
	uint64_t reserved2                                               : 15;

	/**
	* [Bit 63] Suppress \#VE. If the "EPT-violation \#VE" VM-execution control is 1, EPT violations caused by accesses to this
	* page are convertible to virtualization exceptions only if this bit is 0. If "EPT-violation \#VE" VMexecution control is
	* 0, this bit is ignored.
	*
	* @see Vol3C[25.5.6.1(Convertible EPT Violations)]
	*/
	uint64_t suppress_ve                                             : 1;
#define EPDPTE_1GB_SUPPRESS_VE_BIT                                   63
#define EPDPTE_1GB_SUPPRESS_VE_FLAG                                  0x8000000000000000
#define EPDPTE_1GB_SUPPRESS_VE_MASK                                  0x01
#define EPDPTE_1GB_SUPPRESS_VE(_)                                    (((_) >> 63) & 0x01)
  };

  uint64_t flags;
} epdpte_1gb;

/**
 * @brief Format of an EPT Page-Directory-Pointer-Table Entry (PDPTE) that References an EPT Page Directory
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] Read access; indicates whether reads are allowed from the 1-GByte region controlled by this entry.
	*/
	uint64_t read_access                                             : 1;
#define EPDPTE_READ_ACCESS_BIT                                       0
#define EPDPTE_READ_ACCESS_FLAG                                      0x01
#define EPDPTE_READ_ACCESS_MASK                                      0x01
#define EPDPTE_READ_ACCESS(_)                                        (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Write access; indicates whether writes are allowed from the 1-GByte region controlled by this entry.
	*/
	uint64_t write_access                                            : 1;
#define EPDPTE_WRITE_ACCESS_BIT                                      1
#define EPDPTE_WRITE_ACCESS_FLAG                                     0x02
#define EPDPTE_WRITE_ACCESS_MASK                                     0x01
#define EPDPTE_WRITE_ACCESS(_)                                       (((_) >> 1) & 0x01)

	/**
	* [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
	* instruction fetches are allowed from the 1-GByte region controlled by this entry.
	* If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
	* allowed from supervisor-mode linear addresses in the 1-GByte region controlled by this entry.
	*/
	uint64_t execute_access                                          : 1;
#define EPDPTE_EXECUTE_ACCESS_BIT                                    2
#define EPDPTE_EXECUTE_ACCESS_FLAG                                   0x04
#define EPDPTE_EXECUTE_ACCESS_MASK                                   0x01
#define EPDPTE_EXECUTE_ACCESS(_)                                     (((_) >> 2) & 0x01)
	uint64_t reserved1                                               : 5;

	/**
	* [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 1-GByte region
	* controlled by this entry. Ignored if bit 6 of EPTP is 0.
	*
	* @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
	*/
	uint64_t accessed                                                : 1;
#define EPDPTE_ACCESSED_BIT                                          8
#define EPDPTE_ACCESSED_FLAG                                         0x100
#define EPDPTE_ACCESSED_MASK                                         0x01
#define EPDPTE_ACCESSED(_)                                           (((_) >> 8) & 0x01)
	uint64_t reserved2                                               : 1;

	/**
	* [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
	* is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 1-GByte region controlled
	* by this entry. If that control is 0, this bit is ignored.
	*/
	uint64_t user_mode_execute                                       : 1;
#define EPDPTE_USER_MODE_EXECUTE_BIT                                 10
#define EPDPTE_USER_MODE_EXECUTE_FLAG                                0x400
#define EPDPTE_USER_MODE_EXECUTE_MASK                                0x01
#define EPDPTE_USER_MODE_EXECUTE(_)                                  (((_) >> 10) & 0x01)
	uint64_t reserved3                                               : 1;

	/**
	* [Bits 47:12] Physical address of 4-KByte aligned EPT page-directory-pointer table referenced by this entry.
	*/
	uint64_t page_frame_number                                       : 36;
#define EPDPTE_PAGE_FRAME_NUMBER_BIT                                 12
#define EPDPTE_PAGE_FRAME_NUMBER_FLAG                                0xFFFFFFFFF000
#define EPDPTE_PAGE_FRAME_NUMBER_MASK                                0xFFFFFFFFF
#define EPDPTE_PAGE_FRAME_NUMBER(_)                                  (((_) >> 12) & 0xFFFFFFFFF)
	uint64_t reserved4                                               : 16;
  };

  uint64_t flags;
} epdpte;

/**
 * @brief Format of an EPT Page-Directory Entry (PDE) that Maps a 2-MByte Page
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] Read access; indicates whether reads are allowed from the 2-MByte page referenced by this entry.
	*/
	uint64_t read_access                                             : 1;
#define EPDE_2MB_READ_ACCESS_BIT                                     0
#define EPDE_2MB_READ_ACCESS_FLAG                                    0x01
#define EPDE_2MB_READ_ACCESS_MASK                                    0x01
#define EPDE_2MB_READ_ACCESS(_)                                      (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Write access; indicates whether writes are allowed from the 2-MByte page referenced by this entry.
	*/
	uint64_t write_access                                            : 1;
#define EPDE_2MB_WRITE_ACCESS_BIT                                    1
#define EPDE_2MB_WRITE_ACCESS_FLAG                                   0x02
#define EPDE_2MB_WRITE_ACCESS_MASK                                   0x01
#define EPDE_2MB_WRITE_ACCESS(_)                                     (((_) >> 1) & 0x01)

	/**
	* [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
	* instruction fetches are allowed from the 2-MByte page controlled by this entry.
	* If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
	* allowed from supervisor-mode linear addresses in the 2-MByte page controlled by this entry.
	*/
	uint64_t execute_access                                          : 1;
#define EPDE_2MB_EXECUTE_ACCESS_BIT                                  2
#define EPDE_2MB_EXECUTE_ACCESS_FLAG                                 0x04
#define EPDE_2MB_EXECUTE_ACCESS_MASK                                 0x01
#define EPDE_2MB_EXECUTE_ACCESS(_)                                   (((_) >> 2) & 0x01)

	/**
	* [Bits 5:3] EPT memory type for this 2-MByte page.
	*
	* @see Vol3C[28.2.6(EPT and memory Typing)]
	*/
	uint64_t memory_type                                             : 3;
#define EPDE_2MB_MEMORY_TYPE_BIT                                     3
#define EPDE_2MB_MEMORY_TYPE_FLAG                                    0x38
#define EPDE_2MB_MEMORY_TYPE_MASK                                    0x07
#define EPDE_2MB_MEMORY_TYPE(_)                                      (((_) >> 3) & 0x07)

	/**
	* [Bit 6] Ignore PAT memory type for this 2-MByte page.
	*
	* @see Vol3C[28.2.6(EPT and memory Typing)]
	*/
	uint64_t ignore_pat                                              : 1;
#define EPDE_2MB_IGNORE_PAT_BIT                                      6
#define EPDE_2MB_IGNORE_PAT_FLAG                                     0x40
#define EPDE_2MB_IGNORE_PAT_MASK                                     0x01
#define EPDE_2MB_IGNORE_PAT(_)                                       (((_) >> 6) & 0x01)

	/**
	* [Bit 7] Must be 1 (otherwise, this entry references an EPT page table).
	*/
	uint64_t large_page                                              : 1;
#define EPDE_2MB_LARGE_PAGE_BIT                                      7
#define EPDE_2MB_LARGE_PAGE_FLAG                                     0x80
#define EPDE_2MB_LARGE_PAGE_MASK                                     0x01
#define EPDE_2MB_LARGE_PAGE(_)                                       (((_) >> 7) & 0x01)

	/**
	* [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 2-MByte page
	* referenced by this entry. Ignored if bit 6 of EPTP is 0.
	*
	* @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
	*/
	uint64_t accessed                                                : 1;
#define EPDE_2MB_ACCESSED_BIT                                        8
#define EPDE_2MB_ACCESSED_FLAG                                       0x100
#define EPDE_2MB_ACCESSED_MASK                                       0x01
#define EPDE_2MB_ACCESSED(_)                                         (((_) >> 8) & 0x01)

	/**
	* [Bit 9] If bit 6 of EPTP is 1, dirty flag for EPT; indicates whether software has written to the 2-MByte page referenced
	* by this entry. Ignored if bit 6 of EPTP is 0.
	*
	* @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
	*/
	uint64_t dirty                                                   : 1;
#define EPDE_2MB_DIRTY_BIT                                           9
#define EPDE_2MB_DIRTY_FLAG                                          0x200
#define EPDE_2MB_DIRTY_MASK                                          0x01
#define EPDE_2MB_DIRTY(_)                                            (((_) >> 9) & 0x01)

	/**
	* [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
	* is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 2-MByte page controlled
	* by this entry. If that control is 0, this bit is ignored.
	*/
	uint64_t user_mode_execute                                       : 1;
#define EPDE_2MB_USER_MODE_EXECUTE_BIT                               10
#define EPDE_2MB_USER_MODE_EXECUTE_FLAG                              0x400
#define EPDE_2MB_USER_MODE_EXECUTE_MASK                              0x01
#define EPDE_2MB_USER_MODE_EXECUTE(_)                                (((_) >> 10) & 0x01)
	uint64_t reserved1                                               : 10;

	/**
	* [Bits 47:21] Physical address of 4-KByte aligned EPT page-directory-pointer table referenced by this entry.
	*/
	uint64_t page_frame_number                                       : 27;
#define EPDE_2MB_PAGE_FRAME_NUMBER_BIT                               21
#define EPDE_2MB_PAGE_FRAME_NUMBER_FLAG                              0xFFFFFFE00000
#define EPDE_2MB_PAGE_FRAME_NUMBER_MASK                              0x7FFFFFF
#define EPDE_2MB_PAGE_FRAME_NUMBER(_)                                (((_) >> 21) & 0x7FFFFFF)
	uint64_t reserved2                                               : 15;

	/**
	* [Bit 63] Suppress \#VE. If the "EPT-violation \#VE" VM-execution control is 1, EPT violations caused by accesses to this
	* page are convertible to virtualization exceptions only if this bit is 0. If "EPT-violation \#VE" VMexecution control is
	* 0, this bit is ignored.
	*
	* @see Vol3C[25.5.6.1(Convertible EPT Violations)]
	*/
	uint64_t suppress_ve                                             : 1;
#define EPDE_2MB_SUPPRESS_VE_BIT                                     63
#define EPDE_2MB_SUPPRESS_VE_FLAG                                    0x8000000000000000
#define EPDE_2MB_SUPPRESS_VE_MASK                                    0x01
#define EPDE_2MB_SUPPRESS_VE(_)                                      (((_) >> 63) & 0x01)
  };

  uint64_t flags;
} epde_2mb;

/**
 * @brief Format of an EPT Page-Directory Entry (PDE) that References an EPT Page Table
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] Read access; indicates whether reads are allowed from the 2-MByte region controlled by this entry.
	*/
	uint64_t read_access                                             : 1;
#define EPDE_READ_ACCESS_BIT                                         0
#define EPDE_READ_ACCESS_FLAG                                        0x01
#define EPDE_READ_ACCESS_MASK                                        0x01
#define EPDE_READ_ACCESS(_)                                          (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Write access; indicates whether writes are allowed from the 2-MByte region controlled by this entry.
	*/
	uint64_t write_access                                            : 1;
#define EPDE_WRITE_ACCESS_BIT                                        1
#define EPDE_WRITE_ACCESS_FLAG                                       0x02
#define EPDE_WRITE_ACCESS_MASK                                       0x01
#define EPDE_WRITE_ACCESS(_)                                         (((_) >> 1) & 0x01)

	/**
	* [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
	* instruction fetches are allowed from the 2-MByte region controlled by this entry.
	* If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
	* allowed from supervisor-mode linear addresses in the 2-MByte region controlled by this entry.
	*/
	uint64_t execute_access                                          : 1;
#define EPDE_EXECUTE_ACCESS_BIT                                      2
#define EPDE_EXECUTE_ACCESS_FLAG                                     0x04
#define EPDE_EXECUTE_ACCESS_MASK                                     0x01
#define EPDE_EXECUTE_ACCESS(_)                                       (((_) >> 2) & 0x01)
	uint64_t reserved1                                               : 5;

	/**
	* [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 2-MByte region
	* controlled by this entry. Ignored if bit 6 of EPTP is 0.
	*
	* @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
	*/
	uint64_t accessed                                                : 1;
#define EPDE_ACCESSED_BIT                                            8
#define EPDE_ACCESSED_FLAG                                           0x100
#define EPDE_ACCESSED_MASK                                           0x01
#define EPDE_ACCESSED(_)                                             (((_) >> 8) & 0x01)
	uint64_t reserved2                                               : 1;

	/**
	* [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
	* is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 2-MByte region controlled
	* by this entry. If that control is 0, this bit is ignored.
	*/
	uint64_t user_mode_execute                                       : 1;
#define EPDE_USER_MODE_EXECUTE_BIT                                   10
#define EPDE_USER_MODE_EXECUTE_FLAG                                  0x400
#define EPDE_USER_MODE_EXECUTE_MASK                                  0x01
#define EPDE_USER_MODE_EXECUTE(_)                                    (((_) >> 10) & 0x01)
	uint64_t reserved3                                               : 1;

	/**
	* [Bits 47:12] Physical address of 4-KByte aligned EPT page table referenced by this entry.
	*/
	uint64_t page_frame_number                                       : 36;
#define EPDE_PAGE_FRAME_NUMBER_BIT                                   12
#define EPDE_PAGE_FRAME_NUMBER_FLAG                                  0xFFFFFFFFF000
#define EPDE_PAGE_FRAME_NUMBER_MASK                                  0xFFFFFFFFF
#define EPDE_PAGE_FRAME_NUMBER(_)                                    (((_) >> 12) & 0xFFFFFFFFF)
	uint64_t reserved4                                               : 16;
  };

  uint64_t flags;
} epde;

/**
 * @brief Format of an EPT Page-Table Entry that Maps a 4-KByte Page
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] Read access; indicates whether reads are allowed from the 4-KByte page referenced by this entry.
	*/
	uint64_t read_access                                             : 1;
#define EPTE_READ_ACCESS_BIT                                         0
#define EPTE_READ_ACCESS_FLAG                                        0x01
#define EPTE_READ_ACCESS_MASK                                        0x01
#define EPTE_READ_ACCESS(_)                                          (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Write access; indicates whether writes are allowed from the 4-KByte page referenced by this entry.
	*/
	uint64_t write_access                                            : 1;
#define EPTE_WRITE_ACCESS_BIT                                        1
#define EPTE_WRITE_ACCESS_FLAG                                       0x02
#define EPTE_WRITE_ACCESS_MASK                                       0x01
#define EPTE_WRITE_ACCESS(_)                                         (((_) >> 1) & 0x01)

	/**
	* [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
	* instruction fetches are allowed from the 4-KByte page controlled by this entry.
	* If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
	* allowed from supervisor-mode linear addresses in the 4-KByte page controlled by this entry.
	*/
	uint64_t execute_access                                          : 1;
#define EPTE_EXECUTE_ACCESS_BIT                                      2
#define EPTE_EXECUTE_ACCESS_FLAG                                     0x04
#define EPTE_EXECUTE_ACCESS_MASK                                     0x01
#define EPTE_EXECUTE_ACCESS(_)                                       (((_) >> 2) & 0x01)

	/**
	* [Bits 5:3] EPT memory type for this 4-KByte page.
	*
	* @see Vol3C[28.2.6(EPT and memory Typing)]
	*/
	uint64_t memory_type                                             : 3;
#define EPTE_MEMORY_TYPE_BIT                                         3
#define EPTE_MEMORY_TYPE_FLAG                                        0x38
#define EPTE_MEMORY_TYPE_MASK                                        0x07
#define EPTE_MEMORY_TYPE(_)                                          (((_) >> 3) & 0x07)

	/**
	* [Bit 6] Ignore PAT memory type for this 4-KByte page.
	*
	* @see Vol3C[28.2.6(EPT and memory Typing)]
	*/
	uint64_t ignore_pat                                              : 1;
#define EPTE_IGNORE_PAT_BIT                                          6
#define EPTE_IGNORE_PAT_FLAG                                         0x40
#define EPTE_IGNORE_PAT_MASK                                         0x01
#define EPTE_IGNORE_PAT(_)                                           (((_) >> 6) & 0x01)
	uint64_t reserved1                                               : 1;

	/**
	* [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 4-KByte page
	* referenced by this entry. Ignored if bit 6 of EPTP is 0.
	*
	* @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
	*/
	uint64_t accessed                                                : 1;
#define EPTE_ACCESSED_BIT                                            8
#define EPTE_ACCESSED_FLAG                                           0x100
#define EPTE_ACCESSED_MASK                                           0x01
#define EPTE_ACCESSED(_)                                             (((_) >> 8) & 0x01)

	/**
	* [Bit 9] If bit 6 of EPTP is 1, dirty flag for EPT; indicates whether software has written to the 4-KByte page referenced
	* by this entry. Ignored if bit 6 of EPTP is 0.
	*
	* @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
	*/
	uint64_t dirty                                                   : 1;
#define EPTE_DIRTY_BIT                                               9
#define EPTE_DIRTY_FLAG                                              0x200
#define EPTE_DIRTY_MASK                                              0x01
#define EPTE_DIRTY(_)                                                (((_) >> 9) & 0x01)

	/**
	* [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
	* is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 4-KByte page controlled
	* by this entry. If that control is 0, this bit is ignored.
	*/
	uint64_t user_mode_execute                                       : 1;
#define EPTE_USER_MODE_EXECUTE_BIT                                   10
#define EPTE_USER_MODE_EXECUTE_FLAG                                  0x400
#define EPTE_USER_MODE_EXECUTE_MASK                                  0x01
#define EPTE_USER_MODE_EXECUTE(_)                                    (((_) >> 10) & 0x01)
	uint64_t reserved2                                               : 1;

	/**
	* [Bits 47:12] Physical address of the 4-KByte page referenced by this entry.
	*/
	uint64_t page_frame_number                                       : 36;
#define EPTE_PAGE_FRAME_NUMBER_BIT                                   12
#define EPTE_PAGE_FRAME_NUMBER_FLAG                                  0xFFFFFFFFF000
#define EPTE_PAGE_FRAME_NUMBER_MASK                                  0xFFFFFFFFF
#define EPTE_PAGE_FRAME_NUMBER(_)                                    (((_) >> 12) & 0xFFFFFFFFF)
	uint64_t reserved3                                               : 15;

	/**
	* [Bit 63] Suppress \#VE. If the "EPT-violation \#VE" VM-execution control is 1, EPT violations caused by accesses to this
	* page are convertible to virtualization exceptions only if this bit is 0. If "EPT-violation \#VE" VMexecution control is
	* 0, this bit is ignored.
	*
	* @see Vol3C[25.5.6.1(Convertible EPT Violations)]
	*/
	uint64_t suppress_ve                                             : 1;
#define EPTE_SUPPRESS_VE_BIT                                         63
#define EPTE_SUPPRESS_VE_FLAG                                        0x8000000000000000
#define EPTE_SUPPRESS_VE_MASK                                        0x01
#define EPTE_SUPPRESS_VE(_)                                          (((_) >> 63) & 0x01)
  };

  uint64_t flags;
} epte;

/**
 * @brief Format of a common EPT Entry
 */
typedef union
{
  struct
  {
	uint64_t read_access                                             : 1;
#define EPT_ENTRY_READ_ACCESS_BIT                                    0
#define EPT_ENTRY_READ_ACCESS_FLAG                                   0x01
#define EPT_ENTRY_READ_ACCESS_MASK                                   0x01
#define EPT_ENTRY_READ_ACCESS(_)                                     (((_) >> 0) & 0x01)
	uint64_t write_access                                            : 1;
#define EPT_ENTRY_WRITE_ACCESS_BIT                                   1
#define EPT_ENTRY_WRITE_ACCESS_FLAG                                  0x02
#define EPT_ENTRY_WRITE_ACCESS_MASK                                  0x01
#define EPT_ENTRY_WRITE_ACCESS(_)                                    (((_) >> 1) & 0x01)
	uint64_t execute_access                                          : 1;
#define EPT_ENTRY_EXECUTE_ACCESS_BIT                                 2
#define EPT_ENTRY_EXECUTE_ACCESS_FLAG                                0x04
#define EPT_ENTRY_EXECUTE_ACCESS_MASK                                0x01
#define EPT_ENTRY_EXECUTE_ACCESS(_)                                  (((_) >> 2) & 0x01)
	uint64_t memory_type                                             : 3;
#define EPT_ENTRY_MEMORY_TYPE_BIT                                    3
#define EPT_ENTRY_MEMORY_TYPE_FLAG                                   0x38
#define EPT_ENTRY_MEMORY_TYPE_MASK                                   0x07
#define EPT_ENTRY_MEMORY_TYPE(_)                                     (((_) >> 3) & 0x07)
	uint64_t ignore_pat                                              : 1;
#define EPT_ENTRY_IGNORE_PAT_BIT                                     6
#define EPT_ENTRY_IGNORE_PAT_FLAG                                    0x40
#define EPT_ENTRY_IGNORE_PAT_MASK                                    0x01
#define EPT_ENTRY_IGNORE_PAT(_)                                      (((_) >> 6) & 0x01)
	uint64_t large_page                                              : 1;
#define EPT_ENTRY_LARGE_PAGE_BIT                                     7
#define EPT_ENTRY_LARGE_PAGE_FLAG                                    0x80
#define EPT_ENTRY_LARGE_PAGE_MASK                                    0x01
#define EPT_ENTRY_LARGE_PAGE(_)                                      (((_) >> 7) & 0x01)
	uint64_t accessed                                                : 1;
#define EPT_ENTRY_ACCESSED_BIT                                       8
#define EPT_ENTRY_ACCESSED_FLAG                                      0x100
#define EPT_ENTRY_ACCESSED_MASK                                      0x01
#define EPT_ENTRY_ACCESSED(_)                                        (((_) >> 8) & 0x01)
	uint64_t dirty                                                   : 1;
#define EPT_ENTRY_DIRTY_BIT                                          9
#define EPT_ENTRY_DIRTY_FLAG                                         0x200
#define EPT_ENTRY_DIRTY_MASK                                         0x01
#define EPT_ENTRY_DIRTY(_)                                           (((_) >> 9) & 0x01)
	uint64_t user_mode_execute                                       : 1;
#define EPT_ENTRY_USER_MODE_EXECUTE_BIT                              10
#define EPT_ENTRY_USER_MODE_EXECUTE_FLAG                             0x400
#define EPT_ENTRY_USER_MODE_EXECUTE_MASK                             0x01
#define EPT_ENTRY_USER_MODE_EXECUTE(_)                               (((_) >> 10) & 0x01)
	uint64_t reserved1                                               : 1;
	uint64_t page_frame_number                                       : 36;
#define EPT_ENTRY_PAGE_FRAME_NUMBER_BIT                              12
#define EPT_ENTRY_PAGE_FRAME_NUMBER_FLAG                             0xFFFFFFFFF000
#define EPT_ENTRY_PAGE_FRAME_NUMBER_MASK                             0xFFFFFFFFF
#define EPT_ENTRY_PAGE_FRAME_NUMBER(_)                               (((_) >> 12) & 0xFFFFFFFFF)
	uint64_t reserved2                                               : 15;
	uint64_t suppress_ve                                             : 1;
#define EPT_ENTRY_SUPPRESS_VE_BIT                                    63
#define EPT_ENTRY_SUPPRESS_VE_FLAG                                   0x8000000000000000
#define EPT_ENTRY_SUPPRESS_VE_MASK                                   0x01
#define EPT_ENTRY_SUPPRESS_VE(_)                                     (((_) >> 63) & 0x01)
  };

  uint64_t flags;
} ept_entry;

/**
 * @defgroup ept_table_level \
 *           EPT Table level numbers
 *
 * EPT Table level numbers.
 * @{
 */
#define EPT_LEVEL_PML4E                                              0x00000003
#define EPT_LEVEL_PDPTE                                              0x00000002
#define EPT_LEVEL_PDE                                                0x00000001
#define EPT_LEVEL_PTE                                                0x00000000
/**
 * @}
 */

/**
 * @defgroup ept_entry_count \
 *           EPT Entry counts
 *
 * EPT Entry counts.
 * @{
 */
#define EPT_PML4E_ENTRY_COUNT                                        0x00000200
#define EPT_PDPTE_ENTRY_COUNT                                        0x00000200
#define EPT_PDE_ENTRY_COUNT                                          0x00000200
#define EPT_PTE_ENTRY_COUNT                                          0x00000200
/**
 * @}
 */

/**
 * @}
 */

typedef enum
{
  /**
	* If the INVEPT type is 1, the logical processor invalidates all guest-physical mappings and combined mappings associated
	* with the EP4TA specified in the INVEPT descriptor. Combined mappings for that EP4TA are invalidated for all VPIDs and
	* all PCIDs. (The instruction may invalidate mappings associated with other EP4TAs.)
	*/
  invept_single_context                                        = 0x00000001,

  /**
	* If the INVEPT type is 2, the logical processor invalidates guest-physical mappings and combined mappings associated with
	* all EP4TAs (and, for combined mappings, for all VPIDs and PCIDs).
	*/
  invept_all_context                                           = 0x00000002,
} invept_type;

typedef enum
{
  /**
	* If the INVVPID type is 0, the logical processor invalidates linear mappings and combined mappings associated with the
	* VPID specified in the INVVPID descriptor and that would be used to translate the linear address specified in of the
	* INVVPID descriptor. Linear mappings and combined mappings for that VPID and linear address are invalidated for all PCIDs
	* and, for combined mappings, all EP4TAs. (The instruction may also invalidate mappings associated with other VPIDs and
	* for other linear addresses).
	*/
  invvpid_individual_address                                   = 0x00000000,

  /**
	* If the INVVPID type is 1, the logical processor invalidates all linear mappings and combined mappings associated with
	* the VPID specified in the INVVPID descriptor. Linear mappings and combined mappings for that VPID are invalidated for
	* all PCIDs and, for combined mappings, all EP4TAs. (The instruction may also invalidate mappings associated with other
	* VPIDs).
	*/
  invvpid_single_context                                       = 0x00000001,

  /**
	* If the INVVPID type is 2, the logical processor invalidates linear mappings and combined mappings associated with all
	* VPIDs except VPID 0000H and with all PCIDs. (The instruction may also invalidate linear mappings with VPID 0000H.)
	* Combined mappings are invalidated for all EP4TAs.
	*/
  invvpid_all_context                                          = 0x00000002,

  /**
	* If the INVVPID type is 3, the logical processor invalidates linear mappings and combined mappings associated with the
	* VPID specified in the INVVPID descriptor. Linear mappings and combined mappings for that VPID are invalidated for all
	* PCIDs and, for combined mappings, all EP4TAs. The logical processor is not required to invalidate information that was
	* used for global translations (although it may do so). (The instruction may also invalidate mappings associated with
	* other VPIDs).
	*
	* @see Vol3C[4.10(Caching Translation Information)]
	*/
  invvpid_single_context_retaining_globals                     = 0x00000003,
} invvpid_type;

typedef struct
{
  uint64_t ept_pointer;

  /**
	* Must be zero.
	*/
  uint64_t reserved;
} invept_descriptor;

typedef struct
{
  uint16_t vpid;

  /**
	* Must be zero.
	*/
  uint16_t reserved1;

  /**
	* Must be zero.
	*/
  uint32_t reserved2;
  uint64_t linear_address;
} invvpid_descriptor;

/**
 * @brief Format of the VMCS Region
 *
 * A logical processor uses virtual-machine control data structures (VMCSs) while it is in VMX operation. These manage
 * transitions into and out of VMX non-root operation (VM entries and VM exits) as well as processor behavior in VMX
 * non-root operation. This structure is manipulated by the new instructions VMCLEAR, VMPTRLD, VMREAD, and VMWRITE.
 * A VMCS region comprises up to 4-KBytes. The exact size is implementation specific and can be determined by consulting
 * the VMX capability MSR IA32_VMX_BASIC.
 *
 * @see Vol3C[24.2(FORMAT OF THE VMCS REGION)] (reference)
 */
typedef struct
{
  struct
  {
	/**
	* @brief VMCS revision identifier
	*
	* [Bits 30:0] Processors that maintain VMCS data in different formats (see below) use different VMCS revision identifiers.
	* These identifiers enable software to avoid using a VMCS region formatted for one processor on a processor that uses a
	* different format.
	* Software should write the VMCS revision identifier to the VMCS region before using that region for a VMCS. The VMCS
	* revision identifier is never written by the processor; VMPTRLD fails if its operand references a VMCS region whose VMCS
	* revision identifier differs from that used by the processor.
	* Software can discover the VMCS revision identifier that a processor uses by reading the VMX capability MSR
	* IA32_VMX_BASIC.
	*
	* @see Vol3C[24.6.2(Processor-Based VM-Execution Controls)]
	* @see Vol3D[A.1(BASIC VMX INFORMATION)]
	*/
	uint32_t revision_id                                             : 31;

	/**
	* @brief Shadow-VMCS indicator
	*
	* [Bit 31] Software should clear or set the shadow-VMCS indicator depending on whether the VMCS is to be an ordinary VMCS
	* or a shadow VMCS. VMPTRLD fails if the shadow-VMCS indicator is set and the processor does not support the 1-setting of
	* the "VMCS shadowing" VM-execution control. Software can discover support for this setting by reading the VMX capability
	* MSR IA32_VMX_PROCBASED_CTLS2.
	*
	* @see Vol3C[24.10(VMCS TYPES ORDINARY AND SHADOW)]
	*/
	uint32_t shadow_vmcs_indicator                                   : 1;
  };


  /**
	* @brief VMX-abort indicator
	*
	* The contents of these bits do not control processor operation in any way. A logical processor writes a non-zero value
	* into these bits if a VMX abort occurs. Software may also write into this field.
	*
	* @see Vol3D[27.7(VMX Aborts)]
	*/
  uint32_t abort_indicator;

  /**
	* @brief VMCS data (implementation-specific format)
	*
	* These parts of the VMCS control VMX non-root operation and the VMX transitions.
	* The format of these data is implementation-specific. To ensure proper behavior in VMX operation, software should
	* maintain the VMCS region and related structures in writeback cacheable memory. Future implementations may allow or
	* require a different memory type. Software should consult the VMX capability MSR IA32_VMX_BASIC.
	*
	* @see Vol3C[24.11.4(Software Access to Related Structures)]
	* @see Vol3D[A.1(BASIC VMX INFORMATION)]
	*/
  uint8_t data[4088];
} vmcs;

/**
 * @brief Format of the VMXON Region
 *
 * Before executing VMXON, software allocates a region of memory that the logical processor uses to support VMX operation.
 * This region is called the VMXON region.
 * A VMXON region comprises up to 4-KBytes. The exact size is implementation specific and can be determined by consulting
 * the VMX capability MSR IA32_VMX_BASIC.
 *
 * @see Vol3C[24.11.5(VMXON Region)] (reference)
 */
typedef struct
{
  struct
  {
	/**
	* @brief VMCS revision identifier
	*
	* [Bits 30:0] Before executing VMXON, software should write the VMCS revision identifier to the VMXON region.
	* (Specifically, it should write the 31-bit VMCS revision identifier to bits 30:0 of the first 4 bytes of the VMXON
	* region; bit 31 should be cleared to 0.)
	*
	* @see VMCS
	* @see Vol3C[24.2(FORMAT OF THE VMCS REGION)]
	* @see Vol3C[24.11.5(VMXON Region)]
	*/
	uint32_t revision_id                                             : 31;

	/**
	* [Bit 31] Bit 31 is always 0.
	*/
	uint32_t must_be_zero                                            : 1;
  };


  /**
	* @brief VMXON data (implementation-specific format)
	*
	* The format of these data is implementation-specific. To ensure proper behavior in VMX operation, software should not
	* access or modify the VMXON region of a logical processor between execution of VMXON and VMXOFF on that logical
	* processor. Doing otherwise may lead to unpredictable behavior.
	*
	* @see Vol3C[24.11.4(Software Access to Related Structures)]
	* @see Vol3D[A.1(BASIC VMX INFORMATION)]
	*/
  uint8_t data[4092];
} vmxon;

/**
 * @defgroup vmcs_fields \
 *           VMCS (VM Control Structure)
 *
 * Every component of the VMCS is encoded by a 32-bit field that can be used by VMREAD and VMWRITE. This enumerates all
 * fields in the VMCS and their encodings. Fields are grouped by width (16-bit, 32-bit, etc.) and type (guest-state,
 * host-state, etc.).
 *
 * @see Vol3D[B(APPENDIX B FIELD ENCODING IN VMCS)] (reference)
 * @{
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] Access type (0 = full; 1 = high); must be full for 16-bit, 32-bit, and natural-width fields.
	*/
	uint16_t access_type                                             : 1;
#define VMCS_COMPONENT_ENCODING_ACCESS_TYPE_BIT                      0
#define VMCS_COMPONENT_ENCODING_ACCESS_TYPE_FLAG                     0x01
#define VMCS_COMPONENT_ENCODING_ACCESS_TYPE_MASK                     0x01
#define VMCS_COMPONENT_ENCODING_ACCESS_TYPE(_)                       (((_) >> 0) & 0x01)

	/**
	* [Bits 9:1] Index.
	*/
	uint16_t index                                                   : 9;
#define VMCS_COMPONENT_ENCODING_INDEX_BIT                            1
#define VMCS_COMPONENT_ENCODING_INDEX_FLAG                           0x3FE
#define VMCS_COMPONENT_ENCODING_INDEX_MASK                           0x1FF
#define VMCS_COMPONENT_ENCODING_INDEX(_)                             (((_) >> 1) & 0x1FF)

	/**
	* [Bits 11:10] Type:
	* 0: control
	* 1: VM-exit information
	* 2: guest state
	* 3: host state
	*/
	uint16_t type                                                    : 2;
#define VMCS_COMPONENT_ENCODING_TYPE_BIT                             10
#define VMCS_COMPONENT_ENCODING_TYPE_FLAG                            0xC00
#define VMCS_COMPONENT_ENCODING_TYPE_MASK                            0x03
#define VMCS_COMPONENT_ENCODING_TYPE(_)                              (((_) >> 10) & 0x03)

	/**
	* [Bit 12] Reserved (must be 0).
	*/
	uint16_t must_be_zero                                            : 1;
#define VMCS_COMPONENT_ENCODING_MUST_BE_ZERO_BIT                     12
#define VMCS_COMPONENT_ENCODING_MUST_BE_ZERO_FLAG                    0x1000
#define VMCS_COMPONENT_ENCODING_MUST_BE_ZERO_MASK                    0x01
#define VMCS_COMPONENT_ENCODING_MUST_BE_ZERO(_)                      (((_) >> 12) & 0x01)

	/**
	* [Bits 14:13] Width:
	* 0: 16-bit
	* 1: 64-bit
	* 2: 32-bit
	* 3: natural-width
	*/
	uint16_t width                                                   : 2;
#define VMCS_COMPONENT_ENCODING_WIDTH_BIT                            13
#define VMCS_COMPONENT_ENCODING_WIDTH_FLAG                           0x6000
#define VMCS_COMPONENT_ENCODING_WIDTH_MASK                           0x03
#define VMCS_COMPONENT_ENCODING_WIDTH(_)                             (((_) >> 13) & 0x03)
	uint16_t reserved1                                               : 1;
  };

  uint16_t flags;
} vmcs_component_encoding;

/**
 * @defgroup vmcs_16_bit \
 *           16-Bit Fields
 *
 * 16-Bit Fields.
 *
 * @see Vol3D[B.1(16-BIT FIELDS)] (reference)
 * @{
 */
/**
 * @defgroup vmcs_16_bit_control_fields \
 *           16-Bit Control Fields
 *
 * 16-Bit Control Fields.
 * @{
 */
/**
 * Virtual-processor identifier (VPID).
 *
 * @remarks This field exists only on processors that support the 1-setting of the "enable VPID" VM-execution control.
 */
#define VMCS_CTRL_VIRTUAL_PROCESSOR_IDENTIFIER                       0x00000000

/**
 * Posted-interrupt notification vector.
 *
 * @remarks This field exists only on processors that support the 1-setting of the "process posted interrupts" VM-execution
 *          control.
 */
#define VMCS_CTRL_POSTED_INTERRUPT_NOTIFICATION_VECTOR               0x00000002

/**
 * EPTP index.
 *
 * @remarks This field exists only on processors that support the 1-setting of the "EPT-violation \#VE" VM-execution
 *          control.
 */
#define VMCS_CTRL_EPTP_INDEX                                         0x00000004
/**
 * @}
 */

/**
 * @defgroup vmcs_16_bit_guest_state_fields \
 *           16-Bit Guest-State Fields
 *
 * 16-Bit Guest-State Fields.
 * @{
 */
/**
 * Guest ES selector.
 */
#define VMCS_GUEST_ES_SELECTOR                                       0x00000800

/**
 * Guest CS selector.
 */
#define VMCS_GUEST_CS_SELECTOR                                       0x00000802

/**
 * Guest SS selector.
 */
#define VMCS_GUEST_SS_SELECTOR                                       0x00000804

/**
 * Guest DS selector.
 */
#define VMCS_GUEST_DS_SELECTOR                                       0x00000806

/**
 * Guest FS selector.
 */
#define VMCS_GUEST_FS_SELECTOR                                       0x00000808

/**
 * Guest GS selector.
 */
#define VMCS_GUEST_GS_SELECTOR                                       0x0000080A

/**
 * Guest LDTR selector.
 */
#define VMCS_GUEST_LDTR_SELECTOR                                     0x0000080C

/**
 * Guest TR selector.
 */
#define VMCS_GUEST_TR_SELECTOR                                       0x0000080E

/**
 * Guest interrupt status.
 *
 * @remarks This field exists only on processors that support the 1-setting of the "virtual-interrupt delivery"
 *          VM-execution control.
 */
#define VMCS_GUEST_INTERRUPT_STATUS                                  0x00000810

/**
 * PML index.
 *
 * @remarks This field exists only on processors that support the 1-setting of the "enable PML" VM-execution control.
 */
#define VMCS_GUEST_PML_INDEX                                         0x00000812
/**
 * @}
 */

/**
 * @defgroup vmcs_16_bit_host_state_fields \
 *           16-Bit Host-State Fields
 *
 * 16-Bit Host-State Fields.
 * @{
 */
/**
 * Host ES selector.
 */
#define VMCS_HOST_ES_SELECTOR                                        0x00000C00

/**
 * Host CS selector.
 */
#define VMCS_HOST_CS_SELECTOR                                        0x00000C02

/**
 * Host SS selector.
 */
#define VMCS_HOST_SS_SELECTOR                                        0x00000C04

/**
 * Host DS selector.
 */
#define VMCS_HOST_DS_SELECTOR                                        0x00000C06

/**
 * Host FS selector.
 */
#define VMCS_HOST_FS_SELECTOR                                        0x00000C08

/**
 * Host GS selector.
 */
#define VMCS_HOST_GS_SELECTOR                                        0x00000C0A

/**
 * Host TR selector.
 */
#define VMCS_HOST_TR_SELECTOR                                        0x00000C0C
/**
 * @}
 */

/**
 * @}
 */

/**
 * @defgroup vmcs_64_bit \
 *           64-Bit Fields
 *
 * 64-Bit Fields.
 *
 * @see Vol3D[B.2(64-BIT FIELDS)] (reference)
 * @{
 */
/**
 * @defgroup vmcs_64_bit_control_fields \
 *           64-Bit Control Fields
 *
 * 64-Bit Control Fields.
 * @{
 */
/**
 * Address of I/O bitmap A.
 */
#define VMCS_CTRL_IO_BITMAP_A_ADDRESS                                0x00002000

/**
 * Address of I/O bitmap B.
 */
#define VMCS_CTRL_IO_BITMAP_B_ADDRESS                                0x00002002

/**
 * Address of MSR bitmaps.
 */
#define VMCS_CTRL_MSR_BITMAP_ADDRESS                                 0x00002004

/**
 * VM-exit MSR-store address.
 */
#define VMCS_CTRL_VMEXIT_MSR_STORE_ADDRESS                           0x00002006

/**
 * VM-exit MSR-load address.
 */
#define VMCS_CTRL_VMEXIT_MSR_LOAD_ADDRESS                            0x00002008

/**
 * VM-entry MSR-load address.
 */
#define VMCS_CTRL_VMENTRY_MSR_LOAD_ADDRESS                           0x0000200A

/**
 * Executive-VMCS pointer.
 */
#define VMCS_CTRL_EXECUTIVE_VMCS_POINTER                             0x0000200C

/**
 * PML address.
 */
#define VMCS_CTRL_PML_ADDRESS                                        0x0000200E

/**
 * TSC offset.
 */
#define VMCS_CTRL_TSC_OFFSET                                         0x00002010

/**
 * Virtual-APIC address.
 */
#define VMCS_CTRL_VIRTUAL_APIC_ADDRESS                               0x00002012

/**
 * APIC-access address.
 */
#define VMCS_CTRL_APIC_ACCESS_ADDRESS                                0x00002014

/**
 * Posted-interrupt descriptor address
 */
#define VMCS_CTRL_POSTED_INTERRUPT_DESCRIPTOR_ADDRESS                0x00002016

/**
 * VM-function controls.
 */
#define VMCS_CTRL_VMFUNC_CONTROLS                                    0x00002018

/**
 * EPT pointer.
 */
#define VMCS_CTRL_EPT_POINTER                                        0x0000201A

/**
 * EOI-exit bitmap 0.
 */
#define VMCS_CTRL_EOI_EXIT_BITMAP_0                                  0x0000201C

/**
 * EOI-exit bitmap 1.
 */
#define VMCS_CTRL_EOI_EXIT_BITMAP_1                                  0x0000201E

/**
 * EOI-exit bitmap 2.
 */
#define VMCS_CTRL_EOI_EXIT_BITMAP_2                                  0x00002020

/**
 * EOI-exit bitmap 3.
 */
#define VMCS_CTRL_EOI_EXIT_BITMAP_3                                  0x00002022

/**
 * EPTP-list address.
 */
#define VMCS_CTRL_EPT_POINTER_LIST_ADDRESS                           0x00002024

/**
 * VMREAD-bitmap address.
 */
#define VMCS_CTRL_VMREAD_BITMAP_ADDRESS                              0x00002026

/**
 * VMWRITE-bitmap address.
 */
#define VMCS_CTRL_VMWRITE_BITMAP_ADDRESS                             0x00002028

/**
 * Virtualization-exception information address.
 */
#define VMCS_CTRL_VIRTUALIZATION_EXCEPTION_INFORMATION_ADDRESS       0x0000202A

/**
 * XSS-exiting bitmap.
 */
#define VMCS_CTRL_XSS_EXITING_BITMAP                                 0x0000202C

/**
 * ENCLS-exiting bitmap.
 */
#define VMCS_CTRL_ENCLS_EXITING_BITMAP                               0x0000202E

/**
 * TSC multiplier.
 */
#define VMCS_CTRL_TSC_MULTIPLIER                                     0x00002032
/**
 * @}
 */

/**
 * @defgroup vmcs_64_bit_read_only_data_fields \
 *           64-Bit Read-Only Data Field
 *
 * 64-Bit Read-Only Data Field.
 * @{
 */
/**
 * Guest-physical address.
 */
#define VMCS_GUEST_PHYSICAL_ADDRESS                                  0x00002400
/**
 * @}
 */

/**
 * @defgroup vmcs_64_bit_guest_state_fields \
 *           64-Bit Guest-State Fields
 *
 * 64-Bit Guest-State Fields.
 * @{
 */
/**
 * VMCS link pointer.
 */
#define VMCS_GUEST_VMCS_LINK_POINTER                                 0x00002800

/**
 * Guest IA32_DEBUGCTL.
 */
#define VMCS_GUEST_DEBUGCTL                                          0x00002802

/**
 * Guest IA32_PAT.
 */
#define VMCS_GUEST_PAT                                               0x00002804

/**
 * Guest IA32_EFER.
 */
#define VMCS_GUEST_EFER                                              0x00002806

/**
 * Guest IA32_PERF_GLOBAL_CTRL.
 */
#define VMCS_GUEST_PERF_GLOBAL_CTRL                                  0x00002808

/**
 * Guest PDPTE0.
 */
#define VMCS_GUEST_PDPTE0                                            0x0000280A

/**
 * Guest PDPTE1.
 */
#define VMCS_GUEST_PDPTE1                                            0x0000280C

/**
 * Guest PDPTE2.
 */
#define VMCS_GUEST_PDPTE2                                            0x0000280E

/**
 * Guest PDPTE3.
 */
#define VMCS_GUEST_PDPTE3                                            0x00002810

/**
 * Guest IA32_BNDCFGS.
 */
#define VMCS_GUEST_BNDCFGS                                           0x00002812

/**
 * Guest IA32_RTIT_CTL.
 */
#define VMCS_GUEST_RTIT_CTL                                          0x00002814
/**
 * @}
 */

/**
 * @defgroup vmcs_64_bit_host_state_fields \
 *           64-Bit Host-State Fields
 *
 * 64-Bit Host-State Fields.
 * @{
 */
/**
 * Host IA32_PAT.
 */
#define VMCS_HOST_PAT                                                0x00002C00

/**
 * Host IA32_EFER.
 */
#define VMCS_HOST_EFER                                               0x00002C02

/**
 * Host IA32_PERF_GLOBAL_CTRL.
 */
#define VMCS_HOST_PERF_GLOBAL_CTRL                                   0x00002C04
/**
 * @}
 */

/**
 * @}
 */

/**
 * @defgroup vmcs_32_bit \
 *           32-Bit Fields
 *
 * 32-Bit Fields.
 *
 * @see Vol3D[B.3(32-BIT FIELDS)] (reference)
 * @{
 */
/**
 * @defgroup vmcs_32_bit_control_fields \
 *           32-Bit Control Fields
 *
 * 32-Bit Control Fields.
 * @{
 */
/**
 * Pin-based VM-execution controls.
 */
#define VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS                    0x00004000

/**
 * Primary processor-based VM-execution controls.
 */
#define VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS              0x00004002

/**
 * Exception bitmap.
 */
#define VMCS_CTRL_EXCEPTION_BITMAP                                   0x00004004

/**
 * Page-fault error-code mask.
 */
#define VMCS_CTRL_PAGEFAULT_ERROR_CODE_MASK                          0x00004006

/**
 * Page-fault error-code match.
 */
#define VMCS_CTRL_PAGEFAULT_ERROR_CODE_MATCH                         0x00004008

/**
 * CR3-target count.
 */
#define VMCS_CTRL_CR3_TARGET_COUNT                                   0x0000400A

/**
 * VM-exit controls.
 */
#define VMCS_CTRL_VMEXIT_CONTROLS                                    0x0000400C

/**
 * VM-exit MSR-store count.
 */
#define VMCS_CTRL_VMEXIT_MSR_STORE_COUNT                             0x0000400E

/**
 * VM-exit MSR-load count.
 */
#define VMCS_CTRL_VMEXIT_MSR_LOAD_COUNT                              0x00004010

/**
 * VM-entry controls.
 */
#define VMCS_CTRL_VMENTRY_CONTROLS                                   0x00004012

/**
 * VM-entry MSR-load count.
 */
#define VMCS_CTRL_VMENTRY_MSR_LOAD_COUNT                             0x00004014

/**
 * VM-entry interruption-information field.
 */
#define VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD             0x00004016

/**
 * VM-entry exception error code.
 */
#define VMCS_CTRL_VMENTRY_EXCEPTION_ERROR_CODE                       0x00004018

/**
 * VM-entry instruction length.
 */
#define VMCS_CTRL_VMENTRY_INSTRUCTION_LENGTH                         0x0000401A

/**
 * TPR threshold.
 */
#define VMCS_CTRL_TPR_THRESHOLD                                      0x0000401C

/**
 * Secondary processor-based VM-execution controls.
 */
#define VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS    0x0000401E

/**
 * PLE_Gap.
 */
#define VMCS_CTRL_PLE_GAP                                            0x00004020

/**
 * PLE_Window.
 */
#define VMCS_CTRL_PLE_WINDOW                                         0x00004022
/**
 * @}
 */

/**
 * @defgroup vmcs_32_bit_read_only_data_fields \
 *           32-Bit Read-Only Data Fields
 *
 * 32-Bit Read-Only Data Fields.
 * @{
 */
/**
 * VM-instruction error.
 */
#define VMCS_VM_INSTRUCTION_ERROR                                    0x00004400

/**
 * Exit reason.
 */
#define VMCS_EXIT_REASON                                             0x00004402

/**
 * VM-exit interruption information.
 */
#define VMCS_VMEXIT_INTERRUPTION_INFORMATION                         0x00004404

/**
 * VM-exit interruption error code.
 */
#define VMCS_VMEXIT_INTERRUPTION_ERROR_CODE                          0x00004406

/**
 * IDT-vectoring information field.
 */
#define VMCS_IDT_VECTORING_INFORMATION                               0x00004408

/**
 * IDT-vectoring error code.
 */
#define VMCS_IDT_VECTORING_ERROR_CODE                                0x0000440A

/**
 * VM-exit instruction length.
 */
#define VMCS_VMEXIT_INSTRUCTION_LENGTH                               0x0000440C

/**
 * VM-exit instruction information.
 */
#define VMCS_VMEXIT_INSTRUCTION_INFO                                 0x0000440E
/**
 * @}
 */

/**
 * @defgroup vmcs_32_bit_guest_state_fields \
 *           32-Bit Guest-State Fields
 *
 * 32-Bit Guest-State Fields.
 * @{
 */
/**
 * Guest ES limit.
 */
#define VMCS_GUEST_ES_LIMIT                                          0x00004800

/**
 * Guest CS limit.
 */
#define VMCS_GUEST_CS_LIMIT                                          0x00004802

/**
 * Guest SS limit.
 */
#define VMCS_GUEST_SS_LIMIT                                          0x00004804

/**
 * Guest DS limit.
 */
#define VMCS_GUEST_DS_LIMIT                                          0x00004806

/**
 * Guest FS limit.
 */
#define VMCS_GUEST_FS_LIMIT                                          0x00004808

/**
 * Guest GS limit.
 */
#define VMCS_GUEST_GS_LIMIT                                          0x0000480A

/**
 * Guest LDTR limit.
 */
#define VMCS_GUEST_LDTR_LIMIT                                        0x0000480C

/**
 * Guest TR limit.
 */
#define VMCS_GUEST_TR_LIMIT                                          0x0000480E

/**
 * Guest GDTR limit.
 */
#define VMCS_GUEST_GDTR_LIMIT                                        0x00004810

/**
 * Guest IDTR limit.
 */
#define VMCS_GUEST_IDTR_LIMIT                                        0x00004812

/**
 * Guest ES access rights.
 */
#define VMCS_GUEST_ES_ACCESS_RIGHTS                                  0x00004814

/**
 * Guest CS access rights.
 */
#define VMCS_GUEST_CS_ACCESS_RIGHTS                                  0x00004816

/**
 * Guest SS access rights.
 */
#define VMCS_GUEST_SS_ACCESS_RIGHTS                                  0x00004818

/**
 * Guest DS access rights.
 */
#define VMCS_GUEST_DS_ACCESS_RIGHTS                                  0x0000481A

/**
 * Guest FS access rights.
 */
#define VMCS_GUEST_FS_ACCESS_RIGHTS                                  0x0000481C

/**
 * Guest GS access rights.
 */
#define VMCS_GUEST_GS_ACCESS_RIGHTS                                  0x0000481E

/**
 * Guest LDTR access rights.
 */
#define VMCS_GUEST_LDTR_ACCESS_RIGHTS                                0x00004820

/**
 * Guest TR access rights.
 */
#define VMCS_GUEST_TR_ACCESS_RIGHTS                                  0x00004822

/**
 * Guest interruptibility state.
 */
#define VMCS_GUEST_INTERRUPTIBILITY_STATE                            0x00004824

/**
 * Guest activity state.
 */
#define VMCS_GUEST_ACTIVITY_STATE                                    0x00004826

/**
 * Guest SMBASE.
 */
#define VMCS_GUEST_SMBASE                                            0x00004828

/**
 * Guest IA32_SYSENTER_CS.
 */
#define VMCS_GUEST_SYSENTER_CS                                       0x0000482A

/**
 * VMX-preemption timer value.
 */
#define VMCS_GUEST_VMX_PREEMPTION_TIMER_VALUE                        0x0000482E
/**
 * @}
 */

/**
 * @defgroup vmcs_32_bit_host_state_fields \
 *           32-Bit Host-State Field
 *
 * 32-Bit Host-State Field.
 * @{
 */
/**
 * Host IA32_SYSENTER_CS.
 */
#define VMCS_HOST_SYSENTER_CS                                        0x00004C00
/**
 * @}
 */

/**
 * @}
 */

/**
 * @defgroup vmcs_natural_width \
 *           Natural-Width Fields
 *
 * Natural-Width Fields.
 *
 * @see Vol3D[B.4(NATURAL-WIDTH FIELDS)] (reference)
 * @{
 */
/**
 * @defgroup vmcs_natural_width_control_fields \
 *           Natural-Width Control Fields
 *
 * Natural-Width Control Fields
 * @{
 */
/**
 * CR0 guest/host mask.
 */
#define VMCS_CTRL_CR0_GUEST_HOST_MASK                                0x00006000

/**
 * CR4 guest/host mask.
 */
#define VMCS_CTRL_CR4_GUEST_HOST_MASK                                0x00006002

/**
 * CR0 read shadow.
 */
#define VMCS_CTRL_CR0_READ_SHADOW                                    0x00006004

/**
 * CR4 read shadow.
 */
#define VMCS_CTRL_CR4_READ_SHADOW                                    0x00006006

/**
 * CR3-target value 0.
 */
#define VMCS_CTRL_CR3_TARGET_VALUE_0                                 0x00006008

/**
 * CR3-target value 1.
 */
#define VMCS_CTRL_CR3_TARGET_VALUE_1                                 0x0000600A

/**
 * CR3-target value 2.
 */
#define VMCS_CTRL_CR3_TARGET_VALUE_2                                 0x0000600C

/**
 * CR3-target value 3.
 */
#define VMCS_CTRL_CR3_TARGET_VALUE_3                                 0x0000600E
/**
 * @}
 */

/**
 * @defgroup vmcs_natural_width_read_only_data_fields \
 *           Natural-Width Read-Only Data Fields
 *
 * Natural-Width Read-Only Data Fields.
 * @{
 */
/**
 * Exit qualification.
 */
#define VMCS_EXIT_QUALIFICATION                                      0x00006400

/**
 * I/O RCX.
 */
#define VMCS_IO_RCX                                                  0x00006402

/**
 * I/O RSI.
 */
#define VMCS_IO_RSX                                                  0x00006404

/**
 * I/O RDI.
 */
#define VMCS_IO_RDI                                                  0x00006406

/**
 * I/O RIP.
 */
#define VMCS_IO_RIP                                                  0x00006408

/**
 * Guest-linear address.
 */
#define VMCS_EXIT_GUEST_LINEAR_ADDRESS                               0x0000640A
/**
 * @}
 */

/**
 * @defgroup vmcs_natural_width_guest_state_fields \
 *           Natural-Width Guest-State Fields
 *
 * Natural-Width Guest-State Fields.
 * @{
 */
/**
 * Guest CR0.
 */
#define VMCS_GUEST_CR0                                               0x00006800

/**
 * Guest CR3.
 */
#define VMCS_GUEST_CR3                                               0x00006802

/**
 * Guest CR4.
 */
#define VMCS_GUEST_CR4                                               0x00006804

/**
 * Guest ES base.
 */
#define VMCS_GUEST_ES_BASE                                           0x00006806

/**
 * Guest CS base.
 */
#define VMCS_GUEST_CS_BASE                                           0x00006808

/**
 * Guest SS base.
 */
#define VMCS_GUEST_SS_BASE                                           0x0000680A

/**
 * Guest DS base.
 */
#define VMCS_GUEST_DS_BASE                                           0x0000680C

/**
 * Guest FS base.
 */
#define VMCS_GUEST_FS_BASE                                           0x0000680E

/**
 * Guest GS base.
 */
#define VMCS_GUEST_GS_BASE                                           0x00006810

/**
 * Guest LDTR base.
 */
#define VMCS_GUEST_LDTR_BASE                                         0x00006812

/**
 * Guest TR base.
 */
#define VMCS_GUEST_TR_BASE                                           0x00006814

/**
 * Guest GDTR base.
 */
#define VMCS_GUEST_GDTR_BASE                                         0x00006816

/**
 * Guest IDTR base.
 */
#define VMCS_GUEST_IDTR_BASE                                         0x00006818

/**
 * Guest DR7.
 */
#define VMCS_GUEST_DR7                                               0x0000681A

/**
 * Guest RSP.
 */
#define VMCS_GUEST_RSP                                               0x0000681C

/**
 * Guest RIP.
 */
#define VMCS_GUEST_RIP                                               0x0000681E

/**
 * Guest RFLAGS.
 */
#define VMCS_GUEST_RFLAGS                                            0x00006820

/**
 * Guest pending debug exceptions.
 */
#define VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS                          0x00006822

/**
 * Guest IA32_SYSENTER_ESP.
 */
#define VMCS_GUEST_SYSENTER_ESP                                      0x00006824

/**
 * Guest IA32_SYSENTER_EIP.
 */
#define VMCS_GUEST_SYSENTER_EIP                                      0x00006826

/**
 * Guest IA32_S_CET.
 */
#define VMCS_GUEST_S_CET                                             0x00006C28

/**
 * Guest SSP.
 */
#define VMCS_GUEST_SSP                                               0x00006C2A

/**
 * Guest IA32_INTERRUPT_SSP_TABLE_ADDR.
 */
#define VMCS_GUEST_INTERRUPT_SSP_TABLE_ADDR                          0x00006C2C
/**
 * @}
 */

/**
 * @defgroup vmcs_natural_width_host_state_fields \
 *           Natural-Width Host-State Fields
 *
 * Natural-Width Host-State Fields.
 * @{
 */
/**
 * Host CR0.
 */
#define VMCS_HOST_CR0                                                0x00006C00

/**
 * Host CR3.
 */
#define VMCS_HOST_CR3                                                0x00006C02

/**
 * Host CR4.
 */
#define VMCS_HOST_CR4                                                0x00006C04

/**
 * Host FS base.
 */
#define VMCS_HOST_FS_BASE                                            0x00006C06

/**
 * Host GS base.
 */
#define VMCS_HOST_GS_BASE                                            0x00006C08

/**
 * Host TR base.
 */
#define VMCS_HOST_TR_BASE                                            0x00006C0A

/**
 * Host GDTR base.
 */
#define VMCS_HOST_GDTR_BASE                                          0x00006C0C

/**
 * Host IDTR base.
 */
#define VMCS_HOST_IDTR_BASE                                          0x00006C0E

/**
 * Host IA32_SYSENTER_ESP.
 */
#define VMCS_HOST_SYSENTER_ESP                                       0x00006C10

/**
 * Host IA32_SYSENTER_EIP.
 */
#define VMCS_HOST_SYSENTER_EIP                                       0x00006C12

/**
 * Host RSP.
 */
#define VMCS_HOST_RSP                                                0x00006C14

/**
 * Host RIP.
 */
#define VMCS_HOST_RIP                                                0x00006C16

/**
 * Host IA32_S_CET.
 */
#define VMCS_HOST_S_CET                                              0x00006C18

/**
 * Host SSP.
 */
#define VMCS_HOST_SSP                                                0x00006C1A

/**
 * Host IA32_INTERRUPT_SSP_TABLE_ADDR.
 */
#define VMCS_HOST_INTERRUPT_SSP_TABLE_ADDR                           0x00006C1C
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @brief Valid interruption types
 */
typedef enum
{
  /**
	* External interrupt.
	*/
  external_interrupt                                           = 0x00000000,

  /**
	* Non-maskable interrupt (NMI).
	*/
  non_maskable_interrupt                                       = 0x00000002,

  /**
	* Hardware exception (e.g,. \#PF).
	*/
  hardware_exception                                           = 0x00000003,

  /**
	* Software interrupt (INT n).
	*/
  software_interrupt                                           = 0x00000004,

  /**
	* Privileged software exception (INT1).
	*/
  privileged_software_exception                                = 0x00000005,

  /**
	* Software exception (INT3 or INTO).
	*/
  software_exception                                           = 0x00000006,

  /**
	* Other event. This type is used for injection of events that are not delivered through the IDT.
	*/
  other_event                                                  = 0x00000007,
} interruption_type;

/**
 * @brief VM entry can be configured to conclude by delivering an event through the IDT (after all guest state and MSRs
 *        have been loaded). This process is called event injection and is controlled by these VM-entry control fields
 *
 * @see Vol3A[24.8.3(VM-Entry Controls for Event Injection)] (reference)
 */
typedef union
{
  struct
  {
	/**
	* @brief Vector of interrupt or exception
	*
	* [Bits 7:0] Determines which entry in the IDT is used or which other event is injected.
	*/
	uint32_t vector                                                  : 8;
#define VMENTRY_INTERRUPT_INFORMATION_VECTOR_BIT                     0
#define VMENTRY_INTERRUPT_INFORMATION_VECTOR_FLAG                    0xFF
#define VMENTRY_INTERRUPT_INFORMATION_VECTOR_MASK                    0xFF
#define VMENTRY_INTERRUPT_INFORMATION_VECTOR(_)                      (((_) >> 0) & 0xFF)

	/**
	* @brief Interruption type
	*
	* [Bits 10:8] Determines details of how the injection is performed.
	*/
	uint32_t interruption_type                                       : 3;
#define VMENTRY_INTERRUPT_INFORMATION_INTERRUPTION_TYPE_BIT          8
#define VMENTRY_INTERRUPT_INFORMATION_INTERRUPTION_TYPE_FLAG         0x700
#define VMENTRY_INTERRUPT_INFORMATION_INTERRUPTION_TYPE_MASK         0x07
#define VMENTRY_INTERRUPT_INFORMATION_INTERRUPTION_TYPE(_)           (((_) >> 8) & 0x07)

	/**
	* @brief Deliver error code (0 = do not deliver; 1 = deliver)
	*
	* [Bit 11] Determines whether delivery pushes an error code on the guest stack.
	*/
	uint32_t deliver_error_code                                      : 1;
#define VMENTRY_INTERRUPT_INFORMATION_DELIVER_ERROR_CODE_BIT         11
#define VMENTRY_INTERRUPT_INFORMATION_DELIVER_ERROR_CODE_FLAG        0x800
#define VMENTRY_INTERRUPT_INFORMATION_DELIVER_ERROR_CODE_MASK        0x01
#define VMENTRY_INTERRUPT_INFORMATION_DELIVER_ERROR_CODE(_)          (((_) >> 11) & 0x01)
	uint32_t reserved1                                               : 19;

	/**
	* @brief Valid
	*
	* [Bit 31] VM entry injects an event if and only if the valid bit is 1. The valid bit in this field is cleared on every VM
	* exit.
	*/
	uint32_t valid                                                   : 1;
#define VMENTRY_INTERRUPT_INFORMATION_VALID_BIT                      31
#define VMENTRY_INTERRUPT_INFORMATION_VALID_FLAG                     0x80000000
#define VMENTRY_INTERRUPT_INFORMATION_VALID_MASK                     0x01
#define VMENTRY_INTERRUPT_INFORMATION_VALID(_)                       (((_) >> 31) & 0x01)
  };

  uint32_t flags;
} vmentry_interrupt_information;

/**
 * @brief VM entry can be configured to conclude by delivering an event through the IDT (after all guest state and MSRs
 *        have been loaded). This process is called event injection and is controlled by these VM-entry control fields
 *
 * @see Vol3A[24.9.2(Information for VM Exits Due to Vectored Events)] (reference)
 */
typedef union
{
  struct
  {
	/**
	* [Bits 7:0] Vector of interrupt or exception.
	*/
	uint32_t vector                                                  : 8;
#define VMEXIT_INTERRUPT_INFORMATION_VECTOR_BIT                      0
#define VMEXIT_INTERRUPT_INFORMATION_VECTOR_FLAG                     0xFF
#define VMEXIT_INTERRUPT_INFORMATION_VECTOR_MASK                     0xFF
#define VMEXIT_INTERRUPT_INFORMATION_VECTOR(_)                       (((_) >> 0) & 0xFF)

	/**
	* [Bits 10:8] Interruption type.
	*/
	uint32_t interruption_type                                       : 3;
#define VMEXIT_INTERRUPT_INFORMATION_INTERRUPTION_TYPE_BIT           8
#define VMEXIT_INTERRUPT_INFORMATION_INTERRUPTION_TYPE_FLAG          0x700
#define VMEXIT_INTERRUPT_INFORMATION_INTERRUPTION_TYPE_MASK          0x07
#define VMEXIT_INTERRUPT_INFORMATION_INTERRUPTION_TYPE(_)            (((_) >> 8) & 0x07)

	/**
	* [Bit 11] Deliver error code (0 = do not deliver; 1 = deliver).
	*/
	uint32_t error_code_valid                                        : 1;
#define VMEXIT_INTERRUPT_INFORMATION_ERROR_CODE_VALID_BIT            11
#define VMEXIT_INTERRUPT_INFORMATION_ERROR_CODE_VALID_FLAG           0x800
#define VMEXIT_INTERRUPT_INFORMATION_ERROR_CODE_VALID_MASK           0x01
#define VMEXIT_INTERRUPT_INFORMATION_ERROR_CODE_VALID(_)             (((_) >> 11) & 0x01)

	/**
	* [Bit 12] NMI unblocking due to IRET.
	*/
	uint32_t nmi_unblocking                                          : 1;
#define VMEXIT_INTERRUPT_INFORMATION_NMI_UNBLOCKING_BIT              12
#define VMEXIT_INTERRUPT_INFORMATION_NMI_UNBLOCKING_FLAG             0x1000
#define VMEXIT_INTERRUPT_INFORMATION_NMI_UNBLOCKING_MASK             0x01
#define VMEXIT_INTERRUPT_INFORMATION_NMI_UNBLOCKING(_)               (((_) >> 12) & 0x01)
	uint32_t reserved1                                               : 18;

	/**
	* [Bit 31] Valid.
	*/
	uint32_t valid                                                   : 1;
#define VMEXIT_INTERRUPT_INFORMATION_VALID_BIT                       31
#define VMEXIT_INTERRUPT_INFORMATION_VALID_FLAG                      0x80000000
#define VMEXIT_INTERRUPT_INFORMATION_VALID_MASK                      0x01
#define VMEXIT_INTERRUPT_INFORMATION_VALID(_)                        (((_) >> 31) & 0x01)
  };

  uint32_t flags;
} vmexit_interrupt_information;

/**
 * @}
 */

/**
 * @defgroup apic \
 *           Advanced Programmable Interrupt Controller (APIC)
 *
 * Software interacts with the local APIC by reading and writing its registers. APIC registers are memory-mapped to a
 * 4-KByte region of the processor's physical address space with an initial starting address of FEE00000H. For correct APIC
 * operation, this address space must be mapped to an area of memory that has been designated as strong uncacheable (UC).
 *
 * @remarks Registers are 32 bits, 64 bits, or 256 bits in width; all are aligned on 128-bit boundaries. All 32-bit
 *          registers should be accessed using 128-bit aligned 32-bit loads or stores. Some processors may support loads and stores
 *          of less than 32 bits to some of the APIC registers. This is model specific behavior and is not guaranteed to work on all
 *          processors. Any FP/MMX/SSE access to an APIC register, or any access that touches bytes 4 through 15 of an APIC register
 *          may cause undefined behavior and must not be executed. This undefined behavior could include hangs, incorrect results or
 *          unexpected exceptions, including machine checks, and may vary between implementations. Wider registers (64-bit or
 *          256-bit) must be accessed using multiple 32-bit loads or stores, with all accesses being 128-bit aligned.
 * @see Vol3A[10.4.1(The Local APIC Block Diagram)] (reference)
 * @{
 */
/**
 * Local APIC Base Address.
 *
 * @remarks Reserved.
 */
#define APIC_BASE_ADDRESS                                            0xFEE00000

/**
 * Local APIC ID Register.
 */
#define APIC_ID                                                      0x00000020

/**
 * Local APIC Version Register.
 */
#define APIC_VERSION                                                 0x00000030

/**
 * Task Priority Register (TPR).
 */
#define APIC_TASK_PRIORITY                                           0x00000080

/**
 * Arbitration Priority Register (APR).
 */
#define APIC_ARBITRATION_PRIORITY                                    0x00000090

/**
 * Processor Priority Register (PPR).
 */
#define APIC_PROCESSOR_PRIORITY                                      0x000000A0

/**
 * EOI Register.
 */
#define APIC_EOI                                                     0x000000B0

/**
 * Remote Read Register (RRD).
 */
#define APIC_REMOTE_READ                                             0x000000C0

/**
 * Logical Destination Register.
 */
#define APIC_LOGICAL_DESTINATION                                     0x000000D0

/**
 * Destination Format Register.
 *
 * @see Vol3A[10.6.2.2(Logical Destination Mode)]
 */
#define APIC_DESTINATION_FORMAT                                      0x000000E0

/**
 * Spurious Interrupt Vector Register.
 *
 * @see Vol3A[10.9(SPURIOUS INTERRUPT)]
 */
#define APIC_SPURIOUS_INTERRUPT_VECTOR                               0x000000F0

/**
 * In-Service Register (ISR); bits 31:0.
 */
#define APIC_IN_SERVICE_BITS_31_0                                    0x00000100

/**
 * In-Service Register (ISR); bits 63:32.
 */
#define APIC_IN_SERVICE_BITS_63_32                                   0x00000110

/**
 * In-Service Register (ISR); bits 95:64.
 */
#define APIC_IN_SERVICE_BITS_95_64                                   0x00000120

/**
 * In-Service Register (ISR); bits 127:96.
 */
#define APIC_IN_SERVICE_BITS_127_96                                  0x00000130

/**
 * In-Service Register (ISR); bits 159:128.
 */
#define APIC_IN_SERVICE_BITS_159_128                                 0x00000140

/**
 * In-Service Register (ISR); bits 191:160.
 */
#define APIC_IN_SERVICE_BITS_191_160                                 0x00000150

/**
 * In-Service Register (ISR); bits 223:192.
 */
#define APIC_IN_SERVICE_BITS_223_192                                 0x00000160

/**
 * In-Service Register (ISR); bits 255:224.
 */
#define APIC_IN_SERVICE_BITS_255_224                                 0x00000170

/**
 * Trigger Mode Register (TMR); bits 31:0.
 */
#define APIC_TRIGGER_MODE_BITS_31_0                                  0x00000180

/**
 * Trigger Mode Register (TMR); bits 63:32.
 */
#define APIC_TRIGGER_MODE_BITS_63_32                                 0x00000190

/**
 * Trigger Mode Register (TMR); bits 95:64.
 */
#define APIC_TRIGGER_MODE_BITS_95_64                                 0x000001A0

/**
 * Trigger Mode Register (TMR); bits 127:96.
 */
#define APIC_TRIGGER_MODE_BITS_127_96                                0x000001B0

/**
 * Trigger Mode Register (TMR); bits 159:128.
 */
#define APIC_TRIGGER_MODE_BITS_159_128                               0x000001C0

/**
 * Trigger Mode Register (TMR); bits 191:160.
 */
#define APIC_TRIGGER_MODE_BITS_191_160                               0x000001D0

/**
 * Trigger Mode Register (TMR); bits 223:192.
 */
#define APIC_TRIGGER_MODE_BITS_223_192                               0x000001E0

/**
 * Trigger Mode Register (TMR); bits 255:224.
 */
#define APIC_TRIGGER_MODE_BITS_255_224                               0x000001F0

/**
 * Interrupt Request Register (IRR); bits 31:0.
 */
#define APIC_INTERRUPT_REQUEST_BITS_31_0                             0x00000200

/**
 * Interrupt Request Register (IRR); bits 63:32.
 */
#define APIC_INTERRUPT_REQUEST_BITS_63_32                            0x00000210

/**
 * Interrupt Request Register (IRR); bits 95:64.
 */
#define APIC_INTERRUPT_REQUEST_BITS_95_64                            0x00000220

/**
 * Interrupt Request Register (IRR); bits 127:96.
 */
#define APIC_INTERRUPT_REQUEST_BITS_127_96                           0x00000230

/**
 * Interrupt Request Register (IRR); bits 159:128.
 */
#define APIC_INTERRUPT_REQUEST_BITS_159_128                          0x00000240

/**
 * Interrupt Request Register (IRR); bits 191:160.
 */
#define APIC_INTERRUPT_REQUEST_BITS_191_160                          0x00000250

/**
 * Interrupt Request Register (IRR); bits 223:192.
 */
#define APIC_INTERRUPT_REQUEST_BITS_223_192                          0x00000260

/**
 * Interrupt Request Register (IRR); bits 255:224.
 */
#define APIC_INTERRUPT_REQUEST_BITS_255_224                          0x00000270

/**
 * Error Status Register.
 */
#define APIC_ERROR_STATUS                                            0x00000280

/**
 * LVT Corrected Machine Check Interrupt (CMCI) Register.
 */
#define APIC_LVT_CORRECTED_MACHINE_CHECK_INTERRUPT                   0x000002F0

/**
 * Interrupt Command Register (ICR); bits 0-31.
 */
#define APIC_INTERRUPT_COMMAND_BITS_0_31                             0x00000300

/**
 * Interrupt Command Register (ICR); bits 32-63.
 */
#define APIC_INTERRUPT_COMMAND_BITS_32_63                            0x00000310

/**
 * LVT Timer Register.
 */
#define APIC_LVT_TIMER                                               0x00000320

/**
 * LVT Thermal Sensor Register.
 */
#define APIC_LVT_THERMAL_SENSOR                                      0x00000330

/**
 * LVT Performance Monitoring Counters Register.
 */
#define APIC_LVT_PERFORMANCE_MONITORING_COUNTERS                     0x00000340

/**
 * LVT LINT0 Register.
 */
#define APIC_LVT_LINT0                                               0x00000350

/**
 * LVT LINT1 Register.
 */
#define APIC_LVT_LINT1                                               0x00000360

/**
 * LVT Error Register.
 */
#define APIC_LVT_ERROR                                               0x00000370

/**
 * Initial Count Register (for Timer).
 */
#define APIC_INITIAL_COUNT                                           0x00000380

/**
 * Current Count Register (for Timer).
 */
#define APIC_CURRENT_COUNT                                           0x00000390

/**
 * Divide Configuration Register (for Timer).
 */
#define APIC_DIVIDE_CONFIGURATION                                    0x000003E0
/**
 * @}
 */

/**
 * The 32-bit EFLAGS register contains a group of status flags, a control flag, and a group of system flags. The status
 * flags (bits 0, 2, 4, 6, 7, and 11) of the EFLAGS register indicate the results of arithmetic instructions, such as the
 * ADD, SUB, MUL, and DIV instructions.
 * The system flags and IOPL field in the EFLAGS register control operating-system or executive operations.
 *
 * @see Vol1[3.4.3(EFLAGS)] (reference)
 */
typedef union
{
  struct
  {
	/**
	* @brief Carry flag
	*
	* [Bit 0] Set if an arithmetic operation generates a carry or a borrow out of the mostsignificant bit of the result;
	* cleared otherwise. This flag indicates an overflow condition for unsigned-integer arithmetic. It is also used in
	* multiple-precision arithmetic.
	*/
	uint32_t carry_flag                                              : 1;
#define EFLAGS_CARRY_FLAG_BIT                                        0
#define EFLAGS_CARRY_FLAG_FLAG                                       0x01
#define EFLAGS_CARRY_FLAG_MASK                                       0x01
#define EFLAGS_CARRY_FLAG(_)                                         (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Reserved - always 1
	*/
	uint32_t read_as_1                                               : 1;
#define EFLAGS_READ_AS_1_BIT                                         1
#define EFLAGS_READ_AS_1_FLAG                                        0x02
#define EFLAGS_READ_AS_1_MASK                                        0x01
#define EFLAGS_READ_AS_1(_)                                          (((_) >> 1) & 0x01)

	/**
	* @brief Parity flag
	*
	* [Bit 2] Set if the least-significant byte of the result contains an even number of 1 bits; cleared otherwise.
	*/
	uint32_t parity_flag                                             : 1;
#define EFLAGS_PARITY_FLAG_BIT                                       2
#define EFLAGS_PARITY_FLAG_FLAG                                      0x04
#define EFLAGS_PARITY_FLAG_MASK                                      0x01
#define EFLAGS_PARITY_FLAG(_)                                        (((_) >> 2) & 0x01)
	uint32_t reserved1                                               : 1;

	/**
	* @brief Auxiliary Carry flag
	*
	* [Bit 4] Set if an arithmetic operation generates a carry or a borrow out of bit 3 of the result; cleared otherwise. This
	* flag is used in binary-coded decimal (BCD) arithmetic.
	*/
	uint32_t auxiliary_carry_flag                                    : 1;
#define EFLAGS_AUXILIARY_CARRY_FLAG_BIT                              4
#define EFLAGS_AUXILIARY_CARRY_FLAG_FLAG                             0x10
#define EFLAGS_AUXILIARY_CARRY_FLAG_MASK                             0x01
#define EFLAGS_AUXILIARY_CARRY_FLAG(_)                               (((_) >> 4) & 0x01)
	uint32_t reserved2                                               : 1;

	/**
	* @brief Zero flag
	*
	* [Bit 6] Set if the result is zero; cleared otherwise.
	*/
	uint32_t zero_flag                                               : 1;
#define EFLAGS_ZERO_FLAG_BIT                                         6
#define EFLAGS_ZERO_FLAG_FLAG                                        0x40
#define EFLAGS_ZERO_FLAG_MASK                                        0x01
#define EFLAGS_ZERO_FLAG(_)                                          (((_) >> 6) & 0x01)

	/**
	* @brief Sign flag
	*
	* [Bit 7] Set equal to the most-significant bit of the result, which is the sign bit of a signed integer. (0 indicates a
	* positive value and 1 indicates a negative value.)
	*/
	uint32_t sign_flag                                               : 1;
#define EFLAGS_SIGN_FLAG_BIT                                         7
#define EFLAGS_SIGN_FLAG_FLAG                                        0x80
#define EFLAGS_SIGN_FLAG_MASK                                        0x01
#define EFLAGS_SIGN_FLAG(_)                                          (((_) >> 7) & 0x01)

	/**
	* @brief Trap flag
	*
	* [Bit 8] Set to enable single-step mode for debugging; clear to disable single-step mode.
	*/
	uint32_t trap_flag                                               : 1;
#define EFLAGS_TRAP_FLAG_BIT                                         8
#define EFLAGS_TRAP_FLAG_FLAG                                        0x100
#define EFLAGS_TRAP_FLAG_MASK                                        0x01
#define EFLAGS_TRAP_FLAG(_)                                          (((_) >> 8) & 0x01)

	/**
	* @brief Interrupt enable flag
	*
	* [Bit 9] Controls the response of the processor to maskable interrupt requests. Set to respond to maskable interrupts;
	* cleared to inhibit maskable interrupts.
	*/
	uint32_t interrupt_enable_flag                                   : 1;
#define EFLAGS_INTERRUPT_ENABLE_FLAG_BIT                             9
#define EFLAGS_INTERRUPT_ENABLE_FLAG_FLAG                            0x200
#define EFLAGS_INTERRUPT_ENABLE_FLAG_MASK                            0x01
#define EFLAGS_INTERRUPT_ENABLE_FLAG(_)                              (((_) >> 9) & 0x01)

	/**
	* @brief Direction flag
	*
	* [Bit 10] Controls string instructions (MOVS, CMPS, SCAS, LODS, and STOS). Setting the DF flag causes the string
	* instructions to auto-decrement (to process strings from high addresses to low addresses). Clearing the DF flag causes
	* the string instructions to auto-increment (process strings from low addresses to high addresses).
	*/
	uint32_t direction_flag                                          : 1;
#define EFLAGS_DIRECTION_FLAG_BIT                                    10
#define EFLAGS_DIRECTION_FLAG_FLAG                                   0x400
#define EFLAGS_DIRECTION_FLAG_MASK                                   0x01
#define EFLAGS_DIRECTION_FLAG(_)                                     (((_) >> 10) & 0x01)

	/**
	* @brief Overflow flag
	*
	* [Bit 11] Set if the integer result is too large a positive number or too small a negative number (excluding the
	* sign-bit) to fit in the destination operand; cleared otherwise. This flag indicates an overflow condition for
	* signed-integer (two's complement) arithmetic.
	*/
	uint32_t overflow_flag                                           : 1;
#define EFLAGS_OVERFLOW_FLAG_BIT                                     11
#define EFLAGS_OVERFLOW_FLAG_FLAG                                    0x800
#define EFLAGS_OVERFLOW_FLAG_MASK                                    0x01
#define EFLAGS_OVERFLOW_FLAG(_)                                      (((_) >> 11) & 0x01)

	/**
	* @brief I/O privilege level field
	*
	* [Bits 13:12] Indicates the I/O privilege level of the currently running program or task. The current privilege level
	* (CPL) of the currently running program or task must be less than or equal to the I/O privilege level to access the I/O
	* address space. The POPF and IRET instructions can modify this field only when operating at a CPL of 0.
	*/
	uint32_t io_privilege_level                                      : 2;
#define EFLAGS_IO_PRIVILEGE_LEVEL_BIT                                12
#define EFLAGS_IO_PRIVILEGE_LEVEL_FLAG                               0x3000
#define EFLAGS_IO_PRIVILEGE_LEVEL_MASK                               0x03
#define EFLAGS_IO_PRIVILEGE_LEVEL(_)                                 (((_) >> 12) & 0x03)

	/**
	* @brief Nested task flag
	*
	* [Bit 14] Controls the chaining of interrupted and called tasks. Set when the current task is linked to the previously
	* executed task; cleared when the current task is not linked to another task.
	*/
	uint32_t nested_task_flag                                        : 1;
#define EFLAGS_NESTED_TASK_FLAG_BIT                                  14
#define EFLAGS_NESTED_TASK_FLAG_FLAG                                 0x4000
#define EFLAGS_NESTED_TASK_FLAG_MASK                                 0x01
#define EFLAGS_NESTED_TASK_FLAG(_)                                   (((_) >> 14) & 0x01)
	uint32_t reserved3                                               : 1;

	/**
	* @brief Resume flag
	*
	* [Bit 16] Controls the processor's response to debug exceptions.
	*/
	uint32_t resume_flag                                             : 1;
#define EFLAGS_RESUME_FLAG_BIT                                       16
#define EFLAGS_RESUME_FLAG_FLAG                                      0x10000
#define EFLAGS_RESUME_FLAG_MASK                                      0x01
#define EFLAGS_RESUME_FLAG(_)                                        (((_) >> 16) & 0x01)

	/**
	* @brief Virtual-8086 mode flag
	*
	* [Bit 17] Set to enable virtual-8086 mode; clear to return to protected mode without virtual-8086 mode semantics.
	*/
	uint32_t virtual_8086_mode_flag                                  : 1;
#define EFLAGS_VIRTUAL_8086_MODE_FLAG_BIT                            17
#define EFLAGS_VIRTUAL_8086_MODE_FLAG_FLAG                           0x20000
#define EFLAGS_VIRTUAL_8086_MODE_FLAG_MASK                           0x01
#define EFLAGS_VIRTUAL_8086_MODE_FLAG(_)                             (((_) >> 17) & 0x01)

	/**
	* @brief Alignment check (or access control) flag
	*
	* [Bit 18] If the AM bit is set in the CR0 register, alignment checking of user-mode data accesses is enabled if and only
	* if this flag is 1. If the SMAP bit is set in the CR4 register, explicit supervisor-mode data accesses to user-mode pages
	* are allowed if and only if this bit is 1.
	*
	* @see Vol3A[4.6(ACCESS RIGHTS)]
	*/
	uint32_t alignment_check_flag                                    : 1;
#define EFLAGS_ALIGNMENT_CHECK_FLAG_BIT                              18
#define EFLAGS_ALIGNMENT_CHECK_FLAG_FLAG                             0x40000
#define EFLAGS_ALIGNMENT_CHECK_FLAG_MASK                             0x01
#define EFLAGS_ALIGNMENT_CHECK_FLAG(_)                               (((_) >> 18) & 0x01)

	/**
	* @brief Virtual interrupt flag
	*
	* [Bit 19] Virtual image of the IF flag. Used in conjunction with the VIP flag. (To use this flag and the VIP flag the
	* virtual mode extensions are enabled by setting the VME flag in control register CR4.)
	*/
	uint32_t virtual_interrupt_flag                                  : 1;
#define EFLAGS_VIRTUAL_INTERRUPT_FLAG_BIT                            19
#define EFLAGS_VIRTUAL_INTERRUPT_FLAG_FLAG                           0x80000
#define EFLAGS_VIRTUAL_INTERRUPT_FLAG_MASK                           0x01
#define EFLAGS_VIRTUAL_INTERRUPT_FLAG(_)                             (((_) >> 19) & 0x01)

	/**
	* @brief Virtual interrupt pending flag
	*
	* [Bit 20] Set to indicate that an interrupt is pending; clear when no interrupt is pending. (Software sets and clears
	* this flag; the processor only reads it.) Used in conjunction with the VIF flag.
	*/
	uint32_t virtual_interrupt_pending_flag                          : 1;
#define EFLAGS_VIRTUAL_INTERRUPT_PENDING_FLAG_BIT                    20
#define EFLAGS_VIRTUAL_INTERRUPT_PENDING_FLAG_FLAG                   0x100000
#define EFLAGS_VIRTUAL_INTERRUPT_PENDING_FLAG_MASK                   0x01
#define EFLAGS_VIRTUAL_INTERRUPT_PENDING_FLAG(_)                     (((_) >> 20) & 0x01)

	/**
	* @brief Identification flag
	*
	* [Bit 21] The ability of a program to set or clear this flag indicates support for the CPUID instruction.
	*/
	uint32_t identification_flag                                     : 1;
#define EFLAGS_IDENTIFICATION_FLAG_BIT                               21
#define EFLAGS_IDENTIFICATION_FLAG_FLAG                              0x200000
#define EFLAGS_IDENTIFICATION_FLAG_MASK                              0x01
#define EFLAGS_IDENTIFICATION_FLAG(_)                                (((_) >> 21) & 0x01)
	uint32_t reserved4                                               : 10;
  };

  uint32_t flags;
} eflags;

/**
 * The 64-bit RFLAGS register contains a group of status flags, a control flag, and a group of system flags in 64-bit mode.
 * The upper 32 bits of RFLAGS register is reserved. The lower 32 bits of RFLAGS is the same as EFLAGS.
 *
 * @see EFLAGS
 * @see Vol1[3.4.3.4(RFLAGS Register in 64-Bit Mode)] (reference)
 */
typedef union
{
  struct
  {
	/**
	* @brief Carry flag
	*
	* [Bit 0] See the description in EFLAGS.
	*/
	uint64_t carry_flag                                              : 1;
#define RFLAGS_CARRY_FLAG_BIT                                        0
#define RFLAGS_CARRY_FLAG_FLAG                                       0x01
#define RFLAGS_CARRY_FLAG_MASK                                       0x01
#define RFLAGS_CARRY_FLAG(_)                                         (((_) >> 0) & 0x01)

	/**
	* [Bit 1] Reserved - always 1
	*/
	uint64_t read_as_1                                               : 1;
#define RFLAGS_READ_AS_1_BIT                                         1
#define RFLAGS_READ_AS_1_FLAG                                        0x02
#define RFLAGS_READ_AS_1_MASK                                        0x01
#define RFLAGS_READ_AS_1(_)                                          (((_) >> 1) & 0x01)

	/**
	* @brief Parity flag
	*
	* [Bit 2] See the description in EFLAGS.
	*/
	uint64_t parity_flag                                             : 1;
#define RFLAGS_PARITY_FLAG_BIT                                       2
#define RFLAGS_PARITY_FLAG_FLAG                                      0x04
#define RFLAGS_PARITY_FLAG_MASK                                      0x01
#define RFLAGS_PARITY_FLAG(_)                                        (((_) >> 2) & 0x01)
	uint64_t reserved1                                               : 1;

	/**
	* @brief Auxiliary Carry flag
	*
	* [Bit 4] See the description in EFLAGS.
	*/
	uint64_t auxiliary_carry_flag                                    : 1;
#define RFLAGS_AUXILIARY_CARRY_FLAG_BIT                              4
#define RFLAGS_AUXILIARY_CARRY_FLAG_FLAG                             0x10
#define RFLAGS_AUXILIARY_CARRY_FLAG_MASK                             0x01
#define RFLAGS_AUXILIARY_CARRY_FLAG(_)                               (((_) >> 4) & 0x01)
	uint64_t reserved2                                               : 1;

	/**
	* @brief Zero flag
	*
	* [Bit 6] See the description in EFLAGS.
	*/
	uint64_t zero_flag                                               : 1;
#define RFLAGS_ZERO_FLAG_BIT                                         6
#define RFLAGS_ZERO_FLAG_FLAG                                        0x40
#define RFLAGS_ZERO_FLAG_MASK                                        0x01
#define RFLAGS_ZERO_FLAG(_)                                          (((_) >> 6) & 0x01)

	/**
	* @brief Sign flag
	*
	* [Bit 7] See the description in EFLAGS.
	*/
	uint64_t sign_flag                                               : 1;
#define RFLAGS_SIGN_FLAG_BIT                                         7
#define RFLAGS_SIGN_FLAG_FLAG                                        0x80
#define RFLAGS_SIGN_FLAG_MASK                                        0x01
#define RFLAGS_SIGN_FLAG(_)                                          (((_) >> 7) & 0x01)

	/**
	* @brief Trap flag
	*
	* [Bit 8] See the description in EFLAGS.
	*/
	uint64_t trap_flag                                               : 1;
#define RFLAGS_TRAP_FLAG_BIT                                         8
#define RFLAGS_TRAP_FLAG_FLAG                                        0x100
#define RFLAGS_TRAP_FLAG_MASK                                        0x01
#define RFLAGS_TRAP_FLAG(_)                                          (((_) >> 8) & 0x01)

	/**
	* @brief Interrupt enable flag
	*
	* [Bit 9] See the description in EFLAGS.
	*/
	uint64_t interrupt_enable_flag                                   : 1;
#define RFLAGS_INTERRUPT_ENABLE_FLAG_BIT                             9
#define RFLAGS_INTERRUPT_ENABLE_FLAG_FLAG                            0x200
#define RFLAGS_INTERRUPT_ENABLE_FLAG_MASK                            0x01
#define RFLAGS_INTERRUPT_ENABLE_FLAG(_)                              (((_) >> 9) & 0x01)

	/**
	* @brief Direction flag
	*
	* [Bit 10] See the description in EFLAGS.
	*/
	uint64_t direction_flag                                          : 1;
#define RFLAGS_DIRECTION_FLAG_BIT                                    10
#define RFLAGS_DIRECTION_FLAG_FLAG                                   0x400
#define RFLAGS_DIRECTION_FLAG_MASK                                   0x01
#define RFLAGS_DIRECTION_FLAG(_)                                     (((_) >> 10) & 0x01)

	/**
	* @brief Overflow flag
	*
	* [Bit 11] See the description in EFLAGS.
	*/
	uint64_t overflow_flag                                           : 1;
#define RFLAGS_OVERFLOW_FLAG_BIT                                     11
#define RFLAGS_OVERFLOW_FLAG_FLAG                                    0x800
#define RFLAGS_OVERFLOW_FLAG_MASK                                    0x01
#define RFLAGS_OVERFLOW_FLAG(_)                                      (((_) >> 11) & 0x01)

	/**
	* @brief I/O privilege level field
	*
	* [Bits 13:12] See the description in EFLAGS.
	*/
	uint64_t io_privilege_level                                      : 2;
#define RFLAGS_IO_PRIVILEGE_LEVEL_BIT                                12
#define RFLAGS_IO_PRIVILEGE_LEVEL_FLAG                               0x3000
#define RFLAGS_IO_PRIVILEGE_LEVEL_MASK                               0x03
#define RFLAGS_IO_PRIVILEGE_LEVEL(_)                                 (((_) >> 12) & 0x03)

	/**
	* @brief Nested task flag
	*
	* [Bit 14] See the description in EFLAGS.
	*/
	uint64_t nested_task_flag                                        : 1;
#define RFLAGS_NESTED_TASK_FLAG_BIT                                  14
#define RFLAGS_NESTED_TASK_FLAG_FLAG                                 0x4000
#define RFLAGS_NESTED_TASK_FLAG_MASK                                 0x01
#define RFLAGS_NESTED_TASK_FLAG(_)                                   (((_) >> 14) & 0x01)
	uint64_t reserved3                                               : 1;

	/**
	* @brief Resume flag
	*
	* [Bit 16] See the description in EFLAGS.
	*/
	uint64_t resume_flag                                             : 1;
#define RFLAGS_RESUME_FLAG_BIT                                       16
#define RFLAGS_RESUME_FLAG_FLAG                                      0x10000
#define RFLAGS_RESUME_FLAG_MASK                                      0x01
#define RFLAGS_RESUME_FLAG(_)                                        (((_) >> 16) & 0x01)

	/**
	* @brief Virtual-8086 mode flag
	*
	* [Bit 17] See the description in EFLAGS.
	*/
	uint64_t virtual_8086_mode_flag                                  : 1;
#define RFLAGS_VIRTUAL_8086_MODE_FLAG_BIT                            17
#define RFLAGS_VIRTUAL_8086_MODE_FLAG_FLAG                           0x20000
#define RFLAGS_VIRTUAL_8086_MODE_FLAG_MASK                           0x01
#define RFLAGS_VIRTUAL_8086_MODE_FLAG(_)                             (((_) >> 17) & 0x01)

	/**
	* @brief Alignment check (or access control) flag
	*
	* [Bit 18] See the description in EFLAGS.
	*
	* @see Vol3A[4.6(ACCESS RIGHTS)]
	*/
	uint64_t alignment_check_flag                                    : 1;
#define RFLAGS_ALIGNMENT_CHECK_FLAG_BIT                              18
#define RFLAGS_ALIGNMENT_CHECK_FLAG_FLAG                             0x40000
#define RFLAGS_ALIGNMENT_CHECK_FLAG_MASK                             0x01
#define RFLAGS_ALIGNMENT_CHECK_FLAG(_)                               (((_) >> 18) & 0x01)

	/**
	* @brief Virtual interrupt flag
	*
	* [Bit 19] See the description in EFLAGS.
	*/
	uint64_t virtual_interrupt_flag                                  : 1;
#define RFLAGS_VIRTUAL_INTERRUPT_FLAG_BIT                            19
#define RFLAGS_VIRTUAL_INTERRUPT_FLAG_FLAG                           0x80000
#define RFLAGS_VIRTUAL_INTERRUPT_FLAG_MASK                           0x01
#define RFLAGS_VIRTUAL_INTERRUPT_FLAG(_)                             (((_) >> 19) & 0x01)

	/**
	* @brief Virtual interrupt pending flag
	*
	* [Bit 20] See the description in EFLAGS.
	*/
	uint64_t virtual_interrupt_pending_flag                          : 1;
#define RFLAGS_VIRTUAL_INTERRUPT_PENDING_FLAG_BIT                    20
#define RFLAGS_VIRTUAL_INTERRUPT_PENDING_FLAG_FLAG                   0x100000
#define RFLAGS_VIRTUAL_INTERRUPT_PENDING_FLAG_MASK                   0x01
#define RFLAGS_VIRTUAL_INTERRUPT_PENDING_FLAG(_)                     (((_) >> 20) & 0x01)

	/**
	* @brief Identification flag
	*
	* [Bit 21] See the description in EFLAGS.
	*/
	uint64_t identification_flag                                     : 1;
#define RFLAGS_IDENTIFICATION_FLAG_BIT                               21
#define RFLAGS_IDENTIFICATION_FLAG_FLAG                              0x200000
#define RFLAGS_IDENTIFICATION_FLAG_MASK                              0x01
#define RFLAGS_IDENTIFICATION_FLAG(_)                                (((_) >> 21) & 0x01)
	uint64_t reserved4                                               : 42;
  };

  uint64_t flags;
} rflags;

/**
 * @defgroup exceptions \
 *           Exceptions
 * @{
 */
/**
 * @brief Exceptions that can occur when the instruction is executed in protected mode.
 *        Each exception is given a mnemonic that consists of a pound sign (\#) followed by two letters and an optional error code
 *        in parentheses. For example, \#GP(0) denotes a general protection exception with an error code of 0
 *
 * @see Vol2A[3.1.1.13(Protected Mode Exceptions Section)] (reference)
 * @see Vol3A[6.3.1(External Interrupts)] (reference)
 */
typedef enum
{
  /**
	* #DE - Divide Error.
	* Source: DIV and IDIV instructions.
	* Error Code: No.
	*/
  divide_error                                                 = 0x00000000,

  /**
	* #DB - Debug.
	* Source: Any code or data reference.
	* Error Code: No.
	*/
  debug                                                        = 0x00000001,

  /**
	* Nonmaskable Interrupt.
	* Source: Generated externally by asserting the processor's NMI pin or
	*         through an NMI request set by the I/O APIC to the local APIC.
	* Error Code: No.
	*/
  nmi                                                          = 0x00000002,

  /**
	* #BP - Breakpoint.
	* Source: INT3 instruction.
	* Error Code: No.
	*/
  breakpoint                                                   = 0x00000003,

  /**
	* #OF - Overflow.
	* Source: INTO instruction.
	* Error Code: No.
	*/
  overflow                                                     = 0x00000004,

  /**
	* #BR - BOUND Range Exceeded.
	* Source: BOUND instruction.
	* Error Code: No.
	*/
  bound_range_exceeded                                         = 0x00000005,

  /**
	* #UD - Invalid Opcode (Undefined Opcode).
	* Source: UD instruction or reserved opcode.
	* Error Code: No.
	*/
  invalid_opcode                                               = 0x00000006,

  /**
	* #NM - Device Not Available (No Math Coprocessor).
	* Source: Floating-point or WAIT/FWAIT instruction.
	* Error Code: No.
	*/
  device_not_available                                         = 0x00000007,

  /**
	* #DF - Double Fault.
	* Source: Any instruction that can generate an exception, an NMI, or an INTR.
	* Error Code: Yes (zero).
	*/
  double_fault                                                 = 0x00000008,

  /**
	* #\## - Coprocessor Segment Overrun (reserved).
	* Source: Floating-point instruction.
	* Error Code: No.
	*
	* @note Processors after the Intel386 processor do not generate this exception.
	*/
  coprocessor_segment_overrun                                  = 0x00000009,

  /**
	* #TS - Invalid TSS.
	* Source: Task switch or TSS access.
	* Error Code: Yes.
	*/
  invalid_tss                                                  = 0x0000000A,

  /**
	* #NP - Segment Not Present.
	* Source: Loading segment registers or accessing system segments.
	* Error Code: Yes.
	*/
  segment_not_present                                          = 0x0000000B,

  /**
	* #SS - Stack Segment Fault.
	* Source: Stack operations and SS register loads.
	* Error Code: Yes.
	*/
  stack_segment_fault                                          = 0x0000000C,

  /**
	* #GP - General Protection.
	* Source: Any memory reference and other protection checks.
	* Error Code: Yes.
	*/
  general_protection                                           = 0x0000000D,

  /**
	* #PF - Page Fault.
	* Source: Any memory reference.
	* Error Code: Yes.
	*/
  page_fault                                                   = 0x0000000E,

  /**
	* #MF - Floating-Point Error (Math Fault).
	* Source: Floating-point or WAIT/FWAIT instruction.
	* Error Code: No.
	*/
  x87_floating_point_error                                     = 0x00000010,

  /**
	* #AC - Alignment Check.
	* Source: Any data reference in memory.
	* Error Code: Yes.
	*/
  alignment_check                                              = 0x00000011,

  /**
	* #MC - Machine Check.
	* Source: Model dependent machine check errors.
	* Error Code: No.
	*/
  machine_check                                                = 0x00000012,

  /**
	* #XM - SIMD Floating-Point Numeric Error.
	* Source: SSE/SSE2/SSE3 floating-point instructions.
	* Error Code: No.
	*/
  simd_floating_point_error                                    = 0x00000013,

  /**
	* #VE - Virtualization Exception.
	* Source: EPT violations.
	* Error Code: No.
	*/
  virtualization_exception                                     = 0x00000014,
} exception_vector;

/**
 * @brief When an exception condition is related to a specific segment selector or IDT vector, the processor pushes an
 *        error code onto the stack of the exception handler (whether it is a procedure or task). The error code resembles a
 *        segment selector; however, instead of a TI flag and RPL field, the error code contains 3 different flags
 *
 * @see Vol3A[6.13(ERROR CODE)] (reference)
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] When set, indicates that the exception occurred during delivery of an event external to the program, such as an
	* interrupt or an earlier exception. The bit is cleared if the exception occurred during delivery of a software interrupt
	* (INT n, INT3, or INTO).
	*/
	uint32_t external_event                                          : 1;
#define EXCEPTION_ERROR_CODE_EXTERNAL_EVENT_BIT                      0
#define EXCEPTION_ERROR_CODE_EXTERNAL_EVENT_FLAG                     0x01
#define EXCEPTION_ERROR_CODE_EXTERNAL_EVENT_MASK                     0x01
#define EXCEPTION_ERROR_CODE_EXTERNAL_EVENT(_)                       (((_) >> 0) & 0x01)

	/**
	* [Bit 1] When set, indicates that the index portion of the error code refers to a gate descriptor in the IDT; when clear,
	* indicates that the index refers to a descriptor in the GDT or the current LDT.
	*/
	uint32_t descriptor_location                                     : 1;
#define EXCEPTION_ERROR_CODE_DESCRIPTOR_LOCATION_BIT                 1
#define EXCEPTION_ERROR_CODE_DESCRIPTOR_LOCATION_FLAG                0x02
#define EXCEPTION_ERROR_CODE_DESCRIPTOR_LOCATION_MASK                0x01
#define EXCEPTION_ERROR_CODE_DESCRIPTOR_LOCATION(_)                  (((_) >> 1) & 0x01)

	/**
	* [Bit 2] Only used when the IDT flag is clear. When set, the TI flag indicates that the index portion of the error code
	* refers to a segment or gate descriptor in the LDT; when clear, it indicates that the index refers to a descriptor in the
	* current GDT.
	*/
	uint32_t gdt_ldt                                                 : 1;
#define EXCEPTION_ERROR_CODE_GDT_LDT_BIT                             2
#define EXCEPTION_ERROR_CODE_GDT_LDT_FLAG                            0x04
#define EXCEPTION_ERROR_CODE_GDT_LDT_MASK                            0x01
#define EXCEPTION_ERROR_CODE_GDT_LDT(_)                              (((_) >> 2) & 0x01)

	/**
	* [Bits 15:3] The segment selector index field provides an index into the IDT, GDT, or current LDT to the segment or gate
	* selector being referenced by the error code. In some cases the error code is null (all bits are clear except possibly
	* EXT). A null error code indicates that the error was not caused by a reference to a specific segment or that a null
	* segment selector was referenced in an operation.
	*
	* @note The format of the error code is different for page-fault exceptions (#PF).
	*/
	uint32_t index                                                   : 13;
#define EXCEPTION_ERROR_CODE_INDEX_BIT                               3
#define EXCEPTION_ERROR_CODE_INDEX_FLAG                              0xFFF8
#define EXCEPTION_ERROR_CODE_INDEX_MASK                              0x1FFF
#define EXCEPTION_ERROR_CODE_INDEX(_)                                (((_) >> 3) & 0x1FFF)
	uint32_t reserved1                                               : 16;
  };

  uint32_t flags;
} exception_error_code;

/**
 * @brief Page fault exception
 *
 * @see Vol3A[4.7(PAGE-FAULT EXCEPTIONS)] (reference)
 */
typedef union
{
  struct
  {
	/**
	* [Bit 0] This flag is 0 if there is no translation for the linear address because the P flag was 0 in one of the
	* pagingstructure entries used to translate that address.
	*/
	uint32_t present                                                 : 1;
#define PAGE_FAULT_EXCEPTION_PRESENT_BIT                             0
#define PAGE_FAULT_EXCEPTION_PRESENT_FLAG                            0x01
#define PAGE_FAULT_EXCEPTION_PRESENT_MASK                            0x01
#define PAGE_FAULT_EXCEPTION_PRESENT(_)                              (((_) >> 0) & 0x01)

	/**
	* [Bit 1] If the access causing the page-fault exception was a write, this flag is 1; otherwise, it is 0. This flag
	* describes the access causing the page-fault exception, not the access rights specified by paging.
	*/
	uint32_t write                                                   : 1;
#define PAGE_FAULT_EXCEPTION_WRITE_BIT                               1
#define PAGE_FAULT_EXCEPTION_WRITE_FLAG                              0x02
#define PAGE_FAULT_EXCEPTION_WRITE_MASK                              0x01
#define PAGE_FAULT_EXCEPTION_WRITE(_)                                (((_) >> 1) & 0x01)

	/**
	* [Bit 2] If a user-mode access caused the page-fault exception, this flag is 1; it is 0 if a supervisor-mode access did
	* so. This flag describes the access causing the page-fault exception, not the access rights specified by paging.
	*
	* @see Vol3A[4.6(ACCESS RIGHTS)]
	*/
	uint32_t user_mode_access                                        : 1;
#define PAGE_FAULT_EXCEPTION_USER_MODE_ACCESS_BIT                    2
#define PAGE_FAULT_EXCEPTION_USER_MODE_ACCESS_FLAG                   0x04
#define PAGE_FAULT_EXCEPTION_USER_MODE_ACCESS_MASK                   0x01
#define PAGE_FAULT_EXCEPTION_USER_MODE_ACCESS(_)                     (((_) >> 2) & 0x01)

	/**
	* [Bit 3] This flag is 1 if there is no translation for the linear address because a reserved bit was set in one of the
	* pagingstructure entries used to translate that address. (Because reserved bits are not checked in a paging-structure
	* entry whose P flag is 0, bit 3 of the error code can be set only if bit 0 is also set). Bits reserved in the
	* paging-structure entries are reserved for future functionality. Software developers should be aware that such bits may
	* be used in the future and that a paging-structure entry that causes a page-fault exception on one processor might not do
	* so in the future.
	*/
	uint32_t reserved_bit_violation                                  : 1;
#define PAGE_FAULT_EXCEPTION_RESERVED_BIT_VIOLATION_BIT              3
#define PAGE_FAULT_EXCEPTION_RESERVED_BIT_VIOLATION_FLAG             0x08
#define PAGE_FAULT_EXCEPTION_RESERVED_BIT_VIOLATION_MASK             0x01
#define PAGE_FAULT_EXCEPTION_RESERVED_BIT_VIOLATION(_)               (((_) >> 3) & 0x01)

	/**
	* [Bit 4] This flag is 1 if (1) the access causing the page-fault exception was an instruction fetch; and (2) either (a)
	* CR4.SMEP = 1; or (b) both (i) CR4.PAE = 1 (either PAE paging or 4-level paging is in use); and (ii) IA32_EFER.NXE = 1.
	* Otherwise, the flag is 0. This flag describes the access causing the page-fault exception, not the access rights
	* specified by paging.
	*/
	uint32_t execute                                                 : 1;
#define PAGE_FAULT_EXCEPTION_EXECUTE_BIT                             4
#define PAGE_FAULT_EXCEPTION_EXECUTE_FLAG                            0x10
#define PAGE_FAULT_EXCEPTION_EXECUTE_MASK                            0x01
#define PAGE_FAULT_EXCEPTION_EXECUTE(_)                              (((_) >> 4) & 0x01)

	/**
	* [Bit 5] This flag is 1 if (1) IA32_EFER.LMA = CR4.PKE = 1; (2) the access causing the page-fault exception was a data
	* access; (3) the linear address was a user-mode address with protection key i; and (5) the PKRU register is such that
	* either (a) ADi = 1; or (b) the following all hold: (i) WDi = 1; (ii) the access is a write access; and (iii) either
	* CR0.WP = 1 or the access causing the page-fault exception was a user-mode access.
	*
	* @see Vol3A[4.6.2(Protection Keys)]
	*/
	uint32_t protection_key_violation                                : 1;
#define PAGE_FAULT_EXCEPTION_PROTECTION_KEY_VIOLATION_BIT            5
#define PAGE_FAULT_EXCEPTION_PROTECTION_KEY_VIOLATION_FLAG           0x20
#define PAGE_FAULT_EXCEPTION_PROTECTION_KEY_VIOLATION_MASK           0x01
#define PAGE_FAULT_EXCEPTION_PROTECTION_KEY_VIOLATION(_)             (((_) >> 5) & 0x01)
	uint32_t reserved1                                               : 9;

	/**
	* [Bit 15] This flag is 1 if the exception is unrelated to paging and resulted from violation of SGX-specific
	* access-control requirements. Because such a violation can occur only if there is no ordinary page fault, this flag is
	* set only if the P flag (bit 0) is 1 and the RSVD flag (bit 3) and the PK flag (bit 5) are both 0.
	*/
	uint32_t sgx                                                     : 1;
#define PAGE_FAULT_EXCEPTION_SGX_BIT                                 15
#define PAGE_FAULT_EXCEPTION_SGX_FLAG                                0x8000
#define PAGE_FAULT_EXCEPTION_SGX_MASK                                0x01
#define PAGE_FAULT_EXCEPTION_SGX(_)                                  (((_) >> 15) & 0x01)
	uint32_t reserved2                                               : 16;
  };

  uint32_t flags;
} page_fault_exception;

/**
 * @}
 */

/**
 * @defgroup memory_type \
 *           Memory caching type
 *
 * The processor allows any area of system memory to be cached in the L1, L2, and L3 caches. In individual pages or regions
 * of system memory, it allows the type of caching (also called memory type) to be specified.
 *
 * @see Vol3A[11.11(MEMORY TYPE RANGE REGISTERS (MTRRS))]
 * @see Vol3A[11.5(CACHE CONTROL)]
 * @see Vol3A[11.3(METHODS OF CACHING AVAILABLE)] (reference)
 * @{
 */
/**
 * @brief Strong Uncacheable (UC)
 *
 * System memory locations are not cached. All reads and writes appear on the system bus and are executed in program order
 * without reordering. No speculative memory accesses, pagetable walks, or prefetches of speculated branch targets are
 * made. This type of cache-control is useful for memory-mapped I/O devices. When used with normal RAM, it greatly reduces
 * processor performance.
 */
#define MEMORY_TYPE_UNCACHEABLE                                      0x00000000

/**
 * @brief Write Combining (WC)
 *
 * System memory locations are not cached (as with uncacheable memory) and coherency is not enforced by the processor's bus
 * coherency protocol. Speculative reads are allowed. Writes may be delayed and combined in the write combining buffer (WC
 * buffer) to reduce memory accesses. If the WC buffer is partially filled, the writes may be delayed until the next
 * occurrence of a serializing event; such as, an SFENCE or MFENCE instruction, CPUID execution, a read or write to
 * uncached memory, an interrupt occurrence, or a LOCK instruction execution. This type of cache-control is appropriate for
 * video frame buffers, where the order of writes is unimportant as long as the writes update memory so they can be seen on
 * the graphics display. This memory type is available in the Pentium Pro and Pentium II processors by programming the
 * MTRRs; or in processor families starting from the Pentium III processors by programming the MTRRs or by selecting it
 * through the PAT.
 *
 * @see Vol3A[11.3.1(Buffering of Write Combining Memory Locations)]
 */
#define MEMORY_TYPE_WRITE_COMBINING                                  0x00000001

/**
 * @brief Write-through (WT)
 *
 * Writes and reads to and from system memory are cached. Reads come from cache lines on cache hits; read misses cause
 * cache fills. Speculative reads are allowed. All writes are written to a cache line (when possible) and through to system
 * memory. When writing through to memory, invalid cache lines are never filled, and valid cache lines are either filled or
 * invalidated. Write combining is allowed. This type of cache-control is appropriate for frame buffers or when there are
 * devices on the system bus that access system memory, but do not perform snooping of memory accesses. It enforces
 * coherency between caches in the processors and system memory.
 */
#define MEMORY_TYPE_WRITE_THROUGH                                    0x00000004

/**
 * @brief Write protected (WP)
 *
 * Reads come from cache lines when possible, and read misses cause cache fills. Writes are propagated to the system bus
 * and cause corresponding cache lines on all processors on the bus to be invalidated. Speculative reads are allowed. This
 * memory type is available in processor families starting from the P6 family processors by programming the MTRRs.
 */
#define MEMORY_TYPE_WRITE_PROTECTED                                  0x00000005

/**
 * @brief Write-back (WB)
 *
 * Writes and reads to and from system memory are cached. Reads come from cache lines on cache hits; read misses cause
 * cache fills. Speculative reads are allowed. Write misses cause cache line fills (in processor families starting with the
 * P6 family processors), and writes are performed entirely in the cache, when possible. Write combining is allowed. The
 * write-back memory type reduces bus traffic by eliminating many unnecessary writes to system memory. Writes to a cache
 * line are not immediately forwarded to system memory; instead, they are accumulated in the cache. The modified cache
 * lines are written to system memory later, when a write-back operation is performed. Write-back operations are triggered
 * when cache lines need to be deallocated, such as when new cache lines are being allocated in a cache that is already
 * full. They also are triggered by the mechanisms used to maintain cache consistency. This type of cache-control provides
 * the best performance, but it requires that all devices that access system memory on the system bus be able to snoop
 * memory accesses to insure system memory and cache coherency.
 */
#define MEMORY_TYPE_WRITE_BACK                                       0x00000006

/**
 * @brief Uncacheable (UC-)
 *
 * Has same characteristics as the strong uncacheable (UC) memory type, except that this memory type can be overridden by
 * programming the MTRRs for the WC memory type. This memory type is available in processor families starting from the
 * Pentium III processors and can only be selected through the PAT.
 */
#define MEMORY_TYPE_UNCACHEABLE_MINUS                                0x00000007
#define MEMORY_TYPE_INVALID                                          0x000000FF
/**
 * @}
 */

/**
 * @}
 */
};

// Some missing enumerations.
//
// Po
#define IA32_PKG_ENERGY_STATUS          0x611
#define IA32_MSR_DRAM_ENERGY_STATUS     0x619
#define IA32_PPERF                      0x64E
// SMI
#define IA32_MSR_SMI_COUNT              0x00000034
// AMD profiling extensions.
#define IA32_IRPERF                     0x000000E9
#define IA32_PERFEVTSEL0_AMD            0xC0010000
#define IA32_PMC0_AMD                   0xC0010004
#define IA32_A_PERFEVTSEL0_AMD          0xC0010200
#define IA32_A_PMC0_AMD                 0xC0010201
// CPUD faulting.
#define MSR_INTEL_PLATFORM_INFO         0x000000ce
#define MSR_INTEL_MISC_FEATURES_ENABLES 0x00000140
#define PLATFORM_INFO_CPUID_FAULTING    (1ull << 31)
#define MISC_FEATURES_CPUID_FAULTING    (1ull <<  0)
// More complete EFER.
#define EFER_SCE                        (1ull <<  0) /* SYSCALL Enable */
#define EFER_LME                        (1ull <<  8) /* Long Mode Enable */
#define EFER_LMA                        (1ull << 10) /* Long Mode Active */
#define EFER_NXE                        (1ull << 11) /* No Execute Enable */
#define EFER_SVME                       (1ull << 12) /* Secure Virtual Machine Enable */
#define EFER_LMSLE                      (1ull << 13) /* Long Mode Segment Limit Enable */
#define EFER_FFXSR                      (1ull << 14) /* Fast FXSAVE/FXRSTOR */
#define EFER_TCE                        (1ull << 15) /* Translation Cache Extension */

// Fixed versions of some structures and some missing ones.
//
#pragma pack(push, 1)
namespace ia32
{
	// INVPCID descriptor.
	//
	struct invpcid_descriptor
	{
		uint64_t pcid : 12;
		uint64_t rsvd : 48;
		uint64_t address;
	};
	enum class invpcid_type : uint64_t
	{
		individual = 0, // Invalidates the Address in the specific TLB associated with the PCID.
		context =    1, // Invalidates the entire TLB associated with the PCID.
		global =     2, // Invalidates the entire TLB.
		local =      3, // Invalidates the entire TLB excluding global pages.
	};

	// Task state.
	//
	template<size_t N = 0>
	struct tss
	{
		uint32_t reserved_0;
		std::array<xstd::any_ptr, 3> rsp;
		std::array<xstd::any_ptr, 8> ist;
		uint64_t reserved_1;
		uint16_t reserved_2;
		uint16_t iopb_offset;
		uint8_t  iopb[ N ? 1 + ( ( N + 7 ) / 8 ) : 0 ]; // Bitmap to cover N I/O + trailing 0xFF.
	};

	// GDT entry.
	//
	struct gdt_entry
	{
		uint32_t limit_low     : 16;
		uint32_t offset_low    : 16;
		uint32_t offset_middle : 8;
		uint32_t ac            : 1; // accessed
		uint32_t rw            : 1; // read/write
		uint32_t dc            : 1; // direction/conforming
		uint32_t exec          : 1; // executable
		uint32_t s             : 1; // 0 if system segment (tss)
		uint32_t priv          : 2; // privilege level
		uint32_t present       : 1; // present
		uint32_t limit_high    : 4;
		uint32_t avl           : 1; // 0
		uint32_t l             : 1; // if code section, x86-64, else reserved = 0
		uint32_t sz            : 1; // if L||16bit = 0, else = 1
		uint32_t granularity   : 1; // if set, limit <<= 12
		uint32_t offset_high   : 8;

		FORCE_INLINE uint32_t get_limit() const
		{
			uint32_t lim = limit_low;
			lim |= uint32_t( limit_high ) << 16;
			if ( granularity ) return ( lim << 12 ) | 0xFFF;
			else return lim;
		}
		FORCE_INLINE void set_limit( uint32_t lim )
		{
			if ( lim > 0xFFFFF )
			{
				granularity = true;
				lim >>= 12;
			}
			limit_low = lim & 0xFFFF;
			limit_high = ( lim >> 16 ) & 0xF;
		}
		FORCE_INLINE uint32_t get_offset() const
		{
			uint32_t adr = offset_low;
			adr |= uint32_t( offset_middle ) << 16;
			adr |= uint32_t( offset_high ) << 24;
			return adr;
		}
		FORCE_INLINE void set_offset( uint32_t ptr )
		{
			offset_low = ptr & 0xFFFF;
			offset_middle = ( ptr >> 16 ) & 0xFF;
			offset_high = ( ptr >> 24 ) & 0xFF;
		}
	};
	static_assert( sizeof( gdt_entry ) == 8, "GDT entry is broken" );

	// LDT/TSS entry.
	//
	struct gdt_entry_ex
	{
		uint32_t limit_low       : 16;
		uint32_t offset_low      : 16;
		uint32_t offset_middle_0 : 8;
		uint32_t type            : 4;  // type
		uint32_t s               : 1;  // 0 if system segment (tss)
		uint32_t priv            : 2;  // privilege level
		uint32_t present         : 1;  // present
		uint32_t limit_high      : 4;
		uint32_t avl             : 1;  // 0
		uint32_t reserved_0      : 2;  // 0
		uint32_t granularity     : 1;  // if set, limit <<= 12
		uint32_t offset_middle_1 : 8;
		uint64_t offset_high     : 32;
		uint64_t reserved_1      : 32; // 0

		FORCE_INLINE uint32_t get_limit() const
		{
			uint32_t lim = limit_low;
			lim |= uint32_t( limit_high ) << 16;
			if ( granularity ) return lim << 12;
			else return lim;
		}
		FORCE_INLINE void set_limit( uint32_t lim )
		{
			if ( lim > 0xFFFFF )
			{
				granularity = true;
				lim >>= 12;
			}
			limit_low = lim & 0xFFFF;
			limit_high = ( lim >> 16 ) & 0xF;
		}
		FORCE_INLINE xstd::any_ptr get_offset() const
		{
			//uint64_t adr = offset_low;
			//adr |= uint64_t( offset_middle_0 ) << 16;
			//adr |= uint64_t( offset_middle_1 ) << 24;
			//adr |= uint64_t( offset_high ) << 32;
			uint64_t adr = xstd::ref_at<uint32_t>( this, 2 ) & 0xFFFFFF;
			adr |= xstd::ref_at<uint64_t>( this, 7 ) << 24;
			return adr;
		}
		FORCE_INLINE void set_offset( xstd::any_ptr ptr )
		{
			offset_low = ptr & 0xFFFF;
			offset_middle_0 = ( ptr >> 16 ) & 0xFF;
			offset_middle_1 = ( ptr >> 24 ) & 0xFF;
			offset_high = ( ptr >> 32 ) & 0xFFFFFFFF;
		}
	};
	using tss_entry = gdt_entry_ex;
	using ldt_entry = gdt_entry_ex;
	static_assert( sizeof( tss_entry ) == 16, "TSS entry is broken" );

	// IDT entry.
	//
	struct idt_entry
	{
		uint16_t offset_low;
		uint16_t selector;          // segment selector in gdt/ldt
		uint8_t  ist_index     : 3; // interrupt stack table index
		uint8_t  reserved_0    : 5;
		uint8_t  type          : 4; // gate type
		uint8_t  reserved_1    : 1;
		uint8_t  priv          : 2; // privilege level
		uint8_t  present       : 1;
		uint16_t offset_middle;
		uint32_t offset_high;
		uint32_t reserved_2;        // 0

		FORCE_INLINE xstd::any_ptr get_handler() const
		{
			//uint64_t adr = offset_low;
			//adr |= uint64_t( offset_middle ) << 16;
			//adr |= uint64_t( offset_high ) << 32;
			uint64_t adr = offset_low;
			adr |= xstd::ref_at<uint64_t>( this, 6 ) << 16;
			return adr;
		}
		FORCE_INLINE void set_handler( xstd::any_ptr ptr )
		{
			offset_low = ptr & 0xFFFF;
			offset_middle = ( ptr >> 16 ) & 0xFFFF;
			offset_high = ( ptr >> 32 ) & 0xFFFFFFFF;
		}
	};
	static_assert( sizeof( idt_entry ) == 16, "IDT entry is broken" );
};
#pragma pack(pop)

#if __clang__
// This namespace provides IA32 intrinsics that Clang decided not to.
//

using irql_t = uint64_t;
#define PASSIVE_LEVEL 0                 // Passive release level
#define LOW_LEVEL 0                     // Lowest interrupt level
#define APC_LEVEL 1                     // APC interrupt level
#define DISPATCH_LEVEL 2                // Dispatcher level
#define CMCI_LEVEL 5                    // CMCI handler level

#define CLOCK_LEVEL 13                  // Interval clock level
#define IPI_LEVEL 14                    // Interprocessor interrupt level
#define DRS_LEVEL 14                    // Deferred Recovery Service level
#define POWER_LEVEL 14                  // Power failure level
#define PROFILE_LEVEL 15                // Timer used for profiling.
#define HIGH_LEVEL 15                   // Highest interrupt level
#define NO_INTERRUPTS 0x200             // <--- Flag

#define CHECK_IRQL( ... ) dassert( ia32::get_effective_irql() __VA_ARGS__ )

#define _LINKAGE FORCE_INLINE static

namespace ia32
{
	static constexpr size_t page_size = 1ull << 12;
	static constexpr size_t cacheline_size = 64;

	// Read/write control registers.
	//
#define _EXPOSE_REG(reg, t, f, s)                              \
	_LINKAGE t read_##reg()                                     \
	{                                                           \
		t value;                                                 \
		asm volatile( "mov %%" #reg ", %0" : "=r" ( f ) :: s );  \
		return value;                                            \
	}                                                           \
	_LINKAGE void write_##reg( t value )                        \
	{                                                           \
		asm volatile( "mov %0, %%" #reg :: "r" ( f ) : s );      \
	}
	_EXPOSE_REG( cr0, cr0,            value.flags,            );
	_EXPOSE_REG( cr2, xstd::any_ptr,  value.address,          );
	_EXPOSE_REG( cr3, cr3,            value.flags,   "memory" );
	_EXPOSE_REG( cr4, cr4,            value.flags,   "memory" );
	_EXPOSE_REG( cr8, uint64_t,       value,                  );
	_EXPOSE_REG( dr0, xstd::any_ptr,  value.address,          );
	_EXPOSE_REG( dr1, xstd::any_ptr,  value.address,          );
	_EXPOSE_REG( dr2, xstd::any_ptr,  value.address,          );
	_EXPOSE_REG( dr3, xstd::any_ptr,  value.address,          );
	_EXPOSE_REG( dr4, dr6,            value.flags,            );
	_EXPOSE_REG( dr6, dr6,            value.flags,            );
	_EXPOSE_REG( dr5, dr7,            value.flags,            );
	_EXPOSE_REG( dr7, dr7,            value.flags,            );
#undef _EXPOSE_REG

	// Further Cr0 wrappers.
	//
	_LINKAGE void clts() { asm volatile( "clts" ); }
	_LINKAGE ia32::cr0 smsw()
	{
		uint64_t value;
		asm volatile( "smsw %0" : "=r" ( value ) );
		return { .flags = value };
	}
	_LINKAGE void lmsw( ia32::cr0 value )
	{
		uint16_t partial_value = ( uint16_t ) ( value.flags & 0b1111 );
		asm volatile( "lmsw %0" :: "r" ( partial_value ) );
	}

	// CPUID:
	//
	template<typename T = std::array<uint32_t, 4>>
	_LINKAGE T query_cpuid( uint64_t leaf, uint64_t subleaf = 0 )
	{
		static_assert( sizeof( T ) == ( 4 * 4 ), "Invalid type size." );

		uint32_t info[ 4 ];
		asm volatile(
			"movq %%rbx, %%rsi;"
			"cpuid;"
			"xchgq %%rsi, %%rbx;"
			: "=a"( info[ 0 ] ), "=S"( info[ 1 ] ), "=c"( info[ 2 ] ), "=d"( info[ 3 ] )
			: "a"( leaf ), "c"( subleaf )
			);
		return *( T* ) &info[ 0 ];
	}
	template<uint64_t leaf, uint64_t subleaf = 0, typename T = std::array<uint32_t, 4>>
	struct static_cpuid
	{
		inline static const auto result = query_cpuid<T>( leaf, subleaf );
	};
	template<uint64_t leaf, uint64_t subleaf = 0, typename T = std::array<uint32_t, 4>>
	struct static_cpuid_s
	{
		inline static const T result = [ ] () -> T
		{
			if ( query_cpuid<cpuid_eax_00>( 0 ).max_cpuid_input_value >= leaf )
				return query_cpuid<T>( leaf, subleaf );
			else
				return T{};
		}( );
	};

	// Wrappers around EFLAGS.
	//
	_LINKAGE rflags read_flags()
	{
		return { .flags = __readeflags() };
	}
	_LINKAGE void write_flags( rflags flags )
	{
		__writeeflags( flags.flags );
	}
	_LINKAGE void set_ac( bool flag )
	{
		if ( flag )
			asm volatile( "stac" ::: "flags" );
		else
			asm volatile( "clac" ::: "flags" );
	}
	_LINKAGE void set_df( bool flag )
	{
		if ( flag )
			asm volatile( "std" ::: "flags" );
		else
			asm volatile( "cld" ::: "flags" );
	}
	_LINKAGE void set_cf( bool flag )
	{
		if ( flag )
			asm volatile( "stc" ::: "flags" );
		else
			asm volatile( "clc" ::: "flags" );
	}
	_LINKAGE bool get_cf()
	{
		int flag;
		asm volatile( "" : "=@ccc" ( flag ) :: );
		return flag;
	}
	_LINKAGE bool get_sf()
	{
		int flag;
		asm volatile( "" : "=@ccs" ( flag ) :: );
		return flag;
	}
	_LINKAGE bool get_of()
	{
		int flag;
		asm volatile( "" : "=@cco" ( flag ) :: );
		return flag;
	}
	_LINKAGE bool get_zf()
	{
		int flag;
		asm volatile( "" : "=@ccz" ( flag ) :: );
		return flag;
	}
	_LINKAGE bool get_pf()
	{
		int flag;
		asm volatile( "" : "=@ccp" ( flag ) :: );
		return flag;
	}

	// MXCSR helpers.
	//
	_LINKAGE uint32_t read_mxcsr()
	{
		uint32_t value;
		asm volatile( "stmxcsr %0" :: "m" ( value ) );
		return value;
	}
	_LINKAGE void write_mxcsr( const uint32_t& value )
	{
		asm volatile( "ldmxcsr %0" :: "m" ( value ) );
	}

	// GS/FS base wrappers.
	//
	_LINKAGE xstd::any_ptr read_gsbase()
	{
		uint64_t value;
		asm volatile( "rdgsbase %0" : "=r" ( value ) :: );
		return value;
	}
	_LINKAGE void write_gsbase( xstd::any_ptr value )
	{
		asm volatile( "wrgsbase %0" :: "r" ( value.address ) : );
	}
	_LINKAGE xstd::any_ptr read_fsbase()
	{
		uint64_t value;
		asm volatile( "rdfsbase %0" : "=r" ( value ) :: );
		return value;
	}
	_LINKAGE void write_fsbase( xstd::any_ptr value )
	{
		asm volatile( "wrfsbase %0" :: "r" ( value.address ) : );
	}
	_LINKAGE uint32_t read_gsbase32()
	{
		uint32_t value;
		asm volatile( "rdgsbase %0" : "=r" ( value ) :: );
		return value;
	}
	_LINKAGE void write_gsbase32( uint32_t value )
	{
		asm volatile( "wrgsbase %0" :: "r" ( value ) : );
	}
	_LINKAGE uint32_t read_fsbase32()
	{
		uint32_t value;
		asm volatile( "rdfsbase %0" : "=r" ( value ) :: );
		return value;
	}
	_LINKAGE void write_fsbase32( uint32_t value )
	{
		asm volatile( "wrfsbase %0" :: "r" ( value ) : );
	}
	_LINKAGE void swapgs()
	{
		asm volatile( "swapgs" ::: "memory" );
	}

	// Timestamping / Performance counters.
	//
	_LINKAGE uint64_t read_tsc()
	{
		register uint32_t low asm( "eax" );
		register uint32_t high asm( "edx" );
		asm volatile( "rdtsc" : "=r" ( low ), "=r" ( high ) :: );
		return low | ( uint64_t( high ) << 32 );
	}
	_LINKAGE std::pair<uint64_t, uint32_t> read_tscp()
	{
		register uint32_t low asm( "eax" );
		register uint32_t high asm( "edx" );
		register uint32_t pid asm( "ecx" );
		asm volatile( "rdtscp" : "=r" ( low ), "=r" ( pid ), "=r" ( high ) :: );
		return { low | ( uint64_t( high ) << 32 ), pid };
	}
	_LINKAGE uint64_t read_pmc( uint64_t id, bool fixed = false, bool fast = false )
	{
		if ( fast ) id |= 1ull << 31;
		if ( fixed ) id |= 1ull << 30;

		register uint32_t low asm( "eax" );
		register uint32_t high asm( "edx" );
		register uint64_t _id asm( "rcx" ) = id;
		asm volatile( "rdpmc" : "=r" ( low ), "=r" ( high ) : "r" ( _id ) );
		return low | ( uint64_t( high ) << 32 );
	}
	_LINKAGE uint32_t read_pcid()
	{
		register uint32_t pid asm( "ecx" );
		asm volatile( "rdtscp" : "=r" ( pid ) :: "rax", "rdx" );
		return pid;
	}

	// Read write model-specific registers.
	//
	template<typename T = uint64_t> requires ( sizeof( T ) <= 8 )
	_LINKAGE T read_msr( uint64_t id )
	{
		register uint32_t low asm( "eax" );
		register uint32_t high asm( "edx" );
		register uint64_t _id asm( "rcx" ) = id;
		asm volatile( "rdmsr" : "=r" ( low ), "=r" ( high ) : "r" ( _id ) );
		uint64_t value = low | ( uint64_t( high ) << 32 );
		return *( T* ) &value;
	}
	template<typename T = uint64_t> requires ( sizeof( T ) <= 8 )
	_LINKAGE void write_msr( uint64_t id, T tvalue )
	{
		uint64_t value = 0;
		memcpy( &value, &tvalue, sizeof( T ) );
		register uint32_t low asm( "eax" ) = uint32_t( value );
		register uint32_t high asm( "edx" ) = uint32_t( value >> 32 );
		register uint64_t _id asm( "rcx" ) = id;
		asm volatile( "wrmsr" :: "r" ( low ), "r" ( high ), "r" ( _id ) );
	}

	// Extended control registers.
	//
	template<typename T = uint64_t> requires ( sizeof( T ) <= 8 )
	_LINKAGE T read_xcr( uint64_t id )
	{
		register uint32_t low asm( "eax" );
		register uint32_t high asm( "edx" );
		register uint64_t _id asm( "rcx" ) = id;
		asm volatile( "xgetbv" : "=r" ( low ), "=r" ( high ) : "r" ( _id ) : );
		uint64_t value = low | ( uint64_t( high ) << 32 );
		return *( T* ) &value;
	}
	template<typename T = uint64_t> requires ( sizeof( T ) <= 8 )
	_LINKAGE void write_xcr( uint64_t id, T tvalue )
	{
		uint64_t value = 0;
		memcpy( &value, &tvalue, sizeof( T ) );
		register uint32_t low asm( "eax" ) = uint32_t( value );
		register uint32_t high asm( "edx" ) = uint32_t( value >> 32 );
		register uint64_t _id asm( "rcx" ) = id;
		asm volatile( "xsetbv" :: "r" ( low ), "r" ( high ), "r" ( _id ) );
	}

	// Memory intrinsics.
	//
	_LINKAGE void invlpg( xstd::any_ptr ptr ) { asm volatile( "invlpg %0" :: "m" ( *( char* ) ptr.address ) : "memory" ); }
	_LINKAGE void invpcid( invpcid_type type, uint64_t pcid = 0, xstd::any_ptr ptr = nullptr )
	{
		invpcid_descriptor desc = { .pcid = pcid, .rsvd = 0, .address = ptr };
		asm volatile( "invpcid %0, %1" :: "m" ( desc ), "r" ( type ) : "memory" );
	}
	_LINKAGE void touch( xstd::any_ptr ptr )
	{
		uint8_t temp;
		asm volatile( "movb %1, %0" : "=r" ( temp ) : "m" ( *( char* ) ptr.address ) : );
	}
	_LINKAGE void wtouch( xstd::any_ptr ptr )
	{
		asm volatile( "orb $0, %0" :: "m" ( *( char* ) ptr.address ) : "flags" );
	}
	_LINKAGE void flush_tlb()
	{
		if ( static_cpuid_s<7, 0, cpuid_eax_07>::result.ebx.invpcid )
			return invpcid( invpcid_type::global, 0, nullptr );

		auto cr4 = read_cr4();
		if ( cr4.page_global_enable )
		{
			auto cr4_2 = cr4;
			cr4_2.page_global_enable = 0;
			write_cr4( cr4_2 );
			write_cr4( cr4 );
		}
		else
		{
			write_cr3( read_cr3() );
		}
	}

	// String operations.
	//
	template<typename T = uint8_t>
	_LINKAGE void store_string( xstd::any_ptr dst, T value, size_t count )
	{
		if constexpr ( sizeof( T ) == 1 )
			asm volatile( "cld; rep stosb" : "+D" ( dst.address ), "+c" ( count ) : "a" ( value ) : "memory", "flags" );
		else if constexpr ( sizeof( T ) == 2 )
			asm volatile( "cld; rep stosw" : "+D" ( dst.address ), "+c" ( count ) : "a" ( value ) : "memory", "flags" );
		else if constexpr ( sizeof( T ) == 4 )
			asm volatile( "cld; rep stosl" : "+D" ( dst.address ), "+c" ( count ) : "a" ( value ) : "memory", "flags" );
		else if constexpr ( sizeof( T ) == 8 )
			asm volatile( "cld; rep stosq" : "+D" ( dst.address ), "+c" ( count ) : "a" ( value ) : "memory", "flags" );
		else
			unreachable();
	}
	template<typename T = uint8_t>
	_LINKAGE void copy_string( xstd::any_ptr dst, xstd::any_ptr src, size_t count )
	{
		if constexpr ( sizeof( T ) == 1 )
			asm volatile( "cld; rep movsb" : "+D" ( dst.address ), "+c" ( count ), "+S" ( src.address ) :: "memory", "flags" );
		else if constexpr ( sizeof( T ) == 2 )
			asm volatile( "cld; rep movsw" : "+D" ( dst.address ), "+c" ( count ), "+S" ( src.address ) :: "memory", "flags" );
		else if constexpr ( sizeof( T ) == 4 )
			asm volatile( "cld; rep movsl" : "+D" ( dst.address ), "+c" ( count ), "+S" ( src.address ) :: "memory", "flags" );
		else if constexpr ( sizeof( T ) == 8 )
			asm volatile( "cld; rep movsq" : "+D" ( dst.address ), "+c" ( count ), "+S" ( src.address ) :: "memory", "flags" );
		else
			unreachable();
	}
	template<typename T = uint8_t>
	_LINKAGE T load_string( xstd::any_ptr src, size_t count )
	{
		T value;
		if constexpr ( sizeof( T ) == 1 )
			asm volatile( "cld; rep lodsb" : "+S" ( src.address ), "+c" ( count ) :: "flags" );
		else if constexpr ( sizeof( T ) == 2 )
			asm volatile( "cld; rep lodsw" : "+S" ( src.address ), "+c" ( count ) :: "flags" );
		else if constexpr ( sizeof( T ) == 4 )
			asm volatile( "cld; rep lodsl" : "+S" ( src.address ), "+c" ( count ) :: "flags" );
		else if constexpr ( sizeof( T ) == 8 )
			asm volatile( "cld; rep lodsq" : "+S" ( src.address ), "+c" ( count ) :: "flags" );
		else
			unreachable();
		return value;
	}
	template<typename T = uint8_t>
	_LINKAGE xstd::any_ptr find_string( xstd::any_ptr begin, T value, size_t count )
	{
		xstd::any_ptr it = begin;
		uint8_t zero_flag;
		if constexpr ( sizeof( T ) == 1 )
			asm volatile( "cld; repne scasb" : "=@ccz" ( zero_flag ), "+D" ( it.address ), "+c" ( count ) : "a" ( value ) : "flags" );
		else if constexpr ( sizeof( T ) == 2 )
			asm volatile( "cld; repne scasw" : "=@ccz" ( zero_flag ), "+D" ( it.address ), "+c" ( count ) : "a" ( value ) : "flags" );
		else if constexpr ( sizeof( T ) == 4 )
			asm volatile( "cld; repne scasl" : "=@ccz" ( zero_flag ), "+D" ( it.address ), "+c" ( count ) : "a" ( value ) : "flags" );
		else if constexpr ( sizeof( T ) == 8 )
			asm volatile( "cld; repne scasq" : "=@ccz" ( zero_flag ), "+D" ( it.address ), "+c" ( count ) : "a" ( value ) : "flags" );
		else
			unreachable();
		return ( ( T* ) it ) - zero_flag;
	}
	template<typename T = uint8_t>
	_LINKAGE xstd::any_ptr find_string_not( xstd::any_ptr begin, T value, size_t count )
	{
		xstd::any_ptr it = begin;
		uint8_t zero_flag;
		if constexpr ( sizeof( T ) == 1 )
			asm volatile( "cld; repe scasb" : "=@ccz" ( zero_flag ), "+D" ( it.address ), "+c" ( count ) : "a" ( value ) : "flags" );
		else if constexpr ( sizeof( T ) == 2 )
			asm volatile( "cld; repe scasw" : "=@ccz" ( zero_flag ), "+D" ( it.address ), "+c" ( count ) : "a" ( value ) : "flags" );
		else if constexpr ( sizeof( T ) == 4 )
			asm volatile( "cld; repe scasl" : "=@ccz" ( zero_flag ), "+D" ( it.address ), "+c" ( count ) : "a" ( value ) : "flags" );
		else if constexpr ( sizeof( T ) == 8 )
			asm volatile( "cld; repe scasq" : "=@ccz" ( zero_flag ), "+D" ( it.address ), "+c" ( count ) : "a" ( value ) : "flags" );
		else
			unreachable();
		return ( ( T* ) it ) - !zero_flag;
	}
	template<typename T = uint8_t>
	_LINKAGE xstd::any_ptr string_match( xstd::any_ptr a, xstd::any_ptr b, size_t count )
	{
		uint8_t zero_flag;
		if constexpr ( sizeof( T ) == 1 )
			asm volatile( "cld; repe cmpsb" : "=@ccz" ( zero_flag ), "+D" ( a.address ), "+c" ( count ), "+S" ( b.address ) :: "flags" );
		else if constexpr ( sizeof( T ) == 2 )
			asm volatile( "cld; repe cmpsw" : "=@ccz" ( zero_flag ), "+D" ( a.address ), "+c" ( count ), "+S" ( b.address ) :: "flags" );
		else if constexpr ( sizeof( T ) == 4 )
			asm volatile( "cld; repe cmpsl" : "=@ccz" ( zero_flag ), "+D" ( a.address ), "+c" ( count ), "+S" ( b.address ) :: "flags" );
		else if constexpr ( sizeof( T ) == 8 )
			asm volatile( "cld; repe cmpsq" : "=@ccz" ( zero_flag ), "+D" ( a.address ), "+c" ( count ), "+S" ( b.address ) :: "flags" );
		else
			unreachable();
		return ( ( T* ) a ) - !zero_flag;
	}
	template<typename T = uint8_t>
	_LINKAGE xstd::any_ptr string_mismatch( xstd::any_ptr a, xstd::any_ptr b, size_t count )
	{
		uint8_t zero_flag;
		if constexpr ( sizeof( T ) == 1 )
			asm volatile( "cld; repne cmpsb" : "=@ccz" ( zero_flag ), "+D" ( a.address ), "+c" ( count ), "+S" ( b.address ) :: "flags" );
		else if constexpr ( sizeof( T ) == 2 )
			asm volatile( "cld; repne cmpsw" : "=@ccz" ( zero_flag ), "+D" ( a.address ), "+c" ( count ), "+S" ( b.address ) :: "flags" );
		else if constexpr ( sizeof( T ) == 4 )
			asm volatile( "cld; repne cmpsl" : "=@ccz" ( zero_flag ), "+D" ( a.address ), "+c" ( count ), "+S" ( b.address ) :: "flags" );
		else if constexpr ( sizeof( T ) == 8 )
			asm volatile( "cld; repne cmpsq" : "=@ccz" ( zero_flag ), "+D" ( a.address ), "+c" ( count ), "+S" ( b.address ) :: "flags" );
		else
			unreachable();
		return ( ( T* ) a ) - zero_flag;
	}

	// Cache intrinsics.
	//
	_LINKAGE void invd() { asm volatile( "invd" ::: "memory" ); }
	_LINKAGE void wbinvd() { asm volatile( "wbinvd" ::: "memory" ); }
	_LINKAGE void clwb( xstd::any_ptr ptr ) { asm volatile( "clwb %0":: "m" ( *( char* ) ptr.address ) : "memory" ); }
	_LINKAGE void clflush( xstd::any_ptr ptr ) { asm volatile( "clflush %0":: "m" ( *( char* ) ptr.address ) : "memory" ); }
	_LINKAGE void cldemote( xstd::any_ptr ptr ) { asm volatile( "cldemote %0":: "m" ( *( char* ) ptr.address ) : "memory" ); }
	_LINKAGE void clflushopt( xstd::any_ptr ptr ) { asm volatile( "clflushopt %0":: "m" ( *( char* ) ptr.address ) : "memory" ); }
	_LINKAGE void prefetch0( xstd::any_ptr ptr ) { asm volatile( "prefetcht0 %0":: "m" ( *( char* ) ptr.address ) : "memory" ); }
	_LINKAGE void prefetch1( xstd::any_ptr ptr ) { asm volatile( "prefetcht1 %0":: "m" ( *( char* ) ptr.address ) : "memory" ); }
	_LINKAGE void prefetch2( xstd::any_ptr ptr ) { asm volatile( "prefetcht2 %0":: "m" ( *( char* ) ptr.address ) : "memory" ); }
	_LINKAGE void prefetchnt( xstd::any_ptr ptr ) { asm volatile( "prefetchnta %0":: "m" ( *( char* ) ptr.address ) : "memory" ); }

	_LINKAGE void clflushopt_s( xstd::any_ptr ptr )
	{
		if ( static_cpuid_s<7, 0, cpuid_eax_07>::result.ebx.clflushopt )
			clflushopt( ptr );
		else
			clflush( ptr );
	}
	_LINKAGE void clwb_s( xstd::any_ptr ptr )
	{
		if ( static_cpuid_s<7, 0, cpuid_eax_07>::result.ebx.clwb )
			clwb( ptr );
		else
			clflush( ptr );
	}

	// Implement the range helpers.
	//
	namespace impl
	{
		template<size_t U, typename T>
		_LINKAGE inline void unroll_for( const T& fn, xstd::any_ptr ptr, size_t count, size_t granularity )
		{
			size_t n = ( count + ( granularity - 1 ) ) / granularity;
			for ( ; n >= U; n -= U )
			{
				__hint_unroll()
				for ( size_t i = 0; i != U; i++ )
					fn( ptr ), ptr += granularity;
			}

			for ( ; n > 0; n-- )
				fn( ptr ), ptr += granularity;
		}
	}
	_LINKAGE void invpcid( uint64_t pcid, xstd::any_ptr ptr, size_t n, size_t p = page_size )
	{
		impl::unroll_for<8>( [ pcid ] ( xstd::any_ptr it ) { invpcid( invpcid_type::individual, pcid, it ); }, ptr, n, p );
	}
	_LINKAGE void invlpg( xstd::any_ptr ptr, size_t n, size_t p = page_size )
	{
		impl::unroll_for<8>( [ ] ( xstd::any_ptr it ) { invlpg( it ); }, ptr, n, p );
	}
	_LINKAGE void touch( xstd::any_ptr ptr, size_t n, size_t p = page_size )
	{
		impl::unroll_for<8>( [ ] ( xstd::any_ptr it ) { touch( it ); }, ptr, n, p );
	}
	_LINKAGE void wtouch( xstd::any_ptr ptr, size_t n, size_t p = page_size )
	{
		impl::unroll_for<8>( [ ] ( xstd::any_ptr it ) { wtouch( it ); }, ptr, n, p );
	}
	_LINKAGE void clwb( xstd::any_ptr ptr, size_t n, size_t cl = cacheline_size )
	{
		impl::unroll_for<8>( [ ] ( xstd::any_ptr it ) { clwb( it ); }, ptr, n, cl );
	}
	_LINKAGE void clflush( xstd::any_ptr ptr, size_t n, size_t cl = cacheline_size )
	{
		impl::unroll_for<8>( [ ] ( xstd::any_ptr it ) { clflush( it ); }, ptr, n, cl );
	}
	_LINKAGE void cldemote( xstd::any_ptr ptr, size_t n, size_t cl = cacheline_size )
	{
		impl::unroll_for<8>( [ ] ( xstd::any_ptr it ) { cldemote( it ); }, ptr, n, cl );
	}
	_LINKAGE void clflushopt( xstd::any_ptr ptr, size_t n, size_t cl = cacheline_size )
	{
		impl::unroll_for<8>( [ ] ( xstd::any_ptr it ) { clflushopt( it ); }, ptr, n, cl );
	}
	_LINKAGE void clflushopt_s( xstd::any_ptr ptr, size_t n )
	{
		if ( static_cpuid_s<7, 0, cpuid_eax_07>::result.ebx.clflushopt )
			clflushopt( ptr, n );
		else
			clflush( ptr, n );
	}
	_LINKAGE void clwb_s( xstd::any_ptr ptr, size_t n )
	{
		if ( static_cpuid_s<7, 0, cpuid_eax_07>::result.ebx.clwb )
			clwb( ptr, n );
		else
			clflush( ptr, n );
	}

	// IDT/GDT.
	//
	_LINKAGE void write_idtr( const void* ptr ) { asm volatile( "lidt %0" :: "m" ( *( const segment_descriptor_register_64* ) ptr ) ); }
	_LINKAGE void write_gdtr( const void* ptr ) { asm volatile( "lgdt %0" :: "m" ( *( const segment_descriptor_register_64* ) ptr ) ); }
	_LINKAGE void read_idtr( void* ptr ) { asm volatile( "sidt %0" : "=m" ( *( segment_descriptor_register_64* ) ptr ) :: ); }
	_LINKAGE void read_gdtr( void* ptr ) { asm volatile( "sgdt %0" : "=m" ( *( segment_descriptor_register_64* ) ptr ) :: ); }

	_LINKAGE void write_idtr( segment_descriptor_register_64 desc ) { write_idtr( &desc ); }
	_LINKAGE void write_gdtr( segment_descriptor_register_64 desc ) { write_gdtr( &desc ); }
	_LINKAGE segment_descriptor_register_64 read_idtr() { segment_descriptor_register_64 r; read_idtr( &r ); return r; }
	_LINKAGE segment_descriptor_register_64 read_gdtr() { segment_descriptor_register_64 r; read_gdtr( &r ); return r; }

	_LINKAGE std::pair<idt_entry*, size_t> get_idt() 
	{ 
		auto desc = read_idtr(); 
		return { xstd::any_ptr( desc.base_address ), ( size_t( desc.limit ) + 1 ) / sizeof( idt_entry ) }; 
	}
	_LINKAGE std::pair<gdt_entry*, size_t> get_gdt() 
	{ 
		auto desc = read_gdtr(); 
		return { xstd::any_ptr( desc.base_address ), ( size_t( desc.limit ) + 1 ) / sizeof( gdt_entry ) }; 
	}
	_LINKAGE void set_idt( xstd::any_ptr base_address, size_t length ) 
	{ 
		write_idtr( { uint16_t( length * sizeof( idt_entry ) - 1 ), base_address } );
	}
	_LINKAGE void set_gdt( xstd::any_ptr base_address, size_t length ) 
	{ 
		write_gdtr( { uint16_t( length * sizeof( gdt_entry ) - 1 ), base_address } );
	}

	// Segment selectors.
	//
	_LINKAGE segment_selector get_cs() { segment_selector value; asm volatile( "mov %%cs, %0" : "=r" ( value.flags ) :: ); return value; }
	_LINKAGE segment_selector get_ss() { segment_selector value; asm volatile( "mov %%ss, %0" : "=r" ( value.flags ) :: ); return value; }
	_LINKAGE segment_selector get_ds() { segment_selector value; asm volatile( "mov %%ds, %0" : "=r" ( value.flags ) :: ); return value; }
	_LINKAGE segment_selector get_es() { segment_selector value; asm volatile( "mov %%es, %0" : "=r" ( value.flags ) :: ); return value; }
	_LINKAGE segment_selector get_fs() { segment_selector value; asm volatile( "mov %%fs, %0" : "=r" ( value.flags ) :: ); return value; }
	_LINKAGE segment_selector get_gs() { segment_selector value; asm volatile( "mov %%gs, %0" : "=r" ( value.flags ) :: ); return value; }
	_LINKAGE segment_selector get_tr() { segment_selector value; asm volatile( "str %0" : "=r" ( value.flags ) :: ); return value; }
	_LINKAGE segment_selector get_ldtr() { segment_selector value; asm volatile( "sldt %0" : "=r" ( value.flags ) :: ); return value; }
	_LINKAGE void set_cs( segment_selector value ) { asm volatile( "mov %0, %%cs" :: "r" ( value.flags ) : ); }
	_LINKAGE void set_ss( segment_selector value ) { asm volatile( "mov %0, %%ss" :: "r" ( value.flags ) : ); }
	_LINKAGE void set_ds( segment_selector value ) { asm volatile( "mov %0, %%ds" :: "r" ( value.flags ) : ); }
	_LINKAGE void set_es( segment_selector value ) { asm volatile( "mov %0, %%es" :: "r" ( value.flags ) : ); }
	_LINKAGE void set_fs( segment_selector value ) { asm volatile( "mov %0, %%fs" :: "r" ( value.flags ) : ); }
	_LINKAGE void set_gs( segment_selector value ) { asm volatile( "mov %0, %%gs" :: "r" ( value.flags ) : ); }
	_LINKAGE void set_tr( segment_selector value ) { asm volatile( "ltr %0" :: "r" ( value.flags ) : ); }
	_LINKAGE void set_ldtr( segment_selector value ) { asm volatile( "lldt %0" :: "r" ( value.flags ) : ); }

	template<typename T = uint64_t>
	_LINKAGE T load_seg_limit( segment_selector value )
	{
		T limit = 0;
		asm volatile( "lsl %1, %0" : "=r" ( limit ) : "r" ( ( uint32_t ) value.flags ) );
		return limit;
	}
	template<typename T = uint32_t>
	_LINKAGE T load_access_rights( segment_selector value )
	{
		T desc = 0;
		asm volatile( "lar %1, %0" : "=r" ( desc ) : "r" ( ( uint32_t ) value.flags ) );
		return desc;
	}
	_LINKAGE bool verify_seg_read( segment_selector value )
	{
		int flag;
		asm volatile( "verr %1" : "=@ccz" ( flag ) : "r" ( value.flags ) : );
		return flag;
	}
	_LINKAGE bool verify_seg_write( segment_selector value )
	{
		int flag;
		asm volatile( "verw %1" : "=@ccz" ( flag ) : "r" ( value.flags ) : );
		return flag;
	}

	// Serialization intrinsics.
	//
	_LINKAGE void lfence() { asm volatile( "lfence" ::: "memory" ); }
	_LINKAGE void sfence() { asm volatile( "sfence" ::: "memory" ); }
	_LINKAGE void mfence() { asm volatile( "mfence" ::: "memory" ); }
	_LINKAGE void serialize()
	{
		// CLTS is the lowest-cost serializing instruction (3 uOps) when in kernel-mode,
		// and move to SS is the lowest-cost when in user-mode; assuming we can't guarantee 
		// serializing LFENCE behaviour. Checking host support and branching for this reason 
		// does not make sense either given the high performance scenarios this is used where 
		// the timings matter.
		//
		// This choice can however be made by the [[Configuration]] XSTD_SERIALIZING_LFENCE.
		//
#ifndef XSTD_SERIALIZING_LFENCE
	#define XSTD_SERIALIZING_LFENCE 0
#endif

#if !XSTD_SERIALIZING_LFENCE
		if constexpr ( is_kernel_mode() )
			clts();
		else
			set_ss( get_ss() );
#else
		lfence();
#endif
	}

	// IP/SP.
	//
	_LINKAGE xstd::any_ptr get_sp()
	{
		uint64_t out;
		asm( "movq %%rsp, %0": "=r" ( out ) );
		return out;
	}
	_LINKAGE xstd::any_ptr get_ip()
	{
		uint64_t out;
		asm volatile( "0: leaq (%%rip), %0" : "=r" ( out ) );
		return out;
	}

	// Random number generation.
	//
	template<typename T = uint64_t>
	_LINKAGE T rdrand()
	{
		T out;
		asm volatile( R"(
			0:
			rdrand %0
			jnc 0b
		)": "=r" ( out ) );
		return out;
	}
	template<typename T = uint64_t>
	_LINKAGE T rdseed()
	{
		T out;
		asm volatile( R"(
			0:
			rdseed %0
			jnc 0b
		)": "=r" ( out ) );
		return out;
	}

	// I/O ports.
	//
	template<typename T>
	_LINKAGE T read_io( uint16_t adr )
	{
		register uint32_t result asm( "eax" ) = 0;
		register uint16_t adr_reg asm( "dx" ) = adr;
		if constexpr ( sizeof( T ) == 4 )
			asm volatile( "in %%dx, %%eax" : "=r" ( result ) : "r" ( adr_reg ) );
		else if constexpr ( sizeof( T ) == 2 )
			asm volatile( "in %%dx, %%ax" : "=r" ( result ) : "r" ( adr_reg ) );
		else if constexpr ( sizeof( T ) == 1 )
			asm volatile( "in %%dx, %%al" : "=r" ( result ) : "r" ( adr_reg ) );
		else
			unreachable();
		return *( T* ) &result;
	}
	template<typename T>
	_LINKAGE void read_io( xstd::any_ptr dst, uint16_t adr, size_t count )
	{
		if constexpr ( sizeof( T ) == 4 )
			asm volatile( "cld; rep insl" : "+D" ( dst.address ), "+d" ( adr ) "+c" ( count ) : "memory", "flags" );
		else if constexpr ( sizeof( T ) == 2 )
			asm volatile( "cld; rep insw" : "+D" ( dst.address ), "+d" ( adr ) "+c" ( count ) : "memory", "flags" );
		else if constexpr ( sizeof( T ) == 1 )
			asm volatile( "cld; rep insb" : "+D" ( dst.address ), "+d" ( adr ) "+c" ( count ) : "memory", "flags" );
		else
			unreachable();
	}
	template<typename T = uint8_t>
	_LINKAGE void write_io( uint16_t adr, T value )
	{
		uint32_t v = 0;
		*( T* ) &v = value;
		register uint32_t _value asm( "eax" ) = v;
		register uint16_t adr_reg asm( "dx" ) = adr;
		if constexpr ( sizeof( T ) == 4 )
			asm volatile( "out %%eax, %%dx" :: "r" ( _value ), "r" ( adr_reg ) );
		else if constexpr ( sizeof( T ) == 2 )
			asm volatile( "out %%ax, %%dx" :: "r" ( _value ), "r" ( adr_reg ) );
		else if constexpr ( sizeof( T ) == 1 )
			asm volatile( "out %%al, %%dx" :: "r" ( _value ), "r" ( adr_reg ) );
		else
			unreachable();
	}
	template<typename T>
	_LINKAGE void write_io( uint16_t adr, xstd::any_ptr dst, size_t count )
	{
		if constexpr ( sizeof( T ) == 4 )
			asm volatile( "cld; rep outsl" : "+S" ( dst.address ), "+d" ( adr ) "+c" ( count ) : "flags" );
		else if constexpr ( sizeof( T ) == 2 )
			asm volatile( "cld; rep outsw" : "+S" ( dst.address ), "+d" ( adr ) "+c" ( count ) : "flags" );
		else if constexpr ( sizeof( T ) == 1 )
			asm volatile( "cld; rep outsb" : "+S" ( dst.address ), "+d" ( adr ) "+c" ( count ) : "flags" );
		else
			unreachable();
	}
	_LINKAGE void usleep( xstd::time::microseconds u )
	{
		for ( long long n = 0; n < u.count(); n++ )
			write_io<uint8_t>( 0x80, 0 );
	}

	// Interrupt mask.
	//
	_LINKAGE void disable() { asm volatile( "cli" ); }
	_LINKAGE void enable() { asm volatile( "sti" ); }

	// Effective IRQL.
	//
	_LINKAGE irql_t get_irql()
	{
		auto irql = ( irql_t ) read_cr8();
		assume( irql <= 0xF );
		return irql;
	}
	_LINKAGE CONST_FN irql_t get_effective_irql( rflags flags = read_flags(), irql_t irql = get_irql() )
	{
		irql |= flags.interrupt_enable_flag ? 0 : NO_INTERRUPTS;
		return irql;
	}
	_LINKAGE void set_irql( irql_t new_irql )
	{
		write_cr8( ( uint64_t ) new_irql );
	}
	_LINKAGE void set_effective_irql( irql_t new_irql )
	{
		set_irql( new_irql & ~NO_INTERRUPTS );
		if ( new_irql & NO_INTERRUPTS )
			disable();
		else
			enable();
	}
	_LINKAGE void set_effective_irql( rflags* flags, irql_t new_irql )
	{
		set_irql( new_irql & ~NO_INTERRUPTS );
		flags->interrupt_enable_flag = !( new_irql & NO_INTERRUPTS );
	}
	_LINKAGE void lower_irql( irql_t new_irql )
	{
		set_irql( new_irql );
	}
	_LINKAGE irql_t raise_irql( irql_t new_irql )
	{
		irql_t irql = get_irql();
		dassert( irql <= new_irql );
		set_irql( new_irql );
		return irql;
	}
	_LINKAGE irql_t max_irql( irql_t new_irql )
	{
		irql_t irql = get_irql();
		if ( irql <= new_irql )
			set_irql( new_irql );
		return irql;
	}

	// XSAVE family.
	//
	_LINKAGE void fxsave( void* buffer )
	{
		asm volatile( "fxsave %0" : "=m" ( buffer ) );
	}
	_LINKAGE void fxsave64( void* buffer )
	{
		asm volatile( "fxsaveq %0" : "=m" ( buffer ) );
	}
	_LINKAGE void xsave( void* buffer, uint64_t components )
	{
		register uint32_t low asm( "eax" ) = uint32_t( components );
		register uint32_t high asm( "edx" ) = uint32_t( components >> 32 );
		asm volatile( "xsave %0" : "=m" ( buffer ) : "r"( low ), "r"( high ) );
	}
	_LINKAGE void xsave64( void* buffer, uint64_t components )
	{
		register uint32_t low asm( "eax" ) = uint32_t( components );
		register uint32_t high asm( "edx" ) = uint32_t( components >> 32 );
		asm volatile( "xsaveq %0" : "=m" ( buffer ) : "r"( low ), "r"( high ) );
	}
	_LINKAGE void xsaves( void* buffer, uint64_t components )
	{
		register uint32_t low asm( "eax" ) = uint32_t( components );
		register uint32_t high asm( "edx" ) = uint32_t( components >> 32 );
		asm volatile( "xsaves %0" : "=m" ( buffer ) : "r"( low ), "r"( high ) );
	}
	_LINKAGE void xsaves64( void* buffer, uint64_t components )
	{
		register uint32_t low asm( "eax" ) = uint32_t( components );
		register uint32_t high asm( "edx" ) = uint32_t( components >> 32 );
		asm volatile( "xsavesq %0" : "=m" ( buffer ) : "r"( low ), "r"( high ) );
	}
	_LINKAGE void xsavec( void* buffer, uint64_t components )
	{
		register uint32_t low asm( "eax" ) = uint32_t( components );
		register uint32_t high asm( "edx" ) = uint32_t( components >> 32 );
		asm volatile( "xsavec %0" : "=m" ( buffer ) : "r"( low ), "r"( high ) );
	}
	_LINKAGE void xsavec64( void* buffer, uint64_t components )
	{
		register uint32_t low asm( "eax" ) = uint32_t( components );
		register uint32_t high asm( "edx" ) = uint32_t( components >> 32 );
		asm volatile( "xsavecq %0" : "=m" ( buffer ) : "r"( low ), "r"( high ) );
	}
	_LINKAGE void xsaveopt( void* buffer, uint64_t components )
	{
		register uint32_t low asm( "eax" ) = uint32_t( components );
		register uint32_t high asm( "edx" ) = uint32_t( components >> 32 );
		asm volatile( "xsaveopt %0" : "=m" ( buffer ) : "r"( low ), "r"( high ) );
	}
	_LINKAGE void xsaveopt64( void* buffer, uint64_t components )
	{
		register uint32_t low asm( "eax" ) = uint32_t( components );
		register uint32_t high asm( "edx" ) = uint32_t( components >> 32 );
		asm volatile( "xsaveoptq %0" : "=m" ( buffer ) : "r"( low ), "r"( high ) );
	}
	_LINKAGE void fxrstor( void* buffer )
	{
		asm volatile( "fxrstor %0" : "=m" ( buffer ) );
	}
	_LINKAGE void fxrstor64( void* buffer )
	{
		asm volatile( "fxrstorq %0" : "=m" ( buffer ) );
	}
	_LINKAGE void xrstor( void* buffer, uint64_t components )
	{
		register uint32_t low asm( "eax" ) = uint32_t( components );
		register uint32_t high asm( "edx" ) = uint32_t( components >> 32 );
		asm volatile( "xrstor %0" ::  "m" ( buffer ), "r"( low ), "r"( high ) );
	}
	_LINKAGE void xrstor64( void* buffer, uint64_t components )
	{
		register uint32_t low asm( "eax" ) = uint32_t( components );
		register uint32_t high asm( "edx" ) = uint32_t( components >> 32 );
		asm volatile( "xrstorq %0" :: "m" ( buffer ), "r"( low ), "r"( high ) );
	}
	_LINKAGE void xrstors( void* buffer, uint64_t components )
	{
		register uint32_t low asm( "eax" ) = uint32_t( components );
		register uint32_t high asm( "edx" ) = uint32_t( components >> 32 );
		asm volatile( "xrstors %0" :: "m" ( buffer ), "r"( low ), "r"( high ) );
	}
	_LINKAGE void xrstors64( void* buffer, uint64_t components )
	{
		register uint32_t low asm( "eax" ) = uint32_t( components );
		register uint32_t high asm( "edx" ) = uint32_t( components >> 32 );
		asm volatile( "xrstorsq %0" :: "m" ( buffer ), "r"( low ), "r"( high ) );
	}

	// String intrinsics.
	//
	enum sidd : uint8_t
	{
		// Data format.
		sidd_ubyte =                0<<0,
		sidd_uword =                1<<0,
		sidd_sbyte =                2<<0,
		sidd_sword =                3<<0,

		// Comparison operator.
		sidd_cmp_eq_any =           0<<2,
		sidd_cmp_range =            1<<2,
		sidd_cmp_eq_each =          2<<2,
		sidd_cmp_eq_ordered =       3<<2,

		// Polarity.
		sidd_pol_pos =              0<<4,
		sidd_pol_neg =              1<<4,
		sidd_pol_mask_pos =         2<<4,
		sidd_pol_mask_neg =         3<<4,

		// Output selection in cmpXstri.
		sidd_least_sig =            0<<6,
		sidd_most_sig =             1<<6,
		
		// Output selection in cmpXstrm.
		sidd_bit_mask =             0<<6,
		sidd_unit_mask =            1<<6,
	};
	typedef char v16qi __attribute__((__vector_size__(16)));
	template<uint8_t mode> _LINKAGE PURE_FN int cmpstr( v16qi a, v16qi b )  { return __builtin_ia32_pcmpistri128( a, b, mode ); }
	template<uint8_t mode> _LINKAGE PURE_FN int cmpstra( v16qi a, v16qi b ) { return __builtin_ia32_pcmpistria128( a, b, mode ); }
	template<uint8_t mode> _LINKAGE PURE_FN int cmpstrc( v16qi a, v16qi b ) { return __builtin_ia32_pcmpistric128( a, b, mode ); }
	template<uint8_t mode> _LINKAGE PURE_FN int cmpstro( v16qi a, v16qi b ) { return __builtin_ia32_pcmpistrio128( a, b, mode ); }
	template<uint8_t mode> _LINKAGE PURE_FN int cmpstrs( v16qi a, v16qi b ) { return __builtin_ia32_pcmpistris128( a, b, mode ); }
	template<uint8_t mode> _LINKAGE PURE_FN int cmpstrz( v16qi a, v16qi b ) { return __builtin_ia32_pcmpistriz128( a, b, mode ); }
	template<uint8_t mode> _LINKAGE PURE_FN v16qi cmpstrm( v16qi a, v16qi b ) { return __builtin_ia32_pcmpistrm128( a, b, mode ); }
	template<uint8_t mode> _LINKAGE PURE_FN int cmpstr( v16qi a, int la, v16qi b, int lb )  { return __builtin_ia32_pcmpestri128( a, la, b, lb, mode ); }
	template<uint8_t mode> _LINKAGE PURE_FN int cmpstra( v16qi a, int la, v16qi b, int lb ) { return __builtin_ia32_pcmpestria128( a, la, b, lb, mode ); }
	template<uint8_t mode> _LINKAGE PURE_FN int cmpstrc( v16qi a, int la, v16qi b, int lb ) { return __builtin_ia32_pcmpestric128( a, la, b, lb, mode ); }
	template<uint8_t mode> _LINKAGE PURE_FN int cmpstro( v16qi a, int la, v16qi b, int lb ) { return __builtin_ia32_pcmpestrio128( a, la, b, lb, mode ); }
	template<uint8_t mode> _LINKAGE PURE_FN int cmpstrs( v16qi a, int la, v16qi b, int lb ) { return __builtin_ia32_pcmpestris128( a, la, b, lb, mode ); }
	template<uint8_t mode> _LINKAGE PURE_FN int cmpstrz( v16qi a, int la, v16qi b, int lb ) { return __builtin_ia32_pcmpestriz128( a, la, b, lb, mode ); }
	template<uint8_t mode> _LINKAGE PURE_FN v16qi cmpstrm( v16qi a, int la, v16qi b, int lb ) { return __builtin_ia32_pcmpestrm128( a, la, b, lb, mode ); }

	// Misc.
	//
	_LINKAGE void nop()
	{
		asm volatile( "nop" );
	}
	_LINKAGE void pause()
	{
		asm volatile( "pause" ::: "memory" );
	}
	_LINKAGE void halt()
	{
		asm volatile( "hlt" ::: "memory" );
	}
	_LINKAGE void monitor( xstd::any_ptr adr, uint32_t extensions, uint32_t hints )
	{
		__builtin_ia32_monitor( ( void* ) adr, extensions, hints );
	}
	_LINKAGE void mwait( uint32_t extensions, uint32_t hints )
	{
		__builtin_ia32_mwait( extensions, hints );
	}
	_LINKAGE void finit()
	{
		asm volatile( "finit" );
	}
	_LINKAGE void spin()
	{
		if constexpr ( is_kernel_mode() )
		{
			__asm
			{
				pushfq
				cli
				x:
					xor eax, eax
					jz x
				popfq
			}
		}
		else
		{
			__asm
			{
				x:
					xor eax, eax
					jz x
			}
		}
	}
	_LINKAGE void icebp()
	{
		asm volatile ( ".byte 0xF1" );
	}
	_LINKAGE void int3()
	{
		asm volatile ( "int3" );
	}
	_LINKAGE void intn( size_t idx )
	{
		asm volatile ( "int %0" :: "i" ( idx ) );
	}

	// Hardware accelerated crypto.
	//
	template<xstd::Integral T>
	_LINKAGE PURE_FN uint32_t crc32ci( const T& value, uint32_t crc = ~0 )
	{
		if ( __is_local_memory( &value ) )
		{
			if constexpr ( sizeof( T ) == 8 )
				asm( "crc32q %1, %q0" : "+r" ( crc ) : "r" ( value ) );
			else if constexpr ( sizeof( T ) == 4 )
				asm( "crc32l %1, %0"  : "+r" ( crc ) : "r" ( value ) );
			else if constexpr ( sizeof( T ) == 2 )
				asm( "crc32w %1, %0"  : "+r" ( crc ) : "r" ( value ) );
			else
				asm( "crc32b %1, %0"  : "+r" ( crc ) : "r" ( value ) );
		}
		else
		{
			if constexpr ( sizeof( T ) == 8 )
				asm( "crc32q %1, %q0" : "+r" ( crc ) : "m" ( value ) );
			else if constexpr ( sizeof( T ) == 4 )
				asm( "crc32l %1, %0"  : "+r" ( crc ) : "m" ( value ) );
			else if constexpr ( sizeof( T ) == 2 )
				asm( "crc32w %1, %0"  : "+r" ( crc ) : "m" ( value ) );
			else
				asm( "crc32b %1, %0"  : "+r" ( crc ) : "m" ( value ) );
		}
		return crc;
	}
	_LINKAGE PURE_FN uint32_t crc32ci( const volatile void* _ptr, size_t length, uint32_t crc = ~0 )
	{
		xstd::any_ptr ptr = _ptr;

		// If constant length and smaller than 32 bytes, unroll all qword units via register reference.
		//
		if ( __is_consteval( length ) && length <= 32 )
		{
			while ( length >= 8 )
				crc = crc32ci( *( uint64_t* ) ptr, crc ), ptr += 8, length -= 8;
		}

		// CRC in 64-byte units using qword CRCs until we're done.
		//
		length = xstd::unroll_scaled_n<8, 64>( [ & ]
		{
			asm( "crc32q %1, %q0" : "+r" ( crc ) : "m" ( *( uint64_t* ) ptr ) );
			ptr += 8;
		}, length );

		// Handle the leftovers.
		//
		if ( length & 4 ) crc = crc32ci( *( uint32_t* ) ptr, crc ), ptr += 4;
		if ( length & 2 ) crc = crc32ci( *( uint16_t* ) ptr, crc ), ptr += 2;
		if ( length & 1 ) crc = crc32ci( *( uint8_t* ) ptr, crc ), ptr += 1;
		return crc;
	}
	template<xstd::Integral T>
	_LINKAGE PURE_FN uint32_t crc32c( const T& value, uint32_t crc = 0 )
	{
		return ~crc32ci<T>( value, ~crc );
	}
	_LINKAGE PURE_FN uint32_t crc32c( const volatile void* ptr, size_t length, uint32_t crc = 0 )
	{
		return ~crc32ci( ptr, length, ~crc );
	}

	// Non-temporal memory helpers.
	//
	template<typename Vec>
	_LINKAGE PURE_FN Vec load_non_temporal( const Vec* p )
	{
		Vec r;
#if __AVX__
		asm volatile( "vmovntdqa %1, %0" : "=x" ( r ) : "m" ( *p ) );
#else
		asm volatile( "movntdqa %1, %0" : "=x" ( r ) : "m" ( *p ) );
#endif
		return r;
	}
	template<typename Vec>
	_LINKAGE void store_non_temporal( Vec* p, Vec r )
	{
#if __AVX__
		asm volatile( "vmovntdq %1, %0" : "=m" ( *p ) : "x" ( r ) );
#else
		asm volatile( "movntdq %1, %0" : "=m" ( *p ) : "x" ( r ) );
#endif
	}

	// Unaligned memory helpers.
	//
	template<typename Vec>
	_LINKAGE Vec load_unaligned( const Vec* p )
	{
		struct wrapper
		{
			Vec value;
		} __attribute__((packed, may_alias));
		return ( ( wrapper* ) p )->value;
	}
	template<typename Vec> 
	_LINKAGE Vec load_unaligned( const void* p ) { return load_unaligned<Vec>( ( const Vec* ) p ); }
	
	template<typename Vec>
	_LINKAGE void store_unaligned( const Vec* p, Vec r )
	{
		struct wrapper
		{
			Vec value;
		} __attribute__((packed, may_alias));
		( ( wrapper* ) p )->value = r;
	}
	template<typename Vec>
	_LINKAGE void store_unaligned( const void* p, Vec r ) { return store_unaligned<Vec>( ( const Vec* ) p, r ); }
	
	// RAII wrapper for IRQL.
	//
	template<irql_t new_irql>
	struct scope_irql
	{
		irql_t prev;
		scope_irql( irql_t prev = ( uint8_t ) get_irql() ) : prev( prev )
		{
			if ( prev < new_irql )
				set_irql( new_irql );
		}
		scope_irql( scope_irql&& ) noexcept = delete;
		scope_irql( const scope_irql& ) = delete;

		void reset( bool state = false )
		{
			if ( state ) set_irql( new_irql );
			else set_irql( prev );
		}

		~scope_irql()
		{
			reset();
		}
	};

	template<>
	struct scope_irql<NO_INTERRUPTS>
	{
		rflags prev_flags;
		scope_irql( rflags prev_flags = read_flags() ) : prev_flags( prev_flags )
		{
			disable();
		}
		scope_irql( scope_irql&& ) noexcept = delete;
		scope_irql( const scope_irql& ) = delete;

		void reset( bool state = false )
		{
			if ( prev_flags.interrupt_enable_flag )
			{
				if ( state ) disable();
				else enable();
			}
		}

		~scope_irql()
		{
			reset();
		}
	};

	// Checks if the CPU vendor is Intel.
	//
	_LINKAGE bool is_intel() { return static_cpuid<0, 0, cpuid_eax_00>::result.ecx_value_ntel == bswap( 'ntel' ); };

	// TSC/MSR/PMC based profiling in the style of xstd::profile.
	//
	namespace impl
	{
		_LINKAGE uint32_t read_tsc_low()
		{
			register uint32_t low asm( "eax" );
			register uint32_t high asm( "edx" );
			asm volatile( "rdtsc" : "=r" ( low ), "=r" ( high ) :: );
			return low;
		}
		_LINKAGE uint32_t read_tscp_low()
		{
			register uint32_t low asm( "eax" );
			register uint32_t high asm( "edx" );
			register uint32_t pid asm( "ecx" );
			asm volatile( "rdtscp" : "=r" ( low ), "=r" ( pid ), "=r" ( high ) :: );
			return low;
		}
	};
	template<typename T, typename... Tx> requires xstd::InvocableWith<T, Tx...>
	_LINKAGE FLATTEN auto uprofile_tsc( T&& f, Tx&&... args )
	{
		using R = decltype( std::declval<T>()( std::forward<Tx>( args )... ) );

		if constexpr ( xstd::Void<R> )
		{
			uint32_t t0 = impl::read_tscp_low();
			f( std::forward<Tx>( args )... );
			uint32_t t1 = impl::read_tscp_low();
			serialize();
			return t1 - t0;
		}
		else
		{
			uint32_t t0 = impl::read_tscp_low();
			std::pair<R, uint64_t> result = { f( std::forward<Tx>( args )... ), 0ull };
			uint32_t t1 = impl::read_tscp_low();
			serialize();
			result.second = t1 - t0;
			return result;
		}
	}
	template<typename T, typename... Tx> requires xstd::InvocableWith<T, Tx...>
	_LINKAGE FLATTEN auto uprofile_msr( uint64_t id, T&& f, Tx&&... args )
	{
		using result_t = decltype( std::declval<T>()( std::forward<Tx>( args )... ) );

		if constexpr ( std::is_same_v<result_t, void> )
		{
			serialize();
			uint64_t t0 = read_msr( id );
			f( std::forward<Tx>( args )... );
			serialize();
			uint64_t t1 = read_msr( id );
			serialize();
			return t1 - t0;
		}
		else
		{
			serialize();
			uint64_t t0 = read_msr( id );
			std::pair<result_t, uint64_t> result = { f( std::forward<Tx>( args )... ), 0ull };
			serialize();
			uint64_t t1 = read_msr( id );
			serialize();
			result.second = t1 - t0;
			return result;
		}
	}
	template<typename T, typename... Tx> requires xstd::InvocableWith<T, Tx...>
	_LINKAGE FLATTEN auto uprofile_pmc( uint64_t id, T&& f, Tx&&... args )
	{
		using result_t = decltype( std::declval<T>()( std::forward<Tx>( args )... ) );

		if constexpr ( std::is_same_v<result_t, void> )
		{
			serialize();
			uint64_t t0 = read_pmc( id );
			f( std::forward<Tx>( args )... );
			serialize();
			uint64_t t1 = read_pmc( id );
			serialize();
			return t1 - t0;
		}
		else
		{
			serialize();
			uint64_t t0 = read_pmc( id );
			std::pair<result_t, uint64_t> result = { f( std::forward<Tx>( args )... ), 0ull };
			serialize();
			uint64_t t1 = read_pmc( id );
			serialize();
			result.second = t1 - t0;
			return result;
		}
	}
};
#undef _LINKAGE
#endif
