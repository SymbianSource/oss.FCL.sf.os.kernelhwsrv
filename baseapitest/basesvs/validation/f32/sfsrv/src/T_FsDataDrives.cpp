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
#include "T_VolumeInfoData.h"
#include "FileserverUtil.h"

/*@{*/
///Constants

_LIT(KPromptProcess,				"t_prompt.exe");


/// Enumerations
_LIT(KMediaNotPresent,				"EMediaNotPresent");
_LIT(KMediaUnknown,					"EMediaUnknown");
_LIT(KMediaFloppy,					"EMediaFloppy");
_LIT(KMediaHardDisk,				"EMediaHardDisk");
_LIT(KMediaCdRom,					"EMediaCdRom");
_LIT(KMediaRam,						"EMediaRam");
_LIT(KMediaFlash,					"EMediaFlash");
_LIT(KMediaRom,						"EMediaRom");
_LIT(KMediaRemote,					"EMediaRemote");
_LIT(KMediaNANDFlash,				"EMediaNANDFlash");

_LIT(KBatNotSupported,				"EBatNotSupported");
_LIT(KBatLow,						"EBatLow");
_LIT(KBatGood,						"EBatGood");

_LIT(KDriveAttLocalStr,				"KDriveAttLocal");
_LIT(KDriveAttRomStr,				"KDriveAttRom");
_LIT(KDriveAttRedirectedStr,		"KDriveAttRedirected");
_LIT(KDriveAttSubstedStr,			"KDriveAttSubsted");
_LIT(KDriveAttInternalStr,			"KDriveAttInternal");
_LIT(KDriveAttRemovableStr,			"KDriveAttRemovable");

_LIT(KMediaAttVariableSizeStr,		"KMediaAttVariableSize");
_LIT(KMediaAttDualDensityStr,		"KMediaAttDualDensity");
_LIT(KMediaAttFormattableStr,		"KMediaAttFormattable");
_LIT(KMediaAttWriteProtectedStr,	"KMediaAttWriteProtected");
_LIT(KMediaAttLockableStr,			"KMediaAttLockable");
_LIT(KMediaAttLockedStr,			"KMediaAttLocked");


///	Parameters
_LIT(KStore,						"store");
_LIT(KMediaType,					"media_type");
_LIT(KDriveAtt,						"drive_att");
_LIT(KMediaAtt,						"media_att");
_LIT(KDriveIndex,					"drive_index");
_LIT(KDriveChar,					"drive_char");
_LIT(KArraySize,					"array_size");
_LIT(KArrayIndex,					"array_index_");
_LIT(KVolumeLabel,					"volume_label");
_LIT(KSubstPath,					"subst_path");
_LIT(KDriveName,					"drive_name");
_LIT(KDriveOldPassword,				"drive_old_password");
_LIT(KDriveNewPassword,				"drive_new_password");
_LIT(KBytesToReserve,				"bytes_to_reserve");
_LIT(KDriveIsValid,					"drive_is_valid");
_LIT(KDrivePath,					"drive_path");
_LIT(KDrive,						"drive");
_LIT(KBatteryState,					"battery_state");
_LIT(KSaveInInstance,				"save_in_instance");
_LIT(KVolumeObject,					"store_volume");


///	Commands
_LIT(KCmdDriveList,					"DriveList");
_LIT(KCmdDrive,						"Drive");
_LIT(KCmdVolume,					"Volume");
_LIT(KCmdSetVolumeLabel,			"SetVolumeLabel");
_LIT(KCmdSubst,						"Subst");
_LIT(KCmdSetSubst,					"SetSubst");
_LIT(KCmdGetMediaSerialNumber,		"GetMediaSerialNumber");
_LIT(KCmdIsValidDrive,				"IsValidDrive");
_LIT(KCmdCharToDrive,				"CharToDrive");
_LIT(KCmdDriveToChar,				"DriveToChar");
_LIT(KCmdCheckDisk,					"CheckDisk");
_LIT(KCmdScanDrive,					"ScanDrive");
_LIT(KCmdGetDriveName,				"GetDriveName");
_LIT(KCmdSetDriveName,				"SetDriveName");
_LIT(KCmdLockDrive,					"LockDrive");
_LIT(KCmdUnlockDrive,				"UnlockDrive");
_LIT(KCmdClearPassword,				"ClearPassword");
_LIT(KCmdErasePassword,				"ErasePassword");
_LIT(KCmdReserveDriveSpace,			"ReserveDriveSpace");
_LIT(KCmdGetReserveAccess,			"GetReserveAccess");
_LIT(KCmdReleaseReserveAccess,		"ReleaseReserveAccess");
_LIT(KCmdPrompt,					"prompt");
_LIT(KCmdGetSystemDrive,			"GetSystemDrive");
_LIT(KCmdSetSystemDrive,			"SetSystemDrive");
_LIT(KCmdGetSystemDriveChar,		"GetSystemDriveChar");
_LIT(KCmdVolumeIOParam,				"VolumeIOParam");

/*@}*/

