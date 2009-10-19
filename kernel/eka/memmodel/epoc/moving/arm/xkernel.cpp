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
// e32\memmodel\epoc\moving\arm\xkernel.cpp
// 
//

#include "arm_mem.h"


/********************************************
 * Thread
 ********************************************/

TInt DArmPlatThread::SetupContext(SThreadCreateInfo& anInfo)
	{
	switch(iThreadType)
		{
		case EThreadSupervisor:
		case EThreadMinimalSupervisor:
			iNThread.SetDacr(Arm::DefaultDomainAccess);
		case EThreadInitial:
			break;
		case EThreadUser:
			{
			DArmPlatProcess* pP=(DArmPlatProcess*)iOwningProcess;
			iNThread.SetDacr(pP->iDacr);
			if (pP->iAttributes & DMemModelProcess::EVariableAccess)
				iNThread.SetAttributes(KThreadAttImplicitSystemLock|KThreadAttAddressSpace);
#ifndef __SMP__
			iNThread.iSpare3 /*iUserContextType*/ = NThread::EContextUndefined;
#endif
			break;
			}
		}
	iNThread.SetAddressSpace(iOwningProcess);
	__KTRACE_OPT(KTHREAD,Kern::Printf("Thread %O DACR %08x Attrib %02x",this,iNThread.Dacr(),iNThread.Attributes()));
	return KErrNone;
	}

DArmPlatProcess::DArmPlatProcess()
	: iDomain(-1), iDacr(Arm::DefaultDomainAccess|0x3)	// manager access to domain 0
	{}

DArmPlatProcess::~DArmPlatProcess()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DArmPlatProcess destruct, Domain=%d",iDomain));
	if (iDomain>=0)
		ArmMmu::FreeDomain(iDomain);
	DMemModelProcess::Destruct();
	}

TInt DArmPlatProcess::GetNewChunk(DMemModelChunk*& aChunk, SChunkCreateInfo& aInfo)
	{
	aChunk=NULL;
	DArmPlatChunk* pC=new DArmPlatChunk;
	if (!pC)
		return KErrNoMemory;
	aChunk=pC;
	pC->iChunkType=aInfo.iType;
	switch(pC->iChunkType)
		{
		case EKernelData:
		case EKernelStack:
		case EKernelCode:
		case EKernelMessage:
			pC->iAttributes |= DMemModelChunk::EFixedAccess;
			break;
		case ERamDrive:
			pC->iDomain=3;
			pC->iAttributes |= DMemModelChunk::EFixedAccess;
			break;
		case EUserCode:
		case EUserSelfModCode:
			pC->iAttributes |= DMemModelChunk::EFixedAddress;
			if (iDomain>=0)
				{
				__KTRACE_OPT(KMMU,Kern::Printf("DArmPlatChunk create code chunk, owning process domain %d",iDomain));
				pC->iDomain=iDomain;
				pC->iAttributes |= DMemModelChunk::EFixedAccess;
				}
			break;
		case EDll:
			break;
		case EUserData:
		case EDllData:
			if (aInfo.iGlobal && (iAttributes & DMemModelProcess::EFixedAddress || aInfo.iForceFixed))
				{
				TInt domain=ArmMmu::AllocDomain();
				if (domain>=0)
					{
					__KTRACE_OPT(KMMU,Kern::Printf("DArmPlatChunk create global chunk, Domain %d allocated",domain));
					pC->iDomain=domain;
					pC->iAttributes |= DMemModelChunk::EFixedAccess;
					}
				}
			else if (!aInfo.iGlobal && iDomain>=0)
				{
				__KTRACE_OPT(KMMU,Kern::Printf("DArmPlatChunk create local chunk, owning process domain %d",iDomain));
				pC->iDomain=iDomain;
				pC->iAttributes |= DMemModelChunk::EFixedAccess;
				}
			break;
		case ESharedKernelSingle:
		case ESharedKernelMultiple:
		case ESharedIo:
			break;
		default:
			FAULT();
		}
	return KErrNone;
	}

DArmPlatChunk::DArmPlatChunk()
	: iDomain(-1)
	{}

DArmPlatChunk::~DArmPlatChunk()
	{
	if (!iOwningProcess && iDomain>=0)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("DArmPlatChunk destruct, Domain %d freed",iDomain));
		ArmMmu::FreeDomain(iDomain);
		}
	DMemModelChunk::Destruct();
	}

TInt DArmPlatChunk::SetupPermissions()
	{
	Mmu& m = Mmu::Get();
	if(iChunkType==ESharedKernelSingle || iChunkType==ESharedKernelMultiple || iChunkType==ESharedIo)
		{
		// override map attributes for shared kernel chunks
		TUint ma = (iMapAttr &~ EMapAttrAccessMask) | EMapAttrSupRw;
		TPde pde;
		TInt r = m.PdePtePermissions(ma, pde, iPtePermissions);
		if (r != KErrNone)
			return r;
		iMapAttr = ma;
		}
	else
		iPtePermissions=m.PtePermissions(iChunkType);
	// PDE may need to set P bit for ECC in ARMv5 and later.
	if (iDomain<0)
		{
		iPdePermissions[0]=m.PdePermissions(iChunkType,ENotRunning);
		iPdePermissions[1]=m.PdePermissions(iChunkType,ERunningRO);
		iPdePermissions[2]=m.PdePermissions(iChunkType,ERunningRW);
		}
	else
		{
		TPde pdePermissions = PT_PDE(iDomain);
		iPdePermissions[0]=pdePermissions;
		iPdePermissions[1]=pdePermissions;
		iPdePermissions[2]=pdePermissions;
		}
	__KTRACE_OPT(KMMU,Kern::Printf("Chunk permissions PTE=%08x PDE0=%08x PDE1=%08x PDE2=%08x",
				iPtePermissions,iPdePermissions[0],iPdePermissions[1],iPdePermissions[2]));
	return KErrNone;
	}


