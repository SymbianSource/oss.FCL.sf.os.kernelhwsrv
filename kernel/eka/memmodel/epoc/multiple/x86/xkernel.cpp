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
// e32\memmodel\epoc\multiple\x86\xkernel.cpp
// 
//

#include <x86_mem.h>


/********************************************
 * Thread
 ********************************************/

TInt DX86PlatThread::SetupContext(SThreadCreateInfo& aInfo)
	{
	switch(iThreadType)
		{
		case EThreadSupervisor:
		case EThreadMinimalSupervisor:
		case EThreadInitial:
		case EThreadAPInitial:
			break;
		case EThreadUser:
			break;
		}
	iNThread.SetAddressSpace(iOwningProcess);
	iNThread.SetAttributes(KThreadAttAddressSpace);
#ifdef __SMP__
	iCpuRestoreCookie = -1;
#else
	iAliasPdePtr = &PageDirectory(((DMemModelProcess*)iOwningProcess)->iOsAsid)[KIPCAlias>>KChunkShift];
#endif
	return KErrNone;
	}

/********************************************
 * Process
 ********************************************/

DX86PlatProcess::DX86PlatProcess()
	{
	iAddressCheckMaskR=0x000fffffu;	// addresses<0xA0000000 OK
	iAddressCheckMaskW=0x0000ffffu;	// addresses<0x80000000 OK
	}

DX86PlatProcess::~DX86PlatProcess()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DX86PlatProcess destruct"));
	DMemModelProcess::Destruct();
	}

TInt DX86PlatProcess::GetNewChunk(DMemModelChunk*& aChunk, SChunkCreateInfo& aInfo)
	{
	aChunk=NULL;
	DX86PlatChunk* pC=new DX86PlatChunk;
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
			DX86PlatChunk* pM=new DX86PlatChunk;
			if (!pM)
				return KErrNoMemory;
			pC->iKernelMirror = pM;
			pM->iChunkType = ESharedKernelMirror;
			r=pM->SetAttributes(aInfo);
			}
		}
	return r;
	}

TInt DX86PlatChunk::SetAttributes(SChunkCreateInfo& aInfo)
	{
	switch(iChunkType)
		{
		case EKernelData:
			iAttributes = EAddressFixed|EMapTypeGlobal|EPrivate;
			break;
		case ERamDrive:
			iAttributes = EAddressFixed|EMapTypeShared|EPrivate;
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

DX86PlatChunk::DX86PlatChunk()
	{}

DX86PlatChunk::~DX86PlatChunk()
	{
	DMemModelChunk::Destruct();
	}

TInt DX86PlatChunk::SetupPermissions()
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
	TX86ExcInfo& info=*(TX86ExcInfo*)aContext;
	if (info.iExcId==EX86VectorPageFault)
		{
		TLinAddr va=(TLinAddr)info.iFaultAddress;

		TLinAddr aliasAddr = ((DMemModelThread*)aThread)->iAliasLinAddr;
		TBool remoteError;
		if(aliasAddr)
			remoteError = TUint(va^aliasAddr)<TUint(KPageSize);
		else
			remoteError = va>=iRemoteBase && (va-iRemoteBase)<iSize;
		if (remoteError)
			return EExcRemote;

		if (iLocalBase && va>=iLocalBase && (va-iLocalBase)<iSize)
			return EExcLocal;
		}
	else if (info.iExcId==EX86VectorGPF)
		{
		TUint16 ds=(TUint16)info.iDs;
		TUint16 es=(TUint16)info.iEs;
		TUint16 seg=iDir?ds:es;	// write -> local read -> DS restricted, else ES restricted
		if (seg==KRing3DS || seg==KRing3CS)
			return EExcLocal;
		}
	return EExcUnknown;
	}
