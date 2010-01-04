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



#include "t_fat32format.h"

/**
Class Constructor
*/		    
CBaseTestFat32Format::CBaseTestFat32Format() 
	{
	SetTestStepName(KTestStepFormat);
	}
	
/**
Class Destructor
*/
CBaseTestFat32Format::~CBaseTestFat32Format() 
	{
	}


/** 
Get the type of format required from the ini file and format the disk

@return EPass if test passes and EFail if test fails
*/ 

TVerdict CBaseTestFat32Format::doTestStepL()
	{
	_LIT(KFormatType,"FormatType");
	TBuf<4> formatType;
	TPtrC16 format = formatType;
	SetTestStepResult(EFail);
	TDriveUnit drive (CurrentDrive());
	TBool alright = GetStringFromConfig(ConfigSection(), KFormatType, format);
	if(alright)
		{
		TInt r = FormatFat(drive, format);
		if (r != KErrNone)
			{
			_LIT(KFormatError, "Error %d - could not format %c drive");
			INFO_PRINTF3(KFormatError, r, (TUint)iDriveToTest);
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		}
	else
		{
		_LIT(KErrorRead, "Cannot get the format type from the ini file");
		INFO_PRINTF1(KErrorRead);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	SetTestStepResult(EPass);
	return TestStepResult();
	}


/** 
Format the disk

@param aDrive The drive to format
@param aFormat The type of format to perform

@return EPass if test passes and EFail if test fails
*/ 
TInt CBaseTestFat32Format::FormatFat(TDriveUnit aDrive, TPtrC16 aFormat)
	{
	TInt count;
	RFormat format;
	TInt r;
	TFormatMode formatMode = EFullFormat;
	if (aFormat == _L("Full"))
		{
		formatMode = EFullFormat;
		}
	else if (aFormat == _L("Special"))
		{
		formatMode = ESpecialFormat;
		}
	else if (aFormat == _L("Quick"))
		{
		formatMode = EQuickFormat;
		}
		
	r = format.Open(iTheFs,aDrive.Name(),formatMode,count);
	if(r!=KErrNone)
		{
		_LIT(KFormatOpen, "Device could not be opened for formatting");
		INFO_PRINTF1(KFormatOpen);
		return r;
		}
	INFO_PRINTF2(_L("Count = %d"), count);
	do	{
		r = format.Next(count);
		if (r != KErrNone)
			{
			_LIT(KFormatNextError, "format.Next() error %d count %d\n");
			INFO_PRINTF3(KFormatNextError, r, count);
			return r;
			}
		}while (count > 0);
	format.Close();
	INFO_PRINTF2(_L("Count = %d"), count);
	return r; 
	}
