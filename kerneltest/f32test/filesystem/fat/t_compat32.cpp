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
// f32test\fat32\t_compat32.cpp
//
//

#define __E32TEST_EXTENSION__

#include <f32file.h>
#include <e32svr.h>
#include <e32test.h>
#include <f32dbg.h>
#include "t_server.h"

#include "fat_utils.h"
using namespace Fat_Test_Utils;

RTest test(_L("T_COMPAT32"));

static RRawDisk TheDisk;
static TFatBootSector gBootSector;


static void QuickFormat()
    {
    FormatFatDrive(TheFs, CurrentDrive(), ETrue);
    }

static void ReadBootSector(TFatBootSector& aBootSector)
	{

    TInt nRes = ReadBootSector(TheFs, CurrentDrive(), KBootSectorNum<<KDefaultSectorLog2, aBootSector);
    test(nRes == KErrNone);

    if(!aBootSector.IsValid())
        {
        test.Printf(_L("Wrong bootsector! Dump:\n"));
        aBootSector.PrintDebugInfo();
        test(0);
        }
	}


static void GetBootInfo()
	{
	QuickFormat();
	ReadBootSector(gBootSector);
	}

enum TNameCase
	{
	EUpper, // Test directory entries with 8.3 uppercase (no VFAT entries expected)
	ELower, // Test directory entries with 8.3 lowercase (   VFAT entries expected)
	EMixed  // Test directory entries with 8.3 mixed     (   VFAT entries expected)
	};


/**
    Fiddles with root directory entries.
    Creates a file, if it has 1 VFAT and 1 DOS dir. entries, places an illegal lower case
    symbol to the DOS entry, fixing VFAT name checksums
*/
static void DoFiddleWithFileNames(TNameCase aCase)
{
	TFileName fileName = _L("\\WORD");
	TBool expectVfatEntry = EFalse;

	switch(aCase)
		{
		case EUpper:
			break;

		case ELower:
			fileName = _L("\\word");
			expectVfatEntry = ETrue;
			break;

		case EMixed:
			fileName = _L("\\WoRd");
			expectVfatEntry = ETrue;
			break;

		default:
			test(0);
			break;
		}

	RFile file;
	TInt r=file.Create(TheFs,fileName,EFileRead);
	test_KErrNone(r);
	file.Close();
//	Assume this file is the first entry in the root directory


	r=TheDisk.Open(TheFs,CurrentDrive());
	test_KErrNone(r);

    //-- read 1st dir. entry it can be FAT or VFat , depending on the filename
    const TInt posEntry1=gBootSector.RootDirStartSector() << KDefaultSectorLog2; //-- dir entry1 position

    TFatDirEntry fatEntry1;
	TPtr8 ptrEntry1((TUint8*)&fatEntry1,sizeof(TFatDirEntry));

    test(TheDisk.Read(posEntry1, ptrEntry1)==KErrNone);

    if(!fatEntry1.IsVFatEntry())
    {//-- we expected FAT entry, everything is OK
        test(!expectVfatEntry);
    }
    else
    {//-- we have 2 FAT entries, 1st is VFat, 2nd is DOS.
     //-- put lower case letters into DOS entry( not compliant with FAT specs), correct VFat entries checksums,
     //-- in this case the system shall correctly deal with the file, using long names.

        test(expectVfatEntry);
        test(fatEntry1.iData[0] == 0x41); //-- must have only 2 entries

        //-- read DOS entry now
        TFatDirEntry fatEntry2;
        TPtr8 ptrEntry2((TUint8*)&fatEntry2,sizeof(TFatDirEntry));
        const TInt posEntry2 = posEntry1 + sizeof(TFatDirEntry); //-- dir entry2 position

        test(TheDisk.Read(posEntry2, ptrEntry2)==KErrNone);

        //-- ensure that the name and checksum are correct
        test(fatEntry1.iData[13] == CalculateShortNameCheckSum(fatEntry2.Name()));
        test(fatEntry2.Name()==_L8("WORD       "));

        //-- put lower case symbol to the DOS entry and fix the checksum
        _LIT8(KBadDosName, "Word       ");
        fatEntry2.SetName(KBadDosName);
        fatEntry1.iData[13] = CalculateShortNameCheckSum(fatEntry2.Name());

        //-- write data to the disk
        test(TheDisk.Write(posEntry1, ptrEntry1)==KErrNone);
        test(TheDisk.Write(posEntry2, ptrEntry2)==KErrNone);

    }

	TheDisk.Close();

}

