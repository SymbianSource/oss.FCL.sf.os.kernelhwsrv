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
// Tests reading from files
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32test.h>
#include <f32file.h>
#include "utl.h"
#include "randgen.h"

GLREF_D RFs TheFs;

RTest	test( _L("T_ROFSFILE") );


_LIT( KSpotFile1, "root.txt" );
_LIT( KSpotFile2, "Dir1\\level1.txt" );
_LIT( KSpotFile3, "Dir1\\Dir2\\level2.txt" );
_LIT( KSpotFile4, "Dir1\\Dir2\\Dir3\\level3.txt" );
_LIT( KSpotFile5, "Dir1\\Dir2\\Dir3\\Dir4\\level4.txt" );
_LIT( KSpotFile6, "Dir1\\Dir2\\Dir3\\Dir4\\Dir5\\level5.txt" );
_LIT( KSpotFile7, "Dir1\\Dir2\\Dir3\\Dir4\\Dir5\\Dir6\\level6.txt" );
_LIT( KSpotFile8, "Dir1\\Dir2\\Dir3\\Dir4\\Dir5\\Dir6\\Dir7\\level7.txt" );
_LIT( KSpotFile9, "DeepDir1\\DeepDir2\\DeepDir3\\DeepDir4\\DeepDir5\\DeepDir6\\DeepDir7\\DeepDir8\\DeepDir9\\DeepDir10\\DeepDir11\\DeepDir12\\DeepDir13\\DeepDir14\\file.txt" );
_LIT( KSpotFile10, "Parent\\parfile.txt" );
_LIT( KSpotFile11, "Parent\\SubDir1A\\subfileA.txt" );
_LIT( KSpotFile12, "Mixed\\par1.txt" );
_LIT( KSpotFile13, "Mixed\\SubDir1\\sub1.txt" );
_LIT( KSpotFile14, "Mixed\\SubDir2\\sub2.txt" );
_LIT( KSpotFile15, "Dir1\\level1_ext.txt" );


_LIT( KSameDirFile1, "ReadTest\\A\\file1" );
_LIT( KSameDirFile2, "ReadTest\\A\\file2" );
_LIT( KSameDirFile3, "ReadTest\\A\\file3" );
_LIT( KSameDirFile4, "ReadTest\\A\\file4" );
const TInt KSameDirFileLength = 256;
const TUint KSameDirFile1Seed = 0xEF1113BC;
const TUint KSameDirFile2Seed = 0x04082195;
const TUint KSameDirFile3Seed = 0xEC36D359;
const TUint KSameDirFile4Seed = 0x07D4DAC2;

_LIT( KSameNameFile1, "ReadTest\\B\\B1\\testfile" );
_LIT( KSameNameFile2, "ReadTest\\B\\B2\\testfile" );
_LIT( KSameNameFile3, "ReadTest\\B\\B3\\testfile" );
_LIT( KSameNameFile4, "ReadTest\\B\\B4\\testfile" );
const TInt KSameNameFileLength = 256;	// we will read this much of the file
const TUint KSameNameFile1Seed = 0x8DA9AA5A;
const TUint KSameNameFile2Seed = 0x735AA240;
const TUint KSameNameFile3Seed = 0x42D4BF02;
const TUint KSameNameFile4Seed = 0x47C728FB;

_LIT( KRandomReadFile, "ReadTest\\C\\seektest" );
const TInt KRandomReadFileSize = 17466;
const TUint KRandomReadFileSeed = 0x8DA9AA5A;

_LIT( KEofFile1, "ReadTest\\D\\eoftest1" );
_LIT( KEofFile2, "ReadTest\\D\\eoftest2" );
_LIT( KEofFile3, "ReadTest\\D\\eoftest3" );
_LIT( KEofFile4, "ReadTest\\D\\eoftest4" );
_LIT( KEofFile5, "ReadTest\\D\\eoftest5" );
_LIT( KEofFile6, "ReadTest\\D\\eoftest6" );


_LIT( KStreamFile1, "ReadTest\\E\\stream1" );
_LIT( KStreamFile2, "ReadTest\\E\\stream2" );
_LIT( KStreamFile3, "ReadTest\\E\\stream3" );
_LIT( KStreamFile4, "ReadTest\\E\\stream4" );

