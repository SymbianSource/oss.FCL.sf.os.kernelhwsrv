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


#if !defined __COMPLOCL_H__
#define __COMPLOCL_H__

#if !defined(__E32STD_H__)
#include <E32std.h>
#endif

class LLocaleData
	{
	public:
	static const TLanguage Language;
	static const TInt CountryCode;
	static const TInt UniversalTimeOffset;
	static const TDateFormat DateFormat;
	static const TTimeFormat TimeFormat;
	static const TText * const CurrencySymbol;
	static const TLocalePos CurrencySymbolPosition;
	static const TBool CurrencySpaceBetween;
	static const TInt CurrencyDecimalPlaces;
	static const TLocale::TNegativeCurrencyFormat NegativeCurrencyFormat;
	static const TBool CurrencyTriadsAllowed;
	static const TText* const ShortDateFormatSpec;
	static const TText* const LongDateFormatSpec;
	static const TText* const TimeFormatSpec;
	static const TFatUtilityFunctions* const FatUtilityFunctions;
	static const TText * const ThousandsSeparator;
	static const TText * const DecimalSeparator;
	static const TText * const DateSeparator[KMaxDateSeparators];
	static const TText * const TimeSeparator[KMaxTimeSeparators];
	static const TLocalePos AmPmSymbolPosition;
	static const TBool AmPmSpaceBetween;
	static const TDaylightSavingZone HomeDaylightSavingZone;
	static const TUint WorkDays;
	static const TDay StartOfWeek;
	static const TClockFormat ClockFormat;
	static const TUnitsFormat UnitsGeneral;
	static const TUnitsFormat UnitsDistanceLong;
	static const TUnitsFormat UnitsDistanceShort;
	static const TUint ExtraNegativeCurrencyFormatFlags;
	static const TLanguage LanguageDowngrade[3];
	static const TDigitType DigitType;
	};

#endif


