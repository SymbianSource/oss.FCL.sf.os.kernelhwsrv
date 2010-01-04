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


#include "T_ParseData.h"


// Commands
_LIT( KCmdDelete,					"~" );
_LIT( KCmdAddDir,					"AddDir");
_LIT( KCmdPopDir,					"PopDir");
_LIT( KCmdNew,						"new" );
_LIT( KCmdSet,						"Set" );
_LIT( KCmdSetNoWild,				"SetNoWild" );
// Parameters
_LIT( KParamDefaultSpec,			"DefaultSpec" );
_LIT( KParamFileSpec,				"FileName" );
_LIT( KParamRelatedSpec,			"RelatedSpec" );
_LIT( KParamDirName,				"DirName" );


CT_ParseData* CT_ParseData::NewL()
/**
* Two phase constructor
*/
	{
	CT_ParseData* ret = new (ELeave) CT_ParseData();
	CleanupStack::PushL( ret );
	ret->ConstructL();
	CleanupStack::Pop( ret );
	return ret;
	}

TParseBase* CT_ParseData::GetParse()
	{
	return iParse;
	}
	

CT_ParseData::CT_ParseData()
:	iParse(NULL)
/**
* Protected constructor. First phase construction
*/
	{
	}
	
void CT_ParseData::ConstructL()
/**
* Protected constructor. Second phase construction
*/
	{
	}
	
CT_ParseData::~CT_ParseData()
/**
* Destructor.
*/
	{
	DoCleanup();
	}
	

TAny* CT_ParseData::GetObject()
/**
* Return a pointer to the object that the data wraps
*
* @return pointer to the object that the data wraps
*/
	{
	return iParse;
	}

TBool CT_ParseData::DoCommandL( const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex)
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
		
	if ( aCommand == KCmdDelete )
		{
		DoCleanup();
		}
	else if ( aCommand == KCmdNew )
		{
		DoCmdNew();
		}
	else if ( aCommand == KCmdSet )
		{
		DoCmdSet( aSection );
		}
	else if ( aCommand == KCmdSetNoWild )
		{
		DoCmdSetNoWild( aSection );
		}
	else if ( aCommand == KCmdPopDir )
		{
		DoCmdPopDir();
		}
	else if ( aCommand == KCmdAddDir )
		{
		DoCmdAddDir( aSection );
		}
	else
	    {
	    retVal = CT_ParseBaseData::DoCommandL(aCommand,aSection,aAsyncErrorIndex);
	    }
	
	return retVal;
	}
	
void CT_ParseData::DoCleanup()
/** Deletes TParse class instance */
    {
    INFO_PRINTF1( _L( "Delete TParse class instance." ) );
    
	delete iParse;
	iParse = NULL;
    }

void CT_ParseData::DoCmdNew()
/** Creates new TParse class instance */
	{
	DoCleanup();
	
	INFO_PRINTF1( _L( "Create new TParse class instance." ) );
		
	TRAPD( err, iParse = new (ELeave) TParse() );
	if ( err != KErrNone )
		{
		ERR_PRINTF2( _L( "new TParse error=%d" ), err );
		SetError( err );
		}
	}
	
void CT_ParseData::DoCmdSet( const TDesC& aSection )
	{
	TPtrC fileSpec;
	if ( GET_MANDATORY_STRING_PARAMETER( KParamFileSpec, aSection, fileSpec ) )
		{
		TPtrC relatedSpec(_L(""));
		GET_OPTIONAL_STRING_PARAMETER( KParamRelatedSpec, aSection, relatedSpec );
		
		TPtrC defaultSpec(_L(""));
		GET_OPTIONAL_STRING_PARAMETER( KParamDefaultSpec, aSection, defaultSpec );
		
		INFO_PRINTF4( _L( "Set(%S, %S, %S)"), &fileSpec, &relatedSpec, &defaultSpec );
		
		TInt error = iParse->Set( fileSpec, &relatedSpec, &defaultSpec );
		
		if ( error != KErrNone)
			{
			ERR_PRINTF2( _L( "Set() returned %d error"), error);
			SetError(error);
			}
		}
	}
	
void CT_ParseData::DoCmdSetNoWild( const TDesC& aSection )
	{
	TPtrC fileSpec;
	if ( GET_MANDATORY_STRING_PARAMETER( KParamFileSpec, aSection, fileSpec ) )
		{
		TPtrC relatedSpec(_L(""));
		GET_OPTIONAL_STRING_PARAMETER( KParamRelatedSpec, aSection, relatedSpec );
		
		TPtrC defaultSpec(_L(""));
		GET_OPTIONAL_STRING_PARAMETER( KParamDefaultSpec, aSection, defaultSpec );
		
		INFO_PRINTF4( _L( "SetNoWild(%S, %S, %S)"), &fileSpec, &relatedSpec, &defaultSpec );
		
		TInt error = iParse->SetNoWild( fileSpec, &relatedSpec, &defaultSpec );
	
		if ( error != KErrNone)
			{
			ERR_PRINTF2( _L( "SetNoWild() returned %d error"), error);
			SetError(error);
			}
		}
	}
	
	
void CT_ParseData::DoCmdPopDir()
/** Removes the last directory from the path using PopDir(). */
	{
	INFO_PRINTF1( _L( "PopDir()" ) );

	TInt error = iParse->PopDir();
	if( error != KErrNone)
		{
		ERR_PRINTF2( _L( "PopDir() returned %d error"), error);
		SetError(error);
		}
	}
	
    
void CT_ParseData::DoCmdAddDir( const TDesC& aSection )
/** Adds a single directory onto the end of the path using AddDir. */
	{
	TPtrC dirName;
	if ( GET_MANDATORY_STRING_PARAMETER( KParamDirName, aSection, dirName ) )
		{
		INFO_PRINTF2( _L( "AddDir(\"%S\")" ), &dirName );
		
		TInt error = iParse->AddDir( dirName );
		if(	error != KErrNone)
			{
			ERR_PRINTF2( _L( "AddDir() returned %d error"), error);
			SetError(error);
			}
		}
	}
	
