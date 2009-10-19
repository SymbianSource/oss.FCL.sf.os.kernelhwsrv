// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\math\t_real.h
// E32TMATH.H - Test header file for TReal32, TReal64
// 
//


#include <e32std.h>

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

