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
This contains CT_RDirData
*/

//	User includes
#include "T_RDirData.h"
#include "T_SfSrvServer.h"
#include "FileserverUtil.h"



/*@{*/
///	Parameters
_LIT(KPath,										"path");
_LIT(KTUidType,									"uidtype");
_LIT(KRFsObjectName,							"rfsObject");
_LIT(KAsync,									"async");
_LIT(KArrayObjectName,							"arrayObject");
_LIT(KEntryObjectName,							"entryObject");
_LIT(KEntryAttMask,								"attmask");
_LIT(KParamExpectedFileName,					"filename%d");
_LIT(KReadArray,								"readarray");
_LIT(KCompareFiles,								"compare");


///Commands
_LIT(KCmdNew,									"new");
_LIT(KCmdDestructor,							"~");
_LIT(KCmdOpen,									"Open");
_LIT(KCmdRead,									"Read");
_LIT(KCmdClose,									"Close");

//Constants
const TInt	KBufferLength		= 64;

CT_RDirData* CT_RDirData::NewL()
/**
* Two phase constructor
*/
	{
	CT_RDirData* ret = new (ELeave) CT_RDirData();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}



CT_RDirData::CT_RDirData()
:	iRDir(NULL)
,	iRead(NULL)
,	iReadBlock(NULL)
,	iEntry(NULL)
,	iExpectedNames(NULL)
,	iObjName(NULL)
,	iCompare(NULL)
/**
* Protected constructor. First phase construction
*/
	{
	}


void CT_RDirData::ConstructL()
/**
* Protected constructor. Second phase construction
*/
	{
	iRead = CActiveCallback::NewL( *this );
	iReadBlock = CActiveCallback::NewL( *this );
	iExpectedNames = new(ELeave) RPointerArray<TPath>();
	}


CT_RDirData::~CT_RDirData()
/**
* Destructor.
*/
	{
	DoCleanup();
	}


TAny* CT_RDirData::GetObject()
	{
	return iRDir;
	}


TBool CT_RDirData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex)
/**
* Process a command read from the ini file
*
* @param aCommand	the command to process
* @param aSection		the entry in the ini file requiring the command to be processed
*
* @return ETrue if the command is processed
*/
	{
	TBool ret = ETrue;

	if (aCommand == KCmdNew)
		{
		DoCmdNew();
		}
	else if (aCommand == KCmdOpen)
		{
		DoCmdOpenL(aSection);
		}
	else if (aCommand == KCmdRead)
		{
		DoCmdReadL(aSection, aAsyncErrorIndex);
		}
	else if (aCommand == KCmdClose)
		{
		DoCmdClose();
		}
	else if (aCommand == KCmdDestructor)
		{
		DoCmdDestructor();
		}
	else
		{
		ret = EFalse;
		}
	return ret;
	}

void CT_RDirData::DoCleanup()
	{
	INFO_PRINTF1(_L("Deleting current RDir"));
	delete iEntry;
	iEntry = NULL;
	delete iRDir;
	iRDir = NULL;

	delete iRead;
	iRead = NULL;
	delete iReadBlock;
	iReadBlock = NULL;
	iExpectedNames->ResetAndDestroy();	
	}

void CT_RDirData::DoCmdNew()
	{
	INFO_PRINTF1(_L("Create new RDirs class instance"));

	delete iRDir;
	iRDir = NULL;

	TRAPD(err, iRDir = new (ELeave) RDir());
	if ( err != KErrNone )
		{
		ERR_PRINTF2(_L("new error %d"), err);
		SetError( err );
		}
	}
	
void CT_RDirData::DoCmdClose()
/** 
*	Close RDir handle 
*/
	{
	INFO_PRINTF1(_L("Closing RDir"));
	iRDir->Close();
	}

void CT_RDirData::DoCmdDestructor()
/** 
*	Destroy RDir the object 
*/
	{
	INFO_PRINTF1(_L("Destroying the RDir object"));
	DoCleanup();
	}
	
void CT_RDirData::DoCmdOpenL(const TDesC& aSection)
	{		
	INFO_PRINTF1(_L("Opening directory!"));
	
	TBool	dataOk = ETrue;
		
	TPtrC	rfsObjectName;
	RFs*    rfsObject = NULL;
	if(GET_MANDATORY_STRING_PARAMETER(KRFsObjectName(), aSection, rfsObjectName))
	    {
	    rfsObject = (RFs*)GetDataObjectL(rfsObjectName);
	    }
	else
		{
		dataOk = EFalse;
		}
	    
	TPtrC path;
	if(!GET_MANDATORY_STRING_PARAMETER(KPath(), aSection, path))
		{
		dataOk = EFalse;
		}
	
	TUidType	uidType = KNullUid;
	TInt		intUIDType;
	TBool		isUidType = EFalse;

	TUint		attMask = KEntryAttNormal;
	INFO_PRINTF2(_L("section name - %S"),&aSection);
	
	if(!FileserverUtil::GetAttMask(*this, aSection, KEntryAttMask(), attMask))
		{
		if (GET_MANDATORY_INT_PARAMETER(KTUidType(), aSection, intUIDType))
			{
    		TUid id = TUid::Uid(intUIDType);
	   		uidType = TUidType(id);
	   		INFO_PRINTF2(_L("UID type set to %d"), uidType[0].iUid);
	   		isUidType = ETrue;
			}
		else
			{
			dataOk = EFalse;
			ERR_PRINTF2(_L("attmask or %S must be declared !!!"), &KTUidType);
			}
		}
		
	if (dataOk)	
		{
		TInt 	err = KErrNone;
		
		if (isUidType)
			{
			err = iRDir->Open(*rfsObject, path, uidType);
			}
		else
			{
			err = iRDir->Open(*rfsObject, path, attMask);
			}
				
		if (err != KErrNone)
			{
			INFO_PRINTF2(_L("Open() error: %d"), err);
			SetError(err);
			}
		else
			{
			INFO_PRINTF1(_L("Directory has been opened!"));
			}
		}
	}
	
