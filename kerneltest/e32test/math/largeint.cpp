// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\math\largeint.cpp
// 
//

#include "largeint.h"

TLargeIntBase::TLargeIntBase(TInt n)
	: iC(n)
	{
	Mem::FillZ(iX,n*sizeof(TUint32));
	}

TLargeIntBase::TLargeIntBase(TInt n, TInt32 aSigned32)
	: iC(n)
	{
	TUint f = (aSigned32<0) ? 0xff : 0;
	Mem::Fill(iX,n*sizeof(TUint32),f);
	iX[0]=aSigned32;
	}

TLargeIntBase::TLargeIntBase(TInt n, TUint32 aUnsigned32)
	: iC(n)
	{
	Mem::FillZ(iX,n*sizeof(TUint32));
	iX[0]=aUnsigned32;
	}

TLargeIntBase::TLargeIntBase(TInt n, const TLargeIntBase& aSrc, TMode aMode)
	: iC(n)
	{
	__ASSERT(aMode==ETruncate||n>=aSrc.iC);	// if not truncating, dest can't be shorter than source
	__ASSERT(aMode!=ETruncate||n<=aSrc.iC);	// if truncating, dest can't be longer than source
	TInt min = Min(n,aSrc.iC);
	TInt i;
	for (i=0; i<min; ++i)
		iX[i] = aSrc.iX[i];
	if (aMode==ETruncate || n==aSrc.iC)
		return;
	TUint32 f = (aMode==ESignExtend && (iX[i-1] & 0x80000000u)) ? 0xffffffffu : 0;
	for (; i<n; ++i)
		iX[i] = f;
	}

TLargeIntBase::TLargeIntBase(TInt n, const TUint32* aPtr)
	: iC(n)
	{
	Mem::Copy(iX, aPtr, n*sizeof(TUint32));
	}

TLargeIntBase::TLargeIntBase(TInt n, const Int64& aSigned64)
	: iC(n)
	{
	__ASSERT(n>=2);
	Mem::Copy(iX, &aSigned64, 8);
	TUint f = (iX[1] & 0x80000000u) ? 0xff : 0;
	Mem::Fill(iX+2,(n-2)*sizeof(TUint32),f);
	}

TLargeIntBase::TLargeIntBase(TInt n, const Uint64& aUnsigned64)
	: iC(n)
	{
	__ASSERT(n>=2);
	Mem::Copy(iX, &aUnsigned64, 8);
	Mem::FillZ(iX+2,(n-2)*sizeof(TUint32));
	}

void TLargeIntBase::Not()
	{
	TInt i;
	for (i=0; i<iC; ++i)
		iX[i] = ~iX[i];
	}

void TLargeIntBase::Neg()
	{
	Not();
	Inc();
	}

void TLargeIntBase::Abs()
	{
	if (iX[iC-1] & 0x80000000u)
		Neg();
	}

void TLargeIntBase::Inc()
	{
	TInt i;
	for (i=0; i<iC && ++iX[i]==0; ++i) {}
	}

void TLargeIntBase::Dec()
	{
	TInt i;
	for (i=0; i<iC && --iX[i]==0xffffffffu; ++i) {}
	}

TUint32 TLargeIntBase::Lsl()
	{
	TInt i;
	TUint32 c = 0;
	for (i=0; i<iC; ++i)
		{
		TUint32 x = (iX[i]<<1) | (c>>31);
		c = iX[i];
		iX[i] = x;
		}
	return c>>31;
	}

TUint32 TLargeIntBase::Lsr()
	{
	TInt i;
	TUint32 c = 0;
	for (i=iC-1; i>=0; --i)
		{
		TUint32 x = (iX[i]>>1) | (c<<31);
		c = iX[i];
		iX[i] = x;
		}
	return c&1;
	}

TUint32 TLargeIntBase::Asr()
	{
	TInt i=iC-1;
	TUint32 c = iX[i]>>31;
	for (; i>=0; --i)
		{
		TUint32 x = (iX[i]>>1) | (c<<31);
		c = iX[i];
		iX[i] = x;
		}
	return c&1;
	}

