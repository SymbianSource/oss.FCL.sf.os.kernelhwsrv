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
//

#include "memmodel.h"
#include "mm.h"
#include "mmu.h"
#include "mobject.h"
#include "mmapping.h"
#include "mmanager.h"
#include "mpdalloc.h"
#include "mptalloc.h"
#include "mpager.h"
#include "maddressspace.h"




//
// DMutexPool
//

DMutexPool::~DMutexPool()
	{
	TUint i;
	for(i=0; i<iCount; ++i)
		{
		DMutex* mutex = iMembers[i].iMutex;
		if(mutex)
			mutex->Close(0);
		}
	Kern::Free(iMembers);
	}


TInt DMutexPool::Create(TUint aCount, const TDesC* aName, TUint aOrder)
	{
	if(aCount>EMaxPoolSize)
		return KErrTooBig;

	iMembers = (SMember*)Kern::AllocZ(aCount*sizeof(SMember));
	if(!iMembers)
		return KErrNoMemory;

	iCount = aCount;

	TInt r = KErrNone;
	TUint i;
	for(i=0; i<aCount; ++i)
		{
		TKName name;
		if(aName)
			{
			name = *aName;
			name.AppendNum(i);
			}
		K::MutexCreate(iMembers[i].iMutex, name, NULL, EFalse, aOrder);
		if(r!=KErrNone)
			break;
		}

	return r;
	}


/**
@class DMutexPool
@details

The cookie used for dynamically assigned mutexes is broken into three bit fields:
- Bit 0, always set. (To distinguish the cookie from a proper DMutex*).
- Bits 1 through #KMutexPoolIndexBits, these contain the index of the assigned
  mutex within DMutexPool::iMembers.
- Bits (#KMutexPoolIndexBits+1) through 31, the count of the number of threads waiting
  for this particular mutex assignment. When this reaches zero, the mutex can
  be unassigned.
*/

/**
Number of bits used to contain the index value of a dynamically assigned pool mutex.
*/
const TUint KMutexPoolIndexBits = 7;

const TUint KMutexPoolIndexMask = ((1<<KMutexPoolIndexBits)-1)<<1;
const TUint KMutexPoolWaitCountIncrement = 1<<(KMutexPoolIndexBits+1);

__ASSERT_COMPILE(DMutexPool::EMaxPoolSize<=TUint(KMutexPoolIndexMask/2+1)); // required for algorithm correctness

__ASSERT_COMPILE(DMutexPool::EMaxPoolSize<=64); // required to avoid excessive system lock hold time


void DMutexPool::Wait(DMutex*& aMutexRef)
	{
	NKern::LockSystem();

	TUintPtr poolMutex = (TUintPtr)aMutexRef;
	if(!poolMutex)
		{
		// try and find a free mutex, else use the next one...
		TUint next = iNext;
		do
			{
			if(iMembers[next].iUseCount==0)
				break;
			if(++next>=iCount)
				next = 0;
			}
		while(next!=iNext);
		// use found mutex...
		++iMembers[next].iUseCount;
		poolMutex = (next*2)+1; // mutex index*2 | 1
		// update next...
		if(++next>=iCount)
			next = 0;
		iNext = next;
		}

	DMutex* mutex = (DMutex*)poolMutex;
	if(poolMutex&1)
		{
		// mutex is a pool mutex, get pointer, and update wait count...
		SMember* member = &iMembers[(poolMutex&KMutexPoolIndexMask)>>1];
		mutex = member->iMutex;
		poolMutex += KMutexPoolWaitCountIncrement;
		__NK_ASSERT_ALWAYS(poolMutex>=KMutexPoolWaitCountIncrement);
		aMutexRef = (DMutex*)poolMutex;
		}

	mutex->Wait();

	NKern::UnlockSystem();
	}


void DMutexPool::Signal(DMutex*& aMutexRef)
	{
	NKern::LockSystem();

	TUintPtr poolMutex = (TUintPtr)aMutexRef;
	__NK_ASSERT_ALWAYS(poolMutex);

	DMutex* mutex = (DMutex*)poolMutex;

	if(poolMutex&1)
		{
		// mutex is a pool mutex, get pointer, and update wait count...
		SMember* member = &iMembers[(poolMutex&KMutexPoolIndexMask)>>1];
		mutex = member->iMutex;
		__NK_ASSERT_ALWAYS(poolMutex>=KMutexPoolWaitCountIncrement);
		poolMutex -= KMutexPoolWaitCountIncrement;
		if(poolMutex<KMutexPoolWaitCountIncrement)
			{
			--member->iUseCount;
			poolMutex = 0;
			}
		aMutexRef = (DMutex*)poolMutex;
		}

	mutex->Signal();
	}


TBool DMutexPool::IsHeld(DMutex*& aMutexRef)
	{
	TBool held = false;
	NKern::LockSystem();
	TUintPtr poolMutex = (TUintPtr)aMutexRef;
	if(poolMutex)
		{
		DMutex* mutex = (DMutex*)poolMutex;
		if(poolMutex&1)
			{
			SMember* member = &iMembers[(poolMutex&KMutexPoolIndexMask)>>1];
			mutex = member->iMutex;
			}
		held = mutex->iCleanup.iThread==&Kern::CurrentThread();
		}
	NKern::UnlockSystem();
	return held;
	}



