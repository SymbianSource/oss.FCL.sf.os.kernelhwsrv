// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This test tests reading and writing to the protected & unprotected area of an SD card
// & verifies that mounting & dismounting the protected area does not disrupt reads and/or 
// writes to the unprotected area.
// NB For test to work, a valid key which matches the SD card under test needs to be put 
// into the byte array testDeviceKeyRawData[].
// ********* IMPORTANT NOTE TO SYMBIAN ENGINEERS WORKING WITH THIS TEST *********
// The key MUST NOT BE CHECKED INTO PERFORCE as this would contravene the license agreement
// we have with the SD Card Association.
// ********* IMPORTANT NOTE TO SYMBIAN ENGINEERS WORKING WITH THIS TEST *********
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32cons.h>
#include <f32file.h>
#include <e32test.h>

const TUint32 KDriveNumProt = EDriveG;
const TUint32 KDriveNumUnprot = EDriveF;
TChar gDriveLetterProt;
TChar gDriveLetterUnprot;
TBool gMountProtectedArea = EFalse;

GLDEF_D RTest test(_L("T_SETKEY"));

// flags stolen from locmedia.h
//const TInt KForceMediaChangeReOpenAllMediaDrivers	= 0;
const TUint KForceMediaChangeReOpenMediaDriver		= 0x80000000UL;
const TUint KMediaRemountForceMediaChange			= 0x00000001UL;

class TSDCardSecureMountInfo
	{
public:
	TUid iUid;
	const TDesC8* iEncryptedDeviceKey;
	const TDesC8* iRamMKBData;
	TInt iMKBNumber;
	};
typedef TPckgBuf<TSDCardSecureMountInfo> TSDCardSecureMountInfoPckg;

typedef enum 
	{
	EClearMountInfo=0,
	ESetMountInfo=1
	} TMountInfoAction;


// Standard boilerplate for creating a console window ////////////////////////
void StartAppL();
void SetupConsoleL();

GLDEF_C TInt E32Main()									// main function called by E32
    {
	CTrapCleanup* cleanup=CTrapCleanup::New();			// get clean-up stack
#if defined(_DEBUG) 
	TRAPD(error,SetupConsoleL());						// more initialization, then do example
	__ASSERT_DEBUG(!error,User::Panic(_L("BossTextUi"),error));
#else
	TRAP_IGNORE(SetupConsoleL());
#endif
	delete cleanup;										// destroy clean-up stack
	return 0;											// and return
    }

// Determine whether we should run the full test or just mount the protected area
void ParseCommandLine()
	{
	TBuf<32> args;
	User::CommandLine(args);
	
	if (args == _L("-m"))
		{
		gMountProtectedArea = ETrue;
		}
	else if (args == _L("-?"))
		{
		test.Printf(_L("usage: t_setkey [-m]\n"));
		test.Printf(_L("-m => mount proteced area and exit\n"));
		}
	}

void SetupConsoleL()									// initialize and call example code under cleanup stack
    {
	test.Title();
	test.Start(_L("Starting tests..."));

	ParseCommandLine();
	TRAPD(error,StartAppL());					// perform example function
	if (error) 
		test.Printf(_L("failed: leave code=%d\n"), error);
	else 
		test.Printf(_L("ok\n"));

	test.Printf(_L(" [press any key]\n"));
	test.Getch();

	test.End();
	test.Close();
	
    }

// End of standard boilerplate ///////////////////////////////////////////////


static const TUint8 testDeviceKeyRawData[160] =	
	{

// Symbian test key :
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00


	};

