// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Tests erasing of Flash while forcing suspend-resume cycles
// This is a soak-test version that runs continously
// This test is similar to TF_SUSPENDSOAK except that it uses
// writes to interrupt the erase instead of reads.
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include <e32test.h>
#include "randgen.h"
#include "user_config.h"

RTest test( _L("TF_SUSPENDSOAKW") );




class CEraser
	{
	public:
		enum TFunction
			{
			EIdle,
			EEraseBlock
			};

	public:
		~CEraser();
		void CreateL();
		void Stop();
		void WaitForReady();
		inline TBool CheckDone() const
			{
			return (EIdle == iRequestedFunction);
			}

		inline void WaitForDone()
			{
			WaitForReady();
			iWaitingSignal.Signal();	// resignal, ready for next Start()
			};

		void EraseBlock( TUint32 aOffset, TUint aLength );

	private:
		void Panic( TInt aPanicNum );
		void Start( TFunction aFunction );

		static TInt EraserThread( TAny* aParam );

		void DoEraseBlock();

	private:
		RThread		iThread;

		//
		// Shared between main & eraser thread
		//
		TFunction	iRequestedFunction;
		RSemaphore	iGoSignal;
		RSemaphore	iWaitingSignal;
		TBool		iStop;

		//
		// These are local to the eraser thread
		//
		TUint		iOffset;
		TUint		iLength;
		TBusLocalDrive	iDrive;
	};



CEraser::~CEraser()
	{
	iThread.Terminate( KErrNone );
	iThread.Close();
	iGoSignal.Close();
	iWaitingSignal.Close();
	}

void CEraser::Panic( TInt aPanicNum )
	{
	_LIT( KPanicCat, "ERASE-T" );
	User::Panic( KPanicCat, aPanicNum );
	RProcess().Panic( KPanicCat, aPanicNum );
	}


void CEraser::CreateL()
	//
	// Create new thread and wait for it to become ready
	//
	{
	iGoSignal.CreateLocal( 0 );	// initially blocked
	iWaitingSignal.CreateLocal( 0 );	// initially blocked
	iStop = EFalse;
	User::LeaveIfError( iThread.Create( _L("ERASER"), EraserThread, 2048, 2048, 65536, this ) );
	test.Printf( _L("Eraser thread created\n") );
	
	iThread.Resume();
	
	test.Printf( _L("Waiting for thread to become ready\n") );
	WaitForReady();
	iWaitingSignal.Signal();
	}

void CEraser::Start( TFunction aFunction )
	//
	// Start the suspender thread executing function aFunction
	//
	{
	iStop = EFalse;
	WaitForReady();
	iRequestedFunction = aFunction;
	iGoSignal.Signal();
	}

void CEraser::Stop()
	//
	// Stop the thread
	//
	{
	iStop = ETrue;
	}

void CEraser::WaitForReady()
	{
	iWaitingSignal.Wait();
	}

void CEraser::EraseBlock( TUint32 aOffset, TUint aLength )
	{
	iOffset = aOffset;
	iLength = aLength;
	Start( EEraseBlock );
	}


TInt CEraser::EraserThread( TAny* aParam )
	//
	// The thread which executes suspend functions
	//
	{
	RDebug::Print( _L("Eraser thread starts") );

	CEraser& self = *reinterpret_cast<CEraser*>(aParam);

	//
	// Open our own TBusLogicalDevice channel
	//
	TBool changedFlag;
	if( KErrNone != self.iDrive.Connect( KDriveNumber, changedFlag ) )
		{
		self.Panic( 1 );
		}

	RDebug::Print( _L("Eraser thread connected to drive") );
	
	while( !self.iStop )
		{
		//
		// Signal that we are ready for a request
		//
		_LIT( KWaitMsg, "Eraser thread waiting..." );
		RDebug::Print( KWaitMsg );
		self.iWaitingSignal.Signal();

		//
		// Wait for a request
		//
		self.iGoSignal.Wait();
		_LIT( KGoMsg, "Eraser thread go (%d)" );
		RDebug::Print( KGoMsg, self.iRequestedFunction );

		switch( self.iRequestedFunction )
			{
			case EEraseBlock:
				self.DoEraseBlock();
				break;

			case EIdle:
			default:
				self.Panic( 0 );
			}

		self.iRequestedFunction = EIdle;
		}

	self.iDrive.Disconnect();
	return KErrNone;
	}

void CEraser::DoEraseBlock()
	//
	// Issue an erase
	//
	{
	_LIT( KEraseStartMsg, "Eraser starting erase..." );
	RDebug::Print( KEraseStartMsg );
	
	TInt r = iDrive.Format( TInt64(iOffset), iLength );
	
	if( KErrNone != r )
		{
		RDebug::Print( _L("Eraser: FAIL: erase request returns %d"), r );
		Panic( 2 );
		}
	}



