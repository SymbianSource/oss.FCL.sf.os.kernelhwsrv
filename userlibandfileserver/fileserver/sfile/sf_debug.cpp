// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_debug.cpp
// 
//

#include "sf_std.h"
#include <f32dbg.h>
#include "f32image.h"
#include <f32plugin.h>
#include <filesystem_fat.h>
#include "sf_file_cache.h"
#include "sf_memory_man.h"

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)

//
// ONLY INCLUDED IN DEBUG BUILDS
//

void PrintOpenFiles()
	{
	CCacheManager* manager = CCacheManagerFactory::CacheManager();
	TInt allocatedSegmentCount = manager ? manager->Stats().iAllocatedSegmentCount : 0;
	TInt lockedSegmentCount = manager ? manager->Stats().iLockedSegmentCount : 0;
	TInt fileCount = manager ? manager->Stats().iFileCount : 0;
	TInt filesOnClosedQueue = manager ? manager->Stats().iFilesOnClosedQueue : 0;

	RDebug::Print(_L("TRACE: Open files %d allocatedSegmentCount %d lockedSegmentCount %d fileCount %d filesOnClosedQueue %d\n"), 
		Files->Count(), allocatedSegmentCount, lockedSegmentCount, fileCount, filesOnClosedQueue);

	Files->Lock();
	TInt count=Files->Count();

	for (TInt n=0; n<count; n++)
		{
		CFileCB* file=(CFileCB*)(*Files)[n];

		RDebug::Print(_L("%3d: %C:%S fc %x sz %lx\n"), n, file->Drive().DriveNumber() + 'A', &file->FileNameF(), file->FileCache(), file->CachedSize64());
		}
	Files->Unlock();
	}

void PrintHeapSize(const TDesC& aMessage)
//
// Display the total memory available
//
	{

	TInt biggest;
	TInt heapSize = User::Allocator().Available(biggest);
	RDebug::Print(_L("%S size=0x%x largest cell=0x%x"),&aMessage,heapSize,biggest);
	}

static void SetAllocFailure(const RMessage2* aMessage)
//
// Set alloc failure after allocCount allocations
//
	{

	UserHeapAllocFailCount=aMessage->Int1();
	KernHeapAllocFailCount=UserHeapAllocFailCount;
	if (UserHeapAllocFailCount>=0)
		{
		__UHEAP_FAILNEXT(UserHeapAllocFailCount);
		__KHEAP_FAILNEXT(KernHeapAllocFailCount);
		}
	else
		{
		__UHEAP_RESET;
		__KHEAP_RESET;
		}	
	}

TInt FsDebugFunction(CFsRequest* aRequest)
//
// SetAllocFailure - Set alloc failure after allocCount allocations
// SetErrorCondition - Set simulated error failure
// SetDebugRegister - Trigger tracing output
// DebugNotify - Request notification of a file server event - .FSY specific
//
	{

	TInt r=KErrNone;
	const RMessage2* message=&(aRequest->Message());
	switch (message->Int0())
		{
	case EFsSetAllocFailure: 
		SetAllocFailure(message); 
		break;
	case EFsSetErrorCondition:
		ErrorCondition=message->Int1();
		ErrorCount=message->Int2();
		break;
	case EFsSetDebugRegister:
		DebugReg=message->Int1();
		break;
	case EFsDebugNotify:
		{
		TUint notifyType=(aRequest->Message().Int2())&KDebugNotifyMask;
		if (notifyType)
			{
			CDebugChangeInfo* info=new CDebugChangeInfo;
			if(!info)
				return(KErrNoMemory);
			const RMessage2& m=aRequest->Message();
			info->Initialise(notifyType,(TRequestStatus*)m.Ptr3(),m,aRequest->Session());
			r=FsNotify::AddDebug(info);
			if(r!=KErrNone)
				delete(info);
			else
				aRequest->SetCompleted(EFalse);
			}
		else
			r=KErrArgument;
		break;
		}
	default:
		break;
		};
	return(r);
	}

