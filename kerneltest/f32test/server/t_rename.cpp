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

#include <f32file.h>
#include <e32test.h>
#include "t_server.h"

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

GLDEF_D RTest test(_L("T_RENAME"));

TBuf8<26> alphaBuffer=_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
TPtr8 alphaPtr((TText8*)alphaBuffer.Ptr(),alphaBuffer.Size(),alphaBuffer.Size());

TBuf8<17> BeckBuffer=_L8("A Devil's Haircut");
TPtr8 BeckPtr((TText8*)BeckBuffer.Ptr(),BeckBuffer.Size(),BeckBuffer.Size());

/*

	What this test is for:
	Tests bug fix for the bug which created two files of the same name
 
*/

LOCAL_C void CreateTestFiles()
//
//
//
	{
	test.Next(_L("Create test files"));
	TInt r=TheFs.MkDir(_L("\\F32-TST\\"));
	test(r==KErrNone || r==KErrAlreadyExists);

	RFile file;

//	Create \\SessionPath\\testfile
	r=file.Replace(TheFs,_L("\\F32-TST\\testfile"),EFileRead|EFileWrite);
	test(r==KErrNone);
	r=file.Write(BeckPtr);
	test(r==KErrNone);
	file.Close();
	
//	Create \\SessionPath\\rfsfile
	r=file.Replace(TheFs,_L("\\F32-TST\\rfsfile"),EFileRead|EFileWrite);
	test(r==KErrNone);
	r=file.Write(BeckPtr);
	test(r==KErrNone);
	file.Close();

//	Create \\SessionPath\\eikfile
	r=file.Replace(TheFs,_L("\\F32-TST\\eikfile"),EFileRead|EFileWrite);
	test(r==KErrNone);
	r=file.Write(BeckPtr);
	test(r==KErrNone);
	file.Close();

	}
/*
LOCAL_C void CleanUp()
//
//	Delete any files created by the tests
//
	{
	TInt r=TheFs.Delete(_L("\\F32-TST\\TESTFILE"));
	test(r==KErrNone);
	r=TheFs.Delete(_L("\\F32-TST\\RFSFILE"));
	test(r==KErrNone);
	r=TheFs.Delete(_L("\\F32-TST\\EIKFILE"));
	test(r==KErrNone);
	r=TheFs.Delete(_L("\\F32-TST\\TEST"));
	test(r==KErrNone);
	r=TheFs.RmDir(_L("\\F32-TST\\SYSTEM\\"));
	test(r==KErrNone);
	}
*/

LOCAL_C TInt CountFiles(TPtrC aDirectory, TPtrC aFileName)
//
//	Return the number of files of aFileName found in aDirectory
//	
	{
	RDir dir;
	TFileName sessionPath;
	TInt r=TheFs.SessionPath(sessionPath);
	test(r==KErrNone);
	TFileName path=_L("?:");
	path[0]=sessionPath[0];
	path+=aDirectory;
	if (path[path.Length()-1]==KPathDelimiter)
		path.Append('*');
	else
		path.Append(_L("\\*"));
		
	r=dir.Open(TheFs,path,KEntryAttMaskSupported);
	test(r==KErrNone);

	CDir* anEntryList;
	r=TheFs.GetDir(path,KEntryAttMaskSupported,ESortByName,anEntryList);
	test(r==KErrNone);

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

LOCAL_C void TestRFileRename()
//
//	Test RFile::Rename() function
//
	{
	test.Next(_L("Rename file with DOS compatible name using RFile function"));
	TInt r;
	RFile file;
	
	r=file.Open(TheFs,_L("\\F32-TST\\testfile"),EFileRead|EFileWrite);
	test(r==KErrNone);

	r=file.Rename(_L("\\F32-TST\\TESTFILE"));
	test(r==KErrNone);

	file.Close();

	test.Next(_L("Write in some data"));
	r=file.Open(TheFs,_L("\\F32-TST\\TESTFILE"),EFileRead|EFileWrite);
	test(r==KErrNone);

	r=file.Write(alphaPtr);
	test(r==KErrNone);

	file.Close();
	}


LOCAL_C void TestRFsRename()
//
//	Test RFs::Rename() function
//
	{
	test.Next(_L("Rename file with DOS compatible name using RFs function"));
	TInt r;
		
	r=TheFs.Rename(_L("\\F32-TST\\rfsfile"),_L("\\F32-TST\\RFSFILE"));
	test(r==KErrNone);

	RFile file;
	test.Next(_L("Write in some data"));
	r=file.Open(TheFs,_L("\\F32-TST\\RFSFILE"),EFileRead|EFileWrite);
	test(r==KErrNone);

	r=file.Write(alphaPtr);
	test(r==KErrNone);

	file.Close();
	}

LOCAL_C void TestEikonRename()
//
//	Test EIKON style rename by creating a new file, and copying old data into new file
//
	{
	test.Next(_L("Rename file with DOS compatible name simulating EIKON"));
	TInt r;
	RFile file;
	
	test.Next(_L("Create a new file with DOS compatible equivalent name"));
	r=file.Create(TheFs,_L("\\F32-TST\\EIKFILE"),EFileRead|EFileWrite);
	test((r==KErrNone)||(r==KErrAlreadyExists));
	file.Close();

	test.Next(_L("Copy data from original file into new file"));
	r=TheFs.Replace(_L("\\F32-TST\\eikfile"),_L("\\F32-TST\\EIKFILE"));
	test(r==KErrNone);

	test.Next(_L("Open the new file and write into it"));
	r=file.Open(TheFs,_L("\\F32-TST\\EIKFILE"),EFileRead|EFileWrite);
	test(r==KErrNone);

	r=file.Write(alphaPtr);
	test(r==KErrNone);

	file.Close();
	}


LOCAL_C void TestReplaceAndRename()
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
	test(r==KErrNone);
	r=file.Write(BeckPtr);
	test(r==KErrNone);
	file.Close();

	r=TheFs.Rename(_L("\\F32-TST\\test"),_L("\\F32-TST\\TEST"));
	test(r==KErrNone);
	r=file.Replace(TheFs,_L("\\F32-TST\\temp"),EFileWrite);
	test(r==KErrNone);
	r=file.Write(alphaPtr);
	test(r==KErrNone);
	file.Close();

//	Replace(oldName, newName)	
//	Copy oldName to newName (ie temp to TEST)
//	If TEST does not exist, it is created and then temp's attributes etc are copied into it 
//	then temp is deleted.  If it does exist, it must be closed
//	The bug created a second file of the same name	
	r=TheFs.Replace(_L("\\F32-TST\\temp"),_L("\\F32-TST\\TEST"));
	test(r==KErrNone);

//	Check that there's only one file named TEST
	TInt fileCount=0;
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("TEST"));
	test(fileCount==1);
	r=TheFs.Delete(_L("\\F32-TST\\TEST"));
	test(r==KErrNone);
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("TEST"));
	test(fileCount==0);
	test(r==KErrNone);
	

