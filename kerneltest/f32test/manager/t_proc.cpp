// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\manager\t_proc.cpp
// 
//

#include <f32file.h>
#include <e32test.h>
#include <e32svr.h>
#include <hal.h>
#include "t_server.h"
#include "t_chlffs.h"

GLDEF_D RTest test(_L("T_PROC"));

LOCAL_C TInt ThreadMain(TAny* /*aPtr*/)
//
// Startup function for thread
//
	{

	RFs localFs;
	TInt r=localFs.Connect();
	if (r != KErrNone)
		User::Panic(_L("tm0"), r);
	
	r=localFs.SetSessionPath(gSessionPath);
	if (r != KErrNone)
		User::Panic(_L("tm1"), r);

	TInt count;
	RFormat f;
#if defined(__MARM__)
	TBuf<4> dirBuf=_L("?:\\");
	dirBuf[0] = (TText)gDriveToTest;
	r=f.Open(localFs,dirBuf,EQuickFormat,count);
#else	
	r=f.Open(localFs,_L("Y:\\"),EQuickFormat,count);
#endif
	if (r != KErrNone)
		User::Panic(_L("tm2"), r);
	f.Close();

	TInt drv;
	r = RFs::CharToDrive(gSessionPath[0], drv);
	if (r != KErrNone)
		User::Panic(_L("tm3"), r);


	RRawDisk raw;
	r=raw.Open(localFs,drv);
	if (r != KErrNone)
		User::Panic(_L("tm4"), r);

	raw.Close();
	return(KErrNone);
	}

LOCAL_C void testThreads()
//
//	Run a process that opens a file, format, directory and check that when 
//	the process closes they are all still accessible.
//
	{

	RThread thread;
	TInt r=thread.Create(_L("Thread1"),ThreadMain,KDefaultStackSize,KMinHeapSize,0x10000,NULL);
	test(r==KErrNone);
	TRequestStatus reqStat;
	thread.Logon(reqStat);
	thread.Resume();
	thread.Close();
	User::WaitForRequest(reqStat);
//
#if defined(__MARM__)
	TBuf<13> dirBuf=_L("?:\\F32-TST\\");
	dirBuf[0] = (TText)gDriveToTest;
	r=TheFs.MkDir(dirBuf);
#else
	r=TheFs.MkDir(_L("Y:\\F32-TST\\"));
#endif
	test(r==KErrNone || r==KErrAlreadyExists);
	RFile f;	

	r=f.Open(TheFs,_L("THREADFILE.TEST"),EFileRead);
	test(r==KErrNotFound || r==KErrPathNotFound || r==KErrNone);
	f.Close();

	r=f.Replace(TheFs,_L("THREADFILE.TEST"),EFileRead|EFileWrite);
	test(r==KErrNone);
	f.Close();
	}

LOCAL_C void DoTests()
//
// Run tests
//	
	{
//	TInt r=TheFs.SessionPath(gSessionPath);
//	test(r==KErrNone);
	testThreads();
	}


GLDEF_C void CallTestsL(void)
//
// Test the file server.
//
    {
	
	test.Title();

	TChar driveLetter;
	if (IsSessionDriveLFFS(TheFs,driveLetter))
		{
		test.Printf(_L("Skipped: test does not run on LFFS.\n"));
		return;
		}
	test.Start(_L("Starting T_PROC test"));
	TInt uid;
	test(HAL::Get(HAL::EMachineUid,uid)==KErrNone);
	
	if(uid==HAL::EMachineUid_Cogent || uid==HAL::EMachineUid_IQ80310 || uid==HAL::EMachineUid_Integrator || uid==HAL::EMachineUid_X86PC)
		{
		test.Printf(_L("WARNING: d: not tested on cogent or IQ80310 or Integrator\n"));
		goto End;
		}
	DoTests();
End:
	test.End();
	return;
    }

