// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// \e32test\benchmark\bm_mcot_pdd.cpp
// PDD to provide OS timer services to benchmark programs
// 
//

#include <kernel/kernel.h>
#include <cotulla.h>


#include "k32bm.h"

class DBMMcotDevice : public DPhysicalDevice
	{
public:
	DBMMcotDevice();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Validate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	};

class DBMMcotChannel : public DBMPChannel
	{
public:
	DBMMcotChannel();
	~DBMMcotChannel();

	// Report timing spec
	virtual TBMTicks TimerPeriod();
	// Get current OST tick time
	virtual TBMTicks TimerStamp();
	// Tick/nS conversions
	virtual TBMNs TimerTicksToNs(TBMTicks);
	virtual TBMTicks TimerNsToTicks(TBMNs);
	// Pass in client ISRs to invoke 
	virtual TInt BindInterrupt(MBMIsr*);
	virtual TInt BindInterrupt(MBMInterruptLatencyIsr*);
	// Invoke an ISR 
	virtual void RequestInterrupt();
	virtual void CancelInterrupt();

private:

	// Attach to OST interrupt
	TInt BindInterrupt();

	static const TInt		KBMMcotInterruptDelayTicks = KHwOscFreqHz / 1000;	// ie. delay ~ 1 ms timer tick!
	static const TBMTicks	KBMMcotPeriod = (((TBMTicks) 1) << 32);
	static const TBMNs		KBMMcotNsPerTick = (1000*1000*1000) / KHwOscFreqHz;
	
	// Real ISR (calls out to client)
	static void Isr(TAny*);
	
	MBMIsr*						iIsr;
	MBMInterruptLatencyIsr*		iInterruptLatencyIsr;
	};

// Standard boiler plate PDD factory object 

DECLARE_STANDARD_PDD()
//
// Create a new device
//
	{
	__ASSERT_CRITICAL;
	return new DBMMcotDevice;
	}

DBMMcotDevice::DBMMcotDevice()
//
// Constructor
//
	{
	//iUnitsMask=0;
	iVersion = TVersion(1,0,1);
	}

TInt DBMMcotDevice::Install()
//
// Install the device driver.
//
	{
	TInt r = SetName(&KBMPdName);
	return r;
	}

void DBMMcotDevice::GetCaps(TDes8& aDes) const
//
// Return the Comm capabilities.
//
	{
	}

TInt DBMMcotDevice::Create(DBase*& aChannel, TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
//
// Create a channel on the device.
//
	{
	__ASSERT_CRITICAL;
	aChannel = new DBMMcotChannel;
	return aChannel?KErrNone:KErrNoMemory;
	}

TInt DBMMcotDevice::Validate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
	{
	if (!Kern::QueryVersionSupported(iVersion,aVer))
		{
		return KErrNotSupported;
		}
	return KErrNone;
	}

// Actual device channel functions, called by LDD

// Clear saved ISR members
DBMMcotChannel::DBMMcotChannel()
	{
	//	iIsr = NULL;
	//	iInterruptLatencyIsr = NULL;
	}

// If we ever initialised the timers to generate ISRs, clear 
DBMMcotChannel::~DBMMcotChannel()
	{
	if (iIsr || iInterruptLatencyIsr)
		{
		TCotulla::DisableOstInterrupt(KHwOstMatchGeneral);
		TCotulla::SetOstMatchEOI(KHwOstMatchGeneral);
		Interrupt::Disable(KIntIdOstMatchGeneral);
		TCotulla::ModifyIntLevels(KHtIntsOstMatchGeneral,0);
		Interrupt::Unbind(KIntIdOstMatchGeneral);
		}
	}

TBMTicks DBMMcotChannel::TimerPeriod()
	{
	return KBMMcotPeriod;
	}

// Read OST and return tick count
TBMTicks DBMMcotChannel::TimerStamp()
	{
	return TUint(TCotulla::OstData());
	}

TBMNs DBMMcotChannel::TimerTicksToNs(TBMTicks ticks)
	{
	return ticks * KBMMcotNsPerTick;
	}

TBMTicks DBMMcotChannel::TimerNsToTicks(TBMNs ns)
	{
	return ns / KBMMcotNsPerTick;
	}

// Actual ISR called when benchmark timer fires
void DBMMcotChannel::Isr(TAny* ptr)
	{
	DBMMcotChannel* mCh = (DBMMcotChannel*) ptr;
	BM_ASSERT(mCh->iIsr || mCh->iInterruptLatencyIsr);
	if (mCh->iIsr)
		{
		// Call the handler with the current OST time
		mCh->iIsr->Isr(TUint(TCotulla::OstData()));
		}
	else
		{
		// Call the handler with the difference between NOW and the time the match timer was set for
		// ie. the latency between the timer interrupt set and when it was called.
		mCh->iInterruptLatencyIsr->InterruptLatencyIsr(TCotulla::OstData() - TCotulla::OstMatch(KHwOstMatchGeneral));
		}
	TCotulla::DisableOstInterrupt(KHwOstMatchGeneral);
	TCotulla::SetOstMatchEOI(KHwOstMatchGeneral);
	}


// Bind the OST match interrupt to a FIQ ISR - it will interrupt running IRQ service
// routines. 
TInt DBMMcotChannel::BindInterrupt()
	{
	TInt r = Interrupt::Bind(KIntIdOstMatchGeneral, Isr, this);
	if (r != KErrNone)
		{
		return r;
		}
	TCotulla::ModifyIntLevels(0, KHtIntsOstMatchGeneral);	// route new timer interrupt to FIQ
	TCotulla::SetOstMatchEOI(KHwOstMatchGeneral);
	Interrupt::Enable(KIntIdOstMatchGeneral);
	return KErrNone;
	}

// NB! Only one of these is ever used per channel opening, ie. the 
// channel must be closed and re-opened between installing client ISR
TInt DBMMcotChannel::BindInterrupt(MBMIsr* aIsr)
	{
	BM_ASSERT(!iIsr);
	BM_ASSERT(!iInterruptLatencyIsr);
	iIsr = aIsr;
	return BindInterrupt();
	}

TInt DBMMcotChannel::BindInterrupt(MBMInterruptLatencyIsr* aIsr)
	{
	BM_ASSERT(!iIsr);
	BM_ASSERT(!iInterruptLatencyIsr);
	iInterruptLatencyIsr = aIsr;
	return BindInterrupt();
	}

// Called by client to request an interrupt (ISR/latency ISR) invocation in ~1mS.
void DBMMcotChannel::RequestInterrupt()
	{
	BM_ASSERT(iIsr || iInterruptLatencyIsr);
	TCotulla::SetOstMatch(TCotulla::OstData()+KBMMcotInterruptDelayTicks,KHwOstMatchGeneral);
	TCotulla::SetOstMatchEOI(KHwOstMatchGeneral);
	TCotulla::EnableOstInterrupt(KHwOstMatchGeneral);
	}

// Called to cancel if client dies/killed after ISR scheduled ???
void DBMMcotChannel::CancelInterrupt()
	{	
	if (iIsr || iInterruptLatencyIsr)
		{
		TCotulla::DisableOstInterrupt(KHwOstMatchGeneral);
		TCotulla::SetOstMatchEOI(KHwOstMatchGeneral);
		}
	}
	
