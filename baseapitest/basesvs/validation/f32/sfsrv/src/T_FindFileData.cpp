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


#include "T_FindFileData.h"

// Commands
_LIT( KCmdDelete,				"~" );
_LIT( KCmdNew,					"new" );
_LIT( KCmdFile,					"File" );
_LIT( KCmdFind,					"Find" );
_LIT( KCmdFindByDir, 			"FindByDir" );
_LIT( KCmdFindByPath,			"FindByPath" );
_LIT( KCmdFindWild,				"FindWild" );
_LIT( KCmdFindWildByDir,		"FindWildByDir" );
_LIT( KCmdFindWildByPath,		"FindWildByPath" );
_LIT( KCmdSetFindMask,			"SetFindMask" );

// Parameters
_LIT( KParamFileName,			"filename" );
_LIT( KParamFileSession,		"FileSession" );
_LIT( KParamExpected,			"expected" );
_LIT( KParamPathList,			"pathlist" );
_LIT( KDriveAtt,				"drive_att");
_LIT( KDirWrapper,				"dir_wrapper");

// Utility variables
_LIT(KDriveAttLocalStr,				"KDriveAttLocal");
_LIT(KDriveAttRomStr,				"KDriveAttRom");
_LIT(KDriveAttRedirectedStr,		"KDriveAttRedirected");
_LIT(KDriveAttSubstedStr,			"KDriveAttSubsted");
_LIT(KDriveAttInternalStr,			"KDriveAttInternal");
_LIT(KDriveAttRemovableStr,			"KDriveAttRemovable");


CT_FindFileData* CT_FindFileData::NewL()
/**
* Two phase constructor
*/
	{
	CT_FindFileData* ret = new (ELeave) CT_FindFileData();
	CleanupStack::PushL( ret );
	ret->ConstructL();
	CleanupStack::Pop( ret );
	return ret;
	}

CT_FindFileData::CT_FindFileData()
:	iFindFile(NULL)
/**
* Protected constructor. First phase construction
*/
	{
	}
	
void CT_FindFileData::ConstructL()
/**
* Protected constructor. Second phase construction
*/
	{
	}
	
CT_FindFileData::~CT_FindFileData()
/**
* Destructor.
*/
	{
	DoCleanup();
	}
	
TAny* CT_FindFileData::GetObject()
/**
* Return a pointer to the object that the data wraps
*
* @return pointer to the object that the data wraps
*/
	{
	return iFindFile;
	}

TBool CT_FindFileData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/  )
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
	else if ( aCommand == KCmdFile )
		{
		DoCmdFile( aSection );
		}
	else if ( aCommand == KCmdFind )
		{
		DoCmdFind( aSection );
		}
	else if ( aCommand == KCmdFindByDir )
		{
		DoCmdFindByDir( aSection );
		}
	else if ( aCommand == KCmdFindByPath )
		{
		DoCmdFindByPath( aSection );
		}
	else if ( aCommand == KCmdFindWild )
		{
		DoCmdFindWildL( aSection );
		}
	else if ( aCommand == KCmdFindWildByDir )
		{
		DoCmdFindWildByDirL( aSection );
		}
	else if ( aCommand == KCmdFindWildByPath )
		{
		DoCmdFindWildByPathL( aSection );
		}
	else if ( aCommand == KCmdNew )
		{
		DoCmdNew( aSection);
		}
	else if ( aCommand == KCmdSetFindMask )
		{
		DoCmdSetFindMask( aSection);
		}
	else
	    {
	    retVal = EFalse;    
	    }
	
	return retVal;
	}
	
void CT_FindFileData::DoCleanup()
/** Deletes TFindFile class instance */
	{
	INFO_PRINTF1( _L( "Delete TFindFile class instance" ) );
	delete iFindFile;
	iFindFile = NULL;
	}
	
void CT_FindFileData::DoCmdFile( const TDesC& aSection )
/** calls File() and comaperes the result with the expected */
	{
	TPtrC strResult;
	strResult.Set( iFindFile->File() );
	INFO_PRINTF2( _L( "File(): %S" ),  &strResult );
	
	TPtrC strExpected;
	if ( GET_OPTIONAL_STRING_PARAMETER (KParamExpected, aSection, strExpected) )
		{
		if ( strExpected.CompareF(strResult) )
			{
			ERR_PRINTF3( _L( "File returned %S, expected %S") , &strResult, &strExpected );
			SetBlockResult( EFail );
			}
		}
	}
	
