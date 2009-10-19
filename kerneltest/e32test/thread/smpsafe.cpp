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

TInt Affinity;

TInt AffinitySlave(TAny*)
	{
	for (;;)
		{
		__e32_atomic_store_rel32(&Affinity, 0); // we can't be locked if this runs
		User::AfterHighRes(1);
		}
	}

TInt CheckAffinity()
	{
	__e32_atomic_store_rel32(&Affinity, 1); // assume we are locked to a single cpu

	RThread t;
	TInt r = t.Create(_L("AffinitySlave"), AffinitySlave, KDefaultStackSize, NULL, NULL);
	if (r != KErrNone)
		return r;

	TRequestStatus s;
	t.Logon(s);
	t.SetPriority(EPriorityLess);
	t.Resume();

	TUint32 target = User::NTickCount() + 10;
	while (User::NTickCount() < target) {}

	r = __e32_atomic_load_acq32(&Affinity);

	t.Kill(0);
	User::WaitForRequest(s);
	t.Close();

	return r;
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
