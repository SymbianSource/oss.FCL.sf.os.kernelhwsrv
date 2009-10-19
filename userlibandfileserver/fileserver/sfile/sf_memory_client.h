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
// f32\sfile\sf_memory_client.h
// 
//

/**
 @file
 @internalTechnology
 @released
*/

#if !defined(__SF_MEMORY_CLIENT_H__)
#define __SF_MEMORY_CLIENT_H__

#include "sf_memory_man.h"

/**
Cache memory client class, used by various caches as an interface to cache memory manager
@see CCacheMemoryManager
*/
class CCacheMemoryClient : public CBase
	{
public:
	IMPORT_C TUint8* 	AllocateAndLockSegments(TUint32 aSegmentCount);
	IMPORT_C TInt		LockSegments(TUint8* aStartRamAddr, TUint32 aSegmentCount);
	IMPORT_C TInt		UnlockSegments(TUint8* aStartRamAddr, TUint32 aSegmentCount);
	IMPORT_C TInt		DecommitSegments(TUint8* aStartRamAddr, TUint32 aSegmentCount);
	IMPORT_C void 		Reset();

	void	SetBaseOffset(TUint32 aOffset);
	TInt	Initialize();
	static CCacheMemoryClient* NewL(CCacheMemoryManager& aManager, const TDesC& aClientName, TUint32 aOffsetInBytes, TUint32 aMinSizeInSegs, TUint32 aMaxSizeInSegs);	
	CCacheMemoryClient(CCacheMemoryManager& aManager, TUint32 aMinSizeInSegs, TUint32 aMaxSizeInSegs);
	~CCacheMemoryClient();
	TUint32 MaxSizeInBytes();
	const TDesC& Name() const;

private:
	void ConstructL(const TDesC& aClientName, TUint32 aOffsetInBytes);

private:
	CCacheMemoryManager& iManager;		///< reference of the cache memory manager it registers with
	HBufC*	iName;						///< a unique identifier of the client
	TUint8* iBase;						///< the base address of the client memory space
	TUint32	iMinSizeInSegs;				///< the minimum size of the client in segments
	TUint32	iMaxSizeInSegs;				///< the maximum size of the client in segments

	/** an indicator in segment number, indicates how many segments has been used, 
	note this figure never decrease, unless it is reset when disconnection happens */
	TUint32	iTouchedRegionFlag;			 

	/** an indicator in segment number, indicates how many segments should be reserved 
	for permenently locked pages */	
	TUint32 iReservedRegionMarkInSegs;

	/** an array holds all the pages that can be reused, i.e. decommited pages	*/
	RArray<TUint8*>	iReusablePagePool;
	
	TUint32 iSegSizeInBytes;
	TUint iSegSizeInBytesLog2; 
	friend class CCacheMemoryManager;
	};

#endif //__SF_MEMORY_CLIENT_H__
