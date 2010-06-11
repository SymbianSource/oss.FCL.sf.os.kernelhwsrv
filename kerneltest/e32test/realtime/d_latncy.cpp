// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\realtime\d_latncy.cpp
// 
//

#include "platform.h"

#if defined(__MEIG__)
#include <cl7211.h>
#elif defined(__MAWD__)
#include <windermere.h>
#elif defined(__MISA__)
#include <sa1100.h>
#elif defined(__MCOT__)
#include <cotulla.h>
#elif defined(__MI920__) || defined(__NI1136__)
#include <integratorap.h>
//#define FREE_RUNNING_MODE			// runs the millisecond timer in free running mode
#elif defined(__IS_OMAP1610__)
#include <omap_timer.h>
#include <omap_plat.h>
#elif defined(__IS_OMAP2420__) || defined(__WAKEUP_3430__)
#include <omap_hw.h>
#include <shared_instrtimer.h>
#elif defined(__EPOC32__) && defined(__CPU_X86)
#include <x86.h>
#include <x86pc.h>
#elif defined(__RVEMUBOARD__)
#include <rvemuboard.h>
#elif defined(__NE1_TB__)
#include <upd35001_timer.h>
#endif

#ifdef __CPU_ARM
#include <arm.h>
#endif

#include <kernel/kern_priv.h>		//temporary
#include "d_latncy.h"

_LIT(KLddName,"Latency");
_LIT(KThreadName,"LatencyThreadK");

#if defined(__MEIG__)
const TInt KTickPeriodMs=2;
const TInt KTicksPerMillisecond=512;
#elif defined(__MAWD__)
const TInt KTickPeriodMs=1;
const TInt KTicksPerMillisecond=512;
#elif defined(__MISA__) || defined(__MCOT__)
const TInt KTicksPerMillisecond=3686;
const TInt KOstTicks=3685;	// not quite 1ms, so it goes in and out of phase with ms timer
TUint TriggerTime;
#elif defined(__MI920__) || defined(__NI1136__)
const TInt KTickPeriodMs=1;
#if defined(__MI920__) || defined(__NI1136__)
#ifdef FREE_RUNNING_MODE	
const TInt KTicksPerMillisecond=1500;
#else
const TInt KTicksPerMillisecond=24000;
#endif
#endif
#elif defined(__IS_OMAP1610__)
const TInt KTickPeriodMs=1;
TInt KTicksPerMillisecond = TOmapPlat::GetInputClk()/32000;
#elif defined(__IS_OMAP2420__) || defined(__WAKEUP_3430__)
const TInt KTickPeriodMs=1;					// defined for compatibility but not used (ignored)
const TInt KTicksPerMillisecond = 12000; 	// Hard coded (12Mhz)
#elif defined(__EPOC32__) && defined(__CPU_X86)
const TInt KTickPeriodMs=1;
const TInt KTicksPerMillisecond=1193;
#elif defined(__RVEMUBOARD__)
const TInt KTickPeriodMs=1;
const TInt KTicksPerMillisecond=1000;
#elif defined(__NE1_TB__)
const TInt KTickPeriodMs=1;
const TInt KTicksPerMillisecond=66667;
#elif defined(__MRAP__)
const TInt KTickPeriodMs=1;
const TInt KTicksPerMillisecond=1000;
#endif

#ifdef _DEBUG
const TInt KFudgeFactor=1;
#else
const TInt KFudgeFactor=1;
#endif

