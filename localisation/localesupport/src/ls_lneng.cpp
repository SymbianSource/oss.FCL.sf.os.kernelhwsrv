/*
* Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
* Default settings for the English language (UK & US)
*
*/


#include "ls_std.h"

// The suffix table
const TText * const LLanguage::DateSuffixTable[KMaxSuffixes] =
	{
	_S("st"),_S("nd"),_S("rd"),_S("th"),_S("th"),
	_S("th"),_S("th"),_S("th"),_S("th"),_S("th"),
	_S("th"),_S("th"),_S("th"),_S("th"),_S("th"),
	_S("th"),_S("th"),_S("th"),_S("th"),_S("th"),
	_S("st"),_S("nd"),_S("rd"),_S("th"),_S("th"),
	_S("th"),_S("th"),_S("th"),_S("th"),_S("th"),
	_S("st")
	};
// The day names
const TText * const LLanguage::DayTable[KMaxDays] =
	{
	_S("Monday"),
	_S("Tuesday"),
	_S("Wednesday"),
	_S("Thursday"),
	_S("Friday"),
	_S("Saturday"),
	_S("Sunday")
	};
// The abbreviated day names
const TText * const LLanguage::DayAbbTable[KMaxDays] =
	{
	_S("Mon"),
	_S("Tue"),
	_S("Wed"),
	_S("Thu"),
	_S("Fri"),
	_S("Sat"),
	_S("Sun")
	};
// The month names
const TText * const LLanguage::MonthTable[KMaxMonths] =
	{
	_S("January"),
	_S("February"),
	_S("March"),
	_S("April"),
	_S("May"),
	_S("June"),
	_S("July"),
	_S("August"),
	_S("September"),
	_S("October"),
	_S("November"),
	_S("December")
	};
// The abbreviated month names
const TText * const LLanguage::MonthAbbTable[KMaxMonths] =
	{
	_S("Jan"),
	_S("Feb"),
	_S("Mar"),
	_S("Apr"),
	_S("May"),
	_S("Jun"),
	_S("Jul"),
	_S("Aug"),
	_S("Sep"),
	_S("Oct"),
	_S("Nov"),
	_S("Dec")
	};
// The am/pm strings
const TText * const LLanguage::AmPmTable[KMaxAmPms] = {_S("am"),_S("pm")};


