// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Symbian Foundation License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.symbianfoundation.org/legal/sfl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// Default locale settings for the UK
// This UK locale dll is created only for the purpose of instantiating FatUtilityFunctions
// 
//

#include <kernel/localise.h>
#include "complocl.h"
#include "unicodeconv.h"

const TFatUtilityFunctions myFatUtilityFunctions=
	{
	UnicodeConv::ConvertFromUnicodeL,
	UnicodeConv::ConvertToUnicodeL,
	UnicodeConv::IsLegalShortNameCharacter
	};

// The configuration data
const TLanguage LLocaleData::Language = ELangEnglish;
const TInt LLocaleData::CountryCode = 45;
const TInt LLocaleData::UniversalTimeOffset = 0;
const TDateFormat LLocaleData::DateFormat = EDateEuropean;
const TTimeFormat LLocaleData::TimeFormat = ETime12;
const TLocalePos LLocaleData::CurrencySymbolPosition = ELocaleBefore;
const TBool LLocaleData::CurrencySpaceBetween = EFalse;
const TInt LLocaleData::CurrencyDecimalPlaces = 2; 
const TLocale::TNegativeCurrencyFormat LLocaleData::NegativeCurrencyFormat=TLocale::TNegativeCurrencyFormat(0); // replacing CurrencyNegativeInBrackets
const TBool LLocaleData::CurrencyTriadsAllowed = ETrue;
const TText * const LLocaleData::ThousandsSeparator = _S(",");
const TText * const LLocaleData::DecimalSeparator = _S(".");
const TText * const LLocaleData::DateSeparator[KMaxDateSeparators] = {_S(""),_S("/"),_S("/"),_S("")};
const TText * const LLocaleData::TimeSeparator[KMaxTimeSeparators] = {_S(""),_S(":"),_S(":"),_S("")};
const TLocalePos LLocaleData::AmPmSymbolPosition = ELocaleAfter;
const TBool LLocaleData::AmPmSpaceBetween = ETrue;
//const TUint LLocaleData::DaylightSaving = EDstNone;
const TDaylightSavingZone LLocaleData::HomeDaylightSavingZone = EDstEuropean;
const TUint LLocaleData::WorkDays = 0x1f;
const TText * const LLocaleData::CurrencySymbol = _S("\x00a3");
const TText* const LLocaleData::ShortDateFormatSpec = _S("%F%*D/%*M/%Y"); // needs checking by a localisation team (this item was added since real localisation - the value given here has been set by a software developer so it may be wrong)
const TText* const LLocaleData::LongDateFormatSpec = _S("%F%*D%X %N %Y"); // needs checking by a localisation team (this item was added since real localisation - the value given here has been set by a software developer so it may be wrong)
const TText* const LLocaleData::TimeFormatSpec = _S("%F%*I:%T:%S %*A"); // needs checking by a localisation team (this item was added since real localisation - the value given here has been set by a software developer so it may be wrong)

const TFatUtilityFunctions* const LLocaleData::FatUtilityFunctions = &myFatUtilityFunctions;

const TDay LLocaleData::StartOfWeek = EMonday;
const TClockFormat LLocaleData::ClockFormat = EClockAnalog;
const TUnitsFormat LLocaleData::UnitsGeneral = EUnitsImperial;
const TUnitsFormat LLocaleData::UnitsDistanceShort = EUnitsImperial;
const TUnitsFormat LLocaleData::UnitsDistanceLong = EUnitsImperial;
const TUint LLocaleData::ExtraNegativeCurrencyFormatFlags = 0;
const TLanguage LLocaleData::LanguageDowngrade[3] = {ELangNone, ELangNone, ELangNone};
const TDigitType LLocaleData::DigitType = EDigitTypeWestern;

