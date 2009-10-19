// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\kernel\x86\bootdefs.h
// Miscellaneous definitions for bootstrap
// Done as a C++ header file for translation
// Order of entries in the boot table
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @internalTechnology
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
	BTP_PtInfo,					// permissions for page table info
	BTP_User,					// permissions for user RAM (direct model only)
	BTP_Temp,					// permissions for temporary identity mapping
	BTP_Uncached,				// permissions for uncached address
	};

// CPU page
const TInt CpuPageOffset = 0x1000;			/**< @internalComponent */	// offset from super page to CPU page
const TInt CpuAllocDataOffset = 0x1900;		/**< @internalComponent */	// area used for SAllocData
const TInt CpuRomTableOffset = 0x1980;		/**< @internalComponent */	// area used for ROM block descriptors
const TInt CpuRamTableOffset = 0x1A00;		/**< @internalComponent */	// area used for RAM block descriptors
const TInt CpuBootStackOffset = 0x1C00;		/**< @internalComponent */	// top 1K bytes of CPU page are used for a stack
const TInt CpuBootStackTop = 0x2000;		/**< @internalComponent */	//

/** Allocation types

@internalTechnology
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

@internalTechnology
*/
enum TBootParam
	{
	BPR_InitialMMUCRClear = 0,	// bits to clear in MMUCR initially (default 0xffffffff)
	BPR_InitialMMUCRSet,		// bits to set in MMUCR initially (default CPU dependent)
	BPR_FinalMMUCRClear,		// bits to clear in MMUCR when MMU enabled (default 0)
	BPR_FinalMMUCRSet,			// bits to set in MMUCR when MMU enabled (default CPU dependent)

	// direct model only stuff starts at 0x80
	BPR_UncachedLin = 0x80,		// linear address for dummy uncached mapping (direct model only)
	BPR_PageTableSpace,			// space to reserve for page tables (direct model only)
	BPR_KernDataOffset,			// offset from super page to kernel data (direct model only)
	BPR_BootLdrImgAddr,			// physical address of boot image (bootloader only)
	BPR_BootLdrExtraRAM,		// extra RAM to allow for device driver mappings (eg video) (bootloader only)
	};

/** Miscellaneous constants

@internalTechnology
*/
const TInt KMinRamBankSize = 0x10000;		// minimum allowable region of contiguous main memory

/**
@internalComponent
*/
const TInt KMaxAllocSizes = 2;	// 4K, 4M

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
	};

/** Allocator data for direct memory model

@internalComponent
*/
struct SDirectAlloc
	{
	TAny* iNextPageTable;			// address of next page table
	TAny* iLimit;
	};


/** Data block used when switching on MMU

@internalComponent
*/
struct SSwitch
	{
	TAny*	iRomHdrPhys;
	TAny*	iRomHdrLin;
	TAny*	iSuperPagePhys;
	TAny*	iSuperPageLin;
	TAny*	iStackPhys;
	TAny*	iStackLin;
	TAny*	iImgHdrPhys;
	TAny*	iImgHdrLin;
	TAny*	iPageDirPhys;
	TAny*	iPageDirLin;
	TAny*	iTempPtPhys;
	TUint32	iOrigPde;
	TInt	iTempPdeOffset;	// -ve if no temp PDE used
	TAny*	iTempPtePtr;	// address of temp PTE corresponding to ROM header
	TUint32	iTempPte;		// PTE which would map ROM header
	};



