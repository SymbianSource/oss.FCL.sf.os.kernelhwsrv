// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\t_wsd_dl3.cpp
//

// MMH file will define	T_WSD_DL3
#include "t_wsd_tst.h"

#define C 		((void*)DummyFn)
#define D 		((void*)TestData)
#define A4W		C, D, C, D												// 4 words
#define A16W	A4W, A4W, A4W, A4W										// 16 words
#define A64W 	A16W, A16W, A16W, A16W									// 64 words
#define A256W 	A64W, A64W, A64W, A64W									// 256 words
#define A1KW	A256W, A256W, A256W, A256W								// 1Kwords
#define A4KW	A1KW, A1KW, A1KW, A1KW									// 4Kwords

// Non-exported dummy function whose address is put in the initialised
// data, to check that it gets relocated correctly
void DummyFn(void)
	{
	}

// Non-exported const data, which should end up in the text segment
// but doesn't on some platforms :(
static const void* const TestData[] = { A4KW, A4KW };					// 8Kwords
const TInt KSizeOfTestData = sizeof(TestData);							// 32Kbytes

// Exported Ordinals 2-5
EXPORT_D TInt32 TestDataSize = KSizeOfTestData;
EXPORT_D void* WritableTestData[] = { A4KW, A4KW };
EXPORT_D const void* const* PointerToStaticData = TestData;
EXPORT_D void** PointerToWritableData = WritableTestData;

// Dummy function, just so that this DLL contains more than one page of
// text even on platforms where the read-only data ends up in the data
// rather than the text segment ...
TUint AddEmUp()
	{
	const void* const* p = TestData;
	TUint sum = 0;

#define	ADD1W	{ sum += (TUint)*p++; }									// 1 word
#define	ADD4W	{ ADD1W; ADD1W; ADD1W; ADD1W; }							// 4 words
#define	ADD16W	{ ADD4W; ADD4W; ADD4W; ADD4W; }							// 16 words
#define	ADD64W	{ ADD16W; ADD16W; ADD16W; ADD16W; }						// 64 words
#define	ADD256W	{ ADD64W; ADD64W; ADD64W; ADD64W; }						// 256 words
#define	ADD1KW	{ ADD256W; ADD256W; ADD256W; ADD256W; }					// 1K words

	// The macro ADD1KW should expand to ~2K instructions i.e. ~8Kb of inline code
	for (TUint i = 0; i < sizeof(TestData)/(1024*sizeof(void*)); ++i)
		ADD1KW;

	// We've added up all 8Kwords (32kb) of the test data ...
	return sum;
	}

// Exported Ordinal 1
EXPORT_C void** GetAddressOfData(TInt& aSize, void*& aCodeAddr, void*& aDataAddr)
	{
	TAny* p = User::Alloc(KSizeOfTestData);
	aSize = AddEmUp();
	aSize = KSizeOfTestData;
	aCodeAddr = C;
	aDataAddr = D;
	User::Free(p);
	return (void**)&PointerToStaticData;
	}

