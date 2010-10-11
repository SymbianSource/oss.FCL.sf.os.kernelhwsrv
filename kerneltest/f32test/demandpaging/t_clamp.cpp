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
// f32test\demandpaging\t_clamp.cpp
// Test suite for file clamping, file clamping is used to prevent files
// (exes or dlls) from being deleted whilst in use.
// 002 GetDriveLetters()
// 003 Test1() Basic clamp operation
// 004 Test2() Invalid clamp requests
// 005 Test3() Denied FS requests when file(s) are clamped
// 006 Test3Operations() Test other RFile, RFs operations
// 007 Test3Operations() Increase number of clamps to MY_N
// 008 Test3Operations() Decrease number of clamps by MY_M
// 009 Test3Operations() Increase number of clamps by MY_M
// 010 TestDeferredDismount() Open and clamp file, register for dismount 
// notification, then issue dismount instruction.
// 011 Test4() Clamp tests for non-writable file system
// 012 Test5() Clamp requests on non-clamping file systems
// 
//

//! @SYMTestCaseID			KBASE-T_CLAMP-0328
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1110
//! @SYMTestCaseDesc		Demand Paging File Clamp tests
//! @SYMTestActions			001 Starting T_CLAMP
//! @SYMTestExpectedResults All tests should pass.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented

#define __E32TEST_EXTENSION__

#include <e32test.h>
RTest test(_L("T_CLAMP"));

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)

#include <f32file.h>
#include <f32dbg.h>
#include <e32ldr_private.h>
RFs TheFs;

_LIT(KFATName,"FAT");
//_LIT(KFAT32Name,"FAT32");
_LIT(KROFSName,"ROFS");
_LIT(KLFFSName,"LFFS");
_LIT(KCOMPName,"COMPOSITE"); // Z: name if Composite File System
//#ifdef __WINS__
//_LIT(KROMName,"WIN32");	// Clamping is not supported for non-composite filing system on Z:
//#else
_LIT(KROMName,"ROM");		 // Z: name if ROMFS (on hardware, not emulator)
//#endif

TChar NandFatDrv='?';
TChar RofsDrv='?';
TChar LffsDrv='?';
TChar CompDrv='?';


LOCAL_C void Test1()
	{
// Basic clamp operation
	test.Next(_L("T_Clamp - Test1()"));

	TBuf<256> fileName;	
	TBuf<256> buf(_L("buffer for file used"));

	fileName = _L("clampFile.tst");
	RFile testFile;
	TInt r=testFile.Replace(TheFs,fileName,EFileWrite);
	test(r==KErrNone);
	TPtrC8 pBuf((TUint8*)&buf);
	testFile.Write(pBuf);
	testFile.Flush();

	// Clamp file
	RFileClamp handle;
	r=handle.Clamp(testFile);
	test(r==KErrNone);
	TInt64 storedCookie_0=handle.iCookie[0];
	TInt64 storedCookie_1=handle.iCookie[1];

	// Try to clamp previously-clamped file
	RFileClamp handle1;
	r=handle1.Clamp(testFile);
	test(r==KErrNone);

	// Unclamp file
	r=handle.Close(TheFs);
	test (r==KErrNone);
	// Check cookie content has been re-initialised
	test((0==handle.iCookie[0])&&(0==handle.iCookie[1]));

	// Try to unclamp a file that is not clamped
	handle.iCookie[0]=storedCookie_0;
	handle.iCookie[1]=storedCookie_1;
	r=handle.Close(TheFs);
	test (r==KErrNotFound);

	// Check that attempting to unclamp with a zero-content cookie
	// yields no error
	handle.iCookie[0]=0;
	handle.iCookie[1]=0;
	r=handle.Close(TheFs);
	test (r==KErrNone);

	// Clamp the file (again)
	r=handle.Clamp(testFile);
	test(r==KErrNone);

	// Create and clamp a second file ...
	fileName = _L("clampFile2.tst");
	RFile testFile2;
	r=testFile2.Replace(TheFs,fileName,EFileWrite);
	test(r==KErrNone);
	buf=_L("buffer for file 2");
	testFile2.Write(pBuf);
	testFile2.Flush();
	RFileClamp handle2;
	r=handle2.Clamp(testFile2);
	test(r==KErrNone);

	// Create and clamp a third file ...
	RFileClamp handle3;
	fileName = _L("clampFile3.tst");
	RFile testFile3;
	r=testFile3.Replace(TheFs,fileName,EFileWrite);
	test(r==KErrNone);
	buf=_L("buffer for file 3");
	testFile3.Write(pBuf);
	testFile3.Flush();
	r=handle3.Clamp(testFile3);
	test(r==KErrNone);

	// Test can unclamp then reclamp first file
	// then repeat for the third file
	r=handle.Close(TheFs);
	test (r==KErrNone);
	r=handle.Clamp(testFile);
	test(r==KErrNone);
	r=handle3.Close(TheFs);
	test (r==KErrNone);
	r=handle3.Clamp(testFile3);
	test(r==KErrNone);

	// Tidy up
	r=handle.Close(TheFs);
	test (r==KErrNone);
	r=handle1.Close(TheFs);
	test (r==KErrNone);
	testFile.Close();
	r=TheFs.Delete(_L("clampFile.tst"));
	test (r==KErrNone);

	r=handle2.Close(TheFs);
	test (r==KErrNone);
	testFile2.Close();
	r=TheFs.Delete(_L("clampFile2.tst"));
	test (r==KErrNone);

	r=handle3.Close(TheFs);
	test (r==KErrNone);
	testFile3.Close();
	r=TheFs.Delete(_L("clampFile3.tst"));
	test (r==KErrNone);
	}


