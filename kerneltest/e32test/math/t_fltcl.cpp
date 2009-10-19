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
// e32test\math\t_fltcl.cpp
// Class member functions for T_FLOAT.CPP
// T_FLOAT.CPP split into header files because there seems to be too much static
// data for GCC compiler
// 
//


#include "t_float.h"

RtoB_TEST::RtoB_TEST()
	{

	num=0.0;
	format.iType=KRealFormatGeneral;
	format.iWidth=KDefaultRealWidth;
	format.iPlaces=0;
	TLocale locale;
	format.iPoint=locale.DecimalSeparator();
	format.iTriad=locale.ThousandsSeparator();
	format.iTriLen=1;
	res=_S("0");
	}

RtoB_TEST::RtoB_TEST(TReal aNum,TInt aType,TInt aWidth,TInt aDecimals,TChar aPoint,TChar aTriad,TInt aTrilen,const TText* aRes)
	{

	num=aNum;
	format.iType=aType;
	format.iWidth=aWidth;
	format.iPlaces=aDecimals;
	format.iPoint=aPoint;
	format.iTriad=aTriad;
	format.iTriLen=aTrilen;
	res=aRes;
	}

ERR_TEST::ERR_TEST(TReal aNum,TInt aType,TInt aWidth,TInt aDecimals,TChar aPoint,TChar aTriad, TInt aTrilen,const TText* aRes,TInt aErr)
	{

	rtob.num=aNum;
	rtob.format.iType=aType;
	rtob.format.iWidth=aWidth;
	rtob.format.iPlaces=aDecimals;
	rtob.format.iPoint=aPoint;
	rtob.format.iTriad=aTriad;
	rtob.format.iTriLen=aTrilen;
	rtob.res=aRes;
	err=aErr;	
	}

CALC_TEST::CALC_TEST(const TDesC& aDes,TInt aType,TInt aWidth,TInt aDecimals,TChar aPoint,TChar aTriad,TInt aTrilen,const TText* aRes)
	: iLex(aDes)
	{
	iFormat.iType=aType;
	iFormat.iWidth=aWidth;
	iFormat.iPlaces=aDecimals;
	iFormat.iPoint=aPoint;
	iFormat.iTriad=aTriad;
	iFormat.iTriLen=aTrilen;
	iRes=aRes;
	}
