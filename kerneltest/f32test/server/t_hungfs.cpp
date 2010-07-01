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
// f32test\server\t_hungfs.cpp
// this test will either run with a secure mmc or will simulate a
// mmc card.
// 
//


#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include "t_server.h"

GLDEF_D RTest test(_L("T_HUNGFS"));
GLREF_D RFs TheFs;

LOCAL_D RFile SubFile1;
LOCAL_D TInt HungReturnCodeC;

LOCAL_D RSemaphore HungSemaphoreC;
LOCAL_D TInt HungReturnCodeNC;
LOCAL_D RSemaphore HungSemaphoreNC;

GLREF_C void Format(TInt aDrive);

LOCAL_D const TInt KControlIoLockMount=5;
LOCAL_D const TInt KControlIoClearLockMount=6;
LOCAL_D const TInt KControlIoCNotifier=7;
LOCAL_D const TInt KControlIoClearCNotifier=8;

LOCAL_D const TInt KControlIoCancelNCNotifier=KMaxTInt-1;

LOCAL_D TBool isSecureMmc;
LOCAL_D TFileName gLockedPath;
LOCAL_D TFileName gLockedBase;
LOCAL_D TInt gLockedDrive;
GLREF_D TFileName gSessionPath;
LOCAL_D TInt gSessionDrive;

TBuf8<16> KPassword8=_L8("abc");
TBuf16<8> KPassword=_L("abc");
_LIT(File1,"file1");
_LIT(File2,"file2");
_LIT(Dir1,"\\dir1\\");
_LIT(NotifyDir,"\\not\\");
_LIT(NotifyDir2,"\\abc\\");
_LIT(HungDirNC,"\\hungnc\\");
_LIT(HungDirC,"\\hungc\\");

LOCAL_D const TInt KHeapSize=0x2000;

struct SNonCriticalInfo
	{
	TBool iSameSession;
	TBool iUseRFile;
	};

LOCAL_C TInt ThreadFunctionNC(TAny* aInfo)
//
// Thread entry point to put up non-critical notifier
//
	{
	RFs fs;
	TInt r;

	TBuf8<8> mmcBuf;
	// pad out password if using lockable mmc
	if(isSecureMmc)
		{
		TUint8 pBuf[]={0x61,0x00,0x62,0x00,0x63,00};
		mmcBuf.SetLength(6);
		for(TInt i=0;i<6;++i)
			mmcBuf[i]=pBuf[i];
		}

	SNonCriticalInfo* info=NULL;
	if(aInfo!=NULL)
		info=(SNonCriticalInfo*)aInfo;

	r=fs.Connect();
	
	if(isSecureMmc)
		{
		if(info && info->iSameSession)
			r=TheFs.LockDrive(gLockedDrive,NULL,mmcBuf,EFalse);
		else
			{
			r=fs.LockDrive(gLockedDrive,NULL,mmcBuf,EFalse);
			RDebug::Print(_L("l=%d"),r);
			}
		}
	else
		{
		if(info && info->iSameSession)
			r=TheFs.ControlIo(gLockedDrive,KControlIoLockMount,KPassword8);
		else
			r=fs.ControlIo(gLockedDrive,KControlIoLockMount,KPassword8);
		}
//	UserSvr::ForceRemountMedia(ERemovableMedia0);
	User::After(300000);
	HungSemaphoreNC.Signal();

	TFileName hungDir=gLockedBase;
	hungDir+=HungDirNC;
	if(info && info->iSameSession)
		r=TheFs.RmDir(hungDir);
	else if(info && info->iSameSession && info->iUseRFile)
		{
		TBuf8<16> buf=_L8("abc");
		r=SubFile1.Write(buf);
		}
	else
		{
		r=fs.RmDir(hungDir);
		RDebug::Print(_L("rd=%d"),r);
		}
	HungReturnCodeNC=r;

	if(isSecureMmc)
		{
		r=fs.UnlockDrive(gLockedDrive,mmcBuf,EFalse);
		r=fs.ClearPassword(gLockedDrive,mmcBuf);
		}
	else
		r=fs.ControlIo(gLockedDrive,KControlIoClearLockMount,KPassword8);

	HungSemaphoreNC.Signal();

	fs.Close();

	return(KErrNone);
	}

