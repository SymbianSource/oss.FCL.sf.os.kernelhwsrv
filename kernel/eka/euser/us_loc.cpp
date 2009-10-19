// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\us_loc.cpp
// 
//

#include "us_std.h"

EXPORT_C TDayName::TDayName()
/**
Default constructor.

It constructs this object and initialises it with the current locale's text for 
Monday.
*/
	{

	Set(EMonday);
	}




EXPORT_C TDayName::TDayName(TDay aDay)
/**
Constructs this object and initialises it with the current locale's text for 
the specified day of the week.

@param aDay Identifies the day of the week.
*/
	{

	Set(aDay);
	}




EXPORT_C TDayNameAbb::TDayNameAbb()
/**
Default constructor.

It constructs this object and initialises it with the current locale's
abbreviated text for Monday.
*/
	{

	Set(EMonday);
	}




EXPORT_C TDayNameAbb::TDayNameAbb(TDay aDay)
/**
Constructs this object and initialises it with the current locale's abbreviated 
text for the specified day of the week.

@param aDay An enumerator identifying the day of the week.
*/
	{

	Set(aDay);
	}




EXPORT_C TMonthName::TMonthName()
/**
Default constructor.

Constructs the object and initialises it with the current locale's text for 
January.
*/
	{

	Set(EJanuary);
	}




EXPORT_C TMonthName::TMonthName(TMonth aMonth)
/**
Constructs this object and initialises it with the current locale's text for 
the specified month.

@param aMonth Identifies the month.
*/
	{

	Set(aMonth);
	}




EXPORT_C TMonthNameAbb::TMonthNameAbb()
/**
Default constructor.

It constructs this object and initialises it with the current locale's abbreviated 
text for January.
*/
	{

	Set(EJanuary);
	}




EXPORT_C TMonthNameAbb::TMonthNameAbb(TMonth aMonth)
/**
Constructs this object and initialises it with the current locale's abbreviated 
text for the specified month.

@param aMonth Identifies the month.
*/
	{

	Set(aMonth);
	}




EXPORT_C TDateSuffix::TDateSuffix()
/**
Default constructor.

It constructs this object and initialises it with the current locale's date
suffix text for the first day of the month.
*/
	{

	Set(0);
	}




EXPORT_C TDateSuffix::TDateSuffix(TInt aDateSuffix)
/**
Constructs this object and initialises it with the current locale's date suffix 
text for the specified day of the month.

@param aDateSuffix A value identifying the day of the month. The value can 
                   range from 0 to 30 so that the first day of the month is
                   identified by 0, the second day by 1 etc.
                   
@panic USER 69, if aDateSuffix is outside the range 0 to 30.
*/
	{

	Set(aDateSuffix);
	}




EXPORT_C TAmPmName::TAmPmName()
/**
Default constructor.

It constructs this object and initialises it with the current locale's text for 
identifying time before noon.
*/
	{

	Set(EAm);
	}




EXPORT_C TAmPmName::TAmPmName(TAmPm aSelector)
/**
Constructs this object and initialises it with the current locale's text for 
identifying time before or after noon as identified by the specified selector.

@param aSelector The am/pm selector.
*/
	{

	Set(aSelector);
	}




EXPORT_C TCurrencySymbol::TCurrencySymbol()
/**
Default constructor.

It constructs this object and initialises it with the current locale's currency 
symbol(s).
*/
	{

	Set();
	}




EXPORT_C TShortDateFormatSpec::TShortDateFormatSpec()
/**
Default constructor.

Performs construction by calling Set().

@see TShortDateFormatSpec::Set
*/
	{

	Set();
	}




EXPORT_C TLongDateFormatSpec::TLongDateFormatSpec()
/**
Default constructor.

Performs construction by calling Set().

@see TLongDateFormatSpec::Set
*/
	{

	Set();
	}




EXPORT_C TTimeFormatSpec::TTimeFormatSpec()
/**
Default constructor.

Performs construction by calling Set().

@see TTimeFormatSpec::Set
*/
	{

	Set();
	}




EXPORT_C TLocaleMessageText::TLocaleMessageText()
//
// Default Constructor
//
	{}




EXPORT_C TLocaleMessageText::TLocaleMessageText(TLocaleMessage aMsgNo)
//
// Constructor
//
	{

	Set(aMsgNo);
	}


