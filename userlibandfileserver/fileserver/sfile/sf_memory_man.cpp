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
// f32\sfile\sf_memory_man.cpp
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
#include "sf_memory_man.h"
#include "sf_memory_client.h"

/**
Destructor of the CCacheMemoryManager, need to destroy all registered clients.
*/
CCacheMemoryManager::~CCacheMemoryManager()
	{
	for (TInt i = 0; i < iRegisteredClients.Count(); i++)
		{
		iRegisteredClients[i]->Reset();
		delete iRegisteredClients[i];
		}
	iRegisteredClients.Close();
	iChunk.Close();
	}

/**
Static factory function of CCacheMemoryManager
@param	aCacheSize	the total size of the virtual address space
*/
CCacheMemoryManager* CCacheMemoryManager::NewL(TInt aCacheSize)
	{
	CCacheMemoryManager* cacheMemoryManager = new (ELeave) CCacheMemoryManager(aCacheSize);

	CleanupStack::PushL(cacheMemoryManager);
	cacheMemoryManager->ConstructL();
	CleanupStack::Pop(1, cacheMemoryManager);

	return cacheMemoryManager;
	}

/**
Constructor of CCacheMemoryManager
@param	aMaxSize	the total size of the virtual address space
*/
CCacheMemoryManager::CCacheMemoryManager(TUint32 aMaxSize)
:iBase(NULL),
iSizeInBytes(aMaxSize),
iCurrentOffsetMark(0)
	{
	}

/**
Second phase constructor of CCacheMemoryManager.
Creates RChunk object and sets low memory threshold.
*/
void CCacheMemoryManager::ConstructL()
	{
	// calculate the low-memory threshold below which we fail any attempt to allocate memory
	TMemoryInfoV1Buf meminfo;
	TInt r = UserHal::MemoryInfo(meminfo);
	ASSERT(r==KErrNone);
	User::LeaveIfError(r);
	iLowMemoryThreshold = (TInt) (meminfo().iTotalRamInBytes * (TGlobalCacheMemorySettings::LowMemoryThreshold() / 100.00));
	TChunkCreateInfo createInfo;
	createInfo.SetCache(iSizeInBytes);
	createInfo.SetOwner(EOwnerProcess);
	r = iChunk.Create(createInfo);
	ASSERT(r==KErrNone);
	User::LeaveIfError(r);
	UserSvr::RegisterTrustedChunk(iChunk.Handle());
	iBase = iChunk.Base();
	iRegisteredClients.ReserveL(10);
	__PRINT3(_L("CCacheMemoryManager::ConstructL(lowMem=%d, iSize=%d, base=0x%lx)"), iLowMemoryThreshold, iSizeInBytes, iBase);
	}

/**
Connect or register a client.
Note: callers of this function should be constructor of various caches, it is their resposibility
	to make sure that parameters are sensible when calling this function

@internalTechnology
@released

@param	aClientName	an identifier of the client to be connected
@param	aMinSizeInSegs	minimum client size in segments
@param	aMaxSizeInSegs	maximum client size in segments
@return	CCacheMemoryClient*	pointer to the client that connected, or NULL if parameters are not valid.
@leave	if no memory
*/
EXPORT_C CCacheMemoryClient* CCacheMemoryManager::ConnectClientL(const TDesC& aClientName, TUint32 aMinSizeInSegs, TUint32 aMaxSizeInSegs)
	{
	__PRINT3(_L("CCacheMemoryManager::ConnectClientL([%S], minSeg=%d, maxSeg=%d)"), &aClientName, aMinSizeInSegs, aMaxSizeInSegs);
	
	// search for existing clients by name
	for (TInt i = 0; i < iRegisteredClients.Count(); i++)
		{
		if (aClientName.Compare(iRegisteredClients[i]->Name()) == 0)
			{
			ASSERT(iRegisteredClients[i]->iTouchedRegionFlag == 0);
			__PRINT1(_L("CCacheMemoryManager::ConnectClientL: [%S] found!"), &aClientName);
			return iRegisteredClients[i];
			}
		}

	// if it is a new drive/file system who wants to connect, create a new client for it
	// parameter validation
	if (iSizeInBytes < iCurrentOffsetMark + (aMaxSizeInSegs << SegmentSizeInBytesLog2()))
		{
		__PRINT1(_L("CCacheMemoryManager::ConnectClientL([%S]) failed, please check \"GlobalCacheMemorySize\" setting!!!"), &aClientName);
		User::Leave(KErrArgument);
		}
	
	// note: client creation may leave under OOM conditions
	CCacheMemoryClient* client = CCacheMemoryClient::NewL(*this, aClientName, iCurrentOffsetMark, aMinSizeInSegs, aMaxSizeInSegs);

	// if error happens during client registration, the client will be deleted
	// this may leave under OOM conditions
	TInt err = iRegisteredClients.Append(client);
	if (err != KErrNone)
		{
		delete client;
		client = NULL;
		User::Leave(err);
		}

	// record current offset mark for next client
	iCurrentOffsetMark += (aMaxSizeInSegs << SegmentSizeInBytesLog2());
	return client;
	}

