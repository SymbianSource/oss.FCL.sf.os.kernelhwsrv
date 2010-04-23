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

#include <plat_priv.h>
#include "mm.h"
#include "mmu.h"

#include "maddressspace.h"
#include "mpdalloc.h"
#include "mmapping.h"



/**
Allocator for OS Address Space IDs (OS ASIDs).
This is a simple bitmap allocator for KNumOsAsids integers with an
associated mutex to guard against concurrency when allocating and
freeing.
*/
class OsAsidAllocator
	{
public:
	void Init2()
		{
		iAllocator = TBitMapAllocator::New(KNumOsAsids,ETrue);
		__NK_ASSERT_ALWAYS(iAllocator);
		iAllocator->Alloc(KKernelOsAsid,1); // make kernel OS ASID already allocated
		}

	TInt Alloc()
		{
		NKern::FMWait(&iLock);
		TInt osAsid = iAllocator->Alloc();
		NKern::FMSignal(&iLock);
		if(osAsid<0)
			return KErrNoMemory;
		return osAsid;
		}

	void Free(TInt aOsAsid)
		{
		NKern::FMWait(&iLock);
		iAllocator->Free(aOsAsid);
		NKern::FMSignal(&iLock);
		}

private:
	TBitMapAllocator* iAllocator;
	NFastMutex iLock;
	}
OsAsidAllocator;


//
// DAddressSpace
//

DAddressSpace KernelAddressSpace;	///< The kernel's address space object.

__ASSERT_COMPILE(KKernelOsAsid==0);
DAddressSpace* AddressSpace[KNumOsAsids] = { &KernelAddressSpace };

RVirtualAllocator DAddressSpace::UserGlobalVirtualAllocator;
RBackwardsVirtualAllocator DAddressSpace::UserCommonVirtualAllocator;

/**
The read lock used for protecting the mappings container in address spaces (DAddressSpace::iMappings).
A single global lock is used for all processes - this isn't required but it is the simplest
implementation if we want to avoid the memory overhead of allocating a mutex per address space.
*/
NFastMutex TheAddressSpaceMappingLock;


/**
A pool of mutexes which are used to protect an address space's virtual address allocation
and acts as a write lock for the mappings container (DAddressSpace::iMappings).
*/
DMutexPool AddressSpaceMutexPool;


void DAddressSpace::Init2()
	{
	// create allocator for ASIDs...
	OsAsidAllocator.Init2();

	// construct the kernel's address space...
	TInt r = KernelAddressSpace.Construct(0, KKernelSectionBase, KKernelSectionEnd);
	__NK_ASSERT_ALWAYS(r==KErrNone);

	// mark primary i/o region as already allocated...
	__ASSERT_COMPILE(((KPrimaryIOBase|KPrimaryIOEnd)&KChunkMask)==0); // region must be chunk aligned to avoid PDE type conflicts with any new allocations
	TLinAddr addr;
	TUint size;
	r = KernelAddressSpace.AllocateVirtualMemory(addr,size,KPrimaryIOBase,KPrimaryIOEnd-KPrimaryIOBase,0);
	__NK_ASSERT_ALWAYS(r==KErrNone);

	// construct user global memory allocator...
	r = UserGlobalVirtualAllocator.Construct(KGlobalMemoryBase,KUserMemoryLimit,ENumVirtualAllocTypes,AddressSpace[KKernelOsAsid]->iLock);
	__NK_ASSERT_ALWAYS(r==KErrNone);

	// construct user common memory allocator (two slab types, one each for paged and unpaged memory)...
	r = UserCommonVirtualAllocator.Construct(KUserLocalDataBase,KUserLocalDataEnd,ENumVirtualAllocTypes,AddressSpace[KKernelOsAsid]->iLock);
	__NK_ASSERT_ALWAYS(r==KErrNone);

	// reserve virtual memory for XIP user code...
	TUint romDataSize = TheRomHeader().iTotalUserDataSize;
	TLinAddr romDataBase = TheRomHeader().iUserDataAddress-romDataSize;
	__NK_ASSERT_DEBUG(TheRomHeader().iUserDataAddress==KUserLocalDataEnd);
	if(romDataSize)
		{
		r = UserCommonVirtualAllocator.Alloc(addr,size,romDataBase,romDataSize,0);
		__NK_ASSERT_ALWAYS(r==KErrNone);
		}
	}


DAddressSpace::DAddressSpace()
	: iMappings(&TheAddressSpaceMappingLock,iLock)
	{
	}


