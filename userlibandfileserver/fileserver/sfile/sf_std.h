// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_std.h
// 
//
 

#ifndef SF_STD_H
#define SF_STD_H

#include "common.h"
#include "message.h"
#include <f32fsys.h>
#include <f32ver.h>
#include <f32dbg.h>
#include <e32svr.h>
#include <e32hal.h>
#include <e32uid.h>
#include <e32std_private.h>
#include <e32def_private.h>
#include <e32const_private.h>
#include "sf_plugin.h"
#include "sf_func.h"
#include <f32plugin.h>
#include "f32trace.h"


#define __PRINT1TEMP_ALWAYS(t,a) {{TBuf<KMaxFileName>temp(a);RDebug::Print(t,&temp);}}
#define __PRINT2TEMP_ALWAYS(t,a,b) {{TBuf<KMaxFileName>temp(b);RDebug::Print(t,a,&temp);}}
#define __PRINT3TEMP_ALWAYS(t,a,b,c) {{TBuf<KMaxFileName>temp(c);RDebug::Print(t,a,b,&temp);}}
#define __PRINT4TEMP_ALWAYS(t,a,b,c,d) {{TBuf<KMaxFileName>temp(d);RDebug::Print(t,a,b,c,&temp);}}

#define _THRD_BUF() (_L("thread id 0x"));buf.AppendNum(RThread().Id(),EHex);buf.Append(_L(" "))
#define __THRD_PRINT_ALWAYS(t) {{TBuf<KMaxFileName> buf=_THRD_BUF();buf.Append(t);RDebug::Print(buf);}}
#define __THRD_PRINT1_ALWAYS(t,a) {{TBuf<KMaxFileName> buf=_THRD_BUF();buf.Append(t);RDebug::Print(buf,a);}}
#define __THRD_PRINT2_ALWAYS(t,a,b) {{TBuf<KMaxFileName> buf=_THRD_BUF();buf.Append(t);RDebug::Print(buf,a,b);}}
#define __THRD_PRINT3_ALWAYS(t,a,b,c) {{TBuf<KMaxFileName> buf=_THRD_BUF();buf.Append(t);RDebug::Print(buf,a,b,c);}}
#define __THRD_PRINT4_ALWAYS(t,a,b,c,d) {{TBuf<KMaxFileName> buf=_THRD_BUF();buf.Append(t);RDebug::Print(buf,a,b,c,d);}}


#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
#define __PRINT(t) {if (DebugReg&KFSERV) RDebug::Print(t);}
#define __PRINT1(t,a) {if (DebugReg&KFSERV) RDebug::Print(t,a);}
#define __PRINT2(t,a,b) {if (DebugReg&KFSERV) RDebug::Print(t,a,b);}
#define __PRINT3(t,a,b,c) {if (DebugReg&KFSERV) RDebug::Print(t,a,b,c);}
#define __PRINT4(t,a,b,c,d) {if (DebugReg&KFSERV) RDebug::Print(t,a,b,c,d);}
#define __IF_DEBUG(t) {if (DebugReg&KFLDR) RDebug::t;}
#define __LDRTRACE(t) {if (DebugReg&KFLDR) t;}
#define __PRINTALLOC(t,a,b,c) {if (DebugReg&KALLOC) RDebug::Print(t,a,b,c);}
#define __PRINT1TEMP(t,a) {if (DebugReg&KFSERV) {TBuf<KMaxFileName>temp(a);RDebug::Print(t,&temp);}}
#define __PRINT2TEMP(t,a,b) {if (DebugReg&KFSERV) {TBuf<KMaxFileName>temp(b);RDebug::Print(t,a,&temp);}}
#define __PRINT3TEMP(t,a,b,c) {if (DebugReg&KFSERV) {TBuf<KMaxFileName>temp(c);RDebug::Print(t,a,b,&temp);}}
#define __PRINT4TEMP(t,a,b,c,d) {if (DebugReg&KFSERV) {TBuf<KMaxFileName>temp(d);RDebug::Print(t,a,b,c,&temp);}}
#define __CALL(t) {t;}

#define __THRD_PRINT(t) {if (DebugReg&KTHRD) {TBuf<KMaxFileName> buf=_THRD_BUF();buf.Append(t);RDebug::Print(buf);}}
#define __THRD_PRINT1(t,a) {if (DebugReg&KTHRD) {TBuf<KMaxFileName> buf=_THRD_BUF();buf.Append(t);RDebug::Print(buf,a);}}
#define __THRD_PRINT2(t,a,b) {if (DebugReg&KTHRD) {TBuf<KMaxFileName> buf=_THRD_BUF();buf.Append(t);RDebug::Print(buf,a,b);}}
#define __THRD_PRINT3(t,a,b,c) {if (DebugReg&KTHRD) {TBuf<KMaxFileName> buf=_THRD_BUF();buf.Append(t);RDebug::Print(buf,a,b,c);}}
#define __THRD_PRINT4(t,a,b,c,d) {if (DebugReg&KTHRD) {TBuf<KMaxFileName> buf=_THRD_BUF();buf.Append(t);RDebug::Print(buf,a,b,c,d);}}

#define __CACHE_PRINT(t) {if (DebugReg&KCACHE) RDebug::Print(t);}
#define __CACHE_PRINT1(t,a) {if (DebugReg&KCACHE) RDebug::Print(t,a);}
#define __CACHE_PRINT2(t,a,b) {if (DebugReg&KCACHE) RDebug::Print(t,a,b);}
#define __CACHE_PRINT3(t,a,b,c) {if (DebugReg&KCACHE) RDebug::Print(t,a,b,c);}
#define __CACHE_PRINT4(t,a,b,c,d) {if (DebugReg&KCACHE) RDebug::Print(t,a,b,c,d);}
#define __CACHE_PRINT5(t,a,b,c,d,e) {if (DebugReg&KCACHE) RDebug::Print(t,a,b,c,d,e);}
#define __CACHE_PRINT6(t,a,b,c,d,e,f) {if (DebugReg&KCACHE) RDebug::Print(t,a,b,c,d,e,f);}
#define __CACHE_PRINT7(t,a,b,c,d,e,f,g) {if (DebugReg&KCACHE) RDebug::Print(t,a,b,c,d,e,f,g);}
#define __PLUGIN_PRINT(t) {if (DebugReg&KPLUGIN) RDebug::Print(t);}
#define __PLUGIN_PRINT1(t,a) {if (DebugReg&KPLUGIN) RDebug::Print(t,a);}
#define __PLUGIN_PRINT2(t,a,b) {if (DebugReg&KPLUGIN) RDebug::Print(t,a,b);}
#define __PLUGIN_PRINT3(t,a,b,c) {if (DebugReg&KPLUGIN) RDebug::Print(t,a,b,c);}
#else
#define __PRINT(t)
#define __PRINT1(t,a)
#define __PRINT2(t,a,b)
#define __PRINT3(t,a,b,c)
#define __PRINT4(t,a,b,c,d)
#define __IF_DEBUG(t)
#define __LDRTRACE(t)
#define __PRINTALLOC(t,a,b,c) {if (DebugReg&KALLOC) RDebug::Print(t,a,b,c);}
#define __PRINT1TEMP(t,a)
#define __PRINT2TEMP(t,a,b)
#define __PRINT3TEMP(t,a,b,c)
#define __PRINT4TEMP(t,a,b,c,d)
#define __CALL(t)
#define __THRD_PRINT(t)
#define __THRD_PRINT1(t,a)
#define __THRD_PRINT2(t,a,b)
#define __THRD_PRINT3(t,a,b,c)
#define __THRD_PRINT4(t,a,b,c,d)
#define __CACHE_PRINT(t)
#define __CACHE_PRINT1(t,a)
#define __CACHE_PRINT2(t,a,b)
#define __CACHE_PRINT3(t,a,b,c)
#define __CACHE_PRINT4(t,a,b,c,d)
#define __CACHE_PRINT5(t,a,b,c,d,e)
#define __CACHE_PRINT6(t,a,b,c,d,e,f)
#define __CACHE_PRINT7(t,a,b,c,d,e,f,g)
#define __PLUGIN_PRINT(t)
#define __PLUGIN_PRINT1(t,a)
#define __PLUGIN_PRINT2(t,a,b)
#define __PLUGIN_PRINT3(t,a,b,c)
#endif

#define _LOFF(p,T,f) ((T*)(((TUint8*)(p))-_FOFF(T,f)))

const TInt KMaxTotalDriveReserved	=0x100000;
const TInt KMaxSessionDriveReserved	=0x10000;

// If TFsFileDuplciate::DoRequestL() is called a new sub-session is created and 
// the new sub-session handle is returned, mangled by KSubSessionMangleBit - 
// this is to discourage use of the duplicated handle.
// If TFsFileAdopt::DoRequestL() is called then the passed sub-session handle is 
// assumed to be already mangled by KSubSessionMangleBit and the same sub-session 
// handle is returned de-mangled.
const TInt KSubSessionMangleBit = 0x4000;


#define __CHECK_DRIVETHREAD(d) {__ASSERT_DEBUG(FsThreadManager::IsDriveThread(d,ETrue),Fault(EFsDriveThreadError));}
#define __CHECK_MAINTHREAD() {__ASSERT_DEBUG(FsThreadManager::IsMainThread(),Fault(EFsMainThreadError));}

#define __LAZY_DLL_UNLOAD

const TInt KMaxTempNameAttempts=50;
const TInt KHeapMinSize=0x1000;
const TInt KHeapMaxSize=0x400000;
const TInt KLoaderStackSize=0x8000;

//-- maximum file size that was supported before introducing File Server 64-bit addressing support.
//-- It is 2G-1. Used in legacy code that does not understand RFile64
const TUint64 KMaxLegacyFileSize    = 0x7fffffff;

