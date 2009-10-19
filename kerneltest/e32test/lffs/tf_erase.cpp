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
// Tests erasing of Flash. Does not test suspending (see TF_SUSPEND)
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include <e32test.h>
#include "randgen.h"
#include "user_config.h"

RTest test( _L("TF_ERASE") );


class CEraseTest : public CBase
	{
	public:
		~CEraseTest();

		void CreateL();

		void DoTest();

	private:
		void DoSimpleTest();
		void DoSimpleTest2();
		void DoPseudoRandomTest();
		void DoRangeTest();

		TInt EraseOneBlock( TInt aBlockNumber );
		TInt ZeroFillBlock( TInt aBlockNumber );
		TBool ValidateBlock( TInt aBlockNumber, TUint32 aFillWord );


	private:
		TBusLocalDrive	iDrive;
		TBool			iDriveOpened;

		TInt			iFlashSize;
		TInt			iBlockSize;
		TInt			iBlockCount;

		TBuf8<512>		iReadBuffer;

	};


CEraseTest::~CEraseTest()
	{
	if( iDriveOpened )
		{
		iDrive.Disconnect();
		}
	}



void CEraseTest::CreateL()
	{
	//
	// Load the device drivers
	//
#ifndef SKIP_PDD_LOAD
	test.Printf( _L("Loading %S\n"), &KLfsDriverName );
	r = User::LoadPhysicalDevice( KLfsDriverName );
	test( KErrNone == r || KErrAlreadyExists == r );
#endif

#ifdef UNMOUNT_DRIVE
	RFs fs;
	test( KErrNone == fs.Connect() );
#if 0
	// XXX - not eka2
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
	// Open a TBusLogicalDevice to drive
	//
	test.Printf( _L("Opening media channel\n") );
	static TBool changedFlag = EFalse;
	User::LeaveIfError( iDrive.Connect( KDriveNumber, changedFlag ) );
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


void CEraseTest::DoTest()
	//
	// Main test dispatcher
	//
	{
	test.Next( _L("Starting tests...") );
	DoSimpleTest();
	DoSimpleTest2();
	DoPseudoRandomTest();
	DoRangeTest();
	}


TInt CEraseTest::EraseOneBlock( TInt aBlockNumber )
	//
	// Erases block aBlockNumber on Flash
	//
	{
	TInt blockBaseOffset = aBlockNumber * iBlockSize;

	test.Printf( _L("Erasing block %d (offs=0x%x)\n"), aBlockNumber, blockBaseOffset );
	
	TInt r = iDrive.Format( blockBaseOffset, iBlockSize );

	test.Printf( _L("... Format returned %d\n"), r );
	return r;
	}


TBool CEraseTest::ValidateBlock( TInt aBlockNumber, TUint32 aFillWord )
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


TInt CEraseTest::ZeroFillBlock( TInt aBlockNumber )
	//
	// Zero-fills an entire block
	// The requires that writing works (so as a side-effect performs a
	// very basic test of writing)
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
			test.Printf( _L("... FAIL: write failed (%d) at offset 0x%x\n"), r, blockBaseOffset );
			}
		blockBaseOffset += KZeroBufSize;
		}

	return r;
	}


void CEraseTest::DoSimpleTest()
	//
	// Simple erase test. This just zero-fills and then erases each block in turn
	//
	{
	test.Next( _L("Simple test: erases each block in turn\n") );

	for( TInt block = 0; block < iBlockCount; block++ )
		{
		test( KErrNone == ZeroFillBlock( block ) );
		test( ValidateBlock( block, 0 ) );
		test( KErrNone == EraseOneBlock( block ) );
		test( ValidateBlock( block, 0xFFFFFFFF ) );
		}
	}

void CEraseTest::DoSimpleTest2()
	//
	// Another simple erase test.
	// This time we zero-fill all blocks first, then erase them all
	//
	{
	test.Next( _L("Simple test2 : zero-fills whole Flash, then erases all blocks\n") );

	for( TInt block = 0; block < iBlockCount; block++ )
		{
		test( KErrNone == ZeroFillBlock( block ) );
		test( ValidateBlock( block, 0 ) );
		}

	for( TInt block = 0; block < iBlockCount; block++ )
		{
		test( KErrNone == EraseOneBlock( block ) );
		test( ValidateBlock( block, 0xFFFFFFFF ) );
		}
	}



void CEraseTest::DoPseudoRandomTest()
	//
	// Erases the blocks in pseudo-random order, zero-filling first
	//
	{
	test.Next( _L("Test random erase order\n") );

	TRandomGenerator random;
#if 0
	random.SetSeed( TInt64(0x1020466E, 0x3F9C0C00) );
#else
	random.SetSeed( 0x1020466E );
#endif

	for( TInt count = 0; count < 50; count++ )
		{
		TUint block = random.Next() % iBlockCount;
		test( KErrNone == ZeroFillBlock( block ) );

		test( ValidateBlock( block, 0 ) );

		test( KErrNone == EraseOneBlock( block ) );

		test( ValidateBlock( block, 0xFFFFFFFF ) );
		}
	}

void CEraseTest::DoRangeTest()
	//
	// Simple erase test. This just zero-fills and then erases each block in turn
	//
	{
	test.Next( _L("Range test: check that erase only affects erased block\n") );

	//
	// Pre-fill all blocks with zero
	//
	test.Printf( _L("Pre-zeroing blocks\n") );
	for( TInt block = 0; block < iBlockCount; block++ )
		{
		test( KErrNone == ZeroFillBlock( block ) );
		test( ValidateBlock( block, 0 ) );
		}

	//
	// The test is to erase a block. Check it is erased and all
	// other blocks are still zeros. Then we re-zero the block just
	// erased and repeat test with next block
	//
	test.Printf( _L("Now testing erase...\n") );
	for( TInt eraseBlock = 0; eraseBlock < iBlockCount; eraseBlock++ )
		{
		test( KErrNone == EraseOneBlock( eraseBlock ) );
		test( ValidateBlock( eraseBlock, 0xFFFFFFFF ) );
		
		// check all other blocks are still zero
		for( TInt j = 0; j < iBlockCount; j++ )
			{
			if( j != eraseBlock )
				{
				// test if not the one we just erased
				test( ValidateBlock( j, 0 ) );
				}
			}

		// Now zero-fill the block we just erased and move to next block
		test( KErrNone == ZeroFillBlock( eraseBlock ) );
		test( ValidateBlock( eraseBlock, 0 ) );
		}

	}


TInt E32Main()
	{
	test.Title();
	test.Start(_L("Testing media erase operations"));

	CEraseTest eraseTest;
	TRAPD( ret, eraseTest.CreateL() );
	if( KErrNone == ret )
		{
		eraseTest.DoTest();
		}

	test.End();
	return KErrNone;
	}
