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
// Suspend test execution and display dialogue box on screen
// 
//

#include "sddialogbox.h"

/*
Class constructor

@param None
@return None
*/
CBaseTestSDDialogBox::CBaseTestSDDialogBox()
	{
	SetTestStepName(KTestStepDialogBox);
	}

/*
Test Step Preamble
 - Does nothing
 
@param None
@return EPass
@see TVerdict
*/
TVerdict CBaseTestSDDialogBox::doTestStepPreambleL()
	{
	SetTestStepResult(EPass);
	return TestStepResult();
	}

/*
Test step

@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDDialogBox::doTestStepL()
	{
	TInt r;
	TPtrC pLine1;
	TPtrC pLine2;
	TPtrC pButton1;
	TPtrC pButton2;
	TInt buttonvalpass;

	_LIT(KDialogBoxTextLine1, "DialogBoxTextLine1");
	_LIT(KDialogBoxTextLine2, "DialogBoxTextLine2");
	_LIT(KDialogBoxTextButton1, "DialogBoxTextButton1");
	_LIT(KDialogBoxTextButton2, "DialogBoxTextButton2");
	_LIT(KDialogBoxButtonValuePassCondition, "DialogBoxButtonValuePassCondition");

	// Read the text to be displayed from the INI File
	if (!GetStringFromConfig(ConfigSection(), KDialogBoxTextLine1, pLine1))
			{
			ERR_PRINTF1(_L("INI file read error - DialogBoxTextLine1"));
			SetTestStepResult(EFail);
			return TestStepResult();
			}
	if (!GetStringFromConfig(ConfigSection(), KDialogBoxTextLine2, pLine2))
			{
			ERR_PRINTF1(_L("INI file read error - DialogBoxTextLine2"));
			SetTestStepResult(EFail);
			return TestStepResult();
			}
	if (!GetStringFromConfig(ConfigSection(), KDialogBoxTextButton1, pButton1))
			{
			ERR_PRINTF1(_L("INI file read error - DialogBoxTextButton1"));
			SetTestStepResult(EFail);
			return TestStepResult();
			}
	if (!GetStringFromConfig(ConfigSection(), KDialogBoxTextButton2, pButton2))
			{
			ERR_PRINTF1(_L("INI file read error - DialogBoxTextButton2"));
			SetTestStepResult(EFail);
			return TestStepResult();
			}
	if (!GetIntFromConfig(ConfigSection(), KDialogBoxButtonValuePassCondition, buttonvalpass))
			{
			ERR_PRINTF1(_L("INI file read error - KDialogBoxButtonValuePassCondition"));
			SetTestStepResult(EFail);
			return TestStepResult();
			}

	TRequestStatus status;
	RNotifier notif;
	r = notif.Connect();
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("RNotifier::Connect() - Error %d"), r);
		SetTestStepResult(EFail);
		return TestStepResult();
		}

	TInt buttonval;
	notif.Notify(pLine1, pLine2, pButton1, pButton2, buttonval, status);
	User::WaitForRequest(status);

	if (buttonval + 1 == buttonvalpass)
		{
		INFO_PRINTF2(_L("User selected '%S'"), (buttonval ? &pButton2 : &pButton1));
		SetTestStepResult(EPass);
		}
	else
		{
		ERR_PRINTF2(_L("User selected '%S'"), (buttonval ? &pButton2 : &pButton1));
		SetTestStepResult(EFail);
		}

	notif.Close();
	return TestStepResult();
	}
