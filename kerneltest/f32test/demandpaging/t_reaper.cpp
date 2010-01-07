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
// f32test\demandpaging\t_reaper.cpp
//
// Suite of tests for the Reaper, the reaper is used to clean up files
// which were deleted whilst in use, the delete is delayed until the 
// files are no longer in use and then completed by the reaper.
//
// 001 Loader Reaper/Clamp Tests
// 002 Try deleteing file while Open
// 003 Try deleteing file while Clamped and Open
// 004 Try deleteing while Clamped
// 005 Check moved in sys/del
// 006 Check Can't delete
// 007 Unclamp and Delete
// 008 Copy DLL to drive and delete
// 009 Copy DLL, Load, close and RFs::delete
// 010 Copy DLL, Load, close and Loader::delete
// 011 Copy DLL to drive, load and delete
// 012 Check file deleted on close
// 013 Try deleting something of the same name twice while loaded.
// 

//! @SYMTestCaseID			KBASE-T_REAPER-0330
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1110
//! @SYMTestCaseDesc		Demand Paging Reaper.
//! @SYMTestActions			001 Loader Reaper/Clamp Tests
//! @SYMTestExpectedResults All tests should pass.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32svr.h>
#include <f32file.h>
#include <e32ldr.h>
#include <u32hal.h>
#include "u32std.h"

TBool Verbose = EFalse;
RFs TheFs;
RLoader Loader;
TInt DriveNumber=-1;

class TPagingDriveInfo
	{
public:
	TChar iDriveLetter;
	TDriveInfo iDriveInfo;
	};

RArray<TPagingDriveInfo> SupportedDrives;
TInt gNumSupportedDrives = 0;


LOCAL_D RTest test(_L("T_reaper"));

_LIT(KFilePath,":\\sys\\bin\\test.txt");

_LIT(KSysPath,"z:\\sys\\bin\\");
_LIT(KPathDel,":\\sys\\del\\");
_LIT(KDllFile,"t_reaper_test_dll.dll");
const TInt KLibNameLength = 50;

void CopyDll(const TDesC& aSourceName, const TDesC& aDestName)
	{
	const TInt KBufferSize = 3333;
	TBuf8<KBufferSize> buffer;
	RFile in, out;
	
	test.Printf(_L("  copying %S to %S\n"), &aSourceName, &aDestName);

	TInt r = TheFs.MkDirAll(aDestName);
	test_Assert(r == KErrNone || r == KErrAlreadyExists, test.Printf(_L("MkDirAll returned %d\n"),r));

	test_KErrNone(in.Open(TheFs, aSourceName, EFileRead));
	test_KErrNone(out.Replace(TheFs, aDestName, EFileWrite));

	TInt size;
	test_KErrNone(in.Size(size));
	TInt pos = 0;
	while (pos < size)
		{
		test_KErrNone(in.Read(buffer));
		test_KErrNone(out.Write(buffer));
		pos += buffer.Length();
		}
	
	in.Close();
	out.Close();
	}

void CopyDllToDrive(const TDesC& aSourceName, TUint8 aDrive, TBuf<KLibNameLength>& aDestName)
	{
		TBuf<KLibNameLength> sourceName;
		sourceName=KSysPath;
		sourceName.Append(aSourceName);
		aDestName=KSysPath;
		aDestName.Append(aSourceName);
		aDestName[0] = aDrive;
		CopyDll(sourceName, aDestName);
	}

static void CreateTestFile(const TDesC& aTestFile)
/**
	Create an empty file with the supplied name.  This function is used
	to create file which can be deleted with RLoader::Delete.
	
	@param	aFs				Open file server session.
	@param	aTestFile		The test file's name.
 */
	{
	TInt r;
	TheFs.MkDirAll(aTestFile);
	RFile f;
	r = f.Replace(TheFs, aTestFile, EFileWrite | EFileStream | EFileShareExclusive);
	test_KErrNone(r);
	TBuf<256> buf(_L("Space is big, I mean its really big.\n"));
	TPtrC8 pBuf((TUint8*)&buf);
	f.Write(pBuf);
	f.Flush();
	f.Close();
	}

