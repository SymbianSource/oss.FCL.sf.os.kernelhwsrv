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


#include "T_FileTextData.h"

// constants
const TInt	KReadLength		= 64;

const TBool KMandatory			= EFalse;

_LIT( KStrSeekEnd,				"ESeekEnd" );
_LIT( KStrSeekStart,			"ESeekStart" );
_LIT( KStrSeekAddress,			"ESeekAddress");
_LIT( KStrSeekCurrent,			"ESeekCurrent");
// Commands
_LIT( KCmdDestructor,			"~" );
_LIT( KCmdNew,					"new" );
_LIT( KCmdRead,					"Read" );
_LIT( KCmdSeek,					"Seek" );
_LIT( KCmdSet,					"Set" );
_LIT( KCmdWrite,				"Write" );

// Parameters
_LIT( KParamExpectedString,		"expected_str" );
_LIT( KParamObjectName,			"object_name" );
_LIT( KParamSeekMode,			"seek_mode" );
_LIT( KParamText,				"text" );
_LIT( KParamBufferLength,		"buffer_length");

CT_FileTextData* CT_FileTextData::NewL()
/**
* Two phase constructor
*/
	{
	CT_FileTextData* ret = new (ELeave) CT_FileTextData();
	CleanupStack::PushL( ret );
	ret->ConstructL();
	CleanupStack::Pop( ret );
	return ret;
	}

CT_FileTextData::CT_FileTextData():
iFileText(NULL)
/**
* Protected constructor. First phase construction
*/
	{
	}
	
void CT_FileTextData::ConstructL()
/**
* Protected constructor. Second phase construction
*/
	{
	}
	
/**
* Destructor.
*/
CT_FileTextData::~CT_FileTextData()
	{
	DoCleanup();
	}
	
TAny* CT_FileTextData::GetObject()
/**
* Return a pointer to the object that the data wraps
*
* @return pointer to the object that the data wraps
*/
	{
	return iFileText;
	}

TBool CT_FileTextData::DoCommandL( const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/ )
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
	
	if ( aCommand == KCmdDestructor )
		{
		DoCleanup();
		}
	else if ( aCommand == KCmdNew )
		{
		DoCmdNew();
		}
	else if ( aCommand == KCmdRead )
		{
		DoCmdRead( aSection );
		}
	else if ( aCommand == KCmdSeek )
		{
		DoCmdSeek( aSection );
		}
	else if ( aCommand == KCmdSet )
		{
		DoCmdSet( aSection );
		}
	else if ( aCommand == KCmdWrite )
		{
		DoCmdWrite( aSection );
		}
	else
	    {
	    retVal = EFalse;    
	    }
	
	return retVal;
	}
	
void CT_FileTextData::DoCleanup()
/**
* Deletes TFileText class instance
*/
    {
	INFO_PRINTF1( _L( "Delete TFileText class instance" ) );
	delete iFileText;
	iFileText = NULL;
	}
	
void CT_FileTextData::DoCmdNew()
/**
* Creates new TFileText class instance
*/
	{
	DoCleanup();
	
	INFO_PRINTF1( _L( "Create new TFileText class instance." ) );
	TRAPD( err, iFileText = new (ELeave) TFileText() );
	if ( err != KErrNone )
		{
		ERR_PRINTF2( _L( "new TFileText() error %d" ), err );
		SetError( err );
		}
	}
	
void CT_FileTextData::DoCmdRead( const TDesC& aSection )
/**
* Reads one line form file using Read()
*/
	{
	INFO_PRINTF1( _L( "Read one line from file." ) );
	
	RBuf readLine;
	TInt bufferLength = KReadLength;
	
	GET_OPTIONAL_INT_PARAMETER( KParamBufferLength, aSection, bufferLength );
	TInt err = readLine.Create(bufferLength);
		
	if(err == KErrNone)
		{
		err = iFileText->Read( readLine );
	
		INFO_PRINTF2( _L( "FileText::Read() result - %S" ), &readLine );
		
		if ( err != KErrNone )
			{
			ERR_PRINTF2( _L( "Function returned error %d." ), err );
			SetError( err );
			}
		else
			{
			TPtrC expectedLine;
			if ( GET_OPTIONAL_STRING_PARAMETER( KParamExpectedString, aSection, expectedLine ) )
				{
				if ( readLine.Compare(expectedLine)!=0 )
					{
					ERR_PRINTF3( _L( "Read line \"%S\", expected \"%S\"." ), &readLine, &expectedLine );
					SetBlockResult( EFail );
					}
				}
			}
		readLine.Close();
		}
	else
		{
		SetBlockResult( EFail );
		ERR_PRINTF1( _L( "RBuf initialization failed." ));
		}
	}
	
