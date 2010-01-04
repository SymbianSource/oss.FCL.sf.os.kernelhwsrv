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

This contains CT_FileManData
*/

//	User includes
#include "T_FileManData.h"
#include "T_SfSrvServer.h"

const TInt KMaxFileNameLength		=255;

/*@{*/
///	Parameters
_LIT(KPath,										"path");
_LIT(KFileName,									"filename");
_LIT(KFileFrom,									"filefrom");
_LIT(KFileTo,									"fileto");
_LIT(KOldName,									"oldname");
_LIT(KNewName,									"newname");
_LIT(KRFsObject,								"rfsObject");
_LIT(KRFileObject,								"rfileObject");
_LIT(KUseObserver,								"use_observer");
_LIT(KAsync,									"async");
_LIT(KTime,										"time");
_LIT(KSetMask,									"setmask");
_LIT(KClearMask,								"clearmask");
_LIT(KOperation,								"operation");
_LIT(KCurrentAction,							"current_action");
_LIT(KBytes,									"bytes");
_LIT(KError,									"error");
_LIT(KTarget,									"target");
_LIT(KSource,									"source");
_LIT(KNotifyType,								"notify_type");
_LIT(KReadHistory,								"read_history");
_LIT(KName,										"name");
_LIT(KNullDesCStr,								"KNullDesC");
_LIT(KTotalBytes,								"total_bytes");
_LIT(KFlag,										"flag");
_LIT(KClearHistory,								"clear_history");
_LIT(KSetTcontrol,								"set_tcontrol");
//For searching file names.
_LIT(KPattern, 									"*%S*");

/// Commands
_LIT(KCmdNewL,									"NewL");
_LIT(KCmdAttribs,								"Attribs");
_LIT(KCmdCopy,									"Copy");
_LIT(KCmdMove,									"Move");
_LIT(KCmdDelete,								"Delete");
_LIT(KCmdRename,								"Rename");
_LIT(KCmdRmDir,									"RmDir");
_LIT(KCmdSetObserver,							"SetObserver");
_LIT(KCmdCurrentAction,							"CurrentAction");
_LIT(KCmdGetCurrentTarget,						"GetCurrentTarget");
_LIT(KCmdGetCurrentSource,						"GetCurrentSource");
_LIT(KCmdBytesTransferredByCopyStep,			"BytesTransferredByCopyStep");
_LIT(KCmdCurrentEntry,							"CurrentEntry");
_LIT(KCmdAbbreviatedPath,						"AbbreviatedPath");
_LIT(KCmdFullPath,								"FullPath");
_LIT(KCmdGetLastError,							"GetLastError");
_LIT(KCmdGetMoreInfoAboutError,					"GetMoreInfoAboutError");
_LIT(KCmdDestructor,							"~");

// TActions
_LIT(KENone,									"ENone");
_LIT(KEAttribs,									"EAttribs");
_LIT(KECopy,									"ECopy");
_LIT(KEDelete,									"EDelete");
_LIT(KEMove,									"EMove");
_LIT(KERename,									"ERename");
_LIT(KERmDir,									"ERmDir");
_LIT(KERenameInvalidEntry,						"ERenameInvalidEntry");
_LIT(KECopyFromHandle,							"ECopyFromHandle");


// TSwitch
_LIT(KEOverWrite,								"EOverWrite");
_LIT(KERecurse,									"ERecurse");

// TControl
_LIT(KEContinue,								"EContinue");
_LIT(KEAbort,									"EAbort");
_LIT(KECancel,									"ECancel");
_LIT(KERetry,									"ERetry");

// TFileManError
_LIT(KENoExtraInformation,						"ENoExtraInformation");
_LIT(KEInitializationFailed,					"EInitializationFailed");
_LIT(KEScanNextDirectoryFailed,					"EScanNextDirectoryFailed");
_LIT(KESrcOpenFailed,							"ESrcOpenFailed");
_LIT(KETrgOpenFailed,							"ETrgOpenFailed");
_LIT(KENoFilesProcessed,						"ENoFilesProcessed");

//Notifys
_LIT(KNotifyStarted,							"started");
_LIT(KNotifyOperation,							"operation");
_LIT(KNotifyEnded,								"ended");



CT_FileManData* CT_FileManData::NewL()
/**
* Two phase constructor
*/
	{
	CT_FileManData* ret = new (ELeave) CT_FileManData();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}


CT_FileManData::CT_FileManData()
:	iFileMan(NULL)
,	iAsyncErrorIndex(0)
,	iAsyncCall(EFalse)
,	iFileManObserver(NULL)
,	iUseTControl(EFalse)

/**
* Protected constructor. First phase construction
*/
	{
	}


void CT_FileManData::ConstructL()
/**
* Protected constructor. Second phase construction
*/
	{
	}


CT_FileManData::~CT_FileManData()
/**
* Destructor.
*/
	{
	DoCleanup();
	
	iAttribs.ResetAndDestroy();
	iCopy.ResetAndDestroy();
	iDelete.ResetAndDestroy();
	iMove.ResetAndDestroy();
	iRename.ResetAndDestroy();
	iRmDir.ResetAndDestroy();
	
	ClearHistory();
	}
	
TAny* CT_FileManData::GetObject()
	{
	return iFileMan;
	}
	

TBool CT_FileManData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex)
/**
* Process a command read from the ini file
*
* @param aCommand	the command to process
* @param aSection	the section in the ini file requiring the command to be processed
*
* @return ETrue if the command is processed
*/
	{
	TBool retVal = ETrue;

	if (aCommand == KCmdNewL)
		{
		DoCmdNewL(aSection);
		}
	else if (aCommand == KCmdDestructor)
		{
		DoCleanup();
		}
	else if (aCommand == KCmdAttribs)
		{
		DoCmdAttribsL(aSection, aAsyncErrorIndex);
		}
	else if (aCommand == KCmdCopy)
		{ 
		DoCmdCopyL(aSection, aAsyncErrorIndex);
		}
	else if (aCommand == KCmdMove)
		{
		DoCmdMoveL(aSection, aAsyncErrorIndex);
		}
	else if (aCommand == KCmdDelete)
		{
		DoCmdDeleteL(aSection, aAsyncErrorIndex);
		}
	else if (aCommand == KCmdRename)
		{
		DoCmdRenameL(aSection, aAsyncErrorIndex);
		}	
	else if (aCommand == KCmdRmDir)
		{
		DoCmdRmDirL(aSection, aAsyncErrorIndex);
		}
	else if (aCommand == KCmdSetObserver)
		{
		DoCmdSetObserver();
		}
	else if (aCommand == KCmdCurrentAction)
		{
		DoCmdCurrentAction(aSection);
		}
	else if (aCommand == KCmdGetCurrentTarget)
		{
		DoCmdGetCurrentTarget(aSection);
		}
	else if (aCommand == KCmdGetCurrentSource)
		{
		DoCmdGetCurrentSource(aSection);
		}
	else if (aCommand == KCmdBytesTransferredByCopyStep)
		{
		DoCmdBytesTransferredByCopyStep(aSection);
		}
	else if(aCommand == KCmdCurrentEntry)
		{
		DoCmdCurrentEntryL(aSection);
		}
	else if(aCommand == KCmdAbbreviatedPath)
		{
		DoCmdAbbreviatedPath(aSection);
		}
	else if(aCommand == KCmdFullPath)
		{
		DoCmdFullPath(aSection);
		}
	else if(aCommand == KCmdGetLastError)
		{
		DoCmdGetLastError(aSection);
		}
	else if(aCommand == KCmdGetMoreInfoAboutError)
		{
		DoCmdGetMoreInfoAboutError(aSection);
		}
	return retVal;
	}

