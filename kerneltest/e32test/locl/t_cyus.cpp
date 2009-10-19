// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\locl\t_cyus.cpp
// Default locale settings for the US
// 
//

#include <kernel/localise.h>

// The configuration data
const TLanguage LCountry::Language = ELangAmerican;
const TInt LCountry::CountryCode = 1;
const TInt LCountry::UniversalTimeOffset = -6;
const TDateFormat LCountry::DateFormat = EDateAmerican;
const TTimeFormat LCountry::TimeFormat = ETime12;
const TLocalePos LCountry::CurrencySymbolPosition = ELocaleBefore;
const TBool LCountry::CurrencySpaceBetween = EFalse;
const TInt LCountry::CurrencyDecimalPlaces = 2;
const TBool LCountry::CurrencyNegativeInBrackets = EFalse;
const TBool LCountry::CurrencyTriadsAllowed = ETrue;
const TText * const LCountry::ThousandsSeparator = _S(",");
const TText * const LCountry::DecimalSeparator = _S(".");
const TText * const LCountry::DateSeparator[KMaxDateSeparators] = {_S(""),_S("/"),_S("/"),_S("")};
const TText * const LCountry::TimeSeparator[KMaxTimeSeparators] = {_S(""),_S(":"),_S(":"),_S("")};
const TLocalePos LCountry::AmPmSymbolPosition = ELocaleAfter;
const TBool LCountry::AmPmSpaceBetween = ETrue;
//const TUint LCountry::DaylightSaving = EDstNone;
const TDaylightSavingZone LCountry::HomeDaylightSavingZone = EDstNorthern;
const TUint LCountry::WorkDays = 0x1f;
const TText * const LCountry::CurrencySymbol = _S("$");
const TText* const LCountry::ShortDateFormatSpec = _S("%F%*M/%*D/%Y"); // needs checking by a localisation team (this item was added since real localisation - the value given here has been set by a software developer so it may be wrong)
const TText* const LCountry::LongDateFormatSpec = _S("%F%*D%X %N %Y"); // needs checking by a localisation team (this item was added since real localisation - the value given here has been set by a software developer so it may be wrong)
const TText* const LCountry::TimeFormatSpec = _S("%F%*I:%T:%S %*A"); // needs checking by a localisation team (this item was added since real localisation - the value given here has been set by a software developer so it may be wrong)
const TFatUtilityFunctions* const LCountry::FatUtilityFunctions = NULL;
const TDay LCountry::StartOfWeek = ESunday;
const TClockFormat LCountry::ClockFormat = EClockAnalog;
const TUnitsFormat LCountry::UnitsGeneral = EUnitsImperial;
const TUnitsFormat LCountry::UnitsDistanceShort = EUnitsImperial;
const TUnitsFormat LCountry::UnitsDistanceLong = EUnitsImperial;

