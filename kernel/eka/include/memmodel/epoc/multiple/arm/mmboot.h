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
// e32\include\memmodel\epoc\multiple\arm\mmboot.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __MMBOOT_H__
#define __MMBOOT_H__
#include <arm.h>
#include <memmodel.h>
#include <kernel/cache.h>

//
// Linear address map (1Gb configuration) :
// 00000000-001FFFFF	Unmapped
// 00200000-002FFFFF	IPC Alias region
// 00300000-003FFFFF	Unmapped
// 00400000-1FFFFFFF	Local data
// 20000000-3BFFFFFF	Shared data
// 3C000000-3DFFFFFF	RAM loaded code (=phys ram size up to 256M)
// 3E000000-3FFFFFFF	DLL static data (=phys ram size/2 up to 128M)
// 40000000-7FFFFFFF	Unused
// 80000000-8FFFFFFF	ROM
// 90000000-9FFFFFFF	User Global Area
// A0000000-BFFFFFFF	RAM drive
// C0000000-C0001FFF	Super page/CPU page
// C0030000-C0030FFF	KPageInfoMap
// C0038000-C003FFFF	IRQ, FIQ, UND, ABT stacks (4*4K for stacks + 4*4K for guard pages) 
// C0040000-C00403FF	ASID info (256 ASIDs)
// C0080000-C00FFFFF	Page table info	
// C1000000-C13FFFFF	Page directories (up to 256 * 16KB)
// C2000000-C5FFFFFF	Page tables
// C6000000-C6FFFFFF	Primary I/O mappings
// C7000000-C7FFFFFF
// C8000000-C8FFFFFF	Kernel .data/.bss, initial stack, kernel heap
// C9000000-C91FFFFF	Kernel stacks
// C9200000-FBFFFFFF	Extra kernel mappings (I/O, RAM loaded device drivers)
// FC000000-FDFFFFFF	Page Info array
// FFF00000-FFFFFFFF	Exception vectors
//
//
// Linear address map (2Gb configuration) :
// 00000000-001FFFFF	Unmapped
// 00200000-002FFFFF	IPC Alias region
// 00300000-003FFFFF	Unmapped
// 00400000-37FFFFFF	Local data
// 38000000-3FFFFFFF	DLL static data (=phys ram size/2 up to 128M)
// 40000000-6FFFFFFF	Shared data
// 70000000-7FFFFFFF	RAM loaded code (=phys ram size up to 256M)
// 80000000-8FFFFFFF	ROM
// 90000000-9FFFFFFF	User Global Area
// A0000000-BFFFFFFF	RAM drive
// C0000000-C0001FFF	Super page/CPU page
// C0030000-C0030FFF	KPageInfoMap
// C0038000-C003FFFF	IRQ, FIQ, UND, ABT stacks (4*4K for stacks + 4*4K for guard pages) 
// C0040000-C00403FF	ASID info (256 ASIDs)
// C0080000-C00FFFFF	Page table info	
// C1000000-C13FFFFF	Page directories (up to 256 * 16KB)
// C2000000-C5FFFFFF	Page tables
// C6000000-C6FFFFFF	Primary I/O mappings
// C7000000-C7FFFFFF
// C8000000-C8FFFFFF	Kernel .data/.bss, initial stack, kernel heap
// C9000000-C91FFFFF	Kernel stacks
// C9200000-FBFFFFFF	Extra kernel mappings (I/O, RAM loaded device drivers)
// FC000000-FDFFFFFF	Page Info array
// FFF00000-FFFFFFFF	Exception vectors
//

// Linear addresses
const TLinAddr KIPCAlias				=0x00200000u;
const TLinAddr KUserLocalDataBase		=0x00400000u;
const TLinAddr KUserSharedDataBase1GB	=0x20000000u;
const TLinAddr KUserSharedDataEnd1GB	=0x40000000u;
const TLinAddr KUserSharedDataBase2GB	=0x40000000u;
const TLinAddr KUserSharedDataEnd2GB	=0x80000000u;

