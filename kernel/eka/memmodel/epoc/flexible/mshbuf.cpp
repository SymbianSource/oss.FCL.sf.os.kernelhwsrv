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
// e32/memmodel/epoc/flexible/mshbuf.cpp
// Shareable Data Buffers

#include <memmodel.h>
#include "mmu/mm.h"
#include "mmboot.h"
#include <kernel/smap.h>

_LIT(KLitDMemModelAlignedShPool,"DMMAlignedShPool");	// Must be no more than 16 characters!

struct TWait
	{
	void Link(TWait*& aList)
		{
		iSem.SetOwner(NULL);
		iNext = aList;
		aList = this;
		};
	void Wait()
		{
		NKern::FSWait(&iSem);
		}
	NFastSemaphore iSem;
	TWait* iNext;

	static void SignalAll(TWait* aList)
		{
		while (aList)
			{
			TWait* next = aList->iNext;
			NKern::FSSignal(&aList->iSem);
			aList = next;
			}
		}
	};


class DShBufMapping : public DBase
	{
public:
	SDblQueLink iObjLink;
	DMemoryMapping* iMapping;
	TInt iOsAsid;
	TWait* iTransitions; // Mapping and Unmapping operations
	TBool iTransitioning;
	};


DMemModelShPool::DMemModelShPool() : DShPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelShPool::DMemModelShPool"));
	}

DMemModelShPool::~DMemModelShPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelShPool::~DMemModelShPool"));
	}

void DMemModelShPool::DestroyClientResources(DProcess* aProcess)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelShPool::DestroyClientResources"));

	TInt r = DestroyAllMappingsAndReservedHandles(aProcess);
	__NK_ASSERT_DEBUG((r == KErrNone) || (r == KErrDied));
	(void)r;		// Silence warnings
	}

DMemModelAlignedShBuf::DMemModelAlignedShBuf(DShPool* aPool) : DShBuf(aPool)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShBuf::DMemModelAlignedShBuf()"));
	}

TInt DMemModelAlignedShBuf::Construct()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShBuf::Construct()"));

	TInt r = KErrNone;

	r = DShBuf::Construct();

	if (r == KErrNone)
		r = Create();

	return r;
	}

TInt DMemModelAlignedShBuf::Close(TAny* aPtr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShBuf::Close(0x%08x)", aPtr));

	if (aPtr)
		{
		DProcess* pP = reinterpret_cast<DProcess*>(aPtr);
		UnMap(pP);
		iPool->CloseClient(pP);
		}

	return DShBuf::Close(aPtr);
	}

TInt DMemModelAlignedShBuf::AddToProcess(DProcess* aProcess, TUint aAttr)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Adding DMemModelShBuf %O to process %O",this,aProcess));
	TInt r;
	TLinAddr base;
	TUint flags;

	r = iPool->OpenClient(aProcess, flags);

	if (r == KErrNone)
		{
		if ((flags & EShPoolAutoMapBuf) && ((aAttr & EShPoolNoMapBuf) == 0))
			{
			// note we use the client's pool flags and not the buffer attributes
			r = Map(flags, aProcess, base);

			if (aProcess == K::TheKernelProcess)
				iRelAddress = static_cast<TLinAddr>(base);
			}
		}

	return r;
	}

TInt DMemModelAlignedShBuf::Create()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShBuf::Create()"));
	TInt r = KErrNone;

	// calculate memory type...
	TMemoryObjectType memoryType =  EMemoryObjectUnpaged;

	TMemoryAttributes attr = EMemoryAttributeStandard;

	// calculate memory flags...
	TMemoryCreateFlags flags = static_cast<TMemoryCreateFlags>((EMemoryCreateDefault|EMemoryCreateUseCustomWipeByte|(0xAA<<EMemoryCreateWipeByteShift)));

	// note that any guard pages will be included in iBufGap, however the amount of memory committed
	// will be iBufSize rounded up to a page
	r = MM::MemoryNew(iMemoryObject, memoryType, MM::RoundToPageCount(iPool->iBufGap), flags, attr);

	if(r!=KErrNone)
		return r;

	if (iPool->iPoolFlags & EShPoolContiguous)
		{
		TPhysAddr paddr;
		r = MM::MemoryAllocContiguous(iMemoryObject, 0, MM::RoundToPageCount(iPool->iBufSize), 0, paddr);
		}
	else
		{
		r = MM::MemoryAlloc(iMemoryObject, 0, MM::RoundToPageCount(iPool->iBufSize));
		}

	return r;
	}

DMemModelAlignedShBuf::~DMemModelAlignedShBuf()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShBuf::~DMemModelAlignedShBuf()"));

	__NK_ASSERT_DEBUG(iMappings.IsEmpty());

	MM::MemoryDestroy(iMemoryObject);
	}

TInt DMemModelAlignedShBuf::Map(TUint aMapAttr, DProcess* aProcess, TLinAddr& aBase)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShBuf::Map()"));
	TInt r = KErrNone;

	DShBufMapping* m = NULL;
	DMemoryMapping* mapping = NULL;
	DMemModelProcess* pP = reinterpret_cast<DMemModelProcess*>(aProcess);

	TBool write = (TBool)EFalse;

	// User = ETrue, ReadOnlyWrite = ETrue, Execute = EFalse
	if (aMapAttr & EShPoolWriteable)
		write = (TBool)ETrue;

	TMappingPermissions perm = MM::MappingPermissions(pP!=K::TheKernelProcess, write, (TBool)EFalse);
	TWait wait;

	for(;;)
		{
		iPool->LockPool();
		r = FindMapping(m, pP);

		if (r != KErrNone)
			break;
		
		if (m->iTransitioning)
			{
			wait.Link(m->iTransitions);
			iPool->UnlockPool();
			wait.Wait();
			}
		else
			{
			iPool->UnlockPool();
			return KErrAlreadyExists;
			}
		}

	DMemModelAlignedShPoolClient* client = reinterpret_cast<DMemModelAlignedShPoolClient*>(iPool->iClientMap->Find(reinterpret_cast<TUint>(aProcess)));

	__NK_ASSERT_DEBUG(client);

	DMemModelAlignedShPool* pool = reinterpret_cast<DMemModelAlignedShPool*>(iPool);

	__NK_ASSERT_DEBUG(m == NULL);
	r = pool->GetFreeMapping(m, client);

	if (r == KErrNone)
		{
		iMappings.AddHead(&m->iObjLink);
		m->iTransitioning = ETrue;

		mapping = m->iMapping;
		iPool->UnlockPool(); // have to release fast lock for MappingMap

		r = MM::MappingMap(mapping, perm, iMemoryObject, 0, MM::RoundToPageCount(pool->iBufSize));

		iPool->LockPool();

		TWait* list = m->iTransitions;
		m->iTransitions = NULL;

		if (r != KErrNone)
		    pool->ReleaseMapping(m, client);
		else
		    aBase = MM::MappingBase(mapping);

		m->iTransitioning = EFalse;
		iPool->UnlockPool();

		TWait::SignalAll(list);
		}
	else
		iPool->UnlockPool();

	return r;
	}

