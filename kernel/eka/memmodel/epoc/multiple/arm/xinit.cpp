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
// e32\memmodel\epoc\multiple\arm\xinit.cpp
// 
//

#include "arm_mem.h"
#include <e32uid.h>

GLREF_C void DoProcessSwitch();

void MM::Init1()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("MM::Init1()"));

	K::MaxMemCopyInOneGo=KDefaultMaxMemCopyInOneGo;
	MM::MaxPagesInOneGo=KMaxPages;
	TheScheduler.SetProcessHandler((TLinAddr)DoProcessSwitch);

	__KTRACE_OPT(KBOOT,Kern::Printf("K::MaxMemCopyInOneGo=0x%x",K::MaxMemCopyInOneGo));
	__KTRACE_OPT(KBOOT,Kern::Printf("MM::MaxPagesInOneGo=%d",MM::MaxPagesInOneGo));
	}

// Set up virtual addresses used for cache flushing if this is
// done by data read or line allocate
void M::SetupCacheFlushPtr(TInt aCache, SCacheInfo& aInfo)
	{
	}
