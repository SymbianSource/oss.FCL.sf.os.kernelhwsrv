// Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include <e32test.h>
#include "randgen.h"
#include "user_config.h"

RTest test( _L("TF_READ") );


const TInt KTestUserDataSize = 1024;
const TInt KBufferGuardSize = 16384;

const TInt KMaxWriteLength = 512;

const TInt64 KSampleDataRandomSeed = MAKE_TINT64(0x3e000111,0xAFCBDF0F);
const TInt64 KRandomTestSeed = MAKE_TINT64(0x90009901,0xABEF1011);

enum TPanicNo
	{
	EPanicGetDesOverflow,
	EPanicGetDesInitialOverflow,
	EPanicCheckOverflow
	};

LOCAL_D void Panic( TPanicNo aPanic )
	{
	_LIT( KPanicCat, "TF_READ" );
	User::Panic( KPanicCat, aPanic );
	}


class CCheckedBuffer : public CBase
	{
	public:
		CCheckedBuffer( TInt auserDataSize, TInt aGuardSize );
		~CCheckedBuffer();

		void CreateL();
		void InitialiseGuard();
		TBool CheckGuard( TInt aUserDataLength ) const;
		TBool CheckGuardAtStartOfUserData( TInt aGuardLength ) const;
		void GetDes( TPtrC8& aDes ) const;
		void GetDes( TPtr8& aDes, TInt aInitialLength, TInt aMaxLength ) const;
		

	private:
		CCheckedBuffer();

	private:
		TPtr8	iUserData;		// pointer to user data area
		const TInt	iUserDataSize;
		const TInt	iGuardSize;
		TUint8*	iAllocCell;
	};



CCheckedBuffer::CCheckedBuffer( TInt aUserDataSize, TInt aGuardSize )
	: iUserData(0,0), iUserDataSize( aUserDataSize ), iGuardSize( aGuardSize )
	{
	}

CCheckedBuffer::~CCheckedBuffer()
	{
	delete iAllocCell;
	}

void CCheckedBuffer::CreateL()
	{
	TInt totalCellSizeRequired = iUserDataSize + (2 * iGuardSize);

	iAllocCell = (TUint8*)User::AllocL( totalCellSizeRequired );

	test.Printf( _L("Allocated heap cell for checked buffer\n") );

	iUserData.Set( iAllocCell + iGuardSize, iUserDataSize, iUserDataSize );
	}

void CCheckedBuffer::GetDes( TPtrC8& aDes ) const
	//
	// Create descriptor to the whole user data area in aDes
	//
	{
	aDes.Set( iAllocCell + iGuardSize, iUserDataSize );
	}

void CCheckedBuffer::GetDes( TPtr8& aDes, TInt aInitialLength, TInt aMaxLength ) const
	//
	// Create modifiable descriptor to the user data area in aDes,
	// with a maximum length aMaxLength, and initial length aInitialLength
	//
	{
	__ASSERT_ALWAYS( aMaxLength <= iUserDataSize, Panic(EPanicGetDesOverflow) );
	__ASSERT_ALWAYS( aInitialLength <= iUserDataSize, Panic(EPanicGetDesInitialOverflow) );
	aDes.Set( iAllocCell + iGuardSize, aInitialLength, aMaxLength );
	}


void CCheckedBuffer::InitialiseGuard()
	//
	// Create the guard regions
	//
	{
	TInt totalCellSize = User::AllocLen( iAllocCell );
	Mem::Fill( iAllocCell, totalCellSize, 0x5A );
	}

TBool CCheckedBuffer::CheckGuard( TInt aUserDataLength ) const
	//
	// Checks that the guard value is still present before the user data
	// area, and after aUserDataLength bytes of user data
	//
	{
	const TUint8* p = iAllocCell;
	const TUint8* pUserDataStart = iUserData.Ptr();

	for( ; p < pUserDataStart; p++ )
		{
		if( 0x5a != *p )
			{
			return EFalse;
			}
		}

	p = pUserDataStart + aUserDataLength;
	const TUint8* pEnd = iAllocCell + User::AllocLen( iAllocCell );

	for( ; p < pEnd; p++ )
		{
		if( 0x5a != *p )
			{
			return EFalse;
			}
		}
	
	return ETrue;
	}


