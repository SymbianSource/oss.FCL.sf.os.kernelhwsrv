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
// f32test\fsstress\t_rmain.cpp
// 
//

#if !defined(__F32FILE_H__)
#include <f32file.h>
#endif
#if !defined(__E32TEST_H__)
#define	__E32TEST_EXTENSION__
#include <e32test.h>
#endif
#if !defined(__E32HAL_H__)
#include <e32hal.h>
#endif
#if !defined(__E32MATH_H__)
#include <e32math.h>
#endif
#if !defined(__F32DBG_H__)
#include <f32dbg.h>
#endif
#include "t_std.h"


GLDEF_D	RFs TheFs;
GLDEF_D TFileName gSessionPath;
GLDEF_D TInt gAllocFailOff=KAllocFailureOff;
GLDEF_D TInt gAllocFailOn=KAllocFailureOff;
GLDEF_D TInt64 gSeed=51703;
const TInt KHeapSize=0x2000;



GLDEF_C void TurnAllocFailureOff()
//
// Switch off all allocFailure
//
	{

	test.Printf(_L("Disable Alloc Failure\n"));
	TheFs.SetAllocFailure(gAllocFailOff);
	gAllocFailOn=KAllocFailureOff;
	}

GLDEF_C void TurnAllocFailureOn()
//
// Switch off all allocFailure
//
	{

	test.Printf(_L("Enable Alloc Failure\n"));
	gAllocFailOn=KAllocFailureOn; 
	TheFs.SetAllocFailure(gAllocFailOn);
	}


GLDEF_C void Format(TInt aDrive)
//
// Format current drive
//
	{

	test.Next(_L("Format"));
	TBuf<4> driveBuf=_L("?:\\");
	driveBuf[0]=(TText)(aDrive+'A');
	RFormat format;
	TInt count;
	TInt r=format.Open(TheFs,driveBuf,EHighDensity,count);
	test_KErrNone(r);
	while(count)
		{
		TInt r=format.Next(count);
		test_KErrNone(r);
		}
	format.Close();
	}

LOCAL_C void PushLotsL()
//
// Expand the cleanup stack
//
	{
	TInt i;
	for(i=0;i<1000;i++)
		CleanupStack::PushL((CBase*)NULL);
	CleanupStack::Pop(1000);
	}

	
LOCAL_C void DoTests(TInt aDrive)
//
// Do testing on aDrive
//
	{

	gSessionPath=_L("?:\\F32-TST\\");
	TChar driveLetter;
	TInt r=TheFs.DriveToChar(aDrive,driveLetter);
	test_KErrNone(r);
	gSessionPath[0]=(TText)driveLetter;
	r=TheFs.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	r=TheFs.MkDirAll(gSessionPath);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	TheFs.ResourceCountMarkStart();
	
	switch(aDrive)
		{
#if defined (__MARM__)
	case EDriveC:
		TRAP(r,CallTestsYL(aDrive));
		break;
	case EDriveD:
		TRAP(r,CallTestsXL(aDrive));
		break;
#else
	case EDriveX:
		TRAP(r,CallTestsXL(aDrive));
		break;
	case EDriveY:
		TRAP(r,CallTestsYL(aDrive));
		break;
#endif
	case EDriveQ:
		TRAP(r,CallTestsQL(aDrive));
		break;
	default:
		TRAP(r,CallTestsDefaultL(aDrive));	
		break;
		}
	
	if (r==KErrNone)
		TheFs.ResourceCountMarkEnd();
	else
		{
		test.Printf(_L("Error: Leave %d\n"),r);
		test.Getch();
		}
	}
		

LOCAL_C TInt TestXDrive(TAny * /*anArg*/)
//
//	MARM CF card drive testing (WINS emulates CF card on X)
//
	{
	CTrapCleanup* cleanup;
	cleanup=CTrapCleanup::New();

	testx.Title();

	TInt r=TheFs.Connect();
	testx(r==KErrNone);
//	TheFs.SetAllocFailure(gAllocFailOn);
	
#if defined (__WINS__)

//	Run tests on WINS drive X:	
	testx.Start(_L("Starting tests on drive X:"));
	DoTests(EDriveX);
	
#elif defined (__MARM__)
//	Run tests on MARM drive D:
	testx.Start(_L("Starting tests on drive D:"));
	DoTests(EDriveD);
#endif

//	TheFs.SetAllocFailure(gAllocFailOff);
	delete cleanup;
	testx.End();
	
	return KErrNone;
	}


