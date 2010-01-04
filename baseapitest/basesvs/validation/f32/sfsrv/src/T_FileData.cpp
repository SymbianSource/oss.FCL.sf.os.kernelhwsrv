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

This contains CT_FileData
*/

//	User includes
#include "T_FileData.h"


/*@{*/
///	Parameters
_LIT(KFile,								"file");
_LIT(KUParamPath,						"path");
_LIT(KUParamName,						"name");
_LIT(KRFsName,							"RFs");
_LIT(KFileMode,							"file_mode");
_LIT(KUParamExpectedString,				"expected_string");
_LIT(KUParamAsync,						"async");
_LIT(KUParamBufferLength,				"buffer_length");
_LIT(KUParamLength,						"length");
_LIT(KUParamStartPos,					"start_pos");
_LIT(KUParamEndPos,						"end_pos");
_LIT(KUParamUsage,						"usage");
_LIT(KUParamPos,						"position");
_LIT(KSeek, 							"seek");
_LIT(KUParamCancelAll,					"cancel_all");
_LIT(KUParamData, 						"datawrite");
_LIT(KUParamSize, 						"size");
_LIT(KUParamExpectedSize,				"expected_size");
_LIT(KUParamCompareValue,				"compare_value");
_LIT(KUParamSetAttMask,					"set_att_mask");
_LIT(KUParamClearAttMask, 				"clear_att_mask");
_LIT(KUParamDate, 						"date");
_LIT(KFileObjectName,					"file_object_name");
_LIT(KUParamOwnerType,					"owner_type");
_LIT(KUParamComparePath,				"compare_path");

// OwnerType

_LIT(KEOwnerProcess, 					"EOwnerProcess");
_LIT(KEOwnerThread, 					"EOwnerThread");

// Block Usage

_LIT(KEBlockMapUsagePaging, 			"EBlockMapUsagePaging");
_LIT(KETestDebug, 						"ETestDebug");

// FileMode
_LIT(KEFileShareExclusive,	 			"EFileShareExclusive");
_LIT(KEFileShareReadersOnly,	 		"EFileShareReadersOnly");
_LIT(KEFileShareAny,	 				"EFileShareAny");
_LIT(KEFileShareReadersOrWriters,	 	"EFileShareReadersOrWriters");
_LIT(KEFileStream,	 					"EFileStream");
_LIT(KEFileStreamText,					"EFileStreamText");
_LIT(KEFileRead,	 					"EFileRead");
_LIT(KEFileWrite,						"EFileWrite");
_LIT(KEFileReadAsyncAll,	 			"EFileReadAsyncAll");


// Seek
_LIT(KESeekStart,						"ESeekStart");
_LIT(KESeekCurrent,						"ESeekCurrent");
_LIT(KESeekEnd,							"ESeekEnd");

///	Commands
_LIT(KCmdNew,							"new");
_LIT(KCmdClose,							"Close");
_LIT(KCmdDestructor,					"~");
_LIT(KCmdBlockMap,						"BlockMap");
_LIT(KCmdOpen,							"Open");
_LIT(KCmdCreate,						"Create");
_LIT(KCmdWrite,							"Write");
_LIT(KCmdReplace, 						"Replace");
_LIT(KCmdRead,							"Read");
_LIT(KCmdFlush,							"Flush");
_LIT(KCmdTemp,							"Temp");
_LIT(KCmdRename,						"Rename");
_LIT(KCmdSeek,							"Seek");
_LIT(KCmdReadCancel,					"ReadCancel");
_LIT(KCmdLock,							"Lock");
_LIT(KCmdUnLock,						"UnLock");
_LIT(KCmdSize, 							"Size");
_LIT(KCmdSetSize, 						"SetSize");
_LIT(KCmdAtt,							"Att");
_LIT(KCmdSetAtt,						"SetAtt");
_LIT(KCmdModified, 						"Modified");
_LIT(KCmdSetModified,					"SetModified");
_LIT(KCmdSet,							"Set");
_LIT(KCmdChangeMode, 					"ChangeMode");
_LIT(KCmdDrive,							"Drive");
_LIT(KCmdDuplicate, 					"Duplicate");
_LIT(KCmdName,							"Name");
_LIT(KCmdFullName,						"FullName");


//Attributes
_LIT(KDEntryAttNormal,  				"KEntryAttNormal");
_LIT(KDEntryAttReadOnly, 				"KEntryAttReadOnly");
_LIT(KDEntryAttArchive,					"KEntryAttArchive");
_LIT(KDEntryAttHidden,					"KEntryAttHidden");
_LIT(KDEntryAttSystem,					"KEntryAttSystem");
_LIT(KDEntryAttVolume,					"KEntryAttVolume");
_LIT(KDEntryAttDir,						"KEntryAttDir");
_LIT(KDEntryAttXIP,						"KEntryAttXIP");
_LIT(KDEntryAttRemote,					"KEntryAttRemote");

//constants
const TInt KDefaultDescSize = 64;

/*@}*/

CT_FileData* CT_FileData::NewL()
/**
 * Two phase constructor
 */
	{
	CT_FileData* ret = new (ELeave) CT_FileData();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}

CT_FileData::CT_FileData()
/**
 * Protected constructor. First phase construction
 */
:	iFile(NULL)
	{
	}

void CT_FileData::ConstructL()
/**
 * Protected constructor. Second phase construction
 */
	{
	iFileOpened = EFalse;
	}

CT_FileData::~CT_FileData()
/**
 * Destructor.
 */
	{
	DoCleanup();
	}

void CT_FileData::DoCleanup()
/**
 * Contains cleanup implementation
 */
	{
	//Deleting RFile.
	if(iFile != NULL)
		{
		INFO_PRINTF1(_L("Deleting current RFile"));
		delete iFile;
		iFile = NULL;
		}
	iReadCallbackArray.Close();
	iWriteCallbackArray.Close();
	iFlushCallbackArray.Close();
	}

TAny* CT_FileData::GetObject()
/**
 * Return a pointer to the object that the data wraps
 *
 * @return pointer to the object that the data wraps
 */
	{
	return iFile;
	}

