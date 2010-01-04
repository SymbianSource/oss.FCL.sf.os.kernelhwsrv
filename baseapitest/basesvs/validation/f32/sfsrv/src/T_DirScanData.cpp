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
This contains CT_DirScanData
*/

//	User includes
#include "T_DirScanData.h"
#include "T_SfSrvServer.h"

/*@{*/
///	Parameters
_LIT(KAttMask,							"attmask" );
_LIT(KRfsObject,						"rfsObject");
_LIT(KDirObject,						"dirObject");
_LIT(KExpected,							"expected");
_LIT(KPath,								"path");
_LIT(KEntrySortKey,						"sortkey");
_LIT(KScanDirection,					"scandirection");

///Commands
_LIT(KCmdNewL,							"NewL");
_LIT(KCmdNewLC,							"NewLC");
_LIT(KCmdSetScanDataL,					"SetScanDataL");
_LIT(KCmdFullPath,						"FullPath");
_LIT(KCmdAbbreviatedPath,				"AbbreviatedPath");
_LIT(KCmdNextL,							"NextL");
_LIT(KCmdDestructor,					"~");

//	Sort key
_LIT(KESortNone,						"ESortNone");
_LIT(KESortByName,						"ESortByName");
_LIT(KESortByExt,						"ESortByExt");
_LIT(KESortBySize,						"ESortBySize");
_LIT(KESortByDate,						"ESortByDate");
_LIT(KESortByUid,						"ESortByUid");
_LIT(KEDirsAnyOrder,					"EDirsAnyOrder");
_LIT(KEDirsFirst,						"EDirsFirst");
_LIT(KEDirsLast,						"EDirsLast");
_LIT(KEAscending,						"EAscending");
_LIT(KEDescending,						"EDescending");
_LIT(KEDirDescending,					"EDirDescending");

// Scan directions
_LIT(KEScanUpTree,	 					"EScanUpTree");
_LIT(KEScanDownTree,	 				"EScanDownTree");


CT_DirScanData* CT_DirScanData::NewL()
/**
* Two phase constructor
*/
	{
	CT_DirScanData* ret = new (ELeave) CT_DirScanData();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}
	

CT_DirScanData::CT_DirScanData()
:	iDirScan(NULL)
/**
* Protected constructor. First phase construction
*/
	{
	}


void CT_DirScanData::ConstructL()
/**
* Protected constructor. Second phase construction
*/
	{
	}


CT_DirScanData::~CT_DirScanData()
/**
* Destructor.
*/
	{
	DoCleanup();
	}


TAny* CT_DirScanData::GetObject()
	{
	return iDirScan;
	}


TBool	CT_DirScanData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/)
/**
* Process a command read from the ini file
*
* @param aCommand	the command to process
* @param aSection	the section in the ini file requiring the command to be processed
*
* @return ETrue if the command is processed
*/
	{
	TBool	retVal = ETrue;

	if (aCommand == KCmdNewL)
		{
		DoCmdNewL(aSection);
		}
	else if (aCommand == KCmdDestructor)
		{
		DoCleanup();
		}
	else if (aCommand == KCmdNewLC)
		{
		DoCmdNewLC(aSection);
		}
	else if (aCommand == KCmdSetScanDataL)
		{
		DoCmdSetScanDataL(aSection);
		}
	else if (aCommand == KCmdNextL)
		{
		DoCmdNextL(aSection);
		}
	else if (aCommand == KCmdAbbreviatedPath)
		{
		DoCmdAbbreviatedPath(aSection);
		}
	else if (aCommand == KCmdFullPath)
		{
		DoCmdFullPath(aSection);
		}
	else
		{
		retVal = EFalse;
		}
	return retVal;
	}

void CT_DirScanData::DoCleanup()
	{
	INFO_PRINTF1(_L("Doing cleanup!"));
	
	if (iDirScan)
		{
		delete iDirScan;
		iDirScan = NULL;
		}
	}
		