/**
* Process a drives related command read from the ini file
*
* @param aCommand	the command to process
* @param aSection		the entry in the ini file requiring the command to be processed
*
* @return ETrue if the command is processed
*/
TBool CT_FsData::DoCommandDrivesL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/)
	{
	TBool retVal = ETrue;
	//Parsing the aCommand to choose which function has to be executed.

	if (aCommand == KCmdDriveList)
		{
		DoCmdDriveList(aSection);
		}
	else if (aCommand  == KCmdDrive)
		{
		DoCmdDriveL(aSection);
		}
	else if (aCommand == KCmdVolume)
		{
		DoCmdVolumeL(aSection);
		}
	else if (aCommand == KCmdSetVolumeLabel)
		{
		DoCmdSetVolumeLabel(aSection);
		}
	else if (aCommand == KCmdSubst)
		{
		DoCmdSubst(aSection);
		}
	else if (aCommand == KCmdSetSubst)
		{
		DoCmdSetSubst(aSection);
		}
	else if (aCommand == KCmdGetMediaSerialNumber)
		{
		DoCmdGetMediaSerialNumber(aSection);
		}
	else if (aCommand == KCmdIsValidDrive)
		{
		DoCmdIsValidDrive(aSection);
		}
	else if (aCommand == KCmdCharToDrive)
		{
		DoCmdCharToDrive(aSection);
		}
	else if (aCommand == KCmdDriveToChar)
		{
		DoCmdDriveToChar(aSection);
		}
	else if (aCommand == KCmdCheckDisk)
		{
		DoCmdCheckDisk(aSection);
		}
	else if (aCommand == KCmdScanDrive)
		{
		DoCmdScanDrive(aSection);
		}
	else if (aCommand == KCmdGetDriveName)
		{
		DoCmdGetDriveName(aSection);
		}
	else if (aCommand == KCmdSetDriveName)
		{
		DoCmdSetDriveName(aSection);
		}
	else if (aCommand == KCmdLockDrive)
		{
		DoCmdLockDrive(aSection);
		}
	else if (aCommand == KCmdUnlockDrive)
		{
		DoCmdUnlockDrive(aSection);
		}
	else if (aCommand == KCmdClearPassword)
		{
		DoCmdClearPassword(aSection);
		}
	else if (aCommand == KCmdErasePassword)
		{
		DoCmdErasePassword(aSection);
		}
	else if (aCommand == KCmdReserveDriveSpace)
		{
		DoCmdReserveDriveSpace(aSection);
		}
	else if (aCommand == KCmdGetReserveAccess)
		{
		DoCmdGetReserveAccess(aSection);
		}
	else if (aCommand == KCmdReleaseReserveAccess)
		{
		DoCmdReleaseReserveAccess(aSection);
		}
	else if (aCommand == KCmdPrompt)
		{
		DoCmdPromptL(aSection);
		}
	else if (aCommand == KCmdGetSystemDrive)
		{
		DoCmdGetSystemDrive(aSection);
		}
	else if (aCommand == KCmdGetSystemDriveChar)
		{
		DoCmdGetSystemDriveChar(aSection);
		}
	else if (aCommand == KCmdSetSystemDrive)
		{
		DoCmdSetSystemDrive(aSection);
		}
	else if (aCommand == KCmdVolumeIOParam)
		{
		DoCmdVolumeIOParam(aSection);
		}
	else
		{
		retVal = EFalse;
		}

	return retVal;
	}


TBool CT_FsData::GetMediaTypeFromConfig(const TDesC& aSection, TMediaType& aMediaType)
	{
	// Read media type from INI file
	TPtrC mediaTypeStr;
	TBool ret = GET_OPTIONAL_STRING_PARAMETER(KMediaType(), aSection, mediaTypeStr);

	if (ret)
		{
		if (mediaTypeStr == KMediaNotPresent)
			{
			aMediaType = EMediaNotPresent;
			}
		else if (mediaTypeStr == KMediaUnknown)
			{
			aMediaType = EMediaUnknown;
			}
		else if (mediaTypeStr == KMediaFloppy)
			{
			aMediaType = EMediaFloppy;
			}
		else if (mediaTypeStr == KMediaHardDisk)
			{
			aMediaType = EMediaHardDisk;
			}
		else if (mediaTypeStr == KMediaCdRom)
			{
			aMediaType = EMediaCdRom;
			}
		else if (mediaTypeStr == KMediaRam)
			{
			aMediaType = EMediaRam;
			}
		else if (mediaTypeStr == KMediaFlash)
			{
			aMediaType = EMediaFlash;
			}
		else if (mediaTypeStr == KMediaRom)
			{
			aMediaType = EMediaRom;
			}
		else if (mediaTypeStr == KMediaRemote)
			{
			aMediaType = EMediaRemote;
			}
		else if (mediaTypeStr == KMediaNANDFlash)
			{
			aMediaType = EMediaNANDFlash;
			}
		else
			{
			TInt mediaType = 0;
			ret = GET_MANDATORY_INT_PARAMETER(KMediaType, aSection, mediaType);
			if (ret)
				{
				aMediaType = (TMediaType) mediaType;
				}
			}
		}
	return ret;
	}

TBool CT_FsData::ConvertToDriveAtts(const TDesC& aDriveAttStr, TUint& aDriveAtt)
	{
	TBool	ret = ETrue;
	if ( aDriveAttStr==KDriveAttLocalStr )
		{
		aDriveAtt=KDriveAttLocal;
		}
	else if ( aDriveAttStr==KDriveAttRomStr )
		{
		aDriveAtt=KDriveAttRom;
		}
	else if ( aDriveAttStr==KDriveAttRedirectedStr )
		{
		aDriveAtt=KDriveAttRedirected;
		}
	else if ( aDriveAttStr==KDriveAttSubstedStr )
		{
		aDriveAtt=KDriveAttSubsted;
		}
	else if ( aDriveAttStr==KDriveAttInternalStr )
		{
		aDriveAtt=KDriveAttInternal;
		}
	else if ( aDriveAttStr==KDriveAttRemovableStr )
		{
		aDriveAtt=KDriveAttRemovable;
		}
	else
		{
		TInt	location = aDriveAttStr.Match(_L("*|*"));
		if( location!=KErrNotFound )
			{
			//Converting Left part of the data
			TPtrC	tempStr = aDriveAttStr.Left(location);
			ret=ConvertToDriveAtts(tempStr, aDriveAtt);

			//Converting right data can be with another "|"
			tempStr.Set(aDriveAttStr.Mid(location+1));

			TUint	driveAttTmp;
			if ( ConvertToDriveAtts(tempStr, driveAttTmp) )
				{
				aDriveAtt=aDriveAtt|driveAttTmp;
				}
			else
				{
				ret=EFalse;
				}
			}
		else
			{
			ret=EFalse;
			}
		}

	return ret;
	}