LOCAL_C TInt TestYDrive(TAny * /*anArg*/)
//
//	MARM RAM drive testing (WINS emulates FAT filesystem on Y)  		
//
	{
	CTrapCleanup* cleanup;
	cleanup=CTrapCleanup::New();

	testy.Title();
	TInt r=TheFs.Connect();
	testy(r==KErrNone);

//	TheFs.SetAllocFailure(gAllocFailOn);

#if defined (__WINS__)

//	Run tests on WINS drive Y:
	testy.Start(_L("Starting tests on drive Y:"));
	DoTests(EDriveY);

#elif defined (__MARM__)

//	Run tests on MARM drive C:
	testy.Start(_L("Starting tests on drive C:"));
	DoTests(EDriveC);
#endif
	
//	TheFs.SetAllocFailure(gAllocFailOff);	
	delete cleanup;
	testy.End();
	return KErrNone;
	}


LOCAL_C TInt TestRemoteDrive(TAny * /*anArg*/)
//
// Run tests on remote drive Q:
//
	{	
	CTrapCleanup* cleanup;
	cleanup=CTrapCleanup::New();

	testq.Title();
	testq.Start(_L("Starting async tests..."));

	TInt r=TheFs.Connect();
	testq(r==KErrNone);
//	TheFs.SetAllocFailure(gAllocFailOn);

	testq.Next(_L("Mount Remote Drive simulator on Q:"));
	
	r=TheFs.AddFileSystem(_L("T_REMFSY"));
	testq.Next(_L("Add remote file system"));
	testq.Printf(_L("AddFileSystem returned %d\n"),r);
//	testq(r==KErrNone || r==KErrAlreadyExists);

	r=TheFs.MountFileSystem(_L("T_REMFSY"),EDriveQ);
	testq.Next(_L("Mount remote file system"));
	testq.Printf(_L("MountFileSystem returned %d\n"),r);
	testq(r==KErrNone || r==KErrCorrupt || r==KErrNotReady || r==KErrAlreadyExists);
	if (r==KErrCorrupt || r==KErrNotReady)
		Format(EDriveQ);
	
	DoTests(EDriveQ);
//	TheFs.SetAllocFailure(gAllocFailOff);
	
	delete cleanup;
	testq.End();
	return KErrNone;
	}


GLDEF_C TInt E32Main()
//
// Test with drive nearly full
//
    {
	CTrapCleanup* cleanup;
	cleanup=CTrapCleanup::New();
	TRAPD(r,PushLotsL());
 	__UHEAP_MARK;

	test.Title();
	test.Start(_L("Starting tests..."));
	r=TheFs.Connect();
	test_KErrNone(r);
//	TheFs.SetAllocFailure(gAllocFailOn);

//	Default drive testing
	DoTests(KDefaultDrive);	

//	Remote drive testing
	RThread clientThreadQ;
	r=clientThreadQ.Create(_L("TestRemoteDrive"), TestRemoteDrive, KDefaultStackSize,KHeapSize,KHeapSize,NULL);	
	test_KErrNone(r);	
	TRequestStatus statq;
	clientThreadQ.Logon(statq);
	test.Next(_L("Resume clientThreadQ"));	
	clientThreadQ.Resume();

//	MARM CF card drive testing (WINS emulates CF card on X)
	RThread clientThreadX;
	r=clientThreadX.Create(_L("TestXDrive"), TestXDrive, KDefaultStackSize,KHeapSize,KHeapSize,NULL);	
	test_KErrNone(r);	
	TRequestStatus statx;
	clientThreadX.Logon(statx);
	test.Next(_L("Resume clientThreadX"));	
	clientThreadX.Resume();

//	MARM RAM drive testing (WINS emulates FAT filesystem on Y)  	
	RThread clientThreadY;
	r=clientThreadY.Create(_L("TestYDrive"), TestYDrive, KDefaultStackSize,KHeapSize,KHeapSize,NULL);	
	test_KErrNone(r);	
	TRequestStatus staty;
	clientThreadY.Logon(staty);
	test.Next(_L("Resume clientThreadY"));	
	clientThreadY.Resume();

	User::WaitForRequest(statx);
	User::WaitForRequest(staty);
	User::WaitForRequest(statq);

	clientThreadQ.Close();
	clientThreadX.Close();
	clientThreadY.Close();

//	TheFs.SetAllocFailure(gAllocFailOff);
	TheFs.Close();
	  
	test.End();
	test.Close();
	__UHEAP_MARKEND;
	delete cleanup;
	return(KErrNone);
    }
