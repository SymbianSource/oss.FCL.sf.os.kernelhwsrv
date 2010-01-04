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




 
#include "t_fat32larger512.h"

/**
Class Constructor
*/		    
CBaseTestFat32Larger512::CBaseTestFat32Larger512()
	{
	SetTestStepName(KTestStepLarger512);
	}
	
/**
Class Destructor
*/	
CBaseTestFat32Larger512::~CBaseTestFat32Larger512() 
	{
	}


/**
Checks that if the disk size is greater or equal to 512MB, the file system is 
FAT32, else the file system is FAT. 


@return EPass if test passes and EFail if test fails
*/		    			    
TVerdict CBaseTestFat32Larger512::doTestStepL()
	{
	SetTestStepResult(EPass);
	
	if (iDiskSize >= (512*1024*1024))
		{
		if (IsFileSystemFAT32())
			{
			_LIT(KLargCorrect, "Disk size is greater or equal to 512MB and the filesystem is FAT32");
			INFO_PRINTF1(KLargCorrect);
			SetTestStepResult(EPass);
			}
		else
			{
			_LIT(KLargErr, "Disk size is greater or equal to 512MB and the filesystem is not FAT32");
			INFO_PRINTF1(KLargErr);
			SetTestStepResult(EFail);
			}
		}
	if (iDiskSize < (512*1024*1024))
		{
		if (IsFileSystemFAT(iTheFs ,CurrentDrive()))
			{
			_LIT(KLargCorrect, "Disk size is less 512MB and the filesystem is FAT");
			INFO_PRINTF1(KLargCorrect);
			SetTestStepResult(EPass);
			}
		else
			{
			_LIT(KLargErr, "Disk size less than 512MB and the filesystem is not FAT");
			INFO_PRINTF1(KLargErr);
			SetTestStepResult(EFail);
			}
		}
	return TestStepResult();		
	}

