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
//

#include <e32cmn.h>
#include <e32base.h>
#include <e32property.h>
#include <d32otgdi.h>
#include <e32debug.h>

#include "cusbotg.h"
#include "cusbotgwatcher.h"

#include "tmslog.h"
#include "debug.h"


CUsbOtgBaseWatcher::CUsbOtgBaseWatcher(RUsbOtgDriver& aLdd)
:   CActive(CActive::EPriorityStandard),
    iLdd(aLdd)
	{
    __MSFNLOG
	CActiveScheduler::Add(this);
	}


CUsbOtgBaseWatcher::~CUsbOtgBaseWatcher()
	{
    __MSFNLOG
	Cancel();
	}

void CUsbOtgBaseWatcher::Start()
	{
    __MSFNLOG
	Post();
	}



CRequestSessionWatcher* CRequestSessionWatcher::NewL(MUsbRequestSessionObserver& aObserver)
    {
    __MSFNSLOG
    CRequestSessionWatcher* r = new (ELeave) CRequestSessionWatcher(aObserver);
    r->ConstructL();
    return r;
    }

CRequestSessionWatcher::CRequestSessionWatcher(MUsbRequestSessionObserver& aObserver)
:   CActive(EPriorityStandard),
    iObserver(aObserver)
    {
    __MSFNLOG
    }

void CRequestSessionWatcher::ConstructL()
    {
    __MSFNLOG
    User::LeaveIfError(iProperty.Define(KUidUsbManCategory, KUsbRequestSessionProperty, RProperty::EInt));
    User::LeaveIfError(iProperty.Attach(KUidUsbManCategory, KUsbRequestSessionProperty));
    CActiveScheduler::Add(this);

    // initial subscription and process current property value
	RunL();
    }


void CRequestSessionWatcher::DoCancel()
	{
    __MSFNLOG
	iProperty.Cancel();
	}


CRequestSessionWatcher::~CRequestSessionWatcher()
    {
    __MSFNLOG
    Cancel();
    iProperty.Close();
    iProperty.Delete(KUidUsbManCategory, KUsbRequestSessionProperty);
    }


void CRequestSessionWatcher::RunL()
    {
    __MSFNLOG
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
    __MSFNLOG
    __USBOTGPRINT1(_L("CUsbRequestSessionWatcher::RunError[%d]"), aError);
    return KErrNone;
    }









CUsbOtgEventWatcher* CUsbOtgEventWatcher::NewL(RUsbOtgDriver& aLdd,
                                               CUsbOtg& aUsbOtg)
    {
    __MSFNSLOG
    CUsbOtgEventWatcher* r = new (ELeave) CUsbOtgEventWatcher(aLdd, aUsbOtg);
    r->ConstructL();
    return r;
    }

CUsbOtgEventWatcher::CUsbOtgEventWatcher(RUsbOtgDriver& aLdd,
                                         CUsbOtg& aUsbOtg)
:   CUsbOtgBaseWatcher(aLdd),
    iUsbOtg(aUsbOtg)
    {
    __MSFNLOG
    }


void CUsbOtgEventWatcher::ConstructL()
    {
    __MSFNLOG
    }


void CUsbOtgEventWatcher::DoCancel()
	{
    __MSFNLOG
    iLdd.CancelOtgEventRequest();
	}


CUsbOtgEventWatcher::~CUsbOtgEventWatcher()
    {
    __MSFNLOG
    }


void CUsbOtgEventWatcher::Post()
    {
    __MSFNLOG
    iLdd.QueueOtgEventRequest(iEvent, iStatus);
    SetActive();
    }

void CUsbOtgEventWatcher::RunL()
    {
    __MSFNLOG

    TInt r = iStatus.Int();
    User::LeaveIfError(r);

    __USBOTGPRINT1(_L(">> CUsbOtgEventWatcher[%x]"), iEvent);
    User::LeaveIfError(r);

    iUsbOtg.HandleUsbOtgEvent(iEvent);
    Start();
    }

TInt CUsbOtgEventWatcher::RunError(TInt aError)
    {
    __MSFNLOG
    __USBOTGPRINT1(_L("CUsbRequestSessionWatcher::RunError[%d]"), aError);
    return KErrNone;
    }
