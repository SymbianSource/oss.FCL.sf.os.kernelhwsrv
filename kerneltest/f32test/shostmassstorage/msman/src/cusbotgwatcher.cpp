// Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include <e32cmn.h>
#include <e32base.h>
#include <e32property.h>
#include <d32otgdi.h>
#include <e32debug.h>

#include "cusbotg.h"
#include "cusbotgwatcher.h"

#include "debug.h"


CUsbOtgBaseWatcher::CUsbOtgBaseWatcher(RUsbOtgDriver& aLdd)
:   CActive(CActive::EPriorityStandard),
    iLdd(aLdd)
    {
    CActiveScheduler::Add(this);
    }


CUsbOtgBaseWatcher::~CUsbOtgBaseWatcher()
    {
    Cancel();
    }

void CUsbOtgBaseWatcher::Start()
    {
    Post();
    }



CRequestSessionWatcher* CRequestSessionWatcher::NewL(MUsbRequestSessionObserver& aObserver)
    {
    CRequestSessionWatcher* r = new (ELeave) CRequestSessionWatcher(aObserver);
    r->ConstructL();
    return r;
    }

CRequestSessionWatcher::CRequestSessionWatcher(MUsbRequestSessionObserver& aObserver)
:   CActive(EPriorityStandard),
    iObserver(aObserver)
    {
    }

void CRequestSessionWatcher::ConstructL()
    {
    User::LeaveIfError(iProperty.Define(KUidUsbManCategory, KUsbRequestSessionProperty, RProperty::EInt));
    User::LeaveIfError(iProperty.Attach(KUidUsbManCategory, KUsbRequestSessionProperty));
    CActiveScheduler::Add(this);

    // initial subscription and process current property value
    RunL();
    }


void CRequestSessionWatcher::DoCancel()
    {
    iProperty.Cancel();
    }


CRequestSessionWatcher::~CRequestSessionWatcher()
    {
    Cancel();
    iProperty.Close();
    iProperty.Delete(KUidUsbManCategory, KUsbRequestSessionProperty);
    }


void CRequestSessionWatcher::RunL()
    {
    // resubscribe before processing new value to prevent missing updates
    iProperty.Subscribe(iStatus);
    SetActive();
    TInt val;
    User::LeaveIfError(iProperty.Get(KUidUsbManCategory, KUsbRequestSessionProperty, val));
    __USBOTGPRINT1(_L(">> CUsbRequestSessionWatcher[%d]"), val);

    switch(val)
        {
        case KUsbManSessionOpen:
            {
            iObserver.BusRequestL();
            }
            break;
        default:
            __USBOTGPRINT(_L("Event ignored"));
            break;
        }
    }

TInt CRequestSessionWatcher::RunError(TInt aError)
    {
    __USBOTGPRINT1(_L("CUsbRequestSessionWatcher::RunError[%d]"), aError);
    return KErrNone;
    }









CUsbOtgEventWatcher* CUsbOtgEventWatcher::NewL(RUsbOtgDriver& aLdd,
                                               CUsbOtg& aUsbOtg)
    {
    CUsbOtgEventWatcher* r = new (ELeave) CUsbOtgEventWatcher(aLdd, aUsbOtg);
    r->ConstructL();
    return r;
    }

CUsbOtgEventWatcher::CUsbOtgEventWatcher(RUsbOtgDriver& aLdd,
                                         CUsbOtg& aUsbOtg)
:   CUsbOtgBaseWatcher(aLdd),
    iUsbOtg(aUsbOtg)
    {
    }


void CUsbOtgEventWatcher::ConstructL()
    {
    }


void CUsbOtgEventWatcher::DoCancel()
    {
    iLdd.CancelOtgEventRequest();
    }


CUsbOtgEventWatcher::~CUsbOtgEventWatcher()
    {
    }


void CUsbOtgEventWatcher::Post()
    {
    iLdd.QueueOtgEventRequest(iEvent, iStatus);
    SetActive();
    }

void CUsbOtgEventWatcher::RunL()
    {
    TInt r = iStatus.Int();
    User::LeaveIfError(r);

    __USBOTGPRINT1(_L(">> CUsbOtgEventWatcher[%x]"), iEvent);
    User::LeaveIfError(r);

    iUsbOtg.HandleUsbOtgEvent(iEvent);
    Start();
    }

TInt CUsbOtgEventWatcher::RunError(TInt aError)
    {
    __USBOTGPRINT1(_L("CUsbRequestSessionWatcher::RunError[%d]"), aError);
    return KErrNone;
    }