TBool SimulateError(const RMessage2* aMessage)
//
// Return an error message if ErrorCount<=0
//
	{
	TInt function = aMessage->Function() & KIpcFunctionMask;
	
	if (ErrorCondition!=KErrNone && 
		(
		function!=EFsDebugFunction || aMessage->Int0()!=EFsSetErrorCondition) &&
		function!=EFsNotifyChangeCancel &&
		function!=EFsNotifyChangeCancelEx &&
		function!=EFsNotifyDiskSpaceCancel &&
		function!=EFsFormatSubClose && 
		function!=EFsDirSubClose  && 
		function!=EFsFileSubClose && 
		function!=EFsRawSubClose &&
		function!=EFsUnclamp &&

        //-- this operation must not fail. It can only fail if the client's thread has died.
        //-- in this case whole session will be cleaned up.
		function!=EFsFileAdopt 
        )
		{
		if (ErrorCount<=0)
			return(ETrue);
		ErrorCount--;
		}
	return(EFalse);
	}


TPtrC GetFunctionName(TInt aFunction)
//
// Print number of alloc fails to complete a given function
//
	{

	switch (aFunction)
		{
	case EFsDebugFunction: return _L("EFsDebugFunction");
	case EFsAddFileSystem: return _L("EFsAddFileSystem");
	case EFsRemoveFileSystem: return _L("EFsRemoveFileSystem");
	case EFsMountFileSystem: return _L("EFsMountFileSystem");
	case EFsNotifyChange: return _L("EFsNotifyChange");
	case EFsNotifyChangeCancel: return _L("EFsNotifyChangeCancel");
	case EFsDriveList: return _L("EFsDriveList");
	case EFsDrive: return _L("EFsDrive");
	case EFsVolume: return _L("EFsVolume");
	case EFsSetVolume: return _L("EFsSetVolume");
	case EFsSubst: return _L("EFsSubst");
	case EFsSetSubst: return _L("EFsSetSubst");
	case EFsRealName: return _L("EFsRealName");
	case EFsDefaultPath: return _L("EFsDefaultPath");
	case EFsSetDefaultPath: return _L("EFsSetDefaultPath");
	case EFsSessionPath: return _L("EFsSessionPath");
	case EFsSetSessionPath: return _L("EFsSetSessionPath");
	case EFsMkDir: return _L("EFsMkDir");
	case EFsRmDir: return _L("EFsRmDir");
	case EFsParse: return _L("EFsParse");
	case EFsDelete: return _L("EFsDelete");
	case EFsRename: return _L("EFsRename");
	case EFsReplace: return _L("EFsReplace");
	case EFsEntry: return _L("EFsEntry");
	case EFsSetEntry: return _L("EFsSetEntry");
	case EFsGetDriveName: return _L("EFsGetDriveName");
	case EFsSetDriveName: return _L("EFsSetDriveName");
	case EFsFormatSubClose: return _L("EFsFormatSubClose");
	case EFsDirSubClose: return _L("EFsDirSubClose");
	case EFsFileSubClose: return _L("EFsFileSubClose");
	case EFsRawSubClose: return _L("EFsRawSubClose");
	case EFsFileOpen: return _L("EFsFileOpen");
	case EFsFileCreate: return _L("EFsFileCreate");
	case EFsFileReplace: return _L("EFsFileReplace");
	case EFsFileTemp: return _L("EFsFileTemp");
	case EFsFileRead: return _L("EFsFileRead");
	case EFsFileWrite: return _L("EFsFileWrite");
	case EFsFileLock: return _L("EFsFileLock");
	case EFsFileUnLock: return _L("EFsFileUnLock");
	case EFsFileSeek: return _L("EFsFileSeek");
	case EFsFileFlush: return _L("EFsFileFlush");
	case EFsFileSize: return _L("EFsFileSize");
	case EFsFileSetSize: return _L("EFsFileSetSize");
	case EFsFileAtt: return _L("EFsFileAtt");
	case EFsFileSetAtt: return _L("EFsFileSetAtt");
	case EFsFileModified: return _L("EFsFileModified");
	case EFsFileSetModified: return _L("EFsFileSetModified");
	case EFsFileSet: return _L("EFsFileSet");
	case EFsFileChangeMode: return _L("EFsFileChangeMode");
	case EFsFileRename: return _L("EFsFileRename");
	case EFsDirOpen: return _L("EFsDirOpen");
	case EFsDirReadOne: return _L("EFsDirReadOne");
	case EFsDirReadPacked: return _L("EFsDirReadPacked");
	case EFsFormatOpen: return _L("EFsFormatOpen");
	case EFsFormatNext: return _L("EFsFormatNext");
	case EFsRawDiskOpen: return _L("EFsRawDiskOpen");
	case EFsRawDiskRead: return _L("EFsRawDiskRead");
	case EFsRawDiskWrite: return _L("EFsRawDiskWrite");
	case EFsResourceCountMarkStart: return _L("EFsResourceCountMarkStart");
	case EFsResourceCountMarkEnd: return _L("EFsResourceCountMarkEnd");
	case EFsResourceCount: return _L("EFsResourceCount");
	case EFsCheckDisk: return _L("EFsCheckDisk");
	case EFsGetShortName: return _L("EFsGetShortName");
	case EFsGetLongName: return _L("EFsGetLongName");
	case EFsIsFileOpen: return _L("EFsIsFileOpen");
	case EFsListOpenFiles: return _L("EFsListOpenFiles");
	case EFsSetNotifyUser: return _L("EFsSetNotifyUser");
	case EFsIsFileInRom: return _L("EFsIsFileInRom");
	case EFsGetNotifyUser: return _L("EFsGetNotifyUser");
	case EFsIsValidName: return _L("EFsIsValidName");
	case EFsReadFileSection: return _L("EFsReadFileSection");
	case EFsNotifyChangeEx: return _L("EFsNotifyChangeEx");
	case EFsNotifyChangeCancelEx: return _L("EFsNotifyChangeCancelEx");
	case EFsDismountFileSystem: return _L("EFsDismountFileSystem");
	case EFsFileSystemName: return _L("EFsFileSystemName");
	case EFsScanDrive: return _L("EFsScanDrive");
	case EFsControlIo: return _L("EFsControlIo");
	case EFsLockDrive: return _L("EFsLockDrive");
	case EFsUnlockDrive: return _L("EFsUnlockDrive");
	case EFsClearPassword: return _L("EFsClearPassword");
	case EFsNotifyDiskSpace: return _L("EFsNotifyDiskSpace");
	case EFsNotifyDiskSpaceCancel: return _L("EFsNotifyDiskSpaceCancel");
	case EFsMountFileSystemScan: return _L("EFsMountFileSystemScan");
	case EFsSessionToPrivate: return _L("EFsSessionToPrivate");
	case EFsPrivatePath: return _L("EFsPrivatePath");
	case EFsCreatePrivatePath: return _L("EFsCreatePrivatePath");
	case EFsFileDrive: return _L("EFsFileDrive");
	case EFsRemountDrive: return _L("EFsRemountDrive");
	case EFsAddExtension: return _L("EFsAddExtension");
	case EFsMountExtension: return _L("EFsMountExtension");
	case EFsDismountExtension: return _L("EFsDismountExtension");
	case EFsRemoveExtension: return _L("EFsRemoveExtension");
	case EFsExtensionName: return _L("EFsExtensionName");
	case EFsStartupInitComplete: return _L("EFsStartupInitComplete");
	case EFsSetLocalDriveMapping: return _L("EFsSetLocalDriveMapping");
	case EFsFileDuplicate: return _L("EFsFileDuplicate");
	case EFsFileAdopt: return _L("EFsFileAdopt");
	case EFsFinaliseDrive: return _L("EFsFinaliseDrive");
	case EFsSwapFileSystem: return _L("EFsSwapFileSystem");
	case EFsErasePassword: return _L("EFsErasePassword");
	case EFsReserveDriveSpace: return _L("EFsReserveDriveSpace");
	case EFsGetReserveAccess: return _L("EFsGetReserveAccess");
	case EFsReleaseReserveAccess: return _L("EFsReleaseReserveAccess");
	case EFsFileName: return _L("EFsFileName");
    case EFsGetMediaSerialNumber: return _L("EFsGetMediaSerialNumber");
	case EFsFileFullName: return _L("EFsFileFullName");
	case EFsAddPlugin: return _L("EFsAddPlugin");
	case EFsRemovePlugin: return _L("EFsRemovePlugin");
	case EFsMountPlugin: return _L("EFsMountPlugin");
	case EFsDismountPlugin: return _L("EFsDismountPlugin");
	case EFsPluginName: return _L("EFsPluginName");
	case EFsPluginOpen: return _L("EFsPluginOpen");
	case EFsPluginSubClose: return _L("EFsPluginSubClose");
	case EFsPluginDoRequest: return _L("EFsPluginDoRequest");
	case EFsPluginDoControl: return _L("EFsPluginDoControl");
	case EFsPluginDoCancel: return _L("EFsPluginDoCancel");
	case EFsNotifyDismount: return _L("EFsNotifyDismount");
	case EFsNotifyDismountCancel: return _L("EFsNotifyDismountCancel");
	case EFsAllowDismount: return _L("EFsAllowDismount");
	case EFsSetStartupConfiguration: return _L("EFsSetStartupConfiguration");
	case EFsFileReadCancel: return _L("EFsFileReadCancel");
	case EFsAddCompositeMount: return _L("EFsAddCompositeMount");
	case EFsSetSessionFlags: return _L("EFsSetSessionFlags");
	case EFsSetSystemDrive: return _L("EFsSetSystemDrive");
	case EFsBlockMap: return _L("EFsBlockMap");
	case EFsUnclamp: return _L("EFsUnclamp");
	case EFsFileClamp: return _L("EFsFileClamp");
	case EFsQueryVolumeInfoExt: return _L("EFsQueryVolumeInfoExt");
	case EFsInitialisePropertiesFile: return _L("EFsInitialisePropertiesFile");
	case EFsFileWriteDirty: return _L("EFsFileWriteDirty");
	case EFsSynchroniseDriveThread: return _L("EFsSynchroniseDriveThread");
	case EFsAddProxyDrive: return _L("EFsAddProxyDrive");
	case EFsRemoveProxyDrive: return _L("EFsRemoveProxyDrive");
	case EFsMountProxyDrive: return _L("EFsMountProxyDrive");
	case EFsDismountProxyDrive:  return _L("EFsDismountProxyDrive");
	case EFsNotificationAdd : return _L("EFsNotificationAdd"); 
	case EFsNotificationBuffer : return _L("EFsNotificationBuffer");
	case EFsNotificationCancel : return _L("EFsNotificationCancel");
	case EFsNotificationOpen : return _L("EFsNotificationOpen");
	case EFsNotificationRemove : return _L("EFsNotificationRemove");
	case EFsNotificationRequest : return _L("EFsNotificationRequest");
	case EFsNotificationSubClose : return _L("EFsNotificationSubClose");
	case EFsLoadCodePage: return _L("EFsLoadCodePage");
	default:
		return _L("Error unknown function");
		}
	}


