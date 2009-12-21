/*
* Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
	_S("\x661f\x671f\x4e00"),
	_S("\x661f\x671f\x4e8c"),
	_S("\x661f\x671f\x4e09"),
	_S("\x661f\x671f\x56db"),
	_S("\x661f\x671f\x4e94"),
	_S("\x661f\x671f\x516d"),
	_S("\x661f\x671f\x65e5")
	};
// The abbreviated day names
const TText * const LLanguage::DayAbbTable[KMaxDays] =
	{
	_S("\x661f\x671f\x4e00"),
	_S("\x661f\x671f\x4e8c"),
	_S("\x661f\x671f\x4e09"),
	_S("\x661f\x671f\x56db"),
	_S("\x661f\x671f\x4e94"),
	_S("\x661f\x671f\x516d"),
	_S("\x661f\x671f\x65e5")
	};
// The month names
const TText * const LLanguage::MonthTable[KMaxMonths] =
	{
	_S("\x4e00\x6708"),
	_S("\x4e8c\x6708"),
	_S("\x4e09\x6708"),
	_S("\x56db\x6708"),
	_S("\x4e94\x6708"),
	_S("\x516d\x6708"),
	_S("\x4e03\x6708"),
	_S("\x516b\x6708"),
	_S("\x4e5d\x6708"),
	_S("\x5341\x6708"),
	_S("\x5341\x4e00\x6708"),
	_S("\x5341\x4e8c\x6708")
	};
// The abbreviated month names
const TText * const LLanguage::MonthAbbTable[KMaxMonths] =
	{
	_S("\x4e00\x6708"),
	_S("\x4e8c\x6708"),
	_S("\x4e09\x6708"),
	_S("\x56db\x6708"),
	_S("\x4e94\x6708"),
	_S("\x516d\x6708"),
	_S("\x4e03\x6708"),
	_S("\x516b\x6708"),
	_S("\x4e5d\x6708"),
	_S("\x5341\x6708"),
	_S("\x5341\x4e00\x6708"),
	_S("\x5341\x4e8c\x6708")
	};
// The am/pm strings
const TText * const LLanguage::AmPmTable[KMaxAmPms] =
	{
	_S("\x4e0a\x5348"),
	_S("\x4e0b\x5348")
	};

