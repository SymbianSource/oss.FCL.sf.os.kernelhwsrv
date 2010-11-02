// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file
 @internalTechnology
*/

#include <e32base.h>
#include <d32usbdi.h>

#include <d32usbtransfers.h>
#include "msctypes.h"
#include "shared.h"
#include "msgservice.h"
#include "botmsctypes.h"
#include "mprotocol.h"
#include "mtransport.h"
#include "cbulkonlytransport.h"
#include "cusbhostmslogicalunit.h"
#include "cusbhostmsdevice.h"
#include "cusbmssuspendresume.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "cusbhostmsdeviceTraces.h"
#endif


CUsbHostMsDevice* CUsbHostMsDevice::NewL(THostMassStorageConfig& aConfig)
    {
    CUsbHostMsDevice* r = new (ELeave) CUsbHostMsDevice(aConfig);
    CleanupStack::PushL(r);
    r->ConstructL();
    CleanupStack::Pop();
    return r;
    }

void CUsbHostMsDevice::ConstructL()
    {
    iTimer = CPeriodic::NewL(CActive::EPriorityStandard);
    iTimerRunning = EFalse;
    }


CUsbHostMsDevice::CUsbHostMsDevice(THostMassStorageConfig& aConfig)
:   iConfig(aConfig),
    iState(EReady)
    {
    }


CUsbHostMsDevice::~CUsbHostMsDevice()
    {
    delete iTransport;
    delete iDeviceSuspendResume;
    if (iTimer && iTimerRunning)
        {
        iTimer->Cancel();
        }
    delete iTimer;
    }


MTransport* CUsbHostMsDevice::InitialiseTransportL(TTransportType aTransportId)
    {
    switch(aTransportId)
        {
    case BulkOnlyTransport:
        return CBulkOnlyTransport::NewL(iConfig.iInterfaceToken);
    default:
    //      Panic;
        OstTrace0(TRACE_SHOSTMASSSTORAGE_HOST, CUSBHOSTMSDEVICE_10,
                  "Unsupported Transport class requested");
        User::Leave(KErrNotSupported);
        return NULL;
        }
    }

void CUsbHostMsDevice::InitialiseL(const RMessage2& aMessage)
    {
    iTransport = InitialiseTransportL((TTransportType) iConfig.iTransportId);
    TRAPD(r, iDeviceSuspendResume = CUsbMsIfaceSuspendResume::NewL(iTransport, this));
    if(r != KErrNone)
        {
        delete iTransport;
        User::Leave(r);
        }
    iTransport->GetMaxLun(&iMaxLun, aMessage);
    }


void CUsbHostMsDevice::UnInitialiseL()
    {
    StopTimer();
    iLuList.RemoveAllLuL();
    }


TInt CUsbHostMsDevice::AddLunL(TLun aLun)
    {
    TInt r = KErrNone;
    StartTimer();
    CUsbHostMsLogicalUnit* lu = CUsbHostMsLogicalUnit::NewL(aLun);
    CleanupStack::PushL(lu);

    TRAP(r, lu->InitialiseProtocolL(aLun, iConfig, *iTransport));

    if (r == KErrNone)
        {
        TRAP(r, iLuList.AddLuL(lu));
        }

    if (r != KErrNone)
        {
        CleanupStack::PopAndDestroy(lu);
        }
    else
        {
        CleanupStack::Pop(lu);
        }
    return r;
    }


void CUsbHostMsDevice::RemoveLunL(TLun aLun)
    {
    if(iLuList.Count() <= 1)
        StopTimer();
    iLuList.RemoveLuL(aLun);
    }


void CUsbHostMsDevice::InitLunL(TLun aLun)
    {
    CUsbHostMsLogicalUnit& lu = SetLunL(aLun);
    lu.InitL();
    }


void CUsbHostMsDevice::SuspendLunL(TLun aLun)
    {
    if (IsSuspended())
        {
        return;
        }

    iLuList.GetLuL(aLun).ReadyToSuspend();

    // check whether all the luns are suspended, if so then request usb
    // interface suspension to the transport layer
    for (TInt i = 0; i < iLuList.Count(); i++)
        {
        CUsbHostMsLogicalUnit& lu = iLuList.GetLu(i);
        if (!lu.IsReadyToSuspend() && lu.IsConnected())
            return;
        }

    for (TInt i = 0; i < iLuList.Count(); i++)
        {
        CUsbHostMsLogicalUnit& lu = iLuList.GetLu(i);
        SetLunL(lu);
        lu.SuspendL();
        }

    StopTimer();
    iDeviceSuspendResume->Suspend();
    iState = ESuspended;
    }


void CUsbHostMsDevice::Resume(TRequestStatus& aStatus)
    {
    if (iState == ESuspended)
        {
        StartTimer();
        iDeviceSuspendResume->Resume(aStatus);
        }
    }


