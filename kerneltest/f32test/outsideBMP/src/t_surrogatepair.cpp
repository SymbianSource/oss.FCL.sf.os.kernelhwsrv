// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test/outsidebmp/t_surrogatepair.cpp
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <hal.h>
#include <f32fsys.h>
#include <f32dbg.h>
#include "../server/t_server.h"

RTest test(_L("T_SURROGATEPAIR"));

/* 
 * Helper function to capture and return any MakeFile error code.
 */
TInt SurrogatePair_MakeFile(const TDesC& aFileName)
	{
	RFile file;
	TInt r=file.Replace(TheFs,aFileName,0);
	file.Close();
	return r;
	}

/*
 *  Tests given ShortName against LongName and vice versa for a File Entry.
 */
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

	MakeFile(lgnFullPath);

	// Check short name	
	r = TheFs.GetShortName(lgnFullPath, shn);
	test_KErrNone(r);
	r = shn.Compare(aShortName);
	test(r==0);

	// Check long name	
	shnFullPath = gSessionPath;
	shnFullPath += aShortName;

	r = TheFs.GetLongName(shnFullPath, lgn);
	test_KErrNone(r);
	r = lgn.Compare(aLongName);
	test(r==0);

	test_KErrNone(TheFs.Delete(lgnFullPath));
	}

/*
 *  Tests given ShortName against LongName and vice versa for a Directory Entry.
 */
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
	test(r==0);
	
	// Check long name
	shortDirNamePath = gSessionPath;
	shortDirNamePath += shortName;
	shortDirNamePath.Append('\\');
	r = TheFs.GetLongName(shortDirNamePath, longName);
	test_KErrNone(r);
	r = longName.Compare(aLongName);
	test(r==0);

	r = TheFs.RmDir(longDirNamePath);
	test_KErrNone(r);
	}

/*
 *  Tests File entry and Dir entry.
 */
void TestConversion(const TDesC& aLongName, const TDesC& aShortName)
	{
	doFileNameTest(aLongName, aShortName);
	doDirNameTest(aLongName, aShortName);
	}

/*
 * Tests different characters for its validity and conversion in absence of cp dll.
 * Expects ReplacementForUnconvertibleUnicodeCharacters : "_" : UNDERSCORE for all
 * Illegal Characters
 */
