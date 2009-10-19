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
// e32\euser\maths\um_dtor.cpp
// 
//

#include "um_std.h"

const TInt KMaxScanDigits = 19;	// maximum number of decimal digits guaranteed not to overflow TUint64

void TLex8::Scndig(TInt& aSig, TInt& aExp, TUint64& aDl)
//
// Scans a decimal digit field and accumulates the value to a TUint64 at aDl
// Used before decimal point - do not drop trailing zeros.
//
	{
	FOREVER
		{
		if (iNext>=iEnd)
			break;
		TChar c=Peek();
		if (!c.IsDigit())
			break;
		else
			c=Get();
		if (aSig<KMaxScanDigits)
			{
			aDl *= 10;				// Multiply accumulator by 10
			aDl+=((TUint)c)-'0';	// Add current digit
			aSig++;
			}
		else
			aExp++;
		}														  
	}

void TLex8::ScndigAfterPoint(TInt& aSig, TUint64& aDl)
//
// Scans a decimal digit field and accumulates the value to a TUint64 at aDl
// Used after decimal point - drops trailing zeros.
//
// Could be improved with change to header file!!
	{
	TInt trailing=0;	// no of trailing zeros
	TInt leading=0;		// no of leading zeros
	TInt n;
	TChar c;
	
	FOREVER
		{
		if (iNext>=iEnd)
			break;
		c=Peek();
		if (!c.IsDigit())
			break;
		else
			{
			c=Get();
			if (c=='0')
				{
				if (aDl!=0)		// possible trailing zeros
					trailing++;	
				else			// if aDl==0 multiplying by 10 and adding 0 has no effect  
					{
					leading++;
					aSig++;		// leading zeros have significance
					}
				}	
			else if ((aSig<KMaxScanDigits+leading && !trailing) || (trailing && aSig+trailing+1<KMaxScanDigits))
				{
				// first multiply, taking account of preceeding zeros
				for (n=trailing; n>=0; n--)
					{
					aDl *= 10;			// Multiply accumulator by 10
					}
				// now add current digit
				aDl+=((TUint)c)-'0';
				// now update significant digits used
				aSig+=trailing+1;
				trailing=0;
				}
			}
		}
	}	

void TLex16::Scndig(TInt& aSig, TInt& aExp, TUint64& aDl)
//
// Scans a decimal digit field and accumulates the value to a TUint64 at aDl
//
	{

	FOREVER
		{
		TChar c=Peek();
		if (!c.IsDigit())
			break;
		else
			c=Get();
		if (aSig<KMaxScanDigits)
			{
			aDl *= 10;				// Multiply accumulator by 10
			aDl+=((TUint)c)-'0';	// Add current digit
			aSig++;
			}
		else
			aExp++;
		}														  
	}

EXPORT_C TInt TLex8::Val(TReal32& aVal)
//
// Convert a 32 bit real.
//
	{
	TRealX x;
	TInt r=Val(x);
	if (r==KErrNone)
		r=x.GetTReal(aVal);
	return r;
	}

EXPORT_C TInt TLex8::Val(TReal32& aVal, TChar aPoint)
//
// Convert a 32 bit real.
//
	{
	TRealX x;
	TInt r=Val(x,aPoint);
	if (r==KErrNone)
		r=x.GetTReal(aVal);
	return r;
	}

EXPORT_C TInt TLex8::Val(TReal64& aVal)
//
// Convert a 64 bit real.
//
	{
	TRealX x;
	TInt r=Val(x);
	if (r==KErrNone)
		r=x.GetTReal(aVal);
	return r;
	}

EXPORT_C TInt TLex8::Val(TReal64& aVal, TChar aPoint)
//
// Convert a 64 bit real.
//
	{
	TRealX x;
	TInt r=Val(x,aPoint);
	if (r==KErrNone)
		r=x.GetTReal(aVal);
	return r;
	}

TInt TLex8::Val(TRealX& aVal)
//
// Convert an extended real. Use the locale decimal point.
//
	{
	TLocale locale;
	return(Val(aVal,locale.DecimalSeparator()));
	}

