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
//

#include <memmodel.h>
#include "mmu/mm.h"
#include "mmboot.h"


DMemModelChunk::DMemModelChunk()
	{
	}


DMemModelChunk::~DMemModelChunk()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DMemModelChunk destruct %O",this));

	MM::MappingDestroy(iKernelMapping);
	MM::MemoryDestroy(iMemoryObject);

	delete iPageBitMap;
	delete iPermanentPageBitMap;

	TDfc* dfc = iDestroyedDfc;
	if(dfc)
		dfc->QueueOnIdle();

	__KTRACE_OPT(KMEMTRACE, Kern::Printf("MT:D %d %x %O",NTickCount(),this,this));
#ifdef BTRACE_CHUNKS
	BTraceContext4(BTrace::EChunks,BTrace::EChunkDestroyed,this);
#endif
	}


TInt DMemModelChunk::Close(TAny* aPtr)
	{
	if (aPtr)
		{
		DMemModelProcess* pP=(DMemModelProcess*)aPtr;
		__NK_ASSERT_DEBUG(!iOwningProcess || iOwningProcess==pP);
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


void DMemModelChunk::SetPaging(TUint aCreateAtt)
	{
	// Only user data chunks should be able to be data paged, i.e. only those 
	// that can be created via the RChunk create methods.
	if ((iChunkType != EUserData && iChunkType != EUserSelfModCode) ||
		!(K::MemModelAttributes & EMemModelAttrDataPaging))	// Data paging device installed?
		{
		return;
		}
	// Pageable chunks must own their memory.
	__NK_ASSERT_DEBUG(!(iAttributes & EMemoryNotOwned));

	// Set the data paging attributes
	TUint dataPolicy = TheSuperPage().KernelConfigFlags() & EKernelConfigDataPagingPolicyMask;
	if (dataPolicy == EKernelConfigDataPagingPolicyNoPaging)
		{
		return;
		}
	if (dataPolicy == EKernelConfigDataPagingPolicyAlwaysPage)
		{
		iAttributes |= EDataPaged;
		return;
		}
	TUint pagingAtt = aCreateAtt & TChunkCreate::EPagingMask;
	if (pagingAtt == TChunkCreate::EPaged)
		{
		iAttributes |= EDataPaged;
		return;
		}
	if (pagingAtt == TChunkCreate::EUnpaged)
		{
		return;
		}
	// No data paging attribute specified for this chunk so use the process's
	__NK_ASSERT_DEBUG(pagingAtt == TChunkCreate::EPagingUnspec);
	DProcess* currentProcess = TheCurrentThread->iOwningProcess;
	if (currentProcess->iAttributes & DProcess::EDataPaged)
		{
		iAttributes |= EDataPaged;
		}
	}


TInt DMemModelChunk::DoCreate(SChunkCreateInfo& aInfo)
	{
	__ASSERT_COMPILE(!(EMMChunkAttributesMask & EChunkAttributesMask));
	__KTRACE_OPT(KMMU,Kern::Printf("Chunk %O DoCreate att=%08x",this,iAttributes));
	if (aInfo.iMaxSize<=0)
		return KErrArgument;

	iMaxSize = MM::RoundToPageSize(aInfo.iMaxSize);

	TInt maxpages=iMaxSize>>KPageShift;
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

	TMemoryAttributes attr = EMemoryAttributeStandard;
	TBool mapInKernel = false;
	TBool nowipe = false;
	TBool executable = false;
	TBool movable = false;
	TInt r;

	switch(iChunkType)
		{
	case EUserSelfModCode:
		executable = true;
		movable = true;
		break;

	case EUserData:
	case ERamDrive:
		movable = true;
		break;

	case EKernelMessage:
	case ESharedKernelSingle:
	case ESharedKernelMultiple:
	case ESharedIo:
		mapInKernel = true;
		r = MM::MemoryAttributes(attr,*(TMappingAttributes2*)&aInfo.iMapAttr);
		if(r!=KErrNone)
			return r;
		break;

	case EKernelData:
		nowipe = true;
		break;

	case EDllData:
		__NK_ASSERT_DEBUG(0); // invalid chunk type
	case EKernelStack:
		__NK_ASSERT_DEBUG(0); // invalid chunk type
	case EDll: // global code
		__NK_ASSERT_DEBUG(0); // invalid chunk type
	case EKernelCode:
		__NK_ASSERT_DEBUG(0); // invalid chunk type
	case EUserCode: // local code
		__NK_ASSERT_DEBUG(0); // invalid chunk type
	case ESharedKernelMirror:
		__NK_ASSERT_DEBUG(0); // invalid chunk type
	default:
		__NK_ASSERT_DEBUG(0); // invalid chunk type
		return KErrArgument;
		}

	// calculate memory type...
	TMemoryObjectType memoryType = EMemoryObjectUnpaged;
	if (iAttributes & EMemoryNotOwned)
		{
		if (memoryType != EMemoryObjectUnpaged)
			return KErrArgument;
		memoryType = EMemoryObjectHardware;
		}
	if (iAttributes & EDataPaged)
		{
		if (memoryType != EMemoryObjectUnpaged)
			return KErrArgument;
		memoryType = EMemoryObjectPaged;
		}
	if (iAttributes & ECache)
		{
		if (memoryType != EMemoryObjectUnpaged)
			return KErrArgument;
		memoryType = EMemoryObjectDiscardable;
		}
	if (memoryType == EMemoryObjectUnpaged)
		{
		if (movable)
			memoryType = EMemoryObjectMovable;
		}

	// calculate memory flags...
	TMemoryCreateFlags flags = nowipe ? EMemoryCreateNoWipe : EMemoryCreateDefault;
	flags = (TMemoryCreateFlags)(flags|EMemoryCreateUseCustomWipeByte|(iClearByte<<EMemoryCreateWipeByteShift));
	if(executable)
		flags = (TMemoryCreateFlags)(flags|EMemoryCreateAllowExecution);

	r = MM::MemoryNew(iMemoryObject,memoryType,MM::BytesToPages(iMaxSize),flags,attr);
	if(r!=KErrNone)
		return r;

	if(mapInKernel)
		{
		TInt r = MM::MappingNew(iKernelMapping, iMemoryObject, ESupervisorReadWrite, KKernelOsAsid);
		if(r!=KErrNone)
			return r; // Note, iMemoryObject will get cleaned-up when chunk is destroyed
		const TMappingAttributes2& lma = MM::LegacyMappingAttributes(attr,EUserReadWrite);
		*(TMappingAttributes2*)&iMapAttr = lma;
		}

#ifdef BTRACE_CHUNKS
	TKName nameBuf;
	Name(nameBuf);
	BTraceContextN(BTrace::EChunks,BTrace::EChunkCreated,this,iMaxSize,nameBuf.Ptr(),nameBuf.Size());
	if(iOwningProcess)
		BTrace8(BTrace::EChunks,BTrace::EChunkOwner,this,iOwningProcess);
	BTraceContext12(BTrace::EChunks,BTrace::EChunkInfo,this,iChunkType,iAttributes);
#endif
#ifdef BTRACE_FLEXIBLE_MEM_MODEL
	BTrace8(BTrace::EFlexibleMemModel,BTrace::EMemoryObjectIsChunk,iMemoryObject,this);
#endif
	return KErrNone;
	}


void DMemModelChunk::SetFixedAddress(TLinAddr aAddr, TInt aInitialSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk %O SetFixedAddress %08x size %08x",this,aAddr,aInitialSize));
	iFixedBase = aAddr;
	iSize = MM::RoundToPageSize(aInitialSize);
	if(iSize)
		MM::MemoryClaimInitialPages(iMemoryObject,iFixedBase,iSize,ESupervisorReadWrite);
	}


TInt DMemModelChunk::SetAttributes(SChunkCreateInfo& aInfo)
	{
	switch(iChunkType)
		{
		case EKernelData:
		case EKernelMessage:
			iAttributes = EPrivate;
			break;
		case ERamDrive:
			iAttributes = EPrivate;
			break;
		case EUserData:
			if (aInfo.iGlobal)
				iAttributes = EPublic;
			else
				iAttributes = EPrivate;
			break;
		case EUserSelfModCode:
			if (aInfo.iGlobal)
				iAttributes = EPublic|ECode;
			else
				iAttributes = EPrivate|ECode;
			break;
		case ESharedKernelSingle:
		case ESharedKernelMultiple:
		case ESharedIo:
			iAttributes = EPublic;
			break;
		case EDllData:
			__NK_ASSERT_DEBUG(0); // invalid chunk type
		case EKernelStack:
			__NK_ASSERT_DEBUG(0); // invalid chunk type
		case EDll: // global code
			__NK_ASSERT_DEBUG(0); // invalid chunk type
		case EKernelCode:
			__NK_ASSERT_DEBUG(0); // invalid chunk type
		case EUserCode: // local code
			__NK_ASSERT_DEBUG(0); // invalid chunk type
		case ESharedKernelMirror:
			__NK_ASSERT_DEBUG(0); // invalid chunk type
		default:
			FAULT();
		}
	return KErrNone;
	}


TInt DMemModelChunk::Adjust(TInt aNewSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Adjust %08x",aNewSize));
	if (iAttributes & (EDoubleEnded|EDisconnected))
		return KErrGeneral;
	if (aNewSize<0 || aNewSize>iMaxSize)
		return KErrArgument;

	TInt r=KErrNone;
	TInt newSize=MM::RoundToPageSize(aNewSize);
	if (newSize!=iSize)
		{
		MM::MemoryLock(iMemoryObject);
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
		MM::MemoryUnlock(iMemoryObject);
		}
	__COND_DEBUG_EVENT(r==KErrNone, EEventUpdateChunk, this);
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk %O adjusted to %x",this,iSize));
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
	TInt start = aOffset>>KPageShift;
	TInt size = ((aOffset+aSize-1)>>KPageShift)-start+1;
	if(iPermanentPageBitMap->NotAllocated(start,size))
		return KErrNotFound;
	aKernelAddress = MM::MappingBase(iKernelMapping)+aOffset;
	return KErrNone;
	}


TInt DMemModelChunk::PhysicalAddress(TInt aOffset, TInt aSize, TLinAddr& aKernelAddress, TUint32& aPhysicalAddress, TUint32* aPhysicalPageList)
	{
	if(aSize<=0)
		return KErrArgument;
	TInt r = Address(aOffset,aSize,aKernelAddress);
	if(r!=KErrNone)
		return r;
	TInt index = aOffset>>KPageShift;
	TInt count = ((aOffset+aSize-1)>>KPageShift)-index+1;
	r = MM::MemoryPhysAddr(iMemoryObject,index,count,aPhysicalAddress,aPhysicalPageList);
	if(r==KErrNone)
		aPhysicalAddress += aOffset&KPageMask;
	return r;
	}


TInt DMemModelChunk::DoCommit(TInt aOffset, TInt aSize, TCommitType aCommitType, TUint32* aExtraArg)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::DoCommit %x+%x type=%d extra=%08x",aOffset,aSize,aCommitType,aExtraArg));

	__NK_ASSERT_DEBUG(((aOffset|aSize)&KPageMask)==0);

	TInt r = KErrArgument;
	switch(aCommitType)
		{
	case DChunk::ECommitDiscontiguous:
		r = MM::MemoryAlloc(iMemoryObject, MM::BytesToPages(aOffset), MM::BytesToPages(aSize));
		break;

	case DChunk::ECommitDiscontiguousPhysical:
		r = MM::MemoryAddPages(iMemoryObject, MM::BytesToPages(aOffset), MM::BytesToPages(aSize), (TPhysAddr*)aExtraArg);
		break;

	case DChunk::ECommitContiguous:
		r = MM::MemoryAllocContiguous(iMemoryObject, MM::BytesToPages(aOffset), MM::BytesToPages(aSize), 0, *(TPhysAddr*)aExtraArg);
		break;

	case DChunk::ECommitContiguousPhysical:
		r = MM::MemoryAddContiguous(iMemoryObject, MM::BytesToPages(aOffset), MM::BytesToPages(aSize), (TPhysAddr)aExtraArg);
		break;

	case DChunk::ECommitVirtual:
	default:
		__NK_ASSERT_DEBUG(0); // Invalid commit type
		r = KErrNotSupported;
		break;
		}

	if(r==KErrNone)
		{
		iSize += aSize;
		if(iPermanentPageBitMap)
			iPermanentPageBitMap->Alloc(aOffset>>KPageShift,aSize>>KPageShift);
#ifdef BTRACE_CHUNKS
		TInt subcategory = (aCommitType & DChunk::ECommitPhysicalMask) ? BTrace::EChunkMemoryAdded : BTrace::EChunkMemoryAllocated;
		BTraceContext12(BTrace::EChunks,subcategory,this,aOffset,aSize);
#endif
		}

	return r;
	}


void DMemModelChunk::DoDecommit(TInt aOffset, TInt aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::DoDecommit %x+%x",aOffset,aSize));

	__NK_ASSERT_DEBUG(((aOffset|aSize)&KPageMask)==0);

	TUint index = MM::BytesToPages(aOffset);
	TUint count = MM::BytesToPages(aSize);
	iSize -= count*KPageSize;
	if(iAttributes&EMemoryNotOwned)
		MM::MemoryRemovePages(iMemoryObject, index, count, 0);
	else
		MM::MemoryFree(iMemoryObject, index, count);

#ifdef BTRACE_CHUNKS
	if (count != 0)
		{
		TInt subcategory = (iAttributes & EMemoryNotOwned) ? BTrace::EChunkMemoryRemoved : BTrace::EChunkMemoryDeallocated;
		BTraceContext12(BTrace::EChunks,subcategory,this,aOffset,count*KPageSize);
		}
#endif
	}


TInt DMemModelChunk::AdjustDoubleEnded(TInt aBottom, TInt aTop)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::AdjustDoubleEnded %x-%x",aBottom,aTop));
	if ((iAttributes & (EDoubleEnded|EDisconnected))!=EDoubleEnded)
		return KErrGeneral;
	if (aTop<0 || aBottom<0 || aTop<aBottom || aTop>iMaxSize)
		return KErrArgument;

	aBottom &= ~KPageMask;
	aTop = MM::RoundToPageSize(aTop);
	TInt newSize=aTop-aBottom;
	if (newSize>iMaxSize)
		return KErrArgument;

	MM::MemoryLock(iMemoryObject);
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
	MM::MemoryUnlock(iMemoryObject);
	__COND_DEBUG_EVENT(r==KErrNone, EEventUpdateChunk, this);
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk %O adjusted to %x+%x",this,iStartPos,iSize));
	return r;
	}


