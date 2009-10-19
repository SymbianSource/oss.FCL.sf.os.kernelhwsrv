// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\bench\t_desfnc.cpp
// 
//

#include "t_userbm.h"
#include <e32std.h>
#include <e32std_private.h>

_LIT(KCompare16, "abcdefghijklmnopqrstuvwxyz123456");
_LIT(KCompare16_2, "ABcdEfGhijklmnopqrstUVWxYZ123456");
_LIT(KMatch16, "qrstu");
_LIT(KMatch16_2, "qRStU");
const TDesC16& KString16 = KCompare16_2();
const TUint16* KCharData16 = KString16.Ptr();
TChar KChar;

_LIT8(KCompare8, "abcdefghijklmnopqrstuvwxyz123456");
_LIT8(KCompare8_2, "ABcdEfGhijklmnopqrstUVWxYZ123456");
_LIT8(KMatch8, "qrstu");
_LIT8(KMatch8_2, "qRStU");
const TDesC8& KString8 = KCompare8_2();
const TUint8* KCharData8 = KString8.Ptr();

void InitDataL()
	{
	}

// 16 bit descriptors

// Original t_desbm benchmarks first
DEFINE_USER_BENCHMARK(TDes16_Num,
					  TBuf16<32> buf,
					  buf.Num(0x35241525));

DEFINE_USER_BENCHMARK(TDes16_Num_RadixHex,
					  TBuf16<256> buf,
					  buf.Num(0x35241525,EHex));

DEFINE_USER_BENCHMARK(TDes16_Num_RadixDecimal,
					  TBuf16<256> buf,
					  buf.Num(0x35241525,EDecimal));

DEFINE_USER_BENCHMARK(TDes16_Num_RadixOctal,
					  TBuf16<256> buf,
					  buf.Num(0x35241525,EOctal));

DEFINE_USER_BENCHMARK(TDes16_Num_RadixBinary,
					  TBuf16<256> buf,
					  buf.Num(0x35241525,EBinary));

DEFINE_USER_BENCHMARK(TDesC16_Compare,
					  TBuf16<256> buf(KCompare16); TBuf16<256> str(KCompare16),
					  buf.Compare(str));

DEFINE_EXTRA_BENCHMARK(TDesC16_CompareC,
					   TBuf16<256> buf(KCompare16); TBuf16<256> str(KCompare16_2),
					   buf.CompareC(str));

DEFINE_USER_BENCHMARK(TDesC16_CompareF,
					  TBuf16<256> buf(KCompare16); TBuf16<256> str(KCompare16_2),
					  buf.CompareF(str));

DEFINE_USER_BENCHMARK(TDesC16_Match,
					  TBuf16<256> buf(KCompare16); TBuf16<256> str(KMatch16),
					  buf.Match(str));

DEFINE_USER_BENCHMARK(TDesC16_MatchF,
					  TBuf16<256> buf(KCompare16); TBuf16<256> str(KMatch16_2),
					  buf.MatchF(str));


DEFINE_EXTRA_BENCHMARK(TDes16_Append_TChar,
					   TBuf16<256> buf(KCompare16),
					   buf.SetLength(32); buf.Append(KChar));

DEFINE_EXTRA_BENCHMARK(TDes16_Append_TDesC16,
					   TBuf16<256> buf(KCompare16),
					   buf.SetLength(32); buf.Append(KString16));

DEFINE_EXTRA_BENCHMARK(TDes16_Append_pTUint16,
					   TBuf16<256> buf(KCompare16),
					   buf.SetLength(32); buf.Append(KCharData16, 32));

DEFINE_EXTRA_BENCHMARK(TDes16_Copy_TDesC16,
					   TBuf16<256> buf,
					   buf.Copy(KString16));

DEFINE_EXTRA_BENCHMARK(TDes16_Copy_pTUint16,
					   TBuf16<256> buf,
					   buf.Copy(KCharData16));

DEFINE_EXTRA_BENCHMARK(TDes16_Copy_pTUint16_TInt,
					   TBuf16<256> buf,
					   buf.Copy(KCharData16, 32));

