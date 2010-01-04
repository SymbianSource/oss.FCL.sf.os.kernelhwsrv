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
* Default settings for the Dutch language
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
	_S("Maandag"),
	_S("Dinsdag"),
	_S("Woensdag"),
	_S("Donderdag"),
	_S("Vrijdag"),
	_S("Zaterdag"),
	_S("Zondag")
	};
// The abbreviated day names
const TText * const LLanguage::DayAbbTable[KMaxDays] =
	{
	_S("Ma"),
	_S("Di"),
	_S("Wo"),
	_S("Do"),
	_S("Vr"),
	_S("Za"),
	_S("Zo")
	};
// The month names
const TText * const LLanguage::MonthTable[KMaxMonths] =
	{
	_S("Januari"),
	_S("Februari"),
	_S("Maart"),
	_S("April"),
	_S("Mei"),
	_S("Juni"),
	_S("Juli"),
	_S("Augustus"),
	_S("September"),
	_S("Oktober"),
	_S("November"),
	_S("December")
	};
// The abbreviated month names
const TText * const LLanguage::MonthAbbTable[KMaxMonths] =
	{
	_S("Jan"),
	_S("Feb"),
	_S("Maa"),
	_S("Apr"),
	_S("Mei"),
	_S("Jun"),
	_S("Jul"),
	_S("Aug"),
	_S("Sep"),
	_S("Okt"),
	_S("Nov"),
	_S("Dec")
	};
// The am/pm strings
const TText * const LLanguage::AmPmTable[KMaxAmPms] = {_S("am"),_S("pm")};


