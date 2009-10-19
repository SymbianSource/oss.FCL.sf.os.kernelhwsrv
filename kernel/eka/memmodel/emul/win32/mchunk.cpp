// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\emul\win32\mchunk.cpp
// 
//

#include "memmodel.h"
#include <emulator.h>

DWin32Chunk::~DWin32Chunk()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DWin32Chunk destruct %O",this));

	if (iBase)
		{
		VirtualFree(LPVOID(iBase), iMaxSize, MEM_DECOMMIT);
		VirtualFree(LPVOID(iBase), 0, MEM_RELEASE);
		MM::Wait();
		MM::FreeMemory += iSize;
		if(iUnlockedPageBitMap)
			{
			TInt unlockedMemory = MM::RamPageSize*(iUnlockedPageBitMap->iSize-iUnlockedPageBitMap->iAvail);
			if(unlockedMemory<=MM::CacheMemory)
				MM::CacheMemory-=unlockedMemory;
			else
				{
				MM::ReclaimedCacheMemory -= unlockedMemory-MM::CacheMemory;
				MM::CacheMemory = 0;
				}
			MM::CheckMemoryCounters();
			}
		MM::Signal();
		}
	__KTRACE_OPT(KMEMTRACE, {MM::Wait();Kern::Printf("MT:D %d %x %O",NTickCount(),this,this);MM::Signal();});
#ifdef BTRACE_CHUNKS
	BTraceContext4(BTrace::EChunks,BTrace::EChunkDestroyed,this);
#endif
	delete iPageBitMap;
	delete iUnlockedPageBitMap;
	delete iPermanentPageBitMap;

	TDfc* dfc = (TDfc*)__e32_atomic_swp_ord_ptr(&iDestroyedDfc, 0);
	if (dfc)
		dfc->Enque();
	}


TUint8* DWin32Chunk::Base(DProcess* /*aProcess*/)
	{
	return iBase;
	}


TInt DWin32Chunk::DoCreate(SChunkCreateInfo& aInfo)
	{
	__ASSERT_COMPILE(!(EMMChunkAttributesMask & EChunkAttributesMask));

	if(iAttributes&EMemoryNotOwned)
		return KErrNotSupported;
	if (aInfo.iMaxSize<=0)
		return KErrArgument;
	iMaxSize=MM::RoundToChunkSize(aInfo.iMaxSize);
	TInt maxpages=iMaxSize>>MM::RamPageShift;
	if (iAttributes & EDisconnected)
		{
		TBitMapAllocator* pM=TBitMapAllocator::New(maxpages,ETrue);
		if (!pM)
			return KErrNoMemory;
		TBitMapAllocator* pUM=TBitMapAllocator::New(maxpages,ETrue);
		if (!pUM)
			{
			delete pM;
			return KErrNoMemory;
			}
		iPageBitMap=pM;
		iUnlockedPageBitMap=pUM;
		__KTRACE_OPT(KMMU,Kern::Printf("PageBitMap at %08x, MaxPages %d",pM,maxpages));
		}
	switch (iChunkType)
		{
	case ESharedKernelSingle:
	case ESharedKernelMultiple:
		{
		TBitMapAllocator* pM=TBitMapAllocator::New(maxpages,ETrue);
		if (!pM)
			return KErrNoMemory;
		iPermanentPageBitMap = pM;
		}
		// fall through to next case...
	case ESharedIo:
	case EKernelMessage:
	case EUserSelfModCode:
	case EUserData:
		{
		DWORD protect = (iChunkType == EUserSelfModCode) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
		LPVOID base = VirtualAlloc(NULL, iMaxSize, MEM_RESERVE, protect);
		if (!base)
			return KErrNoMemory;
		iBase  = (TUint8*) base;
		__KTRACE_OPT(KMMU,Kern::Printf("Reserved: Base=%08x, Size=%08x",iBase,iMaxSize));
		}
		break;
	default:
		break;
		}
	__KTRACE_OPT(KMEMTRACE, {MM::Wait();Kern::Printf("MT:C %d %x %O",NTickCount(),this,this);MM::Signal();});
#ifdef BTRACE_CHUNKS
	TKName nameBuf;
	Name(nameBuf);
	BTraceContextN(BTrace::EChunks,BTrace::EChunkCreated,this,iMaxSize,nameBuf.Ptr(),nameBuf.Size());
	if(iOwningProcess)
		BTrace8(BTrace::EChunks,BTrace::EChunkOwner,this,iOwningProcess);
	BTraceContext12(BTrace::EChunks,BTrace::EChunkInfo,this,iChunkType,iAttributes);
#endif
	return KErrNone;
	}

