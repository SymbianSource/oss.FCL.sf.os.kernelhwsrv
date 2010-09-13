// Copyright (c) 2005-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_virus.cpp
// 
//

#include <f32file.h>
#include <e32test.h>
#include <e32svr.h>
#include <f32dbg.h>
#include "t_server.h"

GLREF_C void TestIfEqual( TInt aValue, TInt aExpected, TInt aLine, const char aFileName[]);

#define TEST_FOR_ERROR( r )	TestIfEqual( r, KErrNone, __LINE__, __FILE__)
#define TEST_FOR_VALUE( r, expected ) TestIfEqual( r, expected, __LINE__, __FILE__)

_LIT( KValueTestFailMsg, "ERROR Got %d expected %d" );
GLDEF_C void TestIfEqual( TInt aValue, TInt aExpected, TInt aLine, const char aFileName[])
	{
	if( aExpected != aValue )
		{
		TText filenameU[512];
		TUint i = 0;
		for (; (i < sizeof(filenameU)) && (aFileName[i] != (char)0); i++)
			{
			filenameU[i]=aFileName[i];
			}
		filenameU[i]=0;
		test.Printf( KValueTestFailMsg, aValue, aExpected );
		test.operator()( EFalse, aLine, &filenameU[0]);
		}
	}

GLDEF_D RTest test(_L("T_VIRUS"));


void CleanupFiles()
	{
	test.Next(_L("Delete any files leftover from a previous run"));

	TheFs.SetAtt(_L("C:\\sys\\bin\\t_vshook.pxt"),0, KEntryAttReadOnly);
	TheFs.Delete(_L("C:\\sys\\bin\\t_vshook.pxt"));

	TheFs.SetAtt(_L("C:\\virusdef.txt"),0, KEntryAttReadOnly);
	TheFs.Delete(_L("C:\\virusdef.txt"));
	
	TheFs.SetAtt(_L("virus1.txt"),0, KEntryAttReadOnly);
	TheFs.Delete(_L("virus1.txt"));

	TheFs.SetAtt(_L("virus2.txt"),0, KEntryAttReadOnly);
	TheFs.Delete(_L("virus2.txt"));

	TheFs.SetAtt(_L("c:\\virus3.txt"),0, KEntryAttReadOnly);
	TheFs.Delete(_L("c:\\virus3.txt"));

	TheFs.SetAtt(_L("clean.txt"),0, KEntryAttReadOnly);
	TheFs.Delete(_L("clean.txt"));
	}

void CopyFilesL()
	{
	//So that we have the same file on WINS and a platform,
	//we expect to find all the files on Z:\test.  We then
	//copy the virus scanner vsh and the definition file to
	//the C: drive and the files to be scanned to whatever
	//drive we're testing on.
	test.Next(_L("Copying files to the appropriate places"));

	TInt r = TheFs.MkDirAll(_L("C:\\sys\\bin\\"));
	if (r != KErrAlreadyExists)
		TEST_FOR_ERROR(r);

	//If we're not currently on drive C, make a sys\\bin directory
	//so we can test attempting to rename it
	r = TheFs.MkDirAll(_L("\\sys\\bin\\"));
	if (r != KErrAlreadyExists)
		TEST_FOR_ERROR(r);

	CFileMan* pFileMan = CFileMan::NewL(TheFs);
	r = pFileMan->Copy(_L("Z:\\test\\t_vshook.pxt"), _L("C:\\sys\\bin\\t_vshook.pxt"));
	TEST_FOR_ERROR(r);

	r = pFileMan->Copy(_L("Z:\\test\\virusdef.txt"), _L("C:\\virusdef.txt"));
	TEST_FOR_ERROR(r);

	r = pFileMan->Copy(_L("Z:\\test\\virus1.txt"), _L("virus1.txt"));
	TEST_FOR_ERROR(r);

	r = pFileMan->Copy(_L("Z:\\test\\virus2.txt"), _L("virus2.txt"));
	TEST_FOR_ERROR(r);

	r = pFileMan->Copy(_L("Z:\\test\\clean.txt"), _L("clean.txt"));
	TEST_FOR_ERROR(r);

	delete pFileMan;
	}