void TestShortNameCharacter()
	{
	test.Next(_L("++TestShortNameCharacter"));
	TInt r;
	
	// one-byte chars
	_LIT16(Uni_1, 									"\x0019.TXT");
	_LIT16(Uni_1_ShortName, 						"_.TXT");
	TestConversion( Uni_1, Uni_1_ShortName);
	
	_LIT16(Uni_2, 									"\x0020.TXT");
	_LIT(Uni_2_ShortName, 							"_.TXT");
	TestConversion( Uni_2, Uni_2_ShortName);
	
	_LIT16(Uni_3, 									"\x0021.TXT");
	_LIT16(Uni_3_ShortName, 						"\x0021.TXT");
	TestConversion( Uni_3, Uni_3_ShortName);

 	_LIT16(Uni_4, 									"\x0079"); // y lower case 
	_LIT16(Uni_4_ShortName, 						"\x0059");	// Y upper case
	TestConversion( Uni_4, Uni_4_ShortName);

	_LIT16(Uni_5,									"\x0080");
	_LIT16(Uni_5_ShortName, 						"_");
	TestConversion( Uni_5, Uni_5_ShortName);

	_LIT16(Uni_6,									"\x0081");
	_LIT16(Uni_6_ShortName, 						"_");
	TestConversion( Uni_6, Uni_6_ShortName);

	_LIT16(Uni_7, 									"\x00fe");
	_LIT16(Uni_7_ShortName, 						"_");
	TestConversion( Uni_7, Uni_7_ShortName);

	_LIT16(Uni_8, 									"\x00ff");
	_LIT16(Uni_8_ShortName, 						"_");
	TestConversion( Uni_8, Uni_8_ShortName);
	
	// two-byte chars
	_LIT16(Uni_9, 									"\x0100");
	_LIT16(Uni_9_ShortName, 						"_");
	TestConversion( Uni_9, Uni_9_ShortName);

	_LIT16(Uni_10, 									"\x0101");
	_LIT16(Uni_10_ShortName, 						"_");
	TestConversion( Uni_10, Uni_10_ShortName);

	_LIT16(Uni_11, 									"\x0ffe");
	_LIT16(Uni_11_ShortName, 						"_");
	TestConversion( Uni_11, Uni_11_ShortName);

	_LIT16(Uni_12, 									"\x0fff");
	_LIT16(Uni_12_ShortName,						"_");
	TestConversion( Uni_12, Uni_12_ShortName);

	_LIT16(Uni_13, 									"\x1000");
	_LIT16(Uni_13_ShortName, 						"_");
	TestConversion( Uni_13, Uni_13_ShortName);

	_LIT16(Uni_14, 									"\x1001");
	_LIT16(Uni_14_ShortName, 						"_");
	TestConversion( Uni_14, Uni_14_ShortName);

	_LIT16(Uni_15, 									"\x2999.TXT");
	_LIT16(Uni_15_ShortName, 						"_.TXT");
	TestConversion( Uni_15, Uni_15_ShortName);

	_LIT16(Uni_16, 									"\x4E02.TXT");	
	_LIT16(Uni_16_ShortName, 						"_.TXT");
	TestConversion( Uni_16, Uni_16_ShortName);

	_LIT16(Uni_17, 									"\x4E02.TXT");	
	_LIT(Uni_17_ShortName, 							"_.TXT");
	TestConversion( Uni_17, Uni_17_ShortName);

	_LIT16(Uni_18, 									"\xfffe");
	_LIT16(Uni_18_ShortName, 						"_");
	TestConversion( Uni_18, Uni_18_ShortName);

	_LIT16(Uni_19, 									"\xffff");
	_LIT16(Uni_19_ShortName, 						"_");
	TestConversion( Uni_19, Uni_19_ShortName);

	// four-byte surrogate pairs
	_LIT16(Uni_20, 									"\xd840\xdc00");
	_LIT16(Uni_20_ShortName, 						"_");
	TestConversion( Uni_20, Uni_20_ShortName);	

	_LIT16(Uni_21, 									"\xd840\xdc01");
	_LIT16(Uni_21_ShortName, 						"_");
	TestConversion( Uni_21, Uni_21_ShortName);	

	_LIT16(Uni_22, 									"\xD87F\xdffe");
	_LIT16(Uni_22_ShortName, 						"_");
	TestConversion( Uni_22, Uni_22_ShortName);	

	_LIT16(Uni_23, 									"\xD87F\xdfff");
	_LIT16(Uni_23_ShortName, 						"_");
	TestConversion( Uni_23, Uni_23_ShortName);	

	//	surrogate pair
	_LIT16(Uni_24, 									"\xd840\xddad");
	_LIT16(Uni_24_ShortName, 						"_");
	TestConversion( Uni_24, Uni_24_ShortName);	
	
	_LIT16(Uni_25, 									"\xd801\xdd00");
	_LIT16(Uni_25_ShortName, 						"_");
	TestConversion( Uni_25, Uni_25_ShortName);

	// corrupt surrogate in file name
	_LIT16(KTest0xD800, 							"\xD800.TXT");
	r = SurrogatePair_MakeFile(KTest0xD800);
	test(r==KErrBadName);

	// corrupt surrogate in file ext
	_LIT16(KTest0xD800XT, 							"\xD684.\xD800XT");
	r = SurrogatePair_MakeFile(KTest0xD800XT);
	test(r==KErrBadName);

	// corrupt surrogate in file ext
	_LIT16(KTestTX0xD800, 							"\xD684.TX\xD800");
	r = SurrogatePair_MakeFile(KTestTX0xD800);
	test(r==KErrBadName);

	// corrupt surrogate in file name
	_LIT16(KTest0xDFFF, 							"\xDFFF.TXT");
	r = SurrogatePair_MakeFile(KTest0xDFFF);
	test(r==KErrBadName);

	// corrupt surrogate in file ext
	_LIT16(KTest0xDFFFXT, 							"\xD684.\xDFFFXT");
	r = SurrogatePair_MakeFile(KTest0xDFFFXT);
	test(r==KErrBadName);

	// corrupt surrogate in file ext
	_LIT16(KTestTX0xDFFF, 							"\xD684.TX\xDFFF");
	r = SurrogatePair_MakeFile(KTestTX0xDFFF);
	test(r==KErrBadName);

	test.Next(_L("--TestShortNameCharacter"));
	}