//-- absolute maximum file size that file server supports
const TUint64 KMaxSupportedFileSize = KMaxTUint64;

//-- this is a speculative value of a min. amount of free space on the volume necessary to create a file, directory etc.
//-- it is used mostly in "reserve drive space" functionality, which is, actually, fundamentally flawed.
//-- the problem is that the file server can't know exactly how much space is required to create some fs object on the volume, 
//-- so, it has to guess. This is a default "sector size" value; the file system can round it up internally to its cluster size if any.
const TInt KMinFsCreateObjTreshold = KDefaultVolumeBlockSize;


//__DATA_CAGING__
const TUint SHA1_LBLOCK=16;
const TUint SHA1_HASH=20;
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)

class TCorruptNameRec;
class TCorruptLogRec
	{
public:
	TCorruptLogRec();
	~TCorruptLogRec();
	TInt Construct(TCorruptNameRec* aNameRec, TPtrC* aProcessName, TCorruptLogRec* aChain);
	inline TPtrC ProcessName(){return iProcessName->Des();};
	static TInt GetLogRecord(TFsDebugCorruptLogRecordBuf& aLogRecord,TInt aLogRecNum);
	static void DestroyList();
private:
	HBufC* iProcessName;
	TCorruptNameRec* iNameRec;
	TCorruptLogRec* iNext;
	};

TInt OutputTraceInfo(TCorruptNameRec* aNameRec, CFsRequest* aRequest);

class TText8FileReader
	{
	enum {EMaxLine=1024};
public:
	TText8FileReader();
	~TText8FileReader();
	TInt Set(RFile& aFile);
	TInt Read(TPtr& aPtr);	
public:
	RFile iFile;
	TBuf8<EMaxLine> iBuf;
	TInt iBufPos;
	TText* iFileDataBuf;
	TInt iFileSize;	
	};	

class TCorruptNameRec
	{
public:
	TCorruptNameRec();
	TInt Construct(TPtr* aName,TInt aReturnCode, TBool aUseOnce, TCorruptNameRec* aChain);
	static void ResetListConsumed();
	inline TPtrC Name(){return iName->Des();};
	inline TInt ReturnCode(){return iReturnCode;};
	inline TBool Consumed(){return iConsumed;};
	inline void SetConsumed(){iConsumed=iUseOnce;};
	inline TCorruptNameRec* Next(){return iNext;};
private:
	HBufC*	iName;
	TInt	iReturnCode;
	TBool	iUseOnce;
	TBool	iConsumed;
	TCorruptNameRec* iNext;
	};
#endif
//

//
_LIT(KFsClient,"FSCLIENT");

NONSHARABLE_CLASS(CSHA1) : public CBase
	{
public:
	static CSHA1* NewL(void);
	void Update(const TDesC8& aMessage);
	TPtrC8 Final(void);
	~CSHA1(void);
	void Reset(void);
private:
	CSHA1(void);
	TUint iA;
	TUint iB;
	TUint iC;
	TUint iD;
	TUint iE;
	TBuf8<SHA1_HASH> iHash;
	TUint iNl;
	TUint iNh;
	TUint iData[SHA1_LBLOCK*5];
	void DoUpdate(const TUint8* aData,TUint aLength);
	void DoFinal(void);
	void Block();
	void ConstructL(void);
	TUint8* iTempData;
	};

enum TFsPanic
	{
	ELdrImportedOrdinalDoesNotExist	
	};
//
enum TFsFault
	{
	EDrvIllegalShareValue,			//0
	EMainCreateScheduler,
	EMainCreateServer,
	EMainStartServer,
	EMainCreateResources1,
	EMainGetLocalFileSystem,
	EMainGetLocalDefaultPath,
	EInitConnectLocalDrive,
	EMainCreateStartupThread1,
	EInitDriveMappingDriveInfo,
	EInitCreateDriveName,			//10
	EMainStartupNoEStart,
	ESesPathBadDrive,
	EFileDuplicateLock,
	ESysDefaultPathNotSupported,
	ESysAddLocalFileSystem,
	ESvrBadSessionIndex,
	EGetLocalDrive1,
	EGetLocalDrive2,
	ELdrRestartInit,
	ELdrRestartSemaphore,			//20
	EStripBackSlashBadName,
	EIsSubDirBadDes,
	ERawDiskBadAccessCount2,
	ERawDiskBadAccessCount1,
	ERawDiskBadAccessCount,
	EDriveNoDiskMounted,
	ESvrNotifierIsNULL,
	EMainCreateResources5,
	EMainCreateStartupThread2,
	EMainCreateStartupThread3,		//30
	EMainCreateStartupThread4,
	EMainStartupWriteToDiskSemaphore,	
	EMainCreateResources6,
	EMainScanMediaDriversMem1,
	EMainScanMediaDriversLocation,
	EMainScanMediaDriversMem2,
	EMainScanMediaDriverConnect,
	EMainScanMediaDriverDirOpen,
	EMainScanMediaDriverDirRead,
	EMainLoadMediaDriver,			//40
	EInitCreateMediaChangeNotifier,
	ELdrCleanupCreate,
	ELdrSchedulerCreate,
	ELdrServerCreate,
	ELdrFsConnect,
	ELdrFsSetPath,
	ELdrCacheInit,
	ELdrSchedulerStopped,
	ESvrFormatOpenFailed,
	ESvrRawDiskOpenFailed,			//50
	EProxyDriveConstruction,
	ELocDrvInitLocalDrive,
	ELocDrvDismountedLocalDrive,
	EBaseExtConstruction,
	ECreateProxyDriveL,
	EExtensionInfoCount0,
	EExtensionInfoCount1,
	EExtensionInfoCount2,
	EStdChangeRequestType,
	EExtChangeNameLength,			//60
	EDiskSpaceThreshold,
	EDebugChangeType,
	EBaseQueConstruction,
	EChangeQueType,
	EDiskSpaceQueType1,
	EDiskSpaceQueType2,
	EBaseQueCancel,
	EDebugQueType,
	EBaseRequestSrc,
	EBaseRequestDest,			//70
	EBaseRequestDrive,
	EBaseRequestSubstedDrive,
	EBaseRequestSetDrive,
	EBaseRequestSetSubstedDrive,
	EBaseRequestMessage,
	EBaseRequestSet1,
	EBaseRequestSet2,
	EDiskSpaceQueDrive,
	ENotifyInfoDestructor,
	EChangeBadIndex,			//80
	EBadDiskNotifyType,
	EBadDebugNotifyType,
	EDiskChangeDrive,
	EBadChangeNotifyType,
	EDiskBadIndex1,
	EDiskBadIndex2,
	EDiskBadIndex3,
	ESvrFreeDiskSpace,
	EFsThreadBadDrvNum,
	EFsThreadConstructor,			//90
	EFsThreadDriveClose1,
	EFsThreadDriveClose2,
	EFsThreadGetThread,
	EMainInitialiseRomFs,
	EFsDriveThreadError,
	EFsMainThreadError,
	ETFsSetSubstNotNull,
	EMountFileSystemFSys,
	EDisMountFileSystemFSys,
	EIsValidDriveMapping,			//100
	EInitDriveMappingSocketNo,
	ECompleteNotifSocketNo,
	EObjRemoveContainerNotFound,
	EObjDestructorAccessCount,
	EObjDestructorContainer,
	EObjRemoveBadHandle,
	EArrayIndexOutOfRange,
	EObjFindBadHandle,
	EObjRemoveObjectNotFound,
	EObjFindIndexOutOfRange,			//110
	ESubOpenBadHandle,
	EMainDisconnectThread,
	EInternalRequestProcess,
	EInternalRequestComplete1,
	EInternalRequestComplete2,
	EInternalRequestComplete3,
	ERequestThreadDestructor,
	EThreadManagerInitDrive,
	EDriveCurrentWriteFunction,
	EDriveGetNotifyUser,				//120
	EDriveThreadWriteable,
	EDriveThreadNotifyUser1,
	EDriveThreadNotifyUser2,
	ERequestDestructorFree,
	ESessionDisconnectThread1,
	ESessionDisconnectThread2,
	EDismountFsDriveThread,
	EDisconnectRequestDispatch1,
	EDisconnectRequestDispatch2,
	EDisonncectRequestProcess,			//130
	EDispatchObjectDispatch,
	EDispatchObjectThread,
	EInternalRequestDispatch1,
	EInternalRequestDispatch2,
	EInternalRequestDispatch3,
	EFsObjectIxDestructor,
	EDisconnectRequestComplete,
	EMountExtensionFSys,
	EObjectConDestructor,
	EParseSubstSession,					//140
	ELdrGetKernelInfoFailed,
	EObjectIxMainThread,
	ESwapFileSystemNull,
	ESwapFileSystemMount,
	ESwapFileSystemRom,
	EReserveSpaceArithmetic,
	ECloseSubBadMessage,
	EFsCacheLockFailure,
	EFsPluginThreadError,				
	EBadDismountNotifyType,				//150
	ENotifyDismount,
	EAllowDismount,
	ENotifyDismountCancel,
	EFileDuplicateAsyncReadRequest,
	EFileFailedToServiceAsyncReadRequest,
	ELdrCsIdWrap,
	EFileShareBadPromoteCount,
	EInternalRequestDispatchCancelPlugin,
	EMainCreateStartupThread0,
	ERequestAllocatorOpenSubFailed,		//160
	ETParsePoolGet,
	EInvalidDrive,
	ELocDrvInvalidLocalDrive,
	ELdrReaperCreate,
	ELdrReaperCleanupTimerCreate,
	EFsParsePoolLockFailure,
	ELdrReaperCantGetDriveList,
	EFileCacheCreateFailed,
	EBadOperationIndex,
	EBadOperationCompletionCode,		//170
	EReadOffsetNonZero,
	ETooManyOperations,
	EInvalidOperationIndex,
	EInvalidReadLength,
	EInvalidWriteLength,
	ERequestThreadNotInitialised,
	EMemoryInfoFailed,
	EMsgRestartBadFunction,
	EPushOpNoCallback,
	EFreeingMsgWithMsgOp,				//180
	ENotUnused,
	ELdrFileDataAllocInit,
	ELdrFileDataAllocError,
	EDismountLocked,
	EInvalidMsgState,
	EGetProxyDriveMapping1,
	EGetProxyDriveMapping2,
	EExtProxyDriveCaps,
	EIsProxyDrive,
	EClearProxyDriveMapping1,			//190
	EClearProxyDriveMapping2,
	ERequestQueueNotEmpty,
	ESetupMediaChange,
	ECancelNotifyChange,
	EPluginOpError,
	EBadMessageSlotIndex,
	EInvalidCompletionFlags,
	ECacheMemoryManagerCreateFailed,
	EFileBodyIsNull,
	ETraceLddLoadFailure,				//200
	ETooManyDrivesPerSocket,
	ENotificationFault,
	EFsObjectOpen,
	EContainerHeapCorruptionOnRemove,
	ESessionOpenError,
	};


