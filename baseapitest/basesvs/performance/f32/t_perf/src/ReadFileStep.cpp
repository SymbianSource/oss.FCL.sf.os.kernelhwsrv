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


#include "ReadFileStep.h"


/*@{*/
// Literals Used
_LIT(KT_ReadSeek,	"readSeek");
/*@}*/


// Function : CT_ReadFileStep()
// Description :CT_ReadFileStep() class constructor
CT_ReadFileStep::CT_ReadFileStep()
:	CT_SeekFileStep()
,	iSeek(EFalse)
	{
	SetTestStepName(KT_ReadFileStep);
	}


// Function : ~CT_ReadFileStep()
// Description :CT_ReadFileStep() class destructor
CT_ReadFileStep::~CT_ReadFileStep()
	{
	}


// Function : doTestStepPreambleL()
// Description :
// @return :TVerdict EPass/EFail
TVerdict CT_ReadFileStep::doTestStepPreambleL()
	{
	//call Seek base class doTestStepPreambleL
	TVerdict	ret=CT_SeekFileStep::doTestStepPreambleL();

	//Set up seekread
	if (!GetBoolFromConfig(ConfigSection(),KT_ReadSeek,iSeek))
		{
		//defaulting to constructor value
		WARN_PRINTF2(_L("Using default seek value: (%d)"), iSeek);
		}

	return ret;
	}




// Function : ThreadFuncL()
// Description:Thread for performance read test(s)
// @return :TInt
TInt CT_ReadFileStep::ThreadFuncL(RFs& /*aSession*/)
	{
 	RFile	file;	//	File that all operations are acted upon
	//set up for either testcase
	TInt	result=KErrNone;
	HBufC8*	data=HBufC8::NewLC(iBlockSize);
	TPtr8	buf(data->Des());
	buf.SetLength(iBlockSize);
	//test scenarios
	if (iSeek)//perform seek read operation
		{
		TTime	startTime;
		TTime	endTime;
		TInt	pos=0;
		TInt	i=0;
		TInt	sizebuf=iFileSize-iBlockSize;
		//start timer
		startTime.UniversalTime();
		for(i=0; (i<iFuncCalls) && (result==KErrNone); i++)
			{
			pos=i%2?sizebuf-i/2:i/2;
			file=iFileArray[i%iNumOfFiles];
			result=file.Read(pos,buf,iBlockSize);
			}
		//end timer
		endTime.UniversalTime();

		//calculate extra time taken for pos calc (file offset calc)
		TInt	calls=i;
		TInt	res=KErrNone;
		TTime 	startPos;	// Start pos timer
		TTime	endPos;	//End Pos timer
		startPos.UniversalTime();
		for(TInt j=0; (j<calls) && (res==KErrNone); j++)
			{
			pos=j%2?sizebuf-j/2:j/2;
			file=iFileArray[j%iNumOfFiles];
			}
		endPos.UniversalTime();
		endTime=endTime-(endPos.MicroSecondsFrom(startPos));
		iTotalTime=endTime.MicroSecondsFrom(startTime);//Store time-taken
		}
	else //perform read operation only
		{
		TInt	i=0;
		TInt	pos=0;
		TInt	sizebuf=iFileSize-iBlockSize;
		TTime	startTime;
		TTime	endTime;	//End timer
		startTime.UniversalTime();	// Start timer
		for(i=0; (i<iFuncCalls) && (result==KErrNone); i++)
			{
			pos=i%2?sizebuf-i/2:i/2;
			file=iFileArray[i%iNumOfFiles];
			User::LeaveIfError(file.Seek(iSeekMode,pos));
			result=(file.Read(buf,iBlockSize));
			}
		endTime.UniversalTime();

		//calculate extra time taken during seek file and remove
		TInt	calls=i;
		TInt	res=KErrNone;
		TTime	startSeek;	// Start seek timer
		TTime 	endSeek;	//End Seek timer
		startSeek.UniversalTime();
		for(TInt j=0; (j<calls) && (res==KErrNone); j++)
			{
			pos=j%2?sizebuf-j/2:j/2;
			file=iFileArray[j%iNumOfFiles];
			User::LeaveIfError(file.Seek(iSeekMode,pos));
			}
		endSeek.UniversalTime();
		endTime=endTime-(endSeek.MicroSecondsFrom(startSeek));
		iTotalTime=endTime.MicroSecondsFrom(startTime);	//Store time-taken
		}
	//End Test Scenarios
	CleanupStack::PopAndDestroy(1, data);// data
	return result;
 	}