TInt TLex8::Val(TRealX& aVal, TChar aPoint)
//
// Convert an extended real.
//
	{

	TLexMark8 start(iNext);
	if (iNext>=iEnd)
		return(KErrGeneral);
	TUint64 n(0);
	TBool minus=EFalse;
	if (Peek()=='-')
		{
		Inc();
		minus=ETrue;
		}
	else if (Peek()=='+')
		Inc();
	TInt digflg=Peek().IsDigit();
	while (Peek()=='0')		// Skip leading zeros
		Inc();
	TInt nsig=0;
	TInt nskip=0;
	Scndig(nsig,nskip,n);
	TInt nint=nsig;
	TInt nfract=0;
	if (Peek()==aPoint)
		{
		Inc();
		if (!digflg)
			digflg=Peek().IsDigit();
		ScndigAfterPoint(nsig,n);	// skip trailing zeros
		nfract=nsig-nint;
		}
	if (!digflg)
		{
		UnGetToMark(start);
		return(KErrGeneral);	// Not a number
		}
	TInt nexp=0;
	TInt r;
	if (Peek()=='E' || Peek()=='e')
		{
		TLexMark8 rollback(iNext);
		Inc();
		r=Val(nexp);
		if (r!=KErrNone)
			{
			if (r==KErrOverflow)
				{
				aVal.SetInfinite(minus);
				return r;
				}
			else
				{
				//it wasn't a number after the 'e', so rollback to the 'e'
				UnGetToMark(rollback);
				}
			}
		}

	if (n == 0)
		{
		aVal.SetZero();
		return KErrNone;
		}

	// Clear msb and if it was set then add 2^63 to aVal as a TRealX
	// as TRealX can only be set from a TInt64.
	TUint32 nh = I64HIGH(n);
	n <<= 1;	// Clear the msb of n (64 bit number so this is most efficient method).
	n >>= 1;
	aVal = TInt64(n);
	if (nh & 0x80000000u)
		{
		TRealX nhx(1);
		nhx.iExp = (TUint16)(nhx.iExp + 63);
		aVal += nhx;
		}
	if (minus)
		aVal = -aVal;
	nexp += nskip - nfract;
	r=Math::MultPow10X(aVal,nexp);
	return r;
	}

EXPORT_C TInt TLex16::Val(TReal32& aVal)
//
// Convert a 32 bit real.
//
	{
	TRealX x;
	TInt r=Val(x);
	if (r==KErrNone)
		r=x.GetTReal(aVal);
	return r;
	}

EXPORT_C TInt TLex16::Val(TReal32& aVal, TChar aPoint)
//
// Convert a 32 bit real.
//
	{
	TRealX x;
	TInt r=Val(x,aPoint);
	if (r==KErrNone)
		r=x.GetTReal(aVal);
	return r;
	}

EXPORT_C TInt TLex16::Val(TReal64& aVal)
//
// Convert a 64 bit real.
//
	{
	TRealX x;
	TInt r=Val(x);
	if (r==KErrNone)
		r=x.GetTReal(aVal);
	return r;
	}

EXPORT_C TInt TLex16::Val(TReal64& aVal, TChar aPoint)
//
// Convert a 64 bit real.
//
	{
	TRealX x;
	TInt r=Val(x,aPoint);
	if (r==KErrNone)
		r=x.GetTReal(aVal);
	return r;
	}

TInt TLex16::Val(TRealX& aVal)
//
// Convert an extended real. Use the locale decimal point.
//
	{
	TLocale locale;
	return(Val(aVal,locale.DecimalSeparator()));
	}

TInt TLex16::Val(TRealX& aVal, TChar aPoint)
//
// Convert a 64 bit real
//
	{
	
	HBufC8 *temp=HBufC8::New(iEnd-iNext);
	if (temp==NULL)
		return(KErrNoMemory);
	TPtr8 tdes(temp->Des());

	for (const TText* p = (TText*)iNext; p < (TText*)iEnd; p++)
		{
		TChar c = *p;
		if (c == aPoint)
			c = '.';
		else if (c == '.')
			c = ' ';
		else if (c > 255)
			c = ' ';
		tdes.Append((TUint8)c);
		}
	aPoint = '.';

	TLex8 lex(tdes);
	lex.Mark();
	TInt r=lex.Val(aVal,aPoint);
	User::Free(temp);
	if (r==KErrNone)
		Inc(lex.TokenLength());
	return r;
	}
