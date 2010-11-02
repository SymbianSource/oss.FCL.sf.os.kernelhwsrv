// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_notifier.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32svr.h>
#include <hal.h>
#include "t_server.h"
#include "t_chlffs.h"
#include "t_notify_plugin.h"
#include "f32_test_utils.h"

const TInt KNotificationHeaderSize = (sizeof(TUint16)*2)+(sizeof(TUint));
const TInt KMinNotificationBufferSize = 2*KNotificationHeaderSize + 2*KMaxFileName;


RTest test(_L("T_NOTIFIER"));
const TInt KMaxHeapSize = 0x800000;
TInt globalDriveNum;
TBuf<50> filesystem; //storing original file system name
_LIT(KTestNotifyFileSystemExeName,"t_tfsys_notify.fsy");
_LIT(KNotifyTestFileSystem,"CNotifyTestFileSystem");

void DismountPlugin()
	{
	TheFs.DismountPlugin(KNotifyPluginName);
	TheFs.RemovePlugin(KNotifyPluginFileName);
	}

void RemountOriginalFileSystem()
    {
    //Replace old FS.
    TheFs.DismountFileSystem(KNotifyTestFileSystem,globalDriveNum);
    TheFs.MountFileSystem(filesystem,globalDriveNum);
    TheFs.RemoveFileSystem(KNotifyTestFileSystem);
    }

inline void safe_external_test(RTest& aTest, TInt aError, TInt aLine, TText* aName)
    {
    if(aError!=KErrNone)
        {
        test.Printf(_L(": ERROR : %d received on line %d\n"),aError,aLine);
        RemountOriginalFileSystem();
        aTest.operator()(aError==KErrNone,aLine,(TText*)aName);
        }
    }

inline void safe_test(RTest& aTest, TInt aError, TInt aLine, TText* aName)
	{
	if(aError!=KErrNone)
		{
		test.Printf(_L(": ERROR : %d received on line %d\n"),aError,aLine);
		DismountPlugin();
		aTest.operator()(aError==KErrNone,aLine,(TText*)aName);
		}
	}

// Used by TestMultipleNotificationsL() to show which line the function is called from
inline void safe_test(RTest& aTest, TInt aError, TInt aLine, TInt aLineCall)
	{
	if(aError != KErrNone)
		{
		aTest.Printf(_L(": ERROR : %d received on line %d\n"), aError, aLine);
		aTest.Printf(_L(": ERROR : Function called from line number: %d\n"), aLineCall);
		aTest.operator()(aError == KErrNone, aLine);
		}
	}

// Prints out the filename
#define ExpandMe(X)	 L ## X
#define Expand(X)	 ExpandMe(X)

namespace t_notification
	{
	enum EOperation
		{
		//TFsNotification::ECreate
		EFileReplace,
		EFileCreate,
		EFileCreate_subs,			//Create files in subdir, watch subdirs
		EFileCreate_subs_nowatch,	//Create files in subdir, do not monitor subdirs
		EFileCreate_txt_nowatch,	//Create .txt files in subdir, do not monitor subdirs
		EFileCreate_txt,            //Create .txt files
		EFsMkDir,					//Create directory
		//TFsNotification::EAttribute
		EFileSetAtt,
		EFileSetAtt_subs,			//Set attributes in subdir
		EFileSet,
		EFsSetEntry,
		//TFsNotification::ERename
		EFsReplace,					//Replace file
		EFsRename,					//Rename file
		EFsRename_dir,				//Rename directory
		EFileRename,
		EFileRename_wild,			//Rename file using wildcard name
		//TFsNotification::EDelete
		EFsDelete,					//Delete file
		EFsRmDir,					//Remove directory
		EFsRmDir_nonEmpty,			//Remove non-empty directory, which will return KErrInUse
		EFsRmDir_wild,				//Remove subdirectory using wildcard name
		//TFsNotification::EFileChange
		EFileWrite,
		EFileWrite_samesize,		//Write to file without changing its size
		EFileWrite_async,			//Asynchronous write
		EFileSetSize,
		//TFsNotification::EVolumeName
		ESetVolumeLabel,
		//TFsNotification::EDriveName
		ESetDriveName,
		//TFsNotification::EMediaChange
		EMount,
		EDismount,
		EMountScan,
		EMountDismount,
		EFormat,
		EMediaCardRemoval,
		EMediaCardInsertion,
		ERawDiskWrite,
		//Multiple Filters
		EAllOps1,					//Create/Replace and Delete
		EAllOps2,					//Create/Replace, FileChange(Write) and Delete
		EAllOps3,					//Create/Replace, FileChange(SetSize) and Delete
		EAllOps4,					//Create/Replace, Attribute(SetAtt) and Delete
		EAllOps5,					//Create/Replace and Rename
		EAllOps6,					//VolumeName and DriveName
		ECFileManMove               //Create filex in monitored directory, write 4, move to unmonitored, setsize 8, move back, delete
		};
	}

// Package filename and semaphore for thread
struct SThreadPackage
	{
	TFileName iFileName;
	RSemaphore iBarrier;
	};

struct SThreadPackageDualSemaphore
	{
	TFileName iFileName;
	RSemaphore iBarrier;
	RSemaphore iBarrier2;
	};

struct SThreadPackage2
	{
	TInt iTestCase;
	RSemaphore iBarrier;
	};

// Used by TestMultipleNotificationsL
struct SThreadPackageMultiple
	{
	TFileName iString; 		//Commonly stores the filename
	TFileName iFileName; 	//Commonly stores the path (not inc filename)
	RSemaphore iBarrier;
	t_notification::EOperation iOperation;
	TFsNotification::TFsNotificationType iNotifyType;
	TInt iIterations;    	//# of times to 'do' something
	TInt iMaxNotifications; //# of notifications expected
	TInt iBufferSize;
	TInt iLineCall;			//Line where the function is called from
	};

void PrintLine()
	{
	test.Printf(_L("======================================================================\n"));
	}


// We should receive an EMediaChange notification even though we did not register for it 
void TestMediaCardNotificationWhenNotRegisteredForIt()
	{
	RFs fs;
	TInt r = fs.Connect();
	test_KErrNone(r);
	
	CFsNotify* notify = NULL;
	TRAP(r,notify= CFsNotify::NewL(fs,KMinNotificationBufferSize));
	
	TBuf<40> path;
	path.Append((TChar)gDriveToTest);
	path.Append(_L(":\\F32-TST\\T_NOTIFIER\\"));
	
	TBuf<20> filename;
	filename.Append(_L("media.change1"));
	
	r = notify->AddNotification((TUint)TFsNotification::ECreate,path,filename);
	test_KErrNone(r);
	
	TRequestStatus status;
	r = notify->RequestNotifications(status);
	test_KErrNone(r);
	
	test.Printf(_L("*****************************************************************\n"));
	test.Printf(_L("Waiting 10 seconds.\n"));
	test.Printf(_L("This is a MANUAL test, it requires the removal of the media card.\n"));
	test.Printf(_L("PLEASE REMOVE THE MEDIA CARD. (DriveNumber %d)\n"),globalDriveNum);
	test.Printf(_L("Or press Ctrl + F5 on the emulator.\n"));
	test.Printf(_L("*****************************************************************\n"));
	RTimer timer1;
	r = timer1.CreateLocal();
	test_KErrNone(r);
	TRequestStatus timeout;
	TTimeIntervalMicroSeconds32 time = 10000000;
	timer1.After(timeout,time);
	User::WaitForRequest(timeout,status);
	test(status.Int() != KRequestPending);
	timer1.Cancel();
	timer1.Close();
	
	const TFsNotification* notification = notify->NextNotification();
	test(notification != NULL);
	TFsNotification::TFsNotificationType type = notification->NotificationType();
	test(type == TFsNotification::EMediaChange);
	TBuf<2> drive;
	drive.Append((TChar)gDriveToTest);
	drive.Append(_L(":"));
	TPtrC drivePtr;
	r = notification->Path(drivePtr);
	test_KErrNone(r);
	r = drivePtr.Compare(drive);
	test_Value(r, r == 0);
	
	test.Printf(_L("*****************************************************************\n"));
	test.Printf(_L("Waiting 10 seconds.\n"));
	test.Printf(_L("This is a MANUAL test, it requires the insertion of the media card.\n"));
	test.Printf(_L("PLEASE INSERT THE MEDIA CARD. (DriveNumber %d)\n"),globalDriveNum);
	test.Printf(_L("Or press Ctrl + F5 on the emulator.\n"));
	test.Printf(_L("*****************************************************************\n"));
	
	notification = notify->NextNotification();
	if(notification == NULL)
		{
		notify->RequestNotifications(status);
		RTimer timer2;
		r = timer2.CreateLocal();
		test_KErrNone(r);
		TRequestStatus timeout2;
		timer2.After(timeout2,time);
		User::WaitForRequest(timeout2,status);
		test(status.Int() != KRequestPending);
		notification = notify->NextNotification();
		timer2.Cancel();
		timer2.Close();
		}
	test(notification != NULL);
	type = notification->NotificationType();
	test(type == TFsNotification::EMediaChange);

	delete notify;
	fs.Close();
	}

// Creates two sessions, removes the first one
// and then checks if the second one still works
TInt TestClientRemovalL()
	{
	RFs fs;
	TInt r = fs.Connect();
	test_KErrNone(r);
	
	CFsNotify* notify1 = NULL;
	CFsNotify* notify2 = NULL;
	TRAP(r,notify1= CFsNotify::NewL(fs,KMinNotificationBufferSize);
		   notify2= CFsNotify::NewL(fs,KMinNotificationBufferSize);
		);
	if(r!=KErrNone)
		{
		delete notify1;
		delete notify2;
		test_KErrNone(r);
		}
	
	TBuf<40> path;
	path.Append((TChar)gDriveToTest);
	path.Append(_L(":\\F32-TST\\T_NOTIFIER\\"));
	
	TBuf<15> filename;
	filename.Append(_L("create.file"));
	
	TBuf<40> fullname;
	fullname.Append(path);
	fullname.Append(filename);
	
	r = notify1->AddNotification((TUint)TFsNotification::ECreate,path,filename);
	test_KErrNone(r);
	r = notify2->AddNotification((TUint)TFsNotification::ECreate,path,filename);
	test_KErrNone(r);
	
	delete notify1; //Delete notify1 and ensure we still get notification on notify2
	
	TRequestStatus status;
	r = notify2->RequestNotifications(status);
	test_KErrNone(r);
	
	RFile file;
	file.Replace(fs,fullname,EFileWrite); //Replace produces Create notification
	file.Close();
	
	RTimer tim;
	r = tim.CreateLocal();
	test_KErrNone(r);
	
	TRequestStatus timStatus;
	TTimeIntervalMicroSeconds32 time = 10000000; //10 seconds
	tim.After(timStatus,time);
	
	User::WaitForRequest(status,timStatus);
	test(status!=KRequestPending);
	
	r = fs.Delete(fullname);
	test_KErrNone(r);
	
	delete notify2;
	tim.Close();
	fs.Close();
	return KErrNone;
	}

/*
 * This tests that u can set and receive notifications in the root
 * of a drive.
 * 
 * (something which was apparently not possible on RFs::NotifyChange)
 */
TInt TestRootDriveNotifications()
	{
	test.Next(_L("TestRootDriveNotifications"));
	RFs fs;
	fs.Connect();
	
	CFsNotify* notify = NULL;
	
	TRAPD(r,notify = CFsNotify::NewL(fs,KMinNotificationBufferSize););
	test_KErrNone(r);
	test(notify!=NULL);
	
	TBuf<40> path;
	path.Append((TChar)gDriveToTest);
	path.Append(_L(":\\"));
	
	TBuf<15> filename;
	filename.Append(_L("*"));
	
	r = notify->AddNotification((TUint)TFsNotification::ECreate,path,filename);
	test_KErrNone(r);
	
	TRequestStatus status;
	r = notify->RequestNotifications(status);
	test_KErrNone(r);
	
	RFile file;
	TBuf<40> filePath;
	filePath.Append((TChar)gDriveToTest);
	filePath.Append(_L(":\\file.root"));
	r = file.Replace(fs,filePath,EFileRead);
	test_KErrNone(r);
	file.Close();
	
	TRequestStatus s2;
	RTimer tim;
	test(tim.CreateLocal()==KErrNone);
	TTimeIntervalMicroSeconds32 time = 10000000; //10 seconds
	tim.After(s2,time);
	User::WaitForRequest(status,s2);
	test(status!=KRequestPending);
	
	delete notify;
	notify = NULL;
	tim.Close();
	fs.Close();
	return KErrNone;	
	}

/*
 * Creates and deletes loads of CFsNotify objects and makes sure they're all
 * cleaned up afterwards.
 */
TInt TestNewDeleteCFsNotify(TInt aIterations)
	{
	RPointerArray<CFsNotify> array;
	TInt inArray = 0;
	TInt r = KErrNone;
	for(TInt i = 0; i < aIterations; i++)
		{
		CFsNotify* notify = NULL;
		TRAP(r,notify = CFsNotify::NewL(TheFs,500));
		if(r==KErrNone)
			{
			test(notify!=NULL);
			r = array.Append(notify);
			if(r==KErrNone)
				{
				inArray++;
				}
			else
				{
				delete notify;
				break;
				}
			}
		else
			{
			break;
			}
		}
	
	for(TInt j = inArray-1; j >= 0; j--)
		{
		CFsNotify* notify = (CFsNotify*)array[j];
		array.Remove(j);
		delete notify;
		}
	
	array.Reset();
	array.Close();
	return KErrNone;
	}


/*
 * Creates a file
 * Used in SimpleTest1L(), TestTwoDoersL() TestTwoWatchersL(), TestCancelNotificationL()
 */
