// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_deflate.h
// 
//

#ifndef __SF_DEFLATE_H__
#define __SF_DEFLATE_H__
#include "e32huffman.h"
#include <e32base.h>
#include <f32file.h>
typedef TUint8* (*TMemoryMoveFunction)(TAny* aTrg,const TAny* aSrc,TInt aLength);

#define __CONFIGURABLE_F32_LOADER_INFLATE_WINDOW_SIZE__ 0x8000

// deflation constants
const TInt KDeflateLengthMag=8;
const TInt KDeflateDistanceMag=12;
//
const TInt KDeflateMinLength=3;
const TInt KDeflateMaxLength=KDeflateMinLength-1 + (1<<KDeflateLengthMag);
const TInt KDeflateMaxDistance=(1<<KDeflateDistanceMag);
const TInt KDeflateDistCodeBase=0x200;
// hashing
const TUint KDeflateHashMultiplier=0xAC4B9B19u;
const TInt KDeflateHashShift=24;
// inflate
const TInt KInflateWindowSize=__CONFIGURABLE_F32_LOADER_INFLATE_WINDOW_SIZE__ ;

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

NONSHARABLE_CLASS(CInflater) : public CBase
	{
public:
	enum {EBufSize = 0x800, ESafetyZone=8};
public:
	static CInflater* NewLC(TBitInput& aInput);
	~CInflater();
//
	TInt ReadL(TUint8* aBuffer,TInt aLength, TMemoryMoveFunction aMemMovefn);
	TInt SkipL(TInt aLength);
private:
	CInflater(TBitInput& aInput);
	void ConstructL();
	void InitL();
	TInt InflateL();
private:
	TBitInput* iBits;
	const TUint8* iRptr;			// partial segment
	TInt iLen;
	const TUint8* iAvail;			// available data
	const TUint8* iLimit;
	TEncoding* iEncoding;
	TUint8* iOut;					// circular buffer for distance matches
	};

void DeflateL(const TUint8* aBuf, TInt aLength, TBitOutput& aOutput);

NONSHARABLE_CLASS(TFileInput) : public TBitInput
	{
 	enum {KBufSize=KInflateWindowSize};
public:
	TFileInput(RFile& aFile);
	void Cancel();
private:
	void UnderflowL();
private:
	RFile& iFile;
	TRequestStatus iStat;
	TUint8* iReadBuf;
	TPtr8 iPtr;
	TUint8 iBuf1[KBufSize];
	TUint8 iBuf2[KBufSize];
	};

#endif