void CT_FileManData::DoCleanup()
	{
	INFO_PRINTF1(_L("Doing cleanup"));
	
	if (iFileMan)
		{
		delete(iFileMan);
		iFileMan = NULL;
		}
	ClearHistory();
	}
	
/**	Inform the observer that an operation is about to start **/
MFileManObserver::TControl CT_FileManData::NotifyFileManStarted()
	{		
	THistoryData *record = new THistoryData();
	CreateHistoryRecord(*record);
	iStartedHistory.Append(record);
	if(record->iLastError != KErrNone)
		{
		if(iAsyncCall)
			{
			SetAsyncError(iAsyncErrorIndex, record->iLastError);
			iAsyncCall = EFalse;
			}
		else
			{
			SetError(record->iLastError);
			}
		}
	TControl ret = EContinue;	
	//Check if we need to return TControl value specified in INI.
	if(iUseTControl && iObserverNotifyType == ENotifyStarted)
		{
		TBuf<KMaxFileNameLength> buffer;
		buffer.Format(KPattern(), &iNotifyFileName);
		//Check if we need to return TControl for this file.
		if(record->iCurrentSource.Match(buffer) != KErrNotFound)
			{
			iUseTControl = EFalse;
			ret = iTControl;
			}
		}
	return ret;
	}
	
/**	Informs the observer that an operation, i.e. a copy or a move, is proceeding.	**/	
MFileManObserver::TControl CT_FileManData::NotifyFileManOperation()
	{	
	THistoryData *record = new THistoryData();
	CreateHistoryRecord(*record);
	iOperationHistory.Append(record);
	if(record->iLastError != KErrNone)
		{
		if(iAsyncCall)
			{
			SetAsyncError(iAsyncErrorIndex, record->iLastError);
			iAsyncCall = EFalse;
			}
		else
			{
			SetError(record->iLastError);
			}
		}
	TControl ret = EContinue;
	//Check if we need to return TControl value specified in INI.
	if(iUseTControl && iObserverNotifyType == ENotifyOperation)
		{
		TBuf<KMaxFileNameLength> buffer;
		buffer.Format(KPattern(), &iNotifyFileName);
		//Check if we need to return TControl for this file.
		if(record->iCurrentSource.Match(buffer) != KErrNotFound)
			{
			iUseTControl = EFalse;
			ret = iTControl;
			}
		}
	return ret;
	}
	
/**	Informs the observer that an operation is complete.	**/	
MFileManObserver::TControl CT_FileManData::NotifyFileManEnded()
	{
	THistoryData *record = new THistoryData();
	CreateHistoryRecord(*record);
	iEndedHistory.Append(record);
	if(record->iLastError != KErrNone)
		{
		if(iAsyncCall)
			{
			SetAsyncError(iAsyncErrorIndex, record->iLastError);
			iAsyncCall = EFalse;
			}
		else
			{
			SetError(record->iLastError);
			}
		}
	TControl ret = EContinue;
	//Check if we need to return TControl value specified in INI.
	if(iUseTControl && iObserverNotifyType == ENotifyEnded)
		{
		TBuf<KMaxFileNameLength> buffer;
		buffer.Format(KPattern(), &iNotifyFileName);
		//Check if we need to return TControl for this file.
		if(record->iCurrentSource.Match(buffer) != KErrNotFound)
			{
			iUseTControl = EFalse;
			ret = iTControl;
			}
		}
	return ret; 
	}

void CT_FileManData::DoCmdNewL(const TDesC& aSection)
	{	
	DoCleanup();
	TBool useObserver = EFalse;
	
	GET_OPTIONAL_BOOL_PARAMETER(KUseObserver, aSection, useObserver);
	
	TPtrC	rfsObjectName;
	RFs*	rfsObject = NULL;
	if (GET_MANDATORY_STRING_PARAMETER(KRFsObject, aSection, rfsObjectName))
		{
		rfsObject = (RFs*)GetDataObjectL(rfsObjectName);
		}
	
	TInt err = KErrNone;
	if(useObserver)
		{
		INFO_PRINTF1(_L("CFileMan::NewL(RFs, MFileManObserver)"));
		TRAP(err, iFileMan = CFileMan::NewL(*rfsObject, iFileManObserver));
		}
	else
		{
		INFO_PRINTF1(_L("CFileMan::NewL(RFs)"));
		TRAP(err, iFileMan = CFileMan::NewL(*rfsObject));
		}
	if (err != KErrNone)
		{
		ERR_PRINTF1(_L("CFileMan object is not created"));
		SetBlockResult(EFail);
		}
	}
	
void CT_FileManData::DoCmdAttribsL(const TDesC& aSection, TInt aAsyncErrorIndex)
	{
	INFO_PRINTF1(_L("Setting attributes!"));
	
	TBool dataOk = ETrue;
	
	//Reading path for file or files.	
	TPtrC	fileName;
	if (!GET_MANDATORY_STRING_PARAMETER(KFileName, aSection, fileName))
		{
		dataOk = EFalse;
		}
	
	//Reading new TTime from ini
	TPtrC inputTime;
	TTime iniTime;
	if(GET_MANDATORY_STRING_PARAMETER(KTime, aSection, inputTime))
		{	
		TInt err = iniTime.Set(inputTime);
		if (err != KErrNone)
			{
			ERR_PRINTF3(_L("Cannot convert (%S) to time. Error: %d"), &inputTime ,err);
			dataOk = EFalse;
			SetBlockResult(EFail);
			}
		}
	else
		{
		dataOk = EFalse;
		}
		
	//Reading Async parameter (True/False)	
	TBool async = EFalse;
	if(!GET_MANDATORY_BOOL_PARAMETER(KAsync, aSection, async))
		{
		dataOk = EFalse;
		}
	//Read SetMask attribute
	TUint setMask;
	if(!FileserverUtil::GetAttMask(*this, aSection, KSetMask(), setMask))
		{
		dataOk = EFalse;
		}
		
	//Read ClearMask attribute
	TUint clearMask;
	if(!FileserverUtil::GetAttMask(*this, aSection, KClearMask(), clearMask))
		{
		dataOk = EFalse;
		}
		
	//Read operation TSwitch
	TUint operation;
	if(!GetOperationFromConfig(KOperation(), aSection, operation))
		{
		dataOk = EFalse;
		}
	if(dataOk)
		{
		TInt error = KErrNone;
		if(async)
			{
			CActiveCallback* active = CActiveCallback::NewLC(*this);
			iAttribs.AppendL(active);
			CleanupStack::Pop(active);
			error = iFileMan->Attribs(fileName, setMask, clearMask, iniTime, operation, active->iStatus);
			if(error == KErrNone)
				{
				active->Activate(aAsyncErrorIndex);
				iAsyncErrorIndex = aAsyncErrorIndex;
				iAsyncCall = ETrue;
				IncOutstanding();
				}
			}
		else
			{
			error = iFileMan->Attribs(fileName, setMask, clearMask, iniTime, operation);
			}
		if (error != KErrNone)
			{
			ERR_PRINTF3(_L("Attribs(%S) Error: %d"), &fileName, error);
			SetError(error);
			}
		else
			{
			INFO_PRINTF2(_L("Attribs operation on (%S) is successfull!"), &fileName);
			}
		}
	}
