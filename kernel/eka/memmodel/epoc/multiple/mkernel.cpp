// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\epoc\multiple\mkernel.cpp
// 
//

#include "memmodel.h"

/********************************************
 * Thread
 ********************************************/

TInt DThread::AllocateSupervisorStack()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DThread::AllocateSupervisorStack %O %x",this,iSupervisorStackSize));
	iSupervisorStackSize=Mmu::RoundToPageSize(iSupervisorStackSize);
	if (iThreadType!=EThreadInitial)
		{
		TInt offset=MM::SvStackChunk->Allocate(iSupervisorStackSize,PP::SupervisorThreadStackGuard);
		if (offset<0)
			{
			__KTRACE_FAIL(KErrNoMemory,Kern::Printf("ASS: %d",KErrNoMemory));
			return KErrNoMemory;
			}
		iSupervisorStack=MM::SvStackChunk->Base()+offset+PP::SupervisorThreadStackGuard;
		iSupervisorStackAllocated = TRUE;
		__KTRACE_OPT(KTHREAD,Kern::Printf("Supervisor stack at %x, size %x",iSupervisorStack,iSupervisorStackSize));
		}
	return KErrNone;
	}

void DMemModelThread::DoExit1()
	{
	// Regularize the thread state before main exit processing.
	// We must remove any existing alias before we do any operation 
	// which requires aliasing (e.g. logon completion).
	__KTRACE_OPT(KTHREAD,Kern::Printf("DMemModelThread %O DoExit1",this));
	NKern::LockSystem();
	RemoveAlias();
	NKern::UnlockSystem();
	}

void DThread::FreeSupervisorStack()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DThread::FreeSupervisorStack %O",this));
	if (iSupervisorStackAllocated)
		{
		TAny* pStack = __e32_atomic_swp_ord_ptr(&iSupervisorStack, 0);
		if (pStack && iThreadType!=EThreadInitial)
			{
			__KTRACE_OPT(KTHREAD,Kern::Printf("Freeing supervisor stack at %08x, size %x",pStack,iSupervisorStackSize));
			TInt offset=(TUint8*)pStack-MM::SvStackChunk->Base();
			TInt r=MM::SvStackChunk->Decommit(offset-PP::SupervisorThreadStackGuard,iSupervisorStackSize+PP::SupervisorThreadStackGuard);
			__ASSERT_DEBUG(r==KErrNone, MM::Panic(MM::EDecommitFailed));
			(void)r; //Supress the warning in urel build
			}
		}
	iSupervisorStackAllocated = FALSE;
	}

TInt DThread::AllocateUserStack(TInt aSize, TBool /*aPaged*/)
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DThread::AllocateUserStack %O %x",this,aSize));

	aSize=Mmu::RoundToPageSize(aSize);
	if (aSize>PP::MaxUserThreadStack)
		return KErrTooBig;
	TInt guard=PP::UserThreadStackGuard;
	DProcess* pP=iOwningProcess;
	NKern::LockSystem();
	DChunk* pC=pP->iDataBssStackChunk;
	if (!pC || pC->Open()!=KErrNone)	// so chunk doesn't disappear during Allocate()
		{
		NKern::UnlockSystem();
		__KTRACE_FAIL(KErrDied,Kern::Printf("AUS1: %d",KErrDied));
		return KErrDied;
		}
	NKern::UnlockSystem();
	TInt r=pC->Allocate(aSize,guard);
	TInt s=pC->Close(NULL);				// NULL since we didn't add chunk to process again
	if (s & EObjectDeleted)
		{
		__KTRACE_FAIL(KErrDied,Kern::Printf("AUS2: %d",KErrDied));
		return KErrDied;
		}
	if (r<0)
		{
		__KTRACE_FAIL(r,Kern::Printf("AUS3: %d",r));
		return r;
		}
	iUserStackSize=aSize;
	iUserStackRunAddress=pP->iDataBssRunAddress+r+guard;
	__KTRACE_OPT(KTHREAD,Kern::Printf("User stack run address at %x",iUserStackRunAddress));
	return KErrNone;
	}

void DThread::FreeUserStack()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DThread::FreeUserStack %O",this));
	TLinAddr usr_stack = (TLinAddr)__e32_atomic_swp_ord_ptr(&iUserStackRunAddress, 0);
	if (usr_stack)
		{
		__KTRACE_OPT(KTHREAD,Kern::Printf("Freeing user stack at %x",usr_stack));
		DMemModelProcess* pP=(DMemModelProcess*)iOwningProcess;
		NKern::LockSystem();
		DMemModelChunk* pC=(DMemModelChunk*)pP->iDataBssStackChunk;
		if (!pC || pC->Open()!=KErrNone)	// so chunk doesn't disappear during Decommit()
			{
			NKern::UnlockSystem();
			__KTRACE_FAIL(KErrDied,Kern::Printf("FUS"));
			return;
			}
		NKern::UnlockSystem();
		TInt offset=usr_stack-pP->iDataBssRunAddress;
		TInt r=pC->Decommit(offset-PP::UserThreadStackGuard,iUserStackSize+PP::UserThreadStackGuard);
		__ASSERT_DEBUG(r==KErrNone, MM::Panic(MM::EDecommitFailed));
		pC->Close(NULL);			// NULL since we didn't add chunk to process again
		(void)r; //Supress the warning in urel build
		}
	}

void DThread::IpcExcHandler(TExcTrap* aTrap, DThread* aThread, TAny* aContext)
	{
	aThread->iIpcClient = 0;
	TIpcExcTrap& xt=*(TIpcExcTrap*)aTrap;
	switch (xt.ExcLocation(aThread, aContext))
		{
		case TIpcExcTrap::EExcRemote:
			// problem accessing remote address - 'leave' so an error code will be returned
			NKern::LockSystem();
			((DMemModelThread*)aThread)->RemoveAlias();
			xt.Exception(KErrBadDescriptor);  // does not return

		case TIpcExcTrap::EExcLocal:
			// problem accessing local address - return and panic current thread as usual
			NKern::LockSystem();
			((DMemModelThread*)aThread)->RemoveAlias();
			NKern::UnlockSystem();
			return;

		default:
			// otherwise return and fault kernel
			NKern::LockSystem();
			return;
		}
	}
