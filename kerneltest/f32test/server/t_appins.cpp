// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_appins.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <hal.h>
#include "t_server.h"
#include "t_chlffs.h"

#if defined(__WINS__)
#define WIN32_LEAN_AND_MEAN
#pragma warning (disable:4201) // warning C4201: nonstandard extension used : nameless struct/union
#pragma warning (default:4201) // warning C4201: nonstandard extension used : nameless struct/union
#endif

#if defined(_UNICODE)
#if !defined(UNICODE)
#define UNICODE
#endif
#endif

GLDEF_D RTest test(_L("T_APPINS"));
LOCAL_D RFs TheNotifyFs;	//	Fileserver session to receive/request change notification
TInt RemovableDrive;
/*

	What this test is for:
	Tests bug fix for the bug which doesn't notify drive D: when files are installed onto it
 
*/

LOCAL_C void Test1()
//
//	Test notification when session path of notification session is set to be explicitly different 
//	from the drive and directory into which files and directories are created
//
	{
	TFileName path;
	path=_L("?:\\F32-TST\\T_APPINS\\");
		
	TInt r=TheNotifyFs.SessionPath(gSessionPath);
	test_KErrNone(r);

	TChar driveChar;
	r=RFs::DriveToChar(RemovableDrive,driveChar);
	test_KErrNone(r);
	
	if (gSessionPath[0]=='C')
		(gSessionPath[0] == (TText)gDriveToTest)? (path[0] = (TText)driveChar):(path[0] = (TText)gDriveToTest);
	else if (gSessionPath[0]=='Y')
		path[0]='X';
	else if (gSessionPath[0]=='X')
		path[0]='Y';
	else 
		path[0]='C';	//invalid drive numbers shouldn't reach here, must be filtered out from t_main
		//return;
	
	TRequestStatus statEntry(KRequestPending);
	TRequestStatus statFile(KRequestPending);
	TRequestStatus statDir(KRequestPending);
	TRequestStatus statWild(KRequestPending);
	TRequestStatus statWilder(KRequestPending);

	r=TheFs.RmDir(path);
	test_Value(r, (r == KErrNone)||(r==KErrNotFound)||(r==KErrPathNotFound));

	test.Printf(_L("Notify Session Path %S\n"),&gSessionPath);

//	Submit notify change requests (requesting ahead)
	test.Printf(_L("Create a directory %S\n"),&path);
	TheNotifyFs.NotifyChange(ENotifyEntry,statEntry,path);
	TheNotifyFs.NotifyChange(ENotifyFile,statFile,path);
	TheNotifyFs.NotifyChange(ENotifyDir,statDir,path);
	TheNotifyFs.NotifyChange(ENotifyEntry,statWild,_L("?:\\F32-TST\\T_APPINS\\"));
	TheNotifyFs.NotifyChange(ENotifyEntry,statWilder,_L("*:\\"));

	r=TheFs.MkDir(path);	
	test_Value(r, (r == KErrNone)||(r==KErrAlreadyExists));
	User::WaitForAnyRequest();
	test(statEntry==KErrNone);
	test(statFile==KErrNone);
	test(statDir==KErrNone);
	test(statWild==KErrNone);
	test(statWilder==KErrNone);
	
//	Resubmit notify change requests (requesting ahead)
	test.Next(_L("Create a file in the directory"));
	TheNotifyFs.NotifyChange(ENotifyEntry,statEntry,path);
	TheNotifyFs.NotifyChange(ENotifyFile,statFile,path);
	TheNotifyFs.NotifyChange(ENotifyDir,statDir,path);
	TheNotifyFs.NotifyChange(ENotifyEntry,statWild,_L("?:\\F32-TST\\T_APPINS\\"));
	TheNotifyFs.NotifyChange(ENotifyFile,statWilder,_L("*:\\"));

	TFileName filePath;
	filePath=path;
	filePath+=_L("TestFile.app");
	RFile file;
	r=file.Replace(TheFs,filePath,EFileRead|EFileWrite);
	file.Close();
	User::WaitForAnyRequest();
	test_KErrNone(r);
	test (statEntry==KErrNone);
	test(statFile==KErrNone);
	test(statDir==KRequestPending);
	test(statWild==KErrNone);
	test(statWilder==KErrNone);

//	Resubmit notify change requests	
	test.Next(_L("Remove the file from the directory"));
	TheNotifyFs.NotifyChange(ENotifyEntry,statEntry,path);
	TheNotifyFs.NotifyChange(ENotifyFile,statFile,path);
	TheNotifyFs.NotifyChange(ENotifyDir,statWild,_L("?:\\F32-TST\\T_APPINS\\"));
	TheNotifyFs.NotifyChange(ENotifyEntry,statWilder,_L("*:\\"));
	
	r=TheFs.Delete(filePath);
	test_KErrNone(r);
	User::WaitForAnyRequest();
	test (statEntry==KErrNone);
	test(statFile==KErrNone);
	test(statDir==KRequestPending);
	test(statWild==KRequestPending);
	test(statWilder==KErrNone);

//	Resubmit notify change requests	
	test.Next(_L("Remove the directory"));
	TheNotifyFs.NotifyChange(ENotifyEntry,statEntry,path);
	TheNotifyFs.NotifyChange(ENotifyFile,statFile,path);
//	TheNotifyFs.NotifyChange(ENotifyEntry,statWild,_L("?:\\F32-TST\\T_APPINS\\"));
	TheNotifyFs.NotifyChange(ENotifyFile,statWilder,_L("*:\\"));

	r=TheFs.RmDir(path);
	test_KErrNone(r);
	User::WaitForAnyRequest();
	test (statEntry==KErrNone);
	test(statFile==KRequestPending);
	test(statDir==KErrNone);
	test(statWild==KErrNone);
	test(statWilder==KRequestPending);

//	In case any outstanding notification requests exist
	TheNotifyFs.NotifyChangeCancel();
	}

