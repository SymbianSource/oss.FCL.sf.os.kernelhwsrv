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
// e32/memmodel/emul/win32/mshbuf.cpp
// Shareable Data Buffers

#include "memmodel.h"
#include <kernel/smap.h>

_LIT(KLitDWin32ShPool,"DWin32ShPool");
_LIT(KLitDWin32AlignedShPool,"DWin32AlignedShPool");
_LIT(KLitDWin32NonAlignedShPool,"DWin32NonAlignedShPool");


DWin32ShBuf::DWin32ShBuf(DShPool* aPool, TLinAddr aRelAddr) : DShBuf(aPool, aRelAddr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32ShBuf::DWin32ShBuf()"));
	}

DWin32ShBuf::~DWin32ShBuf()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32ShBuf::~DWin32ShBuf()"));
	}

TUint8* DWin32ShBuf::Base(DProcess* aProcess)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32ShBuf::Base(0x%x)", aProcess));

	TUint8* base = reinterpret_cast<DWin32ShPool*>(iPool)->Base(aProcess) + (TUint)iRelAddress;

	return base;
	}

TUint8* DWin32ShBuf::Base()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32ShBuf::Base()"));

	TUint8* base = reinterpret_cast<DWin32ShPool*>(iPool)->Base() + (TUint)iRelAddress;

	return base;
	}

TInt DWin32ShBuf::Map(TUint /* aMapAttr */, DProcess* /* aProcess */, TLinAddr& aBase)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32ShBuf::Map()"));

	TInt r = KErrNotSupported;

	if (iPool->iPoolFlags & EShPoolPageAlignedBuffer)
		{
		if(iMapped)
			{
			r = KErrAlreadyExists;
			}
		else
			{
			aBase = reinterpret_cast<TUint>(reinterpret_cast<DWin32ShPool*>(iPool)->Base() + (TUint)iRelAddress);
			iMapped = ETrue;
			r = KErrNone;
			}
		}

	return r;
	}

TInt DWin32ShBuf::UnMap(DProcess* /* aProcess */)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32ShBuf::UnMap()"));

	TInt r = KErrNotSupported;

	if (iPool->iPoolFlags & EShPoolPageAlignedBuffer)
		{
		if(iMapped)
			{
			iMapped = EFalse;
			r = KErrNone;
			}
		else
			{
			r = KErrNotFound;
			}
		}

	return r;
	}

TInt DWin32ShBuf::AddToProcess(DProcess* aProcess, TUint /* aAttr */)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("Adding DWin32ShBuf %O to process %O", this, aProcess));
	TUint flags;
	TInt r = KErrNone;

	if (aProcess != K::TheKernelProcess)
	    r = iPool->OpenClient(aProcess, flags);

	return r;
	}

TInt DWin32ShBuf::Close(TAny* aPtr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32ShBuf::Close(0x%08x)", aPtr));

	if (aPtr)
		{
		DProcess* pP = reinterpret_cast<DProcess*>(aPtr);

		if (pP != K::TheKernelProcess)
		    iPool->CloseClient(pP);
		}

	return DShBuf::Close(aPtr);
	}

DWin32ShPool::DWin32ShPool()
  : DShPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32ShPool::DWin32ShPool"));
	}


DWin32ShPool::~DWin32ShPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32ShPool::~DWin32ShPool"));

	if (iWin32MemoryBase)
		{
		TUint64 maxSize = static_cast<TUint64>(iMaxBuffers) * static_cast<TUint64>(iBufGap);

		// We know that maxSize is less than KMaxTInt as we tested for this in DoCreate().
		VirtualFree(LPVOID(iWin32MemoryBase), (SIZE_T)maxSize, MEM_DECOMMIT);
		VirtualFree(LPVOID(iWin32MemoryBase), 0, MEM_RELEASE);
		MM::Wait();
		MM::FreeMemory += iWin32MemorySize;
		MM::Signal();
		}

	delete iBufMap;
	}

void DWin32ShPool::DestroyClientResources(DProcess* aProcess)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32ShPool::DestroyClientResources"));

	TInt r = DestroyHandles(aProcess);
	__NK_ASSERT_DEBUG((r == KErrNone) || (r == KErrDied));
	(void)r;		// Silence warnings
	}

