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
// f32\sfile\sf_cache_client.h
// 
//

/**
 @file
 @internalTechnology
 @released
*/

#if !defined(__SF_CACHE_CLIENT_H__)
#define __SF_CACHE_CLIENT_H__

#include "sf_cache_man.h"

// fwd refs
class CCacheManager;	

NONSHARABLE_CLASS(CCacheClient) : public CBase
	{
public:
	~CCacheClient();
	void Purge(TBool aPurgeDirty = EFalse);

	TUint8* AllocateAndLockSegments(TInt64 aPos, TInt& aSegmentCount, TBool aMakeDirty, TBool aExtendExisting);

	TUint8* FindAndLockSegments(TInt64 aPos, TInt& aSegmentCount, TInt& aFilledSegmentCount, TInt& aLockError, TBool aMakeDirty);
	TUint8* FindSegment(TInt64 aPos);
	

	void UnlockSegments(TInt64 aPos);


	void MarkSegmentsAsFilled(TInt64 aPos, TInt aSegmentCount);
	void MarkSegmentsAsDirty(TInt64 aPos, TInt aSegmentCount);

	TInt FindFirstDirtySegment(TInt64& aPos, TUint8*& aAddr);

	void MarkSegmentsAsClean(TInt64 aPos);

	
	void SetMaxSegments(TInt aMaxSegments);

	TBool SegmentEmpty(TInt64 aPos);
	TBool SegmentDirty(TInt64 aPos);
	TBool TooManyLockedSegments();
	TBool LockedSegmentsHalfUsed();

	inline TInt CacheOffset(TInt64 aLinAddr) const \
		{return(I64LOW(aLinAddr)& ((1 << iManager.SegmentSizeLog2())-1));}

	TInt SegmentSize();
	TInt SegmentSizeLog2();
	TInt64 SegmentSizeMask();
	TInt CacheLineSize();
	TInt CacheLineSizeInSegments();

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	void DumpCache();
#endif

private:
	// class can only be instantiated from friend class CCacheManager
	static CCacheClient* NewL(CCacheManager& aManager);
	void ConstructL();
	CCacheClient(CCacheManager& aManager);
	

	TInt LockSegments(TInt64 aPos, TInt aSegmentCount, TBool aMakeDirty);

	const CCacheManager::TCacheLine* FindCacheLine(TInt64 aPos);
	TInt FindCacheLineIndex(TInt64 aPos);

	TBool FreeCacheLine(const CCacheManager::TCacheLine* aCacheLine);

	void RemoveCacheLine(const CCacheManager::TCacheLine* aCacheLine);
	void RemoveEmptySegments(const CCacheManager::TCacheLine* aCacheLine);

	void RemoveLru(const CCacheManager::TCacheLine* aCacheLineToExclude);

	inline TInt StartSegment(const CCacheManager::TCacheLine* aCacheLine, TInt64 aPos);
	inline TInt CacheLineLen(const CCacheManager::TCacheLine* aCacheLine);

	TInt CachedBytes();
	TInt LockedBytes();
private:
	CCacheManager& iManager;

	// segment array for segments that contain valid data
	RPointerArray<CCacheManager::TCacheLine>* iCacheLines;
	TInt iMaxBytesCached;

	TInt iLastCacheLineIndex;

	friend class CCacheManager;
	};



#endif