TInt DMemModelChunk::CheckRegion(TInt& aOffset, TInt& aSize)
	{
	if((iAttributes & (EDoubleEnded|EDisconnected))!=EDisconnected)
		return KErrGeneral;
	if(aOffset<0 || aSize<0)
		return KErrArgument;
	if(aSize==0)
		return KErrNone;

	TUint end = MM::RoundToPageSize(aOffset+aSize);
	if(end>TUint(iMaxSize))
		return KErrArgument;
	aOffset &= ~KPageMask;
	aSize = end-aOffset;
	if(end<=TUint(aOffset))
		return KErrArgument;

	return 1;
	}


TInt DMemModelChunk::Commit(TInt aOffset, TInt aSize, TCommitType aCommitType, TUint32* aExtraArg)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Commit %x+%x type=%d extra=%08x",aOffset,aSize,aCommitType,aExtraArg));

	TInt r = CheckRegion(aOffset,aSize);
	if(r<=0)
		return r;

	MM::MemoryLock(iMemoryObject);
	TInt i=aOffset>>KPageShift;
	TInt n=aSize>>KPageShift;
	if (iPageBitMap->NotFree(i,n))
		r=KErrAlreadyExists;
	else
		{
		r=DoCommit(aOffset,aSize,aCommitType,aExtraArg);
		if (r==KErrNone)
			iPageBitMap->Alloc(i,n);
		}
	MM::MemoryUnlock(iMemoryObject);
	__COND_DEBUG_EVENT(r==KErrNone, EEventUpdateChunk, this);
	return r;
	}


