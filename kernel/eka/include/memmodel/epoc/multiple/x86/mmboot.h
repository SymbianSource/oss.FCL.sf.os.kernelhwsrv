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
// e32\include\memmodel\epoc\multiple\x86\mmboot.h
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __MMBOOT_H__
#define __MMBOOT_H__
#include <x86.h>
#include <memmodel.h>
#include <kernel/cache.h>

//
// Linear address map:
// 00000000-003FFFFF	Unmapped except for AP boot page at 7F000
// 00400000-37FFFFFF	Local data
// 38000000-3FFFFFFF	DLL static data (=phys ram size/2 up to 128M)
// 40000000-6FFFFFFF	Shared data
// 70000000-7FFFFFFF	RAM loaded code (=phys ram size up to 256M)
// 80000000-8FFFFFFF	ROM
// 90000000-9FFFFFFF	User Global Area
// A0000000-BFFFFFFF	RAM drive
// C0000000-C0001FFF	Super page/CPU page (extends to C0005FFF for SMP)
// C0006000-C0006FFF	Trampoline page for SMP
// C0030000-C00303FF	KPageInfoMap
// C0040000-C004FFFF	ASID info
// C0080000-C00FFFFF	Page table info	
// C1000000-C1FFFFFF	Page directories
// C2000000-C5FFFFFF	Page tables
// C6000000-C6FFFFFF	Primary I/O mappings
// C7000000-C7FFFFFF
// C8000000-C8FFFFFF	Kernel .data/.bss, initial stack, kernel heap
// C9000000-C93FFFFF	Kernel stacks
// C9400000-F9FFFFFF	Extra kernel mappings (I/O, RAM loaded device drivers)
// FA000000-FA3FFFFF	IPC Alias for CPU 0
// FA400000-FA7FFFFF	IPC Alias for CPU 1 (SMP only)
// FA800000-FABFFFFF	IPC Alias for CPU 2 (SMP only)
// FAC00000-FAFFFFFF	IPC Alias for CPU 3 (SMP only)
// FB000000-FB3FFFFF	IPC Alias for CPU 4 (SMP only)
// FB400000-FB7FFFFF	IPC Alias for CPU 5 (SMP only)
// FB800000-FBBFFFFF	IPC Alias for CPU 6 (SMP only)
// FBC00000-FBFFFFFF	IPC Alias for CPU 7 (SMP only)
// FC000000-FDFFFFFF	Page Info array
// FE000000-FEBFFFFF	Unused
// FEC00000-FEFFFFFF	IO APIC + Local APIC
// FF000000-FFFFFFFF	Unused

// Linear addresses
const TLinAddr KUserLocalDataBase	=0x00400000u;
const TLinAddr KUserSharedDataBase	=0x40000000u;
const TLinAddr KUserSharedDataEnd	=0x80000000u;	// after code
const TLinAddr KRomLinearBase		=0x80000000u;
const TLinAddr KRomLinearEnd		=0x90000000u;
const TLinAddr KUserGlobalDataBase	=0x90000000u;
const TLinAddr KUserGlobalDataEnd	=0xA0000000u;
const TLinAddr KRamDriveStartAddress=0xA0000000u;
const TInt KRamDriveMaxSize=0x20000000;
const TLinAddr KRamDriveEndAddress	=0xC0000000u;
const TLinAddr KSuperPageLinAddr	=0xC0000000u;
const TLinAddr KPageInfoMap			=0xC0030000u;
const TLinAddr KAsidInfoBase		=0xC0040000u;
const TLinAddr KPageTableInfoBase	=0xC0080000u;
const TLinAddr KPageDirectoryBase	=0xC1000000u;
const TLinAddr KPageTableBase		=0xC2000000u;
const TLinAddr KPrimaryIOBase		=0xC6000000u;
const TLinAddr KKernelSectionEnd	=0xFA000000u;
const TLinAddr KIPCAlias			=0xFA000000u;	// for SMP 4MB for each CPU
													// Thus CPU0->FC0, CPU1->FC4, ..., CPU7->FDC
const TUint32  KIPCAliasAreaSize	=0x02000000u;	// total size of alias area
const TLinAddr KPageInfoLinearBase	=0xFC000000u;

const TLinAddr KMachineConfigLinAddr=0xC0000800u;
const TLinAddr KTempAddr=0xC0010000u;
const TLinAddr KSecondTempAddr=0xC0014000u;
const TLinAddr KDefragAltStackAddr=0xC001F000u;
const TLinAddr KApTrampolinePageLin	=0xC0006000u;
const TLinAddr KBiosInfoLin			=0xC0100000u;	// RAM info + MP info

// Constants for X86 MMU
const TInt KChunkShift=22;
const TInt KChunkSize=1<<KChunkShift;
const TInt KChunkMask=KChunkSize-1;
const TInt KPageTableShift=KChunkShift-12+2;	// PTE is 4 bytes
const TInt KPageTableSize=1<<KPageTableShift;
const TInt KPageTableMask=KPageTableSize-1;
const TInt KPtClusterShift=12-KPageTableShift;
const TInt KPtClusterSize=1<<KPtClusterShift;
const TInt KPtClusterMask=KPtClusterSize-1;
const TInt KPtBlockShift=12-3;					/**< @internalTechnology */	// sizeof(SPageTableInfo)=8
const TInt KPtBlockSize=1<<KPtBlockShift;		/**< @internalTechnology */
const TInt KPtBlockMask=KPtBlockSize-1;			/**< @internalTechnology */
const TInt KPagesInPDEShift=KChunkShift-12;
const TInt KPagesInPDE=1<<KPagesInPDEShift;
const TInt KPagesInPDEMask=KPagesInPDE-1;


const TInt KPageInfoShift = 5;

#endif	// __MMBOOT_H__
