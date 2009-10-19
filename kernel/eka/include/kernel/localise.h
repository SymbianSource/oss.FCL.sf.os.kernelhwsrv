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
// e32\include\kernel\localise.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __K32LOCL_H__
#define __K32LOCL_H__
#include <u32std.h>

class TDesC16;
class TDes16;

#ifdef _UNICODE
#define TLocaleText TText16
#else
#define TLocaleText TText8
#endif

/**
It is a container for functions to convert from a 8-bit string to a 16-bit(UNICODE) string and vice-versa, and to check
whether a character is a legal short character or not.
It declares pointers to functions that accomplishes the above tasks.  

The functions are to be implemented by the locale-DLLs.
*/
struct TFatUtilityFunctions // functions to be implemented by locale-DLLs
	{
	/**
	Specifies the action to be taken if an overflow occurs. It can either Leave or Truncate the overflow part.
	*/
	enum TOverflowAction
		{
		/**
		Will leave if an overflow occurs.
		*/
		EOverflowActionLeave,
		/** 
		Will truncate the data if an overflow occurs.
		*/
		EOverflowActionTruncate
		};
	/**
	Function to convert a string from unicode(16-bit)format to a (8-bit) format.
	
	@param  aForeign  										8-bit descriptor that will contain the converted string.
	@param  aUnicode  										16-bit descriptor which contains the string to be converted.
	@param  aReplacementForUnconvertibleUnicodeCharacters	Any default 8-bit character that will replace any non convertible 16-bit unicode character.
	@param  aOverFlowAction									Enum value specifying action to be taken in case of overflow.
	
	@see TOverflowAction.	
	*/
	typedef void (*TConvertFromUnicodeL)(TDes8& aForeign, const TDesC16& aUnicode, const TDesC8& aReplacementForUnconvertibleUnicodeCharacters, TOverflowAction aOverflowAction);
	
	/**
	Function to convert a string from (8-bit) format to unicode(16-bit)format.
	
	@param  aUnicode  										16-bit descriptor which will contain the converted string.
	@param  aForeign  										8-bit descriptor which contains the string to be converted.
	@param  aOverFlowAction									Enum value specifying action to be taken in case of overflow. 
	
	@see TOverflowAction.
	*/
	typedef void (*TConvertToUnicodeL)(TDes16& aUnicode, const TDesC8& aForeign, TOverflowAction aOverflowAction);
	
	/**
	Function to check whether a character is a legal short name character or not.
	
	@param aCharacter The character to apply the check on.
	
	@return TBool True  If it is a legal short name character
				  False otherwise.
	*/
	typedef TBool (*TIsLegalShortNameCharacter)(TUint aCharacter);
	
	/** 
	A pointer to a function TConvertFromUnicodeL. This is one of the pointers returned when Locl::FatUtilityFunctions is called.
	
	@see TConvertFromUnicodeL.
	*/
	TConvertFromUnicodeL iConvertFromUnicodeL;
	
	/** 
	A pointer to a function TConvertToUnicodeL. This is one of the pointers returned when Locl::FatUtilityFunctions is called.
	
	@see TConvertToUnicodeL.
	*/
	TConvertToUnicodeL iConvertToUnicodeL;
	
	/** 
	A pointer to a function TIsLegalShortNameCharacter. This is one of the pointers returned when Locl::FatUtilityFunctions is called.
	
	@see TIsLegalShortNameCharacter.
	*/
	TIsLegalShortNameCharacter iIsLegalShortNameCharacter;
	};

