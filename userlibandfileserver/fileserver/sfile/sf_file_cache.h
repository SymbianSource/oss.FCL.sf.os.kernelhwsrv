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
// f32\sfile\sf_file_cache.h
// 
//

/**
 @file
 @internalTechnology
*/


#if !defined(__SF_FILE_CACHE_H__)
#define __SF_FILE_CACHE_H__

#include "sf_cache_client.h"

//#define _DEBUG_READ_AHEAD

// if writing sequentially, start flushing dirty data when the cache is half full
//#define DOUBLE_BUFFERED_WRITING


NONSHARABLE_CLASS(CFileCache) : public CFsDispatchObject
	{
public:
	static CFileCache* NewL(CFileShare& aShare);
	CFileCache* ReNewL(CFileShare& aShare);
	~CFileCache();
	void Init(CFileShare& aShare);

	CMountCB& Mount() const;

	CFileCB* FileCB();

	void ReadAhead(CFsMessageRequest& aMsgRequest, TUint aMode);
	void ResetReadAhead();

	static TInt CompleteRead(CFsRequest* aMsgRequest);
	static TInt CompleteWrite(CFsRequest* aMsgRequest);
	TInt ReadBuffered(CFsMessageRequest& aMsgRequest, TUint aMode);
	TInt WriteBuffered(CFsMessageRequest& aMsgRequest, TUint aMode);

	inline TBool IsDirty() {return iDirtyDataOwner != NULL;}
	TInt FlushDirty(CFsRequest* aOldRequest = NULL);
	void Purge(TBool aPurgeDirty = EFalse);

	// from CFsObject
	void Close();

	inline TInt64 Size64() const {return iSize64;}
	
	// Sets 64 bit (cached) file size
	void SetSize64(TInt64 aSize);
	
	TDrive& Drive() const;
	TUint32 NameHash() const;
	HBufC& FileNameF() const;

    void MarkFileClean();

private:
	void ConstructL(CFileShare& aShare);
	CFileCache();
	
	void SetFileCacheFlags(CFileShare& aShare);

	TInt DoReadBuffered(CFsMessageRequest& aMsgRequest, TUint aMode, CFsClientMessageRequest*& aNewRequest);
	TInt DoWriteBuffered(CFsMessageRequest& aMsgRequest, CFsClientMessageRequest*& aNewRequest, TUint aMode);
	enum TFlushMode {EFlushSingle, EFlushHalf, EFlushAll};
	TInt DoFlushDirty(CFsClientMessageRequest*& aNewRequest, CFsRequest* aOldRequest, TFlushMode aFlushMode);


	inline TInt SegmentSize() const;
	inline TInt SegmentSizeLog2() const;
	inline TInt64 SegmentSizeMask() const;

	inline void UpdateSharePosition(CFsMessageRequest& aMsgRequest, TMsgOperation& aCurrentOperation);

	static TInt CompleteFlushDirty(CFsRequest* aMsgRequest);
	TInt FlushDirtySm(CFsMessageRequest& aMsgRequest);

	TInt AllocateRequest(CFsClientMessageRequest*& aNewRequest, TBool aWrite, CSessionFs* aSession = NULL,TUid aUid = KNullUid);

	void DoReadAhead(CFsMessageRequest& aMsgRequest, TUint aMode);

	void FileDirty(CFsMessageRequest& aMsgRequest);
	
    
	
    TInt HandleWriteDirtyError(TInt aError);
	void PropagateFlushErrorToAllFileShares();

	static TInt ClosedTimerEvent(TAny* aFileCache);
	static TInt DirtyTimerEvent(TAny* aFileCache);

	TBool IsDriveThread();
	
private:

	CCacheClient* iCacheClient;

	TInt iMaxReadAheadLen;
	TInt iCacheSize;

	TInt32 iClosedFileKeepAliveTime;	// in microseconds
	TInt32 iDirtyDataFlushTime;			// in microseconds

	CFileCB* iFileCB;
	
	RFastLock iLock;

	TInt iFlushError;
	TBool iFlushBusy;

	CDriveThread* iDriveThread;
	TInt iDriveNum;
	
	// closed queue eject timer
	TThreadTimer iClosedTimer;

	// dirty data flush timer
	TThreadTimer iDirtyTimer;
	// The last session writing to this file. If non-NULL indicates cache contains dirty data
	CSessionFs* iDirtyDataOwner;
	
	// The size of the file.
	TInt64 iSize64;
	
	// The size of the file at the start of a write request
	TInt64 iInitialSize;
	
	// The full name of the file, including drive and extensions - Folded. This is "stolen" 
	// from the owning CFileCB when the CFileCB destructor calls CFileCache::Close().
	HBufC* iFileNameF;

	TUint32	iNameHash;

	TDrive* iDrive;

	CMountCB* iMount;

	// Read-ahead 
	TInt iSequentialReads;
	TInt iReadAheadLen;
	TInt64 iReadAheadPos;
	TInt64 iLastReadPos;
	TInt iLastReadLen;
	TBool iFileCacheReadAsync;
	CFsClientMessageRequest* iReadAheadRequest;

#ifdef DOUBLE_BUFFERED_WRITING
	// sequential append-write detection
	TInt iSequentialAppends;
#endif

	friend class TClosedFileUtils;
	friend class TFsFileWriteDirty;
	};


