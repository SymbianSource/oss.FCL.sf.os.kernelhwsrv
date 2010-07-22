// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfsrv\cl_main.cpp
// 
//

#include "cl_std.h"

#ifdef OST_TRACE_COMPILER_IN_USE
#include "cl_mainTraces.h"
#endif

GLDEF_C void Panic(TClientPanic aPanic)
//
// Panic the current client with a file server client side panic.
//
	{
	OstTrace1(TRACE_PANIC, EFSRV_EPANIC, "%d", aPanic);

	User::Panic(_L("FSCLIENT panic"),aPanic);
	}

GLDEF_C void Fault(TClientFault aFault)
//
// Panic the current client with a file server client side fault.
//
	{

	User::Panic(_L("FSCLIENT fault"),aFault);
	}