TBool CT_FileData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex)
/**
 * Process a command read from the ini file
 *
 * @param aCommand	the command to process
 * @param aSection		the entry in the ini file requiring the command to be processed
 * @param aAsyncErrorIndex the index of asynchronous command error code belongs to.
 *
 * @return ETrue if the command is processed
 */
	{
	TBool retVal = ETrue;

	if (aCommand == KCmdNew)
		{
		DoCmdNewL();
		}
	else if (aCommand == KCmdDestructor)
		{
		DoCmdDestructor();
		}
	else if (aCommand == KCmdOpen)
		{
		DoCmdOpenL(aSection);
		}
	else if (aCommand == KCmdWrite)
		{
		DoCmdWriteL(aSection, aAsyncErrorIndex);
		}
	else if (aCommand == KCmdClose)
		{
		DoCmdClose();
		}
	else if (aCommand == KCmdCreate)
		{
		DoCmdCreateL(aSection);
		}
	else if (aCommand == KCmdReplace)
		{
		DoCmdReplaceL(aSection);
		}
	else if (aCommand == KCmdRead)
		{
		DoCmdReadL(aSection, aAsyncErrorIndex);
		}	
	else if (aCommand == KCmdFlush)
		{
		DoCmdFlushL(aSection, aAsyncErrorIndex);
		}
	else if (aCommand == KCmdTemp)
		{
		DoCmdTempL(aSection);
		}	
	else if (aCommand == KCmdRename)
		{
		DoCmdRenameL(aSection);
		}
	else if (aCommand == KCmdSeek)
		{
		DoCmdSeekL(aSection);
		}
	else if (aCommand == KCmdReadCancel)
		{
		DoCmdReadCancelL(aSection);
		}	
	else if (aCommand == KCmdLock)
		{
		DoCmdLockL(aSection);
		}
	else if (aCommand == KCmdUnLock)
		{
		DoCmdUnLockL(aSection);
		}
	else if (aCommand == KCmdSize)
		{
		DoCmdSizeL(aSection);
		}		
	else if (aCommand == KCmdSetSize)
		{
		DoCmdSetSizeL(aSection);
		}
	else if (aCommand == KCmdAtt)
		{
		DoCmdAttL(aSection);
		}		
	else if (aCommand == KCmdSetAtt)
		{
		DoCmdSetAttL(aSection);
		}		
	else if (aCommand == KCmdModified)
		{
		DoCmdModifiedL(aSection);
		}
	else if (aCommand == KCmdSetModified)
		{
		DoCmdSetModifiedL(aSection);
		}
	else if (aCommand == KCmdSet)
		{
		DoCmdSetL(aSection);
		}
	else if (aCommand == KCmdChangeMode)
		{
		DoCmdChangeModeL(aSection);
		}
	else if (aCommand == KCmdDrive)
		{
		DoCmdDriveL(aSection);
		}
	else if (aCommand == KCmdName)
		{
		DoCmdNameL(aSection);
		}
	else if (aCommand == KCmdDuplicate)
		{
		DoCmdDuplicateL(aSection);
		}
	else if (aCommand == KCmdFullName)
		{
		DoCmdFullName(aSection);
		}
	else if (aCommand == KCmdBlockMap)
		{
		DoCmdBlockMap(aSection);
		}
	else
		{
		retVal = EFalse;
		}
	return retVal;
	}

void CT_FileData::DoCmdNewL()
/** Creates new RFile class instance */
	{
	//Deletes previous RFile class instance if it was already created.
	DoCleanup();

	INFO_PRINTF1(_L("Create new RFile class instance"));

	// do create
	TRAPD(err, iFile = new (ELeave) RFile());
	if ( err!=KErrNone )
		{
		ERR_PRINTF2(_L("Error returned by New()%d"), err);
		SetError(err);
		}
	}

void CT_FileData::DoCmdDestructor()
/** Destroy RFile the object */
	{
	DoCleanup();
	}

void CT_FileData::DoCmdOpenL(const TDesC& aSection)
/**	Opens files */
	{
	RFs*	rfsObject=NULL;
	TPtrC	rfsObjectName;
	TBool	dataOk=GET_MANDATORY_STRING_PARAMETER(KRFsName(), aSection, rfsObjectName);
	if ( dataOk )
		{
		rfsObject=(RFs*)GetDataObjectL(rfsObjectName);
		}

	TPtrC	filePath;
	if ( !GET_MANDATORY_STRING_PARAMETER(KFile(), aSection, filePath) )
		{
		dataOk=EFalse;
		}

	TUint	fileMode = 0;
	if ( !GetFileModeL(KFileMode, aSection, fileMode) )
		{
		ERR_PRINTF2(_L("Open() error reading parameter. %S"), &KFileMode());
		SetBlockResult(EFail);
		dataOk=EFalse;
		}

	if ( dataOk )
		{
		if (rfsObject)
			{
			TInt	err = iFile->Open(*rfsObject, filePath, fileMode);
			if ( err!=KErrNone )
				{
				ERR_PRINTF2(_L("Open() failed with Error:%d"), err);
				SetError(err);
				}
			else
				{
				iFileOpened = true;
				}
			}
		else
			{
			ERR_PRINTF1(_L("RFs object is NULL"));
			SetBlockResult(EFail);
			}
		}
	}


void CT_FileData::DoCmdClose()
/** Close file	 */
	{
	INFO_PRINTF1(_L("Closing RFile"));
	iFile->Close();
	}

void CT_FileData::DoCmdCreateL(const TDesC& aSection)
/** create a new file */
	{
	RFs*	rfsObject=NULL;
	TPtrC	rfsObjectName;
	TBool	dataOk = ETrue;
	if ( GET_MANDATORY_STRING_PARAMETER(KRFsName, aSection, rfsObjectName) )
		{
		rfsObject=(RFs*)GetDataObjectL(rfsObjectName);
		}
	else
		{
		dataOk = EFalse;
		}

	//	Gets name of file from ini file.
	TPtrC	name;
	if ( !GET_MANDATORY_STRING_PARAMETER(KUParamName(), aSection, name) )
		{
		dataOk = EFalse;
		}

	TUint	fileMode = 0;
	if ( !GetFileModeL(KFileMode, aSection, fileMode) )
		{
		ERR_PRINTF2(_L("Create() error reading parameter. %S"), &KFileMode());
		SetBlockResult(EFail);
		dataOk = EFalse;
		}

	if ( dataOk )
		{
		//	Creates and opens a new file for writing.
		if (rfsObject)
			{
			TInt	err = iFile->Create(*rfsObject, name, fileMode);
			if ( err!=KErrNone )
				{
				ERR_PRINTF2(_L("Create(), error create() = %d"), err);
				SetError(err);
				}
			}
		else
			{
			ERR_PRINTF1(_L("RFs object is NULL"));
			SetBlockResult(EFail);
			}
		}
	}
	
void CT_FileData::DoCmdReplaceL(const TDesC& aSection)
/** replace a existing file or create new */
	{
	TPtrC rfsObjectName;
	RFs* rfsObject = NULL;
	
	TBool	dataOk = ETrue;
	if ( GET_MANDATORY_STRING_PARAMETER(KRFsName, aSection, rfsObjectName) )
		{
		rfsObject=(RFs*)GetDataObjectL(rfsObjectName);
		}

	//	Gets name of file from ini file.
	TPtrC path;
	if (!GET_MANDATORY_STRING_PARAMETER(KFile(), aSection, path))
		{
		dataOk = EFalse;
		}
	
	TUint fileMode = 0;
	if(!GetFileModeL(KFileMode, aSection, fileMode))
		{
		ERR_PRINTF2(_L("Replace() error reading parameter. %S"), &KFileMode());
		SetBlockResult(EFail);
		dataOk = EFalse;
		}

	//	Creates and opens a new file for writing.
	if ( dataOk )
		{
		//	Creates and opens a new file for writing.
		if (rfsObject)
			{
			TInt	err = iFile->Replace(*rfsObject, path, fileMode);
			if ( err!=KErrNone )
				{
				ERR_PRINTF2(_L("Replace() failed with error = %d"), err);
				SetError(err);
				}
			else
				{
				iFileOpened = ETrue;
				INFO_PRINTF2(_L("File: %S is replaced successfuly"), &path);				
				}
			}
		else
			{
			ERR_PRINTF1(_L("RFs object is NULL"));
			SetBlockResult(EFail);
			}
		}

	}	

