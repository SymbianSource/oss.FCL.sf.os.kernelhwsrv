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


#include "T_OpenFileScanData.h"
#include "FileserverUtil.h"

//Constants
const TInt	KBufferLength		= 64;


// Commands
_LIT( KCmdDestructor,					"~" );
_LIT( KCmdNew,						"new" );
_LIT( KCmdNextL,					"NextL" );
_LIT( KCmdThreadId,					"ThreadId" );

// Parameters
_LIT( KParamFileSession,				"FileSession" );
_LIT( KParamExpectedFileName,			"expected_filename%d");
_LIT( KParamDirWrapper,					"wrapper");


CT_OpenFileScanData* CT_OpenFileScanData::NewL()
/**
* Two phase constructor
*/
	{
	CT_OpenFileScanData* ret = new (ELeave) CT_OpenFileScanData();
	CleanupStack::PushL( ret );
	ret->ConstructL();
	CleanupStack::Pop( ret );
	return ret;
	}

CT_OpenFileScanData::CT_OpenFileScanData()
:	iOpenFileScan(NULL)
/**
* Protected constructor. First phase construction
*/
	{
	}
	
void CT_OpenFileScanData::ConstructL()
/**
* Protected constructor. Second phase construction
*/
	{
	}
	
CT_OpenFileScanData::~CT_OpenFileScanData()
/**
* Destructor.
*/
	{
	DoCleanup();
	}
	
void CT_OpenFileScanData::DoCleanup()
/**
* Contains cleanup implementation
*/
	{		
	delete iOpenFileScan;
	iOpenFileScan = NULL;
	}
	
TAny* CT_OpenFileScanData::GetObject()
/**
* Return a pointer to the object that the data wraps
*
* @return pointer to the object that the data wraps
*/
	{
	return iOpenFileScan;
	}

TBool CT_OpenFileScanData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/)
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
		DoCmdNew( aSection );
		}
	else if ( aCommand == KCmdNextL )
		{
		DoCmdNextL( aSection );
		}
	else if ( aCommand == KCmdThreadId )
		{
		DoCmdThreadId( aSection );
		}
	else
	    {
	    retVal = EFalse;    
	    }
	
	return retVal;
	}
	
void CT_OpenFileScanData::DoCmdNew( const TDesC& aSection )
/** Creates new TOpenFileScan class instance */
	{
	TPtrC rfsName;
	RFs* rfs = NULL;
	if ( GetRfsParam( aSection, rfs, rfsName ) )
		{
		INFO_PRINTF2( _L( "Create new TOpenFileScan(%S) class instance." ), &rfsName );
		
		// do create		
		TRAPD( err, iOpenFileScan = new (ELeave) TOpenFileScan( *rfs ) );
		if ( err != KErrNone )
			{
			ERR_PRINTF3( _L( "new TOpenFileScan(%S) error=%d" ), &rfsName, err );
			SetError( err );
			}
		}
	}
	
