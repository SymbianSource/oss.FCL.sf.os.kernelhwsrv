// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_main.cpp
//
//

#define __E32TEST_EXTENSION__

#include <f32file.h>
#include <e32test.h>
#include <e32math.h>
#include <f32dbg.h>
#include "t_server.h"

GLDEF_D	RFs TheFs;
GLDEF_D TFileName gSessionPath;
GLDEF_D TInt gAllocFailOff=KAllocFailureOff;
GLDEF_D TInt gAllocFailOn=KAllocFailureOff;
GLDEF_D TInt64 gSeed=51703;

GLDEF_D TChar gDriveToTest;
GLDEF_D TVolumeInfo gVolInfo;	// volume info for current drive
GLDEF_D TFileCacheFlags gDriveCacheFlags;

_LIT(KPrivate, "\\Private\\");


////////////////////////////////////////////////////////////
// Template functions encapsulating ControlIo magic
//
GLDEF_D template <class C>
GLDEF_C TInt controlIo(RFs &fs, TInt drv, TInt fkn, C &c)
{
    TPtr8 ptrC((TUint8 *)&c, sizeof(C), sizeof(C));

    TInt r = fs.ControlIo(drv, fkn, ptrC);

    return r;
}


GLDEF_C void CreateShortName(TDes& aFileName,TInt64& aSeed)
//
// Create a random, dos legal 8.3 char name
//
	{

	TInt length=Math::Rand(aSeed)%11;
	if (length==0)
		length=1;
	else if (length==3)	// don't create three letter names like 'AUX' or 'PRN'
		length++;
	else if (length>8)	// end in '.' if no extension
		length++;

	aFileName.SetLength(length);
	for(TInt i=0;i<length;i++)
		{
		if (i==9)
			{
			aFileName[i]='.';
			continue;
			}
		TInt letter=Math::Rand(aSeed)%26;
		aFileName[i]=(TText)('A'+letter);
		}
	}


GLDEF_C void CreateLongName(TDes& aFileName,TInt64& aSeed,TInt aLength)
//
// Create a random, dos legal 8.3 char name
//
	{

	TInt length;
	if (aLength>0)
		length=aLength;
	else
		{
		length=Math::Rand(aSeed)%128;
		length+=Math::Rand(aSeed)%128;
		length+=Math::Rand(aSeed)%128;
		length+=Math::Rand(aSeed)%128;
		length-=256;
			length=Abs(length);
		if (length==0)
			length=1;
		if (length>220)
			length=31;
		}
	if (length==3)	// don't create three letter names like 'AUX' or 'PRN'
		length++;

	aFileName.SetLength(length);
	TInt spaceChar=-1;
	TInt i;
	for(i=0;i<length;i++)
		{
StartAgain:
		TChar letter=0;
		TBool illegalChar=ETrue;

		while(illegalChar)
			{
#if defined(__WINS__)
			if (gSessionPath[0]=='C')
				letter=(TChar)('A'+Math::Rand(aSeed)%26);
			else
				letter=(TChar)Math::Rand(aSeed)%256;
#else
			letter=(TChar)Math::Rand(aSeed)%256;
#endif
			TBool space=letter.IsSpace();
			if (space && spaceChar==-1)
				spaceChar=i;
			else if (!space && spaceChar!=-1)
				spaceChar=-1;

			switch(letter)
				{
			case '<':
			case '>':
			case ':':
			case '"':
			case '/':
			case '|':
			case '*':
			case '?':
			case '\\':
			case '\0':
				break;
			default:
				illegalChar=EFalse;
				};
			}
		aFileName[i]=(TText)letter;
		}

	if (spaceChar!=-1)
		{
		i=spaceChar;
		goto StartAgain;
		}
	}


GLDEF_C void CheckDisk()
//
// Do a checkdisk and report failure
//
	{
	test.Next(_L("Check Disk"));
	TInt r=TheFs.CheckDisk(gSessionPath);
	if (r!=KErrNone && r!=KErrNotSupported && r!=KErrPermissionDenied)
		ReportCheckDiskFailure(r);
	}

