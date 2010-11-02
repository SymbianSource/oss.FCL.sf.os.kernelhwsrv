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
#include <d32otgdi.h>
#include <e32property.h>

#include "cusbotg.h"
#include "cusbotgwatcher.h"
#include "rusbmspublisher.h"

#include "debug.h"

_LIT(KOtgdiLddFileName, "otgdi");


ROtgStateChangeNotifier::ROtgStateChangeNotifier()
:   iRegistered(EFalse)
    {
    }


ROtgStateChangeNotifier::~ROtgStateChangeNotifier()
    {
    if (iRegistered)
        iMessage.Complete(KErrDisconnected);
    }

/**
Initialise notifier to enable media change notfications.

@param aMessage The message to commplete the notification
*/
void ROtgStateChangeNotifier::Register(const RMessage2& aMessage)
    {
    iRegistered = ETrue;
    iMessage = aMessage;
    }


void ROtgStateChangeNotifier::DoNotifyL()
    {
    CompleteNotifierL(KErrNone);
    }


void ROtgStateChangeNotifier::DoCancelL()
    {
    CompleteNotifierL(KErrCancel);
    }


void ROtgStateChangeNotifier::CompleteNotifierL(TInt aReason)
    {
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
    CUsbOtg* self = new (ELeave) CUsbOtg();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }


void CUsbOtg::ConstructL()
    {

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
    }


CUsbOtg::~CUsbOtg()
    {
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
    return iOtgState == KOtgStateAPlugInserted ? ETrue : EFalse;
    }

void CUsbOtg::NotifyChange(const RMessage2& aMessage)
    {
    iNotifier.Register(aMessage);
    }


void CUsbOtg::NotifyChangeCancel()
    {
    iNotifier.DoCancelL();
    }


TInt CUsbOtg::BusDrop()
    {
    return iUsbOtgDriver.BusDrop();
    }
