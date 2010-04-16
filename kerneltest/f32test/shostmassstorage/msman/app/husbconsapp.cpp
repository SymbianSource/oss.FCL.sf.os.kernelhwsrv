// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// USB Mass Storage Application - also used as an improvised boot loader mechanism
//
//



/**
 @file
*/

#include <e32cons.h>
#include <hal.h>
#include <f32file.h>

#include "rusbhostmsdevice.h"

#include <d32usbdi_hubdriver.h>
#include "usbtypes.h"
#include "rextfilesystem.h"
#include "cusbmsmountmanager.h"

#include "mdrivedisplay.h"
#include "cusbhostao.h"
#include "cusbhost.h"

#include "rusbotgsession.h"

#include "cdisplay.h"
#include "husbconsapp.h"
#include "tmslog.h"


_LIT(KTxtApp,"HOST USB CONSOLE APP");



CHeartBeat* CHeartBeat::NewLC(CDisplay& aDisplay)
	{
	CHeartBeat* me = new(ELeave) CHeartBeat(aDisplay);
	CleanupStack::PushL(me);
	me->ConstructL();
	return me;
	}


CHeartBeat::CHeartBeat(CDisplay& aDisplay)
:   CActive(0),
    iDisplay(aDisplay)
	{}


void CHeartBeat::ConstructL()
	{
	CActiveScheduler::Add(this);
	iTimer.CreateLocal();
	RunL();
	}


CHeartBeat::~CHeartBeat()
	{
	Cancel();
	}


void CHeartBeat::DoCancel()
	{
    iTimer.Cancel();
	}


void CHeartBeat::RunL()
	{
	SetActive();
	// Print RAM usage & up time
	iUpTime++;
    iDisplay.UpTime(iUpTime);

	TInt mem=0;
	if (HAL::Get(HALData::EMemoryRAMFree, mem)==KErrNone)
		{
        iDisplay.MemoryFree(mem);
		}
	iTimer.After(iStatus, 1000000);
	}


GLDEF_C void RunAppL()
    {
    __MSFNSLOG
    CActiveScheduler* sched = new(ELeave) CActiveScheduler;
    CleanupStack::PushL(sched);
    CActiveScheduler::Install(sched);

    RFs fs;
    User::LeaveIfError(fs.Connect());
    CleanupClosePushL(fs);

    RUsbOtgSession usbOtgSession;
    TInt err = usbOtgSession.Connect();
    User::LeaveIfError(err);

    CConsoleBase* console;
	console = Console::NewL(KTxtApp, TSize(KConsFullScreen,KConsFullScreen));
	CleanupStack::PushL(console);

    CDisplay* display = CDisplay::NewLC(fs, *console);
	CMessageKeyProcessor::NewLC(*display, usbOtgSession);
    CHeartBeat::NewLC(*display);

    display->Menu();
    display->DriveListL();


    CUsbHostDisp* usbHost = CUsbHostDisp::NewL(*display);
    CleanupStack::PushL(usbHost);

    usbHost->Start();

    // *************************************************************************
    // Start Active Scheduler
    // *************************************************************************
    CActiveScheduler::Start();

	// 1 sec delay for sessions to stop
	User::After(1000000);
    CleanupStack::PopAndDestroy(usbHost);
    CleanupStack::PopAndDestroy();  // CPeriodUpdate
    CleanupStack::PopAndDestroy();  // CMessageKeyProcessor
    CleanupStack::PopAndDestroy();  // CDisplay
    CleanupStack::PopAndDestroy(console);
    CleanupStack::PopAndDestroy();  // fs
    CleanupStack::PopAndDestroy(sched);
    }



GLDEF_C TInt E32Main()
	{
	__UHEAP_MARK;
	CTrapCleanup* cleanup = CTrapCleanup::New();
	TRAPD(error, RunAppL());
	__ASSERT_ALWAYS(!error, User::Panic(KTxtApp, error));
	delete cleanup;
	__UHEAP_MARKEND;
	return 0;
	}
