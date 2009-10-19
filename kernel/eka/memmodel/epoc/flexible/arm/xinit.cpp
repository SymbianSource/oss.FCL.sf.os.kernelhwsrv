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

#include "arm_mem.h"
#include "../mmu/mm.h"

// Set up virtual addresses used for cache flushing if this is
// done by data read or line allocate
void M::SetupCacheFlushPtr(TInt aCache, SCacheInfo& aInfo)
	{
	}

#ifdef __SMP__
extern DMemoryObject* ExceptionStacks;

void M::GetAPBootInfo(TInt aCpu, volatile SAPBootInfo* aInfo)
	{
	SArmAPBootInfo* b = (SArmAPBootInfo*)aInfo;
	TInt i;
	for (i=1; i<=7; i+=2)
		{
		TUint ix = (TUint)(aCpu*8+i);
		TInt r = MM::MemoryAlloc(ExceptionStacks, ix, 1);
		__NK_ASSERT_ALWAYS(r==KErrNone);
		TLinAddr addr = KExcptStacksLinearBase + (ix<<KPageShift);
		TLinAddr top = addr + KPageSize;
		TUint8 fill = 0xdd;
		switch (i)
			{
			case 1:	b->iInitR13Irq = top; fill=0xaa; break;
			case 3:	b->iInitR13Fiq = top; fill=0xbb; break;
			case 5:	b->iInitR13Und = top; break;
			case 7:	top-=sizeof(SFullArmRegSet); b->iInitR13Abt = top; memclr((TAny*)top,sizeof(SFullArmRegSet)); break;
			default: __NK_ASSERT_ALWAYS(0); break;
			}
		memset((TAny*)addr, fill, top-addr);
		}
	}

void M::Init2AP()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("M::Init2AP()"));
	DThread& t = Kern::CurrentThread();
	SubScheduler().iAddressSpace = t.iOwningProcess;
	t.iNThread.SetAddressSpace(t.iOwningProcess);
	t.iNThread.SetAttributes(KThreadAttAddressSpace);
	}
#endif