void CT_FsData::DoCmdPromptL(const TDesC& /*aSection*/)
	{
	RProcess promptProcess;
	TInt err = promptProcess.Create(KPromptProcess, KNullDesC);
	
	if (err == KErrNone)
		{
		CleanupClosePushL(promptProcess);
		TRequestStatus promptStatus;
		promptProcess.Rendezvous(promptStatus);

		if (promptStatus == KRequestPending)
			{
			promptProcess.Resume();
			User::WaitForRequest(promptStatus);
			if (err != KErrNone)
				{
				ERR_PRINTF2(_L("Prompt process finished with error %d"), promptStatus.Int());
				SetBlockResult(EFail);
				}
			}
		else
			{
			promptProcess.RendezvousCancel(promptStatus);
			promptProcess.Kill(0);
			ERR_PRINTF2(_L("Executing of prompt process failed with error %d"), promptStatus.Int());
			SetBlockResult(EFail);
			}

		CleanupStack::PopAndDestroy();
		}
	else
		{
		ERR_PRINTF2(_L("Failed to create prompt process with error %d"), err);
		SetBlockResult(EFail);
		}
	}
	
TBool CT_FsData::GetDriveAttsFromConfig(const TDesC& aSection, TUint& aDriveAtt)
	{
	aDriveAtt = 0;

	TPtrC	driveAttStr;
	TBool ret = GET_OPTIONAL_STRING_PARAMETER(KDriveAtt(), aSection, driveAttStr);
	if ( ret )
		{
		if ( !ConvertToDriveAtts(driveAttStr, aDriveAtt) )
			{
			TInt	intTemp;
			ret=GET_MANDATORY_INT_PARAMETER(KDriveAtt(), aSection, intTemp);
			if ( ret )
				{
				aDriveAtt=intTemp;
				}
			}
		}

	return ret;
	}

TBool CT_FsData::GetBatteryStateFromConfig(const TDesC& aSection, TBatteryState& aBatteryState)
	{
	// Read media type from INI file
	aBatteryState = EBatLow;

	TPtrC batteryStateStr;
	TBool ret = GET_OPTIONAL_STRING_PARAMETER(KBatteryState(), aSection, batteryStateStr);

	if (ret)
		{
		if (batteryStateStr == KBatNotSupported)
			{
			aBatteryState = EBatNotSupported;
			}
		else if (batteryStateStr == KBatGood)
			{
			aBatteryState = EBatGood;
			}

		if (batteryStateStr == KBatLow)
			{
			aBatteryState = EBatLow;
			}
		else
			{
			aBatteryState = EBatNotSupported;
			}
		}

	return ret;
	}

TBool CT_FsData::ConvertToMediaAtts(const TDesC& aMediaAttStr, TUint& aMediaAtt)
	{
	TBool	ret = ETrue;
	if ( aMediaAttStr==KMediaAttVariableSizeStr )
		{
		aMediaAtt=KMediaAttVariableSize;
		}
	else if ( aMediaAttStr==KMediaAttDualDensityStr )
		{
		aMediaAtt=KMediaAttDualDensity;
		}
	else if ( aMediaAttStr==KMediaAttFormattableStr )
		{
		aMediaAtt=KMediaAttFormattable;
		}
	else if ( aMediaAttStr==KMediaAttWriteProtectedStr )
		{
		aMediaAtt=KMediaAttWriteProtected;
		}
	else if ( aMediaAttStr==KMediaAttLockableStr )
		{
		aMediaAtt=KMediaAttLockable;
		}
	else if ( aMediaAttStr==KMediaAttLockedStr )
		{
		aMediaAtt=KMediaAttLocked;
		}
	else
		{
		TInt	location = aMediaAttStr.Match(_L("*|*"));
		if( location!=KErrNotFound )
			{
			//Converting Left part of the data
			TPtrC	tempStr = aMediaAttStr.Left(location);
			ret=ConvertToMediaAtts(tempStr, aMediaAtt);

			//Converting right data can be with another "|"
			tempStr.Set(aMediaAttStr.Mid(location+1));

			TUint	mediaAttTmp;
			if ( ConvertToMediaAtts(tempStr, mediaAttTmp) )
				{
				aMediaAtt=aMediaAtt|mediaAttTmp;
				}
			else
				{
				ret=EFalse;
				}
			}
		else
			{
			ret=EFalse;
			}
		}

	return ret;
	}

TBool CT_FsData::GetMediaAttsFromConfig(const TDesC& aSection, TUint& aMediaAtt)
	{
	aMediaAtt = 0;

	TPtrC	mediaAttStr;
	TBool	ret = GET_OPTIONAL_STRING_PARAMETER(KMediaAtt(), aSection, mediaAttStr);
	if (ret)
		{
		if ( !ConvertToMediaAtts(mediaAttStr, aMediaAtt) )
			{
			TInt	intTemp;
	 		ret=GET_MANDATORY_INT_PARAMETER(KMediaAtt(), aSection, intTemp);
			if ( ret )
				{
				aMediaAtt=intTemp;
				}
			}
		}

	return ret;
	}

