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


#if !defined __LOCL_LANGUAGE_H__
#define __LOCL_LANGUAGE_H__

#if !defined(__E32STD_H__)
#include <E32std.h>
#endif

#ifdef _UNICODE
#define TLocaleText TText16
#else
#define TLocaleText TText8
#endif

class LanguageAspect
	{
	public:
	static const TLanguage Language;
	static const TLanguage LanguageDowngrade[3];
	static const TDigitType DigitType;
	static const TFatUtilityFunctions* const FatUtilityFunctions;
	static const TLocaleText * const MsgTable[ELocaleMessages_LastMsg];
	static const TLocaleText * const DateSuffixTable[KMaxSuffixes];
	static const TLocaleText * const DayTable[KMaxDays];
	static const TLocaleText * const DayAbbTable[KMaxDays];
	static const TLocaleText * const MonthTable[KMaxMonths];
	static const TLocaleText * const MonthAbbTable[KMaxMonths];
	static const TLocaleText * const AmPmTable[KMaxAmPms];
	};


class LoclLanguage
	{
public:
	IMPORT_C static TLanguage Language();
	IMPORT_C static const TLanguage* LanguageDowngrade();
	IMPORT_C static TDigitType DigitType();	
	IMPORT_C static const TFatUtilityFunctions* FatUtilityFunctions();
	IMPORT_C static const TLocaleText* const *DateSuffixTable();
	IMPORT_C static const TLocaleText* const *DayTable();
	IMPORT_C static const TLocaleText* const *DayAbbTable();
	IMPORT_C static const TLocaleText* const *MonthTable();
	IMPORT_C static const TLocaleText* const *MonthAbbTable();
	IMPORT_C static const TLocaleText* const *AmPmTable();
	IMPORT_C static const TLocaleText* const *MsgTable();
	};

#endif  /* __LOCL_LANGUAGE_H__ */