const TLinAddr KRomLinearBase			=0x80000000u;
const TLinAddr KRomLinearEnd			=0x90000000u;
const TLinAddr KUserGlobalDataBase		=0x90000000u;
const TLinAddr KUserGlobalDataEnd		=0xA0000000u;
const TLinAddr KRamDriveStartAddress	=0xA0000000u;
const TInt KRamDriveMaxSize=0x20000000;
const TLinAddr KRamDriveEndAddress		=0xC0000000u;
const TLinAddr KSuperPageLinAddr		=0xC0000000u;
const TLinAddr KExcptStacksLinearBase	=0xC0038000u;
const TLinAddr KAsidInfoBase			=0xC0040000u;
const TLinAddr KPageTableInfoBase		=0xC0080000u;
const TLinAddr KPageDirectoryBase		=0xC1000000u;
const TLinAddr KPageTableBase			=0xC2000000u;
const TLinAddr KPrimaryIOBase			=0xC6000000u;
const TLinAddr KKernelDataBase			=0xC8000000u;
const TLinAddr KKernelDataEnd			=0xC9200000u;
const TLinAddr KKernelSectionEnd		=0xFC000000u;

const TLinAddr KPageInfoLinearBase		=0xFC000000u;

const TLinAddr KMachineConfigLinAddr	=0xC0000800u;
const TLinAddr KDummyUncachedAddr		=0xC000F000u;
const TLinAddr KTempAddr				=0xC0010000u;
const TLinAddr KSecondTempAddr			=0xC0014000u;
const TLinAddr KDefragAltStackAddr		=0xC001F000u;
const TLinAddr KDemandPagingTempAddr	=0xC0020000u;	// used by demand paging (size of region is 0x10000)
const TLinAddr KPageInfoMap				=0xC0030000u;


// Domain usage
//
// 0 All, except...
// 1 RAM Drive
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
const TInt KPtBlockShift=KPageShift-3;					/**< @internalTechnology */	// sizeof(SPageTableInfo)=8
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

const TInt KArmV6NumAsids=256;

// Permissions - 3 bit field, APX most significant. When __CPU_MEMORY_TYPE_REMAPPING defined, LSB must be 1
#if defined(__CPU_MEMORY_TYPE_REMAPPING)
const TInt KArmV6PermRORO=7;		/**< @internalTechnology */ // sup RO user RO
#else
const TInt KArmV6PermNONO=0;		/**< @internalTechnology */ // no access for anyone
const TInt KArmV6PermRWRO=2;		/**< @internalTechnology */ // sup RW user RO
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
const TUint32 KArmV6PteAPX			=0x00000200;/**< @internalTechnology */	// RO / !RW
const TUint32 KArmV6PteAP1			=0x00000020;/**< @internalTechnology */	// AllAccess / !KernelOnly
const TUint32 KArmV6PteAP0			=0x00000010;/**< @internalTechnology */	// Must be set

const TPde KPdePresentMask=KArmV6PdeTypeMask;				/**< @internalTechnology */
const TPde KPdeTypeMask=KArmV6PdeTypeMask;					/**< @internalTechnology */
const TPde KPdeSectionAddrMask=0xfff00000;					/**< @internalTechnology */
const TPde KPdePageTableAddrMask=0xfffffc00;				/**< @internalTechnology */
const TPte KPteLargePageAddrMask=0xffff0000;				/**< @internalTechnology */
const TPte KPteSmallPageAddrMask=0xfffff000;				/**< @internalTechnology */
const TInt KLargeSmallPageRatio=KLargePageSize/KPageSize;	/**< @internalTechnology */
const TPde KPdeNotPresentEntry=0;							/**< @internalTechnology */
const TPte KPteNotPresentEntry=0;							/**< @internalTechnology */
const TPte KPtePresentMask=KArmV6PteTypeMask;				/**< @internalTechnology */
const TPte KPteTypeMask=KArmV6PteTypeMask;					/**< @internalTechnology */

const TUint32 KTTBRExtraBitsMask	=0x0000007f;	/**< @internalTechnology */	// Extra bits in TTBR in addition to physical address

const TInt KPageInfoShift = 5;

const TInt KAbtStackSize=KPageSize;		/**< @internalComponent */
const TInt KUndStackSize=KPageSize;		/**< @internalComponent */
const TInt KIrqStackSize=KPageSize;		/**< @internalComponent */
const TInt KFiqStackSize=KPageSize;		/**< @internalComponent */

#endif	// __MMBOOT_H__
