// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_dirs.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include "t_server.h"

#include "f32_test_utils.h"

using namespace F32_Test_Utils;


RTest test(_L("T_DIRS"));

TTime gTimeNow;
TBool gTestedZ = EFalse;
TInt gDriveNum = -1;

static void Test1()
//
// Make a directory with lots of entries
//
	{

	RFile f;
	TInt maxEntry=56;
	test.Next(_L("Create a directory with 55 entries"));
	TFileName sessionPath;
	TInt r=TheFs.SessionPath(sessionPath);
	test_KErrNone(r);
	r=TheFs.MkDir(_L("\\F32-TST\\"));
	test_Value(r, (r == KErrNone)||(r==KErrAlreadyExists));
	r=TheFs.MkDir(_L("\\F32-TST\\TDIRS\\"));
	test_Value(r, (r == KErrNone)||(r==KErrAlreadyExists));
	
	for (TInt i=0;i<maxEntry;i++)
		{
		TFileName baseName=_L("\\F32-TST\\TDIRS\\FILE");
		baseName.AppendNum(i);
		r=f.Replace(TheFs,baseName,EFileRead);
		test_KErrNone(r);
		r=f.Write(_L8("Hello World"));
		test_KErrNone(r);
		f.Close();
		}
	test.Next(_L("Test all entries have been created successfully."));
	for (TInt j=0;j<=maxEntry;j++)
		{
		TFileName baseName=_L("\\F32-TST\\TDIRS\\FILE");
		baseName.AppendNum(j);
		TInt r=f.Open(TheFs,baseName,EFileRead);
		if (r!=KErrNone)
			{
			test_Value(r, r == KErrNotFound && j==maxEntry);
			return;
			}
		TBuf8<16> data;
		r=f.Read(data);
		test_KErrNone(r);
		test(data==_L8("Hello World"));
		f.Close();
		}
	}
	
static void Test2()
//
// List all directory entries
//
	{
	
	test.Printf(_L("List all entries in directory %S\n"),&gSessionPath);
	RDir d;

	TInt r=d.Open(TheFs,gSessionPath,KEntryAttMaskSupported);
	if (r==KErrNone)
		{
		TEntry e;
		while ((r=d.Read(e))==KErrNone)
			{
			if (e.IsDir())
				test.Printf(_L("%- 20S <DIR>\n"),&e.iName);
			else
				test.Printf(_L("%- 20S %+ 8d\n"),&e.iName,e.iSize);
			}
		d.Close();
		if (r!=KErrEof)
			test.Printf(_L("Error %d\n"),r);
		}
	else
		test.Printf(_L("Error %d\n"),r);	
	}

static void TestZ()
//
// Check you cannot open a directory on a file
//
	{

	test.Next(_L("Open files and directories on Z:"));
	TEntry entry;
	RDir d;
	
	TInt r=d.Open(TheFs,PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin)?_L("\\Sys\\Bin\\ESHELL.EXE\\*"):_L("\\System\\Bin\\ESHELL.EXE\\*"),KEntryAttMaskSupported);
	test_Value(r, r == KErrPathNotFound);
	
	r=d.Open(TheFs,PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin)?_L("\\Sys\\Bin\\ESHELL.EXE"):_L("\\System\\Bin\\ESHELL.EXE"),KEntryAttMaskSupported);
	test_KErrNone(r);
	
	r=d.Read(entry);
	if (r==KErrEof)
		{
		test.Printf(_L("Error: EShell.EXE not found\n"));
		//test.Getch();
		}
	else
		{
		test_KErrNone(r);
		test(entry.iName.FindF(_L("ESHELL.EXE"))>=0);
		r=d.Read(entry);
		test_Value(r, r == KErrEof);
		}
	d.Close();

	r=d.Open(TheFs,_L("\\*.XQP"),KEntryAttMaskSupported);
	test_KErrNone(r);
	r=d.Read(entry);
	test_Value(r, r == KErrEof);
	d.Close();

	r=d.Open(TheFs,PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin)?_L("\\Sys\\Bin\\"):_L("\\System\\Bin\\"),KEntryAttMaskSupported);
	test_KErrNone(r);
	r=d.Read(entry);
	
	if (r==KErrEof)
		{
		test.Printf(_L("No files found\n"));
		d.Close();
		}
	else
		{
		test_KErrNone(r);
		test.Printf(_L("First Entry = %S\n"),&entry.iName);
		r=d.Read(entry);
		test_KErrNone(r);
		test.Printf(_L("Second Entry = %S\n"),&entry.iName);
		d.Close();
		}

	r=d.Open(TheFs,_L("\\*"),KEntryAttMaskSupported);
	test_KErrNone(r);
	d.Close();
	r=d.Open(TheFs,PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin)?_L("\\Sys\\Bin\\*"):_L("\\System\\Bin\\*"),KEntryAttMaskSupported);
	test_KErrNone(r);
	d.Close();
	}

