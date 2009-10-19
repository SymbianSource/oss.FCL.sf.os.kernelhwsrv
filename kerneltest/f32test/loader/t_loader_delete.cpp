// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// \f32test\loader\t_loader_delete.cpp
// 
//


#include <e32test.h>
#include <f32file.h>

#include "t_loader_delete.h"

static RTest test(_L("t_loader_delete"));

// helper functions
static void TestWithCaps(TUint32 aCaps, TInt aExpectedError);
static void TestWithCaps(TUint32 aCaps, TInt aExpectedError, const TDesC& aFileName);
static void SetHelperCaps(TUint32 aCaps);
static void CreateTestFile(RFs& aFs, const TDesC& aTestFile);
static void RunHelper(const TDesC& aFileToDelete, TInt aExpectedError);

static void TestWithCaps(TUint32 aCaps, TInt aExpectedError)
/**
	Test calling RLoader::Delete from a process with the supplied capabilities.

	@param	aCapMask		Capabilities of process which calls RLoader::Delete.
	@param	aExpectedError	Expected error reason.  The launched executable is expected
							to panic with category KTldPanicCat and this reason, which
							is the expected return code from RLoader::Delete.
 */
	{
	TestWithCaps(aCaps, aExpectedError, KTldTcbFile);
	TestWithCaps(aCaps, aExpectedError, KTldAllFilesFile);

	// the following function function calls should fail with either
	// KErrPermissionDenied if this process is not TCB+AllFiles, or with KErrBadName
	// because the filename is not fully qualified.
	TBool pdExp = (aExpectedError == KErrPermissionDenied);
	
	// test filenames which are not fully qualified
	TInt remapErr = pdExp ? KErrPermissionDenied : KErrBadName;
	TestWithCaps(aCaps, remapErr, KTldFileNoPath);
	TestWithCaps(aCaps, remapErr, KTldFileNoDrive);
	
	// test cannot delete non-existent file
	TInt rootNonExistErr = pdExp ? KErrPermissionDenied : KErrNotFound;
	TestWithCaps(aCaps, rootNonExistErr, KTldFileNonExistRoot);
	TInt dirNonExistErr = pdExp ? KErrPermissionDenied : KErrPathNotFound;
	TestWithCaps(aCaps, dirNonExistErr, KTldFileNonExistDir);
	}

static void TestWithCaps(TUint32 aCaps, TInt aExpectedError, const TDesC& aFileName)
/**
	Helper function for TestWithCaps(TUint32, TInt).  This function invokes
	a helper executable with the supplied capabilities and tells it to delete
	the supplied filename.
	
 	@param	aCapMask		Capabilities of process which calls RLoader::Delete.
	@param	aExpectedError	Expected error reason.  The launched executable is expected
							to panic with category KTldPanicCat and this reason, which
							is the expected return code from RLoader::Delete.
	@param	aFileName		The helper executable is told to delete this file.
*/
	{
	test.Printf(
		_L("TestWithCaps,aCaps=0x%x,aExpectedError=%d,aFileName=\"%S\"\n"),
		aCaps, aExpectedError, &aFileName);

	TInt r;

	// create the file to delete
	RFs fs;
	r = fs.Connect();
	test(r == KErrNone);

	// if this file is expected to exist then create it
	TPtrC dirName;
	TBool shouldExist = (aFileName == KTldTcbFile || aFileName == KTldAllFilesFile);
	if (shouldExist)
		{
		TParsePtrC pp(aFileName);
		dirName.Set(pp.DriveAndPath());
		r = fs.MkDirAll(dirName);
		test(r == KErrNone || r == KErrAlreadyExists);
		CreateTestFile(fs, aFileName);
		}

	SetHelperCaps(aCaps);
	RunHelper(aFileName, aExpectedError);

	if (shouldExist)
		{
		// if the file could not be deleted then delete it now
		TEntry e;
		// a C++ bool for the following equality operator
		bool exists = (fs.Entry(aFileName, e) == KErrNone);
		test(exists == (aExpectedError != KErrNone));
		
		if (exists)
			{
			r = fs.Delete(aFileName);
			test(r == KErrNone);
			}

		// delete the immediate containing directory.  The error code is not
		// used because the directory may be used for something else.
		fs.RmDir(dirName);
		}

	// delete the generated different-caps file
	r = fs.Delete(_L("c:\\sys\\bin\\tld_helper_caps.exe"));
	test(r == KErrNone);
	r = fs.Delete(_L("c:\\sys\\hash\\tld_helper_caps.exe"));
	test(r == KErrNone || r == KErrNotFound || r == KErrPathNotFound);

	fs.Close();
	}

