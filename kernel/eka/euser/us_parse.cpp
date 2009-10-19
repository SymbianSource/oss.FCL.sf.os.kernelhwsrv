// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\us_parse.cpp
// 
//

#include "us_std.h"

class TStringToDateTime
	{
public:
	TStringToDateTime(const TDesC& aDes,TInt aCenturyOffset);
	TInt Parse(TTime& aTime);
	enum {ETimePresent=1,EDatePresent};
private:
// tokens
	enum {EDec=-12,ENov,EOct,ESep,EAug,EJul,EJun,EMay,EApr,EMar,EFeb,EJan};
	enum {ETokenAm=-19,ETokenPm};
	enum TDateSeparators{ESlash=-39,EDash,EComma,ESpace,EDateLocale1,EDateLocale2};
	enum TTimeSeparators{EColon=-49,EDot,ETimeLocale1,ETimeLocale2};
	enum TDecimalSeparators{EDecimalLocale=-59};
	enum {EErrorToken=-99,ENullToken=-100};
//
	enum {ENumberOfDateSep=6,ENumberOfTimeSep=4,EMaxTokens=27};
	enum {EFirstDateSep=ESlash,ELastDateSep=EDateLocale2,EFirstTimeSep=EColon,ELastTimeSep=ETimeLocale2};
private:
	TInt NextToken(TInt& aTokenLen);
	void StripSpaceTokens();
	TInt CrackTokenFormula();
	TInt GetDate(TInt aFormulaPos,TInt& aTokenCount);
	TInt GetTime(TInt aFormulaPos,TInt& aTokenCount);
//	
	TInt GetSeparatorToken(TChar aChar) const;	
	TBool IsTimeSeparator(TChar  aChar) const;
	TBool IsDateSeparator(TChar  aChar) const;
	TBool IsDecimalSeparator(TChar  aChar) const;
	TBool IsSeparator(TChar aChar) const;
	TBool IsSeparator(TInt aToken) const;
	inline TBool IsTimeSeparator(TInt aToken) const;
	inline TBool IsDateSeparator(TInt aToken) const;
	TBool IsDecimalSeparator(TInt aToken) const;
	inline TBool IsAmPm(TInt aToken) const;
	inline TBool IsAlphaMonth(TInt aToken) const;
private:
	TLex iLex;
	TInt iCenturyOffset;
	TDateFormat iDateFormat;
	TChar iTimeSepChars[ENumberOfTimeSep];
	TChar iDateSepChars[ENumberOfDateSep];
	TChar iDecSepChar;
	TDateTime iDateTime;
	TInt iCount;
	TInt iFormula[EMaxTokens];// 27 max possible with valid des (including spaces):" 10 : 00 : 00 . 000000 pm 6 / 12 / 99 "
	TUint8 iTokenLen[EMaxTokens];
	};

inline TBool TStringToDateTime::IsTimeSeparator(TInt aToken) const
	{
	return(aToken >= EFirstTimeSep && aToken <=ELastTimeSep);
	}
inline TBool TStringToDateTime::IsDateSeparator(TInt aToken) const
	{
	return(aToken>=EFirstDateSep && aToken<=ELastDateSep);
	}
inline TBool TStringToDateTime::IsAmPm(TInt aToken) const
	{
	return(aToken==ETokenAm || aToken==ETokenPm);
	}
inline TBool TStringToDateTime::IsAlphaMonth(TInt aToken) const
	{
	return (aToken>=EDec && aToken<=EJan);
	}

inline TBool TStringToDateTime::IsDecimalSeparator(TChar aChar) const
	{
	return(aChar==iDecSepChar);
	}
inline TBool TStringToDateTime::IsDecimalSeparator(TInt aToken) const
	{
	return(aToken==EDecimalLocale);
	}

TStringToDateTime::TStringToDateTime(const TDesC& aDes,TInt aCenturyOffset)
	: iLex(aDes),iCenturyOffset(aCenturyOffset),iDateTime(0,EJanuary,0,0,0,0,0)
	{

	__ASSERT_ALWAYS(aCenturyOffset>=0 && aCenturyOffset<100,Panic(ETTimeValueOutOfRange));
	TLocale locale;
	iDateFormat=locale.DateFormat();
	
	iTimeSepChars[0]=':';
	iTimeSepChars[1]='.';
	iTimeSepChars[2]=locale.TimeSeparator(1);
	iTimeSepChars[3]=locale.TimeSeparator(2);

	iDateSepChars[0]='/';
	iDateSepChars[1]='-';
	iDateSepChars[2]=',';
	iDateSepChars[3]=' ';
	iDateSepChars[4]=locale.DateSeparator(1);
	iDateSepChars[5]=locale.DateSeparator(2);
	iDecSepChar = locale.DecimalSeparator();
	}

