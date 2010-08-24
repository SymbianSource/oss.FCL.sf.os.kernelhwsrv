// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\device\t_usb.cpp
// USB Test Program, main part.
// Device-side part, to work against USBRFLCT running on the host.
// 
//

#include "t_usb.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "t_usbTraces.h"
#endif


void RunAppL(TBool aVerboseOutput)
	{
	OstTrace0(TRACE_NORMAL, RUNAPPL_RUNAPPL, "RunAppL()");
	// Construct the active scheduler
	CActiveScheduler* myScheduler = new (ELeave) CActiveScheduler();

	// Push active scheduler onto the cleanup stack
	CleanupStack::PushL(myScheduler);

	// Install as the active scheduler
	CActiveScheduler::Install(myScheduler);

	// Create console handler
	CConsoleBase* myConsole =
		Console::NewL(_L("T_USB - USB Client Test Program"), TSize(KConsFullScreen, KConsFullScreen));
	CleanupStack::PushL(myConsole);

	CActiveConsole* myActiveConsole = CActiveConsole::NewL(myConsole, aVerboseOutput);
	CleanupStack::PushL(myActiveConsole);

	// Call request function
	myActiveConsole->RequestCharacter();

	// Start active scheduler
	CActiveScheduler::Start();

	// Suspend thread for 2 secs
	User::After(2000000);

	CleanupStack::PopAndDestroy(3);							// myActiveConsole, myConsole, myScheduler
	return;
	}


TInt E32Main()
	{
	OstTrace0(TRACE_NORMAL, E32MAIN_E32MAIN, "E32Main()");

	CTrapCleanup* cleanup = CTrapCleanup::New();			// get clean-up stack

	__UHEAP_MARK;

	_LIT(KArg, "verbose");
	TBuf<64> c;
	User::CommandLine(c);
	TBool verbose = EFalse;
	if (c.CompareF(KArg) == 0)
		{
		OstTrace0(TRACE_NORMAL, E32MAIN_E32MAIN_DUP01, "(Verbose output enabled.)\n");
		verbose = ETrue;
		}

	TRAPD(error, RunAppL(verbose));

	__ASSERT_ALWAYS(!error, User::Panic(_L("T_USB: EPOC32EX"), error));

	__UHEAP_MARKEND;

	delete cleanup;											// destroy clean-up stack

	OstTrace0(TRACE_NORMAL, E32MAIN_E32MAIN_DUP02, "Program exit: done.\n");

	return 0;												// and return
	}


// -eof-
