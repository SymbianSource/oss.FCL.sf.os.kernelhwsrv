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
// e32\include\e32math.h
// 
//

#ifndef __E32MATH_H__
#define __E32MATH_H__
#include <e32std.h>


/**
@publishedAll
@released
*/
const TInt KMaxPrecision=15;

/**
@publishedAll
@released

This constant specifies the maximum number of significant digits available with floating 
point computations. Rounding and string formatting methods will not use more digits than this.
*/
const TInt KPrecisionLimit=12;

/**
@publishedAll
@released

Let D be the set of real numbers exactly representable by an IEEE-754 'double'
For any positive integer n let X_n be the set of real numbers with an exact
decimal representation using n significant digits.
Let r_n : D -> X_n be defined by r_n(x)=y such that
|y-x| = inf { |z-x| : z in X_n }
and (in the case where two such y exist) that the last significant digit in the
decimal representation of y is even.
This constant is the least n such that r_n is injective.
*/
const TInt KIEEEDoubleInjectivePrecision=17;

/**
@publishedAll
@released
*/
const TInt KMantissaBits=53;
/**
@publishedAll
@released
*/
const TInt KMaxExponent=1023;
/**
@publishedAll
@released
*/
const TInt KExponentBias=1022;
/**
@publishedAll
@released
*/
const TInt KSpecialExponent=2047;
//


/**
@publishedAll
@released

The maximum exponent for a 32-bit floating point number.
*/
const TInt KTReal32MaxExponent=128;			// changed from 127


/**
@publishedAll
@released

The minimum exponent for a 32-bit floating point number.
*/
const TInt KTReal32MinExponent=-125;
/**
@publishedAll
@released
*/
const TInt KTReal32ExponentBias=126;
/**
@publishedAll
@released
*/
const TInt KTReal32SpecialExponent=255;		// changed from KTReal32ExponentBad


/**
@publishedAll
@released

A zero exponent value for a 32-bit floating point number. 
*/
const TInt KTReal32ZeroExponent=0;
//


/**
@publishedAll
@released

The maximum exponent for a 64-bit floating point number.
*/
const TInt KTReal64MaxExponent=1024;		// changed from 1023


/**
@publishedAll
@released

The minimum exponent for a 64-bit floating point number.
*/
const TInt KTReal64MinExponent=-1021;


/**
@publishedAll
@released
*/
const TInt KTReal64ExponentBias=1022;


/**
@publishedAll
@released
*/
const TInt KTReal64SpecialExponent=2047;	// changed from KTReal64BadExponent


/**
@publishedAll
@released

A zero exponent value for a 64-bit floating point number. 
*/
const TInt KTReal64ZeroExponent=0;
//


/**
@publishedAll
@released

The minimum value of a 64-bit floating point number.
*/
const TReal KMinTReal=2.2250738585072015E-308;	// changed from TReal64


/**
@publishedAll
@released

The maximum value of a 64-bit floating point number.
*/
const TReal KMaxTReal=1.7976931348623157E+308;	//
//


/**
@publishedAll
@released

The minimum value of a 32-bit floating point number.
*/
const TReal32 KMinTReal32=1.17549435E-38f;


/**
@publishedAll
@released

The maximum value of a 32-bit floating point number.
*/
const TReal32 KMaxTReal32=3.4028234663852885981170418348452e+38f;
//


/**
@publishedAll
@released

The minimum value of a 64-bit floating point number.
*/
const TReal64 KMinTReal64=2.2250738585072015E-308;


/**
@publishedAll
@released

The maximum value of a 64-bit floating point number.
*/
const TReal64 KMaxTReal64=1.7976931348623157E+308;
//


/**
@publishedAll
@released
*/
const TReal KSqhf=0.70710678118654752440;


/**
@publishedAll
@released

Log 2 to the base "e".
*/
const TReal KRln2=1.4426950408889634;


/**
@publishedAll
@released

Log 10 to the base "e".
*/
const TReal KRln10=0.4342944819032518;


/**
@publishedAll
@released

Log 2 to the base 10.
*/
const TReal KRlg2=0.3010299956639812;


/**
@publishedAll
@released

The mathematical constant Pi.
*/
const TReal KPi=3.1415926535897932;


/**
@publishedAll
@released

The reciprocal of the mathematical constant Pi. 
*/
const TReal KPiInv=0.3183098861837907;


/**
@publishedAll
@released

The mathematical constant Pi divided by 2.
*/
const TReal KPiBy2=1.5707963267948966;


/**
@publishedAll
@released

Not used.
*/
const TReal KDrpi=0.6366197723675813;


/**
@publishedAll
@released

The square root of 3.
*/
const TReal KSqt3=1.7320508075688773;


/**
@publishedAll
@released
*/
const TReal KMsq3=0.2679491924311227;


/**
@publishedAll
@released

The multiplying factor to convert radians to degrees.
*/
const TReal KRadToDeg=57.29577951308232;


