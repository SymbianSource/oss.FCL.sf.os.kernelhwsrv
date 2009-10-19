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
// e32test\benchmark\bm_momap_pdd.cpp
// 
//

#include <kernel/kernel.h>
#include <omap.h>
#include <omap_plat.h>
#include <omap_powerresources.h>	// TResourceMgr methods
#include <powerresources_assp.h>	// ASSP resources
#include "k32bm.h"

// Note that this uses GPTimer2, these make it easy to swap around
const TUint KGPTimerBase = KHwBaseGpTimer2Reg;
const TUint KGPTimerClkSel = KHt_MOD_CONF_CTRL1_GPTIMER2_CLK_SEL;
const TUint KGPTimerInt = EIrqLv1_GpTimer2;

class DBmOmap : public DPhysicalDevice
	{
public:
	DBmOmap();
	~DBmOmap();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Validate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	};

class DBmOmapChannel : public DBMPChannel
	{
public:
	DBmOmapChannel();
	~DBmOmapChannel();
	virtual TBMTicks TimerPeriod();						// Report timing spec
	virtual TBMTicks TimerStamp();						// Get current system timer tick time
	virtual TBMNs TimerTicksToNs(TBMTicks);				// Tick/nS conversions
	virtual TBMTicks TimerNsToTicks(TBMNs);
	virtual TInt BindInterrupt(MBMIsr*);				// Pass in client ISRs to invoke
	virtual TInt BindInterrupt(MBMInterruptLatencyIsr*);
	virtual void RequestInterrupt();					// Invoke an ISR
	virtual void CancelInterrupt();

private:
	TInt BindInterrupt();								// Attach to OST interrupt

	static const TInt		KOmapOscFreqHz = 3000000;	// 12Mhz / 4 = 3MHz
	static const TBMTicks	KBMOmapPeriod = (((TBMTicks) 1) << 32);
	static const TBMNs		KBMOmapNsPerTick = (1000*1000*1000) / KOmapOscFreqHz;

	// calculate 1ms in timer ticks
	static const TInt		KBMOmapInterruptDelayTicks = KOmapOscFreqHz / 1000;

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
	return new DBmOmap;
	}

DBmOmap::DBmOmap()
//
// Constructor
//
	{
	//iUnitsMask=0;
	iVersion = TVersion(1,0,1);

	// place requirement on xor clock
	TResourceMgr::Request(KPowerArmEn_XorpCk);
	// Stop timer
	TOmap::ModifyRegister32(KGPTimerBase + KHoGpTimer_TCLR, KHtGpTimer_TCLR_St, KClear32); //Stop the timer

	// Drive this gptimer by the arm xor clock
	TOmapPlat::ModifyConfigReg(KHoBaseConfMOD_CONF_CTRL1, KGPTimerClkSel, 0);
	// Prescale enable = 12/4 = 3MHz
	TOmap::ModifyRegister32(KGPTimerBase + KHoGpTimer_TCLR, (KHmGpTimer_TCLR_PTV),  (1 << KHsGpTimer_TCLR_PTV | KHtGpTimer_TCLR_PRE) );

	// Enable "smart Idle mode"
	TOmap::SetRegister32(KGPTimerBase + KHoGpTimerTIOCP_CFG, (KHtGpTimer_TIOCP_CFG_SmartIdle << KHsGpTimer_TIOCP_CFG_IdleMode));
	// Load TimerLoad register to zero so when overflow occurs the counter starts from zero again.
	TOmap::SetRegister32(KGPTimerBase + KHoGpTimer_TLDR, 0x0);
	// Load Timer Trig register which clears the prescale counter and loads the value in TLDR to TCRR
	TOmap::SetRegister32(KGPTimerBase + KHoGpTimer_TTGR, 0x1);

	// Start the GPTimer. This configuration will pause counting when stopped by the jtag
	TOmap::ModifyRegister32(KGPTimerBase + KHoGpTimer_TCLR, KClear32, (KHtGpTimer_TCLR_St|KHtGpTimer_TCLR_AR|KHtGpTimer_TCLR_CE));
	while(TOmap::Register32(KGPTimerBase + KHoGpTimer_TWPS));
	}

DBmOmap::~DBmOmap()
	{
	// Stop timer
	TOmap::ModifyRegister32(KGPTimerBase + KHoGpTimer_TCLR, KHtGpTimer_TCLR_St, KClear32);
	while(TOmap::Register32(KGPTimerBase + KHoGpTimer_TWPS));

	// Release requirement on xor clock
	TResourceMgr::Release(KPowerArmEn_XorpCk);
	}

TInt DBmOmap::Install()
//
// Install the device driver.
//
	{
	TInt r = SetName(&KBMPdName);
	return r;
	}

