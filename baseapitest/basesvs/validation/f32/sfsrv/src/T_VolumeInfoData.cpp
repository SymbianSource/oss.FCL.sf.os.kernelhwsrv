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


#include "T_VolumeInfoData.h"


// Commands
_LIT( KCmdNew,               "new" );
_LIT( KCmdDestructor,        "~" );

CT_VolumeInfoData* CT_VolumeInfoData::NewL()
/**
* Two phase constructor
*/
	{
	CT_VolumeInfoData* ret = new (ELeave) CT_VolumeInfoData();
	CleanupStack::PushL( ret );
	ret->ConstructL();
	CleanupStack::Pop( ret );
	return ret;
	}

CT_VolumeInfoData::CT_VolumeInfoData()
:	iVolumeInfo(NULL)
/**
* Protected constructor. First phase construction
*/
	{
	}
	
void CT_VolumeInfoData::ConstructL()
/**
* Protected constructor. Second phase construction
*/
	{
	}
	
CT_VolumeInfoData::~CT_VolumeInfoData()
/**
* Destructor.
*/
	{
	DoCleanup();
	}
	

TAny* CT_VolumeInfoData::GetObject()
/**
* Return a pointer to the object that the data wraps
*
* @return pointer to the object that the data wraps
*/
	{
	return iVolumeInfo;
	}

void CT_VolumeInfoData::SetObjectL(TAny* aAny)
	{
	DoCleanup();
	iVolumeInfo = static_cast<TVolumeInfo*> (aAny);
	}
	
void CT_VolumeInfoData::DisownObjectL()
	{
	iVolumeInfo = NULL;
	}
	
inline TCleanupOperation CT_VolumeInfoData::CleanupOperation()
	{
	return CleanupOperation;
	}

void CT_VolumeInfoData::CleanupOperation(TAny* aAny)
	{
	TVolumeInfo* volumeInfo = static_cast<TVolumeInfo*>(aAny);
	delete volumeInfo;
	}

TBool CT_VolumeInfoData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& /*aSection*/, const TInt /*aAsyncErrorIndex*/ )
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
	
	if ( aCommand == KCmdNew )
		{
		DoCmdNew();
		}
	else if ( aCommand == KCmdDestructor )
		{
		DoCleanup();
		}
	else
	    {
	    retVal = EFalse;    
	    }
	
	return retVal;
	}
	
void CT_VolumeInfoData::DoCmdNew()
/** Creates new TVolumeInfo class instance */
	{
	INFO_PRINTF1( _L( "Create new TVolumeInfo class instance" ) );
	
	TRAPD( err, iVolumeInfo = new (ELeave) TVolumeInfo() );
	if ( err!=KErrNone )
		{
		ERR_PRINTF2( _L( "new error %d" ), err );
		SetError( err );
		}
	}

void CT_VolumeInfoData::DoCleanup()
	{
	INFO_PRINTF1( _L( "Delete TVolumeInfo class instance." ) );
	
	delete iVolumeInfo;
	iVolumeInfo = NULL;
	}


