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

#include <e32base.h>
#include <f32file.h>
#include <d32usbdi_hubdriver.h>
#include "usbtypes.h"
#include "rusbhostmsdevice.h"
#include "rextfilesystem.h"

#include "cusbmsmountmanager.h"
//#include "cusbhost.h"
#include "cusbhostao.h"
#include "tmslog.h"
#include "debug.h"


_LIT(KTxtApp,"CUSBHOSTAO");

CUsbHostAo* CUsbHostAo::NewL(RUsbHubDriver& aHubDriver,
                             RUsbHubDriver::TBusEvent& aEvent,
                             MUsbHostBusEventObserver& aObserver)
    {
    __MSFNSLOG
    CUsbHostAo* r = new (ELeave) CUsbHostAo(aHubDriver, aEvent, aObserver);
	r->ConstructL();
	return r;
    }


void CUsbHostAo::ConstructL()
    {
    __MSFNLOG
    }


CUsbHostAo::CUsbHostAo(RUsbHubDriver& aHubDriver,
                       RUsbHubDriver::TBusEvent& aEvent,
                       MUsbHostBusEventObserver& aObserver)
:   CActive(EPriorityStandard),
    iHubDriver(aHubDriver),
    iEvent(aEvent),
    iObserver(aObserver)
    {
    __MSFNLOG
    CActiveScheduler::Add(this);
    }


CUsbHostAo::~CUsbHostAo()
    {
    __MSFNLOG
	Cancel();

    }


void CUsbHostAo::Wait()
    {
    __MSFNLOG
	if (IsActive())
		{
		__ASSERT_ALWAYS(EFalse, User::Panic(KTxtApp, -1));
		return;
		}

    __USBHOSTPRINT(_L("WaitForBusEvent..."));
	iHubDriver.WaitForBusEvent(iEvent, iStatus);
    __USBHOSTPRINT2(_L("WaitForBusEvent done. Event=%d Status=%d"),
                    iEvent.iEventType, iStatus.Int());
	SetActive();
    }


void CUsbHostAo::DoCancel()
	{
    __MSFNLOG
    iHubDriver.CancelWaitForBusEvent();
	}


void CUsbHostAo::RunL()
	{
    __MSFNLOG

    TInt status = iStatus.Int();
	if (status == KErrNotReady)
		{
        const TInt KDelay = 500 * 1000;  // in uSecs
        User::After(KDelay);
        Wait();
		return;
		}

    // Let RunError handle any other error
    User::LeaveIfError(status);

    // Process bus event
    TRAP(status, iObserver.ProcessBusEventL());
	if(status != KErrNone)
		{
        Wait();
		return;
		}

    Wait();
	}

_LIT(KErrLog, "ERROR %d in CActiveUsbHost");

TInt CUsbHostAo::RunError(TInt aError)
	{
	__MSFNLOG
	RDebug::Print(KErrLog, aError);
	return KErrNone;
	}



