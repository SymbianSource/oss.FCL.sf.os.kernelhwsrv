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
This contains CT_DirData
*/

//	User includes
#include "T_DirData.h"
#include "T_SfSrvServer.h"

/*@{*/
///	Parameters
_LIT(KArrayElementNumber,						"element_number");
_LIT(KExpected,									"expected");
_LIT(KSortkey,									"sortkey");
_LIT(KNumSortkey,								"numsortkey");
_LIT(KEntryObject,								"entryObject");

///Commands
_LIT(KCmdCount,									"Count");
_LIT(KCmdOperatorBrackets,						"[]");
_LIT(KCmdSort,									"Sort");
_LIT(KCmdDestructor,							"~");


//Sort keys
_LIT(KESortNone,								"ESortNone");
_LIT(KESortByName,								"ESortByName");
_LIT(KESortByExt,								"ESortByExt");
_LIT(KESortBySize,								"ESortBySize");
_LIT(KESortByDate,								"ESortByDate");
_LIT(KESortByUid,								"ESortByUid");
_LIT(KEDirsAnyOrder,							"EDirsAnyOrder");
_LIT(KEDirsFirst,								"EDirsFirst");
_LIT(KEDirsLast,								"EDirsLast");
_LIT(KEAscending,								"EAscending");
_LIT(KEDescending,								"EDescending");
_LIT(KEDirDescending,							"EDirDescending");


CT_DirData* CT_DirData::NewL()
/**
* Two phase constructor
*/
	{
	CT_DirData* ret = new (ELeave) CT_DirData();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}


CT_DirData::CT_DirData()
:	iDir(NULL)
/**
* Protected constructor. First phase construction
*/
	{
	}


void CT_DirData::ConstructL()
/**
* Protected constructor. Second phase construction
*/
	{
	}


CT_DirData::~CT_DirData()
/**
* Destructor.
*/
	{
	DoCleanup();
	}


TAny* CT_DirData::GetObject()
	{
	return iDir;
	}
	
	
void CT_DirData::SetObjectL(TAny* aAny)
	{
	DoCleanup();
	iDir = static_cast<CDir*> (aAny);
	}


void CT_DirData::DisownObjectL()
	{
	iDir = NULL;
	}
	
	
inline TCleanupOperation CT_DirData::CleanupOperation()
	{
	return CleanupOperation;
	}


void CT_DirData::CleanupOperation(TAny* aAny)
	{
	CDir* dir=static_cast<CDir*>(aAny);
	delete dir;
	}


TBool CT_DirData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/)
/**
* Process a command read from the ini file
*
* @param aCommand	the command to process
* @param aSection	the entry in the ini file requiring the command to be processed
*
* @return ETrue if the command is processed
*/
	{
	TBool retVal = ETrue;

	if (aCommand == KCmdCount)
		{
		DoCmdCount(aSection);
		}
	else if (aCommand == KCmdOperatorBrackets)
		{
		DoCmdOperatorBracketsL(aSection);
		}
	else if (aCommand == KCmdSort)
		{
		DoCmdSort(aSection);
		}
	else if (aCommand == KCmdDestructor)
		{
		DoCleanup();
		}
	else
		{
		retVal = EFalse;
		}
	return retVal;
	}


void CT_DirData::DoCleanup()
	{
	INFO_PRINTF1(_L("Doing cleanup"));
	if (iDir)
		{
		delete iDir;
		iDir = NULL;
		}
	}
	
void CT_DirData::DoCmdCount(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Counts directory entries!"));
	
	TInt	expected;
	if (GET_MANDATORY_INT_PARAMETER(KExpected, aSection, expected))
		{
		TInt count = iDir->Count();
		if (count != expected)
			{
			ERR_PRINTF3(_L("Result didn't match with expected result! COUNT: %d, expected: %d"), count, expected);
			SetBlockResult(EFail);
			}
		else
			{
			INFO_PRINTF2(_L("Result matched with expected result (%d)!"), count);
			}
		}
	}
	
	
