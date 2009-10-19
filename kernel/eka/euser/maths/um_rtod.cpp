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
// e32\euser\maths\um_rtod.cpp
// 
//

#include "um_std.h"

#include <e32debug.h>

#define KNeedsRounding 0x1000
#if (KRealFormatTypeFlagsMask| KRealFormatTypesMask) & KNeedsRounding 
	#error KNeedsRounding already uses 0x1000
#endif

const TInt KMinThreeDigitExponent=100;

_LIT8(KLit8Plus,"+");
_LIT8(KLit8Minus,"-");
_LIT8(KLit8Zero,"0");
_LIT8(KLit8Inf,"Inf");
_LIT8(KLit8Nan,"NaN");

LOCAL_C TInt round(const TText8* aBuf, const TInt aLen, TBool aExact)
//
// Round number in buffer depending on digit at aBuf[aLen]. 
// Return value is carry from most significant digit.
//
	{
	__ASSERT_DEBUG(aLen>=0,Panic(EMathUnexpectedError1));
	TText8* pB=(TText8*)aBuf+aLen;
	if (*pB<'5')
		return 0;	// round down
	if (*pB=='5' && aExact && aLen>0 && !(pB[-1]&1))
		return 0;	// exactly half way - round to even
	while(--pB>=aBuf)
		{
		TText8 d=*pB;
		if (d<'9')
			{
			++d;
			*pB=d;
			return 0;
			}
		*pB='0';
		}
	// carry propagates to exponent
	*++pB='1';
	if (aLen>0)
		pB[aLen]='0';
	return 1;
	}

TUint mult10(TUint64& a)
//
// Multiply a 64-bit binary fraction in a (MSB=2^-1) by 10
// Return the fractional part in a, the integer part in the return value.
//
	{
	const TUint64 ten = 10;
	TUint64 high;
	Math::UMul64(a, ten, high, a);
	return static_cast<TUint>(high);
	}

LOCAL_C TInt fDigLim(TDes8& aTrg, const TReal& aSrc, TInt& aExp, TInt aPrec)
//
// Convert the TReal at address aSrc to a decimal form suitable for use by a
// formatting routine. Writes the result into the descriptor at aTrg and the
// exponent in aExp. Returns the length of the string or a negative error number.
//
// AnnW, November 1996 - changed to handle all numbers in TReal64/96 range.
//
// The first character in aBuf is one of:
//
// '0' - indicates that aSrc is exactly zero.
//
// '+' - indicates a positive number and is followed by between 1 and KMaxPrecision
// decimal digits representing a mantissa in the range 0.1 to less than 1.0 which 
// corresponds to the decimal exponent returned in aExp.
//
// '-' - indicates a negative mantissa and is otherwise the same as for '+'.
// 
	{
	
	TRealX x;
	TInt r=x.Set(aSrc);

	if (x.IsZero())
		{
		aTrg=KLit8Zero();
		return 1;
		}
	else	// sets sign in all cases, including specials
		aTrg=(x.iSign ? KLit8Minus() : KLit8Plus());
		
	if (r!=KErrNone)
		return r;

	x.iSign=0;
	TInt e=TInt(x.iExp)-0x7fff;		// 2^e<=Abs(x)<2^(e+1)
	e*=19728;						// log10(2)*65536 = 19728.301796...
									// max error due to omission of fractional part is 9889 (towards zero)
	e-=9889;						// account for error, e may be too small by up to 19778
									// Now have 10^(e/65536)<=Abs(x)<4.007*10^(e/65536)
	e>>=16;							// Divide by 65536 - this always rounds towards -infinity
									// Now have 10^e<=Abs(x)<40.07*10^e
	e+=1;							// Now have 0.1<=Abs(x)/10^e<4.07
	Math::MultPow10X(x,-e);			// x*=10^-e so now 0.1<=x<4.07 (rel error <= 8 units of 0.5LSB)
	if (x.iExp>=0x7fff)
		{
		++e;
		Math::MultPow10X(x,-1);		// if x>=1, multiply x by 10^-1 and increment e
		}
	// relative error now <= 11 units of 0.5LSB

	TUint64 mantissa = MAKE_TUINT64(x.iMantHi, x.iMantLo);

	if (x.iExp<0x7ffe)
		mantissa >>= (0x7ffe - x.iExp);	// shift to make exponent 0x7ffe, i.e. mantissa in range 0.1<=m<1

	TInt prec = aPrec;
	if (prec > KIEEEDoubleInjectivePrecision)
		prec = KIEEEDoubleInjectivePrecision;

	while ( mantissa && (aTrg.Length() < (prec + 2)) )
		{
		TUint d = mult10(mantissa);
		aTrg.Append(d+'0');
		}
	if (aTrg.Length()>=prec+2)
		{
		e+=round(aTrg.Ptr()+1,prec,mantissa==0);
		aTrg.SetLength(prec+1);
		}
	aExp=e;
	return aTrg.Length();
	}

