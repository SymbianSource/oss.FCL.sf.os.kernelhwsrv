// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\t_bytepair.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32math.h>
#include <e32rom.h>
#include <e32svr.h>
#include "decompress.h"

#define BYTE_PAIR_COMPRESS_INCLUDE_IMPLEMENTATION
#include <byte_pair_compress.h>

const TInt KMaxSize = 0x1000;
const TInt KPageSize = 0x1000;

RTest test(_L("T_BYTEPAIR"));
TUint8 InputBuffer[KMaxSize];
TUint8 CompressedBuffer[4*KMaxSize];
TUint8 OutputBuffer[KMaxSize+1];
TRomHeader* RomHeader = NULL;
TInt RomOffset = 0;
TInt FailCount = 0;
TUint32 RandomState;

void PrintHex(TUint8* aBuffer, TInt aSize)
	{
	const TInt KBytesPerLine = 38;
	TBuf<KBytesPerLine * 2 + 3> buf;
	for (TInt i = 0 ; i < aSize ; )
		{
		buf.Zero();
		buf.Append(_L("  "));
		TInt nextChunk = Min(aSize - i, KBytesPerLine);
		for (TInt j = 0 ; j < nextChunk ; ++j, ++i)
			buf.AppendFormat(_L("%02x"), aBuffer[i]);
		buf.Append(_L("\n"));
		RDebug::RawPrint(buf);
		}
	}

TUint32 Random()
	{
	RandomState = RandomState * 69069 + 1;
	return RandomState;
	}

typedef void (*TGenerator)(TUint8* aDest, TInt aSize);

void GenerateUniform(TUint8* aDest, TInt aSize)
	{
	TInt value = aSize & 255;
	Mem::Fill(aDest, aSize, value);
	}

void GenerateUniformRandom(TUint8* aDest, TInt aSize)
	{
	for (TInt i = 0 ; i < aSize ; ++i)
		aDest[i] = TUint8(Random());
	}

void GenerateZipfRandom(TUint8* aDest, TInt aSize)
	{
	// Some details from http://www.cs.hut.fi/Opinnot/T-106.4000/K2007/Ohjeet/Zipf.html
	const TInt max = 255;
	TReal c;
	test_KErrNone(Math::Log(c, max + 1.0));
	for (TInt i = 0 ; i < aSize ; ++i)
		{
		int r;
		do
			{
			TReal x = Random() / TReal(KMaxTUint32);
			test_KErrNone(Math::Exp(x, x * c));
			r = (int)x - 1;
			}
		while (r > max);
		aDest[i] = TUint8(r);
		}
	}

void GenerateRomPage(TUint8* aDest, TInt aSize)
	{
	if (TUint(RomOffset + aSize) > RomHeader->iUncompressedSize)
		RomOffset = 0;
	Mem::Copy(aDest, ((TUint8*)RomHeader) + RomOffset, aSize);
	RomOffset += KPageSize;
	}

enum TTestMode
	{
	ENormal,
	EOutputBufferTooLong,
	EOutputBufferTooShort,
	ETruncatedCompressedData,
	ECorruptCompressedData,
	ERandomCompressedData
	};

