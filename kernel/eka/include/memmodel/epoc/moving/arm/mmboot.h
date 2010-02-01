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
// e32\include\memmodel\epoc\moving\arm\mmboot.h
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
// Linear address map:
// 00000000-003FFFFF	Unmapped
// 00400000-2FFFFFFF	Moving process data
// 30000000-3FFFFFFF	DLL static data (=phys ram size/2 up to 128M, always ends at 40000000)
// 40000000-5FFFFFFF	RAM drive
// 60000000-60001FFF	Super page/CPU page
// 60030000-600303FF	KPageInfoMap
// 60038000-6003FFFF	IRQ, FIQ, UND, ABT stacks (4*4K for stacks + 4*4K for guard pages) 
// 61000000-61003FFF	Page directory (16K)
// 61020000-6103FFFF	Page table info (4096 * 8bytes = 32K)
// 61100000-611FFFFF	Cache flush area
// 61200000-612FFFFF	Alternate cache flush area
// 62000000-623FFFFF	Page tables (up to 4096 * 1K)
// 63000000-63FFFFFF	Primary I/O mappings
// 64000000-64FFFFFF	Kernel .data/.bss, initial stack, kernel heap
// 65000000-655FFFFF	fixed processes - usually 2 or 3Mb each.
// 65600000-F1FFFFFF	Kernel section (includes extra I/O mappings)
// F0000000-F1FFFFFF	Kernel code (RAM size/2)
// F2000000-F5FFFFFF	User code (RAM size)
// F6000000-F7FFFFFF	Page Info array
// F8000000-FFEFFFFF	ROM
// FFF00000-FFFFFFFF	Exception vectors
//

// Linear addresses
const TLinAddr	KDataSectionBase		=0x00400000u;
const TLinAddr	KDataSectionEnd			=0x40000000u;
const TLinAddr	KRamDriveStartAddress	=0x40000000u;
const TInt		KRamDriveMaxSize		=0x20000000;
const TLinAddr	KRamDriveEndAddress		=0x60000000u;

const TLinAddr	KPageInfoLinearBase		=0xF6000000u;

const TLinAddr	KRomLinearBase			=0xF8000000u;
const TLinAddr	KRomLinearEnd			=0xFFF00000u;
const TLinAddr	KSuperPageLinAddr		=0x60000000u;
const TLinAddr  KExcptStacksLinearBase	=0x60038000u;
const TLinAddr	KPageDirectoryBase		=0x61000000u;
const TLinAddr	KPageTableInfoBase		=0x61020000u;
const TLinAddr	KPageTableBase			=0x62000000u;
const TLinAddr	KPrimaryIOBase			=0x63000000u;
const TLinAddr  KKernelDataBase			=0x64000000u;
const TLinAddr  KKernelDataEnd			=0x65000000u;
const TLinAddr	KKernelSectionEnd		=0xFFF00000u;	// we always use HIVECS

const TLinAddr	KMachineConfigLinAddr	=0x60000800u;
const TLinAddr	KDummyUncachedAddr		=0x6000F000u;
const TLinAddr	KTempAddr				=0x60010000u;
const TLinAddr	KSecondTempAddr			=0x60014000u;
const TLinAddr	KDefragAltStackAddr		=0x6001F000u;
const TLinAddr	KPageInfoMap			=0x60030000u;

const TLinAddr	KDCacheFlushArea		=0x61100000u;
const TInt		KDCacheFlushAreaLimit	=0x00080000;	// 512k
const TLinAddr	KAltDCacheFlushArea		=0x61200000u;
const TInt		KAltDCacheFlushAreaLimit=0x00080000;	// 512k

// Constants for ARM MMU
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

const TPde KPdePresentMask=3;
const TPde KPdeTypeMask=0x3;
const TPde KPdeSectionAddrMask=0xfff00000;
const TPde KPdePageTableAddrMask=0xfffffc00;
const TPte KPteLargePageAddrMask=0xffff0000;
const TPte KPteSmallPageAddrMask=0xfffff000;
const TInt KLargeSmallPageRatio=KLargePageSize/KPageSize;
const TPde KPdeNotPresentEntry=0;
const TPte KPteNotPresentEntry=0;
const TPte KPtePresentMask=0x3;
const TPte KPteTypeMask=0x3;


// Domain usage
//
/** @internalComponent */
enum TArmDomain
	{
	EDomainVarUserRun=0,
	EDomainClient=1,
	EDomainPageTable=2,
	EDomainRamDrive=3,

	ENumDomains=16
	};

// default domain access is 0=manager, 1=client, 2,3=no access, 4-15=client
const TUint32 KDefaultDomainAccess			 = 0x55555507u;	/**< @internalComponent */
const TUint32 KSupervisorInitialDomainAccess = 0x55555557u;	/**< @internalTechnology */

const TUint32 KManzanoTTBRExtraBits		=0x00000018;	/**< @internalTechnology On Manzano, page table walk is L2 cachable*/

const TInt KPageInfoShift = 5;

const TInt KAbtStackSize=KPageSize;		/**< @internalComponent */
const TInt KUndStackSize=KPageSize;		/**< @internalComponent */
const TInt KIrqStackSize=KPageSize;		/**< @internalComponent */
const TInt KFiqStackSize=KPageSize;		/**< @internalComponent */

#endif	// __MMBOOT_H__
