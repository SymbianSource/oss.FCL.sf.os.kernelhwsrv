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
// e32test/domainmgr/t_dmslave_onecap.cpp
//
//

#include <e32test.h>
#include <domainmember.h>

#include "domainpolicytest.h"


RTest test(_L(" T_DMSLAVE_ONECAP "));


GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Call DeferAcknowledgement() with one capability"));

	// Steps 1. & 2. are in the parent process

	// 3. Connect to domain
	RDmDomain domain;
	TInt r = domain.Connect(KDmHierarchyIdTestV2, KDmIdTestA);
	test(r == KErrNone);

	// 4. Request transition notification
	TRequestStatus status;
	test.Printf(_L("Requesting transition notification"));
	domain.RequestTransitionNotification(status);

	// Tell parent we're done
	RProcess().Rendezvous(KErrNone);

	// Step 5. is in the parent process

	User::WaitForRequest(status);
	test(status.Int() == KErrNone);

	// 6. Try to defer acknowledgement (this is the sole purpose of this test)
	test.Printf(_L("Requesting acknowledgement deferral\n"));
	domain.DeferAcknowledgement(status);
	User::WaitForRequest(status);
	test(status.Int() == KErrNone);

	// 7. Acknowledge
	test.Printf(_L("Acknowledging last state\n"));
	domain.AcknowledgeLastState();

	test.End();

	return KErrNone;
	}