/**
A data structure containing the system's locale settings.
The data must be identical to that in TLocale.
*/
struct SLocaleData
	{
	/** 
	Integer value specifying country code.
	The country code is the code used as the international dialling prefix. 
	This code is also used to identify a country by the dialling software.
	*/
	TInt iCountryCode;
	
	/**
	The locale's universal time offset. Offset in seconds from universal time.
	Time zones east of universal time have positive offsets. 
	Time zones west of universal time have negative offsets.
	*/
	TInt iUniversalTimeOffset;
	
	/**
	The date format of the Locale. It can be either of the three formats American,European or Japanese.
    */
	TDateFormat iDateFormat;
	
	/**
	The time formats as either 12 hour or 24 hour.
	*/ 
    TTimeFormat iTimeFormat;
    
    /**
    The currency symbol is located before or after the currency amount.
    */
	TLocalePos iCurrencySymbolPosition;
	
	/**
	Whether or not a space is inserted between the currency symbol and the currency value.
	True if a space exists, False otherwise.
    */
	TBool iCurrencySpaceBetween;
	
	/** The number of decimal places to which currency values are set.*/
	TInt iCurrencyDecimalPlaces;
	
	/**
	Indicates how negative currency values are formatted. 
	*/
	TNegativeCurrencyFormat iNegativeCurrencyFormat;
	
	/**
	Sets whether triads are allowed in currency values.
	True if Triads are allowed, False otherwise.
	*/
	TBool iCurrencyTriadsAllowed;
	
	/**
	The character to be used to separate groups of three digits to the left of the decimal separator.
	A thousands separator character is only displayed in currency values if currency triads are allowed.
	*/
	TChar iThousandsSeparator;
	
	/**
	The character used to separate a whole number from its fractional part.
	*/
	TChar iDecimalSeparator;
	
	/**
	An array containing the four characters used to separate the day, month and year components of the date.
	If the four separators are represented by S0, S1, S2 and S3 
	and the three date components are represented by XX, YY and ZZ,
	then the separators are located: S0 XX S1 YY S2 ZZ S3.
    */
	TChar iDateSeparator[KMaxDateSeparators];
	
	/**
    An array containing the four characters used to separate the hour, second and minute components of the time.
	If the four separators are represented by S0, S1, S2 and S3 
	and the three time components are represented by XX, YY and ZZ, 
	then the separators are located: S0 XX S1 YY S2 ZZ S3.
	*/
	TChar iTimeSeparator[KMaxTimeSeparators];
	
	/**
    Defines whether the am/pm text is located before or after the time.
    */
	TLocalePos iAmPmSymbolPosition;
	
	/**
	Whether or not a space is inserted between the time and the preceding or trailing am/pm text.
	True if a space exists, False otherwise.
	*/
	TBool iAmPmSpaceBetween;
	
	/**
	The zones in which daylight saving is in effect.

	If daylight saving is in effect, one hour is added to the time.

	A bit mask in which the three least significant bits are defined, 
	indicating which of the three daylight saving zones are adjusted for daylight saving. 
	These bits represent: Northern (non-European countries in the northern hemisphere), 
						  Southern (southern hemisphere), 
						  and European. see TDaylightSavingZone.
						  
	@see TDaylightSavingZone.
	*/
	TUint iDaylightSaving;
	
	/**
	The daylight saving zone in which the home city is located.
	*/
	TDaylightSavingZone iHomeDaylightSavingZone;
	
	/**
	A bit mask representing the days of the week which are considered as working days.
	
	A bit mask of seven bits indicating (by being set) which days are workdays. 
	The least significant bit corresponds to Monday, the next bit to Tuesday and so on. 
	*/
	TUint iWorkDays;
	
	/**
	The day which is considered to be the first day of the week.
		
	The enumerator symbol names correspond with the days of the week, i.e. EMonday refers to Monday etc. 
	*/
	TDay iStartOfWeek;
	
	/**
	The clock display format as either analog or digital. 
	*/
	TClockFormat iClockFormat;
	
	/**
	
	The general units of measurement.

	This should be used when both short and long distances use the same units of measurement.
    */
	TUnitsFormat iUnitsGeneral;
	
	/**
	The units of measurement for short distances.

	Short distances are those which would normally be represented by either metres and centimetres or feet and inches.
	*/
	TUnitsFormat iUnitsDistanceShort;
	
	/**
	The units of measurement for long distances.

	Long distances are those which would normally be represented by either miles or kilometres.
	*/
	TUnitsFormat iUnitsDistanceLong;
	
	/**
	Flags for negative currency values formatting.

	EFlagNegativeLoseSpace 			  		If this flag is set and the currency value being formatted is negative,
						 					if there is a space between the currency symbol and the value, that space is lost.
 
	EFlagNegativeCurrencySymbolOpposite 	If this flag is set and the currency value being formatted is negative, 
											the position of the currency symbol is placed in the opposite direction 
											from the position set for the positive currency value.
 	*/
	TUint iExtraNegativeCurrencyFormatFlags;
	
	
	/**
	An array which contains customisable part of the language downgrade path.
	*/
	TUint16 iLanguageDowngrade[3];
	
	TUint16 iSpare16;
	
	/**
	The number mode stored in the locale.
	*/
	TDigitType iDigitType;
	
	/**
	The device time state.
	*/
 	TDeviceTimeState iDeviceTimeState;
 	
 	TInt iSpare[0x1E];
	};

