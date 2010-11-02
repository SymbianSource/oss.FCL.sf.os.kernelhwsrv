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
// f32test\server\t_notify.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32svr.h>
#include <hal.h>
#include "t_server.h"
#include "f32_test_utils.h"

using namespace F32_Test_Utils;

const TInt KHeapSize=0x200;

// wait for a bit since NotifyChange handled asynchronously and SetDriveName synchronously
const TInt KNotifyChangeAfter=100000;
const TInt KMediaRemountForceMediaChange = 0x00000001;

TInt gDriveNum = -1;

RTest test(_L("T_NOTIFY"));
RSemaphore gSleepThread;
TInt gSocketNumber=0;
TInt SocketToDrive[KMaxPBusSockets];

//#define __INCLUDE_MANUAL_TESTS__

void do_check_no_pending_requests(TInt aLine)
	{
	RTimer timer;
	TRequestStatus timerStat;
	timer.CreateLocal();
	timer.After(timerStat, 125000);
	User::WaitForAnyRequest();
	if(timerStat==KRequestPending)
		{
		RDebug::Printf("do_check_no_pending_requests failed at line %d",aLine);
		test(0);
		}
	timer.Close();
	}
#define CHECK_NO_PENDING_REQUESTS do_check_no_pending_requests(__LINE__);


TBool CheckDriveRead(TInt aDrive)
  //Determines if can connect to local drive number and read
	{
    TBusLocalDrive TBLD;
    TBool TBLDChangedFlag;

    TInt r = TBLD.Connect(aDrive, TBLDChangedFlag);
	test.Printf(_L("Connect returned %d\n"), r);
    if (r == KErrNone)
		{
        const TInt KSectSize = 512;
        TBuf8<KSectSize> sect;
        r = TBLD.Read(0, KSectSize, sect);
		test.Printf(_L("Read returned %d\n"), r);
        TBLD.Disconnect();
		if(r!=KErrNone)
			return EFalse;
		else
			return ETrue;
        }
	 else
		return EFalse;
	}

void GenerateMediaChange()
	{
    TPckgBuf<TInt> pckg;
    pckg()=0;

	RFs fs;
	TInt r = fs.Connect();
	if (r == KErrNone)
		{
		r = fs.RemountDrive(CurrentDrive(), &pckg, (TUint)KMediaRemountForceMediaChange);
		if(r == KErrNotReady)
			{
			r = KErrNone;
			}
		fs.Close();
		}

	if (r!=KErrNone)
		{
		RProcess me;
		me.Panic(_L("GenMedChg"),r);
		}
	}

enum TTestCode {ETest1,ETest2,ETest3,ETest4,ETest5,ETest6,ETest7,ETest8,ETest9,ETest10,ETest11,ETest12};
const TUint KGenericEntryChange=0x02;

static TInt ThreadEntryPoint(TAny* aTestCode)
//
// Thread entry point
//
	{
	RFs fs;
	TInt r=fs.Connect();
	test_KErrNone(r);
	r=fs.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	TTestCode testCode=*(TTestCode*)&aTestCode;
	RFile f;
	switch (testCode)
		{

	case ETest1:
		r=f.Replace(fs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileStream);
		test_KErrNone(r);
		f.Close();
		break;

	case ETest2:
		r=f.Replace(fs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),EFileStream);
		test_KErrNone(r);
		f.Close();
		break;

	case ETest3:
		r=fs.MkDir(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\SCARECROW\\"));
		test_Value(r, (r == KErrNone)||(r==KErrAlreadyExists));
		break;

	case ETest4:
		{
		TRequestStatus s;
		fs.NotifyChange(ENotifyAll,s);
		test_Value(s.Int(), s==KRequestPending);
		gSleepThread.Signal();
		User::After(100000000);
		}
		break;

	case ETest5:
		{
		RFile file;
		TInt r=file.Open(fs,_L("\\F32-TST\\NOTIFY\\kangaroo.txt"),EFileRead|EFileWrite);
		test_KErrNone(r);
		r=file.SetSize(sizeof(TCheckedUid));
		test_KErrNone(r);
		r=file.Write(sizeof(TCheckedUid),_L8("012345678912"));
		test_KErrNone(r);
		TBuf8<64> dum;
		r=file.Read(0,dum);
		test_KErrNone(r);
		file.Close();

		r=file.Open(fs,_L("\\F32-TST\\NOTIFY\\koala.txt"),EFileRead|EFileWrite);
		test_KErrNone(r);
		r=file.SetSize(50);
		test_KErrNone(r);
		r=file.Write(sizeof(TCheckedUid),_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
		test_KErrNone(r);
		r=file.Read(0,dum);
		test_KErrNone(r);
		file.Close();

		r=file.Open(fs,_L("\\F32-TST\\NOTIFY\\dingo.txt"),EFileRead|EFileWrite);
		test_KErrNone(r);
		r=file.SetSize(50);
		test_KErrNone(r);
		r=file.Write(sizeof(TCheckedUid),_L8("01234567890123456789"));
		test_KErrNone(r);
		r=file.Read(0,dum);
		test_KErrNone(r);
		file.Close();
		gSleepThread.Signal();
		}
		break;

	case ETest6:
		{
		GenerateMediaChange();
		User::After(300000);			// Wait for a bit
		gSleepThread.Signal();
		}
		break;

	case ETest7:
		{
		RFile file;
		TInt r=file.Open(fs,_L("\\F32-TST\\NOTIFY\\NewFILE.TXT"),EFileRead|EFileWrite);
		test_KErrNone(r);
		r=file.Write(_L8("asdfasdfasdf"));
		test_KErrNone(r);
		file.Close();
		gSleepThread.Signal();
		}
		break;

	case ETest8:
		{
		r=f.Open(fs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\WickedWitch.msg"),EFileRead|EFileWrite);
		test_KErrNone(r);
		f.SetSize(500);
		f.Close();
		break;
		}

	case ETest9:
		{
		TRequestStatus s;
		TFileName path=_L("\\F32-TST\\NOTIFY\\");
		fs.NotifyChange(ENotifyAll,s,path);
		test_Value(s.Int(), s==KRequestPending);
		gSleepThread.Signal();
		User::After(100000000);
		}
		break;
	case ETest10:
		{
		TFileName path=_L("\\F32-TST\\NOTIFY\\BehindTheCurtain\\");
		r=fs.MkDir(path);
		test_KErrNone(r);
		break;
		}
	case ETest11:
		{
		TFileName path=_L("\\F32-TST\\NOTIFY\\BehindTheCurtain\\PayNoAttention.man");
		RFile file;
		r=file.Replace(fs,path,EFileStream);
		test_KErrNone(r);
		file.Close();
		break;
		}
	case ETest12:
		{
		RFile writer;
		TInt r=writer.Open(fs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileWrite|EFileShareAny);
		test_KErrNone(r);
		TInt i;
		for(i=0; i<10; i++)
			{
			r=writer.Write(_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
			test_KErrNone(r);
			User::After(1000000);
			}
		writer.Close();
		break;
		}
	default:
		break;
		}
	return KErrNone;
	}

#if defined (__EPOC32__)//we have no removable media on Emulator yet
static void WaitForMediaChange()
//
// Wait for media driver to register drive is present
//
	{

	TEntry entry;
	FOREVER
		{
		User::After(10000);
		TInt r=TheFs.Entry(_L("xxxxxxxx"),entry);
		if (r!=KErrNotReady)
			break;
		}
	}
#endif

static void Test1()
//
// Test notification of an entry change
//
	{

	test.Next(_L("Test notification of an entry change"));
	TRequestStatus reqStat(KRequestPending);
	TRequestStatus thrdStat(KRequestPending);
	TInt r;
	TheFs.NotifyChange(ENotifyEntry,reqStat);
	RThread thread;
	r=thread.Create(_L("MyThread"),ThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETest1);
	test_KErrNone(r);
	thread.Logon(thrdStat);
	thread.Resume();
	User::WaitForRequest(thrdStat);
	test_KErrNone(thrdStat.Int());
	thread.Close();
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

	RFile file;
	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileWrite|EFileShareExclusive);
	test_KErrNone(r);
	file.Write(_L8("Somewhere over the rainbow..."),reqStat);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	TBuf8<256> buf;
	file.Read(0, buf,reqStat);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	file.Close();

	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileReadAsyncAll|EFileShareExclusive);
	test_Value(r, r == KErrArgument);

	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileReadAsyncAll|EFileShareReadersOnly);
	test_Value(r, r == KErrArgument);

	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileReadAsyncAll|EFileShareAny);
	test_KErrNone(r);
	file.Read(0, buf, 100, reqStat);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	file.Close();
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);

	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileReadAsyncAll|EFileShareAny);
	test_KErrNone(r);
	file.Read(0, buf, 100, reqStat);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	file.ReadCancel(reqStat);
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);
	file.Close();

	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileReadAsyncAll|EFileWrite|EFileShareAny);
	test_KErrNone(r);
	file.Read(0, buf, 100, reqStat);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	file.SetSize(100);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	test_Equal(100, buf.Length());
	file.Close();

	test.Next(_L("Repeat Test notification of an entry change"));
	TheFs.NotifyChange(ENotifyEntry,reqStat);
	thread.Create(_L("MyThread2"),ThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETest1);
	thread.Logon(thrdStat);
	thread.Resume();
	User::WaitForRequest(thrdStat);
	test_KErrNone(thrdStat.Int());
	thread.Close();
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

	test.Next(_L("Test Notify cancel"));
	TheFs.NotifyChange(ENotifyEntry,reqStat);
	TheFs.NotifyChangeCancel();
	User::WaitForRequest(reqStat);

	test.Next(_L("Test notification still works"));
	TheFs.NotifyChange(ENotifyEntry,reqStat);
	thread.Create(_L("MyThread3"),ThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETest1);
	thread.Logon(thrdStat);
	thread.Resume();
	User::WaitForRequest(thrdStat);
	test_KErrNone(thrdStat.Int());
	thread.Close();
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	}

static void Test2()
//
// Test notify for multiple clients
//
	{

	test.Next(_L("Test notification of multiple clients"));

	TRequestStatus reqStat1(KRequestPending);
	RFs fs1;
	TInt r=fs1.Connect();
	test_KErrNone(r);
	fs1.NotifyChange(ENotifyEntry,reqStat1);

	TRequestStatus reqStat2(KRequestPending);
	RFs fs2;
	r=fs2.Connect();
	test_KErrNone(r);
	fs2.NotifyChange(ENotifyEntry,reqStat2);

	test_Value(reqStat1.Int(), reqStat1==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\NEWFILE.TXT"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat1);
	User::WaitForRequest(reqStat2);
	test_KErrNone(reqStat1.Int());
	test_KErrNone(reqStat2.Int());
	}

static void Test3()
//
// Test notify cancel
//
	{

	test.Next(_L("Cancel notification"));
	RFs fs1;
	TInt r=fs1.Connect();
	test_KErrNone(r);

	TRequestStatus status1;
	TRequestStatus status2;
	TRequestStatus status3;
	TRequestStatus status4;
	TRequestStatus status5;

	fs1.NotifyChange(ENotifyAll,status1);
	fs1.NotifyChange(ENotifyAll,status2);
	fs1.NotifyChange(ENotifyAll,status3);
	fs1.NotifyChange(ENotifyAll,status4);
	fs1.NotifyChange(ENotifyAll,status5);
	test_Value(status1.Int(), status1==KRequestPending);
	test_Value(status2.Int(), status2==KRequestPending);
	test_Value(status3.Int(), status3==KRequestPending);
	test_Value(status4.Int(), status4==KRequestPending);
	test_Value(status5.Int(), status5==KRequestPending);

	test.Next(_L("RFs::NotifyCancel()"));
//	Test that one call to RFs::NotifyCancel() cancels all outstanding requests
	fs1.NotifyChangeCancel();
	User::WaitForRequest(status1);
	test_Value(status1.Int(), status1==KErrCancel);
	User::WaitForRequest(status2);
	test_Value(status2.Int(), status2==KErrCancel);
	User::WaitForRequest(status3);
	test_Value(status3.Int(), status3==KErrCancel);
	User::WaitForRequest(status4);
	test_Value(status4.Int(), status4==KErrCancel);
	User::WaitForRequest(status5);
	test_Value(status5.Int(), status5==KErrCancel);
//	Call the cancel function again to check no further action
	fs1.NotifyChangeCancel();

//	Test overloaded function to cancel a single request
	test.Next(_L("Cancel notification request using function overload"));
	fs1.NotifyChange(ENotifyAll,status1);
	fs1.NotifyChange(ENotifyAll,status2);
	fs1.NotifyChange(ENotifyAll,status3);
	fs1.NotifyChange(ENotifyAll,status4);
	fs1.NotifyChange(ENotifyAll,status5);
	test_Value(status1.Int(), status1==KRequestPending);
	test_Value(status2.Int(), status2==KRequestPending);
	test_Value(status3.Int(), status3==KRequestPending);
	test_Value(status4.Int(), status4==KRequestPending);
	test_Value(status5.Int(), status5==KRequestPending);

//	Cancel the outstanding request with status5
	test.Next(_L("RFs::NotifyCancel()"));
	fs1.NotifyChangeCancel(status5);
	User::WaitForRequest(status5);
	test_Value(status1.Int(), status1==KRequestPending);
	test_Value(status2.Int(), status2==KRequestPending);
	test_Value(status3.Int(), status3==KRequestPending);
	test_Value(status4.Int(), status4==KRequestPending);
	test_Value(status5.Int(), status5==KErrCancel);

	fs1.NotifyChangeCancel(status2);
	User::WaitForRequest(status2);

	test_Value(status1.Int(), status1==KRequestPending);
	test_Value(status2.Int(), status2==KErrCancel);
	test_Value(status3.Int(), status3==KRequestPending);
	test_Value(status4.Int(), status4==KRequestPending);

	fs1.NotifyChangeCancel(status4);
	User::WaitForRequest(status4);
	test_Value(status1.Int(), status1==KRequestPending);
	test_Value(status3.Int(), status3==KRequestPending);
	test_Value(status4.Int(), status4==KErrCancel);

	fs1.NotifyChangeCancel(status4);				//	Test no side effects on trying to cancel a request
	test_Value(status4.Int(), status4==KErrCancel);	//	that has already been cancelled

	fs1.NotifyChangeCancel(status1);
	User::WaitForRequest(status1);
	test_Value(status1.Int(), status1==KErrCancel);
	test_Value(status3.Int(), status3==KRequestPending);
	fs1.NotifyChangeCancel(status1);	//	Test no side effects on trying to cancel a request
	test_Value(status1.Int(), status1==KErrCancel);			//	that has already been cancelled

	fs1.NotifyChangeCancel(status3);
	User::WaitForRequest(status3);
	test_Value(status3.Int(), status3==KErrCancel);

	fs1.Close();
	}

static void Test4()
//
// Test notify client death
//
	{

	test.Next(_L("Kill client"));
	TInt r=gSleepThread.CreateLocal(0);
	test_KErrNone(r);
	RThread clientThread;
	r=clientThread.Create(_L("ClientThread"),ThreadEntryPoint,0x4000,KHeapSize,KHeapSize,(TAny*)ETest4);
	if (r!=KErrNone)
		{
		test.Printf(_L(" ERROR: Failed to create clientthread %d\n"),r);
		test(0);
		//test.Getch();
		return;
		}
	clientThread.Resume();
	gSleepThread.Wait();

	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);
	clientThread.Panic(_L("Test client thread panic"),KErrGeneral);
	User::SetJustInTime(jit);

	clientThread.Close();

	r=TheFs.MkDir(_L("\\F32-TST\\NOTIFY\\"));
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	MakeFile(_L("\\F32-TST\\NOTIFY\\NewFile.Txt"));
	User::After(1000);
	}

static void Test5()
//
// Test reads and writes do not cause notification
//
	{

	test.Next(_L("Test reads and writes do not cause notification"));


	RFile file;
	TInt r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\kangaroo.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.Close();
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\koala.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.Close();
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\dingo.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.Close();

	TRequestStatus reqStat=0;
	TheFs.NotifyChange(ENotifyEntry,reqStat);
	test_Value(reqStat.Int(), reqStat==KRequestPending);

	r=gSleepThread.CreateLocal(0);
	test_KErrNone(r);
	RThread clientThread;
	r=clientThread.Create(_L("Test5Thread"),ThreadEntryPoint,0x4000,KHeapSize,KHeapSize,(TAny*)ETest5);
	test_KErrNone(r);
	clientThread.Resume();
	gSleepThread.Wait();
	test_Value(reqStat.Int(), reqStat==KRequestPending);

	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\kangaroo.txt"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\koala.txt"));
	test_KErrNone(r);
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\dingo.txt"));
	test_KErrNone(r);



	gSleepThread.Close();
	clientThread.Close();
	}

