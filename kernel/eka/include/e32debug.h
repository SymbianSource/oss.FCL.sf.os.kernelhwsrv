// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// e32\include\e32debug.h
// 
//

#ifndef __E32DEBUG_H__
#define __E32DEBUG_H__
#include <e32std.h>			// TThreadId

/**	@publishedPartner
	@removed
*/
const TInt KMaxProfiles=64;

/**	@publishedPartner
	@removed
*/
class TProfile
    {
public:
    TInt iTime;
    TInt iCount;
    };
//

/**	@publishedAll
	@released
*/
class RDebug
	{
public:
    IMPORT_C static void Printf(const char*, ...);
    IMPORT_C static void RawPrint(const TDesC8& aDes);
    IMPORT_C static TInt Print(TRefByValue<const TDesC> aFmt,...);
    IMPORT_C static void RawPrint(const TDesC16& aDes);
	};

#endif