void CT_FileManData::DoCmdCopyL(const TDesC& aSection, TInt aAsyncErrorIndex)
	{
	TBool clearHistory = EFalse;
	GET_OPTIONAL_BOOL_PARAMETER(KClearHistory, aSection, clearHistory);
	if(clearHistory)
		{
		ClearHistory();
		}
	TBool setTControl = EFalse;
	GET_OPTIONAL_BOOL_PARAMETER(KSetTcontrol, aSection, setTControl);
	if(setTControl)
		{
		ReadTControl(aSection);
		}
	
	INFO_PRINTF1(_L("Coping files!"));	
	TBool dataOk = ETrue;
	TPtrC oldName;
	TPtrC rfileObjectName;
	RFile*	rfileObject = NULL;
	
	if (!GET_OPTIONAL_STRING_PARAMETER(KOldName, aSection, oldName))
		{
		if (GET_OPTIONAL_STRING_PARAMETER(KRFileObject, aSection, rfileObjectName))
			{
			rfileObject = (RFile*)GetDataObjectL(rfileObjectName);
			}
		else
			{
			ERR_PRINTF1(_L("Cannot read \"oldname\" or \"rfileObject\" arguments!"));
			SetBlockResult(EFail);
			dataOk = EFalse;
			}
		}
		
	TPtrC newName;
	if (!GET_MANDATORY_STRING_PARAMETER(KNewName, aSection, newName))
		{
		dataOk = EFalse;
		}
		
	TUint operation;
	if (!GetOperationFromConfig(KOperation, aSection, operation))
		{
		dataOk = EFalse;
		}
	TBool async;
	if (!GET_MANDATORY_BOOL_PARAMETER(KAsync, aSection, async))
		{
		dataOk = EFalse;
		}
		
	if(dataOk)
		{
		TInt error = KErrNone;
		if(async)
			{
			CActiveCallback* active = CActiveCallback::NewLC(*this);
			iCopy.AppendL(active);
			CleanupStack::Pop(active);
			if(rfileObject)
				{
				error = iFileMan->Copy(*rfileObject, newName, operation, active->iStatus);
				}
			else
				{
				error = iFileMan->Copy(oldName, newName, operation, active->iStatus);	
				}
			if(error == KErrNone)
				{
				active->Activate(aAsyncErrorIndex);
				iAsyncErrorIndex = aAsyncErrorIndex;
				iAsyncCall = ETrue;
	            IncOutstanding();
				}
			}
		else
			{
   			if(rfileObject)
   				{
   				error = iFileMan->Copy(*rfileObject, newName, operation);	
   				}
   			else
   				{
   				error = iFileMan->Copy(oldName, newName, operation);
   				}
			}
		if (error != KErrNone)
			{
			ERR_PRINTF2(_L("Copy(), Error: %d"), error);
			SetError(error);
			}
		else
			{
			INFO_PRINTF1(_L("Copy() operation is successfully!"));
			}	
		}
	}
		
void CT_FileManData::DoCmdDeleteL(const TDesC& aSection, TInt aAsyncErrorIndex)
	{
	TBool clearHistory = EFalse;
	GET_OPTIONAL_BOOL_PARAMETER(KClearHistory, aSection, clearHistory);
	if(clearHistory)
		{
		ClearHistory();
		}
	TBool setTControl = EFalse;
	GET_OPTIONAL_BOOL_PARAMETER(KSetTcontrol, aSection, setTControl);
	if(setTControl)
		{
		ReadTControl(aSection);
		}
		
	INFO_PRINTF1(_L("Deleting files!"));
	
	TBool dataOk = ETrue;
	TPtrC	fileName;
	if (GET_MANDATORY_STRING_PARAMETER(KFileName, aSection, fileName))
		{
		if(fileName == KNullDesCStr)
			{
			fileName.Set(KNullDesC);	
			}
		}
	else
		{
		dataOk = EFalse;
		}
	TUint operation;
	if(!GetOperationFromConfig(KOperation, aSection, operation))
		{
		dataOk = EFalse;
		}
	TBool async = EFalse;
	if (!GET_MANDATORY_BOOL_PARAMETER(KAsync, aSection, async))
		{
		dataOk = EFalse;
		}
	if(dataOk)
		{
		TInt error = KErrNone;
		if(async)
			{
			CActiveCallback* active = CActiveCallback::NewLC(*this);
			iDelete.AppendL(active);
		    CleanupStack::Pop(active);
		    error = iFileMan->Delete(fileName, operation, active->iStatus);
			if (error == KErrNone)
				{
				active->Activate(aAsyncErrorIndex);
				iAsyncErrorIndex = aAsyncErrorIndex;
				iAsyncCall = ETrue;
	            IncOutstanding();
				}
			}
		else
			{
			error = iFileMan->Delete(fileName, operation);
			}
			
		if (error != KErrNone)
			{
			ERR_PRINTF3(_L("Delete (%S), Error: %d"), &fileName, error);
			SetError(error);
			}
		else
			{
			INFO_PRINTF2(_L("Delete (%S) operation is successfully!"), &fileName);
			}
		}
	}
	
void CT_FileManData::DoCmdMoveL(const TDesC& aSection, TInt aAsyncErrorIndex)
	{
	TBool clearHistory = EFalse;
	GET_OPTIONAL_BOOL_PARAMETER(KClearHistory, aSection, clearHistory);
	if(clearHistory)
		{
		ClearHistory();
		}
	TBool setTControl = EFalse;
	GET_OPTIONAL_BOOL_PARAMETER(KSetTcontrol, aSection, setTControl);
	if(setTControl)
		{
		ReadTControl(aSection);
		}
		
	INFO_PRINTF1(_L("Moving files!"));
	
	TBool	dataOk = ETrue;
	TPtrC	fileFrom;
	if (!GET_MANDATORY_STRING_PARAMETER(KFileFrom, aSection, fileFrom))
		{
		dataOk = EFalse;
		}
		
	TPtrC	fileTo;
	if (!GET_MANDATORY_STRING_PARAMETER(KFileTo, aSection, fileTo))
		{
		dataOk = EFalse;	
		}
	TBool	async;
	if (!GET_MANDATORY_BOOL_PARAMETER(KAsync, aSection, async))
		{
		dataOk = EFalse;	
		}
	TUint operation;
	if (!GetOperationFromConfig(KOperation, aSection, operation))
		{
		dataOk = EFalse;	
		}
	if(dataOk)
		{
		TInt error = KErrNone;
		if(async)
			{
			CActiveCallback*	active = CActiveCallback::NewLC(*this);
			iMove.AppendL(active);
	        CleanupStack::Pop(active);	
	        error = iFileMan->Move(fileFrom, fileTo, operation, active->iStatus);
	        if(error == KErrNone)
	        	{
	        	active->Activate(aAsyncErrorIndex);
				iAsyncErrorIndex = aAsyncErrorIndex;
				iAsyncCall = ETrue;
	            IncOutstanding();
	        	}
			}
		else
			{
			error = iFileMan->Move(fileFrom, fileTo, operation);
			}
		if (error != KErrNone)
			{
			ERR_PRINTF2(_L("Move(), Error: %d"), error);
			SetError(error);
			}
		else
			{
			INFO_PRINTF1(_L("Move() operation is successfully!"));
			}
		}	
	}
		
