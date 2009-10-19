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
#include <e32std_private.h>


extern "C" {

IMPORT_C void __rt_div0(); // Import from drtrvct.dll.

EXPORT_C int __aeabi_idiv0 (int return_value)
    {
    __rt_div0();
    return return_value;
    }

EXPORT_C long long __aeabi_ldiv0 (long long return_value)
    {
    __rt_div0();
    return return_value;
    }
}