void TestFileAccessBeforeVirusScanner()
	{

	//Check that we can read all the files we want before the
	//virus scanner is loaded.
	test.Next(_L("Test opening files before virus scanner is installed"));

	RFile file;
	TBuf8<512> buffer;

	TInt r = file.Open(TheFs, _L("C:\\virusdef.txt"), EFileShareAny);

	TEST_FOR_ERROR(r);
	r = file.Read(buffer);
	TEST_FOR_ERROR(r);
	file.Close();

	r = file.Open(TheFs, _L("virus1.txt"), EFileShareAny);
	TEST_FOR_ERROR(r);
	r = file.Read(buffer);
	TEST_FOR_ERROR(r);
	file.Close();

	r = file.Open(TheFs, _L("virus2.txt"), EFileShareAny);
	TEST_FOR_ERROR(r);
	r = file.Read(buffer);
	TEST_FOR_ERROR(r);
	file.Close();

	r = file.Open(TheFs, _L("clean.txt"), EFileShareAny);
	TEST_FOR_ERROR(r);
	r = file.Read(buffer);
	TEST_FOR_ERROR(r);
	file.Close();
	}

void TestLoadingOfVirusScanner()
	{
	test.Next(_L("Test the loading of the virus scanner"));

	// Try loading the virus scanner.
	TInt r = TheFs.AddPlugin(_L("t_vshook"));
	TEST_FOR_ERROR(r);

	// Try loading the virus scanner again.
	r = TheFs.AddPlugin(_L("t_vshook"));
	TEST_FOR_VALUE(r, KErrAlreadyExists);


	// Try mounting the virus scanner.
	r = TheFs.MountPlugin(_L("VsHook"));
	TEST_FOR_ERROR(r);


	//Test the name functions
	TFullName vsName;
	r = TheFs.PluginName(vsName,0,0);
	test.Printf(_L("Virus scanner name is: %S\n"), &vsName);

	}

void TestUnloadingOfVirusScanner()
	{
	test.Next(_L("Test unloading the virus scanner"));

	//Unload the virus scanner
	//Wait for it to empty it's input queue.
	User::After(3000000);

	TInt r = TheFs.DismountPlugin(_L("VsHook"));
	TEST_FOR_ERROR(r);

	r = TheFs.DismountPlugin(_L("VsHook"));
	TEST_FOR_VALUE(r, KErrNotFound);

	r = TheFs.RemovePlugin(_L("VsHook"));
	TEST_FOR_ERROR(r);

	r = TheFs.RemovePlugin(_L("VsHook"));
	TEST_FOR_VALUE(r, KErrNotFound);
	}