TInt DWin32Chunk::Adjust(TInt aNewSize)
//
// Adjust a standard chunk.
//
	{

	__KTRACE_OPT(KMMU,Kern::Printf("DWin32Chunk::Adjust %08x",aNewSize));
	if (iAttributes & (EDoubleEnded|EDisconnected))
		return KErrGeneral;
	if (aNewSize<0 || aNewSize>iMaxSize)
		return KErrArgument;

	TInt r=KErrNone;
	TInt newSize=MM::RoundToPageSize(aNewSize);
	if (newSize!=iSize)
		{
		MM::Wait();
		if (newSize>iSize)
			{
			__KTRACE_OPT(KMMU,Kern::Printf("DWin32Chunk::Adjust growing"));
			r=DoCommit(iSize,newSize-iSize);
			}
		else if (newSize<iSize)
			{
			__KTRACE_OPT(KMMU,Kern::Printf("DWin32Chunk::Adjust shrinking"));
			DoDecommit(newSize,iSize-newSize);
			}
		MM::Signal();
		}

	__COND_DEBUG_EVENT(r==KErrNone, EEventUpdateChunk, this);
	__KTRACE_OPT(KMMU,Kern::Printf("DWin32Chunk %O adjusted to %x",this,iSize));
	return r;
	}

TInt DWin32Chunk::AdjustDoubleEnded(TInt aBottom, TInt aTop)
//
// Adjust a double-ended chunk.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DWin32Chunk::AdjustDoubleEnded %x-%x",aBottom,aTop));
	if ((iAttributes & (EDoubleEnded|EDisconnected))!=EDoubleEnded)
		return KErrGeneral;
	if (0>aBottom || aBottom>aTop || aTop>iMaxSize)
		return KErrArgument;
	aBottom &= ~(MM::RamPageSize-1);
	aTop = MM::RoundToPageSize(aTop);
	TInt newSize=aTop-aBottom;

	MM::Wait();
	TInt initBottom=iStartPos;
	TInt initTop=iStartPos+iSize;
	TInt nBottom=Max(aBottom,initBottom);	// intersection bottom
	TInt nTop=Min(aTop,initTop);	// intersection top
	TInt r=KErrNone;
	if (nBottom<nTop)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("Initial and final regions intersect"));
		if (initBottom<nBottom)
			{
			iStartPos=aBottom;
			DoDecommit(initBottom,nBottom-initBottom);
			}
		if (initTop>nTop)
			DoDecommit(nTop,initTop-nTop);	// this changes iSize
		if (aBottom<nBottom)
			{
			r=DoCommit(aBottom,nBottom-aBottom);
			if (r==KErrNone)
				{
				if (aTop>nTop)
					r=DoCommit(nTop,aTop-nTop);
				if (r==KErrNone)
					iStartPos=aBottom;
				else
					DoDecommit(aBottom,nBottom-aBottom);
				}
			}
		else if (aTop>nTop)
			r=DoCommit(nTop,aTop-nTop);
		}
	else
		{
		__KTRACE_OPT(KMMU,Kern::Printf("Initial and final regions disjoint"));
		if (iSize)
			DoDecommit(initBottom,iSize);
		iStartPos=aBottom;
		if (newSize)
			r=DoCommit(iStartPos,newSize);
		}
	MM::Signal();
	__COND_DEBUG_EVENT(r==KErrNone, EEventUpdateChunk, this);
	__KTRACE_OPT(KMMU,Kern::Printf("DWin32Chunk %O adjusted to %x+%x",this,iStartPos,iSize));
	return r;
	}


