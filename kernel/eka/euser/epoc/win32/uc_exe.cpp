// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\epoc\win32\uc_exe.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32wins.h>
#include <u32exec.h>
#include <e32svr.h>

#ifdef __VC32__
#define __FLTUSED
#endif //__VC32__

// include the static data definitions
#include "win32crt.h"

// include compiler helpers
#include "x86hlp.inl"

GLREF_C TInt E32Main();

#ifdef __CW32__
TBool InitCWRuntime();
TBool CleanupCWRuntime();
#endif

void globalDestructorFunc()
	{
#ifdef __CW32__
	CleanupCWRuntime();
#else
	destroyStatics(); // this is a macro
#endif
	}

extern "C"
EXPORT_C TInt _E32Startup(TInt aReason, TAny* aInfo)
//
// Ordinal 1 - the EPOC executable entrypoint
//
	{
	if (TUint(aReason)<=TUint(KModuleEntryReasonThreadInit))
		{
		SStdEpocThreadCreateInfo& cinfo = *(SStdEpocThreadCreateInfo*)aInfo;

#ifdef USE_INSTRUMENTED_HEAP
		cinfo.iFlags |= ETraceHeapAllocs;
#elif defined(ENABLE_HEAP_MONITORING)
		cinfo.iFlags |= ETraceHeapAllocs|EMonitorHeapMemory;
#endif
		TInt r = UserHeap::SetupThreadHeap( (aReason!=KModuleEntryReasonProcessInit), cinfo);

		if (r==KErrNone)
			r = UserSvr::DllSetTls(KGlobalDestructorTlsKey, KDllUid_Special, (TAny*)globalDestructorFunc);

		if (r==KErrNone)
			{
			if (aReason==KModuleEntryReasonProcessInit)
				{
				// Init statics for implicitly linked DLLs
				User::InitProcess();

#ifdef __CW32__
				// Ensure runtime library gets initialised
				r = InitCWRuntime() ? KErrNone : KErrGeneral;
#endif

				if (r==KErrNone)
					{
					// Init statics for EXE
					constructStatics();

					r = E32Main();
					}
				}
			else
				r = (*cinfo.iFunction)(cinfo.iPtr);
			}
		User::Exit(r);
		}
	if (aReason==KModuleEntryReasonException)
		{
		User::HandleException(aInfo);
		return 0;
		}
	User::Invariant();
	return 0;
	}

GLDEF_C void __stdcall _E32Bootstrap()
//
// Win32 entrypoint, Boot epoc with auto-run
//
	{
	BootEpoc(ETrue);
	}