LOCAL_C void Test2()
//
//	Test notification when session path of the notification session is implicitly different 
//	to the drive and directory into which files and directories are created
//
	{
	TFileName sessionPath;
	sessionPath=_L("?:\\F32-TST\\");
		
	TChar driveChar;
	TInt err=RFs::DriveToChar(RemovableDrive,driveChar);
	test_KErrNone(err);

	if (gSessionPath[0]=='C')
		(gSessionPath[0] == (TText)gDriveToTest)? (sessionPath[0] = (TText)driveChar):(sessionPath[0] = (TText)gDriveToTest);
	else if (gSessionPath[0]=='Y')
		sessionPath[0]='X';
	else if (gSessionPath[0]=='X')
		sessionPath[0]='Y';
	else 
		sessionPath[0]='C';	//invalid drive numbers shouldn't reach here, must be filtered out from t_main
		//return;
	
	TInt r=TheFs.SetSessionPath(sessionPath);
	test_KErrNone(r);

	TFileName path;
	path=_L("\\F32-TST\\T_APPINS\\");	//	Takes drive implicitly from associated session path
						
	r=TheFs.RmDir(path);
	test_Value(r, (r == KErrNone)||(r==KErrNotFound)||(r==KErrPathNotFound));
	
	TRequestStatus statEntry(KRequestPending);
	TRequestStatus statFile(KRequestPending);
	TRequestStatus statDir(KRequestPending);
	TRequestStatus statWild(KRequestPending);
	TRequestStatus statWilder(KRequestPending);

	test.Printf(_L("Notify Session Path %S\n"),&gSessionPath);
	test.Printf(_L("File Creation Session Path %S\n"),&sessionPath);

//	Submit notify change requests (requesting ahead)
	test.Printf(_L("Create a directory %S\n"),&path);

	TheNotifyFs.NotifyChange(ENotifyEntry,statEntry,path);	//	Watches drive associated with
	TheNotifyFs.NotifyChange(ENotifyFile,statFile,path);
	TheNotifyFs.NotifyChange(ENotifyDir,statDir,path);
	TheNotifyFs.NotifyChange(ENotifyEntry,statWild,_L("?:\\F32-TST\\T_APPINS\\"));
	TheNotifyFs.NotifyChange(ENotifyAll,statWilder,_L("*:\\"));

	r=TheFs.MkDir(path);										//	Creates the directory on the drive
	test_Value(r, (r == KErrNone)||(r==KErrAlreadyExists));				//	associated with TheFs session path
	test (statEntry==KRequestPending);
	test(statFile==KRequestPending);
	test(statDir==KRequestPending);	//	No notification because it's watching a different drive!
	test(statWild==KErrNone);	//	BUG FIX TEST
	test(statWilder==KErrNone);

//	Don't need to resubmit notify change requests
	test.Next(_L("Create a file in the directory"));
	TheNotifyFs.NotifyChange(ENotifyFile,statWild,_L("?:\\F32-TST\\T_APPINS\\"));
	TheNotifyFs.NotifyChange(ENotifyFile,statWilder,_L("*:\\"));

	TFileName filePath;
	filePath=path;
	filePath+=_L("TestFile.app");
	RFile file;
	r=file.Replace(TheFs,filePath,EFileRead|EFileWrite);
	file.Close();
	test_KErrNone(r);

	test (statEntry==KRequestPending);	//	No notification!
	test(statFile==KRequestPending);
	test(statDir==KRequestPending);	
	test(statWild==KErrNone);
	test(statWilder==KErrNone);

//	No need to resubmit notify change requests	
	TheNotifyFs.NotifyChange(ENotifyEntry,statWild,_L("?:\\F32-TST\\T_APPINS\\"));
	TheNotifyFs.NotifyChange(ENotifyDir,statWilder,_L("*:\\"));

	r=TheFs.Delete(filePath);
	test_KErrNone(r);
//	Still no notification	
	test (statEntry==KRequestPending);
	test(statFile==KRequestPending);
	test(statDir==KRequestPending);
	test(statWild==KErrNone);
	test(statWilder==KRequestPending);

//	No need to resubmit notify change requests	
	test.Next(_L("Remove the directory"));
	TheNotifyFs.NotifyChange(ENotifyDir,statWild,_L("?:\\F32-TST\\T_APPINS\\"));
//	TheNotifyFs.NotifyChange(ENotifyDir,statWilder,_L("*:\\"));
	r=TheFs.RmDir(path);
	test_KErrNone(r);
//	Still no notification	
	test (statEntry==KRequestPending);
	test(statFile==KRequestPending);
	test(statDir==KRequestPending);
	test(statWild==KErrNone);
	test(statWilder==KErrNone);

//	Cancel the outstanding requests
	TheNotifyFs.NotifyChangeCancel();
	}