void TLargeIntBase::Lsl(TInt aCount)
	{
	while(--aCount>=0)
		Lsl();
	}

void TLargeIntBase::Lsr(TInt aCount)
	{
	while(--aCount>=0)
		Lsr();
	}

void TLargeIntBase::Asr(TInt aCount)
	{
	while(--aCount>=0)
		Asr();
	}

void TLargeIntBase::Add(const TLargeIntBase& a)
	{
	__ASSERT(a.iC==iC);
	TInt i;
	TUint32 c = 0;
	for (i=0; i<iC; ++i)
		{
		TUint32 x = iX[i];
		TUint32 y = a.iX[i];
		TUint32 s = x + y;
		iX[i] = (s + (c>>31));
		TUint32 g = (x & y) | ((x | y) &~ s);
		TUint32 p = ~s ? 0 : s;
		c = g | (c & p);
		}
	}

void TLargeIntBase::Sub(const TLargeIntBase& a)
	{
	__ASSERT(a.iC==iC);
	TInt i;
	TUint32 c = 0x80000000u;
	for (i=0; i<iC; ++i)
		{
		TUint32 x = iX[i];
		TUint32 y = ~a.iX[i];
		TUint32 s = x + y;
		iX[i] = (s + (c>>31));
		TUint32 g = (x & y) | ((x | y) &~ s);
		TUint32 p = ~s ? 0 : s;
		c = g | (c & p);
		}
	}

void TLargeIntBase::Mul(const TLargeIntBase& a)
	{
	__ASSERT(a.iC==iC);
	TUint32 temp[64];	// HACK!!
	Mem::Copy(temp, this, (iC+1)*sizeof(TUint32));
	TLargeIntBase& b = *(TLargeIntBase*)temp;
	new (this) TLargeIntBase(iC,TUint32(0u));
	TInt i;
	for (i=0; i<32*iC; ++i)
		{
		Lsl();
		if (b.Lsl())
			Add(a);
		}
	}

void TLargeIntBase::DivU(const TLargeIntBase& aDivisor, TLargeIntBase& aRem)
	{
	__ASSERT(aDivisor.iC==iC);
	__ASSERT(aRem.iC==iC);
	new (&aRem) TLargeIntBase(iC,TUint32(0u));
	TInt i;
	for (i=0; i<iC*32; ++i)
		{
		aRem.Lsl();
		if (Lsl())
			aRem.Inc();
		if (aRem.Hs(aDivisor))
			aRem.Sub(aDivisor), Inc();
		}
	}

void TLargeIntBase::DivS(const TLargeIntBase& aDivisor, TLargeIntBase& aRem)
	{
	__ASSERT(aDivisor.iC==iC);
	__ASSERT(aRem.iC==iC);
	TUint32 temp[64];	// HACK!!
	Mem::Copy(temp, &aDivisor, (iC+1)*sizeof(TUint32));
	TLargeIntBase& divisor = *(TLargeIntBase*)temp;
	TUint32 rs = iX[iC-1];
	TUint32 qs = divisor.iX[iC-1] ^ rs;
	Abs();
	divisor.Abs();
	DivU(divisor, aRem);
	if (rs & 0x80000000u)
		aRem.Neg();
	if (qs & 0x80000000u)
		Neg();
	}

TInt TLargeIntBase::CompareU(const TLargeIntBase& a) const
	{
	__ASSERT(a.iC==iC);
	TInt i;
	for (i=iC-1; i>=0; --i)
		{
		TUint32 x = iX[i];
		TUint32 y = a.iX[i];
		if (x>y)
			return 1;
		if (x<y)
			return -1;
		}
	return 0;
	}

TInt TLargeIntBase::CompareS(const TLargeIntBase& a) const
	{
	__ASSERT(a.iC==iC);
	TInt i;
	TUint32 m = 0x80000000u;
	for (i=iC-1; i>=0; --i)
		{
		TUint32 x = iX[i] ^ m;
		TUint32 y = a.iX[i] ^ m;
		m = 0;
		if (x>y)
			return 1;
		if (x<y)
			return -1;
		}
	return 0;
	}

