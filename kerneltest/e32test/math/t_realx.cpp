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
// e32test\math\t_realx.cpp
// T_REALX.CPP - Test routines for TRealX
// Overview:
// Test the functionality of a class that encapsulates 64 bit extended precision
// real values.
// API Information:
// TRealX 
// Details:
// - Test the constructor and assignment operator with signed, unsigned, 64 bit
// integer, float and double values.
// - Check the conversion of TRealX value to signed, unsigned, 64 bit integer,
// float and double values is as expected.
// - Set TRealX variable with some special values and check it is as expected.
// - Test addition, multiplication, division of TRealX values is as expected.
// - Test Unary operators, comparison of two TRealX values are as expected.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32test.h>
#include <e32math.h>
#include "t_realxd.h"

LOCAL_D RTest test(_L("T_TREALX"));

// Conversion functions TInt64 <-> TReal64
LOCAL_C void TReal64ToTInt64(TInt64 *aDest, const TReal64 *aSrc)
	{
	TInt *pD=(TInt*)aDest;
	const TInt *pS=(TInt*)aSrc;
#ifdef __DOUBLE_WORDS_SWAPPED__
	pD[1]=pS[0];		// word containing sign,exp
	pD[0]=pS[1];
#else
	pD[1]=pS[1];		// word containing sign,exp
	pD[0]=pS[0];
#endif
	}

LOCAL_C void TInt64ToTReal64(TReal64 *aDest, const TInt64 *aSrc)
	{
	TInt *pD=(TInt*)aDest;
	const TInt *pS=(TInt*)aSrc;
#ifdef __DOUBLE_WORDS_SWAPPED__
	pD[0]=pS[1];		// word containing sign,exp
	pD[1]=pS[0];
#else
	pD[1]=pS[1];		// word containing sign,exp
	pD[0]=pS[0];
#endif
	}

// Test functions
SRealX::SRealX(TUint a, TUint b, TUint c)
	{
	iExpFlagSign=a;
	iMantHigh=b;
	iMantLow=c;
	}

SRealX::SRealX(const TRealX& aRealX)
	{
	const TUint *pR = (const TUint *)&aRealX;
	iExpFlagSign=pR[2];
	iMantHigh=pR[1];
	iMantLow=pR[0];
	}

SRealX& SRealX::operator=(const TRealX& aRealX)
	{
	const TUint *pR = (const TUint *)&aRealX;
	iExpFlagSign=pR[2];
	iMantHigh=pR[1];
	iMantLow=pR[0];
	return *this;
	}

SRealX::operator TRealX() const
	{
	TRealX r;
	TUint *pR=(TUint *)&r;
	pR[2]=iExpFlagSign;
	pR[1]=iMantHigh;
	pR[0]=iMantLow;
	return r;
	}

TBool SRealX::operator==(const SRealX& aSRealX) const
	{
	TUint e1=iExpFlagSign >> 16;
	TUint e2=aSRealX.iExpFlagSign >> 16;
	TUint s1=iExpFlagSign & 0x00000001;
	TUint s2=aSRealX.iExpFlagSign & 0x00000001;
	TUint f1=iExpFlagSign & 0x00000300;
	TUint f2=aSRealX.iExpFlagSign & 0x00000300;
	return( e1==e2 && s1==s2 && (e1==0 || (f1==f2 && iMantHigh==aSRealX.iMantHigh && iMantLow==aSRealX.iMantLow)) );
	}

SConvertFrom32BitTest::SConvertFrom32BitTest(TInt op, const SRealX& res, TInt r)
	{
	iOperand=op;
	iResult=res;
	iReturnCode=r;
	}

