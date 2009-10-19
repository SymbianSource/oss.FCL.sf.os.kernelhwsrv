// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// @file wakeuptimer.cpp
// @internalComponent
// 
//

#include "wakeuptimer.h"


namespace NUnitTesting_USBDI
	{

const TInt KOneSecond(1000000);

CRemoteWakeupTimer* CRemoteWakeupTimer::NewL(RUsbTestDevice& aTestDevice)
	{
	CRemoteWakeupTimer* self = new (ELeave) CRemoteWakeupTimer(aTestDevice);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}


CRemoteWakeupTimer::CRemoteWakeupTimer(RUsbTestDevice& aTestDevice)
:	CTimer(EPriorityStandard),
	iTestDevice(aTestDevice)
	{
	CActiveScheduler::Add(this);
	}


CRemoteWakeupTimer::~CRemoteWakeupTimer()
	{
	}


void CRemoteWakeupTimer::ConstructL()
	{
	LOG_FUNC
	CTimer::ConstructL();
	}


void CRemoteWakeupTimer::WakeUp(TUint16 aInterval)
	{
	LOG_FUNC
	After(aInterval*KOneSecond);
	}


void CRemoteWakeupTimer::RunL()
	{
	LOG_FUNC
	TInt completionCode(iStatus.Int());
	
	if(completionCode != KErrNone)
		{
		RDebug::Printf("<Error %d> software connect/disconnect timer error",completionCode);
		iTestDevice.ReportError(completionCode);
		}
	else
		{
		iTestDevice.RemoteWakeup();
		}	
	}	

	}



