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
// e32/kernel/sshbuf.cpp
// Shareable Data Buffers

#include <kernel/sshbuf.h>
#include <kernel/cache.h>
#include "execs.h"
#include <kernel/smap.h>

/********************************************
 * Kernel-side executive calls
 ********************************************/

TInt ExecHandler::ShPoolCreate(const TShPoolInfo& aInfo, TUint aFlags)
	{
	__KTRACE_OPT(KEXEC, Kern::Printf(">Exec::ShPoolCreate"));

	const TUint acceptableFlags = EShPoolWriteable | EShPoolAllocate;
	TUint validatedFlags = aFlags & acceptableFlags;
	aFlags &= ~acceptableFlags;

	if (aFlags != 0)
		{
		Kern::Printf("Exec::ShPoolCreate: flags 0x%x after 0x%x", aFlags, validatedFlags);
		// SBZ bit set in flags passed to the exec call: panic the caller.
		K::PanicKernExec(EShBufExecBadParameter);
		}

	TShPoolCreateInfo uinfo;

	kumemget32(&uinfo.iInfo, &aInfo, sizeof(uinfo.iInfo));

	NKern::ThreadEnterCS();
	DShPool* pC = NULL;
	TInt r = K::ShPoolCreate(pC, uinfo); // should call the ShPoolCreate method on process

	if (r == KErrNone)
		{
		// The flags are passed down as attributes to RequestUserHandle, Add, and AddToProcess
		r = K::MakeHandle(EOwnerProcess, pC, validatedFlags);	// this will add the pool to the process

		if (r < KErrNone && pC)
			pC->Close(NULL);	// can't have been added so NULL
		}

	NKern::ThreadLeaveCS();

	__KTRACE_OPT(KEXEC, Kern::Printf("<Exec::ShPoolCreate returns %d", r));
	return r;
	}

TInt K::ShPoolCreate(DShPool*& aPool, TShPoolCreateInfo& aInfo)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf(">K::ShPoolCreate"));

	DThread* pT=TheCurrentThread;
	DProcess* pP=pT->iOwningProcess;

	TInt r = pP->NewShPool(aPool, aInfo);

	if (r != KErrNone)
		aPool = NULL;

	__KTRACE_OPT(KEXEC, Kern::Printf("<K::ShPoolCreate returns %d %08x", r, aPool));
	return r;
	}

TInt DShPool::ModifyClientFlags(DProcess* aProcess, TUint aSetMask, TUint aClearMask)
	{
	LockPool();
	DShPoolClient* client = reinterpret_cast<DShPoolClient*>(iClientMap->Find(reinterpret_cast<TUint>(aProcess)));

	if ((client != NULL) && client->iAccessCount)
		{
		// access count must be non-zero otherwise the pool is in the process of being closed
		client->iFlags &= ~aClearMask;
		client->iFlags |= aSetMask;
		}
	UnlockPool();

	return KErrNone;
	}

TInt ExecHandler::ShPoolAlloc(TInt aHandle, TUint aFlags, SShBufBaseAndSize& aBaseAndSize)
	{
	__KTRACE_OPT(KEXEC, Kern::Printf(">Exec::ShPoolAlloc (0x%08x, 0x%x)", aHandle, aFlags));
	TUint attr = 0;
	DShBuf* pC = NULL;

	NKern::LockSystem();
	DShPool* pool = reinterpret_cast<DShPool*>(K::ObjectFromHandle(aHandle, EShPool, attr));
	/* K::ObjectFromHandle will panic on NULL */

	pool->CheckedOpen();

	NKern::ThreadEnterCS();
	NKern::UnlockSystem();

	TInt r = KErrAccessDenied;		// for the case that (attr & EShPoolAllocate) == 0
	SShBufBaseAndSize bs;

	if ((attr & EShPoolAllocate))
		{
		r = pool->Alloc(pC);

		if (r == KErrNone)
			{
			attr |= RObjectIx::EReserved;

			if (aFlags & EShPoolAllocNoMap)
				{
				attr |= EShPoolNoMapBuf;
				}

			r=K::MakeHandle(EOwnerProcess, pC, attr);	// this will add the buffer to the process

			if (r < KErrNone && pC != NULL)
				pC->Close(NULL);				// can't have been added so NULL
			else
				{
				bs.iBase = reinterpret_cast<TUint>(pC->Base(TheCurrentThread->iOwningProcess));
				bs.iSize = pool->BufSize();
				}
			}
		}

	pool->Close(NULL);
	NKern::ThreadLeaveCS();

	if (r > KErrNone)
		{
		kumemput32(&aBaseAndSize, &bs, sizeof(bs));
		}

	__KTRACE_OPT(KEXEC, Kern::Printf("<Exec::ShPoolAlloc returns %d 0x%08x", r, pC));

	return r;
	}

void ExecHandler::ShPoolGetInfo(DShPool* aShPool, TShPoolInfo& aInfo)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf(">Exec::ShPoolGetInfo"));
	TShPoolInfo info;
	aShPool->GetInfo(info);
	NKern::UnlockSystem();
	kumemput32(&aInfo,&info,sizeof(info));
	}

TUint ExecHandler::ShPoolFreeCount(DShPool* aShPool)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf(">Exec::ShPoolFreeCount (0x%08x)", aShPool));
	return aShPool->FreeCount();
	}

TInt ExecHandler::ShPoolNotification(DShPool* aShPool, TShPoolNotifyType aType, TUint aThreshold, TRequestStatus& aStatus)
	{
	__KTRACE_OPT(KEXEC, Kern::Printf(">Exec::ShPoolNotification (0x%08x, %d, 0x%x)", aShPool, aType, &aStatus));
	aShPool->CheckedOpen();
	NKern::ThreadEnterCS();
	NKern::UnlockSystem();
	TInt r = aShPool->AddNotification(aType, aThreshold, aStatus);
	aShPool->Close(NULL);
	NKern::ThreadLeaveCS();

	if (r == KErrArgument)
		K::PanicKernExec(EShBufExecBadNotification);

	return r;
	}

TInt ExecHandler::ShPoolNotificationCancel(DShPool* aShPool, TShPoolNotifyType aType, TRequestStatus& aStatus)
	{
	__KTRACE_OPT(KEXEC, Kern::Printf(">Exec::ShPoolNotificationCancel (0x%08x)", aShPool));

	aShPool->CheckedOpen();
	NKern::ThreadEnterCS();
	NKern::UnlockSystem();
	TInt r = aShPool->RemoveNotification(aType, aStatus);
	aShPool->Close(NULL);
	NKern::ThreadLeaveCS();

	if (r == KErrArgument)
		K::PanicKernExec(EShBufExecBadNotification);

	return r;
	}

TInt ExecHandler::ShPoolBufferWindow(DShPool* aShPool, TInt aWindowSize, TBool aAutoMap)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf(">Exec::ShPoolBufferWindow (0x%08x)", aShPool));

	DProcess* pP = TheCurrentThread->iOwningProcess;
	aShPool->CheckedOpen();
	NKern::ThreadEnterCS();
	NKern::UnlockSystem();
	TInt r = aShPool->SetBufferWindow(pP, aWindowSize);

	if (r == KErrNone)
		{
		r = aShPool->ModifyClientFlags(pP, aAutoMap ? EShPoolAutoMapBuf : 0, aAutoMap ? 0 : EShPoolAutoMapBuf);
		}
	aShPool->Close(NULL);
	NKern::ThreadLeaveCS();
	return r;
	}