/**
Commit a contiguous set of segments.
@param	aStartRamAddr	the start ram address of the region to be committed.
@param	aSegmentCount	the segment number of the contiguous region to be committed.
@return	TInt	KErrNone if succeeded, KErrNoMemory if passed low memory threshold, otherwise a system-wide error code.
*/
TInt CCacheMemoryManager::AllocateAndLockSegments(TUint8* aStartRamAddr, TInt aSegmentCount)
	{
	__PRINT2(_L("CCacheMemoryManager::AllocateAndLockSegments(base=0x%x, seg=%d)"), aStartRamAddr, aSegmentCount);
	TMemoryInfoV1Buf meminfo;
	TInt r = UserHal::MemoryInfo(meminfo);
	__ASSERT_DEBUG(r==KErrNone,Fault(EMemoryInfoFailed));
	if (r != KErrNone)
		{
		return r;
		}

	if (isMemoryLow || (meminfo().iFreeRamInBytes < iLowMemoryThreshold))
		{
		__PRINT(_L("CCacheMemoryManager: free RAM below threshold !!!"));
		return KErrNoMemory;
		}
	return Commit(aStartRamAddr, aSegmentCount);
	}

/**
Notify the change of memory status
@param	aIsMemoryLow	the flag that sets current memory status
*/
void CCacheMemoryManager::FreeMemoryChanged(TBool aIsMemoryLow)
	{
	isMemoryLow = aIsMemoryLow;
	}

/**
Decommit a contiguous set of segments.
@param	aStartRamAddr	the start ram address of the region to be decommitted.
@param	aSegmentCount	the segment number of the contiguous region to be decommitted.
@return	TInt	KErrNone if succeeded, otherwise a system-wide error code.
*/
TInt CCacheMemoryManager::DecommitSegments(TUint8* aStartRamAddr, TUint32 aSegmentCount)
	{
	return Decommit(aStartRamAddr, aSegmentCount);
	}

/**
Lock a contiguous set of segments.
@param	aStartRamAddr	the start ram address of the region to be locked.
@param	aSegmentCount	the segment number of the contiguous region to be locked.
@return	TInt	KErrNone if succeeded, otherwise a system-wide error code.
*/
TInt CCacheMemoryManager::LockSegments(TUint8* aStartRamAddr, TUint32 aSegmentCount)
	{
	return Lock(aStartRamAddr, aSegmentCount);
	}

/**
Unlock a contiguous set of segments.
@param	aStartRamAddr	the start ram address of the region to be unlocked.
@param	aSegmentCount	the segment number of the contiguous region to be unlocked.
@return	TInt	KErrNone if succeeded, otherwise a system-wide error code.
*/
TInt CCacheMemoryManager::UnlockSegments(TUint8* aStartRamAddr, TUint32 aSegmentCount)
	{
	return Unlock(aStartRamAddr, aSegmentCount);
	}

/**
Commit a contiguous set of segments.
@param	aStartRamAddr	the start ram address of the region to be committed.
@param	aSegmentCount	the segment number of the contiguous region to be committed.
@return	TInt	KErrNone if succeeded, otherwise a system-wide error code.
*/
TInt CCacheMemoryManager::Commit(TUint8* aStartRamAddr, TInt aSegmentCount)
	{
	TInt offset = aStartRamAddr - iBase;
	TInt r = iChunk.Commit(offset, aSegmentCount << KSegmentSizeLog2);
	return r;
	}

