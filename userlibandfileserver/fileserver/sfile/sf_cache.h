// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_cache.h
// 
//

#if !defined(__SF_CACHE_H__)
#define __SF_CACHE_H__
#include "sf_image.h"

// Cached information for each file
class TRomImageHeader;
class TDirectoryCacheHeader;
class TFileCacheRecord : public TImageInfo
	{
public:
	inline const TText8* NameText() const
		{ return (const TText8*)(this + 1); }
	inline TPtrC8 Name() const
		{ return TPtrC8(NameText(), iNameLength); }
	inline const TUint8* ExportDescription() const
		{ return NameText() + iNameLength; }
	inline TBool ExtrasValid() const
		{ return iUid[0]; }
	inline const TRomImageHeader* RomImageHeader() const
		{ return (const TRomImageHeader*)iUid[0]; }
	inline void SetXIP(const TRomImageHeader* aR)
		{ iUid[0] = (TUint32)aR; }
	inline TBool IsXIP() const
		{ return !(iUid[0]&3); }
	static TInt Order(const TFileCacheRecord& aL, const TFileCacheRecord& aR);
	void Dump(const char* aTitle);
public:
	TInt GetImageInfo(RImageInfo& aInfo, const TDesC8& aPathName, TDirectoryCacheHeader* aDirHead, TInt aIndex);
public:
	// UID1 must be EXE or DLL, 0 means extra information not valid
	// UID1 nonzero and multiple of 4 means XIP in which case it points to the ROM image header
	// iExportDirCount = number of bytes available for export description if iUid[0]=0
	// 8-bit name follows (store base+ext only, not version)
	// export description follows name
	};

class TFileCacheRecordSearch : public TFileCacheRecord
	{
public:
	TFileCacheRecordSearch(const TDesC8& aSearchName);
public:
	TUint8 iSearchName[KMaxKernelName];
	};

// Record of a cached path.
class TPathListRecord
	{
public:
	static TPathListRecord* FindPathNameInList(const TDesC8& aPath);
	inline const TDesC8* PathName() const
		{ return (const TDesC8*)(this + 1); }
	static TInt Init();
private:
	void MoveToHead();
	static TPathListRecord* New(const TDesC8& aPath, TBool aKeep);
	static TPathListRecord* DoFindPathNameInList(const TDesC8& aPath);
	static TPathListRecord* AddToPathList(const TDesC8& aPath, TBool aKeep);
public:
	TPathListRecord* iNext;
	TBool iKeep;
	// place a TBufC8 immediately after
public:
	static TPathListRecord* First;
	static TPathListRecord* LastStatic;
	};

class TCacheHeapList
	{
public:
	TCacheHeapList(TInt aSize);
	TAny* Allocate(TInt aBytes);
public:
	TCacheHeapList* iNext;
private:
	TInt iAllocated;
	TInt iSize;
	};

class TDirectoryCacheHeader;
NONSHARABLE_CLASS(CCacheNotifyDirChange) : public CActive
	{
public:
	CCacheNotifyDirChange(TDriveNumber aDrive, TDirectoryCacheHeader& aDirHead);
	~CCacheNotifyDirChange();
	TInt RegisterNotification(TPathListRecord* aPathRec, TNotifyType aType);
	void DoCancel();
	void RunL();
private:
    TDriveNumber iDrive;
	TDirectoryCacheHeader* iDirHead;
	};

class TEntry;
class TDirectoryCacheHeader
	{
public:
	TDirectoryCacheHeader(TPathListRecord* aPath);
	~TDirectoryCacheHeader();
	TAny* Allocate(const TInt aBytes);
	TFileCacheRecord* NewRecord(const TDesC8& aName, TUint32 aAttr, TUint32 aVer, const TEntry& aEntry);
	TFileCacheRecord* NewRecord(const TFileCacheRecord& aRecord, TInt aEDS);
	TInt PopulateFromDrive(const TDesC8& aPathName);
public:
	TDirectoryCacheHeader* iNext;		// list of directories per drive
	TCacheHeapList* iFirstHeapBlock;	// heap blocks to hold TFileCacheRecord entries
	TPathListRecord* iPath;
	TInt iRecordCount;					// number of TFileCacheRecord entries
	TFileCacheRecord** iCache;			// array of pointers to TFileCacheRecord entries
	TBool iNotPresent;
	CCacheNotifyDirChange* iNotify;
private:
	TInt GetMoreHeap();
	};

class TDriveCacheHeader
	{
public:
	TDriveCacheHeader();
	~TDriveCacheHeader();
	TDirectoryCacheHeader* FindDirCache(TPathListRecord* aPath);
	TInt GetDirCache(TDirectoryCacheHeader*& aCache, TPathListRecord* aPath, const TDesC8& aDriveAndPath);
public:
	TDirectoryCacheHeader* iDirectoryList; // List of Directories
	TUint iDriveAtt;
	TInt iDriveNumber;
	};


void InitializeFileNameCache();
TInt CheckLoaderCacheInit();

#endif
