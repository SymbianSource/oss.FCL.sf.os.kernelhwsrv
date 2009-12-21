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


#include "UtilsStep.h"

/*@{*/
// Literals Used
_LIT(KT_FormatDrive,	"formatDrive");
_LIT(KT_BaseDirName,	"baseDirName");
/*@}*/

// Function : CT_UtilsStep
// Description :CT_UtilsStep:: class constructor
CT_UtilsStep::CT_UtilsStep()
:	CTestStepV2()
,	iFormatDrive(EFalse)
	{
	}


// Function : ~CT_UtilsStep
// Description :CT_UtilsStep class destructor
CT_UtilsStep::~CT_UtilsStep()
	{
	iFsSession.Close();
	}



//Function:doTestStepPreambleL
//Description :
//@return TVerdict pass / fail
TVerdict CT_UtilsStep::doTestStepPreambleL()
	{
	INFO_PRINTF1(_L("Test Step :Perf Utils step Preamble setup"));
	TVerdict	ret=CTestStepV2::doTestStepPreambleL();

	if (!GetBoolFromConfig(ConfigSection(),KT_FormatDrive,iFormatDrive))
		{
		WARN_PRINTF1(_L("Warning: No Boolean given to indicate a format of drive! \n Using default and not formatting drive "));
		}
	if (!GetStringFromConfig(ConfigSection(), KT_BaseDirName, iDirBaseName))
		{
		ERR_PRINTF1( _L("Unable to read in parent directory name to use within test, Will not be able to create a directory structure! ") );
		ret=EFail;
		SetTestStepResult(EFail);
		}

	User::LeaveIfError(iFsSession.Connect());

	return TestStepResult();
	}

// Function DoFormatDriveL
// Description
void CT_UtilsStep::DoFormatDriveL()
	{
	if ( iFormatDrive )
		{
		TPtrC	driveName=iDirBaseName.Left(2);	// X:
		RFormat	format;
		TInt	tracksRemaining = 0;
		TInt	errformat=format.Open(iFsSession, driveName, EHighDensity, tracksRemaining);
		CleanupClosePushL(format);
		while ( tracksRemaining && (errformat==KErrNone) )
   			{
   			errformat=format.Next(tracksRemaining);
   			}
		if (errformat!=KErrNone)
			{
			ERR_PRINTF2( _L("Format not successfully failed with error::...(%d)"), errformat);
			SetTestStepResult(EFail);
			}
		CleanupStack::PopAndDestroy(&format);
		}
	}

//Function:RemoveDirTreeL
//Description :
//@param TDesC& aDir
//@return TVerdict
TInt CT_UtilsStep::RemoveDirTreeL()
	{
	CFileMan*	fileMan=CFileMan::NewL(iFsSession);
	CleanupStack::PushL(fileMan);

	TInt		err=fileMan->RmDir(iDirBaseName);
	CleanupStack::PopAndDestroy(fileMan);

	return	err;
	}
