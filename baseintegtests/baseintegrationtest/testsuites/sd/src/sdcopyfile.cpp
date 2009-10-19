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
// Copy a file
// 
//

#include "sdcopyfile.h"

/*
Class constructor

@param None
@return None
*/
CBaseTestSDCopyFile::CBaseTestSDCopyFile()
	{
	SetTestStepName(KTestStepCopyFile);
	}

/*
Test Step Preamble
 - Initialise attribute iDrive
 - Connect to the File Server
 - Instatiate a CFileMan object
 
@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDCopyFile::doTestStepPreambleL()
	{
	SetTestStepResult(EFail);
	
	if (!InitDriveLetter())
		return TestStepResult();
	if (!InitFileServer())
		return TestStepResult();
	if (!InitFileMan())
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
TVerdict CBaseTestSDCopyFile::doTestStepL()
	{
	if (TestStepResult() == EPass)
		{
		_LIT(KOriginalFile, "CopyFileOriginalFile");
		_LIT(KDestinationDirectory, "CopyFileDestinationDirectory");
		TPtrC pOrig;
		TPtrC pDest;
		
		if (!GetStringFromConfig(ConfigSection(), KOriginalFile, pOrig))
			{
			ERR_PRINTF1(_L("INI File Read failed"));
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		if (!GetStringFromConfig(ConfigSection(), KDestinationDirectory, pDest))
			{
			ERR_PRINTF1(_L("INI File Read failed"));
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		TFileName orig(pOrig);
		TFileName dest(pDest);
		
		TInt r;
		TFileName driveRoot;
		driveRoot.Format(_L("%c:\\"), 'A' + iDrive);
		r = iFs.SetSessionPath(driveRoot);	
		if (r != KErrNone)
			{
			ERR_PRINTF3(_L("Could not set RFs session path to %S: %d"), &driveRoot, r);
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		
		INFO_PRINTF3(_L("Copy %S to %S"), &orig, &dest);
		
		r = iFileMan->Copy(orig, dest, CFileMan::EOverWrite);
		
		if (r != KErrNone)
			{
			ERR_PRINTF4(_L("Failed to copy %S to %S - return value: %d"), &orig, &dest, r);
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
