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


#include "T_ParseBaseData.h"

// Commands

_LIT( KCmdDrive,					"Drive" );
_LIT( KCmdDriveAndPath,				"DriveAndPath" );
_LIT( KCmdDrivePresent,				"DrivePresent" );
_LIT( KCmdExt,						"Ext" );
_LIT( KCmdExtPresent,				"ExtPresent" );
_LIT( KCmdFullName,					"FullName" );
_LIT( KCmdIsExtWild,				"IsExtWild" );
_LIT( KCmdIsKMatchAny,				"IsKMatchAny" );
_LIT( KCmdIsKMatchOne,				"IsKMatchOne" );
_LIT( KCmdIsNameWild,				"IsNameWild" );
_LIT( KCmdIsRoot,					"IsRoot" );
_LIT( KCmdIsWild,					"IsWild" );
_LIT( KCmdName,						"Name" );
_LIT( KCmdNameAndExt,				"NameAndExt" );
_LIT( KCmdNameOrExtPresent,			"NameOrExtPresent" );
_LIT( KCmdNamePresent,				"NamePresent" );
_LIT( KCmdPath,						"Path" );
_LIT( KCmdPathPresent,				"PathPresent" );

// Parameters
_LIT( KParamExpected,				"expected" );

	
CT_ParseBaseData::~CT_ParseBaseData()
/**
* Destructor.
*/
	{
	}
	

TAny* CT_ParseBaseData::GetObject()
/**
* Return a pointer to the object that the data wraps
*
* @return pointer to the object that the data wraps
*/
	{
	return GetParse();
	}

TBool CT_ParseBaseData::DoCommandL( const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/ )
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
	

	if ( aCommand == KCmdDrive )
		{
		DoCmdDrive( aSection );
		}
	else if ( aCommand == KCmdDriveAndPath )
		{
		DoCmdDriveAndPath( aSection );
		}
	else if ( aCommand == KCmdDrivePresent )
		{
		DoCmdDrivePresent( aSection );
		}
	else if ( aCommand == KCmdExt )
		{
		DoCmdExt( aSection );
		}
	else if ( aCommand == KCmdExtPresent )
		{
		DoCmdExtPresent( aSection );
		}
	else if ( aCommand == KCmdFullName )
		{
		DoCmdFullName( aSection );
		}
	else if ( aCommand == KCmdIsKMatchAny )
		{
		DoCmdIsKMatchAny( aSection );
		}
	else if ( aCommand == KCmdIsKMatchOne )
		{
		DoCmdIsKMatchOne( aSection );
		}
	else if ( aCommand == KCmdIsExtWild )
		{
		DoCmdIsExtWild( aSection );
		}
	else if ( aCommand == KCmdIsNameWild )
		{
		DoCmdIsNameWild( aSection );
		}
	else if ( aCommand == KCmdIsRoot)
		{
		DoCmdIsRoot( aSection );
		}
	else if ( aCommand == KCmdIsWild )
		{
		DoCmdIsWild( aSection );
		}
	else if ( aCommand == KCmdName )
		{
		DoCmdName( aSection );
		}
	else if ( aCommand == KCmdNameAndExt )
		{
		DoCmdNameAndExt( aSection );
		}
	else if ( aCommand == KCmdNameOrExtPresent )
		{
		DoCmdNameOrExtPresent( aSection );
		}
	else if ( aCommand == KCmdNamePresent )
		{
		DoCmdNamePresent( aSection );
		}
	else if ( aCommand == KCmdPath )
		{
		DoCmdPath( aSection );
		}
	else if ( aCommand == KCmdPathPresent )
		{
		DoCmdPathPresent( aSection );
		}
	else
	    {
	    retVal = EFalse;    
	    }
	
	return retVal;
	}
	

void CT_ParseBaseData::DoCmdDrive( const TDesC& aSection )
/** Gets the drive letter using Drive(). */
	{
	INFO_PRINTF1( _L( "Drive()" ) );
		
	TPtrC result = GetParse()->Drive();
	INFO_PRINTF2(_L("Drive() return: %S"), &result);
	
	TPtrC expected;
	if ( GET_OPTIONAL_STRING_PARAMETER( KParamExpected, aSection, expected ) )
		{
		if ( result != expected )
			{
			ERR_PRINTF3(_L("Drive() returns %S but expected %S"), &result, &expected);
			SetBlockResult( EFail);
			}
		}
	}
	