TBool CCheckedBuffer::CheckGuardAtStartOfUserData( TInt aGuardLength ) const
	//
	// Checks that the first aGuardLength bytes of the user data area
	// contain the guard value
	//
	{
	const TUint8* p = iUserData.Ptr();
	const TUint8* pEnd = p + aGuardLength;

	for( ; p < pEnd; p++ )
		{
		if( 0x5a != *p )
			{
			return EFalse;
			}
		}
	
	return ETrue;
	}



class CReadTest : public CBase
	{
	public:
		~CReadTest();

		void CreateL();

		void DoTest();

	private:
		static TInt DummyThread( TAny* aParam );

		void CreateSampleData();
		static TBool CheckZero( const TPtrC8& aDes );
		void CreateTestData( TInt aBlockNumber, TBool aEndOfBlock );
		TBool CompareAgainstFlash( TInt aFlashOffset, const TPtrC8& aDes, TInt aDescOffset );

		void TestSimpleReads();
		void TestSimpleThreadReads();
		void TestUnalignedReads();
		void TestUnalignedThreadReads();
		void TestOffsetBufferThreadReads();
		void TestOffsetBufferUnalignedThreadReads();
		void TestReadsFromAllBlocks();
		void TestSimpleScatterReads1();
		void TestSimpleScatterReads2();
		void TestScatterGather();
		void TestReadAcrossBlock();

		void PerformCheckedRead( TInt aReadPos, TInt aReadLen );
		void PerformCheckedThreadRead( TInt aReadPos, TInt aReadLen, TInt aDescOffset );

	private:
		TInt			iFlashSize;
		TInt			iBlockSize;
		TInt			iBlockCount;

		TBusLocalDrive	iDrive;
		TBool			iDriveOpened;
		TBuf8<512>		iReadBuffer;

		TRandomGenerator	iRandom;

		TBuf8<KTestUserDataSize> iSampleData;

		CCheckedBuffer*	iBuffer;

		RThread			iDummyThread;
	};

CReadTest::~CReadTest()
	{
	if( iDriveOpened )
		{
		iDrive.Disconnect();
		}
	}



void CReadTest::CreateL()
	{
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
#if 0 // XXX - API violation on EKA2
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
	r = iDrive.Connect( KDriveNumber, changedFlag );
	User::LeaveIfError( r );
	iDriveOpened = ETrue;

	//
	// Get size of Flash drive
	//
	TLocalDriveCapsV2Buf info;
    iDrive.Caps(info);
	iFlashSize = I64LOW(info().iSize);
	iBlockSize = info().iEraseBlockSize;
	iBlockCount = iFlashSize / iBlockSize;

	test.Printf( _L("Flash size is 0x%x bytes\n"), iFlashSize );

	//
	// Create a dummy thread that we can use to force
	// other-thread write operations
	//
#if 0
	test( KErrNone == iDummyThread.Create( _L("DUMMY"), DummyThread, 256, KMinHeapSize, KMinHeapSize, NULL ) );
#else
	// XXX TONYL
	test( KErrNone == iDummyThread.Create( _L("DUMMY"), DummyThread, KDefaultStackSize, KMinHeapSize, KMinHeapSize, NULL ) );

//	test.Printf( _L("== do it"));
//	TInt pas = iDummyThread.Create( _L("DUMMY"), DummyThread, KDefaultStackSize, KMinHeapSize, KMinHeapSize, NULL );
//	test.Printf( _L("CREATE = %d"), pas);
//	test (pas == KErrNone);
#endif
#if 1
	iDummyThread.Resume();
#endif

	//
	// Create a checked buffer
	//
	iBuffer = new(ELeave) CCheckedBuffer( KTestUserDataSize, KBufferGuardSize );
	iBuffer->CreateL();

	//
	// Seed the pseudo-random number generator
	//
	iRandom.SetSeed( KSampleDataRandomSeed );

	test.Printf( _L("CreateL complete\n") );
	}