LOCAL_C TInt ThreadFunctionC(TAny* aDrive)
//
// Thread entry point to put up a critical notifier
//
	{
	RFs fs;
	TInt r=fs.Connect();
	test_KErrNone(r);
	
	TInt drive;
#if defined(__WINS__)
	drive=EDriveY;
#else	
	drive=EDriveC;
#endif
	// can only get critcal notifer up using ControIo
	// assumes fat file system running on one of the drives named below
	r=fs.ControlIo(drive,KControlIoCNotifier);
	
	TBool isRemovable=*(TBool*)aDrive;
	TFileName hungDir=(_L("?:"));
#if defined(__WINS__)
	if(isRemovable)
		hungDir[0]=(TText)('A'+EDriveX);
	else
		hungDir[0]=(TText)('A'+EDriveY);
#else
	if(isRemovable)
		hungDir[0]=(TText)('A'+EDriveD);
	else
		hungDir[0]=(TText)('A'+EDriveC);
#endif
	hungDir+=HungDirC;
	r=fs.RmDir(hungDir);
	r=fs.MkDir(hungDir);
	HungReturnCodeC=r;

	r=fs.ControlIo(drive,KControlIoClearCNotifier);

	HungSemaphoreC.Signal();
	r=fs.RmDir(hungDir);
	fs.Close();
	return(KErrNone);
	}

LOCAL_C void PutNonCriticalNotifier(TAny* aInfo,RThread& aThread)
//
// put up a non-critical notifier on a removable media
//
	{
	TInt r=aThread.Create(_L("ThreadNC"),ThreadFunctionNC,KDefaultStackSize,KMinHeapSize,KMinHeapSize,aInfo);
	test_KErrNone(r);	
	aThread.SetPriority(EPriorityMore);
	aThread.Resume();
	HungSemaphoreNC.Wait();
	User::After(1000000);

	}

LOCAL_C void PutCriticalNotifier(TBool aBool,RThread& aThread)
//
// put up a critical notifier on the specified drive
//
	{
	TInt r=aThread.Create(_L("ThreadC"),ThreadFunctionC,KDefaultStackSize,KMinHeapSize,KMinHeapSize,&aBool);
	test_KErrNone(r);
	aThread.SetPriority(EPriorityMore);
	aThread.Resume();
	User::After(1000000);
	}

void TestNotifiers()
//
// Test non-critical notifier is cancelled when the critical notifier is put up
// Needs to be carried out on platform with password notifier which handles requests
// asynchronously and uses CAtaDisk in fat file system on c: drive ie. not internal ram
// drive
//
	{
	test.Next(_L("TestNotifiers"));
	if(isSecureMmc)
		return;
	RThread threadNC,threadC;
	PutNonCriticalNotifier(NULL,threadNC);
	PutCriticalNotifier(EFalse,threadC);
	// get rid of critical notifier
	test.Printf(_L("Press cancel on for critcal notifier\n"));
	test.Printf(_L("Press any character\n"));
	test.Getch();
	TRequestStatus deathStat;
	threadNC.Logon( deathStat );
	HungSemaphoreC.Wait();
	// test(HungReturnCodeNC==KErrAbort);
	TRequestStatus deathStat2;
	threadC.Logon(deathStat2);
	HungSemaphoreNC.Wait();
	//test(HungReturnCodeNC==KErrLocked);
	User::WaitForRequest(deathStat2);
	threadNC.Close();
	threadC.Close();
	}

