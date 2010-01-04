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
#include "T_SfSrvServer.h"
#include "DataWrapperBase.h"

/*@{*/

_LIT(KMessageSlots,							"messageSlots");
_LIT(KPath,									"path");
_LIT(KAddress,								"address");
_LIT(KType,									"type");
_LIT(KAll,									"all");
_LIT(KResources,							"resources");
_LIT(KNotify,								"notify");
_LIT(KInRom,								"inRom");
_LIT(KThreshold,							"threshold");
_LIT(KDrive,								"drive");
_LIT(KFunctionId,							"functionId");
_LIT(KAdd,									"add");
_LIT(KSub,									"sub");
_LIT(KUseFreeSpace,							"use_free_space");
_LIT(KVersionName,							"name");
_LIT(KVersionBuild,							"build");
_LIT(KVersionMajor,							"major");
_LIT(KVersionMinor,							"minor");
_LIT(KIndex,								"index");
_LIT(KParam1,								"param1");
_LIT(KParam2,								"param2");
_LIT(KFile,									"file");
_LIT(KCount,								"count");

///	Commands
_LIT(KCmdNew,								"new");
_LIT(KCmdClose,								"Close");
_LIT(KCmdDestructor,						"~");
_LIT(KCmdConnect,							"Connect");
_LIT(KCmdVersion,							"Version");
_LIT(KCmdNotifyChange,						"NotifyChange");
_LIT(KCmdNotifyChangeCancel,				"NotifyChangeCancel");
_LIT(KCmdNotifyDiskSpace,					"NotifyDiskSpace");
_LIT(KCmdNotifyDiskSpaceCancel,				"NotifyDiskSpaceCancel");
_LIT(KCmdIsRomAddress,						"IsRomAddress");
_LIT(KCmdResourceCountMarkStart,			"ResourceCountMarkStart");
_LIT(KCmdResourceCountMarkEnd,				"ResourceCountMarkEnd");
_LIT(KCmdResourceCount,						"ResourceCount");
_LIT(KCmdGetNotifyUser,						"GetNotifyUser");
_LIT(KCmdSetNotifyUser,						"SetNotifyUser");
_LIT(KCmdSetNotifyChange,					"SetNotifyChange");
_LIT(KCmdLoaderHeapFunction,				"LoaderHeapFunction");
_LIT(KCmdInitialisePropertiesFile,			"InitialisePropertiesFile");
/*@}*/

CT_FsData* CT_FsData::NewL()
/**
 * Two phase constructor
 */
	{
	CT_FsData*	ret = new (ELeave) CT_FsData();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;	
	}

CT_FsData::CT_FsData()
/**
 * Protected constructor. First phase construction
 */
	{
	}

void CT_FsData::ConstructL()
/**
 * Protected constructor. Second phase construction
 */
	{
	
	}

CT_FsData::~CT_FsData()
/**
 * Destructor.
 */
	{
	DoCleanup();

	iNotifyChange.ResetAndDestroy();
	iNotifyDiskSpace.ResetAndDestroy();
	iNotifyDismount.ResetAndDestroy();
	iStartupInitComplete.ResetAndDestroy();
	}

void CT_FsData::DoCleanup()
/**
 * Contains cleanup implementation
 */
	{
	//Deleting RFs.
	if(iFs != NULL)
		{
		INFO_PRINTF1(_L("Deleting current RFs"));
		delete iFs;
		iFs = NULL;
		}
	}

TAny* CT_FsData::GetObject()
/**
 * Return a pointer to the object that the data wraps
 *
 * @return pointer to the object that the data wraps
 */
	{
	return iFs;
	}

TUint64 CT_FsData::ThreadId()
	{
	RThread currentThread;
	return currentThread.Id().Id();
	}

