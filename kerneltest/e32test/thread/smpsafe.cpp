// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\thread\smpsafe.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32atomics.h>

IMPORT_C void DoNothingA();
IMPORT_C void DoNothingB();
IMPORT_C void DoNothingC();
IMPORT_C void DoNothingD();
IMPORT_C void DoNothingE();

#ifdef MAKE_DLL

#ifdef PROVIDE_A
EXPORT_C void DoNothingA() {}
#endif

#ifdef PROVIDE_B
EXPORT_C void DoNothingB() { DoNothingA(); }
#endif

#ifdef PROVIDE_C
EXPORT_C void DoNothingC() { DoNothingD(); DoNothingE(); }
#endif

#ifdef PROVIDE_D
EXPORT_C void DoNothingD() { DoNothingC(); }
#endif

#ifdef PROVIDE_E
EXPORT_C void DoNothingE() { DoNothingC(); }
#endif

#else // !MAKE_DLL

volatile TInt Affinity;
RSemaphore Start;
RSemaphore Stop;

const TInt KLoopTries = 100;

// This gets run in a low priority thread. Each time around the loop it waits to be told to go,
// then sets Affinity to 0, then tells the other thread it's done. If we're actually locked to
// the same processor as the main thread, however, then we won't get to run until the other thread
// waits for the Stop semaphore, and thus Affinity will not get set to 0 until the other thread
// checked it already.
TInt AffinitySlave(TAny*)
	{
	for (TInt i = KLoopTries; i>0; --i)
		{
		Start.Wait();
		Affinity = 0;
		Stop.Signal();
		}
	return KErrNone;
	}

TInt CheckAffinity()
	{
	RThread t;
	TInt r = t.Create(_L("AffinitySlave"), AffinitySlave, KDefaultStackSize, NULL, NULL);
	if (r != KErrNone)
		return r;

	Start.CreateLocal(0);
	Stop.CreateLocal(0);

	TRequestStatus s;
	t.Logon(s);
	t.SetPriority(EPriorityLess);
	t.Resume();

	TInt a = 1;
	for (TInt i = KLoopTries; i>0; --i)
		{
		Affinity = 1; // assume we are locked to a single cpu
		Start.Signal(); // tell the other thread to run
		TUint32 target = User::NTickCount() + 10;
		while (User::NTickCount() < target)
			{
			// spin, waiting to see if the other thread actually *does* run
			}
		a = Affinity;
		if (a == 0)
			break;
		Stop.Wait(); // We didn't see it this time, but try again in case of scheduling fluke
		}

	t.Kill(0);
	User::WaitForRequest(s);
	t.Close();

	return a;
	}

#ifndef OMIT_MAIN
GLDEF_C TInt E32Main()
	{
#ifdef USE_A
	DoNothingA();
#endif
#ifdef USE_B
	DoNothingB();
#endif
	return CheckAffinity();
	}
#endif // OMIT_MAIN

#endif
