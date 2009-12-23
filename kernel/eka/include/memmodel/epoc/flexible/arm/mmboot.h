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
//

/**
 @file
 @publishedPartner
 @prototype
*/

#ifndef __MMBOOT_H__
#define __MMBOOT_H__

#include <nk_cpu.h>

// Linear address map
// 00000000-003FFFFF	Unmapped
// 00400000-7EFFFFFF	Local data
// 7F000000-7FFFFFFF	IPC Alias region
// 80000000-			ROM
//         -8FFFFFFF	Global user area
// 90000000-EFFFFFFF	Kernel memory
// F0000000-FFF00000	Fixed kernel mappings
// FFF00000-FFFFFFFF	Exception vectors
//

// Linear addresses

const TLinAddr KUserLocalDataBase		=0x00400000u;

const TLinAddr KUserLocalDataEnd		=0x7f000000u;

const TLinAddr KIPCAlias				=0x7f000000u;
const TLinAddr KIPCAliasAreaSize		=0x01000000u;

/** Everything above here is global (visible to all processes).*/
const TLinAddr KGlobalMemoryBase		=0x80000000u;

const TLinAddr KRomLinearBase			=0x80000000u;

/** Everything below here has user access permissions, everything above is supervisor only. */
const TLinAddr KUserMemoryLimit			=0x90000000u;

const TLinAddr KKernelSectionBase		=0x90000000u;

const TLinAddr KPrimaryIOBase			=0xC6000000u; // XXX This magic constant is hard coded into baseports!
const TLinAddr KPrimaryIOEnd			=0xC8000000u;

const TLinAddr KKernelDataBase			=0xC8000000u;

const TLinAddr KKernelSectionEnd		=0xF0000000u;

const TLinAddr KSuperPageLinAddr		=0xF0000000u;
const TLinAddr KMachineConfigLinAddr	=0xF0000800u;
#ifdef __SMP__
const TLinAddr KAPBootPageDirLin		=0xF0004000u;
const TLinAddr KAPBootPageTableLin		=0xF0008000u;
const TLinAddr KAPBootPageLin			=0xF0009000u;
#endif
const TLinAddr KDummyUncachedAddr		=0xF000F000u;
const TLinAddr KPageInfoMap				=0xF0010000u;
const TLinAddr KExcptStacksLinearBase	=0xF0040000u;
const TLinAddr KExcptStacksLinearEnd	=0xF0080000u;
const TLinAddr KTempAddr				=0xF0080000u;
const TLinAddr KTempAddrEnd				=0xF0100000u;

const TLinAddr KPageTableInfoBase		=0xF0C00000u;
const TLinAddr KPageTableInfoEnd		=0xF1000000u;

const TLinAddr KPageArraySegmentBase	=0xF1000000u;
const TLinAddr KPageArraySegmentEnd		=0xF2000000u;

const TLinAddr KPageInfoLinearBase		=0xF2000000u;
const TLinAddr KPageInfoLinearEnd		=0xF4000000u;

const TLinAddr KPageDirectoryBase		=0xF4000000u;
const TLinAddr KPageDirectoryEnd		=0xF8000000u;

const TLinAddr KPageTableBase			=0xF8000000u;
const TLinAddr KPageTableEnd			=0xFFF00000u;


// Domain usage
//
// 0 All, except...
// 2 IPC Alias chunk
// 15 User memory when __USER_MEMORY_GUARDS_ENABLED__ defined
const TInt KIPCAliasDomain = 2;
const TInt KNumArmDomains = 16;								/**< @internalTechnology */

// default domain access is client of domain 0, no access to rest
const TUint32 KDefaultDomainAccess			 = 0x00000001u;	/**< @internalTechnology */
const TUint32 KSupervisorInitialDomainAccess = 0x00000001u;	/**< @internalTechnology */

#define	PDE_IN_DOMAIN(aPde, aDomain)	(((aPde) & ~(15 << 5)) | ((aDomain) << 5))

// Constants for ARM V6 MMU
const TInt KPageShift=12;
const TInt KPageSize=1<<KPageShift;
const TInt KPageMask=KPageSize-1;
const TInt KChunkShift=20;
const TInt KChunkSize=1<<KChunkShift;
const TInt KChunkMask=KChunkSize-1;
const TInt KPageTableShift=KChunkShift-KPageShift+2;	// PTE is 4 bytes
const TInt KPageTableSize=1<<KPageTableShift;
const TInt KPageTableMask=KPageTableSize-1;
const TInt KPtClusterShift=KPageShift-KPageTableShift;
const TInt KPtClusterSize=1<<KPtClusterShift;
const TInt KPtClusterMask=KPtClusterSize-1;
const TInt KPtBlockShift=KPageShift-4;					/**< @internalTechnology */	// sizeof(SPageTableInfo)=16
const TInt KPtBlockSize=1<<KPtBlockShift;				/**< @internalTechnology */
const TInt KPtBlockMask=KPtBlockSize-1;					/**< @internalTechnology */
const TInt KPagesInPDEShift=KChunkShift-KPageShift;
const TInt KPagesInPDE=1<<KPagesInPDEShift;
const TInt KPagesInPDEMask=KPagesInPDE-1;
const TInt KLargePageShift=16;
const TInt KLargePageSize=1<<KLargePageShift;
const TInt KLargePageMask=KLargePageSize-1;

