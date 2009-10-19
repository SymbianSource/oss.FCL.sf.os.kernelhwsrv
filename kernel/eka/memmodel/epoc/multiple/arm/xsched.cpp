// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\epoc\multiple\arm\xsched.cpp
// 
//

#include "arm_mem.h"

#define iMState		iWaitLink.iSpare1

#ifdef __REQUEST_COMPLETE_MACHINE_CODED__
#if defined(_DEBUG)
extern "C" void __DebugMsgDThreadRequestComplete(TInt a0, TInt a1)
	{
	DThread* pT=(DThread*)a0;
	__KTRACE_OPT(KDATAPAGEWARN,Kern::Printf("Data paging: Use of deprecated DThread::RequestComplete API by %O at %08x", pT, a1));
	}

extern "C" void __DebugMsgRequestComplete(TInt a0, TInt a1, TInt a2)
	{
	DThread* pT=(DThread*)a0;
	__KTRACE_OPT(KTHREAD,Kern::Printf("Thread %O RequestComplete %08x %d",pT,a1,a2));
	}

extern "C" void __DebugMsgReqCompleteWrite(TInt a0, TInt a1, TInt a2)
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("Writing %d to %08x",a2,a0+a1));
	}
#endif
#endif