void CT_FsData::DoCmdDriveList(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::DriveList()."));

	//Get the list of drives which presents in system. There should be 26 slots where each non-zero slot means a drive in system.
	TDriveList driveList;
	TInt errorCode = iFs->DriveList(driveList);

	//Check error code.
	if (errorCode != KErrNone)
		{
		ERR_PRINTF2(_L("Function DriveList() failed with %d error code."), errorCode);
		SetError(errorCode);
		}
	else
		{
		//Get data from config
		//Get first drive index from config.
		TInt arraySize = 0;
		TLex lexer;
		if (GET_MANDATORY_INT_PARAMETER(KArraySize(), aSection, arraySize))
			{

			TInt arrayIndex = 0;
			for(TInt i = 0; i < arraySize; i++)
				{
				TBuf<KBufferStringLength>tmpBuff;

				tmpBuff.Append(KArrayIndex);
				tmpBuff.AppendNum(i+1);
				if (GET_MANDATORY_INT_PARAMETER( tmpBuff, aSection, arrayIndex))
					{

					//Check that drive C and drive Z or others what realy present are in list. Also user can change the drives and check their presence.
					if (driveList[arrayIndex] == 0)
						{
						ERR_PRINTF2(_L(" Disk no. %d not present."), arrayIndex);
						SetBlockResult(EFail);
						}
					}
				}

			}
		}
	}


void CT_FsData::DoCmdDriveL(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::Drive()."));

	//Get drive index from config.
	TDriveNumber	driveIndex;
	TBool			nonDefault = GetDriveNumberFromConfig(aSection, KDrive(), driveIndex);

	//Get info about drive.
	TDriveInfo		driveInfo;
	TInt			errorCode;
	if(nonDefault)
		{
		errorCode = iFs->Drive(driveInfo, driveIndex);
		}
	else
		{
		errorCode = iFs->Drive(driveInfo);
		}
		
	if(errorCode == KErrNone)
		{
		if ( !VerifyTDriveInfoDataFromIniL(aSection, driveInfo))
			{
			SetBlockResult(EFail);
			}
		}
	else
		{
		ERR_PRINTF2(_L("Function Drive() failed with %d error code"), errorCode);
		SetError(errorCode);
		}	
	}
	
TBool CT_FsData::VerifyTDriveInfoDataFromIniL(const TDesC& aSection, TDriveInfo& aDriveInfo)
	{
	TBool ret = ETrue;
	
	INFO_PRINTF2(_L("TDriveInfo.iType    =%d"), aDriveInfo.iType);
	INFO_PRINTF2(_L("TDriveInfo.iBattery =%d"), aDriveInfo.iBattery);
	INFO_PRINTF2(_L("TDriveInfo.iDriveAtt=0x%X"), aDriveInfo.iDriveAtt);
	TUint	driveAttMask[]=
		{
		KDriveAttLocal,
		KDriveAttRom,
		KDriveAttRedirected,
		KDriveAttSubsted,
		KDriveAttInternal,
		KDriveAttRemovable,
		};
	TPtrC	driveAttText[]=
		{
		KDriveAttLocalStr(),
		KDriveAttRomStr(),
		KDriveAttRedirectedStr(),
		KDriveAttSubstedStr(),
		KDriveAttInternalStr(),
		KDriveAttRemovableStr(),
		};
	TInt	size = sizeof(driveAttMask) / sizeof(driveAttMask[0]);
	TInt	index;
	for ( index=0; index<size; ++index )
		{
		if ( aDriveInfo.iDriveAtt&driveAttMask[index] )
			{
			INFO_PRINTF2(_L("DriveAtt %S ON"), &driveAttText[index]);
			}
		}
	INFO_PRINTF2(_L("TDriveInfo.iMediaAtt=0x%X"), aDriveInfo.iMediaAtt);
	TUint	mediaAttMask[]=
		{
		KMediaAttVariableSize,
		KMediaAttDualDensity,
		KMediaAttFormattable,
		KMediaAttWriteProtected,
		KMediaAttLockable,
		KMediaAttLocked,
		};
	TPtrC	mediaAttText[]=
		{
		KMediaAttVariableSizeStr(),
		KMediaAttDualDensityStr(),
		KMediaAttFormattableStr(),
		KMediaAttWriteProtectedStr(),
		KMediaAttLockableStr(),
		KMediaAttLockedStr(),
		};

	size = sizeof(mediaAttMask) / sizeof(mediaAttMask[0]);
	for ( index=0; index<size; ++index )
		{
		if ( aDriveInfo.iMediaAtt&mediaAttMask[index] )
			{
			INFO_PRINTF2(_L("MediaAtt %S ON"), &mediaAttText[index]);
			}
		}

	//Get drive type from config.
	TMediaType	mediaType;
	if (GetMediaTypeFromConfig(aSection, mediaType))
		{
		//Checking that type of drive is equal to passed through config file.
		if (aDriveInfo.iType != mediaType)
			{
			ERR_PRINTF3(_L("Drive has wrong type. actual: %d, expected: %d"), aDriveInfo.iType, mediaType);
			ret = EFalse;
			}
		}
	
	//Optional battery state checking
	TBatteryState	batteryState;
	if (GetBatteryStateFromConfig(aSection, batteryState))
		{
		//Checking that type of drive is equal to passed through config file.
		if (aDriveInfo.iBattery != batteryState)
			{
			ERR_PRINTF3(_L("BatteryState is wrong. actual: %d, expected: %d"), aDriveInfo.iBattery, batteryState);
			ret = EFalse;
			}
		}
	
	//Optional media attributes checking
	TUint	mediaAtts;
	if (GetMediaAttsFromConfig(aSection, mediaAtts))
		{
		//Checking that type of drive is equal to passed through config file.
		if ( (aDriveInfo.iMediaAtt&mediaAtts)==mediaAtts )
			{
			ERR_PRINTF1(_L("Media attributes are wrong"));
			ret = EFalse;
			}
		}

	//Optional drive attributes checking.
	TUint	driveAtts;
	if (GetDriveAttsFromConfig(aSection, driveAtts))
		{
		//Checking that type of drive is equal to passed through config file.
		if ( (aDriveInfo.iDriveAtt!=driveAtts)==driveAtts )
			{
			ERR_PRINTF1(_L("Drive attributes are wrong"));
			ret = EFalse;
			}
		}
	
	return ret;
	}

