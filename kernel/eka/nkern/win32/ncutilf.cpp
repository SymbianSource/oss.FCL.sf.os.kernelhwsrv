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
// e32\nkern\win32\ncutilf.cpp
// 
//

#include "nk_priv.h"

// Win32 fast counter implementation

static TBool FastCounterSupported = FALSE;
static TInt FastCounterShift;
static TInt FastCounterAdjustedFreq;

/** Initialise the fast counter.

    Check whether a high performance counter is available and determine its
    frequency.  Work out a scaling that allows a 32 bit counter value that wraps
    at most every 16 seconds.
*/
void FastCounterInit()
	{
	// Test support by getting count - QueryPerformanceFrequency can succeed
	// even if not supported
	LARGE_INTEGER count;
	count.QuadPart = 0;
	if (!QueryPerformanceCounter(&count) || count.QuadPart == 0)
		return;  // not supported
	
	LARGE_INTEGER freq;
	freq.QuadPart = 0;
	if (!QueryPerformanceFrequency(&freq) || freq.QuadPart == 0)
		return;  // not supported

	TInt mso = __e32_find_ms1_32(freq.LowPart);
	FastCounterShift = (mso > 27) ? mso - 27 : 0;
	
	FastCounterSupported = TRUE;
	FastCounterAdjustedFreq = freq.LowPart >> FastCounterShift;	
	}
    
/** Get the current value of the high performance counter.

    If a high performance counter is not available, this uses the millisecond
    tick count instead.
*/
EXPORT_C TUint32 NKern::FastCounter()
	{
	if (FastCounterSupported)
		{
		LARGE_INTEGER count;
		count.QuadPart = 0;
		QueryPerformanceCounter(&count);
		return (TUint32)(count.QuadPart >> FastCounterShift);
		}
	else
		return NTickCount();
	}


/** Get the frequency of counter queried by NKern::FastCounter().
*/
EXPORT_C TInt NKern::FastCounterFrequency()
	{
	if (FastCounterSupported)
		return FastCounterAdjustedFreq;
	else
		return 1000000 / TickPeriod();
	}