void SConvertFrom32BitTest::Test(TConversionFrom32Bits aConversion) const
	{
	TInt r=iReturnCode;
	SRealX sr;
	switch(aConversion)
		{
		case EConstructInt:
			{
			TRealX x(iOperand);
			sr=(SRealX)x;
			break;
			}
		case EAssignInt:
			{
			TRealX x;
			x=iOperand;
			sr=(SRealX)x;
			break;
			}
		case ESetInt:
			{
			TRealX x;
			r=x.Set(iOperand);
			sr=(SRealX)x;
			break;
			}
		case EConstructUint:
			{
			TUint uint=(TUint)iOperand;
			TRealX x(uint);
			sr=(SRealX)x;
			break;
			}
		case EAssignUint:
			{
			TUint uint=(TUint)iOperand;
			TRealX x;
			x=uint;
			sr=(SRealX)x;
			break;
			}
		case ESetUint:
			{
			TUint uint=(TUint)iOperand;
			TRealX x;
			r=x.Set(uint);
			sr=(SRealX)x;
			break;
			}
		case EConstructFloat:
			{
			TReal32 f;
			TInt *pF=(TInt*)&f;
			*pF=iOperand;
			TRealX x(f);
			sr=(SRealX)x;
			break;
			}
		case EAssignFloat:
			{
			TReal32 f;
			TInt *pF=(TInt*)&f;
			*pF=iOperand;
			TRealX x;
			x=f;
			sr=(SRealX)x;
			break;
			}
		case ESetFloat:
			{
			TReal32 f;
			TInt *pF=(TInt*)&f;
			*pF=iOperand;
			TRealX x;
			r=x.Set(f);
			sr=(SRealX)x;
			break;
			}
		}
	if (sr==iResult && r==iReturnCode)
		return;
	test.Printf(_L("Conversion %d from 32 bit operand test failed\noperand = %08X\n"),
		(TInt)aConversion, iOperand );
	test.Printf(_L("Result %08X %08X %08X\nReturn code %d\n"),
		sr.iExpFlagSign, sr.iMantHigh, sr.iMantLow, r );
	test.Printf(_L("Correct result %08X %08X %08X\nCorrect return code %d\n"),
		iResult.iExpFlagSign, iResult.iMantHigh, iResult.iMantLow, iReturnCode );
	//test.Getch();
	test(0);
	}

SConvertFrom64BitTest::SConvertFrom64BitTest(TInt64 op, const SRealX& res, TInt r)
	{
	iOperand=op;
	iResult=res;
	iReturnCode=r;
	}

void SConvertFrom64BitTest::Test(TConversionFrom64Bits aConversion) const
	{
	TInt r=iReturnCode;
	SRealX sr;
	switch(aConversion)
		{
		case EConstructInt64:
			{
			TRealX x(iOperand);
			sr=(SRealX)x;
			break;
			}
		case EAssignInt64:
			{
			TRealX x;
			x=iOperand;
			sr=(SRealX)x;
			break;
			}
		case ESetInt64:
			{
			TRealX x;
			r=x.Set(iOperand);
			sr=(SRealX)x;
			break;
			}
/*		case EConstructUint:
			{
			TUint uint=(TUint)iOperand;
			TRealX x(uint);
			sr=(SRealX)x;
			break;
			}
		case EAssignUint:
			{
			TUint uint=(TUint)iOperand;
			TRealX x;
			x=uint;
			sr=(SRealX)x;
			break;
			}
		case ESetUint:
			{
			TUint uint=(TUint)iOperand;
			TRealX x;
			r=x.Set(uint);
			sr=(SRealX)x;
			break;
			}*/
		case EConstructDouble:
			{
			TReal64 d;
			TInt64ToTReal64(&d, &iOperand);
			TRealX x(d);
			sr=(SRealX)x;
			break;
			}
		case EAssignDouble:
			{
			TReal64 d;
			TInt64ToTReal64(&d, &iOperand);
			TRealX x;
			x=d;
			sr=(SRealX)x;
			break;
			}
		case ESetDouble:
			{
			TReal64 d;
			TInt64ToTReal64(&d, &iOperand);
			TRealX x;
			r=x.Set(d);
			sr=(SRealX)x;
			break;
			}
		}
	if (sr==iResult && r==iReturnCode)
		return;
	test.Printf(_L("Conversion %d from 64 bit operand test failed\noperand = %08X %08X\n"),
		(TInt)aConversion, I64HIGH(iOperand), I64LOW(iOperand) );
	test.Printf(_L("Result %08X %08X %08X\nReturn code %d\n"),
		sr.iExpFlagSign, sr.iMantHigh, sr.iMantLow, r );
	test.Printf(_L("Correct result %08X %08X %08X\nCorrect return code %d\n"),
		iResult.iExpFlagSign, iResult.iMantHigh, iResult.iMantLow, iReturnCode );
	//test.Getch();
	test(0);
	}

SConvertTo32BitTest::SConvertTo32BitTest(const SRealX& op, TInt res, TInt r)
	{
	iOperand=op;
	iResult=res;
	iReturnCode=r;
	}