TInt CReadTest::DummyThread( TAny* /* aParam */ )
	//
	// Thread does nothing at all
	//
	{
#if 1
	test.Printf( _L("== do it"));
#endif
	for(;;)
		{
		User::WaitForAnyRequest();	// just block
		}
	}


void CReadTest::TestSimpleReads()
	//
	// Makes reads of 1 byte to 512 bytes into the start of the
	// checked buffer and tests that only the expected bytes have changed
	// This uses the simple read function from TBusLocalDrive, and 
	// reads from an aligned Flash address
	//
	{
	test.Next( _L("Testing simple reads\n") );

	//
	// Descriptor to user data area, passed to media driver
	//
	TPtr8 des(0,0);

	for( TInt readLen = 1; readLen <= 512; readLen++ )
		{
		test.Printf( _L("Reading %d bytes\n"), readLen );
		
		//
		// Prepare the guard data
		//
		iBuffer->InitialiseGuard();
	
		//
		// Set up the descriptor, length=0, maxlen=readLen
		//
		iBuffer->GetDes( des, 0, readLen );

		//
		// Now read some data into it
		//
		test( KErrNone == iDrive.Read( 0, readLen, des ) );

		//
		// Check what we got
		//
		test( des.Length() == readLen );
		
		TPtrC8 newDes;

	iBuffer->GetDes( newDes );

		test( newDes.Ptr() == des.Ptr() );

		test( iBuffer->CheckGuard( readLen ) );

		test( CompareAgainstFlash( 0, des, 0 ) );

		}
	}

void CReadTest::TestSimpleThreadReads()
	//
	// Makes reads of 1 byte to 512 bytes into the start of the
	// checked buffer and tests that only the expected bytes have changed
	// This uses the more complex read function from TBusLocalDrive, and 
	// reads from an aligned Flash address
	//
	{
	test.Next( _L("Testing simple reads using other-thread read function\n") );

	//
	// Descriptor to user data area, passed to media driver
	//
	TPtr8 des(0,0);

	for( TInt readLen = 1; readLen <= 512; readLen++ )
		{
		test.Printf( _L("Reading %d bytes\n"), readLen );
		
		//
		// Prepare the guard data
		//
		iBuffer->InitialiseGuard();
		test.Printf( _L("AA\n"));
		
		//
		// Set up the descriptor, length=0, maxlen=readLen
		//
		iBuffer->GetDes( des, 0, readLen );
		test.Printf( _L("BB\n"));

		//
		// Now read some data into it
		//
		test( KErrNone == iDrive.Read( 0, readLen, &des, KLocalMessageHandle, 0 ) );
		test.Printf( _L("CC\n"));
#if 0
		test( KErrNone == iDrive.Read( 0, readLen, &des, iDummyThread.Handle(), 0 ) );
#else
		// XXX - this works
		test( KErrNone == iDrive.Read( 0, readLen, &des, KLocalMessageHandle, 0 ) );
#endif

		//
		// Check what we got
		//
		test.Printf( _L("DD\n"));
		test.Printf( _L("DD\n"));
		test.Printf( _L("DD\n"));
		test.Printf( _L("DD\n"));
		test( des.Length() == readLen );
		
		TPtrC8 newDes;
		test.Printf( _L("EE\n"));
		iBuffer->GetDes( newDes );
		test.Printf( _L("FF\n"));
		test( newDes.Ptr() == des.Ptr() );

		test( iBuffer->CheckGuard( readLen ) );

		test.Printf( _L("GG\n"));
		test( CompareAgainstFlash( 0, des, 0 ) );
		test.Printf( _L("HH\n"));

		}
	}


