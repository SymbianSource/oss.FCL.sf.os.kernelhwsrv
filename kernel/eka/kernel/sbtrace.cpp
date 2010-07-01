// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\sbtrace.cpp
// 
//

#include <kernel/kern_priv.h>
#include "execs.h"
#include <e32panic.h>
#include "memmodel.h"

//SBTraceData BTraceData = { {0},0,0 };


TBool DummyBTraceHandler(TUint32,TUint32,const TUint32,const TUint32,const TUint32,const TUint32,const TUint32,const TUint32)
	{
	return EFalse;
	}


void BTrace::Init0()
	{
	BTrace::SetHandler(DummyBTraceHandler);
	TUint32* src = Kern::SuperPage().iInitialBTraceFilter;
	TUint32* srcEnd = src+256/32;

	// always have EMetaTrace enabled if any trace category is enabled...
	TUint32 anySet = 0;
	TUint32* scan = src;
	do anySet |= *scan++;
	while(scan<srcEnd);
	if(anySet)
		SetFilter(BTrace::EMetaTrace,1);

	TInt category = 0;
	do
		{
		TUint32 bits = *src++;
		do
			{
			if(category!=BTrace::EMetaTrace)
				SetFilter(category,(bits&1));
			++category;
			bits >>= 1;
			}
		while(category&31);
		}
	while(src<srcEnd);
	}


EXPORT_C TInt BTrace::Control(BTrace::TControl aFunction, TAny* aArg1, TAny* aArg2)
	{
	return (*BTraceData.iControl)(aFunction, aArg1, aArg2);
	}


EXPORT_C BTrace::THandler BTrace::SetHandler(BTrace::THandler aHandler)
	{
	BTrace::TControlFunction oldControl;
	BTrace::THandler oldHandler;
	SetHandlers(aHandler,0,oldHandler,oldControl);
	return oldHandler;
	}

void TraceFastMutexName(NFastMutex* aMutex, const char* aName)
	{
	TPtrC8 name((const TUint8*)aName);
	BTraceN(BTrace::EFastMutex, BTrace::EFastMutexName, aMutex, 0, name.Ptr(), name.Length());
	}

void TraceDObject(DObject* aObj, TUint aCat, TUint aSub, const char* aName)
	{
	if (!aObj)
		return;
	DObject* owner = aObj->iOwner;
	if (!aObj->iName && aName)
		{
		TPtrC8 name((const TUint8*)aName);
		BTraceN(aCat, aSub, aObj, owner, name.Ptr(), name.Size());
		}
	else
		{
		TKName nameBuf;
		aObj->Name(nameBuf);
		BTraceN(aCat, aSub, aObj, owner, nameBuf.Ptr(), nameBuf.Size());
		}
	}

// IMPORTANT, this function must not be used for objects which have overridden Close()
// because the use of AsyncClose() by this function would then be unsafe
void TraceContainerContents(DObjectCon* aCon, TUint aCat, TUint aSub)
	{
	if (!aCon)
		return;
	NKern::ThreadEnterCS();
	aCon->Wait();
	TInt num = aCon->Count();
	for (TInt i=0; i<num; i++)
		{
		DObject* obj = (DObject*)(*aCon)[i];
		if (obj->Open() == KErrNone)
			{
			TraceDObject(obj, aCat, aSub, 0);
			obj->AsyncClose();
			}
		}
	aCon->Signal();
	NKern::ThreadLeaveCS();
	}