void TestVolumeLabel()
	{
	test.Next(_L("++TestVolumeLabel"));
	test.Next(_L("Test unicode volume labels"));

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

	// Tests setting volume label with 1 byte characters
	_LIT(K1ByteVolumeLabel, 				"\x0079\x0079\x0079");
	_LIT(K1ByteVolumeLabel_ShortName, 		"\x0079\x0079\x0079");
	
	r = TheFs.SetVolumeLabel(K1ByteVolumeLabel, driveNum);
	test_KErrNone(r);
	r = TheFs.Volume(vInfo, driveNum);
	test_KErrNone(r);
	r = vInfo.iName.Compare(K1ByteVolumeLabel_ShortName);
	test_Equal(r, 0);
	
	// Tests setting volume label with 2 byte characters
	_LIT(K2ByteVolumeLabel, 				"\x65B0\x65B0\x65B0");
	_LIT(K2ByteVolumeLabel_ShortName, 		"___");

	r = TheFs.SetVolumeLabel(K2ByteVolumeLabel, driveNum);
	test_KErrNone(r);
	r = TheFs.Volume(vInfo, driveNum);
	test_KErrNone(r);
	r = vInfo.iName.Compare(K2ByteVolumeLabel_ShortName);
	test_Equal(r, 0);

	// Tests setting volume label with surrogate pair
	_LIT(KTestVolumeLabelSurrogatePair1, 			"\xD846\xDF1D\x0041\x0042");
	_LIT(KTestVolumeLabelSurrogatePair1_ShortName,	"_AB");

	r = TheFs.SetVolumeLabel(KTestVolumeLabelSurrogatePair1, driveNum);
	test_KErrNone(r);
	r = TheFs.Volume(vInfo, driveNum);
	test_KErrNone(r);
	r = vInfo.iName.Compare(KTestVolumeLabelSurrogatePair1_ShortName);
	test_Equal(r, 0);

	_LIT(KTestVolumeLabelSurrogatePair2, 			"\x0041\x0042\xD846\xDF1D");
	_LIT(KTestVolumeLabelSurrogatePair2_ShortName,	"AB_");

	r = TheFs.SetVolumeLabel(KTestVolumeLabelSurrogatePair2, driveNum);
	test_KErrNone(r);
	r = TheFs.Volume(vInfo, driveNum);
	test_KErrNone(r);
	r = vInfo.iName.Compare(KTestVolumeLabelSurrogatePair2_ShortName);
	test_Equal(r, 0);

	// Sets back the original volume label
	r = TheFs.SetVolumeLabel(originalVolumeLabel, driveNum);
	test_KErrNone(r);
	
	test.Next(_L("--TestVolumeLabel"));
	}

