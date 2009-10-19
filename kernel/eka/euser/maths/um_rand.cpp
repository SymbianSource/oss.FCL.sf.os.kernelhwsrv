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
// e32\euser\maths\um_rand.cpp
// 
//

#include "um_std.h"




EXPORT_C TInt Math::Rand(TInt64 &aSeed)
/**
Generates a stream of uniformly distributed pseudo-random integers
in the range, 0 to KMaxTInt.

For each stream of pseudo-random numbers you wish to generate, you should
pass the reference to the same 64-bit seed on each call to this function.
You should not change the seed between calls.

@param aSeed A reference to a 64-bit seed, which is updated
             as a result of the call.

@return The next pseudo-random number in the sequence. 
*/
	{

	aSeed*=214013;
    aSeed+=2531011;
    return(((TInt)(aSeed>>16))&0x7fffffff);
	}




EXPORT_C TReal Math::FRand(TInt64& aSeed) __SOFTFP
/**
Generates a stream of uniformly distributed pseudo-random real numbers
in the range, 0 to 1.

@param aSeed A reference to a 64-bit seed, which is updated
             as a result of the call.

@return The next pseudo-random number in the sequence. 
*/
	{
	TUint low = (TUint)Math::Rand(aSeed);
	TUint high = (TUint)Math::Rand(aSeed)&0x7FFFFFFF;	// make sure TInt64 is positive
	TRealX f((static_cast<TInt64>(high) << 32) | low);	// construct TRealX 0<=f<2^63
	if (f.iExp)
		f.iExp-=63;						// Scale f to range 0<=f<1
    return(TReal(f));
	}
