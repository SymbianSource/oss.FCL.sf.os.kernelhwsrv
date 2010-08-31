// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_corruptlog.cpp
// Includes
// 
//

//! @file
//! @SYMTestCaseID FSBASE_corrupt_log
//! @SYMPREQ 908
//! @SYMTestSuites Base tests
//! @SYMInitialAuthor Bob Jackman
//! @SYMCreationDate 1/11/04
//! @SYMTestCaseDesc Check trap action of CorruptFileNames.lst

#define __E32TEST_EXTENSION__
#include <e32test.h>
RTest test(_L("t_corruptlog"));

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
#include <f32file.h>
#include <f32dbg.h>

// Error every time
_LIT(KTestFile1,"z:\\system\\data\\BadFile1.txt");
const TInt KTestFile1Error1=-6;
const TInt KTestFile1Error2=KErrNone;
//
// Error once
_LIT(KTestFile2,"z:\\system\\data\\BadFile2.txt");
const TInt KTestFile2Error1=-20;
const TInt KTestFile2Error2=-20;
   
////////////////////////////////////////////////////////////
// Globals
//
static RFs TheFs;

static TInt numberOfTraps=0;

////////////////////////////////////////////////////////////

inline TInt controlIo(RFs &fs, TInt drv, TInt fn)
	{
	TPtr8 pDummy(NULL,0,0);
	TInt r = fs.ControlIo(drv, fn, pDummy);
	return r;
	}

inline TInt controlIo(RFs &fs, TInt drv, TInt fn, TInt &aNumRecs)
	{
	TPtr8 pNum((TUint8*)(&aNumRecs),sizeof(aNumRecs),sizeof(aNumRecs));
	TInt r = fs.ControlIo(drv, fn, pNum);
	return r;
	}

inline TInt controlIo(RFs &fs, TInt drv, TInt fn, TDes& aFileName)
	{
	TBuf8<KMaxFileName> fileName8;
	TInt r = fs.ControlIo(drv, fn, fileName8);
	if(r==KErrNone)
		{
		aFileName.Copy(fileName8);
		}
	return r;
	}


inline TInt controlIo(RFs &fs, TInt drv, TInt fn, TFsDebugCorruptLogRecordBuf &alogRec, TInt aRecNum)
	{
    TInt r = fs.ControlIo(drv, fn, alogRec, *((TDes8*)aRecNum));
    return r;
	}

void PrintLogRecord(TFsDebugCorruptLogRecordBuf &alogRec, TInt aRecordNumber)
	{
	test.Printf(_L("#%d Process: %S tried to access %S errorCode=%d\n"),aRecordNumber,
																		&(alogRec().iProcessName),
																		&(alogRec().iFileName),
																		alogRec().iError);
	}

TInt ResetCorruptLogRecords()
	{
	// Allows the test to be run again, by resetting the record to an unused state,
	// and destroying the trap records
	// Enables this test to be ran again.
	TInt r=controlIo(TheFs, EDriveC, KControlIoCorruptLogRecordReset);
	return r;
	}	

TInt GetNumberOfTraps()
	{
	// fetchs the number of corrupt file trap records that exist
	// there is a separate record for every attempted access to a nominated file
	// Note that C: is used in the IoControl call, which requires a valid drive, but has no other relevance
	TInt numberOfRecords;
	TInt r=controlIo(TheFs, EDriveC, KControlIoGetNumberOfCorruptLogRecords, numberOfRecords);
	test_KErrNone(r);
	return numberOfRecords;
	}

TInt GetTrapLogRecord(TFsDebugCorruptLogRecordBuf &alogRec, TInt aRecordNumber)
	{
	// fetchs a trap record
	// Note that C: is used in the IoControl call, which requires a valid drive, but has no other relevance
	TInt r=controlIo(TheFs, EDriveC, KControlIoGetCorruptLogRecord, alogRec, aRecordNumber);
	test_KErrNone(r);
	return r;
	}

TInt GetCorruptFileListFile(TDes& aFileName)
	{
	// Retrieves the name of the file containing the list of files, nominated as corrupt, used to generate
	// the corrupt files list.
	// Note that C: is used in the IoControl call, which requires a valid drive, but has no other relevance
	TInt r=controlIo(TheFs, EDriveC, KControlIoGetCorruptListFile, aFileName);
	test_KErrNone(r);
	return r;
	}

void AccessFiles()
	{
	test.Next(_L("Access corrupt files"));
	RFile f;
	const TInt attribs=EFileShareExclusive|EFileStreamText|EFileRead;
	// File1 
	TInt r=f.Open(TheFs,KTestFile1,attribs);
	test_Value(r, r == KTestFile1Error1);
	f.Close();
	numberOfTraps+=(r==KErrNone?0:1);
	// try again
	r=f.Open(TheFs,KTestFile1,attribs);
	test_Value(r, r == KTestFile1Error2);
	f.Close();
	numberOfTraps+=(r==KErrNone?0:1);
	// File2 
	r=f.Open(TheFs,KTestFile2,attribs);
	test_Value(r, r == KTestFile2Error1);
	f.Close();
	numberOfTraps+=(r==KErrNone?0:1);
	// try again
	r=f.Open(TheFs,KTestFile2,attribs);
	test_Value(r, r == KTestFile2Error2);
	f.Close();
	numberOfTraps+=(r==KErrNone?0:1);
	}

void DoTests()
	{
	TFileName corruptFileNamesList;
    test.Next(_L("Get name of file with list of nominated files"));
	TInt r=GetCorruptFileListFile(corruptFileNamesList);
    test_KErrNone(r);
 	test.Printf(_L("Using %S\n"),&corruptFileNamesList);

	AccessFiles();
    test.Next(_L("Get Number of traps"));
	TInt nRecs=GetNumberOfTraps();
    test.Next(_L("Test Number of traps"));
	test(nRecs==numberOfTraps);

	TFsDebugCorruptLogRecordBuf logRec;
	for (TInt i=1;i<=nRecs;i++)
		{ // fetch record #i
		TInt r=GetTrapLogRecord(logRec,i);
		test_KErrNone(r);
		r=logRec().iProcessName.CompareF(_L("t_corruptlog.exe"));
		test_KErrNone(r);
		PrintLogRecord(logRec,i);
		}
	}

#endif

extern TInt E32Main()
{

    CTrapCleanup* cleanup;
    cleanup=CTrapCleanup::New();
    __UHEAP_MARK;
    
    test.Title();
    test.Start(_L("Corrupt File trap log"));
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	TInt r=TheFs.Connect();
    test_KErrNone(r);

    TheFs.ResourceCountMarkStart();
    
    DoTests();
    
    TheFs.ResourceCountMarkEnd();
	ResetCorruptLogRecords();
    TheFs.Close();
#endif
    
    test.End();
    test.Close();
    __UHEAP_MARKEND;
    delete cleanup;



    return KErrNone;
}
