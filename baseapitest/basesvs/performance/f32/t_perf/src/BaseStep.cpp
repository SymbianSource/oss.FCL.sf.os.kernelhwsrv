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


#include "BaseStep.h"

const TInt	KMaxString=1000;
const TInt	KOneMB=1048576;
const TReal	KMicroSecondsPerSecond=1000000.0;

/*@{*/
// Literals Used
_LIT(KT_NumOfFunctionCall,				"functionCalls");
_LIT(KT_ThreadName,						"threadname");
_LIT(KT_StackSize,						"stacksize");
_LIT(KT_MinHeapSize,					"minheapsize");
_LIT(KT_MaxHeapSize,					"maxheapsize");
_LIT(KT_OwnerType,						"ownertype");
_LIT(KT_ThreadPriority,					"threadpriority");
_LIT(KT_ThreadDefaultName,				"thread");
_LIT(KT_EOwnerProcess,					"EOwnerProcess");
_LIT(KT_EOwnerThread,					"EOwnerThread");
_LIT(KT_EPriorityLess,					"EPriorityLess");
_LIT(KT_EPriorityNormal,				"EPriorityNormal");
_LIT(KT_EPriorityMore,					"EPriorityMore");
_LIT(KT_EPriorityRealTime,				"EPriorityRealTime");
_LIT(KT_EPriorityAbsoluteForeground,	"EPriorityAbsoluteForeground");
_LIT(KT_EPriorityAbsoluteBackground,	"EPriorityAbsoluteBackground");
_LIT(KT_ResultTitle,					"-------------RESULTS-------------");
_LIT(KT_BufResultFuncCalls,				"Number of function calls is:             (%d)");
_LIT(KT_BufResultBlockSize,				"Block size:                              (%d) bytes");
_LIT(KT_BufResultBytesProcessed,		"Bytes processed:                         (%d)");
_LIT(KT_BufResultTotalTime,				"Time taken for all function calls:       (%Ld) microseconds");
_LIT(KT_BufResultAverageTime,			"Approximate Average Time Taken per call: (%Ld) microseconds");
_LIT(KT_BufResultThroughput,			"Throughput:                              (%f) MB/sec");
_LIT(KT_ResultEnd,						"-----------END-RESULTS-----------");
_LIT(KT_NumOfFiles,						"numOfFiles");
_LIT(KT_BaseFileName,					"fileBaseName");
_LIT(KT_SubDirName,						"subDirName");
_LIT(KT_DirTreeDepth,					"dirTreeDepth");
_LIT(KT_DotSeparator,					".");
_LIT(KT_DirSpearator,					"\\");
_LIT(KT_BaseDirName,					"baseDirName");
_LIT(KT_FileSize,						"fileSize");
_LIT(KT_BlockSize,						"blockSize");
_LIT(KT_CsvName,						"csvFileName");
_LIT(KT_CsvDefaultName,					"\\f32-perf.csv");
_LIT(KT_CsvTitle,						"Operation,Calls,Time taken, Average time \n");
_LIT(KT_NewLine,						"\n");
_LIT(KT_Comma,							",");
_LIT(KT_Operation,						"operation");
/*@}*/



// Function : CT_F32BaseStep()
// Description :CT_F32BaseStep class constructor
CT_F32BaseStep::CT_F32BaseStep(TBool aOpenFiles)
:	CTestStepV2()
,	iFuncCalls(0)
,	iNumOfFiles(0)
,	iDirTreeDepth(0)
,	iFileSize(0)
,	iBlockSize(0)
,	iTotalTime(0)
,	iOpenFiles(aOpenFiles)
	{
	}


// Function : ~CT_F32BaseStep()
// Description :CT_F32BaseStep class destructor
CT_F32BaseStep:: ~CT_F32BaseStep()
	{
	iFilePathArray.Reset();
	iFilePathArray.Close();
	iFileArray.Reset();
	iFileArray.Close();
	}


