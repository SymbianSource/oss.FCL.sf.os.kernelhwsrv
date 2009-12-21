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
const TLanguage LanguageAspect::Language = ELangGerman;
const TLanguage LanguageAspect::LanguageDowngrade[3] = {ELangNone, ELangNone, ELangNone};
const TDigitType LanguageAspect::DigitType = EDigitTypeWestern;
const TFatUtilityFunctions* const LanguageAspect::FatUtilityFunctions = NULL;


const TText * const LanguageAspect::MsgTable[ELocaleMessages_LastMsg] =
	{
// Fileserver
	_S("Wiederholen"),								// Button 1
	_S("Stopp"),								// Button 2
	_S("Disk wieder einlegen,"),						// Put the card back - line1
	_S("sonst Datenverlust"),						// Put the card back - line2
	_S("Batterien zu schwach"),						// Low power - line1
	_S("Kann Schreiben auf Disk nicht abschlie\337en"),					// Low power - line2
	_S("Diskfehler - kann Schreiben nicht abschlie\337en"),				// Disk error - line1
	_S("Wiederholen, sonst Datenverlust"),		                        // Disk error - line2
// SoundDriver
	_S("Melodie"),								// Chimes
	_S("Klingel"),								// Rings
	_S("Signal"),								// Signal
// MediaDriver diskname (max 16 chars)
	_S("Intern"),								// Internal
	_S("Extern(01)"),							// External(01)
	_S("Extern(02)"),							// External(02)
	_S("Extern(03)"),							// External(03)
	_S("Extern(04)"),							// External(04)
	_S("Extern(05)"),							// External(05)
	_S("Extern(06)"),							// External(06)
	_S("Extern(07)"),							// External(07)
	_S("Extern(08)"),							// External(08)
// MediaDriver socketname (max 16 chars)
	_S("Steckplatz(01)"),							// Socket(01)
	_S("Steckplatz(02)"),							// Socket(02)
	_S("Steckplatz(03)"),							// Socket(03)
	_S("Steckplatz(04)")							// Socket(04)
	};
	
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
	_S("Montag"),
	_S("Dienstag"),
	_S("Mittwoch"),
	_S("Donnerstag"),
	_S("Freitag"),
	_S("Samstag"),
	_S("Sonntag")
	};
// The abbreviated day names
const TText * const LanguageAspect::DayAbbTable[KMaxDays] =
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
const TText * const LanguageAspect::MonthTable[KMaxMonths] =
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
const TText * const LanguageAspect::MonthAbbTable[KMaxMonths] =
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
const TText * const LanguageAspect::AmPmTable[KMaxAmPms] = {_S("am"),_S("pm")};
