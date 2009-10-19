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
// f32test\plugins\trace\t_tracehook.cpp
// 
//

#include "t_tracehook.h"
#include <f32pluginutils.h>

_LIT(KTracePluginName, "A test plugin which logs all file server messages");


TPtrC GetFunctionName(TInt aFunction)
//
// Print number of alloc fails to complete a given function
//
	{
	switch (aFunction)
		{
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
		case EFsGetNotifyUser: return _L("EFsGetNotifyUser");
		case EFsSetNotifyUser: return _L("EFsSetNotifyUser");
		case EFsIsFileInRom: return _L("EFsIsFileInRom");
		case EFsIsValidName: return _L("EFsIsValidName");
		case EFsDebugFunction: return _L("EFsDebugFunction");
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
		case EFsFileDrive: return _L("EFsFileDrive");
		case EFsRemountDrive: return _L("EFsRemountDrive");
		case EFsMountFileSystemScan: return _L("EFsMountFileSystemScan");
		case EFsSessionToPrivate: return _L("EFsSessionToPrivate");
		case EFsPrivatePath: return _L("EFsPrivatePath");
		case EFsCreatePrivatePath: return _L("EFsCreatePrivatePath");
		case EFsAddExtension: return _L("EFsAddExtension");
		case EFsMountExtension: return _L("EFsMountExtension");
		case EFsDismountExtension: return _L("EFsDismountExtension");
		case EFsRemoveExtension: return _L("EFsRemoveExtension");
		case EFsExtensionName: return _L("EFsExtensionName");
		case EFsStartupInitComplete: return _L("EFsStartupInitComplete");
		case EFsSetLocalDriveMapping: return _L("EFsSetLocalDriveMapping");
		case EFsFinaliseDrive: return _L("EFsFinaliseDrive");
		case EFsFileDuplicate: return _L("EFsFileDuplicate");
		case EFsFileAdopt: return _L("EFsFileAdopt");
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
		default:
			return _L("Error unknown function");
		}
	}
/**
Leaving New function for the plugin
@internalComponent
*/
CTestTraceHook* CTestTraceHook::NewL()
	{
	return new(ELeave) CTestTraceHook;
	}


/**
Constructor for the plugin
@internalComponent
*/
CTestTraceHook::CTestTraceHook()
	{
	}


/**
The destructor for the test trace plugin hook. 
@internalComponent
*/
CTestTraceHook::~CTestTraceHook()
	{
	iFs.Close();
	}

/**
Initialise the trace plugin.
@internalComponent
*/
void CTestTraceHook::InitialiseL()
	{
	User::LeaveIfError(RegisterIntercept(EFsAddFileSystem				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsRemoveFileSystem			,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsMountFileSystem				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsNotifyChange				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsNotifyChangeCancel			,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDriveList					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDrive						,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsVolume						,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSetVolume					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSubst						,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSetSubst					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsRealName					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDefaultPath					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSetDefaultPath				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSessionPath					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSetSessionPath				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsMkDir						,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsRmDir						,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsParse						,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDelete						,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsRename						,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsReplace						,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsEntry						,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSetEntry					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsGetDriveName				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSetDriveName				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFormatSubClose				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDirSubClose					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSubClose				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsRawSubClose					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileOpen					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileCreate					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileReplace					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileTemp					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileRead					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileWrite					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileLock					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileUnLock					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSeek					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileFlush					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSize					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSetSize					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileAtt						,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSetAtt					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileModified				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSetModified				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSet						,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileChangeMode				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileRename					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDirOpen						,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDirReadOne					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDirReadPacked				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFormatOpen					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFormatNext					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsRawDiskOpen					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsRawDiskRead					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsRawDiskWrite				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsResourceCountMarkStart		,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsResourceCountMarkEnd		,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsResourceCount				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsCheckDisk					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsGetShortName				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsGetLongName					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsIsFileOpen					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsListOpenFiles				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsGetNotifyUser				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSetNotifyUser				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsIsFileInRom					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsIsValidName					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDebugFunction				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsReadFileSection				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsNotifyChangeEx				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsNotifyChangeCancelEx		,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDismountFileSystem			,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSystemName				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsScanDrive					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsControlIo					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsLockDrive					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsUnlockDrive					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsClearPassword				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsNotifyDiskSpace				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsNotifyDiskSpaceCancel		,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileDrive					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsRemountDrive				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsMountFileSystemScan			,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSessionToPrivate			,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsPrivatePath					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsCreatePrivatePath			,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsAddExtension				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsMountExtension				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDismountExtension			,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsRemoveExtension				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsExtensionName				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsStartupInitComplete			,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSetLocalDriveMapping		,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFinaliseDrive				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileDuplicate				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileAdopt					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSwapFileSystem				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsErasePassword				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsReserveDriveSpace			,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsGetReserveAccess			,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsReleaseReserveAccess		,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileName					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsGetMediaSerialNumber		,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileFullName				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsAddPlugin					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsRemovePlugin				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsMountPlugin					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsPluginName					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsPluginOpen					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsPluginSubClose				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsPluginDoRequest				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsPluginDoControl				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsPluginDoCancel				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsNotifyDismount				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsNotifyDismountCancel		,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsAllowDismount				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSetStartupConfiguration		,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileReadCancel				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsAddCompositeMount			,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSetSessionFlags				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSetSystemDrive				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsBlockMap					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsUnclamp						,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileClamp					,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsQueryVolumeInfoExt			,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsInitialisePropertiesFile	,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileWriteDirty				,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSynchroniseDriveThread		,	EPrePostIntercept));



	User::LeaveIfError(iFs.Connect());
	}