void TestFileAccessDuringVirusScannerL()
	{

	RFile file;
	TBuf8<512> buffer;

	test.Next(_L("Test opening files with the virus scanner installed"));

	TInt r = file.Open(TheFs, _L("C:\\virusdef.txt"), EFileShareAny);
	TEST_FOR_VALUE(r, KErrAccessDenied);

	r = file.Open(TheFs, _L("virus1.txt"), EFileShareAny);
	TEST_FOR_VALUE(r, KErrAccessDenied);

	r = file.Open(TheFs, _L("virus2.txt"), EFileShareAny);
	TEST_FOR_VALUE(r, KErrAccessDenied);

	r = file.Open(TheFs, _L("C:\\sys\\bin\\t_vshook.pxt"), EFileShareAny);
	TEST_FOR_VALUE(r, KErrAccessDenied);

	// Set up a change notifier
	// - The virus scanner renames files before scanning, so check that the
	//	 internal rename operation hasn't caused a notification.
	TRequestStatus reqStat;
	TheFs.NotifyChange(ENotifyAll, reqStat, _L("C:\\F32-TST\\"));
	TEST_FOR_VALUE(reqStat.Int(), KRequestPending);

	r = file.Open(TheFs, _L("clean.txt"), EFileShareAny);
	TEST_FOR_ERROR(r);
	r = file.Read(buffer);
	TEST_FOR_ERROR(r);
	file.Close();

	// Verify that cleaning file didn't cause a notification
	TEST_FOR_VALUE(reqStat.Int(), KRequestPending);
	TheFs.NotifyChangeCancel(reqStat);

	//Files on Z: are not scanned, so we should be able to open
	//an infected file
	r = file.Open(TheFs, _L("Z:\\test\\virus1.txt"), EFileShareAny);
	TEST_FOR_ERROR(r);
	r = file.Read(buffer);
	TEST_FOR_ERROR(r);
	file.Close();

	test.Next(_L("Test opening of scanner files"));
	r = file.Open(TheFs, _L("C:\\sys\\bin\\t_vshook.pxt"), EFileShareAny);
	TEST_FOR_VALUE(r, KErrAccessDenied);

	r = file.Open(TheFs, _L("C:\\virusdef.txt"), EFileShareAny);
	TEST_FOR_VALUE(r, KErrAccessDenied);



	test.Next(_L("Test renaming of infected files"));
	//With RFs::Rename()
	r = TheFs.Rename(_L("virus1.txt"), _L("renamed1.txt"));
	TEST_FOR_VALUE(r, KErrAccessDenied);

	//Test RFs::Replace()
	r = TheFs.Replace(_L("virus1.txt"), _L("renamed1.txt"));
	TEST_FOR_VALUE(r, KErrAccessDenied);

	//CFileMan opens the file for exclusive access,
	//so wait for the virus scanner to finish.
	User::After(1000000);

	//With CFileMan
	CFileMan* pFileMan = CFileMan::NewL(TheFs);
	r = pFileMan->Rename(_L("virus1.txt"), _L("renamed1.txt"));
	TEST_FOR_VALUE(r, KErrAccessDenied);

	test.Next(_L("Test renaming virus scanner files"));
	//Test renaming of virus definition file
	r = TheFs.Rename(_L("C:\\virusdef.txt"), _L("C:\\renameddef.txt"));
	TEST_FOR_VALUE(r, KErrAccessDenied);

	//With CFileMan
	r = pFileMan->Rename(_L("C:\\virusdef.txt"), _L("C:\\renameddef.txt"));
	TEST_FOR_VALUE(r, KErrAccessDenied);

	//Test renaming of virus scanner hook
	r = TheFs.Rename(_L("C:\\sys\\bin\\t_vshook.pxt"), _L("C:\\sys\\bin\\renames.pxt"));
	TEST_FOR_VALUE(r, KErrAccessDenied);

	//With CFileMan
	r = pFileMan->Rename(_L("C:\\sys\\bin\\t_vshook.pxt"), _L("C:\\sys\\bin\\renames.pxt"));
	TEST_FOR_VALUE(r, KErrAccessDenied);


	test.Next(_L("Test deleting an infected file"));
	r = TheFs.SetAtt(_L("virus2.txt"), 0,KEntryAttReadOnly); 
	TEST_FOR_ERROR(r);
	r = TheFs.Delete(_L("virus2.txt"));
	TEST_FOR_ERROR(r);

	test.Next(_L("Test closing an infected file"));
	r = file.Create(TheFs, _L("c:\\virus3.txt"), EFileWrite | EFileShareAny);
	TEST_FOR_ERROR(r);
	r = file.Write(_L8("This a generated infected file."));
	TEST_FOR_ERROR(r);
	file.Close();

	//Test renaming a directory
	test.Next(_L("Test renaming a directory"));
	r = TheFs.Rename(_L("\\sys\\bin\\"), _L("\\virusdir1\\"));
	TEST_FOR_VALUE(r, KErrAccessDenied);

	delete pFileMan;
	}

void TestReadFileSectionInterceptL()
	{
	TBuf8<16> testDes;

	test.Next(_L("Test ReadFileSection intercept GetName and GetFileAccessInfo support"));	
	// trigger RFs::ReadFileSection intercept
	TInt r = TheFs.ReadFileSection(_L("clean.txt"),0,testDes,8);
	test(r==KErrNone);
	}