EXPORT_C void BTrace::Prime(TInt aCategory)
	{
	(void)aCategory;
#ifdef BTRACE_CPU_USAGE
	if(aCategory==BTrace::ECpuUsage || aCategory==-1)
		{
		BTraceContext0(BTrace::ECpuUsage,BTrace::ENewThreadContext);
		}
#endif

#if defined(BTRACE_THREAD_IDENTIFICATION) || defined(BTRACE_FLEXIBLE_MEM_MODEL)
	if(aCategory==BTrace::EThreadIdentification || aCategory==BTrace::EFlexibleMemModel || aCategory==-1)
		{
		DObjectCon* processes=Kern::Containers()[EProcess];
		if(processes)
			{
			NKern::ThreadEnterCS();
			DCodeSeg::Wait();	// FMM implementation needs to traverse code seg graph
			processes->Wait();
			TInt numProcesses = processes->Count();
			for(TInt i=0; i<numProcesses; i++)
				{
				DProcess* process = (DProcess*)(*processes)[i];
				if (process->Open() == KErrNone)
					{
					process->BTracePrime(aCategory);
					process->AsyncClose();
					}
				}
			processes->Signal();
			DCodeSeg::Signal();
			NKern::ThreadLeaveCS();
			}
		}
#endif

#if defined(BTRACE_THREAD_IDENTIFICATION) || defined(BTRACE_FLEXIBLE_MEM_MODEL)
	if(aCategory==BTrace::EThreadIdentification || aCategory==BTrace::EFlexibleMemModel || aCategory==-1)
		{
		DObjectCon* threads=Kern::Containers()[EThread];
		if(threads)
			{
			NKern::ThreadEnterCS();
			threads->Wait();
			TInt numThread = threads->Count();
			for(TInt i=0; i<numThread; i++)
				{
				DThread* thread = (DThread*)(*threads)[i];
				if (thread->Open() == KErrNone)
					{
					thread->BTracePrime(aCategory);
					thread->AsyncClose();
					}
				}
			threads->Signal();
			NKern::ThreadLeaveCS();
			}
		}
#endif
#if defined(BTRACE_CHUNKS) || defined(BTRACE_FLEXIBLE_MEM_MODEL)
	if(aCategory==BTrace::EChunks || aCategory==BTrace::EFlexibleMemModel || aCategory==-1)
		{
		DObjectCon* chunks=Kern::Containers()[EChunk];
		if(chunks)
			{
			NKern::ThreadEnterCS();
			chunks->Wait();
			TInt num = chunks->Count();
			for(TInt i=0; i<num; i++)
				{
				DChunk* chunk = (DChunk*)(*chunks)[i];
				if (chunk->Open() == KErrNone)
					{
					chunk->BTracePrime(aCategory);
					chunk->AsyncClose();
					}
				}
			chunks->Signal();
			NKern::ThreadLeaveCS();
			}
		}
#endif
#if defined(BTRACE_CODESEGS) || defined(BTRACE_FLEXIBLE_MEM_MODEL)
	if(aCategory==BTrace::ECodeSegs || aCategory==BTrace::EFlexibleMemModel || aCategory==-1)
		{
		NKern::ThreadEnterCS();
		DCodeSeg::Wait();
		SDblQueLink* anchor=&DCodeSeg::GlobalList.iA;
		SDblQueLink* pL=anchor->iNext;
		for (; pL!=anchor; pL=pL->iNext)
			{
			DCodeSeg* seg=_LOFF(pL,DCodeSeg,iLink);
			seg->CheckedOpen();
			seg->BTracePrime(aCategory);
			seg->CheckedClose();
			}
		DCodeSeg::Signal();
		NKern::ThreadLeaveCS();
		}
#endif
#ifdef BTRACE_PAGING
	if(aCategory==BTrace::EPaging || aCategory==-1)
		{
		BTrace4(BTrace::EPaging,BTrace::EPagingMemoryModel,K::MemModelAttributes & EMemModelTypeMask);
		}
#endif
#ifdef BTRACE_THREAD_PRIORITY
	if(aCategory==BTrace::EThreadPriority || aCategory==-1)
		{
		DObjectCon* threads=Kern::Containers()[EThread];
		if(threads)
			{
			NKern::ThreadEnterCS();
			threads->Wait();
			TInt numThread = threads->Count();
			for(TInt i=0; i<numThread; i++)
				{
				DThread* thread = (DThread*)(*threads)[i];
				DProcess* process = thread->iOwningProcess;
				NThread* nThread = &thread->iNThread;
				BTrace8(BTrace::EThreadPriority,BTrace::EProcessPriority,process,process->iPriority);
				BTrace12(BTrace::EThreadPriority,BTrace::EDThreadPriority,nThread,thread->iThreadPriority,thread->iDefaultPriority);
				BTrace8(BTrace::EThreadPriority,BTrace::ENThreadPriority,nThread,nThread->iPriority);
				}
			threads->Signal();
			NKern::ThreadLeaveCS();
			}
		}
#endif

#ifdef BTRACE_KERNEL_MEMORY
	if(aCategory==BTrace::EKernelMemory || aCategory==-1)
		M::BTracePrime(aCategory);
#endif

#ifdef BTRACE_RAM_ALLOCATOR
	if (aCategory == BTrace::ERamAllocator || aCategory == -1)
		M::BTracePrime(aCategory);
#endif

#ifdef BTRACE_FAST_MUTEX
	if (aCategory == BTrace::EFastMutex || aCategory == -1)
		{
		// Log the Name and Address of the system lock
		TraceFastMutexName(&TheScheduler.iLock, "System Lock");
		TraceFastMutexName(&TMessageQue::MsgLock, "MsgLock");
		TraceFastMutexName(&DObject::Lock, "ObjLock");
		TraceFastMutexName(&TLogon::LogonLock, "LogonLock");
		}
#endif

#ifdef BTRACE_SYMBIAN_KERNEL_SYNC
	if (aCategory == BTrace::ESymbianKernelSync || aCategory == -1)
		{
		TInt i;
		for (i=0; i<ENumObjectTypes; ++i)
			TraceDObject(K::Containers[i]->Lock(), BTrace::ESymbianKernelSync, BTrace::EMutexCreate, 0);
		TraceDObject(RObjectIx::HandleMutex, BTrace::ESymbianKernelSync, BTrace::EMutexCreate, 0);
		TraceDObject(DCodeSeg::CodeSegLock, BTrace::ESymbianKernelSync, BTrace::EMutexCreate, 0);
		TraceDObject(TTickQ::Mutex, BTrace::ESymbianKernelSync, BTrace::EMutexCreate, 0);
		TraceDObject(K::MachineConfigMutex, BTrace::ESymbianKernelSync, BTrace::EMutexCreate, 0);
		TraceDObject(((RHeapK*)K::Allocator)->Mutex(), BTrace::ESymbianKernelSync, BTrace::EMutexCreate, 0);
		TraceContainerContents(K::Containers[ESemaphore], BTrace::ESymbianKernelSync, BTrace::ESemaphoreCreate);
		TraceContainerContents(K::Containers[EMutex], BTrace::ESymbianKernelSync, BTrace::EMutexCreate);
		TraceContainerContents(K::Containers[ECondVar], BTrace::ESymbianKernelSync, BTrace::ECondVarCreate);
		}
#endif

#ifdef BTRACE_CLIENT_SERVER
	if(aCategory==BTrace::EClientServer || aCategory==-1)
		{
		DObjectCon* servers=Kern::Containers()[EServer];
		if(servers)
			{
			NKern::ThreadEnterCS();
			servers->Wait();
			TInt num = servers->Count();
			for(TInt i=0; i<num; i++)
				{
				DServer* server = (DServer*)(*servers)[i];
				if (server->Open() == KErrNone)
					{
					server->BTracePrime(aCategory);
					server->AsyncClose();
					}
				}
			servers->Signal();
			NKern::ThreadLeaveCS();
			}

		DObjectCon* sessions=Kern::Containers()[ESession];
		if(sessions)
			{
			NKern::ThreadEnterCS();
			sessions->Wait();
			TInt num = sessions->Count();
			for(TInt i=0; i<num; i++)
				{
				DSession* session = (DSession*)(*sessions)[i];
				if (session->Open() == KErrNone)
					{
					session->BTracePrime(aCategory);
					session->AsyncClose();
					}
				}
			sessions->Signal();
			NKern::ThreadLeaveCS();
			}
		}
#endif
	}