void CT_DirScanData::DoCmdNewL(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Constructing CDirScan object!"));
	
	RFs*	rfsObject = NULL;
	TPtrC	rfsObjectName;
	if (GET_MANDATORY_STRING_PARAMETER(KRfsObject, aSection, rfsObjectName))
		{
		rfsObject = (RFs*)GetDataObjectL(rfsObjectName);
		}
	
	DoCleanup();
	TRAPD (error, iDirScan = CDirScan::NewL(*rfsObject));
	if (error == KErrNone)
		{
		INFO_PRINTF1(_L("CDirScan object has been created with NewL constructor!"));			
		}
	else
		{
		ERR_PRINTF2(_L("CDirScan object is not created with NewL constructor! Error code: %d"), error);
		SetError(error);
		}
	}
	
void CT_DirScanData::DoCmdNewLC(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Constructing CDirScan object!"));
	
	RFs*	rfsObject = NULL;
	TPtrC	rfsObjectName;
	if (GET_MANDATORY_STRING_PARAMETER(KRfsObject, aSection, rfsObjectName))
		{
		rfsObject = (RFs*)GetDataObjectL(rfsObjectName);
		}
	
	DoCleanup();
	TRAPD (error, iDirScan = CDirScan::NewLC(*rfsObject); CleanupStack::Pop(iDirScan) );
	if (error == KErrNone)
		{
		INFO_PRINTF1(_L("CDirScan object has been created with NewLC constructor!"));
		}
	else
		{
		ERR_PRINTF2(_L("CDirScan object is not created with NewLC constructor! Error code: %d"), error);
		SetError(error);
		}
	}

void CT_DirScanData::DoCmdSetScanDataL(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Setting data scanning!"));
	
	TBool	dataOk = ETrue;
	
	TPtrC	path;
	if (!GET_MANDATORY_STRING_PARAMETER(KPath(), aSection, path))
		{
		dataOk = EFalse;
		}

	TUint	attMask = KEntryAttHidden;
	if ( !FileserverUtil::GetAttMask(*this, aSection, KAttMask(), attMask))
        {
       	dataOk = EFalse;
       	}

	TPtrC	entrySortKey;
	TUint	sortKey = ESortNone;
	
	if (GET_OPTIONAL_STRING_PARAMETER(KEntrySortKey(), aSection, entrySortKey))
		{
		if ( !ConvertToSortKey(entrySortKey, sortKey) )
			{
			TInt	intTemp;
			if ( GET_MANDATORY_INT_PARAMETER(KEntrySortKey(), aSection, intTemp) )
				{
				sortKey=intTemp;
				}
			else
				{
				dataOk = EFalse;
				}
			}
		}
		
	TPtrC						entryScanDirection;
	CDirScan::TScanDirection	scanDirection = CDirScan::EScanDownTree;
	
	if (GET_OPTIONAL_STRING_PARAMETER(KScanDirection(), aSection, entryScanDirection))
		{
		if ( !ConvertToScanDirection(entryScanDirection, scanDirection))
            {
           	dataOk = EFalse;
           	}
		}
		
	
	if (dataOk)
		{
		TRAPD (err, iDirScan->SetScanDataL(path, attMask, sortKey, scanDirection));
		if(err == KErrNone)
			{
			INFO_PRINTF1(_L("SetScanDataL complete!"));
			}
		else
			{
			ERR_PRINTF2(_L("SetScanDataL() Error: %d"), err);
			SetError(err);
			}
		}
	}	

void CT_DirScanData::DoCmdNextL(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Scanning next directory entries!"));
	
	CDir*		dirObject = NULL;
	CT_DirData* dirWrapperObject = NULL;
	TPtrC		dirObjectName;
	
	if (GET_MANDATORY_STRING_PARAMETER(KDirObject, aSection, dirObjectName))
		{
		dirWrapperObject = static_cast<CT_DirData*>(GetDataWrapperL(dirObjectName));
		}
	TRAPD (err, iDirScan->NextL(dirObject));
	if (err == KErrNone)
		{
		if (dirObject == NULL)
			{
			INFO_PRINTF1(_L("CDir instance is NULL, there is no directory left to go."));
			}
		else
			{
			INFO_PRINTF1(_L("Scanning the next directory entry completed successfully!"));
			}
		}
	else
		{
		ERR_PRINTF2(_L("Can't scan the next directory entry in the structure Error code: %d"), err);
		SetError(err);
		}
		
	if (dirObject)
		{
		if(dirWrapperObject)
			{
			dirWrapperObject->SetObjectL(dirObject);
			}
		else
		    {
		    delete dirObject;
		    dirObject = NULL;			        
		    }
		}	
	}
	