TInt DWin32Chunk::Commit(TInt aOffset, TInt aSize, TCommitType aCommitType, TUint32* aExtraArg)
//
// Commit to a disconnected chunk.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DWin32Chunk::Commit %x+%x type=%d extra=%08x",aOffset,aSize,aCommitType,aExtraArg));
	if ((iAttributes & (EDoubleEnded|EDisconnected))!=EDisconnected)
		return KErrGeneral;
	if (aOffset<0 || aSize<0 || (aOffset+aSize)>iMaxSize)
		return KErrArgument;
	if(LOGICAL_XOR((TInt)aCommitType&DChunk::ECommitPhysicalMask, iAttributes&DChunk::EMemoryNotOwned))
		return KErrNotSupported;  // Commit type doesn't match 'memory owned' type

	TInt top = MM::RoundToPageSize(aOffset + aSize);
	aOffset &= ~(MM::RamPageSize - 1);
	aSize = top - aOffset;

	TInt r=KErrNone;
	TInt i=aOffset>>MM::RamPageShift;
	TInt n=aSize>>MM::RamPageShift;
	MM::Wait();
	if (iPageBitMap->NotFree(i,n))
		r=KErrAlreadyExists;
	else
		{
		switch(aCommitType)
			{
		case DChunk::ECommitDiscontiguous:
			if(aExtraArg==0)
				r=DoCommit(aOffset,aSize);
			else
				r = KErrArgument;
			break;

		case DChunk::ECommitContiguous:
			r=DoCommit(aOffset,aSize);
			 // Return a fake physical address which is == linear address
			if(r==KErrNone)
				*aExtraArg = (TUint)(iBase+aOffset);
			break;

		case DChunk::ECommitDiscontiguousPhysical:
		case DChunk::ECommitContiguousPhysical:
			// The emulator doesn't do physical address allocation
			r=KErrNotSupported;
			break;

		default:
			r = KErrArgument;
			break;
			};
		if (r==KErrNone)
			iPageBitMap->Alloc(i,n);
		}
	MM::CheckMemoryCounters();
	MM::Signal();
	__COND_DEBUG_EVENT(r==KErrNone, EEventUpdateChunk, this);
	return r;
	}

TInt DWin32Chunk::Allocate(TInt aSize, TInt aGuard, TInt aAlign)
//
// Allocate offset and commit to a disconnected chunk.
//
	{
	(void)aAlign;
	__KTRACE_OPT(KMMU,Kern::Printf("DWin32Chunk::Allocate %x %x %d",aSize,aGuard,aAlign));
	if ((iAttributes & (EDoubleEnded|EDisconnected))!=EDisconnected)
		return KErrGeneral;
	if (aSize<=0 || aGuard<0 || aSize+aGuard>iMaxSize)
		return KErrArgument;

	aSize = MM::RoundToPageSize(aSize);
	aGuard = MM::RoundToPageSize(aGuard);

	TInt r=KErrNone;
	TInt n=(aSize+aGuard)>>MM::RamPageShift;
	MM::Wait();
	TInt i=iPageBitMap->AllocConsecutive(n,EFalse);		// allocate the offset
	if (i<0)
		r=KErrNoMemory;		// run out of reserved space for this chunk
	else
		{
		TInt offset=i<<MM::RamPageShift;
		__KTRACE_OPT(KMMU,Kern::Printf("Offset %x allocated",offset));
		r=DoCommit(offset+aGuard,aSize);
		if (r==KErrNone)
			{
			iPageBitMap->Alloc(i,n);
			r=offset;		// if operation successful, return allocated offset
			}
		}
	MM::Signal();
	__KTRACE_OPT(KMMU,Kern::Printf("DWin32Chunk::Allocate returns %x",r));
	__COND_DEBUG_EVENT(r==KErrNone, EEventUpdateChunk, this);
	return r;
	}

TInt DWin32Chunk::Decommit(TInt anOffset, TInt aSize)
//
// Decommit from a disconnected chunk.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DWin32Chunk::Decommit %x+%x",anOffset,aSize));
	if ((iAttributes & (EDoubleEnded|EDisconnected))!=EDisconnected)
		return KErrGeneral;
	if (anOffset<0 || aSize<0 || (anOffset+aSize)>iMaxSize)
		return KErrArgument;
	
	TInt top = MM::RoundToPageSize(anOffset + aSize);
	anOffset &= ~(MM::RamPageSize - 1);
	aSize = top - anOffset;

	MM::Wait();

	// limit the range to the home region range
	__KTRACE_OPT(KMMU,Kern::Printf("Rounded and Clipped range %x+%x",anOffset,aSize));

	TInt i=anOffset>>MM::RamPageShift;
	TInt n=aSize>>MM::RamPageShift;
	// check for decommiting unlocked pages...
	for(TInt j=i; j<i+n; j++)
		{
		if(iUnlockedPageBitMap->NotFree(j,1))
			{
			iUnlockedPageBitMap->Free(j);
			if(MM::ReclaimedCacheMemory)
				{
				MM::ReclaimedCacheMemory -= MM::RamPageSize;
				MM::FreeMemory -= MM::RamPageSize; // reclaimed memory already counted, so adjust
				}
			else
				MM::CacheMemory -= MM::RamPageSize;
			}
		}
	__KTRACE_OPT(KMMU,Kern::Printf("Calling SelectiveFree(%d,%d)",i,n));
	iPageBitMap->SelectiveFree(i,n);	// free those positions which are actually allocated
	DoDecommit(anOffset,aSize);
	MM::CheckMemoryCounters();
	MM::Signal();
	__DEBUG_EVENT(EEventUpdateChunk, this);
	return KErrNone;
	}