// Get the list of pageable drives
void GetSupportedDrives()
	{
	TChar ch;
	TBuf<256> fileSystemName;
	TDriveList driveList;
	TDriveInfo driveInfo;

	TInt r = TheFs.DriveList(driveList);
    test_KErrNone(r);

	TBool NandPageableMediaFound = EFalse;

	for (TInt drvNum=0; drvNum<KMaxDrives; ++drvNum)
		{
	    if(!driveList[drvNum])
	        continue;   //-- skip unexisting drive
	
	    test_KErrNone( TheFs.Drive(driveInfo, drvNum) );
		test_KErrNone( TheFs.DriveToChar(drvNum, ch) );
		test_KErrNone( TheFs.FileSystemName(fileSystemName, drvNum) );

		if (Verbose)
			test.Printf(_L("GetSupportedDrives, Drive %c iType %d iDriveAtt %08x iMediaAtt %08X, fileSystemName %S\n"), 
				(TInt) ch, driveInfo.iType, driveInfo.iDriveAtt, driveInfo.iMediaAtt, &fileSystemName);
	
		if ((driveInfo.iDriveAtt & KDriveAttPageable) && (driveInfo.iType == EMediaNANDFlash))
			NandPageableMediaFound = ETrue;

		TBool pageable = EFalse;
		if (driveInfo.iDriveAtt & KDriveAttPageable)
			pageable = ETrue;

		// If we've already found a pageable NAND drive, then assume the Z: drive is pageable too 
		// if it's got a composite file system (fudge)
		_LIT(KCompositeName,"Composite");
		if (fileSystemName == KCompositeName())
			{
			if (NandPageableMediaFound)
				pageable = ETrue;
			driveInfo.iMediaAtt|=KMediaAttWriteProtected;
			}
		if (pageable)
			{
			TPagingDriveInfo pagingDriveInfo;
			pagingDriveInfo.iDriveLetter = ch;
			pagingDriveInfo.iDriveInfo = driveInfo;


			test_KErrNone( SupportedDrives.Append(pagingDriveInfo) );
			if (Verbose)
				test.Printf(_L("Drive %c supports paging\n"), (TInt) pagingDriveInfo.iDriveLetter);
			gNumSupportedDrives++;
			}
		}
	}



TInt FindDeletedFile(TUint8 aDrive, TDes& aFile, const TDesC& aExcludedFile=KNullDesC)
	{
	aFile.Zero();

	_LIT(KSearchPathDel,"?:\\sys\\del\\*");
	TBuf<13> delPath;
	delPath = KSearchPathDel;
	delPath[0] = aDrive;

	CDir* dir=NULL;
	
	CTrapCleanup* c = CTrapCleanup::New();
	TInt r = TheFs.GetDir(delPath,KEntryAttMatchExclude | KEntryAttDir ,ESortNone,dir);
	delete c;
	if(r==KErrPathNotFound)
		return 0; // no files
	test_KErrNone(r);

	test.Printf(_L("Files: %d\n"),dir->Count() );
	TInt count = dir->Count();
	TBool found = false;
	for (TInt i=count-1;i>=0;i--)
		{
		TBuf<KLibNameLength> tempPath;
		tempPath.Append(aDrive);
		tempPath.Append(KPathDel);
		tempPath.Append((*dir)[i].iName);
		test.Printf(_L("%S\n"), &tempPath);
		if(!found && tempPath.CompareF(aExcludedFile)!=0)
			{
			found = true;
			aFile = tempPath;
			}
		}
	
	delete dir;
	return count;
	}


