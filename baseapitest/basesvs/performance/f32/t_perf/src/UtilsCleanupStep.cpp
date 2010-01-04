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


#include "UtilsCleanupStep.h"



// Function : CT_CleanupStep
// Description :CT_CleanupStep:: class constructor
CT_CleanupStep::CT_CleanupStep()
:	CT_UtilsStep()
	{
	}


// Function : ~CT_CleanupStep
// Description :CT_CleanupStep class destructor
CT_CleanupStep:: ~CT_CleanupStep()
	{
	}

//Function:doTestStepL
//Description :
//@return TVerdict pass / fail
TVerdict CT_CleanupStep::doTestStepL()
	{
	//clean up dir
	INFO_PRINTF1( _L("Removing directory: connecting to session"));
	if (RemoveDirTreeL()==KErrNone)
		{
		DoFormatDriveL();
		}

	return TestStepResult();
	}
