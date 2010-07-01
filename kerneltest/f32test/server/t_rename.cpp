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
// f32test\server\t_rename.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include "t_server.h"
#include "f32_test_utils.h"

using namespace F32_Test_Utils;
TInt gDriveNum = -1;

RTest test(_L("T_RENAME"));

TBuf8<26> alphaBuffer=_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
TPtr8 alphaPtr((TText8*)alphaBuffer.Ptr(),alphaBuffer.Size(),alphaBuffer.Size());

TBuf8<17> BeckBuffer=_L8("A Devil's Haircut");
TPtr8 BeckPtr((TText8*)BeckBuffer.Ptr(),BeckBuffer.Size(),BeckBuffer.Size());


static void CreateTestFiles()
	{
	test.Next(_L("Create test files"));
	TInt r=TheFs.MkDir(_L("\\F32-TST\\"));
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);

	RFile file;

//	Create \\SessionPath\\testfile
	r=file.Replace(TheFs,_L("\\F32-TST\\testfile"),EFileRead|EFileWrite);
	test_KErrNone(r);
	r=file.Write(BeckPtr);
	test_KErrNone(r);
	file.Close();
	
//	Create \\SessionPath\\rfsfile
	r=file.Replace(TheFs,_L("\\F32-TST\\rfsfile"),EFileRead|EFileWrite);
	test_KErrNone(r);
	r=file.Write(BeckPtr);
	test_KErrNone(r);
	file.Close();

//	Create \\SessionPath\\eikfile
	r=file.Replace(TheFs,_L("\\F32-TST\\eikfile"),EFileRead|EFileWrite);
	test_KErrNone(r);
	r=file.Write(BeckPtr);
	test_KErrNone(r);
	file.Close();

	}

static TInt CountFiles(TPtrC aDirectory, TPtrC aFileName)
//
//	Return the number of files of aFileName found in aDirectory
//	
	{
	
    RDir dir;
	TFileName sessionPath;
	TInt r=TheFs.SessionPath(sessionPath);
	test_KErrNone(r);
	TFileName path=_L("?:");
	path[0]=sessionPath[0];
	path+=aDirectory;
	if (path[path.Length()-1]==KPathDelimiter)
		path.Append('*');
	else
		path.Append(_L("\\*"));
		
	r=dir.Open(TheFs,path,KEntryAttMaskSupported);
	test_KErrNone(r);

	CDir* anEntryList;
	r=TheFs.GetDir(path,KEntryAttMaskSupported,ESortByName,anEntryList);
	test_KErrNone(r);

//	Sets the new length of path to the position of the last path delimiter +1
	path.SetLength(path.LocateReverse(KPathDelimiter)+1);
	TInt fileCount=0; 
	TEntry entry;
		
	TInt count=anEntryList->Count();
	for (TInt j=0;j<count;j++)
		{
		entry=anEntryList->operator[](j);
		if ((entry.iName)==(aFileName))
			fileCount++;
		}
	
	dir.Close();
	delete anEntryList;
	return(fileCount);
	}

static void TestRFileRename()
//
//	Test RFile::Rename() function
//
	{
	test.Next(_L("Rename file with DOS compatible name using RFile function"));
	TInt r;
	RFile file;
	
	r=file.Open(TheFs,_L("\\F32-TST\\testfile"),EFileRead|EFileWrite);
	test_KErrNone(r);

	r=file.Rename(_L("\\F32-TST\\TESTFILE"));
	test_KErrNone(r);

	file.Close();

	test.Next(_L("Write in some data"));
	r=file.Open(TheFs,_L("\\F32-TST\\TESTFILE"),EFileRead|EFileWrite);
	test_KErrNone(r);

	r=file.Write(alphaPtr);
	test_KErrNone(r);

	file.Close();
	}


static void TestRFsRename()
//
//	Test RFs::Rename() function
//
	{
	test.Next(_L("Rename file with DOS compatible name using RFs function"));
	TInt r;
		
	r=TheFs.Rename(_L("\\F32-TST\\rfsfile"),_L("\\F32-TST\\RFSFILE"));
	test_KErrNone(r);

	RFile file;
	test.Next(_L("Write in some data"));
	r=file.Open(TheFs,_L("\\F32-TST\\RFSFILE"),EFileRead|EFileWrite);
	test_KErrNone(r);

	r=file.Write(alphaPtr);
	test_KErrNone(r);

	file.Close();
	}

