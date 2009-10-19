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

#include "k32bm.h"

#include <emulator.h>

class DBMWinsDevice : public DPhysicalDevice
	{
public:
	DBMWinsDevice();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Validate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	};

class DBMWinsChannel : public DBMPChannel
	{
public:
	DBMWinsChannel();
	~DBMWinsChannel();
	virtual TBMTicks TimerPeriod();
	virtual TBMTicks TimerStamp();
	virtual TBMNs TimerTicksToNs(TBMTicks);
	virtual TBMTicks TimerNsToTicks(TBMNs);
	virtual TInt BindInterrupt(MBMIsr*);
	virtual TInt BindInterrupt(MBMInterruptLatencyIsr*);
	virtual void RequestInterrupt();
	virtual void CancelInterrupt();

private:

	static const TInt		KBMWinsInterruptDelayMs;
	static const TBMNs		KBMWinsNsPerTick;
	static const TBMTicks	KBMWinsPeriod;

	static void Isr(TAny*);
	static void InterruptLatencyIsr(TAny*);

	TInt						iRequestedTime;
	MBMIsr*						iIsr;
	MBMInterruptLatencyIsr*		iInterruptLatencyIsr;
	NTimer						iTimer;
	};
	
const TInt		DBMWinsChannel::KBMWinsInterruptDelayMs = 4;
const TBMNs		DBMWinsChannel::KBMWinsNsPerTick = 100;
const TBMTicks	DBMWinsChannel::KBMWinsPeriod = (((TBMTicks) 1) << 32);

DECLARE_STANDARD_PDD()
//
// Create a new device
//
	{
	__ASSERT_CRITICAL;
	return new DBMWinsDevice;
	}

DBMWinsDevice::DBMWinsDevice()
//
// Constructor
//
	{
	//iUnitsMask=0;
	iVersion = TVersion(1,0,1);
	}

TInt DBMWinsDevice::Install()
//
// Install the device driver.
//
	{
	TInt r = SetName(&KBMPdName);
	return r;
	}

void DBMWinsDevice::GetCaps(TDes8&) const
//
// Return the Comm capabilities.
//
	{
	}

TInt DBMWinsDevice::Create(DBase*& aChannel, TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
//
// Create a channel on the device.
//
	{
	__ASSERT_CRITICAL;
	aChannel = new DBMWinsChannel();
	return aChannel ? KErrNone : KErrNoMemory;
	}

TInt DBMWinsDevice::Validate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
	{
	if (!Kern::QueryVersionSupported(iVersion,aVer))
		{
		return KErrNotSupported;
		}
	return KErrNone;
	}

DBMWinsChannel::DBMWinsChannel() : iTimer(Isr, this)
	{
	// iIsr = NULL;
	// iInterruptLatencyIsr = NULL;
	}

DBMWinsChannel::~DBMWinsChannel()
	{
	}

TBMTicks DBMWinsChannel::TimerPeriod()
	{
	return KBMWinsPeriod;
	}

TBMTicks DBMWinsChannel::TimerStamp()
	{
	FILETIME now;
	GetSystemTimeAsFileTime(&now);
	return now.dwLowDateTime; 
	}

TBMNs DBMWinsChannel::TimerTicksToNs(TBMTicks ticks)
	{
	return ticks * KBMWinsNsPerTick;
	}

TBMTicks DBMWinsChannel::TimerNsToTicks(TBMNs ns)
	{
	return ns / KBMWinsNsPerTick;
	}

void DBMWinsChannel::Isr(TAny* ptr)
	{
	FILETIME now;
	GetSystemTimeAsFileTime(&now);
	DBMWinsChannel* pCh = (DBMWinsChannel*) ptr;
	BM_ASSERT(pCh->iIsr);
	pCh->iIsr->Isr(now.dwLowDateTime);
	}

TInt DBMWinsChannel::BindInterrupt(MBMIsr* aIsr)
	{
	BM_ASSERT(!iIsr);
	BM_ASSERT(!iInterruptLatencyIsr);
	iIsr = aIsr;
	return KErrNone;
	}

void DBMWinsChannel::InterruptLatencyIsr(TAny* ptr)
	{
	FILETIME now;
	GetSystemTimeAsFileTime(&now);
	DBMWinsChannel* pCh = (DBMWinsChannel*) ptr;
	TBMTicks latency = now.dwLowDateTime - pCh->iRequestedTime;
	BM_ASSERT(pCh->iInterruptLatencyIsr);
	pCh->iInterruptLatencyIsr->InterruptLatencyIsr(latency);
	}

TInt DBMWinsChannel::BindInterrupt(MBMInterruptLatencyIsr* aIsr)
	{
	BM_ASSERT(!iIsr);
	BM_ASSERT(!iInterruptLatencyIsr);
	iInterruptLatencyIsr = aIsr;
	iTimer.iFunction = InterruptLatencyIsr;
	return KErrNone;
	}

void DBMWinsChannel::RequestInterrupt()
	{
	BM_ASSERT(iIsr || iInterruptLatencyIsr);
	FILETIME now;
	GetSystemTimeAsFileTime(&now);
	iRequestedTime = now.dwLowDateTime + ((KBMWinsInterruptDelayMs * ((1000 * 1000) / (TInt) KBMWinsNsPerTick)));
	iTimer.OneShot(KBMWinsInterruptDelayMs);
	}
	
void DBMWinsChannel::CancelInterrupt()
	{
	iTimer.Cancel();
	}