void TestCriticalFunctions() 
//
// test that only a subset of functions are supported when the critical notifier is up
//
	{
	test.Next(_L("test functions supported with critical notifier"));
	// used for EFsSubClose
	RFile file;
	TInt r=file.Create(TheFs,File1,EFileShareAny|EFileWrite);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	if(r==KErrAlreadyExists)
		{
		r=file.Open(TheFs,File1,EFileShareAny|EFileWrite);
		test_KErrNone(r);
		}
	TBuf8<16> buf=_L8("abcdefghijklmnop");
	r=file.Write(buf);
	test_KErrNone(r);

	r=TheFs.MkDir(NotifyDir);
	test_KErrNone(r);

	RThread thread;
	PutCriticalNotifier(ETrue,thread);

	// test functions that are supported with critical notifier
	// EFsNotifyChange
	test.Next(_L("test functions that are supported with critical notifier"));
	TRequestStatus status;
	TheFs.NotifyChange(ENotifyAll,status);
	test(status==KRequestPending);
	// EFsNotifyChangeCancel
	TheFs.NotifyChangeCancel();
	test(status==KErrCancel);
	// EFsNotifyChangeEx
	TheFs.NotifyChange(ENotifyAll,status,gSessionPath);
	test(status==KRequestPending);
	// EFsNotifyChangeCancelEx
	TheFs.NotifyChangeCancel(status);
	test(status==KErrCancel);
	// EFsSubClose
	// difficult  to test properly because this does not return an error value
	file.Close();

	// test that notifications are not completed when a critical notifier completes
	test.Next(_L("test notifications not completed"));
	TheFs.NotifyChange(ENotifyDisk,status);
	test(status==KRequestPending);
	TRequestStatus status2;
	TheFs.NotifyChange(ENotifyDir,status2,NotifyDir);
	test(status2==KRequestPending);

	// test some functions that are not supported when critcical notifier up
	// EFsFileOpen
	test.Next(_L("test functions that are not supported with critical notifier"));
	r=file.Open(TheFs,File2,EFileShareAny);
	test_Value(r, r == KErrInUse);
	// EFsFileCreate
	r=file.Create(TheFs,File2,EFileShareAny);
	test_Value(r, r == KErrInUse);
	// EFsMkDir
	r=TheFs.MkDir(Dir1);
	test_Value(r, r == KErrInUse);
	// EFsVolume
	TVolumeInfo info;
	r=TheFs.Volume(info,gSessionDrive);
	test_Value(r, r == KErrInUse);

	// get rid of critical notifier
	test.Printf(_L("Press escape on the critical notifier\n"));
	TRequestStatus deathStat;
	thread.Logon(deathStat);
	HungSemaphoreC.Wait();
	test(HungReturnCodeC==KErrAbort);
	User::WaitForRequest(deathStat);
	thread.Close();

	// test notifiers have not gone off
	test(status==KRequestPending&&status2==KRequestPending);

	// test that notification setup on  default drive has had the drive changed
	// to * since the critical notifier was up
	TFileName notDir=gLockedBase;
	notDir+=NotifyDir;
	r=TheFs.MkDir(notDir);
	test_KErrNone(r);
	test(status==KRequestPending&&status2==KErrNone);
	TheFs.NotifyChangeCancel();
	test(status==KErrCancel);
	r=TheFs.Delete(File1);
	test_KErrNone(r);
	r=TheFs.RmDir(notDir);
	test_KErrNone(r);
	r=TheFs.RmDir(NotifyDir);
	test_KErrNone(r);
	}