void CT_FileData::DoCmdReadL(const TDesC& aSection, const TInt aAsyncErrorIndex)
/** wrapper for read commands */
	{
	TBool async = EFalse;
		if(GET_OPTIONAL_BOOL_PARAMETER(KUParamAsync, aSection, async))
			{
			if (async)
				{
				DoAsynchronousReadsL(aSection, aAsyncErrorIndex);	
				}
			else
				{
				DoSynchronousReadsL(aSection);
				}
			
			}
		else
			{
			DoSynchronousReadsL(aSection);
			}
	}

TBool CT_FileData::GetFileModeL(const TDesC& aParameterName, const TDesC& aSection, TUint& aFileMode)
/** process the filemode from ini  */
	{
	TBool ret = ETrue;
	
	TPtrC aModeStr;
	
	if(GET_OPTIONAL_STRING_PARAMETER(aParameterName, aSection, aModeStr))
		{
		//break the file mode string to array of file modes
		//in case there's more than one filemode		
		RPointerArray<HBufC> fileModes = SplitL(aModeStr, "|");		
			
		for ( int i = 0 ; i < fileModes.Count() ; i++ )
			{
			if (*fileModes[i] == KEFileShareExclusive)
				{
				aFileMode = aFileMode | EFileShareExclusive ;
				}
			else if (*fileModes[i] ==  KEFileShareReadersOnly)
				{
				aFileMode = aFileMode | EFileShareReadersOnly;
				}
			else if (*fileModes[i] ==  KEFileShareAny)
				{
				aFileMode = aFileMode | EFileShareAny;
				}		
			else if (*fileModes[i] ==  KEFileShareReadersOrWriters)
				{
				aFileMode = aFileMode | EFileShareReadersOrWriters;
				}					
			else if (*fileModes[i] ==  KEFileStream)
				{
				aFileMode = aFileMode | EFileStream;
				}					
			else if (*fileModes[i] ==  KEFileStreamText)
				{
				aFileMode = aFileMode | EFileStreamText;
				}					
			else if (*fileModes[i] ==  KEFileRead)
				{
				aFileMode = aFileMode | EFileRead;
				}					
			else if (*fileModes[i] ==  KEFileWrite)
				{
				aFileMode = aFileMode | EFileWrite;
				}
			else if (*fileModes[i] ==  KEFileReadAsyncAll)
				{
				aFileMode = aFileMode | EFileReadAsyncAll;
				}
			else
				{
				aFileMode = EFileShareAny;
				ret = EFalse;
				}
			}
			
		for (int j=0 ; j<fileModes.Count() ; j++)
			{
			HBufC* temp = fileModes[j];
			delete temp;
			temp = NULL;
			}
			
		fileModes.Close();
		}
	else
		{
		ret = EFalse;
		}

	return ret;
	}


void CT_FileData::DoSynchronousReadsL(const TDesC& aSection)
/** process all synchronous reads */
	{	
	TInt length;
	TInt pos;
	
	HBufC8* filedata = NULL;	
	TInt bufferLength;
	
//get optional buffer length from ini file	
	if (GET_OPTIONAL_INT_PARAMETER(KUParamBufferLength, aSection, bufferLength))
		{
		filedata = HBufC8::NewLC(bufferLength);
		}
	else
		{
		filedata = HBufC8::NewLC(KDefaultDescSize);
		}
				
		
	TPtr8 filedataPtr = filedata->Des();
	TInt err = 0;
//devide which read to call, by switching between parameters fetched from ini file.	
	if(GET_OPTIONAL_INT_PARAMETER(KUParamLength, aSection, length))
		{
		if(GET_OPTIONAL_INT_PARAMETER(KUParamPos, aSection, pos))
			{
			INFO_PRINTF1(_L("calling RFile::Read(TInt& aPos, TDes8 &aData, TInt aLen)"));
			err = iFile->Read(pos, filedataPtr, length);
			HandleExpectedString(filedataPtr, aSection);
			}
		else
			{
			INFO_PRINTF1(_L("calling RFile::Read(TDes8 &aData, TInt aLen)"));
			err = iFile->Read(filedataPtr, length);
			HandleExpectedString(filedataPtr, aSection);
			}
		}
	else
		{
		if(GET_OPTIONAL_INT_PARAMETER(KUParamPos, aSection, pos))
			{
			INFO_PRINTF1(_L("calling RFile::Read(TInt& aPos, TDes8 &aData)"));
			err = iFile->Read(pos, filedataPtr);
			HandleExpectedString(filedataPtr, aSection);
			}
		else
			{
			INFO_PRINTF1(_L("calling RFile::Read(TDes8 &aData)"));
			err = iFile->Read(filedataPtr);
			HandleExpectedString(filedataPtr, aSection);
			}
		}
		
	TBuf<KDefaultDescSize> filedata16;
	filedata16.Copy(filedataPtr);
	INFO_PRINTF2(_L("read \"%S\" from file"), &filedata16);		
		
//did the read complete with error code?		
	if (err != KErrNone)
		{
		ERR_PRINTF2(_L("error executing synchronous read %d"), err);
		SetError(err);
		}
		
	CleanupStack::PopAndDestroy(1);		
		
	}
		
void CT_FileData::DoAsynchronousReadsL(const TDesC& aSection, const TInt aAsyncErrorIndex)
/**process all asynchronous reads */
	{
	TInt length;
	TInt pos;
	
	CT_FileActiveCallback* active = CT_FileActiveCallback::NewL(*this);
	iReadCallbackArray.Append(active);
		
	TInt bufferLength;
	
	//Get the buffer length, and create the buffer for activeCallback object
	if (GET_OPTIONAL_INT_PARAMETER(KUParamBufferLength, aSection, bufferLength))
		{
		active->CreateFileDataBufferL(bufferLength);
		}
	else
		{
		active->CreateFileDataBufferL(KDefaultDescSize);
		}	
		
	TPtr8 fileDataPtr = active->iFileData->Des();
		
	//decide which read to call	
	if(GET_OPTIONAL_INT_PARAMETER(KUParamLength, aSection, length))
		{
		if(GET_OPTIONAL_INT_PARAMETER(KUParamPos, aSection, pos))
			{
			INFO_PRINTF1(_L("calling RFile::Read(TInt& aPos, TDes8 &aData, TInt aLen, TRequestiStatus aiStatus)"));
			iFile->Read(pos, fileDataPtr, length,active->iStatus);
			}
		else
			{
			INFO_PRINTF1(_L("calling RFile::Read(TDes8 &aData, TInt aLen, TRequestiStatus aiStatus)"));
			iFile->Read(fileDataPtr, length, active->iStatus);
			}
		}
	else
		{
		if(GET_OPTIONAL_INT_PARAMETER(KUParamPos, aSection, pos))
			{
			INFO_PRINTF1(_L("calling RFile::Read(TInt& aPos, TDes8 &aData, TRequestiStatus aiStatus)"));
			iFile->Read(pos, fileDataPtr, active->iStatus);
			}
		else
			{
			INFO_PRINTF1(_L("calling RFile::Read(TDesC &aData, TRequestiStatus aiStatus)"));
			iFile->Read(fileDataPtr, active->iStatus);
			}
		}
		
	//acitvate callback object
	active->SetSection(aSection);
	active->Activate(aAsyncErrorIndex);
	IncOutstanding();

	}
	
	