/**
An interface defined to provide support for localisation for components 
that are too low-level to participate in the normal EPOC localisation 
mechanisms.
*/
class Locl
	{
public:
	/**
	Returns the Language type.
	
	@return The value corresponding to a particular language as specified in TLanguage.
	*/
	IMPORT_C static TLanguage Language();
	
	/**
	Returns whether it is a Unicode Build or not.
	
	@return True 	If it is a Unicode Build.
	@return False	Otherwise.
	*/
	IMPORT_C static TBool UniCode();
	
	/**
	Create the Localisation Table.
	
	@param aLocale a pointer to a structure of type SLocaleData. 
	*/
	IMPORT_C static void LocaleData(SLocaleData *aLocale);
	
	/**
	Returns the address of the Currency Symbol.
	
	@return const TText16 * Address of the Currency Symbol.
	*/
	IMPORT_C static const TLocaleText* CurrencySymbol();
	
	/**
	Returns the address of the short date format.
	
	@return  const TText16 * Address of the Short date format.
	*/
	IMPORT_C static const TLocaleText* ShortDateFormatSpec();
	
	/**
	Returns the address of the long date format.
	
	@return  const TText16 * Address of the Long date format. 
	*/
	IMPORT_C static const TLocaleText* LongDateFormatSpec();
	
	/**
	Returns the address of the time format.
	
	@return  const TText16 * Address of the time format. 
	*/
	IMPORT_C static const TLocaleText* TimeFormatSpec();
	
	/**
	Returns the addresses of the FAT utility functions.
	
	@return  const TFatUtilityFunctions * Addresses of the FAT utility functions.
	*/
	IMPORT_C static const TFatUtilityFunctions* FatUtilityFunctions();
	
	/**
	Returns the address of the data suffix table.
	
	@return  const TText16 * Address of the Date Suffix Table.
	*/
	IMPORT_C static const TLocaleText* const *DateSuffixTable();
	
	/**
	Returns the address of the day table.
	
	@return  const TText16 * Address of the Day table.
	*/
	IMPORT_C static const TLocaleText* const *DayTable();
	
	/**
	Returns the address of the abbreviated day table.
	
	@return  const TText16 * Address of the abbreviated day table.
	*/
	IMPORT_C static const TLocaleText* const *DayAbbTable();
	
	/**
	Returns the address of the month table.
	
	@return  const TText16 * Address of the month table.
	*/
	IMPORT_C static const TLocaleText* const *MonthTable();
	
	/**
	Returns the address of the abbreviated month table.
	
	@return  const TText16 * Address of the abbreviated month table.
	*/
	IMPORT_C static const TLocaleText* const *MonthAbbTable();
	
	/**
	Returns the address of the ampm table.
	
	@return  const TText16 * Address of the ampm table.
	*/
	IMPORT_C static const TLocaleText* const *AmPmTable();
	
	/**
	Returns the address of the message table.
	
	@return  const TText16 * Address of the message table.
	*/
	IMPORT_C static const TLocaleText* const *MsgTable();
	
	/**
	Returns the address of the locale character set object: contains collation rules etc.
	
	@return	 const LCharSet * Address of the locale character set if its a UNICODE build; NULL otherwise.
	*/
	IMPORT_C static const LCharSet *CharSet();
	
	/**
	Returns the address of the character type conversion table.
	
	@return  const TUint8 * Address of the character type conversion table if its a NON-UNICODE build;NULL otherwise
	*/
	IMPORT_C static const TUint8 *TypeTable();
	
	/**
	Returns the address of the uppercase table.
	
	@return  const TText16 * Address of the uppercase table if its a NON-UNICODE build;NULL otherwise
	
	*/
	IMPORT_C static const TLocaleText* UpperTable();
	
	/**
	Returns the address of the lowercase table.
	
	@return  const TText16 * Address of the lowercase table if its a NON-UNICODE build;NULL otherwise

	
	*/
	IMPORT_C static const TLocaleText* LowerTable();
	
	/**
	Returns the address of the fold table.
	
	@return  const TText16 * Address of the fold table if its a NON-UNICODE build;NULL otherwise
	
	*/
	IMPORT_C static const TLocaleText* FoldTable();
	
	/**
	Returns the address of the collate table.
	
	@return  const TText16 * Address of the fold table if its a NON-UNICODE build;NULL otherwise
	
	*/
	IMPORT_C static const TLocaleText* CollTable();
	};

