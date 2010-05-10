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
// e32\memmodel\epoc\multiple\mchunk.cpp
// 
//

#include "memmodel.h"
#include "cache_maintenance.h"
#include <mmubase.inl>
#include <ramalloc.h>

DMemModelChunk::DMemModelChunk()
	{
	}

TLinearSection* DMemModelChunk::LinearSection()
	{
	Mmu& m=Mmu::Get();
	TInt ar=(iAttributes&EAddressRangeMask);
	switch (ar)
		{
		case EAddressLocal:			return ((DMemModelProcess*)iOwningProcess)->iLocalSection;
		case EAddressFixed:			return NULL;
		case EAddressShared:		return m.iSharedSection;
		case EAddressUserGlobal:	return m.iUserGlobalSection;
		case EAddressKernel:		return m.iKernelSection;
		}
	MM::Panic(MM::EChunkBadAddressRange);
	return NULL;
	}

void DMemModelChunk::Destruct()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DMemModelChunk destruct %O",this));
	if (iPageTables)
		{
#ifdef _DEBUG
		TInt r;
#define SET_R_IF_DEBUG(x)	r = (x)
#else
#define SET_R_IF_DEBUG(x)	(void)(x)
#endif
		if (iAttributes & EDisconnected)
			SET_R_IF_DEBUG(Decommit(0,iMaxSize));
		else if (iAttributes & EDoubleEnded)
			SET_R_IF_DEBUG(AdjustDoubleEnded(0,0));
		else
			SET_R_IF_DEBUG(Adjust(0));
		__ASSERT_DEBUG(r==KErrNone, MM::Panic(MM::EDecommitFailed));
#ifdef _DEBUG
		// check all page tables have been freed...
		Mmu& m=Mmu::Get();
		TInt nPdes=(iMaxSize+m.iChunkMask)>>m.iChunkShift;
		for(TInt i=0; i<nPdes; i++)
			{
			__NK_ASSERT_DEBUG(iPageTables[i]==0xffff);
			}
#endif
		}
	if (iBase)
		{
		TLinearSection* s=LinearSection();
		if(s)
			{
			Mmu::Wait();
			__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::~DMemModelChunk remove region"));
			Mmu& m=Mmu::Get();
			s->iAllocator.Free( (TLinAddr(iBase)-s->iBase)>>m.iChunkShift, iMaxSize>>m.iChunkShift);
			Mmu::Signal();
			}
		}
	delete iOsAsids;
	Kern::Free(iPageTables);
	delete iPageBitMap;
	delete iPermanentPageBitMap;

	if(iKernelMirror)
		iKernelMirror->Close(NULL);

	TDfc* dfc = iDestroyedDfc;
	if (dfc)
		dfc->QueueOnIdle();

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
		if ((iAttributes&EMapTypeMask)==EMapTypeLocal)
			pP=(DMemModelProcess*)iOwningProcess;
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
	__KTRACE_OPT(KMMU,Kern::Printf("Chunk %O DoCreate att=%08x",this,iAttributes));

	__ASSERT_COMPILE(!(EMMChunkAttributesMask & EChunkAttributesMask));

	if (aInfo.iMaxSize<=0)
		return KErrArgument;

	if (iKernelMirror)
		{
		iKernelMirror->iAttributes |= iAttributes|EMemoryNotOwned;
		TInt r=iKernelMirror->DoCreate(aInfo);
		if(r!=KErrNone)
			return r;
		}

	Mmu& m=Mmu::Get();
	TInt nPdes=(aInfo.iMaxSize+m.iChunkMask)>>m.iChunkShift;
	iMaxSize=nPdes<<m.iChunkShift;
	iMapAttr = aInfo.iMapAttr;
	SetupPermissions();
	TInt mapType=iAttributes & EMapTypeMask;
	if (mapType==EMapTypeShared)
		{
		iOsAsids=TBitMapAllocator::New(m.iNumOsAsids,ETrue);
		if (!iOsAsids)
			return KErrNoMemory;
		}
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
	iPageTables=(TUint16*)Kern::Alloc(nPdes*sizeof(TUint16));
	if (!iPageTables)
		return KErrNoMemory;
	memset(iPageTables,0xff,nPdes*sizeof(TUint16));
	MmuBase::Wait();
	TInt r=AllocateAddress();
	__KTRACE_OPT(KMEMTRACE,Kern::Printf("MT:C %d %x %O",NTickCount(),this,this));
	MmuBase::Signal();
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