void CT_FsData::DoCmdVolumeL(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs:: Volume()."));

	//Get drive index from config.
	TDriveNumber driveIndex;

	TBool nonDefault = GetDriveNumberFromConfig(aSection, KDrive(), driveIndex);

	//Get info about volume.
	TInt	errorCode;

	if(nonDefault)
		{
		errorCode = iFs->Volume(iVolumeInfo, driveIndex);
		}
	else
		{
		errorCode = iFs->Volume(iVolumeInfo);
		}
	//Check error code.
	if (errorCode != KErrNone)
		{
		ERR_PRINTF2(_L("Function Volume() failed with %d error code"), errorCode);
		SetError(errorCode);
		}
	else
		{
		if ( !FileserverUtil::VerifyTVolumeInfoDataFromIniL(*this, aSection, iVolumeInfo))
			{
			SetBlockResult(EFail);
			}
		
		if ( !VerifyTDriveInfoDataFromIniL(aSection, iVolumeInfo.iDrive))
			{
			SetBlockResult(EFail);
			}	
		
			
		TBool	save;
		if(GET_OPTIONAL_BOOL_PARAMETER(KSaveInInstance(), aSection, save))
			{
			if(save)
				{
				iVolumeLabel.Copy(iVolumeInfo.iName);
				}
			}
	
		TPtrC		volumeObjectName;
		if (GET_OPTIONAL_STRING_PARAMETER(KVolumeObject, aSection, volumeObjectName))
			{
			CT_VolumeInfoData* volumeWrapperObject = NULL;
			volumeWrapperObject = static_cast<CT_VolumeInfoData*>(GetDataWrapperL(volumeObjectName));
			if(volumeWrapperObject)
				{
				TVolumeInfo* volumeObject = new(ELeave) TVolumeInfo();
				*volumeObject = iVolumeInfo;
				volumeWrapperObject->SetObjectL(volumeObject);
				}
			}
		}

	}


void CT_FsData::DoCmdSetVolumeLabel(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs:: SetVolumeLabel()."));

	//Get drive index from config.
	TDriveNumber	driveIndex;
	TBool			nonDefault = GetDriveNumberFromConfig(aSection, KDrive(), driveIndex);

	//Get volume name from config.
	TPtrC	volumeLabel;
	if (!GET_OPTIONAL_STRING_PARAMETER(KVolumeLabel(), aSection, volumeLabel))
		{
		volumeLabel.Set(iVolumeLabel);
		}

	//Trying to set the volume label. By some reason it always retturn KErrNotSupported.
	TInt	errorCode;
	if(nonDefault)
		{
		errorCode = iFs->SetVolumeLabel(volumeLabel, driveIndex);
		}
	else
		{
		errorCode = iFs->SetVolumeLabel(volumeLabel);
		}

	//Check error code.
	if (errorCode != KErrNone)
		{
		ERR_PRINTF2(_L("Function SetVolumeLabel() failed with %d error code"), errorCode);
		SetError(errorCode);
		}
	}


void CT_FsData::DoCmdSubst(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs:: Subst()."));

	//Get drive index from config.
	TDriveNumber driveIndex;


	TBool	nonDefault = GetDriveNumberFromConfig(aSection, KDrive(), driveIndex);


	TBuf16<KBufferStringLength> substPath;

	//Get the path to substed drive.

	TInt errorCode;
	if(nonDefault)
		{
		errorCode = iFs->Subst(substPath, driveIndex);
		}
	else
		{
		errorCode = iFs->Subst(substPath);
		}

	//Check error code.
	if (errorCode != KErrNone)
		{
		ERR_PRINTF2(_L("Function Subst() failed with %d error code"), errorCode);
		SetError(errorCode);
		}
	else
		{
		//Get drive index from config.
		TPtrC substPathFromConf;
		if (GET_MANDATORY_STRING_PARAMETER(KSubstPath(), aSection, substPathFromConf))
			{

			//As long as it has to be in similar format we must convert substPathFromConf to TBuf16<255>

			//Comparing paths.
			if (substPathFromConf.CompareF(substPath) != 0)
				{
				ERR_PRINTF2(_L("Subst path is wrong %S"), &substPathFromConf);
				SetBlockResult(EFail);
				}
			}
		}


	}


void CT_FsData::DoCmdSetSubst(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs:: SetSubst()."));


	//Get drive index from config.
	TDriveNumber driveIndex;
	TBool nonDefault = GetDriveNumberFromConfig(aSection, KDrive(), driveIndex);


	//Get drive index from config.
	TPtrC substPath;
	if (GET_MANDATORY_STRING_PARAMETER(KSubstPath(), aSection, substPath))
		{

	//Substing the drive with index driveIndex to path substPath.
		TInt errorCode;
		if(nonDefault)
			{
			errorCode = iFs->SetSubst(substPath, driveIndex);
			}
		else
			{
			errorCode = iFs->SetSubst(substPath);
			}

		//Check error code.
		if (errorCode != KErrNone)
			{
			ERR_PRINTF2(_L("Function SetSubst() failed with %d error code"), errorCode);
			SetError(errorCode);
			}

		}



	}


