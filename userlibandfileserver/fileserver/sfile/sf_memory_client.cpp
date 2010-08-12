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
// f32\sfile\sf_memory_client.cpp
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
#include "sf_memory_man.h"
#include "sf_memory_client.h"


CCacheMemoryClient::~CCacheMemoryClient()
	{
	const TUint32 segCnt = iTouchedRegionFlag <= iReservedRegionMarkInSegs ? 
										iReservedRegionMarkInSegs : iTouchedRegionFlag;
	TInt r = DecommitSegments(iBase, segCnt);
    if (r != KErrNone)  // this 'if() {}' is to remove build warnings
	{
	ASSERT(0);
	}
	iReusablePagePool.Close();
	delete iName;
	}

/**
Static factory function for CCacheMemoryClient
@param	aManager	reference of CCacheMemoryManager that this client register with
@param	aClientName	the unique identification of CCacheMemoryClient
@param	aOffsetInBytes	the offset to the base position of CCacheMemoryManager it registered
@param	aMinSizeInSegs	the minimum client size in segments, equals to the number of reserved segments
@param	aMaxSizeInSegs	the maximum client size in segments
*/
CCacheMemoryClient* CCacheMemoryClient::NewL(CCacheMemoryManager& aManager, const TDesC& aClientName, TUint32 aOffsetInBytes, TUint32 aMinSizeInSegs, TUint32 aMaxSizeInSegs)
	{
	// only create clients when sensible parameters are provided, otherwise will cause fatal error to file server 
	if (aMinSizeInSegs > 0 && aMaxSizeInSegs >= aMinSizeInSegs && aClientName.Length() > 0)
		{
		CCacheMemoryClient* self = new(ELeave) CCacheMemoryClient(aManager, aMinSizeInSegs, aMaxSizeInSegs);
		CleanupStack::PushL(self);
		self->ConstructL(aClientName, aOffsetInBytes);
		CleanupStack::Pop(1, self);
		return self;
		}
	return NULL;
	}

/**
Constructor of CCacheMemoryClient
@param	aManager	reference of CCacheMemoryManager that this client register with
@param	aMinSizeInSegs	the minimum client size in segments, equals to the number of reserved segments
@param	aMaxSizeInSegs	the maximum client size in segments
*/
CCacheMemoryClient::CCacheMemoryClient(CCacheMemoryManager& aManager, TUint32 aMinSizeInSegs, TUint32 aMaxSizeInSegs)
:iManager(aManager),
iMinSizeInSegs(aMinSizeInSegs),
iMaxSizeInSegs(aMaxSizeInSegs)
	{
	}

/**
Second phase constructor of CCacheMemoryClient
@param	aClientName	the unique identification of CCacheMemoryClient
@param	aOffsetInBytes	the offset to the base position of CCacheMemoryManager it registered
*/
void CCacheMemoryClient::ConstructL(const TDesC& aClientName, TUint32 aOffsetInBytes)
	{
    __PRINT4(_L("CCacheMemoryClient::ConstructL(%S, min=%d, max=%d, base=0x%X)"), &aClientName, iMinSizeInSegs, iMaxSizeInSegs, iBase);
	iName = HBufC::NewMaxL(aClientName.Length());
	*iName = aClientName;
	iSegSizeInBytesLog2 = iManager.SegmentSizeInBytesLog2();
	iSegSizeInBytes = 1 << iSegSizeInBytesLog2;
	iReusablePagePool.Close();
	iReusablePagePool.ReserveL(iMinSizeInSegs);
	iBase = iManager.Base() + aOffsetInBytes;
	iReservedRegionMarkInSegs = iMinSizeInSegs;
	TInt r = iManager.AllocateAndLockSegments(iBase, iReservedRegionMarkInSegs);
	User::LeaveIfError(r);
	iTouchedRegionFlag = 0;
	__PRINT(_L("CCacheMemoryClient::ConstructL() return 0"));
	}