void CT_FileManData::DoCmdRenameL(const TDesC& aSection, TInt aAsyncErrorIndex)
	{
	TBool clearHistory = EFalse;
	GET_OPTIONAL_BOOL_PARAMETER(KClearHistory, aSection, clearHistory);
	if(clearHistory)
		{
		ClearHistory();
		}
	TBool setTControl = EFalse;
	GET_OPTIONAL_BOOL_PARAMETER(KSetTcontrol, aSection, setTControl);
	if(setTControl)
		{
		ReadTControl(aSection);
		}
	INFO_PRINTF1(_L("Renaming files!"));
		
	TBool dataOk = ETrue;
	
	//Reading Old Name
	TPtrC	oldName;
	if (!GET_MANDATORY_STRING_PARAMETER(KOldName, aSection, oldName))
		{
		dataOk = EFalse;
		}
		
	//Reading New Name
	TPtrC	newName;
	if (!GET_MANDATORY_STRING_PARAMETER(KNewName, aSection, newName))
		{
		dataOk = EFalse;
		}
		
	//Rading TSwitch - Operation
	TUint operation;
	if (!GetOperationFromConfig(KOperation(), aSection, operation))
		{
		dataOk = EFalse;
		}
	//Reading async
	TBool async;
	if (!GET_MANDATORY_BOOL_PARAMETER(KAsync, aSection, async))
		{
		dataOk = EFalse;
		}
	if(dataOk)
		{
		TInt error = KErrNone;
		if(async)
			{
			CActiveCallback* active = CActiveCallback::NewLC(*this);
			iRename.AppendL(active);
			CleanupStack::Pop(active);
			error = iFileMan->Rename(oldName, newName, operation, active->iStatus);
			if(error == KErrNone)	
				{
				active->Activate(aAsyncErrorIndex);
				iAsyncErrorIndex = aAsyncErrorIndex;
				iAsyncCall = ETrue;
				IncOutstanding();
				}			
			}
		else
			{
			error = iFileMan->Rename(oldName, newName, operation);
			}
		if (error != KErrNone)
			{
			ERR_PRINTF2(_L("Rename() Error: %d"), error);
			SetError(error);
			}
		else
			{
			INFO_PRINTF1(_L("Rename() operation is successfully!"));
			}
		}	
	}
		
void CT_FileManData::DoCmdRmDirL(const TDesC& aSection, TInt aAsyncErrorIndex)
	{
	INFO_PRINTF1(_L("Removing directory!"));
	
	TBool	dataOk = ETrue;
	TPtrC	path;
	
	if (!GET_MANDATORY_STRING_PARAMETER(KPath, aSection, path))
		{
		dataOk = EFalse;
		}
		
	TBool async = EFalse;
	if (!GET_MANDATORY_BOOL_PARAMETER(KAsync, aSection, async))
		{
		dataOk = EFalse;
		}
		
	if(dataOk)
		{
		TInt error = KErrNone;
		if(async)
			{
			CActiveCallback* active = CActiveCallback::NewLC(*this);
			iRmDir.AppendL(active);
			CleanupStack::Pop(active);
			error = iFileMan->RmDir(path, active->iStatus);
			if(error == KErrNone)
				{
				active->Activate(aAsyncErrorIndex);
				iAsyncErrorIndex = aAsyncErrorIndex;
				iAsyncCall = ETrue;
				IncOutstanding();
				}
			}
		else
			{
			error = iFileMan->RmDir(path);
			}
		if (error != KErrNone)
			{
			ERR_PRINTF3(_L("RmDir(%S), Error: %d"), &path, error);
			SetError(error);
			}
		else
			{
			INFO_PRINTF2(_L("Callled RmDir(%S)"), &path);
			}
		}
	}
	
void CT_FileManData::DoCmdSetObserver()
	{
	INFO_PRINTF1(_L("Setting observer"));
	
	iFileMan->SetObserver(this);
	}
	
void CT_FileManData::DoCmdCurrentAction(const TDesC& aSection)
	{
	TBool readHistory = EFalse;
	GET_OPTIONAL_BOOL_PARAMETER(KReadHistory, aSection, readHistory);
	CFileMan::TAction currentAction = CFileMan::ENone;
	TBool found = ETrue;
	if(readHistory)
		{
		/** Get needed history array **/
		RPointerArray<THistoryData>* history = GetHistoryDataByType(aSection);
		if(history)
			{
			CFileMan::TAction expectedAction;
			found = EFalse;
			if(GetActionFromConfig(KCurrentAction(), aSection, expectedAction))
				{
				CFileMan::TAction action;
				TInt count = history->Count();
				/** Search for expected TAction in history **/
				for(TInt i = 0; (i < count) && (!found); i++)
					{
					action = ((*history)[i])->iCurrentAction;
					if( action == expectedAction)
						{
						currentAction = action;
						found = ETrue;
						}
					}
				if(!found)
					{
					TPtrC expectedActionStr;
					ConvertActionToString(expectedAction, expectedActionStr);
					ERR_PRINTF2(_L("CurrentAction() %S not found in history!"), &expectedActionStr);
					SetBlockResult(EFail);
					}
				}
			else
				{
				ERR_PRINTF2(_L("Cannot read %S"), &KCurrentAction());
				SetBlockResult(EFail);
				}
			}
		}
	else
		{
		currentAction = iFileMan->CurrentAction();	
		}
		
	if(found)
		{
		TPtrC actionStr;
		ConvertActionToString(currentAction, actionStr);
		
		CFileMan::TAction expectedAction;
		if(GetActionFromConfig(KCurrentAction(), aSection, expectedAction))
			{
			TPtrC expectedActionStr;
			ConvertActionToString(expectedAction, expectedActionStr);
			if(expectedAction != currentAction)
				{
				ERR_PRINTF3(_L("CurrentAction(): %S != %S Expected Action"), &actionStr, &expectedActionStr);			
				SetBlockResult(EFail);
				}
			else
				{
				INFO_PRINTF3(_L("CurrentAction(): %S == %S Expected Action"), &actionStr, &expectedActionStr);			
				}
			}
		else
			{
			INFO_PRINTF2(_L("CurrentAction: %S"), &actionStr);
			}
		}
	}
	
