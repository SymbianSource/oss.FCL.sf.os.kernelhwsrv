// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// Run RFs::CheckDisk
// 
//

#include "sdcheckdisk.h"

/*
Class constructor

@param None
@return None
*/
CBaseTestSDCheckDisk::CBaseTestSDCheckDisk()
	{
	SetTestStepName(KTestStepCheckDisk);
	}

/*
Test Step Preamble
 - Initialise attribute iDrive
 - Connect to the File Server

@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDCheckDisk::doTestStepPreambleL()
	{
	SetTestStepResult(EFail);
	
	if (!InitDriveLetter())
		return TestStepResult();
	if (!InitFileServer())
		return TestStepResult();

	SetTestStepResult(EPass);
	return TestStepResult();
	}

/*
Test step

@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDCheckDisk::doTestStepL()
	{
	if (TestStepResult() == EPass)
		{
		TInt r;
		TBuf<3> driveToTest;
		driveToTest.Format(_L("%c:\\"), 'A' + iDrive);
		r = iFs.CheckDisk(driveToTest);
		if (r != KErrNone)
			{
			ERR_PRINTF2(_L("RFs::CheckDisk returned: %d"), r);
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		INFO_PRINTF1(_L("RFs::CheckDisk returned no error"));
		}
	else
		{
		INFO_PRINTF1(_L("Test preamble did not complete succesfully - Test Step skipped"));
		}
	return TestStepResult();
	}
