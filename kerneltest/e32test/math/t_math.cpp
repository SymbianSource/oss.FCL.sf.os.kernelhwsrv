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
// e32test\math\t_math.cpp
// T_MATH.CPP - Test routines for the maths functions
// NB When considering the accuracy of the results (i.e. the tolerance used in testApprox()) it
// should be remembered that the results expected are not always given to full precision and so
// the results obtained are mostly as accurate as can be expected.
// Overview:
// Test functionality of the Math library.
// API Information:
// Math.
// Details:
// - Test math's trigonometric, powers, roots, logs, modulo, sqrt, exp,
// Int, Frac, rounding for range of input values are as expected.
// - Test the returned error values are as expected when illegal math's
// operations are done.
// - Check the return value is KErrTotalLossOfPrecision when incorrect values
// is passed to modulo function.
// - Test for success when the same variable for both operands in some 
// Math functions are used.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include "t_math.h"
#include "t_vals.h"

LOCAL_D RTest test(_L("T_MATH"));

LOCAL_D TInt64 rseed = MAKE_TINT64(123456789,987654321);

typedef struct
    {
    TReal num; // input number
    TReal res; // expected result
    } SQRT_TEST;

LOCAL_D SQRT_TEST testsqrt[]=
    {
    {0.0,0.0}, // zero
	{KNegZeroTReal64,KNegZeroTReal64},
    {1.0,1.0},
    {.64,.8},
    {.81,.9},
    {9,3},
    {25,5},
    {10000,100},
    {400,20},
    {6.25,2.5},
    {1E-98,1E-49},
    {1E-98,1E-49},
    {1E98,1E49},
    {1.0000000001,1.00000000005}
    };

typedef struct
    {
    TReal num; // input number
    TReal res; // expected result
    } TESTLN;

LOCAL_D TESTLN testln[]=
    {
    {.001,-6.9077552789821317},
    {.002,-6.2146080984221917},
    {.023,-3.7722610630529874},
    {.004,-5.5214609178622464},
    {.050,-2.9957322735539910},
    {.100,-2.3025850929940457},
    {.150,-1.8971199848858813},
    {.200,-1.6094379124341004},
    {.250,-1.3862943611198906},
    {.300,-1.2039728043259360},
    {.350,-1.0498221244986777},
    {.400,-0.9162907318741551},
    {.450,-0.7985076962177716},
    {.500,-0.6931471805599453},
    {.550,-0.5978370007556204},
    {.600,-0.5108256237659907},
    {.650,-0.4307829160924543},
    {.700,-0.3566749439387324},
    {.750,-0.2876820724517809},
    {.980,-0.0202027073175194},
    {.985,-0.0151136378100482},
    {.990,-0.0100503358535014},
    {.995,-0.0050125418235443},
    {.088,-2.4304184645039306},
    {1,0}
    };

typedef struct
    {
    TReal val; // value for which the exponent is to be found
    TReal result; // result
    } EXP;

LOCAL_D EXP testexp[]=
    {
    {4E-20,1.0}, 
	{5.4E-20,1.0},
	{0.0,1.0},
	{5E-324,1.0},
    };

typedef struct
    {
    TReal number; // number to be raised to a power
    TReal power; // power
    TReal result; // result
    } POWER;

LOCAL_D POWER testpow[]=
    {
	{45,3,91125.0},
	{-2,4,16},
    {2,-3,0.125},
    {-2,3,-8},
    {16,20,1.208925819614628E+24},
    };

// Added by AnnW, October 1996
LOCAL_D const POWER testpowexact[]=
	{
	{0.0,1.0,0.0},
	{0,7,0},
	{0.0,16.0,0.0},
	{0.0,3.9271E-17,0.0},
	{-2,0,1},
    {1,0,1},
	{1.545243,0,1},
	{4.8,0.0,1.0},
	{195.0,0.0,1.0},
	{1.0E-7,0.0,1.0},
	{1.0,2.0,1.0},
	{1.0,1.0E-6,1.0},
	{1.0,1.0E+10,1.0},
	{-1.0,2.0,1.0},
	{-1.0,1.0000000001E+10,-1.0},
	{-1.0,1.0E+10,1.0},
	{1.593704102953967e+3,1.0,1.593704102953967e+3},
	{1.234567E+50,1.0,1.234567E+50},
	{1.2345678901234567E+146,1.0,1.2345678901234567E+146},
	{-7.6543210987654321E-53,1.0,-7.6543210987654321E-53},
	{0.0,2.0,0.0},
	{KNegZeroTReal64,4.0,0.0},
	{KPosInfTReal64,-2.0,0.0},
	{KNegInfTReal64,-2.0,0.0},
	{2.0,KNegInfTReal64,0.0},
	{-2.0,KNegInfTReal64,0.0},
	{0.5,KPosInfTReal64,0.0},
	{-0.5,KPosInfTReal64,0.0},
	{KPosInfTReal64,-5.0,0.0},
	{KPosInfTReal64,-6.0,0.0},
	{KNegInfTReal64,KNegInfTReal64,0.0},
	{KPosInfTReal64,KNegInfTReal64,0.0},
	};

// Check ISO requirements on Pow()
//
typedef struct
	{
	TReal number;	// number to be raised to a power
	TReal power;	// power
	TInt rc;		// return value from Pow()
	TReal result;	// numerical result
	} POWERISO;

const TReal KPosZeroTReal64 = 0.0;

LOCAL_D const POWERISO testpow_iso[] =
	{
	// pow(+/-0, y) returns +/-INF and raises the ''divide-by-zero''
	// floating-point exception for y an odd integer < 0
	{ KPosZeroTReal64, -3.0, KErrOverflow, KPosInfTReal64 },	// 0
	{ KNegZeroTReal64, -3.0, KErrOverflow, KNegInfTReal64 },	// 1

	// pow(+/-0, y) returns +INF and raises the ''divide-by-zero''
	// floating-point exception for y < 0 and not an odd integer
	{ KPosZeroTReal64, -2.0, KErrOverflow, KPosInfTReal64 },	// 2
	{ KNegZeroTReal64, -2.0, KErrOverflow, KPosInfTReal64 },	// 3

	// pow(+/-0, y) returns +/-0 for y an odd integer > 0
	{ KPosZeroTReal64, 3.0, KErrNone, KPosZeroTReal64 },		// 4
	{ KNegZeroTReal64, 3.0, KErrNone, KNegZeroTReal64 },		// 5

	// pow(+/-0, y) returns +0 for y > 0 and not an odd integer
	{ KPosZeroTReal64, 2.0, KErrNone, KPosZeroTReal64 },		// 6
	{ KNegZeroTReal64, 2.0, KErrNone, KPosZeroTReal64 },		// 7

	// pow(-1, +/-INF) returns 1
	{ -1.0, KPosInfTReal64, KErrNone, 1.0 },					// 8
	{ -1.0, KNegInfTReal64, KErrNone, 1.0 },					// 9

	// pow(+1, y) returns 1 for any y, even a NaN
	{ 1.0, 1.0, KErrNone, 1.0 },								// 10
	{ 1.0, 10.0, KErrNone, 1.0 },								// 11
	{ 1.0, -1.0, KErrNone, 1.0 },								// 12
	{ 1.0, -10.0, KErrNone, 1.0 },								// 13
	{ 1.0, 0.5, KErrNone, 1.0 },								// 14
	{ 1.0, -0.5, KErrNone, 1.0 },								// 15
	{ 1.0, KPosInfTReal64, KErrNone, 1.0 },						// 16
	{ 1.0, KNegInfTReal64, KErrNone, 1.0 },						// 17
	{ 1.0, KNaNTReal64, KErrNone, 1.0 },						// 18

	// pow(x, +/-0) returns 1 for any x, even a NaN
	{  1.0, KPosZeroTReal64, KErrNone, 1.0 },					// 19
	{  1.0, KNegZeroTReal64, KErrNone, 1.0 },					// 20
	{  2.0, KPosZeroTReal64, KErrNone, 1.0 },					// 21
	{  2.0, KNegZeroTReal64, KErrNone, 1.0 },					// 22
	{  0.5, KPosZeroTReal64, KErrNone, 1.0 },					// 23
	{  0.5, KNegZeroTReal64, KErrNone, 1.0 },					// 24
	{ -1.0, KPosZeroTReal64, KErrNone, 1.0 },					// 25
	{ -1.0, KNegZeroTReal64, KErrNone, 1.0 },					// 26
	{ -2.0, KPosZeroTReal64, KErrNone, 1.0 },					// 27
	{ -2.0, KNegZeroTReal64, KErrNone, 1.0 },					// 28
	{ -0.5, KPosZeroTReal64, KErrNone, 1.0 },					// 29
	{ -0.5, KNegZeroTReal64, KErrNone, 1.0 },					// 30
	{ KPosZeroTReal64, KPosZeroTReal64, KErrNone, 1.0 },		// 31
	{ KPosZeroTReal64, KNegZeroTReal64, KErrNone, 1.0 },		// 32
	{ KNegZeroTReal64, KPosZeroTReal64, KErrNone, 1.0 },		// 33
	{ KNegZeroTReal64, KNegZeroTReal64, KErrNone, 1.0 },		// 34
	{ KPosInfTReal64, KPosZeroTReal64, KErrNone, 1.0 },			// 35
	{ KPosInfTReal64, KNegZeroTReal64, KErrNone, 1.0 },			// 36
	{ KNegInfTReal64, KPosZeroTReal64, KErrNone, 1.0 },			// 37
	{ KNegInfTReal64, KNegZeroTReal64, KErrNone, 1.0 },			// 38
	{ KNaNTReal64, KPosZeroTReal64, KErrNone, 1.0 },			// 39
	{ KNaNTReal64, KNegZeroTReal64, KErrNone, 1.0 },			// 40

	// pow(x, y) returns a NaN and raises the ''invalid'' floating-point
	// exception for finite x < 0 and finite non-integer y
	{ -1.0, 1.5, KErrArgument, KNaNTReal64 },					// 41

	// pow(x, -INF) returns +INF for |x| < 1
	{ 0.5, KNegInfTReal64, KErrOverflow, KPosInfTReal64 },		// 42
	{ -0.5, KNegInfTReal64, KErrOverflow, KPosInfTReal64 },		// 43

	// pow(x, -INF) returns +0 for |x| > 1
	{ 2, KNegInfTReal64, KErrNone, KPosZeroTReal64 },			// 44
	{ -2, KNegInfTReal64, KErrNone, KPosZeroTReal64 },			// 45
	{ 4.5, KNegInfTReal64, KErrNone, KPosZeroTReal64 },			// 46
	{ -4.5, KNegInfTReal64, KErrNone, KPosZeroTReal64 },		// 47

	// pow(x, +INF) returns +0 for |x| < 1
	{ .5, KPosInfTReal64, KErrNone, KPosZeroTReal64 },			// 48
	{ -.5, KPosInfTReal64, KErrNone, KPosZeroTReal64 },			// 49

	// pow(x, +INF) returns +INF for |x| > 1
	{ 2, KPosInfTReal64, KErrOverflow, KPosInfTReal64 },		// 50
	{ -2, KPosInfTReal64, KErrOverflow, KPosInfTReal64 },		// 51
	{ 4.5, KPosInfTReal64, KErrOverflow, KPosInfTReal64 },		// 52
	{ -4.5, KPosInfTReal64, KErrOverflow, KPosInfTReal64 },		// 53

	// pow(-INF, y) returns -0 for y an odd integer < 0
	{ KNegInfTReal64, -1, KErrNone, KNegZeroTReal64 },			// 54
	{ KNegInfTReal64, -5, KErrNone, KNegZeroTReal64 },			// 55

	// pow(-INF, y) returns +0 for y < 0 and not an odd integer
	{ KNegInfTReal64, -2, KErrNone, KPosZeroTReal64 },			// 56
	{ KNegInfTReal64, -5.5, KErrNone, KPosZeroTReal64 },		// 57

	// pow(-INF, y) returns -INF for y an odd integer > 0
	{ KNegInfTReal64, 1, KErrOverflow, KNegInfTReal64 },		// 58
	{ KNegInfTReal64, 5, KErrOverflow, KNegInfTReal64 },		// 59

	// pow(-INF, y) returns +INF for y > 0 and not an odd integer
	{ KNegInfTReal64, 2, KErrOverflow, KPosInfTReal64 },		// 60
	{ KNegInfTReal64, 5.5, KErrOverflow, KPosInfTReal64 },		// 61

	// pow(+INF, y) returns +0 for y < 0
	{ KPosInfTReal64, -1, KErrNone, KPosZeroTReal64 },			// 62
	{ KPosInfTReal64, -2, KErrNone, KPosZeroTReal64 },			// 63
	{ KPosInfTReal64, -5, KErrNone, KPosZeroTReal64 },			// 64
	{ KPosInfTReal64, -5.5, KErrNone, KPosZeroTReal64 },		// 65

	// pow(+INF, y) returns +INF for y > 0
	{ KPosInfTReal64, 1, KErrOverflow, KPosInfTReal64 },		// 66
	{ KPosInfTReal64, 2, KErrOverflow, KPosInfTReal64 },		// 67
	{ KPosInfTReal64, 5, KErrOverflow, KPosInfTReal64 },		// 68
	{ KPosInfTReal64, 5.5, KErrOverflow, KPosInfTReal64 },		// 69
	};