//
// Replace a 8.3 filename with upper and lower case letters which is, actually out of FAT specs.
// I.e. VFAT entries are valid, but DOS entry has a lower case symbol, which is wrong.
//
static void Test1(TNameCase aCase)
	{
	test.Next(_L("Replace a file with a wrong DOS entry"));
	QuickFormat();

    //-- N.B. This shall be the before any dir. entries creation in the root directory
    //-- because it directly accesses the directory's 1st file
    DoFiddleWithFileNames(aCase);

    RFile file;
    TInt r;

	r=file.Replace(TheFs,_L("\\FILE.TMP"),EFileRead);
	test_KErrNone(r);
	r=file.Write(_L8("Hello World"));
	file.Close();

	r=TheFs.Replace(_L("\\File.tmp"),_L("\\Word"));
	test_KErrNone(r);

	CDir* entryCount;
	r=TheFs.GetDir(_L("\\*.*"),KEntryAttMaskSupported,ESortNone,entryCount);
	test_KErrNone(r);
	TInt count=entryCount->Count();

	test(count==1);
	delete entryCount;
	}


//
// Renaming a 8.3 filename with upper and lower case letters which is, actually out of FAT specs.
// I.e. VFAT entries are valid, but DOS entry has a lower case symbol, which is wrong.
//
static void Test2(TNameCase aCase)
	{
	test.Next(_L("Rename a file with a wrong DOS entry"));
	QuickFormat();
	RFile file;
    TInt r;

    //-- N.B. This shall be the before any dir. entries creation in the root dir
    //-- because it directly accesses the directory's 1st file
    DoFiddleWithFileNames(aCase);

	r=file.Create(TheFs,_L("\\TEST"),EFileRead);
	test_KErrNone(r);
	file.Close();

	r=TheFs.Rename(_L("\\TEST"),_L("\\Word"));
	test_Value(r, r == KErrAlreadyExists);
	r=TheFs.Delete(_L("\\TEST"));
	test_KErrNone(r);

	CDir* entryCount;
	r=TheFs.GetDir(_L("\\*.*"),KEntryAttMaskSupported,ESortNone,entryCount);
	test_KErrNone(r);
	TInt count=entryCount->Count();
	test(count==1);
	delete entryCount;
	}