NONSHARABLE_CLASS(CFsObjectConIx) : public CBase
	{
public:
	static CFsObjectConIx* NewL();
	~CFsObjectConIx();
	CFsObjectCon* CreateL();
	void Remove(CFsObjectCon* aCon);
protected:
	CFsObjectConIx();
	void CreateContainerL(CFsObjectCon*& anObject);
private:
	TInt iCount;
	TInt iAllocated;
	TInt iNextUniqueID;
	CFsObjectCon** iContainers;
	};

struct SFsObjectIxRec
	{
	TInt16 instance;
	TInt16 uniqueID;
	CFsObject* obj;
	};

NONSHARABLE_CLASS(CFsObjectIx) : public CBase
	{
public:
	static CFsObjectIx* NewL();
	~CFsObjectIx();
	TInt AddL(CFsObject* anObj,TBool aLock);
	void Remove(TInt aHandle,TBool aLock);
	CFsObject* At(TInt aHandle,TInt aUniqueID,TBool aLock);	
	CFsObject* At(TInt aHandle,TBool aLock);
	TInt At(const CFsObject* anObj,TBool aLock);
	CFsObject* operator[](TInt anIndex);
	void CloseMainThreadObjects();
	inline TInt Count() const;
	inline TInt ActiveCount() const;
	inline void Lock();
	inline void Unlock();
protected:
	CFsObjectIx();
private:
	TInt iNumEntries;		// number of actual entries in the index
	TInt iHighWaterMark;	// 1+highest active index
	TInt iAllocated;		// max entries before realloc needed
	TInt iNextInstance;
	SFsObjectIxRec *iObjects;
	RFastLock iLock;
	};

NONSHARABLE_CLASS(CFsObjectCon) : public CBase
	{
protected:
	enum {ENotOwnerID};
public:
	static CFsObjectCon* NewL();
	~CFsObjectCon();
	void AddL(CFsObject* anObj,TBool aLock);
	void Remove(CFsObject* anObj,TBool aLock);
	CFsObject* operator[](TInt anIndex);
	CFsObject* At(TInt aFindHandle) const;
	CFsObject* AtL(TInt aFindHandle) const;	
	TInt FindByName(TInt& aFindHandle,const TDesC& aMatch) const;
	inline void Lock();
	inline void Unlock();
	inline TInt UniqueID() const;
	inline TInt Count() const;
protected:
	CFsObjectCon(TInt aUniqueID);
	TInt CheckUniqueName(const CFsObject* anObject) const;
	TBool NamesMatch(const TName& anObjectName, const CFsObject* aCurrentObject) const;
public:
	TInt iUniqueID;
	TInt iCount;
	TInt iAllocated;
	CFsObject** iObjects;
	RFastLock iLock;
friend class CFsObjectConIx;
friend class CFsObject;
	};


NONSHARABLE_CLASS(CFsSyncMessageScheduler) : public CActive
	{
public:
	static CFsSyncMessageScheduler* NewL();
	void DoCancel();
	void RunL();
	void Dispatch(CFsRequest* aRequest);
private:
	CFsSyncMessageScheduler();
	void ConstructL();
private:
	RThread iThread;
	RFastLock iLock;
	TDblQue<CFsRequest> iList;
	TBool iSignalled;
	};


NONSHARABLE_CLASS(CNotifyMediaChange) : public CActive
	{
public:
	CNotifyMediaChange(RLocalDrive* aDrive,TInt aSocketNo);
	void DoCancel() {};
	void RunL();
private:
	RLocalDrive* iDrive;
	TInt iSocket;
	};


const TInt KMaxDrivesPerSocket=16;

class LocalDrives
	{
public:
	static void Initialise();
	static TBusLocalDrive& GetLocalDrive(TInt aDriveNumber);
	static CExtProxyDrive* GetProxyDrive(TInt aDrive);
	static TInt InitProxyDrive(CFsRequest* aRequest);
	static TInt MountProxyDrive(CFsRequest* aRequest);
	static TInt DismountProxyDrive(TInt iDriveNumber);
	static TBool IsValidDriveMapping(TInt aDrvNumber);
	static TInt DriveNumberToLocalDriveNumber(TInt aDrvNumber);
	static TInt SetDriveMappingL(CFsRequest* aRequest);
	static void CompleteNotifications(TInt aSocket);
	static void CompleteDriveNotifications(TInt aDriveNumber);
	static TInt GetLocalSocket(TInt aControllerRelativeSocket, TMediaDevice aMediaType);
	static TInt GetDriveFromLocalDrive(TInt aLocDrv);
	static TInt GetLocalDriveNumber(TBusLocalDrive* aLocDrv);
	static TBool IsProxyDrive(TInt aDrive);
	static void ClearProxyDriveMapping(TInt aDrive);
	static TBool IsProxyDriveInUse(CExtProxyDriveFactory* aDevice);
	static TInt SetupMediaChange(TInt aDrive);
	static void NotifyChangeCancel(TInt aDrive);
private:
	static void InitDriveMapping();
	static TInt SwapDriveMapping(TInt aFirstDrive,TInt aSecondDrive);
private:
	class TSocketDesc
		{
	public:
		TInt iDriveNumbers[KMaxDrivesPerSocket];	// drive numbers assigned to this socket
		CNotifyMediaChange* iMediaChanges;
		TMediaDevice iMediaType;
		TInt iControllerRelativeSocket;
		};
	static TSocketDesc iSocketDescs[KMaxPBusSockets];
	static TBusLocalDrive iLocalDrives[KMaxLocalDrives];			
	static TInt iMapping[KMaxDrives];		// maps drive to local drive
	static TBool iMappingSet;
	static TInt iReverseMapping[KMaxLocalDrives];// opposite mapping of iMapping. local drive to drive (1 to [potentially] many)
	// i.e. LocalDrive1 -> DriveX
	//		LocalDrive2 -> DriveX
	static TBool iIsMultiSlotDrive[KMaxDrives]; // index is drive number
	static CExtProxyDrive* iProxyDriveMapping[KMaxProxyDrives];
	
	friend void TFsAddCompositeMount::AddFsToCompositeMountL(TInt aDriveNumber, CFileSystem& aFileSystem, TInt aLocalDriveNumber);
	friend void TDrive::MultiSlotDriveCheck(); // for dual/multi slot drive map swapping. (iMapping/iReverseMapping)
	};

NONSHARABLE_CLASS(CLogon) : public CActive
	{
public:
	enum {EPriority=3000};
public:
	static CLogon* NewL();
	TInt Logon(RThread aThread);
	virtual void DoCancel();
	virtual void RunL();
protected:
	CLogon(TInt aPriority);
private:
	RThread iThread;
	};

#ifdef __LAZY_DLL_UNLOAD
NONSHARABLE_CLASS(CLazyUnloadTimer): public CTimer
	{
public:
	CLazyUnloadTimer();
	~CLazyUnloadTimer();
	static void New();
	static void Finish();
private:
	void RunL();
	void Start();
	};
#endif

NONSHARABLE_CLASS(CSessionLoader) : public CSession2
	{
private:
	virtual void ServiceL(const RMessage2& aMessage);
	TInt DeleteExecutable(const TDesC& aName);
	};

NONSHARABLE_CLASS(CServerLoader) : public CServer2
	{
public:
	enum {EPriority=2000};
public:
	static CServerLoader* New();
	virtual CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMessage) const;
	virtual TInt RunError(TInt aError);
private:
	CServerLoader(TInt aPriority);
	};

class RLoaderMsg : public RMessage2
	{
public:
	TInt GetString(HBufC8*& aBuf, TInt aParam, TInt aMaxLen, TInt aHeadroom, TBool aReduce) const;
	TInt GetLdrInfo(TLdrInfo& aInfo) const;
	TInt UpdateLdrInfo(const TLdrInfo& aInfo) const;
	};

 void Fault(TFsFault aFault);

class CSessionFs;
class TOperation;
class CRequestThread;

typedef TInt (*TFsRequestFunc)(CFsRequest*);

class TThreadTimer
	{
public:
	TThreadTimer(TInt (*aCallBackFunction)(TAny* aPtr),TAny* aPtr);

	void Start(CRequestThread* aRequestThread, TTimeIntervalMicroSeconds32 aTime);
	void Stop();

private:
	TTickCountQueLink iLink;
	TCallBack iCallBack;
	CRequestThread* iRequestThread;	// the thread the timer is running on, NULL if timer not running

	friend class CFsDeltaTimer;
	};

