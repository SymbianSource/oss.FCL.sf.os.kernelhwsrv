// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_cache_man.cpp
// 
//

/**
 @file
 @internalTechnology
*/


#include <e32std.h>
#include <e32std_private.h>
#include "sf_std.h"
#include <e32uid.h>
#include <e32wins.h>
#include <f32file.h>
#include <hal.h>
#include "sf_cache_man.h"
#include "sf_cache_client.h"
#include "sf_file_cache.h"
#include "sf_file_cache_defs.h"

//#undef __SIMULATE_LOCK_FAILURES__
//#define __RAMDOM_LOCK_FAILURES__
//#define __PSEUDO_RANDOM_FAILURES__		
//#define __LOCK_ENTIRE_CACHELINE__

#ifdef __RAMDOM_LOCK_FAILURES__	
#include <e32math.h>
#endif

#ifdef OST_TRACE_COMPILER_IN_USE
#include "sf_cache_manTraces.h"
#endif

#define CACHE_NUM_TO_ADDR(n) (iBase + (n << iCacheLineSizeLog2))
#define ADDR_TO_CACHELINE_ADDR(a) ((((a - iBase) >> iCacheLineSizeLog2) << iCacheLineSizeLog2) + iBase)
#define ADDR_TO_INDEX(a) ((a - iBase) >> iCacheLineSizeLog2)

#if defined(_DEBUG)
	#define __CACHELINE_INVARIANT(aCacheLine)						\
	__ASSERT_DEBUG(												\
		aCacheLine.iDirtySegments <= aCacheLine.iFilledSegments &&	\
		aCacheLine.iFilledSegments <= aCacheLine.iAllocatedSegments,	\
		Fault(EInvalidCacheLine));

#else
	#define __CACHELINE_INVARIANT(aCacheLine)
#endif

const TInt KSegmentSize = 4096;
const TInt KSegmentSizeLog2 = 12;
const TInt64 KSegmentSizeMask = MAKE_TINT64(0x7FFFFFFF,0xFFFFF000);

const TInt KCacheLineSizeLog2 = 17;	// 128K
const TInt KCacheLineSize = (1 << KCacheLineSizeLog2 );
const TInt KCacheLineSizeInSegments = (KCacheLineSize >> KSegmentSizeLog2);

CCacheManager* CCacheManagerFactory::iCacheManager = NULL;


LOCAL_C void Fault(TCacheManagerFault aFault)
//
// Report a fault in the cache manager
//
	{
	User::Panic(_L("FSCACHEMAN"), aFault);
	}


/*
Indicates if a number passed in is a power of two

@param aNum number to be tested
@return Flag to indicate the result of the test
*/
LOCAL_C TBool IsPowerOfTwo(TInt aNum)
	{
	return (aNum != 0 && (aNum & -aNum) == aNum);
	}


//********************
// CCacheManagerFactory
//********************
void CCacheManagerFactory::CreateL()
	{
	__ASSERT_ALWAYS(iCacheManager == NULL, Fault(ECacheAlreadyCreated));

	if (TGlobalFileCacheSettings::Enabled())
		{
		iCacheManager = CCacheManager::NewCacheL(TGlobalFileCacheSettings::CacheSize());
		}
	else
		{
		Destroy(); 
		}
	}

CCacheManager* CCacheManagerFactory::CacheManager()
	{
	return iCacheManager;
	}

TInt CCacheManagerFactory::Destroy()
	{
	delete iCacheManager;
	iCacheManager = NULL;
	return KErrNone;
	}

//********************
// CCacheManager
//********************
CCacheManager* CCacheManager::NewCacheL(TInt aCacheSize)
	{
	CCacheManager* cacheManager = new (ELeave) CCacheManager(aCacheSize);

	CleanupStack::PushL(cacheManager);
	cacheManager->ConstructL();
	CleanupStack::Pop(1, cacheManager);

	return cacheManager;
	}

CCacheManager::~CCacheManager()
	{
	CacheLock();

	iFreeQueue.Close();
	iUsedQueue.Close();

	delete [] iCacheLines;

	CacheUnlock();
	iLock.Close();

	iChunk.Close();
	}


CCacheManager::CCacheManager(TUint aCacheSize)
	{
	// Get the page size
	TInt pageSize;
	TInt r = HAL::Get(HAL::EMemoryPageSize, pageSize);
	if (r != KErrNone)
		pageSize = KSegmentSize;
	__ASSERT_ALWAYS(IsPowerOfTwo(pageSize), Fault(EIllegalPageSize));

	// For the time being we only a support page size of 4K and we assume
	// that page size = segment size, since the extra overhead of supporting 
	// variable page sizes now is non-trivial.
	//
	// If a processor with a different page size needs to be supported 
	// in the future, then we need to either rework this code to be able to
	// devide up pages into smaller segments or analyze the impact of having 
	// a different page size on performance.
	__ASSERT_ALWAYS(pageSize == KSegmentSize, Fault(EIllegalPageSize));

	iCacheLineSize = KCacheLineSize;
	iCacheLineSizeLog2 = KCacheLineSizeLog2;

	iCacheSize = aCacheSize;

	iSegmentsPerCacheLine = 1 << (iCacheLineSizeLog2 - KSegmentSizeLog2);

	iMaxLockedSegments = TGlobalFileCacheSettings::MaxLockedSize() >> KSegmentSizeLog2;

#ifdef __SIMULATE_LOCK_FAILURES__
	iSimulateLockFailureMode = EFalse;	// off by default unless switched on by KControlIoSimulateLockFailureMode
#endif
	}

