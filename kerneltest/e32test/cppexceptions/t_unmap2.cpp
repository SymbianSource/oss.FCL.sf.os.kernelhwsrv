// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\cppexceptions\t_unmap2.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include "d_unmap.h"

_LIT(KTestName, "t_unmap2");
RTest test(KTestName);
RSemaphore WaitSemaphore;
RSemaphore SignalSemaphore;

void TestLibraryCleanup();

TInt E32Main()
   	{
	test.Start(_L("Check code seg unmapping over User::Leave()/C++ exceptions."));

	__UHEAP_MARK;
   	CTrapCleanup* cleanup = CTrapCleanup::New();
   	TInt r = KErrNoMemory;
   	if (cleanup)
   		{
		test.Printf(_L("Opening semaphores for barrier with other process\n"));
		test(WaitSemaphore.Open(1)==KErrNone);
		test(SignalSemaphore.Open(2)==KErrNone);
		TRAP(r, TestLibraryCleanup());
		test.Printf(_L("Returned %d, expected %d\n"), r, KErrGeneral);
		test(r == KErrGeneral);
		WaitSemaphore.Close();
		SignalSemaphore.Close();
		r = KErrNone;
   		}

	delete cleanup;
 	__UHEAP_MARKEND;

	test.End();
   	return r;
   	}

void Checkpoint(TAny*)
	{
	test.Printf(_L("Checkpointing...\n"));
	SignalSemaphore.Signal();
	WaitSemaphore.Wait();
	}

void TestLibraryCleanup()
	{
    test.Printf(_L("Loading DLL.\n"));
	RLibrary library;
	test(library.Load(KLeavingDll2) == KErrNone);

    test.Printf(_L("Pushing cleanup item to synchronise after closing library handle.\n"));
	CleanupStack::PushL(TCleanupItem(&Checkpoint, NULL));

    test.Printf(_L("Pushing handle to dynamically loaded DLL onto the cleanup stack.\n"));
	CleanupClosePushL(library);

	test.Printf(_L("Pushing cleanup item to synchronise during leave\n"));
	CleanupStack::PushL(TCleanupItem(&Checkpoint, NULL));

    test.Printf(_L("Looking up leaving function.\n"));
	TLibraryFunction leaving = library.Lookup(1);
	test(leaving != NULL);

	test.Printf(_L("Check-point whilst holding the open library handle.\n"));
	Checkpoint(NULL);

	test.Printf(_L("Calling leaving function.\n"));
	(*leaving)();

	test.Printf(_L("Cleaning up checkpoint operation.\n"));
	CleanupStack::Pop(); // pause leave op

	test.Printf(_L("Cleaning up DLL handle.\n"));
	CleanupStack::PopAndDestroy();

	test.Printf(_L("Cleaning up checkpoint operation.\n"));
	CleanupStack::Pop(); // pause leave op
	}