//---------------------------------------------
//! @SYMTestCaseID			PBASE-T_COMPAT32-0686
//! @SYMTestType			CT
//! @SYMREQ					DEF115314
//! @SYMTestCaseDesc		Test character '`' (0x60) is recognized as a legal char for short file names.
//! @SYMTestActions			Creates a file named "\x60\x60\x60.TXT", checks only DOS entry is created for
//!							it and its short name equals "```.TXT" instead of "___.TXT"
//! @SYMTestExpectedResults	The operation completes with error code KErrNone;
//! @SYMTestPriority		High
//! @SYMTestStatus			Implemented
//---------------------------------------------
void TestDEF115314()
	{
	test.Next(_L("Test DEF115314: TTG:<`(0x60) code cannot be used as valid Short File Name>"));
	QuickFormat();
	RFile file;
    TInt r;

    TFileName fn;
    fn.Format(_L("%c:\\\x60\x60\x60.TXT"), (TUint8)gDriveToTest);

    r = TheFs.Delete(fn);
	test_Value(r, r == KErrNone || r==KErrNotFound);

	r = file.Create(TheFs, fn, EFileRead);
	test_KErrNone(r);
	file.Close();

	r=TheDisk.Open(TheFs,CurrentDrive());
	test_KErrNone(r);

    //-- read 1st dir. it should be DOS Entry
    const TInt posEntry1=gBootSector.RootDirStartSector() << KDefaultSectorLog2; //-- dir entry1 position
    TFatDirEntry fatEntry1;
	TPtr8 ptrEntry1((TUint8*)&fatEntry1,sizeof(TFatDirEntry));
    test(TheDisk.Read(posEntry1, ptrEntry1)==KErrNone);
    TheDisk.Close();
    test(!fatEntry1.IsVFatEntry());

    // tests short name
    TFileName sn;
    r = TheFs.GetShortName(fn, sn);
    test_KErrNone(r);
    test(sn.Compare(_L("```.TXT"))==0);

    r = TheFs.Delete(fn);
	test_KErrNone(r);
	}

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
_LIT(KTestLocale, 			"t_tlocl_cp932.dll");
_LIT(KTestUnicodeFileName, 	"\\\x65B0\x6587\x4EF6");
#endif //_DEBUG || _DEBUG_RELEASE

//---------------------------------------------
//! @SYMTestCaseID			PBASE-T_COMPAT32-0685
//! @SYMTestType			CT
//! @SYMREQ					DEF113633
//! @SYMTestCaseDesc		Test FAT volume creates VFat entries for short unicode named files
//! @SYMTestActions			Enables FatUtilityFunctions. Loads cp932 codepage dll. Create a file
//!							named as "\x65B0\x6587\x4EF6", checks both a VFat entry and a DOS
//!							entry have been created for the file.
//! @SYMTestExpectedResults	The operation completes with error code KErrNone;
//! @SYMTestPriority		High
//! @SYMTestStatus			Implemented
//---------------------------------------------
void TestDEF113633()
	{
	test.Next(_L("Test DEF113633 - FAT should create VFat entries for unicode character contained file names"));
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	QuickFormat();
	RFile file;
    TInt r;
    TInt drvNum;

    r = TheFs.CharToDrive(gDriveToTest,drvNum);
	test_KErrNone(r);

    // turn on FatUtilityFunctions
    r = TheFs.ControlIo(drvNum, KControlIoEnableFatUtilityFunctions);
	test_KErrNone(r);

	// load cp932 codepage dll
	r = UserSvr::ChangeLocale(KTestLocale);
	test_KErrNone(r);

    // create file "\x65B0\x6587\x4EF6", check DOS entry & VFat entry
	r = file.Create(TheFs, KTestUnicodeFileName, EFileRead);
	test_KErrNone(r);
	file.Close();

	r=TheDisk.Open(TheFs,CurrentDrive());
	test_KErrNone(r);

    //-- read 1st dir. it should be VFat
//    const TInt posEntry1=gRootDirStart; //-- dir entry1 position
    const TInt posEntry1=gBootSector.RootDirStartSector() << KDefaultSectorLog2; //-- dir entry1 position
    TFatDirEntry fatEntry1;
	TPtr8 ptrEntry1((TUint8*)&fatEntry1,sizeof(TFatDirEntry));

    test(TheDisk.Read(posEntry1, ptrEntry1)==KErrNone);

    test(fatEntry1.IsVFatEntry());

    test(fatEntry1.iData[0] == 0x41); //-- must have only 2 entries

    //-- read DOS entry now
    TFatDirEntry fatEntry2;
    TPtr8 ptrEntry2((TUint8*)&fatEntry2,sizeof(TFatDirEntry));
    const TInt posEntry2 = posEntry1 + sizeof(TFatDirEntry); //-- dir entry2 position

    test(TheDisk.Read(posEntry2, ptrEntry2)==KErrNone);

    //-- ensure that the name and checksum are correct
    test(!fatEntry2.IsVFatEntry());
    test(fatEntry1.iData[13] == CalculateShortNameCheckSum(fatEntry2.Name()));

    // delete file
    TheDisk.Close();
    r = TheFs.Delete(KTestUnicodeFileName);
	test_KErrNone(r);

	// turn off FatUtilityFunctions
	r = TheFs.ControlIo(drvNum, KControlIoDisableFatUtilityFunctions);
	test_KErrNone(r);

#else
	test.Printf(_L("Test only runs on DEBUG builds, see test logs of debug builds for details."));
#endif  // _DEBUG) || _DEBUG_RELEASE
	}