TInt DMemModelAlignedShBuf::FindMapping(DShBufMapping*& aMapping, DMemModelProcess* aProcess)
	{
	// Must be in critical section so we don't leak os asid references.
	__ASSERT_CRITICAL;
	__NK_ASSERT_DEBUG(iPool->iLock.HeldByCurrentThread());

	TInt r = KErrNotFound;
	aMapping = NULL;

	// Open a reference on aProcess's os asid so that it can't be freed and 
	// reused while searching.
	TInt osAsid = aProcess->TryOpenOsAsid();
	if (osAsid < 0)
		{// aProcess has died and freed its os asid.
		return KErrDied;
		}

	SDblQueLink* pLink = iMappings.First();
	SDblQueLink* end = reinterpret_cast<SDblQueLink*>(&iMappings);
	DShBufMapping* m = NULL;

	while (pLink != end)
		{
		m = _LOFF(pLink, DShBufMapping, iObjLink);

		if (m->iOsAsid == osAsid)
			{
			aMapping = m;
			r = KErrNone;
			break;
			}
		pLink = pLink->iNext;
		}

	// Close the reference on the os asid as if we have a mapping then its lifetime will 
	// determine whether the process still owns an os asid.
	aProcess->CloseOsAsid();	
	return r;
	}

TInt DMemModelAlignedShBuf::UnMap(DProcess* aProcess)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShBuf::UnMap()"));

	TInt r = KErrNone;

	DMemModelProcess* pP = reinterpret_cast<DMemModelProcess*>(aProcess);

	DShBufMapping* m = NULL;
	TWait wait;

	for(;;)
		{
		iPool->LockPool();
		r = FindMapping(m, pP);

		if (r != KErrNone)
			{
			iPool->UnlockPool();
			return KErrNotFound;
			}

		if (m->iTransitioning)
			{
			wait.Link(m->iTransitions);
			iPool->UnlockPool();
			wait.Wait();
			}
		else
			{
			break;
			}
		}

	m->iTransitioning = ETrue;
	iPool->UnlockPool();

	MM::MappingUnmap(m->iMapping);

	iPool->LockPool();
	DMemModelAlignedShPoolClient* client = reinterpret_cast<DMemModelAlignedShPoolClient*>(iPool->iClientMap->Find(reinterpret_cast<TUint>(aProcess)));

	__NK_ASSERT_DEBUG(client);

	TWait* list = m->iTransitions;
	m->iTransitions = NULL;
	m->iObjLink.Deque();
	m->iTransitioning = EFalse;

	DMemModelAlignedShPool* pool = reinterpret_cast<DMemModelAlignedShPool*>(iPool);
	pool->ReleaseMapping(m, client);

	if (aProcess == K::TheKernelProcess)
	    iRelAddress = NULL;

	iPool->UnlockPool();

	wait.SignalAll(list);
	return KErrNone;
	}

TUint8* DMemModelAlignedShBuf::Base(DProcess* aProcess)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShBuf::Base()"));
	DMemModelProcess* pP = reinterpret_cast<DMemModelProcess*>(aProcess);

	DShBufMapping* mapping = NULL;
	iPool->LockPool();
	TInt r = FindMapping(mapping, pP);
	TUint8* base = NULL;

	if (r == KErrNone)
		base = reinterpret_cast<TUint8*>(MM::MappingBase(mapping->iMapping));
	iPool->UnlockPool();

	return base;
	}

TUint8* DMemModelAlignedShBuf::Base()
	{
	return reinterpret_cast<TUint8*>(iRelAddress);
	}

TInt DMemModelAlignedShBuf::Pin(TPhysicalPinObject* aPinObject, TBool aReadOnly, TPhysAddr& aAddress, TPhysAddr* aPages, TUint32& aMapAttr, TUint& aColour)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DMemModelAlignedShBuf::Pin");

	TInt r = MM::PinPhysicalMemory(iMemoryObject, (DPhysicalPinMapping*)aPinObject, 0,
								   MM::RoundToPageCount(Size()),
								   aReadOnly, aAddress, aPages, aMapAttr, aColour);

	return r;
	}

TInt DMemModelAlignedShPool::GetFreeMapping(DShBufMapping*& aMapping, DMemModelAlignedShPoolClient* aClient)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShPool::GetFreeMapping()"));
	__NK_ASSERT_DEBUG(iLock.HeldByCurrentThread());

	TInt r = KErrNotFound;
	aMapping = NULL;

	if (aClient)
		{
		if (!aClient->iMappingFreeList.IsEmpty())
			{
			aMapping = _LOFF(aClient->iMappingFreeList.GetFirst(), DShBufMapping, iObjLink);
			r = KErrNone;
			}
		else
			{
			r = KErrNoMemory;
			}
		}

	__KTRACE_OPT(KMMU2, Kern::Printf("DMemModelAlignedShPool::GetFreeMapping(0x%08x, 0x%08x) returns %d", aMapping, aClient, r));
	return r;
	}

TInt DMemModelAlignedShPool::ReleaseMapping(DShBufMapping*& aMapping, DMemModelAlignedShPoolClient* aClient)
	{
	__KTRACE_OPT(KMMU2, Kern::Printf("DMemModelAlignedShPool::ReleaseMapping(0x%08x,0x%08x)",aMapping,aClient));
	__NK_ASSERT_DEBUG(iLock.HeldByCurrentThread());

	TInt r = KErrNone;

	if (aClient)
		{
		aClient->iMappingFreeList.AddHead(&aMapping->iObjLink);
		aMapping = NULL;
		}
	else
		{
		// pool has probably been closed delete mapping
		r = KErrNotFound;
		__KTRACE_OPT(KMMU2, Kern::Printf("DMemModelAlignedShPool::ReleaseMapping delete 0x%08x",aMapping));
		UnlockPool(); // have to release fast lock for MappingDestroy
		MM::MappingDestroy(aMapping->iMapping);
		delete aMapping;
		aMapping = NULL;
		LockPool();
		}

	return r;
	}

TInt DMemModelAlignedShPool::SetBufferWindow(DProcess* aProcess, TInt aWindowSize)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShPool::SetBufferWindow()"));

	// Create and construct mappings but do not map
	// also allocate reserved handles
	TInt r = KErrNone;
	TUint noOfBuffers = aWindowSize;

	if (aWindowSize > static_cast<TInt>(iMaxBuffers))
		return KErrArgument;

	Kern::MutexWait(*iProcessLock);

	LockPool();
	DMemModelAlignedShPoolClient* client = reinterpret_cast<DMemModelAlignedShPoolClient*>(iClientMap->Find(reinterpret_cast<TUint>(aProcess)));
	UnlockPool();

	if (client)
		{
		if (client->iWindowSize != 0)
			{
			Kern::MutexSignal(*iProcessLock);
			return KErrAlreadyExists;
			}

		if (aWindowSize < 0)
			{
			noOfBuffers = iTotalBuffers;
			}

		DMemModelProcess* pP = reinterpret_cast<DMemModelProcess*>(aProcess);
		r = CreateMappings(client, noOfBuffers, pP);

		if (r == KErrNone)
			{
			client->iWindowSize = aWindowSize;
			}
		else
			{
			DestroyMappings(client, noOfBuffers);
			}
		}
	else
		{
		r = KErrNotFound;
		}

	Kern::MutexSignal(*iProcessLock);

	return r;
	}