class CSuspendTest : public CBase
	{
	public:
		~CSuspendTest();

		void CreateL();

		void DoTest();

	private:

		TInt EraseOneBlock( TInt aBlockNumber );
		TInt ZeroFillBlock( TInt aBlockNumber );
		TBool ValidateBlock( TInt aBlockNumber, TUint32 aFillWord );
		TInt ZeroAllBlocks();
		TBool ValidateAllBlocks( TUint32 aFillWord );
		void TimeSinceStart() const;

		void DoRandomWriteSoak();

	private:
		TBusLocalDrive	iDrive;
		TBool			iDriveOpened;

		CEraser*		iEraser;

		TInt			iFlashSize;
		TInt			iBlockSize;
		TInt			iBlockCount;

		TTime			iStartTime;

		TBuf8<512>		iReadBuffer;
	};


CSuspendTest::~CSuspendTest()
	{
	if( iDriveOpened )
		{
		iDrive.Disconnect();
		}

	delete iEraser;
	}



void CSuspendTest::CreateL()
	{
	//
	// Create the eraser thread
	//
	iEraser = new(ELeave) CEraser;
	iEraser->CreateL();

	//
	// Load the device drivers
	//
	TInt r;
#ifndef SKIP_PDD_LOAD
	test.Printf( _L("Loading %S\n"), &KLfsDriverName );
	r = User::LoadPhysicalDevice( KLfsDriverName );
	test( KErrNone == r || KErrAlreadyExists == r );
#endif

#ifdef UNMOUNT_DRIVE
	RFs fs;
	test( KErrNone == fs.Connect() );
	test( KErrNone == fs.SetSessionPath( _L("Z:\\") ) );
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
	r = iDrive.Connect( KDriveNumber, changedFlag );
	User::LeaveIfError( r );
	iDriveOpened = ETrue;

	//
	// Get size of Flash drive, block size, block count
	//
	TLocalDriveCapsV2Buf info;
    iDrive.Caps(info);
	iFlashSize = I64LOW(info().iSize);
	iBlockSize = info().iEraseBlockSize;
	iBlockCount = iFlashSize / iBlockSize;

	test.Printf( _L("Flash size is 0x%x bytes\n"), iFlashSize );
	test.Printf( _L("Block size is 0x%x bytes\n"), iBlockSize );
	test.Printf( _L("Block count is %d\n"), iBlockCount );

	test.Printf( _L("CreateL complete\n") );
	}


void CSuspendTest::DoTest()
	//
	// Main test dispatcher
	//
	{
	DoRandomWriteSoak();
	}


TInt CSuspendTest::EraseOneBlock( TInt aBlockNumber )
	//
	// Erases block aBlockNumber on Flash
	//
	{
	TInt blockBaseOffset = aBlockNumber * iBlockSize;

	test.Printf( _L("Erasing block %d (offs=0x%x)\n"), aBlockNumber, blockBaseOffset );
	
	TInt r = iDrive.Format( blockBaseOffset, iBlockSize );

	test.Printf( _L("... block erased, rv=%d\n"), r );
	return r;
	}


TBool CSuspendTest::ValidateBlock( TInt aBlockNumber, TUint32 aFillWord )
	//
	// Checks that every word in block aBlockNumber has the value aFillWord
	//
	{
	TUint offset = aBlockNumber * iBlockSize;
	test.Printf( _L("Validating block %d (offs=0x%x)\n"), aBlockNumber, offset );
	
	TBool failed = EFalse;
	const TInt readBufLen = iReadBuffer.MaxLength();

	for( TInt len = iBlockSize; len > 0 && !failed ;)
		{
		TInt r = iDrive.Read( offset, readBufLen, iReadBuffer );
		if( r != KErrNone )
			{
			test.Printf( _L("... FAIL: read failed (%d) at offset 0x%x\n"), r, offset );
			test( KErrNone == r );
			}
		test( iReadBuffer.Length() == readBufLen );

		TUint32* p = (TUint32*)iReadBuffer.Ptr();
		for( TInt i = 0; i < readBufLen; i += 4 )
			{
			if( aFillWord != *p )
				{
				failed = ETrue;
				test.Printf( _L("... FAILED: word @ offs=0x%x, read=0x%x, expected=0x%x\n"), 
								offset+i, p[0], aFillWord );
				break;
				}
			++p;
			}
		offset += readBufLen;
		len -= readBufLen;
		}
	
	return !failed;
	}


TInt CSuspendTest::ZeroFillBlock( TInt aBlockNumber )
	//
	// Zero-fills and entire block
	// The requires that writing works
	//
	{
	test.Printf( _L("Zero-filling block %d\n"), aBlockNumber );

	//
	// Create a buffer full of zeros
	//
	const TInt KZeroBufSize = 512;

	TBuf8<KZeroBufSize> buf;
	buf.FillZ( buf.MaxLength() );

	//
	// Write the data out to the Flash
	//
	TInt writeCount = iBlockSize / KZeroBufSize;
	TInt r = KErrNone;
	TUint blockBaseOffset = aBlockNumber * iBlockSize;
	TInt pos = blockBaseOffset;
	for( ; (writeCount > 0) && (KErrNone == r); writeCount-- )
		{
		r = iDrive.Write( pos, buf );
		if( r != KErrNone )
			{
			test.Printf( _L("... FAIL: write failed (%d) at offset 0x%x\n"), pos );
			}
		pos += KZeroBufSize;
		}

	return r;
	}


