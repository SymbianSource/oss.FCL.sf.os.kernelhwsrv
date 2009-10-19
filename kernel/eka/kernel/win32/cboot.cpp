// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\win32\cboot.cpp
// 
//

#include "memmodel.h"
#include <emulator.h>

extern "C" void KernelMain();

GLDEF_D TSuperPage SuperPage;

GLDEF_C void BootEpoc()
//
// The emulator boot code
// This is invoked after EKERN static data has been initialised
//
// Need to set up the super page before entering the main kernel startup
//
	{
	const static Emulator::SInit data = {0, &SchedulerLock, &SchedulerUnlock, &SchedulerEscape, &SchedulerReenter, &NThread::ExceptionHandler};
	Emulator::Init(data);
	SuperPageAddress = (TLinAddr)&SuperPage;
//	TheSuperPage().iCpuId = 0;	automatic since TheSuperPage is in .bss
	KernelMain();
	}