LOCAL_C void Test2()
	{
// Invalid clamp requests
	test.Next(_L("T_Clamp - Test2()"));
	
	// Test attempt to clamp empty file is rejected
	RFileClamp handle4;
	TBuf<256> file4Name;	
	file4Name = _L("clampFile4.tst");
	RFile testFile4;
	TInt r=testFile4.Replace(TheFs,file4Name,EFileWrite);
	test(r==KErrNone);
	r=handle4.Clamp(testFile4);
	test(r==KErrEof);

	// Preparation for next test - create a valid clamp handle
	TBuf<256> buf4(_L("buffer for file 4"));
	TPtrC8 pBuf4((TUint8*)&buf4);
	testFile4.Write(pBuf4);
	testFile4.Flush();
	r=handle4.Clamp(testFile4);
	test(r==KErrNone);

	// Try to unclamp non-existant file
	RFileClamp handle5;
	memcpy((TAny*)&handle5,(TAny*)&handle4,sizeof(RFileClamp));
	handle5.iCookie[0] = MAKE_TINT64(-1,-1); // iCookie[0] holds the unique ID
	r=handle5.Close(TheFs);
	test (r==KErrNotFound);

	// Tidy up
	r=handle4.Close(TheFs);
	test (r==KErrNone);
	testFile4.Close();
	r=TheFs.Delete(_L("clampFile4.tst"));
	test (r==KErrNone);
	}



void RemountFileSystem(TInt aDriveNo, TDesC& aFsName, TDesC& aFsExtName0, TDesC& aFsExtName1)
	{
	TInt r;
	if(aFsExtName0.Length() > 0)
		{
		r=TheFs.MountFileSystem(aFsName,aFsExtName0,aDriveNo);
		test(r==KErrNone);
		}
	else if(aFsExtName1.Length() > 0) // untested !
		{
		r=TheFs.MountFileSystem(aFsName,aFsExtName1,aDriveNo);
		test(r==KErrNone);
		}
	else 
		{
		r=TheFs.MountFileSystem(aFsName,aDriveNo);
		test(r==KErrNone);
		}
	}