TBool BTrace::IsSupported(TUint aCategory)
	{
	if(aCategory>255)
		return EFalse;
	switch(aCategory)
		{
	// traces which are always supported...
	case ERDebugPrintf:
	case EKernPrintf:
	case EKernPerfLog:
	case EProfiling:
	case ETest1:
	case ETest2:
		return ETrue;

	// traces which are conditional...

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	case EPlatsecPrintf:
		if(TheSuperPage().KernelConfigFlags() & EKernelConfigPlatSecDiagnostics)
			return ETrue;
		return EFalse;
#endif

#ifdef BTRACE_THREAD_IDENTIFICATION
	case EThreadIdentification:
		return ETrue;
#endif

#ifdef BTRACE_CPU_USAGE
	case ECpuUsage:
		return ETrue;
#endif

#ifdef BTRACE_CLIENT_SERVER
	case EClientServer:
		return ETrue;
#endif

#ifdef BTRACE_REQUESTS
	case ERequests:
		return ETrue;
#endif

#ifdef BTRACE_CHUNKS
	case EChunks:
		return ETrue;
#endif

#ifdef BTRACE_CODESEGS
	case ECodeSegs:
		return ETrue;
#endif

#ifdef BTRACE_PAGING
	case EPaging:
		return ETrue;
#endif

#ifdef BTRACE_THREAD_PRIORITY
	case EThreadPriority:
		return ETrue;
#endif

#ifdef BTRACE_PAGING_MEDIA
	case EPagingMedia:
		return ETrue;
#endif

#ifdef BTRACE_KERNEL_MEMORY
	case EKernelMemory:
		return ETrue;
#endif

	case EHeap:
	case EMetaTrace:
		return ETrue;

#ifdef BTRACE_RAM_ALLOCATOR
	case ERamAllocator:
		return ETrue;
#endif

#ifdef BTRACE_FAST_MUTEX
	case EFastMutex:
		return ETrue;
#endif

#ifdef BTRACE_RESOURCE_MANAGER
    case EResourceManager:
       return ETrue;

#endif

#ifdef BTRACE_RESMANUS
    case EResourceManagerUs:
       return ETrue;
#endif

    case EIic:
       return ETrue;

#ifdef BTRACE_TRAWEVENT
	case ERawEvent:
		return ETrue;
#endif

#ifdef BTRACE_SYMBIAN_KERNEL_SYNC
	case ESymbianKernelSync:
		return ETrue;
#endif

#ifdef BTRACE_FLEXIBLE_MEM_MODEL
	case EFlexibleMemModel:
		return ETrue;
#endif

#ifdef __SMP__
	case EHSched:
		return ETrue;
#endif

	default:
		return aCategory>=128; // all categories >=128 are 'supported'
		}
	}