void CT_FindFileData::DoCmdFind( const TDesC& /*aSection*/ )
/** performs a search of a file using Find */
	{
	TInt result = iFindFile->Find();
	
	if( result != KErrNone)
		{
		ERR_PRINTF2( _L( "Find() return error = %d"), result );
		SetError(result);
		}
	}
	
void CT_FindFileData::DoCmdFindByDir( const TDesC& aSection )
/** performs a search of a file using FindByDir */
	{
	TPtrC fileName;	
	if( GET_MANDATORY_STRING_PARAMETER( KParamFileName, aSection, fileName ) &&
		GET_MANDATORY_STRING_PARAMETER( KParamPathList, aSection, iPathList ) )
		{
		TInt err = iFindFile->FindByDir( fileName, iPathList );
		
		if( err != KErrNone)
			{
			ERR_PRINTF2( _L( "FindByDir() return error = %d"), err );
			SetError(err);
			}		
		}
	}

void CT_FindFileData::DoCmdFindByPath( const TDesC& aSection )
/** performs a search of a file using FindByPath */
	{
	TPtrC fileName;	
	if( GET_MANDATORY_STRING_PARAMETER( KParamFileName, aSection, fileName ) &&
		GET_MANDATORY_STRING_PARAMETER( KParamPathList, aSection, iPathList ) )
		{
		TInt err = iFindFile->FindByPath( fileName, &iPathList );
		
		if( err != KErrNone)
			{
			ERR_PRINTF2( _L( "FindByPath() return error = %d"), err );
			SetError(err);
			}
		
		}
	
	}
	
void CT_FindFileData::DoCmdFindWildL( const TDesC& aSection)
/** performs a search of a file using FindWild */
	{
	TPtrC dirWrapperName;
	if (GET_MANDATORY_STRING_PARAMETER(KDirWrapper, aSection, dirWrapperName))
		{
		CT_DirData *dirWrapper = NULL;
		TRAPD( error, dirWrapper = static_cast<CT_DirData*>(GetDataWrapperL(dirWrapperName)))
		
		if( dirWrapper && (error == KErrNone) )
			{
			CDir* dir = NULL;
			TInt err = iFindFile->FindWild( dir );
		
			if( err != KErrNone)
				{
				ERR_PRINTF2( _L( "FindWild() return error = %d"), err );
				SetError(err);
				}
			else
				{
				dirWrapper->SetObjectL(dir);
				}
			}
		else
			{
			ERR_PRINTF2( _L( "Wrong CDir wrapper name %S"), &dirWrapperName );
			SetBlockResult( EFail );
			}
		}
	}
	
void CT_FindFileData::DoCmdFindWildByDirL( const TDesC& aSection )
/** performs a search of a file using FindWildByDir */
	{
	TPtrC fileName;	
	TPtrC dirWrapperName;
	if( GET_MANDATORY_STRING_PARAMETER( KParamFileName, aSection, fileName ) &&
		GET_MANDATORY_STRING_PARAMETER( KParamPathList, aSection, iPathList ) &&
		GET_MANDATORY_STRING_PARAMETER( KDirWrapper, aSection, dirWrapperName) )
		{
		CT_DirData *dirWrapper = NULL;
		TRAPD( error, dirWrapper = static_cast<CT_DirData*>(GetDataWrapperL(dirWrapperName)))
		
		if(dirWrapper && (error == KErrNone) )
			{	
			CDir* dir = NULL;
			TInt err = iFindFile->FindWildByDir( fileName, iPathList, dir );
			
			if( err != KErrNone)
				{
				ERR_PRINTF2( _L( "FindWildByDir() return error = %d"), err );
				SetError(err);
				}
			else
				{
				dirWrapper->SetObjectL(dir);
				}
			}
		else
			{
			ERR_PRINTF2( _L( "Wrong CDir wrapper name %S"), &dirWrapperName );
			SetBlockResult( EFail );
			}
		}
	}
	
void CT_FindFileData::DoCmdFindWildByPathL( const TDesC& aSection )
/** performs a search of a file using FindWildByPath */
	{
	TPtrC fileName;	
	TPtrC dirWrapperName;
	if( GET_MANDATORY_STRING_PARAMETER( KParamFileName, aSection, fileName ) &&
		GET_MANDATORY_STRING_PARAMETER( KParamPathList, aSection, iPathList ) &&
		GET_MANDATORY_STRING_PARAMETER( KDirWrapper, aSection, dirWrapperName) )
		{
		CT_DirData *dirWrapper = NULL;
		TRAPD( error, dirWrapper = static_cast<CT_DirData*>(GetDataWrapperL(dirWrapperName)))
		
		if( dirWrapper && (error == KErrNone))
			{
			CDir* dir = NULL;
			TInt err = iFindFile->FindWildByPath( fileName, &iPathList, dir );
			
			if( err != KErrNone)
				{
				ERR_PRINTF2( _L( "FindWildByPath() return error = %d"), err );
				SetError(err);
				}
			else
				{
				dirWrapper->SetObjectL(dir);
				}
			}
		else
			{
			ERR_PRINTF2( _L( "Wrong CDir wrapper name %S"), &dirWrapperName );
			SetBlockResult( EFail );
			}
		}
	
	}

	