void DBmOmap::GetCaps(TDes8& aDes) const
//
// Return the Comm capabilities.
//
	{
	}

TInt DBmOmap::Create(DBase*& aChannel, TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
//
// Create a channel on the device.
//
	{
	__ASSERT_CRITICAL;
	aChannel = new DBmOmapChannel;
	return aChannel?KErrNone:KErrNoMemory;
	}

TInt DBmOmap::Validate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
	{
	if (!Kern::QueryVersionSupported(iVersion,aVer))
		{
		return KErrNotSupported;
		}
	return KErrNone;
	}

// Note that the standard benchmark tests will expect to create >1
// channel (with the same LDD/PDD(unit)) hence this function must be
// capable of being invoked multiple times.

DBmOmapChannel::DBmOmapChannel()
	{
	//	iIsr = NULL;
	//	iInterruptLatencyIsr = NULL;
	}

DBmOmapChannel::~DBmOmapChannel()
	{
	if (iIsr || iInterruptLatencyIsr)
		{
		// Leave timer running
		// TOmap::ModifyRegister32(KGPTimerBase + KHoGpTimer_TCLR, KHtGpTimer_TCLR_St, KClear32); //Stop the timer
		Interrupt::Disable(KGPTimerInt);
		Interrupt::Unbind(KGPTimerInt);
		}
	}

TBMTicks DBmOmapChannel::TimerPeriod()
	{
	return KBMOmapPeriod;
	}

TBMTicks DBmOmapChannel::TimerStamp()
	{
	return TUint(TOmap::Register32(KGPTimerBase + KHoGpTimer_TCRR));
	}

TBMNs DBmOmapChannel::TimerTicksToNs(TBMTicks ticks)
	{
	return ticks * KBMOmapNsPerTick;
	}

TBMTicks DBmOmapChannel::TimerNsToTicks(TBMNs ns)
	{
	return ns / KBMOmapNsPerTick;
	}

void DBmOmapChannel::Isr(TAny* ptr)
	{
	DBmOmapChannel* mCh = (DBmOmapChannel*) ptr;
	BM_ASSERT(mCh->iIsr || mCh->iInterruptLatencyIsr);
	if (mCh->iIsr)
		{
		mCh->iIsr->Isr(TUint(TOmap::Register32(KGPTimerBase + KHoGpTimer_TCRR)));
		}
	else
		{
		mCh->iInterruptLatencyIsr->InterruptLatencyIsr(TOmap::Register32(KGPTimerBase+KHoGpTimer_TCRR) - TOmap::Register32(KGPTimerBase+KHoGpTimer_TMAR));
		}
	TOmap::ModifyRegister32(KGPTimerBase+KHoGpTimer_TIER, KHtGpTimer_TIER_Match, KClear32);
	}

TInt DBmOmapChannel::BindInterrupt()
	{
	TInt r=Interrupt::Bind(KGPTimerInt, Isr, this);
	if (r != KErrNone)
		{
		return r;
		}
	// Clear Match interrupt status bit
	TOmap::SetRegister32(KGPTimerBase + KHoGpTimer_TISR, KHtGpTimer_TISR_Match);

	Interrupt::Enable(KGPTimerInt);
	return KErrNone;
	}

TInt DBmOmapChannel::BindInterrupt(MBMIsr* aIsr)
	{
	BM_ASSERT(!iIsr);
	BM_ASSERT(!iInterruptLatencyIsr);
	iIsr = aIsr;
	return BindInterrupt();
	}

TInt DBmOmapChannel::BindInterrupt(MBMInterruptLatencyIsr* aIsr)
	{
	BM_ASSERT(!iIsr);
	BM_ASSERT(!iInterruptLatencyIsr);
	iInterruptLatencyIsr = aIsr;
	return BindInterrupt();
	}


void DBmOmapChannel::RequestInterrupt()
	{
	BM_ASSERT(iIsr || iInterruptLatencyIsr);
	TOmap::SetRegister32(KGPTimerBase+KHoGpTimer_TMAR, TOmap::Register32(KGPTimerBase+KHoGpTimer_TCRR) + KBMOmapInterruptDelayTicks);
	// Clear Match interrupt
	TOmap::SetRegister32(KGPTimerBase+KHoGpTimer_TISR, KHtGpTimer_TISR_Match);
	// Enable Match interrupt
	TOmap::SetRegister32(KGPTimerBase+KHoGpTimer_TIER, KHtGpTimer_TIER_Match);
	}

void DBmOmapChannel::CancelInterrupt()
	{
	if (iIsr || iInterruptLatencyIsr)
		{
		// Disable Match interrupt
		TOmap::ModifyRegister32(KGPTimerBase+KHoGpTimer_TIER, KHtGpTimer_TIER_Match, KClear32);
		}
	}
