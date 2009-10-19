// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// base\omap_hrp\h4_bootloader\inflate2.h
// 
//


#include <f32file.h>

#ifndef __INFLATE2_H__ 
#define __INFLATE2_H__

#define __CONFIGURABLE_F32_LOADER_INFLATE_WINDOW_SIZE__ 0x8000

// inflate
const TInt KInflateWindowSize=__CONFIGURABLE_F32_LOADER_INFLATE_WINDOW_SIZE__ ;


typedef struct
	{
	TUint				iPhysicalSector;
	TUint				iSemiPhysicalSector;	
	} TNandReadInfo;

//for asm mem copy
//#define __JUMP(cc,r) asm("mov"#cc " pc, "#r )
//#define __POPRET(rlist) asm("ldmfd sp!, {"##rlist##"pc} ")

void memcpy1(TAny*, const TAny*, TUint);
void memset1(void *, int, unsigned);
TInt memcmp1(const TUint8* aTrg, const TUint8* aSrc, TInt aLength);

void leds(TUint32); 
extern "C" void memdump(TUint32* aAddr, TUint32* aEnd);

#ifdef __cplusplus
extern "C" {
#endif
extern void countout(void);
extern void charout(TUint8 aChar);

extern void WriteW(TUint32);
extern void WriteB(TUint8);
extern void mmuoff(void);
#ifdef __cplusplus
}
#endif



/** Bit input stream. Good for reading bit streams for packed, compressed or huffman
	data algorithms.
*/
class TBitInput
	{
public:
	TBitInput();
	TBitInput(const TUint8* aPtr, TInt aLength, TInt aOffset=0);
	void Set(const TUint8* aPtr, TInt aLength, TInt aOffset=0);
//
	TUint ReadL();
	TUint ReadL(TInt aSize);
	TUint HuffmanL(const TUint32* aTree);
private:
	virtual void UnderflowL();
private:
    TInt iCount;
	TUint iBits;
	TInt iRemain;
	const TUint32* volatile iPtr;
	};

const TInt KHuffTerminate=0x0001;
const TUint32 KBranch1=sizeof(TUint32)<<16;


/** Huffman code toolkit.

	This class builds a huffman encoding from a frequency table and builds
	a decoding tree from a code-lengths table

	The encoding generated is based on the rule that given two symbols s1 and s2, with 
	code length l1 and l2, and huffman codes h1 and h2:

		if l1<l2 then h1<h2 when compared lexicographically
		if l1==l2 and s1<s2 then h1<h2 ditto

	This allows the encoding to be stored compactly as a table of code lengths
*/
class Huffman
	{
public:
	enum {KMaxCodeLength=27};
	enum {KMetaCodes=KMaxCodeLength+1};
	enum {KMaxCodes=0x8000};
public:
	static void Decoding(const TUint32 aHuffman[],TInt aNumCodes,TUint32 aDecodeTree[],TInt aSymbolBase=0);
	static TBool IsValid(const TUint32 aHuffman[],TInt aNumCodes);
//
	static void InternalizeL(TBitInput& aInput,TUint32 aHuffman[],TInt aNumCodes);
	};


// deflation constants
const TInt KDeflateLengthMag=8;
const TInt KDeflateDistanceMag=12;

const TInt KDeflateMinLength=3;
const TInt KDeflateMaxLength=KDeflateMinLength-1 + (1<<KDeflateLengthMag);
const TInt KDeflateMaxDistance=(1<<KDeflateDistanceMag);
const TInt KDeflateDistCodeBase=0x200;


class TEncoding
	{
public:
	enum {ELiterals=256,ELengths=(KDeflateLengthMag-1)*4,ESpecials=1,EDistances=(KDeflateDistanceMag-1)*4};
	enum {ELitLens=ELiterals+ELengths+ESpecials};
	enum {EEos=ELiterals+ELengths};
public:
	TUint32 iLitLen[ELitLens];
	TUint32 iDistance[EDistances];
	};

const TInt KDeflationCodes=TEncoding::ELitLens+TEncoding::EDistances;

class Inflater
	{
public:
	static TInt Inflate(TBitInput& aBits, TUint8* aBuffer, TInt aSize);
private:
	static TInt Init(TBitInput& aBits, TEncoding& aEncoding);
	static TInt DoInflate(TBitInput& aBits, TEncoding& aEncoding, TUint8* aBuffer, TInt aSize);
	};


class TFileInput : public TBitInput
	{
 	enum {KBufSize=KInflateWindowSize};

public:
	TFileInput(TInt aBlockLen, TInt aFileSize);
	void Init(void);

private:
	void UnderflowL();

private:
	TUint8* iReadBuf;
	TPtr8   iPtr;
	TUint8  iBuf1[KBufSize];
	TInt    iState;
	TInt    iBlockLen;
	TInt    iFileSize;
	TInt    iImageReadProgress;
	};



#endif