struct POW10_TEST
    {
    TInt num; // input number
    TReal res; // expected result
    };

LOCAL_D POW10_TEST pow10teste[]=
	{
	{300,1.0E300},		
	{-162,1.0E-162},
	{-300,1.0E-300},
	{-99,1.0E-99},
//	};

//LOCAL_D POW10_TEST pow10testa[]=
//	{
	{99,1.0E99},
	{283,1.0E283},
	{-89,1.0E-89},
	{-200,1.0E-200},
	{-43,1.0E-43},
	{24,1.0E24},
 	{-310,K1EMinus310Real64},
 	{-323,K1EMinus323Real64}
	};

typedef struct
    {
    TReal num; // input number
    TReal res; // expected result
    } TESTSINE;

#pragma warning ( disable : 4204 ) // non-constant aggregate initializer
LOCAL_D TESTSINE testsin[]=
    {
	{0.5,0.4794255386042029},						// These were found using S3a
	{1.2,0.9320390859672263},
	{1.6,0.9995736030415051},
	{28.6,-0.3199399618841981},
	{-18.3,0.5223085896267315},
	{KPi/4,0.7071067811865474},
	{3*KPi/4,0.7071067811865474},
	{5*KPi/4,-0.7071067811865474},
	{-KPi/4,-0.7071067811865474},
	{KPi/3,0.8660254037844387},
	{-KPi/3,-0.8660254037844387},
	{KPi/6,0.5},
	{-KPi/6,-0.5},
	{150*KDegToRad,0.5},
	{210*KDegToRad,-0.5},
//	{KPi+1.0E-15,-7.657143961860984E-16},	// loss of significance will limit accuracy here
//	2*(KPi+1.0E-15),1.5314287923721969e-15}
    };
    
typedef struct
    {
    TReal num; // input number
    TReal res; // expected result
    } TESTCOSINE;

LOCAL_D TESTCOSINE testcos[]=
	{
	{0.5,0.8775825618903727},			// These were found using S3a
	{1.2,0.3623577544766734},
	{1.6,-0.0291995223012888},
	{28.6,-0.9474378189567576},
	{-18.3,0.8527565521308730},
	{KPi/4,0.7071067811865474},
	{3*KPi/4,-0.7071067811865474},
	{5*KPi/4,-0.7071067811865474},
	{-KPi/4,0.7071067811865474},
	{KPi/6,0.8660254037844387},
	{5*KPi/6,-0.8660254037844387},
	{KPi/3,0.5},
	{4*KPi/3,-0.5},
	{120*KDegToRad,-0.5},
	{300*KDegToRad,0.5},
	{KPi+1.0E-15,-1.0},
	{2*(KPi+1.0E-15),1.0}
	};

typedef struct
    {
    TReal angle; // angle for which the tangent is to be found
    TReal result; // result
    } TAN;

LOCAL_D TAN testtan[]=
    {
	{KPi/4,1.0},
	{-KPi/4,-1.0},
	{45*KDegToRad,1.0},
	{KPi/3,1.732050807568877},					// Added by AnnW - Calculated on S3a
	{2*KPi/3,-1.732050807568878},				//
	{KPi/6,0.5773502691896257},					//
	{-KPi/6,-0.5773502691896257},				//
	{89*KDegToRad,57.28996163075913},			// these two should be the same!
	{91*KDegToRad,-57.28996163075955},			//
    {4E-123,4E-123},								
    {-4E-123,-4E-123},	
    };
    
typedef struct
    {
    TReal num; // input number
    TReal res; // expected result
    } TESTASC;

LOCAL_D TESTASC testas[]=
    {
    {.75,.848062078981},
    {.82,.961411018764},
    {.87,1.055202320549},
    {.89,1.097345169523},
    {.90,1.119769514999},
    {.92,1.168080485214},
    {.94,1.222630305522},
    {.96,1.287002217587},
    {.99,1.429256853470},
    {1.0,1.570796326795},
	{0.0,0},
	{-1.0, -90.0*KDegToRad},
	{0.5,30.0*KDegToRad}
    };

typedef struct
    {
    TReal num1; // Divisor
    TReal num2; // Divand
    TReal res; // expected result
    } TESTATAN2;

LOCAL_D TESTATAN2 testat2[]=
    {
    {5E-49,7E306,0.0}, // underflow, zero returned
    {5E49,7E-306,KPiBy2}, // overflow, pi/2 returned
    {0.45,0.5,0.732815101787},
    {0.12,0.3,0.380506377112},
    {0.3,0.0,KPiBy2}, // overflow, pi/2 returned
    {-0.3,0.0,-KPiBy2}, // overflow, -pi/2 returned
    {0.0,0.3,0.0},
    };
#pragma warning ( default : 4204 )

typedef struct
    {
    TReal num; // input number
    TReal res; // expected result
    } INT_TEST;

LOCAL_D INT_TEST testint1[]=
    {
    {1.0,1.0},
    {1.47934,1.0},
    {-72.86345,-72.0},
    {-734.9999,-734.0},
    {4855.9974,4855.0},
    {232478.35,232478.0},
    {0.029345,0.0},
    {0.9437,0.0},
    {-0.2634,0.0},
    {-0.98976,0.0},
    {32769.36946,32769.0},
    {-32774.997937,-32774.0},
    {8738465.38749,8738465.0},
    {-2348645.34965,-2348645.0},
    {2147483655.7565,2147483655.0},
    {-2147483657.89453,-2147483657.0},
    {2374843546.34E2,2374843546.34E2},
    {34780656.37643E12,34780656.37643E12},
    {-2374843546.34E2,-2374843546.34E2},
    {-34780656.37643E12,-34780656.37643E12},
    {468650.3874E47,468650.3874E47},
    {-4965.5987636E34,-4965.5987636E34},
    };

typedef struct
    {
    TReal num; // input number
    TInt16 res; // expected result
    } INTI_TEST;

LOCAL_D INTI_TEST testint2[]=
    {
    {1.0,1},
    {1.47934,1},
    {-72.86345,-72},
    {-734.9999,-734},
    {4855.9974,4855},
    {0.029345,0},
    {0.9437,0},
    {-0.2634,0},
    {-0.98976,0},
    {3234.56,3234},
    {4698.435,4698},
    {-32767.47658,-32767},
    {32767.9830857,32767},
    {-32768.47658,-32767-1}
    };

typedef struct
    {
    TReal num; // input number
    TInt32 res; // expected result
    } INTL_TEST;

LOCAL_D INTL_TEST testint3[]=
    {
    {1.0,1l},
    {1.47934,1l},
    {-72.86345,-72l},
    {-734.9999,-734l},
    {4855.9974,4855l},
    {0.029345,0l},
    {0.9437,0l},
    {-0.2634,0l},
    {-0.98976,0l},
    {3234.56,3234l},
    {4698.435,4698l},
    {-32767.47658,-32767l},
    {32767.9830857,32767l},
    {32769.36946,32769l},
    {-32774.997937,-32774l},
    {64835903.74605,64835903l},
    {-46652024.393,-46652024l},
    {2147483647.34576,2147483647l},
    {-2147483647.9501,-2147483647l},
    {-2147483648.00,0x80000000l}, 
    {-2147483648.6843,0x80000000l}
    };

typedef struct
    {
    TReal num; // input number
    TReal res; // expected result
    } FRAC_TEST;

LOCAL_D FRAC_TEST testfrac[]=
    {
	{0.0,0.0},
	{KNegZeroTReal64,0.0},
    {1.0,0.0},
    {1.47934,.47934},
    {-72.86345,-.86345},
    {-734.9999,-.9999},
    {4855.9974,.9974},
    {232478.35,.35},
    {0.029345,.029345},
    {0.9437,0.9437},
    {-0.2634,-.2634},
    {-0.98976,-.98976},
    {32769.36946,.36946},
    {-32774.997937,-0.997937},
    {8738465.38749,0.38749},
    {-2348645.34965,-0.34965},
    {2147483655.7565,0.7565},
    {-2147483657.89453,-.89453},
    {2374843546.34E2,0.0},
    {34780656.37643E12,0.0},
    {-2374843546.34E2,0.0},
    {-34780656.37643E12,0.0},
    {468650.3874E47,0.0},
    {-4965.5987636E34,0.0}
    };

typedef struct
    {
    TReal num; // input number
    TReal mod; // modulo
    TReal res; // expected result
    } MOD_TEST;

LOCAL_D MOD_TEST testmod[]=
    {
    {4.0,2.0,0.0},
    {3.0,2.0,1.0},
    {56.847,2.3,1.647},
    {-65.6478,.65,-.6478},
    {-6858.78432,-87.5323,-31.26492},
    {7665.140215,-34.98,4.520215},
    {.4645,1.0,0.4645},
    {-.246,1.0,-.246},
	{1.0,KPosInfTReal64,1.0},
	{1.0,KNegInfTReal64,1.0},
	{1.0E17,8.0,0.0},
	//
	{1.0,3.0,1.0},				//0
	{2.0,3.0,2.0},
	{4.0,3.0,1.0},
	{8.0,3.0,2.0},
	{16.0,3.0,1.0},
	{32.0,3.0,2.0},
	{64.0,3.0,1.0},
	{128.0,3.0,2.0},
	{256.0,3.0,1.0},
	{512.0,3.0,2.0},
	{1024.0,3.0,1.0},			//10
	{2048.0,3.0,2.0},
	{4096.0,3.0,1.0},
	{8192.0,3.0,2.0},
	{16384.0,3.0,1.0},
	{32768.0,3.0,2.0},
	{65536.0,3.0,1.0},
	{131072.0,3.0,2.0},
	{262144.0,3.0,1.0},
	{524288.0,3.0,2.0},
	{1048576.0,3.0,1.0},		//20
	{2097152.0,3.0,2.0},
	{4194304.0,3.0,1.0},
	{8388608.0,3.0,2.0},
	{16777216.0,3.0,1.0},
	{33554432.0,3.0,2.0},
	{67108864.0,3.0,1.0},
	{134217728.0,3.0,2.0},
	{268435456.0,3.0,1.0},
	{536870912.0,3.0,2.0},
	{1073741824.0,3.0,1.0},		//30
	{2147483648.0,3.0,2.0},
	{4294967296.0,3.0,1.0},
	{8589934592.0,3.0,2.0},
	{17179869184.0,3.0,1.0},
	{34359738368.0,3.0,2.0},
	{68719476736.0,3.0,1.0},
	{137438953472.0,3.0,2.0},
	{274877906944.0,3.0,1.0},
	{549755813888.0,3.0,2.0},
	{1099511627776.0,3.0,1.0},	//40
	{2199023255552.0,3.0,2.0},
	{4398046511104.0,3.0,1.0},
	{8796093022208.0,3.0,2.0},
	{17592186044416.0,3.0,1.0},
	{35184372088832.0,3.0,2.0},
	{70368744177664.0,3.0,1.0},
	{140737488355328.0,3.0,2.0},
	{281474976710656.0,3.0,1.0},
	{562949953421312.0,3.0,2.0},
	{1125899906842624.0,3.0,1.0},	//50
	{2251799813685248.0,3.0,2.0},
	{4503599627370496.0,3.0,1.0},
	{9007199254740992.0,3.0,2.0},
	{18014398509481984.0,3.0,1.0},
	{6.626176E-34,299792458.0,6.626176E-34},
	{-1.6022E-19,6.022045E23,-1.6022E-19},
	{0.0,2.71828182845904524,0.0}
    };