static void Test3()
//
// Check you cannot open a directory on a file
//
	{

	test.Next(_L("Open files and directories"));
	TEntry entry;
	RFile f;
	TInt r=f.Replace(TheFs,_L("\\F32-TST\\TDIRS\\TESTFILEORISITA.DIR"),EFileWrite);
	test_KErrNone(r);
	r=f.Write(_L8("TESTDATATESTDATATESTDATATESTDATATESTDATATESTDATATESTDATATESTDATATESTDATATESTDATATESTDATATESTDATA"));
	test_KErrNone(r);
	r=TheFs.Delete(_L("\\F32-TST\\TDIRS\\TESTFILEORISITA.DIR"));
	test_Value(r, r == KErrInUse);
	f.Close();
	RDir d;
	r=d.Open(TheFs,_L("\\F32-TST\\TDIRS\\TESTFILEORISITA.DIR\\*"),KEntryAttMaskSupported);
	test_Value(r, r == KErrPathNotFound);
	r=d.Open(TheFs,_L("\\F32-TST\\TDIRS\\*.XQP"),KEntryAttMaskSupported);
	test_KErrNone(r);
	r=d.Read(entry);
	test_Value(r, r == KErrEof);
	d.Close();
	r=d.Open(TheFs,_L("\\F32-TST\\TDIRS\\TESTFILEORISITA.DIR"),KEntryAttMaskSupported);
	test_KErrNone(r);
	r=d.Read(entry);
	test_KErrNone(r);
	test(entry.iName.FindF(_L("TESTFILEORISITA.DIR"))>=0);
	r=d.Read(entry);
	test_Value(r, r == KErrEof);
	d.Close();
	r=d.Open(TheFs,_L("\\"),KEntryAttMaskSupported);
	test_KErrNone(r);
	d.Close();
	r=d.Open(TheFs,_L("\\F32-TST\\"),KEntryAttMaskSupported);
	test_KErrNone(r);
	d.Close();
	r=d.Open(TheFs,_L("\\*"),KEntryAttMaskSupported);
	test_KErrNone(r);
	d.Close();
	r=d.Open(TheFs,_L("\\F32-TST\\*"),KEntryAttMaskSupported);
	test_KErrNone(r);
	d.Close();
	
	// create a small file on the root
	test(f.Replace(TheFs, _L("\\TEST.FILE"), EFileWrite) == KErrNone);
	test(f.Write(_L8("1234567890987654321234567890")) == KErrNone);
	f.Close();
	// try some directory operations on the file
	test(TheFs.RmDir(_L("\\TEST.FILE\\")) == KErrPathNotFound);
	test(TheFs.RmDir(_L("\\TEST.FILE\\ZZZ\\")) == KErrPathNotFound);
	test(TheFs.MkDir(_L("\\TEST.FILE\\ZZZ\\")) == KErrPathNotFound);
	// cleanup
	TheFs.Delete(_L("\\TEST.FILE"));	
	
	
	r=TheFs.MkDir(_L("\\F32-TST\\EMPTY\\"));
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	r=d.Open(TheFs,_L("\\F32-TST\\EMPTY\\*"),KEntryAttMaskSupported);
	test_KErrNone(r);
//	r=TheFs.RmDir(_L("\\F32-TST\\EMPTY\\"));
	r=d.Read(entry);
	test_Value(r, r == KErrEof);
//	r=TheFs.RmDir(_L("\\F32-TST\\EMPTY\\"));
//	test_Value(r, r == KErrInUse);
	r=d.Read(entry);
	r=d.Read(entry);
	r=d.Read(entry);
	d.Close();
	r=TheFs.RmDir(_L("\\F32-TST\\EMPTY\\"));
	test_KErrNone(r);
	}


