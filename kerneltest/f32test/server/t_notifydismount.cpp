// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\manager\t_notifydismount.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32hal.h>
#include <e32math.h>
#include <f32dbg.h>
#include <hal.h>
#include "t_server.h"

LOCAL_D TFullName fsname;
LOCAL_D	RFile file; 
LOCAL_D TRequestStatus stat;
LOCAL_D	TBuf8<0x10> buf; 
LOCAL_D	TFileName fn;
GLDEF_D	RTest test(_L("t_notifydismount"));

#if defined(_DEBUG)
const TInt KControlIoRuggedOn=2;
const TInt KControlIoRuggedOff=3;
const TInt KControlIoIsRugged=4;
#endif

#if defined(_DEBUG)
LOCAL_C void TestFileHandleClosure(TInt aDrvNum)
//
// check that open file handles may be closed following a NotifyDismount in
// forced dismount mode
//	
{
	test.Next( _L("Test File Handle Closure"));

	TInt r = file.Replace(TheFs, fn, EFileWrite); 
	test_KErrNone(r); 
	r = TheFs.FileSystemName(fsname,aDrvNum);
	test_KErrNone(r); 
	buf = _L8("handle test23456");
	r = file.Write(buf); 
	test_KErrNone(r); 
	TheFs.NotifyDismount(aDrvNum, stat, EFsDismountForceDismount); 
	User::WaitForRequest(stat);         
	test(stat.Int() == KErrNone);

	// PDEF137626 Connectivity: Phone reboots automatically when connecting to PC via USB after pl 
	// Check that writing data to a file when the drive is dismounted doesn't upset the file server
	r = file.Write(buf); 
	test_Value(r, r == KErrNotReady || r == KErrDisMounted); 

	// PDEF091956 was a file server fault EFsDriveThreadError when the file 
	// handle was closed
	file.Close(); 
	r = TheFs.MountFileSystem(fsname,aDrvNum);
	test_KErrNone(r); 
}


LOCAL_C void TestRequestCancelling(TInt aDrvNum)
//
// check that Cancelling all drive thread requests allows File server object to be closed down gracefully
// PDEF101895- Device crash in efile.exe when plugging/unplugging USB cable using fast file ... 
//
	{
	test.Next( _L("Test Request Cancelling") );

	TInt r = TheFs.FileSystemName(fsname,aDrvNum);
	test_KErrNone(r); 
	
	//***************************************
	// first test with an open file handle
	//***************************************
	r = file.Replace(TheFs, fn, EFileWrite); 
	test_KErrNone(r); 

	// up the priority of this thread so that we can queue 2 requests onto the drive thread - 
	// i.e. a TFsNotifyDismount and a TFsCloseObject
	RThread				thisThread;
	thisThread.SetPriority(EPriorityRealTime);

	// Post a TFsNotifyDismount do drive thread - this will cancel all requests when it runs
	// including the subsequent TFsCloseObject...
	TheFs.NotifyDismount(aDrvNum, stat, EFsDismountForceDismount); 

	
	// Post a TFsCloseObject do drive thread - this should be cancelled before it is processed
	// by the earlier TFsNotifyDismount
	file.Close();

	User::WaitForRequest(stat);         
	test(stat.Int() == KErrNone);
	
	thisThread.SetPriority(EPriorityNormal);

	r = TheFs.MountFileSystem(fsname,aDrvNum);
	test_KErrNone(r); 


	//***************************************
	// now test with an open directory handle
	//***************************************

	RDir dir;
	TFileName sessionPath;
	r=TheFs.SessionPath(sessionPath);
	test_KErrNone(r);
	TFileName path=_L("?:\\*");
	path[0]=sessionPath[0];
	r=dir.Open(TheFs,path,KEntryAttMaskSupported);
	test_KErrNone(r);

	thisThread.SetPriority(EPriorityRealTime);
	TheFs.NotifyDismount(aDrvNum, stat, EFsDismountForceDismount); 
	dir.Close();

	User::WaitForRequest(stat);         
	test(stat.Int() == KErrNone);
	
	thisThread.SetPriority(EPriorityNormal);

	r = TheFs.MountFileSystem(fsname,aDrvNum);
	test_KErrNone(r); 
	}