TBool CT_FsData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex)
/**
 * Process a command read from the ini file
 *
 * @param aCommand	the command to process
 * @param aSection		the entry in the ini file requiring the command to be processed
 *
 * @return ETrue if the command is processed
 */
	{
	TBool retVal = ETrue;
	if (aCommand == KCmdNew)
		{
		DoCmdNewL();
		}
	else if (aCommand == KCmdClose)
		{
		DoCmdClose();
		}
	else if (aCommand == KCmdDestructor)
		{
		DoCmdDestructor();
		}
	else if (aCommand == KCmdConnect)
		{
		DoCmdConnect(aSection);
		}
	else if (aCommand == KCmdVersion)
		{
		DoCmdVersion(aSection);
		}
	else if (aCommand == KCmdNotifyChange)
		{
		DoCmdNotifyChangeL(aSection, aAsyncErrorIndex);
		}
	else if (aCommand == KCmdNotifyChangeCancel)
		{
		DoCmdNotifyChangeCancel(aSection);
		}
	else if (aCommand == KCmdNotifyDiskSpace)
		{
		DoCmdNotifyDiskSpaceL(aSection, aAsyncErrorIndex);
		}
	else if (aCommand == KCmdNotifyDiskSpaceCancel)
		{
		DoCmdNotifyDiskSpaceCancel(aSection);
		}
	else if (aCommand == KCmdIsRomAddress)
		{
		DoCmdIsRomAddress(aSection);
		}
	else if (aCommand == KCmdResourceCountMarkStart)
		{
		DoCmdResourceCountMarkStart();
		}
	else if (aCommand == KCmdResourceCountMarkEnd)
		{
		DoCmdResourceCountMarkEnd();
		}
	else if (aCommand == KCmdResourceCount)
		{
		DoCmdResourceCount(aSection);
		}
	else if (aCommand == KCmdGetNotifyUser)
		{
		DoCmdGetNotifyUser(aSection);
		}
	else if (aCommand == KCmdSetNotifyUser)
		{
		DoCmdSetNotifyUser(aSection);
		}
	else if (aCommand == KCmdLoaderHeapFunction)
		{
		DoCmdLoaderHeapFunctionL(aSection);
		}
	else if (aCommand == KCmdSetNotifyChange)
		{
		DoCmdSetNotifyChange(aSection);
		}
	else if (aCommand == KCmdInitialisePropertiesFile)
		{
		DoCmdInitialisePropertiesFile(aSection);
		}
	else
		{
		if (!DoCommandDrivesL(aCommand, aSection, aAsyncErrorIndex))
			{
			if (!DoCommandMountsL(aCommand, aSection, aAsyncErrorIndex))
				{
				if (!DoCommandFilesL(aCommand, aSection, aAsyncErrorIndex))
					{
					retVal = EFalse;
					}
				}
			}
		}
	return retVal;
	}


void CT_FsData::DoCmdNewL()
/** Creates new RFs class instance */
	{
	INFO_PRINTF1(_L("Create new RFs class instance"));
	
	//Deletes previous RFs class instance if it was already created.
	DoCleanup();

	// do create
	TRAPD(err, iFs = new (ELeave) RFs());
	if ( err!=KErrNone )
		{
		ERR_PRINTF2(_L("new error %d"), err);
		SetError( err );
		}
	}

void CT_FsData::DoCmdClose()
/** Close RFs handle */
	{
	INFO_PRINTF1(_L("Closing RFs"));
	iFs->Close();
	}


void CT_FsData::DoCmdDestructor()
/** Destroy RFs the object */
	{
	INFO_PRINTF1(_L("Destroying the RFs object"));
	DoCleanup();
	}

void CT_FsData::DoCmdConnect(const TDesC& aSection)
/** Connects a client to the file server */
	{

	TInt	messageSlots = KFileServerDefaultMessageSlots;
	GetIntFromConfig(aSection, KMessageSlots, messageSlots);

	TInt err = iFs->Connect(messageSlots);
	INFO_PRINTF2(_L("Call Connect(messageSlots = %d)"), messageSlots);

	if ( err!=KErrNone )
		{
		ERR_PRINTF2(_L("Connect() error %d"), err);
		SetError( err );
		}
	}

