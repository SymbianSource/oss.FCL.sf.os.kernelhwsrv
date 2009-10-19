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

#include <x86_mem.h>
#include <e32uid.h>

// Set up virtual addresses used for cache flushing if this is
// done by data read or line allocate
void M::SetupCacheFlushPtr(TInt aCache, SCacheInfo& aInfo)
	{
	}


#ifdef __SMP__
void M::GetAPBootInfo(TInt /*aCpu*/, volatile SAPBootInfo* /*aInfo*/)
	{
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