TInt ExecHandler::ShBufMap(DShBuf* aBuf, TBool aReadOnly, SShBufBaseAndSize& aBaseAndSize)
	{
	__KTRACE_OPT(KEXEC, Kern::Printf(">Exec::ShBufMap (0x%08x)", aBuf));

	DProcess* pP = TheCurrentThread->iOwningProcess;
	DShPool* pool = aBuf->iPool;
	SShBufBaseAndSize bs;

	TInt r = KErrAccessDenied;

	aBuf->CheckedOpen();
	NKern::ThreadEnterCS();
	NKern::UnlockSystem();


	pool->LockPool();
	DShPoolClient* client = reinterpret_cast<DShPoolClient*>(pool->iClientMap->Find(reinterpret_cast<TUint>(pP)));
	TUint attr = client->iFlags;
	pool->UnlockPool();

	// if the buffer is being mapped as writeable check that the pool has the corresponding
	// capabilities

	if (aReadOnly)
		attr &= ~EShPoolWriteable;

	r = aBuf->Map(attr, pP, bs.iBase);
	bs.iSize = aBuf->Size();

	aBuf->Close(NULL);
	NKern::ThreadLeaveCS();

	if (r == KErrNone)
		{
		kumemput32(&aBaseAndSize, &bs, sizeof(bs));
		}

	__KTRACE_OPT(KEXEC, Kern::Printf("<Exec::ShBufMap returns %d 0x%08x",r, bs.iBase));
	return r;
	}

TInt ExecHandler::ShBufUnMap(DShBuf* aShBuf)
	{
	__KTRACE_OPT(KEXEC, Kern::Printf(">Exec::ShBufUnMap (0x%08x)", aShBuf));

	aShBuf->CheckedOpen();
	NKern::ThreadEnterCS();
	DProcess* pP = TheCurrentThread->iOwningProcess;
	NKern::UnlockSystem();
	TInt r = aShBuf->UnMap(pP);
	aShBuf->Close(NULL);
	NKern::ThreadLeaveCS();
	return r;
	}

TInt ExecHandler::ShBufBaseAndSize(DShBuf* aShBuf, SShBufBaseAndSize& aBaseAndSize)
	{
	__KTRACE_OPT(KEXEC, Kern::Printf(">Exec::ShBufBaseAndSize (0x%08x)", aShBuf));

	aShBuf->CheckedOpen();
	NKern::ThreadEnterCS();
	NKern::UnlockSystem();
	SShBufBaseAndSize bs;

	bs.iBase = reinterpret_cast<TUint>(aShBuf->Base(TheCurrentThread->iOwningProcess));
	bs.iSize = aShBuf->Size();
	aShBuf->Close(NULL);
	NKern::ThreadLeaveCS();

	kumemput32(&aBaseAndSize, &bs, sizeof(bs));

	__KTRACE_OPT(KEXEC, Kern::Printf("<Exec::ShBufBaseAndSize (0x%08x, 0x%08x, 0x%08x)", aShBuf, bs.iBase, bs.iSize));

	return KErrNone;
	}

/********************************************
 * DShPool and DShBuf
 ********************************************/
EXPORT_C TShPoolInfo::TShPoolInfo()
	{
	memclr(this, sizeof(TShPoolInfo));
	}

EXPORT_C TShPoolCreateInfo::TShPoolCreateInfo(TShPoolPageAlignedBuffers aFlag, TUint aBufSize, TUint aInitialBufs)
	{
	iInfo.iBufSize = aBufSize;
	iInfo.iInitialBufs = aInitialBufs;
	iInfo.iFlags = aFlag;
	SetSizingAttributes(aInitialBufs,0,0,0);
	iPhysAddr.iPhysAddrList = 0;
	iPages = 0;
	}

EXPORT_C TShPoolCreateInfo::TShPoolCreateInfo(TShPoolNonPageAlignedBuffers aFlag, TUint aBufSize, TUint aInitialBufs, TUint aAlignment)
	{
	iInfo.iBufSize = aBufSize;
	iInfo.iInitialBufs = aInitialBufs;
	iInfo.iAlignment = aAlignment;
	iInfo.iFlags = aFlag;
	SetSizingAttributes(aInitialBufs,0,0,0);
	iPhysAddr.iPhysAddrList = 0;
	iPages = 0;
	}

/**
Sets the pool to be created in a specific physical address range

@param aFlag		Indicates pool to be created within a specific physical address range
@param aBufSize		Size of a single buffer within the pool
@param aInitialBufs Initial number of buffers allocated to the pool
@param aAlignment	Alignment of the start of each buffer in the pool
@param aPages		Number of pages to commit. Must be a multiple of the MMU
@param aPhysicalAddressList A pointer to a list of physical addresses, one address for
						each page of memory committed. Each physical address must be
						a multiple of the MMU page size.
@see DShPool::Create()
*/
EXPORT_C TShPoolCreateInfo::TShPoolCreateInfo(TShPoolMemoryDevice aFlag, TUint aBufSize, TUint aInitialBufs, TUint aAlignment, TUint aPages, TPhysAddr* aPhysicalAddressList)
	{
	iInfo.iBufSize = aBufSize;
	iInfo.iInitialBufs = aInitialBufs;
	iInfo.iAlignment = aAlignment;
	iPhysAddr.iPhysAddrList = aPhysicalAddressList;
	iPages = aPages;
	iInfo.iFlags = aFlag;
	SetSizingAttributes(aInitialBufs,0,0,0);
	}

/**
Sets the pool to be created in a specific physical address range

@param aFlag		Indicates pool to be created within a specific physical address range
@param aBufSize		Size of a single buffer within the pool
@param aInitialBufs Initial number of buffers allocated to the pool
@param aAlignment	Alignment of the start of each buffer in the pool
@param aPages		Number of pages to commit. Must be a multiple of the MMU
@param aPhysicalAddress A physical address, Each physical address must be
						a multiple of the MMU page size.
@see DShPool::Create()
*/
EXPORT_C TShPoolCreateInfo::TShPoolCreateInfo(TShPoolMemoryDevice aFlag, TUint aBufSize, TUint aInitialBufs, TUint aAlignment, TUint aPages, TPhysAddr aPhysicalAddress)
	{
	iInfo.iBufSize = aBufSize;
	iInfo.iInitialBufs = aInitialBufs;
	iInfo.iAlignment = aAlignment;
	iPhysAddr.iPhysAddr = aPhysicalAddress;
	iPages = aPages;
	iInfo.iFlags = aFlag;
	SetContiguous();
	SetSizingAttributes(aInitialBufs,0,0,0);
	}

EXPORT_C TInt TShPoolCreateInfo::SetSizingAttributes(TUint aMaxBufs, TUint aGrowTriggerRatio,
					TUint aGrowByRatio, TUint aShrinkHysteresisRatio)
	{
	if (aGrowTriggerRatio == 0 || aGrowByRatio == 0)	// No automatic growing/shrinking
		{
		aGrowTriggerRatio = aGrowByRatio = 0;
		if (aMaxBufs != iInfo.iInitialBufs)
			return KErrArgument;
		}
	else
		{
		// aGrowTriggerRatio must be < 1.0 (i.e. 256)
		// aShrinkHysteresisRatio must be > 1.0
		if (aGrowTriggerRatio >= 256 || aShrinkHysteresisRatio <= 256)
			return KErrArgument;

		if ((iInfo.iFlags & EShPoolContiguous) && (iInfo.iFlags & EShPoolNonPageAlignedBuffer))
			return KErrNotSupported;
		}

	iInfo.iMaxBufs			= aMaxBufs;
	iInfo.iGrowTriggerRatio	= aGrowTriggerRatio;
	iInfo.iGrowByRatio		= aGrowByRatio;
	iInfo.iShrinkHysteresisRatio = aShrinkHysteresisRatio;

	return KErrNone;
	}

EXPORT_C TInt TShPoolCreateInfo::SetExclusive()
	{
	iInfo.iFlags |= EShPoolExclusiveAccess;

	return KErrNone;
	}

EXPORT_C TInt TShPoolCreateInfo::SetGuardPages()
	{
	iInfo.iFlags |= EShPoolGuardPages;

	return KErrNone;
	}

EXPORT_C TInt TShPoolCreateInfo::SetContiguous()
	{
	iInfo.iFlags |= EShPoolContiguous;

	return KErrNone;
	}