TInt SimpleSingleNotificationTFDoer(TAny* aAny)
	{
	SThreadPackageDualSemaphore pkgDoer = *(SThreadPackageDualSemaphore*)aAny;
	RTest simpleTestDoer(_L("SimpleSingleNotificationTFDoer"));
	simpleTestDoer.Start(_L("SimpleSingleNotificationTFDoer"));
	TBuf<40> path;
	path.Append(gDriveToTest);
	path.Append(_L(":\\F32-TST\\T_NOTIFIER\\"));
	path.Append(pkgDoer.iFileName);
	
	//Delete file so we definitely get a create notification
	RFs fs;
	TInt r = fs.Connect();
	safe_test(simpleTestDoer,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
	r = fs.Delete(path);
	if(r==KErrNone || r==KErrPathNotFound || r==KErrNotFound)
		r = KErrNone;
	safe_test(simpleTestDoer,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
	r = fs.MkDirAll(path);
	if(r==KErrNone || r==KErrAlreadyExists)
		r = KErrNone;
	safe_test(simpleTestDoer,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
	
	simpleTestDoer.Printf(_L("SimpleSingleNotificationTFDoer - Create File %S\n"),&path);
	//Create file
	RFile file;
	r = file.Create(fs,path,EFileWrite);	
	safe_test(simpleTestDoer,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
	file.Close();	

	fs.Close();
	simpleTestDoer.End();
	simpleTestDoer.Close();
	return KErrNone;
	}

/*
 * Watches 1 file creation
 * Used in SimpleTest1L()
 */
TInt SimpleSingleNotificationTFWatcher(TAny* aAny)
	{
	CTrapCleanup* cleanup;
	cleanup = CTrapCleanup::New();
	RThread thread;
	TUint64 threadId = thread.Id().Id();
	
	SThreadPackage pkgDoer = *(SThreadPackage*)aAny;
	RSemaphore& simpleBarrierTest = pkgDoer.iBarrier;
	
	RTest simpleTestWatcher(_L("SimpleSingleNotificationTFWatcher"));
	simpleTestWatcher.Start(_L("SimpleSingleNotificationTFWatcher"));
	
	RFs fs;
	fs.Connect();
	
	simpleTestWatcher.Printf(_L("SimpleSingleNotificationTFWatcher(%d) - Create CFsNotify\n"),threadId);
	CFsNotify* notify = NULL;
	TRAPD(r,notify = CFsNotify::NewL(fs,100); );
	safe_test(simpleTestWatcher,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
	TBuf<40> path;
	path.Append(gDriveToTest);
	path.Append(_L(":\\F32-TST\\T_NOTIFIER\\")); //len=22
	
	TBuf<20> filename;
	filename.Append(pkgDoer.iFileName);
	
	TBuf<40> fullname;
	fullname.Append(path);
	fullname.Append(filename);
	
	simpleTestWatcher.Printf(_L("SimpleSingleNotificationTFWatcher - Add Notification for %S\n"),&path);
	r = notify->AddNotification((TUint)TFsNotification::ECreate,path,filename);
	safe_test(simpleTestWatcher,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
	TRequestStatus status;
	simpleTestWatcher.Printf(_L("SimpleSingleNotificationTFWatcher(%d) - Request Notifications\n"),threadId);
	r = notify->RequestNotifications(status);
	safe_test(simpleTestWatcher,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
	
	simpleBarrierTest.Signal();
	
	simpleTestWatcher.Printf(_L("SimpleSingleNotificationTFWatcher(%d) - Wait for status to return\n"),threadId);
	User::WaitForRequest(status);
		
	simpleTestWatcher.Printf(_L("(%d) NextNotification\n"),threadId);
	const TFsNotification* notification = notify->NextNotification();
	//Test notification is not null.
	//We should be getting 1 notification.
	if(notification == NULL)
		safe_test(simpleTestWatcher,KErrNotFound,__LINE__,(TText*)Expand("t_notifier.cpp"));
	
	simpleTestWatcher.Printf(_L("(%d) - Notification Type\n"),threadId);
	TFsNotification::TFsNotificationType notificationType = ((TFsNotification*)notification)->NotificationType();
	if(notificationType != TFsNotification::ECreate)
		safe_test(simpleTestWatcher,KErrGeneral,__LINE__,(TText*)Expand("t_notifier.cpp"));
	simpleTestWatcher.Printf(_L("(%d) - Notification Path\n"),threadId);
	TPtrC _pathC;
	((TFsNotification*)notification)->Path(_pathC);
	simpleTestWatcher.Printf(_L("Notification Path = %S\n"),&_pathC);
	TBuf<40> _path;
	_path.Copy(_pathC);
	if(_path.Match(fullname)!=KErrNone)
		safe_test(simpleTestWatcher,KErrBadName,__LINE__,(TText*)Expand("t_notifier.cpp"));
	
	
	TInt driveNumber = 0;
	TInt gDriveNum = -1;
	notification->DriveNumber(driveNumber);
	RFs::CharToDrive(_pathC[0],gDriveNum);
	if(driveNumber != gDriveNum)
		safe_test(simpleTestWatcher,KErrBadHandle,__LINE__,(TText*)Expand("t_notifier.cpp"));
	
	TUid uid;
	TInt32 realUID = 0x76543210;
	r = notification->UID(uid);
	safe_test(simpleTestWatcher,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
	safe_test(simpleTestWatcher,((realUID == uid.iUid)?KErrNone:KErrNotFound),__LINE__,(TText*)Expand("t_notifier.cpp"));
	
	delete notify;
	fs.Close();
	simpleTestWatcher.End();
	simpleTestWatcher.Close();
	delete cleanup;
	return KErrNone;
	}

/*
 * SimpleTest1L - Runs a simple Create test, gets 1 notification, calls type, path etc and exits
 * Two threads: 1 watcher, 1 doer
 */
TInt SimpleCreateTestL()
	{
	test.Next(_L("SimpleTest"));
	RFs fs;
	fs.Connect();
	_LIT(KFileName,"simple.create");
	SThreadPackage pkgDoer;
	pkgDoer.iFileName = KFileName;
	
	SThreadPackage watcherPkg;
	watcherPkg.iFileName = KFileName;
	User::LeaveIfError(pkgDoer.iBarrier.CreateLocal(0));
	User::LeaveIfError(watcherPkg.iBarrier.CreateLocal(0));
	RThread watcher;
	RThread doer;
	watcher.Create(_L("Simple1WatcherThread"),SimpleSingleNotificationTFWatcher,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&watcherPkg);
	doer.Create(_L("Simple1DoerThread"),SimpleSingleNotificationTFDoer,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&pkgDoer);
	watcher.Resume();
	watcherPkg.iBarrier.Wait(); //Wait till Watcher has requested notification
	doer.Resume();
	
	TRequestStatus status;
	doer.Logon(status);
	User::WaitForRequest(status);
	test.Printf(_L("SimpleCreateTest - Doer Exit Reason = %d\n"),doer.ExitReason());
	safe_test(test,doer.ExitReason(),__LINE__,(TText*)Expand("t_notifier.cpp"));
	
	RDebug::Print(_L("Line %d"),__LINE__);
	

	watcher.Logon(status);
	RTimer timer1;
	TInt r = timer1.CreateLocal();
	safe_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
	TRequestStatus timeout;
	TTimeIntervalMicroSeconds32 time = 10000000;    //10 seconds
	timer1.After(timeout,time);
	User::WaitForRequest(timeout,status);
	test(status.Int() != KRequestPending);
	timer1.Cancel();
	timer1.Close();

	
//	User::WaitForRequest(status);
	test.Printf(_L("SimpleCreateTest - Watcher Exit Reason = %d\n"),watcher.ExitReason());
	safe_test(test,watcher.ExitReason(),__LINE__,(TText*)Expand("t_notifier.cpp"));
	
	CLOSE_AND_WAIT(doer);
	CLOSE_AND_WAIT(watcher);
	
	pkgDoer.iBarrier.Close();
	watcherPkg.iBarrier.Close();
	fs.Close();
	return KErrNone;
	}

// Doer thread for TestMultipleNotificationsL
TInt MultipleNotificationTFDoer(TAny* aAny)
	{
	RDebug::Print(_L("MultipleNotificationTFDoer - Start, Line %d"), __LINE__);
	SThreadPackageMultiple pkgDoer = *(SThreadPackageMultiple*)aAny;
	RTest multipleTestDoer(_L("MultipleNotificationTFDoer"));
	multipleTestDoer.Start(_L("Multi-Doer"));
	multipleTestDoer.Printf(_L("MultipleNotificationTFDoer - Line %d"),__LINE__);
	TBuf<40> basepath;
	basepath.Append(gDriveToTest);
	basepath.Append(_L(":\\F32-TST\\T_NOTIFIER\\"));
	
	RThread thread;
	RDebug::Print(_L("MultipleNotificationTFDoer - Line %d"), __LINE__);
	TUint64 threadID = thread.Id().Id();
	RDebug::Print(_L("MultipleNotificationTFDoer - Line %d"), __LINE__);
	
	//Delete file so we definitely get a create notification
	RFs fs;
	TInt r = fs.Connect();
	safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);

	TEntry entry;
	TBool wildcard = EFalse;
	TBuf<40> _path;
	_path.Append(basepath);
	_path.Append(pkgDoer.iFileName);
	_path.Append(pkgDoer.iString);
	r = fs.Entry(_path, entry);
	if (pkgDoer.iNotifyType != TFsNotification::EMediaChange &&
		pkgDoer.iNotifyType != TFsNotification::EDriveName &&
		pkgDoer.iNotifyType != TFsNotification::EVolumeName &&
		pkgDoer.iOperation != t_notification::EAllOps6)
		{
		if (r == KErrBadName)
			{
			wildcard = ETrue;
			}
		if(r != KErrNone && r != KErrPathNotFound && r != KErrNotFound && r != KErrBadName)
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
		if (r == KErrNone && !entry.IsDir() && pkgDoer.iOperation != t_notification::EFsDelete && !wildcard &&
			pkgDoer.iOperation != t_notification::EFsRmDir && pkgDoer.iOperation != t_notification::EFsRmDir_nonEmpty)
			{
			r = fs.Delete(_path);
			if(r != KErrNone && r != KErrPathNotFound && r != KErrNotFound)
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			}
		r = fs.MkDirAll(basepath);
		if(r != KErrNone && r != KErrAlreadyExists)
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
		}
	
	multipleTestDoer.Printf(_L("MultipleNotificationTFDoer (%d) - Line %d"),threadID,__LINE__);
	
	switch(pkgDoer.iOperation)
		{
		case t_notification::EFileReplace:
			{
			RFile file;
			//Generate Notification
			multipleTestDoer.Printf(_L("File Replace - (%d)\n"),threadID);
			r = file.Replace(fs,_path,EFileWrite);	//RFile::Replace -> TFsNotification::ECreate
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			file.Close();
			
			r = fs.Delete(_path);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			break;
			}
		case t_notification::EFileCreate:
		case t_notification::EFileCreate_txt_nowatch:
		case t_notification::EFileCreate_txt:
		case t_notification::EAllOps1:
			{
			multipleTestDoer.Printf(_L("MultipleNotificationTFDoer (%d) - Line %d"),threadID,__LINE__);
			if(wildcard)
				{
				for (TInt i = 0; i < pkgDoer.iIterations; i++)
					{
					multipleTestDoer.Printf(_L("MultipleNotificationTFDoer (%d) - Line %d"),threadID,__LINE__);
					RFile file;
					TBuf<40> path;
					path.Append(basepath);
					path.AppendNum(i);
					if(pkgDoer.iOperation == t_notification::EFileCreate_txt)
						{
						//Create file with different extension (no notification)
						path.Append(_L(".wrg"));
						fs.Delete(path);
						multipleTestDoer.Printf(_L("File Create (wrong extension) - %S (%d)\n"),&path,threadID);
						r = file.Create(fs, path, EFileWrite);
						safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
						//Change path to contain file with correct extension (notification occurs)
						path.Replace(path.Length()-3,3,_L("txt"));
						}
					else if(pkgDoer.iOperation == t_notification::EFileCreate_txt_nowatch)
					    {
					    path.Append(_L(".txt"));
					    }
					fs.Delete(path);
					
					multipleTestDoer.Printf(_L("File Create - %S (%d)\n"),&path,threadID);
					r = file.Create(fs, path, EFileWrite);
					safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
					file.Close();
					r = fs.Delete(path);
					safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
					}
				}
			else
				{
				multipleTestDoer.Printf(_L("MultipleNotificationTFDoer (%d) - Line %d"),threadID,__LINE__);
				for(TInt i = 0; i < pkgDoer.iIterations; i++)
					{
					multipleTestDoer.Printf(_L("MultipleNotificationTFDoer (%d) - Line %d"),threadID,__LINE__);
					RFile file;
					multipleTestDoer.Printf(_L("File Create - %S (%d)\n"),&_path,threadID);
					r = file.Create(fs,_path,EFileWrite);
					safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
					file.Close();
					r = fs.Delete(_path);
					safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
					}
				}
			break;
			}
		case t_notification::EFileCreate_subs:
		case t_notification::EFileCreate_subs_nowatch:
			{
			if (wildcard)
				{
				for (TInt i = 0; i < pkgDoer.iIterations; i++)
					{
					RFile file;
					TBuf<40> path;
					path.Append(basepath);
					path.Append(_L("SubDir\\"));
					r = fs.MkDirAll(path);
					if(r != KErrNone && r != KErrAlreadyExists)
						safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
					path.AppendNum(i);
					fs.Delete(path);
					multipleTestDoer.Printf(_L("File Create - %S (%d)\n"),&path,threadID);
					r = file.Create(fs, path, EFileWrite);
					safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
					file.Close();
					r = fs.Delete(path);
					safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
					}
				}
			break;
			}
		case t_notification::EFileWrite:
		case t_notification::EAllOps2:
			{
			//Works on single file
			RFile file;
			TBuf<40> path;
			path.Append(basepath);
			path.Append(pkgDoer.iFileName);
			path.Append(pkgDoer.iString);
			r = file.Replace(fs,path,EFileWrite);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			for(TInt i = 0; i < pkgDoer.iIterations; i++)
				{
				multipleTestDoer.Printf(_L("File Write - %S (%d)\n"),&path,threadID);
				r = file.Write(4*i,_L8("abcd"));
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
                //If cache is enabled, a notification is received only when the cache is flushed
                //We flush the file to make this a general test
				multipleTestDoer.Printf(_L("File Flush - (%d)\n"),threadID);
                r = file.Flush();
                safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				}
			file.Close();
			r = fs.Delete(path);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			break;
			}
		case t_notification::EFileWrite_samesize:
			{
			RFile file;
			TBuf<40> path;
			path.Append(basepath);
			path.Append(pkgDoer.iFileName);
			path.Append(pkgDoer.iString);
			r = file.Replace(fs,path,EFileWrite);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			multipleTestDoer.Printf(_L("File Write - %S (%d)\n"),&path,threadID);
			r = file.Write(0,_L8("abcd"));
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			
            //If cache is enabled, a notification is received only when the cache is flushed
            //We flush the file to make this a general test
            multipleTestDoer.Printf(_L("File Flush - (%d)\n"),threadID);
            r = file.Flush();
            safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
            
			for(TInt i = 0; i < pkgDoer.iIterations; i++)
				{
				TBuf<2> toWrite;
				toWrite.AppendNum(i);
				multipleTestDoer.Printf(_L("File Write - %S (%d)\n"),&path,threadID);
				r = file.Write(0,(TDesC8&)toWrite);
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				multipleTestDoer.Printf(_L("File Flush - (%d)\n"),threadID);
                r = file.Flush();
                safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				}
			file.Close();
			r = fs.Delete(path);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			break;
			}
		case t_notification::EFileWrite_async:
			{
			RFile file;
			TBuf<40> path;
			path.Append(basepath);
			path.Append(pkgDoer.iFileName);
			path.Append(pkgDoer.iString);
			r = file.Replace(fs,path,EFileWrite);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			for(TInt i = 0; i < pkgDoer.iIterations; i++)
				{
				TRequestStatus status;
				file.Write(52*i, _L8("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"), status);			
				User::WaitForRequest(status);
				multipleTestDoer.Printf(_L("File Write Async - %S (%d)\n"),&path,threadID);
				TInt fileSize;
				file.Size(fileSize);
				multipleTestDoer.Printf(_L("File Write Async - FileSize: %d\n"),fileSize);
				multipleTestDoer.Printf(_L("File Flush - (%d)\n"),threadID);
                r = file.Flush();
                safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				}
			file.Close();
			r = fs.Delete(path);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			break;
			}
		case t_notification::EFileSetSize:
		case t_notification::EAllOps3:
			{
			//Works on single file
			RFile file;
			TBuf<40> path;
			path.Append(basepath);
			path.Append(pkgDoer.iFileName);
			path.Append(pkgDoer.iString);
			r = file.Replace(fs,path,EFileWrite);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			
			//Increase file size
			for(TInt i = 0; i < pkgDoer.iIterations; i++)
				{
				r = file.SetSize(4*(i+1));
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				multipleTestDoer.Printf(_L("File Set Size - %d (%d)\n"),4*(i+1),threadID);
				}
			
			//Decrease file size
			for(TInt j = pkgDoer.iIterations - 2; j >= 0; j--)
				{
				r = file.SetSize(4*(j+1));
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				multipleTestDoer.Printf(_L("File Set Size - %d (%d)\n"),4*(j+1),threadID);
				}	
			
			file.Close();	
			r = fs.Delete(path);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			break;
			}
		case t_notification::EFsReplace:
			{
			TBuf<45> path;
			path.Append(basepath);
			path.Append(pkgDoer.iFileName);
			path.Append(pkgDoer.iString);
			path.AppendNum(0);
			
			RFile tempFile;
			multipleTestDoer.Printf(_L("RFs Replace (Create temp file) - (%d)\n"),threadID);
			r = tempFile.Replace(fs,_path,EFileWrite);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			tempFile.Close();
			
			multipleTestDoer.Printf(_L("RFs Replace - (%d)\n"),threadID);
			r = fs.Replace(_path,path); //RFs::Replace -> TFsNotification::ERename
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			
			r = fs.Delete(path);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			break;
			}
		case t_notification::EFsRename:
			{
			TBuf<45> path;
			path.Append(basepath);
			path.Append(pkgDoer.iFileName);
			path.Append(pkgDoer.iString);
			path.AppendNum(0);
	
			RFile file;
			r = file.Replace(fs,_path,EFileWrite);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			file.Close();

			multipleTestDoer.Printf(_L("RFs Rename - (%d)\n"),threadID);
			r = fs.Rename(_path,path);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
						
			r = fs.Delete(path);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			break;
			}
		case t_notification::EFileRename:
		case t_notification::EFileRename_wild:
		case t_notification::EAllOps5:
			{
			TBuf<45> path;
			path.Append(basepath);
			if (!wildcard)
				{
				path.Append(pkgDoer.iFileName);
				path.Append(pkgDoer.iString);
				}
			path.AppendNum(0);
			
			//Delete new path to ensure it does not exist
			r = fs.Delete(path);
			if(r != KErrNone && r != KErrNotFound && r != KErrPathNotFound)
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			
			if (wildcard)
				{
				_path.Delete(_path.Length()-1,1);
				_path.AppendNum(9);
				}
			
			RFile file;
			r = file.Replace(fs, _path, EFileWrite);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);

			if (wildcard)
				{
				for(TInt i = 1; i <= pkgDoer.iIterations; i++)
					{
					path.Delete(path.Length()-1,1);
					path.AppendNum(i);
					multipleTestDoer.Printf(_L("File Rename - %S (%d)\n"),&path,threadID);
					r = file.Rename(path);
					safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
					}
				}
			else
				{
				multipleTestDoer.Printf(_L("File Rename - (%d)\n"),threadID);
				r = file.Rename(path);
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				}
			
			file.Close();
			r = fs.Delete(path);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			break;
			}
		case t_notification::EFsRename_dir:
			{
			r = fs.MkDirAll(_path);
			if(r != KErrNone && r != KErrAlreadyExists)
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			
			TBuf<45> newPath;
			newPath.Copy(_path);
			newPath.Delete(newPath.Length()-1,1);
			newPath.AppendNum(0);
			newPath.Append(KPathDelimiter);
			
			//Delete new path to ensure it does not exist
			r = fs.RmDir(newPath);
			if(r != KErrNone && r != KErrNotFound && r != KErrPathNotFound)
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);

			multipleTestDoer.Printf(_L("RFs Rename Dir - (%d)\n"),threadID);
			r = fs.Rename(_path,newPath);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			break;
			}
		case t_notification::EFsDelete:
			{
			if (wildcard)
				{
				for(TInt i = 0; i < pkgDoer.iIterations; i++)
					{
					//Create/replace file
					RFile file;
					TBuf<40> path;
					path.Append(basepath);
					r = fs.MkDirAll(path);
					if(r != KErrNone && r != KErrAlreadyExists)
						safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
					path.AppendNum(i);
					path.Append(_L(".txt"));
					r = file.Replace(fs,path,EFileWrite);
					safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
					file.Close();
					
					//Delete file
					multipleTestDoer.Printf(_L("RFs Delete - %S (%d)\n"),&path,threadID);
					r = fs.Delete(path);
					safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
					
					//Create file with a different extension which should not produce notifications
					path.AppendNum(i);
					r = file.Replace(fs,path,EFileWrite);
					safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
					file.Close();
					
					//Delete that file
					multipleTestDoer.Printf(_L("RFs Delete - %S (%d)\n"),&path,threadID);
					r = fs.Delete(path);
					safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
					}
				}
			else
				{
				RFile file;
				r = file.Replace(fs,_path,EFileWrite); //Make sure file exists
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				file.Close();

				multipleTestDoer.Printf(_L("RFs Delete - (%d)\n"),threadID);
				r = fs.Delete(_path);
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				}
			break;
			}
		case t_notification::EFileSet:
			{
			RFile file;
			r = file.Replace(fs,_path,EFileWrite);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			
			multipleTestDoer.Printf(_L("File Set - (%d)\n"),threadID);
			r = file.Set(TTime(0),KEntryAttHidden,KEntryAttReadOnly);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			
			file.Close();
			break;
			}
		case t_notification::EFileSetAtt:
		case t_notification::EAllOps4:
			{
			RFile file;
			r = file.Replace(fs,_path,EFileWrite);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			
			multipleTestDoer.Printf(_L("File SetAtt - (%d)\n"),threadID);
			r = file.SetAtt(KEntryAttHidden,KEntryAttReadOnly);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			
			file.Close();
			r = fs.Delete(_path);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			break;
			}
		case t_notification::EFileSetAtt_subs:
			{
			if (wildcard)
				{
				TBuf<40> path;
				path.Append(basepath);
				path.Append(_L("SubDir\\"));
				r = fs.MkDirAll(path);
				if(r != KErrNone && r != KErrAlreadyExists)
					safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				
				//Create/replace files and set their attributes
				for(TInt i = 0; i < pkgDoer.iIterations; i++)
					{
					RFile file;
					TBuf<40> dirPath;
					dirPath.Append(path);
					dirPath.AppendNum(i);
					dirPath.Append(_L(".ext"));
					r = file.Replace(fs, dirPath, EFileWrite);
					safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
					
					multipleTestDoer.Printf(_L("File SetAtt Subs - %d.ext - (%d)\n"),i,threadID);
					r = file.SetAtt(KEntryAttHidden,KEntryAttReadOnly);
					safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
					
					file.Close();
					r = fs.Delete(dirPath);
					safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
					}							
				}
			break;
			}
		case t_notification::EFsSetEntry:
			{
			RFile file;
			r = file.Replace(fs,_path,EFileWrite);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			file.Close();

			multipleTestDoer.Printf(_L("RFs SetEntry - (%d)\n"),threadID);
			r = fs.SetEntry(_path,TTime(0),KEntryAttHidden,KEntryAttReadOnly);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			break;
			}
		case t_notification::EDismount:
			{
			multipleTestDoer.Printf(_L("DismountFileSystem - (%d)\n"),threadID);
			r = fs.DismountFileSystem(pkgDoer.iFileName,globalDriveNum);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			break;
			}
		case t_notification::EMount:
			{
			multipleTestDoer.Printf(_L("MountFileSystem - (%d)\n"),threadID);
			r = fs.MountFileSystem(pkgDoer.iFileName,globalDriveNum);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			break;
			}
		case t_notification::EMountScan:
			{
			multipleTestDoer.Printf(_L("DismountFileSystem - (%d)\n"),threadID);
			r = fs.DismountFileSystem(pkgDoer.iFileName,globalDriveNum);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			TBool isMount;
			multipleTestDoer.Printf(_L("MountFileSystemAndScan - (%d)\n"),threadID);
			r = fs.MountFileSystemAndScan(pkgDoer.iFileName,globalDriveNum,isMount);
			if(!isMount)
				safe_test(multipleTestDoer,KErrGeneral,__LINE__,pkgDoer.iLineCall);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			break;
			}
		case t_notification::EMountDismount:
			{
			for(TInt i = 0; i < pkgDoer.iIterations; i++)
				{
				multipleTestDoer.Printf(_L("DismountFileSystem - (%d)\n"),threadID);
				r = fs.DismountFileSystem(pkgDoer.iFileName,globalDriveNum);
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				
				multipleTestDoer.Printf(_L("MountFileSystem - (%d)\n"),threadID);
				r = fs.MountFileSystem(pkgDoer.iFileName,globalDriveNum);
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				}
			break;
			}
		case t_notification::EFormat:
			{
			RFormat format;
			TInt count = -1;
			TBuf<2> driveDes;
			driveDes.Append(pkgDoer.iFileName);
			driveDes.Append((TChar)':');
			format.Open(fs,driveDes,EQuickFormat,count);
			multipleTestDoer.Printf(_L("Format - (%d)\n"),threadID);
			while(count != 0)
				{
				format.Next(count);
				}
			format.Close();			
			
			break;
			}
		case t_notification::EMediaCardRemoval:
		case t_notification::EMediaCardInsertion:
			{
			//These are MANUAL tests, they require the removal/insertion of the media card
			//Instructions are given out in the watcher thread
			break;
			}
		case t_notification::ESetDriveName:
			{
			for(TInt i = 0; i < pkgDoer.iIterations; i++)
				{
				multipleTestDoer.Printf(_L("SetDriveName - MyDrive\n"));
				_LIT(KDriveName,"MyDrive");
				r = fs.SetDriveName(globalDriveNum,KDriveName);
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				
				multipleTestDoer.Printf(_L("SetDriveName - MyDrive2\n"));
				_LIT(KDriveName2,"MyDrive2");
				r = fs.SetDriveName(globalDriveNum,KDriveName2);
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				}
			break;
			}
		case t_notification::ERawDiskWrite:
			{
			RRawDisk rawdisk;
			TInt drive = 0;
			TBuf<1> driveDes;
			driveDes.Append(pkgDoer.iFileName);
			RFs::CharToDrive(pkgDoer.iFileName[0],drive);
			r = rawdisk.Open(fs,drive);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			
			//Read some data
			TBuf8<10> readData;
			r = rawdisk.Read(0,readData);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			
			//Write it back
			TPtrC8 dataPtr(readData);
			r = rawdisk.Write((TInt64)0,dataPtr);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			rawdisk.Close();
			break;
			}
		case t_notification::ESetVolumeLabel:
			{
			for(TInt i = 0; i < pkgDoer.iIterations; i++)
				{
				multipleTestDoer.Printf(_L("SetVolumeLabel - MyVolume\n"));
				_LIT(KVolumeLabel,"MyVolume");
				r = fs.SetVolumeLabel(KVolumeLabel,globalDriveNum);
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				
				multipleTestDoer.Printf(_L("SetVolumeLabel - MyVolume2\n"));
				_LIT(KVolumeLabel2,"MyVolume2");
				r = fs.SetVolumeLabel(KVolumeLabel2,globalDriveNum);
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				}
			break;
			}
		case t_notification::EFsRmDir:
			{
			r = fs.MkDirAll(_path); //Make sure directory exists
			if(r != KErrNone && r != KErrAlreadyExists)
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			multipleTestDoer.Printf(_L("RFs RmDir - Remove directory\n"));
			r = fs.RmDir(_path);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			break;
			}
		case t_notification::EFsRmDir_wild:
			{
			if (wildcard)
				{
				TBuf<40> path;
				path.Append(basepath);
				path.Append(_L("SubDir\\"));
				r = fs.MkDirAll(path);
				if(r != KErrNone && r != KErrAlreadyExists)
					safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				
				multipleTestDoer.Printf(_L("RFs RmDir - Remove directory\n"));
				r = fs.RmDir(path);
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				break;
				}
			}
		case t_notification::EFsRmDir_nonEmpty:
		case t_notification::EFsMkDir:
			{
			TBuf<50> path;
			path.Append(_path);
			path.Append(pkgDoer.iFileName); //Append another sub-directory
			multipleTestDoer.Printf(_L("RFs RmDir \n"));
			r=fs.RmDir(path);
			if(r != KErrNone && r != KErrPathNotFound && r != KErrNotFound)
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			r=fs.RmDir(_path);
			if(r != KErrNone && r != KErrPathNotFound && r != KErrNotFound)
				safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			
			multipleTestDoer.Printf(_L("RFs MkDir \n"));
			r=fs.MkDir(path);
			multipleTestDoer (r==KErrPathNotFound);
			r=fs.MkDir(_path);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			r=fs.MkDir(path);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
				
			multipleTestDoer.Printf(_L("RFs RmDir nonEmpty - Full path: %S"),&_path);
			r = fs.RmDir(_path);
			if(r != KErrInUse)
				safe_test(multipleTestDoer,KErrGeneral,__LINE__,pkgDoer.iLineCall);

			r = fs.RmDir(path);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
		
			multipleTestDoer.Printf(_L("RFs EFsRmDir_nonEmpty - Full path: %S"),&_path);
			r = fs.RmDir(_path);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);

			break;
			}
		case t_notification::EAllOps6:
			{
			//Set drive name to TestDrive
			multipleTestDoer.Printf(_L("SetDriveName - TestDrive\n"));
			_LIT(KDriveName,"TestDrive");
			r = fs.SetDriveName(globalDriveNum,KDriveName);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			//Set volume label to TestVolume
			multipleTestDoer.Printf(_L("SetVolumeLabel - TestVolume\n"));
			_LIT(KVolumeLabel,"TestVolume");
			r = fs.SetVolumeLabel(KVolumeLabel,globalDriveNum);
			safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
			break;
			}
		case t_notification::ECFileManMove:
		    {
		    CTrapCleanup* cleanup;
		    cleanup = CTrapCleanup::New();
		      
		    multipleTestDoer.Printf(_L("Doer - ECFileManMove_part1\n"));
		    
		    //Stage 1 - Create File & write some data
		    RFile file;
		    r = file.Replace(fs,_path,EFileWrite);
		    safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
		    r = file.SetSize(4);
		    safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
		    file.Close();
		    
		    CFileMan* cfman = NULL;
		    TRAP(r, cfman = CFileMan::NewL(fs);)
		    test_KErrNone(r);
            test(cfman != NULL);
            TBuf<40> unmonitored_path;
            unmonitored_path.Append(gDriveToTest);
            unmonitored_path.Append(_L(":\\F32-TST\\"));
            unmonitored_path.Append(pkgDoer.iString);

            //Stage 2 - Move to unmonitored Dir
            //Rename 1 
            r = cfman->Move(_path,unmonitored_path);
            safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
            
            //SetSize (size := 8)
            RFile file2;
            r = file2.Open(fs,unmonitored_path,EFileWrite);
            safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
            r = file2.SetSize(8);
            safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
            file2.Flush();
            file2.Close();            
            
            //Stage 3 - Move back to monitored Dir
            //Rename 2
            r = cfman->Move(unmonitored_path,_path);
            safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
            
            //Stage 4 - Delete
            //Delete
            r = cfman->Delete(_path);
            safe_test(multipleTestDoer,r,__LINE__,pkgDoer.iLineCall);
            
            delete cleanup;
            delete cfman;
            break;
		    }
		default:
			{
			break;
			}
		}
	
	fs.Close();
	multipleTestDoer.End();
	multipleTestDoer.Close();
	return KErrNone;	
	}

/*
 * Test process Capabilites tries to set up filters on the
 * the private folder for uid 01234567.
 * This function returns the results to trying to add filters on the specified path.
 * 
 * There are three MMP files:
 * t_notifier_nocaps.mmp, t_notifier_allfiles.mmp, t_notifier_belongs.mmp
 * 
 * t_notifier_nocaps.mmp - 
 * 		This process does not have any capabilites, so should not be allowed to set the filter.
 * t_notifier_allfiles.mmp -
 * 		 This process has ALLFILES capability, so should
 * t_notifier_belongs.mmp - 
 * 		This process is process with UID 01234567 so this should work too.
 * 
 * See: f32test\server\t_notifier_caps.cpp
 */
TInt TestProcessCapabilities(const TDesC& aProcessName)
	{
	RProcess process;
	TUidType uid;
	TPtrC command((TText*)&gDriveToTest,1);
	TInt r = process.Create(aProcessName,command,uid);
	test_KErrNone(r);
	process.Resume();
	TRequestStatus s1;
	TRequestStatus s2;
	RTimer tim;
	r = tim.CreateLocal();
	test_KErrNone(r);
	TTimeIntervalMicroSeconds32 delay = 5000000; //5 seconds
	tim.After(s1,delay);
	process.Logon(s2);
	User::WaitForRequest(s1,s2);
	test(s2.Int()!=KRequestPending);
	r = process.ExitReason();
	process.Close();
	return r;
	}

/*
 * Creates a file and writes to it twice.
 * Used in TestTwoNotificationsL().
 */
TInt TwoNotificationsTFDoer(TAny* aAny)
	{
	RTest testDoer(_L("TestTwoNotificationsThreadFunctionDoer"));
	testDoer.Start(_L("TestTwoNotificationsThreadFunctionDoer"));
	
	SThreadPackageMultiple package = *(SThreadPackageMultiple*)aAny;
	TBuf<40> path;
	path.Append(gDriveToTest);
	path.Append(_L(":\\F32-TST\\T_NOTIFIER\\")); //len=22
	path.Append(package.iFileName);
	
	//Delete file so we definitely get a create notification
	RFs fs;
	TInt r = fs.Connect();
	testDoer(r==KErrNone);
	r = fs.Delete(path);
	testDoer(r==KErrNone || r==KErrPathNotFound || r==KErrNotFound);
	r = fs.MkDirAll(path);
	testDoer(r==KErrNone || r==KErrAlreadyExists);
		
	testDoer.Printf(_L("TestTwoNotificationsThreadFunctionDoer - Create File %S\n"),&path);
	//Create file
	RFile file;
	r = file.Create(fs,path,EFileWrite);
	testDoer(r == KErrNone);
	
	testDoer.Printf(_L("TestTwoNotificationsThreadFunctionDoer - Size File 1 %S\n"),&path);
	TInt fileSize = 0;
	r = file.Size(fileSize);
	testDoer(r == KErrNone);
	testDoer(fileSize==0);
		
	testDoer.Printf(_L("TestTwoNotificationsThreadFunctionDoer - Write File 1 %S\n"),&path);
	r = file.Write(_L8("1234"));
	testDoer(r == KErrNone);
	testDoer.Printf(_L("TestTwoNotificationsThreadFunctionDoer - Size File 2 %S\n"),&path);
	r = file.Size(fileSize);
	testDoer(r == KErrNone);
	test(fileSize==4);

	testDoer.Printf(_L("TestTwoNotificationsThreadFunctionDoer - Write File 2 %S\n"),&path);
	r = file.Write(_L8("5678"));
	testDoer(r == KErrNone);
	testDoer.Printf(_L("TestTwoNotificationsThreadFunctionDoer - Size File 3 %S\n"),&path);
	r = file.Size(fileSize);
	testDoer(r == KErrNone);
	test(fileSize==8);
	
	file.Close();	

	fs.Close();
	testDoer.End();
	testDoer.Close();
	return KErrNone;	
	}

	/*
	 * The following enum are for the CFileMan test
	 */
	enum TCFsManEnum
        {
        ECFManCreate1       = 0x01,
        ECFManWrite1        = 0x02,
        ECFManDelete1       = 0x04,
        ECFManCreate2       = 0x08,
        ECFManSetSize1      = 0x10,
        ECFManWrite2        = 0x20,
        ECFManAtt1          = 0x40,
        ECFManDelete2       = 0x80,
        //Either a create, copy, delete (above - not supported in test) 
        //or rename operation (below) 
        ECFManRename1      = 0x100,
        ECFManRename2      = 0x200
        };

/*
 * Used by MultipleNotificationsTFWatcher() to test the notifications.
 */
void HandleMultipleNotifications(RTest& aTest, SThreadPackageMultiple& aPackage, CFsNotify* notify, TDesC& aFullname)
	{
	RThread thread;
	TUint64 threadID = thread.Id().Id();
	TInt numNotifications = 0;
	TInt64 fileSize = 0;
	TBool overflow = EFalse;
	
	TInt scratch = 0;
	
	for(TInt i = 0; i < aPackage.iMaxNotifications; ) //Outer-loop to control when we should exit
		{
		aTest.Printf(_L("(%d) - NextNotification\n"),threadID);
		const TFsNotification* notification = notify->NextNotification();
		while(notification != NULL)
				{
				numNotifications++;
				aTest.Printf(_L("NumNotifications = %d\n"),numNotifications);
				//Test notification is not null.
				//We should be getting 1 notification.
				aTest(notification != NULL);

				aTest.Printf(_L("(%d) - Notification Type\n"),threadID);
				TFsNotification::TFsNotificationType notificationType = notification->NotificationType();
				aTest(notificationType & aPackage.iNotifyType ||
					  notificationType &	TFsNotification::EOverflow);
				
				aTest.Printf(_L("Notification Type = %u - (%d)\n"),notificationType,threadID);
				
				if(notificationType != TFsNotification::EOverflow)
					{
					aTest.Printf(_L("(%d) - Notification Path\n"),threadID);
					TPtrC _pathC;
					((TFsNotification*) notification)->Path(_pathC);
					aTest.Printf(_L("%S - (%d)\n"),&_pathC,threadID);
					
					if(aPackage.iOperation == t_notification::ECFileManMove 
					        && (scratch == (ECFManWrite1 | ECFManCreate1)))
					    {
					    TChar drive = gDriveToTest;
					    TBuf<40> unmodified_path;
					    unmodified_path.Append(drive);
					    unmodified_path.Append(_L(":\\F32-TST\\"));
					    unmodified_path.Append(_L("cf1le.man"));
					    ((TFsNotification*) notification)->NewName(_pathC);
					    TInt matches = _pathC.Match(unmodified_path);
					    safe_test(aTest,matches,__LINE__,aPackage.iLineCall); 
					    }
					else if((scratch == (ECFManWrite1 | ECFManCreate1 | ECFManRename1)))
					    {
					    ((TFsNotification*) notification)->NewName(_pathC);
					    safe_test(aTest,_pathC.Match(aFullname),__LINE__,aPackage.iLineCall); 
					    }
					else
					    {
					    safe_test(aTest,_pathC.Match(aFullname),__LINE__,aPackage.iLineCall);  
					    }
					}
				else
					{
					aTest.Printf(_L("(%d) - OVERFLOW\n"),threadID);
					//Overflow
					overflow = ETrue;					
					return;
					}
				
				//notificationType will only be of 1 type
				if(notificationType == TFsNotification::EFileChange)
				    {
				    if(!(((aPackage.iNotifyType & TFsNotification::EFileChange) == TFsNotification::EFileChange) &&
				           (aPackage.iOperation == t_notification::EFileWrite ||
				            aPackage.iOperation == t_notification::EFileWrite_async ||
				            aPackage.iOperation == t_notification::EFileWrite_samesize ||
				            aPackage.iOperation == t_notification::EFileSetSize ||
				            aPackage.iOperation == t_notification::ECFileManMove ||
				            aPackage.iOperation == t_notification::EAllOps2 ||
				            aPackage.iOperation == t_notification::EAllOps3)))
				    	safe_test(aTest,KErrGeneral,__LINE__,aPackage.iLineCall);				    	
				    
                    if (aPackage.iOperation == t_notification::ECFileManMove)
                        {
                        ((TFsNotification*) notification)->FileSize(fileSize);
                        //File just been created and written to.
                        if(fileSize == 4)
                            {
                            scratch |= ECFManWrite1;    
                            }
                        else if(fileSize == 8)
                            {
                            scratch |= ECFManWrite2;
                            }                            
                        }
                    else if (aPackage.iNotifyType == TFsNotification::EFileChange)
				        {
				        ((TFsNotification*) notification)->FileSize(fileSize);
				        aTest.Printf(_L("Filesize - %d (%d)\n"),fileSize,threadID);
				        //A notification is received every time the size is changed
				        //due to the flushing
				        if (aPackage.iOperation == t_notification::EFileWrite_async)
                            {
                            //We write 52 letters
                            if(fileSize != 52*(i+1))
                            	safe_test(aTest,KErrGeneral,__LINE__,aPackage.iLineCall);
                            }
                        else if (aPackage.iOperation == t_notification::EFileWrite_samesize)
                            {
                            //Only 4 letters in file
                            if(fileSize != 4)
                            	safe_test(aTest,KErrGeneral,__LINE__,aPackage.iLineCall);
                            }
                        else if (i < aPackage.iIterations)
                            {
                            //We write/increase size by 4 letters/bytes
                            if(fileSize != 4*(i+1))
                            	safe_test(aTest,KErrGeneral,__LINE__,aPackage.iLineCall);
                            }
                        else
                            {
                            //We decrease size by 4 bytes
                            if(fileSize != 4*(aPackage.iMaxNotifications-i))
                            	safe_test(aTest,KErrGeneral,__LINE__,aPackage.iLineCall);
                            }                           
				        }
				    }
				else if(notificationType == TFsNotification::ECreate)
					{
					if(!(((aPackage.iNotifyType & TFsNotification::ECreate) == TFsNotification::ECreate) &&
						   (aPackage.iOperation == t_notification::EFileCreate ||
							aPackage.iOperation == t_notification::EFileReplace ||
							aPackage.iOperation == t_notification::EFileCreate_subs || 
							aPackage.iOperation == t_notification::EFileCreate_subs_nowatch ||
							aPackage.iOperation == t_notification::EFileCreate_txt_nowatch ||
							aPackage.iOperation == t_notification::EFileCreate_txt ||
							aPackage.iOperation == t_notification::EFsMkDir ||
							aPackage.iOperation == t_notification::ECFileManMove ||
							aPackage.iOperation == t_notification::EAllOps1 ||
							aPackage.iOperation == t_notification::EAllOps2 ||
							aPackage.iOperation == t_notification::EAllOps3 ||
							aPackage.iOperation == t_notification::EAllOps4 ||
							aPackage.iOperation == t_notification::EAllOps5)))
						safe_test(aTest,KErrGeneral,__LINE__,aPackage.iLineCall);
					if (aPackage.iOperation == t_notification::EFileCreate_txt)
					    {
					    //Check filename is of correct extension
					    TPtrC _path;
					    ((TFsNotification*) notification)->Path(_path);
					    TBuf<5> _ext;
					    _ext.Append(_L(".txt"));
					    TBuf<5> ext;
					    ext.Append(_path.Right(4));
					    safe_test(aTest,ext.Match(_ext),__LINE__,aPackage.iLineCall);
					    }
					else if (aPackage.iOperation == t_notification::ECFileManMove)
					    {
					    if(scratch & ECFManCreate1)
					        {
					        scratch |= ECFManCreate2;  //File created second time    
					        }
					    else
					        {
					        scratch |= ECFManCreate1;  //File created first time
					        }
					    }
					}
				else if(notificationType == TFsNotification::EDelete)
					{
					if(!(((aPackage.iNotifyType & TFsNotification::EDelete) == TFsNotification::EDelete) &&
						   (aPackage.iOperation == t_notification::EFsDelete ||
							aPackage.iOperation == t_notification::EFsRmDir ||
							aPackage.iOperation == t_notification::EFsRmDir_nonEmpty ||
							aPackage.iOperation == t_notification::EFsRmDir_wild ||
							aPackage.iOperation == t_notification::ECFileManMove ||
							aPackage.iOperation == t_notification::EAllOps1 ||
							aPackage.iOperation == t_notification::EAllOps2 ||
							aPackage.iOperation == t_notification::EAllOps3 ||
							aPackage.iOperation == t_notification::EAllOps4)))
						safe_test(aTest,KErrGeneral,__LINE__,aPackage.iLineCall);
					
					if(aPackage.iOperation == t_notification::ECFileManMove)
					    {
					    if(scratch & ECFManDelete1)
					        {
					        scratch |= ECFManDelete2;
					        }
					    else
					        {
					        scratch |= ECFManDelete1;
					        }
					    }
					}
				else if(notificationType == TFsNotification::ERename)
					{
					if(!(((aPackage.iNotifyType & TFsNotification::ERename) == TFsNotification::ERename) &&
						   (aPackage.iOperation == t_notification::EFileRename ||
							aPackage.iOperation == t_notification::EFileRename_wild ||
							aPackage.iOperation == t_notification::EFsReplace ||
							aPackage.iOperation == t_notification::ECFileManMove ||
							aPackage.iOperation == t_notification::EFsRename ||
							aPackage.iOperation == t_notification::EFsRename_dir ||
							aPackage.iOperation == t_notification::EAllOps5)))
						safe_test(aTest,KErrGeneral,__LINE__,aPackage.iLineCall);
					TPtrC newnameC;
					((TFsNotification*)notification)->NewName(newnameC);
					aTest.Printf(_L("%S - (%d)\n"),&newnameC,threadID);
					
					TPtrC _pathC;
					((TFsNotification*) notification)->Path(_pathC);
					
					TBuf<40> _path;
					_path.Copy(_pathC);
					
					if (aPackage.iOperation == t_notification::EFsRename_dir)
						{
						_path.Delete(_path.Length()-1,1);
						_path.AppendNum(0);
						_path.Append(KPathDelimiter);
						}
					else if (aPackage.iOperation == t_notification::EFileRename_wild)
						{
						_path.Delete(_path.Length()-1,1);
						_path.AppendNum(numNotifications);
						}
					else
						{
						_path.AppendNum(0);
						}
				
					if(aPackage.iOperation != t_notification::ECFileManMove)
					    {
					    safe_test(aTest,newnameC.Match(_path),__LINE__,aPackage.iLineCall);    
					    }
					else if(scratch & ECFManRename1)
					    {
					    scratch |= ECFManRename2;
					    }
					else 
					    {
					    scratch |= ECFManRename1;
					    }
					
					}
				else if(notificationType == TFsNotification::EAttribute)
					{
					if(!(((aPackage.iNotifyType & TFsNotification::EAttribute) == TFsNotification::EAttribute) &&
						   (aPackage.iOperation == t_notification::EFsSetEntry ||
							aPackage.iOperation == t_notification::EFileSet ||
							aPackage.iOperation == t_notification::EFileSetAtt ||
							aPackage.iOperation == t_notification::EFileSetAtt_subs ||
							aPackage.iOperation == t_notification::ECFileManMove ||
							aPackage.iOperation == t_notification::EAllOps4)))
						safe_test(aTest,KErrGeneral,__LINE__,aPackage.iLineCall);
					TUint setAtt = 0;
					TUint clearAtt = 0;
					((TFsNotification*) notification)->Attributes(setAtt, clearAtt);
					
				    if(aPackage.iOperation == t_notification::ECFileManMove)
				        {
				        scratch |= ECFManAtt1;
				        }
				    else
				        {
			            if(setAtt != KEntryAttHidden)
			                safe_test(aTest,KErrGeneral,__LINE__,aPackage.iLineCall);
			            if(clearAtt != KEntryAttReadOnly)
			                safe_test(aTest,KErrGeneral,__LINE__,aPackage.iLineCall);
				        }
					}
				else if(notificationType == TFsNotification::EMediaChange)
					{
					if(!(((aPackage.iNotifyType & TFsNotification::EMediaChange) == TFsNotification::EMediaChange) &&
						   (aPackage.iOperation == t_notification::EMount || 
							aPackage.iOperation == t_notification::EMountScan || 
							aPackage.iOperation == t_notification::EDismount ||
							aPackage.iOperation == t_notification::EMountDismount ||
							aPackage.iOperation == t_notification::EMediaCardInsertion ||
							aPackage.iOperation == t_notification::EMediaCardRemoval ||
							aPackage.iOperation == t_notification::EFormat ||
							aPackage.iOperation == t_notification::ECFileManMove ||
							aPackage.iOperation == t_notification::ERawDiskWrite)))
						safe_test(aTest,KErrGeneral,__LINE__,aPackage.iLineCall);
					}
				else if(notificationType == TFsNotification::EDriveName)
					{
					if(!(((aPackage.iNotifyType & TFsNotification::EDriveName) == TFsNotification::EDriveName) &&
						   (aPackage.iOperation == t_notification::ESetDriveName ||
							aPackage.iOperation == t_notification::EAllOps6)))
						safe_test(aTest,KErrGeneral,__LINE__,aPackage.iLineCall);
					}
				else if(notificationType == TFsNotification::EVolumeName)
					{
					if(!(((aPackage.iNotifyType & TFsNotification::EVolumeName) == TFsNotification::EVolumeName) &&
						   (aPackage.iOperation == t_notification::ESetVolumeLabel ||
							aPackage.iOperation == t_notification::EAllOps6)))
						safe_test(aTest,KErrGeneral,__LINE__,aPackage.iLineCall);
					
					TPtrC newnameC;
					((TFsNotification*)notification)->NewName(newnameC);
					aTest.Printf(_L("New volume name: %S - (%d)\n"),&newnameC,threadID);
					}

				i++;
				notification = notify->NextNotification();
				if (notification == NULL)
					aTest.Printf(_L("Notification is NULL - (%d)\n"),threadID);
				}
		
		if(i==1) //First iteration will only ever get 1 notification
			{
			if(numNotifications != 1)
				safe_test(aTest,KErrGeneral,__LINE__,aPackage.iLineCall);
			}
		
		if(numNotifications < aPackage.iMaxNotifications && !overflow) //Ensure we get all of the notifications we expected
			{
			TRequestStatus status;
			notify->RequestNotifications(status);
			User::WaitForRequest(status);
			}
		}
	
	//0x307 = create1 | write1 | delete1 | rename1 | rename2 
	if(aPackage.iOperation == t_notification::ECFileManMove && (scratch != 0x307))
	    {
	    aTest.Printf(_L("CFileManMove test failure - scratch = 0x%x"),scratch);
	    safe_test(aTest,KErrGeneral,__LINE__,aPackage.iLineCall);
	    }
	    
	}

/*
 * Watches for changes in files/directories.
 * Used in TestMultipleNotificationsL().
 */
TInt MultipleNotificationsTFWatcher(TAny* aAny)
	{
	CTrapCleanup* cleanup;
	cleanup = CTrapCleanup::New();
	
	RThread thread;
	TUint64 threadID = thread.Id().Id();
	
	SThreadPackageMultiple package = *(SThreadPackageMultiple*) aAny;
	RTest multipleWatcherTest(_L("MultipleNotificationsTFWatcher"));
	multipleWatcherTest.Start(_L("MultipleNotificationsTFWatcher"));
	
	RFs fs;
	fs.Connect();
	
	multipleWatcherTest.Printf(_L("MultipleNotificationsTFWatcher (%d) - Create CFsNotify\n"),threadID);
	CFsNotify* notify = NULL;
	TRAPD(r,notify = CFsNotify::NewL(fs,package.iBufferSize));
	safe_test(multipleWatcherTest,r,__LINE__,package.iLineCall);
	TBuf<40> path;
	TBuf<20> filename;
	if(package.iNotifyType != TFsNotification::EMediaChange &&
	   package.iNotifyType != TFsNotification::EDriveName &&
	   package.iNotifyType != TFsNotification::EVolumeName &&
	   package.iOperation != t_notification::EAllOps6)
		{
		path.Append(gDriveToTest);
		path.Append(_L(":\\F32-TST\\T_NOTIFIER\\")); //len=22
		path.Append(package.iFileName);	
		filename.Append(package.iString);
		}
	else
		{
		path.Append((TChar)globalDriveNum+(TChar)'A');
		path.Append(_L(":"));
		}
	TBuf<40> fullname;
	fullname.Append(path);
	fullname.Append(filename);
	
	if (package.iNotifyType == TFsNotification::EVolumeName ||
		package.iOperation == t_notification::EAllOps6)
		{
		//Ensure volume has no label
		multipleWatcherTest.Printf(_L("Set volume label to nothing\n"));
		r = fs.SetVolumeLabel(_L(""),globalDriveNum);
		safe_test(multipleWatcherTest,r,__LINE__,package.iLineCall);
		}
	
	if (package.iNotifyType == TFsNotification::EDriveName ||
		package.iOperation == t_notification::EAllOps6)
		{
		//Ensure drive has no name
		multipleWatcherTest.Printf(_L("Set drive name to nothing\n"));
		r = fs.SetDriveName(globalDriveNum,_L(""));
		safe_test(multipleWatcherTest,r,__LINE__,package.iLineCall);
		}
	
	multipleWatcherTest.Printf(_L("MultipleNotificationsTFWatcher - Add Notification for path %S\n"),&path);
	multipleWatcherTest.Printf(_L("Add Notification for type %u - (%d)\n"),(TUint)package.iNotifyType,threadID);	
	r = notify->AddNotification((TUint)package.iNotifyType,path,filename);	
	safe_test(multipleWatcherTest,r,__LINE__,package.iLineCall);

	TRequestStatus status;
	multipleWatcherTest.Printf(_L("(%d) - Request Notifications\n"),threadID);
	r = notify->RequestNotifications(status);
	safe_test(multipleWatcherTest,r,__LINE__,package.iLineCall);
	
	if (package.iOperation == t_notification::EMediaCardRemoval)
		{
		multipleWatcherTest.Printf(_L("*****************************************************************\n"));
		multipleWatcherTest.Printf(_L("Waiting 10 seconds.\n"));
		multipleWatcherTest.Printf(_L("This is a MANUAL test, it requires the removal of the media card.\n"));
		multipleWatcherTest.Printf(_L("PLEASE REMOVE THE MEDIA CARD. (DriveNumber %d)\n"),globalDriveNum);
		multipleWatcherTest.Printf(_L("Or press Ctrl + F5 on the emulator.\n"));
		multipleWatcherTest.Printf(_L("*****************************************************************\n"));
		}
	if (package.iOperation == t_notification::EMediaCardInsertion)
		{
		multipleWatcherTest.Printf(_L("*******************************************************************\n"));
		multipleWatcherTest.Printf(_L("Waiting 10 seconds.\n"));
		multipleWatcherTest.Printf(_L("This is a MANUAL test, it requires the insertion of the media card.\n"));
		multipleWatcherTest.Printf(_L("PLEASE INSERT THE MEDIA CARD. (DriveNumber %d)\n"),globalDriveNum);
		multipleWatcherTest.Printf(_L("*******************************************************************\n"));
		}
	
	multipleWatcherTest.Printf(_L("(%d) - Signal Test thread to start Doer thread\n"),threadID);
	package.iBarrier.Signal();
	User::WaitForRequest(status);
	
	multipleWatcherTest.Printf(_L("(%d) - MultipleNotificationsTFWatcher Line %d\n"),threadID,__LINE__);
	
	//Handles the notifications
	HandleMultipleNotifications(multipleWatcherTest, package, notify, (TDesC&)fullname);
	
	multipleWatcherTest.Printf(_L("(%d) - MultipleNotificationsTFWatcher Line %d\n"),threadID,__LINE__);
	
	delete notify;
	fs.Close();
	multipleWatcherTest.End();
	multipleWatcherTest.Close();
	delete cleanup;
	return KErrNone;	
	}

/*
 * TestTwoNotificationsL - Tests File Write, 1 Doer writes to a file twice.
 *  1 Watcher watches for file write changes. Just so happens that the second one overflows.
 */
TInt TestTwoNotificationsL()
	{
	test.Next(_L("TestTwoNotifications"));
	
	RFs fs;
	fs.Connect();
	RSemaphore twoNotificationsDoerBar;
	SThreadPackageMultiple package;
	_LIT(KFileName,"file0.write");
	package.iIterations = 10;
	package.iOperation = t_notification::EFileWrite;
	package.iNotifyType = TFsNotification::EFileChange;
	package.iFileName = KFileName;
	package.iBufferSize = 100; //Should get changed to KMin... in CFsNotify::NewL
	
	User::LeaveIfError(twoNotificationsDoerBar.CreateLocal(0));
	User::LeaveIfError(package.iBarrier.CreateLocal(0));
	RThread watcher;
	RThread doer;
	
	TInt r = watcher.Create(_L("TestTwoNotificationsWatcherThread"),MultipleNotificationsTFWatcher,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&package);
	test_KErrNone(r);
	r = doer.Create(_L("TestTwoNotificationsDoerThread"),TwoNotificationsTFDoer,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&package);
	test_KErrNone(r);
	test.Next(_L("TestTwoNotifications - Resume Watcher"));
	watcher.Resume();
	test.Next(_L("TestTwoNotifications - Wait for Watcher to be ready"));
	package.iBarrier.Wait(); //Wait till Watcher has requested notification
	test.Next(_L("TestTwoNotifications - Resume Doer"));
	doer.Resume();
		
	test.Next(_L("TestTwoNotifications - Wait for doer thread death"));
	TRequestStatus status;
	doer.Logon(status);
	User::WaitForRequest(status);
	test(doer.ExitReason()==KErrNone);
	
	test.Next(_L("TestTwoNotifications - Wait for watcher thread death"));
	watcher.Logon(status);
	User::WaitForRequest(status);
	test(watcher.ExitReason()==KErrNone);
	
	CLOSE_AND_WAIT(doer);
	CLOSE_AND_WAIT(watcher);
	
	twoNotificationsDoerBar.Close();
	package.iBarrier.Close();
	fs.Close();
	
	return KErrNone;	
	}


/*
 * Watch two threads to receive two notifications.
 * Used in TestTwoDoersL().
 */
TInt TestTwoDoersWatcher(TAny* aAny)
	{
	CTrapCleanup* cleanup;
	cleanup = CTrapCleanup::New();
	
	RSemaphore& twoThreadsBarrier = *(RSemaphore*)aAny;
	RTest twoThreadsWatcherTest(_L("TwoThreadsWatcher"));
	twoThreadsWatcherTest.Start(_L("TwoThreadsWatcher"));
	
	RFs fs;
	fs.Connect();
	
	twoThreadsWatcherTest.Next(_L("Create CFsNotify"));
	CFsNotify* notify = NULL;
	TRAPD(r,notify = CFsNotify::NewL(fs,200));
	twoThreadsWatcherTest(r == KErrNone);
	
	TBuf<40> path1;
	TBuf<20> filename1;
	TBuf<40> path2;
	TBuf<20> filename2;
	path1.Append(gDriveToTest);
	path2.Append(gDriveToTest);
	path1.Append(_L(":\\F32-TST\\T_NOTIFIER\\"));
	filename1.Append(_L("file1.create"));
	path2.Append(_L(":\\F32-TST\\T_NOTIFIER\\"));
	filename2.Append(_L("file2.create"));
	TBuf<40> fullname1;
	fullname1.Append(path1);
	fullname1.Append(filename1);
	TBuf<40> fullname2;
	fullname2.Append(path2);
	fullname2.Append(filename2);
	
	twoThreadsWatcherTest.Printf(_L("TwoThreadsWatcher - Add Notification for %S\n"),&path1);
	r = notify->AddNotification((TUint)TFsNotification::ECreate,path1,filename1);
	twoThreadsWatcherTest(r == KErrNone);
	
	twoThreadsWatcherTest.Printf(_L("TwoThreadsWatcher - Add Notification for %S\n"),&path2);
	r = notify->AddNotification((TUint)TFsNotification::ECreate,path2,filename2);
	twoThreadsWatcherTest(r == KErrNone);
		
	TRequestStatus status;
	twoThreadsWatcherTest.Next(_L("TwoThreadsWatcher - Request Notifications"));
	r = notify->RequestNotifications(status);
	twoThreadsWatcherTest(r == KErrNone);

	twoThreadsBarrier.Signal(); //Signal Doer threads to start
	User::WaitForRequest(status);

	// We should be getting 2 notifications
	// Test notifications are not null and check notification types and paths
	// 1st notification:
	twoThreadsWatcherTest.Next(_L("TwoThreadsWatcher - First Notification"));
	const TFsNotification* notification = notify->NextNotification();
	twoThreadsWatcherTest(notification != NULL);
	twoThreadsWatcherTest.Next(_L("TwoThreadsWatcher - First Notification Type"));
	TFsNotification::TFsNotificationType notificationType = ((TFsNotification*)notification)->NotificationType();
	twoThreadsWatcherTest(notificationType == TFsNotification::ECreate);
	twoThreadsWatcherTest.Next(_L("TwoThreadsWatcher - First Notification Path"));
	TPtrC _pathC;
	((TFsNotification*)notification)->Path(_pathC);
	twoThreadsWatcherTest.Printf(_L("TwoThreadsWatcher - First Notification Path returned %S\n"),&_pathC);
	TBuf<40> _path;
	_path.Copy(_pathC);
	//We can't guarantee which thread ran first so check that it was either path1 or path2
	twoThreadsWatcherTest(_path.Match(fullname1) == KErrNone || _path.Match(fullname2) == KErrNone);
		
	// 2nd notification:
	twoThreadsWatcherTest.Next(_L("TwoThreadsWatcher - Second Notification"));
	notification = notify->NextNotification();	
	// Check if next notification exists
	if (!notification)
		{
		notify->RequestNotifications(status);
		User::WaitForRequest(status);
		notification = notify->NextNotification();
		}	
	twoThreadsWatcherTest(notification != NULL);
	twoThreadsWatcherTest.Next(_L("TwoThreadsWatcher - Second Notification Type"));
	notificationType = ((TFsNotification*)notification)->NotificationType();
	twoThreadsWatcherTest(notificationType == TFsNotification::ECreate);
	twoThreadsWatcherTest.Next(_L("TwoThreadsWatcher - Second Notification Path"));
	((TFsNotification*)notification)->Path(_pathC);
	twoThreadsWatcherTest.Printf(_L("TwoThreadsWatcher - Second Notification Path returned %S\n"),&_pathC);
	_path.Copy(_pathC);
	twoThreadsWatcherTest(_path.Match(fullname1) == KErrNone || _path.Match(fullname2) == KErrNone);
	
	delete notify;	
	fs.Close();
	twoThreadsWatcherTest.End();
	twoThreadsWatcherTest.Close();
	delete cleanup;
	return KErrNone;	
	}

/*
 * TestTwoDoersL - Two Doer threads create a file each and there's one Watcher
 *  which expects two notifications (one from each Doer).
 */
TInt TestTwoDoersL()
	{
	test.Next(_L("TestTwoDoers"));
	
	RFs fs;
	fs.Connect();
	
	_LIT(KFileName1,"file1.create");
	_LIT(KFileName2,"file2.create");
	RSemaphore simpleBarrierTest;
	SThreadPackage pkgDoer1;
	SThreadPackage pkgDoer2;
	pkgDoer1.iFileName = KFileName1;
	pkgDoer2.iFileName = KFileName2;
		
	User::LeaveIfError(pkgDoer1.iBarrier.CreateLocal(0));
	User::LeaveIfError(pkgDoer2.iBarrier.CreateLocal(0));
	User::LeaveIfError(simpleBarrierTest.CreateLocal(0));
	RThread watcher;
	RThread doer1;
	RThread doer2;
	
	watcher.Create(_L("TestTwoDoers-WatcherThread"),TestTwoDoersWatcher,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&simpleBarrierTest);
	doer1.Create(_L("TestTwoDoers-DoerThread1"),SimpleSingleNotificationTFDoer,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&pkgDoer1);
	doer2.Create(_L("TestTwoDoers-DoerThread2"),SimpleSingleNotificationTFDoer,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&pkgDoer2);
	watcher.Resume();
	simpleBarrierTest.Wait();	//Wait until Watcher has created CFsNotify
	
	doer1.Resume();
	doer2.Resume();
	
	test.Next(_L("TestTwoDoers - Wait for doer1 thread death"));
	TRequestStatus status;
	doer1.Logon(status);
	User::WaitForRequest(status);
	test(doer1.ExitReason()==KErrNone);
	
	test.Next(_L("TestTwoDoers - Wait for doer2 thread death"));
	doer2.Logon(status);
	User::WaitForRequest(status);
	test(doer2.ExitReason()==KErrNone);
	
	test.Next(_L("TestTwoDoers - Wait for watcher thread death"));
	watcher.Logon(status);
	RTimer timer1;
	TInt r = timer1.CreateLocal();
	safe_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
	TRequestStatus timeout;
	TTimeIntervalMicroSeconds32 time = 10000000;    //10 seconds
	timer1.After(timeout,time);
	User::WaitForRequest(timeout,status);
	test(status.Int() != KRequestPending);
	timer1.Cancel();
	timer1.Close();
	//User::WaitForRequest(status);
	test(watcher.ExitReason()==KErrNone);
	
	CLOSE_AND_WAIT(doer1);
	CLOSE_AND_WAIT(doer2);
	CLOSE_AND_WAIT(watcher);
	
	pkgDoer1.iBarrier.Close();
	pkgDoer2.iBarrier.Close();
	simpleBarrierTest.Close();
	fs.Close();
	return KErrNone;
	}

/*
 * TestTwoWatchersL - Uses two watcher threads and one doer thread to test that running 
 *  two distinct sub sessions at the same time (both watch the same file).
 */
TInt TestTwoWatchersL()
	{
	test.Next(_L("TestTwoWatchers"));
	RFs fs;
	fs.Connect();
	_LIT(KFileName,"file.creat3");
	SThreadPackage pkgDoer;
	pkgDoer.iFileName = KFileName;
	
	SThreadPackage watcherPkg;
	watcherPkg.iFileName = KFileName;
		
	User::LeaveIfError(pkgDoer.iBarrier.CreateLocal(0));
	User::LeaveIfError(watcherPkg.iBarrier.CreateLocal(0));
	RThread watcher;
	RThread watcher2;
	RThread doer;
	watcher.Create(_L("TestTwoWatchersWatcherThread"),SimpleSingleNotificationTFWatcher,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&watcherPkg);
	watcher2.Create(_L("TestTwoWatchersWatcher2Thread"),SimpleSingleNotificationTFWatcher,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&watcherPkg);
	doer.Create(_L("TestTwoWatchersDoerThread"),SimpleSingleNotificationTFDoer,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&pkgDoer);
	watcher.Resume();
	watcher2.Resume();
	watcherPkg.iBarrier.Wait(); //Wait till both watchers have requested notification
	watcherPkg.iBarrier.Wait(); 
	doer.Resume();

	test.Printf(_L("Wait for DOER to terminate , Line %d"),__LINE__);
	TRequestStatus status;
	doer.Logon(status);
	User::WaitForRequest(status);
	test(doer.ExitReason()==KErrNone);
	
	test.Printf(_L("Wait for WATCHER to terminate , Line %d"),__LINE__);
	watcher.Logon(status);
	RTimer timer1;
	TInt r = timer1.CreateLocal();
	safe_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
	TRequestStatus timeout;
	TTimeIntervalMicroSeconds32 time = 10000000;    //10 seconds
	timer1.After(timeout,time);
	User::WaitForRequest(timeout,status);
	test(status.Int() != KRequestPending);
	timer1.Cancel();
	timer1.Close();
//	User::WaitForRequest(status);
	test(watcher.ExitReason()==KErrNone);
	
	test.Printf(_L("Wait for WATCHER2 to terminate , Line %d"),__LINE__);
	watcher2.Logon(status);
	RTimer timer2;
	r = timer2.CreateLocal();
	safe_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
	TRequestStatus timeout2;
	timer2.After(timeout2,time);
	User::WaitForRequest(timeout2,status);
	test(status.Int() != KRequestPending);
	timer2.Cancel();
	timer2.Close();
	//User::WaitForRequest(status);
	test(watcher2.ExitReason()==KErrNone);
	
	CLOSE_AND_WAIT(doer);
	CLOSE_AND_WAIT(watcher);
	CLOSE_AND_WAIT(watcher2);
	
	pkgDoer.iBarrier.Close();
	watcherPkg.iBarrier.Close();
	fs.Close();
	return KErrNone;	
	}


/*
 * TestTwoWatchersTwoDoersL - Two watcher threads watches two different doer threads.
 */
TInt TestTwoWatchersTwoDoersL()
	{
	test.Next(_L("TestTwoWatchersTwoDoers"));
	RFs fs;
	fs.Connect();
	_LIT(KFileName1,"f1le.create");
	_LIT(KFileName2,"f2le.create");
	SThreadPackage package1;
	package1.iFileName = KFileName1;
	
	SThreadPackage package2;
	package2.iFileName = KFileName2;
		
	User::LeaveIfError(package1.iBarrier.CreateLocal(0));
	User::LeaveIfError(package2.iBarrier.CreateLocal(0));
	RThread watcher;
	RThread watcher2;
	RThread doer;
	RThread doer2;
	watcher.Create(_L("TestTwoWatchersTwoDoersWatcherThread"),SimpleSingleNotificationTFWatcher,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&package1);
	doer.Create(_L("TestTwoWatchersTwoDoersDoerThread"),SimpleSingleNotificationTFDoer,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&package1);
	
	watcher2.Create(_L("TestTwoWatchersTwoDoersWatcher2Thread"),SimpleSingleNotificationTFWatcher,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&package2);
	doer2.Create(_L("TestTwoWatchersTwoDoersDoer2Thread"),SimpleSingleNotificationTFDoer,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&package2);
	watcher.Resume();
	watcher2.Resume();
	package1.iBarrier.Wait(); //Wait till both watchers have requested notification
	package2.iBarrier.Wait(); 
	doer.Resume();
	doer2.Resume();
	
	test.Printf(_L("Wait for DOER to terminate , Line %d"),__LINE__);
	TRequestStatus status;
	doer.Logon(status);
	User::WaitForRequest(status);
	test(doer.ExitReason()==KErrNone);
	
	test.Printf(_L("Wait for DOER2 to terminate , Line %d"),__LINE__);
	doer2.Logon(status);
	User::WaitForRequest(status);
	test(doer2.ExitReason()==KErrNone);

	test.Printf(_L("Wait for WATCHER to terminate , Line %d"),__LINE__);
	watcher.Logon(status);
	RTimer timer1;
	TInt r = timer1.CreateLocal();
	safe_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
	TRequestStatus timeout;
	TTimeIntervalMicroSeconds32 time = 10000000;    //10 seconds
	timer1.After(timeout,time);
	User::WaitForRequest(timeout,status);
	test(status.Int() != KRequestPending);
	timer1.Cancel();
	timer1.Close();
	test(watcher.ExitReason()==KErrNone);
	
	test.Printf(_L("Wait for WATCHER2 to terminate , Line %d"),__LINE__);
	watcher2.Logon(status);
	RTimer timer2;
	r = timer2.CreateLocal();
	safe_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
	TRequestStatus timeout2;
	timer2.After(timeout2,time);
	User::WaitForRequest(timeout2,status);
	test(status.Int() != KRequestPending);
	timer2.Cancel();
	timer2.Close();
	test(watcher2.ExitReason()==KErrNone);
	
	CLOSE_AND_WAIT(doer);
	CLOSE_AND_WAIT(doer2);
	CLOSE_AND_WAIT(watcher);
	CLOSE_AND_WAIT(watcher2);
	
	package1.iBarrier.Close();
	package2.iBarrier.Close();
	fs.Close();
	return KErrNone;		
	}


/*
 * Multi-purpose test
 * 
 * If aFailureExpected is ETrue, it is expected that the watcher thread is not terminated normally,
 * due to the notification(s) not being sent.
 * Since this function is called many times, aLineCall is used to show the line where it is called from.
 * See SThreadPackageMultiple.
 */
TInt TestMultipleNotificationsL(const TDesC& aFilename, const TDesC& aString, TInt aIterations,
								TInt aMaxNotifications, t_notification::EOperation aOperation,
								TUint aNotifyType, TInt aBufferSize, TBool aFailureExpected, TInt aLineCall)
	{
	test.Next(_L("TestMultipleNotifications"));
	
	RFs fs;
	fs.Connect();
	SThreadPackageMultiple package;
	package.iIterations = aIterations;
	package.iMaxNotifications = aMaxNotifications;
	package.iOperation = aOperation;
	package.iNotifyType = (TFsNotification::TFsNotificationType)aNotifyType;
	package.iString = aString;
	package.iFileName = aFilename;
	package.iBufferSize = aBufferSize;
	package.iLineCall = aLineCall;
	
	User::LeaveIfError(package.iBarrier.CreateLocal(0));
	RThread watcher;
	RThread doer;
	RTimer tim;
	User::LeaveIfError(tim.CreateLocal());
	
	TInt r = watcher.Create(_L("TestMultipleNotificationsWatcherThread"),MultipleNotificationsTFWatcher,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&package);
	safe_test(test,r,__LINE__,package.iLineCall);
	r = doer.Create(_L("TestMultipleNotificationsDoerThread"),MultipleNotificationTFDoer,KDefaultStackSize*2,KMinHeapSize,KMaxHeapSize,&package);
	safe_test(test,r,__LINE__,package.iLineCall);
	test.Next(_L("TestMultipleNotifications - Resume Watcher"));
	watcher.Resume();
	test.Next(_L("TestMultipleNotifications - Wait for Watcher to be ready"));
	package.iBarrier.Wait(); //Wait till Watcher has requested notification
	test.Next(_L("TestMultipleNotifications - Resume Doer"));
	doer.Resume();
		
	test.Next(_L("TestMultipleNotifications - Wait for doer thread death"));
	TRequestStatus status;
	doer.Logon(status);
	User::WaitForRequest(status);
	test.Printf(_L("TestMultipleNotifications - Doer Exit Reason %d\n"),doer.ExitReason());
	safe_test(test,doer.ExitReason(),__LINE__,package.iLineCall);
	
	TRequestStatus timStatus;
	TTimeIntervalMicroSeconds32 timeout;
	if (aFailureExpected && !(package.iOperation == t_notification::EMediaCardInsertion ||
							  package.iOperation == t_notification::EMediaCardRemoval))
		{
		timeout = 1500000; //1.5 seconds, we don't want to wait too long if we expect it to fail
		}
	else
		{
		timeout = 10000000; //10 seconds
		}
	tim.After(timStatus,timeout);
	
	test.Next(_L("TestMultipleNotifications - Wait for watcher thread death or timeout"));
	watcher.Logon(status);
	User::WaitForRequest(status,timStatus);
	if(!(status != KRequestPending || aFailureExpected))
		safe_test(test,KErrGeneral,__LINE__,package.iLineCall);
	
	test.Printf(_L("TestMultipleNotifications - Watcher Exit Reason %d\n"),watcher.ExitReason());
	safe_test(test,watcher.ExitReason(),__LINE__,package.iLineCall);
	
	CLOSE_AND_WAIT(doer);
	
	if(status == KRequestPending)
		{
		watcher.Kill(KErrTimedOut);
		test.Printf(_L("TestMultipleNotifications - Watcher timed out\n"));
		}
	CLOSE_AND_WAIT(watcher);
	
	package.iBarrier.Close();
	fs.Close();
	tim.Close();
	test.Printf(_L("----------------------------------------------------------------------\n"));
	return KErrNone;	
	}

TInt TestMultipleNotificationsL(const TDesC& aFilename, const TDesC& aString, TInt aIterations,
								TInt aMaxNotifications, t_notification::EOperation aOperation,
								TFsNotification::TFsNotificationType aNotifyType, TInt aBufferSize,
								TBool aFailureExpected, TInt aLineCall)
	{
	return TestMultipleNotificationsL(aFilename, aString, aIterations, aMaxNotifications, aOperation,
									 (TUint)aNotifyType, aBufferSize, aFailureExpected, aLineCall);
	}

TInt TestMultipleNotificationsL(const TDesC& aFilename, TInt aIterations, TInt aMaxNotifications,
								t_notification::EOperation aOperation, TUint aNotifyType, TInt aBufferSize,
								TBool aFailureExpected, TInt aLineCall)
	{
	return TestMultipleNotificationsL(aFilename,_L(""), aIterations, aMaxNotifications, aOperation, aNotifyType,
									  aBufferSize, aFailureExpected, aLineCall);
	}


// Watcher for TestAddRemoveNotificationL()
TInt TestAddRemoveNotificationWatcher(TAny* aAny)
	{
	CTrapCleanup* cleanup;
	cleanup = CTrapCleanup::New();
	RThread thread;
	TUint64 threadId = thread.Id().Id();
	
	SThreadPackage pkgDoer = *(SThreadPackage*)aAny;
	RSemaphore& addRemoveBarrier = pkgDoer.iBarrier;
	
	RTest addRemoveWatcherTest(_L("TestAddRemoveNotificationWatcher"));
	addRemoveWatcherTest.Start(_L("TestAddRemoveNotificationWatcher"));
	
	RFs fs;
	fs.Connect();
	
	addRemoveWatcherTest.Printf(_L("TestAddRemoveNotificationWatcher(%d) - Create CFsNotify\n"),threadId);
	CFsNotify* notify = NULL;
	TRAPD(r,notify = CFsNotify::NewL(fs,100); );
	addRemoveWatcherTest( r == KErrNone);
	TBuf<40> path;
	TBuf<20> filename;
	path.Append(gDriveToTest);
	path.Append(_L(":\\F32-TST\\T_NOTIFIER\\")); //len=22
	filename.Append(pkgDoer.iFileName);
	
	addRemoveWatcherTest.Printf(_L("TestAddRemoveNotificationWatcher - Add Notification for %S\n"),&path);
	r = notify->AddNotification((TUint)TFsNotification::ECreate,path,filename);
	addRemoveWatcherTest(r==KErrNone);
	addRemoveWatcherTest.Printf(_L("TestAddRemoveNotificationWatcher(%d) - Remove Notifications\n"),threadId);
	r = notify->RemoveNotifications();
	addRemoveWatcherTest(r==KErrNone);
	addRemoveWatcherTest.Printf(_L("TestAddRemoveNotificationWatcher(%d) - Request Notifications\n"),threadId);
	TRequestStatus status;
	r = notify->RequestNotifications(status);
	addRemoveWatcherTest(r==KErrNone);
	addRemoveWatcherTest.Printf(_L("TestAddRemoveNotificationWatcher status = %d"),status.Int());
	addRemoveBarrier.Signal();
	
	addRemoveWatcherTest.Printf(_L("TestAddRemoveNotificationWatcher(%d) - NextNotification\n"),threadId);
	//We should not be getting any notifications as the notification request has been removed
	const TFsNotification* notification = notify->NextNotification();
	addRemoveWatcherTest(notification == NULL);
	
	delete notify;
	fs.Close();
	addRemoveWatcherTest.End();
	addRemoveWatcherTest.Close();
	delete cleanup;
	return KErrNone;
	}


/*
 * TestAddRemoveNotificationL - Watcher adds and removes notification request.
 *  Any changes by doer thread should not be detected.
 */
TInt TestAddRemoveNotificationL()
	{
	test.Next(_L("TestAddRemoveNotification"));
	RFs fs;
	fs.Connect();

	SThreadPackage package;
	_LIT(KFileName,"noFile.create");
	package.iFileName = KFileName;

	User::LeaveIfError(package.iBarrier.CreateLocal(0));
	RThread watcher;
	RThread doer;

	watcher.Create(_L("TestAddRemoveNotification-WatcherThread"),TestAddRemoveNotificationWatcher,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&package);
	doer.Create(_L("TestAddRemoveNotification-DoerThread"),SimpleSingleNotificationTFDoer,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&package);
	watcher.Resume();
	
	test.Printf(_L("TestAddRemoveNotification - Wait until Watcher has created CFsNotify\n"));
	package.iBarrier.Wait();	//Wait until Watcher has created CFsNotify
	doer.Resume();
	
	test.Next(_L("TestAddRemoveNotification - Wait for doer thread death"));
	TRequestStatus status;
	doer.Logon(status);
	User::WaitForRequest(status);
	test(doer.ExitReason()==KErrNone);

	test.Next(_L("TestAddRemoveNotification - Wait for watcher thread death"));
	watcher.Logon(status);
	RTimer timer1;
	TInt r = timer1.CreateLocal();
	safe_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
	TRequestStatus timeout;
	TTimeIntervalMicroSeconds32 time = 10000000;    //10 seconds
	timer1.After(timeout,time);
	User::WaitForRequest(timeout,status);
	test(status.Int() != KRequestPending);
	timer1.Cancel();
	timer1.Close();
	test.Printf(_L("Test - Watcher Exit Reason %d\n"),watcher.ExitReason());
	test(watcher.ExitReason()==KErrNone);

	CLOSE_AND_WAIT(doer);
	CLOSE_AND_WAIT(watcher);

	package.iBarrier.Close();
	fs.Close();
	return KErrNone;	
	}


/*
 * Adds and cancels notification request.
 * Used in TestCancelNotificationL().
 */
TInt TestCancelNotificationWatcher(TAny* aAny)
	{
	CTrapCleanup* cleanup;
	cleanup = CTrapCleanup::New();
	
	RTest cancelNotificationsWatcherTest(_L("TestCancelNotificationWatcher"));
	cancelNotificationsWatcherTest.Start(_L("TestCancelNotificationWatcher"));
	
	RThread thread;
	TUint64 threadId = thread.Id().Id();
	
	SThreadPackageDualSemaphore pkgDoer = *(SThreadPackageDualSemaphore*)aAny;
	RSemaphore& addRemoveBarrier = pkgDoer.iBarrier;
	RSemaphore& addRemoveBarrier2 = pkgDoer.iBarrier2;
	
	RFs fs;
	fs.Connect();
	
	cancelNotificationsWatcherTest.Printf(_L("TestCancelNotificationWatcher(%d) - Create CFsNotify\n"),threadId);
	CFsNotify* notify = NULL;
	TRAPD(r,notify = CFsNotify::NewL(fs,100); );
	cancelNotificationsWatcherTest(r == KErrNone);
	TBuf<40> path;
	TBuf<20> filename;
	path.Append(gDriveToTest);
	path.Append(_L(":\\F32-TST\\T_NOTIFIER\\")); //len=22
	filename.Append(pkgDoer.iFileName);
	
	cancelNotificationsWatcherTest.Printf(_L("TestCancelNotificationWatcher - Add Notification for %S\n"),&path);
	r = notify->AddNotification((TUint)TFsNotification::ECreate,path,filename);
	cancelNotificationsWatcherTest(r==KErrNone);
	cancelNotificationsWatcherTest.Printf(_L("TestCancelNotificationWatcher(%d) - Request Notifications\n"),threadId);
	TRequestStatus status;
	r = notify->RequestNotifications(status);
	cancelNotificationsWatcherTest(r==KErrNone);
	
	cancelNotificationsWatcherTest.Printf(_L("TestCancelNotificationWatcher(%d) - Cancel Notifications\n"),threadId);
	r = notify->CancelNotifications(status);
	cancelNotificationsWatcherTest(r==KErrNone);
	
	cancelNotificationsWatcherTest.Printf(_L("TestCancelNotificationWatcher(%d) - Signal W1 - Start doer\n"),threadId);
	addRemoveBarrier.Signal(); //W1 - Start doer

	cancelNotificationsWatcherTest.Printf(_L("TestCancelNotificationWatcher(%d) - Wait S1 - doer complete\n"),threadId);
	addRemoveBarrier2.Wait();	//S1 - Wait for doer to have created file
	
	cancelNotificationsWatcherTest.Printf(_L("TestCancelNotificationWatcher(%d) - NextNotification\n"),threadId);
	//We should not be getting any notifications as the notification request has been removed
	const TFsNotification* notification = notify->NextNotification();
	cancelNotificationsWatcherTest(notification == NULL);
	
	delete notify;
	fs.Close();
	cancelNotificationsWatcherTest.Printf(_L("TestCancelNotificationWatcher(%d) - Complete\n"),threadId);
	cancelNotificationsWatcherTest.End();
	cancelNotificationsWatcherTest.Close();
	delete cleanup;
	return KErrNone;
	}


/*
 * TestCancelNotificationL - Watcher adds and cancels notification request.
 */
TInt TestCancelNotificationL()
	{
	test.Next(_L("TestCancelNotification"));
	RFs fs;
	fs.Connect();

	SThreadPackageDualSemaphore package;
	_LIT(KFileName,"cancel.create");
	package.iFileName = KFileName;

	User::LeaveIfError(package.iBarrier.CreateLocal(0));
	User::LeaveIfError(package.iBarrier2.CreateLocal(0));
	RThread watcher;
	RThread doer;

	TInt r = watcher.Create(_L("TestCancelNotification-WatcherThread"),TestCancelNotificationWatcher,KDefaultStackSize*2,KMinHeapSize,KMaxHeapSize,&package);
	test_KErrNone(r);
	r = doer.Create(_L("TestCancelNotification-DoerThread"),SimpleSingleNotificationTFDoer,KDefaultStackSize*2,KMinHeapSize,KMaxHeapSize,&package);
	test_KErrNone(r);
	test.Printf(_L("TestCancelNotificationL - Watcher.Resume()"));
	watcher.Resume();
	test.Printf(_L("TestCancelNotificationL - Waiting on package.iBarrier.Wait()"));
	package.iBarrier.Wait();	//W1 - Wait until Watcher has created CFsNotify
	test.Printf(_L("TestCancelNotificationL -Doer Resume"));
	TRequestStatus status;
	doer.Logon(status);
	doer.Resume();
	
	test.Next(_L("TestCancelNotification - Wait for doer thread death"));
	User::WaitForRequest(status);
	test(doer.ExitReason()==KErrNone);
	
	package.iBarrier2.Signal(); //S1

	test.Next(_L("TestCancelNotification - Wait for watcher thread death"));
	watcher.Logon(status);
	
	RTimer tim;
	r = tim.CreateLocal();
	test_KErrNone(r);
	
	TRequestStatus timStatus;
	TTimeIntervalMicroSeconds32 time = 10000000; //10 seconds
	tim.After(timStatus,time);
	
	User::WaitForRequest(status,timStatus);
	test(status!=KRequestPending);
	test(watcher.ExitReason()==KErrNone);

	CLOSE_AND_WAIT(doer);
	CLOSE_AND_WAIT(watcher);

	package.iBarrier.Close();
	package.iBarrier2.Close();
	fs.Close();
	tim.Close();
	return KErrNone;	
	}

/*
 * Test that if we close the session 
 * before closing the subsession (deleting CFsNotify)
 * that everything is A-Ok.
 */
TInt TestSessionCloseTF(TAny* aTestCase)
	{
	CTrapCleanup* cleanup;
	cleanup = CTrapCleanup::New();
	
	TRAPD(r,
	RFs fs;
	fs.Connect();
	RDebug::Printf("TestSessionClose\n");
	
	SThreadPackage2 package = *(SThreadPackage2*)aTestCase;
	package.iBarrier.Signal();
	
	switch(package.iTestCase)
		{
		case 1:
			{
			RDebug::Printf("TestSessionCloseTF - Case 1 - NewL\n");
			CFsNotify* notify = CFsNotify::NewL(fs,KMinNotificationBufferSize);
			User::LeaveIfNull(notify);
			
			RDebug::Printf("TestSessionCloseTF - Case 1 - Fs.Close\n");
			fs.Close();
			
			TBuf<45> path;
			path.Append((TChar)gDriveToTest);
			path.Append(_L(":\\F32-TST\\T_NOTIFIER\\"));
			
			TBuf<20> filename;
			filename.Append(_L("session.close"));
			
			CleanupStack::PushL(notify);
			
			RDebug::Printf("TestSessionCloseTF - Case 1 - Add Notification - Panic Expected\n");
			r = notify->AddNotification((TUint)TFsNotification::ECreate,path,filename);
			User::LeaveIfError(r);
			
			RDebug::Printf("TestSessionCloseTF - Case 1 - After Session Close\n");
			fs.Close();
			CleanupStack::Pop(notify);
			
			RDebug::Printf("TestSessionCloseTF - Case 1 - After Delete Notify\n");
			delete notify;
			break;
			}
		case 2:
			{
			RDebug::Printf("TestSessionCloseTF - Case 2 - NewL\n");
			CFsNotify* notify = CFsNotify::NewL(fs,KMinNotificationBufferSize);
			User::LeaveIfNull(notify);
			
			TBuf<45> path;
			path.Append((TChar)gDriveToTest);
			path.Append(_L(":\\F32-TST\\T_NOTIFIER\\"));
			TBuf<20> filename;
			filename.Append(_L("session.close"));
			
			RDebug::Printf("TestSessionCloseTF - Case 2 - Add Notification\n");
			r = notify->AddNotification((TUint)TFsNotification::ECreate,path,filename);
			test_KErrNone(r);
			
			RDebug::Printf("TestSessionCloseTF - Case 2 - Fs.Close\n");
			fs.Close();

			CleanupStack::PushL(notify);
			TRequestStatus status;
			RDebug::Printf("TestSessionCloseTF - Case 2 - Request Notification - Panic Expected\n");
			r = notify->RequestNotifications(status);
			CleanupStack::Pop(notify);
			
			RDebug::Printf("TestSessionCloseTF - Case 2 - After Delete Notify\n");
			delete notify;
			break;
			}
		default:
			{
			break;
			}
		}
	);
	delete cleanup;
	return r;
	}

/*
 * Test that if we close the session
 * before closing the subsession (deleting CFsNotify)
 * that everything is A-Ok.
 */
void TestSessionClose(TInt aTestCase)
	{
	RSemaphore sem;
	User::LeaveIfError(sem.CreateLocal(0));
	
	SThreadPackage2 package;
	package.iTestCase = aTestCase;
	package.iBarrier = sem;
	
	RThread thread;
	thread.Create(_L("TestSessionClose"),TestSessionCloseTF,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&package);
	
	thread.Resume();
	sem.Wait();
	
	TRequestStatus status;
	thread.Logon(status);
	User::WaitForRequest(status);
	test.Printf(_L("Kern-Exec 0 is EXPECTED\n"));
	TInt err = thread.ExitReason();
	test_KErrNone(err);
	TExitType et = thread.ExitType();
	test(et == EExitPanic);
	CLOSE_AND_WAIT(thread);
	sem.Close();
	}

const TInt KNotificationOverflowIterationLimit = 7;

/*
 * Does stuff for TestOverflowL
 * Synchronises such that watchers have seen 1 change.
 * Then fills their buffers up to KNotificationOverflowIterationLimit.
 * 
 */
TInt TestOverflowDoerTF(TAny* aAny)
	{
	RFs fs;
	fs.Connect();
	
	SThreadPackage& package = *(SThreadPackage*) aAny; 
	
	TBuf<45> path;
	path.Append((TChar)gDriveToTest);
	path.Append(_L(":\\F32-TST\\T_NOTIFIER\\"));
	path.Append(package.iFileName);
	
	fs.MkDirAll(path);
	
	RFile file;
	TInt r = file.Replace(fs,path,EFileWrite);
	User::LeaveIfError(r);
	
	//Perform first set size.
	r = file.SetSize(1);
	User::LeaveIfError(r);
	
	//Wait until both watchers have received this change.
	package.iBarrier.Wait();
	
	for(TInt i = 0; i< KNotificationOverflowIterationLimit; i++)
		{
		file.SetSize(i);
		}
	
	file.Close();
	fs.Close();	
	return KErrNone;
	}

/*
 * Thread function used as part of TestOverflowL
 * Counts the number of notifications and ensures it the correct number before overflow is received#
 */
TInt TestOverflowWatcher1TF(TAny* aAny)
	{
	CTrapCleanup* cleanup;
	cleanup = CTrapCleanup::New();
	
	RTest overflowTest(_L("TestOverflowWatcher1TF"));
	overflowTest.Start(_L("TestOverflowWatcher1TF"));
	
	SThreadPackage& package = *(SThreadPackage*) aAny; 
	RFs fs;
	fs.Connect();
	TBuf<45> path;
	TBuf<20> filename;
	path.Append((TChar)gDriveToTest);
	path.Append(_L(":\\F32-TST\\T_NOTIFIER\\"));
	filename.Append(package.iFileName);
	
	TRequestStatus status;
	CFsNotify* notify = NULL;
	
	//This notification's size is 80.
	//80*7 = 560.
	// -4 means we should get 6 notifications
	//Except the first one will still be in the buffer
	// (as we've not called RequestNotification yet) so we'll only actually get 5.
	TRAPD(r, notify = CFsNotify::NewL(fs,(80*7)-4));
	test_KErrNone(r);
	User::LeaveIfNull(notify);
	notify->AddNotification(TFsNotification::EFileChange,path,filename);
	notify->RequestNotifications(status);
	
	//Signal that we are ready for doer to start (W1)
	package.iBarrier.Signal();
	
	//We wait for the 1 notification (doer only does 1 at first)
	User::WaitForRequest(status);
	
	overflowTest.Next(_L("Overflow- Get First Notification (Start framework)"));
	const TFsNotification *notification = notify->NextNotification();
	
	TFsNotification::TFsNotificationType type = notification->NotificationType();
	overflowTest.Printf(_L("Overflow - First Notification Type = %d\n"),type);
	
	//Signal the test thread (W2)
	package.iBarrier.Signal();
	//Wait for Signal to continue (W3);
	package.iBarrier.Wait();
	
	notify->RequestNotifications(status);
	User::WaitForRequest(status);
	
	TInt count = 0;
	overflowTest.Next(_L("Overflow- Get the rest of the notifications"));
	notification = notify->NextNotification();
	while(notification != NULL)
		{
		
		type = notification->NotificationType();
		overflowTest.Printf(_L("Overflow - NotificationType = %d\n"),type);
		if(type & TFsNotification::EOverflow)
			{
			delete notify;
			fs.Close();
			overflowTest.Printf(_L("Overflow +- Count = %d\n"),count);
			overflowTest.End();
			overflowTest.Close();
			return count;
			}
		
		notification = notify->NextNotification();
		count++;
		}
	overflowTest.Printf(_L("Overflow -- Count = %d\n"),count);
	
	overflowTest.End();
	overflowTest.Close();
	delete notify;
	delete cleanup;
	fs.Close();
	return -1;
	}


/*
 * Overflow test
 * As some of the tests above assume sucess if they receive an overflow 
 * we need to ensure that overflow works properly!
 */
TInt TestOverflowL()
	{
	/*
	 * The scheme used is as follows:
	 * 1 Doer thread which is setting the size of a file, over and over.
	 * 1 watcher thread.
	 * 
	 * The doer thread does 1 operation then waits on a signal.
	 * The watcher thread requests notification and receives 1 notification.
	 * It then signals the Doer thread.
	 * 
	 * The doer thread continues doing setsize until the number of notifications
	 * should have overflowed.
	 * 
	 * The watcher Waits for a signal from doer (that all of the notifications have been sent).
	 * The watcher's last notification should be an overflow
	 */
	test.Next(_L("TestOverflow"));
	RFs fs;
	TInt r = fs.Connect();
	test_KErrNone(r);
	_LIT(KFileName,"over.flow");
	SThreadPackage doerPkg;
	doerPkg.iFileName = KFileName;
	
	SThreadPackage watcher1Pkg;
	watcher1Pkg.iFileName = KFileName;
		
	User::LeaveIfError(doerPkg.iBarrier.CreateLocal(0));
	User::LeaveIfError(watcher1Pkg.iBarrier.CreateLocal(0));
	RThread watcher1;
	RThread doer;
	watcher1.Create(_L("TestOverflowWatcher1Thread"),TestOverflowWatcher1TF,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&watcher1Pkg);
	doer.Create(_L("TestOverflowDoerThread"),TestOverflowDoerTF,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&doerPkg);
	watcher1.Resume();

	//Wait until Request has been requested. (W1)
	watcher1Pkg.iBarrier.Wait();
	
	doer.Resume();
	
	//Wait till watcher has received first notification (W2)
	watcher1Pkg.iBarrier.Wait(); 

	//Signal the doer that it is free to continue
	//doing the rest of the operations
	doerPkg.iBarrier.Signal();
	
	test.Next(_L("TestOverflow - Wait for doer thread death"));
	TRequestStatus status;
	doer.Logon(status);
	User::WaitForRequest(status);
	test(doer.ExitReason()==KErrNone);
	CLOSE_AND_WAIT(doer);
	
	//Wait until doer has finished doing notifications
	//thus the watcher should receive an overflow
	// (W3)
	watcher1Pkg.iBarrier.Signal();
	
	RTimer tim;
	r = tim.CreateLocal();
	test_KErrNone(r);
	TRequestStatus timStatus;
	
	test.Next(_L("TestOverflow - Wait for watcher1 thread death"));
	TTimeIntervalMicroSeconds32 interval = 10000000; //10 seconds
	tim.After(timStatus,interval);
	watcher1.Logon(status);
	User::WaitForRequest(status,timStatus);
	test(status != KRequestPending);
	/*
	 * The number of notifications returned here should be 5.
	 * This is because :
	 * 
	 * The first notification means that the buffer has lost 80 (the size of this
	 * particular notification). Even though the client has read it becase they've not called
	 * RequestNotification the server doesn't know that yet so that's why it's 5 not 6.
	 * 
	 * That leaves 556 - 80. Which means only 5 notifications will fit.
	 */
	TInt count = watcher1.ExitReason();
	test(count==5);
	
	CLOSE_AND_WAIT(watcher1);
	watcher1Pkg.iBarrier.Close();
	doerPkg.iBarrier.Close();
	fs.Close();
	tim.Close();
	return KErrNone;
	}

/*
 * Does stuff for TestPostOverflowL
 * Synchronises such that watchers have seen 1 change.
 * Then fills their buffers up to KNotificationOverflowIterationLimit.
 * Then continues to request changes and akes sure that it gets 3 non-overflow notifications
 * For DEF140387.
 */
TInt TestPostOverflowDoerTF(TAny* aAny)
    {
    RFs fs;
    fs.Connect();
    
    SThreadPackage& package = *(SThreadPackage*) aAny; 
    
    TBuf<45> path;
    path.Append((TChar)gDriveToTest);
    path.Append(_L(":\\F32-TST\\T_NOTIFIER\\"));
    path.Append(package.iFileName);
    
    fs.MkDirAll(path);
    
    RFile file;
    TInt r = file.Replace(fs,path,EFileWrite);
    User::LeaveIfError(r);
    
    //Perform first set size.
    r = file.SetSize(1);
    User::LeaveIfError(r);
    
    //Wait until both watchers have received this change.
    //D-W-1
    package.iBarrier.Wait();
    
    for(TInt i = 0; i< KNotificationOverflowIterationLimit; i++)
        {
        file.SetSize(i);
        }
    
    file.Close();
    fs.Close(); 
    return KErrNone;
    }

TInt HandlePostOverflow(SThreadPackage& aPackage, CFsNotify* aNotify)
    {
    TRequestStatus status;
    TInt r = aNotify->RequestNotifications(status);
    test_KErrNone(r);
    //Signal that overflow has been found (W4)
    aPackage.iBarrier.Signal();
    
    User::WaitForRequest(status);
    
    const TFsNotification* notification = NULL;
    TInt count = 1;
    
    notification = aNotify->NextNotification();
    test(notification != NULL);
    
    //3 set sizes will be done (Sx)
    aPackage.iBarrier.Wait();
    
    while(count < 3)
        {
        TUint type = notification->NotificationType();
        if(type & TFsNotification::EOverflow)
            {
            return KErrOverflow;
            }
        notification = aNotify->NextNotification();
        if(notification == NULL)
            {
            r = aNotify->RequestNotifications(status);
            test_KErrNone(r);
            User::WaitForRequest(status);
            notification = aNotify->NextNotification();
            }
        test(notification != NULL);
        count++;
        }
    return count;
    }


/*
 * Thread function used as part of TestOverflowL
 * Counts the number of notifications and ensures it the correct number before overflow is received#
 */
TInt TestPostOverflowWatcher1TF(TAny* aAny)
    {
    CTrapCleanup* cleanup;
    cleanup = CTrapCleanup::New();
    
    RTest overflowTest(_L("TestOverflowWatcher1TF"));
    overflowTest.Start(_L("TestOverflowWatcher1TF"));
    
    SThreadPackage& package = *(SThreadPackage*) aAny; 
    RFs fs;
    fs.Connect();
    TBuf<45> path;
    TBuf<20> filename;
    path.Append((TChar)gDriveToTest);
    path.Append(_L(":\\F32-TST\\T_NOTIFIER\\"));
    filename.Append(package.iFileName);
    
    TRequestStatus status;
    CFsNotify* notify = NULL;
    
    //This notification's size is 80.
    //80*7 = 560.
    // -4 means we should get 6 notifications
    //Except the first one will still be in the buffer
    // (as we've not called RequestNotification yet) so we'll only actually get 5.
    TRAPD(r, notify = CFsNotify::NewL(fs,(80*7)-4));
    test_KErrNone(r);
    User::LeaveIfNull(notify);
    notify->AddNotification(TFsNotification::EFileChange,path,filename);
    notify->RequestNotifications(status);
    
    //Signal that we are ready for doer to start (W1)
    package.iBarrier.Signal();
    
    //We wait for the 1 notification (doer only does 1 at first)
    User::WaitForRequest(status);
    
    overflowTest.Next(_L("Overflow- Get First Notification (Start framework)"));
    const TFsNotification *notification = notify->NextNotification();
    
    TFsNotification::TFsNotificationType type = notification->NotificationType();
    overflowTest.Printf(_L("Overflow - First Notification Type = %d\n"),type);
    
    //Signal the test thread (W2)
    package.iBarrier.Signal();
    //Wait for Signal to continue (W3);
    package.iBarrier.Wait();
    
    notify->RequestNotifications(status);
    User::WaitForRequest(status);
    
    TInt handlePostOverflow = 0;
    TInt count = 0;
    overflowTest.Next(_L("Overflow- Get the rest of the notifications"));
    notification = notify->NextNotification();
    while(notification != NULL)
        {
        
        type = notification->NotificationType();
        overflowTest.Printf(_L("Overflow - NotificationType = %d\n"),type);
        if(type & TFsNotification::EOverflow)
            {
            overflowTest.Printf(_L("Overflow +- Count = %d\n"),count);
            if(handlePostOverflow)
                {
                count = HandlePostOverflow(package,notify);
                }
            delete notify;
            fs.Close();
            overflowTest.End();
            overflowTest.Close();
            return count;
            }
        notification = notify->NextNotification();
        count++;
        
        if(count==5)
            handlePostOverflow = 1;
        }
    overflowTest.Printf(_L("Overflow -- Count = %d\n"),count);
    
    overflowTest.End();
    overflowTest.Close();
    delete notify;
    delete cleanup;
    fs.Close();
    return -1;
    }


TInt TestPostOverflowNotifications()
    {
    test.Next(_L("TestPostOverflowNotifications"));
    RFs fs;
    TInt r = fs.Connect();
    test_KErrNone(r);
    _LIT(KFileName,"post.over");
    SThreadPackage doerPkg;
    doerPkg.iFileName = KFileName;
    
    SThreadPackage watcher1Pkg;
    watcher1Pkg.iFileName = KFileName;
        
    User::LeaveIfError(doerPkg.iBarrier.CreateLocal(0));
    User::LeaveIfError(watcher1Pkg.iBarrier.CreateLocal(0));
    RThread watcher1;
    RThread doer;
    watcher1.Create(_L("TestPostOverflowWatcher1Thread"),TestPostOverflowWatcher1TF,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&watcher1Pkg);
    doer.Create(_L("TestPostOverflowDoerThread"),TestPostOverflowDoerTF,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&doerPkg);
    watcher1.Resume();

    //Wait until Request has been requested. (W1)
    watcher1Pkg.iBarrier.Wait();
    
    doer.Resume();
    
    //Wait till watcher has received first notification (W2)
    watcher1Pkg.iBarrier.Wait(); 

    //Signal the doer that it is free to continue
    //doing the rest of the operations (D-W-1)
    doerPkg.iBarrier.Signal();
    
    test.Next(_L("TestOverflow - Wait for doer thread death"));
    TRequestStatus status;
    doer.Logon(status);
    User::WaitForRequest(status);
    test(doer.ExitReason()==KErrNone);
    CLOSE_AND_WAIT(doer);
    
    //Wait until doer has finished doing notifications
    //thus the watcher should receive an overflow
    // (W3)
    watcher1Pkg.iBarrier.Signal();
    
    
    //wait for the watcher to have processed the first overflow
    //and to have requested notification.
    //Then we will perform some actions here
    // The watcher will wait on the semaphore until we are doing
    // doing all the operations we want to do
    // then it should process next notification
    watcher1Pkg.iBarrier.Wait(); //W4

    TBuf<45> path;
    path.Append((TChar)gDriveToTest);
    path.Append(_L(":\\F32-TST\\T_NOTIFIER\\"));
    path.Append(watcher1Pkg.iFileName);
    RFile file;
    r = file.Open(fs,path,EFileWrite);
    test_KErrNone(r);
    
    r = file.SetSize(1);
    test_KErrNone(r);
    r = file.SetSize(2);
    test_KErrNone(r);
    r = file.SetSize(3);
    test_KErrNone(r);
    file.Close();
    
    watcher1Pkg.iBarrier.Signal(); // Signal post operations complete (Sx)
    
    RTimer tim;
    r = tim.CreateLocal();
    test_KErrNone(r);
    TRequestStatus timStatus;
    
    test.Next(_L("TestOverflow - Wait for watcher1 thread death"));
    TTimeIntervalMicroSeconds32 interval = 10000000; //10 seconds
    tim.After(timStatus,interval);
    watcher1.Logon(status);
    User::WaitForRequest(status,timStatus);
    test(status != KRequestPending);
    /*
     * The number of notifications returned here should be 3.
     * This is because :
     * 
     * The first notification means that the buffer has lost 80 (the size of this
     * particular notification). Even though the client has read it becase they've not called
     * RequestNotification the server doesn't know that yet so that's why it's 5 not 6.
     * 
     * That leaves 556 - 80. Which means only 5 notifications will fit.
     * 
     * Then overflow occurs.
     * 
     * Then count is reset and 3 more operations are performed.
     */
    TInt count = watcher1.ExitReason();
    test(count==3);
    
    CLOSE_AND_WAIT(watcher1);
    watcher1Pkg.iBarrier.Close();
    doerPkg.iBarrier.Close();
    fs.Close();
    tim.Close();
    return KErrNone;    
    }

/*
 * Call AddNotification with a file without a path nor drive
 */
void TestNonDriveFilters()
	{
	test.Next(_L("TestNonDriveFilters"));
	RFs fs;
	TInt r = fs.Connect();
	test_KErrNone(r);
	
	TDriveList drives;
	r = fs.DriveList(drives);
	test_KErrNone(r);
	
	CFsNotify* notify = NULL;
	TRAP(r,notify= CFsNotify::NewL(fs,KMinNotificationBufferSize));
	
	TBuf<20> testfile;
	testfile.Append(_L("test.file"));
	
	r = notify->AddNotification((TUint)TFsNotification::ECreate,_L(""),testfile);
	test_KErrNone(r);
	
	TRequestStatus status;
	r = notify->RequestNotifications(status);
	test_KErrNone(r);
	
	TBuf<40> path;
	path.Append((TChar)gDriveToTest);
	path.Append(_L(":\\F32-TST\\T_NOTIFIER\\"));
	
	TBuf<40> fullname;
	fullname.Append(path);
	fullname.Append(testfile);
	
	RFile file;
	r = fs.MkDirAll(path);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	r = file.Replace(fs,fullname,EFileWrite);
	test_KErrNone(r);
	file.Close();
	
	fs.Delete(fullname);
	
	TChar testDrive = (TChar)gDriveToTest;
	testDrive.UpperCase();

	//Also create the file on another drive;
	for(TInt i = 0; i < KMaxDrives; i++)
		{
		TChar drive = drives[i];
		if(drive == testDrive)
			continue;
		
		if(drive)
			{
			TText16 drive16 = (TText16)(i+(TChar)'A');
			fullname.operator [](0) = drive16;
			break;
			}
		}
	
	r = fs.MkDirAll(fullname);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	r = file.Replace(fs,fullname,EFileWrite);
	test_KErrNone(r);
	file.Close();
	
	RTimer timer1;
	r = timer1.CreateLocal();
	test_KErrNone(r);
	TRequestStatus timeout;
	TTimeIntervalMicroSeconds32 time = 10000000;    //10 seconds
	timer1.After(timeout,time);
	User::WaitForRequest(timeout,status);
	test(status.Int() != KRequestPending);
	timer1.Cancel();
	timer1.Close();

	const TFsNotification* notification = notify->NextNotification();
	test(notification != NULL);
	TPtrC _path;
	r = notification->Path(_path);
	test_KErrNone(r);
	TChar driveletter = _path[0];
	driveletter.UpperCase();
	test(driveletter ==  (TChar)gDriveToTest);
	
	if(notification = notify->NextNotification(), notification==NULL)
		{
		TRequestStatus status2;
		r = notify->RequestNotifications(status2);
		test_KErrNone(r);
		
		RTimer timer2;
		r = timer2.CreateLocal();
		test_KErrNone(r);
		TRequestStatus timeout2;
		TTimeIntervalMicroSeconds32 time2 = 10000000;    //10 seconds
		timer2.After(timeout2,time2);
		User::WaitForRequest(timeout2,status2);
		test(status2.Int() != KRequestPending);
		timer2.Cancel();
		timer2.Close();
		
		notification = notify->NextNotification();
		}
	test(notification != NULL);
	r = notification->Path(_path);
	test_KErrNone(r);
	driveletter = _path[0];
	driveletter.UpperCase();
	test(driveletter == (TChar)'C');	
	
	delete notify;
	fs.Close();	
	}

// Negative testing for directory without *
// We receive no notifications for files changed under the directory
void NegativeTestDirStar()
	{
	RFs fs;
	TInt r = fs.Connect();
	test_KErrNone(r);
	
	CFsNotify* notify = NULL;
	TRAP(r,notify= CFsNotify::NewL(fs,KMinNotificationBufferSize));
	
	TBuf<40> path;
	path.Append((TChar)gDriveToTest);
	path.Append(_L(":\\F32-TST\\T_NOTIFIER\\"));
	r = fs.MkDirAll(path);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	
	r = notify->AddNotification((TUint)TFsNotification::ECreate,path,_L(""));
	test_KErrNone(r);
	
	TRequestStatus status;
	r = notify->RequestNotifications(status);
	test_KErrNone(r);
	
	TBuf<40> filename;
	filename.Append((TChar)gDriveToTest);
	filename.Append(_L(":\\F32-TST\\T_NOTIFIER\\dir.star"));
	
	RFile file;
	r = file.Replace(fs,filename,EFileWrite);
	test_KErrNone(r);
	file.Close();
	
	RTimer timer1;
	r = timer1.CreateLocal();
	test_KErrNone(r);
	TRequestStatus timeout;
	TTimeIntervalMicroSeconds32 time = 2000000;    //2 seconds
	timer1.After(timeout,time);
	User::WaitForRequest(timeout,status);
	test(status.Int() == KRequestPending);
	timer1.Cancel();
	timer1.Close();

	const TFsNotification* notification = notify->NextNotification();
	test(notification == NULL);
	
	delete notify;
	fs.Close();	
	}

/*
 * Negative Testing
 */
void NegativeTests()
	{
	test.Next(_L("Negative Tests"));
	//1
	test.Printf(_L("NegativeTests() A\n"));
	RFs fs;
	CFsNotify* notify = NULL;
	TInt r = fs.Connect();
	test_KErrNone(r);
	TRAP(r,notify = CFsNotify::NewL(fs,0));
	test(notify != NULL);
	delete notify;
	notify = NULL;
	
	//2
	test.Printf(_L("NegativeTests() B\n"));
	TRAP(r,notify = CFsNotify::NewL(fs,-1));
	test(notify != NULL);
	delete notify;
	notify = NULL;

	test.Printf(_L("NegativeTests() C\n"));
	TRAP(r,notify = CFsNotify::NewL(fs,KMaxTInt));
	test_Value(r, r == KErrArgument);
	test(notify==NULL);
	
	//3
	test.Printf(_L("NegativeTests() D\n"));
	TBuf<40> path;
	path.Append((TChar)gDriveToTest);
	path.Append(_L(":\\F32-TST\\T_NOTIFIER\\"));
	TBuf<20> filename;
	filename.Append(_L("file.txt"));
	TRAP(r,notify = CFsNotify::NewL(fs,KMinNotificationBufferSize));
	test_KErrNone(r);
	test(notify!=NULL);
	r = notify->AddNotification(0,path,filename);
	test_Value(r, r == KErrArgument);
	
	test.Printf(_L("NegativeTests() E\n"));
	r = notify->AddNotification((TUint)0x8000,path,filename); //invalid value
	test_Value(r, r == KErrArgument);
	
	test.Printf(_L("NegativeTests() F\n"));
	TBuf<40> invalidPath;
	invalidPath.Append(_L("1:\\*"));
	r = notify->AddNotification((TUint)TFsNotification::ECreate,invalidPath,filename);
	test_Value(r, r == KErrNotFound || r == KErrPathNotFound);
	
	//4
	test.Printf(_L("NegativeTests() G\n"));
	TRequestStatus wrongStatus;
	wrongStatus = KRequestPending;
	r = notify->RequestNotifications(wrongStatus);
	test_Value(r, r == KErrInUse);
	
	test.Printf(_L("NegativeTests() H\n"));
	TRequestStatus status;
	r = notify->RequestNotifications(status);
	test_KErrNone(r);
	r = notify->CancelNotifications(wrongStatus);
	test_Value(r, r == KErrInUse);
	
	delete notify;
	notify = NULL;
	fs.Close();
	}


/*
 * RPlugin devired.
 * Doesn't actually do anything special.
 * Can probably be deleted.
 */
class MyRPlugin : public RPlugin
	{
public:
	void DoRequest(TInt aReqNo,TRequestStatus& aStatus) const;
	void DoRequest(TInt aReqNo,TRequestStatus& aStatus,TDes8& a1) const;
	void DoRequest(TInt aReqNo,TRequestStatus& aStatus,TDes8& a1,TDes8& a2) const;
	TInt DoControl(TInt aFunction) const;
	TInt DoControl(TInt aFunction,TDes8& a1) const;
	TInt DoControl(TInt aFunction,TDes8& a1,TDes8& a2) const;
	void DoCancel(TUint aReqMask) const;
	};

void MyRPlugin::DoRequest(TInt aReqNo,TRequestStatus& aStatus) const
	{
	RPlugin::DoRequest(aReqNo,aStatus);
	}
void MyRPlugin::DoRequest(TInt aReqNo,TRequestStatus& aStatus,TDes8& a1) const
	{
	RPlugin::DoRequest(aReqNo,aStatus,a1);
	}
void MyRPlugin::DoRequest(TInt aReqNo,TRequestStatus& aStatus,TDes8& a1,TDes8& a2) const
	{
	RPlugin::DoRequest(aReqNo,aStatus,a1,a2);
	}
TInt MyRPlugin::DoControl(TInt aFunction) const
	{
	return RPlugin::DoControl(aFunction);
	}
TInt MyRPlugin::DoControl(TInt aFunction,TDes8& a1) const
	{
	return RPlugin::DoControl(aFunction,a1);
	}
TInt MyRPlugin::DoControl(TInt aFunction,TDes8& a1,TDes8& a2) const
	{
	return RPlugin::DoControl(aFunction,a1,a2);
	}
void MyRPlugin::DoCancel(TUint aReqMask) const
	{
	RPlugin::DoCancel(aReqMask);
	}

/*
 * This tests that when file server plugins perform operations that
 * the framework doesn't notify about them
 */
TInt TestNotificationsWithFServPlugins()
	{
	TInt r = TheFs.AddPlugin(KNotifyPluginFileName);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	r = TheFs.MountPlugin(KNotifyPluginName,(TUint)gDriveToTest.GetUpperCase() - 65);
	if (r == KErrNotSupported)
		{
		test.Printf(_L("Plugins are not supported on pagable drives.\nSkipping test.\n"));
		safe_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
		}
	
	MyRPlugin rplugin;
	TPckgBuf<TChar> drivePckg(gDriveToTest);
	
	test.Next(_L("Open RPlugin connection for NotifyPlugin"));
	r = rplugin.Open(TheFs,KNotifyPos);
	safe_test(test,r,__LINE__,(TText*)Expand("t_notify_plugin.cpp"));

	test.Next(_L("Send drive letter to test down to plugin"));
	r = rplugin.DoControl(KPluginSetDrive,drivePckg);
	safe_test(test,r,__LINE__,(TText*)Expand("t_notify_plugin.cpp"));
	rplugin.Close();
	
	r = SimpleCreateTestL();
	safe_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
	
	DismountPlugin();
	return KErrNone;
	}

_LIT(KPhantomExtendedReplace,"?:\\PhantomExtended_Replaced.txt");
_LIT(KPhantomExtendedRenamed,"?:\\PhantomExtended_Renamed.txt");
_LIT(KPhantomExtendedRenameMe,"?:\\PhantomExtended_RenameMe.txt");

TInt DoTestExternalNotificationL()
    {
    TRequestStatus statusN, statusT;
    RTimer timer1;
    TTimeIntervalMicroSeconds32 time = 10000000;    
    
    test.Printf(_L("DoTestExternalNotification"));
    CFsNotify* notify = CFsNotify::NewL(TheFs,1024);
    TInt r = notify->AddNotification(TFsNotification::ECreate, _L("?:\\"),_L("PhantomExtended_Replaced.txt"));
    safe_external_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
    r = notify->RequestNotifications(statusN);
    safe_external_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
    
    RFile file;
    r = file.Replace(TheFs,_L("\\Extended_Replaced.txt"),EFileWrite);
    safe_external_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
    file.Close();    
    
    r = timer1.CreateLocal();
    safe_external_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
    timer1.After(statusT,time);
    User::WaitForRequest(statusN,statusT);
    test_Compare(statusN.Int(),!=,KRequestPending)
    timer1.Cancel();
    timer1.Close();
    User::WaitForRequest(statusT);
    
    const TFsNotification* notification = notify->NextNotification();
    if(!notification)
        safe_external_test(test,KErrUnderflow,__LINE__,(TText*)Expand("t_notifier.cpp"));
    
    //Check Path
    TPtrC path;
    r = notification->Path(path);
    safe_external_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
    
    if(path.Match(KPhantomExtendedReplace)==KErrNotFound)
        {
        safe_external_test(test,KErrNotFound,__LINE__,(TText*)Expand("t_notifier.cpp"));
        }
   
    //Check NewName
    TPtrC newName;
    r = notification->NewName(newName);
    safe_external_test(test,(r==KErrNotSupported) ? KErrNone : r,__LINE__,(TText*)Expand("t_notifier.cpp"));
    
    notify->CancelNotifications(statusN);
    delete notify;
    
    //*************************************************************
    // Rename:
    //*************************************************************
    
    notify = CFsNotify::NewL(TheFs,1024);
    r = notify->AddNotification(TFsNotification::ERename, _L("?:\\"),_L("PhantomExtended_Renamed.txt"));
    safe_external_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
    r = notify->RequestNotifications(statusN);
    safe_external_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));

    r = file.Replace(TheFs,_L("\\Extended_RenameMe.txt"),EFileWrite);
    safe_external_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
    file.Close();    

    r = timer1.CreateLocal();
    safe_external_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
    timer1.After(statusT,time);
    User::WaitForRequest(statusN,statusT);
    test_Compare(statusN.Int(),!=,KRequestPending)
    timer1.Cancel();
    timer1.Close();
    User::WaitForRequest(statusT);

    notification = notify->NextNotification();
    if(!notification)
        safe_external_test(test,KErrUnderflow,__LINE__,(TText*)Expand("t_notifier.cpp"));

    r = notification->Path(path);
    safe_external_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));

    if(path.Match(KPhantomExtendedRenameMe)==KErrNotFound)
        {
        safe_external_test(test,KErrNotFound,__LINE__,(TText*)Expand("t_notifier.cpp"));
        }
    
    //Check NewName
    r = notification->NewName(newName);
    if(newName.Match(KPhantomExtendedRenamed)==KErrNotFound)
        {
        safe_external_test(test,KErrNotFound,__LINE__,(TText*)Expand("t_notifier.cpp"));
        }

    notify->CancelNotifications(statusN);
    delete notify;
        
    return KErrNone;
    }

