// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\window\t_mmcpw.cpp
// Tests MMC card password notifier
// 
//

#include <e32svr.h>

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32twin.h>
#include <e32def.h>
#include <e32def_private.h>
#include <e32std.h>
#include <e32std_private.h>

static RTest test(_L("T_MMCPW"));

GLDEF_C TInt E32Main()
	{
	test.Start(_L("E32Main"));

	test.Next(_L("Creating notifier."));
	RNotifier n;
	
	test.Next(_L("Connected to notify server."));
	TInt r = n.Connect();
	test_KErrNone(r);

	TPckgBuf<TMediaPswdSendNotifyInfoV1> send;
	send().iVersion = TVersion(1, 0, 0);
	TPckgBuf<TMediaPswdReplyNotifyInfoV1> reply;
	reply().iVersion = TVersion(1, 0, 0);

	test.Next(_L("Launching notify server."));
	TRequestStatus rs;
	n.StartNotifierAndGetResponse(rs, TUid::Uid(KMediaPasswordNotifyUid), send, reply);
	
	test.Next(_L("Waiting for dialog to respond."));
	User::WaitForRequest(rs);

	test.Next(_L("Reading exit mode and resultant password."));
	test(reply().iEM == EMPEMUnlock || reply().iEM == EMPEMCancel || reply().iEM == EMPEMUnlockAndStore);
	test.Printf(_L("reply().iEM = %d(d).\n"), TInt(reply().iEM));

	switch (reply().iEM)
		{
	case EMPEMUnlock:
	case EMPEMUnlockAndStore:
		{
		if (reply().iEM == EMPEMUnlock)
			test.Printf(_L("EMPEMUnlock selected.\n"));
		else
			test.Printf(_L("EMPEMUnlockAndStore selected.\n"));

		TMediaPassword pw;
		pw.Copy(reply().iPW, KMaxMediaPassword);

		TInt i;									// bad for-scope under VC

		for (i = 0; i < KMaxMediaPassword; i++)
			test.Printf(_L("%02x "), pw[i]);
		test.Printf(_L("\n"));

		for (i = 0; i < KMaxMediaPassword; i++)
			test.Printf(_L("%02x "), i);
		test.Printf(_L("\n"));
		}
		break;

	case EMPEMCancel:
		test.Printf(_L("EMPEMCancel selected.\n"));
		break;
		}

	test.Next(_L("Closing notifier."));
	n.Close();

	test.Getch();

	test.End();
	
	return KErrNone;
    }

