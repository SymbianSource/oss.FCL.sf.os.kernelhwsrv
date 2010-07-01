// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// N.B. Before running this test on WINS, ensure that the estart.txt file contains 
// nothing but  EFAT32 i.e. no EFAT - otherwise the FAT16 file system will be used
// On target ensure that the FAT32 filesystem is in the ROM instead of the FAT16 file system
// This test expects the following files to be present before running the test:
// size			name
// 2147483647	\F32-TST\File2GBMinusOne.txt
// 2147483648	\F32-TST\File2GB.txt
// 3221225472	\F32-TST\File3GB.txt
// 4294967295	\F32-TST\File4GBMinusOne.txt	// may be absent on an 8GB disk
// For verification purposes, Every 4 bytes of each file contains the current position, e.g.
// 0000: 00 00 00 00 
// 0004: 04 00 00 00
// 0008: 08 00 00 00
// .. etc
// These files can be created using the BigFileWriter tool in f32test/tool
// If this test is run on the emulator and the __MOUNT_RAW_EXT__ macro is defined (see below) then
// the T_RAWEXT file system extension will be loaded; this extension allows reading and writing to 
// a windows disk in "raw" format, thus allowing direct access to a windows disk. see f32test/ext/t_rawext
// for more details.
// 
//


#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32svr.h>
#include "t_server.h"


GLDEF_D RTest test(_L("T_BIGFILE"));

#ifdef __WINS__
// enable this macro to mount the RAWEXT.FXT file system extension to test on a particular windows drive
#define __MOUNT_RAW_EXT__
#endif

#ifdef __MOUNT_RAW_EXT__
_LIT(KExtName,"RAWEXT");

_LIT(KFAT32FName,"EFAT32");
_LIT(KFATName,"FAT");

TFullName gOldFsName;
#endif

TInt gDrive;
TBool gNTFS=EFalse;

const TUint K1Kb = 1 << 10;
//const TUint K1Mb = 1 << 20;
const TUint K1Gb = 1 << 30;
const TUint K2Gb = 0x80000000;
const TUint K2GbMinusOne = 0x7FFFFFFF;
const TUint K3Gb = 0xC0000000;
const TUint K4GbMinusOne = 0xFFFFFFFF;
const TUint KPosMask     = 0xFFFFFFFC;

//const TUint KBigFileSizeSigned = KMaxTInt32;		// 2Gb -1
//const TUint KBigFileSizeUnsigned = KMaxTUint32;	// 4Gb -1

const TInt KBufSize = (256 * K1Kb);
HBufC8* gBuf = NULL;
TPtr8 gBufPtr(NULL, 0, 0);


_LIT(KFile2GBMinusOne, "File2GBMinusOne.txt");
_LIT(KFile2GB, "File2GB.txt");
_LIT(KFile3GB, "File3GB.txt");
_LIT(KFile4GBMinusOne, "File4GBMinusOne.txt");
TInt gFilesInDirectory = 4;


// return ETrue if the specifiled file is present
TBool FilePresent(const TDesC& aFileName)
	{
	TEntry entry;
	TInt r = TheFs.Entry(aFileName, entry);
	return (r == KErrNone ? (TBool)ETrue : (TBool)EFalse);
	}

class CFileManObserver : public CBase, public MFileManObserver
	{
public:
	CFileManObserver(CFileMan* aFileMan);

	TControl NotifyFileManStarted();
	TControl NotifyFileManOperation();
	TControl NotifyFileManEnded();
private:
	CFileMan* iFileMan;
public:
	TInt iNotifyEndedSuccesses;
	TInt iNotifyEndedFailures;
	};

CFileManObserver::CFileManObserver(CFileMan* aFileMan)
	{
	__DECLARE_NAME(_S("CFileManObserver"));
	iFileMan=aFileMan;
	}

MFileManObserver::TControl CFileManObserver::NotifyFileManStarted()
	{
    (void)MFileManObserver::NotifyFileManStarted();
	TInt lastError = iFileMan->GetLastError();
	TFileName fileName = iFileMan->CurrentEntry().iName;
	test.Printf(_L("NotifyFileManStarted(): Error %d File %S\n"),lastError, &fileName);
	return(MFileManObserver::EContinue);
	}