class TFileCacheSettings
	{
public:
	class TFileCacheConfig
		{
	public:
		TFileCacheConfig(TInt aDrive);
		static TFileCacheFlags ConvertEnumToFlags(const TInt aFileCacheRead, const TInt aFileCacheReadAhead, const TInt aFileCacheWrite);
	public:
		TInt iDrive;
		TFileCacheFlags iFlags;

		TBool iFileCacheReadAsync;
		TInt32 iFairSchedulingLen;			// in bytes
		TInt32 iCacheSize;					// in bytes
		TInt32 iMaxReadAheadLen;			// in bytes
		TInt32 iClosedFileKeepAliveTime;	// in microseconds
		TInt32 iDirtyDataFlushTime;			// in microseconds
		};

public:
	static TInt ReadPropertiesFile(TInt aDriveNumber);
	static void SetFlags(TInt aDrive, TFileCacheFlags alags);
	static TFileCacheFlags Flags(TInt aDrive);


	static TInt FileCacheReadAsync(TInt aDrive);
	static TInt FairSchedulingLen(TInt aDrive);
	static TInt MaxReadAheadLen(TInt aDrive);
	static TInt CacheSize(TInt aDrive);
	static TInt ClosedFileKeepAliveTime(TInt aDrive);
	static TInt DirtyDataFlushTime(TInt aDrive);
	static TInt GetFileCacheConfig(TInt aDrive, TFileCacheConfig*& aConfig);
private:
	static void Init();
	static void ReadEnum(const TDesC8& aSection, const TDesC8& aProperty, TInt32& aEnumVal, const TPtrC8* aEnumStrings);

private:
	static RArray<TFileCacheConfig>* iFileCacheSettings;
	};


class TClosedFileUtils
	{
public:
	static void InitL();

	static TInt Count();
	static CFileCache* At(TInt aIndex);
	static TBool IsClosed(CFileCache* aFileCache);

	static void AddL(CFileCache* aFileCache, TBool aLock);

	static void Remove();
	static void Remove(TInt aDrvNumber);
	static void Remove(CFileCache* aFileCache);

	static void Lock();
	static void Unlock();

	static void ReOpen(CFileCache* aFileCache, TBool aLock);
private:
	typedef TBool (*TTestFunc)(CFileCache& aFileCache, TAny* aVal);
	static void RemoveFiles(TTestFunc aTestFunc, TAny* aTestVal);

	static TBool TestDrive(CFileCache& aFileCache, TAny* aVal);
	static TBool TestFile(CFileCache& aFileCache, TAny* aVal);

private:
	static CFsObjectCon* iClosedFiles;
	};



#endif