void TestFileAccessAfterVirusScanner()
	{

	test.Next(_L("Test opening files after virus scanner is uninstalled"));

	RFile file;
	TBuf8<512> buffer;

	test.Next(_L("Test the virus definition file"));
	TInt r = file.Open(TheFs, _L("C:\\virusdef.txt"), EFileShareAny);
	TEST_FOR_ERROR(r);
	r = file.Read(buffer);
	TEST_FOR_ERROR(r);
	file.Close();

	if (buffer.Length() == buffer.MaxLength())
		{
		//The definition file should be less than 512 bytes in length.
		r = KErrOverflow;
		TEST_FOR_ERROR(r);
		}

	//Make sure that the virus scanner has not "fixed" this file
	r = buffer.Find(_L8("infection"));
	TEST_FOR_VALUE(r, KErrNotFound);

	test.Next(_L("Test the clean file has not been corrected"));
	r = file.Open(TheFs, _L("clean.txt"), EFileShareAny);
	TEST_FOR_ERROR(r);
	r = file.Read(buffer);
	TEST_FOR_ERROR(r);
	file.Close();

	if (buffer.Length() == buffer.MaxLength())
		{
		//The clean file should be less than 512 bytes in length.
		r = KErrOverflow;
		TEST_FOR_ERROR(r);
		}

	//Make sure that the virus scanner has not "fixed" this file
	r = buffer.Find(_L8("infection"));
	TEST_FOR_VALUE(r, KErrNotFound);
	

	//Virus2.txt should have been successfully deleted.
	r = file.Open(TheFs, _L("virus2.txt"), EFileRead);
	TEST_FOR_VALUE(r, KErrNotFound);

	//Virus1.txt should have been corrected several times.
	//Here's the order in which we did things to it in
	//TestFileAccessDuringVirusScannerL():
	//1. Open for read.
	//2. Rename using RFS
	//3. Rename using CFileMan
	test.Next(_L("Test the infected file has been corrected"));
	r = file.Open(TheFs, _L("virus1.txt"), EFileShareAny);
	TEST_FOR_ERROR(r);
	r = file.Read(buffer);
	TEST_FOR_ERROR(r);
	file.Close();

	if (buffer.Length() == buffer.MaxLength())
		{
		//The infected file should be less than 512 bytes in length.
		r = KErrOverflow;
		TEST_FOR_ERROR(r);
		}

	//Make sure that the virus scanner has "fixed" this file,
	//and that the fixing order is correct.
	TInt fixPos1 = buffer.Find(_L8("infection detected - file open"));
	if (fixPos1 < 0)
		{
		TEST_FOR_ERROR(fixPos1);
		}
	TInt fixPos2 = buffer.Find(_L8("infection detected - file rename"));
	if (fixPos2 < 0)
		{
		TEST_FOR_ERROR(fixPos2);
		}
	//The -32 is the length of the "infection detected - file rename"
	TPtrC8 rightPart = buffer.Right((buffer.Length() - fixPos2) - 32);
	TInt fixPos3 = rightPart.Find(_L8("infection detected - file rename"));
	fixPos3 += fixPos2 + 32;
	if (fixPos3 < 0)
		{
		TEST_FOR_ERROR(fixPos3);
		}
	
	if (!(fixPos3 > fixPos2 && fixPos3 > fixPos1 && fixPos2 > fixPos1))
		{
		//The fixes have not been written to the file in the correct
		//order
		r = KErrGeneral;
		TEST_FOR_ERROR(r);
		}


	test.Next(_L("Test a generated infection is detected"));
	r = file.Open(TheFs, _L("c:\\virus3.txt"), EFileShareAny);
	TEST_FOR_ERROR(r);
	r = file.Read(buffer);
	TEST_FOR_ERROR(r);
	file.Close();

	if (buffer.Length() == buffer.MaxLength())
		{
		//The clean file should be less than 512 bytes in length.
		r = KErrOverflow;
		TEST_FOR_ERROR(r);
		}

	//Make sure that the virus scanner has "fixed" this file
	r = buffer.Find(_L8("infection detected - file close"));
	if (r < 0)
		TEST_FOR_ERROR(r);
	}

void DeleteFiles()
	{

	test.Next(_L("Cleanup files"));
	//Delete all of the files created using CopyFilesL() so
	//that the test can run multiple times.
	TInt r = TheFs.SetAtt(_L("C:\\sys\\bin\\t_vshook.pxt"),0, KEntryAttReadOnly);
	TEST_FOR_ERROR(r);
	r = TheFs.Delete(_L("C:\\sys\\bin\\t_vshook.pxt"));
	TEST_FOR_ERROR(r);

	r = TheFs.RmDir(_L("\\sys\\bin\\"));
	if (r != KErrInUse)
		TEST_FOR_ERROR(r);

	r = TheFs.SetAtt(_L("C:\\virusdef.txt"),0, KEntryAttReadOnly);
	TEST_FOR_ERROR(r);
	r = TheFs.Delete(_L("C:\\virusdef.txt"));
	TEST_FOR_ERROR(r);

	r = TheFs.SetAtt(_L("virus1.txt"),0, KEntryAttReadOnly);
	TEST_FOR_ERROR(r);
	r = TheFs.Delete(_L("virus1.txt"));
	TEST_FOR_ERROR(r);

	r = TheFs.SetAtt(_L("c:\\virus3.txt"),0, KEntryAttReadOnly);
	TEST_FOR_ERROR(r);
	r = TheFs.Delete(_L("c:\\virus3.txt"));
	TEST_FOR_ERROR(r);

	r = TheFs.SetAtt(_L("clean.txt"),0, KEntryAttReadOnly);
	TEST_FOR_ERROR(r);
	r = TheFs.Delete(_L("clean.txt"));
	TEST_FOR_ERROR(r);
	}	


GLDEF_C void CallTestsL()
	{

	CleanupFiles();
	CopyFilesL();

	TestFileAccessBeforeVirusScanner();

	TestLoadingOfVirusScanner();

	TestFileAccessDuringVirusScannerL();

	TestReadFileSectionInterceptL();

	TestUnloadingOfVirusScanner();

	TestFileAccessAfterVirusScanner();

	DeleteFiles();

	}

