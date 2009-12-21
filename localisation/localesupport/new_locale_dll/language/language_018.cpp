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
const TLanguage LanguageAspect::Language = ELangDutch;
const TLanguage LanguageAspect::LanguageDowngrade[3] = {ELangNone, ELangNone, ELangNone};
const TDigitType LanguageAspect::DigitType = EDigitTypeWestern;
const TFatUtilityFunctions* const LanguageAspect::FatUtilityFunctions = NULL;


const TText * const LanguageAspect::MsgTable[ELocaleMessages_LastMsg] =
	{
// Fileserver
	_S("Opnieuw"),					                       // Button 1
	_S("Breek af"),					        	       // Button 2
	_S("Plaats disk terug"),			                       // Put the card back - line1
	_S("Anders gaan er gegevens verloren"),			                       // Put the card back - line2
	_S("Batterij is zwak"),			                       // Low power - line1
	_S("Kan schrijven naar disk niet voltooien"),		                       // Low power - line2
	_S("Diskfout - kan schrijven niet voltooien"),	                       // Disk error - line1
	_S("Probeer het opnieuw, anders gaan er gegevens verloren"),		                       // Disk error - line2
// SoundDriver
	_S("Carillon"),								// Chimes
	_S("Bel"),								// Rings
	_S("Signaal"),								// Signal
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
	_S("Verbinding(01)"),							// Socket(01)
	_S("Verbinding(02)"),							// Socket(02)
	_S("Verbinding(03)"),							// Socket(03)
	_S("Verbinding(04)")							// Socket(04)
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
	_S("Maandag"),
	_S("Dinsdag"),
	_S("Woensdag"),
	_S("Donderdag"),
	_S("Vrijdag"),
	_S("Zaterdag"),
	_S("Zondag")
	};
// The abbreviated day names
const TText * const LanguageAspect::DayAbbTable[KMaxDays] =
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
const TText * const LanguageAspect::MonthTable[KMaxMonths] =
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
const TText * const LanguageAspect::MonthAbbTable[KMaxMonths] =
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
const TText * const LanguageAspect::AmPmTable[KMaxAmPms] = {_S("am"),_S("pm")};

