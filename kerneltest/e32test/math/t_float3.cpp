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
// e32test\math\t_float3.cpp
// File for T_FLOAT.CPP containing data	for test5, test6, test7 and test8
// T_FLOAT.CPP split into header files because there seems to be too much static data for GCC
// compiler
// 
//

#include "t_float.h"
#include <e32math.h>

GLDEF_D DtoR_TEST64 testd2[]=
    {
	DtoR_TEST64(_L("1.000000000000000000000001"),1.0),
	DtoR_TEST64(_L("1.00000000000000000000001"),1.0),
	DtoR_TEST64(_L("1.0000000000000000000001"),1.0),
	DtoR_TEST64(_L("1.000000000000000000001"),1.0),
	DtoR_TEST64(_L("1.0000000000000000001"),1.0),
	DtoR_TEST64(_L("1.000000000000000001"),1.0),
	DtoR_TEST64(_L("1.000000000000001"),1.000000000000001),
	DtoR_TEST64(_L("1.0000000000000001E+10"),1.0000000000000002E+10),
	DtoR_TEST64(_L("1.0000000000000001E-10"),1.0E-10),
	DtoR_TEST64(_L("1.00000000000001E8"),1.00000000000001E8),
	DtoR_TEST64(_L("1.00000000000001E-8"),1.00000000000001E-8),
	//
	DtoR_TEST64(_L("1.23412341234e-7"),1.23412341234E-7),
	DtoR_TEST64(_L("0.000000123412341234"),1.23412341234E-7),
	DtoR_TEST64(_L("0.0000000123412341234"),1.23412341234E-8),
	//
	DtoR_TEST64(_L("0.29"),0.29),
	DtoR_TEST64(_L("0.2900"),0.2900),
	DtoR_TEST64(_L("0.2900"),0.29),
	//
    DtoR_TEST64(_L("0.0"),0.0),
    DtoR_TEST64(_L("1.0"),1.0),
    DtoR_TEST64(_L("10.0"),10.0),
    DtoR_TEST64(_L("100.0"),100.0),
    DtoR_TEST64(_L("1000.0"),1000.0),
    DtoR_TEST64(_L("0.1"),0.1),
    DtoR_TEST64(_L("0.01"),0.01),
    DtoR_TEST64(_L("0.001"),0.001),
    DtoR_TEST64(_L("0.0001"),0.0001),
	//
	DtoR_TEST64(_L("9.99999999999999999"),9.99999999999999999),
	DtoR_TEST64(_L("9.99999999999999"),9.99999999999999),
	DtoR_TEST64(_L("9.9999999999999"),9.9999999999999),
    DtoR_TEST64(_L("9.999999999"),9.999999999),
	//
	DtoR_TEST64(_L(".015"),0.015),
	DtoR_TEST64(_L("1.00E-15"),1.0E-15),
	DtoR_TEST64(_L("1.0E-07"),1.00E-7),
	//
    DtoR_TEST64(_L("1024.0"),1024.0),
    DtoR_TEST64(_L("1E10"),1E10),
    DtoR_TEST64(_L("1E20"),1E20),
    DtoR_TEST64(_L("1E30"),1E30),
    DtoR_TEST64(_L("1E40"),1E40),
    DtoR_TEST64(_L("1E50"),1E50),
//	DtoR_TEST64(_L("1E60"),1E60),		
    DtoR_TEST64(_L("1E70"),1E70),
    DtoR_TEST64(_L("1E80"),1E80),
    DtoR_TEST64(_L("1E90"),1E90),
    DtoR_TEST64(_L("1E98"),1E98),
//	DtoR_TEST64(_L("1E99"),1E99),
    DtoR_TEST64(_L("1E100"),1E100),
    DtoR_TEST64(_L("1E200"),1E200),
	DtoR_TEST64(_L("1E300"),1E300),		
    DtoR_TEST64(_L("1.7976931348623157E+308"),1.7976931348623157E+308),
	//
	DtoR_TEST64(_L("-1024.0"),-1024.0),
    DtoR_TEST64(_L("-1E10"),-1E10),
    DtoR_TEST64(_L("-1E20"),-1E20),
    DtoR_TEST64(_L("-1E30"),-1E30),
    DtoR_TEST64(_L("-1E40"),-1E40),
	DtoR_TEST64(_L("-1E50"),-1E50),
//  DtoR_TEST64(_L("-1E60"),-1E60),		
    DtoR_TEST64(_L("-1E70"),-1E70),
    DtoR_TEST64(_L("-1E80"),-1E80),
    DtoR_TEST64(_L("-1E90"),-1E90),
    DtoR_TEST64(_L("-1E98"),-1E98),
//	DtoR_TEST64(_L("-1E99"),-1E99),									
	DtoR_TEST64(_L("-1E100"),-1E100),
    DtoR_TEST64(_L("-1E200"),-1E200),
	DtoR_TEST64(_L("-1E300"),-1E300),	
    DtoR_TEST64(_L("-1.7976931348623157E+308"),-1.7976931348623157E+308),
	//
	DtoR_TEST64(_L("123456789012345.0"),123456789012345.0),
	DtoR_TEST64(_L("12345678901234567.0"),12345678901234568.0),		// 54 bits.  Check we are rounding up
	DtoR_TEST64(_L("1.23456789012345"),1.23456789012345),
	DtoR_TEST64(_L("1.2345678901234567"),1.2345678901234567),
	DtoR_TEST64(_L("1.2345678901234567890"),1.2345678901234567890), 
//	DtoR_TEST64(_L("1.23456789012345E+38"),1.23456789012345E+38),
	DtoR_TEST64(_L("1.2345678901234567E+38"),1.2345678901234567E+38),
	DtoR_TEST64(_L("1.23456789012345E+299"),1.23456789012345E+299),
//	DtoR_TEST64(_L("1.2345678901234567E+299"),1.2345678901234567E+299),	
//	DtoR_TEST64(_L("1.23456789012345E-75"),1.23456789012345E-75),
	DtoR_TEST64(_L("1.2345678901234567E-75"),1.2345678901234567E-75),
	DtoR_TEST64(_L("1.23456789012345E-146"),1.23456789012345E-146),
//	DtoR_TEST64(_L("1.2345678901234567E-146"),1.2345678901234567E-146),
	//
	DtoR_TEST64(_L("1E-10"),1E-10),
//	DtoR_TEST64(_L("1E-20"),1E-20),		
    DtoR_TEST64(_L("1E-30"),1E-30),
//	DtoR_TEST64(_L("1E-40"),1E-40),		
	DtoR_TEST64(_L("1E-50"),1E-50),		
    DtoR_TEST64(_L("1E-60"),1E-60),
	DtoR_TEST64(_L("1E-70"),1E-70),		
    DtoR_TEST64(_L("1E-80"),1E-80),
	DtoR_TEST64(_L("1E-90"),1E-90),
    DtoR_TEST64(_L("1E-98"),1E-98),
	DtoR_TEST64(_L("1E-99"),1E-99),
//	DtoR_TEST64(_L("1E-100"),1E-100),
//	DtoR_TEST64(_L("1E-200"),1E-200),
	DtoR_TEST64(_L("1E-300"),1E-300),  
	DtoR_TEST64(_L("2.2250738585072015E-308"),2.2250738585072015E-308),
	//
	DtoR_TEST64(_L("-1E-10"),-1E-10),
//	DtoR_TEST64(_L("-1E-20"),-1E-20),	
    DtoR_TEST64(_L("-1E-30"),-1E-30),
//	DtoR_TEST64(_L("-1E-40"),-1E-40),	
	DtoR_TEST64(_L("-1E-50"),-1E-50),	
    DtoR_TEST64(_L("-1E-60"),-1E-60),	
	DtoR_TEST64(_L("-1E-70"),-1E-70),	
    DtoR_TEST64(_L("-1E-80"),-1E-80),
	DtoR_TEST64(_L("-1E-90"),-1E-90),
    DtoR_TEST64(_L("-1E-98"),-1E-98),
	DtoR_TEST64(_L("-1E-99"),-1E-99), 
//	DtoR_TEST64(_L("-1E-100"),-1E-100),
//	DtoR_TEST64(_L("-1E-200"),-1E-200),
	DtoR_TEST64(_L("-1E-300"),-1E-300),
	DtoR_TEST64(_L("-2.2250738585072015E-308"),-2.2250738585072015E-308),
	//
	DtoR_TEST64(_L("+1.23"),1.23),
	DtoR_TEST64(_L("003.45"),3.45),
	DtoR_TEST64(_L("0.0000000000000015"),1.5E-15),
//	DtoR_TEST64(_L("1.234e-40"),1.234E-40)	
    };