// must hold process lock while iterating through thread list
void DArmPlatProcess::AdjustDomainAccess(TUint32 aClearMask, TUint32 aSetMask)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Process %O AdjustDomainAccess, clear %08x set %08x",
			this,aClearMask,aSetMask));
	iDacr=(iDacr & ~aClearMask)|aSetMask;
	__KTRACE_OPT(KMMU,Kern::Printf("DACR set to %08x",iDacr));
	SDblQueLink* pLink=iThreadQ.First();
	while (pLink!=&iThreadQ.iA)
		{
		DArmPlatThread* pT=_LOFF(pLink,DArmPlatThread,iProcessLink);
		pLink=pLink->iNext;
		pT->iNThread.ModifyDacr(aClearMask,aSetMask);
		}
	}

// must hold process lock while iterating through thread list
void DArmPlatProcess::AdjustThreadAttributes(TUint8 aClearMask, TUint8 aSetMask)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Process %O AdjustThreadAttributes, clear %02x set %02x",
			this,aClearMask,aSetMask));
	SDblQueLink* pLink=iThreadQ.First();
	NKern::LockSystem();
	while (pLink!=&iThreadQ.iA)
		{
		DArmPlatThread* pT=_LOFF(pLink,DArmPlatThread,iProcessLink);
		pLink=pLink->iNext;
		pT->iNThread.ModifyAttributes(aClearMask,aSetMask);
		NKern::FlashSystem();
		}
	NKern::UnlockSystem();
	}

TInt DArmPlatProcess::AddFixedAccessChunk(DMemModelChunk *aChunk)
	{
	DArmPlatChunk* pC=(DArmPlatChunk*)aChunk;
	if (pC->iChunkType==ESharedKernelSingle || pC->iChunkType==ESharedIo)
		{
		if (iDomain<0)
			return KErrGeneral;
		pC->iDomain = iDomain;
		TInt r = pC->SetupPermissions();
		__ASSERT_ALWAYS(r==KErrNone, MM::Panic(MM::EAddFixedBadPerm));
		}

	__KTRACE_OPT(KMMU,Kern::Printf("Add fixed access chunk, domain=%d",pC->iDomain));

	// if this is one of process's local chunks, nothing to do
	if (pC->iDomain!=iDomain && !(iAttributes&ESupervisor))
		{
		AdjustDomainAccess(0,2<<(pC->iDomain<<1));	// grant manager access to chunk's domain
		}
	return KErrNone;
	}

TInt DArmPlatProcess::RemoveFixedAccessChunk(DMemModelChunk *aChunk)
	{
	DArmPlatChunk* pC=(DArmPlatChunk*)aChunk;
	__KTRACE_OPT(KMMU,Kern::Printf("Remove fixed access chunk, domain=%d",pC->iDomain));
	if (pC->iChunkType==ESharedKernelSingle || pC->iChunkType==ESharedIo)
		{
		if (iDomain<0)
			return KErrGeneral;
		pC->iDomain = -1;
		TInt r = pC->SetupPermissions();
		__ASSERT_ALWAYS(r==KErrNone, MM::Panic(MM::ERemoveFixedBadPerm));
		return KErrNone;
		}

	// if this is one of process's local chunks, nothing to do
	if (pC->iDomain!=iDomain && !(iAttributes&ESupervisor))
		{
		AdjustDomainAccess(2<<(pC->iDomain<<1),0);	// remove manager access to chunk's domain
		}
	return KErrNone;
	}

void DArmPlatProcess::CheckForFixedAccess()
	{
	TInt domain=ArmMmu::AllocDomain();
	if (domain>=0)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("DArmPlatProcess create, Domain %d allocated",domain));
		iDomain=domain;
		iDacr |= (2<<(domain<<1));	// grant manager access to allocated domain
		iDacr &= ~2;				// demote domain 0 to client access
		}
	}

void DArmPlatProcess::DoAttributeChange()
	{
	// Called when a process changes from fixed to variable access or vice-versa.
	TBool variable=iAttributes & EVariableAccess;
	if (variable)
		{
		// process changing from fixed access to variable access
		__KTRACE_OPT(KMMU,Kern::Printf("Process %O becomes variable access",this));
		AdjustThreadAttributes(0,KThreadAttImplicitSystemLock|KThreadAttAddressSpace);
		AdjustDomainAccess(0,2);		// promote domain 0 to manager access
		}
	else
		{
		// process changing from variable access to fixed access
		__KTRACE_OPT(KMMU,Kern::Printf("Process %O becomes fixed access",this));
		AdjustDomainAccess(2,0);		// demote domain 0 to client access
		AdjustThreadAttributes(KThreadAttImplicitSystemLock|KThreadAttAddressSpace,0);
		}
	}

TIpcExcTrap::TExcLocation TIpcExcTrap::ExcLocation(DThread* /*aThread*/, TAny* aContext)
	{
	TArmExcInfo& info=*(TArmExcInfo*)aContext;

	if (info.iExcCode==EArmExceptionDataAbort)
		{
		TLinAddr va=(TLinAddr)info.iFaultAddress;
		if (va>=iRemoteBase && (va-iRemoteBase)<iSize)
			return EExcRemote;
		if (iLocalBase && va>=iLocalBase && (va-iLocalBase)<iSize)
			return EExcLocal;
		}
	return EExcUnknown;
	}