void TestCompressDecompress(TGenerator aGenFunc, TInt aSize, TTestMode aMode = ENormal)
	{
	ASSERT(aSize <= KMaxSize);
	
	TInt compressedSize;
	if (aMode != ERandomCompressedData)
		{
		// Prepare intput data
		aGenFunc(InputBuffer, aSize);
	
		// Compress input data
		compressedSize = BytePairCompress(CompressedBuffer, InputBuffer, aSize);
		ASSERT(compressedSize <= KMaxSize+1);
		}
	else
		{
		// Generate random compressed data
		compressedSize = aSize;
		GenerateUniformRandom(CompressedBuffer, compressedSize);
		}

	if (aMode == ETruncatedCompressedData)
		{
		// Truncate compressed data by up to half its length
		compressedSize -= Math::Random() % (compressedSize / 2);
		}
	else if (aMode == ECorruptCompressedData)
		{
		// Corrupt a random byte of the compressed data
		TInt pos = Random() % compressedSize;
		CompressedBuffer[pos] = TUint8(Random());
		}
	
	// Decomress compressed data
	Mem::Fill(OutputBuffer, KMaxSize+1, 0);
	TUint8* srcNext = NULL;
	TInt outputBufferSize = aSize;
	if (aMode == EOutputBufferTooLong || aMode == ERandomCompressedData)
		outputBufferSize = KMaxSize+1;
	else if (aMode == EOutputBufferTooShort)
		outputBufferSize = aSize / 2 + 1;
	TInt decompressedSize = BytePairDecompress(OutputBuffer, outputBufferSize, CompressedBuffer, compressedSize, srcNext);
	TInt srcUsed = srcNext ? srcNext - CompressedBuffer : 0;

	// Print stats
	RDebug::Printf("%d -> %d -> %d, %d, %d", aSize, compressedSize, outputBufferSize, srcUsed, decompressedSize);
	
	TBool ok = ETrue;

	// Check decompressed data not larger than output buffer
	if (decompressedSize > outputBufferSize)
		ok = EFalse;
	
	// Check output buffer not written beyond what was reported
	if (decompressedSize >= 0 && OutputBuffer[decompressedSize] != 0)
		ok = EFalse;

	if (aMode == ETruncatedCompressedData || aMode == ECorruptCompressedData || aMode == ERandomCompressedData)
		{
		// Input corrupt, expect error or partial sucess
		
		// If there was an error, check it was KErrCorrupt and srcNext was set to NULL
		if (decompressedSize < 0 && (decompressedSize != KErrCorrupt || srcNext != NULL))
			ok = EFalse;	
		}
	else if (aMode == EOutputBufferTooShort)
		{
		// Input consistent, output buffer too short

		// Expect error, or initial part correctly decompressed
		if (decompressedSize < 0)
			{
			if (decompressedSize != KErrCorrupt || srcNext != NULL)
				ok = EFalse;
			}
		else
			{
			if (decompressedSize > aSize  ||
				srcUsed > compressedSize ||
				Mem::Compare(InputBuffer, decompressedSize, OutputBuffer, decompressedSize) != 0)
				ok = EFalse;
			}
		}
	else
		{
		// Input consistent, expect success

		// Check no error, correct size, all compressed input used, and output same as orignal data
		if (decompressedSize < 0 ||
			aSize != decompressedSize ||
			srcUsed != compressedSize ||
			Mem::Compare(InputBuffer, decompressedSize, OutputBuffer, decompressedSize) != 0)
			ok = EFalse;		
		}

	if (!ok)
		{
		RDebug::Printf("Failure:");
		RDebug::Printf("Input");
		PrintHex(InputBuffer, aSize);
		RDebug::Printf("Compressed");
		PrintHex(CompressedBuffer, compressedSize);
		RDebug::Printf("Output");
		PrintHex(OutputBuffer, decompressedSize);
		++FailCount;
		}
	}

TInt E32Main()
//
// Benchmark for Mem functions
//
    {
	TInt i;
    test.Title();
    test.Start(_L("T_BYTEPAIR"));

	RandomState = User::FastCounter();
	RDebug::Printf("RandomState == %08x", RandomState);
	
	test_Equal(0, FailCount);

	const TInt KStartSize = KMaxSize / 2;

	// Test correct operation

	test.Next(_L("Test compressing uniform data"));	
	for (i = KStartSize ; i < KMaxSize ; i += 19)
		TestCompressDecompress(GenerateUniform, i);

	test.Next(_L("Test compressing uniformly distributed random data"));				
	for (i = KStartSize + 2 ; i < KMaxSize ; i += 19)
		TestCompressDecompress(GenerateUniformRandom, i);
	
	test.Next(_L("Test compressing zipf-distributed random data"));				
	for (i = KStartSize + 3 ; i < KMaxSize ; i += 19)
		TestCompressDecompress(GenerateZipfRandom, i);
	
#ifdef __EPOC32__
	RomHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
	TGenerator pageGen = GenerateRomPage;
#else
	TGenerator pageGen = GenerateZipfRandom;
#endif
	
	test.Next(_L("Test compressing pages"));				
	for (i = 0 ; i < 100 ; ++i)
		TestCompressDecompress(pageGen, KPageSize);

	// Test failure modes

	test.Next(_L("Test output buffer too short"));				
	for (i = KStartSize ; i < KMaxSize ; i += 19)
		TestCompressDecompress(pageGen, i, EOutputBufferTooShort);

	test.Next(_L("Test output buffer too long"));				
	for (i = KStartSize + 1 ; i < KMaxSize ; i += 19)
		TestCompressDecompress(pageGen, i, EOutputBufferTooLong);
	
	test.Next(_L("Test truncated compressed data"));				
	for (i = KStartSize + 2 ; i < KMaxSize ; i += 19)
		TestCompressDecompress(pageGen, i, ETruncatedCompressedData);
	
	test.Next(_L("Test corrupt compressed data "));				
	for (i = KStartSize + 3 ; i < KMaxSize ; i += 19)
		TestCompressDecompress(pageGen, i, ECorruptCompressedData);
	
	test.Next(_L("Test random compressed data"));				
	for (i = KStartSize + 4 ; i < KMaxSize ; i += 19)
		TestCompressDecompress(GenerateUniformRandom, i, ERandomCompressedData);

	test_Equal(0, FailCount);
	
    test.End();
	return(KErrNone);
    }
