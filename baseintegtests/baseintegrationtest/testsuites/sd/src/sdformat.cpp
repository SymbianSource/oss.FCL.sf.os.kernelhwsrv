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
// Format disk
// 
//

#include "sdformat.h"

/*
Class constructor

@param None
@return None
*/
CBaseTestSDFormat::CBaseTestSDFormat()
	{
	SetTestStepName(KTestStepFormat);
	}

/*
Test Step Preamble
 - Initialise attribute iDrive
 - Connect to the File Server

@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDFormat::doTestStepPreambleL()
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
TVerdict CBaseTestSDFormat::doTestStepL()
	{
	if (TestStepResult() == EPass)
		{
		TInt r;
		TInt count;
		TBuf<2> drive = _L("?:");
		drive[0] = 'A' + iDrive;
		
		_LIT(KFormatType, "FormatType");
		TInt formatType = 0;
		GetIntFromConfig(ConfigSection(), KFormatType, formatType);
		
		if ((formatType != 1) && (formatType != 2))
			{
			ERR_PRINTF1(_L("Invalid Format Type Value"));
			SetTestStepResult(EFail);
			return TestStepResult();
			}
						
		INFO_PRINTF2(_L("Formatting %S"), &drive);
		RFormat format;
		// 1 -> EQuickFormat
		// 2 -> EFullFormat
		r = format.Open(iFs, drive, formatType == 1 ? EQuickFormat : EFullFormat, count);
		if (r != KErrNone)
			{
			ERR_PRINTF2(_L("RFormat::Open returned %d"), r);
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		
		if (count != 100)
			{
			ERR_PRINTF2(_L("Format count != 100 : %d"), count);
			SetTestStepResult(EFail);
			return TestStepResult();
			}
	
		do
			{
			r = format.Next(count);
			if (r != KErrNone)
				{
				ERR_PRINTF3(_L("RFormat::Next error %d count %d\n"), r, count);
				SetTestStepResult(EFail);
				return TestStepResult();
				}
			} while (count > 0);
		format.Close();
		}
	else
		{
		INFO_PRINTF1(_L("Test preamble did not complete succesfully - Test Step skipped"));
		}
	return TestStepResult();
	}
