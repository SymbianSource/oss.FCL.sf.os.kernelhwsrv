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
// function the runtime can call to exit process, 'finalizing'
// the runtime library but avoiding atexit processing.
// 
//

#include <e32std.h>
#include <e32std_private.h>

extern "C" {
IMPORT_C extern void __rt_lib_shutdown(void);

EXPORT_C void __rt_exit(TInt aReturnCode) /* never returns */
    {
    __rt_lib_shutdown();
    User::Exit(aReturnCode);
    }
}