void TestParsingFunctions()	
//
// Test functions that use Parse* and Validate* functions with 
// non-critical notifier up
// 
	{
	test.Next(_L("TestParsingFunctions"));
	RThread thread;
	PutNonCriticalNotifier(NULL,thread);
	
	// test on same drive as notifier
	test.Next(_L("test parsing functions on same drive"));
	// Using ParseSubst
	TFileName dir=gLockedBase;
	dir+=Dir1;
	TInt r=TheFs.MkDir(dir);
	test_Value(r, r == KErrInUse);
	TFileName file=gLockedPath;
	file+=File1;
	RFile f;
	r=f.Create(TheFs,file,EFileShareAny);
	test_Value(r, r == KErrInUse);
	// Using ParsePathPtr0
	r=TheFs.SetSubst(gLockedPath,EDriveO);
	test_Value(r, r == KErrInUse);
	// ValidateDrive
	TVolumeInfo info;
	r=TheFs.Volume(info,gLockedDrive);
	test_Value(r, r == KErrInUse);

	TFileName origSessPath;
	r=TheFs.SessionPath(origSessPath);
	test_KErrNone(r);

	// test these work ok
	r=TheFs.SetSessionPath(gLockedPath);
	test_KErrNone(r);
	r=TheFs.SetSessionPath(origSessPath);
	test_KErrNone(r);

	// test on different drive from notifier - the session path
	test.Next(_L("test parsing functions on a different drive"));
	// Using ParseSubst
	r=TheFs.MkDir(Dir1);
	test_KErrNone(r);
	r=TheFs.RmDir(Dir1);
	test_KErrNone(r);
	r=f.Create(TheFs,File1,EFileShareAny);
	test_KErrNone(r);
	f.Close();
	r=TheFs.Delete(File1);
	test_KErrNone(r);
	// Using ParsePathPtr0
	r=TheFs.SetSubst(gSessionPath,EDriveO);
	test_KErrNone(r);
	r=TheFs.SetSubst(_L(""),EDriveO);
	test_KErrNone(r);
	// ValidateDrive
	r=TheFs.Volume(info,gSessionDrive);
	test_KErrNone(r);

	// get rid of non-critical notifier
	test.Printf(_L("Enter %S on the notifier\n"),&KPassword);

	TRequestStatus deathStat;
	thread.Logon( deathStat );
	HungSemaphoreNC.Wait();
	test(HungReturnCodeNC==KErrNotFound);
	User::WaitForRequest( deathStat );
	thread.Close();

//	test.End();
	}
	
void TestTFsFunctions()	
//
// test functions that do not use the Parse* and Validate* functions
//
	{
	test.Next(_L("TestTFsFunctions"));
	TFileName sessName,lockedName;
	TInt r=TheFs.FileSystemName(sessName,gSessionDrive);
	test_KErrNone(r);
	r=TheFs.FileSystemName(lockedName,gLockedDrive);
	test_KErrNone(r);

	RThread thread;
	PutNonCriticalNotifier(NULL,thread);

	// test functions on hung drive - should return KErrInUse
	test.Next(_L("test TFs functions on hung drive"));
	// TFsDismountFileSystem
	r=TheFs.DismountFileSystem(lockedName,gLockedDrive);
	test_Value(r, r == KErrInUse);
	// TFsMountFileSystem
	r=TheFs.MountFileSystem(lockedName,gLockedDrive);
	test_Value(r, r == KErrInUse);
	// TFsFileSystemName
	r=TheFs.FileSystemName(lockedName,gLockedDrive);
	test_Value(r, r == KErrInUse);

	// test functions on drive other than hung drive
	test.Next(_L("test TFs functions on drive that is not hung"));
	// TFsDismountFileSystem
#if defined(__WINS__)
	// bug in TFsMountFileSystem which needs to be fixed before running on EDriveC on WINS
	if(gSessionDrive!=EDriveC)
		{
#endif
	r=TheFs.DismountFileSystem(sessName,gSessionDrive);
	test_KErrNone(r);
	// TFsMountFileSystem
	r=TheFs.MountFileSystem(sessName,gSessionDrive);
	test_KErrNone(r);
#if defined(__WINS__)
		}
#endif
	// TFsFileSystemName
	TFileName fsName;
	r=TheFs.FileSystemName(fsName,gSessionDrive);
	test_KErrNone(r);
	test(fsName==sessName);

	// test functions that fail on all drives
	test.Next(_L("test TFs functions that fail on all drives"));
	// TFsListOpenFiles
	CFileList* list=NULL;
	TOpenFileScan fileScan(TheFs);
	TRAP(r,fileScan.NextL(list));
	test_Value(r, r == KErrInUse);

	// test functions that should pass on any drive
	test.Next(_L("test TFs functions that pass on all drives"));
	// TFsSetDefaultPath
	// TFsSetSessionPath
	r=TheFs.SetSessionPath(gLockedPath);
	test_KErrNone(r);
	r=TheFs.SetSessionPath(gSessionPath);
	test_KErrNone(r);

	// get rid of non-critical notifier
	test.Printf(_L("Enter %S on the notifier\n"),&KPassword);
	TRequestStatus deathStat;
	thread.Logon( deathStat );
	HungSemaphoreNC.Wait();
	test(HungReturnCodeNC==KErrNotFound);
	User::WaitForRequest( deathStat );
	thread.Close();
//	test.End();
	}