//*****************************************************
//	The same test but with different source directories
	test.Next(_L("Rename test to and replace \\SYSTEM\\temp with TEST"));
	r=file.Replace(TheFs,_L("\\F32-TST\\test"),EFileWrite);
	test(r==KErrNone);
	r=file.Write(BeckPtr);
	test(r==KErrNone);
	file.Close();

	r=TheFs.Rename(_L("\\F32-TST\\test"),_L("\\F32-TST\\TEST"));
	test(r==KErrNone);
	r=file.Replace(TheFs,_L("\\F32-TST\\SYSTEM\\temp"),EFileWrite);
	test(r==KErrNone);
	r=file.Write(alphaPtr);
	test(r==KErrNone);
	file.Close();
	
//	The bug created a second file of the same name	
	r=TheFs.Replace(_L("\\F32-TST\\SYSTEM\\temp"),_L("\\F32-TST\\TEST"));
	test(r==KErrNone);

	fileCount=0;
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("TEST"));
	test(fileCount==1);
	r=TheFs.Delete(_L("\\F32-TST\\TEST"));
	test(r==KErrNone);
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("TEST"));
	test(fileCount==0);
//	Test that system directory is now empty	
	fileCount=0;
	fileCount=CountFiles(_L("\\F32-TST\\SYSTEM\\"),_L("temp"));
	test(fileCount==0);
	test(r==KErrNone);

//	*************************************************************************
//	Test with a DOS compatible name renamed to a different DOS compatible name
	test.Next(_L("Rename little to BIG and replace temp with BIG"));
	r=file.Replace(TheFs,_L("\\F32-TST\\little"),EFileWrite);
	test(r==KErrNone);
	r=file.Write(BeckPtr);
	test(r==KErrNone);
	file.Close();

	// Test a long path (>250 chrs)
	r=TheFs.Rename(_L("\\F32-TST\\little"),_L("\\F32-TST\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\0495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\PLATTEST\\FileStore\\TestData\\20495_Folder\\middle.gif"));
	test(r==KErrBadName);

	r=TheFs.Rename(_L("\\F32-TST\\little"),_L("\\F32-TST\\BIG"));
	test(r==KErrNone);
	r=file.Replace(TheFs,_L("\\F32-TST\\temp"),EFileWrite);
	test(r==KErrNone);
	r=file.Write(alphaPtr);
	test(r==KErrNone);
	file.Close();
	
	r=TheFs.Replace(_L("\\F32-TST\\temp"),_L("\\F32-TST\\BIG"));
	test(r==KErrNone);

	fileCount=0;
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("BIG"));
	test(fileCount==1);
	r=TheFs.Delete(_L("\\F32-TST\\BIG"));
	test(r==KErrNone);
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("BIG"));
	test(fileCount==0);
	test(r==KErrNone);

