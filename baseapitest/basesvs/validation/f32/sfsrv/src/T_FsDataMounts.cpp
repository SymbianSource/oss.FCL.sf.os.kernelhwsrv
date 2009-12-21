/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/


/**
@test
@internalComponent

This contains CT_FsData
*/

//	User includes
#include "T_FsData.h"

//	EPOC includes
#include <f32fsys.h>

/*@{*/
/// Enumerations
_LIT(KDriveA,							"EDriveA");
_LIT(KDriveB,							"EDriveB");
_LIT(KDriveC,							"EDriveC");
_LIT(KDriveD,							"EDriveD");
_LIT(KDriveE,							"EDriveE");
_LIT(KDriveF,							"EDriveF");
_LIT(KDriveG,							"EDriveG");
_LIT(KDriveH,							"EDriveH");
_LIT(KDriveI,							"EDriveI");
_LIT(KDriveJ,							"EDriveJ");
_LIT(KDriveK,							"EDriveK");
_LIT(KDriveL,							"EDriveL");
_LIT(KDriveM,							"EDriveM");
_LIT(KDriveN,							"EDriveN");
_LIT(KDriveO,							"EDriveO");
_LIT(KDriveP,							"EDriveP");
_LIT(KDriveQ,							"EDriveQ");
_LIT(KDriveR,							"EDriveR");
_LIT(KDriveS,							"EDriveS");
_LIT(KDriveT,							"EDriveT");
_LIT(KDriveU,							"EDriveU");
_LIT(KDriveV,							"EDriveV");
_LIT(KDriveW,							"EDriveW");
_LIT(KDriveX,							"EDriveX");
_LIT(KDriveY,							"EDriveY");
_LIT(KDriveZ,							"EDriveZ");

_LIT(KWriteMappingsAndSet,				"EWriteMappingsAndSet");
_LIT(KWriteMappingsNoSet,				"EWriteMappingsNoSet");
_LIT(KSwapIntMappingAndSet,				"ESwapIntMappingAndSet");


///	Parameters
_LIT(KDrive,							"drive");
_LIT(KDriveMappingElement,				"drive_mapping_element_");
_LIT(KDriveMappingSize,					"drive_mapping_size");
_LIT(KDriveMappingOperation,			"drive_mapping_operation");
_LIT(KFileName,							"file_name");
_LIT(KFileSystemName,					"file_system_name");
_LIT(KNewFileSystemName,				"new_file_system_name");
_LIT(KFlags,							"flags");
_LIT(KIsSync,							"is_sync");
_LIT(KExtensionName,					"extension_name");
_LIT(KPosition,							"position");
_LIT(KMode,								"mode");
_LIT(KAll,								"all");
_LIT(KCommandNum,						"command_num");
_LIT(KSaveInInstance,					"save_in_instance");
_LIT(KLocalDrive,						"local_drive");
_LIT(KCompositeDrive,					"composite_drive");
_LIT(KIsMountSuccess,					"is_mount_success");
_LIT(KParam1,							"param1");
_LIT(KParam2,							"param2");
_LIT(KIndex,							"index");
_LIT(KSubTypeName,						"sub_type_name");


///	Commands
_LIT(KCmdAddFileSystem,					"AddFileSystem");
_LIT(KCmdDismountFileSystem,			"DismountFileSystem");
_LIT(KCmdFileSystemName,				"FileSystemName");
_LIT(KCmdMountFileSystem,				"MountFileSystem");
_LIT(KCmdMountFileSystemAndScan,		"MountFileSystemAndScan");
_LIT(KCmdRemountDrive,					"RemountDrive");
_LIT(KCmdRemoveFileSystem,				"RemoveFileSystem");
_LIT(KCmdAddExtension,					"AddExtension");
_LIT(KCmdMountExtension,				"MountExtension");
_LIT(KCmdRemoveExtension,				"RemoveExtension");
_LIT(KCmdExtensionName,					"ExtensionName");
_LIT(KCmdAddCompositeMount,				"AddCompositeMount");
_LIT(KCmdAllowDismount,					"AllowDismount");
_LIT(KCmdNotifyDismount,				"NotifyDismount");
_LIT(KCmdNotifyDismountCancel,			"NotifyDismountCancel");
_LIT(KCmdStartupInitComplete,			"StartupInitComplete");
_LIT(KCmdSwapFileSystem,				"SwapFileSystem");
_LIT(KCmdSetStartupConfiguration,		"SetStartupConfiguration");
_LIT(KCmdDismountExtension,				"DismountExtension");
_LIT(KCmdSetLocalDriveMapping,			"SetLocalDriveMapping");
_LIT(KCmdFinaliseDrives,				"FinaliseDrives");
_LIT(KCmdFileSystemSubType,				"FileSystemSubType");
/*@}*/