void CT_FsData::DoCmdVersion(const TDesC& aSection)
/** Gets the client side version number */
	{
	
	TVersion		version=iFs->Version();
	TVersionName	versionName	= version.Name();
	INFO_PRINTF2(_L("Version name  : %S"), &versionName);
	INFO_PRINTF2(_L("Version build : %d"), (TInt)version.iBuild);
	INFO_PRINTF2(_L("Version major : %d"), (TInt)version.iMajor);
	INFO_PRINTF2(_L("Version minor : %d"), (TInt)version.iMinor);

	TPtrC	name;
	if ( GET_OPTIONAL_STRING_PARAMETER(KVersionName, aSection, name))
		{
		if ( name != version.Name() )
			{
			ERR_PRINTF3(_L("Name does not match expected name: %S, %S"), &name, &versionName);
			SetBlockResult(EFail);
			}
		}

	TInt	intTemp;
	if ( GET_OPTIONAL_INT_PARAMETER(KVersionBuild, aSection, intTemp) )
		{
		if ( intTemp != version.iBuild )
			{
			ERR_PRINTF1(_L("Build does not match expected build"));
			SetBlockResult(EFail);
			}
		}

	if ( GET_OPTIONAL_INT_PARAMETER(KVersionMajor, aSection, intTemp) )
		{
		if ( intTemp != version.iMajor )
			{
			ERR_PRINTF1(_L("Major does not match expected major"));
			SetBlockResult(EFail);
			}
		}

	if ( GET_OPTIONAL_INT_PARAMETER(KVersionMinor, aSection, intTemp) )
		{
		if ( intTemp != version.iMinor )
			{
			ERR_PRINTF1(_L("Minor does not match expected minor"));
			SetBlockResult(EFail);
			}
		}

	if (version.Name() == _L("") && version.iBuild == 0 && version.iMajor == 0 && version.iMinor == 0)
		{
		ERR_PRINTF1(_L("Some version fields are not set!"));
		SetBlockResult(EFail);
		}
	}

void CT_FsData::DoCmdNotifyChangeL(const TDesC& aSection, const TInt aAsyncErrorIndex)
/** Requests a notification of change to files or directories */
	{
	TNotifyType	type = ENotifyAll;
	TPtrC		typeTemp;
	TBool		dataOk = ETrue;

	if ( GET_MANDATORY_STRING_PARAMETER(KType, aSection, typeTemp) )
		{
		if(!ConvertToNotifyType(typeTemp, type))
			{
			TInt	typeNumber;
			if(GetIntFromConfig(aSection, KType(), typeNumber))
				{
				type = (TNotifyType)typeNumber;
				}
			else
				{
				ERR_PRINTF3(_L("NotifyChange() incorrect parameter %S in %S"), &typeTemp, &KType());
				SetBlockResult(EFail);
				dataOk = EFalse;
				}
			}
		}
	else
		{
		dataOk = EFalse;
		}

	TInt	count=1;
	GET_OPTIONAL_INT_PARAMETER(KCount, aSection, count);

	if(dataOk)
		{
		CT_ActiveNotifyChange*	active = CT_ActiveNotifyChange::NewLC(count, aAsyncErrorIndex, *this);
		iNotifyChange.AppendL(active);
		CleanupStack::Pop(active);

		TPtrC	path;
		if ( GetStringFromConfig(aSection, KPath(), path) )
			{
			iFs->NotifyChange(type, active->iStatus, path);
			INFO_PRINTF1(_L("NotifyChange(TNotifyType, TRequestStatus, const TDesC)"));
			}
		else
			{
			iFs->NotifyChange(type, active->iStatus);
			INFO_PRINTF1(_L("NotifyChange(TNotifyType TRequestStatus)"));
			}
			
		active->Activate();
		IncOutstanding();
		}
	}


