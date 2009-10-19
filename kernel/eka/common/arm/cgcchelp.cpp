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
// e32\common\arm\cgcchelp.cpp
// 
//

#include "../common.h"
#ifdef __KERNEL_MODE__
#include <nkern.h>
#endif

extern "C" {
extern "C" void __division_by_zero()
	{
	DIVISION_BY_ZERO();
    }

#ifdef __GCCV3__
EXPORT_C int __cxa_pure_virtual()
#else
EXPORT_C int __pure_virtual()
#endif
//
// Gets called for any unreplaced pure virtual methods.
//
	{
#ifdef __STANDALONE_NANOKERNEL__
	__crash();
#else
	Panic(EPureVirtualCalled);
#endif
	return 0;
	}
}