TInt DWin32Chunk::Unlock(TInt aOffset, TInt aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DWin32Chunk::Unlock %x+%x",aOffset,aSize));
	if (!(iAttributes&ECache))
		return KErrGeneral;
	if ((iAttributes & (EDoubleEnded|EDisconnected))!=EDisconnected)
		return KErrGeneral;
	if (aOffset<0 || aSize<0 || (aOffset+aSize)>iMaxSize)
		return KErrArgument;
	
	TInt top = MM::RoundToPageSize(aOffset + aSize);
	aOffset &= ~(MM::RamPageSize - 1);
	aSize = top - aOffset;

	MM::Wait();

	TInt i=aOffset>>MM::RamPageShift;
	TInt n=aSize>>MM::RamPageShift;
	TInt r;
	if (iPageBitMap->NotAllocated(i,n))
		r=KErrNotFound; // some pages aren't committed
	else
		{
		for(TInt j=i; j<i+n; j++)
			{
			if(iUnlockedPageBitMap->NotAllocated(j,1))
				{
				// unlock this page...
				iUnlockedPageBitMap->Alloc(j,1);
				MM::CacheMemory += MM::RamPageSize;
				}
			}
		r = KErrNone;
		}

	MM::CheckMemoryCounters();
	MM::Signal();
	return r;
	}

TInt DWin32Chunk::Lock(TInt aOffset, TInt aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DWin32Chunk::Lock %x+%x",aOffset,aSize));
	if (!(iAttributes&ECache))
		return KErrGeneral;
	if ((iAttributes & (EDoubleEnded|EDisconnected))!=EDisconnected)
		return KErrGeneral;
	if (aOffset<0 || aSize<0 || (aOffset+aSize)>iMaxSize)
		return KErrArgument;
	
	TInt top = MM::RoundToPageSize(aOffset + aSize);
	aOffset &= ~(MM::RamPageSize - 1);
	aSize = top - aOffset;

	MM::Wait();

	TInt i=aOffset>>MM::RamPageShift;
	TInt n=aSize>>MM::RamPageShift;
	TInt r;
	if (iPageBitMap->NotAllocated(i,n))
		r=KErrNotFound; // some pages aren't committed
	else
		{
		r = KErrNone;
		for(TInt j=i; j<i+n; j++)
			{
			if(iUnlockedPageBitMap->NotFree(j,1))
				{
				// lock this page...
				if(MM::ReclaimedCacheMemory)
					{
					r = KErrNotFound;
					break;
					}
				iUnlockedPageBitMap->Free(j);
				MM::CacheMemory -= MM::RamPageSize;
				}
			}
		}
	if(r!=KErrNone)
		{
		// decommit memory on error...
		for(TInt j=i; j<i+n; j++)
			{
			if(iUnlockedPageBitMap->NotFree(j,1))
				{
				iUnlockedPageBitMap->Free(j);
				if(MM::ReclaimedCacheMemory)
					{
					MM::ReclaimedCacheMemory -= MM::RamPageSize;
					MM::FreeMemory -= MM::RamPageSize; // reclaimed memory already counted, so adjust
					}
				else
					MM::CacheMemory -= MM::RamPageSize;
				}
			}
		iPageBitMap->SelectiveFree(i,n);
		DoDecommit(aOffset,aSize);
		}
	MM::CheckMemoryCounters();
	MM::Signal();
	return r;
	}

TInt DWin32Chunk::CheckAccess()
	{
	DProcess* pP=TheCurrentThread->iOwningProcess;
	if (iAttributes&EPrivate)
		{
		if (iOwningProcess && iOwningProcess!=pP && pP!=K::TheKernelProcess)
			return KErrAccessDenied;
		}
	return KErrNone;
	}