#if defined (__EPOC32__)//we have no removable media on Emulator yet	.. ??? we do now
static void Test6()
//
//	Test media change notification
//
	{
	TDriveInfo driveInfo;
	TInt r=TheFs.Drive(driveInfo,CurrentDrive());
	test_KErrNone(r);
	// only test on removable media
	if (driveInfo.iDriveAtt&KDriveAttRemovable)
        {
        TBuf<64> b;
        b.Format(_L("Test Media change notification (socket:%d)"),gSocketNumber);
        test.Next(b);
        TRequestStatus reqStat=0;
        TInt r;
        TheFs.NotifyChange(ENotifyEntry,reqStat);
        test_Value(reqStat.Int(), reqStat==KRequestPending);
        r=gSleepThread.CreateLocal(0);
        test_KErrNone(r);
        RThread clientThread;
        r=clientThread.Create(_L("Test6Thread"),ThreadEntryPoint,0x4000,KHeapSize,KHeapSize,(TAny*)ETest6);
        test_KErrNone(r);
        clientThread.Resume();
        gSleepThread.Wait();
        TInt reqInt=reqStat.Int();
        test_KErrNone(reqInt);
        User::WaitForRequest(reqStat);
        WaitForMediaChange();
        gSleepThread.Close();
        clientThread.Close();
        }
	
        //-- it seems that after generating media change the meia driver isn't ready for some time
        User::After(2000000);
	    r=TheFs.Drive(driveInfo,CurrentDrive());
	    test_KErrNone(r);

    
    }
#endif

static void Test7()
//
// Test Write to uid region does not trigger notification
//
	{

	test.Next(_L("Test Write to uid region does not trigger notification"));
	TRequestStatus reqStat=0;
	MakeFile(_L("NewFile.TXT"));
	TInt r;
	TheFs.NotifyChange(ENotifyEntry,reqStat);
	test_Value(reqStat.Int(), reqStat==KRequestPending);

	r=gSleepThread.CreateLocal(0);
	test_KErrNone(r);
	RThread clientThread;
	r=clientThread.Create(_L("Test7Thread"),ThreadEntryPoint,0x4000,KHeapSize,KHeapSize,(TAny*)ETest7);
	test_KErrNone(r);
	clientThread.Resume();
	gSleepThread.Wait();
	test_Value(reqStat.Int(), reqStat==KRequestPending);

	r=TheFs.Delete(_L("Newfile.txt"));
	test_KErrNone(r);

	User::WaitForRequest(reqStat);

	gSleepThread.Close();
	clientThread.Close();
	}

#if defined (__EPOC32__)//we have no removable media on Emulator yet
static void MediaChangeExtendedNotification()
//
//	Test media change notification
//	Always notified of media change - regardless of requested TNotifyType
//
	{
	TDriveInfo driveInfo;
	TInt r=TheFs.Drive(driveInfo,CurrentDrive());
	test_KErrNone(r);
	// only test on removable media
	if (driveInfo.iDriveAtt&KDriveAttRemovable)
		{
		test.Next(_L("Test Media change extended notification"));
		TRequestStatus reqStat=0;
		TFileName path = _L("\\F32-tst\\NOTIFY\\");
		TInt r;
		TheFs.NotifyChange(ENotifyEntry,reqStat,path);
		test_Value(reqStat.Int(), reqStat==KRequestPending);
		r=gSleepThread.CreateLocal(0);
		test_KErrNone(r);
		RThread clientThread;
		gSocketNumber=0;
		r=clientThread.Create(_L("Test6Thread1"),ThreadEntryPoint,0x4000,KHeapSize,KHeapSize,(TAny*)ETest6);	//only generates a media change on removable media
		test_KErrNone(r);
		clientThread.Resume();
		gSleepThread.Wait();
		User::WaitForRequest(reqStat);
		test_KErrNone(reqStat.Int());
		WaitForMediaChange();
		gSleepThread.Close();
		clientThread.Close();

        //-- it seems that after generating media change the meia driver isn't ready for some time
        User::After(2000000);
	    r=TheFs.Drive(driveInfo,CurrentDrive());
	    test_KErrNone(r);


		TheFs.NotifyChange(ENotifyDisk,reqStat,path);
		test_Value(reqStat.Int(), reqStat==KRequestPending);
		r=gSleepThread.CreateLocal(0);
		test_KErrNone(r);
		r=clientThread.Create(_L("Test6Thread2"),ThreadEntryPoint,0x4000,KHeapSize,KHeapSize,(TAny*)ETest6);
		test_KErrNone(r);
		clientThread.Resume();
		gSleepThread.Wait();
		User::WaitForRequest(reqStat);
		test_KErrNone(reqStat.Int());
		WaitForMediaChange();
		gSleepThread.Close();
		clientThread.Close();

        //-- it seems that after generating media change the meia driver isn't ready for some time
        User::After(2000000);
	    r=TheFs.Drive(driveInfo,CurrentDrive());
	    test_KErrNone(r);

		TheFs.NotifyChange(ENotifyWrite,reqStat,path);
		test_Value(reqStat.Int(), reqStat==KRequestPending);
		r=gSleepThread.CreateLocal(0);
		test_KErrNone(r);
		r=clientThread.Create(_L("Test6Thread3"),ThreadEntryPoint,0x4000,KHeapSize,KHeapSize,(TAny*)ETest6);
		test_KErrNone(r);
		clientThread.Resume();
		gSleepThread.Wait();
		User::WaitForRequest(reqStat);
		test_KErrNone(reqStat.Int());
		WaitForMediaChange();
		gSleepThread.Close();
		clientThread.Close();

        //-- it seems that after generating media change the meia driver isn't ready for some time
        User::After(2000000);
	    r=TheFs.Drive(driveInfo,CurrentDrive());
	    test_KErrNone(r);

		}
	}
#endif

static void TestRequestAhead()
//
//	Test extended notification works when path initially does not exist
//
	{
	test.Next(_L("Test Request Ahead"));
//	First a simple example

	TInt r=TheFs.RmDir(_L("\\F32-TST\\NOTIFY\\BehindTheCurtain\\"));
	test_Value(r, (r == KErrNotFound)||(r==KErrPathNotFound)||(r==KErrNone));

	TFileName path=_L("\\F32-TST\\NOTIFY\\BehindTheCurtain\\");
	TRequestStatus reqStat(KRequestPending);
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);

	r=TheFs.MkDir(_L("\\F32-TST\\NOTIFY\\BehindTheCurtain\\"));
	test_KErrNone(r);

	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

	path=_L("\\F32-TST\\NOTIFY\\BehindTheCurtain\\PayNoAttention.man");
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);

	RFile file;
	r=file.Replace(TheFs,path,EFileStream);
	test_KErrNone(r);
	file.Close();

	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

	r=TheFs.Delete(path);
	test_KErrNone(r);

	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
//	Now cancel the outstanding request
	TheFs.NotifyChangeCancel(reqStat);
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);

//	Repeat with a ENotifyFile request
	TheFs.NotifyChange(ENotifyFile,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);

	r=file.Replace(TheFs,path,EFileStream);
	test_KErrNone(r);
	file.Close();

	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

	r=TheFs.Delete(path);
	test_KErrNone(r);

	TheFs.NotifyChange(ENotifyFile,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
//	Now cancel the outstanding request
	TheFs.NotifyChangeCancel(reqStat);
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);

//	Repeat with an ENotifyAttributes request
	TheFs.NotifyChange(ENotifyAttributes,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);

	r=file.Replace(TheFs,path,EFileStream);
	test_KErrNone(r);
	file.Close();

	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());	//	Monitoring attributes but informed anyway

	r=TheFs.Delete(path);
	test_KErrNone(r);

	TheFs.NotifyChange(ENotifyAttributes,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
//	Now cancel the outstanding request
	TheFs.NotifyChangeCancel(reqStat);
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);

//	Repeat with an ENotifyWrite request
	TheFs.NotifyChange(ENotifyWrite,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);

	r=file.Replace(TheFs,path,EFileStream);
	test_KErrNone(r);
	file.Close();

	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());	//	Monitoring file writing but informed anyway

	r=TheFs.Delete(path);
	test_KErrNone(r);

	TheFs.NotifyChange(ENotifyWrite,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
//	Now cancel the outstanding request
	TheFs.NotifyChangeCancel(reqStat);
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);

//	Repeat with an ENotifyDisk request
	TheFs.NotifyChange(ENotifyDisk,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);

	r=file.Replace(TheFs,path,EFileStream);
	test_KErrNone(r);
	file.Close();

	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());	//	Monitoring disk activity but informed anyway

	r=TheFs.Delete(path);
	test_KErrNone(r);

	TheFs.NotifyChange(ENotifyAttributes,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
//	Now cancel the outstanding request
	TheFs.NotifyChangeCancel(reqStat);
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);

//	Now do much the same with directory monitoring
	path=_L("\\F32-TST\\NOTIFY\\BehindTheCurtain\\");
	TheFs.RmDir(path);

	TheFs.NotifyChange(ENotifyDir,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);

	TheFs.MkDir(path);
	test_KErrNone(r);

	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

	TheFs.RmDir(path);
	test_KErrNone(r);

	TheFs.NotifyChange(ENotifyDir,reqStat,path);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);

//	Now cancel the outstanding request
	TheFs.NotifyChangeCancel(reqStat);
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);

	TheFs.NotifyChange(ENotifyDir,reqStat,path);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);

//	Get a separate thread to create the directory
	RThread thread;
	thread.Create(_L("RequestAheadyThready"),ThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETest10);
	TRequestStatus thrdStat(KRequestPending);
	thread.Logon(thrdStat);
	thread.Resume();
	thread.Close();

	User::WaitForRequest(thrdStat);
	test_KErrNone(thrdStat.Int());
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

	TheFs.RmDir(path);
	test_KErrNone(r);

//	Check that notification is not received for a non-existent file if only the previously
//	non existent directory that contains it is created
	path=_L("\\F32-TST\\NOTIFY\\BehindTheCurtain\\PayNoAttention.man");
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);

	thread.Create(_L("RequestAheadThread"),ThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETest10);
	thread.Logon(thrdStat);
	thread.Resume();
	thread.Close();

	User::WaitForRequest(thrdStat);
	test_KErrNone(thrdStat.Int());
	test_Value(reqStat.Int(), reqStat==KRequestPending);

//	Now get a thread to create the file
	thread.Create(_L("RequestAhead"),ThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETest11);
	thread.Logon(thrdStat);
	thread.Resume();
	User::WaitForRequest(thrdStat);
	test_KErrNone(thrdStat.Int());
	thread.Close();

	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

	TheFs.Delete(path);
	test_KErrNone(r);

	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);

//	Now cancel the outstanding request
	TheFs.NotifyChangeCancel(reqStat);
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);

	path=_L("\\F32-TST\\NOTIFY\\BehindTheCurtain\\");
	TheFs.RmDir(path);
	test_KErrNone(r);
	}


static void Test8()
//
// Test extended notification of an entry change
//
	{
//	Test notification of an entry change in directory F32-TST
	test.Next(_L("Test notification of an entry change"));
	TRequestStatus reqStat(KRequestPending);
	TRequestStatus thrdStat(KRequestPending);
	TFileName path=(_L("\\F32-TST\\"));

	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	RThread thread;
	TInt r=thread.Create(_L("MyThread"),ThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETest1);
	test_KErrNone(r);
	thread.Logon(thrdStat);
	thread.Resume();
	User::WaitForRequest(thrdStat);
	test_KErrNone(thrdStat.Int());
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	thread.Close();

//	Repeat the test
	test.Next(_L("Repeat Test notification of an entry change"));
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=thread.Create(_L("MyThread2"),ThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETest1);
	test_KErrNone(r);
	thread.Logon(thrdStat);
	thread.Resume();
	User::WaitForRequest(thrdStat);
	test_KErrNone(thrdStat.Int());
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	thread.Close();

//	Test it can be cancelled
	test.Next(_L("Test Notify cancel"));
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	TheFs.NotifyChangeCancel();
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);

//	Test it can be notified again
	test.Next(_L("Test notification still works"));
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=thread.Create(_L("MyThread3"),ThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETest1);
	test_KErrNone(r);
	thread.Logon(thrdStat);
	thread.Resume();
	User::WaitForRequest(thrdStat);
	test_KErrNone(thrdStat.Int());
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	thread.Close();

//	Test notification doesn't occur when a change occurs above the directory monitored
//	(Notification of rename events occurring above the directory which affect the path
//	will occur - this is tested for in Test18())
	test.Next(_L("Test changing above monitored directory"));
	r=TheFs.MkDir(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\"));
	test_Value(r, (r == KErrNone)||(r==KErrAlreadyExists));
	path=_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\");
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=thread.Create(_L("MyThread4"),ThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETest1);
	test_KErrNone(r);
	thread.Logon(thrdStat);
	thread.Resume();
	User::WaitForRequest(thrdStat);
	test_KErrNone(thrdStat.Int());
	User::After(500000);
	thread.Close();
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	TheFs.NotifyChangeCancel();
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);

//	Test notification occurs when a change is made to the subdirectory monitored
	test.Next(_L("Create a file in monitored subdirectory"));
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=thread.Create(_L("MyThread5"),ThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETest2);
	test_KErrNone(r);
	thread.Logon(thrdStat);
	thread.Resume();
	User::WaitForRequest(thrdStat);
	test_KErrNone(thrdStat.Int());
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	thread.Close();

	test.Next(_L("Create a directory in monitored subdirectory"));
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=thread.Create(_L("MyThread6"),ThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETest3);
	test_KErrNone(r);
	thread.Logon(thrdStat);
	thread.Resume();
	User::WaitForRequest(thrdStat);
	test_KErrNone(thrdStat.Int());
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	thread.Close();

	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.MkDir(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\SCARECROW\\TINMAN\\"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());


	r=TheFs.RmDir(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\SCARECROW\\TINMAN\\"));
	test_KErrNone(r);
	r=TheFs.RmDir(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\SCARECROW\\"));
	test_KErrNone(r);

//	Test again that notification doesn't occur above the subdirectory being monitored
	test.Next(_L("Test changing above monitored directory"));
	r=TheFs.MkDir(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\"));
	test_Value(r, (r == KErrNone)||(r==KErrAlreadyExists));
	path=_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\");

	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=thread.Create(_L("MyThread7"),ThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETest1);
	test_KErrNone(r);
	thread.Logon(thrdStat);
	thread.Resume();
	User::WaitForRequest(thrdStat);
	test_KErrNone(thrdStat.Int());
	User::After(500000);
	thread.Close();
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	TheFs.NotifyChangeCancel();
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);

//	Test notification occurs when a change is made to the subdirectory monitored
	test.Next(_L("Delete a file in monitored subdirectory"));
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

	RFile file;
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\WickedWitch.msg"),EFileStream);
	test_KErrNone(r);
	file.Close();

//	Test notification on a specific file
	test.Next(_L("Monitor changes to a specific file"));
	path+=_L("WickedWitch.msg");
	TheFs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=thread.Create(_L("MyThread8"),ThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETest8);
	test_KErrNone(r);
	thread.Logon(thrdStat);
	thread.Resume();
	User::WaitForRequest(thrdStat);
	test_KErrNone(thrdStat.Int());
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	thread.Close();

//	Test notification does not occur if a change is made above the file
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	thread.Create(_L("MyThread9"),ThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETest2);
	thread.Logon(thrdStat);
	thread.Resume();
	User::WaitForRequest(thrdStat);
	test_KErrNone(thrdStat.Int());
	User::After(500000);
	thread.Close();
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	TheFs.NotifyChangeCancel();
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);

//	Test notification occurs when a change is made to the file
	test.Next(_L("Delete monitored file"));
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\WickedWitch.Msg"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	Test notification request is now submitted on the non existent path successfully
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	TheFs.NotifyChangeCancel(reqStat);
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);

	path=_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.Doc");
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.Doc"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

	path=_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\");
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.RmDir(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	Submit a request for a path which does not yet exist
	path=_L("\\F32-TST\\NOTIFY\\GOOD_WITCH\\");
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
//	Now create the directory we are waiting on
	r=TheFs.MkDir(path);
	test_KErrNone(r);
//	Make sure the notification has now been received
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	Submit a request for a file which does not yet exist
	path=_L("\\F32-TST\\NOTIFY\\GOOD_WITCH\\Red-Shoes.red");
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
//	Now create the file we are waiting on
	r=file.Replace(TheFs,path,EFileStream);
	test_KErrNone(r);
	file.Close();
//	Make sure the notification has now been received
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
//	Submit another notification request and delete the file
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.Delete(path);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	path=_L("\\F32-TST\\NOTIFY\\GOOD_WITCH\\");
	TheFs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.RmDir(path);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	// test passing in an empty string
	TheFs.NotifyChange(ENotifyEntry,reqStat,_L(""));
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrArgument);
	}

