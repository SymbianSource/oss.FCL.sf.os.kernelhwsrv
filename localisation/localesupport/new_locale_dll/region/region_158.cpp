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
#include "locl_region.h"

const TInt RegionAspect::CountryCode = 81;
const TInt RegionAspect::UniversalTimeOffset = 9*3600;
const TDateFormat RegionAspect::DateFormat = EDateJapanese;
const TTimeFormat RegionAspect::TimeFormat = ETime12;
const TLocalePos RegionAspect::CurrencySymbolPosition = ELocaleBefore;
const TBool RegionAspect::CurrencySpaceBetween = EFalse;
const TInt RegionAspect::CurrencyDecimalPlaces = 0;
const TLocale::TNegativeCurrencyFormat RegionAspect::NegativeCurrencyFormat=TLocale::TNegativeCurrencyFormat(0); // replacing CurrencyNegativeInBrackets
const TBool RegionAspect::CurrencyTriadsAllowed = ETrue;
const TText * const RegionAspect::ThousandsSeparator = _S(",");
const TText * const RegionAspect::DecimalSeparator = _S(".");
const TText * const RegionAspect::DateSeparator[KMaxDateSeparators] = {_S(""),_S("/"),_S("/"),_S("")};
const TText * const RegionAspect::TimeSeparator[KMaxTimeSeparators] = {_S(""),_S(":"),_S(":"),_S("")};
const TLocalePos RegionAspect::AmPmSymbolPosition = ELocaleBefore;
const TBool RegionAspect::AmPmSpaceBetween = EFalse;
const TDaylightSavingZone RegionAspect::HomeDaylightSavingZone = EDstNone;
const TUint RegionAspect::WorkDays = 0x1f;
const TText * const RegionAspect::CurrencySymbol = _S("\x00a5");
const TText* const RegionAspect::ShortDateFormatSpec = _S("%F%*Y/%*M/%D"); // needs checking by a localisation team (this item was added since real localisation - the value given here has been set by a software developer so it may be wrong)
const TText* const RegionAspect::LongDateFormatSpec = _S("%F%Y\x5e74%*M\x6708%*D\x65e5");  // needs checking by a localisation team (this item was added since real localisation - the value given here has been set by a software developer so it may be wrong)
const TText* const RegionAspect::TimeFormatSpec = _S("%F%*I:%T:%S %*A"); // needs checking by a localisation team (this item was added since real localisation - the value given here has been set by a software developer so it may be wrong)
const TDay RegionAspect::StartOfWeek = EMonday;
const TClockFormat RegionAspect::ClockFormat = EClockAnalog;
const TUnitsFormat RegionAspect::UnitsGeneral = EUnitsMetric;
const TUnitsFormat RegionAspect::UnitsDistanceShort = EUnitsMetric;
const TUnitsFormat RegionAspect::UnitsDistanceLong = EUnitsMetric;
const TUint16 RegionAspect::RegionCode = 158;
const TUint RegionAspect::ExtraNegativeCurrencyFormatFlags = 0;