LOCAL_C void Test3()
//
//	Test notification when session path of the notification session is set to be the 
//	same as the drive and directory into which files and directories are created
//
	{
	TFileName path;
	path=_L("\\F32-TST\\T_APPINS\\");
	TInt r=TheFs.RmDir(path);
	test_Value(r, (r == KErrNone)||(r==KErrNotFound)||(r==KErrPathNotFound));
	
	TRequestStatus statEntry(KRequestPending);
	TRequestStatus statFile(KRequestPending);
	TRequestStatus statDir(KRequestPending);
	TRequestStatus statWild(KRequestPending);
	TRequestStatus statWilder(KRequestPending);

	test.Printf(_L("Session Path %S\n"),&gSessionPath);
//	Set the session path of the session	which creates the file/directory to be
//	the same as the notification session's session path
	r=TheFs.SetSessionPath(gSessionPath);
	test_KErrNone(r);

//	Submit notify change requests (requesting ahead)
	test.Printf(_L("Create a directory %S\n"),&path);
	TheNotifyFs.NotifyChange(ENotifyEntry,statEntry,path);
	TheNotifyFs.NotifyChange(ENotifyFile,statFile,path);
	TheNotifyFs.NotifyChange(ENotifyDir,statDir,path);
	TheNotifyFs.NotifyChange(ENotifyEntry,statWild,_L("?:\\F32-TST\\"));
	TheNotifyFs.NotifyChange(ENotifyEntry,statWilder,_L("*:\\"));

	r=TheFs.MkDir(path);	
	test_Value(r, (r == KErrNone)||(r==KErrAlreadyExists));
	User::WaitForAnyRequest();
	test (statEntry==KErrNone);
	test(statFile==KErrNone);
	test(statDir==KErrNone);
	test(statWild==KErrNone);
	test(statWilder==KErrNone);

//	Resubmit notify change requests (requesting ahead)
	test.Next(_L("Create a file in the directory"));
	TheNotifyFs.NotifyChange(ENotifyEntry,statEntry,path);
	TheNotifyFs.NotifyChange(ENotifyFile,statFile,path);
	TheNotifyFs.NotifyChange(ENotifyDir,statDir,path);
	TheNotifyFs.NotifyChange(ENotifyEntry,statWild,_L("?:\\F32-TST\\"));
	TheNotifyFs.NotifyChange(ENotifyEntry,statWilder,_L("*:\\"));


	TFileName filePath;
	filePath=path;
	filePath+=_L("TestFile.app");
	RFile file;
	r=file.Replace(TheFs,filePath,EFileRead|EFileWrite);
	file.Close();
	User::WaitForAnyRequest();
	test_KErrNone(r);
	test (statEntry==KErrNone);
	test(statFile==KErrNone);
	test(statDir==KRequestPending);	
	test(statWild==KErrNone);
	test(statWilder==KErrNone);

//	Resubmit notify change requests	
	test.Next(_L("Remove the file from the directory"));
	TheNotifyFs.NotifyChange(ENotifyEntry,statEntry,path);
	TheNotifyFs.NotifyChange(ENotifyFile,statFile,path);
	TheNotifyFs.NotifyChange(ENotifyEntry,statWild,_L("?:\\F32-TST\\"));
	TheNotifyFs.NotifyChange(ENotifyEntry,statWilder,_L("*:\\"));

	r=TheFs.Delete(filePath);
	test_KErrNone(r);
	User::WaitForAnyRequest();
	test (statEntry==KErrNone);
	test(statFile==KErrNone);
	test(statDir==KRequestPending);
	test(statWild==KErrNone);
	test(statWilder==KErrNone);

//	Resubmit notify change requests	
	test.Next(_L("Remove the directory"));
	TheNotifyFs.NotifyChange(ENotifyEntry,statEntry,path);
	TheNotifyFs.NotifyChange(ENotifyFile,statFile,path);
	TheNotifyFs.NotifyChange(ENotifyEntry,statWild,_L("?:\\F32-TST\\"));
	TheNotifyFs.NotifyChange(ENotifyEntry,statWilder,_L("*:\\"));

	r=TheFs.RmDir(path);
	test_KErrNone(r);
	User::WaitForAnyRequest();
	test (statEntry==KErrNone);
	test(statFile==KRequestPending);
	test(statDir==KErrNone);
	test(statWild==KErrNone);
	test(statWilder==KErrNone);

//	Cancel any outstanding notification change requests
	TheNotifyFs.NotifyChangeCancel();
	}

