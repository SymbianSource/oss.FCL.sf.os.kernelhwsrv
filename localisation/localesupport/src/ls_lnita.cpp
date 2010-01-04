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
* Default settings for the Italian language
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
	_S("Luned\354"),
	_S("Marted\354"),
	_S("Mercoled\354"),
	_S("Gioved\354"),
	_S("Venerd\354"),
	_S("Sabato"),
	_S("Domenica")
	};
// The abbreviated day names
const TText * const LLanguage::DayAbbTable[KMaxDays] =
	{
	_S("Lun"),
	_S("Mar"),
	_S("Mer"),
	_S("Gio"),
	_S("Ven"),
	_S("Sab"),
	_S("Dom")
	};
// The month names
const TText * const LLanguage::MonthTable[KMaxMonths] =
	{
	_S("Gennaio"),
	_S("Febbraio"),
	_S("Marzo"),
	_S("Aprile"),
	_S("Maggio"),
	_S("Giugno"),
	_S("Luglio"),
	_S("Agosto"),
	_S("Settembre"),
	_S("Ottobre"),
	_S("Novembre"),
	_S("Dicembre")
	};
// The abbreviated month names
const TText * const LLanguage::MonthAbbTable[KMaxMonths] =
	{
	_S("Gen"),
	_S("Feb"),
	_S("Mar"),
	_S("Apr"),
	_S("Mag"),
	_S("Giu"),
	_S("Lug"),
	_S("Ago"),
	_S("Set"),
	_S("Ott"),
	_S("Nov"),
	_S("Dic")
	};
// The am/pm strings
const TText * const LLanguage::AmPmTable[KMaxAmPms] = {_S("am"),_S("pm")};