TBool CT_FsData::DoCommandMountsL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex)
/**
 * Process a mounts related command read from the ini file
 *
 * @param aCommand	the command to process
 * @param aSection		the entry in the ini file requiring the command to be processed
 *
 * @return ETrue if the command is processed
 */
	{
	TBool retVal = ETrue;
	
	if (aCommand == KCmdAddFileSystem)
		{
		DoCmdAddFileSystem(aSection);
		}
	else if (aCommand == KCmdDismountFileSystem)
		{
		DoCmdDismountFileSystem(aSection);
		}
	else if (aCommand == KCmdFileSystemName)
		{
		DoCmdFileSystemName(aSection);
		}
	else if (aCommand == KCmdMountFileSystem)
		{
		DoCmdMountFileSystem(aSection);
		}
	else if (aCommand == KCmdMountFileSystemAndScan)
		{
		DoCmdMountFileSystemAndScan(aSection);
		}
	else if (aCommand == KCmdRemoveFileSystem)
		{
		DoCmdRemoveFileSystem(aSection);
		}
	else if (aCommand == KCmdRemountDrive)
		{
		DoCmdRemountDriveL(aSection);
		}
	else if (aCommand == KCmdAddExtension)
		{
		DoCmdAddExtension(aSection);
		}
	else if (aCommand == KCmdRemoveExtension)
		{
		DoCmdRemoveExtension(aSection);
		}
	else if (aCommand == KCmdExtensionName)
		{
		DoCmdExtensionName(aSection);
		}
	else if (aCommand == KCmdMountExtension)
		{
		DoCmdMountExtension(aSection);
		}
	else if (aCommand == KCmdAllowDismount)
		{
		DoCmdAllowDismount(aSection);
		}
	else if (aCommand == KCmdNotifyDismount)
		{
		DoCmdNotifyDismountL(aSection, aAsyncErrorIndex);
		}
	else if (aCommand == KCmdNotifyDismountCancel)
		{
		DoCmdNotifyDismountCancel(aSection);
		}
	else if (aCommand == KCmdStartupInitComplete)
		{
		DoCmdStartupInitCompleteL(aAsyncErrorIndex);
		}
	else if (aCommand == KCmdSwapFileSystem)
		{
		DoCmdSwapFileSystem(aSection);
		}
	else if (aCommand == KCmdSetStartupConfiguration)
		{
		DoCmdSetStartupConfigurationL(aSection);
		}
	else if (aCommand == KCmdFinaliseDrives)
		{
		DoCmdFinaliseDrives();
		}
    else if (aCommand == KCmdAddCompositeMount)
		{
		DoCmdAddCompositeMount(aSection);
		}
    else if (aCommand == KCmdDismountExtension)
		{
		DoCmdDismountExtension(aSection);
		}
	else if (aCommand == KCmdSetLocalDriveMapping)
		{
		DoCmdSetLocalDriveMappingL(aSection);
		}
    else if (aCommand == KCmdFileSystemSubType)
		{
		DoCmdFileSystemSubType(aSection);
		}
 	else
		{
		retVal = EFalse;
		}

	return retVal;
	}


void CT_FsData::DoCmdAddFileSystem(const TDesC& aSection)
/** Calls RFs::AddFileSystem() */
	{
	INFO_PRINTF1(_L("Calls RFs::AddFileSystem()"));

	// get file name from parameters
	TPtrC	fileName;
	if ( GET_MANDATORY_STRING_PARAMETER(KFileName(), aSection, fileName))
		{
		// call AddFileSystem()
		TInt	err = iFs->AddFileSystem(fileName);

		// check error code
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("AddFileSystem error: %d"), err);
			SetError(err);
			}
		}
	}

void CT_FsData::DoCmdMountFileSystem(const TDesC& aSection)
/** Calls RFs::MountFileSystem() */
	{
	INFO_PRINTF1(_L("Calls RFs::MountFileSystem()"));

	TBool dataOk = ETrue;

	// get drive number from parameters
	TDriveNumber	driveNumber = EDriveA;
	if (!GetDriveNumberFromConfig(aSection, KDrive(), driveNumber))
		{
		ERR_PRINTF2(_L("No %S"), &KDrive());
		SetBlockResult(EFail);
		dataOk = EFalse;
		}

	// get file system name from parameters
	TPtrC	fileSystemName;
	if ( !GET_OPTIONAL_STRING_PARAMETER(KFileSystemName(), aSection, fileSystemName) )
		{
		fileSystemName.Set(iFileSystemName);
		}

	if(dataOk)
		{
		// get file system name from parameters
		TBool	isSync = EFalse;
		TInt	err = KErrNone;

		TPtrC	extensionName;
		TBool	hasExtensionName=GET_OPTIONAL_STRING_PARAMETER(KExtensionName(), aSection, extensionName);
		if (GET_OPTIONAL_BOOL_PARAMETER(KIsSync(), aSection, isSync))
			{
			if ( hasExtensionName )
				{
				// call MountFileSystem()
				INFO_PRINTF5(_L("MountFileSystem(%S, %S, %d, %d)"), &fileSystemName, &extensionName, driveNumber, isSync);
				err = iFs->MountFileSystem(fileSystemName, extensionName, driveNumber, isSync);
				}
			else
				{
				// call MountFileSystem()
				INFO_PRINTF4(_L("MountFileSystem(%S, %d, %d)"), &fileSystemName, driveNumber, isSync);
				err = iFs->MountFileSystem(fileSystemName, driveNumber, isSync);
				}
			}
		else
			{
			if ( hasExtensionName )
				{
				// call MountFileSystem()
				INFO_PRINTF4(_L("MountFileSystem(%S, %S, %d)"), &fileSystemName, &extensionName, driveNumber);
				err = iFs->MountFileSystem(fileSystemName, extensionName, driveNumber);
				}
			else
				{
				// call MountFileSystem()
				INFO_PRINTF3(_L("MountFileSystem(%S, %d)"), &fileSystemName, driveNumber);
				err = iFs->MountFileSystem(fileSystemName, driveNumber);
				}
			}
		// check error code
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("MountFileSystem error: %d"), err);
			SetError(err);
			}
		}
	}


