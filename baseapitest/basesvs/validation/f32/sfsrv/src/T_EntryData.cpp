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


#include "T_EntryData.h"
#include "T_SfSrvServer.h"
#include "FileserverUtil.h"

// Commands
_LIT( KCmdAssignmentOperator,	"=" );
_LIT( KCmdIndexOperator,		"[]" );
_LIT( KCmdDelete,				"~" );
_LIT( KCmdIsArchive,			"IsArchive" );
_LIT( KCmdIsDir,				"IsDir" );
_LIT( KCmdIsHidden,				"IsHidden" );
_LIT( KCmdIsReadOnly,			"IsReadOnly" );
_LIT( KCmdIsSystem,				"IsSystem" );
_LIT( KCmdIsTypeValid,			"IsTypeValid" );
_LIT( KCmdIsUidPresent,			"IsUidPresent" );
_LIT( KCmdMostDerivedUid,		"MostDerivedUid" );
_LIT( KCmdNew,					"new" );
_LIT( KCmdSetAttribute,			"SetAttribute" );

// Parameters
_LIT( KParamAttribute,			"attribute" );
_LIT( KParamObject,				"object" );
_LIT( KExpected,				"expected" );
_LIT( KParamIndex,				"index" );
_LIT( KParamState,				"state" );
_LIT( KValue,					"value" );

// Attributes
_LIT(KEntryAttNormalStr, 		"KEntryAttNormal");
_LIT(KEntryAttReadOnlyStr, 		"KEntryAttReadOnly");
_LIT(KEntryAttHiddenStr, 		"KEntryAttHidden");
_LIT(KEntryAttSystemStr, 		"KEntryAttSystem");
_LIT(KEntryAttVolumeStr,		"KEntryAttVolume");
_LIT(KEntryAttDirStr, 			"KEntryAttDir");
_LIT(KEntryAttArchiveStr, 		"KEntryAttArchive");
_LIT(KEntryAttXIPStr, 			"KEntryAttXIP");


// other string values
#define KTimeFormatSize 		30


CT_EntryData* CT_EntryData::NewL( )
/**
* Two phase constructor
*/
	{
	CT_EntryData* ret = new (ELeave) CT_EntryData( );
	CleanupStack::PushL( ret );
	ret->ConstructL();
	CleanupStack::Pop( ret );
	return ret;
	}

CT_EntryData::CT_EntryData( )
:	iEntry(NULL)
,	iFs(NULL)	

/**
* Protected constructor. First phase construction
*/
	{
	}
	
void CT_EntryData::ConstructL()
/**
* Protected constructor. Second phase construction
*/
	{
	}
	
CT_EntryData::~CT_EntryData()
/**
* Destructor.
*/
	{
	DoCleanup();
	}
	
void CT_EntryData::DoCleanup()
/** Deltes TEntry class instance */
    {
	INFO_PRINTF1( _L( "Delete TEntry class instance" ) );
	delete iEntry;
	iEntry = NULL;
    }
    
TAny* CT_EntryData::GetObject()
/**
* Return a pointer to the object that the data wraps
*
* @return pointer to the object that the data wraps
*/
	{
	return iEntry;
	}

void CT_EntryData::SetObjectL( TAny* aAny )
/**
* Set the wrapped data object with new value
*/
	{
	DoCleanup();
	iEntry = static_cast<TEntry*> ( aAny );
	}

void CT_EntryData::DisownObjectL()
/**
* Clear the wrapped data object pointer w/o de-initialization
*/
	{
	iEntry = NULL;
	}
	
inline TCleanupOperation CT_EntryData::CleanupOperation()
/**
* Return static cleanup function
*/
	{
	return CleanupOperation;
	}

void CT_EntryData::CleanupOperation( TAny* aAny )
/**
* Static cleanup function
*/
	{
	TEntry* entry = static_cast<TEntry*> ( aAny );
	delete entry;
	}