// Class adapted from CDeltaTimer (but unlike CDeltaTimer, not derived from CActive).
// Used to support multiple timer events in drive threads.
NONSHARABLE_CLASS(CFsDeltaTimer) : public CBase
	{
public:
	static CFsDeltaTimer* New(CRequestThread& aRequestThread, TInt aPriority);
	~CFsDeltaTimer();

	void Start(TThreadTimer& aEntry, TTimeIntervalMicroSeconds32 aTime);
	void Stop(TThreadTimer& aEntry);

	void RunL();
private:
	CFsDeltaTimer(CRequestThread& aRequestThread, TInt aPriority, TInt aTickPeriod);
	void Cancel();
	void Activate();
	TInt QueueLong(TTimeIntervalMicroSeconds aTimeInMicroSeconds, TThreadTimer& aEntry);
private:	
	CRequestThread& iRequestThread;
	TThreadId iThreadId;
	RTimer iTimer;
	TTickCountQue iQueue;
	const TInt iTickPeriod;
	TBool iQueueBusy;
	RFastLock iLock;
	TBool iRestartNeeded;	// timer needs to be restarted as it was scheduled from a different thread
public:
	TRequestStatus iStatus;
	};


NONSHARABLE_CLASS(CRequestThread) : public CBase
	{
public:
	TInt ThreadFunction();
	void DeliverBack(CFsRequest* aRequest, TBool aLowPriority = EFalse);
	void DeliverFront(CFsRequest* aRequests);

	~CRequestThread();

	CFsDeltaTimer* Timer();

	void CompleteAllRequests(TInt aValue);
protected:
	CRequestThread();
	TInt DoStart(RThread& aThread);
	inline TInt Initialise();
	void Deliver(CFsRequest* aRequest,TBool aIsFront, TBool aLowPriority = EFalse);
	void Receive();
	virtual TInt DoThreadInitialise();
protected:
	CFsRequest* iRequest;
	TDblQue<CFsRequest> iList;
	RFastLock iListLock;
	TBool iIsWaiting;
	TBool iExit;
	RThread iThread;
	CFsDeltaTimer* iTimer;
	TBool iLowPriority;		// if true, drive thread's priority has been (temporarily) reduced
	friend class CFsDeltaTimer;
	};




NONSHARABLE_CLASS(CDriveThread) : public CRequestThread
	{
public:
	void CompleteReadWriteRequests();
	void CompleteClientRequests(TInt aValue);
	TBool IsRequestWriteable();
	TBool IsSessionNotifyUser();
private:
	CDriveThread();
	static CDriveThread* NewL();
	TUint StartL(TInt aDrvNumber);
	TInt DoThreadInitialise();

	void StartFinalisationTimer();
	void StopFinalisationTimer();

	static TInt FinaliseTimerEvent(TAny* aFileCache);
private:
	TInt iDriveNumber;
	TThreadTimer iFinaliseTimer;

friend class FsThreadManager;
	};

class CFsInternalRequest;

class CFsPlugin;
NONSHARABLE_CLASS(CPluginThread) : public CRequestThread
	{
public:
	CPluginThread(CFsPlugin& aPlugin, RLibrary aLibrary);
	~CPluginThread();
	
	/** @prototype */
	void OperationLockWait();

	/** @prototype */
	void OperationLockSignal();

private:
	static CPluginThread* NewL(CFsPlugin& aPlugin, RLibrary aLibrary);
	TUint StartL();
	virtual TInt DoThreadInitialise();
private:
	CFsPlugin& iPlugin;

	/** @prototype */
	RSemaphore iOperationLock;

	RLibrary iLib;	// contains a handle to the library	which created the plugin
friend class FsPluginManager;
	};

class TFsDriveThread
	{
public:
	TFsDriveThread();
public:
	RMutex iFSLock;
	TBool iIsAvailable;
	TBool iIsSync;
	CDriveThread* iThread;
	TUint iId;
	TBool iIsHung;				// drive is hung waiting for a critical notifier
	TBool iMediaChangePending;	// media change is pending while hung
	};


class FsThreadManager
	{
public:
//
	static void SetMainThreadId();
	static TBool IsMainThread();
//
	static TInt InitDrive(TInt aDrvNumber,TBool aIsSync);
	static TInt ChangeSync(TInt aDrvNumber,TBool aIsSync);

	static TInt GetDriveThread(TInt aDrvNumber, CDriveThread** aDrvThread);
	static void CloseDrive(TInt aDrvNumber);
	static TBool IsDriveThread(TInt aDrvNumber,TBool aLock);
	static TBool IsDriveSync(TInt aDrvNumber,TBool aLock);
	static TBool IsDriveAvailable(TInt aDrvNumber,TBool aLock);
	static void LockDrive(TInt aDrvNumber);
	static void UnlockDrive(TInt aDrvNumber);			
	static void SetDriveHung(TInt aDrvNumber, TBool aIsHung);
	static TBool IsDriveHung(TInt aDrvNumber);
	static void SetMediaChangePending(TInt aDrvNumber);
	static void StartFinalisationTimer(TInt aDriveNumber);
	static void StopFinalisationTimer(TInt aDriveNumber);
private:
	inline static TFsDriveThread& GetFsDriveThread(TInt aDrvNumber) {return(iFsThreads[aDrvNumber]);}
private:
	static TFsDriveThread iFsThreads[KMaxDrives];
	static TUint iMainId;
	static TUint iDisconnectThreadId;
	};


const TInt KReservedDriveAccessArrayGranularity = 2;

class TReservedDriveAccess
	{
public:
	inline TReservedDriveAccess(TInt aDriveNumber);
	inline TReservedDriveAccess(TInt aDriveNumber, TInt aReservedSpace);
private:
	TReservedDriveAccess();
public:
	TInt iDriveNumber;
	TInt iReservedSpace;
	TInt iReservedAccess;
	};


class CFsMessageRequest;
NONSHARABLE_CLASS(CSessionFs) : public CSession2
	{
public:
	static CSessionFs* NewL();
	virtual void CreateL();

	inline void Open();
	void Close();

	TInt CurrentDrive();
	void ServiceL(const RMessage2& aMessage);
	TInt CountResources();
	void ResourceCountMarkStart();
	void ResourceCountMarkEnd(const RMessage2& aMessage);
	TBool GetNotifyUser();
	void SetNotifyUser(TBool aNotification);
	TBool IsChangeNotify();
	void SetSessionFlags(TUint32 aBitsToSet, TUint32 aBitsToClear);
	TBool TestSessionFlags(TUint32 aFlags);
	void CloseRequestCountInc();
	void CloseRequestCountDec();
	
	//
	virtual void Disconnect(const RMessage2& aMessage);
	//
	inline void IncResourceCount();
	inline void DecResourceCount();
	inline CFsObjectIx& Handles();
	inline HBufC& Path();
	inline void SetPath(HBufC* aPath);
	inline TThreadId& ThreadId();
	inline void SetThreadId(const TThreadId& aId);
	//
	TUint Reserved(TInt aDriveNumber) const;
	TInt SetReserved(const TInt aDriveNumber, const TInt aReservedValue);
	TBool ReservedAccess(TInt aDriveNumber) const;
	void SetReservedAccess(const TInt aDriveNumber, const TBool aReservedAccess);
private:
	CSessionFs();
	~CSessionFs();

private:
	TInt iResourceCountMark;
	TInt iResourceCount;
	TInt iSessionFlags;
	RFastLock iSessionFlagsLock;
	CFsObjectIx* iHandles;
	HBufC* iPath;
	RArray<TReservedDriveAccess> iReservedDriveAccess;
	TThreadId iId;
	TInt iCloseRequestCount;	// number of close requests owned by this sessions on the RequestAllocator close queue
	TInt iAccessCount;
	RMessage2 iMessage;		// message passed to CSessionFs::Disconnect()
	};

NONSHARABLE_CLASS(CServerFs) : public CServer2
	{
public:
	enum {EPriority=1000};
public:
	virtual ~CServerFs();
	static void New();
	virtual CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMessage) const;
	CSessionFs* operator[](TInt anIndex);
	inline void SessionQueueLockWait();
	inline void SessionQueueLockSignal();
protected:
	// from CServerFs
	void RunL();

private:
	CServerFs(TInt aPriority);
	// lock to protect iSessionQ
	RFastLock iSessionQueueLock;
	};

NONSHARABLE_CLASS(CActiveSchedulerFs) : public CActiveScheduler
	{
public:
	static void New();
	virtual void Error(TInt anError) const;
	};

NONSHARABLE_CLASS(CActiveSchedulerLoader) : public CActiveScheduler
	{
public:
	static CActiveSchedulerLoader* New();
	virtual void Error(TInt anError) const;
	};


class TNameChecker
	{
public:
	TNameChecker(const TDesC& aPathName): iName(aPathName){};
	TBool IsIllegalChar(TText& aChar);
	TBool IsIllegalName();
	TBool IsIllegalName(TText& aChar);
	TBool IsIllegalPath();
	TBool IsIllegalPath(TText& aChar);
	void SetName(const TDesC& aName){iName=aName;};
private:
	TBool IsIllegal(TText& aChar) const;
private:
	TFileName iName;
	TParse iParse;
	};

enum TOperationFlags
		{
		ESync = 0x01, 
		EInternalRequest = 0x02,	// NB Not really used!
		EParseSrc = 0x04, 
		EParseDst = 0x08,
		EFileShare = 0x10,			// Operates on an open file share. NB not currently used
		EFsDspObj = 0x20,			// Bottom 32 bits of scratch value is a CFsDispatchObject
		};