void SConvertTo32BitTest::Test(TConversionTo32Bits aConversion) const
	{
	TInt r=iReturnCode;
	TRealX op=(TRealX)iOperand;
	TInt result=0;
	switch(aConversion)
		{
		case EOperatorInt:
			result=(TInt)op;
			break;
		case EOperatorUint:
			{
			TUint uint;
			uint=(TUint)op;
			result=(TInt)uint;
			break;
			}
		case EOperatorTReal32:
			{
			TReal32 x;
			x=(TReal32)op;
			result=*((TInt*)&x);
			break;
			}
		case EGetTReal32:
			{
			TReal32 x;
			r=op.GetTReal(x);
			result=*((TInt*)&x);
			break;
			}
		}
	if (result==iResult && r==iReturnCode)
		return;
	test.Printf(_L("Conversion %d to 32 bit operand test failed\noperand = %08X %08X %08X\n"),
		(TInt)aConversion, iOperand.iExpFlagSign, iOperand.iMantHigh, iOperand.iMantLow );
	test.Printf(_L("Result %08X\nReturn code %d\n"),
		result, r );
	test.Printf(_L("Correct result %08X\nCorrect return code %d\n"),
		iResult, iReturnCode );
	//test.Getch();
	test(0);
	}

SConvertTo64BitTest::SConvertTo64BitTest(const SRealX& op, TInt64 res, TInt r)
	{
	iOperand=op;
	iResult=res;
	iReturnCode=r;
	}

void SConvertTo64BitTest::Test(TConversionTo64Bits aConversion) const
	{
	TInt r=iReturnCode;
	TRealX op=(TRealX)iOperand;
	TInt64 result=0;
	switch(aConversion)
		{
		case EOperatorInt64:
			result=op.operator TInt64();  // odd conversion syntax required for VC5 compilation
			break;
		case EOperatorTReal64:
			{
			TReal64 d;
			d=(TReal64)op;
			TReal64ToTInt64(&result, &d);
			break;
			}
		case EGetTReal64:
			{
			TReal64 d;
			r=op.GetTReal(d);
			TReal64ToTInt64(&result, &d);
			break;
			}
		}
	if (result==iResult && r==iReturnCode)
		return;
	test.Printf(_L("Conversion %d to 64 bit operand test failed\noperand = %08X %08X %08X\n"),
		(TInt)aConversion, iOperand.iExpFlagSign, iOperand.iMantHigh, iOperand.iMantLow );
	test.Printf(_L("Result %08X %08X\nReturn code %d\n"),
		I64HIGH(result), I64LOW(result), r );
	test.Printf(_L("Correct result %08X %08X\nCorrect return code %d\n"),
		I64HIGH(iResult), I64LOW(iResult), iReturnCode );
	//test.Getch();
	test(0);
	}

SCompareTest::SCompareTest(const SRealX& o1, const SRealX& o2, TInt r)
	{
	iOperand1=o1;
	iOperand2=o2;
	iReturnCode=r;
	}

void SCompareTest::Test() const
	{
	TRealX op1=(TRealX)iOperand1;
	TRealX op2=(TRealX)iOperand2;
	TRealX::TRealXOrder r=op1.Compare(op2);
	TBool lt=op1<op2;
	TBool le=op1<=op2;
	TBool gt=op1>op2;
	TBool ge=op1>=op2;
	TBool eq=op1==op2;
	TBool ne=op1!=op2;
	if ((TInt)r==iReturnCode)
		{
		switch(r)
			{
			case TRealX::ELessThan:
				if (lt && le && !gt && !ge && !eq && ne)
					return;
				break;

			case TRealX::EEqual:
				if (!lt && le && !gt && ge && eq && !ne)
					return;
				break;
			case TRealX::EGreaterThan:
				if (!lt && !le && gt && ge && !eq && ne)
					return;
				break;

			case TRealX::EUnordered:
				if (!lt && !le && !gt && !ge && !eq && ne)
					return;
				break;
			}
		}
	test.Printf(_L("Compare test failed\nop1 = %08X %08X %08X\nop2 = %08X %08X %08X\n"),
		iOperand1.iExpFlagSign, iOperand1.iMantHigh, iOperand1.iMantLow,
		iOperand2.iExpFlagSign, iOperand2.iMantHigh, iOperand2.iMantLow );
	test.Printf(_L("Return code %d\nlt=%d, le=%d, gt=%d, ge=%d, eq=%d, ne=%d\n"),
		r, lt, le, gt, ge, eq, ne );
	//test.Getch();
	test(0);
	}

