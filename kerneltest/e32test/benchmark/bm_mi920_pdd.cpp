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
#include <integratorap.h>
#include <iolines.h>

//	NOTE:
//	We use 2 different timers on this test:
//		1) the Core module FRC - 32-bit clocked at 24MHz, non-interruptible, for the time stamps
//		2) IntegratorAP Timer 1 - 16-bit, clocked at 1.5MHz , interruptible, for the Interrupts latency measurements
//

#define KHwFrcFreqHz	24000000			// uses the core module FRC clocked at 24MHz
#define KHwApFreqHz		24000000			// uses the IntegratorAP Timer 1 clocked at 24MHz

#include "k32bm.h"

class DBMMi920Device : public DPhysicalDevice
	{
public:
	DBMMi920Device();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Validate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	};

class DBMMi920Channel : public DBMPChannel
	{
public:
	DBMMi920Channel();
	~DBMMi920Channel();
	TInt InitialiseTimer();
	virtual TBMTicks TimerPeriod();
	virtual TBMTicks TimerStamp();
	virtual TBMNs TimerTicksToNs(TBMTicks);
	virtual TBMTicks TimerNsToTicks(TBMNs);
	virtual TInt BindInterrupt(MBMIsr*);
	virtual TInt BindInterrupt(MBMInterruptLatencyIsr*);
	virtual void RequestInterrupt();
	virtual void CancelInterrupt();

private:

	static void ClearAp1TimerInt();
	TInt BindInterrupt();

	static const TBMTicks	KBMMi920FrcPeriod = (((TBMTicks) 1) << 32);				// FRC
	static const TUint		KBMMi920ApPeriod = (((TBMTicks) 1) << 16);				// FRC
	static const TBMNs		KBMMi920NsPerTick = (1000*1000*1000) / KHwFrcFreqHz;	// FRC
	static const TUint		KBMMi920InterruptDelayTicks = KHwApFreqHz / 1000;		// 1ms in AP Timer 1 ticks
	
	static void Isr(TAny*);
	
	MBMIsr*						iIsr;
	MBMInterruptLatencyIsr*		iInterruptLatencyIsr;
	TInt				iInterruptId;
	};

DECLARE_STANDARD_PDD()
//
// Create a new device
//
	{
	__ASSERT_CRITICAL;
	return new DBMMi920Device;
	}

DBMMi920Device::DBMMi920Device()
//
// Constructor
//
	{
	//iUnitsMask=0;
	iVersion = TVersion(1,0,1);
	}

TInt DBMMi920Device::Install()
//
// Install the device driver.
//
	{
	TInt r = SetName(&KBMPdName);
	return r;
	}

void DBMMi920Device::GetCaps(TDes8& aDes) const
//
// Return the Comm capabilities.
//
	{
	}