LOCAL_C TInt doExponent(TDes8* This, TDes8& aDigBuf, const TInt afDigLimSize, TInt aExp,
	const TInt aNumPlcs, const TInt aNumSpace, const TText aPoint, const TUint flags)
//
// Convert the intermediate number represented in aDigBuf into its exponential representation
// and place into aTrg.
// This representation ensures that numbers are displayed to aNumDecPlcs+1 significant figures,
// but this is NOT a constant value in KTRealFormatGeneral mode. 
//
// AnnW, November 1996 - changed to be able to take three-figure exponents if allowed by flags.
// 
	{

	TInt err;
	TInt expSpace;
	TInt useSigFigs=flags & KUseSigFigs;
	TInt nDig=(useSigFigs ? Min(aNumPlcs,afDigLimSize) : aNumPlcs+1);	
	TInt threeDigitExp=flags & KAllowThreeDigitExp;

	if ((flags & KNeedsRounding) && afDigLimSize>nDig)
		aExp+=round(aDigBuf.Ptr()+1,nDig,EFalse);

	if (useSigFigs)			// discard trailing zeros
		{
		while(nDig>1 && aDigBuf[nDig]=='0')
			{
			nDig--;
			}
		}
	
	if (aDigBuf[0]!='0')
	// 100.5 is stored in aDigBuf as +1005 with and exp of 3, but it is to be displayed
	// as 1.005 so exp must be decremented to 2
		aExp--;

	// Added by AnnW
	if (threeDigitExp)
		{
		expSpace=(Abs(aExp)>=KMinThreeDigitExponent)?5:4;
		}
	else
		{
		err=(aExp<=-KMinThreeDigitExponent ? KErrUnderflow : (aExp>=KMinThreeDigitExponent ? KErrOverflow : KErrNone));
		if (err!=KErrNone)
			return(err);
		expSpace=4;
		}
	
	// Check that number will fit in aNumSpace
	if (aNumSpace<(expSpace+nDig+(nDig>1?1:0)))
	// exponent + significant figures + point(if necessary)
		return(KErrGeneral);
	// end of added

	if (aDigBuf[0]=='0')  // number to be converted is 0
		{
		This->Append('0');
		if (nDig>1 && !useSigFigs)
			{
			This->Append(aPoint);
			This->AppendFill('0',aNumPlcs);
			}
		}
	else
		{
		This->Append(TChar(aDigBuf[1]));
		if (nDig>1)
			{
			This->Append(aPoint);
			for (TInt ii=2; ii<=nDig; ii++)
				{
				if (!useSigFigs)
					// pad with zeros
					This->Append(TChar(ii<aDigBuf.Length() ? aDigBuf[ii]:'0'));
				else
					// do not pad with zeros
					This->Append(TChar(aDigBuf[ii]));
				}
			}
		}
	
	This->Append('E');
	if (aExp<0) 
		{
		aExp=-aExp;
		This->Append('-');
		}
	else
		This->Append('+');

	// Added by AnnW
	TInt tempExp;
	if (threeDigitExp && aExp>99)		
		{								
		This->Append(aExp/100+'0');
		tempExp=aExp%100;
		}								
	else								
		tempExp = aExp;				
	// end of added

	This->Append(tempExp/10+'0'); 
	This->Append(tempExp%10+'0');
	return(KErrNone);
	}

LOCAL_C TInt doFixed(TDes8 *This,TDes8 &aDigBuf,const TInt afDigLimSize,TInt aExp,const TInt aNumDecPlcs,
				const TInt aNumSpace,const TRealFormat &aFormat,const TUint flags)
