// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Symbian Foundation License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.symbianfoundation.org/legal/sfl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//

#include <e32std.h>
#include <e32def.h>
#include "convdatastruct.h"
#include <kernel/localise.h>


class UnicodeConv
	{
	public:
		static TInt ConvertFromUnicodeL(TDes8& aForeign, const TDesC16& aUnicode, const TDesC8& aReplacementChar, TFatUtilityFunctions::TOverflowAction aOA);
		static TInt ConvertToUnicodeL(TDes16& aUnicode, const TDesC8& aForeign, TFatUtilityFunctions::TOverflowAction aOA);
		static TBool IsLegalShortNameCharacter (TUint aCharacter);
		};

