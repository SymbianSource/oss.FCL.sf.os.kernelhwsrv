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
// Perform a second set of file operations as per the Test Specification
// 
//

#include "sdfileoperations2.h"

/*
Class constructor

@param None
@return None
*/
CBaseTestSDFileOperations2::CBaseTestSDFileOperations2()
	{
	SetTestStepName(KTestStepFileOperations2);
	}

/*
Test step

@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDFileOperations2::doTestStepL()
	{
	if (TestStepResult() == EPass)
		{
		TInt r;
		TBuf<4> sessionPath;
		sessionPath.Format(_L("%c:\\"), 'A' + iDrive);
		r = iFs.SetSessionPath(sessionPath);
		if (r != KErrNone)
			{
			ERR_PRINTF3(_L("Could not set session path to %c: (r = %d)"), 'A' + iDrive, r);
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		if (SetVolumeName() != iExpectedErrorCode)
			{
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		TBuf<50> subdir;
		subdir.Format(_L("\\dir%03d\\"), iRootEntries / 2 - 1);
		if (DeleteSubDirEntries(subdir) != iExpectedErrorCode)
			{
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		subdir.Format(_L("\\dir%03d\\"), iRootEntries / 2 - 2);
		if (CreateSubDirEntries(subdir) != iExpectedErrorCode)
			{
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		if (MoveFile(_L("LONG FILE NAME"), _L("C:\\")) != iExpectedErrorCode)
			{
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		if (CopyFile(_L("C:\\file002"), _L("BACK")) != iExpectedErrorCode)
			{
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		}
	else
		{
		INFO_PRINTF1(_L("Test preamble did not complete succesfully - Test Step skipped"));
		}
	return TestStepResult();
	}