//
// Convert the intermediate number represented in aDigBuf into its fixed representation and
// place into aTrg
//
// AnnW, November 1996 - changed to allow extra space to be left for potential sign, so that
// positive and negative numbers of the same exponent are displayed to the same precision.
//
	{

	TInt err;
	TInt doNotUseTriads=flags & KDoNotUseTriads;
	TInt newNumSpace=aNumSpace;
	// To allow positive and negative numbers with the same exponent to	have the same number of 
	// significant figures.
	if ((flags & KExtraSpaceForSign) && (aDigBuf[0]!='-'))
		newNumSpace--;

	TInt roundOffset = aNumDecPlcs+aExp;
	if (roundOffset>=0 && afDigLimSize>roundOffset && (flags & KNeedsRounding))
		aExp+=round(aDigBuf.Ptr()+1,roundOffset,EFalse);

	if (newNumSpace<((aExp>0?aExp:1)+(aNumDecPlcs?aNumDecPlcs+1:0)+(!doNotUseTriads && aFormat.iTriLen && (aExp>(TInt)aFormat.iTriLen)?(aExp-1)/3:0)))
	// exponent +ve and space < space needed for (digits before point + point + decimal places + triads)
		{
		err=(aExp>0 ? KErrOverflow : KErrGeneral);
		return(err);
		}

	if (aExp<=0) // hence number is of the form 0.NNNN
		{
		This->Append('0');
		if (aNumDecPlcs)
			{
			aExp=-aExp;
			TInt nDig=aNumDecPlcs-aExp;	// number of digits required from aDigBuf
			This->Append(aFormat.iPoint);
			if (aExp>aNumDecPlcs)	 
				aExp=aNumDecPlcs;
			This->AppendFill('0',aExp);
			if (nDig>0)	 
				{
				for (TInt ii=1; ii<nDig+1; ii++)
					This->Append(TChar(ii<aDigBuf.Length() ? aDigBuf[ii]:'0'));
				}
			}
		else	// no decimal places
			{
			if (aDigBuf[0]=='-')
				{
				This->Delete(0,2);	// delete -0 from This
				This->Append('0');
				}
			}
		}
	else  // aExp > 0 hence number is of the form NNNN.NNNN
		{
		for (TInt jj=1,ii=aExp; ii; ii--,jj++)
			{
			if (!doNotUseTriads && aFormat.iTriLen && aExp>(TInt)aFormat.iTriLen && !(ii%3) && ii!=aExp)
				This->Append(aFormat.iTriad);
			This->Append(TChar(jj>=aDigBuf.Length() ? '0' : aDigBuf[jj]));
			}
		if (aNumDecPlcs>0)
			{
			This->Append(aFormat.iPoint);
			for (TInt ii=aExp+1; ii<aNumDecPlcs+aExp+1; ii++)
				This->Append(TChar(ii<aDigBuf.Length() ? aDigBuf[ii] : '0'));
			}
		}

	return(KErrNone);
	}

LOCAL_C TInt doNoExponent(TDes8 *This, TDes8 &aDigBuf,const TInt afDigLimSize,TInt aExp,
	const TInt aMaxSigFigs,const TInt aNumSpace, const TRealFormat &aFormat,const TUint flags)
//
// Added by AnnW, November 1996
// Convert the intermediate number represented in aDigBuf into its no exponent representation
// and place into aTrg
// Changed April 1997 - If there is not enough space for the number of s.f., zeros, points, triads,
// etc, then the number of significant figs is reduced to fit.  Overflow if too big to fit and 
// underflow if no significance can be seen.
//
	{

	TInt doNotUseTriads=flags & KDoNotUseTriads;
	TInt numSpace=aNumSpace;
	if ((flags & KExtraSpaceForSign) && (aDigBuf[0]!='-'))
		numSpace--;

	TInt nTriadSeps=(!doNotUseTriads && aFormat.iTriLen && (aExp>(TInt)aFormat.iTriLen))?(aExp-1)/3:0;
	TInt maxDig=Min(aMaxSigFigs,afDigLimSize);
	TInt maxSpace=numSpace-(aExp<=0?(2-aExp):((aExp<maxDig?1:aExp-maxDig)+nTriadSeps));
	TInt nDig=Min(maxSpace,maxDig);
	if (afDigLimSize>nDig && nDig<15 && nDig>=0 && (flags & KNeedsRounding) && round(aDigBuf.Ptr()+1,nDig,EFalse)) 
		aExp++;
		
	if (aDigBuf[0]=='0')	// do zero first (numSpace>=0 so OK)
		This->Append('0');
	else
		{
		// check for overflow/underflow
		if ((aExp+nTriadSeps)>numSpace)
			return(KErrOverflow);
		if (nDig<=0)
			return(KErrUnderflow);

		if ((flags&(TUint)KRealFormatTypesMask)!=(TUint)KRealFormatCalculator && aExp>aMaxSigFigs)
			return(KErrOverflow);
		
		TInt nDecPlcs=nDig-aExp;
		while(nDecPlcs>0 && aDigBuf[nDig]=='0')
			{ // discard trailing zeros (already done in calculator)
			nDecPlcs--;
			nDig--;
			}
		
		if (aExp<=0) // hence number is of the form 0.NNNN
			{
			This->Append('0');
			aExp=-aExp;
			// if (nDecPlcs<=0) do nothing	
			if (nDecPlcs>0)	 
				{
				This->Append(aFormat.iPoint);
				This->AppendFill('0',aExp);
				for (TInt ii=1; ii<=nDig; ii++)
					This->Append(TChar(aDigBuf[ii]));
				}
			}
		else  // aExp > 0 hence number is of the form NNNN.NNNN
			{
			for (TInt jj=1,ii=aExp; ii; ii--,jj++)
				{
				if (!doNotUseTriads && aFormat.iTriLen && aExp>(TInt)aFormat.iTriLen && !(ii%3) && ii!=aExp)
					This->Append(aFormat.iTriad);
				This->Append(TChar(jj<=nDig ? aDigBuf[jj] : '0'));
				}
			if (nDecPlcs>0)	
				{
				This->Append(aFormat.iPoint);
				for (TInt ii=aExp+1; ii<=nDig; ii++)
					This->Append(TChar(aDigBuf[ii]));	
				}
			}
		}

	return(KErrNone);
	}