class TOperation
	{
public:
	TBool IsChangeNotify() const;
	TBool IsDiskSpaceNotify() const;
	TBool IsWrite() const;
	TUint NotifyType() const;
	TBool IsCompleted() const;
	TBool IsOpenSubSess() const; //used to allocate for close as well as open task for subsessions
	TBool IsCloseSubSess() const;

	inline TBool IsSync() const;
	inline TInt Function();
	inline TInt Initialise(CFsRequest* aRequest);
	inline TInt PostInitialise(CFsRequest* aRequest);
	inline TInt DoRequestL(CFsRequest* aRequest);
	inline TFsPluginRequest::TF32ArgType Arg(TUint aIndex);
	
public:
	TInt iFunction;
	TUint iFlags;
	TFsRequestFunc iInitialise;
	// optional processing step. Runs initially in context of main file server thread
	// but may be executed again in drive thread context if request is postponed
	TFsRequestFunc iPostInitialise;
	TFsRequestFunc iDoRequestL;
	TUint32 iArgs;
	};

class TMsgOperation
	{
public:
	inline void Set(TInt64 aPos, TInt aLength, TUint8* aData, TInt aOffset = 0, TInt aNextState = 0);
	inline void Set(TInt64 aPos, TInt aLength, TDesC8* aData, TInt aOffset = 0, TInt aNextState = 0);
public:
	
	typedef struct
		{
		TAny* iData;	// pointer to local buffer (TUint8*) or remote client decriptor (TDesC8*)
		TInt iTotalLength;
		TInt64 iPos;
		TInt iLength;	// length of current fair-scheduled read/write 
		TInt iOffset;	// offset into iData
		} SReadWriteArgs;

	union
		{
		TInt iArgs[KMaxMessageArguments];
		SReadWriteArgs iReadWriteArgs;
		};

	TInt iFunction;				// the current function (usually, but not always the same as TOperation.iFunction
	TFsRequestFunc iComplete;	// function to call when CFsMessageRequest::Complete() is called
	CFsPlugin* iCurrentPlugin;	// The current plugin at the stage this Operation was pushed

	TBool iClientRequest;		// The current request originated through RFile/RFilePlugin, so uses a descriptor buffer: 
								// An EFSRV client request 		non-local handle	non-local buffer	descriptor
								// A plugin request				local handle		local buffer		descriptor
								// An internal (cache) request	local handle		local buffer		raw pointer
	TInt iState;
	TMsgOperation* iNext;
	TMsgOperation* iPrev;
	TBool iIsAllocated;
	TAny* iScratchValue0;
	TAny* iScratchValue1;
	TInt64 iClientPosition;
	};



NONSHARABLE_CLASS(CFsRequest) : public CBase
	{
public:
	/** Request states - these indicate a message's current stage of processing 
	*/
	enum TReqStates
		{
		/** Need to call iOperation.iInitialise() */
		EReqStateInitialise,
		/** Need to call PostInitialise() */
		EReqStatePostInitialise,
		/** Need to call iOperation.iDoRequestL */
		EReqStateDoRequest,
		};


	/** 
	Request actions - these indicate what further processing is required 
	and may be returned by any of:	

		CFsRequest::iOperation->Initialise()
		CFsPlugin::DoRequestL()
		CFsMessageRequest::iOperation->iPostInitialise()
		CFsRequest::iOperation->DoRequestL()
		CFsMessageRequest::iCurrentOperation->iComplete()

	The last 3 bits of each return code are used in CFsMessageRequest::Complete() 
	as an array look up to determine what action to take
	 */
	enum TReqActions 
		{
		/** 
		Continue with processing, dispatching to drive thread if necessary
		NB same value as KErrNone to maintain compatibility with existing code
		*/
		EReqActionContinue = KErrNone,			// 0x00000000, last 3 bits = 0

		/** 
		Complete and free the message 
		*/
		EReqActionComplete = KErrCompletion,		// 0xFFFFFFEF, last 3 bits = 7
		
		/** 
		A resource is in use, so dispatch message again to back of drive thread's request queue
		If returned by Initialise(), then Initialse() will be called again
		If returned by Complete(), then iPostInitialise() will be called again
		*/
		EReqActionBusy = -0x1002,				// 0xFFFFEFFE, last 3 bits = 6
		
		/** 
		Neither dispatch nor complete the message - 
		a plugin thread has taken ownership of the message
		*/
		EReqActionOwnedByPlugin = -0x1003,		// 0xFFFFEFFD, last 3 bits = 5

		/**
		The request cannot be processed because there is already an active read/write request 
		for the associated file share. This request has been linked to the currently active
		request and will be dispatched to the the drive thread when the current request has completed.
		@see CFileShare::RequestStart() & CFileShare::RequestEnd()
		*/
		EReqActionPending = EReqActionOwnedByPlugin,
		};
public:
	~CFsRequest();

	void ReadL(const TInt aMsgNum,TDes8& aDes);
	void ReadL(const TInt aMsgNum,TDes8& aDes,TInt anOffset);
	void ReadL(const TInt aMsgNum,TDes16& aDes);
	void ReadL(const TInt aMsgNum,TDes16& aDes,TInt anOffset);

	void WriteL(const TInt aMsgNum,const TDesC8& aDes);
	void WriteL(const TInt aMsgNum,const TDesC8& aDes,TInt anOffset);
	void WriteL(const TInt aMsgNum,const TDesC16& aDes);
	void WriteL(const TInt aMsgNum,const TDesC16& aDes,TInt anOffset);

	TInt Read(const TInt aMsgNum,TDes8& aDes);
	TInt Read(const TInt aMsgNum,TDes8& aDes,TInt anOffset);
	TInt Read(const TInt aMsgNum,TDes16& aDes);
	TInt Read(const TInt aMsgNum,TDes16& aDes,TInt anOffset);

	TInt Write(const TInt aMsgNum,const TDesC8& aDes);
	TInt Write(const TInt aMsgNum,const TDesC8& aDes,TInt anOffset);
	TInt Write(const TInt aMsgNum,const TDesC16& aDes);
	TInt Write(const TInt aMsgNum,const TDesC16& aDes,TInt anOffset);

	TInt GetDesLength(const TInt aMsgNum);

	inline void Kill(TInt aReason);
	inline void Terminate(TInt aReason);
	inline void Panic(const TDesC &aCategory,TInt aReason);
	inline TBool ErrorPlugin(TInt aReason);

	TInt Read(TFsPluginRequest::TF32ArgType aType, TInt& aVal);
	TInt Read(TFsPluginRequest::TF32ArgType aType, TUint& aVal);
	TInt Read(TFsPluginRequest::TF32ArgType aType, TInt64& aVal);
	//
	TInt Read(TFsPluginRequest::TF32ArgType aType, TDes8& aDes,  TInt aOffset = 0);
	TInt Read(TFsPluginRequest::TF32ArgType aType, TDes16& aDes, TInt aOffset = 0);
	//
	TInt Write(TFsPluginRequest::TF32ArgType aType, const TDesC8& aDes,  TInt aOffset = 0);
	TInt Write(TFsPluginRequest::TF32ArgType aType, const TDesC16& aDes, TInt aOffset = 0);

	inline TInt Initialise(){return iOperation->Initialise(this);};
	virtual void Process()=0;
	virtual void Complete(TInt aError)=0;
	virtual void Dispatch()=0;
	virtual void Free()=0;
	//
	virtual TParse& Src();
	virtual TParse& Dest();
	virtual TDrive* Drive();
	virtual TDrive* SubstedDrive();
	virtual void SetDrive(TDrive* aDrive);
	virtual void SetSubstedDrive(TDrive* aDrive);
	virtual const RMessage2& Message();
	//
	inline TOperation* Operation();
	inline CSessionFs* Session();
	inline void SetSession(CSessionFs* aSession);
	inline TInt DriveNumber();
	inline void SetDriveNumber(TInt aDriveNumber);
	inline TBool IsCompleted();
	inline void SetCompleted(TBool aIsCompleted);
	inline TUint ScratchValue();
	inline void SetScratchValue(const TUint aValue);
	inline TInt64 ScratchValue64();
	inline void SetScratchValue64(const TInt64& aValue);
	inline TBool IsSeparateThread();
	inline TBool IsPostOperation() const;
	inline TBool IsPluginSpecific() const;
	inline TBool IsExpectedResult(TInt err) const;
	inline TBool IsChangeNotify() const;
	inline void SetState(TReqStates aReqState);
	inline TBool DirectToDrive();
	inline TBool IsDescData(TInt aMsgNum);
	inline TInt FsFunction();
	void Close();	// close the session & dispatch object this request is using

public:
	CFsRequest();
protected:
	inline void Set(const TOperation& aOperation,CSessionFs* aSession);
	inline void Set(CSessionFs* aSession);
	inline void SetError(TInt aError);
	inline TInt GetError() const;
	inline void SetPostOperation(TBool aSet);


	void OpenDispatchObject(const TInt64& aValue);	// open the dispatch object this request is using
	void CloseDispatchObject();
	void OpenSession(CSessionFs* aSession);			// open the session this request is using
	void CloseSession();

private:
	TInt GetSlot(TFsPluginRequest::TF32ArgType aType);
public:
	TDblQueLink iLink;
	CFsPlugin* iCurrentPlugin;
	CFsPlugin* iOwnerPlugin;	// the plugin which originated this request
	TThreadId iClientThreadId;
	TBool iDirectToDrive;
protected:
	CSessionFs* iSession;
	TOperation* iOperation;
	TInt iDriveNumber;
	TBool iIsCompleted;
	TInt64 iScratchValue;
	
	TReqStates iReqState;

	/** defines for iFlags
	*/
	enum TFsRequestFlags
		{
		EIsAllocated			= 0x01,
		EFreeChanged			= 0x02,		// valid only for EFsFileWrite
		EPostInterceptEnabled	= 0x04,
		EPostOperation			= 0x08,
		EFsDspObjOpen			= 0x10,		// scratch value (a CFsDispatchObject) has been opened
		};
	TUint iFlags;

	TInt iError;
	};

