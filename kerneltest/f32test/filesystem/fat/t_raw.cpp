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
// f32test\server\t_raw.cpp
//
//

#define __E32TEST_EXTENSION__

#include <f32file.h>
#include <e32test.h>
#include "t_server.h"

#include "fs_utils.h"
#include "fat_utils.h"
using namespace Fat_Test_Utils;


GLDEF_D RTest test(_L("T_RAW"));

LOCAL_D TInt gDrive;

LOCAL_C void Test1()
//
// Test all methods
//
	{

	test.Start(_L("Test all methods"));
	RRawDisk rd;
	TInt r=rd.Open(TheFs,gDrive);
	test_KErrNone(r);
	TBuf8<16> buf;
	r=rd.Read(0,buf);
	test_KErrNone(r);
	r=rd.Write(0,buf);
	test_KErrNone(r);
	rd.Close();
	test.End();
	}

LOCAL_C void Test2()
//
// Test cannot open a RRawDisk while other resources are open on it.
//
	{

	MakeFile(_L("TRAW.TST"));
	MakeDir(_L("\\F32-TST\\TRAW\\TRAWTEST\\"));

	RFile f;
	TInt r=f.Open(TheFs,_L("TRAW.TST"),EFileWrite);
	test_KErrNone(r);
	RRawDisk rd;
	r=rd.Open(TheFs,gDrive);
	test_Value(r, r == KErrInUse);
	f.Close();
	r=rd.Open(TheFs,gDrive);
	test_KErrNone(r);
	rd.Close();

	TFileName fileName;
	r=f.Temp(TheFs,_L(""),fileName,EFileWrite);
	test_KErrNone(r);
	r=rd.Open(TheFs,gDrive);
	test_Value(r, r == KErrInUse);
	f.Close();
	r=rd.Open(TheFs,gDrive);
	test_KErrNone(r);
	rd.Close();

	RDir d;
	r=d.Open(TheFs,_L("TRAWTEST"),KEntryAttNormal);
	test_KErrNone(r);
	r=rd.Open(TheFs,gDrive);
	test_Value(r, r == KErrInUse);
	d.Close();
	r=rd.Open(TheFs,gDrive);
	test_KErrNone(r);
	rd.Close();

    RFormat fmt;
    TInt count;
    r=fmt.Open(TheFs,gSessionPath,EQuickFormat,count);
    test_KErrNone(r);
    r=rd.Open(TheFs,gDrive); // Raw disk access ok during formatting
    test.Printf(_L("open rd when fmt opn r=%d"),r);
	test_Value(r, r == KErrInUse);
    fmt.Close();
    r=rd.Open(TheFs,gDrive);
	test.Printf(_L("open rd when fmt closed r=%d"),r);
    test_KErrNone(r);
	RRawDisk rd2;
	r=rd2.Open(TheFs,gDrive);	//should only have one Raw object open
	test_Value(r, r == KErrInUse);
	rd2.Close();
    rd.Close();
	}

LOCAL_C void Test3()
//
// Test cannot open resources on a disk while raw access is taking place
//
	{

	MakeFile(_L("TRAW.TST"));
	MakeDir(_L("TRAW"));

	RRawDisk rd;
	TInt r=rd.Open(TheFs,gDrive);
	test_KErrNone(r);

	RFile f;
	TFileName fileName;
	r=f.Open(TheFs,_L("TRAW.TST"),EFileWrite);
	test_Value(r, r == KErrInUse);
	r=f.Temp(TheFs,_L(""),fileName,EFileWrite);
	test_Value(r, r == KErrInUse);

	RDir d;
	r=d.Open(TheFs,_L("TRAW"),KEntryAttNormal);
	test_Value(r, r == KErrInUse);

	RFormat fmt;
	TInt count;
	r=fmt.Open(TheFs,gSessionPath,EQuickFormat,count);
    if (r != KErrInUse)
        test.Printf(_L("Error %d. Sess = %S"), r, &gSessionPath);
	test_Value(r, r == KErrInUse);

	CDir* dir=(CDir*)0x05;
	r=TheFs.GetDir(_L("\\F32-TST\\*.*"),KEntryAttNormal,ESortNone,dir);
	test_Value(r, r == KErrInUse);
	test(dir==NULL);

	RRawDisk rd2;
	r=rd2.Open(TheFs,gDrive);
	test_Value(r, r == KErrInUse);

//	fmt.Close();
	rd.Close();
//	rd2.Close();
	}