TInt DWin32ShPool::DeleteInitialBuffers()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32ShPool::DeleteInitialBuffers"));

	if (iInitialBuffersArray != NULL)
		{
		for (TUint i = 0; i < iInitialBuffers; i++)
			{
			iInitialBuffersArray[i].iObjLink.Deque(); // remove from free list
			iInitialBuffersArray[i].Dec();
			iInitialBuffersArray[i].~DWin32ShBuf();
			}

		Kern::Free(iInitialBuffersArray);
		iInitialBuffersArray = NULL;
		}

	return KErrNone;
	}

TInt DWin32ShPool::DestroyHandles(DProcess* aProcess)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32ShPool::DestroyHandles(0x%08x)", aProcess));

	TInt r = KErrNone;
	Kern::MutexWait(*iProcessLock);
	DShPoolClient* client = reinterpret_cast<DShPoolClient*>(iClientMap->Remove(reinterpret_cast<TUint>(aProcess)));

	__NK_ASSERT_DEBUG(client);
	__NK_ASSERT_DEBUG(client->iAccessCount == 0);

	delete client;

	if (aProcess != K::TheKernelProcess)
		{
		// Remove reserved handles
		r = aProcess->iHandles.Reserve(-TInt(iTotalBuffers));
		}

	Kern::MutexSignal(*iProcessLock);

	return r;
	}


TInt DWin32ShPool::Close(TAny* aPtr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32ShPool::Close(0x%08x)", aPtr));

	if (aPtr) // not NULL must be user side
		{
		DProcess* pP = reinterpret_cast<DProcess*>(aPtr);

		CloseClient(pP);
		}

	return DShPool::Close(aPtr);
	}


TInt DWin32ShPool::CreateInitialBuffers()
	{
	__KTRACE_OPT(KMMU,Kern::Printf(">DWin32ShPool::CreateInitialBuffers"));

	iInitialBuffersArray = reinterpret_cast<DWin32ShBuf*>(Kern::Alloc(iInitialBuffers * sizeof(DWin32ShBuf)));

	if (iInitialBuffersArray == NULL)
		return KErrNoMemory;

	TLinAddr offset = 0;
	for (TUint i = 0; i < iInitialBuffers; i++)
		{
		DWin32ShBuf *buf = new (&iInitialBuffersArray[i]) DWin32ShBuf(this, offset);
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

	iFreeBuffers = iInitialBuffers;
	iTotalBuffers = iInitialBuffers;

	iBufMap->Alloc(0, iInitialBuffers);

	return KErrNone;
	}


TUint8* DWin32ShPool::Base()
	{
	return iWin32MemoryBase;
	}


TUint8* DWin32ShPool::Base(DProcess* /*aProcess*/)
	{
	return iWin32MemoryBase;
	}


TInt DWin32ShPool::AddToProcess(DProcess* aProcess, TUint aAttr)
	{
	__KTRACE_OPT(KEXEC, Kern::Printf("Adding DWin32ShPool %O to process %O", this, aProcess));

	TInt r = KErrNone;

	Kern::MutexWait(*iProcessLock);
	LockPool();
	DShPoolClient* client = reinterpret_cast<DShPoolClient*>(iClientMap->Find(reinterpret_cast<TUint>(aProcess)));
	UnlockPool();

	if (!client)
		{
		client = new DShPoolClient;

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


TInt DWin32ShPool::DoCreate(TShPoolCreateInfo& aInfo)
	{
	TUint64 maxSize = static_cast<TUint64>(aInfo.iInfo.iMaxBufs) * static_cast<TUint64>(iBufGap);

	if (maxSize > static_cast<TUint64>(KMaxTInt))
		{
		return KErrArgument;
		}

	__KTRACE_OPT(KMMU,Kern::Printf("DWin32ShPool::DoCreate (maxSize = 0x%08x, iBufGap = 0x%08x)",
		static_cast<TInt>(maxSize), iBufGap));

	iWin32MemoryBase = (TUint8*) VirtualAlloc(NULL, (SIZE_T)maxSize, MEM_RESERVE, PAGE_READWRITE);
	if (iWin32MemoryBase == NULL)
		{
		return KErrNoMemory;
		}

	__KTRACE_OPT(KMMU,Kern::Printf("DWin32ShPool::DoCreate (iWin32MemoryBase = 0x%08x)", iWin32MemoryBase));

	iBufMap = TBitMapAllocator::New(aInfo.iInfo.iMaxBufs, (TBool)ETrue);
	if (iBufMap == NULL)
		{
		return KErrNoMemory;
		}

	return KErrNone;
	}


TBool DWin32ShPool::IsOpen(DProcess* /*aProcess*/)
	{
	// could do we some kind of check here?
	return (TBool)ETrue;
	}


TInt DWin32ShPool::UpdateFreeList()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32ShPool::UpdateFreeList"));

	SDblQue temp;
	SDblQueLink* anchor = reinterpret_cast<SDblQueLink*>(&iFreeList);

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

			for (;;)
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

	__KTRACE_OPT(KMMU, Kern::Printf("<DWin32ShPool::UpdateFreeList"));
	return KErrNone;
	}


void DWin32ShPool::Free(DShBuf* aBuf)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32ShPool::Free (aBuf = 0x%08x, aBuf->Base() 0x%08x)", aBuf, aBuf->Base()));

	TLinAddr newAddr = (TLinAddr)aBuf->Base();
#ifdef _DEBUG
	memset((TAny*)newAddr,0xde,aBuf->Size());
#else
	memclr((TAny*)newAddr,aBuf->Size());
#endif

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

	Close(NULL); // decrement pool reference count
	}

// Kernel side API
TInt DWin32ShPool::Alloc(DShBuf*& aShBuf)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32ShPool::Alloc (DShBuf)"));

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
			--iFreeBuffers;
			Open(); // increment pool reference count
			r = KErrNone;
			}
		}

	UnlockPool();

	if (HaveWorkToDo())
		KickManagementDfc();

	__KTRACE_OPT(KMMU, Kern::Printf("<DWin32ShPool::Alloc return buf = 0x%08x", aShBuf));
	return r;
	}