DEFINE_EXTRA_BENCHMARK(TDes16_Fill,
					   TBuf16<256> buf(KCompare16),
					   buf.Fill(KChar));

DEFINE_EXTRA_BENCHMARK(TDes16_FillZ,
					   TBuf16<256> buf(KCompare16),
					   buf.FillZ());

DEFINE_EXTRA_BENCHMARK(TDes16_LeftTPtr,
					   TBuf16<256> buf(KCompare16),
					   buf.LeftTPtr(10));

DEFINE_EXTRA_BENCHMARK(TDes16_RightTPtr,
					   TBuf16<256> buf(KCompare16),
					   buf.RightTPtr(10));

DEFINE_EXTRA_BENCHMARK(TDes16_MidTPtr,
					   TBuf16<256> buf(KCompare16),
					   buf.MidTPtr(10));

DEFINE_EXTRA_BENCHMARK(TDes16_PtrZ,
					   TBuf16<256> buf(KCompare16),
					   buf.PtrZ());

DEFINE_EXTRA_BENCHMARK(TDes16_SetLength,
					   TBuf16<256> buf(KCompare16),
					   buf.SetLength(10));

DEFINE_EXTRA_BENCHMARK(TDes16_SetMax,
					   TBuf16<256> buf(KCompare16),
					   buf.SetMax());

DEFINE_EXTRA_BENCHMARK(TDes16_ZeroTerminate,
					   TBuf16<256> buf(KCompare16),
					   buf.ZeroTerminate());

DEFINE_EXTRA_BENCHMARK(TDesC16_AtC,
					   ,
					   KString16[10]);

DEFINE_EXTRA_BENCHMARK(TDesC16_Left,
					   ,
					   KString16.Left(10));

DEFINE_EXTRA_BENCHMARK(TDesC16_Right,
					   ,
					   KString16.Right(10));

DEFINE_EXTRA_BENCHMARK(TDesC16_Mid,
					   ,
					   KString16.Mid(10));

DEFINE_EXTRA_BENCHMARK(TDesC16_Locate,
					   ,
					   KString16.Locate(KChar));

DEFINE_EXTRA_BENCHMARK(TDesC16_LocateReverse,
					   ,
					   KString16.LocateReverse(KChar));

DEFINE_EXTRA_BENCHMARK(TDesC16_Ptr,
					   ,
					   KString16.Ptr());

DEFINE_EXTRA_BENCHMARK(TPtr16_TPtr16,
					   TPtr16 p(NULL, 0),
					   new (&p) TPtr16(NULL, 0));

DEFINE_EXTRA_BENCHMARK(TPtr16_TPtr16_2,
					   TPtr16 p(NULL, 0),
					   new (&p) TPtr16(NULL, 0, 0));

DEFINE_EXTRA_BENCHMARK(TPtrC16_TPtrC16,
					   TPtrC16 p,
					   new (&p) TPtrC16());

DEFINE_EXTRA_BENCHMARK(TPtrC16_TPtrC16_2,
					   TPtrC16 p,
					   new (&p) TPtrC16(KString16));

DEFINE_EXTRA_BENCHMARK(TPtrC16_TPtrC16_3,
					   TPtrC16 p,
					   new (&p) TPtrC16(KCharData16));

DEFINE_EXTRA_BENCHMARK(TPtrC16_TPtrC16_4,
					   TPtrC16 p,
					   new (&p) TPtrC16(KCharData16, 32));

DEFINE_EXTRA_BENCHMARK(TBufBase16_TBufBase16_1,
					   TBuf16<256> b,
					   new (&b) TBuf16<256>());

DEFINE_EXTRA_BENCHMARK(TBufBase16_TBufBase16_2,
					   TBuf16<256> b,
					   new (&b) TBuf16<256>(32));

DEFINE_EXTRA_BENCHMARK(TBufBase16_TBufBase16_3,
					   TBuf16<256> b,
					   new (&b) TBuf16<256>(KCharData16));

DEFINE_EXTRA_BENCHMARK(TBufBase16_TBufBase16_4,
					   TBuf16<256> b,
					   new (&b) TBuf16<256>(KString16));