// expected result is unused in following - will be zero in all cases
LOCAL_D MOD_TEST testmod2[]=
    {
	{1.0E17,7.9,0.0},
	{1.0E100,4.0,0.0},
	{KMaxTReal64,5.0,0.0},
	{-KMaxTReal64,5.0,0.0},
	{0.125,1.0E-17,0.0},
	{36028797019963968.0,2.0,0.0},   // 2**55,2**1
	//
	{36028797019963968.0,3.0,0.0},	//55
	{72057594039927936.0,3.0,0.0},
	{144115188079855872.0,3.0,0.0},
	{288230376159711744.0,3.0,0.0},
	};

TInt testApprox(TReal aFound,TReal aExpect,TReal aTol)
//
// Tests relative error, i.e. whether (aFound-aExpect)/aFound <= aTol
//
	{

	TRealX diff,check,l,r,t;
	l.Set(aFound);
	r.Set(aExpect);
	t.Set(aTol);
	if (l.Mult(check,t)==KErrUnderflow)
		{
		l*=TRealX(1.0E20);
		r*=TRealX(1.0E20);
		}
	diff=l-r;
	if (diff.IsZero())
		return ETrue;
	if (!l.IsZero())
		diff.DivEq(l);
	if (Abs(TReal(diff))<=aTol)
		return ETrue;
	return EFalse;
	}

LOCAL_C void randrng(TReal& pret,TReal& llim,TReal& ulim)
/*
Returns a random number in the range [llim,ulim]
*/
    {

    pret=Math::FRand(rseed);
    pret*=ulim-llim;
    pret+=llim;
    }

LOCAL_C TReal taylor(TReal x,TInt k)
/*
Evaluate the Taylor series approximation to arc sine up to terms of order k
*/
    //TReal x; // argument
    //TInt k; // Highest order term
    {

    TInt i,j;
    TReal den,num,res,term,di;

    den=1;
    num=1;
    term=0;
    for (i=1;i<=k;i+=2)
		{
		for (j=2;j<i;j+=2)
			{
			num*=j;
			if (j<(i-1))
			den*=j+1;
			}
		di=(TReal)i;
		Math::Pow(res,x,di);
		term+=(res*den)/(i*num);
		num=1;
		den=1;
		}
    return(term);
    }

LOCAL_C TReal tayatan(TReal val)
/* 
Finds the taylor series approximation to the arc tangent function 
*/
    //TReal val;
    {

    TInt i;
    TReal sgn,s,d,di,term,res;
    
    term=0.0;
    s=(-1.0);
    for (i=0;i<8;i++)
		{
		di=(TReal)i;
		d=2.0*di;
		Math::Pow(sgn,s,di);
		Math::Pow(res,val,d);
		term+=(sgn*res)/(2.0*di+1.0);
		}
    return(val*term);
    }

LOCAL_C void AssortedTests()
//
// Tests the methods with just a handful of values each 
// All tests as accurate as possible - if exact answer given, tests for equality
//
	{

	TReal trg,src;

	// ASin
	test.Start(_L("Math::ASin()"));
	test(Math::ASin(trg,0.0)==KErrNone);
	test(trg==0.0);

	test(Math::ASin(trg,1.0)==KErrNone);
	test(testApprox(trg,1.5707963267949,5.0E-15));

	// ACos
	test.Next(_L("Math::ACos()"));
	test(Math::ACos(trg,0)==KErrNone);
	test(testApprox(trg,1.5707963267949,5.0E-15));

	test(Math::ACos(trg,1.0)==KErrNone);
	test(trg==0.0);

	// ATan
	test.Next(_L("Math::ATan()"));
	test(Math::ATan(trg,0.0)==KErrNone);
	test(trg==0.0);

	test(Math::ATan(trg,1.0)==KErrNone);
	test(testApprox(trg,0.78539816339745,5.0E-15));	

	test(Math::Tan(trg,KPi/4)==KErrNone);
	test(testApprox(trg,1.0,1.0E-15));
	test(Math::ATan(trg,trg)==KErrNone);
	test(testApprox(trg,KPi/4,1e-15));

	// Sqrt
	test.Next(_L("Math::Sqrt()"));
	test(Math::Sqrt(trg,0.0)==KErrNone);
	test(trg==0.0);
	
	test(Math::Sqrt(trg,-1.0)==KErrArgument);

	test(Math::Sqrt(trg,100.0)==KErrNone);
	test(testApprox(trg,10.0,1.0E-15));	

	test(Math::Sqrt(trg,56.25)==KErrNone);
	test(trg==7.5);

	// Pow10
	test.Next(_L("Math::Pow10()"));
	test(Math::Pow10(trg,-2)==KErrNone);
	test(trg==0.01);	

	test(Math::Pow10(trg,-1)==KErrNone);
	test(trg==0.1);

	test(Math::Pow10(trg,0)==KErrNone);
	test(trg==1.0);

	test(Math::Pow10(trg,1)==KErrNone);
	test(trg==10.0);

	test(Math::Pow10(trg,2)==KErrNone);
	test(trg==100.0);

	// Ln
	test.Next(_L("Math::Ln()"));
	test(Math::Ln(trg,0.0)==KErrOverflow);
	
	test(Math::Ln(trg,1.0)==KErrNone);
	test(trg==0.0);

	test(Math::Ln(trg,2)==KErrNone);
	test(testApprox(trg,0.69314718055995,1.0E-14));	

	// Log
	test.Next(_L("Math::Log()"));
	test(Math::Log(trg,0)==KErrOverflow);

	test(Math::Log(trg,1)==KErrNone);
	test(trg==0);

	test(Math::Log(trg,10)==KErrNone);
	test(trg==1);

	test(Math::Log(trg,100000)==KErrNone);
	test(trg==5);

	// Sin
	test.Next(_L("Math::Sin()"));
	test(Math::Sin(trg,0)==KErrNone);
	test(trg==0);   

	test(Math::Sin(trg,1)==KErrNone);
	test(testApprox(trg,0.84147098480790,5.0E-15));	

	test(Math::Sin(trg,KPi)==KErrNone);
//    test(trg==0.0);
	test(Abs(trg)<1e-15);

	test(Math::Sin(trg,KPiBy2)==KErrNone);
	test(testApprox(trg,1.0,1.0E-15));

	test(Math::Sin(trg,10.0*KPi)==KErrNone);
//   test(trg==0.0);
	test(Abs(trg)<2e-15);

	test(Math::Sin(trg,3)==KErrNone);
	test(trg==0.1411200080598672);

	test(Math::Sin(trg,4)==KErrNone);
	test(trg==-0.7568024953079282);

	test(Math::Sin(trg,3.1415)==KErrNone);
	test(testApprox(trg,9.26535896605E-5,2.0E-13));	

	test(Math::Sin(trg,3.1416)==KErrNone);
	test(testApprox(trg,-7.3464102066435914E-6,1.0E-11));	

	test(Math::Sin(trg,(10.0*KPi)+0.001)==KErrNone);
	test(testApprox(trg,0.000999999833333,4.0E-13));	

	// Cos
	test.Next(_L("Math::Cos()"));
	test(Math::Cos(trg,0.0)==KErrNone);
	test(testApprox(trg,1.0,1.0E-15));		

	test(Math::Cos(trg,1)==KErrNone);
	test(testApprox(trg,0.54030230586814,1.0E-15));		

    test(Math::Cos(trg,KPiBy2)==KErrNone);
//    test(trg==0.0);
	test(Abs(trg)<1e-15);

	test(Math::Cos(trg,KPi)==KErrNone);
	test(trg==-1.0);

    test(Math::Cos(trg,KPiBy2+KPi)==KErrNone);
//    test(trg==0.0);
	test(Abs(trg)<1e-15);
	
	test(Math::Cos(trg,89.99999*KDegToRad)==KErrNone);
	test(testApprox(trg,1.745329252E-07,5.0E-10));		

	test(Math::Cos(trg,90.00001*KDegToRad)==KErrNone);
	test(testApprox(trg,-1.7453292516217e-007,5.0E-10));			

	// Tan
	test.Next(_L("Math::Tan()"));
	test(Math::Tan(trg,0.0)==KErrNone);
    test(trg==0.0);   

	test(Math::Tan(trg,1)==KErrNone);
	test(testApprox(trg,1.5574077246549,2.0E-15));			

	// Pow
	test.Next(_L("Math::Pow()"));
	src=10;
	test(Math::Pow(trg,src,-1.0)==KErrNone);
	test(testApprox(trg,0.1,1.0E-15));			

	test(Math::Pow(trg,src,0.0)==KErrNone);
	test(trg==1.0);

	test(Math::Pow(trg,src,2.0)==KErrNone);
	test(testApprox(trg,100.0,1.0E-15));			

	src=1.0;
	test(Math::Pow(trg,src,10000000000000000.0)==KErrNone);
	test(trg==1.0);

	test.End();
	}       

LOCAL_C void sqrtest1(TReal low,TReal upp)
/*
Test the identity sqrt(x*x)=x  on the range low<=x<upp
*/
    {
    
	TReal x,y,res;

    for (TInt j=0;j<100;j++)
		{
		randrng(x,low,upp);
		y=x*x;
		test(Math::Sqrt(res,y)==KErrNone);
		test(testApprox(res,x,1.0E-15));
		}
    }

LOCAL_C void sqrtest2()
/*
Tests specific numbers
*/
    {
    
	TReal root;

	// test errors
	test(Math::Sqrt(root,KNaNTReal64)==KErrArgument);
	test(Math::IsNaN(root));
	test(Math::Sqrt(root,-1)==KErrArgument);
	test(Math::IsNaN(root));
	test(Math::Sqrt(root,KNegInfTReal64)==KErrArgument);
	test(Math::IsNaN(root));
	test(Math::Sqrt(root,KPosInfTReal64)==KErrOverflow);
	test(root==KPosInfTReal64);

    TInt i=sizeof(testsqrt)/sizeof(SQRT_TEST);
    for (TInt j=0;j<i;j++) 
		{
		test(Math::Sqrt(root,testsqrt[j].num)==KErrNone);
		test(testApprox(root,testsqrt[j].res,1.0E-15));
		}

	// a couple of denormal tests
	test(Math::Sqrt(root,4E-322)==KErrNone);
	test(testApprox(root,2E-161,1.0E-3));
	test(Math::Sqrt(root,1.6E-309)==KErrNone);
	test(testApprox(root,4E-155,1.0E-15));	
    }

LOCAL_C void logtest()
/*
Test numbers in the range sqrt(.1) to .9, using the identity 
log(x)=log(11x/10)-log(1.1) 
*/
    {

    TReal res,x;
    TReal cnstlog,cnstlogx;

    TReal low=.316227766017;
    TReal upp=0.9;
    TReal cnst=11.0/10.0;
    test(Math::Log(cnstlog,cnst)==KErrNone);
    for (TInt j=0;j<10;j++)
		{
		randrng(x,low,upp);
		test(Math::Log(res,x)==KErrNone); 
		TReal num=cnst*x;
		test(Math::Log(cnstlogx,num)==KErrNone);
		test(testApprox(res,(cnstlogx-cnstlog),1.0E-15));
		}
    }