void CReadTest::TestUnalignedReads()
	//
	// Makes reads of 1 byte to 512 bytes into the start of the
	// checked buffer and tests that only the expected bytes have changed
	// This uses the simple read function from TBusLocalDrive.
	// The data is read from an unaligned address (0ffset 1, 2, 3)
	//
	{
	test.Next( _L("Testing unaligned reads\n") );

	//
	// Descriptor to user data area, passed to media driver
	//
	TPtr8 des(0,0);

	for( TInt readLen = 1; readLen <= 512; readLen++ )
		{
		//
		// Set up the descriptor, length=0, maxlen=readLen
		//
		iBuffer->GetDes( des, 0, readLen );

		//
		// Repeat for each offset
		//
		for( TInt offs = 1; offs < 4; offs++ )
			{
			test.Printf( _L("Reading %d unaligned bytes from offset %d\n"), readLen, offs );

			iBuffer->InitialiseGuard();
			test( KErrNone == iDrive.Read( offs, readLen, des ) );

			test( des.Length() == readLen );
			
			TPtrC8 newDes;
			iBuffer->GetDes( newDes );
			test( newDes.Ptr() == des.Ptr() );

			test( iBuffer->CheckGuard( readLen ) );

			test( CompareAgainstFlash( offs, des, 0 ) );
			}

		}
	}


void CReadTest::TestUnalignedThreadReads()
	//
	// Makes reads of 1 byte to 512 bytes into the start of the
	// checked buffer and tests that only the expected bytes have changed
	// This uses the thread read function from TBusLocalDrive.
	// The data is read from an unaligned address (0ffset 1, 2, 3)
	//
	{
	test.Next( _L("Testing unaligned other-thread reads\n") );

	//
	// Descriptor to user data area, passed to media driver
	//
	TPtr8 des(0,0);

	for( TInt readLen = 1; readLen <= 512; readLen++ )
		{
		//
		// Set up the descriptor, length=0, maxlen=readLen
		//
		iBuffer->GetDes( des, 0, readLen );

		//
		// Repeat for each offset
		//
		for( TInt offs = 1; offs < 4; offs++ )
			{
			test.Printf( _L("Reading %d unaligned bytes from offset %d\n"), readLen, offs );

			iBuffer->InitialiseGuard();
#if 0
			test( KErrNone == iDrive.Read( offs, readLen, &des, iDummyThread.Handle(), 0 ) );
#else
			test( KErrNone == iDrive.Read( offs, readLen, &des, KLocalMessageHandle, 0 ) );
#endif

			test( des.Length() == readLen );
			
			TPtrC8 newDes;
			iBuffer->GetDes( newDes );
			test( newDes.Ptr() == des.Ptr() );

			test( iBuffer->CheckGuard( readLen ) );

			test( CompareAgainstFlash( offs, des, 0 ) );
			}

		}
	}


void CReadTest::TestOffsetBufferThreadReads()
	//
	// Makes reads of 1 byte to 512 bytes to an offset position in the
	// checked buffer and tests that only the expected bytes have changed
	// This uses the more complex read function from TBusLocalDrive, and 
	// reads from an aligned Flash address
	//
	{
	test.Next( _L("Testing other-thread reads into offset position in descriptor\n") );

	//
	// Descriptor to user data area, passed to media driver
	//
	TPtr8 des(0,0);

	for( TInt readLen = 1; readLen <= 512; readLen++ )
		{
		test.Printf( _L("Reading %d bytes\n"), readLen );
		

		//
		// Repeat test for offsets 0..64 in buffer
		//
		for( TInt destOffset = 1; destOffset < 64; destOffset++ )
			{
//			test.Printf( _L("... dest offset = %d"), destOffset );

			//
			// Prepare the guard data
			//
			iBuffer->InitialiseGuard();
			
			//
			// Set up the descriptor, length=0, maxlen=readLen+destOffset
			//
			iBuffer->GetDes( des, 0, readLen + destOffset );

#if 0
			test( KErrNone == iDrive.Read( 0, readLen, &des, iDummyThread.Handle(), destOffset ) );
#else
			test( KErrNone == iDrive.Read( 0, readLen, &des, KLocalMessageHandle, destOffset ) );
#endif

			//
			// Check what we got
			//
			test( des.Length() == readLen + destOffset );
			
			TPtrC8 newDes;
			iBuffer->GetDes( newDes );
			test( newDes.Ptr() == des.Ptr() );

			//
			// end of written data is at readLen + destOffset
			//
			test( iBuffer->CheckGuard( readLen+destOffset ) );
			//
			// check the section between that start of the user data and
			// the offset position still contains guard data
			//
			test( iBuffer->CheckGuardAtStartOfUserData( destOffset ) );

			test( CompareAgainstFlash( 0, des, destOffset ) );
			}

		}
	}


