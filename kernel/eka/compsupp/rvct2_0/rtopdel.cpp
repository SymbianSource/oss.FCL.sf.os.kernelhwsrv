// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "ARM EABI LICENCE.txt"
// which accompanies this distribution, and is available
// in kernel/eka/compsupp.
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//

//#include <stddef.h>
#include <e32std.h>
#include <e32std_private.h>
//#include <e32math.h>
//#include "uc_std.h"


EXPORT_C void operator delete(TAny *aPtr)
//
// The replacement delete operator.
//
	{

	User::Free(aPtr);
	}

EXPORT_C void operator delete[](TAny * aPtr)
    {

    User::Free(aPtr);
    }