void CT_ParseBaseData::DoCmdDriveAndPath( const TDesC& aSection )
/** Gets the path and drive letter using DriveAndPath(). */
	{
	INFO_PRINTF1( _L( "DriveAndPath()" ) );
		
	TPtrC result = GetParse()->DriveAndPath();
	INFO_PRINTF2(_L("DriveAndPath() return: %S"), &result);
	
	TPtrC expected;
	if ( GET_OPTIONAL_STRING_PARAMETER( KParamExpected, aSection, expected ) )
	{
	if ( result != expected )
		{
		ERR_PRINTF3(_L("DriveAndPath() returns %S but expected %S"), &result, &expected);
		SetBlockResult( EFail);
		}
	}
	

	}
	
void CT_ParseBaseData::DoCmdDrivePresent( const TDesC& aSection )
/** Tests whether a drive is present using DrivePresent(). */
	{
	INFO_PRINTF1( _L( "DrivePresent()" ) );
		
	TBool result = GetParse()->DrivePresent();
	
	TPtrC strIsTrue = result ? _L("TRUE") : _L("FALSE");
	INFO_PRINTF2(_L("DrivePresent() return: %S"), &strIsTrue);
				
	TBool expected;	
	if ( GET_OPTIONAL_BOOL_PARAMETER( KParamExpected, aSection, expected ) )
		{
		if ( (result>0) != (expected>0) )
			{
			ERR_PRINTF3(_L("DrivePresent() returns %d but expected %d"), result, expected);
			SetBlockResult( EFail);
			}
		}
	}
	
void CT_ParseBaseData::DoCmdExt( const TDesC& aSection )
/** Gets the file extension using Ext(). */
	{
	INFO_PRINTF1( _L( "Ext()" ) );
		
	TPtrC result = GetParse()->Ext();
	INFO_PRINTF2(_L("Ext() returns: %S"), &result);
	
	TPtrC expected;
	if ( GET_OPTIONAL_STRING_PARAMETER( KParamExpected, aSection, expected ) )
		{
		if ( result != expected )
			{
			ERR_PRINTF3(_L("Ext() returns %S but expected %S"), &result, &expected);
			SetBlockResult( EFail);
			}
		}
	}
	
void CT_ParseBaseData::DoCmdExtPresent( const TDesC& aSection )
/** Tests whether an extension is present using ExtPresent(). */
	{
	INFO_PRINTF1( _L( "ExtPresent()" ) );
		
	TBool result = GetParse()->ExtPresent();
	
	TPtrC strIsTrue = result ? _L("TRUE") : _L("FALSE");
	INFO_PRINTF2(_L("ExtPresent() return: %S"), &strIsTrue);
	
	TBool expected;	
	if ( GET_OPTIONAL_BOOL_PARAMETER( KParamExpected, aSection, expected ) )
		{
		if ( (result>0) != (expected>0) )
			{
			ERR_PRINTF3(_L("ExtPresent() returns %d but expected %d"), result, expected);
			SetBlockResult( EFail);
			}
		}
	}
	
void CT_ParseBaseData::DoCmdFullName( const TDesC& aSection )
/** Gets the complete file specification using FullName(). */
	{
	INFO_PRINTF1( _L( "FullName()" ) );
		
	TPtrC result = GetParse()->FullName();
	INFO_PRINTF2(_L("FullName() return: %S"), &result);
	
	TPtrC expected;
	if ( GET_OPTIONAL_STRING_PARAMETER( KParamExpected, aSection, expected ) )
		{
		if ( result != expected )
			{
			ERR_PRINTF3(_L("FullName() returns %S but expected %S"), &result, &expected);
			SetBlockResult( EFail);
			}
		}
	}
	
void CT_ParseBaseData::DoCmdIsExtWild( const TDesC& aSection )
/** Tests whether the extension in the fully parsed specification contains one or 
 *  more wildcard characters using IsExtWild()
 */
	{
	INFO_PRINTF1( _L( "IsExtWild()" ) );
		
	TBool result = GetParse()->IsExtWild();
	TPtrC temp = result ? _L("TRUE") : _L("FALSE");
	INFO_PRINTF2(_L("IsExtWild() return: %S"), &temp);
	
	TBool expected;
	if ( GET_OPTIONAL_BOOL_PARAMETER( KParamExpected, aSection, expected ) )
		{
		//NB! IsExtWild can return values >1
		if ( (result>0) != (expected>0) )
			{
			ERR_PRINTF3(_L("IsExtWild() returns %S but expected %S"), &result, &expected);
			SetBlockResult( EFail);
			}
		}
	}
	
