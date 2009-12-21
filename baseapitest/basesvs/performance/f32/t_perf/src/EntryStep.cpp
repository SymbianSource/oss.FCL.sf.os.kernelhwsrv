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


#include "EntryStep.h"


/*@{*/
// Literals Used
_LIT(KT_DirSpearator,	"\\");
/*@}*/


// Function : CT_EntryStep()
// Description :CT_EntryStep class constructor
CT_EntryStep::CT_EntryStep()
:	CT_F32BaseStep(EFalse)
	{
	SetTestStepName(KT_EntryStep);
	}


// Function : ~CT_EntryStep()
// Description :CT_EntryStep class destructor
CT_EntryStep::~CT_EntryStep()
	{
	iFilePathArray.Reset();
	iFilePathArray.Close();
	}


// Function : doTestStepPreambleL()
// Description :
// @return :TVerdict EPass/EFail
TVerdict CT_EntryStep::doTestStepPreambleL()
	{
	//call base class doTestStepPreambleL
	TVerdict	result=CT_F32BaseStep::doTestStepPreambleL();	
	//fill fulldir array with all paths
	TFileName	pathDepth=iDirBaseName;//parent base directory name
	TInt 		index=0;
  	for(TInt i=0;i<iDirTreeDepth; i++)
  		{
  		pathDepth+=iDirSubName;
 		pathDepth+=KT_DirSpearator;
		iFilePathArray.InsertL(pathDepth,index);//dir add me
  		for(TInt j=0; j<iNumOfFiles; j++)
			{
			index++;
			}
		index++;
  		}
	return result;
	}


// Function : ThreadFuncL()
// @return :TInt
// Description:Thread for performance entry test
TInt CT_EntryStep::ThreadFuncL(RFs& aSession)
	{
	//Preparation
	TInt 	result=KErrNone;
	TInt 	contents=iNumOfFiles*iDirTreeDepth+iDirTreeDepth;
	TEntry 	entry;
	// Start timer
	TTime 	startTime;
	TTime 	endTime;
	startTime.UniversalTime();
	for(TInt i=0; (i<iFuncCalls) && (result==KErrNone); i++)
		{
		result=(aSession.Entry((iFilePathArray[i%contents]),entry));
		}
	endTime.UniversalTime();
	iTotalTime=endTime.MicroSecondsFrom(startTime);	//Store time-taken
	return result;
	}
