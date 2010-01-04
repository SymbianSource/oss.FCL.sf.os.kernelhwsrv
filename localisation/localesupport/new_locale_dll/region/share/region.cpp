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


#include <ls_std.h>
#include "locl_region.h"

EXPORT_C void LoclRegion::LocaleData(SLocaleRegion *aLocale)
	{
	aLocale->iCountryCode=RegionAspect::CountryCode;
	aLocale->iRegionCode=RegionAspect::RegionCode;
	aLocale->iUniversalTimeOffset=RegionAspect::UniversalTimeOffset;
	aLocale->iDateFormat=RegionAspect::DateFormat;
	aLocale->iTimeFormat=RegionAspect::TimeFormat;
	aLocale->iCurrencySymbolPosition=RegionAspect::CurrencySymbolPosition;
	aLocale->iCurrencySpaceBetween=RegionAspect::CurrencySpaceBetween;
	aLocale->iCurrencyDecimalPlaces=RegionAspect::CurrencyDecimalPlaces;
	aLocale->iNegativeCurrencyFormat=TNegativeCurrencyFormat(RegionAspect::NegativeCurrencyFormat);
	aLocale->iCurrencyTriadsAllowed=RegionAspect::CurrencyTriadsAllowed;
	aLocale->iThousandsSeparator=*RegionAspect::ThousandsSeparator;
	aLocale->iDecimalSeparator=*RegionAspect::DecimalSeparator;
	TInt i=0;
	for(;i<KMaxDateSeparators;i++)
		aLocale->iDateSeparator[i]=*RegionAspect::DateSeparator[i];
	for(i=0;i<KMaxTimeSeparators;i++)
		aLocale->iTimeSeparator[i]=*RegionAspect::TimeSeparator[i];
	aLocale->iAmPmSymbolPosition=RegionAspect::AmPmSymbolPosition;
	aLocale->iAmPmSpaceBetween=RegionAspect::AmPmSpaceBetween;
	aLocale->iHomeDaylightSavingZone=RegionAspect::HomeDaylightSavingZone;
	aLocale->iWorkDays=RegionAspect::WorkDays;
	aLocale->iStartOfWeek=RegionAspect::StartOfWeek;
	aLocale->iClockFormat=RegionAspect::ClockFormat;
	aLocale->iUnitsGeneral=RegionAspect::UnitsGeneral;
	aLocale->iUnitsDistanceLong=RegionAspect::UnitsGeneral;
	aLocale->iUnitsDistanceShort=RegionAspect::UnitsGeneral;
	aLocale->iExtraNegativeCurrencyFormatFlags=RegionAspect::ExtraNegativeCurrencyFormatFlags;
	aLocale->iDeviceTimeState = EDeviceUserTime;
	}

EXPORT_C const TText* LoclRegion::CurrencySymbol()
	{
	return(RegionAspect::CurrencySymbol);
	}
EXPORT_C const TText* LoclRegion::ShortDateFormatSpec()
	{
	return(RegionAspect::ShortDateFormatSpec);
	}
EXPORT_C const TText* LoclRegion::LongDateFormatSpec()
	{
	return(RegionAspect::LongDateFormatSpec);
	}
EXPORT_C const TText* LoclRegion::TimeFormatSpec()
	{
	return(RegionAspect::TimeFormatSpec);
	}