static void CreateSortNoneTestDirectoryStructure()
//
// Create files
//
	{
//	Delete the directory to be tested if it already exists as a result of this
//	test being run previously.  It's necessary remove it and then recreate it 
//	because a later test relies on the time of file/directory creation to be
//	the time this function was run...
	
	CFileMan* fMan=CFileMan::NewL(TheFs);
	test(fMan!=NULL);
	TInt r=fMan->RmDir(_L("\\F32-TST\\TDIRS\\SORT_NONE\\"));
	test_Value(r, (r == KErrNone)||(r==KErrPathNotFound));
	delete fMan;	

	gTimeNow.HomeTime();	//	Set global TTime gTimeNow to time now - for later tests
	r=TheFs.MkDirAll(_L("\\F32-TST\\TDIRS\\SORT_NONE\\"));
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	MakeFile(_L("\\f32-tst\\tdirs\\sort_none\\file1.txt"));
	r=TheFs.MkDir(_L("\\F32-TST\\TDIRS\\SORT_NONE\\FILE_DIR1.APP\\"));
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	MakeFile(_L("\\f32-tst\\tdirs\\sort_none\\file1.app"));
	r=TheFs.MkDir(_L("\\F32-TST\\TDIRS\\SORT_NONE\\FILE_DIR2.TXT\\"));
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	MakeFile(_L("\\f32-tst\\tdirs\\sort_none\\file2.txt"));
	r=TheFs.MkDir(_L("\\F32-TST\\TDIRS\\SORT_NONE\\FILE_DIR3.APP\\"));
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	MakeFile(_L("\\f32-tst\\tdirs\\sort_none\\ZZZZ"));
	MakeFile(_L("\\f32-tst\\tdirs\\sort_none\\AAAA"));
	MakeFile(_L("\\f32-tst\\tdirs\\sort_none\\WWWW"));
	MakeFile(_L("\\f32-tst\\tdirs\\sort_none\\file2.app"));
	MakeFile(_L("\\f32-tst\\tdirs\\sort_none\\file3.txt"));
	MakeFile(_L("\\f32-tst\\tdirs\\sort_none\\file3.app"));
	MakeFile(_L("\\f32-tst\\tdirs\\sort_none\\NOEXT1"));
	MakeFile(_L("\\f32-tst\\tdirs\\sort_none\\NOEXT2"));
	MakeFile(_L("\\f32-tst\\tdirs\\sort_none\\EXTMISSING"));
	}

static void Test4()
//
// Test ESortNone
//
	{

	test.Next(_L("Test ESortNone"));
	CreateSortNoneTestDirectoryStructure();
	CDir* dir;
	CDir* dirSorted;
	
//
// GetDir OOM failure passes 'callback' test from client side but not server side
//
	TheFs.SetAllocFailure(gAllocFailOff);

	TInt r=TheFs.GetDir(_L("\\f32-tst\\tdirs\\sort_none\\*"),KEntryAttMaskSupported,ESortNone,dir);
	test_KErrNone(r);
	TInt count=dir->Count();
	test(count==15);
	r=TheFs.GetDir(_L("\\f32-tst\\tdirs\\sort_none\\*"),KEntryAttMaskSupported,ESortByName,dirSorted);
	test_KErrNone(r);
	test(dirSorted->Count()==15);
	delete dirSorted;
	delete dir;

	r=TheFs.GetDir(_L("\\f32-tst\\tdirs\\sort_none\\*.txt"),KEntryAttNormal,ESortNone,dir);
	test_KErrNone(r);
	test(dir->Count()==3);
	delete dir;
	r=TheFs.GetDir(_L("\\f32-tst\\tdirs\\sort_none\\*.app"),KEntryAttNormal,ESortNone,dir);
	test_KErrNone(r);
	test(dir->Count()==3);
	delete dir;
	r=TheFs.GetDir(_L("\\f32-tst\\tdirs\\sort_none\\*.app"),KEntryAttNormal|KEntryAttDir,ESortNone,dir);
	test_KErrNone(r);
	test(dir->Count()==5);
	delete dir;
	r=TheFs.GetDir(_L("\\f32-tst\\tdirs\\sort_none\\*.app"),KEntryAttNormal|KEntryAttDir,ESortNone|EDirsFirst,dir);
	test_KErrNone(r);
	test(dir->Count()==5);
	delete dir;
	r=TheFs.GetDir(_L("\\f32-tst\\tdirs\\sort_none\\*.app"),KEntryAttNormal|KEntryAttDir,ESortNone|EDirsLast,dir);
	test_KErrNone(r);
	test(dir->Count()==5);
	delete dir;

	TheFs.SetAllocFailure(gAllocFailOn);
	}