GLDEF_D TUint size_testd2=sizeof(testd2);

GLDEF_D DtoR_TEST64 testApprox[]=
	{

	DtoR_TEST64(_L("1.23412341234e-11"),1.23412341234E-11),
	DtoR_TEST64(_L("0.0000000000123412341234"),1.23412341234E-11),	

	// Pow10() exact
	DtoR_TEST64(_L("1.2345678901234567E-146"),1.2345678901234567E-146),

	// Pow10() inexact
	DtoR_TEST64(_L("1E-40"),1E-40),
	DtoR_TEST64(_L("-1E-40"),-1E-40),
	DtoR_TEST64(_L("1E60"),1E60),
	DtoR_TEST64(_L("-1E60"),-1E60),
	DtoR_TEST64(_L("1E-20"),1E-20),
	DtoR_TEST64(_L("-1E-20"),-1E-20),
	DtoR_TEST64(_L("1E99"),1E99),
	DtoR_TEST64(_L("-1E99"),-1E99),
	DtoR_TEST64(_L("1.2345678901234567E+299"),1.2345678901234567E+299),
	DtoR_TEST64(_L("1.23456789012345E-75"),1.23456789012345E-75),
	DtoR_TEST64(_L("1E-100"),1E-100),
	DtoR_TEST64(_L("-1E-100"),-1E-100),
	DtoR_TEST64(_L("1E-200"),1E-200),
	DtoR_TEST64(_L("-1E-200"),-1E-200),
	DtoR_TEST64(_L("1.234e-40"),1.234E-40),
	DtoR_TEST64(_L("1.23456789012345E+38"),1.23456789012345E+38),
	};