TInt DMemModelAlignedShPool::MappingNew(DShBufMapping*& aMapping, DMemModelProcess* aProcess)
	{
	// Must be in critical section so we don't leak os asid references.
	__ASSERT_CRITICAL;

	TMappingCreateFlags flags=EMappingCreateDefault;

	FlagSet(flags, EMappingCreateReserveAllResources);

	// Open a reference to aProcess's os so it isn't freed and reused while
	// we're creating this mapping.
	TInt osAsid = aProcess->TryOpenOsAsid();
	if (osAsid < 0)
		{// The process has freed its os asid so can't create a new mapping.
		return KErrDied;
		}

	DMemoryMapping* mapping = NULL;
	DShBufMapping* m = NULL;
	TInt r = MM::MappingNew(mapping, MM::RoundToPageCount(iBufGap), osAsid, flags);

	if (r == KErrNone)
		{
		m = new DShBufMapping;

		if (m)
			{
			m->iMapping = mapping;
			m->iOsAsid = osAsid;
			}
		else
			{
			MM::MappingDestroy(mapping);
			r = KErrNoMemory;
			}
		}

	// Close the reference on the os asid as while aMapping is valid then the 
	// os asid must be also.
	aProcess->CloseOsAsid();

	aMapping = m;
	__KTRACE_OPT(KMMU2, Kern::Printf("DMemModelAlignedShPool::MappingNew returns 0x%08x,%d",aMapping,r));
	return r;
	}

TInt DMemModelAlignedShPool::AddToProcess(DProcess* aProcess, TUint aAttr)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Adding DMemModelAlignedShPool %O to process %O",this,aProcess));
	TInt r = KErrNone;

	Kern::MutexWait(*iProcessLock);

	LockPool();
	DShPoolClient* client = reinterpret_cast<DShPoolClient*>(iClientMap->Find(reinterpret_cast<TUint>(aProcess)));
	UnlockPool();

	if (!client)
		{
		client = new DMemModelAlignedShPoolClient;
		if (client)
			{
			client->iFlags = aAttr;
			r = iClientMap->Add(reinterpret_cast<TUint>(aProcess), client);

			if (r == KErrNone)
				{
				if (aProcess != K::TheKernelProcess)
					{
					r = aProcess->iHandles.Reserve(iTotalBuffers);

					if (r != KErrNone)
						{
						iClientMap->Remove(reinterpret_cast<TUint>(aProcess));
						}
					}
				}
			if (r != KErrNone)
				{
				delete client;
				r = KErrNoMemory;
				}
			}
		else
			{
			r = KErrNoMemory;
			}
		}
	else
		{
		LockPool();
		client->iAccessCount++;
		UnlockPool();
		}

	Kern::MutexSignal(*iProcessLock);

	return r;
	}

DMemModelAlignedShPool::DMemModelAlignedShPool() :	DMemModelShPool()

	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShPool::DMemModelAlignedShPool"));
	}

void DMemModelAlignedShPool::Free(DShBuf* aBuf)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShPool::Free (aBuf = 0x%08x)", aBuf));

	LockPool();
#ifdef _DEBUG
	// Remove from allocated list
	aBuf->iObjLink.Deque();
#endif

	DMemModelAlignedShBuf* buf = reinterpret_cast<DMemModelAlignedShBuf*>(aBuf);

	if (MM::MemoryIsNotMapped(buf->iMemoryObject))
		{
		UnlockPool(); // have to release fast mutex
		MM::MemoryWipe(buf->iMemoryObject);
		LockPool();

		// we want to put the initial buffers at the head of the free list
		// and the grown buffers at the tail as this makes shrinking more efficient
		if (aBuf >= iInitialBuffersArray && aBuf < (iInitialBuffersArray + iInitialBuffers))
			{
			iFreeList.AddHead(&aBuf->iObjLink);
			}
		else
			{
			iFreeList.Add(&aBuf->iObjLink);
			}
		++iFreeBuffers;
#ifdef _DEBUG
		--iAllocatedBuffers;
#endif
		}
	else
		{
		iPendingList.Add(&aBuf->iObjLink);
		}

	iPoolFlags &= ~EShPoolSuppressShrink;		// Allow shrinking again, if it was blocked
	UnlockPool();

	// queue ManagementDfc which completes notifications as appropriate
	if (HaveWorkToDo())
		KickManagementDfc();

	DShPool::Close(NULL); // decrement pool reference count
	}

TInt DMemModelAlignedShPool::UpdateFreeList()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShPool::UpdateFreeList"));

	LockPool();
	SDblQueLink* pLink = iPendingList.First();
	UnlockPool();

	SDblQueLink* anchor = &iPendingList.iA;

	while (pLink != anchor)
		{
		DMemModelAlignedShBuf* buf = _LOFF(pLink, DMemModelAlignedShBuf, iObjLink);
		LockPool();
		pLink = pLink->iNext;
		UnlockPool();

		if (MM::MemoryIsNotMapped(buf->iMemoryObject))
			{
			LockPool();
			buf->iObjLink.Deque();
			UnlockPool();

			MM::MemoryWipe(buf->iMemoryObject);

			LockPool();
			if (buf >= iInitialBuffersArray && buf < (iInitialBuffersArray + iInitialBuffers))
				{
				iFreeList.AddHead(&buf->iObjLink);
				}
			else
				{
				iFreeList.Add(&buf->iObjLink);
				}
			++iFreeBuffers;
#ifdef _DEBUG
			--iAllocatedBuffers;
#endif
			UnlockPool();
			}
		}

	__KTRACE_OPT(KMMU, Kern::Printf("<DMemModelAlignedShPool::UpdateFreeList"));
	return KErrNone;
	}

DMemModelAlignedShPool::~DMemModelAlignedShPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShPool::~DMemModelAlignedShPool"));
	}

TInt DMemModelAlignedShPool::DoCreate(TShPoolCreateInfo& aInfo)
	{

	TUint64 maxSize64 = static_cast<TUint64>(aInfo.iInfo.iMaxBufs) * static_cast<TUint64>(iBufGap);

	if (maxSize64 > static_cast<TUint64>(KMaxTInt) || maxSize64 <= static_cast<TUint64>(0))
		return KErrArgument;

	iMaxPages = MM::RoundToPageCount(static_cast<TInt>(maxSize64));

	return KErrNone;
	}

TInt DMemModelAlignedShPool::DestroyAllMappingsAndReservedHandles(DProcess* aProcess)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShPool::DestroyAllMappingsAndReservedHandles(0x%08x)", aProcess));

	TInt r = KErrNone;
	Kern::MutexWait(*iProcessLock);
	DMemModelAlignedShPoolClient* client = reinterpret_cast<DMemModelAlignedShPoolClient*>(iClientMap->Remove(reinterpret_cast<TUint>(aProcess)));

	__NK_ASSERT_DEBUG(client);
	__NK_ASSERT_DEBUG(client->iAccessCount == 0);

	DestroyMappings(client, KMaxTInt);
	delete client;

	if (aProcess != K::TheKernelProcess)
		{
		// Remove reserved handles
		r = aProcess->iHandles.Reserve(-iTotalBuffers);
		}

	Kern::MutexSignal(*iProcessLock);

	__KTRACE_OPT(KMMU, Kern::Printf("<DMemModelAlignedShPool::DestroyAllMappingsAndReservedHandles(0x%08x)", aProcess));

	return r;
	}

