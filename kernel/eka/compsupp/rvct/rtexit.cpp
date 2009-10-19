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


extern "C" {

// The run-time system can call this function to exit the process, 'finalizing' the
// run-time library but avoiding atexit processing.
EXPORT_C void __rt_exit(TInt aReturnCode) /* never returns */
    {
    User::Exit(aReturnCode);
    }


} // extern "C"
