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
// This file is part of drtrvct.dll and drtrvct_vfpv2.dll.
// 
//

#include <e32std.h>


extern "C" {
  
// TODO: Can this be removed?
EXPORT_C  void _fp_init() 
    {
    }
  
// TODO: Is this ever used?
EXPORT_C TAny * __rt_fp_status_addr() 
    { 
    TAny* aTls = Dll::Tls();

    if (aTls)
        return aTls;

    // If this is the first time we are called we need to set up some TLS for
    // the FP status word.

    TUint32* aReg = (TUint32*)User::AllocZ(sizeof(TUint32));

    _LIT(KFpGeneralPanic, "FP Emulator");

    if (aReg) 
        {
        TInt r = Dll::SetTls(aReg);
        if (r==KErrNone)
            return aReg;

        // if we get here we really in trouble. Just Panic.
        User::Panic(KFpGeneralPanic, KErrGeneral);
        }
    else
        {
        // If we get here, we're toast anyway....
        User::Panic(KFpGeneralPanic, KErrNoMemory);
        }

    return 0;
    }

}
