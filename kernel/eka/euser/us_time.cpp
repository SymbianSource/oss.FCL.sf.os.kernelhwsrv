// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\us_time.cpp
// System date and time functions
// 
//


#include "us_std.h"

// Date and time related constants

static const TInt KMinutesToMicroSeconds = 60000000;
static const TInt KSecondsToMicroSeconds = 1000000;
static const TInt64 KDaysToMicroSeconds = I64LIT(86400000000);
static const TInt64 KHoursToMicroSeconds = I64LIT(3600000000);

// Days in each month
LOCAL_D const TInt8 mTab[2][12]=
    {
    {31,28,31,30,31,30,31,31,30,31,30,31}, // 28 days in Feb
    {31,29,31,30,31,30,31,31,30,31,30,31}  // 29 days in Feb
    };

// Days in year before 1st of each month
LOCAL_D const TInt cmTab[2][12]=
	{
	{0,31,59,90,120,151,181,212,243,273,304,334},
	{0,31,60,91,121,152,182,213,244,274,305,335}
	};

//
// Time::FormatL overflow handler
//
#if defined(_UNICODE)
NONSHARABLE_CLASS(TTimeOverflowLeave) : public TDes16Overflow
	{
public:
	virtual void Overflow(TDes16 &aDes);
	};
void TTimeOverflowLeave::Overflow(TDes16 &/*aDes*/)
	{
	User::Leave(KErrOverflow);
	}
#else
NONSHARABLE_CLASS(TTimeOverflowLeave) : public TDes8Overflow
	{
public:
	virtual void Overflow(TDes8 &aDes);
	};
void TTimeOverflowLeave::Overflow(TDes8 &/*aDes*/)
	{
	User::Leave(KErrOverflow);
	}
#endif
//

EXPORT_C TDateTime::TDateTime(TInt aYear,TMonth aMonth,TInt aDay,TInt aHour,TInt aMinute,TInt aSecond,TInt aMicroSecond)
//
// always panic on a bad date/time field
//
/**
Constructs the TDateTime object with the seven fields which comprise a date 
and time.

@param aYear        The year. No check is made for validity.
@param aMonth       The month. Range is EJanuary to EDecember.
@param aDay         The day. Range is zero to number of days in month minus one.
@param aHour        The hour. Range is 0 to 23.
@param aMinute      The minute. Range is 0 to 59.
@param aSecond      The second. Range is 0 to 59
@param aMicroSecond The microsecond. Range is 0 to 999999

@panic USER 3, if an attempt is made to set an invalid value for any of 
       the fields, except for the year. No check is made upon the validity
       of the year.
*/
	{

	TInt ret=Set(aYear,aMonth,aDay,aHour,aMinute,aSecond,aMicroSecond);
	__ASSERT_ALWAYS(ret==KErrNone,Panic(ETDateTimeBadDateTime));
	}

EXPORT_C TInt TDateTime::Set(TInt aYear,TMonth aMonth,TInt aDay,TInt aHour,TInt aMinute,TInt aSecond,TInt aMicroSecond)
//
// set the various time fields checking that each is valid
// bomb out as soon as invalid field is set to forestall causing a panic
//
/**
Sets all date and time components.

Note:

1. When setting the day and month, subtract one because the ranges are offset 
   from zero. 

2. If the function returns an error, only those fields preceding the field which 
   caused the error will be changed. For example, if the hour is out of range, 
   the year, month and day will be set, all other components will remain unchanged.

@param aYear        Year. No check is made on its validity, except that if the
                    date is set to February 29th, the year can only be set to a
                    leap year, otherwise  an error is returned.
@param aMonth       Month. The valid range is EJanuary to EDecember. If an
                    attempt is made to set an invalid month, or if the current
                    day number in the month is greater than or equal to the
                    number of days in the new month, an error is returned.
@param aDay         The number of the day within the month, offset from zero.
                    If greater than or equal to the total number of days in
                    the month,an error is returned.
@param aHour        Hour. Range is 0 to 23.
@param aMinute      Minute. Range is 0 to 59.
@param aSecond      Second. Range is 0 to 59.
@param aMicroSecond Microsecond. Range is 0 to 999999.

@return KErrNone if successful, KErrGeneral if not.
*/
	{

	iYear=aYear;

	if (aMonth<EJanuary || aMonth>EDecember)
		return KErrGeneral;
	iMonth=aMonth;

	if (aDay<0 || aDay>=Time::DaysInMonth(iYear,iMonth))
		return KErrGeneral;
	iDay=aDay;

	if (aHour<0 || aHour>=24)
		return KErrGeneral;
	iHour=aHour;

	if (aMinute<0 || aMinute>=60)
		return KErrGeneral;
	iMinute=aMinute;

	if (aSecond<0 || aSecond>=60)
		return KErrGeneral;
	iSecond=aSecond;

	if (aMicroSecond<0 || aMicroSecond>=1000000)
		return KErrGeneral;
	iMicroSecond=aMicroSecond;

	return KErrNone;
	}

EXPORT_C TInt TDateTime::SetYear(TInt aYear)
//
// doesnt let you reset 29th February to non-leap year, no check on year range
//
/**
Sets the year without a leap year check.

No check is made on the validity of the year except that if the current date
is February 29th, the year can only be changed to another leap year, otherwise
an error is returned.

@param aYear The year.

@return KErrNone if successful, KErrGeneral if not.
*/
	{

	if (iDay>=Time::DaysInMonth(aYear,iMonth))
		return KErrGeneral;
	iYear=aYear;
	return KErrNone;
	}

EXPORT_C TInt TDateTime::SetYearLeapCheck(TInt aYear)
//
// lets you reset 29th February to non-leap year(moves date to 28th/Feb), no check on year range
//
/**
Sets the year with a leap year check.

Unlike SetYear(), if the date is the 29th February, this function allows
the year to be set to a non-leap year. In this case, the date is reset to
the 28th February.

@param aYear The year.

@return KErrNone if successful, KErrGeneral if not.

@see TDateTime::SetYear
*/
	{

	if (iDay>=Time::DaysInMonth(aYear,iMonth))
        iDay=27;
    iYear=aYear;
	return KErrNone;
	}

EXPORT_C TInt TDateTime::SetMonth(TMonth aMonth)
/**
Sets the month component of the date/time.

@param aMonth The month to be set. The range is from EJanuary to EDecember.
              If an attempt is made to set an invalid month, or if the current
              day number in the month is greater than or equal to the number of
              days in the new month, an error is returned.
              
@return KErrNone if successful, KErrGeneral if not.
*/
	{

	if (aMonth<EJanuary || aMonth>EDecember || iDay>=Time::DaysInMonth(iYear,aMonth))
		return KErrGeneral;
	iMonth=aMonth;
	return KErrNone;
	}

EXPORT_C TInt TDateTime::SetDay(TInt aDay)
/**
Sets the day component of the date/time.

@param aDay The number of the day within the month, offset from zero. If equal 
            to or greater than the total number of days in the month, an error
            is returned.
            
@return KErrNone if successful, KErrGeneral if not.
*/
	{

	if (aDay<0 || aDay>=Time::DaysInMonth(iYear,iMonth))
		return KErrGeneral;
	iDay=aDay;
	return KErrNone;
	}

EXPORT_C TInt TDateTime::SetHour(TInt aHour)
/**
Sets the hour component of the date/time.

@param aHour The hour. Range is 0 to 23.

@return KErrNone if successful, KErrGeneral if not.
*/
	{

	if (aHour<0 || aHour>=24) // GC - bug fix
		return KErrGeneral;
	iHour=aHour;
	return KErrNone;
	}

EXPORT_C TInt TDateTime::SetMinute(TInt aMinute)
/**
Sets the minute component of the date/time.

@param aMinute The minute. Range is 0 to 59.

@return KErrNone if successful, KErrGeneral if not.
*/
	{

	if (aMinute<0 || aMinute>=60)
		return KErrGeneral;
	iMinute=aMinute;
	return KErrNone;
	}

EXPORT_C TInt TDateTime::SetSecond(TInt aSecond)
/**
Sets the second component of the date/time.

@param aSecond The second. Range is 0 to 59.

@return KErrNone if successful, KErrGeneral if not.
*/
	{

	if (aSecond<0 || aSecond>=60)
		return KErrGeneral;
	iSecond=aSecond;
	return KErrNone;
	}

EXPORT_C TInt TDateTime::SetMicroSecond(TInt aMicroSecond)
/**
Sets the microsecond component of the date/time.

@param aMicroSecond The microsecond. Range is 0 to 999999.

@return KErrNone if successful, KErrGeneral if not.
*/
	{

	if (aMicroSecond<0 || aMicroSecond>=1000000)
		return KErrGeneral;
	iMicroSecond=aMicroSecond;
	return KErrNone;
	}

// class TTime

EXPORT_C TTime::TTime(const TDesC &aString)
/**
Constructs a TTime object with a text string.

The string consists of up to three components, any or all of which
may be omitted:

1. year, month and day, followed by a colon

2. hour, minute and second, followed by a dot

3. microsecond

When all three components are present, the string should take the form:

YYYYMMDD:HHMMSS.MMMMMM

The conversion from text to time is carried out in the same manner as that 
used in TTime::Set().

For a list of the range of valid values for date and time components,
see TDateTime::Set().

@param aString Date and time string for initializing the TTime object. 

@panic USER 113, if the string is syntactically incorrect, for example, if 
                 neither a colon nor a dot is present, or if any component of
                 the date or time is assigned an invalid value, or the year
                 is negative.

@see TTime::Set
@see TDateTime::Set
*/
	{

	__ASSERT_ALWAYS(Set(aString)==KErrNone,Panic(ETTimeValueOutOfRange));
	}

EXPORT_C TTime::TTime(const TDateTime &aDateTime) : iTime(Convert(aDateTime).Int64())
/**
Constructs a TTime object with the seven fields which comprise a date and time.

@param aDateTime Date and time to which to initialise the TTime object.
*/

    {}

