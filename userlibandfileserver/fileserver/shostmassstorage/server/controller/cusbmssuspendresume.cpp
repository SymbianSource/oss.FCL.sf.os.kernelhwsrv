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
#include "cusbhostmslogicalunit.h"
#include "cusbhostmsdevice.h"
#include "cbulkonlytransport.h"
#include "cusbmssuspendresume.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "cusbmssuspendresumeTraces.h"
#endif


void CUsbMsIfaceSuspendResume::RunL()
    {
    iDevice->ResumeCompletedL();
    if(iCancelSuspend)
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_SUSPEND, CUSBMSSUSPENDRESUME_10,
                  "SUSPEND/RESUME completed.");
        User::RequestComplete(iDeviceStatus, iStatus.Int());
        iDeviceStatus = NULL;
        }
    else
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_SUSPEND, CUSBMSSUSPENDRESUME_11,
                  "REMOTE WAKEUP");
        iDevice->DoHandleRemoteWakeupL();
        }
    }

/**
Cancellation of outstanding request
*/
void CUsbMsIfaceSuspendResume::DoCancel()
    {
    }

TInt CUsbMsIfaceSuspendResume::RunError(TInt aError)
    {
    OstTrace1(TRACE_SHOSTMASSSTORAGE_SUSPEND, CUSBMSSUSPENDRESUME_12,
                  "Error = %d", aError);
    return KErrNone;
    }


void CUsbMsIfaceSuspendResume::Resume(TRequestStatus& aStatus)
    {
    OstTrace0(TRACE_SHOSTMASSSTORAGE_SUSPEND, CUSBMSSUSPENDRESUME_13,
                  "RESUME");
    iCancelSuspend = ETrue;
    aStatus = KRequestPending;
    iDeviceStatus = &aStatus;
    iTransport->Resume();
    }

void CUsbMsIfaceSuspendResume::Suspend()
    {
    OstTrace0(TRACE_SHOSTMASSSTORAGE_SUSPEND, CUSBMSSUSPENDRESUME_14,
                  "SUSPEND");
    if(!IsActive())
        SetActive();
    iCancelSuspend = EFalse;
    iTransport->Suspend(iStatus);
    }

CUsbMsIfaceSuspendResume* CUsbMsIfaceSuspendResume::NewL(MTransport *aTransport, CUsbHostMsDevice *aDevice)
    {
    return new (ELeave) CUsbMsIfaceSuspendResume(aTransport, aDevice);
    }

CUsbMsIfaceSuspendResume::CUsbMsIfaceSuspendResume(MTransport* aTransport, CUsbHostMsDevice* aDevice)
:   CActive(EPriorityHigh),
    iTransport(aTransport),
    iDevice(aDevice),
    iCancelSuspend(EFalse)
    {
    CActiveScheduler::Add(this);
    }

CUsbMsIfaceSuspendResume::~CUsbMsIfaceSuspendResume()
    {
    Cancel();
    }

