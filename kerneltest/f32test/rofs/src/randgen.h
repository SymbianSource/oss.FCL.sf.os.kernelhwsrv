// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Pseudo-random number generator
// 
//

#ifndef RANDGEN_H
#define RANDGEN_H

#if defined(__EPOC32__) || defined (__WINS__)
#include <e32std.h>

#else
typedef	unsigned int	TUint;
#endif

class TRandomGenerator
	{
	public:
		TRandomGenerator();
		inline void SetSeed( const TUint aSeed );
		TUint Next();

	private:
		TUint	iValue;
	};

inline void TRandomGenerator::SetSeed( const TUint aSeed )
	{
	iValue = aSeed;
	}

#endif
