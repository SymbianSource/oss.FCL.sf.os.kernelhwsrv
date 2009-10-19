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
// f32test\fsstress\t_sess.cpp
//
//

#include "t_sess.h"

#include "t_server.h"
#include "t_chlffs.h"

GLDEF_D RTest test(_L("T_SESS"));
GLDEF_D TFileName gTestSessionPath;

// These objects are too large to be placed on the stack - ~15 sessions is enough
// to cause a stack overflow on ARM4.
const TInt KMaxNumberSessions=25;
RFs gSession[KMaxNumberSessions];
TSessionTest gTestObject[KMaxNumberSessions];


LOCAL_C void SetSessionPath(RFs aFs,TChar aDriveLetter)
//
//	Set the session path for a RFs connection to aDrive
//
	{
	gTestSessionPath=(_L("?:\\SESSION_TEST\\"));
	gTestSessionPath[0]=(TText)aDriveLetter;
	TInt r=aFs.SetSessionPath(gTestSessionPath);
	test(r==KErrNone);
	r=aFs.MkDirAll(gTestSessionPath);
	test(r==KErrNone || r==KErrAlreadyExists);
	}


GLDEF_C void CallTestsL()
//
//	This test makes a number of fileserver connections (sessions)
//	The fileserver is stressed by running these sessions concurrently, swapping
//	between them whilst testing various fileserver API functions
//	Further testing of the fileserver is performed by closing connections one by
//	one whilst ensuring their closure does not affect the other connected sessions
//
    {
	test.Title();

	TChar driveLetter;
	if (IsSessionDriveLFFS(TheFs,driveLetter))
		{
		test.Printf(_L("CallTestsL: Skipped: test does not run on LFFS.\n"));
		return;
		}

	test.Start(_L("Starting T_SESSION tests..."));

//	Create an array of fileserver sessions
//	Create an array of TSessionTest objects

	TInt i=0;

	TInt r;
	for (; i<KMaxNumberSessions; i++)
		{
		r=gSession[i].Connect();
		test(r==KErrNone);

		SetSessionPath(gSession[i],driveLetter);
		gSession[i].ResourceCountMarkStart();
		gTestObject[i].Initialise(gSession[i]);
		gTestObject[i].RunTests();	//	Run the set of tests for each session
		}							//	Leave each session open


	for (i=0; i<(KMaxNumberSessions-1); i++)
		{
	//	Alternate tests between open sessions
		gTestObject[i].testSetVolume();
		gTestObject[i+1].testInitialisation();
		gTestObject[i].testSubst();
		gTestObject[i+1].testInitialisation();
		gTestObject[i].testInitialisation();
		gTestObject[i].testDriveList();
		gTestObject[i+1].CopyFileToTestDirectory();
		gTestObject[i].MakeAndDeleteFiles();
	//	Close gSession[i] and check that session[i+1] is OK
		gSession[i].ResourceCountMarkEnd();
		gSession[i].Close();
		gTestObject[i+1].testInitialisation();
		gTestObject[i+1].testSetVolume();
		gTestObject[i+1].testInitialisation();
		gTestObject[i+1].testSubst();
		gTestObject[i+1].testDriveList();
	//	Reconnect gSession[i]
		r=gSession[i].Connect();
		test(r==KErrNone);
		SetSessionPath(gSession[i],driveLetter);
		gSession[i].ResourceCountMarkStart();
		gTestObject[i].Initialise(gSession[i]);
		gTestObject[i].testSetVolume();
		gTestObject[i+1].testInitialisation();
		gTestObject[i].testSubst();
		gTestObject[i+1].testInitialisation();
		gTestObject[i].testInitialisation();
		gTestObject[i].testDriveList();
	//	Close gSession[i+1] and check that session[i] is OK
		gSession[i+1].ResourceCountMarkEnd();
		gSession[i+1].Close();
		gTestObject[i].testInitialisation();
		gTestObject[i].testSetVolume();
		gTestObject[i].testInitialisation();
		gTestObject[i].testSubst();
		gTestObject[i].testDriveList();
	//	Reconnect gSession[i+1]
		r=gSession[i+1].Connect();
		test(r==KErrNone);
		SetSessionPath(gSession[i+1],driveLetter);
		gSession[i].ResourceCountMarkStart();
		gTestObject[i+1].Initialise(gSession[i+1]);
		gTestObject[i].testSetVolume();
		gTestObject[i+1].testInitialisation();
		gTestObject[i].testSubst();
		gTestObject[i+1].testInitialisation();
	//	Close session[i] and check that session[i+1] is OK
		gSession[i].ResourceCountMarkEnd();
		gSession[i].Close();
		gTestObject[i+1].testInitialisation();
		gTestObject[i+1].testSetVolume();
		gTestObject[i+1].testInitialisation();
		gTestObject[i+1].testSubst();
		gTestObject[i+1].testDriveList();

		if (i==KMaxNumberSessions-1)	//	Tidy up by closing remaining open session
			{
			gSession[i].ResourceCountMarkEnd();
			gSession[i+1].Close();
			}
		}

//	Set up the arrays again and open sessions ready for more testing

	for (i=0; i<KMaxNumberSessions; i++)
		{
		r=gSession[i].Connect();
		test(r==KErrNone);
		SetSessionPath(gSession[i],driveLetter);
		gSession[i].ResourceCountMarkStart();
		gTestObject[i].Initialise(gSession[i]);
		}


	for (i=0; i<KMaxNumberSessions-1; i++)
		{
		gTestObject[i].testInitialisation();
		gTestObject[i+1].testSubst();
		gTestObject[i].testDriveList();
		gSession[i].ResourceCountMarkEnd();
		gSession[i].Close();
		gTestObject[i+1].testInitialisation();
		gTestObject[i+1].testSetVolume();
		if (i==KMaxNumberSessions-1)	//	Tidy up by closing remaining open session
			{
			gSession[i+1].ResourceCountMarkEnd();
			gSession[i+1].Close();
			}
		}


	test.End();
	test.Close();
	return;
    }