static void Test5()
//
// Test return values
//
	{

	test.Next(_L("Test return values"));
	RDir dir;
	TInt r=dir.Open(TheFs,_L("\\DoesNotExist\\*"),KEntryAttMaskSupported);
	test_Value(r, r == KErrPathNotFound);
	r=dir.Open(TheFs,_L("\\"),KEntryAttMaskSupported);
	test_KErrNone(r);
	dir.Close();
	}


static void Test6()
//
// Test that "*.*" matches all files/directories
//
	{

	test.Next(_L("Test *.* matches all files"));
	CDir* dirList;
	TInt r=TheFs.GetDir(_L("\\f32-tst\\tdirs\\sort_none\\*.*"),KEntryAttNormal|KEntryAttDir,ESortByName|EDirsLast,dirList);
	test_KErrNone(r);
	TInt count=dirList->Count();
	test(count==15);
	TEntry entry=(*dirList)[0];
	test(entry.iName.FindF(_L("AAAA"))>=0);
	entry=(*dirList)[1];
	test(entry.iName.FindF(_L("EXTMISSING"))>=0);
	entry=(*dirList)[2];
	test(entry.iName.FindF(_L("FILE1.APP"))>=0);
	entry=(*dirList)[3];
	test(entry.iName.FindF(_L("FILE1.TXT"))>=0);
	entry=(*dirList)[4];
	test(entry.iName.FindF(_L("FILE2.APP"))>=0);
	entry=(*dirList)[5];
	test(entry.iName.FindF(_L("FILE2.TXT"))>=0);
	entry=(*dirList)[6];
	test(entry.iName.FindF(_L("FILE3.APP"))>=0);
	entry=(*dirList)[7];
	test(entry.iName.FindF(_L("FILE3.TXT"))>=0);
	entry=(*dirList)[8];
	test(entry.iName.FindF(_L("NOEXT1"))>=0);
	entry=(*dirList)[9];
	test(entry.iName.FindF(_L("NOEXT2"))>=0);
	entry=(*dirList)[10];
	test(entry.iName.FindF(_L("WWWW"))>=0);
	entry=(*dirList)[11];
	test(entry.iName.FindF(_L("ZZZZ"))>=0);
	entry=(*dirList)[12];
	test(entry.iName.FindF(_L("FILE_DIR1.APP"))>=0);
	entry=(*dirList)[13];
	test(entry.iName.FindF(_L("FILE_DIR2.TXT"))>=0);
	entry=(*dirList)[14];
	test(entry.iName.FindF(_L("FILE_DIR3.APP"))>=0);
	delete dirList;

	RDir dir;
	r=dir.Open(TheFs,_L("\\f32-tst\\tdirs\\sort_none\\*.*"),KEntryAttNormal|KEntryAttDir);
	test_KErrNone(r);
	
	TTime time;
	TInt64 difference;
	TInt64 maxOK=1000000;
	maxOK*=60;
	maxOK*=3;

	for (TInt i=0; i<15; i++)
		{
		r=dir.Read(entry);
		test_KErrNone(r);
		time=entry.iModified;
		difference=time.Int64()-gTimeNow.Int64();
		test(difference<maxOK);
		}

	r=dir.Read(entry);
	test_Value(r, r == KErrEof);
	dir.Close();

	TheFs.SetAllocFailure(gAllocFailOn);
	}