void DMemModelChunk::ClaimInitialPages()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Chunk %O ClaimInitialPages()",this));
	Mmu& m=Mmu::Get();
	TInt offset=0;
	TUint32 ccp=K::CompressKHeapPtr(this);
	NKern::LockSystem();
	while(offset<iSize)
		{
		TInt ptid=m.PageTableId(TLinAddr(iBase)+offset);
		__ASSERT_ALWAYS(ptid>=0,MM::Panic(MM::EClaimInitialPagesBadPageTable));
		__KTRACE_OPT(KMMU,Kern::Printf("Offset %x PTID=%d",offset,ptid));
		iPageTables[offset>>m.iChunkShift]=ptid;
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
				SPageInfo* info = SPageInfo::SafeFromPhysAddr(phys);
				if(info)
					{
					info->SetChunk(this,offset>>m.iPageShift);
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
	}

void DMemModelChunk::SetFixedAddress(TLinAddr aAddr, TInt aInitialSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk %O SetFixedAddress %08x size %08x",this,aAddr,aInitialSize));
	iBase=(TUint8*)aAddr;
	iSize=Mmu::RoundToPageSize(aInitialSize);
	ClaimInitialPages();
	}

TInt DMemModelChunk::Reserve(TInt aInitialSize)
//
// Reserve home section address space for a chunk
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk %O Reserve() size %08x",this,aInitialSize));
	iSize=Mmu::RoundToPageSize(aInitialSize);
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
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk %O adjusted to %x base %08x",this,iSize,iBase));
	return r;
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
	aKernelAddress = (TLinAddr)iKernelMirror->iBase+aOffset;
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
	// Enter and leave with system locked.
	// This is sometimes called with interrupts disabled and should leave them alone.
	Mmu& m = Mmu::Get();
	__ASSERT_ALWAYS(iKernelMirror==NULL,MM::Panic(MM::EChunkRemapUnsupported));
	
	TInt ptid=iPageTables[aOffset>>m.iChunkShift];
	if(ptid==0xffff)
		MM::Panic(MM::EChunkRemapNoPageTable);

	// Permissions for global code will have been overwritten with ApplyPermissions
	// so we can't trust iPtePermissions for those chunk types
	TPte perms;
   	if(iChunkType==EKernelCode)
		perms = m.iKernelCodePtePerm;
	else if(iChunkType==EDll)
		perms = m.iGlobalCodePtePerm;
	else
		perms = iPtePermissions;

	m.RemapPage(ptid, (TLinAddr)iBase+aOffset, aOldAddr, aNewAddr, perms, iOwningProcess);
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
	__ASSERT_MUTEX(MmuBase::RamAllocatorMutex);
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::DoCommit %x+%x type=%d extra=%08x",aOffset,aSize,aCommitType,aExtraArg));
	TInt offset=aOffset;
	TInt endOffset=offset+aSize;
	TInt newPtId=-1;
	Mmu& m = Mmu::Get();
	DRamAllocator& a = *m.iRamPageAllocator;
	TInt r=KErrNone;
	TPhysAddr pageList[KMaxPages];
	TPhysAddr* pPageList=0; 	// In case of discontiguous commit it points to the list of physical pages.
	TPhysAddr nextPage=0; 		// In case of contiguous commit, it points to the physical address to commit
	SPageInfo::TType type = SPageInfo::EChunk;

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
		if(!physicalCommit && aCommitType != DChunk::ECommitVirtual)
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

	case DChunk::ECommitVirtual:
#ifndef __MARM__
		return KErrNotSupported;
#endif
		break;

	default:
		return KErrNotSupported;
		}

	while(offset<endOffset)
		{
		TInt np=(endOffset-offset)>>m.iPageShift;	// pages remaining to satisfy request
		TInt npEnd=(m.iChunkSize-(offset&m.iChunkMask))>>m.iPageShift;// number of pages to end of page table
		if (np>npEnd)
			np=npEnd;								// limit to single page table
		if (np>MM::MaxPagesInOneGo)
			np=MM::MaxPagesInOneGo;					// limit
		TInt ptid=iPageTables[offset>>m.iChunkShift];
		newPtId=-1;
		if (ptid==0xffff)
			{
			// need to allocate a new page table
			newPtId=m.AllocPageTable();
			if (newPtId<0)
				{
				r=KErrNoMemory;
				break;	// Exit the loop. Below, we'll free all ram
						// that is allocated in the previous loop passes.
				}
			ptid=newPtId;
			}

		if(aCommitType==DChunk::ECommitDiscontiguous)
			{
			pPageList = pageList;
			r=m.AllocRamPages(pPageList,np, GetPageType());	// try to allocate pages
			if (r!=KErrNone)  //If fail, clean up what was allocated in this loop.
				{
				if (newPtId>=0)
					m.FreePageTable(newPtId);
					break;	// Exit the loop. Below, we'll free all ram
							// that is allocated in the previous loop passes.
				}
			if(clearRam)
				m.ClearPages(np, pPageList, iClearByte);	// clear RAM if required
			}

		TInt commitSize = np<<m.iPageShift;
		

		// In shared chunks (visible to both user and kernel side), it is always kernel side
		// to be mapped the first. Decommiting will go in reverse order.
		if(iKernelMirror)
			{
			// Map the same memory into the kernel mirror chunk
			if(pPageList)
				r = iKernelMirror->DoCommit(offset,commitSize,ECommitDiscontiguousPhysical,pPageList);
			else
				r = iKernelMirror->DoCommit(offset,commitSize,ECommitContiguousPhysical,(TUint32*)nextPage);
			__KTRACE_OPT(KMMU,Kern::Printf("iKernelMirror->DoCommit returns %d",r));
			if(r!=KErrNone) //If fail, clean up what was allocated in this loop.
				{
				if(aCommitType==DChunk::ECommitDiscontiguous)
					m.FreePages(pPageList,np,EPageFixed);
				if (newPtId>=0)
					m.FreePageTable(newPtId);
				
				break;	// Exit the loop. Below, we'll free all ram
						// that is allocated in the previous loop passes.
				}
			}

		// Commit the memory.
		NKern::LockSystem(); // lock the system while we change the MMU mappings
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

		if (newPtId>=0)
			{
			// We have allocated a new page table, now we must assign it
			iPageTables[offset>>m.iChunkShift]=ptid;
			TLinAddr addr=(TLinAddr)iBase+offset;	// current address
			m.AssignPageTable(ptid, SPageTableInfo::EChunk, this, addr, iPdePermissions);
			newPtId = -1;
			}
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
		// We ran out of memory somewhere.
		// Free any memory we succeeded in allocating in the loops before the one that failed
		if (iChunkType != ESharedKernelMirror) //Kernel mirror chunk will be decommited alongside the main chunk.
			{
			DChunk::TDecommitType decommitType = aCommitType==DChunk::ECommitVirtual ?
																DChunk::EDecommitVirtual : DChunk::EDecommitNormal;
			DoDecommit(aOffset,offset-aOffset,decommitType);
			}

		if(aCommitType==DChunk::ECommitContiguous)
			{
			// Free the pages we allocated but didn't get around to commiting
			// It has to go page after page as we cannot use FreePhysicalRam here because the part of
			// of original allocated contiguous memory is already partly freed (in DoDecommit).
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
	// enter and leave with system unlocked
	// must hold RamAlloc mutex before calling this function
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::DoDecommit %x+%x",aOffset,aSize));
	
	TBool ownsMemory = !(iAttributes&EMemoryNotOwned);

	TInt deferred=0;
	TInt offset=aOffset;
	TInt endOffset=offset+aSize;
	Mmu& m = Mmu::Get();
	DRamAllocator& a = *m.iRamPageAllocator;
	TPhysAddr pageList[KMaxPages];
	TLinAddr linearPageList[KMaxPages];
	const TAny* asids=GLOBAL_MAPPING;
	if (iOsAsids)
		asids=iOsAsids;
	else if (iOwningProcess)
		asids=(const TAny*)((DMemModelProcess*)iOwningProcess)->iOsAsid;
	TUint size_in_pages = (TUint)(Min(aSize,iSize)>>m.iPageShift);
	TBool sync_decommit = (size_in_pages<m.iDecommitThreshold);
	TInt total_freed=0;
	while(offset<endOffset)
		{
		TInt np=(endOffset-offset)>>m.iPageShift;		// number of pages remaining to decommit
		TInt pdeEnd=(offset+m.iChunkSize)&~m.iChunkMask;
		TInt npEnd=(pdeEnd-offset)>>m.iPageShift;		// number of pages to end of page table
		if (np>npEnd)
			np=npEnd;									// limit to single page table
		if (np>MM::MaxPagesInOneGo)
			np=MM::MaxPagesInOneGo;						// limit
		TLinAddr addr=(TLinAddr)iBase+offset;			// current address
		TInt ptid=iPageTables[offset>>m.iChunkShift];	// get page table ID if a page table is already assigned here
		if (ptid!=0xffff)
			{
			TInt nPtes=0;
			TInt nUnmapped=0;

#ifdef BTRACE_CHUNKS
			TUint oldFree = m.FreeRamInBytes();
#endif
			// Unmap the pages, clear the PTEs and place the physical addresses of the now-free RAM pages in
			// pageList. Return nPtes=number of pages placed in list, remain=number of PTEs remaining in page table
			// Bit 31 of return value is set if TLB flush may be incomplete
			NKern::LockSystem();
			TInt remain;
			if (ownsMemory)
				{
				if (aDecommitType == EDecommitVirtual)
					remain=m.UnmapVirtual(ptid,addr,np,pageList,ETrue,nPtes,nUnmapped,iOwningProcess);
				else
					remain=m.UnmapPages(ptid,addr,np,pageList,ETrue,nPtes,nUnmapped,iOwningProcess);
				}
			else
				{
				if (aDecommitType == EDecommitVirtual)
					remain=m.UnmapUnownedVirtual(ptid,addr,np,pageList,linearPageList,nPtes,nUnmapped,iOwningProcess);
				else
					remain=m.UnmapUnownedPages(ptid,addr,np,pageList,linearPageList,nPtes,nUnmapped,iOwningProcess);
				}
            TInt nFree = ownsMemory ? nUnmapped : 0; //The number of pages to free
			deferred |= remain;
			TInt decommitSize=nPtes<<m.iPageShift;
			iSize-=decommitSize;                // reduce the committed size
			NKern::UnlockSystem();

			

			__KTRACE_OPT(KMEMTRACE,Kern::Printf("MT:A %d %x %x %O",NTickCount(),this,iSize,this));
#ifdef BTRACE_CHUNKS
			TUint reclaimed = (oldFree-m.FreeRamInBytes())>>m.iPageShift; // number of 'unlocked' pages reclaimed from ram cache
			if(nFree-reclaimed)
				BTraceContext12(BTrace::EChunks,ownsMemory?BTrace::EChunkMemoryDeallocated:BTrace::EChunkMemoryRemoved,this,offset,(nFree-reclaimed)<<m.iPageShift);
#endif

			if (sync_decommit && (remain & KUnmapPagesTLBFlushDeferred))
				{
				// must ensure DTLB flushed before doing cache purge on decommit
				m.GenericFlush(Mmu::EFlushDTLB);
				}

			// if page table is now completely empty, unassign it and update chunk PDE info
			remain &= KUnmapPagesCountMask;
			if (remain==0)
				{
				m.DoUnassignPageTable(addr, asids);
				m.FreePageTable(ptid);
				iPageTables[offset>>m.iChunkShift]=0xffff;
				}

			// Physical memory not owned by the chunk has to be preserved from cache memory.
			if(!ownsMemory)
				{
				// If a chunk has Kernel mirror, it is sufficient to do it just once.
				if (!iKernelMirror)
					{
					TInt i;
					for (i=0;i<nUnmapped;i++)
						m.CacheMaintenanceOnPreserve(pageList[i], KPageSize, linearPageList[i], iMapAttr);
					}
				}
			else if (nFree)
				{
	            // We can now return the decommitted pages to the free page list and sort out caching.
				total_freed+=nFree;
				if (sync_decommit) //Purge cache if the size is below decommit threshold
					m.CacheMaintenanceOnDecommit(pageList, nFree);
				a.FreeRamPages(pageList,nFree, GetPageType());
				}

			offset+=(np<<m.iPageShift);
			}
		else
			{
			__KTRACE_OPT(KMMU,Kern::Printf("No page table at %08x",addr));
			if ((iAttributes&EDisconnected)==0)
				MM::Panic(MM::EChunkDecommitNoPageTable);
			offset=pdeEnd;	// disconnected chunk - step on to next PDE
			}
		}
	if (deferred & KUnmapPagesTLBFlushDeferred)
		m.GenericFlush( (iAttributes&ECode) ? Mmu::EFlushDTLB|Mmu::EFlushITLB : Mmu::EFlushDTLB );
	
	if (total_freed && !sync_decommit) //Flash entire cache if the size exceeds decommit threshold
		CacheMaintenance::SyncPhysicalCache_All();		//On ARMv6, this deals with both L1 & L2 cache

	// Kernel mapped part of the chunk is removed at the end. At this point, no user side is mapped 
	// which ensures that evicting data from cache will surely succeed.
	if(iKernelMirror)
		iKernelMirror->DoDecommit(aOffset,aSize);
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
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk %O adjusted to %x+%x base %08x",this,iStartPos,iSize,iBase));
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
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Allocate %x %x %d",aSize,aGuard,aAlign));

	// Only allow this to be called on disconnected chunks and not disconnected 
	// cache chunks as when guards pages exist the bit map can't be used to determine
	// the size of disconnected cache chunks as is required by Decommit().
	if ((iAttributes & (EDoubleEnded|EDisconnected|ECache))!=EDisconnected)
		return KErrGeneral;

	if (aSize<=0 || aGuard<0)
		return KErrArgument;
	Mmu& m = Mmu::Get();
	aAlign=Max(aAlign-m.iPageShift,0);
	TInt base=TInt(TLinAddr(iBase)>>m.iPageShift);
	aSize=(aSize+m.iPageMask)&~m.iPageMask;
	aGuard=(aGuard+m.iPageMask)&~m.iPageMask;
	if ((aSize+aGuard)>iMaxSize)
		return KErrArgument;

	Mmu::Wait();
	TInt r=KErrNone;
	TInt n=(aSize+aGuard)>>m.iPageShift;
	TInt i=iPageBitMap->AllocAligned(n,aAlign,base,EFalse);		// allocate the offset
	if (i<0)
		r=KErrNoMemory;		// run out of reserved space for this chunk
	else
		{
		TInt offset=i<<m.iPageShift;
		__KTRACE_OPT(KMMU,Kern::Printf("Offset %x allocated",offset));
		r=DoCommit(offset+aGuard,aSize);
		if (r==KErrNone)
			{
			iPageBitMap->Alloc(i,n);
			r=offset;		// if operation successful, return allocated offset
			}
		}
	Mmu::Signal();
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Allocate returns %x",r));
	__COND_DEBUG_EVENT(r==KErrNone, EEventUpdateChunk, this);
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
// Decommit from a disconnected chunk
// 
// @param aDecommitType Used to indicate whether area was originally committed with the
// 					  	ECommitVirtual type
//
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Decommit %x+%x",aOffset,aSize));
	if ((iAttributes & (EDoubleEnded|EDisconnected))!=EDisconnected)
		return KErrGeneral;
	if (aOffset<0 || aSize<0)
		return KErrArgument;
	if (aSize==0)
		return KErrNone;
#ifndef __MARM__
	if (aDecommitType == EDecommitVirtual)
		return KErrNotSupported;
#endif
	Mmu& m = Mmu::Get();
	aSize+=(aOffset & m.iPageMask);
	aOffset &= ~m.iPageMask;
	aSize=(aSize+m.iPageMask)&~m.iPageMask;
	if ((aOffset+aSize)>iMaxSize)
		return KErrArgument;

	Mmu::Wait();

	// limit the range to the home region range
	__KTRACE_OPT(KMMU,Kern::Printf("Rounded and Clipped range %x+%x",aOffset,aSize));

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
		r=m.UnlockRamCachePages((TLinAddr)(iBase+aOffset),n,iOwningProcess);
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
		r=m.LockRamCachePages((TLinAddr)(iBase+aOffset),n,iOwningProcess);
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

TInt DMemModelChunk::AllocateAddress()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Chunk %O AllocateAddress()",this));
	TLinearSection* s=LinearSection();
	if (!s)
		return KErrNone;				// chunk has fixed preallocated address

	Mmu& m=Mmu::Get();
	TUint32 required=iMaxSize>>m.iChunkShift;
	__KTRACE_OPT(KMMU,Kern::Printf("Searching from low to high addresses"));
	TInt r=s->iAllocator.AllocConsecutive(required, EFalse);
	if (r<0)
		return KErrNoMemory;
	s->iAllocator.Alloc(r, required);
	iBase=(TUint8*)(s->iBase + (r<<m.iChunkShift));
	__KTRACE_OPT(KMMU,Kern::Printf("Address %08x allocated",iBase));
	return KErrNone;
	}

void DMemModelChunk::ApplyPermissions(TInt aOffset, TInt aSize, TPte aPtePerm)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Chunk %O ApplyPermissions(%x+%x,%08x)",this,aOffset,aSize,aPtePerm));
	__ASSERT_ALWAYS(aOffset>=0 && aSize>=0, MM::Panic(MM::EChunkApplyPermissions1));
	if (aSize==0)
		return;
	Mmu& m=Mmu::Get();
	aOffset &= ~m.iPageMask;
	aSize=(aSize+m.iPageMask)&~m.iPageMask;
	TInt endOffset=aOffset+aSize;
	__ASSERT_ALWAYS(endOffset<=iMaxSize, MM::Panic(MM::EChunkApplyPermissions2));

	Mmu::Wait();
	while(aOffset<endOffset)
		{
		TInt ptid=iPageTables[aOffset>>m.iChunkShift];
		TInt pdeEnd=(aOffset+m.iChunkSize)&~m.iChunkMask;
		if (ptid==0xffff)
			{
			aOffset=pdeEnd;
			continue;
			}
		TInt np=(endOffset-aOffset)>>m.iPageShift;		// number of pages remaining to process
		TInt npEnd=(pdeEnd-aOffset)>>m.iPageShift;		// number of pages to end of page table
		if (np>npEnd)
			np=npEnd;									// limit to single page table
		if (np>MM::MaxPagesInOneGo)
			np=MM::MaxPagesInOneGo;						// limit
		m.ApplyPagePermissions(ptid, (aOffset&m.iChunkMask)>>m.iPageShift, np, aPtePerm);
		aOffset+=(np<<m.iPageShift);
		}
	Mmu::Signal();
	}

