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
// f32test\locl\t_cyge.cpp
// Default locale settings for Germany
// 
//

#include <kernel/localise.h>

// The configuration data
const TLanguage LCountry::Language = ELangGerman;
const TInt LCountry::CountryCode = 49;
const TInt LCountry::UniversalTimeOffset = 1;
const TDateFormat LCountry::DateFormat = EDateEuropean;
const TTimeFormat LCountry::TimeFormat = ETime24;
const TLocalePos LCountry::CurrencySymbolPosition = ELocaleAfter;
const TBool LCountry::CurrencySpaceBetween = ETrue;
const TInt LCountry::CurrencyDecimalPlaces = 2;
const TBool LCountry::CurrencyNegativeInBrackets = ETrue;
const TBool LCountry::CurrencyTriadsAllowed = ETrue;
const TText * const LCountry::ThousandsSeparator = _S(".");
const TText * const LCountry::DecimalSeparator = _S(",");
const TText * const LCountry::DateSeparator[KMaxDateSeparators] = {_S(""),_S("."),_S("."),_S("")};
const TText * const LCountry::TimeSeparator[KMaxTimeSeparators] = {_S(""),_S(":"),_S(":"),_S("")};
const TLocalePos LCountry::AmPmSymbolPosition = ELocaleAfter;
const TBool LCountry::AmPmSpaceBetween = ETrue;
//const TUint LCountry::DaylightSaving = EDstNone;
const TDaylightSavingZone LCountry::HomeDaylightSavingZone = EDstEuropean;
const TUint LCountry::WorkDays = 0x1f;
const TText * const LCountry::CurrencySymbol = _S("DM");
const TText* const LCountry::ShortDateFormatSpec = _S("%F%*D.%*M.%Y"); // needs checking by a localisation team (this item was added since real localisation - the value given here has been set by a software developer so it may be wrong)
const TText* const LCountry::LongDateFormatSpec = _S("%F%*D%X %N %Y"); // needs checking by a localisation team (this item was added since real localisation - the value given here has been set by a software developer so it may be wrong)
const TText* const LCountry::TimeFormatSpec = _S("%F%H:%T:%S"); // needs checking by a localisation team (this item was added since real localisation - the value given here has been set by a software developer so it may be wrong)
const TFatUtilityFunctions* const LCountry::FatUtilityFunctions = NULL;
const TDay LCountry::StartOfWeek = EMonday;
const TClockFormat LCountry::ClockFormat = EClockDigital;
const TUnitsFormat LCountry::UnitsGeneral = EUnitsMetric;
const TUnitsFormat LCountry::UnitsDistanceShort = EUnitsMetric;
const TUnitsFormat LCountry::UnitsDistanceLong = EUnitsMetric;

// end of LS_CYGE.CPP
