// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\t_nanowait.cpp
// Overview:
// Test Kernal::NanoWait() against the FastCounter.
// API Information:
// NTimer
// Details:
// - Start one 100 msec NanoWait delay and verify that the 
// delay time is correct.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32uid.h>
#include <e32hal.h>
#include <hal.h>
#include "d_nanowait.h"

RTest test(_L("t_nanowait"));
RNanoWait nanowait;

TBool PauseOnError = 0;
#define GETCH()		(PauseOnError&&test.Getch())

#define TEST(c)		((void)((c)||(test.Printf(_L("Failed at line %d\n"),__LINE__),GETCH(),test(0),0)))
#define CHECK(c)	((void)(((c)==0)||(test.Printf(_L("Error %d at line %d\n"),(c),__LINE__),GETCH(),test(0),0)))
#ifdef __WINS__
#define TESTTIME(v,min,max) test.Printf(_L("Expected range [%d,%d]\n"),min,max+1)
#else
#define TESTTIME(v,min,max) TEST(v>=min && v<max)
#endif

const TPtrC KLddFileName=_L("D_NANOWAIT.LDD");

GLDEF_C TInt E32Main()
//
// Test NanoWait() delays
//
    {

    
    // To use in command line
    	TBool pause = EFalse;
    TBuf<256> cmdline;
	User::CommandLine(cmdline);
	if( cmdline == _L("-p"))
		{
			pause = ETrue;			
		}

    
	test.Title();

	test.Start(_L("Load D_NanoWait.LDD"));
	TInt r=User::LoadLogicalDevice(KLddFileName);
	TEST(r==KErrNone || r==KErrAlreadyExists);
	
	test.Next(_L("Test NanoWait delay"));
	r=nanowait.Open();
	CHECK(r);

	TInt requested = 100000000;  // nsec = 100 msec
	TUint32 startTick;
	TUint32 endTick;
	
	TInt nanokernel_tick_period;
	HAL::Get(HAL::ENanoTickPeriod, nanokernel_tick_period );
	nanokernel_tick_period *= 1000; 	// convert from micro sec to nsec

	
	test.Printf(_L("Test parameters: total requested:%d nsec\n"), requested);
	
	startTick = User::NTickCount();
	r=nanowait.StartNanoWait(1, requested);
	endTick = User::NTickCount();
	
	startTick *= nanokernel_tick_period;	// in nsec
	endTick   *= nanokernel_tick_period;	// in nsec
	
	TInt measured  = endTick-startTick;
	TInt diff = measured - requested;
	test.Printf(_L("Requested delay:%10d nsec\n"), requested);
	test.Printf(_L("Measured delay :%10d nsec\n"), measured);
	test.Printf(_L("Difference     :%10d nsec\n"), diff);
	
	
	test.End();
	if( pause )
		{
		test.Printf(_L("Press any key\n"));
		test.Getch();	
		}
	
	return(KErrNone);
    }