DWin32AlignedShPool::DWin32AlignedShPool()
  : DWin32ShPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32AlignedShPool::DWin32AlignedShPool"));
	}


DWin32AlignedShPool::~DWin32AlignedShPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32AlignedShPool::~DWin32AlignedShPool"));
	}


TInt DWin32AlignedShPool::DoCreate(TShPoolCreateInfo& aInfo)
	{
	TInt r;
	// Create Chunk
	r = DWin32ShPool::DoCreate(aInfo);
	if (r != KErrNone)
		{
		return r;
		}

	if (iPoolFlags & EShPoolGuardPages)
		{
		TUint numOfBytes = iBufGap - MM::RamPageSize;
		iCommittedPages = MM::RoundToPageSize(iInitialBuffers * numOfBytes) >> MM::RamPageShift;

		for (TUint i = 0; i < iInitialBuffers; ++i)
			{
			TUint offset = iBufGap * i;

			MM::Wait();
			if (MM::Commit(reinterpret_cast<TLinAddr>(iWin32MemoryBase+offset), numOfBytes, 0xFF, EFalse) != KErrNone)
				{
				MM::Signal();
				return KErrNoMemory;
				}
			iWin32MemorySize += numOfBytes;

			MM::Signal();
			}

		iMaxPages = MM::RoundToPageSize(aInfo.iInfo.iMaxBufs * numOfBytes) >> MM::RamPageShift;
		}
	else
		{
		// Make sure we give the caller the number of buffers they were expecting
		iCommittedPages = MM::RoundToPageSize(iInitialBuffers * iBufGap) >> MM::RamPageShift;
		MM::Wait();
		if (MM::Commit(reinterpret_cast<TLinAddr>(iWin32MemoryBase), iCommittedPages << MM::RamPageShift, 0xFF, EFalse) != KErrNone)
			{
			MM::Signal();
			return KErrNoMemory;
			}
		iWin32MemorySize = iCommittedPages << MM::RamPageShift;

		MM::Signal();

		iMaxPages = MM::RoundToPageSize(aInfo.iInfo.iMaxBufs * iBufGap) >> MM::RamPageShift;
		}

	return r;
	}


TInt DWin32AlignedShPool::SetBufferWindow(DProcess* /*aProcess*/, TInt /*aWindowSize*/ )
	{
	return KErrNone;
	}