static void Test9()
//
// Test notify for multiple clients
//
	{

	test.Next(_L("Test notification of multiple clients"));

//	Create five sessions monitoring various levels of a directory tree

	TInt r=TheFs.MkDir(_L("\\F32-TST\\NOTIFY\\ANIMAL\\"));
	test_Value(r, (r == KErrNone)||(r==KErrAlreadyExists));
	RFile file;
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\ANIMAL\\cat.txt"),EFileStream);
	test_KErrNone(r);
	file.Close();
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\ANIMAL\\dog.txt"),EFileStream);
	test_KErrNone(r);
	file.Close();

	TFileName path1=_L("\\F32-TST\\");
	TFileName path2=_L("\\F32-TST\\NOTIFY\\");
	TFileName path3=_L("\\F32-TST\\NOTIFY\\ANIMAL\\");
	TFileName path4=_L("\\F32-TST\\NOTIFY\\ANIMAL\\cat.txt");
	TFileName path5=_L("\\F32-TST\\NOTIFY\\ANIMAL\\dog.txt");
	TFileName path6=_L("?:\\F32-TST\\");

	TRequestStatus reqStat1(KRequestPending);
	RFs fs1;
	r=fs1.Connect();
	test_KErrNone(r);
	r=fs1.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	fs1.NotifyChange(ENotifyEntry,reqStat1,path1);

	TRequestStatus reqStat2(KRequestPending);
	RFs fs2;
	r=fs2.Connect();
	test_KErrNone(r);
	r=fs2.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	fs2.NotifyChange(ENotifyEntry,reqStat2,path2);

	TRequestStatus reqStat3(KRequestPending);
	RFs fs3;
	r=fs3.Connect();
	test_KErrNone(r);
	r=fs3.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	fs3.NotifyChange(ENotifyEntry,reqStat3,path3);

	TRequestStatus reqStat4(KRequestPending);
	RFs fs4;
	r=fs4.Connect();
	test_KErrNone(r);
	r=fs4.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	fs4.NotifyChange(ENotifyEntry,reqStat4,path4);

	TRequestStatus reqStat5(KRequestPending);
	RFs fs5;
	r=fs5.Connect();
	test_KErrNone(r);
	r=fs5.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	fs5.NotifyChange(ENotifyEntry,reqStat5,path5);

	TRequestStatus reqStat6(KRequestPending);
	RFs fs6;
	r=fs6.Connect();
	test_KErrNone(r);
	r=fs6.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	fs6.NotifyChange(ENotifyEntry,reqStat6,path6);

	test_Value(reqStat1.Int(), reqStat1==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	test_Value(reqStat5.Int(), reqStat5==KRequestPending);
	test_Value(reqStat6.Int(), reqStat6==KRequestPending);

//	Make a change a the top level and check that only the session monitoring
//	that level is notified
	test.Next(_L("Test only client monitoring top level is notified"));
	r=file.Replace(TheFs,_L("\\F32-TST\\NewFile.txt"),EFileStream);
	test_KErrNone(r);
	file.Close();
	User::WaitForRequest(reqStat1);
	test_KErrNone(reqStat1.Int());
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	test_Value(reqStat5.Int(), reqStat5==KRequestPending);
	User::WaitForRequest(reqStat6);
	test_KErrNone(reqStat6.Int());

	r=TheFs.Delete(_L("\\F32-TST\\NewFile.txt"));
	test_KErrNone(r);

//	Renew the notify request at the top level and make a change one step lower
	fs1.NotifyChange(ENotifyEntry,reqStat1,path1);
	fs6.NotifyChange(ENotifyEntry,reqStat6,path6);
	test_Value(reqStat1.Int(), reqStat1==KRequestPending);
	test_Value(reqStat6.Int(), reqStat6==KRequestPending);

	test.Next(_L("Test clients monitoring levels 1 and 2 are notified"));
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileStream);
	test_KErrNone(r);
	file.Close();

	User::WaitForRequest(reqStat1);
	User::WaitForRequest(reqStat2);
	test_KErrNone(reqStat1.Int());
	test_KErrNone(reqStat2.Int());
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	test_Value(reqStat5.Int(), reqStat5==KRequestPending);
	User::WaitForRequest(reqStat6);
	test_KErrNone(reqStat6.Int());

	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\NewFile.txt"));
	test_KErrNone(r);

//	Renew the notify request at the top and second levels and make a change
//	one step lower still
	fs1.NotifyChange(ENotifyEntry,reqStat1,path1);
	fs2.NotifyChange(ENotifyEntry,reqStat2,path2);
	fs6.NotifyChange(ENotifyEntry,reqStat6,path6);
	test_Value(reqStat1.Int(), reqStat1==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat6.Int(), reqStat6==KRequestPending);

	test.Next(_L("Test clients monitoring levels 1,2 and 3 are notified"));
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\ANIMAL\\NewFile.txt"),EFileStream);
	test_KErrNone(r);
	file.Close();

	User::WaitForRequest(reqStat1);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat1.Int());
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	test_Value(reqStat5.Int(), reqStat5==KRequestPending);
	User::WaitForRequest(reqStat6);
	test_KErrNone(reqStat6.Int());

	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\ANIMAL\\NewFile.txt"));
	test_KErrNone(r);

//	Renew the notify request at the top, second and third levels and make a change
//	one step lower still
	fs1.NotifyChange(ENotifyEntry,reqStat1,path1);
	fs2.NotifyChange(ENotifyEntry,reqStat2,path2);
	fs3.NotifyChange(ENotifyEntry,reqStat3,path3);
	fs6.NotifyChange(ENotifyEntry,reqStat6,path6);
	test_Value(reqStat1.Int(), reqStat1==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat6==KRequestPending);

	test.Next(_L("Test clients monitoring levels 1 - 4 are notified"));
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\ANIMAL\\cat.txt"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat1);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
	User::WaitForRequest(reqStat4);
	test_KErrNone(reqStat1.Int());
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());
	test_KErrNone(reqStat4.Int());
	test_Value(reqStat5.Int(), reqStat5==KRequestPending);
	User::WaitForRequest(reqStat6);
	test_KErrNone(reqStat6.Int());

//	Renew the notify request at the top, second and third levels and on the file deleted above
//	which will be successful, but will not complete (for obvious reasons)

//	Make a change one step lower still
	fs1.NotifyChange(ENotifyEntry,reqStat1,path1);
	fs2.NotifyChange(ENotifyEntry,reqStat2,path2);
	fs3.NotifyChange(ENotifyEntry,reqStat3,path3);
	fs6.NotifyChange(ENotifyEntry,reqStat6,path6);
	fs4.NotifyChange(ENotifyEntry,reqStat4,path4);
	test_Value(reqStat1.Int(), reqStat1==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	test_Value(reqStat6.Int(), reqStat6==KRequestPending);

	test.Next(_L("Test clients monitoring levels 1 - 3 and 5 are notified"));
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\ANIMAL\\dog.txt"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat1);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
//	Don't wait for reqStat4
	User::WaitForRequest(reqStat5);
	User::WaitForRequest(reqStat6);
	test_KErrNone(reqStat1.Int());
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);	//	File does not exist
	test_KErrNone(reqStat5.Int());
	test_KErrNone(reqStat6.Int());

	fs4.NotifyChangeCancel(reqStat4);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrCancel);
//	Renew the notify request at the top, second and third levels and attempt to renew
//	the request on the files deleted above (which will fail).

	test.Next(_L("Test clients monitoring levels 1 - 3 are notified"));
	fs1.NotifyChange(ENotifyEntry,reqStat1,path1);
	fs2.NotifyChange(ENotifyEntry,reqStat2,path2);
	fs3.NotifyChange(ENotifyEntry,reqStat3,path3);
	fs4.NotifyChange(ENotifyEntry,reqStat4,path4);
	fs6.NotifyChange(ENotifyEntry,reqStat6,path6);
	fs5.NotifyChange(ENotifyEntry,reqStat5,path5);
	test_Value(reqStat1.Int(), reqStat1==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	test_Value(reqStat5.Int(), reqStat5==KRequestPending);
	test_Value(reqStat6.Int(), reqStat6==KRequestPending);

	r=TheFs.RmDir(_L("\\F32-TST\\NOTIFY\\ANIMAL\\"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat1);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat1.Int());
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	test_Value(reqStat5.Int(), reqStat5==KRequestPending);
	User::WaitForRequest(reqStat6);
	test_KErrNone(reqStat6.Int());

//	Renew the notify request at the top and second levels on the third level
//	which was removed - it'll succeed but won't complete.

	test.Next(_L("Test clients monitoring levels 1 and 2 are notified"));
	test.Next(_L("Test clients' attempts to monitor levels 3-5 fail"));
	fs1.NotifyChange(ENotifyEntry,reqStat1,path1);
	fs2.NotifyChange(ENotifyEntry,reqStat2,path2);
	fs3.NotifyChange(ENotifyEntry,reqStat3,path3);
	fs6.NotifyChange(ENotifyEntry,reqStat6,path6);

	test_Value(reqStat1.Int(), reqStat1==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	test_Value(reqStat5.Int(), reqStat5==KRequestPending);
	test_Value(reqStat6.Int(), reqStat6==KRequestPending);

	fs1.NotifyChangeCancel();
	fs2.NotifyChangeCancel();

	fs1.Close();
	fs2.Close();
//	Close the other sessions with requests outstanding to test that there's no evilness
	fs3.Close();
	fs4.Close();
	fs5.Close();
	fs6.Close();

	User::WaitForRequest(reqStat1);
	User::WaitForRequest(reqStat2);
	// Closing file server sessions doesn't seem to complete notifications, is this a bug?
//	User::WaitForRequest(reqStat3);
//	User::WaitForRequest(reqStat4);
//	User::WaitForRequest(reqStat5);
//	User::WaitForRequest(reqStat6);
	}

static void Test10()
//
// Test notify cancel
//
	{

	test.Next(_L("Cancel notification request using simple function"));
	TFileName path=_L("\\F32-TST\\NOTIFY\\");
	RFs fs1;
	TInt r=fs1.Connect();
	test_KErrNone(r);
	r=fs1.SetSessionPath(gSessionPath);
	test_KErrNone(r);

	TRequestStatus status1;
	TRequestStatus status2;
	TRequestStatus status3;
	TRequestStatus status4;
	TRequestStatus status5;

	fs1.NotifyChange(ENotifyAll,status1,path);
	fs1.NotifyChange(ENotifyAll,status2,path);
	fs1.NotifyChange(ENotifyAll,status3,path);
	fs1.NotifyChange(ENotifyAll,status4,path);
	fs1.NotifyChange(ENotifyAll,status5,path);
	test_Value(status1.Int(), status1==KRequestPending);
	test_Value(status2.Int(), status2==KRequestPending);
	test_Value(status3.Int(), status3==KRequestPending);
	test_Value(status4.Int(), status4==KRequestPending);
	test_Value(status5.Int(), status5==KRequestPending);

	test.Next(_L("RFs::NotifyCancel()"));
//	Test that one call to RFs::NotifyCancel() cancels all outstanding requests
	fs1.NotifyChangeCancel();
	User::WaitForRequest(status1);
	test_Value(status1.Int(), status1==KErrCancel);
	User::WaitForRequest(status2);
	test_Value(status2.Int(), status2==KErrCancel);
	User::WaitForRequest(status3);
	test_Value(status3.Int(), status3==KErrCancel);
	User::WaitForRequest(status4);
	test_Value(status4.Int(), status4==KErrCancel);
	User::WaitForRequest(status5);
	test_Value(status5.Int(), status5==KErrCancel);
//	Call the cancel function again to check no further action
	fs1.NotifyChangeCancel();

//	Test overloaded function to cancel a single request
	test.Next(_L("Cancel notification request using function overload"));
	fs1.NotifyChange(ENotifyAll,status1,path);
	fs1.NotifyChange(ENotifyAll,status2,path);
	fs1.NotifyChange(ENotifyAll,status3,path);
	fs1.NotifyChange(ENotifyAll,status4,path);
	fs1.NotifyChange(ENotifyAll,status5,path);
	test_Value(status1.Int(), status1==KRequestPending);
	test_Value(status2.Int(), status2==KRequestPending);
	test_Value(status3.Int(), status3==KRequestPending);
	test_Value(status4.Int(), status4==KRequestPending);
	test_Value(status5.Int(), status5==KRequestPending);

//	Cancel the outstanding request with status5
	test.Next(_L("RFs::NotifyCancel()"));
	fs1.NotifyChangeCancel(status5);
	User::WaitForRequest(status5);
	test_Value(status1.Int(), status1==KRequestPending);
	test_Value(status2.Int(), status2==KRequestPending);
	test_Value(status3.Int(), status3==KRequestPending);
	test_Value(status4.Int(), status4==KRequestPending);
	test_Value(status5.Int(), status5==KErrCancel);

	r=TheFs.MkDir(_L("\\F32-TST\\TROPICANA\\"));
	test_KErrNone(r);
	test_Value(status1.Int(), status1==KRequestPending);
	test_Value(status2.Int(), status2==KRequestPending);
	test_Value(status3.Int(), status3==KRequestPending);
	test_Value(status4.Int(), status4==KRequestPending);

	fs1.NotifyChangeCancel(status2);
	User::WaitForRequest(status2);

	test_Value(status1.Int(), status1==KRequestPending);
	test_Value(status2.Int(), status2==KErrCancel);
	test_Value(status3.Int(), status3==KRequestPending);
	test_Value(status4.Int(), status4==KRequestPending);

	r=TheFs.RmDir(_L("\\F32-TST\\TROPICANA\\"));
	test_KErrNone(r);
	test_Value(status1.Int(), status1==KRequestPending);
	test_Value(status3.Int(), status3==KRequestPending);
	test_Value(status4.Int(), status4==KRequestPending);

	fs1.NotifyChangeCancel(status4);
	User::WaitForRequest(status4);
	test_Value(status1.Int(), status1==KRequestPending);
	test_Value(status3.Int(), status3==KRequestPending);
	test_Value(status4.Int(), status4==KErrCancel);

	fs1.NotifyChangeCancel(status4);	//	Test no side effects on trying to cancel a request
	test_Value(status4.Int(), status4==KErrCancel);			//	that has already been cancelled

	fs1.NotifyChangeCancel(status1);
	User::WaitForRequest(status1);
	test_Value(status1.Int(), status1==KErrCancel);
	test_Value(status3.Int(), status3==KRequestPending);
	fs1.NotifyChangeCancel(status1);	//	Test no side effects on trying to cancel a request
	test_Value(status1.Int(), status1==KErrCancel);			//	that has already been cancelled

	fs1.NotifyChangeCancel(status3);
	User::WaitForRequest(status3);
	test_Value(status3.Int(), status3==KErrCancel);

	fs1.Close();
	}

static void Test11()
//
// Test notify client death
//
	{

	test.Next(_L("Kill client while it is monitoring changes to a directory"));
//	Call CreateLocal to create RSemaphore gSleepThread which is local to this process
	TInt r=gSleepThread.CreateLocal(0);
	test_KErrNone(r);

	RThread clientThread;
	r=clientThread.Create(_L("ClientThread"),ThreadEntryPoint,0x4000,KHeapSize,KHeapSize,(TAny*)ETest9);
	test_KErrNone(r);
	clientThread.Resume();
	gSleepThread.Wait();	//	Wait for gSleepThread to be signalled
							//	Client thread is waiting for notification of changes
							//	to directory \\F32-TST\\NOTIFY

	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);
	clientThread.Panic(_L("Test client thread panic"),KErrGeneral);	//	Panic client
	User::SetJustInTime(jit);

	clientThread.Close();

//	Make a change and check there's no disaster
	r=TheFs.MkDir(_L("\\F32-TST\\NOTIFY\\"));
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	MakeFile(_L("\\F32-TST\\NOTIFY\\NewFile.Txt"));
	User::After(1000);
	}


