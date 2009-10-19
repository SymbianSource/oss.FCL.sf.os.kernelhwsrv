// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// template\template_assp\assp.cpp
// 
//

#include <template_assp_priv.h>

TemplateAssp* TemplateAssp::Variant=NULL;
TPhysAddr TemplateAssp::VideoRamPhys;

DECLARE_STANDARD_ASSP()

EXPORT_C TemplateAssp::TemplateAssp()
	{
	TemplateAssp::Variant=this;
	}

extern void MsTimerTick(TAny* aPtr);

EXPORT_C TMachineStartupType TemplateAssp::StartupReason()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("TemplateAssp::StartupReason"));
#ifdef _DEBUG															// REMOVE THIS
	TUint s = Kern::SuperPage().iHwStartupReason;
	__KTRACE_OPT(KBOOT,Kern::Printf("CPU page value %08x", s));
#endif																	// REMOVE THIS
	//
	// TO DO: (mandatory)
	//
	// Map the startup reason read from the Super Page to one of TMachineStartupType enumerated values
	// and return this
	//
	return EStartupCold;   // EXAMPLE ONLY
	}

EXPORT_C void TemplateAssp::Init1()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("TemplateAssp::Init1()"));
	//
	// TO DO: (optional)
	//
	TemplateInterrupt::Init1();			// initialise the ASSP interrupt controller

	//
	// TO DO: (optional)
	//
	// Initialises any hardware blocks which require early initialisation, e.g. enable and power the LCD, set up
	// RTC clocks, disable DMA controllers. etc.
	//
	}

EXPORT_C void TemplateAssp::Init3()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("TemplateAssp::Init3()"));

	TTemplate::Init3();

	NTimerQ& m=*(NTimerQ*)NTimerQ::TimerAddress();
	iTimerQ=&m;
	//
	// TO DO: (mandatory)
	//
	// If Hardware Timer used for System Ticks cannot give exactly the period required store the initial rounding value
	// here which is updated every time a match occurrs. Note this leads to "wobbly" timers whose exact period change
	// but averages exactly the required value
	// e.g.
	// m.iRounding=-5;
	//
	
	TInt r=Interrupt::Bind(KIntIdOstMatchMsTimer,MsTimerTick,&m);	// bind the System Tick interrupt
	if (r!=KErrNone)
		Kern::Fault("BindMsTick",r);

	// 
	// TO DO: (mandatory)
	//
	// Clear any pending OST interrupts and enable any OST match registers.
	// If possible may reset the OST here (to start counting from a full period). Set the harwdare to produce an 
	// interrupt on full count
	//

	r=Interrupt::Enable(KIntIdOstMatchMsTimer);	// enable the System Tick interrupt
	if (r!=KErrNone)
		Kern::Fault("EnbMsTick",r);

	// 
	// TO DO: (optional)
	//
	// Allocate physical RAM for video buffer, as per example below. However with some hardware, the Video Buffer
	// may not reside in main System memory, it may be dedicated memory.
	//
	// EXAMPLE ONLY
	TInt vSize=VideoRamSize();
	r=Epoc::AllocPhysicalRam(2*vSize,TemplateAssp::VideoRamPhys);
	if (r!=KErrNone)
		Kern::Fault("AllocVRam",r);
	}

EXPORT_C TInt TemplateAssp::MsTickPeriod()
	{
	// 
	// TO DO: (mandatory)
	//
	// Return the OST tick period (System Tick) in microseconds ( 10E-06 s ).
	//
	return 1000;   // EXAMPLE ONLY
	}

EXPORT_C TInt TemplateAssp::SystemTimeInSecondsFrom2000(TInt& aTime)
	{
	aTime=(TInt)TTemplate::RtcData();
	__KTRACE_OPT(KHARDWARE,Kern::Printf("RTC READ: %d",aTime));
	return KErrNone;
	}

EXPORT_C TInt TemplateAssp::SetSystemTimeInSecondsFrom2000(TInt aTime)
	{
	//
	// TO DO: (optional)
	//
	// Check if the RTC is running and is stable
	//
	__KTRACE_OPT(KHARDWARE,Kern::Printf("Set RTC: %d",aTime));
	TTemplate::SetRtcData(aTime);
	__KTRACE_OPT(KHARDWARE,Kern::Printf("RTC: %d",TTemplate::RtcData()));
	return KErrNone;
	}

EXPORT_C TUint32 TemplateAssp::NanoWaitCalibration()
	{
	// 
	// TO DO: (mandatory)
	//
	// Return the minimum time in nano-seconds that it takes to execute the following code:
	//	 nanowait_loop:
	//	 		  subs r0, r0, r1
	//	 		  bhi nanowait_loop
	//
	// If accurate timings are required by the Base Port, then it should provide it's own implementation 
	// of NanoWait which uses a hardware counter. (See Kern::SetNanoWaitHandler)
	//
	
	return 0;   // EXAMPLE ONLY
	}