void TestConsistentShortNameGeneration()
	{
	test.Next(_L("++TestConsistentShortNameGeneration"));
	test.Next(_L("Test consistent short name generation"));

	//unicode characters.
	_LIT(KTestFileName1,		 				"a\x65B0(bcd)");
	_LIT(KTestFileName1_ShortName,	 			"A_(BCD)");
	_LIT(KTestFileName2, 						"ab\x65B0(cdef)");
	_LIT(KTestFileName2_ShortName,				"AB_(CD~1");
	_LIT(KTestFileName3, 						"abc\x65B0(def)");
	_LIT(KTestFileName3_ShortName,				"ABC_(D~1");
	_LIT(KTestFileName4, 						"abcd\x65B0(ef)");
	_LIT(KTestFileName4_ShortName,				"ABCD_(~1");
	_LIT(KTestFileName5, 						"abcde\x65B0(f)");
	_LIT(KTestFileName5_ShortName,				"ABCDE_~1");
	_LIT(KTestFileNameSurrogatePair1, 			"\x0041\x0308\x006F\xD846\xDF1D\x0042");
	_LIT(KTestFileNameSurrogatePair1_ShortName,	"A_O_B");
	_LIT(KTestFileNameSurrogatePair2, 			"\xD846\xDF1D\x0041\x0042");
	_LIT(KTestFileNameSurrogatePair2_ShortName,	"_AB");
	_LIT(KTestFileNameSurrogatePair3, 			"\x0041\x0042\xD846\xDF1D");
	_LIT(KTestFileNameSurrogatePair3_ShortName,	"AB_");

	// Test file creation and long/short name generation
	TestConversion(KTestFileName1,				KTestFileName1_ShortName);
	TestConversion(KTestFileName2, 				KTestFileName2_ShortName);
	TestConversion(KTestFileName3, 				KTestFileName3_ShortName);
	TestConversion(KTestFileName4, 				KTestFileName4_ShortName);
	TestConversion(KTestFileName5, 				KTestFileName5_ShortName);
	TestConversion(KTestFileNameSurrogatePair1,	KTestFileNameSurrogatePair1_ShortName);
	TestConversion(KTestFileNameSurrogatePair2,	KTestFileNameSurrogatePair2_ShortName);
	TestConversion(KTestFileNameSurrogatePair3,	KTestFileNameSurrogatePair3_ShortName);

	test.Next(_L("--TestConsistentShortNameGeneration"));
	}

void TestConsistentShortNameExtGeneration()
	{
	test.Next(_L("++TestConsistentShortNameExtGeneration"));
	test.Next(_L("Test consistent short name extensions are generated"));
	
	// File names will be used for testing boundaries
	_LIT(KTestFileNameExt1, 						"abcdefg.\xFFFF");
	_LIT(KTestFileNameExt1_ShortName,				"ABCDEFG._");
	_LIT(KTestFileNameExt2, 						"abcdefg.t_");
	_LIT(KTestFileNameExt2_ShortName, 				"ABCDEFG.T_");
	_LIT(KTestFileNameExt3, 						"abcdefg.\xFFFFt");
	_LIT(KTestFileNameExt3_ShortName,				"ABCDEFG._T");
	_LIT(KTestFileNameExt4, 						"abcdefg.\xFFFF\xFFFF");
	_LIT(KTestFileNameExt4_ShortName,				"ABCDEFG.__");
	_LIT(KTestFileNameExtSurrogatePair1, 			"ABCDEFG.\xD846\xDF1D\x0041");
	_LIT(KTestFileNameExtSurrogatePair1_ShortName,	"ABCDEFG._A");
	_LIT(KTestFileNameExtSurrogatePair2, 			"ABCDEFG.\x0041\xD846\xDF1D");
	_LIT(KTestFileNameExtSurrogatePair2_ShortName,	"ABCDEFG.A_");

	// Test file creation and long/short name generation
	TestConversion(KTestFileNameExt1, 				KTestFileNameExt1_ShortName);
	TestConversion(KTestFileNameExt2, 				KTestFileNameExt2_ShortName);
	TestConversion(KTestFileNameExt3, 				KTestFileNameExt3_ShortName);
	TestConversion(KTestFileNameExt4, 				KTestFileNameExt4_ShortName);
	TestConversion(KTestFileNameExtSurrogatePair1, 	KTestFileNameExtSurrogatePair1_ShortName);
	TestConversion(KTestFileNameExtSurrogatePair2, 	KTestFileNameExtSurrogatePair2_ShortName);

	test.Next(_L("--TestConsistentShortNameExtGeneration"));
	}