void CReadTest::TestOffsetBufferUnalignedThreadReads()
	//
	// Makes reads of 1 byte to 512 bytes to an offset position in the
	// checked buffer and tests that only the expected bytes have changed
	// This uses the more complex read function from TBusLocalDrive, and 
	// reads from an aligned Flash address
	//
	{
	test.Next( _L("Testing other-thread unaligned reads into offset position in descriptor\n") );

	//
	// Descriptor to user data area, passed to media driver
	//
	TPtr8 des(0,0);

	for( TInt readLen = 1; readLen <= 500; readLen++ )
		{
		test.Printf( _L("Reading %d bytes\n"), readLen );
		

		//
		// Repeat test for offsets 0..64 in buffer
		//
		for( TInt destOffset = 1; destOffset < 64; destOffset++ )
			{
//			test.Printf( _L("... dest offset = %d"), destOffset );

			//
			// repeat for each source offset
			//
			for( TInt offs = 1; offs < 4; offs++ )
				{
				//
				// Prepare the guard data
				//
				iBuffer->InitialiseGuard();
				
				//
				// Set up the descriptor, length=0, maxlen=readLen+destOffset
				//
				iBuffer->GetDes( des, 0, readLen + destOffset );

#if 0
				test( KErrNone == iDrive.Read( offs, readLen, &des, iDummyThread.Handle(), destOffset ) );
#else
				test( KErrNone == iDrive.Read( offs, readLen, &des, KLocalMessageHandle, destOffset ) );
#endif


				//
				// Check what we got
				//
				test( des.Length() == readLen + destOffset );
				
				TPtrC8 newDes;
				iBuffer->GetDes( newDes );
				test( newDes.Ptr() == des.Ptr() );

				//
				// end of written data is at readLen + destOffset
				//
				test( iBuffer->CheckGuard( readLen+destOffset ) );
				//
				// check the section between that start of the user data and
				// the offset position still contains guard data
				//
				test( iBuffer->CheckGuardAtStartOfUserData( destOffset ) );

				test( CompareAgainstFlash( offs, des, destOffset ) );
				} // end for
			}
		}
	}


void CReadTest::PerformCheckedRead( TInt aReadPos, TInt aReadLen )
	{
	TPtr8 des(0,0);
	iBuffer->InitialiseGuard();
	iBuffer->GetDes( des, 0, aReadLen );

	test.Printf( _L("Reading %d byte(s) from offset 0x%x\n"), aReadLen, aReadPos );
	test( KErrNone == iDrive.Read( aReadPos, aReadLen, des ) );
	test( des.Length() == aReadLen );
	test( iBuffer->CheckGuard( aReadLen ) );
	test( CompareAgainstFlash( aReadPos, des, 0 ) );
	}

void CReadTest::PerformCheckedThreadRead( TInt aReadPos, TInt aReadLen, TInt aDescOffset )
	{
	TPtr8 des(0,0);
	iBuffer->InitialiseGuard();
	iBuffer->GetDes( des, 0, aReadLen + aDescOffset );

	test.Printf( _L("Reading %d byte(s) from offset 0x%x to thread descriptor offset %d\n"), aReadLen, aReadPos, aDescOffset );
#if 0
	test( KErrNone == iDrive.Read( aReadPos, aReadLen, &des, iDummyThread.Handle(), aDescOffset ) );
#else
	test( KErrNone == iDrive.Read( aReadPos, aReadLen, &des, KLocalMessageHandle, aDescOffset ) );
#endif

//	test.Printf( _L("Check descriptor length") );
	test( des.Length() == aReadLen + aDescOffset );
//	test.Printf( _L("Check guard") );
	test( iBuffer->CheckGuard( aReadLen + aDescOffset ) );
//	test.Printf( _L("Check guard at start of descriptor") );
	test( iBuffer->CheckGuardAtStartOfUserData( aDescOffset ) );
	test( CompareAgainstFlash( aReadPos, des, aDescOffset ) );
	}


