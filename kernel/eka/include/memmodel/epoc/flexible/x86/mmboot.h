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

// Linear address map
// 00000000-003FFFFF	Unmapped
// 00400000-7FFFFFFF	Local data
// 80000000-			ROM
//         -8FFFFFFF	Global user area
// 90000000-EFFFFFFF	Kernel memory
// F0000000-FEBFFFFF	Fixed kernel mappings
// FEC00000-FEFFFFFF	IO APIC + Local APIC
// FF000000-FFFFFFFF	???

// Linear addresses

const TLinAddr KUserLocalDataBase		=0x00400000u;

const TLinAddr KUserLocalDataEnd		=0x80000000u;

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

const TLinAddr KPageInfoMap				=0xF0010000u;
const TLinAddr KApTrampolinePageLin		=0xF0040000u;
const TLinAddr KTempAddr				=0xF0080000u;
const TLinAddr KTempAddrEnd				=0xF0100000u;
const TLinAddr KBiosInfoLin				=0xF0100000u;	// RAM info + MP info

const TLinAddr KPageTableInfoBase		=0xF0C00000u;
const TLinAddr KPageTableInfoEnd		=0xF1000000u;

const TLinAddr KPageArraySegmentBase	=0xF1000000u;
const TLinAddr KPageArraySegmentEnd		=0xF2000000u;

const TLinAddr KPageInfoLinearBase		=0xF2000000u;
const TLinAddr KPageInfoLinearEnd		=0xF4000000u;

const TLinAddr KPageDirectoryBase		=0xF4000000u;
const TLinAddr KPageDirectoryEnd		=0xF5000000u;

const TLinAddr KIPCAlias				=0xF6000000u;
const TLinAddr KIPCAliasAreaSize		=0x02000000u;

const TLinAddr KPageTableBase			=0xF8000000u;
const TLinAddr KPageTableEnd			=0xFEC00000u;

										// FEC00000-FEFFFFFF	IO APIC + Local APIC



// Constants for X86 MMU
#ifndef __KPAGESIZE_DEFINED__
const TInt KPageShift=12;
const TInt KPageSize=1<<KPageShift;
const TInt KPageMask=KPageSize-1;
#define __KPAGESIZE_DEFINED__
#endif
const TInt KChunkShift=22;
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

const TInt KPageDirectoryShift=32-KChunkShift+2;		// PDE is 4 bytes
const TInt KPageDirectorySize=1<<KPageDirectoryShift;
const TInt KPageDirectoryMask=KPageDirectorySize-1;

const TInt KNumOsAsids=1024;
const TUint KKernelOsAsid=0;

const TUint32 KPdePtePresent=0x01;
const TUint32 KPdePteWrite=0x02;
const TUint32 KPdePteUser=0x04;
const TUint32 KPdePteWriteThrough=0x08;
const TUint32 KPdePteUncached=0x10;
const TUint32 KPdePteAccessed=0x20;
const TUint32 KPdePteDirty=0x40;
const TUint32 KPdeLargePage=0x80;						// Pentium and above, not 486
const TUint32 KPdePteGlobal=0x100;						// P6 and above, not 486 or Pentium

const TUint32 KPdePtePhysAddrMask=0xfffff000u;
const TUint32 KPdeLargePagePhysAddrMask=0xffc00000u;	// Pentium and above, not 486
const TUint32 KPdeUnallocatedEntry=0;
const TUint32 KPteUnallocatedEntry=0;
const TUint32 KPdeMatchMask=KPdePteAccessed|KPdePteDirty|KPdePteUser|KPdePteWrite; // ignore 'user' and 'write' in PDEs
const TUint32 KPteMatchMask=KPdePteAccessed|KPdePteDirty;

const TInt KPageInfoShift = 5;

const TInt KPageColourShift=0;
const TInt KPageColourCount=(1<<KPageColourShift);
const TInt KPageColourMask=KPageColourCount-1;

#endif	// __MMBOOT_H__