void CT_DirData::DoCmdOperatorBracketsL(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Getting element and compare it with expected!"));
	
	TInt	elementNumber;
		
	if (GET_MANDATORY_INT_PARAMETER(KArrayElementNumber, aSection, elementNumber))
		{
        INFO_PRINTF2( _L( "Get element[%d]" ), elementNumber);
		TEntry* entryObject = new(ELeave) TEntry();
		CleanupStack::PushL(entryObject);
		
		*entryObject = iDir->operator[](elementNumber);

	    if ( !FileserverUtil::VerifyTEntryDataFromIniL(*this, aSection, *entryObject))
   		    {
   		    SetBlockResult(EFail);
   		    }
    		    
		TPtrC	entryObjectName;
		if (GET_OPTIONAL_STRING_PARAMETER(KEntryObject, aSection, entryObjectName))
			{
			CT_EntryData* entryWrapperObject = static_cast<CT_EntryData*>(GetDataWrapperL(entryObjectName));
		    if(entryWrapperObject)
				{
				entryWrapperObject->SetObjectL(entryObject);
				entryObject = NULL;
				}
			else
				{
				SetBlockResult(EFail);
				}
			}
			
		CleanupStack::Pop();
		delete entryObject;
		entryObject = NULL;
		}
	}	
   	
void CT_DirData::DoCmdSort(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Sorting directory entries!"));
	
	TPtrC	sortKey;
		
	if (GET_OPTIONAL_STRING_PARAMETER(KSortkey, aSection, sortKey))
		{	
		TUint fixedKey = 0;
		
		if ( !ConvertSortKeys(sortKey, fixedKey) )
			{
			ERR_PRINTF2(_L("Given sortkey (%S) is invalid"), &sortKey);
			SetBlockResult(EFail);
			}
					
		TInt err = iDir->Sort(fixedKey);
		
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("Directory entries have not been sorted! Error code = %d"), err);
			SetError(err);
			}
		else
			{
			INFO_PRINTF1(_L("Directory entries have been sorted!"));
			}
		}
	else
		{
		TInt	numSortKey;
		if (GET_MANDATORY_INT_PARAMETER(KNumSortkey, aSection, numSortKey))
			{
			TInt err = iDir->Sort(numSortKey);
			
			if (err != KErrNone)
				{
				ERR_PRINTF2(_L("Directory entries has not been sorted! Error code = %d"), err);
				SetError(err);
				}
			else
				{
				INFO_PRINTF1(_L("Directory entries has been sorted!"));
				}
			}
		}
	INFO_PRINTF1(_L("Results after sorting!"));
	for (TInt i = 0; i < iDir->Count(); i++)
	    {
		INFO_PRINTF3(_L("%d) %S"), i+1, &(*iDir)[i].iName);
		}
	}
	
TBool CT_DirData::ConvertSortKeys(TDesC &aConstantName, TUint& aSortKey)
	{
	
	TBool ret = ETrue;
	
	if (aConstantName == KESortByName)
		{
		aSortKey = ESortByName;
		}
	else if (aConstantName == KESortByExt)
		{
		aSortKey = ESortByExt;
		}
	else if (aConstantName == KESortBySize)
		{
		aSortKey = ESortBySize;
		}
	else if (aConstantName == KESortByDate)
		{
		aSortKey = ESortByDate;
		}
	else if (aConstantName == KESortByUid)
		{
		aSortKey = ESortByUid;
		}
	else if (aConstantName == KEDirsAnyOrder)
		{
		aSortKey = EDirsAnyOrder;
		}
	else if (aConstantName == KEDirsFirst)
		{
		aSortKey = EDirsFirst;
		}
	else if (aConstantName == KEDirsLast)
		{
		aSortKey = EDirsLast;
		}
	else if (aConstantName == KEAscending)
		{
		aSortKey = EAscending;
		}
	else if (aConstantName == KEDescending)
		{
		aSortKey = EDescending;
		}
	else if (aConstantName == KEDirDescending)
		{
		aSortKey = EDirDescending;
		}
	else if (aConstantName == KESortNone)
		{
		aSortKey = ESortNone;
		}
	else
		{
		TInt	location = aConstantName.Match(_L("*|*"));
		if( location != KErrNotFound )
			{
			//Converting Left part of the data
			TPtrC	tempStr = aConstantName.Left(location);
			ret = ConvertSortKeys(tempStr, aSortKey);

			//Converting right data can be with another "|"
			tempStr.Set(aConstantName.Mid(location + 1));

			TUint	tmp;
			if ( ConvertSortKeys(tempStr, tmp) )
				{
				aSortKey |= tmp;
				}
			else
				{
				ret = EFalse;
				}
			}
		}

	return ret;
	}
