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
// randgen
// Random number generator for the tests
// 
//


#ifndef __RANDGEN_H__
#define __RANDGEN_H__

#include <e32std.h>


class TRandomGenerator
	{
	public:
		TRandomGenerator();
		inline void SetSeed( const TInt64& aSeed );
		TUint Next();

	private:
		TInt64	iValue;
	};


inline void TRandomGenerator::SetSeed( const TInt64& aSeed )
	{
	iValue = aSeed;
	}


#endif
