// Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Random number generator
// 
//

#if defined(__EPOC32__) || defined (__WINS__)
#include <e32std.h>
#include <e32std_private.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#endif

#include "randgen.h"

TRandomGenerator::TRandomGenerator()
	: iValue(0x90099090)
	{
	}


TUint TRandomGenerator::Next()
	{
	iValue *= 214013;
    iValue += 2531011;
	return iValue;
	}