static void TestEikonRename()
//
//	Test EIKON style rename by creating a new file, and copying old data into new file
//
	{
	test.Next(_L("Rename file with DOS compatible name simulating EIKON"));
	TInt r;
	RFile file;
	
	test.Next(_L("Create a new file with DOS compatible equivalent name"));
	r=file.Create(TheFs,_L("\\F32-TST\\EIKFILE"),EFileRead|EFileWrite);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	file.Close();

	test.Next(_L("Copy data from original file into new file"));
	r=TheFs.Replace(_L("\\F32-TST\\eikfile"),_L("\\F32-TST\\EIKFILE"));
	test_KErrNone(r);

	test.Next(_L("Open the new file and write into it"));
	r=file.Open(TheFs,_L("\\F32-TST\\EIKFILE"),EFileRead|EFileWrite);
	test_KErrNone(r);

	r=file.Write(alphaPtr);
	test_KErrNone(r);

	file.Close();
	}


static void TestReplaceAndRename()
//
//	Tests the bug which allows 2 files of the same name to be created has been fixed
//
	{
	TInt r;
	RFile file;

//	*************************************************************************
//	First test with a non DOS compatible name renamed to a DOS compatible name
	test.Next(_L("Rename test to TEST and replace temp with TEST"));
	r=file.Replace(TheFs,_L("\\F32-TST\\test"),EFileWrite);
	test_KErrNone(r);
	r=file.Write(BeckPtr);
	test_KErrNone(r);
	file.Close();

	r=TheFs.Rename(_L("\\F32-TST\\test"),_L("\\F32-TST\\TEST"));
	test_KErrNone(r);
	r=file.Replace(TheFs,_L("\\F32-TST\\temp"),EFileWrite);
	test_KErrNone(r);
	r=file.Write(alphaPtr);
	test_KErrNone(r);
	file.Close();

//	Replace(oldName, newName)	
//	Copy oldName to newName (ie temp to TEST)
//	If TEST does not exist, it is created and then temp's attributes etc are copied into it 
//	then temp is deleted.  If it does exist, it must be closed
//	The bug created a second file of the same name	
	r=TheFs.Replace(_L("\\F32-TST\\temp"),_L("\\F32-TST\\TEST"));
	test_KErrNone(r);

//	Check that there's only one file named TEST
	TInt fileCount=0;
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("TEST"));
	test(fileCount==1);
	r=TheFs.Delete(_L("\\F32-TST\\TEST"));
	test_KErrNone(r);
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("TEST"));
	test(fileCount==0);
	test_KErrNone(r);
	

//*****************************************************
//	The same test but with different source directories
	test.Next(_L("Rename test to and replace \\SYSTEM\\temp with TEST"));
	r=file.Replace(TheFs,_L("\\F32-TST\\test"),EFileWrite);
	test_KErrNone(r);
	r=file.Write(BeckPtr);
	test_KErrNone(r);
	file.Close();

	r=TheFs.Rename(_L("\\F32-TST\\test"),_L("\\F32-TST\\TEST"));
	test_KErrNone(r);
	r=file.Replace(TheFs,_L("\\F32-TST\\SYSTEM\\temp"),EFileWrite);
	test_KErrNone(r);
	r=file.Write(alphaPtr);
	test_KErrNone(r);
	file.Close();
	
//	The bug created a second file of the same name	
	r=TheFs.Replace(_L("\\F32-TST\\SYSTEM\\temp"),_L("\\F32-TST\\TEST"));
	test_KErrNone(r);

	fileCount=0;
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("TEST"));
	test(fileCount==1);
	r=TheFs.Delete(_L("\\F32-TST\\TEST"));
	test_KErrNone(r);
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("TEST"));
	test(fileCount==0);
//	Test that system directory is now empty	
	fileCount=0;
	fileCount=CountFiles(_L("\\F32-TST\\SYSTEM\\"),_L("temp"));
	test(fileCount==0);
	test_KErrNone(r);