GLDEF_C void ReportCheckDiskFailure(TInt aRet)
//
// Report the failure of checkdisk
//
	{

	test.Printf(_L("CHECKDISK FAILED: "));
	switch(aRet)
		{
	case 1:	test.Printf(_L("File cluster chain contains a bad value (<2 or >maxCluster)\n")); break;
	case 2:	test.Printf(_L("Two files are linked to the same cluster\n")); break;
	case 3:	test.Printf(_L("Unallocated cluster contains a value != 0\n"));	break;
	case 4:	test.Printf(_L("Size of file != number of clusters in chain\n")); break;
	default: test.Printf(_L("Undefined Error value %d\n"),aRet);
		}
	test(EFalse);
	}


GLDEF_C void MakeFile(const TDesC& aFileName,const TUidType& aUidType,const TDesC8& aFileContents)
//
// Make a file and write uid and data
//
	{

	RFile file;
	TInt r=file.Replace(TheFs,aFileName,0);
	if (r==KErrPathNotFound)
		{
		r=TheFs.MkDirAll(aFileName);
		test_KErrNone(r);
		r=file.Replace(TheFs,aFileName,0);
		}
	test_KErrNone(r);
	TCheckedUid checkedUid(aUidType);
	TPtrC8 uidData((TUint8*)&checkedUid,sizeof(TCheckedUid));
	r=file.Write(uidData);
	test_KErrNone(r);
	r=file.Write(aFileContents);
	test_KErrNone(r);
	file.Close();
	}

GLDEF_C void MakeFile(const TDesC& aFileName,const TDesC8& aFileContents)
//
// Make a file and write something in it
//
	{

	RFile file;
	TInt r=file.Replace(TheFs,aFileName,0);
	if (r==KErrPathNotFound)
		{
		r=TheFs.MkDirAll(aFileName);
		test_KErrNone(r);
		r=file.Replace(TheFs,aFileName,0);
		}
	test_KErrNone(r);
	r=file.Write(aFileContents);
	test_KErrNone(r);
	file.Close();
	}

GLDEF_C void MakeFile(const TDesC& aFileName,TInt anAttributes)
//
// Make a file and write something in it
//
	{

	RFile file;
	TInt r=file.Replace(TheFs,aFileName,0);
	if (r==KErrPathNotFound)
		{
		r=TheFs.MkDirAll(aFileName);
		test_KErrNone(r);
		r=file.Replace(TheFs,aFileName,0);
		}
	test_KErrNone(r);
	file.Close();
	r=TheFs.SetAtt(aFileName,anAttributes,0);
	test_KErrNone(r);
	}

GLDEF_C void MakeFile(const TDesC& aFileName)
//
// Make a file
//
	{

	MakeFile(aFileName,_L8(""));
	}

GLDEF_C void MakeDir(const TDesC& aDirName)
//
// Make a directory
//
	{

	TInt r=TheFs.MkDirAll(aDirName);
	if (r!=KErrNone && r!=KErrAlreadyExists)
		{
		test.Printf(_L("%c: MakeDir Error %d\n"),aDirName[0],r);
		test(0);
		}
	}


GLDEF_C void DeleteTestDirectory()
//
// Delete the leaf session path directory
//
	{

	TheFs.SetAtt(_L("\\F32-TST\\SCANTEST\\Left\\Dir3\\Dir4\\Hidden"), 0, KEntryAttHidden);
	TheFs.SetAtt(_L("\\F32-TST\\SCANTEST\\Left\\Dir3\\Dir4\\Hidden\\HiddenFile"), 0, KEntryAttHidden);
	TheFs.SetAtt(_L("\\F32-TST\\SCANTEST\\Left\\Dir3\\Dir4\\Hidden\\System"), 0, KEntryAttSystem);
	test.Next(_L("Delete test directory"));
	CFileMan* fMan=CFileMan::NewL(TheFs);
	test(fMan!=NULL);
	TInt r=TheFs.SessionPath(gSessionPath);
	test_KErrNone(r);
	r=TheFs.CheckDisk(gSessionPath);
	if (r!=KErrNone && r!=KErrNotSupported)
		ReportCheckDiskFailure(r);
	r=fMan->RmDir(gSessionPath);
	test_KErrNone(r);
	delete fMan;
	}