void TestSubsessions()	
//
// test subsession functions
//
	{
	test.Next(_L("TestSubsessions"));
	RThread thread;
	PutNonCriticalNotifier(NULL,thread);

	// test that subsessions cannot be opened on a hung drive
	test.Next(_L("test subsessions cannot be opened on a hung drive"));
	// EFsFileCreate
	RFile file;
	TFileName fileName=gLockedPath;
	fileName+=File1;
	TInt r=file.Create(TheFs,fileName,EFileShareAny);
	test_Value(r, r == KErrInUse);
	// EFsFormatOpen
	RFormat format;
	TInt count;
	r=format.Open(TheFs,gLockedPath,EHighDensity,count);
	test_Value(r, r == KErrInUse);
	// EFsDirOpen
	RDir dir;
	r=dir.Open(TheFs,gLockedPath,KEntryAttMaskSupported);
	test_Value(r, r == KErrInUse);
	// EFsRawDiskOpen
	RRawDisk raw;
	r=raw.Open(TheFs,gLockedDrive);
	test_Value(r, r == KErrInUse);

	// get rid of non-critical notifier
	test.Printf(_L("Enter %S on the notifier\n"),&KPassword);
	TRequestStatus deathStat;
	thread.Logon( deathStat );
	HungSemaphoreNC.Wait();
	test(HungReturnCodeNC==KErrNotFound);
	User::WaitForRequest( deathStat );
	thread.Close();

	// now open the subsessions
	r=file.Create(TheFs,fileName,EFileShareAny);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	if(r==KErrAlreadyExists)
		{
		r=file.Open(TheFs,fileName,EFileShareAny);
		test_KErrNone(r);
		}
	r=dir.Open(TheFs,gLockedPath,KEntryAttMaskSupported);
	test_KErrNone(r);
	
	// put notifier back up
	PutNonCriticalNotifier(NULL,thread);

	// test subsession operations fail on a hung drive
	test.Next(_L("test subsession operations fail on a hung drive"));
	// EFsFileRead
	TBuf8<16> readBuf;
	r=file.Read(readBuf);
	test_Value(r, r == KErrInUse);
	// subsession should be able to be closed ok
	file.Close();
	// EFsDelete
	r=TheFs.Delete(fileName);
	test_Value(r, r == KErrInUse);
	// EFsDirRead
	TEntry entry;
	r=dir.Read(entry);
	test_Value(r, r == KErrInUse);
	// subsession should be able to be closed ok
	dir.Close();

	// not going to test operations on a drive the is not hung
	// since this will be tested on other tests

	// get rid of non-critical notifier
	test.Printf(_L("Enter %S on the notifier\n"),&KPassword);
	thread.Logon( deathStat );
	HungSemaphoreNC.Wait();
	test(HungReturnCodeNC==KErrNotFound);
	User::WaitForRequest( deathStat );
	thread.Close();

	r=TheFs.Delete(fileName);
	test_KErrNone(r);
//	test.End();
	}

