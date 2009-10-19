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
#include <sa1100.h>

#include "k32bm.h"

class DBMMisaDevice : public DPhysicalDevice
	{
public:
	DBMMisaDevice();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Validate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	};

class DBMMisaChannel : public DBMPChannel
	{
public:
	DBMMisaChannel();
	~DBMMisaChannel();
	virtual TBMTicks TimerPeriod();
	virtual TBMTicks TimerStamp();
	virtual TBMNs TimerTicksToNs(TBMTicks);
	virtual TBMTicks TimerNsToTicks(TBMNs);
	virtual TInt BindInterrupt(MBMIsr*);
	virtual TInt BindInterrupt(MBMInterruptLatencyIsr*);
	virtual void RequestInterrupt();
	virtual void CancelInterrupt();

private:

	TInt BindInterrupt();

	static const TInt		KBMMisaInterruptDelayTicks = KHwOscFreqHz / 1000;
	static const TBMTicks	KBMMisaPeriod = (((TBMTicks) 1) << 32);
	static const TBMNs		KBMMisaNsPerTick = (1000*1000*1000) / KHwOscFreqHz;
	
	static void Isr(TAny*);
	
	MBMIsr*						iIsr;
	MBMInterruptLatencyIsr*		iInterruptLatencyIsr;
	};


DECLARE_STANDARD_PDD()
//
// Create a new device
//
	{
	__ASSERT_CRITICAL;
	return new DBMMisaDevice;
	}

DBMMisaDevice::DBMMisaDevice()
//
// Constructor
//
	{
	//iUnitsMask=0;
	iVersion = TVersion(1,0,1);
	}

TInt DBMMisaDevice::Install()
//
// Install the device driver.
//
	{
	TInt r = SetName(&KBMPdName);
	return r;
	}

void DBMMisaDevice::GetCaps(TDes8& aDes) const
//
// Return the Comm capabilities.
//
	{
	}

TInt DBMMisaDevice::Create(DBase*& aChannel, TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
//
// Create a channel on the device.
//
	{
	__ASSERT_CRITICAL;
	aChannel = new DBMMisaChannel;
	return aChannel?KErrNone:KErrNoMemory;
	}

TInt DBMMisaDevice::Validate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
	{
	if (!Kern::QueryVersionSupported(iVersion,aVer))
		{
		return KErrNotSupported;
		}
	return KErrNone;
	}

DBMMisaChannel::DBMMisaChannel()
	{
	//	iIsr = NULL;
	//	iInterruptLatencyIsr = NULL;
	}

DBMMisaChannel::~DBMMisaChannel()
	{
	if (iIsr || iInterruptLatencyIsr)
		{
		TSa1100::DisableOstInterrupt(KHwOstMatchGeneral);
		TSa1100::SetOstMatchEOI(KHwOstMatchGeneral);
		Interrupt::Disable(KIntIdOstMatchGeneral);
		TSa1100::ModifyIntLevels(KHtIntsOstMatchGeneral,0);
		Interrupt::Unbind(KIntIdOstMatchGeneral);
		}
	}

TBMTicks DBMMisaChannel::TimerPeriod()
	{
	return KBMMisaPeriod;
	}

TBMTicks DBMMisaChannel::TimerStamp()
	{
	return TUint(TSa1100::OstData());
	}

TBMNs DBMMisaChannel::TimerTicksToNs(TBMTicks ticks)
	{
	return ticks * KBMMisaNsPerTick;
	}

TBMTicks DBMMisaChannel::TimerNsToTicks(TBMNs ns)
	{
	return ns / KBMMisaNsPerTick;
	}

void DBMMisaChannel::Isr(TAny* ptr)
	{
	DBMMisaChannel* mCh = (DBMMisaChannel*) ptr;
	BM_ASSERT(mCh->iIsr || mCh->iInterruptLatencyIsr);
	if (mCh->iIsr)
		{
		mCh->iIsr->Isr(TUint(TSa1100::OstData()));
		}
	else
		{
		mCh->iInterruptLatencyIsr->InterruptLatencyIsr(TSa1100::OstData() - TSa1100::OstMatch(KHwOstMatchGeneral));
		}
	TSa1100::DisableOstInterrupt(KHwOstMatchGeneral);
	TSa1100::SetOstMatchEOI(KHwOstMatchGeneral);
	}

TInt DBMMisaChannel::BindInterrupt()
	{
	TInt r = Interrupt::Bind(KIntIdOstMatchGeneral, Isr, this);
	if (r != KErrNone)
		{
		return r;
		}
	TSa1100::ModifyIntLevels(0, KHtIntsOstMatchGeneral);	// route new timer interrupt to FIQ
	TSa1100::SetOstMatchEOI(KHwOstMatchGeneral);
	Interrupt::Enable(KIntIdOstMatchGeneral);
	return KErrNone;
	}

TInt DBMMisaChannel::BindInterrupt(MBMIsr* aIsr)
	{
	BM_ASSERT(!iIsr);
	BM_ASSERT(!iInterruptLatencyIsr);
	iIsr = aIsr;
	return BindInterrupt();
	}

TInt DBMMisaChannel::BindInterrupt(MBMInterruptLatencyIsr* aIsr)
	{
	BM_ASSERT(!iIsr);
	BM_ASSERT(!iInterruptLatencyIsr);
	iInterruptLatencyIsr = aIsr;
	return BindInterrupt();
	}


void DBMMisaChannel::RequestInterrupt()
	{
	BM_ASSERT(iIsr || iInterruptLatencyIsr);
	TSa1100::SetOstMatch(KHwOstMatchGeneral, TSa1100::OstData() + KBMMisaInterruptDelayTicks);
	TSa1100::SetOstMatchEOI(KHwOstMatchGeneral);
	TSa1100::EnableOstInterrupt(KHwOstMatchGeneral);
	}

void DBMMisaChannel::CancelInterrupt()
	{	
	if (iIsr || iInterruptLatencyIsr)
		{
		TSa1100::DisableOstInterrupt(KHwOstMatchGeneral);
		TSa1100::SetOstMatchEOI(KHwOstMatchGeneral);
		}
	}
	