void CT_FileData::HandleExpectedString(const TDesC& aReadedData, const TDesC& aSection)
/** compare the expected string with readed string, takes 16-bit parameter*/
	{
	TPtrC expectedString;
	if(GET_OPTIONAL_STRING_PARAMETER(KUParamExpectedString, aSection, expectedString))
		{
		if (aReadedData == expectedString)
			{
			INFO_PRINTF1(_L("expected string matches with the data read"));			
			}
		else
			{
			ERR_PRINTF1(_L("expected string does not match with the data read"));
			SetBlockResult(EFail);
			}
		}
	}
	

void CT_FileData::HandleExpectedString(const TDesC8& aReadedData, const TDesC& aSection)
/** compare the expected string with readed string, takes 16-bit parameter */
	{
	TBuf<KDefaultDescSize> readedData16;
	readedData16.Copy(aReadedData);
	TPtrC expectedString;
	if(GET_OPTIONAL_STRING_PARAMETER(KUParamExpectedString, aSection, expectedString))
		{
		if (readedData16 == expectedString)
			{
			INFO_PRINTF1(_L("expected string matches with the data read"));			
			}
		else
			{
			ERR_PRINTF1(_L("expected string does not match with the data read"));
			SetBlockResult(EFail);
			}
		}
	}		
	
	
void CT_FileData::DoCmdFlushL(const TDesC& aSection, const TInt aAsyncErrorIndex)
/** flush all the internal buffers to file */
	{
	TBool async = false;
	TInt err = 0;
	//get the parameter from ini, to decide which version of RFile::Flush to call
	GET_OPTIONAL_BOOL_PARAMETER(KUParamAsync, aSection, async);
	if (async == true)
		{		
		CActiveCallback* active = CActiveCallback::NewL(*this);
		iFlushCallbackArray.Append( active );
		iFile->Flush(active->iStatus);
		active->Activate(aAsyncErrorIndex);
		IncOutstanding();
		}
	else
		{
		err = iFile->Flush();
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("Flush() failed with error code. %d"), err);
			SetError(err);
			}
		else
			{
			INFO_PRINTF1(_L("Succesfully flushed the file"));
			}
		}	
	}	
	

void CT_FileData::DoCmdTempL(const TDesC& aSection)
/* create temporary file */
	{
	
	TPtrC rfsObjectName;
	RFs* rfsObject = NULL;
	
	TBool	dataOk=GET_MANDATORY_STRING_PARAMETER(KRFsName, aSection, rfsObjectName);
	if ( dataOk )
		{
		rfsObject=(RFs*)GetDataObjectL(rfsObjectName);
		}

	//	Gets name of file from ini file.
	TPtrC path;
	if (!GET_MANDATORY_STRING_PARAMETER(KUParamPath(), aSection, path))
		{
		dataOk=EFalse;
		}
	
	TUint fileMode = 0;
	if(!GetFileModeL(KFileMode, aSection, fileMode))
		{
		ERR_PRINTF2(_L("Temp() error reading parameter. %S"), &KFileMode());
		SetBlockResult(EFail);
		dataOk=EFalse;
		}

	//	Creates Temporary file.
	TFileName fileName;
	if ( dataOk )
		{
		if (rfsObject)
			{
			TInt	err = iFile->Temp(*rfsObject, path, fileName,fileMode);
			INFO_PRINTF2(_L("File: %S is created"), &fileName);			
			if ( err!=KErrNone )
				{
				ERR_PRINTF2(_L("Temp() failed with error = %d"), err);
				SetError(err);
				}
			else
				{
				iFileOpened = ETrue;				
				}				
			}
		else
			{
			ERR_PRINTF1(_L("RFs object is NULL"));
			SetBlockResult(EFail);
			}
		}	
	
	}	
	
void CT_FileData::DoCmdRenameL(const TDesC& aSection)
/** rename the file */
	{
	
	TPtrC newName;
	//get the name from ini and rename the file.
	if(GET_MANDATORY_STRING_PARAMETER(KUParamName(), aSection, newName))
		{	
		TInt err = iFile->Rename(newName);
	
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("Rename() failed with error code %d"), err);
			SetError(err);
			}
		}
	
	}	
	
void CT_FileData::DoCmdSeekL(const TDesC& aSection)
/** set the file cursor to specified location in file */
	{
	TSeek seek = ESeekStart;
	TBool dataOk = EFalse;
	TInt pos;

	//convert the mode string from ini to TSeek
	if(ConvertToSeek(KSeek, aSection,seek))
		{
		dataOk = GET_MANDATORY_INT_PARAMETER(KUParamPos(), aSection, pos);		
		}	
		
	if(dataOk)
		{
		TInt err = iFile->Seek(seek, pos);
	
		if(err != KErrNone)	
			{
			ERR_PRINTF2(_L("Seek(), error seeking() = %d"), err);
			SetError(err);
			}
		else
			{
			INFO_PRINTF1(_L("Succesfully seeked the file"));
			}
		}
	}	
	