//---------------------------------------------
// If the parent directory of a directory is the root directory,
// the '..' entry should point to start cluster 0 in any case
//---------------------------------------------
void TestPDEF116912()
	{
	test.Next(_L("Test PDEF116912 - Check that '..' parent cluster address is 0 after renaming\n"));

  	TInt drvNum;
	test(KErrNone == TheFs.CharToDrive(gDriveToTest, drvNum));

	if(!Is_Fat32(TheFs, drvNum))
		{
		_LIT(KMessage, "Test only applicable to FAT32 file systems. Skipping.\n");
		test.Printf(KMessage);
		return;
		}

	QuickFormat();

	_LIT(KDirA, "\\dirA\\");
	_LIT(KDirB, "\\dirB\\");

	test(KErrNone == TheFs.MkDir(KDirA));
	test(KErrNone == TheFs.Rename(KDirA, KDirB));

	test(gBootSector.IsValid());
	TInt mediaPosition = gBootSector.RootDirStartSector() * gBootSector.BytesPerSector();

	TFatDirEntry dirEntry;
	TPtr8 ptrEntry((TUint8*) &dirEntry,sizeof(TFatDirEntry));

	_LIT8(KDirBMatchPattern, "DIRB *");
	_LIT8(KDotDotMatchPattern, ".. *");

	const TInt KMaxEntriesToSearch = (gBootSector.SectorsPerCluster() * gBootSector.BytesPerSector()) / KSizeOfFatDirEntry;
	test(KErrNone == TheDisk.Open(TheFs, drvNum));

	for(TInt c = 0; c < KMaxEntriesToSearch; c++)
		{
	    test(KErrNone == TheDisk.Read(mediaPosition, ptrEntry));

		if(KErrNotFound == ptrEntry.Match(KDirBMatchPattern))
			{
			// keep scanning
			mediaPosition += sizeof(TFatDirEntry);
			}
		else
			{
			// found, locate '..' entry
			test(dirEntry.StartCluster() >= KFatFirstSearchCluser);
			mediaPosition  = gBootSector.FirstDataSector();
			mediaPosition += (dirEntry.StartCluster() - KFatFirstSearchCluser) * gBootSector.SectorsPerCluster();
			mediaPosition *= gBootSector.BytesPerSector();
			mediaPosition += KSizeOfFatDirEntry; // '..' is always the 2nd entry

			test(KErrNone == TheDisk.Read(mediaPosition, ptrEntry));

			test(KErrNotFound != ptrEntry.Match(KDotDotMatchPattern));
			test(dirEntry.StartCluster() == 0);

			TheDisk.Close();
			return;
			}
		}

	// dirB entry not found - test failed
	TheDisk.Close();
	test(0);
	}

