// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\inc\f32dbg.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
 @released
*/
#if !defined(__F32DBG_H__)
#define __F32DBG_H__
#include <e32std.h>
#include <e32std_private.h>


/**
@publishedPartner
@released

A bit value indicating that tracing is to be active in
file system code, when set through a call to RFs::SetDebugRegister()
in _DEBUG or _DEBUG_RELEASE mode.
*/
#define KFSYS       0x0001


/**
@publishedPartner
@released

A bit value indicating that tracing is to be active in
file server code, when set through a call to RFs::SetDebugRegister()
in _DEBUG or _DEBUG_RELEASE mode.
*/
#define KFSERV      0x0002


/**
@publishedPartner
@released

A bit value indicating that tracing is to be active in
the loader code, when set through a call to RFs::SetDebugRegister()
in _DEBUG or _DEBUG_RELEASE mode.
*/
#define KFLDR       0x0004


/**
@publishedPartner
@released

A bit value indicating that the number of (simulated) allocation
failures to complete a given function is to be printed, when set
through a call to RFs::SetDebugRegister() in _DEBUG or _DEBUG_RELEASE mode.
*/
#define KALLOC		0x0008


/**
@publishedPartner
@released

A bit value indicating that tracing is to be active in
LFFS code, when set through a call to RFs::SetDebugRegister()
in _DEBUG or _DEBUG_RELEASE mode.
*/
#define KLFFS		0x0010


/**
@publishedPartner
@released

A bit value indicating that tracing is to be active in
ISO9660 code, when set through a call to RFs::SetDebugRegister()
in _DEBUG or _DEBUG_RELEASE mode.
*/
#define	KISO9660	0x0020


/**
@publishedPartner
@released

A bit value indicating that tracing is to be active in
NTFS code, when set through a call to RFs::SetDebugRegister()
in _DEBUG or _DEBUG_RELEASE mode.
*/
#define	KNTFS		0x0040


/**
@publishedPartner
@released

A bit value indicating that tracing is to be active in
ROFS code, when set through a call to the function RFs::SetDebugRegister()
in _DEBUG or _DEBUG_RELEASE mode.
*/
#define	KROFS		0x0080


/**
@publishedPartner
@released

A bit value indicating that tracing is to be active in
concurrent file system code, when set through a call to the function RFs::SetDebugRegister()
in _DEBUG or _DEBUG_RELEASE mode.
*/
#define KTHRD		0x0100

/**
@internalTechnology
*/
#define KCACHE		0x0200


/**
@internalTechnology
@prototype

A bit value indicating that tracing is to be active in
COMPFS code, when set through a call to the function RFs::SetDebugRegister()
in _DEBUG or _DEBUG_RELEASE mode.
*/
#define	KCOMPFS		0x0400


/**
@internalTechnology
@prototype

A bit value indicating that tracing is to be active in
file server plugin support code, when set through a call
to the function RFs::SetDebugRegister()
*/
#define KPLUGIN		0x0800


// #define _DEBUG_RELEASE

const TInt KAllocFailureOn=0;
const TInt KAllocFailureOff=-1;


#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
const TInt KControlIoCancelNCNotifier=KMaxTInt-1;
const TInt KControlIoNCDebugNotifierOn=KMaxTInt-2;
const TInt KControlIoNCDebugNotifierOff=KMaxTInt-3;
const TInt KControlIoGetCorruptLogRecord=KMaxTInt-4;
const TInt KControlIoGetNumberOfCorruptLogRecords=KMaxTInt-5;
const TInt KControlIoGetCorruptListFile=KMaxTInt-6;
const TInt KControlIoCorruptLogRecordReset=KMaxTInt-7;
const TInt KControlIoCacheCount=KMaxTInt-8;
const TInt KControlIoGetLocalDriveNumber=KMaxTInt-9;
const TInt KControlIoCancelDeferredDismount=KMaxTInt-10;
const TInt KControlIoFileCacheFlagsWrite=KMaxTInt-11;
const TInt KControlIoFileCacheStats=KMaxTInt-12;
const TInt KControlIoFlushClosedFiles=KMaxTInt-13;
const TInt KControlIoSimulateLockFailureMode=KMaxTInt-14;
const TInt KControlIoFileCacheDump=KMaxTInt-15;
const TInt KControlIoAllocateMaxSegments=KMaxTInt-16;

const TInt KControlIoDisableFatUtilityFunctions=KMaxTInt-17;
const TInt KControlIoEnableFatUtilityFunctions=KMaxTInt-18;
const TInt KControlIoFileCacheConfig=KMaxTInt-19;
const TInt KControlIoSimulateFileCacheWriteFailure=KMaxTInt-20;

const TInt KControlIoSessionCount=KMaxTInt-21;
const TInt KControlIoObjectCount=KMaxTInt-22;