void CCacheManager::ConstructL()
	{

	// calculate the low-memory threshold below which we fail any attempt to allocate memory
	TMemoryInfoV1Buf meminfo;
	TInt r = UserHal::MemoryInfo(meminfo);
	__ASSERT_ALWAYS(r==KErrNone,Fault(EMemoryInfoFailed));

	iLowMemoryThreshold = (meminfo().iTotalRamInBytes  / 100) * TGlobalFileCacheSettings::LowMemoryThreshold();
	__CACHE_PRINT4(_L("CACHEMAN: totalRAM %d freeRAM %d KDefaultLowMemoryThresholdPercent %d iLowMemoryThreshold %d"), 
		meminfo().iTotalRamInBytes, meminfo().iFreeRamInBytes, KDefaultLowMemoryThreshold, iLowMemoryThreshold);
	__CACHE_PRINT1(_L("CACHEMAN: iCacheSize %d"), iCacheSize);
	TChunkCreateInfo createInfo;
	createInfo.SetCache(iCacheSize);
	createInfo.SetOwner(EOwnerProcess);
	r = iChunk.Create(createInfo);
	User::LeaveIfError(r);

#ifdef	OST_TRACE_COMPILER_IN_USE
	TName chunkName = iChunk.Name();
	OstTraceData(TRACE_FILECACHE_MANAGER, FILECACHEMAN_CHUNK_NAME, "Chunk name %S", chunkName.Ptr(), chunkName.Length()<<1);
	OstTraceExt5(TRACE_FILECACHE_MANAGER, FILECACHEMAN_CHUNK_PARM, 
		"totalRAM %d freeRAM %d KDefaultLowMemoryThresholdPercent %d iLowMemoryThreshold %d iCacheSize %d", 
		meminfo().iTotalRamInBytes, meminfo().iFreeRamInBytes, KDefaultLowMemoryThreshold, iLowMemoryThreshold, iCacheSize);
#endif	// OST_TRACE_COMPILER_IN_USE
	
	TInt mm = UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, 0, 0) & EMemModelTypeMask;
	if (mm < EMemModelTypeFlexible)
		{
		// On memory models before flexible, File System has to register chunks that will be DMA-ed.
		// On flexible memory model, local media is using new physical pinning Kernel interface that
		// doesn't require registration of the chunk.
		UserSvr::RegisterTrustedChunk(iChunk.Handle());
		}
	

	__ASSERT_ALWAYS(iCacheSize > KSegmentSize, Fault(EIllegalCacheSize));

	r = iLock.CreateLocal();
	User::LeaveIfError(r);

	iNumOfCacheLines = iCacheSize >> iCacheLineSizeLog2;
	iBase = Base();

	iCacheLines = new (ELeave) TCacheLine[iNumOfCacheLines];

	for (TInt n=0; n<iNumOfCacheLines; n++)
		{
		// coverity[var_decl]
		TCacheLine cacheLine;
		cacheLine.iAddr = CACHE_NUM_TO_ADDR(n);
		cacheLine.iAllocatedSegments = 0;
		cacheLine.iFilledSegments = 0;
		cacheLine.iDirtySegments = 0;
		cacheLine.iLockCount = 0;
		cacheLine.iLockedSegmentStart = 0;
		cacheLine.iLockedSegmentCount = 0;
		cacheLine.iClient = NULL;
		// coverity[uninit_use]
		iCacheLines[n] = cacheLine;

		r = iFreeQueue.Append(&iCacheLines[n]);
		User::LeaveIfError(r);
		}

	}

CCacheClient* CCacheManager::CreateClientL()
	{
	CCacheClient* client = CCacheClient::NewL(*this);
	return client;
	}

void CCacheManager::RegisterClient(CCacheClient& /*aClient*/)
	{
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	iStats.iFileCount++;
#endif
	}

void CCacheManager::DeregisterClient(CCacheClient& /*aClient*/)
	{
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	iStats.iFileCount--;
#endif
	}


void CCacheManager::FreeMemoryChanged(TBool aMemoryLow)
	{
	iMemoryLow = aMemoryLow;
	}

TBool CCacheManager::TooManyLockedSegments()
	{
	return (iLockedSegmentCount >= iMaxLockedSegments)?(TBool)ETrue:(TBool)EFalse;
	}

TInt CCacheManager::SegmentSize()
	{
	return KSegmentSize;
	}

TInt CCacheManager::SegmentSizeLog2()
	{
	return KSegmentSizeLog2;
	}

TInt64 CCacheManager::SegmentSizeMask()
	{
	return KSegmentSizeMask;
	}

TInt CCacheManager::CacheLineSize()
	{
	return KCacheLineSize;
	}

TInt CCacheManager::CacheLineSizeInSegments()
	{
	return KCacheLineSizeInSegments;
	}

TInt CCacheManager::CacheLineSizeLog2()
	{
	return KCacheLineSizeLog2;
	}


#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
TFileCacheStats& CCacheManager::Stats()
	{
	iStats.iFreeCount = iFreeQueue.Count();
	iStats.iUsedCount = iUsedQueue.Count();
	iStats.iLockedSegmentCount = iLockedSegmentCount;
	iStats.iAllocatedSegmentCount = iAllocatedSegmentCount;
	iStats.iFilesOnClosedQueue = TClosedFileUtils::Count();
	__ASSERT_DEBUG(iStats.iFreeCount >= 0, Fault(EInvalidStats));
	__ASSERT_DEBUG(iStats.iUsedCount >= 0, Fault(EInvalidStats));
	__ASSERT_DEBUG(iStats.iLockedSegmentCount >= 0, Fault(EInvalidStats));
	__ASSERT_DEBUG(iStats.iFilesOnClosedQueue >= 0, Fault(EInvalidStats));
	return iStats;
	}
#endif	// #if defined(_DEBUG) || defined(_DEBUG_RELEASE)



void CCacheManager::CacheLock()
	{
	iLock.Wait();

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	iManagerLocked = ETrue;
#endif
	}

void CCacheManager::CacheUnlock()
	{
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	iManagerLocked = EFalse;
#endif
	iLock.Signal();
	}