TLun CUsbHostMsDevice::GetAndSetLunL(const RMessage2& aMessage)
    {
    // Subssessions need a positive value to store in the handles. We represent Luns as LunId+1
    // We represent LunId in MSC from 0 to MaxLun-1 as represented in BOT so subtract 1 from the Id
    // received from RMessage

    TInt lun = aMessage.Int3() - 1;
    if (lun < 0)
        {
        User::Leave(KErrArgument);
        }
    SetLunL(static_cast<TLun>(lun));
    return static_cast<TLun>(lun);
    }


CUsbHostMsLogicalUnit& CUsbHostMsDevice::GetLuL(TInt aLunNum) const
    {
    return iLuList.GetLuL(aLunNum);
    }


void CUsbHostMsDevice::SetLunL(CUsbHostMsLogicalUnit& aLu)
    {
    TLun lun = aLu.Lun();
    if (lun <= iMaxLun)
        {
        OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, CUSBHOSTMSDEVICE_20,
                  "SetLun %d", lun);
        iTransport->SetLun(lun);
        if (aLu.IsReadyToSuspend())
            {
            aLu.CancelReadyToSuspend();
            }
        }
    }


CUsbHostMsLogicalUnit& CUsbHostMsDevice::SetLunL(TLun aLun)
    {
    CUsbHostMsLogicalUnit& lu = iLuList.GetLuL(aLun);
    SetLunL(lu);
    return lu;
    }

/**
Starts timer to periodically check LUN. If the timer is not yet running then
start it.
*/
void CUsbHostMsDevice::StartTimer()
    {
    if (!iTimerRunning)
        {
        // Period of the LUN Ready check
        const TTimeIntervalMicroSeconds32 KInterval = iConfig.iStatusPollingInterval * 1000 * 1000;
        TCallBack callback(TimerCallback, this);
        OstTrace0(TRACE_SHOSTMASSSTORAGE_HOST, CUSBHOSTMSDEVICE_21,
                  "Starting timer");
        iTimer->Start(KInterval, KInterval, callback);
        iTimerRunning = ETrue;
        }
    }


/**
Ensure that the Timer is stopped
*/
void CUsbHostMsDevice::StopTimer()
    {
    if (iTimer && iTimerRunning)
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_HOST, CUSBHOSTMSDEVICE_30,
                  "Stopping timer");
        if (iTimer->IsActive())
            {
            iTimer->Cancel();
            }
        iTimerRunning = EFalse;
        }
    }

/**
A static wrapper for the DoLunReadyCheckEvent member function for use as a timer
callback function.

@param obj 'this' pointer
@return not used in CPeriodic callback (see TCallback)
*/
TInt CUsbHostMsDevice::TimerCallback(TAny* obj)
    {
    CUsbHostMsDevice* device = static_cast<CUsbHostMsDevice*>(obj);
    TRAPD(err, device->DoLunReadyCheckEventL());
    return err;
    }

void CUsbHostMsDevice::DoLunReadyCheckEventL()
    {
    TInt err;
    for (TInt i = 0; i < iLuList.Count(); i++)
        {
        CUsbHostMsLogicalUnit& lu = iLuList.GetLu(i);
        SetLunL(lu);
        TRAP(err, lu.DoLunReadyCheckL());
        }
    }

void CUsbHostMsDevice::DoHandleRemoteWakeupL()
    {
    DoResumeLogicalUnitsL();
    DoLunReadyCheckEventL();    // For remote wakeup we do not wait for timer to expire

    // check whether all the luns are suspended, if so then request usb
    // interface suspension to the transport layer
    for (TInt i = 0; i < iLuList.Count(); i++)
        {
        CUsbHostMsLogicalUnit& lu = iLuList.GetLu(i);
        // Has any of the logical units have got its state changed?
        if ( (lu.IsReadyToSuspend() && !lu.IsConnected()) ||
                (!lu.IsReadyToSuspend() && lu.IsConnected()) )
            {
            StartTimer(); // Now start the timer
            return;
            }
        }

    for (TInt i = 0; i < iLuList.Count(); i++)
        {
        CUsbHostMsLogicalUnit& lu = iLuList.GetLu(i);
        SetLunL(lu);
        lu.SuspendL();
        }

    iDeviceSuspendResume->Suspend();
    iState = ESuspended;
    }

void CUsbHostMsDevice::DoResumeLogicalUnitsL()
    {
    for (TInt i = 0; i < iLuList.Count(); i++)
        {
        CUsbHostMsLogicalUnit& lu = iLuList.GetLu(i);
        SetLunL(lu);
        lu.ResumeL();
        }
    }

void CUsbHostMsDevice::ResumeCompletedL()
    {
    iState = EReady;
    DoResumeLogicalUnitsL();
    }
