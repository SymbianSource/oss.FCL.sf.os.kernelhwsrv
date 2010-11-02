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
// f32\sfile\sf_cache_man.h
// 
//

/**
 @file
 @internalTechnology
*/

#if !defined(__SF_CACHE_MAN_H__)
#define __SF_CACHE_MAN_H__

#ifdef _DEBUG
#define __SIMULATE_LOCK_FAILURES__
#endif

// forward ref
class CCacheManager;
class CCacheClient;

const TInt KByteToByteShift = 10;

enum TCacheManagerFault
	{
	ENoCacheManager				= 0,
	ECacheAlreadyCreated		= 1,
	EIllegalDriveNumber			= 2,
	EIllegalCacheSize			= 3,			
	EIllegalPageSize			= 4,
	EInvalidAllocCount			= 5,
	EInvalidLockCount			= 6,
	EInvalidFillCount			= 7,
	EAppendToFreeQueueFailed	= 8,
	ESegmentNotFound			= 9,
	EUnlockFailed				= 10,
	EInvalidSegmentCount		= 11,
	EInvalidAddress				= 12,
	EInvalidDirtyCount			= 13,
	EDecommitFailed				= 14,
	EUnexpectedCommitFailure	= 15,
	EUnexpectedLockFailure		= 16,
	EInvalidCacheLine			= 17,
	EInvalidClient				= 18,
	ERemovingEmptyUnlocked		= 19,
	EFreeingLockedCacheLine		= 20,
	EFreeingDirtyCacheLine		= 21,
	ESetDirtyNotLocked			= 22,
	EClearDirtyNotLocked		= 23,
	ESetFilledNotLocked			= 24,
	EManagerNotLocked			= 25,
	EExtendingUnownedCacheline	= 26,
	EUnlockingUnownedCacheline	= 27,
	EInvalidLockedPageStart		= 28,
	EInvalidLockedPageCount		= 29,
	EInvalidLockRange			= 30,
	ESetDirtyInvalidLockRange	= 31,
	ELockingAndAlreadyDirty		= 32,
	EInvalidStats				= 33
	};


class CCacheManagerFactory
	{
public:
	static void CreateL();
	static TInt Destroy();
	static CCacheManager* CacheManager();
private:
	static CCacheManager* iCacheManager;
	};


