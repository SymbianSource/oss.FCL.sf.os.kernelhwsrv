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
* Default settings for the German language
*
*/


#include "ls_std.h"

// The suffix table
const TText * const LLanguage::DateSuffixTable[KMaxSuffixes] =
	{
	_S(""),_S(""),_S(""),_S(""),_S(""),
	_S(""),_S(""),_S(""),_S(""),_S(""),
	_S(""),_S(""),_S(""),_S(""),_S(""),
	_S(""),_S(""),_S(""),_S(""),_S(""),
	_S(""),_S(""),_S(""),_S(""),_S(""),
	_S(""),_S(""),_S(""),_S(""),_S(""),
	_S("")
	};
// The day names
const TText * const LLanguage::DayTable[KMaxDays] =
	{
	_S("Montag"),
	_S("Dienstag"),
	_S("Mittwoch"),
	_S("Donnerstag"),
	_S("Freitag"),
	_S("Samstag"),
	_S("Sonntag")
	};
// The abbreviated day names
const TText * const LLanguage::DayAbbTable[KMaxDays] =
	{
	_S("Mo"),
	_S("Di"),
	_S("Mi"),
	_S("Do"),
	_S("Fr"),
	_S("Sa"),
	_S("So")
	};
// The month names
const TText * const LLanguage::MonthTable[KMaxMonths] =
	{
	_S("Januar"),
	_S("Februar"),
	_S("M\344rz"),
	_S("April"),
	_S("Mai"),
	_S("Juni"),
	_S("Juli"),
	_S("August"),
	_S("September"),
	_S("Oktober"),
	_S("November"),
	_S("Dezember")
	};
// The abbreviated month names
const TText * const LLanguage::MonthAbbTable[KMaxMonths] =
	{
	_S("Jan"),
	_S("Feb"),
	_S("M\344r"),
	_S("Apr"),
	_S("Mai"),
	_S("Jun"),
	_S("Jul"),
	_S("Aug"),
	_S("Sep"),
	_S("Okt"),
	_S("Nov"),
	_S("Dez")
	};
// The am/pm strings
const TText * const LLanguage::AmPmTable[KMaxAmPms] = {_S("am"),_S("pm")};

