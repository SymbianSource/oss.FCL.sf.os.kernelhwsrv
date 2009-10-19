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
// Implementation of the t_localtime test. This tests functionality introduced
// in CR1084 ie. That removable (in practice this means FAT) file systems
// can be made to use local time for timestamps.
// 
//

/**
 @file
 @test
*/
#define __E32TEST_EXTENSION__
#include <e32test.h>

#include "t_localtime.h"
#include "t_server.h"

RTest test(KTestGroupName);

/**
Constructor for generic test.
@param aTest The RTest handle to use.
@param aDriveLetter The drive to be tested.
@param aBuild Specifies whether tests or being run under UDEB or UREL
*/
CLocalTimeTest::CLocalTimeTest(RTest& aTest, const TDesC& aDriveLetter, TBuild aBuild) 
	:iDriveLetter(aDriveLetter), iTest(aTest), iBuild(aBuild), iOriginalUseLocalTimeFlag(EFalse)
	{
	
	}

/**
Factory function for test.
@param aTest The RTest handle to use.
@param aDriveLetter The drive to be tested.
@param aBuild Specifies whether tests or being run under UDEB or UREL
@return A test object
*/
CLocalTimeTest* CLocalTimeTest::NewLC(RTest& aTest, const TDesC& aDriveLetter, TBuild aBuild)
	{
	CLocalTimeTest* self = new(ELeave) CLocalTimeTest(aTest, aDriveLetter, aBuild);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}

/** 
Connect the test to the File server
*/
void CLocalTimeTest::ConstructL()
	{
	User::LeaveIfError(iRFs.Connect() );
	iRFs.CharToDrive(iDriveLetter[0], iDrive);
	
	SetTestTypeL();
	if(iTestType !=ENoTest)
		{
		MakeTestPathL();
		
		iTestFile = new (ELeave) CTestFile(KFile, &iRFs);
		iTestFile->SetPath(iTestPath);
		iTestFileRFs = new (ELeave) CTestFileRFs(KFile, &iRFs );
		iTestFileRFs->SetPath(iTestPath);
		iTestDirectory = new (ELeave) CTestDirectory(KDirectory, &iRFs);
		iTestDirectory->SetPath(iTestPath);
		
		if(iBuild==EUdeb)
			iOriginalUseLocalTimeFlag=IsLocalTimeOnRemMediaL(); //store initial setting
		}

	if(iTestType==EPositive)
		iExpectedTimeStampOffset=KTimeOffset;
	else
		iExpectedTimeStampOffset=KNullTimeOffset;
	}




CLocalTimeTest::~CLocalTimeTest()
	{
	if(iTestType!=ENoTest && iBuild==EUdeb) //restore flag to original value
		{
		TRAPD(err,
			{
			if(iOriginalUseLocalTimeFlag)
				LocalTimeForRemMediaOnL();
			else
				LocalTimeForRemMediaOffL();
			}
		);
		__ASSERT_ALWAYS(err==KErrNone, User::PanicUnexpectedLeave());
		}

	if(iTestFile)
		delete iTestFile;
	if(iTestFileRFs)
		delete iTestFileRFs;
	if(iTestDirectory)
		delete iTestDirectory;
	}

/**
Check that the test can read and modify the flag within
the FAT or FAT32 plugin.
*/
void CLocalTimeTest::TestDebugInterfaceL()
	{
	iTest.Start(_L("Checking debug interface"));
		
	TBool localTimeEnabled(EFalse);
	localTimeEnabled = IsLocalTimeOnRemMediaL();
	iTest.Printf(_L("Use localtime enable intially? %d\n"), localTimeEnabled);
	
	LocalTimeForRemMediaOffL();
	localTimeEnabled=IsLocalTimeOnRemMediaL();
	iTest.Next(_L("Disabling flag..."));
	iTest(!localTimeEnabled);
	
	LocalTimeForRemMediaOnL();
	localTimeEnabled=IsLocalTimeOnRemMediaL();
	iTest.Next(_L("Enabling flag..."));
	iTest(localTimeEnabled);
	
	iTest.End();
	}

