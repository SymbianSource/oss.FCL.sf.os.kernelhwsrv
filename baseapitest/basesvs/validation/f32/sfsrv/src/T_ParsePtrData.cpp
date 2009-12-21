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


#include "T_ParsePtrData.h"


// Commands
_LIT( KCmdDelete,					"~" );
_LIT( KCmdNew,						"new" );
_LIT( KCmdAddDir,					"AddDir");
_LIT( KCmdPopDir,					"PopDir");

// Parameters
_LIT( KParamFileName,				"FileName" );
_LIT( KParamDirName,				"DirName" );

CT_ParsePtrData* CT_ParsePtrData::NewL()
/**
* Two phase constructor
*/
	{
	CT_ParsePtrData* ret = new (ELeave) CT_ParsePtrData();
	CleanupStack::PushL( ret );
	ret->ConstructL();
	CleanupStack::Pop( ret );
	return ret;
	}

CT_ParsePtrData::CT_ParsePtrData( )
:	iParsePtr(NULL)
/**
* Protected constructor. First phase construction
*/
	{
	}

TParseBase* CT_ParsePtrData::GetParse()
	{
	return iParsePtr;
	}

void CT_ParsePtrData::ConstructL()
/**
* Protected constructor. Second phase construction
*/
	{
	}
	
CT_ParsePtrData::~CT_ParsePtrData()
/**
* Destructor.
*/
	{
	DoCleanup();
	}
	

TAny* CT_ParsePtrData::GetObject()
/**
* Return a pointer to the object that the data wraps
*
* @return pointer to the object that the data wraps
*/
	{
	return iParsePtr;
	}

TBool CT_ParsePtrData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex )
/**
* Process a command read from the ini file
*
* @param aCommand	the command to process
* @param aEntry		the entry in the ini file requiring the command to be processed
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
		DoCmdNew( aSection );
		}
	else if ( aCommand == KCmdAddDir )
		{
		DoCmdAddDir( aSection );
		}
	else if ( aCommand == KCmdPopDir )
		{
		DoCmdPopDir();
		}
	else
	    {
	    retVal = CT_ParseBaseData::DoCommandL(aCommand, aSection, aAsyncErrorIndex);    
	    }
	
	return retVal;
	}
	
void CT_ParsePtrData::DoCleanup()
/** Deletes TParsePtr class instance */
    {
    INFO_PRINTF1( _L( "Delete TParsePtr class instance." ) );
    
	delete iParsePtr;
	iParsePtr = NULL;
    }

void CT_ParsePtrData::DoCmdNew( const TDesC& aSection )
/** Creates new TParsePtr class instance */
	{
	DoCleanup();

	TPtrC fileName;
	if ( GET_MANDATORY_STRING_PARAMETER( KParamFileName, aSection, fileName ) )
		{
		INFO_PRINTF2( _L( "Create new TParsePtr(\"%S\") class instance." ), &fileName );
		
		iFileName = fileName;
		TRAPD( err, iParsePtr = new (ELeave) TParsePtr( iFileName ) );
		if ( err != KErrNone )
			{
			ERR_PRINTF3( _L( "new TParsePtr(\"%S\") error=%d" ), &fileName, err );
			SetError( err );
			}
		}
	}

 
void CT_ParsePtrData::DoCmdPopDir()
/** Removes the last directory from the path using PopDir(). */
	{
	INFO_PRINTF1( _L( "PopDir()" ) );
		
	TInt err = iParsePtr->PopDir();
	
	if( err != KErrNone)
			{
			ERR_PRINTF2( _L( "PopDir() returned %d error"), err);
			SetError(err);
			}
	}


    
void CT_ParsePtrData::DoCmdAddDir( const TDesC& aSection )
/** Adds a single directory onto the end of the path using AddDir. */
	{
	TPtrC dirName;
	if ( GET_MANDATORY_STRING_PARAMETER( KParamDirName, aSection, dirName ) )
		{
		INFO_PRINTF2( _L( "AddDir(\"%S\")" ), &dirName );
		
		TInt err = iParsePtr->AddDir( dirName );
		if(	err != KErrNone)
			{
			ERR_PRINTF2( _L( "AddDir() returned %d error"), err);
			SetError(err);
			}
		}
	}
