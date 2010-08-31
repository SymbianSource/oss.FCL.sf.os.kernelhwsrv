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
// e32test/usb\t_usb_device\src\activestallnotifier.cpp
// USB Test Program T_USB_DEVICE, functional part.
// Device-side part, to work against T_USB_HOST running on the host.
// 
//

#include "general.h"									// CActiveControl, CActiveRW
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "activestallnotifierTraces.h"
#endif
#include "activestallnotifier.h"

extern RTest test;
extern TBool gVerbose;
extern TBool gSkip;
extern TBool gStopOnFail;
extern TInt gSoakCount;

//
// --- class CActiveStallNotifier ---------------------------------------------------------
//

CActiveStallNotifier::CActiveStallNotifier(CConsoleBase* aConsole, RDEVCLIENT* aPort)
	: CActive(EPriorityNormal),
	  iConsole(aConsole),
	  iPort(aPort),
	  iEndpointState(0)
	{
	CActiveScheduler::Add(this);
	}

CActiveStallNotifier* CActiveStallNotifier::NewL(CConsoleBase* aConsole, RDEVCLIENT* aPort)
	{
	CActiveStallNotifier* self = new (ELeave) CActiveStallNotifier(aConsole, aPort);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();									// self
	return self;
	}


void CActiveStallNotifier::ConstructL()
	{}


CActiveStallNotifier::~CActiveStallNotifier()
	{
	TUSB_VERBOSE_PRINT("CActiveStallNotifier::~CActiveStallNotifier()");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVESTALLNOTIFIER_DCACTIVESTALLNOTIFIER, "CActiveStallNotifier::~CActiveStallNotifier()");
	    }
	Cancel();												// base class
	}


void CActiveStallNotifier::DoCancel()
	{
	TUSB_VERBOSE_PRINT("CActiveStallNotifier::DoCancel()");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVESTALLNOTIFIER_DOCANCEL, "CActiveStallNotifier::DoCancel()");
	    }
	iPort->EndpointStatusNotifyCancel();
	}


void CActiveStallNotifier::RunL()
	{
	// This just displays the bitmap, showing which endpoints (if any) are now stalled.
	// In a real world program, the user could take here appropriate action (cancel a
	// transfer request or whatever).
	TUSB_VERBOSE_PRINT1("StallNotifier: Endpointstate 0x%x\n", iEndpointState);
	if(gVerbose)
	    {
	    OstTrace1(TRACE_VERBOSE, CACTIVESTALLNOTIFIER_RUNL, "StallNotifier: Endpointstate 0x%x\n", iEndpointState);
	    }
	Activate();
	}


void CActiveStallNotifier::Activate()
	{
	__ASSERT_ALWAYS(!IsActive(), User::Panic(KActivePanic, 666));
	iPort->EndpointStatusNotify(iStatus, iEndpointState);
	SetActive();
	}



// -eof-