static void Test7()
//
// Fill up the root directory
//
	{
    test.Next(_L("Fill up the root directory"));
    
    if(!Is_Fat12(TheFs, gDriveNum) && !Is_Fat16(TheFs, gDriveNum))
        {
        test.Printf(_L("Skipping. Applicable for FAT12/16 only!\n"));
        return;
        }    

    TInt r = FormatDrive(TheFs, gDriveNum, ETrue);
    test_KErrNone(r);

	TBuf<32> baseName=_L("\\RD");
	TBuf<32> id;
	TBuf<32> fileName;
	TInt count=0;

	RFile f;
	TParsePtrC parser(gSessionPath);
	FOREVER
		{
		id.Num(count+1);
		fileName=parser.Drive();
		fileName+=baseName;
		fileName+=id;
		TInt r=f.Replace(TheFs,fileName,EFileWrite);
		if(r==KErrDirFull)
			{
			break;
			}
		test_KErrNone(r);
		f.Close();
		count++;
		if(count >= 1000)
			{
			break;
			}
		test.Printf(_L("CreateFile	:	%d	: %S\r"),count,&fileName);
		}
	test.Printf(_L("\n"));

	while (count)
		{
		id.Num(count);
		fileName=parser.Drive();
		fileName+=baseName;
		fileName+=id;
		TInt r=TheFs.Delete(fileName);
		test_KErrNone(r);
		test.Printf(_L("DeleteFile	:	%d	: %S\r"),count,&fileName);
		--count;
		}
	test.Printf(_L("\n"));

	test.Next(_L("Long filenames in root"));
	TFileName longFileName;
	longFileName.SetLength(254);
//	Mem::Fill((TUint8*)longFileName.Ptr(),254,'A');
	Mem::Fill((TUint8*)longFileName.Ptr(),254*sizeof(TText),'A');
	longFileName[0]='\\';
	longFileName[253]='\\';
	r=TheFs.MkDir(longFileName);
	test_KErrNone(r);
	CDir* dirList=NULL;
	r=TheFs.GetDir(longFileName,KEntryAttMaskSupported,ESortByName,dirList);
	test_KErrNone(r);
	count=dirList->Count();
	test(count==0);
	delete dirList;
	TParse parse;
	r=TheFs.Parse(longFileName,parse);
	test_KErrNone(r);
	TEntry entry;
	r=TheFs.Entry(longFileName,entry);
	test_KErrNone(r);
	r=TheFs.SetSessionPath(longFileName);
	test_KErrNone(r);
	r=TheFs.GetDir(longFileName,KEntryAttMaskSupported,ESortByName,dirList);
	test_KErrNone(r);
	count=dirList->Count();
	test(count==0);
	delete dirList;
	r=TheFs.Parse(longFileName,_L("*"),parse);
	test_Value(r, r == KErrBadName);
	r=f.Open(TheFs,_L("asdf.asdf"),0);
	test_Value(r, r == KErrBadName);
	r=TheFs.Entry(longFileName,entry);
	test_KErrNone(r);
	r=TheFs.RmDir(longFileName);
	test_KErrNone(r);
	}

static void Test8()
//
// Regression tests
//
	{

	test.Next(_L("Open dir and change drives"));
	MakeDir(_L("C:\\MOON\\"));
	RDir dir;
	TInt r=dir.Open(TheFs,_L("C:\\MOON\\"),0);
	test_KErrNone(r);
	TFileName driveName;
	r=TheFs.GetDriveName(11,driveName);
	test_KErrNone(r);
	TEntryArray entryArray;
	r=dir.Read(entryArray);
	test_Value(r, r == KErrEof);
	test(entryArray.Count()==0);
	dir.Close();
	r=TheFs.RmDir(_L("C:\\MOON\\"));
	test_KErrNone(r);

	test.Next(_L("MkDir all on nonexistent drive"));
	r=TheFs.MkDirAll(_L("L:\\MOON"));
	test_Value(r, (r == KErrNotReady)||(r==KErrPathNotFound));
	}

static void CleanupL()
//
// Clean up tests
//
	{

	test.Next(_L("Delete test directory"));
	CFileMan* fMan=CFileMan::NewL(TheFs);
	TInt r=fMan->RmDir(gSessionPath);
	test_KErrNone(r);
	r=fMan->Delete(_L("\\Filluptherootdir*"));
	test_Value(r, r == KErrNone || r==KErrNotFound);
	delete fMan;
	}

static void Test9()
//
// Test directories with trailing dots (ref. DEF047684) 
//
	{

	test.Next(_L("Testing directory names with trailing dots"));
	TInt r;
	r=TheFs.MkDir(_L("\\test9..\\"));
	test_Value(r, r == KErrBadName);
	r=TheFs.MkDir(_L("\\test9\\"));
	test_Value(r, (r == KErrNone)||(r==KErrAlreadyExists));
	r=TheFs.Rename(_L("\\test9\\"),_L("\\test9..\\"));
	test_Value(r, r == KErrBadName);
	r= TheFs.RmDir(_L("\\test9\\"));
	test_KErrNone(r);
	r=TheFs.MkDir(_L("\\t.\\"));
	test_Value(r, r == KErrBadName);
	}