void CT_FsData::DoCmdNotifyChangeCancel(const TDesC& aSection)
/** Cancels outstanding request(s) for notification of change to files or directories */
	{
	TBool	all = ETrue;

	if ( GET_MANDATORY_BOOL_PARAMETER(KAll, aSection, all) )
		{
		if(all)
			{
			// Cancels all outstanding requests.
			iFs->NotifyChangeCancel();
			INFO_PRINTF1(_L("NotifyChangeCancel()"));
			}
		else
			{
			// Cancels a specific outstanding request
			TInt	index=0;
			GetIntFromConfig(aSection, KIndex(), index);

			iFs->NotifyChangeCancel(iNotifyChange[index]->iStatus);
			INFO_PRINTF1(_L("NotifyChangeCancel(TRequestStatus)"));
			}
		}
	}

void CT_FsData::DoCmdNotifyDiskSpaceL(const TDesC& aSection, const TInt aAsyncErrorIndex)
/** Requests notification when the free disk space on the specified drive crosses the specified threshold value */
	{
	//Get drive number from config
	TDriveNumber	drive = EDriveA;
	TBool 			dataOk = ETrue;
	if (!GetDriveNumberFromConfig(aSection, KDrive, drive))
		{
		ERR_PRINTF2(_L("No %S"), &KDrive());
		SetBlockResult(EFail);
		dataOk = EFalse;
		}

	TBool	useFreeSpace = EFalse;
	GET_OPTIONAL_BOOL_PARAMETER(KUseFreeSpace, aSection, useFreeSpace);

	TInt64 threshold = 0;
	//Defining treshold by free space
	if(useFreeSpace)
		{
		threshold = iVolumeInfo.iFree;
		//For Calculation free-temp(_sub), free+temp(_add)
		TInt temp;
		if(GetIntFromConfig(aSection, KAdd, temp))
			{
			threshold += temp;
			temp = 0;
			}
		if(GetIntFromConfig(aSection, KSub, temp))
			{
			threshold -= temp;
			temp = 0;
			}
		}
	//Defining treshold by some value
	else
		{
		TPtrC thold;
		if(GET_MANDATORY_STRING_PARAMETER(KThreshold, aSection, thold))
			{
			TLex lex(thold);
			TInt err = lex.Val(threshold);
			if(err)
				{
				ERR_PRINTF1(_L("NotifyDiskSpace() bad treshold value"));
				SetBlockResult(EFail);
				dataOk = EFalse;
				}
			}
		}

	if(dataOk)
		{
		CActiveCallback*	active = CActiveCallback::NewLC(*this);
		iNotifyDiskSpace.AppendL(active);
		CleanupStack::Pop(active);

		iFs->NotifyDiskSpace(threshold, drive, active->iStatus);
		INFO_PRINTF1(_L("NotifyDiskSpace()"));
		active->Activate(aAsyncErrorIndex);
		IncOutstanding();
		}
	}


void CT_FsData::DoCmdNotifyDiskSpaceCancel(const TDesC& aSection)
/** Cancels a specific outstanding request for free disk space notification */
	{

	TBool all = ETrue;

	if(GET_MANDATORY_BOOL_PARAMETER(KAll, aSection, all))
		{
		if (all)
			{
			//Cancels all outstanding requests.
			iFs->NotifyDiskSpaceCancel();
			INFO_PRINTF1(_L("NotifyDiskSpaceCancel()"));
			}
		else
			{
			// Cancels a specific outstanding request
			TInt	index=0;
			GetIntFromConfig(aSection, KIndex, index);

			iFs->NotifyDiskSpaceCancel(iNotifyDiskSpace[index]->iStatus);
			}
		}
	}