_LIT( KTestFileExt, "ext.txt" );

const TUint KStreamReadSeed = 0x8DA9AA5A;

_LIT( KDriveBase, " :\\" );


const TInt KHello8FileSize = 5;
_LIT8( KHello8FileContent, "hello" );


// A large buffer for file reads
LOCAL_D TBuf8<18000> gFileBuffer;


LOCAL_C void SpotCheckFilesL(TInt aDriveToTest, TBool aExtension)
//
// Tests a few files for size & content. These are all the hello8.txt test file
//
	{
	const TDesC* const fileArray[15] =
		{
		&KSpotFile1, &KSpotFile2, &KSpotFile3, &KSpotFile4, &KSpotFile5,
		&KSpotFile6, &KSpotFile7, &KSpotFile8, &KSpotFile9, &KSpotFile10,
		&KSpotFile11, &KSpotFile12, &KSpotFile13, &KSpotFile14, &KSpotFile15
		};

	test.Next( _L("Spot-check some files") );
	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);

	TInt index =0;
	if(aExtension) index =1;
	for( TInt i = index; i < 14; i++ )
		{
		name.SetLength( 3 );	// trim back to drive specifier
		if(aExtension &&(i==1))
			name.Append( *fileArray[i+13] );
		else
			name.Append( *fileArray[i] );

		test.Printf( _L("Opening file %S"), &name );
		RFile file;
		TInt r = file.Open( TheFs, name, EFileRead );
		TEST_FOR_ERROR( r );

		TInt fileSize;
		r = file.Size( fileSize );
		TEST_FOR_ERROR( r );
		TEST_FOR_MATCH( fileSize, KHello8FileSize );

		TBuf8<5> buf;
		r = file.Read( buf );
		TEST_FOR_ERROR( r );
		TEST_FOR_MATCH( buf.Length(), KHello8FileSize );
		test(buf == KHello8FileContent);
		file.Close();
		}
	}


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

LOCAL_C TBool ValidateRandomBuffer( const TDes8& aDes, TUint aSeed )
	{
	const TUint* ptr = (const TUint*)aDes.Ptr();
	TInt length = aDes.Length();
	
	TRandomGenerator rand;
	rand.SetSeed( aSeed );
	while( length >= 4 )
		{
		TUint v = rand.Next();
		if( *ptr++ != v )
			{
			return EFalse;
			}
		length -= 4;
		}
	if( length )
		{
		TUint v = rand.Next();
		TUint8* p8 = (TUint8*)ptr;
		while( length )
			{
			if( *p8++ != (TUint8)(v & 0xFF) )
				{
				return EFalse;
				}
			v >>= 8;
			--length;
			}
		}
	return ETrue;
	}



LOCAL_C void TestFilesInSameDirectoryL(TInt aDriveToTest)
	//
	// Tests opening different files in the same directory
	//
	{
	test.Next( _L("Testing files in same directory") );
	
	struct TTheFiles
		{
		RFile	iFile;
		TRandomGenerator	iRand;
		};
	TTheFiles files[4];

	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);
	
	name.Append( KSameDirFile1 );
	TInt r = files[0].iFile.Open( TheFs, name, EFileRead );
	TEST_FOR_ERROR( r );

	name.SetLength( 3 );
	name.Append( KSameDirFile2 );
	r = files[1].iFile.Open( TheFs, name, EFileRead );
	TEST_FOR_ERROR( r );

	name.SetLength( 3 );
	name.Append( KSameDirFile3 );
	r = files[2].iFile.Open( TheFs, name, EFileRead );
	TEST_FOR_ERROR( r );

	name.SetLength( 3 );
	name.Append( KSameDirFile4 );
	r = files[3].iFile.Open( TheFs, name, EFileRead );
	TEST_FOR_ERROR( r );

	const TInt KReadLen = KSameDirFileLength / 16;
	TBuf8<KReadLen> readBuf;
	TBuf8<KReadLen> randomBuf;

	files[0].iRand.SetSeed( KSameDirFile1Seed );
	files[1].iRand.SetSeed( KSameDirFile2Seed );
	files[2].iRand.SetSeed( KSameDirFile3Seed );
	files[3].iRand.SetSeed( KSameDirFile4Seed );

	for( TInt fragment = 0; fragment < (KSameDirFileLength / KReadLen); ++fragment )
		{
		// read next fragment from each file and compare
		for( TInt j = 0; j < 4; ++j )
			{
			FillRandomBuffer( randomBuf, files[j].iRand, KReadLen );
			r = files[j].iFile.Read( readBuf );
			TEST_FOR_ERROR( r );
			TEST_FOR_MATCH( readBuf.Length(), KReadLen );
			test( readBuf == randomBuf );
			}
		}
	files[0].iFile.Close();
	files[1].iFile.Close();
	files[2].iFile.Close();
	files[3].iFile.Close();
	}