void TestDlls(TUint8 aDrive)
	{
	TEntry e;
	TBuf<KLibNameLength> libName;
	TBuf<KLibNameLength> delName;
	TBuf<KLibNameLength> delName2;
	TBuf<KLibNameLength> libName2;

	RLibrary library;
	RLibrary library2;

	test.Next(_L("Copy DLL to drive and delete"));
	CopyDllToDrive(KDllFile,aDrive,libName);
	test_Equal(0, FindDeletedFile(aDrive,delName)); // check no files in sys/del
	test_KErrNone(Loader.Delete(libName));
	test_Equal(KErrNotFound, TheFs.Entry(libName,e));
	test_Equal(0, FindDeletedFile(aDrive,delName)); // check no files in sys/del
	
	test.Next(_L("Copy DLL, Load, close and RFs::delete"));
	CopyDllToDrive(KDllFile,aDrive,libName);
	test_KErrNone( library.Load(libName));
	test_Equal(KErrInUse, TheFs.Delete(libName));
	CLOSE_AND_WAIT(library);
	test_KErrNone( TheFs.Delete(libName));
	test_Equal(KErrNotFound, TheFs.Entry(libName,e));
	
	test.Next(_L("Copy DLL, Load, close and Loader::delete"));
	CopyDllToDrive(KDllFile,aDrive,libName);
	test_KErrNone( library.Load(libName));
	CLOSE_AND_WAIT(library);
	test_KErrNone( Loader.Delete(libName));
	test_Equal(KErrNotFound, TheFs.Entry(libName,e));
	test_Equal(0, FindDeletedFile(aDrive,delName)); // check no files in sys/del

	test.Next(_L("Copy DLL to drive, load and delete"));
	CopyDllToDrive(KDllFile,aDrive,libName);
	test_KErrNone( library.Load(libName));
	test_Equal(KErrInUse, TheFs.Delete(libName));
	test_KErrNone( Loader.Delete(libName));
	test_Equal(KErrNotFound, TheFs.Entry(libName,e));
	test_Equal(1, FindDeletedFile(aDrive,delName)); // get name of 'deleted' file
	test_Equal(KErrInUse, TheFs.Delete(delName));
	
	test.Next(_L("Check file deleted on close"));
	CLOSE_AND_WAIT(library);
	test_Equal(KErrNotFound, TheFs.Entry(delName,e));	

	test.Next(_L("Try deleting something of the same name twice while loaded."));
	CopyDllToDrive(KDllFile,aDrive,libName);
	test_KErrNone( library.Load(libName));
	test_KErrNone( Loader.Delete(libName));
	test_Equal(KErrNotFound, TheFs.Entry(libName,e));
	test_Equal(1, FindDeletedFile(aDrive,delName)); // get name of 'deleted' file
	
	test.Printf(_L("Load a Secord Copy\n"));
	CopyDllToDrive(KDllFile,aDrive,libName);
	libName2=libName;
	libName2[27]='2';
	test_KErrNone( TheFs.Rename(libName,libName2));
	test_KErrNone( library2.Load(libName2));
	test_KErrNone( TheFs.Rename(libName2,libName));
	
	test.Printf(_L("Try and delete second copy\n"));
	test_KErrNone( Loader.Delete(libName));
	test_Equal(2, FindDeletedFile(aDrive,delName2,delName)); // get name of second 'deleted' file

	test.Printf(_L("Now close and watch deletions\n"));
	CLOSE_AND_WAIT(library);
	test_Equal(KErrNotFound, TheFs.Entry(delName,e));
		test_KErrNone( TheFs.Entry(delName2,e));
	CLOSE_AND_WAIT(library2);
	test_Equal(KErrNotFound, TheFs.Entry(delName2,e));
	}


