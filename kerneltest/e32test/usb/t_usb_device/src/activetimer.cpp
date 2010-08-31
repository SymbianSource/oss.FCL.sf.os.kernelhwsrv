// Copyright (c) 2000-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/usb\t_usb_device\src\activetimer.cpp
// USB Test Program T_USB_DEVICE, functional part.
// Device-side part, to work against T_USB_HOST running on the host.
// 
//

#include "general.h"									// CActiveControl, CActiveRW
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "activetimerTraces.h"
#endif
#include "activetimer.h"

extern RTest test;
extern TBool gVerbose;
extern TBool gSkip;
extern TBool gStopOnFail;
extern TInt gSoakCount;

//
// --- class CActiveTimer ---------------------------------------------------------
//

CActiveTimer::CActiveTimer(CConsoleBase* aConsole, RDEVCLIENT* aPort)
	: CActive(EPriorityNormal),
	  iConsole(aConsole),
	  iPort(aPort)
	{
	CActiveScheduler::Add(this);
	}


CActiveTimer* CActiveTimer::NewL(CConsoleBase* aConsole, RDEVCLIENT* aPort)
	{
	CActiveTimer* self = new (ELeave) CActiveTimer(aConsole, aPort);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();									// self
	return self;
	}


void CActiveTimer::ConstructL()
	{
	User::LeaveIfError(iTimer.CreateLocal());
	}


CActiveTimer::~CActiveTimer()
	{
	TUSB_VERBOSE_PRINT("CActiveTimer::~CActiveTimer()");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVETIMER_DCACTIVETIMER, "CActiveTimer::~CActiveTimer()");
	    }
	Cancel();												// base class
	iTimer.Close();
	}


void CActiveTimer::DoCancel()
	{
	TUSB_VERBOSE_PRINT("CActiveTimer::DoCancel()");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVETIMER_DOCANCEL, "CActiveTimer::DoCancel()");
	    }
	iTimer.Cancel();
	}


void CActiveTimer::RunL()
	{
	TUSB_VERBOSE_PRINT("CActiveTimer::RunL()");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVETIMER_RUNL, "CActiveTimer::RunL()");
	    }
	// Nothing to do here, as we call ReadCancel() after a manual WaitForRequest()
	// (in CActiveRW::ReceiveVersion()).
	}


void CActiveTimer::Activate(TTimeIntervalMicroSeconds32 aDelay)
	{
	__ASSERT_ALWAYS(!IsActive(), User::Panic(KActivePanic, 666));
	iTimer.After(iStatus, aDelay);
	SetActive();
	}


// -eof-