void CReadTest::TestReadsFromAllBlocks()
	//
	// Does some spot-test reads from all blocks to make sure
	// that reading across the whole Flash works
	//
	{
	test.Next( _L("Testing reads from all blocks\n") );

	for( TInt block = 0; block < iBlockCount; block++ )
		{
		test.Printf( _L("Reading from block %d"), block );
		TInt readBase = (block * iBlockSize);
		
		PerformCheckedRead( readBase, 1 );
		PerformCheckedRead( readBase, 24 );
		PerformCheckedRead( readBase, 99 );
		PerformCheckedRead( readBase, 511 );
		PerformCheckedRead( readBase+1, 1 );
		PerformCheckedRead( readBase+1, 24 );
		PerformCheckedRead( readBase+1, 99 );
		PerformCheckedRead( readBase+1, 511 );
		PerformCheckedRead( readBase+3, 1 );
		PerformCheckedRead( readBase+3, 24 );
		PerformCheckedRead( readBase+3, 99 );
		PerformCheckedRead( readBase+3, 511 );

		PerformCheckedThreadRead( readBase, 1, 0 );
		PerformCheckedThreadRead( readBase, 24, 0 );
		PerformCheckedThreadRead( readBase, 99, 2 );
		PerformCheckedThreadRead( readBase, 511, 0 );
		PerformCheckedThreadRead( readBase+1, 1, 11 );
		PerformCheckedThreadRead( readBase+1, 24, 4 );
		PerformCheckedThreadRead( readBase+1, 99, 24 );
		PerformCheckedThreadRead( readBase+1, 511, 0 );
		PerformCheckedThreadRead( readBase+3, 1, 32 );
		PerformCheckedThreadRead( readBase+3, 24, 333 );
		PerformCheckedThreadRead( readBase+3, 99, 0 );
		PerformCheckedThreadRead( readBase+3, 511, 1 );
		}
	}

void CReadTest::TestSimpleScatterReads1()
	//
	// Does some simple reads of varying length from the
	// blocks in pseudo-random order.
	//
	{
	test.Next( _L("Testing simple scatter reads\n") );

	TRandomGenerator random;
	random.SetSeed( KRandomTestSeed );

	for( TInt readLen = 1; readLen <= 512; readLen++ )
		{
		TInt block = random.Next() % iBlockCount;
		test.Printf( _L("Reading block %d"), block );
		TInt readBase = (block * iBlockSize);
		PerformCheckedRead( readBase, readLen );
		}
	}

void CReadTest::TestSimpleScatterReads2()
	//
	// Does some simple reads of varying length from the
	// blocks in pseudo-random order.
	//
	// This is similar to TestSimpleScatterReads1 except that
	// as the length reduces the read position is moved along
	// and the test uses the thread-read variant
	//
	{
	test.Next( _L("Testing simple scatter reads\n") );

	TRandomGenerator random;
	random.SetSeed( KRandomTestSeed );

	for( TInt readLen = 1; readLen <= 512; readLen++ )
		{
		TInt block = random.Next() % iBlockCount;
		test.Printf( _L("Reading block %d"), block );
		TInt readBase = (block * iBlockSize) + (512 - readLen);
		PerformCheckedRead( readBase, readLen );
		}
	}

