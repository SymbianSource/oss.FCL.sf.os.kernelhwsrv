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
// e32\include\e32math.inl
// 
//

#if !defined(__E32MATH_INL__)
#define __E32MATH_INL__




// class TRealX

TBool TRealX::operator==(const TRealX &aVal) const
/**
@publishedAll
@released

Compares this extended precision number for equality with another.

@param aVal A reference to the extended precision value to be compared.

@return True, if this extended precision number is equal to aVal;
        false, otherwise.
*/
	{
	return(Compare(aVal)&EEqual);
	}




TBool TRealX::operator!=(const TRealX &aVal) const
/**
@publishedAll
@released

Compares this extended precision number for in-equality with another.

@param aVal A reference to the extended precision value to be compared.

@return True, if this extended precision number is not equal to aVal;
        false, otherwise.
*/
	{
	return!(Compare(aVal)&EEqual);
	}




TBool TRealX::operator>=(const TRealX &aVal) const
/**
@publishedAll
@released

Compares this extended precision number for being greater than
or equal to another.

@param aVal A reference to the extended precision value to be compared.

@return True, if this extended precision number is greater than or equal
        to aVal, false, otherwise.
*/
	{
	return(Compare(aVal)&(EEqual|EGreaterThan));
	}




TBool TRealX::operator<=(const TRealX &aVal) const
/**
@publishedAll
@released

Compares this extended precision number for being less than
or equal to another.

@param aVal A reference to the extended precision value to be compared.

@return True, if this extended precision number is less than or equal
        to aVal, false, otherwise.
*/
	{
	return(Compare(aVal)&(ELessThan|EEqual));
	}




TBool TRealX::operator>(const TRealX &aVal) const
/**
@publishedAll
@released

Compares this extended precision number for being greater than
another.

@param aVal A reference to the extended precision value to be compared.

@return True, if this extended precision number is greater than aVal,
        false, otherwise.
*/
	{
	return(Compare(aVal)&EGreaterThan);
	}




TBool TRealX::operator<(const TRealX &aVal) const
/**
@publishedAll
@released

Compares this extended precision number for being less than
another.

@param aVal A reference to the extended precision value to be compared.

@return True, if this extended precision number is less than aVal,
        false, otherwise.
*/
	{
	return(Compare(aVal)&ELessThan);
	}

#endif
