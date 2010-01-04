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

This contains CT_FormatData
*/

//	User includes
#include "T_FormatData.h"

/*@{*/
///	Parameters
_LIT(KRFsName,							"RFs");
_LIT(KDrive,							"drive");
_LIT(KFormat,							"format");
_LIT(KSpecialInfo,						"specialinfo");
_LIT(KAsync,							"async");
_LIT(KEnd,								"end");
_LIT(KCount,							"count");

//	Format mode
_LIT(KFormatHighDensity,				"EHighDensity");
_LIT(KFormatLowDensity,					"ELowDensity");
_LIT(KFormatFullFormat,					"EFullFormat");
_LIT(KFormatQuickFormat,				"EQuickFormat");
_LIT(KFormatSpecialFormat,				"ESpecialFormat");
_LIT(KFormatForceErase,					"EForceErase");

///	Commands
_LIT(KCmdNew,							"new");
_LIT(KCmdDestructor,					"~");
_LIT(KCmdOpen,							"Open");
_LIT(KCmdClose,							"Close");
_LIT(KCmdNext,							"Next");
/*@}*/

CT_FormatData* CT_FormatData::NewL()
/**
 * Two phase constructor
 */
	{
	CT_FormatData* ret = new ( ELeave ) CT_FormatData();
	CleanupStack::PushL( ret );
	ret->ConstructL();
	CleanupStack::Pop( ret );
	return ret;
	}

CT_FormatData::CT_FormatData()
:	iFormat( NULL )
,	iNext( NULL )
,	iCount( 0 )
,	iCountNextEnd( 0 )
/**
 * Protected constructor. First phase construction
 */
	{
	}

void CT_FormatData::ConstructL()
/**
 * Protected constructor. Second phase construction
 */
	{
	iNext = CActiveCallback::NewL( *this );
	}

CT_FormatData::~CT_FormatData()
/**
 * Destructor.
 */
	{
	delete iNext;
	iNext = NULL;
	DoCleanup();
	}

void CT_FormatData::DoCleanup()
/**
 * Contains cleanup implementation
 */
	{
	//Deleting RFormat.
	if(iFormat != NULL)
		{
		INFO_PRINTF1( _L("Deleting current RFormat") );
		delete iFormat;
		iFormat = NULL;
		}
	}

TAny* CT_FormatData::GetObject()
/**
 * Return a pointer to the object that the data wraps
 *
 * @return	pointer to the object that the data wraps
 */
	{
	return iFormat;
	}

TBool CT_FormatData::DoCommandL( const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex )
/**
 * Process a command read from the ini file
 *
 * @param	aCommand requiring command to be processed
 * @param	aSection the section in the ini file requiring the command to be processed
 * @param	aAsyncErrorIndex the index of asynchronous command error code belongs to.
 *
 * @leave	system wide error
 *
 * @return	ETrue if the command is processed
 */
	{
	TBool retVal = ETrue;

	if ( aCommand == KCmdNew )
		{
		DoCmdNewL();
		}
	else if ( aCommand == KCmdDestructor )
		{
		DoCmdDestructor();
		}
	else if ( aCommand == KCmdOpen )
		{
		DoCmdOpenL( aSection );
		}
	else if ( aCommand == KCmdClose )
		{
		DoCmdClose();
		}
	else if ( aCommand == KCmdNext )
		{
		DoCmdNext( aSection, aAsyncErrorIndex );
		}
	else
		{
		retVal = EFalse;
		}
	return retVal;
	}

void CT_FormatData::DoCmdNewL()
/**
 * Creates new RFormat class instance
 */
	{
	//Deletes previous RFormat class instance if it was already created.
	DoCleanup();

	INFO_PRINTF1( _L("Create new RFormat class instance") );

	// do create
	TRAPD( err, iFormat = new ( ELeave ) RFormat() );
	if ( err != KErrNone )
		{
		ERR_PRINTF2( _L("new error %d"), err );
		SetError( err );
		}
	}

void CT_FormatData::DoCmdDestructor()
/**
 * Destroy RFormat the object
 */
	{
	DoCleanup();
	}

void CT_FormatData::DoCmdOpenL( const TDesC& aSection )
/**
 * RFormat::Open
 *
 * @leave	system wide error
 */
	{
	TBool	dataOk = ETrue;

	RFs*	rfsObject = NULL;
	TPtrC	rfsObjectName;
	if ( GET_MANDATORY_STRING_PARAMETER( KRFsName, aSection, rfsObjectName ) )
		{
		TRAPD( err, rfsObject = ( RFs* )GetDataObjectL( rfsObjectName ));
		
		if ( err != KErrNone )
			{
			ERR_PRINTF1( _L("Error with fileserver"));
			SetBlockResult( EFail );
			}
		}
	else
		{
		dataOk = EFalse;
		}

	TPtrC	drive;
	if ( !GET_MANDATORY_STRING_PARAMETER( KDrive, aSection, drive ) )
		{
		dataOk = EFalse;
		}

	TUint	formatMode = 0;
	if ( !GetFormatMode( KFormat, aSection, formatMode ) )
		{
		dataOk = EFalse;
		}

	if ( dataOk )
		{
		TInt	err = KErrNone;

		TPtrC	specialInfo;
		if ( GET_OPTIONAL_STRING_PARAMETER( KSpecialInfo, aSection, specialInfo ) )
			{
			HBufC8*	buf = HBufC8::NewLC( specialInfo.Length() );
			buf->Des().Copy( specialInfo );
			err = iFormat->Open( *rfsObject, drive, formatMode, iCount, buf->Des() );
			CleanupStack::PopAndDestroy( buf );
			}
		else
			{
			err = iFormat->Open( *rfsObject, drive, formatMode, iCount );
			}
		INFO_PRINTF2( _L("iCount %d"), iCount );
		iCountPckg = iCount;
		if ( err != KErrNone )
			{
			ERR_PRINTF2( _L("Open() error %d"), err );
			SetError( err );
			}
		}
	}