TInt DMemModelAlignedShPool::DestroyMappings(DMemModelAlignedShPoolClient* aClient, TInt aNoOfMappings)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShPool::DestroyMappings(0x%08x)", aClient));

	TInt r = KErrNone;
	TInt i = 0;

	DShBufMapping* m = NULL;
	SDblQueLink* pLink = NULL;

	while (i < aNoOfMappings && !aClient->iMappingFreeList.IsEmpty())
		{
		LockPool();
		pLink = aClient->iMappingFreeList.GetFirst();
		UnlockPool();

		if (pLink == NULL)
			break;

		m = _LOFF(pLink, DShBufMapping, iObjLink);
		__KTRACE_OPT(KMMU2, Kern::Printf("DMemModelAlignedShPool::DestroyMappings delete 0x%08x",m));
		MM::MappingClose(m->iMapping);
		delete m;
		++i;
		}

	__KTRACE_OPT(KMMU, Kern::Printf("<DMemModelAlignedShPool::DestroyMappings"));

	return r;
	}


TInt DMemModelAlignedShPool::CreateMappings(DMemModelAlignedShPoolClient* aClient, TInt aNoOfMappings, DMemModelProcess* aProcess)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShPool::CreateMappings"));

	__ASSERT_MUTEX(iProcessLock);

	TInt r = KErrNone;

	for (TInt i = 0; i < aNoOfMappings; ++i)
		{
		DShBufMapping* mapping;
		r = MappingNew(mapping, aProcess);
		if (r == KErrNone)
			{
			LockPool();
			aClient->iMappingFreeList.AddHead(&mapping->iObjLink);
			UnlockPool();
			}
		else
			{
			r = KErrNoMemory;
			break;
			}
		}

	return r;
	}

TInt DMemModelAlignedShPool::UpdateMappingsAndReservedHandles(TInt aNoOfBuffers)
	{
	__KTRACE_OPT(KMMU2, Kern::Printf(">DMemModelAlignedShPool::UpdateMappingsAndReservedHandles(0x%08x)", aNoOfBuffers));

	SMap::TIterator iter(*iClientMap);
	SMap::TEntry* entry;
	SMap::TEntry* lastEntry = NULL;
	DMemModelProcess* pP;
	DMemModelAlignedShPoolClient* client;
	TInt result = KErrNone;

	Kern::MutexWait(*iProcessLock);

	// First handle the case of increasing allocation
	if (aNoOfBuffers > 0)
		while ((entry = iter.Next()) != lastEntry)
			{
			// Try to update handle reservation; skip if process is null or has gone away
			client = (DMemModelAlignedShPoolClient*)(entry->iObj);
			pP = (DMemModelProcess*)(entry->iKey);
			if (!pP)
				continue;
			TInt r = pP->iHandles.Reserve(aNoOfBuffers);
			if (r)
				__KTRACE_OPT(KMMU2, Kern::Printf("?DMemModelAlignedShPool::UpdateMappingsAndReservedHandles(0x%08x) Reserve failed %d", aNoOfBuffers, r));
			if (r == KErrDied)
				continue;

			if (r == KErrNone && client->iWindowSize <= 0)
				{
				// A positive window size means the number of mappings is fixed, so we don't need to reserve more.
				// But here zero or negative means a variable number, so we need to create extra mappings now.
				r = CreateMappings(client, aNoOfBuffers, pP);
				if (r != KErrNone)
					{
					__KTRACE_OPT(KMMU2, Kern::Printf("?DMemModelAlignedShPool::UpdateMappingsAndReservedHandles(0x%08x) CreateMappings failed %d", aNoOfBuffers, r));
					pP->iHandles.Reserve(-aNoOfBuffers); // Creation failed, so release the handles reserved above
					}
				}

			if (r != KErrNone)
				{
				// Some problem; cleanup as best we can by falling into the loop below to undo what we've done
				result = r;
				iter.Reset();
				lastEntry = entry;
				aNoOfBuffers = -aNoOfBuffers;
				break;
				}
			}

	// Now handle the case of decreasing allocation; also used for recovery from errors, in which case
	// this loop iterates only over the elements that were *successfully* processed by the loop above
	if (aNoOfBuffers < 0)
		while ((entry = iter.Next()) != lastEntry)
			{
			// Try to update handle reservation; skip if process is null or has gone away
			client = (DMemModelAlignedShPoolClient*)(entry->iObj);
			pP = (DMemModelProcess*)(entry->iKey);
			if (!pP)
				continue;
			TInt r = pP->iHandles.Reserve(aNoOfBuffers);
			if (r == KErrDied)
				continue;

			if (r == KErrNone && client->iWindowSize <= 0)
				r = DestroyMappings(client, -aNoOfBuffers);
			// De-allocation by Reserve(-n) and/or DestroyMappings() should never fail
			if (r != KErrNone)
				Kern::PanicCurrentThread(KLitDMemModelAlignedShPool, r);
			}

	Kern::MutexSignal(*iProcessLock);

	__KTRACE_OPT(KMMU2, Kern::Printf("<DMemModelAlignedShPool::UpdateMappingsAndReservedHandles(0x%08x) returning %d", aNoOfBuffers, result));
	return result;
	}

TInt DMemModelAlignedShPool::DeleteInitialBuffers()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShPool::DeleteInitialBuffers"));

	if (iInitialBuffersArray != NULL)
		{
		for (TUint i = 0; i < iInitialBuffers; i++)
			{
			iInitialBuffersArray[i].iObjLink.Deque(); // remove from free list
			iInitialBuffersArray[i].Dec();
			iInitialBuffersArray[i].~DMemModelAlignedShBuf();
			}
		}

	Kern::Free(iInitialBuffersArray);
	iInitialBuffersArray = NULL;

	return KErrNone;
	}

TInt DMemModelAlignedShPool::Close(TAny* aPtr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShPool::Close(0x%08x)", aPtr));

	if (aPtr)
		{
		DProcess* pP = reinterpret_cast<DProcess*>(aPtr);

		CloseClient(pP);
		}
	__KTRACE_OPT(KMMU, Kern::Printf("<DMemModelAlignedShPool::Close(0x%08x)", aPtr));
	return DShPool::Close(aPtr);
	}

TInt DMemModelAlignedShPool::CreateInitialBuffers()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShPool::CreateInitialBuffers"));

	iInitialBuffersArray = reinterpret_cast<DMemModelAlignedShBuf*>(Kern::Alloc(iInitialBuffers * sizeof(DMemModelAlignedShBuf)));

	if (iInitialBuffersArray == NULL)
		return KErrNoMemory;

	for (TUint i = 0; i < iInitialBuffers; i++)
		{
		// always use kernel linear address in DShBuf
		DMemModelAlignedShBuf *buf = new (&iInitialBuffersArray[i]) DMemModelAlignedShBuf(this);
		TInt r = buf->Construct();

		if (r == KErrNone)
			{
			iFreeList.Add(&buf->iObjLink);
			}
		else
			{
			iInitialBuffers = i;
			return KErrNoMemory;
			}
		}

	iFreeBuffers  = iInitialBuffers;
	iTotalBuffers = iInitialBuffers;
	return KErrNone;
	}


