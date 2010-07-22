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
// e32\memmodel\epoc\moving\mchunk.cpp
// 
//

#include "memmodel.h"
#include "cache_maintenance.h"
#include <mmubase.inl>
#include <ramalloc.h>

DMemModelChunk::DMemModelChunk()
	{
	}

void DMemModelChunk::Destruct()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DMemModelChunk destruct %O",this));
	Mmu& m = Mmu::Get();
	TInt nPdes=iMaxSize>>m.iChunkShift;
	if (nPdes<=32 || iPdeBitMap!=NULL)
		{
		if ((iAttributes & EDisconnected) && iPageBitMap!=NULL)
			Decommit(0,iMaxSize);
		else if (iAttributes & EDoubleEnded)
			AdjustDoubleEnded(0,0);
		else
			Adjust(0);
		}

	if ((iAttributes&EFixedAddress) && iHomeRegionBase>=m.iKernelSection->iBase)
		{
		Mmu::Wait();
		__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::~DMemModelChunk remove region"));
		if (TLinAddr(iBase)==iHomeBase)
			iBase=NULL;
		DeallocateHomeAddress();	// unlink from home section queue
		iHomeRegionBase=0;
		iHomeBase=0;
		Mmu::Signal();
		}
	if ((iMaxSize>>m.iChunkShift) > 32)
		{
		TAny* pM = __e32_atomic_swp_ord_ptr(&iPdeBitMap, 0);
		Kern::Free(pM);
		}
	TBitMapAllocator* pM = (TBitMapAllocator*)__e32_atomic_swp_ord_ptr(&iPageBitMap, 0);
	delete pM;
	pM = (TBitMapAllocator*)__e32_atomic_swp_ord_ptr(&iPermanentPageBitMap, 0);
	delete pM;

	TDfc* dfc = (TDfc*)__e32_atomic_swp_ord_ptr(&iDestroyedDfc, 0);
	if(dfc)
		dfc->Enque();

	__KTRACE_OPT(KMEMTRACE, {Mmu::Wait(); Kern::Printf("MT:D %d %x %O",NTickCount(),this,this);Mmu::Signal();});
#ifdef BTRACE_CHUNKS
	BTraceContext4(BTrace::EChunks,BTrace::EChunkDestroyed,this);
#endif
	}

TInt DMemModelChunk::Close(TAny* aPtr)
	{
	if (aPtr)
		{
		DMemModelProcess* pP=(DMemModelProcess*)aPtr;
		pP->RemoveChunk(this);
		}
	TInt r=Dec();
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Close %d %O",r,this));
	__NK_ASSERT_DEBUG(r > 0); // Should never be negative.
	if (r==1)
		{
		K::ObjDelete(this);
		return EObjectDeleted;
		}
	return 0;
	}


TUint8* DMemModelChunk::Base(DProcess* aProcess)
	{
	return iBase;
	}


TInt DMemModelChunk::DoCreate(SChunkCreateInfo& aInfo)
	{
	__ASSERT_COMPILE(!(EMMChunkAttributesMask & EChunkAttributesMask));

	if (aInfo.iMaxSize<=0)
		return KErrArgument;
	Mmu& m=Mmu::Get();
	TInt nPdes=(aInfo.iMaxSize+m.iChunkMask)>>m.iChunkShift;
	iMaxSize=nPdes<<m.iChunkShift;
	iMapAttr = aInfo.iMapAttr;
	SetupPermissions();
	if (nPdes>32)
		{
		TInt words=(nPdes+31)>>5;
		iPdeBitMap=(TUint32*)Kern::Alloc(words*sizeof(TUint32));
		if (!iPdeBitMap)
			return KErrNoMemory;
		memclr(iPdeBitMap, words*sizeof(TUint32));
		}
	else
		iPdeBitMap=NULL;

	TInt maxpages=iMaxSize>>m.iPageShift;
	if (iAttributes & EDisconnected)
		{
		TBitMapAllocator* pM=TBitMapAllocator::New(maxpages,ETrue);
		if (!pM)
			return KErrNoMemory;
		iPageBitMap=pM;
		__KTRACE_OPT(KMMU,Kern::Printf("PageBitMap at %08x, MaxPages %d",pM,maxpages));
		}
	if(iChunkType==ESharedKernelSingle || iChunkType==ESharedKernelMultiple)
		{
		TBitMapAllocator* pM=TBitMapAllocator::New(maxpages,ETrue);
		if (!pM)
			return KErrNoMemory;
		iPermanentPageBitMap = pM;
		}
	__KTRACE_OPT(KMEMTRACE, {Mmu::Wait();Kern::Printf("MT:C %d %x %O",NTickCount(),this,this);Mmu::Signal();});
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

void DMemModelChunk::ClaimInitialPages()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Chunk %O ClaimInitialPages()",this));
	Mmu& m=Mmu::Get();
	TInt offset=0;
	TUint32 ccp=K::CompressKHeapPtr(this);
	NKern::LockSystem();
	while(offset<iSize)
		{
		TInt ptid=m.GetPageTableId(TLinAddr(iBase)+offset);
		__ASSERT_ALWAYS(ptid>=0,MM::Panic(MM::EClaimInitialPagesBadPageTable));
		__KTRACE_OPT(KMMU,Kern::Printf("Offset %x PTID=%d",offset,ptid));
		AddPde(offset);
		SPageTableInfo& ptinfo = m.PtInfo(ptid);
		ptinfo.SetChunk(ccp,offset>>m.iChunkShift);
		TPte* pPte=(TPte*)m.PageTableLinAddr(ptid);
		TInt i;
		TInt np = 0;
		TInt flashCount = MM::MaxPagesInOneGo;
		for (i=0; i<m.iChunkSize>>m.iPageShift; ++i, offset+=m.iPageSize)
			{
			if(--flashCount<=0)
				{
				flashCount = MM::MaxPagesInOneGo;
				NKern::FlashSystem();
				}
			TPte pte=pPte[i];
			if (m.PteIsPresent(pte))
				{
				++np;
				TPhysAddr phys=m.PtePhysAddr(pte, i);
				__KTRACE_OPT(KMMU,Kern::Printf("Offset %x phys %08x",offset,phys));
				SPageInfo* pi = SPageInfo::SafeFromPhysAddr(phys);
				if (pi)
					{
					pi->SetChunk(this,offset>>m.iPageShift);
#ifdef BTRACE_KERNEL_MEMORY
					--Epoc::KernelMiscPages; // page now owned by chunk, and is not 'miscelaneous'
#endif
					}
				}
			}
		ptinfo.iCount = np;
		__KTRACE_OPT(KMMU,Kern::Printf("Offset %x PTID %d NP %d", offset, ptid, np));
		}
	NKern::UnlockSystem();
	__KTRACE_OPT(KMMU,Kern::Printf("nPdes=%d, Pdes=%08x, HomePdes=%08x",iNumPdes,iPdes,iHomePdes));
	}