//
// DBTraceFilter2
//

#ifdef __SMP__
TSpinLock BTraceFilter2Lock(TSpinLock::EOrderBTrace);
#endif

DBTraceFilter2* DBTraceFilter2::iCleanupHead = 0;


DBTraceFilter2* DBTraceFilter2::New(TInt aNumUids)
	{
	DBTraceFilter2* self = (DBTraceFilter2*)Kern::AllocZ(sizeof(DBTraceFilter2)+aNumUids*sizeof(TUint32));
	if (self!=NULL)
		self->iAccessCount = 1;
	return self;
	}


void DBTraceFilter2::Cleanup()
	{
	FOREVER
		{
		TInt irq = __SPIN_LOCK_IRQSAVE(BTraceFilter2Lock);
		DBTraceFilter2* p = iCleanupHead;
		if (p)
			iCleanupHead = p->iCleanupLink;
		__SPIN_UNLOCK_IRQRESTORE(BTraceFilter2Lock, irq);
		if (!p)
			break;
		delete p;
		}
	}


DBTraceFilter2* DBTraceFilter2::Open(DBTraceFilter2*volatile& aFilter2)
	{
	TInt irq = __SPIN_LOCK_IRQSAVE(BTraceFilter2Lock);
	DBTraceFilter2* filter2 = aFilter2;
	if ((TLinAddr)filter2>1u)
		++filter2->iAccessCount;
	__SPIN_UNLOCK_IRQRESTORE(BTraceFilter2Lock, irq);
	return filter2;
	}


void DBTraceFilter2::Close()
	{
	if ((TLinAddr)this<=1u)
		return;
	TInt irq = __SPIN_LOCK_IRQSAVE(BTraceFilter2Lock);
	TInt access = iAccessCount;
	__NK_ASSERT_DEBUG(access>0);
	iAccessCount = access-1;
	if (access==1)
		{
		iCleanupLink = iCleanupHead;
		iCleanupHead = this;
		}
	__SPIN_UNLOCK_IRQRESTORE(BTraceFilter2Lock, irq);
	}


#ifndef __MARM__
TBool DBTraceFilter2::Check(TUint32 aUid)
	{
	TInt l = 0;
	TInt r = iNumUids;
	while(r>l)
		{
		TUint m = (l+r)>>1;
		TUint32 x = iUids[m];
		if(aUid>x)
			l = m+1;
		else if(aUid<x)
			r = m;
		else
			return 1;
		}
	return 0;
	}
#endif

