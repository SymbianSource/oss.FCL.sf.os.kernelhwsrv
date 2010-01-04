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
#include "convdatastruct.h"


class UnicodeConv
	{
	public:
		IMPORT_C static void ConvertFromUnicodeL(TDes8& aForeign, const TDesC16& aUnicode);
		IMPORT_C static void ConvertToUnicodeL(TDes16& aUnicode, const TDesC8& aForeign);
		IMPORT_C static TBool IsLegalShortNameCharacter (TUint aCharacter);
		IMPORT_C static TInt ConvertFromUnicodeL(TDes8& aForeign, const TDesC16& aUnicode, TBool leaveWhenOverflow /*when overflow, ETrue: leave; EFalse: truncate*/);
		IMPORT_C static TInt ConvertToUnicodeL(TDes16& aUnicode, const TDesC8& aForeign, TBool leaveWhenOverflow /*when overflow, ETrue: leave; EFalse: truncate*/);
	};