void CT_FsData::DoCmdIsRomAddress(const TDesC& aSection)
/** Tests whether the specified address is in ROM */
	{
	//reading address
	TUint8* address = NULL;
	TInt intAddress;
	TBool dataOk = ETrue;
	
	if(GetHexFromConfig(aSection, KAddress, intAddress))
		{
		address = (TUint8*)intAddress;
		}
	else if(iIsFileInRom)
		{
		address = iIsFileInRom;
		}
	else
		{
		ERR_PRINTF1(_L("Address not specified"));
		SetBlockResult(EFail);
		dataOk = EFalse;
		}
	if(dataOk)
		{
		INFO_PRINTF2(_L("Address %X"), address);
		//Address of file in ROM
		TBool	actual = RFs::IsRomAddress(address);
		INFO_PRINTF2(_L("RFs::IsRomAddress() %d"), actual);
			//Reading expected result
		TBool	expected = EFalse;
		if ( GET_OPTIONAL_BOOL_PARAMETER(KInRom, aSection, expected) )
			{
			if ( expected != actual )
				{
				ERR_PRINTF1(_L("Expected result does not match actual"));
				SetBlockResult(EFail);
				}
			}
		}
	}

void CT_FsData::DoCmdResourceCountMarkStart()
/** Marks the start of resource count checking */
	{
	iFs->ResourceCountMarkStart();
	INFO_PRINTF1(_L("ResourceCountMarkStart()"));
	}

void CT_FsData::DoCmdResourceCountMarkEnd()
/** Ends resource count checking */
	{
	iFs->ResourceCountMarkEnd();
	INFO_PRINTF1(_L("ResourceCountMarkEnd()"));
	}

void CT_FsData::DoCmdResourceCount(const TDesC& aSection)
/** Gets the number of currently open resources */
	{
	TInt	resources = iFs->ResourceCount();//Number of resources currently opened
	INFO_PRINTF2(_L("ResourceCount() = %d"), resources);

	TInt expectedValue = 0; //Number of resources from config
	if(GetIntFromConfig(aSection, KResources, expectedValue))
		{
		if (expectedValue != resources)
			{
			ERR_PRINTF3(_L("ResourceCount() %d != %d expected value"), resources, expectedValue);
			SetBlockResult(EFail);
			}
		}
	}

void CT_FsData::DoCmdGetNotifyUser(const TDesC& aSection)
/** Tests whether user notification of file read or write failure is in effect */
	{
	TBool	actual = iFs->GetNotifyUser();
	INFO_PRINTF2(_L("iFs->GetNotifyUser() %d"), actual);

	TBool expected = EFalse;
	if ( GET_OPTIONAL_BOOL_PARAMETER(KNotify, aSection, expected) )
		{
		if ( expected!=actual )
			{
			ERR_PRINTF1(_L("Expected result does not match actual"));
			SetBlockResult(EFail);
			}
		}
	}

void CT_FsData::DoCmdSetNotifyUser(const TDesC& aSection)
/** Sets whether the user should be notified of file read or write failure */
	{
	TBool notify = EFalse;

	if(GET_MANDATORY_BOOL_PARAMETER(KNotify, aSection, notify))
		{
		iFs->SetNotifyUser(notify);
		TPtrC val = notify ? _L("TRUE") : _L("FALSE");
		INFO_PRINTF2(_L("SetNotifyUser() %S"), &val);
		}


	}

void CT_FsData::DoCmdLoaderHeapFunctionL(const TDesC& aSection)
/** returns KErrNotSupported*/
	{
	TPtrC	name;
	TAny*	param1 = NULL;
	if(GetStringFromConfig(aSection, KParam1(), name))
		{
			//param1 = iDataDictionary->GetObject(name);
		}

	TAny* param2 = NULL;
	if(GetStringFromConfig(aSection, KParam2(), name))
		{
			//param2 = iDataDictionary->GetObjectL(name);
		}

	TInt	functionId = 0;
	if(GET_MANDATORY_INT_PARAMETER(KFunctionId(), aSection, functionId))
		{
		TInt err = iFs->LoaderHeapFunction(functionId, param1, param2);
		if( err!=KErrNone )
			{
			ERR_PRINTF2(_L("LoaderHeapFunction() ERROR: %d"), err);
			SetError(err);
			}
		}
	}