/**
@publishedAll
@released

The multiplying factor to convert degrees to radians.
*/
const TReal KDegToRad=0.017453292519943296;




class TRealX
/**
@publishedAll
@released

A class encapsulating an extended precision real value.

This class provides 64 bit precision and a dynamic range of approximately
1E-9863 to 1E+9863. All member functions are optimized for speed.
*/
	{
public:
	enum TRealXOrder {ELessThan=1,EEqual=2,EGreaterThan=4,EUnordered=8};
public:
	IMPORT_C TRealX();
	IMPORT_C TRealX(TInt aInt);
	IMPORT_C TRealX(TUint aInt);
	IMPORT_C TRealX(TUint aExp, TUint aMantHi, TUint aMantLo);
	IMPORT_C TRealX(const TInt64 &aInt);
	IMPORT_C TRealX(TReal32 aReal) __SOFTFP;
	IMPORT_C TRealX(TReal64 aReal) __SOFTFP;
	IMPORT_C TRealX &operator=(TInt aInt);
	IMPORT_C TRealX &operator=(TUint aInt);
	IMPORT_C TRealX &operator=(const TInt64& aInt);
	IMPORT_C TRealX &operator=(TReal32 aReal) __SOFTFP;
	IMPORT_C TRealX &operator=(TReal64 aReal) __SOFTFP;
	IMPORT_C TInt Set(TInt aInt);
	IMPORT_C TInt Set(TUint aInt);
	IMPORT_C TInt Set(const TInt64& aInt);
	IMPORT_C TInt Set(TReal32 aReal) __SOFTFP;
	IMPORT_C TInt Set(TReal64 aReal) __SOFTFP;
	IMPORT_C operator TInt() const;
	IMPORT_C operator TUint() const;
	IMPORT_C operator TInt64() const;
	IMPORT_C operator TReal32() const __SOFTFP;
	IMPORT_C operator TReal64() const __SOFTFP;
	IMPORT_C TInt GetTReal(TReal32 &aVal) const;
	IMPORT_C TInt GetTReal(TReal64 &aVal) const;
	IMPORT_C void SetZero(TBool aNegative=EFalse);
	IMPORT_C void SetNaN();
	IMPORT_C void SetInfinite(TBool aNegative);
	IMPORT_C TBool IsZero() const;
	IMPORT_C TBool IsNaN() const;
	IMPORT_C TBool IsInfinite() const;
	IMPORT_C TBool IsFinite() const;
	IMPORT_C const TRealX &operator+=(const TRealX &aVal);
	IMPORT_C const TRealX &operator-=(const TRealX &aVal);
	IMPORT_C const TRealX &operator*=(const TRealX &aVal);
	IMPORT_C const TRealX &operator/=(const TRealX &aVal);
	IMPORT_C const TRealX &operator%=(const TRealX &aVal);
	IMPORT_C TInt AddEq(const TRealX &aVal);
	IMPORT_C TInt SubEq(const TRealX &aVal);
	IMPORT_C TInt MultEq(const TRealX &aVal);
	IMPORT_C TInt DivEq(const TRealX &aVal);
	IMPORT_C TInt ModEq(const TRealX &aVal);
	IMPORT_C TRealX operator+() const;
	IMPORT_C TRealX operator-() const;
	IMPORT_C TRealX &operator++();
	IMPORT_C TRealX operator++(TInt);
	IMPORT_C TRealX &operator--();
	IMPORT_C TRealX operator--(TInt);
	IMPORT_C TRealX operator+(const TRealX &aVal) const;
	IMPORT_C TRealX operator-(const TRealX &aVal) const;
	IMPORT_C TRealX operator*(const TRealX &aVal) const;
	IMPORT_C TRealX operator/(const TRealX &aVal) const;
	IMPORT_C TRealX operator%(const TRealX &aVal) const;
	IMPORT_C TInt Add(TRealX& aResult,const TRealX &aVal) const;
	IMPORT_C TInt Sub(TRealX& aResult,const TRealX &aVal) const;
	IMPORT_C TInt Mult(TRealX& aResult,const TRealX &aVal) const;
	IMPORT_C TInt Div(TRealX& aResult,const TRealX &aVal) const;
	IMPORT_C TInt Mod(TRealX& aResult,const TRealX &aVal) const;
	IMPORT_C TRealXOrder Compare(const TRealX& aVal) const;
	inline TBool operator==(const TRealX &aVal) const;
	inline TBool operator!=(const TRealX &aVal) const;
	inline TBool operator>=(const TRealX &aVal) const;
	inline TBool operator<=(const TRealX &aVal) const;
	inline TBool operator>(const TRealX &aVal) const;
	inline TBool operator<(const TRealX &aVal) const;
public:
    /**
	The mantissa.
	*/
	// Represented as two adjacent 32 bit values, rather than one 64 value.
	// This is to avoid EABI introduced padding overheads and BC breakages. 
	// This representation works because the mantissa is always accessed from
	// assembler code as two 32 bit quantities. The C++ code that accesses it
	// now constructs an automatic TInt64 with the two components.
	TUint32 iMantLo;
	TUint32 iMantHi;
	
	/**
	The sign: 0 for +, 1 for -
	*/
	TInt8 iSign;	
	
	/**
	Flags: 0 for exact, 1 for rounded down, 2 for rounded up
	*/ 
	TUint8 iFlag;
	
	/**
	Exponent: biased by 32767, iExp=0 => zero, +65535 => infinity or NaN
	*/
	TUint16 iExp;
	};