//Function: doTestStepPreambleL
//Description :
//@return TVerdict pass / fail
TVerdict CT_F32BaseStep::doTestStepPreambleL()
	{
	INFO_PRINTF1(_L("Test Step : PerfBaseStep Preamble setup"));
	TVerdict  result=CTestStepV2::doTestStepPreambleL();

	if (!GetStringFromConfig(ConfigSection(),KT_CsvName,iCsvFileName))
		{
		iCsvFileName.Set(KT_CsvDefaultName);
		WARN_PRINTF1(_L(".csv file name not set reverting to default"));
		}
	if (!GetIntFromConfig(ConfigSection(),	KT_NumOfFunctionCall,iFuncCalls))
		{
		ERR_PRINTF2( _L("Unable to read number of function calls, it is set to: (%d) "),iFuncCalls );
		SetTestStepResult(EFail);
		}
	if (!GetIntFromConfig(ConfigSection(),	KT_NumOfFiles,iNumOfFiles))
		{
		ERR_PRINTF2( _L("Unable to read number of files created (%d)"), iNumOfFiles);
		SetTestStepResult(EFail);
		}
	if(!GetStringFromConfig(ConfigSection(),KT_BaseFileName,iFileBaseName))
		{
		ERR_PRINTF1( _L("Unable to read base file name ") );
		SetTestStepResult(EFail);
		}
	if (!GetStringFromConfig(ConfigSection(),KT_SubDirName,iDirSubName))
		{
		ERR_PRINTF1( _L("Unable to read in sub directory name.") );
		SetTestStepResult(EFail);
		}
	if (!GetIntFromConfig(ConfigSection(),KT_DirTreeDepth,iDirTreeDepth))
		{
		ERR_PRINTF2( _L("Invalid directory tree depth given, it is set to:...(%d)"),iDirTreeDepth);
		SetTestStepResult(EFail);
		}
	if (!GetStringFromConfig(ConfigSection(),KT_BaseDirName,iDirBaseName))
		{
		ERR_PRINTF1( _L("Unable to read in parent directory name to use within test, Will not be able to create a directory structure! ") );
		SetTestStepResult(EFail);
		}
	if (!GetIntFromConfig(ConfigSection(),KT_FileSize,iFileSize))
		{
		ERR_PRINTF2( _L("Invalid fileSize given, file size set to: (%d)"), iFileSize );
		SetTestStepResult(EFail);
		};
	if (!GetStringFromConfig(ConfigSection(),KT_Operation,iOperation))
		{
		ERR_PRINTF1( _L("Unable to read in operation name! ") );
		SetTestStepResult(EFail);
		}
	if(iOpenFiles)//if we are not an entry test
		{
		if (!GetIntFromConfig(ConfigSection(),KT_BlockSize,iBlockSize))
			{
			WARN_PRINTF2( _L("Invalid Integer blocksize given, it is set to:...(%d)"), iBlockSize);
			}
		}
	SetFilePathArrayL();	//fill iFilePathArray
	RFile	file;	//	fill in RFileArray
	for(TInt i=0; i<iDirTreeDepth*iNumOfFiles;i++)
		{
		iFileArray.AppendL(file);
		}

	return result;
	}


//Function: doTestStepPostambleL
//Description :
//@return TVerdict pass / fail
TVerdict CT_F32BaseStep::doTestStepPostambleL()
	{
	PrintResults();	//print out results
	CreateCsvFileL();
	return TestStepResult();
	}