static void Test12()
//
// Test reads and writes do not cause notification under ENotifyEntry
// Test reads and writes do cause notification under ENotifyAll
//
	{

	test.Next(_L("Test reads and writes do not cause notification under ENotifyEntry"));

	RFile file;
	TInt r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\kangaroo.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.Close();
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\koala.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.Close();
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\dingo.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.Close();

	TFileName path=_L("\\F32-TST\\NOTIFY\\");
	TRequestStatus reqStat1(KRequestPending);
	RFs fs1;
	r=fs1.Connect();
	test_KErrNone(r);
	r=fs1.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	fs1.NotifyChange(ENotifyEntry,reqStat1,path);

	TRequestStatus reqStat2(KRequestPending);
	RFs fs2;
	r=fs2.Connect();
	test_KErrNone(r);
	r=fs2.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	fs2.NotifyChange(ENotifyEntry,reqStat2,path);

	TRequestStatus reqStat3(KRequestPending);
	RFs fs3;
	r=fs3.Connect();
	test_KErrNone(r);
	r=fs3.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	fs3.NotifyChange(ENotifyEntry,reqStat3,path);

	test_Value(reqStat1.Int(), reqStat1==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);

	r=gSleepThread.CreateLocal(0);
	test_KErrNone(r);
	RThread thread1;
	r=thread1.Create(_L("TestThread1"),ThreadEntryPoint,0x4000,KHeapSize,KHeapSize,(TAny*)ETest5);
	test_KErrNone(r);
	thread1.Resume();
	gSleepThread.Wait();

	test_Value(reqStat1.Int(), reqStat1==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);

	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\kangaroo.txt"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat1);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat1.Int());	//	All three notifications occur because they
	test_KErrNone(reqStat2.Int());	//	are all monitoring the top level directory
	test_KErrNone(reqStat3.Int());	//	Later, we'll test monitoring individual files...

	gSleepThread.Close();
	thread1.Close();

	test.Next(_L("Test reads and writes do cause notification under ENotifyAll"));
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\kangaroo.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.Close();

	fs1.NotifyChange(ENotifyAll,reqStat1,path);
	fs2.NotifyChange(ENotifyAll,reqStat2,path);
	fs3.NotifyChange(ENotifyAll,reqStat3,path);

	test_Value(reqStat1.Int(), reqStat1==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);

	r=gSleepThread.CreateLocal(0);
	test_KErrNone(r);
	RThread thread2;
	r=thread2.Create(_L("TestThread2"),ThreadEntryPoint,0x4000,KHeapSize,KHeapSize,(TAny*)ETest5);
	test_KErrNone(r);
	thread2.Resume();
	gSleepThread.Wait();

	User::WaitForRequest(reqStat1);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat1.Int());
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());

	gSleepThread.Close();
	thread2.Close();

	test.Next(_L("Monitor reads and writes on specific files with either TNotifyType"));
	TFileName path1=path;
	TFileName path2=path;
	TFileName path3=path;
	path1+=_L("kangaroo.txt");
	path2+=_L("koala.txt");
	path3+=_L("dingo.txt");

	fs1.NotifyChange(ENotifyAll,reqStat1,path1);
	fs2.NotifyChange(ENotifyEntry,reqStat2,path2);
	fs3.NotifyChange(ENotifyAll,reqStat3,path3);

	test_Value(reqStat1.Int(), reqStat1==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);

	r=gSleepThread.CreateLocal(0);
	test_KErrNone(r);
	RThread thread3;
	r=thread3.Create(_L("TestThread3"),ThreadEntryPoint,0x4000,KHeapSize,KHeapSize,(TAny*)ETest5);
	test_KErrNone(r);
	thread3.Resume();
	gSleepThread.Wait();

	User::WaitForRequest(reqStat1);
	test_KErrNone(reqStat1.Int());
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);	//	Monitoring with ENotifyEntry
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat3.Int());

	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\koala.txt"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat2);
	test_KErrNone(reqStat2.Int());

	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\kangaroo.txt"));
	test_KErrNone(r);
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\dingo.txt"));
	test_KErrNone(r);

	gSleepThread.Close();
	thread3.Close();
	fs1.Close();
	fs2.Close();
	fs3.Close();
	}


static void Test13()
//
//	Test file notification
//
	{
	RFs fs;	//	Session to be notified of any changes
	TInt r=fs.Connect();
	test_KErrNone(r);
	r=fs.SetSessionPath(gSessionPath);
	test_KErrNone(r);


	r=TheFs.MkDir(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\"));
	test_Value(r, r == KErrNone||r==KErrAlreadyExists);

	RFile file;
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\WickedWitch.msg"),EFileStream);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	file.Close();

//	Test notification on a specific file
	test.Next(_L("Monitor changes to a specific file"));
	TFileName path=_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\WickedWitch.msg");
	TRequestStatus reqStat(KRequestPending);
	TRequestStatus thrdStat(KRequestPending);
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	RThread thread;
	r=thread.Create(_L("MyThread7"),ThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETest8);
	test_KErrNone(r);
	thread.Logon(thrdStat);
	thread.Resume();
	User::WaitForRequest(thrdStat);
	test_KErrNone(thrdStat.Int());
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	thread.Close();
//	Test notification does not occur if a change is made above the file
	fs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=thread.Create(_L("MyThread8"),ThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETest1);
	test_KErrNone(r);
	thread.Logon(thrdStat);
	thread.Resume();
	User::WaitForRequest(thrdStat);
	test_KErrNone(thrdStat.Int());
	User::After(500000);
	thread.Close();
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\NEWFILE.TXT"));
	test_KErrNone(r);

//	Test notification does not occur if a change is made to another file
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Lion.log"),EFileStream);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	file.Close();
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Lion.log"));
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);


//	Test notification occurs when a change is made to the file
	test.Next(_L("Delete monitored file"));
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\WickedWitch.Msg"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

	fs.Close();
	}

static void Test14()

	{
//
//	Test notification request succeeds with all RFile and RFs operations which result in
//	notifications
//
	RFs fs;
	TInt r=fs.Connect();	//	Session to be notified of any changes
	test_KErrNone(r);
	r=fs.SetSessionPath(gSessionPath);
	test_KErrNone(r);

//	RFile::Write() to a file within the monitored directory
	test.Next(_L("RFile::Write()"));
	TFileName path=_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.Doc");
	TRequestStatus reqStat(KRequestPending);

	RFile file;

	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),EFileRead|EFileWrite);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=file.Write(0,_L8("Pay no attention to the man behind the curtain"));
	test_KErrNone(r);
	file.Close();
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFile::Read() a file within the monitored directory - no notification for reads
	path=_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\");
	TBuf8<100> temp;
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),EFileRead|EFileWrite);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=file.Read(0,temp,100);
	test_Value(reqStat.Int(), reqStat==KRequestPending);

//	RFile::SetAtt() of a file within the monitored directory
	test.Next(_L("RFile::SetAtt()"));
	r=file.SetAtt(KEntryAttSystem,KEntryAttNormal);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);

//	RFile::SetSize() of a file within the monitored directory
	test.Next(_L("RFile::SetSize()"));
	r=file.SetSize(256);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	file.Close();