const TInt KOperationFunctionUnaltered = -1;

NONSHARABLE_CLASS(CFsMessageRequest) : public CFsRequest
	{
public:
	void Set(const RMessage2& aMessage,CSessionFs* aSession);
	void Set(const RMessage2& aMessage,const TOperation& aOperation,CSessionFs* aSession);
	void Set(const TOperation& aOperation);
	inline void SetPostInitialise(TFsRequestFunc aCacheFunction);
	//
	inline void SetMessage(RMessage2& aMessage);
	//
	virtual void Process();
	virtual void Complete(TInt aError);

	virtual void Dispatch();
	virtual void Free();
	virtual TDrive* Drive();
	virtual TDrive* SubstedDrive();
	virtual void SetDrive(TDrive* aDrive);
	virtual void SetSubstedDrive(TDrive* aDrive);
	virtual const RMessage2& Message();

	inline TBool IsFreeChanged();
	inline void SetFreeChanged(TBool aChanged);

	inline TBool PostInterceptEnabled();
	inline void EnablePostIntercept(TBool aEnable);

	inline TBool IsAllocated();
	inline void SetAllocated();
	inline CFsMessageRequest();
	
	TInt PushOperation(TInt64 aPos, TInt aLength, TUint8* aData, TInt aOffset = 0, TFsRequestFunc aCallback = NULL, TInt aNextState = 0, TInt aFunction = KOperationFunctionUnaltered);
	TInt PushOperation(TInt64 aPos, TInt aLength, TDesC8* aData, TInt aOffset = 0, TFsRequestFunc aCallback = NULL, TInt aNextState = 0, TInt aFunction = KOperationFunctionUnaltered);
	TInt PushOperation(TFsRequestFunc aCallback = NULL, TInt aNextState = 0, TInt aFunction = KOperationFunctionUnaltered);
	void PopOperation();
	void SetOperationFunc(TInt aFunction);
	TMsgOperation& CurrentOperation();
	inline TMsgOperation* CurrentOperationPtr();
	void Dispatch(TBool aInitialise, TBool aLowPriority = EFalse, TBool aDispatchToFront = EFalse);

	inline TInt& LastError();
	inline void SetLastError(TInt aLastError);
	inline void Init();
	void ReStart();
	TBool IsPluginRequest();
	static inline CFsMessageRequest* RequestFromMessage(const RMessagePtr2& aMessage);
	
   // UID of the process to touching the file. (To be used in notification framework).
   // TUid iUID;
private:
	void DoNotify(TInt aError);
	void DoNotifyDiskSpace(TInt aError);
	TInt DoInitialise();
	TInt PostInitialise();
	TBool DispatchToPlugin();
	void ProcessPostOperation();
	void ProcessPreOperation();
	void ProcessDriveOperation();
	TBool CurrentPluginWaiting();
	inline TInt DispatchToDrive(TBool aLowPriority, TBool aDispatchToFront);
	TBool IsNotifierSpecific() const;
	TBool IsNotifierSupported() const;
protected:
	RMessage2 iMessage;
	TDrive* iDrive;
	TDrive* iSubstedDrive;
private:
	TMsgOperation* iCurrentOperation;
	TInt iLastError;
	};


NONSHARABLE_CLASS(TParsePool)
	{
private:
	enum {KEBlockSize = 4};

public:
	static TInt			Init();
	static TParsePool*	Get();
	static void			Release(TParsePool* aObject);
	TParsePool ();
	TParse& GetObject() {return   iObject;};

private:
	TParsePool*        iNext;
	TParsePool*        iPrev;

	TBool              iFree;
	TParse             iObject;
	static TParsePool* iFreeHead;
	static TParsePool* iClosedHead;
	static TInt        iCountFree;
	static RFastLock   iLock;
	};

	
NONSHARABLE_CLASS(CFsClientMessageRequest) : public CFsMessageRequest
	{
public:
    CFsClientMessageRequest();
	virtual TParse& Src();
	virtual TParse& Dest();
	virtual void Free();
	TInt AllocParseObjects(const TOperation& aOperation);
public:
	CFsClientMessageRequest* iNext;
protected:
	TParsePool* iPoolSrc;
	TParsePool* iPoolDest;
	};

NONSHARABLE_CLASS(CFsInternalRequest) : public CFsRequest
	{
public:
	CFsInternalRequest();
	void Set(const TOperation& aOperation,CSessionFs* aSession);
	//
	inline void SetThreadHandle(TInt aThreadHandle);
	inline TInt ThreadHandle();
	inline TRequestStatus& Status();
	inline TBool IsAllocated();
	inline void SetAllocated();
	//
	virtual void Process();
	virtual void Complete(TInt aError);
	virtual void Dispatch();
	virtual void Free();
private:
	TUint iThreadHandle;
	TRequestStatus iStatus;
	TBool iIsAllocated;
	};

// If the number of requests on the free queue reaches this value then completed requests 
// are returned to the heap rather than being added to thefree queue
const TInt KFreeCountMax = 64;

class TParseCon
	{
public:
	TParse iParse;
	TParseCon* iNext;
	};


class RequestAllocator
	{
public:
	static TInt Initialise();

	static TInt GetMessageRequest(const TOperation& aOperation,const RMessage2& aMessage,CFsClientMessageRequest* &aRequest);
	static void FreeRequest(CFsClientMessageRequest* aRequest);
	static void OpenSubFailed(CSessionFs* aSession); 

#if defined(_USE_CONTROLIO) || defined(_DEBUG) || defined(_DEBUG_RELEASE)
	inline static TInt RequestCount();
	inline static TInt RequestCountPeak();
	static TInt CloseCount();
	static TInt FreeCount();
#endif

private:
	static CFsClientMessageRequest* GetRequest();

private:
	static RFastLock iCacheLock;
	static CFsClientMessageRequest* iFreeHead;				
	static CFsClientMessageRequest* iCloseHead;

	static TInt iRequestCount;			// current number of requests
	static TInt iFreeCount;				// current number of requests on free queue
	static TInt iRequestCountPeak;				// maximum value of requests reached
	};

class OperationAllocator
	{
public:
	static TInt Initialise();

	static TInt GetOperation(TMsgOperation* &aOperation);
	static void FreeOperation(TMsgOperation* aOperation);

#if defined(_USE_CONTROLIO) || defined(_DEBUG) || defined(_DEBUG_RELEASE)
	inline static TInt RequestCount();
	inline static TInt RequestCountPeak();
	inline static TInt FreeCount();
#endif

private:
	static RFastLock iCacheLock;
	static TMsgOperation* iFreeHead;

	static TInt iRequestCount;			// current number of requests
	static TInt iFreeCount;				// current number of requests on free queue
	static TInt iRequestCountPeak;				// maximum value of requests reached
	};


NONSHARABLE_CLASS(CNotifyInfo) : public CBase
	{
public:
	
    enum TInfoType {EDiskSpace,EStdChange,EExtChange,EDebugChange,EDismount};

public:
	~CNotifyInfo();
	void Initialise(TInfoType aType,TRequestStatus* aStatus,const RMessagePtr2& aMessage,CSessionFs* aSession);
	void Complete(TInt aError);
	//
	inline CSessionFs* Session();
	inline TRequestStatus* Status();
	inline TInfoType Type() const {return(iType);}
public:
	TDblQueLink iLink;
protected:
	TInfoType iType;
	TRequestStatus* iStatus;
	RMessagePtr2 iMessage;
	CSessionFs* iSession;
	};


NONSHARABLE_CLASS(CStdChangeInfo) : public CNotifyInfo
	{
public:
	void Initialise(TNotifyType aChangeType,TRequestStatus* aStatus,const RMessagePtr2& aMessage,CSessionFs* aSession);
	TUint RequestNotifyType(CFsRequest* aRequest);
	TBool IsMatching(CFsRequest* aRequest);
protected:
	TNotifyType iChangeType;
	};

NONSHARABLE_CLASS(CExtChangeInfo) : public CStdChangeInfo
	{
public:
	void Initialise(TNotifyType aChangeType,TRequestStatus* aStatus,const RMessagePtr2& aMessage,CSessionFs* aSession,const TDesC& aName);
	TBool IsMatching(CFsRequest* aRequest);
private:
	TFileName iName;
	};

NONSHARABLE_CLASS(CDiskSpaceInfo) : public CNotifyInfo
	{
public:
	void Initialise(TRequestStatus* aStatus,const RMessagePtr2& aMessage,CSessionFs* aSession,TInt64 aThreshold);
	TBool IsMatching(TInt64& aBefore,TInt64& aAfter);
private:
	TInt64 iThreshold;
	};

NONSHARABLE_CLASS(CDebugChangeInfo) : public CNotifyInfo
	{
public:
	void Initialise(TUint aDebugType,TRequestStatus* iStatus,const RMessagePtr2& aMessage,CSessionFs* aSession);
	TBool IsMatching(TUint aChange);
private:
	TUint iDebugType;
	};

NONSHARABLE_CLASS(CDismountNotifyInfo) : public CNotifyInfo
	{
public:
	~CDismountNotifyInfo();
	void Initialise(TNotifyDismountMode aMode, TInt aDriveNumber,TRequestStatus* iStatus,const RMessagePtr2& aMessage,CSessionFs* aSession);
	TBool IsMatching(TNotifyDismountMode aMode, TInt aDriveNumber, CSessionFs* aSession);
	inline TInt DriveNumber() {return iDriveNumber;}
private:
	TNotifyDismountMode iMode;
	TInt iDriveNumber;
	};

const TInt KMaxNotifyQues=KMaxDrives+1;
const TInt KMaxDiskQues=KMaxDrives;	

