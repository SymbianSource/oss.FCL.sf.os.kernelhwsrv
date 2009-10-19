// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include "tf_write.h"

_LIT( KTestName, "TF_WRITE" );
RTest test( KTestName );


const TInt64 KRandomSeed1(MAKE_TINT64(0x3e000111,0xAFCBDF0F));


GLDEF_C void Panic( TPanicNo aPanic )
	{
	User::Panic( KTestName, aPanic );
	}


// **********************************************************************
// Implementation of the writer classes

TWriteBase::TWriteBase( CWriteTest& aOwner )
	: iOwner( aOwner )
	{
	}

void TWriteBase::CheckedWrite(TInt aPos,const TDesC8& aSrc)
	{
	Write( aPos, aSrc );
	test( iOwner.CompareAgainstFlash( aPos, aSrc ) );
	}


TSimpleWrite::TSimpleWrite( CWriteTest& aOwner )
	: TWriteBase( aOwner ), iDrive( aOwner.Drive() )
	{
	}

void TSimpleWrite::Write(TInt aPos,const TDesC8& aSrc)
	{
	TInt64	pos( aPos );
//	test( KErrNone == iDrive.Write( pos, aSrc ) );
	TInt rv = iDrive.Write( pos, aSrc );
	if( KErrNone != rv )
		{
		test.Printf( _L("TBusLocalDrive::Write returned %d"), rv );
		test( EFalse );
		}
	}


	
TThreadWrite::TThreadWrite( CWriteTest& aOwner )
	: TWriteBase( aOwner ), iDrive( aOwner.Drive() ),
	iThreadHandle( aOwner.DummyThreadHandle() )
	{
	}

void TThreadWrite::Write(TInt aPos,const TDesC8& aSrc)
	{
	TInt64	pos( aPos );
#if 0
	test( KErrNone == iDrive.Write( pos, aSrc.Length(), &aSrc, iThreadHandle, 0 ) );
#else
	test( KErrNone == iDrive.Write( pos, aSrc.Length(), &aSrc, KLocalMessageHandle, 0 ) );
#endif
	}

		
void TThreadWrite::CheckedThreadWrite(TInt aPos, TInt aLength, const TDesC8& aSrc, TInt aDescOffset )
	{
	TInt64	pos( aPos );
#if 0
	test( KErrNone == iDrive.Write( pos, aLength, &aSrc, iThreadHandle, aDescOffset ) );
#else
	test( KErrNone == iDrive.Write( pos, aLength, &aSrc, KLocalMessageHandle, aDescOffset ) );
#endif
	test( iOwner.CompareAgainstFlash( aPos, aLength, aSrc, aDescOffset ) );
	}

void TThreadWrite::CurrentThreadCheckedThreadWrite(TInt aPos, TInt aLength, const TDesC8& aSrc, TInt aDescOffset )
	{
	TInt64	pos( aPos );
#if 0
	test( KErrNone == iDrive.Write( pos, aLength, &aSrc, RThread().Handle(), aDescOffset ) );
#else
	test( KErrNone == iDrive.Write( pos, aLength, &aSrc, KLocalMessageHandle, aDescOffset ) );
#endif
	test( iOwner.CompareAgainstFlash( aPos, aLength, aSrc, aDescOffset ) );
	}

// **********************************************************************
// Implementation of CBlockManager

CBlockManager::CBlockManager( TBusLocalDrive& aDrive, CWriteTest& aOwner )
	: iDrive( aDrive ), iOwner( aOwner )
	{
	}

CBlockManager::~CBlockManager()
	{
	delete[] iEraseArray;
	}

void CBlockManager::CreateL()
	{
	//
	// Get size of Flash drive
	//
	test.Printf( _L("Reading block info...") );
	TLocalDriveCapsV2Buf info;
    iDrive.Caps(info);
	TUint flashSize = I64LOW(info().iSize);
	test( 0 == I64HIGH(info().iSize));
	iBlockSize = info().iEraseBlockSize;
	test( 0 == (iBlockSize & 3) );
	iBlockCount = flashSize / iBlockSize;
	test( 0 != iBlockCount );

	test.Printf( _L("Flash block size=0x%x; block count=%d\n"), iBlockSize, iBlockCount );
	
	iEraseArray = new(ELeave) TEraseStatus[iBlockCount];
	test.Printf( _L("Erase status array created") );
	}


