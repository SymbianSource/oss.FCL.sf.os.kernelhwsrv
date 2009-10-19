// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This file is part of scppnwdl.dll.
// 
//

#include <e32std.h>
#include <e32std_private.h>

EXPORT_C void operator delete(TAny *aPtr) __NO_THROW
	{

	User::Free(aPtr);
	}

EXPORT_C void operator delete[](TAny * aPtr) __NO_THROW
    {

    User::Free(aPtr);
    }

EXPORT_C void operator delete(TAny *aPtr, const std::nothrow_t& aNoThrow) __NO_THROW
	{
	(void)aNoThrow;
	User::Free(aPtr);
	}

EXPORT_C void operator delete[](TAny * aPtr, const std::nothrow_t& aNoThrow) __NO_THROW
	{
	(void)aNoThrow;
	User::Free(aPtr);
	}
