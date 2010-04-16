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
// e32\memmodel\epoc\multiple\minit.cpp
// 
//

#include "memmodel.h"
#include "cache_maintenance.h"

_LIT(KLitSvStack,"SvStack");

const TInt KMaxThreads=1024;

void M::Init1()
	{
	// Memory model dependent CPU stuff
	MM::Init1();

	// Set up cache info
	CacheMaintenance::Init1();

	// First phase MMU initialisation
	Mmu& m=Mmu::Get();
	m.Init1();
	}

void M::Init2()
	{
	// Second phase MMU initialisation
	Mmu& m=Mmu::Get();
	m.Init2();
	}

void M::Init3()
	{
	// Third phase MMU initialisation
	Mmu& m=Mmu::Get();
	m.Init3();
	}

void M::Init4()
    {
    // Fourth phase MMU initialisation - Not required on this memory model.
    }

TInt M::InitSvHeapChunk(DChunk* aChunk, TInt aSize)
	{
	TInt r;
	TLinAddr base = TheRomHeader().iKernDataAddress;
	DMemModelChunk* pC = (DMemModelChunk*)aChunk;
	K::HeapInfo.iChunk = aChunk;
	K::HeapInfo.iBase = (TUint8*)base;
	K::HeapInfo.iMaxSize = pC->MaxSize();
	pC->SetFixedAddress(base, aSize);
	__KTRACE_OPT(KBOOT,Kern::Printf("Created SvHeap chunk, addr %08X, init size %08X max size %08X",pC->Base(),aSize,pC->MaxSize()));
	TLinAddr dataSectionBase=0;
	r=((DMemModelProcess*)K::TheKernelProcess)->AddChunk(pC,dataSectionBase,EFalse);
 	__KTRACE_OPT(KBOOT,Kern::Printf("Added kernel heap chunk to current process, %d",r));
	return r;
	}

TInt M::InitSvStackChunk()
	{
	// create a chunk to hold supervisor-mode stacks
	Mmu& m=Mmu::Get();
	TInt alias_space=m.iAliasSize+m.iPageSize;
	TInt total=alias_space+K::SupervisorThreadStackSize+PP::SupervisorThreadStackGuard;
	total=(total+m.iAliasMask)&~m.iAliasMask;
	DMemModelChunk* pC=NULL;
	DMemModelProcess* pP=(DMemModelProcess*)K::TheKernelProcess;
	TInt maxsize=total*KMaxThreads;
	TLinAddr dataSectionBase=0;
	SChunkCreateInfo cinfo;
	cinfo.iGlobal=EFalse;
	cinfo.iAtt=TChunkCreate::EDisconnected;
	cinfo.iForceFixed=EFalse;
	cinfo.iOperations=0;
	cinfo.iType=EKernelStack;
	cinfo.iMaxSize=maxsize;
	cinfo.iPreallocated=0;
	cinfo.iName.Set(KLitSvStack);
	cinfo.iOwner=K::TheKernelProcess;
	TInt r=pP->NewChunk((DChunk*&)pC,cinfo,dataSectionBase);
	__KTRACE_OPT(KBOOT,Kern::Printf("Created SvStack, %d",r));
	if (r!=KErrNone)
		return r;
	r=pC->Reserve(0);
	if (r!=KErrNone)
		return r;
	__KTRACE_OPT(KBOOT,Kern::Printf("Created SvStack chunk, addr %08X, max size %08X",pC->Base(),maxsize));
	MM::SvStackChunk=pC;
	r=pP->AddChunk(pC,dataSectionBase,EFalse);
 	__KTRACE_OPT(KBOOT,Kern::Printf("Added SvStack chunk to current process, %d",r));
	return r;
	}