/**
Test that after creating a file or directory its creation time
has been offset (or not) as expected.
*/
void CLocalTimeTest::TestReadCreationTimeL(CFileSystemEntry* aFsEntry)
	{
	iTest.Next(_L("Read creation time"));
	iTest.Printf(_L("Testing on %S"), &aFsEntry->Name());
#if defined(_DEBUG)	
	LocalTimeForRemMediaOnL();
	TTime now;
	test_KErrNone(aFsEntry->DeleteCreate());
	now.UniversalTime(); //hopefuly "now" will be within the 2-second error allowed.
				
	aFsEntry->Close();
	TTime creationTimeLocal = aFsEntry->CreationTimeL();
	
	PrintTimeL(_L("Current UTC time"), now);
	PrintExpectedOffset();
	
	PrintTimeL(_L("creation time"), creationTimeLocal);
	iTest(CLocalTimeTest::FuzzyTimeMatch(creationTimeLocal, now+iExpectedTimeStampOffset));

#else
	test.Printf(_L("Creation times cannot be accessed in release build\n"));

#endif
	}

/**
Test that when reading a modification time it has been translated (or not)
as expected.
@param aFsEntry A file or directory to be tested
*/ 
void CLocalTimeTest::TestReadModificationTimeL(CFileSystemEntry* aFsEntry)
	{
	TInt r= KErrNone;
	iTest.Next(_L("Reading modification time - offset should be subtracted from timestamp "));
	iTest.Printf(_L("Testing on %S \n"), &aFsEntry->Name());
	LocalTimeForRemMediaOffL();
	r = aFsEntry->DeleteCreate();
	test_KErrNone(r);
	TTime now;
	now.UniversalTime();
	PrintTimeL(_L("Current UTC time"), now);
	//timestamp on disk will be UTC
	r = aFsEntry->SetModificationTime(now);
	test_KErrNone(r);
	aFsEntry->Close();
	LocalTimeForRemMediaOnL();
	r = aFsEntry->Open();
	test_KErrNone(r);
	
	TTime modTime = aFsEntry->ModificationTimeL();
	test_KErrNone(r);
	
	PrintExpectedOffset();
	PrintTimeL(_L("modification time"), modTime);
	iTest(CLocalTimeTest::FuzzyTimeMatch(modTime, now - iExpectedTimeStampOffset));
	aFsEntry->Close();
	}

/**
Test that when setting a modification time it is modified as expected.
@param aFsEntry A file or directory to be tested
*/
void CLocalTimeTest::TestSetModificationTimeL(CFileSystemEntry* aFsEntry)
	{
	iTest.Next(_L("Setting modification time - offset should be added to timestamp"));
	iTest.Printf(_L("Testing on %S \n"), &aFsEntry->Name());		
	TTime now;
	now.UniversalTime();
	PrintTimeL(_L("Modification time set"), now);
	LocalTimeForRemMediaOnL();
	TInt r = KErrNone;
	r = aFsEntry->DeleteCreate();
	test_KErrNone(r);

	//timestamp on disk will be local
	r = aFsEntry->SetModificationTime(now);
	test_KErrNone(r);
	aFsEntry->Close();
	
	LocalTimeForRemMediaOffL();
	aFsEntry->Open();
	
	TTime modTime = aFsEntry->ModificationTimeL();
	
	PrintExpectedOffset();
	PrintTimeL(_L("Modification time read"), modTime);
	iTest(CLocalTimeTest::FuzzyTimeMatch(modTime, now+iExpectedTimeStampOffset));
	
	aFsEntry->Close();
	}
