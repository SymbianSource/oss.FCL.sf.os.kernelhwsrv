/*
* Copyright (c) 2005 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
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
#include "t_tlocl32_convdatastruct.h"

class UnicodeConv
	{
	public:
		static void ConvertFromUnicodeL(TDes8& aForeign, const TDesC16& aUnicode);
		static void ConvertToUnicodeL(TDes16& aUnicode, const TDesC8& aForeign);
		static TBool IsLegalShortNameCharacter (TUint aCharacter);
	};