void CT_FileManData::DoCmdGetCurrentTarget(const TDesC& aSection)
	{
	TBool readHistory = EFalse;
	GET_OPTIONAL_BOOL_PARAMETER(KReadHistory, aSection, readHistory);
	TFileName target;
	TBool found = ETrue;
	if(readHistory)
		{
		/** Get needed history array **/
		RPointerArray<THistoryData>* history = GetHistoryDataByType(aSection);
		if(history)
			{
			TPtrC expectedTarget;
			found = EFalse;
			if(GET_MANDATORY_STRING_PARAMETER(KTarget, aSection, expectedTarget))
				{
				TInt count = history->Count();
				/** Search for expected target in history **/
				for(TInt i = 0; (!found) && (i < count); i++)
					{		
					if( ((*history)[i])->iCurrentTarget == expectedTarget)
						{
						target = ((*history)[i])->iCurrentTarget;
						found = ETrue;
						}
					}
				if(!found)
					{
					ERR_PRINTF2(_L("GetCurrentTarget() %S not found in history!"), &expectedTarget);
					SetBlockResult(EFail);
					}
				}
			}
		}
	else
		{
		iFileMan->GetCurrentTarget(target);
		}
	if(found)
		{
		TPtrC expectedTarget;
		if(GET_OPTIONAL_STRING_PARAMETER(KTarget, aSection, expectedTarget))
			{
			if(target == expectedTarget)
				{
				INFO_PRINTF3(_L("GetCurrentTarget() %S == %S ExpectedResult"), &target, &expectedTarget);
				}
			else
				{
				ERR_PRINTF3(_L("GetCurrentTarget() %S != %S ExpectedResult"), &target, &expectedTarget);
				SetBlockResult(EFail);
				}
			
			}
		else
			{
			INFO_PRINTF2(_L("GetCurrentTarget() %S"), &target);	
			}
		}
	}
void CT_FileManData::DoCmdGetCurrentSource(const TDesC& aSection)
	{
	TBool readHistory = EFalse;	
	GET_OPTIONAL_BOOL_PARAMETER(KReadHistory, aSection, readHistory);
	TFileName source;
	TBool found = ETrue;
	if(readHistory)
		{
		/** Get needed history array **/
		RPointerArray<THistoryData>* history = GetHistoryDataByType(aSection);
		if(history)
			{
			TPtrC expectedSource;
			found = EFalse;
			if(GET_MANDATORY_STRING_PARAMETER(KSource, aSection, expectedSource))
				{
				TInt count = history->Count();
				/** Search for expected source in history **/
				for(TInt i = 0; (!found) && (i < count); i++)
					{				
					if( (*history)[i]->iCurrentSource == expectedSource)
						{
						source = (*history)[i]->iCurrentSource;
						found = ETrue;
						}
					}
				if(!found)
					{
					ERR_PRINTF2(_L("GetCurrentSource() %S not found in history!"), &expectedSource);
					SetBlockResult(EFail);
					}
				}
			}
		}
	else
		{
		iFileMan->GetCurrentSource(source);
		}
		
	if (found)
		{
		TPtrC expectedSource;
		if(GET_OPTIONAL_STRING_PARAMETER(KSource, aSection, expectedSource))
			{
			if(source == expectedSource)
				{
				INFO_PRINTF3(_L("GetCurrentSource() %S == %S ExpectedResult"), &source, &expectedSource);
				}
			else
				{
				ERR_PRINTF3(_L("GetCurrentSource() %S != %S ExpectedResult"), &source, &expectedSource);
				SetBlockResult(EFail);
				}
			
			}
		else
			{
			INFO_PRINTF2(_L("GetCurrentSource() %S"), &source);	
			}
		}
		
	}
void CT_FileManData::DoCmdBytesTransferredByCopyStep(const TDesC& aSection)
	{
	TBool readHistory = EFalse;
	GET_OPTIONAL_BOOL_PARAMETER(KReadHistory, aSection, readHistory);
	TInt bytes = 0;
	TBool found = ETrue;
	if(readHistory)
		{
		/** Get needed history array **/
		RPointerArray<THistoryData>* history = GetHistoryDataByType(aSection);
		if(history)
			{
			TInt expectedBytes;
			found = EFalse;
			if(GET_MANDATORY_INT_PARAMETER(KBytes, aSection, expectedBytes))
				{
				TBool totalBytes = EFalse;
				GET_OPTIONAL_BOOL_PARAMETER(KTotalBytes, aSection, totalBytes);
				TInt count = history->Count();
				TInt bytesTemp;
				/** Search for expected transferred bytes in history **/
				for(TInt i = 0; (!found) && (i < count); i++)
					{
					bytesTemp = ((*history)[i])->iBytesTransferred;
					if(totalBytes)
						{
						bytes +=bytesTemp;
						}
					else
						{
						if( bytesTemp == expectedBytes)
							{
							bytes = bytesTemp;
							found = ETrue;
							}
						}
					}
				if(totalBytes)
					{
					found = ETrue;
					}
				if(!found)
					{
					ERR_PRINTF2(_L("BytesTransferredByCopyStep() %d not found in history!"), expectedBytes);
					SetBlockResult(EFail);
					}
				}
			}
		}
	else
		{
		bytes = iFileMan->BytesTransferredByCopyStep();
		}
	if (found)
		{
		TInt expectedBytes;	
		if(GET_OPTIONAL_INT_PARAMETER(KBytes, aSection, expectedBytes))
			{		
			if(expectedBytes != bytes)
				{
				ERR_PRINTF3(_L("BytesTransferredByCopyStep(): %d != %d Expected Bytes"), bytes, expectedBytes);
				SetBlockResult(EFail);
				}
			else
				{
				INFO_PRINTF3(_L("BytesTransferredByCopyStep(): %d == %d Expected Bytes"), bytes, expectedBytes);
				}
			}
		else
			{
			INFO_PRINTF2(_L("BytesTransferredByCopyStep(): %d"), bytes);
			}
		}
	}
void CT_FileManData::DoCmdCurrentEntryL(const TDesC& aSection)
	{
	
	TEntry* entryObject = NULL;
	TBool found = ETrue;
	TBool readHistory = EFalse;	
	GET_OPTIONAL_BOOL_PARAMETER(KReadHistory, aSection, readHistory);
	if(readHistory)
		{
		/** Get needed history array **/
		RPointerArray<THistoryData>* history = GetHistoryDataByType(aSection);
		if(history)
			{
			TPtrC expectedName;
			found = EFalse;
			if(GET_MANDATORY_STRING_PARAMETER(KName, aSection, expectedName))
				{
				TInt count = history->Count();
				TEntry entry;
				/** Search for expected TEntry.iName in history **/
				for(TInt i = 0; (!found) && (i < count); i++)
					{
					entry = ((*history)[i])->iCurrentEntry;
					if( entry.iName == expectedName)
						{
						entryObject = &entry;
						found = ETrue;
						}
					}
				if(!found)
					{
					ERR_PRINTF1(_L("CurrentEntry() not found in history!"));
					SetBlockResult(EFail);
					}
				}	
			}
		}
	else
		{
		*entryObject = iFileMan->CurrentEntry();
		}
	if(found)
		{
		if(!FileserverUtil::VerifyTEntryDataFromIniL(*this, aSection, *entryObject))
			{
			SetBlockResult(EFail);	
			}
		}
		
	}
