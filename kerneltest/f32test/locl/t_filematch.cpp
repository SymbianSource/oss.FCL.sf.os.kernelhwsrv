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
// A very boring piece of test code that tests a few filenames in various
// languages to make sure that support exists.
// Current languages:
// Polish, Czech, Hungarian, Greek.
// 
//

#ifdef __VC32__
    // Solve compilation problem caused by non-English locale
    #pragma setlocale("english")
#endif

// Need to define this macro to use error-code macros in e32test.h
#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <f32file.h>

#include "f32_test_utils.h"

using namespace F32_Test_Utils;

#include "t_server.h"

_LIT(KPolish1, "Jugos\x142\x61wii");
_LIT(KPolish1Match1, "jUGOS\x141\x41WII");
_LIT(KPolish1Match2, "jUGOS\x141\x41*WII");
_LIT(KPolish1Match3, "jUGOS\x141*i");
_LIT(KPolish1Match4, "Ju*wIi");
_LIT(KPolish1Match5, "jugos?awii");
_LIT(KPolish1NonMatch1, "jugoslawii");
_LIT(KPolish1NonMatch2, "jugosl*awii");
_LIT(KPolish1NonMatch3, "jugosl?awii");
_LIT(KPolish2, "KSI\x104Z\x307KI");
_LIT(KPolish2Match1, "ksia\x328z\x307ki");
_LIT(KPolish2Match2, "ksi?z\x307ki");
_LIT(KPolish2Match3, "ksia\x328?ki");
_LIT(KPolish2Match4, "ksi\x105\x17Cki");
_LIT(KPolish2NonMatch1, "ksiazki");
_LIT(KPolish2NonMatch2, "ksia*z\x307ki");
_LIT(KPolish2NonMatch3, "ksia\x328z*ki");
_LIT(KCzech1, "VY\x301KAZU\x30A");
_LIT(KCzech1Match1, "V\xDDKAZ\x16E");
_LIT(KCzech1Match2, "v\xFDkaz\x16F");
_LIT(KCzech1Match3, "*\x16F");
_LIT(KCzech1NonMatch1, "VY?KAZU??");
_LIT(KCzech2, "\x10Dtvrtlet\xED");
_LIT(KCzech2Match1, "c\x30Ctvrtleti\x301");
_LIT(KCzech2Match2, "?tvrtlet?");
_LIT(KHungarian1, "KO\x308ZE\x301RDEKU\x30B");
_LIT(KHungarian1Match1, "K\xD6Z\xC9RDEK\x170");
_LIT(KHungarian1Match2, "k\xF6z\xE9rdek\x171");
_LIT(KHungarian1Match3, "k?z?rdek?");
_LIT(KGreek1, "\x39B\x395\x3A6\x39A\x391\x3A3");
_LIT(KGreek1Match1, "\x3BB\x3B5\x3C6\x3BA\x3B1\x3C2");
_LIT(KGreek1Match2, "\x3BB\x3B5\x3C6\x3BA\x3B1\x3C3");
_LIT(KGreek1Match3, "??????");

CTrapCleanup* TrapCleanup;
RTest test(_L("T_FileMatch: testing file names in various languages."));

_LIT(KPath, "\\T_FileMatch\\");

TInt gDriveNum = -1;

void Begin()
	{
	TInt r = TheFs.MkDirAll(KPath);
	test_Assert(r == KErrNone || r == KErrAlreadyExists, test.Printf(_L("MkDirAll returned %d\n"),r));
	}

void CreateFile(const TDesC& aFileName)
	{
	TInt r;
	TFileName name;
	name = KPath;
	name.Append(aFileName);
	RFile file;
	r = file.Create(TheFs, name, EFileShareAny);
	test_KErrNone(r);
	file.Close();
	}

void DeleteFile(const TDesC& aFileName)
	{
	TInt r;
	TFileName name;
	name = KPath;
	name.Append(aFileName);
	r = TheFs.Delete(name);
	test_KErrNone(r);
	}

void CheckMatch(const TDesC& aFileName)
	{
	TInt r;
	RDir dir;
	TFileName name;
	name = KPath;
	name.Append(aFileName);
	r = dir.Open(TheFs, name, KEntryAttNormal);
	test_KErrNone(r);
	TEntry entry;
	r = dir.Read(entry);
	test_KErrNone(r);
	dir.Close();
	}

void CheckNonMatch(const TDesC& aFileName)
	{
	TInt r;
	RDir dir;
	TFileName name;
	name = KPath;
	name.Append(aFileName);
	r = dir.Open(TheFs, name, KEntryAttNormal);
	test_KErrNone(r);
	TEntry entry;
	r = dir.Read(entry);
	test_Equal(KErrEof, r);
	dir.Close();
	}

void TestFilenameMatches()
	{
	test.Next(_L("TestFilenameMatches()"));
    
    Begin();
	CreateFile(KPolish1);
	CheckMatch(KPolish1Match1);
	CheckMatch(KPolish1Match2);
	CheckMatch(KPolish1Match3);
	CheckMatch(KPolish1Match4);
	CheckMatch(KPolish1Match5);
	CheckNonMatch(KPolish1NonMatch1);
	CheckNonMatch(KPolish1NonMatch2);
	CheckNonMatch(KPolish1NonMatch3);
	DeleteFile(KPolish1);
	CreateFile(KPolish2);
	CheckMatch(KPolish2Match1);
	CheckMatch(KPolish2Match2);
	CheckMatch(KPolish2Match3);
	CheckMatch(KPolish2Match4);
	CheckNonMatch(KPolish2NonMatch1);
	CheckNonMatch(KPolish2NonMatch2);
	CheckNonMatch(KPolish2NonMatch3);
	DeleteFile(KPolish2);
	CreateFile(KCzech1);
	CheckMatch(KCzech1Match1);
	CheckMatch(KCzech1Match2);
	CheckMatch(KCzech1Match3);
	CheckNonMatch(KCzech1NonMatch1);
	DeleteFile(KCzech1);
	CreateFile(KCzech2);
	CheckMatch(KCzech2Match1);
	CheckMatch(KCzech2Match2);
	DeleteFile(KCzech2);
	CreateFile(KHungarian1);
	CheckMatch(KHungarian1Match1);
	CheckMatch(KHungarian1Match2);
	CheckMatch(KHungarian1Match3);
	DeleteFile(KHungarian1);
	CreateFile(KGreek1);
	CheckMatch(KGreek1Match1);
	CheckMatch(KGreek1Match2);
	CheckMatch(KGreek1Match3);
	DeleteFile(KGreek1);
    }

void CallTestsL(void)
	{
    //-- set up console output 
    F32_Test_Utils::SetConsole(test.Console()); 
    
    TInt nRes=TheFs.CharToDrive(gDriveToTest, gDriveNum);
    test_KErrNone(nRes);
    
    PrintDrvInfo(TheFs, gDriveNum);

    if(Is_SimulatedSystemDrive(TheFs, gDriveNum) || Is_Fat(TheFs, gDriveNum) || Is_Lffs(TheFs, gDriveNum))
        {
	    TestFilenameMatches();
        }
    else
        {
        test.Printf(_L("This test can't be performed on this file system. Skipping.\n"));
        }

	}

