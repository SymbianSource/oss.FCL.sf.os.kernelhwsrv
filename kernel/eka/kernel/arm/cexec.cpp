// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\arm\cexec.cpp
// 
//

#include <arm.h>
#include <kernel/cache.h>

GLREF_C TInt CalcKernelHeapUsed();
void GetLatencyValues(TInt aMode, TInt& aCount, TInt* aDest);
void KernMsgTest();
void InvalidExecHandler();
void InvalidFastExec();
void PreprocessHandler();

#include "execs.h"

/***********************************************************************************
 * User-side executive handlers
 ***********************************************************************************/

#ifdef KEXEC
LOCAL_C TLinAddr UserReturnAddress()
	{
	TUint32* svcStackTop=(TUint32*)((TInt)TheCurrentThread->iSupervisorStack+TheCurrentThread->iSupervisorStackSize-4);
	return *svcStackTop;
	}
#endif

TInt ExecHandler::BreakPoint()
	{
	// Don't implement without considering Platform Security!
	__KTRACE_OPT(KEXEC,Kern::Printf("ExecHandler::Breakpoint"));
	return 0;
	}

TInt ExecHandler::ProfileStart(TInt aProfile)
	{
	// Don't implement without considering Platform Security!
	__KTRACE_OPT(KEXEC,Kern::Printf("ExecHandler::ProfileStart(%d) called from %08x",
		aProfile,UserReturnAddress()));
	return 0;
	}

TInt ExecHandler::ProfileEnd(TInt aProfile)
	{
	// Don't implement without considering Platform Security!
	__KTRACE_OPT(KEXEC,Kern::Printf("ExecHandler::ProfileEnd(%d) called from %08x",
		aProfile,UserReturnAddress()));
	return 0;
	}

/***********************************************************************************
 * Exec dispatch code
 ***********************************************************************************/

void InvalidExecHandler()
	{
	K::PanicKernExec(EInvalidSystemCall);
	}

void GetLatencyValues(TInt /*aMode*/, TInt& /*aCount*/, TInt* /*aDest*/)
	{
	}

void dummyMsg(TAny*);
TMessageQue DummyMsgQ(dummyMsg,NULL,NULL,1);

GLDEF_C void InitDummyMsgQ()
	{
	DummyMsgQ.SetDfcQ(K::DfcQ0);
	DummyMsgQ.Receive();
	}

void dummyMsg(TAny*)
	{
	DummyMsgQ.iMessage->Complete(KErrNone,ETrue);
	}

void KernMsgTest()
	{
	TMessageBase& m=Kern::Message();
	m.SendReceive(&DummyMsgQ);
	}

TInt ExecHandler::SetFloatingPointMode(TFloatingPointMode aMode, TFloatingPointRoundingMode aRoundingMode)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("ExecHandler::SetFloatingPointMode(%d, %d)", aMode, aRoundingMode));
#ifdef __CPU_HAS_VFP
	if (TUint(aRoundingMode)>=TUint(EFpRoundNumModes))
		return KErrArgument;
	switch (aMode)
		{
		case EFpModeRunFast:
			Arm::ModifyFpScr(VFP_FPSCR_EXCEPTIONS|VFP_FPSCR_RMODE_MASK, VFP_FPSCR_RUNFAST|(aRoundingMode<<VFP_FPSCR_RMODE_SHIFT));
			return KErrNone;
		case EFpModeIEEENoExceptions:
#ifndef __VFP_V3
			if (Arm::VfpBounceHandler)
#endif
				{
				Arm::ModifyFpScr(VFP_FPSCR_RUNFAST|VFP_FPSCR_EXCEPTIONS|VFP_FPSCR_RMODE_MASK, aRoundingMode<<VFP_FPSCR_RMODE_SHIFT);
				return KErrNone;
				}
#ifndef __VFP_V3
			else
				return KErrNotSupported;
#endif
		default:
			return KErrArgument;
		}
#else
	return KErrNotSupported;
#endif
	}