void CBlockManager::EraseBlock( TInt aBlockNumber )
	{
	__ASSERT_ALWAYS( aBlockNumber < iBlockCount, Panic( EPanicEraseBlockOOR ) );
	__ASSERT_ALWAYS( aBlockNumber < iBlockCount, Panic( EPanicEraseBlockNeg ) );
	_LIT( KEraseMsg, "Erasing block %d" );
	test.Printf( KEraseMsg, aBlockNumber );
	test( KErrNone == iDrive.Format( BlockAddress( aBlockNumber ), iBlockSize ) );
	VerifyErased( aBlockNumber );
	iEraseArray[ aBlockNumber ] = EErased;
	}

void CBlockManager::EraseAllBlocks()
	{
	_LIT( KEraseMsg, "Erasing all blocks" );
	test.Printf( KEraseMsg );
	for( TInt i = 0; i < iBlockCount; i++ )
		{
		EraseBlock( i );
		}
	}

void CBlockManager::VerifyErased( TInt aBlockNumber )
	{
	TUint offset = aBlockNumber * iBlockSize;
	
	TBool failed = EFalse;
	const TInt readBufLen = iReadBuffer.MaxLength();

	for( TInt remaining = iBlockSize; remaining > 0 && !failed ;)
		{
		TInt r = iDrive.Read( offset, readBufLen, iReadBuffer );
		if( r != KErrNone )
			{
			test.Printf( _L("... FAIL: read failed (%d) at offset 0x%x\n"), r, offset );
			test( KErrNone == r );
			}
		test( iReadBuffer.Length() == readBufLen );

		const TUint32* p = (const TUint32*)iReadBuffer.Ptr();
		for( TInt i = 0; i < readBufLen; i += 4 )
			{
			if( 0xFFFFFFFF != *p )
				{
				failed = ETrue;
				test.Printf( _L("... FAILED: byte @ offs=0x%x, read=0x%x, expected=0xFF\n"), 
								offset+i, p[0] );
				test(EFalse);
				}
			++p;
			}
		offset += readBufLen;
		remaining -= readBufLen;
		}
	}


void CBlockManager::InitialiseSequentialBlockAllocator()
	//
	// Clears the erase status and resets to block zero
	//
	{
	for( TInt i = 0; i < iBlockCount; i++ )
		{
		iEraseArray[i] = ENotErased;
		}
	iNextBlock = 0;
	}


TInt CBlockManager::NextErasedBlock()
	{
	if( iNextBlock >= iBlockCount )
		{
		iNextBlock = 0;
		}

	if( ENotErased == iEraseArray[iNextBlock] )
		{
		EraseBlock( iNextBlock );
		}
	iEraseArray[iNextBlock] = ENotErased;	// assume it is going to be used
	return iNextBlock;
	}

void CBlockManager::InitialiseDataChunkAllocator()
	{
	iDataBlock = NextErasedBlock();
	iDataOffset = 0;
	}


TUint CBlockManager::NextErasedDataChunk( TInt aRequiredLength, TInt aMultiple )
	//
	// Request a chunk of erased flash of size aRequiredLength bytes on a
	// boundary of aMultiple bytes. E,g, to allocate a buffer on 12 bytes length
	// on a 32-byte boundary, aRequiredLength = 12, aMultiple=12
	//
	// The byte count is rounded up to a multiple of 4 bytes
	//
	{
	aRequiredLength = (aRequiredLength + 3) & ~0x3;
	
	TUint chunkBase = ((iDataOffset + aMultiple - 1) / aMultiple) * aMultiple;
	if( chunkBase > (TUint)iBlockSize || chunkBase + aRequiredLength > (TUint)iBlockSize )
		{
		iDataBlock = NextErasedBlock();
		chunkBase = 0;
		}
	
	iDataOffset = ( chunkBase + aRequiredLength + 3) & ~0x3;

	return BlockAddress( iDataBlock ) + chunkBase;
	}



inline TInt CBlockManager::BlockCount() const
	{
	return iBlockCount;
	}

inline TInt CBlockManager::BlockSize() const
	{
	return iBlockSize;
	}

inline TInt CBlockManager::FlashSize() const
	{
	return iBlockSize * iBlockCount;
	}

inline TUint CBlockManager::BlockAddress( TInt aBlockNumber ) const
	{
	return (TUint)aBlockNumber * (TUint)iBlockSize;
	}


// **********************************************************************
// Implementation of CWriteTest

CWriteTest::~CWriteTest()
	{
	if( iDriveOpened )
		{
		iDrive.Disconnect();
		}

	delete iBlocks;
	delete iSimpleWriter;
	delete iThreadWriter;
	}


