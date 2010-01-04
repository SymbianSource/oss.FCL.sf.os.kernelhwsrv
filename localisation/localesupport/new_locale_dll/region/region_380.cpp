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

const TInt RegionAspect::CountryCode = 39;
const TInt RegionAspect::UniversalTimeOffset = 3600;
const TDateFormat RegionAspect::DateFormat = EDateEuropean;
const TTimeFormat RegionAspect::TimeFormat = ETime24;
const TLocalePos RegionAspect::CurrencySymbolPosition = ELocaleBefore;
const TBool RegionAspect::CurrencySpaceBetween = EFalse;
const TInt RegionAspect::CurrencyDecimalPlaces = 0;
const TLocale::TNegativeCurrencyFormat RegionAspect::NegativeCurrencyFormat=TLocale::TNegativeCurrencyFormat(0); // replacing CurrencyNegativeInBrackets
const TBool RegionAspect::CurrencyTriadsAllowed = ETrue;
const TText * const RegionAspect::ThousandsSeparator = _S(".");
const TText * const RegionAspect::DecimalSeparator = _S(",");
const TText * const RegionAspect::DateSeparator[KMaxDateSeparators] = {_S(""),_S("/"),_S("/"),_S("")};
const TText * const RegionAspect::TimeSeparator[KMaxTimeSeparators] = {_S(""),_S("."),_S("."),_S("")};
const TLocalePos RegionAspect::AmPmSymbolPosition = ELocaleAfter;
const TBool RegionAspect::AmPmSpaceBetween = ETrue;
const TDaylightSavingZone RegionAspect::HomeDaylightSavingZone = EDstEuropean;
const TUint RegionAspect::WorkDays = 0x1f;
const TText * const RegionAspect::CurrencySymbol = _S("\x20ac");
const TText* const RegionAspect::ShortDateFormatSpec = _S("%F%*D/%*M/%Y"); // needs checking by a localisation team (this item was added since real localisation - the value given here has been set by a software developer so it may be wrong)
const TText* const RegionAspect::LongDateFormatSpec = _S("%F%*D%X %N %Y"); // needs checking by a localisation team (this item was added since real localisation - the value given here has been set by a software developer so it may be wrong)
const TText* const RegionAspect::TimeFormatSpec = _S("%F%H.%T.%S");  // needs checking by a localisation team (this item was added since real localisation - the value given here has been set by a software developer so it may be wrong)
const TDay RegionAspect::StartOfWeek = EMonday;
const TClockFormat RegionAspect::ClockFormat = EClockAnalog;
const TUnitsFormat RegionAspect::UnitsGeneral = EUnitsMetric;
const TUnitsFormat RegionAspect::UnitsDistanceShort = EUnitsMetric;
const TUnitsFormat RegionAspect::UnitsDistanceLong = EUnitsMetric;
const TUint16 RegionAspect::RegionCode = 380;
const TUint RegionAspect::ExtraNegativeCurrencyFormatFlags = 0;