void CT_FsData::DoCmdGetMediaSerialNumber(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::GetMediaSerialNumber()."));

	//Get drive index from config.
	TDriveNumber driveIndex;
	if (GetDriveNumberFromConfig(aSection, KDrive(), driveIndex))
		{
		//Get the media serial number.
		TMediaSerialNumber	mediaSerialNumber;
		TInt				errorCode = iFs->GetMediaSerialNumber(mediaSerialNumber, driveIndex);

		//Check error code.
		if (errorCode != KErrNone)
			{
			ERR_PRINTF2(_L("Function GetMediaSerialNumber() failed with %d error code"), errorCode);
			SetError(errorCode);
			}
		}
	else
		{
		ERR_PRINTF2(_L("Not found %S parameter"), &KDrive());
		SetBlockResult(EFail);
		}

	}


void CT_FsData::DoCmdIsValidDrive(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::IsValidDirve()."));

	//Get drive index from config.
	TDriveNumber driveIndex;
	if (GetDriveNumberFromConfig(aSection, KDrive(), driveIndex))
		{
		TBool	actualValid = RFs::IsValidDrive(driveIndex);
		INFO_PRINTF2(_L("iFs->IsValidDrive = %d"), actualValid);

		//Get drive validity flag rom config.
		TBool	expectedValid;
		if ( GET_OPTIONAL_BOOL_PARAMETER(KDriveIsValid(), aSection, expectedValid) )
			{
			if ( actualValid!=expectedValid )
				{
				ERR_PRINTF1(_L("Function IsValid() returned unexpected result"));
				SetBlockResult(EFail);
				}
			}
		}
	else
		{
		ERR_PRINTF2(_L("Not found %S parameter"), &KDrive());
		SetBlockResult(EFail);
		}
	}


void CT_FsData::DoCmdCharToDrive(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs:: CharToDrive()."));

	//Get drive index from config.
	//We meed integer there, because function uses TInt, not TDriveNumber.

	TDriveNumber driveIndex;


	//Get drive char from config.
	TPtrC driveCharTmp;
	if (GET_MANDATORY_STRING_PARAMETER(KDriveChar(), aSection, driveCharTmp))
		{

		TChar driveChar = driveCharTmp.Ptr()[0];
		TInt driveIndex2;
		TInt errorCode = RFs::CharToDrive(driveChar, driveIndex2);

		//Check error code.
		if (errorCode != KErrNone)
			{
			ERR_PRINTF2(_L("Function CharToDrive() failed with %d error code"), errorCode);
			SetError(errorCode);
			}
		else
			{
			//Get drive index from config
			if (GetDriveNumberFromConfig(aSection, KDriveIndex(), driveIndex))
				{
				if (driveIndex != driveIndex2)
					{
					ERR_PRINTF1(_L("Wrong conversion from char to index"));
					SetBlockResult(EFail);
					}
				}
			}
		}
	else
		{
		ERR_PRINTF2(_L("Not found %S parameter"), &KDrive());
		SetBlockResult(EFail);
		}
	}


void CT_FsData::DoCmdDriveToChar(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs:: DriveToChar()."));

	TPtrC driveCharTmp;

	//Get drive path from config.
	TDriveNumber	driveIndex;
	if (GetDriveNumberFromConfig(aSection, KDriveIndex(), driveIndex))
		{

		TChar	driveChar2;
		TInt	errorCode = RFs::DriveToChar(driveIndex, driveChar2);
		//Check error code.

		if (errorCode != KErrNone)
			{
			ERR_PRINTF2(_L("Function DriveToChar() failed with %d error code"), errorCode);
			SetError(errorCode);
			}
		else
			{
			INFO_PRINTF3(_L("DriveToChar(%d) = %c"), driveIndex, TUint(driveChar2));

			//Get drive char from config.
			if (GET_OPTIONAL_STRING_PARAMETER(KDriveChar(), aSection, driveCharTmp))
				{

				TChar driveChar= driveCharTmp.Ptr()[0];
				if (driveChar != driveChar2)
					{
					ERR_PRINTF1(_L("Wrong conversion from char to index"));
					SetBlockResult(EFail);
					}
				}
			}
		}
	else
		{
		ERR_PRINTF2(_L("Not found %S parameter"), &KDrive());
		SetBlockResult(EFail);
		}
	}


void CT_FsData::DoCmdCheckDisk(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs:: CheckDisk()."));

	//Get drive path from config.
	TPtrC drivePath;
	if (GET_MANDATORY_STRING_PARAMETER(KDrivePath(), aSection, drivePath))
		{

		TInt errorCode = iFs->CheckDisk(drivePath);

		//Check error code.
		if (errorCode != KErrNone)
			{
			ERR_PRINTF2(_L("Function CheckDisk() failed with %d error code"), errorCode);
			SetError(errorCode);
			}

		}

	}

void CT_FsData::DoCmdScanDrive(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs:: ScanDisk()."));

	//Get drive path from config.
	TPtrC drivePath;
	if (GET_MANDATORY_STRING_PARAMETER(KDrivePath(), aSection, drivePath))
		{

		TInt errorCode = iFs->ScanDrive(drivePath);

		//Check error code.
		if (errorCode != KErrNone)
			{
			ERR_PRINTF2(_L("Function ScanDrive() failed with %d error code"), errorCode);
			SetError(errorCode);
			}

		}

	}


