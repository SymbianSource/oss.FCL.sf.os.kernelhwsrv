// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\epoc\multiple\mdefrag.cpp
// 
//
#include <memmodel.h>
#include <defrag.h>
#include "mmboot.h"
#include <ramalloc.h>
#include "cache_maintenance.h"
/*
 * Move a kernel page from aOld to aNew, updating the page table in aChunk.
 * Enter with system locked, exit with system unlocked (!!)
 * Must hold RAM alloc mutex.
 */
TInt Mmu::MoveKernelPage(DChunk* aChunk, TUint32 aOffset, TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TBool aBlockRest)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Defrag::MoveKernelPage() off=%08x old=%08x",aOffset,aOld));
	Mmu& m=Mmu::Get();
	
	// Release the system lock - the kernel chunks can't ever be freed
	// and the ramalloc mutex protects us from decommit.
	NKern::UnlockSystem();

	// Allocate new page, map old and new
	TPhysAddr newPage;
	if (m.AllocRamPages(&newPage, 1, EPageMovable, aBlockZoneId, aBlockRest) != KErrNone)
		return KErrNoMemory;
	TLinAddr vOld = m.MapTemp(aOld, aOffset); // enough of address for page colour
	TLinAddr vNew = m.MapSecondTemp(newPage, aOffset);
	
	// With interrupts disabled, copy the page's contents and remap its PTE
	// System lock is required as well for Substitute
	NKern::LockSystem();
	TInt irq = NKern::DisableAllInterrupts();
	pagecpy((TAny*)vNew, (TAny*)vOld);
	aChunk->Substitute(aOffset, aOld, newPage);
	NKern::RestoreInterrupts(irq);
	NKern::UnlockSystem();

	// Before we sort out cache for the old page, check if the required mapping 
	// atributes for that operation is what we have at the moment.
	if (CacheMaintenance::TemporaryMapping() != EMemAttNormalCached)
		{
		// Remove temporary mapping and map old page as required by CacheMaintenance
		m.UnmapTemp();
		vOld = m.MapTemp(aOld, aOffset,1, CacheMaintenance::TemporaryMapping());
		}

	//Sort out cache for the memory not in use anymore.
	CacheMaintenance::PageToReuse(vOld, EMemAttNormalCached, aOld);

	// Unalias pages
	m.UnmapTemp();
	m.UnmapSecondTemp();

	// Free old page
#ifdef _DEBUG
	m.ClearPages(1, (TPhysAddr*)(aOld|1));
#endif
	m.iRamPageAllocator->FreeRamPage(aOld, EPageMovable);

	aNew = newPage;
	return KErrNone;
	}

/*
 * Move a code page from aOld to aNew, updating all page tables which refer
 * to it.
 * Enter with system locked, exit with system unlocked (!!)
 * Must hold RAM alloc mutex.
 */
TInt Mmu::MoveCodeSegMemoryPage(DMemModelCodeSegMemory* aCodeSegMemory, TUint32 aOffset, TPhysAddr aOld,
		TPhysAddr& aNew, TUint aBlockZoneId, TBool aBlockRest)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Defrag::MoveCodeSegMemoryPage() off=%08x old=%08x",aOffset,aOld));
	Mmu& m=Mmu::Get();

	// if the code seg is not done loading yet, we can't move it the easy way
	// also, if it's being unloaded the codeseg will have gone.
	DCodeSeg* codeseg = aCodeSegMemory->iCodeSeg;
	if (!codeseg || !(codeseg->iMark & DCodeSeg::EMarkLoaded))
		{
		NKern::UnlockSystem();
		return KErrInUse;
		}

	// Release system lock as page can't be decommitted while we hold ramalloc mutex
	NKern::UnlockSystem();

	// Allocate new page, map old and new
	TPhysAddr newPage;
	if (m.AllocRamPages(&newPage, 1, EPageMovable, aBlockZoneId, aBlockRest) != KErrNone)
		return KErrNoMemory;
	TLinAddr vOld = m.MapTemp(aOld, aOffset); // enough of address for page colour
	TLinAddr vNew = m.MapSecondTemp(newPage, aOffset);

	// Copy the page and remap it wherever it's still mapped
	// Need to clean the new page to get the data to icache
	pagecpy((TAny*)vNew, (TAny*)vOld);
	
	//Sort out cache for the code that has just been altered. 
	CacheMaintenance::CodeChanged(vNew, KPageSize);
	
	//Replace old page in the mapping with the new one.
	aCodeSegMemory->Substitute(aOffset, aOld, newPage);

	// Before we sort out cache for the old page, check if the required mapping 
	// atributes for that operation is what we have at the moment.
	if (CacheMaintenance::TemporaryMapping() != EMemAttNormalCached)
		{
		// Remove temporary mapping and map old page as required by CacheMaintenance
		m.UnmapTemp();
		vOld = m.MapTemp(aOld, aOffset,1, CacheMaintenance::TemporaryMapping());
		}

	//Sort out cache for the memory not in use anymore.
	CacheMaintenance::PageToReuse(vOld, EMemAttNormalCached, aOld);

	// Unalias pages
	m.UnmapTemp();
	m.UnmapSecondTemp();

	// Free old page
#ifdef _DEBUG
	m.ClearPages(1, (TPhysAddr*)(aOld|1));
#endif
	m.iRamPageAllocator->FreeRamPage(aOld, EPageMovable);

	aNew = newPage;
	return KErrNone;
	}

/*
 * Move a code chunk page from aOld to aNew, updating the page table in aChunk.
 * Enter with system locked, exit with system unlocked (!!)
 * Must hold RAM alloc mutex.
 */