EXPORT_C TInt TTime::Set(const TDesC &aString)
//
// Convert string to time. String is in the format:
//
// YYYYMMDD:HHMMSS.MMMMMM
//
// Any part may be ommitted, but either the
// dot or colon or both must be present
//
/**
Assigns a date and time contained in a descriptor to this TTime object.

The string consists of up to three components, any or all of which may
be omitted:

1. year, month and day, followed by a colon 

2. hour, minute and second, followed by a dot 

3. microsecond

When all three components are present, the string should take the form:

YYYYMMDD:HHMMSS.MMMMMM

If omitted, the first component defaults to 1 January, year 0 (nominal Gregorian).
This is the base date for TTime, so the result of this method will be the number
of microseconds in the time specified by the second and third components.

If either the second or third components are omitted, they are set to zero.

Notes:

1. The month and day values are offset from zero.

2. The only situations in which either the colon or dot may be omitted are as 
   follows:
   
   2.1 If the microsecond component is omitted, the preceding dot may also
       be omitted.

   2.2 The colon can be omitted only if a dot is located at string position
       zero (indicating that the first two components are missing), or at
       string position six (indicating that the first component only is
       missing).

@param aString The date and time to be assigned to this TTime object.
       
@return KErrNone if successful,
        KErrGeneral if the string is syntactically incorrect, for example,
        if neither a colon nor a dot is present, or if any component of the
        date or time is given an invalid value, or the year is negative.
        For a list of valid values for date and time components,
        see TDateTime::Set().
        If an error occurs, the date and time will remain unchanged.
*/
	{

//
// Get position of the colon and dot separators
//
    TInt colon=aString.Locate(':');
    TInt dot=aString.Locate('.');

    if(colon==KErrNotFound && dot==KErrNotFound)
        return(KErrGeneral);
//
// Zero parts that aren't supplied
//
    TInt yy=0;
    TInt mm=0;
    TInt dd=0;
    TInt hr=0;
    TInt mi=0;
    TInt se=0;
    TInt ms=0;
//
// Convert YYYYMMDD if present
//
    switch(colon)
        {
        case 0:
       	    break;
        case KErrNotFound:
            if(dot!=0 && dot!=6)
                return(KErrGeneral);
            colon=-1;
            break;
        case 8:
	   		{
            TLex y=aString.Left(4);
            TLex m=aString.Mid(4,2);
            TLex d=aString.Mid(6,2);
            y.Val(yy);
            m.Val(mm);
            d.Val(dd);
	    	}
            break;
        default: // Colon in wrong position - return error
            return(KErrGeneral);
        }
//
// Convert HHMMSS if present
//
    if(dot==KErrNotFound)
        dot=aString.Length();
     
    if(dot==colon+7)
        {
        TLex h=aString.Mid(dot-6,2);
        TLex m=aString.Mid(dot-4,2);
        TLex s=aString.Mid(dot-2,2);
        h.Val(hr);
        m.Val(mi);
        s.Val(se);
        }
    else if(dot!=KErrNotFound && dot!=0 && dot!=colon+1)
    	return(KErrGeneral);

    if(dot!=KErrNotFound)
        {
        if(aString.Length()>dot+7)
            return(KErrGeneral); // microseconds is more than 6 digits
        if(dot<aString.Length())
        	{
        	TLex m=aString.Mid(dot+1);
        	m.Val(ms);
        	}
        }
        
//
// Set the time! Do not construct newtime using the values or
// it may cause TTime::Set() to panic rather than return an error
//
	TDateTime newtime;
	if(newtime.Set(yy,TMonth(mm),dd,hr,mi,se,ms)!=KErrNone)
		return(KErrGeneral);
    (*this)=newtime;
	return KErrNone;
	}

EXPORT_C TInt TTime::HomeTimeSecure()
/**
Sets the date and time of this TTime to the secure home time. 
Returns KErrNoSecureTime if there is no secure time source
*/
	{
	TInt utOffset=0;
	TInt r = Exec::TimeNowSecure(*(TInt64*)this,utOffset);
    operator+=(TTimeIntervalSeconds(utOffset));
	return r;
	}

EXPORT_C TInt TTime::UniversalTimeSecure()
/**
Sets the date and time of this TTime to the secure universal time.
*/
	{
	TInt utOffset=0;
	return Exec::TimeNowSecure(*(TInt64*)this,utOffset);
	}

EXPORT_C void TTime::HomeTime()
/**
Sets the date and time of this TTime to the home time.
*/
	{
	TInt utOffset=0;
	Exec::TimeNow(*(TInt64*)this,utOffset);
    operator+=(TTimeIntervalSeconds(utOffset));
	}

EXPORT_C void TTime::UniversalTime()
/**
Sets the date and time of this TTime to the universal time.
*/
	{
	TInt utOffset=0;
	Exec::TimeNow(*(TInt64*)this,utOffset);
	}

EXPORT_C void TTime::RoundUpToNextMinute()
/**
Rounds this TTime up to the next minute.

Both the seconds and microseconds components are set to zero.
*/
	{

	if (iTime>0)
		iTime+=59999999;
//*	TInt64 remainder;
//*	Int64().DivMod(60000000,remainder);
//*	iTime-=remainder;	
	iTime-=iTime%60000000;
	}

TTime TTime::Convert(const TDateTime &aDateTime)
//
// converts TDateTime into a TTime, doesnt check for overflows
//
	{
	
	TInt days=365*aDateTime.Year()+Time::LeapYearsUpTo(aDateTime.Year());
	TBool isleap=Time::IsLeapYear(aDateTime.Year());
	days+=cmTab[isleap][aDateTime.Month()];
	days+=aDateTime.Day();

	TUint sum=aDateTime.MicroSecond()+aDateTime.Second()*KSecondsToMicroSeconds+aDateTime.Minute()*KMinutesToMicroSeconds;
	return(((TInt64(days*3)<<3)+TInt64(aDateTime.Hour()))*KHoursToMicroSeconds+TInt64(sum));
	}

EXPORT_C TTime &TTime::operator=(const TDateTime &aDateTime)
/**
Assigns a TDateTime object to this TTime object.

@param aDateTime The date and time to assign to this TTime object.

@return This TTime object.
*/
	{

	iTime=Convert(aDateTime).Int64();
	return(*this);
	}

EXPORT_C TDateTime TTime::DateTime() const
//
// converts iTime back into its TDateTime components
//
/**
Converts the TTime object into a TDateTime object.

This conversion must be done before the seven fields which comprise a date
and time can be accessed.

@return The components of the time, indicating year, month, day, hour, minute, 
        second, microsecond.
*/
	{

	TInt64 rem;
	TInt64 daysSince0AD64(iTime);
	
	rem = daysSince0AD64 % KDaysToMicroSeconds;
	daysSince0AD64 /= KDaysToMicroSeconds;

	TInt daysSince0AD = static_cast<TInt>(daysSince0AD64);

	TInt year;
	TInt daysLeft;
	if (iTime<0)
		{ // -1 to make daysLeft +ve and assume leap year every 4 years
		if (rem!=TInt64(0))
			{
			daysSince0AD--;
			rem=iTime-TInt64(daysSince0AD)*KDaysToMicroSeconds;
			}
		year=(4*daysSince0AD)/((4*365)+1);
		if ((4*daysSince0AD)%((4*365)+1))
			year--;
		daysLeft=daysSince0AD-((year*365)+Time::LeapYearsUpTo(year));
		}
	else
		{ // after 1600 leap years less than every four years
		year=(4*daysSince0AD)/((4*365)+1);
		daysLeft=daysSince0AD-((year*365)+Time::LeapYearsUpTo(year));
		TInt daysInYear=365+Time::IsLeapYear(year);
	    while (daysLeft>=daysInYear)
		    {
			year++;
	        daysLeft-=daysInYear;
			daysInYear=365+Time::IsLeapYear(year);
			}
		}

	TDateTime result(0,EJanuary,0,0,0,0,0);
	result.SetYear(year);

   	TBool isleap=Time::IsLeapYear(year);
    TInt month=11;
	const TInt* pCM=&(cmTab[isleap][11])+1;
	while(daysLeft<*--pCM)
		month--;
	daysLeft-=*pCM;

	result.SetMonth((TMonth)month);
	result.SetDay(daysLeft);

	TInt hour = static_cast<TInt>(rem >> 10) / 3515625;	// 3515625=KHoursToMicroSeconds/1024
	result.SetHour(hour);
	TUint rem32=I64LOW(rem-(TInt64(hour*3515625)<<10));
	TUint min=rem32/KMinutesToMicroSeconds;
	result.SetMinute((TInt)min);
	rem32-=min*KMinutesToMicroSeconds;
	TUint sec=rem32/KSecondsToMicroSeconds;
	result.SetSecond((TInt)sec);
	rem32-=sec*KSecondsToMicroSeconds;
	result.SetMicroSecond(TInt(rem32));
	return(result);
	}

EXPORT_C TTimeIntervalMicroSeconds TTime::MicroSecondsFrom(TTime aTime) const
//
// this - aTime
//
/**
Calculates the number of microseconds difference between the specified TTime
and this TTime.

@param aTime The time to be compared with this TTime.

@return Difference in microseconds between the two times. If the time specified 
        in the argument is later than this TTime, this value is negative.
*/
	{

	TInt64 difference=iTime-aTime.Int64();
	return(difference);
	}

