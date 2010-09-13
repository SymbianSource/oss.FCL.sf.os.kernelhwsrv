/*
* Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* /sf/os/kernelhwsrv/kerneltest/f32test/server/t_cp_plugintest.cpp
*/


#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <hal.h>
#include <f32fsys.h>
#include <f32dbg.h>
#include "../server/t_server.h"
#include "fat_utils.h"

	
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
using namespace Fat_Test_Utils;
static RRawDisk TheDisk;
static TFatBootSector gBootSector;

#define MakeFile_CPTest(filename)											\
	{																		\
	test.Printf(_L("MakeFile(%S)	at	LINE:%d\n"),&filename,__LINE__);	\
	MakeFile(filename);														\
	}

void QuickFormat()
    {
    FormatFatDrive(TheFs, CurrentDrive(), ETrue);
    }

void ReadBootSector(TFatBootSector& aBootSector)
	{
    TInt nRes = ReadBootSector(TheFs, CurrentDrive(), KBootSectorNum<<KDefaultSectorLog2, aBootSector);
    test_KErrNone(nRes);

    if(!aBootSector.IsValid())
        {
        test.Printf(_L("Wrong bootsector! Dump:\n"));
        aBootSector.PrintDebugInfo();
        test(0);
        }
	}

void GetBootInfo()
	{
	QuickFormat();
	ReadBootSector(gBootSector);
	}

void doDirNameTest(const TDesC& aLongName, const TDesC& aShortName)
	{
	TBuf<KMaxFileName> longDirNamePath;
	TBuf<KMaxFileName> shortDirNamePath;
	TBuf<KMaxFileName> longName;
	TBuf<KMaxFileName> shortName;
	longDirNamePath = gSessionPath;
	longDirNamePath += aLongName;
	longDirNamePath.Append('\\');

	// Create new directory and check creation
	TInt r = TheFs.MkDir(longDirNamePath);
	test_KErrNone(r);
	
	TUint dumUint=0;
	CDir* dumDir;
	r= TheFs.GetDir(longDirNamePath, dumUint, dumUint, dumDir);
	test_KErrNone(r);
	test_NotNull(dumDir);
	delete dumDir;

	// Check short name
	r = TheFs.GetShortName(longDirNamePath, shortName);
	test_KErrNone(r);
	r = shortName.Compare(aShortName);
	test_Value(r, r == 0);
	
	// Check long name
	shortDirNamePath = gSessionPath;
	shortDirNamePath += shortName;
	shortDirNamePath.Append('\\');
	r = TheFs.GetLongName(shortDirNamePath, longName);
	test_KErrNone(r);
	r = longName.Compare(aLongName);
	test_Value(r, r == 0);

	r = TheFs.RmDir(longDirNamePath);
	test_KErrNone(r);
	}

void doFileNameTest(const TDesC& aLongName, const TDesC& aShortName)
	{
	TFileName lgnFullPath;
	TFileName shnFullPath;
	TFileName lgn;
	TFileName shn;

	TInt r = TheFs.SessionPath(gSessionPath);
	test_KErrNone(r);
	lgnFullPath = gSessionPath;
	lgnFullPath += aLongName;
	
	MakeFile_CPTest(lgnFullPath);
	// Check short name	
	r = TheFs.GetShortName(lgnFullPath, shn);
	test_KErrNone(r);
	r = shn.Compare(aShortName);
	test_Value(r, r == 0);

	// Check long name	
	shnFullPath = gSessionPath;
	shnFullPath += aShortName;

	r = TheFs.GetLongName(shnFullPath, lgn);
	test_KErrNone(r);
	r = lgn.Compare(aLongName);
	test_Value(r, r == 0);

	r = TheFs.Delete(lgnFullPath);
	test_KErrNone(r);

	}
