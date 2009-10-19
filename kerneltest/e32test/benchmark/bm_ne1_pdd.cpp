// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <kernel/kernel.h>
#include <upd35001_timer.h>

#include "k32bm.h"

class DBMNE1Device : public DPhysicalDevice
	{
public:
	DBMNE1Device();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Validate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	};

class DBMNE1Channel : public DBMPChannel
	{
public:
	DBMNE1Channel();
	~DBMNE1Channel();
	virtual TBMTicks TimerPeriod();
	virtual TBMTicks TimerStamp();
	virtual TBMNs TimerTicksToNs(TBMTicks);
	virtual TBMTicks TimerNsToTicks(TBMNs);
	virtual TInt BindInterrupt(MBMIsr*);
	virtual TInt BindInterrupt(MBMInterruptLatencyIsr*);
	virtual void RequestInterrupt();
	virtual void CancelInterrupt();

private:
	static const TBMTicks	KBMNE1Period = (((TBMTicks) 1) << 32);
	
	static void Isr(TAny*);
	
	MBMIsr*						iIsr;
	MBMInterruptLatencyIsr*		iInterruptLatencyIsr;
	TUint						iPrescale;
	TUint						iNsPerTick;
	NTimer						iTimer;
	volatile TUint				iStartCount;
	volatile TUint				iRunCount;
	volatile TUint				iCancelCount;
	};


DECLARE_STANDARD_PDD()
//
// Create a new device
//
	{
	__ASSERT_CRITICAL;
	return new DBMNE1Device;
	}

DBMNE1Device::DBMNE1Device()
//
// Constructor
//
	{
	//iUnitsMask=0;
	iVersion = TVersion(1,0,1);
	}

TInt DBMNE1Device::Install()
//
// Install the device driver.
//
	{
	TInt r = SetName(&KBMPdName);
	return r;
	}

void DBMNE1Device::GetCaps(TDes8& aDes) const
//
// Return the Comm capabilities.
//
	{
	}

TInt DBMNE1Device::Create(DBase*& aChannel, TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
//
// Create a channel on the device.
//
	{
	__ASSERT_CRITICAL;
	aChannel = new DBMNE1Channel;
	return aChannel?KErrNone:KErrNoMemory;
	}

TInt DBMNE1Device::Validate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
	{
	if (!Kern::QueryVersionSupported(iVersion,aVer))
		{
		return KErrNotSupported;
		}
	return KErrNone;
	}

DBMNE1Channel::DBMNE1Channel()
	:	iTimer(&Isr, this)
	{
	//	iIsr = NULL;
	//	iInterruptLatencyIsr = NULL;
	NETimer& T2 = NETimer::Timer(2);
	iPrescale = __e32_find_ms1_32(T2.iPrescaler & 0x3f);
	iNsPerTick = 15u << iPrescale;
	}

DBMNE1Channel::~DBMNE1Channel()
	{
	CancelInterrupt();
	}

TBMTicks DBMNE1Channel::TimerPeriod()
	{
	return KBMNE1Period;
	}

TBMTicks DBMNE1Channel::TimerStamp()
	{
	return NETimer::Timer(2).iTimerCount;
	}

TBMNs DBMNE1Channel::TimerTicksToNs(TBMTicks ticks)
	{
	return ticks * (TBMTicks)iNsPerTick;
	}

TBMTicks DBMNE1Channel::TimerNsToTicks(TBMNs ns)
	{
	return ns / (TBMTicks)iNsPerTick;
	}

void DBMNE1Channel::Isr(TAny* ptr)
	{
	DBMNE1Channel* mCh = (DBMNE1Channel*) ptr;
	BM_ASSERT(mCh->iIsr || mCh->iInterruptLatencyIsr);
	if (mCh->iIsr)
		{
		mCh->iIsr->Isr(NETimer::Timer(2).iTimerCount);
		}
	else
		{
		TUint x = NETimer::Timer(0).iTimerCount;
		x = (x + (1u<<mCh->iPrescale) - 1) >> mCh->iPrescale;
		mCh->iInterruptLatencyIsr->InterruptLatencyIsr(x);
		}
	__e32_atomic_add_ord32(&mCh->iRunCount, 1);
	}

TInt DBMNE1Channel::BindInterrupt(MBMIsr* aIsr)
	{
	BM_ASSERT(!iIsr);
	BM_ASSERT(!iInterruptLatencyIsr);
	iIsr = aIsr;
	return KErrNone;
	}

TInt DBMNE1Channel::BindInterrupt(MBMInterruptLatencyIsr* aIsr)
	{
	BM_ASSERT(!iIsr);
	BM_ASSERT(!iInterruptLatencyIsr);
	iInterruptLatencyIsr = aIsr;
	return KErrNone;
	}


void DBMNE1Channel::RequestInterrupt()
	{
	BM_ASSERT(iIsr || iInterruptLatencyIsr);
	if (iTimer.OneShot(1)==KErrNone)
		__e32_atomic_add_ord32(&iStartCount, 1);
	}

void DBMNE1Channel::CancelInterrupt()
	{
	if (iTimer.Cancel())
		__e32_atomic_add_ord32(&iCancelCount, 1);
	while (iStartCount != iCancelCount + iRunCount)
		{}
	}
	