TInt CCacheManager::AllocateAndLockCacheLine(CCacheClient* aClient, TInt64 aPos, const TCacheLine*& aCacheLine, TInt aSegmentCount)
	{
	__ASSERT_DEBUG(aSegmentCount <= iSegmentsPerCacheLine, Fault(EInvalidAllocCount));

	// check for low system memory
	TMemoryInfoV1Buf meminfo;
	TInt r = UserHal::MemoryInfo(meminfo);
	__ASSERT_ALWAYS(r==KErrNone,Fault(EMemoryInfoFailed));
	if (iMemoryLow || (meminfo().iFreeRamInBytes < iLowMemoryThreshold))
		{
		__CACHE_PRINT(_L("CACHEMAN: free RAM below threshold !!!"));
		OstTrace0(TRACE_FILECACHE_MANAGER, FILECACHEMAN_LOW_MEM, "Free RAM below threshold");
		return KErrNoMemory;
		}

	CacheLock();

	__CACHE_PRINT1(_L("CACHEMAN: Allocate %d segments, Lock %d"), aSegmentCount);
	__CACHE_PRINT2(_L("CACHEMAN: iFreeQueue %d iUsedQueue %d"), iFreeQueue.Count(), iUsedQueue.Count());

#ifdef  __SIMULATE_LOCK_FAILURES__
	if (SimulatedFailure(iAllocFailureCount))
		{
		__CACHE_PRINT(_L("CACHEMAN: simulating allocation failure"));
		OstTrace0(TRACE_FILECACHE_MANAGER, FILECACHEMAN_SIMULATING_ALLOC_FAILURE, "Simulating alloc failure");

		CacheUnlock();
		return KErrNoMemory;
		}
#endif

	// any cachelines free ?
	TInt freeCacheLines = iFreeQueue.Count();
	TCacheLine* cacheLine = NULL;
	if (freeCacheLines == 0 && !StealCacheLine(aClient))
		{
		CacheUnlock();
		return KErrNotFound;
		}

	cacheLine = iFreeQueue[0];

	__CACHELINE_INVARIANT((*cacheLine));
	__ASSERT_DEBUG( cacheLine->iAllocatedSegments == 0, Fault(EInvalidAllocCount));
	__ASSERT_DEBUG( cacheLine->iFilledSegments == 0, Fault(EInvalidFillCount));
	__ASSERT_DEBUG( cacheLine->iDirtySegments == 0, Fault(EInvalidFillCount));
	__ASSERT_DEBUG( cacheLine->iLockCount == 0, Fault(EInvalidLockCount));
	__ASSERT_DEBUG( cacheLine->iLockedSegmentStart == 0, Fault(EInvalidLockedPageStart));
	__ASSERT_DEBUG( cacheLine->iLockedSegmentCount == 0, Fault(EInvalidLockedPageCount));
	__ASSERT_DEBUG( cacheLine->iClient == NULL, Fault(EInvalidClient));

	TUint8* addr = cacheLine->iAddr;

	// Commit it
	r = Commit(addr, aSegmentCount);
	if (r != KErrNone)
		{
		CacheUnlock();
		return r;
		}
	cacheLine->iAllocatedSegments = (TUint8) (aSegmentCount);
	cacheLine->iLockedSegmentStart = 0;
	cacheLine->iLockedSegmentCount = (TUint8) aSegmentCount;

	// Add to used queue
	r = iUsedQueue.InsertInAddressOrder(cacheLine);
	if (r != KErrNone)
		{
		Decommit(addr, aSegmentCount);
		CacheUnlock();
		return r;
		}

	cacheLine->iClient = aClient;
	cacheLine->iPos = aPos;

	// Remove from free queue
	iFreeQueue.Remove(0);

	OstTraceExt2(TRACE_FILECACHE_MANAGER, FILECACHEMAN_ALLOC_CACHELINE, "cachelines: free %d used %d", iFreeQueue.Count(), iUsedQueue.Count());

	// RChunk will lock segments initially unless explicitly unlocked

	cacheLine->iLockCount++;
	
	__CACHE_PRINT2(_L("CACHEMAN: LockCount for %08X is %d"), cacheLine->iAddr, cacheLine->iLockCount);

	CacheUnlock();

	aCacheLine = cacheLine;
	return r;

	}

/**
CCacheManager::ExtendCacheLine()

Attempts to extend the length of an existing cacheline by committing extra segments
The cacheline must be owned and locked already.

@param aCacheLine A reference to a locked, owned cacheline
@param aSegmentCount The new length of the cacheline (including the existing segments)
					 This must be greater then the existing length.

@return KErrNone if successful
		or other system wide error code.
*/
TInt CCacheManager::ExtendCacheLine(CCacheClient* aClient, const TCacheLine& aCacheLine, TInt aSegmentCount)
	{
	CacheLock();

	__ASSERT_DEBUG(aSegmentCount > 0, Fault(EInvalidSegmentCount));

	TCacheLine& cacheLine = const_cast<TCacheLine&>(aCacheLine);
	__CACHELINE_INVARIANT((cacheLine));
	__ASSERT_DEBUG(aSegmentCount <= iSegmentsPerCacheLine, Fault(EInvalidSegmentCount));
	__ASSERT_DEBUG(aSegmentCount > cacheLine.iAllocatedSegments, Fault(EInvalidSegmentCount));
	__ASSERT_DEBUG(cacheLine.iLockCount > 0, Fault(EInvalidLockCount));	// must be locked already
	__ASSERT_ALWAYS(cacheLine.iClient == aClient, Fault(EExtendingUnownedCacheline));

	// ensure all pages in cachline are locked
	__ASSERT_DEBUG(cacheLine.iLockedSegmentStart == 0, Fault(EInvalidLockRange));
	__ASSERT_DEBUG(cacheLine.iLockedSegmentCount == cacheLine.iAllocatedSegments, Fault(EInvalidLockRange));

	__CACHE_PRINT2(_L("CACHEMAN: ExtendCacheLine(%08X, %d)"), cacheLine.iAddr, aSegmentCount);

	// Commit the new segments
	TUint8* addrNewSegment = cacheLine.iAddr + (cacheLine.iAllocatedSegments << KSegmentSizeLog2);
	TInt segmentCountNew = aSegmentCount - cacheLine.iAllocatedSegments;

	TInt r = Commit(addrNewSegment, segmentCountNew);
	if (r == KErrNone)
		{
		cacheLine.iAllocatedSegments = cacheLine.iLockedSegmentCount = (TUint8) aSegmentCount;
		}
	

	CacheUnlock();
	return r;
	}

