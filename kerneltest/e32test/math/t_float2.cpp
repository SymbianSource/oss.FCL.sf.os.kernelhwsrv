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
// e32test\math\t_float2.cpp
// File for T_FLOAT.CPP containing data for test2, test3 and test4
// T_FLOAT.CPP split into header files because there seems to be too much static
// data for GCC compiler
// 
//

#include "t_float.h"

GLDEF_D ROUND_TEST testround[]=
{
{30,0,30},
{1.47934,2,1.48},
{-72.86345,3,-72.863},
{-734.9999,0,-735.0},
{4855.9974,1,4856.0},
{232478.35,-1,232480.0},
{1.0,2,1.00},
{.0006,3,0.001},
{0.00000012,-1,0.0},
{4923.45,-1,4920.0},
{0,4,0},
{0,0,0},
{7493650136.435,-5,7493700000.0},
{-34.566732,7,-34.566732},
{-36842.6570524,-2,-36800.0},
{9,2,9.0},
{.0000003,7,.0000003,},
{.0000003,6,0.0},
{476253405.342,-12,0.0},
{.00000000000657,-3,0.0},
{0.1,0,0},
{9.9,0,10.0},
{.0000008,6,.000001},
{18.0,0,18.0},  
{9.1,0,9.0},
{9.5,0,10.0},
{9.9,0,10.0},
{0.9,0,1.0},
{0.9,1, 0.9},
{0.99,1,1.0}
};

GLDEF_D TUint size_testround=sizeof(testround);