#if !defined(__WINS__)

LOCAL_C void Test4()
//
//	Test notification when session path is set to be different from the drive and directory
//	into which files and directories are created
//
	{
	TFileName path;
	TBuf<23> pathBuf=_L("?:\\F32-TST\\T_APPINS\\");
	
	TChar driveChar;
	TInt r=RFs::DriveToChar(RemovableDrive,driveChar);
	test_KErrNone(r);
	
	if (gDriveToTest =='C')
		pathBuf[0]=driveChar;
	else
		pathBuf[0] =gDriveToTest;
	
	path = pathBuf;
	r=TheFs.RmDir(path);
	test_Value(r, (r == KErrNone)||(r==KErrNotFound)||(r==KErrPathNotFound));
		
	TInt result;
	result=TheFs.MkDir(_L("C:\\SILLY\\"));
	test_Value(result, (result == KErrNone)||(result==KErrAlreadyExists));
	result=TheFs.MkDir(_L("C:\\SILLY\\SILLIER\\"));
	test_Value(result, (result == KErrNone)||(result==KErrAlreadyExists));
	result=TheFs.MkDir(_L("C:\\SILLY\\SILLIER\\SILLIEST\\"));
	test_Value(result, (result == KErrNone)||(result==KErrAlreadyExists));

	result=TheNotifyFs.SetSessionPath(_L("C:\\SILLY\\SILLIER\\SILLIEST\\"));
	test_KErrNone(result);
	
	result=TheNotifyFs.SessionPath(gSessionPath);
	test_KErrNone(result);
	test.Printf(_L("Session Path %S\n"),&gSessionPath);

	TRequestStatus statEntry(KRequestPending);
	TRequestStatus statFile(KRequestPending);
	TRequestStatus statDir(KRequestPending);

//	Submit notify change requests (requesting ahead)
	test.Printf(_L("Create a directory %S\n"),&path);
	TheNotifyFs.NotifyChange(ENotifyEntry,statEntry,path);
	TheNotifyFs.NotifyChange(ENotifyFile,statFile,path);
	TheNotifyFs.NotifyChange(ENotifyDir,statDir,path);

	r=TheFs.MkDir(path);	
	test_Value(r, (r == KErrNone)||(r==KErrAlreadyExists));
	User::WaitForAnyRequest();
	test (statEntry==KErrNone);
	test(statFile==KErrNone);
	test(statDir==KErrNone);

//	Resubmit notify change requests (requesting ahead)
	test.Next(_L("Create a file in the directory"));
	TheNotifyFs.NotifyChange(ENotifyEntry,statEntry,path);
	TheNotifyFs.NotifyChange(ENotifyFile,statFile,path);
	TheNotifyFs.NotifyChange(ENotifyDir,statDir,path);

	TFileName filePath;
	filePath=path;
	filePath+=_L("TestFile.app");
	RFile file;
	r=file.Replace(TheFs,filePath,EFileRead|EFileWrite);
	file.Close();
	User::WaitForAnyRequest();
	test_KErrNone(r);
	test (statEntry==KErrNone);
	test(statFile==KErrNone);
	test(statDir==KRequestPending);

//	Resubmit notify change requests	
	test.Next(_L("Remove the file from the directory"));
	TheNotifyFs.NotifyChange(ENotifyEntry,statEntry,path);
	TheNotifyFs.NotifyChange(ENotifyFile,statFile,path);
	
	r=TheFs.Delete(filePath);
	test_KErrNone(r);
	User::WaitForAnyRequest();
	test (statEntry==KErrNone);
	test(statFile==KErrNone);
	test(statDir==KRequestPending);

//	Resubmit notify change requests	
	test.Next(_L("Remove the directory"));
	TheNotifyFs.NotifyChange(ENotifyEntry,statEntry,path);
	TheNotifyFs.NotifyChange(ENotifyFile,statFile,path);
	r=TheFs.RmDir(path);
	test_KErrNone(r);
	User::WaitForAnyRequest();
	test (statEntry==KErrNone);
	test(statFile==KRequestPending);
	test(statDir==KErrNone);

	result=TheFs.RmDir(_L("C:\\SILLY\\SILLIER\\SILLIEST\\"));
	test_Value(result, (result == KErrNone)||(result==KErrAlreadyExists));
	result=TheFs.RmDir(_L("C:\\SILLY\\SILLIER\\"));
	test_Value(result, (result == KErrNone)||(result==KErrAlreadyExists));
	result=TheFs.RmDir(_L("C:\\SILLY\\"));
	test_Value(result, (result == KErrNone)||(result==KErrAlreadyExists));	
	}