#endif //_DEBUG || _DEBUG_RELEASE

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-t_fatcharsetconv-0407
//! @SYMTestType        CIT
//! @SYMDEF             DEF101875
//! @SYMTestCaseDesc    This test case is to test volume label setting with Unicode characters
//! @SYMTestActions     1. Changes locale to load testing codepage dll, check returning error code
//!                     2. Sets volume label with Unicode (less than 11 bytes long), test setting 
//!                        success, gets volume label and compares it with the original Unicode string;
//!                     3. Sets volume label with a Unicode string contains less than 11 characters 
//!                        but occupies bigger than 11 bytes, checks the returning error code;
//!                     4. Compares the volume label with the Unicode string used in action 2
//! @SYMTestExpectedResults
//!                     1. KErrNone should be returned;
//!                     2. The comparison should return 0;
//!                     3. KErrBadName should be returned;
//!                     4. The comparison should return 0;
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void TestUnicodeVolumeLabel()
	{
	test.Next(_L("Test unicode volume labels"));
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)

	TInt r = TheFs.SessionPath(gSessionPath);
	test_KErrNone(r);
	TInt driveNum;
	r = TheFs.CharToDrive(gSessionPath[0], driveNum);
	test_KErrNone(r);

	// Retrieves the original volume label
	TVolumeInfo vInfo;
	r = TheFs.Volume(vInfo, driveNum);
	const TInt KVolumeLabelSize = 11;
	TBuf<KVolumeLabelSize> originalVolumeLabel(vInfo.iName);

	// Tests setting volume label with unicode characters
	_LIT(KUnicodeVolumeLabel, 		"\x65B0\x65B0\x65B0");

	r = TheFs.SetVolumeLabel(KUnicodeVolumeLabel, driveNum);
	test_KErrNone(r);
	r = TheFs.Volume(vInfo, driveNum);
	test_KErrNone(r);
	r = vInfo.iName.Compare(KUnicodeVolumeLabel);
	test_KErrNone(r);

	// Tests setting volume label with unicode characters that bigger than 11 bytes
	_LIT(KVolumeLabelOverflow,		"\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0");
	r = TheFs.SetVolumeLabel(KVolumeLabelOverflow, driveNum);
	test_Value(r, r == KErrOverflow);

	// Sets back the original volume label
	r = TheFs.SetVolumeLabel(originalVolumeLabel, driveNum);
	test_KErrNone(r);
	r = TheFs.Volume(vInfo, driveNum);
	test_KErrNone(r);
	r = vInfo.iName.Compare(originalVolumeLabel);
	test_Value(r, r == 0);
#else
	test.Printf(_L("Test only runs on DEBUG builds, see test logs of debug builds for details."));
#endif  // _DEBUG) || _DEBUG_RELEASE
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-t_fatcharsetconv-0408
//! @SYMTestType        CIT
//! @SYMDEF             DEF101875
//! @SYMTestCaseDesc    This test case is to test the "8 bytes" boundary of short name length, 
//!                     which is defined in FAT Specification.
//! @SYMTestActions     1. Creates files and directories with different Unicode strings that 
//!                        have different combinations of length (of characters) and size (of 
//!                        bytes occupied), check for error of creation.
//!                     2. Gets their short name, compares with expected values, gets their long 
//!                        name, compares with original Unicode strings.
//!                        The strings and short name tested are as below:
//!                        2.1. original string: "Abc"
//!                             expected short name: "ABC";
//!                        2.2. original string: "\x65B0\x6587\x4EF6\x4EF6"
//!                             expected short name: "\x65B0\x6587\x4EF6\x4EF6";
//!                        2.3. original string: "\x65B0\x6587\x4EF6(01)"
//!                             expected short name: "\x65B0\x6587\x4EF6~1";
//!                        2.4. original string: "Abcdefghi"
//!                             expected short name: "ABCDEF~1";
//!                        2.5. original string: "\x65B0(Abcdefgh)"
//!                             expected short name: "\x65B0(ABC~1";
//! @SYMTestExpectedResults
//!						1. File should be created with no error;
//!                     2. Comparisons should return 0;
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void TestShortNameBoundary()
	{
	test.Next(_L("8 bytes' name boundary tests"));
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)

	// File names will be used for testing boundaries
	_LIT(KTestFileName3C3B, 		"Abc");						// 3 characters, 3 bytes long
	_LIT(KTestFileName3C3B_short,	"ABC");						// Expected short name
	_LIT(KTestFileName4C8B, 		"\x65B0\x6587\x4EF6\x4EF6");// 4 characters, 8 bytes long
	_LIT(KTestFileName4C8B_short,	"\x65B0\x6587\x4EF6\x4EF6");// Expected short name
	_LIT(KTestFileName7C10B, 		"\x65B0\x6587\x4EF6(01)");	// 7 characters, 10 bytes long
	_LIT(KTestFileName7C10B_short, 	"\x65B0\x6587\x4EF6~1");	// Expected short name
	_LIT(KTestFileName9C9B, 		"Abcdefghi");				// 9 characters, 9 bytes long
	_LIT(KTestFileName9C9B_short, 	"ABCDEF~1");				// Expected short name
	_LIT(KTestFileName9C10B, 		"\x65B0(Abcdefgh)");		// 9 characters, 10 bytes long
	_LIT(KTestFileName9C10B_short, 	"\x65B0(ABC~1");			// Expected short name

	// Test file creation and long/short name generation
	doFileNameTest(KTestFileName3C3B, 		KTestFileName3C3B_short); 
	doFileNameTest(KTestFileName4C8B, 		KTestFileName4C8B_short);
	doFileNameTest(KTestFileName7C10B, 		KTestFileName7C10B_short);
	doFileNameTest(KTestFileName9C9B, 		KTestFileName9C9B_short);
	doFileNameTest(KTestFileName9C10B, 		KTestFileName9C10B_short);

	doDirNameTest(KTestFileName3C3B, 		KTestFileName3C3B_short);
	doDirNameTest(KTestFileName4C8B, 		KTestFileName4C8B_short);
	doDirNameTest(KTestFileName7C10B, 		KTestFileName7C10B_short);
	doDirNameTest(KTestFileName9C9B, 		KTestFileName9C9B_short);
	doDirNameTest(KTestFileName9C10B, 		KTestFileName9C10B_short);