void DMemModelChunk::SetFixedAddress(TLinAddr aAddr, TInt aInitialSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk %O SetFixedAddress %08X size %08X",this,aAddr,aInitialSize));
	iHomeRegionOffset=0;
	iHomeRegionBase=aAddr;
	iHomeBase=aAddr;
	iBase=(TUint8*)aAddr;
	iHomeRegionSize=iMaxSize;
	iAttributes|=EFixedAddress;
	iSize=Mmu::RoundToPageSize(aInitialSize);
	ClaimInitialPages();
	}

TInt DMemModelChunk::Reserve(TInt aInitialSize)
//
// Reserve home section address space for a chunk
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk %O Reserve() size %08x",this,aInitialSize));
	iHomeRegionOffset=0;
	if (!K::Initialising)
		Mmu::Wait();
	iHomeRegionBase=AllocateHomeAddress(iMaxSize);
	if (!K::Initialising)
		Mmu::Signal();
	iHomeBase=iHomeRegionBase;
	iBase=(TUint8*)iHomeRegionBase;
	if (iHomeRegionBase==0)
		return KErrNoMemory;
	iSize=Mmu::RoundToPageSize(aInitialSize);
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk %O address %08x",this,iHomeRegionBase));
	ClaimInitialPages();
	return KErrNone;
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

	TInt r=KErrNone;
	TInt newSize=Mmu::RoundToPageSize(aNewSize);
	if (newSize!=iSize)
		{
		Mmu::Wait();
		if (newSize>iSize)
			{
			__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Adjust growing"));
			r=DoCommit(iSize,newSize-iSize);
			}
		else if (newSize<iSize)
			{
			__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Adjust shrinking"));
			DoDecommit(newSize,iSize-newSize);
			}
		Mmu::Signal();
		}
	__COND_DEBUG_EVENT(r==KErrNone, EEventUpdateChunk, this);
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk %O adjusted to %x base %08x home %08x",this,iSize,iBase,iHomeRegionBase));
	return r;
	}

TInt DMemModelChunk::ExpandHomeRegion(TInt aOffset, TInt aSize)
	{
	// Ensure that the chunk's home region is big enough to accommodate extra RAM being committed
	__KTRACE_OPT(KMMU,Kern::Printf("Chunk %O ExpandHomeRegion(%x,%x)",this,aOffset,aSize));
	Mmu& m = Mmu::Get();
	TBool lowerLimitOk=(aOffset>=iHomeRegionOffset && aOffset<=iHomeRegionOffset+iHomeRegionSize);
	TBool upperLimitOk=(aOffset+aSize>=iHomeRegionOffset && aOffset+aSize<=iHomeRegionOffset+iHomeRegionSize);
	if (lowerLimitOk && upperLimitOk)
		return KErrNone;	// no change required
	TInt newLowerLimit;
	TInt newUpperLimit;
	if (iHomeRegionSize)
		{
		newLowerLimit=Min(iHomeRegionOffset,aOffset);
		newUpperLimit=Max(iHomeRegionOffset+iHomeRegionSize,aOffset+aSize);
		}
	else
		{
		newLowerLimit=aOffset;
		newUpperLimit=aOffset+aSize;
		}
	newLowerLimit &= ~m.iChunkMask;
	newUpperLimit = (newUpperLimit+m.iChunkMask)&~m.iChunkMask;
	TInt newHomeRegionSize=newUpperLimit-newLowerLimit;
	__KTRACE_OPT(KMMU,Kern::Printf("newLowerLimit=%x, newUpperLimit=%x",newLowerLimit,newUpperLimit));
	if (newHomeRegionSize>iMaxSize)
		return KErrArgument;
	TLinAddr newHomeRegionBase;
	if (iHomeRegionSize==0)
		newHomeRegionBase=AllocateHomeAddress(newHomeRegionSize);
	else
		newHomeRegionBase=ReallocateHomeAddress(newHomeRegionSize);
	__KTRACE_OPT(KMMU,Kern::Printf("newHomeRegionBase=%08x",newHomeRegionBase));
	if (newHomeRegionBase==0)
		return KErrNoMemory;
	TInt deltaOffset=iHomeRegionOffset-newLowerLimit;
	TLinAddr newHomeBase=newHomeRegionBase-newLowerLimit;
	TLinAddr translatedHomeBase=newHomeRegionBase+deltaOffset;

	// lock the kernel while we change the chunk's home region
	// Note: The new home region always contains the original home region, so
	// if we reach here, it must be strictly larger.
	NKern::LockSystem();
	if (iNumPdes && iHomeRegionBase!=translatedHomeBase)
		{
		TLinAddr oldBase=TLinAddr(iBase);
		if (oldBase==iHomeBase)
			{
			// chunk is currently at home, so must move it
			// Note: this operation must cope with overlapping initial and final regions
			m.GenericFlush(Mmu::EFlushDMove);		// preemption could occur here...
			if (TLinAddr(iBase)==iHomeBase)	// ...so need to check chunk is still at home address
				{
				m.MoveChunk(iHomeRegionBase,translatedHomeBase,iNumPdes);
				iBase=(TUint8*)newHomeBase;
				MoveCurrentPdes(iHomeRegionBase,translatedHomeBase);
				MoveHomePdes(iHomeRegionBase,translatedHomeBase);
				}
			}
		else
			{
			MoveHomePdes(iHomeRegionBase,translatedHomeBase);
			}
		__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::ExpandHomeRegion moved home base from %08x to %08x",
							iHomeRegionBase,newHomeRegionBase));
		}
	if (!iBase)
		iBase=(TUint8*)newHomeBase;
	iHomeRegionBase=newHomeRegionBase;
	iHomeRegionOffset=newLowerLimit;
	iHomeBase=newHomeBase;
	__KTRACE_OPT(KMMU,Kern::Printf("Final iHomeRegionBase=%08x, iHomeRegionOffset=%08x",iHomeRegionBase,iHomeRegionOffset));
	__KTRACE_OPT(KMMU,Kern::Printf("Final iHomeRegionSize=%08x, iBase=%08x, iHomeBase=%08x",iHomeRegionSize,iBase,iHomeBase));
	__KTRACE_OPT(KMMU,Kern::Printf("nPdes=%d, Pdes=%08x, HomePdes=%08x",iNumPdes,iPdes,iHomePdes));
	NKern::UnlockSystem();
	return KErrNone;
	}