TBool CT_EntryData::DoCommandL( const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/)
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
	
	if ( aCommand == KCmdAssignmentOperator )
		{
		DoCmdAssignmentOperatorL( aSection );
		}
	else if ( aCommand == KCmdIndexOperator )
		{
		DoCmdIndexOperator( aSection );
		}
	else if ( aCommand == KCmdDelete )
		{
		DoCleanup();
		}
	else if ( aCommand == KCmdIsArchive )
		{
		DoCmdIsArchive( aSection );
		}
	else if ( aCommand == KCmdIsDir )
		{
		DoCmdIsDir( aSection );
		}
	else if ( aCommand == KCmdIsHidden )
		{
		DoCmdIsHidden( aSection );
		}
	else if ( aCommand == KCmdIsReadOnly )
		{
		DoCmdIsReadOnly( aSection );
		}
	else if ( aCommand == KCmdIsSystem )
		{
		DoCmdIsSystem( aSection );
		}
	else if ( aCommand == KCmdIsTypeValid )
		{
		DoCmdIsTypeValid( aSection );
		}
	else if ( aCommand == KCmdIsUidPresent )
		{
		DoCmdIsUidPresent( aSection );
		}
	else if ( aCommand == KCmdMostDerivedUid )
		{
		DoCmdMostDerived( aSection );
		}
	else if ( aCommand == KCmdNew )
		{
		DoCmdNew( aSection );
		}
	else if ( aCommand == KCmdSetAttribute )
		{
		DoCmdSetAttribute( aSection );
		}
	else
	    {
	    retVal = EFalse;    
	    }
	
	return retVal;
	}

void CT_EntryData::DoCmdNew( const TDesC& aSection )
/** Creates new TEntry class instance */
	{
	DoCleanup();
	
	TPtrC entryObjectName;	
	if( GET_OPTIONAL_STRING_PARAMETER( KParamObject, aSection, entryObjectName ) )
		{
		INFO_PRINTF1( _L( "Create new TEntry(TEntry) class instance." ) );
		
		TEntry* entryObject = NULL;
		TRAPD( err, entryObject = (TEntry*)GetDataObjectL(entryObjectName));
		if ( err == KErrNone )
			{
			TRAPD( err, iEntry = new (ELeave) TEntry(*entryObject) );
			if ( err != KErrNone )
				{
				ERR_PRINTF2( _L( "new TEntry(TEntry) error %d" ), err );
				SetError( err );
				}
			}
		else
			{
			ERR_PRINTF3( _L( "Unrecognized object name parameter value: %S. Error %d"), &entryObjectName, err );
			SetBlockResult( EFail );
			}
		}
	else
		{
		INFO_PRINTF1( _L( "Create new TEntry() class instance." ) );
		TRAPD( err, iEntry = new (ELeave) TEntry() );
		if ( err!=KErrNone )
			{
			ERR_PRINTF2( _L( "new TEntry() error %d" ), err );
			SetError( err );
			}
		}
	}
	
void CT_EntryData::DoCmdIndexOperator( const TDesC& aSection )
/** Checks if the given UID is the same as required */
	{
	TInt iniIndex;	
	if ( GET_MANDATORY_INT_PARAMETER( KParamIndex, aSection, iniIndex ) )
		{
		TInt value = (*iEntry)[iniIndex].iUid;
		INFO_PRINTF3( _L( "UID[%d] = %d" ), iniIndex, value );		
		
		TInt iniExpected;
		if ( GET_OPTIONAL_INT_PARAMETER( KExpected, aSection, iniExpected ) )
			{			
			if ( value != iniExpected )
				{
				INFO_PRINTF3( _L( "values not the same %d != %d" ), value, iniExpected );
				SetBlockResult( EFail );
				}
			}
		}
	}
	
