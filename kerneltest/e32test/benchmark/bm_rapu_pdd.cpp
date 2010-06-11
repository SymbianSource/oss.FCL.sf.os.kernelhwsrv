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
/*
Change History:
VERSION     : 2     26-01-2010     Ruixing Yang
REASON      : 
REFERENCE   : 
DESCRIPTION : The timer resolution is modified for better interrupt latency calculation

Change History:
VERSION     : 1     25-01-2010     Ruixing Yang
REASON      : 
REFERENCE   : 
DESCRIPTION : Initial implementation of BM_SUITE PDD for Rapu platform

*/


#include <kernel/kernel.h>
#include <internal/rap_hw.h>
#include <internal/rap.h>
#include "k32bm.h"




	

class DBMRapuDevice : public DPhysicalDevice
	{
public:
	DBMRapuDevice();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Validate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	};

class DBMRapuChannel : public DBMPChannel
	{
public:
	DBMRapuChannel();
	~DBMRapuChannel();
	virtual TBMTicks TimerPeriod();
	virtual TBMTicks TimerStamp();
	virtual TBMNs TimerTicksToNs(TBMTicks);
	virtual TBMTicks TimerNsToTicks(TBMNs);
	virtual TInt BindInterrupt(MBMIsr*);
	virtual TInt BindInterrupt(MBMInterruptLatencyIsr*);
	virtual void RequestInterrupt();
	virtual void CancelInterrupt();

private:
	
	static const TBMTicks	KBMRapuPeriod = (((TBMTicks) 1) << 32);
	// Ticks at 1000Hz, input clock to timer @ 32.768 MHz.
	static const TInt KHighResTimerFrequency = 32768000;   // 32.768 MHz
	
	static const TBMNs		KBMRapuNsPerTick = (1000*1000*1000) / KHighResTimerFrequency;	
	static const TInt KRTC_Freq = 32768;
	static const TInt KRTC_Ratio = 1172;
	

		
	static void Isr(TAny*);
	
	MBMIsr*								iIsr;
	MBMInterruptLatencyIsr*		iInterruptLatencyIsr;	
	TUint									iTmpStartCount;
	TUint									iTmpLongCount;	
	NTimer								iTimer;
	volatile TUint				iStartCount;
	volatile TUint				iRunCount;
	volatile TUint				iCancelCount;
	};
	
	RTC001_STR& RTC001 = *reinterpret_cast<RTC001_STR*>(KRapRegRTC001);
	GPT003_STR& GPT003 = *reinterpret_cast<GPT003_STR*>(KRapRegGPT003A0);
	
	
DECLARE_STANDARD_PDD()
//
// Create a new device
//
	{
	__ASSERT_CRITICAL;
	return new DBMRapuDevice;
	}
	
DBMRapuDevice::DBMRapuDevice()
//
// Constructor
//
	{
	
	iVersion = TVersion(1,0,1);
	}
	
TInt DBMRapuDevice::Install()
//
// Install the device driver.
//
	{
		
	TInt r = SetName(&KBMPdName);
	return r;
	}
	
void DBMRapuDevice::GetCaps(TDes8& aDes) const
//
// Return the Comm capabilities.
//
	{
	
	}
	