//
// Path and File names for sorting by name
//
// The correctly sorted directory listing should be:
//
//     b.doc
//     bb.doc
//     bs.doc
//
_LIT(KSortByNamePath, "\\F32-TST\\TDIRS\\SORT_NAME\\");
_LIT(KFileBS,         "bs.doc");
_LIT(KFileBB,         "bb.doc");
_LIT(KFileB,          "b.doc");
_LIT(KSortAll,        "*.*");
_LIT(KPrintFileName,  "%S\n");


static void DeleteTestDirectoryStructure(const TDesC& aPath)
//
//	Delete the directory to be tested if it already exists as a result of this
//	test being run previously.
//
	{
	
	CFileMan* fMan=CFileMan::NewL(TheFs);
	test(fMan!=NULL);
	TInt r=fMan->RmDir(aPath);
	test_Value(r, (r == KErrNone)||(r==KErrPathNotFound));
	delete fMan;	
	}


static void CreateTestDirectoryStructure(const TDesC& aPath, const TDesC** aFileArray, TInt aNumFiles)
//
// Create files
//
	{
	DeleteTestDirectoryStructure(aPath);	

	gTimeNow.HomeTime();	//	Set global TTime gTimeNow to time now - for later tests
	TInt r=TheFs.MkDirAll(aPath);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);

	TBuf<128> fileName;
	for (TInt i = 0; i < aNumFiles; i++)
		{
		fileName = aPath;
		fileName.Append(*aFileArray[i]);
		MakeFile(fileName);
		}
	}


static void TestSortByName()
//
// Test that sorting by name works for different length filenames.
//
	{
	const TDesC* theFiles[] = {&KFileBS, &KFileBB, &KFileB};
	TInt numFiles = sizeof(theFiles)/sizeof(theFiles[0]);
	CreateTestDirectoryStructure(KSortByNamePath, theFiles, numFiles);

	test.Next(_L("Test ESortByName"));
	CDir* dirList;
	TBuf<128> sortSpec(KSortByNamePath);
	sortSpec.Append(KSortAll);
	TInt r=TheFs.GetDir(sortSpec, KEntryAttNormal | KEntryAttDir, ESortByName | EDirsLast, dirList);
	test_KErrNone(r);
	TInt count=dirList->Count();
	test(count==numFiles);


	TInt i;
	for (i = 0; i < count; i++)
		{
		test.Printf(KPrintFileName, &(*dirList)[i].iName);
		}
	
	TEntry entry=(*dirList)[0];
	test(entry.iName.FindF(KFileB)>=0);
	entry=(*dirList)[1];
	test(entry.iName.FindF(KFileBB)>=0);
	entry=(*dirList)[2];
	test(entry.iName.FindF(KFileBS)>=0);
	delete dirList;
	dirList = 0;


	test.Next(_L("Test ESortByName (descending)"));


	r=TheFs.GetDir(sortSpec, KEntryAttNormal | KEntryAttDir, ESortByName | EDirsLast | EDescending, dirList);
	test_KErrNone(r);
	count=dirList->Count();
	test(count==numFiles);


	for (i = 0; i < count; i++)
		{
		test.Printf(KPrintFileName, &(*dirList)[i].iName);
		}

	
	entry=(*dirList)[0];
	test(entry.iName.FindF(KFileBS)>=0);
	entry=(*dirList)[1];
	test(entry.iName.FindF(KFileBB)>=0);
	entry=(*dirList)[2];
	test(entry.iName.FindF(KFileB)>=0);
	delete dirList;
	dirList = 0;
	
	
	DeleteTestDirectoryStructure(KSortByNamePath);
	}



//
// Path and File names for sorting by extension
//
// The correctly sorted directory listing should be:
//
//     sortext.a
//     sortext.bbb.a
//     sortext1.ddd.a
//     sortext.aaa.b
//     sortext.b
//     sortext.c
//     sortext.ccc.c
//
// as we should sort by the substring after the last '.'
//
_LIT(KSortByExtPath, "\\F32-TST\\TDIRS\\SORT_EXT\\");
_LIT(KFile1,         "sortext.aaa.b");
_LIT(KFile2,         "sortext.bbb.a");
_LIT(KFile3,         "sortext.ccc.c");
_LIT(KFile4,         "sortext1.ddd.a");
_LIT(KFile5,         "sortext.a");
_LIT(KFile6,         "sortext.b");
_LIT(KFile7,         "sortext.c");


