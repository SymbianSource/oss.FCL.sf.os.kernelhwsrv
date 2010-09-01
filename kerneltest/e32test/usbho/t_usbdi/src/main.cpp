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
// @file main.cpp
// @internalComponent
// 
//

#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>

#include "testdebug.h"
#include "TestEngine.h"

using namespace NUnitTesting_USBDI;

// The test object

RTest gtest(_L("USBDI Unit Testing"));

static void MainL()
	{
	LOG_CFUNC
	// Leave the hooks in for platform security
#ifdef __DATA_CAGING__
	RProcess().DataCaging(RProcess::EDataCagingOn);
	RProcess().SecureApi(RProcess::ESecureApiOn);
#endif		

	// Identify the process and main thread
	RProcess testProcess;
	RThread().Process(testProcess);
	testProcess.RenameMe(_L("t_usbdi.exe"));
	RThread().RenameMe(_L("t_usbdi.exe main thread"));

	// Create a new active scheduler for this main thread
	CActiveScheduler* sched = new (ELeave) CActiveScheduler;
	CleanupStack::PushL(sched);
	CActiveScheduler::Install(sched);
	
	// create test engine for host or client depending upon command line arguments.
	CTestEngine* testEngine = NULL;
	TRAPD(err, testEngine = CTestEngine::NewL());
	if(err == KErrNone)
		{
		CleanupStack::PushL(testEngine);
			
		// Synchronise with the client and start the active scheduler
		RProcess::Rendezvous(KErrNone);

        User::After(150000);
		RDebug::Print(_L("CActiveScheduler::Start MainL"));	
		CActiveScheduler::Start();
		
		CleanupStack::PopAndDestroy(testEngine);
		}
	else
		{
		gtest.Printf(_L("Unable to create the test engine: %d\n"),err);
		}
		
	User::After(5000000);
	CleanupStack::PopAndDestroy(sched);		
	}

TInt E32Main()
	{
	LOG_CFUNC
	// Create the new trap-cleanup mechanism
	CTrapCleanup* cleanup = CTrapCleanup::New();
	
	if(cleanup == NULL)
		{
		return KErrNoMemory;
		}
		
	// Perform the tests
	TRAPD(err,MainL());
	if(err != KErrNone)
		{
		gtest.Printf(_L("MainL error: %d\n"),err);		
		}
	
	delete cleanup;
	
	// Provide no error
	return KErrNone;
	}


