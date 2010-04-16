// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// e32\include\kernel\arm\bootdefs.h
// Miscellaneous definitions for bootstrap
// Done as a C++ header file for translation
// Order of entries in the boot table
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @publishedPartner
 @released
*/
enum TBootTableEntry
	{
	BTF_WriteC,					// output char to debug port
	BTF_RamBanks,				// get list of RAM banks
	BTF_SetupRamBank,			// set up controller for RAM bank
	BTF_RomBanks,				// get list of ROM banks
	BTF_SetupRomBank,			// set up controller for ROM bank
	BTF_HwBanks,				// get list of primary HW addresses
	BTF_Reserve,				// allocate any physical RAM before final addresses determined
	BTF_Params,					// get miscellaneous parameters
	BTF_Final,					// do any final initialisation prior to booting the kernel
	BTF_Alloc,					// allocate RAM
	BTF_GetPdePerm,				// get PDE permissions for a mapping
	BTF_GetPtePerm,				// get PTE permissions for a mapping
	BTF_PTUpdate,				// called when page table updated
	BTF_EnableMMU,				// enable MMU


	BTP_Rom,					// permissions for ROM
	BTP_Kernel,					// permissions for kernel data/stack/heap
	BTP_SuperCPU,				// permissions for Super page/CPU page
	BTP_PageTable,				// permissions for page tables
	BTP_Vector,					// permissions for exception vectors
	BTP_Hw,						// permissions for hardware mappings
	BTP_MiniCache,				// permissions for minicache flush area
	BTP_MainCache,				// permissions for main cache flush area
	BTP_PtInfo,					// permissions for page table info
	BTP_User,					// permissions for user RAM (direct model only)
	BTP_Temp,					// permissions for temporary identity mapping
	BTP_Uncached,				// permissions for uncached address
	};

// CPU page
const TInt CpuPageOffset = 0x1000;			/**< @internalComponent */	// offset from super page to CPU page
const TInt CpuAllocDataOffset = 0x1780;		/**< @internalComponent */	// area used for SAllocData
const TInt CpuRomTableOffset = 0x1800;		/**< @internalComponent */	// area used for ROM block descriptors
const TInt CpuRomTableTop = 0x1900;			/**< @internalComponent */	// offset to byte after the area used for ROM block descriptors
const TInt CpuRamTableOffset = 0x1900;		/**< @internalComponent */	// area used for RAM block descriptors
const TInt CpuRamTableTop = 0x1C00;			/**< @internalComponent */	// offset to byte after the area used for RAM block descriptors
const TInt CpuSmrTableOffset = 0x1C00;		/**< @internalComponent */	// area used for SMR block descriptors (SSmrBank, 7+1 null max)
const TInt CpuSmrTableTop = 0x1C80;			/**< @internalComponent */	// offset to byte after last byte in the SMR block descriptor area
const TInt CpuBootStackOffset = 0x1C80;		/**< @internalComponent */	// top 1K-128 bytes of CPU page are used for a stack
const TInt CpuBootStackTop = 0x2000;		/**< @internalComponent */	//

/** Allocation types

@publishedPartner
@released
*/
enum TBootMemAlloc
	{
	BMA_Init,					// initialise allocator
	BMA_SuperCPU,				// return final super page/CPU page address
	BMA_PageDirectory,			// return page directory address
	BMA_PageTable,				// allocate a page table
	BMA_Kernel,					// allocate general kernel RAM
	BMA_Reloc,					// relocate following super page relocation
	BMA_NUM_FUNCTIONS
	};

/** Parameters obtained from BTF_Params call

@publishedPartner
@released
*/
enum TBootParam
	{
	BPR_InitialMMUCRClear = 0,	// bits to clear in MMUCR initially (default 0xffffffff)
	BPR_InitialMMUCRSet,		// bits to set in MMUCR initially (default CPU dependent)
	BPR_FinalMMUCRClear,		// bits to clear in MMUCR when MMU enabled (default 0)
	BPR_FinalMMUCRSet,			// bits to set in MMUCR when MMU enabled (default CPU dependent)
	BPR_AuxCRClear,				// MMU auxiliary control register value clear (if it exists)
	BPR_AuxCRSet,				// MMU auxiliary control register value set (if it exists)
	BPR_CAR,					// Coprocessor access register value (if it exists)
/**
Defines Memory Types for Tex[0]:C:B = 101, 110 & 111 mapping on platforms with Memory Type Remapping (ARMv6K & ARMv7 & later).
They correspond to EMemAttPlatformSpecific5, EMemAttPlatformSpecific6 & EMemAttPlatformSpecific7 memory types.
They are not used in Kernel internally but could be used by device drivers.
Use TPlatformSpecificMappings enum to build up the most common values.
The value is built up as follows:
	bits 	Meaning
	---------------
	00:01	Memory type for TEX[0]:C:B = 101 (EMemAttPlatformSpecific5).
	02:03	Memory type for TEX[0]:C:B = 110 (EMemAttPlatformSpecific6).
	04:05	Memory type for TEX[0]:C:B = 111 (EMemAttPlatformSpecific7).
	06:09	Any
	10:11	Inner Cache Attributes for EMemAttPlatformSpecific5 - if Normal memory type.
	12:13	Inner Cache Attributes for EMemAttPlatformSpecific6 - if Normal memory type.
	14:15	Inner Cache Attributes for EMemAttPlatformSpecific7 - if Normal memory type.
	16:25	Any
	26:27	Outer Cache Attributes for EMemAttPlatformSpecific5 - if Normal memory type.
	28:29	Outer Cache Attributes for EMemAttPlatformSpecific6 - if Normal memory type.
	30:31	Outer Cache Attributes for EMemAttPlatformSpecific7 - if Normal memory type.

Memory Type coding is as folows: 00:strongly ordered,    01:device, 10:normal, 11 reserved
Cache Attr. coding is as follows: 00:non-cached,buffered, 01:WBWA,   10:WTRA,   11:WBRA
Note: Memory mapping for TEX[0]:C:B=110 is IMPLEMENTATION DEFINED. Check ARM core's document for details.
@see TPlatformSpecificMappings
*/
	BPR_Platform_Specific_Mappings,
	BPR_TTBRExtraBits,			// bits 0-6 of TTBR0 and TTBR1

	// direct model only stuff starts at 0x80
	BPR_UncachedLin = 0x80,		// linear address for dummy uncached mapping (direct model only)
	BPR_PageTableSpace,			// space to reserve for page tables (direct model only)
	BPR_KernDataOffset,			// offset from super page to kernel data (direct model only)
	BPR_BootLdrImgAddr,			// physical address of boot image (bootloader only)
	BPR_BootLdrExtraRAM,		// extra RAM to allow for device driver mappings (eg video) (bootloader only)
	BPR_APBootLin,				// AP boot page
	};