void CT_DirScanData::DoCmdAbbreviatedPath(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Getting directory abbreviated path!"));
	
	TPtrC	expected;
	if (GET_MANDATORY_STRING_PARAMETER(KExpected(), aSection, expected))
		{
		TPtrC	abbPath;
		abbPath.Set(iDirScan->AbbreviatedPath());
		
		if (abbPath != expected)
			{
			ERR_PRINTF3(_L("Result (%S) didn't match with expected result (%S)!"), &abbPath, &expected);
			SetBlockResult(EFail);
			}
		else 
			{
			INFO_PRINTF2(_L("Result (%S) match with expected result!"), &abbPath);
			}
		}
	}

void CT_DirScanData::DoCmdFullPath(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("Getting directory full path!"));
	
	TPtrC	expected;
	if (GET_MANDATORY_STRING_PARAMETER(KExpected(), aSection, expected))
		{
		TPtrC	fullPath;
		fullPath.Set(iDirScan->FullPath());
		
		if (fullPath != expected)
			{
			ERR_PRINTF3(_L("Result (%S) didn't match with expected result (%S)!"), &fullPath, &expected);
			SetBlockResult(EFail);
			}
		else 
			{
			INFO_PRINTF2(_L("Result (%S) match with expected result!"), &fullPath);
			}
		}
	}
	
	
TBool CT_DirScanData::ConvertToSortKey(const TDesC& aSortKeyStr, TUint& aSortKey)
	{
	TBool ret = ETrue;

	if (aSortKeyStr == KESortNone)
		{
		aSortKey = ESortNone;
		}
	else if (aSortKeyStr == KESortByName)
		{
		aSortKey = ESortByName;
		}
	else if (aSortKeyStr == KESortByExt)
		{
		aSortKey = ESortByExt;
		}
	else if (aSortKeyStr == KESortBySize)
		{
		aSortKey = ESortBySize;
		}
	else if (aSortKeyStr == KESortByDate)
		{
		aSortKey = ESortByDate;
		}
	else if (aSortKeyStr == KESortByUid)
		{
		aSortKey = ESortByUid;
		}
	else if (aSortKeyStr == KEDirsAnyOrder)
		{
		aSortKey = EDirsAnyOrder;
		}
	else if (aSortKeyStr == KEDirsFirst)
		{
		aSortKey = EDirsFirst;
		}
	else if (aSortKeyStr == KEDirsLast)
		{
		aSortKey = EDirsLast;
		}
	else if (aSortKeyStr == KEAscending)
		{
		aSortKey = EAscending;
		}
	else if (aSortKeyStr == KEDescending)
		{
		aSortKey = EDescending;
		}
	else if (aSortKeyStr == KEDirDescending)
		{
		aSortKey = EDirDescending;
		}
	else if (aSortKeyStr.Match((_L("*|*"))) != KErrNotFound)
		{
		TUint tmpSortKey;

		TInt location = aSortKeyStr.Match(_L("*|*"));
		//Converting Left part of the data
		TPtrC left = aSortKeyStr.Left(location);
		if (ConvertToSortKey(left, tmpSortKey))
			{
			aSortKey = tmpSortKey;
			}
		else
			{
			ret = EFalse;
			}

		//Converting right data can be with another "|"
		TPtrC right = aSortKeyStr.Mid(location + 1);

		if (ConvertToSortKey(right, tmpSortKey))
			{
			aSortKey = aSortKey | tmpSortKey;
			}
		else
			{
			ret = EFalse;
			}
		}
	else
		{
		ret = EFalse;
		}

	return ret;
	}


	
TBool CT_DirScanData::ConvertToScanDirection(const TDesC& aScanDirectionStr, CDirScan::TScanDirection& aScanDirection)
	{
	TBool ret = ETrue;

	if (aScanDirectionStr == KEScanUpTree)
		{
		aScanDirection = CDirScan::EScanUpTree;
		}
	else if (aScanDirectionStr == KEScanDownTree)
		{
		aScanDirection = CDirScan::EScanDownTree;
		}
	else
		{
		ret = EFalse;
		}

	return ret;
	}