TInt DMemModelChunkHw::Close(TAny*)
	{
	__KTRACE_OPT(KOBJECT,Kern::Printf("DMemModelChunkHw::Close %d %O",AccessCount(),this));
	TInt r=Dec();
	if (r==1)
		{
		if (iLinAddr)
			{
			// Save data for cache maintenance before beind destroyed by DeallocateLinearAddress
			TPhysAddr pa = iPhysAddr;
			TLinAddr la = iLinAddr;
			TInt size = iSize;
			TUint attr = iAttribs;
			
			MmuBase& m=*MmuBase::TheMmu;
			MmuBase::Wait();
			m.Unmap(iLinAddr,iSize);
			MmuBase::Signal();
			DeallocateLinearAddress();

			// Physical memory has to be evicted from cache(s).
			// Must be preserved as it can still be in use by the driver.
			MmuBase::Wait();
			m.CacheMaintenanceOnPreserve(pa, size ,la ,attr);
			MmuBase::Signal();
			}
		K::ObjDelete(this);
		}
	return r;
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


void DMemModelChunk::BTracePrime(TInt aCategory)
	{
	DChunk::BTracePrime(aCategory);
	
#ifdef BTRACE_CHUNKS
	if (aCategory == BTrace::EChunks || aCategory == -1)
		{
		MmuBase::Wait();

		TBool memoryOwned = !(iAttributes&EMemoryNotOwned);
		MmuBase& m=*MmuBase::TheMmu;
		TInt committedBase = -1;

		// look at each page table in this chunk...
		TUint chunkEndIndex = iMaxSize>>KChunkShift;
		for(TUint chunkIndex=0; chunkIndex<chunkEndIndex; ++chunkIndex)
			{
			TInt ptid = iPageTables[chunkIndex];
			if(ptid==0xffff)
				{
				// no page table...
				if(committedBase!=-1)
					{
					TUint committedEnd = chunkIndex*KChunkSize;
					BTrace12(BTrace::EChunks, memoryOwned?BTrace::EChunkMemoryAllocated:BTrace::EChunkMemoryAdded,this,committedBase,committedEnd-committedBase);
					committedBase = -1;
					}
				continue;
				}

			TPte* pPte=(TPte*)m.PageTableLinAddr(ptid);

			// look at each page in page table...
			NKern::LockSystem();
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

			NKern::UnlockSystem();
			}

		if(committedBase!=-1)
			{
			TUint committedEnd = chunkEndIndex*KChunkSize;
			BTrace12(BTrace::EChunks, memoryOwned?BTrace::EChunkMemoryAllocated:BTrace::EChunkMemoryAdded,this,committedBase,committedEnd-committedBase);
			}

		MmuBase::Signal();
		}
#endif
	}