/**
Actual implementation of DecommitSegments().
@see CCacheMemoryManager::DecommitSegments().
*/
TInt CCacheMemoryManager::Decommit(TUint8* aStartRamAddr, TInt aSegmentCount)
	{
	return iChunk.Decommit(aStartRamAddr - iBase, aSegmentCount << KSegmentSizeLog2);
	}

/**
Actual implementation of UnlockSegments().
@see CCacheMemoryManager::UnlockSegments().
*/
TInt CCacheMemoryManager::Unlock(TUint8* aStartRamAddr, TInt aSegmentCount)
	{
	TInt r = iChunk.Unlock(aStartRamAddr - iBase, aSegmentCount << KSegmentSizeLog2);
	return r;
	}

/**
Actual implementation of LockSegments().
@see CCacheMemoryManager::LockSegments().
*/
TInt CCacheMemoryManager::Lock(TUint8* aStartRamAddr, TInt aSegmentCount)
	{
	return iChunk.Lock(aStartRamAddr - iBase, aSegmentCount << KSegmentSizeLog2);
	}


//=====================================================================
TUint8* CCacheMemoryManager::Base()
	{
	return iChunk.Base();
	}

/**
Get function, returns log2 value of a segment size in bytes.

@internalTechnology
@released
*/
EXPORT_C TUint CCacheMemoryManager::SegmentSizeInBytesLog2() const
	{
	return KSegmentSizeLog2;
	}


//=============================================================================
/**
The singleton of global cache memory manager
*/
CCacheMemoryManager* CCacheMemoryManagerFactory::iCacheMemoryManager = NULL;

/**
Global factory function of CCacheMemoryManager. 
*/
void CCacheMemoryManagerFactory::CreateL()
	{
	// Panic in DEBUG mode when GlobalCacheMemorySize is set as a negative value.  
	ASSERT(TGlobalCacheMemorySettings::CacheSize() >= 0);
	ASSERT(TGlobalCacheMemorySettings::LowMemoryThreshold() >= 0);
	
	if (TGlobalCacheMemorySettings::CacheSize() > 0)
	    iCacheMemoryManager = CCacheMemoryManager::NewL(TGlobalCacheMemorySettings::CacheSize());
	else
	    __PRINT(_L("\"GlobalCacheMemorySize\" set <= 0, CCacheMemoryManager is not created!!!"));
	}

/**
Global get function of CCacheMemoryManager. 

@internalTechnology
@released
*/
EXPORT_C CCacheMemoryManager* CCacheMemoryManagerFactory::CacheMemoryManager()
	{
	return iCacheMemoryManager;
	}

/**
Global destroy function of CCacheMemoryManager. 
*/
void CCacheMemoryManagerFactory::Destroy()
	{
	delete iCacheMemoryManager;
	iCacheMemoryManager = NULL;
	}

//=============================================================================
const TInt KByteToByteShift = 10;
TInt32 TGlobalCacheMemorySettings::iCacheSizeInBytes	= KDefaultGlobalCacheMemorySize << KByteToByteShift;
TInt32 TGlobalCacheMemorySettings::iLowMemoryThreshold  = KDefaultLowMemoryThreshold;
_LIT8(KLitSectionNameCacheMemory,"CacheMemory");

/**
Read ESTART.TXT file for global cache memory settings 
*/
void TGlobalCacheMemorySettings::ReadPropertiesFile()
	{
	// Get size of cache in kilobytes
	TInt32 cacheSizeInKBytes;
	if (F32Properties::GetInt(KLitSectionNameCacheMemory, _L8("GlobalCacheMemorySize"), cacheSizeInKBytes))
		{
		iCacheSizeInBytes = cacheSizeInKBytes << KByteToByteShift;
		}

	// Get low memory threshold
	TInt32 lowMemoryThreshold;
	if (F32Properties::GetInt(KLitSectionNameCacheMemory, _L8("LowMemoryThreshold"), lowMemoryThreshold))
		iLowMemoryThreshold = lowMemoryThreshold;
	}

TInt32 TGlobalCacheMemorySettings::CacheSize()
	{
	return iCacheSizeInBytes;
	}

TInt32 TGlobalCacheMemorySettings::LowMemoryThreshold()
	{
	return iLowMemoryThreshold;
	}