TInt DMemModelChunk::Address(TInt aOffset, TInt aSize, TLinAddr& aKernelAddress)
	{
	if(!iPermanentPageBitMap)
		return KErrAccessDenied;
	if(TUint(aOffset)>=TUint(iMaxSize))
		return KErrArgument;
	if(TUint(aOffset+aSize)>TUint(iMaxSize))
		return KErrArgument;
	if(aSize<=0)
		return KErrArgument;
	TInt pageShift = Mmu::Get().iPageShift;
	TInt start = aOffset>>pageShift;
	TInt size = ((aOffset+aSize-1)>>pageShift)-start+1;
	if(iPermanentPageBitMap->NotAllocated(start,size))
		return KErrNotFound;
	aKernelAddress = (TLinAddr)iBase+aOffset;
	return KErrNone;
	}

TInt DMemModelChunk::PhysicalAddress(TInt aOffset, TInt aSize, TLinAddr& aKernelAddress, TUint32& aPhysicalAddress, TUint32* aPhysicalPageList)
	{
	TInt r=Address(aOffset,aSize,aKernelAddress);
	if(r!=KErrNone)
		return r;

	return Mmu::Get().LinearToPhysical(aKernelAddress,aSize,aPhysicalAddress,aPhysicalPageList);
	}

void DMemModelChunk::Substitute(TInt aOffset, TPhysAddr aOldAddr, TPhysAddr aNewAddr)
	{
	// Substitute the page mapping at aOffset with aNewAddr.
	// Called with the system lock held.
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Substitute %x %08x %08x",aOffset,aOldAddr,aNewAddr));
	Mmu& m = Mmu::Get();
	
	TLinAddr addr=(TLinAddr)iBase+aOffset;
	TInt ptid=m.GetPageTableId(addr);
	if(ptid<0)
		MM::Panic(MM::EChunkRemapNoPageTable);

	m.RemapPage(ptid, addr, aOldAddr, aNewAddr, iPtePermissions, iOwningProcess);
	if(iChunkType==EKernelCode || iChunkType==EDll || iChunkType==EUserSelfModCode)
		m.SyncCodeMappings();
	}

/**
Get the movability type of the chunk's pages
@return How movable the chunk's pages are
*/
TZonePageType DMemModelChunk::GetPageType()
	{
	// Shared chunks have their physical addresses available
	if (iChunkType == ESharedKernelSingle ||
		iChunkType == ESharedKernelMultiple || 
		iChunkType == ESharedIo ||
		iChunkType == ESharedKernelMirror ||
		iChunkType == EKernelMessage ||
		iChunkType == EKernelData)	// Don't move kernel heap pages as DMA may be accessing them.
		{
		return EPageFixed;
		}
	// All other types of chunk are movable
	return EPageMovable;
	}

