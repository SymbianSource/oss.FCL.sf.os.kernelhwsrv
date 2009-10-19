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
// e32test\debug\t_schedhook.cpp
// Overview:
// Test the device hook functionality.
// API Information:
// RBusLogicalChannel, DLogicalDevice
// Details:	
// - Load the specified logical driver dll and check the return value 
// is as expected. Create the logical channel for the current thread 
// and check the return value as KErrNone.
// - Install the scheduler hook and check it is installed successfully.
// - Enable scheduler callback, setup test threads, test the scheduler hook 
// before thread resume and after resume is as expected.
// - Perform some context switching and check rescheduler count is as expected.
// - Disable scheduler callback, perform some context switching and check 
// thread rescheduler count is as expected.
// - Re-enable scheduler callback, perform some context switching and check 
// thread rescheduler count is as expected.
// - Remove schedule hook and check it is successfully removed. perform some 
// context switching and check thread rescheduler count is as expected.
// - Check exception during context switching, user mode interrupt, WFAR, 
// exec call are as expected.
// - Uninstall scheduler hook and check it is as expected.
// Platforms/Drives/Compatibility:
// Hardware (Automatic).
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include "context.h"
#define __KERNEL_MODE__
#include "nk_priv.h"
#include <nk_plat.h>
#undef __KERNEL_MODE__

RTest test(_L("T_SCHEDHOOK"));

RSchedhookTest ldd;

RThread Thread1;
TRequestStatus Request1;



RThread Thread2;
TRequestStatus Request2;

TInt Thread2Main(TAny*)
	{
	for(;;)
		{
		Request2 = KRequestPending;
		TRequestStatus* request = &Request1;
		Thread1.RequestComplete(request,KErrNone);
		User::WaitForRequest(Request2);
		}
	}



RThread		ThreadException;
TArmRegSet	ThreadExceptionData;
RThread		ThreadWFAR;
TArmRegSet	ThreadWFARData;
RThread		ThreadUserInt;
TArmRegSet	ThreadUserIntData;
RThread		ThreadExecCall;
TArmRegSet	ThreadExecCallData;



TInt GetThreadContext(RThread aThread,TArmRegSet& aContext)
	{
	TPtr8 context((TUint8*)&aContext,sizeof(TArmRegSet),sizeof(TArmRegSet));
	return ldd.GetThreadContext(aThread.Id(),context);
	}



void DumpContext(TArmRegSet& aContext,TInt aType)
	{
	test.Printf(_L("  Context type %d\n"),aType);
	test.Printf(_L("  r0 =%08x r1 =%08x r2 =%08x r3 =%08x\n"),aContext.iR0,aContext.iR1,aContext.iR2,aContext.iR3);
	test.Printf(_L("  r4 =%08x r5 =%08x r6 =%08x r7 =%08x\n"),aContext.iR4,aContext.iR5,aContext.iR6,aContext.iR7);
	test.Printf(_L("  r8 =%08x r9 =%08x r10=%08x r11=%08x\n"),aContext.iR8,aContext.iR9,aContext.iR10,aContext.iR11);
	test.Printf(_L("  r12=%08x r13=%08x r14=%08x r15=%08x\n"),aContext.iR12,aContext.iR13,aContext.iR14,aContext.iR15);
	test.Printf(_L("  cpsr=%08x"),aContext.iFlags);
	}



void TestContext()
	{
	TInt r;
	TArmRegSet context;

	test.Start(_L("Insert scheduler hook"));
	r = ldd.InsertHooks();
	test(r==KErrNone);

	test.Next(_L("Enable scheduler callback"));
	r = ldd.EnableCallback();
	test(r==KErrNone);

	test.Next(_L("Test exception context"));
	r = ThreadException.Create(KNullDesC,ThreadContextHwExc,KDefaultStackSize,&User::Allocator(),&ThreadExceptionData);
	test(r==KErrNone);
	ThreadException.SetPriority(EPriorityMore);
	r = ldd.SetTestThread(ThreadException.Id());   // So ldd handles the exception for this thread
	test(r==KErrNone);
	ThreadException.Resume();
	User::After(250000);   // Let thread run
	r = GetThreadContext(ThreadException,context);
	DumpContext(context,r);
	test(r==NThread::EContextException);
	test(CheckContextHwExc(&context,&ThreadExceptionData));

	test.Next(_L("Test user mode interupt context"));
	r = ThreadUserInt.Create(KNullDesC,ThreadContextUserInt,KDefaultStackSize,&User::Allocator(),&ThreadUserIntData);
	test(r==KErrNone);
	ThreadUserInt.SetPriority(EPriorityLess);
	ThreadUserInt.Resume();
	User::After(250000);   // Let thread run
	r = GetThreadContext(ThreadUserInt,context);
	DumpContext(context,r);
	test(r==NThread::EContextUserInterrupt);
	test(CheckContextUserInt(&context,&ThreadUserIntData));

	test.Next(_L("Test WFAR context"));
	r = ThreadWFAR.Create(KNullDesC,ThreadContextWFAR,KDefaultStackSize,&User::Allocator(),&ThreadWFARData);
	test(r==KErrNone);
	ThreadWFAR.SetPriority(EPriorityMore);
	ThreadWFAR.Resume();
	User::After(250000);   // Let thread run
	r = GetThreadContext(ThreadWFAR,context);
	DumpContext(context,r);
	test(r==NThread::EContextWFAR);
	test(CheckContextWFAR(&context,&ThreadWFARData));

	test.Next(_L("Test exec call context"));
	r = ThreadExecCall.Create(KNullDesC,ThreadContextExecCall,KDefaultStackSize,&User::Allocator(),&ThreadExecCallData);
	test(r==KErrNone);
	ThreadExecCall.SetPriority(EPriorityMore);
	ThreadExecCall.Resume();
	User::After(250000);   // Let thread run
	r = GetThreadContext(ThreadExecCall,context);
	DumpContext(context,r);
	test(r==NThread::EContextExec);
	test(CheckContextExecCall(&context,&ThreadExecCallData));

	test.End();
	}