void CT_FsData::DoCmdMountFileSystemAndScan(const TDesC& aSection)
/** Calls RFs::MountFileSystemAndScan() */
	{
	INFO_PRINTF1(_L("Calls RFs::MountFileSystemAndScan()"));

	TBool dataOk = ETrue;

	// get drive number from parameters
	TDriveNumber	driveNumber = EDriveA;
	if (!GetDriveNumberFromConfig(aSection, KDrive(), driveNumber))
		{
		ERR_PRINTF2(_L("No %S"), &KDrive());
		SetBlockResult(EFail);
		dataOk = EFalse;
		}

	// get file system name from parameters
	TPtrC	fileSystemName;
	if ( !GET_OPTIONAL_STRING_PARAMETER(KFileSystemName(), aSection, fileSystemName) )
		{
		fileSystemName.Set(iFileSystemName);
		}

	if(dataOk)
		{
		TInt	err=KErrNone;
		TBool	isMountSuccess = EFalse;
		// get extension name from parameters
		TPtrC	extensionName;
		if (GET_OPTIONAL_STRING_PARAMETER(KExtensionName(), aSection, extensionName))
			{
			//call MountFileSystemAndScan
			INFO_PRINTF4(_L("MountFileSystemAndScan(%S, %S, %d, isMountSuccess)"), &fileSystemName, &extensionName, driveNumber);
			err = iFs->MountFileSystemAndScan(fileSystemName, extensionName, driveNumber, isMountSuccess);
			}
		else
			{
			//call MountFileSystemAndScan
			INFO_PRINTF3(_L("MountFileSystemAndScan(%S, %d, isMountSuccess)"), &fileSystemName, driveNumber);
			err = iFs->MountFileSystemAndScan(fileSystemName, driveNumber, isMountSuccess);
			}

		// check error code
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("MountFileSystemAndScan error: %d"), err);
			SetError(err);
			}

		//Only for info
		if(isMountSuccess)
			{
			INFO_PRINTF1(_L("MountFileSystemAndScan() successfully mounted"));
			}
		else
			{
			INFO_PRINTF1(_L("MountFileSystemAndScan() not mounted"));
			}

		//check if mount was successful
		TBool	expected = EFalse;
		if ( GET_OPTIONAL_BOOL_PARAMETER(KIsMountSuccess(), aSection, expected) )
			{
			if ( isMountSuccess!=expected )
				{
				ERR_PRINTF1(_L("MountFileSystemAndScan() does not match expected value"));
				SetBlockResult(EFail);
				}
			}
		}
	}


void CT_FsData::DoCmdDismountFileSystem(const TDesC& aSection)
/** Calls RFs::DismountFileSystem() */
	{
	INFO_PRINTF1(_L("Calls RFs::DismountFileSystem()"));

	TBool dataOk = ETrue;

	// get drive number from parameters
	TDriveNumber	driveNumber = EDriveA;
	if (!GetDriveNumberFromConfig(aSection, KDrive(), driveNumber))
		{
		ERR_PRINTF2(_L("No %S"), &KDrive());
		SetBlockResult(EFail);
		dataOk = EFalse;
		}
	// get file system name from parameters
	TPtrC	fileSystemName;
	if (!GET_OPTIONAL_STRING_PARAMETER(KFileSystemName(), aSection, fileSystemName))
		{
		fileSystemName.Set(iFileSystemName);
		}

	if(dataOk)
		{
		// call DismountFileSystem()
		TInt err = iFs->DismountFileSystem(fileSystemName, driveNumber);

		// check error code
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("DismountFileSystem error: %d"), err);
			SetError(err);
			}
		}
	}


void CT_FsData::DoCmdRemoveFileSystem(const TDesC& aSection)
/** Calls RFs::RemoveFileSystem() */
	{
	INFO_PRINTF1(_L("Calls RFs::RemoveFileSystem()"));

	// get file name from parameters
	TPtrC	fileSystemName;
	if ( !GET_OPTIONAL_STRING_PARAMETER(KFileSystemName(), aSection, fileSystemName) )
		{
		fileSystemName.Set(iFileSystemName);
		}

	// call RemoveFileSystem()
	TInt err = iFs->RemoveFileSystem(fileSystemName);

	// check error code
	if (err != KErrNone)
		{
		ERR_PRINTF2(_L("RemoveFileSystem error: %d"), err);
		SetError(err);
		}
	}