TBool TStringToDateTime::IsTimeSeparator(TChar aChar) const
	{

	for (TInt ii=0;ii<ENumberOfTimeSep;++ii)
		if (aChar==iTimeSepChars[ii])
			return ETrue;
	return(EFalse);
	}

TBool TStringToDateTime::IsDateSeparator(TChar aChar) const
	{

	for (TInt ii=0;ii<ENumberOfDateSep;++ii)
		if (aChar==iDateSepChars[ii])
			return ETrue;
	return(EFalse);
	}

TBool TStringToDateTime::IsSeparator(TChar  aChar) const
	{

	return(IsTimeSeparator(aChar) || IsDateSeparator(aChar) || IsDecimalSeparator(aChar));
	}

TBool TStringToDateTime::IsSeparator(TInt aToken) const
	{

	return(IsTimeSeparator(aToken) || IsDateSeparator(aToken));
	}

TInt TStringToDateTime::GetSeparatorToken(TChar aChar) const
	{

	TInt ii=0;
	for (ii=0;ii<ENumberOfDateSep;++ii)
		if (aChar == iDateSepChars[ii])
			return(EFirstDateSep+ii);
	for (ii=0;ii<ENumberOfTimeSep;++ii)
		if (aChar == iTimeSepChars[ii])
			return(EFirstTimeSep+ii);
	if (aChar == iDecSepChar)
		return(EDecimalLocale);
	return(ENullToken);
	}

void TStringToDateTime::StripSpaceTokens()
// Removes excess space tokens from the formula
// The end of the formula is marked with a Null token
	{

	TInt t = 0;
	for (TInt s = 0 ; s < iCount ; ++s)
		{
		if (iFormula[s]==ESpace &&
			(IsSeparator(iFormula[s-1]) || s == iCount-1 || IsSeparator(iFormula[s+1]) || IsAmPm(iFormula[s+1])))
			continue;// Skip unwanted space token
		iFormula[t]=iFormula[s];
		iTokenLen[t]=iTokenLen[s];
		++t;
		}
	iCount=t;
	iFormula[t]=ENullToken;
	}

TInt TStringToDateTime::CrackTokenFormula()
	{

	if (iCount==0)
		return KErrArgument;// Nothing to read
	TInt token0=iFormula[0];
	TInt token1=iFormula[1];
	TInt numberOfTokens;
	TInt dummy=0;
	TInt error;
	if (IsDateSeparator(token1) || IsAlphaMonth(token0))
		{// Assume formula is a Date or DateTime
		if ((error=GetDate(0,numberOfTokens))!=EDatePresent)
			return error;
		numberOfTokens+=1;// Space char between the Date & Time
		return(GetTime(numberOfTokens,dummy));
		}
	else if (IsTimeSeparator(token1) || IsAmPm(token1))
		{// Assume formula is a Time or TimeDate
		if ((error=GetTime(0,numberOfTokens))!=ETimePresent)
			return error;
		numberOfTokens+=1;// Space char between the Time & Date
		return(GetDate(numberOfTokens,dummy));
		}
	else
		return(KErrArgument);
	}