#else
	test.Printf(_L("Test only runs on DEBUG builds, see test logs of debug builds for details."));
#endif  // _DEBUG) || _DEBUG_RELEASE
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-t_fatcharsetconv-0794
//!	@SYMTestCaseDesc    This test case is to test the extensions of the short names generated are consistent for file names
//!                     consisting of UNICODE characters. In a shortname, if the character at the third location (in 8.3 format)
//!						is a head byte of a UNICODE character then it should be ignored.
//! @SYMTestActions     1. Creates files and directories with different Unicode strings that 
//!                        have different combinations of extension length (of characters) and size (of 
//!                        bytes occupied), check for error of creation.
//!                     2. Gets their short name, compares with expected values, gets their long 
//!                        name, compares with original Unicode strings.
//!                        The strings and short name tested are as below:
//!                        2.1. original string: "abcdef.\x65B0"
//!                             expected short name: "ABCDEF.\x65B0";
//!                        2.2. original string: "abcdef.t\x65B0"
//!                             expected short name: "ABCDEF.T\x65B0";
//!                        2.3. original string: "abcdef.\x65B0t"
//!                             expected short name: "ABCDEF.\x65B0T";
//!                        2.4. original string: "abcdefg.\x65B0\x65B0"
//!                             expected short name: "ABCDEF~1.\x65B0";
//!                        
//! @SYMTestExpectedResults
//!						1. File should be created with no error;
//!                     2. Comparisons should return 0;
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------