void CT_RDirData::DoCmdReadL(const TDesC& aSection, const TInt aAsyncErrorIndex)
	{
	INFO_PRINTF1(_L("Reading directory!"));
	
	TBool 	async 		= EFalse;
	TBool	readArray	= EFalse;
	TBool	compare		= EFalse;
	
	GET_OPTIONAL_BOOL_PARAMETER(KAsync(), aSection, async);
	
	if(GET_OPTIONAL_BOOL_PARAMETER(KReadArray(),aSection,readArray))
		{
		ReadExpectedNamesL( aSection);
		}
		
	if(GET_OPTIONAL_BOOL_PARAMETER(KCompareFiles(),aSection,compare))
		{
		iCompare = compare;
		}

	TPtrC entryArrayName;
	if (GET_OPTIONAL_STRING_PARAMETER(KArrayObjectName(), aSection, entryArrayName))
		{
		TEntryArray* entryArray = new(ELeave) TEntryArray();
		CleanupStack::PushL(entryArray);
		
		CT_EntryArrayData* entryArrayWrapper = 
			static_cast<CT_EntryArrayData*> (GetDataWrapperL(entryArrayName));
		if (entryArrayWrapper)
			{
			entryArrayWrapper->SetObjectL(entryArray);
			}
		else
			{
			ERR_PRINTF2(_L("Not initialized %S"), &entryArrayName);
			SetBlockResult(EFail);
			}
		CleanupStack::Pop(entryArray);
			
		if (async)
			{
			iObjName = entryArrayName;
			iRDir->Read(*entryArray, iReadBlock->iStatus);
			iReadBlock->Activate(aAsyncErrorIndex);
			IncOutstanding();
			}
		else
			{
			TInt err = iRDir->Read(*entryArray);
			
			if((err == KErrNone) || (err == KErrEof))
				{
				INFO_PRINTF1(_L("Read function completed successfully"));
				SetError(err);
				if(compare)
					{
					CompareEntryArray(entryArray);
					}
				}
			else
				{
				ERR_PRINTF2(_L("Read function failed with error: %d"), err);
				SetError(err);	
				}
			}
		}
	else
		{
		TPtrC entryName;
		if (GET_MANDATORY_STRING_PARAMETER(KEntryObjectName(), aSection, entryName))
			{
			TEntry* entry = new(ELeave) TEntry();
			CleanupStack::PushL(entry);
			
			CT_EntryData* entryWrapper = 
				static_cast<CT_EntryData*> (GetDataWrapperL(entryName));
			
			if (entryWrapper)
				{
				entryWrapper->SetObjectL(entry);
				}
			else
				{
				ERR_PRINTF2(_L("Not initialized %S"), &entryName);
				SetBlockResult(EFail);
				}
			
			CleanupStack::Pop(entry);
			
			if (async)
				{
				iObjName = entryName;
				delete iEntry;
				iEntry = NULL;
				iEntry = new(ELeave) TPckg<TEntry>(*entry);
				iRDir->Read(*iEntry, iRead->iStatus);
				iRead->Activate(aAsyncErrorIndex);
				IncOutstanding();
				}
			else
				{
				TInt err = iRDir->Read(*entry);
			
				if((err == KErrNone) || (err == KErrEof))
					{
					INFO_PRINTF1(_L("Read function completed successfully"));
					SetError(err);
					if(compare)
						{
						CompareEntryData(entry);
						}
					}
				else
					{
					ERR_PRINTF2(_L("Read function failed with error: %d"), err);
					SetError(err);	
					}
				}
			}
		}
	}
	
		
void CT_RDirData::ReadExpectedNamesL( const TDesC& aSection )
/**
*	Read list of expected file names from ili file
*/
	{
	iExpectedNames->ResetAndDestroy();

	TBool moreData = EFalse;
	TInt index = 0;
	do
		{
		TBuf<KBufferLength> tempStore;
		tempStore.Format(KParamExpectedFileName(), ++index);
		TPtrC fileName;
		moreData = GET_OPTIONAL_STRING_PARAMETER(tempStore, aSection, fileName);
		if (moreData)
			{
			TPath* path = new(ELeave) TPath(fileName);
			CleanupStack::PushL(path);
			iExpectedNames->AppendL(path);
			CleanupStack::Pop();
			}
		}
	while (moreData);
	}