void CT_FsData::DoCmdFileSystemName(const TDesC& aSection)
/** Calls RFs::FileSystemName() */
	{
	INFO_PRINTF1(_L("Calls RFs::FileSystemName()"));

	// get a flag if we need to remember the file system name in an instance variable
	TBool	save=ETrue;
	GET_OPTIONAL_BOOL_PARAMETER(KSaveInInstance(), aSection, save);

	// get drive number from parameters
	TDriveNumber	driveNumber = EDriveA;
	if (!GetDriveNumberFromConfig(aSection, KDrive(), driveNumber))
		{
		ERR_PRINTF2(_L("No %S"), &KDrive());
		SetBlockResult(EFail);
		}
	else
		{
		// call FileSystemName()
		TFullName	fileSystemName;
		TInt		err = iFs->FileSystemName(fileSystemName, driveNumber);
		INFO_PRINTF2(_L("File system name: %S"), &fileSystemName);

		// check error code
		if (err == KErrNone)
			{
			if ( save )
				{
				iFileSystemName=fileSystemName;
				}
			}
		else
			{
			ERR_PRINTF2(_L("FileSystemName error: %d"), err);
			SetError(err);
			}

		//Check expected result
		TPtrC	fsNameExpect;
		if (GET_OPTIONAL_STRING_PARAMETER(KFileSystemName(), aSection, fsNameExpect))
			{
			if ( fsNameExpect != fileSystemName)
				{
				ERR_PRINTF1(_L("File system name != expected name"));
				SetBlockResult(EFail);
				}
			}
		}
	}


void CT_FsData::DoCmdAddExtension(const TDesC& aSection)
/** Calls RFs::AddExtension() */
	{
	INFO_PRINTF1(_L("Calls RFs::AddExtension()"));
	TPtrC fileName;
	if ( GET_MANDATORY_STRING_PARAMETER(KFileName(), aSection, fileName) )
		{
		//call AddExtension
		TInt err = iFs->AddExtension(fileName);

		// check error code
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("AddExtension error: %d"), err);
			SetError(err);
			}
		}
	}


void CT_FsData::DoCmdMountExtension(const TDesC& aSection)
/** Calls RFs::MountExtension() */
	{
	INFO_PRINTF1(_L("Calls RFs::MountExtension()"));

	TBool dataOk = ETrue;

	// get drive number from parameters
	TDriveNumber	driveNumber = EDriveA;
	if (!GetDriveNumberFromConfig(aSection, KDrive(), driveNumber))
		{
		ERR_PRINTF2(_L("No %S"), &KDrive());
		SetBlockResult(EFail);
		dataOk = EFalse;
		}

	// get extension name from parameters
	TPtrC extensionName;
	if (!GET_MANDATORY_STRING_PARAMETER(KExtensionName(), aSection, extensionName))
		{
		dataOk = EFalse;
		}

	if(dataOk)
		{
		TInt	err = iFs->MountExtension(extensionName, driveNumber);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("MountExtension error: %d"), err);
			SetError(err);
			}
		}
	}

void CT_FsData::DoCmdDismountExtension(const TDesC& aSection)
/** Calls RFs::DismountExtension() */
	{
	INFO_PRINTF1(_L("Calls RFs::DismountExtension()"));

	TBool dataOk = ETrue;

	// get drive number from parameters
	TDriveNumber	driveNumber = EDriveA;
	if (!GetDriveNumberFromConfig(aSection, KDrive(), driveNumber))
		{
		ERR_PRINTF2(_L("No %S"), &KDrive());
		SetBlockResult(EFail);
		dataOk = EFalse;
		}

	TPtrC extensionName;
	if (!GET_MANDATORY_STRING_PARAMETER(KExtensionName(), aSection, extensionName))
		{
		dataOk = EFalse;
		}
	if(dataOk)
		{
		//call DismountExtension
		TInt err = iFs->DismountExtension(extensionName, driveNumber);

		//check error code
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("DismountExtension error: %d"), err);
			SetError(err);
			}
		}

	}


void CT_FsData::DoCmdRemoveExtension(const TDesC& aSection)
/** Calls RFs::RemoveExtension() */
	{
	INFO_PRINTF1(_L("Calls RFs::RemoveExtension()"));

	TPtrC extensionName;
	if (GET_MANDATORY_STRING_PARAMETER(KExtensionName(), aSection, extensionName))
		{
		//call RemoveExtension
		TInt err = iFs->RemoveExtension(extensionName);

		// check error code
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("RemoveExtension error: %d"), err);
			SetError(err);
			}
		}
	}


void CT_FsData::DoCmdExtensionName(const TDesC& aSection)
/** Calls RFs::ExtensionName() */
	{
	INFO_PRINTF1(_L("Calls RFs::ExtensionName()"));

	TBool dataOk = ETrue;

	// get drive number from parameters
	TDriveNumber	driveNumber = EDriveA;
	if (!GetDriveNumberFromConfig(aSection, KDrive(), driveNumber))
		{
		ERR_PRINTF2(_L("No %S"), &KDrive());
		SetBlockResult(EFail);
		dataOk = EFalse;
		}
	//get position of the extension in the extension hierarchy.
	TInt pos = 0;
	if (!GET_MANDATORY_INT_PARAMETER(KPosition(), aSection, pos))
		{
		dataOk = EFalse;
		}

	if(dataOk)
		{

		TInt err = iFs->ExtensionName(iExtensionName, driveNumber, pos);

		if(err == KErrNone)
			{
			TPtrC extNameExpect;
			if (GET_OPTIONAL_STRING_PARAMETER(KExtensionName(), aSection, extNameExpect))
				{
				if(extNameExpect != iExtensionName)
					{
					ERR_PRINTF3(_L("ExtensionName: %S != %S expected name"), &iExtensionName, &extNameExpect);
					SetBlockResult(EFail);
					}
				else
					{
					INFO_PRINTF2(_L("ExtensionName: %S"), &iExtensionName);
					}
				}
			else
				{
				INFO_PRINTF2(_L("ExtensionName: %S"), &iExtensionName);
				}
			}
		// check error code
		else
			{
			ERR_PRINTF2(_L("ExtensionName error: %d"), err);
			SetError(err);
			}
		}
	}


