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
    RDebug::Printf("CTransferHandle::~CTransferHandle");
    Cancel();
    iTimer.Close();
    }

void CTransferHandle::DoCancel()
    {
    RDebug::Printf("CTransferHandle::DoCancel");
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
		RDebug::Printf("CTransferHandle::RunL");
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


