// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Utility functions for the Mass Storage file system.
// 
//

/**
 @file
 @internalTechnology
*/

#include <e32std.h>
#include <e32std_private.h>

/**
Returns ETrue if aNum is a power of two
*/
GLDEF_C TBool IsPowerOfTwo(TInt aNum)
	{

	if (aNum==0)
		return(EFalse);

	while(aNum)
		{
		if (aNum & 0x01)
			{
			if (aNum>>1) 
				return EFalse;
			break;
			}
		aNum>>=1;
		}
	return ETrue;
	}

/**
Returns the position of the highest bit in aNum or -1 if aNum == 0
*/
GLDEF_C TInt Log2(TInt aNum)
	{

	TInt res=-1;
	while(aNum)
		{
		res++;
		aNum>>=1;
		}
	return(res);
	}
	