TInt DMemModelChunk::DoCommit(TInt aOffset, TInt aSize, TCommitType aCommitType, TUint32* aExtraArg)
	{
	// Commit more RAM to a chunk at a specified offset
	// enter and leave with system unlocked
	// must hold RamAlloc mutex before calling this function
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::DoCommit %x+%x type=%d extra=%08x",aOffset,aSize,aCommitType,aExtraArg));
	TInt offset=aOffset;
	TInt endOffset=offset+aSize;
	TInt newPtId=-1;
	Mmu& m = Mmu::Get();
	DRamAllocator& a = *m.iRamPageAllocator;
	TInt r=KErrNone;
	TPhysAddr pageList[KMaxPages];
	TPhysAddr* pPageList=0;
	TPhysAddr nextPage=0;
	TUint32 ccp=K::CompressKHeapPtr(this);
	SPageInfo::TType type = SPageInfo::EChunk;

	if (iHomeRegionSize==0 || (iAttributes&EFixedAddress)==0)
		{
		r=ExpandHomeRegion(aOffset,aSize);
		if (r!=KErrNone)
			return r;
		}

	// Set flag to indicate if RAM should be cleared before being committed.
	// Note, EDll, EUserCode are covered in the code segment, in order not to clear
	// the region overwritten by the loader
	TBool clearRam =	iChunkType==EUserData
					 || iChunkType==EDllData
					 || iChunkType==EUserSelfModCode
					 || iChunkType==ESharedKernelSingle
					 || iChunkType==ESharedKernelMultiple
					 || iChunkType==ESharedIo
                     || iChunkType==ERamDrive;


	TBool ownsMemory = !(iAttributes&EMemoryNotOwned);
	TBool physicalCommit = aCommitType&DChunk::ECommitPhysicalMask;
	if(ownsMemory)
		{
		if(physicalCommit)
			return KErrNotSupported;
		}
	else
		{
		if(!physicalCommit)
			return KErrNotSupported;
		type = SPageInfo::EInvalid;	// to indicate page info not to be updated
		}

	switch(aCommitType)
		{
	case DChunk::ECommitDiscontiguous:
		// No setup to do
		break;

	case DChunk::ECommitContiguous:
		{
		// Allocate a block of contiguous RAM from the free pool
		TInt numPages=(endOffset-offset)>>m.iPageShift;
		__NK_ASSERT_DEBUG(EPageFixed == GetPageType());
		r=m.AllocContiguousRam(numPages<<m.iPageShift, nextPage, 0);
		if (r!=KErrNone)
			return r;
		if(clearRam)
			m.ClearPages(numPages, (TPhysAddr*)(nextPage|1), iClearByte);  // clear RAM if required
		*aExtraArg = nextPage;	// store physical address of RAM as return argument
		}
		break;

	case DChunk::ECommitDiscontiguousPhysical:
		{
		pPageList = aExtraArg;				// use pages given given to us

		// Check address of pages are multiples of page size...
		TInt numPages=(endOffset-offset)>>m.iPageShift;
		TUint32* ptr = aExtraArg;
		TUint32* endPtr = aExtraArg+numPages;
		if(ptr>=endPtr)
			return KErrNone;				// Zero size commit is OK
		TPhysAddr pageBits = 0;
		do
			pageBits |= *ptr++;
		while(ptr<endPtr);
		if(pageBits&(m.iPageSize-1))
			return KErrArgument;			// all addresses must be multiple of page size
		}
		break;

	case DChunk::ECommitContiguousPhysical:
		nextPage = (TPhysAddr)aExtraArg;	// we have been given the physical address to use
		if(nextPage&(m.iPageSize-1))
			return KErrArgument;			// address must be multiple of page size
		break;

#ifdef __MARM__
	case DChunk::ECommitVirtual:
		break;
#endif

	default:
		return KErrNotSupported;
		}

	// Commit memory a bit at a time (so system lock is only needs to be held for limited time)
	while(offset<endOffset)
		{
		TInt np=(endOffset-offset)>>m.iPageShift;	// pages remaining to satisfy request
		TInt npEnd=(m.iChunkSize-(offset&m.iChunkMask))>>m.iPageShift;	// number of pages to end of page table
		if (np>npEnd)
			np=npEnd;								// limit to single page table
		if (np>MM::MaxPagesInOneGo)
			np=MM::MaxPagesInOneGo;					// limit
		NKern::LockSystem();						// lock the system while we look at the page directory
		TLinAddr addr=(TLinAddr)iBase+offset;		// current address
		TInt ptid=m.GetPageTableId(addr);			// get page table ID if a page table is already assigned here
		NKern::UnlockSystem();						// we can now unlock the system
		newPtId=-1;
		if (ptid<0)
			{
			// need to allocate a new page table
			newPtId=m.AllocPageTable();
			if (newPtId<0)
				{
				// out of memory, so break out and revert
				r=KErrNoMemory;
				break;
				}
			ptid=newPtId;
			}

		if(aCommitType==DChunk::ECommitDiscontiguous)
			{
			pPageList = pageList;
			r=m.AllocRamPages(pPageList,np, GetPageType());	// try to allocate pages
			if (r!=KErrNone)
				break;							// if we fail, break out and revert
			if(clearRam)
				m.ClearPages(np, pPageList, iClearByte);	// clear RAM if required
			}

		// lock the system while we change the MMU mappings
		NKern::LockSystem();
		TInt commitSize = np<<m.iPageShift;
		iSize += commitSize;					// update committed size
		if (aCommitType==DChunk::ECommitVirtual)
			m.MapVirtual(ptid, np);
		else if(pPageList)
			{
			m.MapRamPages(ptid, type, this, offset, pPageList, np, iPtePermissions);
			pPageList += np;
			}
		else
			{
			m.MapPhysicalPages(ptid, type, this, offset, nextPage, np, iPtePermissions);
			nextPage += commitSize;
			}
		NKern::UnlockSystem();

		NKern::LockSystem();
		if (newPtId>=0)
			{
			// We have allocated a new page table, now we must assign it and update PDE info
			SPageTableInfo& pti=m.PtInfo(ptid);
			pti.SetChunk(ccp, offset>>m.iChunkShift);
			TLinAddr addr=(TLinAddr)iBase+offset;	// current address
			m.DoAssignPageTable(ptid, addr, iPdePermissions[iChunkState]);
			AddPde(offset);						// update PDE info
			}
		__KTRACE_OPT(KMMU,Kern::Printf("nPdes=%d, Pdes=%08x, HomePdes=%08x",iNumPdes,iPdes,iHomePdes));
		NKern::UnlockSystem();
		__KTRACE_OPT(KMEMTRACE,Kern::Printf("MT:A %d %x %x %O",NTickCount(),this,iSize,this));
#ifdef BTRACE_CHUNKS
		BTraceContext12(BTrace::EChunks,ownsMemory?BTrace::EChunkMemoryAllocated:BTrace::EChunkMemoryAdded,this,offset,commitSize);
#endif

		offset += commitSize;				// update offset
		}

	if (r==KErrNone)
		{
		if(iPermanentPageBitMap)
			iPermanentPageBitMap->Alloc(aOffset>>m.iPageShift,aSize>>m.iPageShift);
		}
	else
		{
		// we ran out of memory somewhere
		// first check if we have an unassigned page table
		if (newPtId>=0)
			m.FreePageTable(newPtId);			// free the unassigned page table

		// now free any memory we succeeded in allocating and return the chunk to its initial state
		DChunk::TDecommitType decommitType = aCommitType==DChunk::ECommitVirtual ?
			DChunk::EDecommitVirtual : DChunk::EDecommitNormal;
		DoDecommit(aOffset,offset-aOffset,decommitType);

		if(aCommitType==DChunk::ECommitContiguous)
			{
			// Free the pages we allocated but didn't get around to commiting
			TPhysAddr last = nextPage + ((endOffset-offset)>>m.iPageShift<<m.iPageShift);
			while(nextPage<last)
				{
				a.FreeRamPage(nextPage, GetPageType());
				nextPage += m.iPageSize;
				}
			*aExtraArg = KPhysAddrInvalid;	// return invalid physical address
			}

		m.iAllocFailed=ETrue;
		}
	return r;
	}

