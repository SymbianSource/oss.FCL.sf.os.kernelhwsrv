// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <plat_priv.h>
#include "mm.h"
#include "mmu.h"

#include "mcleanup.h"


//
// TMemoryCleanup
//

static TDfc CleanupDfc(TMemoryCleanup::Cleanup, NULL, 2);

static TMemoryCleanup* CleanupList = 0;

#ifdef __SMP__
static TSpinLock CleanupLock(TSpinLock::EOrderEventHandlerList);
#endif

void TMemoryCleanup::Init2()
	{
	CleanupDfc.SetDfcQ(K::SvMsgQ);
	}


void TMemoryCleanup::Add(TMemoryCleanupCallback aCallback, TAny* aArg)
	{
	__SPIN_LOCK_IRQ(CleanupLock);
	if(iCallback)
		{
		// already queued...
		__NK_ASSERT_DEBUG(iCallback==aCallback);
		__SPIN_UNLOCK_IRQ(CleanupLock);
		}
	else
		{
		iCallback = aCallback;
		iArg = aArg;
		iNext = CleanupList;
		CleanupList = this;
		__SPIN_UNLOCK_IRQ(CleanupLock);
		CleanupDfc.Enque();
		}
	}


void TMemoryCleanup::Cleanup(TAny* /*aDummy*/)
	{
	for(;;)
		{
		__SPIN_LOCK_IRQ(CleanupLock);
		TMemoryCleanup* c = CleanupList;
		if(!c)
			{
			__SPIN_UNLOCK_IRQ(CleanupLock);
			return;
			}

		CleanupList = c->iNext;
		TMemoryCleanupCallback fn = c->iCallback;
		TAny* arg = c->iArg;
		c->iCallback = 0;
		__SPIN_UNLOCK_IRQ(CleanupLock);
		(*fn)(arg);
		}
	}
