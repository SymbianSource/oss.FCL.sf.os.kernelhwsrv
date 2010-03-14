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
//

#include <memmodel.h>
#include "mmu/mm.h"
#include "mmboot.h"

#include "cache_maintenance.h"
#include "mmu/mmu.h"
#include "mmu/mpager.h"


extern void DoProcessSwitch();

void M::Init1()
	{
	// Memory model dependent CPU stuff
	__KTRACE_OPT(KBOOT,Kern::Printf("MM::Init1()"));

	K::MaxMemCopyInOneGo=KDefaultMaxMemCopyInOneGo;
	TheScheduler.SetProcessHandler((TLinAddr)DoProcessSwitch);

	__KTRACE_OPT(KBOOT,Kern::Printf("K::MaxMemCopyInOneGo=0x%x",K::MaxMemCopyInOneGo));

	// Set up cache info
	CacheMaintenance::Init1();

	// First phase MMU initialisation
	MM::Init1();
	}


void M::Init2()
	{
	// Second phase MMU initialisation
	MM::Init2();
	}


void M::Init3()
	{
	// Third phase MMU initialisation
	MM::Init3();
	}

void M::Init4()
    {
    // Fourth phase MMU initialisation
    ThePager.InitCache();
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
	__KTRACE_OPT(KBOOT,Kern::Printf("Created SvHeap chunk, addr %08X, init size %08X max size %08X",base,aSize,pC->MaxSize()));
	r=((DMemModelProcess*)K::TheKernelProcess)->DoAddChunk(pC,EFalse);
 	__KTRACE_OPT(KBOOT,Kern::Printf("Added kernel heap chunk to current process, %d",r));
	__NK_ASSERT_DEBUG(pC->Base(K::TheKernelProcess)==(TUint8*)base);
	return r;
	}


TInt M::InitSvStackChunk()
	{
	// nothing to do...
	return KErrNone;
	}


void RHeapK::Mutate(TInt aOffset, TInt aMaxLength)
//
// Used by the kernel to mutate a fixed heap into a chunk heap.
//
	{
	iMinLength += aOffset;
	iMaxLength = aMaxLength + aOffset;
	iOffset = aOffset;
	iChunkHandle = (TInt)K::HeapInfo.iChunk;
	iPageSize = KPageSize;
	iGrowBy = iPageSize;
	iFlags = 0;
	MM::MemorySetLock(((DMemModelChunk*)K::HeapInfo.iChunk)->iMemoryObject,*(DMutex**)&iLock);
	}