void DMemModelChunk::DoDecommit(TInt aOffset, TInt aSize, TDecommitType aDecommitType)
	{
	// Decommit RAM from a chunk at a specified offset
	// enter and leave with kernel unlocked
	// must hold RamAlloc mutex before calling this function
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::DoDecommit %x+%x",aOffset,aSize));
	if (iHomeRegionBase==0)
		return;
	
	TBool ownsMemory = !(iAttributes&EMemoryNotOwned);
	if (!ownsMemory)
		{
		// Physical memory not owned by the chunk also has to be evicted from cache(s).
		// We cannot just purge, as it can still be in use by the driver. Therefore, we'll flush it.
		// Purging physical memory from cache(s) that is owned by the chunk is done below.
		CacheMaintenance::MemoryToPreserveAndReuse((TLinAddr)(iBase+aOffset), aSize, iMapAttr);			
		}
	
	TInt offset=aOffset;
	TInt endOffset=offset+aSize;
	Mmu& m = Mmu::Get();
	DRamAllocator& a = *m.iRamPageAllocator;
	TPhysAddr pageList[KMaxPages];
#ifdef __CPU_WRITE_BACK_CACHE
	TInt size_reduction = Min(aSize,iSize);
	TBool selectiveFlush=((TUint)size_reduction<=(CacheMaintenance::SyncAllPerformanceThresholdPages()<<KPageShift));
#endif
	while(offset<endOffset)
		{
		TInt np=(endOffset-offset)>>m.iPageShift;		// number of pages remaining to decommit
		TInt pdeEnd=(offset+m.iChunkSize)&~m.iChunkMask;
		TInt npEnd=(pdeEnd-offset)>>m.iPageShift;		// number of pages to end of page table
		if (np>npEnd)
			np=npEnd;									// limit to single page table
		if (np>MM::MaxPagesInOneGo)
			np=MM::MaxPagesInOneGo;						// limit
		NKern::LockSystem();							// lock the system while we look at the page directory
		TUint8* base=iBase;								// save base address
		TLinAddr addr=(TLinAddr)base+offset;			// current address
		TInt ptid=m.GetPageTableId(addr);				// get page table ID if a page table is already assigned here
		if (ptid>=0)
			{
			TInt nPtes=0;
			TInt nFree=0;

			// Unmap the pages, clear the PTEs and place the physical addresses of the now-free RAM pages in
			// pageList. Return nPtes=number of pages placed in list, remain=number of PTEs remaining in page table
			// This also invalidates any TLB entries for the unmapped pages.
			// NB for WriteBack cache, we must also invalidate any cached entries for these pages - this might be done
			// by invalidating entry-by-entry or by a complete cache flush at the end.
			// NB For split TLB, ITLB may not be invalidated. In that case it will be invalidated by
			// Mmu::SyncCodeMappings() at the end of the function.
			TInt remain;
			if (aDecommitType == EDecommitVirtual)
				remain=m.UnmapVirtual(ptid,addr,np,pageList,ownsMemory,nPtes,nFree,iOwningProcess);
			else
				remain=m.UnmapPages(ptid,addr,np,pageList,ownsMemory,nPtes,nFree,iOwningProcess);
			TInt decommitSize=nPtes<<m.iPageShift;
			iSize-=decommitSize;				// reduce the committed size

			// if page table is now completely empty, unassign it and update chunk PDE info
			remain &= KUnmapPagesCountMask;
			if (remain==0)
				{
				m.DoUnassignPageTable(addr);
				RemovePde(offset);
				NKern::UnlockSystem();
				m.FreePageTable(ptid);
				NKern::LockSystem();
				}
			__KTRACE_OPT(KMMU,Kern::Printf("nPdes=%d, Pdes=%08x, HomePdes=%08x",iNumPdes,iPdes,iHomePdes));
#ifdef __CPU_WRITE_BACK_CACHE
			if (selectiveFlush)
				{
				TInt n=np;
				while(n && iBase==base)	// reschedule may move base, but then cache will have been flushed so we can stop purging L1
					{
					CacheMaintenance::PageToReuseVirtualCache(addr);
					addr+=m.iPageSize;
					--n;
					NKern::FlashSystem();
					}
				Mmu::Get().CacheMaintenanceOnDecommit(pageList, nFree);	//On ARMv5, this deals with L2 cache only
				}
#endif
			NKern::UnlockSystem();				// we can now unlock the system
			__KTRACE_OPT(KMEMTRACE,Kern::Printf("MT:A %d %x %x %O",NTickCount(),this,iSize,this));
#ifdef BTRACE_CHUNKS
			if(nFree)
				BTraceContext12(BTrace::EChunks,ownsMemory?BTrace::EChunkMemoryDeallocated:BTrace::EChunkMemoryRemoved,this,offset,nFree<<m.iPageShift);
#endif

			// We can now return the decommitted pages to the free page list
			if (nFree)
				a.FreeRamPages(pageList,nFree, GetPageType());

			offset+=(np<<m.iPageShift);
			}
		else
			{
			NKern::UnlockSystem();
			__KTRACE_OPT(KMMU,Kern::Printf("No page table at %08x",addr));
			if ((iAttributes&EDisconnected)==0)
				MM::Panic(MM::EChunkDecommitNoPageTable);
			offset=pdeEnd;	// disconnected chunk - step on to next PDE
			}
		}
	if (iSize==0 && (iAttributes&EFixedAddress)==0)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::DoDecommit remove region"));
		NKern::LockSystem();
		if (TLinAddr(iBase)==iHomeBase)
			iBase=NULL;
		DeallocateHomeAddress();
		NKern::UnlockSystem();
		}
#ifdef __CPU_WRITE_BACK_CACHE
	if (!selectiveFlush)
		{
		NKern::LockSystem();
		m.GenericFlush((TUint)Mmu::EFlushDDecommit); 	//Flush virtual DCache
		CacheMaintenance::SyncPhysicalCache_All();
		NKern::UnlockSystem();
		}
#endif
	if (iAttributes & ECode)
		m.SyncCodeMappings();		// flush ITLB if necessary
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
	Mmu& m = Mmu::Get();
	aBottom &= ~m.iPageMask;
	aTop=(aTop+m.iPageMask)&~m.iPageMask;
	TInt newSize=aTop-aBottom;
	if (newSize>iMaxSize)
		return KErrArgument;

	Mmu::Wait();
	TInt initBottom=iStartPos;
	TInt initTop=iStartPos+iSize;
	TInt nBottom=Max(aBottom,iStartPos);	// intersection bottom
	TInt nTop=Min(aTop,iStartPos+iSize);	// intersection top
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
	Mmu::Signal();
	__COND_DEBUG_EVENT(r==KErrNone, EEventUpdateChunk, this);
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk %O adjusted to %x+%x base %08x home %08x",this,iStartPos,iSize,iBase,iHomeRegionBase));
	return r;
	}

TInt DMemModelChunk::Commit(TInt aOffset, TInt aSize, TCommitType aCommitType, TUint32* aExtraArg)
//
// Commit to a disconnected chunk.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Commit %x+%x type=%d extra=%08x",aOffset,aSize,aCommitType,aExtraArg));
	if ((iAttributes & (EDoubleEnded|EDisconnected))!=EDisconnected)
		return KErrGeneral;
	if (aOffset<0 || aSize<0)
		return KErrArgument;
	if (aSize==0)
		return KErrNone;
	Mmu& m = Mmu::Get();
	aSize+=(aOffset & m.iPageMask);
	aOffset &= ~m.iPageMask;
	aSize=(aSize+m.iPageMask)&~m.iPageMask;
	if ((aOffset+aSize)>iMaxSize)
		return KErrArgument;

	Mmu::Wait();
	TInt r=KErrNone;
	TInt i=aOffset>>m.iPageShift;
	TInt n=aSize>>m.iPageShift;
	if (iPageBitMap->NotFree(i,n))
		r=KErrAlreadyExists;
	else
		{
		r=DoCommit(aOffset,aSize,aCommitType,aExtraArg);
		if (r==KErrNone)
			iPageBitMap->Alloc(i,n);
		}
	Mmu::Signal();
	__COND_DEBUG_EVENT(r==KErrNone, EEventUpdateChunk, this);
	return r;
	}