/**
   Creates a pool of buffers with the specified attributes

   @param aPool[out]      Returns a pointer to the pool.
   @param aInfo           Reference to pool create information.
   @param aMap   		  ETrue specifies that an allocated or received buffer
	                      will be automatically mapped into the process's address space.
   @param aFlags		  Flags to modify the behaviour of the handle.  This should be a bit-wise
						  OR of values from TShPoolHandleFlags.
   @pre Calling thread must be in a critical section.

   @return the KErrNone if successful, otherwise one of the system-wide error codes.
*/
EXPORT_C TInt Kern::ShPoolCreate(TShPool*& aPool, TShPoolCreateInfo& aInfo, TBool aMap, TUint aFlags)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">Kern::ShPoolCreate, (aInfo 0x%08x)", &aInfo));
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "Kern::ShPoolCreate");

	aPool = NULL;

	const TUint acceptableFlags = EShPoolWriteable | EShPoolAllocate;

	TUint validatedFlags = aFlags & acceptableFlags;

	aFlags &= ~acceptableFlags;

	if (aFlags != 0)
		{
		Kern::Printf("Kern::ShPoolCreate: flags 0x%x after 0x%x", aFlags, validatedFlags);
		return KErrArgument;
		}

	DShPool* pool;
	TInt r = K::TheKernelProcess->NewShPool(pool, aInfo);
	if (r == KErrNone)
		{
		if (aMap) validatedFlags |= EShPoolAutoMapBuf;

		r = pool->AddToProcess(K::TheKernelProcess, validatedFlags);
		if (r == KErrNone)
			{
			aPool = reinterpret_cast<TShPool*>(pool);
			}
		else
			{
			pool->Close(NULL);	// can't have been added so NULL
			}
		}

	__KTRACE_OPT(KMMU, Kern::Printf("<Kern::ShPoolCreate, pool(%x)", aPool));
	return r;
	}

/**
   Opens a pool of buffers in the kernel address space using a user process
   handle

   @param aPool[out]      Returns pointer to the pool.
   @param aThread         Pointer to user process thread, if null
                          current thread is used.
   @param aHandle         User process handle to open pool from
   @param aMap   		  ETrue specifies that an allocated or received buffer
	                      will be automatically mapped into the process's address space.
   @param aFlags		  Flags to modify the behaviour of the handle.  This should be a bit-wise
						  OR of values from TShPoolHandleFlags.
   @pre Calling thread must be in a critical section.

   @return the KErrNone if successful, otherwise one of the system-wide error codes.
*/
EXPORT_C TInt Kern::ShPoolOpen(TShPool*& aPool, DThread* aThread, TInt aHandle, TBool aMap, TUint aFlags)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">Kern::ShPoolOpen, 0x%08x %d", aThread, aHandle));
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "Kern::ShPoolOpen");

	const TUint acceptableFlags = EShPoolWriteable | EShPoolAllocate;

	TUint validatedFlags = aFlags & acceptableFlags;

	aFlags &= ~acceptableFlags;

	if (aFlags != 0)
		{
		Kern::Printf("Kern::ShPoolOpen: flags 0x%x after 0x%x", aFlags, validatedFlags);
		return KErrArgument;
		}

	TInt r = KErrNotFound;
	aPool = NULL;
	TUint attr;

	NKern::LockSystem();
	DShPool* pool = reinterpret_cast<DShPool*>(aThread->ObjectFromHandle(aHandle, EShPool, attr));
	if (pool != NULL)
		{
		r = pool->Open();
		}

	NKern::UnlockSystem();

	attr |= validatedFlags;

	if (r == KErrNone)
		{
		if (aMap) attr |= EShPoolAutoMapBuf;

		r = pool->AddToProcess(K::TheKernelProcess, attr);
		if (r == KErrNone)
			{
			aPool = reinterpret_cast<TShPool*>(pool);
			}
		else
			{
			pool->Close(NULL);	// can't have been added so NULL
			}
		}

	return r;
	}

/**
   Closes a pool of buffers in the kernel address

   @param aPool     Pointer to the pool.

   @return          ETrue if the reference count of the pool has gone to zero,
                    otherwise one of the system-wide error codes.
*/
EXPORT_C TInt Kern::ShPoolClose(TShPool* aPool)

	{
	__KTRACE_OPT(KMMU, Kern::Printf(">Kern::ShPoolClose(%x)", aPool));

	TInt r = reinterpret_cast<DShPool*>(aPool)->Close(K::TheKernelProcess);

	__KTRACE_OPT(KMMU, Kern::Printf("<Kern::ShPoolClose(%x)", aPool));
	return r;
	}


/**
   Creates a user process handle to a pool of buffers in the kernel address

   @param aPool           Pointer to the pool.
   @param aThread         Pointer to user process thread, if null
                          current thread is used.
   @param aFlags          Handle flags (attributes)

   @pre Calling thread must be in a critical section.

   @return the handle if successful, otherwise one of the system-wide error codes.
*/
EXPORT_C TInt Kern::ShPoolMakeHandleAndOpen(TShPool* aPool, DThread* aThread, TUint aFlags)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "Kern::ShPoolMakeHandleAndOpen");

	if (!aThread)
		aThread = TheCurrentThread;

	TInt h;
	TInt r = aThread->MakeHandleAndOpen(EOwnerProcess, reinterpret_cast<DShPool*>(aPool), h, aFlags);
	return (r == KErrNone) ? h : r;
	}
/**
   Allocates a shared data buffer.

   By default this method will return immediately with KErrNoMemory if no buffer is
   available on the pool's free list, even if the pool could grow automatically.

   By default it will also map the allocated buffer into the calling process's address space.

   Setting EShPoolAllocCanWait in the flags indicates that the caller is prepared to
   wait while the pool is grown if a buffer is not immediately available on the free list.

   Setting EShPoolAllocNoMap in the flags indicates that the caller does not want the
   buffer to be automatically mapped into its address space.  This can improve performance
   on buffers from page-aligned pools if the caller will not need to access the data in the
   buffer (e.g. if it will just be passing it on to another component).  This only prevents
   mapping if the pool is set to not automatically map buffers into processes' address space.

   @param aPool           Pointer to the pool
   @param aBuf	          Pointer to buffer
   @param aFlags	      Bitwise OR of values from TShPoolAllocFlags to specify non-default behaviour.

   @pre Calling thread must be in a critical section.

   @return KErrNone if successful, otherwise one of the system-wide error codes.

   @see TShPoolAllocFlags
*/
EXPORT_C TInt Kern::ShPoolAlloc(TShPool* aPool, TShBuf*& aBuf, TUint aFlags)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">Kern::ShPoolAlloc(%x)", aPool));
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "Kern::ShPoolAlloc");

	aBuf = NULL;

	const TUint acceptableFlags = EShPoolAllocNoMap;

	TUint validatedFlags = aFlags & acceptableFlags;

	aFlags &= ~acceptableFlags;

	if (aFlags != 0)
		{
		Kern::Printf("Kern::ShPoolAlloc: flags 0x%x after 0x%x", aFlags, validatedFlags);
		return KErrArgument;
		}

	DShBuf* buf;
	TInt r = reinterpret_cast<DShPool*>(aPool)->Alloc(buf);
	if (r == KErrNone)
		{
		if (buf->iPool->iPoolFlags & EShPoolPageAlignedBuffer)
			{
			TUint attr = 0;
			if (validatedFlags & EShPoolAllocNoMap)
				{
				attr = EShPoolNoMapBuf;
				}

			r = buf->AddToProcess(K::TheKernelProcess, attr);
			}
		if (r == KErrNone)
			{
			aBuf = reinterpret_cast<TShBuf*>(buf);
			}
		else
			{
			buf->Close(NULL);
			}
		}

	__KTRACE_OPT(KMMU, Kern::Printf("<Kern::ShPoolAlloc(%x)", aPool));
	return r;
	}