LOCAL_C TInt doGeneral(TDes8 *This,TReal aSrc,TDes8 &aDigBuf,const TInt afDigLimSize,TInt aExp,
			TInt aNumDecPlcs,const TInt aNumSpace,const TRealFormat &aFormat,TUint flags) __SOFTFP
//
// Convert the intermediate number represented in aDigBuf into either its fixed representation or
// its exponential representation as appropriate and place the result in aTrg
// 
// Annw, November 1996 - changed to allow space for sign in fixed mode, three-figure exponent.
	{

	TBool rounded=((flags & KNeedsRounding)==0);
	TInt nDig=aDigBuf.Length()-1;	 // no of digits without sign
	TInt type;

	// Set up tempNumSpace to allow for leaving one space free for +ve nos in fixed format
	TInt fixedNumSpace=aNumSpace;
	if ((flags & KExtraSpaceForSign) && (aDigBuf[0]!='-'))
		fixedNumSpace--;
	if (fixedNumSpace<=0)
		return(KErrGeneral);

	FOREVER
		{
		// If aNumSpace < 5 cannot use exponential format, i.e. not enough space for XE+NN.
		// If the exponent >= -3 (i.e. aExp >= -2), it is always more (or equally) efficient
		// to use non-exponential format for negative exponents, i.e. XE-03 takes same no of
		// spaces as 0.00X, and for positive exponents use fixed form as far as possible. 
		
		// for Java do not used fixed format for exponents >=7  
		// expMax=Min(fixedNumSpace,7)
		// and replace "fixednumSpace" with "expMax" in next line

		if (aNumSpace<5 || (aExp>=-2 && aExp<=fixedNumSpace))
			{
			type=KRealFormatFixed;

			// if there is at least one digit before decimal point or no. is zero
			if (aExp>0 || !aSrc)
				{
				if (nDig!=aExp)
				// nDig is the number of digits which will be used
				// a decimal point needed if exponent < digits in digbuf and numspace < nDig,
				// so nDig is one less than otherwise
				nDig=((nDig-aExp)>0 && fixedNumSpace>aExp)?Min(fixedNumSpace-1,nDig):Min(fixedNumSpace,nDig);
				aNumDecPlcs=nDig-aExp;
				}
		 	else
				{
				// need space for "0." and to avoid white spaces
				aNumDecPlcs=Min(fixedNumSpace-2,nDig-aExp); 
				// need space for "0.0...0" before any digits used
				nDig=aNumDecPlcs+aExp; 
				if (nDig<0)
					return KErrGeneral;
				}
			}
		else
			{
			type=KRealFormatExponent;		// Do NOT use significant figures
			// Need to allow space for exponent
			TInt tempNumSpace=aNumSpace-4;	// 4 = E+NN
			if ((flags & KAllowThreeDigitExp) && (Abs(aExp-1)>=100))
				tempNumSpace--;				// 5 = E+NNN			
			// if more than one digit available and enough digits to fill space, need to reduce
			// number of digits to allow for '.'
			if (((nDig=Min(tempNumSpace,nDig))>1) && nDig==tempNumSpace) 
				nDig--; 
			// in any case, aNumDecPlcs is one less that the number of digits,
			// i.e. one digit before the point
			aNumDecPlcs=nDig-1;
			}
		// if too many digbuf chars to fit then we need to round
		// round() returns 1 if had to carry from msdigit
		if ((afDigLimSize>nDig && !rounded) && ((rounded=round(aDigBuf.Ptr()+1,nDig,EFalse))!=0))
			aExp++;
		else
			break;
		}		
	while(aNumDecPlcs>0 && aDigBuf[nDig]=='0')
		{ // discard trailing zeros
		aNumDecPlcs--;
		nDig--;
		}	
	flags=flags & ~KNeedsRounding;
	
	if (type==KRealFormatExponent)
		return(doExponent(This,aDigBuf,afDigLimSize,aExp,aNumDecPlcs,aNumSpace,(TText)aFormat.iPoint,flags));
	return(doFixed(This,aDigBuf,afDigLimSize,aExp,aNumDecPlcs,aNumSpace,aFormat,flags));
	}

 LOCAL_C TInt doCalculator(TDes8 *This,TDes8 &aDigBuf,const TInt afDigLimSize,TInt aExp,
			TInt aMaxSigFigs,const TInt aMaxSpace,const TRealFormat &aFormat, TUint flags)