void CT_FsData::DoCmdRemountDriveL(const TDesC& aSection)
/** Calls RFs::RemountDrive() */
	{
	INFO_PRINTF1(_L("Calls RFs::RemountDrive()"));
	TBool	dataOk = ETrue;

	// get drive number from parameters
	TDriveNumber	driveNumber = EDriveA;
	if (!GetDriveNumberFromConfig(aSection, KDrive(), driveNumber))
		{
		ERR_PRINTF2(_L("No %S"), &KDrive());
		SetBlockResult(EFail);
		dataOk = EFalse;
		}

	// get flags from parameters can be in hex(0x00000001) or in decimal(1)
	TUint	flags = 0;
	TInt	tempVal = 0;
	
	if(GetHexFromConfig(aSection, KFlags, tempVal))
		{
		flags = tempVal;
		}
	else if(GET_OPTIONAL_INT_PARAMETER(KFlags(), aSection, tempVal))
		{
		flags = tempVal;
		}
	else
		{
		ERR_PRINTF2(_L("No parameter %S"), &KFlags());
		SetBlockResult(EFail);
		dataOk = EFalse;
		}

	RFile	file;
	HBufC8*	fileDataBuff = NULL;

	// get file name from parameters
	TPtrC	fileName;
	TBool	useMountInfo = GET_OPTIONAL_STRING_PARAMETER(KFileName(), aSection, fileName);
	if ( useMountInfo )
		{
		// open file
		TInt error = file.Open(FileServer(), fileName, EFileRead | EFileShareAny);
		if(error == KErrNone)
			{
			CleanupClosePushL(file);
			INFO_PRINTF1(_L("File Opened."));
			// get size
			TInt	size = 0;
			error = file.Size(size);
			INFO_PRINTF2(_L("File Size=%d."), size);
			if(error == KErrNone)
				{
				// read file
				fileDataBuff = HBufC8::NewL(size);
				INFO_PRINTF1(_L("Buffer created."));

				CleanupStack::PushL(fileDataBuff);
				TPtr8	fileData = fileDataBuff->Des();
				error = file.Read(fileData);
				if(error != KErrNone)
					{
					ERR_PRINTF2(_L("Reading File, Error: %d"), error);
					dataOk = EFalse;
					SetBlockResult(EFail);
					}
				}
			else
				{
				ERR_PRINTF2(_L("File Size, Error: %d."), error);
				dataOk = EFalse;
				SetBlockResult(EFail);
				}
			}
		else
			{
			ERR_PRINTF2(_L("File Open, Error: %d"), error);
			dataOk = EFalse;
			SetBlockResult(EFail);
			}
		}

	if ( dataOk )
		{
		// call RemountDrive()
		TInt err = iFs->RemountDrive(driveNumber, fileDataBuff, flags);
		INFO_PRINTF2(_L("RemountDrive Flags: %u"), flags);

		// check error code
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("RemountDrive error: %d"), err);
			SetError(err);
			}
		}

	// cleanup if mountInfo was used
	if (useMountInfo)
		{
		CleanupStack::PopAndDestroy(2, &file); // file, fileDataBuff
		}

	}


void CT_FsData::DoCmdNotifyDismountL(const TDesC& aSection, const TInt aAsyncErrorIndex)
/** Calls RFs::NotifyDismount() */
	{
	INFO_PRINTF1(_L("Calls RFs::NotifyDismount()"));

	TNotifyDismountMode mode = EFsDismountRegisterClient;
	TPtrC modeStr;
	//get NotifyDismountMode from parameters
	if(GET_MANDATORY_STRING_PARAMETER(KMode(), aSection, modeStr))
		{
		if(modeStr == _L("EFsDismountRegisterClient"))
			{
			mode = EFsDismountRegisterClient;
			}
		else if(modeStr == _L("EFsDismountNotifyClients"))
			{
			mode = EFsDismountNotifyClients;
			}
		else if(modeStr == _L("EFsDismountForceDismount"))
			{
			mode = EFsDismountForceDismount;
			}
		else
			{
			TInt modeNumber;
			if(GET_OPTIONAL_INT_PARAMETER(KMode, aSection, modeNumber))
				{
				mode = (TNotifyDismountMode)modeNumber;
				}
			else
				{
				ERR_PRINTF3(_L("NotifyDismount() incorrect parameter %S in %S"), &modeStr, &KMode());
				SetBlockResult(EFail);
				}
			}
		}

	TDriveNumber	driveNumber = EDriveA;
	if (!GetDriveNumberFromConfig(aSection, KDrive(), driveNumber))
		{
		ERR_PRINTF2(_L("No %S"), &KDrive());
		SetBlockResult(EFail);
		}
	else
		{
		// call NotifyDismount()
		CActiveCallback*	active = CActiveCallback::NewLC(*this);
		iNotifyDismount.AppendL(active);
		CleanupStack::Pop(active);
		
		iFs->NotifyDismount(driveNumber, active->iStatus, mode);
		active->Activate(aAsyncErrorIndex);
		IncOutstanding();
		}
	}


