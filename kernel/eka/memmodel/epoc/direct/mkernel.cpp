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
// e32\memmodel\epoc\direct\mkernel.cpp
// 
//

#include <memmodel.h>

EXPORT_C TInt Kern::FreeRamInBytes()
	{
	return MM::RamAllocator->Avail()<<MM::RamBlockShift;
	}

/********************************************
 * Thread
 ********************************************/

TInt DThread::AllocateSupervisorStack()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DThread::AllocateSupervisorStack %O %x",this,iSupervisorStackSize));
	iSupervisorStackSize=MM::RoundToBlockSize(iSupervisorStackSize);
	if (iThreadType!=EThreadInitial)
		{
		TAny* p=Kern::Alloc(iSupervisorStackSize);	// just allocate on kernel heap
		if (!p)
			return KErrNoMemory;
		iSupervisorStack=p;
		iSupervisorStackAllocated = TRUE;
		__KTRACE_OPT(KTHREAD,Kern::Printf("Supervisor stack at %x, size %x",iSupervisorStack,iSupervisorStackSize));
		}
	return KErrNone;
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
			Kern::Free(pStack);
			}
		}
	iSupervisorStackAllocated = FALSE;
	}

TInt DThread::AllocateUserStack(TInt aSize, TBool /*aPaged*/)
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DThread::AllocateUserStack %O %x",this,aSize));

	aSize=MM::RoundToBlockSize(aSize);
	if (aSize>PP::MaxUserThreadStack)
		return KErrTooBig;
	TInt r;
	MM::WaitRamAlloc();
	r=MM::AllocRegion(iUserStackRunAddress, aSize);
	if (r==KErrNone)
		iUserStackSize=aSize;
	else
		{
		iUserStackSize=0;
		MM::AllocFailed=ETrue;
		}
	MM::SignalRamAlloc();
	__KTRACE_OPT(KTHREAD,Kern::Printf("User stack run address at %x",iUserStackRunAddress));
	return r;
	}

void DThread::FreeUserStack()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DThread::FreeUserStack %O",this));
	TLinAddr usr_stack_size = (TLinAddr)__e32_atomic_swp_ord32(&iUserStackSize, 0);
	if (usr_stack_size)
		{
		__KTRACE_OPT(KTHREAD,Kern::Printf("Freeing user stack at %x+%x",iUserStackRunAddress,usr_stack_size));
		MM::WaitRamAlloc();
		MM::FreeRegion(iUserStackRunAddress, usr_stack_size);
		iUserStackRunAddress=0;
		MM::SignalRamAlloc();
		}
	}