/**
CCacheManager::ReUseCacheLine()

Attempts to lock and then extend or shrink an already owned cacheline, ready for re-use.
If successful the cacheline is returned locked and all segments are marked as empty.
The cacheline must initially be unlocked.

@param aCacheLine A reference to an unlocked cacheline
@param aSegmentCount The new length of the cacheline;
					 this may be greater or less than the existing length.

@return KErrNone if successful
		or other system wide error code.
*/
TInt CCacheManager::ReAllocateAndLockCacheLine(CCacheClient* aClient, TInt64 aPos, const TCacheLine& aCacheLine, TInt aSegmentCount)
	{
	TCacheLine& cacheLine = const_cast<TCacheLine&>(aCacheLine);
	__CACHELINE_INVARIANT((cacheLine));

	__ASSERT_DEBUG(aSegmentCount > 0, Fault(EInvalidSegmentCount));
	__ASSERT_DEBUG(aSegmentCount <= iSegmentsPerCacheLine, Fault(EInvalidSegmentCount));

	__CACHE_PRINT2(_L("CACHEMAN: ReUseCacheLine(%08X, %d)"), cacheLine.iAddr, aSegmentCount);

	TInt r;
	// old cacheline same size or bigger ?
	if (cacheLine.iAllocatedSegments >= aSegmentCount)
		{
		r = LockCacheLine(aClient, aCacheLine, 0, aSegmentCount);
		if (r!= KErrNone)
			return r;
		if (cacheLine.iAllocatedSegments > aSegmentCount)
			{
			cacheLine.iFilledSegments = (TUint8) aSegmentCount;
			RemoveEmptySegments(aClient, aCacheLine);
			}
		}
	// old cacheline smaller
	else 
		{
		r = LockCacheLine(aClient, aCacheLine, 0, cacheLine.iAllocatedSegments);
		if (r != KErrNone)
			return r;
		r = ExtendCacheLine(aClient, aCacheLine, aSegmentCount);
		if (r != KErrNone)
			{
			UnlockCacheLine(aClient, aCacheLine);
			return r;
			}
		}
	cacheLine.iFilledSegments = 0;
	cacheLine.iPos = aPos;

	return KErrNone;
	}


/**
CCacheManager::LockCacheLine()

@return KErrNone on success
*/
TInt CCacheManager::LockCacheLine(
	CCacheClient* aClient, 
	const TCacheLine& aCacheLine,
	TInt aLockedPageStart,
	TInt aLockedPageCount)
	{
	CacheLock();


	TCacheLine& cacheLine = const_cast<TCacheLine&>(aCacheLine);
	__CACHELINE_INVARIANT((cacheLine));
	
	// has the cacheline been stolen ?
	if (cacheLine.iClient != aClient)
		{
		__CACHE_PRINT(_L("CCacheManager::LockCacheLine(), Cacheline Stolen !\n"));
		CacheUnlock();
		return KErrNotFound;
		}
	
	// validate lock range
	__ASSERT_DEBUG(aLockedPageStart >= 0, Fault(EInvalidLockRange));
	__ASSERT_DEBUG(aLockedPageStart + aLockedPageCount <= cacheLine.iAllocatedSegments, Fault(EInvalidLockRange));
	__ASSERT_DEBUG(aLockedPageCount <= iSegmentsPerCacheLine, Fault(EInvalidLockRange));
	// if already locked, don't allow lock range to grow down or up (to simplify code)
	__ASSERT_DEBUG(cacheLine.iLockCount == 0 || 
			aLockedPageStart >= cacheLine.iLockedSegmentStart, 
		Fault(EInvalidLockRange));
	__ASSERT_DEBUG(cacheLine.iLockCount == 0 || 
		aLockedPageStart + aLockedPageCount <= cacheLine.iLockedSegmentStart + cacheLine.iLockedSegmentCount, 
		Fault(EInvalidLockRange));
	
	__CACHE_PRINT1(_L("CACHEMAN: LockCacheLine(%08X, %d)"), cacheLine.iAddr);

	if (InUse(aCacheLine))
		{
		__CACHE_PRINT(_L("CCacheManager::LockCacheLine(), Cacheline in use !\n"));
		CacheUnlock();
		return KErrInUse;
		}

	TInt lockErr = KErrNone;
	
	// increment the lock count
	// if not already locked, lock requested segments
	if (cacheLine.iLockCount++ == 0)
		{
#ifdef __LOCK_ENTIRE_CACHELINE__
		lockErr = Lock(cacheLine.iAddr, cacheLine.iAllocatedSegments);
#else
		__ASSERT_DEBUG(cacheLine.iDirtySegments == 0, Fault(ELockingAndAlreadyDirty));
		lockErr = Lock(	cacheLine.iAddr + (aLockedPageStart<<KSegmentSizeLog2), aLockedPageCount);
#endif

		if (lockErr == KErrNone)
			{
			cacheLine.iLockedSegmentStart = (TUint8) aLockedPageStart;
			cacheLine.iLockedSegmentCount = (TUint8) aLockedPageCount;
			}
		else	// if (lockErr != KErrNone)
			{
			__CACHE_PRINT2(_L("CACHEMAN: LockCacheLine(%08X) failure %d"), cacheLine.iAddr, lockErr);
			cacheLine.iLockCount--;
			// NB a lock failure implies segment is decomitted
			FreeCacheLine(cacheLine);
			}
		}
	// if already locked (because cacheline is dirty) ensure lock range 
	// isn't growing (this isn't allowed to keep the code simpler) 
	else
		{
		__ASSERT_DEBUG(cacheLine.iLockedSegmentStart == 0, Fault(EInvalidLockRange));
		__ASSERT_DEBUG(cacheLine.iLockedSegmentCount >= aLockedPageStart + aLockedPageCount, Fault(EInvalidLockRange));
		}

	CacheUnlock();
	return lockErr;
	}

