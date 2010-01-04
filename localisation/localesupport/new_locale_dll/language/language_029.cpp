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
*
*/


#include "ls_std.h"
#include "locl_language.h"

// The configuration data
const TLanguage LanguageAspect::Language = ELangTaiwanChinese;
const TLanguage LanguageAspect::LanguageDowngrade[3] = {ELangNone, ELangNone, ELangNone};
const TDigitType LanguageAspect::DigitType = EDigitTypeWestern;
const TFatUtilityFunctions* const LanguageAspect::FatUtilityFunctions = NULL;


const TText * const LanguageAspect::MsgTable[ELocaleMessages_LastMsg] =
	{
// Fileserver
	_S("Retry"),								// Button 1
	_S("Stop"),									// Button 2
	_S("Put the disk back"),					// Put the card back - line1
	_S("or data will be lost"),					// Put the card back - line2
	_S("Batteries too low"),					// Low power - line1
	_S("Cannot complete write to disk"),		// Low power - line2
	_S("Disk error - cannot complete write"),	// Disk error - line1
	_S("Retry or data will be lost"),			// Disk error - line2
// SoundDriver
	_S("Chimes"),								// Chimes
	_S("Rings"),								// Rings
	_S("Signal"),								// Signal
// MediaDriver diskname (max 16 chars)
	_S("Internal"),								// Internal
	_S("External(01)"),							// External(01)
	_S("External(02)"),							// External(02)
	_S("External(03)"),							// External(03)
	_S("External(04)"),							// External(04)
	_S("External(05)"),							// External(05)
	_S("External(06)"),							// External(06)
	_S("External(07)"),							// External(07)
	_S("External(08)"),							// External(08)
// MediaDriver socketname (max 16 chars)
	_S("Socket(01)"),							// Socket(01)
	_S("Socket(02)"),							// Socket(02)
	_S("Socket(03)"),							// Socket(03)
	_S("Socket(04)")							// Socket(04)
	};

	
// The suffix table
const TText * const LanguageAspect::DateSuffixTable[KMaxSuffixes] =
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
const TText * const LanguageAspect::DayTable[KMaxDays] =
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
const TText * const LanguageAspect::DayAbbTable[KMaxDays] =
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
const TText * const LanguageAspect::MonthTable[KMaxMonths] =
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
const TText * const LanguageAspect::MonthAbbTable[KMaxMonths] =
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
const TText * const LanguageAspect::AmPmTable[KMaxAmPms] =
	{
	_S("\x4e0a\x5348"),
	_S("\x4e0b\x5348")
	};