TInt DMemModelAlignedShPool::GrowPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShPool::GrowPool()"));
	TInt r = KErrNone;
	SDblQue temp;

	Kern::MutexWait(*iProcessLock);

	TUint32 headroom = iMaxBuffers - iTotalBuffers;

	// How many buffers to grow by?
	TUint32 grow = mult_fx248(iTotalBuffers, iGrowByRatio);
	if (grow == 0)			// Handle round-to-zero
		grow = 1;
	if (grow > headroom)
		grow = headroom;

	TUint i;
	for (i = 0; i < grow; ++i)
		{
		DMemModelAlignedShBuf *buf = new DMemModelAlignedShBuf(this);

		if (buf == NULL)
			{
			r = KErrNoMemory;
			break;
			}

		TInt r = buf->Construct();

		if (r != KErrNone)
			{
			buf->DObject::Close(NULL);
			break;
			}

		temp.Add(&buf->iObjLink);
		}

	r = UpdateMappingsAndReservedHandles(i);

	if (r == KErrNone)
		{
		LockPool();
		iFreeList.MoveFrom(&temp);
		iFreeBuffers += i;
		iTotalBuffers += i;
		UnlockPool();
		}
	else
		{
		// couldn't create either the mappings or reserve handles so have no choice but to
		// delete the buffers
		SDblQueLink *pLink;
		while ((pLink = temp.GetFirst()) != NULL)
			{
			DShBuf* buf = _LOFF(pLink, DShBuf, iObjLink);
			buf->DObject::Close(NULL);
			}
		}

	CalculateGrowShrinkTriggers();

	Kern::MutexSignal(*iProcessLock);

	__KTRACE_OPT(KMMU, Kern::Printf("<DMemModelAlignedShPool::GrowPool()"));
	return r;
	}

TInt DMemModelAlignedShPool::ShrinkPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShPool::ShrinkPool()"))

	Kern::MutexWait(*iProcessLock);

	TUint32 grownBy = iTotalBuffers - iInitialBuffers;

	// How many buffers to shrink by?
	TUint32 shrink = mult_fx248(iTotalBuffers, iShrinkByRatio);
	if (shrink == 0)		// Handle round-to-zero
		shrink = 1;
	if (shrink > grownBy)
		shrink = grownBy;
	if (shrink > iFreeBuffers)
		shrink = iFreeBuffers;

	// work backwards as the grown buffers should be at the back
	TUint i;
	for (i = 0; i < shrink; i++)
		{
		LockPool();

		if (iFreeList.IsEmpty())
			{
			UnlockPool();
			break;
			}

		DShBuf* buf = _LOFF(iFreeList.Last(), DShBuf, iObjLink);

		// can't delete initial buffers
		if (buf >= iInitialBuffersArray && buf < (iInitialBuffersArray + iInitialBuffers))
			{
			UnlockPool();
			break;
			}

		buf->iObjLink.Deque();
		--iFreeBuffers;
		--iTotalBuffers;
		UnlockPool();
		buf->DObject::Close(NULL);
		}

	TInt r = UpdateMappingsAndReservedHandles(-i);

	// If we couldn't shrink the pool by this many buffers, wait until we Free() another
	// buffer before trying to shrink again.
	if (i < shrink)
		iPoolFlags |= EShPoolSuppressShrink;

	CalculateGrowShrinkTriggers();

	Kern::MutexSignal(*iProcessLock);

	__KTRACE_OPT(KMMU, Kern::Printf("<DMemModelAlignedShPool::ShrinkPool()"));
	return r;
	}

// Kernel side API
TInt DMemModelAlignedShPool::Alloc(DShBuf*& aShBuf)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelAlignedShPool::Alloc (DShBuf)"));

	TInt r = KErrNoMemory;
	aShBuf = NULL;

	LockPool();

	if (!iFreeList.IsEmpty())
		{
		aShBuf = _LOFF(iFreeList.GetFirst(), DShBuf, iObjLink);

#ifdef _DEBUG
		iAllocated.Add(&aShBuf->iObjLink);
		iAllocatedBuffers++;
#endif
		--iFreeBuffers;
		Open(); // increment pool reference count
		r = KErrNone;
		}

	UnlockPool();

	if (HaveWorkToDo())
		KickManagementDfc();

	__KTRACE_OPT(KMMU, Kern::Printf("<DMemModelAlignedShPool::Alloc return buf = 0x%08x", aShBuf));
	return r;
	}

DMemModelNonAlignedShBuf::DMemModelNonAlignedShBuf(DShPool* aPool, TLinAddr aRelAddr) : DShBuf(aPool, aRelAddr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelNonAlignedShBuf::DMemModelNonAlignedShBuf()"));
	}

DMemModelNonAlignedShBuf::~DMemModelNonAlignedShBuf()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelNonAlignedShBuf::~DMemModelNonAlignedShBuf()"));
	}

TInt DMemModelNonAlignedShBuf::Close(TAny* aPtr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelNonAlignedShBuf::Close(0x%08x)", aPtr));

	if (aPtr)
		{
		DProcess* pP = reinterpret_cast<DProcess*>(aPtr);

		// there no per buffer resources for kernel clients for non-aligned buffers
		if (pP != K::TheKernelProcess)
		    iPool->CloseClient(pP);
		}

	return DShBuf::Close(aPtr);
	}

TInt DMemModelNonAlignedShBuf::AddToProcess(DProcess* aProcess, TUint /* aAttr */)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("Adding DMemModelShBuf %O to process %O", this, aProcess));
	TUint flags;

	return iPool->OpenClient(aProcess, flags);
	}


TUint8* DMemModelNonAlignedShBuf::Base(DProcess* aProcess)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelNonAlignedShBuf::Base(0x%x)", aProcess));

	TUint8* base = reinterpret_cast<DMemModelNonAlignedShPool*>(iPool)->Base(aProcess) + (TUint)iRelAddress;

	return base;
	}

TUint8* DMemModelNonAlignedShBuf::Base()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelNonAlignedShBuf::Base()"));

	TUint8* base = reinterpret_cast<DMemModelNonAlignedShPool*>(iPool)->Base();

	return base ? base + iRelAddress : NULL;
	}

TInt DMemModelNonAlignedShBuf::Map(TUint /* aMapAttr */, DProcess* /* aProcess */, TLinAddr& /* aBase */)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelNonAlignedShBuf::Map()"));

	return KErrNotSupported;
	}

TInt DMemModelNonAlignedShBuf::UnMap(DProcess* /* aProcess */)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelNonAlignedShBuf::UnMap()"));

	return KErrNotSupported;
	}

TInt DMemModelNonAlignedShBuf::Pin(TPhysicalPinObject* aPinObject, TBool aReadOnly, TPhysAddr& aAddress, TPhysAddr* aPages, TUint32& aMapAttr, TUint& aColour)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DMemModelNonAlignedShBuf::Pin");

	DMemModelNonAlignedShPool* pool = reinterpret_cast<DMemModelNonAlignedShPool*>(iPool);

	NKern::ThreadEnterCS();

	TInt startPage = iRelAddress >> KPageShift;
	TInt lastPage = MM::RoundToPageCount(iRelAddress + Size());

	TInt pages = lastPage - startPage;

	if (!pages) pages++;

	TInt r = MM::PinPhysicalMemory(pool->iMemoryObject, (DPhysicalPinMapping*)aPinObject,
									startPage, pages, aReadOnly, aAddress, aPages, aMapAttr, aColour);

	// adjust physical address to start of the buffer
	if (r == KErrNone)
		{
		aAddress += (iRelAddress - (startPage << KPageShift));
		}
	NKern::ThreadLeaveCS();
	return r;
	}

DMemModelNonAlignedShPool::DMemModelNonAlignedShPool() : DMemModelShPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelNonAlignedShPool::DMemModelNonAlignedShPool"));
	}

DMemModelNonAlignedShPool::~DMemModelNonAlignedShPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelNonAlignedShPool::~DMemModelNonAlignedShPool"));

	MM::MemoryDestroy(iMemoryObject);

	delete iPagesMap;
	delete iBufMap;
	}