//Added by AnnW
GLDEF_D RtoB_TEST testcalc[]=
	{
	RtoB_TEST(-1.234E+19,KRealFormatCalculator,20,12,'.',',',0,_S("-1.234E+19")),
	// NO EXPONENT FORMAT - no triads because triad length==0 
    RtoB_TEST(0,KRealFormatNoExponent,7,5,'.','0',0,_S("0")),			
    RtoB_TEST(0,KRealFormatNoExponent,1,1,'.','0',0,_S("0")),
	RtoB_TEST(0.00,KRealFormatNoExponent,7,4,'.','0',0,_S("0")),					
    RtoB_TEST(3.000,KRealFormatNoExponent,7,5,'.','0',0,_S("3")),
    RtoB_TEST(12.9999,KRealFormatNoExponent,6,4,'.','0',0,_S("13")),
    RtoB_TEST(12.9999,KRealFormatNoExponent,4,4,'.','0',0,_S("13")),
    RtoB_TEST(123456.0,KRealFormatNoExponent,6,6,'.','0',0,_S("123456")),
	RtoB_TEST(123456.0,KRealFormatNoExponent,9,6,'.','0',0,_S("123456")),
    RtoB_TEST(123456.0,KRealFormatNoExponent,10,8,'.',',',0,_S("123456")),
	RtoB_TEST(123456.0000,KRealFormatNoExponent,16,8,'.',',',0,_S("123456")),
	RtoB_TEST(123456.789,KRealFormatNoExponent,10,8,'.','0',0,_S("123456.79")),
	RtoB_TEST(123456.789,KRealFormatNoExponent,11,9,'.',',',0,_S("123456.789")),
	RtoB_TEST(123456.78900,KRealFormatNoExponent,13,11,'.',',',0,_S("123456.789")),
	RtoB_TEST(1234567890123.00,KRealFormatNoExponent,18,16,'.',',',0,_S("1234567890123")),
	RtoB_TEST(1234567890123.00,KRealFormatNoExponent,13,13,'.',',',0,_S("1234567890123")),
    RtoB_TEST(1234567890123.,KRealFormatNoExponent,20,16,'.','0',0,_S("1234567890123")),
	RtoB_TEST(.0453,KRealFormatNoExponent,12,10,'.','0',0,_S("0.0453")),
    RtoB_TEST(.0453,KRealFormatNoExponent,12,1,'.','0',0,_S("0.05")),
    RtoB_TEST(.00000876,KRealFormatNoExponent,12,10,'.','0',0,_S("0.00000876")),
    RtoB_TEST(.00000876,KRealFormatNoExponent,12,8,'.','0',0,_S("0.00000876")),
    RtoB_TEST(.00000876,KRealFormatNoExponent,9,7,'.','0',0,_S("0.0000088")),
    RtoB_TEST(.00000876,KRealFormatNoExponent,8,6,'.','0',0,_S("0.000009")),
    RtoB_TEST(.123,KRealFormatNoExponent,7,5,'.','0',0,_S("0.123")),
	RtoB_TEST(.1230000,KRealFormatNoExponent,7,5,'.','0',0,_S("0.123")),
	RtoB_TEST(.1230,KRealFormatNoExponent,7,5,'.','0',0,_S("0.123")),
	RtoB_TEST(.1230,KRealFormatNoExponent,5,4,'.','0',0,_S("0.123")),
    RtoB_TEST(.1235,KRealFormatNoExponent,6,4,'.','0',0,_S("0.1235")),
	RtoB_TEST(10000.00,KRealFormatNoExponent,10,7,'.',',',0,_S("10000")),
    RtoB_TEST(1.0E4,KRealFormatNoExponent,10,7,'.',',',0,_S("10000")),
	RtoB_TEST(1234.9876,KRealFormatNoExponent,8,8,'.',',',0,_S("1234.988")),
	RtoB_TEST(-1234.9876,KRealFormatNoExponent,9,8,'.',',',0,_S("-1234.988")),
	RtoB_TEST(1234.9876E-8,KRealFormatNoExponent,8,8,'.',',',0,_S("0.000012")),
	RtoB_TEST(-1234.9876E-8,KRealFormatNoExponent,8,7,'.',',',0,_S("-0.00001")),    
	//					  - no triads because flag set
	RtoB_TEST(0,noExponentNoTriads,7,5,'.','0',3,_S("0")),								
    RtoB_TEST(3.000,noExponentNoTriads,7,5,'.','0',3,_S("3")),
	RtoB_TEST(3.000,noExponentNoTriads,1,1,'.','0',3,_S("3")),
    RtoB_TEST(12.9999,noExponentNoTriads,6,4,'.','0',3,_S("13")),
    RtoB_TEST(123456.0,noExponentNoTriads,9,6,'.','0',0,_S("123456")),  // flag set and length==0
    RtoB_TEST(123456.0,noExponentNoTriads,10,8,'.',',',3,_S("123456")),
	RtoB_TEST(123456.0000,noExponentNoTriads,16,8,'.',',',3,_S("123456")),
	RtoB_TEST(123456.789,noExponentNoTriads,10,8,'.','0',0,_S("123456.79")),  // flag set and length==0
	RtoB_TEST(123456.789,noExponentNoTriads,9,8,'.','0',0,_S("123456.79")),  // flag set and length==0
	RtoB_TEST(123456.789,noExponentNoTriads,11,9,'.',',',3,_S("123456.789")),
	RtoB_TEST(123456.78900,noExponentNoTriads,13,11,'.',',',0,_S("123456.789")),  // flag set and length==0
	RtoB_TEST(1234567890123.00,noExponentNoTriads,18,16,'.',',',3,_S("1234567890123")),
    RtoB_TEST(1234567890123.,noExponentNoTriads,20,16,'.','0',0,_S("1234567890123")),  // flag set and length==0
	RtoB_TEST(.0453,noExponentNoTriads,12,10,'.','0',3,_S("0.0453")),
    RtoB_TEST(.0453,noExponentNoTriads,6,5,'.','0',3,_S("0.0453")),
	RtoB_TEST(.0453,noExponentNoTriads,5,3,'.','0',3,_S("0.045")),
    RtoB_TEST(.00000876,noExponentNoTriads,12,10,'.','0',3,_S("0.00000876")),
	// 					  - UK triad separators
	RtoB_TEST(0.0,KRealFormatNoExponent,9,6,'.',',',3,_S("0")),
    RtoB_TEST(.00568,KRealFormatNoExponent,9,3,'.',',',3,_S("0.00568")),
    RtoB_TEST(12345.0,KRealFormatNoExponent,9,6,'.',',',3,_S("12,345")),
    RtoB_TEST(12345.669,KRealFormatNoExponent,10,7,'.',',',3,_S("12,345.67")),
    RtoB_TEST(100000.0,KRealFormatNoExponent,10,8,'.',',',3,_S("100,000")),
    RtoB_TEST(99999.999,KRealFormatNoExponent,10,7,'.',',',3,_S("100,000")),
    RtoB_TEST(1234567890.675,KRealFormatNoExponent,17,12,'.',',',3,_S("1,234,567,890.68")),
    RtoB_TEST(1234567890.675,KRealFormatNoExponent,16,12,'.',',',3,_S("1,234,567,890.68")),
	RtoB_TEST(1.0E3,KRealFormatNoExponent,10,7,'.',',',3,_S("1,000")),
	RtoB_TEST(12345.669,KRealFormatNoExponent,8,7,'.',',',3,_S("12,345.7")),
	// 					  - French Triad separators
	RtoB_TEST(0.0,KRealFormatNoExponent,10,7,',',' ',4,_S("0")),
    RtoB_TEST(.00568,KRealFormatNoExponent,10,3,',',' ',4,_S("0,00568")),
    RtoB_TEST(1.0E3,KRealFormatNoExponent,10,7,',',' ',4,_S("1000")),
    RtoB_TEST(1234.0,KRealFormatNoExponent,10,7,',',' ',4,_S("1234")),
    RtoB_TEST(12345.0,KRealFormatNoExponent,9,6,',',' ',4,_S("12 345")),
    RtoB_TEST(12345.669,KRealFormatNoExponent,9,7,',',' ',4,_S("12 345,67")),
    RtoB_TEST(100000.0,KRealFormatNoExponent,10,8,',',' ',4,_S("100 000")),
	RtoB_TEST(100000.0,KRealFormatNoExponent,7,6,',',' ',4,_S("100 000")),
    RtoB_TEST(99999.999,KRealFormatNoExponent,10,7,',',' ',4,_S("100 000")),
    RtoB_TEST(1234567890.675,KRealFormatNoExponent,16,12,',',' ',4,_S("1 234 567 890,68")),
	// '.' as triad separator, and ',' as decimal point
    RtoB_TEST(1234.0,KRealFormatNoExponent,10,8,',','.',3,_S("1.234")),
	RtoB_TEST(0.0,KRealFormatNoExponent,10,8,',','.',3,_S("0")),
    RtoB_TEST(.00568,KRealFormatNoExponent,10,3,',','.',3,_S("0,00568")),
    RtoB_TEST(1.0E3,KRealFormatNoExponent,10,6,',','.',3,_S("1.000")),
    RtoB_TEST(12345.669,KRealFormatNoExponent,10,7,',','.',3,_S("12.345,67")),
	RtoB_TEST(12345.669,KRealFormatNoExponent,9,7,',','.',3,_S("12.345,67")),
    RtoB_TEST(12345.67,KRealFormatNoExponent,9,7,',','.',4,_S("12.345,67")),
    RtoB_TEST(1234567890.675,KRealFormatNoExponent,16,12,',','.',3,_S("1.234.567.890,68")),
	// Limited
	RtoB_TEST(1.23456789012501,noExponentLimit,32,15,'.',',',3,_S("1.23456789013")),
    RtoB_TEST(1.2345678901249,noExponentLimit,32,15,'.',',',3,_S("1.23456789012")),
    RtoB_TEST(1.99999999996,noExponentLimit,32,15,'.',',',3,_S("1.99999999996")),
    RtoB_TEST(1.999999999996,noExponentLimit,32,15,'.',',',3,_S("2")),
    RtoB_TEST(1.9999999999996,noExponentLimit,32,15,'.',',',3,_S("2")),
    RtoB_TEST(1.99999999999996,noExponentLimit,32,15,'.',',',3,_S("2")),
	// Calc setting - no triads and space for sign, width=14, dp=12
	RtoB_TEST(1.234567890123,noExponentCalc,14,12,'.',',',3,_S("1.23456789012")),
    RtoB_TEST(1.234567890129,noExponentCalc,14,12,'.',',',3,_S("1.23456789013")),
	RtoB_TEST(-1.234567890123,noExponentCalc,14,12,'.',',',3,_S("-1.23456789012")),
    RtoB_TEST(-1.234567890129,noExponentCalc,14,12,'.',',',3,_S("-1.23456789013")),
	RtoB_TEST(123456789012.00,noExponentCalc,14,12,'.',',',0,_S("123456789012")),    
	RtoB_TEST(999999999999.00,noExponentCalc,14,12,'.',',',0,_S("999999999999")),    
	//
	// CALCULATOR format
	// No Exponent format 
    RtoB_TEST(1.234E-8,KRealFormatCalculator,20,12,'.',',',0,_S("0.00000001234")),
	RtoB_TEST(1.234E-7,KRealFormatCalculator,20,12,'.',',',0,_S("0.0000001234")),
	RtoB_TEST(1.234E-6,KRealFormatCalculator,20,12,'.',',',0,_S("0.000001234")),
	RtoB_TEST(1.234E-3,KRealFormatCalculator,20,12,'.',',',0,_S("0.001234")),
	RtoB_TEST(1.234E-2,KRealFormatCalculator,20,12,'.',',',0,_S("0.01234")),
	RtoB_TEST(1E-11,KRealFormatCalculator,20,12,'.',',',0,_S("0.00000000001")),
	RtoB_TEST(1.2E-10,KRealFormatCalculator,20,12,'.',',',0,_S("0.00000000012")),
	RtoB_TEST(1.23E-9,KRealFormatCalculator,20,12,'.',',',0,_S("0.00000000123")),
	RtoB_TEST(1.234E-8,KRealFormatCalculator,20,12,'.',',',0,_S("0.00000001234")),
	RtoB_TEST(1.2345E-7,KRealFormatCalculator,20,12,'.',',',0,_S("0.00000012345")),
	RtoB_TEST(1.23456E-6,KRealFormatCalculator,20,12,'.',',',0,_S("0.00000123456")),
	RtoB_TEST(1.234567E-5,KRealFormatCalculator,20,12,'.',',',0,_S("0.00001234567")),
	RtoB_TEST(1.2345678E-4,KRealFormatCalculator,20,12,'.',',',0,_S("0.00012345678")),
	RtoB_TEST(1.23456789E-3,KRealFormatCalculator,20,12,'.',',',0,_S("0.00123456789")),
	RtoB_TEST(1.234567891E-2,KRealFormatCalculator,20,12,'.',',',0,_S("0.01234567891")),
	RtoB_TEST(1.2345678912E-1,KRealFormatCalculator,20,12,'.',',',0,_S("0.12345678912")),
	RtoB_TEST(1.23456789123E0,KRealFormatCalculator,20,12,'.',',',0,_S("1.23456789123")),
	//
	RtoB_TEST(0.000,KRealFormatCalculator,10,4,'.','0',0,_S("0")),
    RtoB_TEST(98.000,KRealFormatCalculator,10,4,'.','0',0,_S("98")),
    RtoB_TEST(12345.6700009,KRealFormatCalculator,17,11,'.','0',3,_S("12345.670001")),
    RtoB_TEST(1.E2,KRealFormatCalculator,10,4,'.','0',0,_S("100")),
    RtoB_TEST(1234.0,KRealFormatCalculator,10,4,'.','0',3,_S("1234")),
    RtoB_TEST(1.2345,KRealFormatCalculator,10,3,'.','0',0,_S("1.23")),
    RtoB_TEST(1.235,KRealFormatCalculator,9,3,'.','0',0,_S("1.24")),
    RtoB_TEST(98765.0,KRealFormatCalculator,13,6,'.','0',0,_S("98765")),
    RtoB_TEST(100000000.0,KRealFormatCalculator,15,9,'.','0',0,_S("100000000")),
    RtoB_TEST(123.098785E00,KRealFormatCalculator,15,9,'.','0',0,_S("123.098785")),
	RtoB_TEST(-123.098785E00,KRealFormatCalculator,15,8,'.','0',0,_S("-123.09879")),
    RtoB_TEST(1234.9876E-4,KRealFormatCalculator,15,9,'.','0',0,_S("0.12349876")),
    RtoB_TEST(1234.9876E-5,KRealFormatCalculator,16,10,'.','0',0,_S("0.012349876")),
 	RtoB_TEST(0.0099,KRealFormatCalculator,11,5,'.','0',0,_S("0.0099")),
	RtoB_TEST(0.099,KRealFormatCalculator,10,4,'.','0',0,_S("0.099")),
	RtoB_TEST(0.001209,KRealFormatCalculator,15,7,'.','0',0,_S("0.001209")),
	RtoB_TEST(0.00100987,KRealFormatCalculator,15,9,'.','0',0,_S("0.00100987")),
	RtoB_TEST(0.678,KRealFormatCalculator,10,4,'.','0',0,_S("0.678")),
    RtoB_TEST(99999.0,KRealFormatCalculator,12,6,'.','0',0,_S("99999")),
	RtoB_TEST(12345678901234567890.0,KRealFormatCalculator,20,12,'.','0',0,_S("1.23456789012E+19")),
    RtoB_TEST(1234567890123.0,KRealFormatCalculator,13,7,'.','0',0,_S("1.234568E+12")),
	// Initially the format type is exponent, but after rounding it is changed to fixed
    RtoB_TEST(0.00099999,KRealFormatCalculator,10,4,'.','0',0,_S("0.001")),
    // Exponent format, two digit exponent only 
	RtoB_TEST(1.23412341234E-11,KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-11")),
	RtoB_TEST(1.23412341234E-8,KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-08")),
	RtoB_TEST(1.23412341234E-7,KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-07")),
	RtoB_TEST(1.23412341234E-6,KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-06")),
	RtoB_TEST(1.23412341234E-3,KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-03")),
	RtoB_TEST(1.23412341234E-2,KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-02")),
	RtoB_TEST(1.23412341234123E-7,KRealFormatCalculator,20,12,'.',',',0,_S("1.23412341234E-07")),
	RtoB_TEST(1.234E-11,KRealFormatCalculator,20,12,'.',',',0,_S("1.234E-11")),
	RtoB_TEST(1.234E-9,KRealFormatCalculator,20,12,'.',',',0,_S("1.234E-09")),
	//
	RtoB_TEST(10000000.0,KRealFormatCalculator,13,7,'.','0',0,_S("1E+07")),
	RtoB_TEST(0.001209,KRealFormatCalculator,11,5,'.','0',0,_S("1.209E-03")),
	RtoB_TEST(0.00100987,KRealFormatCalculator,12,6,'.','0',0,_S("1.00987E-03")),
	RtoB_TEST(0.00099999109,KRealFormatCalculator,13,7,'.','0',0,_S("9.999911E-04")),
	RtoB_TEST(0.0000678,KRealFormatCalculator,9,3,'.','0',0,_S("6.78E-05")),
    RtoB_TEST(1234567.8765E12,KRealFormatCalculator,11,5,'.','0',0,_S("1.2346E+18")),
    RtoB_TEST(123.098785E56,KRealFormatCalculator,13,7,'.','0',0,_S("1.230988E+58")),
    RtoB_TEST(0.0000001234,KRealFormatCalculator,10,4,'.','0',0,_S("1.234E-07")),
    RtoB_TEST(.99999999E99,KRealFormatCalculator,12,6,'.','0',0,_S("1E+99")),
    RtoB_TEST(1.0E99,KRealFormatCalculator,12,6,'.','0',0,_S("1E+99")),
    RtoB_TEST(.00000000001,KRealFormatCalculator,12,6,'.','0',0,_S("1E-11")),
    RtoB_TEST(5384795.26E-52,KRealFormatCalculator,13,7,'.','0',0,_S("5.384795E-46")),
    RtoB_TEST(123.098785E-87,KRealFormatCalculator,13,7,'.','0',0,_S("1.230988E-85")),
    RtoB_TEST(.99999999E-99,KRealFormatCalculator,12,6,'.','0',0,_S("1E-99")),
    RtoB_TEST(1.0E-99,KRealFormatCalculator,12,6,'.','0',0,_S("1E-99")),
	//
	RtoB_TEST(1E-12,KRealFormatCalculator,20,12,'.',',',0,_S("1E-12")),
	RtoB_TEST(1.2E-11,KRealFormatCalculator,20,12,'.',',',0,_S("1.2E-11")),
	RtoB_TEST(1.23E-10,KRealFormatCalculator,20,12,'.',',',0,_S("1.23E-10")),
	RtoB_TEST(1.234E-9,KRealFormatCalculator,20,12,'.',',',0,_S("1.234E-09")),
	RtoB_TEST(1.2345E-8,KRealFormatCalculator,20,12,'.',',',0,_S("1.2345E-08")),
	RtoB_TEST(1.23456E-7,KRealFormatCalculator,20,12,'.',',',0,_S("1.23456E-07")),
	RtoB_TEST(1.234567E-6,KRealFormatCalculator,20,12,'.',',',0,_S("1.234567E-06")),
	RtoB_TEST(1.2345678E-5,KRealFormatCalculator,20,12,'.',',',0,_S("1.2345678E-05")),
	RtoB_TEST(1.23456789E-4,KRealFormatCalculator,20,12,'.',',',0,_S("1.23456789E-04")),
	RtoB_TEST(1.234567891E-3,KRealFormatCalculator,20,12,'.',',',0,_S("1.234567891E-03")),
	RtoB_TEST(1.2345678912E-2,KRealFormatCalculator,20,12,'.',',',0,_S("1.2345678912E-02")),
	RtoB_TEST(1.23456789123E-1,KRealFormatCalculator,20,12,'.',',',0,_S("1.23456789123E-01")),
	RtoB_TEST(1.234567891234,KRealFormatCalculator,20,12,'.',',',0,_S("1.23456789123")),
	// Exponent format, three-digit exponents allowed
    RtoB_TEST(9999999.0,calculatorThreeDigitExp,11,4,'.','0',0,_S("1E+07")),
    RtoB_TEST(1234567890123.0,calculatorThreeDigitExp,20,13,'.','0',0,_S("1234567890123")),	// Leave room for three-digit exponent, so can have fixed numbers one digit longer 
    RtoB_TEST(1234567.8765E12,calculatorThreeDigitExp,13,6,'.','0',0,_S("1.23457E+18")),
    RtoB_TEST(123.098785E56,calculatorThreeDigitExp,14,7,'.','0',0,_S("1.230988E+58")),
    RtoB_TEST(.99999999E99,calculatorThreeDigitExp,13,6,'.','0',0,_S("1E+99")),
    RtoB_TEST(1.0E99,calculatorThreeDigitExp,13,6,'.','0',0,_S("1E+99")),
    RtoB_TEST(.000000000001,calculatorThreeDigitExp,13,6,'.','0',0,_S("1E-12")),
    RtoB_TEST(5384795.26E-52,calculatorThreeDigitExp,14,7,'.','0',0,_S("5.384795E-46")),
    RtoB_TEST(.99999999E-99,calculatorThreeDigitExp,13,6,'.','0',0,_S("1E-99")),
    RtoB_TEST(1.0E-99,calculatorThreeDigitExp,13,6,'.','0',0,_S("1E-99")),
	RtoB_TEST(1234.9876E-103,calculatorThreeDigitExp,13,6,'.','0',0,_S("1.23499E-100")),
    RtoB_TEST(1234.9876E-107,calculatorThreeDigitExp,15,8,'.','0',0,_S("1.2349876E-104")),
    RtoB_TEST(1234.9876E-109,calculatorThreeDigitExp,15,8,'.','0',0,_S("1.2349876E-106")),
    RtoB_TEST(1234.9876E-110,calculatorThreeDigitExp,16,9,'.','0',0,_S("1.2349876E-107")),
    RtoB_TEST(1234.9876E-200,calculatorThreeDigitExp,17,10,'.','0',0,_S("1.2349876E-197")),
    RtoB_TEST(1234.9876E-300,calculatorThreeDigitExp,17,10,'.','0',0,_S("1.2349876E-297")),
    RtoB_TEST(1234.9876E97,calculatorThreeDigitExp,13,6,'.','0',0,_S("1.23499E+100")),
    RtoB_TEST(1234.9876E100,calculatorThreeDigitExp,15,8,'.','0',0,_S("1.2349876E+103")),
    RtoB_TEST(1234.9876E103,calculatorThreeDigitExp,15,8,'.','0',0,_S("1.2349876E+106")),
    RtoB_TEST(1234.9876E200,calculatorThreeDigitExp,16,9,'.','0',0,_S("1.2349876E+203")),
    RtoB_TEST(1234.9876E300,calculatorThreeDigitExp,17,10,'.','0',0,_S("1.2349876E+303")),
	//
	RtoB_TEST(1.2E+100,calculatorThreeDigitExp,8,1,'.','0',0,_S("1E+100")),
    RtoB_TEST(1.2E-100,calculatorThreeDigitExp,8,1,'.','0',0,_S("1E-100")),
    RtoB_TEST(-1.2E+100,calculatorThreeDigitExp,8,1,'.','0',0,_S("-1E+100")),
    RtoB_TEST(-1.2E-100,calculatorThreeDigitExp,8,1,'.','0',0,_S("-1E-100")),
    RtoB_TEST(1.2E+100,calculatorThreeDigitExp,9,2,'.','0',0,_S("1.2E+100")),
    RtoB_TEST(1.2E-100,calculatorThreeDigitExp,9,2,'.','0',0,_S("1.2E-100")),
    RtoB_TEST(-1.2E+100,calculatorThreeDigitExp,9,2,'.','0',0,_S("-1.2E+100")),
    RtoB_TEST(-1.2E-100,calculatorThreeDigitExp,9,2,'.','0',0,_S("-1.2E-100")),
    RtoB_TEST(1E+100,calculatorThreeDigitExp,9,2,'.','0',0,_S("1E+100")),
    RtoB_TEST(1E-100,calculatorThreeDigitExp,9,2,'.','0',0,_S("1E-100")),
    RtoB_TEST(-1E+100,calculatorThreeDigitExp,9,2,'.','0',0,_S("-1E+100")),
    RtoB_TEST(-1E-100,calculatorThreeDigitExp,9,2,'.','0',0,_S("-1E-100")),
    RtoB_TEST(1E+100,calculatorThreeDigitExp,8,1,'.','0',0,_S("1E+100")),
    RtoB_TEST(1E-100,calculatorThreeDigitExp,8,1,'.','0',0,_S("1E-100")),
    RtoB_TEST(-1E+100,calculatorThreeDigitExp,8,1,'.','0',0,_S("-1E+100")),
    RtoB_TEST(-1E-100,calculatorThreeDigitExp,8,1,'.','0',0,_S("-1E-100")),
	//
	RtoB_TEST(1.4E+308,calculatorThreeDigitExp,8,1,'.','0',0,_S("1E+308")),
    RtoB_TEST(2.3E-308,calculatorThreeDigitExp,8,1,'.','0',0,_S("2E-308")),
    RtoB_TEST(-1.4E+308,calculatorThreeDigitExp,8,1,'.','0',0,_S("-1E+308")),
    RtoB_TEST(-2.3E-308,calculatorThreeDigitExp,8,1,'.','0',0,_S("-2E-308")),
    RtoB_TEST(1.7E+308,calculatorThreeDigitExp,9,2,'.','0',0,_S("1.7E+308")),
    RtoB_TEST(2.3E-308,calculatorThreeDigitExp,10,3,'.','0',0,_S("2.3E-308")),
    RtoB_TEST(-1.7E+308,calculatorThreeDigitExp,10,3,'.','0',0,_S("-1.7E+308")),
    RtoB_TEST(-2.3E-308,calculatorThreeDigitExp,9,2,'.','0',0,_S("-2.3E-308")),
    RtoB_TEST(1.797693E+308,calculatorThreeDigitExp,15,7,'.','0',0,_S("1.797693E+308")),
    RtoB_TEST(2.225074E-308,calculatorThreeDigitExp,16,8,'.','0',0,_S("2.225074E-308")),
    RtoB_TEST(-1.797693E+308,calculatorThreeDigitExp,16,8,'.','0',0,_S("-1.797693E+308")),
    RtoB_TEST(-2.225074E-308,calculatorThreeDigitExp,16,7,'.','0',0,_S("-2.225074E-308")),
	// Limited
	RtoB_TEST(1.23456789012501E24,calculatorLimit,21,15,'.',',',3,_S("1.23456789013E+24")),
    RtoB_TEST(1.2345678901249E+97,calculatorLimit,32,15,'.',',',3,_S("1.23456789012E+97")),
    RtoB_TEST(1.99999999996E-19,calculatorLimit,21,15,'.',',',3,_S("1.99999999996E-19")),
    RtoB_TEST(1.999999999996E-53,calculatorLimit,32,15,'.',',',3,_S("2E-53")),
    RtoB_TEST(1.9999999999996E+46,calculatorLimit,32,15,'.',',',3,_S("2E+46")),
	//
	RtoB_TEST(1.23456789012501,calculatorLimit,32,15,'.',',',3,_S("1.23456789013")),
    RtoB_TEST(1.2345678901249,calculatorLimit,32,15,'.',',',3,_S("1.23456789012")),
    RtoB_TEST(1.99999999996,calculatorLimit,32,15,'.',',',3,_S("1.99999999996")),
    RtoB_TEST(1.999999999996,calculatorLimit,32,15,'.',',',3,_S("2")),
    RtoB_TEST(1.9999999999996,calculatorLimit,32,15,'.',',',3,_S("2")),
    RtoB_TEST(1.99999999999996,calculatorLimit,32,15,'.',',',3,_S("2")),
	//
	RtoB_TEST(1.23456789012501E24,calculatorLimitAndThreeDigExp,22,15,'.',',',3,_S("1.23456789013E+24")),
    RtoB_TEST(1.2345678901249E+197,calculatorLimitAndThreeDigExp,32,15,'.',',',3,_S("1.23456789012E+197")),
    RtoB_TEST(1.99999999996E-19,calculatorLimitAndThreeDigExp,22,15,'.',',',3,_S("1.99999999996E-19")),
    RtoB_TEST(1.999999999996E-153,calculatorLimitAndThreeDigExp,32,15,'.',',',3,_S("2E-153")),
    RtoB_TEST(1.9999999999996E+246,calculatorLimitAndThreeDigExp,32,15,'.',',',3,_S("2E+246")),
	RtoB_TEST(1.99999999999996E-302,calculatorLimitAndThreeDigExp,32,15,'.',',',3,_S("2E-302")),

	// new - exponent with significant figures
	RtoB_TEST(10000000.0,exponentSigFigs,13,7,'.','0',0,_S("1E+07")),
    RtoB_TEST(9999999.0,exponentSigFigs,12,6,'.','0',0,_S("1E+07")),
    RtoB_TEST(1234567890123.0,exponentSigFigs,13,7,'.','0',0,_S("1.234568E+12")),
    RtoB_TEST(1234567.8765E12,exponentSigFigs,11,5,'.','0',0,_S("1.2346E+18")),
    RtoB_TEST(123.098785E56,exponentSigFigs,13,7,'.','0',0,_S("1.230988E+58")),
    RtoB_TEST(0.0000678,exponentSigFigs,9,3,'.','0',0,_S("6.78E-05")),
    RtoB_TEST(0.0000001234,exponentSigFigs,10,4,'.','0',0,_S("1.234E-07")),
    RtoB_TEST(.99999999E99,exponentSigFigs,12,6,'.','0',0,_S("1E+99")),
    RtoB_TEST(1.0E99,exponentSigFigs,12,6,'.','0',0,_S("1E+99")),
    RtoB_TEST(.0000000001,exponentSigFigs,12,6,'.','0',0,_S("1E-10")),
    RtoB_TEST(5384795.26E-52,exponentSigFigs,13,7,'.','0',0,_S("5.384795E-46")),
    RtoB_TEST(123.098785E-87,exponentSigFigs,13,7,'.','0',0,_S("1.230988E-85")),
    RtoB_TEST(.99999999E-99,exponentSigFigs,12,6,'.','0',0,_S("1E-99")),
    RtoB_TEST(1.0E-99,exponentSigFigs,12,6,'.','0',0,_S("1E-99")),
	// three-digit exponents allowed
    RtoB_TEST(9999999.0,exponentThreeDigitExpAndSigFigs,11,4,'.','0',0,_S("1E+07")),
    RtoB_TEST(1234567.8765E12,exponentThreeDigitExpAndSigFigs,13,6,'.','0',0,_S("1.23457E+18")),
    RtoB_TEST(123.098785E56,exponentThreeDigitExpAndSigFigs,14,7,'.','0',0,_S("1.230988E+58")),
    RtoB_TEST(.99999999E99,exponentThreeDigitExpAndSigFigs,13,6,'.','0',0,_S("1E+99")),
    RtoB_TEST(1.0E99,exponentThreeDigitExpAndSigFigs,13,6,'.','0',0,_S("1E+99")),
    RtoB_TEST(.0000000001,exponentThreeDigitExpAndSigFigs,13,6,'.','0',0,_S("1E-10")),
    RtoB_TEST(5384795.26E-52,exponentThreeDigitExpAndSigFigs,14,7,'.','0',0,_S("5.384795E-46")),
    RtoB_TEST(.99999999E-99,exponentThreeDigitExpAndSigFigs,13,6,'.','0',0,_S("1E-99")),
    RtoB_TEST(1.0E-99,exponentThreeDigitExpAndSigFigs,13,6,'.','0',0,_S("1E-99")),
	RtoB_TEST(1234.9876E-103,exponentThreeDigitExpAndSigFigs,13,6,'.','0',0,_S("1.23499E-100")),
    RtoB_TEST(1234.9876E-107,exponentThreeDigitExpAndSigFigs,15,8,'.','0',0,_S("1.2349876E-104")),
    RtoB_TEST(1234.9876E-109,exponentThreeDigitExpAndSigFigs,15,8,'.','0',0,_S("1.2349876E-106")),
    RtoB_TEST(1234.9876E-110,exponentThreeDigitExpAndSigFigs,16,9,'.','0',0,_S("1.2349876E-107")),
    RtoB_TEST(1234.9876E-200,exponentThreeDigitExpAndSigFigs,17,10,'.','0',0,_S("1.2349876E-197")),
    RtoB_TEST(1234.9876E-300,exponentThreeDigitExpAndSigFigs,17,10,'.','0',0,_S("1.2349876E-297")),
    RtoB_TEST(1234.9876E97,exponentThreeDigitExpAndSigFigs,13,6,'.','0',0,_S("1.23499E+100")),
    RtoB_TEST(1234.9876E100,exponentThreeDigitExpAndSigFigs,15,8,'.','0',0,_S("1.2349876E+103")),
    RtoB_TEST(1234.9876E103,exponentThreeDigitExpAndSigFigs,15,8,'.','0',0,_S("1.2349876E+106")),
    RtoB_TEST(1234.9876E200,exponentThreeDigitExpAndSigFigs,16,9,'.','0',0,_S("1.2349876E+203")),
    RtoB_TEST(1234.9876E300,exponentThreeDigitExpAndSigFigs,17,10,'.','0',0,_S("1.2349876E+303"))
	};

GLDEF_D TUint size_testcalc=sizeof(testcalc);
	
GLDEF_D ERR_TEST testerr[]=
    {
    // FIXED - special values
	ERR_TEST(KPosInfTReal64,fixedNoTriads,32,4,'.','0',0,_S("Inf"),KErrOverflow),
	ERR_TEST(KNegInfTReal64,fixedNoTriads,32,4,'.','0',0,_S("-Inf"),KErrOverflow),
	ERR_TEST(KNaNTReal64,fixedNoTriads,32,4,'.','0',0,_S("NaN"),KErrArgument),	
	ERR_TEST(K1EMinus324Real64,fixedNoTriads,32,4,'.','0',0,_S("0.0000"),6),	
	//       - no space for sign and no triads - does not fit 
	ERR_TEST(1234.9876E95,fixedNoTriads,12,0,'.','0',0,_S("Inf"),KErrOverflow),
	ERR_TEST(-1234.9876E95,fixedNoTriads,12,0,'.','0',0,_S("-Inf"),KErrOverflow),
	ERR_TEST(1234.9876,fixedNoTriads,8,4,'.','0',0,_S("Inf"),KErrOverflow),
	ERR_TEST(1234.9876E-8,fixedNoTriads,3,7,'.','0',0),
	ERR_TEST(1234.9876E-8,fixedNoTriads,8,7,'.','0',0),
	//		 - no space for sign and triads - does not fit
	ERR_TEST(1234.9876E-8,KRealFormatFixed,10,10,'.','0',0),
	ERR_TEST(1234.9876E-107,KRealFormatFixed,12,11,'.','0',0),
    ERR_TEST(1234.9876E-109,KRealFormatFixed,12,11,'.','0',0),
    ERR_TEST(1234.9876E-110,KRealFormatFixed,12,11,'.','0',0),
    ERR_TEST(1234.9876E-200,KRealFormatFixed,12,11,'.','0',0),
    ERR_TEST(1234.9876E-300,KRealFormatFixed,12,11,'.','0',0),
	//
	ERR_TEST(1234.9876E4,KRealFormatFixed,8,0,'.',',',3,_S("Inf"),KErrOverflow),
	ERR_TEST(1234.9876E4,KRealFormatFixed,9,0,'.',',',3,_S("Inf"),KErrOverflow),
	ERR_TEST(1234.9876E95,KRealFormatFixed,12,6,'.','0',0,_S("Inf"),KErrOverflow),
	ERR_TEST(1234.9876E96,KRealFormatFixed,12,6,'.','0',0,_S("Inf"),KErrOverflow),		
    ERR_TEST(1234.9876E97,KRealFormatFixed,12,6,'.','0',0,_S("Inf"),KErrOverflow),	
    ERR_TEST(1234.9876E100,KRealFormatFixed,12,6,'.','0',0,_S("Inf"),KErrOverflow),	
    ERR_TEST(1234.9876E103,KRealFormatFixed,12,6,'.','0',0,_S("Inf"),KErrOverflow),	
    ERR_TEST(1234.9876E200,KRealFormatFixed,12,6,'.','0',0,_S("Inf"),KErrOverflow),	
    ERR_TEST(1234.9876E300,KRealFormatFixed,12,6,'.','0',0,_S("Inf"),KErrOverflow),
	//		 - space for sign and no triads - does not fit
	ERR_TEST(1234.9876,fixedSpaceForSign,9,4,'.','0',0,_S("Inf"),KErrOverflow),
	ERR_TEST(1234.9876E-8,fixedSpaceForSign,11,9,'.','0',0),
	//		 - space for sign and triads - does not fit
	ERR_TEST(1234.9876E4,fixedTriadsAndSign,9,0,'.',',',3,_S("Inf"),KErrOverflow),
	ERR_TEST(1234.9876E4,fixedTriadsAndSign,10,0,'.',',',3,_S("Inf"),KErrOverflow),
	//		 - no error, returns length
	ERR_TEST(1234.9876,fixedNoTriads,9,4,'.','0',0,_S("1234.9876"),9),
    ERR_TEST(1234.9876E4,KRealFormatFixed,11,0,'.',',',3,_S("12,349,876"),10),
	ERR_TEST(1234.9876,fixedSpaceForSign,10,4,'.','0',0,_S("1234.9876"),9),
	ERR_TEST(-1234.9876,fixedSpaceForSign,10,4,'.','0',0,_S("-1234.9876"),10),
    ERR_TEST(1234.9876E4,fixedTriadsAndSign,12,0,'.',',',3,_S("12,349,876"),10),
	ERR_TEST(1234.9876E-101,KRealFormatFixed,12,6,'.','0',0,_S("0.000000"),8),
    ERR_TEST(1234.9876E-102,KRealFormatFixed,12,6,'.','0',0,_S("0.000000"),8),
    ERR_TEST(1234.9876E-103,KRealFormatFixed,12,6,'.','0',0,_S("0.000000"),8),
	
	// EXPONENT - special values
	ERR_TEST(KPosInfTReal64,KRealFormatExponent,32,6,'.','0',0,_S("Inf"),KErrOverflow),
	ERR_TEST(KNegInfTReal64,KRealFormatExponent,32,6,'.','0',0,_S("-Inf"),KErrOverflow),  
	ERR_TEST(KNaNTReal64,KRealFormatExponent,32,6,'.','0',0,_S("NaN"),KErrArgument),
 	ERR_TEST(K5EMinus324Real64,exponentThreeDigitExp,32,6,'.',',',0,_S("4.940656E-324"),13),
 	ERR_TEST(K1EMinus324Real64,KRealFormatExponent,32,6,'.','0',0,_S("0.000000E+00"),12),
	//			- only two digits allowed - underflow
	ERR_TEST(1234.9876E-103,KRealFormatExponent,12,6,'.','0',0,_S("0"),KErrUnderflow),
    ERR_TEST(1234.9876E-110,KRealFormatExponent,12,6,'.','0',0,_S("0"),KErrUnderflow),
    ERR_TEST(1234.9876E-200,KRealFormatExponent,12,6,'.','0',0,_S("0"),KErrUnderflow),
    ERR_TEST(1234.9876E-300,KRealFormatExponent,12,6,'.','0',0,_S("0"),KErrUnderflow),
    //									  - overflow
	ERR_TEST(1234.9876E97,KRealFormatExponent,12,6,'.','0',0,_S("Inf"),KErrOverflow),
	ERR_TEST(-1234.9876E97,KRealFormatExponent,12,6,'.','0',0,_S("-Inf"),KErrOverflow),
    ERR_TEST(1234.9876E100,KRealFormatExponent,12,6,'.','0',0,_S("Inf"),KErrOverflow),
    ERR_TEST(1234.9876E200,KRealFormatExponent,12,6,'.','0',0,_S("Inf"),KErrOverflow),
    ERR_TEST(1234.9876E300,KRealFormatExponent,12,6,'.','0',0,_S("Inf"),KErrOverflow),
	//									  - does not fit
	ERR_TEST(1234.9876E24,KRealFormatExponent,12,7,'.','0',0),
    ERR_TEST(1234.9876E95,KRealFormatExponent,6,3,'.','0',0),
    ERR_TEST(1234.9876E-76,KRealFormatExponent,8,7,'.','0',0),
    ERR_TEST(1234.9876E-98,KRealFormatExponent,9,4,'.','0',0),
	// 			- three digits allowed - does not fit
	ERR_TEST(1234.9876E240,exponentThreeDigitExp,13,7,'.','0',0),
    ERR_TEST(1234.9876E98,exponentThreeDigitExp,6,1,'.','0',0),
    ERR_TEST(1234.9876E-160,exponentThreeDigitExp,9,7,'.','0',0),
    ERR_TEST(1234.9876E-98,exponentThreeDigitExp,9,4,'.','0',0),
	// 			- two digit exponent - no error, returns length
	ERR_TEST(1234.9876E95,KRealFormatExponent,12,6,'.','0',0,_S("1.234988E+98"),12),
    ERR_TEST(1234.9876E96,KRealFormatExponent,12,6,'.','0',0,_S("1.234988E+99"),12),
	ERR_TEST(1234.9876E-101,KRealFormatExponent,12,6,'.','0',0,_S("1.234988E-98"),12),
    ERR_TEST(1234.9876E-102,KRealFormatExponent,12,6,'.','0',0,_S("1.234988E-99"),12),
	//			- three-digit exponent - no error, returns length
    ERR_TEST(1234.9876E125,exponentThreeDigitExp,13,6,'.','0',0,_S("1.234988E+128"),13),
    ERR_TEST(1234.9876E126,exponentThreeDigitExp,13,6,'.','0',0,_S("1.234988E+129"),13),
	ERR_TEST(1234.9876E-139,exponentThreeDigitExp,13,6,'.','0',0,_S("1.234988E-136"),13),
    ERR_TEST(1234.9876E-142,exponentThreeDigitExp,13,6,'.','0',0,_S("1.234988E-139"),13),

	// GENERAL - only two-digit exponents allowed - underflow
	ERR_TEST(1234.9876E-103,KRealFormatGeneral,12,6,'.','0',0,_S("0"),KErrUnderflow),
    ERR_TEST(1234.9876E-107,KRealFormatGeneral,12,6,'.','0',0,_S("0"),KErrUnderflow),
    ERR_TEST(1234.9876E-200,KRealFormatGeneral,12,6,'.','0',0,_S("0"),KErrUnderflow),
    ERR_TEST(1234.9876E-300,KRealFormatGeneral,12,6,'.','0',0,_S("0"),KErrUnderflow),    
	// 											  - overflow
	ERR_TEST(1234.9876E97,KRealFormatGeneral,12,6,'.','0',0,_S("Inf"),KErrOverflow),
    ERR_TEST(1234.9876E100,KRealFormatGeneral,12,6,'.','0',0,_S("Inf"),KErrOverflow),
    ERR_TEST(1234.9876E200,KRealFormatGeneral,12,6,'.','0',0,_S("Inf"),KErrOverflow),
    ERR_TEST(1234.9876E300,KRealFormatGeneral,12,6,'.','0',0,_S("Inf"),KErrOverflow),
	// 		   - space for sign needed
	ERR_TEST(1,generalSpaceForSign,1,0,'.',',',0),
    //		   - no errors, returns length
	ERR_TEST(1234.9876E-101,KRealFormatGeneral,12,6,'.','0',0,_S("1.234988E-98"),12),
    ERR_TEST(1234.9876E-104,generalThreeDigitExp,13,6,'.','0',0,_S("1.234988E-101"),13),
    ERR_TEST(1234.9876E95,KRealFormatGeneral,12,6,'.','0',0,_S("1.234988E+98"),12),
    ERR_TEST(1234.9876E99,generalThreeDigitExp,13,6,'.','0',0,_S("1.234988E+102"),13),
	ERR_TEST(1234.9876,generalSpaceForSign,10,4,'.','0',0,_S("1234.9876"),9),
	ERR_TEST(-1234.9876,generalSpaceForSign,10,4,'.','0',0,_S("-1234.9876"),10),

	// NO EXPONENT - not enough space
	ERR_TEST(1234.9876E95,KRealFormatNoExponent,12,10,'.','0',0,_S("Inf"),KErrOverflow),
	ERR_TEST(1234.9876E103,KRealFormatNoExponent,12,10,'.','0',0,_S("Inf"),KErrOverflow),	
    ERR_TEST(1234.9876E200,KRealFormatNoExponent,12,10,'.','0',0,_S("Inf"),KErrOverflow),	
    ERR_TEST(1234.9876E300,KRealFormatNoExponent,12,10,'.','0',0,_S("Inf"),KErrOverflow),
	//
	ERR_TEST(1234.9876E-101,KRealFormatNoExponent,12,10,'.','0',0,_S("0"),KErrUnderflow),
    ERR_TEST(1234.9876E-104,KRealFormatNoExponent,13,11,'.','0',0,_S("0"),KErrUnderflow),
	ERR_TEST(1234.9876E-107,KRealFormatNoExponent,12,10,'.','0',0,_S("0"),KErrUnderflow),
    ERR_TEST(1234.9876E-109,KRealFormatNoExponent,12,10,'.','0',0,_S("0"),KErrUnderflow),
    ERR_TEST(1234.9876E-110,KRealFormatNoExponent,12,10,'.','0',0,_S("0"),KErrUnderflow),
    ERR_TEST(1234.9876E-200,KRealFormatNoExponent,12,10,'.','0',0,_S("0"),KErrUnderflow),
    ERR_TEST(1234.9876E-300,KRealFormatNoExponent,12,10,'.','0',0,_S("0"),KErrUnderflow),
	//
    ERR_TEST(.00568,KRealFormatNoExponent,3,3,'.','0',0,_S("0"),KErrUnderflow),
    ERR_TEST(12345.0,KRealFormatNoExponent,5,5,'.',',',3,_S("Inf"),KErrOverflow),
    ERR_TEST(100000.0,KRealFormatNoExponent,6,6,'.',',',3,_S("Inf"),KErrOverflow),
	ERR_TEST(100000.0,KRealFormatNoExponent,6,5,'.',',',3,_S("Inf"),KErrOverflow),
    ERR_TEST(99999.999,KRealFormatNoExponent,6,6,'.',',',3,_S("Inf"),KErrOverflow),
    ERR_TEST(1234567890.675,KRealFormatNoExponent,12,12,'.',',',3,_S("Inf"),KErrOverflow),
    ERR_TEST(1234567890.675,KRealFormatNoExponent,12,9,'.',',',3,_S("Inf"),KErrOverflow),
	ERR_TEST(1.0E3,KRealFormatNoExponent,4,4,'.',',',3,_S("Inf"),KErrOverflow),
	ERR_TEST(1234567890120.00,noExponentCalc,14,12,'.',',',0,_S("Inf"),KErrOverflow),
	ERR_TEST(999999999999.90,noExponentCalc,14,12,'.',',',0,_S("Inf"),KErrOverflow),
	//
	ERR_TEST(1234.9876,KRealFormatNoExponent,9,8,'.','0',0,_S("1234.9876"),9),
	ERR_TEST(1234.9876,KRealFormatNoExponent,12,12,'.','0',0,_S("1234.9876"),9),
	ERR_TEST(-1234.9876,KRealFormatNoExponent,10,10,'.','0',0,_S("-1234.9876"),10),
	ERR_TEST(-1234.9876,KRealFormatNoExponent,12,11,'.','0',0,_S("-1234.9876"),10),
	//
	//CALCULATOR - only two-digit exponents allowed - underflow
	ERR_TEST(1234.9876E-103,KRealFormatCalculator,12,6,'.','0',0,_S("0"),KErrUnderflow),
    ERR_TEST(1234.9876E-107,KRealFormatCalculator,12,6,'.','0',0,_S("0"),KErrUnderflow),
    ERR_TEST(1234.9876E-200,KRealFormatCalculator,12,6,'.','0',0,_S("0"),KErrUnderflow),
    ERR_TEST(1234.9876E-300,KRealFormatCalculator,12,6,'.','0',0,_S("0"),KErrUnderflow),    
	//  											- overflow
	ERR_TEST(1234.9876E97,KRealFormatCalculator,12,6,'.','0',0,_S("Inf"),KErrOverflow),
    ERR_TEST(1234.9876E100,KRealFormatCalculator,12,6,'.','0',0,_S("Inf"),KErrOverflow),
    ERR_TEST(1234.9876E200,KRealFormatCalculator,12,6,'.','0',0,_S("Inf"),KErrOverflow),
    ERR_TEST(1234.9876E300,KRealFormatCalculator,12,6,'.','0',0,_S("Inf"),KErrOverflow),
	//									            - does not fit
	ERR_TEST(1234.9876E24,KRealFormatCalculator,12,7,'.','0',0),
    ERR_TEST(1234.9876E95,KRealFormatCalculator,6,2,'.','0',0),
    ERR_TEST(1234.9876E-76,KRealFormatCalculator,9,6,'.','0',0),
    ERR_TEST(1234.9876E-98,KRealFormatCalculator,6,4,'.','0',0),
	ERR_TEST(-1234.9876E24,KRealFormatCalculator,9,8,'.','0',0),
    ERR_TEST(-1234.9876E95,KRealFormatCalculator,4,2,'.','0',0),
    ERR_TEST(-1234.9876E-76,KRealFormatCalculator,10,7,'.','0',0),
    ERR_TEST(-1234.9876E-98,KRealFormatCalculator,9,4,'.','0',0),
	// 			- three digits allowed - does not fit
	ERR_TEST(1234.9876E240,calculatorThreeDigitExp,13,7,'.','0',0),
    ERR_TEST(1234.9876E98,calculatorThreeDigitExp,6,1,'.','0',0),
    ERR_TEST(1234.9876E-160,calculatorThreeDigitExp,9,7,'.','0',0),
    ERR_TEST(1234.9876E-98,calculatorThreeDigitExp,10,6,'.','0',0),
	ERR_TEST(-1234.9876E240,calculatorThreeDigitExp,11,8,'.','0',0),
    ERR_TEST(-1234.9876E98,calculatorThreeDigitExp,7,1,'.','0',0),
    ERR_TEST(-1234.9876E-160,calculatorThreeDigitExp,10,7,'.','0',0),
    ERR_TEST(-1234.9876E-98,calculatorThreeDigitExp,11,6,'.','0',0),
	//
    ERR_TEST(1234.9876E24,calculatorThreeDigitExp,13,7,'.','0',0),
    ERR_TEST(1234.9876E95,calculatorThreeDigitExp,9,7,'.','0',0),
    ERR_TEST(1234.9876E-76,calculatorThreeDigitExp,12,6,'.','0',0),
    ERR_TEST(1234.9876E-98,calculatorThreeDigitExp,6,4,'.','0',0),
	ERR_TEST(-1234.9876E24,calculatorThreeDigitExp,9,8,'.','0',0),
    ERR_TEST(-1234.9876E95,calculatorThreeDigitExp,8,2,'.','0',0),
    ERR_TEST(-1234.9876E-76,calculatorThreeDigitExp,10,7,'.','0',0),
    ERR_TEST(-1234.9876E-98,calculatorThreeDigitExp,10,4,'.','0',0)
	};

GLDEF_D TUint size_testerr=sizeof(testerr);


GLDEF_D DtoR_ERR_TEST testerr2[]=
	{
	DtoR_ERR_TEST(KNullDesC,KErrGeneral),
	DtoR_ERR_TEST(_L("E-20"),KErrGeneral), 
	DtoR_ERR_TEST(_L("1.#INF"),KErrNone),
	DtoR_ERR_TEST(_L("1.#NAN"),KErrNone),
	DtoR_ERR_TEST(_L("c.fgh"),KErrGeneral),
	DtoR_ERR_TEST(_L("xyz"),KErrGeneral),
	DtoR_ERR_TEST(_L("1.0E"),KErrNone),
	DtoR_ERR_TEST(_L("1.0E2147483648"),KErrOverflow),
	DtoR_ERR_TEST(_L("1.234567801234567890E+308"),KErrNone),
	DtoR_ERR_TEST(_L("3.456789012345678901E-308"),KErrNone),
	DtoR_ERR_TEST(_L("1.797693134862316E+308"),KErrOverflow),
	DtoR_ERR_TEST(_L("2.225073858507201E-308"),KErrNone),
	DtoR_ERR_TEST(_L("3.0E+308"),KErrOverflow),
	DtoR_ERR_TEST(_L("1.0E-308"),KErrNone),
	DtoR_ERR_TEST(_L("3.0E310"),KErrOverflow),
	DtoR_ERR_TEST(_L("3.0E-310"),KErrNone),
	DtoR_ERR_TEST(_L("1.0E-325"),KErrUnderflow),
	DtoR_ERR_TEST(_L("2.0E-400"),KErrUnderflow),
	DtoR_ERR_TEST(_L("2.0E+400"),KErrOverflow),
	};

GLDEF_D TUint size_testerr2=sizeof(testerr2);