TBool CT_FileData::ConvertToSeek(const TDesC& aParameterName, const TDesC& aSection, TSeek& aSeek)
/** convert a seek mode string from ini to TSeek */
	{
	TBool ret = ETrue;
	
	TPtrC aModeStr;
	
	if(GET_MANDATORY_STRING_PARAMETER(aParameterName, aSection, aModeStr))
		{
		
		if (aModeStr == KESeekStart)
			{
			aSeek = ESeekStart;
			}
		else if (aModeStr == KESeekCurrent)
			{
			aSeek = ESeekCurrent;
			}
		else if (aModeStr == KESeekEnd)
			{
			aSeek = ESeekEnd;
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

	return ret;
	}	
	
	
void CT_FileData::DoCmdReadCancelL(const TDesC& aSection)
/** wrapper for readcancels */
	{
	
	TBool cancelAll;
	
	//decide which ReadCancel to call
	if(GET_OPTIONAL_BOOL_PARAMETER(KUParamCancelAll, aSection, cancelAll))
		{
		if (cancelAll == true)
			{
			ReadCancelAll();
			}
		else
			{
			ReadCancel();
			}
		}
	else
		{
		ReadCancel();
		}
	}	
		
void CT_FileData::ReadCancel()
/** will cancel the last active request by default */
	{
	iFile->ReadCancel(iReadCallbackArray[iReadCallbackArray.Count()-1]->iStatus);
	}
	
void CT_FileData::ReadCancelAll()
/** cancel all outstanding read requests */
	{
	iFile->ReadCancel();
	}

// the write implementation
void CT_FileData::DoCmdWriteL(const TDesC& aSection, const TInt aAsyncErrorIndex)
/** wrapper for write commands */
	{
	TBool async = EFalse;
	//decide which write to call
	GET_OPTIONAL_BOOL_PARAMETER(KUParamAsync, aSection, async);
	if (async == true)
		{
		DoAsynchronousWritesL(aSection, aAsyncErrorIndex);	
		}
	else
		{
		DoSynchronousWritesL(aSection);
		}
	}
	
void CT_FileData::DoAsynchronousWritesL(const TDesC& aSection, const TInt aAsyncErrorIndex)
/** call asynchronous writes */
	{
	HBufC8* dataWrite8;
	TInt bufferLength;
	
	//Get the buffer length, and create the buffer for activeCallback object
	if (GET_OPTIONAL_INT_PARAMETER(KUParamBufferLength, aSection, bufferLength))
		{
		dataWrite8 = HBufC8::NewLC(bufferLength);
		}
	else
		{
		dataWrite8 = HBufC8::NewLC(KDefaultDescSize);
		}	
	TPtr8 data8Ptr = dataWrite8->Des();
	
	TPtrC data;
	// Get the data to write
	if(GET_MANDATORY_STRING_PARAMETER(KUParamData(), aSection, data))
		{
		//convert it to 8-bit
		data8Ptr.Copy(data);
		
		CActiveCallback* active = CActiveCallback::NewL(*this);
	
		iWriteCallbackArray.Append( active );
		
		TInt length;
		TInt pos;	
		//now, decide which read to call
		if(GET_OPTIONAL_INT_PARAMETER(KUParamLength, aSection, length))
			{
			if(GET_OPTIONAL_INT_PARAMETER(KUParamPos, aSection, pos))
				{
				INFO_PRINTF1(_L("calling RFile::Write(TInt& aPos, TDesC8 &aData, TInt aLen, TRequestStatus &aStatus)"));
				iFile->Write(pos, *dataWrite8, length, active->iStatus);
				}
			else
				{
				INFO_PRINTF1(_L("calling RFile::Write(TDesC8 &aData, TInt aLen, , TRequestStatus &aStatus)"));
				iFile->Write(*dataWrite8, length, active->iStatus);
				}
			}
		else
			{
			if(GET_OPTIONAL_INT_PARAMETER(KUParamPos, aSection, pos))
				{
				INFO_PRINTF1(_L("calling RFile::Write(TInt& aPos, TDesC8 &aData, TRequestStatus &aStatus)"));
				iFile->Write(pos, *dataWrite8, active->iStatus);
				}
			else
				{
				INFO_PRINTF1(_L("calling RFile::Write(TDesC8 &aData, TRequestStatus &aStatus)"));
				iFile->Write(*dataWrite8, active->iStatus);
				}
			}
			
		active->Activate(aAsyncErrorIndex);
		IncOutstanding();
		}

	CleanupStack::PopAndDestroy();	
	}
	
void CT_FileData::DoSynchronousWritesL(const TDesC& aSection)
/** cakk synchronous writes */
	{
	HBufC8* dataWrite8;
	TInt bufferLength;
	
	//Get the buffer length, and create the buffer for activeCallback object
	if (GET_OPTIONAL_INT_PARAMETER(KUParamBufferLength, aSection, bufferLength))
		{
		dataWrite8 = HBufC8::NewLC(bufferLength);
		}
	else
		{
		dataWrite8 = HBufC8::NewLC(KDefaultDescSize);
		}	
	TPtr8 data8Ptr = dataWrite8->Des();
	TInt err = 0;
	
	TPtrC data;	
	// get the data
	if (GET_MANDATORY_STRING_PARAMETER(KUParamData(), aSection, data))
		{
		//convert it to 8-bit		
		data8Ptr.Copy(data);
		
		TInt length;
		TInt pos;
		
		//will decide which write to call 
		if(GET_OPTIONAL_INT_PARAMETER(KUParamLength, aSection, length))
			{
			if(GET_OPTIONAL_INT_PARAMETER(KUParamPos, aSection, pos))
				{
				INFO_PRINTF1(_L("calling RFile::Write(TInt& aPos, TDesC8 &aData, TInt aLen)"));
				err = iFile->Write(pos, *dataWrite8, length);
				}
			else
				{
				INFO_PRINTF1(_L("calling RFile::Write(TDesC8 &aData, TInt aLen)"));
				err = iFile->Write(*dataWrite8, length);
				}
			}
		else
			{
			if(GET_OPTIONAL_INT_PARAMETER(KUParamPos, aSection, pos))
				{
				INFO_PRINTF1(_L("calling RFile::Write(TInt& aPos, TDesC8 &aData)"));
				err = iFile->Write(pos, *dataWrite8);
				}
			else
				{
				INFO_PRINTF1(_L("calling RFile::Write(TDesC8 &aData)"));
				err = iFile->Write(*dataWrite8);
				}
			}
			
		if (err == KErrNone)
			{
			INFO_PRINTF2(_L("written \"%S\" to file"), &data);
			}
		else
			{
			ERR_PRINTF2(_L("Error executing synchronous write %d"), err);
			SetError(err);
			}
		}
	CleanupStack::PopAndDestroy();	
	}	
	
void CT_FileData::DoCmdLockL(const TDesC& aSection)
/** lock a region in file */
	{
	
	TInt pos=0;
	TBool dataOk = ETrue;
	
	//get the position
	if(!GET_MANDATORY_INT_PARAMETER(KUParamPos(), aSection, pos))
		{
		dataOk = EFalse;
		}
		
	TInt length=0;	
	
	// get the length
	if(!GET_MANDATORY_INT_PARAMETER(KUParamLength(), aSection, length))
		{
		dataOk = EFalse;
		}
	
	//Lock!!
	if (dataOk)
		{
		TInt err = iFile->Lock(pos, length);
		
		if(err!=KErrNone)
			{
			ERR_PRINTF2(_L("Lock() error locking file. %d"), err);
			SetError(err);	
			}	
		}

	}	
	
	
void CT_FileData::DoCmdUnLockL(const TDesC& aSection)
/** unlock a region that has been previously locked */
	{
	TBool dataOk = ETrue;
	
	TInt pos=0;
	
	//get the position
	if(!GET_MANDATORY_INT_PARAMETER(KUParamPos(), aSection, pos))
		{
		dataOk = EFalse;
		}

	TInt length=0;
	
	// get the length
	if(!GET_MANDATORY_INT_PARAMETER(KUParamLength(), aSection, length))
		{
		dataOk = EFalse;
		}
	
	//call UnLock
	if (dataOk)
		{
		TInt err = iFile->UnLock(pos, length);
		
		if(err!=KErrNone)
			{
			ERR_PRINTF2(_L("UnLock() error unlocking file. %d"), err);
			SetError(err);	
			}
		else
			{
			INFO_PRINTF1(_L("Succesfully unlocked the file"));
			}
		}
	
	}	
	
	
void CT_FileData::DoCmdSizeL(const TDesC& aSection)
/** get file size */
	{
	
	TInt expectedSize =  0;
	
	TInt size = 0;
	TInt err = iFile->Size(size);
	INFO_PRINTF2(_L("The Size of the file is %d bytes"), size);	
	
	if (err != KErrNone)
		{
		ERR_PRINTF2(_L("Size() error getting file size. %d"), err);
		SetError(err);
		}
	else
		{
		//get expected size from ini
		if(GET_OPTIONAL_INT_PARAMETER(KUParamExpectedSize(), aSection, expectedSize))		
			{
			if(expectedSize != size)
				{
				ERR_PRINTF3(_L("The file size does not match the expected size %d != %d"), size, expectedSize)				
				SetBlockResult(EFail);
				}
			}
		}
	
	}

void CT_FileData::DoCmdSetSizeL(const TDesC& aSection)
/** set file size */
	{
	TInt size=0;
	
	if(GET_MANDATORY_INT_PARAMETER(KUParamSize(), aSection, size))
		{
		TInt err = iFile->SetSize(size);
		
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("SetSize() error setting file size. %d"), err);
			SetError(err);	
			}
		else
			{
			INFO_PRINTF2(_L("The Size of the fail is set to %d bytes"), size);	
			}			
		}
	}	
	