GLDEF_C void CreateTestDirectory(const TDesC& aSessionPath)
//
// Create directory for test
//
	{
	TParsePtrC path(aSessionPath);
	test(path.DrivePresent()==EFalse);

	TInt r=TheFs.SetSessionPath(aSessionPath);
	test_KErrNone(r);
	r=TheFs.SessionPath(gSessionPath);
	test_KErrNone(r);
	r=TheFs.MkDirAll(gSessionPath);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	}

GLDEF_C TInt CurrentDrive()
//
// Return the current drive number
//
	{

	TInt driveNum;
	TInt r=TheFs.CharToDrive(gSessionPath[0],driveNum);
	test_KErrNone(r);
	return(driveNum);
	}

GLDEF_C void Format(TInt aDrive)
//
// Format current drive
//
	{

	test.Next(_L("Format"));
	TBuf<4> driveBuf=_L("?:\\");
	driveBuf[0]=(TText)(aDrive+'A');
	RFormat format;
	TInt count;
	TInt r=format.Open(TheFs,driveBuf,EQuickFormat,count);
	test_KErrNone(r);
	while(count)
		{
		TInt r=format.Next(count);
		test_KErrNone(r);
		}
	format.Close();
	}

LOCAL_C void PushLotsL()
//
// Expand the cleanup stack
//
	{
	TInt i;
	for(i=0;i<1000;i++)
		CleanupStack::PushL((CBase*)NULL);
	CleanupStack::Pop(1000);
	}


LOCAL_C void DoTests(TInt aDrive)
//
// Do testing on aDrive
//
	{

	gSessionPath=_L("?:\\F32-TST\\");
	TChar driveLetter;
	TInt r=TheFs.DriveToChar(aDrive,driveLetter);
	test_KErrNone(r);
	gSessionPath[0]=(TText)driveLetter;
	r=TheFs.SetSessionPath(gSessionPath);
	test_KErrNone(r);

	User::After(1000000);

//	Format(CurrentDrive());

	test.Printf(_L("Creating session path"));
	r=TheFs.MkDirAll(gSessionPath);
	if(r == KErrCorrupt)
		{
		test.Printf(_L("Attempting to create directory \'%S\' failed, KErrCorrupt\n"), &gSessionPath);
		test.Printf(_L("This could be caused by a previous failing test, or a test media defect\n"));
		test.Printf(_L("Formatting drive, retrying MkDirall\nShould subsequent tests fail with KErrCorrupt (%d) as well, replace test medium !\n"),
			r);
		Format(aDrive);
		r=TheFs.MkDirAll(gSessionPath);
		test_KErrNone(r);
		}
	else if (r == KErrNotReady)
		{
		TDriveInfo d;
		r=TheFs.Drive(d, aDrive);
		test_KErrNone(r);
		if (d.iType == EMediaNotPresent)
			test.Printf(_L("%c: Medium not present - cannot perform test.\n"), (TUint)driveLetter);
		else
			test.Printf(_L("medium found (type %d) but drive %c: not ready\nPrevious test may have hung; else, check hardware.\n"), (TInt)d.iType, (TUint)driveLetter);
		}
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	TheFs.ResourceCountMarkStart();
	test.Printf(_L("Calling main test sequence ...\n"));
	TRAP(r,CallTestsL());
	test_KErrNone(r);
	test.Printf(_L("test sequence completed without error\n"));
	TheFs.ResourceCountMarkEnd();

	CheckDisk();
	}


void ParseCommandArguments()
//
//
//
	{
	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	TPtrC token=lex.NextToken();
	TFileName thisfile=RProcess().FileName();
	if (token.MatchF(thisfile)==0)
		{
		token.Set(lex.NextToken());
		}
	test.Printf(_L("CLP=%S\n"),&token);

	if(token.Length()!=0)
		{
		gDriveToTest=token[0];
		gDriveToTest.UpperCase();
		}
	else
		gDriveToTest='C';
	}

TFullName gExtName;
TBool gPrimaryExtensionExists = EFalse;

