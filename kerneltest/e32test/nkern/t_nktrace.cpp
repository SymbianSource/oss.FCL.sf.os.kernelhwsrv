// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\nkern\t_nktrace.cpp
// Overview:
// Test the KTRACE macros and debug mask related functions.
// API Information:
// UserSvr::DebugMask(), User::SetDebugMask, KDebugMask(), KDebugNum()
// Details:	
// Set debug masks and verify they are as expected. Test using 
// UserSvr::DebugMask() & User::SetDebugMask().
// In a kernel level LDD, test the kernel functions using 
// KDebugMask() & KDebugNum().
// 
//

#include <e32test.h>
#include <e32svr.h>
#include "nk_trace.h"
#include "d_nktrace.h"

LOCAL_D RTest test(_L("T_NKTRACE"));

void Wait(TInt aSeconds)
	{
	while (aSeconds>0)
		{
		--aSeconds;
		User::After(1000000);
		test.Printf(_L("."));
		}
	}

GLDEF_C TInt E32Main()
    {
	test.Title();

	test.Start(_L("Load LDD"));
	TInt r=User::LoadLogicalDevice(_L("D_NKTRACE"));
	test(r==KErrNone || r==KErrAlreadyExists);
	test.Next(_L("Open channel"));
	RNKTraceTest traceSys;
	r=traceSys.Open();
	test(r==KErrNone);

	// verify access to debug mask 0, with and without argument
	TUint32 initial[8];
	TInt i;
	
	for (i=0; i<8; i++)
		   initial[i]= UserSvr::DebugMask(i);

	test.Printf(_L("initial DebugMask value = 0x%08x\n"), initial[0]);
	TUint32 m = UserSvr::DebugMask();
	test(m==initial[0]);
	User::SetDebugMask(~m);
	m = UserSvr::DebugMask(0);
	test(m!=initial[0]);
	test(m==~initial[0]);

	// set and verify each of the 8 debug masks
	User::SetDebugMask(0x12340000, 0);
	User::SetDebugMask(0x12341111, 1);
	User::SetDebugMask(0x12342222, 2);
	User::SetDebugMask(0x12343333, 3);
	User::SetDebugMask(0x12344444, 4);
	User::SetDebugMask(0x12345555, 5);
	User::SetDebugMask(0x12346666, 6);
	User::SetDebugMask(0x12347777, 7);

	m = UserSvr::DebugMask(0);
	test(m==0x12340000);
	m = UserSvr::DebugMask(1);
	test(m==0x12341111);
	m = UserSvr::DebugMask(2);
	test(m==0x12342222);
	m = UserSvr::DebugMask(3);
	test(m==0x12343333);
	m = UserSvr::DebugMask(4);
	test(m==0x12344444);
	m = UserSvr::DebugMask(5);
	test(m==0x12345555);
	m = UserSvr::DebugMask(6);
	test(m==0x12346666);
	m = UserSvr::DebugMask(7);
	test(m==0x12347777);

	// verify correct results when no index argument is given
	User::SetDebugMask(0xC0000000);
	m = UserSvr::DebugMask();
	test.Printf(_L("UserSvr::DebugMask = 0x%08x\n"), m);
	test(m==0xC0000000);

	User::SetDebugMask(0x80000000);
	m = UserSvr::DebugMask();
	test.Printf(_L("UserSvr::DebugMask = 0x%08x\n"), m);
	test(m==0x80000000);

	test.Next(_L("Test the Kernel level code..."));
	User::SetDebugMask(0xC0000000, 0);    // set bits 30 & 31
	User::SetDebugMask(0x00000008, 1);    // set bit 35
	User::SetDebugMask(0x00000010, 2);    // set bit 68
	User::SetDebugMask(0x00000020, 3);    // set bit 101
	User::SetDebugMask(0x00000100, 4);    // set bit 136
	User::SetDebugMask(0x00001000, 5);    // set bit 172
	User::SetDebugMask(0x00000001, 6);    // set bit 192
	User::SetDebugMask(0x00000040, 7);    // set bit 230

	// test the KTRACE_OPT macros in server code
	r=traceSys.KTrace(RNKTraceTest::ETestKTrace);

	// test the KDebugMask() and KDebugNum() functions in server code
	r=traceSys.KDebug(RNKTraceTest::ETestKTrace);

    test.Printf(_L("Kern::Printf() output goes\n"));
	test.Printf(_L("to the serial port or the \n"));
	test.Printf(_L("file epocwind.out when \n"));
	test.Printf(_L("tested on the emulator. \n"));
    test.Printf(_L("The following results\n"));
    test.Printf(_L("should be displayed:\n"));

   	test.Printf(_L("Test __KTRACE_OPT macros\n"));
    test.Printf(_L("KALWAYS\n"));
    test.Printf(_L("KPANIC\n"));
    test.Printf(_L("KSCRATCH\n"));
    test.Printf(_L("Debug bit 35 is set\n"));
    test.Printf(_L("Debug bit 68 is set\n"));
    test.Printf(_L("Debug bit 101 is set\n"));
    test.Printf(_L("Debug bit 136 is set\n"));
    test.Printf(_L("Debug bit 172 is set\n"));
    test.Printf(_L("Debug bit 192 is set\n"));
    test.Printf(_L("Debug bit 230 is set\n"));    
    test.Printf(_L("KTRACE_ALL returned true\n"));    

	// Wait for a while, or for a key press
	test.Printf(_L("Press a key to continue..."));
	TRequestStatus keyStat;
	test.Console()->Read(keyStat);
	RTimer timer;
	test(timer.CreateLocal()==KErrNone);
	TRequestStatus timerStat;
	timer.After(timerStat,10*1000000);
	User::WaitForRequest(timerStat,keyStat);
	(void)test.Console()->KeyCode();
	timer.Cancel();
	test.Console()->ReadCancel();
	User::WaitForAnyRequest();

	test.Printf(_L("\n"));
    test.Printf(_L("KDebug tests (0)\n"));
    test.Printf(_L("KDebugMask() = 0xc0000000\n"));
    test.Printf(_L("KDebugNum(30) = 1\n"));
    test.Printf(_L("KDebugNum(31) = 1\n"));
    test.Printf(_L("KDebugNum(3) = 0\n"));
    test.Printf(_L("KDebugNum(9) = 0\n"));
    test.Printf(_L("KDebugNum(10000) = 0\n"));
    test.Printf(_L("KDebugNum(-1) = 1\n"));
    test.Printf(_L("KDebugNum(-2) = 0\n"));
    test.Printf(_L("KDebugNum(35) = 1\n"));
    test.Printf(_L("KDebugNum(36) = 0\n"));
    test.Printf(_L("KDebugNum(101) = 1\n"));
    test.Printf(_L("KDebugNum(192) = 1\n"));
    test.Printf(_L("KDebugNum(230) = 1\n"));

	// set the debug masks back to the original state
	for (i=0; i<8; i++)
		User::SetDebugMask(initial[i], i);

	m = UserSvr::DebugMask(0);
	test(m==initial[0]);
	m = UserSvr::DebugMask(1);
	test(m==initial[1]);
	m = UserSvr::DebugMask(2);
	test(m==initial[2]);
	m = UserSvr::DebugMask(3);
	test(m==initial[3]);
	m = UserSvr::DebugMask(4);
	test(m==initial[4]);
	m = UserSvr::DebugMask(5);
	test(m==initial[5]);
	m = UserSvr::DebugMask(6);
	test(m==initial[6]);
	m = UserSvr::DebugMask(7);
	test(m==initial[7]);

	test.End();

	return(0);
    }

