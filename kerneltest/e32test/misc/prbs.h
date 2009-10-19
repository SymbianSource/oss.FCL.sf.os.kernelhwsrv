// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\prbs.h
// 
//

#ifndef __PRBS_H__
#define __PRBS_H__
#include <e32cmn.h>

LOCAL_C TUint Random(TUint* aSeed)
	{
	TUint x = aSeed[0];
	TUint r3 = x >> 1;
	r3 |= (aSeed[1] << 31);
	aSeed[1] = x & 1;
	r3 ^= (x << 12);
	x = r3 ^ (r3 >> 20);
	aSeed[0] = x;
	return x;
	}

#endif