void PrintStartUpReason(TMachineStartupType aReason)
//
// Print the reason the machine is booting up
//
	{

	TBuf<64> nameBuf;
	switch (aReason)
		{
	case EStartupCold: nameBuf=_L("EStartupCold"); break;
	case EStartupColdReset: nameBuf=_L("EStartupColdReset"); break;
	case EStartupNewOs: nameBuf=_L("EStartupNewOs"); break;
	case EStartupPowerFail: nameBuf=_L("StartupPowerFail"); break;
	case EStartupWarmReset: nameBuf=_L("EStartupWarmReset"); break;
	case EStartupKernelFault: nameBuf=_L("EStartupKernelFault"); break;
	case EStartupSafeReset: nameBuf=_L("EStartupSafeReset"); break;
	default:
		nameBuf=_L("Error unknown startup type");
		}
	__PRINT1(_L("Machine startup - %S"),&nameBuf);
	};

LOCAL_C void AllocPrint(TInt aFunction)
//
// Print number of alloc fails to complete a given function
//
	{
	if (UserHeapAllocFailCount || KernHeapAllocFailCount)
		{	
		TPtrC funcName=GetFunctionName(aFunction);
			{
			__PRINTALLOC(_L("Function %S UserAllocs %d KernAllocs %d"),&funcName,UserHeapAllocFailCount,KernHeapAllocFailCount);
			}
		}
	}

