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
//

#include <memmodel.h>
#include "mmu/mm.h"
#include "mmboot.h"


/********************************************
 * Thread
 ********************************************/

TInt DThread::AllocateSupervisorStack()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DThread::AllocateSupervisorStack %O %x",this,iSupervisorStackSize));
	iSupervisorStackSize = MM::RoundToPageSize(iSupervisorStackSize);
	if(iThreadType==EThreadInitial)
		return KErrNone;

	TUint guardSize = PP::SupervisorThreadStackGuard;
	TUint virtualSize = guardSize+iSupervisorStackSize;
	DMemoryObject* memory;
	TInt r = MM::MemoryNew(memory,EMemoryObjectUnpaged,MM::BytesToPages(virtualSize));
	if(r==KErrNone)
		{
		r = MM::MemoryAlloc(memory,MM::BytesToPages(guardSize),MM::BytesToPages(iSupervisorStackSize));
		if(r==KErrNone)
			{
			DMemoryMapping* mapping;
			r = MM::MappingNew(mapping,memory,ESupervisorReadWrite,KKernelOsAsid);
			if(r==KErrNone)
				{
				__NK_ASSERT_DEBUG(!((DMemModelThread*)this)->iKernelStackMapping);
				((DMemModelThread*)this)->iKernelStackMapping = mapping;
				iSupervisorStack = (TAny*)(MM::MappingBase(mapping)+guardSize);
				__KTRACE_OPT(KTHREAD,Kern::Printf("Supervisor stack at %x, size %x",iSupervisorStack,iSupervisorStackSize));
				}
			}
		if(r!=KErrNone)
			MM::MemoryDestroy(memory);
		else
			{
#ifdef BTRACE_FLEXIBLE_MEM_MODEL
			BTrace8(BTrace::EFlexibleMemModel,BTrace::EMemoryObjectIsSupervisorStack,memory,&iNThread);
#endif
			}
		}
	return r;
	}


void DThread::FreeSupervisorStack()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DThread::FreeSupervisorStack %O",this));
	if(iThreadType==EThreadInitial) // !!!???
		return;
	MM::MappingAndMemoryDestroy(((DMemModelThread*)this)->iKernelStackMapping);
	}


TInt DThread::AllocateUserStack(TInt aSize, TBool aPaged)
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DThread::AllocateUserStack %O %x",this,aSize));
	aSize = MM::RoundToPageSize(aSize);
	if(aSize>PP::MaxUserThreadStack)
		return KErrTooBig;

	TMemoryObjectType memoryType = (aPaged)? EMemoryObjectPaged : EMemoryObjectMovable;
	TUint guardSize = PP::UserThreadStackGuard;
	TUint virtualSize = guardSize+aSize;
	// wipe user thread stack with 0x29292929
	TMemoryCreateFlags flags = (TMemoryCreateFlags)(EMemoryCreateDefault | EMemoryCreateUseCustomWipeByte | (0x29 << EMemoryCreateWipeByteShift));
	DMemoryObject* memory;
	TInt r = MM::MemoryNew(memory, memoryType, MM::BytesToPages(virtualSize),flags);
	if(r==KErrNone)
		{
		r = MM::MemoryAlloc(memory,MM::BytesToPages(guardSize),MM::BytesToPages(aSize));
		if(r==KErrNone)
			{
			DMemoryMapping* mapping;
			// Get os asid, no need to open a reference as this only invoked where 
			// the current thread is owned by iOwningProcess, iOwningProcess is 
			// the kernel process or this is the first thread of a process that 
			// isn't fully created yet.
			TUint osAsid = ((DMemModelProcess*)iOwningProcess)->OsAsid();
			r = MM::MappingNew(mapping,memory,EUserReadWrite,osAsid);
			if(r==KErrNone)
				{
				__NK_ASSERT_DEBUG(!((DMemModelThread*)this)->iUserStackMapping);
				((DMemModelThread*)this)->iUserStackMapping = mapping;
				iUserStackSize = aSize;
				iUserStackRunAddress = MM::MappingBase(mapping)+guardSize;
				__KTRACE_OPT(KTHREAD,Kern::Printf("User stack at %x, size %x",iUserStackRunAddress,iUserStackSize));
				}
			}
		if(r!=KErrNone)
			MM::MemoryDestroy(memory);
		else
			{
#ifdef BTRACE_FLEXIBLE_MEM_MODEL
			BTrace8(BTrace::EFlexibleMemModel,BTrace::EMemoryObjectIsUserStack,memory,&iNThread);
#endif
			}
		}
	return r;
	}


