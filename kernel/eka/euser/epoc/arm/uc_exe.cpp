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
// e32\euser\epoc\arm\uc_exe.cpp
// 
//

#include "u32std.h"
#include <u32exec.h>
#include <e32svr.h>

GLREF_C TInt E32Main();

extern "C" {

#if defined(__GCC32__)
typedef void (*PFV)();
extern PFV __CTOR_LIST__[];
extern PFV __DTOR_LIST__[];

void globalDestructorFunc()
	{
	TUint i=1;
	while (__DTOR_LIST__[i])
		(*__DTOR_LIST__[i++])();
	}

void RunThread(TBool aNotFirst, SThreadCreateInfo& aInfo)
	{
	SStdEpocThreadCreateInfo& cinfo = (SStdEpocThreadCreateInfo&)aInfo;

#ifdef USE_INSTRUMENTED_HEAP
	cinfo.iFlags |= ETraceHeapAllocs;
#elif defined(ENABLE_HEAP_MONITORING)
	cinfo.iFlags |= ETraceHeapAllocs|EMonitorHeapMemory;
#endif
	TInt r = UserHeap::SetupThreadHeap(aNotFirst, cinfo);

	if (r==KErrNone)
		r = UserSvr::DllSetTls(KGlobalDestructorTlsKey, KDllUid_Special, (TAny*)globalDestructorFunc);

	if (r==KErrNone)
		{
		if (aNotFirst)
			r = (*cinfo.iFunction)(cinfo.iPtr);
		else
			{
			// Init statics for implicitly linked DLLs
			User::InitProcess();

			// Init statics for EXE
			TUint i=1;
			while (__CTOR_LIST__[i])
				(*__CTOR_LIST__[i++])();

			r = E32Main();
			}
		}
	User::Exit(r);
	}
}

#elif defined(__ARMCC__)

TInt CallThrdProcEntry(TInt (*aFn)(void*), void* aPtr, TInt aNotFirst);

__weak void run_static_dtors(void);

void globalDestructorFunc()
	{
	int call_static_dtors = (int)run_static_dtors;
	if (call_static_dtors)
		run_static_dtors();
	}

void RunThread(TBool aNotFirst, SThreadCreateInfo& aInfo)
	{
	SStdEpocThreadCreateInfo& cinfo = (SStdEpocThreadCreateInfo&)aInfo;

#ifdef USE_INSTRUMENTED_HEAP
	cinfo.iFlags |= ETraceHeapAllocs;
#elif defined(ENABLE_HEAP_MONITORING)
	cinfo.iFlags |= ETraceHeapAllocs|EMonitorHeapMemory;
#endif
	TInt r = UserHeap::SetupThreadHeap(aNotFirst, cinfo);

	if (r==KErrNone)
		r = UserSvr::DllSetTls(KGlobalDestructorTlsKey, KDllUid_Special, (TAny*)globalDestructorFunc);

	if (r==KErrNone)
		r = CallThrdProcEntry(cinfo.iFunction, cinfo.iPtr, aNotFirst);

	User::Exit(r);
	}
}

#else
#error not supported
#endif