/**
   Retrieves information about the pool.

   @param aPool           Pointer to the pool
   @param aInfo[out]      Returns a reference to pool info.
*/
EXPORT_C void Kern::ShPoolGetInfo(TShPool* aPool, TShPoolInfo& aInfo)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">Kern::ShPoolGetInfo(%x)", aPool));

	reinterpret_cast<DShPool*>(aPool)->GetInfo(aInfo);

	__KTRACE_OPT(KMMU, Kern::Printf("<Kern::ShPoolGetInfo(%x)", aPool));
	}

/**
   @param aPool           Pointer to the pool
   @return                the size of each buffer in the pool.
*/
EXPORT_C TUint Kern::ShPoolBufSize(TShPool* aPool)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">Kern::ShPoolBufSize(%x)", aPool));

	return reinterpret_cast<DShPool*>(aPool)->BufSize();
	}

/**
   @param aPool           Pointer to the pool
   @return                the number of free buffers in the pool
*/
EXPORT_C TUint Kern::ShPoolFreeCount(TShPool* aPool)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">Kern::ShPoolFreeCount(%x)", aPool));

	return reinterpret_cast<DShPool*>(aPool)->FreeCount();
	}

/**
   Specifies how many buffers of a page-aligned pool this process will require
   concurrent access to.

   This determines how much of the kernel's address space will be allocated for
   buffers of this pool.

   If the pool and it's corresponding buffers are not going to be mapped in the
   kernel's address space then it is not necessary to call this API

   @param aPool           Pointer to the pool
   @param aCount		  Specifies the number of buffers to map into the process's
                          virtual address space
						  (-1 specifies that all buffers will be mapped).

   @pre Calling thread must be in a critical section.
   @pre The pool's buffers must be page-aligned.

   @return KErrNone if successful, otherwise one of the system-wide error codes.
*/
EXPORT_C TInt Kern::ShPoolSetBufferWindow(TShPool* aPool, TInt aWindowSize)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">Kern::ShPoolSetBufferWindow(%x %d)", aPool, aWindowSize));
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "Kern::ShPoolSetBufferWindow");
	TInt r = reinterpret_cast<DShPool*>(aPool)->SetBufferWindow(K::TheKernelProcess, aWindowSize);
	return r;
	}

/**
   Opens a buffer in the kernel address space using a user process handle.

   @param aBuf[out]       Returns pointer to the buffer.
   @param aThread         Pointer to user process thread, if null
                          current thread is used.
   @param aHandle         User process handle to open pool from

   @pre Calling thread must be in a critical section.

   @return the KErrNone if successful, otherwise one of the system-wide error codes.
*/
EXPORT_C TInt Kern::ShBufOpen(TShBuf*& aBuf, DThread* aThread, TInt aHandle)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">Kern::ShBufOpen(0x%08x %d)", aThread, aHandle));
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "Kern::ShBufOpen");

	TInt r = KErrNotFound;
	aBuf = NULL;

	NKern::LockSystem();
	DShBuf* buf = reinterpret_cast<DShBuf*>(aThread->ObjectFromHandle(aHandle,EShBuf));
	if (buf != NULL)
		{
		r = buf->Open();
		}

	NKern::UnlockSystem();

	if (r == KErrNone)
		{
		if (buf->iPool->iPoolFlags & EShPoolPageAlignedBuffer)
			{
		    r = buf->AddToProcess(K::TheKernelProcess, 0);
			}
		if (r == KErrNone)
			{
			aBuf = reinterpret_cast<TShBuf*>(buf);
			}
		else
			{
			buf->Close(NULL);
			}
		}

	return r;
	}

/**
   Opens a buffer in the kernel address space using a user process handle.

   @param aBuf              Pointer to the buffer.
   @param aPinObject	    The physical pin mapping.
   @param aReadOnly   	    Indicates whether memory should be pinned as read only.
   @param aAddress[out]	    The value is the physical address of the first page
						    in the region.
   @param aPages[out]       If not zero, this points to an array of TPhysAddr
	                        objects. On success, this array will be filled
						    with the addresses of the physical pages which
						    contain the specified region. If aPages is
						    zero, then the function will fail with
						    KErrNotFound if the specified region is not
						    physically contiguous.
   @param aMapAttr[out]    Memory attributes defined by TMappingAttributes2.
   @param aColour[out]     The mapping colour of the first physical page.

   @pre Calling thread must be in a critical section.

   @return the KErrNone if successful, otherwise one of the system-wide error codes.
*/
EXPORT_C TInt Kern::ShBufPin(TShBuf* aBuf, TPhysicalPinObject* aPinObject, TBool aReadOnly, TPhysAddr& aAddress, TPhysAddr* aPages, TUint32& aMapAttr, TUint& aColour)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">Kern::ShBufPin(%x)", aBuf));
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "Kern::ShBufPin");

	return reinterpret_cast<DShBuf*>(aBuf)->Pin(aPinObject, aReadOnly, aAddress, aPages, aMapAttr, aColour);
	}

/**
   @param aBuf              Pointer to the buffer.

   @return                  the size of the buffer
*/
EXPORT_C TUint Kern::ShBufSize(TShBuf* aBuf)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">Kern::ShBufSize(%x)", aBuf));

	return reinterpret_cast<DShBuf*>(aBuf)->Size();
	}

/**
   @param aBuf              Pointer to the buffer.

   @return                 A pointer to the start of the buffer.
*/
EXPORT_C TUint8* Kern::ShBufPtr(TShBuf* aBuf)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">Kern::ShBufPtr(%x)", aBuf));

	return reinterpret_cast<DShBuf*>(aBuf)->Base();
	}

/**
   Closes a buffer in the kernel address

   @param aPool     Pointer to the buffer.

   @return          ETrue if the reference count of the pool has gone to zero,
                    otherwise one of the system-wide error codes.
*/
EXPORT_C TInt Kern::ShBufClose(TShBuf* aBuf)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">Kern::ShBufClose(%x)", aBuf));

	return reinterpret_cast<DShBuf*>(aBuf)->Close(K::TheKernelProcess);
	}

/**
   Creates a user process handle to a buffer in the kernel address

   @param aBuf            Pointer to the buffer.
   @param aThread         Pointer to user process thread, if null
                          current thread is used.

   @pre Calling thread must be in a critical section.

   @return the handle if successful, otherwise one of the system-wide error codes.
*/
EXPORT_C TInt Kern::ShBufMakeHandleAndOpen(TShBuf* aBuf, DThread* aThread)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "Kern::ShBufMakeHandleAndOpen");

	if (!aThread)
		aThread = TheCurrentThread;

	TUint attr = (TUint)RObjectIx::EReserved;

	TInt h;
	TInt r = aThread->MakeHandleAndOpen(EOwnerProcess, reinterpret_cast<DShBuf*>(aBuf), h, attr);
	return (r == KErrNone) ? h : r;
	}

// DShBuf implementation
DShBuf::DShBuf(DShPool* aPool, TLinAddr aRelAddr) : iPool(aPool), iRelAddress(aRelAddr)
	{
	}

TInt DShBuf::Construct()
	{
	return K::AddObject(this, EShBuf);
	}

DShBuf::DShBuf(DShPool* aPool) : iPool(aPool), iRelAddress(0)
	{
	}

DShBuf::~DShBuf()
	{
	}

TInt DShBuf::RequestUserHandle(DThread* __DEBUG_ONLY(aThread), TOwnerType aType, TUint aAttr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DShBuf::RequestUserHandle (aThread = 0x%08x, aType = 0x%08x)", aThread, aType));

	if (aType != EOwnerProcess)
		{
		__KTRACE_OPT(KFAIL, Kern::Printf("Tried to create thread handle to DShBuf"));
		return KErrNotSupported;
		}

	if ((aAttr & RObjectIx::EReserved) != RObjectIx::EReserved)
		{
		return KErrNotSupported;
		}

	return KErrNone;
	}

TInt DShBuf::Close(TAny* __DEBUG_ONLY(aPtr))
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DShBuf::Close (0x%08x)", aPtr));

	if (AccessCount() == 1)
		{
		__KTRACE_OPT(KMMU, Kern::Printf("Closing DShBuf"));
		iPool->Free(this); // put back on free list
		}
	else
		{
		Dec();
		}

	return KErrNone;
	}

