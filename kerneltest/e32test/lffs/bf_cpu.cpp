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
// Very simple test of CPU overhead
// 
//

/**
 @file bf_cpu.cpp
*/

#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32svr.h>
#include "user_config.h"


#define TEST_WRITE_OVERHEAD
#define NON_WRITING_LOOPS

const TInt KAverageOverInSeconds=10;	///< Number of seconds to run tests for

TInt64 Count;				///< Global variable used to count number of operations completed
RSemaphore CountSem;		///< control access to Count;


RTest test(_L("BF_CPU"));



TBusLocalDrive	drive;
TLocalDriveCapsV2Buf driveInfo;

LOCAL_D TBool StopTest;		///< set to ETrue to stop the test


#ifdef TEST_WRITE_OVERHEAD
LOCAL_D TBool StopZeroTest;

LOCAL_C TInt WriteZeroThread(TAny*)
	/**
	 * Performs writes of zero length continuously
	 */
	{
#if 0
	_LIT( KPanicCat, "ZERWRTH" );
#endif
	
	TBuf8<513> buf;
	buf.SetLength(513);
	
	while( !StopZeroTest )
		{
		// Return values are bogus when doing overhead testing
		drive.Write( 513, buf );
		}
	return KErrNone;
	}
#endif


LOCAL_C TInt WriteThread(TAny*)
	/**
	 * Performs writes continuously
	 */
	{
	_LIT( KPanicCat, "WRTHRD" );
	
	TBuf8<512> buf;
	buf.SetLength(512);
	buf.Fill(0xFF);		// all 0xFF so we can repeatedly overwrite
	
	while( !StopTest )
		{
		TInt r = drive.Write( 0, buf );
		if( KErrNone != r )
			{
			User::Panic( KPanicCat, r );
			}
		}
	return KErrNone;
	}



LOCAL_C TInt CpuThread(TAny*)
	/**
	 * Just increments the counter
	 */
	{
	while( !StopTest )
		{
		CountSem.Wait();
#ifdef NON_WRITING_LOOPS
		for( volatile TInt i = 5000; i > 0; i-- );
#endif
		++Count;
		CountSem.Signal();
		}
	return KErrNone;
	}


void runTest()
	{
    RThread writeThread;
	TInt r=writeThread.Create(_L("WRITER"),WriteThread,KDefaultStackSize,&User::Heap(),NULL);
	test(r==KErrNone);

	RThread cpuThread;
	r=cpuThread.Create(_L("CPU-ER"),CpuThread,KDefaultStackSize,&User::Heap(),NULL);
	test(r==KErrNone);

#ifdef TEST_WRITE_OVERHEAD
    RThread writeZeroThread;
	r=writeZeroThread.Create(_L("WRITERZERO"),WriteZeroThread,KDefaultStackSize,&User::Heap(),NULL);
	test(r==KErrNone);
#endif

	r = CountSem.CreateLocal(1);
	test(r==KErrNone);


	StopTest = EFalse;	// allow the test to run

    TRequestStatus deadStatWrite;
    TRequestStatus deadStatCpu;
	writeThread.Logon( deadStatWrite );
	cpuThread.Logon( deadStatCpu );
	
	// make writer thread have priority over CPU usage thread
	writeThread.SetPriority( EPriorityMore );

	// make this thread highest priority
	RThread().SetPriority( EPriorityMuchMore );
	
	
    cpuThread.Resume();
	
#ifdef TEST_WRITE_OVERHEAD
    TRequestStatus deadStatWriteZero;
	writeZeroThread.Logon( deadStatWriteZero );
	// make writer thread have priority over CPU usage thread
	writeZeroThread.SetPriority( EPriorityMore );
	StopZeroTest = EFalse;
	writeZeroThread.Resume();
#endif
	
	// wait for thread to initialise
	User::After(1000000);

	CountSem.Wait();
	Count=0;
	CountSem.Signal();

    User::After(KAverageOverInSeconds*1000000);
    
	CountSem.Wait();
	TInt64 noWriteCount( Count );	// number of counts when not writing
	CountSem.Signal();

	
#ifdef TEST_WRITE_OVERHEAD
	// kill the zero writer
	StopZeroTest = ETrue;
	User::WaitForRequest( deadStatWriteZero );
	CLOSE_AND_WAIT(writeZeroThread);
#endif
    
	test.Printf( _L("Loops without writing = %ld"), noWriteCount );
	
	// start write thread
	writeThread.Resume();
	User::After(1000000);
    
	CountSem.Wait();
	Count=0;
	CountSem.Signal();

    User::After(KAverageOverInSeconds*1000000);
    
	CountSem.Wait();
	TInt64 withWriteCount( Count );	// number of counts when writing
	CountSem.Signal();

	test.Printf( _L("Loops while writing = %ld"), withWriteCount );
	
	// tell test to stop and wait for thread to exit.
	cpuThread.Kill(KErrNone);
	StopTest = ETrue;
	User::WaitForRequest( deadStatWrite );
	
	CLOSE_AND_WAIT(writeThread);
	CLOSE_AND_WAIT(cpuThread);
    


	TInt64 calc( withWriteCount );
	calc = calc * 100;
	calc = calc / noWriteCount;
		
	test.Printf( _L("%% CPU used = %d"), 100 - I64LOW(calc) );
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
	test.Printf( _L("erase returns %d"), r );
	return  r;
	}




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
	// XXX not EKA2
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
	}



TInt E32Main()
    {

    test.Title();
    test.Start(_L("Testing CPU overhead"));

	Initialize();

	test.Printf( _L("Erasing first segment") );
	TInt r = EraseSegment( 0 );
	test( KErrNone == r );
	test.Printf( _L("Segment erased") );

	runTest();

	drive.Disconnect();
    test.End();
	return(KErrNone);
    }