TInt DMemModelChunk::Allocate(TInt aSize, TInt aGuard, TInt aAlign)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Allocate %x %x %d",aSize,aGuard,aAlign));

	// the flexible memory model doesn't implement aGuard and aAlign...
	__NK_ASSERT_DEBUG(aGuard==0);
	(void)aGuard;
	__NK_ASSERT_DEBUG(aAlign==0);
	(void)aAlign;

	TInt dummyOffset = 0;
	TInt r = CheckRegion(dummyOffset,aSize);
	if(r<=0)
		return r;

	MM::MemoryLock(iMemoryObject);
	TInt n=aSize>>KPageShift;
	TInt i=iPageBitMap->AllocConsecutive(n, EFalse);		// allocate the offset
	if (i<0)
		r=KErrNoMemory;		// run out of reserved space for this chunk
	else
		{
		TInt offset=i<<KPageShift;
		__KTRACE_OPT(KMMU,Kern::Printf("Offset %x allocated",offset));
		r=DoCommit(offset,aSize);
		if (r==KErrNone)
			{
			iPageBitMap->Alloc(i,n);
			r=offset;		// if operation successful, return allocated offset
			}
		}
	MM::MemoryUnlock(iMemoryObject);
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Allocate returns %x",r));
	__COND_DEBUG_EVENT(r==KErrNone, EEventUpdateChunk, this);
	return r;
	}