//
// DReferenceCountedObject
//

DReferenceCountedObject::~DReferenceCountedObject()
	{
	__NK_ASSERT_DEBUG(iReferenceCount==0);
	}


void DReferenceCountedObject::Open()
	{
	CHECK_PRECONDITIONS(MASK_NO_KILL_OR_SUSPEND, "DReferenceCountedObject::Open");
	TInt orig = __e32_atomic_tas_ord32(&iReferenceCount, 1, 1, 0);
	if (orig <= 0)
		__crash();
	}


TBool DReferenceCountedObject::TryOpen()
	{
	CHECK_PRECONDITIONS(MASK_NO_KILL_OR_SUSPEND, "DReferenceCountedObject::Open");
	TInt orig = __e32_atomic_tas_ord32(&iReferenceCount, 1, 1, 0);
	return (orig>0);
	}


TBool DReferenceCountedObject::CheckCloseIsSafe()
	{
	__ASSERT_CRITICAL
#ifdef _DEBUG
	NFastMutex* fm = NKern::HeldFastMutex();
	if(fm)
		{
		Kern::Printf("DReferenceCountedObject[0x%08x]::Close() fast mutex violation %M",this,fm);
		return false;
		}
	SDblQue& ml = TheCurrentThread->iMutexList;
	if(!ml.IsEmpty())
		{
		DMutex* m = _LOFF(ml.First(), DMutex, iOrderLink);
		if(m->iOrder<KMutexOrdKernelHeap)
			{
			Kern::Printf("DReferenceCountedObject[0x%08x]::Close() mutex order violation holding mutex %O",this,m);
			return false;
			}
		}
#endif
	return true;
	}


void DReferenceCountedObject::Close()
	{
	__NK_ASSERT_DEBUG(CheckCloseIsSafe());
	TInt orig = __e32_atomic_tas_ord32(&iReferenceCount, 1, -1, 0);
	if (orig == 1)
		delete this;
	else if (orig <= 0)
		__crash();
	}


void DReferenceCountedObject::AsyncClose()
	{
	CHECK_PRECONDITIONS(MASK_NO_KILL_OR_SUSPEND, "DReferenceCountedObject::AsyncClose");
	TInt orig = __e32_atomic_tas_ord32(&iReferenceCount, 1, -1, 0);
	if (orig == 1)
		AsyncDelete();
	else if (orig <= 0)
		__crash();
	}


//
// Memory object functions
//

TInt MM::MemoryNew(DMemoryObject*& aMemory, TMemoryObjectType aType, TUint aPageCount, TMemoryCreateFlags aCreateFlags, TMemoryAttributes aAttributes)
	{
	TRACE(("MM::MemoryNew(?,0x%08x,0x%08x,0x%08x,0x%08x)",aType,aPageCount,aCreateFlags,*(TUint32*)&aAttributes));

	DMemoryManager* manager;
	if(aCreateFlags&EMemoryCreateCustomManager)
		manager = (DMemoryManager*)aType;
	else
		{
		switch(aType)
			{
		case EMemoryObjectUnpaged:
			manager = TheUnpagedMemoryManager;
			break;
		case EMemoryObjectMovable:
			manager = TheMovableMemoryManager;
			break;
		case EMemoryObjectPaged:
			manager = TheDataPagedMemoryManager;
			break;
		case EMemoryObjectDiscardable:
			manager = TheDiscardableMemoryManager;
			break;
		case EMemoryObjectHardware:
			manager = TheHardwareMemoryManager;
			break;
		default:
			manager = 0;
			__NK_ASSERT_DEBUG(0);
			break;
			}
		}
	TMemoryCreateFlags flags = (TMemoryCreateFlags)(aCreateFlags&~(EMemoryCreateDemandPaged));
	TInt r = manager->New(aMemory,aPageCount,aAttributes,flags);
	TRACE(("MM::MemoryNew returns %d, aMemory=0x%08x",r,aMemory));
#ifdef BTRACE_FLEXIBLE_MEM_MODEL
	if (r == KErrNone)
		aMemory->BTraceCreate();
#endif
	return r;
	}


TInt MM::MemoryClaimInitialPages(DMemoryObject* aMemory, TLinAddr aBase, TUint aSize, TMappingPermissions aPermissions, TBool aAllowGaps, TBool aAllowNonRamPages)
	{
	TRACE(("MM::MemoryClaimInitialPages(0x%08x,0x%08x,0x%08x,0x%08x,%d,%d)",aMemory,aBase,aPermissions,aSize,aAllowGaps!=0,aAllowNonRamPages!=0));
	TInt r = aMemory->ClaimInitialPages(aBase,aSize,aPermissions,aAllowGaps,aAllowNonRamPages);
	TRACE(("MM::MemoryClaimInitialPages returns %d",r));
	__NK_ASSERT_DEBUG(r==KErrNone);
	return r;
	}