void CWriteTest::CreateL()
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
	r = iDrive.Connect( KDriveNumber, changedFlag );
	User::LeaveIfError( r );
	iDriveOpened = ETrue;

	//
	// Initialise the block manager
	//
	iBlocks = new(ELeave) CBlockManager( iDrive, *this );
	iBlocks->CreateL();

	//
	// Create a dummy thread that we can use to force
	// other-thread write operations
	//
#if 0
	test( KErrNone == iDummyThread.Create( _L("DUMMY"), DummyThread, 256, KMinHeapSize, KMinHeapSize, NULL ) );
#else
	test( KErrNone == iDummyThread.Create( _L("DUMMY"), DummyThread, KDefaultStackSize, KMinHeapSize, KMinHeapSize, NULL ) );
#endif
	test.Printf( _L("Main thread handle=%d; dummy thread handle=%d"),
		RThread().Handle(), DummyThreadHandle() );

	//
	// Create the writer classes
	//
	iSimpleWriter = new(ELeave) TSimpleWrite( *this );
	iThreadWriter = new(ELeave) TThreadWrite( *this );
	
	//
	// Seed the pseudo-random number generator
	//
	iRandom.SetSeed( KRandomSeed1 );


	test.Printf( _L("CWriteTest::CreateL complete\n") );
	}


TInt CWriteTest::DummyThread( TAny* /* aParam */ )
	//
	// Thread does nothing at all
	//
	{
	for(;;)
		{
		User::WaitForAnyRequest();	// just block
		}
	}

void CWriteTest::CreateRandomData( TDes8& aDestBuf, TInt aLength )
	//
	// Fills supplied descriptor with aLength bytes of pseudo-random test data
	//
	{
	aDestBuf.SetLength( aLength );
	TUint32* p = (TUint32*)aDestBuf.Ptr();
	for( TInt j = aLength/4; j > 0 ; j-- )
		{
		*p++ = iRandom.Next();
		}
	
	if( aLength & 0x3 )
		{
		TUint8* q = (TUint8*)p;
		for( TInt k = aLength & 3; k > 0; k-- )
			{
			*q++ = (TUint8)iRandom.Next();
			}
		}
	}


TBool CWriteTest::CheckOnes( TUint aFlashOffset, TInt aLength )
	//
	// Checks that aLength bytes of data from offset aFlashOffset
	// all contain 0xFF
	//
	{
	TUint offset = aFlashOffset;
	
	TBool failed = EFalse;
	const TInt readBufLen = iReadBuffer.MaxLength();

	for( TInt remaining = aLength; remaining > 0 && !failed ;)
		{
		TInt readLen = Min( remaining, readBufLen );
		TInt r = iDrive.Read( offset, readLen, iReadBuffer );
		if( r != KErrNone )
			{
			test.Printf( _L("... FAIL: read failed (%d) at offset 0x%x\n"), r, offset );
			test( KErrNone == r );
			}
		test( iReadBuffer.Length() == readLen );

		const TUint8* p = iReadBuffer.Ptr();
		for( TInt i = 0; i < readLen; ++i )
			{
			if( 0xFF != *p )
				{
				failed = ETrue;
				test.Printf( _L("... FAILED: byte @ offs=0x%x, read=0x%x, expected=0xFF\n"), 
								offset+i, p[0] );
				break;
				}
			++p;
			}
		offset += readLen;
		remaining -= readLen;
		}
	
	return !failed;
	}


