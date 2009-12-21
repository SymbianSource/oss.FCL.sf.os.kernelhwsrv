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


#if !defined __LOCL_REGION_H__
#define __LOCL_REGION_H__

#if !defined(__E32STD_H__)
#include <E32std.h>
#endif

#ifdef _UNICODE
#define TLocaleText TText16
#else
#define TLocaleText TText8
#endif

class RegionAspect
	{
	public:
	static const TInt CountryCode;  //telephone country code
	static const TUint16 RegionCode;		//ISO3166 country code
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
	};

struct SLocaleRegion
	{
	TInt iCountryCode;
	TInt iUniversalTimeOffset;
	TDateFormat iDateFormat;
    TTimeFormat iTimeFormat;
	TLocalePos iCurrencySymbolPosition;
	TBool iCurrencySpaceBetween;
	TInt iCurrencyDecimalPlaces;
	TNegativeCurrencyFormat iNegativeCurrencyFormat;
	TBool iCurrencyTriadsAllowed;
	TChar iThousandsSeparator;
	TChar iDecimalSeparator;
	TChar iDateSeparator[KMaxDateSeparators];
	TChar iTimeSeparator[KMaxTimeSeparators];
	TLocalePos iAmPmSymbolPosition;
	TBool iAmPmSpaceBetween;
	TUint iDaylightSaving;
	TDaylightSavingZone iHomeDaylightSavingZone;
	TUint iWorkDays;
	TDay iStartOfWeek;
	TClockFormat iClockFormat;
	TUnitsFormat iUnitsGeneral;
	TUnitsFormat iUnitsDistanceShort;
	TUnitsFormat iUnitsDistanceLong;
	TUint iExtraNegativeCurrencyFormatFlags;
	TUint16 iSpare16v1[3];   //used for language downgrade and 1 spare, but deleted now
	TUint16 iRegionCode;
	TUint16 iSpare16v2[2];    //used for digit type
	TDeviceTimeState iDeviceTimeState; 	
	TInt iSpare[0x1E];
	
	};

class LoclRegion
	{
public:
	IMPORT_C static void LocaleData(SLocaleRegion *aLocale);
	IMPORT_C static const TLocaleText* CurrencySymbol();
	IMPORT_C static const TLocaleText* ShortDateFormatSpec();
	IMPORT_C static const TLocaleText* LongDateFormatSpec();
	IMPORT_C static const TLocaleText* TimeFormatSpec();
	};

#endif  /* __LOCL_REGION_H__ */