void CT_FileTextData::DoCmdSeek( const TDesC& aSection )
/**
* performs a seek to start or end of file using Seek()
*/
	{
	TSeek mode;
	if ( GetSeekMode( aSection, mode, KMandatory ) )
		{
		TInt err = iFileText->Seek( mode );
	
		if ( err != KErrNone )
			{
			ERR_PRINTF2( _L( "Function returned %d." ), err);
			SetError( err );
			}
		}
	}

void CT_FileTextData::DoCmdSet( const TDesC& aSection )
/**
* Sets the file to be read from or written to using Set()
*/
	{
	TPtrC fileObjectName;
	if ( GET_MANDATORY_STRING_PARAMETER( KParamObjectName, aSection, fileObjectName ) )
		{
		INFO_PRINTF2( _L( "Set( %S )" ), &fileObjectName );
		
		RFile* fileObject = NULL;
		TRAPD( err, fileObject = (RFile*)GetDataObjectL( fileObjectName ) );

		if ( err == KErrNone )
			{
			iFileText->Set( *fileObject );
			}
		else
			{
			ERR_PRINTF3( _L( "Unable to access object %S (error = %d)"), &fileObjectName, err );
			SetBlockResult( EFail );
			}
		
		}
	}
	
void CT_FileTextData::DoCmdWrite( const TDesC& aSection )
/**
* writes one line of text into file using Write()
*/
	{
	TPtrC writeLine;
	if ( GET_MANDATORY_STRING_PARAMETER( KParamText, aSection, writeLine ) )
		{
		
		INFO_PRINTF2( _L( "Write \"%S\" into file" ), &writeLine );
		TInt err = iFileText->Write( writeLine );
		if ( err != KErrNone)
			{
			ERR_PRINTF2( _L( "Function returned %d." ), err);
			SetError( err );
			}
		}
	}

	

TBool CT_FileTextData::GetSeekMode( const TDesC& aSection, TSeek& aMode, TBool aOptional )
/** 
* retrieves "seek_mode" parameter value and converts it into its TSeek representation
* @param aSection 	- the entry in the ini file requiring the command to be processed
* @param aMode		- the returned TSeek representation of "seek_mode" command parameter
* @param aOptional	- represents the function which is called to retrieve the value
*					KOptional	- GET_OPTIONAL_STRING_PARAMETER is called
*					KMandatory	- GET_MANDATORY_STRING_PARAMETER is called
*
* @return ETrue if "seek_mode" parameter is present and has been sucessfully converted
*/
	{
	TBool result = ETrue;
	
	TPtrC strSeekMode;
	
	// Get "seek_mode" command parameter string value
	if ( aOptional )
		{
		result = GET_OPTIONAL_STRING_PARAMETER( KParamSeekMode, aSection, strSeekMode );
		}
	else
		{
		result = GET_MANDATORY_STRING_PARAMETER( KParamSeekMode, aSection, strSeekMode );
		}
	
	// Convert it into TSeek representation
	if ( result )
		{
		if ( strSeekMode == KStrSeekEnd )
			{
			aMode = ESeekEnd;
			}
		else if ( strSeekMode == KStrSeekStart )
			{
			aMode = ESeekStart;
			}
		else if ( strSeekMode == KStrSeekAddress )
			{
			aMode = ESeekAddress;
			}
		else if ( strSeekMode == KStrSeekCurrent )
			{
			aMode = ESeekCurrent;
			}
		else
			{
			ERR_PRINTF2( _L( "Unrecognized seek_mode value: %S" ), &strSeekMode );
			result = EFalse;
			}
		}
			
	return result;
	}
 
 
