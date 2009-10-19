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
// File Name:		f32test\bench\t_fat_perf_main.cpp
// This file is intentionally created although t_main.cpp exists 
// to start our tests.This file contains only necessary code to 
// start our tests, whereas t_main.cpp creates test directory 
// \\F32-TST\\, which will affect 'LeafDirCache' test cases
// (PREQ 1885).
// 
//

#define __E32TEST_EXTENSION__

#include <f32file.h>
#include <e32test.h>
#include <e32hal.h>
#include <e32math.h>
#include <f32dbg.h>
#include "t_server.h"

RFs TheFs;
TFileName gSessionPath;
TChar gDriveToTest;


void SetSessionPath(const TDesC& aPathName)
//
// Set the session path and update gSessionPath
//
	{

	TInt r=TheFs.SetSessionPath(aPathName);
	test(r==KErrNone);
	r=TheFs.SessionPath(gSessionPath);
	test(r==KErrNone);
	}

TInt CurrentDrive()
//
// Return the current drive number
//
	{

	TInt driveNum;
	TInt r=TheFs.CharToDrive(gSessionPath[0],driveNum);
	test(r==KErrNone);
	return(driveNum);
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

	gSessionPath=_L("?:\\");
	TChar driveLetter;
	TInt r=TheFs.DriveToChar(aDrive,driveLetter);
	test(r==KErrNone);
	gSessionPath[0]=(TText)driveLetter;
	r=TheFs.SetSessionPath(gSessionPath);
	test(r==KErrNone);

	TheFs.ResourceCountMarkStart();
	TRAP(r,CallTestsL());
	if (r==KErrNone)
		TheFs.ResourceCountMarkEnd();
	else
		{
		test.Printf(_L("Error: Leave %d\n"),r);
		test(EFalse);
		}

	}


void ParseCommandArguments()
	{
	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	TPtrC token=lex.NextToken();
	TFileName thisfile=RProcess().FileName();
	if (token.MatchF(thisfile)==0)
		{
		token.Set(lex.NextToken());
		}
	test.Printf(_L("CLP=%S"),&token);

	if (token.Length()!=0)		
		{
		gDriveToTest=token[0];
		gDriveToTest.UpperCase();
		}
	else						
		{
		gDriveToTest='C';		
		}
	}


TInt E32Main()
    {

	CTrapCleanup* cleanup;
	cleanup=CTrapCleanup::New();
	TRAPD(r,PushLotsL());
	__UHEAP_MARK;

	test.Title();
	test.Start(_L("Starting tests..."));

	ParseCommandArguments(); //need this for drive letter to test

	r=TheFs.Connect();
	test(r==KErrNone);

	TInt theDrive;
	r=TheFs.CharToDrive(gDriveToTest,theDrive);
    test(r==KErrNone);
	
	DoTests(theDrive);

	TheFs.Close();
	test.End();
	test.Close();
	__UHEAP_MARKEND;
	delete cleanup;
	return(KErrNone);
    }

//EOF
