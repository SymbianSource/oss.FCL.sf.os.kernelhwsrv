// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <memmodel.h>
#include <ramalloc.h>
#include "mm.h"
#include "mmu.h"
#include "mmanager.h"
#include "mobject.h"
#include "mpager.h"
#include "mmapping.h"


TInt M::RamDefragFault(TAny* /*aExceptionInfo*/)
	{// Defag faults are handled by Mmu::HandlePageFault() on the flexible memory model.
	return KErrAbort;
	} 


EXPORT_C TInt Epoc::MovePhysicalPage(TPhysAddr aOld, TPhysAddr& aNew, TRamDefragPageToMove aPageToMove)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Epoc::MovePhysicalPage");
	__KTRACE_OPT(KMMU,Kern::Printf("Epoc::MovePhysicalPage() old=%08x pageToMove=%d",aOld,aPageToMove));
	// Mark aNew as invalid if a page is moved then this will be updated.
	// However, if the page couldn't be moved or is discarded then it must be invalid.
	aNew = KPhysAddrInvalid;

	TInt r = KErrNotFound;

	switch (aPageToMove)
		{
		case ERamDefragPage_Physical :
			break;

		case ERamDefragPage_PageTable :
			{
			// Assume aOld is a linear address in current thread and get the physical
			// address of the page table that maps it and move that.
			TLinAddr linAddr = (TLinAddr) aOld;
			DMemModelThread* thread = (DMemModelThread*)TheCurrentThread;
			// Get the os asid of current thread's process so no need to open reference on it.
			TUint osAsid = ((DMemModelProcess*)thread->iOwningProcess)->OsAsid();
			TUint offsetInMapping;
			TUint mapInstanceCount;
			DMemoryMapping* mapping = MM::FindMappingInAddressSpace(osAsid,linAddr,1,offsetInMapping,mapInstanceCount);
			if (!mapping)
				return r;
			MmuLock::Lock();
			TUint memoryIndex = (offsetInMapping >> KPageShift)+ mapping->iStartIndex;
			TPte* pte = mapping->FindPageTable(linAddr, memoryIndex);
			if (mapInstanceCount != mapping->MapInstanceCount() || !pte)
				{
				MmuLock::Unlock();
				mapping->Close();
				return r;
				}
			TPhysAddr physAddr = TheMmu.LinearToPhysical((TLinAddr)pte, KKernelOsAsid);
			__NK_ASSERT_DEBUG(physAddr != KPhysAddrInvalid);
			aOld = physAddr;	// Have physical address of page table page so move it.
			MmuLock::Unlock();
			mapping->Close();
			break;
			}

		case ERamDefragPage_PageTableInfo :
			{
			// Assume aOld is a linear address in current thread and get physical 
			// address of the page table info of the page table that maps it 
			// and move that.
			TLinAddr linAddr = (TLinAddr) aOld;
			DMemModelThread* thread = (DMemModelThread*)TheCurrentThread;
			// Get the os asid of current thread's process so no need to open reference on it.
			TUint osAsid = ((DMemModelProcess*)thread->iOwningProcess)->OsAsid();
			TUint offsetInMapping;
			TUint mapInstanceCount;
			DMemoryMapping* mapping = MM::FindMappingInAddressSpace(osAsid,linAddr,1,offsetInMapping,mapInstanceCount);
			if (!mapping)
				return r;
			MmuLock::Lock();
			TUint memoryIndex = (offsetInMapping >> KPageShift)+ mapping->iStartIndex;
			TPte* pte = mapping->FindPageTable(linAddr, memoryIndex);
			if (mapInstanceCount != mapping->MapInstanceCount() || !pte)
				{
				MmuLock::Unlock();
				mapping->Close();
				return r;
				}
			
			SPageTableInfo* pti = SPageTableInfo::FromPtPtr(pte);
			TPhysAddr physAddr = TheMmu.LinearToPhysical((TLinAddr)pti, KKernelOsAsid);
			__NK_ASSERT_DEBUG(physAddr != KPhysAddrInvalid);
			aOld = physAddr;	// Have physical address of page table info page so move it.
			MmuLock::Unlock();
			mapping->Close();
			break;
			}

		default :
			r = KErrNotSupported;
			return r;
		}

	RamAllocLock::Lock();

	// Move the page to any RAM zone.
	r = M::MovePage(aOld, aNew, KRamZoneInvalidId, EFalse);

	RamAllocLock::Unlock();
	return r;
	}


TInt M::MovePage(TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TBool aBlockRest)
	{
	TInt r;

	// get memory object corresponding to the page...
	DMemoryObject* memory = 0;
	MmuLock::Lock();
	SPageInfo* pi = SPageInfo::SafeFromPhysAddr(aOld&~KPageMask);
	if(pi)
		{
		if (pi->PagedState() != SPageInfo::EUnpaged)
			{// The page is paged so let the pager handle it.
			return ThePager.DiscardPage(pi, aBlockZoneId, aBlockRest);
			}
		if (pi->Type()==SPageInfo::EManaged)
			memory = pi->Owner();
		}
	MmuLock::Unlock();

	// Note, whilst we hold the RamAllocLock the page can't change it's use
	// and we can safely assume that it still belongs to the memory object
	// at a fixed page index.
	// Also, as memory objects can't be destroyed whilst they still own pages
	// we can safely access this object without taking an explicit referernce,
	// i.e. we don't need to Open() the memory object.
	if (!pi)
		{// page info for aOld not found so aOld is not a RAM page...
		r = KErrArgument;
		}
	else if(!memory)
		{
		// page does not have a memory manager, so we can't move it...
		r = KErrNotSupported;
		}
	else
		{
		// move page...
		r = memory->iManager->MovePage(memory, pi, aNew, aBlockZoneId, aBlockRest);
		}
	return r;
	}