const TInt KPageDirectoryShift=32-KChunkShift+2;		// PDE is 4 bytes
const TInt KPageDirectorySize=1<<KPageDirectoryShift;
const TInt KPageDirectoryMask=KPageDirectorySize-1;

const TInt KMmuAsidCount=256;
const TInt KMmuAsidMask=KMmuAsidCount-1;

const TUint KNumOsAsids=KMmuAsidCount;
const TUint KKernelOsAsid=0;

// Permissions - 3 bit field, APX most significant. When __CPU_MEMORY_TYPE_REMAPPING defined, LSB must be 1
#if defined(__CPU_MEMORY_TYPE_REMAPPING)
const TInt KArmV6PermRORO=7;		/**< @internalTechnology */ // sup RO user RO
#else
const TInt KArmV6PermRORO=6;		/**< @internalTechnology */ // sup RO user RO
#endif
const TInt KArmV6PermRWNO=1;		/**< @internalTechnology */ // sup RW user no access
const TInt KArmV6PermRWRW=3;		/**< @internalTechnology */ // sup RW user RW
const TInt KArmV6PermRONO=5;		/**< @internalTechnology */ // sup RO user no access

#if defined(__CPU_MEMORY_TYPE_REMAPPING)
// ARM1176, ARM11MPCORE, ARMv7
// TMemoryType is used to describe cache attributes
// 3 bits are reserved in page table: TEX[0]:C:B
#else
// Attributes - 5 bit field, TEX in 2-4, CB in 1,0
const TInt KArmV6MemAttSO			=0x00;	/**< @internalTechnology */ // strongly ordered
const TInt KArmV6MemAttSD			=0x01;	/**< @internalTechnology */ // shared device
const TInt KArmV6MemAttNSD			=0x08;	/**< @internalTechnology */ // non-shared device
const TInt KArmV6MemAttNCNC			=0x04;	/**< @internalTechnology */ // normal, outer uncached, inner uncached
const TInt KArmV6MemAttWTRAWTRA		=0x02;	/**< @internalTechnology */ // normal, outer WTRA cached, inner WTRA cached
const TInt KArmV6MemAttWBRAWBRA		=0x03;	/**< @internalTechnology */	// normal, outer WBRA cached, inner WBRA cached
const TInt KArmV6MemAttWBWAWBWA		=0x15;	/**< @internalTechnology */	// normal, outer WBWA cached, inner WBWA cached
const TInt KArmV6MemAttNCWTRA		=0x12;	/**< @internalTechnology */	// normal, outer uncached, inner WTRA cached
const TInt KArmV6MemAttNCWBRA		=0x13;	/**< @internalTechnology */	// normal, outer uncached, inner WBRA cached
const TInt KArmV6MemAttNCWBWA		=0x11;	/**< @internalTechnology */	// normal, outer uncached, inner WBWA cached
const TInt KArmV6MemAttWTRANC		=0x18;	/**< @internalTechnology */	// normal, outer WTRA cached, inner uncached
const TInt KArmV6MemAttWTRAWBRA		=0x1B;	/**< @internalTechnology */	// normal, outer WTRA cached, inner WBRA cached
const TInt KArmV6MemAttWTRAWBWA		=0x19;	/**< @internalTechnology */	// normal, outer WTRA cached, inner WBWA cached
const TInt KArmV6MemAttWBRANC		=0x1C;	/**< @internalTechnology */	// normal, outer WBRA cached, inner uncached
const TInt KArmV6MemAttWBRAWTRA		=0x1E;	/**< @internalTechnology */	// normal, outer WBRA cached, inner WTRA cached
const TInt KArmV6MemAttWBRAWBWA		=0x1D;	/**< @internalTechnology */	// normal, outer WBRA cached, inner WBWA cached
const TInt KArmV6MemAttWBWANC		=0x14;	/**< @internalTechnology */	// normal, outer WBWA cached, inner uncached
const TInt KArmV6MemAttWBWAWTRA		=0x16;	/**< @internalTechnology */	// normal, outer WBWA cached, inner WTRA cached
const TInt KArmV6MemAttWBWAWBRA		=0x17;	/**< @internalTechnology */	// normal, outer WBWA cached, inner WBRA cached
#endif