static void SetHelperCaps(TUint32 aCaps)
/**
	Create an instance of the helper executable, tld_helper.exe,
	with the supplied capabilities.
 */
	{
	TInt r;
	_LIT(KCommandLineArgsFormat, "tld_helper.exe %x c:\\sys\\bin\\tld_helper_caps.exe");
	TBuf<128> cmdLine;
	cmdLine.Format(KCommandLineArgsFormat, aCaps);

	RProcess p;
	r = p.Create(_L("setcap.exe"), cmdLine);
	test(r == KErrNone);

	TRequestStatus rs;
	p.Logon(rs);
	test(rs == KRequestPending);
	p.Resume();
	User::WaitForRequest(rs);

	p.Close();
	}

static void CreateTestFile(RFs& aFs, const TDesC& aTestFile)
/**
	Create an empty file with the supplied name.  This function is used
	to create file which can be deleted with RLoader::Delete.
	
	@param	aFs				Open file server session.
	@param	aTestFile		The test file's name.
 */
	{
	TInt r;

	RFile f;
	r = f.Replace(aFs, aTestFile, EFileWrite | EFileStream | EFileShareExclusive);
	test(r == KErrNone);
	f.Close();
	}

static void RunHelper(const TDesC& aFileToDelete, TInt aExpectedError)
/**
	Invoke the helper executable, tell it to delete the supplied file.
	
	@param	aFileToDelete	Name of file which helper executable should delete
							with RLoader::Delete.
	@param	aExpectedError	The expected return code from RLoader::Delete.
 */
	{
	TInt r;
	
	// run the helper exe, which will try to delete the file with RLoader::Delete
	RProcess ph;
	r = ph.Create(_L("tld_helper_caps.exe"), aFileToDelete);
	test(r == KErrNone);

	TRequestStatus rsh;
	ph.Logon(rsh);
	test(rsh == KRequestPending);
	ph.Resume();
	User::WaitForRequest(rsh);

	// process has died so check the panic category and reason match the expected values
	test(ph.ExitType() == EExitPanic);
	test(ph.ExitCategory() == KTldPanicCat);
	test(ph.ExitReason() == aExpectedError);

	ph.Close();
	}

TInt E32Main()
/** 
	Executable entrypoint calls test functions within heap check.
 */
	{
	test.Title();
	test.Start(_L("Testing RLoader::Delete"));

	__UHEAP_MARK;
	const TUint32 KTcbMask = 1UL << ECapabilityTCB;
	const TUint32 KAllFilesMask = 1UL << ECapabilityAllFiles;

	//Check whether RLoader::Delete handles the case of a bad descriptor being passed 
	//as the filename( KBadDescriptor is not itself the malformed desciptor but
	//it trigers the check)
	TestWithCaps(KTcbMask | KAllFilesMask     , KErrBadDescriptor, KBadDescriptor);

	// TCB | AllFiles sufficient without any other caps
	TestWithCaps(KTcbMask | KAllFilesMask, KErrNone);
	// TCB necessary
	TestWithCaps(~KTcbMask, KErrPermissionDenied);
	// AllFiles necessary
	TestWithCaps(~KAllFilesMask, KErrPermissionDenied);
	// neither TCB nor AllFiles
	TestWithCaps(~(KTcbMask | KAllFilesMask), KErrPermissionDenied);
	TestWithCaps(0, KErrPermissionDenied);
	__UHEAP_MARKEND;

	test.End();
	return KErrNone;
    }