/**
Check that modification times of copied files are preserved
@param aFsEntry A file or directory to be tested
*/
void CLocalTimeTest::TestCopyL(CFileSystemEntry* aFsEntry)
	{
	LocalTimeForRemMediaOnL();
		
	iTest.Next(_L("Test copying"));
	iTest.Printf(_L("Testing on %S \n"), &aFsEntry->Name());
	aFsEntry->DeleteCreate();
	TTime mtime = aFsEntry->ModificationTimeL();
	PrintTimeL(_L("Original mtime"), mtime);
	
	
	CFileSystemEntry* file2 = aFsEntry->CopyL();
	file2->Close();
	CleanupStack::PushL(file2);
	
	file2->Open();
	TTime mtime2 = file2->ModificationTimeL();
	
	
	iTest.Printf(_L("Modification times should be preserved\n"));
	
	PrintTimeL(_L("Copy's mtime"), mtime2 );
	iTest(FuzzyTimeMatch(mtime2, mtime) ); //mod time should always be preserved on copy
	
	CleanupStack::PopAndDestroy(file2);
	aFsEntry->Close();
	}

/**
Check that modification times of copied directories are preserved
*/
void CLocalTimeTest::TestCopyDirL()
	{
	LocalTimeForRemMediaOnL();
	
	iTest.Next(_L("Test copying directory - modtimes should be preserved"));
	_LIT(KSubDir, "SubDir\\");
	TPath parentDir(iTestPath);
	parentDir+=KDirectory;
	parentDir.Delete(parentDir.Length()-1, 1);
	
	TPath subDir(parentDir);
	subDir.Append(KPathDelimiter);
	subDir+=KSubDir;
	
	
	TPath destDir(iTestPath);
	destDir+=_L("copyDir");
	destDir.Append(KPathDelimiter);
		
	iRFs.RmDir(subDir);
	iRFs.MkDirAll(subDir);
	
	TPath destSubDir(destDir);
	destSubDir+=KSubDir;
	iRFs.RmDir(destSubDir);
	
	CFileMan* fMan = CFileMan::NewL(iRFs);
	CleanupStack::PushL(fMan );
	
	
	
	fMan->Copy(parentDir, destDir , CFileMan::EOverWrite|CFileMan::ERecurse);
	
	TTime originalModtime;
	TTime newDirModtime;
	
	test_KErrNone(iRFs.Modified(subDir, originalModtime) );
	test_KErrNone(iRFs.Modified(destSubDir, newDirModtime));
	PrintTimeL(_L("Orginal modtime"), originalModtime);
	PrintTimeL(_L("Copy's modtime"), newDirModtime);
	
	iRFs.RmDir(subDir);
	iRFs.RmDir(destSubDir);
	

	iTest(FuzzyTimeMatch(originalModtime,newDirModtime) );
	
	CleanupStack::PopAndDestroy(fMan);
	}

/**
Checks whether two times match, to a certain tolerance. By default allow for a 2 second error.
@param aTestTime One time
@param aRefTime Second time.
@return Whether the times matched.
*/
TBool CLocalTimeTest::FuzzyTimeMatch(const TTime& aTestTime, const TTime& aRefTime)
	{
	return(aTestTime>=(aRefTime-KModTimeThreshold) && aTestTime<=(aRefTime+KModTimeThreshold));
	}

/**
If on UDEB will switch on the flag held in the CFatMountCB object to
use localtimes on removable media.
If on UREL will just set the UTC offset on.
*/
void CLocalTimeTest::LocalTimeForRemMediaOnL()
	{
#if defined(_DEBUG)
	User::LeaveIfError(iRFs.ControlIo(iDrive, ELocalTimeForRemovableMediaOn,NULL ,NULL) );
#else
	User::SetUTCOffset(KTimeOffset);
#endif
	}

/**
If on UDEB will switch off the flag held in the CFatMountCB object to
use localtimes on removable media.
If on UREL will just set the UTC offset to nothing,
so that no time conversions are carried out.
*/
void CLocalTimeTest::LocalTimeForRemMediaOffL()
	{

#if defined(_DEBUG)
	User::LeaveIfError(iRFs.ControlIo(iDrive, ELocalTimeForRemovableMediaOff,NULL ,NULL) );
#else
	User::SetUTCOffset(KNullTimeOffset);
#endif
	}