/**
UnlockCacheLine()
*/
TBool CCacheManager::UnlockCacheLine(CCacheClient* aClient, const TCacheLine& aCacheLine)
	{
	CacheLock();

	TCacheLine& cacheLine = const_cast<TCacheLine&>(aCacheLine);
	__CACHELINE_INVARIANT((cacheLine));
	
	__ASSERT_DEBUG(cacheLine.iLockCount > 0, Fault(EInvalidLockCount));
	__ASSERT_ALWAYS(cacheLine.iClient == aClient, Fault(EUnlockingUnownedCacheline));

	
	__CACHE_PRINT2(_L("CACHEMAN: UnlockCacheLine(%08X, %d)"), cacheLine.iAddr, cacheLine.iAllocatedSegments);


	// decrement the lock count
	TBool success = ETrue;

	if (cacheLine.iLockCount > 1)
		{
		cacheLine.iLockCount--;
		}
	else
		{
		if (cacheLine.iDirtySegments == 0)
			{
			cacheLine.iLockCount--;
#ifdef __LOCK_ENTIRE_CACHELINE__
			Unlock(cacheLine.iAddr, cacheLine.iAllocatedSegments);
#else
			Unlock(
				cacheLine.iAddr + (cacheLine.iLockedSegmentStart<<KSegmentSizeLog2), 
				cacheLine.iLockedSegmentCount);
#endif

			cacheLine.iLockedSegmentStart = cacheLine.iLockedSegmentCount = 0;
			}
		else
			{
			__CACHE_PRINT(_L("CACHEMAN: UnlockCacheLine - not unlocking segment dirty"));
			success = EFalse;
			}
		}

	CacheUnlock();

	return success;
	}



TBool CCacheManager::StealCacheLine(CCacheClient* aClient)
	{
	__ASSERT_DEBUG(iManagerLocked, Fault(EManagerNotLocked));

	TInt usedQueueSize = iUsedQueue.Count();

	#define INC_INDEX(x) (x++, x = (x >= usedQueueSize ? 0 : x))

	__CACHE_PRINT2(_L("CACHEMAN: StealCacheLine, iNextCacheLineToSteal %d used count %d"), iNextCacheLineToSteal, iUsedQueue.Count());

	TInt iInitIndex = iNextCacheLineToSteal;

	// Loop through all used cachelines, starting at the last stolen 
	// cacheline index + 1 and ending when we find a suitable cacheline 
	// to steal or we have looped back to the start
	for (INC_INDEX(iNextCacheLineToSteal);
		iNextCacheLineToSteal != iInitIndex; 
		INC_INDEX(iNextCacheLineToSteal))
		{
		TCacheLine& cacheLine = *iUsedQueue[iNextCacheLineToSteal];
		if (cacheLine.iLockCount == 0 && cacheLine.iClient != aClient)
			{
			__CACHE_PRINT3(_L("CACHEMAN: StealCacheLine, stealing %d from %08X to %08X"), 
				  iNextCacheLineToSteal, cacheLine.iClient, aClient);
			FreeCacheLine(cacheLine);
			return ETrue;
			}
		}
	return EFalse;
	}


TBool CCacheManager::FreeCacheLine(CCacheClient* aClient, const TCacheLine& aCacheLine)
	{
	CacheLock();

	TCacheLine& cacheLine = const_cast<TCacheLine&>(aCacheLine);
	__CACHELINE_INVARIANT((cacheLine));

	// Has the cacheline been stolen ? (Assume success if it has)
	if (cacheLine.iClient != aClient)
		{
		__CACHE_PRINT(_L("CCacheManager::FreeCacheLine(), Cacheline Stolen !!!!\n"));
		CacheUnlock();
		return ETrue;		
		}
	
	// Is the cacheline locked ?
	if (cacheLine.iLockCount > 0)
		{
		__CACHE_PRINT(_L("CCacheManager::FreeCacheLine(), attempt to free locked cacheline\n"));
		CacheUnlock();
		return EFalse;
		}

	FreeCacheLine(cacheLine);

	CacheUnlock();
	return ETrue;
	}

TInt CCacheManager::LockCount(CCacheClient* aClient, const TCacheLine& aCacheLine)
	{
	TCacheLine& cacheLine = const_cast<TCacheLine&>(aCacheLine);
	__CACHELINE_INVARIANT((cacheLine));
	
	// cacheline stolen ?
	if (cacheLine.iClient != aClient)
		return 0;

	return cacheLine.iLockCount;
	}

TInt CCacheManager::FillCount(CCacheClient* aClient, const TCacheLine& aCacheLine)
	{
	TCacheLine& cacheLine = const_cast<TCacheLine&>(aCacheLine);
	__CACHELINE_INVARIANT((cacheLine));
	
	// cacheline stolen ?
	if (cacheLine.iClient != aClient)
		return 0;

	return cacheLine.iFilledSegments;
	}

TInt CCacheManager::DirtyCount(CCacheClient* aClient, const TCacheLine& aCacheLine)
	{
	TCacheLine& cacheLine = const_cast<TCacheLine&>(aCacheLine);
	__CACHELINE_INVARIANT((cacheLine));

	// cacheline stolen ?
	if (cacheLine.iClient != aClient)
		return 0;

	return cacheLine.iDirtySegments;
	}

TBool CCacheManager::InUse(const TCacheLine& aCacheLine)
	{
	TCacheLine& cacheLine = const_cast<TCacheLine&>(aCacheLine);
	__CACHELINE_INVARIANT((cacheLine));

	// busy if lock count >= 1 and there are no dirty segments
	// or   if lock count >= 2 and there are dirty segments
	return (cacheLine.iLockCount - 
		   (cacheLine.iDirtySegments?1:0)) > 0 ? (TBool)ETrue : (TBool)EFalse;
	}

void CCacheManager::SetFilled(CCacheClient* aClient, const TCacheLine& aCacheLine,  TInt aSegmentCount)
	{
	TCacheLine& cacheLine = const_cast<TCacheLine&>(aCacheLine);
	__CACHELINE_INVARIANT((cacheLine));
	__ASSERT_DEBUG(aSegmentCount <= iSegmentsPerCacheLine, Fault(EInvalidSegmentCount));
	__ASSERT_DEBUG(aSegmentCount <= cacheLine.iAllocatedSegments , Fault(EInvalidDirtyCount));
	__ASSERT_DEBUG(cacheLine.iLockCount > 0, Fault(ESetFilledNotLocked));
	__ASSERT_ALWAYS(cacheLine.iClient == aClient, Fault(EExtendingUnownedCacheline));

	
	cacheLine.iFilledSegments = Max(cacheLine.iFilledSegments, (TUint8) aSegmentCount);

	__ASSERT_DEBUG(cacheLine.iFilledSegments >= cacheLine.iDirtySegments , Fault(EInvalidDirtyCount));
	}

