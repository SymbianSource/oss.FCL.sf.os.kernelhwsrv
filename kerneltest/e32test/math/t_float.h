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
// e32test\math\t_float.h
// Header file for T_FLOAT.CPP
// 
//


#if !defined(__T_FLTCL_H__)
#define __T_FLTCL_H__

#if !defined(__E32STD_H__)
#include <e32std.h> 
#endif

#include "t_vals.h"

//Added by AnnW
const TInt fixedLimitAndTriads=KRealFormatFixed | KGeneralLimit;
const TInt fixedNoTriads=KRealFormatFixed | KDoNotUseTriads;
const TInt fixedLimit=fixedNoTriads | KGeneralLimit;
const TInt fixedSpaceForSign=fixedNoTriads | KExtraSpaceForSign;
const TInt fixedTriadsAndSign=KRealFormatFixed | KExtraSpaceForSign; 
const TInt fixedLimitAndSpaceForSign=fixedLimit |KExtraSpaceForSign;
const TInt exponentThreeDigitExp=KRealFormatExponent | KAllowThreeDigitExp;
const TInt exponentLimit=KRealFormatExponent | KGeneralLimit;
const TInt exponentLimitAndThreeDigExp=exponentLimit | KAllowThreeDigitExp;
const TInt generalSpaceForSign=KRealFormatGeneral | KExtraSpaceForSign;
const TInt generalThreeDigitExp=KRealFormatGeneral | KAllowThreeDigitExp;
const TInt generalLimit=KRealFormatGeneral | KGeneralLimit;
const TInt noExponentNoTriads=KRealFormatNoExponent | KDoNotUseTriads;
const TInt noExponentLimit=noExponentNoTriads | KGeneralLimit;
const TInt calculatorThreeDigitExp=KRealFormatCalculator | KAllowThreeDigitExp;
const TInt calculatorLimit=KRealFormatCalculator | KGeneralLimit;
const TInt calculatorLimitAndThreeDigExp=calculatorLimit | KAllowThreeDigitExp;
const TInt noExponentCalc=noExponentNoTriads | KExtraSpaceForSign;
//
// new - 17/3/97
const TInt exponentThreeDigitExpAndSigFigs=KRealFormatExponent | KAllowThreeDigitExp | KUseSigFigs;
const TInt exponentSigFigs=KRealFormatExponent | KUseSigFigs;


// classes used by T_FLOAT.CPP

class ROUND_TEST
	{
public:
    TReal num; // input number 
    TInt plcs; // number of places to be rounded 
    TReal res; // expected result 
    };

class RtoB_TEST
    {
public: 
	RtoB_TEST();
	RtoB_TEST(TReal aNum,TInt aType,TInt aWidth,TInt aDecimals,TChar aPoint,TChar aTriad,TInt aTrilen,const TText* aRes);
    TReal num; // input number 
    TRealFormat format; // format 
    const TText* res; // expected result 
    };

class ERR_TEST
	{
public:
	ERR_TEST(TReal aNum,TInt aType,TInt aWidth,TInt aDecimals,TChar aPoint,TChar aTriad, TInt aTrilen,const TText* aRes=_S(""),TInt aErr=KErrGeneral);
	RtoB_TEST rtob;
	TInt err;
	};

class DtoR_TEST64
    {
public:
	DtoR_TEST64(const TDesC& aDes,const TReal aRes) : iLex(aDes),iRes(aRes) {}
    TLex iLex ;	// input string
    TReal iRes;	// expected result
    };

class DtoR_TEST32
    {
public:
	DtoR_TEST32(const TDesC& aDes,const TReal32 aRes) : iLex(aDes),iRes(aRes) {}
    TLex iLex ;	// input string
    TReal32 iRes;	// expected result
    };

class DtoR_ERR_TEST
	{
public:
	DtoR_ERR_TEST(const TDesC& aDes,const TInt aErr) : iLex(aDes),iErr(aErr) {}
	TLex iLex;
	TInt iErr;
	};

class CALC_TEST
	{
public:
	CALC_TEST(const TDesC& aDes,TInt aType,TInt aWidth,TInt aDecimals,TChar aPoint,TChar aTriad,TInt aTrilen,const TText* aRes);
	TLex iLex;
	TRealFormat iFormat; 
    const TText* iRes; 
	};

#endif
