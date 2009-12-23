// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "msdebug.h"
#include "debug.h"


void CUsbMsIfaceSuspendResume::RunL()
    {
    __MSFNLOG
	iDevice->ResumeCompletedL();
	if(iCancelSuspend)
		{
		User::RequestComplete(iDeviceStatus, iStatus.Int());
		iDeviceStatus = NULL;
		}
	else
		{
		iDevice->DoHandleRemoteWakeupL();
		}
    }

/**
Cancellation of outstanding request
*/
void CUsbMsIfaceSuspendResume::DoCancel()
	{
    __MSFNLOG
	}

TInt CUsbMsIfaceSuspendResume::RunError(TInt aError)
	{
    __MSFNLOG
    return KErrNone;
	}


void CUsbMsIfaceSuspendResume::Resume(TRequestStatus& aStatus)
	{
    __MSFNLOG
	iCancelSuspend = ETrue;
    aStatus = KRequestPending;
	iDeviceStatus = &aStatus;
	iTransport->Resume();
	}

void CUsbMsIfaceSuspendResume::Suspend()
	{
    __MSFNLOG
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
:	CActive(EPriorityHigh),
    iTransport(aTransport),
    iDevice(aDevice),
    iCancelSuspend(EFalse)
	{
    __MSFNLOG
	CActiveScheduler::Add(this);
	}

CUsbMsIfaceSuspendResume::~CUsbMsIfaceSuspendResume()
	{
    __MSFNLOG
	Cancel();
	}

