// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\locl\t_lneng2.cpp
// Settings for the English language (UK & US), but with Vietnamese month names
// 
//

#include <kernel/localise.h>

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
	_S("Thang 1"),
	_S("Thang 2"),
	_S("Thang 3"),
	_S("Thang 4"),
	_S("Thang 5"),
	_S("Thang 6"),
	_S("Thang 7"),
	_S("Thang 8"),
	_S("Thang 9"),
	_S("Thang 10"),
	_S("Thang 11"),
	_S("Thang 12")
	};
// The abbreviated month names
const TText * const LLanguage::MonthAbbTable[KMaxMonths] =
	{
	_S("TH1"),
	_S("TH2"),
	_S("TH3"),
	_S("TH4"),
	_S("TH5"),
	_S("TH6"),
	_S("TH7"),
	_S("TH8"),
	_S("TH9"),
	_S("TH10"),
	_S("TH11"),
	_S("TH12")
	};
// The am/pm strings
const TText * const LLanguage::AmPmTable[KMaxAmPms] = {_S("am"),_S("pm")};


