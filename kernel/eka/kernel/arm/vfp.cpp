// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\evfp.cpp
// 
//

#include <kernel/kern_priv.h>
#include <arm.h>
#include <vfpsupport.h>
#include <arm_vfp.h>


// Trap support for ARM's vfp engine - this is called by the ARM vfpsupport
// library when it wants to raise a floating point exception in the user thread.
// We don't currently handle floating point exceptions in VFP code, and
// never enable them for the user, so if the user has enabled them and we've
// ended up here, we panic their thread.
void _vfp_fp_trap()
	{
	__KTRACE_OPT(KPANIC, Kern::Printf("ARM VFP support library raised a floating point exception, killing user thread"));
	Kern::PanicCurrentThread(KLitKernExec, ECausedException);
	}	


// Undefined instruction handler for VFP bounces, called by the kernel
// when the undefined instruction is a coprocessor instruction for
// CP10 or CP11.
TInt HandleBounce(TArmExcInfo* aExcInfo)
	{
	// iFaultAddress in aExcInfo is actually the instruction that bounced

	if (!VFPIsComputeException(aExcInfo->iFaultAddress))
		{
		// EX bit is clear, this is a real undefined instruction
		return KErrGeneral;
		}
	
	// Get a computation description buffer to feed to ARM's vfp library
	TVFPComputationDescription desc;
	TBool skipInstruction = VFPCollectTrapDescription(desc,aExcInfo->iFaultAddress);

	// Invoke ARM VFP computation engine to do the work
	_VFP_Computation_Engine(desc);

	if (skipInstruction)
		{
		if (aExcInfo->iCpsr & 0x20) // if in thumb mode
			aExcInfo->iR15 += 2; // skip 2 bytes, 1 thumb instruction
		else
			aExcInfo->iR15 += 4; // skip 4 bytes, 1 ARM instruction
		}
	
	return KErrNone;
	}


DECLARE_STANDARD_EXTENSION()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("Enabling VFPv2 bounce support extension"));
	Arm::SetVfpBounceHandler(HandleBounce);
	Arm::SetVfpDefaultFpScr(VFP_FPSCR_IEEE_NO_EXC);
	return KErrNone;
	}


// RVCT 3.1 vfpsupport library expects the CRT abort() function to be present.
#ifdef __ARMCC_3_1__
extern "C" void abort()
	{
	Kern::PanicCurrentThread(_L("CRT_ABORT"), 0);
	}
#endif