void TestConsistentShortNameExtGeneration()
	{
	test.Next(_L("Test consistent short name extensions are generated"));
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	
	// File names will be used for testing boundaries
	_LIT(KTestFileNameExt1C2B, 			"abcdefg.\x65B0");		// 1 characters, 2 bytes long ;only one Unicode character in the extension
	_LIT(KTestFileNameExt1C2B_short,	"ABCDEFG.\x65B0");		//Expected short name					
	_LIT(KTestFileNameExt2C3B2U, 		"abcdefg.t\x65B0");		// 2 characters, 3 bytes long ;Unicode character at the 2nd location of the extension
	_LIT(KTestFileNameExt2C3B2U_short, 	"ABCDEFG.T\x65B0");		//Expected short name
	_LIT(KTestFileNameExt2C3B1U, 		"abcdefg.\x65B0t");		// 2 characters, 3 bytes long ;Unicode character at the 1st location of the extension
	_LIT(KTestFileNameExt2C3B1U_short,	"ABCDEFG.\x65B0T");		//Expected short name
	_LIT(KTestFileNameExt2C4B, 			"abcdefg.\x65B0\x65B0");// 2 characters, 4 bytes long ;both are Unicode characters in the extension
	_LIT(KTestFileNameExt2C4B_short,	"ABCDEF~1.\x65B0");		//Expected short name				
					

	// Test file creation and long/short name generation
	doFileNameTest(KTestFileNameExt1C2B, 	KTestFileNameExt1C2B_short);
	doFileNameTest(KTestFileNameExt2C3B2U, 	KTestFileNameExt2C3B2U_short);
	doFileNameTest(KTestFileNameExt2C3B1U, 	KTestFileNameExt2C3B1U_short);
	doFileNameTest(KTestFileNameExt2C4B, 	KTestFileNameExt2C4B_short);
	
	doDirNameTest(KTestFileNameExt1C2B, 	KTestFileNameExt1C2B_short);
	doDirNameTest(KTestFileNameExt2C3B2U, 	KTestFileNameExt2C3B2U_short);
	doDirNameTest(KTestFileNameExt2C3B1U, 	KTestFileNameExt2C3B1U_short);
	doDirNameTest(KTestFileNameExt2C4B, 	KTestFileNameExt2C4B_short);
#else
	test.Printf(_L("Test only runs on DEBUG builds, see test logs of debug builds for details."));
#endif  // _DEBUG) || _DEBUG_RELEASE
	}


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID		PBASE-t_fatcharsetconv-0795
//! @SYMTestCaseDesc    This test case is to test whether the short names generated for file names
//!                     consisting of UNICODE characters are consistent or not. In a shortname, if the character immediately
//!						preceding the tilde(~) is a head byte of a UNICODE character then it should be ignored.
//! @SYMTestActions     1. Creates files and directories with different Unicode strings that 
//!                        have different combinations of length (of characters) and size (of 
//!                        bytes occupied), check for error of creation.
//!                     2. Gets their short name, compares with expected values, gets their long 
//!                        name, compares with original Unicode strings.
//!                        The strings and short name tested are as below:
//!                        2.1. original string: "a\x65B0(bcd)"
//!                             expected short name: "A\x65B0(BCD)";
//!                        2.2. original string: "ab\x65B0(cdef)"
//!                             expected short name: "AB\x65B0(C~1")";
//!                        2.3. original string: "abc\x65B0(def)"
//!                             expected short name: "ABC\x65B0(~1";
//!                        2.4. original string: "abcd\x65B0(ef)"
//!                             expected short name: "ABCD\x65B0~1";
//!                        2.5. original string: "abcde\x65B0(f)"
//!                             expected short name: "ABCDE~1";
//! @SYMTestExpectedResults
//!						1. File should be created with no error;
//!                     2. Comparisons should return 0;
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void TestConsistentShortNameGeneration()
	{
	test.Next(_L("Test consistent short name generation"));
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)

	//unicode characters.
	
	_LIT(KTestFileName7C8B2U, 		"a\x65B0(bcd)");			// 7 characters, 8 bytes long ,Unicode character at the 2nd location.
	_LIT(KTestFileName7C8B2U_short, "A\x65B0(BCD)");			// Expected short name
	_LIT(KTestFileName9C10B3U, 		"ab\x65B0(cdef)");			// 9 characters, 10 bytes long ,Unicode character at the 3rd location.
	_LIT(KTestFileName9C10B3U_short,"AB\x65B0(C~1");			// Expected short name
	_LIT(KTestFileName9C10B4U, 		"abc\x65B0(def)");			// 9 characters, 10 bytes long ,Unicode character at the 4th location.
	_LIT(KTestFileName9C10B4U_short,"ABC\x65B0(~1");			// Expected short name
	_LIT(KTestFileName9C10B5U, 		"abcd\x65B0(ef)");			// 9 characters, 10 bytes long ,Unicode character at the 6th location.
	_LIT(KTestFileName9C10B5U_short,"ABCD\x65B0~1");			// Expected short name
	_LIT(KTestFileName9C10B6U, 		"abcde\x65B0(f)");			// 9 characters, 10 bytes long ,repeat Unicode character at the 6th location.
	_LIT(KTestFileName9C10B6U_short,"ABCDE~1");					// Expected short name
		
	// Test file creation and long/short name generation
	doFileNameTest(KTestFileName7C8B2U, 		KTestFileName7C8B2U_short);
	doFileNameTest(KTestFileName9C10B3U, 		KTestFileName9C10B3U_short);
	doFileNameTest(KTestFileName9C10B4U, 		KTestFileName9C10B4U_short);
	doFileNameTest(KTestFileName9C10B5U, 		KTestFileName9C10B5U_short);
	doFileNameTest(KTestFileName9C10B6U, 		KTestFileName9C10B6U_short);
		 
	doDirNameTest(KTestFileName7C8B2U, 			KTestFileName7C8B2U_short);
	doDirNameTest(KTestFileName9C10B3U, 		KTestFileName9C10B3U_short);
	doDirNameTest(KTestFileName9C10B4U, 		KTestFileName9C10B4U_short);
	doDirNameTest(KTestFileName9C10B5U, 		KTestFileName9C10B5U_short);
	doDirNameTest(KTestFileName9C10B6U, 		KTestFileName9C10B6U_short);