TInt TStringToDateTime::GetDate(TInt aOffset,TInt& aTokenCount)
	// if aOffset == 0  then Date or DateTime format
	// if aOffset != 0  then TimeDate format
	{

	TInt relativeCount=iCount;
	if (aOffset!=0)// aFormat==ETimeDate
		{
		relativeCount-=aOffset;
		if (relativeCount<=-1)
			return(ETimePresent);
		}
	TInt numberOfDateFields=0;
	if (relativeCount==3)
		numberOfDateFields=2;
	else if (relativeCount==5)
		numberOfDateFields=3;
	else if (aOffset==0)
		{// DateTime
		if (IsTimeSeparator(iFormula[5]) || IsAmPm(iFormula[5]))
			numberOfDateFields=2;
		else
			numberOfDateFields=3;
		}
	else// (aOffset!=0)
		{// Date
		if (relativeCount==3)
			numberOfDateFields=2;
		else if (relativeCount==5)
			numberOfDateFields=3;
		else
			return(KErrArgument);
		}

	if (!IsDateSeparator(iFormula[1+aOffset])) 
		return(KErrArgument);
	if (numberOfDateFields==2)
		{
		if (aOffset!=0 && relativeCount!=3)// ie TimeDate
			return(KErrArgument);
		}
	if (numberOfDateFields==3)
		{
		if (aOffset!=0 && relativeCount!=5)// ie TimeDate
			return(KErrArgument);
		if (!IsDateSeparator(iFormula[3+aOffset]))
			return(KErrArgument);
		}

	// A month will always be in the first two fields // DMY MDY YMD
	TBool alphaMonth=(IsAlphaMonth(iFormula[0+aOffset]) || IsAlphaMonth(iFormula[2+aOffset]) );
	
	TInt dayIndex;
	TInt monthIndex;
	TInt yearIndex=4;// Reset if Japanese

	if (iDateFormat==EDateJapanese)
		{// 1996 feb 3
		if (numberOfDateFields==2)
			{
			monthIndex=0;
			dayIndex=2;
			}
		else
			{
			yearIndex=0;
			monthIndex=2;
			dayIndex=4;
			}
		}
	else if (IsAlphaMonth(iFormula[0+aOffset])
		|| (!alphaMonth && iDateFormat==EDateAmerican))// Amer Euro 
		{// feb 3 1996 valid Amer or Euro format // 2 3 1996 Amer
		monthIndex=0;
		dayIndex=2;
		}		
	else
		{// 3 feb 1996 valid Amer or Euro format // 3 2 1996 Euro
		__ASSERT_DEBUG(
			IsAlphaMonth(iFormula[2+aOffset]) || 
			(!alphaMonth && iDateFormat==EDateEuropean),User::Invariant());
		monthIndex=2;
		dayIndex=0;
		}
	
	TTime timeNow;
	timeNow.HomeTime();
	TDateTime now=timeNow.DateTime();
	TInt currentCentury=((now.Year()/100)*100);// Integer arithmetic
	TInt currentTwoDigitYear=now.Year()-currentCentury;

	TInt year=0;
	if (numberOfDateFields==3)// then year value exists
		{
		year=iFormula[yearIndex+aOffset];
		if (year<0)// ie a token has been returned as a year
			return(KErrArgument);
		else if (iTokenLen[yearIndex+aOffset]<=2)
			{
			if (currentTwoDigitYear>=iCenturyOffset)
				{
				if (year>=00 && year<iCenturyOffset) 
					year+=currentCentury+100;// next century
				else
					year+=currentCentury;
				}
			else
				{
				if (year>=00 && year<iCenturyOffset) 
					year+=currentCentury;
				else
					year+=currentCentury-100;// last century
				}
			}
		}

	TInt month=iFormula[monthIndex+aOffset];
	if (IsAlphaMonth(month))
		month=-month;// alphaMonth is -ve enum token
	month-=1;// months start at zero

	TInt error;// Set Year, Month and Day
	if ((error=iDateTime.SetYear(year))==KErrNone)
		if ((error=iDateTime.SetMonth((TMonth)month))==KErrNone)
			error=iDateTime.SetDay(iFormula[dayIndex+aOffset]-1);
	if (error!=KErrNone)
		return(error);


	if (numberOfDateFields==2)
		aTokenCount=3;
	else if (numberOfDateFields==3)
		aTokenCount=5;
	
	if (aOffset!=0)
		return(EDatePresent|ETimePresent);
	return(EDatePresent);
	}