void MM::MemorySetLock(DMemoryObject* aMemory, DMutex* aLock)
	{
	aMemory->SetLock(aLock);
	}


void MM::MemoryLock(DMemoryObject* aMemory)
	{
	MemoryObjectLock::Lock(aMemory);
	}


void MM::MemoryUnlock(DMemoryObject* aMemory)
	{
	MemoryObjectLock::Unlock(aMemory);
	}


void MM::MemoryDestroy(DMemoryObject*& aMemory)
	{
	DMemoryObject* memory = (DMemoryObject*)__e32_atomic_swp_ord_ptr(&aMemory, 0);
	if (!memory)
		return;
	TRACE(("MM::MemoryDestroy(0x%08x)",memory));
#ifdef BTRACE_FLEXIBLE_MEM_MODEL
	BTraceContext4(BTrace::EFlexibleMemModel,BTrace::EMemoryObjectDestroy,memory);
#endif
	memory->iManager->Destruct(memory);
	}


TInt MM::MemoryAlloc(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	TRACE(("MM::MemoryAlloc(0x%08x,0x%08x,0x%08x)",aMemory,aIndex,aCount));
	MemoryObjectLock::Lock(aMemory);
	TInt r;
	if(!aMemory->CheckRegion(aIndex,aCount))
		r = KErrArgument;
	else
		r = aMemory->iManager->Alloc(aMemory,aIndex,aCount);
	MemoryObjectLock::Unlock(aMemory);
	TRACE(("MM::MemoryAlloc returns %d",r));
	return r;
	}


TInt MM::MemoryAllocContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TUint aAlign, TPhysAddr& aPhysAddr)
	{
	TRACE(("MM::MemoryAllocContiguous(0x%08x,0x%08x,0x%08x,%d,?)",aMemory,aIndex,aCount,aAlign));
	MemoryObjectLock::Lock(aMemory);
	TInt r;
	if(!aMemory->CheckRegion(aIndex,aCount))
		r = KErrArgument;
	else
		r = aMemory->iManager->AllocContiguous(aMemory,aIndex,aCount,MM::RoundToPageShift(aAlign),aPhysAddr);
	MemoryObjectLock::Unlock(aMemory);
	TRACE(("MM::MemoryAlloc returns %d (aPhysAddr=0x%08x)",r,aPhysAddr));
	return r;
	}


void MM::MemoryFree(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	TRACE(("MM::MemoryFree(0x%08x,0x%08x,0x%08x)",aMemory,aIndex,aCount));
	MemoryObjectLock::Lock(aMemory);
	aMemory->ClipRegion(aIndex,aCount);
	aMemory->iManager->Free(aMemory,aIndex,aCount);
	MemoryObjectLock::Unlock(aMemory);
	}


TInt MM::MemoryAddPages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, const TPhysAddr* aPages)
	{
	TRACE(("MM::MemoryAddPages(0x%08x,0x%08x,0x%08x,?)",aMemory,aIndex,aCount));
	MemoryObjectLock::Lock(aMemory);
	TInt r;
	if(!aMemory->CheckRegion(aIndex,aCount))
		r = KErrArgument;
	else
		r = aMemory->iManager->AddPages(aMemory,aIndex,aCount,aPages);
	MemoryObjectLock::Unlock(aMemory);
	TRACE(("MM::MemoryAddPages returns %d",r));
	return r;
	}


TInt MM::MemoryAddContiguous(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr aPhysAddr)
	{
	TRACE(("MM::MemoryAddContiguous(0x%08x,0x%08x,0x%08x,0x%08x)",aMemory,aIndex,aCount,aPhysAddr));
	MemoryObjectLock::Lock(aMemory);
	TInt r;
	if(!aMemory->CheckRegion(aIndex,aCount))
		r = KErrArgument;
	else
		r = aMemory->iManager->AddContiguous(aMemory,aIndex,aCount,aPhysAddr);
	MemoryObjectLock::Unlock(aMemory);
	TRACE(("MM::MemoryAddContiguous returns %d",r));
	return r;
	}


TUint MM::MemoryRemovePages(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr* aPages)
	{
	TRACE(("MM::MemoryRemovePages(0x%08x,0x%08x,0x%08x)",aMemory,aIndex,aCount));
	MemoryObjectLock::Lock(aMemory);
	aMemory->ClipRegion(aIndex,aCount);
	TInt r = aMemory->iManager->RemovePages(aMemory,aIndex,aCount,aPages);
	if(r<0)
		r = 0;
	MemoryObjectLock::Unlock(aMemory);
	TRACE(("MM::MemoryRemovePages returns %d",r));
	return r;
	}


TInt MM::MemoryAllowDiscard(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	TRACE(("MM::MemoryAllowDiscard(0x%08x,0x%08x,0x%08x)",aMemory,aIndex,aCount));
	MemoryObjectLock::Lock(aMemory);
	TInt r;
	if(!aMemory->CheckRegion(aIndex,aCount))
		r = KErrArgument;
	else
		r = aMemory->iManager->AllowDiscard(aMemory,aIndex,aCount);
	MemoryObjectLock::Unlock(aMemory);
	TRACE(("MM::MemoryAllowDiscard returns %d",r));
	return r;
	}