//Function: doTestStepL
//Description :
//@return TVerdict pass / fail
TVerdict CT_F32BaseStep::doTestStepL()
	{
	//read in params from ini file
	TPtrC	threadname(KT_ThreadDefaultName);
 	GetStringFromConfig(ConfigSection(),KT_ThreadName,threadname);

	TInt	stackSize=KDefaultStackSize;
	GetHexFromConfig(ConfigSection(),KT_StackSize,stackSize);

	TInt	minHeapSize=KMinHeapSize;
	GetHexFromConfig(ConfigSection(),KT_MinHeapSize,minHeapSize);

	TInt	maxHeapSize=KMinHeapSize;
	GetHexFromConfig(ConfigSection(),KT_MaxHeapSize,maxHeapSize);

	TPtrC	owner(KT_EOwnerProcess);
	GetStringFromConfig(ConfigSection(),KT_OwnerType,owner);
	TOwnerType	ownerType;
	if ( !SetOwnerType(owner, ownerType) )
		{
 		ERR_PRINTF2(_L("TOwnerType invalid (%S)"), &owner);
 		SetTestStepResult(EFail);
		}

	TPtrC	priority(KT_EPriorityLess);
	GetStringFromConfig(ConfigSection(),KT_ThreadPriority,priority);
	TThreadPriority	threadPriority;
	if ( !SetThreadPriority(priority, threadPriority) )
		{
 		ERR_PRINTF2(_L("TThreadPriority invalid (%S)"), &priority);
 		SetTestStepResult(EFail);
		}

	TRequestStatus	status;
	RThread			thread;
	TRAPD(err, thread.Create(threadname,ThreadFunction,stackSize,minHeapSize,maxHeapSize, static_cast<CT_F32BaseStep*>(this), ownerType));

	if (err==KErrNone)
		{
		CleanupClosePushL(thread);
		thread.SetPriority(threadPriority);
		thread.Logon(status);
		thread.Resume();
		User::WaitForRequest(status);
		thread.Close();
		CleanupStack::PopAndDestroy(1,&thread);
		}
	else
		{
		ERR_PRINTF2(_L("Cannot create thread. Error %d"), err);
		SetTestStepResult(EFail);
		}

	return TestStepResult();
	}



//Function:SetOwnerType
//Description:used in doTestStepL
// @return :TOwnerType
// @param:const TDesC& aName
TBool CT_F32BaseStep::SetOwnerType(const TDesC& aName, TOwnerType& aOwnerType)
	{
	TBool	ret=ETrue;

	if (aName==KT_EOwnerProcess)
 		{
 		aOwnerType=EOwnerProcess;
 		}
 	else if (aName==KT_EOwnerThread)
 		{
 		aOwnerType=EOwnerThread;
 		}
 	else
 		{
		ret=EFalse;
 		}

 	return ret;
	}

//Function:SetThreadPriority
//Description : used in doTestStepL
// @return :TThreadPriority
// @param:const TDesC& aName
TBool CT_F32BaseStep::SetThreadPriority(const TDesC& aName, TThreadPriority& aThreadPriority)
	{
	TBool	ret=ETrue;
	if(aName==KT_EPriorityLess)
 		{
 		aThreadPriority=EPriorityLess;
 		}
 	else if(aName==KT_EPriorityNormal)
 		{
 		aThreadPriority=EPriorityNormal;
 		}
 	else if(aName==KT_EPriorityMore)
 		{
 		aThreadPriority=EPriorityMore;
 		}
	else if(aName==KT_EPriorityRealTime)
 		{
 		aThreadPriority=EPriorityRealTime;
 		}
 	else if(aName==KT_EPriorityAbsoluteForeground)
 		{
 		aThreadPriority=EPriorityAbsoluteForeground;
 		}
 	else if(aName==KT_EPriorityAbsoluteBackground)
 		{
 		aThreadPriority=EPriorityAbsoluteBackground;
 		}
 	else
 		{
		ret=EFalse;
 		}

 	return ret;
	}


//Function:ThreadPostFuncL
//Description : post thread operation
void CT_F32BaseStep::ThreadPostFuncL(RFs& aSession)
	{
	//open rfile objects
	for(TInt i=0; i<iFileArray.Count();i++)
		{
		iFileArray[i].Close();
		}

	aSession.Close();
	}


//Function:ThreadPreFuncL
//Description :pre thread operation
//@return TInt KErrNone/error value
TInt CT_F32BaseStep::ThreadPreFuncL(RFs& aSession)
	{
	TInt	result=aSession.Connect();

	if(iOpenFiles)
		{
		//open rfile objects
		for(TInt i=0; (i<iFileArray.Count()) && (result==KErrNone);i++)
			{
			result=iFileArray[i].Open(aSession,iFilePathArray[i],EFileWrite);
			}
		}

	return result;
	}


