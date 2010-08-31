// Copyright (c) 2009-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkern\nk_mon.cpp
// Kernel crash debugger - NKERN platform-independent portion
// 
//

#include <kernel/monitor.h>
#include "nk_priv.h"

extern void DisplayNThreadPlatformSpecific(Monitor& m, NThread* pT);


EXPORT_C void Monitor::DisplayNThreadInfo(NThread* pT)
	{
	TBuf8<80> buf=_L8("NThread @ ");
	buf.AppendNumFixedWidth((TUint)pT,EHex,8);
	buf+=_L8(" Pri ");
	buf.AppendNum((TUint)pT->iPriority);
	buf+=_L8(" NState ");
	switch (pT->iSpare1)
		{
		case NThread::EReady: buf+=_L8("READY"); break;
		case NThread::ESuspended: buf+=_L8("SUSPENDED"); break;
		case NThread::EWaitFastSemaphore: buf+=_L8("WAITFSEM "); buf.AppendNumFixedWidth((TUint)pT->iWaitObj,EHex,8); break;
		case NThread::ESleep: buf+=_L8("SLEEP"); break;
		case NThread::EBlocked: buf+=_L8("BLOCKED"); break;
		case NThread::EDead: buf+=_L8("DEAD"); break;
		case NThread::EWaitDfc: buf+=_L8("WAITDFC"); break;
		default: buf+=_L8("??"); buf.AppendNumFixedWidth((TUint)pT->iSpare1,EHex,8); break;
		}
	PrintLine(buf);
	Printf("SavedSP=%08x Next=%08x Prev=%08x Att=%02x\r\n",pT->iSavedSP,pT->iNext,pT->iPrev,pT->iSpare2);
	Printf("HeldFM=%08x WaitFM=%08x AddrSp=%08x\r\n",pT->iHeldFastMutex,pT->iWaitFastMutex,pT->iAddressSpace);
	Printf("Time=%d Timeslice=%d ReqCount=%d\r\n",pT->iTime,pT->iTimeslice,pT->iRequestSemaphore.iCount);
	Printf("SuspendCount=%d CsCount=%d CsFunction=%08x\r\n",pT->iSuspendCount,pT->iCsCount,pT->iCsFunction);
	Printf("iUserModeCallbacks=%08x iSpare7=%08x iSpare8=%08x\r\n", pT->iUserModeCallbacks, pT->iSpare7, pT->iSpare8);
	DisplayNThreadPlatformSpecific(*this, pT);
	NewLine();
	}

void Monitor::DisplayNFastSemInfo(NFastSemaphore* pS)
	{
	Printf("NFastSemaphore @ %08x Count %d OwningThread %08x\r\n",pS,pS->iCount,pS->iOwningThread);
	}

void Monitor::DisplayNFastMutexInfo(NFastMutex* pM)
	{
	Printf("NFastMutex @ %08x HoldingThread %08x iWaiting %08x\r\n",pM,pM->iHoldingThread,pM->iWaiting);
	}

void Monitor::DisplaySchedulerInfo()
	{
	TScheduler* pS=TScheduler::Ptr();
	Printf("SCHEDULER @%08x: CurrentThread %08x\r\n",pS,pS->iCurrentThread);
	Printf("RescheduleNeeded=%02x DfcPending=%02x KernCSLocked=%08x\r\n",pS->iRescheduleNeededFlag,pS->iDfcPendingFlag,pS->iKernCSLocked);
	Printf("DFCS: next %08x prev %08x\r\n",pS->iDfcs.iA.iNext,pS->iDfcs.iA.iPrev);
	Printf("ProcessHandler=%08x, AddressSpace=%08x\r\n",pS->iProcessHandler,pS->iAddressSpace);
	Printf("SYSLOCK: HoldingThread %08x iWaiting %08x\r\n",pS->iLock.iHoldingThread,pS->iLock.iWaiting);
	Printf("Extras 0: %08x 1: %08x 2: %08x 3: %08x\r\n",pS->iExtras[0],pS->iExtras[1],pS->iExtras[2],pS->iExtras[3]);
	Printf("Extras 4: %08x 5: %08x 6: %08x 7: %08x\r\n",pS->iExtras[4],pS->iExtras[5],pS->iExtras[6],pS->iExtras[7]);
	Printf("Extras 8: %08x 9: %08x A: %08x B: %08x\r\n",pS->iExtras[8],pS->iExtras[9],pS->iExtras[10],pS->iExtras[11]);
	Printf("Extras C: %08x D: %08x E: %08x F: %08x\r\n",pS->iExtras[12],pS->iExtras[13],pS->iExtras[14],pS->iExtras[15]);
	}