TInt MM::MemoryDisallowDiscard(DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	TRACE(("MM::MemoryDisallowDiscard(0x%08x,0x%08x,0x%08x)",aMemory,aIndex,aCount));
	MemoryObjectLock::Lock(aMemory);
	TInt r;
	if(!aMemory->CheckRegion(aIndex,aCount))
		r = KErrArgument;
	else
		r = aMemory->iManager->DisallowDiscard(aMemory,aIndex,aCount);
	MemoryObjectLock::Unlock(aMemory);
	TRACE(("MM::MemoryDisallowDiscard returns %d",r));
	return r;
	}


TInt MM::MemoryPhysAddr(DMemoryObject* aMemory, TUint aIndex, TUint aCount, TPhysAddr& aPhysicalAddress, TPhysAddr* aPhysicalPageList)
	{
	TRACE(("MM::MemoryPhysAddr(0x%08x,0x%08x,0x%08x,?,?)",aMemory,aIndex,aCount));
	TInt r = aMemory->PhysAddr(aIndex,aCount,aPhysicalAddress,aPhysicalPageList);
	TRACE(("MM::MemoryPhysAddr returns %d aPhysicalAddress=0x%08x",r,aPhysicalAddress));
	return r;
	}


void MM::MemoryBTracePrime(DMemoryObject* aMemory)
	{
	aMemory->BTraceCreate();
	aMemory->iMappings.Lock();
	TMappingListIter iter;
	DMemoryMapping* mapping = (DMemoryMapping*)iter.Start(aMemory->iMappings);
	while(mapping)
		{
		aMemory->iMappings.Unlock();	
		mapping->BTraceCreate();
		aMemory->iMappings.Lock();
		mapping = (DMemoryMapping*)iter.Next();
		}
	iter.Finish();
	aMemory->iMappings.Unlock();	
	}


void MM::MemoryClose(DMemoryObject* aMemory)
	{
	aMemory->Close();
	}


TBool MM::MemoryIsNotMapped(DMemoryObject* aMemory)
	{
	TBool r = aMemory->iMappings.IsEmpty();
	TRACE2(("MM::MemoryIsNotMapped(0x%08x) returns %d",aMemory,r));
	return r;
	}

//
// Physical pinning
//

TInt MM::PinPhysicalMemory(DMemoryObject* aMemory, DPhysicalPinMapping* aPinObject, TUint aIndex, TUint aCount, TBool aReadOnly, TPhysAddr& aAddress, TPhysAddr* aPages, TUint32& aMapAttr, TUint& aColour)
	{

	if (!aMemory->CheckRegion(aIndex,aCount))
	    return KErrArgument;

	TMappingPermissions permissions = aReadOnly ? ESupervisorReadOnly : ESupervisorReadWrite;
	TInt r = aPinObject->Pin(aMemory, aIndex, aCount, permissions);
	if (r == KErrNone)
		{
		r = aPinObject->PhysAddr(aIndex, aCount, aAddress, aPages);
		if (r>=KErrNone)
			{
			r = KErrNone; //Do not report discontigious memory in return value.
			const TMappingAttributes2& mapAttr2 =
				MM::LegacyMappingAttributes(aMemory->Attributes(), permissions);
			*(TMappingAttributes2*)&aMapAttr = mapAttr2;
			}
		else
			{
			aPinObject->Unpin();
			}
		}

	aColour = 0;
	return r;
	}


TInt MM::MemoryWipe(DMemoryObject* aMemory)
	{
	__NK_ASSERT_ALWAYS(aMemory->iMappings.IsEmpty()); // can't be mapped otherwise confidentiality can't be guaranteed
	TRACE2(("MM::MemoryWipe(0x%08x)",aMemory));
	MemoryObjectLock::Lock(aMemory);
	TInt r = aMemory->iManager->Wipe(aMemory);
	MemoryObjectLock::Unlock(aMemory);
	return r;
	}


TInt MM::MemorySetReadOnly(DMemoryObject* aMemory)
	{
	TRACE2(("MM::MemorySetReadOnly(0x%08x)",aMemory));
	MemoryObjectLock::Lock(aMemory);
	TInt r = aMemory->SetReadOnly();
	MemoryObjectLock::Unlock(aMemory);
	return r;
	}

//
// Mapping functions
//

