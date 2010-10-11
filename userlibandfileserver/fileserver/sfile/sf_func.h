// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_func.h
// 
//

#if !defined __SF_FUNC_H
#define __SF_FUNC_H



TBool SimulateError(const RMessage2* aMessage);
TInt DoFsSubClose(CSessionFs* aSession);

class CFsRequest;

class TFsAddFileSystem
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};
	
class TFsRemoveFileSystem
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};
	
class TFsMountFileSystem
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsMountFileSystemScan
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};


class TFsDismountFileSystem
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileSystemName
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};
	
class TFsAddExtension
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsMountExtension
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsDismountExtension
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsRemoveExtension
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsExtensionName
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};
	
class TFsRemountDrive
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};
	
class TFsNotifyChange
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsNotifyChangeEx
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsNotifyChangeCancel
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsNotifyChangeCancelEx
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsDriveList
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsDrive
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsVolume
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsSetVolume
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};


class TFsSubst
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsSetSubst
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsRealName
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsDefaultPath
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsSetDefaultPath
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};
//#endif


class TFsSessionPath
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsSetSessionPath
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsMkDir
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsRmDir
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsParse
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsDelete
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsRename
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsReplace
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsEntry
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
private:
	};

class TFsSetEntry
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsSubClose
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	static TInt Complete(CFsRequest* aRequest);
	};

class TFsFileOpen
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileCreate
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileReplace
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileTemp
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
private:
	};

class TFsFileRead
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt PostInitialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	static TInt Complete(CFsRequest* aRequest);
private:
	};

class TFsFileWrite
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt PostInitialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	static TInt Complete(CFsRequest* aRequest);
	static void CommonEnd(CFsMessageRequest* aRequest, TInt aRetVal, TUint64 aInitSize, TUint64 aCurrentSize, TInt64 aNewPos, TBool aFileWrite);
private:
	static TInt CommonInit(CFileShare* aShare, CFileCB* aFile, TInt64& aPos, TInt& aLen, TInt64 aFileSize, TFsMessage aFsOp);
	};

class TFsFileLock
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
private:
	};

class TFsFileUnlock
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileSeek
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileFlush
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileSize
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileSetSize
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileAtt
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileSetAtt
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileModified
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileSetModified
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileSet
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileChangeMode
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileRename
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsDirOpen
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsDirReadOne
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsDirReadPacked
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFormatOpen
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFormatNext
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsRawDiskOpen
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsRawDiskClose
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsRawDiskRead
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsRawDiskWrite
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsResourceCountMarkStart
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsResourceCountMarkEnd
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsResourceCount
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsCheckDisk
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsScanDrive
	{
public:
	static TInt Initialise(CFsRequest* aReqeust);
	static TInt DoRequestL(CFsRequest* aReqeust);
	};

class TFsGetShortName
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsGetLongName
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsIsFileOpen
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsListOpenFiles
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsSetNotifyUser
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsGetNotifyUser
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsIsFileInRom
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsIsValidName
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsGetDriveName
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsSetDriveName
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsDebugFunc
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsControlIo
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsReadFileSection
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	static TInt Complete(CFsRequest* aRequest);
	};

class TFsLockDrive
	{
public:
	static TInt Initialise(CFsRequest *aRequest);
	static TInt DoRequestL(CFsRequest *aRequest);
	};

class TFsUnlockDrive
	{
public:
	static TInt Initialise(CFsRequest *aRequest);
	static TInt DoRequestL(CFsRequest *aRequest);
	};

class TFsClearPassword
	{
public:
	static TInt Initialise(CFsRequest *aRequest);
	static TInt DoRequestL(CFsRequest *aRequest);
	};

class TFsErasePassword
	{
public:
	static TInt Initialise(CFsRequest *aRequest);
	static TInt DoRequestL(CFsRequest *aRequest);
	};

class TFsNotifyDiskSpace
	{
public:
	static TInt Initialise(CFsRequest *aRequest);
	static TInt DoRequestL(CFsRequest *aRequest);
	};

class TFsNotifyDiskSpaceCancel
	{
public:
	static TInt Initialise(CFsRequest *aRequest);
	static TInt DoRequestL(CFsRequest *aRequest);
	};

