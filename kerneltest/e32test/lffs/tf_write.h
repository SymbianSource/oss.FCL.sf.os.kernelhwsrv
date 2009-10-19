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


#ifndef __TF_WRITE_H__
#define __TF_WRITE_H__

#include <e32std.h>

enum TPanicNo
	{
	EPanicGetDesOverflow,
	EPanicGetDesInitialOverflow,
	EPanicCheckOverflow,
	EPanicCompareDescOverflow,
	EPanicEraseBlockOOR,
	EPanicEraseBlockNeg,
	EPanicJoinMaths
	};

GLREF_C void Panic( TPanicNo aPanic );

class CWriteTest;

class MGeneralizedWrite
	//
	// Provides an generic interface to a write function which
	// can be either the simple of thread version. Used to hide which
	// version of the write function a test is using
	//
	{
	public:
		virtual void Write(TInt aPos,const TDesC8& aSrc) = 0;
		virtual void CheckedWrite(TInt aPos,const TDesC8& aSrc) = 0;
	};

class TWriteBase: public MGeneralizedWrite
	{
	public:
		TWriteBase( CWriteTest& aOwner );
		virtual void CheckedWrite(TInt aPos,const TDesC8& aSrc);
	
	protected:
		CWriteTest&	iOwner;
	};


class TSimpleWrite : public TWriteBase
	//
	// Simple implementation of write function
	//
	{
	public:
		TSimpleWrite( CWriteTest& aOwner );
		virtual void Write( TInt aPos, const TDesC8& aSrc );

	private:
		TBusLocalDrive&	iDrive;
	};

class TThreadWrite : public TWriteBase
	//
	// Thread implementation of write function
	//
	{
	public:
		TThreadWrite( CWriteTest& aOwner );
		virtual void Write(TInt aPos,const TDesC8& aSrc);
		
		// Thread functions with offset, added by this class
		void CheckedThreadWrite(TInt aPos, TInt aLength, const TDesC8& aSrc, TInt aDescOffset );
		void CurrentThreadCheckedThreadWrite(TInt aPos, TInt aLength, const TDesC8& aSrc, TInt aDescOffset );

	private:
		TBusLocalDrive& iDrive;
		const TInt iThreadHandle;
	};


class CBlockManager : public CBase
	//
	// class used to control erasing and allocation of blocks
	//
	{
	public:
		CBlockManager( TBusLocalDrive& iDrive, CWriteTest& aOwner );
		~CBlockManager();

		void CreateL();

		void EraseBlock( TInt aBlockNumber );
		void EraseAllBlocks();
		void VerifyErased( TInt aBlockNumber );

		void InitialiseSequentialBlockAllocator();
		TInt NextErasedBlock();

		void InitialiseDataChunkAllocator();
		TUint NextErasedDataChunk( TInt aRequiredLength, TInt aMultiple=4 );


		inline TInt BlockCount() const;
		inline TInt BlockSize() const;
		inline TInt FlashSize() const;
		inline TUint BlockAddress( TInt aBlockNumber ) const;

	private:
		TBusLocalDrive&	iDrive;
		TBuf8<512>	iReadBuffer;
		CWriteTest&	iOwner;
		TInt	iBlockCount;
		TInt	iBlockSize;

		enum TEraseStatus
			{
			EErased,
			ENotErased
			};
		TEraseStatus*	iEraseArray;

		TInt iNextBlock;

		TInt iDataBlock;
		TInt iDataOffset;
	};



class CWriteTest : public CBase
	{
	public:
		~CWriteTest();

		void CreateL();

		void DoTests();

		TBool CompareAgainstFlash( TInt aFlashOffset, const TDesC8& aDes );
		TBool CompareAgainstFlash( TInt aFlashOffset, TInt aLength, const TDesC8& aDes, TInt aDescOffset );

		inline const TUint8* ChunkBase() const;
		inline TBusLocalDrive& Drive();
		inline TInt DummyThreadHandle() const;

	private:
		static TInt DummyThread( TAny* aParam );

		void CreateRandomData( TDes8& aDestBuf, TInt aLength );
		TBool CheckOnes( TUint aFlashOffset, TInt aLength );
		void CreateTestData( TInt aBlockNumber, TBool aEndOfBlock );

		void SimpleWriteTest();
		void SimpleThreadWriteTest();
		void DoSimpleWriteTest( MGeneralizedWrite& aWriter );

		void AlignedWriteTest();
		void AlignedThreadWriteTest();
		void DoAlignedWriteTest( MGeneralizedWrite& aWriter );

		void UnalignedWriteTest();
		void UnalignedThreadWriteTest();
		void DoUnalignedWriteTest( MGeneralizedWrite& aWriter );

		void OffsetDescriptorAlignedWriteTest();
		void OffsetDescriptorUnalignedWriteTest();
		void OffsetDescriptorCurrentThreadAlignedWriteTest();
		void OffsetDescriptorCurrentThreadUnalignedWriteTest();

		void JoinedWriteTest();
		void JoinedThreadWriteTest();

		void SingleBitOverwriteTest();
		void TwoBitOverwriteTest();
		
		void RunSimulationTest();


	private:
		TBusLocalDrive	iDrive;
		TBool			iDriveOpened;
		TBuf8<512>		iReadBuffer;
		CBlockManager*	iBlocks;

		TRandomGenerator	iRandom;

		TSimpleWrite*	iSimpleWriter;
		TThreadWrite*	iThreadWriter;

		RThread	iDummyThread;
	};




inline TBusLocalDrive& CWriteTest::Drive()
	{
	return iDrive;
	}

inline TInt CWriteTest::DummyThreadHandle() const
	{
	return iDummyThread.Handle();
	}


#endif