TInt MM::MappingNew(DMemoryMapping*& aMapping, DMemoryObject* aMemory, TMappingPermissions aPermissions, TInt aOsAsid, TMappingCreateFlags aFlags, TLinAddr aAddr, TUint aIndex, TUint aCount)
	{
	TRACE(("MM::MappingNew(?,0x%08x,0x%08x,%d,0x%08x,0x%08x,0x%08x,0x%08x)",aMemory, aPermissions, aOsAsid, aFlags, aAddr, aIndex, aCount));

	/**
	@todo Make mappings created with this function fail (panic?) if the are reused to map
	another object.
	*/
	if(aCount==~0u)
		aCount = aMemory->iSizeInPages-aIndex;

	// if memory object reserves all resources, make mappings also do so...
	if(aMemory->iFlags&DMemoryObject::EReserveResources)
		FlagSet(aFlags,EMappingCreateReserveAllResources);

	// check if mapping is for global user data...
	if(aOsAsid==(TInt)KKernelOsAsid && aPermissions&EUser)
		FlagSet(aFlags,EMappingCreateUserGlobalVirtual);
	else
		FlagClear(aFlags,EMappingCreateUserGlobalVirtual);

	// set paged attribute for mapping...
	if(aMemory->IsDemandPaged())
		FlagSet(aFlags,EMappingCreateDemandPaged);
	else
		FlagClear(aFlags,EMappingCreateDemandPaged);

	DMemoryMapping* mapping = 0;
	TInt r = KErrNone;
	if(!aMemory->CheckRegion(aIndex,aCount))
		r = KErrArgument;
	else
		{
		mapping = aMemory->CreateMapping(aIndex, aCount);
		if(!mapping)
			r = KErrNoMemory;
		}

	if(!mapping)
		{
		// free any virtual address the mapping should have adopted...
		if(aFlags&EMappingCreateAdoptVirtual)
			MM::VirtualFree(aOsAsid, aAddr, aCount<<KPageShift);
		}
	else
		{
		r = mapping->Construct(aMemory->Attributes(), aFlags, aOsAsid, aAddr, aCount<<KPageShift, aIndex<<KPageShift);
		if(r==KErrNone)
			r = mapping->Map(aMemory, aIndex, aCount, aPermissions);
		if(r!=KErrNone)
			{
			mapping->Close();
			mapping = 0;
			}
		}

	aMapping = mapping;
	TRACE(("MM::MappingNew returns %d (aMapping=0x%0x)",r,aMapping));
#ifdef BTRACE_FLEXIBLE_MEM_MODEL
	if (r == KErrNone)
		aMapping->BTraceCreate();
#endif
	return r;
	}


TInt MM::MappingNew(DMemoryMapping*& aMapping, TUint aCount, TInt aOsAsid, TMappingCreateFlags aFlags, TLinAddr aAddr, TLinAddr aColourOffset)
	{
	TRACE2(("MM::MappingNew(?,0x%08x,%d,0x%08x,0x%08x,0x%08x)",aCount, aOsAsid, aFlags, aAddr, aColourOffset));

	FlagClear(aFlags,EMappingCreateDemandPaged); // mapping can't use demand paged page tables

	TInt r = KErrNone;
	DMemoryMapping* mapping = new DFineMapping();
	if(!mapping)
		r = KErrNoMemory;

	if(!mapping)
		{
		// free any virtual address the mapping should have adopted...
		if(aFlags&EMappingCreateAdoptVirtual)
			MM::VirtualFree(aOsAsid, aAddr, aCount<<KPageShift);
		}
	else
		{
		r = mapping->Construct(EMemoryAttributeStandard, aFlags, aOsAsid, aAddr, aCount<<KPageShift, aColourOffset);
		if(r!=KErrNone)
			{
			mapping->Close();
			mapping = 0;
			}
		}

	aMapping = mapping;
	TRACE2(("MM::MappingNew returns %d (aMapping=0x%0x)",r,aMapping));

	return r;
	}


TInt MM::MappingMap(DMemoryMapping* aMapping, TMappingPermissions aPermissions, DMemoryObject* aMemory, TUint aIndex, TUint aCount)
	{
	TRACE2(("MM::MappingMap(0x%08x,0x%08x,0x%08x,0x%x,0x%x)",aMapping,aPermissions,aMemory,aIndex,aCount));
	if(aCount==~0u)
		aCount = aMemory->iSizeInPages-aIndex;
	TInt r = aMapping->Map(aMemory, aIndex, aCount, aPermissions);
	TRACE2(("MM::MappingMap returns %d",r));
	return r;
	}


void MM::MappingUnmap(DMemoryMapping* aMapping)
	{
	if(aMapping->IsAttached())
		{
		TRACE2(("MM::MappingUnmap(0x%08x)",aMapping));
		aMapping->Unmap();
		}
	}


void MM::MappingDestroy(DMemoryMapping*& aMapping)
	{
	DMemoryMapping* mapping = (DMemoryMapping*)__e32_atomic_swp_ord_ptr(&aMapping, 0);
	if (!mapping)
		return;
	TRACE(("MM::MappingDestroy(0x%08x)",mapping));
#ifdef BTRACE_FLEXIBLE_MEM_MODEL
	BTraceContext4(BTrace::EFlexibleMemModel,BTrace::EMemoryMappingDestroy,mapping);
#endif
	if(mapping->IsAttached())
		mapping->Unmap();
	mapping->Close();
	}


void MM::MappingDestroy(TLinAddr aAddr, TInt aOsAsid)
	{
	DMemoryMapping* mapping = AddressSpace[aOsAsid]->GetMapping(aAddr);
	MM::MappingDestroy(mapping);
	}