TInt Mmu::MoveCodeChunkPage(DChunk* aChunk, TUint32 aOffset, TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TBool aBlockRest)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Defrag::MoveCodeChunkPage() off=%08x old=%08x",aOffset,aOld));
	Mmu& m=Mmu::Get();
	
	// look up the code seg that corresponds to this page
	TLinAddr aLinearAddress = (TLinAddr)(aChunk->Base() + (aOffset));
	DMemModelCodeSeg* codeseg = (DMemModelCodeSeg*)DCodeSeg::CodeSegsByAddress.Find(aLinearAddress);

	// if the code seg is not done loading yet, we can't move it the easy way
	if (!(codeseg->iMark & DCodeSeg::EMarkLoaded))
		{
		NKern::UnlockSystem();
		return KErrInUse;
		}

	// Release system lock as page can't be decommitted while we hold ramalloc mutex
	NKern::UnlockSystem();

	// Allocate new page, map old and new
	TPhysAddr newPage;
	if (m.AllocRamPages(&newPage, 1, EPageMovable, aBlockZoneId, aBlockRest) != KErrNone)
		return KErrNoMemory;
	TLinAddr vOld = m.MapTemp(aOld, aOffset); // enough of address for page colour
	TLinAddr vNew = m.MapSecondTemp(newPage, aOffset);

	// Copy the page and remap it
	// Need to clean the new page to get the data to icache
	pagecpy((TAny*)vNew, (TAny*)vOld);

	//Sort out cache for the code that has just been altered. 
	CacheMaintenance::CodeChanged(vNew, KPageSize);
	
	NKern::LockSystem();
	aChunk->Substitute(aOffset, aOld, newPage);
	NKern::UnlockSystem();

	// Before we sort out cache for the old page, check if the required mapping 
	// atributes for that operation is what we have at the moment.
	if (CacheMaintenance::TemporaryMapping() != EMemAttNormalCached)
		{
		// Remove temporary mapping and map old page as required by CacheMaintenance
		m.UnmapTemp();
		vOld = m.MapTemp(aOld, aOffset,1, CacheMaintenance::TemporaryMapping());
		}

	//Sort out cache for the memory not in use anymore.
	CacheMaintenance::PageToReuse(vOld, EMemAttNormalCached, aOld);
	
	// Unalias pages
	m.UnmapTemp();
	m.UnmapSecondTemp();

	// Free old page
#ifdef _DEBUG
	m.ClearPages(1, (TPhysAddr*)(aOld|1));
#endif
	m.iRamPageAllocator->FreeRamPage(aOld, EPageMovable);

	aNew = newPage;
	return KErrNone;
	}

/*
 * Move a data chunk page from aOld to aNew, updating the page table in aChunk.
 * Enter with system locked, exit with system unlocked (!!)
 * Must hold RAM alloc mutex.
 */
TInt Mmu::MoveDataChunkPage(DChunk* aChunk, TUint32 aOffset, TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TBool aBlockRest)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Defrag::MoveDataChunkPage() off=%08x old=%08x",aOffset,aOld));
	Mmu& m=Mmu::Get();
	TInt r;
	
	// Release system lock as page can't be decommitted while we hold ramalloc mutex
	NKern::UnlockSystem();

	// Allocate new page, map old and new
	TPhysAddr newPage;
	if (m.AllocRamPages(&newPage, 1, EPageMovable, aBlockZoneId, aBlockRest) != KErrNone)
		return KErrNoMemory;
	TLinAddr vOld = m.MapTemp(aOld, aOffset); // enough of address for page colour
	TLinAddr vNew = m.MapSecondTemp(newPage, aOffset);
	
	// Mark the PTE as readonly to avoid the data being overwritten while we copy
	DisablePageModification((DMemModelChunk*)aChunk, aOffset);

	// Copy the page's contents and remap its PTE
	pagecpy((TAny*)vNew, (TAny*)vOld);
	if (aChunk->iChunkType == EUserSelfModCode)//Sort out cache for the code that has just been altered
		CacheMaintenance::CodeChanged(vNew, KPageSize);		

	NKern::LockSystem();
	if (iDisabledPte != NULL)
		{
		// Access wasn't reenabled, so we can continue
		aChunk->Substitute(aOffset, aOld, newPage);
		iDisabledAddr = 0;
		iDisabledAddrAsid = -1;
		iDisabledPte = NULL;
		iDisabledOldVal = 0;
		r = KErrNone;
		}
	else
		r = KErrInUse;
	NKern::UnlockSystem();
	
	
	TLinAddr vUnused = vOld; 
	TPhysAddr pUnused = aOld;

	if (r != KErrNone)
		{
		//Substitute has failed. Sort out cache for the new page,  not the old one.
		vUnused = vNew;
		pUnused = newPage;
		}
	// Before we sort out cache for the unused page, check if the required mapping 
	// atributes for that operation is what we have at the moment.
	if (CacheMaintenance::TemporaryMapping() != EMemAttNormalCached)
		{
		// Remove temporary mapping and map the page as required by CacheMaintenance
		m.UnmapTemp();
		vUnused = m.MapTemp(pUnused, aOffset,1, CacheMaintenance::TemporaryMapping());
		}

	//Sort out cache for the memory not in use anymore.
	CacheMaintenance::PageToReuse(vUnused, EMemAttNormalCached, pUnused);

	// Unalias pages
	m.UnmapTemp();
	m.UnmapSecondTemp();

	if (r == KErrNone)
		{
		// Free old page
#ifdef _DEBUG
		m.ClearPages(1, (TPhysAddr*)(aOld|1));
#endif
		m.iRamPageAllocator->FreeRamPage(aOld, EPageMovable);
		aNew = newPage;
		}
	else
		{
		// Free new page
		m.iRamPageAllocator->FreeRamPage(newPage, EPageMovable);
		}

	return r;
	}