void CT_OpenFileScanData::DoCmdNextL( const TDesC& aSection )
/** Calls NextL() function */
	{
	RPointerArray<TPath> pathsToFind;
	CleanupResetAndDestroyPushL(pathsToFind);
	
	// Read expected file names into array
	TBool eof = EFalse;
	TInt index = 0;
	do
		{
		TBuf<KBufferLength> tempStore;
		tempStore.Format(KParamExpectedFileName(), ++index);
		TPtrC fileName;
		eof = !GET_OPTIONAL_STRING_PARAMETER(tempStore, aSection, fileName);
		if (!eof)
			{
			TPath* path = new(ELeave) TPath(fileName);
			CleanupStack::PushL(path);
			pathsToFind.AppendL(path);
			CleanupStack::Pop();
			}
		}
	while (!eof);
 
	TInt err = KErrNone;

	CFileList* fileList = NULL;
	if(pathsToFind.Count() == 0)
		{
		TRAP( err, iOpenFileScan->NextL( fileList ));
		
		if( fileList)
			{
			CleanupStack::PushL(fileList);
			// NB! CDir == CFileList
			TPtrC wrapperName;
			if (!err && GET_OPTIONAL_STRING_PARAMETER(KParamDirWrapper, aSection, wrapperName))
				{
				CT_DirData *wrapper = NULL;
				
				TRAPD( error, wrapper = static_cast<CT_DirData*>(GetDataWrapperL(wrapperName)))
				
				if( wrapper && (error==KErrNone) )
					{
					wrapper->SetObjectL(fileList);
					fileList = NULL;
					}
				else
					{
					ERR_PRINTF2( _L( "Wrong CDir wrapper name %S"), &wrapperName );
					SetBlockResult( EFail );
					}
				}
			
			CleanupStack::Pop();
			delete fileList;
			fileList = NULL;
			}
		}
	else
		{
		// Check expected file names
		for( eof = EFalse ; !eof && (pathsToFind.Count() > 0);  ) 
			{
			iOpenFileScan->NextL(fileList);
			if( fileList)
				{
				for ( TInt i = 0; i < fileList->Count(); i++ )
					{
					INFO_PRINTF1((*fileList)[i].iName );
					for(TInt j =  pathsToFind.Count() - 1 ; j >= 0; j--)
						{			
						if( *(pathsToFind[j]) == (*fileList)[i].iName )
							{
							TPath* elemForRemove = pathsToFind[j];
							pathsToFind.Remove(j);
							delete elemForRemove;
							}
						}
					}
					
				delete fileList;
				fileList = NULL;
				}
			else
				{
				eof = ETrue;
				}
			}
			
		// Some file names are not found
		if (pathsToFind.Count() > 0)
			{
			for(TInt i = 0; i < pathsToFind.Count(); i++)
				{
				ERR_PRINTF2( _L( "File %S haven't been found" ), pathsToFind[i] );
				}
			SetBlockResult( EFail );
			}
		}
	
   	if ( err != KErrNone )
		{
		ERR_PRINTF2( _L( "NextL() error=%d" ), err );
		SetError( err );
		}
	
	//Free massive
	CleanupStack::PopAndDestroy(&pathsToFind);
	}
	
// Calls ThreadId() function
void CT_OpenFileScanData::DoCmdThreadId( const TDesC& aSection )
	{
	TPtrC rfsName;
	
	if(GET_MANDATORY_STRING_PARAMETER( KParamFileSession, aSection, rfsName))
		{
		CT_FsData *fsData = NULL;
		
		TRAPD(err,fsData =  static_cast<CT_FsData*>(GetDataWrapperL(rfsName)));
		
		if( err==KErrNone )
			{
			//Recieving thread ids.
			TUint64 rfsThreadId = fsData->ThreadId();
			TUint64 id = iOpenFileScan->ThreadId().Id();
			//Comparing id's
			if(rfsThreadId != id)
				{
				ERR_PRINTF3( _L( "Diffrent thread id's %u %u"),rfsThreadId,id);
				SetBlockResult( EFail );
				}
			}
		else
			{
			ERR_PRINTF2( _L( "Wrong session name:%S"),&rfsName);
			SetBlockResult( EFail );
			}
		}

	}
	
TBool CT_OpenFileScanData::GetRfsParam( const TDesC& aSection, RFs*& aRfs, TPtrC& aRfsName )
/** this function retrieves the "rfs" current command parameter using
*	GET_MANDATORY_STRING_PAREMETER macro
* 
*	@param	aSection		- the entry in the ini file requiring the command to be processed
*	@param	aRfs 		- the returned RFs pointer
*	@param	aRfsName	- the returned RFs's name as stated in ini file
*
*	@return	ETrue 		- if the the parameter has been successfully read and interpreted
*				EFalse		- otherwise
*/
	{
	TBool result = EFalse;
	
	if ( GET_MANDATORY_STRING_PARAMETER( KParamFileSession, aSection, aRfsName ) )
		{
		TRAPD( err, aRfs = (RFs*)GetDataObjectL( aRfsName ) );
		
		if ( err != KErrNone )
			{
			ERR_PRINTF3( _L( "Unrecognized object name: %S (error = %d)" ), &aRfsName, err );
			SetBlockResult( EFail );
			}
		else
			{
			result = ETrue;	
			}
		}
	
	return result;
	}
 
