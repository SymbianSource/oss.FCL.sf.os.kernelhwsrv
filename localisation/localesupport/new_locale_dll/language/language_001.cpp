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
const TLanguage LanguageAspect::Language = ELangEnglish;
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
	_S("Retry or data would be lost"),			// Disk error - line2
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
	_S("st"),_S("nd"),_S("rd"),_S("th"),_S("th"),
	_S("th"),_S("th"),_S("th"),_S("th"),_S("th"),
	_S("th"),_S("th"),_S("th"),_S("th"),_S("th"),
	_S("th"),_S("th"),_S("th"),_S("th"),_S("th"),
	_S("st"),_S("nd"),_S("rd"),_S("th"),_S("th"),
	_S("th"),_S("th"),_S("th"),_S("th"),_S("th"),
	_S("st")
	};
// The day names
const TText * const LanguageAspect::DayTable[KMaxDays] =
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
const TText * const LanguageAspect::DayAbbTable[KMaxDays] =
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
const TText * const LanguageAspect::MonthTable[KMaxMonths] =
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
const TText * const LanguageAspect::MonthAbbTable[KMaxMonths] =
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
const TText * const LanguageAspect::AmPmTable[KMaxAmPms] = {_S("am"),_S("pm")};