//---------------------------------------------
/**
    Test replacing files by theis short names
*/
void TestReplaceByShortName()
{
    test.Next(_L("Test replacing files using short names\n"));
    QuickFormat();

    _LIT(KLongName1,  "abcdefghi.txt");
    _LIT(KShortName1, "ABCDEF~1.TXT");
    const TInt KFile1Sz = 100;

    _LIT(KLongName2,  "abcdefghij.txt");
    _LIT(KShortName2, "ABCDEF~2.TXT");
    const TInt KFile2Sz = 200;


    TInt        nRes;
    TFileName   fn;
    TEntry      entry;

    TheFs.SetSessionPath(_L("\\"));

    nRes = CreateCheckableStuffedFile(TheFs, KLongName1, KFile1Sz);
    test_KErrNone(nRes);

    nRes = TheFs.GetShortName(KLongName1, fn);
    test(nRes == KErrNone && fn == KShortName1); //-- just check short name generation

    nRes =CreateCheckableStuffedFile(TheFs, KLongName2, KFile2Sz);
    test_KErrNone(nRes);

    nRes = TheFs.GetShortName(KLongName2, fn);
    test(nRes == KErrNone && fn == KShortName2); //-- just check short name generation

    //-- try to replace the file with itself using its short name alias
    //-- nothing shall happen and the file must remain the same
    nRes = TheFs.Replace(KLongName1, KShortName1);
    test(nRes == KErrNone);

    nRes = TheFs.Entry(KLongName1, entry);
    test(nRes == KErrNone);
    test(entry.iSize == KFile1Sz);

    nRes = TheFs.Entry(KShortName1, entry);
    test(nRes == KErrNone);
    test(entry.iSize == KFile1Sz);


    nRes = TheFs.Replace(KShortName1, KLongName1);
    test(nRes == KErrNone);

    nRes = TheFs.Entry(KLongName1, entry);
    test(nRes == KErrNone);
    test(entry.iSize == KFile1Sz);

    nRes = TheFs.Entry(KShortName1, entry);
    test(nRes == KErrNone);
    test(entry.iSize == KFile1Sz);

    nRes = VerifyCheckableFile(TheFs, KLongName1);
    test(nRes == KErrNone);

    nRes = VerifyCheckableFile(TheFs, KShortName1);
    test(nRes == KErrNone);


    //-- replace "abcdefghi.txt" by "ABCDEF~2.TXT" which is the alias for "abcdefghij.txt"
    //-- expected: contents and all attributes of the "abcdefghij.txt" is replaced with "abcdefghi.txt"
    //-- "abcdefghi.txt" entries gets deleted.

    nRes = TheFs.Replace(KLongName1, KShortName2);
    test(nRes == KErrNone);

    //User::After(5*K1Sec);
    /*
    nRes = VerifyCheckableFile(TheFs, KLongName2);
    test(nRes == KErrNone);

    nRes = VerifyCheckableFile(TheFs, KShortName2);
    test(nRes == KErrNone);
    */


    nRes = TheFs.Entry(KLongName2, entry);
    test(nRes == KErrNone);
    test(entry.iSize == KFile1Sz);

    nRes = TheFs.Entry(KShortName2, entry);
    test(nRes == KErrNone);
    test(entry.iSize == KFile1Sz && entry.iName == KLongName2);


}