TBool CLocalTimeTest::IsLocalTimeOnRemMediaL()
	{
#if defined(_DEBUG)
	TBool flag(EFalse);
	TPckg<TBool> flagPckg(flag);
	User::LeaveIfError( iRFs.ControlIo(iDrive, ELocalTimeUsedOnRemovableMedia, flagPckg) );
	return flagPckg();
#else
	return( User::UTCOffset()==KTimeOffset );
#endif
	}

void CLocalTimeTest::SetTestTypeL()
	{
	TDriveInfo info;
	
	TFSName fileSystem;
	TInt err;
	err = iRFs.FileSystemName(fileSystem, iDrive);

	User::LeaveIfError(err);
	
	//not currently testing on urel due to lack of support for configuration file/estart.txt  managment
	if(iBuild==EUrel)
		{
		iTestType=ENoTest;
		return;
		}

	if(fileSystem != KFatFileSystem)
		{
		iTestType=ENoTest;
		return;
		}
	err = iRFs.Drive(info, iDrive);
	User::LeaveIfError(err);

	if(info.iDriveAtt&KDriveAttRemovable)
		{
		iTestType=EPositive;
		}
	else
		{
		iTestType=ENegative;
		}
	return;
	}



/**
@return Drive letter being tested
*/
const TDesC& CLocalTimeTest::DriveLetter() const
	{
	return iDriveLetter;
	}
/**
@return Drive number of test.
*/
TInt CLocalTimeTest::DriveNumber() const
	{
	return iDrive;
	}

/**
Print the drive number and letter of the test to the RTest console.
*/
void CLocalTimeTest::PrintDrive() const
	{
	TFSName fileSystem;
	TInt err;
	err = iRFs.FileSystemSubType(iDrive, fileSystem);
	test_KErrNone(err);
	test.Printf(_L("Using drive %d %S: Fs Type: %S\n"), DriveNumber(), &DriveLetter(), &fileSystem );
	}

void CLocalTimeTest::PrintExpectedOffset() const
	{
	iTest.Printf(_L("Expected offset: %d hours\n"), iExpectedTimeStampOffset.Int()/KSecondsPerHour);
	}

/**
Create directories for the test if necessary.
*/
void CLocalTimeTest::MakeTestPathL()
	{
	iTestPath.Append(iDriveLetter);
	iTestPath.Append(KDriveDelimiter);
	iTestPath.Append(KPathDelimiter);
	iTestPath.Append(KTestDir);
	iTestPath.Append(KPathDelimiter);
		
	TInt err=iRFs.MkDirAll(iTestPath);
	if(err!=KErrNone && err!=KErrAlreadyExists)
		User::Leave(err);
	iRFs.SetSessionPath(iTestPath);
	}

void CLocalTimeTest::PrintTimeL(const TDesC& aMessg, const TTime& aTime) const
	{
	TBuf<32> timeBuf;
	_LIT(KTimeFormat, "%F%H:%T:%S");
	aTime.FormatL(timeBuf, KTimeFormat);

	iTest.Printf(_L("%S: %S\n"), &aMessg, &timeBuf);
	}


/**
A callback function passed into a TCleanupItem to restore the system's UTC offset at the end
of the test.
*/
void RestoreOffset(TAny* aOffset)
	{
	User::SetUTCOffset(*static_cast<TTimeIntervalSeconds*>(aOffset) );
	}






////////////////////////////////////////////////
//////////CFileSystemEntry/////////////////////
////////////////////////////////////////////////

/**
@param aPath Name or full path for entry.
@param aFs the RFs handle to be used.
*/
CFileSystemEntry::CFileSystemEntry(const TDesC& aPath, RFs* aFs )
	: iRFs(aFs), iFullPath(aPath)
	{
	}

CFileSystemEntry::~CFileSystemEntry()
	{}

/**
Prepends a path to the existing name or path.
@param aPath The path to use.
*/
void CFileSystemEntry::SetPath(const TDesC& aPath)
	{
	iFullPath.Insert(0, aPath);
	}

void CFileSystemEntry::SetFileServer(RFs* aFs)
	{
	iRFs = aFs;	
	}

