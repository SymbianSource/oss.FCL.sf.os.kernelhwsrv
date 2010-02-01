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
// f32test\demandpaging\t_denyclamp.cpp
// This test suite has a reduced set of the tests in T_CLAMP, to verify
// the behaviour when file clamping is attempted by an invalid user.
// 002 GetDriveLetters() Assign the first drive that matches the required criteria
// 003 Test1() Basic clamp operation
// 004 Test2() Invalid clamp requests
// 005 Test4() Clamp tests for non-writable file system
// 006 Test5() Clamp requests on non-clamping file systems
// This file has a reduced set of tests present in T_CLAMP, to verify
// the behaviour when file clamping is attempted by an invalid user
// 
//

//! @SYMTestCaseID			KBASE-T_DENYCLAMP-0329
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1110
//! @SYMTestCaseDesc		Demand Paging File Clamp tests (Deny)
//! @SYMTestActions			001 Starting T_DENYCLAMP ...
//! @SYMTestExpectedResults All tests should pass.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented

#include <e32test.h>
RTest test(_L("T_DENYCLAMP"));

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
	test.Next(_L("T_DENYCLAMP - Test1()"));

	TBuf<256> fileName;	
	TBuf<256> buf(_L("buffer for file used"));

	fileName = _L("clampFile.tst");
	RFile testFile;
	TInt r=testFile.Replace(TheFs,fileName,EFileWrite);
	test(r==KErrNone);
	TPtrC8 pBuf((TUint8*)&buf);
	testFile.Write(pBuf);
	testFile.Flush();

	// Attempt to clamp file should be rejected
	RFileClamp handle;
	r=handle.Clamp(testFile);
	test(r==KErrPermissionDenied);

	// Attempt to unclamp a file should be rejected
	// Using an invalid-content cookie is OK - the request should
	// be rejected before the content is examined
	handle.iCookie[0]=MAKE_TINT64(-1,-1);
	handle.iCookie[1]=0;
	r=handle.Close(TheFs);
	test (r==KErrPermissionDenied);

	// Tidy up
	testFile.Close();
	r=TheFs.Delete(_L("clampFile.tst"));
	test (r==KErrNone);
	}


LOCAL_C void Test2()
	{
// Invalid clamp requests
	test.Next(_L("T_DENYCLAMP - Test2()"));
	
	// Test attempt to clamp empty file is rejected
	RFileClamp handle2;
	TBuf<256> file2Name;	
	file2Name = _L("clampFile2.tst");
	RFile testFile2;
	TInt r=testFile2.Replace(TheFs,file2Name,EFileWrite);
	test(r==KErrNone);
	r=handle2.Clamp(testFile2);
	test(r==KErrPermissionDenied);


	// Try to unclamp non-existant file
	// Using a invalid-content cookie is OK - the request should
	// be rejected before the content is examined
	handle2.iCookie[0] = MAKE_TINT64(-1,-1); // iCookie[0] holds the unique ID
	handle2.iCookie[1] = MAKE_TINT64(-1,-1);
	r=handle2.Close(TheFs);
	test (r==KErrPermissionDenied);

	// Tidy up
	testFile2.Close();
	r=TheFs.Delete(_L("clampFile2.tst"));
	test (r==KErrNone);
	}


LOCAL_C void Test4(TDesC& aRoot)
	{
// Clamp tests for non-writable file system
	test.Next(_L("T_DENYCLAMP - Test4()"));

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

	// Attempt to clamp file
	RFileClamp handle;
	r=handle.Clamp(testFile);
	test(r==KErrPermissionDenied);

	// Unclamp file
	// Using an invalid-content cookie is OK - the request should
	// be rejected before the content is examined
	handle.iCookie[0]=MAKE_TINT64(-1,-1);
	handle.iCookie[1]=0;
	r=handle.Close(TheFs);
	test (r==KErrPermissionDenied);

	testFile.Close();
	}


LOCAL_C void Test5()
	{
// Clamp requests on non-clamping file systems
	test.Next(_L("T_DENYCLAMP - Test5()"));

	TBuf<256> unsuppFile;	
	unsuppFile = _L("unsuppFile.tst");
	RFile testFile;
	TInt r=testFile.Replace(TheFs,unsuppFile,EFileWrite);
	test(r==KErrNone);

	// Try to clamp a file on a file system that does
	// not support clamping
	RFileClamp handle;
	r=handle.Clamp(testFile);
	test(r==KErrPermissionDenied);

	// Tidy up
	testFile.Close();
	r=TheFs.Delete(_L("unsuppFile.tst"));
	test (r==KErrNone);
	}	


LOCAL_C void GetDriveLetters()
	{
// Assign the first drive that matches the required criteria
	test.Next(_L("T_DENYCLAMP - GetDriveLetters()"));

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

	RDebug::Printf("T_DENYCLAMP: FAT drive=%C, ROFS drive=%C, LFFS drive=%C, ROM-COMP drive=%C \n",(TText)NandFatDrv,(TText)RofsDrv,(TText)LffsDrv,(TText)CompDrv);
	return;
	}


//
// E32Main
//

TInt E32Main()
	{
	TInt r;
	test.Title();
	test.Start(_L("Starting T_DENYCLAMP ..."));
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
		test.Printf( _L("T_DENYCLAMP: testing FAT drive on %C\n"),(TText)NandFatDrv);

		Test1();		// Basic clamp operation
		Test2();		// Invalid clamp requests
//		Test3(pathName);// Denied FS requests when files are clamped - invalid for T_DENYCLAMP

		r=TheFs.RmDir(pathName);
		test(r==KErrNone);
		}
	else
		test.Printf( _L("T_DENYCLAMP: FAT drive not tested\n"));

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
		test.Printf( _L("T_DENYCLAMP: testing ROFS drive on %C\n"),(TText)RofsDrv);

		Test4(pathName);	// Clamp tests for non-writable file system
		}
	else
		test.Printf( _L("T_DENYCLAMP: ROFS drive not tested\n"));

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
		test.Printf( _L("T_DENYCLAMP: testing Z drive (on %C)\n"),(TText)CompDrv);

		Test4(pathName);	// Clamp tests for non-writable file system
		}
	else
		test.Printf( _L("T_DENYCLAMP: Z drive not tested\n"));

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
		test.Printf( _L("T_DENYCLAMP: testing LFFS drive on %C\n"),(TText)LffsDrv);

		Test5();		// Clamp requests on non-clamping file systems
		}
	else
		test.Printf( _L("T_DENYCLAMP: LFFS drive not tested\n"));

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
