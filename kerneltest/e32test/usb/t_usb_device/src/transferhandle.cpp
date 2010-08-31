/*
* Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Header file NTB build policy base class
*
*/


/**
@file
@internalComponent
*/

#include "transferhandle.h"
#include "transferserver.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "transferhandleTraces.h"
#endif

CTransferHandle* CTransferHandle::NewL(CTransferServer& aServer)
    {
    CTransferHandle *self=new (ELeave) CTransferHandle(aServer);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;    
    }

CTransferHandle::CTransferHandle(CTransferServer& aServer)
    : CActive(CActive::EPriorityStandard), iServer(aServer)
    {
    }

CTransferHandle::~CTransferHandle()
    {
    OstTrace0(TRACE_NORMAL, CTRANSFERHANDLE_DCTRANSFERHANDLE, "CTransferHandle::~CTransferHandle");
    Cancel();
    iTimer.Close();
    }

void CTransferHandle::DoCancel()
    {
    OstTrace0(TRACE_NORMAL, CTRANSFERHANDLE_DOCANCEL, "CTransferHandle::DoCancel");
    iTimer.Cancel();
    }

void CTransferHandle::ConstructL()
    {
    CActiveScheduler::Add(this);
    User::LeaveIfError(iTimer.CreateLocal());
    }

_LIT(KPanic, "CTransferHandle");
const TInt KTimerError = 1;


void CTransferHandle::RunL()
	{
	if(iStatus.Int() != KErrNone)
		{		
		OstTrace0(TRACE_FATAL, CTRANSFERHANDLE_RUNL, "CTransferHandle::RunL");
		User::Panic(KPanic, KTimerError);
		return;
		}
	iServer.TransferHandleL();
	}

void CTransferHandle::StartTimer()
	{
	iStatus = KRequestPending;
    iTimer.After(iStatus, 10000);
    SetActive();
	}