void CT_ParseBaseData::DoCmdIsKMatchAny( const TDesC& aSection )
/** Tests whether the name or the extension contains asterisk wildcards usinbg IsKMatchAny(). */
	{
	INFO_PRINTF1( _L( "IsKMatchAny()" ) );

	TBool result = GetParse()->IsKMatchAny();
	TPtrC temp = result ? _L("TRUE") : _L("FALSE");
	INFO_PRINTF2(_L("IsKMatchAny() return: %S"), &temp);
	
	TBool expected;	
	if ( GET_OPTIONAL_BOOL_PARAMETER( KParamExpected, aSection, expected ) )
		{
		if ( (result>0) != (expected>0) )
			{
			ERR_PRINTF3(_L("IsKMatchAny() returns %d but expected %d"), result, expected);
			SetBlockResult( EFail);
			}
		}
	}
	
void CT_ParseBaseData::DoCmdIsKMatchOne( const TDesC& aSection )
/** Tests whether the name or the extension contains a question mark wildcard using IsKMatchOne() */
	{
	INFO_PRINTF1( _L( "IsKMatchOne()" ) );

	TBool result = GetParse()->IsKMatchOne();
	TPtrC temp = result ? _L("TRUE") : _L("FALSE");
	INFO_PRINTF2(_L("IsKMatchOne() return: %S"), &temp);

	TBool expected;
	if ( GET_OPTIONAL_BOOL_PARAMETER( KParamExpected, aSection, expected ) )
		{
		if ( (result>0) != (expected>0) )
			{
			ERR_PRINTF3(_L("IsKMatchOne() returns %d but expected %d"), result, expected);
			SetBlockResult( EFail);
			}
		}
	}
	
void CT_ParseBaseData::DoCmdIsNameWild( const TDesC& aSection )
/** Tests whether the filename in the fully parsed specification contains one or more wildcard 
 *  characters using IsNameWild() 
 */
	{
	INFO_PRINTF1( _L( "IsNameWild()" ) );
		
	TBool result = GetParse()->IsNameWild();
	TPtrC temp = result ? _L("TRUE") : _L("FALSE");
	INFO_PRINTF2(_L("IsNameWild() return: %S"), &temp);

	TBool expected;
	if ( GET_OPTIONAL_BOOL_PARAMETER( KParamExpected, aSection, expected ) )
		{
		if ( (result>0) != (expected>0) )
			{
			ERR_PRINTF3(_L("IsNameWild() returns %d but expected %d"), result, expected);
			SetBlockResult( EFail);
			}
		}
	}

void CT_ParseBaseData::DoCmdIsRoot( const TDesC& aSection )
/** Tests whether the path in the fully parsed specification is the root directory using IsRoot(). */
	{
	INFO_PRINTF1( _L( "IsRoot()" ) );
		
	TBool result = GetParse()->IsRoot();
	TPtrC temp = result ? _L("TRUE") : _L("FALSE");
	INFO_PRINTF2(_L("IsRoot() return: %S"), &temp);

	TBool expected;
	if ( GET_OPTIONAL_BOOL_PARAMETER( KParamExpected, aSection, expected ) )
		{
		if ( (result>0) != (expected>0) )
			{
			ERR_PRINTF3(_L("IsRoot() returns %d but expected %d"), result, expected);
			SetBlockResult( EFail);
			}
		}
	}
	
void CT_ParseBaseData::DoCmdIsWild( const TDesC& aSection )
/** Tests whether the filename or the extension in the fully parsed specification contains one or 
  * more wildcard characters using IsWild()
  */
	{
	INFO_PRINTF1( _L( "IsWild()" ) );
		
	TBool result = GetParse()->IsWild();
	TPtrC temp = result ? _L("TRUE") : _L("FALSE");
	INFO_PRINTF2(_L("IsWild() return: %S"), &temp);

	TBool expected;	
	if ( GET_OPTIONAL_BOOL_PARAMETER( KParamExpected, aSection, expected ) )
		{
		if ( (result>0) != (expected>0) )
			{
			ERR_PRINTF3(_L("IsWild() returns %d but expected %d"), result, expected);
			SetBlockResult( EFail);
			}
		}
	
	}