EXPORT_C TInt TTime::SecondsFrom(TTime aTime,TTimeIntervalSeconds &aInterval) const
//
// this - aTime as whole seconds
// this function may fail if difference > no of seconds that can be represented in a TInt
//
/**
Calculates the number of seconds difference between the specified TTime and
this TTime.

The difference may be positive or negative.

@param aTime     The time to be compared with this TTime.
@param aInterval On return contains the difference in seconds between the two 
                 times. If the time specified in the first argument is later than
                 this TTime, then this returned value is negative.
                 
@return Error code. KErrNone if successful. 
                    KErrOverflow, if the calculated interval is too large for
                    a 32-bit integer.
*/
	{
	TInt64 diff;
	if (iTime>aTime.Int64())
		{
		diff= TInt64(TUint64(iTime-aTime.Int64())/KSecondsToMicroSeconds);	
		}
	else 
		{
		diff= -TInt64(TUint64(aTime.Int64()-iTime)/KSecondsToMicroSeconds);
		}	
	if (diff>KMaxTInt || diff<KMinTInt)	
	    return KErrOverflow; 
	aInterval = static_cast<TInt>(diff);
	return KErrNone;
	}
	
EXPORT_C TInt TTime::MinutesFrom(TTime aTime,TTimeIntervalMinutes &aInterval) const
//
// iTime - aTime as whole minutes
// function may fail if difference can't be represented as a TInt
//
/**
Calculates the number of minutes difference between the specified TTime and
this TTime.

The difference may be positive or negative.

@param aTime     The time to be compared with this TTime.
@param aInterval On return contains the difference in minutes between the two 
                 times. If the time specified in the first argument is later
                 than this TTime, then this returned value is negative.
                 
@return Error code. KErrNone if successful. 
                    KErrOverflow, if the calculated interval is too large for
                    a 32-bit integer.
*/
	{
	TInt64 diff;
	if (iTime>aTime.Int64())
		{
		diff= TInt64(TUint64(iTime-aTime.Int64())/KMinutesToMicroSeconds);	
		}
	else 
		{
		diff= -TInt64(TUint64(aTime.Int64()-iTime)/KMinutesToMicroSeconds);
		}	
	if (diff>KMaxTInt || diff<KMinTInt)	
	    return KErrOverflow; 
	aInterval = static_cast<TInt>(diff);
	return KErrNone; 
	}

EXPORT_C TInt TTime::HoursFrom(TTime aTime,TTimeIntervalHours &aInterval) const
//
// iTime - aTime as whole hours
// function may fail if difference can't be represented as a TInt
//
/**
Calculates the number of hours difference between the specified TTime and
this TTime. 

The difference may be positive or negative.

@param aTime     The time to be compared with this TTime.
@param aInterval On return contains the difference in hours between the two 
                 times. If the time specified in the first argument is later
                 than this TTime, then this returned value is negative.
                 
@return Error code. KErrNone if successful. 
                    KErrOverflow, if the calculated interval is too large for
                    a 32-bit integer.
*/
	{
	TInt64 diff;
	if (iTime>aTime.Int64())
		{
		diff= TInt64(TUint64(iTime-aTime.Int64())/KHoursToMicroSeconds);	
		}
	else 
		{
		diff= -TInt64(TUint64(aTime.Int64()-iTime)/KHoursToMicroSeconds);
		}
	if (diff>KMaxTInt || diff<KMinTInt)	
	    return KErrOverflow; 
	aInterval = static_cast<TInt>(diff);
	return KErrNone;
	}  


EXPORT_C TTimeIntervalDays TTime::DaysFrom(TTime aTime) const
//
// iTime - aTime as whole days
//
/**
Calculates the number of days difference between the specified TTime and
this TTime. 

The difference may be positive or negative.

@param aTime  The time to be compared with this TTime.

@return Difference in days between the two times. If the time specified in 
        aTime is later than this TTime, the returned value will be negative.
*/
	{
	if (iTime>aTime.Int64())
		{
		return TInt(TUint64(iTime-aTime.Int64())/KDaysToMicroSeconds);	
		}
	else 
		{
		return -TInt(TUint64(aTime.Int64()-iTime)/KDaysToMicroSeconds);
		}	
	}

EXPORT_C TTimeIntervalMonths TTime::MonthsFrom(TTime aTime) const
//
// iTime - aTime as whole months - ie aTime must be on a later day in the month and later in that day
// except for last days etc eg 31st October - 30 November is one month to be consistent with other
// functions
//
/**
Calculates the number of months between the specified TTime and this TTime.

The result may be positive or negative.

The interval in months between two TTimes is calculated by incrementing it 
by one each time the same day number and time in the previous or following 
month has been reached. Exceptions to this rule occur when this TTime is on 
the last day of the month. In this case, the following rules apply:

When comparing this TTime with a later time:

1. if the following month is shorter, one month is deemed to separate the times 
   when the same time on the last day of the following month is reached. In this 
   case, the two day numbers are not the same.

When comparing this TTime with an earlier time:

1. if the previous month is shorter, one month is deemed to separate the times 
   when the last microsecond of the previous month is reached (23:59:59.999999 
   on the last day of the month).

2. if the previous month is longer, one month is deemed to separate the times 
   when the same time on the last day of previous month is reached. In this case, 
   the two day numbers are not the same.

@param aTime The time to be compared with this TTime.

@return Difference in months between the two times. If the time specified in 
        the argument is later than this TTime, the interval is negative.
*/
	{

	TDateTime dateTimei=DateTime();
	TDateTime dateTimea=aTime.DateTime();
	
	TInt monthsDifference=(dateTimei.Year()-dateTimea.Year())*12+(dateTimei.Month()-dateTimea.Month());

	if (monthsDifference>0)
		{
		if (dateTimei.Day()<=dateTimea.Day())
			{
			if (iTime%KDaysToMicroSeconds<aTime.Int64()%KDaysToMicroSeconds || (dateTimei.Day()!=dateTimea.Day() && dateTimei.Day()!=DaysInMonth()-1))
				monthsDifference--;
			}
		}
	else
		if (monthsDifference!=0)//monthsDifference<0
			{
			if (dateTimei.Day()>=dateTimea.Day())
				{
				if (iTime%KDaysToMicroSeconds>aTime.Int64()%KDaysToMicroSeconds || (dateTimei.Day()!=dateTimea.Day() && dateTimea.Day()!=aTime.DaysInMonth()-1))
					monthsDifference++;
				}
			}

	return(monthsDifference);			
	}

EXPORT_C TTimeIntervalYears TTime::YearsFrom(TTime aTime) const
//
// as above,but for twelve months
//
/**
Calculates the number of years between the specified TTime and this TTime.

The result may be positive or negative.

Note that the interval in years between two TTimes is calculated by
incrementing it by one each time the same day number and time in the previous
or following year has been reached. The exception to this rule occurs when this
TTime is the last day in February in a leap year. In this case, one year is
deemed to have passed when the same time of day on the last day in the preceding 
or following February has been reached.

@param aTime The time to be compared with this TTime.

@return Difference in years between the two times. If the time specified in 
        the argument is later than this TTime, the interval is negative.
*/
	{

	TTimeIntervalMonths mos= TTime::MonthsFrom(aTime);
	TTimeIntervalYears ret=mos.Int()/12;
	return(ret);			
	}

EXPORT_C TTime TTime::operator+(TTimeIntervalYears aYear) const
/**
Adds a time interval to this TTime, returning the result
as a TTime.

Note that in a leap year, when adding one year to the 29th February, the result
is the 28th February in the following year.

Note also that this TTime object is not changed.

@param aYear A time interval in years. The argument is stored as a 32 bit
             signed integer. The maximum value which it can represent is
             2147483647. Any attempt to add more than this amount will
             produce incorrect results.

@return The new time.
*/
	{

	return((*this)+TTimeIntervalMonths(aYear.Int()*12));
	}

EXPORT_C TTime TTime::operator+(TTimeIntervalMonths aMonth) const
/**
Adds a time interval to this TTime, returning the result
as a TTime.

Note that when adding one month to the last day in the month, if the following
month is shorter, the result is the last day in the following month.
For example, when adding one month to 31st August, the result is
the 30th September.

Note also that this TTime object is not changed.

@param aMonth A time interval in months. The argument is stored as a 32 bit
              signed integer. The maximum value which it can represent is
              2147483647. Any attempt to add more than this amount will
              produce incorrect results.

@return The new time.
*/
	{

	TDateTime dateTime=DateTime();
	TInt month=dateTime.Month()+(dateTime.Year()*12)+aMonth.Int();
	TInt day=dateTime.Day();
	TInt year=month/12;
	month%=12;
	if (month<0)
		{
		year--;
		month+=12;
		}
	TInt daysInMonth=(mTab[Time::IsLeapYear(year)][month]-1); 
	if (day>=daysInMonth)
		day=daysInMonth;
	__ASSERT_ALWAYS(dateTime.Set(year,TMonth(month),day,dateTime.Hour(),dateTime.Minute(),dateTime.Second(),dateTime.MicroSecond())==KErrNone,Panic(ETDateTimeBadDateTime));
	return(dateTime);
	}
							 
EXPORT_C TTime TTime::operator+(TTimeIntervalDays aDay) const
/**
Adds a time interval to this TTime, returning the result
as a TTime.

Note that this TTime object is not changed.

@param aDay A time interval in days. The argument is stored as a 32 bit
            signed integer. The maximum value which it can represent is
            2147483647. Any attempt to add more than this amount will
            produce incorrect results.

@return The new time.
*/
	{ 

	return(iTime+TInt64(aDay.Int())*KDaysToMicroSeconds);
	}

EXPORT_C TTime TTime::operator+(TTimeIntervalHours aHour) const
/**
Adds a time interval to this TTime, returning the result
as a TTime.

Note that this TTime object is not changed.

@param aHour A time interval in hours. The argument is stored as a 32 bit
             signed integer. The maximum value which it can represent is
             2147483647. Any attempt to add more than this amount will
             produce incorrect results.

@return The new time.
*/
	{

	return(iTime+TInt64(aHour.Int())*KHoursToMicroSeconds);
	}

