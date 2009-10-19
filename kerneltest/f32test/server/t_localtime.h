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
// Declarations for the t_localtime test. This tests functionality introduced
// in CR1084 ie. That removable (in practice this means FAT) file systems
// can be made to use local time for timestamps.
// 
//

/**
 @file
 @test
*/


#ifndef T_LOCALTIME_H
#define T_LOCALTIME_H

#include <f32file.h>



/**
Provides a common test interface for files and directories.
*/
class CFileSystemEntry : public CBase 
	{
	
public:
	virtual ~CFileSystemEntry()=0;
	virtual TInt Delete();
	virtual TInt Create()=0;
	virtual void SetPath(const TDesC& aPath);
	virtual void SetFileServer(RFs* aFs);
	virtual TInt Open()=0;
	virtual TInt DeleteCreate();	
	virtual void Close();
	virtual TTime ModificationTimeL()=0;
	virtual TInt SetModificationTime(const TTime&)=0;
	virtual TTime CreationTimeL();				
	virtual CFileSystemEntry* CopyL()=0;
	virtual const TDesC& Name() const;

protected:
	CFileSystemEntry(const TDesC& aPath, RFs* aFs);
	RFs* iRFs; ///<A pointer to some external RFs
	TFileName iFullPath; ///<Name of the file or directory.
	TPtrC iName;
	};

_LIT(KTestDirectoryName, "Directory");
/**
Implementation of interface to directories
*/
class CTestDirectory : public CFileSystemEntry
	{
public:
	CTestDirectory(const TDesC& aPath, RFs* aFs = NULL);
	virtual TInt Open();
	virtual TInt Delete();
	virtual TInt Create();
	virtual TTime ModificationTimeL();
	virtual TInt SetModificationTime(const TTime&);
	virtual CFileSystemEntry* CopyL();
	};

_LIT(KTestFileRFile, "File - accessed with RFile");
/**
File interface, uses RFile to access mod time
*/
class CTestFile : public CFileSystemEntry
	{
public:
	CTestFile(const TDesC& aPath, RFs* aFs = NULL);
	virtual TInt Open();
	virtual TInt Create();
	virtual void Close();
	virtual TTime ModificationTimeL();
	virtual TInt SetModificationTime(const TTime&);
	virtual CFileSystemEntry* CopyL();
	virtual ~CTestFile();

protected:
	RFile iRFile; ///< Handle to the file the class is wrapping.
	};

_LIT(KTestFileRFs, "File - accessed with RFs");
/**
File interface-uses RFs to access mod time
*/
class CTestFileRFs : public CTestFile
	{
public:
	CTestFileRFs(const TDesC& aPath, RFs* aFs = NULL);
	virtual TTime ModificationTimeL();
	virtual TInt SetModificationTime(const TTime&);
	virtual ~CTestFileRFs();

	};



_LIT(KTestGroupName, "Local timestamps on removable media");

_LIT(KTestDir, "F32-TST\\t_localtime");


//For emulator
_LIT(KNonRemovableDrive, "Y:\\");
_LIT(KRemovableDrive, "X:\\");


_LIT(KFile,"utc-test-file");
_LIT(KDirectory,"utc-test-dir\\");
_LIT(KRemMedia, "removable-media");
_LIT(KNonRemMedia, "non-removable-media");


_LIT(KFatFileSystem, "Fat");
_LIT(KFat32FileSystem, "FAT32");
_LIT(KFat16FileSystem, "FAT16");

const TInt KSecondsPerHour(3600);
const TInt KHoursOffset(6);
const TTimeIntervalSeconds KTimeOffset(KHoursOffset*KSecondsPerHour);
const TTimeIntervalSeconds KNullTimeOffset(0);
const TTimeIntervalSeconds KModTimeThreshold(3); //FAT timestamp resolution is 2 seconds, +1 tolerence for delays. 

/**
Base class for the t_localtime tests. Defines generic test steps as well as utility
functions. Subclassed tests implement RunTests to call specific test steps.

*/
class CLocalTimeTest : public CBase
	{
public:
	/**
	Used as a parameter to tell test what type of drive to use
	*/
	enum TDriveType
	{
		ERemovable,
		ENonRemovable
	};

	enum TBuild
	{
		EUdeb,
		EUrel
	};

	enum TTestType
	{
		EPositive, ///< Times are expected to be translated
		ENegative, ///<Times are expected to be preserved
		ENoTest ///<Tests won't be carried out.
	};
	
	static CLocalTimeTest* NewLC(RTest& aTest, const TDesC& aDriveLetter, TBuild aBuild);
	virtual ~CLocalTimeTest();
	
	void RunTestsL(); ///<Defines what steps the test will carry out.
	
	/////// Generic test steps ///////////
	void TestDebugInterfaceL();
	void TestReadCreationTimeL(CFileSystemEntry* aFsEntry);
	void TestReadModificationTimeL(CFileSystemEntry* aFsEntry);
	void TestSetModificationTimeL(CFileSystemEntry* aFsEntry);
	void TestCopyL(CFileSystemEntry* aFsEntry);
	void TestCopyDirL();

	////////Utility Functions/////////
	static TBool FuzzyTimeMatch(const TTime& aTestTime, const TTime& aRefTime);
	void LocalTimeForRemMediaOnL();
	void LocalTimeForRemMediaOffL();
	TBool IsLocalTimeOnRemMediaL();
	void PrintTimeL(const TDesC& aMessg, const TTime& aTime) const;
	const TDesC& DriveLetter() const;
	TInt DriveNumber() const;
	void PrintExpectedOffset() const;
	void PrintDrive() const;
	
protected:
	CLocalTimeTest(RTest& aTest, const TDesC& aDriveLetter, TBuild aBuild);
	void ConstructL();
	
	void MakeTestPathL();
	void SetTestTypeL();

	TDriveType iDriveType; ///< Can be removable or non-removable
	TInt iDrive; ///<Stores the number of the drive in use.
	TBuf<1> iDriveLetter; ///<Stores the letter of the drive in use.
	
	CTestFile* iTestFile; ///<The file or directory to be used in the test
	CTestFileRFs* iTestFileRFs;
	CTestDirectory* iTestDirectory;

	TPath iTestPath; ///<The absolute path of the test directory
	RTest& iTest;	///<The test to be used
	TTestType iTestType;
	TBuild iBuild;
	
	/**
	The expected time difference the test expects to see between files' or directories'
	actual timestamps and the ones reported through the api. If local time timestamps are not in
	use then this member will be 0.
	*/	
	TTimeIntervalSeconds iExpectedTimeStampOffset;

	RFs iRFs; ///< File server handle for the test.
	TBool iOriginalUseLocalTimeFlag; ///<Stored at begginng of test, must be restored at end
	};


/**
Defines opcodes for RFs::ControlIO functions used. These definitions must match
up with those in F32\sfat\inc\sl_std.h and F32\sfat32\inc\sl_std.h
*/
enum TUTCControlIO
	{
	ELocalTimeForRemovableMediaOn=10, ///< 10
	ELocalTimeForRemovableMediaOff=11, ///< 11
	ELocalTimeUsedOnRemovableMedia=12, ///< 12
	ECreationTime=13 ///<13
	};



#endif// T_LOCALTIME_H