void CT_FsData::DoCmdSetNotifyChange(const TDesC& aSection)
/** Enables/Disables change notification on a per-session basis. */
	{

	TBool notify = EFalse;

	if(GET_MANDATORY_BOOL_PARAMETER(KNotify, aSection, notify))
		{
		TInt err = iFs->SetNotifyChange(notify);

		TPtrC val = notify ? _L("TRUE") : _L("FALSE");
		if (err == KErrNone)
			{
			INFO_PRINTF2(_L("SetNotifyChange() %S"), &val);
			}
		else
			{
			ERR_PRINTF3(_L("SetNotifyChange() %S, Error: %d"), &val, err);
			SetError(err);
			}
		}
	}

void CT_FsData::DoCmdInitialisePropertiesFile(const TDesC& aSection)
/** Sets the F32 properties file ONLY in ESTART */
	{
	TPtrC tempStr;
	if(GET_MANDATORY_STRING_PARAMETER(KFile, aSection, tempStr))
		{
		TBuf8<KBufferStringLength>	buffer;
		buffer.Copy(tempStr);
		TPtrC8						iniFilePtr(buffer);
		INFO_PRINTF2(_L("InitialisePropertiesFile() %S"), &tempStr);

		TInt	err = iFs ->InitialisePropertiesFile(iniFilePtr);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("InitialisePropertiesFile() Error: %d"), err);
			SetError(err);
			}
		}
	}

void CT_FsData::RunL(CActive* aActive, TInt aIndex)
	{
	INFO_PRINTF1(_L("CT_FsData::RunL Called"));
	TBool	foundActiveObject = EFalse;
	TInt	index=0;
	TInt	count=0;
	TBool	completed=ETrue;

	// See if it is in iNotifyChange
	count=iNotifyChange.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iNotifyChange[index] )
			{
			INFO_PRINTF1(_L("RunL iNotifyChange called"));
			foundActiveObject = ETrue;
			if ( iNotifyChange[index]->DecCount()>0 )
				{
				completed=EFalse;
				iNotifyChange[index]->Activate();
				}
			else
				{
				iNotifyChange.Remove(index);
				}
	 		}
		}

	// See if it is in iNotifyDiskSpace
	count=iNotifyDiskSpace.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iNotifyDiskSpace[index] )
			{
			INFO_PRINTF1(_L("RunL iNotifyDiskSpace called"));
			foundActiveObject = ETrue;
			iNotifyDiskSpace.Remove(index);
	 		}
		}

	// See if it is in iNotifyDismount
	count=iNotifyDismount.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iNotifyDismount[index] )
			{
			INFO_PRINTF1(_L("RunL iNotifyDismount called"));
			foundActiveObject = ETrue;
			iNotifyDismount.Remove(index);
	 		}
		}

	// See if it is in iStartupInitComplete
	count=iStartupInitComplete.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iStartupInitComplete[index] )
			{
			INFO_PRINTF1(_L("RunL iStartupInitComplete called"));
			foundActiveObject = ETrue;
			iStartupInitComplete.Remove(index);
	 		}
		}

 	if( foundActiveObject )
 		{
		TInt	err = aActive->iStatus.Int();
		if( err != KErrNone )
			{
			ERR_PRINTF2(_L("RunL Error %d"), err);
			SetAsyncError( aIndex, err );
			}

		if ( completed )
			{
			// Reset the outstanding request state
			delete aActive;
			DecOutstanding();
			}
		}
	else
		{
 		ERR_PRINTF1(_L("Stray RunL signal"));
 		SetBlockResult(EFail);
		}
	}