TInt CSuspendTest::ZeroAllBlocks()
	//
	// Writes zeros to all blocks
	//
	{
	test.Printf( _L("Zeroing all blocks\n") );

	TInt r = KErrNone;
	for( TInt i = 0; (i < iBlockCount) && (KErrNone == r); i++ )
		{
		r = ZeroFillBlock( i );
		}

	return r;
	}

TBool CSuspendTest::ValidateAllBlocks( TUint32 aFillWord )
	//
	// Checks that all blocks contain aFillWord
	//
	{
	test.Printf( _L("Validating all blocks\n") );

	TBool failed = EFalse;
	for( TInt i = 0; (i < iBlockCount) && (!failed); i++ )
		{
		failed = !ValidateBlock( i, aFillWord );
		}

	return !failed;
	}


void CSuspendTest::TimeSinceStart() const
	{
	TTimeIntervalSeconds timeTaken;
	TTime time;
	time.HomeTime();
	TInt r = time.SecondsFrom(iStartTime, timeTaken);
	test(r == KErrNone);
	TInt totalTime = timeTaken.Int();

	TInt seconds = totalTime % 60;
	TInt minutes = (totalTime / 60) % 60;
	TInt hours   = totalTime / 3600;

	test.Printf( _L("Time since test started = %d:%d:%d\n"), hours, minutes, seconds );
	}



void CSuspendTest::DoRandomWriteSoak()
	//
	// For each block issues an erase and then
	// starts issuing write requests. The intervals
	// between write requests are derived from the
	// pseudo-random number generator.
	// Each block is checked after is has been erased
	//
	// The same data is written each time. This data is
	// also generated from the random number generator. After
	// each erase the data is read back to check that it is 
	// correct.
	//
	{
	test.Next( _L("Erase suspend soak test using random writes") );

	TRandomGenerator random;
	random.SetSeed( 0x13E00103 );
	
	test.Printf( _L("Preparing buffer") );
	TBuf8<20>	writeBuf;
	writeBuf.SetLength( writeBuf.MaxLength() );
	for( TInt i = writeBuf.Length(); i > 0;)
		{
		--i;
		writeBuf[i] = static_cast<TUint8>( random.Next() );
		}
	
	test.Printf( _L("Starting test") );
	random.SetSeed( MAKE_TINT64( 0xA05BE111,0x00101111 ) );
	iStartTime.HomeTime();

	//
	// We repeat the test for each block, erasing block n and reading from
	// block (n+1) modulo iBlockCount
	//
	
	//
	// Writes are always done to the block that we just erased. This
	// TBool prevents us starting writes until we have erased a block.
	//
	TBool firstErase = ETrue;
	for(;;)
		{
		TimeSinceStart();

		for( TInt eraseBlock = 0; eraseBlock < iBlockCount; eraseBlock++ )
			{
			TUint32 writeBlock = eraseBlock - 1;
			if( 0 == eraseBlock )
				{
				writeBlock = iBlockCount - 1;
				}
			
			TUint32 erasePos = eraseBlock * iBlockSize;
			TInt writePos = writeBlock * iBlockSize;

			test.Printf( _L("Erasing block %d, writing to block %d"),
						eraseBlock, writeBlock );

			//
			// Zero the block we are about to erase
			//
			test( KErrNone == ZeroFillBlock( eraseBlock ) );
			test( ValidateBlock( eraseBlock, 0 ) );
			
			//
			// Start the erase
			//
			_LIT( KEraseNotify, "Main thread starting erase\n" );
			test.Printf( KEraseNotify );
			iEraser->EraseBlock( erasePos, iBlockSize );

			//
			// Now we loop, waiting for random intervals, issuing
			// writes, until the erase completes
			//

			TBool didWrite = EFalse;
			while( !iEraser->CheckDone() )
				{
				//
				// Get a pseudo-random interval between 0 and 524.288 milliseconds
				//
				TInt delayInMicroseconds = random.Next() % 0x80000;
				User::After( delayInMicroseconds );

				if( !firstErase )
					{
					test( KErrNone == iDrive.Write( writePos, writeBuf ) );
					_LIT( KWriteNotify, "Done write" );
					test.Printf( KWriteNotify );
					didWrite = ETrue;
					}
				}

			//
			// Now check that the block was erased
			//
			test( ValidateBlock( eraseBlock, 0xFFFFFFFF ) );
			firstErase = EFalse;

			//
			// Also check that the data written to the Flash is correct.
			//
			if( didWrite )
				{
				TBuf8<20>	readBuf;
				test( KErrNone == iDrive.Read( writePos, writeBuf.Length(), readBuf ) );
				test( readBuf == writeBuf );
				test.Printf( _L("Write data is ok") );
				}
			}
		}
	}




void E32Main()
	{
	test.Title();
	test.Start(_L("Testing media erase+suspend operations"));

	CSuspendTest suspendTest;
	TRAPD( ret, suspendTest.CreateL() );
	if( KErrNone == ret )
		{
		suspendTest.DoTest();
		}

	test.End();
	}