LOCAL_C void TestFilesSameNameDifferentDirectoryL(TInt aDriveToTest)
	//
	// Tests opening files with the same name but in different directories
	//
	{
	test.Next( _L("Testing files with same name in different directories") );
	
	struct TTheFiles
		{
		RFile	iFile;
		TRandomGenerator	iRand;
		};
	TTheFiles files[4];

	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);
	
	name.Append( KSameNameFile1 );
	TInt r = files[0].iFile.Open( TheFs, name, EFileRead );
	TEST_FOR_ERROR( r );

	name.SetLength( 3 );
	name.Append( KSameNameFile2 );
	r = files[1].iFile.Open( TheFs, name, EFileRead );
	TEST_FOR_ERROR( r );

	name.SetLength( 3 );
	name.Append( KSameNameFile3 );
	r = files[2].iFile.Open( TheFs, name, EFileRead );
	TEST_FOR_ERROR( r );

	name.SetLength( 3 );
	name.Append( KSameNameFile4 );
	r = files[3].iFile.Open( TheFs, name, EFileRead );
	TEST_FOR_ERROR( r );

	const TInt KReadLen = KSameNameFileLength / 16;
	TBuf8<KReadLen> readBuf;
	TBuf8<KReadLen> randomBuf;

	files[0].iRand.SetSeed( KSameNameFile1Seed );
	files[1].iRand.SetSeed( KSameNameFile2Seed );
	files[2].iRand.SetSeed( KSameNameFile3Seed );
	files[3].iRand.SetSeed( KSameNameFile4Seed );

	for( TInt fragment = 0; fragment < (KSameDirFileLength / KReadLen); ++fragment )
		{
		// read next fragment from each file and compare
		for( TInt j = 0; j < 4; ++j )
			{
			FillRandomBuffer( randomBuf, files[j].iRand, KReadLen );
			r = files[j].iFile.Read( readBuf );
			TEST_FOR_ERROR( r );
			TEST_FOR_MATCH( readBuf.Length(), KReadLen );
			test( readBuf == randomBuf );
			}
		}
	files[0].iFile.Close();
	files[1].iFile.Close();
	files[2].iFile.Close();
	files[3].iFile.Close();
	}




LOCAL_C void TestRandomSeekL(TInt aDriveToTest)
	//
	// Tests reading from random positions in a file
	//
	{
	test.Next( _L("Testing random read position") );
	
	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);
	
	name.Append( KRandomReadFile );
	RFile file;
	TInt r = file.Open( TheFs, name, EFileRead );
	TEST_FOR_ERROR( r );

	// Check file size
	TInt fileSize;
	r = file.Size( fileSize ), 
	TEST_FOR_ERROR( r );
	TEST_FOR_MATCH( fileSize, KRandomReadFileSize );

	// Fill buffer with expected file contents
	TRandomGenerator rand;
	rand.SetSeed( KRandomReadFileSeed );
	FillRandomBuffer( gFileBuffer, rand, KRandomReadFileSize );


	// Read from random positions within the file
	rand.SetSeed( 0xA6E320F1 );
	const TInt KMaxRead = 256;
	TBuf8<KMaxRead> buf;
	const TInt maxRead = Min( fileSize, buf.MaxLength() );
	for( TInt readCount = 0; readCount < 100; ++readCount )
		{
		TInt readLen = rand.Next() % (maxRead+1);
		if( readLen > buf.MaxLength() )
			{
			readLen = buf.MaxLength();
			}
		TInt readPos = rand.Next() % (fileSize - readLen);

		_LIT( KMessage, "Reading %d bytes from 0x%x" );
		test.Printf( KMessage, readLen, readPos );
		
		const TUint8* pExpected = gFileBuffer.Ptr() + readPos;
		r = file.Seek( ESeekStart, readPos );
		TEST_FOR_ERROR( r );

		r = file.Read( buf, readLen );
		TEST_FOR_ERROR( r );
		TEST_FOR_MATCH( buf.Length(), readLen );

		const TUint8* pRead = buf.Ptr();
		test( 0 == Mem::Compare( pExpected, readLen, pRead, readLen ) );
		}
	}