NONSHARABLE_CLASS(CCacheManager) : public CBase
	{
private:

	class TCacheLine
		{
	public:
		TUint8* iAddr;
		TUint8 iAllocatedSegments;	// number of allocated pages 
		TUint8 iFilledSegments;		// number of full pages (i.e. pages with data in)
		TUint8 iDirtySegments;		// number of dirty pages
		TUint8 iLockCount;			// number of times cacheline has been locked
		TUint8 iLockedSegmentStart;	// zero based index of first locked paged
		TUint8 iLockedSegmentCount;	// number of locked pages
		TUint8 iSpare[2];			// padding
		CCacheClient* iClient;		// owner of this cacheline
		TInt64 iPos;				// arbitrary data owned by client
		};

public:
	CCacheClient* CreateClientL();
	void RegisterClient(CCacheClient& aClient);
	void DeregisterClient(CCacheClient& aClient);


	TInt SegmentSize();
	TInt SegmentSizeLog2();
	TInt64 SegmentSizeMask();
	TInt CacheLineSize();
	TInt CacheLineSizeLog2();
	TInt CacheLineSizeInSegments();

	// called from CCacheClient
	TInt AllocateAndLockCacheLine(CCacheClient* aClient, TInt64 aPos, const TCacheLine*& aCacheLine, TInt aSegmentCount);
	TInt ReAllocateAndLockCacheLine(CCacheClient* aClient, TInt64 aPos, const TCacheLine& aCacheLine, TInt aSegmentCount);

	TInt ExtendCacheLine(CCacheClient* aClient, const TCacheLine& aCacheLine, TInt aSegmentCount);
	void RemoveEmptySegments(CCacheClient* aClient, const TCacheLine& aCacheLine);

	TInt LockCacheLine(CCacheClient* aClient, const TCacheLine& aCacheLine, TInt aLockedPageStart, TInt aLockedPageCount);
	TBool UnlockCacheLine(CCacheClient* aClient, const TCacheLine& aCacheLine);

	TBool FreeCacheLine(CCacheClient* aClient, const TCacheLine& aCacheLine);


	TInt LockCount(CCacheClient* aClient, const TCacheLine& aCacheLine);
	TInt FillCount(CCacheClient* aClient, const TCacheLine& aCacheLine);
	TInt DirtyCount(CCacheClient* aClient, const TCacheLine& aCacheLine);
	TBool TooManyLockedSegments();

	void SetFilled(CCacheClient* aClient, const TCacheLine& aCacheLine,  TInt aSegmentCount);
	void SetDirty(CCacheClient* aClient, const TCacheLine& aCacheLine,  TInt aSegmentCount);
	void ClearDirty(CCacheClient* aClient, const TCacheLine& aCacheLine);
	TBool InUse(const TCacheLine& aCacheLine);

	// called from CKernEventNotifier::FreeMemoryChangeCallback()
	void FreeMemoryChanged(TBool aMemoryLow);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	void DumpCache();
	void DumpCacheLine(TCacheLine& aCacheLine);
	void SimulateLockFailureMode(TBool aEnable);
	void AllocateMaxSegments(TBool aEnable);
	TBool AllocateMaxSegments();
	// stats
	TFileCacheStats& Stats();

	void SimulateWriteFailure();
	TBool SimulateWriteFailureEnabled();
	TBool SimulatedFailure(TInt& aFailureCount);
#endif

private:
	~CCacheManager();

	CCacheManager(TUint aCacheSize);
	void ConstructL();

	inline TUint8* Base();

	inline void CacheLock();
	inline void CacheUnlock();

	TBool StealCacheLine(CCacheClient* aClient);

	TInt Lock(TUint8* aAddr, TInt aSegmentCount);
	TInt Unlock(TUint8* aAddr, TInt aSegmentCount);
	TInt Commit(TUint8* aAddr, TInt aSegmentCount);
	TInt Decommit(TUint8* aAddr, TInt aSegmentCount);

	static CCacheManager* NewCacheL(TInt aUncommitedCacheSize);

	void FreeCacheLine(TCacheLine& aCacheLine);

private:

	RFastLock iLock;

	TCacheLine* iCacheLines;
	RPointerArray<TCacheLine> iFreeQueue;
	RPointerArray<TCacheLine> iUsedQueue;

	TUint8* iBase;
	
	TInt iNumOfCacheLines;
	
	TInt iCacheLineSize;
	TInt iCacheLineSizeLog2;

	TInt iCacheSize;
	TInt iMaxLockedSegments;
	
	TInt iSegmentsPerCacheLine;

	TInt iLockedSegmentCount;
	TInt iAllocatedSegmentCount;

	RChunk iChunk;
	TBool iSimulateLockFailureMode;

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	TBool iManagerLocked;
	TInt iLockFailureCount;
	TInt iCommitFailureCount;
	TInt iAllocFailureCount;
	TBool iAllocateMaxSegments;

	TFileCacheStats iStats;
	TBool iSimulateWriteFailure;
#endif

	// low memory notification stuff
	TBool iMemoryLow;			// ETrue if kernel has notified us that memory has fallen below a specified threshold
	TInt iLowMemoryThreshold;

	// index of the next cacheline in iUsedQueue to steal if 
	// all cachelines are in use
	TInt iNextCacheLineToSteal;

friend class CCacheManagerFactory;
friend class CCacheClient;
	};


NONSHARABLE_CLASS(TGlobalFileCacheSettings)
	{
public:
	static void ReadPropertiesFile();

	static TBool Enabled();
	static TInt CacheSize();
	static TInt MaxLockedSize();
	static TInt LowMemoryThreshold();
private:
	static TBool iEnabled;
	static TInt32 iCacheSize;
	static TInt32 iMaxLockedSize;
	static TInt32 iLowMemoryThreshold;
	};

#endif	// !defined(__SF_CACHE_MAN_H__)