TBool CWriteTest::CompareAgainstFlash( TInt aFlashOffset, TInt aLength, const TDesC8& aDes, TInt aDescOffset )
	//
	// Checks that the data in aDes matches that in the Flash at position
	// aFlashOffset.
	// The test starts at offset aDescOffset in aSampleData. aLength bytes
	// are tested.
	//
	{
	__ASSERT_ALWAYS( aDescOffset + aLength <= aDes.Length(), Panic( EPanicCompareDescOverflow ) );
	TInt dataLength = aLength;
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

TBool CWriteTest::CompareAgainstFlash( TInt aFlashOffset, const TDesC8& aDes )
	//
	// Checks that the data in aDes matches that in the Flash at position
	// aFlashOffset.
	// aDes->Length() bytes are tested.
	//
	{
	return CompareAgainstFlash( aFlashOffset, aDes.Length(), aDes, 0 );
	}


void CWriteTest::SimpleWriteTest()
	{
	test.Next( _L("Simple write test, simple write function") );
	DoSimpleWriteTest( *iSimpleWriter );
	}

void CWriteTest::SimpleThreadWriteTest()
	{
	test.Next( _L("Simple write test, thread write function") );
	DoSimpleWriteTest( *iThreadWriter );
	}


void CWriteTest::DoSimpleWriteTest( MGeneralizedWrite& aWriter )
	//
	// Writes some random test data to the start of a block, checks that
	// it is written correctly and that the source data isn't modified
	//
	{
	TInt blockNo = iBlocks->NextErasedBlock();
	TUint blockBase = iBlocks->BlockAddress( blockNo );

	TBuf8<512> randomData;
	CreateRandomData( randomData, randomData.MaxLength() );

	TBuf8<512> randomDataDuplicate;
	randomDataDuplicate.Copy( randomData );
	test( randomDataDuplicate == randomData );

	TBuf8<sizeof(TPtr)> ptrCopy;	// used to take copies of descriptors

	//
	// Write using a constant descriptor TPtrC
	//
	test.Printf( _L("Write using TPtrC") );
	TPtrC8 ptrC( randomData );
	ptrCopy.Copy( (TUint8*)&ptrC, sizeof(ptrC) );

	aWriter.CheckedWrite( blockBase + 0, ptrC );

	test.Printf( _L("Check descriptor not modified by write function") );
	test( 0 == Mem::Compare( (TUint8*)&ptrC, sizeof(ptrC), ptrCopy.Ptr(), sizeof(ptrC) ) );

	test.Printf( _L("Check data not modified by write function") );
	test( randomDataDuplicate == randomData );

	//
	// Write using a modifiable descriptor TPtr
	//
	test.Printf( _L("Write using TPtr") );
	TPtr8 ptr( (TUint8*)randomData.Ptr(), randomData.Length(), randomData.Length() );
	ptrCopy.Copy( (TUint8*)&ptr, sizeof(ptr) );
	
	aWriter.CheckedWrite( blockBase + 1024, ptr );

	test.Printf( _L("Check descriptor not modified by write function") );
	test( 0 == Mem::Compare( (TUint8*)&ptr, sizeof(ptr), ptrCopy.Ptr(), sizeof(ptr) ) );

	test.Printf( _L("Check data not modified by write function") );
	test( randomDataDuplicate == randomData );

	//
	// Write using a modifiable descriptor TBuf
	//
	test.Printf( _L("Write using TBuf") );
	
	aWriter.CheckedWrite( blockBase + 2048, randomData );

	test.Printf( _L("Check descriptor not modified by write function") );
	test( ptrC.Ptr() == randomData.Ptr() );
	test( 512 == randomData.Length() );
	test( 512 == randomData.MaxLength() );

	test.Printf( _L("Check data not modified by write function") );
	test( randomDataDuplicate == randomData );

	//
	// Read the data back and check it matches
	//
	test.Printf( _L("Reading data back with TBusLocalDrive::Read") );
	test( KErrNone == iDrive.Read( blockBase + 0, 512, randomDataDuplicate ) );
	test( randomDataDuplicate == randomData );
	test( KErrNone == iDrive.Read( blockBase + 1024, 512, randomDataDuplicate ) );
	test( randomDataDuplicate == randomData );
	test( KErrNone == iDrive.Read( blockBase + 2048, 512, randomDataDuplicate ) );
	test( randomDataDuplicate == randomData );
	}



void CWriteTest::AlignedWriteTest()
	{
	test.Next( _L("Aligned write test, simple write function") );
	DoAlignedWriteTest( *iSimpleWriter );
	}

void CWriteTest::AlignedThreadWriteTest()
	{
	test.Next( _L("Aligned write test, thread write function") );
	DoAlignedWriteTest( *iThreadWriter );
	}


void CWriteTest::DoAlignedWriteTest( MGeneralizedWrite& aWriter )
	//
	// Writes data of various lengths to word-aligned addresses
	//
	{
	iBlocks->InitialiseDataChunkAllocator();

	TBuf8<512> data;	

	_LIT( KWriteMsg, "  writing %d bytes @0x%x" );

	test.Printf( _L("Testing small writes") );

	for( TInt length = 1; length < 16; length++ )
		{
		CreateRandomData( data, length );
		
		// get a 32-byte data chunk on a word boundary
		TUint offset = iBlocks->NextErasedDataChunk( 32, 4 );

		test.Printf( KWriteMsg, length, offset );
		aWriter.CheckedWrite( offset, data );
		// check that the section after the data still contains all ones
		test( CheckOnes( offset + length, 32 - length ) );
		}


	test.Printf( _L("Testing large writes") );
	for( TInt length = 512-32; length <= 512 ; length++ )
		{
		CreateRandomData( data, length );
		
		// get a 544-byte data chunk on a word boundary
		TUint offset = iBlocks->NextErasedDataChunk( 544, 4 );

		test.Printf( KWriteMsg, length, offset );
		aWriter.CheckedWrite( offset, data );

		// check that the section after the data still contains all ones
		test( CheckOnes( offset + length, 544 - length ) );
		}
	}




void CWriteTest::UnalignedWriteTest()
	{
	test.Next( _L("Unaligned write test, simple write function") );
	DoUnalignedWriteTest( *iSimpleWriter );
	}

void CWriteTest::UnalignedThreadWriteTest()
	{
	test.Next( _L("Unaligned write test, thread write function") );
	DoUnalignedWriteTest( *iThreadWriter );
	}


void CWriteTest::DoUnalignedWriteTest( MGeneralizedWrite& aWriter )
	//
	// Tests writing to unaligned addresses. "Unaligned" here means
	// addresses that are not on a word boundary.
	//
	{
	TBuf8<32> data;

	_LIT( KWriteMsg, "  writing 32 bytes @0x%x" );


	for( TInt offset = 1; offset < 32; offset++ )
		{
		CreateRandomData( data, data.MaxLength() );
		
		//
		// get a 64-byte data chunk on a 256-byte boundary, then
		// start the write at <offset> bytes into this buffer
		//
		TUint dataChunk = iBlocks->NextErasedDataChunk( 64, 256 );

		test.Printf( KWriteMsg, dataChunk + offset );
		aWriter.CheckedWrite( dataChunk + offset, data );

		_LIT( KBeforeMsg,  " checking unused portion before data" );
		test.Printf( KBeforeMsg );
		test( CheckOnes( dataChunk, offset ) );

		// check that the section after the data still contains all ones
		_LIT( KAfterMsg, " checking unused portion after data" );
		test.Printf( KAfterMsg );
		test( CheckOnes( dataChunk + offset + data.Length(), 64 - offset - data.Length() ) );
		}
	}



void CWriteTest::OffsetDescriptorAlignedWriteTest()
	//
	// Tests writing using an offset into the source data buffer. Writes
	// are done to word-aligned destination addresses.
	//
	{
	test.Next( _L("Offset-desc write test, aligned dest address") );

	TBuf8<64> data;

	_LIT( KWriteMsg, "  writing 32 bytes from offset %d to @0x%x" );

//	CreateRandomData( data, data.MaxLength() );
	data.SetLength(64);
	for( TInt i = 0; i < 64; i++ )
		{
		data[i] = i;
		}

	for( TInt descOffset = 1; descOffset < 32; descOffset++ )
		{
		//
		// Get a 32-byte data chunk on a word boundary.
		//
		TUint dataChunk = iBlocks->NextErasedDataChunk( 32, 4 );

		test.Printf( KWriteMsg, descOffset, dataChunk );
		iThreadWriter->CheckedThreadWrite( dataChunk, 32, data, descOffset );

		//
		// Read the data back out and check it matches
		//
		_LIT( KReadBackMsg, "Reading back data" );
		test.Printf( KReadBackMsg );
		TBuf8<32> readData;
		iDrive.Read( dataChunk, 32, readData );
		TPtrC8 ptr( data.Ptr() + descOffset, 32 );
		test( ptr == readData );
		}
	}


void CWriteTest::OffsetDescriptorUnalignedWriteTest()
	//
	// This is a variation of OffsetDescriptorAlignedWriteTest that
	// also writes to non-word-aligned destionation addresses.
	//
	{
	test.Next( _L("Offset-desc write test, unaligned dest address") );

	TBuf8<64> data;

	_LIT( KWriteMsg, "  writing 32 bytes from offset %d to @0x%x" );

	CreateRandomData( data, data.MaxLength() );

	for( TInt descOffset = 1; descOffset < 32; descOffset++ )
		{
		for( TInt unalign = 1; unalign < 4; unalign++ )
			{
			//
			// Get a 40-byte data chunk on a word boundary.
			//
			TUint dataChunk = iBlocks->NextErasedDataChunk( 40, 4 );
			TUint destOffset = dataChunk + unalign;

			test.Printf( KWriteMsg, descOffset, destOffset );
			iThreadWriter->CheckedThreadWrite( destOffset, 32, data, descOffset );

			//
			// Read the data back out and check it matches
			//
			_LIT( KReadBackMsg, "Reading back data" );
			test.Printf( KReadBackMsg );
			TBuf8<32> readData;
			iDrive.Read( destOffset, 32, readData );
			TPtrC8 ptr( data.Ptr() + descOffset, 32 );
			test( ptr == readData );
			}
		}
	}


void CWriteTest::OffsetDescriptorCurrentThreadAlignedWriteTest()
	//
	// Tests writing using an offset into the source data buffer. Writes
	// are done to word-aligned destination addresses. This uses the
	// thread variant of the write function but passes the handle
	// of this thread.
	//
	{
	test.Next( _L("Offset-desc write test, current thread, aligned dest address") );

	TBuf8<64> data;

	_LIT( KWriteMsg, "  writing 32 bytes from offset %d to @0x%x" );

//	CreateRandomData( data, data.MaxLength() );
	data.SetLength(64);
	for( TInt i = 0; i < 64; i++ )
		{
		data[i] = i;
		}

	for( TInt descOffset = 1; descOffset < 32; descOffset++ )
		{
		//
		// Get a 32-byte data chunk on a word boundary.
		//
		TUint dataChunk = iBlocks->NextErasedDataChunk( 32, 4 );

		test.Printf( KWriteMsg, descOffset, dataChunk );
		iThreadWriter->CurrentThreadCheckedThreadWrite( dataChunk, 32, data, descOffset );

		//
		// Read the data back out and check it matches
		//
		_LIT( KReadBackMsg, "Reading back data" );
		test.Printf( KReadBackMsg );
		TBuf8<32> readData;
		iDrive.Read( dataChunk, 32, readData );
		TPtrC8 ptr( data.Ptr() + descOffset, 32 );
		test( ptr == readData );
		}
	}


void CWriteTest::OffsetDescriptorCurrentThreadUnalignedWriteTest()
	//
	// This is a variation of OffsetDescriptorCurrentThreadAlignedWriteTest
	// that also writes to non-word-aligned destionation addresses.
	//
	{
	test.Next( _L("Offset-desc write test, current thread, unaligned dest address") );

	TBuf8<64> data;

	_LIT( KWriteMsg, "  writing 32 bytes from offset %d to @0x%x" );

	CreateRandomData( data, data.MaxLength() );

	for( TInt descOffset = 1; descOffset < 32; descOffset++ )
		{
		for( TInt unalign = 1; unalign < 4; unalign++ )
			{
			//
			// Get a 40-byte data chunk on a word boundary.
			//
			TUint dataChunk = iBlocks->NextErasedDataChunk( 40, 4 );
			TUint destOffset = dataChunk + unalign;

			test.Printf( KWriteMsg, descOffset, destOffset );
			iThreadWriter->CurrentThreadCheckedThreadWrite( destOffset, 32, data, descOffset );

			//
			// Read the data back out and check it matches
			//
			_LIT( KReadBackMsg, "Reading back data" );
			test.Printf( KReadBackMsg );
			TBuf8<32> readData;
			iDrive.Read( destOffset, 32, readData );
			TPtrC8 ptr( data.Ptr() + descOffset, 32 );
			test( ptr == readData );
			}
		}
	}



void CWriteTest::JoinedWriteTest()
	//
	// Makes two consecutive writes. Checks that the complete
	// data block was written correctly. The data is written within
	// a 64-byte window and the join position is moved along to each
	// possible location
	{
	
	test.Next( _L("Joined write test, simple writes") );
	
	//
	// Reinitialise the chunk allocator
	//
	iBlocks->InitialiseDataChunkAllocator();
	
	for( TInt join = 1; join < 63; join++ )
		{
		TBuf8<64> fullData;
		CreateRandomData( fullData, fullData.MaxLength() );
		
		//
		// Create two TPtrC8s to the two parts of the data
		//
		TPtrC8 first( fullData.Ptr(), join );
		TPtrC8 second( fullData.Ptr() + join, fullData.MaxLength() - join );
		__ASSERT_ALWAYS( first.Length() + second.Length() == 64, Panic( EPanicJoinMaths ) );

		//
		// Get a location in the Flash to write to
		//
		TUint dataChunk = iBlocks->NextErasedDataChunk( 64, 64 );

		//
		// Write the two halves of the data
		//
		_LIT( KWriteMsg, "  writing %d bytes @ 0x%x and %d bytes @ 0x%x" );
		test.Printf( KWriteMsg, first.Length(), dataChunk,
								second.Length(), dataChunk + first.Length() );
		test( KErrNone == iDrive.Write( dataChunk, first ) );
		test( KErrNone == iDrive.Write( dataChunk + first.Length(), second ) );

		//
		// Compare the data
		//
		_LIT( KCompareMsg, "  comparing data against Flash" );
		test.Printf( KCompareMsg );
		test( CompareAgainstFlash( dataChunk, fullData ) );
		}
	}


void CWriteTest::JoinedThreadWriteTest()
	//
	// Makes two consecutive writes. Checks that the complete
	// data block was written correctly. The data is written within
	// a 64-byte window and the join position is moved along to each
	// possible location
	//
	// This is similar to JoinedWriteTest except that the thread write
	// function is used with a descriptor offset to chop up the
	// source data
	//
	{
	
	test.Next( _L("Joined write test, thread writes") );
	
	//
	// Reinitialise the chunk allocator
	//
	iBlocks->InitialiseDataChunkAllocator();
	
	for( TInt join = 1; join < 63; join++ )
		{
		TBuf8<64> fullData;
		CreateRandomData( fullData, fullData.MaxLength() );
		
		//
		// Get a location in the Flash to write to
		//
		TUint dataChunk = iBlocks->NextErasedDataChunk( 64, 64 );

		//
		// Write the two halves of the data
		//
		_LIT( KWriteMsg, "  writing %d bytes @ 0x%x and %d bytes @ 0x%x" );
		test.Printf( KWriteMsg, join, dataChunk, 64 - join, dataChunk + join );
#if 0
		test( KErrNone == iDrive.Write( dataChunk, join, &fullData, DummyThreadHandle(), 0 ) );
		test( KErrNone == iDrive.Write( dataChunk + join, 64-join, &fullData, DummyThreadHandle(), join ) );
#else
		test( KErrNone == iDrive.Write( dataChunk, join, &fullData, KLocalMessageHandle, 0 ) );
		test( KErrNone == iDrive.Write( dataChunk + join, 64-join, &fullData, KLocalMessageHandle, join ) );
#endif


		//
		// Compare the data
		//
		_LIT( KCompareMsg, "  comparing data against Flash" );
		test.Printf( KCompareMsg );
		test( CompareAgainstFlash( dataChunk, fullData ) );
		}
	}



void CWriteTest::SingleBitOverwriteTest()
	//
	// Tests overwriting single bits within a byte. a 32-byte
	// section of Flash is filled with data, with one byte initially
	// 0xFF. A bit is then written to zero and the whole data block
	// is verified.
	//
	{
	test.Next( _L("Single bit overwrite test") );

	iBlocks->InitialiseDataChunkAllocator();

	for( TInt testByteOffset = 0; testByteOffset < 32; testByteOffset++ )
		{
		for( TInt testBitNumber = 0; testBitNumber < 8; testBitNumber++ )
			{
			TBuf8<32> data;
			CreateRandomData( data, data.MaxLength() );
			data[ testByteOffset ] = 0xFF;	// force test byte to 0xFF

			TUint flashOffset = iBlocks->NextErasedDataChunk( 32, 32 );
			
			_LIT( KWriteMsg, "writing test data @0x%x, test byte offset=%d; test bit #%d");
			test.Printf( KWriteMsg, flashOffset, testByteOffset, testBitNumber );

			iSimpleWriter->CheckedWrite( flashOffset, data );

			// clear the test bit
			TBuf8<1> byte;
			byte.SetLength(1);
			byte[0] = ~(1 << testBitNumber);
			data[ testByteOffset ] = byte[0];
			
			iSimpleWriter->CheckedWrite( flashOffset + testByteOffset, byte );

			// check that the contents of the Flash matches the buffer
			test( CompareAgainstFlash( flashOffset, data ) );
			}
		}
	}

void CWriteTest::TwoBitOverwriteTest()
	//
	// Tests overwriting two bits within a byte. a 32-byte
	// section of Flash is filled with data, with one byte initially
	// 0xFF. Two bits are then written to zero and the whole data block
	// is verified.
	//
	{
	static const TUint pattConv[16] =
		{
		// used to create a string representation of binary value
		0x0000, 0x0001, 0x0010, 0x0011, 0x0100, 0x0101, 0x0110, 0x0111,
		0x1000, 0x1001, 0x1010, 0x1011, 0x1100, 0x1101, 0x1110, 0x1111
		};
	test.Next( _L("Two bit overwrite test") );

	for( TInt testByteOffset = 0; testByteOffset < 32; testByteOffset++ )
		{
		for( TInt testBitJ = 0; testBitJ < 7; testBitJ++ )
			{
			for( TInt testBitK = testBitJ+1; testBitK < 8; testBitK++ )
				{
				TBuf8<32> data;
				CreateRandomData( data, data.MaxLength() );
				data[ testByteOffset ] = 0xFF;	// force test byte to 0xFF

				TUint flashOffset = iBlocks->NextErasedDataChunk( 32, 32 );
				
				TUint8 testPattern = ~((1 << testBitJ) | (1 << testBitK));

				_LIT( KWriteMsg, "writing test data @0x%x, test byte offset=%d; test pattern = %04x%04x");
				test.Printf( KWriteMsg, flashOffset, testByteOffset, 
							pattConv[ testPattern >> 4 ], pattConv[ testPattern&0xF ] );

				iSimpleWriter->CheckedWrite( flashOffset, data );

				TBuf8<1> byte;
				byte.SetLength(1);
				byte[0] = testPattern;
				data[ testByteOffset ] = testPattern;
				
				iSimpleWriter->CheckedWrite( flashOffset + testByteOffset, byte );

				// check that the contents of the Flash matches the buffer
				test( CompareAgainstFlash( flashOffset, data ) );
				}
			}
		}
	}


void CWriteTest::RunSimulationTest()
	//
	// A simulation of the way the LFFS filesystem will use a Flash block
	// Alternately writes 24 bytes to bottom of block, 512 bytes to top,
	// clears a bit in the 24-byte block. Repeats until block is full.
	//
	{
	test.Next( _L("Simulation test") );

	TUint blockBase = iBlocks->BlockAddress( iBlocks->NextErasedBlock() );

	TUint lowAddress = blockBase;
	TUint highAddress = blockBase + iBlocks->BlockSize() - 512;

	TBuf8<24> lowData;
	TBuf8<512> highData;
	TPtrC8 overwritePtr( lowData.Ptr(), 1 );

	while( lowAddress + 24 < highAddress )
		{
		CreateRandomData( lowData, lowData.MaxLength() );
		CreateRandomData( highData, highData.MaxLength() );
		lowData[0] = 0xE7;	// just some non-0xFF value

		_LIT( KWriteMsg, "Writing block size 24 @ 0x%x; block size 512 @ 0x%x" );
		test.Printf( KWriteMsg, lowAddress, highAddress );

		iSimpleWriter->CheckedWrite( lowAddress, lowData );
		iSimpleWriter->Write( highAddress, highData );

		// Overwrite the byte
		lowData[0] = 0xA7;
		iSimpleWriter->Write( lowAddress, overwritePtr );

		test( CompareAgainstFlash( lowAddress, lowData ) );
		test( CompareAgainstFlash( highAddress, highData ) );

		lowAddress += lowData.Length();
		highAddress -= highData.Length();
		}
	}



void CWriteTest::DoTests()
	//
	// Main test dispatcher
	//
	{
	test.Next( _L("Erasing all blocks") );
	iBlocks->InitialiseSequentialBlockAllocator();
	iBlocks->EraseAllBlocks();

	//
	// Basic tests that we can write data correctly without corrupting
	// the source buffer
	//
	SimpleWriteTest();
	SimpleThreadWriteTest();

	//
	// Test aligned writes of various lengths
	//
	AlignedWriteTest();
	AlignedThreadWriteTest();

	//
	// Test writing to unaligned locations
	//
	UnalignedWriteTest();
	UnalignedThreadWriteTest();

	//
	// Test writes with offset into source desriptor
	//
	OffsetDescriptorCurrentThreadAlignedWriteTest();
	OffsetDescriptorCurrentThreadUnalignedWriteTest();
	OffsetDescriptorAlignedWriteTest();
	OffsetDescriptorUnalignedWriteTest();

	//
	// Test two consecutive writes
	//
	JoinedWriteTest();
	JoinedThreadWriteTest();

	//
	// Test that we can overwrite bits
	//
	SingleBitOverwriteTest();
	TwoBitOverwriteTest();

	//
	// A simulation test of LFFS usage
	//
	RunSimulationTest();
	}










TInt E32Main()
	{
	test.Title();
	test.Start(_L("Testing media read operations"));

	CWriteTest writeTest;
	TRAPD( ret, writeTest.CreateL() );
	test( KErrNone == ret );
	writeTest.DoTests();
	test.End();

	return 0;
	}
