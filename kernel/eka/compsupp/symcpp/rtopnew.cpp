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

EXPORT_C TAny* operator new(TUint aSize) __NO_THROW
//
// The global new operator.
//
	{
	return User::Alloc(aSize);
	}

EXPORT_C TAny* operator new[](TUint aSize) __NO_THROW
	{
	return User::Alloc(aSize);
	}

EXPORT_C TAny* operator new(TUint aSize, const std::nothrow_t& aNoThrow) __NO_THROW
//
// The global new operator.
//
	{
	(void) aNoThrow;
	return User::Alloc(aSize);
	}

EXPORT_C TAny* operator new[](TUint aSize, const std::nothrow_t& aNoThrow) __NO_THROW
	{
	(void) aNoThrow;
	return User::Alloc(aSize);
	}

EXPORT_C TAny* operator new(TUint aSize,TUint anExtraSize) __NO_THROW
//
// Allocate the requested size plus the extra.
//
	{
	return User::Alloc(aSize + anExtraSize);
	}