void CT_FsData::DoCmdNotifyDismountCancel(const TDesC& aSection)
/** Calls RFs::NotifyDismountCancel() */
	{
	INFO_PRINTF1(_L("Calls RFs::NotifyDismountCancel()"));

	TBool all = ETrue;

	if(GET_MANDATORY_BOOL_PARAMETER(KAll(), aSection, all))
		{
		if(all)
			{
			//Cancels all outstanding requests.
			iFs->NotifyDismountCancel();
			INFO_PRINTF1(_L("NotifyDismountCancel()"));
			}
		else
			{
			// Cancels a specific outstanding request
			TInt	index=0;
			GET_OPTIONAL_INT_PARAMETER(KIndex, aSection, index);

			//Cancels a specific outstanding request
			iFs->NotifyDismountCancel(iNotifyDismount[index]->iStatus);
			INFO_PRINTF1(_L("NotifyDismountCancel(TRequestStatus)"));
			}
		}
	}


void CT_FsData::DoCmdAllowDismount(const TDesC& aSection)
/** Calls RFs::AllowDismount() */
	{
	INFO_PRINTF1(_L("Calls RFs::AllowDismount()"));

	// get drive number from parameters
	TDriveNumber	driveNumber = EDriveA;
	if(!GetDriveNumberFromConfig(aSection, KDrive(), driveNumber))
		{
		ERR_PRINTF2(_L("No %S"), &KDrive());
		SetBlockResult(EFail);
		}
	else
		{
		//call AllowDismount
		TInt err = iFs->AllowDismount(driveNumber);
		// check error code
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("AllowDismount error: %d"), err);
			SetError(err);
			}
		}
	}


void CT_FsData::DoCmdSetStartupConfigurationL(const TDesC& aSection)
/** Calls RFs::SetStartupConfiguration() */
	{
	INFO_PRINTF1(_L("Calls RFs::SetStartupConfiguration()"));

	//get Command from parameters
	TPtrC commandNum;
	TStartupConfigurationCmd configCommand = ELoaderPriority;
	if(GET_MANDATORY_STRING_PARAMETER(KCommandNum(), aSection, commandNum))
		{
		//Converting from string to TStartupConfigurationCmd
		if(commandNum == _L("ELoaderPriority"))
			{
			configCommand = ELoaderPriority;
			}
        else if(commandNum == _L("ESetRugged"))
			{
			configCommand = ESetRugged;
			}
		else if(commandNum == _L("EMaxStartupConfigurationCmd"))
			{
			configCommand = EMaxStartupConfigurationCmd;
			}
		TAny* param1 = NULL;
		TPtrC name;
		if(GET_OPTIONAL_STRING_PARAMETER(KParam1(), aSection, name))
			{
			param1 = GetDataObjectL(name);
			}
		TAny* param2 = NULL;

		if(GET_OPTIONAL_STRING_PARAMETER(KParam2(), aSection, name))
			{		
			param2 = GetDataObjectL(name);
			}
		//call SetStartupConfiguration
		TInt err = iFs->SetStartupConfiguration(configCommand, param1, param2);
		INFO_PRINTF2(_L("RFs::SetStartupConfiguration() %S"), &commandNum);
		// check error code
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("SetStartupConfiguration() error: %d"), err);
			SetError(err);
			}
		}
	}

void CT_FsData::DoCmdAddCompositeMount(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::AddCompositeMount()"));

	TBool dataOk = ETrue;

	// get drive number from parameters
	TDriveNumber	localDriveToMount = EDriveA;
	if (!GetDriveNumberFromConfig(aSection, KLocalDrive(), localDriveToMount))
		{
		ERR_PRINTF2(_L("No %S"), &KLocalDrive());
		SetBlockResult(EFail);
		dataOk = EFalse;
		}

	// get composite drive number from parameters
	TDriveNumber	compositeDrive = EDriveA;
	if (!GetDriveNumberFromConfig(aSection, KCompositeDrive(), compositeDrive))
		{
		ERR_PRINTF2(_L("No %S"), &KCompositeDrive());
		SetBlockResult(EFail);
		dataOk = EFalse;
		}

	// 	get Sync from parameters
	TBool	sync = EFalse;
	if(!GET_MANDATORY_BOOL_PARAMETER(KIsSync(), aSection, sync))
		{
		dataOk = EFalse;
		}

	// get fileSystemName from parameters
	TPtrC	fileSystemName;
	if ( !GET_OPTIONAL_STRING_PARAMETER(KFileSystemName(), aSection, fileSystemName) )
		{
		fileSystemName.Set(iFileSystemName);
		}

	if(dataOk)
		{
		TInt	err = iFs->AddCompositeMount(fileSystemName, localDriveToMount, compositeDrive, sync);
		INFO_PRINTF1(_L("Calls RFs::AddCompositeMount()"));
		// check error code
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("AddCompositeMount() error: %d"), err);
			SetError(err);
			}
		}
	}
