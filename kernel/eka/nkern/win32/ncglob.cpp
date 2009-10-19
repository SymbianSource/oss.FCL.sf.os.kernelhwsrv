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
// e32\nkern\win32\ncglob.cpp
// 
//

#include "nk_priv.h"

TScheduler TheScheduler;
Win32Interrupt Interrupt;
TBool Win32AtomicSOAW;
TAny* Win32ExcAddress;
TAny* Win32ExcDataAddress;
TUint Win32ExcCode;
TBool Win32TraceThreadId=TRUE;
TInt Win32SingleCpu=-1;

SBTraceData BTraceData = { {0}, 0, 0 };