//	RFile::Temp() to create a temp file within the monitored directory
	test.Next(_L("RFile::Temp()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	TFileName fileName;
	r=file.Temp(TheFs,path,fileName,EFileWrite);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	file.Close();

//	RFile::SetModified() to change modification time of a file within monitored dir
	test.Next(_L("RFile::SetModified()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	TTime now;
	now.HomeTime();
	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),EFileRead|EFileWrite);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	file.SetModified(now);
	file.Close();
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFs::SetEntry() to change a directory entry within the monitored directory
	test.Next(_L("RFs::SetEntry()"));
	TEntry entry;
	fs.NotifyChange(ENotifyAll,reqStat,path);
	r=TheFs.Entry(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),entry);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	now.HomeTime();
	r=TheFs.SetEntry(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),now,KEntryAttHidden,KEntryAttNormal);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFile::Set() to change file's modification time and attributes
	test.Next(_L("RFile::Set()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),EFileRead|EFileWrite);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	now.HomeTime();
	r=file.Set(now,KEntryAttNormal,KEntryAttHidden);
	file.Close();
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFs::SetDriveName()
	test.Next(_L("RFs::SetDriveName()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	User::After(KNotifyChangeAfter);
	r=TheFs.SetDriveName(KDefaultDrive,_L("DRIVETEST"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	fs.NotifyChange(ENotifyEntry,reqStat,path);
	User::After(KNotifyChangeAfter);
	r=TheFs.SetDriveName(KDefaultDrive,_L("TEST"));
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	fs.NotifyChangeCancel(reqStat);
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);
	fs.NotifyChange(ENotifyDisk,reqStat,path);
	User::After(KNotifyChangeAfter);
	r=TheFs.SetDriveName(KDefaultDrive,_L("DRIVE"));
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	fs.NotifyChangeCancel(reqStat);
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);


//	RFs::MkDir()
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test.Next(_L("RFs::MkDir()"));
	r=TheFs.MkDir(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\EMERALD_CITY\\"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFs::RmDir()
	test.Next(_L("RFs::RmDir()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.RmDir(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\EMERALD_CITY\\"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFile::Create()
	test.Next(_L("RFile::Create()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=file.Create(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Good_Witch.bat"),EFileRead|EFileWrite);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	file.Close();

//	RFs::Delete()
	test.Next(_L("RFs::Delete()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Good_Witch.bat"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFile::Replace()
	test.Next(_L("RFile::Replace()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Good_Witch.bat"),EFileRead|EFileWrite);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	file.Close();

//	RFs::Delete()
	test.Next(_L("RFs::Delete()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Good_Witch.bat"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFs::SetVolumeLabel() - should only be notification when monitoring relevant TNotifyTypes
	test.Next(_L("RFs::SetVolumeLabel"));
	fs.NotifyChange(ENotifyAll,reqStat,path);

	TInt driveNum=CurrentDrive();
	TVolumeInfo volInfo;
	TFileName currentVolName;

	r=TheFs.Volume(volInfo,driveNum);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	currentVolName=volInfo.iName;

	r=TheFs.SetVolumeLabel(_L("VOL"),driveNum);
	if (r==KErrNone)
		{
		User::WaitForRequest(reqStat);
		test_KErrNone(reqStat.Int());
		r=TheFs.Volume(volInfo,driveNum);
		test_KErrNone(r);
		test(volInfo.iName==_L("VOL"));
	//	Test notification occurs under ENotifyDisk
		fs.NotifyChange(ENotifyDisk,reqStat,path);
		test_Value(reqStat.Int(), reqStat==KRequestPending);
		r=TheFs.SetVolumeLabel(_L("ABCDEFGHIJK"),driveNum);
		test_KErrNone(r);
		User::WaitForRequest(reqStat);
		test_KErrNone(reqStat.Int());
		r=TheFs.Volume(volInfo,driveNum);
		test_KErrNone(r);

		test(volInfo.iName==_L("ABCDEFGHIJK"));

	//	Test notification does not occur under ENotifyAttributes
		fs.NotifyChange(ENotifyAttributes,reqStat,path);
		r=TheFs.SetVolumeLabel(_L("TROPICANA"),driveNum);
		test_KErrNone(r);
		test_Value(reqStat.Int(), reqStat==KRequestPending);
		r=TheFs.Volume(volInfo,driveNum);
		test_KErrNone(r);

		test(volInfo.iName==_L("TROPICANA"));

		fs.NotifyChangeCancel(reqStat);
		User::WaitForRequest(reqStat);
		test_Value(reqStat.Int(), reqStat==KErrCancel);
	//	Test notification occurs under ENotifyEntry
		fs.NotifyChange(ENotifyEntry,reqStat,path);
		test_Value(reqStat.Int(), reqStat==KRequestPending);
		r=TheFs.SetVolumeLabel(currentVolName,driveNum);
		test_KErrNone(r);
		User::WaitForRequest(reqStat);
		test_KErrNone(reqStat.Int());
		r=TheFs.Volume(volInfo,driveNum);
		test_KErrNone(r);
		test(volInfo.iName==currentVolName);
		}

	else	//	RFs::SetVolumeLabel() doesn't work on subst drives
		{
		fs.NotifyChangeCancel();
		User::WaitForRequest(reqStat);
		test.Printf(_L("Cannot set volume label on a substed drive\n"));
		}


//	RFs::Rename()

	test.Next(_L("RFs::Rename()"));
	fs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.Rename(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Toto.doc"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

	r=TheFs.Rename(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Toto.doc"),_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"));
	test_KErrNone(r);

	if(!Is_SimulatedSystemDrive(TheFs,driveNum))
		{// Skip emulator/PlatSim drive C:
		test.Next(_L("RFs::Rename() with max path length"));
		TFileName longName=_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\");
		while(longName.Length()<(KMaxFileName-2))
			longName+=_L("a");
		r=TheFs.Rename(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),longName);
		test_KErrNone(r);
		fs.NotifyChange(ENotifyEntry,reqStat,longName);
		test_Value(reqStat.Int(), reqStat==KRequestPending);
		r=TheFs.Rename(longName,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"));
		test_KErrNone(r);
		User::WaitForRequest(reqStat);
		test_KErrNone(reqStat.Int());
		}

	fs.Close();
	}


static void Test15()

	{
//
//	Repeat Test15 operations in a subtree of that monitored, and ensure notification
//	occurs for a variety of RFile and RFs operations
//
	RFs fs;					//	Session to be notified when a change occurs
	TInt r=fs.Connect();
	test_KErrNone(r);
	r=fs.SetSessionPath(gSessionPath);
	test_KErrNone(r);

//	RFile::Write() to a file in the subtree
	test.Next(_L("RFile::Write()"));
	TFileName path=_L("\\F32-TST\\NOTIFY\\");
	TRequestStatus reqStat(KRequestPending);
	fs.NotifyChange(ENotifyAll,reqStat,path);

	RFile file;
	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),EFileRead|EFileWrite);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=file.Write(0,_L8("Pay no attention to the man behind the curtain"));
	file.Close();
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFile::Read() a file within the monitored directory - no notification for reads
	fs.NotifyChange(ENotifyAll,reqStat,path);
	TBuf8<100> temp;
	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),EFileRead|EFileWrite);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=file.Read(0,temp,100);
	test_Value(reqStat.Int(), reqStat==KRequestPending);

//	RFile::SetAtt() of a file within the monitored directory
	test.Next(_L("RFile::SetAtt()"));
	r=file.SetAtt(KEntryAttNormal,KEntryAttHidden);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFile::SetSize() of a file within the monitored directory
	test.Next(_L("RFile::SetSize()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=file.SetSize(256);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	file.Close();

//	RFile::Temp() to create a temp file in the subtree
	test.Next(_L("RFile::Temp()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	TFileName fileName;
	r=file.Temp(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\"),fileName,EFileWrite);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	file.Close();

//	RFile::SetModified() to change modification time of a file within monitored dir
	test.Next(_L("RFile::SetModified()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	TTime now;
	now.HomeTime();
	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),EFileRead|EFileWrite);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	file.SetModified(now);
	file.Close();
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFs::Entry() to change a directory entry within the monitored directory
	test.Next(_L("RFs::Entry()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	TEntry entry;
	r=TheFs.Entry(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),entry);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	now.HomeTime();
	r=TheFs.SetEntry(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),now,KEntryAttHidden,KEntryAttNormal);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFile::Set() to change file's modification time and attributes
	test.Next(_L("RFile::Set()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),EFileRead|EFileWrite);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	now.HomeTime();
	r=file.Set(now,KEntryAttNormal,KEntryAttHidden);
	file.Close();
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFs::SetDriveName()
	test.Next(_L("RFs::SetDriveName()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	User::After(KNotifyChangeAfter);
	r=TheFs.SetDriveName(KDefaultDrive,_L("DRIVETEST"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	fs.NotifyChange(ENotifyEntry,reqStat,path);
	r=TheFs.SetDriveName(KDefaultDrive,_L("TEST"));
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	User::After(KNotifyChangeAfter);
	fs.NotifyChangeCancel(reqStat);
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);
	fs.NotifyChange(ENotifyDisk,reqStat,path);
	User::After(KNotifyChangeAfter);
	r=TheFs.SetDriveName(KDefaultDrive,_L("DRIVE"));
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	fs.NotifyChangeCancel(reqStat);
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);

//	RFs::MkDir()
	test.Next(_L("RFs::MkDir()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.MkDir(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\EMERALD_CITY\\"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFs::RmDir()
	test.Next(_L("RFs::RmDir()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.RmDir(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\EMERALD_CITY\\"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFile::Create()
	test.Next(_L("RFile::Create()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=file.Create(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Good_Witch.bat"),EFileRead|EFileWrite);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	file.Close();

//	RFs::Delete()
	test.Next(_L("RFs::Delete()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Good_Witch.bat"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFile::Replace()
	test.Next(_L("RFile::Replace()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Good_Witch.bat"),EFileRead|EFileWrite);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	file.Close();

//	RFs::Delete()
	test.Next(_L("RFs::Delete()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Good_Witch.bat"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFs::SetVolumeLabel() - should be notification under relevant TNotifyType monitoring
//	The operation is non-path specific so all outstanding interested requests are notified
	test.Next(_L("RFs::SetVolumeLabel()"));

	fs.NotifyChange(ENotifyAll,reqStat,path);

	TInt driveNum=CurrentDrive();
	TVolumeInfo volInfo;
	TFileName currentVolName;
	r=TheFs.Volume(volInfo,driveNum);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	currentVolName=volInfo.iName;

	r=TheFs.SetVolumeLabel(_L("VOL"),driveNum);
	if (r==KErrNone)
		{
		User::WaitForRequest(reqStat);
		test_KErrNone(reqStat.Int());
		r=TheFs.Volume(volInfo,driveNum);
		test_KErrNone(r);
		test(volInfo.iName==_L("VOL"));
	//	Test notification occurs under ENotifyDisk
		fs.NotifyChange(ENotifyDisk,reqStat,path);
		test_Value(reqStat.Int(), reqStat==KRequestPending);
		r=TheFs.SetVolumeLabel(_L("ABCDEFGHIJK"),driveNum);
		test_KErrNone(r);
		User::WaitForRequest(reqStat);
		test_KErrNone(reqStat.Int());
		r=TheFs.Volume(volInfo,driveNum);
		test_KErrNone(r);

		test(volInfo.iName==_L("ABCDEFGHIJK"));

	//	Test notification does not occur under ENotifyAttributes
		fs.NotifyChange(ENotifyAttributes,reqStat,path);
		r=TheFs.SetVolumeLabel(_L("TROPICANA"),driveNum);
		test_KErrNone(r);
		test_Value(reqStat.Int(), reqStat==KRequestPending);
		r=TheFs.Volume(volInfo,driveNum);
		test_KErrNone(r);

		test(volInfo.iName==_L("TROPICANA"));

		fs.NotifyChangeCancel(reqStat);
		User::WaitForRequest(reqStat);
		test_Value(reqStat.Int(), reqStat==KErrCancel);
	//	Test notification occurs under ENotifyEntry
		fs.NotifyChange(ENotifyEntry,reqStat,path);
		test_Value(reqStat.Int(), reqStat==KRequestPending);
		r=TheFs.SetVolumeLabel(currentVolName,driveNum);
		test_KErrNone(r);
		User::WaitForRequest(reqStat);
		test_KErrNone(reqStat.Int());
		r=TheFs.Volume(volInfo,driveNum);
		test_KErrNone(r);
		test(volInfo.iName==currentVolName);
		}

	else	//	RFs::SetVolumeLabel() doesn't work on subst drives
		{
		fs.NotifyChangeCancel();
		User::WaitForRequest(reqStat);
		test.Printf(_L("Cannot set volume label on a substed drive\n"));
		}



//	Test that notification is made when change is made to monitored directory
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.MkDir(_L("\\F32-TST\\NOTIFY\\EMERALD_CITY\\"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.RmDir(_L("\\F32-TST\\NOTIFY\\EMERALD_CITY\\"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	fs.Close();
	}


static void Test16()

	{
//
//	Repeat Test15 operations in a subtree of that monitored, and ensure notification
//	does occur for a variety of file operations when subtree watching is on
//
	RFs fs;
	TInt r=fs.Connect();	//	Session to be notified when a change occurs
	test_KErrNone(r);
	r=fs.SetSessionPath(gSessionPath);
	test_KErrNone(r);

//	RFile::Write() to a file in the subtree
	TFileName path=_L("\\F32-TST\\NOTIFY\\");
	TRequestStatus reqStat(KRequestPending);
	fs.NotifyChange(ENotifyAll,reqStat,path);

	RFile file;
	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),EFileRead|EFileWrite);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=file.Write(0,_L8("Pay no attention to the man behind the curtain"));
	file.Close();
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFile::Read() a file within the monitored directory - no notification for reads
	fs.NotifyChange(ENotifyAll,reqStat,path);
	TBuf8<100> temp;
	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),EFileRead|EFileWrite);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=file.Read(0,temp,100);
	test_Value(reqStat.Int(), reqStat==KRequestPending);

//	RFile::SetAtt() of a file within the monitored directory
	r=file.SetAtt(KEntryAttNormal,KEntryAttHidden);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFile::SetSize() of a file within the monitored directory
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=file.SetSize(256);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	file.Close();


//	RFile::SetModified() to change modification time of a file within monitored dir
	fs.NotifyChange(ENotifyAll,reqStat,path);
	TTime now;
	now.HomeTime();
	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),EFileRead|EFileWrite);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	file.SetModified(now);
	file.Close();
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFs::Entry() to change a directory entry within the monitored directory
	fs.NotifyChange(ENotifyAll,reqStat,path);
	TEntry entry;
	r=TheFs.Entry(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),entry);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	now.HomeTime();
	r=TheFs.SetEntry(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),now,KEntryAttHidden,KEntryAttNormal);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFile::Set() to change file's modification time and attributes
	fs.NotifyChange(ENotifyAll,reqStat,path);
	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),EFileRead|EFileWrite);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	now.HomeTime();
	r=file.Set(now,KEntryAttNormal,KEntryAttHidden);
	file.Close();
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFs::SetDriveName() - should be no notification ever with extended notification
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	User::After(KNotifyChangeAfter);
	r=TheFs.SetDriveName(KDefaultDrive,_L("DRIVETEST"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	fs.NotifyChange(ENotifyEntry,reqStat,path);
	User::After(KNotifyChangeAfter);
	r=TheFs.SetDriveName(KDefaultDrive,_L("TEST"));
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	fs.NotifyChangeCancel(reqStat);
	User::WaitForRequest(reqStat);
	fs.NotifyChange(ENotifyDisk,reqStat,path);
	User::After(KNotifyChangeAfter);
	r=TheFs.SetDriveName(KDefaultDrive,_L("DRIVE"));
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	fs.NotifyChangeCancel(reqStat);
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);


//	RFs::MkDir()
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.MkDir(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\EMERALD_CITY\\"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFs::RmDir()
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.RmDir(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\EMERALD_CITY\\"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFile::Create()
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=file.Create(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Good_Witch.bat"),EFileRead|EFileWrite);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	file.Close();

//	RFs::Delete()
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Good_Witch.bat"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFile::Replace()
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Good_Witch.bat"),EFileRead|EFileWrite);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	file.Close();

//	RFs::Delete()
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Good_Witch.bat"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	RFs::SetVolumeLabel()
//	Not path specific, so all outstanding requests of correct TNotifyType are notified
	fs.NotifyChange(ENotifyAll,reqStat,path);
	TInt driveNum=CurrentDrive();
	TVolumeInfo volInfo;
	TFileName currentVolName;
	r=TheFs.Volume(volInfo,driveNum);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	currentVolName=volInfo.iName;

	r=TheFs.SetVolumeLabel(_L("VOL"),driveNum);
	if (r==KErrNone)
		{
		User::WaitForRequest(reqStat);
		test_KErrNone(reqStat.Int());
		r=TheFs.Volume(volInfo,driveNum);
		test_KErrNone(r);
		test(volInfo.iName==_L("VOL"));
	//	Test notification occurs under ENotifyDisk
		fs.NotifyChange(ENotifyDisk,reqStat,path);
		test_Value(reqStat.Int(), reqStat==KRequestPending);
		r=TheFs.SetVolumeLabel(_L("ABCDEFGHIJK"),driveNum);
		test_KErrNone(r);
		User::WaitForRequest(reqStat);
		test_KErrNone(reqStat.Int());
		r=TheFs.Volume(volInfo,driveNum);
		test_KErrNone(r);

		test(volInfo.iName==_L("ABCDEFGHIJK"));

	//	Test notification does not occur under ENotifyAttributes
		fs.NotifyChange(ENotifyAttributes,reqStat,path);
		r=TheFs.SetVolumeLabel(_L("TROPICANA"),driveNum);
		test_KErrNone(r);
		test_Value(reqStat.Int(), reqStat==KRequestPending);
		r=TheFs.Volume(volInfo,driveNum);
		test_KErrNone(r);

		test(volInfo.iName==_L("TROPICANA"));

		fs.NotifyChangeCancel(reqStat);
		User::WaitForRequest(reqStat);
		test_Value(reqStat.Int(), reqStat==KErrCancel);
	//	Test notification occurs under ENotifyEntry
		fs.NotifyChange(ENotifyEntry,reqStat,path);
		test_Value(reqStat.Int(), reqStat==KRequestPending);
		r=TheFs.SetVolumeLabel(currentVolName,driveNum);
		test_KErrNone(r);
		User::WaitForRequest(reqStat);
		test_KErrNone(reqStat.Int());
		r=TheFs.Volume(volInfo,driveNum);
		test_KErrNone(r);
		test(volInfo.iName==currentVolName);
		}

	else	//	RFs::SetVolumeLabel() doesn't work on subst drives
		{
		fs.NotifyChangeCancel();
		User::WaitForRequest(reqStat);
		test.Printf(_L("Cannot set volume label on a substed drive\n"));
		}

//	RFs::Rename()
	fs.NotifyChange(ENotifyEntry,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.Rename(_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Dorothy.doc"),_L("\\F32-TST\\NOTIFY\\MUNCHKINS\\Toto.doc"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

//	Test that notification is made when change is made to monitored directory
	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.MkDir(_L("\\F32-TST\\NOTIFY\\EMERALD_CITY\\"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.RmDir(_L("\\F32-TST\\NOTIFY\\EMERALD_CITY\\"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	fs.Close();
	}


static void Test17()
//
//	Test multiple requests from a single session
//
	{

	test.Next(_L("Test reads and writes do not cause notification under ENotifyEntry"));

	RFile file;
	TInt r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\kangaroo.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.Close();
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\koala.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.Close();
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\dingo.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.Close();

	RFs fs;
	r=fs.Connect();
	test_KErrNone(r);
	r=fs.SetSessionPath(gSessionPath);
	test_KErrNone(r);

	TRequestStatus reqStat1(KRequestPending);
	TFileName path1=_L("\\F32-TST\\NOTIFY\\");
	fs.NotifyChange(ENotifyEntry,reqStat1,path1);

	TRequestStatus status1(KRequestPending);
	TRequestStatus status2(KRequestPending);
	TRequestStatus status3(KRequestPending);
	TRequestStatus status4(KRequestPending);
	TRequestStatus status5(KRequestPending);
	TRequestStatus status6(KRequestPending);

	TRequestStatus statusExtended1(KRequestPending);
	TRequestStatus statusExtended2(KRequestPending);
	TRequestStatus statusExtended3(KRequestPending);
	TRequestStatus statusExtended4(KRequestPending);
	TRequestStatus statusExtended5(KRequestPending);
	TRequestStatus statusExtended6(KRequestPending);

//	Request multiple notifications using standard change notification request
	fs.NotifyChange(ENotifyEntry,status1);
	fs.NotifyChange(ENotifyEntry,status2);
	fs.NotifyChange(ENotifyEntry,status3);
	fs.NotifyChange(ENotifyEntry,status4);
	fs.NotifyChange(ENotifyEntry,status5);
	fs.NotifyChange(ENotifyEntry,status6);

//	Request multiple notifications using extended change notification request
	fs.NotifyChange(ENotifyEntry,statusExtended1,path1);
	fs.NotifyChange(ENotifyEntry,statusExtended2,path1);
	fs.NotifyChange(ENotifyEntry,statusExtended3,path1);
	fs.NotifyChange(ENotifyEntry,statusExtended4,path1);
	fs.NotifyChange(ENotifyEntry,statusExtended5,path1);
	fs.NotifyChange(ENotifyEntry,statusExtended6,path1);

	TRequestStatus reqStat2(KRequestPending);
	TFileName path2=_L("\\F32-TST\\NOTIFY\\kangaroo.txt");
	fs.NotifyChange(ENotifyEntry,reqStat2,path2);

	TRequestStatus reqStat3(KRequestPending);
	TFileName path3=_L("\\F32-TST\\NOTIFY\\koala.txt");
	fs.NotifyChange(ENotifyEntry,reqStat3,path3);

	TRequestStatus reqStat4(KRequestPending);
	TFileName path4=_L("\\F32-TST\\NOTIFY\\dingo.txt");
	fs.NotifyChange(ENotifyEntry,reqStat4,path4);


	r=gSleepThread.CreateLocal(0);
	test_KErrNone(r);
	RThread thread1;
	r=thread1.Create(_L("TestThread1"),ThreadEntryPoint,0x4000,KHeapSize,KHeapSize,(TAny*)ETest5);
	test_KErrNone(r);
	thread1.Resume();
	gSleepThread.Wait();

	test_Value(status1.Int(), status1==KRequestPending);
	test_Value(status2.Int(), status2==KRequestPending);
	test_Value(status3.Int(), status3==KRequestPending);
	test_Value(status4.Int(), status4==KRequestPending);
	test_Value(status5.Int(), status5==KRequestPending);
	test_Value(status6.Int(), status6==KRequestPending);

	test_Value(statusExtended1.Int(), statusExtended1==KRequestPending);
	test_Value(statusExtended2.Int(), statusExtended2==KRequestPending);
	test_Value(statusExtended3.Int(), statusExtended3==KRequestPending);
	test_Value(statusExtended4.Int(), statusExtended4==KRequestPending);
	test_Value(statusExtended5.Int(), statusExtended5==KRequestPending);
	test_Value(statusExtended6.Int(), statusExtended6==KRequestPending);

	test_Value(reqStat1.Int(), reqStat1==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\kangaroo.txt"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat1);
	test_KErrNone(reqStat1.Int());
	User::WaitForRequest(status1);
	test_KErrNone(status1.Int());
	User::WaitForRequest(status2);
	test_KErrNone(status2.Int());
	User::WaitForRequest(status3);
	test_KErrNone(status3.Int());
	User::WaitForRequest(status4);
	test_KErrNone(status4.Int());
	User::WaitForRequest(status5);
	test_KErrNone(status5.Int());
	User::WaitForRequest(status6);
	test_KErrNone(status6.Int());

	User::WaitForRequest(statusExtended1);
	test_KErrNone(statusExtended1.Int());
	User::WaitForRequest(statusExtended2);
	test_KErrNone(statusExtended2.Int());
	User::WaitForRequest(statusExtended3);
	test_KErrNone(statusExtended3.Int());
	User::WaitForRequest(statusExtended4);
	test_KErrNone(statusExtended4.Int());
	User::WaitForRequest(statusExtended5);
	test_KErrNone(statusExtended5.Int());
	User::WaitForRequest(statusExtended6);
	test_KErrNone(statusExtended6.Int());

	User::WaitForRequest(reqStat2);
	test_KErrNone(reqStat2.Int());
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	fs.NotifyChangeCancel();	//	Cancels both remaining notification requests

	User::WaitForRequest(reqStat3);
	User::WaitForRequest(reqStat4);

	gSleepThread.Close();
	thread1.Close();

	test.Next(_L("Test reads and writes do cause notification under ENotifyAll"));
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\kangaroo.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.Close();

	fs.NotifyChange(ENotifyAll,reqStat1,path1);
	fs.NotifyChange(ENotifyEntry,reqStat2,path2);
	fs.NotifyChange(ENotifyAll,reqStat3,path3);
	fs.NotifyChange(ENotifyEntry,reqStat4,path4);
	test_Value(reqStat1.Int(), reqStat1==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=gSleepThread.CreateLocal(0);
	test_KErrNone(r);
	RThread thread2;
	r=thread2.Create(_L("TestThread2"),ThreadEntryPoint,0x4000,KHeapSize,KHeapSize,(TAny*)ETest5);
	test_KErrNone(r);
	thread2.Resume();
	gSleepThread.Wait();

	User::WaitForRequest(reqStat1);
	test_KErrNone(reqStat1.Int());
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	gSleepThread.Close();
	thread2.Close();

	fs.NotifyChange(ENotifyAll,reqStat1,path1);
	fs.NotifyChange(ENotifyAll,reqStat3,path3);

	test_Value(reqStat1.Int(), reqStat1==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=gSleepThread.CreateLocal(0);
	test_KErrNone(r);
	RThread thread3;
	r=thread3.Create(_L("TestThread3"),ThreadEntryPoint,0x4000,KHeapSize,KHeapSize,(TAny*)ETest5);
	test_KErrNone(r);
	thread3.Resume();
	gSleepThread.Wait();

	User::WaitForRequest(reqStat1);
	test_KErrNone(reqStat1.Int());
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);	//	Monitoring with ENotifyEntry
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);	//	Monitoring with ENotifyEntry

	RFs fs2;
	r=fs2.Connect();
	test_KErrNone(r);
	r=fs2.SetSessionPath(gSessionPath);
	test_KErrNone(r);

	TRequestStatus reqStat(KRequestPending);
	fs2.NotifyChange(ENotifyEntry,reqStat);
	test_Value(reqStat.Int(), reqStat==KRequestPending);

	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\kangaroo.txt"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat2);
	test_KErrNone(reqStat2.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());

	fs2.NotifyChange(ENotifyAll,reqStat);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\koala.txt"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\dingo.txt"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat4);
	test_KErrNone(reqStat4.Int());

	gSleepThread.Close();
	thread3.Close();
	fs.Close();
	}

static void Test18()

	{
//
//	Test notification request succeeds or fails as appropriate to the notification type
//	with all file operations which result in notifications
//	enum TNotifyType {ENotifyEntry=0x00,ENotifyAll=0x01,ENotifyFile=0x04,ENotifyDir=0x08,
//				ENotifyAttributes=0x10,ENotifyWrite=0x20,ENotifyDisk=0x40};
//
	RFs fs;
	TInt r=fs.Connect();	//	Session to be notified of any changes
	test_KErrNone(r);
	r=fs.SetSessionPath(gSessionPath);
	test_KErrNone(r);

//	RFile::Write() to a file within the monitored directory
	test.Next(_L("RFile::Write()"));
	TFileName path=_L("\\F32-TST\\NOTIFY\\NewFile.txt");
	TRequestStatus reqStat(KRequestPending);
	TRequestStatus reqStat2(KRequestPending);
	TRequestStatus reqStat3(KRequestPending);
	TRequestStatus reqStat4(KRequestPending);
	TRequestStatus reqStat5(KRequestPending);
	TRequestStatus reqStat6(KRequestPending);
	TRequestStatus reqStat7(KRequestPending);


	RFile file;
	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);

	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyFile,reqStat2,path);
	fs.NotifyChange(ENotifyWrite,reqStat3,path);
	fs.NotifyChange(ENotifyDir,reqStat4,path);
	fs.NotifyChange(ENotifyEntry,reqStat5,path);
	fs.NotifyChange(ENotifyAttributes,reqStat6,path);
	fs.NotifyChange(ENotifyDisk,reqStat7,path);


	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrArgument); //	Cannot monitor a file with ENotifyDir
	test_Value(reqStat5.Int(), reqStat5==KRequestPending);
	test_Value(reqStat6.Int(), reqStat6==KRequestPending);
	fs.NotifyChange(ENotifyEntry,reqStat4,path);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=file.Write(0,_L8("Pay no attention to the man behind the curtain"));
	file.Close();

	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);	//	Monitoring with ENotifyFile
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);	//	Monitoring with ENotifyEntry
	test_Value(reqStat5.Int(), reqStat5==KRequestPending);
	test_Value(reqStat6.Int(), reqStat6==KRequestPending);
	test_Value(reqStat7.Int(), reqStat7==KRequestPending);
	fs.NotifyChangeCancel();			//	Cancels all outstanding notification requests

	User::WaitForRequest(reqStat2);
	test_Value(reqStat2.Int(), reqStat2==KErrCancel);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrCancel);
	User::WaitForRequest(reqStat5);
	test_Value(reqStat5.Int(), reqStat5==KErrCancel);
	User::WaitForRequest(reqStat6);
	test_Value(reqStat6.Int(), reqStat6==KErrCancel);
	User::WaitForRequest(reqStat7);
	test_Value(reqStat7.Int(), reqStat7==KErrCancel);

	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);

//	RFile::SetAtt() of a file within the monitored directory
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyFile,reqStat2,path);
	fs.NotifyChange(ENotifyEntry,reqStat3,path);
	fs.NotifyChange(ENotifyAttributes,reqStat4,path);
	fs.NotifyChange(ENotifyDir,reqStat5,path);
	fs.NotifyChange(ENotifyWrite,reqStat6,path);
	fs.NotifyChange(ENotifyDisk,reqStat7,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	User::WaitForRequest(reqStat5);
	test_Value(reqStat5.Int(), reqStat5==KErrArgument);
	test_Value(reqStat6.Int(), reqStat6==KRequestPending);
	test_Value(reqStat7.Int(), reqStat7==KRequestPending);

	test.Next(_L("RFile::SetAtt()"));
	r=file.SetAtt(KEntryAttSystem,KEntryAttNormal);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat4);
	test_KErrNone(reqStat.Int());
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);	//	Monitoring with ENotifyFile
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);	//	Monitoring with ENotifyEntry
	test_KErrNone(reqStat4.Int());				//	Monitoring a file - can't use ENotifyDir
	test_Value(reqStat6.Int(), reqStat6==KRequestPending);
	test_Value(reqStat7.Int(), reqStat7==KRequestPending);

	fs.NotifyChange(ENotifyWrite,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	fs.NotifyChange(ENotifyDir,reqStat4,path);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrArgument);
	r=file.SetAtt(KEntryAttNormal,KEntryAttSystem);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);		//	Monitoring with ENotifyWrite
	fs.NotifyChangeCancel();	//	Cancel outstanding notification request

	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);
	User::WaitForRequest(reqStat2);
	test_Value(reqStat2.Int(), reqStat2==KErrCancel);
	User::WaitForRequest(reqStat3);
	test_Value(reqStat3.Int(), reqStat3==KErrCancel);
	User::WaitForRequest(reqStat6);
	test_Value(reqStat6.Int(), reqStat6==KErrCancel);
	User::WaitForRequest(reqStat7);
	test_Value(reqStat7.Int(), reqStat7==KErrCancel);

	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyFile,reqStat2,path);
	fs.NotifyChange(ENotifyEntry,reqStat3,path);
	fs.NotifyChange(ENotifyAttributes,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

//	RFile::SetSize() of a file within the monitored directory
	test.Next(_L("RFile::SetSize()"));
	r=file.SetSize(256);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat4);
	test_KErrNone(reqStat.Int());
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);	//	Monitoring with ENotifyFile
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);	//	Monitoring with ENotifyEntry
	test_KErrNone(reqStat4.Int());

	fs.NotifyChange(ENotifyWrite,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	fs.NotifyChange(ENotifyDir,reqStat4,path);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrArgument);
	r=file.SetSize(200);
	test_KErrNone(r);
	User::After(1000000);
	test_Value(reqStat.Int(), reqStat==KRequestPending);		//	Monitoring with ENotifyWrite

	file.Close();
	fs.NotifyChangeCancel();			//	Cancels all outstanding notification requests

	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);
	User::WaitForRequest(reqStat2);
	test_Value(reqStat2.Int(), reqStat2==KErrCancel);
	User::WaitForRequest(reqStat3);
	test_Value(reqStat3.Int(), reqStat3==KErrCancel);

//	RFile::Temp() to create a temp file within the monitored directory
	test.Next(_L("RFile::Temp()"));
	path=_L("\\F32-TST\\NOTIFY\\");

	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyDir,reqStat2,path);
	fs.NotifyChange(ENotifyEntry,reqStat3,path);
	fs.NotifyChange(ENotifyAttributes,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	TFileName fileName;
	r=file.Temp(TheFs,path,fileName,EFileWrite);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);	//	Monitoring ENotifyEntry
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);	//	Monitoring ENotifyAttributes
	file.Close();
	fs.NotifyChangeCancel();

	User::WaitForRequest(reqStat2);
	test_Value(reqStat2.Int(), reqStat2==KErrCancel);
	User::WaitForRequest(reqStat3);
	test_Value(reqStat3.Int(), reqStat3==KErrCancel);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrCancel);

	fs.NotifyChange(ENotifyFile,reqStat,path);
	fs.NotifyChange(ENotifyDisk,reqStat2,path);
	fs.NotifyChange(ENotifyWrite,reqStat3,path);
	r=file.Temp(TheFs,path,fileName,EFileWrite);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);	//	Monitoring ENotifyFile
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);	//	Monitoring ENotifyDisk
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);	//	Monitoring ENotifyWrite
	file.Close();

	fs.NotifyChangeCancel();	//	Cancels all outstanding notification requests

	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);
	User::WaitForRequest(reqStat2);
	test_Value(reqStat2.Int(), reqStat2==KErrCancel);
	User::WaitForRequest(reqStat3);
	test_Value(reqStat3.Int(), reqStat3==KErrCancel);

//	RFile::SetModified() to change modification time of a file within monitored dir
	test.Next(_L("RFile::SetModified()"));
	path=_L("\\F32-TST\\NOTIFY\\NewFile.txt");
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyAttributes,reqStat3,path);
	fs.NotifyChange(ENotifyFile,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	TTime now;
	now.HomeTime();
	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	file.SetModified(now);
	file.Close();
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	fs.NotifyChangeCancel();

	User::WaitForRequest(reqStat2);
	test_Value(reqStat2.Int(), reqStat2==KErrCancel);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrCancel);

//	RFs::SetEntry() to change a directory entry within the monitored directory
	test.Next(_L("RFs::SetEntry()"));
	TEntry entry;
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyAttributes,reqStat3,path);
	fs.NotifyChange(ENotifyDisk,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=TheFs.Entry(_L("\\F32-TST\\NOTIFY\\NewFile.txt"),entry);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	now.HomeTime();
	r=TheFs.SetEntry(_L("\\F32-TST\\NOTIFY\\NewFile.txt"),now,KEntryAttHidden,KEntryAttNormal);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat.Int());
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	fs.NotifyChangeCancel();

	User::WaitForRequest(reqStat2);
	test_Value(reqStat2.Int(), reqStat2==KErrCancel);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrCancel);

//	RFile::Set() to change file's modification time and attributes
	test.Next(_L("RFile::Set()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyAttributes,reqStat3,path);
	fs.NotifyChange(ENotifyWrite,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	now.HomeTime();
	r=file.Set(now,KEntryAttNormal,KEntryAttHidden);
	file.Close();
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat.Int());
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	fs.NotifyChangeCancel();

	User::WaitForRequest(reqStat2);
	test_Value(reqStat2.Int(), reqStat2==KErrCancel);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrCancel);

//	RFs::SetDriveName()

	test.Next(_L("RFs::SetDriveName()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyDisk,reqStat3,path);
	fs.NotifyChange(ENotifyAttributes,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	User::After(KNotifyChangeAfter);

	r=TheFs.SetDriveName(KDefaultDrive,_L("DRIVETEST"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	test_KErrNone(reqStat.Int());
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	fs.NotifyChangeCancel();

	User::WaitForRequest(reqStat2);
	test_Value(reqStat2.Int(), reqStat2==KErrCancel);
	User::WaitForRequest(reqStat3);
	test_Value(reqStat3.Int(), reqStat3==KErrCancel);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrCancel);

//	RFs::MkDir()
	test.Next(_L("RFs::MkDir()"));
	path=_L("\\F32-TST\\NOTIFY\\");

	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyDir,reqStat3,path);
	fs.NotifyChange(ENotifyFile,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=TheFs.MkDir(_L("\\F32-TST\\NOTIFY\\EMERALD_CITY\\"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

//	RFs::RmDir()
	test.Next(_L("RFs::RmDir()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyDir,reqStat2,path);
	fs.NotifyChange(ENotifyWrite,reqStat3,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);

	r=TheFs.RmDir(_L("\\F32-TST\\NOTIFY\\EMERALD_CITY\\"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	fs.NotifyChangeCancel();

	User::WaitForRequest(reqStat3);
	test_Value(reqStat3.Int(), reqStat3==KErrCancel);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrCancel);

//	RFile::Create()
	test.Next(_L("RFile::Create()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyDir,reqStat3,path);
	fs.NotifyChange(ENotifyFile,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=file.Create(TheFs,_L("\\F32-TST\\NOTIFY\\Good_Witch.bat"),EFileRead|EFileWrite);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat4);
	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);	//	Monitoring ENotifyDir
	test_KErrNone(reqStat4.Int());
	file.Close();
	fs.NotifyChangeCancel(reqStat3);
	User::WaitForRequest(reqStat3);

	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyDisk,reqStat3,path);
	fs.NotifyChange(ENotifyWrite,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=file.Create(TheFs,_L("\\F32-TST\\NOTIFY\\Bad_Witch.bat"),EFileRead|EFileWrite);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	file.Close();
	fs.NotifyChangeCancel();

	User::WaitForRequest(reqStat3);
	test_Value(reqStat3.Int(), reqStat3==KErrCancel);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrCancel);

//	RFs::Delete()
	test.Next(_L("RFs::Delete()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyDir,reqStat3,path);
	fs.NotifyChange(ENotifyFile,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\Good_Witch.bat"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat4);
	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);	//	Monitoring ENotifyDir
	test_KErrNone(reqStat4.Int());
	fs.NotifyChangeCancel(reqStat3);
	User::WaitForRequest(reqStat3);
	test_Value(reqStat3.Int(), reqStat3==KErrCancel);

	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyAttributes,reqStat3,path);
	fs.NotifyChange(ENotifyAll,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\Bad_Witch.bat"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat4);
	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_KErrNone(reqStat4.Int());
	fs.NotifyChangeCancel(reqStat3);
	User::WaitForRequest(reqStat3);
	test_Value(reqStat3.Int(), reqStat3==KErrCancel);

//	RFile::Replace()
	test.Next(_L("RFile::Replace()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyFile,reqStat3,path);
	fs.NotifyChange(ENotifyDir,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=file.Replace(TheFs,_L("\\F32-TST\\NOTIFY\\Good_Witch.bat"),EFileRead|EFileWrite);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	file.Close();
	fs.NotifyChangeCancel();

	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrCancel);

//	RFs::Delete()
	test.Next(_L("RFs::Delete()"));
	path=_L("\\F32-TST\\NOTIFY\\Good_Witch.bat");
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyFile,reqStat3,path);
	fs.NotifyChange(ENotifyDir,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrArgument);

	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\Good_Witch.bat"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KErrArgument);

//	RFs::SetVolumeLabel()
	test.Next(_L("RFs::SetVolumeLabel()"));
	path=_L("\\F32-TST\\NOTIFY\\");
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyFile,reqStat3,path);
	fs.NotifyChange(ENotifyDir,reqStat4,path);
	fs.NotifyChange(ENotifyWrite,reqStat5,path);
	fs.NotifyChange(ENotifyAttributes,reqStat6,path);
	fs.NotifyChange(ENotifyDisk,reqStat7,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	test_Value(reqStat5.Int(), reqStat5==KRequestPending);
	test_Value(reqStat6.Int(), reqStat6==KRequestPending);
	test_Value(reqStat7.Int(), reqStat7==KRequestPending);

	TInt driveNum=CurrentDrive();
	TVolumeInfo volInfo;
	TFileName currentVolName;
	r=TheFs.Volume(volInfo,driveNum);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	currentVolName=volInfo.iName;

	r=TheFs.SetVolumeLabel(_L("VOL"),driveNum);
	if (r==KErrNone)
		{
		User::WaitForRequest(reqStat);
		User::WaitForRequest(reqStat2);
		User::WaitForRequest(reqStat7);

		test_KErrNone(reqStat.Int());
		test_KErrNone(reqStat2.Int());
		test_Value(reqStat3.Int(), reqStat3==KRequestPending);
		test_Value(reqStat4.Int(), reqStat4==KRequestPending);
		test_Value(reqStat5.Int(), reqStat5==KRequestPending);
		test_Value(reqStat6.Int(), reqStat6==KRequestPending);
		test_KErrNone(reqStat7.Int());

		fs.NotifyChange(ENotifyAll,reqStat,path);
		fs.NotifyChange(ENotifyEntry,reqStat2,path);
		fs.NotifyChange(ENotifyDisk,reqStat7,path);

		test_Value(reqStat.Int(), reqStat==KRequestPending);
		test_Value(reqStat2.Int(), reqStat2==KRequestPending);
		test_Value(reqStat7.Int(), reqStat7==KRequestPending);

		r=TheFs.SetVolumeLabel(currentVolName,driveNum);
		test_KErrNone(r);

		User::WaitForRequest(reqStat);
		User::WaitForRequest(reqStat2);
		User::WaitForRequest(reqStat7);

		test_KErrNone(reqStat.Int());
		test_KErrNone(reqStat2.Int());
		test_Value(reqStat3.Int(), reqStat3==KRequestPending);
		test_Value(reqStat4.Int(), reqStat4==KRequestPending);
		test_Value(reqStat5.Int(), reqStat5==KRequestPending);
		test_Value(reqStat6.Int(), reqStat6==KRequestPending);
		test_KErrNone(reqStat7.Int());

		r=TheFs.Volume(volInfo,driveNum);
		test_KErrNone(r);
		test(volInfo.iName==currentVolName);

		fs.NotifyChangeCancel();

		User::WaitForRequest(reqStat3);
		test_Value(reqStat3.Int(), reqStat3==KErrCancel);
		User::WaitForRequest(reqStat4);
		test_Value(reqStat4.Int(), reqStat4==KErrCancel);
		User::WaitForRequest(reqStat5);
		test_Value(reqStat5.Int(), reqStat5==KErrCancel);
		User::WaitForRequest(reqStat6);
		test_Value(reqStat6.Int(), reqStat6==KErrCancel);
		}

	else	//	RFs::SetVolumeLabel() doesn't work on subst drives
		{
		fs.NotifyChangeCancel();
		User::WaitForRequest(reqStat);
		User::WaitForRequest(reqStat2);
		User::WaitForRequest(reqStat3);
		User::WaitForRequest(reqStat4);
		User::WaitForRequest(reqStat5);
		User::WaitForRequest(reqStat6);
		User::WaitForRequest(reqStat7);
		test.Printf(_L("Cannot set volume label on a substed drive!\n"));
		}


//	RFile::Rename()
	test.Next(_L("RFile::Rename()"));
	path=_L("\\F32-TST\\NOTIFY\\");

	file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileShareExclusive|EFileWrite);
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyFile,reqStat3,path);
	fs.NotifyChange(ENotifyDir,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=file.Rename(_L("\\F32-TST\\NOTIFY\\OldFile.abc"));
	test_Value(r, r == KErrNone||r==KErrAlreadyExists);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);	//	Monitoring ENotifyDir
	file.Close();
	fs.NotifyChangeCancel();
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrCancel);

	path=_L("\\F32-TST\\NOTIFY\\OldFile.abc");

	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\OldFile.abc"),EFileShareExclusive|EFileWrite);
	test_KErrNone(r);
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyFile,reqStat3,path);
	fs.NotifyChange(ENotifyDir,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrArgument);

	r=file.Rename(_L("\\F32-TST\\NOTIFY\\NewFile.xyz"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KErrArgument);
	file.Close();

//	RFs::Rename()
	test.Next(_L("RFs::Rename()"));
	path=_L("\\F32-TST\\NOTIFY\\");
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyFile,reqStat3,path);
	fs.NotifyChange(ENotifyDir,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=TheFs.Rename(_L("\\F32-TST\\NOTIFY\\NewFile.xyz"),_L("\\F32-TST\\NOTIFY\\NewerFile.cat"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);	//	Changed a file not a directory entry
	fs.NotifyChangeCancel();

	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrCancel);

	path=_L("\\F32-TST\\NOTIFY\\NewerFile.cat");
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyFile,reqStat3,path);
	fs.NotifyChange(ENotifyDir,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrArgument);

	r=TheFs.Rename(_L("\\F32-TST\\NOTIFY\\NewerFile.cat"),_L("\\F32-TST\\NOTIFY\\Original.dog"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());

	path=_L("\\F32-TST\\NOTIFY\\");
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyFile,reqStat3,path);
	fs.NotifyChange(ENotifyDir,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=TheFs.Rename(_L("\\F32-TST\\NOTIFY\\"),_L("\\F32-TST\\NOTIFY_TEMP\\"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
	User::WaitForRequest(reqStat4);
	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());	//	Changed a directory entry but notified anyway despite
	test_KErrNone(reqStat4.Int());	//	requesting file notification only because the path we
									//	were monitoring has changed

	path=_L("\\F32-TST\\NOTIFY_TEMP\\Original.dog");
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyFile,reqStat3,path);
	fs.NotifyChange(ENotifyDir,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrArgument);

	r=TheFs.Rename(_L("\\F32-TST\\NOTIFY_TEMP\\"),_L("\\F32-TST\\NOTIFY\\"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat.Int());			//	Modified a directory above the level at which we
	test_KErrNone(reqStat2.Int());			//	are monitoring for changes - we must be notified
	test_KErrNone(reqStat3.Int());			//	anyway because the path has changed

	fs.NotifyChange(ENotifyAll,reqStat,path);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	fs.NotifyChange(ENotifyFile,reqStat3,path);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);

	fs.NotifyChangeCancel(reqStat);
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);
	fs.NotifyChangeCancel(reqStat2);
	User::WaitForRequest(reqStat2);
	test_Value(reqStat2.Int(), reqStat2==KErrCancel);
	fs.NotifyChangeCancel(reqStat3);
	User::WaitForRequest(reqStat3);
	test_Value(reqStat3.Int(), reqStat3==KErrCancel);

	path=_L("\\F32-TST\\NOTIFY\\Original.dog");
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyFile,reqStat3,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);

	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\Original.dog"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());

	path=_L("\\F32-TST\\NOTIFY\\");
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyDir,reqStat3,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);

	r=TheFs.Rename(_L("\\F32-TST\\"),_L("\\F32-TEST\\"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat.Int());			//	Modified a directory above the level at which we
	test_KErrNone(reqStat2.Int());			//	are monitoring for changes but we receive notification
	test_KErrNone(reqStat3.Int());			//	because the notification path has been changed

	fs.NotifyChange(ENotifyAll,reqStat,path);
	//	Notification request is submitted, despite the subject's disappearance
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	//	Notification request is submitted, despite the subject's disappearance
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	fs.NotifyChange(ENotifyFile,reqStat3,path);
	//	Notification request is submitted, despite the subject's disappearance
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);

	fs.NotifyChangeCancel(reqStat);
	User::WaitForRequest(reqStat);
	test_Value(reqStat.Int(), reqStat==KErrCancel);
	fs.NotifyChangeCancel(reqStat2);
	User::WaitForRequest(reqStat2);
	test_Value(reqStat2.Int(), reqStat2==KErrCancel);
	fs.NotifyChangeCancel(reqStat3);
	User::WaitForRequest(reqStat3);
	test_Value(reqStat3.Int(), reqStat3==KErrCancel);

	path=_L("\\F32-TEST\\NOTIFY\\");
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyDir,reqStat3,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);

	r=TheFs.Rename(_L("\\F32-TEST\\NOTIFY\\"),_L("\\F32-TEST\\NOTIFY_CHANGED\\"));
	test_KErrNone(r);

	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);

	test_KErrNone(reqStat.Int());			//	Rename the directory we were monitoring
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());

//	Tidy up the test directory before continuing (while testing notifications of renaming to the monitored directory)

	path=_L("\\F32-TST\\NOTIFY_CHANGED\\");
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyDir,reqStat3,path);
	fs.NotifyChange(ENotifyFile,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=TheFs.Rename(_L("\\F32-TEST\\"),_L("\\F32-TST\\"));
	test_KErrNone(r);

	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);

	test_KErrNone(reqStat.Int());			//	Renaming to (under) the directory we were monitoring
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	fs.NotifyChangeCancel(reqStat4);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrCancel);

	path=_L("\\F32-TST\\NOTIFY\\");
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange(ENotifyEntry,reqStat2,path);
	fs.NotifyChange(ENotifyDir,reqStat3,path);
	fs.NotifyChange(ENotifyFile,reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=TheFs.Rename(_L("\\F32-TST\\NOTIFY_CHANGED\\"),_L("\\F32-TST\\NOTIFY\\"));
	test_KErrNone(r);

	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);

	test_KErrNone(reqStat.Int());			//	Renaming to the directory we were monitoring
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	fs.NotifyChangeCancel(reqStat4);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrCancel);

//	Test combinations of notify types
	test.Next(_L("Test combinations of notify types"));

	path=_L("\\F32-TST\\NOTIFY\\");
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange((TNotifyType)(KGenericEntryChange|ENotifyAttributes),reqStat2,path);
	fs.NotifyChange((TNotifyType)(ENotifyDir|ENotifyFile),reqStat3,path);
	fs.NotifyChange((TNotifyType)(ENotifyDisk|ENotifyAttributes),reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=file.Create(TheFs,_L("\\F32-TST\\NOTIFY\\Munchkin.msg"),EFileRead|EFileWrite);
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);	//	Monitoring ENotifyAttributes|ENotifyDisk
	file.Close();
	fs.NotifyChangeCancel(reqStat4);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrCancel);

//	RFile::SetModified()
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange((TNotifyType)(KGenericEntryChange|ENotifyAttributes),reqStat2,path);
	fs.NotifyChange((TNotifyType)(ENotifyDir|ENotifyFile),reqStat3,path);
	fs.NotifyChange((TNotifyType)(ENotifyDisk|ENotifyAttributes),reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	TTime nowTime;
	nowTime.HomeTime();
	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\Munchkin.msg"),EFileRead|EFileWrite);
	test_KErrNone(r);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	file.SetModified(now);
	file.Close();
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat4);
	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_KErrNone(reqStat4.Int());
	fs.NotifyChangeCancel();
	User::WaitForRequest(reqStat3);
	test_Value(reqStat3.Int(), reqStat3==KErrCancel);

//	RFile::Write()
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange((TNotifyType)(KGenericEntryChange|ENotifyAttributes),reqStat2,path);
	fs.NotifyChange((TNotifyType)(ENotifyFile|ENotifyWrite),reqStat3,path);
	fs.NotifyChange((TNotifyType)(ENotifyDir|ENotifyWrite),reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=file.Open(TheFs,_L("\\F32-TST\\NOTIFY\\Munchkin.msg"),EFileRead|EFileWrite);
	test_KErrNone(r);
	r=file.Write(0,_L8("Pay no attention to the man behind the curtain"));
	file.Close();
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat3);
	User::WaitForRequest(reqStat4);
	test_KErrNone(reqStat.Int());
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_KErrNone(reqStat3.Int());
	test_KErrNone(reqStat4.Int());
	fs.NotifyChangeCancel(reqStat2);	//	Cancels all outstanding notification requests
	User::WaitForRequest(reqStat2);
	test_Value(reqStat2.Int(), reqStat2==KErrCancel);

//	RFs::Delete()
	test.Next(_L("RFs::Delete()"));
	fs.NotifyChange(ENotifyAll,reqStat,path);
	fs.NotifyChange((TNotifyType)(KGenericEntryChange|ENotifyAttributes),reqStat2,path);
	fs.NotifyChange((TNotifyType)(ENotifyFile|ENotifyWrite),reqStat3,path);
	fs.NotifyChange((TNotifyType)(ENotifyDir|ENotifyWrite),reqStat4,path);

	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test_Value(reqStat2.Int(), reqStat2==KRequestPending);
	test_Value(reqStat3.Int(), reqStat3==KRequestPending);
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);

	r=TheFs.Delete(_L("\\F32-TST\\NOTIFY\\Munchkin.msg"));
	test_KErrNone(r);
	User::WaitForRequest(reqStat);
	User::WaitForRequest(reqStat2);
	User::WaitForRequest(reqStat3);
	test_KErrNone(reqStat.Int());
	test_KErrNone(reqStat2.Int());
	test_KErrNone(reqStat3.Int());
	test_Value(reqStat4.Int(), reqStat4==KRequestPending);
	fs.NotifyChangeCancel(reqStat4);
	User::WaitForRequest(reqStat4);
	test_Value(reqStat4.Int(), reqStat4==KErrCancel);

	fs.Close();
	}