SOneOpTest::SOneOpTest(const SRealX& op, const SRealX& res, TInt r)
	{
	iOperand=op;
	iResult=res;
	iReturnCode=r;
	}

TInt SOneOpTest::DoTest(TUnaryOperation anOperation, TRealX *aResult) const
	{
	TInt r=KErrNone;
	TRealX op;
	*aResult=(TRealX)iOperand;

	switch(anOperation)
		{
		case EUnaryPlus:
			*aResult=+(*aResult);
			break;
		case EUnaryMinus:
			*aResult=-(*aResult);
			break;
		case EPreInc:
			++*aResult;
			break;
		case EPreDec:
			--*aResult;
			break;
		case EPostInc:
			op=(*aResult)++;
			if (!(SRealX(op)==iOperand))
				r=KErrGeneral;
			break;
		case EPostDec:
			op=(*aResult)--;
			if (!(SRealX(op)==iOperand))
				r=KErrGeneral;
			break;
		}
	return r;
	}

TInt OneOpTestThreadFunction(TAny *anInfo)
	{
	SOneOpTestThreadInfo *pI=(SOneOpTestThreadInfo *)anInfo;
	TInt r=pI->iTest->DoTest(pI->iOperation,pI->iResult);
	return r;
	}

void SOneOpTest::Test(TUnaryOperation anOperation) const
	{
	SOneOpTestThreadInfo info;
	TRealX result;
	info.iTest=this;
	info.iOperation=anOperation;
	info.iResult=&result;
	RThread t;
	TInt r=t.Create(_L("TestThread"),OneOpTestThreadFunction,0x1000,0x1000,0x100000,&info);
	test(r==KErrNone);
	t.SetPriority(EPriorityMore);	// so we will not run again until thread terminates
	TRequestStatus s;
	t.Logon(s);
	t.Resume();
	User::WaitForRequest(s);
	TExitType exittype=t.ExitType();
	r=s.Int();
	CLOSE_AND_WAIT(t);
	SRealX sr(result);
	if (sr==iResult && r==iReturnCode)
		{
		if (anOperation>EUnaryMinus)
			{
			if (r==KErrNone && exittype==EExitKill)
				return;
			if (r!=KErrNone && exittype==EExitPanic)
				return;
			}
		else
			{
			if (exittype==EExitKill)
				return;
			}
		}
	
	test.Printf(_L("Unary operation %d test failed\nop = %08X %08X %08X\n"),
		(TInt)anOperation,
		iOperand.iExpFlagSign, iOperand.iMantHigh, iOperand.iMantLow );
	test.Printf(_L("Result %08X %08X %08X\nReturn code %d Exit type %d\n"),
		sr.iExpFlagSign, sr.iMantHigh, sr.iMantLow, r, (TInt)exittype );
	test.Printf(_L("Correct result %08X %08X %08X\nCorrect return code %d\n"),
		iResult.iExpFlagSign, iResult.iMantHigh, iResult.iMantLow, iReturnCode );
	//test.Getch();
	test(0);
	}

STwoOpTest::STwoOpTest(const SRealX& o1, const SRealX& o2, const SRealX& res, TInt r)
	{
	iOperand1=o1;
	iOperand2=o2;
	iResult=res;
	iReturnCode=r;
	}

TInt STwoOpTest::DoTest(TBinaryOperation anOperation, TRealX *aResult, TBool aSwap) const
	{
	TInt r=KErrNone;
	TRealX op1, op2;
	if (aSwap)
		{
		op2=(TRealX)iOperand1;
		op1=(TRealX)iOperand2;
		*aResult=(TRealX)iOperand2;
		}
	else
		{
		op1=(TRealX)iOperand1;
		op2=(TRealX)iOperand2;
		*aResult=(TRealX)iOperand1;
		}

	switch(anOperation)
		{
		case EAddEq:
			r=aResult->AddEq(op2);
			break;
		case ESubEq:
			r=aResult->SubEq(op2);
			break;
		case EMultEq:
			r=aResult->MultEq(op2);
			break;
		case EDivEq:
			r=aResult->DivEq(op2);
			break;
		case EAdd:
			r=op1.Add(*aResult,op2);
			break;
		case ESub:
			r=op1.Sub(*aResult,op2);
			break;
		case EMult:
			r=op1.Mult(*aResult,op2);
			break;
		case EDiv:
			r=op1.Div(*aResult,op2);
			break;
		case EPlusEq:
			*aResult+=op2;
			break;
		case EMinusEq:
			*aResult-=op2;
			break;
		case EStarEq:
			*aResult*=op2;
			break;
		case ESlashEq:
			*aResult/=op2;
			break;
		case EPlus:
			*aResult=op1+op2;
			break;
		case EMinus:
			*aResult=op1-op2;
			break;
		case EStar:
			*aResult=op1*op2;
			break;
		case ESlash:
			*aResult=op1/op2;
			break;
		}
	return r;
	}