void BasicTest(TUint8 aDrive)
	{
	TBuf<256> startFileName;	
	TBuf<256> endFileName;
	RFile file;
	RFileClamp fileClamp;

	startFileName.Append((TChar) aDrive);
	startFileName+=KFilePath;

	
	CreateTestFile(startFileName);
	
	test.Next(_L("Try deleteing file while Open"));
	test_KErrNone( file.Open(TheFs,startFileName,EFileShareExclusive|EFileWrite));
	test_Equal(KErrInUse, Loader.Delete(startFileName));

	test.Next(_L("Try deleteing file while Clamped and Open"));
	test_KErrNone( fileClamp.Clamp(file));
	test_Equal(KErrInUse, Loader.Delete(startFileName));

	test.Next(_L("Try deleteing while Clamped"));
	file.Close(); 
	test_Equal(0, FindDeletedFile(aDrive,endFileName)); // check no files in sys/del
	test_KErrNone( Loader.Delete(startFileName));

	test.Next(_L("Check moved in sys/del"));
	test_Equal(1, FindDeletedFile(aDrive,endFileName)); // check one file in sys/del
	
	test.Next(_L("Check Can't delete"));
	test_Equal(KErrInUse, TheFs.Delete(endFileName));

	test.Next(_L("Unclamp and Delete"));
	test_KErrNone( fileClamp.Close(TheFs));
	test_KErrNone( TheFs.Delete(endFileName));
	
	
	}


//
// ParseCommandLine reads the arguments and sets globals accordingly.
//

TInt ParseCommandLine()
	{
	TBuf<32> args;
	User::CommandLine(args);
	TLex lex(args);
	TInt err=KErrNone;
	FOREVER
		{
		TPtrC token=lex.NextToken();
		if(token.Length()!=0)
			{
			if ((token.Length()==1) || ((token.Length()==2) && (token[1]==':')))
				{
				TChar driveLetter = User::UpperCase(token[0]); 
				if ((driveLetter>='A') && (driveLetter<='Z'))
					DriveNumber=driveLetter - (TChar) 'A';
				else 
					err=KErrArgument;
				}
			else if ((token==_L("help")) || (token==_L("-h")) || (token==_L("-?")))
				{
				test.Printf(_L("\nThis tests the loader's reaper, which is used in code paging to postpone deletions of paged code from disk.\n")); 
				test.Printf(_L("\nIf no drive letter is supplied, then all suitable drives are checked.\n\n"));
				err=KErrCancel;
				}
			else
				err=KErrArgument;
			}
		else
			break;
		
		if (err!=KErrNone)
			{
			if (err==KErrArgument)
				test.Printf(_L("\nUnknown argument '%S'\n"), &token);
			test.Printf(_L("\nUsage:  t_reaper [-h] [<driveletter>]\n\n"));
			test.Getch();
			return err;
			}
		}
	return KErrNone;
	}



GLDEF_C TInt E32Main()
	{
	TInt i;
	test.Title();
	
	if (ParseCommandLine())
		return KErrNone;
	
	TheFs.Connect();
	
	TUint32 memModelAttributes=UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, NULL, NULL);
	TUint32 pagingPolicy = E32Loader::PagingPolicy();
	if((memModelAttributes&EMemModelAttrCodePaging)==0 || pagingPolicy==EKernelConfigCodePagingPolicyNoPaging)
		{
		test.Start(_L("TESTS NOT RUN - Code paging not enabled on system."));
		test.End();
		return KErrNone;
		}

	test.Start(_L("Loader Reaper/Clamp Tests"));
	
	// Turn off evil lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	GetSupportedDrives();
	test_Compare(gNumSupportedDrives,>,0);
	test_KErrNone( Loader.Connect());
	test_KErrNone( Loader.CancelLazyDllUnload());
	
	for (i=0;i<gNumSupportedDrives;i++)
		{
		if ((DriveNumber==-1) || (DriveNumber== (TInt) (SupportedDrives[i].iDriveLetter - (TChar) 'A')))
			{
			test.Printf(_L("Testing drive: %c\n"),(TInt) SupportedDrives[i].iDriveLetter);
			if (!(SupportedDrives[i].iDriveInfo.iMediaAtt&KMediaAttWriteProtected))
				{
				TUint8 drive = SupportedDrives[i].iDriveLetter;
				BasicTest(drive);
				TestDlls(drive);
				}
			}
				
		}
		
	Loader.Close();
	TheFs.Close();
	
	test.End();
	
	return KErrNone;
	}
