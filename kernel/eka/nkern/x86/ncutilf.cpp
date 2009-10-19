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
// e32\nkern\x86\ncutilf.cpp
// 
//

#include <nkern.h>


/** Get the current value of the high performance counter.

    If a high performance counter is not available, this uses the millisecond
    tick count instead.
*/
EXPORT_C TUint32 NKern::FastCounter()
	{
	return TickCount();  // not implemented for x86
	}


/** Get the frequency of counter queried by NKern::FastCounter().
*/
EXPORT_C TInt NKern::FastCounterFrequency()
	{
	return 1000000 / TickPeriod();  // not implemented for x86
	}
