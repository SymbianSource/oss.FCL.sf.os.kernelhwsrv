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
#include <d32otgdi.h>
#include <e32property.h>

#include "cusbotg.h"
#include "cusbotgwatcher.h"
#include "rusbmspublisher.h"


#include "tmslog.h"
#include "debug.h"

_LIT(KOtgdiLddFileName, "otgdi");


ROtgStateChangeNotifier::ROtgStateChangeNotifier()
:   iRegistered(EFalse)
    {
    __MSFNSLOG
    }


ROtgStateChangeNotifier::~ROtgStateChangeNotifier()
    {
    __MSFNSLOG
    if (iRegistered)
        iMessage.Complete(KErrDisconnected);
    }

/**
Initialise notifier to enable media change notfications.

@param aMessage The message to commplete the notification
*/
void ROtgStateChangeNotifier::Register(const RMessage2& aMessage)
    {
    __MSFNLOG
	iRegistered = ETrue;
	iMessage = aMessage;
    }


void ROtgStateChangeNotifier::DoNotifyL()
    {
	__MSFNLOG
	CompleteNotifierL(KErrNone);
    }


void ROtgStateChangeNotifier::DoCancelL()
    {
	__MSFNLOG
	CompleteNotifierL(KErrCancel);
    }


void ROtgStateChangeNotifier::CompleteNotifierL(TInt aReason)
	{
    __MSFNLOG
	if (iRegistered)
        {
		TBool changed = ETrue;
        TPckgBuf<TBool> p(changed);
		iMessage.WriteL(0, p);
		iMessage.Complete(aReason);
		iRegistered = EFalse;
        }
	}


CUsbOtg* CUsbOtg::NewL()
	{
    __MSFNSLOG
	CUsbOtg* self = new (ELeave) CUsbOtg();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}


void CUsbOtg::ConstructL()
	{
    __MSFNLOG

    TInt r = User::LoadLogicalDevice(KOtgdiLddFileName);

    if (r != KErrNone && r != KErrAlreadyExists)
        {
        __USBOTGPRINT1(_L("   LoadLogicalDevice(KOtgdiLddFileName) error = %d"), r);
        User::Leave(r);
        }

    r = iUsbOtgDriver.Open();
    if (r != KErrNone && r != KErrAlreadyExists)
        {
        __USBOTGPRINT1(_L("   otg.Open fails %d"), r);
        User::FreeLogicalDevice(RUsbOtgDriver::Name());
        User::Leave(r);
        }

    StartL();

    r = iUsbOtgDriver.StartStacks();
    if (r != KErrNone)
        {
        __USBOTGPRINT1(_L("   otg.StartStacks fails %d"), r);
        User::FreeLogicalDevice(RUsbOtgDriver::Name());
        User::Leave(r);
        }

    __USBOTGPRINT(_L("   otg stacks successfully started"));
	}


CUsbOtg::CUsbOtg()
:   iOtgState(KOtgStateStart)
    {
    __MSFNLOG
    }


CUsbOtg::~CUsbOtg()
	{
    __MSFNLOG
    Stop();

    TInt r = iUsbOtgDriver.BusDrop();

    // Unload OTGDI components if it was ever started

    if (iUsbOtgDriver.Handle())
        {
        iUsbOtgDriver.StopStacks();
        iUsbOtgDriver.Close();

        }

    TInt err = User::FreeLogicalDevice(RUsbOtgDriver::Name());
	}


void CUsbOtg::StartL()
    {
    __MSFNLOG
	// Request Otg notifications
    iOtgEventWatcher = CUsbOtgEventWatcher::NewL(iUsbOtgDriver, *this);
	iOtgEventWatcher->Start();

    iRequestSessionWatcher = CRequestSessionWatcher::NewL(*this);
    }


void CUsbOtg::Stop()
/**
 * Stop the USB OTG events watcher
 */
    {
    __MSFNLOG

    if (iOtgEventWatcher)
        {
        iOtgEventWatcher->Cancel();
        delete iOtgEventWatcher;
        iOtgEventWatcher = NULL;
        }

    if (iRequestSessionWatcher)
        {
        delete iRequestSessionWatcher;
        iRequestSessionWatcher = NULL;
        }
    }


void CUsbOtg::BusRequestL()
    {
    __MSFNLOG
    if (iOtgState == KOtgStateAPlugInserted)
        {
        TInt err = iUsbOtgDriver.BusRequest();
        if (err)
            {
            __USBOTGPRINT1(_L("OTG::BusRequest[%d] failed !"), err);
            }
        User::LeaveIfError(err);
        iOtgState = KOtgStateSessionOpen;
        }

    }


void CUsbOtg::HandleUsbOtgEvent(RUsbOtgDriver::TOtgEvent aEvent)
    {
    __MSFNLOG

    switch (aEvent)
        {
        case RUsbOtgDriver::EEventAPlugInserted:
            {
            __USBOTGPRINT(_L(">> UsbOtgEvent[EEventAPlugInserted]"));
            /*
            RUsbOtgEventPublisher eventPublisher;
            eventPublisher.PublishEvent(aEvent);
            */
            iNotifier.DoNotifyL();
            iOtgState = KOtgStateAPlugInserted;
            }
            break;

        case RUsbOtgDriver::EEventBusConnectionBusy:
            {
            RUsbManConnectionStatePublisher publisher;
            publisher.PublishEvent(ETrue);
            }
            break;
        case RUsbOtgDriver::EEventBusConnectionIdle:
            {
            RUsbManConnectionStatePublisher publisher;
            publisher.PublishEvent(EFalse);
            }
            break;
        default:
            __USBOTGPRINT1(_L(">> UsbOtgEvent[%x]"), aEvent);
            break;
        }
    }


TBool CUsbOtg::DeviceInserted()
    {
    __MSFNLOG
    return iOtgState == KOtgStateAPlugInserted ? ETrue : EFalse;
    }

void CUsbOtg::NotifyChange(const RMessage2& aMessage)
    {
    __MSFNLOG
    iNotifier.Register(aMessage);
    }


void CUsbOtg::NotifyChangeCancel()
    {
    __MSFNLOG
    iNotifier.DoCancelL();
    }


TInt CUsbOtg::BusDrop()
    {
    __MSFNLOG
    return iUsbOtgDriver.BusDrop();
    }