/**
@internalComponent
*/
TInt CTestTraceHook::DoRequestL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;

	TInt function = aRequest.Function();
	
	iDrvNumber = aRequest.DriveNumber();

	TPtrC functionName = GetFunctionName(function);
	

	if (!aRequest.IsPostOperation())
		{
		iPreIntercepts++;
		TFileName name;

		TInt nameRet = GetName(&aRequest, name);
		if (nameRet == KErrNotSupported)
			nameRet = GetPath(&aRequest, name);

		TFileName newName;
		TInt newNameRet = GetNewName(&aRequest, newName);

		TInt len;
		TInt pos;
		TInt fileAccessRet = GetFileAccessInfo(&aRequest, len, pos);

		if (newNameRet == KErrNone)
			RDebug::Print(_L(">%S\t(%S,%S)"), &functionName, &name, &newName);
		else if (fileAccessRet == KErrNone)
			RDebug::Print(_L(">%S\t%08X@%08X\t(%S)"), &functionName, len, pos, &name);
		else if (nameRet == KErrNone)
			RDebug::Print(_L(">%S\t(%S)"), &functionName, &name);
		else
			RDebug::Print(_L(">%S"), &functionName);
		}
	else
		{
		iPostIntercepts++;
		
		// we should never see any internal file caching requests
		ASSERT(function != EFsFileWriteDirty);

		// for EFsFileSubClose, check we can read the file name & use the request's message handle
		if (function == EFsFileSubClose)
			{
			TFileName fileName;
#ifdef _DEBUG
	        TInt r = 
#endif
			GetName( &aRequest, fileName);
			ASSERT(r == KErrNone);
			// get process id (uses message handle)
			TUid processId = aRequest.Message().Identity();
            RDebug::Print(_L("<%S\t(%S), PID %X"),&functionName, &fileName, processId.iUid);
			}
		else
			{
			RDebug::Print(_L("<%S"), &functionName);
			}
		}

	return err;
	}




/**
@internalComponent
*/
TInt CTestTraceHook::TracePluginName(TDes& aName)
	{
	aName = KTracePluginName;
	return KErrNone;
	}




//factory functions

class CTraceHookFactory : public CFsPluginFactory
	{
public:
	CTraceHookFactory();
	virtual TInt Install();			
	virtual CFsPlugin* NewPluginL();
	virtual CFsPlugin* NewPluginConnL();
	virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
*/
CTraceHookFactory::CTraceHookFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
*/
TInt CTraceHookFactory::Install()
	{
	iSupportedDrives = KPluginAutoAttach;

	_LIT(KTraceHookName,"TraceHook");
	return(SetName(&KTraceHookName));
	}

/**
@internalComponent
*/
TInt CTraceHookFactory::UniquePosition()
	{
	return(0x1EC);
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CTraceHookFactory::NewPluginL()

	{
	return CTestTraceHook::NewL();
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CTraceHookFactory::NewPluginConnL()

	{
	return CTestTraceHook::NewL();
	}

/**
Create a new Plugin
@internalComponent
*/
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
	{
	return(new CTraceHookFactory());
	}
}