TInt DWin32Chunk::Address(TInt aOffset, TInt aSize, TLinAddr& aKernelAddress)
	{
	if(!iPermanentPageBitMap)
		return KErrAccessDenied;
	if(TUint(aOffset)>=TUint(iMaxSize))
		return KErrArgument;
	if(TUint(aOffset+aSize)>TUint(iMaxSize))
		return KErrArgument;
	if(aSize<=0)
		return KErrArgument;
	TInt pageShift = MM::RamPageShift;
	TInt start = aOffset>>pageShift;
	TInt size = ((aOffset+aSize-1)>>pageShift)-start+1;
	if(iPermanentPageBitMap->NotAllocated(start,size))
		return KErrNotFound;
	aKernelAddress = (TLinAddr)iBase+aOffset;
	return KErrNone;
	}

void DWin32Chunk::Substitute(TInt /*aOffset*/, TPhysAddr /*aOldAddr*/, TPhysAddr /*aNewAddr*/)
	{
	MM::Panic(MM::ENotSupportedOnEmulator);
	}

TInt DWin32Chunk::PhysicalAddress(TInt aOffset, TInt aSize, TLinAddr& aKernelAddress, TUint32& aPhysicalAddress, TUint32* aPhysicalPageList)
	{
	TInt r=Address(aOffset,aSize,aKernelAddress);
	if(r!=KErrNone)
		return r;

	// return fake physical addresses which are the same as the linear address
	aPhysicalAddress = 	aKernelAddress;

	TInt pageShift = MM::RamPageShift;
	TUint32 page = aKernelAddress>>pageShift<<pageShift;
	TUint32 lastPage = (aKernelAddress+aSize-1)>>pageShift<<pageShift;
	TUint32* pageList = aPhysicalPageList;
	TUint32 pageSize = 1<<pageShift;
	if(pageList)
		for(; page<=lastPage; page += pageSize)
			*pageList++ = page;
	return KErrNone;
	}

TInt DWin32Chunk::DoCommit(TInt aOffset, TInt aSize)
//
// Get win32 to commit the pages.
// We know they are not already committed - this is guaranteed by the caller so we can update the memory info easily
//
	{
	if (aSize==0)
		return KErrNone;

	TBool execute = (iChunkType == EUserSelfModCode) ? ETrue : EFalse;

	TInt r = MM::Commit(reinterpret_cast<TLinAddr>(iBase + aOffset), aSize, iClearByte, execute);

	if (r == KErrNone)
		{
		iSize += aSize;

		if(iPermanentPageBitMap)
	        iPermanentPageBitMap->Alloc(aOffset>>MM::RamPageShift,aSize>>MM::RamPageShift);

		__KTRACE_OPT(KMEMTRACE, {Kern::Printf("MT:A %d %x %x %O",NTickCount(),this,iSize,this);});
#ifdef BTRACE_CHUNKS
		BTraceContext12(BTrace::EChunks,BTrace::EChunkMemoryAllocated,this,aOffset,aSize);
#endif
		return KErrNone;
		}

	return KErrNoMemory;
	}

void DWin32Chunk::DoDecommit(TInt anOffset, TInt aSize)
//
// Get win32 to decommit the pages.
// The pages may or may not be committed: we need to find out which ones are so that the memory info is updated correctly
//
	{
	TInt freed = MM::Decommit(reinterpret_cast<TLinAddr>(iBase+anOffset), aSize);

	iSize -= freed;
	__KTRACE_OPT(KMEMTRACE, {Kern::Printf("MT:A %d %x %x %O",NTickCount(),this,iSize,this);});
	}

TUint32 MM::RoundToChunkSize(TUint32 aSize)
	{
	TUint32 m=MM::RamChunkSize-1;
	return (aSize+m)&~m;
	}

void DWin32Chunk::BTracePrime(TInt aCategory)
	{
	DChunk::BTracePrime(aCategory);
	
#ifdef BTRACE_CHUNKS
	if (aCategory == BTrace::EChunks || aCategory == -1)
		{
		MM::Wait();
		// it is essential that the following code is in braces because __LOCK_HOST
		// creates an object which must be destroyed before the MM::Signal() at the end.
			{
			__LOCK_HOST;
			// output traces for all memory which has been committed to the chunk...
			TInt offset=0;
			while(offset<iMaxSize)
				{
				MEMORY_BASIC_INFORMATION info;
				VirtualQuery(LPVOID(iBase + offset), &info, sizeof(info));
				TUint size = Min(iMaxSize-offset, info.RegionSize);
				if(info.State == MEM_COMMIT)
					BTrace12(BTrace::EChunks, BTrace::EChunkMemoryAllocated,this,offset,size);
				offset += size;
				}
			}
			MM::Signal();
		}
#endif
	}