TInt DWin32AlignedShPool::GrowPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32AlignedShPool::GrowPool()"));

	Kern::MutexWait(*iProcessLock);

	// How many bytes to commit for each new buffer (must be whole number of pages)
	TUint bytes = (iPoolFlags & EShPoolGuardPages) ? iBufGap - MM::RamPageSize : iBufGap;

	__ASSERT_DEBUG(!(bytes % MM::RamPageSize), Kern::PanicCurrentThread(KLitDWin32AlignedShPool, __LINE__));

	TInt pages = bytes >> MM::RamPageShift;

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

		MM::Wait();
		if (MM::Commit(reinterpret_cast<TLinAddr>(iWin32MemoryBase+offset), bytes, 0xFF, EFalse) != KErrNone)
			{
			r = KErrNoMemory;
			}
		iWin32MemorySize += bytes;
		MM::Signal();

		if (r != KErrNone)
			{
			iBufMap->Free(offset / iBufGap);
			break;
			}

		DWin32ShBuf *buf = new DWin32ShBuf(this, offset);

		if (buf == NULL)
			{
			MM::Wait();
			MM::Decommit(reinterpret_cast<TLinAddr>(iWin32MemoryBase+offset), bytes);
			iWin32MemorySize -= bytes;
			MM::Signal();
			iBufMap->Free(offset / iBufGap);
			r = KErrNoMemory;
			break;
			}

		TInt r = buf->Construct();

		if (r != KErrNone)
			{
			MM::Wait();
			MM::Decommit(reinterpret_cast<TLinAddr>(iWin32MemoryBase+offset), bytes);
			iWin32MemorySize -= bytes;
			MM::Signal();
			iBufMap->Free(offset / iBufGap);
			buf->DObject::Close(NULL);
			break;
			}

		iCommittedPages += pages;

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
		// else delete buffers
		SDblQueLink *pLink;
		while ((pLink = temp.GetFirst()) != NULL)
			{
			DShBuf* buf = _LOFF(pLink, DShBuf, iObjLink);
			TLinAddr offset = buf->iRelAddress;
			iBufMap->Free(offset / iBufGap);
			MM::Wait();
			MM::Decommit(reinterpret_cast<TLinAddr>(iWin32MemoryBase+offset), bytes);
			iWin32MemorySize -= bytes;
			MM::Signal();
			iCommittedPages -= pages;
			buf->DObject::Close(NULL);
			}
		}

	CalculateGrowShrinkTriggers();

	Kern::MutexSignal(*iProcessLock);

	__KTRACE_OPT(KMMU, Kern::Printf("<DWin32AlignedShPool::GrowPool()"));
	return r;
	} // DWin32AlignedShPool::GrowPool


TInt DWin32AlignedShPool::ShrinkPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32AlignedShPool::ShrinkPool()"));

	Kern::MutexWait(*iProcessLock);

	// How many bytes to commit for each new buffer (must be whole number of pages)
	TUint bytes = (iPoolFlags & EShPoolGuardPages) ? iBufGap - MM::RamPageSize : iBufGap;

	__ASSERT_DEBUG(!(bytes % MM::RamPageSize), Kern::PanicCurrentThread(KLitDWin32AlignedShPool, __LINE__));

	TInt pages = bytes >> MM::RamPageShift;

	// Grab pool stats
	TUint32 grownBy = iTotalBuffers - iInitialBuffers;

	// How many buffers to shrink by?
	TUint32 shrink = mult_fx248(iTotalBuffers, iShrinkByRatio);
	if (shrink == 0)		// Handle round-to-zero
		shrink = 1;
	if (shrink > grownBy)
		shrink = grownBy;
	if (shrink > iFreeBuffers)
		shrink = iFreeBuffers;

	// work backwards
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
		iCommittedPages -= pages;
		UnlockPool();

		TLinAddr offset = pBuf->iRelAddress;

		iBufMap->Free(offset / iBufGap);

		MM::Wait();
		MM::Decommit(reinterpret_cast<TLinAddr>(iWin32MemoryBase+offset), iBufSize);
		iWin32MemorySize -= iBufSize;
		MM::Signal();
		pBuf->DObject::Close(NULL);
		}

	TInt r = UpdateReservedHandles(-(TInt)i);

	// If we couldn't shrink the pool by this many buffers, wait until we Free() another
	// buffer before trying to shrink again.
	if (i < shrink)
		iPoolFlags |= EShPoolSuppressShrink;

	CalculateGrowShrinkTriggers();

	Kern::MutexSignal(*iProcessLock);

	__KTRACE_OPT(KMMU, Kern::Printf("<DWin32AlignedShPool::ShrinkPool()"));
	return r;
	} // DWin32AlignedShPool::ShrinkPool