void CT_FileData::DoCmdAttL(const TDesC& aSection)
/** get file attributes */
	{
	
	TUint attValue = 0;
	
	TInt err = iFile->Att(attValue);
	
	PrintFileAttributes(attValue);
	
	if (err != KErrNone)
		{
		ERR_PRINTF2(_L("Att() getting attributes. %d"), err);
		SetError(err);	
		}
	else
		{
		TUint bitmask = 0x000000FF;
		//lets filter out symbian specific bits
		attValue = attValue & bitmask;
			
		TUint attCompare = 0;
		
		//compare the expected att value with the file attributes
		if (ConvertToAttributeL(KUParamCompareValue, aSection, attCompare))
			{
			if (attCompare == attValue)
				{
				INFO_PRINTF1(_L("attributes match"));
				}
			else
				{
				ERR_PRINTF1(_L("Attributes does not match"));
				SetBlockResult(EFail);
				}
			}
				
		}
	
	}
	
void CT_FileData::DoCmdSetAttL(const TDesC& aSection)
/** set file attributes */
	{
	
	TUint setAttMask = 0;
	TBool dataOk = ETrue;
	
	//get the attribute mask to set
	if(!ConvertToAttributeL(KUParamSetAttMask, aSection, setAttMask))
		{
		ERR_PRINTF2(_L("SetATt() error reading parameter. %S"), &KUParamSetAttMask());
		SetBlockResult(EFail);	
		dataOk = EFalse;
		}
		
	TUint clearAttMask = 0;
	
	//get the attribute mask to clear
	if(!ConvertToAttributeL(KUParamClearAttMask, aSection, clearAttMask))
		{
		ERR_PRINTF2(_L("SetAtt() error reading parameter. %S"), &KUParamClearAttMask());
		SetBlockResult(EFail);	
		dataOk = EFalse;
		}
		
	// all ok? let's call SetAtt		
	if (dataOk)
		{
		TInt err = iFile->SetAtt(setAttMask, clearAttMask);
		
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("SetAtt() failed with, err: %d"), err);
			SetError(err);	
			}
		else
			{
			INFO_PRINTF1(_L("The attribute value has been set"));	
			}	
		}
		
	}	
	
	
TBool CT_FileData::ConvertToAttributeL(const TDesC& aParameterName, const TDesC& aSection, TUint& aAttribute)
/** convert attribute string from ini to file attribute bitmask */
	{
	TBool ret = ETrue;
	
	TPtrC aModeStr;
	
	if(GET_OPTIONAL_STRING_PARAMETER(aParameterName, aSection, aModeStr))
		{
		//break the file mode string to array of file modes
		//in case there's more than one filemode		
		RPointerArray<HBufC> fileModes = SplitL(aModeStr, "|");		
			
		for ( int i = 0 ; i < fileModes.Count() ; i++ )
			{
			if (aModeStr == KDEntryAttNormal)
				{
				aAttribute = aAttribute | KEntryAttNormal;
				}
			else if (aModeStr == KDEntryAttReadOnly)
				{
				aAttribute = aAttribute | KEntryAttReadOnly;
				}
			else if (aModeStr == KDEntryAttArchive)
				{
				aAttribute = aAttribute | KEntryAttArchive;
				}
			else if (aModeStr == KDEntryAttHidden)
				{
				aAttribute = aAttribute | KEntryAttHidden;
				}

			else if (aModeStr == KDEntryAttSystem)
				{
				aAttribute = aAttribute | KEntryAttSystem;
				}

			else if (aModeStr == KDEntryAttVolume)
				{
				aAttribute = aAttribute | KEntryAttVolume;
				}

			else if (aModeStr == KDEntryAttDir)
				{
				aAttribute = aAttribute | KEntryAttDir;
				}

			else if (aModeStr == KDEntryAttXIP)
				{
				aAttribute = aAttribute | KEntryAttXIP;
				}

			else if (aModeStr == KDEntryAttRemote)
				{
				aAttribute = aAttribute | KEntryAttRemote;
				}
			else
				{
				ret = EFalse;
				}
			}
			
		for (int j=0 ; j<fileModes.Count() ; j++)
			{
			HBufC* temp = fileModes[j];
			delete temp;
			temp = NULL;
			}
			
		fileModes.Close();
		}
	else
		{
		ret = EFalse;
		}

	return ret;
	
	}	
	
void CT_FileData::DoCmdModifiedL(const TDesC& aSection)
/** get file modification date */
	{
	
	TTime time;
	
	TInt err = iFile->Modified(time);
	
	_LIT(KDateString,"%E%D%X%N%Y %1 %2 %3");	
		
	TBuf<KDefaultDescSize> dateString;
	
	//format the time to string	
	TRAPD(err2, time.FormatL(dateString, KDateString));
	
	if(err2 != KErrNone)
		{
		ERR_PRINTF2(_L("Modified() error formating date string err: %d"), err);
		}
	
	INFO_PRINTF2(_L("Modified() returned: %S"), &dateString);
	
	if (err != KErrNone)
		{
		ERR_PRINTF2(_L("Modified() failed with, err: %d"), err);
		SetError(err);	
		}
	else
		{
		
		//compare the file modification date with the date readed from inin
		TPtrC compValue;
		
		if (GET_OPTIONAL_STRING_PARAMETER(KUParamCompareValue, aSection, compValue))
			{
			
			TTime compTime;
			err = compTime.Set(compValue);
			
			if ( err != KErrNone )
				{
				ERR_PRINTF1(_L("invalid compare value"));
				}
			else
				{
				if (compTime == time)
					{
					INFO_PRINTF1(_L("The dates match"));
					}
				else
					{
					ERR_PRINTF1(_L("The values do not match"));
					SetBlockResult(EFail);
					}
				}
				
			}		
		}		
	}

void CT_FileData::DoCmdSetModifiedL(const TDesC& aSection)
/** set file modification date */
	{
	
	TPtrC dateDesc;
	TBool dataOk = ETrue;
	TTime time;
	TInt err = KErrNone;
	
	if(!GET_MANDATORY_STRING_PARAMETER(KUParamDate(), aSection, dateDesc))
		{
		dataOk = EFalse;
		}
	else
		{
		err = time.Set(dateDesc);
		}
	
	if (err != KErrNone)
		{
		ERR_PRINTF2(_L("time.Set() failed with error code %d"), err);
		SetBlockResult(EFail);
		dataOk = EFalse;
		}
		
	if (dataOk)
		{
		err = iFile->SetModified(time);
		
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("SetModified() failed with error code %d"), err);
			SetError(err);			
			}		
		}
	}
	
void CT_FileData::DoCmdSetL(const TDesC& aSection)
/** set file attributes and modification time */
	{
	TPtrC dateDesc;
	TBool dataOk = ETrue;
	TInt err = KErrNone;
	TTime time;
	
	//get and process the date string
	if(!GET_MANDATORY_STRING_PARAMETER(KUParamDate(), aSection, dateDesc))
		{
		dataOk = EFalse;
		}
	else
		{
		err = time.Set(dateDesc);
		}		
		
	TUint setAttMask = 0;
		
	// get the attribute masks	
	if(!ConvertToAttributeL(KUParamSetAttMask, aSection,setAttMask))
		{
		dataOk = EFalse;
		}	
		
	TUint clearAttMask = 0;	
		
	if(!ConvertToAttributeL(KUParamClearAttMask, aSection, clearAttMask))
		{
		dataOk = EFalse;
		}			
	// if all Ok then proceed with Set	
	if (dataOk)
		{
		err = iFile->Set(time, setAttMask, clearAttMask);
		
		if ( err != KErrNone)
			{
			ERR_PRINTF2(_L("Set() failed with error code %d"), err);
			SetError(err);
			}		
		}
	}
	