TInt DShBuf::Pin(TPhysicalPinObject* /* aPinObject */, TBool /* aReadOnly */, TPhysAddr& /* aAddress */, TPhysAddr* /* aPages */, TUint32& /* aMapAttr */, TUint& /* aColour */)
	{
	return KErrNone;
	}


// DShPool implementation

DShPool::DShPool() : iNotifDfc(ManagementDfc,this, 3)
	{}

DShPool::~DShPool()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DShPool::~DShPool"));

	if (iProcessLock)
		iProcessLock->Close(NULL);

	delete iClientMap;

	__KTRACE_OPT(KMMU, Kern::Printf("<DShPool::~DShPool"));
	}

TInt DShPool::Create(DObject* aOwner, TShPoolCreateInfo& aInfo)
	{
	__KTRACE_OPT(KMMU,Kern::Printf(">DShPool::Create (iBufSize = 0x%08x, iInitialBufs = %d, iMaxBufs = %d, iFlags = 0x%08x, iPhysAddr.iPhysAddrList = 0x%08x, iAlignment = %d)", aInfo.iInfo.iBufSize, aInfo.iInfo.iInitialBufs, aInfo.iInfo.iMaxBufs, aInfo.iInfo.iFlags, aInfo.iPhysAddr.iPhysAddrList, aInfo.iInfo.iAlignment));

	TInt r = SetOwner(aOwner);
	if (r != KErrNone)
		return r;

	TUint32 pageSize = Kern::RoundToPageSize(1);
	TUint32 pageShift = __e32_find_ms1_32(pageSize);

	// Sanity-check arguments
	// Don't allow buffer size, growth threshold or alignment <= 0
	// Alignment needs to be between 0 and 12 (don't allow > one page)
	// We will force alignment to be at least 4 bytes (others have suggested 64 bytes, cache line size)
	// We also require that there are an exact number of iBufSize's in iInitialBufs.
	// For EDevice pools, buffer size must be a multiple of alignment, and iGrowRatio must be 0.
	//

	// Remember buffer attributes
	iBufSize = aInfo.iInfo.iBufSize;
	iGrowTriggerRatio = aInfo.iInfo.iGrowTriggerRatio;
	iGrowByRatio = aInfo.iInfo.iGrowByRatio;
	iShrinkHysteresisRatio = aInfo.iInfo.iShrinkHysteresisRatio;
	iInitialBuffers = aInfo.iInfo.iInitialBufs;
	iMaxBuffers = aInfo.iInfo.iMaxBufs;
	iPoolFlags = aInfo.iInfo.iFlags;

	// No automatic growing and shrinking if the pool is already at its maximum size
	// or if either grow ratios are zero.
	if (iInitialBuffers == iMaxBuffers ||iGrowTriggerRatio == 0 || iGrowByRatio == 0)
		{
		iGrowTriggerRatio = aInfo.iInfo.iGrowTriggerRatio = 0;
		iGrowByRatio = aInfo.iInfo.iGrowByRatio = 0;
		}
	else
		{
		// Arbitrarily cap iGrowByRatio at something large (later on we will divide by
		// iGrowByRatio + 256, so that sum must not end up as 0x100000000).
		// iGrowTriggerRatio must represent a number < 1, or the pool will grow immediately.
		if (iGrowTriggerRatio > 256 || iGrowByRatio > (TUint)KMaxTInt32)
			return KErrArgument;

		// If growing or shrinking, hysteresis must be >= 1.  Also cap arbitrarily.
		// (1.0 as fx24.8 == 256)
		if (iShrinkHysteresisRatio < 256 || iShrinkHysteresisRatio > (TUint)KMaxTInt32)
			return KErrArgument;
		}

	if (iPoolFlags & EShPoolPageAlignedBuffer)
		{
		iAlignment = aInfo.iInfo.iAlignment = pageShift;
		}
	else
		{
		// How we're going to cut the buffer up
		iAlignment = aInfo.iInfo.iAlignment;

		// Ensure buffers will be aligned on cache line boundaries, so that DMA
		// will work properly.
		TUint minAlignment = __e32_find_ms1_32(Cache::DmaBufferAlignment());
		if (minAlignment < 5)					// Absolute minimum 32-byte alignment
			minAlignment = 5;
		if (minAlignment > pageShift)			// Absolute maximum page alignment
			minAlignment = pageShift;

		if (iAlignment < minAlignment) iAlignment = minAlignment;

		// Can't have exclusive access on a non-page-aligned pool.
		if (iPoolFlags & EShPoolExclusiveAccess)
			return KErrArgument;
		}

	// XXX implementation of exclusive access is Phase 2.
	if (iPoolFlags & EShPoolExclusiveAccess)
		return KErrNotSupported;

	iBufGap = (iBufSize + (1 << iAlignment) - 1) & ~((1 << iAlignment) - 1);

	if (iPoolFlags & EShPoolGuardPages)
		{
		// must be aligned
		if ((iPoolFlags & EShPoolPageAlignedBuffer) == 0)
			return KErrArgument;
		iBufGap += pageSize;
		}

	// Checks that are valid for both ERAM and EDevice
	if ((iMaxBuffers == 0) || (iBufSize == 0) || (iBufSize > (1 << 30)) ||
		(aInfo.iInfo.iAlignment > pageShift))
		{
		return KErrArgument;
		}

	if (iPoolFlags & EShPoolPhysicalMemoryPool)
		{
		// Checks that are only valid for EDevice
		if (aInfo.iPhysAddr.iPhysAddrList == 0 || iGrowTriggerRatio != 0 || iGrowByRatio != 0 ||
			iBufSize > iBufGap || iMaxBuffers != iInitialBuffers)
			{
			return KErrArgument;
			}

		// check there are enough pages to fit all the buffers
		iCommittedPages = ((iInitialBuffers * iBufGap) + pageSize - 1) >> pageShift;

		if (iCommittedPages > aInfo.iPages)
			return KErrArgument;

		iCommittedPages = aInfo.iPages;
		}
	else
		{
		// Checks that are only valid for ERAM
		if (aInfo.iPhysAddr.iPhysAddrList != 0 ||
			iInitialBuffers > iMaxBuffers)
			{
			return KErrArgument;
			}
		if ((iGrowTriggerRatio == 0 || iGrowByRatio == 0) &&
			(iInitialBuffers != iMaxBuffers))
			{
			return KErrArgument;
			}
		if ((iGrowTriggerRatio != 0 || iGrowByRatio != 0) &&
			(iPoolFlags & EShPoolNonPageAlignedBuffer) &&
			(iPoolFlags & EShPoolContiguous))
			{
			return KErrArgument;
			}
		}

	r = Kern::MutexCreate(iProcessLock, _L("ShPoolProcessLock"), KMutexOrdShPool);

	if (r == KErrNone)
		r = DoCreate(aInfo);

	if (r == KErrNone)
		r = CreateInitialBuffers();

	if (r == KErrNone)
		{
		iClientMap = new SMap(&iLock, iProcessLock);
		if (iClientMap == NULL)
			r = KErrNoMemory;
		}

	// Pre-calculate the shrink-by ratio as 1 - 1 / (1 + G), where
	// G is the grow-by ratio.  The value of iGrowByRatio is capped above,
	// so we know we won't be dividing by zero.
	if (r == KErrNone)
		{
		iShrinkByRatio = 256 - 65536 / (256 + iGrowByRatio);

		CalculateGrowShrinkTriggers();

		iFreeSpaceThreshold = iMaxBuffers + 1;		// make sure this won't fire just yet

		iNotifDfc.SetDfcQ(&iSharedDfcQue);

		r = K::AddObject(this, EShPool);
		}

	__KTRACE_OPT(KMMU, Kern::Printf("<DShPool::Create returns %d", r));
	return r;
	}

