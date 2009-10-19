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
// e32\euser\epoc\x86\uc_exe.cpp
// 
//

#include "u32std.h"
#include <u32exec.h>
#include <e32svr.h>

// include the static data definitions
#define __FLTUSED
#include "win32crt.h"
#include "nwdl.h"


GLREF_C TInt E32Main();

extern "C" {

void globalDestructorFunc()
	{
	destroyStatics(); // this is a macro
	}

void __fastcall RunThread(TBool aNotFirst, SThreadCreateInfo& aInfo)
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
			constructStatics();
			r = E32Main();
			}
		}
	User::Exit(r);
	}

void _fltused() {}	
}