TInt DMemModelChunk::Allocate(TInt aSize, TInt aGuard, TInt aAlign)
//
// Allocate offset and commit to a disconnected chunk.
//
	{
	TInt r = DoAllocate(aSize, aGuard, aAlign, ETrue);
	__COND_DEBUG_EVENT(r==KErrNone, EEventUpdateChunk, this);
	return r;
	}

TInt DMemModelChunk::FindFree(TInt aSize, TInt aGuard, TInt aAlign)
//
// Find free offset but don't commit any memory.
//
	{
	return DoAllocate(aSize, aGuard, aAlign, EFalse);
	}

TInt DMemModelChunk::DoAllocate(TInt aSize, TInt aGuard, TInt aAlign, TBool aCommit)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::DoAllocate %x %x %d",aSize,aGuard,aAlign));

	// Only allow this to be called on disconnected chunks and not disconnected 
	// cache chunks as when guards pages exist the bit map can't be used to determine
	// the size of disconnected cache chunks as is required by Decommit().
	if ((iAttributes & (EDoubleEnded|EDisconnected|ECache))!=EDisconnected)
		return KErrGeneral;

	if (aSize<=0 || aGuard<0)
		return KErrArgument;
	Mmu& m = Mmu::Get();
	aAlign=Max(aAlign-m.iPageShift,0);
	aSize=(aSize+m.iPageMask)&~m.iPageMask;
	aGuard=(aGuard+m.iPageMask)&~m.iPageMask;
	if ((aSize+aGuard)>iMaxSize)
		return KErrArgument;

	Mmu::Wait();
	TInt r=KErrNone;
	TInt n=(aSize+aGuard)>>m.iPageShift;
	TInt i=iPageBitMap->AllocAligned(n,aAlign,0,EFalse);	// allocate the offset
	if (i<0)
		r=KErrNoMemory;		// run out of reserved space for this chunk
	else
		{
		TInt offset=i<<m.iPageShift;
		__KTRACE_OPT(KMMU,Kern::Printf("Offset %x allocated",offset));
		if (aCommit)
			{
			r=DoCommit(offset+aGuard,aSize,ECommitDiscontiguous);
			if (r==KErrNone)
				iPageBitMap->Alloc(i,n);
			}
		if (r==KErrNone)
			r=offset;		// if operation successful, return allocated offset
		}
	Mmu::Signal();
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::DoAllocate returns %x",r));
	return r;
	}

TInt DMemModelChunk::Decommit(TInt aOffset, TInt aSize)
//
// Decommit from a disconnected chunk.
//
	{
	return Decommit(aOffset, aSize, EDecommitNormal);
	}

TInt DMemModelChunk::Decommit(TInt aOffset, TInt aSize, TDecommitType aDecommitType)
//
// Decommit from a disconnected chunk.
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Decommit %x+%x",aOffset,aSize));
	if ((iAttributes & (EDoubleEnded|EDisconnected))!=EDisconnected)
		return KErrGeneral;
	if (aOffset<0 || aSize<0)
		return KErrArgument;
	if (aSize==0)
		return KErrNone;
	Mmu& m = Mmu::Get();
	aSize+=(aOffset & m.iPageMask);
	aOffset &= ~m.iPageMask;
	aSize=(aSize+m.iPageMask)&~m.iPageMask;
	if ((aOffset+aSize)>iMaxSize)
		return KErrArgument;

	Mmu::Wait();

	// limit the range to the home region range
	TInt end = aOffset+aSize;
	if (aOffset<iHomeRegionOffset)
		aOffset=iHomeRegionOffset;
	if (end>iHomeRegionOffset+iHomeRegionSize)
		end=iHomeRegionOffset+iHomeRegionSize;
	aSize = end-aOffset;
	if(aSize<0)
		aSize=0;
	__KTRACE_OPT(KMMU,Kern::Printf("Rounded and Clipped range %x+%x",aOffset,aSize));

	if (aSize)
		{
		TInt i=aOffset>>m.iPageShift;
		TInt n=aSize>>m.iPageShift;
		__KTRACE_OPT(KMMU,Kern::Printf("Calling SelectiveFree(%d,%d)",i,n));
		TUint oldAvail = iPageBitMap->iAvail;
		TUint oldSize = iSize;

		// Free those positions which are still commited and also any guard pages, 
		// i.e. pages that are reserved in this chunk but which are not commited.
		iPageBitMap->SelectiveFree(i,n);
		DoDecommit(aOffset,aSize,aDecommitType);

		if (iAttributes & ECache)
			{// If this is the file server cache chunk then adjust the size based 
			// on the bit map size because:-
			//	- 	Unlocked and reclaimed pages will be unmapped without updating
			// 		iSize or the bit map. 
			//	-	DoDecommit() only decommits the mapped pages.
			// For all other chunks what is mapped is what is committed to the 
			// chunk so iSize is accurate.
			TUint actualFreedPages = iPageBitMap->iAvail - oldAvail;
			iSize = oldSize - (actualFreedPages << KPageShift);
			}
		}

	Mmu::Signal();
	__DEBUG_EVENT(EEventUpdateChunk, this);
	return KErrNone;
	}

TInt DMemModelChunk::Unlock(TInt aOffset, TInt aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Unlock %x+%x",aOffset,aSize));
	if (!(iAttributes&ECache))
		return KErrGeneral;
	if ((iAttributes & (EDoubleEnded|EDisconnected))!=EDisconnected)
		return KErrGeneral;

	// Mark this as the file server cache chunk.  This is safe as it is only the 
	// file server that can invoke this function.
	iAttributes |= ECache;

	if (aOffset<0 || aSize<0)
		return KErrArgument;
	if (aSize==0)
		return KErrNone;
	Mmu& m = Mmu::Get();
	aSize+=(aOffset & m.iPageMask);
	aOffset &= ~m.iPageMask;
	aSize=(aSize+m.iPageMask)&~m.iPageMask;
	if ((aOffset+aSize)>iMaxSize)
		return KErrArgument;

	Mmu::Wait();
	TInt r=KErrNone;
	TInt i=aOffset>>m.iPageShift;
	TInt n=aSize>>m.iPageShift;
	if (iPageBitMap->NotAllocated(i,n))
		r=KErrNotFound;
	else
		{
#ifdef BTRACE_CHUNKS
		TUint oldFree = m.FreeRamInBytes();
#endif
		r=Mmu::Get().UnlockRamCachePages(iBase,i,n);
#ifdef BTRACE_CHUNKS
		if(r==KErrNone)
			{
			TUint unlocked = m.FreeRamInBytes()-oldFree; // size of memory unlocked
			if(unlocked)
				BTraceContext12(BTrace::EChunks,BTrace::EChunkMemoryDeallocated,this,aOffset,unlocked);
			}
#endif
		}
	Mmu::Signal();
	__COND_DEBUG_EVENT(r==KErrNone, EEventUpdateChunk, this);
	return r;
	}