void CT_FileManData::DoCmdAbbreviatedPath(const TDesC& aSection)
	{
	TBool readHistory = EFalse;	
	GET_OPTIONAL_BOOL_PARAMETER(KReadHistory, aSection, readHistory);
	TPtrC path;
	TBool found = ETrue;
	if(readHistory)
		{
		/** Get needed history array **/
		RPointerArray<THistoryData>* history = GetHistoryDataByType(aSection);
		if(history)
			{
			TPtrC expectedPath;
			found = EFalse;
			if(GET_MANDATORY_STRING_PARAMETER(KPath, aSection, expectedPath))
				{
				TInt count = history->Count();
				/** Search for expected abbreviated path in history **/
				for(TInt i = 0; (!found) && (i < count); i++)
					{	
					if( ((*history)[i])->iAbbreviatedPath == expectedPath)
						{
						path.Set(((*history)[i])->iAbbreviatedPath);
						found = ETrue;
						}
					}
				if(!found)
					{
					ERR_PRINTF2(_L("AbbreviatedPath() %S not found in history!"), &expectedPath);
					SetBlockResult(EFail);
					}
				}
			}
		}
	else
		{
		path.Set(iFileMan->AbbreviatedPath());
		}
		
	if (found)
		{
		TPtrC expectedPath;
		if(GET_OPTIONAL_STRING_PARAMETER(KPath, aSection, expectedPath))
			{		
			if(expectedPath.Compare(path) != 0)
				{
				ERR_PRINTF3(_L("AbbreviatedPath(): %S != %S Expected Path"), &path, &expectedPath);
				SetBlockResult(EFail);
				}
			else
				{
				INFO_PRINTF3(_L("AbbreviatedPath(): %S == %S Expected Path"), &path, &expectedPath);
				}
			}
		else
			{
			INFO_PRINTF2(_L("AbbreviatedPath(): %S"), &path);
			}
		}
	}
void CT_FileManData::DoCmdFullPath(const TDesC& aSection)
	{
	TBool readHistory = EFalse;	
	GET_OPTIONAL_BOOL_PARAMETER(KReadHistory, aSection, readHistory);
	TPtrC fullPath;
	TBool found = ETrue;
	if(readHistory)
		{
		/** Get needed history array **/
		RPointerArray<THistoryData>* history = GetHistoryDataByType(aSection);
		if(history)
			{
			TPtrC expectedPath;
			found = EFalse;
			if(GET_MANDATORY_STRING_PARAMETER(KPath, aSection, expectedPath))
				{
				TInt count = history->Count();
				/** Search for expected full path in history **/
				for(TInt i = 0; (!found) && (i < count); i++)
					{	
					if( ((*history)[i])->iFullPath == expectedPath)
						{
						fullPath.Set(((*history)[i])->iFullPath);
						found = ETrue;
						}
					}
				if(!found)
					{
					ERR_PRINTF2(_L("FullPath() %S not found in history!"), &expectedPath);
					SetBlockResult(EFail);
					}	
				}
			}
		}
	else
		{
		fullPath.Set(iFileMan->FullPath());
		}
	if(found)
		{
		TPtrC expectedPath;
		if(GET_OPTIONAL_STRING_PARAMETER(KPath, aSection, expectedPath))
			{		
			if(expectedPath.Compare(fullPath) != 0)
				{
				ERR_PRINTF3(_L("FullPath(): %S != %S Expected Path"), &fullPath, &expectedPath);
				SetBlockResult(EFail);
				}
			else
				{
				INFO_PRINTF3(_L("FullPath(): %S == %S Expected Path"), &fullPath, &expectedPath);
				}
			}
		else
			{
			INFO_PRINTF2(_L("FullPath(): %S"), &fullPath);
			}
		}
	}
void CT_FileManData::DoCmdGetLastError(const TDesC& aSection)
	{
	TBool readHistory = EFalse;	
	GET_OPTIONAL_BOOL_PARAMETER(KReadHistory, aSection, readHistory);
	TInt lastError = KErrNone;
	TBool found = ETrue;
	if(readHistory)
		{
		/** Get needed history array **/
		RPointerArray<THistoryData>* history = GetHistoryDataByType(aSection);
		if(history)
			{
			TInt expectedError;
			found = EFalse;
			if(GET_MANDATORY_INT_PARAMETER(KError, aSection, expectedError))
				{
				TInt count = history->Count();
				TInt err;
				/** Search for expected error in history **/
				for(TInt i = 0; (!found) && (i < count); i++)
					{	
					err = (*history)[i]->iLastError;			
					if( err == expectedError)
						{
						lastError = err;
						found = ETrue;
						}
					}
				if(!found)
					{
					ERR_PRINTF2(_L("GetLastError() %d not found in history!"), expectedError);
					SetBlockResult(EFail);
					}
				}
			}
		}
	else
		{
		lastError = iFileMan->GetLastError();
		}
	if (found)
		{
		TInt expectedError;	
		if(GET_OPTIONAL_INT_PARAMETER(KError, aSection, expectedError))
			{		
			if(expectedError != lastError)
				{
				ERR_PRINTF3(_L("GetLastError(): %d != %d Expected Error"), lastError, expectedError);
				SetBlockResult(EFail);
				}
			else
				{
				INFO_PRINTF3(_L("GetLastError(): %d == %d Expected Error"), lastError, expectedError);
				}
			}
		else
			{
			INFO_PRINTF2(_L("GetLastError(): %d"), lastError);
			}
		}
	}
		
void CT_FileManData::DoCmdGetMoreInfoAboutError(const TDesC& aSection)
	{
	TBool readHistory = EFalse;	
	GET_OPTIONAL_BOOL_PARAMETER(KReadHistory, aSection, readHistory);
	TFileManError lastError;
	TBool found = ETrue;
	if(readHistory)
		{
		/** Get needed history array **/
		RPointerArray<THistoryData>* history = GetHistoryDataByType(aSection);
		if(history)
			{
			TFileManError expectedError;
			found = EFalse;
			if(GetFileManErrorFromConfig(KError, aSection, expectedError))
				{
				TInt count = history->Count();
				TFileManError error;
				/** Search for expected error in history **/
				for(TInt i = 0; (!found) && (i < count); i++)
					{
					error = (*history)[i]->iMoreInfoAboutError;
					if( (error == expectedError))
						{
						lastError = error;
						found = ETrue;
						}
					}
				if(!found)
					{
					ERR_PRINTF2(_L("GetMoreInfoAboutError() %S not found in history!"), expectedError);
					SetBlockResult(EFail);
					}
				}
			}
		}
	else
		{
		lastError = iFileMan->GetMoreInfoAboutError();
		}
	if (found)
		{
		TPtrC errorStr;
		ConvertFileManErrorToString(lastError, errorStr);
		TFileManError expectedError;
		if(GetFileManErrorFromConfig(KError, aSection, expectedError))
			{
			TPtrC expectedErrorStr;
			ConvertFileManErrorToString(expectedError, expectedErrorStr);
			
			if(expectedError != lastError)
				{
				ERR_PRINTF3(_L("GetMoreInfoAboutError(): %S != %S Expected Error"), &errorStr, &expectedErrorStr);
				SetBlockResult(EFail);
				}
			else
				{
				INFO_PRINTF3(_L("GetMoreInfoAboutError(): %S == %S Expected Error"), &errorStr, &expectedErrorStr);
				}
			}
		else
			{
			INFO_PRINTF2(_L("GetMoreInfoAboutError(): %S"), &errorStr);
			}
		}
		
	}
