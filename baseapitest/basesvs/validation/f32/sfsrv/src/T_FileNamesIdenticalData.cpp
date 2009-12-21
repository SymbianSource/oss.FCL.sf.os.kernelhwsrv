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


/**
@test
@internalComponent
v
This contains CT_FileNamesIdenticalData
*/

//	User includes
#include "T_FileNamesIdenticalData.h"
#include "T_SfSrvServer.h"

/*@{*/
///	Parameters
_LIT(KFile1,										"file1");
_LIT(KFile2,										"file2");
_LIT(KExpected,										"expected");


///Commands
_LIT(KCmdFileNamesIdentical,						"FileNamesIdentical");


CT_FileNamesIdenticalData* CT_FileNamesIdenticalData::NewL()
/**
* Two phase constructor
*/
	{
	CT_FileNamesIdenticalData* ret = new (ELeave) CT_FileNamesIdenticalData();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}


CT_FileNamesIdenticalData::CT_FileNamesIdenticalData()
/**
* Protected constructor. First phase construction
*/
	{
	}


void CT_FileNamesIdenticalData::ConstructL()
/**
* Protected constructor. Second phase construction
*/
	{
	}


CT_FileNamesIdenticalData::~CT_FileNamesIdenticalData()
/**
* Destructor.
*/
	{
	}


TAny* CT_FileNamesIdenticalData::GetObject()
	{
	return NULL;
	}


TBool CT_FileNamesIdenticalData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/)
/**
* Process a command read from the ini file
*
* @param aCommand	the command to process
* @param aSection	the section in the ini file requiring the command to be processed
*
* @return ETrue if the command is processed
*/
	{
	TBool retVal = ETrue;

	if (aCommand == KCmdFileNamesIdentical)
		{
		DoCmdFileNamesIdentical(aSection);
		}
	return retVal;
	}
	
void CT_FileNamesIdenticalData::DoCmdFileNamesIdentical(const TDesC& aSection)
	{
	TPtrC file1, file2;
	TBool expected;
	if(GET_MANDATORY_STRING_PARAMETER(KFile1, aSection, file1)
		&& GET_MANDATORY_STRING_PARAMETER(KFile2, aSection, file2)
		&& GET_MANDATORY_BOOL_PARAMETER(KExpected, aSection, expected))
		{
		TBool identical = EFalse;
		
		INFO_PRINTF1(_L("Comparing files:"));
		INFO_PRINTF2(_L("File 1: %S"), &file1);
		INFO_PRINTF2(_L("File 2: %S"), &file2);
		
		identical = FileNamesIdentical(file1,file2);
		if(identical != expected)
			{
			ERR_PRINTF1(_L("File names are not identical and it's not expected!"));
			SetBlockResult(EFail);
			}
		else
			{
			if (expected)
				{
				INFO_PRINTF1(_L("File names are identical."));
				}
			else
				{
				INFO_PRINTF1(_L("File names are not identical, but it's expected."));
				}
			}
		}
	}