GLDEF_D TUint size_testApprox=sizeof(testApprox);

GLDEF_D DtoR_TEST32 testd32[]=
    {
	DtoR_TEST32(_L("0.29"),0.29f),
	//
    DtoR_TEST32(_L("0.0"),0.0f),
    DtoR_TEST32(_L("1.0"),1.0f),
    DtoR_TEST32(_L("10.0"),10.0f),
    DtoR_TEST32(_L("100.0"),100.0f),
    DtoR_TEST32(_L("1000.0"),1000.0f),
    DtoR_TEST32(_L("0.1"),0.1f),
    DtoR_TEST32(_L("0.01"),0.01f),
    DtoR_TEST32(_L("0.001"),0.001f),
    DtoR_TEST32(_L("0.0001"),0.0001f),
	//
	DtoR_TEST32(_L("9.99999999999999"),10.0f),
	DtoR_TEST32(_L("9.9999999"),9.99999999f),
    DtoR_TEST32(_L("9.9999"),9.9999f),
	//
    DtoR_TEST32(_L(".015"),0.015f),
	DtoR_TEST32(_L("1.00E-15"),1.0E-15f),
	DtoR_TEST32(_L("1.0E-07"),1.0E-7f),
	//
    DtoR_TEST32(_L("1024.0"),1024.0f),
    DtoR_TEST32(_L("1E10"),1E10f),
    DtoR_TEST32(_L("1E20"),1E+20f),
    DtoR_TEST32(_L("1E30"),1E30f),
    DtoR_TEST32(_L("3.40282356E+38"),KMaxTReal32),
	//
	DtoR_TEST32(_L("-1024.0"),-1024.0f),
    DtoR_TEST32(_L("-1E10"),-1E10f),
    DtoR_TEST32(_L("-1E20"),-1E20f),
    DtoR_TEST32(_L("-1E30"),-1E+30f),
    DtoR_TEST32(_L("-3.40282356E+38"),-KMaxTReal32),
	//
	DtoR_TEST32(_L("1234567.0"),1234567.0f),
	DtoR_TEST32(_L("1234567890.0"),1234567890.0f),
	DtoR_TEST32(_L("1.234567"),1.234567f),
	DtoR_TEST32(_L("1.234567890"),1.234567890f),
	DtoR_TEST32(_L("1.23456789012"),1.23456789012f),
	DtoR_TEST32(_L("1.23456E+38"),1.23456E+38f),
	DtoR_TEST32(_L("1.23456789E+38"),1.23456789E+38f),
	DtoR_TEST32(_L("1.23456E-13"),1.23456E-13f),
	DtoR_TEST32(_L("1.23456789012E-13"),1.23456789012E-13f),
	//
	DtoR_TEST32(_L("1E-10"),1E-10f),
	DtoR_TEST32(_L("1E-20"),1E-20f),
    DtoR_TEST32(_L("1E-30"),1E-30f),
    DtoR_TEST32(_L("1.17549436e-38"),1.17549436e-38f),
	//
	DtoR_TEST32(_L("-1E-10"),-1E-10f),
	DtoR_TEST32(_L("-1E-20"),-1E-20f),
    DtoR_TEST32(_L("-1E-30"),-1E-30f),
    DtoR_TEST32(_L("-1.17549436e-38"),-1.17549436e-38f),
	//
	DtoR_TEST32(_L("+1.23"),1.23f),
	DtoR_TEST32(_L("003.45"),3.45f),
	DtoR_TEST32(_L("0.0000000000000015"),1.5E-15f),
	DtoR_TEST32(_L("1.234e-4"),1.234E-4f)	
    };

