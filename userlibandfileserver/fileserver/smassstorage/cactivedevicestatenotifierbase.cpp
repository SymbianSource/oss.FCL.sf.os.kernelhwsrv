/*
* Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Class definition for Device State Notifier Base Class
*
*/


/**
 @file
 @internalTechnology
*/

#include <e32base.h>

#include "mtransport.h"
#include "mldddevicestatenotification.h"
#include "drivemanager.h"
#include "cusbmassstoragecontroller.h"
#include "cbulkonlytransport.h"

#include "cactivedevicestatenotifierbase.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "cactivedevicestatenotifierbaseTraces.h"
#endif


CActiveDeviceStateNotifierBase::CActiveDeviceStateNotifierBase(CBulkOnlyTransport& aBot,
                                                               MLddDeviceStateNotification& aLddDeviceStateNotification)
    : CActive(EPriorityStandard),
      iBot(aBot),
      iLddDeviceStateNotification(aLddDeviceStateNotification),
      iDeviceState(EUsbcNoState),
      iOldDeviceState(EUsbcNoState)
    {

    }


CActiveDeviceStateNotifierBase* CActiveDeviceStateNotifierBase::NewL(CBulkOnlyTransport& aBot,
                                                                     MLddDeviceStateNotification& aLddDeviceStateNotification)
    {
    CActiveDeviceStateNotifierBase* self = new (ELeave) CActiveDeviceStateNotifierBase(aBot, aLddDeviceStateNotification);
    CleanupStack::PushL(self);
    self->ConstructL();
    CActiveScheduler::Add(self);
    CleanupStack::Pop();                                    // self
    return (self);
    }


void CActiveDeviceStateNotifierBase::ConstructL()
    {
    }


CActiveDeviceStateNotifierBase::~CActiveDeviceStateNotifierBase()
    {
    Cancel();                                               // base class
    }


void CActiveDeviceStateNotifierBase::DoCancel()
    {
    iLddDeviceStateNotification.Cancel();
    }


void CActiveDeviceStateNotifierBase::RunL()
    {
    // This displays the device state.
    // In a real world program, the user could take here appropriate action (cancel a
    // transfer request or whatever).

    OstTrace1(TRACE_SMASSSTORAGE_STATE, CACTIVEDEVICESTATENOTIFIERBASE_100,
              "DeviceState Notification = %d", iDeviceState);

    if (!(iDeviceState & KUsbAlternateSetting))
        {
        switch (iDeviceState)
            {
        case EUsbcDeviceStateUndefined:         //0
        case EUsbcDeviceStateDefault:           //3
            iBot.HwStop();
            break;

        case EUsbcDeviceStateAttached:          //1
        case EUsbcDeviceStatePowered:           //2
            // do nothing
            break;

        case EUsbcDeviceStateAddress:           //4
            if (iOldDeviceState == EUsbcDeviceStateConfigured)
                {
                iBot.StopBulkOnlyEndpoint();
                }
            break;

        case EUsbcDeviceStateConfigured:        //5
            if (iOldDeviceState == EUsbcDeviceStateSuspended)
                {
                iBot.HwResume();
                }
            else
                {
                iBot.HwStart();
                }
            break;
        case EUsbcDeviceStateSuspended:         //6
            if (iOldDeviceState == EUsbcDeviceStateConfigured)
                {
                iBot.HwSuspend();
                }
            break;
        default:
            OstTrace0(TRACE_SMASSSTORAGE_STATE, CACTIVEDEVICESTATENOTIFIERBASE_101,
                      "Device State notifier: ***BAD***");
            iBot.HwStop();
            break;
            }
        iOldDeviceState = iDeviceState;
        }
    else if (iDeviceState & KUsbAlternateSetting)
        {
        OstTrace1(TRACE_SMASSSTORAGE_STATE, CACTIVEDEVICESTATENOTIFIERBASE_102,
                  "Device State notifier: Alternate interface setting has changed: now %d",
                  iDeviceState & ~KUsbAlternateSetting);
        }
    Activate();
    }


void CActiveDeviceStateNotifierBase::Activate()
    {
    if (IsActive())
        {
        return;
        }
    iLddDeviceStateNotification.Activate(iStatus, iDeviceState);
    SetActive();
    }