#else
	test.Printf(_L("Test only runs on DEBUG builds, see test logs of debug builds for details."));
#endif  // _DEBUG) || _DEBUG_RELEASE
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-t_fatcharsetconv-0409
//! @SYMTestType        CIT
//! @SYMDEF             DEF101875 INC100580
//! @SYMTestCaseDesc    This test case is to test creation of files with their Unicode names are 
//!                     duplicate to existing files.
//! @SYMTestActions     1. Creates a file with its name contains 4 Unicode characters but occupies 
//!                        8 bytes ("\x65B0\x6587\x4EF6\x4EF6"), check for creation;
//!                     2. Creates a file with 11 bytes' long Unicode name and first 8 bytes are 
//!                        identical with the file created in Action 1 ("\x65B0\x6587\x4EF6\x4EF6(A)"), 
//!                        check for file creation;
//!                     3. Gets the short name of the file created in Action 2, compares it with 
//!                        expected short name ("\x65B0\x6587\x4EF6~1"), gets the long name of this 
//!                        file and compares it with its original Unicode name;
//!                     4. Creates a file with 12 bytes' long Unicode name and first 8 bytes are 
//!                        identical with the file created in Action 1 and Action 2 
//!                        ("\x65B0\x6587\x4EF6\x4EF6(AB)"), check for file creation;
//!                     5. Gets the short name of the file created in Action 4, compares it with 
//!                        expected values ("\x65B0\x6587\x4EF6~2"), gets the long name of this file 
//!                        and compares it with its original Unicode name;
//! @SYMTestExpectedResults
//!						1. File creation should return KErrNone;
//!                     2. File creation should return KErrNone;
//!                     3. Comparisons should return 0;
//!                     4. File creation should return KErrNone;
//!                     5. Comparisons should return 0;
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void TestDuplicateLongFileNames()
	{
	test.Next(_L("Testing tilde and numbers (\"~n\") are applied correctly for multiple long-named files"));
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)

	// These are to test "~1", "~2" behaviours when the first 8 bytes of new files
	//  are identical with existing files
	_LIT(KTestFileName4C8B, 			"\x65B0\x6587\x4EF6\x4EF6.TXT");
	_LIT(KTestFileName7C11B, 			"\x65B0\x6587\x4EF6\x4EF6(A).TXT");	
	_LIT(KTestFileName7C11B_short, 		"\x65B0\x6587\x4EF6~1.TXT");
	_LIT(KTestFileName8C12B, 			"\x65B0\x6587\x4EF6\x4EF6(AB).TXT");	
	_LIT(KTestFileName8C12B_short, 		"\x65B0\x6587\x4EF6~2.TXT");	

	////////////////////////////////////////
	// 1. Test duplicate long file names
	////////////////////////////////////////
	TFileName sn;
	TInt r;	
	MakeFile_CPTest(KTestFileName4C8B);
	MakeFile_CPTest(KTestFileName7C11B);
	r = TheFs.GetShortName(KTestFileName7C11B, sn);
	test_KErrNone(r);
	r = sn.Compare(KTestFileName7C11B_short);
	test_Value(r, r == 0);


	MakeFile_CPTest(KTestFileName8C12B);
	r = TheFs.GetShortName(KTestFileName8C12B, sn);
	test_KErrNone(r);
	r = sn.Compare(KTestFileName8C12B_short);
	test_Value(r, r == 0);

	r = TheFs.Delete(KTestFileName4C8B);
	test_KErrNone(r);
		
	r = TheFs.Delete(KTestFileName7C11B);
	test_KErrNone(r);
		
	r = TheFs.Delete(KTestFileName8C12B);
	test_KErrNone(r);

#else
	test.Printf(_L("Test only runs on DEBUG builds, see test logs of debug builds for details."));