void CT_FsData::DoCmdStartupInitCompleteL(TInt aAsyncErrorIndex)
/** Calls RFs::StartupInitComplete() */
	{
	INFO_PRINTF1(_L("Calls RFs::StartupInitComplete()"));

	// call StartupInitComplete()
	CActiveCallback*	active = CActiveCallback::NewLC(*this);
	iStartupInitComplete.AppendL(active);
	CleanupStack::Pop(active);
		
	iFs->StartupInitComplete(active->iStatus);
	active->Activate(aAsyncErrorIndex);
	IncOutstanding();
	}


void CT_FsData::DoCmdFinaliseDrives()
/** Calls RFs::FinaliseDrives() */
	{
	INFO_PRINTF1(_L("Calls RFs::FinaliseDrives()"));

	TInt err = iFs->FinaliseDrives();

	// check error code
	if (err != KErrNone)
		{
		ERR_PRINTF2(_L("FinaliseDrives() error: %d"), err);
		SetError(err);
		}
	}


void CT_FsData::DoCmdSetLocalDriveMappingL(const TDesC& aSection)
/** Calls RFs::SetLocalDriveMapping() */
	{
	INFO_PRINTF1(_L("Calls RFs::SetLocalDriveMapping()"));

	// get operation from config	
	TLocalDriveMappingInfo::TDrvMapOperation operation = TLocalDriveMappingInfo::EWriteMappingsAndSet;
	if (!GetDrvMapOperationFromConfig(aSection, KDriveMappingOperation(), operation))
		{
		ERR_PRINTF2(_L("No %S"), &KDriveMappingOperation());
		SetBlockResult(EFail);
		}
		
	// create an info buffer and set the operation
	TLocalDriveMappingInfoBuf infoBuf;
	TLocalDriveMappingInfo& info = infoBuf();
	info.iOperation = operation;

	// get drive mapping array from config
	TInt driveMappingSize = 0;
	if (GET_MANDATORY_INT_PARAMETER(KDriveMappingSize(), aSection, driveMappingSize))
		{
		for(TInt i = 0; i < driveMappingSize; i++)
			{
			TBuf<KBufferStringLength> tmpBuff;

			tmpBuff.Append(KDriveMappingElement);
			tmpBuff.AppendNum(i);

			TDriveNumber driveNumber;
			if (GetDriveNumberFromConfig(aSection, tmpBuff, driveNumber))
				{
				info.iDriveMapping[i] = driveNumber;
				INFO_PRINTF3(_L("Drive mapping element[%d] = %d"), i, driveNumber);
				}
			else
				{
				ERR_PRINTF2(_L("No %S"), &tmpBuff);
				SetBlockResult(EFail);
				}
			}
		}

	// call SetLocalDriveMapping()
	TInt err = iFs->SetLocalDriveMapping(infoBuf);

	// check error code
	if (err != KErrNone)
		{
		ERR_PRINTF2(_L("SetLocalDriveMapping() error: %d"), err);
		SetError(err);
		}
	}


void CT_FsData::DoCmdSwapFileSystem(const TDesC& aSection)
/** Calls RFs::SwapFileSystem() */
	{
	INFO_PRINTF1(_L("Calls RFs::SwapFileSystem()"));

	//get OldFileSystemName
	// get fileSystemName from parameters
	TPtrC	fileSystemName;
	if ( !GET_OPTIONAL_STRING_PARAMETER(KFileSystemName(), aSection, fileSystemName) )
		{
		fileSystemName.Set(iFileSystemName);
		}

	//get NewFileSystemName
	TPtrC newFsName;
	if(!GET_OPTIONAL_STRING_PARAMETER(KNewFileSystemName(), aSection, newFsName))
		{
		newFsName.Set(iFileSystemName);
		}

	// get drive number from parameters
	TBool dataOk = ETrue;
	TDriveNumber	driveNumber = EDriveA;
	if (!GetDriveNumberFromConfig(aSection, KDrive(), driveNumber))
		{
		ERR_PRINTF2(_L("No %S"), &KDrive());
		SetBlockResult(EFail);
		dataOk = EFalse;
		}

	if(dataOk)
		{
		TInt err = iFs->SwapFileSystem(fileSystemName, newFsName, driveNumber);
		// check error code
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("SwapFileSystem() error: %d"), err);
			SetError(err);
			}
		}

	}

void CT_FsData::DoCmdFileSystemSubType(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::FileSystemSubType()"));

	// get drive number from parameters
	TDriveNumber	driveNumber = EDriveA;
	if (GetDriveNumberFromConfig(aSection, KDrive(), driveNumber))
		{
		TFSName subTypeName;
		TInt err = iFs->FileSystemSubType(driveNumber, subTypeName);
		// check error code
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("FileSystemSubType() error: %d"), err);
			SetError(err);
			}
		else
			{
			INFO_PRINTF2(_L("FileSystemSubType() subTypeName = %S"), &subTypeName);
			//Check expected result
			TPtrC	subTypeNameExpect;
			if (GET_OPTIONAL_STRING_PARAMETER(KSubTypeName(), aSection, subTypeNameExpect))
				{
				if ( subTypeNameExpect != subTypeName)
					{
					ERR_PRINTF1(_L("File system name != expected name"));
					SetBlockResult(EFail);
					}
				}
			}
		}
	else
		{
		ERR_PRINTF2(_L("No %S"), &KDrive());
		SetBlockResult(EFail);
		}
	}