void CT_EntryData::DoCmdAssignmentOperatorL( const TDesC& aSection )
/** Assigns another TEntry to this TEntry using "=" operator */
	{
	TPtrC entryObjectName;
	if( GET_MANDATORY_STRING_PARAMETER( KParamObject, aSection, entryObjectName ) )
		{
		INFO_PRINTF2( _L( "Assign a %S to this TEntry" ), &entryObjectName );
		
		TEntry* entryObject = NULL;
		TRAPD( err, entryObject = (TEntry*)GetDataObjectL(entryObjectName));
		if ( err == KErrNone && entryObject)
			{
			*iEntry = iEntry->operator=(*entryObject);
			}
		else
			{
			ERR_PRINTF3( _L( "Object not found or not initialised: %S. Error %d"), &entryObjectName, err );
			SetBlockResult( EFail );
			}
		}

	if ( !FileserverUtil::VerifyTEntryDataFromIniL(*this, aSection, *iEntry))
		{
		SetBlockResult(EFail);
		}
	}
	
void CT_EntryData::DoCmdIsArchive( const TDesC& aSection )
/** Checks if TEntry has Archive attribute using IsArchive() function */
	{
	INFO_PRINTF1( _L( "IsArchive()" ) );
	TBool result = iEntry->IsArchive();
		
	TBool expected;
	if( GET_MANDATORY_BOOL_PARAMETER( KExpected, aSection, expected ) )
		{
		if ( !( CompareBool(result, expected) ) )
			{
			ERR_PRINTF1( _L( "Error compare function result and expected value" ));
			SetBlockResult(EFail);
			}
		}
	}
	
void CT_EntryData::DoCmdIsDir( const TDesC& aSection )
/** Checks if TEntry has Dir attribute using IsDir() function */
	{
	INFO_PRINTF1( _L( "IsDir()" ) );
	TBool result = iEntry->IsDir();
						
	TBool expected;
	if( GET_MANDATORY_BOOL_PARAMETER( KExpected, aSection, expected ) )
		{
		if ( !( CompareBool(result, expected) ) )
			{
			ERR_PRINTF1( _L( "Error compare function result and expected value" ));
			SetBlockResult(EFail);
			}
		}
	}
	
void CT_EntryData::DoCmdIsHidden( const TDesC& aSection )
/** Checks if TEntry has Hidden attribute using IsHidden() function */
	{
	INFO_PRINTF1( _L( "IsHidden()" ) );
	TBool result = iEntry->IsHidden();
		
	TBool expected;
	if( GET_MANDATORY_BOOL_PARAMETER( KExpected, aSection, expected ) )
		{
		if ( !( CompareBool(result, expected) ) )
			{
			ERR_PRINTF1( _L( "Error compare function result and expected value" ));
			SetBlockResult(EFail);
			}
		}
	}

void CT_EntryData::DoCmdIsReadOnly( const TDesC& aSection )
/** Checks if TEntry has ReadOnly attribute using IsReadOnly() function */
	{
	INFO_PRINTF1( _L( "IsReadOnly()" ) );
	TBool result = iEntry->IsReadOnly();
		
	TBool expected;
	if( GET_MANDATORY_BOOL_PARAMETER( KExpected, aSection, expected ) )
		{
		if ( !( CompareBool(result, expected) ) )
			{
			ERR_PRINTF1( _L( "Error compare function result and expected value" ));
			SetBlockResult(EFail);
			}
		}
	}

void CT_EntryData::DoCmdIsSystem( const TDesC& aSection )
/** Checks if TEntry has System attribute using IsSystem() function */
	{
	INFO_PRINTF1( _L( "IsSystem()" ) );
	TBool result = iEntry->IsSystem();
	
	TBool expected;
	if( GET_MANDATORY_BOOL_PARAMETER( KExpected, aSection, expected ) )
		{
		if ( !( CompareBool(result, expected) ) )
			{
			ERR_PRINTF1( _L( "Error compare function result and expected value" ));
			SetBlockResult(EFail);
			}
		}
	}

void CT_EntryData::DoCmdIsTypeValid( const TDesC& aSection )
/** Checks if TEntry has a valid UID using IsTypeValid() function */
	{
	INFO_PRINTF1( _L( "IsTypeValid()" ) );
	TBool result = iEntry->IsTypeValid();
		
	TBool expected;
	if( GET_MANDATORY_BOOL_PARAMETER( KExpected, aSection, expected ) )
		{
		if ( !( CompareBool(result, expected) ) )
			{
			ERR_PRINTF1( _L( "Error compare function result and expected value" ));
			SetBlockResult(EFail);
			}
		}
	}
	