//
// Added by AnnW, November 1996
// Convert the intermediate number represented in aDigBuf into either its no exponent or its 
// exponential representation with a fixed number of significant figures and place the result 
// in aTrg
// 
	{

	TBool threeDigExp=((flags & KAllowThreeDigitExp)!=0);

	// first check that enough space has been allowed for all the possible characters
	// point + sign + all sig figs + exponent
	if (aMaxSpace<(2+aMaxSigFigs+(threeDigExp?5:4)))
		return(KErrGeneral);

	// now discard trailing zeros
	TInt nDig=afDigLimSize;
	while(nDig>1 && aDigBuf[nDig]=='0')
		{
		nDig--;
		}

	TInt maxDig=Min(aMaxSigFigs,nDig);	// max digs available
	TBool rounded=((flags & KNeedsRounding)==0);
	TInt type;	
	TBool useNoExp;

	FOREVER
		{
		useNoExp=ETrue;
		nDig=maxDig;
		
		// use no exponent for all numbers which will not use > aMaxSigFigs digits
		if (aExp>aMaxSigFigs || (aExp<=0 && (1-aExp+nDig)>aMaxSigFigs))
			useNoExp=EFalse;
		
		if (useNoExp)
			type=KRealFormatNoExponent;
		else
			{
			type=KRealFormatExponent;
			threeDigExp=((Abs(aExp-1)>=KMinThreeDigitExponent && threeDigExp)!=0);
			TInt temp=aMaxSpace-(threeDigExp?5:4);
			nDig=Min(maxDig,temp-((temp>1 && maxDig>1)?1:0));
			}
			
		// if too many digbuf chars to fit then we need to round
		// round() returns 1 if had to carry from msdigit
		if ((afDigLimSize>nDig && !rounded) && ((rounded=round(aDigBuf.Ptr()+1,nDig,EFalse))!=0))
			{
			aExp++;
			maxDig=1;
			}
		else
			break;
		}
 
	TInt numSpace=aMaxSpace-(aDigBuf[0]=='-'?1:0);
	flags=flags & ~KNeedsRounding;

	if (type==KRealFormatExponent)
		return(doExponent(This,aDigBuf,afDigLimSize,aExp,nDig,numSpace,(TText)aFormat.iPoint,flags));
	else
		{
		flags|=KExtraSpaceForSign;
		return(doNoExponent(This,aDigBuf,afDigLimSize,aExp,nDig,numSpace,aFormat,flags));
		}
	}

TInt ProcessErrors(TDes8* aDes, TInt anError)
	{
	if (anError==KErrNone)
		return aDes->Length();
	if (anError==KErrUnderflow)
		aDes->Append(TChar('0'));
	if (anError==KErrOverflow)
		aDes->Append(KLit8Inf());
	if (anError==KErrArgument)
		aDes->Append(KLit8Nan());
	return anError;
	}

LOCAL_C TInt rtob(TDes8 *This,TReal aVal,const TRealFormat &aFormat) __SOFTFP
//
// Converts the real at aSrc. Returns the length of the converted string or an error number
// if the buffer is too small, the number is out of range or there is insufficient KMaxPrecision
// to represent the number.
//
// The conversion format is interpreted as follows:
// KRealFormatFixed - ndec decimal places (including zero), negative values with a leading minus 
// sign, triad separators are available and a space may be left in front of positive numbers to 
// allow negative positve numbers to be given to the same precision.
// KRealFormatExponent - exponent notation specifying either decimal places or significant
// figures in the mantissa and a signed exponent given to a maximum of two or three digits,
// and no triad separator.
// KTRealFormatGeneral - converts either as fixed or exponent to make best use of the available
// width. The number of decimal spaces is chosen automatically as a function of width
// (ndec is ignored),  no triad.
// KRealFormatNoExponent - as KRealForamtFixed, but specifying maximum significant figures and
// not introducing trailing zeros.
// KRealFormatCalculator - as KRealFormatGeneral, but behaves as a conventional calculator. A
// maximum number of significant figures is specified and the number is displayed without an
// exponent whenever possible, with no trailing zeros and no triads.
// 
// If an error value other than KErrGeneral is returned the real is converted to some string:
// "+/-Inf" if the error is KErrOverflow, "NaN" if the error is KErrArgument or "0" if it is 
// KErrUnderflow.
//
	{
	if (aFormat.iWidth>This->MaxLength())
		return(KErrGeneral);	 
	TBuf8<0x20> digbuf;
	TInt exp=0;
	TInt numspace=aFormat.iWidth;
	TInt maxspace=numspace;
	TInt prec = KMaxPrecision;
	if (aFormat.iType & KRealInjectiveLimit)
		prec = KIEEEDoubleInjectivePrecision;
	else if (aFormat.iType & KGeneralLimit)
		prec = KPrecisionLimit;
	TInt ret=fDigLim(digbuf,aVal,exp,prec);
	digbuf.ZeroTerminate();
	TInt type, flags;

	if (digbuf[0]=='0')
		exp=0;
	else 
		{
		if (digbuf[0]=='-' && ret!=KErrArgument)	// NaN has no sign
			{
			This->Append('-');
			numspace--;
			}
		if (ret<0)
			return ProcessErrors(This, ret);
		else
			ret--;
		}

	//Added by AnnW
	if (numspace<=0)
		return(KErrGeneral);
	if (aFormat.iType & ~KRealFormatTypesMask & ~KRealFormatTypeFlagsMask)
		return(KErrGeneral);
	type=aFormat.iType & KRealFormatTypesMask;
	flags=((aFormat.iType & KRealFormatTypeFlagsMask)| KNeedsRounding);
	// end of added

	switch(type)
		{
	case KRealFormatFixed:
		flags=flags & ~KUseSigFigs;	// if flag is NOT set and iTriLen!=0, uses triads
		ret=doFixed(This,digbuf,ret,exp,aFormat.iPlaces,numspace,aFormat,flags);
		break;
	case KRealFormatExponent:
		ret=doExponent(This,digbuf,ret,exp,aFormat.iPlaces,numspace,(TText)aFormat.iPoint,flags);
		break;
	case KRealFormatGeneral:
		flags=(flags & ~KUseSigFigs) | KDoNotUseTriads;
		ret=doGeneral(This,aVal,digbuf,ret,exp,aFormat.iPlaces,numspace,aFormat,flags);
		break;
	case KRealFormatNoExponent:
		flags=flags | KUseSigFigs;	// if flag is NOT set and iTriLen!=0, uses triads	
		ret=doNoExponent(This,digbuf,ret,exp,aFormat.iPlaces,numspace,aFormat,flags);
		break;
	case KRealFormatCalculator:
		flags=(flags | KUseSigFigs) | KDoNotUseTriads | KRealFormatCalculator;
		ret=doCalculator(This,digbuf,ret,exp,aFormat.iPlaces,maxspace,aFormat,flags);
		break;
	default:
		return(KErrGeneral);
		}
	return ProcessErrors(This, ret);
	}



	  