void CT_FileManData::RunL(CActive* aActive, TInt aIndex)
/**
 Virtual RunL - Called on completion of an asynchronous command
 @internalComponent
 @see MActiveCallback
 @param aActive Active Object that RunL has been called on
 @pre N/A
 @post N/A
 @leave system wide error code
*/
	{
	INFO_PRINTF1(_L("CT_FileManData::RunL Called"));
	TBool	foundActiveObject = EFalse;
	TInt	index=0;
	TInt	count=0;
	TBool	completed=ETrue;
	
	count = iAttribs.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iAttribs[index] )
			{   
			INFO_PRINTF1(_L("RunL iAttribs called"));
			foundActiveObject = ETrue;
			iAttribs.Remove(index);
	 		}
		}

	count = iCopy.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iCopy[index] )
			{						    
			INFO_PRINTF1(_L("RunL iCopy called"));
			foundActiveObject = ETrue;
			iCopy.Remove(index);
	 		}
		}	
	
	count = iDelete.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iDelete[index] )
			{						    
			INFO_PRINTF1(_L("RunL iDelete called"));
			foundActiveObject = ETrue;
			iDelete.Remove(index);
	 		}
		}
		
	count = iMove.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iMove[index] )
			{						    
			INFO_PRINTF1(_L("RunL iMove called"));
			foundActiveObject = ETrue;
			iMove.Remove(index);
	 		}
		}
		
	count = iRename.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iRename[index] )
			{						    
			INFO_PRINTF1(_L("RunL iRename called"));
			foundActiveObject = ETrue;
			iRename.Remove(index);
	 		}
		}	
	
	count = iRmDir.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iRmDir[index] )
			{						    
			INFO_PRINTF1(_L("RunL iRmDir called"));
			foundActiveObject = ETrue;
			iRmDir.Remove(index);
	 		}
		}
	
	if( foundActiveObject )
 		{
		TInt	err = aActive->iStatus.Int();
		if( err != KErrNone )
			{
			ERR_PRINTF2(_L("RunL Error %d"), err);
			SetAsyncError( aIndex, err );
			iAsyncCall = EFalse;
			}

		if ( completed )
			{
			// Reset the outstanding request state
			DecOutstanding();
			delete aActive;
			}
		}
	else
		{
 		ERR_PRINTF1(_L("Stray RunL signal"));
 		SetBlockResult(EFail);
		}
	}
	
	
