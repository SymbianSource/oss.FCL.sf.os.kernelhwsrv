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
// e32\memmodel\emul\win32\mutils.cpp
// 
//

#include "memmodel.h"
#include <kernel/cache.h>
#include <emulator.h>

void MM::Panic(MM::TMemModelPanic aPanic)
	{
	Kern::Fault("MemModel", aPanic);
	}

TInt M::PageSizeInBytes()
	{
	return MM::RamPageSize;
	}

TBool M::IsRomAddress(const TAny* )
	{
	return EFalse;
	}

TUint32 MM::RoundToPageSize(TUint32 aSize)
	{
	TUint32 m=MM::RamPageSize-1;
	return (aSize+m)&~m;
	}

EXPORT_C TUint32 Kern::RoundToPageSize(TUint32 aSize)
	{
	return MM::RoundToPageSize(aSize);
	}

EXPORT_C TUint32 Kern::RoundToChunkSize(TUint32 aSize)
	{
	return MM::RoundToChunkSize(aSize);
	}

void MM::Init1()
	{
	TheScheduler.SetProcessHandler((TLinAddr)DoProcessSwitch);
	}
void MM::Wait()
	{
	Kern::MutexWait(*RamAllocatorMutex);
	if (RamAllocatorMutex->iHoldCount==1)
		{
		InitialFreeMemory=FreeMemory;
		AllocFailed=EFalse;
		}
	}

TInt MM::Commit(TLinAddr aBase, TInt aSize, TInt aClearByte, TBool aExecute)
//
// Get win32 to commit the pages.
// We know they are not already committed - this is guaranteed by the caller so we can update the memory info easily
//
	{
	__ASSERT_MUTEX(RamAllocatorMutex);

	if (aSize==0)
		return KErrNone;

	if (MM::FreeMemory+MM::CacheMemory >= aSize)
		{
		__LOCK_HOST;
		DWORD protect = aExecute ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
		if (VirtualAlloc(LPVOID(aBase), aSize, MEM_COMMIT, protect))
			{
			TInt reclaimed = aSize - MM::FreeMemory;
			if(reclaimed<=0)
				MM::FreeMemory -= aSize;
			else
				{
				// some cache memory was needed for this commit...
				MM::FreeMemory = 0;
				MM::CacheMemory -= reclaimed;
				MM::ReclaimedCacheMemory += reclaimed;
				}
			MM::CheckMemoryCounters();

			// Clear memory to value determined by chunk member
			memset(reinterpret_cast<void*>(aBase), aClearByte, aSize);

			return KErrNone;
			}
		}
	MM::AllocFailed = ETrue;
	return KErrNoMemory;
	}

TInt MM::Decommit(TLinAddr aBase, TInt aSize)
//
// Get win32 to decommit the pages.
// The pages may or may not be committed: we need to find out which ones are so that the memory info is updated correctly
//
	{
	__ASSERT_MUTEX(RamAllocatorMutex);

	TInt freed = 0;
	TInt remain = aSize;
	TLinAddr base = aBase;
	__LOCK_HOST;
	while (remain > 0)
		{
		MEMORY_BASIC_INFORMATION info;
		VirtualQuery(LPVOID(base), &info, sizeof(info));
		TInt size = Min(remain, info.RegionSize);
		if (info.State == MEM_COMMIT)
			freed += size;

#ifdef BTRACE_CHUNKS
		BTraceContext12(BTrace::EChunks,BTrace::EChunkMemoryDeallocated,NULL,base,size);
#endif

		base += info.RegionSize;
		remain -= info.RegionSize;
		}
	VirtualFree(LPVOID(aBase), aSize, MEM_DECOMMIT);
	MM::FreeMemory += freed;
	__KTRACE_OPT(KMEMTRACE, {Kern::Printf("MT:A %d %x %x %O",NTickCount(),NULL,aSize,NULL);});

	return freed;
	}

void MM::CheckMemoryCounters()
	{
	__NK_ASSERT_ALWAYS(MM::CacheMemory>=0);
	__NK_ASSERT_ALWAYS(MM::ReclaimedCacheMemory>=0);
	__NK_ASSERT_ALWAYS(MM::FreeMemory+MM::CacheMemory>=0);
	}

void MM::Signal()
	{
	if (RamAllocatorMutex->iHoldCount>1)
		{
		Kern::MutexSignal(*RamAllocatorMutex);
		return;
		}
	TInt initial=InitialFreeMemory;
	TBool failed=AllocFailed;
	TInt final=FreeMemory;
	Kern::MutexSignal(*RamAllocatorMutex);
	K::CheckFreeMemoryLevel(initial,final,failed);
	}