void TestSameSession()
//
// Test functions that can and cannot be handled from same session when notifier is up
//
	{
	test.Next(_L("TestSameSession"));

	RFile file;
	TInt r=file.Create(TheFs,File1,EFileWrite);
	test_KErrNone(r);

	// put notifier up using TheFs session
	SNonCriticalInfo info={ETrue,0};
	RThread thread;
	PutNonCriticalNotifier(&info,thread);

	// test critical functions can be handled
	test.Next(_L("test critical functions can be handled"));
	// EFsNotifyChange
	TRequestStatus status;
	TheFs.NotifyChange(ENotifyAll,status);
	test(status==KRequestPending);
	// EFsNotifyChangeCancel
	TheFs.NotifyChangeCancel();
	test(status==KErrCancel);
	// EFsNotifyChange again
	TheFs.NotifyChange(ENotifyAll,status);
	test(status==KRequestPending);
	// EFsNotifyChangeCancelEx
	TheFs.NotifyChangeCancel(status);
	test(status==KErrCancel);
	// not going to test EFsSubClose

	// test other functions are not handled
	test.Next(_L("test other functions cannot be handled"));
	// EFsNotifyChangeEx
	TheFs.NotifyChange(ENotifyAll,status,gSessionPath);
	test(status==KErrInUse);
	TFileName defPath;
	// EFsCheckDisk
	r=TheFs.CheckDisk(gSessionPath);
	test_Value(r, r == KErrInUse);
	// EFsFileWrite
	_LIT8(buffer,"abc");
	TBuf8<8> buf(buffer);
	r=file.Write(buf);
	test_Value(r, r == KErrInUse);

	// this file should be able to be closed
	file.Close();

	// get rid of non-critical notifier
	test.Printf(_L("Enter %S on the notifier\n"),&KPassword);
	
	TRequestStatus deathStat;
	thread.Logon( deathStat );
	HungSemaphoreNC.Wait();
	test(HungReturnCodeNC==KErrNotFound);
	User::WaitForRequest( deathStat );
	thread.Close();

	r=TheFs.Delete(File1);
	test_KErrNone(r);
//	test.End();
	}

void TestSameSubSession()
// 
// test hung file server with same subsession
//
	{
	_LIT(file1Name,"\\SubFile1");
	_LIT(file2Name,"\\SubFile2");
	_LIT(file3Name,"\\SubFile3");
	_LIT(file4Name,"\\SubFile4");
	TFileName origSession;
	TInt r=TheFs.SessionPath(origSession);
	test_KErrNone(r);
	TFileName file1Path(gLockedBase);
	file1Path+=file1Name;
	TFileName file2Path(file2Name);
	TFileName file3Path(gLockedBase);
	file3Path+=file3Name;
	TFileName file4Path(file4Name);
	// create file that will be used to hang file server
	r=SubFile1.Create(TheFs,file1Path,EFileWrite);
	test_KErrNone(r);
	// create file with same session but on different drive
	RFile subfile2;
	r=subfile2.Create(TheFs,file2Path,EFileWrite);
	test_KErrNone(r);
	// create file on unhung drive and with different session
	RFs fs2;
	r=fs2.Connect();
	test_KErrNone(r);
	r=fs2.SetSessionPath(origSession);
	test_KErrNone(r);
	RFile subfile3;
	r=subfile3.Create(fs2,file3Path,EFileWrite);
	test_KErrNone(r);
	// create file on unhung drive and with different session
	RFile subfile4;
	r=subfile4.Create(fs2,file4Path,EFileWrite);
	test_KErrNone(r);
	// in a different thread cause the server to hang using one of the File1 subsession
	// put notifier up using TheFs session
	SNonCriticalInfo info={ETrue,ETrue};
	RThread thread;
	PutNonCriticalNotifier(&info,thread);
	test.Next(_L("test writing to files with hung file server"));
	// check only file4 can be written to
	_LIT8(buffer,"abc");
	TBuf8<8> buf(buffer);
	// File1 caused hung file server
	r=SubFile1.Write(buf);
	test_Value(r, r == KErrInUse);
	// file2 is same session as subsession that caused hung file server
	r=subfile2.Write(buf);
	test_Value(r, r == KErrInUse);
	// file3 is opened on hung drive
	r=subfile3.Write(buf);
	test_Value(r, r == KErrInUse);
	// file4 should be ok
	r=subfile4.Write(buf);
	test_KErrNone(r);
	// hard to test EFsSubClose since does not return an error value
	test.Next(_L("test closing down the subsessions"));
	subfile4.Close();
	// calling close on the same subsession as is hung will cause this subsession
	// to be closed once the original request is completed
	SubFile1.Close();
	subfile2.Close();

	// get rid of non-critical notifier
	test.Printf(_L("Enter %S on the notifier\n"),&KPassword);
	TRequestStatus deathStat;
	thread.Logon( deathStat );
	HungSemaphoreNC.Wait();
	test(HungReturnCodeNC==KErrNotFound);
	User::WaitForRequest( deathStat );
	thread.Close();
	// close remaining files
	subfile3.Close();
	// clean up
	fs2.Close();
	r=TheFs.Delete(file1Path);
	test_KErrNone(r);
	r=TheFs.Delete(file2Path);
	test_KErrNone(r);
	r=TheFs.Delete(file3Path);
	test_KErrNone(r);
	r=TheFs.Delete(file4Path);
	test_KErrNone(r);
	}