void CT_FindFileData::DoCmdNew( const TDesC& aSection )
/** Creates new TFindFile class instance */
	{
	DoCleanup();
	
	TPtrC fsObjectName;	
	if( GET_MANDATORY_STRING_PARAMETER( KParamFileSession, aSection, fsObjectName ) )
		{
		RFs* fsObject = NULL;
		TRAPD( err, fsObject = (RFs*)GetDataObjectL(fsObjectName));
		if ( err == KErrNone )
			{
			INFO_PRINTF1( _L( "Create new TFindFile class instance." ) );
			TRAP( err, iFindFile = new (ELeave) TFindFile( *fsObject ) );
			if ( err != KErrNone )
				{
				ERR_PRINTF3( _L( "new TFindFile(%S) error %d" ), &fsObjectName, err );
				SetBlockResult( EFail );
				}
			}
		else
			{
			ERR_PRINTF3( _L( "Unrecognized object name parameter value: %S. Error %d"), &fsObjectName, err );
			SetBlockResult( EFail );
			}
		}
	}
 
void CT_FindFileData::DoCmdSetFindMask( const TDesC& aSection )
/** Specify a combination of drive attributes */
	{
	TUint iniMask;
	if( GetDriveAttsFromConfig( aSection, iniMask ) )
		{
		INFO_PRINTF2( _L( "Mask is  %d"), iniMask );
		TInt err = iFindFile->SetFindMask( iniMask );
		
		if ( err != KErrNone)
			{
			ERR_PRINTF2( _L( "SetFindMask return error = %d"), err );
			SetError( err );
			}
		else
			{
			INFO_PRINTF1( _L( "SetFindMask OK"));
			}
		}	
	}


TBool CT_FindFileData::ConvertToDriveAtts(const TDesC& aDriveAttStr, TUint& aDriveAtt)
/** Convert string to mask */
	{
	TBool	ret = ETrue;
	if ( aDriveAttStr==KDriveAttLocalStr )
		{
		aDriveAtt=KDriveAttLocal;
		}
	else if ( aDriveAttStr==KDriveAttRomStr )
		{
		aDriveAtt=KDriveAttRom;
		}
	else if ( aDriveAttStr==KDriveAttRedirectedStr )
		{
		aDriveAtt=KDriveAttRedirected;
		}
	else if ( aDriveAttStr==KDriveAttSubstedStr )
		{
		aDriveAtt=KDriveAttSubsted;
		}
	else if ( aDriveAttStr==KDriveAttInternalStr )
		{
		aDriveAtt=KDriveAttInternal;
		}
	else if ( aDriveAttStr==KDriveAttRemovableStr )
		{
		aDriveAtt=KDriveAttRemovable;
		}
	else
		{
		TInt	location = aDriveAttStr.Match(_L("*|*"));
		if( location!=KErrNotFound )
			{
			//Converting Left part of the data
			TPtrC	tempStr = aDriveAttStr.Left(location);
			ret=ConvertToDriveAtts(tempStr, aDriveAtt);

			//Converting right data can be with another "|"
			tempStr.Set(aDriveAttStr.Mid(location+1));

			TUint	driveAttTmp;
			if ( ConvertToDriveAtts(tempStr, driveAttTmp) )
				{
				aDriveAtt=aDriveAtt|driveAttTmp;
				}
			else
				{
				ret=EFalse;
				}
			}
		else
			{
			ret=EFalse;
			}
		}

	return ret;
	}
		
	
TBool CT_FindFileData::GetDriveAttsFromConfig(const TDesC& aSection, TUint& aDriveAtt)
	{
	TPtrC	driveAttStr;
	TBool ret = GET_MANDATORY_STRING_PARAMETER(KDriveAtt(), aSection, driveAttStr);
	if ( ret )
		{
		aDriveAtt = 0;
		if ( !ConvertToDriveAtts(driveAttStr, aDriveAtt) )
			{
			TInt	intTemp;
			ret=GET_MANDATORY_INT_PARAMETER(KDriveAtt(), aSection, intTemp);
			if ( ret )
				{
				aDriveAtt=intTemp;
				}
			}
		}

	return ret;
	}