void CT_FileManData::DoCancel(CActive* aActive, TInt aIndex)
/**
 Virtual DoCancel - Request to cancel the asynchronous command
 @internalComponent
 @see - MActiveCallback
 @param aActive Active Object that DoCancel has been called on
 @pre - N/A
 @post - N/A
 @leave system wide error code
*/
	{
	INFO_PRINTF1(_L("CT_FileManData::DoCancelL Called"));
	
	TBool	foundActiveObject = EFalse;
	TInt	index=0;
	TInt	count=0;
	
	
	// See if it is in iAttribs
	count = iAttribs.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iAttribs[index] )
			{  
			INFO_PRINTF1(_L("DoCancel iAttribs called"));
			foundActiveObject = ETrue;
			iAttribs.Remove(index);
	 		}
		}
	
	// See if it is in iCopy2
	count = iCopy.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iCopy[index] )
			{  
			INFO_PRINTF1(_L("DoCancel iCopy called"));
			foundActiveObject = ETrue;
			iCopy.Remove(index);
	 		}
		}
				
	// See if it is in iDelete
	count = iDelete.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iDelete[index] )
			{  
			INFO_PRINTF1(_L("DoCancel iDelete called"));
			foundActiveObject = ETrue;
			iDelete.Remove(index);
	 		}
		}
		
	// See if it is in iMove
	count = iMove.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iMove[index] )
			{  
			INFO_PRINTF1(_L("DoCancel iMove called"));
			foundActiveObject = ETrue;
			iMove.Remove(index);
	 		}
		}
		
	// See if it is in iRename
	count = iRename.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iRename[index] )
			{  
			INFO_PRINTF1(_L("DoCancel iRename called"));
			foundActiveObject = ETrue;
			iRename.Remove(index);
	 		}
		}
		
	// See if it is in iRmDir
	count = iRmDir.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iRmDir[index] )
			{  
			INFO_PRINTF1(_L("DoCancel iRmDir called"));
			foundActiveObject = ETrue;
			iRmDir.Remove(index);
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
		
/** Reads TFileManError from INI and converts it to TFileManError type. **/	
TBool CT_FileManData::GetFileManErrorFromConfig(const TDesC& aParameterName, const TDesC& aSection, TFileManError& aError)
	{
	TPtrC errorStr;
	TBool ret = GET_OPTIONAL_STRING_PARAMETER(aParameterName, aSection, errorStr);
	if(ret)
		{
		if(errorStr == KENoExtraInformation)
			{
			aError = ENoExtraInformation;
			}
		else if(errorStr == KEInitializationFailed)
			{
			aError = EInitializationFailed;
			}
		else if (errorStr == KEScanNextDirectoryFailed)
			{
			aError = EScanNextDirectoryFailed;
			}
		else if (errorStr == KESrcOpenFailed)
			{
			aError = ESrcOpenFailed;
			}
		else if (errorStr == KETrgOpenFailed)
			{
			aError = ETrgOpenFailed;
			}
		else if (errorStr == KENoFilesProcessed)
			{
			aError = ENoFilesProcessed;
			}
		else
			{
			ret = EFalse;	
			}
		}
	return ret;	
	}

/** Converts TFileManError type to string **/
void CT_FileManData::ConvertFileManErrorToString(TFileManError& aError, TPtrC& aErrorStr)
	{
	if(aError == ENoExtraInformation)
		{
		aErrorStr.Set(KENoExtraInformation());
		}
	else if(aError == EInitializationFailed)
		{
		aErrorStr.Set(KEInitializationFailed());
		}
	else if (aError == EScanNextDirectoryFailed)
		{
		aErrorStr.Set(KEScanNextDirectoryFailed());
		}
	else if (aError == ESrcOpenFailed)
		{
		aErrorStr.Set(KESrcOpenFailed());
		}
	else if (aError == ETrgOpenFailed)
		{
		aErrorStr.Set(KETrgOpenFailed());
		}
	else if (aError == ENoFilesProcessed)
		{
		aErrorStr.Set(KENoFilesProcessed());
		}
	}
	
/** Converts CFileMan::TAction type to string **/
void CT_FileManData::ConvertActionToString(CFileMan::TAction aAction, TPtrC& aActionStr)
	{
	if (aAction == CFileMan::EAttribs)
		{
		aActionStr.Set(KEAttribs());
		}
	else if (aAction == CFileMan::ECopy)
		{
		aActionStr.Set(KECopy());
		}
	else if (aAction == CFileMan::EDelete)
		{
		aActionStr.Set(KEDelete());
		}
	else if (aAction == CFileMan::EMove)
		{
		aActionStr.Set(KEMove());
		}
	else if (aAction == CFileMan::ERename)
		{
		aActionStr.Set(KERename());
		}
	else if (aAction == CFileMan::ERmDir)
		{
		aActionStr.Set(KERmDir());
		}
	else if (aAction == CFileMan::ERenameInvalidEntry)
		{
		aActionStr.Set(KERenameInvalidEntry());
		}
	else if (aAction == CFileMan::ECopyFromHandle)
		{
		aActionStr.Set(KECopyFromHandle());
		}
	else
		{
		aActionStr.Set(KENone());
		}
		
	}
	
/** Reads CFileMan::TAction from INI and converts it to CFileMan::TAction type. **/
TBool CT_FileManData::GetActionFromConfig(const TDesC& aParameterName, const TDesC& aSection, CFileMan::TAction& aAction)
	{
	TPtrC actionStr;
	TBool ret = GET_OPTIONAL_STRING_PARAMETER(aParameterName, aSection, actionStr);
	if(ret)
		{		
		if (actionStr == KEAttribs)
			{
			aAction = CFileMan::EAttribs;
			}
		else if (actionStr == KECopy)
			{
			aAction = CFileMan::ECopy;
			}
		else if (actionStr == KEDelete)
			{
			aAction = CFileMan::EDelete;
			}
		else if (actionStr == KEMove)
			{
			aAction = CFileMan::EMove;
			}
		else if (actionStr == KERename)
			{
			aAction = CFileMan::ERename;
			}
		else if (actionStr == KERmDir)
			{
			aAction = CFileMan::ERmDir;
			}
		else if (actionStr == KERenameInvalidEntry)
			{
			aAction = CFileMan::ERenameInvalidEntry;
			}
		else if (actionStr == KECopyFromHandle)
			{
			aAction = CFileMan::ECopyFromHandle;
			}
		else if (actionStr == KENone)
			{
			aAction = CFileMan::ENone;
			}
		else
			{
			ret = EFalse;
			}
		}
		return ret;
	}
	
/** Reads CFileMan::TSwitch from INI and converts it to CFileMan::TSwitch type. **/
TBool CT_FileManData::GetOperationFromConfig(const TDesC& aParameterName, const TDesC& aSection, TUint& aSwitch)
	{
	TPtrC	operation;
	TBool	ret=GET_MANDATORY_STRING_PARAMETER(aParameterName, aSection, operation);
	if(ret)
		{	
		if (operation == KEOverWrite)
			{
			aSwitch = CFileMan::EOverWrite;
			}
		else if (operation == KERecurse)
			{
			aSwitch = CFileMan::ERecurse;
			}
		else
			{
			TInt operationInt=0;
			ret = GET_MANDATORY_INT_PARAMETER(aParameterName, aSection, operationInt);
			if (ret)
				{
				aSwitch = (CFileMan::TSwitch) operationInt;
				}
			}
		}
	return ret;	
	}
	
/** Clears all arrays with history **/
void CT_FileManData::ClearHistory()
	{
	INFO_PRINTF1(_L("Clearing history!"));
	
	iStartedHistory.ResetAndDestroy();
	iOperationHistory.ResetAndDestroy();
	iEndedHistory.ResetAndDestroy();	
	}
	
/**
* Creates a history entry.
* This method runs only from NotifyFileManStarted(), NotifyFileManOperation(), NotifyFileManEnded().
**/
void CT_FileManData::CreateHistoryRecord(THistoryData& aRecord)
	{
	TFileName source;
	iFileMan->GetCurrentSource(source);
	aRecord.iCurrentSource = source;
	
	TFileName target;
	iFileMan->GetCurrentTarget(target);
	aRecord.iCurrentTarget = target;
	
	aRecord.iCurrentEntry = iFileMan->CurrentEntry();
	aRecord.iBytesTransferred = iFileMan->BytesTransferredByCopyStep();
	aRecord.iCurrentAction = iFileMan->CurrentAction();
	aRecord.iLastError = iFileMan->GetLastError();
	aRecord.iMoreInfoAboutError = iFileMan->GetMoreInfoAboutError();
	aRecord.iFullPath.Set( iFileMan->FullPath() );
	aRecord.iAbbreviatedPath.Set( iFileMan->AbbreviatedPath() );	
	}
	
/** Reads type of notification from INI and converts it to TObserverNotifyType **/
TBool CT_FileManData::GetNotifyType(const TDesC& aParameterName, const TDesC& aSection, TObserverNotifyType& aType)
	{
	TPtrC	type;
	TBool	ret=GET_MANDATORY_STRING_PARAMETER(aParameterName, aSection, type);
	if(ret)
		{	
		if (type == KNotifyStarted)
			{
			aType = ENotifyStarted;
			}
		else if (type == KNotifyOperation)
			{
			aType = ENotifyOperation;
			}
		else if (type == KNotifyEnded)
			{
			aType = ENotifyEnded;
			}
		else
			{
			ret = EFalse;
			}
		}
	return ret;	
	}

/** Returns specific array of history using TObserverNotifyType value   **/	
RPointerArray<THistoryData>* CT_FileManData::GetHistoryDataByType(const TDesC& aSection)
	{	
	TObserverNotifyType type;
	RPointerArray<THistoryData>* history = NULL;
	//Reads history name which must be returned.
	if (GetNotifyType(KNotifyType(), aSection, type))
		{
		if(type == ENotifyStarted)
			{
			history = (RPointerArray<THistoryData>*)&iStartedHistory;
			}
		else if(type == ENotifyOperation)
			{
			history = (RPointerArray<THistoryData>*)&iOperationHistory;
			}
		else if(type == ENotifyEnded)
			{
			history = (RPointerArray<THistoryData>*)&iEndedHistory;
			}
		}
	return history;
	}
/** Reads TControl from config and converts it to TControl type  **/
TBool CT_FileManData::GetTControlFromConfig(const TDesC& aParameterName, const TDesC& aSection, TControl& aFlag)
	{
	TPtrC flag;
	TBool	ret=GET_MANDATORY_STRING_PARAMETER(aParameterName, aSection, flag);
	if(ret)
		{	
		if (flag == KEContinue)
			{
			aFlag = EContinue;
			}
		else if (flag == KEAbort)
			{
			aFlag = EAbort;
			}
		else if (flag == KECancel)
			{
			aFlag = ECancel;
			}
		else if (flag == KERetry)
			{
			aFlag = ERetry;
			}
		else
			{
			ret = EFalse;
			}
		}
	return ret;	
	}

/** Reads TControl and file name from INI  **/	
void CT_FileManData::ReadTControl(const TDesC& aSection)
	{
	TBool dataOk = ETrue;
	if(!GetNotifyType(KNotifyType(), aSection, iObserverNotifyType))
		{
			dataOk = EFalse;
		}
	TPtrC fileName;
	if(GET_MANDATORY_STRING_PARAMETER(KFileName(), aSection, fileName))
		{
		iNotifyFileName = fileName;
		}
	else
		{
		dataOk = EFalse;	
		}
	if(!GetTControlFromConfig(KFlag(), aSection, iTControl))
		{
		dataOk = EFalse;
		}
	if(dataOk)
		{
		iUseTControl = ETrue;
		}
	}


