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
* Default settings for the French language
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
	_S("Lundi"),
	_S("Mardi"),
	_S("Mercredi"),
	_S("Jeudi"),
	_S("Vendredi"),
	_S("Samedi"),
	_S("Dimanche")
	};
// The abbreviated day names
const TText * const LLanguage::DayAbbTable[KMaxDays] =
	{
	_S("Lun"),
	_S("Mar"),
	_S("Mer"),
	_S("Jeu"),
	_S("Ven"),
	_S("Sam"),
	_S("Dim")
	};
// The month names
const TText * const LLanguage::MonthTable[KMaxMonths] =
	{
	_S("Janvier"),
	_S("F\351vrier"),
	_S("Mars"),
	_S("Avril"),
	_S("Mai"),
	_S("Juin"),
	_S("Juillet"),
	_S("Ao\373t"),
	_S("Septembre"),
	_S("Octobre"),
	_S("Novembre"),
	_S("D\351cembre")
	};
// The abbreviated month names
const TText * const LLanguage::MonthAbbTable[KMaxMonths] =
	{
	_S("Jan"),
	_S("F\351v"),
	_S("Mar"),
	_S("Avr"),
	_S("Mai"),
	_S("Jun"),
	_S("Jul"),
	_S("Ao\373"),
	_S("Sep"),
	_S("Oct"),
	_S("Nov"),
	_S("D\351c")
	};
// The am/pm strings
const TText * const LLanguage::AmPmTable[KMaxAmPms] = {_S("am"),_S("pm")};