/**
@internalTechnology
@deprecated
*/
class LCountry
	{
public:
	static const TLanguage Language;
	static const TInt CountryCode;
	static const TInt UniversalTimeOffset;
	static const TDateFormat DateFormat;
	static const TTimeFormat TimeFormat;
	static const TLocaleText * const CurrencySymbol;
	static const TLocalePos CurrencySymbolPosition;
	static const TBool CurrencySpaceBetween;
	static const TInt CurrencyDecimalPlaces;
	static const TBool CurrencyNegativeInBrackets;
	static const TBool CurrencyTriadsAllowed;
	static const TLocaleText* const ShortDateFormatSpec;
	static const TLocaleText* const LongDateFormatSpec;
	static const TLocaleText* const TimeFormatSpec;
	static const TFatUtilityFunctions* const FatUtilityFunctions;
	static const TLocaleText * const ThousandsSeparator;
	static const TLocaleText * const DecimalSeparator;
	static const TLocaleText * const DateSeparator[KMaxDateSeparators];
	static const TLocaleText * const TimeSeparator[KMaxTimeSeparators];
	static const TLocalePos AmPmSymbolPosition;
	static const TBool AmPmSpaceBetween;
//	static const TUint DaylightSaving;
	static const TDaylightSavingZone HomeDaylightSavingZone;
	static const TUint WorkDays;
	static const TDay StartOfWeek;
	static const TClockFormat ClockFormat;
	static const TUnitsFormat UnitsGeneral;
	static const TUnitsFormat UnitsDistanceLong;
	static const TUnitsFormat UnitsDistanceShort;
	};


/**
Settings for the locale language. They are some text tables of 
day names, month names etc.
*/
class LLanguage
	{
public:
	/** Text table containing the suffix strings of all days in a month. */
	static const TLocaleText * const DateSuffixTable[KMaxSuffixes];
	/** Text table containing the names of days in a week. */
	static const TLocaleText * const DayTable[KMaxDays];
	/** Text table containing the abbreviated names of days in a week. */
	static const TLocaleText * const DayAbbTable[KMaxDays];
	/** Text table containing the month names. */
	static const TLocaleText * const MonthTable[KMaxMonths];
	/** Text table containing the abbreviated month names. */
	static const TLocaleText * const MonthAbbTable[KMaxMonths];
	/** Text table containing the am/pm strings. */
	static const TLocaleText * const AmPmTable[KMaxAmPms];
	};

class LMessages
	{
public:
	/** Text table containing the default messages for File Server, Sound Driver, and Media Drivers. */
	static const TLocaleText * const MsgTable[ELocaleMessages_LastMsg];
	};

/*
 * The LAlphabet class has been abolished in the Unicode build.
 * Locale-specific character set information is kept in a unique LCharSet
 * structure (defined in U32STD.H; used by the Exec, Locl, and K classes).
 * The reason for having an actual object (rather than a class with no
 * instances but with static members) is so that its address can be
 * returned by the new Exec function GetLocaleCharSet().
 */
extern const LCharSet TheCharSet;	// the one and only LCharSet object

#undef TLocaleText

#endif