TInt TwoOpTestThreadFunction(TAny *anInfo)
	{
	STwoOpTestThreadInfo *pI=(STwoOpTestThreadInfo *)anInfo;
	TInt r=pI->iTest->DoTest(pI->iOperation,pI->iResult,pI->iSwap);
	return r;
	}

void STwoOpTest::Test(TBinaryOperation anOperation, TBool aSwap) const
	{
	STwoOpTestThreadInfo info;
	TRealX result;
	info.iTest=this;
	info.iOperation=anOperation;
	info.iResult=&result;
	info.iSwap=aSwap;
	RThread t;
	TInt r=t.Create(_L("TestThread"),TwoOpTestThreadFunction,0x1000,0x1000,0x100000,&info);
	test(r==KErrNone);
	t.SetPriority(EPriorityMore);	// so we will not run again until thread terminates
	TRequestStatus s;
	t.Logon(s);
	t.Resume();
	User::WaitForRequest(s);
	TExitType exittype=t.ExitType();
	r=s.Int();
	CLOSE_AND_WAIT(t);
	SRealX sr(result);
	if (anOperation>=EPlus && exittype==EExitPanic)
		{
		// if +,-,*,/ operation paniced, result will be lost
		sr=iResult;
		}
	if (sr==iResult && r==iReturnCode)
		{
		if (anOperation>=EPlusEq)
			{
			if (r==KErrNone && exittype==EExitKill)
				return;
			if (r!=KErrNone && exittype==EExitPanic)
				return;
			}
		else
			{
			if (exittype==EExitKill)
				return;
			}
		}
	
	test.Printf(_L("Binary operation %d test failed\nop1 = %08X %08X %08X\nop2 = %08X %08X %08X\n"),
		(TInt)anOperation,
		iOperand1.iExpFlagSign, iOperand1.iMantHigh, iOperand1.iMantLow,
		iOperand2.iExpFlagSign, iOperand2.iMantHigh, iOperand2.iMantLow );
	test.Printf(_L("Result %08X %08X %08X\nReturn code %d Exit type %d\n"),
		sr.iExpFlagSign, sr.iMantHigh, sr.iMantLow, r, (TInt)exittype );
	test.Printf(_L("Correct result %08X %08X %08X\nCorrect return code %d\n"),
		iResult.iExpFlagSign, iResult.iMantHigh, iResult.iMantLow, iReturnCode );
	//test.Getch();
	test(0);
	}

SSpecialValueTest::SSpecialValueTest(const SRealX& op, TInt aResults)
	{
	iOperand=op;
	iIsZero=aResults & 8;
	iIsNaN=aResults & 4;
	iIsInfinite=aResults & 2;
	iIsFinite=aResults & 1;
	}

LOCAL_C TBool same(TBool a, TBool b)
	{
	return( (a && b) || (!a && !b) );
	}

void SSpecialValueTest::Test() const
	{
	TRealX x=(TRealX)iOperand;
	TBool isZero=x.IsZero();
	TBool isNaN=x.IsNaN();
	TBool isInfinite=x.IsInfinite();
	TBool isFinite=x.IsFinite();
	if ( same(isZero,iIsZero) && same(isNaN,iIsNaN) && same(isInfinite,iIsInfinite) && same(isFinite,iIsFinite) )
		return;
	test.Printf(_L("Special value test failed\nOperand %08X %08X %08X\n"),
		iOperand.iExpFlagSign, iOperand.iMantHigh, iOperand.iMantLow );
	test.Printf(_L("Results IsZero=%d, IsNaN=%d, IsInfinite=%d, IsFinite=%d "),
		isZero, isNaN, isInfinite, isFinite );
	test.Printf(_L("Correct results IsZero=%d, IsNaN=%d, IsInfinite=%d, IsFinite=%d "),
		iIsZero, iIsNaN, iIsInfinite, iIsFinite );
	//test.Getch();
	test(0);
	}

