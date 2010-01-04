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
const TLanguage LanguageAspect::Language = ELangJapanese;
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

	
const TText hi[]={0x65e5,0};
const TText getsuyoubi[]={0x6708,0x66dc,0x65e5,0};
const TText kayoubi[]={0x706b,0x66dc,0x65e5,0};
const TText suiyoubi[]={0x6c34,0x66dc,0x65e5,0};
const TText mokuyoubi[]={0x6728,0x66dc,0x65e5,0};
const TText kinyoubi[]={0x91d1,0x66dc,0x65e5,0};
const TText doyoubi[]={0x571f,0x66dc,0x65e5,0};
const TText nichiyoubi[]={0x65e5,0x66dc,0x65e5,0};
const TText getsu[]={0x6708,0};
const TText ka[]={0x706b,0};
const TText sui[]={0x6c34,0};
const TText moku[]={0x6728,0};
const TText kin[]={0x91d1,0};
const TText dou[]={0x571f,0};
const TText nichi[]={0x65e5,0};
const TText ichigatsu[]={0xff11,0};
const TText nigatsu[]={0xff12,0};
const TText sangatsu[]={0xff13,0};
const TText shigatsu[]={0xff14,0};
const TText gogatsu[]={0xff15,0};
const TText rokugatsu[]={0xff16,0};
const TText shichigatsu[]={0xff17,0};
const TText hachigatsu[]={0xff18,0};
const TText kugatsu[]={0xff19,0};
const TText jyugatsu[]={0x0031,0x0030,0};
const TText jyuichigatsu[]={0x0031,0x0031,0};
const TText jyunigatsu[]={0x0031,0x0032,0};
const TText a_ichigatsu[]={0x0031,0x6708,0};
const TText a_nigatsu[]={0x0032,0x6708,0};
const TText a_sangatsu[]={0x0033,0x6708,0};
const TText a_shigatsu[]={0x0034,0x6708,0};
const TText a_gogatsu[]={0x0035,0x6708,0};
const TText a_rokugatsu[]={0x0036,0x6708,0};
const TText a_shichigatsu[]={0x0037,0x6708,0};
const TText a_hachigatsu[]={0x0038,0x6708,0};
const TText a_kugatsu[]={0x0039,0x6708,0};
const TText a_jyugatsu[]={0x0031,0x0030,0x6708,0};
const TText a_jyuichigatsu[]={0x0031,0x0031,0x6708,0};
const TText a_jyunigatsu[]={0x0031,0x0032,0x6708,0};
const TText gozen[]={0x5348,0x524d,0};
const TText gogo[]={0x5348,0x5f8c,0};



// The suffix table
const TText * const LanguageAspect::DateSuffixTable[KMaxSuffixes] =
	{
	hi,hi,hi,hi,hi,
	hi,hi,hi,hi,hi,
	hi,hi,hi,hi,hi,
	hi,hi,hi,hi,hi,
	hi,hi,hi,hi,hi,
	hi,hi,hi,hi,hi,
	hi
	};
// The day names
const TText * const LanguageAspect::DayTable[KMaxDays] =
	{
	getsuyoubi,
	kayoubi,
	suiyoubi,
	mokuyoubi,
	kinyoubi,
	doyoubi,
	nichiyoubi
	};
// The abbreviated day names
const TText * const LanguageAspect::DayAbbTable[KMaxDays] =
	{
	getsu,
	ka,
	sui,
	moku,
	kin,
	dou,
	nichi
	};
// The month names
const TText * const LanguageAspect::MonthTable[KMaxMonths] =
	{
	ichigatsu,
	nigatsu,
	sangatsu,
	shigatsu,
	gogatsu,
	rokugatsu,
	shichigatsu,
	hachigatsu,
	kugatsu,
	jyugatsu,
	jyuichigatsu,
	jyunigatsu
	};
// The abbreviated month names
const TText * const LanguageAspect::MonthAbbTable[KMaxMonths] =
	{
	a_ichigatsu,
	a_nigatsu,
	a_sangatsu,
	a_shigatsu,
	a_gogatsu,
	a_rokugatsu,
	a_shichigatsu,
	a_hachigatsu,
	a_kugatsu,
	a_jyugatsu,
	a_jyuichigatsu,
	a_jyunigatsu
	};
// The am/pm strings
const TText * const LanguageAspect::AmPmTable[KMaxAmPms] = {gozen,gogo};

