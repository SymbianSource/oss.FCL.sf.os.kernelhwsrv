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


#include "T_EntryArrayData.h"
#include "T_EntryData.h"
#include "FileserverUtil.h"

// Commands
_LIT( KCmdCount,				"count" );
_LIT( KCmdDelete,				"~" );
_LIT( KCmdNew,					"new" );
_LIT( KCmdIndexOperator,		"[]" );

// Parameters
_LIT( KParamDestination,		"destination" );
_LIT( KParamExpected,			"expected" );
_LIT( KParamIndex,				"index" );


CT_EntryArrayData* CT_EntryArrayData::NewL()
/**
* Two phase constructor
*/
	{
	CT_EntryArrayData* ret = new (ELeave) CT_EntryArrayData();
	CleanupStack::PushL( ret );
	ret->ConstructL();
	CleanupStack::Pop( ret );
	return ret;
	}

CT_EntryArrayData::CT_EntryArrayData()
:	iEntryArray(NULL)
/**
* Protected constructor. First phase construction
*/
	{
	}
	
void CT_EntryArrayData::ConstructL()
/**
* Protected constructor. Second phase construction
*/
	{
	}
	
CT_EntryArrayData::~CT_EntryArrayData()
/**
* Destructor.
*/

	{
	DoCmdDelete();
	}
	
TAny* CT_EntryArrayData::GetObject()
/**
* Return a pointer to the object that the data wraps
*
* @return pointer to the object that the data wraps
*/

	{
	return iEntryArray;
	}

TBool CT_EntryArrayData::DoCommandL( const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/ )
/**
* Process a command read from the ini file
*
* @param aCommand	the command to process
* @param aSection		the entry in the ini file requiring the command to be processed
*
* @return ETrue if the command is processed
*/	
	{
	TBool retVal = ETrue;
	
	if ( aCommand == KCmdIndexOperator )
		{
		DoCmdIndexOperatorL( aSection );
		}
	else if ( aCommand == KCmdCount )
		{
		DoCmdCount( aSection );
		}
	else if ( aCommand == KCmdDelete )
		{
		DoCmdDelete();
		}
	else if ( aCommand == KCmdNew )
		{
		DoCmdNew();
		}
	else
	    {
	    retVal = EFalse;    
	    }
	
	return retVal;
	}
	
void CT_EntryArrayData::DoCmdIndexOperatorL( const TDesC& aSection )
/**
* Assigns an element of TEntryArray to another TEntry object using "=" operator
*/
	{
	TInt index;
	TPtrC entryObjectName;
	
	if( GET_MANDATORY_INT_PARAMETER( KParamIndex, aSection, index ) && 
		GET_MANDATORY_STRING_PARAMETER( KParamDestination, aSection, entryObjectName ))
		{
		TEntry* entry = new(ELeave) TEntry();
		CleanupStack::PushL(entry);
		
		*entry = iEntryArray->operator[](index);
		CT_EntryData* entryWrapperObject = static_cast<CT_EntryData*>(GetDataWrapperL(entryObjectName));
		
		if(!entryWrapperObject)
		    {
			ERR_PRINTF2(_L("%S is not initialised"), &entryObjectName);
		    SetBlockResult(EFail); 
		    CleanupStack::PopAndDestroy(entry);
		    }
		else
			{
			entryWrapperObject->SetObjectL(entry);
			CleanupStack::Pop(entry);
			}
		}
	}
	
void CT_EntryArrayData::DoCmdCount( const TDesC& aSection )
/**
* Checks if TEntryArray has expected number of elements
*/
	{
	TInt expected;	
	if( GET_MANDATORY_INT_PARAMETER( KParamExpected, aSection, expected ) )
		{
		INFO_PRINTF2( _L( "Count(). Expected value = %d" ), expected );
		
		TInt result = iEntryArray->Count();
		if ( result != expected )
			{
			ERR_PRINTF3(_L("Count %d != expected %d"), result, expected );
			SetBlockResult( EFail );
			}
		}
	}
	
void CT_EntryArrayData::DoCmdDelete()
/**
* Deletes TEntryArray class instance
*/
    {
	DoCleanup();
    }
	
void CT_EntryArrayData::DoCmdNew()
/**
* Creates new TEntryArray class instance
*/
	{
	DoCmdDelete();
	
	INFO_PRINTF1( _L( "Create new TEntryArray() class instance." ) );
	TRAPD( err, iEntryArray = new (ELeave) TEntryArray() );
	if ( err != KErrNone )
		{
		ERR_PRINTF2( _L( "new TEntryArray() error %d" ), err );
		SetBlockResult( EFail );
		}
	}
	
	
void CT_EntryArrayData::SetObjectL( TAny* aAny )
/**
* Set the wrapped data object with new value
*/
	{
	DoCleanup();
	iEntryArray = static_cast<TEntryArray*> ( aAny );
	}
	
void CT_EntryArrayData::DoCleanup()
/** Deltes TEntry class instance */
    {
	INFO_PRINTF1( _L( "Delete TEntryArray class instance" ) );
	delete iEntryArray;
	iEntryArray = NULL;
    }

	