/**
Reset the client state, this is normally issued from the cache it connected with.
For exmaple, a FAT volume is dismounted so the directory cache needs to be reset.
Although the cache is reset, the CCacheMemoryClient object should not be destroyed and the reserved region should still 
be hold to make sure when the cache re-connect with the CCacheMemoryClient, it will not fail due to OOM conditions.

@internalTechnology
@released
*/
EXPORT_C void CCacheMemoryClient::Reset()
	{
	__PRINT3(_L("CCacheMemoryClient::Reset(%S, reserved=%d, touched=%d)"), iName, iReservedRegionMarkInSegs, iTouchedRegionFlag);
	
	// reset the cache memeory client to initial states: 
	//     1. all memory that have been 'touched' should be decommitted
	//     2. the reserved region of memory should be re-locked

	// if we have touched more than reserved region of memory, we shall decommit all of them 
	if (iTouchedRegionFlag > iReservedRegionMarkInSegs)
	    {
	    TInt r = DecommitSegments(iBase, iTouchedRegionFlag);
	    if (r != KErrNone)  // this 'if() {}' is to remove build warnings in debug mode, same is below
	        {
	        ASSERT(0);
	        }
	    }
	else   // otherwise we decommit the reserved region of memory only.
	    {
	    TInt r = DecommitSegments(iBase, iReservedRegionMarkInSegs);
	    if (r != KErrNone)  // this 'if() {}' is to remove build warnings in debug mode, same is below
	        {
	        ASSERT(0);
	        }
	    }

    // re-lock the reserved region of memory
	TInt r = iManager.AllocateAndLockSegments(iBase, iReservedRegionMarkInSegs);
    if (r != KErrNone)
        {
        ASSERT(0);
        }

	iTouchedRegionFlag = 0;
	iReusablePagePool.Close();
	r = iReusablePagePool.Reserve(iReservedRegionMarkInSegs);
    if (r != KErrNone)
        {
        ASSERT(0);
        }
	}

/**
Commit an unused set of contiguous segments. 
@param	aSegmentCount	the segment number to be commited.
@return	TUint8*	the starting ram address of the commited segments.

@internalTechnology
@released
*/
EXPORT_C TUint8* CCacheMemoryClient::AllocateAndLockSegments(TUint32 aSegmentCount)
	{
	__PRINT4(_L("CCacheMemoryClient::AllocateAndLockSegments(%S, segs=%d, reserved=%d, touched=%d)"), iName, aSegmentCount, iReservedRegionMarkInSegs, iTouchedRegionFlag);
//	__PRINT2(_L("iBase = 0x%x, segcnt = %d"), iBase, aSegmentCount);
    TUint8* addr = NULL;
    // if we are walking through the reserved region first time, we should
    // make assumption that this area is locked already
    if (iTouchedRegionFlag + aSegmentCount <= iReservedRegionMarkInSegs)
       	{
       	addr = iBase + (iTouchedRegionFlag << iSegSizeInBytesLog2);
       	iTouchedRegionFlag += aSegmentCount;
//       	__PRINT3(_L("!! USED RESERVED SEGS: addr=0x%x, touched=%d, reserved=%d"), addr, iTouchedRegionFlag, iReservedRegionMarkInSegs);
       	return addr;
       	}
    
    TBool touchedMore = EFalse;
	// if we have touched beyond reserved region, get new pages from reusable pool first
    if (iReusablePagePool.Count())
    	{
		addr = iReusablePagePool[0];
		iReusablePagePool.Remove(0);
//       	__PRINT2(_L("!! USED REUSABLE POOL SEGS: addr=0x%x, reusable.Count()=%d"), addr, iReusablePagePool.Count());
    	}
    // or we grow the touched region flag
    else
    	{
    	addr = iBase + (iTouchedRegionFlag << iSegSizeInBytesLog2);
    	touchedMore = ETrue;
//       	__PRINT2(_L("!! GROW TOUCHED SEGS: addr=0x%x, touched=%d"), addr, iTouchedRegionFlag);
    	}
	
    // parameter validation
    ASSERT(((addr - iBase) >> iSegSizeInBytesLog2) + aSegmentCount <= iMaxSizeInSegs);
    if(((addr - iBase) >> iSegSizeInBytesLog2) + aSegmentCount > iMaxSizeInSegs)
    	{
    	ASSERT(0);
    	return NULL;
    	}

    TInt r = iManager.AllocateAndLockSegments(addr, aSegmentCount);
	if (r != KErrNone)
		return NULL;

	if (touchedMore)
	    {
	    iTouchedRegionFlag += aSegmentCount;
	    }
	return addr;
	}

