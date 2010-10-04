// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/domainmgr/t_domain_manual.cpp
//
// Overview:
// Domain Manager Manual test cases
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32ldr_private.h>

#include "t_domain.h"


RTest test(_L(" T_DOMAIN_MANUAL "));


class CDmShutdownTest : public MDmTest
	{
public:
	virtual ~CDmShutdownTest()
		{
		iManager.Close();
		}
	void Perform();
	void Release();
	TInt TransitionNotification(MDmDomainMember&)
		{
		test(0);
		return KErrNone;
		}
	void TransitionRequestComplete()
		{}
private:
	RDmDomainManager iManager;
	};


void CDmShutdownTest::Perform()
	{
	test.Next(_L("CDmShutdownTest"));

	// 1. Set up test hierarchy/domain & join it
	const TInt r = iManager.Connect();
	test_KErrNone(r);

	// 2. Call the Shutdown API
	iManager.SystemShutdown();

	test(0);	// Never reaches here!
	}


void CDmShutdownTest::Release()
	{
	delete this;
	}


///////////////////////////////////////////////////////////////////////////////
// --- Main() ---

GLDEF_C TInt E32Main()
	{
	CTrapCleanup* trapHandler = CTrapCleanup::New();
	test(trapHandler != NULL);

	CActiveScheduler* scheduler = new CActiveScheduler();
	test(scheduler != NULL);
	CActiveScheduler::Install(scheduler);

	// Turn off evil lazy dll unloading
	RLoader l;
	test(l.Connect() == KErrNone);
	test(l.CancelLazyDllUnload()== KErrNone);
	l.Close();

	test.Title();

	test.Start(_L("Test starting..."));

	// Remember the number of open handles. Just for a sanity check
	TInt start_thc, start_phc;
	RThread().HandleCount(start_phc, start_thc);

	MDmTest* tests[] =
		{
		// Always the last manual test as it shuts down the board/emulator
		new CDmShutdownTest(),
		};

	for (unsigned int i = 0; i < sizeof(tests)/sizeof(*tests); ++i)
		{
		test(tests[i] != NULL);
		tests[i]->Perform();
		tests[i]->Release();
		}

	test.End();

	// Sanity check for open handles and for pending requests
	TInt end_thc, end_phc;
	RThread().HandleCount(end_phc, end_thc);
	test(start_thc == end_thc);
	test(start_phc == end_phc);
	test(RThread().RequestCount() >= 0);

	delete scheduler;
	delete trapHandler;

	return KErrNone;
	}