void CT_FileData::DoCmdChangeModeL(const TDesC& aSection)
/** Change the file mode */
	{
	TUint fileMode = 0;
	
	if(!GetFileModeL(KFileMode, aSection, fileMode))
		{
		ERR_PRINTF2(_L("Set() error reading parameter. %S"), &KFileMode());
		SetBlockResult(EFail);	
		}
	else
		{
		TInt err = iFile->ChangeMode((TFileMode)fileMode);
		
		if ( err != KErrNone )
			{
			ERR_PRINTF2(_L("ChangeMode() failed with error code %d"), err);
			SetError(err);
			}	
		}
	}	
	
void CT_FileData::DoCmdDriveL(const TDesC& aSection)
/** Get the drive info, in which the file is stored */
	{
	TDriveInfo driveInfo;
	TInt driveNumber;
	int err = iFile->Drive(driveNumber, driveInfo);
	
	INFO_PRINTF2(_L("TDriveInfo.iType    =%d"), driveInfo.iType);
	INFO_PRINTF2(_L("TDriveInfo.iBattery =%d"), driveInfo.iBattery);
	INFO_PRINTF2(_L("TDriveInfo.iDriveAtt=0x%X"), driveInfo.iDriveAtt);
	INFO_PRINTF2(_L("Drivenumber 		 =%d"), driveNumber);	
	
	if (err != KErrNone)
		{
		ERR_PRINTF2(_L("Drive() failed with error code %d"), err);
		SetError(err);
		}
	else
		{
		TInt compDriveNumber;
			
		if ( GET_OPTIONAL_INT_PARAMETER(KUParamCompareValue, aSection, compDriveNumber))
			{
				if ( compDriveNumber == driveNumber )
					{
					INFO_PRINTF3(_L("drivenumbers match %d == %d "), compDriveNumber, driveNumber);
					}
				else 
					{
					ERR_PRINTF3(_L("drivenumbers do not match %d != %d"), compDriveNumber, driveNumber);
					SetBlockResult(EFail);
					}
			}		
		}
	
	}	
	
void CT_FileData::DoCmdDuplicateL(const TDesC& aSection)
/** Duplicate the file */
	{
	TPtrC rFileObjectName;
	TBool dataOk = GET_MANDATORY_STRING_PARAMETER(KFileObjectName, aSection, rFileObjectName);
		
	// get the RFile handle to duplicate	
	RFile* rFileObject = NULL;
	if (dataOk)
		{
		rFileObject=(RFile*)GetDataObjectL(rFileObjectName);
	
		if(rFileObject == NULL)
			{
			ERR_PRINTF2(_L("Duplicate() error getting object. %S"), KFileObjectName);
			SetBlockResult(EFail);	
			dataOk = EFalse;
			}
		}
		
	// if handle ok then procees with duplication	
	if (dataOk)
		{
		TOwnerType ownerType;
			
		TInt err = KErrNone;	
		// determine the owner type to pass to duplicate
		if (ConvertToOwnerType(KUParamOwnerType, aSection, ownerType) && dataOk)
			{
			err = iFile->Duplicate(*rFileObject, ownerType);
			}
		else
			{
			err = iFile->Duplicate(*rFileObject);
			}
			
		
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("Duplicate() error duplicating %d"), err);
			SetError(err);
			}
		else
			{	
			//lets mark the file to be opened
			iFileOpened = ETrue;		
			}
		}
	
	}	
	