void MM::MappingAndMemoryDestroy(DMemoryMapping*& aMapping)
	{
	DMemoryMapping* mapping = (DMemoryMapping*)__e32_atomic_swp_ord_ptr(&aMapping, 0);
	TRACE(("MM::MappingAndMemoryDestroy(0x%08x)",mapping));
	if (!mapping)
		return;
	DMemoryObject* memory = mapping->Memory(true); // safe because we assume owner hasn't unmapped mapping
	MM::MappingDestroy(mapping);
	MM::MemoryDestroy(memory);
	}


void MM::MappingAndMemoryDestroy(TLinAddr aAddr, TInt aOsAsid)
	{
	DMemoryMapping* mapping = AddressSpace[aOsAsid]->GetMapping(aAddr);
	MM::MappingAndMemoryDestroy(mapping);
	}


TLinAddr MM::MappingBase(DMemoryMapping* aMapping)
	{
	TLinAddr base = aMapping->Base();
	TRACE2(("MM::MappingBase(0x%08x) returns 0x%08x",aMapping,base));
	return base;
	}


TInt MM::MappingOsAsid(DMemoryMapping* aMapping)
	{
	return aMapping->OsAsid();
	}


DMemoryObject* MM::MappingGetAndOpenMemory(DMemoryMapping* aMapping)
	{
	MmuLock::Lock();
	DMemoryObject* memory = aMapping->Memory();
	if (memory)
		memory->Open();
	MmuLock::Unlock();
	TRACE2(("MM::MappingGetAndOpenMemory(0x%08x) returns 0x%08x",aMapping,memory));
	return memory;
	}


void MM::MappingClose(DMemoryMapping* aMapping)
	{
	TRACE2(("MM::MappingClose(0x%08x)",aMapping));
	aMapping->Close();
	}


DMemoryMapping* MM::FindMappingInThread(DMemModelThread* aThread, TLinAddr aAddr, TUint aSize, 
										TUint& aOffsetInMapping, TUint& aInstanceCount)
	{
	if(aAddr>=KGlobalMemoryBase)
		{
		// Address in global region, so look it up in kernel's address space...
		return FindMappingInAddressSpace(KKernelOsAsid, aAddr, aSize, aOffsetInMapping, aInstanceCount);
		}

	// Address in thread's process address space so open a reference to its os asid
	// so that it remains valid for FindMappingInAddressSpace() call.
	DMemModelProcess* process = (DMemModelProcess*)aThread->iOwningProcess;
	TInt osAsid = process->TryOpenOsAsid();
	if (osAsid < 0)
		{// The process no longer owns an address space so can't have any mappings.
		return NULL;
		}

	DMemoryMapping* r = FindMappingInAddressSpace(osAsid, aAddr, aSize, aOffsetInMapping, aInstanceCount);

	process->CloseOsAsid();
	return r;
	}


DMemoryMapping* MM::FindMappingInProcess(DMemModelProcess* aProcess, TLinAddr aAddr, TUint aSize, 
										 TUint& aOffsetInMapping, TUint& aInstanceCount)
	{
	if(aAddr>=KGlobalMemoryBase)
		{
		// Address in global region, so look it up in kernel's address space...
		return MM::FindMappingInAddressSpace(KKernelOsAsid, aAddr, aSize, aOffsetInMapping, aInstanceCount);
		}

	// Address in thread's process address space so open a reference to its os asid
	// so that it remains valid for FindMappingInAddressSpace() call.
	TInt osAsid = aProcess->TryOpenOsAsid();
	if (osAsid < 0)
		{// The process no longer owns an address space so can't have any mappings.
		return NULL;
		}

	DMemoryMapping* r = MM::FindMappingInAddressSpace(osAsid, aAddr, aSize, aOffsetInMapping, aInstanceCount);

	aProcess->CloseOsAsid();
	return r;
	}


DMemoryMapping* MM::FindMappingInAddressSpace(	TUint aOsAsid, TLinAddr aAddr, TUint aSize, 
												TUint& aOffsetInMapping, TUint& aInstanceCount)
	{
	return AddressSpace[aOsAsid]->FindMapping(aAddr, aSize, aOffsetInMapping, aInstanceCount);
	}



//
// Address space
//

TInt MM::AddressSpaceAlloc(TPhysAddr& aPageDirectory)
	{
	return DAddressSpace::New(aPageDirectory);
	}


void MM::AddressSpaceFree(TUint aOsAsid)
	{
	AddressSpace[aOsAsid]->Close();
	}


void MM::AsyncAddressSpaceFree(TUint aOsAsid)
	{
	AddressSpace[aOsAsid]->AsyncClose();
	}


TInt MM::VirtualAllocCommon(TLinAddr& aLinAddr, TUint aSize, TBool aDemandPaged)
	{
	TRACE(("MM::VirtualAllocCommon(?,0x%08x,%d)",aSize,aDemandPaged));
	TUint pdeType = aDemandPaged ? EVirtualSlabTypeDemandPaged : 0;
	TInt r = DAddressSpace::AllocateUserCommonVirtualMemory(aLinAddr, aSize, 0, aSize, pdeType);
	TRACE(("MM::VirtualAllocCommon returns %d region=0x%08x+0x%08x",r,aLinAddr,aSize));
	return r;
	}