class TBaseQue
	{
protected:
	TBaseQue();
	~TBaseQue();
	void DoAddNotify(CNotifyInfo* aInfo);
	TBool DoCancelSession(CSessionFs* aSession,TInt aCompletionCode,TRequestStatus* aStatus=NULL);
	void DoCancelAll(TInt aCompletionCode);
	CNotifyInfo* DoFindEntry(CSessionFs* aSession, TRequestStatus* aStatus=NULL);
	TBool IsEmpty();
protected:
	TDblQue<CNotifyInfo> iHeader;
	RFastLock iQLock;
	};

class TChangeQue :public TBaseQue
	{
public:
	TInt AddNotify(CNotifyInfo* aInfo);
	TBool CancelSession(CSessionFs* aSession,TInt aCompletionCode,TRequestStatus* aStatus=NULL);
	void CancelAll(TInt aCompletionCode);
	void CheckChange(CFsRequest* aRequest);
	TBool IsEmpty();
	};

class TDiskSpaceQue : public TBaseQue
	{
public:
	inline void SetDriveNumber(TInt aDriveNumber) {iDriveNumber=aDriveNumber;}
	TInt AddNotify(CNotifyInfo* aInfo);
	TInt CancelSession(CSessionFs* aSession,TInt aCompletionCode,TRequestStatus* aStatus=NULL);
	void CancelAll(TInt aCompletionCode);
	void CheckDiskSpace();
	void CheckDiskSpace(TInt64& aFreeDiskSpace);
	TBool IsEmpty();
private:
	TInt GetFreeDiskSpace(TInt64& aFreeDiskSpace);
private:
	TInt64 iFreeDiskSpace;
	TInt iDriveNumber;
	TInt64 iReservedDiskSpace;
	};

class TDebugQue : public TBaseQue
	{
public:
	TInt AddNotify(CNotifyInfo* aInfo);
	TInt CancelSession(CSessionFs* aSession,TInt aCompletionCode,TRequestStatus* aStatus=NULL);
	void CancelAll(TInt aCompletionCode);
	void CheckDebug(TUint aDebugChange);
	};

class TDismountNotifyQue : public TBaseQue
	{
public:
	TInt AddNotify(CNotifyInfo* aInfo);
	TInt CancelSession(CSessionFs* aSession,TInt aCompletionCode,TRequestStatus* aStatus=NULL);
	void CancelAll(TInt aCompletionCode);
	void CheckDismount(TNotifyDismountMode aMode, TInt aDrive, TBool aRemove, TInt aError);
	TBool HandlePendingDismount(CSessionFs* aSession, TInt aDrive);
	};

class FsNotify
	{
public:
	static void Initialise();
	static TInt AddChange(CNotifyInfo* aInfo,TInt aDrive);
	static TInt AddDiskSpace(CNotifyInfo* aDiskInfo,TInt aDrive);
	static TInt AddDebug(CNotifyInfo* aDebugInfo);
	static TInt AddDismountNotify(CNotifyInfo* aDismountNotifyInfo);
	static void DiskChange(TInt aDrive);
	static void HandleChange(CFsRequest* aRequest,TInt aDrive);
	static void HandleDiskSpace(CFsRequest* aRequest,TInt aDrive);
	static void HandleDiskSpace(TInt aDrive, TInt64& aFreeSpace);
	static void HandleDebug(TUint aFunction);
	static void HandleDismount(TNotifyDismountMode aMode, TInt aDrive, TBool aRemove, TInt err);
	static void CancelChangeSession(CSessionFs* aSession,TRequestStatus* aStatus=NULL);
	static void CancelDiskSpaceSession(CSessionFs* aSession,TRequestStatus* aStatus=NULL);
	static void CancelDebugSession(CSessionFs* aSession, TRequestStatus* aStatus=NULL);
	static TInt CancelDismountNotifySession(CSessionFs* aSession, TRequestStatus* aStatus=NULL);
	static void CancelSession(CSessionFs* aSession);
	static TBool HandlePendingDismount(CSessionFs* aSession, TInt aDriveNumber);
	static TBool IsChangeQueEmpty(TInt aDrive);
	static TBool IsDiskSpaceQueEmpty(TInt aDrive);
private:
	static TInt ChangeIndex(TInt aDrive);
private:
	static TChangeQue iChangeQues[KMaxNotifyQues];
	static TDiskSpaceQue iDiskSpaceQues[KMaxDiskQues];
	static TDebugQue iDebugQue;
	static TDismountNotifyQue iDismountNotifyQue;
	};

class CObjPromotion : public CFsObject
	{
public:
	TInt UniqueID() const {return(CFsObject::UniqueID());}
	};	

NONSHARABLE_CLASS(CKernEventNotifier) : public CActive
     {
public:
     static CKernEventNotifier* New(TInt aPriority=EPriorityStandard);
     ~CKernEventNotifier();
     void Start();
     inline TInt Change() const {return iChange;}
private:
     CKernEventNotifier(TInt aPriority) : CActive(aPriority) {}
     void RunL();
     void DoCancel();
private:
     static TInt LocaleChangeCallback(TAny* aPtr=NULL);
     TInt FreeMemoryChangeCallback();
private:
     TInt iChange;
     RChangeNotifier iChangeNotifier;
     };

#if defined(_LOCKABLE_MEDIA)

class TDelayedWriterInit
	{
public:
	const TDesC *iFileName;
	const TDesC8 *iData;
	const TDesC *iSemName;
	};


class TDelayedWriter
	{
public:
	TDelayedWriter();
	~TDelayedWriter();

	static TDelayedWriter *NewL(const TDelayedWriterInit *dwi);
	void ConstructL(const TDelayedWriterInit *dwi);

	HBufC *iFileName;
	HBufC8 *iData;
	};

#endif

extern CFsObjectConIx* TheContainer;
extern CFsObjectCon* FileSystems;
extern CFsObjectCon* Files;
extern CFsObjectCon* FileShares;
extern CFsObjectCon* Dirs;
extern CFsObjectCon* Formats;
extern CFsObjectCon* RawDisks;
extern CFsObjectCon* Extensions;
extern CFsObjectCon* ProxyDrives;

extern CKernEventNotifier* TheKernEventNotifier;

extern RThread TheServerThread;
extern RAllocator* ServerThreadAllocator;

extern CServerFs* TheFileServer;

extern HBufC* TheDriveNames[];
extern TDrive TheDrives[KMaxDrives];
extern TFileName TheDefaultPath;

extern SCapabilitySet AllCapabilities;
extern SCapabilitySet DisabledCapabilities;

const TInt KDispatchObjectClose=KMaxTInt-1;
const TInt KSessionInternalReserved2=KMaxTInt-2;	// not used any more - placeholder
const TInt KSessionInternalReserved3=KMaxTInt-3;	// not used any more - placeholder
const TInt KSessionInternalReserved4=KMaxTInt-4;	// not used any more - placeholder
const TInt KFileShareClose=KMaxTInt-5;
const TInt KFlushDirtyData=KMaxTInt-6;

const TOperation DispatchObjectCloseOp=	{KDispatchObjectClose,	EInternalRequest,	&TFsCloseObject::Initialise,		NULL,	&TFsCloseObject::DoRequestL			};
const TOperation FileShareCloseOp=		{KFileShareClose,		EInternalRequest,	&TFsCloseFileShare::Initialise,		NULL,	&TFsCloseFileShare::DoRequestL		};

extern TBool OpenOnDriveZOnly;
extern TBool LocalFileSystemInitialized;
extern TBool StartupInitCompleted;
extern TBool RefreshZDriveCache;
extern TBool CompFsMounted;
extern TBool CompFsSync;

 TInt InitializeLocalFileSystem(const TDesC& aName);
 void InstallRomFileSystemL();
 void InstallFatFileSystemL();
 TInt InstallFileSystem(CFileSystem* aSys,RLibrary aLib);

 TInt LoadFileSystem(const TDesC& aName);

 CFsObject* SessionObjectFromHandle(TInt aHandle,TInt aUniqueID, CSessionFs* aSession);
 CFileShare* GetShareFromHandle(CSessionFs* aSession, TInt aHandle);

 TInt  ValidateDrive(TInt aDriveNumber,CFsRequest* aRequest);
 TInt  ValidateDriveDoSubst(TInt aDriveNumber,CFsRequest* aRequest);
 void  ValidateAtts(TUint& aSetAttMask,TUint& aClearAttMask);
 TInt  ParseSubstPtr0(CFsRequest* aRequest,TParse& aParse, TBool aUseSessionPath = ETrue);
 TInt  ParseNoWildSubstPtr0(CFsRequest* aRequest,TParse& aParse, TBool aUseSessionPath = ETrue);
 TInt  ParseNoWildSubstPtr1(CFsRequest* aRequest,TParse& aParse);
 TInt  ParseNoWildSubstCheckPtr0(CFsRequest* aRequest,TParse& aParse, TBool aUseSessionPath = ETrue);
 TInt  ParseNoWildSubstCheckPtr1(CFsRequest* aRequest,TParse& aParse);
 TInt  ParseNoWildSubstFileCheckPtr1(CFsRequest* aRequest,TParse& aParse);
 TInt  ParseNoWildSubstCheckPathPtr0(CFsRequest* aRequest,TParse& aParse);
 TInt  ParseNoWildSubstCheckPathPtr1(CFsRequest* aRequest,TParse& aParse);
 TInt  ParsePathPtr0(CFsRequest* aRequest,TParse& aParse);
 TInt  ParseNotificationPath(CFsRequest* aRequest, TParse& aParse, TDes& aNotifyPath);
 TBool IsIllegalFullName(const TDesC& aName);
 TBool IsIllegalFullName(const TParse& aParse);
 void  AddResource(CMountCB& aMount);
 void  RemoveResource(CMountCB& aMount);
 void  AddDiskAccess(CMountCB& aMount);
 void  RemoveDiskAccess(CMountCB& aMount);
 void  NextInPath(const TDesC& aPath,TPtrC& anEntry,TInt& aPos);
 TBool PowerOk();
 void  Get16BitDllName(TDes& aFileName, const TDesC8& aDllName);
 void  Get8BitDllName(TDes8& aDllName, const TDesC& aFileName);
 TInt  MatchUidType(const TUidType &aMatch, const TUidType &aType);
 TBool IsWriteFunction(TInt aFunction);
 TInt  InitLoader();
 TInt  CheckDiskSpace(TInt64 aThreshold, CFsRequest* aRequest);
 void  CheckForLeaveAfterOpenL(TInt leaveError, CFsRequest* aRequest, TInt aHandle);
 TInt  ParseSubst(const TInt aP, CFsRequest* aRequest,TParse& aParse, TBool aUseSessionPath = ETrue);