TBool CT_FileData::ConvertToOwnerType(const TDesC& aParameterName, const TDesC& aSection, TOwnerType& aResult)
/** convert the ownerType string from ini to EOwnerType */
	{
	
	TBool ret = ETrue;
	
	TPtrC ownerTypeString;
	
	if(GET_OPTIONAL_STRING_PARAMETER(aParameterName, aSection, ownerTypeString))
		{
		
		if (ownerTypeString == KEOwnerProcess )
			{
			aResult = EOwnerProcess;
			}
		else if (ownerTypeString == KEOwnerThread)
			{
			aResult = EOwnerThread;
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

	return ret;
	}	
	
void CT_FileData::DoCmdFullName(const TDesC& aSection)
/** get the file fullname */
	{
	TBuf<128> name;
	TInt err = iFile->FullName(name);
	
	INFO_PRINTF2(_L("FullName() returned  %S"), &name);		
	
	if(err != KErrNone)
		{
		ERR_PRINTF2(_L("FullName() error, returned. %d"), err);
		SetError(err);	
		}
	else
		{
		TPtrC path;	
		if(GET_OPTIONAL_STRING_PARAMETER(KUParamComparePath, aSection, path))
			{
			if (path == name)
				{
				INFO_PRINTF1(_L("the paths match"));	
				}
			else
				{
				ERR_PRINTF1(_L("the paths do not match"));
				SetBlockResult(EFail);
				}
			}				
		}
	}	
	
void CT_FileData::DoCmdNameL(const TDesC& aSection)
/** get the file name */
	{
	TBuf<KDefaultDescSize> name;
	TInt err = iFile->Name(name);
	INFO_PRINTF2(_L("Name() returned  %S"), &name);	
	
	if(err != KErrNone)
		{
		ERR_PRINTF2(_L("Name() error, returned. %d"), err);
		SetError(err);	
		}
	else	
		{
		
				
		TPtrC compValue;	
			
		if (GET_OPTIONAL_STRING_PARAMETER(KUParamCompareValue, aSection, compValue))
			{
			if (compValue == name)
				{
				INFO_PRINTF3(_L("The names match %S == %S"), &name, &compValue);
				}
			else
				{
				ERR_PRINTF3(_L("The names do not match %S == %S"), &name, &compValue);			
				SetBlockResult(EFail);
				}
			}		
		}
	}	

TBool CT_FileData::ConvertToBlockMapUsage(const TDesC& aParameterName, const TDesC& aSection, TBlockMapUsage& aResult)
	{
	
	TBool ret = ETrue;
	
	TPtrC blockmapeUsageString;
	
	if(GET_OPTIONAL_STRING_PARAMETER(aParameterName, aSection, blockmapeUsageString))
		{
		
		if (blockmapeUsageString == KEBlockMapUsagePaging )
			{
			aResult = EBlockMapUsagePaging;
			}
		else if (blockmapeUsageString == KETestDebug)
			{
			aResult = ETestDebug;
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

	return ret;
	}	
	
void CT_FileData::DoCmdBlockMap(const TDesC& aSection)
	{
	TInt64 startPos = 0;
	GET_OPTIONAL_INT64_PARAMETER(KUParamStartPos, aSection, startPos);
		
	SBlockMapInfo blockMapInfo;
	TInt err = KErrNone;
	TInt64 endPos = -1;
	if (GET_OPTIONAL_INT64_PARAMETER(KUParamEndPos, aSection, endPos))
		{
		TBlockMapUsage usage;
		if (ConvertToBlockMapUsage(KUParamUsage, aSection, usage))
			{
			err = iFile->BlockMap(blockMapInfo, startPos, endPos, usage);
			}
		else
			{
			err = iFile->BlockMap(blockMapInfo, startPos, endPos);
			}
		}
	else
		{
		TBlockMapUsage usage;
		if (ConvertToBlockMapUsage(KUParamUsage, aSection, usage))
			{
			err = iFile->BlockMap(blockMapInfo, startPos, usage);
			}
		else
			{		
			err = iFile->BlockMap(blockMapInfo, startPos);
			}
		}
		
	if (KErrNone != err)
		{
		ERR_PRINTF2(_L("BlockMap Returned an error %d"), err);
		SetError(err);
		}
	if (KErrNone == err || KErrCompletion == err)
		{
		INFO_PRINTF2(_L("Returned start position %d"), startPos);

		INFO_PRINTF2(_L("Local drive number %d"), blockMapInfo.iLocalDriveNumber);
		INFO_PRINTF2(_L("Block start address %d"), blockMapInfo.iStartBlockAddress);
		INFO_PRINTF2(_L("Block start offset %u"), blockMapInfo.iBlockStartOffset);
		INFO_PRINTF2(_L("Block graduality %u"), blockMapInfo.iBlockGranularity);	
		}
	
	}

//function that adds the buffer to the end of the array
void CT_FileData::ToArrayL(RPointerArray<HBufC>& aArray, HBufC*& aBuffer)
	{
	HBufC* arrayElement = HBufC::NewL(aBuffer->Length());
	TPtr arrayElementPtr = arrayElement->Des();
	arrayElementPtr.Copy(*aBuffer);
	arrayElementPtr.TrimAll();
	aArray.Append(arrayElement);
	arrayElement = NULL;
	}
	

// helper function to split filemodes or attributes to array
RPointerArray<HBufC> CT_FileData::SplitL(const TDesC& aInput, const char* aToken)
	{
	RPointerArray<HBufC> fileModeArray;
	HBufC*  buffer = HBufC::NewL(aInput.Length());
	TPtr bufferPtr = buffer->Des();
	for (int i=0;i<aInput.Length();i++)
		{
		if (aInput[i] == *aToken)
			{
			ToArrayL(fileModeArray, buffer);
			delete buffer;	
				
			buffer = NULL;
			buffer = HBufC::NewL( aInput.Length() - i);	
			bufferPtr = buffer->Des();
			}
		else
			{
			bufferPtr.Append(aInput[i]);
			}
		}
	
	ToArrayL(fileModeArray, buffer);		
	delete buffer;
	
	buffer = NULL;		
		
	return fileModeArray;
 	
	}
	
void CT_FileData::DoCancel(CActive* aActive, TInt aIndex)
	{
	TBool	foundActiveObject = EFalse;

	TInt	index=0;
	TInt	count=0;

	count=iReadCallbackArray.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iReadCallbackArray[index] )
			{
			INFO_PRINTF1(_L("DoCancel iReadCallbackArray called"));
			foundActiveObject = ETrue;
			iReadCallbackArray.Remove(index);
	 		}
		}

	count=iWriteCallbackArray.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iWriteCallbackArray[index] )
			{
			INFO_PRINTF1(_L("DoCancel iWriteCallbackArray called"));
			foundActiveObject = ETrue;
			iWriteCallbackArray.Remove(index);
	 		}
		}

	// See if it is in iFlushCallbackArray
	count=iFlushCallbackArray.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iFlushCallbackArray[index] )
			{
			INFO_PRINTF1(_L("DoCancel iFlushCallbackArray called"));
			foundActiveObject = ETrue;
			iFlushCallbackArray.Remove(index);
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

void CT_FileData::RunL(CActive* aActive, TInt aIndex)
	{

	
	TBool	foundActiveObject = EFalse;
	TInt	index=0;
	TInt	count=0;
	
	count=iReadCallbackArray.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iReadCallbackArray[index] )
			{
			foundActiveObject = ETrue;
			TInt	err = aActive->iStatus.Int();
			
			TBuf<KDefaultDescSize> info;
			info.Copy(iReadCallbackArray[index]->iFileData->Des());
			INFO_PRINTF2(_L("readed \"%S\" from file"), &info);
			
			HandleExpectedString(iReadCallbackArray[index]->iFileData->Des(), *iReadCallbackArray[index]->iSection);
			
			if( err != KErrNone )
				{
				ERR_PRINTF2(_L("Async Read error %d"), err);
				SetAsyncError( aIndex, err );
				}
			else
				{
				INFO_PRINTF1(_L("Succesfully completed Async Read"));
				}
				
			iReadCallbackArray.Remove(index);	
			delete aActive;
			
	 		}
		}
		
	count=iWriteCallbackArray.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iWriteCallbackArray[index] )
			{
			foundActiveObject = ETrue;
			TInt	err = aActive->iStatus.Int();
			
			if( err != KErrNone )
				{
				ERR_PRINTF2(_L("Async Write Error %d"), err);
				SetAsyncError( aIndex, err );
				}				
			else
				{
				INFO_PRINTF1(_L("Succesfully completed Async Write"));
				}
			iWriteCallbackArray.Remove(index);				
			delete aActive;			
	 		}
		}	
		
	count=iFlushCallbackArray.Count();
	for( index=0; (index<count) && (!foundActiveObject); ++index )
 		{
 		if( aActive == iFlushCallbackArray[index] )
			{
			foundActiveObject = ETrue;
			TInt	err = aActive->iStatus.Int();
			
			if( err != KErrNone )
				{
				ERR_PRINTF2(_L("Async Flush Error %d"), err);
				SetAsyncError( aIndex, err );
				}				
			iFlushCallbackArray.Remove(index);				
			delete aActive;			
	 		}
		}				
	
	DecOutstanding();
	}

void CT_FileData::PrintFileAttributes(TUint aAttValue)
/** Prints file attributes */
	{
	if ((aAttValue & KEntryAttNormal) == KEntryAttNormal)
		{
		INFO_PRINTF1(_L("KEntryAttNormal"));
		}
	if ((aAttValue & KEntryAttReadOnly) == KEntryAttReadOnly)
		{
		INFO_PRINTF1(_L("KEntryAttReadOnly"));
		}
		
	if ((aAttValue & KEntryAttHidden) == KEntryAttHidden)
		{
		INFO_PRINTF1(_L("KEntryAttHidden"));
		}

	if ((aAttValue & KEntryAttSystem) == KEntryAttSystem)
		{
		INFO_PRINTF1(_L("KEntryAttSystem"));
		}

	if ((aAttValue & KEntryAttVolume) == KEntryAttVolume)
		{
		INFO_PRINTF1(_L("KEntryAttVolume"));
		}

	if ((aAttValue & KEntryAttDir) == KEntryAttDir)
		{
		INFO_PRINTF1(_L("KEntryAttDir"));
		}

	if ((aAttValue & KEntryAttArchive) == KEntryAttArchive)
		{
		INFO_PRINTF1(_L("KEntryAttArchive"));
		}

	if ((aAttValue & KEntryAttXIP) == KEntryAttXIP)
		{
		INFO_PRINTF1(_L("KEntryAttXIP"));
		}

	if ((aAttValue & KEntryAttRemote) == KEntryAttRemote)
		{
		INFO_PRINTF1(_L("KEntryAttRemote"));
		}	
	}