void SimulateAllocFailure(TInt aFunctionReturnValue,TInt aFunction)
//
// Simulate alloc failure
//
	{

	
	if (UserHeapAllocFailCount>=0 && (aFunctionReturnValue==KErrDiskFull || aFunctionReturnValue==KErrNoMemory))
		{
		if (KernHeapAllocFailCount<20)
			{
			__KHEAP_FAILNEXT(++KernHeapAllocFailCount);
			__UHEAP_FAILNEXT(UserHeapAllocFailCount);
			}
		else
			{
			KernHeapAllocFailCount=0;
			__UHEAP_FAILNEXT(++UserHeapAllocFailCount);
			__KHEAP_FAILNEXT(KernHeapAllocFailCount);
			}
		if (UserHeapAllocFailCount<100)
			return;
		}
	if (UserHeapAllocFailCount>=0)
		{
		AllocPrint(aFunction);
		UserHeapAllocFailCount=0;
		KernHeapAllocFailCount=0;
		__UHEAP_FAILNEXT(UserHeapAllocFailCount);
		__KHEAP_FAILNEXT(KernHeapAllocFailCount);
		}
	}

#else
TInt FsDebugFunction(CFsRequest* /*aRequest*/)
//
// Not in the release build
//
	{
	return KErrNotSupported;
	}