void CT_RDirData::CompareEntryArray(TEntryArray* aEntryArray)
/**
*	Compare TEntryArray entryes with list of expected files
*/
	{
	if(aEntryArray->Count() == iExpectedNames->Count())
		{
		for(TInt i = 0; i < aEntryArray->Count(); i++)
			{
			TBool eof = EFalse;
			for(TInt k = 0; !eof && (k < iExpectedNames->Count()); k++)
				{
				TEntry* tmpEntry = new(ELeave) TEntry();
				*tmpEntry = aEntryArray->operator[](i);
				if(*(iExpectedNames->operator[](k)) == tmpEntry->iName)
					{
					TPath* name = iExpectedNames->operator[](k);
					INFO_PRINTF3( _L( "Entry name = expected name, %S = %S"), &tmpEntry->iName, name);
					
					iExpectedNames->Remove(k);
					delete name;
					eof = ETrue;	
					}
					
				delete tmpEntry;
				}
			}
		if(iExpectedNames->Count() > 0)
			{
			ERR_PRINTF1(_L("TEntryArray members are not equal to expected"));
			SetBlockResult(EFail);
			}
		else
			{
			INFO_PRINTF1( _L( "TEntryArray members are equal to expected"));
			}
		}
	else if(aEntryArray->Count() > iExpectedNames->Count())
		{
		ERR_PRINTF1(_L("Found unexpecded file(s)"));
		SetBlockResult(EFail);
		}
	else if(aEntryArray->Count() < iExpectedNames->Count())
		{
		ERR_PRINTF1(_L("List of expected files more than amount of real files"));
		SetBlockResult(EFail);
		}
	}
	
void CT_RDirData::CompareEntryData(TEntry* aEntry)
/**
*	Compare TEntry with entry in list of expected files
*/
	{
	TBool eof = EFalse;	
	TBool fileCompare = EFalse;
	for(TInt i = 0; !eof && (i < iExpectedNames->Count()); i++)
		{
		if(*(iExpectedNames->operator[](i)) == aEntry->iName)
			{
			TPath* name = iExpectedNames->operator[](i);
			fileCompare = ETrue;
				
			iExpectedNames->Remove(i);
			delete name;
			eof = ETrue;
			}
		}
	if(!fileCompare)
		{
		ERR_PRINTF2(_L("Unexpected file - %S"),&aEntry->iName);
		SetBlockResult(EFail);
		}
	}
	

void CT_RDirData::RunL(CActive* aActive, TInt aIndex)
	{	
	INFO_PRINTF1(_L("RunL Called!"));
	
	if ((aActive == iReadBlock) || (aActive == iRead))
		{

		TInt err = aActive->iStatus.Int();
		if (err == KErrNone)
			{
			TEntry	&actualEntry = iEntry->operator()();
			INFO_PRINTF2(_L("Asynchronous Read function completed successfully, %S"),&(actualEntry.iName));
			}
		else if (err == KErrEof)
			{
			INFO_PRINTF1(_L("Asynchronous Read function reached the end and returned expectedly KErrEof"));
			SetAsyncError(aIndex, err);
			}
		else
			{
			ERR_PRINTF2(_L("RunL Error: %d"), err);
			SetAsyncError(aIndex, err);
			}
		

		if(iCompare && ((err == KErrNone) || (err == KErrEof)))
			{
			if(aActive == iReadBlock)
				{
				TEntryArray *entryArray = static_cast<TEntryArray*>(GetDataObjectL(iObjName));
				if (entryArray)
					{
					CompareEntryArray(entryArray);
					}
				else
					{
					SetBlockResult(EFail);
					ERR_PRINTF2(_L("Empty Object: %S"), &iObjName);
					}
				}
			else if(aActive == iRead)
				{
				TEntry* entry = static_cast<TEntry*>(GetDataObjectL(iObjName));
				if (entry)
					{
					CompareEntryData(entry);
					}
				else
					{
					SetBlockResult(EFail);
					ERR_PRINTF2(_L("Empty Object: %S"), &iObjName);
					}
				}
			}

		delete iEntry;
		iEntry = NULL;
		
		DecOutstanding();
		}
	else
		{
 		ERR_PRINTF1( _L("Stray RunL signal") );
 		SetBlockResult( EFail );
		}
	}
	
void CT_RDirData::DoCancel(CActive* aActive, TInt aIndex)
	{
	INFO_PRINTF1(_L("DoCancel Called!"));
	
	if ( aActive == iRead )
		{	
		TInt err = aActive->iStatus.Int();
		if(err != KErrNone )
			{
			ERR_PRINTF2(_L("DoCancel Error: %d"), err);
			SetAsyncError(aIndex, err);
			}
		DecOutstanding();
		}
	else
		{
 		ERR_PRINTF1( _L("Stray RunL signal") );
 		SetBlockResult( EFail );
		}	
	}