EXPORT_C TRealFormat::TRealFormat()
/**
Default constructor.

The public data members of the constructed object are assigned
the following values:

TRealFormat::iType   - set to KRealFormatGeneral

TRealFormat::iWidth  - set to KDefaultRealWidth

TRealFormat::iPlaces - set to 0

TRealFormat::iPoint  - set to the decimal separator character defined in
                       a TLocale object and returned by the DecimalSeparator()
                       member function of that class.

TRealFormat::iTriad  - set to the character used to delimit groups of three
                       digits in the integer portion of a number; the character
                       is defined in a TLocale object and returned by the
                       ThousandsSeparator() member function of that class.

TRealFormat::iTriLen - set to 1

@see TLocale::DecimalSeparator
@see TLocale::ThousandsSeparator
*/
	{

	iType=KRealFormatGeneral;
	iWidth=KDefaultRealWidth;
	iPlaces=0;
	TLocale locale;
	iPoint=locale.DecimalSeparator();
	iTriad=locale.ThousandsSeparator();
	iTriLen=1;
	}




EXPORT_C TRealFormat::TRealFormat(TInt aWidth)
/**
Constructs the object taking the width of the character representation.

The remaining public data members of the constructed object are assigned
the following values:

TRealFormat::iType   - set to KRealFormatGeneral

TRealFormat::iWidth  - set to the aWidth argument

TRealFormat::iPlaces - set to 0

TRealFormat::iPoint  - set to the decimal separator character defined in
                       a TLocale object and returned by the DecimalSeparator()
                       member function of that class.

TRealFormat::iTriad  - set to the character used to delimit groups of three
                       digits in the integer portion of a number; the character
                       is defined in a TLocale object and returned by the
                       ThousandsSeparator() member function of that class.

TRealFormat::iTriLen - set to 1

@param aWidth The width of the character representation of the real number.

@see TLocale::DecimalSeparator
@see TLocale::ThousandsSeparator
*/
	{

	iType=KRealFormatGeneral;
	iWidth=aWidth;
	iPlaces=0;
	TLocale locale;
	iPoint=locale.DecimalSeparator();
	iTriad=locale.ThousandsSeparator();
	iTriLen=1;
	}




EXPORT_C TRealFormat::TRealFormat(TInt aWidth,TInt aDecimals)
/**
Constructs the object taking the width of the character representation
and a value which is interpreted as the number of digits to follow
the decimal point.

The remaining public data members of the constructed object are assigned
the following values:

TRealFormat::iType   - set to KRealFormatFixed

TRealFormat::iWidth  - set to the aWidth argument

TRealFormat::iPlaces - set to the aDecimals argument

TRealFormat::iPoint  - set to the decimal separator character defined in
                       a TLocale object and returned by the DecimalSeparator()
                       member function of that class.

TRealFormat::iTriad  - set to the character used to delimit groups of three
                       digits in the integer portion of a number; the character
                       is defined in a TLocale object and returned by the
                       ThousandsSeparator() member function of that class.

TRealFormat::iTriLen - set to 1

Note that if the iType data member is changed after construction, aDecimalPlaces
may be interpreted as the number of significant digits. For more information,
see KRealFormatFixed and the other format types, and KExtraSpaceForSign and the
other format flags.

@param aWidth    The width of the character representation of the real number.
@param aDecimals The number of digits to follow the decimal point.

@see TLocale::DecimalSeparator() 
@see TLocale::ThousandsSeparator() 
@see KRealFormatFixed 
@see KExtraSpaceForSign 
*/
	{

	iType=KRealFormatFixed;
	iWidth=aWidth;
	iPlaces=aDecimals;
	TLocale locale;
	iPoint=locale.DecimalSeparator();
	iTriad=locale.ThousandsSeparator();
	iTriLen=1;
	}



					