void CT_FsData::DoCancel(CActive* aActive, TInt aIndex)
	{
	TBool	foundActiveObject = EFalse;

	TInt	index=0;
	TInt	count=0;

	// See if it is in iNotifyChange
	count=iNotifyChange.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iNotifyChange[index] )
			{
			INFO_PRINTF1(_L("DoCancel iNotifyChange called"));
			foundActiveObject = ETrue;
			iNotifyChange.Remove(index);
	 		}
		}

	// See if it is in iNotifyDiskSpace
	count=iNotifyDiskSpace.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iNotifyDiskSpace[index] )
			{
			INFO_PRINTF1(_L("DoCancel iNotifyDiskSpace called"));
			foundActiveObject = ETrue;
			iNotifyDiskSpace.Remove(index);
	 		}
		}

	// See if it is in iNotifyDismount
	count=iNotifyDismount.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iNotifyDismount[index] )
			{
			INFO_PRINTF1(_L("DoCancel iNotifyDismount called"));
			foundActiveObject = ETrue;
			iNotifyDismount.Remove(index);
	 		}
		}

	// See if it is in iStartupInitComplete
	count=iStartupInitComplete.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iStartupInitComplete[index] )
			{
			INFO_PRINTF1(_L("DoCancel iStartupInitComplete called"));
			foundActiveObject = ETrue;
			iStartupInitComplete.Remove(index);
	 		}
		}

 	if( foundActiveObject )
 		{
		TInt	err = aActive->iStatus.Int();
		if( err != KErrNone )
			{
			ERR_PRINTF2(_L("DoCancel Error %d"), err);
			SetAsyncError( aIndex, err );
			}

		// Reset the outstanding request state
		DecOutstanding();

		delete aActive;
		}
	else
		{
 		ERR_PRINTF1(_L("Stray DoCancel signal"));
 		SetBlockResult(EFail);
		}
	}
TBool CT_FsData::ConvertToNotifyType(const TDesC& aNotifyTypeStr, TNotifyType& aNotifyType)
{
	TBool ret = ETrue;
	if(aNotifyTypeStr == _L("ENotifyAll"))
		{
		aNotifyType = ENotifyAll;
		}
	else if(aNotifyTypeStr == _L("ENotifyEntry"))
		{
		aNotifyType = ENotifyEntry;
		}
	else if(aNotifyTypeStr == _L("ENotifyFile"))
		{
		aNotifyType = ENotifyFile;
		}
	else if(aNotifyTypeStr == _L("ENotifyDir"))
		{
		aNotifyType = ENotifyDir;
		}
	else if(aNotifyTypeStr == _L("ENotifyAttributes"))
		{
		aNotifyType = ENotifyAttributes;
		}
	else if(aNotifyTypeStr == _L("ENotifyWrite"))
		{
		aNotifyType = ENotifyWrite;
		}
	else if(aNotifyTypeStr == _L("ENotifyDisk"))
		{
		aNotifyType = ENotifyDisk;
		}
	//For COnverting data ENotifyFile|ENotifyDir
	else
		{
		TInt	location=aNotifyTypeStr.Match(_L("*|*"));
		if( location!=KErrNotFound )
			{
			//Converting Left part of the data
			TPtrC		tempStr=aNotifyTypeStr.Left(location);
			ret=ConvertToNotifyType(tempStr, aNotifyType);

			//Converting right data can be with another "|"
			tempStr.Set(aNotifyTypeStr.Mid(location+1));

			TNotifyType	notifyTypeTmp;
			if ( ConvertToNotifyType(tempStr, notifyTypeTmp) )
				{
				aNotifyType =(TNotifyType)((TInt)aNotifyType|(TInt)notifyTypeTmp);
				}
			else
				{
				ret = EFalse;
				}
			}
		else
			{
			ret = EFalse;
			}
		}

	return ret;
	}