void CCacheManager::SetDirty(CCacheClient* aClient, const TCacheLine& aCacheLine,  TInt aSegmentCount)
	{
	TCacheLine& cacheLine = const_cast<TCacheLine&>(aCacheLine);
	__CACHELINE_INVARIANT((cacheLine));
	__ASSERT_DEBUG(aSegmentCount <= iSegmentsPerCacheLine, Fault(EInvalidSegmentCount));
	__ASSERT_DEBUG(aSegmentCount <= cacheLine.iAllocatedSegments , Fault(EInvalidDirtyCount));
	__ASSERT_DEBUG(cacheLine.iLockCount > 0, Fault(ESetDirtyNotLocked));
	__ASSERT_ALWAYS(cacheLine.iClient == aClient, Fault(EExtendingUnownedCacheline));

	// ensure that lock range is valid - we insist on dirty
	// cachelines having all dirty pages locked up to the the last dirty page
	__ASSERT_DEBUG(cacheLine.iLockedSegmentStart == 0, Fault(ESetDirtyInvalidLockRange));
	__ASSERT_DEBUG(cacheLine.iLockedSegmentCount >= aSegmentCount, Fault(ESetDirtyInvalidLockRange));
	
	cacheLine.iFilledSegments = Max(cacheLine.iFilledSegments, (TUint8) aSegmentCount);
	cacheLine.iDirtySegments = Max(cacheLine.iDirtySegments, (TUint8) aSegmentCount);

	__ASSERT_DEBUG(cacheLine.iFilledSegments >= cacheLine.iDirtySegments , Fault(EInvalidDirtyCount));
	}

void CCacheManager::ClearDirty(CCacheClient* aClient, const TCacheLine& aCacheLine)
	{
	TCacheLine& cacheLine = const_cast<TCacheLine&>(aCacheLine);
	__CACHELINE_INVARIANT((cacheLine));
	__ASSERT_DEBUG(cacheLine.iLockCount > 0, Fault(EClearDirtyNotLocked));
	__ASSERT_ALWAYS(cacheLine.iClient == aClient, Fault(EExtendingUnownedCacheline));

	TInt dirtySegments = cacheLine.iDirtySegments;
	cacheLine.iDirtySegments = 0;
	SetFilled(aClient, cacheLine, dirtySegments);
	UnlockCacheLine(aClient, cacheLine);
	}


void CCacheManager::RemoveEmptySegments(CCacheClient* aClient, const TCacheLine& aCacheLine)
	{
	CacheLock();

	TCacheLine& cacheLine = const_cast<TCacheLine&>(aCacheLine);
	__CACHELINE_INVARIANT((cacheLine));

	// has the cacheline been stolen ?
	if (cacheLine.iClient != aClient)
		{
		__CACHE_PRINT(_L("CCacheManager::RemoveEmptySegments((), Cacheline Stolen ! !!!\n"));
		CacheUnlock();
		return;
		}
		
	__ASSERT_DEBUG(cacheLine.iLockCount > 0, Fault(ERemovingEmptyUnlocked));

	// Unlock any locked segments past the last filled segment
	TInt filledSegmentCount = cacheLine.iFilledSegments;
	TInt firstSegmentToUnlock;
	TInt segmentsToUnlock;
#ifdef __LOCK_ENTIRE_CACHELINE__
	firstSegmentToUnlock = filledSegmentCount;
	segmentsToUnlock = cacheLine.iAllocatedSegments - filledSegmentCount;
#else
	TInt firstLockedSegment = cacheLine.iLockedSegmentStart;
	if (firstLockedSegment <= filledSegmentCount)
		{
		firstSegmentToUnlock = filledSegmentCount;
		segmentsToUnlock = firstLockedSegment + cacheLine.iLockedSegmentCount - filledSegmentCount;
		}
	else
		{
		firstSegmentToUnlock = firstLockedSegment;
		segmentsToUnlock = cacheLine.iLockedSegmentCount;
		}
#endif
	TUint8* addrFirstSegmentToUnlock = 
		cacheLine.iAddr + (firstSegmentToUnlock << KSegmentSizeLog2);
	if (segmentsToUnlock > 0)
		{
		Unlock(addrFirstSegmentToUnlock, segmentsToUnlock);
		cacheLine.iLockedSegmentCount = 
			(TUint8) (cacheLine.iLockedSegmentCount - segmentsToUnlock);
		}

	// Decommit any segments past the last filled segment
	Decommit(
		cacheLine.iAddr + (filledSegmentCount << KSegmentSizeLog2), 
		cacheLine.iAllocatedSegments - filledSegmentCount);
	cacheLine.iAllocatedSegments = (TUint8) filledSegmentCount;

	CacheUnlock();
	}


void CCacheManager::FreeCacheLine(TCacheLine& aCacheLine)
	{
	__ASSERT_DEBUG(iManagerLocked, Fault(EManagerNotLocked));
	aCacheLine.iClient = NULL;
	__ASSERT_ALWAYS( (aCacheLine.iLockCount == 0), Fault(EFreeingLockedCacheLine));
	__ASSERT_ALWAYS( (aCacheLine.iDirtySegments == 0), Fault(EFreeingDirtyCacheLine));

	Decommit(aCacheLine.iAddr, aCacheLine.iAllocatedSegments);
	aCacheLine.iAllocatedSegments = 0;
	aCacheLine.iFilledSegments = 0;
	aCacheLine.iLockedSegmentStart = 0;
	aCacheLine.iLockedSegmentCount = 0;

	// Remove from used queue
	TInt index = iUsedQueue.FindInAddressOrder(&aCacheLine);
	__ASSERT_ALWAYS(index != KErrNotFound, Fault(ESegmentNotFound));
	iUsedQueue.Remove(index);

	// put back on free queue
//	TInt r = iFreeQueue.Append(&aCacheLine);
	TInt r = iFreeQueue.InsertInAddressOrder(&aCacheLine);
	__ASSERT_ALWAYS(r == KErrNone, Fault(EAppendToFreeQueueFailed));

	__CACHE_PRINT2(_L("CACHEMAN: FreeCacheLine, iFreeQueue %d iUsedQueue %d"), iFreeQueue.Count(), iUsedQueue.Count());
	OstTraceExt2(TRACE_FILECACHE_MANAGER, FILECACHEMAN_FREE_CACHELINE, "cachelines: free %d used %d", iFreeQueue.Count(), iUsedQueue.Count());
	}


