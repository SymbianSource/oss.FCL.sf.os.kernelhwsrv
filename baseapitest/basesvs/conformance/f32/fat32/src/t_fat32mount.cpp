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



#include "t_fat32mount.h"

/**
Class Constructor
*/		    
CBaseTestFat32Mount::CBaseTestFat32Mount() 
	{
	SetTestStepName(KTestStepMount);
	}
	
/**
Class Destructor
*/
CBaseTestFat32Mount::~CBaseTestFat32Mount() 
	{
	}

/** 
Mounting the file system
From the ini file obtain whether the mount should pass or fail

@return EPass if test passes and EFail if test fails
*/ 
TVerdict CBaseTestFat32Mount::doTestStepL()
	{
	SetTestStepResult(EFail);
	TDriveUnit drive (CurrentDrive());
	TInt r;
	TInt res;
	_LIT(KMountPass, "Pass");
	_LIT(KMountFail, "Fail");
	_LIT(KCorrectVerdict,"CorrectVerdict");
	TBuf<4> correctVerdict;
	TPtrC16 correct = correctVerdict;
	TBuf<4> actualVerdict;
	TPtrC16 actual = actualVerdict;

	if(IsFileSystemFAT32())
		{
		TFullName oldFs;
		res = iTheFs.FileSystemName(oldFs,CurrentDrive());
		res = iTheFs.DismountFileSystem(oldFs,CurrentDrive());
		if (res != KErrNone)
			{
			_LIT(KDismountError, "Error %d - could not could not dismount filesystem from drive %d");
			INFO_PRINTF3(KDismountError, res, CurrentDrive());
			}
		_LIT(KFsNameFat32, "Fat");
		r = iTheFs.AddFileSystem(KFsNameFat32);
		
		TBool alright = GetStringFromConfig(ConfigSection(), KCorrectVerdict, correct);
		if(alright)
			{
			r = iTheFs.MountFileSystem(KFsNameFat32, CurrentDrive());
			if (r != KErrNone)
				{
				actualVerdict = KMountFail;
				}
			else 
				{
				actualVerdict = KMountPass;
				}
			if (actualVerdict == correct)
				{
				SetTestStepResult(EPass);
				return TestStepResult();
				}
			else
				{
				SetTestStepResult(EFail);
				return TestStepResult();
				}
			}
		}
	else
		{
			_LIT(KFsNameFat, "Fat");
		res = iTheFs.DismountFileSystem(KFsNameFat,CurrentDrive());
		if (res !=KErrNone)
			{
			_LIT(KDismountError, "Error %d - could not could not dismount filesystem from drive %d");
			INFO_PRINTF3(KDismountError, res, CurrentDrive());
			}
		r = iTheFs.MountFileSystem(KFsNameFat, CurrentDrive());
		}
	if (r != KErrNone)
		{
		_LIT(KMountError, "Error %d - could not mount filesystem on drive %d");
		INFO_PRINTF3(KMountError, r, CurrentDrive());
		SetTestStepResult(EFail);
		return TestStepResult();
		}

	SetTestStepResult(EPass);
	return TestStepResult();
	}