void TestExtendedNotifier()
//
//
//
	{
	test.Next(_L("TestExtendedNotifier"));
	// setup a notification on a removable media
	test.Next(_L("test with drive specified"));
	TFileName notDir=gLockedBase;
	notDir+=NotifyDir;
	TRequestStatus status;
	TheFs.NotifyChange(ENotifyAll,status,notDir);
	test(status==KRequestPending);
	// now do an operation on c: and test no notification
	TInt r=TheFs.MkDir(NotifyDir);
	test_KErrNone(r);
	r=TheFs.RmDir(NotifyDir);
	test_KErrNone(r);
	User::After(1000000);
	test(status==KRequestPending);
	TheFs.NotifyChangeCancel(status);
	test(status==KErrCancel);

	// now put up the notifier
	RThread thread;
	PutNonCriticalNotifier(NULL,thread);

	// repeat the above notification
	// setup a notification on a removable media - the drive should be changed
	// to * since notifier is up
	test.Next(_L("test with wildcard for drive"));
	TheFs.NotifyChange(ENotifyAll,status,notDir);
	test(status==KRequestPending);
	// test notification does not go off with wrong path
	r=TheFs.MkDir(NotifyDir2);
	test_KErrNone(r);
	r=TheFs.RmDir(NotifyDir2);
	test_KErrNone(r);
	test(status==KRequestPending);
	// now do an operation on c: and test there has been a notification
	r=TheFs.MkDir(NotifyDir);
	test_KErrNone(r);
	r=TheFs.RmDir(NotifyDir);
	test_KErrNone(r);
	User::After(1000000);
	// request should be completed this time
	test(status==KErrNone);

	// set up a notification on drive that is not removable
	// the path should not be changed
	test.Next(_L("test with non-removable drive"));
	TheFs.NotifyChange(ENotifyAll,status,notDir);
	test(status==KRequestPending);
	TRequestStatus status2;
	TFileName origSession;
	r=TheFs.SessionPath(origSession);
	test_KErrNone(r);
	RFs fs2;
	r=fs2.Connect();
	test_KErrNone(r);
	r=fs2.SetSessionPath(origSession);
	test_KErrNone(r);
	_LIT(notifyDirZ,"z:\\test\\");
	_LIT(notifyDirSess,"\\test\\");
	// notifyDirZ already exists, create test dir in session path
	r=fs2.MkDir(notifyDirSess);
	test_KErrNone(r);
	fs2.NotifyChange(ENotifyDir,status2,notifyDirZ);
	test(status2==KRequestPending);
	TRequestStatus status3;
	fs2.NotifyChange(ENotifyDir,status3,notifyDirSess);
	test(status3==KRequestPending);
	// now delete session dir and test no notification
	r=TheFs.RmDir(notifyDirSess);
	test_KErrNone(r);
	test(status2==KRequestPending && status3==KErrNone);

	// get rid of non-critical notifier
	test.Printf(_L("Enter %S on the notifier\n"),&KPassword);
	TRequestStatus deathStat;
	thread.Logon( deathStat );
	HungSemaphoreNC.Wait();
	test(HungReturnCodeNC==KErrNotFound);
	User::WaitForRequest( deathStat );
	thread.Close();
	test(status==KErrNone&&status2==KErrNone);
	fs2.Close();

	test.Next(_L("test extended notification with critical notifier"));
	RThread threadC;
	// put a a critical notifier
	PutCriticalNotifier(ETrue,threadC);
	// setup extended change notification on session path, drive should be changed to *
	TheFs.NotifyChange(ENotifyAll,status,NotifyDir);
	test(status==KRequestPending);
	// get rid of notifier
	test.Printf(_L("Press cancel on for critcal notifier\n"));
	threadC.Logon(deathStat);
	HungSemaphoreC.Wait();
	User::WaitForRequest(deathStat);
	threadC.Close();
	// test that notification has not gone off
	test(status==KRequestPending);
	// create directory on locked drive
	r=TheFs.MkDir(notDir);
	test_KErrNone(r);
	// test notifier goes off
	test(status==KErrNone);
	r=TheFs.RmDir(notDir);
	test_KErrNone(r);
	// get rid of critical notifier
	}	