void CT_FormatData::DoCmdClose()
/**
 * RFormat::Close
 */
	{
	iFormat->Close();
	}

void CT_FormatData::DoCmdNext( const TDesC& aSection, const TInt aAsyncErrorIndex )
/**
 * RFormat::Next
 */
	{
	TBool	async = EFalse;
	GET_OPTIONAL_BOOL_PARAMETER( KAsync, aSection, async );

	iCountNextEnd = 0;
	GET_OPTIONAL_INT_PARAMETER( KEnd, aSection, iCountNextEnd );
	
	GET_OPTIONAL_INT_PARAMETER( KCount, aSection, iCount );

	if ( async )
		{
		iFormat->Next( iCountPckg, iNext->iStatus );
		iNext->Activate( aAsyncErrorIndex );
		IncOutstanding();
		}
	else
		{
		TInt	err = KErrNone;
		while ( ( iCount > iCountNextEnd ) && ( err == KErrNone ) )
			{
			err = iFormat->Next( iCount );
			INFO_PRINTF2( _L("iCount %d"), iCount );
			}
		if ( err != KErrNone )
			{
			ERR_PRINTF2( _L("Next() error %d"), err );
			SetError( err );
			}
		}
	}

TBool CT_FormatData::GetFormatMode( const TDesC& aParameterName, const TDesC& aSection, TUint& aFormatMode )
	{
	aFormatMode = 0;

	TPtrC	formatStr;
	TBool	ret = GET_MANDATORY_STRING_PARAMETER( aParameterName, aSection, formatStr );
	if ( ret )
		{
		if ( !ConvertToFormatMode( formatStr, aFormatMode ) )
			{
			TInt	intTemp;
			ret = GET_MANDATORY_INT_PARAMETER( aParameterName, aSection, intTemp );
			if ( ret )
				{
				aFormatMode = intTemp;
				}
			}
		}

	return ret;
	}

TBool CT_FormatData::ConvertToFormatMode( const TDesC& aFormatModeStr, TUint& aFormatMode )
	{
	TBool	ret = ETrue;
	if ( aFormatModeStr == KFormatHighDensity )
		{
		aFormatMode = EHighDensity;
		}
	else if ( aFormatModeStr == KFormatLowDensity )
		{
		aFormatMode = ELowDensity;
		}
	else if ( aFormatModeStr == KFormatFullFormat )
		{
		aFormatMode = EFullFormat;
		}
	else if ( aFormatModeStr == KFormatQuickFormat )
		{
		aFormatMode = EQuickFormat;
		}
	else if ( aFormatModeStr == KFormatSpecialFormat )
		{
		aFormatMode = ESpecialFormat;
		}
	else if ( aFormatModeStr == KFormatForceErase )
		{
		aFormatMode = EForceErase;
		}
	else
		{
		TInt	location = aFormatModeStr.Match( _L("*|*") );
		if( location != KErrNotFound )
			{
			//Converting Left part of the data
			TPtrC		tempStr = aFormatModeStr.Left( location );
			ret = ConvertToFormatMode( tempStr, aFormatMode );

			//Converting right data can be with another "|"
			tempStr.Set( aFormatModeStr.Mid( location + 1 ) );

			TUint	formatModeTmp;
			if ( ConvertToFormatMode( tempStr, formatModeTmp ) )
				{
				aFormatMode |= formatModeTmp;
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
		}

	return ret;
	}

void CT_FormatData::RunL( CActive* aActive, TInt aIndex )
	{
	if ( aActive == iNext )
		{
		TInt	err = aActive->iStatus.Int();
		if( err != KErrNone )
			{
			ERR_PRINTF2( _L("DoCancel Error %d"), err );
			SetAsyncError( aIndex, err );
			DecOutstanding();
			}
		else
			{
			// Reset the outstanding request state
			INFO_PRINTF2( _L("RunL iCount %d"), iCountPckg() );
			if ( iCountPckg() > iCountNextEnd )
				{
				iFormat->Next( iCountPckg, iNext->iStatus );
				iNext->Activate( aIndex );
				}
			else
				{
				DecOutstanding();
				}
			}
		}
	else
		{
 		ERR_PRINTF1( _L("Stray RunL signal") );
 		SetBlockResult( EFail );
		}
	}

void CT_FormatData::DoCancel( CActive* aActive, TInt aIndex )
	{
	if ( aActive == iNext )
		{
		TInt	err = aActive->iStatus.Int();
		if( err != KErrNone )
			{
			ERR_PRINTF2( _L("DoCancel Error %d"), err );
			SetAsyncError( aIndex, err );
			}

		// Reset the outstanding request state
		DecOutstanding();
		}
	else
		{
 		ERR_PRINTF1( _L("Stray RunL signal") );
 		SetBlockResult( EFail );
		}
	}