LOCAL_C void TestDeferredDismount(TDesC& aRoot, TDesC& aFileName, RFileClamp* handlePtr)
	{
	// Open and clamp file, register for dismount notification, then issue 
	// dismount instruction.
	// Since there are no other clients registered for dismount notification,
	// this would normally lead too dismount being instigated. However, since
	// the file is clamped, dismount should be deferred
	test.Next(_L("T_Clamp - TestDeferredDismount()"));

	// File system details required for clean-up
	const TInt KMaxFileSystemNameLength=100; // Arbitrary length
	const TInt KMaxFileSystemExtNameLength=100; // Arbitrary length
	TBuf<KMaxFileSystemNameLength> fsName;
	TBuf<KMaxFileSystemExtNameLength> fsExtName_0;
	TBuf<KMaxFileSystemExtNameLength> fsExtName_1;
	TInt driveNo, r;
	r=TheFs.CharToDrive(aRoot[0], driveNo);
	test(r==KErrNone);
	r=TheFs.FileSystemName(fsName, driveNo);
	test(r==KErrNone);

	//*******************************************************************************************************
	// Create a file & write to it so that we can test whether dismounting works correctly with dirty data
	//*******************************************************************************************************
	test.Next(_L("T_Clamp - TestDeferredDismount(), testing unmounting with dirty data, registered clients & clamped files"));

	TDriveInfo driveInfo;
	test(TheFs.Drive(driveInfo, driveNo) == KErrNone);
	TFileName dirtyFileName(_L("dirtyFile.tst"));
	RFile dirtyFile;
	TBool writeProtectedMedia = driveInfo.iMediaAtt & KMediaAttWriteProtected;
	if (!writeProtectedMedia)
		{
		r=dirtyFile.Replace(TheFs, dirtyFileName, EFileWrite);
		test(r==KErrNone);
		r=dirtyFile.Write(_L8("My name is Michael Caine"));
		test(r==KErrNone);
		}


	RFile testFile;
	r=testFile.Open(TheFs,aFileName,EFileRead);
	test(r==KErrNone);
	r=handlePtr->Clamp(testFile);
	test(r==KErrNone);
	testFile.Close();

	TRequestStatus clientNotify=KErrNone;
	TRequestStatus clientDismount=KErrNone;
	TheFs.NotifyDismount(driveNo, clientNotify); // Register for dismount notification
	test(clientNotify == KRequestPending);

	// register for disk change notifcation, so we can detect when dismount has actually taken place
	TRequestStatus diskChangeStatus;
	TheFs.NotifyChange(ENotifyDisk, diskChangeStatus);
	test.Printf(_L("diskChangeStatus %d"), diskChangeStatus.Int());
	test(diskChangeStatus == KRequestPending); // no disk change yet
	

	TheFs.NotifyDismount(driveNo, clientDismount, EFsDismountNotifyClients);
	test(clientDismount == KRequestPending);
	User::WaitForRequest(clientNotify);
	test(clientNotify == KErrNone);

	r=TheFs.AllowDismount(driveNo);	// Respond to dismount notification
	test(r == KErrNone);
	test(clientDismount == KRequestPending); // Dismount is deferred
	test(diskChangeStatus == KRequestPending); // no disk change yet

	//
	// Now unclamp the file, and check that the deferred dismount is performed.
	r=handlePtr->Close(TheFs);
	test(r==KErrNone);
	User::WaitForRequest(clientDismount);
	test(clientDismount == KErrNone);	

	// wait for disk change notification following dismount
	User::WaitForRequest(diskChangeStatus);
	test(diskChangeStatus == KErrNone); // should have got a disk change notification after dismount
	r = TheFs.Drive(driveInfo, driveNo);
	test (r==KErrNone);
	test (driveInfo.iType == EMediaNotPresent);

	// re-register for disk change notifcation, so we can detect when remount has actually taken place
	TheFs.NotifyChange(ENotifyDisk, diskChangeStatus);
	test.Printf(_L("diskChangeStatus %d"), diskChangeStatus.Int());
	test(diskChangeStatus == KRequestPending); // no disk change yet
	
	// Try to write to the opened file: this should return KErrNotReady as there is no drive thread
	if (!writeProtectedMedia)
		{
		r=dirtyFile.Write(_L8("My name isn't really Michael Caine"));
		test(r==KErrNotReady);
		}

	// Re-mount the file system
	RemountFileSystem(driveNo, fsName, fsExtName_0, fsExtName_1);

	// wait for disk change notification following remount
	User::WaitForRequest(diskChangeStatus);
	test(diskChangeStatus == KErrNone); // should have got a disk change notification after dismount
	r = TheFs.Drive(driveInfo, driveNo);
	test (r==KErrNone);
	test (driveInfo.iType != EMediaNotPresent);

	// create some more dirty data to verify that the file server can cope with the drive thread 
	// having gone & come back again
	if (!writeProtectedMedia)
		{
		r=dirtyFile.Write(_L8("My name is Michael Phelps and I'm a fish."));
		test(r==KErrDisMounted);

		dirtyFile.Close();
		r = TheFs.Delete(dirtyFileName);
		test(r == KErrNone);
		}

	//*******************************************************************************************************
	// Issue a EFsDismountNotifyClients with no clients but with files clamped
	// & verify that the dismount request completes when clamps are removed
	//*******************************************************************************************************
	test.Next(_L("T_Clamp - TestDeferredDismount(), testing unmounting with no registered clients & clamped files"));

	r=testFile.Open(TheFs,aFileName,EFileRead);
	test(r==KErrNone);
	r=handlePtr->Clamp(testFile);
	test(r==KErrNone);
	testFile.Close();
	TheFs.NotifyDismount(driveNo, clientDismount, EFsDismountNotifyClients);
	
	test(clientDismount == KRequestPending);
	r=handlePtr->Close(TheFs);
	test(r==KErrNone);
	User::WaitForRequest(clientDismount);
	test(clientDismount == KErrNone);	
	// Re-mount the file system again
	RemountFileSystem(driveNo, fsName, fsExtName_0, fsExtName_1);


	// Issue a EFsDismountForceDismount with no clients but with files clamped
	// & verify that the dismount request completes when clamps are removed
	r=testFile.Open(TheFs,aFileName,EFileRead);
	test(r==KErrNone);
	r=handlePtr->Clamp(testFile);
	test(r==KErrNone);
	testFile.Close();
	TheFs.NotifyDismount(driveNo, clientDismount, EFsDismountForceDismount);
	
	test(clientDismount == KRequestPending);
	r=handlePtr->Close(TheFs);
	test(r==KErrNone);
	User::WaitForRequest(clientDismount);
	test(clientDismount == KErrNone);	


	// Re-mount the file system again
	RemountFileSystem(driveNo, fsName, fsExtName_0, fsExtName_1);


	const TInt KNumClients = 5;
    RFs clientFs[KNumClients];
	TRequestStatus clientNotifies[KNumClients];
	TRequestStatus clientDiskChanges[KNumClients];
	TRequestStatus clientComplete;

	#define LOG_AND_TEST(a, e) {if (a!=e) {test.Printf(_L("lvalue %d, rvalue%d\n\r"), a,e); test(EFalse);}}
	
	//*******************************************************************************************************
	// Test unmounting with multiple registered clients which do not respond & close their sessions
	//*******************************************************************************************************
	test.Next(_L("T_Clamp - TestDeferredDismount(), testing unmounting with multiple registered clients which do not respond & close their sessions"));

	TheFs.NotifyChange(ENotifyDisk, diskChangeStatus);
	test.Printf(_L("diskChangeStatus %d"), diskChangeStatus.Int());

	TInt i;
	for (i=0; i< KNumClients; i++)
		{
		LOG_AND_TEST(KErrNone, clientFs[i].Connect());
   		clientFs[i].NotifyDismount(driveNo, clientNotifies[i]);
		test(clientNotifies[i] == KRequestPending);
		}

	test.Next(_L("Close all but one client sessions with outstanding notifiers"));
	for (i=0; i< KNumClients-1; i++)
		clientFs[i].Close();

	// Since all clients have NOT been closed, the next stage should not yet complete
	test.Next(_L("Notify clients of pending media removal and check status - should not complete"));
	TheFs.NotifyDismount(driveNo, clientComplete, EFsDismountNotifyClients);
	test(clientComplete == KRequestPending);


	test.Next(_L("Close the remaining sessions with an outstanding notifier"));
	clientFs[KNumClients-1].Close();

	// Check that the dismount completes now that all session have been closed
	test.Next(_L("Check that the dismount completes"));
    User::WaitForRequest(clientComplete);
	test_KErrNone(clientComplete.Int());

	// wait for disk change notification following dismount
	User::WaitForRequest(diskChangeStatus);
	test(diskChangeStatus == KErrNone); // should have got a disk change notification after dismount
	r = TheFs.Drive(driveInfo, driveNo);
	test (r==KErrNone);
	test (driveInfo.iType == EMediaNotPresent);

	// re-register for disk change notifcation, so we can detect when remount has actually taken place
	TheFs.NotifyChange(ENotifyDisk, diskChangeStatus);
	test.Printf(_L("diskChangeStatus %d"), diskChangeStatus.Int());
	test(diskChangeStatus == KRequestPending); // no disk change yet
	

	// Re-mount the file system again
	RemountFileSystem(driveNo, fsName, fsExtName_0, fsExtName_1);

	// wait for disk change notification following remount
	User::WaitForRequest(diskChangeStatus);
	test(diskChangeStatus == KErrNone); // should have got a disk change notification after dismount
	r = TheFs.Drive(driveInfo, driveNo);
	test (r==KErrNone);
	test (driveInfo.iType != EMediaNotPresent);

	

	//*******************************************************************************************************
	// Issue a EFsDismountNotifyClients with multiple clients 
	// Verify that the dismount completes if a client re-registers for dismount notifications BEFORE calling AllowDismount
	//*******************************************************************************************************

	test.Next(_L("T_Clamp - TestDeferredDismount(), testing unmounting with multiple registered clients & a re-registration"));

	for(i=0; i< KNumClients; i++)
		{
   		r=clientFs[i].Connect();
		test(r==KErrNone);
		}
	// Cancel any deferred dismount in preparation for the next test
	TheFs.NotifyDismountCancel();

	// All clients register for dismount notification
	for(i=0; i< KNumClients; i++)
		{
		clientNotifies[i] = KErrNone;
   		clientFs[i].NotifyDismount(driveNo, clientNotifies[i]);
		test(clientNotifies[i] == KRequestPending);
		}
	
	// Issue a EFsDismountNotifyClients & wait for clients to respond
	clientDismount = KErrNone;
   	TheFs.NotifyDismount(driveNo, clientDismount, EFsDismountNotifyClients);
	test(clientDismount == KRequestPending);

	// Check all clients have received the notification
	for(i=0; i< KNumClients; i++)
		{
		User::WaitForRequest(clientNotifies[i]);
		test(clientNotifies[i] == KErrNone);
		}
	// All clients - except first one - invoke AllowDismount
	for(i=1; i< KNumClients; i++)
		{
		r=clientFs[i].AllowDismount(driveNo);
		test(r==KErrNone);
		}


	// verify dismount has not yet completed
	test(clientDismount == KRequestPending);


	// first client re-registers for dismount notifications
	clientFs[0].NotifyDismount(driveNo, clientNotifies[0]);
	test(clientNotifies[0] == KRequestPending);

	// first client allows dismount
	clientFs[0].AllowDismount(driveNo);
	test(r==KErrNone);

	// Wait for dismount
	User::WaitForRequest(clientDismount);
	test(clientDismount == KErrNone);


	// verify the first client's re-issued dismount notification is still pending
	test(clientNotifies[0] == KRequestPending);


	// Re-mount the file system again
	RemountFileSystem(driveNo, fsName, fsExtName_0, fsExtName_1);


	// Issue a EFsDismountNotifyClients again & check previously re-registered notification completes
	test(clientNotifies[0] == KRequestPending);
	clientDismount = KErrNone;
   	TheFs.NotifyDismount(driveNo, clientDismount, EFsDismountNotifyClients);
	test(clientDismount == KRequestPending);

	// wait for notification
	User::WaitForRequest(clientNotifies[0]);
	
	// first client allows dismount
	clientFs[0].AllowDismount(driveNo);
	test(r==KErrNone);

	// Wait for dismount
	User::WaitForRequest(clientDismount);
	test(clientDismount == KErrNone);



	// Re-mount the file system again
	RemountFileSystem(driveNo, fsName, fsExtName_0, fsExtName_1);

	

	//*******************************************************************************************************
	// Issue a EFsDismountNotifyClients again with a multiple clients & then call RFs::NotifyDismountCancel()
	// Verify that all clients receive a disk change notification
	//*******************************************************************************************************
	test.Next(_L("T_Clamp - TestDeferredDismount(), testing unmounting with registered clients, a re-registration & a cancel"));
	
	// All clients register for dismount notification & disk change notification
	for(i=0; i< KNumClients; i++)
		{
		clientNotifies[i] = KErrNone;
   		clientFs[i].NotifyDismount(driveNo, clientNotifies[i], EFsDismountRegisterClient);
		test(clientNotifies[i] == KRequestPending);
		
		clientFs[i].NotifyChange(ENotifyDisk, clientDiskChanges[i]);
		test.Printf(_L("diskChangeStatus %d"), clientDiskChanges[i].Int());
		test(clientDiskChanges[i] == KRequestPending);
		}
	

	// Issue a EFsDismountNotifyClients
   	TheFs.NotifyDismount(driveNo, clientComplete, EFsDismountNotifyClients);
	test(clientComplete == KRequestPending);


	// Check all clients have received the notification
	for(i=0; i< KNumClients; i++)
		{
		User::WaitForRequest(clientNotifies[i]);
		test(clientNotifies[i] == KErrNone);

		test.Printf(_L("diskChangeStatus %d"), clientDiskChanges[i].Int());
		test(clientDiskChanges[i] == KRequestPending);
		}

	// verify dismount has not yet completed
	test(clientComplete == KRequestPending);


	// first client re-registers for dismount notifications
	clientFs[0].NotifyDismount(driveNo, clientNotifies[0]);
	test(clientNotifies[0] == KRequestPending);

	// first client acknowledges the dismount request
	r = clientFs[0].AllowDismount(driveNo);
	test(r == KErrNone);

	
	// cancel dismount
//	TheFs.NotifyDismountCancel(clientComplete);
	TheFs.NotifyDismountCancel();
	test(clientComplete == KErrCancel);
	User::WaitForRequest(clientComplete);

	// Check all clients have received a disk change notification - 
	// the file server should send a disk change notification when RFs::NotifyDismountCancel() is called
	for(i=0; i< KNumClients; i++)
		{
		User::WaitForRequest(clientDiskChanges[i]);
		test.Printf(_L("diskChangeStatus %d"), clientDiskChanges[i].Int());
		test(clientDiskChanges[i] == KErrNone);
		}


	// cleanup
	for(i=0; i< KNumClients; i++)
		{
   		clientFs[i].Close();
		test(r==KErrNone);
		}
	}