void TestExternalNotifications()
    {
    test.Printf(_L("Test External Notifications (Load test file system)"));

    if(F32_Test_Utils::Is_SimulatedSystemDrive(TheFs,globalDriveNum))
        {
        test.Printf(_L("Not testing External Notifications on SimulatedSystemDrive"));
        return;
        }

    TInt r = TheFs.FileSystemName(filesystem,globalDriveNum);
    safe_external_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
    r = TheFs.DismountFileSystem(filesystem,globalDriveNum);
    safe_external_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
    r = TheFs.AddFileSystem(KTestNotifyFileSystemExeName);
    safe_external_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
    r = TheFs.MountFileSystem(KNotifyTestFileSystem,globalDriveNum);
    safe_external_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
    
    TRAP(r,DoTestExternalNotificationL());
    safe_external_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
        
    //Replace old FS.
    r = TheFs.DismountFileSystem(KNotifyTestFileSystem,globalDriveNum);
    safe_external_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
    r = TheFs.MountFileSystem(filesystem,globalDriveNum);
    safe_external_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
    r = TheFs.RemoveFileSystem(KNotifyTestFileSystem);
    safe_external_test(test,r,__LINE__,(TText*)Expand("t_notifier.cpp"));
    }

/*
 * This test is testing the use cases
 * and for negative testing of SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION
 * 
 * Performance tests can be found in test t_notify_perf
 */
