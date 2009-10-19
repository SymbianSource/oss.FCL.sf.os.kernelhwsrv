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
// f32test\math\t_math.h
// Copy of e32\umath\um_std.h for E32\TMATH directory
// 
//


#include <e32math.h>
#include <e32std.h>
#include <e32test.h>

struct SReal32
    {
    unsigned man:23;
    unsigned exp:8;
    unsigned sign:1;
    };

#if defined(__DOUBLE_WORDS_SWAPPED__)
struct SReal64
    {
    unsigned msm:20;
    unsigned exp:11;
    unsigned sign:1;
    TUint lsm;
    };
#define DVAL(m0,m1,m2,m3,e)     {(TUint)((m0<<16)|m1),e+KExponentBias,0,(TUint)((m2<<16)|m3)}
#else
struct SReal64
    {
    TUint lsm;
    unsigned msm:20;
    unsigned exp:11;
    unsigned sign:1;
    };
#define DVAL(m0,m1,m2,m3,e)     {(TUint)((m2<<16)|m3),(TUint)((m0<<16)|m1),e+KExponentBias,0}
#endif

enum TMathPanic
    {
	EMathDivideByZero,
	EMathOverflow,
	EMathUnderflow,
	EMathBadOperand,
    EMathUnknownError
    };

GLREF_C void Panic(TMathPanic aPanic);
