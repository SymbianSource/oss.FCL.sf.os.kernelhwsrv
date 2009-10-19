// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// USB Host Mass Storage
// 
//



/**
 @file
*/

#include <e32std.h>
#include <e32property.h>


#include "ttestutils.h"
#include "tmslog.h"
#include "e32test.h"
#include "e32debug.h"

extern RTest test;

    const TUid KMyPropertyCat = {0x10285B2E};
    enum TMyPropertyKeys
        {
        EMyPropertyEvent = 1,
        EMyPropertyServer = 2,
        EMyPropertyConnectionStateEvent = 3
        };

    const TUid KUsbmanSvrSid = {0x101fe1db};



void TTestUtils::WaitForBusEventL()
    {
    __MSFNSLOG
    _LIT_SECURITY_POLICY_PASS(KMsAllowAllPolicy);
    _LIT_SECURITY_POLICY_S0(KMsWritePolicy, KUsbmanSvrSid.iUid);

    RProperty evProp;
    User::LeaveIfError(evProp.Define(KMyPropertyCat,
                                     EMyPropertyEvent,
                                     RProperty::EInt,
                                     KMsAllowAllPolicy,
                                     KMsWritePolicy));

    TInt err = evProp.Attach(KMyPropertyCat, EMyPropertyEvent);
    User::LeaveIfError(err);

    TRequestStatus status;
    evProp.Subscribe(status);
    User::WaitForRequest(status);

    // Notification complete, retrieve the counter value.
    TInt event;
    evProp.Get(event);

    RProperty::Delete(EMyPropertyEvent);
    }


TBool TTestUtils::WaitForConnectionStateEventL()
    {
    __MSFNSLOG
    _LIT_SECURITY_POLICY_PASS(KMsAllowAllPolicy);
    _LIT_SECURITY_POLICY_S0(KMsWritePolicy, KUsbmanSvrSid.iUid);

    RProperty evProp;
    User::LeaveIfError(evProp.Define(KUsbmanSvrSid,
                                     EMyPropertyConnectionStateEvent,
                                     RProperty::EInt,
                                     KMsAllowAllPolicy,
                                     KMsWritePolicy));

    TInt err = evProp.Attach(KUsbmanSvrSid, EMyPropertyConnectionStateEvent);
    User::LeaveIfError(err);

    TRequestStatus status;
    evProp.Subscribe(status);
    User::WaitForRequest(status);

    // Notification complete, retrieve the counter value.
    TBool event;
    evProp.Get(event);

    RProperty::Delete(KUsbmanSvrSid, EMyPropertyConnectionStateEvent);
    return event;
    }



TTestTimer::TTestTimer()
    {

    }

void TTestTimer::Start()
    {
    iStart.HomeTime();
    }

void TTestTimer::End()
    {
    iEnd.HomeTime();

    TTimeIntervalSeconds timeTaken;
    iEnd.SecondsFrom(iStart, timeTaken);
    TUint totalTime = timeTaken.Int();
    test.Printf(_L("Time taken is %dHrs:%dmins:%dsecs\n"),
                  totalTime/3600,
                  (totalTime/60)%60,
                  totalTime%60);
    }

TTestTimer::~TTestTimer()
    {
    }