void DThread::FreeUserStack()
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DThread::FreeUserStack %O",this));
	MM::MappingAndMemoryDestroy(((DMemModelThread*)this)->iUserStackMapping);
	}


void DMemModelThread::SetPaging(TUint& aCreateFlags)
	{
	TUint pagingAtt = aCreateFlags & EThreadCreateFlagPagingMask;
	TUint dataPolicy = TheSuperPage().KernelConfigFlags() & EKernelConfigDataPagingPolicyMask;
	if (dataPolicy == EKernelConfigDataPagingPolicyNoPaging ||
		!(K::MemModelAttributes & EMemModelAttrDataPaging))
		{// No paging policy set or no data paging device installed.
		pagingAtt = EThreadCreateFlagUnpaged;
		}
	else if (dataPolicy == EKernelConfigDataPagingPolicyAlwaysPage)
		{
		pagingAtt = EThreadCreateFlagPaged;
		}
	else if (pagingAtt == EThreadCreateFlagPagingUnspec)
		{// No data paging attribute specified for this chunk so use the process's.
		if (iOwningProcess->iAttributes & DProcess::EDataPaged)
			pagingAtt = EThreadCreateFlagPaged;
		else
			pagingAtt = EThreadCreateFlagUnpaged;
		}
#ifdef _DEBUG
	else
		{
		__NK_ASSERT_DEBUG(	pagingAtt == EThreadCreateFlagPaged || 
							pagingAtt == EThreadCreateFlagUnpaged);
		}
#endif
	// Save the paging attributes for when the stack and heap are created later.
	aCreateFlags &= ~EThreadCreateFlagPagingMask;
	aCreateFlags |= pagingAtt;
	}


void DMemModelThread::DoExit1()
	{
	// Regularize the thread state before main exit processing.
	// We must remove any existing alias before we do any operation 
	// which requires aliasing (e.g. logon completion).
	__KTRACE_OPT(KTHREAD,Kern::Printf("DMemModelThread %O DoExit1",this));
	RemoveAlias();
	}


void DThread::IpcExcHandler(TExcTrap* aTrap, DThread* aThread, TAny* aContext)
	{
	aThread->iIpcClient = 0;
	TIpcExcTrap& xt=*(TIpcExcTrap*)aTrap;
	switch (xt.ExcLocation(aThread, aContext))
		{
		case TIpcExcTrap::EExcRemote:
			// problem accessing remote address - 'leave' so an error code will be returned
			((DMemModelThread*)aThread)->RemoveAlias();
			xt.Exception(KErrBadDescriptor);  // does not return

		case TIpcExcTrap::EExcLocal:
			// problem accessing local address - return and panic current thread as usual
			((DMemModelThread*)aThread)->RemoveAlias();
			return;

		default:
			// otherwise return and fault kernel
			NKern::LockSystem();
			return;
		}
	}


void DMemModelThread::BTracePrime(TInt aCategory)
	{
	DThread::BTracePrime(aCategory);
#ifdef BTRACE_FLEXIBLE_MEM_MODEL
	if(aCategory==BTrace::EFlexibleMemModel || aCategory==-1)
		{
		if (iKernelStackMapping)
			{
			DMemoryObject* memory = MM::MappingGetAndOpenMemory(iKernelStackMapping);
			if (memory)
				{
				MM::MemoryBTracePrime(memory);
				BTrace8(BTrace::EFlexibleMemModel,BTrace::EMemoryObjectIsSupervisorStack,memory,&iNThread);
				MM::MemoryClose(memory);
				}
			}
		if (iUserStackMapping)
			{
			DMemoryObject* memory = MM::MappingGetAndOpenMemory(iUserStackMapping);
			if (memory)
				{
				MM::MemoryBTracePrime(memory);
				BTrace8(BTrace::EFlexibleMemModel,BTrace::EMemoryObjectIsUserStack,memory,&iNThread);
				MM::MemoryClose(memory);
				}
			}
		}
#endif
	}
