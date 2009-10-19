// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\math\t_vals.h
// 
//

#include <e32std.h>

class TInt32x
	{
public:
	TInt32x(TInt32 aInt) {iInt=aInt;}
public:
	TInt32 iInt;
	};

#if defined  (__DOUBLE_WORDS_SWAPPED__)
#define TINT64(a,b) MAKE_TINT64(b,a)
#else	// for WINS and X86 (little-endian doubles) 
#define TINT64(a,b) MAKE_TINT64(a,b)
#endif

const TInt64 minTReal32in64 = TINT64(0x38100000,0x0);
const TInt64 maxTReal32in64 = TINT64(0x47efffff,0xe0000000);
const TInt64 sqrtMaxTReal64 = TINT64(0x5fefffff,0xffd9c605);
const TInt64 sqrtMinTReal64 = TINT64(0x20000000,0x0);
const TInt64 negZeroTReal64 = TINT64(0x80000000,0x0);
const TInt64 posInfTReal64 = TINT64(0x7ff00000,0x0);
const TInt64 negInfTReal64 = TINT64(0xfff00000,0x0);
const TInt64 NaNTReal64 = TINT64(0x7fffffff,0xffffffff);
const TInt64 K1EMinus322 = TINT64(0x00000000, 0x00000014);
const TInt64 K1EMinus313 = TINT64(0x00000004, 0xB6695433);
const TInt64 K5EMinus324 = TINT64(0x00000000, 0x00000001);
const TInt64 K4EMinus320 = TINT64(0x00000000, 0x00001FA0);
const TInt64 K1EMinus310 = TINT64(0x00001268, 0x8B70E62B);
const TInt64 K1EMinus323 = TINT64(0x00000000, 0x00000002);
const TInt64 K1Point2EMinus320 = TINT64(0x00000000, 0x0000097D);
const TInt64 K5EMinus321 = TINT64(0x00000000, 0x000003F4);
const TInt64 K2EMinus321 = TINT64(0x00000000, 0x00000195);
const TInt64 K1Point234EMinus316 = TINT64(0x000000000, 0x017D1C36);
const TInt64 K1EMinus324 = TINT64(0x000000000, 0x00000000);

const TInt32x sqrtMaxTReal32 = TInt32x(0x5f7ffffd);
const TInt32x sqrtMinTReal32 = TInt32x(0x20000000);
const TInt32x negZeroTReal32 = TInt32x(0x80000000);
const TReal64 KMinTReal32in64 = *(TReal64*)&minTReal32in64;
const TReal64 KMaxTReal32in64 = *(TReal64*)&maxTReal32in64;
const TReal64 KSqrtMaxTReal64 = *(TReal64*)&sqrtMaxTReal64;
const TReal64 KSqrtMinTReal64 = *(TReal64*)&sqrtMinTReal64;
const TReal64 KNegZeroTReal64 = *(TReal64*)&negZeroTReal64;
const TReal64 KPosInfTReal64 = *(TReal64*)&posInfTReal64;
const TReal64 KNegInfTReal64 = *(TReal64*)&negInfTReal64;
const TReal64 KNaNTReal64 = *(TReal64*)&NaNTReal64;
const TReal32 KSqrtMaxTReal32 = *(TReal32*)&sqrtMaxTReal32;
const TReal32 KSqrtMinTReal32 = *(TReal32*)&sqrtMinTReal32;
const TReal32 KNegZeroTReal32 = *(TReal32*)&negZeroTReal32;
const TReal KMinTReal32inTReal = TReal(KMinTReal32in64);
const TReal KMaxTReal32inTReal = TReal(KMaxTReal32in64);
const TReal KNegZeroTReal = TReal(KNegZeroTReal64);
const TReal64 K1EMinus322Real64 = *(TReal64*)&K1EMinus322;
const TReal64 K1EMinus313Real64 = *(TReal64*)&K1EMinus313;
const TReal64 K5EMinus324Real64 = *(TReal64*)&K5EMinus324;
const TReal64 K4EMinus320Real64 = *(TReal64*)&K4EMinus320;
const TReal64 K1EMinus310Real64 = *(TReal64*)&K1EMinus310;
const TReal64 K1EMinus323Real64 = *(TReal64*)&K1EMinus323;
const TReal64 K1Point2EMinus320Real64 = *(TReal64*)&K1Point2EMinus320;
const TReal64 K5EMinus321Real64 = *(TReal64*)&K5EMinus321;
const TReal64 K2EMinus321Real64 = *(TReal64*)&K2EMinus321;
const TReal64 K1Point234EMinus316Real64 = *(TReal64*)&K1Point234EMinus316;
const TReal64 K1EMinus324Real64 = *(TReal64*)&K1EMinus324;