TInt DMemModelNonAlignedShPool::DoCreate(TShPoolCreateInfo& aInfo)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("DMemModelNonAlignedShPool::DoCreate(%d, %d, %d)", aInfo.iInfo.iMaxBufs, iBufGap, iBufSize));

	TInt r;
	TUint64 maxSize64 = static_cast<TUint64>(aInfo.iInfo.iMaxBufs) * static_cast<TUint64>(iBufGap);

	if (maxSize64 > static_cast<TUint64>(KMaxTInt) || maxSize64 <= static_cast<TUint64>(0))
		return KErrArgument;

	TInt maxPages = MM::RoundToPageCount(static_cast<TInt>(maxSize64));

	iBufMap = TBitMapAllocator::New(aInfo.iInfo.iMaxBufs, (TBool)ETrue);
	if (iBufMap == NULL)
		return KErrNoMemory;

	iPagesMap = TBitMapAllocator::New(maxPages, (TBool)ETrue);
	if (iPagesMap == NULL)
		return KErrNoMemory;

	// Memory attributes
	TMemoryAttributes attr = EMemoryAttributeStandard;

	// Memory type
	TMemoryObjectType memoryType = (iPoolFlags & EShPoolPhysicalMemoryPool) ? EMemoryObjectHardware : EMemoryObjectUnpaged;

	// Memory flags
	TMemoryCreateFlags memoryFlags = EMemoryCreateDefault;	// Don't leave previous contents of memory

	// Now create the memory object
	r = MM::MemoryNew(iMemoryObject, memoryType, maxPages, memoryFlags, attr);
	if (r != KErrNone)
		return r;

	// Make sure we give the caller the number of buffers they were expecting
	iCommittedPages = MM::RoundToPageCount(iInitialBuffers * iBufGap);

	if (iPoolFlags & EShPoolPhysicalMemoryPool)
		{
		__KTRACE_OPT(KMMU, Kern::Printf("DMemModelNonAlignedShPool::DoCreate(iCommittedPages = 0x%08x, aInfo.iPhysAddr.iPhysAddrList = 0x%08x )", iCommittedPages, aInfo.iPhysAddr.iPhysAddrList));
		if (iPoolFlags & EShPoolContiguous)
			{
			r = MM::MemoryAddContiguous(iMemoryObject, 0, iCommittedPages, aInfo.iPhysAddr.iPhysAddr);
			}
		else
			{
			r = MM::MemoryAddPages(iMemoryObject, 0, iCommittedPages, aInfo.iPhysAddr.iPhysAddrList);
			}

		iMaxPages = iCommittedPages;
		}
	else
		{
		__KTRACE_OPT(KMMU, Kern::Printf("DMemModelNonAlignedShPool::DoCreate(iCommittedPages = %d, contig = %d)", iCommittedPages, iPoolFlags & EShPoolContiguous));

		if (iPoolFlags & EShPoolContiguous)
			{
			TPhysAddr paddr;
			r = MM::MemoryAllocContiguous(iMemoryObject, 0, iCommittedPages, 0, paddr);
			}
		else
			{
			r = MM::MemoryAlloc(iMemoryObject, 0, iCommittedPages);
			}

		iMaxPages = maxPages;
		}

	iPagesMap->Alloc(0, iCommittedPages);
	
	return r;
	}

TUint8* DMemModelNonAlignedShPool::Base(DProcess* aProcess)
	{
	TUint8 *base = 0;

	LockPool();
	DMemModelNonAlignedShPoolClient* client = reinterpret_cast<DMemModelNonAlignedShPoolClient*>(iClientMap->Find(reinterpret_cast<TUint>(aProcess)));

	__NK_ASSERT_DEBUG(client); // ASSERT because pool must be already opened in the clients address space
	__NK_ASSERT_DEBUG(client->iMapping); // ASSERT because non-aligned buffers are mapped by default in user space

	base = reinterpret_cast<TUint8*>(MM::MappingBase(client->iMapping));

	UnlockPool();

	return base;
	}

TInt DMemModelNonAlignedShPool::CreateInitialBuffers()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelNonAlignedShPool::CreateInitialBuffers"));

	iInitialBuffersArray = reinterpret_cast<DMemModelNonAlignedShBuf*>(Kern::Alloc(iInitialBuffers * sizeof(DMemModelNonAlignedShBuf)));

	if (iInitialBuffersArray == NULL)
		return KErrNoMemory;

	TLinAddr offset = 0;
	for (TUint i = 0; i < iInitialBuffers; i++)
		{
		DMemModelNonAlignedShBuf *buf = new (&iInitialBuffersArray[i]) DMemModelNonAlignedShBuf(this, offset);
		TInt r = buf->Construct();

		if (r == KErrNone)
			{
			iFreeList.Add(&buf->iObjLink);
			}
		else
			{
			iInitialBuffers = i;
			return KErrNoMemory;
			}

		offset += iBufGap;
		}

	iFreeBuffers  = iInitialBuffers;
	iTotalBuffers = iInitialBuffers;
	iBufMap->Alloc(0, iInitialBuffers);

	return KErrNone;
	}

TInt DMemModelNonAlignedShPool::AddToProcess(DProcess* aProcess, TUint aAttr)
	{
	// Must be in critical section so we don't leak os asid references.
	__ASSERT_CRITICAL;
	__KTRACE_OPT(KMMU, Kern::Printf("Adding DMemModelShPool %O to process %O", this, aProcess));

	DMemoryMapping* mapping = NULL;

	TBool write = (TBool)EFalse;

	// User = ETrue, ReadOnlyWrite = ETrue, Execute = EFalse
	if (aAttr & EShPoolWriteable)
		write = (TBool)ETrue;

	TMappingPermissions perm = MM::MappingPermissions(ETrue,	// user
													  write,	// writeable
													  EFalse);	// execute

	TMappingCreateFlags mappingFlags = EMappingCreateDefault;

	DMemModelProcess* pP = reinterpret_cast<DMemModelProcess*>(aProcess);

	Kern::MutexWait(*iProcessLock);
	TInt r = KErrNone;

	LockPool();
	DMemModelNonAlignedShPoolClient* client = reinterpret_cast<DMemModelNonAlignedShPoolClient*>(iClientMap->Find(reinterpret_cast<TUint>(aProcess)));
	UnlockPool();

	if (!client)
		{
		client = new DMemModelNonAlignedShPoolClient;

		if (client)
			{
			// map non aligned pools in userside processes by default
			if (aAttr & EShPoolAutoMapBuf || pP != K::TheKernelProcess)
				{
				// Open a reference on the os asid so it doesn't get freed and reused.
				TInt osAsid = pP->TryOpenOsAsid();
				if (osAsid < 0)
					{// The process freed its os asid so can't create a new mapping.
					r = KErrDied;
					}
				else
					{
					r = MM::MappingNew(mapping, iMemoryObject, perm, osAsid, mappingFlags);
					// Close the reference as the mapping will be destroyed if the process dies.
					pP->CloseOsAsid();
					}

				if ((r == KErrNone) && (pP == K::TheKernelProcess))
					{
					iBaseAddress = MM::MappingBase(mapping);
					}
				}

			if (r == KErrNone)
				{
				client->iMapping = mapping;
				client->iFlags = aAttr;
				r = iClientMap->Add(reinterpret_cast<TUint>(aProcess), client);

				if (r == KErrNone)
					{
					if (pP != K::TheKernelProcess)
						{
						r = aProcess->iHandles.Reserve(iTotalBuffers);

						if (r != KErrNone)
							{
							iClientMap->Remove(reinterpret_cast<TUint>(aProcess));
							}
						}
					}

				if (r != KErrNone)
					{
					delete client;
					MM::MappingDestroy(mapping);
					}
				}
			else
				{
				delete client;
				}
			}
		else
			{
			r = KErrNoMemory;
			}
		}
	else
		{
		LockPool();
		client->iAccessCount++;
		UnlockPool();
		}

	Kern::MutexSignal(*iProcessLock);

	return r;
	}