DEFINE_EXTRA_BENCHMARK(TBufCBase16_TBufBaseC16_1,
					   TBufC16<256> b,
					   new (&b) TBufC16<256>());

DEFINE_EXTRA_BENCHMARK(TBufCBase16_TBufBaseC16_2,
					   TBufC16<256> b,
					   new (&b) TBufC16<256>(KCharData16));

DEFINE_EXTRA_BENCHMARK(TBufCBase16_TBufBaseC16_3,
					   TBufC16<256> b,
					   new (&b) TBufC16<256>(KString16));

// 8 bit descriptors

DEFINE_EXTRA_BENCHMARK(TDes8_Num,
					   TBuf8<32> buf,
					   buf.Num(0x35241525));

DEFINE_EXTRA_BENCHMARK(TDes8_Num_RadixHex,
					   TBuf8<256> buf,
					   buf.Num(0x35241525,EHex));

DEFINE_EXTRA_BENCHMARK(TDes8_Num_RadixDecimal,
					   TBuf8<256> buf,
					   buf.Num(0x35241525,EDecimal));

DEFINE_EXTRA_BENCHMARK(TDes8_Num_RadixOctal,
					   TBuf8<256> buf,
					   buf.Num(0x35241525,EOctal));

DEFINE_EXTRA_BENCHMARK(TDes8_Num_RadixBinary,
					   TBuf8<256> buf,
					   buf.Num(0x35241525,EBinary));

DEFINE_EXTRA_BENCHMARK(TDesC8_Compare,
					   TBuf8<256> buf(KCompare8); TBuf8<256> str(KCompare8),
					   buf.Compare(str));

DEFINE_EXTRA_BENCHMARK(TDesC8_CompareC,
					   TBuf8<256> buf(KCompare8); TBuf8<256> str(KCompare8_2),
					   buf.CompareC(str));

DEFINE_EXTRA_BENCHMARK(TDesC8_CompareF,
					   TBuf8<256> buf(KCompare8); TBuf8<256> str(KCompare8_2),
					   buf.CompareF(str));

DEFINE_EXTRA_BENCHMARK(TDesC8_Match,
					   TBuf8<256> buf(KCompare8); TBuf8<256> str(KMatch8),
					   buf.Match(str));

DEFINE_EXTRA_BENCHMARK(TDesC8_MatchF,
					   TBuf8<256> buf(KCompare8); TBuf8<256> str(KMatch8_2),
					   buf.MatchF(str));

DEFINE_EXTRA_BENCHMARK(TDes8_Append_TChar,
					   TBuf8<256> buf(KCompare8),
					   buf.SetLength(32); buf.Append(KChar));

DEFINE_EXTRA_BENCHMARK(TDes8_Append_TDesC8,
					   TBuf8<256> buf(KCompare8),
					   buf.SetLength(32); buf.Append(KString8));

DEFINE_EXTRA_BENCHMARK(TDes8_Append_pTUint8,
					   TBuf8<256> buf(KCompare8),
					   buf.SetLength(32); buf.Append(KCharData8, 32));

DEFINE_EXTRA_BENCHMARK(TDes8_Copy_TDesC8,
					   TBuf8<256> buf,
					   buf.Copy(KString8));

DEFINE_EXTRA_BENCHMARK(TDes8_Copy_pTUint8,
					   TBuf8<256> buf,
					   buf.Copy(KCharData8));

DEFINE_EXTRA_BENCHMARK(TDes8_Copy_pTUint8_TInt,
					   TBuf8<256> buf,
					   buf.Copy(KCharData8, 32));

DEFINE_EXTRA_BENCHMARK(TDes8_Fill,
					   TBuf8<256> buf(KCompare8),
					   buf.Fill(KChar));

DEFINE_EXTRA_BENCHMARK(TDes8_FillZ,
					   TBuf8<256> buf(KCompare8),
					   buf.FillZ());

DEFINE_EXTRA_BENCHMARK(TDes8_LeftTPtr,
					   TBuf8<256> buf(KCompare8),
					   buf.LeftTPtr(10));

