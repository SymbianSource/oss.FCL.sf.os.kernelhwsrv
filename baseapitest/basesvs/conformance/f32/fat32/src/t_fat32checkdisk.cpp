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



#include "t_fat32checkdisk.h"

/**
Class Constructor
*/		    
CBaseTestFat32CheckDisk::CBaseTestFat32CheckDisk()
	{
	SetTestStepName(KTestStepCheckDisk);
	}
	
/**
Class Destructor
*/
CBaseTestFat32CheckDisk::~CBaseTestFat32CheckDisk() 
	{
	}


/** 
The function performs the following actions
1. Verify the integrity of the disk by calling RFs::ScanDisk() 
2. Check whether the code returned match the code specified in the ini file. 
3. If scan dsik is specified in the ini file then call RFs::ScanDrive()

@return EPass if test passes and EFail if test fails
*/ 
TVerdict CBaseTestFat32CheckDisk::doTestStepL()
	{
	SetTestStepResult(EFail);
	TInt r; 
	_LIT(KCheckCode,"CheckCode");
	TInt checkcode;
	_LIT(KScan,"Scan");
	TInt scan;
	_LIT(KPassRegardless,"PassRegardless");
	TBuf<4> passRegardless;
	TPtrC16 passregardless = passRegardless;
	TBool alright = GetIntFromConfig(ConfigSection(), KCheckCode, checkcode);
	if(alright)
		{
		TBool alright2 = GetStringFromConfig(ConfigSection(), KPassRegardless, passregardless);
		if(alright2)
			{
			// Perform a check disk. 
			r=iTheFs.CheckDisk(iSessionPath);
			if (r!=checkcode)
				{
				// If the code returned does not match the code expected and would
				// like only a warning to be displayed as opposed to failing the 
				// test
				if (passregardless == _L("Y"))
					{
					SetTestStepResult(EPass);
					_LIT(KErrorPass, "RFs::CheckDisk() returns %d & does not match expected value %d");
					INFO_PRINTF3(KErrorPass, r, checkcode);		
					}
				// If the code returned does not match the code expected and would
				// like to fail the test
				else
					{
					SetTestStepResult(EFail);
					_LIT(KCheckDiskFail, "RFs::CheckDisk() returns %d & does not match expected value %d");
					INFO_PRINTF3(KCheckDiskFail, r, checkcode);
					}
				}
			else 
				{
				SetTestStepResult(EPass);
				_LIT(KCheckDiskPass, "RFs::CheckDisk() returns %d & matches expected value %d");
				INFO_PRINTF3(KCheckDiskPass, r, checkcode);	
				}
			// If specified in the ini file, perform a ScanDrive on the volume
			TBool alright = GetIntFromConfig(ConfigSection(), KScan, scan);
			if(alright)
				{
				r=iTheFs.ScanDrive(iSessionPath);
				if (r!=KErrNone)
					{
					SetTestStepResult(EFail);
					_LIT(KScanFail, "RFs::ScanDrive() failed returns %d");
					INFO_PRINTF2(KScanFail, r);		
					}
				else
					{
					SetTestStepResult(EPass);
					_LIT(KScanPass, "RFs::ScanDrive() passed");
					INFO_PRINTF1(KScanPass);
					}
				}
			else
				{
				_LIT(KNoScan, "RFs::ScanDrive() not indicated in ini file");
				INFO_PRINTF1(KNoScan);	
				}
			return TestStepResult();	
			}
			
		else
			{
			_LIT(KReadIniFail, "Could not read PassRegardless from ini file");
			INFO_PRINTF1(KReadIniFail);
			return TestStepResult();
			}
		}
	else
		{
		_LIT(KReadIniFail, "Could not read CheckCode from ini file");
		INFO_PRINTF1(KReadIniFail);
		return TestStepResult();
		}
	}
