// Copyright (c) 1994-2010 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file
 @publishedAll
 @interim
*/

#ifndef __TMSMAIN_H__
#define __TMSMAIN_H__

#include <f32file.h>
#include <e32test.h>
#include <e32math.h>
#include "mstestdata.h"

GLREF_D RTest test;

GLREF_C void CallTestsL();

GLDEF_C void TestIfEqual( TInt aValue, TInt aExpected, TInt aLine, const char aFileName[]);
#define TEST_FOR_VALUE( r, expected ) TestIfEqual( r, expected, __LINE__, __FILE__)

#endif //__TMSMAIN_H__


