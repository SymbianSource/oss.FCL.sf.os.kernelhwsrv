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
// e32\memmodel\epoc\multiple\arm\xkernel.cpp
// 
//

#include "arm_mem.h"


/********************************************
 * Thread
 ********************************************/

TInt DArmPlatThread::SetupContext(SThreadCreateInfo& aInfo)
	{
	switch(iThreadType)
		{
		case EThreadSupervisor:
		case EThreadMinimalSupervisor:
		case EThreadInitial:
		case EThreadAPInitial:
			break;
		case EThreadUser:
#ifndef __SMP__
			iNThread.iSpare3 /*iUserContextType*/ = NThread::EContextUndefined;
#endif
			break;
		}
	iNThread.SetAddressSpace(iOwningProcess);
	iNThread.SetAttributes(KThreadAttAddressSpace);
	iAliasPdePtr = &PageDirectory(((DMemModelProcess*)iOwningProcess)->iOsAsid)[KIPCAlias>>KChunkShift];
	return KErrNone;
	}

/********************************************
 * Process
 ********************************************/

DArmPlatProcess::DArmPlatProcess()
	{
	iAddressCheckMaskR=0x000fffffu;	// addresses<0xA0000000 OK
	iAddressCheckMaskW=0x0000ffffu;	// addresses<0x80000000 OK
	}

void FlushTLBs();
DArmPlatProcess::~DArmPlatProcess()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DArmPlatProcess destruct"));
	FlushTLBs();
	DMemModelProcess::Destruct();
	}

TInt DArmPlatProcess::GetNewChunk(DMemModelChunk*& aChunk, SChunkCreateInfo& aInfo)
	{
	aChunk=NULL;
	DArmPlatChunk* pC=new DArmPlatChunk;
	if (!pC)
		return KErrNoMemory;
	aChunk=pC;
	TChunkType type = aInfo.iType;
	pC->iChunkType=type;
	TInt r=pC->SetAttributes(aInfo);
	if(r==KErrNone)
		{
		if(type==ESharedKernelSingle || type==ESharedKernelMultiple || type==ESharedIo)
			{
			DArmPlatChunk* pM=new DArmPlatChunk;
			if (!pM)
				return KErrNoMemory;
			pC->iKernelMirror = pM;
			pM->iChunkType = ESharedKernelMirror;
			r=pM->SetAttributes(aInfo);
			}
		}
	return r;
	}

TInt DArmPlatChunk::SetAttributes(SChunkCreateInfo& aInfo)
	{
	switch(iChunkType)
		{
		case EKernelData:
			iAttributes = EAddressFixed|EMapTypeGlobal|EPrivate;
			break;
		case ERamDrive:
			iAttributes = EAddressFixed|EMapTypeGlobal|EPrivate;
			break;
		case EKernelStack:
			iAttributes = EAddressKernel|EMapTypeGlobal|EPrivate;
			break;
		case EKernelCode:
			iAttributes = EAddressKernel|EMapTypeGlobal|EPrivate|ECode;
			break;
		case EDll:
		case EUserCode:
			if (aInfo.iGlobal)
				iAttributes = EAddressUserGlobal|EMapTypeGlobal|ECode;
			else
				iAttributes = EAddressFixed|EMapTypeLocal|EPrivate|ECode;
			break;
		case EUserData:
			if (aInfo.iGlobal)
				iAttributes = EAddressShared|EMapTypeShared;
			else
				iAttributes = EAddressLocal|EMapTypeLocal|EPrivate;
			break;
		case EDllData:
			iAttributes = EAddressFixed|EMapTypeLocal|EPrivate;
			break;
		case EUserSelfModCode:
			if (aInfo.iGlobal)
				iAttributes = EAddressShared|EMapTypeShared|ECode;
			else
				iAttributes = EAddressLocal|EMapTypeLocal|EPrivate|ECode;
			break;
		case ESharedKernelSingle:
		case ESharedKernelMultiple:
		case ESharedIo:
			iAttributes = EAddressShared|EMapTypeShared;
			break;
		case ESharedKernelMirror:
			iAttributes = EAddressKernel|EMapTypeGlobal|EPrivate;
			break;
		case EKernelMessage:
			iAttributes = EAddressKernel|EMapTypeGlobal|EPrivate;
			break;
		default:
			FAULT();
		}
	return KErrNone;
	}

/********************************************
 * Chunk
 ********************************************/

DArmPlatChunk::DArmPlatChunk()
	{}

DArmPlatChunk::~DArmPlatChunk()
	{
	DMemModelChunk::Destruct();
	}

TInt DArmPlatChunk::SetupPermissions()
	{
	Mmu& m=Mmu::Get();
	if(iChunkType==ESharedKernelSingle || iChunkType==ESharedKernelMultiple || iChunkType==ESharedIo || iChunkType==ESharedKernelMirror)
		{
		// override map attributes for shared kernel chunks
		TUint ma = (iMapAttr &~ EMapAttrAccessMask) | (iChunkType==ESharedKernelMirror?EMapAttrSupRw:EMapAttrUserRw);
		TInt r = m.PdePtePermissions(ma, iPdePermissions, iPtePermissions);
		if (r != KErrNone)
			return r;
		iMapAttr = ma;
		}
	else
		{
		iPtePermissions=m.PtePermissions(iChunkType);
		iPdePermissions=m.PdePermissions(iChunkType,EFalse);
		}

	__KTRACE_OPT(KMMU,Kern::Printf("Chunk permissions PTE=%08x PDE=%08x",iPtePermissions,iPdePermissions));
	return KErrNone;
	}

TIpcExcTrap::TExcLocation TIpcExcTrap::ExcLocation(DThread* aThread, TAny* aContext)
	{
	TArmExcInfo& info=*(TArmExcInfo*)aContext;
	if (info.iExcCode==EArmExceptionDataAbort)
		{
		TLinAddr va=(TLinAddr)info.iFaultAddress;

		TLinAddr aliasAddr = ((DMemModelThread*)aThread)->iAliasLinAddr;
		TBool remoteError;
		if(aliasAddr)
			remoteError = TUint(va^aliasAddr)<TUint(KPageSize);
		else
			// The second clause in the statement below was "va < iRemoteBase + iSize".
			// iRemoteBase + iSize might conceivably wrap round.
			// The usual fix for this is to change
			//			va >= base && va < base + size
			// to		va >= base && (va - base) < size
			// but this requires the first clause (va >= base) so that va-base doesn't wrap negative.
			// Since the first clause in this expression is va >= (iRemoteBase & ~3)
			// we have to re-write the expression as follows:
			// Let base' = iRemoteBase & ~3
			// so  base  = base' + (base & 3)
			// then we have va >= base' && va < base' + (base & 3) + iSize
			// (effectively the & ~3 on the first clause extends the range downwards by base & 3)
			remoteError = va>=(iRemoteBase&~3) &&
							(va - (iRemoteBase & ~3)) < iSize + (iRemoteBase & 3);
		if (remoteError)
			return EExcRemote;

		// Third clause was va < iLocalBase + iSize, fixed as in the "remoteError =" line above
		if (iLocalBase && va>=(iLocalBase&~3) &&
			(va - (iLocalBase & ~3)) < iSize + (iLocalBase & 3))
			return EExcLocal;
		}
	return EExcUnknown;
	}