static void TestSortByExt()
//
// Test that sorting by extension works. This includes filenames
// that contain multiple .'s
//
	{
	const TDesC* theFiles[] = {&KFile1, &KFile2, &KFile3, &KFile4, &KFile5, &KFile6, &KFile7};
	TInt numFiles = sizeof(theFiles)/sizeof(theFiles[0]);
	CreateTestDirectoryStructure(KSortByExtPath, theFiles, numFiles);

	test.Next(_L("Test ESortByExt"));

	CDir* dirList;
	TBuf<128> sortSpec(KSortByExtPath);
	sortSpec.Append(KSortAll);
	TInt r=TheFs.GetDir(sortSpec, KEntryAttNormal | KEntryAttDir, ESortByExt | EDirsLast, dirList);
	test_KErrNone(r);
	TInt count=dirList->Count();
	test(count==numFiles);


	TInt i;
	for (i = 0; i < count; i++)
		{
		test.Printf(KPrintFileName, &(*dirList)[i].iName);
		}
	
	
	//
	// Verify that the files have been sorted correctly by extension
	//
	TEntry entry=(*dirList)[0];
	test(entry.iName.FindF(KFile5)>=0);
	entry=(*dirList)[1];
	test(entry.iName.FindF(KFile2)>=0);
	entry=(*dirList)[2];
	test(entry.iName.FindF(KFile4)>=0);
	entry=(*dirList)[3];
	test(entry.iName.FindF(KFile1)>=0);
	entry=(*dirList)[4];
	test(entry.iName.FindF(KFile6)>=0);
	entry=(*dirList)[5];
	test(entry.iName.FindF(KFile7)>=0);
	entry=(*dirList)[6];
	test(entry.iName.FindF(KFile3)>=0);
	delete dirList;
	dirList = 0;


	test.Next(_L("Test ESortByExt (descending)"));

	r=TheFs.GetDir(sortSpec, KEntryAttNormal | KEntryAttDir, ESortByExt | EDirsLast | EDescending, dirList);
	test_KErrNone(r);
	count=dirList->Count();
	test(count==numFiles);


	for (i = 0; i < count; i++)
		{
		test.Printf(KPrintFileName, &(*dirList)[i].iName);
		}
	
	
	//
	// Verify that the files have been sorted correctly by extension
	// Note that this listing should be the reverse of that above.
	//
	entry=(*dirList)[0];
	test(entry.iName.FindF(KFile3)>=0);
	entry=(*dirList)[1];
	test(entry.iName.FindF(KFile7)>=0);
	entry=(*dirList)[2];
	test(entry.iName.FindF(KFile6)>=0);
	entry=(*dirList)[3];
	test(entry.iName.FindF(KFile1)>=0);
	entry=(*dirList)[4];
	test(entry.iName.FindF(KFile4)>=0);
	entry=(*dirList)[5];
	test(entry.iName.FindF(KFile2)>=0);
	entry=(*dirList)[6];
	test(entry.iName.FindF(KFile5)>=0);
	delete dirList;
	dirList = 0;

	
	DeleteTestDirectoryStructure(KSortByExtPath);
	}