#endif

TInt TFsDebugFunc::DoRequestL(CFsRequest* aRequest)
//
// Not in the release build
//
	{
	return FsDebugFunction(aRequest);
	}

TInt TFsDebugFunc::Initialise(CFsRequest* /*aRequest*/)
//
// Not in the release build
//
	{
	return KErrNone;
	}

TInt TFsControlIo::DoRequestL(CFsRequest* aRequest)
//
// General purpose test interface - .FSY specific.
// Not in the release build
//
	{
	TInt command=aRequest->Message().Int1();
	TAny* param1=(TAny*)aRequest->Message().Ptr2();
	TAny* param2=(TAny*)aRequest->Message().Ptr3();


#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	switch(command)
		{	//These are non-drive commands
		case KControlIoGetCorruptLogRecord:
			{
			// number of the log record to be retrieved (1-based)
			TInt logRecNum=(TInt)param2;
			TFsDebugCorruptLogRecordBuf logBuf;
			TInt r=TCorruptLogRec::GetLogRecord(logBuf,logRecNum);
			if(r!=KErrNone)
				return r;
			r=aRequest->Write(2,logBuf,0);
			return r;
			}
		case KControlIoGetNumberOfCorruptLogRecords:
			{
			TPtrC8 ptrHits((TUint8*)&gNumberOfCorruptHits,sizeof(gNumberOfCorruptHits));
			TInt r=aRequest->Write(2,ptrHits,0);

			return r;
			}
		case KControlIoGetCorruptListFile:
			{
			// Get name of file that contains the filenames nominated as "corrupt"
			TBuf8<KMaxFileName> fileName8;
			if(gCorruptFileNamesListFile)
				{
				fileName8.Copy(*gCorruptFileNamesListFile);
				}
			else
				{
				fileName8.SetLength(0);
				}
			TInt r=aRequest->Write(2,fileName8,0);
			return r;
			}
		case KControlIoCorruptLogRecordReset:
			{
			TCorruptLogRec::DestroyList();
			TCorruptNameRec::ResetListConsumed();
			return KErrNone;
			}
		case KControlIoCacheCount:
			{
			TIOCacheValues cacheValues;
			cacheValues.iCloseCount= RequestAllocator::CloseCount();
			cacheValues.iFreeCount= RequestAllocator::FreeCount();
			cacheValues.iAllocated=	0;	// no longer used
			cacheValues.iTotalCount= RequestAllocator::RequestCount();
			cacheValues.iRequestCountPeak = RequestAllocator::RequestCountPeak();

			cacheValues.iOpFreeCount= OperationAllocator::FreeCount();
			cacheValues.iOpRequestCount= OperationAllocator::RequestCount();
			cacheValues.iOpRequestCountPeak = OperationAllocator::RequestCountPeak();


			TPckgBuf<TIOCacheValues> pkgBuf(cacheValues);

			// ensure we only write what the client buffer can hold -
			// this allows TIOCacheValues to increase in size without breaking BC
			pkgBuf.SetLength(Min(pkgBuf.MaxLength(), aRequest->Message().GetDesMaxLengthL(2)));
			
			TInt r=aRequest->Write(2,pkgBuf);
			return r;
			}
		case KControlIoGetLocalDriveNumber:
			{
			return DriveNumberToLocalDriveNumber(aRequest->Drive()->DriveNumber());
			}
		case KControlIoCancelDeferredDismount:
			{
			// Cancel and clear deferred dismount information
			aRequest->Drive()->ClearDeferredDismount();
			return KErrNone;
			}
		case KControlIoFileCacheStats:
			{
			CCacheManager* manager = CCacheManagerFactory::CacheManager();
			if (manager == NULL)
				return KErrNotSupported;

			TFileCacheStats& stats = manager->Stats();

			TPckgBuf<TFileCacheStats> pkgBuf(stats);
			TInt r=aRequest->Write(2,pkgBuf);
			return r;
			}
		case KControlIoFileCacheConfig:
			{
			TInt driveNumber = aRequest->Drive()->DriveNumber();

			TFileCacheSettings::TFileCacheConfig* driveConfig = NULL;
			TInt r = TFileCacheSettings::GetFileCacheConfig(driveNumber, driveConfig);
			if (( r != KErrNone) || (driveConfig == NULL))
				return KErrNotSupported;

			TFileCacheConfig config;
			config.iDrive = driveConfig->iDrive;
			config.iFlags = driveConfig->iFlags;
			config.iFileCacheReadAsync = driveConfig->iFileCacheReadAsync;
			config.iFairSchedulingLen = driveConfig->iFairSchedulingLen;
			config.iCacheSize = driveConfig->iCacheSize;
			config.iMaxReadAheadLen = driveConfig->iMaxReadAheadLen;
			config.iClosedFileKeepAliveTime = driveConfig->iClosedFileKeepAliveTime;
			config.iDirtyDataFlushTime = driveConfig->iDirtyDataFlushTime;

			TPckgBuf<TFileCacheConfig> pkgBuf(config);
			r=aRequest->Write(2,pkgBuf);
			return r;
			}
		case KControlIoFileCacheFlagsWrite:
			{
			TInt drive = aRequest->Message().Int0();

			TPckgBuf<TFileCacheFlags> driveFlagsPkg;
			TFileCacheFlags& driveFlags = driveFlagsPkg();

			TInt r = aRequest->Read(2, driveFlagsPkg);
			if (r != KErrNone)
				return r;

			__CACHE_PRINT2(_L("CACHE: TFileCacheFlags %x on drive %d"), driveFlags, drive);
			TFileCacheSettings::SetFlags(drive, driveFlags);

			return r;
			}
		case KControlIoFlushClosedFiles:
			{
			TClosedFileUtils::Remove();
			return KErrNone;
			}
		case KControlIoSimulateLockFailureMode:
			{
			TPckgBuf<TBool> enabledPkg;
			TInt r = aRequest->Read(2, enabledPkg);
			if (r != KErrNone)
				return r;

			CCacheManager* manager = CCacheManagerFactory::CacheManager();
			if (manager == NULL)
				return KErrNotSupported;
			manager->SimulateLockFailureMode(enabledPkg());
			return KErrNone;
			}
		case KControlIoSimulateFileCacheWriteFailure:
			{
			CCacheManager* manager = CCacheManagerFactory::CacheManager();
			if (manager == NULL)
				return KErrNotSupported;
			manager->SimulateWriteFailure();
			return KErrNone;
			}
		case KControlIoFileCacheDump:
			{
			CCacheManager* manager = CCacheManagerFactory::CacheManager();
			if (manager == NULL)
				return KErrNotSupported;
			manager->DumpCache();
			return KErrNone;
			}
		case KControlIoAllocateMaxSegments:
			{
			TPckgBuf<TBool> enabledPkg;
			TInt r = aRequest->Read(2, enabledPkg);
			if (r != KErrNone)
				return r;

			CCacheManager* manager = CCacheManagerFactory::CacheManager();
			if (manager == NULL)
				return KErrNotSupported;
			manager->AllocateMaxSegments(enabledPkg());
			return KErrNone;
			}
		case KControlIoDisableFatUtilityFunctions:
			{
			EnableFatUtilityFunctions = EFalse;
			return KErrNone;
			}
		case KControlIoEnableFatUtilityFunctions:
			{
			EnableFatUtilityFunctions = ETrue;
			return KErrNone;
			}
        case KControlIoSessionCount:
            {
            TPckgBuf<TInt> pkgBuf(SessionCount);
            TInt r=aRequest->Write(2,pkgBuf);
            return r;
            }
        case KControlIoObjectCount:
            {
            TPckgBuf<TInt> pkgBuf(ObjectCount);
            TInt r=aRequest->Write(2,pkgBuf);
            return r;
            }

		case KControlIoHeapCellCount:
			{
            TPckgBuf<TInt> pkgBuf(User::Allocator().Count());
            TInt r=aRequest->Write(2,pkgBuf);
            return r;
			}

        // Check if the file is in 'file sequential/non-rugged file' mode
        case KControlIoIsFileSequential:
        	{
        	TDrive* drive = aRequest->Drive();
        	if(!drive)
        		return KErrNotSupported;
        	
        	// RFs::ControlIO uses narrow descriptors, so convert narrow back to wide
        	TBuf8<KMaxPath> fileNameNarrow;
        	TInt r = aRequest->Read(2, fileNameNarrow);
        	if (r != KErrNone)
        		return r;
        	TFileName fileNameWide;
        	fileNameWide.Copy(fileNameNarrow);
        	
        	// Locate the file
        	CFileCB* file = drive->LocateFile(fileNameWide);
        	if(!file)
        		return KErrNotFound;
        	
        	// isFileSequential = 1 or 0 for EFileSequential mode enabled or disabled respectively
        	TUint8 isFileSequential = (file->IsSequentialMode() != 0);
        	TPtr8 pkgBuf(&isFileSequential,1,1);
        	aRequest->Write(3, pkgBuf);
        	
        	return KErrNone;
        	}
        case KControlIoGlobalCacheConfig:
		// read ESTART.TXT file for global memory settings
            {
            TGlobalCacheConfig globalCacheConfig;
            TInt32 rel;
            
            const TInt KByteToByteShift = 10;
            _LIT8(KLitSectionNameCacheMemory,"CacheMemory");
            
            if (F32Properties::GetInt(KLitSectionNameCacheMemory, _L8("GlobalCacheMemorySize"), rel))
                globalCacheConfig.iGlobalCacheSizeInBytes = rel << KByteToByteShift;
            else
                globalCacheConfig.iGlobalCacheSizeInBytes = KErrNotFound;

            if (F32Properties::GetInt(KLitSectionNameCacheMemory, _L8("LowMemoryThreshold"), rel))
                globalCacheConfig.iGlobalLowMemoryThreshold = rel;
            else
                globalCacheConfig.iGlobalLowMemoryThreshold = KErrNotFound;

            TPckgBuf<TGlobalCacheConfig> pkgBuf(globalCacheConfig);
            TInt r=aRequest->Write(2,pkgBuf);
            return r;
            }
        case KControlIoGlobalCacheInfo:
    	// get system's current global cache memory info
            {
            TGlobalCacheInfo info;
            info.iGlobalCacheSizeInBytes = TGlobalCacheMemorySettings::CacheSize();
            info.iGlobalLowMemoryThreshold = TGlobalCacheMemorySettings::LowMemoryThreshold();
            TPckgBuf<TGlobalCacheInfo> pkgBuf(info);
            TInt r=aRequest->Write(2,pkgBuf);
            return r;
            }
         case KControlIoDirCacheConfig:
         // read ESTART.TXT file for per-drive directory cache settings
            {
            TInt driveNumber = aRequest->Drive()->DriveNumber();
            TDirCacheConfig dirCacheConfig;
            TInt32 rel;
            dirCacheConfig.iDrive = driveNumber;
            
            TBuf8<32> driveSection;
            F32Properties::GetDriveSection(driveNumber, driveSection);
            
            if (F32Properties::GetInt(driveSection, _L8("FAT_LeafDirCacheSize"), rel))
                dirCacheConfig.iLeafDirCacheSize = rel;
            else
                dirCacheConfig.iLeafDirCacheSize = KErrNotFound;

            if (F32Properties::GetInt(driveSection, _L8("FAT_DirCacheSizeMin"), rel))
                dirCacheConfig.iDirCacheSizeMin = rel << KByteToByteShift;
            else
                dirCacheConfig.iDirCacheSizeMin = KErrNotFound;

            if (F32Properties::GetInt(driveSection, _L8("FAT_DirCacheSizeMax"), rel))
                dirCacheConfig.iDirCacheSizeMax = rel << KByteToByteShift;
            else
                dirCacheConfig.iDirCacheSizeMax = KErrNotFound;

            TPckgBuf<TDirCacheConfig> pkgBuf(dirCacheConfig);
            TInt r=aRequest->Write(2,pkgBuf);
            return r;
            }
         case KControlIoDirCacheInfo:
         // get system's current per-drive directory cache settings
		 //  currently only supports FAT file system
             {
             TFSName fsName;
             aRequest->Drive()->CurrentMount().FileSystemName(fsName);
             if (fsName.CompareF(KFileSystemName_FAT) == 0)
                 {
                 // 16 is the control cmd used for FAT
                 //  see EFATDirCacheInfo in FAT code please. 
                 const TInt KFATDirCacheInfo = 16;
                 return(aRequest->Drive()->ControlIO(aRequest->Message(),KFATDirCacheInfo,param1,param2));
                 }
             return KErrNotSupported;
             }
         case KControlIoSimulateMemoryLow:
             {
             CCacheMemoryManager* cacheMemManager = CCacheMemoryManagerFactory::CacheMemoryManager();
             if (cacheMemManager)
                 cacheMemManager->SetMemoryLow(ETrue);
             else
                 return KErrNotSupported;
             return KErrNone;
             }
         case KControlIoStopSimulateMemoryLow:
             {
             CCacheMemoryManager* cacheMemManager = CCacheMemoryManagerFactory::CacheMemoryManager();
             if (cacheMemManager)
                 cacheMemManager->SetMemoryLow(EFalse);
             else
                 return KErrNotSupported;
             return KErrNone;
             }
		
		}
#endif

	return(aRequest->Drive()->ControlIO(aRequest->Message(),command,param1,param2));
	}

TInt TFsControlIo::Initialise(CFsRequest* aRequest)
//
// General purpose test interface - .FSY specific.
// Not in the release build
//
	{
	TInt driveNumber=aRequest->Message().Int0();
	if(driveNumber<0||driveNumber>=KMaxDrives)
		return(KErrArgument);
	ValidateDriveDoSubst(driveNumber,aRequest);
	return KErrNone;
	}