LOCAL_C void Test3Operations(TDesC& aRoot, TDesC& aFileName)
	{
	test.Next(_L("T_Clamp - Test3Operations()"));
	// RFormat::Open
#ifdef __WINS__
	if (User::UpperCase(aRoot[0]) != 'C')
#endif
		{
		TBuf<4> driveBuf=_L("?:\\");
		driveBuf[0] = aRoot[0];
		RFormat format;
		TInt count;
		TInt r=format.Open(TheFs,driveBuf,EFullFormat,count);
		test(r==KErrInUse);
		format.Close();
		}

	// Dismount: synchronous requests
	// RFs::DismountFileSystem, RFs::SwapFileSystem
	const TInt KMaxFileSystemNameLength=100; // Arbitrary length
	TBuf<KMaxFileSystemNameLength> fileSysName;
	TInt driveNo, r;
	r=TheFs.CharToDrive(aRoot[0], driveNo);
	test(r==KErrNone);
	r=TheFs.FileSystemName(fileSysName,driveNo);
	test(r==KErrNone);

	r=TheFs.DismountFileSystem(fileSysName,driveNo);
	test(r==KErrInUse);
	
	r=TheFs.SwapFileSystem(fileSysName,fileSysName,driveNo);
	test(r==KErrInUse);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	// The cancellation of deferred dismounts requires controlIO 
	// functionality available in debug versions of the code.

	// Dismount: asynchronous requests
	// RFs::NotifyDismount, RFs::AllowDismount
	const TInt KNumClients = 5;
    RFs clientFs[KNumClients];
	TRequestStatus clientNotify[KNumClients];
	TRequestStatus clientComplete;
	TInt i=0;
	for(i=0; i< KNumClients; i++)
		{
   		r=clientFs[i].Connect();
		test(r==KErrNone);
		}
	// Cancel any deferred dismount in preparation for the next test
	r=TheFs.ControlIo(driveNo,KControlIoCancelDeferredDismount);
	test(r==KErrNone);	

	// Use case 1: Orderly dismount
	// All clients register for dismount notification
	for(i=0; i< KNumClients; i++)
		{
		clientNotify[i] = KErrNone;
   		clientFs[i].NotifyDismount(driveNo, clientNotify[i]);
		test(clientNotify[i] == KRequestPending);
		}
	// First client notifies intent to dismount
	clientComplete = KErrNone;
   	clientFs[0].NotifyDismount(driveNo, clientComplete, EFsDismountNotifyClients);
	test(clientComplete == KRequestPending);
	// Check all clients have received the notification
	for(i=0; i< KNumClients; i++)
		{
		test(clientNotify[i] == KErrNone);
		}
	// All clients invoke AllowDismount
	for(i=0; i< KNumClients; i++)
		{
		r=clientFs[i].AllowDismount(driveNo);
		test(r==KErrNone);
		}
	// Dismount is deferred
	test(clientComplete == KRequestPending);


	// Cancel the deferred dismount in preparation for the next test
	clientFs[0].NotifyDismountCancel(clientComplete);
	test(clientComplete == KErrCancel);
	r=TheFs.ControlIo(driveNo,KControlIoCancelDeferredDismount);
	test(r==KErrNone);	
	clientComplete=KErrNone; // Re-initialise the TRequestStatus


	// Use case 2: Forced dismount
	// All clients register for dismount notification
	for(i=0; i< KNumClients; i++)
		{
   		clientFs[i].NotifyDismount(driveNo, clientNotify[i]);
		test(clientNotify[i] == KRequestPending);
		}
	// First client notifies intent to dismount
   	clientFs[0].NotifyDismount(driveNo, clientComplete, EFsDismountNotifyClients);
	test(clientComplete == KRequestPending);
	// Check all clients have received the notification
	for(i=0; i< KNumClients; i++)
		{
		test(clientNotify[i] == KErrNone);
		}
	// Not all other clients invoke AllowDismount
	for(i=0; i< KNumClients-1; i++)
		{
		clientFs[i].AllowDismount(driveNo);
		}
	// First client attempts forced dismount
	test(clientComplete == KRequestPending);
   	clientFs[0].NotifyDismount(driveNo, clientComplete, EFsDismountForceDismount);
	// Dismount is deferred
	test(clientComplete == KRequestPending);

	// Cancel the deferred dismount in preparation for the next test
	// Also cancel the 'un-Allowed' notification request
	clientFs[0].NotifyDismountCancel(clientComplete);
	test(clientComplete == KErrCancel);
	r=TheFs.ControlIo(driveNo,KControlIoCancelDeferredDismount);
	test(r==KErrNone);	
	clientComplete=KErrNone; // Re-initialise the TRequestStatus
#endif

	// RFile::Open with EFileWrite
	RFile testFile;
	r=testFile.Open(TheFs,aFileName,EFileWrite|EFileShareReadersOrWriters);
	test(r==KErrInUse);

	// RFile::Replace
	RFile testFile2;
	r=testFile2.Replace(TheFs,aFileName,EFileRead);
	test(r==KErrInUse);
	testFile2.Close();

	// RFile::Set - this should not be prevented by clamping
	r=testFile.Open(TheFs,aFileName,EFileRead|EFileShareAny);
	test(r == KErrNone);

	TTime origTime;
	TUint origAtt;
	r=testFile.Att(origAtt);
	test(r==KErrNone);
	r=testFile.Modified(origTime);
	test(r==KErrNone);

	TTime time;									// Arbitrary value
	TUint setMask=0xA5A5&~KEntryAttReadOnly;	// Not read-only, otherwise arbitrary value
	TUint clearMask=0x5A5A & KEntryAttReadOnly;	// Not read-only, otherwise arbitrary value
	r=testFile.Set(time,setMask,clearMask);
	test(r==KErrNone);

	r=testFile.Set(origTime,origAtt,~origAtt); // restore original values
	test(r==KErrNone);
	testFile.Close();

	// RFs::Rename - this should not be prevented by clamping
	r=TheFs.Rename(aFileName,_L("aDummyName"));
	test(r==KErrNone);
	r=TheFs.Rename(_L("aDummyName"),aFileName); // restore original name
	test(r==KErrNone);

	// RFs::Replace
	r=TheFs.Replace(aFileName,_L("aDummyName"));
	test(r==KErrInUse);

	// RFs::SetEntry - this should not be prevented by clamping
	r=TheFs.SetEntry(aFileName,time,setMask,clearMask);
	test(r==KErrNone);
	r=TheFs.SetEntry(aFileName,origTime,origAtt,~origAtt); // restore original values
	test(r==KErrNone);

	// RFs::Delete
	r=TheFs.Delete(aFileName);
	test(r==KErrInUse);

	// RRawDisk::Open (*** no longer required ***)
	}

