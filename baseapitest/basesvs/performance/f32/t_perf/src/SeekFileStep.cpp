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


#include "SeekFileStep.h"

/*@{*/
// Literals Used
_LIT(KT_SeekMode,						"seekMode");
_LIT(KT_SeekStart,						"ESeekStart");
_LIT(KT_SeekCurrent,					"ESeekCurrent");
_LIT(KT_SeekEnd,						"ESeekEnd ");
/*@}*/




// Function : CT_SeekFileStep
// Description :CT_SeekFileStep class constructor
CT_SeekFileStep ::CT_SeekFileStep ()
:	CT_F32BaseStep(ETrue)
,	iSeekMode(ESeekStart)//default
	{
	SetTestStepName(KT_SeekFileStep);
	}




// Function : ~CT_SeekFileStep
// Description :CT_SeekFileStep class destructor
CT_SeekFileStep ::~CT_SeekFileStep ()
	{
	}


// Function : doTestStepPreambleL()
// Description :
// @return :TVerdict EPass/EFail
TVerdict CT_SeekFileStep ::doTestStepPreambleL()
	{
	//call base class doTestStepPreambleL
	TVerdict 	result=CT_F32BaseStep::doTestStepPreambleL();

	TPtrC 	seekmode;
	if (!GetStringFromConfig(ConfigSection(),KT_SeekMode,seekmode))
		{
		SetSeekMode(seekmode);	//set data:seekmode
		WARN_PRINTF1( _L("Corrupt seekmode will revert to default:ESeekStart"));
		}	
	return result;
	}



// Function : SetSeekMode
// Description :
//@param :TDesC& aSeekmode
void CT_SeekFileStep::SetSeekMode(TDesC& aSeekmode)
	{
	if (aSeekmode==KT_SeekStart)
		{
		iSeekMode=ESeekStart;
		}
	else if (aSeekmode==KT_SeekCurrent)
		{
		iSeekMode=ESeekCurrent;
		}
	else if (aSeekmode==KT_SeekEnd)
		{
		iSeekMode=ESeekCurrent;
		}
	else
		{
		iSeekMode=ESeekStart;//default
		}
	}


// Function : ThreadFunc
// Description :Thread call back that seeks positions in files
// @return :TInt
TInt CT_SeekFileStep::ThreadFuncL(RFs& /*aSession*/)
	{
 	RFile	file;	//	File that all operations are acted upon
 	//set up
	TInt	result=KErrNone;
	HBufC8*	data=HBufC8::NewLC(iBlockSize);
  	TPtr8	buf(data->Des());
  	buf.SetLength(iBlockSize);

  	TInt 	i=0;
	TInt 	pos=0;
	TInt 	sizebuf=iFileSize-iBlockSize;
	TTime 	startTime;
	TTime 	endTime;	//End timer
	startTime.UniversalTime();		// Start timer
	for(i=0; (i<iFuncCalls) && (result==KErrNone);i++)
		{
		pos=i%2?sizebuf-i/2:i/2;
		file=iFileArray[i%iNumOfFiles];
		result=(file.Seek(iSeekMode,pos));
		}
	endTime.UniversalTime();

	//calculate extra time taken for pos calc (file offset calc)
	TInt	calls=i;
	TInt	res=KErrNone;
	TTime 	startPos;	// Start pos timer
	TTime 	endPos;	//End Pos timer
	startPos.UniversalTime();
	for(TInt j=0; (j<calls) && (res==KErrNone); j++)
		{
		pos=j%2?sizebuf-j/2:j/2;
		file=iFileArray[j%iNumOfFiles];
		}
	endPos.UniversalTime();
	endTime=endTime-(endPos.MicroSecondsFrom(startPos));
	iTotalTime=endTime.MicroSecondsFrom(startTime);	//Store time-taken

	CleanupStack::PopAndDestroy(1, data);// data
	return result;
 	}
