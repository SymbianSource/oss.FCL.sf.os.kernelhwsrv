// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Performs some benchmarking of the LFFS media driver
// 
//

/**
 @file bf_raw.cpp
*/

#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32svr.h>
#include "bf_raw.h"
#include "user_config.h"

const TInt KAverageOverInSeconds=10;	///< Number of seconds to run tests for

//TInt64 Count;				///< Global variable used to count number of operations completed
TUint32 Count;

RTest test(_L("BF_RAW"));


TTestInfo	TestInfo;		///< Data passed to exector thread

TBusLocalDrive	drive;
TLocalDriveCapsV2Buf driveInfo;

GLDEF_D	HBufC8*	writeBuffer;	///< Buffer for transferring data
GLDEF_D	HBufC8*	readBuffer;		///< Buffer for transferring data

GLDEF_D TBool StopTest;		///< set to ETrue to stop the test


GLREF_C TInt BmWrite(TAny*);
GLREF_C TInt BmWriteThread(TAny*);
GLREF_C TInt BmRead(TAny*);
GLREF_C TInt BmReadThread(TAny*);

GLDEF_D TInt	mainThreadHandle;

TUint32 runTest(TThreadFunction aFunction)
    /**
	 * Function which actually runs the test.
	 *
	 * It creates a new thread to exeute the operations and then waits
	 * for a pre-determined time. The executor thread increments a counter
	 * each time it completes one operation. After the time period expires
	 * the counter is averaged to get an "operations per second". This is
	 * then multiplied by the data size to obtain tranfer rate
	 */
	{

    RThread thread;
	TInt r=thread.Create(_L("TESTER"),aFunction,KDefaultStackSize,&User::Heap(),NULL);
	if(r!=KErrNone)
		{
		test.Printf(_L("Failed to create thread with error %d\n"),r);
		return(r);
		}
	StopTest = EFalse;	// allow the test to run
    TRequestStatus deadStat;
	thread.Logon( deadStat );

	RThread().SetPriority( EPriorityMuchMore );

	thread.Resume();
    User::After(1000000);
    Count=0;
    User::After(KAverageOverInSeconds*1000000);
    TUint32 result=Count;
	
	// tell test to stop and wait for thread to exit.
	StopTest = ETrue;
	User::WaitForRequest( deadStat );
	
	CLOSE_AND_WAIT(thread);
    return(result);
    }

void PrintResult( TUint32 aCount, const TDesC& aTitle )
	/**
	 * Prints result of test
	 */
	{
	TInt64 count(static_cast<TUint>(aCount));
	TInt64 transferRate = (count * TestInfo.iLength) / KAverageOverInSeconds;

    test.Printf(_L("%S: %d bytes/second\n"),
					&aTitle,
					I64LOW(transferRate) );
	}


LOCAL_C TInt EraseSegment( TInt aSegmentNumber )
	/**
	 * Erases a segment on Flash
	 *
	 * @param aSegmentNumber index of segment to erase
	 * @return KErrNone or error code
	 */
	{
	TInt offset = aSegmentNumber * driveInfo().iEraseBlockSize;
	
	TInt r = drive.Format( offset, driveInfo().iEraseBlockSize );
	return  r;
	}


/**
 * Structure defining tests
 */
class TTestData
	{
	public:
		TThreadFunction		iFunction;	///< function to execute, NULL for end of list
		TInt				iLength;	///< data length
		TInt				iOffset;	///< Flash offset
		const TDesC*				iDescription;	///< descriptive text
	};


_LIT( KErasingSegment0, "Erasing segment 0" );

_LIT( KWrite1Start, "Write, 1 byte, 32-byte boundary, current thread" );
_LIT( KWrite24Start, "Write, 24 bytes, 32-byte boundary, current thread" );
_LIT( KWrite64Start, "Write, 64 bytes, 32-byte boundary, current thread" );
_LIT( KWrite512Start, "Write, 512 bytes, 32-byte boundary, current thread" );

_LIT( KRead1Start, "Read, 1 byte, 32-byte boundary, current thread" );
_LIT( KRead24Start, "Read, 24 bytes, 32-byte boundary, current thread" );
_LIT( KRead64Start, "Read, 64 bytes, 32-byte boundary, current thread" );
_LIT( KRead512Start, "Read, 512 bytes, 32-byte boundary, current thread" );

const TThreadFunction KEraseSegment = (TThreadFunction)0x000000F1;

const TTestData testData[] =		///< the test data
	{
		{ KEraseSegment, 0, 0, &KErasingSegment0 },

		{ BmWrite, 1, 0, &KWrite1Start },
		{ BmWrite, 24, 32, &KWrite24Start },
		{ BmWrite, 64, 64, &KWrite64Start },
		{ BmWrite, 512, 128, &KWrite512Start },

		{ BmRead, 1, 0, &KRead1Start },
		{ BmRead, 24, 0, &KRead24Start },
		{ BmRead, 64, 0, &KRead64Start },
		{ BmRead, 512, 0, &KRead512Start },

		{ NULL, 0, 0, NULL }
	};



void Initialize()
	/**
	 * Open channel to media driver
	 */
	{
	//
	// Load the media driver
	//
#ifndef SKIP_PDD_LOAD
	test.Printf( _L("Loading %S\n"), &KLfsDriverName );
	TInt r = User::LoadPhysicalDevice( KLfsDriverName );
	test( KErrNone == r || KErrAlreadyExists == r );
#endif

#ifdef UNMOUNT_DRIVE
	RFs fs;
	test( KErrNone == fs.Connect() );
#if 0
	// XXX - not EKA2
	test( KErrNone == fs.SetDefaultPath( _L("Z:\\") ) );
#endif
	TFullName name;
	fs.FileSystemName( name, KLffsLogicalDriveNumber );
	if( name.Length() > 0 )
		{
		test.Printf( _L("Unmounting drive") );
		test( KErrNone == fs.DismountFileSystem( _L("Lffs"), KLffsLogicalDriveNumber) );
		User::After( 2000000 );
		test.Printf( _L("Drive unmounted") );
		}
	fs.Close();
#endif

	//
	// Open a TBusLogicalDevice to it
	//
	test.Printf( _L("Opening media channel\n") );
	TBool changedFlag = EFalse;
	test( KErrNone == drive.Connect( KDriveNumber, changedFlag ) );
	
	//
	// Get size of Flash drive, block size, block count
	//
    drive.Caps(driveInfo);

	//
	// Create data buffer
	//
	writeBuffer = HBufC8::New( 1024 );
	test( NULL != writeBuffer );
	writeBuffer->Des().FillZ(1024);

	readBuffer = HBufC8::New( 1024 );
	test( NULL != readBuffer );
	readBuffer->Des().FillZ(1024);

	mainThreadHandle = RThread().Handle();
	}



TInt E32Main()
    {

    test.Title();
    test.Start(_L("Benchmarks for media driver"));

	Initialize();

	const TTestData* pTest = &testData[0];
	while( pTest->iFunction )
		{
		if( KEraseSegment == pTest->iFunction )
			{
			test.Printf( *pTest->iDescription );
			TInt r = EraseSegment( pTest->iOffset );
			test( KErrNone == r );
			test.Printf( _L("Segment erased") );
			}
		else
			{
			TestInfo.iLength = pTest->iLength;
			TestInfo.iOffset = pTest->iOffset;
			PrintResult( runTest( pTest->iFunction ), *pTest->iDescription );
			}
		++pTest;
		}

	drive.Disconnect();
    test.End();
	return(KErrNone);
    }

