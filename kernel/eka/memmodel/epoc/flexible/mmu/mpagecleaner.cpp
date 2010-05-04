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

#include <kernel.h>
#include "mpagecleaner.h"
#include "mm.h"
#include "mmu.h"
#include "mpager.h"

#ifdef __PAGING_PRE_CLEAN_DIRTY_PAGES

inline void IgnorePrintf(...) { }

#define PAGE_CLEANER_TRACE IgnorePrintf
//#define PAGE_CLEANER_TRACE Kern::Printf

_LIT(KThreadName, "PageCleaner");

const TInt KThreadPriority = 25;

/// The length of time the paging device is idle before we decide to use it for cleaning dirty
/// pages, in milliseconds.
const TInt KIdleDelayInMillis = 2;

class DPageCleaner
	{
public:
	DPageCleaner();
	void Start();
	void NotifyPagingDeviceIdle();
	void NotifyPagingDeviceBusy();
	void NotifyPagesToClean();

private:
	inline TBool IsRunning();
	void UpdateBusyCount(TInt aChange);
	void IdleTimerExpired(TUint aInitialNotificationCount);
	void TryToClean();
	
private:
	static void TimerDfcFn(TAny*);
	static void CleanerDfcFn(TAny*);
	
private:
	TInt iIdleDelayInTicks;
	NTimer iDelayTimer;
	TDfcQue iDfcQue;
	TDfc iTimerDfc;
	TDfc iCleanerDfc;
	TBool iRunning;

	// All state below is accessed with the MmuLock held.

	/// Whether the paging device is currently idle.
	TBool iPagingDeviceIdle;

	/// Whether the paging device has been idle for longer than the wait period.
	TBool iIdleForAWhile;
	
	/// Whether the page cleaner is currently running.
	TBool iCleaningInProgress;	
	};

DPageCleaner ThePageCleaner;

DPageCleaner::DPageCleaner() :
	iTimerDfc(TimerDfcFn, NULL, 1),
	iCleanerDfc(CleanerDfcFn, NULL, 1),
	iRunning(EFalse),
	iPagingDeviceIdle(ETrue),
	iIdleForAWhile(ETrue),
	iCleaningInProgress(EFalse)
	{
	}

void DPageCleaner::Start()
	{
	TBool alreadyRunning = __e32_atomic_swp_ord32(&iRunning, ETrue);
	if (alreadyRunning)
		return;

	iIdleDelayInTicks = NKern::TimerTicks(KIdleDelayInMillis);
	
	TInt r = Kern::DfcQInit(&iDfcQue, KThreadPriority, &KThreadName);
	__NK_ASSERT_ALWAYS(r == KErrNone);
	iTimerDfc.SetDfcQ(&iDfcQue);
	iCleanerDfc.SetDfcQ(&iDfcQue);

	PAGE_CLEANER_TRACE("PageCleaner started");
	}

FORCE_INLINE TBool DPageCleaner::IsRunning()
	{
	return __e32_atomic_load_acq32(&iRunning);
	}

void DPageCleaner::NotifyPagingDeviceIdle()
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	if (IsRunning())
		{
		iPagingDeviceIdle = ETrue;
		if (!iDelayTimer.IsPending())
			iDelayTimer.OneShot(iIdleDelayInTicks, iTimerDfc);
		}
	}

void DPageCleaner::NotifyPagingDeviceBusy()
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	if (IsRunning())
		{
		iDelayTimer.Cancel();
		iPagingDeviceIdle = EFalse;
		iIdleForAWhile = EFalse;
		}		
	}

void DPageCleaner::NotifyPagesToClean()
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	if (IsRunning())
		{
		if (!iCleaningInProgress && iIdleForAWhile)
			iCleanerDfc.Enque();
		}
	}

void DPageCleaner::TimerDfcFn(TAny* aPtr)
	{
	ThePageCleaner.IdleTimerExpired((TUint)aPtr);	
	}

void DPageCleaner::IdleTimerExpired(TUint aInitialNotificationCount)
	{
	MmuLock::Lock();	
	if (iPagingDeviceIdle)
		{
		iIdleForAWhile = ETrue;
		if (!iCleaningInProgress && ThePager.HasPagesToClean())
			iCleanerDfc.Enque();
		}
	MmuLock::Unlock();
	}

void DPageCleaner::CleanerDfcFn(TAny*)
	{
	ThePageCleaner.TryToClean();
	}

void DPageCleaner::TryToClean()
	{
	MmuLock::Lock();
	TBool workToDo = iIdleForAWhile && ThePager.HasPagesToClean();
	iCleaningInProgress = workToDo;
	MmuLock::Unlock();
	
	if (!workToDo)
		{
		PAGE_CLEANER_TRACE("PageCleaner - started but no work to do");
		return;
		}

	for (;;)
		{
		PageCleaningLock::Lock();
		MmuLock::Lock();
		if (!iIdleForAWhile)
			break;
		TInt attempted = ThePager.CleanSomePages(ETrue);
		if (attempted == 0)
			break;
		PAGE_CLEANER_TRACE("PageCleaner - attempted to clean %d pages", attempted);
		MmuLock::Unlock();
		PageCleaningLock::Unlock();
		}
	
	if (iIdleForAWhile)
		PAGE_CLEANER_TRACE("PageCleaner - no more pages to clean");
	else
		PAGE_CLEANER_TRACE("PageCleaner - device now busy");
	
	iCleaningInProgress = EFalse;
	MmuLock::Unlock();
	PageCleaningLock::Unlock();
	}

void PageCleaner::Start()
	{
	ThePageCleaner.Start();
	}

void PageCleaner::NotifyPagesToClean()
	{
	ThePageCleaner.NotifyPagesToClean();
	}

EXPORT_C void DPagingDevice::NotifyIdle()
	{
	ThePageCleaner.NotifyPagingDeviceIdle();
	}

EXPORT_C void DPagingDevice::NotifyBusy()
	{
	ThePageCleaner.NotifyPagingDeviceBusy();
	}

#else  // __PAGING_PRE_CLEAN_DIRTY_PAGES not defined

void PageCleaner::Start()
	{
	}

void PageCleaner::NotifyPagesToClean()
	{
	}

EXPORT_C void DPagingDevice::NotifyIdle()
	{
	}

EXPORT_C void DPagingDevice::NotifyBusy()
	{
	}

#endif

EXPORT_C NFastMutex* DPagingDevice::NotificationLock()
	{
	// use the MmuLock
	return &MmuLock::iLock;
	}