DWin32NonAlignedShPool::DWin32NonAlignedShPool()
  : DWin32ShPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32NonAlignedShPool::DWin32NonAlignedShPool"));
	}


DWin32NonAlignedShPool::~DWin32NonAlignedShPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32NonAlignedShPool::~DWin32NonAlignedShPool"));

	delete iPagesMap;
	}


TInt DWin32NonAlignedShPool::DoCreate(TShPoolCreateInfo& aInfo)
	{
	// Create Chunk
	TInt r;

	r = DWin32ShPool::DoCreate(aInfo);

	if (r != KErrNone)
		{
		return r;
		}

	if (iPoolFlags & EShPoolPhysicalMemoryPool)
		{
		return KErrNotSupported;
		}
	else
		{
		// Make sure we give the caller the number of buffers they were expecting
		iCommittedPages = MM::RoundToPageSize(iInitialBuffers * iBufGap) >> MM::RamPageShift;

		MM::Wait();
		if (MM::Commit(reinterpret_cast<TLinAddr>(iWin32MemoryBase), iCommittedPages << MM::RamPageShift, 0xFF, EFalse) != KErrNone)
			{
			MM::Signal();
			return KErrNoMemory;
			}
		iWin32MemorySize = iCommittedPages << MM::RamPageShift;

		MM::Signal();
		iMaxPages = MM::RoundToPageSize(aInfo.iInfo.iMaxBufs * iBufGap) >> MM::RamPageShift;
		}

	iPagesMap = TBitMapAllocator::New(iMaxPages, (TBool)ETrue);

	if(!iPagesMap)
		{
		return KErrNoMemory;
		}

	iPagesMap->Alloc(0, iCommittedPages);
	return r;
	}


void DWin32NonAlignedShPool::FreeBufferPages(TUint aOffset)
	{
	TLinAddr firstByte = aOffset;	// offset of first byte in buffer
	TLinAddr lastByte = firstByte+iBufGap-1;	// offset of last byte in buffer
	TUint firstPage = firstByte>>MM::RamPageShift;	// index of first page containing part of the buffer
	TUint lastPage = lastByte>>MM::RamPageShift;		// index of last page containing part of the buffer

	TUint firstBuffer = (firstByte&~(MM::RamPageSize - 1))/iBufGap; // index of first buffer which lies in firstPage
	TUint lastBuffer = (lastByte|(MM::RamPageSize - 1))/iBufGap;    // index of last buffer which lies in lastPage
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
		MM::Wait();
		MM::Decommit(reinterpret_cast<TLinAddr>(iWin32MemoryBase+(firstPage << MM::RamPageShift)), (numPages << MM::RamPageShift));
		iWin32MemorySize -= (numPages << MM::RamPageShift);
		MM::Signal();
		iCommittedPages -= numPages;
		}
	}


TInt DWin32NonAlignedShPool::GrowPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32NonAlignedShPool::GrowPool()"));

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

		TInt lastPage = (offset + iBufSize - 1) >> MM::RamPageShift;

		// Allocate one page at a time.
		for (TInt page = offset >> MM::RamPageShift; page <= lastPage; ++page)
			{
			// Is the page allocated?
			if (iPagesMap->NotAllocated(page, 1))
				{
				MM::Wait();
				if (MM::Commit(reinterpret_cast<TLinAddr>(iWin32MemoryBase+(page << MM::RamPageShift)), MM::RamPageSize, 0xFF, EFalse) != KErrNone)
					{
					MM::Signal();
					r = KErrNoMemory;
					break;
					}
				iWin32MemorySize += MM::RamPageSize;

				MM::Signal();
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

		DWin32ShBuf *buf = new DWin32ShBuf(this, offset);

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

	__KTRACE_OPT(KMMU, Kern::Printf("<DWin32NonAlignedShPool::GrowPool()"));
	return r;
	} // DWin32NonAlignedShPool::GrowPool


TInt DWin32NonAlignedShPool::ShrinkPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DWin32NonAlignedShPool::ShrinkPool()"));

	Kern::MutexWait(*iProcessLock);

	// Grab pool stats
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

	__KTRACE_OPT(KMMU, Kern::Printf("<DWin32NonAlignedShPool::ShrinkPool()"));

	return KErrNone;
	} // DWin32NonAlignedShPool::ShrinkPool