void CT_FsData::DoCmdGetDriveName(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::Calls GetDriveName()"));

	//Get drive index from config.
	TDriveNumber	driveIndex;

	if (GetDriveNumberFromConfig(aSection, KDrive(), driveIndex))
		{
		TBuf<KBufferStringLength>	driveName;
		TInt		errorCode = iFs->GetDriveName(driveIndex, driveName);

		//Check error code.
		if (errorCode != KErrNone)
			{
			ERR_PRINTF2(_L("Function SetDriveLabel() failed with %d error code"), errorCode);
			SetError(errorCode);
			}
		else
			{
			//Get drive name from config.
			TPtrC driveNameFromConf;
			if(GET_OPTIONAL_STRING_PARAMETER(KDriveName(), aSection, driveNameFromConf))
				{
				TPtrC driveNamePtr = driveName;
				if (driveNamePtr != driveNameFromConf)
					{
					ERR_PRINTF3(_L("Names mismatch: %S != %S"), &driveNamePtr, &driveNameFromConf);
					SetBlockResult(EFail);
					}
				}

			TBool save;

			if(GET_OPTIONAL_BOOL_PARAMETER(KSaveInInstance(), aSection, save))
				{
				if(save)
					{
					iDriveName.Copy(iDriveName);
					}
				}
			}
		}
	else
		{
		ERR_PRINTF2(_L("Not found %S parameter"), &KDrive());
		SetBlockResult(EFail);
		}
	}


void CT_FsData::DoCmdSetDriveName(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::SetDriveName()."));

	//Get drive index from config.
	TDriveNumber	driveIndex;
	if ( GetDriveNumberFromConfig(aSection, KDrive(), driveIndex) )
		{
		//Get drive name from config.
		TPtrC	driveName;
		if ( !GET_OPTIONAL_STRING_PARAMETER(KDriveName(), aSection, driveName) )
			{
			driveName.Set(iDriveName);
			}

		//Check error code.
		TInt	errorCode = iFs->SetDriveName(driveIndex, driveName);
		if (errorCode != KErrNone)
			{
			ERR_PRINTF2(_L("Function SetDriveLabel() failed with %d error code"), errorCode);
			SetError(errorCode);
			}
		}
	else
		{
		ERR_PRINTF2(_L("Not found %S parameter"), KDriveIndex);
		SetBlockResult(EFail);
		}
	}

//
void CT_FsData::DoCmdLockDrive(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::LockDrive()."));

	//Get old password from config.
	TPtrC	drivePassword;
	if ( GET_MANDATORY_STRING_PARAMETER(KDriveOldPassword(), aSection, drivePassword) )
		{
		//Get drive index from config.
		TDriveNumber	driveIndex;
		if ( GetDriveNumberFromConfig(aSection, KDrive(), driveIndex) )
			{
			//Get new password from config.
			TPtrC	driveNewPassword;
			if ( GET_MANDATORY_STRING_PARAMETER(KDriveNewPassword(), aSection, driveNewPassword) )
				{
				// get boolean value from config
				TBool	store;
				if (GET_MANDATORY_BOOL_PARAMETER(KStore(), aSection, store))
					{
					TMediaPassword	password;
					password.Copy(driveNewPassword);
					iPassword.Copy(drivePassword);

					TInt errorCode = iFs->LockDrive(driveIndex, iPassword, password, store);

					//Check error code.
					if (errorCode == KErrNone)
						{
						iPassword=password;
						}
					else
						{
						ERR_PRINTF2(_L("Function LockDrive() failed with %d error code"), errorCode);
						SetError(errorCode);
						}
					}
				}
			}
		}
	}


void CT_FsData::DoCmdUnlockDrive(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::UnlockDrive()."));

	//Get old password from config.
	TPtrC	drivePassword;
	if ( GET_MANDATORY_STRING_PARAMETER(KDriveOldPassword(), aSection, drivePassword) )
		{
		//Get drive index from config.
		TDriveNumber	driveIndex;
		if (GetDriveNumberFromConfig(aSection, KDrive(), driveIndex))
			{
			// get boolean value from config
			TBool	store;
			if (GET_MANDATORY_BOOL_PARAMETER(KStore(), aSection, store))
				{
				TMediaPassword	password;
				//Converting string to appropriative password format(TMediaPassword is typedef for TBuf<16>)
				password.Copy(drivePassword);

				TInt	errorCode = iFs->UnlockDrive(driveIndex, password, store);

				//Check error code.
				if (errorCode == KErrNone)
					{
					iPassword=password;
					}
				else
					{
					ERR_PRINTF2(_L("Function UnlockDrive() failed with %d error code"), errorCode);
					SetError(errorCode);
					}
				}
			}
		else
			{
			ERR_PRINTF2(_L("Not found %S parameter"),KDrive);
			SetBlockResult(EFail);
			}
		}
	}


void CT_FsData::DoCmdClearPassword(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::ClearPassword()."));

	//Get old password from config.
	TPtrC	driveOldPasswordTmp;
	if (GET_MANDATORY_STRING_PARAMETER(KDriveOldPassword(), aSection, driveOldPasswordTmp))
		{
		//Get drive index from config.
		TDriveNumber	driveIndex;
		if (GetDriveNumberFromConfig(aSection, KDrive(), driveIndex))
			{
			//Converting string to appropriative password format(TMediaPassword is typedef for TBuf<16>)
			TMediaPassword	driveOldPassword;
			driveOldPassword.Copy(driveOldPasswordTmp);

			TInt	errorCode = iFs->ClearPassword(driveIndex, driveOldPassword);
			if (errorCode != KErrNone)
				{
				ERR_PRINTF2(_L("Function ClearPassword() failed with %d error code"), errorCode);
				SetError(errorCode);
				}
			}
		else
			{
			ERR_PRINTF2(_L("Not found %S parameter"),KDrive);
			SetBlockResult(EFail);
			}
		}
	}


void CT_FsData::DoCmdErasePassword(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::ErasePassword()."));

	//Get drive from config.
	TDriveNumber	driveIndex;
	if (GetDriveNumberFromConfig(aSection, KDrive(), driveIndex))
		{
		TInt	errorCode = iFs->ErasePassword(driveIndex);

		//Check error code.
		if (errorCode != KErrNone)
			{
			ERR_PRINTF2(_L("Function ErasePassword() failed with %d error code"), errorCode);
			SetError(errorCode);
			}
		}
	else
		{
		ERR_PRINTF2(_L("Not found %S parameter"), &KDrive());
		SetBlockResult(EFail);
		}
	}