/**
Decommit a set of contiguous segments. 
@param	aStartRamAddr	the start ram address of the region to be decommitted.
@param	aSegmentCount	the segment number to be decommited.
@return		KErrNone if succeed, otherwise a system-wide error code.

@internalTechnology
@released
*/
EXPORT_C TInt CCacheMemoryClient::DecommitSegments(TUint8* aStartRamAddr, TUint32 aSegmentCount)
	{
	__PRINT4(_L("CCacheMemoryClient::DecommitSegments(%S, reserved=%d, touched=%d, segNo=%d)"), iName, iReservedRegionMarkInSegs, iTouchedRegionFlag, aSegmentCount);
	// parameter validation
	if(aStartRamAddr < iBase || 
			(((aStartRamAddr - iBase) >> iSegSizeInBytesLog2) + aSegmentCount > iMaxSizeInSegs))
		{
		ASSERT(0);
		return KErrArgument;
		}
	
	TInt err = iManager.DecommitSegments(aStartRamAddr, aSegmentCount);
	ASSERT(err == KErrNone);
	if (err != KErrNone)
		return err;
	err = iReusablePagePool.Append(aStartRamAddr);
    if (err != KErrNone)
        return err;
	return KErrNone;
	}

/**
Lock a set of contiguous segments. 
@param	aStartRamAddr	the start ram address of the region to be locked.
@param	aSegmentCount	the segment number to be locked.
@return		KErrNone if succeed, otherwise a system-wide error code.

@internalTechnology
@released
*/
EXPORT_C TInt CCacheMemoryClient::LockSegments(TUint8* aStartRamAddr, TUint32 aSegmentCount)
	{
	__PRINT4(_L("CCacheMemoryClient::LockSegments(%S, reserved=%d, touched=%d, segNo=%d)"), iName, iReservedRegionMarkInSegs, iTouchedRegionFlag, (aStartRamAddr - iBase) >> iSegSizeInBytesLog2);
	// parameter validation
	if(aStartRamAddr < iBase || 
			(((aStartRamAddr - iBase) >> iSegSizeInBytesLog2) + aSegmentCount > iMaxSizeInSegs))
		{
		ASSERT(0);
		return KErrArgument;
		}
	return iManager.LockSegments(aStartRamAddr, aSegmentCount);
	}

/**
Unlock a set of contiguous segments. 
@param	aStartRamAddr	the start ram address of the region to be unlocked.
@param	aSegmentCount	the segment number to be unlocked.
@return		KErrNone if succeed, otherwise a system-wide error code.

@internalTechnology
@released
*/
EXPORT_C TInt CCacheMemoryClient::UnlockSegments(TUint8* aStartRamAddr, TUint32 aSegmentCount)
	{
	__PRINT4(_L("CCacheMemoryClient::UnlockSegments(%S, reserved=%d, touched=%d, segNo=%d)"), iName, iReservedRegionMarkInSegs, iTouchedRegionFlag, (aStartRamAddr - iBase) >> iSegSizeInBytesLog2);
	// validate parameters
	if(aStartRamAddr < iBase || 
			(((aStartRamAddr - iBase) >> iSegSizeInBytesLog2) + aSegmentCount > iMaxSizeInSegs))
		{
		ASSERT(0);
		return KErrArgument;
		}
	return iManager.UnlockSegments(aStartRamAddr, aSegmentCount);
	}

//////////////////////////// auxiliary functions /////////////////////////
TUint32 CCacheMemoryClient::MaxSizeInBytes()
	{
	return iMaxSizeInSegs  << iSegSizeInBytesLog2;
	}

const TDesC& CCacheMemoryClient::Name() const
	{
	return *iName;
	}

