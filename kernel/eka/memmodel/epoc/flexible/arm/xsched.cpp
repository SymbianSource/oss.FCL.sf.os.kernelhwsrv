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
//

#include "arm_mem.h"
#include "mpager.h"

#define iMState		iWaitLink.iSpare1

#ifdef __REQUEST_COMPLETE_MACHINE_CODED__

void DoRequestComplete(DThread* aThread, TRequestStatus* aStatus, TInt aReason);

void DThread::RequestComplete(TRequestStatus*& aStatus, TInt aReason)
//
// Signal this threads request semaphore.
// Enter with system locked, return with system unlocked.
//
	{
	DThread* thread = TheCurrentThread;
	TRequestStatus* status = aStatus;
	aStatus = NULL;

#ifdef _DEBUG
	if (KDebugNum(KFORCEKUPAGEFAULTS))
		{
		NKern::UnlockSystem();
		TInt r = ThePager.FlushRegion((DMemModelProcess*)thread->iOwningProcess,
									  (TLinAddr)status, sizeof(TRequestStatus));
		(void)r; // ignore errors
		NKern::LockSystem();
		}
#endif
	
	TInt pagingFault;
	do
		{
		XTRAP_PAGING_START(pagingFault);
		CHECK_PAGING_SAFE;
		thread->iIpcClient = this;
		DoRequestComplete(this, status,aReason);
		thread->iIpcClient = NULL;
		XTRAP_PAGING_END;
		}
	while (pagingFault > 0); // ignore errors
	}

#if defined(_DEBUG)
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