//	*************************************************************************
//	Test with a DOS compatible name renamed to a different DOS compatible name
	test.Next(_L("Rename little to BIG and replace temp with BIG"));
	r=file.Replace(TheFs,_L("\\F32-TST\\little"),EFileWrite);
	test_KErrNone(r);
	r=file.Write(BeckPtr);
	test_KErrNone(r);
	file.Close();

	// Test a long path (>250 chrs)
	r=TheFs.Rename(_L("\\F32-TST\\little"),_L("\\F32-TST\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\0495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\middle.gif"));
	test_Value(r, r == KErrBadName);

	r=TheFs.Rename(_L("\\F32-TST\\little"),_L("\\F32-TST\\BIG"));
	test_KErrNone(r);
	r=file.Replace(TheFs,_L("\\F32-TST\\temp"),EFileWrite);
	test_KErrNone(r);
	r=file.Write(alphaPtr);
	test_KErrNone(r);
	file.Close();
	
	r=TheFs.Replace(_L("\\F32-TST\\temp"),_L("\\F32-TST\\BIG"));
	test_KErrNone(r);

	fileCount=0;
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("BIG"));
	test(fileCount==1);
	r=TheFs.Delete(_L("\\F32-TST\\BIG"));
	test_KErrNone(r);
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("BIG"));
	test(fileCount==0);
	test_KErrNone(r);

//	***********************************	
//	Test with a non-DOS compatible name
	test.Next(_L("Rename veryLongFileName to VERYLONGFILENAME"));
	r=file.Replace(TheFs,_L("\\F32-TST\\veryLongFileName"),EFileWrite);
	test_KErrNone(r);
	r=file.Write(BeckPtr);
	test_KErrNone(r);
	file.Close();

	r=TheFs.Rename(_L("\\F32-TST\\veryLongFileName"),_L("\\F32-TST\\VERYLONGFILENAME"));
	test_KErrNone(r);
	r=file.Replace(TheFs,_L("\\F32-TST\\temp"),EFileWrite);
	test_KErrNone(r);
	r=file.Write(alphaPtr);
	test_KErrNone(r);
	file.Close();
	
	r=TheFs.Replace(_L("\\F32-TST\\temp"),_L("\\F32-TST\\VERYLONGFILENAME"));
	test_KErrNone(r);

	fileCount=0;
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("VERYLONGFILENAME"));
	test(fileCount==1);
	r=TheFs.Delete(_L("\\F32-TST\\VERYLONGFILENAME"));
	test_KErrNone(r);
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("VERYLONGFILENAME"));
	test(fileCount==0);
	test_KErrNone(r);

//	*******************************
//	Test with a DOS compatible name
	test.Next(_L("Rename FILE to FILE and replace temp with FILE"));
	r=file.Replace(TheFs,_L("\\F32-TST\\FILE"),EFileWrite);
	test_KErrNone(r);
	r=file.Write(BeckPtr);
	test_KErrNone(r);
	file.Close();

	r=TheFs.Rename(_L("\\F32-TST\\FILE"),_L("\\F32-TST\\FILE"));
	test_KErrNone(r);
	r=file.Replace(TheFs,_L("\\F32-TST\\temp"),EFileWrite);
	test_KErrNone(r);
	r=file.Write(alphaPtr);
	test_KErrNone(r);
	file.Close();
	
	r=TheFs.Replace(_L("\\F32-TST\\temp"),_L("\\F32-TST\\FILE"));
	test_KErrNone(r);

	fileCount=0;
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("FILE"));
	test(fileCount==1);
	r=TheFs.Delete(_L("\\F32-TST\\FILE"));
	test_KErrNone(r);
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("FILE"));
	test(fileCount==0);
	test_KErrNone(r);

//	**************************************************
//	Test with a DOS compatible name which is kept open
	test.Next(_L("Rename test1 to TEST1 and replace temp1 with TEST1 while it's open"));
	r=file.Replace(TheFs,_L("\\F32-TST\\test1"),EFileWrite);
	test_KErrNone(r);
	r=file.Write(BeckPtr);
	test_KErrNone(r);
	file.Close();

	r=TheFs.Rename(_L("\\F32-TST\\test1"),_L("\\F32-TST\\TEST1"));
	test_KErrNone(r);