TInt DBMMi920Device::Create(DBase*& aChannel, TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
//
// Create a channel on the device.
//
	{
	__ASSERT_CRITICAL;
	DBMMi920Channel* pD= new DBMMi920Channel;
	aChannel = pD;
	TInt r=KErrNoMemory;
	if (pD)
		r=pD->InitialiseTimer();
	return r;
	}

TInt DBMMi920Device::Validate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
	{
	if (!Kern::QueryVersionSupported(iVersion,aVer))
		{
		return KErrNotSupported;
		}
	return KErrNone;
	}

void DBMMi920Channel::ClearAp1TimerInt()
	{
	do
		TIntegratorAP::ClearTimerInt(TIntegratorAP::ECounterTimer1);
		// WARNING: Integrator bug! The timer interrupts are not always cleared the first time round
		// so keep on clearing them until they are no longer active.
	while (*(volatile TUint*)(KHwIrqBaseSet0) & KHtIntTimer1);
	}

DBMMi920Channel::DBMMi920Channel()
	{
	//	iIsr = NULL;
	//	iInterruptLatencyIsr = NULL;
	iInterruptId = KIntIdMatchGeneral2;
	}

DBMMi920Channel::~DBMMi920Channel()
	{
	// Disable the IntegratorAP Timer 1 and associated interrupt
	if (iIsr || iInterruptLatencyIsr)
		{
		ClearAp1TimerInt();
		Interrupt::Disable(iInterruptId);
		Interrupt::Unbind(iInterruptId);
		}
	TIntegratorAP::EnableTimer(TIntegratorAP::ECounterTimer1, TIntegratorAP::EDisable);
	}

TInt DBMMi920Channel::InitialiseTimer()
	{
	// Set up the IntegratorAP Timer 1 to run at 1.5MHz, free-running mode, and enable it
    TIntegratorAP::SetTimerMode(TIntegratorAP::ECounterTimer1, TIntegratorAP::ETimerModeFreeRunning);
	TIntegratorAP::SetTimerPreScale(TIntegratorAP::ECounterTimer1, TIntegratorAP::ETimerPreScaleNone);	// 24MHz
    TIntegratorAP::EnableTimer(TIntegratorAP::ECounterTimer1, TIntegratorAP::EEnable);
	return KErrNone;
	}

TBMTicks DBMMi920Channel::TimerPeriod()
	{
	return KBMMi920FrcPeriod;
	}

TBMTicks DBMMi920Channel::TimerStamp()
	{
	return (*(volatile TUint*)(KHwRwCoreClkCounter));		// FRC
	}

TBMNs DBMMi920Channel::TimerTicksToNs(TBMTicks ticks)
	{
	return ticks * KBMMi920NsPerTick;						// FRC
	}

TBMTicks DBMMi920Channel::TimerNsToTicks(TBMNs ns)
	{
	return ns / KBMMi920NsPerTick;							// FRC
	}


void DBMMi920Channel::Isr(TAny* ptr)
	{
	DBMMi920Channel* mCh = (DBMMi920Channel*) ptr;
	BM_ASSERT(mCh->iIsr || mCh->iInterruptLatencyIsr);
	if (mCh->iIsr)
		{
		mCh->iIsr->Isr(*(volatile TUint*)(KHwRwCoreClkCounter));	// read timestamp off FRC
		}
	else
		{ // calculate time elapsed between the interrupt going off and now in FRC ticks
		TUint value = TIntegratorAP::TimerValue(TIntegratorAP::ECounterTimer1);
		BM_ASSERT(((KHwFrcFreqHz/KHwApFreqHz)*KHwApFreqHz) == KHwFrcFreqHz);
		mCh->iInterruptLatencyIsr->InterruptLatencyIsr((KBMMi920ApPeriod - value) * (KHwFrcFreqHz/KHwApFreqHz));
		}
	ClearAp1TimerInt();
	Interrupt::Disable(mCh->iInterruptId);
	}

TInt DBMMi920Channel::BindInterrupt()
	{
	BM_ASSERT(iInterruptId==KIntIdMatchGeneral2);
	TInt r=Interrupt::Bind(iInterruptId, Isr, this);
	if (r<0)
		{
		return r;
		}
	iInterruptId = r;
	return KErrNone;
	}

TInt DBMMi920Channel::BindInterrupt(MBMIsr* aIsr)
	{
	BM_ASSERT(!iIsr);
	BM_ASSERT(!iInterruptLatencyIsr);
	iIsr = aIsr;
	return BindInterrupt();
	}

TInt DBMMi920Channel::BindInterrupt(MBMInterruptLatencyIsr* aIsr)
	{
	BM_ASSERT(!iIsr);
	BM_ASSERT(!iInterruptLatencyIsr);
	iInterruptLatencyIsr = aIsr;
	return BindInterrupt();
	}


void DBMMi920Channel::RequestInterrupt()
	{
	BM_ASSERT(iIsr || iInterruptLatencyIsr);
	TIntegratorAP::SetTimerLoad(TIntegratorAP::ECounterTimer1, KBMMi920InterruptDelayTicks);
	ClearAp1TimerInt();
	Interrupt::Enable(iInterruptId);					// Timer interrupts on Integrator are always enabled!
	}

void DBMMi920Channel::CancelInterrupt()
	{	
	if (iIsr || iInterruptLatencyIsr)
		{
		TIntegratorAP::EnableTimer(TIntegratorAP::ECounterTimer1, TIntegratorAP::EDisable);
		}
	}
	