void TestDuplicateLongFileNames()
	{
	test.Next(_L("++TestDuplicateLongFileNames"));
	test.Next(_L("Testing tilde and numbers (\"~n\") are applied correctly for multiple long-named files"));

	// These are to test "~1", "~2" behaviours when the first 8 bytes of new files
	// are identical with existing files
	_LIT(KTestFileName1, 							"ABCD\xFFFE(A).TXT");
	_LIT(KTestFileName1_ShortName, 					"ABCD_(A).TXT");
	_LIT(KTestFileName2, 							"ABCD\xFFFE(AB).TXT");
	_LIT(KTestFileName2_ShortName, 					"ABCD_(~1.TXT");
	_LIT(KTestFileName3, 							"ABCD\xFFFE(ABC).TXT");
	_LIT(KTestFileName3_ShortName, 					"ABCD_(~2.TXT");
	_LIT(KTestFileNameSurrogatePair1, 				"ABCD\xD846\xDF1D(ABC).TXT");
	_LIT(KTestFileNameSurrogatePair1_ShortName,		"ABCD_(~3.TXT");
	_LIT(KTestFileNameSurrogatePair2, 				"ABCD\xD846\xDF1D(DEF).TXT");
	_LIT(KTestFileNameSurrogatePair2_ShortName,		"ABCD_(~4.TXT");

	TFileName sn;
	MakeFile(KTestFileName1);
	TInt r = TheFs.GetShortName(KTestFileName1, sn);
	test_KErrNone(r);
	r = sn.Compare(KTestFileName1_ShortName);
	test_Equal(r, 0);

	MakeFile(KTestFileName2);
	r = TheFs.GetShortName(KTestFileName2, sn);
	test_KErrNone(r);
	r = sn.Compare(KTestFileName2_ShortName);
	test_Equal(r, 0);

	MakeFile(KTestFileName3);
	r = TheFs.GetShortName(KTestFileName3, sn);
	test_KErrNone(r);
	r = sn.Compare(KTestFileName3_ShortName);
	test_Equal(r, 0);

	MakeFile(KTestFileNameSurrogatePair1);
	r = TheFs.GetShortName(KTestFileNameSurrogatePair1, sn);
	test_KErrNone(r);
	r = sn.Compare(KTestFileNameSurrogatePair1_ShortName);
	test_Equal(r, 0);

	MakeFile(KTestFileNameSurrogatePair2);
	r = TheFs.GetShortName(KTestFileNameSurrogatePair2, sn);
	test_KErrNone(r);
	r = sn.Compare(KTestFileNameSurrogatePair2_ShortName);
	test_Equal(r, 0);

	test_KErrNone(TheFs.Delete(KTestFileName1));
	test_KErrNone(TheFs.Delete(KTestFileName2));
	test_KErrNone(TheFs.Delete(KTestFileName3));
	test_KErrNone(TheFs.Delete(KTestFileNameSurrogatePair1));
	test_KErrNone(TheFs.Delete(KTestFileNameSurrogatePair2));

	test.Next(_L("--TestDuplicateLongFileNames"));
	}