TInt DBMRapuDevice::Create(DBase*& aChannel, TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
//
// Create a channel on the device.
//
	{
		
	__ASSERT_CRITICAL;
	aChannel = new DBMRapuChannel;
	return aChannel?KErrNone:KErrNoMemory;
	}

TInt DBMRapuDevice::Validate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
	{
		
	if (!Kern::QueryVersionSupported(iVersion,aVer))
		{
		return KErrNotSupported;
		}
	return KErrNone;
	}
	
DBMRapuChannel::DBMRapuChannel()
	: iTimer(&Isr, this)
	{
	iTmpStartCount = 0;
	iTmpLongCount = 0;
	}
	
DBMRapuChannel::~DBMRapuChannel()
	{
	//Kern::Printf(("DBMRapuChannel::~DBMRapuChannel()"));	
	CancelInterrupt();
	}

TBMTicks DBMRapuChannel::TimerPeriod()
	{
		//Kern::Printf(("DBMRapuChannel::TimerPeriod()"));
	return KBMRapuPeriod;
	}

TBMTicks DBMRapuChannel::TimerStamp()
	{
	//Kern::Printf(("DBMRapuChannel::TimerStamp(), iTimerCount = %u"), RPTimer1::Timer().iTimerCount);	
	TUint tmpTimeStamp;
	RTC001.TRIGGER = 0;	
	tmpTimeStamp = RTC001.LONGCOUNT;
	tmpTimeStamp *= KRTC_Ratio;
	tmpTimeStamp += RTC001.SHORTCOUNT;		
	return tmpTimeStamp;	
	}	


TBMNs DBMRapuChannel::TimerTicksToNs(TBMTicks ticks)
	{
	//Kern::Printf(("DBMRapuChannel::TimerTIcksToNs(), iNsPerTick = %u"), (TBMTicks)iNsPerTick);
	return ticks * KBMRapuNsPerTick;
	}

TBMTicks DBMRapuChannel::TimerNsToTicks(TBMNs ns)
	{
	//Kern::Printf(("DBMRapuChannel::TimerNsToTicks()"));
	return ns / KBMRapuNsPerTick;	
	}
	
void DBMRapuChannel::Isr(TAny* ptr)
	{
	//Kern::Printf(("DBMRapuChannel::Isr()"));
	// Read RTC001	
	RTC001.TRIGGER = 0;
	TUint x = RTC001.LONGCOUNT;
	x *= KRTC_Ratio;
	x += RTC001.SHORTCOUNT;
	TUint wasteTime = 1000000/KBMRapuNsPerTick; //NTimer's resolution is 1 ms = 1000000 ns	
			
	DBMRapuChannel* mCh = (DBMRapuChannel*) ptr;
	BM_ASSERT(mCh->iIsr || mCh->iInterruptLatencyIsr);
	if (mCh->iIsr)
		{				
		mCh->iIsr->Isr( x);	
		}
	else
		{				
		TUint y = (TUint)( x - mCh->iTmpLongCount - wasteTime);
		//Kern::Printf(("DBMRapuChannel::Isr(), latency = %u"), y);	
		mCh->iInterruptLatencyIsr->InterruptLatencyIsr(y);
		}
	__e32_atomic_add_ord32(&mCh->iRunCount, 1);
	}	

TInt DBMRapuChannel::BindInterrupt(MBMIsr* aIsr)
	{
	//Kern::Printf(("DBMRapuChannel::BindInterrupt(MBMIsr* aIsr)"));
	BM_ASSERT(!iIsr);
	BM_ASSERT(!iInterruptLatencyIsr);
	iIsr = aIsr;
	return KErrNone;
	}

TInt DBMRapuChannel::BindInterrupt(MBMInterruptLatencyIsr* aIsr)
	{
	//Kern::Printf(("DBMRapuChannel::BindInterrupt(MBMInterruptLatencyIsr* aIsr)"));
	BM_ASSERT(!iIsr);
	BM_ASSERT(!iInterruptLatencyIsr);
	iInterruptLatencyIsr = aIsr;
	return KErrNone;
	}


void DBMRapuChannel::RequestInterrupt()
	{
	//Kern::Printf(("DBMRapuChannel::RequestInterrupt()"));	
	BM_ASSERT(iIsr || iInterruptLatencyIsr);		
	// Read RTC001
	RTC001.TRIGGER = 0;	
	iTmpLongCount = RTC001.LONGCOUNT;
	iTmpLongCount *= KRTC_Ratio;
	iTmpLongCount += RTC001.SHORTCOUNT;					
	if (iTimer.OneShot(1)==KErrNone)
		__e32_atomic_add_ord32(&iStartCount, 1);
	
	}

void DBMRapuChannel::CancelInterrupt()
	{
	iTmpStartCount = 0;
	//Kern::Printf(("DBMRapuChannel::CancelInterrupt()"));		
	if (iTimer.Cancel())
		__e32_atomic_add_ord32(&iCancelCount, 1);
	while (iStartCount != iCancelCount + iRunCount)
		{}
	}
	
	
		