MFileManObserver::TControl CFileManObserver::NotifyFileManOperation()
	{
    (void)MFileManObserver::NotifyFileManOperation();
	TInt lastError = iFileMan->GetLastError();
	TFileName fileName = iFileMan->CurrentEntry().iName;
	test.Printf(_L("NotifyFileManOperation(): Error %d File %S\n"),lastError, &fileName);
	return(MFileManObserver::EContinue);
	}

MFileManObserver::TControl CFileManObserver::NotifyFileManEnded()
	{
	TInt lastError = iFileMan->GetLastError();
	TFileName fileName = iFileMan->CurrentEntry().iName;
	test.Printf(_L("NotifyFileManEnded(): Error %d File %S\n"),lastError, &fileName);
	if (lastError == KErrNone)
		iNotifyEndedSuccesses++;
	else
		iNotifyEndedFailures++;
	return(MFileManObserver::EContinue);
	}



//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_BIGFILE-0001
//! @SYMTestType        CIT
//! @SYMTestCaseDesc    Test that 2GB-1 file can be opened and read
//! @SYMTestActions     Open the file, seek to end-1K and read some data. Verify the results
//! @SYMTestExpectedResults Should succeed
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void OpenAndRead2GBMinusOne()
	{
	RFile f;
	TEntry entry;
	TUint testSize;
	TUint size;
	TUint testPos;
	TInt r;

	TPtr8 bufPtr = gBuf->Des();
	bufPtr.SetLength(bufPtr.MaxLength());

	const TFileName fname = KFile2GBMinusOne();

	test.Next(_L("2GBMinusOne File: Open"));

	r = f.Open(TheFs, fname, EFileRead);
	test_KErrNone(r);

	testSize = K2GbMinusOne;
	
	test.Next(_L("2GBMinusOne File: Read"));

	r=f.Size((TInt&) size);
	test_KErrNone(r);
	test(size == testSize);
	
	r = TheFs.Entry(fname, entry);
	test_KErrNone(r);
	test ((TUint) entry.iSize == testSize);

	// seek to just below 2GB
	testPos = (K2GbMinusOne - K1Kb) & KPosMask;
	r = f.Seek(ESeekStart, (TInt&) testPos);
	test_KErrNone(r);

	r = f.Read(bufPtr);
	test_KErrNone(r);

	TUint posRead =  * ((TUint*) &bufPtr[0]);
	test.Printf(_L("position read %08X, expected %08X\n"), posRead, testPos);
	test(posRead == testPos);

	f.Close();
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_BIGFILE-0002
//! @SYMTestType        CIT
//! @SYMTestCaseDesc    Test that attempting to open a 2GB file fails
//! @SYMTestActions     Open the file
//! @SYMTestExpectedResults KErrToBig
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void Open2GB()
	{
	RFile f;
	TEntry entry;
	TUint testSize;
	TInt r;

	TPtr8 bufPtr = gBuf->Des();
	bufPtr.SetLength(bufPtr.MaxLength());

	const TFileName fname = KFile2GB();
	testSize = K2Gb;

	test.Next(_L("2GB File: Test the size with RFs::Entry"));
	r = TheFs.Entry(fname, entry);
	test_KErrNone(r);
	test ((TUint) entry.iSize == testSize);

	test.Next(_L("2GB File: Attempt to open (should fail with KErrToBig)"));

	r = f.Open(TheFs, fname, EFileRead);
	test_Value(r, r == KErrTooBig);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_BIGFILE-0003
//! @SYMTestType        CIT
//! @SYMTestCaseDesc    Test that attempting to open a 2GB file fails
//! @SYMTestActions     Open the file
//! @SYMTestExpectedResults KErrToBig
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void Open3GB()
	{
	RFile f;
	TEntry entry;
	TUint testSize;
	TInt r;

	TPtr8 bufPtr = gBuf->Des();
	bufPtr.SetLength(bufPtr.MaxLength());

	const TFileName fname = KFile3GB();
	testSize = K3Gb;

	test.Next(_L("3GB File: Test the size with RFs::Entry"));
	r = TheFs.Entry(fname, entry);
	test_KErrNone(r);
	test ((TUint) entry.iSize == testSize);

	test.Next(_L("3GB File: Attempt to open (should fail with KErrToBig)"));

	r = f.Open(TheFs, fname, EFileRead);
	test_Value(r, r == KErrTooBig);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_BIGFILE-0004
//! @SYMTestType        CIT
//! @SYMTestCaseDesc    Test that attempting to open a 4GB file fails
//! @SYMTestActions     Open the file
//! @SYMTestExpectedResults KErrToBig
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void Open4GB()
	{
	RFile f;
	TEntry entry;
	TUint testSize;
	TInt r;

	TPtr8 bufPtr = gBuf->Des();
	bufPtr.SetLength(bufPtr.MaxLength());

	const TFileName fname = KFile4GBMinusOne();
	testSize = K4GbMinusOne;

	test.Next(_L("4GB File: Test the size with RFs::Entry"));
	r = TheFs.Entry(fname, entry);
	
	test_KErrNone(r);
	test ((TUint) entry.iSize == testSize);

	test.Next(_L("4GB File: Attempt to open (should fail with KErrToBig)"));

	r = f.Open(TheFs, fname, EFileRead);
	test_Value(r, r == KErrTooBig);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_BIGFILE-0005
//! @SYMTestType        CIT
//! @SYMTestCaseDesc    Attempt to append to the end of a 2GB-1 file
//! @SYMTestActions     Open the file, seek to end and write one byte
//! @SYMTestExpectedResults RFile::Write(0 returns KErrToBig
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void Extend2GBMinusOne()
	{
	RFile f;
	TEntry entry;
	TUint testSize;
	TUint size;
	TUint testPos;
	TInt r;

	TPtr8 bufPtr = gBuf->Des();
	bufPtr.SetLength(bufPtr.MaxLength());

	const TFileName fname = KFile2GBMinusOne();
	testSize = K2GbMinusOne;

	test.Next(_L("2GBMinusOne File: Open"));

	r = f.Open(TheFs, fname, EFileRead | EFileWrite);
	test_KErrNone(r);

	
	test.Next(_L("2GBMinusOne File: Attempt to extend"));

	r=f.Size((TInt&) size);
	test_KErrNone(r);
	test(size == testSize);
	
	r = TheFs.Entry(fname, entry);
	test_KErrNone(r);
	test ((TUint) entry.iSize == testSize);

	// seek to end
	testPos = 0;
	r = f.Seek(ESeekEnd, (TInt&) testPos);
	test_KErrNone(r);

	bufPtr.SetLength(1);
	r = f.Write(bufPtr);
	test_Value(r, r == KErrTooBig);

	f.Close();
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_BIGFILE-0006
//! @SYMTestType        CIT
//! @SYMTestCaseDesc    Check that deleting a large file frees cluster properly
//! @SYMTestActions     Delete the passed file name, call RFs::CheckDisk
//!						On windows, we could run chkdsk utility 
//! @SYMTestExpectedResults RFs::CheckDisk returns success
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void DeleteLargeFile(const TDesC& aFileName)
	{
	test.Next(_L("Delete large file"));
	test.Printf(_L("Deleting %S\n"), &aFileName);

	TInt r = TheFs.Delete(aFileName);
	test_KErrNone(r);

	CheckDisk();
	}


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_BIGFILE-0007
//! @SYMTestType        CIT
//! @SYMTestCaseDesc    Check that we can get a valid directory listing of a directory 
//!						containing large files using RDir and then CDir
//! @SYMTestActions     Open the directory using RDir and examine the results
//!						On windows, we could run chkdsk utility 
//! @SYMTestExpectedResults The expected number of files should exist with the correct sizes
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void ReadDirectory()
	{
	test.Next(_L("Read a directory containing large files using RDir"));

	RDir dir;
	TInt r = dir.Open(TheFs, _L("*.*"), KEntryAttNormal);
	test_KErrNone(r);
	
	TEntryArray entryArray;
	r = dir.Read(entryArray);
	test_Value(r, r == KErrEof);

	test(entryArray.Count() == gFilesInDirectory);

	TInt n;
	for (n=0; n<entryArray.Count(); n++)
		{
		const TEntry& entry = entryArray[n];
		if (entry.iName.MatchF(KFile2GBMinusOne()) == 0)
			{
			test((TUint) entry.iSize == K2GbMinusOne);
			}
		else if (entry.iName.MatchF(KFile2GB()) == 0)
			{
			test((TUint) entry.iSize == K2Gb);
			}
		else if (entry.iName.MatchF(KFile3GB()) == 0)
			{
			test((TUint) entry.iSize == K3Gb);
			}
		else if (entry.iName.MatchF(KFile4GBMinusOne()) == 0)
			{
			test((TUint) entry.iSize == K4GbMinusOne);
			}
		else
			test(EFalse);
		}

	dir.Close();

	test.Next(_L("Read a directory containing large files using CDir & sort by size"));
	CDir* dirList;
	r=TheFs.GetDir(_L("*.*"), KEntryAttMaskSupported, ESortBySize, dirList);
	test_KErrNone(r);
	test(dirList->Count() == gFilesInDirectory);
	for (n=0; n<dirList->Count(); n++)
		{
		TEntry entry;
		entry=(*dirList)[n];
		// test.Printf(_L("#%d: %08X %d %S"), n, entry.iSize, entry.iSize, &entry.iName);
		if (entry.iName.MatchF(KFile2GBMinusOne()) == 0)
			{
			test((TUint) entry.iSize == K2GbMinusOne);
			test(n == 0);	// test entry has been sorted correctly (i.e. according to size)
			}
		else if (entry.iName.MatchF(KFile2GB()) == 0)
			{
			test((TUint) entry.iSize == K2Gb);
			test(n == 1);
			}
		else if (entry.iName.MatchF(KFile3GB()) == 0)
			{
			test((TUint) entry.iSize == K3Gb);
			test(n == 2);
			}
		else if (entry.iName.MatchF(KFile4GBMinusOne()) == 0)
			{
			test((TUint) entry.iSize == K4GbMinusOne);
			test(n == 3);
			}
		else
			test(EFalse);
		}

	delete dirList;

	
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_BIGFILE-0008
//! @SYMTestType        CIT
//! @SYMTestCaseDesc    Check that we can a move a directory containing large files 
//!						Using CFileMan::Move()	
//! @SYMTestActions     Use CFileMan::Move() to move files from one directory to another
//! @SYMTestExpectedResults The files should be moved correctly
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void MoveDirectory()
	{
	test.Next(_L("Move a directory containing large files"));

	CFileMan* fileMan = CFileMan::NewL(TheFs);
	test(fileMan != NULL);
	
	TPath filePathOld = gSessionPath;
	filePathOld+= _L("*.*");
	TPath filePathNew =	_L("?:\\TEST\\");
	TChar driveLetter;
	TInt r=TheFs.DriveToChar(gDrive,driveLetter);
	test_KErrNone(r);
	filePathNew[0] = (TText) driveLetter;

	// move to new directory
	r = fileMan->Move(filePathOld, filePathNew, CFileMan::ERecurse | CFileMan::EOverWrite);
	test_KErrNone(r);

	// then move back again
	r = fileMan->Move(filePathNew, filePathOld);
	test_KErrNone(r);

	delete fileMan;
	}


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_BIGFILE-0009
//! @SYMTestType        CIT
//! @SYMTestCaseDesc    Check that we can copy a directory containing large file(s)
//!						Using CFileMan::Copy()	
//! @SYMTestActions     Use CFileMan::Copy() to copy files from one directory to another
//! @SYMTestExpectedResults The files should be copied correctly
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
void CopyDirectory()
	{
	test.Next(_L("Copy a directory containing large files"));
	CFileMan* fileMan = CFileMan::NewL(TheFs);
	test(fileMan != NULL);
	
	CFileManObserver* observer = new CFileManObserver(fileMan);
	test(observer != NULL);

	TPath filePathOld = gSessionPath;
	filePathOld+= _L("*.*");
	TPath filePathNew =	_L("?:\\TEST\\");
	TChar driveLetter;
	TInt r = TheFs.DriveToChar(gDrive,driveLetter);
	test_KErrNone(r);
	filePathNew[0] = (TText) driveLetter;

	// create some small files in the source directory 
	// so that there is a combination of small files and one large files
	RFile file;
	_LIT(KFileSmall1, "FileSmallOne.txt");
	_LIT(KFileSmall2, "FileSmallTwo.txt");
	_LIT(KFileSmall3, "FileSmallThree.txt");
	r = file.Create(TheFs, KFileSmall1(), EFileWrite | EFileShareAny);
	test_KErrNone(r);
	r = file.Write(_L8("1"));
	test_KErrNone(r);
	file.Close();

	r = file.Create(TheFs, KFileSmall2(), EFileWrite | EFileShareAny);
	test_KErrNone(r);
	r = file.Write(_L8("12"));
	test_KErrNone(r);
	file.Close();

	r = file.Create(TheFs, KFileSmall3(), EFileWrite | EFileShareAny);
	test_KErrNone(r);
	r = file.Write(_L8("123"));
	test_KErrNone(r);
	file.Close();

	// copy to new directory
	r = fileMan->Copy(filePathOld, filePathNew, CFileMan::ERecurse | CFileMan::EOverWrite);
	test_Value(r, r == KErrNone || r == KErrTooBig);


	// check SMALL files have been copied
	RDir dir;
	r = dir.Open(TheFs, filePathNew, KEntryAttNormal);
	test_KErrNone(r);
	TEntryArray entryArray;
	r = dir.Read(entryArray);
	test_Value(r, r == KErrEof);
	test(entryArray.Count() == 3);
	dir.Close();
	
	// then delete the new directory
	r = fileMan->Delete(filePathNew);
	test_KErrNone(r);

	
	// attempt to copy to new directory again - this time with an observer
	fileMan->SetObserver(observer);
	r = fileMan->Copy(filePathOld, filePathNew, CFileMan::ERecurse | CFileMan::EOverWrite);
	test_Value(r, r == KErrNone || r == KErrTooBig);
	
	// test that 3 small files were copied and 1 or 2 large files failed to copy
	// (For 8 GB disk, the 4GB file is missing)
	test(observer->iNotifyEndedSuccesses == 3);
	test(observer->iNotifyEndedFailures == 1 || observer->iNotifyEndedFailures == 2);

	// check SMALL files have been copied
	r = dir.Open(TheFs, filePathNew, KEntryAttNormal);
	test_KErrNone(r);
	r = dir.Read(entryArray);
	test_Value(r, r == KErrEof);
	test(entryArray.Count() == 3);
	dir.Close();
	
	// then delete the new directory
	r = fileMan->Delete(filePathNew);
	test_KErrNone(r);

	delete observer;
	delete fileMan;
	}


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_BIGFILE-000A
//! @SYMTestType        CIT
//! @SYMTestCaseDesc    Check that CDirScan works correctly with a directory containing large file(s)
//! @SYMTestActions     Use CFileMan::Copy() to copy files from one directory to another
//! @SYMTestExpectedResults The files should be copied correctly
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
TInt ScanDir(const TDesC& aName, CDirScan::TScanDirection aDirection, TInt aError)
	{
	TInt r;
	TFileName dirName;

	CDirScan* scanner = NULL;
	TRAP(r, scanner = CDirScan::NewL(TheFs));
	test_Value(r, r == KErrNone && scanner);

	TRAP(r, scanner->SetScanDataL(aName,KEntryAttDir,ESortByName|EAscending,aDirection));
	test_KErrNone(r);
	
	CDir *entryList=NULL;
	TInt filesFound = 0;
	for (;;)
		{
		TRAP(r, scanner->NextL(entryList));
		test_Value(r, r == aError);
		if (entryList==NULL)
			break;
		TInt count = entryList->Count();
		while (count--)
			{
			TEntry data=(*entryList)[count];
			TBuf<KMaxFileName> path=scanner->AbbreviatedPath();
			dirName = path;
			dirName.Append(data.iName);
			test.Printf(_L("    %S\n"),&dirName);
			filesFound++;
			}

		delete entryList;
		entryList=NULL;
		}
	delete scanner;

	return filesFound;
	}



GLDEF_C void CallTestsL()
//
// Do tests relative to the session path
//
	{
	
#if defined(__WINS__)
	if (gSessionPath[0]=='C')
		gNTFS=ETrue;
	else
		gNTFS=EFalse;
#endif

	// don't test on NTFS
	if (gNTFS)
		{
		test.Printf(_L("Skipping test: Drive is NTFS\n"));
		return;
		}

	TInt r;

	r = TheFs.CharToDrive(gDriveToTest, gDrive);
	test_KErrNone(r);

#ifdef __MOUNT_RAW_EXT__
	r=TheFs.FileSystemName(gOldFsName, gDrive);
	test_KErrNone(r);

	if (gOldFsName.CompareF(KFATName) != 0)
		{
		test.Printf(_L("Skipping test: Not a FAT drive\n"));
		return;
		}

    r = TheFs.AddExtension(KExtName);
    test_Value(r, r == KErrNone || r==KErrAlreadyExists);
    r = TheFs.MountExtension(KExtName, gDrive);
    test_Value(r, r == KErrNone || r==KErrAlreadyExists);
#endif

	TVolumeInfo vi;
	test((r = TheFs.Volume(vi, gDrive)) == KErrNone);
	test.Printf(_L("vi.iSize = %ld\n"), vi.iSize);
	
	// don't test if media sise is less than 7GB
	if (vi.iSize < TInt64(K1Gb) * TInt64(7))
		{
		test.Printf(_L("Skipping test: Drive is not big enough\n"));
		}
	if (!FilePresent(KFile2GB()))
		{
		test.Printf(_L("Skipping test: Test files not present on drive\n"));
		}
	else
		{
		gBuf = HBufC8::NewL(KBufSize);
		if (gBuf == NULL)
			User::Leave(KErrNoMemory);
		gBufPtr = gBuf->Des();


		TInt r;

		// Test that RFs::CheckDisk() succeeds with large files present
		CheckDisk();
		
		test.Next(_L("Scan Drive"));
		r = TheFs.ScanDrive(gSessionPath);
		test_KErrNone(r);

		// NB the 4GB file will not be present unless the disk is > 8GB (because it doesn't fit)
		if (!FilePresent(KFile4GBMinusOne()))
			gFilesInDirectory--;

		// test CDirScan
		// the number of files & directories found should be 5 or 4
		TInt filesFound = ScanDir(_L("\\"), CDirScan::EScanUpTree, KErrNone);
		test (filesFound == gFilesInDirectory+1);
		filesFound = ScanDir(_L("\\"), CDirScan::EScanDownTree, KErrNone);
		test (filesFound == gFilesInDirectory+1);

		OpenAndRead2GBMinusOne();
		Open2GB();
		Open3GB();

		// the 4GB file will not be present unless the disk is > 8GB
		if (FilePresent(KFile4GBMinusOne()))
			Open4GB();

		Extend2GBMinusOne();

		ReadDirectory();

		MoveDirectory();

		
		// delete the 2 smaller files to make some space
		DeleteLargeFile(KFile2GB());
		DeleteLargeFile(KFile2GBMinusOne());
		
		CopyDirectory();
		
		// delete the 3GB file and check the disk
		DeleteLargeFile(KFile3GB());

		if (FilePresent(KFile4GBMinusOne()))
			DeleteLargeFile(KFile4GBMinusOne());

		// Finally check that we can format the drive...
		Format (gDrive);
		}

#ifdef __MOUNT_RAW_EXT__
	r = TheFs.DismountExtension(KExtName, gDrive);
	test_KErrNone(r);

	r = TheFs.RemoveExtension(KExtName);
	test_KErrNone(r);

#endif

	delete gBuf; gBuf = NULL;
	}

