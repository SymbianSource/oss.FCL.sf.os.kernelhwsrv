// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\window\t_keyrepeat.cpp
// Overview:
// Test the TRawEvent APIS and events related to keyrepeat
// Tests both user and kernel side functions
// API Information:
// UserSvr, TRawEvent
// Details:
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites: 
// Failures and causes:
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32svr.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include "d_keyrepeat.h"

LOCAL_D RTest test(_L("T_KEYREPEAT"));

RTestKeyRepeatLdd gLdd;

// default constructor
RKeyEvent::RKeyEvent()
	{
	}

// constructor for RKeyEvent
RKeyEvent::RKeyEvent(TStdScanCode aKey, TInt aRepeatCount)
: iKey(aKey), iRepeatCount(aRepeatCount)
	{
	}

void LoadDeviceDriver()
	{
	TInt r;

	r=User::LoadLogicalDevice(KLddName);
	test_KErrNone(r);
	
	r=gLdd.Open();
	test_KErrNone(r);
	}

void UnloadDeviceDriver()
	{
	TInt r;
	gLdd.Close();

	r = User::FreeLogicalDevice(KLddName);
	test_KErrNone(r);
	User::After(100000);
	}


GLDEF_C TInt E32Main()
//
//
    {
 	test.Title();
	test.Start(_L("Testing user side TRawEvent::SetRepeat and ::Repeats API"));
	TInt numRepeats=10;

	// create event objects
	RKeyEvent theKeyEvent(EStdKeySpace, numRepeats);
	TRawEvent theEvent;
	// set repeat
	theEvent.SetRepeat(TRawEvent::EKeyRepeat, theKeyEvent.iKey, theKeyEvent.iRepeatCount);
	// send event
	test_KErrNone(UserSvr::AddEvent(theEvent));
	// check repeat value
	test_Equal(theKeyEvent.iRepeatCount, theEvent.Repeats());
	test.Printf(_L("T_KEYREPEAT: USER SIDE TEST Successfully Completed\n"));

	test.Next(_L("Testing kernel side TRawEvent::SetRepeat and ::Repeats API"));

	LoadDeviceDriver();
	// call kernel side to set repeat values and send the event
	test_KErrNone(gLdd.SetRepeat(theKeyEvent));
	// call kernel side to check that the repeat count was set correctly
	test_KErrNone(gLdd.Repeats());

	UnloadDeviceDriver();
	test.Printf(_L("T_KEYREPEAT: KERNEL SIDE TEST Successfully Completed\n"));

	test.End();
	test.Close();
    return KErrNone;
    }