EXPORT_C TInt Math::Round(TReal &aTrg,const TReal &aSrc,TInt aDecimalPlaces)
/**
Rounds to a specified number of decimal places.

The function rounds a number to a given number, n, of decimal places.
Rounding may be thought of as multiplying the number by 10 to the power of n,
rounding to the nearest integer, and then dividing the result by 10 to
the power of n and returning that as the answer.

In the process of rounding, numbers ending with .5 are rounded away from zero,
so that 1.5 becomes 2, 2.5 becomes 3, -1.5 becomes -2, etc.

@param aTrg           A reference containing the result. 
@param aSrc           The number to be rounded. 
@param aDecimalPlaces The number of decimal places to round to: must be
                      zero or positive.

@return KErrNone if successful, otherwise another of the system-wide error codes. 
*/
	{

	if (aSrc==0.0)
		{
		aTrg=aSrc;
		return(KErrNone);
		}
	TInt ret,exp;
	TBuf8<0x20> rbuf;
	if ((ret=fDigLim(rbuf,aSrc,exp,KIEEEDoubleInjectivePrecision))<0) 
		return(ret);
	if ((exp+aDecimalPlaces)<0)
		{ // Number too small to be rounded
		aTrg=0;
		return(KErrNone);
		}    
	if ((ret-2)<(exp+aDecimalPlaces))	//ret is the string length, including prefixed +/-		
		{ // Rounding will have no effect
		aTrg=aSrc;
		return(KErrNone);
		}       
	if ((ret=round(rbuf.Ptr()+1,exp+aDecimalPlaces,EFalse))!=KErrNone)
		exp++;
 	rbuf.Insert(1,TPtrC8((TText8*)".",1));
	rbuf.SetLength(exp+aDecimalPlaces+2);
	if (!(exp+aDecimalPlaces))
		rbuf.Append('0');
//	rbuf.AppendFormat(TPtrC8((TText8*)"%c%d",4),'E',exp);
	rbuf.Append(TChar('E'));
	rbuf.AppendNum(exp);
	return(((TLex8)rbuf).Val(aTrg,(TChar)'.'));
	}        

EXPORT_C TInt TDes8::Num(TReal aVal,const TRealFormat &aFormat) __SOFTFP
/**
Converts the specified floating point number into a character representation 
and copies the conversion into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.
	
The character representation of the real number is dictated by the specified 
format.

Note that the function leaves if the iType data member of the specified
TRealFormat object has both an invalid character representation format
(i.e. the format type) and invalid format flags.	        

@param aVal    The floating point number to be converted. 
@param aFormat The format of the conversion. 

@return If the conversion is successful, the length of this descriptor. If 
        the conversion fails, a negative value indicating the cause of failure.
        In addition, extra information on the cause of the failure may be
        appended onto this descriptor. The possible values and their meaning
        are:
        
        1.KErrArgument - the supplied floating point number is not a valid
          number. The three characters NaN are appended to this descriptor.
          
        2.KErrOverflow - the number is too large to represent.
        2.1 For positive overflow, the three characters Inf are appended 
            to this descriptor.
        2.2 For negative overflow, the four characters -Inf are appended 
	        to this descriptor.
	        
	    3.KErrUnderflow - the number is too small to represent.
	    3.1 For positive underflow, the three characters Inf are appended
	        to this descriptor. 
        3.2	For negative underflow, the four characters -Inf are appended
            to this descriptor. 
	    
	    4.KErrGeneral - the conversion cannot be completed. There are a
	      number of possible reasons for this, but the two most common
	      are:
	    4.1 the maximum number of characters necessary to represent the number,
	        as defined in the TRealFormat object, is greater than the maximum
	        length of this descriptor
	    4.2 the character representation format (i.e. the format type), as
	        defined in the TRealFormat object is not recognised.
	        
@see TRealFormat::iType
*/
	{

	Zero();
	return(rtob(this,aVal,aFormat));
	}