LOCAL_C void lntest1()
/* 
Test selected numbers 
*/
    {
 
    TReal res;

	// test errors
//	test(Math::Ln(res,KNegZeroTReal64)==KErrArgument);
	test(Math::Ln(res,KNegZeroTReal64)==KErrOverflow);
	test(Math::IsInfinite(res));
	test(Math::Ln(res,-34)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Ln(res,KNaNTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Ln(res,KNegInfTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Ln(res,KPosInfTReal64)==KErrOverflow);
	test(res==KPosInfTReal64);
	test(Math::Ln(res,0.0)==KErrOverflow);
	test(res==KNegInfTReal64);
	test(Math::Ln(res,2.71828182845904524)==KErrNone);
	test(testApprox(res,1.0,1e-15));
	test(Math::Ln(res,7.389056098930650227)==KErrNone);
	test(testApprox(res,2.0,1e-15));

    TInt i=sizeof(testln)/sizeof(TESTLN);
    for (TInt j=0;j<i;j++) 
		{
		test(Math::Ln(res,testln[j].num)==KErrNone);
		test(testApprox(res,testln[j].res,1.0E-14));
		}

	// test some denormals
 	test(Math::Log(res,K1EMinus322Real64)==KErrNone);
	test(testApprox(res,-322.0,2.0E-5));
 	test(Math::Log(res,K1EMinus313Real64)==KErrNone);
	test(testApprox(res,-313.0,1.0E-13));	
    }

LOCAL_C void lntest2()
/* 
Test numbers near to one against the Taylor series approximation 
*/
    {
    
	TReal x,res;
    
    TReal low=.999999989463;
    TReal upp=1.00000001054;
    for (TInt k=0;k<10;k++)
		{
		randrng(x,low,upp);
		TRealX tot=0.0;
		TRealX xx(x-1);
		TInt sign=-1;
		for (TInt i=4;i>0;i--)
			{
			tot+=TRealX(sign)/TRealX(i);
			tot*=xx;
			sign=-sign;
			}
		TReal tot2=(TReal)tot;
		test(Math::Ln(res,x)==KErrNone);
		test(testApprox(res,tot2,1.0E-15));
		}
    }

LOCAL_C void lntest3()
/* 
Test numbers in the range sqrt(.5) to 15/16, using the identity 
ln(x)=ln(17x/16)-ln(17/16) 
*/
    {

    TReal x,cnstln,cnstlnx,res;

	TReal low=KSqhf;
    TReal upp=15.0/16.0;
    TReal cnst=17.0/16.0;
    test(Math::Ln(cnstln,cnst)==KErrNone);
	for (TInt j=0;j<10;j++)
		{
		randrng(x,low,upp);
		test(Math::Ln(res,x)==KErrNone);
		TReal num=cnst*x;
		test(Math::Ln(cnstlnx,num)==KErrNone);
		test(testApprox(res,(cnstlnx-cnstln),1.0E-15));
		}
    }

LOCAL_C void lntest4()
/* 
Test numbers in the range 16 to 240 using the identity ln(x*x)=2ln(x) 
*/
    {

    TReal cnstlnx,res;

    TReal low=16.0;
    TReal upp=240.0;
    TReal x=16.0;
	test(Math::Ln(res,-1)==KErrArgument);
    for (TInt j=0;j<10;j++)
		{
		randrng(x,low,upp);
		TReal num=x*x;
		test(Math::Ln(res,num)==KErrNone);
		test(Math::Ln(cnstlnx,x)==KErrNone);
		test(testApprox(res,2*cnstlnx,1.0E-15));
		}
    }

LOCAL_C void exptest1()
/* 
To test exponent for specific values 
*/
    {

    TReal res;

	// test errors
	test(Math::Exp(res,KNaNTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Exp(res,KPosInfTReal64)==KErrOverflow);
	test(res==KPosInfTReal64);
	test(Math::Exp(res,709.8)==KErrOverflow);
	test(res==KPosInfTReal64);
	test(Math::Exp(res,KNegInfTReal64)==KErrUnderflow);
	test(Math::IsZero(res));
	test(Math::Exp(res,-745.2)==KErrUnderflow);
	test(Math::IsZero(res));

    TInt i=sizeof(testexp)/sizeof(EXP);
    for (TInt j=0;j<i;j++)
		{
		test(Math::Exp(res,testexp[j].val)==KErrNone);
		test(testApprox(res,testexp[j].result,0));	// NB only tests values with results of 1
		}

	// test some denormals
	test(Math::Exp(res,5E-324)==KErrNone);
	test(testApprox(res,1.0,0));
	test(Math::Exp(res,-6E-318)==KErrNone);
	test(testApprox(res,1.0,0));	
	}

LOCAL_C void exptest2(TReal cnst,TReal ll,TReal ul)
/*
Test the identity exp(x-cnst)=exp(x)*exp(-cnst) for x in the range [ul,ll]
*/
    //TReal cnst; // constant used in the identity
    //TReal ll; // Lower limit of the range
    //TReal ul; // Upper limit of the range
    {

    TReal cnstexp,cnstexpx,x,res;

    test(Math::Exp(cnstexp,cnst)==KErrNone);
    for (TInt j=0;j<10;j++)
		{
		randrng(x,ll,ul);
		test(Math::Exp(res,x)==KErrNone);
		TReal num=x+cnst;
		test(Math::Exp(cnstexpx,num)==KErrNone);
		test(testApprox(cnstexpx,(res*cnstexp),1.0E-15));
		}
    }

LOCAL_C void exptest3()
/* 
Test for systematic error 
*/
    {
    
	TReal step,ul,v;

    TReal x=1.0123;
    TReal y=x/2;
    test(Math::Exp(v,y)==KErrNone);
    test(Math::Exp(step,x)==KErrNone);
    test(Math::Sqrt(ul,step)==KErrNone);
	test(testApprox(ul,v,1.0E-15));
    }

LOCAL_C void powtest1()
/*
Test selected numbers
*/
    {
    
	TReal res;
	
	// test errors
	test(Math::Pow(res,10,-1E8)==KErrUnderflow);
	test(res==0.0);
	test(Math::Pow(res,10,-KMaxTReal64)==KErrUnderflow);
	test(res==0.0);
	test(Math::Pow(res,10,-5.5E307)==KErrUnderflow);
	test(res==0.0);
	test(Math::Pow(res,10,-5.4E307)==KErrUnderflow);
	test(res==0.0);
	test(Math::Pow(res,10,-1E300)==KErrUnderflow);
	test(res==0.0);
	test(Math::Pow(res,10,-1E10)==KErrUnderflow);
	test(res==0.0);
	
	test(Math::Pow(res,10,5.5E307)==KErrOverflow);
	test(res==KPosInfTReal64);
	test(Math::Pow(res,10,5.4E307)==KErrOverflow);
	test(res==KPosInfTReal64);
	test(Math::Pow(res,10,1E308)==KErrOverflow);
	test(res==KPosInfTReal64);
	test(Math::Pow(res,10,1.7E308)==KErrOverflow);
	test(res==KPosInfTReal64);
	test(Math::Pow(res,10,KMaxTReal64)==KErrOverflow);
	test(res==KPosInfTReal64);
	
	test(Math::Pow(res,1.0,KNaNTReal64)==KErrNone);
	test(res==1.0);
	test(Math::Pow(res,KNaNTReal64,1.0)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Pow(res,0.0,KNaNTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Pow(res,KNaNTReal64,0.0)==KErrNone);
	test(res==1.0);
	test(Math::Pow(res,KNaNTReal64,KNaNTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Pow(res,KPosInfTReal64,KPosInfTReal64)==KErrOverflow);
	test(res==KPosInfTReal64);
//	test(Math::Pow(res,KNegInfTReal64,KPosInfTReal64)==KErrOverflow);
//	test(res==KPosInfTReal64);
	test(Math::Pow(res,KNegInfTReal64,KPosInfTReal64)==KErrOverflow);
	test(res==KPosInfTReal64);
	test(Math::Pow(res,2.0,KPosInfTReal64)==KErrOverflow);
	test(res==KPosInfTReal64);
//	test(Math::Pow(res,-2.0,KPosInfTReal64)==KErrOverflow);
//	test(res==KPosInfTReal64);
	test(Math::Pow(res,-2.0,KPosInfTReal64)==KErrOverflow);
	test(res==KPosInfTReal64);
	test(Math::Pow(res,0.5,KNegInfTReal64)==KErrOverflow);
	test(res==KPosInfTReal64);
//	test(Math::Pow(res,-0.5,KNegInfTReal64)==KErrOverflow);
//	test(res==KPosInfTReal64);
	test(Math::Pow(res,-0.5,KNegInfTReal64)==KErrOverflow);
	test(res==KPosInfTReal64);
//	test(Math::Pow(res,1.0,KPosInfTReal64)==KErrArgument);
//	test(Math::IsNaN(res));
	test(Math::Pow(res,1.0,KPosInfTReal64)==KErrNone);
	test(res==1.0);
	test(Math::Pow(res,-1.0,KPosInfTReal64)==KErrNone);
	test(res==1.0);
//	test(Math::Pow(res,1.0,KNegInfTReal64)==KErrArgument);
//	test(Math::IsNaN(res));
	test(Math::Pow(res,1.0,KNegInfTReal64)==KErrNone);
	test(res==1.0);
	test(Math::Pow(res,-1.0,KNegInfTReal64)==KErrNone);
	test(res==1.0);
	test(Math::Pow(res,0.0,0.0)==KErrNone);
	test(res==1.0);
	test(Math::Pow(res,KNegZeroTReal64,KNegZeroTReal64)==KErrNone);
	test(res==1.0);
	test(Math::Pow(res,0.0,KNegZeroTReal64)==KErrNone);
	test(res==1.0);
	test(Math::Pow(res,KNegZeroTReal64,0.0)==KErrNone);
	test(res==1.0);
	test(Math::Pow(res,KPosInfTReal64,2.0)==KErrOverflow);
	test(res==KPosInfTReal64);
	test(Math::Pow(res,0.0,-2.0)==KErrOverflow);
	test(res==KPosInfTReal64);
	test(Math::Pow(res,-2.0,-2.6)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Pow(res,-2.0,4.8)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Pow(res,KNegZeroTReal64,-5)==KErrOverflow);
	test(res==KNegInfTReal64);
	test(Math::Pow(res,KNegZeroTReal64,-6)==KErrOverflow);
	test(res==KPosInfTReal64);
	test(Math::Pow(res,30,999999)==KErrOverflow);	// checking bug fixed
	test(res==KPosInfTReal64);
	test(Math::Pow(res,200,200)==KErrOverflow);
	test(res==KPosInfTReal64);
	test(Math::Pow(res,200,2000)==KErrOverflow);	// checking bug fixed
	test(res==KPosInfTReal64);
	test(Math::Pow(res,1000,1000)==KErrOverflow);
	test(res==KPosInfTReal64);
	test(Math::Pow(res,1000,100)==KErrNone);
	test(testApprox(res,1E+300,3.0E-15));
	test(Math::Pow(res,1000,-1000)==KErrUnderflow);
	test(Math::IsZero(res));
	test(Math::Pow(res,1000,-100)==KErrNone);
	test(testApprox(res,1E-300,4.0E-15));
	
	TInt j;
    TInt i=sizeof(testpow)/sizeof(POWER);
    for (j=0;j<i;j++)
		{
		test(Math::Pow(res,testpow[j].number,testpow[j].power)==KErrNone);
		test(testApprox(res,testpow[j].result,1.0E-15));
		}

	// Added by AnnW, October 1996
	TInt size = sizeof(testpowexact)/sizeof(POWER);
	for (j=0; j<size; j++)
		{
		test(Math::Pow(res,testpowexact[j].number,testpowexact[j].power)==KErrNone);
		test(res==testpowexact[j].result);
		}

	// denormals (base only - do not know results for denormal power)
 	test(Math::Pow(res,K5EMinus324Real64,1.0)==KErrNone);
 	test(res==K5EMinus324Real64);
 	test(Math::Pow(res,K5EMinus324Real64,0.0)==KErrNone);
	test(res==1.0);
	test(Math::Pow(res,2E-160,2.0)==KErrNone);
	test(testApprox(res,K4EMinus320Real64,1.0E-4));		

	// This test is to check that reduce() is working properly
	// This is only a very approximate test due to loss of significance for such nos
	TReal base,power;
	for (TReal powerOfTwo=16.0; powerOfTwo<=54.0; powerOfTwo++)
		{
		Math::Pow(power,2.0,powerOfTwo);
		power+=0.7;
 		Math::Pow(base,2.0,1/power);
		test(Math::Pow(res,base,power)==KErrNone);
		test((2.0-res)<=1.0);
		}
    }

LOCAL_C void powtest2(TReal low,TReal upp)
/*
Test the identity (x**2)**1.5=x**3  on the range low<=x<upp
*/
    //TReal low; // lower limit of range to test
    //TReal upp; // upper limit of range to test 
    {
    
	TReal res,rres,x;

	for (TInt j=0;j<10;j++)
		{
		randrng(x,low,upp);
		TReal y=2;
		test(Math::Pow(res,x,y)==KErrNone);
		TReal xr=res;
		y=1.5;
		test(Math::Pow(res,xr,y)==KErrNone);
		TReal yr=3;
		test(Math::Pow(rres,x,yr)==KErrNone);    
		test(testApprox(rres,res,1.0E-14));
		}
    }

LOCAL_C void powtest3()
/* 
Test the identity x**1=x 
*/
    {
    
	TReal x,res;
 
    TReal low=.5;
    TReal upp=1.0;
    for (TInt j=0;j<10;j++)
		{
		randrng(x,low,upp);
		TReal y=1.0;
		test(Math::Pow(res,x,y)==KErrNone);
		test(testApprox(res,x,1.0E-15));
		}
    }

LOCAL_C void powtest4()
/* 
Test the identity (x**2)**(y/2)=x**y 
*/
    {
    
	TReal res,xr,rres,x,y;
    
    TReal low=.01;
    TReal upp=10.0;
    TReal lowy=-98; // range for y
    TReal uppy=98;
    for (TInt j=0;j<10;j++)
		{
		randrng(x,low,upp);
		randrng(y,lowy,uppy);
		test(Math::Pow(res,x,y)==KErrNone);
		TReal yr=2;
		test(Math::Pow(xr,x,yr)==KErrNone);
		y/=2;
		test(Math::Pow(rres,xr,y)==KErrNone);
		test(testApprox(res,rres,5.0E-14));
		}
    }

LOCAL_C void powtest5()
/* 
Test the identity x**y=1/(x**(-y)) 
*/
    {
    
	TReal x,y;
    TReal res,rres;
    
	test(Math::Pow(res,-2,-3.765)==KErrArgument);
    TReal low=0.5;
    TReal upp=1.0;
    for (TInt j=0;j<10;j++)
		{
		randrng(x,low,upp);
		randrng(y,low,upp);
		test(Math::Pow(res,x,y)==KErrNone);
		y*=(-1);
		test(Math::Pow(rres,x,y)==KErrNone);
		rres=1/rres;
		test(testApprox(res,rres,5.0E-15));
		}
    }

LOCAL_C void powtest6()
/* 
Test specific ISO requirements on Pow()
*/
	{
	TInt i;
	TInt n = sizeof(testpow_iso) / sizeof(POWERISO);
	for (i = 0; i < n; i++)
		{
		TReal ans;
		TInt rc;

		// If one of these tests fails, convert the "failed check xx" number
		// to an index in testpow_iso[] by subtracting 1 and then dividing by 2.
		// If the original number was odd, the first test (rc == xxx) failed.
		// If the original number was even, the second test (.result) failed.
		rc = Math::Pow(ans, testpow_iso[i].number, testpow_iso[i].power);
		test(rc == testpow_iso[i].rc);
		test((rc == KErrArgument) || (ans == testpow_iso[i].result));
		}
	}

LOCAL_C void pow10test()
//
// Test Pow10() for various selected values - results should indicate which string to 
// binary conversions would NOT be expected to be exact - see t_float
//
	{

	TReal res;

	// test errors
	test(Math::Pow10(res,-324)==KErrUnderflow);
	test(res==0.0);
	test(Math::Pow10(res,-400)==KErrUnderflow);
	test(res==0.0);
	test(Math::Pow10(res,309)==KErrOverflow);
	test(res==KPosInfTReal64);
	test(Math::Pow10(res,400)==KErrOverflow);
	test(res==KPosInfTReal64);

	TInt j;
	TInt i=sizeof(pow10teste)/sizeof(POW10_TEST);

	for (j=0; j<i; j++)
		{
		test(Math::Pow10(res,pow10teste[j].num)==KErrNone);
		test(res==pow10teste[j].res);
		}

/*	i=sizeof(pow10testa)/sizeof(POW10_TEST);
	
	for (j=0; j<i; j++)
		{
		test(Math::Pow10(res,pow10testa[j].num)==KErrNone);
		test(testApprox(res,pow10testa[j].res,1.0E-15));
		}
*/	}

LOCAL_C void sintest1(TReal low,TReal upp)
/*
Test the identity sin(x)=sin(x/3)[3-4*(sin(x/3))**2] on the range low<=x<upp
*/
    //TReal low; // lower limit of range to test
    //TReal upp; // upper limit of range to test 
    {
    
	TReal x,res,rres;

    for (TInt j=0;j<100;j++)
		{
		randrng(x,low,upp);
		test(Math::Sin(res,x)==KErrNone);
		x/=3;
		test(Math::Sin(rres,x)==KErrNone);
		TReal err=rres*rres;
		err*=4;
		err=3-err;
		err*=rres;
		test(testApprox(res,err,1.0E-12));
		}
    }

LOCAL_C void sintest2()
/* 
Test selected values (which may not give exact results) 
*/
    {
    
	TReal res;
	
	// test errors
	test(Math::Sin(res,KNaNTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Sin(res,KPosInfTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Sin(res,KNegInfTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Sin(res,2147483648.0*KPi)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Sin(res,-1E+10)==KErrArgument);
	test(Math::IsNaN(res));

	TInt i=sizeof(testsin)/sizeof(TESTSINE);
    TInt j;
    
	for (j=0;j<i;j++)
		{
		TReal x=testsin[j].num;
		TReal y=testsin[j].res;
		test(Math::Sin(res,x)==KErrNone);
   		test(testApprox(res,y,1.0E-15));
		}

	//Added by AnnW, October 1996
	TInt mult=101;
	for (j=-(mult-1); j<mult; j++)
		{
		test(Math::Sin(res, (4*j+1)*KPiBy2)==KErrNone);
		test(testApprox(res,1.0,1.0E-15));

		test(Math::Sin(res, (4*j+3)*KPiBy2)==KErrNone);
		test(testApprox(res,-1.0,1.0E-15));

		test(Math::Sin(res, ((4*j+1)*90)*KDegToRad)==KErrNone);
		test(testApprox(res,1.0,1.0E-15));

		test(Math::Sin(res, ((4*j+3)*90)*KDegToRad)==KErrNone);
		test(testApprox(res,-1.0,1.0E-15));
		}
	//
    }

LOCAL_C void sintest3()
/* 
To test the identity sin(-x)=-sin(x) on the range [0,10*pi] 
*/        
    {
    
	TReal x,res,rres;

    TReal low=0.0;
    TReal upp=10*KPi;
    for (TInt j=0;j<10;j++)
		{
		randrng(x,low,upp);
		test(Math::Sin(res,x)==KErrNone);
		x*=(-1);
		test(Math::Sin(rres,x)==KErrNone);
		test(testApprox(rres,-res,1.0E-15));
		}
    }

LOCAL_C void sintest4()
/* 
To test the identity sin(x)=x for x<<1 
*/        
    {
    
	TReal res,x;
    TReal low=1E-90;
    TReal upp=1E-10;

    for (TInt j=0;j<10;j++)
		{
		randrng(x,low,upp);
		test(Math::Sin(res,x)==KErrNone);
		test(testApprox(res,x,1.0E-15));
		}

	// test some denormals
	test(Math::Sin(res,5E-324)==KErrNone);
	test(testApprox(res,5E-324,1.0E-15));
	test(Math::Sin(res,7E-317)==KErrNone);
	test(testApprox(res,7E-317,1.0E-15));		
    }
/*
LOCAL_C void sintest5()
//
// To test that exact results are given for multiples of pi and
// values sufficiently close to them 
// Added by AnnW, October 1996
//
	{
	
	TReal res;
	TInt j;
	TInt mult=101; // can use up to 32768

    test(Math::Sin(res,KNegZeroTReal64)==KErrNone);
	test(res==0.0);

    for (j=-(mult-1); j<mult; j++)
		{
		test(Math::Sin(res, j*KPi)==KErrNone);
		test(res==0.0);
		test(Math::Sin(res, j*(KPi+1.224E-16))==KErrNone);
		test(res==0.0);
		test(Math::Sin(res, (j*180)*KDegToRad)==KErrNone);
		test(res==0.0);
		if (j!=0)
			{
			test(Math::Sin(res, j*(KPi+1.0E-14))==KErrNone);
			test(res!=0.0);
			}		
		}
	}
*/
LOCAL_C void costest1()
/* 
To test the identity cos(x)=cos(x/3)[4*(cos(x/3)**2)-3] on the interval 
[7*pi,7.5*pi] 
Added by AnnW, October 1996
*/
    {

    TReal x,res,rres;

    TReal low=7*KPi;
    TReal upp=7.5*KPi;
    for (TInt j=0;j<100;j++)
		{
		randrng(x,low,upp);
		test(Math::Cos(res,x)==KErrNone);
		x/=3;
		test(Math::Cos(rres,x)==KErrNone);
		test(testApprox(res,rres*(4*(rres*rres)-3),5.0E-13));
		}
    }

LOCAL_C void costest2()
/*
Test selected values (which may not give exact results) 
Added by AnnW, October 1996
*/
    {
    
	TReal res;

	// test errors
	test(Math::Cos(res,KNaNTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Cos(res,KPosInfTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Cos(res,KNegInfTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Cos(res,(2147483648.0*KPi))==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Sin(res,-1E+10)==KErrArgument);
	test(Math::IsNaN(res));

	TInt j;
	TInt mult=101;
	TInt i=sizeof(testcos)/sizeof(TESTCOSINE);

    for (j=0; j<i; j++)
		{
		test(Math::Cos(res,testcos[j].num)==KErrNone);
		test(testApprox(res,testcos[j].res,1.0E-15));		
		}

	test(Math::Cos(res,KNegZeroTReal64)==KErrNone);
	test(testApprox(res,1.0,1E-15));

    for (j=-(mult-1); j<mult; j++)
		{
		test(Math::Cos(res, (2*j)*KPi)==KErrNone);
		test(testApprox(res,1.0,1.0E-15));		

		test(Math::Cos(res, (2*j+1)*KPi)==KErrNone);
		test(testApprox(res,-1.0,1.0E-15));		

		test(Math::Cos(res, (2*j)*(KPi+1.224E-16))==KErrNone);
		test(testApprox(res,1.0,1.0E-15));		

		test(Math::Cos(res, (2*j+1)*(KPi+1.224E-16))==KErrNone);
		test(testApprox(res,-1.0,1.0E-15));		

		test(Math::Cos(res, ((2*j)*180)*KDegToRad)==KErrNone);
		test(testApprox(res,1.0,1.0E-15));		

		test(Math::Cos(res, ((2*j+1)*180)*KDegToRad)==KErrNone);
		test(testApprox(res,-1.0,1.0E-15));		
		}
    }

LOCAL_C void costest3()
/* 
To test the identity cos(-x)=cos(x) on the range [0,10*pi]
Added by AnnW, October 1996 
*/        
    {

    TReal x,res,rres;

    TReal low=0.0;
    TReal upp=10*KPi;
    for (TInt j=0;j<10;j++)
		{
		randrng(x,low,upp);
		test(Math::Cos(res,x)==KErrNone);
		x*=(-1);
		test(Math::Cos(rres,x)==KErrNone);
		test(testApprox(rres,res,1.0E-15));		
		}
    }

LOCAL_C void costest4()
/* 
To test the identity cos(x)=1 for x<<1 
Added by Annw, October 1996
*/        
    {

    TReal res,x;
    TReal low=1E-90;
    TReal upp=1E-10;

    for (TInt j=0;j<10;j++)
		{
		randrng(x,low,upp);
		test(Math::Cos(res,x)==KErrNone);
		test(testApprox(res,1.0,1.0E-15));
		}

	// test some denormals
	test(Math::Cos(res,5E-324)==KErrNone);
	test(testApprox(res,1.0,1.0E-15));
	test(Math::Cos(res,1.34E-315)==KErrNone);
	test(testApprox(res,1.0,1.0E-15));			
    }
/*
LOCAL_C void costest5()
//
// To test that exact results are given for multiples of KPi and
// values sufficiently close to them 
// Added by AnnW, October 1996
//
	{

	TReal res;
	TInt mult=101;	// can use up to 32768
	TInt j;
    
    for (j=-(mult-1); j<mult; j++)
		{
		test(Math::Cos(res, (2*j+1)*KPiBy2)==KErrNone);
		test(res==0.0);
		test(Math::Cos(res, (2*j+1)*KPiBy2+(j+1)*1.224E-16)==KErrNone);
		test(res==0.0);
		test(Math::Cos(res, (2*j+1)*90*KDegToRad)==KErrNone);
		test(res==0.0);
		if (j!=0)
			{
			test(Math::Sin(res, (2*j+1)*(KPiBy2+1.0E-14))==KErrNone);
			test(res!=0.0);
			}
		}
	}
*/
LOCAL_C void tantest1(TReal low,TReal upp)
/*
Test the identity tan(x)=(2*tan(x/2))/(1-tan(x/2)**2) on the range low<=x<upp
*/
    //TReal low; // lower limit of range to test
    //TReal upp; // upper limit of range to test 
    {

    TReal x,res,rres;

    for (TInt j=0;j<100;j++)
		{
		if (j==90)
			{
			test(1);
			}
		randrng(x,low,upp);
		test(Math::Tan(res,x)==KErrNone);
		x/=2;
		test(Math::Tan(rres,x)==KErrNone);
		TReal ex=(2*rres)/(1-rres*rres);
		test(testApprox(res,ex,1.0E-15));		
		}
    }

LOCAL_C void tantest2()
/* 
To test tangent for specific  arguments 
*/
    {

    TReal res;

	// test errors
	test(Math::Tan(res,KNaNTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Tan(res,KPosInfTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Tan(res,KNegInfTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Tan(res, 1073741824.0*KPi)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Tan(res, 4.0E+102)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Tan(res, -4.0E+102)==KErrArgument);
	test(Math::IsNaN(res));
    
	TInt j;
	TInt mult=101;	// can use up to 32768
    TInt i=sizeof(testtan)/sizeof(TAN);
    for (j=0;j<i;j++)
		{
		test(Math::Tan(res,testtan[j].angle)==KErrNone);
		test(testApprox(res,testtan[j].result,1.0E-15));		
		}

	//Added by AnnW, October 1996
	for (j=-(mult-1); j<mult; j++)
		{
//		test(Math::Tan(res, (2*j+1)*KPiBy2)==KErrOverflow);
//		test(Math::IsInfinite(res));	// this test is no longer valid
		test(Math::Tan(res, (2*j+1)*(KPiBy2+1.0E-15))!=KErrOverflow);
		test(Math::IsFinite(res));
		}
	
	// Check that signs are correct
	test(Math::Tan(res,KPiBy2+5E-16)==KErrNone);
	test(res<0);
	test(Math::Tan(res,KPiBy2-5E-16)==KErrNone);
	test(res>0);
	}

LOCAL_C void tantest3()
/* 
To test the identity tan(-x)=-tan(x) on the range [-1.5,1.5] 
*/        
    {

    TReal x,res,rres;

    TReal low=(-1.5);
    TReal upp=1.5;
    for (TInt j=0;j<10;j++)
		{
		randrng(x,low,upp);
		test(Math::Tan(res,x)==KErrNone);
		x*=(-1);
		test(Math::Tan(rres,x)==KErrNone);
		test(testApprox(rres,-res,1.0E-15));		
		}
    }

LOCAL_C void tantest4()
/* 
To test the identity tan(x)=x for x<<1 
*/        
    {

    TReal x,res;

    TReal low=1E-90;
    TReal upp=1E-10;
    for (TInt j=0;j<10;j++)
		{
		randrng(x,low,upp);
		test(Math::Tan(res,x)==KErrNone);
		test(testApprox(res,x,1.0E-15));		
		}

	// Check some denormals
	test(Math::Tan(res,5E-324)==KErrNone);
	test(res==5E-324);
	test(Math::Tan(res,-1.234567891234E-315)==KErrNone);
	test(res==-1.234567891234E-315);	
    }
/*
LOCAL_C void tantest5()

// To test that exact results are given for multiples of KPi
// Added by AnnW, October 1996

	{

    TReal res;
	TInt j;
	TInt mult=101;	// can use up to 32768

	test(Math::Tan(res,KNegZeroTReal64)==KErrNone);
	test(res==KNegZeroTReal64);
    
    for (j=-(mult-1); j<mult; j++)
		{
		test(Math::Tan(res, j*KPi)==KErrNone);
		test(res==0.0);
		test(Math::Tan(res, j*(KPi+1.224E-16))==KErrNone);
		test(res==0.0);
		test(Math::Tan(res, (j*180)*KDegToRad)==KErrNone);
		test(res==0.0);
		if (j!=0)
			{
			test(Math::Sin(res, j*(KPi+1.0E-14))==KErrNone);
			test(res!=0.0);
			}
		}
	}
*/
LOCAL_C void astest1(TReal low,TReal upp,TInt k,TInt cosflg)
/*
Tests random numbers in the range [low,upp] using the Taylor approximation 
*/
    //TReal low; // lower limit of range to test
    //TReal upp; // upper limit of range to test 
    //TInt k; // Highest order term to be used in the taylor approximation
    //TInt cosflg; // Flag for arc cos
    {

    TReal res,x;

    for (TInt j=0;j<100;j++)
		{
		randrng(x,low,upp);
		if (cosflg)
			test(Math::ACos(res,x)==KErrNone);
		else
			test(Math::ASin(res,x)==KErrNone);
		TReal tres=taylor(x,k);
		if (cosflg)
			tres=KPiBy2-tres;
		test(testApprox(tres,res,5.0E-15));		
		}
    }

LOCAL_C void astest2()
/* 
To test the identity arc sin(x)=x for x<<1 
*/        
    {

    TReal x,res;

    TReal low=1E-90;
    TReal upp=1E-10;
    for (TInt j=0;j<100;j++)
		{
		randrng(x,low,upp);
		test(Math::ASin(res,x)==KErrNone);
		test(testApprox(res,x,1.0E-15));		
		}

	// Check some denormals
	test(Math::ASin(res,5E-324)==KErrNone);
	test(res==5E-324);		
	test(Math::ASin(res,-8.912345678E-318)==KErrNone);
	test(res==-8.912345678E-318);		
    }

LOCAL_C void astest3()
/* 
To test the identity arc sin(-x)=-arc sin(x) 
*/        
    {

    TReal res,rres,x;

    TReal low=0.0;
    TReal upp=1.0;
    for (TInt j=0;j<100;j++)
		{
		randrng(x,low,upp);
		test(Math::ASin(res,x)==KErrNone);
		TReal y=(-x);
		test(Math::ASin(rres,y)==KErrNone);
		test(testApprox(rres,-res,1.0E-15));		
		}
    }

LOCAL_C void astest4(TInt k,TInt sgn)
/* 
Test selected numbers 
*/
    //TInt k; // arc cosine flag
    //TInt sgn; // sign flag for range    
    {

    TReal res;

	// test errors
	test(Math::ASin(res,KNaNTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::ASin(res,KPosInfTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::ASin(res,KNegInfTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::ASin(res,1.0000000000001)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::ASin(res,-1.0000000000001)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::ACos(res,KNaNTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::ACos(res,KPosInfTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::ACos(res,KNegInfTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::ACos(res,1.0000000000001)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::ACos(res,-1.0000000000001)==KErrArgument);
	test(Math::IsNaN(res));

	test(Math::ASin(res,0.0)==KErrNone);
	test(res==0.0);
	test(Math::ASin(res,KNegZeroTReal64)==KErrNone);
	test(res==0.0);

    TInt i=sizeof(testas)/sizeof(TESTASC);
    for (TInt j=0;j<i;j++) 
		{
		// NB Results for comparison only given to 12 or 13 decimal places, so can't expect 
		// better accuracy
		if (k)
			{
			testas[j].num*=sgn;
			testas[j].res*=sgn;
			test(Math::ACos(res,testas[j].num)==KErrNone);
			test(testApprox(res,(KPiBy2-testas[j].res),1.0E-11));	
			}
		else
			{
			test(Math::ASin(res,testas[j].num)==KErrNone);
			test(testApprox(res,testas[j].res,1.0E-12));	
			}
		}

	// Check some denormals for ACos()
	test(Math::ACos(res,5E-324)==KErrNone);
	test(res==KPiBy2);	
	test(Math::ACos(res,-9.87654E-320)==KErrNone);
	test(res==KPiBy2);						
    }

LOCAL_C void attest1()
/* 
Random argument tests for x in the primary range, comparing the result with a 
Taylor series approximation
*/
    {

    TReal res,x;

    TReal low=(-0.0625);
    TReal upp=0.0625;
    for (TInt i=0;i<10;i++)
		{
		randrng(x,low,upp);
		test(Math::ATan(res,x)==KErrNone);
		TReal tres=tayatan(x);
		test(testApprox(res,tres,1.0E-15));		
		}
    }

LOCAL_C void attest2()
/* 
Random argument tests for x outside the primary range, using the identity
arctan(u)=arctan(v)+arctan[(u-v)/(1+uv)]
*/
    {

    TReal x,res,rres,atcnst;

    TReal low=0.0625;
    TReal upp=2.0-KSqt3;
    TReal cnst=0.0625;
    test(Math::ATan(atcnst,cnst)==KErrNone);
    for (TInt i=0;i<10;i++)
		{
		randrng(x,low,upp);
		test(Math::ATan(res,x)==KErrNone);
		TReal y=(x-cnst)/(1+x*cnst);
		test(Math::ATan(rres,y)==KErrNone);
		test(testApprox(res,(atcnst+rres),1.0E-15));		
		}
    }                   

LOCAL_C void attest3()
/*
Check that the identity arctan(-x)=-arctan(x) holds
*/
    {

    TReal res,rres,x;
    TReal low=0.0;
    TReal upp=1.0;
    for (TInt i=0;i<10;i++)
		{
		randrng(x,upp,low);
		test(Math::ATan(res,x)==KErrNone);
		x=(-x);
		test(Math::ATan(rres,x)==KErrNone);
		test(testApprox(res,-rres,1.0E-15));		
		}
    }           

LOCAL_C void attest4()
/* 
Check that the identity arctan(x)=x for Abs(x)<1 holds
*/
    {

    TReal x,res;

    TReal low=1E-90;
    TReal upp=1E-20;
    for (TInt i=0;i<10;i++)
		{
		randrng(x,low,upp);
		test(Math::ATan(res,x)==KErrNone);
		test(testApprox(res,x,1.0E-15));		
		}

	// Check some denormals
	test(Math::ATan(res,-5E-324)==KErrNone);
	test(res==-5E-324);		
	test(Math::ATan(res,7.123E-322)==KErrNone);
	test(res==7.123E-322);			
    }

LOCAL_C void attest5()
/*
Tests selected values
*/
    {

    TReal res;

	// test errors, special cases
	test(Math::ATan(res,KNaNTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::ATan(res,0.0)==KErrNone);
	test(res==0.0);
	test(Math::ATan(res,KNegZeroTReal64)==KErrNone);
	test(res==0.0);
	test(Math::ATan(res,KPosInfTReal64)==KErrNone);
	test(res==KPiBy2);
	test(Math::ATan(res,KNegInfTReal64)==KErrNone);
	test(res==-KPiBy2);

	test(Math::ATan(res,KNaNTReal64,1.0)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::ATan(res,1.0,KNaNTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::ATan(res,KNaNTReal64,KNaNTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::ATan(res,0.0,KNegZeroTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::ATan(res,KNegZeroTReal64,KNegZeroTReal64)==KErrArgument);
	test(Math::IsNaN(res));	
	test(Math::ATan(res,0.0,0.0)==KErrArgument);
	test(Math::IsNaN(res));	
	test(Math::ATan(res,KNegZeroTReal64,KNegZeroTReal64)==KErrArgument);
	test(Math::IsNaN(res));	
	test(Math::ATan(res,KPosInfTReal64,KNegInfTReal64)==KErrNone);
	test(res==3.0*(KPiBy2/2.0));
	test(Math::ATan(res,KPosInfTReal64,KPosInfTReal64)==KErrNone);
	test(res==KPiBy2/2.0);
	test(Math::ATan(res,KNegInfTReal64,KPosInfTReal64)==KErrNone);
	test(res==-(KPiBy2/2.0));
	test(Math::ATan(res,KNegInfTReal64,KNegInfTReal64)==KErrNone);
	test(res==-3.0*(KPiBy2/2.0));
	test(Math::ATan(res,KNegZeroTReal64,1.0)==KErrNone);
	test(res==0.0);
	test(Math::ATan(res,0.0,1.0)==KErrNone);
	test(res==0.0);	
	test(Math::ATan(res,0.0,-1.0)==KErrNone);
	test(res==KPi);	
	test(Math::ATan(res,1.0,KPosInfTReal64)==KErrNone);
	test(res==0.0);
	test(Math::ATan(res,1.0,KNegInfTReal64)==KErrNone);
	test(res==KPi);
	test(Math::ATan(res,0.0,KPosInfTReal64)==KErrNone);	
	test(res==0.0);
	test(Math::ATan(res,KPosInfTReal64,1.0)==KErrNone);	
	test(res==KPiBy2);
	test(Math::ATan(res,KNegInfTReal64,1.0)==KErrNone);	
	test(res==-KPiBy2);
	test(Math::ATan(res,1.0,0.0)==KErrNone);	
	test(res==KPiBy2);
	test(Math::ATan(res,1.0,KNegZeroTReal64)==KErrNone);	
	test(res==KPiBy2);
	test(Math::ATan(res,KPosInfTReal64,-1.0)==KErrNone);	
	test(res==KPiBy2);
	test(Math::ATan(res,KNegInfTReal64,-1.0)==KErrNone);	
	test(res==-KPiBy2);
	test(Math::ATan(res,-1.0,0.0)==KErrNone);	
	test(res==-KPiBy2);
	test(Math::ATan(res,-1.0,KNegZeroTReal64)==KErrNone);	
	test(res==-KPiBy2);
	test(Math::ATan(res,5E-324,10)==KErrNone);	
	test(res==0.0);
	test(Math::ATan(res,1E+308,0.1)==KErrNone);	
	test(res==KPiBy2);

    TInt i=sizeof(testat2)/sizeof(TESTATAN2);
    for (TInt j=0;j<i;j++) 
		{
		// NB Some results only given to 12 dp so cannot expect better accuracy
		test(Math::ATan(res,testat2[j].num1,testat2[j].num2)==KErrNone);
		test(testApprox(res,testat2[j].res,1.0E-12));		
		}	
    }

LOCAL_C void inttest1()
/*
Tests specific numbers
*/
    {

    TReal res;

	// Specials
	test(Math::Int(res,KNaNTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Int(res,KPosInfTReal64)==KErrOverflow);
	test(res==KPosInfTReal64);
	test(Math::Int(res,KNegInfTReal64)==KErrOverflow);
	test(res==KNegInfTReal64);

    TInt i=sizeof(testint1)/sizeof(INT_TEST);
    for (TInt j=0;j<i;j++) 
		{
		test(Math::Int(res,testint1[j].num)==KErrNone);
		test(res==testint1[j].res);
		}

	// Check some denormals
	test(Math::Int(res,5E-324)==KErrNone);
	test(res==0.0);
	test(Math::Int(res,1.45E-309)==KErrNone);
	test(res==0.0);
    }

LOCAL_C void inttest2()
/*
Tests specific numbers
*/
    {

    TInt16 res;

	// test errors
	test(Math::Int(res,KNaNTReal64)==KErrArgument);
	test(res==0);
	test(Math::Int(res,KPosInfTReal64)==KErrOverflow);
	test(res==TInt16(KMaxTInt16));	
	test(Math::Int(res,32768.9830857)==KErrOverflow);
	test(res==TInt16(KMaxTInt16));
	test(Math::Int(res,32769.36946)==KErrOverflow);
	test(res==TInt16(KMaxTInt16));
	test(Math::Int(res,KNegInfTReal64)==KErrUnderflow);
    test(res==TInt16(KMinTInt16));
	test(Math::Int(res,-32774.997937)==KErrUnderflow);
    test(res==TInt16(KMinTInt16));

    TInt i=sizeof(testint2)/sizeof(INTI_TEST);
    for (TInt j=0;j<i;j++) 
		{
		test(Math::Int(res,testint2[j].num)==KErrNone);
		test(res==testint2[j].res);		
		}

	// Check some denormals
	test(Math::Int(res,5E-324)==KErrNone);
	test(res==0.0);
	test(Math::Int(res,1.45E-309)==KErrNone);
	test(res==0.0);
	}

LOCAL_C void inttest3()
/*
Tests specific numbers
*/
    {

    TInt32 res;

    // test errors
	test(Math::Int(res,KNaNTReal64)==KErrArgument);
	test(res==0);
	test(Math::Int(res,KPosInfTReal64)==KErrOverflow);
	test(res==KMaxTInt32);
	test(Math::Int(res,2147483648.34576)==KErrOverflow);
	test(res==KMaxTInt32);
    test(Math::Int(res,2147553576.8794365)==KErrOverflow);
	test(res==KMaxTInt32);
    test(Math::Int(res,KNegInfTReal64)==KErrUnderflow);
	test(res==KMinTInt32);
	test(Math::Int(res,-2147496757.583)==KErrUnderflow);
	test(res==KMinTInt32);
    
	TInt i=sizeof(testint3)/sizeof(INTL_TEST);
    for (TInt j=0;j<i;j++) 
		{
		test(Math::Int(res,testint3[j].num)==KErrNone);
		test(res==testint3[j].res);
		}

	// Check some denormals
	test(Math::Int(res,5E-324)==KErrNone);
	test(res==0.0);
	test(Math::Int(res,1.45E-309)==KErrNone);
	test(res==0.0);
	}

LOCAL_C void inttest4()
	{
	// tests Int()
	TInt16 tint16;
	TInt32 tint32;
	TReal trg,src=100.0;

	test.Start(_L("Math::Int()"));
	src=0.0;
	test(Math::Int(trg,src)==KErrNone);  
	test(trg==0.0);
	test(Math::Int(tint16,src)==KErrNone);
	test(tint16==0);
	test(Math::Int(tint32,src)==KErrNone);
	test(tint32==0);

    src=0.1233456789;
	test(Math::Int(trg,src)==KErrNone);  
	test(trg==0.0);
	test(Math::Int(tint16,src)==KErrNone);
	test(tint16==0);
	test(Math::Int(tint32,src)==KErrNone);
	test(tint32==0);

	src=-0.5;
	test(Math::Int(trg,src)==KErrNone);  
	test(trg==0.0);
	test(Math::Int(tint16,src)==KErrNone);
	test(tint16==0);
	test(Math::Int(tint32,src)==KErrNone);
	test(tint32==0);

	src=1.123456789;
	test(Math::Int(trg,src)==KErrNone);  
	test(trg==1.0);
	test(Math::Int(tint16,src)==KErrNone);
	test(tint16==1);
	test(Math::Int(tint32,src)==KErrNone);
	test(tint32==1);

	src=-1.12345678;
	test(Math::Int(trg,src)==KErrNone);  
	test(trg==-1.0);
	test(Math::Int(tint16,src)==KErrNone);
	test(tint16==-1);
	test(Math::Int(tint32,src)==KErrNone);
	test(tint32==-1);

	src=KMaxTInt16-0.1; 
	test(Math::Int(trg,src)==KErrNone);  
	test(trg==KMaxTInt16-1);
	test(Math::Int(tint16,src)==KErrNone);
	test(tint16==KMaxTInt16-1);
	test(Math::Int(tint32,src)==KErrNone);
	test(tint32==KMaxTInt16-1);

	src=KMaxTInt16+0.5; 
	test(Math::Int(trg,src)==KErrNone);  
	test(trg==KMaxTInt16);
	test(Math::Int(tint16,src)==KErrNone);
	test(tint16==KMaxTInt16);
	test(Math::Int(tint32,src)==KErrNone);
	test(tint32==KMaxTInt16);

	src=KMaxTInt16+1; 
	test(Math::Int(trg,src)==KErrNone);  
	test(trg==KMaxTInt16+1);
	test(Math::Int(tint16,src)==KErrOverflow);
	test(Math::Int(tint32,src)==KErrNone);
	test(tint32==KMaxTInt16+1);

	src=KMinTInt16-0.1; 
	test(Math::Int(trg,src)==KErrNone);  
	test(trg==KMinTInt16);
	test(Math::Int(tint16,src)==KErrNone);
	test(tint16==KMinTInt16);
	test(Math::Int(tint32,src)==KErrNone);
	test(tint32==KMinTInt16);

	src=KMinTInt16; 
	test(Math::Int(trg,src)==KErrNone);  
	test(trg==KMinTInt16);
	test(Math::Int(tint16,src)==KErrNone);
	test(tint16==KMinTInt16);
	test(Math::Int(tint32,src)==KErrNone);
	test(tint32==KMinTInt16);

	src=KMinTInt16-1; 
	test(Math::Int(trg,src)==KErrNone);  
	test(trg==KMinTInt16-1);
	test(Math::Int(tint16,src)==KErrUnderflow);
	test(Math::Int(tint32,src)==KErrNone);
	test(tint32==KMinTInt16-1);

	src=KMaxTInt32-0.1; 
	test(Math::Int(trg,src)==KErrNone);  
	test(trg==KMaxTInt32-1);
	test(Math::Int(tint16,src)==KErrOverflow);
	test(Math::Int(tint32,src)==KErrNone);
	test(tint32==KMaxTInt32-1);

	src=KMaxTInt32+0.5; 
	test(Math::Int(trg,src)==KErrNone);  
	test(trg==KMaxTInt32);
	test(Math::Int(tint16,src)==KErrOverflow);
	test(Math::Int(tint32,src)==KErrNone);
	test(tint32==KMaxTInt32);

	src=KMaxTInt32; 
	src+=1;
	test(Math::Int(trg,src)==KErrNone);  
	test(trg==(TUint32)KMaxTInt32+1);
	test(Math::Int(tint16,src)==KErrOverflow);
	test(Math::Int(tint32,src)==KErrOverflow);

	src=KMinTInt32+0.1; 
	test(Math::Int(trg,src)==KErrNone);  
	test(trg==KMinTInt32+1);
	test(Math::Int(tint16,src)==KErrUnderflow);
	test(Math::Int(tint32,src)==KErrNone);
	test(tint32==KMinTInt32+1);

	src=KMinTInt32; 
	test(Math::Int(trg,src)==KErrNone);  
	test(trg==KMinTInt32);
	test(Math::Int(tint16,src)==KErrUnderflow);
	test(Math::Int(tint32,src)==KErrNone);
	test(tint32==KMinTInt32);

	src=KMinTInt32;
	src-=1; 
	test(Math::Int(trg,src)==KErrNone);  
	test((trg+1)==KMinTInt32);
	test(Math::Int(tint16,src)==KErrUnderflow);
	test(Math::Int(tint32,src)==KErrUnderflow);

	src=KMaxTUint32-0.1;
	test(Math::Int(trg,src)==KErrNone);  
	test(trg==KMaxTUint32-1);
	test(Math::Int(tint16,src)==KErrOverflow);
	test(Math::Int(tint32,src)==KErrOverflow);

	src=KMaxTUint32;
	test(Math::Int(trg,src)==KErrNone);  
	test(trg==KMaxTUint32);
	test(Math::Int(tint16,src)==KErrOverflow);
	test(Math::Int(tint32,src)==KErrOverflow);

	test.End();
	}

LOCAL_C void fractest1()
/*
Tests specific numbers
*/
    {

    TReal res;

	// test errors
	test(Math::Frac(res,KNaNTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Frac(res,KPosInfTReal64)==KErrOverflow);
	test(res==0.0);
	test(Math::Frac(res,KNegInfTReal64)==KErrOverflow);
	test(res==0.0);

    TInt i=sizeof(testfrac)/sizeof(FRAC_TEST);
    for (TInt j=0;j<i;j++) 
		{
		test(Math::Frac(res,testfrac[j].num)==KErrNone);
		TReal err=(res-testfrac[j].res);
		if (res)
			err/=testfrac[j].num;	// NB num not res
		test(Abs(err)<1.0E-15);
		}

	// Check some denormals
	test(Math::Frac(res,5E-324)==KErrNone);
	test(res==5E-324);
	test(Math::Frac(res,1.23456789E-314)==KErrNone);
	test(res==1.23456789E-314);
    }

LOCAL_C void fractest2()
	{
	// tests Frac() 
	test.Start(_L("Math::Frac()"));
	TReal trg,src;

	src=0.0;
	test(Math::Frac(trg,src)==KErrNone);
	test(trg==0.0);

	src=0.1;
	test(Math::Frac(trg,src)==KErrNone);
	test(trg==0.1);

	src=-0.1;
	test(Math::Frac(trg,src)==KErrNone);
	test(trg==-0.1);

	src=7.5;
	test(Math::Frac(trg,src)==KErrNone);
	test(trg==0.5);

	src=-7.5;
	test(Math::Frac(trg,src)==KErrNone);
	test(trg==-0.5);

	src=5.998046875;
	test(Math::Frac(trg,src)==KErrNone);
	test(trg==0.998046875);

	src=-5.998046875;
	test(Math::Frac(trg,src)==KErrNone);
	test(trg==-0.998046875);

	src=-0.00000000001;
	test(Math::Frac(trg,src)==KErrNone);
	test(trg==-0.00000000001);

	src=1000000000000.5;
	test(Math::Frac(trg,src)==KErrNone);
	test(trg==0.5);

	src=1099511627776.0;
	src+=0.000244140625;
	test(Math::Frac(trg,src)==KErrNone);
	test(trg==0.000244140625);

	src=-KMaxTInt32;
	src+=0.5;
	test(Math::Frac(trg,src)==KErrNone);
	test(trg==-0.5);

	src=KMaxTUint32;
	src+=0.5;
	test(Math::Frac(trg,src)==KErrNone);
	test(trg==0.5);

	test.End();
	}

LOCAL_C void modtest1()
/*
Test modulo function using specified values
*/
    {

    TReal res;

	// test errors
	test(Math::Mod(res,KNaNTReal64,1.0)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Mod(res,1.0,KNaNTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Mod(res,KNaNTReal64,KNaNTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Mod(res,KPosInfTReal64,2.0)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Mod(res,KNegInfTReal64,2.0)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Mod(res,2.0,KNegZeroTReal64)==KErrArgument);
	test(Math::IsNaN(res));
	test(Math::Mod(res,1.0,0.0)==KErrArgument);
	test(Math::IsNaN(res));

    TInt i=sizeof(testmod)/sizeof(MOD_TEST);
    for (TInt j=0;j<i;j++) 
		{
		test(Math::Mod(res,testmod[j].num,testmod[j].mod)==KErrNone);
		test(testApprox(res,testmod[j].res,5.0E-13));		
		}

	// Check some denormals
 	test(Math::Mod(res,K1Point2EMinus320Real64,K5EMinus321Real64)==KErrNone);
 	test(res==K2EMinus321Real64);		
 	test(Math::Mod(res,K1Point234EMinus316Real64,K1Point234EMinus316Real64)==KErrNone);
	test(res==0.0);		
    }

LOCAL_C void modtest2()
/*
Test modulo function for values which will be incorrect so return KErrTotalLossOfPrecision
*/
    {

    TReal res;

	TInt i=sizeof(testmod2)/sizeof(MOD_TEST);
    for (TInt j=0;j<i;j++) 
		{
		test(Math::Mod(res,testmod2[j].num,testmod2[j].mod)==KErrTotalLossOfPrecision);
		test(Math::IsZero(res));
		}
	}

LOCAL_C void DuplicateTest()
//
// Tests that you can use the same variable for both operands in some Math functions
// NB results only given to 12 or 13 significant figures so cannot expect better accuracy
//
	{

	TReal inOut;
	test.Start(_L("ACos"));
	inOut=-0.5;
	test(Math::ACos(inOut,inOut)==KErrNone);
	test(testApprox(inOut,2.094395102393,1.0E-13));			

	test.Next(_L("ASin"));
	inOut=-0.5;
	test(Math::ASin(inOut,inOut)==KErrNone);
	test(testApprox(inOut,-0.523598775598,6.0E-13));			

	test.Next(_L("ATan"));
	inOut=0.5;
	test(Math::ATan(inOut,inOut)==KErrNone);
	test(testApprox(inOut,0.463647609001,5.0E-13));			
	inOut=-0.25;
	TReal another=-0.5;
	test(Math::ATan(inOut,inOut,another)==KErrNone);
	test(testApprox(inOut,-2.677945044589,5.0E-15));			
	inOut=-0.5;
	another=0.25;
	test(Math::ATan(inOut,another,inOut)==KErrNone);
	test(testApprox(inOut,2.677945044589,5.0E-15));			

	test.Next(_L("Cos"));
	inOut=1;
	test(Math::Cos(inOut,inOut)==KErrNone);
	test(testApprox(inOut,0.540302305868,3.0E-13));			

	test.Next(_L("Exp"));
	inOut=0.5;
	test(Math::Exp(inOut,inOut)==KErrNone);
	test(testApprox(inOut,1.648721270700,1.0E-13));			

	test.Next(_L("Frac"));
	inOut=56.123456789;
	test(Math::Frac(inOut,inOut)==KErrNone);
	test(testApprox(inOut,0.123456789,2.0E-14));

	test.Next(_L("Int"));
	inOut=56.123456789;
	test(Math::Int(inOut,inOut)==KErrNone);
	test(inOut==56);
	
	test.Next(_L("Log"));
	inOut=0.5;
	test(Math::Log(inOut,inOut)==KErrNone);
	test(testApprox(inOut,-0.301029995664,7.0E-14));				

	test.Next(_L("Ln"));
	inOut=0.5;
	test(Math::Ln(inOut,inOut)==KErrNone);
	test(testApprox(inOut,-0.693147180560,8.0E-14));				

	test.Next(_L("Mod"));
	inOut=53;
	another=17;
	test(Math::Mod(inOut,inOut,another)==KErrNone);
	test(inOut==2);
	inOut=17;
	another=53;
	test(Math::Mod(inOut,another,inOut)==KErrNone);
	test(inOut==2);

	test.Next(_L("Pow"));
	inOut=-5;
	another=3;
	test(Math::Pow(inOut,inOut,another)==KErrNone);
	test(inOut==-125.0);
	another=-5;
	inOut=3;
	test(Math::Pow(inOut,another,inOut)==KErrNone);
	test(inOut==-125.0);

	test.Next(_L("Sin"));
	inOut=1;
	test(Math::Sin(inOut,inOut)==KErrNone);
	test(testApprox(inOut,0.84147098480790,5.0E-15));				

	test.Next(_L("Round"));
	inOut=123.4567;
	test(Math::Round(inOut,inOut,2)==KErrNone);
	test(testApprox(inOut,123.46,1.0E-15));				

	test.Next(_L("Sqrt"));
	inOut=53;
	test(Math::Sqrt(inOut,inOut)==KErrNone);
	test(testApprox(inOut,7.280109889281,7.0E-14));				

	test.Next(_L("Tan"));
	inOut=1;
	test(Math::Tan(inOut,inOut)==KErrNone);
	test(testApprox(inOut,1.557407724655,7.0E-14));				

	test.End();
	}

LOCAL_C void specialtest()
//
// Tests functions which test for specials
// 
	{

	test(Math::IsZero(0.0));
	test(Math::IsZero(KNegZeroTReal64));
	test(Math::IsZero(0.0));
	test(!Math::IsZero(1.0));
	test(!Math::IsZero(KPosInfTReal64));
	test(!Math::IsZero(KNaNTReal64));
 	test(!Math::IsZero(K5EMinus324Real64));

	test(Math::IsNaN(KNaNTReal64));
	test(!Math::IsNaN(KPosInfTReal64));
	test(!Math::IsNaN(KNegInfTReal64));
	test(!Math::IsNaN(0.0));
	test(!Math::IsNaN(1.0));

	test(Math::IsInfinite(KPosInfTReal64));
	test(Math::IsInfinite(KNegInfTReal64));
	test(!Math::IsInfinite(KNaNTReal64));
	test(!Math::IsInfinite(0.0));
	test(!Math::IsInfinite(KMaxTReal64));

	test(!Math::IsFinite(KPosInfTReal64));
	test(!Math::IsFinite(KNegInfTReal64));
	test(!Math::IsFinite(KNaNTReal64));
	test(Math::IsFinite(0.0));
	test(Math::IsFinite(KMaxTReal64));
	test(Math::IsFinite(5E-324));	
	test(Math::IsFinite(1.0));
	}

void _matherr(TExcType aType)
//
// Dummy function to handle exceptions
//
	{

	test.Printf(_L("_matherr: Exception type %u handled\n"),TUint(aType));
	}

#ifdef __GCC32__
#define FSTCW(x) asm("mov eax, %0\nfstcw [eax]": : "i"(&x))
#define FLDCW(x) asm("mov eax, %0\nfldcw [eax]": : "i"(&x))
#else
#define FSTCW(x) _asm fstcw x
#define FLDCW(x) _asm fldcw x
#endif
TInt16 cw=0; // must be global or GCC/GAS can't get the address!

GLDEF_C TInt E32Main()
    {     

#if defined (__X86__)
	FSTCW(cw);
	test.Printf(_L("control word = 0x%x\n"),cw);
	cw=0x27f;	// WINS value
	FLDCW(cw);
#endif

	test.Title();

	test.Start(_L("Assorted tests"));
	AssortedTests();

	test.Next(_L("sqrtest1(KSqhf,1.0)"));
    sqrtest1(KSqhf,1.0);
	test.Next(_L("sqrtest1(1.0,1.41421356238)"));
    sqrtest1(1.0,1.41421356238);
	test.Next(_L("sqrtest2"));
    sqrtest2();                  

	test.Next(_L("logtest"));
    logtest();
	test.Next(_L("lntest1"));
    lntest1();
	test.Next(_L("lntest2"));
    lntest2();
	test.Next(_L("lntest3"));
    lntest3();
	test.Next(_L("lntest4"));
    lntest4();

	test.Next(_L("exptest1"));
    exptest1();
	test.Next(_L("exptest2(-0.0625,-.9375,1.0625)"));
    exptest2(-0.0625,-0.9375,1.0625);
	test.Next(_L("exptest2(-29.0/16.0),1.0,88.0)"));
    exptest2((-29.0/16.0),1.0,88.0);
	test.Next(_L("exptest2(-29.0/16.0),-1.0,-88.0)"));
    exptest2((-29.0/16.0),-1.0,-88.0);
	test.Next(_L("exptest3"));
    exptest3();

	test.Next(_L("powtest1"));
    powtest1();
	test.Next(_L("powtest2(.5,1.0)"));
    powtest2(.5,1.0);
	test.Next(_L("powtest2(1.0,1.0E33)"));
    powtest2(1.0,1.0E33);
	test.Next(_L("powtest3"));
    powtest3();
	test.Next(_L("powtest4"));
    powtest4();
	test.Next(_L("powtest5"));
    powtest5();
	test.Next(_L("powtest6"));
    powtest6();
	
	test.Next(_L("pow10test"));
	pow10test();
														
	test.Next(_L("sintest1(3*KPi,3.5*KPi)"));
    sintest1(3*KPi,3.5*KPi);
	test.Next(_L("sintest1(3*KPi,3.5*KPi)"));
    sintest1(6*KPi,6.5*KPi);
	test.Next(_L("sintest2"));
    sintest2();
	test.Next(_L("sintest3"));    
	sintest3();
	test.Next(_L("sintest4"));
    sintest4();
//	test.Next(_L("sintest5"));		// this test is no longer valid
//	sintest5();

	test.Next(_L("costest1"));
	costest1();
	test.Next(_L("costest2"));
	costest2();
	test.Next(_L("costest3"));
	costest3();
	test.Next(_L("costest4"));
	costest4();
//	test.Next(_L("costest5"));		// this test is no longer valid
//	costest5();

	test.Next(_L("tantest1(-.25*KPi,.25*KPi)"));                                            
    tantest1(-.25*KPi,.25*KPi);
	test.Next(_L("tantest1(.875*KPi,1.125*KPi)"));
    tantest1(.875*KPi,1.125*KPi);
	test.Next(_L("tantest1(6*KPi,6.25*KPi)"));
    tantest1(6*KPi,6.25*KPi);
	test.Next(_L("tantest2"));
    tantest2();   
	test.Next(_L("tantest3"));
    tantest3();
	test.Next(_L("tantest4"));
    tantest4();
//	test.Next(_L("tantest5"));		// this test is no longer valid
//	tantest5();

	test.Next(_L("astest1(-.125,0.125,15,0)"));
    astest1(-.125,0.125,15,0);
	test.Next(_L("astest1(-.125,0.125,15,1)"));
    astest1(-.125,0.125,15,1);
	test.Next(_L("astest2"));
    astest2();
	test.Next(_L("astest3"));
    astest3();
	test.Next(_L("astest4(0,1)"));
    astest4(0,1);
	test.Next(_L("astest4(1,1)"));
    astest4(1,1);
	test.Next(_L("astest4(1,-1)"));
    astest4(1,-1);
		  
	test.Next(_L("attest1"));
    attest1();
	test.Next(_L("attest2"));
    attest2();
	test.Next(_L("attest3"));
    attest3();
	test.Next(_L("attest4"));
    attest4();
	test.Next(_L("attest5"));
    attest5();

    test.Next(_L("inttest1"));
    inttest1();	
	test.Next(_L("intitest2"));
    inttest2();	
	test.Next(_L("inttest3"));
    inttest3();	
	test.Next(_L("inttest4"));
	inttest4();	

	test.Next(_L("fractest1"));
    fractest1();	
	test.Next(_L("fractest2"));
	fractest2();

	test.Next(_L("modtest1"));
    modtest1();
	test.Next(_L("modtest2"));
    modtest2();

	test.Next(_L("Test duplicate parameters"));
	DuplicateTest();

	test.Next(_L("Test Math::Is...() functions"));
	specialtest();

	test.End();
	return(KErrNone);
    }