GLDEF_C TInt DismountFileSystem(RFs& aFs, const TDesC& aFileSystemName,TInt aDrive)
	{
	//Make note of the first extension if it exists, so that we remount
	//it when the file system is remounted.
	TInt r = aFs.ExtensionName(gExtName, aDrive, 0);

	if (r == KErrNone)
		{
		gPrimaryExtensionExists = ETrue;
		}
	return aFs.DismountFileSystem(aFileSystemName, aDrive);
	}

GLDEF_C TInt MountFileSystem(RFs& aFs, const TDesC& aFileSystemName,TInt aDrive, TBool aIsSync)
	{
	TInt r;
	if (gPrimaryExtensionExists)
		{
		r = aFs.MountFileSystem(aFileSystemName, gExtName, aDrive, aIsSync);
		}
	else
		{
		r = aFs. MountFileSystem(aFileSystemName, aDrive, aIsSync);
		}
	return r;
	}

GLDEF_C TInt E32Main()
//
// Test with drive nearly full
//
    {

	CTrapCleanup* cleanup;
	cleanup=CTrapCleanup::New();
	TRAPD(r,PushLotsL());
	__UHEAP_MARK;

	test.Title();
	test.Start(_L("Starting tests..."));


	ParseCommandArguments(); //need this for drive letter to test


	r=TheFs.Connect();
	test_KErrNone(r);
	TheFs.SetAllocFailure(gAllocFailOn);
	TTime timerC;
	timerC.HomeTime();
	TFileName sessionp;
	TheFs.SessionPath(sessionp);

	TBuf<30> privatedir;
	privatedir = KPrivate;

	TUid thisUID = RProcess().Identity();
	privatedir.AppendFormat(_L("%08x"),thisUID.iUid);
	privatedir.Append(_L("\\"));

	test(privatedir == sessionp.Mid(2,sessionp.Length()-2));

	test.Printf(_L("sp=%S\n"),&sessionp);
	sessionp[0]=(TText)gDriveToTest;
	test.Printf(_L("sp1=%S\n"),&sessionp);

	TInt theDrive;
	r=TheFs.CharToDrive(gDriveToTest,theDrive);
	test_KErrNone(r);

	// Get the TFileCacheFlags for this drive
	r = TheFs.Volume(gVolInfo, theDrive);
	if (r == KErrNotReady)
		{
		TDriveInfo info;
		TInt err = TheFs.Drive(info,theDrive);
		test_KErrNone(err);
		if (info.iType == EMediaNotPresent)
			test.Printf(_L("%c: Medium not present - cannot perform test.\n"), (TUint)gDriveToTest);
		else
			test.Printf(_L("%c: medium found (type %d) but drive not ready\nPrevious test may have hung; else, check hardware.\n"), (TUint)gDriveToTest, (TInt)info.iType);
		}
	else if (r == KErrCorrupt)
		{
		test.Printf(_L("%c: Media corruption; previous test may have aborted; else, check hardware\n"), (TUint)gDriveToTest);
		}
	test_KErrNone(r);
	gDriveCacheFlags = gVolInfo.iFileCacheFlags;
	test.Printf(_L("DriveCacheFlags = %08X\n"), gDriveCacheFlags);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	TPckgBuf<TIOCacheValues> pkgOrgValues;
	TIOCacheValues& orgValues=pkgOrgValues();
	r = controlIo(TheFs,theDrive, KControlIoCacheCount, orgValues);
	test_KErrNone(r);

	test.Printf(_L("\n"));
	test.Printf(_L("Requests on close queue at start=%d\n"),orgValues.iCloseCount);
	test.Printf(_L("Requests on free queue at start=%d\n"),orgValues.iFreeCount);
	test.Printf(_L("Requests dynamically allocated at start=%d\n"),orgValues.iAllocated);
	test.Printf(_L("Requests in total at start=%d\n"),orgValues.iTotalCount);

	// File cache

	// flush closed files queue
	r = TheFs.ControlIo(theDrive, KControlIoFlushClosedFiles);
	test_KErrNone(r);

	// get number of items on File Cache
	TFileCacheStats startFileCacheStats;
	r = controlIo(TheFs,theDrive, KControlIoFileCacheStats, startFileCacheStats);
	test_Value(r, r == KErrNone || r == KErrNotSupported);
	test.Printf(_L("File cache: Cachelines (free %d, used %d), Segments(allocated %d locked %d). Closed files(%d)\n"),
		startFileCacheStats.iFreeCount,
		startFileCacheStats.iUsedCount,
		startFileCacheStats.iAllocatedSegmentCount,
		startFileCacheStats.iLockedSegmentCount,
		startFileCacheStats.iFilesOnClosedQueue);
#endif

	DoTests(theDrive);

	TTime endTimeC;
	endTimeC.HomeTime();
	TTimeIntervalSeconds timeTakenC;
	r=endTimeC.SecondsFrom(timerC,timeTakenC);
	test_KErrNone(r);
	test.Printf(_L("Time taken for test = %d seconds\n"),timeTakenC.Int());
	TheFs.SetAllocFailure(gAllocFailOff);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	TPckgBuf<TIOCacheValues> pkgValues;
	TIOCacheValues& values=pkgValues();
	r = controlIo(TheFs,theDrive, KControlIoCacheCount, values);
	test_KErrNone(r);

	test.Printf(_L("Requests on close queue at end=%d\n"),values.iCloseCount);
	test.Printf(_L("Requests on free queue at end=%d\n"),values.iFreeCount);
	test.Printf(_L("Requests dynamically allocated at end=%d\n"),values.iAllocated);
	test.Printf(_L("Requests in total at end=%d\n"),values.iTotalCount);

	test(orgValues.iCloseCount==values.iCloseCount);
	test(orgValues.iAllocated == values.iAllocated);
	// The free count can increase if the file server runs out of requests in the RequestAllocator
	// free pool but this should never decrease - this implies a request leak
	test(orgValues.iFreeCount <= values.iFreeCount);

	// The total number of allocated requests should be equal to :
	// requests on the close queue + requests on free queue
	// + 1 (because we used one request to issue KControlIoCacheCount)
	// If this doesn't equate then this implies a request leak
	test(values.iTotalCount == values.iCloseCount + values.iFreeCount + 1);

	// File cache
	TFileCacheStats endFileCacheStats;
	r = controlIo(TheFs,theDrive, KControlIoFileCacheStats, endFileCacheStats);
	test_Value(r, r == KErrNone || r == KErrNotSupported);

	test.Printf(_L("File cache: Cachelines (free %d, used %d), Segments(allocated %d locked %d). Closed files(%d)\n"),
		endFileCacheStats.iFreeCount,
		endFileCacheStats.iUsedCount,
		endFileCacheStats.iAllocatedSegmentCount,
		endFileCacheStats.iLockedSegmentCount,
		endFileCacheStats.iFilesOnClosedQueue);

	// flush closed files queue
	test.Printf(_L("Flushing close queue..."));
	r = TheFs.ControlIo(theDrive, KControlIoFlushClosedFiles);
	test_KErrNone(r);

	r = controlIo(TheFs,theDrive, KControlIoFileCacheStats, endFileCacheStats);
	test_Value(r, r == KErrNone || r == KErrNotSupported);
	test.Printf(_L("File cache: Cachelines (free %d, used %d), Segments(allocated %d locked %d). Closed files(%d)\n"),
		endFileCacheStats.iFreeCount,
		endFileCacheStats.iUsedCount,
		endFileCacheStats.iAllocatedSegmentCount,
		endFileCacheStats.iLockedSegmentCount,
		endFileCacheStats.iFilesOnClosedQueue);


	if (r == KErrNone)
		{
		test(startFileCacheStats.iFreeCount == endFileCacheStats.iFreeCount);
		test(startFileCacheStats.iUsedCount == endFileCacheStats.iUsedCount);
		test(startFileCacheStats.iAllocatedSegmentCount == endFileCacheStats.iAllocatedSegmentCount);
		test(startFileCacheStats.iLockedSegmentCount == endFileCacheStats.iLockedSegmentCount);
		test(startFileCacheStats.iFileCount == endFileCacheStats.iFileCount);
		}
#endif

	TheFs.Close();
	test.End();
	test.Close();
	__UHEAP_MARKEND;
	delete cleanup;
	return(KErrNone);
    }


