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

#if defined(__ARMCC__) && __ARMCC_VERSION >= 400000
__asm void _hide_unwanted_exports()
	{
	IMPORT _vfp_f2h_single;
	IMPORT _vfp_fpe_d2f;
	IMPORT _vfp_fpe_d2f_quiet;
	IMPORT _vfp_fpe_dabs;
	IMPORT _vfp_fpe_dadd;
	IMPORT _vfp_fpe_dcmp;
	IMPORT _vfp_fpe_dcmpe;
	IMPORT _vfp_fpe_ddiv;
	IMPORT _vfp_fpe_dfcmp;
	IMPORT _vfp_fpe_dfcmpe;
	IMPORT _vfp_fpe_dfix;
	IMPORT _vfp_fpe_dfix_z;
	IMPORT _vfp_fpe_dfixll;
	IMPORT _vfp_fpe_dfixll_z;
	IMPORT _vfp_fpe_dfixllp;
	IMPORT _vfp_fpe_dfixu;
	IMPORT _vfp_fpe_dfixu_z;
	IMPORT _vfp_fpe_dfixull;
	IMPORT _vfp_fpe_dfixull_z;
	IMPORT _vfp_fpe_dflt;
	IMPORT _vfp_fpe_dfltll;
	IMPORT _vfp_fpe_dfltll_scaled;
	IMPORT _vfp_fpe_dfltllp;
	IMPORT _vfp_fpe_dfltu;
	IMPORT _vfp_fpe_dfltull;
	IMPORT _vfp_fpe_dmul;
	IMPORT _vfp_fpe_dneg;
	IMPORT _vfp_fpe_drdiv;
	IMPORT _vfp_fpe_drem;
	IMPORT _vfp_fpe_drnd;
	IMPORT _vfp_fpe_drsb;
	IMPORT _vfp_fpe_dsqrt;
	IMPORT _vfp_fpe_dsub;
	IMPORT _vfp_fpe_f2d;
	IMPORT _vfp_fpe_f2d_quiet;
	IMPORT _vfp_fpe_f2h;
	IMPORT _vfp_fpe_fabs;
	IMPORT _vfp_fpe_fadd;
	IMPORT _vfp_fpe_fcmp;
	IMPORT _vfp_fpe_fcmpe;
	IMPORT _vfp_fpe_fdcmp;
	IMPORT _vfp_fpe_fdcmpe;
	IMPORT _vfp_fpe_fdiv;
	IMPORT _vfp_fpe_ffix;
	IMPORT _vfp_fpe_ffix_z;
	IMPORT _vfp_fpe_ffixll;
	IMPORT _vfp_fpe_ffixll_z;
	IMPORT _vfp_fpe_ffixllp;
	IMPORT _vfp_fpe_ffixu;
	IMPORT _vfp_fpe_ffixu_z;
	IMPORT _vfp_fpe_ffixull;
	IMPORT _vfp_fpe_ffixull_z;
	IMPORT _vfp_fpe_fflt;
	IMPORT _vfp_fpe_fflt_scaled;
	IMPORT _vfp_fpe_ffltll;
	IMPORT _vfp_fpe_ffltll_scaled;
	IMPORT _vfp_fpe_ffltllp;
	IMPORT _vfp_fpe_ffltu;
	IMPORT _vfp_fpe_ffltull;
	IMPORT _vfp_fpe_fma;
	IMPORT _vfp_fpe_fmaf;
	IMPORT _vfp_fpe_fmul;
	IMPORT _vfp_fpe_fneg;
	IMPORT _vfp_fpe_frdiv;
	IMPORT _vfp_fpe_frem;
	IMPORT _vfp_fpe_frnd;
	IMPORT _vfp_fpe_frsb;
	IMPORT _vfp_fpe_fsqrt;
	IMPORT _vfp_fpe_fsub;
	IMPORT _vfp_fpe_h2f;
	IMPORT _vfp_fpe_hcmp;
	IMPORT _vfp_fpe_IEEE;
	IMPORT _vfp_fpe_IEEE_rd;
	IMPORT _vfp_fpe_IEEE_ru;
	IMPORT _vfp_fpe_IEEE_rz;
	IMPORT _vfp_fpe_ilogb;
	IMPORT _vfp_fpe_ilogbf;
	IMPORT _vfp_fpe_logb;
	IMPORT _vfp_fpe_logbf;
	IMPORT _vfp_fpe_nextafter;
	IMPORT _vfp_fpe_nextafterf;
	IMPORT _vfp_fpe_nexttowardf;
	IMPORT _vfp_fpe_RunFast;
	IMPORT _vfp_fpe_RunFast_oldfz;
	IMPORT _vfp_fpe_scalbn;
	IMPORT _vfp_fpe_scalbnf;
	IMPORT _vfp_h2f_single;
	}
#endif