LOCAL_C void TestFileSizeFlushing(TInt aDrvNum)
//
// check that new file sizes are flushed during a NotifyDismount in forced
// dismount mode
//	 
	{	
	test.Next( _L("Test File Size Flushing with EFsDismountForceDismount") );

	TInt size = 0;
	TInt r = file.Replace(TheFs, fn, EFileWrite); 
	test_KErrNone(r); 
	r = TheFs.FileSystemName(fsname,aDrvNum);
	test_KErrNone(r); 
	buf = _L8("size test9123456"); 
	r = file.Write(buf); 
	test_KErrNone(r); 
	r = file.Flush(); 
	test_KErrNone(r); 
	r = file.Write(buf); 
	test_KErrNone(r); 
	TheFs.NotifyDismount(aDrvNum, stat, EFsDismountForceDismount); 
	User::WaitForRequest(stat);         
	test(stat.Int() == KErrNone);
	file.Close();
	r = TheFs.MountFileSystem(fsname,aDrvNum);
	test_KErrNone(r); 
	file.Open(TheFs, fn, EFileWrite);
	r = file.Size(size); 
	test_KErrNone(r); 
	// PDEF091956 was, for example, a file size of 16 rather than 32. new file sizes were
	// not flushed for the forced dismount. this was only a problem with rugged fat off.
	test(size == 32);
	file.Close();

	test.Next( _L("Test File Size Flushing with EFsDismountNotifyClients") );
	size = 0;
	r = file.Replace(TheFs, fn, EFileWrite); 
	test_KErrNone(r); 

	r = file.Write(buf); 
	test_KErrNone(r); 

	r = file.Write(buf); 
	test_KErrNone(r); 

	TheFs.NotifyDismount(aDrvNum, stat, EFsDismountNotifyClients); 
	User::WaitForRequest(stat);

	test(stat.Int() == KErrNone);
	file.Close();

	r = TheFs.MountFileSystem(fsname,aDrvNum);
	test_KErrNone(r); 
	file.Open(TheFs, fn, EFileWrite);
	r = file.Size(size); 
	test_KErrNone(r); 
	test(size == 32);
	file.Close();
	}
#endif

LOCAL_C void TestNotifyCancel(TInt aDrvNum)
//
//	
	{
	test.Next( _L("Test Cancelling a notifier"));

	TRequestStatus status;
	TheFs.NotifyDismount( aDrvNum, status, EFsDismountRegisterClient );
	TheFs.NotifyDismountCancel(status);
	User::WaitForRequest( status );
	test(status.Int() == KErrCancel);

	// up the priority of this thread so that we can queue 2 requests onto the drive thread - 
	// to test CNotifyInfo objects are cleaned up correctly even if the drive thread doesn't run
	RThread	thisThread;
	thisThread.SetPriority(EPriorityRealTime);
	TheFs.NotifyDismount( aDrvNum, status, EFsDismountRegisterClient );
	TheFs.NotifyDismountCancel(status);
	User::WaitForRequest( status );
	test(status.Int() == KErrCancel);
	}

GLDEF_C void CallTestsL()
//
// Call tests that may leave
//
	{
	test.Title();
	TInt drvNum, r;

	r=TheFs.CharToDrive(gDriveToTest,drvNum);
	test_KErrNone(r);


	// dismounting with file system extension present doesn't seem to work
	// so skip the test for now.
	TFullName extName;
	r = TheFs.ExtensionName(extName,drvNum, 0);
	if (r == KErrNone)
		{
		test.Printf(_L("File system extension present (%S). Skipping test.\n"), &extName);
		return;
		}

	
	fn.Format(_L("%c:\\notifydismount.tst"), TUint(gDriveToTest));

	test.Start( _L("Test Notify Dismount") );

#if defined(_DEBUG)
	// the EFsDriveThreadError file server fault (PDEF091956) was only detected 
	// in debug mode
 	TestFileHandleClosure(drvNum);
	TestRequestCancelling(drvNum);
#else
	test.Printf(_L("CallTestsL: Skip TestFileHandleClosure - urel mode.\n"));
#endif
	
	TestNotifyCancel(drvNum);

#if defined(_DEBUG)
	// failure to observe flushing of file size (PDEF091956) only observed 
	// in debug mode
	// debug mode required to determine rugged or non rugged FAT and to switch between these 
	// modes 
	if (IsFileSystemFAT(TheFs,drvNum) || IsFileSystemFAT32(TheFs,drvNum))
		{
		// next test requires rugged fat off
		TUint8 isRugged;
		TPtr8 pRugged(&isRugged,1,1);
		r=TheFs.ControlIo(drvNum,KControlIoIsRugged,pRugged);
		test_KErrNone(r);
		if(isRugged)
			{
			r=TheFs.ControlIo(drvNum,KControlIoRuggedOff);
			test_KErrNone(r);
			}

		TestFileSizeFlushing(drvNum);
	
		// if originally rugged set system back to rugged
		if(isRugged)
			{
			r=TheFs.ControlIo(drvNum,KControlIoRuggedOn);
			test_KErrNone(r);
			}	
		}
	else
		{
		test.Printf(_L("CallTestsL: Skip TestFileSizeFlushing - not a FAT filesystem.\n"));
		}
#else 
		test.Printf(_L("CallTestsL: Skip TestFileSizeFlushing - urel mode.\n"));
#endif

	test.End();
 
	}