EXPORT_C TTime TTime::operator+(TTimeIntervalMinutes aMinute) const
/**
Adds a time interval to this TTime, returning the result
as a TTime.

Note that this TTime object is not changed.

@param aMinute A time interval in minutes. The argument is stored as a 32 bit
               signed integer. The maximum value which it can represent is
               2147483647. Any attempt to add more than this amount will
               produce incorrect results.

@return The new time.
*/
	{

	return(iTime+TInt64(aMinute.Int())*KMinutesToMicroSeconds);
	}

EXPORT_C TTime TTime::operator+(TTimeIntervalSeconds aSecond) const
/**
Adds a time interval to this TTime, returning the result
as a TTime.

Note that this TTime object is not changed.

@param aSecond A time interval in seconds. The argument is stored as a 32 bit
               signed integer. The maximum value which it can represent is
               2147483647. Any attempt to add more than this amount will
               produce incorrect results.

@return The new time.
*/
	{

	return(iTime+TInt64(aSecond.Int())*KSecondsToMicroSeconds);
	}

 
EXPORT_C TTime TTime::operator+(TTimeIntervalMicroSeconds aMicroSecond) const
/**
Adds a time interval to this TTime, returning the result
as a TTime.

Note that this TTime object is not changed.

@param aMicroSecond A time interval in microseconds.

@return The new time.
*/
	{

	return(iTime+(aMicroSecond.Int64()));
	}

EXPORT_C TTime TTime::operator+(TTimeIntervalMicroSeconds32 aMicroSecond) const
/**
Adds a time interval to this TTime, returning the result
as a TTime.

Note that this TTime object is not changed.

@param aMicroSecond A time interval in microseconds. The argument is stored as
                    a 32 bit signed integer. The maximum value which it can
                    represent is 2147483647. Any attempt to add more than this
                    amount will produce incorrect results.

@return The new time.
*/
	{

	return(iTime+aMicroSecond.Int());
	}

EXPORT_C TTime TTime::operator-(TTimeIntervalYears aYear) const
/**
Substracts a time interval from this TTime, returning the result
as a TTime.

Note that in a leap year, when subtracting one year from the 29th February,
the result is 28th February in the preceding year.

Note also that this TTime object is not changed.

@param aYear A time interval in years. The argument is stored as
             a 32 bit signed integer. The maximum value which it can
             represent is 2147483647. Any attempt to subtract more than this
             amount will produce incorrect results.

@return The new time.
*/
	{

	return((*this)-TTimeIntervalMonths(aYear.Int()*12));
	}

EXPORT_C TTime TTime::operator-(TTimeIntervalMonths aMonth) const
/**
Substracts a time interval from this TTime, returning the result
as a TTime.

Note that when subtracting one month from the last day in the month, if the
preceding month is shorter, the result is the last day in the preceding month.
For example, when subtracting 1 month from 31st October, the result is
the 30th September.

Note also that this TTime object is not changed.

@param aMonth A time interval in months. The argument is stored as
              a 32 bit signed integer. The maximum value which it can
              represent is 2147483647. Any attempt to subtract more than this
              amount will produce incorrect results.

@return The new time.
*/
	{

	return((*this)+TTimeIntervalMonths(aMonth.Int()*-1));
	}
							 
EXPORT_C TTime TTime::operator-(TTimeIntervalDays aDay) const
/**
Substracts a time interval from this TTime, returning the result
as a TTime.

Note that this TTime object is not changed.

@param aDay A time interval in days. The argument is stored as
            a 32 bit signed integer. The maximum value which it can
            represent is 2147483647. Any attempt to subtract more than this
            amount will produce incorrect results.

@return The new time.
*/
	{ 

	return(iTime-TInt64(aDay.Int())*KDaysToMicroSeconds);
	}

EXPORT_C TTime TTime::operator-(TTimeIntervalHours aHour) const
/**
Substracts a time interval from this TTime, returning the result
as a TTime.

Note that this TTime object is not changed.

@param aHour A time interval in hours. The argument is stored as
             a 32 bit signed integer. The maximum value which it can
             represent is 2147483647. Any attempt to subtract more than this
             amount will produce incorrect results.

@return The new time.
*/
	{

	return(iTime-TInt64(aHour.Int())*KHoursToMicroSeconds);
	}

EXPORT_C TTime TTime::operator-(TTimeIntervalMinutes aMinute) const
/**
Substracts a time interval from this TTime, returning the result
as a TTime.

Note that this TTime object is not changed.

@param aMinute A time interval in minutes. The argument is stored as
               a 32 bit signed integer. The maximum value which it can
               represent is 2147483647. Any attempt to subtract more than this
               amount will produce incorrect results.

@return The new time.
*/
	{

	return(iTime-TInt64(aMinute.Int())*KMinutesToMicroSeconds);
	}

EXPORT_C TTime TTime::operator-(TTimeIntervalSeconds aSecond) const
/**
Substracts a time interval from this TTime, returning the result
as a TTime.

Note that this TTime object is not changed.

@param aSecond A time interval in seconds. The argument is stored as
               a 32 bit signed integer. The maximum value which it can
               represent is 2147483647. Any attempt to subtract more than this
               amount will produce incorrect results.

@return The new time.
*/
	{

	return(iTime-TInt64(aSecond.Int())*KSecondsToMicroSeconds);
	}

EXPORT_C TTime TTime::operator-(TTimeIntervalMicroSeconds aMicroSecond) const
/**
Substracts a time interval from this TTime, returning the result
as a TTime.

Note that this TTime object is not changed.

@param aMicroSecond A time interval in microseconds.

@return The new time.
*/
	{

	return(iTime-(aMicroSecond.Int64()));
	}

EXPORT_C TTime TTime::operator-(TTimeIntervalMicroSeconds32 aMicroSecond) const
/**
Substracts a time interval from this TTime, returning the result
as a TTime.

Note that this TTime object is not changed.

@param aMicroSecond A time interval in microseconds. The argument is stored as
                    a 32 bit signed integer. The maximum value which it can
                    represent is 2147483647. Any attempt to subtract more than
                    this amount will produce incorrect results.

@return The new time.
*/
	{

	return(iTime-aMicroSecond.Int());
	}

EXPORT_C TTime &TTime::operator+=(TTimeIntervalYears aYear)
/**
Adds a time interval to this TTime, returning a reference to this TTime.

@param aYear A time interval in years.

@return A reference to this TTime.
*/
	{	

	TTime tim=(*this)+aYear;
	iTime=tim.Int64();
	return(*this);
	}

EXPORT_C TTime &TTime::operator+=(TTimeIntervalMonths aMonth)
/**
Adds a time interval to this TTime, returning a reference to this TTime.

@param aMonth A time interval in months.

@return A reference to this TTime.
*/
	{

	TTime tim=(*this)+aMonth;
	iTime=tim.Int64();
	return(*this);
	}

EXPORT_C TTime &TTime::operator+=(TTimeIntervalDays aDay)
/**
Adds a time interval to this TTime, returning a reference to this TTime.

@param aDay A time interval in days.

@return A reference to this TTime.
*/
	{

	iTime+=TInt64(aDay.Int())*KDaysToMicroSeconds;
	return(*this);
	}

EXPORT_C TTime &TTime::operator+=(TTimeIntervalHours aHour)
/**
Adds a time interval to this TTime, returning a reference to this TTime.

@param aHour A time interval in hours.

@return A reference to this TTime.
*/
	{

	iTime+=TInt64(aHour.Int())*KHoursToMicroSeconds;
	return(*this);
	}

EXPORT_C TTime &TTime::operator+=(TTimeIntervalMinutes aMinute)
/**
Adds a time interval to this TTime, returning a reference to this TTime.

@param aMinute A time interval in minutes.

@return A reference to this TTime.
*/
	{

	iTime+=TInt64(aMinute.Int())*KMinutesToMicroSeconds;
	return(*this);
	}

EXPORT_C TTime &TTime::operator+=(TTimeIntervalSeconds aSecond)
/**
Adds a time interval to this TTime, returning a reference to this TTime.

@param aSecond A time interval in seconds.

@return A reference to this TTime.
*/
	{

	iTime+=TInt64(aSecond.Int())*KSecondsToMicroSeconds;
	return(*this);
	}

EXPORT_C TTime &TTime::operator+=(TTimeIntervalMicroSeconds aMicroSecond)
/**
Adds a time interval to this TTime, returning a reference to this TTime.

@param aMicroSecond A time interval in microseconds.

@return A reference to this TTime.
*/
	{

	iTime+=aMicroSecond.Int64();
	return(*this);
	}
 
EXPORT_C TTime &TTime::operator+=(TTimeIntervalMicroSeconds32 aMicroSecond)
/**
Adds a time interval to this TTime, returning a reference to this TTime.

@param aMicroSecond A time interval in microseconds, as a 32-bit integer.

@return A reference to this TTime.
*/
	{

	iTime+=aMicroSecond.Int();
	return(*this);
	}
 
EXPORT_C TTime &TTime::operator-=(TTimeIntervalYears aYear)
/**
Subtracts a time interval from this TTime, returning a reference to this TTime.

@param aYear A time interval in years.

@return A reference to this TTime.
*/
	{	

	TTime tim=(*this)-aYear;
	iTime=tim.Int64();
	return(*this);
	}

EXPORT_C TTime &TTime::operator-=(TTimeIntervalMonths aMonth)
/**
Subtracts a time interval from this TTime, returning a reference to this TTime.

@param aMonth A time interval in months.

@return A reference to this TTime.
*/
	{

	TTime tim=(*this)-aMonth;
	iTime=tim.Int64();
	return(*this);
	}

EXPORT_C TTime &TTime::operator-=(TTimeIntervalDays aDay)
/**
Subtracts a time interval from this TTime, returning a reference to this TTime.

@param aDay A time interval in days.

@return A reference to this TTime.
*/
	{

	iTime-=TInt64(aDay.Int())*KDaysToMicroSeconds;
	return(*this);
	}