//#ifdef __DATA_CAGING__

 TBool CompareResource(const TDesC & aThePath);

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
 TInt PathCheck(CFsRequest* aRequest, const TDesC& aThePath, const TSecurityPolicy* aSysCap, const TSecurityPolicy* aPriCap, const char* aDiag);
 TInt PathCheck(CFsRequest* aRequest, const TDesC& aThePath, const TSecurityPolicy* aSysCap, const TSecurityPolicy* aPriCap, const TSecurityPolicy* aROCap, const char* aDiag);
 TInt PathCheck(CFsRequest* aRequest, const TDesC& aThePath,const TSecurityPolicy* aCap, const char* aDiag, TBool aExactMatchAllowed = EFalse);
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
 TInt PathCheck(CFsRequest* aRequest, const TDesC& aThePath, const TSecurityPolicy* aSysCap, const TSecurityPolicy* aPriCap, OnlyCreateWithNull aDiag);
 TInt PathCheck(CFsRequest* aRequest, const TDesC& aThePath, const TSecurityPolicy* aSysCap, const TSecurityPolicy* aPriCap, const TSecurityPolicy* aROCap, OnlyCreateWithNull aDiag); 
 TInt PathCheck(CFsRequest* aRequest, const TDesC& aThePath,const TSecurityPolicy* aCap, OnlyCreateWithNull aDiag, TBool aExactMatchAllowed = EFalse);
#endif //!__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

 TBool ComparePrivate(const TDesC & aThePath);
 TBool CompareSystem(const TDesC & aThePath);
 TBool SIDCheck(CFsRequest* aRequest, const TDesC& aThePath);
 TBool ComparePaths(const TDesC& aPath1,const TDesC& aPath2);
 TUint32 CalcNameHash(const TDesC& aName);

const TInt KResourceLength			=	9;
const TInt KSystemLength			=	4;
const TInt KPrivateLength			=	8;
const TInt KPrivateLengthCheck		=	17;
const TInt KSIDLength				=	8;
const TInt KSIDPathOffset			=	9;

_LIT(KSlash, "\\");
_LIT(KPrivate,"\\Private");
_LIT(KPrivateSlash,"\\Private\\");
_LIT(KSysHash,"?:\\Sys\\Hash\\");
const TInt KBSlash='\\';
const TInt KHashFileReadSize = 1024*8;


const TInt KMsgPtr0	= 0;
const TInt KMsgPtr1	= 1;
const TInt KMsgPtr2	= 2;
const TInt KMsgPtr3	= 3;

const TInt KIpcFunctionMask = 0x0000ffff;
const TInt KIpcFlagMask     = 0xffff0000;
const TInt KIpcFlagOffset   = 16;

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
 void  PrintStartUpReason(TMachineStartupType aReason);
 TPtrC GetFunctionName(TInt aFunction);
 void  SimulateAllocFailure(TInt aFunctionReturnValue,TInt aFunction);

extern TInt ErrorCondition;
extern TInt ErrorCount;
extern TUint32 DebugReg;
extern TInt UserHeapAllocFailCount;
extern TInt KernHeapAllocFailCount;
extern TInt MessageCount;
extern TInt SessionCount;
extern TInt ObjectCount;

void PrintHeapSize(const TDesC& aMessage);

extern TCorruptNameRec* gCorruptFileNameList;
extern TCorruptLogRec* gCorruptLogRecordList;
extern TInt gNumberOfCorruptHits;
extern HBufC* gCorruptFileNamesListFile;
#endif

typedef TPckgBuf<TMediaPswdReplyNotifyInfoV1> TMediaPswdReplyNotifyInfoV1Buf;

enum TDllFindMethod {EFindInPath, EFindInSystemLibs, EFindInSystemBin, EFindExhausted};

//---------------------------------------------------------------------------------------------------------------------

NONSHARABLE_CLASS(CFileBody) : public CBase, public CFileCB::MExtendedFileInterface
	{
protected:
	~CFileBody();
	CFileBody(CFileCB* aFileCB, CFileCB::MExtendedFileInterface* aExtendedFileInterface);
	void InitL();

	TBool ExtendedFileInterfaceSupported();

	// from MExtendedFileInterface	
	virtual void ReadL(TInt64 aPos,TInt& aLength,TDes8* aDes,const RMessagePtr2& aMessage, TInt aOffset = 0);
	virtual void WriteL(TInt64 aPos,TInt& aLength,const TDesC8* aDes,const RMessagePtr2& aMessage, TInt aOffset = 0);
	virtual void SetSizeL(TInt64 aSize);


private:
	CFileCB* iFileCB;
	CFileCB::MExtendedFileInterface* iExtendedFileInterface;
	TInt iFairSchedulingLen;
	TBool iNotifyAsyncReadersPending;
	TBool iDeleteOnClose;
	TDblQue<CFileShare> iShareList;	// A list containing the CFileShare objects associated with the file

protected:
	TInt iPromotedShares;
	RArray<TAsyncReadRequest>* iAsyncReadRequests;
	CFileCache* iFileCache;	// pointer to owner CFileCache 
	TBool iLocalBufferSupport;

    /** 
    maximum file size supported by the filesystem that instantiates the CFileCB, associated with this object.
    For example, FAT32 supports files not larger than 4GB-1. Other file systems can support larger files.
    This member allows file server to know maximum allowed position in the file.
    The default value is KMaxTUint64
    */
    TUint64 iMaxSupportedFileSize;
    
    TInt iNonSequentialFileModes;	// Count of clients without the 'Sequential' mode enabled
	TBool iSequential;				// Indicates whether the file is in 'Sequential' mode

public:
	// Provides support for large file size ( file size > 4GB - 1)
	// Upper / High 32 bit word of the file size is saved here.
	// This can be non-zero only if the CFileShare::iMode is ORed with EFileBigFile
	// and the file size is > 4GB - 1.
	// This shall be queried by CFileCB::Size64() from file systems or by direct access from file server
	// This shall be updated by CFileCB::SetSize64() from file systems or by direct access from file server. 
	TUint iSizeHigh;
	
protected:
friend class CFileCB;
friend class CFileCache;
	};

//---------------------------------------------------------------------------------------------------------------------

NONSHARABLE_CLASS(CMountBody) : public CBase, public CMountCB::MFileAccessor, public CMountCB::MFileExtendedInterface
	{
protected:
	CMountBody(CMountCB* aMountCB, CMountCB::MFileAccessor* aFileAccessor = NULL, CMountCB::MFileExtendedInterface* aFileInterface = NULL);
	~CMountBody();

	// Clamping support
	TInt ClampFile(const TInt aDriveNo,const TDesC& aName,TAny* aHandle);
	TInt UnclampFile(RFileClamp* aHandle);
	TInt IsFileClamped(const TInt64 aUniqueId);
	TInt NoOfClamps();

	// Internal support clamping
	static TInt CompareClampsByIdAndCount(const RFileClamp& aClampA, const RFileClamp& aClampB);
	static TInt CompareClampsById(const RFileClamp& aClampA, const RFileClamp& aClampB);
	static TInt FindClampByIdAndCount(const RFileClamp& aClampA, const RFileClamp& aClampB);
	static TInt FindClampById(const RFileClamp& aClampA, const RFileClamp& aClampB);

	// from MFileAccessor
	virtual TInt GetFileUniqueId(const TDesC& aName, TInt64& aUniqueId);
	virtual TInt Spare3(TInt aVal, TAny* aPtr1, TAny* aPtr2);
	virtual TInt Spare2(TInt aVal, TAny* aPtr1, TAny* aPtr2);
	virtual TInt Spare1(TInt aVal, TAny* aPtr1, TAny* aPtr2);
	
	// From MFileExtendedInterface
	virtual void ReadSection64L(const TDesC& aName, TInt64 aPos, TAny* aTrg, TInt aLength, const RMessagePtr2& aMessage);

	void SetProxyDriveDismounted();
	TBool ProxyDriveDismounted();
	

    inline CFileSystem* GetFileSystem() const;
    inline void SetFileSystem(CFileSystem* aFsys);

private:
	CMountCB* iMountCB;
	RArray<RFileClamp> iClampIdentifiers;
	TInt32 iClampCount;
	CMountCB::MFileAccessor* iFileAccessor;
	CMountCB::MFileExtendedInterface* iFileExtendedInterface;
	TBool iProxyDriveDismounted;
	CFileSystem* iFSys;  ///< link to the FileSystem object that has produced the mount (iMountCB)
friend class CMountCB;
	};

// extension to CProxyDrive
class CProxyDriveBody : public CBase
	{
public:
	RLibrary iLibrary;
	};

#include "sf_ops.h"
#include "sf_std.inl"


#endif //SF_STD_H