TInt TStringToDateTime::GetTime(TInt aOffset,TInt& aTokenCount)
	// aFormulaPos == 0  Time format or TimeDate format with Time first
	// aFormulaPos != 0  Date preceeds Time i.e. DateTime format
	// 7 formats 10:00:00.012345 // 10:00:00.012345pm // 10:00:00pm // 10:00:00 // 10:00pm // 10:00 // 10pm
	// offset and relativeCount allow this function to check times 
	// when both the Time(10:00pm) and DateTime(3-feb-69 10:00pm) formats are used.
	{

	TInt relativeCount=iCount;
	if (aOffset!=0)// DateTime // else format==Time
		{
		relativeCount-=aOffset;
		if (relativeCount<=-1)
			return(EDatePresent);
		}
	TInt fields=0;

	if (IsTimeSeparator(iFormula[1+aOffset]) && IsTimeSeparator(iFormula[3+aOffset])&& 
		 (IsTimeSeparator(iFormula[5+aOffset]) || IsDecimalSeparator(iFormula[5+aOffset])))
		{
		fields=4;// 10:00:00.000000 (am)
		aTokenCount=7; 
		if (IsAmPm(iFormula[7+aOffset]))
			aTokenCount+=1;
		}
	else if (IsTimeSeparator(iFormula[1+aOffset]) && IsTimeSeparator(iFormula[3+aOffset]))
		{
		fields=3;// 10:00:00 (am)
		aTokenCount=5; 
		if (IsAmPm(iFormula[5+aOffset]))
			aTokenCount+=1;
		}
	else if (IsTimeSeparator(iFormula[1+aOffset]))
		{
		fields=2;// 10:00 (am)
		aTokenCount=3;
		if (IsAmPm(iFormula[3+aOffset]))
			aTokenCount+=1;
		}
	else if (IsAmPm(iFormula[1+aOffset]))
		{
		fields=1;// 10am
		aTokenCount=2;
		}
	if (fields==0 || (fields==4 && relativeCount==6) || (fields==3 && relativeCount==4) || (fields==2 && relativeCount==2))
		return(KErrArgument);// Colon\DecimalPoint in wrong place 10:00:00. || 10:00: || 10:
	
	TInt error;
	if ((error=iDateTime.SetHour(iFormula[0+aOffset]))!=KErrNone)
		return error;
	if (fields==2)
		error=iDateTime.SetMinute(iFormula[2+aOffset]);
	else if (fields==3)
		{
		if ((error=iDateTime.SetMinute(iFormula[2+aOffset]))==KErrNone)
			error=iDateTime.SetSecond(iFormula[4+aOffset]);
		}
	else if (fields==4)
		{
		if ((error=iDateTime.SetMinute(iFormula[2+aOffset]))==KErrNone)
			 if ((error=iDateTime.SetSecond(iFormula[4+aOffset]))==KErrNone)
				 error = iDateTime.SetMicroSecond(iFormula[6+aOffset]);
		}
	if (error!=KErrNone)
		return(error);

	TInt ampmIndex=2*fields-1;
	if (iFormula[ampmIndex+aOffset]==ETokenAm && iDateTime.Hour()==12)
		error=iDateTime.SetHour(00);// 12am->00 hrs. Ignore 13am
	else if (iFormula[ampmIndex+aOffset]==ETokenPm && iDateTime.Hour()<12)
		error=iDateTime.SetHour(iDateTime.Hour()+12);
	if (error!=KErrNone)
		return(error);

	if (aOffset!=0)
		return(ETimePresent|EDatePresent);
	return(ETimePresent);
	}

TInt TStringToDateTime::NextToken(TInt& aTokenLen)
	{
	if (iLex.Eos())
		return ENullToken;

	TChar ch=iLex.Peek();

	if (ch.IsDigit())
		{
		iLex.Mark();
		do iLex.Inc(); while (iLex.Peek().IsDigit());

		TPtrC des=iLex.MarkedToken();

		TInt digit;
		TLex lex(des);
		if (lex.Val(digit)!=KErrNone)
			return(EErrorToken);
		aTokenLen = des.Length();
		return(digit);
		}
	else if (IsSeparator(ch))
		{
		iLex.Inc();
		iLex.SkipSpace();
		aTokenLen = 1;
		return(GetSeparatorToken(ch));
		}
	else
		{
		iLex.Mark();
		do iLex.Inc(); while (iLex.Peek().IsAlpha() || iLex.Peek().IsDigit());

		TPtrC des=iLex.MarkedToken();
		aTokenLen = des.Length();

		for (TInt month=EJanuary; month<=EDecember; ++month)
			{
			// Abbreviated month name
			TMonthNameAbb nameAbb((TMonth)month);
			if (nameAbb.CompareF(des)==0)
				return(-(month+1)); // All values negative

			// Full month name
			TMonthName name((TMonth)month);
			if (name.CompareF(des)==0)
				return(-(month+1)); // All values negative
			}

		// Substring of am or pm
		TAmPmName am(EAm);
		TAmPmName pm(EPm);
			
		if (am.FindF(des)==0)
			return(ETokenAm);
		else if (pm.FindF(des)==0)
			return(ETokenPm);
		
		return(EErrorToken);
		}
	}


TInt TStringToDateTime::Parse(TTime& aTime)
	{

	iLex.SkipSpace();
	TInt i = 0;
	for (;;)
		{
		if (i==EMaxTokens-1)	// space left to append NullToken
			return KErrArgument;
		TInt len;
		TInt token=NextToken(len);// uses iLex
		if (token==EErrorToken)
			return KErrArgument;
		if (token==ENullToken)
			break;
		iFormula[i]=token;	// append token to formula
		iTokenLen[i]=(TUint8)Min(len, 255);
		++i;
		}
	iCount=i;

	StripSpaceTokens();// Uses then resets iCount
	TInt ret=CrackTokenFormula();
	if (ret<0)
		return(ret);
	if (iDateTime.Year()>9999)
		return KErrArgument;
	aTime=iDateTime;
	return(ret);
	}