EXPORT_C TTime &TTime::operator-=(TTimeIntervalHours aHour)
/**
Subtracts a time interval from this TTime, returning a reference to this TTime.

@param aHour A time interval in hours.

@return A reference to this TTime.
*/
	{

	iTime-=TInt64(aHour.Int())*KHoursToMicroSeconds;
	return(*this);
	}

EXPORT_C TTime &TTime::operator-=(TTimeIntervalMinutes aMinute)
/**
Subtracts a time interval from this TTime, returning a reference to this TTime.

@param aMinute A time interval in minutes.

@return A reference to this TTime.
*/
	{

	iTime-=TInt64(aMinute.Int())*KMinutesToMicroSeconds;
	return(*this);
	}

EXPORT_C TTime &TTime::operator-=(TTimeIntervalSeconds aSecond)
/**
Subtracts a time interval from this TTime, returning a reference to this TTime.

@param aSecond A time interval in seconds.

@return A reference to this TTime.
*/
	{

	iTime-=TInt64(aSecond.Int())*KSecondsToMicroSeconds;
	return(*this);
	}

EXPORT_C TTime &TTime::operator-=(TTimeIntervalMicroSeconds aMicroSecond)
/**
Subtracts a time interval from this TTime, returning a reference to this TTime.

@param aMicroSecond A time interval in microseconds.

@return A reference to this TTime.
*/
	{

	iTime-=aMicroSecond.Int64();
	return(*this);
	}

EXPORT_C TTime &TTime::operator-=(TTimeIntervalMicroSeconds32 aMicroSecond)
/**
Subtracts a time interval from this TTime, returning a reference to this TTime.

@param aMicroSecond A time interval in microseconds, as a 32-bit integer.

@return A reference to this TTime.
*/
	{

	iTime-=aMicroSecond.Int();
	return(*this);
	}
 
EXPORT_C TInt TTime::DaysInMonth() const
/**
Gets the number of days in the current month.

@return The number of days in the month.
*/
	{

	TDateTime dateTime=DateTime();
	return(Time::DaysInMonth(dateTime.Year(),dateTime.Month()));
	}

EXPORT_C TDay TTime::DayNoInWeek() const
//
// 1st January year 0 was a Monday
//
/**
Gets the day number within the current week.

This is a value in the range zero to six inclusive, and honours the 
setting specified in TLocale::SetStartOfWeek().

By default the first day in the week is Monday.

@return The number of the day within the week. The range is EMonday to ESunday.

@see TLocale::SetStartOfWeek
*/
	{


	TInt64 fullDays=iTime/KDaysToMicroSeconds;
	TInt day = static_cast<TInt>(fullDays) % 7;
	if (iTime<0)
		{
		if (fullDays*KDaysToMicroSeconds!=iTime)
			day+=6;
		else
			if (day!=0)
				day+=7;
		}
	return((TDay)day);
	}

EXPORT_C TInt TTime::DayNoInMonth() const
/**
Gets the day number in the month.

@return The day number in the month. The first day in the month is numbered 
        zero.
*/
	{

	return(DateTime().Day());
	}

EXPORT_C TInt TTime::DayNoInYear() const
//
// day number in comparison to 1st January
//
/**
Gets the day number in the year. 

@return The day number in the year. The first day in the year is day one.
*/
	{

	TDateTime dateTime=DateTime();
	TTime jan1st=TDateTime(dateTime.Year(),EJanuary,0,0,0,0,0);
	return(DayNoInYear(jan1st));
	}

EXPORT_C TInt TTime::DayNoInYear(TTime aStartDate) const
//
// day number in comparison to given date, check is made to ensure first day is within a year before aDay
//
/**
Gets the day number in the year when the start of the year is aStartDate. 

If no start date is specified, the default is January 1st.

@param aStartDate Indicates the date which is to be considered the start of 
                  the year. Default is 1st January.
                  
@return The day number in the year. The first day in the year is day one.
*/
	{

	TInt y=DateTime().Year();
	TMonth m=aStartDate.DateTime().Month();
	TInt d=aStartDate.DateTime().Day();
    if (d>=Time::DaysInMonth(y,m))
        d=27;
    TDateTime yearStart(y,m,d,0,0,0,0);              // LEAP YEAR PROBLEMS ???
	aStartDate=yearStart;
	if (aStartDate>*this)
		{
		yearStart.SetYearLeapCheck(y-1);
		aStartDate=yearStart;
		}
    return((DaysFrom(aStartDate).Int())+1) ;
    }

EXPORT_C TInt TTime::WeekNoInYear() const
/**
Gets the number of the current week in the year.

@return Week number in the year.
*/
	{

	return(WeekNoInYear(EFirstFourDayWeek));
	}

EXPORT_C TInt TTime::WeekNoInYear(TTime aStartDate) const
/**
Gets the number of the current week in the year when the year starts
on aStartDate. 

@param aStartDate If specified, indicates the date which is to be considered 
                  the start of the year. Default is 1st January.
                  
@return Week number in the year.
*/
	{
    
    return(WeekNoInYear(aStartDate,EFirstFourDayWeek));
	}

EXPORT_C TInt TTime::WeekNoInYear(TFirstWeekRule aRule) const
/**
Finds the number of the current week in the year using the first week rule 
specified in aRule. 

@param aRule Determines how the first week in the year is to be calculated. 
             By default EFirstFourDayWeek.
             
@return Week number in the year.
*/
	{
	
	TInt year=DateTime().Year();
	TTime startDate=TDateTime(year,EJanuary,0,0,0,0,0);
	return(WeekNoInYear(startDate,aRule));
	}

EXPORT_C TInt TTime::WeekNoInYear(TTime aStartDate,TFirstWeekRule aRule) const
//
// number of weeks between aTime and aStartDate according to given rule
// the first week starts either on the week containing the first day (EFirstWeek), 
// the first week having at least four days within the new year (EFirstFourDayWeek,
//  default) or the first full week in the year (EFirstFullWeek)
//
/**
Finds the number of the current week in the year when the year starts from 
aStartDate and when using the start week rule aRule.

@param aStartDate If specified, indicates the date which is to be considered 
                  the start of the year. Default is 1st January.
@param aRule      Determines how the first week in the year is to be
                  calculated. By default EFirstFourDayWeek.
                  
@return Week number in the year.
*/
	{                    
	TInt dayNoInWeek=DayNoInWeek();
	TInt dayNoInYear=(DayNoInYear(aStartDate))-1;    // puts start into correct year
	TDateTime startDateTime(aStartDate.DateTime());
	TDateTime nextYearStartDate(startDateTime);
	nextYearStartDate.SetYearLeapCheck(DateTime().Year());    // find start of next year
	TTime nextYearStartTime(nextYearStartDate);            // makes sure start date for year
	if (*this>nextYearStartTime)                           // is in the very next year
		{
		nextYearStartDate.SetYearLeapCheck(nextYearStartDate.Year()+1);
		nextYearStartTime=nextYearStartDate;
		}
	nextYearStartTime+=TTimeIntervalMicroSeconds(KDaysToMicroSeconds-1); // avoid problems if the time is not midnight
	TLocale local;
	TDay startOfFirstWeek=local.StartOfWeek();
	// calculate the day-in-week number (0 to 6) based on the locale start-of-week
	dayNoInWeek -= startOfFirstWeek;
	if (dayNoInWeek < 0)
		dayNoInWeek += 7;
	// calculate the days from the start-of-week to the start-of-next-year
	TInt daysFrom=nextYearStartTime.DaysFrom(*this).Int()+dayNoInWeek;
	// calculate the days from start-of-year to start-of-week (note this may be negative, but never < -6)
	TInt days=dayNoInYear-dayNoInWeek;

	// the rule allows a certain number of week-1 days to lie in the previous year
	TInt prevyeardays;
	switch (aRule)
		{
	default:
		return -1;
	case EFirstWeek:
		prevyeardays = 6;
		break;
	case EFirstFourDayWeek:
		prevyeardays = 3;
		break;
	case EFirstFullWeek:
		prevyeardays = 0;
		break;
		}

	// check for a week which belongs to last year
	if (days + prevyeardays < 0)
		{
		// in week 52 or 53 of last year, find the week # of the first day in the week
		startDateTime.SetYearLeapCheck(startDateTime.Year()-1);
		return (*this-TTimeIntervalDays(dayNoInWeek)).WeekNoInYear(TTime(startDateTime),aRule);
		}

	// check for a week which belongs to next year
	if (daysFrom <= prevyeardays)
		return 1;

	// calculate the week number, accounting for the requested week-1 rule
	return (days + 7 + prevyeardays)/7;
	}

