/*
* Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/
#define __E32TEST_EXTENSION__

#include "t_fatcharsetconv_aux.h"
#include <f32file.h>
#include <e32test.h>
#include <e32svr.h>
#include <hal.h>
#include <f32fsys.h>
#include <f32dbg.h>
#include "../server/t_server.h"

//#include "fat_utils.h"

extern RFile TheFile;

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
void TestIllegalCharsWithDll()
	{
	test.Next(_L("Test Illegal Character handling with DLL"));			
	__UHEAP_MARK;
	
	// logging for failure
	gTCType = ESymbianFATSpecific;
	RBuf failedOnBuf;
	failedOnBuf.CreateL(gLogFailureData.iFuncName);
	gTCId = 0;
	RBuf tcUniquePath;
	tcUniquePath.CreateL(KNone());

	QuickFormat();
	CreateTestDirectory(_L("\\F32-TST\\T_FATCHARSETCONV\\"));
	
	TInt r = TheFs.ControlIo(CurrentDrive(), KControlIoEnableFatUtilityFunctions);
	testAndLog(r==KErrNone);

	r = UserSvr::ChangeLocale(KTestLocale);
	testAndLog(r==KErrNone);

	_LIT(KTestNormalUnicodeFileName, 				"\x65B0\x6587\x4EF6.TXT");
	_LIT(KTestNormalUnicodeFileNameShortDefualt, 	"\x65B0\x6587\x4EF6.TXT");
	_LIT(KTestFileNameWithIllegalChars, 			"\x222F\x2F3A\x3C3E\x7C00.TXT");
	_LIT(KFileNameWithIllegalCharsShort, 			"___\x7C00.TXT");
	_LIT(KTestFileNameWithWildChars, 				"\x235B\x245C\x255D\x265E.TXT");
	_LIT(KTestFileNameWithWildCharsShort, 			"____.TXT");

	TFileName shn;

	MakeFile(KTestNormalUnicodeFileName);
	r = TheFs.GetShortName(KTestNormalUnicodeFileName, shn);
	testAndLog(r==KErrNone);
	r = shn.Compare(KTestNormalUnicodeFileNameShortDefualt);
	testAndLog(r==0);

	TFileName lgn;
	r = TheFs.GetLongName(KTestNormalUnicodeFileNameShortDefualt, lgn);
	testAndLog(r==KErrNone);
	r = lgn.Compare(KTestNormalUnicodeFileName);
	testAndLog(r==0);

	// Test illegal 8-bit ASCII chars in 16-bit Unicode chars.
	MakeFile(KTestFileNameWithIllegalChars);
	r = TheFs.GetShortName(KTestFileNameWithIllegalChars, shn);
	testAndLog(r==KErrNone);
	r = shn.Compare(KFileNameWithIllegalCharsShort);
	testAndLog(r==0);
	
	// Test wildcards, 8-bit ASCII chars in 16-bit Unicode chars.
	MakeFile(KTestFileNameWithWildChars);
	r = TheFs.GetShortName(KTestFileNameWithWildChars, shn);
	test_KErrNone(r);
	r = shn.Compare(KTestFileNameWithWildCharsShort);
	testAndLog(r==0);
	
	r=TheFs.Delete(KTestFileNameWithIllegalChars);
	testAndLog(r==0);
	r=TheFs.Delete(KTestNormalUnicodeFileName);
	testAndLog(r==0);
	r=TheFs.Delete(KTestFileNameWithWildChars);
	testAndLog(r==0);
	
	r = TheFs.ControlIo(CurrentDrive(), KControlIoDisableFatUtilityFunctions);
	testAndLog(r==KErrNone);
	failedOnBuf.Close();
	tcUniquePath.Close();
	__UHEAP_MARKEND;
	}

void TestIllegalCharsWithoutDLL()
	{
	test.Next(_L("Test Illegal Character handling without DLL"));			
	__UHEAP_MARK;
	
	// logging for failure
	gTCType = ESymbianFATSpecific;
	RBuf failedOnBuf;
	failedOnBuf.CreateL(gLogFailureData.iFuncName);
	gTCId = 0;
	RBuf tcUniquePath;
	tcUniquePath.CreateL(KNone());

	QuickFormat();
	CreateTestDirectory(_L("\\F32-TST\\T_FATCHARSETCONV\\"));

	TInt r = TheFs.ControlIo(CurrentDrive(), KControlIoDisableFatUtilityFunctions);
	testAndLog(r==KErrNone);	

	_LIT(KTestNormalUnicodeFileName, 				"\x65B0\x6587\x4EF6.TXT");
	_LIT(KTestNormalUnicodeFileNameShortDefualt, 	"___.TXT");
	_LIT(KTestFileNameWithIllegalChars, 			"\x222F\x2F3A\x3C3E\x7C00.TXT");
	_LIT(KFileNameWithIllegalCharsShort, 			"____.TXT");
	_LIT(KTestFileNameWithWildChars, 				"\x235B\x245C\x255D\x265E.TXT");
	_LIT(KTestFileNameWithWildCharsShort, 			"____~1.TXT");
	
	TFileName shn;
		
	MakeFile(KTestNormalUnicodeFileName);
	r = TheFs.GetShortName(KTestNormalUnicodeFileName, shn);
	testAndLog(r==KErrNone);
	r = shn.Compare(KTestNormalUnicodeFileNameShortDefualt);
	testAndLog(r==0);
	
	TFileName lgn;
	r = TheFs.GetLongName(KTestNormalUnicodeFileNameShortDefualt, lgn);
	testAndLog(r==KErrNone);
	r = lgn.Compare(KTestNormalUnicodeFileName);
	testAndLog(r==0);

	// Test illegal 8-bit ASCII chars in 16-bit Unicode chars.
	MakeFile(KTestFileNameWithIllegalChars);
	r = TheFs.GetShortName(KTestFileNameWithIllegalChars, shn);
	testAndLog(r==KErrNone);
	r = shn.Compare(KFileNameWithIllegalCharsShort);
	testAndLog(r==0);
	
	// Test illegal 8-bit ASCII chars in 16-bit Unicode chars.
	MakeFile(KTestFileNameWithWildChars);
	r = TheFs.GetShortName(KTestFileNameWithWildChars, shn);
	testAndLog(r==KErrNone);
	r = shn.Compare(KTestFileNameWithWildCharsShort);
	testAndLog(r==0);
	
	r=TheFs.Delete(KTestFileNameWithIllegalChars);
	testAndLog(r==0);
	r=TheFs.Delete(KTestNormalUnicodeFileName);
	testAndLog(r==0);
	r=TheFs.Delete(KTestFileNameWithWildChars);
	testAndLog(r==0);

	failedOnBuf.Close();
	tcUniquePath.Close();
	__UHEAP_MARKEND;
	}

void TestFileLengthMax()
	{
	test.Next(_L("Test accesing a long file with filename length <250 characters"));
	__UHEAP_MARK;
	
	// logging for failure
	gTCType = ESymbianFATSpecific;
	RBuf failedOnBuf;
	failedOnBuf.CreateL(gLogFailureData.iFuncName);
	gTCId = 0;
	RBuf tcUniquePath;
	tcUniquePath.CreateL(KNone());

	QuickFormat();
	CreateTestDirectory(_L("\\F32-TST\\T_FATCHARSETCONV\\"));


	TBuf<KMaxFileName> longName;

	_LIT(KTestDir1,	"\\TEST\\TESTLONGFILENAMELENGTH\\TESTMORETHAN250CHARACTERS\\") ;
	_LIT(KTestDir2,  "\x65B0\x6587\x4EF6\x4EF6(ABCDEFGH)\\(\x65B0\x6587\x4EF6\x4EF6)PQRST\\");
	_LIT(KTestDir3,  "MULTILEVEL-FOLDER1\\MULTILEVEL-FOLDER2\\MULTILEVEL-FOLDER3\\");
	_LIT(KTestDir4,  "\x65B0\x65B0\x65B0\x65B0(x6587)\\(\x6587\x6587\x6587\x6587)PQRST\\");
	_LIT(KTestDir5,  "\x4EF6\x4EF6\x4EF6\x4EF6(x4EF6)\\(\x6587\x6587\x6587\x6587)XYZ\\");	
	_LIT(KTestDir6,  "TESTINGINPROGRESS\\");
	_LIT(KTestLongFileLength,  "\x4EF6\x4EF6\x4EF6\x4EF6(x4EF6).TXT");
	
	longName += KTestDir1;
	longName += KTestDir2;
	longName += KTestDir3;
	longName += KTestDir4;
	longName += KTestDir5;
	longName += KTestDir6;	

	test.Printf(_L("longName count is  %d "), longName.Length());
	TInt r=TheFs.MkDirAll(longName);
	
	longName += KTestLongFileLength;
	test.Printf(_L("longName count is  %d "), longName.Length());

	testAndLog(longName.Length()<256);

	r=TheFile.Create(TheFs,longName ,EFileWrite);
	testAndLog(r==KErrNone);
	TheFile.Close();
		
	r=TheFile.Open(TheFs,longName ,EFileWrite);
	testAndLog(r==KErrNone);
	TheFile.Close();

	TheFs.Delete(longName);

	failedOnBuf.Close();
	tcUniquePath.Close();
	__UHEAP_MARKEND;
	}
	
void TestFileLengthExceedMax()
	{
	test.Next(_L("Test accesing a long file with filename length >250 characters"));
	__UHEAP_MARK;

	// logging for failure
	gTCType = ESymbianFATSpecific;
	RBuf failedOnBuf;
	failedOnBuf.CreateL(gLogFailureData.iFuncName);
	gTCId = 0;
	RBuf tcUniquePath;
	tcUniquePath.CreateL(KNone());

	QuickFormat();
	CreateTestDirectory(_L("\\F32-TST\\T_FATCHARSETCONV\\"));

	TInt r = TheFs.SessionPath(gSessionPath);
	testAndLog(r==KErrNone);
	
	TBuf<350> longName;

	_LIT(KTestDir1,	"\\TEST\\TESTLONGFILENAMELENGTH\\TESTMORETHAN260CHARACTERS\\") ;
	_LIT(KTestDir2,  "\x65B0\x6587\x4EF6\x4EF6(ABCDEFGH)\\(\x65B0\x6587\x4EF6\x4EF6)PQRST\\");
	_LIT(KTestDir3,  "MULTILEVEL-FOLDER1\\MULTILEVEL-FOLDER2\\MULTILEVEL-FOLDER3\\");
	_LIT(KTestDir4,  "\x65B0\x65B0\x65B0\x65B0(x6587)\\(\x6587\x6587\x6587\x6587)PQRST\\");
	_LIT(KTestDir5,  "\x4EF6\x4EF6\x4EF6\x4EF6(x4EF6)\\(\x6587\x6587\x6587\x6587)XYZ\\");	
	_LIT(KTestDir6,  "TESTINGINPROGRESS(TESTLENGTH)>256\\");	
	_LIT(KTestLongFileLength,  "\x4EF6\x4EF6\x4EF6\x4EF6(x4EF6).TXT");
	
	longName = gSessionPath;
	longName += KTestDir1;
	longName += KTestDir2;
	longName += KTestDir3;
	longName += KTestDir4;
	longName += KTestDir5;
	longName += KTestDir6;
	
	test.Printf(_L("longName length is  %d "), longName.Length());
	r=TheFs.MkDirAll(longName);
	
	longName += KTestLongFileLength;
	test.Printf(_L("longName count is  %d "), longName.Length());

	testAndLog(longName.Length()>256);
	
	r=TheFile.Create(TheFs,longName ,EFileWrite);
	testAndLog(r==KErrBadName);
	// TheFile.Close();
		
	r=TheFile.Open(TheFs,longName ,EFileWrite);
	testAndLog(r==KErrBadName);
	// TheFile.Close();
	
	TheFs.Delete(longName);	
	failedOnBuf.Close();
	tcUniquePath.Close();
	__UHEAP_MARKEND;		
	}
	
void TestLeadingE5Handling()
	{
	test.Next(_L("Test Leading \'E5\' byte handling"));
	__UHEAP_MARK;
	
	// logging for failure
	gTCType = ESymbianFATSpecific;
	RBuf failedOnBuf;
	failedOnBuf.CreateL(gLogFailureData.iFuncName);
	gTCId = 0;
	RBuf tcUniquePath;
	tcUniquePath.CreateL(KNone());

	QuickFormat();
	CreateTestDirectory(_L("\\F32-TST\\T_FATCHARSETCONV\\"));

	// Enables codepage dll implementation of LocaleUtils functions for this test only
	TInt r = TheFs.ControlIo(CurrentDrive(), KControlIoEnableFatUtilityFunctions);
	testAndLog(r==KErrNone);
	
	r = UserSvr::ChangeLocale(KTestLocale);
	testAndLog(r==KErrNone);

	r=TheFs.SessionPath(gSessionPath);
	testAndLog(r==KErrNone);

	_LIT(KTestFilePathAndName,		"\\F32-TST\\T_FATCHARSETCONV\\\x88F9.TXT");
	_LIT(KTestFileShortName, 		"\x88F9.TXT");
	
	MakeFile(KTestFilePathAndName);
	TFileName sn;
	r = TheFs.GetShortName(KTestFilePathAndName, sn);
	testAndLog(r==KErrNone);
	r = sn.Compare(KTestFileShortName);
	testAndLog(r==KErrNone);
	
	r=TheFs.Delete(KTestFilePathAndName);
	testAndLog(r==KErrNone);

	// Disables codepage dll implementation of LocaleUtils functions for other base tests
	r = TheFs.ControlIo(CurrentDrive(), KControlIoDisableFatUtilityFunctions);
	testAndLog(r==KErrNone);
	failedOnBuf.Close();
	tcUniquePath.Close();
	__UHEAP_MARKEND;
	}
	
void TestVFATCase3()
	{
	test.Next(_L("Test With VFAT entry, and DOS entry using Non-CP932 Japanese file's access"));
	__UHEAP_MARK;

	// logging for failure
	gTCType = ESymbianFATSpecific;
	RBuf failedOnBuf;
	failedOnBuf.CreateL(gLogFailureData.iFuncName);
	gTCId = 0;
	RBuf tcUniquePath;
	tcUniquePath.CreateL(KNone());

	QuickFormat();
	CreateTestDirectory(_L("\\F32-TST\\T_FATCHARSETCONV\\"));

	_LIT(KTestNormalFileName, 				"\x65B0\x6587\x4EF6.TXT");	
	_LIT(KTestNormalFileNameWithUnicode, 	"___.TXT");
	
	
	test.Printf(_L("Create a file without the DLL installed, and get the shortname"));
	TInt r=TheFile.Create(TheFs,KTestNormalFileName,EFileRead|EFileWrite);
	testAndLog(r==KErrNone);
	TFileName sn;
	r = TheFs.GetShortName(KTestNormalFileName, sn);
	testAndLog(r==KErrNone);
	r = sn.Compare(KTestNormalFileNameWithUnicode);
	testAndLog(r==0);
	TheFile.Close();
	
	test.Printf(_L("Access the file without the DLL installed"));
	r=TheFile.Open(TheFs,KTestNormalFileName,EFileWrite);
	testAndLog(r==KErrNone);
	
	TBuf<50> name;	
	r=TheFile.FullName(name);
	testAndLog(r==KErrNone);
	TheFile.Close();
	
	r=TheFile.Open(TheFs,KTestNormalFileNameWithUnicode,EFileWrite);
	testAndLog(r==KErrNone);
	TheFile.Close();
	
	//load the dll
	
	r = TheFs.ControlIo(CurrentDrive(), KControlIoEnableFatUtilityFunctions);
	testAndLog(r==KErrNone);
		
	r = UserSvr::ChangeLocale(KTestLocale);
	testAndLog(r==KErrNone);
	
	test.Printf(_L("Access the file with the DLL installed"));
	r=TheFile.Open(TheFs,KTestNormalFileName,EFileWrite);
	testAndLog(r==KErrNone);

	r=TheFile.FullName(name);
	testAndLog(r==KErrNone);
	TheFile.Close();
	
	r=TheFile.Open(TheFs,KTestNormalFileNameWithUnicode,EFileWrite);
	testAndLog(r==KErrNone);
	TheFile.Close();
	
	r=TheFs.Delete(KTestNormalFileName);
			
    r = TheFs.ControlIo(CurrentDrive(), KControlIoDisableFatUtilityFunctions);
	testAndLog(r==KErrNone);
	failedOnBuf.Close();
	tcUniquePath.Close();
	__UHEAP_MARKEND;
	}

void TestVFATCase2()
	{
	test.Next(_L("Test With VFAT entry, and DOS entry using CP932 Japanese file's access"));
	__UHEAP_MARK;
	
	// logging for failure
	gTCType = ESymbianFATSpecific;
	RBuf failedOnBuf;
	failedOnBuf.CreateL(gLogFailureData.iFuncName);
	gTCId = 0;
	RBuf tcUniquePath;
	tcUniquePath.CreateL(KNone());

	QuickFormat();
	CreateTestDirectory(_L("\\F32-TST\\T_FATCHARSETCONV\\"));

	TInt r = TheFs.ControlIo(CurrentDrive(), KControlIoEnableFatUtilityFunctions);
	test_KErrNone(r);
		
	r = UserSvr::ChangeLocale(KTestLocale);
	test_KErrNone(r);
	r=TheFs.SetSessionPath(gSessionPath);
	testAndLog(r==KErrNone);
	
	_LIT(KTestNormalFileName, 		"\x65B0\x6587\x4EF6.TXT");	
	_LIT(KTestNormalFileShortName, 	"\x65B0\x6587\x4EF6.TXT");
	
	test.Printf(_L("Create a file with the DLL installed, and get the shortname"));
	r=TheFile.Create(TheFs,KTestNormalFileName,EFileRead|EFileWrite);
	testAndLog(r==KErrNone);
	TFileName sn;
	r = TheFs.GetShortName(KTestNormalFileName, sn);
	testAndLog(r==KErrNone);
	r = sn.Compare(KTestNormalFileShortName);
	testAndLog(r==0);
	TheFile.Close();
	
	test.Printf(_L("Access the file with the DLL installed"));
	r=TheFile.Open(TheFs,KTestNormalFileName,EFileWrite);
	testAndLog(r==KErrNone);
	
	TBuf<50> name;	
	r=TheFile.FullName(name);
	testAndLog(r==KErrNone);
	TheFile.Close();
	
	r=TheFile.Open(TheFs,KTestNormalFileShortName,EFileWrite);
	testAndLog(r==KErrNone);
	TheFile.Close();
	
	r = TheFs.ControlIo(CurrentDrive(), KControlIoDisableFatUtilityFunctions);
	testAndLog(r==KErrNone);
	
	test.Printf(_L("Access the file without the DLL installed"));
	
	r=TheFile.Open(TheFs,KTestNormalFileName,EFileWrite);
	testAndLog(r==KErrNone);

	r=TheFile.FullName(name);
	testAndLog(r==KErrNone);
	TheFile.Close();
	
	r=TheFile.Open(TheFs,KTestNormalFileShortName,EFileWrite);
	testAndLog(r==KErrNone);
	TheFile.Close();
	
	r=TheFs.Delete(KTestNormalFileName);
	failedOnBuf.Close();
	tcUniquePath.Close();
	__UHEAP_MARKEND;
	}
	
void TestVFATCase1()
	{
	test.Next(_L("Test Without VFAT entry, but DOS entry uses CP932 Japanese file's access"));
	__UHEAP_MARK;
	
	// logging for failure
	gTCType = ESymbianFATSpecific;
	RBuf failedOnBuf;
	failedOnBuf.CreateL(gLogFailureData.iFuncName);
	gTCId = 0;
	RBuf tcUniquePath;
	tcUniquePath.CreateL(KNone());

	QuickFormat();
	CreateTestDirectory(_L("\\F32-TST\\T_FATCHARSETCONV\\"));

	GetBootInfo();

   	RFile file;
  	TFileName fn = _L("\\ABCD");
  	
  	TInt r=file.Create(TheFs,fn,EFileRead);
  	testAndLog(r==KErrNone);
  	file.Close();
  
  	//	Assume this file is the first entry in the root directory
  	r=TheDisk.Open(TheFs,CurrentDrive());
  	testAndLog(r==KErrNone);

    //-- read the 1st dir entry, it should be a DOS entry 
    const TInt posEntry1=gBootSector.RootDirStartSector() << KDefaultSectorLog2; //-- dir entry1 position
      
    TFatDirEntry fatEntry1;
 	TPtr8 ptrEntry1((TUint8*)&fatEntry1,sizeof(TFatDirEntry));
	testAndLog(TheDisk.Read(posEntry1, ptrEntry1)==KErrNone); 
	testAndLog(!fatEntry1.IsVFatEntry());

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
	testAndLog(TheDisk.Write(posEntry1, ptrEntry1)==KErrNone);
	TheDisk.Close();

  	// The Unicode file name of the file that is created
  	fn = _L("\\ABCD");
  	fn[1] = 0x798F;
  	fn[2] = 0x5C71;
  	fn[3] = 0x96C5;
  	fn[4] = 0x6CBB;
  	
  	// Access the file using its unicode file name without the DLL loaded.
      r = TheFs.ControlIo(CurrentDrive(), KControlIoDisableFatUtilityFunctions);
  	test_KErrNone(r);
  	
  	TEntry entry;
  	TInt err = TheFs.Entry(fn, entry);
  	testAndLog(err==KErrNotFound);
  	
      // Access the file using its unicode file name with the DLL loaded.
      r = TheFs.ControlIo(CurrentDrive(), KControlIoEnableFatUtilityFunctions);  	
	test_KErrNone(r);
 	r = UserSvr::ChangeLocale(KTestLocale);  	
	test_KErrNone(r);
  
  	err = TheFs.Entry(fn, entry);
  	testAndLog(err==KErrNone);
  	
  	//file is no more required,delete it.
  	err = TheFs.Delete(fn);
  	testAndLog(err==KErrNone);
  	
      r = TheFs.ControlIo(CurrentDrive(), KControlIoDisableFatUtilityFunctions);
  	test_KErrNone(r);
  	
 	r=TheFs.SessionPath(gSessionPath);
  	test_KErrNone(r);
	failedOnBuf.Close();
	tcUniquePath.Close();
	__UHEAP_MARKEND;
	}

void DoSymbianSpecificCases()
	{
	gLogFailureData.iTCTypeName = KSymbianFATSpecific;
	gLogFailureData.iAPIName= KNone;

	// the preprocessor macro gives a C/C++ zero-terminated string; use overloaded ::Copy()
	// Why oh why does TPtrC8 not have a "char *" constructor ...
	gFileName.Copy(TPtrC8((const TUint8*)__FILE__));

	gLogFailureData.iFuncName = KTestLeadingE5Handling;
	TestLeadingE5Handling();

	gLogFailureData.iFuncName = KTestIllegalCharsWithDll;
	TestIllegalCharsWithDll();

	gLogFailureData.iFuncName = KTestIllegalCharsWithoutDLL;
	TestIllegalCharsWithoutDLL();

	gLogFailureData.iFuncName = KTestFileLengthMax;
	TestFileLengthMax();

	gLogFailureData.iFuncName = KTestFileLengthExceedMax;
	TestFileLengthExceedMax();
	
	gLogFailureData.iFuncName = KTestVFATCase2;
	TestVFATCase2();

	gLogFailureData.iFuncName = KTestVFATCase3;
	TestVFATCase3();

	gLogFailureData.iFuncName = KTestVFATCase1;
	TestVFATCase1();
		
	}

#endif //defined(_DEBUG) || defined(_DEBUG_RELEASE)