extern void HeapSortUnsigned(TUint* aEntries,TInt aCount);
/**
Sort UIDs and remove duplicates.
Return number of unique uids.
*/
static TInt Sort(TUint32* aUids, TInt aNumUids)
	{
	HeapSortUnsigned((TUint*)aUids,aNumUids);
	TUint32* end = aUids+aNumUids-1;
	// remove duplicates...
	TUint32* src = aUids;
	TUint32* dst = aUids;
	if(src<=end)
		{
		TUint32 a = *src++;
		TUint32 b = a;
		*dst++ = b;
		while(src<=end)
			{
			a = *src++;
			if(a!=b)
				{
				b = a;
				*dst++ = b;
				}
			}
		}
	return dst-aUids;
	}


/**
Remove aUid from list aSrc and store result at aDst.
*/
static TUint Remove(TUint32* aDst, TUint32* aSrc, TInt aSrcCount, TUint32 aUid)
	{
	TUint32* dst = aDst;
	TUint32* end = aSrc+aSrcCount;
	while(aSrc<end)
		{
		TUint32 a = *aSrc++;
		if(a!=aUid)
			*dst++ = a;
		}
	return dst-aDst;
	}


/**
Insert aUid into list aSrc and store result at aDst.
*/
static TUint Insert(TUint32* aDst, TUint32* aSrc, TInt aSrcCount, TUint32 aUid)
	{
	TUint32* dst = aDst;
	TUint32* end = aSrc+aSrcCount;
	TUint32 a;
	while(aSrc<end)
		{
		a = *aSrc++;
		if(a<aUid)
			*dst++ = a;
		else
			goto done;
		}
	*dst++ = aUid;
	return dst-aDst;
done:
	if(a!=aUid)
		*dst++ = aUid;
	*dst++ = a;
	while(aSrc<end)
		*dst++ = *aSrc++;
	return dst-aDst;
	}


EXPORT_C TInt BTrace::SetFilter2(TUint32 aUid, TBool aValue)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"BTrace::Filter2");
	NKern::ThreadEnterCS();
	SBTraceData& traceData = BTraceData;
	DBTraceFilter2* oldFilter = DBTraceFilter2::Open(traceData.iFilter2);
	if((TUint)oldFilter==1u && !aValue)
		{
		NKern::ThreadLeaveCS();
		return KErrNotSupported; // can't clear a single uid when global filter is in 'pass all' mode
		}
	TBool oldValue = (TLinAddr)oldFilter<2u ? (TBool)oldFilter : oldFilter->Check(aUid);
	if(aValue!=oldValue && (TUint)aValue<=1u)
		{
		TUint count = (TLinAddr)oldFilter<2u ? 0 : oldFilter->iNumUids;
		TUint newCount = count+(aValue?1:-1);
		DBTraceFilter2* newFilter = DBTraceFilter2::New(newCount);

		if(!newFilter)
			oldValue = KErrNoMemory;
		else
			{
			if(aValue)
				{
				// add aUid...
				newFilter->iNumUids = ::Insert(newFilter->iUids,oldFilter->iUids,count,aUid);
				__NK_ASSERT_DEBUG(newFilter->iNumUids==newCount);
				}
			else
				{
				// remove aUid...
				newFilter->iNumUids = ::Remove(newFilter->iUids,oldFilter->iUids,count,aUid);
				__NK_ASSERT_DEBUG(newFilter->iNumUids==newCount);
				if(!newCount)
					{
					newFilter->Close();
					newFilter = 0;
					}
				}
			// finished with old filter...
			oldFilter->Close();

			// use newFilter...
			TInt irq = __SPIN_LOCK_IRQSAVE(BTraceFilter2Lock);
			oldFilter = traceData.iFilter2;
			traceData.iFilter2 = newFilter;
			__SPIN_UNLOCK_IRQRESTORE(BTraceFilter2Lock, irq);
			// oldFilter is now the one we replaced, which is not necessarily the same
			// as the previous oldFilter...
			}
		}
	oldFilter->Close();
	DBTraceFilter2::Cleanup();
	NKern::ThreadLeaveCS();
	return oldValue;
	}