void CReadTest::TestScatterGather()
	//
	// This reads bytes from all over the Flash and concatenates
	// them into a single descriptor. This isn't representative of
	// anything a real filesystem would do (at present!) but
	// is an interesting test of the media driver
	//
	{
	test.Next( _L("Testing scatter-gather reads\n") );

	TRandomGenerator random;
	random.SetSeed( KRandomTestSeed );

	const TInt KMaxReads = 500;
	struct SReadInfo
		{
		TInt	iOffset;
		TInt	iLength;
		};

	SReadInfo* readInfoArray = new SReadInfo[KMaxReads];
	test( NULL != readInfoArray );

	TPtr8 des(0,0);
	iBuffer->InitialiseGuard();
	iBuffer->GetDes( des, 0, KTestUserDataSize );
	TInt descOffset = 0;

	TInt readCount;
	for( readCount = 0; readCount < KMaxReads; readCount++ )
		{
		//
		// Create random read position and length
		//
		TInt block = random.Next() % iBlockCount;
		TInt blockOffset = random.Next() % 1000;
		if( blockOffset > 500 )
			{
			blockOffset = iBlockSize - 1 - blockOffset;
			}
		TInt readOffset = (block * iBlockSize) + blockOffset;
		TInt readLength = (random.Next() % 8) + 1;

		if( des.Length() + readLength > des.MaxLength() )
			{
			break;	// finished
			}
		
		//
		// Save the position & length
		//
		readInfoArray[readCount].iOffset = readOffset;
		readInfoArray[readCount].iLength = readLength;

		//
		// do the read
		//
		_LIT( KScatterReadMsg, "Reading Flash @%x %d bytes to desc offset %d" );
		test.Printf( KScatterReadMsg, readOffset, readLength, descOffset );
#if 0
		test( KErrNone == iDrive.Read( readOffset, readLength, &des, iDummyThread.Handle(), descOffset ) );
#else
		test( KErrNone == iDrive.Read( readOffset, readLength, &des, KLocalMessageHandle, descOffset ) );
#endif
		test( des.Length() == descOffset + readLength );

		descOffset += readLength;
		}

	//
	// Now check all the data against the Flash contents
	//
	descOffset = 0;
	for( TInt i = 0; i < readCount; i++ )
		{
		TInt readOffset = readInfoArray[i].iOffset ;
		TInt readLength = readInfoArray[i].iLength;

		TPtrC8 ptr( des.Ptr() + descOffset, readLength );
		test( CompareAgainstFlash( readOffset, ptr, 0 ) );
		descOffset += readLength;
		}

	delete[] readInfoArray;

	}



void CReadTest::TestReadAcrossBlock()
	//
	// Test reads that cross a block boundary
	//
	{
	test.Next( _L("Testing reads across block boundary\n") );

	for( TInt block = 1; block < iBlockCount - 1; block++ )
		{
		for( TInt readLen = 2; readLen <= 1024; readLen++ )
			{
			TInt blockBase = (block * iBlockSize);
			TInt readOffs = blockBase + (iBlockSize - (readLen/2));
			PerformCheckedRead( readOffs, readLen );
			}
		}
	}



void CReadTest::CreateSampleData()
	//
	// Fills iSampleData with pseudo-random test data
	//
	{
	TUint32* p = (TUint32*)iSampleData.Ptr();
	for( TInt j = 0; j < KTestUserDataSize/4; j++ )
		{
		*p++ = iRandom.Next();
		}

	iSampleData.SetLength( KTestUserDataSize );
	}


TBool CReadTest::CheckZero( const TPtrC8& aDes )
	//
	// Checks that all bytes in aDes are zero
	//
	{
	for( TInt i = aDes.Length(); i > 0; )
		{
		--i;
		if( 0 != aDes[i] )
			{
			return EFalse;
			}
		}
	return ETrue;
	}