#endif
LOCAL_C void DoTests()
//
// Do all tests
//
	{
	Test1();
	Test2();
	Test3();
	}

GLDEF_D void CallTestsL(void)
//
//
//
    {
	test.Title();
	
	TChar driveLetter;
	TChar driveChar;
	TDriveList drvList;
	TBuf<13> dirBuf=_L("?:\\F32-TST\\");


	if (IsSessionDriveLFFS(TheFs,driveLetter))
		{
		test.Printf(_L("DoTestsL(): Skipped: test does not run on LFFS.\n"));
		return;
		}

	test.Start(_L("Testing filesystem"));
	TInt r=TheNotifyFs.Connect();
	test_KErrNone(r);
	TFileName sessionPath;
	TInt uid;
	test(HAL::Get(HAL::EMachineUid,uid)==KErrNone);
	if(uid==HAL::EMachineUid_Cogent || uid==HAL::EMachineUid_IQ80310 || uid==HAL::EMachineUid_X86PC)
		{
		test.Printf(_L("WARNING: d: not tested on cogent or IQ80310 \n"));
		goto End;
		}

#if !defined(__WINS__)
//	MARM TESTS
	r=TheFs.MkDir(_L("C:\\F32-TST\\"));
	test_Value(r, (r == KErrNone)||(r==KErrAlreadyExists));

	if(KErrNone == TheFs.DriveList(drvList))
		{
		TInt i;
		//should be successful, otherwise it means a system w/o any drive!!!
		for(i=0;i<KMaxDrives;i++)
			{
			TDriveInfo driveInfo;
			if((drvList[i] != 0)
				&& (KErrNone == TheFs.Drive(driveInfo, i))
				&& (driveInfo.iType == EMediaHardDisk)
				&& (driveInfo.iDriveAtt & KDriveAttRemovable))
				{
				RemovableDrive = i;
				test.Printf(_L("RemovableDrive = %d\n"),RemovableDrive);
				break;
				}
			}
		if(i == KMaxDrives)
			{
			test.Printf(_L("No Removable media found! Testing discontinued.\n"));
			User::Exit(KErrNone);
			}			
		}
	else
		{
		test.Printf(_L("No Drive found! Testing discontinued.\n"));
		User::Exit(KErrNone);
		}
	
	r=RFs::DriveToChar(RemovableDrive,driveChar);
	test_KErrNone(r);
	
	if(gDriveToTest == 'C')
		dirBuf[0] = (TText)driveChar;
	else
		dirBuf[0] = (TText)gDriveToTest;
		
	r=TheFs.MkDir(dirBuf);
	if ((r!=KErrNone)||(r!=KErrAlreadyExists))
		{
		test.Printf(_L("TEST REQUIRES A CF CARD - "));
		//test.Printf(_L("PUT A VALID CARD IN AND PRESS A KEY..."));
		//test.Getch();
		r=TheFs.MkDir(dirBuf);
		}
	test_Value(r, (r == KErrNone)||(r==KErrAlreadyExists));	
		
//	Set the notification session path to the test directory on C drive
	sessionPath=_L("C:\\F32-TST\\");
	r=TheNotifyFs.SetSessionPath(sessionPath);
	test_KErrNone(r);
//	Run tests
	TRAP(r,DoTests());
	if (r!=KErrNone)
		test.Printf(_L("Error: %d\n"),r);
	Test4();
	CheckDisk();

//	Set the notification session path to the test directory on gDriveToTest
	if(gDriveToTest == 'C')
		sessionPath[0] = (TText)driveChar;
	else
		sessionPath[0] = (TText)gDriveToTest;
		

	r=TheNotifyFs.SetSessionPath(sessionPath);
	test_KErrNone(r);

	test_KErrNone(r);
	TRAP(r,DoTests());
	if (r!=KErrNone)
		test.Printf(_L("Error: %d\n"),r);
	Test4();
	CheckDisk();
#elif defined (__WINS__)
	r=TheFs.MkDir(_L("X:\\F32-TST\\"));
	test_Value(r, (r == KErrNone)||(r==KErrAlreadyExists));
	r=TheFs.MkDir(_L("Y:\\F32-TST\\"));
	test_Value(r, (r == KErrNone)||(r==KErrAlreadyExists));
//	Set session path to test directory on Y drive
	r=TheNotifyFs.SetSessionPath(_L("Y:\\F32-TST\\"));
	test_KErrNone(r);
	TRAP(r,DoTests());
	if (r!=KErrNone)
		test.Printf(_L("Error: %d\n"),r);

	CheckDisk();

	//we have no X drive on eka2 yet
//	Set session path to test directory on X drive	
//	r=TheNotifyFs.SetSessionPath(_L("X:\\F32-TST\\"));
//	test_KErrNone(r);
//	TRAP(r,DoTests());

	if (r!=KErrNone)
		test.Printf(_L("Error: %d\n"),r);

	CheckDisk();
#endif

End:
	TheNotifyFs.Close();
	test.End();
	test.Close();
    }