//	***********************************	
//	Test with a non-DOS compatible name
	test.Next(_L("Rename veryLongFileName to VERYLONGFILENAME"));
	r=file.Replace(TheFs,_L("\\F32-TST\\veryLongFileName"),EFileWrite);
	test(r==KErrNone);
	r=file.Write(BeckPtr);
	test(r==KErrNone);
	file.Close();

	r=TheFs.Rename(_L("\\F32-TST\\veryLongFileName"),_L("\\F32-TST\\VERYLONGFILENAME"));
	test(r==KErrNone);
	r=file.Replace(TheFs,_L("\\F32-TST\\temp"),EFileWrite);
	test(r==KErrNone);
	r=file.Write(alphaPtr);
	test(r==KErrNone);
	file.Close();
	
	r=TheFs.Replace(_L("\\F32-TST\\temp"),_L("\\F32-TST\\VERYLONGFILENAME"));
	test(r==KErrNone);

	fileCount=0;
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("VERYLONGFILENAME"));
	test(fileCount==1);
	r=TheFs.Delete(_L("\\F32-TST\\VERYLONGFILENAME"));
	test(r==KErrNone);
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("VERYLONGFILENAME"));
	test(fileCount==0);
	test(r==KErrNone);

//	*******************************
//	Test with a DOS compatible name
	test.Next(_L("Rename FILE to FILE and replace temp with FILE"));
	r=file.Replace(TheFs,_L("\\F32-TST\\FILE"),EFileWrite);
	test(r==KErrNone);
	r=file.Write(BeckPtr);
	test(r==KErrNone);
	file.Close();

	r=TheFs.Rename(_L("\\F32-TST\\FILE"),_L("\\F32-TST\\FILE"));
	test(r==KErrNone);
	r=file.Replace(TheFs,_L("\\F32-TST\\temp"),EFileWrite);
	test(r==KErrNone);
	r=file.Write(alphaPtr);
	test(r==KErrNone);
	file.Close();
	
	r=TheFs.Replace(_L("\\F32-TST\\temp"),_L("\\F32-TST\\FILE"));
	test(r==KErrNone);

	fileCount=0;
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("FILE"));
	test(fileCount==1);
	r=TheFs.Delete(_L("\\F32-TST\\FILE"));
	test(r==KErrNone);
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("FILE"));
	test(fileCount==0);
	test(r==KErrNone);

//	**************************************************
//	Test with a DOS compatible name which is kept open
	test.Next(_L("Rename test1 to TEST1 and replace temp1 with TEST1 while it's open"));
	r=file.Replace(TheFs,_L("\\F32-TST\\test1"),EFileWrite);
	test(r==KErrNone);
	r=file.Write(BeckPtr);
	test(r==KErrNone);
	file.Close();

	r=TheFs.Rename(_L("\\F32-TST\\test1"),_L("\\F32-TST\\TEST1"));
	test(r==KErrNone);

//	Try with the file open
	RFile openFile;
	r=openFile.Open(TheFs,_L("\\F32-TST\\TEST1"),EFileRead|EFileWrite);
	test(r==KErrNone);

	r=file.Replace(TheFs,_L("\\F32-TST\\temp"),EFileWrite);
	test(r==KErrNone);
	r=file.Write(alphaPtr);
	test(r==KErrNone);
	file.Close();
	
	r=TheFs.Replace(_L("\\F32-TST\\temp"),_L("\\F32-TST\\TEST1"));
	test(r==KErrInUse);	//	Fails as it should!  But not intuitive bearing in mind the other bug...

	openFile.Close();
	
	r=TheFs.Replace(_L("\\F32-TST\\temp"),_L("\\F32-TST\\TEST1"));
	test(r==KErrNone);

	fileCount=0;
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("TEST1"));
	test(fileCount==1);
	r=TheFs.Delete(_L("\\F32-TST\\TEST1"));
	test(r==KErrNone);
	fileCount=CountFiles(_L("\\F32-TST\\"),_L("TEST1"));
	test(fileCount==0);
	test(r==KErrNone);

//	Clean up
	RFormat format;
	TInt count;
	TFileName sessionPath;
	r=TheFs.SessionPath(sessionPath);
	r=format.Open(TheFs,sessionPath,EQuickFormat,count);
	if (r == KErrAccessDenied)
		return;
	test(r==KErrNone);
	while(count && r==KErrNone)
		r=format.Next(count);
	format.Close();
	}


GLDEF_C void CallTestsL(void)
//
// Do all tests
//
	{
	
	test.Title();
	test.Start(_L("Testing rename"));

	TheFs.MkDir(_L("\\F32-TST\\SYSTEM\\"));
	CreateTestFiles();
	TestRFsRename();
	TestRFileRename();
	TestEikonRename();
	TestReplaceAndRename();

	test.End();
	test.Close();
	}