void CReadTest::CreateTestData( TInt aBlockNumber, TBool aEndOfBlock )
	//
	// Writes some test data to the Flash. If aEndOfBlock is EFalse the
	// data is created at the start of the block. If it is ETrue then
	// the data is created right at the end of the block
	//
	{

	test.Printf( _L("Writing test data to Flash block %d\n"), aBlockNumber );
	
	//
	// Generate some test data
	//
	CreateSampleData();

	test.Printf( _L("Erasing block") );
	TInt writeBaseOffset = (aBlockNumber * iBlockSize);
	test( KErrNone == iDrive.Format( writeBaseOffset, iBlockSize ) );

	
	TInt writeCount = iSampleData.Length() / KMaxWriteLength;
	TInt r = KErrNone;
	if( aEndOfBlock )
		{
		writeBaseOffset += iBlockSize - iSampleData.Length();
		}

	TInt writeOffset = writeBaseOffset;

	const TUint8* src = iSampleData.Ptr();

	test.Printf( _L("Writing data") );
	for( ; (writeCount > 0) && (KErrNone == r); writeCount-- )
		{
		TPtrC8 buf( src, KMaxWriteLength );
		test( KErrNone == iDrive.Write( writeOffset, buf ) );
		writeOffset += KMaxWriteLength;
		src += KMaxWriteLength;
		}
	test( r == KErrNone );

	//
	// check that the data was written ok
	//
	test.Printf( _L("Verifying data") );
	test( CompareAgainstFlash( writeBaseOffset, iSampleData, 0 ) );

	test.Printf( _L("... test data written\n") );
	}

TBool CReadTest::CompareAgainstFlash( TInt aFlashOffset, const TPtrC8& aDes, TInt aDescOffset )
	//
	// Checks that the data in aDes matches that in the Flash at position
	// aFlashOffset.
	// The test starts at offset aDescOffset in aSampleData. The data length
	// tested is aDes->Length() - aDescOffset
	//
	{
	TInt dataLength = aDes.Length() - aDescOffset;
	const TUint8* srcPtr = aDes.Ptr() + aDescOffset;

	TUint offset = aFlashOffset;
	
	TBool failed = EFalse;
	const TInt readBufLen = iReadBuffer.MaxLength();

	while( (dataLength > 0) && !failed )
		{
		TInt len = Min( dataLength, readBufLen );
		TInt r = iDrive.Read( offset, len, iReadBuffer );
		if( r != KErrNone )
			{
			test.Printf( _L("... FAIL: read failed (%d) at offset 0x%x\n"), r, offset );
			test( KErrNone == r );
			}
		test( iReadBuffer.Length() == len );

		if( 0 != Mem::Compare( srcPtr, len, iReadBuffer.Ptr(), len ) )
			{
			test.Printf( _L("... FAIL: mismatch around offset 0x%x\n"), offset );
			failed = ETrue;
			}
		offset += len;
		dataLength -= len;
		srcPtr += len;
		}
	
	return !failed;
	}



void CReadTest::DoTest()
	//
	// Main test dispatcher
	//
	{
	//
	// Create some test data at start of block 0
	//
	CreateTestData( 0, EFalse );

	//
	// Now do the simple tests, all reads will return zeros
	//
#if 0
	TestSimpleReads();
#endif
	TestSimpleThreadReads();
	TestUnalignedReads();
	TestUnalignedThreadReads();
	TestOffsetBufferThreadReads();
	TestOffsetBufferUnalignedThreadReads();

	//
	// Create some more data at start of all other blocks
	//
	test.Next( _L("Creating more test data in other blocks") );
	for( TInt i = 1; i < iBlockCount; i++ )
		{
		CreateTestData( i, EFalse );
		}

	//
	// Make sure we can read valid data out of the other blocks
	//
	TestReadsFromAllBlocks();

	//
	// Now do some scatter-read tests
	//
	TestSimpleScatterReads1();
	TestSimpleScatterReads2();

	//
	// Create some more testdata at end of all blocks
	//
	test.Next( _L("Creating test data at end of blocks") );
	for( TInt i = 0; i < iBlockCount; i++ )
		{
		CreateTestData( i, ETrue );
		}

	//
	// Do a full scatter-gather test
	//
	TestScatterGather();

	TestReadAcrossBlock();
	}





TInt E32Main()
	{
	test.Title();
	test.Start(_L("Testing media read operations"));

	CReadTest reader;
	TRAPD( ret, reader.CreateL() );
	test( KErrNone == ret );
	reader.DoTest();
	test.End();

	return 0;
	}
