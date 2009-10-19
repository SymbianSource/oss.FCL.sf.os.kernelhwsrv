// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\earlyextension\earlyextension.cpp
// This is to test the registration of early extension and will do the following
// - Constructing the static data in VariantInit0 (the main extension object)
// - In Init3 entrypoint allocating space in Kernel Heap to store the time stamp obtained
// calling Kern::SystemTime().
// - Supplying one exported function to allow reading the time stamp.
// 
//


#include <kernel/kernel.h>
#include "earlyextension.h"

static TestEarlyExtension* EarlyExtension;

/** Registering as early extension */
DECLARE_EXTENSION_WITH_PRIORITY(EARLY_EXTENSION_PRIORITY)
	{
	TestEarlyExtension* ee = new TestEarlyExtension;
	if(!ee)
		return KErrNoMemory;
	EarlyExtension=ee;
	EarlyExtension->iTime = new TTimeK; //Allocate memory for storing time stamp.
    if(!EarlyExtension->iTime)
		return KErrNoMemory;
	*EarlyExtension->iTime = Kern::SystemTime(); //Store time stamp
	// wait one tick to guarantee that system time will be different if called from the entry point of a following extension
	NKern::Sleep(1);
    return KErrNone;
	}

/** This function allows to read the time stamp taken during the init3 entry point */
EXPORT_C void TestEarlyExtension::GetTimeStamp(TTimeK& aTime)
    {
    if(!EarlyExtension->iTime)
        {
        aTime = 0;
        return;
        }    
	aTime = *EarlyExtension->iTime;
	return;
	}


