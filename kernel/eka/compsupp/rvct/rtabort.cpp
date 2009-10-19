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
// This file is part of drtrvct.dll and drtrvct_vfpv2.dll
// 
//

#include <e32std.h>


// The run-time can call this function to abort a process. The run-time library is
// not finalized and no atexit processing takes places.
extern "C" EXPORT_C void __rt_abort() /* never returns */
    {
    RThread().Kill(KErrAbort);
    }

EXPORT_C void std::terminate() 
    {
    __rt_abort();
    }