#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
void CCacheManager::DumpCacheLine(TCacheLine& aCacheLine)
	{
	RDebug::Print(_L("CACHEMAN: Cacheline client %08X addr %08X pos %d alloc %d filled %d dirty %d lock %d \tData: %02X %02X %02X %02X %02X %02X %02X %02X "), 
		aCacheLine.iClient,
		aCacheLine.iAddr,
		I64LOW(aCacheLine.iPos),

		aCacheLine.iAllocatedSegments,
		aCacheLine.iFilledSegments,
		aCacheLine.iDirtySegments,
		aCacheLine.iLockCount,

		aCacheLine.iAddr[0], 
		aCacheLine.iAddr[1], 
		aCacheLine.iAddr[2], 
		aCacheLine.iAddr[3], 
		aCacheLine.iAddr[4], 
		aCacheLine.iAddr[5], 
		aCacheLine.iAddr[6], 
		aCacheLine.iAddr[7]
		);
	}

void CCacheManager::DumpCache()
	{
	CacheLock();
	
	RPointerArray<CCacheClient> clients;

	RDebug::Print(_L("**** CACHEMAN: CacheLines ****\n"));
	TInt usedQueueSize = iUsedQueue.Count();
	TInt n;
	for (n=0; n<usedQueueSize; n++)
		{
		TCacheLine& cacheLine = *iUsedQueue[n];
		DumpCacheLine(cacheLine);

		clients.InsertInAddressOrder(cacheLine.iClient);
		}

	TInt clientCount = clients.Count();

	for (n=0; n<clientCount; n++)
		{
		RDebug::Print(_L("**** CACHEMAN: CacheClient #%d ****\n"), n);
		clients[n]->DumpCache();
		}

	clients.Close();

	CacheUnlock();
	}
#endif




TUint8* CCacheManager::Base()
	{
	return iChunk.Base();
	}


TInt CCacheManager::Lock(TUint8* const aAddr, TInt aSegmentCount)
	{
	TInt r = iChunk.Lock(aAddr-iBase, aSegmentCount << KSegmentSizeLog2);
//RDebug::Print(_L("RChunk::Lock(%08X, %d) %d"), aAddr-iBase, aSegmentCount << KSegmentSizeLog2, r);
	__ASSERT_DEBUG(r == KErrNone || r == KErrNoMemory || r == KErrNotFound, Fault(EUnexpectedLockFailure));

	__CACHE_PRINT3(_L("CACHEMAN: LOCK %08X %d %d"), aAddr, aSegmentCount, r);

#ifdef  __SIMULATE_LOCK_FAILURES__
	if (SimulatedFailure(iLockFailureCount))
		{
		__CACHE_PRINT(_L("CACHEMAN: simulating lock failure"));
		OstTrace0(TRACE_FILECACHE_MANAGER, FILECACHEMAN_SIMULATING_LOCK_FAILURE, "Simulating lock failure");

		r = KErrNotFound;
		}
#endif

	if (r == KErrNone)
		{
		iLockedSegmentCount+= aSegmentCount;

		OstTrace1(TRACE_FILECACHE_MANAGER, FILECACHEMAN_LOCK_SEGMENTS, "Locked segments %d", iLockedSegmentCount);
		}
	else
		{
		__CACHE_PRINT(_L("CACHEMAN: LOCK FAILED"));
		OstTrace1(TRACE_FILECACHE_MANAGER, FILECACHEMAN_LOCK_FAILURE, "Failure r %d", r);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		iStats.iLockFailureCount++;
#endif
		}

	return r;
	}

TInt CCacheManager::Unlock(TUint8* const aAddr, TInt aSegmentCount)
	{
	TInt r = iChunk.Unlock(aAddr-iBase, aSegmentCount << KSegmentSizeLog2);
//RDebug::Print(_L("RChunk::Unlock(%08X, %d) %d"), aAddr-iBase, aSegmentCount << KSegmentSizeLog2, r);

	__CACHE_PRINT3(_L("CACHEMAN: UNLOCK %08X %d %d"), aAddr, aSegmentCount, r);
	if (r == KErrNone)
		{
		iLockedSegmentCount-= aSegmentCount;

		OstTrace1(TRACE_FILECACHE_MANAGER, FILECACHEMAN_UNLOCK_SEGMENTS, "Locked segments %d", iLockedSegmentCount);
		}
	else
		{
		__CACHE_PRINT(_L("CACHEMAN: UNLOCK FAILED"));
		}

	return r;
	}

TInt CCacheManager::Commit(TUint8* const aAddr, TInt aSegmentCount)
	{
#ifdef  __SIMULATE_LOCK_FAILURES__
	if (SimulatedFailure(iCommitFailureCount))
		{
		__CACHE_PRINT(_L("CACHEMAN: simulating commit failure "));
		OstTrace0(TRACE_FILECACHE_MANAGER, FILECACHEMAN_SIMULATING_COMMIT_FAILURE, "Simulating commit failure");

		return KErrNoMemory;
		}
#endif

	TInt r = iChunk.Commit(aAddr-iBase, aSegmentCount << KSegmentSizeLog2);
//RDebug::Print(_L("RChunk::Commit(%08X, %d) %d, "), aAddr-iBase, aSegmentCount << KSegmentSizeLog2, r);
	
	__CACHE_PRINT3(_L("CACHEMAN: COMMIT: %08X %d %d, "), aAddr, aSegmentCount, r);
	if (r == KErrNone)
		{
		iLockedSegmentCount+= aSegmentCount;
		iAllocatedSegmentCount+= aSegmentCount;

		OstTrace1(TRACE_FILECACHE_MANAGER, FILECACHEMAN_COMMIT_SEGMENTS, "Allocated segments %d", iAllocatedSegmentCount);
		}
	else
		{
		__CACHE_PRINT(_L("CACHEMAN: COMMIT FAILED"));
		OstTrace1(TRACE_FILECACHE_MANAGER, FILECACHEMAN_COMMIT_FAILURE, "Failure r %d", r);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		iStats.iCommitFailureCount++;
#endif
		}


	__ASSERT_DEBUG(r == KErrNone || r == KErrNoMemory, Fault(EUnexpectedCommitFailure));

	return r;
	}