EXPORT_C TInt TTime::Parse(const TDesC& aDes,TInt aCenturyOffset)
/**
Parses a descriptor containing either or both a date and time, and sets this 
TTime to the value of the parsed descriptor.
	
The descriptor may contain the date only, the time only, the date followed 
by the time, or the time followed by the date. When both the date and time 
are specified in the descriptor, they should be separated using one or more 
space characters. 
	
Leading zeros and spaces preceding any time or date components are discarded.
	
Dates may be specified either with all three components (day, month and year), 
or with just two components; for example month and day. The date suffix ("st" 
"nd" "rd" or "th") may not be included in the descriptor.
	
The date and its components may take different forms:
	
1. The month may be represented by text or by numbers.
	
2  European (DD/MM/YYYY), American (MM/DD/YYYY) and Japanese (YYYY/MM/DD) date 
   formats are supported. An exception to this ordering of date components occurs 
   when European or American formatting is used and the month is represented 
   by text. In this case, the month may be positioned in either the first or 
   second field. When using Japanese date format, the month, whether text or 
  numbers, must always be the second field.
	
3. The year may be two or four digits. When the year is a two digit number, (e.g. 
   97 rather than 1997), to resolve any confusion as to which century the year 
   falls in, the second argument determines the century. For example, if the 
   current year is 1997, a value for aCenturyOffset of 20 means that any two 
   digit year will resolve to a year in the range 1920 to 2019. In this case, 
   two digit years between 00 and 19 inclusive refer to the years between 2000 
   and 2019 and two digit years between 20 and 99 inclusive refer to the years 
   between 1920 and 1999. By default, two digit years are in the current century 
   (aCenturyOffset = 0).
	
4. Any of the following characters may be used as the date separator: /(slash) 
   - (dash) , (comma), spaces, or either of the date separator characters specified 
   in TLocale::SetDateSeparator() (at index 1 or 2). Other characters are illegal.
	
If a colon or a dot has been specified in TLocale as the date separator character, 
neither may be used as date separators in this function.
	
If specified, the time must include the hour, but both minutes and seconds, 
or seconds alone may be omitted. 
	
The time and its components may take different forms:
	
1. An am/pm time suffix may be appended to the time. If 24 hour clock format 
   is in use, this text will be ignored. 
	
2. The am/pm suffix may be abbreviated to "a" or "p".
	
3. Any of the following characters may be used as the time separator: :(colon) 
   .(dot) or either of the time separator characters specified in
   TLocale::SetDateSeparator() (at index 1 or 2). Other characters are illegal.
	
When a character can be interpreted as either a date or time separator character, 
this function will interpret it as a date separator.
	
Look out for cases in which wrongly interpreting the contents of a descriptor, 
based on the interpretation of separator characters, causes an error. For 
example, trying to interpret "5.6.1996" as a time is invalid and will return 
an error of -2 because 1,996 seconds is out of range.
	
Notes:
	
1. The entire content of the descriptor must be valid and syntactically correct, 
   or an error will be returned and the parse will fail. So, excepting whitespace, 
   which is discarded, any trailing characters within the descriptor which do 
   not form part of the date or time are illegal. 
	
2. If no time is specified in the descriptor, the hours, minutes and seconds 
   of this TTime are all set to zero, corresponding to midnight at the start 
   of the day specified in the date. If no date is specified, each of this TTime's 
   date components are set to zero.

@param aDes           Descriptor containing any combination of date and
                      time as text.
@param aCenturyOffset Offset between zero (the default) and 99. Allows a flexible 
                      interpretation of the century for two digit year values.
                      If less than zero, or greater than 99, a panic occurs.
                      
@return If equal to or greater than zero, the function completed successfully. 
        EParseDatePresent and/or EParseTimePresent indicate whether either or both 
        of the date or time are present.
        If less than zero, an error code.
        KErrGeneral indicates that the time or date value is out of range,
        e.g. if the hour is greater than 23 or if the minute is greater
        than 59.
        KErrNotSupported indicates that a two field date has been entered.
        KErrArgument indicates that the descriptor was syntactically incorrect.
        If the function fails, this TTime object remains unchanged.
*/
	{

	TStringToDateTime parse(aDes,aCenturyOffset);
	return parse.Parse(*this);
	}