LOCAL_C void Test3(TDesC& aRoot)
	{
// Denied FS requests when file(s) are clamped.
	test.Next(_L("T_Clamp - Test3()"));

// Clamping is reference counted, so we need a test to check that
// a file clamped N times cannot be modified until it has been unclamped N times.
// Should also check
// - Clamp N times
// - Unclamp M times (M<N)
// - Clamp M times.
// - Unclamp N times

#define MY_N 16
#define MY_M 12

	// Create a file for use
	TBuf<256> fileName;	
	TBuf<256> buf(_L("buffer for file used"));
	fileName = _L("clampFile.tst");
	RFile testFile;
	TInt r=testFile.Replace(TheFs,fileName,EFileWrite);
	test(r==KErrNone);
	TPtrC8 pBuf((TUint8*)&buf);
	testFile.Write(pBuf);
	testFile.Flush();
	// Close file,then re-open (to support clamping) in sharable mode
	// (to allow testing of RFile::Open with EFileWrite)
	testFile.Close();
	r=testFile.Open(TheFs,fileName,EFileWrite|EFileShareReadersOrWriters);
	test(r==KErrNone);
	// Show, prior to clamping, that the file can be opened with EFileWrite
	RFile testFile2;
	r=testFile2.Open(TheFs,fileName,EFileWrite|EFileShareReadersOrWriters);
	test(r==KErrNone);
	// Close the second RFile instance
	testFile2.Close();

	// Clamp and unclamp a number of times, and invoke the
	// operations to test
	RFileClamp myHandles[MY_N];
	RFileClamp *handlePtr = myHandles;
	TInt i = 0;

	// Clamp once
	r=handlePtr->Clamp(testFile);
	test(r==KErrNone);
	i++;

	// RFile::SetAtt - this should not be prevented by clamping
	TTime origTime;
	TUint origAtt;
	r=testFile.Att(origAtt);
	test(r==KErrNone);
	r=testFile.Modified(origTime);
	test(r==KErrNone);
	TTime time;									// Arbitrary value
	TUint setMask=0xA5A5&~KEntryAttReadOnly;	// Not read-only, otherwise arbitrary value
	TUint clearMask=0x5A5A & KEntryAttReadOnly;	// Not read-only, otherwise arbitrary value
	r=testFile.Att(origAtt);
	test(r==KErrNone);
	r=testFile.SetAtt(setMask,clearMask);
	test(r==KErrNone);
	r=testFile.Set(origTime,origAtt,~origAtt); // restore original values
	test(r==KErrNone);

	// RFile::SetModified - this should not be prevented by clamping
	r=testFile.Modified(origTime);
	test(r==KErrNone);
	r=testFile.SetModified(time);
	test(r==KErrNone);
	r=testFile.SetModified(origTime); // restore original value
	test(r==KErrNone);

	// RFile::Rename - this should not be prevented by clamping
	// Need file to be opened in EFileShareExclusive sharing mode,
	// so close, unclamp, re-open appropriately and re-clamp
	testFile.Close();
	r=handlePtr->Close(TheFs);
	test(r==KErrNone);
	i--;
	r=testFile.Open(TheFs,fileName,EFileWrite|EFileShareExclusive);
	test(r==KErrNone);
	r=handlePtr->Clamp(testFile);
	test(r==KErrNone);
	i++;
	r=testFile.Rename(_L("aDummyName"));
	test(r==KErrNone);
	r=testFile.Rename(fileName);
	test(r==KErrNone);

	// RFile::SetSize
	r=testFile.SetSize(1000); // Arbitrary value
	test(r==KErrInUse);

	// Test other RFile, RFs operations
	testFile.Close();
	Test3Operations(aRoot,fileName);

	// Increase number of clamps to MY_N
	r=testFile.Open(TheFs,fileName,EFileRead);
	test(r==KErrNone);
	for(; i < MY_N; i++)
		{
		handlePtr++;
		r=handlePtr->Clamp(testFile);
		test(r==KErrNone);
		}
	testFile.Close();
	Test3Operations(aRoot,fileName);

	// Decrease number of clamps by MY_M
	for(;i > (MY_N - MY_M); i--)
		{
		r=handlePtr->Close(TheFs);
		test(r==KErrNone);
		if(handlePtr!=myHandles)
			handlePtr--;
		else
			break;
		}
	Test3Operations(aRoot,fileName);

	// Increase number of clamps by MY_M
	r=testFile.Open(TheFs,fileName,EFileRead);
	test(r == KErrNone);
	TInt j=0;
	for(;j < MY_M; j++)
		{
		handlePtr++;
		r=handlePtr->Clamp(testFile);
		test(r==KErrNone);
		i++;
		}
	testFile.Close();
	Test3Operations(aRoot,fileName);

	// Decrease number of clamps by MY_N
	for(;i > 0; i--)
		{
		r=handlePtr->Close(TheFs);
		test(r==KErrNone);
		if(handlePtr!=myHandles)
			handlePtr--;
		else
			break;
		}

	// Test deferred dismount - use next free handle
	TestDeferredDismount(aRoot,fileName,handlePtr);

	// Re-create the test directory
	r=TheFs.MkDirAll(aRoot);
	test(r==KErrNone || r== KErrAlreadyExists);
	TheFs.SetSessionPath(aRoot);

	// No clamps remain - prove RFile::Open with EFileWrite
	r=testFile2.Open(TheFs,fileName,EFileWrite|EFileShareReadersOrWriters);
	test(r==KErrNone);
	testFile2.Close();

	// No clamps remain - prove that file can now be deleted
	r=TheFs.Delete(_L("clampFile.tst"));
	test (r==KErrNone);
	}


