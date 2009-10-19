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
// e32\common\gcchelp.cpp
// 
//

#include "common.h"
#ifdef __KERNEL_MODE__
#include <kernel/kernel.h>
#endif

#ifndef __X86__ // the declarations in this block are done elsewhere on x86

EXPORT_C TAny* operator new(TUint aSize) __NO_THROW
//
// The global new operator.
//
	{

	return STD_CLASS::Alloc(aSize);
	}

EXPORT_C TAny* operator new[](TUint aSize) __NO_THROW
    {

    return STD_CLASS::Alloc(aSize);
    }

EXPORT_C TAny* operator new(TUint aSize, TUint aExtraSize) __NO_THROW
//
// Allocate the requested size plus the extra.
//
	{

	return  STD_CLASS::Alloc(aSize + aExtraSize);
	}

EXPORT_C void operator delete(TAny* aPtr) __NO_THROW
//
// The replacement delete operator.
//
	{

	STD_CLASS::Free(aPtr);
	}

EXPORT_C void operator delete[](TAny* aPtr) __NO_THROW
    {

	STD_CLASS::Free(aPtr);
    }

#endif //!defined(__X86__)

#ifdef __ARMCC__

EXPORT_C TAny* operator new(TUint aSize, const std::nothrow_t& aNoThrow) __NO_THROW
//
// The global new operator.
//
	{
	(void)aNoThrow;
	return STD_CLASS::Alloc(aSize);
	}

EXPORT_C TAny* operator new[](TUint aSize, const std::nothrow_t& aNoThrow) __NO_THROW
    {
    (void)aNoThrow;
    return STD_CLASS::Alloc(aSize);
    }

#endif

