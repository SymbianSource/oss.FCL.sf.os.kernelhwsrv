// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "wakeuptimerTraces.h"
#endif


namespace NUnitTesting_USBDI
	{

const TInt KOneSecond(1000000);

CRemoteWakeupTimer* CRemoteWakeupTimer::NewL(RUsbTestDevice& aTestDevice)
	{
	OstTraceFunctionEntry1( CREMOTEWAKEUPTIMER_NEWL_ENTRY, ( TUint )&( aTestDevice ) );
	CRemoteWakeupTimer* self = new (ELeave) CRemoteWakeupTimer(aTestDevice);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CREMOTEWAKEUPTIMER_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}


CRemoteWakeupTimer::CRemoteWakeupTimer(RUsbTestDevice& aTestDevice)
:	CTimer(EPriorityStandard),
	iTestDevice(aTestDevice)
	{
	OstTraceFunctionEntryExt( CREMOTEWAKEUPTIMER_CREMOTEWAKEUPTIMER_ENTRY, this );
	CActiveScheduler::Add(this);
	OstTraceFunctionExit1( CREMOTEWAKEUPTIMER_CREMOTEWAKEUPTIMER_EXIT, this );
	}


CRemoteWakeupTimer::~CRemoteWakeupTimer()
	{
	OstTraceFunctionEntry1( CREMOTEWAKEUPTIMER_CREMOTEWAKEUPTIMER_ENTRY_DUP01, this );
	OstTraceFunctionExit1( CREMOTEWAKEUPTIMER_CREMOTEWAKEUPTIMER_EXIT_DUP01, this );
	}


void CRemoteWakeupTimer::ConstructL()
	{
	OstTraceFunctionEntry1( CREMOTEWAKEUPTIMER_CONSTRUCTL_ENTRY, this );
	CTimer::ConstructL();
	OstTraceFunctionExit1( CREMOTEWAKEUPTIMER_CONSTRUCTL_EXIT, this );
	}


void CRemoteWakeupTimer::WakeUp(TUint16 aInterval)
	{
	OstTraceFunctionEntryExt( CREMOTEWAKEUPTIMER_WAKEUP_ENTRY, this );
	After(aInterval*KOneSecond);
	OstTraceFunctionExit1( CREMOTEWAKEUPTIMER_WAKEUP_EXIT, this );
	}


void CRemoteWakeupTimer::RunL()
	{
	OstTraceFunctionEntry1( CREMOTEWAKEUPTIMER_RUNL_ENTRY, this );
	TInt completionCode(iStatus.Int());
	
	if(completionCode != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CREMOTEWAKEUPTIMER_RUNL, "<Error %d> software connect/disconnect timer error",completionCode);
		iTestDevice.ReportError(completionCode);
		}
	else
		{
		iTestDevice.RemoteWakeup();
		}	
	OstTraceFunctionExit1( CREMOTEWAKEUPTIMER_RUNL_EXIT, this );
	}	

	}