GLDEF_D TUint size_testd32=sizeof(testd32);

GLDEF_D DtoR_ERR_TEST testerr3[]=
	{
	DtoR_ERR_TEST(KNullDesC,KErrGeneral),
	DtoR_ERR_TEST(_L("E-20"),KErrGeneral), 
	DtoR_ERR_TEST(_L("1.#INF"),KErrNone),
	DtoR_ERR_TEST(_L("1.#NAN"),KErrNone),
	DtoR_ERR_TEST(_L("c.fgh"),KErrGeneral),
	DtoR_ERR_TEST(_L("xyz"),KErrGeneral),
	DtoR_ERR_TEST(_L("1.0E"),KErrNone),
	DtoR_ERR_TEST(_L("1.0E2147483648"),KErrOverflow),
	DtoR_ERR_TEST(_L("1.234567801234567890E+38"),KErrNone),
	DtoR_ERR_TEST(_L("3.456789012345678901E-38"),KErrNone),
	DtoR_ERR_TEST(_L("3.4028236E+38"),KErrOverflow),
	DtoR_ERR_TEST(_L("1.1754943E-38"),KErrNone),
	DtoR_ERR_TEST(_L("1.0E-38"),KErrNone),
	DtoR_ERR_TEST(_L("4.0E+38"),KErrOverflow),
	DtoR_ERR_TEST(_L("1.0E-39"),KErrNone),
	DtoR_ERR_TEST(_L("1.0E+39"),KErrOverflow),
	DtoR_ERR_TEST(_L("3.0E40"),KErrOverflow),
	DtoR_ERR_TEST(_L("6.9E-46"),KErrUnderflow),
	DtoR_ERR_TEST(_L("1.0E-325"),KErrUnderflow),
	DtoR_ERR_TEST(_L("2.0E-400"),KErrUnderflow),
	DtoR_ERR_TEST(_L("2.0E+400"),KErrOverflow),
	};

GLDEF_D TUint size_testerr3=sizeof(testerr3);

