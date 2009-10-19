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
// e32\memmodel\epoc\direct\mchunk.cpp
// 
//

#include <memmodel.h>

DMemModelChunk::~DMemModelChunk()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DMemModelChunk destruct %O",this));
	if (iRegionSize)
		{
		MM::WaitRamAlloc();
		MM::FreeRegion(iRegionBase,iRegionSize);
		__KTRACE_OPT(KMEMTRACE, Kern::Printf("MT:D %d %x %O",NTickCount(),this,this););
		MM::SignalRamAlloc();
#ifdef BTRACE_CHUNKS
		BTraceContext4(BTrace::EChunks,BTrace::EChunkDestroyed,this);
#endif
		}
	iRegionSize=0;

	TDfc* dfc = (TDfc*)__e32_atomic_swp_ord_ptr(&iDestroyedDfc, 0);
	if(dfc)
		dfc->Enque();
	}


TUint8* DMemModelChunk::Base(DProcess* aProcess)
	{
	return iBase;
	}


TInt DMemModelChunk::DoCreate(SChunkCreateInfo& anInfo)
	{
	__ASSERT_COMPILE(!(EMMChunkAttributesMask & EChunkAttributesMask));

	if(iAttributes&EMemoryNotOwned)
		return KErrNotSupported;
	if (anInfo.iMaxSize<=0)
		return KErrArgument;
	TInt r=KErrNone;
	iMaxSize=MM::RoundToBlockSize(anInfo.iMaxSize);
	switch (anInfo.iType)
		{
		case EDll:
		case EUserCode:
		case EUserSelfModCode:
		case EUserData:
		case EDllData:
		case ESharedKernelSingle:
		case ESharedKernelMultiple:
		case ESharedIo:
		case EKernelMessage:
			MM::WaitRamAlloc();
			r=MM::AllocRegion(iRegionBase, iMaxSize);
			if (r==KErrNone)
				iRegionSize=iMaxSize;
			else
				MM::AllocFailed=ETrue;
			MM::SignalRamAlloc();
			iBase=(TUint8*)iRegionBase;
			iSize=iMaxSize;
			if(r==KErrNone)
				{
				iMapAttr = EMapAttrCachedMax;
				__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::DoCreate clear %x+%x",iRegionBase,iRegionSize));

				// Clear memory to value determined by chunk member
				memset((TAny*)iRegionBase, iClearByte, MM::RoundToBlockSize(iRegionSize));
				}
			break;
		default:
			break;
		}

	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::DoCreate %O ret %d",this,r));
	__KTRACE_OPT(KMMU,Kern::Printf("RegionBase=%08x, RegionSize=%08x",iRegionBase,iRegionSize));
	__KTRACE_OPT(KMEMTRACE, {MM::WaitRamAlloc();Kern::Printf("MT:C %d %x %O",NTickCount(),this,this);MM::SignalRamAlloc();});
#ifdef BTRACE_CHUNKS
	TKName nameBuf;
	Name(nameBuf);
	BTraceContextN(BTrace::EChunks,BTrace::EChunkCreated,this,iMaxSize,nameBuf.Ptr(),nameBuf.Size());
	if(iOwningProcess)
		BTrace8(BTrace::EChunks,BTrace::EChunkOwner,this,iOwningProcess);
	BTraceContext12(BTrace::EChunks,BTrace::EChunkInfo,this,iChunkType,iAttributes);
#endif
	return r;
	}

void DMemModelChunk::SetFixedAddress(TLinAddr anAddr, TInt aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk %O SetFixedAddress %08X size %08X",this,anAddr,aSize));
	iSize=MM::RoundToBlockSize(aSize);
	if (iSize>iMaxSize)
		iMaxSize=iSize;
	iBase=(TUint8*)anAddr;
	}

TInt DMemModelChunk::Adjust(TInt aNewSize)
//
// Adjust a standard chunk.
//
	{

	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Adjust %08x",aNewSize));
	if (iAttributes & (EDoubleEnded|EDisconnected))
		return KErrGeneral;
	if (aNewSize<0 || aNewSize>iMaxSize)
		return KErrArgument;

	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk %O adjusted to %x",this,iSize));
	__KTRACE_OPT(KMEMTRACE, {MM::WaitRamAlloc();Kern::Printf("MT:A %d %x %x %O",NTickCount(),this,iSize,this);MM::SignalRamAlloc();});
	return KErrNone;
	}