TInt DMemModelNonAlignedShPool::DeleteInitialBuffers()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelNonAlignedShPool::DeleteInitialBuffers"));

	if (iInitialBuffersArray != NULL)
		{
		for (TUint i = 0; i < iInitialBuffers; i++)
			{
			iInitialBuffersArray[i].iObjLink.Deque(); // remove from free list
			iInitialBuffersArray[i].Dec();
			iInitialBuffersArray[i].~DMemModelNonAlignedShBuf();
			}
		}

	Kern::Free(iInitialBuffersArray);
	iInitialBuffersArray = NULL;

	return KErrNone;
	}

TInt DMemModelNonAlignedShPool::DestroyAllMappingsAndReservedHandles(DProcess* aProcess)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelNonAlignedShPool::DestroyAllMappingsAndReservedHandles(0x%08x)", aProcess));

	TInt r = KErrNone;
	Kern::MutexWait(*iProcessLock);
	DMemModelNonAlignedShPoolClient* client = reinterpret_cast<DMemModelNonAlignedShPoolClient*>(iClientMap->Remove(reinterpret_cast<TUint>(aProcess)));

	__NK_ASSERT_DEBUG(client);
	__NK_ASSERT_DEBUG(client->iAccessCount == 0);

	if (client->iMapping)
		{
		MM::MappingDestroy(client->iMapping);
		}
	delete client;

	if (aProcess != K::TheKernelProcess)
		{
		// Remove reserved handles
		r = aProcess->iHandles.Reserve(-(iTotalBuffers));
		}
	else
		{
		iBaseAddress = 0;
		}

	Kern::MutexSignal(*iProcessLock);

	__KTRACE_OPT(KMMU, Kern::Printf("<DMemModelNonAlignedShPool::DestroyAllMappingsAndReservedHandles(0x%08x)", aProcess));

	return r;
	}


TInt DMemModelNonAlignedShPool::Close(TAny* aPtr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelNonAlignedShPool::Close(0x%08x)", aPtr));

	if (aPtr)
		{
		DProcess* pP = reinterpret_cast<DProcess*>(aPtr);

		CloseClient(pP);
		}

	return DShPool::Close(aPtr);
	}

void DMemModelNonAlignedShPool::FreeBufferPages(TUint aOffset)
	{
	TLinAddr firstByte = aOffset;	// offset of first byte in buffer
	TLinAddr lastByte = firstByte+iBufGap-1;	// offset of last byte in buffer
	TUint firstPage = firstByte>>KPageShift;	// index of first page containing part of the buffer
	TUint lastPage = lastByte>>KPageShift;		// index of last page containing part of the buffer

	TUint firstBuffer = (firstByte&~KPageMask)/iBufGap; // index of first buffer which lies in firstPage
	TUint lastBuffer = (lastByte|KPageMask)/iBufGap;    // index of last buffer which lies in lastPage
	TUint thisBuffer = firstByte/iBufGap;				// index of the buffer to be freed

	// Ensure lastBuffer is within bounds (there may be room in the last
	// page for more buffers than we have allocated).
	if (lastBuffer >= iMaxBuffers)
		lastBuffer = iMaxBuffers-1;

	if(firstBuffer!=thisBuffer && iBufMap->NotFree(firstBuffer,thisBuffer-firstBuffer))
		{
		// first page has other allocated buffers in it,
		// so we can't free it and must move on to next one...
		if (firstPage >= lastPage)
			return;
		++firstPage;
		}

	if(lastBuffer!=thisBuffer && iBufMap->NotFree(thisBuffer+1,lastBuffer-thisBuffer))
		{
		// last page has other allocated buffers in it,
		// so we can't free it and must step back to previous one...
		if (lastPage <= firstPage)
			return;
		--lastPage;
		}

	if(firstPage<=lastPage)
		{
		// we can free pages firstPage trough to lastPage...
		TUint numPages = lastPage-firstPage+1;
		iPagesMap->SelectiveFree(firstPage,numPages);
		MM::MemoryLock(iMemoryObject);
		MM::MemoryFree(iMemoryObject, firstPage, numPages);
		MM::MemoryUnlock(iMemoryObject);
		iCommittedPages -= numPages;
		}
	}

TInt DMemModelNonAlignedShPool::GrowPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelNonAlignedShPool::GrowPool()"));

	// Don't do anything with physical memory pools
	if (iPoolFlags & EShPoolPhysicalMemoryPool)
		return KErrNone;

	Kern::MutexWait(*iProcessLock);

	TUint32 headroom = iMaxBuffers - iTotalBuffers;

	// How many buffers to grow by?
	TUint32 grow = mult_fx248(iTotalBuffers, iGrowByRatio);
	if (grow == 0)			// Handle round-to-zero
		grow = 1;
	if (grow > headroom)
		grow = headroom;

	TInt r = KErrNone;
	SDblQue temp;

	TUint i;
	for (i = 0; i < grow; ++i)
		{
		TInt offset = iBufMap->Alloc();

		if (offset < 0)
			{
			r = KErrNoMemory;
			break;
			}

		offset *= iBufGap;

		TInt lastPage = (offset + iBufSize - 1) >> KPageShift;

		// Allocate one page at a time.
		for (TInt page = offset >> KPageShift; page <= lastPage; ++page)
			{
			// Is the page allocated?
			if (iPagesMap->NotAllocated(page, 1))
				{
				MM::MemoryLock(iMemoryObject);
				r = MM::MemoryAlloc(iMemoryObject, page, 1);
				MM::MemoryUnlock(iMemoryObject);

				if (r != KErrNone)
					{
					break;
					}

				++iCommittedPages;
				iPagesMap->Alloc(page, 1);
				}
			}

		if (r != KErrNone)
			{
			iBufMap->Free(offset / iBufGap);
			FreeBufferPages(offset);
			break;
			}

		DMemModelNonAlignedShBuf *buf = new DMemModelNonAlignedShBuf(this, offset);

		if (buf == NULL)
			{
			iBufMap->Free(offset / iBufGap);
			FreeBufferPages(offset);
			r = KErrNoMemory;
			break;
			}

		r = buf->Construct();

		if (r != KErrNone)
			{
			iBufMap->Free(offset / iBufGap);
			FreeBufferPages(offset);
			buf->DObject::Close(NULL);
			break;
			}

		temp.Add(&buf->iObjLink);
		}

	r = UpdateReservedHandles(i);

	if (r == KErrNone)
		{
		LockPool();
		iFreeList.MoveFrom(&temp);
		iFreeBuffers += i;
		iTotalBuffers += i;
		UnlockPool();
		}
	else
		{
		// couldn't reserve handles so have no choice but to
		// delete the buffers
		__KTRACE_OPT(KMMU, Kern::Printf("GrowPool failed with %d, deleting buffers", r));
		SDblQueLink *pLink;
		while ((pLink = temp.GetFirst()) != NULL)
			{
			DShBuf* buf = _LOFF(pLink, DShBuf, iObjLink);
			TLinAddr offset = buf->iRelAddress;
			iBufMap->Free(offset / iBufGap);
			FreeBufferPages(offset);
			buf->DObject::Close(NULL);
			}
		__KTRACE_OPT(KMMU, Kern::Printf("Buffers deleted"));
		}

	CalculateGrowShrinkTriggers();

	Kern::MutexSignal(*iProcessLock);

	__KTRACE_OPT(KMMU, Kern::Printf("<DMemModelNonAlignedShPool::GrowPool()"));
	return r;
	}