TInt DShPool::Close(TAny* aPtr)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DShPool::Close (0x%08x)", aPtr));

	if (AccessCount() > 1)
		{
		Dec();
		return KErrNone;
		}

	CompleteAllNotifications();

	DeleteInitialBuffers();

	SDblQueLink* pLink;
	// free up any remaining buffers, these must have been dynamically allocated
	// because we already deleted the initial buffers
	while (!iFreeList.IsEmpty())
		{
		pLink = iFreeList.GetFirst();
		DShBuf* buf = _LOFF(pLink, DShBuf, iObjLink);
		buf->DObject::Close(NULL);
		}

	// free up any remaining buffers, these must have been dynamically allocated
	// because we already deleted the initial buffers
	while (!iAltFreeList.IsEmpty())
		{
		pLink = iAltFreeList.GetFirst();
		DShBuf* buf = _LOFF(pLink, DShBuf, iObjLink);
		buf->DObject::Close(NULL);
		}

	// call base class

	return DObject::Close(aPtr);
	}

void DShPool::GetInfo(TShPoolInfo& aInfo)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DShPool::GetInfo"));
	aInfo.iBufSize		= iBufSize;
	aInfo.iInitialBufs	= iInitialBuffers;
	aInfo.iMaxBufs		= iMaxBuffers;
	aInfo.iGrowTriggerRatio	= iGrowTriggerRatio;
	aInfo.iGrowByRatio	= iGrowByRatio;
	aInfo.iShrinkHysteresisRatio	= iShrinkHysteresisRatio;
	aInfo.iAlignment	= iAlignment;
	aInfo.iFlags		= iPoolFlags;
	}

// This method is called after we grow or shrink the pool.  It re-calculates
// the actual numbers of trigger buffers for growing and shrinking based on
// the fx24.8 ratios.
//
// The triggers are set so that they can be blindly compared against, even if
// no automatic growing or shrinking is happening.
//
void DShPool::CalculateGrowShrinkTriggers()
	{
	LockPool();

	// If the pool is at its maximum size, we can't grow
	if (iTotalBuffers >= iMaxBuffers || iGrowTriggerRatio == 0)
		{
		iGrowTrigger = 0;
		}
	else
		{
		iGrowTrigger = mult_fx248(iTotalBuffers, iGrowTriggerRatio);

		// Deal with rounding towards zero
		if (iGrowTrigger == 0)
			iGrowTrigger = 1;
		}

	// If no growing has happened, we can't shrink
	if (iTotalBuffers <= iInitialBuffers || iGrowTriggerRatio == 0 || (iPoolFlags & EShPoolSuppressShrink) != 0)
		{
		iShrinkTrigger = iMaxBuffers;
		}
	else
		{
		// To ensure that shrinking doesn't immediately happen after growing, the trigger
		// amount is the grow trigger + the grow amount (which is the number of free buffers
		// just after a grow) times the shrink hysteresis value.
		iShrinkTrigger = mult_fx248(iTotalBuffers, iGrowTriggerRatio + iGrowByRatio);
		iShrinkTrigger = mult_fx248(iShrinkTrigger, iShrinkHysteresisRatio);

		// Deal with rounding towards zero
		if (iShrinkTrigger == 0)
			iShrinkTrigger = 1;

		// If the shrink trigger ends up > the number of buffers currently in
		// the pool, set it to that number (less 1, since the test is "> trigger").
		// This means the pool will only shrink when all the buffers have been freed.
		if (iShrinkTrigger >= iTotalBuffers)
		    iShrinkTrigger = iTotalBuffers - 1;
		}

	UnlockPool();
	}

// Multiplies an unsigned integer by an fx24.8 fixed-point value
// Returns the value, or 0xFFFFFFFF if there was overflow.
//
TUint DShPool::mult_fx248(TUint n, TUint f)
	{
	TUint64 r = (TUint64) n * f;

	I64LSR(r, 8);

	return r > KMaxTUint32 ? KMaxTUint32 : I64LOW(r);
	}

TInt DShPool::CreateInitialBuffers()
	{
	// This virtual must always be implemented in the derived class
	K::Fault(K::EShBufVirtualNotDefined);

	return KErrNone;
	}

TInt DShPool::DeleteInitialBuffers()
	{
	// This virtual must always be implemented in the derived class
	K::Fault(K::EShBufVirtualNotDefined);

	return KErrNone;
	}

TUint DShPool::FreeCount()
	{
	return iFreeBuffers;
	}

void DShPool::ManagementDfc(TAny* aPool)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DShPool::ManagementDfc (aPool = 0x%08x)", aPool));

	DShPool* pool = static_cast<DShPool*>(aPool);

	// DFC to auto alloc and complete space notifications
	// It is queued as a result of alloc'ing and freeing
	// it looks at the difference between total committed memory and used committed memory and eventually commits more memory (up to Max pool size)
	// and calls CheckAndCompleteNotifications to complete any pending free space notifications.
	// Even when it does not commit memory it must call CheckAndCompleteNotifications with the difference above, as it is also queued from Free
	// and should notify clients that enough space became free
	pool->UpdateFreeList();

	pool->LockPool();
	if (pool->iFreeBuffers < pool->iGrowTrigger)		// do not use <=, since iGrowTrigger of 0
		{												// => no growing.
		pool->UnlockPool();
		__KTRACE_OPT(KMMU, Kern::Printf("GrowPool() because free %d < grow trigger %d", pool->iFreeBuffers, pool->iGrowTrigger));
		pool->GrowPool();
		}
	else if (pool->iFreeBuffers > pool->iShrinkTrigger)		// do not use >=, since iShrinkTrigger of 0
		{													// => no shrinking.
		pool->UnlockPool();
		__KTRACE_OPT(KMMU, Kern::Printf("ShrinkPool() because free %d > shrink trigger %d", pool->iFreeBuffers, pool->iShrinkTrigger));
		pool->ShrinkPool();
		}
	else
		{
		pool->UnlockPool();
		}

	pool->CheckAndCompleteNotifications(EFalse);

	// We might be able to grow/shrink some more. Give the Management DFC another kick if necessary.
	if (pool->HaveWorkToDo())
		{
		pool->KickManagementDfc();
		}

	pool->Close(NULL);
	__KTRACE_OPT(KMMU, Kern::Printf("<DShPool::ManagementDfc (aPool = 0x%08x)", aPool));
	}


// Forward declaration needed by TShPoolNotificationCleanup
class TShPoolNotificationRequest;

/**
	@internalComponent
	@prototype
*/
class TShPoolNotificationCleanup : public TThreadCleanup
	{
public:
	virtual void Cleanup();
	TShPoolNotificationRequest& NotificationRequest();
	};

/**
	@internalComponent
	@prototype
*/
class TShPoolNotificationRequest : public TClientRequest
// notification requests need to be queued
	{
public:
	TShPoolNotificationRequest(TShPoolNotifyType aType, TUint aThreshold, DThread* aThread, TRequestStatus* aStatus);
	void Complete(TInt aReason);
	void AddCleanup();
	void RemoveCleanup();

public:
	TShPoolNotifyType iNotificationType;
	TUint iThreshold;	// Our key to order this queue in ascending order. When growing pool we follow list down to appropriate threshold and complete requests accordingly.
	SDblQueLink iObjLink;
	DShPool* iPool;		// pointer to pool, not reference counted, removed when notification removed from pool, which always happens prior to pool deletion
	DThread* iOwningThread;
	TShPoolNotificationCleanup iCleanup;

	static NFastMutex ShPoolNotifierLock;		// fast mutex to protect notifier list

	inline static void Lock()
		{ NKern::FMWait(&ShPoolNotifierLock); }
	inline static void Unlock()
		{ NKern::FMSignal(&ShPoolNotifierLock); }
	};

NFastMutex TShPoolNotificationRequest::ShPoolNotifierLock;		// fast mutex to protect notifier list

inline TShPoolNotificationRequest& TShPoolNotificationCleanup::NotificationRequest()
	{ return *_LOFF(this, TShPoolNotificationRequest, iCleanup); }


