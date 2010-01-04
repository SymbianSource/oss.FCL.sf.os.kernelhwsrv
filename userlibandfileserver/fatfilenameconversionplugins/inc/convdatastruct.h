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


#include <e32std.h>
#include <e32def.h>

struct TLeadOrSingle
	{
	/**iUnicodeIfSingle can have any of the following values...
	-Unicode character if this is a single byte character;
	-0xFFFD if it's an unidentified character;
	-0xFFFF if it's an "empty" Leadbyte -doesn't have any Tailbytes
	-0 if it's a Leadbyte*/
	TUint16 iUnicodeIfSingle;
	/**Index into the double byte table.*/
	TUint16 iDoubleByteIndex;
	};

class TConvDataStruct
	{
	public:
		static TInt ConvertSingleUnicode(TInt aUnicode, TInt& aTrailByte);
		
	public:
		/**Conversion table for single bytes and lead bytes from 0x80 to 0xFF.*/
		static const TLeadOrSingle KFirstByteConversions[128];
		/**The double-byte table, stores all Unicode values
		corresponding to double byte characters.*/
		static const TUint16 KDoubleByteConversions[];
		/**Length of double-byte conversion table.*/
		static const TUint16 KDoubleByteConversionLength;
		/**Minimum calue a trail byte may take.*/
		static const TUint8 KMinTrailByte;
		/**Maximum value a trail byte may take.*/
		static const TUint8 KMaxTrailByte;
	};