void TestDuplicateLongDirNames()
	{
	test.Next(_L("++TestDuplicateLongDirNames"));
	test.Next(_L("Testing tilde and number appended correctly for duplicate long name dirs"));

	TheFs.SessionPath(gSessionPath);

	// These are to test "~1", "~2" behaviours when the first 8 bytes of new directories
	// are identical with existing directories
	_LIT(KTestDirName1, 							"\\F32-TST\\T_SURROGATEPAIR\\ABCD\xFFFE(A)\\");
	_LIT(KTestDirName1_ShortName, 					"ABCD_(A)");
	_LIT(KTestDirName2, 							"\\F32-TST\\T_SURROGATEPAIR\\ABCD\xFFFE(AB)\\");
	_LIT(KTestDirName2_ShortName, 					"ABCD_(~1");
	_LIT(KTestDirName3, 							"\\F32-TST\\T_SURROGATEPAIR\\ABCD\xFFFE(ABC)\\");
	_LIT(KTestDirName3_ShortName,				 	"ABCD_(~2");
	_LIT(KTestDirNameSurrogatePair1, 				"\\F32-TST\\T_SURROGATEPAIR\\ABCD\xD846\xDF1D(ABCD)\\");
	_LIT(KTestDirNameSurrogatePair1_ShortName,		"ABCD_(~3");
	_LIT(KTestDirNameSurrogatePair2, 				"\\F32-TST\\T_SURROGATEPAIR\\ABCD\xD846\xDF1D(ABCDE)\\");
	_LIT(KTestDirNameSurrogatePair2_ShortName,		"ABCD_(~4");

	TFileName sn;
	MakeDir(KTestDirName1);
	TInt r = TheFs.GetShortName(KTestDirName1, sn);
	test_KErrNone(r);
	r = sn.Compare(KTestDirName1_ShortName);
	test_Equal(r, 0);

	MakeDir(KTestDirName2);
	r = TheFs.GetShortName(KTestDirName2, sn);
	test_KErrNone(r);
	r = sn.Compare(KTestDirName2_ShortName);
	test_Equal(r, 0);

	MakeDir(KTestDirName3);
	r = TheFs.GetShortName(KTestDirName3, sn);
	test_KErrNone(r);
	r = sn.Compare(KTestDirName3_ShortName);
	test_Equal(r, 0);

	MakeDir(KTestDirNameSurrogatePair1);
	r = TheFs.GetShortName(KTestDirNameSurrogatePair1, sn);
	test_KErrNone(r);
	r = sn.Compare(KTestDirNameSurrogatePair1_ShortName);
	test_Equal(r, 0);

	MakeDir(KTestDirNameSurrogatePair2);
	r = TheFs.GetShortName(KTestDirNameSurrogatePair2, sn);
	test_KErrNone(r);
	r = sn.Compare(KTestDirNameSurrogatePair2_ShortName);
	test_Equal(r, 0);

	test_KErrNone(TheFs.RmDir(KTestDirName1));
	test_KErrNone(TheFs.RmDir(KTestDirName2));
	test_KErrNone(TheFs.RmDir(KTestDirName3));
	test_KErrNone(TheFs.RmDir(KTestDirNameSurrogatePair1));
	test_KErrNone(TheFs.RmDir(KTestDirNameSurrogatePair2));

	test.Next(_L("--TestDuplicateLongDirNames"));
	}

void CallTestsL(void)
	{

	test.Title();
	test.Start(_L("Starting T_SURROGATEPAIR tests"));

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)

	// Test only runs on Fat file systems
	TheFs.SessionPath(gSessionPath);
	TInt driveNum = CurrentDrive();
	TFSName name;
	TInt r = TheFs.FileSystemName(name, driveNum);
	if (KErrNone == r)
		{
		if (name.Compare(_L("Fat")) != 0)
			{
			test.Printf(_L("Test only runs on 'Fat' drives"));
			}
		else
			{
			// Check for the default implementation
			// Disables codepage dll implementation of LocaleUtils functions
			r = TheFs.ControlIo(driveNum, KControlIoDisableFatUtilityFunctions);
			test_KErrNone(r);

			CreateTestDirectory(_L("\\F32-TST\\T_SURROGATEPAIR\\"));

			TestVolumeLabel();
			TestShortNameCharacter();
			TestConsistentShortNameGeneration();
			TestConsistentShortNameExtGeneration();
			TestDuplicateLongFileNames();
			TestDuplicateLongDirNames();

			DeleteTestDirectory();

			// Disables codepage dll implementation of LocaleUtils functions for other base tests
			r = TheFs.ControlIo(driveNum, KControlIoDisableFatUtilityFunctions);
			test_KErrNone(r);
			}
		}
	else
		{
		test.Printf(_L("Drive %C: is not ready!"), 'A'+driveNum);
		test(EFalse);
		}
#else
	test.Printf(_L("Test only runs on DEBUG builds, see test logs of debug builds for details."));
#endif  // _DEBUG) || _DEBUG_RELEASE
	test.End();
	}