class DDeviceLatency : public DLogicalDevice
	{
public:
	DDeviceLatency();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DLatencyPowerHandler : public DPowerHandler
	{
public: // from DPOwerHandler
	void PowerUp();
	void PowerDown(TPowerState);
public:
	DLatencyPowerHandler(DLatency* aChannel);
public:
	DLatency* iChannel;
	};



inline TUint DLatency::Ticks()
	{
#if defined(__MEIG__)
	return KTicksPerMillisecond-(*(volatile TUint*)(KEigerTimer2Data16+KEigerBaseAddress)&0xffff);
#elif defined(__MAWD__)
	return KTicksPerMillisecond-(*(volatile TUint*)(KWindTimer2Value16+KWindBaseAddress)&0xffff);
#elif defined(__MISA__) || defined(__MCOT__)
	return *(volatile TUint*)KHwRwOstOscr-iTriggerTime;
#elif defined(__MI920__) || defined(__NI1136__)
	return KTicksPerMillisecond-(*(volatile TUint*)(KHwCounterTimer2+KHoTimerValue)&0xffff);
#elif defined(__IS_OMAP1610__)
	return KTicksPerMillisecond - *(volatile TUint*)(KHwBaseOSTimer1Reg+KHoOSTimer_READ_TIM);
#elif defined(__IS_OMAP2420__) || defined(__WAKEUP_3430__)
	return (*(volatile TUint*)(iTimerInfo.iAddress + KHoGpTimer_TCRR)) - iTimerLoadValue;
#elif defined(__X86PC__)
	return 1194 - __HwTimer();
#elif defined(__RVEMUBOARD__)
	return KTicksPerMillisecond-(*(volatile TUint*)(KHwCounterTimer1+KHoTimerValue)&0xffff);
#elif defined(__NE1_TB__)
	return NETimer::Timer(0).iTimerCount;	// counts up, reset timer + interrupt on match
#endif
	}

#if !defined(__SMP__)
#if !defined(__EPOC32__) || !defined(__CPU_X86)
extern TUint IntStackPtr();
#endif
#endif

DECLARE_STANDARD_LDD()
	{
	return new DDeviceLatency;
	}

DDeviceLatency::DDeviceLatency()
//
// Constructor
//
	{
	//iParseMask=0;
	//iUnitsMask=0;
	iVersion=TVersion(1,0,1);
	}

TInt DDeviceLatency::Install()
//
// Install the device driver.
//
	{
	TInt r=SetName(&KLddName);
	return r;
	}

void DDeviceLatency::GetCaps(TDes8& aDes) const
//
// Return the Comm capabilities.
//
	{
	}

TInt DDeviceLatency::Create(DLogicalChannelBase*& aChannel)
//
// Create a channel on the device.
//
	{
	aChannel=new DLatency;
	return aChannel?KErrNone:KErrNoMemory;
	}

DLatency::DLatency()
	:	iMsCallBack(MsCallBack,this),
		iMsDfc(MsDfc,this,NULL,1)
//
// Constructor
//
	{
#if !defined(__SMP__)
#if !defined(__EPOC32__) || !defined(__CPU_X86)
	iIntStackTop=(TUint*)IntStackPtr();
#endif
#endif
#if defined(__MISA__) || defined(__MCOT__)
	iTickIncrement=KOstTicks*KFudgeFactor;
#endif
#if defined(__IS_OMAP2420__) || defined(__WAKEUP_3430__)
	iTimerInfo.iAddress = 0;
#endif
	}

DLatency::~DLatency()
//
// Destructor
//
	{
	iOff = (TUint8)ETrue;
	StopTimer();
	iMsDfc.Cancel();

	if (iRtDfcQ)
		iRtDfcQ->Destroy();

	if (iPowerHandler)
		{
		iPowerHandler->Remove();
		delete iPowerHandler;
		}

	Kern::SafeClose((DObject*&)iClient, NULL);
	}

TInt DLatency::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
// Create the channel from the passed info.
//
	{
	if (!Kern::QueryVersionSupported(TVersion(1,0,1),aVer))
		return KErrNotSupported;

	// create the power handler
	iPowerHandler = new DLatencyPowerHandler(this);
	if (!iPowerHandler)
		return KErrNoMemory;
	iPowerHandler->Add();

	// Allocate a kernel thread to run the DFC 
	TInt r = Kern::DynamicDfcQCreate(iRtDfcQ, KNumPriorities-1,KThreadName);

	if (r != KErrNone)
		return r;
	
#ifdef CPU_AFFINITY_ANY
	NKern::ThreadSetCpuAffinity((NThread*)(iRtDfcQ->iThread), KCpuAffinityAny);			
#endif

	iMsDfc.SetDfcQ(iRtDfcQ);
	iClient=&Kern::CurrentThread();
	iClient->Open();
	Kern::SetThreadPriority(KNumPriorities-2);
	return KErrNone;
	}

#if defined(__MISA__) 
// For SA1100/SA1110 use a separate timer on a FIQ interrupt (OST match 0)
TInt DLatency::StartTimer()
	{
	TInt r=Interrupt::Bind(KIntIdOstMatchGeneral,MsCallBack,this);
	if (r==KErrNone)
		{
		TSa1100::ModifyIntLevels(0,KHtIntsOstMatchGeneral);	// route new timer interrupt to FIQ
		TSa1100::SetOstMatchEOI(KHwOstMatchGeneral);
		TUint oscr=TSa1100::OstData();
		iTriggerTime=oscr+KOstTicks*KFudgeFactor;
		TSa1100::SetOstMatch(KHwOstMatchGeneral,iTriggerTime);
		TSa1100::EnableOstInterrupt(KHwOstMatchGeneral);
		Interrupt::Enable(KIntIdOstMatchGeneral);
		}
	return r;
	}
#elif defined(__MCOT__)
// For Cotulla use a separate timer on a FIQ interrupt (OST match 0)
TInt DLatency::StartTimer()
	{
	TInt r=Interrupt::Bind(KIntIdOstMatchGeneral,MsCallBack,this);
	if (r==KErrNone)
		{
		TCotulla::ModifyIntLevels(0,KHtIntsOstMatchGeneral);	// route new timer interrupt to FIQ
		TCotulla::SetOstMatchEOI(KHwOstMatchGeneral);
		TUint oscr=TCotulla::OstData();
		iTriggerTime=oscr+KOstTicks*KFudgeFactor;
		TCotulla::SetOstMatch(iTriggerTime,KHwOstMatchGeneral);
		TCotulla::EnableOstInterrupt(KHwOstMatchGeneral);
		Interrupt::Enable(KIntIdOstMatchGeneral);
		}
	return r;
	}
#elif defined(__IS_OMAP2420__) || defined(__WAKEUP_3430__)
TInt DLatency::StartTimer()
/* 
 *  For OMAP2420 initialise a new timer to generate an interrupt every 1ms
 */
	{
	__ASSERT_ALWAYS(!iTimerInfo.iAddress, Kern::Fault("D_Latncy: timer allocated twice.",
													 iTimerInfo.iAddress));

	// Get an available Timer from the system
    TInt r = OmapTimerMgr::GetTimer(iGPTimerId, iTimerInfo);
    if (KErrNone != r)
    	{
    	return r;
    	}
    	
    // Configure the timer
    r = ConfigureTimer();
    if (KErrNone != r)
    	{
    	DisableTimer();
    	return r;
    	}
    
    // Bind to timer interrupt
    r = Interrupt::Bind(iTimerInfo.iInterruptId, MsCallBack, this);
    if (KErrNone != r)
        {
        DisableTimer();
        return r;
        }
              
    // Unmask timer IT in interrupt controller
    r = Interrupt::Enable(iTimerInfo.iInterruptId);
    if (KErrNone != r)
		{
		Interrupt::Unbind(iTimerInfo.iInterruptId);
		DisableTimer();
    	return r;
    	}
    
    // Start timer
    TOmap::ModifyRegister32(iTimerInfo.iAddress + KHoGpTimer_TCLR, KClear32,
                        KHtGpTimer_TCLR_St);
        
    return KErrNone;
	}

void DLatency::DisableTimer()
/*
 *	Disable the interface and functional clock and mark the timer as available 
 */
	{
	  // Stop timer
    TOmap::ModifyRegister32(iTimerInfo.iAddress + KHoGpTimer_TCLR,
                        KHtGpTimer_TCLR_St, KClear32);

#if defined(__WAKEUP_3430__)
    // Disable Timer clocks using Timer framework instead of using TPRcm direct calls for 3430
    TInt r = OmapTimerMgr::DisableClocks(iGPTimerId);
    if (r != KErrNone)
        __ASSERT_ALWAYS(r, Kern::Fault("Timer clocks disable failed", 0)) ;
#else
	// Disable timer interface clock in PRCM
	TPrcm::InterfaceClkCtrl(iTimerInfo.iPrcmDeviceId, EFalse);
	
	// Disable timer functional clock in PRCM
	TPrcm::FunctionalClkCtrl(iTimerInfo.iPrcmDeviceId, EFalse);
#endif

	// Release the timer
	OmapTimerMgr::ReleaseTimer(iGPTimerId);
	
	iTimerInfo.iAddress = 0;
	}


TInt DLatency::ConfigureTimer()
/*
 *	This method will configure a timer to:
 *		-	run at the system clock (12Mhz)
 *		-	no prescaler (disable TCLR[PRE])
 *		-   autoreload and overflow interrupt enabled (TLDR will contain a
 *			value to generate an interrupt every 1000microsec)
 */
	{

#if defined(__WAKEUP_3430__)
	// Enable Timer clocks using timer framework instead of TPrcm direct calls for 3430
    TInt r = OmapTimerMgr::EnableClocks(iGPTimerId);
    if (r != KErrNone)
        __ASSERT_ALWAYS(r, Kern::Fault("Timer Clocks enable failed", 0)) ;

	// Select the input clock to be system clock 
    r = OmapTimerMgr::SetTimerClkSrc(iGPTimerId, ESysClk);
#else
	// Enable timer interface clock in PRCM  
	TPrcm::InterfaceClkCtrl(iTimerInfo.iPrcmDeviceId, ETrue, ETrue);
	// Enable timer functional clock in PRCM
	TPrcm::FunctionalClkCtrl(iTimerInfo.iPrcmDeviceId, ETrue, ETrue);

	// Select the input clock to be system clock 
    TInt r = OmapTimerMgr::SetTimerClkSrc(iGPTimerId, ESysClk);
#endif

    if (KErrNone != r)	
    	return r;

    // Timer OCP configuration: - software reset
    TOmap::SetRegister32( iTimerInfo.iAddress + KHoGpTimerTIOCP_CFG,
                          KHtGpTimer_TIOCP_CFG_SoftReset);

    // Wait for reset to be complete
    TUint16 timeOut = 1000;
    while ( !(TOmap::Register32(iTimerInfo.iAddress + KHoGpTimer_TISTAT) & 
    			KHtGpTimer_TISTAT_ResetComplete)
    			&& --timeOut);
    
   // Check if the timer has been reset or we hit the timeout
   __ASSERT_ALWAYS((TOmap::Register32(iTimerInfo.iAddress + KHoGpTimer_TISTAT) & 
    			KHtGpTimer_TISTAT_ResetComplete), Kern::Fault("D_Latncy: failed to reset timer.",
													 iGPTimerId));
	
    // Set PRE to be 0, PTV value is ignored, AutoReload is enabled
    TOmap::SetRegister32(iTimerInfo.iAddress + KHoGpTimer_TCLR, KHtGpTimer_TCLR_AR );

	//PTV argument is 0 because of TCLR[PRE] = 0 (prescaling disabled)
	TInt timerPTV = 0;
	
	// Calculate clock frequence from the ticks per ms
	TInt timerClkSrcFreq = KTicksPerMillisecond * 1000;
    
    iTimerLoadValue = OmapTimerMgr::TimerLoadValue(/*microsecs*/1000, timerClkSrcFreq, timerPTV);                          

	// First, load value in TCRR and TLDR registers
    TOmap::SetRegister32(iTimerInfo.iAddress + KHoGpTimer_TCRR, iTimerLoadValue);
    TOmap::SetRegister32(iTimerInfo.iAddress + KHoGpTimer_TLDR, iTimerLoadValue);

    // Enable overflow interrupt
    TOmap::SetRegister32(iTimerInfo.iAddress + KHoGpTimer_TIER,
                         KHtGpTimer_TIER_OverFlow);

    return KErrNone;
	}
#else
TInt DLatency::StartTimer()
	{
	iMsCallBack.OneShot(KTickPeriodMs*KFudgeFactor);
	return KErrNone;
	}
#endif

#if defined(__MISA__) 
// For SA1100/SA1110 use a separate timer on a FIQ interrupt (OST match 0)
void DLatency::StopTimer()
	{
	TSa1100::ModifyIntLevels(KHtIntsOstMatchGeneral,0);
	TSa1100::DisableOstInterrupt(KHwOstMatchGeneral);
	Interrupt::Disable(KIntIdOstMatchGeneral);
	Interrupt::Unbind(KIntIdOstMatchGeneral);
	TSa1100::SetOstMatchEOI(KHwOstMatchGeneral);
	}
#elif defined(__MCOT__)
// For Cotulla use a separate timer on a FIQ interrupt (OST match 0)
void DLatency::StopTimer()
	{
	TCotulla::ModifyIntLevels(KHtIntsOstMatchGeneral,0);
	TCotulla::DisableOstInterrupt(KHwOstMatchGeneral);
	Interrupt::Disable(KIntIdOstMatchGeneral);
	Interrupt::Unbind(KIntIdOstMatchGeneral);
	TCotulla::SetOstMatchEOI(KHwOstMatchGeneral);
	}
#elif defined(__IS_OMAP2420__) || defined(__WAKEUP_3430__)
void DLatency::StopTimer()
	{
	Interrupt::Disable(iTimerInfo.iInterruptId);
	Interrupt::Unbind(iTimerInfo.iInterruptId);
	DisableTimer();
	}
#else
void DLatency::StopTimer()
	{
	iMsCallBack.Cancel();
	}
#endif

TInt DLatency::Request(TInt aFunction, TAny* a1, TAny* a2)
//
// Client requests
//
	{
	// Kern::Printf("DLatency::Request() 0x%x)\n", aFunction);
	TInt r=KErrNone;
	switch (aFunction)
		{
		case RLatency::EControlStart:
			iStarted = (TUint8)ETrue;
			StartTimer();
			break;
		case RLatency::EControlTicksPerMs:
			r=KTicksPerMillisecond;
			break;
		case RLatency::EControlGetResults:
			iResults.iUserThreadTicks = Ticks();
			kumemput32(a1, &iResults, sizeof(SLatencyResults));
			break;
		default:
			r = KErrNotSupported;
			break;
		}
	return(r);
	}

#ifdef __CAPTURE_EXTRAS
extern void CaptureExtras(SLatencyResults&);
#endif

#if !defined(__MISA__) && !defined(__MCOT__)
void DLatency::MsCallBack(TAny* aPtr)
	{
	DLatency* pL = (DLatency*)aPtr;
#if defined(__IS_OMAP2420__) || defined(__WAKEUP_3430__)
	pL->iResults.iIntTicks = pL->Ticks();
	TOmap::SetRegister32(pL->iTimerInfo.iAddress + KHoGpTimer_TISR, KHtGpTimer_TISR_OverFlow);
#else
	pL->iResults.iIntTicks = Ticks();
#endif
#ifdef __CAPTURE_EXTRAS
	CaptureExtras(pL->iResults);
#endif
#if defined(__EPOC32__) && defined(__CPU_X86)
	pL->iResults.iIntRetAddr = X86::IrqReturnAddress();
#elif defined(__CPU_ARM) && defined(__SMP__)
	pL->iResults.iIntRetAddr = Arm::IrqReturnAddress();
#else
	pL->iResults.iIntRetAddr=(pL->iIntStackTop)[-1];
#endif
	if (!pL->iOff)
		{
		pL->iMsCallBack.Again(KTickPeriodMs*KFudgeFactor);
		pL->iMsDfc.Add();
		}
	}
#endif

void DLatency::MsDfc(TAny* aPtr)
	{
	DLatency* pL = (DLatency*)aPtr;
	pL->iResults.iKernThreadTicks=pL->Ticks();
	NKern::ThreadRequestSignal(&pL->iClient->iNThread);
	}

DLatencyPowerHandler::DLatencyPowerHandler(DLatency* aChannel)
	:	DPowerHandler(KLddName), 
		iChannel(aChannel)
	{
	}

void DLatencyPowerHandler::PowerUp()
	{
	iChannel->iOff = (TUint8)EFalse;
	if (iChannel->iStarted)
		iChannel->StartTimer();
	PowerUpDone();
	}

void DLatencyPowerHandler::PowerDown(TPowerState)
	{
	iChannel->iOff = (TUint8)ETrue;
	iChannel->StopTimer();
	PowerDownDone();
	}