EXPORT_C void TTime::FormatL(TDes &aDes,const TDesC &aFormat) const
//
// Fill aString with current Date and Time according to given aFormat string
//
/**
Puts this TTime into a descriptor and formats it according to the format string 
specified in the second argument.

Many of the formatting commands use the 
system's locale settings for the date and time, for example the characters 
used to separate components of the date and time and the ordering of day, 
month and year. The list of formatting commands below is divided into two 
sections, the first of which lists the commands which operate without reference 
to the locale's date and time settings (see class TLocale) and the second 
table lists the commands which do use these settings.

The following formatting commands do not honour the locale-specific system 
settings:

\%\% : Include a single '%' character in the string

\%* : Abbreviate following item (the following item should not be preceded 
by a '%' character).

\%C : Interpret the argument as the six digit microsecond component of the 
time. In its abbreviated form, ('%*C') this should be followed by an integer 
between zero and six, where the integer indicates the number of digits to display.

\%D : Interpret the argument as the two digit day number in the month. Abbreviation 
suppresses leading zero.

\%E : Interpret the argument as the day name. Abbreviation is language-specific 
(e.g. English uses the first three letters).

\%F : Use this command for locale-independent ordering of date components. 
This orders the following day/month/year component(s) (\%D, \%M, \%Y for example) 
according to the order in which they are specified in the string. This removes 
the need to use \%1 to \%5 (described below).

\%H : Interpret the argument as the one or two digit hour component of the 
time in 24 hour time format. Abbreviation suppresses leading zero. For locale-dependent 
hour formatting, use \%J.

\%I : Interpret the argument as the one or two digit hour component of the 
time in 12 hour time format. The leading zero is automatically suppressed 
so that abbreviation has no effect. For locale-dependent hour formatting, 
use \%J.

\%M : Interpret the argument as the one or two digit month number. Abbreviation 
suppresses leading zero.

\%N : Interpret the argument as the month name. Abbreviation is language specific, e.g. 
English uses the first three letters only. When using locale-dependent formatting, 
(that is, \%F has not previously been specified), specifying \%N causes any 
subsequent occurrence of a month specifier in the string to insert the month 
as text rather than in numeric form. When using locale-independent formatting, 
specifying \%N causes the month to be inserted as text at that position, but 
any subsequent occurrence of \%M will cause the month to be inserted in numeric 
form.

\%S : Interpret the argument as the one or two digit seconds component of the 
time. Abbreviation suppresses leading zero.

\%T : Interpret the argument as the one or two digit minutes component of the 
time. Abbreviation suppresses leading zero.

\%W : Interpret the argument as the one or two digit week number in year. Abbreviation 
suppresses leading zero.

\%X : Interpret the argument as the date suffix. Cannot be abbreviated. When 
using locale-dependent formatting (that is, \%F has not previously been specified), 
\%X causes all further occurrences of the day number to be displayed with the 
date suffix. When using locale-independent formatting, a date suffix will 
be inserted only after the occurrence of the day number which \%X follows in 
the format string. Any further occurrence of \%D without a following \%X will 
insert the day number without a suffix.

\%Y : Interpret the argument as the four digit year number. Abbreviation suppresses 
the first two digits.

\%Z : Interpret the argument as the one, two or three digit day number in the 
year. Abbreviation suppresses leading zeros.

The following formatting commands do honour the locale-specific system settings:

\%. : Interpret the argument as the decimal separator character (as set by 
TLocale::SetDecimalSeparator()). The decimal separator is used to separate 
seconds and microseconds, if present.

\%: : Interpret the argument as one of the four time separator characters (as 
set by TLocale::SetTimeSeparator()). Must be followed by an integer between 
zero and three inclusive to indicate which time separator character is being 
referred to.

\%/ : Interpret the argument as one of the four date separator characters (as 
set by TLocale::SetDateSeparator()). Must be followed by an integer between 
zero and three inclusive to indicate which date separator character is being 
referred to.

\%1 : Interpret the argument as the first component of a three component date 
(i.e. day, month or year) where the order has been set by TLocale::SetDateFormat(). 
When the date format is EDateEuropean, this is the day, when EDateAmerican, 
the month, and when EDateJapanese, the year. For more information on this 
and the following four formatting commands, see the Notes section immediately 
below.

\%2 : Interpret the argument as the second component of a three component date 
where the order has been set by TLocale::SetDateFormat(). When the date format 
is EDateEuropean, this is the month, when EDateAmerican, the day and when 
EDateJapanese, the month.

\%3 : Interpret the argument as the third component of a three component date 
where the order has been set by TLocale::SetDateFormat(). When the date format 
is EDateEuropean, or EDateAmerican this is the year and when EDateJapanese, 
the day.

\%4 : Interpret the argument as the first component of a two component date 
(day and month) where the order has been set by TLocale::SetDateFormat(). 
When the date format is EDateEuropean this is the day, and when EDateAmerican 
or EDateJapanese, the month.

\%5 : Interpret the argument as the second component of a two component date 
(day and month) where the order has been set by TLocale::SetDateFormat(). 
When the date format is EDateEuropean this is the month, and when EDateAmerican 
or EDateJapanese, the day.

\%A : Interpret the argument as "am" or "pm" text according to the current 
language and time of day. Unlike the \%B formatting command (described below), 
\%A disregards the locale's 12 or 24 hour clock setting, so that when used 
without an inserted + or - sign, am/pm text will always be displayed. Whether 
a space is inserted between the am/pm text and the time depends on the locale-specific 
settings. However, if abbreviated (\%*A), no space is inserted, regardless 
of the locale's settings. The am/pm text appears before or after the time, 
according to the position of the \%A, regardless of the locale-specific settings. 
For example, the following ordering of formatting commands causes am/pm text 
to be printed after the time: \%H \%T \%S \%A. Optionally, a minus or plus sign 
may be inserted between the "%" and the "A". This operates as follows:

\%-A causes am/pm text to be inserted into the descriptor only if the am/pm 
symbol position has been set in the locale to ELocaleBefore. Cannot be abbreviated 
using asterisk.

\%+A causes am/pm text to be inserted into the descriptor only if the am/pm 
symbol position has been set in the locale to ELocaleAfter. Cannot be abbreviated 
using asterisk. For example, the following formatting commands will cause 
am/pm text to be displayed after the time if the am/pm position has been set 
in the locale to ELocaleAfter or before the time if ELocaleBefore: \%-A \%H 
\%T \%S \%+A.

\%B Interpret the argument as am or pm text according to the current language 
and time of day. Unlike the \%A command, when using \%B, am/pm text is displayed 
only if the clock setting in the locale is 12-hour. Whether a space is inserted 
between the am/pm text and the time depends on the locale-specific settings. 
However, if abbreviated (\%*B), no space is inserted, regardless of the locale's 
settings. The am/pm text appears before or after the time, according to the 
location of the "%B", regardless of the locale-specific settings. For example, 
the following formatting commands cause am/pm text to be printed after the 
time: \%H \%T \%S \%B. Optionally, a minus or plus sign may be inserted between 
the "%" and the "B". This operates as follows:

\%-B causes am/pm text to be inserted into the descriptor only if using a 12 
hour clock and the am/pm symbol position has been set in the locale to ELocaleBefore. 
Cannot be abbreviated using asterisk.

\%+B causes am/pm text to be inserted into the descriptor only if using a 12 
hour clock and the am/pm symbol position has been set in the locale to ELocaleAfter. 
Cannot be abbreviated using asterisk. For example, the following formatting 
commands cause am/pm text to be printed after the time if the am/pm position 
has been set in the locale to ELocaleAfter or before the time if ELocaleBefore: 
\%-B \%H \%T \%S \%+B.

\%J Interpret the argument as the hour component of the time in either 12 or 
24 hour clock format depending on the locale's clock format setting. When 
the clock format has been set to 12 hour, leading zeros are automatically 
suppressed so that abbreviation has no effect. Abbreviation suppresses leading 
zero only when using a 24 hour clock.

Notes:

The \%1, \%2, \%3, \%4 and \%5 formatting commands are used in conjunction with 
\%D, \%M and \%Y to format the date locale-dependently. When formatting the date 
locale-dependently, the order of the day, month and year components within 
the string is determined by the order of the \%1 to \%5 formatting commands, 
not that of \%D, \%M, \%Y.

When formatting the date locale-independently (that is, \%F has been specified 
in the format string), the \%1 to \%5 formatting commands are not required, 
and should be omitted. In this case, the order of the date components is determined 
by the order of the \%D, \%M, \%Y format commands within aFormat.

Up to four date separators and up to four time separators can be used to separate 
the components of a date or time. When formatting a numerical date consisting 
of the day, month and year or a time containing hours, minutes and seconds, 
all four separators should always be specified in the format command string. 
Usually, the leading and trailing separators should not be displayed. In this 
case, the first and fourth separators should still be specified, but should 
be represented by a null character.

The date format follows the pattern:

DateSeparator[0] DateComponent1 DateSeparator[1] DateComponent2 DateSeparator[2] 
DateComponent3 DateSeparator[3]

where the ordering of date components is determined by the locale's date format 
setting.

The time format follows the pattern:

TimeSeparator[0] Hours TimeSeparator[1] Minutes TimeSeparator[2] Seconds TimeSeparator[3]

If the time includes a microseconds component, the third separator should 
occur after the microseconds, and the seconds and microseconds should be separated 
by the decimal separator. When formatting a two component time, the following 
rules apply:

if the time consists of hours and minutes, the third time delimiter should 
be omitted 

if the time consists of minutes and seconds, the second time delimiter should 
be omitted

@param aDes    Descriptor, which,  on return contains the formatted date/time string.
@param aFormat Format string which determines the format of the date and time.

@leave KErrOverflow The date/time string is too long for the descriptor aDes.
@leave KErrGeneral  A formatting error has occurred.
*/
	{
	TLocale local;
	FormatL(aDes,aFormat,local);
	}

