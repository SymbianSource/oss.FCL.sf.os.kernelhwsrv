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
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include <e32test.h>
#include "user_config.h"

RTest test( _L("TF_SUSPEND") );



class CEraser
	{
	public:
		enum TFunction
			{
			EEraseBlock
			};

	public:
		~CEraser();
		void CreateL();
		void Stop();
		void WaitForReady();

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
	//
	// Execute a block erase
	//
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
		RDebug::Print( _L("Eraser thread waiting...") );
		self.iWaitingSignal.Signal();

		//
		// Wait for a request
		//
		self.iGoSignal.Wait();
		RDebug::Print( _L("Eraser thread go (%d)"), self.iRequestedFunction );

		switch( self.iRequestedFunction )
			{
			case EEraseBlock:
				self.DoEraseBlock();
				break;

			default:
				self.Panic( 0 );
			}

		}

	self.iDrive.Disconnect();
	return KErrNone;
	}

void CEraser::DoEraseBlock()
	//
	// Issue an erase
	//
	{
	RDebug::Print( _L("Eraser starting erase...") );
	
	TInt64 offs( iOffset );
	TInt r = iDrive.Format( offs, iLength );
	
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

		void DoImmediateSuspendTest();

	private:
		TBusLocalDrive	iDrive;
		TBool			iDriveOpened;

		CEraser*		iEraser;

		TInt			iFlashSize;
		TInt			iBlockSize;
		TInt			iBlockCount;

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
	// Open a TBusLogicalDevice to the driver
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
	DoImmediateSuspendTest();
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
	TInt blockBaseOffset = aBlockNumber * iBlockSize;
	for( ; (writeCount > 0) && (KErrNone == r); writeCount-- )
		{
		r = iDrive.Write( blockBaseOffset, buf );
		if( r != KErrNone )
			{
			test.Printf( _L("... FAIL: write failed (%d) at offset 0x%x\n"), blockBaseOffset );
			}
		blockBaseOffset += KZeroBufSize;
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


void CSuspendTest::DoImmediateSuspendTest()
	//
	// For each block issues an erase and then immediately
	// requests a read on another block. Waits for erase to
	// finish and validates it
	//
	{
	test.Next( _L("Immediate suspend test") );
	test( KErrNone == ZeroAllBlocks() );
	test( ValidateAllBlocks( 0 ) );

	//
	// We repeat the test for each block, erasing block n and reading from
	// block (n+1) modulo iBlockCount
	//
	for( TInt eraseBlock = 0; eraseBlock < iBlockCount; eraseBlock++ )
		{
		TUint32 readBlock = (eraseBlock + 1) % iBlockCount;
		TUint32 erasePos = eraseBlock * iBlockSize;
		TInt readPos = readBlock * iBlockSize;

		TBuf8<32> buf;

		//
		// Start the erase
		//
		iEraser->EraseBlock( erasePos, iBlockSize );

		//
		// Do a read immediately
		//
		test.Printf( _L("main thread requesting read") );
		test( KErrNone == iDrive.Read( readPos, buf.MaxLength(), buf ) );

		//
		// Wait for erase to finish
		//
		test.Printf( _L("main thread waiting for erase to finish...") );
		iEraser->WaitForDone();

		//
		// Now check that the block was erased
		//
		test( ValidateBlock( eraseBlock, 0xFFFFFFFF ) );
		
		}

	}




TInt E32Main()
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
	return KErrNone;
	}