void CT_ParseBaseData::DoCmdName( const TDesC& aSection )
/** Gets the file name using Name(). */
	{
	INFO_PRINTF1( _L( "Name()" ) );
		
	TPtrC result = GetParse()->Name();
	INFO_PRINTF2(_L( "Name() return: %S" ), &result);

	TPtrC expected;
	if ( GET_OPTIONAL_STRING_PARAMETER( KParamExpected, aSection, expected ) )
		{
		if ( result != expected )
			{
			ERR_PRINTF3(_L("Name() returns %S but expected %S"), &result, &expected);
			SetBlockResult( EFail);
			}
		}
	}
	
void CT_ParseBaseData::DoCmdNameOrExtPresent( const TDesC& aSection )
/** Tests whether a name or an extension are present using NameOrExtPresent(). */
	{
	INFO_PRINTF1( _L( "NameOrExtPresent()" ) );
		
	TBool result = GetParse()->NameOrExtPresent();
	TPtrC temp = result ? _L("TRUE") : _L("FALSE");
	INFO_PRINTF2(_L("NameOrExtPresent() return: %S"), &temp);

	TBool expected;
	if ( GET_OPTIONAL_BOOL_PARAMETER( KParamExpected, aSection, expected ) )
		{
		if ( (result>0) != (expected>0) )
			{
			ERR_PRINTF3(_L("NameOrExtPresent() returns %d but expected %d"), result, expected);
			SetBlockResult( EFail);
			}
		}
	}
	
void CT_ParseBaseData::DoCmdNameAndExt( const TDesC& aSection )
/** Gets the file name and extension using NameAndExt(). */
	{
	INFO_PRINTF1( _L( "NameAndExt()" ) );
		
	TPtrC result = GetParse()->NameAndExt();
	INFO_PRINTF2( _L( "NameAndExt() return: %S" ), &result );

	TPtrC expected;
	if ( GET_OPTIONAL_STRING_PARAMETER( KParamExpected, aSection, expected ) )
		{
		if ( result != expected )
			{
			ERR_PRINTF3(_L("NameAndExt() returns %S but expected %S"), &result, &expected);
			SetBlockResult( EFail);
			}
		}
	}
	
void CT_ParseBaseData::DoCmdNamePresent( const TDesC& aSection )
/** Tests whether a file name is present using NamePresent(). */
	{
	INFO_PRINTF1( _L( "NamePresent()" ) );
		
	TBool result = GetParse()->NamePresent();
	TPtrC temp = result ? _L("TRUE") : _L("FALSE");
	INFO_PRINTF2(_L("NamePresent() return: %S"), &temp);

	TBool expected;
	if ( GET_OPTIONAL_BOOL_PARAMETER( KParamExpected, aSection, expected ) )
		{
		if ( (result>0) != (expected>0) )
			{
			ERR_PRINTF3(_L("NamePresent() returns %d but expected %d"), result, expected);
			SetBlockResult( EFail);
			}
		}
	}
	
void CT_ParseBaseData::DoCmdPath( const TDesC& aSection )
/** Gets the path using Path(). */
	{
	INFO_PRINTF1( _L( "Path()" ) );
		
	TPtrC result = GetParse()->Path();
	INFO_PRINTF2( _L( "Path() return: %S" ), &result );

	TPtrC expected;
	if ( GET_OPTIONAL_STRING_PARAMETER( KParamExpected, aSection, expected ) )
		{
		if ( result != expected )
			{
			ERR_PRINTF3(_L("Path() returns %S but expected %S"), &result, &expected);
			SetBlockResult( EFail);
			}
		}
	}
	
void CT_ParseBaseData::DoCmdPathPresent( const TDesC& aSection )
/** Tests whether a path is present using PathPresent(). */
	{
	INFO_PRINTF1( _L( "PathPresent()" ) );
		
	TBool result = GetParse()->PathPresent();
	TPtrC temp = result ? _L("TRUE") : _L("FALSE");
	INFO_PRINTF2(_L("PathPresent() return: %S"), &temp);		

	TBool expected;
	if ( GET_OPTIONAL_BOOL_PARAMETER( KParamExpected, aSection, expected ) )
		{
		if ( (result>0) != (expected>0) )
			{
			ERR_PRINTF3(_L("PathPresent() returns %d but expected %d"), result, expected);
			SetBlockResult( EFail);
			}
		}
	}
	