EXPORT_C void TTime::FormatL(TDes &aDes,const TDesC &aFormat,const TLocale &aLocale) const
//
// Fill aString with current Date and Time according to given aFormat string
//
/**
Puts this TTime into a descriptor and formats it according to the format string 
specified in the second argument.

Many of the formatting commands use the 
system's locale settings for the date and time, for example the characters 
used to separate components of the date and time and the ordering of day, 
month and year. The list of formatting commands below is divided into two 
sections, the first of which lists the commands which operate without reference 
to the locale's date and time settings (see class TLocale) and the second 
table lists the commands which do use these settings.

The following formatting commands do not honour the locale-specific system 
settings:

\%\% : Include a single '%' character in the string

\%* : Abbreviate following item (the following item should not be preceded 
by a '%' character).

\%C : Interpret the argument as the six digit microsecond component of the 
time. In its abbreviated form, ('%*C') this should be followed by an integer 
between zero and six, where the integer indicates the number of digits to display.

\%D : Interpret the argument as the two digit day number in the month. Abbreviation 
suppresses leading zero.

\%E : Interpret the argument as the day name. Abbreviation is language-specific 
(e.g. English uses the first three letters).

\%F : Use this command for locale-independent ordering of date components. 
This orders the following day/month/year component(s) (\%D, \%M, \%Y for example) 
according to the order in which they are specified in the string. This removes 
the need to use \%1 to \%5 (described below).

\%H : Interpret the argument as the one or two digit hour component of the 
time in 24 hour time format. Abbreviation suppresses leading zero. For locale-dependent 
hour formatting, use \%J.

\%I : Interpret the argument as the one or two digit hour component of the 
time in 12 hour time format. The leading zero is automatically suppressed 
so that abbreviation has no effect. For locale-dependent hour formatting, 
use \%J.

\%M : Interpret the argument as the one or two digit month number. Abbreviation 
suppresses leading zero.

\%N : Interpret the argument as the month name. Abbreviation is language specific, e.g. 
English uses the first three letters only. When using locale-dependent formatting, 
(that is, \%F has not previously been specified), specifying \%N causes any 
subsequent occurrence of a month specifier in the string to insert the month 
as text rather than in numeric form. When using locale-independent formatting, 
specifying \%N causes the month to be inserted as text at that position, but 
any subsequent occurrence of \%M will cause the month to be inserted in numeric 
form.

\%S : Interpret the argument as the one or two digit seconds component of the 
time. Abbreviation suppresses leading zero.

\%T : Interpret the argument as the one or two digit minutes component of the 
time. Abbreviation suppresses leading zero.

\%W : Interpret the argument as the one or two digit week number in year. Abbreviation 
suppresses leading zero.

\%X : Interpret the argument as the date suffix. Cannot be abbreviated. When 
using locale-dependent formatting (that is, \%F has not previously been specified), 
\%X causes all further occurrences of the day number to be displayed with the 
date suffix. When using locale-independent formatting, a date suffix will 
be inserted only after the occurrence of the day number which \%X follows in 
the format string. Any further occurrence of \%D without a following \%X will 
insert the day number without a suffix.

\%Y : Interpret the argument as the four digit year number. Abbreviation suppresses 
the first two digits.

\%Z : Interpret the argument as the one, two or three digit day number in the 
year. Abbreviation suppresses leading zeros.

The following formatting commands do honour the locale-specific system settings:

\%. : Interpret the argument as the decimal separator character (as set by 
TLocale::SetDecimalSeparator()). The decimal separator is used to separate 
seconds and microseconds, if present.

\%: : Interpret the argument as one of the four time separator characters (as 
set by TLocale::SetTimeSeparator()). Must be followed by an integer between 
zero and three inclusive to indicate which time separator character is being 
referred to.

\%/ : Interpret the argument as one of the four date separator characters (as 
set by TLocale::SetDateSeparator()). Must be followed by an integer between 
zero and three inclusive to indicate which date separator character is being 
referred to.

\%1 : Interpret the argument as the first component of a three component date 
(i.e. day, month or year) where the order has been set by TLocale::SetDateFormat(). 
When the date format is EDateEuropean, this is the day, when EDateAmerican, 
the month, and when EDateJapanese, the year. For more information on this 
and the following four formatting commands, see the Notes section immediately 
below.

\%2 : Interpret the argument as the second component of a three component date 
where the order has been set by TLocale::SetDateFormat(). When the date format 
is EDateEuropean, this is the month, when EDateAmerican, the day and when 
EDateJapanese, the month.

\%3 : Interpret the argument as the third component of a three component date 
where the order has been set by TLocale::SetDateFormat(). When the date format 
is EDateEuropean, or EDateAmerican this is the year and when EDateJapanese, 
the day.

\%4 : Interpret the argument as the first component of a two component date 
(day and month) where the order has been set by TLocale::SetDateFormat(). 
When the date format is EDateEuropean this is the day, and when EDateAmerican 
or EDateJapanese, the month.

\%5 : Interpret the argument as the second component of a two component date 
(day and month) where the order has been set by TLocale::SetDateFormat(). 
When the date format is EDateEuropean this is the month, and when EDateAmerican 
or EDateJapanese, the day.

\%A : Interpret the argument as "am" or "pm" text according to the current 
language and time of day. Unlike the \%B formatting command (described below), 
\%A disregards the locale's 12 or 24 hour clock setting, so that when used 
without an inserted + or - sign, am/pm text will always be displayed. Whether 
a space is inserted between the am/pm text and the time depends on the locale-specific 
settings. However, if abbreviated (\%*A), no space is inserted, regardless 
of the locale's settings. The am/pm text appears before or after the time, 
according to the position of the \%A, regardless of the locale-specific settings. 
For example, the following ordering of formatting commands causes am/pm text 
to be printed after the time: \%H \%T \%S \%A. Optionally, a minus or plus sign 
may be inserted between the "%" and the "A". This operates as follows:

\%-A causes am/pm text to be inserted into the descriptor only if the am/pm 
symbol position has been set in the locale to ELocaleBefore. Cannot be abbreviated 
using asterisk.

\%+A causes am/pm text to be inserted into the descriptor only if the am/pm 
symbol position has been set in the locale to ELocaleAfter. Cannot be abbreviated 
using asterisk. For example, the following formatting commands will cause 
am/pm text to be displayed after the time if the am/pm position has been set 
in the locale to ELocaleAfter or before the time if ELocaleBefore: \%-A \%H 
\%T \%S \%+A.

\%B Interpret the argument as am or pm text according to the current language 
and time of day. Unlike the \%A command, when using \%B, am/pm text is displayed 
only if the clock setting in the locale is 12-hour. Whether a space is inserted 
between the am/pm text and the time depends on the locale-specific settings. 
However, if abbreviated (\%*B), no space is inserted, regardless of the locale's 
settings. The am/pm text appears before or after the time, according to the 
location of the "%B", regardless of the locale-specific settings. For example, 
the following formatting commands cause am/pm text to be printed after the 
time: \%H \%T \%S \%B. Optionally, a minus or plus sign may be inserted between 
the "%" and the "B". This operates as follows:

\%-B causes am/pm text to be inserted into the descriptor only if using a 12 
hour clock and the am/pm symbol position has been set in the locale to ELocaleBefore. 
Cannot be abbreviated using asterisk.

\%+B causes am/pm text to be inserted into the descriptor only if using a 12 
hour clock and the am/pm symbol position has been set in the locale to ELocaleAfter. 
Cannot be abbreviated using asterisk. For example, the following formatting 
commands cause am/pm text to be printed after the time if the am/pm position 
has been set in the locale to ELocaleAfter or before the time if ELocaleBefore: 
\%-B \%H \%T \%S \%+B.

\%J Interpret the argument as the hour component of the time in either 12 or 
24 hour clock format depending on the locale's clock format setting. When 
the clock format has been set to 12 hour, leading zeros are automatically 
suppressed so that abbreviation has no effect. Abbreviation suppresses leading 
zero only when using a 24 hour clock.

Notes:

The \%1, \%2, \%3, \%4 and \%5 formatting commands are used in conjunction with 
\%D, \%M and \%Y to format the date locale-dependently. When formatting the date 
locale-dependently, the order of the day, month and year components within 
the string is determined by the order of the \%1 to \%5 formatting commands, 
not that of \%D, \%M, \%Y.

When formatting the date locale-independently (that is, \%F has been specified 
in the format string), the \%1 to \%5 formatting commands are not required, 
and should be omitted. In this case, the order of the date components is determined 
by the order of the \%D, \%M, \%Y format commands within aFormat.

Up to four date separators and up to four time separators can be used to separate 
the components of a date or time. When formatting a numerical date consisting 
of the day, month and year or a time containing hours, minutes and seconds, 
all four separators should always be specified in the format command string. 
Usually, the leading and trailing separators should not be displayed. In this 
case, the first and fourth separators should still be specified, but should 
be represented by a null character.

The date format follows the pattern:

DateSeparator[0] DateComponent1 DateSeparator[1] DateComponent2 DateSeparator[2] 
DateComponent3 DateSeparator[3]

where the ordering of date components is determined by the locale's date format 
setting.

The time format follows the pattern:

TimeSeparator[0] Hours TimeSeparator[1] Minutes TimeSeparator[2] Seconds TimeSeparator[3]

If the time includes a microseconds component, the third separator should 
occur after the microseconds, and the seconds and microseconds should be separated 
by the decimal separator. When formatting a two component time, the following 
rules apply:

if the time consists of hours and minutes, the third time delimiter should 
be omitted 

if the time consists of minutes and seconds, the second time delimiter should 
be omitted

@param aDes    Descriptor, which,  on return contains the formatted date/time string.
@param aFormat Format string which determines the format of the date and time.
@param aLocale Specific locale which formatting will be based on.

@leave KErrOverflow The date/time string is too long for the descriptor aDes.
@leave KErrGeneral  A formatting error has occurred.
*/
	{

	TDateTime dateTime=DateTime();
	aDes.Zero(); // ensure string is empty at start

 	TLex aFmt(aFormat);
	TBool fix=EFalse; // fixed date format
	TBool da=EFalse; // day unabreviated
	TBool ma=EFalse; // month unabreviated
	TBool ya=EFalse; // year unabreviated
	TBool suff=EFalse; // default no suffix
	TBool mnam=EFalse; // default month as a number
	TTimeOverflowLeave overflowLeave;

   	while (!aFmt.Eos())
		{
		TChar ch=aFmt.Get();
		TBool abb=EFalse;
		const TInt NoPosSpecified=-1;
		TInt pos=NoPosSpecified;
		if (ch=='%')
			ch=aFmt.Get();
		else // not formatting,just want to add some characters to string
			goto doAppend; 
		if (ch=='*') // => abbreviate next field
			{
			abb=ETrue;
			ch=aFmt.Get();
			}
		else if (ch=='+' || ch=='-') // => leading or following Am/Pm
			{
			pos= ((ch=='+') ? ELocaleAfter : ELocaleBefore);
			ch=aFmt.Get();
			if (ch!='A' && ch!='B')
				User::Leave(KErrGeneral);
			}
		switch (ch)
			{
		case ':': // local time separator
				{
				if (aDes.Length()==aDes.MaxLength())
					User::Leave(KErrOverflow);
				ch=aFmt.Get();//Which separator?
				if (ch<'0' || ch>='0'+KMaxTimeSeparators)
					User::Leave(KErrGeneral);
				ch-='0';
				TChar separator=aLocale.TimeSeparator(ch);
				if (separator!=0)
					aDes.Append(separator);
				}
			break;
		case '/': // local date separator
				{
				if (aDes.Length()==aDes.MaxLength())
					User::Leave(KErrOverflow);
				ch=aFmt.Get();//Which separator?
				if (ch<'0' || ch>='0'+KMaxDateSeparators)
					User::Leave(KErrGeneral);
				ch-='0';
				TChar separator=aLocale.DateSeparator(ch);
				if (separator!=0)
					aDes.Append(separator);
				}
			break;
		case '.': // local decimal separator
				{
				if (aDes.Length()==aDes.MaxLength())
					User::Leave(KErrOverflow);
				aDes.Append(aLocale.DecimalSeparator());
				}
			break;
		case '1': // 1st element of date,local order
			switch (aLocale.DateFormat())
				{
			case EDateAmerican:
				goto doMonth;
			case EDateJapanese:
				goto doYear;
			default: // European
				goto doDay;
				}
		case '2': // 2nd element of date,local order
			switch (aLocale.DateFormat())
				{
			case EDateAmerican:
				goto doDay;
			default: // European and Japanese have month second
				goto doMonth;
				}
		case '3': // 3rd element of date,local order
			switch (aLocale.DateFormat())
				{
			case EDateJapanese:
				goto doDay;
			default: // European and American have year last
				goto doYear;
				}
		case '4': // 1st element of date (no year),local order
			switch (aLocale.DateFormat())
				{
			case EDateEuropean:
				goto doDay;
			default:
				goto doMonth;
				}
		case '5': // 2nd element of date (no year),local order
			switch (aLocale.DateFormat())
				{
			case EDateEuropean:
				goto doMonth;
			default:
				goto doDay;
				}
		case 'A': // am/pm text
doAmPm:
            {
            if (pos==NoPosSpecified || pos==aLocale.AmPmSymbolPosition())
				{
				TBuf<KMaxAmPmName+1> format(_S("%S"));
				if (!abb && aLocale.AmPmSpaceBetween())
					{
					if (aLocale.AmPmSymbolPosition()==ELocaleBefore)
						format.Append(' ');
					else
						{
						if (aDes.Length()==aDes.MaxLength())
							User::Leave(KErrOverflow);
						aDes.Append(' ');
						}
					}
				TAmPmName amPm((dateTime.Hour()<12) ? EAm : EPm);
				aDes.AppendFormat(format,&overflowLeave,&amPm);
				}
			break;
            }
		case 'B': // am/pm text if local time format is 12 hour clock
			if (aLocale.TimeFormat()==ETime24)
				break;
			else
				goto doAmPm;
		case 'C':
			{
			TBuf<6> digits;
			digits.AppendFormat(_L("%06d"),dateTime.MicroSecond());
			TUint numChars=6;	// Default length
			if (abb)
				{
				ch=aFmt.Get();
				if (ch>='0' && ch<='6')
					{
					numChars=ch;
					numChars-='0';
					}
				}
			if (aDes.Length()>(TInt)(aDes.MaxLength()-numChars))
			    User::Leave(KErrOverflow);
			aDes.Append(digits.Left(numChars));
			}	
			break;
		case 'D': // day in date
			if (abb)
				da=ETrue;
			if (!fix)
				break;
			else
				{
doDay:
				aDes.AppendFormat((da||abb) ? _L("%d"):_L("%02d"),&overflowLeave,dateTime.Day()+1);
				if (suff)
doSuffix:
                    {
                    TDateSuffix day(dateTime.Day());
					aDes.AppendFormat(_L("%S"),&overflowLeave,&day);
                    }
				break;
				}
		case 'E': // Day name
            {
			TDay day=DayNoInWeek();
			if (abb)
				{
	            TDayNameAbb nameAbb(day);
				aDes.AppendFormat(_L("%S"),&overflowLeave,&nameAbb);
				}
			else
				{
	            TDayName name(day);
				aDes.AppendFormat(_L("%S"),&overflowLeave,&name);
				}
			break;
            }
		case 'F': // => user wants day,month,year order fixed
			fix=ETrue;
			break;
		case 'H': // hour in 24 hour time format
do24:
			aDes.AppendFormat((abb) ? _L("%d"):_L("%02d"),&overflowLeave,dateTime.Hour());
			break;
		case 'I': // hour in 12 hour time format
do12:
			{
			TInt hour=dateTime.Hour();
			if (hour==0)
				hour=12;
			else if (hour>12)
				hour-=12;
			aDes.AppendFormat(_L("%d"),&overflowLeave,hour);
			break;
			}
		case 'J': //default time format for hour
			if (aLocale.TimeFormat()==ETime12)
				goto do12;
			else
				goto do24;
		case 'M': // month as a number (default value)
			if (abb)
				ma=ETrue;
			if (fix)
				goto doMonth;
			break;
		case 'N': // month as a name
			mnam=ETrue;
			if (abb)
				ma=ETrue;
			if (!fix)
				break;
			else
				{
doMonth:
				if (mnam)
					{
					TMonth month=dateTime.Month();
					if (ma || abb)
						{
		                TMonthNameAbb nameAbb(month);
						aDes.AppendFormat(_L("%S"),&overflowLeave,&nameAbb);
						}
					else
						{
	                    TMonthName name(month);
						aDes.AppendFormat(_L("%S"),&overflowLeave,&name);
						}
					}
				else
					aDes.AppendFormat((ma||abb) ? _L("%d"):_L("%02d"),&overflowLeave,dateTime.Month()+1);
				break;
				}
		case 'S': // seconds
			aDes.AppendFormat((abb) ? _L("%d"):_L("%02d"),&overflowLeave,dateTime.Second());
			break;	
		case 'T': // minutes	
			aDes.AppendFormat((abb) ? _L("%d"):_L("%02d"),&overflowLeave,dateTime.Minute());
			break;
		case 'W': // week no in year
			aDes.AppendFormat((abb) ? _L("%d"):_L("%02d"),&overflowLeave,WeekNoInYear());
			break;
		case 'X': // => wants day suffix
			if (fix)
				goto doSuffix;
			else
				{
				suff=ETrue;
				break;
				}
		case 'Y': // year
			if (abb)
				ya=ETrue;
			if (!fix)
				break;
			else
				{
doYear:
				if (ya || abb)
					aDes.AppendFormat(_L("%02d"),&overflowLeave,((dateTime.Year())%100));
				else
					aDes.AppendFormat(_L("%04d"),&overflowLeave,dateTime.Year());
				break;
				}
		case 'Z': // day no in year
			aDes.AppendFormat((abb) ? _L("%d"):_L("%03d"),&overflowLeave,DayNoInYear());
			break;
		default:
doAppend:
			if (aDes.Length()==aDes.MaxLength())
				User::Leave(KErrOverflow);
			aDes.Append(ch);
			break;
			}
		}
	}