//--------------------------------------------- 
//! @SYMTestCaseID			PBASE-T_DIRS-1310
//! @SYMTestType			CT 
//! @SYMREQ					DEF125143
//! @SYMTestCaseDesc		Test that directory name is handled by File Server interfaces properly.
//! @SYMTestActions			Uses RFs::IsValidName(), RFs::MkDir(), RDir::Open(), RFs::RmDir() to test
//!							various dir name handling.
//! @SYMTestExpectedResults	Proper error code is returned.
//! @SYMTestPriority		High
//! @SYMTestStatus			Implemented 
//--------------------------------------------- 	
void TestDirNameHandling()
	{	
	test.Next(_L("Test Dir Name Handling Interfaces"));
	TFileName dirTest1;
	dirTest1 = _L("\\F32-TST\\TDIRS\\test1\\FILE.TXT");
	TFileName dirTest2;
	dirTest2 = _L("\\F32-TST\\TDIRS\\test2.\\FILE.TXT");
	TFileName dirTest3;
	dirTest3 = _L("\\F32-TST\\TDIRS\\test3. \\FILE.TXT");
	TFileName dirTest4;
	dirTest4 = _L("\\F32-TST\\TDIRS\\test4. . \\FILE.TXT");
	TFileName dirTest5;
	dirTest5 = _L("\\F32-TST\\TDIRS\\test5.\\FILE.TXT");
	TFileName dirTest6;
	dirTest6 = _L("\\F32-TST\\TDIRS\\test6. .\\FILE.TXT");

    TBool valid = TheFs.IsValidName( dirTest1 );
    test(valid);
    valid = TheFs.IsValidName( dirTest2 );
    test(!valid);
    valid = TheFs.IsValidName( dirTest3 );
    test(!valid);
    valid = TheFs.IsValidName( dirTest4 );
    test(!valid);
    valid = TheFs.IsValidName( dirTest5 );
    test(!valid);
    valid = TheFs.IsValidName( dirTest6 );
    test(!valid);

	dirTest1 = _L("\\F32-TST\\TDIRS\\test1\\");
	dirTest2 = _L("\\F32-TST\\TDIRS\\test2.\\");
	dirTest3 = _L("\\F32-TST\\TDIRS\\test3. \\");
	dirTest4 = _L("\\F32-TST\\TDIRS\\test4. . \\");
	dirTest5 = _L("\\F32-TST\\TDIRS\\test5.\\");
	dirTest6 = _L("\\F32-TST\\TDIRS\\test6. .\\");

    TInt err = TheFs.MkDir(dirTest1);
    test_KErrNone(err);
    err = TheFs.MkDir(dirTest2);
    test_Value(err, err == KErrBadName);
    err = TheFs.MkDir(dirTest3);
    test_Value(err, err == KErrBadName);
    err = TheFs.MkDir(dirTest4);
    test_Value(err, err == KErrBadName);
    err = TheFs.MkDir(dirTest5);
    test_Value(err, err == KErrBadName);
    err = TheFs.MkDir(dirTest6);
    test_Value(err, err == KErrBadName);

    RDir rdir;
    err = rdir.Open(TheFs, dirTest1, 0);
    rdir.Close();
    test_KErrNone(err);

    err = rdir.Open(TheFs, dirTest2, 0);
    rdir.Close();
    test_Value(err, err == KErrBadName);

    err = rdir.Open(TheFs, dirTest3, 0);
    rdir.Close();
    test_Value(err, err == KErrBadName);

    err = rdir.Open(TheFs, dirTest4, 0);
    rdir.Close();
    test_Value(err, err == KErrBadName);

    err = rdir.Open(TheFs, dirTest5, 0);
    rdir.Close();
    test_Value(err, err == KErrBadName);

    err = rdir.Open(TheFs, dirTest6, 0);
    rdir.Close();
    test_Value(err, err == KErrBadName);

    err = TheFs.RmDir(dirTest1);
    test_KErrNone(err);
    err = TheFs.RmDir(dirTest2);
    test_Value(err, err == KErrBadName);
    err = TheFs.RmDir(dirTest3);
    test_Value(err, err == KErrBadName);
    err = TheFs.RmDir(dirTest4);
    test_Value(err, err == KErrBadName);
    err = TheFs.RmDir(dirTest5);
    test_Value(err, err == KErrBadName);
    err = TheFs.RmDir(dirTest6);
    test_Value(err, err == KErrBadName);
	}

void CallTestsL()
//
// Do all tests
//
	{

    //-- set up console output 
    F32_Test_Utils::SetConsole(test.Console()); 
    
    TInt nRes=TheFs.CharToDrive(gDriveToTest, gDriveNum);
    test_KErrNone(nRes);
    
    PrintDrvInfo(TheFs, gDriveNum);

	TurnAllocFailureOff();
	CreateTestDirectory(_L("\\F32-TST\\TDIRS\\"));
	
	if (!gTestedZ)
		{
		TInt r=TheFs.SetSessionPath(_L("Z:\\"));
		test_KErrNone(r);
		Test2();
		TestZ();
		r=TheFs.SetSessionPath(gSessionPath);
		test_KErrNone(r);
		test.Next(_L("Run all other tests from \\F32-TST\\TDIRS\\"));
		gTestedZ=ETrue;
		}

	Test8();
	
	Test7(); 

	Test1();
	Test2();
	Test3();
	Test4();
	Test5();
	Test6();
	Test9();

	TestSortByName();
	TestSortByExt();
	TestDirNameHandling();

	CleanupL();
	}