const TUint32 KArmV6PdePageTable	=0x00000001;/**< @internalTechnology */	// L1 descriptor is page table
const TUint32 KArmV6PdeSection		=0x00000002;/**< @internalTechnology */	// L1 descriptor is section
const TUint32 KArmV6PdeTypeMask		=0x00000003;/**< @internalTechnology */
const TUint32 KArmV6PdeECCEnable	=0x00000200;/**< @internalTechnology */	// ECC enable (all L1 descriptors)
const TUint32 KArmV6PdeSectionXN	=0x00000010;/**< @internalTechnology */	// Section not executable
const TUint32 KArmV6PdeSectionS		=0x00010000;/**< @internalTechnology */	// Section shared
const TUint32 KArmV6PdeSectionNG	=0x00020000;/**< @internalTechnology */	// Section not global
const TUint32 KArmV6PdePermMask		=0x00008c00;/**< @internalTechnology */	// Section permission bits
const TUint32 KArmV6PdeAttMask		=0x0000700c;/**< @internalTechnology */	// Section memory attribute bits
const TUint32 KArmV6PteLargePage	=0x00000001;/**< @internalTechnology */	// L2 descriptor is large page
const TUint32 KArmV6PteSmallPage	=0x00000002;/**< @internalTechnology */	// L2 descriptor is small page
const TUint32 KArmV6PteTypeMask		=0x00000003;/**< @internalTechnology */
const TUint32 KArmV6PteLargeXN		=0x00008000;/**< @internalTechnology */	// Large page not executable
const TUint32 KArmV6PteSmallXN		=0x00000001;/**< @internalTechnology */	// Small page not executable
const TUint32 KArmV6PteS			=0x00000400;/**< @internalTechnology */	// Large or small page shared
const TUint32 KArmV6PteNG			=0x00000800;/**< @internalTechnology */	// Large or small page not global
const TUint32 KArmV6PtePermMask		=0x00000230;/**< @internalTechnology */	// Large or small page permission bits
const TUint32 KArmV6PteLargeAttMask	=0x0000700c;/**< @internalTechnology */	// Large page memory attribute bits
const TUint32 KArmV6PteSmallAttMask	=0x000001cc;/**< @internalTechnology */	// Small page memory attribute bits
// Remapped Access Permission coding:
const TUint32 KArmV6PteAP2			=0x00000200;/**< @internalTechnology */	// RO / !RW
const TUint32 KArmV6PteSmallTEX2	=0x00000100;/**< @internalTechnology */
const TUint32 KArmV6PteSmallTEX1	=0x00000080;/**< @internalTechnology */
const TUint32 KArmV6PteSmallTEX0	=0x00000040;/**< @internalTechnology */
const TUint32 KArmV6PteAP1			=0x00000020;/**< @internalTechnology */	// AllAccess / !KernelOnly
const TUint32 KArmV6PteAP0			=0x00000010;/**< @internalTechnology */	// Must be set

const TUint32  KPdePresentMask=KArmV6PdeTypeMask;				/**< @internalTechnology */
const TUint32  KPdeTypeMask=KArmV6PdeTypeMask;					/**< @internalTechnology */
const TUint32  KPdeSectionAddrMask=0xfff00000;					/**< @internalTechnology */
const TUint32  KPdePageTableAddrMask=0xfffffc00;				/**< @internalTechnology */
const TUint32  KPteLargePageAddrMask=0xffff0000;				/**< @internalTechnology */
const TUint32  KPteSmallPageAddrMask=0xfffff000;				/**< @internalTechnology */
const TInt KLargeSmallPageRatio=KLargePageSize/KPageSize;	/**< @internalTechnology */
const TUint32  KPdeUnallocatedEntry=0;							/**< @internalTechnology */
const TUint32  KPteUnallocatedEntry=0;							/**< @internalTechnology */
const TUint32  KPdeMatchMask=0;									/**< @internalTechnology */
const TUint32  KPteMatchMask=0;									/**< @internalTechnology */
const TUint32  KPtePresentMask=KArmV6PteTypeMask;				/**< @internalTechnology */
const TUint32  KPteTypeMask=KArmV6PteTypeMask;					/**< @internalTechnology */

const TUint32 KTTBRExtraBitsMask	=0x0000007f;	/**< @internalTechnology */	// Extra bits in TTBR in addition to physical address

const TInt KPageInfoShift = 5;

#ifdef __CPU_CACHE_HAS_COLOUR
const TInt KPageColourShift=2;
#else
const TInt KPageColourShift=0;
#endif
const TInt KPageColourCount=(1<<KPageColourShift);
const TInt KPageColourMask=KPageColourCount-1;	
	
const TInt KAbtStackSize=KPageSize;		/**< @internalComponent */
const TInt KUndStackSize=KPageSize;		/**< @internalComponent */
const TInt KIrqStackSize=KPageSize;		/**< @internalComponent */
const TInt KFiqStackSize=KPageSize;		/**< @internalComponent */
const TInt KExcStackAddressSpace = KIrqStackSize + KFiqStackSize + KUndStackSize + KAbtStackSize + 4*KPageSize;	/**< @internalComponent */

const TInt KMaxCPUs = 8;
__ASSERT_COMPILE(KExcptStacksLinearEnd-KExcptStacksLinearBase >= (TLinAddr)KExcStackAddressSpace * (TUint)KMaxCPUs);

#endif	// __MMBOOT_H__