void MM::DoProcessSwitch(TAny* aAddressSpace)
// Kernel locked on entry and exit
	{
	__NK_ASSERT_LOCKED;
	
	if (!aAddressSpace)
		return;
	
	DWin32Process* proc = (DWin32Process*)aAddressSpace;

	if (proc == K::TheKernelProcess) return;

	int count = proc->iDllData.Count();
	for (int ii=0; ii<count; ii++)
		{
		SProcessDllDataBlock& procData = proc->iDllData[ii];
		DWin32CodeSeg* codeSeg = procData.iCodeSeg;
		if (!codeSeg)
			continue;
		DWin32Process*& liveProc = codeSeg->iLiveProcess;
		if (liveProc == proc)
			continue;			// no change in live mapping
		if (liveProc)
			{
			// copy out old process data
			TInt liveIx = liveProc->iDllData.FindInUnsignedKeyOrder(procData);
			__ASSERT_ALWAYS(liveIx >= 0,MM::Panic(MM::EWsdDllNotInProcess));
			SProcessDllDataBlock& oldProcData = liveProc->iDllData[liveIx];
			memcpy(oldProcData.iDataCopy, (const TAny*)codeSeg->iDataDest, codeSeg->iRealDataSize);
			memcpy(oldProcData.iBssCopy, (const TAny*)codeSeg->iBssDest, codeSeg->iRealBssSize);
			}
		// copy new data in
		memcpy((TAny*)codeSeg->iDataDest, procData.iDataCopy, codeSeg->iRealDataSize);
		memcpy((TAny*)codeSeg->iBssDest, procData.iBssCopy, codeSeg->iRealBssSize);
		liveProc = proc;
		}
	}

TAny* MM::CurrentAddress(DThread* aThread, const TAny* aPtr, TInt aSize, TBool /*aWrite*/, TBool& aLocked)
// Enter and leave with system locked
// Kernel unlocked on entry
// Kernel may be locked on exit, iff aPtr is in DLL WSD.
// this is because the returned address is only valid until the
// target process DLL WSD changes live status, which can happen 
// independently when another thread runs.
// Lock status signaled in aLocked.
// Why? This allows the optimisation that WSD is only copied on
// process switch when necessary. The gain from that optimisation is
// expected to be much higher than the cost of leaving the kernel locked
// during (rare) IPC to DLL WSD
	{
	DWin32Process* proc = (DWin32Process*)aThread->iOwningProcess;
	// Is the address in DLL static data?
	NKern::Lock();
		
	TInt count = proc->iDllData.Count();
	TLinAddr p = (TLinAddr)aPtr;
	TLinAddr base = 0;
	TInt size = 0;
	TBool data = EFalse;
	aLocked = EFalse;
	for (TInt ii=0; ii<count; ii++)
		{
		const SProcessDllDataBlock& procData = proc->iDllData[ii];
		DWin32CodeSeg* codeSeg = procData.iCodeSeg;
		if (codeSeg->iDataDest <= p && p < codeSeg->iDataDest + codeSeg->iRealDataSize)
			{
			base = codeSeg->iDataDest;
			size = codeSeg->iRealDataSize;
			data = ETrue;
			}
		else if (codeSeg->iBssDest <= p && p < codeSeg->iBssDest + codeSeg->iRealBssSize)
			{
			base = codeSeg->iBssDest;
			size = codeSeg->iRealBssSize;
			}
		if (base)
			{
			// This is a DLL static address, check range validity
			if (p + aSize > base + size)
				{
				NKern::Unlock();
				return NULL;
				}
			
			DWin32Process* liveProc = codeSeg->iLiveProcess;

			if (proc == liveProc)
				{
				// If the target process is live, don't remap
				NKern::Unlock();
				return (TAny*)aPtr;
				}
			else
				{
				aLocked = ETrue;
				TLinAddr procBase = (TLinAddr)(data ? procData.iDataCopy : procData.iBssCopy);
				TLinAddr remapped = procBase + (p - base);
				return (TAny*) remapped;
				}
			}
		}
	NKern::Unlock();
	// No, the address does not need to be remapped
	return (TAny*)aPtr;
	}

void M::BTracePrime(TUint aCategory)
	{
	(void)aCategory;
#ifdef BTRACE_KERNEL_MEMORY
	// Must check for -1 as that is the default value of aCategory for
	// BTrace::Prime() which is intended to prime all categories that are 
	// currently enabled via a single invocation of BTrace::Prime().
	if(aCategory==BTrace::EKernelMemory || (TInt)aCategory == -1)
		{
		BTrace4(BTrace::EKernelMemory,BTrace::EKernelMemoryInitialFree,TheSuperPage().iTotalRamSize);
		BTrace4(BTrace::EKernelMemory,BTrace::EKernelMemoryCurrentFree,Kern::FreeRamInBytes());
		}
#endif
	}

/** 
Restart the system. 
On hardware targets this calls the Restart Vector in the ROM Header.
Note, aMode is set to zero when this function is used by Kern::Fault()

@param aMode Argument passed to the restart routine. The meaning of this value
depends on the bootstrap implementation. 		
*/
EXPORT_C void Kern::Restart(TInt)
	{
	ExitProcess(0);
	}