DEFINE_EXTRA_BENCHMARK(TDes8_RightTPtr,
					   TBuf8<256> buf(KCompare8),
					   buf.RightTPtr(10));

DEFINE_EXTRA_BENCHMARK(TDes8_MidTPtr,
					   TBuf8<256> buf(KCompare8),
					   buf.MidTPtr(10));

DEFINE_EXTRA_BENCHMARK(TDes8_PtrZ,
					   TBuf8<256> buf(KCompare8),
					   buf.PtrZ());

DEFINE_EXTRA_BENCHMARK(TDes8_SetLength,
					   TBuf8<256> buf(KCompare8),
					   buf.SetLength(10));

DEFINE_EXTRA_BENCHMARK(TDes8_SetMax,
					   TBuf8<256> buf(KCompare8),
					   buf.SetMax());

DEFINE_EXTRA_BENCHMARK(TDes8_ZeroTerminate,
					   TBuf8<256> buf(KCompare8),
					   buf.ZeroTerminate());

DEFINE_EXTRA_BENCHMARK(TDesC8_AtC,
					   ,
					   KString8[10]);

DEFINE_EXTRA_BENCHMARK(TDesC8_Left,
					   ,
					   KString8.Left(10));

DEFINE_EXTRA_BENCHMARK(TDesC8_Right,
					   ,
					   KString8.Right(10));

DEFINE_EXTRA_BENCHMARK(TDesC8_Mid,
					   ,
					   KString8.Mid(10));

DEFINE_EXTRA_BENCHMARK(TDesC8_Locate,
					   ,
					   KString8.Locate(KChar));

DEFINE_EXTRA_BENCHMARK(TDesC8_LocateReverse,
					   ,
					   KString8.LocateReverse(KChar));

DEFINE_EXTRA_BENCHMARK(TDesC8_Ptr,
					   ,
					   KString8.Ptr());

DEFINE_EXTRA_BENCHMARK(TPtr8_TPtr8,
					   TPtr8 p(NULL, 0),
					   new (&p) TPtr8(NULL, 0));

DEFINE_EXTRA_BENCHMARK(TPtr8_TPtr8_2,
					   TPtr8 p(NULL, 0),
					   new (&p) TPtr8(NULL, 0, 0));

DEFINE_EXTRA_BENCHMARK(TPtrC8_TPtrC8,
					   TPtrC8 p,
					   new (&p) TPtrC8());

DEFINE_EXTRA_BENCHMARK(TPtrC8_TPtrC8_2,
					   TPtrC8 p,
					   new (&p) TPtrC8(KString8));

DEFINE_EXTRA_BENCHMARK(TPtrC8_TPtrC8_3,
					   TPtrC8 p,
					   new (&p) TPtrC8(KCharData8));

DEFINE_EXTRA_BENCHMARK(TPtrC8_TPtrC8_4,
					   TPtrC8 p,
					   new (&p) TPtrC8(KCharData8, 32));

DEFINE_EXTRA_BENCHMARK(TBufBase8_TBufBase8_1,
					   TBuf8<256> b,
					   new (&b) TBuf8<256>());

DEFINE_EXTRA_BENCHMARK(TBufBase8_TBufBase8_2,
					   TBuf8<256> b,
					   new (&b) TBuf8<256>(32));

DEFINE_EXTRA_BENCHMARK(TBufBase8_TBufBase8_3,
					   TBuf8<256> b,
					   new (&b) TBuf8<256>(KCharData8));

DEFINE_EXTRA_BENCHMARK(TBufBase8_TBufBase8_4,
					   TBuf8<256> b,
					   new (&b) TBuf8<256>(KString8));

DEFINE_EXTRA_BENCHMARK(TBufCBase8_TBufBaseC8_1,
					   TBufC8<256> b,
					   new (&b) TBufC8<256>());

DEFINE_EXTRA_BENCHMARK(TBufCBase8_TBufBaseC8_2,
					   TBufC8<256> b,
					   new (&b) TBufC8<256>(KCharData8));

DEFINE_EXTRA_BENCHMARK(TBufCBase8_TBufBaseC8_3,
					   TBufC8<256> b,
					   new (&b) TBufC8<256>(KString8));