LOCAL_C void Test4()
//
// Test Read and Write
//
	{

	TBuf8<32> contents=_L8("This File says BOO");
	MakeFile(_L("TRAW.TST"),contents);

	RRawDisk rd;
	TInt r=rd.Open(TheFs,gDrive);
	test_KErrNone(r);

	TBuf8<32> textBuf;
	TInt64 pos=0;

	test.Printf(_L("doing rawread"));
	FOREVER
		{
		r=rd.Read(pos,textBuf);
		if (r!=KErrNone)
			{
			test.Printf(_L("ERROR: RawDisk read returned %d at pos %ld"),r, pos);
			test(0);
			//test.Getch();
			break;
			}
		if ((pos % (1024*1024)) == 0)
			test.Printf(_L("Read position 0x%x            \r"), pos);
		textBuf.SetLength(contents.Length());
		if (textBuf==contents)
			break;
		pos+=512;
		}
	test.Printf(_L("\n"));

	TBuf8<32> contents2=_L8("This File says MOO");
	r=rd.Write(pos,contents2);
	test_KErrNone(r);
	rd.Close();

	RFile f;
	r=f.Open(TheFs,_L("TRAW.TST"),EFileRead);
	test_KErrNone(r);
	r=f.Read(textBuf);
	test_KErrNone(r);
	test(textBuf==contents2);
	f.Close();
	}

enum TTestCommands
	{
	EThreadForgetToCloseSession,
	EThreadForgetToCloseRaw,
	EThreadHang
	};

RSemaphore gSemaphore;

TInt MyThreadFunction(TAny* aThreadCommand)
//
// Do nasty things
//
	{

	RRawDisk rd;
	RFs fs;
	TInt r=fs.Connect();
	if (r!=KErrNone)
		goto Error;
	r=rd.Open(fs,gDrive);
	if (r!=KErrNone)
		goto Error;

	switch((TTestCommands)(TInt)aThreadCommand)
		{
	case EThreadForgetToCloseSession:
		return(KErrNone);

	case EThreadForgetToCloseRaw:
		break;

	case EThreadHang:
		gSemaphore.Signal();
		FOREVER{};

	default:
		goto Error;
		}
	fs.Close();
	return(KErrNone);

Error:
	User::Panic(_L("Shouldn't be here!"),0);
	return(KErrNone);
	}

LOCAL_C void Test5()
//
// Test thread panics
//
	{

	test.Next(_L("Test thread panics"));
	MakeFile(_L("TEST.FILE"));

	RThread thread;
	TInt r=thread.Create(_L("MyThread"),MyThreadFunction,0x1000,0x1000,0x1000,(TAny*)EThreadForgetToCloseSession);
	test_KErrNone(r);
	TRequestStatus reqStat;
	thread.Logon(reqStat);
	thread.Resume();
	User::WaitForRequest(reqStat);
	CLOSE_AND_WAIT(thread);

	// We know the disconnect has been sent to the file server by this point
	// but we don't know it has been processed.
	// Connect and disconnect a session here to make sure.
	FsBarrier();

	RFile f;
	r=f.Open(TheFs,_L("TEST.FILE"),EFileWrite);
	test_KErrNone(r);
	f.Close();

	r=thread.Create(_L("MyThread"),MyThreadFunction,0x1000,0x1000,0x1000,(TAny*)EThreadForgetToCloseRaw);
	test_KErrNone(r);
	thread.Logon(reqStat);
	thread.Resume();
	User::WaitForRequest(reqStat);
	CLOSE_AND_WAIT(thread);

	// We know the disconnect has been sent to the file server by this point
	// but we don't know it has been processed.
	// Connect and disconnect a session here to make sure.
	FsBarrier();

	r=f.Open(TheFs,_L("TEST.FILE"),EFileWrite);
	test_KErrNone(r);
	f.Close();

	r=gSemaphore.CreateGlobal(_L("MySemaphore"),0);
	test_KErrNone(r);
	r=thread.Create(_L("MyThread"),MyThreadFunction,0x1000,0x1000,0x1000,(TAny*)EThreadHang);
	test_KErrNone(r);
	thread.Resume();
	gSemaphore.Wait();
	gSemaphore.Close();

	r=f.Open(TheFs,_L("TEST.FILE"),EFileWrite);
	test_Value(r, r == KErrInUse);
	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);
	thread.Kill(KErrGeneral);
	CLOSE_AND_WAIT(thread);
	User::SetJustInTime(jit);

	// We know the disconnect has been sent to the file server by this point
	// but we don't know it has been processed.
	// Connect and disconnect a session here to make sure.
	FsBarrier();

	r=f.Open(TheFs,_L("TEST.FILE"),EFileWrite);
	test_KErrNone(r);
	f.Close();
	}


GLDEF_C void CallTestsL()
//
// Do all tests
//
	{

    //-- set up console output
    Fat_Test_Utils::SetConsole(test.Console());

	TInt r=TheFs.CharToDrive(gSessionPath[0],gDrive);
	test_KErrNone(r);

    PrintDrvInfo(TheFs, gDrive);

    //-- check if this is FAT
    if(!Is_Fat(TheFs, gDrive))
    {
        test.Printf(_L("Skipping. This test requires FAT drive.\n"));
        return;
    }

    //-- format the drive.
    r = FormatFatDrive(TheFs, gDrive, ETrue);
    test_KErrNone(r);

	CreateTestDirectory(_L("\\F32-TST\\TRAW\\"));

	Test1();
	Test2();
	Test3();

	Test4(); // (silly test) Not valid on LFFS

    Test5();

	DeleteTestDirectory();
	}