TInt DMemModelChunk::Lock(TInt aOffset, TInt aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Lock %x+%x",aOffset,aSize));
	if (!(iAttributes&ECache))
		return KErrGeneral;
	if ((iAttributes & (EDoubleEnded|EDisconnected))!=EDisconnected)
		return KErrGeneral;
	if (aOffset<0 || aSize<0)
		return KErrArgument;
	if (aSize==0)
		return KErrNone;
	Mmu& m = Mmu::Get();
	aSize+=(aOffset & m.iPageMask);
	aOffset &= ~m.iPageMask;
	aSize=(aSize+m.iPageMask)&~m.iPageMask;
	if ((aOffset+aSize)>iMaxSize)
		return KErrArgument;

	Mmu::Wait();
	TInt r=KErrNone;
	TInt i=aOffset>>m.iPageShift;
	TInt n=aSize>>m.iPageShift;
	if (iPageBitMap->NotAllocated(i,n))
		r=KErrNotFound;
	else
		{
#ifdef BTRACE_CHUNKS
		TUint oldFree = m.FreeRamInBytes();
#endif
		r=Mmu::Get().LockRamCachePages(iBase,i,n);
#ifdef BTRACE_CHUNKS
		if(r==KErrNone)
			{
			TUint locked = oldFree-m.FreeRamInBytes();
			if(locked)
				BTraceContext12(BTrace::EChunks,BTrace::EChunkMemoryAllocated,this,aOffset,locked);
			}
#endif
		}
	if(r!=KErrNone)
		{
		// decommit memory on error...
		__KTRACE_OPT(KMMU,Kern::Printf("Calling SelectiveFree(%d,%d)",i,n));
		TUint oldAvail = iPageBitMap->iAvail;
		iPageBitMap->SelectiveFree(i,n);	// free those positions which are actually allocated
		TUint oldSize = iSize;

		DoDecommit(aOffset,aSize);

		// Use the bit map to adjust the size of the chunk as unlocked and reclaimed pages
		// will have been unmapped but not removed from the bit map as DoDecommit() only 
		// decommits the mapped pages.
		TUint actualFreedPages = iPageBitMap->iAvail - oldAvail;
		iSize = oldSize - (actualFreedPages << KPageShift);
		}

	Mmu::Signal();
	__COND_DEBUG_EVENT(r==KErrNone, EEventUpdateChunk, this);
	return r;
	}

#ifndef __SCHEDULER_MACHINE_CODED__
// System locked in this function for a time proportional to chunk size.
// This is unavoidable since the chunk state must always be well defined
// whenever the system is unlocked.
TUint32 DMemModelChunk::ApplyTopLevelPermissions(TChunkState aChunkState)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ApplyTopLevelPermissions ChunkState=%d",aChunkState));
	if (!(iAttributes&EFixedAccess))
		{
		iChunkState=aChunkState;
		if (iSize)
			{
			Mmu& m = Mmu::Get();
			TLinAddr base=(TLinAddr)iBase;
			TInt size=iSize;
			TUint32 mask=m.iChunkMask;
			if (iAttributes & EDoubleEnded)
				{
				base+=(iStartPos & ~mask);
				size=((iStartPos&mask)+size+mask)&~mask;
				}
			m.ApplyTopLevelPermissions(base,size,iPdePermissions[aChunkState]);
			}
		return (iAttributes&ECode)?Mmu::EFlushDPermChg|Mmu::EFlushIPermChg:Mmu::EFlushDPermChg;
		}
	return 0;
	}

// System locked in this function for a time proportional to chunk size.
// This is unavoidable since the chunk state must always be well defined
// whenever the system is unlocked.
TUint32 DMemModelChunk::MoveToRunAddress(TLinAddr aLinearAddr, TChunkState aChunkState)
	{
	iChunkState=aChunkState;
	if (iSize)
		{
		TLinAddr base=(TLinAddr)iBase;
		TLinAddr dest=aLinearAddr;
		TInt size=iSize;
		if (iAttributes & EDoubleEnded)
			{
			Mmu& m = Mmu::Get();
			TUint32 mask=m.iChunkMask;
			base+=(iStartPos & ~mask);
			dest+=(iStartPos & ~mask);
			size=((iStartPos&mask)+size+mask)&~mask;
			}
		m.MoveChunk(base,size,dest,iPdePermissions[aChunkState]);
		}
	MoveCurrentPdes((TLinAddr)iBase,aLinearAddr);
	iBase=(TUint8 *)aLinearAddr;
	return Mmu::EFlushDMove;	// chunk can't contain code
	}

// System locked in this function for a time proportional to chunk size.
// This is unavoidable since the chunk state must always be well defined
// whenever the system is unlocked.
TUint32 DMemModelChunk::MoveToHomeSection()
	{
	iChunkState=ENotRunning;
	if (iSize)
		{
		TLinAddr base=TLinAddr(iBase);
		TLinAddr home=iHomeRegionBase;
		TInt size=iSize;
		if (iAttributes & EDoubleEnded)
			{
			Mmu& m = Mmu::Get();
			TUint32 mask=m.iChunkMask;
			base+=(iStartPos & ~mask);
			home+=(iStartPos & ~mask);
			size=((iStartPos&mask)+size+mask)&~mask;
			}
		m.MoveChunk(base,size,home,iPdePermissions[0]);
		}
	iBase=(TUint8 *)iHomeRegionBase;
	iHomePdes=iPdes;
	return Mmu::EFlushDMove;	// chunk can't contain code
	}
#endif

