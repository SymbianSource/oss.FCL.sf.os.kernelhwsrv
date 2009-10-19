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
// This file is part of drtaeabi.dll.
// 
//

#include <e32std.h>


extern "C"
{
EXPORT_C void __aeabi_idiv0()
	{
	User::RaiseException(EExcIntegerDivideByZero);
	}

EXPORT_C void __aeabi_ldiv0()
	{
	__aeabi_idiv0();
	}
}