EXPORT_C TInt BTrace::SetFilter2(const TUint32* aUids, TInt aNumUids)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"BTrace::Filter2");
	NKern::ThreadEnterCS();
	DBTraceFilter2* newFilter = DBTraceFilter2::New(aNumUids);
	if(!newFilter)
		{
		NKern::ThreadLeaveCS();
		return KErrNoMemory;
		}

	memcpy(&newFilter->iUids,aUids,aNumUids*sizeof(TUint32));
	aNumUids = Sort(newFilter->iUids, aNumUids);
	newFilter->iNumUids = aNumUids;

	TInt irq = __SPIN_LOCK_IRQSAVE(BTraceFilter2Lock);
	DBTraceFilter2* oldFilter = BTraceData.iFilter2;
	BTraceData.iFilter2 = newFilter;
	__SPIN_UNLOCK_IRQRESTORE(BTraceFilter2Lock, irq);
	oldFilter->Close();
	DBTraceFilter2::Cleanup();
	NKern::ThreadLeaveCS();
	return KErrNone;
	}


EXPORT_C TInt BTrace::SetFilter2(TInt aGlobalFilter)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"BTrace::Filter2");
	NKern::ThreadEnterCS();
	DBTraceFilter2* oldFilter;
	if((TUint)aGlobalFilter>1u)
		oldFilter = BTraceData.iFilter2; // just query existing value
	else
		{
		// replace filter with 0 or 1...
		TInt irq = __SPIN_LOCK_IRQSAVE(BTraceFilter2Lock);
		oldFilter = BTraceData.iFilter2;
		BTraceData.iFilter2 = (DBTraceFilter2*)aGlobalFilter;
		__SPIN_UNLOCK_IRQRESTORE(BTraceFilter2Lock, irq);
		oldFilter->Close();
		}
	DBTraceFilter2::Cleanup();
	NKern::ThreadLeaveCS();
	return (TUint)oldFilter>1u ? -1 : (TInt)oldFilter;
	}


EXPORT_C TInt BTrace::Filter2(TUint32*& aUids, TInt& aGlobalFilter)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"BTrace::Filter2");
	DBTraceFilter2* filter = DBTraceFilter2::Open(BTraceData.iFilter2);
	TInt r = 0;
	aUids = 0;
	aGlobalFilter = (TBool)filter;
	if((TUint)filter>1u)
		{
		aGlobalFilter = -1;
		r = filter->iNumUids;
		TUint size = r*sizeof(TUint32);
		aUids = (TUint32*)Kern::Alloc(size);
		if(aUids)
			memcpy(aUids,filter->iUids,size);
		else
			r = KErrNoMemory;
		}
	filter->Close();
	return r;
	}

#ifndef __MARM__
TBool SBTraceData::CheckFilter2(TUint32 aUid)
	{
	// quick check for global filter setting...
	TUint global = (TUint)iFilter2;
	if(global<2)
		return global;

	TBool enterCs = (NKern::CurrentContext()==NKern::EThread) && !NKern::KernelLocked();
	if (enterCs)
		NKern::_ThreadEnterCS();
	DBTraceFilter2* filter = DBTraceFilter2::Open(iFilter2);
	TBool value = (TLinAddr)filter<2u ? (TBool)filter : filter->Check(aUid);
	filter->Close();
	if (enterCs)
		NKern::_ThreadLeaveCS();
	return value;
	}
#endif

EXPORT_C TBool BTrace::CheckFilter2(TUint32 aCategory,TUint32 aUid)
	{
	SBTraceData& traceData = BTraceData;
	if(!traceData.iFilter[aCategory&0xff])
		return EFalse;
	return traceData.CheckFilter2(aUid);
	}


EXPORT_C TBool BTrace::CheckFilter(TUint32 aCategory)
	{
	return BTraceData.iFilter[aCategory&0xff];
	}


//
//
//

