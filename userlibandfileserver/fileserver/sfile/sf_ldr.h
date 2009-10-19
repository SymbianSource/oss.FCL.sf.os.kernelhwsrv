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
// f32\sfile\sf_ldr.h
// 
//

#ifndef __SF_LDR_H__
#define __SF_LDR_H__

class TLoaderDeletedList
	{
public:
	TLoaderDeletedList* iNext;
	TInt iDriveNumber;
	TInt64 iStartBlock;
	HBufC* iFileName;
	};


NONSHARABLE_CLASS(CReaperCleanupTimer): public CTimer
	{
public:
	CReaperCleanupTimer();
	static TInt New();
	static void Complete();
private:
	~CReaperCleanupTimer();
	void RunL();
	void Start();
	static CReaperCleanupTimer* Timer;
	};

NONSHARABLE_CLASS(CActiveReaper) : public CActive
	{
public:
	static CActiveReaper* New();
	~CActiveReaper();
	void StartReaper();
	
	void AddDeleted(TLoaderDeletedList* aLink);	
	void InitDelDir();
private:
	CActiveReaper();

	void Construct();
	virtual void RunL();
	virtual void DoCancel();
	
	TLoaderDeletedList* iLoaderDeletedList;
	};

NONSHARABLE_CLASS(CSlottedChunkAllocator) : public CBase
	{
public:
	TInt Construct();
	TAny* Alloc(TUint aSize);
	void Free(TAny* aPtr);

private:
	// slot size must be multiple of page size
	enum { KSlots=2, KSlotSize=16777216 };
	TUint iUsed[KSlots];
	RChunk iChunk;
	TUint8* iBase;
	};

extern CSlottedChunkAllocator gFileDataAllocator;


//#define TRACE_CHECK_FAILURES
//#define TRACE_ON


#ifdef TRACE_CHECK_FAILURES
static TInt CheckFail(const char* aFile,TUint aLine)
	{
	RDebug::Printf("\nLoader check failure: %s line %d\n",aFile,aLine);
	return 0;
	}
#define CHECK_FAIL CheckFail(__FILE__,__LINE__)
#define CHECK_FAILURE(_r) (void)(_r==0 || CHECK_FAIL)
#else
#define CHECK_FAIL ((void)0)
#define CHECK_FAILURE(_r) ((void)0)
#endif

#define RETURN_FAILURE(_r) return (CHECK_FAIL,_r)
#define LEAVE_FAILURE(_r) User::Leave((CHECK_FAIL,_r))

#ifdef TRACE_ON
#define TRACE(_t) RDebug::Printf _t
#else
#define TRACE(_t) ((void)0)
#endif
#define E32IMAGEHEADER_TRACE TRACE



#endif