EXPORT_C TTime Time::NullTTime()
/**
Gets a TTime with a null value.

@return TTime object with a null value.
*/
	{
	return UI64LIT(0x8000000000000000);
	}

EXPORT_C TTime Time::MaxTTime()
/**
Gets the maximum time value which can be held in a TTime object.

@return The maximum TTime value.
*/
	{
	return I64LIT(0x7fffffffffffffff);
	}

EXPORT_C TTime Time::MinTTime()
/**
Gets the minimum time value which can be held in a TTime object.

@return The minimum TTime value.
*/
	{
	return UI64LIT(0x8000000000000001);
	}

EXPORT_C TInt Time::DaysInMonth(TInt aYear,TMonth aMonth)
/**
Gets the number of days in a month.

@param aYear  The year. Must be specified because of leap years.
@param aMonth Month, from EJanuary to EDecember.

@return The number of days in the month.
*/
	{

    __ASSERT_DEBUG(aMonth<=EDecember && aMonth>=EJanuary,::Panic(ETTimeValueOutOfRange));
    return(mTab[IsLeapYear(aYear)][aMonth]);
	}

EXPORT_C TBool Time::IsLeapYear(TInt aYear)
//
// up to and including 1600 leap years were every 4 years; since then leap years are every 4 years unless
// the year falls on a century which is not divisible by 4 (i.e. 1900 wasn't, 2000 will be)
// for simplicity define year 0 as a leap year
//
/**
Tests whether a year is a leap year.

@param aYear The year of interest.

@return True if leap year, False if not.
*/
	{

	if (aYear>1600)
    	return(!(aYear%4) && (aYear%100 || !(aYear%400)));
	return(!(aYear%4));
	}

EXPORT_C TInt Time::LeapYearsUpTo(TInt aYear)
//
// from year 0 to specified year according to the rule above
//
/**
Gets the number of leap years between year 0 (nominal Gregorian) and the specified 
year, inclusive.

The calendar used is nominal Gregorian with astronomical numbering (where
year 2000 = 2000 AD, year 1600 = 1600 AD, year 1 = 1 AD, and so year 0 =
1 BC, year -100 = 101 BC).  No days are removed from September 1752 or any
other month.  Leap year calculation before 1600 uses the Julian method of
every four years, even for years which are exactly divisible by 100 but not
by 400.  Thus leap years include: 1200, 1300, 1400, 1500, 1600 and 2000;
non-leap years include: 1601, 1700, 1800, 1900 and 2100.

@param aYear The final year in the range to search. If negative, the function 
             will return a negative number of leap years.

@return The number of leap years between year 0 (nominal Gregorian) and aYear.
*/
	{

	if (aYear<=0)
		return(aYear/4);
	if (aYear<=1600)
		return(1+((aYear-1)/4));
	TInt num=401; // 1600/4+1
	aYear-=1601;
	TInt century=aYear/100;
	num+=(aYear/4-century+century/4);
	return(num);
	}