//Function:PrintResults
//Description : Print results to TEF log  file
void CT_F32BaseStep::PrintResults()
	{
	const TInt64	averageTime=((iTotalTime.Int64())/ (TInt64)iFuncCalls);

	INFO_PRINTF1(KT_ResultTitle);
	INFO_PRINTF2(KT_BufResultFuncCalls,			iFuncCalls);
	INFO_PRINTF2(KT_BufResultTotalTime,			iTotalTime.Int64());
	INFO_PRINTF2(KT_BufResultAverageTime,		averageTime);
	if(iOpenFiles)//if we are not an entry test
		{
		INFO_PRINTF2(KT_BufResultBlockSize,			iBlockSize);
		INFO_PRINTF2(KT_BufResultBytesProcessed,	iFuncCalls*iBlockSize);
		INFO_PRINTF2(KT_BufResultThroughput,		KMicroSecondsPerSecond*iFuncCalls*iBlockSize/iTotalTime.Int64()/KOneMB);
		}
	
	INFO_PRINTF1(KT_ResultEnd);
	}


//Function:SetFilePathArrayL
//Description : Adds all file paths to array
void CT_F32BaseStep::SetFilePathArrayL()
	{
	//place all filepaths into an array
	TFileName 	extNum=_L("");//placeholder
	TFileName 	filename=iFileBaseName;
	TFileName 	pathDepth=iDirBaseName;//parent base directory name

  	for( TInt i=0; i<iDirTreeDepth; i++)
		{
		pathDepth+=iDirSubName;
 		pathDepth+=KT_DirSpearator;
 		for (TInt j=0; j<iNumOfFiles;j++)
 			{
 			extNum.Zero();
 			extNum.AppendNum(j+1);
			filename.Insert((filename.Find(KT_DotSeparator)),extNum);
			filename.Insert(0,pathDepth);
			iFilePathArray.AppendL(filename);
			filename.Delete(0,((pathDepth.Length())));
			filename.Delete((filename.Find(KT_DotSeparator)-1),extNum.Length());
			}

		}
	}



// Function CreateCsvFileL
// Description
// @return :void
// @param:
void CT_F32BaseStep::CreateCsvFileL()
	{
	RFs		session;
	User::LeaveIfError(session.Connect());
	CleanupClosePushL(session);

 	RFile 	file;
	if ( file.Open(session, iCsvFileName, EFileShareAny | EFileWrite)!=KErrNone )
		{
		User::LeaveIfError(file.Create(session, iCsvFileName, EFileShareAny | EFileWrite));
		User::LeaveIfError(file.Open(session, iCsvFileName, EFileShareAny | EFileWrite));
		INFO_PRINTF1(_L("Csv file successfully created "));
		iPtr.Set(KT_CsvTitle);
		AppendDataL(file, iPtr);
		}
	else
		{
		INFO_PRINTF1( _L("CSV File not created as already found!"));
		}
	CleanupClosePushL(file);

	TInt 	pos=0;
	User::LeaveIfError(file.Seek(ESeekEnd, pos));

	//add results
	TBuf<KMaxString>	buffer=_L("");
	buffer.Append(iOperation);//name of operation
	buffer.Append(KT_Comma);

	buffer.AppendNum(iFuncCalls);//the number of calls
	buffer.Append(KT_Comma);

	buffer.AppendNum(iTotalTime.Int64());//the time taken
	buffer.Append(KT_Comma);

	buffer.AppendNum((iTotalTime.Int64()/ (TInt64)iFuncCalls));//the average time
	buffer.Append(KT_NewLine);

	iPtr.Set(buffer);
	TInt 	appendData=AppendDataL(file,iPtr);
	if (appendData!=KErrNone)
		{
		ERR_PRINTF2( _L("Adding Data to .Csv file failed errorcode (%d) "), appendData );
		SetTestStepResult(EFail);
		}

	CleanupStack::PopAndDestroy(2, &session);
	}

// Function AppendDataL
// Description
// @return :TInt
// @param:RFile& aFile,TDesC& aString
TInt CT_F32BaseStep::AppendDataL( RFile& aFile,TDesC& aString)
	{
	TInt 	err=KErrNone;
	HBufC8* tempbuf=HBufC8::NewLC(aString.Length());
  	TPtr8 	data(tempbuf->Des());

  	data.Copy(aString);
  	err=aFile.Write(data);
  	CleanupStack::PopAndDestroy(1,tempbuf);
	return err;
	}