LOCAL_C TBool TestSessionPath()
//
// 
//
	{
#if defined(__WINS__)
	TInt r=TheFs.CharToDrive(gSessionPath[0],gSessionDrive);
	test_KErrNone(r);
	if(gSessionDrive==EDriveX)
		return(EFalse);
#else
	TInt r=TheFs.CharToDrive(gSessionPath[0],gSessionDrive);
	test_KErrNone(r);
	TDriveList list;
	r=TheFs.DriveList(list);
	test_KErrNone(r);
	if((list[gSessionDrive])&KDriveAttRemovable)
		return(EFalse);
#endif
	return(ETrue);
	}


GLDEF_C void CallTestsL()
//
// Test a hung file server
//
	{
	if(!IsTestTypeStandard())
		return;

	if(!TestSessionPath())
		{
		test.Printf(_L("Not testing on %S\n"),&gSessionPath);
		return;
		}
#if defined(__WINS__)
	gLockedDrive=EDriveX;
	gLockedPath=_L("?:\\");
	gLockedBase=_L("?:");
	gLockedPath[0]=gLockedBase[0]=(TText)('A'+gLockedDrive);
#else
	gLockedDrive=EDriveD;
	gLockedPath=_L("?:\\");
	gLockedBase=_L("?:");
	gLockedPath[0]=gLockedBase[0]=(TText)('A'+gLockedDrive);
#endif

	test.Printf(_L("Is a secure mmc present? Press y for yes\n"));
	TChar c=test.Getch();
	if(c=='y'||c=='Y')
		isSecureMmc=ETrue;
	else
		isSecureMmc=EFalse;

	TInt r=HungSemaphoreC.CreateLocal(0);
	test_KErrNone(r);
	r=HungSemaphoreNC.CreateLocal(0);
	test_KErrNone(r);

	// create sharable session
	TheFs.ShareAuto();

	TestParsingFunctions();
	TestTFsFunctions();
	TestExtendedNotifier();
	TestSubsessions();
//	TestNotifiers();
	TestCriticalFunctions();
	TestSameSession();
	TestSameSubSession();

	HungSemaphoreC.Close();
	HungSemaphoreNC.Close();
}
