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
// e32test\math\largeint.h
// 
//

#ifndef __LARGEINT_H__
#define __LARGEINT_H__
#include <e32std.h>

#define __ASSERT(c)		__ASSERT_DEBUG(c,User::Invariant())

class TLargeIntBase
	{
public:
	enum TMode {EZeroExtend,ESignExtend,ETruncate};
	TLargeIntBase(TInt n);
	TLargeIntBase(TInt n, TInt32 aSigned32);
	TLargeIntBase(TInt n, TUint32 aUnsigned32);
	TLargeIntBase(TInt n, const TLargeIntBase&, TMode);
	TLargeIntBase(TInt n, const TUint32* aPtr);
	TLargeIntBase(TInt n, const Int64& aSigned64);
	TLargeIntBase(TInt n, const Uint64& aUnsigned64);
public:
	void Not();
	void Neg();
	void Abs();
	void Inc();
	void Dec();
	TUint32 Lsl();
	TUint32 Lsr();
	TUint32 Asr();
	void Lsl(TInt aCount);
	void Lsr(TInt aCount);
	void Asr(TInt aCount);
	void Add(const TLargeIntBase&);
	void Sub(const TLargeIntBase&);
	void Mul(const TLargeIntBase&);
	void DivU(const TLargeIntBase& aDivisor, TLargeIntBase& aRem);
	void DivS(const TLargeIntBase& aDivisor, TLargeIntBase& aRem);
	TInt CompareU(const TLargeIntBase&) const;
	TInt CompareS(const TLargeIntBase&) const;
	inline TLargeIntBase& operator++() {Inc(); return *this;}
	inline TLargeIntBase& operator--() {Dec(); return *this;}
public:
	inline TBool operator==(const TLargeIntBase& a) const
		{return CompareU(a)==0;}
	inline TBool operator!=(const TLargeIntBase& a) const
		{return CompareU(a)!=0;}
	inline TBool Hi(const TLargeIntBase& a) const
		{return CompareU(a)>0;}
	inline TBool Hs(const TLargeIntBase& a) const
		{return CompareU(a)>=0;}
	inline TBool Lo(const TLargeIntBase& a) const
		{return CompareU(a)<0;}
	inline TBool Ls(const TLargeIntBase& a) const
		{return CompareU(a)<=0;}
	inline TBool Gt(const TLargeIntBase& a) const
		{return CompareS(a)>0;}
	inline TBool Ge(const TLargeIntBase& a) const
		{return CompareS(a)>=0;}
	inline TBool Lt(const TLargeIntBase& a) const
		{return CompareS(a)<0;}
	inline TBool Le(const TLargeIntBase& a) const
		{return CompareS(a)<=0;}
	inline TUint32& operator[](TInt a)
		{__ASSERT(TUint32(a)<TUint32(iC)); return iX[a];}
	inline TUint32 operator[](TInt a) const
		{__ASSERT(TUint32(a)<TUint32(iC)); return iX[a];}
public:
	TInt iC;
	TUint32 iX[1];
	};

template <TInt n>
class TLargeInt : public TLargeIntBase
	{
public:
	inline TLargeInt() : TLargeIntBase(n) {}
	inline TLargeInt(TInt32 aSigned32) : TLargeIntBase(n,aSigned32) {}
	inline TLargeInt(TUint32 aUnsigned32) : TLargeIntBase(n,aUnsigned32) {}
	inline TLargeInt(const TLargeIntBase& aSrc, TMode aMode) : TLargeIntBase(n,aSrc,aMode) {}
	inline TLargeInt(const TUint32* aPtr) : TLargeIntBase(n, aPtr) {}
	inline TLargeInt(const Int64& aSigned64) : TLargeIntBase(n, aSigned64) {}
	inline TLargeInt(const Uint64& aUnsigned64) : TLargeIntBase(n, aUnsigned64) {}
	inline TLargeInt<2*n> LongMultU(const TLargeInt<n>& a) const
		{TLargeInt<2*n> x(*this,EZeroExtend); TLargeInt<2*n> y(a,EZeroExtend); x.Mul(y); return x;}
	inline TLargeInt<2*n> LongMultS(const TLargeInt<n>& a) const
		{TLargeInt<2*n> x(*this,ESignExtend); TLargeInt<2*n> y(a,ESignExtend); x.Mul(y); return x;}
public:
	TUint32 iExtra[n-1];
	};

#define CHECK(X)	void __compile_assert(int __check[(X)?1:-1])

#endif