class TFsFileDrive
	{
public:
	static TInt Initialise(CFsRequest *aRequest);
	static TInt DoRequestL(CFsRequest *aRequest);
	};

class TFsSessionToPrivate
	{
public:
	static TInt Initialise(CFsRequest *aRequest);
	static TInt DoRequestL(CFsRequest *aRequest);
	};

class TFsPrivatePath
	{
public:
	static TInt Initialise(CFsRequest *aRequest);
	static TInt DoRequestL(CFsRequest *aRequest);
	};

class TFsCreatePrivatePath
	{
public:
	static TInt Initialise(CFsRequest *aRequest);
	static TInt DoRequestL(CFsRequest *aRequest);
	};

class TFsStartupInitComplete
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsSetLocalDriveMapping
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileDuplicate
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileAdopt
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFinaliseDrive
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsCloseObject
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	static TInt Complete(CFsRequest* aRequest);
	};

class TFsFlushDirtyData
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsSessionDisconnect
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsCancelPlugin
	{
	public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsCloseFileShare : public TFsCloseObject
	{
public:
	static TInt DoRequestL(CFsRequest* aRequest);
	static TInt Complete(CFsRequest* aRequest);
	};

class TFsSwapFileSystem
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsReserveDriveSpace
	{
public:
	static TInt Initialise(CFsRequest *aRequest);
	static TInt DoRequestL(CFsRequest *aRequest);
	};

class TFsGetReserveAccess
	{
public:
	static TInt Initialise(CFsRequest *aRequest);
	static TInt DoRequestL(CFsRequest *aRequest);
	};

class TFsReleaseReserveAccess
	{
public:
	static TInt Initialise(CFsRequest *aRequest);
	static TInt DoRequestL(CFsRequest *aRequest);
	};

class TFsFileName
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileFullName
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};
	
class TFsGetMediaSerialNumber
    {
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
    };

class TFsAddPlugin
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsMountPlugin
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsDismountPlugin
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsRemovePlugin
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsPluginName
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsPluginOpen
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsPluginDoRequest
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsPluginDoControl
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsPluginDoCancel
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsNotifyDismount
    {
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
private:
	static TInt RegisterNotify(CFsRequest* aRequest);
    };

class TFsNotifyDismountCancel
    {
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
    };

class TFsAllowDismount
    {
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
    };

class TFsSetStartupConfiguration
    {
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
    };

class TFsFileReadCancel
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsAddCompositeMount
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
#ifndef __GCC32__ // GCC does not like AddFsToCompositeMountL() being private 
private:
#endif
	static void AddFsToCompositeMountL(TInt aDriveNumber, CFileSystem& aFileSystem, TInt aLocalDriveNumber);
	};


class TFsSetSessionFlags
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};


class TFsSetSystemDrive
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};


class TFsBlockMap
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileClamp
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsUnclamp
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsQueryVolumeInfoExt
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsInitialisePropertiesFile
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsFileWriteDirty
	{
public:
	static TInt PostInitialise(CFsRequest* aRequest);
	};

class TFsSynchroniseDriveThread
	{
public:
	static TInt Initialise(CFsRequest *aRequest);
	static TInt DoRequestL(CFsRequest *aRequest);
	};

class TFsAddProxyDrive
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsRemoveProxyDrive
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsMountProxyDrive
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsDismountProxyDrive
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};
	
class CFsNotifyRequest; //Forward declaration
class TFsNotificationOpen
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	static void HandleRequestL(CFsRequest* aRequest, CFsNotifyRequest* aNotifyRequest, TInt& aHandle,TBool& aAddedToManager);
	};

class TFsNotificationBuffer
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);	
	};

class TFsNotificationAdd
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsNotificationRemove
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsNotificationRequest
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsNotificationCancel
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsNotificationSubClose
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsLoadCodePage
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

class TFsDeferredDismount
	{
public:
	static TInt Initialise(CFsRequest* aRequest);
	static TInt DoRequestL(CFsRequest* aRequest);
	};

void GetFileFromScratch(CFsRequest* aRequest, CFileShare*& aShare, CFileCB*& aFile);

#endif	// __SF_FUNC_H