GLDEF_D CALC_TEST calctest[]=
	{
	CALC_TEST(_L("1.23412341234"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234")),
	CALC_TEST(_L("0.123412341234"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-01")),
	CALC_TEST(_L("0.0123412341234"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-02")),
	CALC_TEST(_L("0.00123412341234"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-03")),
	CALC_TEST(_L("0.000123412341234"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-04")),
	CALC_TEST(_L("0.0000123412341234"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-05")),
	CALC_TEST(_L("0.00000123412341234"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-06")),
	CALC_TEST(_L("0.000000123412341234"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-07")),
	CALC_TEST(_L("0.0000000123412341234"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-08")),
	CALC_TEST(_L("0.00000000123412341234"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-09")),
	CALC_TEST(_L("0.000000000123412341234"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-10")),
	CALC_TEST(_L("0.0000000000123412341234"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-11")),
	CALC_TEST(_L("0.00000000000123412341234"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-12")),
	CALC_TEST(_L("1.23412341234E-7"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-07")),
	CALC_TEST(_L("1.23412341234E-8"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-08")),
	CALC_TEST(_L("1.23412341234E-9"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-09")),
	CALC_TEST(_L("1.23412341234E-10"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-10")),
	CALC_TEST(_L("1.23412341234E-11"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-11")),
	CALC_TEST(_L("1.23412341234E-12"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-12")),
	CALC_TEST(_L("0.00000000000000"),KRealFormatCalculator,20,12,'.',',',0,_S("0")),
	CALC_TEST(_L("0.000000000000009999999999999"),KRealFormatCalculator,20,12,'.',',',0,_S("1E-14")),
	CALC_TEST(_L("0.000000000000000000009999999999999"),KRealFormatCalculator,20,12,'.',',',0,_S("1E-20")),
	CALC_TEST(_L("999999999999.5"),KRealFormatCalculator,20,12,'.',',',0,_S("1E+12")),
	CALC_TEST(_L("999999999999.4"),KRealFormatCalculator,20,12,'.',',',0,_S("999999999999")),
	CALC_TEST(_L("1.234123412341"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234")),
	CALC_TEST(_L("0.123412341234"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-01")),
	CALC_TEST(_L("0.012341234123"),KRealFormatCalculator,20,12,'.',',',0,_S("1.2341234123E-02")),
	CALC_TEST(_L("0.001234123412"),KRealFormatCalculator,20,12,'.',',',0,_S("1.234123412E-03")),
	CALC_TEST(_L("0.000123412341"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341E-04")),
	CALC_TEST(_L("0.000012341234"),KRealFormatCalculator,20,12,'.',',',0,_S("1.2341234E-05")),
	CALC_TEST(_L("0.000001234123"),KRealFormatCalculator,20,12,'.',',',0,_S("1.234123E-06")),
	CALC_TEST(_L("0.000000123412"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23412E-07")),
	CALC_TEST(_L("0.000000012341"),KRealFormatCalculator,20,12,'.',',',0,_S("1.2341E-08")),
	CALC_TEST(_L("0.000000001234"),KRealFormatCalculator,20,12,'.',',',0,_S("1.234E-09")),
	CALC_TEST(_L("0.000000000123"),KRealFormatCalculator,20,12,'.',',',0,_S("1.23E-10")),
	CALC_TEST(_L("0.000000000012"),KRealFormatCalculator,20,12,'.',',',0,_S("1.2E-11")),
	CALC_TEST(_L("0.000000000001"),KRealFormatCalculator,20,12,'.',',',0,_S("1E-12")),
	CALC_TEST(_L("0.12341234123"),KRealFormatCalculator,20,12,'.',',',0,_S("0.12341234123")),
	CALC_TEST(_L("0.01234123412"),KRealFormatCalculator,20,12,'.',',',0,_S("0.01234123412")),
	CALC_TEST(_L("0.00123412341"),KRealFormatCalculator,20,12,'.',',',0,_S("0.00123412341")),
	CALC_TEST(_L("0.00012341234"),KRealFormatCalculator,20,12,'.',',',0,_S("0.00012341234")),
	CALC_TEST(_L("0.00001234123"),KRealFormatCalculator,20,12,'.',',',0,_S("0.00001234123")),
	CALC_TEST(_L("0.00000123412"),KRealFormatCalculator,20,12,'.',',',0,_S("0.00000123412")),
	CALC_TEST(_L("0.00000012341"),KRealFormatCalculator,20,12,'.',',',0,_S("0.00000012341")),
	CALC_TEST(_L("0.00000001234"),KRealFormatCalculator,20,12,'.',',',0,_S("0.00000001234")),
	CALC_TEST(_L("0.00000000123"),KRealFormatCalculator,20,12,'.',',',0,_S("0.00000000123")),
	CALC_TEST(_L("0.00000000012"),KRealFormatCalculator,20,12,'.',',',0,_S("0.00000000012")),
	CALC_TEST(_L("0.00000000001"),KRealFormatCalculator,20,12,'.',',',0,_S("0.00000000001")),
	CALC_TEST(_L("0.000000000001"),KRealFormatCalculator,20,12,'.',',',0,_S("1E-12")),	
	CALC_TEST(_L("-1.234E19"),KRealFormatCalculator,20,12,'.',',',0,_S("-1.234E+19")),	
	};

GLDEF_D TUint size_calctest=sizeof(calctest);




