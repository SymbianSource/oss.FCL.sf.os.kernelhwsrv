// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Soak tests reading from files
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32test.h>
#include <f32file.h>
#include "utl.h"
#include "randgen.h"

GLREF_D RFs TheFs;
GLDEF_D TInt gDriveNum;

RTest	test( _L("T_ROFSSOAK") );

_LIT( KTestFile1, "ReadTest\\A\\file1" );
_LIT( KTestFile2, "ReadTest\\A\\file2" );
_LIT( KTestFile3, "ReadTest\\B\\B3\\testfile" );
_LIT( KTestFile4, "ReadTest\\B\\B4\\testfile" );
_LIT( KTestFile5, "ReadTest\\D\\eoftest4" );
_LIT( KTestFile6, "ReadTest\\D\\eoftest5" );
_LIT( KTestFile7, "ReadTest\\E\\stream3" );
_LIT( KTestFile8, "ReadTest\\E\\stream4" );
_LIT( KTestFile9, "ReadTest\\C\\seektest" );
_LIT( KTestFile10, "ReadTest\\D\\eoftest1" );
_LIT( KTestFile11, "ReadTest\\D\\eoftest2" );
_LIT( KTestFile12, "ReadTest\\D\\eoftest3" );
_LIT( KTestFile13, "ReadTest\\A\\file3" );
_LIT( KTestFile14, "ReadTest\\A\\file4" );
_LIT( KTestFile15, "ReadTest\\B\\B2\\testfile" );
_LIT( KTestFile16, "ReadTest\\E\\stream2" );

const TInt KTestFileCount = 16;

struct TestFiles
	{
	const TDesC*	iName;
	TInt			iLength;
	TUint			iSeed;
	};

const TestFiles TheFileArray[KTestFileCount] =
	{
		{ &KTestFile1, 256, 0xEF1113BC},
		{ &KTestFile2, 256, 0x04082195},
		{ &KTestFile3, 800, 0x42D4BF02},
		{ &KTestFile4, 304, 0x47C728FB},
		{ &KTestFile5, 30, 0x0CAAF228},
		{ &KTestFile6, 7000, 0x1128A9A5},
		{ &KTestFile7, 2000, 0x8DA9AA5A},
		{ &KTestFile8, 17466, 0x8DA9AA5A},
		{ &KTestFile9, 17466, 0x8DA9AA5A},
		{ &KTestFile10, 3, 0x8DA9AA5A},
		{ &KTestFile11, 7, 0x8DA9AA5A},
		{ &KTestFile12, 64, 0x8DA9AA5A},
		{ &KTestFile13, 256, 0xEC36D359},
		{ &KTestFile14, 256, 0x07D4DAC2},
		{ &KTestFile15, 500, 0x735AA240}, 
		{ &KTestFile16, 5000, 0x8DA9AA5A}
	};


_LIT( KDriveBase, " :\\" );




LOCAL_C void FillRandomBuffer( TDes8& aDes, TRandomGenerator& aRand, TInt aLength )
	{
	aDes.SetLength( aLength );
	TUint* ptr = (TUint*)aDes.Ptr();
	while( aLength >= 4 )
		{
		TUint v = aRand.Next();
		*ptr++ = v;
		aLength -= 4;
		}
	if( aLength )
		{
		TUint v = aRand.Next();
		TUint8* p8 = (TUint8*)ptr;
		while( aLength )
			{
			*p8++ = (TUint8)(v & 0xFF);
			v >>= 8;
			--aLength;
			}
		}
	}


LOCAL_C void Die()
	{
	_LIT( KCat, "Fail" );
	RProcess().Panic( KCat, 0 );
	//RProcess().Terminate(0);
	}