TInt DoContextSwitches(TInt aCount)
	{
	TInt r = ldd.SetTestThread(Thread2.Id());   // Zero test count
	test(r==KErrNone);
	while(aCount)
		{
		Request1 = KRequestPending;
		TRequestStatus* request = &Request2;
		Thread2.RequestComplete(request,KErrNone);
		User::WaitForRequest(Request1);
		--aCount;
		}
	r = ldd.GetTestCount();
	test(r>=0);
	return r;
	}



GLDEF_C TInt E32Main()
    {
	test.Title();
	TInt r;
	
	test.Start(_L("Loading LDD"));
	r = User::LoadLogicalDevice(_L("D_SCHEDHOOK"));
	test(r==KErrNone || r==KErrAlreadyExists);

	test.Next(_L("Open channel to LDD"));
	r = ldd.Open();
	test(r==KErrNone);

	test.Next(_L("Installing scheduler hooks"));
	r = ldd.Install();
	if (r==KErrNotSupported)
		{
		test.Next(_L("Scheduler hooks not supported on this platform, skipping test"));
		ldd.Close();
		test.End();
		return KErrNone;
		}
	test(r==KErrNone);

	test.Next(_L("Enable scheduler callback"));
	r = ldd.EnableCallback();
	test(r==KErrNone);

	test.Next(_L("Setting up test thread"));
	r= Thread1.Open(RThread().Id());
	test(r==KErrNone);
	r = Thread2.Create(KNullDesC,Thread2Main,KDefaultStackSize,&User::Allocator(),NULL);
	test(r==KErrNone);
	r = ldd.SetTestThread(Thread2.Id());
	test(r==KErrNone);

	test.Next(_L("Test scheduler hook (wait)"));
	User::After(1000000); // 1 second
	TInt count = ldd.GetTestCount();
	test.Printf(_L("count=%d\n"),count);
	test(count==0);

	test.Next(_L("Test scheduler hook (resume)"));
	Request1 = KRequestPending;
	Thread2.Resume();
	User::WaitForRequest(Request1);
	count = ldd.GetTestCount();
	test.Printf(_L("count=%d\n"),count);
	test(count>0);

	test.Next(_L("Test scheduler hook (context switching)"));
	count = DoContextSwitches(1000);
	test.Printf(_L("count=%d\n"),count);
	test(count>=1000);

	test.Next(_L("Disable scheduler callback"));
	r = ldd.DisableCallback();
	test(r==KErrNone);
	count = DoContextSwitches(1000);
	test.Printf(_L("count=%d\n"),count);
	test(count==0);

	test.Next(_L("Re-enable scheduler callback"));
	r = ldd.EnableCallback();
	test(r==KErrNone);
	count = DoContextSwitches(1000);
	test.Printf(_L("count=%d\n"),count);
	test(count>=1000);

	test.Next(_L("Removing scheduler hook"));
	r = ldd.RemoveHooks();
	test(r==KErrNone);
	count=DoContextSwitches(1000);
	r = ldd.GetTestCount();
	test.Printf(_L("count=%d\n"),r);
	test(r==0);

	test.Next(_L("Test thread context"));
	TestContext();

	test.Next(_L("Uninstalling scheduler hook"));
	r = ldd.Uninstall();
	test(r==KErrNone);

	test.Next(_L("Closing ldd"));
	ldd.Close();
	
	test.End();

	return(0);
    }