TBool ExecHandler::BTraceOut(TUint32 aHeader, TUint32 a1, const BTrace::SExecExtension& aExt, TInt aDataSize)
	{
	SBTraceData& traceData = BTraceData;
	if(!traceData.iFilter[(aHeader>>BTrace::ECategoryIndex*8)&0xff])
		return EFalse;

	if(aHeader&(BTrace::EMissingRecord<<BTrace::EFlagsIndex*8))
		{
		// EMissingRecord flag is overloaded to mean that secondary filter should be checked
		aHeader &= ~(BTrace::EMissingRecord<<BTrace::EFlagsIndex*8);
		if(!traceData.CheckFilter2(a1))
			return EFalse;
		}

	// only PC and Context flags allowed...
	if(aHeader&((0xff^BTrace::EContextIdPresent^BTrace::EPcPresent)<<BTrace::EFlagsIndex*8))
		goto error;

	{
	// get size of trace data excluding aDataSize
	TUint size = (aHeader>>BTrace::ESizeIndex*8)&0xff;
	if(aHeader&(BTrace::EPcPresent<<BTrace::EFlagsIndex*8))
		size -= 4;
	TUint32 context = 0;
	if(aHeader&(BTrace::EContextIdPresent<<BTrace::EFlagsIndex*8))
		{
		size -= 4;
		context = (TUint32)NKern::CurrentThread();
		}

	if(!aDataSize)
		{
		if((size-4)>(16-4)) // size must be 4...16
			goto error;
		__ACQUIRE_BTRACE_LOCK();
		TBool r = traceData.iHandler(aHeader,0,context,a1,aExt.iA2,aExt.iA3,0,aExt.iPc);
		__RELEASE_BTRACE_LOCK();
		return r;
		}

	if(size!=12)
		goto error;
	if(TUint(aDataSize)>KMaxBTraceDataArray)
		{
		aDataSize = KMaxBTraceDataArray;
		aHeader |= BTrace::ERecordTruncated<<(BTrace::EFlagsIndex*8);
		}
	{
	aHeader += aDataSize<<(BTrace::ESizeIndex*8);
	TUint32 data[KMaxBTraceDataArray/4];
	kumemget32(data,(const TAny*)aExt.iA3,(aDataSize+3)&~3);
	TUint32 a3 = aDataSize<=4 ? data[0] : (TUint32)&data;
	__ACQUIRE_BTRACE_LOCK();
	TBool r = traceData.iHandler(aHeader,0,context,a1,aExt.iA2,a3,0,aExt.iPc);
	__RELEASE_BTRACE_LOCK();
	return r;
	}
	}
error:
	return KErrArgument;
	}


TBool ExecHandler::BTraceOutBig(TUint32 aHeader, TUint32 a1, const BTrace::SExecExtension& aExt, TInt aDataSize)
	{
	SBTraceData& traceData = BTraceData;
	if(!traceData.iFilter[(aHeader>>BTrace::ECategoryIndex*8)&0xff])
		return EFalse;

	if(aHeader&(BTrace::EMissingRecord<<BTrace::EFlagsIndex*8))
		{
		// EMissingRecord flag is overloaded to mean that secondary filter should be checked
		aHeader &= ~(BTrace::EMissingRecord<<BTrace::EFlagsIndex*8);
		if(!traceData.CheckFilter2(a1))
			return EFalse;
		}

	// only PC and Context flags allowed...
	if(aHeader&((0xff^BTrace::EContextIdPresent^BTrace::EPcPresent)<<BTrace::EFlagsIndex*8))
		goto error;

	{
	// get size of trace data excluding aDataSize
	TUint size = (aHeader>>BTrace::ESizeIndex*8)&0xff;
	if(aHeader&(BTrace::EPcPresent<<BTrace::EFlagsIndex*8))
		size -= 4;
	TUint32 context = 0;
	if(aHeader&(BTrace::EContextIdPresent<<BTrace::EFlagsIndex*8))
		{
		size -= 4;
		context = (TUint32)NKern::CurrentThread();
		}
	TUint32 pc = aExt.iPc;

	if(size!=8)
		goto error; // size whould be 8 (for data in aHeader and a1)
	if(TUint(aDataSize)<KMaxBTraceDataArray+4)
		goto error; // trace too small for a big trace

	// adjust for header2, extra, and size word...
	aHeader |= BTrace::EHeader2Present<<(BTrace::EFlagsIndex*8)|BTrace::EExtraPresent<<(BTrace::EFlagsIndex*8);
	aHeader += 12;

	TUint8* userData = (TUint8*)aExt.iA3;
	TUint32 data[KMaxBTraceDataArray/4];

	TUint32 traceId = __e32_atomic_add_ord32(&BTrace::BigTraceId, 1);
	TUint32 header2 = BTrace::EMultipartFirst;
	TInt offset = 0;
	do
		{
		TUint32 size = aDataSize-offset;
		if(size>KMaxBTraceDataArray)
			size = KMaxBTraceDataArray;
		else
			header2 = BTrace::EMultipartLast;

		kumemget32(data,userData,(size+3)&~3);
		TUint32 dataPtr = (TUint32)&data;
		if(size<=4)
			dataPtr = data[0]; // 4 bytes or less are passed by value, not pointer

		__ACQUIRE_BTRACE_LOCK();
		TBool result = traceData.iHandler(aHeader+size,header2,context,aDataSize,a1,dataPtr,traceId,pc);
		__RELEASE_BTRACE_LOCK();
		if(!result)
			return result;

		offset += size;
		userData += size;

		header2 = BTrace::EMultipartMiddle;
		a1 = offset;
		}
	while(offset<aDataSize);

	return ETrue;
	}
error:
	return KErrArgument;
	}