EXPORT_C TInt TDes8::AppendNum(TReal aVal,const TRealFormat &aFormat) __SOFTFP
/**
Converts the specified floating point number into a character representation 
and appends the conversion onto the end of this descriptor's data.

The length of this descriptor is incremented to reflect the new content.
	
The character representation of the real number is dictated by the specified 
format.
	
@param aVal    The floating point number to be converted. 
@param aFormat The format of the conversion. 

@return If the conversion is successful, the length of this descriptor. If 
        the conversion fails, a negative value indicating the cause of failure.
        In addition, extra information on the cause of the failure may be
        appended onto this descriptor. The possible values and their meaning
        are:
        
        1.KErrArgument - the supplied floating point number is not a valid
          number. The three characters NaN are appended to this descriptor.
          
        2.KErrOverflow - the number is too large to represent.
        2.1 For positive overflow, the three characters Inf are appended 
            to this descriptor.
        2.2 For negative overflow, the four characters -Inf are appended 
	        to this descriptor.
	        
	    3.KErrUnderflow - the number is too small to represent.
	    3.1 For positive underflow, the three characters Inf are appended
	        to this descriptor. 
        3.2	For negative underflow, the four characters -Inf are appended
            to this descriptor. 
	    
	    4.KErrGeneral - the conversion cannot be completed. There are a
	      number of possible reasons for this, but the two most common
	      are:
	    4.1 the maximum number of characters necessary to represent the number,
	        as defined in the TRealFormat object, is greater than the maximum
	        length of this descriptor
	    4.2 the character representation format (i.e. the format type), as
	        defined in the TRealFormat object is not recognised
*/
	{

	return(rtob(this,aVal,aFormat));
	}

EXPORT_C TInt TDes16::Num(TReal aVal,const TRealFormat &aFormat) __SOFTFP
/**
Converts the specified floating point number into a character representation 
and copies the conversion into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.
	
The character representation of the real number is dictated by the specified 
format.

Note that the function leaves if the iType data member of the specified
TRealFormat object has both an invalid character representation format
(i.e. the format type) and invalid format flags.	        

@param aVal    The floating point number to be converted. 
@param aFormat The format of the conversion. 

@return If the conversion is successful, the length of this descriptor. If 
        the conversion fails, a negative value indicating the cause of failure.
        In addition, extra information on the cause of the failure may be
        appended onto this descriptor. The possible values and their meaning
        are:
        
        1.KErrArgument - the supplied floating point number is not a valid
          number. The three characters NaN are appended to this descriptor.
          
        2.KErrOverflow - the number is too large to represent.
        2.1 For positive overflow, the three characters Inf are appended 
            to this descriptor.
        2.2 For negative overflow, the four characters -Inf are appended 
	        to this descriptor.
	        
	    3.KErrUnderflow - the number is too small to represent.
	    3.1 For positive underflow, the three characters Inf are appended
	        to this descriptor. 
        3.2	For negative underflow, the four characters -Inf are appended
            to this descriptor. 
	    
	    4.KErrGeneral - the conversion cannot be completed. There are a
	      number of possible reasons for this, but the two most common
	      are:
	    4.1 the maximum number of characters necessary to represent the number,
	        as defined in the TRealFormat object, is greater than the maximum
	        length of this descriptor
	    4.2 the character representation format (i.e. the format type), as
	        defined in the TRealFormat object is not recognised.
	        
@see TRealFormat::iType
*/
	{

	Zero();
	return (AppendNum(aVal,aFormat));
	}

EXPORT_C TInt TDes16::AppendNum(TReal aVal,const TRealFormat &aFormat) __SOFTFP
/**
Converts the specified floating point number into a character representation 
and appends the conversion onto the end of this descriptor's data.

The length of this descriptor is incremented to reflect the new content.
	
The character representation of the real number is dictated by the specified 
format.
	
@param aVal    The floating point number to be converted. 
@param aFormat The format of the conversion. 

@return If the conversion is successful, the length of this descriptor. If 
        the conversion fails, a negative value indicating the cause of failure.
        In addition, extra information on the cause of the failure may be
        appended onto this descriptor. The possible values and their meaning
        are:
        
        1.KErrArgument - the supplied floating point number is not a valid
          number. The three characters NaN are appended to this descriptor.
          
        2.KErrOverflow - the number is too large to represent.
        2.1 For positive overflow, the three characters Inf are appended 
            to this descriptor.
        2.2 For negative overflow, the four characters -Inf are appended 
	        to this descriptor.
	        
	    3.KErrUnderflow - the number is too small to represent.
	    3.1 For positive underflow, the three characters Inf are appended
	        to this descriptor. 
        3.2	For negative underflow, the four characters -Inf are appended
            to this descriptor. 
	    
	    4.KErrGeneral - the conversion cannot be completed. There are a
	      number of possible reasons for this, but the two most common
	      are:
	    4.1 the maximum number of characters necessary to represent the number,
	        as defined in the TRealFormat object, is greater than the maximum
	        length of this descriptor
	    4.2 the character representation format (i.e. the format type), as
	        defined in the TRealFormat object is not recognised
*/
	{

	HBufC8 *temp=HBufC8::New(MaxLength());
	if (temp==NULL)
		return(KErrNoMemory);
	TPtr8 p(temp->Des());
	TInt ret=rtob(&p,aVal,aFormat);
	const TText8 *pTemp=temp->Ptr();
	for (TInt ii=temp->Length();ii>0;ii--)
		Append(*pTemp++);
	if (ret>0)
		ret=Length();
	User::Free(temp);
	return (ret);
	}
