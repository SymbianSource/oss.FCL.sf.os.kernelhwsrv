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
#include "complocl.h"

// The Chinese configuration data

const TLanguage LLocaleData::Language = ELangTaiwanChinese;
const TInt LLocaleData::CountryCode = 886;
const TInt LLocaleData::UniversalTimeOffset = 8*3600;
const TDateFormat LLocaleData::DateFormat = EDateJapanese;
const TTimeFormat LLocaleData::TimeFormat = ETime12;
const TLocalePos LLocaleData::CurrencySymbolPosition = ELocaleBefore;
const TBool LLocaleData::CurrencySpaceBetween = EFalse;
const TInt LLocaleData::CurrencyDecimalPlaces = 0;
const TLocale::TNegativeCurrencyFormat LLocaleData::NegativeCurrencyFormat=TLocale::TNegativeCurrencyFormat(0); // replacing CurrencyNegativeInBrackets
const TBool LLocaleData::CurrencyTriadsAllowed = ETrue;
const TText * const LLocaleData::ThousandsSeparator = _S(",");
const TText * const LLocaleData::DecimalSeparator = _S(".");
const TText * const LLocaleData::DateSeparator[KMaxDateSeparators] = {_S(""),_S("/"),_S("/"),_S("")};
const TText * const LLocaleData::TimeSeparator[KMaxTimeSeparators] = {_S(""),_S(":"),_S(":"),_S("")};
const TLocalePos LLocaleData::AmPmSymbolPosition = ELocaleBefore;
const TBool LLocaleData::AmPmSpaceBetween = EFalse;
//const TUint LLocaleData::DaylightSaving = EDstNone;
const TDaylightSavingZone LLocaleData::HomeDaylightSavingZone = EDstNone;
const TUint LLocaleData::WorkDays = 0x1f;
const TText * const LLocaleData::CurrencySymbol = _S("NT$");
const TText* const LLocaleData::ShortDateFormatSpec = _S("%F%*D/%*M/%Y");
const TText* const LLocaleData::LongDateFormatSpec = _S("%F%Y\x5e74%*M\x6708%*D\x65e5");
const TText* const LLocaleData::TimeFormatSpec = _S("%F%*I:%T:%S %*A");
const TFatUtilityFunctions* const LLocaleData::FatUtilityFunctions = NULL;
const TDay LLocaleData::StartOfWeek = EMonday;
const TClockFormat LLocaleData::ClockFormat = EClockAnalog;
const TUnitsFormat LLocaleData::UnitsGeneral = EUnitsMetric;
const TUnitsFormat LLocaleData::UnitsDistanceShort = EUnitsMetric;
const TUnitsFormat LLocaleData::UnitsDistanceLong = EUnitsMetric;
const TUint LLocaleData::ExtraNegativeCurrencyFormatFlags = 0;
const TLanguage LLocaleData::LanguageDowngrade[3] = {ELangNone, ELangNone, ELangNone};
const TDigitType LLocaleData::DigitType = EDigitTypeWestern;