struct SPoly
/**
@publishedAll
@released

A structure containing the set of coefficients for a polynomial.

@see Math::Poly
*/
    {
    TInt num;
	TReal c[1];
    };




class Math
/**
@publishedAll
@released

A collection of mathematical functions.
*/
	{
public:
	IMPORT_C static TInt ACos(TReal &aTrg,const TReal &aSrc);
	IMPORT_C static TInt ASin(TReal &aTrg,const TReal &aSrc);
	IMPORT_C static TInt ATan(TReal &aTrg,const TReal &aSrc);
	IMPORT_C static TInt ATan(TReal &aTrg,const TReal &aSrcY,const TReal &aSrcX);
	IMPORT_C static TInt Cos(TReal &aTrg,const TReal &aSrc);
	
	/**
	This function is not implemented by Symbian OS.
	*/
	IMPORT_C static TInt DtoR(TReal &aTrg,const TDesC &aSrc,TInt &aPos,const TChar aPoint);
	IMPORT_C static TInt Exp(TReal &aTrg,const TReal &aSrc);
	IMPORT_C static TInt Frac(TReal &aTrg,const TReal &aSrc);
	IMPORT_C static TInt Int(TReal &aTrg,const TReal &aSrc);
	IMPORT_C static TInt Int(TInt16 &aTrg,const TReal &aSrc);
	IMPORT_C static TInt Int(TInt32 &aTrg,const TReal &aSrc);
	IMPORT_C static TInt Log(TReal &aTrg,const TReal &aSrc);
	IMPORT_C static TInt Ln(TReal &aTrg,const TReal &aSrc);
	IMPORT_C static TInt Mod(TReal &aTrg,const TReal &aSrc,const TReal &aModulus);
	IMPORT_C static TReal Poly(TReal aVal,const SPoly *aPoly) __SOFTFP;
	IMPORT_C static TInt Pow(TReal &aTrg,const TReal &aSrc,const TReal &aPower);
	IMPORT_C static TInt Pow10(TReal &aTrg,const TInt exp);
	IMPORT_C static TInt Rand(TInt64 &aSeed);
	IMPORT_C static TReal FRand(TInt64 &aSeed) __SOFTFP;
	IMPORT_C static TUint32 Random();
	IMPORT_C static void Random(TDes8& aRandomValue);
	IMPORT_C static void RandomL(TDes8& aRandomValue);
	IMPORT_C static TUint32 RandomL();
	IMPORT_C static TInt Round(TReal &aTrg,const TReal &aSrc,TInt aDecimalPlaces);
	IMPORT_C static TInt Sin(TReal &aTrg,const TReal &aSrc); 
	IMPORT_C static TInt Sqrt(TReal &aTrg,const TReal &aSrc);
	IMPORT_C static TInt Tan(TReal &aTrg,const TReal &aSrc);
	IMPORT_C static TBool IsZero(const TReal &aVal);
	IMPORT_C static TBool IsNaN(const TReal &aVal);
	IMPORT_C static TBool IsInfinite(const TReal &aVal);
	IMPORT_C static TBool IsFinite(const TReal &aVal);
	IMPORT_C static void PolyX(TRealX& aY, const TRealX& aX, TInt aDeg, const TRealX *aCoef);
	static TInt MultPow10X(TRealX& aTrg, TInt aPower);
	IMPORT_C static void Mul64(Int64 aX, Int64 aY, Int64& aOutH, Uint64& aOutL);
	IMPORT_C static void UMul64(Uint64 aX, Uint64 aY, Uint64& aOutH, Uint64& aOutL);
	IMPORT_C static Int64 DivMod64(Int64 aDividend, Int64 aDivisor, Int64& aRemainder);
	IMPORT_C static Uint64 UDivMod64(Uint64 aDividend, Uint64 aDivisor, Uint64& aRemainder);
private:
	IMPORT_C static void SetZero(TReal &aVal,TInt aSign=0);
	IMPORT_C static void SetNaN(TReal &aVal);
	IMPORT_C static void SetInfinite(TReal &aVal,TInt aSign);
	};

#include <e32math.inl>

#endif // __E32MATH_H__