void CallTestsL()
	{
	CTrapCleanup* cleanup;
	cleanup = CTrapCleanup::New();
	
	globalDriveNum = gDriveToTest - (TChar)'A';
	
	PrintLine();
	test.Start(_L("T_NOTIFIER Test Start"));
	TInt r = KErrNone;
	
	//=========================================================================================
	//! @SYMTestCaseID			PBASE-T_NOTIFY-2443
	//! @SYMTestType 			CIT
	//! @SYMREQ 				PREQ1847
	//! @SYMTestCaseDesc 		Simple Tests/User Heap Tests
	//! @SYMTestStatus          Implemented
	//=========================================================================================
	//
	// 1.	Create and delete many CFsNotify objects
	//
	PrintLine();
	test.Next(_L("CFsNotify Creation and Delete Tests"));
	//Creates and Deletes 1 CFsNotify
	__UHEAP_MARK;
	r = TestNewDeleteCFsNotify(1);
	__UHEAP_MARKEND;
	test_KErrNone(r);	
	//Creates and Deletes 50 CFsNotifys
	__UHEAP_MARK;
	r = TestNewDeleteCFsNotify(50);
	__UHEAP_MARKEND;
	test_KErrNone(r);
	test.Printf(_L("------- End of User Heap Tests ---------------------------------------\n"));
	//
	// 2.	Add notification for creating a file
	//		Create that file
	//
	PrintLine();
	__UHEAP_MARK;
	r = SimpleCreateTestL();
	__UHEAP_MARKEND;
	test_KErrNone(r);
	test.Printf(_L("------- End of CFsNotify Creation and Delete Tests -------------------\n"));
	//
	// 3.	Add notification at the root of a drive
	//		Create a file in that drive
	//
	PrintLine();
	TestRootDriveNotifications();
	test.Printf(_L("------- End of RootDriveNotifications Test ---------------------------\n"));
	//
	// 4.	Add notification for a filename without a drive
	//		Create that file in the current drive
	//		Create that file in another drive
	//
	PrintLine();
	TestNonDriveFilters();
	test.Printf(_L("------- End of TestNonDriveFilters Test ------------------------------\n"));
	//
	// 5.	Add notifications for 2 file creations
	//		Create 2 clients
	//		The clients create a file each
	//
	PrintLine();
	__UHEAP_MARK;
	r = TestTwoDoersL();
	__UHEAP_MARKEND;
	test_KErrNone(r);
	test.Printf(_L("------- End of TwoDoers Test -----------------------------------------\n"));
	//
	// 6.	Create 2 file server sessions
	//		Add a notification on each session for the same specific file creation
	//		Create that file
	//
	PrintLine();
	__UHEAP_MARK;
	r = TestTwoWatchersL();
	__UHEAP_MARKEND;
	test_KErrNone(r);
	test.Printf(_L("------- End of TwoWatchers Test --------------------------------------\n"));
	//
	// 7.	Create 2 file server sessions and 2 clients
	//		Add a notification on each session for different file creations
	//		Clients create a file each
	//
	PrintLine();
	__UHEAP_MARK;
	r = TestTwoWatchersTwoDoersL();
	__UHEAP_MARKEND;
	test_KErrNone(r);
	test.Printf(_L("------- End of TwoWatchersTwoDoers Test ------------------------------\n"));
	//
	// 8.	Add notification for a specific file creation
	//		Cancel the notification request
	//		Create that file
	//
	PrintLine();
	__UHEAP_MARK;
	r =  TestCancelNotificationL();
	__UHEAP_MARKEND;
	test_KErrNone(r);
	test.Printf(_L("------- End of CancelNotification Test -------------------------------\n"));
	//
	// 9.	Create 2 file server sessions
	//		Add a notification on each session for the same specific file creation
	//		Delete the first notification
	//		Create that file
	//
	PrintLine();
	test.Next(_L("TestClientRemoval"));
	__UHEAP_MARK;
	r = TestClientRemovalL();
	__UHEAP_MARKEND;
	test_KErrNone(r);
	test.Printf(_L("------- End of TestClientRemoval Test --------------------------------\n"));
	//
	// 10.	Create a CFsNotify object
	//		Close the session before closing the subsession
	//		Add notification and request notifications
	//
	PrintLine();
	__UHEAP_MARK;
	// Close session after creating the object
	TestSessionClose(1);
	__UHEAP_MARKEND;
	__UHEAP_MARK;
	// Close session after adding the notification
	TestSessionClose(2);
	__UHEAP_MARKEND;
	test.Printf(_L("------- End of TestSessionClose Test ---------------------------------\n"));
		

	//=========================================================================================
	//! @SYMTestCaseID			PBASE-T_NOTIFY-2444
	//! @SYMTestType 			UT
	//! @SYMREQ 				PREQ1847
	//! @SYMTestCaseDesc 		File/Directory Create and Replace – Single File Server Session
	//! @SYMTestStatus          Implemented
	//!
	//! TFsNotificationType		ECreate
    //=========================================================================================
	//
	// RFile::Create
	// 1.  Add notification for a specific file creation
	//     Create that file
	//
	PrintLine();
	test.Next(_L("EFileCreate Tests"));
	_LIT(KFilename3,"file.create");
	r = TestMultipleNotificationsL(_L(""),KFilename3,5,5,t_notification::EFileCreate,TFsNotification::ECreate,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	test.Printf(_L("------- End of EFileCreate Tests -------------------------------------\n"));
	//
	// RFs::MkDir
	// 2.  Add notification for a specific directory creation 
	//     Create that directory
	//
	PrintLine();
	test.Next(_L("EFsMkDir Test"));
	_LIT(KDirName1,"dirCreate\\");
	r = TestMultipleNotificationsL(KDirName1,_L(""),1,1,t_notification::EFsMkDir,TFsNotification::ECreate,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	test.Printf(_L("------- End of EFsMkDir Test -----------------------------------------\n"));
	//
	// RFile::Replace
	// 3.  Add notification for a specific file creation
	//     Replace that file
	//
	PrintLine();
	test.Next(_L("EFileReplace Test"));
	r = TestMultipleNotificationsL(_L(""),KFilename3,1,1,t_notification::EFileReplace,TFsNotification::ECreate,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	test.Printf(_L("------- End of EFileReplace Test -------------------------------------\n"));
	//
	// 4.  Add notification for a specific file creation
	//     Remove that notification
	//     Create that file
	//
	PrintLine();
	__UHEAP_MARK;
	r =  TestAddRemoveNotificationL();
	__UHEAP_MARKEND;
	test_KErrNone(r);
	test.Printf(_L("------- End of Add and Remove Notification Test ----------------------\n"));
	//
	// Wildcard Create Tests
	// 5.  Add notification for file creation using wildcard name
	//     Add notification for file/directory wildcard including subdirectories 
	//     Create number of files and directories that match each notification
	//
	PrintLine();
	test.Next(_L("Wildcard Create Tests"));
	//
	// Wildcard Name
	_LIT(KWildcardName1,"*");
	r = TestMultipleNotificationsL(_L(""),KWildcardName1,1,1,t_notification::EFileCreate,TFsNotification::ECreate,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	r = TestMultipleNotificationsL(KWildcardName1,KWildcardName1,1,1,t_notification::EFileCreate_subs,TFsNotification::ECreate,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	r = TestMultipleNotificationsL(KWildcardName1,KWildcardName1,1,1,t_notification::EFileCreate_subs_nowatch,TFsNotification::ECreate,KMinNotificationBufferSize,(TBool)ETrue,__LINE__);
	test_KErrNone(r);
	//
	// Wildcard including Subdirectories
	_LIT(KWildcardName2,"*\\");
	r = TestMultipleNotificationsL(KWildcardName2,KWildcardName1,1,1,t_notification::EFileCreate,TFsNotification::ECreate,KMinNotificationBufferSize,(TBool)ETrue,__LINE__);
	test_KErrNone(r);
	r = TestMultipleNotificationsL(KWildcardName2,KWildcardName1,1,1,t_notification::EFileCreate_subs_nowatch,TFsNotification::ECreate,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	_LIT(KDirName2,"SubDir\\");
	_LIT(KWildcardName3,"?");
	r = TestMultipleNotificationsL(KDirName2,KWildcardName3,1,1,t_notification::EFileCreate_subs_nowatch,TFsNotification::ECreate,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	//
	// Wildcard Type
	_LIT(KWildcardName4,"*.*");
	r = TestMultipleNotificationsL(_L(""),KWildcardName4,1,1,t_notification::EFileCreate_txt_nowatch,TFsNotification::ECreate,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	r = TestMultipleNotificationsL(_L(""),KWildcardName4,1,1,t_notification::EFileCreate_subs_nowatch,TFsNotification::ECreate,KMinNotificationBufferSize,(TBool)ETrue,__LINE__);
	test_KErrNone(r);
	//
	// 6.  Add notification for file creation for a specific type
	//     Create file with that type
	//     Create file with different type
	//
	_LIT(KWildcardName5,"*.txt");
	r = TestMultipleNotificationsL(_L(""),KWildcardName5,1,1,t_notification::EFileCreate_txt,TFsNotification::ECreate,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	test.Printf(_L("------- End of Wildcard Create Tests ---------------------------------\n"));
	
	
	//=============================================================================
	//! @SYMTestCaseID			PBASE-T_NOTIFY-2445
	//! @SYMTestType 			UT
	//! @SYMREQ 				PREQ1847
	//! @SYMTestCaseDesc 		File Attribute Change – Single File Server Session
	//! @SYMTestStatus          Implemented
	//
	//	TFsNotificationType		EAttribute
	//=============================================================================
	//
	// RFile::SetAtt, RFile::Set and RFs::SetEntry
	// 1.  Add notification for a specific file attribute change
	//     Change the attribute for that file
	//
	PrintLine();
	test.Next(_L("Attribute Tests"));
	_LIT(KFilename4,"file.setatts");
	r = TestMultipleNotificationsL(_L(""),KFilename4,1,1,t_notification::EFileSetAtt,TFsNotification::EAttribute,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	r = TestMultipleNotificationsL(_L(""),KFilename4,1,1,t_notification::EFileSet,TFsNotification::EAttribute,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	r = TestMultipleNotificationsL(_L(""),KFilename4,1,1,t_notification::EFsSetEntry,TFsNotification::EAttribute,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	//
	// Wildcard Attribute Test including subdirectories
	// 2.  Add notification for file attribute change using wildcard name
	//     Create number of files that match the notification
	//     Change attributes of some files
	//
	r = TestMultipleNotificationsL(KWildcardName2,_L("*"),3,3,t_notification::EFileSetAtt_subs,TFsNotification::EAttribute,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	test.Printf(_L("------- End of Attribute Tests ---------------------------------------\n"));
	
	
	//=============================================================================
	//! @SYMTestCaseID			PBASE-T_NOTIFY-2446
	//! @SYMTestType 			UT
	//! @SYMREQ 				PREQ1847
	//! @SYMTestCaseDesc 		File/Directory Rename – Single File Server Session
	//! @SYMTestStatus          Implemented
	//
	//	TFsNotificationType		ERename
	//=============================================================================
	//
	// RFs::Replace, RFs::Rename and RFile::Rename
	// 1.  Add notification for a specific file rename change
	//     Rename that file
	// 
	PrintLine();
	test.Next(_L("Rename Tests"));
	_LIT(KFilename5,"file.rename");
	r = TestMultipleNotificationsL(_L(""),KFilename5,1,1,t_notification::EFsReplace,TFsNotification::ERename,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	r = TestMultipleNotificationsL(_L(""),KFilename5,1,1,t_notification::EFsRename,TFsNotification::ERename,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	r = TestMultipleNotificationsL(_L(""),KFilename5,1,1,t_notification::EFileRename,TFsNotification::ERename,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	//
	// 2.  Add notification for a specific directory rename
	//     Rename that directory
	//
	_LIT(KDirName3,"dirRename\\");
	r = TestMultipleNotificationsL(KDirName3,_L(""),1,1,t_notification::EFsRename_dir,TFsNotification::ERename,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	//
	// 3.  Add notification for file rename using wildcard name
	//     Create file that match the notification
	//     Repeatedly rename the file
	//
	r = TestMultipleNotificationsL(_L(""),KWildcardName1,3,3,t_notification::EFileRename_wild,TFsNotification::ERename,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	test.Printf(_L("------- End of Rename Tests ------------------------------------------\n"));
	
	
	//=============================================================================
	//! @SYMTestCaseID			PBASE-T_NOTIFY-2447
	//! @SYMTestType 			UT
	//! @SYMREQ 				PREQ1847
	//! @SYMTestCaseDesc 		File/Directory Delete – Single File Server Session
	//! @SYMTestStatus          Implemented
	//
	//	TFsNotificationType		EDelete
	//=============================================================================
	//
	// RFs::Delete
	// 1.  Add notification for a specific file delete
	//     Delete that file
	//
	PrintLine();
	test.Next(_L("EFsDelete Test"));
	_LIT(KFilename6,"file.delete");
	r = TestMultipleNotificationsL(_L(""),KFilename6,1,1,t_notification::EFsDelete,TFsNotification::EDelete,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);	
	//
	// RFs::RmDir
	// 2.  Add notification for a specific directory delete
	//     Delete that directory
	//
	PrintLine();
	test.Next(_L("EFsRmDir Tests"));
	_LIT(KDirName4,"dirRemove\\");
	r = TestMultipleNotificationsL(KDirName4,_L(""),1,1,t_notification::EFsRmDir,TFsNotification::EDelete,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
    //
    // This test should not receive any notifications because a non-empty directory cannot be removed
    // 3.  Add notification for specific directory delete
    //     Create files inside that directory
    //     Delete the directory
    //
    _LIT(KDirName5,"dirRmNonEmp\\");
    r = TestMultipleNotificationsL(KDirName5,_L(""),1,1,t_notification::EFsRmDir_nonEmpty,TFsNotification::EDelete,KMinNotificationBufferSize,(TBool)ETrue,__LINE__);
    test_KErrNone(r);	
    //
    // Wildcard Name ("*")
    // 4.  Add notification for directory delete using wildcard name
    //     Create directory that match the notification
	//	   Delete that directory
    //
    r = TestMultipleNotificationsL(KWildcardName1,_L(""),1,1,t_notification::EFsRmDir_wild,TFsNotification::EDelete,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
    test_KErrNone(r);	
    //
    // Wildcard Type ("*.txt")
    // Creates files with different types and should only receive notifications from "*.txt" file deletions
    // 5.  Add notification for file deletes using wildcard type
    //     Create number of files that match the notification
    //     Delete those files
    //
    r = TestMultipleNotificationsL(_L(""),KWildcardName4,3,3,t_notification::EFsDelete,TFsNotification::EDelete,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
    test_KErrNone(r);
    test.Printf(_L("------- End of Delete Tests ------------------------------------------\n"));
	
	
	//======================================================================
	//! @SYMTestCaseID			PBASE-T_NOTIFY-2448
	//! @SYMTestType 			UT
	//! @SYMREQ 				PREQ1847
	//! @SYMTestCaseDesc 		File Change – Single File Server Session
    //! @SYMTestStatus          Implemented
    //
	//	TFsNotificationType		EFileChange
	//======================================================================
	//
	// File Write
	// If caching is enabled, notifications are received only when the file cache is flushed
	// We flush everytime we do a write to ensure the tests work regardless of cache
    //
    // 1.   Add notification for a specific file change
    //      Create the file
    //      Write to that file
	//
	PrintLine();
	test.Next(_L("EFileWrite Tests"));
	_LIT(KFilename7,"file.write");
	__UHEAP_MARK;
	r = TestMultipleNotificationsL(_L(""),KFilename7,7,7,t_notification::EFileWrite,TFsNotification::EFileChange,3*KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	__UHEAP_MARKEND;
	test_KErrNone(r);
	//
	// 2.  Add notification for a specific file change
	//     Write to the specified file a number of times without changing its size
	//
	// Four letters are written to a file, then the first letter in the file is replaced aIterations times
	//     aMaxNotifications = 1 + aIterations
	//
	r = TestMultipleNotificationsL(_L(""),KFilename7,3,4,t_notification::EFileWrite_samesize,TFsNotification::EFileChange,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	test.Printf(_L("------- End of EFileWrite Tests --------------------------------------\n"));
	//
	// 3.  Add notification for a specific file change
	//     Write to that file asynchronously
	//
	PrintLine();
	test.Next(_L("EFileWrite_async Tests"));
	_LIT(KFilename8,"async.write");
	__UHEAP_MARK;
	r = TestMultipleNotificationsL(_L(""),KFilename8,4,4,t_notification::EFileWrite_async,TFsNotification::EFileChange,2*KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	__UHEAP_MARKEND;
	test_KErrNone(r);
	test.Printf(_L("------- End of EFileWrite_async Tests --------------------------------\n"));	
	//
	// File Set Size
	// 4.  Add notification for a specific file change
	//     Both increase and decrease the file sizes a number of times
	//
	// The file size is increased aIterations times, and decreased (aIterations - 1) times
	//     aMaxNotifications = 2*aIterations - 1
	//
	PrintLine();
	test.Next(_L("EFileSetSize Tests"));
	_LIT(KFilename9,"file.setsize");
	r = TestMultipleNotificationsL(_L(""),KFilename9,5,9,t_notification::EFileSetSize,TFsNotification::EFileChange,3*KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	test.Printf(_L("------- End of EFileSetSize Tests ------------------------------------\n"));


	//
    PrintLine();
	test.Next(_L("CFileMan Tests"));
	_LIT(KFilenameCFMan,"cf1le.man");
	TUint notificationTypes = (TUint)TFsNotification::ECreate|TFsNotification::EFileChange|TFsNotification::EAttribute|TFsNotification::EDelete|TFsNotification::ERename;
	r = TestMultipleNotificationsL(_L(""),KFilenameCFMan,1,5,t_notification::ECFileManMove,notificationTypes,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	test.Printf(_L("------- End of CFileMan Tests -------------------------------------\n"));
	
	
	//========================================================================================
	//! @SYMTestCaseID			PBASE-T_NOTIFY-2449
	//! @SYMTestType 			UT
	//! @SYMREQ 				PREQ1847
	//! @SYMTestCaseDesc 		File System Mounted/Dismounted, Media Card Removal/Insertion,
	//                          RawDisk Write – Single File Server Session
	//! @SYMTestStatus          Implemented
	//
	//	TFsNotificationType		EMediaChange
	//========================================================================================
	//
	// RFs::DismountFileSystem
	// 1.  Add notification for media change
	//     Dismount the file system
	//
	PrintLine();
	test.Next(_L("Mount Tests"));
	TFullName filesystemName;
	r = TheFs.FileSystemName(filesystemName,globalDriveNum);
	test_KErrNone(r);
	r = TestMultipleNotificationsL(filesystemName,1,1,t_notification::EDismount,TFsNotification::EMediaChange,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	//
	// RFs::MountFileSystem
	// 2.  Add notification for media change
	//     Mount the file system
	//
	r = TestMultipleNotificationsL(filesystemName,1,1,t_notification::EMount,TFsNotification::EMediaChange,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	//
	// Repeatedly mount and dismount the file system
	// 3.  Add notification for media change
	//     Repeatedly dismount and mount the file system
	//
	// The file system is dismounted and mounted aIterations times
	//     aMaxNotifications = 2*aIterations
	//
	r = TestMultipleNotificationsL(filesystemName,5,10,t_notification::EMountDismount,TFsNotification::EMediaChange,3*KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	//
	// RFs::MountFileSystemAndScan
	// 4.  Add notification for media change
	//     Mount and scan the file system
	//
	// The file system is dismounted and mounted aIterations times
	//     aMaxNotifications = 2*aIterations
	//
//#ifndef __WINS__
//    r = TestMultipleNotificationsL(filesystemName,1,2,t_notification::EMountScan,TFsNotification::EMediaChange,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
//    test_KErrNone(r);
//#endif
	test.Printf(_L("------- End of Mount Tests -------------------------------------------\n"));
	TDriveInfo drvInfo;
	TInt driveNum;
	TheFs.CharToDrive(gDriveToTest,driveNum);
	r = TheFs.Drive(drvInfo,driveNum);
	test_KErrNone(r);
	TPtrC driveDes((TText*)&gDriveToTest,1);
	//
	// Manual Tests - Will only run on removable drives
	//
/*	if(drvInfo.iDriveAtt & KDriveAttRemovable)
		{
		//
		// 5. Add notification for media change
		//    Remove media card manually
		//
		PrintLine();
		test.Next(_L("Media Card Removal/Insertion Tests"));
		r = TestMultipleNotificationsL(driveDes,1,1,t_notification::EMediaCardRemoval,TFsNotification::EMediaChange,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
		test_KErrNone(r);
		//
		// 6. Add notification for media change
		//    Insert media card manually
		//
		r = TestMultipleNotificationsL(driveDes,1,1,t_notification::EMediaCardInsertion,TFsNotification::EMediaChange,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
		test_KErrNone(r);
		test.Printf(_L("------- End of Media Card Removal/Insertion Tests --------------------\n"));
		//
		// We should receive an EMediaChange notification even though we did not register for it
		// 7. Do not add notification for media change
		//    Remove and insert media card manually
		//  
		PrintLine();
		TestMediaCardNotificationWhenNotRegisteredForIt();
		test.Printf(_L("------- End of TestMediaCardNotificationWhenNotRegisteredForIt -------\n"));
		}
*/	//
	// RRawDisk::Write
	// 8.  Add notification for media change
	//     Write directly to the media
	//
#ifdef __WINS__
    if(gDriveToTest-(TChar)'A' != 2)
#endif
        {
        PrintLine();
        test.Next(_L("RRawDisk::Write Tests"));
        r = TestMultipleNotificationsL(driveDes,1,1,t_notification::ERawDiskWrite,TFsNotification::EMediaChange,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
        test_KErrNone(r);
        test.Printf(_L("------- End of RRawDisk::Write Test ------------------------------  \n"));
        }	
	
	
	//===============================================================================
	//! @SYMTestCaseID			PBASE-T_NOTIFY-2450
	//! @SYMTestType 			UT
	//! @SYMREQ 				PREQ1847
	//! @SYMTestCaseDesc 		Drive Name Modification – Single File Server Session
    //! @SYMTestStatus          Implemented
    //
	//	TFsNotificationType		EDriveName
	//===============================================================================
	//
	// RFs::SetDriveName
    // The drive name is renamed 2*aIterations times
    //     aMaxNotifications = 2*aIterations
    //
    // 1.   Add notification for a specific drive name change
    //      Change the drive name
	//
	PrintLine();
	test.Next(_L("DriveName Test"));
	r = TestMultipleNotificationsL(driveDes,1,2,t_notification::ESetDriveName,TFsNotification::EDriveName,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	//
	// 2.  Add notification for a specific drive name change
	//     Repeatedly rename the drive
	//
	r = TestMultipleNotificationsL(driveDes,3,6,t_notification::ESetDriveName,TFsNotification::EDriveName,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	test.Printf(_L("------- End of DriveName Test ----------------------------------------\n"));
	
	
	//================================================================================
	//! @SYMTestCaseID			PBASE-T_NOTIFY-2451
	//! @SYMTestType 			UT
	//! @SYMREQ 				PREQ1847
	//! @SYMTestCaseDesc 		Volume Name Modification – Single File Server Session
    //! @SYMTestStatus          Implemented
    //
	//	TFsNotificationType		EVolumeName
	//================================================================================
	//
	// RFs::SetVolumeLabel - Does not run on WINS
    // The volume name is renamed 2*aIterations times
    //     aMaxNotifications = 2*aIterations
	//
#ifndef __WINS__
	PrintLine();
	test.Next(_L("VolumeName Test"));
	//
	// 1.  Add notification for a specific volume name change
	//     Change the volume name
	//
	r = TestMultipleNotificationsL(driveDes,1,2,t_notification::ESetVolumeLabel,TFsNotification::EVolumeName,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	//
	// 2.  Add notification for a specific volume name change
	//     Repeatedly rename the volume
	//
	r = TestMultipleNotificationsL(driveDes,3,6,t_notification::ESetVolumeLabel,TFsNotification::EVolumeName,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	test.Printf(_L("------- End of VolumeName Test ---------------------------------------\n"));
#endif
		
	
    //=============================================================================
    //! @SYMTestCaseID          PBASE-T_NOTIFY-2452
    //! @SYMTestType            UT
    //! @SYMREQ                 PREQ1847
    //! @SYMTestCaseDesc        All Operations Filter – Single File Server Session
    //! @SYMTestStatus          Implemented
    //
    //  TFsNotificationType     EAllOps
	//=============================================================================
	PrintLine();
	test.Next(_L("AllOps Tests"));
	//
	// 1.	Add notification for all operations
	//		Create a file
	//		Delete the file
	//
	// EAllOps1: A file is created and deleted aIterations times
	// 		aMaxNotification = 2*aIterations
	//
	_LIT(KFilename10,"file.allops");
	r = TestMultipleNotificationsL(_L(""),KFilename10,4,8,t_notification::EAllOps1,(TUint)TFsNotification::EAllOps,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	//
	// 2.	Add notification for all operations
	//		Create a file
	//		Write to the file
	//		Delete the file
	//
	// EAllOps2: A file is created, written to aIterations times and then deleted
	// 		aMaxNotification = 2 + aIterations (See File Write Tests)
	//
	r = TestMultipleNotificationsL(_L(""),KFilename10,4,6,t_notification::EAllOps2,(TUint)TFsNotification::EAllOps,2*KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	//
	// 3.	Add notification for all operations
	//		Create a file
	//		Change the file size
	//		Delete the file
	//
	// EAllOps3: A file is created, its size is increased size aIterations times, decreased (aIterations - 1) times
	// 			 and then deleted
	//     aMaxNotifications = 1 + 2*aIterations
	//
	r = TestMultipleNotificationsL(_L(""),KFilename10,4,9,t_notification::EAllOps3,(TUint)TFsNotification::EAllOps,KMinNotificationBufferSize*2,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	//
	// 4.	Add notification for all operations
	//		Create a file
	//		Change the file attribute
	//		Delete the file
	//
	// EAllOps4: A file is created, its attribute is changed and the file is deleted
	// 		aMaxNotification = 3
	//
	r = TestMultipleNotificationsL(_L(""),KFilename10,1,3,t_notification::EAllOps4,(TUint)TFsNotification::EAllOps,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	//
	// 5.	Add notification for all operations
	//		Create a file
	//		Rename the file
	//
	// EAllOps5: A file is created and renamed
	// 		aMaxNotification = 2
	//
	r = TestMultipleNotificationsL(_L(""),KFilename10,1,2,t_notification::EAllOps5,(TUint)TFsNotification::EAllOps,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	//
	// 6.	Add notification for all operations
	//		Change drive name
	//		Change volume name
	//
	// SetVolumeLabel does not run on WINS
	// EAllOps6: The drive and volume names are changed
	//		aMaxNotification = 2
	//
#ifndef __WINS__
	r = TestMultipleNotificationsL(driveDes,1,2,t_notification::EAllOps6,(TUint)TFsNotification::EAllOps,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
#endif
	test.Printf(_L("------- End of AllOps Tests ------------------------------------------\n"));
	
	
    //=============================================================================
    //! @SYMTestCaseID          PBASE-T_NOTIFY-2453
    //! @SYMTestType            UT
    //! @SYMREQ                 PREQ1847
    //! @SYMTestCaseDesc        Multiple Filters – Single File Server Session
    //! @SYMTestStatus          Implemented
	//=============================================================================
	PrintLine();
	test.Next(_L("Multiple-Filter Tests"));
	//
	// TFsNotification::ECreate | TFsNotification::EDelete
	// 1.	Add notification for create and delete for a specific file
	//		Create that file
	//		Delete the file
	//
	// A file is created and deleted aIterations times
	// 		aMaxNotification = 2*aIterations
	//
	_LIT(KFilename11,"file.mulfil");
	r = TestMultipleNotificationsL(_L(""),KFilename11,3,6,t_notification::EAllOps1,TFsNotification::ECreate | TFsNotification::EDelete,2*KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	//
	// TFsNotification::EDelete | TFsNotification::ECreate | TFsNotification::EFileChange
	// 2.	Add notification for create, file change and delete for a specific file
	//		Create a file
	//		Change the file size
	//		Delete the file
	//
	// A file is created, its size is increased size aIterations times, decreased (aIterations - 1) times
	// and then deleted
	//     aMaxNotifications = 1 + 2*aIterations
	//
	r = TestMultipleNotificationsL(_L(""),KFilename11,4,9,t_notification::EAllOps3,TFsNotification::EDelete | TFsNotification::ECreate | TFsNotification::EFileChange,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	//
	// TFsNotification::EAttribute | TFsNotification::EDelete | TFsNotification::ECreate
	// 3.	Add notification for create, attribute change and delete for a specific file
	//		Create a file
	//		Change the file attribute
	//		Delete the file
	//
	// A file is created, its attribute is changed and the file is deleted
	// 		aMaxNotification = 3
	//
	r = TestMultipleNotificationsL(_L(""),KFilename11,1,3,t_notification::EAllOps4,TFsNotification::EAttribute | TFsNotification::EDelete | TFsNotification::ECreate,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	//
	// TFsNotification::ERename | TFsNotification::ECreate
	// 4.	Add notification for create and rename for a specific file
	//		Create a file
	//		Rename the file
	//
	// A file is created and renamed
	// 		aMaxNotification = 2
	//
	r = TestMultipleNotificationsL(_L(""),KFilename11,1,2,t_notification::EAllOps5,TFsNotification::ERename | TFsNotification::ECreate,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
	//
	// TFsNotification::EVolumeName | TFsNotification::EDriveName
	// 5.	Add notification for drive and volume name change for a specific drive
	//		Change drive name
	//		Change volume name
	//
	// SetVolumeLabel does not run on WINS
	// The drive and volume names are changed
	//		aMaxNotification = 2
	//
#ifndef __WINS__
	r = TestMultipleNotificationsL(driveDes,1,2,t_notification::EAllOps6,TFsNotification::EVolumeName | TFsNotification::EDriveName,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
	test_KErrNone(r);
#endif
	test.Printf(_L("------- End of Multiple-Filter Tests ---------------------------------\n"));
	
	
	//==============================================================================
	//! @SYMTestCaseID			PBASE-T_NOTIFY-2454
	//! @SYMTestType 			UT
	//! @SYMREQ 				PREQ1847
	//! @SYMTestCaseDesc 		Overflow Notification – Single File Server Session
    //! @SYMTestStatus          Implemented
	//==============================================================================
	//
	// 1.	Add notification with a small buffer size, for a specific file change
	//		Change the file size once
	// 2.	Make continuous file size changes to the file
	// 3.	When overflow notification occurs, delete the notification
	//
	PrintLine();
	r = TestOverflowL();
	test_KErrNone(r);
	
	//For DEF140387
	PrintLine();
	r= TestPostOverflowNotifications();
	test_KErrNone(r);
	test.Printf(_L("------- End of Overflow Test -----------------------------------------\n"));
	
	
	//============================================================================
	//! @SYMTestCaseID			PBASE-T_NOTIFY-2455
	//! @SYMTestType 			CIT
	//! @SYMREQ 				PREQ1847
	//! @SYMTestCaseDesc 		API Negative Testing – Single File Server Session
    //! @SYMTestStatus          Implemented
	//============================================================================
	// 1.
	//	a-CFsNotify class creation with zero buffer size
	//
	// 2.
	//	b-CFsNotify class creation with negative buffer size
	//	c-CFsNotify class creation with buffer size that is too large
	//
	// 3.
	//	d-Call AddNotification with aNotiififcationType zero
	//	e-Call AddNotification with aNotiififcationType invalid
	//	f-Call AddNotification with many different invalid paths
	//
	// 4.
	//	g-Call RequestNotifications with status that is already in use
	//	h-Call CancelNotifications with wrong status
	//
	PrintLine();
	__UHEAP_MARK;
	NegativeTests();
	__UHEAP_MARKEND;
	//
	// 5.
	//	i-Negative testing for directory without *
	//
	test.Printf(_L("NegativeTests() I\n"));
	NegativeTestDirStar();
	test.Printf(_L("------- End of Negative Tests ----------------------------------------\n"));
	
	
    //=============================================================================
    //! @SYMTestCaseID          PBASE-T_NOTIFY-2461
    //! @SYMTestType            CIT
    //! @SYMREQ                 PREQ1847
    //! @SYMTestCaseDesc        Plugin Tests
    //! @SYMTestStatus          
	//=============================================================================
	PrintLine();
	r = TestNotificationsWithFServPlugins();
	test_KErrNone(r);
	test.Printf(_L("------- End of Plugin Tests ------------------------------------------\n"));
		
	
	//======================================================================
	//! @SYMTestCaseID			PBASE-T_NOTIFY-2459
	//! @SYMTestType 			UT
	//! @SYMREQ 				PREQ1847
	//! @SYMTestCaseDesc 		Drive Formatting – Single File Server Session
	//! @SYMTestStatus          Implemented
	//
	//	TFsNotificationType		EMediaChange
	//======================================================================
	//
	// RFormat
	// We do these last so that we can be sure to have deleted anything we've inadvertently not deleted
	//
	// 1.  Add notification for media change of a specific drive
	//     Format the drive
	//
#ifdef __WINS__
	if(gDriveToTest-(TChar)'A' != 2)
#endif
		{
		PrintLine();
		test.Next(_L("Format Tests"));
		r = TestMultipleNotificationsL(driveDes,1,1,t_notification::EFormat,TFsNotification::EMediaChange,KMinNotificationBufferSize,(TBool)EFalse,__LINE__);
		test_KErrNone(r);
		test.Printf(_L("------- End of Format Tests ------------------------------------------\n"));
		}
	
	
	//======================================================================
	//! @SYMTestCaseID			PBASE-T_NOTIFY-2460
	//! @SYMTestType 			UT
	//! @SYMREQ 				PREQ1847
	//! @SYMTestCaseDesc 		Notifications for Data Caged Areas
	//! @SYMTestStatus          Implemented
	//======================================================================
	//
	// Create a private folder for a specified uid
	// Add notification filter using the following processes:
	//	1.	A process with no capability
	//	2.	A process with all capabilities
	//	3.	A process with the specified uid
	//
	PrintLine();
	test.Next(_L("Test T_NOTIFIER_NOCAPS.EXE"));
	r = TestProcessCapabilities(_L("T_NOTIFIER_NOCAPS.EXE"));
	test_Value(r, r == KErrPermissionDenied); //Failure on emulator -> Did you forget to do a wintest?
	
	test.Next(_L("Test T_NOTIFIER_ALLFILES.EXE"));
	r = TestProcessCapabilities(_L("T_NOTIFIER_ALLFILES.EXE"));
	test_KErrNone(r);
	
	test.Next(_L("Test T_NOTIFIER_BELONGS.EXE"));
	r = TestProcessCapabilities(_L("T_NOTIFIER_BELONGS.EXE"));
	test_KErrNone(r);
	test.Printf(_L("------- End of Data-Caging Tests -------------------------------------\n"));
	
	PrintLine();
	test.Next(_L("Test TestExternalNotifications()"));
	TestExternalNotifications();
	test.Printf(_L("------- End of TestExternalNotifications Tests -------------------------------------\n"));
	
	test.End();
	test.Close();
	delete cleanup;
	}	//End of CallTestsL