TInt DMemModelNonAlignedShPool::ShrinkPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelNonAlignedShPool::ShrinkPool()"));

	// Don't do anything with physical memory pools
	if (iPoolFlags & EShPoolPhysicalMemoryPool)
		return KErrNone;

	Kern::MutexWait(*iProcessLock);

	TUint32 grownBy = iTotalBuffers - iInitialBuffers;

	// How many buffers to shrink by?
	TUint32 shrink = mult_fx248(iTotalBuffers, iShrinkByRatio);
	if (shrink == 0)		// Handle round-to-zero
		shrink = 1;
	if (shrink > grownBy)
		shrink = grownBy;
	if (shrink > iFreeBuffers)
		shrink = iFreeBuffers;

	TUint i;
	for (i = 0; i < shrink; ++i)
		{
		LockPool();

		if (iFreeList.IsEmpty())
			{
			UnlockPool();
			break;
			}

		// work from the back of the queue
		SDblQueLink *pLink = iFreeList.Last();

		DShBuf* pBuf = _LOFF(pLink, DShBuf, iObjLink);

		if (pBuf >= iInitialBuffersArray && pBuf < (iInitialBuffersArray + iInitialBuffers))
			{
			UnlockPool();
			break;
			}

		--iFreeBuffers;
		--iTotalBuffers;
		pLink->Deque();
		UnlockPool();

		TLinAddr offset = pBuf->iRelAddress;
		iBufMap->Free(offset / iBufGap);
		FreeBufferPages(offset);

		pBuf->DObject::Close(NULL);
		}

	UpdateReservedHandles(-(TInt)i);

	// If we couldn't shrink the pool by this many buffers, wait until we Free() another
	// buffer before trying to shrink again.
	if (i < shrink)
		iPoolFlags |= EShPoolSuppressShrink;

	CalculateGrowShrinkTriggers();

	Kern::MutexSignal(*iProcessLock);

	__KTRACE_OPT(KMMU, Kern::Printf("<DMemModelNonAlignedShPool::ShrinkPool()"));

	return KErrNone;
	}

TInt DMemModelNonAlignedShPool::UpdateFreeList()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelNonAlignedShPool::UpdateFreeList"));

	SDblQue temp;

	LockPool();
	while(!iAltFreeList.IsEmpty())
		{
		// sort a temporary list of 'n' object with the lowest index first
		for (TInt n = 0; n < 8 && !iAltFreeList.IsEmpty(); ++n)
			{
			// bit of an assumption, lets assume that the lower indexes will be allocated and freed first
			// and therefore will be nearer the front of the list
			DShBuf* buf = _LOFF(iAltFreeList.GetFirst(), DShBuf, iObjLink);

			SDblQueLink* anchor = reinterpret_cast<SDblQueLink*>(&temp);
			SDblQueLink* pLink = temp.Last();

			while (ETrue)
				{
				// traverse the list starting at the back
				if ((pLink != anchor) && (_LOFF(pLink, DShBuf, iObjLink)->iRelAddress > buf->iRelAddress))
					{
					pLink = pLink->iPrev;
					}
				else
					{
					buf->iObjLink.InsertAfter(pLink);
					break;
					}
				}
			}

		// now merge with the free list
		while(!temp.IsEmpty())
			{
			if (iFreeList.IsEmpty())
				{
				iFreeList.MoveFrom(&temp);
				break;
				}

			// working backwards with the highest index
			DShBuf* buf = _LOFF(temp.Last(), DShBuf, iObjLink);
			SDblQueLink* anchor = reinterpret_cast<SDblQueLink*>(&iFreeList);
			SDblQueLink* pLink = iFreeList.Last();

			while (!NKern::FMFlash(&iLock))
				{
				if ((pLink != anchor) && (_LOFF(pLink, DShBuf, iObjLink)->iRelAddress > buf->iRelAddress))
					{
					pLink = pLink->iPrev;
					}
				else
					{
					buf->iObjLink.Deque();
					buf->iObjLink.InsertAfter(pLink);
					// next buffer
					if (temp.IsEmpty())
						break;
					buf = _LOFF(temp.Last(), DShBuf, iObjLink);
					}
				}
			}
		NKern::FMFlash(&iLock);
		}
	UnlockPool();

	__KTRACE_OPT(KMMU, Kern::Printf("<DMemModelNonAlignedShPool::UpdateFreeList"));
	return KErrNone;
	}

void DMemModelNonAlignedShPool::Free(DShBuf* aBuf)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelNonAlignedShPool::Free (aBuf = 0x%08x, aBuf->Base() 0x%08x)", aBuf, aBuf->iRelAddress));

	LockPool();
#ifdef _DEBUG
	// Remove from allocated list
	aBuf->iObjLink.Deque();
#endif

	// we want to put the initial buffers at the head of the free list
	// and the grown buffers at the tail as this makes shrinking more efficient
	if (aBuf >= iInitialBuffersArray && aBuf < (iInitialBuffersArray + iInitialBuffers))
		{
		iFreeList.AddHead(&aBuf->iObjLink);
		}
	else
		{
		iAltFreeList.Add(&aBuf->iObjLink);
		}

	++iFreeBuffers;
#ifdef _DEBUG
	--iAllocatedBuffers;
#endif
	iPoolFlags &= ~EShPoolSuppressShrink;		// Allow shrinking again, if it was blocked
	UnlockPool();

	// queue ManagementDfc which completes notifications as appropriate
	if (HaveWorkToDo())
		KickManagementDfc();

	DShPool::Close(NULL); // decrement pool reference count
	}

// Kernel side API
TInt DMemModelNonAlignedShPool::Alloc(DShBuf*& aShBuf)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DMemModelNonAlignedShPool::Alloc (DShBuf)"));

	aShBuf = NULL;

	LockPool();

	if (!iFreeList.IsEmpty())
		{
		aShBuf = _LOFF(iFreeList.GetFirst(), DShBuf, iObjLink);
#ifdef _DEBUG
		iAllocated.Add(&aShBuf->iObjLink);
		iAllocatedBuffers++;
#endif
		}
	else
		{
		// try alternative free list
		if (!iAltFreeList.IsEmpty())
			{
			aShBuf = _LOFF(iAltFreeList.GetFirst(), DShBuf, iObjLink);
#ifdef _DEBUG
			iAllocated.Add(&aShBuf->iObjLink);
			iAllocatedBuffers++;
#endif
			}
		else
			{
			UnlockPool();
			KickManagementDfc(); // Try to grow
			return KErrNoMemory;
			}
		}

	--iFreeBuffers;
	Open(); // increment pool reference count

	UnlockPool();

	if (HaveWorkToDo())
		KickManagementDfc();

	__KTRACE_OPT(KMMU, Kern::Printf("<DMemModelNonAlignedShPool::Alloc return buf = 0x%08x", aShBuf));
	return KErrNone;
	}
