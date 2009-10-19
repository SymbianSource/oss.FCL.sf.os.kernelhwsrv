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
// e32\kernel\x86\cexec.cpp
// 
//

#include <x86_mem.h>
#include "u32std.h"
#include <kernel/cache.h>

GLREF_C TInt CalcKernelHeapUsed();
void GetLatencyValues(TInt aMode, TInt& aCount, TInt* aDest);
void KernMsgTest();
void InvalidExecHandler();
void InvalidFastExec();
void PreprocessHandler();


/***********************************************************************************
 * User-side executive handlers
 ***********************************************************************************/
#include "execs.h"

#ifdef KEXEC
TLinAddr UserReturnAddress()
	{
	TUint32* svcStackTop=(TUint32*)((TInt)TheCurrentThread->iSupervisorStack+TheCurrentThread->iSupervisorStackSize-4);
	return svcStackTop[-4];
	}
#endif

TInt ExecHandler::BreakPoint()
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("ExecHandler::Breakpoint"));
	return 0;
	}

TInt ExecHandler::ProfileStart(TInt aProfile)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("ExecHandler::ProfileStart(%d) called from %08x",
		aProfile,UserReturnAddress()));
	return 0;
	}

TInt ExecHandler::ProfileEnd(TInt aProfile)
	{
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


TInt ExecHandler::SetFloatingPointMode(TFloatingPointMode /*aMode*/, TFloatingPointRoundingMode /*aRoundingMode*/)
	{
	return KErrNotSupported;
	}