void MM::VirtualFreeCommon(TLinAddr aLinAddr, TUint aSize)
	{
	TRACE(("MM::VirtualFreeCommon(0x%08x,0x%08x)",aLinAddr,aSize));
	DAddressSpace::FreeUserCommonVirtualMemory(aLinAddr, aSize);
	}


TInt MM::VirtualAlloc(TInt aOsAsid, TLinAddr& aLinAddr, TUint aSize, TBool aDemandPaged)
	{
	TRACE(("MM::VirtualAlloc(?,%d,0x%08x,%d)",aOsAsid,aSize,aDemandPaged));
	TUint pdeType = aDemandPaged ? EVirtualSlabTypeDemandPaged : 0;
	TInt r = AddressSpace[aOsAsid]->AllocateVirtualMemory(aLinAddr, aSize, 0, aSize, pdeType);
	TRACE(("MM::VirtualAlloc returns %d region=0x%08x+0x%08x",r,aLinAddr,aSize));
	return r;
	}


void MM::VirtualFree(TInt aOsAsid, TLinAddr aLinAddr, TUint aSize)
	{
	TRACE(("MM::VirtualFree(%d,0x%08x,0x%08x)",aOsAsid,aLinAddr,aSize));
	AddressSpace[aOsAsid]->FreeVirtualMemory(aLinAddr, aSize);
	}



//
// Init
//

void MM::Init1()
	{
	TheMmu.Init1();
	}


extern DMutexPool MemoryObjectMutexPool;
extern DMutexPool AddressSpaceMutexPool;

void MM::Init2()
	{
	TInt r;

	TheMmu.Init2();

	// create mutex pools before calling any functions which require them...
	_LIT(KAddressSpaceMutexName,"AddressSpaceMutex");
	r = AddressSpaceMutexPool.Create(4, &KAddressSpaceMutexName, KMutexOrdAddresSpace);
	__NK_ASSERT_ALWAYS(r==KErrNone);
	_LIT(KMemoryObjectMutexName,"MemoryObjectMutex");
	r = MemoryObjectMutexPool.Create(8, &KMemoryObjectMutexName, KMutexOrdMemoryObject);
	__NK_ASSERT_ALWAYS(r==KErrNone);

	// use the Ram Allocator mutex for low-level memory functions...
	DMutex* mmuAllocMutex = TheMmu.iRamAllocatorMutex;

	// memory cleanup needs initialising before any memory is freed...
	TMemoryCleanup::Init2();

	// initialise allocators used for MMU operations...
	RPageArray::Init2A();
	PageTables.Init2(mmuAllocMutex); // must come before any other code which allocates memory objects
	RPageArray::Init2B(mmuAllocMutex);
	PageTables.Init2B();
	PageDirectories.Init2();

	// initialise address spaces...
	DAddressSpace::Init2();

	TheMmu.Init2Final();
	}

 
/** HAL Function wrapper for the RAM allocator.
*/
TInt RamHalFunction(TAny*, TInt aFunction, TAny* a1, TAny* a2)
	{
	return TheMmu.RamHalFunction(aFunction, a1, a2);
	}


void MM::Init3()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("MM::Init3"));
	ThePager.Init3();

	// Register a HAL Function for the Ram allocator.
	TInt r = Kern::AddHalEntry(EHalGroupRam, RamHalFunction, 0);
	__NK_ASSERT_ALWAYS(r==KErrNone);

	TheMmu.Init3();
	}


TInt MM::InitFixedKernelMemory(DMemoryObject*& aMemory,
							   TLinAddr aStart,
							   TLinAddr aEnd,
							   TUint aInitSize,
							   TMemoryObjectType aType,
							   TMemoryCreateFlags aMemoryCreateFlags,
							   TMemoryAttributes aMemoryAttributes,
							   TMappingCreateFlags aMappingCreateFlags
							   )
	{
	TUint maxSize = aEnd-aStart;
	TInt r = MM::MemoryNew(aMemory, aType, MM::BytesToPages(maxSize), aMemoryCreateFlags, aMemoryAttributes);
	if(r==KErrNone)
		{
		TBool allowGaps = aInitSize&1; // lower bit of size is set if region to be claimed contains gaps
		aInitSize &= ~1;
		r = MM::MemoryClaimInitialPages(aMemory,aStart,aInitSize,ESupervisorReadWrite,allowGaps);
		if(r==KErrNone)
			{
			DMemoryMapping* mapping;
			r = MM::MappingNew(mapping,aMemory,ESupervisorReadWrite,KKernelOsAsid,aMappingCreateFlags,aStart);
			// prevent any further mappings of this memory,
			// this is needed for realtime and OOM guarantees...
			aMemory->DenyMappings();
			}
		}
	// Note, no cleanup is done if an error occurs because this function is only
	// used at boot time and the system can't recover from an error
	return r;
	}