TShPoolNotificationRequest::TShPoolNotificationRequest(TShPoolNotifyType aType, TUint aThreshold, DThread* aThread, TRequestStatus* aStatus)
	: TClientRequest()
	{
	iNotificationType = aType;
	iThreshold = aThreshold;
	iObjLink.iNext = NULL;
	iPool = NULL;
	iOwningThread = aThread;
	SetStatus(aStatus);
	}

void TShPoolNotificationRequest::Complete(TInt aReason)
	{
	NKern::LockSystem();
	RemoveCleanup();
	NKern::UnlockSystem();

	Kern::QueueRequestComplete(iOwningThread, this, aReason);
	DThread *pT = iOwningThread;
	Close();
	pT->Close(NULL);
	}

void TShPoolNotificationRequest::AddCleanup()
	{
	NKern::LockSystem();
	TheCurrentThread->AddCleanup(&iCleanup);
	NKern::UnlockSystem();
	}

void TShPoolNotificationRequest::RemoveCleanup()
	{
	if (iCleanup.iThread != NULL)
		{
		iCleanup.Remove();
		iCleanup.iThread = NULL;
		}
	}

// Called when the thread that requested a notification exits.
// Called in the context of the exiting thread, with the system locked.
void TShPoolNotificationCleanup::Cleanup()
	{
	__ASSERT_SYSTEM_LOCK;
	TShPoolNotificationRequest& req = NotificationRequest();
	Remove();				// take this cleanup item off thread cleanup list
	iThread = NULL;
	NKern::UnlockSystem();
	// Notifier may still be on the pool notifier list

	TShPoolNotificationRequest::Lock();
	if (req.iObjLink.iNext)
		{
		DShPool* pool = req.iPool;
		SDblQueLink* anchor = NULL;
		switch (req.iNotificationType)
			{
			case EShPoolLowSpace:
				anchor = &pool->iNotifLowReqQueue.iA;
				break;
			case EShPoolFreeSpace:
				anchor = &pool->iNotifFreeReqQueue.iA;
				break;
			}

		req.iObjLink.Deque();
		req.iObjLink.iNext = NULL;

		// We "should not" have anything other than LowSpace or FreeSpace
		// notification objects.
		__NK_ASSERT_DEBUG(anchor != NULL);

		TBool empty = (anchor->iNext == anchor);
		switch (req.iNotificationType)
			{
			case EShPoolLowSpace:
				pool->iLowSpaceThreshold = empty ? 0 : _LOFF(anchor->iNext, TShPoolNotificationRequest, iObjLink)->iThreshold + 1;
				break;
			case EShPoolFreeSpace:
				pool->iFreeSpaceThreshold = empty ? pool->iMaxBuffers + 1 : _LOFF(anchor->iNext, TShPoolNotificationRequest, iObjLink)->iThreshold;
				break;
			}

		TShPoolNotificationRequest::Unlock();

		DThread* pT = req.iOwningThread;
		req.iOwningThread = NULL;

		req.Close();

		// If the notification code isn't looking at this request, close the owning thread.
		// This balances the Open() in AddNotification()
		if (pT)
			pT->Close(NULL);
		}
	else
		{
		TShPoolNotificationRequest::Unlock();
		}

	NKern::LockSystem();
	}

TInt DShPool::AddNotification(TShPoolNotifyType aType, TUint aThreshold, TRequestStatus& aStatus)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DShPool::AddNotification(%d, %d)", aType, aThreshold));

	DThread* pT = TheCurrentThread;	// This might involve calling a function, so only do it once

	TShPoolNotificationRequest* req;
	switch (aType)
		{
		case EShPoolLowSpace:
			req = new TShPoolNotificationRequest(aType, aThreshold, pT, &aStatus);
			break;
		case EShPoolFreeSpace:
			req = new TShPoolNotificationRequest(aType, aThreshold, pT, &aStatus);
			break;
		default:
			return KErrArgument;
		}

	if (req == NULL)
		return KErrNoMemory;

	if ((aType == EShPoolLowSpace && iFreeBuffers <= req->iThreshold) ||
	    (aType == EShPoolFreeSpace && iFreeBuffers > req->iThreshold))
		{
		// Complete immediately
		Kern::QueueRequestComplete(pT, req, KErrNone);
		req->Close();
		}
	else
		{
		// Add notifier to thread before adding it to pool, since thread can't die but
		// notifier could complete as soon as we release ShPoolNotifierLock after adding to pool
		pT->Open();
		req->AddCleanup();
		TShPoolNotificationRequest::Lock();
		req->iPool = this;		// for QueueAnchor

		// add this request to its queue of requests
		SDblQueLink* anchor;
		SDblQueLink* pLink;
		switch (aType)
			{
			case EShPoolLowSpace:
				anchor = &iNotifLowReqQueue.iA;

				for (pLink = anchor->iNext;
					pLink != anchor && _LOFF(pLink, TShPoolNotificationRequest, iObjLink)->iThreshold >= aThreshold;
					pLink = pLink->iNext)
					{}		/* nothing */

				// Insert before first entry with strictly lower threshold;
				// if no such entry, inserting before the anchor makes it last in the list
				req->iObjLink.InsertBefore(pLink);

				// Remember the threshold of the first notification on the list (will be the highest).
				iLowSpaceThreshold = _LOFF(anchor->iNext, TShPoolNotificationRequest, iObjLink)->iThreshold + 1;
				break;

			case EShPoolFreeSpace:
				anchor = &iNotifFreeReqQueue.iA;

				for (pLink = anchor->iNext;
					pLink != anchor && _LOFF(pLink, TShPoolNotificationRequest, iObjLink)->iThreshold < aThreshold;
					pLink = pLink->iNext)
					{}		/* nothing */

				// Insert before first entry with strictly higher threshold;
				// if no such entry, inserting before the anchor makes it last in the list
				req->iObjLink.InsertBefore(pLink);

				// Remember the threshold of the first notification on the list (will be the lowest).
				iFreeSpaceThreshold = _LOFF(anchor->iNext, TShPoolNotificationRequest, iObjLink)->iThreshold;
				break;
			}

		TShPoolNotificationRequest::Unlock();

		// Care required here: new notifier could be completed right here
		}

	// Queue the ManagementDfc, which completes notifications as appropriate
	if (HaveWorkToDo())
		KickManagementDfc();

	return KErrNone;
	}

void DShPool::KickManagementDfc()
	{
	Open();
	if (!iNotifDfc.Enque())
		Close(NULL);
	}

TBool DShPool::HaveWorkToDo()
	{
	// Sufficiently few free buffers that at least one low space notifier will be triggered
	// Avoid accessing notifier list here since we don't hold any locks
	if (iFreeBuffers < iLowSpaceThreshold)		// iLowSpaceThreshold == 0 => no low space waiters
		return ETrue;							// else it's the first threshold + 1

	if (iFreeBuffers >= iFreeSpaceThreshold)	// iFreeSpaceThreshold is the first threshold value
		return ETrue;							// (no +1 here)

	if (iFreeBuffers < iGrowTrigger)			// do not use <=, it will break things
		return ETrue;

	if (iFreeBuffers > iShrinkTrigger)			// do not use >=, it will break things
		return ETrue;

	return EFalse;
	}