TInt DMemModelChunk::AdjustDoubleEnded(TInt aBottom, TInt aTop)
//
// Adjust a double-ended chunk.
//
	{

	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::AdjustDoubleEnded %x-%x",aBottom,aTop));
	if ((iAttributes & (EDoubleEnded|EDisconnected))!=EDoubleEnded)
		return KErrGeneral;
	if (aTop<0 || aBottom<0 || aTop<aBottom || aTop>iMaxSize)
		return KErrArgument;
	TInt newSize=aTop-aBottom;
	if (newSize>iMaxSize)
		return KErrArgument;
	iStartPos=aBottom;

	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk %O adjusted to %x+%x",this,iStartPos,iSize));
	__KTRACE_OPT(KMEMTRACE, {MM::WaitRamAlloc();Kern::Printf("MT:A %d %x %x %O",NTickCount(),this,iSize,this);MM::SignalRamAlloc();});
	return KErrNone;
	}

TInt DMemModelChunk::Address(TInt aOffset, TInt aSize, TLinAddr& aKernelAddress)
	{
	if(TUint(aOffset)>=TUint(iMaxSize))
		return KErrArgument;
	if(TUint(aOffset+aSize)>TUint(iMaxSize))
		return KErrArgument;
	if(aSize<=0)
		return KErrArgument;
	aKernelAddress = (TLinAddr)iBase+aOffset;
	return KErrNone;
	}

TInt DMemModelChunk::PhysicalAddress(TInt aOffset, TInt aSize, TLinAddr& aKernelAddress, TUint32& aPhysicalAddress, TUint32* aPhysicalPageList)
	{
	TInt r=Address(aOffset,aSize,aKernelAddress);
	if(r!=KErrNone)
		return r;

	TPhysAddr physStart = Epoc::LinearToPhysical(aKernelAddress);

	TInt pageShift = 12;
	TUint32 page = aKernelAddress>>pageShift<<pageShift;
	TUint32 lastPage = (aKernelAddress+aSize-1)>>pageShift<<pageShift;
	TUint32* pageList = aPhysicalPageList;
	TUint32 nextPhys = Epoc::LinearToPhysical(page);
	TUint32 pageSize = 1<<pageShift;
	while(page<=lastPage)
		{
		TPhysAddr phys = Epoc::LinearToPhysical(page);
		if(pageList)
			*pageList++ = phys;
		if(phys!=nextPhys)
			nextPhys = KPhysAddrInvalid;
		else
			nextPhys += pageSize;
		page += pageSize;
		}
	if(nextPhys==KPhysAddrInvalid)
		{
		// Memory is discontiguous...
		aPhysicalAddress = KPhysAddrInvalid;
		return 1;
		}
	else
		{
		// Memory is contiguous...
		aPhysicalAddress = physStart;
		return KErrNone;
		}
	}

TInt DMemModelChunk::Commit(TInt aOffset, TInt aSize, TCommitType aCommitType, TUint32* aExtraArg)
//
// Commit to a disconnected chunk.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Commit %x+%x type=%d extra=%08x",aOffset,aSize,aCommitType,aExtraArg));
	if ((iAttributes & (EDoubleEnded|EDisconnected))!=EDisconnected)
		return KErrGeneral;
	if (aOffset<0 || aSize<0 || (aOffset+aSize)>iMaxSize)
		return KErrArgument;
	if(LOGICAL_XOR((TInt)aCommitType&DChunk::ECommitPhysicalMask, iAttributes&DChunk::EMemoryNotOwned))
		return KErrNotSupported;  // Commit type doesn't match 'memory owned' type

	if((TInt)aCommitType&DChunk::ECommitPhysicalMask)
		return KErrNotSupported;
	if(aCommitType==DChunk::ECommitContiguous)
		{
		// We can't commit contiguous memory, we just have to take what's already there.
		// So check to see if memory is contiguous, and if not, return KErrNoMemory -
		// which is what other Memory Models do if they can't find enough contiguous RAM.
		TLinAddr kernAddr;
		if(PhysicalAddress(aOffset,aSize,kernAddr,*aExtraArg)!=KErrNone)
			return KErrNoMemory;
		}
	else if(aCommitType!=DChunk::ECommitDiscontiguous)
		return KErrArgument;

	return KErrNone;
	}

TInt DMemModelChunk::Allocate(TInt aSize, TInt aGuard, TInt aAlign)
//
// Allocate offset and commit to a disconnected chunk.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Allocate %x %x %d",aSize,aGuard,aAlign));
	if ((iAttributes & (EDoubleEnded|EDisconnected))!=EDisconnected)
		return KErrGeneral;
	if (aSize<=0 || aSize>iMaxSize)
		return KErrArgument;
	TInt r=KErrNotSupported;
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Allocate returns %x",r));
	return r;
	}