#if defined __EPOC32__ && defined __INCLUDE_MANUAL_TESTS__
// Manual media change test
// assumes the media is intially present and then prompts
// the user to remove the media
static void Test99()
	{
	TBuf<64> b;
	b.Format(_L("Test Manual Media change notification (socket:%d)"),gSocketNumber);
	test.Next(b);
	TRequestStatus reqStat=0;

	RFs fs;
	TInt r=fs.Connect();
	test_KErrNone(r);

	TDriveInfo driveInfo;
	TInt driveNum = EDriveC + SocketToDrive[gSocketNumber];

	// verify TDriveInfo.iType == EMediaHardDisk
	r = fs.Drive(driveInfo, driveNum);
	test_KErrNone(r);
	test.Printf(_L("iType = %d\n"), driveInfo.iType);
	test(driveInfo.iType == EMediaHardDisk);


	// ask the user to eject the media
	TheFs.NotifyChange(ENotifyEntry,reqStat);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test.Printf(_L("Please eject media on drive %C...\n"), 'A' + driveNum);
	User::WaitForRequest(reqStat);
	test.Printf(_L("Done.\n"));
	TInt reqInt=reqStat.Int();
	test_KErrNone(reqInt);
	User::WaitForRequest(reqStat);

	// verify TDriveInfo.iType == EMediaNotPresent
	r = fs.Drive(driveInfo, driveNum);
	test_KErrNone(r);
	test.Printf(_L("iType = %d\n"), driveInfo.iType);
	test(driveInfo.iType == EMediaNotPresent);


	// ask the user to re-insert the media
	TheFs.NotifyChange(ENotifyEntry,reqStat);
	test_Value(reqStat.Int(), reqStat==KRequestPending);
	test.Printf(_L("Please re-insert media...\n"));
	User::WaitForRequest(reqStat);
	test.Printf(_L("Done.\n"));
	reqInt = reqStat.Int();
	test_KErrNone(reqInt);
	User::WaitForRequest(reqStat);

	// verify TDriveInfo.iType == EMediaHardDisk
	r = fs.Drive(driveInfo, driveNum);
	test_KErrNone(r);
	test.Printf(_L("iType = %d\n"), driveInfo.iType);
	test(driveInfo.iType == EMediaHardDisk);

	fs.Close();
	}