LOCAL_C void Test4(TDesC& aRoot)
	{
// Clamp tests for non-writable file system
	test.Next(_L("T_Clamp - Test4()"));

	// Tests are limited to clamp, unclamp and denied requests
	// when clamps are present.
	TBuf<256> pathName;	
#ifdef __WINS__
	if((aRoot[0]=='Z')||(aRoot[0]=='z'))
		pathName=_L("clean.txt");
	else
		pathName=_L("root.txt");
#else
	if((aRoot[0]=='Z')||(aRoot[0]=='z'))
		pathName=_L("UnicodeData.txt");
	else
		pathName=_L("\\Test\\clamp.txt");	// For (non-composite) ROFS drive
#endif
	RFile testFile;
	TInt r=testFile.Open(TheFs, pathName, EFileRead);
	test(r==KErrNone);

	// Clamp file
	RFileClamp handle;
	r=handle.Clamp(testFile);
	test(r==KErrNone);
	TInt64 storedCookie_0=handle.iCookie[0];
	TInt64 storedCookie_1=handle.iCookie[1];

	// Try to clamp previously-clamped file
	RFileClamp handle1;
	r=handle1.Clamp(testFile);
	test(r==KErrNone);

	// Unclamp file
	r=handle.Close(TheFs);
	test (r==KErrNone);
	// Check cookie content has been re-initialised
	test((0==handle.iCookie[0])&&(0==handle.iCookie[1]));

	// Try to unclamp a file that is not clamped
	handle.iCookie[0]=storedCookie_0;
	handle.iCookie[1]=storedCookie_1;
	r=handle.Close(TheFs);
	test (r==KErrNotFound);
	// Remove remaining clamp
	r=handle1.Close(TheFs);
	test (r==KErrNone);

	testFile.Close();

	if((aRoot[0]!='Z')&&(aRoot[0]!='z'))	// Can not dismount Z:
		TestDeferredDismount(aRoot,pathName,&handle);
	}