TBool CT_FsData::GetDriveNumberFromConfig(const TDesC& aSection, const TDesC& aParameterName, TDriveNumber& aDriveNumber)
/** Reads drive number from INI-file */
	{
	// Read drive number from INI file
	TPtrC	driveNumberStr;
	TBool	ret = GET_OPTIONAL_STRING_PARAMETER(aParameterName, aSection, driveNumberStr);
	if (ret)
		{
		if (driveNumberStr == KDriveA)
			{
			aDriveNumber = EDriveA;
			}
		else if (driveNumberStr == KDriveB)
			{
			aDriveNumber = EDriveB;
			}
		else if (driveNumberStr == KDriveC)
			{
			aDriveNumber = EDriveC;
			}
		else if (driveNumberStr == KDriveD)
			{
			aDriveNumber = EDriveD;
			}
		else if (driveNumberStr == KDriveE)
			{
			aDriveNumber = EDriveE;
			}
		else if (driveNumberStr == KDriveF)
			{
			aDriveNumber = EDriveF;
			}
		else if (driveNumberStr == KDriveG)
			{
			aDriveNumber = EDriveG;
			}
		else if (driveNumberStr == KDriveH)
			{
			aDriveNumber = EDriveH;
			}
		else if (driveNumberStr == KDriveI)
			{
			aDriveNumber = EDriveI;
			}
		else if (driveNumberStr == KDriveJ)
			{
			aDriveNumber = EDriveJ;
			}
		else if (driveNumberStr == KDriveK)
			{
			aDriveNumber = EDriveK;
			}
		else if (driveNumberStr == KDriveL)
			{
			aDriveNumber = EDriveL;
			}
		else if (driveNumberStr == KDriveM)
			{
			aDriveNumber = EDriveM;
			}
		else if (driveNumberStr == KDriveN)
			{
			aDriveNumber = EDriveN;
			}
		else if (driveNumberStr == KDriveO)
			{
			aDriveNumber = EDriveO;
			}
		else if (driveNumberStr == KDriveP) // Sorry, it's a bit long. But looks nice!
			{
			aDriveNumber = EDriveP;
			}
		else if (driveNumberStr == KDriveQ)
			{
			aDriveNumber = EDriveQ;
			}
		else if (driveNumberStr == KDriveR)
			{
			aDriveNumber = EDriveR;
			}
		else if (driveNumberStr == KDriveS)
			{
			aDriveNumber = EDriveS;
			}
		else if (driveNumberStr == KDriveT)
			{
			aDriveNumber = EDriveT;
			}
		else if (driveNumberStr == KDriveU)
			{
			aDriveNumber = EDriveU;
			}
		else if (driveNumberStr == KDriveV)
			{
			aDriveNumber = EDriveV;
			}
		else if (driveNumberStr == KDriveW)
			{
			aDriveNumber = EDriveW;
			}
		else if (driveNumberStr == KDriveX)
			{
			aDriveNumber = EDriveX;
			}
		else if (driveNumberStr == KDriveY)
			{
			aDriveNumber = EDriveY;
			}
		else if (driveNumberStr == KDriveZ)
			{
			aDriveNumber = EDriveZ;
			}
		else
			{
			TInt driveNumber = 0;
			ret = GET_OPTIONAL_INT_PARAMETER(aParameterName, aSection, driveNumber);
			if (ret)
				{
				aDriveNumber = (TDriveNumber) driveNumber;
				}
			}
		}

	return ret;
	}


TBool CT_FsData::GetDrvMapOperationFromConfig(const TDesC& aSection, const TDesC& aParameterName, TLocalDriveMappingInfo::TDrvMapOperation& aOperation)
/** Reads drive mapping operation name from INI-file */
	{
	// Read drives mapping operation name from INI file
	TPtrC operationStr;
	TBool ret = GET_OPTIONAL_STRING_PARAMETER(aParameterName, aSection, operationStr);
	if (ret)
		{
		if (operationStr == KWriteMappingsAndSet)
			{
			aOperation = TLocalDriveMappingInfo::EWriteMappingsAndSet;
			}
		else if (operationStr == KWriteMappingsNoSet)
			{
			aOperation = TLocalDriveMappingInfo::EWriteMappingsNoSet;
			}
		else if (operationStr == KSwapIntMappingAndSet)
			{
			aOperation = TLocalDriveMappingInfo::ESwapIntMappingAndSet;
			}
		else
			{
			TInt operation = 0;
			ret = GET_OPTIONAL_INT_PARAMETER(aParameterName, aSection, operation);
			if (ret)
				{
				aOperation = (TLocalDriveMappingInfo::TDrvMapOperation) operation;
				}
			}
		}

	return ret;
	}