#endif	// __INCLUDE_MANUAL_TESTS__


//---------------------------------------------
//! @SYMTestCaseID PBASE-T_NOTIFY-0042
//! @SYMTestType UT
//! @SYMREQ REQ5664
//! @SYMTestCaseDesc Test asynchronous RFile API's
//! @SYMTestActions Test normal asynchronous read, share modes, read cancellation, race conditions
//! (with RFile::SetSize() and RFile::Write()), multiple asynchronous readers.
//! @SYMTestExpectedResults Expected behaviour reached.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
static void TestAsyncReader()
//
// Test asynchronous read notifications
//
	{
	TRequestStatus readStat1(KRequestPending);
	TRequestStatus readStat2(KRequestPending);
	TRequestStatus thrdStat(KRequestPending);

	MakeFile(_L("\\F32-TST\\NOTIFY\\NewFile.Txt"));

	test.Next(_L("Test original behaviour of asynchronous read API"));
	RFile reader;
	TInt r=reader.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileShareAny);
	test_KErrNone(r);
	TBuf8<596> readBuf;
	reader.Read(0, readBuf, 100, readStat1);
	User::WaitForRequest(readStat1);
	test_KErrNone(readStat1.Int());
	test_Equal(0, readBuf.Length());
	reader.Close();

	test.Next(_L("Test asynchronous read fails in EFileShareExclusive mode"));
	r=reader.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileReadAsyncAll|EFileShareExclusive);
	test_Value(r, r == KErrArgument);

	test.Next(_L("Test asynchronous read fails in EFileShareReadersOnly mode"));
	r=reader.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileReadAsyncAll|EFileShareReadersOnly);
	test_Value(r, r == KErrArgument);

	test.Next(_L("Test asynchronous read is cancelled when file is closed"));
	r=reader.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileReadAsyncAll|EFileShareAny);
	test_KErrNone(r);
	reader.Read(0, readBuf, 100, readStat1);
	test_Value(readStat1.Int(), readStat1==KRequestPending);
	reader.Close();
	User::WaitForRequest(readStat1);
	test_Value(readStat1.Int(), readStat1==KErrCancel);

	test.Next(_L("Test asynchronous read can be cancelled"));
	r=reader.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileReadAsyncAll|EFileShareAny);
	test_KErrNone(r);
	reader.Read(0, readBuf, 100, readStat1);
	test_Value(readStat1.Int(), readStat1==KRequestPending);
	reader.ReadCancel(readStat1);
	User::WaitForRequest(readStat1);
	test_Value(readStat1.Int(), readStat1==KErrCancel);
	reader.Close();

	// DEF105438: File server thread safety issues
	// Up the priority of this thread so that we can cancel the request before the drive thread
	// runs (to test whether cancelling still works...)
	test.Next(_L("Test asynchronous read is cancelled when running at high priority"));
	r=reader.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileReadAsyncAll|EFileShareAny);
	test_KErrNone(r);
	RThread	thisThread;
	thisThread.SetPriority(EPriorityRealTime);
	reader.Read(0, readBuf, 100, readStat1);
	test_Value(readStat1.Int(), readStat1==KRequestPending);
	reader.ReadCancel(readStat1);
	test.Printf(_L("readStat1 %d"), readStat1.Int());
	User::WaitForRequest(readStat1);
	test_Value(readStat1.Int(), readStat1==KErrCancel);
	reader.Close();
	thisThread.SetPriority(EPriorityNormal);

	test.Next(_L("Test asynchronous read is notified due to RFile::SetSize()"));
	r=reader.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileReadAsyncAll|EFileWrite|EFileShareAny);
	test_KErrNone(r);
	reader.Read(0, readBuf, 100, readStat1);
	test_Value(readStat1.Int(), readStat1==KRequestPending);
	r = reader.SetSize(100);
	test_KErrNone(r);
	User::WaitForRequest(readStat1);
	test_KErrNone(readStat1.Int());
	test_Equal(100, readBuf.Length());
	r=reader.SetSize(0);
	test_KErrNone(r);
	reader.Close();

	test.Next(_L("Test asynchronous read is notified due to RFile::Write()"));
	r=reader.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileReadAsyncAll|EFileWrite|EFileShareAny);
	test_KErrNone(r);
	reader.Read(0, readBuf, 26, readStat1);
	test_Value(readStat1.Int(), readStat1==KRequestPending);
	RFile writer;
	r=writer.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileWrite|EFileShareAny);
	writer.Write(_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
	User::WaitForRequest(readStat1);
	test_KErrNone(readStat1.Int());
	test_Equal(26, readBuf.Length());
	reader.Close();
	writer.Close();

	test.Next(_L("Test multiple asynchronous readers notified from separate thread"));
	r=reader.Open(TheFs,_L("\\F32-TST\\NOTIFY\\NewFile.txt"),EFileRead|EFileReadAsyncAll|EFileWrite|EFileShareAny);
	test_KErrNone(r);
	r=reader.SetSize(0);
	test_KErrNone(r);
	const TInt KReadLen = 26;
	test.Printf(_L(">Read%d[%d]\n"), 0, KReadLen);
	reader.Read(0, readBuf, KReadLen, readStat1);
	TBuf8<596> readBuf2;
	test.Printf(_L(">Read%d[%d]\n"), 1, KReadLen);
	reader.Read(KReadLen, readBuf2, KReadLen, readStat2);
	test_Value(readStat1.Int(), readStat1==KRequestPending);
	test_Value(readStat2.Int(), readStat2==KRequestPending);

	RThread thread;
	r=thread.Create(_L("MyThread"),ThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)ETest12);
	test_KErrNone(r);
	thread.Logon(thrdStat);
	thread.Resume();
	thread.Close();

	RTimer timer;
	TRequestStatus timerStat(KRequestPending);
	timer.CreateLocal();
	timer.After(timerStat, 30000000);	// 30 seconds timeout (the following async test should take 10 seconds)
	test_Value(timerStat.Int(), timerStat==KRequestPending);

	#define ODDPASS  (pass&0x01)
	#define REQSTAT  (ODDPASS ? readStat2 : readStat1)
	#define PENDSTAT (ODDPASS ? readStat1 : readStat2)
	#define COMPLEN  (ODDPASS ? KReadLen  : (pass+1)*KReadLen)
	#define READLEN  (ODDPASS ? KReadLen  : (pass+3)*KReadLen)
	#define READPOS  (ODDPASS ? (pass+2)*KReadLen : 0)
	#define READBUF  (ODDPASS ? readBuf2 : readBuf)

	TInt pass = 0;
	FOREVER
		{
        User::WaitForRequest(REQSTAT, timerStat);
		test_KErrNone(REQSTAT.Int());
		test_Value(timerStat.Int(), timerStat==KRequestPending);
		test_Equal(COMPLEN, READBUF.Length());
		test(READBUF.Right(KReadLen) == _L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
		test.Printf(_L(">Read%d[%d]\n"), pass&0x01, READLEN);
		reader.Read(READPOS, READBUF, READLEN, REQSTAT);
		test_Value(REQSTAT.Int(), REQSTAT==KRequestPending);
		if(++pass==10)
			break;
		}

	test.Next(_L("Close reader and test multiple outstanding requests are cancelled"));
	timer.Close();
	User::WaitForRequest(timerStat);
	reader.Close();
	User::WaitForRequest(readStat1);
	test_Value(readStat1.Int(), readStat1==KErrCancel);
	User::WaitForRequest(readStat2);
	test_Value(readStat2.Int(), readStat2==KErrCancel);

	User::WaitForRequest(thrdStat);
	test_KErrNone(thrdStat.Int());
	}