/**
Close and delete the entry.
@return An error code indicating success or failure.
*/
TInt CFileSystemEntry::Delete()
	{
	Close();
	return iRFs->Delete(iFullPath);
	}

/**
Delete and then make a new file/directory of the same name
@return An error code indicating success or failure.
*/
TInt CFileSystemEntry::DeleteCreate()
	{
	Delete();
	return Create();
	}

void CFileSystemEntry::Close()
	{	
	}

/**
@return The creation time of the entry.
*/
TTime CFileSystemEntry::CreationTimeL()
	{
	TParsePtrC parse(iFullPath);
	//check there is a drive specified
	if(!parse.DrivePresent() )
		User::Panic(KTestGroupName, KErrBadName);
	
	TInt driveNumber(0); 
	User::LeaveIfError(iRFs->CharToDrive(parse.Drive()[0], driveNumber) );

	TBuf8<KMaxPath> narrowPath;
	narrowPath.Copy(parse.Path() );
	narrowPath.Append(parse.NameAndExt() );
	
	//remove trailing slash if present
	if(narrowPath[narrowPath.Length()-1]==KPathDelimiter)
		narrowPath.Delete(narrowPath.Length()-1, 1);

	TTime creationTime=0;
	TPckg<TTime> timePckg(creationTime);

	User::LeaveIfError(iRFs->ControlIo(driveNumber, ECreationTime, narrowPath, timePckg) );

	return timePckg();
	}

const TDesC& CFileSystemEntry::Name() const
	{
	return iName;
	}


////////////////////////////////////////////////
//////////CTestDirectory////////////////////////
////////////////////////////////////////////////

CTestDirectory::CTestDirectory(const TDesC& aPath, RFs* aFs)
:CFileSystemEntry(aPath, aFs)
	{
	iName.Set(KTestDirectoryName);
	}

TInt CTestDirectory::Open()
	{
	return KErrNone; //directories can't be opened.
	}

TInt CTestDirectory::Create()
	{
	return iRFs->MkDir(iFullPath);
	}

TInt CTestDirectory::Delete()
	{
	return iRFs->RmDir(iFullPath );
	}

TTime CTestDirectory::ModificationTimeL()
	{
	TTime time;
	User::LeaveIfError( iRFs->Modified(iFullPath, time) );
	return time;
	}
TInt CTestDirectory::SetModificationTime(const TTime& aTime)
	{
	return iRFs->SetModified(iFullPath, aTime);
	}
CFileSystemEntry* CTestDirectory::CopyL()
	{
	return NULL;
	}


////////////////////////////////////////////////
//////////CTestFile/////////////////////////////
////////////////////////////////////////////////

CTestFile::CTestFile(const TDesC& aPath, RFs* aFs)
:CFileSystemEntry(aPath, aFs)
	{
	iName.Set(KTestFileRFile);
	}

CTestFile::~CTestFile()
	{
	Close();
	}

TInt CTestFile::Open()
	{
	return iRFile.Open(*iRFs, iFullPath, EFileShareExclusive|EFileWrite);
	}

TInt CTestFile::Create()
	{
	return iRFile.Replace(*iRFs, iFullPath, EFileShareExclusive|EFileWrite);	
	}
void CTestFile::Close()
	{
	iRFile.Close();
	}

TTime CTestFile::ModificationTimeL()
	{
	TTime time;
	User::LeaveIfError(iRFile.Modified(time) );
	return time;
	}

TInt CTestFile::SetModificationTime(const TTime& aTime)
	{
	return iRFile.SetModified(aTime);
	}

CFileSystemEntry* CTestFile::CopyL()
	{
	CFileMan* fMan = CFileMan::NewL(*iRFs);
	CleanupStack::PushL(fMan);
	
	TFileName newName(iFullPath);
	newName.Append(_L("~"));
	CFileSystemEntry* copy= new(ELeave) CTestFile(newName, iRFs);
	CleanupStack::PushL(copy);
	copy->Delete(); //delete anything at the path already
	Close();
	User::LeaveIfError(fMan->Copy(iFullPath,newName) );
	
	CleanupStack::Pop(copy);
	CleanupStack::PopAndDestroy(fMan);
	return copy;
	}