LOCAL_C void Test5()
	{
// Clamp requests on non-clamping file systems
	test.Next(_L("T_Clamp - Test5()"));

	TBuf<256> unsuppFile;	
	unsuppFile = _L("unsuppFile.tst");
	RFile testFile;
	TInt r=testFile.Replace(TheFs,unsuppFile,EFileWrite);
	test(r==KErrNone);

	// Try to clamp a file on a file system that does
	// not support clamping
	RFileClamp handle;
	r=handle.Clamp(testFile);
	test(r==KErrNotSupported);

	// Tidy up
	testFile.Close();
	r=TheFs.Delete(_L("unsuppFile.tst"));
	test (r==KErrNone);
	}	


LOCAL_C void GetDriveLetters()
	{
// Assign the first drive that matches the required criteria
	test.Next(_L("T_Clamp - GetDriveLetters()"));

	TDriveList driveList;
	TDriveInfo driveInfo;
	TInt r=TheFs.DriveList(driveList);
	test(r==KErrNone);
	TInt drvNum;
	TBool drivesFound = EFalse;
	for(drvNum=0; (drvNum<KMaxDrives) && !drivesFound; drvNum++)
		{
		TChar drvLetter='?';
		TFileName fileSystem;
		if(!driveList[drvNum])
			continue;
		test(TheFs.Drive(driveInfo, drvNum) == KErrNone);
		test(TheFs.DriveToChar(drvNum,drvLetter) == KErrNone);
		r=TheFs.FileSystemName(fileSystem,drvNum);
		fileSystem.UpperCase();
		test((r==KErrNone)||(r==KErrNotFound));
		if (!(driveInfo.iDriveAtt & KDriveAttInternal))
			continue;
		// Check for FAT on NAND
		if(NandFatDrv=='?')
			{
			if((driveInfo.iType==EMediaNANDFlash) && (fileSystem.Compare(KFATName)==0))
				NandFatDrv=drvLetter;
			}
		// Check for ROFS
		if(RofsDrv=='?')
			{
			if((driveInfo.iType==EMediaNANDFlash) && (fileSystem.Compare(KROFSName)==0))
				RofsDrv=drvLetter;
			}
		// Check for LFFS
		if(LffsDrv=='?')
			{
			if((driveInfo.iType==EMediaFlash) && (fileSystem.Compare(KLFFSName)==0))
				LffsDrv=drvLetter;
			}
		// Check for CompFSys
		if(CompDrv=='?')
			{
			if((driveInfo.iType==EMediaRom) && ((fileSystem.Compare(KROMName)==0)||(fileSystem.Compare(KCOMPName)==0)))
				CompDrv=drvLetter;
			}
		drivesFound=((NandFatDrv!='?')&&(RofsDrv!='?')&&(LffsDrv!='?')&&(CompDrv!='?'));
		}
	if(NandFatDrv!='?')
		test((NandFatDrv!=RofsDrv)&&(NandFatDrv!=LffsDrv)&&(NandFatDrv!=CompDrv));
	if(RofsDrv!='?')
		test((RofsDrv!=LffsDrv)&&(RofsDrv!=CompDrv));
	if(LffsDrv!='?')
		test(LffsDrv!=CompDrv);

	RDebug::Printf("T_CLAMP: FAT drive=%C, ROFS drive=%C, LFFS drive=%C, ROM-COMP drive=%C \n",(TText)NandFatDrv,(TText)RofsDrv,(TText)LffsDrv,(TText)CompDrv);
	return;
	}


