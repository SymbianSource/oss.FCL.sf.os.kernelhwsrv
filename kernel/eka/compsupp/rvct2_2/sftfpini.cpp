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
//

#include <e32std.h>
#include <e32std_private.h>

// support functions for ARM supplied softvfp compiler-helper functions

extern "C" {

//void __rt_fp_status_register_cleanup(TAny * aReg)
//	{
//	delete aReg;
//	}

// set up the fp status register 
EXPORT_C  void _fp_init(void) 
	{
	TUint32 * aReg = (TUint32 *)User::AllocZ(sizeof(TUint32));
	if (aReg) 
		{
		Dll::SetTls(aReg /*, __rt_fp_status_register_cleanup*/);
		}
	else
		{
		// This will force us to try again if we actually get an FP 
		// exception later.
		Dll::SetTls(0 /*, __rt_fp_status_register_cleanup*/);
		}
	}
  
EXPORT_C TAny * __rt_fp_status_addr(void) 
	{ 
	//return &__fp_status_register; 
	TAny* aTls = Dll::Tls();
	if (aTls)
		return aTls;
	// we obviously failed to set it up before. Try again, so we can
	// at least try to error meaningfully
	TUint32* aReg = (TUint32*)User::AllocZ(sizeof(TUint32));
	_LIT(KFpGeneralPanic, "FP Emulator");
	if (aReg) 
		{
		TInt r = Dll::SetTls(aReg /*, __rt_fp_status_register_cleanup*/);
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