LOCAL_C void TestAssignConstruct()
	{
	TInt i;

	// test default constructor
	TRealX z;
	SRealX sz(z);
	test(sz.iExpFlagSign==0);
	test(sz.iMantHigh==0);
	test(sz.iMantLow==0);

	for (i=0; i<NumConvertFromIntTests; i++)
		{
		const SConvertFrom32BitTest *pT=&(ConvertFromIntTests[i]);
		pT->Test(EConstructInt);
		pT->Test(EAssignInt);
		pT->Test(ESetInt);
		}
	for (i=0; i<NumConvertFromUintTests; i++)
		{
		const SConvertFrom32BitTest *pT=&(ConvertFromUintTests[i]);
		pT->Test(EConstructUint);
		pT->Test(EAssignUint);
		pT->Test(ESetUint);
		}
	for (i=0; i<NumConvertFromInt64Tests; i++)
		{
		const SConvertFrom64BitTest *pT=&(ConvertFromInt64Tests[i]);
		pT->Test(EConstructInt64);
		pT->Test(EAssignInt64);
		pT->Test(ESetInt64);
		}
	for (i=0; i<NumConvertFromFloatTests; i++)
		{
		const SConvertFrom32BitTest *pT=&(ConvertFromFloatTests[i]);
		pT->Test(EConstructFloat);
		pT->Test(EAssignFloat);
		pT->Test(ESetFloat);
		}
	for (i=0; i<NumConvertFromDoubleTests; i++)
		{
		const SConvertFrom64BitTest *pT=&(ConvertFromDoubleTests[i]);
		pT->Test(EConstructDouble);
		pT->Test(EAssignDouble);
		pT->Test(ESetDouble);
		}
	}

LOCAL_C void TestConversions()
	{
	TInt i;
	for (i=0; i<NumConvertToIntTests; i++)
		{
		const SConvertTo32BitTest *pT=&(ConvertToIntTests[i]);
		pT->Test(EOperatorInt);
		}
	for (i=0; i<NumConvertToUintTests; i++)
		{
		const SConvertTo32BitTest *pT=&(ConvertToUintTests[i]);
		pT->Test(EOperatorUint);
		}
	for (i=0; i<NumConvertToInt64Tests; i++)
		{
		const SConvertTo64BitTest *pT=&(ConvertToInt64Tests[i]);
		pT->Test(EOperatorInt64);
		}
	for (i=0; i<NumConvertToFloatTests; i++)
		{
		const SConvertTo32BitTest *pT=&(ConvertToFloatTests[i]);
		pT->Test(EOperatorTReal32);
		pT->Test(EGetTReal32);
		}
	for (i=0; i<NumConvertToDoubleTests; i++)
		{
		const SConvertTo64BitTest *pT=&(ConvertToDoubleTests[i]);
		pT->Test(EOperatorTReal64);
		pT->Test(EGetTReal64);
		}
	}

LOCAL_C void TestSpecials()
	{
	TRealX x;
	SRealX sx;
	x.SetInfinite(EFalse);
	sx=x;
	test(sx.iExpFlagSign==0xFFFF0000);
	test(sx.iMantHigh==0x80000000);
	test(sx.iMantLow==0);
	x.SetZero();
	sx=x;
	test(sx.iExpFlagSign==0x00000000);
	x.SetInfinite(ETrue);
	sx=x;
	test(sx.iExpFlagSign==0xFFFF0001);
	test(sx.iMantHigh==0x80000000);
	test(sx.iMantLow==0);
	x.SetNaN();
	sx=x;
	test(sx.iExpFlagSign==0xFFFF0001);
	test(sx.iMantHigh==0xC0000000);
	test(sx.iMantLow==0);
	x.SetZero(ETrue);
	sx=x;
	test(sx.iExpFlagSign==0x00000001);

	TInt i;
	for(i=0; i<NumSpecialValueTests; i++)
		{
		const SSpecialValueTest *pT=&(SpecialValueTests[i]);
		pT->Test();
		}
	}