void MM::Panic(MM::TMemModelPanic aPanic)
	{
	Kern::Fault("MemModel", aPanic);
	}


//
//
//

TUint MM::BytesToPages(TUint aBytes)
	{
	if(aBytes&KPageMask)
		Panic(EBadBytesToPages);
	return aBytes>>KPageShift;
	}


TUint MM::RoundToPageSize(TUint aSize)
	{
	return (aSize+KPageMask)&~KPageMask;
	}


TUint MM::RoundToPageCount(TUint aSize)
	{
	return (aSize+KPageMask)>>KPageShift;
	}


TUint MM::RoundToPageShift(TUint aShift)
	{
	return aShift>(TUint)KPageShift ? aShift-KPageShift : 0;
	}


//
//
//

void MM::ValidateLocalIpcAddress(TLinAddr aAddr, TUint aSize, TBool aWrite)
	{
	__NK_ASSERT_DEBUG(aSize);

	TLinAddr end = aAddr+aSize-1;
	if(end<aAddr)
		end = ~(TLinAddr)0; // clip to end of memory

	// if IPC region is in process local data area then it's OK...
	if(end<KUserLocalDataEnd && aAddr>=KUserLocalDataBase)
		return;

	// if region overlaps alias region...
	if(end>=KIPCAlias && aAddr<KIPCAlias+KIPCAliasAreaSize)
		{
		// remove alias...
		((DMemModelThread*)TheCurrentThread)->RemoveAlias();
		// make sure start address is in alias region...
		if(aAddr<KIPCAlias)
			aAddr = KIPCAlias;
		// then cause fault now...
		MM::UserPermissionFault(aAddr,aWrite);
		}

	if(end<(TLinAddr)KUserMemoryLimit)
		return; // user memory is safe
	
	// Compare the current thread's process os asid to kernel asid, no need to 
	// open a reference on the os asid as it is the current thread.
	if(((DMemModelProcess*)TheCurrentThread->iOwningProcess)->OsAsid()==(TInt)KKernelOsAsid)
		return; // kernel can access everything

	// make sure address is in supervisor only region...
	if(aAddr<KUserMemoryLimit)
		aAddr = KUserMemoryLimit;
	// then cause fault now...
	MM::UserPermissionFault(aAddr,aWrite);
	}


void MM::UserPermissionFault(TLinAddr aAddr, TBool aWrite)
	{
	// Access aAddr with user permissions to generate an exception...
	if(aWrite)
		UserWriteFault(aAddr);
	else
		UserReadFault(aAddr);
	__NK_ASSERT_ALWAYS(0); // shouldn't get here
	}


#ifndef __SMP__
void MM::IpcAliasPde(TPde*& aPdePtr, TUint aOsAsid)
	{
	aPdePtr = &Mmu::PageDirectory(aOsAsid)[KIPCAlias>>KChunkShift];
	}
#endif


TMappingPermissions MM::MappingPermissions(TBool aUser, TBool aWrite, TBool aExecute)
	{
	TUint perm	= 0;
	if(aUser)
		perm |= EUser;
	if(aWrite)
		perm |= EReadWrite;
	if(aExecute)
		perm |= EExecute;
	return (TMappingPermissions)perm;
	}


TInt MM::MappingPermissions(TMappingPermissions& aPermissions, TMappingAttributes2 aLegacyAttributes)
	{
	TUint attr2 = *(TUint32*)&aLegacyAttributes;

	TUint read = attr2&EMapAttrReadMask;
	TUint write = (attr2&EMapAttrWriteMask)>>4;
	TUint execute = (attr2&EMapAttrExecMask)>>8;

	read |= execute; 	// execute access requires read access

	if(write==0) 		// no write required
		{
		if((read&5)==0)
			return KErrNotSupported; // neither supervisor nor user read specified
		}
	else if(write<4)	// supervisor write required
		{
		if(read>=4)
			return KErrNotSupported; // user read requested (but no user write)
		}

	read |= write;		// write access implies read access

	TUint user = read&4;
	aPermissions = MappingPermissions(user,write,execute);

	return KErrNone;
	}


TInt MM::MemoryAttributes(TMemoryAttributes& aAttributes, TMappingAttributes2 aLegacyAttributes)
	{
	TUint attr = aLegacyAttributes.Type();
	if (aLegacyAttributes.Shared())
		attr |= EMemoryAttributeShareable;
	if (aLegacyAttributes.Parity())
		attr |= EMemoryAttributeUseECC;
	aAttributes = Mmu::CanonicalMemoryAttributes((TMemoryAttributes)attr);
	return KErrNone;
	}


TMappingAttributes2 MM::LegacyMappingAttributes(TMemoryAttributes aAttributes, TMappingPermissions aPermissions)
	{
	TUint attr = Mmu::CanonicalMemoryAttributes(aAttributes);
	return TMappingAttributes2
		(
		(TMemoryType)(attr&EMemoryAttributeTypeMask),
		aPermissions&EUser,
		aPermissions&EReadWrite,
		aPermissions&EExecute,
		attr&EMemoryAttributeShareable,
		attr&EMemoryAttributeUseECC
		);
	}