TLinAddr DMemModelChunk::AllocateHomeAddress(TInt aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::AllocateHomeAddress size %08x",aSize));
	Mmu& m = Mmu::Get();
	TLinearSection* s = m.iKernelSection;
	TUint required;
	if (iAttributes&EFixedAddress)
		required=Mmu::RoundToChunkSize(iMaxSize);
	else
		required=Mmu::RoundToChunkSize(aSize);
	required >>= m.iChunkShift;
	TInt r = s->iAllocator.AllocConsecutive(required, EFalse);
	if (r<0)
		return 0;
	s->iAllocator.Alloc(r, required);
	TLinAddr addr = s->iBase + (r<<m.iChunkShift);
	__KTRACE_OPT(KMMU,Kern::Printf("Address %08x allocated",addr));
	iHomeRegionSize = required << m.iChunkShift;
	return addr;
	}

void DMemModelChunk::DeallocateHomeAddress()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::DeallocateHomeAddress %08x+%x", iHomeRegionBase, iHomeRegionSize));
	if (iHomeRegionSize)
		{
		Mmu& m = Mmu::Get();
		TLinearSection* s = m.iKernelSection;
		TInt first = (TInt)((iHomeRegionBase - s->iBase)>>m.iChunkShift);
		TInt count = (TInt)(iHomeRegionSize >> m.iChunkShift);
		s->iAllocator.Free(first, count);
		iHomeRegionBase=0;
		iHomeRegionSize=0;
		}
	}

TLinAddr DMemModelChunk::ReallocateHomeAddress(TInt aNewSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::ReallocateHomeAddress(%08x) for chunk %O",aNewSize,this));

	// can never be called for a fixed address chunk
	__ASSERT_ALWAYS((iAttributes&(EFixedAddress))==0,MM::Panic(MM::EFixedChunkMoving));

	Mmu& m = Mmu::Get();
	TLinearSection* s = m.iKernelSection;
	TUint required=Mmu::RoundToChunkSize(aNewSize);
	TInt next = (TInt)((iHomeRegionBase + iHomeRegionSize - s->iBase)>>m.iChunkShift);
	TInt count = (TInt)((required - iHomeRegionSize) >> m.iChunkShift);
	if (!s->iAllocator.NotFree(next, count))
		{
		// we can expand in place
		s->iAllocator.Alloc(next, count);
		iHomeRegionSize = required;
		return iHomeRegionBase;
		}
	TUint oldHomeSize = iHomeRegionSize;
	TLinAddr addr = AllocateHomeAddress(required);	// try to get a new home address
	if (addr && oldHomeSize)
		{
		// succeeded - free old region
		next = (TInt)((iHomeRegionBase - s->iBase)>>m.iChunkShift);
		count = (TInt)(oldHomeSize >> m.iChunkShift);
		s->iAllocator.Free(next, count);
		}
	// if it fails, keep our current home region
	return addr;
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

TInt DMemModelChunkHw::Close(TAny*)
	{
	__KTRACE_OPT(KOBJECT,Kern::Printf("DMemModelChunkHw::Close %d %O",AccessCount(),this));
	TInt r=Dec();
	if (r==1)
		{
		if (iLinAddr)
			{
			// Physical memory has to be evicted from cache(s).
			// Must be preserved as well, as it can still be in use by the driver.
			CacheMaintenance::MemoryToPreserveAndReuse(iLinAddr, iSize, iAttribs);			

			MmuBase& m=*MmuBase::TheMmu;
			MmuBase::Wait();
			m.Unmap(iLinAddr,iSize);
			MmuBase::Signal();
			DeallocateLinearAddress();
			}
		K::ObjDelete(this);
		}
	return r;
	}

void DMemModelChunk::BTracePrime(TInt aCategory)
	{
	DChunk::BTracePrime(aCategory);
	
#ifdef BTRACE_CHUNKS
	if (aCategory == BTrace::EChunks || aCategory == -1)
		{
		MmuBase::Wait();

		TBool memoryOwned = !(iAttributes&EMemoryNotOwned);
		Mmu& m=Mmu::Get();
		TInt committedBase = -1;

		// look at each page table in this chunk...
		TUint chunkEndIndex = iMaxSize>>KChunkShift;
		NKern::LockSystem();
		for(TUint chunkIndex=0; chunkIndex<chunkEndIndex; ++chunkIndex)
			{
			TLinAddr addr=(TLinAddr)iBase+chunkIndex*KChunkSize;		// current address
			TInt ptid = m.GetPageTableId(addr);
			if(ptid<0)
				{
				// no page table...
				if(committedBase!=-1)
					{
					NKern::FlashSystem();
					TUint committedEnd = chunkIndex*KChunkSize;
					BTrace12(BTrace::EChunks, memoryOwned?BTrace::EChunkMemoryAllocated:BTrace::EChunkMemoryAdded,this,committedBase,committedEnd-committedBase);
					committedBase = -1;
					}
				continue;
				}
			TPte* pPte=(TPte*)m.PageTableLinAddr(ptid);

			// look at each page in page table...
			for(TUint pageIndex=0; pageIndex<KChunkSize/KPageSize; ++pageIndex)
				{
				TBool committed = false;
				TPhysAddr phys = m.PtePhysAddr(pPte[pageIndex], pageIndex);
				if(phys!=KPhysAddrInvalid)
					{
					// we have a page...
					if(!memoryOwned)
						committed = true;
					else
						{
						// make sure we own the page...
						SPageInfo* pi = SPageInfo::SafeFromPhysAddr(phys);
						if(pi && pi->Type()==SPageInfo::EChunk && pi->Owner()==this)
							committed = true;
						}
					}

				if(committed)
					{
					if(committedBase==-1)
						committedBase = chunkIndex*KChunkSize+pageIndex*KPageSize; // start of new region
					}
				else
					{
					if(committedBase!=-1)
						{
						// generate trace for region...
						NKern::FlashSystem();
						TUint committedEnd = chunkIndex*KChunkSize+pageIndex*KPageSize;
						BTrace12(BTrace::EChunks, memoryOwned?BTrace::EChunkMemoryAllocated:BTrace::EChunkMemoryAdded,this,committedBase,committedEnd-committedBase);
						committedBase = -1;
						}
					}

				if((pageIndex&15)==0)
					NKern::FlashSystem();
				}
			}
		NKern::UnlockSystem();

		if(committedBase!=-1)
			{
			TUint committedEnd = chunkEndIndex*KChunkSize;
			BTrace12(BTrace::EChunks, memoryOwned?BTrace::EChunkMemoryAllocated:BTrace::EChunkMemoryAdded,this,committedBase,committedEnd-committedBase);
			}

		MmuBase::Signal();
		}
#endif
	}
