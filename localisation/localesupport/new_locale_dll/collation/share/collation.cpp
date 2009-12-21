/*
* Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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


#include "locl_collation.h"

/**
Gets the address of the locale character set object which contains 
collation rules etc. It is used in Unicode builds to supply 
locale-specific character attribute and collation data.
@return The address of the locale character set object, or NULL 
in case of a non-UNICODE build.
*/
EXPORT_C const LCharSet* LoclCollation::CharSet()
	{
	#ifdef _UNICODE
		return &TheCharSet;
	#else
		return NULL;
	#endif
	}

/**
Gets the address of the character type conversion table.
The character type conversion table does not exist in 
the Unicode build. This table has 256 items which classifies
256 ASCII codes into: Uppercase letter, Lowercase letter, 
Punctuation, Decimal digit etc..
@return The address of the character type conversion table, 
or NULL in case of a UNICODE build.
*/
EXPORT_C const TUint8 * LoclCollation::TypeTable()
	{
	#ifdef _UNICODE
		return NULL;
	#else
		return(&LAlphabet::TypeTable[0]);
	#endif 		
	}


/**
Gets the address of the uppercase table. The uppercase table 
does not exist in the Unicode build. It is used to convert 
the letter in lowercase to uppercase.
@return The address of the uppercase table, or NULL
in case of a UNICODE build.
*/
EXPORT_C const TText * LoclCollation::UpperTable()
	{
	#ifdef _UNICODE
		return NULL;
	#else
		return(&LAlphabet::UpperTable[0]);
	#endif 

	}

/**
Gets the address of the lowercase table. The lowercase table
does not exist in the Unicode build. It is used to convert 
the letter in uppercase to lowercase.
@return The address of the lowercase table, or NULL
in case of a UNICODE build.
*/
EXPORT_C const TText * LoclCollation::LowerTable()
	{
	#ifdef _UNICODE
		return NULL;
	#else
		return(&LAlphabet::LowerTable[0]);
	#endif 
	}

/**
Gets the address of the fold table. The fold table does not exist 
in the Unicode build. It is used to fold the character according
to a specified folding method: converting characters to their 
lower case form, if any; stripping accents; converting digits 
representing values 0..9 to characters '0'..'9' etc..
@return The address of the fold table, or NULL
in case of a UNICODE build.
*/
EXPORT_C const TText * LoclCollation::FoldTable()
	{
	#ifdef _UNICODE
		return NULL;

	#else
		return(&LAlphabet::FoldTable[0]);
	#endif 
	}

/**
Gets the address of the collate table. The collate table does
not exist in the Unicode build. This table is used to collate
strings to remove differences between characters that are deemed 
unimportant for the purposes of ordering characters.
@return The address of the collate table, or NULL
in case of a UNICODE build.
*/
EXPORT_C const TText * LoclCollation::CollTable()
	{
	#ifdef _UNICODE
		return NULL;
	#else
		return(&LAlphabet::CollTable[0]);
	#endif 
	}

/**
Check whether it is a Unicode Build.
@return ETrue for Unicode Build, EFalse for non-Unicode Build.
*/
EXPORT_C TBool LoclCollation::UniCode()
	{
	#ifdef _UNICODE
		return ETrue;
	#else
		return EFalse;
	#endif 
	}