TInt WriteThreadEntryPoint( TAny* aParam )
	{
	(void)aParam;
	
	RTest test(_L("WriteThread"));

	test.Title();
	
	test.Start(_L("Starting WriteThread ..."));

	TInt r;
	RFs fs;

	r = fs.Connect();
	if(r != KErrNone)
		{
		test.Printf(_L("WT Connect err %d\n"), r);
		}

	RFile file;

	TFileName fileNameUnprot = _L("?:\\TESTTHRD.TXT");
	fileNameUnprot[0] = (TText) gDriveLetterUnprot;

	//Open file on the user area drive
	r = file.Replace(fs, fileNameUnprot, EFileWrite);
	if(r != KErrNone)
		{
		test.Printf(_L("WT File Replace err %d\n\n"), r);
		test(0);
		}

	TPtrC8 data(testDeviceKeyRawData, 8);

	RThread().Rendezvous(KErrNone);

	for(TInt iter=0; iter < KMaxTInt;iter++)	// keep sending write requests to the user area for ever
		{
		
		r = file.Write(data);
//test.Printf(_L("WT File Write err %d iter %d\r"), r, iter);

		if(r != KErrNone)
			{
			test.Printf(_L("WT File Write err %d\n"), r);
			test(0);
			}
		}

	file.Close();
	fs.Close();

	test.End();
	test.Close();

	return 0;
	}
    