void CT_FsData::DoCmdReserveDriveSpace(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::ReserveDriveSpace()."));

	//Get amount of bytes to be reserved from config.
	TInt	bytesToReserve;
	if (GET_MANDATORY_INT_PARAMETER(KBytesToReserve(), aSection, bytesToReserve))
		{
		TDriveNumber	driveIndex;

		if (GetDriveNumberFromConfig(aSection, KDrive(), driveIndex))
			{
			TInt	errorCode = iFs->ReserveDriveSpace(driveIndex, bytesToReserve);

			//Check error code.
			if (errorCode != KErrNone)
				{
				ERR_PRINTF2(_L("Function ReserveDriveSpace() failed with %d error code"), errorCode);
				SetError(errorCode);
				}
			}
		else
			{
			ERR_PRINTF2(_L("Not found %S parameter"),KDrive);
			SetBlockResult(EFail);
			}
		}
	}


void CT_FsData::DoCmdGetReserveAccess(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::GetReserveAccess()."));

	//Get drive index from config.
	TDriveNumber	driveIndex;
	if (GetDriveNumberFromConfig(aSection, KDrive(), driveIndex))
		{
		TInt	errorCode = iFs->GetReserveAccess(driveIndex);

		//Check error code.
		if (errorCode != KErrNone)
			{
			ERR_PRINTF2(_L("Function GetReserveAccess() failed with %d error code"), errorCode);
			SetError(errorCode);
			}
		}
	else
		{
		ERR_PRINTF2(_L("Not found %S parameter"), &KDrive());
		SetBlockResult(EFail);
		}
	}


void CT_FsData::DoCmdReleaseReserveAccess(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::ReleaseReserveAccess()."));

	//Get drive index from config.
	TDriveNumber	driveIndex;
	if (GetDriveNumberFromConfig(aSection, KDrive(), driveIndex))
		{
		TInt	errorCode = iFs->ReleaseReserveAccess(driveIndex);

		//Check error code.
		if (errorCode != KErrNone)
			{
			ERR_PRINTF2(_L("Function ReleaseReserveAccess() failed with %d error code"), errorCode);
			SetError(errorCode);
			}
		}
	else
		{
		ERR_PRINTF2(_L("Not found %S parameter"), &KDrive());
		SetBlockResult(EFail);
		}
	}

void CT_FsData::DoCmdGetSystemDrive(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::GetSystemDrive()."));

	TDriveNumber	driveNumber = RFs::GetSystemDrive();
	INFO_PRINTF2(_L("System Drive = %d"), driveNumber);

	//Get drive index from config.
	TDriveNumber	driveIndex;
	if ( GetDriveNumberFromConfig(aSection, KDrive(), driveIndex) )
		{
		//Check error code.
		if (driveNumber != driveIndex)
			{
			ERR_PRINTF1(_L("GetSystemDrive() returned unexpected drive"));
			SetBlockResult(EFail);
			}
		}
	else
		{
		ERR_PRINTF2(_L("Not found %S parameter"), &KDrive());
		SetBlockResult(EFail);
		}
	}

void CT_FsData::DoCmdSetSystemDrive(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::SetSystemDrive()"));

	// get drive number from parameters
	TDriveNumber	driveNumber = EDriveA;
	if (GetDriveNumberFromConfig(aSection, KDrive(), driveNumber))
		{
		TInt	err = iFs->SetSystemDrive(driveNumber);
		// check error code
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("SetSystemDrive() error: %d"), err);
			SetError(err);
			}
		}
	else
		{
		ERR_PRINTF2(_L("No %S"), &KDrive());
		SetBlockResult(EFail);
		}

	}

void CT_FsData::DoCmdGetSystemDriveChar(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::GetSystemDriveChar()"));

	TChar	drvChar = RFs::GetSystemDriveChar();
	INFO_PRINTF2(_L("GetSystemDriveChar() = %c"), TUint(drvChar));
	//Get drive char from config.
	TPtrC	driveCharTmp;
	if (GET_OPTIONAL_STRING_PARAMETER(KDriveChar(), aSection, driveCharTmp))
		{
		TChar	driveCharExpect = driveCharTmp.Ptr()[0];
		driveCharExpect.UpperCase();
		drvChar.UpperCase();
		if ( drvChar != driveCharExpect)
			{
			ERR_PRINTF1(_L("Drive char != expected drive char"));
			SetBlockResult(EFail);
			}
		}
	}

void CT_FsData::DoCmdVolumeIOParam(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Calls RFs::VolumeIOParam()"));

	// get drive number from parameters
	TDriveNumber	driveNumber = EDriveA;
	if (GetDriveNumberFromConfig(aSection, KDrive(), driveNumber))
		{
		TVolumeIOParamInfo	volumeIOParamInf;
		TInt				err = iFs->VolumeIOParam(driveNumber, volumeIOParamInf);
		// check error code
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("VolumeIOParam()  error: %d"), err);
			SetError(err);
			}
		else
			{
			INFO_PRINTF2(_L("VolumeIOParam BlockSize: %d"), volumeIOParamInf.iBlockSize );
			INFO_PRINTF2(_L("VolumeIOParam ClusterSize: %d"), volumeIOParamInf.iClusterSize );
			INFO_PRINTF2(_L("VolumeIOParam RecReadBufSize: %d"), volumeIOParamInf.iRecReadBufSize );
			INFO_PRINTF2(_L("VolumeIOParam RecWriteBufSize: %d"), volumeIOParamInf.iRecWriteBufSize );
			}
		}
	else
		{
		ERR_PRINTF2(_L("No %S"), &KDrive());
		SetBlockResult(EFail);
		}
	}