//-----------------------------------------------------------------------

/**
    Testing that TheFs.NotifyChange() works for the root directory of the drive
*/
void TestRootDirNotifyChange()
{
    test.Next(_L("Testing RFs::NotifyChange() on drive's root directory."));

    TInt nRes;
    TRequestStatus reqStatNotify1(KRequestPending);

    _LIT(KTestPath, "\\"); //-- root dir, actually

    //-- set up notifier
    TheFs.NotifyChange(ENotifyAll, reqStatNotify1, KTestPath);
    test_Value(reqStatNotify1.Int(), reqStatNotify1 == KRequestPending);

    //-- create a file in the root dir
    RFile       file;
    TFileName   fileName(KTestPath);

    fileName.Append(_L("TestFile.tst"));

    nRes=file.Replace(TheFs, fileName, EFileWrite|EFileRead);
    test_Value(nRes, nRes == KErrNone || nRes ==KErrAlreadyExists);
    file.Close();

    //-- check that the notifier worked
    User::WaitForRequest(reqStatNotify1);
    test_KErrNone(reqStatNotify1.Int());

}

_LIT(KTestNotifyFileSystemExeName,"t_tfsys_notify.fsy");
_LIT(KNotifyTestFileSystem,"CNotifyTestFileSystem");

TInt TestExternalNotificationsWellformedness()
    {
    //Do Something to cause a notification:
 
    //********************************************
    //
    // Opening(Replacing) \\Wellformed_functionWrite, 
    //             will issue a create notification on file \PhantomFile_functionWrite.txt
    TRequestStatus status1, status2;
    TheFs.NotifyChange(ENotifyFile,status1,_L("\\PhantomFile_functionReplace.txt"));
    RFile file;
    TInt r = file.Replace(TheFs,_L("\\Wellformed_functionReplace.txt"),EFileWrite);
    test_KErrNone(r);

    
    RTimer timer1;
    r = timer1.CreateLocal();
    test_KErrNone(r);
    TTimeIntervalMicroSeconds32 time = 10000000;
    timer1.After(status2,time);
    User::WaitForRequest(status1,status2);
    test_Compare(status1.Int(),!=,KRequestPending)
    timer1.Cancel();
    timer1.Close();
    User::WaitForRequest(status2);
    
    //********************************************
    //
    // Rename \Wellformed_functionWrite.txt --> \Wellformed_functionRename.txt, 
    // will issue a rename notification on  \PhantomFile_functionWrite.txt --> \PhantomFile_functionRename.txt
    TheFs.NotifyChange(ENotifyEntry,status1,_L("\\PhantomFile_functionRename.txt"));
    r = file.Rename(_L("\\Wellformed_functionWrite.txt"));
    test_KErrNone(r);

    r = timer1.CreateLocal();
    test_KErrNone(r);
    timer1.After(status2,time);
    User::WaitForRequest(status1,status2);
    test_Compare(status1.Int(),!=,KRequestPending);
    timer1.Cancel();
    timer1.Close();
    User::WaitForRequest(status2);
    file.Close();
    
    //********************************************
    //
    // SetAttributes
    //
    TheFs.NotifyChange(ENotifyAttributes,status1,_L("\\PhantomFile_functionAttributes.txt"));
    r = file.Open(TheFs,_L("\\Wellformed_functionAttributes.txt"),EFileWrite);
    test_KErrNone(r);
    r = file.SetAtt(KEntryAttHidden,KEntryAttSystem);
    test_KErrNone(r);
    
    {
        r = timer1.CreateLocal();
        test_KErrNone(r);
        timer1.After(status2,time);
        User::WaitForRequest(status1,status2);
        test_Compare(status1.Int(),!=,KRequestPending);
        timer1.Cancel();
        timer1.Close();
        User::WaitForRequest(status2);
    }
    file.Close();
    
    //********************************************
    //
    // File Write
    // \\Wellformed_functionWrite.txt -> \\PhantomFile_functionWrite.txt
    //
    TheFs.NotifyChange(ENotifyWrite,status1,_L("\\PhantomFile_functionWrite.txt"));
    r = file.Replace(TheFs,_L("\\Wellformed_functionWrite.txt"),EFileWrite);
    test_KErrNone(r);
    TBuf8<4> blah;
    blah.Append(_L("Blah"));
    r = file.Write(blah);
    test_KErrNone(r);
    {
        r = timer1.CreateLocal();
        test_KErrNone(r);
        timer1.After(status2,time);
        User::WaitForRequest(status1,status2);
        test_Compare(status1.Int(),!=,KRequestPending);
        timer1.Cancel();
        timer1.Close();
        User::WaitForRequest(status2);
    }
    
    file.Close();
    return KErrNone;
    }

TInt TestExternalNotificationsMalformed()
    {

    TRequestStatus status1, status2;
    RTimer timer1;
    TInt r = timer1.CreateLocal();
    test_KErrNone(r);
    TTimeIntervalMicroSeconds32 time = 5000000;
    
    //********************************************
    //
    // Don't set file size
    //
    TheFs.NotifyChange(ENotifyWrite,status1,_L("\\PhantomFileMalformed_functionWrite.txt"));
    RFile file;
    r = file.Open(TheFs,_L("\\Malformed_functionWrite.txt"),EFileWrite);
    test_KErrNone(r);
    r = file.Write(_L8("abcd"));
    test_Value(KErrArgument,r);
    {
        r = timer1.CreateLocal();
        test_KErrNone(r);
        timer1.After(status2,time);
        test.Printf(_L("Wait for timeout.."));
        User::WaitForRequest(status1,status2);
        test_Compare(status1.Int(),==,KRequestPending);
        timer1.Cancel();
        timer1.Close();
        TheFs.NotifyChangeCancel(status1);
        User::WaitForRequest(status1);
    }
    
    //********************************************
    //
    // Set new name incorrectly
    //
     TheFs.NotifyChange(ENotifyFile,status1,_L("\\PhantomFileMalformed_functionRename.txt"));
     r = file.Rename(_L("\\Malformed_functionRename.txt"));
     test_Value(KErrArgument,r);
     {
         r = timer1.CreateLocal();
         test_KErrNone(r);
         timer1.After(status2,time);
         test.Printf(_L("Wait for timeout.."));
         User::WaitForRequest(status1,status2);
         test_Compare(status1.Int(),==,KRequestPending)
         timer1.Cancel();
         timer1.Close();
         TheFs.NotifyChangeCancel(status1);
         User::WaitForRequest(status1);
     }
    
    
    file.Close();
    return KErrNone;
    }

void TestExternalNotifications()
    {
    test.Printf(_L("Test External Notifications (Load test file system)"));
    
    if(F32_Test_Utils::Is_SimulatedSystemDrive(TheFs,gDriveNum))
        {
        test.Printf(_L("Not testing External Notifications on SimulatedSystemDrive"));
        return;
        }
    
    test.Printf(_L("Test External Notifications (get file system name)"));
    TBuf<50> filesystem;
    TInt r = TheFs.FileSystemName(filesystem,gDriveNum);
    test_KErrNone(r);
    test.Printf(_L("Test External Notifications (add new file system)"));
    r = TheFs.AddFileSystem(KTestNotifyFileSystemExeName);
    if(r != KErrNone && r!=KErrAlreadyExists)
        {
        test_KErrNone(r);
        }
    test.Printf(_L("Test External Notifications (dismount existing file system)"));
    r = TheFs.DismountFileSystem(filesystem,gDriveNum);
    test_KErrNone(r);
    test.Printf(_L("Test External Notifications (mount new file system)"));
    r = TheFs.MountFileSystem(KNotifyTestFileSystem,gDriveNum);
    test_KErrNone(r);
    
    CHECK_NO_PENDING_REQUESTS;
    
    test.Printf(_L("Test External Notifications (Perform tests)"));
    r = TestExternalNotificationsWellformedness();
    test_KErrNone(r); 
    
    CHECK_NO_PENDING_REQUESTS;
    
// only test this in UREL
// as in DEBUG it PANICS.
#if !defined _DEBUG
    test.Printf(_L("Test External Notifications (Perform malformed tests (UREL only))"));
    r = TestExternalNotificationsMalformed();
    test_KErrNone(r); 
    CHECK_NO_PENDING_REQUESTS;
#endif
    
    test.Printf(_L("Test External Notifications (Replace FS)"));
    //Replace old FS.
    r = TheFs.DismountFileSystem(KNotifyTestFileSystem,gDriveNum);
    test_KErrNone(r);
    r = TheFs.MountFileSystem(filesystem,gDriveNum);
    test_KErrNone(r);
    r = TheFs.RemoveFileSystem(KNotifyTestFileSystem);
    test_KErrNone(r);
    }

//-----------------------------------------------------------------------

//
// Do all tests
//
GLDEF_C void CallTestsL()
	{
    TInt nRes=TheFs.CharToDrive(gDriveToTest, gDriveNum);
    test_KErrNone(nRes);
            
	CreateTestDirectory(_L("\\F32-TST\\NOTIFY\\"));

//	Test RFs::NotifyChange()
	CHECK_NO_PENDING_REQUESTS;
	Test1();
	CHECK_NO_PENDING_REQUESTS;
	Test2();
	CHECK_NO_PENDING_REQUESTS;
	Test3();
	CHECK_NO_PENDING_REQUESTS;
	Test4();
	CHECK_NO_PENDING_REQUESTS;
	Test5();
	CHECK_NO_PENDING_REQUESTS;

#if defined (__EPOC32__)//we have no removable media on Emulator yet	.. ??? we do now
#if defined __INCLUDE_MANUAL_TESTS__

	// Media change notification on every socket supported
	TInt i;
	for (i=0; i<KMaxPBusSockets; i++)
		SocketToDrive[i]=-1;
	TDriveInfoV1Buf dinfo;
	UserHal::DriveInfo(dinfo);
	TDriveInfoV1 &di=dinfo();
	TInt driveCount=dinfo().iTotalSupportedDrives;
	TInt socketCount=dinfo().iTotalSockets;
	TInt drv;
	test.Printf(_L("Socket count: %d\n"), socketCount);
	TUint32 mask=~(0xffffffffu<<socketCount);

	for (drv=0; drv<driveCount; drv++)
		{
		TBool flag=EFalse;
		RLocalDrive d;
		TInt r=d.Connect(drv,flag);

		//Not all the drives are used at present
		if (r == KErrNotSupported)
			continue;

		test_KErrNone(r);
		TInt sockNum = 0;
		if (d.IsRemovable(sockNum)>0)
			{
			if (mask & (1<<sockNum))
				{
				SocketToDrive[sockNum]=drv;
				mask &= ~(1<<sockNum);
				}
			}
		d.Close();
		}


	// Manual media change test
	for (gSocketNumber=socketCount-1; gSocketNumber>=0; gSocketNumber--)
		{
		if (SocketToDrive[gSocketNumber] != -1)
			{
			test.Printf(_L("Testing socket %u, drive %u...\n"),
				gSocketNumber, SocketToDrive[gSocketNumber]);
			Test99();
			}
		}
#endif
    Test6();
#endif
	CHECK_NO_PENDING_REQUESTS;
	Test7();
	CHECK_NO_PENDING_REQUESTS;


#if defined (__EPOC32__)//we have no removable media on Emulator yet

//	Test RFs::NotifyChange() extended notification
	TInt uid;
	test_KErrNone(HAL::Get(HAL::EMachineUid,uid));
	if(uid!=HAL::EMachineUid_Cogent && uid!=HAL::EMachineUid_IQ80310 &&
				uid != HAL::EMachineUid_Integrator && uid!=HAL::EMachineUid_X86PC)
		MediaChangeExtendedNotification();

#endif

	CHECK_NO_PENDING_REQUESTS;
	TestRequestAhead();
	CHECK_NO_PENDING_REQUESTS;
	Test8();
	CHECK_NO_PENDING_REQUESTS;
	Test9();
	CHECK_NO_PENDING_REQUESTS;
	Test10();
	CHECK_NO_PENDING_REQUESTS;
	Test11();
	CHECK_NO_PENDING_REQUESTS;
	Test12();
	CHECK_NO_PENDING_REQUESTS;
	Test13();
	CHECK_NO_PENDING_REQUESTS;
	Test14();
	CHECK_NO_PENDING_REQUESTS;
	Test15();
	CHECK_NO_PENDING_REQUESTS;
	Test16();
	CHECK_NO_PENDING_REQUESTS;
	Test17();
	CHECK_NO_PENDING_REQUESTS;
	Test18();
	CHECK_NO_PENDING_REQUESTS;
	TestAsyncReader();
	CHECK_NO_PENDING_REQUESTS;
	DeleteTestDirectory();
	CHECK_NO_PENDING_REQUESTS;
    TestRootDirNotifyChange();
	CHECK_NO_PENDING_REQUESTS;
    TestExternalNotifications();
    CHECK_NO_PENDING_REQUESTS;
	}