//---------------------------------------------
/**
    Test that the created VFAT entryset corresponds to what Windows creates in the 
    same situation
*/
void TestVFatEntryInterop()
{
    test.Next(_L("Testind VFAT entries interoperability\n"));
    QuickFormat();

    TInt nRes;
    _LIT(KFName, "\\longfilename12345678");
    
    //-- 1. create a file with long FN that isn't multiple of 13 (max unicode characters in VFAT entry)
    const TUint KFileSize = 24;
    nRes = CreateEmptyFile(TheFs, KFName, KFileSize);
    test(nRes == KErrNone);

    //-- 2. verify that the dir. entries are the same what Windows creates.
	nRes = TheDisk.Open(TheFs,CurrentDrive());
	test(nRes == KErrNone);

    //-- read 1st dir. entry from the root dir and check it.
    //-- this is the rest of the LFN 
    TInt64 posEntry = gBootSector.RootDirStartSector() << KDefaultSectorLog2; //-- dir entry1 position

    TFatDirEntry fatEntry;
	TPtr8 ptrEntry((TUint8*)&fatEntry, KSizeOfFatDirEntry);

    nRes = TheDisk.Read(posEntry, ptrEntry);
    test(nRes == KErrNone);

    //-- the expected entry #1 contents (what Windows places there). 
    const TUint8 KEntry1[KSizeOfFatDirEntry] = {0x42, 0x32, 0x00, 0x33, 0x00, 0x34, 0x00, 0x35, 0x00, 0x36, 0x00, 0x0F, 0x00, 0xF7, 0x37, 0x00,
                                                0x38, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF };

    nRes = Mem::Compare(fatEntry.iData, KSizeOfFatDirEntry, KEntry1, KSizeOfFatDirEntry);
    test(nRes == KErrNone);

    //-- read 2nd dir. entry from the root dir and check it.
    //-- this is the beginning of the LFN 

    posEntry += KSizeOfFatDirEntry;
    nRes = TheDisk.Read(posEntry, ptrEntry);
    test(nRes == KErrNone);

    //-- the expected entry #2 contents (what Windows places there). 
    const TUint8 KEntry2[KSizeOfFatDirEntry] = { 0x01, 0x6C, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x67, 0x00, 0x66, 0x00, 0x0F, 0x00, 0xF7, 0x69, 0x00,
                                                 0x6C, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x61, 0x00, 0x6D, 0x00, 0x00, 0x00, 0x65, 0x00, 0x31, 0x00 };

    nRes = Mem::Compare(fatEntry.iData, KSizeOfFatDirEntry, KEntry2, KSizeOfFatDirEntry);
    test(nRes == KErrNone);

    //-- read the last, 3rd entry from the root dir and check it.
    //-- this is the DOS entry

    posEntry += KSizeOfFatDirEntry;
    nRes = TheDisk.Read(posEntry, ptrEntry);
    test(nRes == KErrNone);

    //-- first 13 bytes of DOS entry SFN, attributes and DIR_NTRes field
    const TUint8 KEntry3[13] = {'L','O','N','G','F','I','~','1',' ',' ',' ', 0x20, 0x00 };
    nRes = Mem::Compare(fatEntry.iData, 13, KEntry3, 13);
    test(nRes == KErrNone);

    //-- skip file time stamps, they are not consistent

    //-- test file size and start cluster of the file
    test(fatEntry.StartCluster() != gBootSector.RootClusterNum() && fatEntry.StartCluster() != 0);
    test(fatEntry.Size() == KFileSize);

    //-- goto the next entry, this must be the end of directory
    posEntry += KSizeOfFatDirEntry;
    nRes = TheDisk.Read(posEntry, ptrEntry);
    test(nRes == KErrNone);
    test(fatEntry.IsEndOfDirectory());

    TheDisk.Close();

}


void CallTestsL()
	{

	TInt drvNum;
	TInt r=TheFs.CharToDrive(gDriveToTest,drvNum);
	test_KErrNone(r);

    if (!Is_Fat(TheFs,drvNum))
		{
		test.Printf(_L("CallTestsL: Skipped: Requires FAT filesystem to run.\n"));
		return;
		}


    //-- set up console output
    SetConsole(test.Console());

    //-- print drive information
    PrintDrvInfo(TheFs, drvNum);

	GetBootInfo();

    TestVFatEntryInterop();

	Test1(EUpper); // Test directory entries with 8.3 uppercase (no VFAT entries expected)
	Test1(ELower); // Test directory entries with 8.3 lowercase (   VFAT entries expected)
	Test1(EMixed); // Test directory entries with 8.3 mixed     (   VFAT entries expected)

	Test2(EUpper); // Test directory entries with 8.3 uppercase (no VFAT entries expected)
	Test2(ELower); // Test directory entries with 8.3 lowercase (   VFAT entries expected)
	Test2(EMixed); // Test directory entries with 8.3 mixed     (   VFAT entries expected)

	TestDEF115314();
	TestDEF113633();
	TestPDEF116912();

    TestReplaceByShortName();
	}