//
// E32Main
//

TInt E32Main()
	{
	TInt r;
	test.Title();
	test.Start(_L("Starting T_CLAMP ..."));
	test(TheFs.Connect()==KErrNone);

	GetDriveLetters();
	TBuf<256> pathName;	

	//************************************************************************
	//
	// Test on FAT (writable file system)
	//
	//************************************************************************
	if(NandFatDrv!='?')
		{
		pathName=_L("?:\\CLAMP-TST\\");	// FAT on NAND
		pathName[0]=(TText)NandFatDrv;
		r=TheFs.MkDirAll(pathName);
		test(r==KErrNone || r== KErrAlreadyExists);
		TheFs.SetSessionPath(pathName);
		test.Printf( _L("T_CLAMP: testing FAT drive on %C\n"),(TText)NandFatDrv);

		Test1();		// Basic clamp operation
		Test2();		// Invalid clamp requests
		Test3(pathName);// Denied FS requests when files are clamped

		r=TheFs.RmDir(pathName);
		test(r==KErrNone);
		}
	else
		test.Printf( _L("T_CLAMP: FAT drive not tested\n"));

	//************************************************************************
	//
	// Test on ROFS (non-writable file system) 
	//
	//************************************************************************
	if(RofsDrv!='?')
		{
		pathName=_L("?:\\");
		pathName[0]=(TText)RofsDrv;
		TheFs.SetSessionPath(pathName);
		test.Printf( _L("T_CLAMP: testing ROFS drive on %C\n"),(TText)RofsDrv);

		Test4(pathName);	// Clamp tests for non-writable file system
		}
	else
		test.Printf( _L("T_CLAMP: ROFS drive not tested\n"));

	//************************************************************************
	//
	// Test on Z: - Composite File System, or ROMFS (non-writable file system)
	//
	//************************************************************************
	if(CompDrv!='?')
		{
		pathName=_L("?:\\TEST\\");
		pathName[0]=(TText)CompDrv;
		TheFs.SetSessionPath(pathName);
		test.Printf( _L("T_CLAMP: testing Z drive (on %C)\n"),(TText)CompDrv);

		Test4(pathName);	// Clamp tests for non-writable file system
		}
	else
		test.Printf( _L("T_CLAMP: Z drive not tested\n"));

	//************************************************************************
	//
	// Test on LFFS (non-clampable file system)
	//
	//************************************************************************
	if(LffsDrv!='?')
		{
		TBuf<256> unsuppPath;	
		unsuppPath=_L("?:\\CLAMP-TST\\");
		unsuppPath[0]=(TText)LffsDrv;
		r=TheFs.MkDirAll(unsuppPath);
		test(r==KErrNone || r== KErrAlreadyExists);
		TheFs.SetSessionPath(unsuppPath);
		test.Printf( _L("T_CLAMP: testing LFFS drive on %C\n"),(TText)LffsDrv);

		Test5();		// Clamp requests on non-clamping file systems
		}
	else
		test.Printf( _L("T_CLAMP: LFFS drive not tested\n"));

	test.End();
	return 0;
	}

#else

TInt E32Main()
	{
	test.Title();
	test.Start(_L("Test does not run on UREL builds."));
	test.End();
	return 0;
	}
#endif