TInt DMemModelChunk::Decommit(TInt anOffset, TInt aSize)
//
// Decommit from a disconnected chunk.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Decommit %x+%x",anOffset,aSize));
	if ((iAttributes & (EDoubleEnded|EDisconnected))!=EDisconnected)
		return KErrGeneral;
	if (anOffset<0 || aSize<0 || (anOffset+aSize)>iMaxSize)
		return KErrArgument;
	return KErrNone;
	}

void DMemModelChunk::Substitute(TInt /*aOffset*/, TPhysAddr /*aOldAddr*/, TPhysAddr /*aNewAddr*/)
	{
	MM::Panic(MM::EUnsupportedOperation);
	}

TInt DMemModelChunk::Unlock(TInt anOffset, TInt aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Decommit %x+%x",anOffset,aSize));
	if (!(iAttributes&ECache))
		return KErrGeneral;
	if ((iAttributes & (EDoubleEnded|EDisconnected))!=EDisconnected)
		return KErrGeneral;
	if (anOffset<0 || aSize<0 || (anOffset+aSize)>iMaxSize)
		return KErrArgument;
	return KErrNone;
	}

TInt DMemModelChunk::Lock(TInt anOffset, TInt aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Decommit %x+%x",anOffset,aSize));
	if (!(iAttributes&ECache))
		return KErrGeneral;
	if ((iAttributes & (EDoubleEnded|EDisconnected))!=EDisconnected)
		return KErrGeneral;
	if (anOffset<0 || aSize<0 || (anOffset+aSize)>iMaxSize)
		return KErrArgument;
	return KErrNone;
	}

TInt DMemModelChunk::CheckAccess()
	{
	DProcess* pP=TheCurrentThread->iOwningProcess;
	if (iAttributes&EPrivate)
		{
		if (iOwningProcess && iOwningProcess!=pP && pP!=K::TheKernelProcess)
			return KErrAccessDenied;
		}
	return KErrNone;
	}

TUint32 MM::RoundToBlockSize(TUint32 aSize)
	{
	TUint32 m=MM::RamBlockSize-1;
	return (aSize+m)&~m;
	}

void MM::FreeRegion(TLinAddr aBase, TInt aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MM::FreeRegion base %08x size %08x",aBase,aSize));
	aSize=MM::RoundToBlockSize(aSize);
	__ASSERT_ALWAYS(aBase>=MM::UserDataSectionBase && aBase+aSize<=MM::UserDataSectionEnd, MM::Panic(MM::EFreeInvalidRegion));
	TInt block=(aBase-MM::UserDataSectionBase)>>MM::RamBlockShift;
	TInt nBlocks=aSize>>MM::RamBlockShift;
	MM::RamAllocator->Free(block, nBlocks);
	}

TInt MM::AllocRegion(TLinAddr& aBase, TInt aSize, TInt aAlign)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MM::AllocRegion size 0x%x align %d",aSize,aAlign));
	TInt align=Max(aAlign-MM::RamBlockShift, 0);
	TInt nBlocks=MM::RoundToBlockSize(aSize)>>MM::RamBlockShift;
	TInt base=(TInt)(MM::UserDataSectionBase>>MM::RamBlockShift);
	TInt block=MM::RamAllocator->AllocAligned(nBlocks, align, base, ETrue);	// returns first block number or -1
	if (block<0)
		return KErrNoMemory;
	MM::RamAllocator->Alloc(block,nBlocks);
	aBase=MM::UserDataSectionBase+(block<<MM::RamBlockShift);
	__KTRACE_OPT(KMMU,Kern::Printf("MM::AllocRegion address %08x",aBase));
	return KErrNone;
	}

TInt MM::ClaimRegion(TLinAddr aBase, TInt aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MM::ClaimRegion base %08x size %08x",aBase,aSize));
	TUint32 m=MM::RamBlockSize-1;
	aSize=MM::RoundToBlockSize(aSize+(aBase&m));
	aBase&=~m;
	if (aBase<MM::UserDataSectionBase || TUint32(aSize)>MM::UserDataSectionEnd-aBase)
		return KErrArgument;
	TInt block=(aBase-MM::UserDataSectionBase)>>MM::RamBlockShift;
	TInt nBlocks=aSize>>MM::RamBlockShift;
	if (MM::RamAllocator->NotFree(block, nBlocks))
		return KErrInUse;
	MM::RamAllocator->Alloc(block, nBlocks);
	return KErrNone;
	}