////////////////////////////////////////////////
////////////CTestFileRFs////////////////////////
////////////////////////////////////////////////
CTestFileRFs::CTestFileRFs(const TDesC& aPath, RFs* aFs) : CTestFile(aPath, aFs)
	{
	iName.Set(KTestFileRFs);
	}
TTime CTestFileRFs::ModificationTimeL()
	{
	TBool isOpen=KErrNone;
	test_KErrNone(iRFs->IsFileOpen(iFullPath, isOpen));
	if(isOpen)
		Close();
	TTime time;
	User::LeaveIfError(iRFs->Modified(iFullPath,time) );
	if(isOpen)
		Open();
	return time;
	
	}

TInt CTestFileRFs::SetModificationTime(const TTime& aTime)
	{
	TBool isOpen=KErrNone;
	test_KErrNone(iRFs->IsFileOpen(iFullPath, isOpen));
	if(isOpen)
		Close();
	TInt err = iRFs->SetModified(iFullPath, aTime);
	if(isOpen)
		Open();

	return err;
	}

CTestFileRFs::~CTestFileRFs()
	{
	Close();
	}


////////////////////////////////////////////////////
//////////Entry point and main//////////////////////
////////////////////////////////////////////////////

/**
Construct and run the various tests.
*/
void CallTestsL()
	{
	test.Start(KTestGroupName);
#if defined(_DEBUG)
	CLocalTimeTest::TBuild build = CLocalTimeTest::EUdeb;
#else
	CLocalTimeTest::TBuild build = CLocalTimeTest::EUrel;
#endif
	TPtrC drive((TUint16*)&gDriveToTest, 1);
	
	CLocalTimeTest* timeTest= CLocalTimeTest::NewLC(test, drive , build);
	timeTest->RunTestsL();
	CleanupStack::PopAndDestroy(timeTest);

	test.End();
	}

void CLocalTimeTest::RunTestsL()
	{
	PrintDrive();
	if(iTestType==ENoTest)
		{
		iTest.Printf(_L("Not runnning tests on this drive\n"));
		return;
		}
	
	iTest.Start(_L("Running tests")); 

	//Be able to restore to original timezone after test
	TTimeIntervalSeconds savedUTCOffset = User::UTCOffset();
	TCleanupItem restoreOffset(RestoreOffset, &savedUTCOffset);
	CleanupStack::PushL(restoreOffset);
	
	//This functionallity must be tested with a non-zero GMT offset.
	test.Printf(_L("Setting UTC offset to %d hours\n"), KHoursOffset);
	User::SetUTCOffset(KTimeOffset);
	
	if(iBuild==EUdeb)
		iTest.Printf(_L("Testing on UDEB build\n"));
	else if(iBuild==EUrel)
		iTest.Printf(_L("Testing on UREL build\n"));

	if(iTestType==EPositive)
		iTest.Printf(_L("Drive is removable, running positive tests\n"));
	else if(iTestType==ENegative)
		iTest.Printf(_L("Drive is non-removable, running negative tests\n"));
			
	
	if(iBuild==EUdeb) //these tests cannot be used without ControlIO
		{
		TestDebugInterfaceL();
		TestReadCreationTimeL(iTestFile);
		TestReadCreationTimeL(iTestFileRFs);
		TestReadCreationTimeL(iTestDirectory);
		}
	
	TestReadModificationTimeL(iTestFile);
	TestReadModificationTimeL(iTestFileRFs);
	TestReadModificationTimeL(iTestDirectory);

	TestSetModificationTimeL(iTestFile);
	TestSetModificationTimeL(iTestFileRFs);
	TestSetModificationTimeL(iTestDirectory);
	
	TestCopyL(iTestFile);
	TestCopyL(iTestFileRFs);
	TestCopyDirL();

	CleanupStack::PopAndDestroy(&savedUTCOffset);
	iTest.End();
	}