GLDEF_C void StartAppL()
    {

	// Exit if no key defined
	if (testDeviceKeyRawData[0] == 0 && testDeviceKeyRawData[1] == 0)
		{
		test.Printf(_L("This test needs to be recompiled with a valid key\n"));
		test(0);
		}

	RFs fs;
	TInt r;


	test.Printf(_L("Starting Setkey\n"));
	if(fs.Connect()!=KErrNone)
		return;

	test.Printf(_L("Connected to fileserver OK\n"));
	TBuf8<160> mkBData;

	mkBData.SetLength(160);
	TInt i;
	for (i = 0; i < 160; i++)
		{
		mkBData[i] = testDeviceKeyRawData[i];
		}
	
	TSDCardSecureMountInfoPckg pckg;


	pckg().iRamMKBData=(const TDesC8*)0x01;   //Unused for now.
	pckg().iMKBNumber = 0;
//	pckg().iMKBNumber = 11;		// for SD Binding ?
	static TPtrC8 encryptedDeviceKey(testDeviceKeyRawData, sizeof(testDeviceKeyRawData));
	pckg().iEncryptedDeviceKey = &encryptedDeviceKey;


	r = fs.DriveToChar(KDriveNumProt, gDriveLetterProt);
	r = fs.DriveToChar(KDriveNumUnprot, gDriveLetterUnprot);
	TFileName fileNameUnprot = _L("?:\\TEST.TXT");
	TFileName fileNameProt = _L("?:\\TEST.TXT");
	fileNameUnprot[0] = (TText) gDriveLetterUnprot;
	fileNameProt[0] = (TText) gDriveLetterProt;

	TVolumeInfo v;

	r = fs.Volume(v, KDriveNumProt); //Check
test.Printf(_L("Volume() returned %d\n"), r);
	test(r == KErrNotReady || r == KErrNone);


	if (gMountProtectedArea)
		{
		test.Next(_L("test remount with KForceMediaChangeReOpenMediaDriver flag"));
		// verify that the unprotected area does not get a change notification
		TFileName filePathUnprot = _L("?:\\");
		filePathUnprot[0] = (TText) gDriveLetterUnprot;
		TRequestStatus changeStatus;
		fs.NotifyChange(ENotifyAll, changeStatus, filePathUnprot);

		r = fs.RemountDrive(KDriveNumProt, &pckg, (TUint) KForceMediaChangeReOpenMediaDriver);
		test(r == KErrNone);
		r = fs.Volume(v, KDriveNumProt); //Check
		test.Printf(_L("Volume() returned %d\n"), r);
		test(r == KErrNone || r == KErrCorrupt);

		test.Next(_L("test this causes no change to unprotected drive"));
		test.Printf(_L("changeStatus %d\n"), changeStatus.Int());
		test (changeStatus.Int() == KRequestPending);
		fs.NotifyChangeCancel(changeStatus);

		test.Printf(_L("Remount Drive suceeded.  Secure area is now unlocked\n"));
		fs.Close();
		return;
		}

	// Set the mount info for the secure area using KMediaRemountForceMediaChange flag
	// This is asynchronous in behaviour i.e. we need to wait for (two) media change notifications
	// before the drive is ready for use
	test.Next(_L("test remount with KMediaRemountForceMediaChange flag"));
	TRequestStatus changeStatus;
	fs.NotifyChange(ENotifyAll, changeStatus);
	r = fs.RemountDrive(KDriveNumProt, &pckg, (TUint) KMediaRemountForceMediaChange);
test.Printf(_L("RemountDrive() returned %d\n"), r);
	test(r == KErrNotReady || r == KErrNone);
	
	test.Printf(_L("Waiting for media change...\n"));
	User::WaitForRequest(changeStatus);
	
	do
		{


		r = fs.Volume(v, KDriveNumProt); //Check
		test.Printf(_L("Volume() returned %d\n"), r);

	fs.NotifyChange(ENotifyAll, changeStatus);
		}
	while (r == KErrNotReady);
	fs.NotifyChangeCancel(changeStatus);
	


	// Set the mount info for the secure area using KForceMediaChangeReOpenMediaDriver flag
	// This should be synchronous in behaviour
	test.Next(_L("test remount with KForceMediaChangeReOpenMediaDriver flag"));
	r = fs.RemountDrive(KDriveNumProt, NULL, (TUint) KForceMediaChangeReOpenMediaDriver);
	test(r == KErrNone);
	r = fs.RemountDrive(KDriveNumProt, &pckg, (TUint) KForceMediaChangeReOpenMediaDriver);
	test(r == KErrNone);
	r = fs.Volume(v, KDriveNumProt); //Check
test.Printf(_L("Volume() returned %d\n"), r);
	test(r == KErrNone);

	test.Printf(_L("Remount Drive suceeded.  Secure area is now unlocked\n"));
	test.Printf(_L("Press any key to continue...\n"));


	test.Next(_L("test writing to protected & unprotected areas"));

	RFile f1,f2;
	TRequestStatus req1, req2, req3, req4;

	const TInt KBufSize = 32 * 1024;
	LOCAL_D TBuf8<KBufSize> gBuf;


	test.Printf(_L("Opening files...\n"));
test.Printf(_L("Opening %S...\n"), &fileNameUnprot);
	r = f1.Replace(fs, fileNameUnprot,EFileStreamText|EFileWrite);
	test (r == KErrNone);

test.Printf(_L("Opening %S...\n"), &fileNameProt);
	r = f2.Replace(fs, fileNameProt,EFileStreamText|EFileWrite);
	test (r == KErrNone);
	

//	test.Printf(_L("Wait 10 secs for stack to power down...\n"));
//	User::After(10 * 1000000);
//	test.Printf(_L("done\n"));
	
	gBuf.SetLength(KBufSize);
	
test.Printf(_L("Writing files...\n"));

	req1 = KRequestPending;
	req2 = KRequestPending;
	req3 = KRequestPending;
	req4 = KRequestPending;

	f1.Write(gBuf, req1);
r = fs.RemountDrive(KDriveNumProt, &pckg, (TUint) KForceMediaChangeReOpenMediaDriver);
test (r == KErrNone);
	f2.Write(gBuf, req2);

test.Printf(_L("req1 %d, req2 %d\n"), req1.Int(), req2.Int());
	test (req1 == KRequestPending || req1 == KErrNone);
	test (req2 == KRequestPending || req2 == KErrNone);

	// force a remount to test whether a write to the unprotected area
	// is disrupted.... 
	// NB Should set aFlags to KForceMediaChangeReOpenMediaDriver, otherwise a media change will occur which WILL
	// prematurely complete any pending writes 
r = fs.RemountDrive(KDriveNumProt, &pckg, (TUint) KForceMediaChangeReOpenMediaDriver);
test (r == KErrNone);

//r = fs.RemountDrive(KDriveNumProt, &pckg, 1);
test.Printf(_L("RemountDrive() returned %d\n"), r);
	
	f1.Write(gBuf, req3);
	f2.Write(gBuf, req4);

test.Printf(_L("WaitForRequests...\n"));
	User::WaitForRequest(req1);
	User::WaitForRequest(req2);
	User::WaitForRequest(req3);
	User::WaitForRequest(req4);

test.Printf(_L("req1 %d, req2 %d req3 %d, req4 %d\n"), req1.Int(), req2.Int(), req3.Int(), req4.Int());
	test (req1 == KErrNone);
	test (req2 == KErrNone);
	test (req3 == KErrNone);
	test (req4 == KErrNone);
	
	f1.Close();
	f2.Close();


	// create a thread which continuously writes to the unprotected area
	// whilst continually remounting protected area in this thread.
	
	test.Next(_L("test writing to protected & unprotected areas using different threads"));

test.Printf(_L("Opening %S for read access...\n"), &fileNameUnprot);
	r = f2.Open(fs, fileNameProt, EFileShareReadersOnly);
	test (r == KErrNone);


	test (r == KErrNone);
	RThread thread;
	r = thread.Create( _L("Write Thread"), WriteThreadEntryPoint, KDefaultStackSize, NULL, NULL);
	if(r !=KErrNone)
		{
		test.Printf(_L("MT Thread Create 1 ret = %d\n"),r);
		return;
		}
	TRequestStatus writeThreadStat;
	
	thread.Rendezvous(writeThreadStat);
	thread.Resume();
	User::WaitForRequest(writeThreadStat);
//	thread.Close(); //close handle to thread
	if(writeThreadStat.Int() != KErrNone)
		{
		test.Printf(_L("MT Thread Create 2 ret = %d\n"),writeThreadStat.Int());
		test(0);
		}

	TBuf8<14> buf;


	const TInt KMaxIterations = 1000;
	for(TInt n=0; n<KMaxIterations; n++)
		{
		if (n % 10 == 0)
			test.Printf(_L("iteration %d of %d\r"), n, KMaxIterations);

		//Set the mount info for the secure area.
		r = fs.RemountDrive(KDriveNumProt, &pckg, (TUint) KForceMediaChangeReOpenMediaDriver);
		if (r == KErrNone)
			{
//			test.Printf(_L("Remount Drive KEY suceeded. Secure area is now unlocked\n"));
			}
		else
			{
			test.Printf(_L("Remount Drive KEY failed with %d\n"), r);
			test(0);
			}

		// take it in turns to 
		// (0) call Rfs:Volume()
		// (1) call RFile::Read
		// (2) do nothing
		TInt testNum = n & 0x3;
		if (testNum == 0)
			{
			TVolumeInfo volumeInfo;
			r = fs.Volume( volumeInfo, KDriveNumProt );
			if (r != KErrNone)
				{
				RDebug::Print( _L("Volume returned %d after RemountDrive"), r);
				break;
				}
			}
		else if (testNum == 1)
			{
			r = f2.Read(0,buf);
			if(r != KErrNone)
				{
				test.Printf(_L("protected Drive Access failed %d\n"), r);
				test(0);
				}
			}
		else if (testNum == 2)
			{
			}
		else if (testNum == 3)
			{
			}
		

//test.Printf(_L("protected Drive KEY Access succeeded\n"));
		r = fs.RemountDrive(KDriveNumProt, NULL, (TUint) KForceMediaChangeReOpenMediaDriver);
		if (r == KErrNone)
			{
//			test.Printf(_L("Remount Drive NULL suceeded. Secure area is now locked\n"));
			}
		else
			{
			test.Printf(_L("Remount Drive NULL failed with %d\n"), r);
			test(0);
			}

		if (testNum == 1)
			{
			r = f2.Read(0,buf);
//test.Printf(_L("protected Drive NULL Access failed with %d\n"), r);
			if (r != KErrNotReady)
				{
				test.Printf(_L("protected Drive NULL Access failed with unexpected error %d\n"), r);
				test(0);
				}
			}
	
		}

	f1.Close();


	// comment in the next 2 lines to leave the protected area mounted...
	r = fs.RemountDrive(KDriveNumProt, &pckg, (TUint) KForceMediaChangeReOpenMediaDriver);
	test(r == KErrNone);

	thread.Kill(KErrNone);
	thread.Close(); //close handle to thread

	fs.Close();

	}