EXPORT_C TInt TInternalRamDrive::MaxSize()
	{
	return PP::RamDriveMaxSize;
	}

void M::FsRegisterThread()
	{
	}

void P::SetSuperPageSignature()
	{
	TUint32* sig = TheSuperPage().iSignature;
	sig[0] = 0xb504f333;
	sig[1] = 0xf9de6484;
	}

TBool P::CheckSuperPageSignature()
	{
	const TUint32* sig = TheSuperPage().iSignature;
	return ( sig[0]==0xb504f333 && sig[1]==0xf9de6484 );
	}

// Dummy implementation of kernel pin APIs

class TVirtualPinObject
	{	
	};

TInt M::CreateVirtualPinObject(TVirtualPinObject*& aPinObject)
	{
	aPinObject = new TVirtualPinObject;
	return aPinObject != NULL ? KErrNone : KErrNoMemory;
	}

TInt M::PinVirtualMemory(TVirtualPinObject* aPinObject, TLinAddr, TUint, DThread*)
	{
	__ASSERT_DEBUG(aPinObject, K::Fault(K::EVirtualPinObjectBad));
	(void)aPinObject;
	return KErrNone;
	}

TInt M::CreateAndPinVirtualMemory(TVirtualPinObject*& aPinObject, TLinAddr, TUint)
	{
	aPinObject = 0;
	return KErrNone;
	}

void M::UnpinVirtualMemory(TVirtualPinObject* aPinObject)
	{
	__ASSERT_DEBUG(aPinObject, K::Fault(K::EVirtualPinObjectBad));
	(void)aPinObject;
	}

void M::DestroyVirtualPinObject(TVirtualPinObject*& aPinObject)
	{
	TVirtualPinObject* object = (TVirtualPinObject*)__e32_atomic_swp_ord_ptr(&aPinObject, 0);
	if (object)
		Kern::AsyncFree(object);
	}


class TPhysicalPinObject
	{	
	};

TInt M::CreatePhysicalPinObject(TPhysicalPinObject*& aPinObject)
	{
	aPinObject = new TPhysicalPinObject;
	return aPinObject != NULL ? KErrNone : KErrNoMemory;
	}

TInt M::PinPhysicalMemory(TPhysicalPinObject* aPinObject, TLinAddr, TUint, TBool, TPhysAddr&, TPhysAddr*, TUint32&, TUint&, DThread*)
	{
	__ASSERT_DEBUG(aPinObject, K::Fault(K::EPhysicalPinObjectBad));
	(void)aPinObject;
	return KErrNone;
	}

void M::UnpinPhysicalMemory(TPhysicalPinObject* aPinObject)
	{
	__ASSERT_DEBUG(aPinObject, K::Fault(K::EPhysicalPinObjectBad));
	(void)aPinObject;
	}

void M::DestroyPhysicalPinObject(TPhysicalPinObject*& aPinObject)
	{
	TPhysicalPinObject* object = (TPhysicalPinObject*)__e32_atomic_swp_ord_ptr(&aPinObject, 0);
	if (object)
		Kern::AsyncFree(object);
	}

//
// Kernel map and pin (Not supported on the emulator).
//

TInt M::CreateKernelMapObject(TKernelMapObject*&, TUint)
	{
	return KErrNotSupported;
	}


TInt M::MapAndPinMemory(TKernelMapObject*, DThread*, TLinAddr, TUint, TUint, TLinAddr&, TPhysAddr*)
	{
	return KErrNotSupported;
	}


void M::UnmapAndUnpinMemory(TKernelMapObject*)
	{
	}


void M::DestroyKernelMapObject(TKernelMapObject*&)
	{
	}


// Misc DPagingDevice methods

EXPORT_C NFastMutex* DPagingDevice::NotificationLock()
	{
	// use the system lock
	return &TheScheduler.iLock;
	}

EXPORT_C void DPagingDevice::NotifyIdle()
	{
	// Not used on this memory model
	}

EXPORT_C void DPagingDevice::NotifyBusy()
	{
	// Not used on this memory model
	}

EXPORT_C TInt Cache::SyncPhysicalMemoryBeforeDmaWrite(TPhysAddr* , TUint , TUint , TUint , TUint32 )
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::SyncPhysicalMemoryBeforeDmaWrite");
	return KErrNotSupported;
	}

EXPORT_C TInt Cache::SyncPhysicalMemoryBeforeDmaRead(TPhysAddr* , TUint , TUint , TUint , TUint32 )
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::SyncPhysicalMemoryBeforeDmaRead");
	return KErrNotSupported;
	}
EXPORT_C TInt Cache::SyncPhysicalMemoryAfterDmaRead(TPhysAddr* , TUint , TUint , TUint , TUint32 )
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::SyncPhysicalMemoryAfterDmaRead");
	return KErrNotSupported;
	}