#endif  // _DEBUG) || _DEBUG_RELEASE
	}


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-t_fatcharsetconv-0410
//! @SYMTestType        CIT
//! @SYMDEF             DEF101875 INC100580
//! @SYMTestCaseDesc    This test case is to test creation of directories with their Unicode names are 
//!                     duplicate to existing directories .
//! @SYMTestActions     1. Creates a directories  with its name contains 4 Unicode characters but occupies 
//!                        8 bytes ("\x65B0\x6587\x4EF6\x4EF6"), check for creation;
//!                     2. Creates a directories  with 11 bytes' long Unicode name and first 8 bytes are 
//!                        identical with the directories created in Action 1 ("\x65B0\x6587\x4EF6\x4EF6(A)"), 
//!                        check for file creation;
//!                     3. Gets the short name of the directories created in Action 2, compares it with 
//!                        expected short name ("\x65B0\x6587\x4EF6~1"), gets the long name of this 
//!                        directories and compares it with its original Unicode name;
//!                     4. Creates a directories with 12 bytes' long Unicode name and first 8 bytes are 
//!                        identical with the directories created in Action 1 and Action 2 
//!                        ("\x65B0\x6587\x4EF6\x4EF6(AB)"), check for directories creation;
//!                     5. Gets the short name of the directories created in Action 4, compares it with 
//!                        expected values ("\x65B0\x6587\x4EF6~2"), gets the long name of this directories 
//!                        and compares it with its original Unicode name;
//! @SYMTestExpectedResults
//!						1. Dir creation should return KErrNone;
//!                     2. Dir creation should return KErrNone;
//!                     3. Comparisons should return 0;
//!                     4. Dir creation should return KErrNone;
//!                     5. Comparisons should return 0;
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void TestDuplicateLongDirNames()
	{
	test.Next(_L("Testing tilde and number appended correctly for duplicate long name dirs"));
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	TheFs.SessionPath(gSessionPath);

	// These are to test "~1", "~2" behaviours when the first 8 bytes of new directories
	//  are identical with existing directories
	_LIT(KTestDirName4C8B, 				"\\F32-TST\\T_CODEPAGE_PLUGIN\\\x65B0\x6587\x4EF6\x4EF6\\");
	_LIT(KTestDirName7C11B, 			"\\F32-TST\\T_CODEPAGE_PLUGIN\\\x65B0\x6587\x4EF6\x4EF6(A)\\");	
	_LIT(KTestDirName7C11B_short, 		"\x65B0\x6587\x4EF6~1");	
	_LIT(KTestDirName8C12B, 			"\\F32-TST\\T_CODEPAGE_PLUGIN\\\x65B0\x6587\x4EF6\x4EF6(AB)\\");	
	_LIT(KTestDirName8C12B_short, 		"\x65B0\x6587\x4EF6~2");	

	// Create 1st file with 8 bytes long/short name
	TInt r;
	MakeDir(KTestDirName4C8B);
	MakeDir(KTestDirName7C11B);
	
	TFileName sn;
	
	r = TheFs.GetShortName(KTestDirName7C11B, sn);
	test_KErrNone(r);
	r = sn.Compare(KTestDirName7C11B_short);
	test_Value(r, r == 0);

	MakeDir(KTestDirName8C12B);
	r = TheFs.GetShortName(KTestDirName8C12B, sn);
	test_KErrNone(r);
	r = sn.Compare(KTestDirName8C12B_short);
	test_Value(r, r == 0);

#else
	test.Printf(_L("Test only runs on DEBUG builds, see test logs of debug builds for details."));
#endif  // _DEBUG) || _DEBUG_RELEASE
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-t_fatcharsetconv-0791
//! @SYMTestType        CIT
//! @SYMDEF             DEF117345
//! @SYMTestCaseDesc    This test case is to test short name with 'E5' as the leading byte is correctly 
//!						handled on FAT implementations 
//! @SYMTestActions     1. Creates a file with unicode long name "\x88F9.TXT", it will be converted into
//!							"\xE5E5.TXT" according to codepage 932 when creating short name.
//!                     2. Gets the short name and compare it with its original unicode "\x88F9.TXT", make
//!							sure the conversion "\xE5E5.TXT" has been handled correctly  
//! @SYMTestExpectedResults
//!                     1. Comparisons should return 0;
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void TestLeadingE5Handling()
	{
	test.Next(_L("Test Leading \'E5\' byte handling (DEF117345)"));
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	TheFs.SessionPath(gSessionPath);

	_LIT(KTestFilePathAndName,		"\\F32-TST\\T_CODEPAGE_PLUGIN\\\x88F9.TXT");
	_LIT(KTestFileShortName, 		"\x88F9.TXT");

	TInt r;
	MakeFile_CPTest(KTestFilePathAndName);
	TFileName sn;
	r = TheFs.GetShortName(KTestFilePathAndName, sn);
	test_KErrNone(r);
	r = sn.Compare(KTestFileShortName);
	test_Value(r, r == 0);

#else
	test.Printf(_L("Test only runs on DEBUG builds, see test logs of debug builds for details."));
#endif  // _DEBUG) || _DEBUG_RELEASE
	}
//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-t_fatcharsetconv-2320
//! @SYMTestType        CIT
//! @SYMDEF             PDEF130334
//! @SYMTestCaseDesc    This test case is to test creating a file with "\u3005" name correctly 
//! @SYMTestActions     1. Creates a file with unicode long name "\u3005.TXT"
//!                     2. Gets the short name and compare it with its original unicode "\u3005.TXT"
//!						3. Gets the long name and compare it with its original unicode "\u3005.TXT"
//! @SYMTestExpectedResults
//!                     1. Comparisons should return 0;
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void TestDEF130334()
	{
	test.Next(_L("Test creating a file with \\x3005 name correctly (DEF128521)"));
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	TheFs.SessionPath(gSessionPath);

	_LIT(KTestFilePathAndName,		"\\F32-TST\\T_CODEPAGE_PLUGIN\\\x3005.TXT");
	_LIT(KTestFileName, 		"\x3005.TXT");

	TInt r;
	MakeFile_CPTest(KTestFilePathAndName);
	
	TFileName sn;
	r = TheFs.GetShortName(KTestFilePathAndName, sn);
	test_KErrNone(r);
	r = sn.Compare(KTestFileName);
	test_Value(r, r == 0);
	TFileName ln;
	r = TheFs.GetLongName(KTestFilePathAndName, ln);
	test_KErrNone(r);
	r = ln.Compare(KTestFileName);
	test_Value(r, r == 0);

#else
	test.Printf(_L("Test only runs on DEBUG builds, see test logs of debug builds for details."));
#endif  
	}
//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-t_fatcharsetconv-1359
//! @SYMTestType        CIT
//! @SYMDEF             INC125768
//! @SYMTestCaseDesc    This test case is to test the compatibility of file opening, to make sure files
//!						with only one DOS entry which contains unicode short name is accessible on Symbian OS
//!						from version 9.3 onwards.
//! @SYMTestActions     Manually creates a single entried, unicode named file under root direcotry, then
//!							access it via RFs::Entry() API using its uniocde name. Check the entry is accessible.
//! @SYMTestExpectedResults
//!						RFs::Entry() should return with KErrNone;
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void TestCompatibility()
	{
	test.Next(_L("test file opening compatibility"));
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	GetBootInfo();

	RFile file;
	TFileName fn = _L("\\ABCD");
	
	TInt r=file.Create(TheFs,fn,EFileRead);
	test_KErrNone(r);
	file.Close();

	//	Assume this file is the first entry in the root directory
	r=TheDisk.Open(TheFs,CurrentDrive());
	test_KErrNone(r);
	
    //-- read the 1st dir entry, it should be a DOS entry 
    const TInt posEntry1=gBootSector.RootDirStartSector() << KDefaultSectorLog2; //-- dir entry1 position
    
    TFatDirEntry fatEntry1;
	TPtr8 ptrEntry1((TUint8*)&fatEntry1,sizeof(TFatDirEntry));
    test(TheDisk.Read(posEntry1, ptrEntry1)==KErrNone); 
    test(!fatEntry1.IsVFatEntry());

    // Manually modify the short name into unicode characters
    // 	Unicode: 	0x(798F 5C71 96C5 6CBB)
    //	Shift-JIS: 	0x(959F 8E52 89EB 8EA1)

    TBuf8<8> unicodeSN = _L8("ABCD1234");
    unicodeSN[0] = 0x95;
    unicodeSN[1] = 0x9F;
    unicodeSN[2] = 0x8E;
    unicodeSN[3] = 0x52;
    unicodeSN[4] = 0x89;
    unicodeSN[5] = 0xEB;
    unicodeSN[6] = 0x8E;
    unicodeSN[7] = 0xA1;
    
    fatEntry1.SetName(unicodeSN);
    test(TheDisk.Write(posEntry1, ptrEntry1)==KErrNone);
    TheDisk.Close();

	fn = _L("\\ABCD");
	fn[1] = 0x798F;
	fn[2] = 0x5C71;
	fn[3] = 0x96C5;
	fn[4] = 0x6CBB;
	
	TEntry entry;
	TInt err = TheFs.Entry(fn, entry);
	test_KErrNone(err);
	err = TheFs.Delete(fn);
	test_KErrNone(err);
#else
	test.Printf(_L("Test only runs on DEBUG builds, see test logs of debug builds for details."));
#endif  // _DEBUG) || _DEBUG_RELEASE
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-t_fatcharsetconv-1395
//! @SYMTestType        CIT
//! @SYMDEF             INC126563
//! @SYMTestCaseDesc    This test case is to test the definition of valid DOS characters on Symbian 
//!						FAT/FAT32 complies with FAT Spec.
//! @SYMTestActions     Manually creates a file with "0x7F" characters in its name, then check it can
//!						ben accessed successfully.
//! @SYMTestExpectedResults
//!						RFs::Entry() should return with KErrNone;
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void TestINC126563()
	{
	test.Next(_L("Test INC126563: FAT/FAT32: unable to open or delete file whose name contains illegal characters"));
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	GetBootInfo();

	RFile file;
	TFileName fn = _L("\\AB");
	
	test.Next(_L("create file \"AB\" under root directory"));
	TInt r=file.Create(TheFs,fn,EFileRead);
	test_KErrNone(r);
	file.Close();

	test.Next(_L("manually change file name to \"0x7F0x450x7F0x45\" via raw disk accessing"));
	//	Assume this file is the first entry in the root directory
	r=TheDisk.Open(TheFs,CurrentDrive());
	test_KErrNone(r);
	
    //-- read the first dir entry, it should be a DOS entry 
    const TInt posEntry1=gBootSector.RootDirStartSector() << KDefaultSectorLog2; //-- dir entry1 position
    
    TFatDirEntry fatEntry1;
	TPtr8 ptrEntry1((TUint8*)&fatEntry1,sizeof(TFatDirEntry));
    test(TheDisk.Read(posEntry1, ptrEntry1)==KErrNone); 
    test(!fatEntry1.IsVFatEntry());

    TBuf8<8> unicodeSN = _L8("ABCD");
    unicodeSN[0] = 0x7F;
    unicodeSN[1] = 0x45;
    unicodeSN[2] = 0x7F;
    unicodeSN[3] = 0x45;
    
    fatEntry1.SetName(unicodeSN);
    test(TheDisk.Write(posEntry1, ptrEntry1)==KErrNone);
    TheDisk.Close();

 	test.Next(_L("access entries under root directory via RDir::Open()"));
	RDir dir;
	r = dir.Open(TheFs, _L("\\"), KEntryAttNormal);
	test(KErrNone == r);
	TEntryArray entryArray;
	r = dir.Read(entryArray);
	test(entryArray.Count()==1);
	TBuf<0x10> name;
	TEntry entry;
	entry = entryArray[0];
	test.Printf(_L("entryArray[0] = \"%S\"\n"), &entry.iName);
	name = entry.iName;
	dir.Close();
	
	TFileName fullname= _L("\\");
	fullname.Append(name);
	
	test.Next(_L("try to open or delete file entries retrieved"));
	r = file.Open(TheFs, fullname, EFileRead);
	test(KErrNone == r);
	file.Close();
	
	r = TheFs.Delete(fullname);
	test(KErrNone == r);
#else
	test.Printf(_L("Test only runs on DEBUG builds, see test logs of debug builds for details."));
#endif  // _DEBUG) || _DEBUG_RELEASE
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-t_fatcharsetconv-1402
//! @SYMTestType        CIT
//! @SYMDEF             INC127905
//! @SYMTestCaseDesc    This test case is to test RFs::ScanDrive() does not incorrectly remove files
//!						with unicode short file names.
//! @SYMTestActions     Creates a file with unicode file names then check if the file is still accessible
//!						after scandrive operations.
//! @SYMTestExpectedResults
//!						RFs::Delete() should return with KErrNone;
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void TestINC127905()
	{
	test.Next(_L("Test INC127905: Unicode name file deleted after Scandrive"));
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	TFileName fn = _L("ABCDE");
	fn[0] = 0x3055;
	fn[1] = 0x307E;
	fn[2] = 0x3056;
	fn[3] = 0x307E;
	fn[4] = 0x306A;


	TInt r;
	MakeFile_CPTest(fn);
	
	_LIT(KShortName, "\x3055\x307E\x3056~1");
	TFileName sn;
	r = TheFs.GetShortName(fn, sn);
	test_KErrNone(r);
	r = sn.Compare(KShortName);
	test_Value(r, r == 0);
	
	r = TheFs.ScanDrive(_L("gSessionPath"));
	test_KErrNone(r);

	r = TheFs.Delete(fn);
	test_KErrNone(r);
#else
	test.Printf(_L("Test only runs on DEBUG builds, see test logs of debug builds for details."));
#endif  // _DEBUG) || _DEBUG_RELEASE
	}