LOCAL_C void TestEofReadL(TInt aDriveToTest)
	//
	// Test reading off the end of a file
	//
	{
	test.Next( _L("Testing reading past EOF") );

	
	const TDesC* const fileArray[6] =
		{
		&KEofFile1, &KEofFile2, &KEofFile3, &KEofFile4, &KEofFile5, &KEofFile6
		};

	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);

	TRandomGenerator rand;
	rand.SetSeed( 0x1001A005 );
	for( TInt i = 0; i < 6; i++ )
		{
		name.SetLength( 3 );	// trim back to drive specifier
		name.Append( *fileArray[i] );
		test.Printf( _L("Opening file %S"), &name );
		RFile file;
		TInt r = file.Open( TheFs, name, EFileRead );
		TEST_FOR_ERROR( r );
		
		TInt fileSize;
		r = file.Size( fileSize ), 
		TEST_FOR_ERROR( r );
		test( fileSize > 0 );
		
		TInt readPos = fileSize - ( rand.Next() % 12);
		if( readPos < 0 )
			{
			readPos = 0;
			}
		r = file.Seek( ESeekStart, readPos );
		TEST_FOR_ERROR( r );
		
		TBuf8<32> buf;
		r = file.Read( buf );
		TEST_FOR_ERROR( r );
		TEST_FOR_MATCH( buf.Length(), fileSize - readPos );
		file.Close();
		}
	
	}



LOCAL_C void TestStreamReadL(TInt aDriveToTest)
	//
	// Reads the whole file into a buffer in a single read
	// and validates the data
	//
	{
	test.Next( _L("Testing whole file read") );

	const TDesC* const fileArray[4] =
		{
		&KStreamFile1, &KStreamFile2, &KStreamFile3, &KStreamFile4
		};
	
	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);

	for( TInt i = 0; i < 4; ++i )
		{
		name.SetLength(3);
		name.Append( *fileArray[i] );

		test.Printf( _L("... file %S"), &name );
		
		RFile file;
		TInt r = file.Open( TheFs, name, EFileRead );
		TEST_FOR_ERROR( r );

		TInt fileSize;
		r = file.Size( fileSize ), 
		TEST_FOR_ERROR( r );
		test( fileSize > 0 );

		r = file.Read( gFileBuffer );
		TEST_FOR_ERROR( r );
		TEST_FOR_MATCH( fileSize, gFileBuffer.Length() );

		test( ValidateRandomBuffer( gFileBuffer, KStreamReadSeed ) );

		file.Close();
		}
	}

//************************
// Entry point

void DoTestL(TInt aDriveToTest)
	{
	test.Title();
	test.Start( _L("Testing ROFS file reading") );
	
	test.Printf( _L("Looking for ROFS extension\n"));
	TBool extension = EFalse;
	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);

	name.SetLength( 3 );	// trim back to drive specifier
	name.Append( KTestFileExt );

	RFile file;
	test.Printf( _L("Opening file %S"), &name );
	TInt r = file.Open( TheFs, name, EFileRead );
	if(r==KErrNone)
		{
		extension=ETrue;
		file.Close();
		test.Printf( _L("ROFS extension found\n"));
		}
	else if(r==KErrNotFound)
		{
		test.Printf( _L("Not found, ROFS extension not present\n"));
		}

	SpotCheckFilesL(aDriveToTest, extension);
	TestFilesInSameDirectoryL(aDriveToTest);
	TestFilesSameNameDifferentDirectoryL(aDriveToTest);
	TestRandomSeekL(aDriveToTest);
	TestEofReadL(aDriveToTest);
	TestStreamReadL(aDriveToTest);

	test.End();
	}
