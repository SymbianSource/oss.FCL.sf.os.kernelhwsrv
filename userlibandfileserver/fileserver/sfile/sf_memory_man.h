// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_memory_man.h
// 
//

/**
 @file
 @internalTechnology
*/

#if !defined(__SF_MEMORY_MAN_H__)
#define __SF_MEMORY_MAN_H__

#include <e32def.h>

const TUint KSegmentSizeLog2 = 12;
/**	Kernel's memory page size in bytes */
const TUint KSegmentSize = 1 << KSegmentSizeLog2;

/**	Forward declaration */
class CCacheMemoryClient;

/**	Cache memory manager class */
class CCacheMemoryManager : public CBase
	{
public:
	static CCacheMemoryManager* NewL(TInt aSizeInBytes);
	IMPORT_C CCacheMemoryClient* ConnectClientL(const TDesC& aClientName, TUint32 aMinSizeInSegs, TUint32 aMaxSizeInSegs);
	IMPORT_C TUint SegmentSizeInBytesLog2() const;
	TInt	RegisterClient(CCacheMemoryClient* aClient);
	TInt 	AllocateAndLockSegments(TUint8* aStartRamAddr, TInt aSegmentCount);
	TInt 	LockSegments(TUint8* aStartRamAddr, TUint32 aSegmentCount);
	TInt 	UnlockSegments(TUint8* aStartRamAddr, TUint32 aSegmentCount);
	TInt 	DecommitSegments(TUint8* aStartRamAddr, TUint32 aSegmentCount);
	TUint8* Base();
	void 	FreeMemoryChanged(TBool aIsMemoryLow);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	void    SetMemoryLow(TBool aSetMemoryLow) {isMemoryLow = aSetMemoryLow;}; 
#endif //defined(_DEBUG) || defined(_DEBUG_RELEASE)

private:
	~CCacheMemoryManager();
	CCacheMemoryManager(TUint32 aMaxSize);
	void 	ConstructL();
	TInt 	Lock(TUint8* aStartRamAddr, TInt aSegmentCount);
	TInt 	Unlock(TUint8* aStartRamAddr, TInt aSegmentCount);
	TInt	Commit(TUint8* aStartRamAddr, TInt aSegmentCountt);
	TInt 	Decommit(TUint8* aStartRamAddr, TInt aSegmentCount);

private:
	RChunk 	iChunk;					///< the RChunk interface to interact with demand paging sub-system
	TUint8* iBase;					///< the base ram address of the manager
	TUint32 iSizeInBytes;			///< virtual ram address size in bytes
	TUint32 iCurrentOffsetMark;		///< a flag in bytes to record current used virtual address space 
	TInt	iLowMemoryThreshold;	///< a threshold in bytes to flag the low memory condition, data type is aligned with TMemoryInfoV1::iTotalRamInBytes
	TBool	isMemoryLow;			///< the flag indicates low memory condition

	/**	an array holds all the registered clients */
	RPointerArray<CCacheMemoryClient> iRegisteredClients;

	friend class CCacheMemoryManagerFactory;
	};

/**
The factory class for CCacheMemoryManager
*/
class CCacheMemoryManagerFactory
	{
public:
	static void CreateL();
	static void Destroy();
	IMPORT_C static CCacheMemoryManager* CacheMemoryManager();
private:
	static CCacheMemoryManager* iCacheMemoryManager;
	};

/**
A static class to read cache memory manager settings
*/
NONSHARABLE_CLASS(TGlobalCacheMemorySettings)
	{
public:
	static void ReadPropertiesFile();

	static TInt32 CacheSize();
	static TInt32 LowMemoryThreshold();
private:
	static TInt32 iCacheSizeInBytes;
	static TInt32 iLowMemoryThreshold;
	};

/** Default cache memory size in KBytes (8192 KBytes)*/
const TInt 	KDefaultGlobalCacheMemorySize = (8 << 10);
/** 
Low memory threshold as a percentage of total RAM (10 %)
If the amount of RAM drops below this value, attempts to allocate memory will fail
*/
const TInt 	KDefaultLowMemoryThreshold = 10;

#endif	// !defined(__SF_MEMORY_MAN_H__)