/**
The most common values for BPR_Platform_Specific_Mappings boot parameter. Any *_Mapping5 value can be orred with any
*_Mapping6 and/or *_Mapping7. For example:
@code
ParameterTable
		DCD 	BPR_Platform_Specific_Mappings, E_WT_WT_Mapping5+E_FC_NC_Mapping6+E_NC_FC_Mapping7
@codeend
specifies:
 - EMemAttPlatformSpecific5 to be write-through mapping in both inner and outer cache.
 - EMemAttPlatformSpecific6 to be fully cached (write-back) in inner cache and not cached in outer cache.
 - EMemAttPlatformSpecific7 to be not cached in inner cache and fully cached (write-back) in outer cache.

@see BPR_Platform_Specific_Mappings
@publishedPartner
@released
*/
enum TPlatformSpecificMappings
	{
	E_WT_WT_Mapping5 = 0x08000802, /**Write Through caching on both levels for mapping #5.*/
    E_WT_NC_Mapping5 = 0x00000802, /**Write Through caching at level 1, not cached at level 2 for mapping #5.*/
    E_NC_WT_Mapping5 = 0x08000002, /**Not cached at level 1, Write Through cached at level for mapping #5.*/
	E_FC_NC_Mapping5 = 0x00000402, /**Maximum caching at level 1, not cached at level 2 for mapping #5.*/
	E_NC_FC_Mapping5 = 0x04000002, /**Not cached at level 1, maximum caching at level 2 for mapping #5.*/

	E_WT_WT_Mapping6 = 0x20002008, /**Write Through caching on both levels for mapping #6.*/
    E_WT_NC_Mapping6 = 0x00002008, /**Write Through caching at level 1, not cached at level 2 for mapping #6.*/
    E_NC_WT_Mapping6 = 0x20000008, /**Not cached at level 1, Write Through cached at level for mapping #6.*/
	E_FC_NC_Mapping6 = 0x00001008, /**Maximum caching at level 1, not cached at level 2 for mapping #6.*/
	E_NC_FC_Mapping6 = 0x10000008, /**Not cached at level 1, maximum caching at level 2 for mapping #6.*/

	E_WT_WT_Mapping7 = 0x80008020, /**Write Through caching on both levels for mapping #7.*/
    E_WT_NC_Mapping7 = 0x00008020, /**Write Through caching at level 1, not cached at level 2 for mapping #7.*/
    E_NC_WT_Mapping7 = 0x80000020, /**Not cached at level 1, Write Through cached at level for mapping #7.*/
	E_FC_NC_Mapping7 = 0x00004020, /**Maximum caching at level 1, not cached at level 2 for mapping #7.*/
	E_NC_FC_Mapping7 = 0x40000020, /**Not cached at level 1, maximum caching at level 2 for mapping #7.*/
	};

/** Miscellaneous constants

@publishedPartner
@released
*/
const TInt KMinRamBankSize = 0x10000;		// minimum allowable region of contiguous main memory

/**
@internalComponent
*/
const TInt KMaxAllocSizes = 4;	// 4K, 16K, 64K, 1M

struct SRamBank;

/** Allocator data

@internalComponent
*/
struct SAllocData
	{
	struct SRange
		{
		TAny*	iBase;
		TAny*	iSize;
		};
	struct SPos
		{
		TAny*	iNext;				// starting point for next allocation
		const SRamBank* iBank;		// RAM bank in which iNext resides
		TUint32 iMask;				// allocation size - 1 (size is always a power of 2)
		TInt	iFinal;				// is this the largest allocation size?
		};
	SPos iPos[KMaxAllocSizes];		// data for each allocation size, in increasing size order
	const SRange* iSkip;			// list of reserved physical address ranges, terminated by zero size
	SRange iExtraSkip;				// for super page/CPU page
	TAny* iNextPageTable;			// address of next page table
	};

/** Allocator data for direct memory model

@internalComponent
*/
struct SDirectAlloc
	{
	TAny* iNextPageTable;			// address of next page table
	TAny* iLimit;
	};