// Allocate a physically contiguous region
TInt MM::AllocContiguousRegion(TLinAddr& aBase, TInt aSize, TInt aAlign)
	{
#ifndef __CPU_HAS_MMU
	return MM::AllocRegion(aBase, aSize, aAlign);
#else
	__KTRACE_OPT(KMMU,Kern::Printf("MM::AllocContiguousRegion size 0x%x align %d",aSize,aAlign));
	TBitMapAllocator* sa = MM::SecondaryAllocator;
	if (!sa)
		return MM::AllocRegion(aBase, aSize, aAlign);	// only one physical bank

	TBitMapAllocator* ra = MM::RamAllocator;
	TInt align=Max(aAlign-MM::RamBlockShift, 0);
	TUint32 alignmask = (1u<<align)-1;
	TInt nBlocks=MM::RoundToBlockSize(aSize)>>MM::RamBlockShift;
	TInt base=(TInt)(MM::UserDataSectionBase>>MM::RamBlockShift);
	const SRamBank* banks = (const SRamBank*)TheSuperPage().iRamBootData;
	const SRamBank* pB = banks;
	TInt bnum = 0;
	TInt block = -1;
	for (; pB->iSize; ++pB)
		{
		TInt nb = pB->iSize >> MM::RamBlockShift;
		sa->CopyAlignedRange(ra, bnum, nb);
		TInt basealign = (base + bnum) & alignmask;
		block = sa->AllocAligned(nBlocks, align, basealign, ETrue);	// returns first block number or -1
		if (block>=0)
			break;
		bnum += nb;
		}
	if (pB->iSize == 0)
		return KErrNoMemory;
	MM::RamAllocator->Alloc(block + bnum, nBlocks);
	aBase = MM::UserDataSectionBase + ((block + bnum)<<MM::RamBlockShift);
	__KTRACE_OPT(KMMU,Kern::Printf("MM::AllocContiguousRegion address %08x",aBase));
	return KErrNone;
#endif
	}

TInt MM::BlockNumber(TPhysAddr aAddr)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("MM::BlockNumber %08x",aAddr));
	const SRamBank* banks = (const SRamBank*)TheSuperPage().iRamBootData;
	const SRamBank* pB = banks;
	TInt bnum = 0;
	for (; pB->iSize; ++pB)
		{
		if (aAddr >= pB->iBase)
			{
			TUint32 offset = aAddr - pB->iBase;
			if (offset < pB->iSize)
				{
				TInt bn = bnum + TInt(offset>>MM::RamBlockShift);
				__KTRACE_OPT(KMMU,Kern::Printf("MM::BlockNumber %08x->%x",aAddr,bn));
				return bn;
				}
			}
		TInt nb = pB->iSize >> MM::RamBlockShift;
		bnum += nb;
		}
	return KErrNotFound;
	}

/********************************************
 * Hardware chunk abstraction
 ********************************************/

/**
	@pre	Call in a thread context.
	@pre	Interrupts must be enabled.
	@pre	Kernel must be unlocked.
	@pre    No fast mutex can be held.
	@pre	Calling thread must be in a critical section.
 */
EXPORT_C TInt DPlatChunkHw::New(DPlatChunkHw*& aChunk, TPhysAddr aAddr, TInt aSize, TUint aAttribs)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DPlatChunkHw::New");
	__KTRACE_OPT(KMMU,Kern::Printf("DPlatChunkHw::New phys=%08x, size=%x, attribs=%x",aAddr,aSize,aAttribs));
	aChunk=NULL;
	if (aSize<=0)
		return KErrArgument;
	DPlatChunkHw* pC=new DPlatChunkHw;
	if (!pC)
		return KErrNoMemory;
	__KTRACE_OPT(KMMU,Kern::Printf("DPlatChunkHw created at %08x",pC));

	pC->iPhysAddr=aAddr;
	pC->iLinAddr=aAddr;
	pC->iSize=aSize;
	aChunk=pC;
	return KErrNone;
	}


void DMemModelChunk::BTracePrime(TInt aCategory)
	{
	DChunk::BTracePrime(aCategory);
	
#ifdef BTRACE_CHUNKS
	if (aCategory == BTrace::EChunks || aCategory == -1)
		{
		BTrace12(BTrace::EChunks, BTrace::EChunkMemoryAllocated,this,0,this->iSize);
		}
#endif
	}
