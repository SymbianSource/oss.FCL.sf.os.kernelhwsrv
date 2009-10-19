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
// e32test\math\t_realxd.h
// T_REALXD.H - Structure definitions for TRealX test
// 
//


#ifndef __REALXTST_H__
#define __REALXTST_H__

#include <e32std.h>
#include <e32math.h>

// Some constants
const TUint KExpFlagSignMask = 0xFFFF0301;

// Structure definitions
struct SRealX
	{
	SRealX() {}
	SRealX(TUint a, TUint b, TUint c);
	SRealX(const TRealX& aRealX);
	SRealX& operator=(const TRealX& aRealX);
	operator TRealX() const;
	TBool operator==(const SRealX& aSRealX) const;

	TUint iExpFlagSign;
	TUint iMantHigh;
	TUint iMantLow;
	};

enum TConversionFrom32Bits
	{
	EConstructInt=0, EAssignInt=1, ESetInt=2,
	EConstructUint=3, EAssignUint=4, ESetUint=5,
	EConstructFloat=6, EAssignFloat=7, ESetFloat=8
	};

struct SConvertFrom32BitTest
	{
	SConvertFrom32BitTest(TInt op, const SRealX& res, TInt r);
	void Test(TConversionFrom32Bits aConversion) const;

	TInt iOperand;
	SRealX iResult;
	TInt iReturnCode;
	};

enum TConversionFrom64Bits
	{
	EConstructInt64=0, EAssignInt64=1, ESetInt64=2,
	EConstructDouble=3, EAssignDouble=4, ESetDouble=5
	};

struct SConvertFrom64BitTest
	{
	SConvertFrom64BitTest(TInt64 op, const SRealX& res, TInt r);
	void Test(TConversionFrom64Bits aConversion) const;

	TInt64 iOperand;
	SRealX iResult;
	TInt iReturnCode;
	};

enum TConversionTo32Bits
	{
	EOperatorInt=0, EOperatorUint=1,
	EOperatorTReal32=2, EGetTReal32=3
	};

struct SConvertTo32BitTest
	{
	SConvertTo32BitTest(const SRealX& op, TInt res, TInt r);
	void Test(TConversionTo32Bits aConversion) const;

	SRealX iOperand;
	TInt iResult;
	TInt iReturnCode;
	};

enum TConversionTo64Bits
	{
	EOperatorInt64=0,
	EOperatorTReal64=1, EGetTReal64=2
	};

struct SConvertTo64BitTest
	{
	SConvertTo64BitTest(const SRealX& op, TInt64 res, TInt r);
	void Test(TConversionTo64Bits aConversion) const;

	SRealX iOperand;
	TInt64 iResult;
	TInt iReturnCode;
	};

enum TUnaryOperation
	{
	EUnaryPlus=0, EUnaryMinus=1,
	EPreInc=2, EPreDec=3, EPostInc=4, EPostDec=5
	};

struct SOneOpTest
	{
	SOneOpTest(const SRealX& op, const SRealX& res, TInt r);
	void Test(TUnaryOperation anOperation) const;
	TInt DoTest(TUnaryOperation anOperation, TRealX *aResult) const;

	SRealX iOperand;
	SRealX iResult;
	TInt iReturnCode;
	};

struct SOneOpTestThreadInfo
	{
	const SOneOpTest *iTest;
	TUnaryOperation iOperation;
	TRealX *iResult;
	};

struct SCompareTest
	{
	SCompareTest(const SRealX& o1, const SRealX& o2, TInt r);
	void Test() const;

	SRealX iOperand1;
	SRealX iOperand2;
	TInt iReturnCode;
	};

enum TBinaryOperation
	{
	EAddEq=0, ESubEq=1, EMultEq=2, EDivEq=3,			// TInt TRealX::AddEq() etc
	EAdd=4, ESub=5, EMult=6, EDiv=7,					// TInt TRealX::Add() etc
	EPlusEq=8, EMinusEq=9, EStarEq=10, ESlashEq=11,		// += -= *= /=
	EPlus=12, EMinus=13, EStar=14, ESlash=15			// + - * /
	};

struct STwoOpTest
	{
	STwoOpTest(const SRealX& o1, const SRealX& o2, const SRealX& res, TInt r);
	void Test(TBinaryOperation anOperation, TBool aSwap=EFalse) const;
	TInt DoTest(TBinaryOperation anOperation, TRealX *aResult, TBool aSwap) const;

	SRealX iOperand1;
	SRealX iOperand2;
	SRealX iResult;
	TInt iReturnCode;
	};

struct STwoOpTestThreadInfo
	{
	const STwoOpTest *iTest;
	TBinaryOperation iOperation;
	TRealX *iResult;
	TBool iSwap;
	};

struct SSpecialValueTest
	{
	SSpecialValueTest(const SRealX& op, TInt aResults);
	void Test() const;

	SRealX iOperand;
	TBool iIsZero;
	TBool iIsNaN;
	TBool iIsInfinite;
	TBool iIsFinite;
	};

GLREF_D const TInt NumBinaryOpNaNTests;
GLREF_D const STwoOpTest BinaryOpNaNTests[];

GLREF_D const TInt NumAdditionTests;
GLREF_D const STwoOpTest AdditionTests[];

GLREF_D const TInt NumMultiplicationTests;
GLREF_D const STwoOpTest MultiplicationTests[];

GLREF_D const TInt NumDivisionTests;
GLREF_D const STwoOpTest DivisionTests[];

GLREF_D const TInt NumComparisonTests;
GLREF_D const SCompareTest ComparisonTests[];

GLREF_D const TInt NumUnaryPlusTests;
GLREF_D const SOneOpTest UnaryPlusTests[];

GLREF_D const TInt NumUnaryMinusTests;
GLREF_D const SOneOpTest UnaryMinusTests[];

GLREF_D const TInt NumIncTests;
GLREF_D const SOneOpTest IncTests[];

GLREF_D const TInt NumDecTests;
GLREF_D const SOneOpTest DecTests[];

GLREF_D const TInt NumConvertFromIntTests;
GLREF_D const SConvertFrom32BitTest ConvertFromIntTests[];

GLREF_D const TInt NumConvertFromUintTests;
GLREF_D const SConvertFrom32BitTest ConvertFromUintTests[];

GLREF_D const TInt NumConvertFromFloatTests;
GLREF_D const SConvertFrom32BitTest ConvertFromFloatTests[];

GLREF_D const TInt NumConvertFromInt64Tests;
GLREF_D const SConvertFrom64BitTest ConvertFromInt64Tests[];

GLREF_D const TInt NumConvertFromDoubleTests;
GLREF_D const SConvertFrom64BitTest ConvertFromDoubleTests[];

GLREF_D const TInt NumConvertToIntTests;
GLREF_D const SConvertTo32BitTest ConvertToIntTests[];

GLREF_D const TInt NumConvertToUintTests;
GLREF_D const SConvertTo32BitTest ConvertToUintTests[];

GLREF_D const TInt NumConvertToInt64Tests;
GLREF_D const SConvertTo64BitTest ConvertToInt64Tests[];

GLREF_D const TInt NumConvertToFloatTests;
GLREF_D const SConvertTo32BitTest ConvertToFloatTests[];

GLREF_D const TInt NumConvertToDoubleTests;
GLREF_D const SConvertTo64BitTest ConvertToDoubleTests[];

GLREF_D const TInt NumSpecialValueTests;
GLREF_D const SSpecialValueTest SpecialValueTests[];

#endif
