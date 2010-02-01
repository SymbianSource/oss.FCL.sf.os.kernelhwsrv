// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Other possible inputs to thrashing detection:
// - cache size and free ram
// - pin failures
//



#include <kernel/kern_priv.h>
#include "mthrash.h"

const TInt KUpdatePeriod = 1000;   // Update every second

DThrashMonitor TheThrashMonitor;

DThrashMonitor::DThrashMonitor() :
	iRunning(EFalse),
	iUpdateTimer(NULL, this),
	iUpdateDfc(UpdateDfcFunc, this, 0),
	iThrashLevel(0),
	iThresholdThrashing(200),
	iThresholdGood(150)
	{
	}

void DThrashMonitor::Start()
	{
	TBool alreadyRunning = __e32_atomic_swp_ord32(&iRunning, ETrue);
	if (alreadyRunning)
		return;
	
	// reset
	memclr(&iCount[0], sizeof(iCount));
	iLastUpdateTime = NKern::TickCount();
	
	iUpdateDfc.SetDfcQ(Kern::DfcQue0());
	TInt r = iUpdateTimer.OneShot(KUpdatePeriod, iUpdateDfc);
	__NK_ASSERT_ALWAYS(r == KErrNone);
	}

TInt DThrashMonitor::ThrashLevel()
	{
	return iThrashLevel;
	}

TInt DThrashMonitor::SetThresholds(TUint aThrashing, TUint aGood)
	{
	if (aThrashing < aGood || aThrashing > 255)
		return KErrArgument;
	iThresholdThrashing = aThrashing;
	iThresholdGood = aGood;
	return KErrNone;
	}

void DThrashMonitor::UpdateCount(TCount aCount, TInt aDelta)
	{
	TCountData& c = iCount[aCount];

	NKern::FMWait(&iMutex);
	
	TUint32 currentTime = NKern::TickCount();
	c.iTotal += (TInt64)c.iCount * (currentTime - c.iLastUpdateTime);
	c.iCount += aDelta;
	c.iLastUpdateTime = currentTime;
	
	if(!iUpdateTimer.IsPending())	
		{
		TInt r = iUpdateTimer.OneShot(KUpdatePeriod, iUpdateDfc);
		__NK_ASSERT_ALWAYS(r == KErrNone);
		}
	
	NKern::FMSignal(&iMutex);
	
	__NK_ASSERT_DEBUG(c.iCount >= 0);
	}

void DThrashMonitor::NotifyStartPaging()
	{
	UpdateCount(ECountThreadsPaging, 1);
	}

void DThrashMonitor::NotifyEndPaging()
	{
	UpdateCount(ECountThreadsPaging, -1);
	}

void DThrashMonitor::UpdateDfcFunc(TAny* aPtr)
	{
	DThrashMonitor* self = (DThrashMonitor*)aPtr;
	self->RecalculateThrashLevel();
	}

void DThrashMonitor::RecalculateThrashLevel()
	{
	TInt currentTime = NKern::TickCount();
	TInt elapsedTicks = currentTime - iLastUpdateTime;

	NKern::FMWait(&iMutex);
	for (TInt i = 0 ; i < EMaxCount ; ++i)
		{
		TCountData& c = iCount[i];
		c.iTotal += (TInt64)c.iCount * (currentTime - c.iLastUpdateTime);
		c.iAverage = (TInt)((256 * c.iTotal) / elapsedTicks);
		c.iTotal = 0;
		c.iLastUpdateTime = currentTime;
		}
	NKern::FMSignal(&iMutex);

	TInt pagingActivity = Min(iCount[ECountThreadsPaging].iAverage, 255);

	// Base thrash level entirely on the average number of threads paging
	TInt newThrashLevel = pagingActivity;
	TInt oldThrashLevel = iThrashLevel;

	// Make thrash level increase slowly over time, but decrease quickly
	if (newThrashLevel > oldThrashLevel)
		newThrashLevel = (3 * oldThrashLevel + pagingActivity) >> 2;

	iThrashLevel = newThrashLevel;
	
	// Notify user-side if thrashing thresholds passed
	TBool notifyChange = EFalse;
	if (oldThrashLevel < iThresholdThrashing && newThrashLevel >= iThresholdThrashing)
		{
		iIsThrashing = ETrue;
		notifyChange = ETrue;
		}
	else if (iIsThrashing && oldThrashLevel >= iThresholdGood && newThrashLevel < iThresholdGood)
		{
		iIsThrashing = EFalse;
		notifyChange = ETrue;
		}
	
	if (notifyChange)
		{
		NKern::ThreadEnterCS();
		Kern::AsyncNotifyChanges(EChangesThrashLevel);
		NKern::ThreadLeaveCS();
		}
	
	iLastUpdateTime = currentTime;
	
	if(iThrashLevel != 0)
	    {
	    NKern::FMWait(&iMutex);
	    if(!iUpdateTimer.IsPending())
	        {
            TInt r = iUpdateTimer.Again(KUpdatePeriod);
            if (r == KErrArgument)
                {
                r = iUpdateTimer.OneShot(KUpdatePeriod, iUpdateDfc);  
                }
            __NK_ASSERT_ALWAYS(r == KErrNone);
	        }
        NKern::FMSignal(&iMutex);    
        
	    }
	
	}