//	Try with the file open
	RFile openFile;
	r=openFile.Open(TheFs,_L("\\F32-TST\\TEST1"),EFileRead|EFileWrite);
	test_KErrNone(r);

	r=file.Replace(TheFs,_L("\\F32-TST\\temp"),EFileWrite);
	test_KErrNone(r);
	r=file.Write(alphaPtr);
	test_KErrNone(r);
	file.Close();
	
	r=TheFs.Replace(_L("\\F32-TST\\temp"),_L("\\F32-TST\\TEST1"));
	test_Value(r, r == KErrInUse);	//	Fails as it should!  But not intuitive bearing in mind the other bug...

	openFile.Close();
	
	r=TheFs.Replace(_L("\\F32-TST\\temp"),_L("\\F32-TST\\TEST1"));
	test_KErrNone(r);

	fileCount=0;
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("TEST1"));
	test(fileCount==1);
	r=TheFs.Delete(_L("\\F32-TST\\TEST1"));
	test_KErrNone(r);
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("TEST1"));
	test(fileCount==0);
	test_KErrNone(r);

	}


//-------------------------------------------------------------------
/**
    Create a directory; create many files in it (the directory will become more that 1 cluster)
    Then rename every file in this directory to a new name.
*/
void TestRenameManyFilesInTheSameDir()
{
    test.Next(_L("TestRenameManyFilesInTheSameDir"));
    
    if(Is_Win32(TheFs, gDriveNum))
    {
        test.Printf(_L("Skipping on WINS drive\n"));
        return;
    }

    _LIT(KDir,  "\\dir1\\");
    _LIT(KFile, "filename_long-");
    
    //-- the number of files is chosen the way to have the directory file at least 2 clusters long (on FAT)
    //-- "filename_long-XXX" will correspond to 2 VFAT entries in the directory; max. cluster size of FAT is 32K
    //--  2*32*600 = 38400 > 32K
    const TInt KNumFiles = 600;
    
    TName   fName;
    TInt    i;
    TInt    nRes;

    //-- quick format the drive 
    nRes = FormatDrive(TheFs, gDriveNum, ETrue); 
    test_KErrNone(nRes);

    MakeDir(KDir);

    //-- create a number of files in a single directory, it shall be larger than 1 cluster.
    for(i=0; i<KNumFiles; ++i)
        {
        fName.Format(_L("%S%S%03d"), &KDir, &KFile, i);   
        nRes = CreateEmptyFile(TheFs, fName, 0);
        test_KErrNone(nRes);
        }

    //-- rename all files in the same directory
    TName   fNameNew;
    for(i=0; i<KNumFiles; ++i)
        {
        fName.Format(_L("%S%S%03d"), &KDir, &KFile, i);   
        fNameNew.Format(_L("%S%S%03d_new"), &KDir, &KFile, i);   

        nRes = TheFs.Rename(fName, fNameNew);
        test_KErrNone(nRes);

        }

   fName.Format(_L("%c:"), gDriveNum+'A');
   nRes = TheFs.CheckDisk(fName);
   test_Value(nRes, nRes == KErrNone || nRes == KErrNotSupported);

   //-- clean up
    for(i=0; i<KNumFiles; ++i)
        {
        fNameNew.Format(_L("%S%S%03d_new"), &KDir, &KFile, i);   
        nRes = TheFs.Delete(fNameNew);
        test_KErrNone(nRes);
        }

   fName.Format(_L("%c:"), gDriveNum+'A');
   nRes = TheFs.CheckDisk(fName);
   test_Value(nRes, nRes == KErrNone || nRes == KErrNotSupported);


   nRes = TheFs.RmDir(KDir);
   test_KErrNone(nRes);


}


void CallTestsL(void)
	{
	
	test.Title();
	test.Start(_L("Testing rename"));

    //-- set up console output
    F32_Test_Utils::SetConsole(test.Console());

    TInt nRes=TheFs.CharToDrive(gDriveToTest, gDriveNum);
    test_KErrNone(nRes);
    
    PrintDrvInfo(TheFs, gDriveNum);

    if(!Is_Win32(TheFs, gDriveNum))
        {
        nRes = FormatDrive(TheFs, gDriveNum, ETrue);
        test_KErrNone(nRes);
        }
    

	MakeDir(_L("\\F32-TST\\SYSTEM\\"));
	CreateTestFiles();
	TestRFsRename();
	TestRFileRename();
	TestEikonRename();
	TestReplaceAndRename();
    TestRenameManyFilesInTheSameDir();

    if(!Is_Win32(TheFs, gDriveNum))
        {
        nRes = FormatDrive(TheFs, gDriveNum, ETrue);
        test_KErrNone(nRes);
        }

	test.End();
	test.Close();
	}