TInt DAddressSpace::New(TPhysAddr& aPageDirectory)
	{
	TRACE(("DAddressSpace::New(?)"));
	TInt r;
	TInt osAsid = OsAsidAllocator.Alloc();
	if(osAsid<0)
		r = KErrNoMemory;
	else
		{
		r = PageDirectories.Alloc(osAsid,aPageDirectory);
		if(r!=KErrNone)
			OsAsidAllocator.Free(osAsid);
		else
			{
			DAddressSpace*& info = AddressSpace[osAsid];
			__NK_ASSERT_DEBUG(!info);
			info = new DAddressSpace();
			if(!info)
				{
				PageDirectories.Free(osAsid);
				OsAsidAllocator.Free(osAsid);
				r = KErrNoMemory;
				}
			else
				{
				r = info->Construct(osAsid,KUserLocalDataBase,KUserLocalDataEnd);
				if(r!=KErrNone)
					{
					info->Close();
					info = 0;
					}
				}
			}
		}

	if(r==KErrNone)
		r = osAsid;
	else
		aPageDirectory = KPhysAddrInvalid;

	TRACE(("DAddressSpace::New returns %d",r));
	return r;
	}



DAddressSpace::~DAddressSpace()
	{
	TRACE(("DAddressSpace[0x%08x]::~DAddressSpace() osAsid = %d",this,iOsAsid));
#ifdef _DEBUG
	if(iMappings.Count())
		Dump();
#endif
	__NK_ASSERT_DEBUG(iMappings.Count()==0);

	TInt osAsid = iOsAsid;
	AddressSpace[osAsid] = 0;
	PageDirectories.Free(osAsid);
	InvalidateTLBForAsid(osAsid);
	OsAsidAllocator.Free(osAsid);
	}


TInt DAddressSpace::Construct(TInt aOsAsid, TLinAddr aStart, TLinAddr aEnd)
	{
	TRACE(("DAddressSpace::Construct(%d,0x%08x,0x%08x)",aOsAsid,aStart,aEnd));
	iOsAsid = aOsAsid;
	return iVirtualAllocator.Construct(aStart,aEnd,ENumVirtualAllocTypes,iLock);
	}


void DAddressSpace::Lock()
	{
	AddressSpaceMutexPool.Wait(iLock);
	}


void DAddressSpace::Unlock()
	{
	AddressSpaceMutexPool.Signal(iLock);
	}


TInt DAddressSpace::AllocateVirtualMemory(TLinAddr& aAddr, TUint& aSize, TLinAddr aRequestedAddr, TUint aRequestedSize, TUint aPdeType)
	{
	TRACE(("DAddressSpace::AllocateVirtualMemory(?,?,0x%08x,0x%08x,%d) osAsid=%d",aRequestedAddr,aRequestedSize,aPdeType,iOsAsid));
	__NK_ASSERT_DEBUG(aPdeType<ENumVirtualAllocTypes);
	Lock();
	TInt r = iVirtualAllocator.Alloc(aAddr,aSize,aRequestedAddr,aRequestedSize,aPdeType);
	if(r==KErrNone)
		Open();
	Unlock();
	TRACE(("DAddressSpace::AllocateVirtualMemory returns %d region=0x%08x+0x%08x",r,aAddr,aSize));
	return r;
	}


TInt DAddressSpace::AllocateUserGlobalVirtualMemory(TLinAddr& aAddr, TUint& aSize, TLinAddr aRequestedAddr, TUint aRequestedSize, TUint aPdeType)
	{
	TRACE(("DAddressSpace::AllocateUserGlobalVirtualMemory(?,?,0x%08x,0x%08x,%d)",aRequestedAddr,aRequestedSize,aPdeType));
	__NK_ASSERT_DEBUG(aPdeType<ENumVirtualAllocTypes);
	KernelAddressSpace.Lock();
	TInt r = UserGlobalVirtualAllocator.Alloc(aAddr,aSize,aRequestedAddr,aRequestedSize,aPdeType);
	KernelAddressSpace.Unlock();
	TRACE(("DAddressSpace::AllocateUserGlobalVirtualMemory returns %d region=0x%08x+0x%08x",r,aAddr,aSize));
	return r;
	}


void DAddressSpace::FreeVirtualMemory(TLinAddr aAddr, TUint aSize)
	{
	TRACE(("DAddressSpace::FreeVirtualMemory(0x%08x,0x%08x) osAsid=%d",aAddr, aSize, iOsAsid));
	Lock();
	TBool global = iOsAsid==(TInt)KKernelOsAsid && UserGlobalVirtualAllocator.InRange(aAddr,aSize);
	if(global)
		UserGlobalVirtualAllocator.Free(aAddr,aSize);
	else
		iVirtualAllocator.Free(aAddr,aSize);
	Unlock();
	if (!global)
		AsyncClose();
	}