const TInt KControlIoIsFileSequential=KMaxTInt-23;
const TInt KControlIoGlobalCacheConfig=KMaxTInt-24;
const TInt KControlIoGlobalCacheInfo=KMaxTInt-25;
const TInt KControlIoDirCacheConfig=KMaxTInt-26;
const TInt KControlIoDirCacheInfo=KMaxTInt-27;
const TInt KControlIoSimulateMemoryLow=KMaxTInt-28;
const TInt KControlIoStopSimulateMemoryLow=KMaxTInt-29;

const TInt KNCDebugNotifierValue=-500000;	// between 0 and 1 second

GLREF_D TInt DebugNCNotifier;

class TIOCacheValues
	{ 
public:
	TInt iFreeCount;	// current number of requests on free queue
	TInt iCloseCount;	// current number of requests on close queue
	TInt iAllocated;	// no longer used
	TInt iTotalCount;	// current number of requests
	TInt iRequestCountPeak;	// peak number of requests, i.e. peak value reached by iTotalCount


	// the same again but for the OperationAllocator
	TInt iOpFreeCount;
	TInt iOpRequestCount;
	TInt iOpRequestCountPeak;
	};

class TFileCacheStats
	{ 
public:
	TInt iFreeCount;
	TInt iUsedCount;
	TInt iLockedSegmentCount;
	TInt iAllocatedSegmentCount;
	TInt iFileCount;
	TInt iFilesOnClosedQueue;
	TInt iHoleCount;

	TInt iUncachedPacketsRead;
	TInt iUncachedBytesRead;

	TInt iUncachedPacketsWritten;
	TInt iUncachedBytesWritten;

	TInt iCommitFailureCount;
	TInt iLockFailureCount;

	TInt iWriteThroughWithDirtyDataCount;
	};

class TFileCacheConfig
	{ 
public:
	TInt iDrive;
	TInt iFlags;
	TBool iFileCacheReadAsync;
	TInt32 iFairSchedulingLen;			// in bytes
	TInt32 iCacheSize;					// in bytes
	TInt32 iMaxReadAheadLen;			// in bytes
	TInt32 iClosedFileKeepAliveTime;	// in microseconds
	TInt32 iDirtyDataFlushTime;			// in microseconds
	};

struct TFsDebugCorruptLogRecord
	{
	TFileName iProcessName;
	TFileName iFileName;
	TInt iError;
	};

typedef TPckgBuf <TFsDebugCorruptLogRecord> TFsDebugCorruptLogRecordBuf;

extern TBool EnableFatUtilityFunctions;

class TGlobalCacheConfig
    {
public:
    TInt32 iGlobalCacheSizeInBytes;      		// in bytes 
    TInt32 iGlobalLowMemoryThreshold;         	// in percentage
    };

class TGlobalCacheInfo
    {
public:
    TInt32 iGlobalCacheSizeInBytes;     		// in bytes 
    TInt32 iGlobalLowMemoryThreshold;   		// in percentage
    };

class TDirCacheConfig
    {
public:
    TInt iDrive;

    TInt32 iLeafDirCacheSize;           // in number of most recently visited leaf directories
    TInt32 iDirCacheSizeMin;            // in bytes
    TInt32 iDirCacheSizeMax;            // in bytes
    };

class TDirCacheInfo
    {
public:
    TInt iDrive;

    /**
    Segment size in bytes. A memory segment is the smallest memory unit that Kernel manages through RChunk.
    */
    TInt32 iMemorySegmentSize;

    /**
    Size of memory a page occupies, in bytes.
    Note: following restrictions may result a difference between page size in memory & page size in data:
        1. page size can not be smaller than segment size
        2. one page can not contain data from two different clusters
    */
    TInt32 iPageSizeInMemory;

    /**
    Size of actual data a page contains, in bytes.
    Note: following restrictions may result a difference between page size in memory & page size in data:
        1. page size can not be smaller than segment size
        2. one page can not contain data from two different clusters
    */
    TInt32 iPageSizeInData;
    
    /**
    The minimum number of pages that the cache can contain, even under low memory conditions.
    */
    TInt32 iMinCacheSizeInPages;

    /**
    The maximum number of pages that the cache can contain, when there is enough free memory in the system.
    */
    TInt32 iMaxCacheSizeInPages;
    
    /**
    The minimum cache size in memory. In bytes.
    */
    TInt32 iMinCacheSizeInMemory;

    /**
    The maximum cache size in memroy. In bytes.
    */
    TInt32 iMaxCacheSizeInMemory;
    
    /**
    Current count of the locked pages.
    */
    TInt32 iLockedPageNumber;

    /**
    Current count of the unlocked pages.
    */
    TInt32 iUnlockedPageNumber;
    
    };

#endif

enum TLoaderDebugFunction
	{
	ELoaderDebug_SetHeapFail,
	ELoaderDebug_SetRFsFail
	};


enum TFsDebugFunction
	{
	EFsSetAllocFailure,
	EFsSetErrorCondition,
	EFsSetDebugRegister,
	EFsDebugNotify
	};

#endif