TInt DMemModelChunk::Decommit(TInt aOffset, TInt aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Decommit %x+%x",aOffset,aSize));
	TInt r = CheckRegion(aOffset,aSize);
	if(r<=0)
		return r;

	MM::MemoryLock(iMemoryObject);

	TInt i=aOffset>>KPageShift;
	TInt n=aSize>>KPageShift;
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

	MM::MemoryUnlock(iMemoryObject);

	r=KErrNone;
	__COND_DEBUG_EVENT(r==KErrNone, EEventUpdateChunk, this);
	return r;
	}


TInt DMemModelChunk::Unlock(TInt aOffset, TInt aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Unlock %x+%x",aOffset,aSize));
	if(!(iAttributes&ECache))
		return KErrGeneral;
	TInt r = CheckRegion(aOffset,aSize);
	if(r<=0)
		return r;

	MM::MemoryLock(iMemoryObject);

	TInt i=aOffset>>KPageShift;
	TInt n=aSize>>KPageShift;
	if(iPageBitMap->NotAllocated(i,n))
		r = KErrNotFound;
	else
		r = MM::MemoryAllowDiscard(iMemoryObject,i,n);

	MM::MemoryUnlock(iMemoryObject);

	return r;
	}


TInt DMemModelChunk::Lock(TInt aOffset, TInt aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DMemModelChunk::Lock %x+%x",aOffset,aSize));
	if(!(iAttributes&ECache))
		return KErrGeneral;
	TInt r = CheckRegion(aOffset,aSize);
	if(r<=0)
		return r;

	r = MM::MemoryDisallowDiscard(iMemoryObject, MM::BytesToPages(aOffset), MM::BytesToPages(aSize));
	if(r!=KErrNone)
		Decommit(aOffset,aSize);

	return r;
	}


TInt DMemModelChunk::CheckAccess()
	{
	if(iOwningProcess && iOwningProcess!=TheCurrentThread->iOwningProcess)
		return KErrAccessDenied;
	return KErrNone;
	}


void DMemModelChunk::BTracePrime(TInt aCategory)
	{
	DChunk::BTracePrime(aCategory);
#ifdef BTRACE_FLEXIBLE_MEM_MODEL
	if (aCategory == BTrace::EFlexibleMemModel || aCategory == -1)
		{
		if (iMemoryObject)
			{
			MM::MemoryBTracePrime(iMemoryObject);
			BTrace8(BTrace::EFlexibleMemModel,BTrace::EMemoryObjectIsChunk,iMemoryObject,this);
			}
		}
#endif
	}


void DMemModelChunk::Substitute(TInt /*aOffset*/, TPhysAddr /*aOldAddr*/, TPhysAddr /*aNewAddr*/)
	{
	MM::Panic(MM::EUnsupportedOperation);
	}