void CT_EntryData::DoCmdIsUidPresent( const TDesC& aSection )
/** Checks if TEntry has a valid UID using IsTypeValid() function */
	{
	TInt theIntUid;
	TBool expected;
	if( GET_MANDATORY_BOOL_PARAMETER( KExpected, aSection, expected ) &&
		GET_MANDATORY_INT_PARAMETER( KValue, aSection, theIntUid ) )
		{
		INFO_PRINTF2( _L( "IsUidPresent( Uid(%d) )" ), theIntUid );
		TUid theUid;
		theUid.iUid = theIntUid;
		
		TBool result = iEntry->IsUidPresent( theUid );
		
		if ( !( CompareBool(result, expected) ) )
			{
			ERR_PRINTF1( _L( "Error compare function result and expected value" ));
			SetBlockResult(EFail);
			}
		}
	}

void CT_EntryData::DoCmdMostDerived( const TDesC& aSection )
/** checks if UID with given index is the same as the most derived one by using MostDerived() function */
	{
	TInt id = iEntry->MostDerivedUid().iUid;		
	INFO_PRINTF2( _L( "MostDerivedUid = %d" ), id);
		
	TInt expected;
	if( GET_OPTIONAL_INT_PARAMETER( KExpected, aSection, expected ) )
		{		
		if ( expected  != id )
			{
			ERR_PRINTF3( _L( "Error compare UID %d != expected %d" ), id, expected );
			SetBlockResult( EFail );
			}
		}
	}
	
void CT_EntryData::DoCmdSetAttribute( const TDesC& aSection )
/** Sets or resets attributes */
	{
	TPtrC attrName;
	TBool attrState;	
	if( GET_MANDATORY_BOOL_PARAMETER( KParamState, aSection, attrState ) &&
		GET_MANDATORY_STRING_PARAMETER( KParamAttribute, aSection, attrName ) )
		{
		TBool errHappened = EFalse;
		TUint attr;
		if (attrName == KEntryAttNormalStr)
			{
			attr = KEntryAttNormal;
			}
		else if (attrName == KEntryAttReadOnlyStr)
			{
			attr = KEntryAttReadOnly;
			}
		else if (attrName == KEntryAttHiddenStr)
			{
			attr = KEntryAttHidden;
			}
		else if (attrName == KEntryAttSystemStr)
			{
			attr = KEntryAttSystem;
			}
		else if (attrName == KEntryAttVolumeStr)
			{
			attr = KEntryAttVolume;
			}
		else if (attrName == KEntryAttDirStr)
			{
			attr = KEntryAttDir;
			}
		else if (attrName == KEntryAttArchiveStr)
			{
			attr = KEntryAttArchive;
			}
		else if (attrName == KEntryAttXIPStr)
			{
			attr = KEntryAttXIP;
			}
		else
			{
			errHappened = ETrue;
			}
			
		if ( errHappened )
			{
			ERR_PRINTF2( _L( "Unknown attribute name parameter: %S" ), &attrName );
			SetBlockResult( EFail );
			}
		else
			{
			TPtrC attrStateString;
			GET_MANDATORY_STRING_PARAMETER( KParamState, aSection, attrStateString );
			INFO_PRINTF3( _L( "SetAttribute(%S, %S)" ),  &attrName, &attrStateString);
			
			if ( attrState ) // set attribute
				{
				iEntry->iAtt = iEntry->iAtt | attr;
				}
			else // reset attribute
				{
				iEntry->iAtt = iEntry->iAtt ^ ( iEntry->iAtt & attr );
				}
			}
		}
	}


TBool CT_EntryData::CompareBool( TBool aResult, TBool aExpected )
/** compare 2 TBool values, which can be !=0 and !=1 - some TEntry functions return 2, 3 etc */
	{
	return ( ( aResult > 0 ) == ( aExpected > 0 ) );
	}