TInt DAddressSpace::AllocateUserCommonVirtualMemory(TLinAddr& aAddr, TUint& aSize, TLinAddr aRequestedAddr, TUint aRequestedSize, TUint aPdeType)
	{
	TRACE(("DAddressSpace::AllocateUserCommonVirtualMemory(?,?,0x%08x,0x%08x,%d)",aRequestedAddr,aRequestedSize,aPdeType));
	__NK_ASSERT_DEBUG(aPdeType<ENumVirtualAllocTypes);
	KernelAddressSpace.Lock();
	TInt r = UserCommonVirtualAllocator.Alloc(aAddr,aSize,aRequestedAddr,aRequestedSize,aPdeType);
	KernelAddressSpace.Unlock();
	TRACE(("DAddressSpace::AllocateUserCommonVirtualMemory returns %d region=0x%08x+0x%08x",r,aAddr,aSize));
	return r;
	}


void DAddressSpace::FreeUserCommonVirtualMemory(TLinAddr aAddr, TUint aSize)
	{
	TRACE(("DAddressSpace::FreeUserCommonVirtualMemory(0x%08x,0x%08x)",aAddr,aSize));
	KernelAddressSpace.Lock();
	UserCommonVirtualAllocator.Free(aAddr,aSize);
	KernelAddressSpace.Unlock();
	}


TInt DAddressSpace::AddMapping(TLinAddr aAddr, DMemoryMapping* aMapping)
	{
	Lock();
	TRACE(("DAddressSpace::AddMapping(0x%08x,0x%08x) osAsid=%d",aAddr, aMapping, iOsAsid));
	TInt r = iMappings.Add(aAddr,aMapping);
	TRACE(("DAddressSpace::AddMapping osAsid=%d returns %d",iOsAsid, r));
	Unlock();
	return r;
	}


DMemoryMapping* DAddressSpace::RemoveMapping(TLinAddr aAddr)
	{
	Lock();
	DMemoryMapping* removed = (DMemoryMapping*)iMappings.Remove(aAddr);
	TRACE(("DAddressSpace::RemoveMapping(0x%08x) osAsid=%d returns 0x%08x",aAddr, iOsAsid, removed));
	Unlock();
	return removed;
	}


DMemoryMapping* DAddressSpace::GetMapping(TLinAddr aAddr)
	{
	iMappings.ReadLock();
	DMemoryMapping* mapping = (DMemoryMapping*)iMappings.Find(aAddr);
	TRACE(("DAddressSpace::GetMapping(0x%08x) osAsid=%d returns 0x%08x",aAddr, iOsAsid, mapping));
	__NK_ASSERT_DEBUG(mapping); // caller must know there is a mapping
	iMappings.ReadUnlock();
	return mapping;
	}


DMemoryMapping* DAddressSpace::FindMapping(TLinAddr aAddr, TUint aSize, TUint& aOffsetInMapping, TUint& aInstanceCount)
	{
	__ASSERT_CRITICAL;

	DMemoryMapping* result = NULL;

	// find mapping...
	iMappings.ReadLock();
	TUint dummy;
	DMemoryMapping* mapping = (DMemoryMapping*)iMappings.Find(aAddr,dummy);
	if(mapping && mapping->IsAttached())
		{
		// found mapping, check addresses are in range...
		TUint offset = aAddr-mapping->Base();
		TUint end = offset+aSize;
		if(offset<end && end<=mapping->iSizeInPages<<KPageShift)
			{
			// addresses OK, get a reference on the mapping before releasing list lock...
			aOffsetInMapping = offset;
			aInstanceCount = mapping->MapInstanceCount();
			mapping->Open(); // can't fail because mapping IsAttached
			result = mapping;
			}
		}
	iMappings.ReadUnlock();

	return result;
	}


TBool DAddressSpace::CheckPdeType(TLinAddr aAddr, TUint aSize, TUint aPdeType)
	{
	TRACE(("DAddressSpace::CheckPdeType(0x%08x,0x%08x,%d) osAsid=%d",aAddr, aSize, aPdeType, iOsAsid));
	TBool r;
	Lock();
	if(iOsAsid==(TInt)KKernelOsAsid && UserGlobalVirtualAllocator.InRange(aAddr,aSize))
		r = UserGlobalVirtualAllocator.CheckSlabType(aAddr,aSize,aPdeType);
	else
		r = iVirtualAllocator.CheckSlabType(aAddr,aSize,aPdeType);
	TRACE(("DAddressSpace::CheckPdeType returns %d",r));
	Unlock();
	return r;
	}



//
// Debug
//

#ifdef _DEBUG

void DAddressSpace::Dump()
	{
	Kern::Printf("DAddressSpace[0x%08x]::Dump() osAsid = %d",this,iOsAsid);
	TLinAddr virt = 0;
	do
		{
		--virt;
		iMappings.ReadLock();
		TUint offsetInMapping = 0;
		DMemoryMapping* mapping = (DMemoryMapping*)iMappings.Find(virt,offsetInMapping);
		if(mapping)
			{
			if(!mapping->TryOpen())
				mapping = NULL;
			virt -= offsetInMapping;
			}
		iMappings.ReadUnlock();
		if(!mapping)
			break;
		mapping->Dump();
		mapping->Close();
		}
	while(virt);
	}

#endif // _DEBUG