TInt CCacheManager::Decommit(TUint8* const aAddr, TInt aSegmentCount)
	{
	TInt r = iChunk.Decommit(aAddr-iBase, aSegmentCount << KSegmentSizeLog2);
//RDebug::Print(_L("RChunk::Decommit(%08X, %d), %d"), aAddr-iBase, aSegmentCount << KSegmentSizeLog2, r);

	__CACHE_PRINT3(_L("CACHEMAN: DECOMMIT: %08X %d %d, "), aAddr, aSegmentCount, r);
	if (r == KErrNone)
		{
		iAllocatedSegmentCount-= aSegmentCount;

		OstTrace1(TRACE_FILECACHE_MANAGER, FILECACHEMAN_DECOMMIT_SEGMENTS, "Allocated segments %d", iAllocatedSegmentCount);
		}
	else
		{
		__CACHE_PRINT(_L("CACHEMAN: DECOMMIT FAILED"));
		}

	return r;
	}

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)

void CCacheManager::SimulateLockFailureMode(TBool aEnable)
	{
	iSimulateLockFailureMode = aEnable;

	__CACHE_PRINT1(_L("CACHEMAN: SimulateLockFailureMode: %d, "), iSimulateLockFailureMode);
	OstTrace1(TRACE_FILECACHE_MANAGER, FILECACHEMAN_SIMULATE_LOCK_FAILURE_MODE, "aEnable %d", aEnable);
	}

void CCacheManager::AllocateMaxSegments(TBool aEnable)
	{
	iAllocateMaxSegments = aEnable;
	__CACHE_PRINT1(_L("CACHEMAN: iAllocateMaxSegments: %d, "), iAllocateMaxSegments);
	}

TBool CCacheManager::AllocateMaxSegments()
	{
	return iAllocateMaxSegments;
	}

void CCacheManager::SimulateWriteFailure()
	{
	iSimulateWriteFailure = ETrue;
	}

TBool CCacheManager::SimulateWriteFailureEnabled()
	{
	TBool b = iSimulateWriteFailure;
	iSimulateWriteFailure = EFalse;
	return b;
	}

TBool CCacheManager::SimulatedFailure(TInt& aFailureCount)
	{
#ifdef  __SIMULATE_LOCK_FAILURES__
	if (iSimulateLockFailureMode)
		{
#if defined  (__RAMDOM_LOCK_FAILURES__)
		static TInt FailCount = 15;
		if (++aFailureCount >= FailCount)
#elif defined (__PSEUDO_RANDOM_FAILURES__)
		const TInt FailCounts[] = {15,2,0,21,1,12,24};
		const TInt FailCountSize = sizeof(FailCounts) / sizeof(TInt);
		static TInt FailCountIndex = 0; 
		if (++aFailureCount >= FailCounts[FailCountIndex])
#else
		const TInt KFailCount = 15;
		if (++aFailureCount >= KFailCount)
#endif
			{
			aFailureCount = 0;
#if defined (__RAMDOM_LOCK_FAILURES__)
			FailCount = Math::Random() & 0x1F;
			__CACHE_PRINT1(_L("FailCount %d"), FailCount);
#endif

#if defined (__PSEUDO_RANDOM_FAILURES__)
			FailCountIndex = (FailCountIndex +1) % FailCountSize ;
			__CACHE_PRINT1(_L("FailCount %d"), FailCounts[FailCountIndex]);
#endif

			return ETrue;
			}
		}
#endif
	return EFalse;
	}
#endif	// defined(_DEBUG) || defined(_DEBUG_RELEASE)

//************************************
// TGlobalFileCacheSettings
//************************************

//TGlobalCacheFlags TGlobalFileCacheSettings::iFlags = TGlobalCacheFlags(ECacheEnabled);
TInt32 TGlobalFileCacheSettings::iCacheSize	= KDefaultGlobalCacheSize << KByteToByteShift;
TInt32 TGlobalFileCacheSettings::iMaxLockedSize	= KDefaultGlobalCacheMaxLockedSize << KByteToByteShift;
TInt32 TGlobalFileCacheSettings::iLowMemoryThreshold = KDefaultLowMemoryThreshold;
TBool TGlobalFileCacheSettings::iEnabled	= KDefaultGlobalCacheEnabled; 

_LIT8(KLitSectionNameFileCache,"FileCache");

void TGlobalFileCacheSettings::ReadPropertiesFile()
	{
	F32Properties::GetBool(KLitSectionNameFileCache, _L8("GlobalCacheEnabled"), iEnabled);
	
	// Get size of cache in kilobytes
	TInt32 cacheSizeInKBytes;
	if (F32Properties::GetInt(KLitSectionNameFileCache, _L8("GlobalCacheSize"), cacheSizeInKBytes))
		iCacheSize = cacheSizeInKBytes << KByteToByteShift;

	// Get maximum amount of locked data i.e data unavailable for paging
	TInt32 maxLockedSize;
	if (F32Properties::GetInt(KLitSectionNameFileCache, _L8("GlobalCacheMaxLockedSize"), maxLockedSize))
		iMaxLockedSize = maxLockedSize << KByteToByteShift;

	// Get low memory threshold
	TInt32 lowMemoryThreshold;
	if (F32Properties::GetInt(KLitSectionNameFileCache, _L8("LowMemoryThreshold"), lowMemoryThreshold))
		iLowMemoryThreshold = lowMemoryThreshold;

	}


TBool TGlobalFileCacheSettings::Enabled()
	{
	return iEnabled;
	}

TInt TGlobalFileCacheSettings::CacheSize()
	{
	return iCacheSize;
	}
TInt TGlobalFileCacheSettings::MaxLockedSize()
	{
	return iMaxLockedSize;
	}

TInt TGlobalFileCacheSettings::LowMemoryThreshold()
	{
	return iLowMemoryThreshold;
	}
