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
// This file is part of drtrvct.dll and drtrvct_vfpv2.dll.
//

#include <e32std.h>

#if __ARMCC_VERSION < 300000

extern "C"
{

IMPORT_C void _fp_init();
extern void __cpp_initialise();

EXPORT_C void __rt_lib_shutdown()
	{
	}

EXPORT_C void __rt_lib_init()
    {
    _fp_init();
    __cpp_initialise();
    }
}

#endif