LOCAL_C void DoTestFile( TInt aFileIndex, TInt aThreadNumber )
	{
	RFile	file;
	TRandomGenerator	rand;

	TFileName name(KDriveBase);
	name[0] = TText('A' + gDriveNum);
	name.Append( *TheFileArray[aFileIndex].iName );
	
	_LIT( KOpeningMessage, "Thread %d opening file %S" );
	RDebug::Print( KOpeningMessage, aThreadNumber, &name );
		
	TInt r = file.Open( TheFs, name, EFileRead | EFileShareReadersOnly );
	if( KErrNone != r )
		{
		_LIT( KOpenFailMessage, "Thread %d open failed (%d)" );
		RDebug::Print( KOpenFailMessage, aThreadNumber, r );
		Die();
		}


	TInt fileLength;
	r = file.Size( fileLength );
	if( KErrNone != r )
		{
		_LIT( KSizeFailMessage, "Thread %d open failed (%d)" );
		RDebug::Print( KSizeFailMessage, aThreadNumber, r );
		Die();
		}

	if( fileLength != TheFileArray[aFileIndex].iLength )
		{
		_LIT( KLengthFailMessage, "Thread %d file length mismatch (%d %d)" );
		RDebug::Print( KLengthFailMessage, aThreadNumber, fileLength, TheFileArray[aFileIndex].iLength );
		Die();
		}
		
	const TInt KReadLen = 16;
	TBuf8<KReadLen> readBuf;
	TBuf8<KReadLen> randomBuf;

	rand.SetSeed( TheFileArray[aFileIndex].iSeed );

	TInt fragmentCount = (fileLength + KReadLen - 1) / KReadLen;
	TInt lengthRemaining = fileLength;

	for( TInt fragment = 0; fragment < fragmentCount; ++fragment )
		{
		TInt readLen = Min( KReadLen, lengthRemaining );

		// read next fragment from each file and compare
		FillRandomBuffer( randomBuf, rand, readLen );
		r = file.Read( readBuf );
		
		if( KErrNone != r )
			{
			_LIT( KReadFailMessage, "Thread %d read failed (%d)" );
			RDebug::Print( KReadFailMessage, aThreadNumber, r );
			Die();
			}
		if( readLen != readBuf.Length() )
			{
			_LIT( KReadLenFailMessage, "Thread %d read length mismatch (%d %d)" );
			RDebug::Print( KReadLenFailMessage, aThreadNumber, readLen, readBuf.Length() );
			Die();
			}

		if( readBuf != randomBuf )
			{
			_LIT( KCmpFailMessage, "Thread %d read data mismatch" );
			RDebug::Print( KCmpFailMessage, aThreadNumber );
			Die();
			}
		lengthRemaining -= readLen;
		}
	file.Close();
	}





LOCAL_C TInt SoakThread( TAny* aParam )
	//
	// Runs through the list of files opening and reading them
	// comparing against the random number generator
	//
	// aParam is the index in TheFileArray of the first file to test
	// and is also the thread number
	//
	{
	const TInt threadNumber = (TInt)aParam;
	TInt nextFileIndex = (3 * threadNumber) % KTestFileCount;

	TRandomGenerator randTime;
	randTime.SetSeed( 0xA4BB926A * nextFileIndex );

	for(;;)
		{
		DoTestFile( nextFileIndex, threadNumber );
		User::After( randTime.Next() % 250000 );
		nextFileIndex = (nextFileIndex + 1) % KTestFileCount;
		}
	}







//************************
// Entry point

void DoTestL(TInt aDriveToTest)
	{
	test.Title();
	test.Start( _L("Testing ROFS file reading") );

	gDriveNum = aDriveToTest;
	// Share file server session between all threads
	TheFs.ShareAuto();

	RThread thread1;
	RThread thread2;
	RThread thread3;
	RThread thread4;

	thread1.Create( _L("ROFSSOAK1"), SoakThread, 4096, 4096, 65536, (TAny*)0 );
	thread2.Create( _L("ROFSSOAK2"), SoakThread, 4096, 4096, 65536, (TAny*)1 );
	thread3.Create( _L("ROFSSOAK3"), SoakThread, 4096, 4096, 65536, (TAny*)2 );
	thread4.Create( _L("ROFSSOAK4"), SoakThread, 4096, 4096, 65536, (TAny*)3 );

	thread1.Resume();
	thread2.Resume();
	thread3.Resume();
	thread4.Resume();

	RThread().Suspend();
	}