LOCAL_C void TestUnaryOperators()
	{
	TInt i;
	for (i=0; i<NumUnaryPlusTests; i++)
		{
		const SOneOpTest *pT=&(UnaryPlusTests[i]);
		pT->Test(EUnaryPlus);
		}
	for (i=0; i<NumUnaryMinusTests; i++)
		{
		const SOneOpTest *pT=&(UnaryMinusTests[i]);
		pT->Test(EUnaryMinus);
		}
	for (i=0; i<NumIncTests; i++)
		{
		const SOneOpTest *pT=&(IncTests[i]);
		pT->Test(EPreInc);
		pT->Test(EPostInc);
		}
	for (i=0; i<NumDecTests; i++)
		{
		const SOneOpTest *pT=&(DecTests[i]);
		pT->Test(EPreDec);
		pT->Test(EPostDec);
		}
	}

LOCAL_C void TestAddition()
	{
	TInt i;
	for (i=0; i<NumAdditionTests; i++)
		{
		const STwoOpTest *pT=&(AdditionTests[i]);
		pT->Test(EAddEq,EFalse);
		pT->Test(EAddEq,ETrue);
		pT->Test(EAdd,EFalse);
		pT->Test(EAdd,ETrue);
		pT->Test(EPlusEq,EFalse);
		pT->Test(EPlusEq,ETrue);
		pT->Test(EPlus,EFalse);
		pT->Test(EPlus,ETrue);
		}
	for (i=0; i<NumBinaryOpNaNTests; i++)
		{
		const STwoOpTest *pT=&(BinaryOpNaNTests[i]);
		pT->Test(EAddEq,EFalse);
		pT->Test(EAddEq,ETrue);
		pT->Test(EAdd,EFalse);
		pT->Test(EAdd,ETrue);
		pT->Test(EPlusEq,EFalse);
		pT->Test(EPlusEq,ETrue);
		pT->Test(EPlus,EFalse);
		pT->Test(EPlus,ETrue);
		}
	}

LOCAL_C void TestMultiplication()
	{
	TInt i;
	for (i=0; i<NumMultiplicationTests; i++)
		{
		const STwoOpTest *pT=&(MultiplicationTests[i]);
		pT->Test(EMultEq,EFalse);
		pT->Test(EMultEq,ETrue);
		pT->Test(EMult,EFalse);
		pT->Test(EMult,ETrue);
		pT->Test(EStarEq,EFalse);
		pT->Test(EStarEq,ETrue);
		pT->Test(EStar,EFalse);
		pT->Test(EStar,ETrue);
		}
	for (i=0; i<NumBinaryOpNaNTests; i++)
		{
		const STwoOpTest *pT=&(BinaryOpNaNTests[i]);
		pT->Test(EMultEq,EFalse);
		pT->Test(EMultEq,ETrue);
		pT->Test(EMult,EFalse);
		pT->Test(EMult,ETrue);
		pT->Test(EStarEq,EFalse);
		pT->Test(EStarEq,ETrue);
		pT->Test(EStar,EFalse);
		pT->Test(EStar,ETrue);
		}
	}

LOCAL_C void TestDivision()
	{
	TInt i;
	for (i=0; i<NumDivisionTests; i++)
		{
		const STwoOpTest *pT=&(DivisionTests[i]);
		pT->Test(EDivEq,EFalse);
		pT->Test(EDiv,EFalse);
		pT->Test(ESlashEq,EFalse);
		pT->Test(ESlash,EFalse);
		}
	for (i=0; i<NumBinaryOpNaNTests; i++)
		{
		const STwoOpTest *pT=&(BinaryOpNaNTests[i]);
		pT->Test(EDivEq,EFalse);
		pT->Test(EDiv,EFalse);
		pT->Test(ESlashEq,EFalse);
		pT->Test(ESlash,EFalse);
		}
	}

LOCAL_C void TestComparison()
	{
	TInt i;
	for (i=0; i<NumComparisonTests; i++)
		{
		const SCompareTest *pT=&(ComparisonTests[i]);
		pT->Test();
		}
	}


GLDEF_C TInt E32Main()
//
//	Test TRealX
//
    {

	User::SetJustInTime(EFalse);
	test.Title();
	test.Start(_L("Assignment Operator and Constructors"));	
	TestAssignConstruct();
	test.Next(_L("Conversions"));
	TestConversions();
	test.Next(_L("Setting and Checking Specials"));
	TestSpecials();
	test.Next(_L("Addition"));
	TestAddition();
	test.Next(_L("Multiplication"));
	TestMultiplication();
	test.Next(_L("Division"));
	TestDivision();
	test.Next(_L("Unary Operators"));
	TestUnaryOperators();
	test.Next(_L("Comparisons"));
	TestComparison();
	
	test.End();
	return(KErrNone);
    }