TBool ExecHandler::UTraceOut(TUint32 aHeader, TUint32 a1, const BTrace::SExecExtension& aExt, TInt aDataSize)
	{
	SBTraceData& traceData = BTraceData;
	if(!traceData.iFilter[(aHeader>>BTrace::ECategoryIndex*8)&0xff])
		return EFalse;

	if(aHeader&(BTrace::EMissingRecord<<BTrace::EFlagsIndex*8))
		{
		// EMissingRecord flag is overloaded to mean that secondary filter should be checked
		aHeader &= ~(BTrace::EMissingRecord<<BTrace::EFlagsIndex*8);
		if(!traceData.CheckFilter2(a1))
			return EFalse;
		}

	// only PC and Context flags allowed...
	if(aHeader&((0xff^BTrace::EContextIdPresent^BTrace::EPcPresent)<<BTrace::EFlagsIndex*8))
		return KErrArgument;

	// get size of trace data excluding aDataSize
	TUint size = (aHeader>>BTrace::ESizeIndex*8)&0xff;
	if(aHeader&(BTrace::EPcPresent<<BTrace::EFlagsIndex*8))
		size -= 4;
	TUint32 context = 0;
	if(aHeader&(BTrace::EContextIdPresent<<BTrace::EFlagsIndex*8))
		{
		size -= 4;
		context = (TUint32)NKern::CurrentThread();
		}

	if(size!=8)
		return KErrArgument; // size whould be 8 (for data in aHeader and a1)
	if(TUint(aDataSize)<KMaxBTraceDataArray)
		return KErrArgument; // trace too small for a big trace

	// adjust for header2, extra, and size word...
	aHeader |= BTrace::EHeader2Present<<(BTrace::EFlagsIndex*8)|BTrace::EExtraPresent<<(BTrace::EFlagsIndex*8);
	aHeader += 12;

	// send the first trace including the formatId
	TUint8* userData = (TUint8*)aExt.iA3;
	TUint32 data[KMaxBTraceDataArray/4];
	data[0] = aExt.iA2; // add the formatId for the first trace
	TUint32 traceId = NKern::LockedInc((TInt&)BTrace::BigTraceId);
	TUint32 header2 = BTrace::EMultipartFirst;
	TInt additionalIdentifiers = 4;
	TInt identifierOffset = additionalIdentifiers; // bytes
	TBool result = ETrue;
	TInt offset = 0; // offset into the payload

	do
		{
		TUint32 dataSize = aDataSize - offset;
		if(dataSize > (KMaxBTraceDataArray - identifierOffset))
			dataSize = KMaxBTraceDataArray - identifierOffset;
		else
			header2 = BTrace::EMultipartLast;

		kumemget32(data+identifierOffset/4,userData,(dataSize+3)&~3); //add the rest of the payload, 4 byte aligned

		TUint32 dataPtr = (TUint32)&data;
		if(dataSize<=4)
			dataPtr = data[0]; // 4 bytes or less are passed by value, not pointer

		__ACQUIRE_BTRACE_LOCK();
		result = traceData.iHandler(aHeader+dataSize,header2,context,aDataSize,a1,dataPtr,traceId,aExt.iPc);
		__RELEASE_BTRACE_LOCK();
		if(!result)
			return result;

		offset += dataSize - identifierOffset;
		userData += dataSize - identifierOffset;
		a1 = offset;
		header2 = BTrace::EMultipartMiddle;
		identifierOffset = 0; // we are only adding identifiers into the first trace
		}
	while(offset<aDataSize);

	return result;//ETrue
	}