TInt DShPool::RemoveNotification(TShPoolNotifyType aType, TRequestStatus& aStatus)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DShPool::RemoveNotification"));

	SDblQueLink* anchor = NULL;
	switch (aType)
		{
		case EShPoolLowSpace:
			anchor = &iNotifLowReqQueue.iA;
			break;
		case EShPoolFreeSpace:
			anchor = &iNotifFreeReqQueue.iA;
			break;
		}

	if (anchor == NULL)
		return KErrArgument;

	DThread* pT = TheCurrentThread;	// This might involve calling a function, so only do it once
	SDblQueLink* pLink;

	TShPoolNotificationRequest* req = NULL;
	TShPoolNotificationRequest::Lock();
	for (pLink = anchor->iNext; pLink != anchor; pLink = pLink->iNext)
		{
		req = _LOFF(pLink, TShPoolNotificationRequest, iObjLink);
		if ((req->iStatus == ((T_UintPtr)&aStatus & ~KClientRequestFlagMask))
			&&	(req->iOwningThread == pT))
			break;
		}

	if (pLink == anchor)
		{
		// Not found on the list.  Ah well.
		TShPoolNotificationRequest::Unlock();
		return KErrNotFound;
		}

	TBool first = (pLink == anchor->iNext);
	TBool last = (pLink == anchor->iPrev);
	pLink->Deque();
	pLink->iNext = NULL;

	// Notifier won't now be completed, and since its owning thread is the current
	// thread we are now home and dry: no-one else can touch this notifier.
	if (first)
		{
		switch (aType)
			{
			case EShPoolLowSpace:
				// This was first on the list, so adjust iLowSpaceThreshold
				iLowSpaceThreshold = last ? 0 : _LOFF(anchor->iNext, TShPoolNotificationRequest, iObjLink)->iThreshold + 1;
				break;
			case EShPoolFreeSpace:
				// This was first on the list, so adjust iFreeSpaceThreshold
				iFreeSpaceThreshold = last ? iMaxBuffers + 1 : _LOFF(anchor->iNext, TShPoolNotificationRequest, iObjLink)->iThreshold;
				break;
			}
		}

	TShPoolNotificationRequest::Unlock();
	req->Complete(KErrCancel);
	return KErrNone;			// removed successfully
	}

void DShPool::CompleteAllNotifications()
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DShPool::CompleteAllNotifications"));

	// Cancel all outstanding notifications on pool close
	CheckAndCompleteNotifications(ETrue);
	}

void DShPool::CheckAndCompleteNotifications(TBool aAll)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DShPool::CheckAndCompleteNotifications"));

	CheckLowSpaceNotificationQueue(aAll);
	CheckFreeSpaceNotificationQueue(aAll);
	}

void DShPool::CheckLowSpaceNotificationQueue(TBool aAll)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DShPool::CheckLowSpaceNotificationQueue"));
	// Goes through one of the ordered lists and completes all notifications on this new size
	TInt result = aAll ? KErrCancel : KErrNone;

	SDblQueLink* anchor = &iNotifLowReqQueue.iA;

	TShPoolNotificationRequest::Lock();

	while (aAll || iFreeBuffers < iLowSpaceThreshold)
		{
		// take first notifier off list and complete it, provided owning thread has not terminated
		SDblQueLink* pLink = anchor->iNext;
		if (pLink == anchor)
			break;

		TBool last = (pLink == anchor->iPrev);
		iLowSpaceThreshold = last ? 0 : _LOFF(pLink->iNext, TShPoolNotificationRequest, iObjLink)->iThreshold + 1;

		pLink->Deque();
		pLink->iNext = NULL;

		// The notifier has been detached from the pool's notifier list but is still attached to its
		// owning thread. As soon as we signal ShPoolNotifierLock the notifier may be zapped by the owning
		// thread's cleanup code.
		TShPoolNotificationRequest* req = _LOFF(pLink, TShPoolNotificationRequest, iObjLink);
		TShPoolNotificationRequest::Unlock();

		// Owning thread is still alive, detach notifier from it and complete
		req->Complete(result);
		TShPoolNotificationRequest::Lock();
		}

	TShPoolNotificationRequest::Unlock();
	}

void DShPool::CheckFreeSpaceNotificationQueue(TBool aAll)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DShPool::CheckFreeSpaceNotificationQueue"));
	// Goes through one of the ordered lists and completes all notifications on this new size
	TInt result = aAll ? KErrCancel : KErrNone;

	SDblQueLink* anchor = &iNotifFreeReqQueue.iA;

	TShPoolNotificationRequest::Lock();

	while (aAll || iFreeBuffers >= iFreeSpaceThreshold)
		{
		// take first notifier off list and complete it, provided owning thread has not terminated
		SDblQueLink* pLink = anchor->iNext;
		if (pLink == anchor)
			break;

		TBool last = (pLink == anchor->iPrev);
		iFreeSpaceThreshold = last ? iMaxBuffers + 1 : _LOFF(pLink->iNext, TShPoolNotificationRequest, iObjLink)->iThreshold;

		pLink->Deque();
		pLink->iNext = NULL;

		// The notifier has been detached from the pool's notifier list but is still attached to its
		// owning thread. As soon as we signal ShPoolNotifierLock the notifier may be zapped by the owning
		// thread's cleanup code.
		TShPoolNotificationRequest* req = _LOFF(pLink, TShPoolNotificationRequest, iObjLink);
		TShPoolNotificationRequest::Unlock();

		// Owning thread is still alive, detach notifier from it and complete
		req->Complete(result);
		TShPoolNotificationRequest::Lock();
		}

	TShPoolNotificationRequest::Unlock();
	}


TInt DShPool::OpenClient(DProcess* aProcess, TUint& aFlags)
	{
	LockPool();
	DShPoolClient* client = reinterpret_cast<DShPoolClient*>(iClientMap->Find(reinterpret_cast<TUint>(aProcess)));

	if ((client != NULL) && client->iAccessCount)
		{
		// access count must be non-zero otherwise the pool is in the process of being closed
		aFlags = client->iFlags;
		client->iAccessCount++;
		}
	else
		{
		UnlockPool();
		aFlags = 0;
		return KErrNotFound;
		}

	UnlockPool();
	return KErrNone;
	}

void DShPool::CloseClient(DProcess* aProcess)
	{
	LockPool();
	DShPoolClient* client = reinterpret_cast<DShPoolClient*>(iClientMap->Find(reinterpret_cast<TUint>(aProcess)));

	if (client == NULL)
		{
		UnlockPool();
		return;
		}

	TInt r = --client->iAccessCount;

	UnlockPool();

	if (r == 0)
		{
		// check that it has not been reopened by another thread in the same process
		Kern::MutexWait(*iProcessLock);
		if (r == client->iAccessCount)
			{
			DestroyClientResources(aProcess);
			}
		Kern::MutexSignal(*iProcessLock);
		}
	}

TInt DShPool::DoCreate(TShPoolCreateInfo& /*aInfo*/)
	{
	// This virtual must always be implemented in the derived class
	K::Fault(K::EShBufVirtualNotDefined);

	return KErrNotSupported;
	}

TInt DShPool::RequestUserHandle(DThread* __DEBUG_ONLY(aThread), TOwnerType aType)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DShPool::RequestUserHandle (aThread = 0x%08x, aType = 0x%08x)", aThread, aType));

	if (aType != EOwnerProcess)
		{
		__KTRACE_OPT(KFAIL,Kern::Printf("Tried to create thread handle to DShPool"));
		return KErrNotSupported;
		}

	return KErrNone;
	}

TInt DShPool::UpdateReservedHandles(TInt aNoOfBuffers)
	{
	__KTRACE_OPT(KMMU, Kern::Printf(">DShPool::UpdateReservedHandles(%d)", aNoOfBuffers));

	TInt ret = KErrNone;

	// reserve handles for interested processes
	Kern::MutexWait(*iProcessLock);
	SMap::TIterator iter(*iClientMap);

	SMap::TEntry* entry;
	SMap::TEntry* lastEntry = NULL;

	while ((entry = iter.Next()) != lastEntry)
		{
		DProcess* pP = reinterpret_cast<DProcess*>(entry->iKey);

		if (pP)
			{
			TInt r = pP->iHandles.Reserve(aNoOfBuffers);

			// try to cleanup the best we can
			if (r != KErrNone && ret == KErrNone)
				{
				iter.Reset();
				lastEntry = entry;
				aNoOfBuffers = -aNoOfBuffers;
				ret = r;
				}
			}
		}

	Kern::MutexSignal(*iProcessLock);

	return ret;
	}

TInt DShPool::SetBufferWindow(DProcess* /* aProcess */, TInt /* aWindowSize*/ )
	{
	return KErrNotSupported;
	}
