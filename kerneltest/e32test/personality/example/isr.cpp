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
// e32test\personality\example\isr.cpp
// Test code for example RTOS personality.
// 
//

#include "plat_priv.h"
#include <personality/example/personality.h>

#if defined(__MAWD__)
#include <windermere.h>
#elif defined(__MISA__)
#define __ISR_SUPPORTED__
#include <sa1100.h>
#elif defined(__MCOT__)
#define __ISR_SUPPORTED__
#include <cotulla.h>
#elif defined(__MI920__) || defined(__NI1136__)
#include <integratorap.h>
#elif defined(__EPOC32__) && defined(__CPU_X86)
#include <x86.h>
#endif

#ifdef __ISR_SUPPORTED__
#include "../../misc/prbs.h"
#endif

extern "C" void stop_random_isr(void);

typedef void (*isr_entry)(unsigned);

volatile TUint RandomSeed[2] = {0xb504f333u, 0xf9de6484u};
volatile isr_entry IsrVector = 0;
volatile TUint IsrCount = 0;

void timer_isr(void*)
	{
#if defined(__MISA__) 
	TUint interval = Random((TUint*)RandomSeed);
	interval &= 0x3ff;
	interval += 256;	// 256-1279 ticks = approx 69 to 347 microseconds
	TUint oscr=TSa1100::OstData();
	TSa1100::SetOstMatch(KHwOstMatchGeneral, oscr + interval);
	TSa1100::SetOstMatchEOI(KHwOstMatchGeneral);
#elif defined(__MCOT__)
	TUint interval = Random((TUint*)RandomSeed);
	interval &= 0x3ff;
	interval += 256;	// 256-1279 ticks = approx 69 to 347 microseconds
	TUint oscr=TCotulla::OstData();
	TCotulla::SetOstMatch(KHwOstMatchGeneral, oscr + interval);
	TCotulla::SetOstMatchEOI(KHwOstMatchGeneral);
#endif
	(*IsrVector)(IsrCount++);
	}

#ifdef __ISR_SUPPORTED__
void start_timer(void)
	{
#if defined(__MISA__) 
	// for SA11x0 use OST match 0
	TInt r=Interrupt::Bind(KIntIdOstMatchGeneral, &timer_isr, 0);
	assert(r==KErrNone);
	TSa1100::ModifyIntLevels(0,KHtIntsOstMatchGeneral);	// route new timer interrupt to FIQ
	TSa1100::SetOstMatchEOI(KHwOstMatchGeneral);
	TUint oscr=TSa1100::OstData();
	TSa1100::SetOstMatch(KHwOstMatchGeneral, oscr + 5000);
	TSa1100::EnableOstInterrupt(KHwOstMatchGeneral);
	Interrupt::Enable(KIntIdOstMatchGeneral);
#elif defined(__MCOT__)
	// for SA11x0 use OST match 0
	TInt r=Interrupt::Bind(KIntIdOstMatchGeneral, &timer_isr, 0);
	assert(r==KErrNone);
	TCotulla::ModifyIntLevels(0,KHtIntsOstMatchGeneral);	// route new timer interrupt to FIQ
	TCotulla::SetOstMatchEOI(KHwOstMatchGeneral);
	TUint oscr=TCotulla::OstData();
	TCotulla::SetOstMatch(KHwOstMatchGeneral, oscr + 5000);
	TCotulla::EnableOstInterrupt(KHwOstMatchGeneral);
	Interrupt::Enable(KIntIdOstMatchGeneral);
#endif
	}
#endif

extern "C" int start_random_isr(isr_entry vector)
	{
	stop_random_isr();
	IsrVector = vector;
#ifdef __ISR_SUPPORTED__
	start_timer();
	return OK;
#else
	return KErrNotSupported;
#endif
	}

extern "C" void stop_random_isr(void)
	{
#if defined(__MISA__) 
	Interrupt::Disable(KIntIdOstMatchGeneral);
	Interrupt::Unbind(KIntIdOstMatchGeneral);
#elif defined(__MCOT__)
	Interrupt::Disable(KIntIdOstMatchGeneral);
	Interrupt::Unbind(KIntIdOstMatchGeneral);
#endif
	IsrVector = 0;
	}

