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
// e32test\datetime\t_time.cpp
// Overview:
// Date & time tests
// API Information:
// TDateTime, TTimeInterval...
// Details:
// - Set various locale settings to known values.
// - Test the TDateTime class by checking year, month, day, hour, minute,
// second and microsecond ranges then verify setting individual ranges.
// - Test TTimeIntervalMicroSeconds class: verify construction and initialization.
// Verify the "=", "<", ">", "!=", ">=" and "<=" operators.
// - Test TTimeIntervalSeconds class: verify construction and initialization.
// Verify the "=", "<", ">", "!=", ">=" and "<=" operators.
// - Test TTimeIntervalMinutes, TTimeIntervalHours, TTimeIntervalDays, 
// TTimeIntervalMonths and TTimeIntervalYears classes: verify construction,
// initialization and "=" operator.
// - Test conversions between TDateTime and TTime objects.
// - Test adding and differencing between TDateTime and TTime objects. Including
// the methods: YearsFrom, MonthsFrom, DaysFrom, HoursFrom, MinutesFrom, 
// SecondsFrom and MicroSecondsFrom. Also specific tests for adding months, 
// adding days, adding hours, adding minutes, adding seconds, adding microseconds 
// and invalid differences. 
// - Test adding and subtracting different TTimeIntervals and verify the results.
// - Test TTime's date property functions. Verify results are as expected.
// - Test different date formats and string parsing. Verify results are as expected.
// - Test a variety of time change scenarios and verify results are as expected.
// - Test the TTime::Set() method with different combinations of data, verify the
// results are as expected.
// - Test a variety of operations involving negative times. Verify results are 
// as expected.
// - Test year 2000 and print the results.
// - Test secure clock is not affected by changes 
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32debug.h>
#include <hal.h>

LOCAL_D RTest test(_L("T_TIME"));

//
// duplication of local variable in UC_TIME
LOCAL_D const TInt8 mTab[2][12]=
    {
    {31,28,31,30,31,30,31,31,30,31,30,31}, // 28 days in Feb
    {31,29,31,30,31,30,31,31,30,31,30,31}  // 29 days in Feb
    };
const TInt64 KDaysToMicroSeconds(MAKE_TINT64(20,500654080));
const TInt64 KHoursToMicroSeconds(3600000000u);
const TInt KSecondsToMicroSeconds=1000000;

class TestTTime
	{
public:
	void Test1(void);
    void Test2(void);
	void Test3(void);
	void Test4(void);
	void Test5(void);
	void Test6(void);
	void Test7(void);
	void Test8(void);
	void Test9(void);
	void Test10(void);
	void Test11(void);
	void Test12(void);
 	void Test13(void);
	void TestSecureClock(void);
	};


void PrintTime(char* sz, TTime& t)
{
	TDateTime dateTime(t.DateTime());
	RDebug::Printf("%s%+02d/%+02d/%+04d %+02d:%+02d:%+02d.%+06d", sz, dateTime.Day()+1,dateTime.Month()+1,dateTime.Year(),dateTime.Hour(),dateTime.Minute(),dateTime.Second(),dateTime.MicroSecond());
}

void TestTTime::Test1(void)
//
// Tests for TDateTime
//
	{
	TInt year=1980; //leap year
	TMonth month=EJanuary;
	TInt day=0;
	TInt hour=0;
	TInt minute=0;
	TInt second=0;
	TInt microSecond=10;

	TDateTime dateTime(year,month,day,hour,minute,second,microSecond);

	test.Next(_L("Testing year ranges"));
    TInt ii;
	for (ii=1970; ii<2100; ii++)
		{
		test(dateTime.Set(ii,ENovember,day,hour,minute,second,microSecond)==KErrNone);
		TTime time(dateTime);
		TDateTime dateTime2(time.DateTime());
		test(dateTime2.Year()==ii);
		test(dateTime2.Month()==ENovember);
		test(dateTime2.Day()==day);
		test(dateTime2.Hour()==hour);
		test(dateTime2.Minute()==minute);
		test(dateTime2.Second()==second);
		test(dateTime2.MicroSecond()==microSecond);
		}

	test.Next(_L("Testing month ranges"));
 	for (ii=0; ii<12; ii++)
		test(dateTime.Set(year,TMonth(ii),day,hour,minute,second,microSecond)==0);
	test(dateTime.Set(year,TMonth(12),day,hour,minute,second,microSecond)!=0);

	test.Next(_L("Testing day ranges"));
	for (ii=0; ii<12; ii++)
		{
		test(dateTime.Set(year,TMonth(ii),(mTab[1][ii]-1),hour,minute,second,microSecond)==0);
		test(dateTime.Set(year+1,TMonth(ii),(mTab[0][ii]-1),hour,minute,second,microSecond)==0);
		test(dateTime.Set(year+1,TMonth(ii),(mTab[0][ii]),hour,minute,second,microSecond)!=0);
		}
	test(dateTime.Set(year,month,-1,hour,minute,second,microSecond)!=0);

	test.Next(_L("Testing hour ranges"));
	for (ii=0; ii<24; ii++)
		test(dateTime.Set(year,EMarch,10,ii,minute,second,microSecond)==0);
	test(dateTime.Set(year,EMarch,10,-1,minute,second,microSecond)!=0); 
	test(dateTime.Set(year,EMarch,10,24,minute,second,microSecond)!=0); 

	test.Next(_L("Testing minute ranges"));
	for (ii=0; ii<60; ii++)
		test(dateTime.Set(year,EMarch,0,0,ii,second,microSecond)==0);
	test(dateTime.Set(year,EMarch,0,0,-1,second,microSecond)!=0);
	test(dateTime.Set(year,EMarch,0,0,60,second,microSecond)!=0);

	test.Next(_L("Testing second ranges"));
	for (ii=0; ii<60; ii++)
		test(dateTime.Set(year,EMarch,0,0,0,ii,microSecond)==0);
	test(dateTime.Set(year,EMarch,0,0,0,-1,microSecond)!=0);
	test(dateTime.Set(year,EMarch,0,0,0,60,microSecond)!=0);

	test.Next(_L("Testing microsecond ranges"));
	for (ii=0; ii<100; ii++)
		test(dateTime.Set(year,EMarch,0,0,0,0,ii)==0);
	test(dateTime.Set(year,EMarch,0,0,0,0,-1)!=0);
	test(dateTime.Set(year,EMarch,0,0,0,0,1000000)!=0);

	test.Next(_L("Testing setting individual ranges"));

	dateTime.Set(year,month,day,hour,minute,second,microSecond);
	year=1984;
	test(dateTime.SetYear(year)==0);
	test(dateTime.Year()==year);
	test(dateTime.Month()==month);
	test(dateTime.Day()==day);
	test(dateTime.Hour()==hour);
	test(dateTime.Minute()==minute);
	test(dateTime.Second()==second);
	test(dateTime.MicroSecond()==microSecond);
	month=EFebruary;
	test(dateTime.SetMonth(month)==0);
	test(dateTime.SetYear(year)==0);
	test(dateTime.Year()==year);
	test(dateTime.Month()==month);
	test(dateTime.Day()==day);
	test(dateTime.Hour()==hour);
	test(dateTime.Minute()==minute);
	test(dateTime.Second()==second);
	test(dateTime.MicroSecond()==microSecond);
	day=28;
	test(dateTime.SetDay(day)==0);
	test(dateTime.SetYear(year)==0);
	test(dateTime.Year()==year);
	test(dateTime.Month()==month);
	test(dateTime.Day()==day);
	test(dateTime.Hour()==hour);
	test(dateTime.Minute()==minute);
	test(dateTime.Second()==second);
	test(dateTime.MicroSecond()==microSecond);
	hour=12;
	test(dateTime.SetHour(hour)==0);
	test(dateTime.SetYear(year)==0);
	test(dateTime.Year()==year);
	test(dateTime.Month()==month);
	test(dateTime.Day()==day);
	test(dateTime.Hour()==hour);
	test(dateTime.Minute()==minute);
	test(dateTime.Second()==second);
	test(dateTime.MicroSecond()==microSecond);
	minute=57;
	test(dateTime.SetMinute(minute)==0);
	test(dateTime.SetYear(year)==0);
	test(dateTime.Year()==year);
	test(dateTime.Month()==month);
	test(dateTime.Day()==day);
	test(dateTime.Hour()==hour);
	test(dateTime.Minute()==minute);
	test(dateTime.Second()==second);
	test(dateTime.MicroSecond()==microSecond);
	second=2;
	test(dateTime.SetSecond(second)==0);
	test(dateTime.SetYear(year)==0);
	test(dateTime.Year()==year);
	test(dateTime.Month()==month);
	test(dateTime.Day()==day);
	test(dateTime.Hour()==hour);
	test(dateTime.Minute()==minute);
	test(dateTime.Second()==second);
	test(dateTime.MicroSecond()==microSecond);
	microSecond=99999;
	test(dateTime.SetMicroSecond(microSecond)==0);
	test(dateTime.SetYear(year)==0);
	test(dateTime.Year()==year);
	test(dateTime.Month()==month);
	test(dateTime.Day()==day);
	test(dateTime.Hour()==hour);
	test(dateTime.Minute()==minute);
	test(dateTime.Second()==second);
	test(dateTime.MicroSecond()==microSecond);

	test(dateTime.SetYear(1981)!=0);
	test(dateTime.SetMonth((TMonth)15)!=0);
	test(dateTime.SetDay(-1)!=0);
	test(dateTime.SetHour(100)!=0);
	test(dateTime.SetMinute(-15)!=0);
	test(dateTime.SetSecond(60)!=0);
	test(dateTime.SetMicroSecond(-2)!=0);
	test(dateTime.Year()==year);
	test(dateTime.Month()==month);
	test(dateTime.Day()==day);
	test(dateTime.Hour()==hour);
	test(dateTime.Minute()==minute);
	test(dateTime.Second()==second);
	test(dateTime.MicroSecond()==microSecond);
	}

 void TestTTime::Test2(void)
 //
 // Tests for TTimeIntervalMicroSeconds
 //
	{
	test.Next(_L("Construction"));
	TTimeIntervalMicroSeconds t1; // uninitialised
	TTimeIntervalMicroSeconds t2(0);
	test(t2.Int64()==0 );
	TTimeIntervalMicroSeconds t3(1000000);
	test(t3.Int64()==1000000 );
	TTimeIntervalMicroSeconds t4(-452);
	test(t4.Int64()==-452 );				

	TTimeIntervalMicroSeconds  t5(MAKE_TINT64(0x7fffffff,0xffffffff));
    t5.Int64();

	test.Next(_L("operator ="));
	TInt num(1234);
	t1=num;
	t2=t1;
	test(t1.Int64()==t2.Int64());

	test.Next(_L("operator <"));
	test((t4<t1)!=0);
	test((t3<t2)==0);
	test((t2<t3)!=0);

	test.Next(_L("operator >"));
	test((t1>t4)!=0);
	test((t2>t3)==0);
	test((t2>t1)==0);

	test.Next(_L("operator !="));
	test((t1!=t3)!=0);
	test((t1!=t2)==0);

	test.Next(_L("operator >="));
	test((t3>=t4)!=0);
	test((t1>=t2)!=0);
	test((t1>=t3)==0);

	test.Next(_L("operator <="));
	test((t4<=t3)!=0);
	test((t1<=t2)!=0);
	test((t3<=t2)==0);
	}

void TestTTime::Test3(void)
//
// Tests for TTimeIntervaSeconds (and therefore TTimeIntervalBase)
//
	{
	test.Next(_L("Construction"));
    TTimeIntervalSeconds s1; // uninitialised
    TTimeIntervalSeconds s2(0);
    test(s2.Int()==0 );

    TTimeIntervalSeconds s3(1);
    test(s3.Int()==1 );
    test(s3.Int()!=0 );

    TTimeIntervalSeconds s4(-1);
    test(s4.Int()==-1 );				

 	TTimeIntervalSeconds s8(2147483647);
    test(s8.Int()== 2147483647);		

    test.Next(_L("operator ="));
    s1=0;
   	test(s1.Int()==0 );
    TTimeIntervalSeconds s5(5),s6;
    s6=s5;
    test(s5.Int()==s6.Int());
	s6=3;
	test(s5.Int()!=s6.Int());

	test.Next(_L("operator <"));
	test((s6<s3)==0);
	test((s3<s5)!=0);
	test((s4<s1)!=0);

	test.Next(_L("operator >"));
	test((s3>s6)==0);
	test((s5>s3)!=0);
	test((s1>s4)!=0);

	test.Next(_L("operator !="));
	s6=s5;
	test((s6!=s5)==0);
	test((s3!=s4)!=0);
	test((s1!=s2)==0);

	test.Next(_L("operator >="));
	test((s1>=s6)==0);
	test((s3>=s5)==0);
	test((s5>=s3)!=0);
	test((s6>=s5)!=0);
	test((s1>=s2)!=0);

	test.Next(_L("operator <="));
	test((s6<=s1)==0);
	test((s5<=s3)==0);
	test((s3<=s5)!=0);
	test((s6<=s5)!=0); 
	test((s1<=s2)!=0);
	}

void TestTTime::Test4()
//
// Tests for all other time intervals
//
	{
	test.Next(_L("TTimeIntervalMinutes"));
    test.Next(_L("Construction"));
    TTimeIntervalMinutes m1; // uninitialised
    TTimeIntervalMinutes m2(0);
    test(m2.Int()==0 );
    TTimeIntervalMinutes m3(1);
    test(m3.Int()==1 );
    test(m3.Int()!=0 );
    TTimeIntervalMinutes m4a(-1);
    test(m4a.Int()==-1 );				
    TTimeIntervalMinutes m4(0xffffffff);
    test((TUint)m4.Int()==0xffffffff);
    test.Next(_L("operator ="));
    m1=0;
   	test(m1.Int()==0 ); 
    TTimeIntervalMinutes m5(5),m6;
    m6=m5;
    test(m5.Int()==m6.Int());
    m6=3;
    m5=m6;
    test(m5.Int()==m6.Int());

	test.Next(_L("TTimeIntervalHours"));
    test.Next(_L("Construction"));
    TTimeIntervalHours h1; // uninitialised
    TTimeIntervalHours h2(0);
    test(h2.Int()==0 );
    TTimeIntervalHours h3(1);
    test(h3.Int()==1 );
    test(h3.Int()!=0 );
    TTimeIntervalHours h4a(-1);
    test(h4a.Int()==-1 );				
    TTimeIntervalHours h4(0xffffffff);
    test((TUint)h4.Int()==0xffffffff);
    test.Next(_L("operator ="));
    h1=0;
    test(h1.Int()==0 );
    TTimeIntervalHours h5(5),h6;
    h6=h5;
    test(h5.Int()==h6.Int());

	test.Next(_L("TTImeIntervalDays"));
    test.Next(_L("Construction"));
    TTimeIntervalDays d1; // uninitialised
   	TTimeIntervalDays d2(0);
    test(d2.Int()==0 );
    TTimeIntervalDays d3(1);
    test(d3.Int()==1 );
    test(d3.Int()!=0 );
    TTimeIntervalDays d4a(-1);
    test(d4a.Int()==-1 );				
    TTimeIntervalDays d4(0xffffffff);
    test((TUint)d4.Int()==0xffffffff);
    test.Next(_L("operator ="));
    d1=0;
   	test(d1.Int()==0 ); 
    TTimeIntervalDays d5(5),d6;
    d6=d5;
    test(d5.Int()==d6.Int());

    test.Next(_L("TTimeIntervalMonths"));
    test.Next(_L("Construction"));
    TTimeIntervalMonths mo1; // uninitialised
    TTimeIntervalMonths mo2(0);
    test(mo2.Int()==0 );
    TTimeIntervalMonths mo3(1);
    test(mo3.Int()==1 );
    test(mo3.Int()!=0 );
    TTimeIntervalMonths mo4a(-1);
    test(mo4a.Int()==-1 );				
    TTimeIntervalMonths mo4(0xffffffff);
    test((TUint)mo4.Int()==0xffffffff);
    test.Next(_L("operator ="));
    mo1=0;
   	test(mo1.Int()==0 );
    TTimeIntervalMonths mo5(5),mo6;
    mo6=mo5;
    test(mo5.Int()==mo6.Int());

    test.Next(_L("TTimeIntervalYears"));
    test.Next(_L("Construction"));
    TTimeIntervalYears y1; // uninitialised
    TTimeIntervalYears y2(0);
    test(y2.Int()==0 );
    TTimeIntervalYears y3(1);
    test(y3.Int()==1 );
    test(y3.Int()!=0 );
    TTimeIntervalYears y4a(-1);
    test(y4a.Int()==-1 );				
    TTimeIntervalYears y4(0xffffffff);
    test((TUint)y4.Int()==0xffffffff);
    test.Next(_L("operator ="));
    y1=0;
   	test(y1.Int()==0 );
    TTimeIntervalYears y5(17),y6;
    y6=y5;
    test(y5.Int()==y6.Int());
	y6=16;
	test(y5.Int()!=y6.Int());
	y5=16;
    test(y5.Int()==y6.Int());
    }


void TestTTime::Test5()
//
// TDateTime to TTime convertions and vice versa, very large loop, so in own function for easy removal
//
	{
	TInt microSecond=500000;
	TDateTime dateTime(0,EJanuary,0,0,0,0,microSecond);
    TInt year;
	for(year=1590;year<1710;year+=2)
		{
		dateTime.SetYear(year);
		for(TInt m=0;m<6;m++)
			{
			static TInt months[]={0,1,2,8,9,11};
			TInt month=months[m];
			dateTime.SetDay(0); // to make sure next line always suceeds
			dateTime.SetMonth((TMonth)month);
			for(TInt d=0;d<6;d++)
				{
				static TInt days[]={0,1,27,28,29,30};
				TInt day=days[d];
				if (day>=mTab[Time::IsLeapYear(year)][month])
					break;
				dateTime.SetDay(day);
				for(TInt h=0;h<4;h++)
					{
					static TInt hours[]={0,11,12,23};
					TInt hour=hours[h];
					dateTime.SetHour(hour);
					for(TInt minute=0;minute<60;minute+=59)
						{
						dateTime.SetMinute(minute);
						for(TInt second=0;second<60;second+=59)
							{
							dateTime.SetSecond(second);
							TTime tim(dateTime);
							dateTime = tim.DateTime();  
							test(dateTime.Year()==year);
							test(dateTime.Month()==(TMonth)month);
							test(dateTime.Day()==day);
							test(dateTime.Hour()==hour);
							test(dateTime.Minute()==minute);
							test(dateTime.Second()==second);
							test(dateTime.MicroSecond()==microSecond);
							}
						}
					}
				}
			}
		}

// smaller loop for -ve dates
	for (year=-150; year<5; year+=15)
		{
		dateTime.SetYear(year);
		for(TInt month=0; month<12; month+=5)
			{
			dateTime.SetDay(0); // to make sure next line always suceeds
			dateTime.SetMonth((TMonth)month);
			for(TInt day=0; day<30; day+=7)
				{
				if (day>=mTab[Time::IsLeapYear(year)][month])
					break;
				dateTime.SetDay(day);
				for(TInt hour=0; hour<24; hour+=6)
					{
					dateTime.SetHour(hour);
					for(TInt minute=0; minute<60; minute+=15)
						{
						dateTime.SetMinute(minute);
						for(TInt second=0; second<60; second+=20)
							{
							dateTime.SetSecond(second);
							TTime tim(dateTime);
							dateTime = tim.DateTime();  
							test(dateTime.Year()==year);
							test(dateTime.Month()==(TMonth)month);
							test(dateTime.Day()==day);
							test(dateTime.Hour()==hour);
							test(dateTime.Minute()==minute);
							test(dateTime.Second()==second);
							test(dateTime.MicroSecond()==microSecond);
							}
						}
					}
				}
			}
		}

	TTime tim(MAKE_TINT64(0x7fffffff,0xffffffff));
	dateTime = tim.DateTime();
	tim = dateTime;
	test(tim.Int64()==MAKE_TINT64(0x7fffffff,0xffffffff));
	}

void TestTTime::Test6()
//
// Adding and differencing
//
	{
	TDateTime dateTime(4,EJanuary,30,0,0,0,0);

	test.Next(_L("TTimeIntervalYears"));
	TTime base=dateTime;
	TTimeIntervalYears year(1);
	TTime result=base+year;
	dateTime=result.DateTime();																 												
	test(dateTime.Year()==5);
	test(dateTime.Month()==EJanuary);
	test(dateTime.Day()==30);
	test(result.YearsFrom(base)==year);
	year=2000;
	result+=year;
	dateTime=result.DateTime();
	test(dateTime.Year()==2005);
	test(dateTime.Month()==EJanuary);
	test(dateTime.Day()==30);
	test(result.YearsFrom(base)==TTimeIntervalYears(2001));
	test(base.YearsFrom(result)==TTimeIntervalYears(-2001));

	test.Next(_L("YearsFrom"));
	TTime timeNow;
	timeNow.HomeTime();
	TTime timeFuture=timeNow+TTimeIntervalYears(10);
	test(timeFuture.YearsFrom(timeNow).Int()==10);
	test(timeNow.YearsFrom(timeFuture).Int()==-10);
	TTime mintime = Time::MinTTime();
	test(timeNow.YearsFrom(mintime).Int()>0);//must be positive value
	test(mintime.YearsFrom(timeNow).Int()<0);//must be negative value
	TTime maxtime = Time::MaxTTime();
	test(timeNow.YearsFrom(maxtime).Int()<0);//must be negative value
	test(maxtime.YearsFrom(timeNow).Int()>0);//must be positive value

	test.Next(_L("Adding months"));
	TTimeIntervalMonths month(1);
	result=base+month;
	dateTime=result.DateTime();
	test(dateTime.Year()==4);
	test(dateTime.Month()==EFebruary);
	test(dateTime.Day()==28); // leap year
	test(result.YearsFrom(base)==TTimeIntervalYears(0));
	test(base.YearsFrom(result)==TTimeIntervalYears(0));
	test(result.MonthsFrom(base)==month);
	test(base.MonthsFrom(result)==TTimeIntervalMonths(-month.Int()));
	month=12;
	result+=month;
	dateTime=result.DateTime();
	test(dateTime.Year()==5);
	test(dateTime.Month()==EFebruary);
	test(dateTime.Day()==27); // not aleap year
	test(result.YearsFrom(base)==TTimeIntervalYears(1));
	test(base.YearsFrom(result)==TTimeIntervalYears(-1));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(13));
	test(base.MonthsFrom(result)==TTimeIntervalYears(-13));

	test.Next(_L("MonthsFrom"));
	timeNow.HomeTime();
	timeFuture=timeNow+TTimeIntervalMonths(10);
	test(timeFuture.MonthsFrom(timeNow).Int()==10);
	test(timeNow.MonthsFrom(timeFuture).Int()==-10);
	test(timeNow.MonthsFrom(mintime).Int()>0);//must be positive value
	test(mintime.MonthsFrom(timeNow).Int()<0);//must be negative value
	test(timeNow.MonthsFrom(maxtime).Int()<0);//must be negative value
	test(maxtime.MonthsFrom(timeNow).Int()>0);//must be positive value

	test.Next(_L("Adding days"));
	TTimeIntervalDays day(1);
	result=base+day;
	dateTime=result.DateTime();
	test(dateTime.Year()==4);
	test(dateTime.Month()==EFebruary);
	test(dateTime.Day()==0);
	test(result.YearsFrom(base)==TTimeIntervalYears(0));
	test(base.YearsFrom(result)==TTimeIntervalYears(0));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(0));
	test(base.MonthsFrom(result)==TTimeIntervalMonths(0));
	test(result.DaysFrom(base)==day);
	test(base.DaysFrom(result)==TTimeIntervalDays(-day.Int()));
	day=60;
	result+=day;
	dateTime=result.DateTime();
	test(dateTime.Year()==4);
	test(dateTime.Month()==EApril);
	test(dateTime.Day()==0);
	test(result.YearsFrom(base)==TTimeIntervalYears(0));
	test(base.YearsFrom(result)==TTimeIntervalYears(0));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(2));
	test(base.MonthsFrom(result)==TTimeIntervalMonths(-2));
	test(result.DaysFrom(base)==TTimeIntervalDays(61));
	test(base.DaysFrom(result)==TTimeIntervalDays(-61));

	test.Next(_L("DaysFrom"));
	timeNow.HomeTime();
	timeFuture=timeNow+TTimeIntervalDays(10);
	test(timeFuture.DaysFrom(timeNow).Int()==10);
	test(timeNow.DaysFrom(timeFuture).Int()==-10);
	test(timeNow.DaysFrom(mintime).Int()>0);//must be positive value
	test(mintime.DaysFrom(timeNow).Int()<0);//must be negative value	
	test(timeNow.DaysFrom(maxtime).Int()<0);//must be negative value
	test(maxtime.DaysFrom(timeNow).Int()>0);//must be positive value

	test.Next(_L("Adding hours"));
	TTimeIntervalHours hour(6);
	result=base+hour;
	dateTime=result.DateTime();
	test(dateTime.Year()==4);
	test(dateTime.Month()==EJanuary);
	test(dateTime.Day()==30);
	test(dateTime.Hour()==6);
	test(result.YearsFrom(base)==TTimeIntervalYears(0));
	test(base.YearsFrom(result)==TTimeIntervalYears(0));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(0));
	test(base.MonthsFrom(result)==TTimeIntervalMonths(0));
	test(result.DaysFrom(base)==TTimeIntervalDays(0));
	test(base.DaysFrom(result)==TTimeIntervalDays(0));
	TInt ret=result.HoursFrom(base,hour);
	test(ret==0);
	test(hour==TTimeIntervalHours(6));
	ret=base.HoursFrom(result,hour);
	test(ret==0);
	test(hour==TTimeIntervalHours(-6));
	hour=20;
	result+=hour;
	dateTime=result.DateTime();
	test(dateTime.Year()==4);
	test(dateTime.Month()==EFebruary);
	test(dateTime.Day()==0);
	test(dateTime.Hour()==2);
	test(result.YearsFrom(base)==TTimeIntervalYears(0));
	test(base.YearsFrom(result)==TTimeIntervalYears(0));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(0));
	test(base.MonthsFrom(result)==TTimeIntervalMonths(0));
	test(result.DaysFrom(base)==TTimeIntervalDays(1));
	test(base.DaysFrom(result)==TTimeIntervalDays(-1));
	ret=result.HoursFrom(base,hour);
	test(ret==0);
	test(hour==TTimeIntervalHours(26));
	ret=base.HoursFrom(result,hour);
	test(ret==0);
	test(hour==TTimeIntervalHours(-26));

	test.Next(_L("HoursFrom"));
	timeNow.HomeTime();
	timeFuture=timeNow+TTimeIntervalHours(10);
	test(timeFuture.HoursFrom(timeNow,hour)==KErrNone);
	test(hour.Int()==10);
	test(timeNow.HoursFrom(timeFuture,hour)==KErrNone);
	test(hour.Int()==-10); // fails this in 059
	timeFuture=timeNow+TTimeIntervalHours(KMaxTInt);
	test(timeFuture.HoursFrom(timeNow,hour)==KErrNone);
	test(hour.Int()==KMaxTInt);
	test(timeNow.HoursFrom(timeFuture,hour)==KErrNone);
	test(hour.Int()==-KMaxTInt);
	timeFuture=timeFuture+TTimeIntervalHours(1);
	test(timeFuture.HoursFrom(timeNow,hour)==KErrOverflow);
	test(timeNow.HoursFrom(timeFuture,hour)==KErrNone);
	test(hour.Int()==KMinTInt);
	timeFuture=timeFuture+TTimeIntervalHours(1);
	test(timeFuture.HoursFrom(timeNow,hour)==KErrOverflow);
	test(timeNow.HoursFrom(timeFuture,hour)==KErrOverflow);

	test.Next(_L("Adding minutes"));
	TTimeIntervalMinutes minute(73);
	result=base+minute;
	dateTime=result.DateTime();
	test(dateTime.Year()==4);
	test(dateTime.Month()==EJanuary);
	test(dateTime.Day()==30);
	test(dateTime.Hour()==1);
	test(dateTime.Minute()==13);
	test(result.YearsFrom(base)==TTimeIntervalYears(0));
	test(base.YearsFrom(result)==TTimeIntervalYears(0));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(0));
	test(base.MonthsFrom(result)==TTimeIntervalMonths(0));
	test(result.DaysFrom(base)==TTimeIntervalDays(0));
	test(base.DaysFrom(result)==TTimeIntervalDays(0));
	ret=result.HoursFrom(base,hour);
	test(ret==0);
	test(hour==TTimeIntervalHours(1));
	ret=base.HoursFrom(result,hour);
	test(ret==0);
	test(hour==TTimeIntervalHours(-1));
	ret=result.MinutesFrom(base,minute);
	test(ret==0);
	test(minute==TTimeIntervalMinutes(73));
	ret=base.MinutesFrom(result,minute);
	test(ret==0);
	test(minute==TTimeIntervalMinutes(-73));
	minute=1367;
	result+=minute;
	dateTime=result.DateTime();
	test(dateTime.Year()==4);
	test(dateTime.Month()==EFebruary);
	test(dateTime.Day()==0);
	test(dateTime.Hour()==0);
	test(dateTime.Minute()==0);
	test(result.YearsFrom(base)==TTimeIntervalYears(0));
	test(base.YearsFrom(result)==TTimeIntervalYears(0));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(0));
	test(base.MonthsFrom(result)==TTimeIntervalMonths(0));
	test(result.DaysFrom(base)==TTimeIntervalDays(1));
	test(base.DaysFrom(result)==TTimeIntervalDays(-1));
	ret=result.HoursFrom(base,hour);
	test(ret==0);
	test(hour==TTimeIntervalHours(24));
	ret=base.HoursFrom(result,hour);
	test(ret==0);
	test(hour==TTimeIntervalHours(-24));
	ret=result.MinutesFrom(base,minute);
	test(ret==0);
	test(minute==TTimeIntervalMinutes(1440));
	ret=base.MinutesFrom(result,minute);
	test(ret==0);
	test(minute==TTimeIntervalMinutes(-1440));

	test.Next(_L("MinutesFrom"));
	timeNow.HomeTime();
	timeFuture=timeNow+TTimeIntervalMinutes(10);
	test(timeFuture.MinutesFrom(timeNow,minute)==KErrNone);
	test(minute.Int()==10);
	test(timeNow.MinutesFrom(timeFuture,minute)==KErrNone);
	test(minute.Int()==-10); // fails this	in 059
	timeFuture=timeNow+TTimeIntervalMinutes(KMaxTInt);
	test(timeFuture.MinutesFrom(timeNow,minute)==KErrNone);
	test(minute.Int()==KMaxTInt);
	test(timeNow.MinutesFrom(timeFuture,minute)==KErrNone);
	test(minute.Int()==-KMaxTInt);
	timeFuture=timeFuture+TTimeIntervalMinutes(1);
	test(timeFuture.MinutesFrom(timeNow,minute)==KErrOverflow);
	test(timeNow.MinutesFrom(timeFuture,minute)==KErrNone);
	test(minute.Int()==KMinTInt);
	timeFuture=timeFuture+TTimeIntervalMinutes(1);
	test(timeFuture.MinutesFrom(timeNow,minute)==KErrOverflow);
	test(timeNow.MinutesFrom(timeFuture,minute)==KErrOverflow);

	test.Next(_L("Adding seconds"));
	TTimeIntervalSeconds second(305222);
	result=base+second;
	dateTime=result.DateTime();
	test(dateTime.Year()==4);
	test(dateTime.Month()==EFebruary);
	test(dateTime.Day()==2);
	test(dateTime.Hour()==12);
	test(dateTime.Minute()==47);
	test(dateTime.Second()==2);
	test(result.YearsFrom(base)==TTimeIntervalYears(0));
	test(base.YearsFrom(result)==TTimeIntervalYears(0));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(0));
	test(base.MonthsFrom(result)==TTimeIntervalMonths(0));
	test(result.DaysFrom(base)==TTimeIntervalDays(3));
	test(base.DaysFrom(result)==TTimeIntervalDays(-3));
	ret=result.HoursFrom(base,hour);
	test(ret==0);
	test(hour==TTimeIntervalHours(84));
	ret=base.HoursFrom(result,hour);
	test(ret==0);
	test(hour==TTimeIntervalHours(-84));
	ret=result.MinutesFrom(base,minute);
	test(ret==0);
	test(minute==TTimeIntervalMinutes(5087));
	ret=base.MinutesFrom(result,minute);
	test(ret==0);
	test(minute==TTimeIntervalMinutes(-5087));
	ret=result.SecondsFrom(base,second);
	test(ret==0);
	test(second==TTimeIntervalSeconds(305222));
	ret=base.SecondsFrom(result,second);
	test(ret==0);
	test(second==TTimeIntervalSeconds(-305222));
	second=58;
	result+=second;
	dateTime=result.DateTime();
	test(dateTime.Year()==4);
	test(dateTime.Month()==EFebruary);
	test(dateTime.Day()==2);
	test(dateTime.Hour()==12);
	test(dateTime.Minute()==48);
	test(dateTime.Second()==0);
	test(result.YearsFrom(base)==TTimeIntervalYears(0));
	test(base.YearsFrom(result)==TTimeIntervalYears(0));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(0));
	test(base.MonthsFrom(result)==TTimeIntervalMonths(0));
	test(result.DaysFrom(base)==TTimeIntervalDays(3));
	test(base.DaysFrom(result)==TTimeIntervalDays(-3));
	ret=result.HoursFrom(base,hour);
	test(ret==0);
	test(hour==TTimeIntervalHours(84));
	ret=base.HoursFrom(result,hour);
	test(ret==0);
	test(hour==TTimeIntervalHours(-84));
	ret=result.MinutesFrom(base,minute);
	test(ret==0);
	test(minute==TTimeIntervalMinutes(5088));
	ret=base.MinutesFrom(result,minute);
	test(ret==0);
	test(minute==TTimeIntervalMinutes(-5088));
	ret=result.SecondsFrom(base,second);
	test(ret==0);
	test(second==TTimeIntervalSeconds(305280));
	ret=base.SecondsFrom(result,second);
	test(ret==0);
	test(second==TTimeIntervalSeconds(-305280));

	test.Next(_L("SecondsFrom"));
	timeNow.HomeTime();
	timeFuture=timeNow+TTimeIntervalSeconds(10);
	test(timeFuture.SecondsFrom(timeNow,second)==KErrNone);
	test(second.Int()==10);
	test(timeNow.SecondsFrom(timeFuture,second)==KErrNone);
	test(second.Int()==-10);
	timeFuture=timeNow+TTimeIntervalSeconds(KMaxTInt);
	test(timeFuture.SecondsFrom(timeNow,second)==KErrNone);
	test(second.Int()==KMaxTInt);
	test(timeNow.SecondsFrom(timeFuture,second)==KErrNone);
	test(second.Int()==-KMaxTInt);
	timeFuture=timeFuture+TTimeIntervalSeconds(1);
	test(timeFuture.SecondsFrom(timeNow,second)==KErrOverflow);
	test(timeNow.SecondsFrom(timeFuture,second)==KErrNone);
	test(second.Int()==KMinTInt);
	timeFuture=timeFuture+TTimeIntervalSeconds(1);
	test(timeFuture.SecondsFrom(timeNow,second)==KErrOverflow);
	test(timeNow.SecondsFrom(timeFuture,second)==KErrOverflow);

	test.Next(_L("Adding microseconds"));
	TTimeIntervalMicroSeconds microsecond=KDaysToMicroSeconds+KHoursToMicroSeconds+MAKE_TINT64(0,5000);
	result=base+microsecond;
	dateTime=result.DateTime();
	test(dateTime.Year()==4);
	test(dateTime.Month()==EFebruary);
	test(dateTime.Day()==0);
	test(dateTime.Hour()==1);
	test(dateTime.Minute()==0);
	test(dateTime.Second()==0);
	test(dateTime.MicroSecond()==5000);
	test(result.YearsFrom(base)==TTimeIntervalYears(0));
	test(base.YearsFrom(result)==TTimeIntervalYears(0));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(0));
	test(base.MonthsFrom(result)==TTimeIntervalMonths(0));
	test(result.DaysFrom(base)==TTimeIntervalDays(1));
	test(base.DaysFrom(result)==TTimeIntervalDays(-1));
	ret=result.HoursFrom(base,hour);
	test(ret==0);
	test(hour==TTimeIntervalHours(25));
	ret=base.HoursFrom(result,hour);
	test(ret==0);
	test(hour==TTimeIntervalHours(-25));
	ret=result.MinutesFrom(base,minute);
	test(ret==0);
	test(minute==TTimeIntervalMinutes(1500));
	ret=base.MinutesFrom(result,minute);
	test(ret==0);
	test(minute==TTimeIntervalMinutes(-1500));
	ret=result.SecondsFrom(base,second);
	test(ret==0);
	test(second==TTimeIntervalSeconds(90000));
	ret=base.SecondsFrom(result,second);
	test(ret==0);
	test(second==TTimeIntervalSeconds(-90000));
	test(result.MicroSecondsFrom(base)==microsecond);
	microsecond=5008;
	result+=microsecond;
	dateTime=result.DateTime();
	test(dateTime.Year()==4);
	test(dateTime.Month()==EFebruary);
	test(dateTime.Day()==0);
	test(dateTime.Hour()==1);
	test(dateTime.Minute()==0);
	test(dateTime.Second()==0);
	test(dateTime.MicroSecond()==10008);

	test.Next(_L("MicroSecondsFrom"));
	timeNow.HomeTime();
	timeFuture=timeNow+TTimeIntervalMicroSeconds(10);
	test(timeFuture.MicroSecondsFrom(timeNow).Int64()==10);
	test(timeNow.MicroSecondsFrom(timeFuture).Int64()==-10);

	test.Next(_L("Testing invalid differences"));
	TInt64 overflow(KMaxTInt);
	overflow++;
	overflow*=KSecondsToMicroSeconds;
	result=base+TTimeIntervalMicroSeconds(overflow);
	ret=result.SecondsFrom(base,second);
	test(ret==KErrOverflow);
	overflow*=60;
	result=base+TTimeIntervalMicroSeconds(overflow);
	ret=result.MinutesFrom(base,minute);
	test(ret==KErrOverflow);
	overflow*=60;
	result=base+TTimeIntervalMicroSeconds(overflow);
	ret=result.HoursFrom(base,hour);
	test(ret==KErrOverflow);

	test.Next(_L("Specific MonthsFrom() tests"));

	base=TDateTime(1995,EJanuary,30,0,0,0,0);
	result=TDateTime(1995,EFebruary,27,0,0,0,0);
	test(result.MonthsFrom(base)==TTimeIntervalMonths(1));

	base=TDateTime(1995,EJanuary,27,0,0,0,0);
	result=TDateTime(1995,EFebruary,27,0,0,0,0);
	test(result.MonthsFrom(base)==TTimeIntervalMonths(1));

	base=TDateTime(1995,EJanuary,29,0,0,0,0);
	result=TDateTime(1995,EFebruary,27,0,0,0,0);
	test(result.MonthsFrom(base)==TTimeIntervalMonths(1));

	base=TDateTime(1995,EJanuary,30,0,0,0,0);
	result=TDateTime(1995,EFebruary,27,0,0,0,0);
	test(base.MonthsFrom(result)==TTimeIntervalMonths(-1));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(1));

	base=TDateTime(1995,EJanuary,27,0,0,0,0);
	result=TDateTime(1995,EFebruary,27,0,0,0,0);
	test(base.MonthsFrom(result)==TTimeIntervalMonths(-1));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(1));

	base=TDateTime(1995,EJanuary,29,0,0,0,0);
	result=TDateTime(1995,EFebruary,27,0,0,0,0);
	test(base.MonthsFrom(result)==TTimeIntervalMonths(-1));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(1));

	base=TDateTime(1995,EJanuary,26,0,0,0,0);
	result=TDateTime(1995,EFebruary,27,0,0,0,0);
	test(base.MonthsFrom(result)==TTimeIntervalMonths(-1));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(1));

	base=TDateTime(1995,EFebruary,27,0,0,0,0);
	result=TDateTime(1995,EMarch,29,0,0,0,0);
	test(base.MonthsFrom(result)==TTimeIntervalMonths(-1));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(1));

	base=TDateTime(1995,EFebruary,27,0,0,0,0);
	result=TDateTime(1995,EMarch,30,0,0,0,0);
	test(base.MonthsFrom(result)==TTimeIntervalMonths(-1));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(1));

	base=TDateTime(1995,EFebruary,27,13,0,0,0);
	result=TDateTime(1995,EJanuary,29,12,0,0,0);
	test(base.MonthsFrom(result)==TTimeIntervalMonths(1));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(-1));

	base=TDateTime(1995,EFebruary,27,12,0,0,0);
	result=TDateTime(1995,EJanuary,29,13,0,0,0);
	test(base.MonthsFrom(result)==TTimeIntervalMonths(0));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(0));

	base=TDateTime(1995,EJanuary,27,12,0,0,0);
	result=TDateTime(1995,EJanuary,29,13,0,0,0);
	test(base.MonthsFrom(result)==TTimeIntervalMonths(0));
	test(result.MonthsFrom(base)==TTimeIntervalMonths(0));

	test.Next(_L("Looped MonthsFrom() test"));
	const TTime endBase=MAKE_TINT64(74334524,25422354);
	const TTime endResult=MAKE_TINT64(154334524,25422354);
	const TTimeIntervalMicroSeconds plus=MAKE_TINT64(1234567,23453452);
	for (base=MAKE_TINT64(3563656,3456235623u);base<endBase;base+=plus)
		for (result=MAKE_TINT64(3563656,3456235623u);result<endResult;result+=plus)
			test(base.MonthsFrom(result).Int()==-result.MonthsFrom(base).Int()); 
	}

void TestTTime::Test7()
//
// subtracting
//
	{
	TDateTime dateTime(1996,EApril,3,0,0,0,0);
	TTime base(dateTime);
	TTime tim(base);

	tim+=TTimeIntervalYears(7);
	tim+=TTimeIntervalMonths(3);
	tim+=TTimeIntervalDays(40);
	tim+=TTimeIntervalHours(-3);
	tim+=TTimeIntervalMinutes(1234);
	tim+=TTimeIntervalSeconds(666);
	tim+=TTimeIntervalMicroSeconds(-876540);
	tim-=TTimeIntervalMicroSeconds(-876540);
	tim-=TTimeIntervalSeconds(666);
	tim-=TTimeIntervalMinutes(1234);
	tim-=TTimeIntervalHours(-3);
	tim-=TTimeIntervalDays(40);
	tim-=TTimeIntervalMonths(3);
	tim-=TTimeIntervalYears(7);
	test (base==tim);

	tim-=TTimeIntervalMicroSeconds(9999999);
	tim-=TTimeIntervalSeconds(52);
	tim-=TTimeIntervalMinutes(-13);
	tim-=TTimeIntervalHours(-337);
	tim-=TTimeIntervalDays(1010);
	tim-=TTimeIntervalMonths(-150);
	tim-=TTimeIntervalYears(337);
	tim+=TTimeIntervalYears(337);
	tim+=TTimeIntervalMonths(-150);
	tim+=TTimeIntervalDays(1010);
	tim+=TTimeIntervalHours(-337);
	tim+=TTimeIntervalMinutes(-13);
	tim+=TTimeIntervalSeconds(52);
	tim+=TTimeIntervalMicroSeconds(9999999);
	test (base==tim);

	tim=TDateTime(-50,EMarch,6,14,45,3,100);
	dateTime=tim.DateTime();
	test(dateTime.Year()==-50);
	test(dateTime.Month()==EMarch);
	test(dateTime.Day()==6);
	test(dateTime.Hour()==14);
	test(dateTime.Minute()==45);
	test(dateTime.Second()==3);
	test(dateTime.MicroSecond()==100);

	tim=TDateTime(-241,EJanuary,0,0,0,0,0);
	tim-=TTimeIntervalMicroSeconds(1);
	dateTime=tim.DateTime();
	test(dateTime.Year()==-242);
	test(dateTime.Month()==EDecember);
	test(dateTime.Day()==30);
	test(dateTime.Hour()==23);
	test(dateTime.Minute()==59);
	test(dateTime.Second()==59);
	test(dateTime.MicroSecond()==999999);

	tim=Time::MaxTTime();
	dateTime=tim.DateTime();
	tim=dateTime;
	test(tim==Time::MaxTTime());

	tim=Time::MinTTime();
	dateTime=tim.DateTime();
	tim=dateTime;
	test(tim==Time::MinTTime());
	}

void TestTTime::Test8()
//
// Test TTime's date property functions
// this implicitly tests Time's date property functions.
//
	{
	test.Next(_L("Thorough Test with 4 day week rule"));

	TInt year=4;
	TMonth month=EJanuary;
	TInt day=30;
	TInt hour=0;
	TInt minute=0;
	TInt second=0;
	TInt microSecond=0;
	TDateTime dateTime(year,month,day,hour,minute,second,microSecond);
	TTime tim(dateTime);			

	test(tim.DayNoInWeek()==0);
	test(tim.DayNoInYear()==31);
	test(tim.WeekNoInYear()==5);

	dateTime.SetDay(29);
	tim=dateTime;
	test(tim.DayNoInWeek()==6);
	test(tim.DayNoInYear()==30);
	test(tim.WeekNoInYear()==4);

	dateTime.SetMonth(EJanuary);
	dateTime.SetDay(0);
    TInt y;
	for (y=1990;y<2020;y++)
		{
		dateTime.SetYear(y);
		tim=dateTime;
		test(tim.DayNoInYear()==1);
		TInt r=tim.WeekNoInYear();
		if (tim.DayNoInWeek()<=3)
			test(r==1);
		else
			test(r==52 || r==53);
		}

	dateTime.SetMonth(EDecember);
	dateTime.SetDay(30);
    TInt m(0);
    TInt d(0);
    TInt dn(0);
    TInt wkn(0);
    TInt wk(0);
    for (y=1900;y<1921;y++)          // MUST BEGIN 0N 1900 (been run to 2500)
		{
        dateTime.SetYear(y);
        for (m=0;m<12;m++)
            {
            dateTime.SetMonth(TMonth(m));
            for (d=0;d<Time::DaysInMonth(y,TMonth(m));d++)
                {
                dateTime.SetDay(d);
           		tim=dateTime;
                wk=tim.WeekNoInYear();
                dn++;
                if (dn>6)
                dn=0;
                if (dn==1)
                    {
                    wkn++;
                    if((m==11 && d>=28) | (m==0 && d<=3))
                    wkn=1;
                    }
              	test(wkn==wk);
                }
            dateTime.SetDay(0);
            }
        }

	test.Next(_L("Testing wk53 in a year with 4 days in last week"));
	dateTime.SetYear(2009);
	dateTime.SetMonth(EDecember);
	dateTime.SetDay(27); // 28th, day is 0-based
	dateTime.SetHour(8); // Ensure the remaining days are 3.somefraction to test rounding
	tim=dateTime;
	test(tim.DayNoInWeek()==0);
	test(tim.DayNoInMonth()==27);
	test(tim.DayNoInYear()==362);
	test(tim.WeekNoInYear()==53);
	dateTime.SetYear(2010);
	dateTime.SetMonth(EJanuary);
	dateTime.SetDay(3); // 4th, day is 0-based
	tim=dateTime;
	test(tim.DayNoInWeek()==0);
	test(tim.DayNoInMonth()==3);
	test(tim.DayNoInYear()==4);
	test(tim.WeekNoInYear()==1);
	dateTime.SetHour(0);

    test.Next(_L("Testing other week no. rules"));
	dateTime.SetYear(1995);
	dateTime.SetDay(14);
	dateTime.SetMonth(ENovember);
	tim=dateTime;
	test(tim.DayNoInWeek()==2);
	test(tim.DayNoInYear()==319);
	test(tim.WeekNoInYear()==46);

// Different First Week rules
	test.Next(_L("Test week no in year by different rules"));
	test(tim.WeekNoInYear(EFirstFullWeek)==46);
	test(tim.WeekNoInYear(EFirstWeek)==47);
	test(tim.WeekNoInYear(EFirstFourDayWeek)==46);

	dateTime.SetYear(1997);
	dateTime.SetMonth(EJanuary);
	dateTime.SetDay(6);
	tim=dateTime;
	test(tim.WeekNoInYear()==2);
	test(tim.WeekNoInYear(EFirstFullWeek)==1);
	test(tim.WeekNoInYear(EFirstWeek)==2);
	test(tim.WeekNoInYear(EFirstFourDayWeek)==2);


	dateTime.SetYear(1999);
	tim=dateTime;
	test(tim.WeekNoInYear()==1);
	test(tim.WeekNoInYear(EFirstFullWeek)==1);
	test(tim.WeekNoInYear(EFirstWeek)==2);
	test(tim.WeekNoInYear(EFirstFourDayWeek)==1);

// Year start dates different from jan 1st
	dateTime.SetYear(1995);
	dateTime.SetMonth(ENovember);
	dateTime.SetDay(14);
	TTime tim2(dateTime);  // cTime
	dateTime.SetMonth(EJune);
	tim=dateTime;	 //dTime

	test(tim2.DayNoInYear(tim)==154);
	test(tim2.WeekNoInYear(tim)==23);
	test(tim2.WeekNoInYear(tim,EFirstFullWeek)==22);
	test(tim.DayNoInYear(tim2)==213);
	test(tim.WeekNoInYear(tim2)==31);
	test(tim.WeekNoInYear(tim2,EFirstFullWeek)==30);

	dateTime.SetYear(1999);
	dateTime.SetMonth(EJanuary);
	dateTime.SetDay(6);
	tim2=dateTime;
	test(tim2.WeekNoInYear(tim,EFirstFullWeek)==30);
	test(tim2.WeekNoInYear(tim,EFirstWeek)==30);
	test(tim2.WeekNoInYear(tim,EFirstFourDayWeek)==30);

    dateTime.SetYear(1904);
	dateTime.SetMonth(EFebruary);
	dateTime.SetDay(28);
    tim=dateTime;
    dateTime.SetYear(1955);
    dateTime.SetMonth(EJanuary);
    tim2=dateTime;
    test(tim2.WeekNoInYear(tim,EFirstFullWeek)==48);
    test(tim2.WeekNoInYear(tim,EFirstWeek)==49);
    test(tim2.WeekNoInYear(tim,EFirstFourDayWeek)==48);
    dateTime.SetMonth(EMarch);
    tim2=dateTime;
    test(tim2.WeekNoInYear(tim,EFirstFourDayWeek)==5);
    test(tim2.WeekNoInYear(tim,EFirstFullWeek)==5);
    dateTime.SetYear(1994);
    dateTime.SetMonth(EMarch);
    dateTime.SetDay(0);
    dateTime.SetHour(12);
    tim2=dateTime;
    test(tim2.WeekNoInYear(tim,EFirstFullWeek)==1);
    test(tim2.WeekNoInYear(tim,EFirstWeek)==1);
    test(tim2.WeekNoInYear(tim,EFirstFourDayWeek)==1);
    dateTime.SetYear(1991);
    dateTime.SetMonth(EMarch);
    dateTime.SetDay(0);
    dateTime.SetHour(12);
    tim2=dateTime;
    test(tim2.WeekNoInYear(tim,EFirstFullWeek)==52);
    test(tim2.WeekNoInYear(tim,EFirstWeek)==1);
    test(tim2.WeekNoInYear(tim,EFirstFourDayWeek)==1);
    }


void TestTTime::Test9()
//
// string handling
//
	{
	TInt lyear =1993;
	TInt lmonth =6;
	TInt lday	=3;
	TInt lhour	=13;
	TInt lminute =53;
	TInt lsecond =20;
	TInt lmicroSecond =12345;
	TDateTime aDate(lyear,TMonth(lmonth),lday,lhour,lminute,lsecond,lmicroSecond);
	test.Next(_L("Different DateFormats"));
	TTime aTime(aDate);
	TBuf<0x80> testString;
	TDateTime aDateTime=aTime.DateTime();
    aDateTime.Month();
	aTime.FormatL(testString,(_L("%E")));
	if (testString.Compare(_L("Sunday")))
		test.Panic(_L("%%E"));
	aTime.FormatL(testString,(_L("%*E")));
	if (testString.Compare(_L("Sun")))
		test.Panic(_L("%%*E"));
	TLocale local;
	local.SetDateFormat(EDateEuropean);
	local.Set();
	aTime.FormatL(testString,(_L("%D%M%Y%/0%1%/1%2%/2%3%/3"))); 
	if (testString.Compare(_L("04/07/1993")))
		test.Panic(_L("%%D%%M%%Y"));
	local.SetDateFormat(EDateAmerican);
	local.Set();
	aTime.FormatL(testString,(_L("%*D%X%N%Y%1 %2 '%*3")));
	if (testString.Compare(_L("July 4th '93")))
		test.Panic(_L("%%*D%%X%%N'%%*Y, American"));
	local.SetDateFormat(EDateJapanese);
	local.Set();
	aTime.FormatL(testString,(_L("%*D%*N%4 %5")));
	if (testString.Compare(_L("Jul 4")))
		test.Panic(_L("%%*D%%*N, Japanese"));
	aTime.FormatL(testString,(_L("%F%Y %D%X %N"))); 
	if (testString.Compare(_L("1993 04th July")))
		test.Panic(_L("%%F%%Y %%D%%X %%N"));
	test.Next(_L("Times"));
	aTime.FormatL(testString,(_L("%*I%:1%T%A")));
	if (testString.Compare(_L("1:53 pm")))
		test.Panic(_L("%%*I%%:1%%T%%A"));
	local.SetAmPmSymbolPosition(ELocaleBefore);
	local.Set();
	aTime.FormatL(testString,(_L("%*I%:1%T%A")));
	if (testString.Compare(_L("1:53pm ")))
		test.Panic(_L("%%*I%%:1%%T%%A Bef"));
	local.SetAmPmSpaceBetween(EFalse);
	local.Set();
	aTime.FormatL(testString,(_L("%*I%:1%T%A")));
	if (testString.Compare(_L("1:53pm")))
		test.Panic(_L("%%*I%%:1%%T%%A Bef NoSp"));
	local.SetAmPmSymbolPosition(ELocaleAfter);
	local.Set();
	aTime.FormatL(testString,(_L("%*I%:1%T%A")));
	if (testString.Compare(_L("1:53pm")))
		test.Panic(_L("%%*I%%:1%%T%%A NoSp"));

	aTime.FormatL(testString,(_L("%-A%*I%:1%T%+A")));
	if (testString.Compare(_L("1:53pm")))
		test.Panic(_L("%%-A%%*I%%:1%%T%%+A NoSp"));
	local.SetAmPmSymbolPosition(ELocaleBefore);
	local.Set();
	aTime.FormatL(testString,(_L("%-A%*I%:1%T%+A")));
	if (testString.Compare(_L("pm1:53")))
		test.Panic(_L("%%-A%%*I%%:1%%T%%+A Bef NoSp"));
	local.SetAmPmSpaceBetween(ETrue);
	local.Set();
	aTime.FormatL(testString,(_L("%-A%*I%:1%T%+A")));
	if (testString.Compare(_L("pm 1:53")))
		test.Panic(_L("%%-A%%*I%%:1%%T%%+A Bef"));
	local.SetAmPmSymbolPosition(ELocaleAfter);
	local.Set();
	aTime.FormatL(testString,(_L("%-A%*I%:1%T%+A")));
	if (testString.Compare(_L("1:53 pm")))
		test.Panic(_L("%%-A%%*I%%:1%%T%%+A"));

	aTime.FormatL(testString,(_L("%:0%H%:1%T%:2%S%.%C%:3"))); 
	if (testString.Compare(_L("13:53:20.012345")))
		test.Panic(_L("%%:0%%H%%:1%%T%%:2%%S%%.%%C%%:3 1"));
	local.SetDecimalSeparator(',');
	local.Set();
	aTime.FormatL(testString,(_L("%:0%H%:1%T%:2%S%.%C%:3"))); 
	if (testString.Compare(_L("13:53:20,012345")))
		test.Panic(_L("%%:0%%H%%:1%%T%%:2%%S%%.%%C%%:3 2"));
	local.SetDecimalSeparator('.');
	local.Set();

	aTime.FormatL(testString,(_L("%T%:2%S%.%*C0"))); 
	if (testString.Compare(_L("53:20.")))
		test.Panic(_L("%%T%%:2%%S.%%*C0"));
	aTime.FormatL(testString,(_L("%S%.%*C1"))); 
	if (testString.Compare(_L("20.0")))
		test.Panic(_L("%%S.%%*C1"));
	aTime.FormatL(testString,(_L(".%*C3"))); 
	if (testString.Compare(_L(".012")))
		test.Panic(_L(".%%*C3"));
	aTime.FormatL(testString,(_L("%*C6"))); 
	if (testString.Compare(_L("012345")))
		test.Panic(_L("%%*C6"));
	aTime.FormatL(testString,(_L(".%*CZTest"))); 
	if (testString.Compare(_L(".012345Test")))
		test.Panic(_L("%%*C6"));
	aTime.FormatL(testString,(_L("%J%:1%T%B")));
	if (testString.Compare(_L("1:53 pm")))
		test.Panic(_L("%%J%%:1%%T%%B"));
	aTime.FormatL(testString,(_L("%J%:1%T%*B")));
	if (testString.Compare(_L("1:53pm")))
		test.Panic(_L("%%J%%:1%%T%%*B"));
	local.SetTimeFormat(ETime24);
	local.Set();
	aTime.FormatL(testString,(_L("%J%:1%T%B")));
	if (testString.Compare(_L("13:53")))
		test.Panic(_L("%%J%%:1%%T%%B, ETime24"));
	aTime.FormatL(testString,(_L("%J%:1%T%*B")));
	if (testString.Compare(_L("13:53")))
		test.Panic(_L("%%J%%:1%%T%%*B, ETime24"));
	test.Next(_L("Miscellaneous"));
	aTime.FormatL(testString,(_L("%W")));
	if (testString.Compare(_L("26")))
		test.Panic(_L("%%W"));
	aTime.FormatL(testString,(_L("%*Z")));
	if (testString.Compare(_L("185")))
		test.Panic(_L("%%*Z"));
	test.Next(_L("Junk strings"));
	aTime.FormatL(testString,(_L("%F %M%O%N%D%A%Y")));
	if (testString.Compare(_L(" 07OJuly04 pm1993")))
		test.Panic(_L(" MONDAY"));
	aTime.FormatL(testString,(_L("%*D%X %N '%*Y")));
	if (testString.Compare(_L("  '")))
		test.Panic(_L("  '"));
	aTime.FormatL(testString,(_L("%G%K%L%O%P%Q%R%U%V%%")));
	if (testString.Compare(_L("GKLOPQRUV%")))
		test.Panic(_L("GKLOPQRUV%%"));
	aDate.Set(1993,TMonth(6),3,0,0,0,0);
	aTime=aDate;
	aTime.FormatL(testString,(_L("%*I%:1%T%A")));
	if (testString.Compare(_L("12:00 am")))
		test.Panic(_L("testDate->time"));
	aTime.FormatL(testString,(_L("%*I%:1%T%*A")));
	if (testString.Compare(_L("12:00am")))
		test.Panic(_L("testDate->time 2"));
	aTime.FormatL(testString,(_L("unformatted string")));  // test added 25/08/95
	if (testString.Compare(_L("unformatted string")))		
		test.Panic(_L("unformatted string"));
	TBuf<8> buf;
	TRAPD(r,aTime.FormatL(buf,_L("%F %M%O%N%D%A%Y")));
	test(r==KErrOverflow);
	TRAP(r,aTime.FormatL(buf,_L("qwertyuiop")));
	test(r==KErrOverflow);
	TRAP(r,aTime.FormatL(testString,_L("%:4")));
	test(r==KErrGeneral);
 	TRAP(r,aTime.FormatL(testString,_L("%/4")));
	test(r==KErrGeneral);
 	TRAP(r,aTime.FormatL(testString,_L("%:/")));
	test(r==KErrGeneral);
 	TRAP(r,aTime.FormatL(testString,_L("%//")));
	test(r==KErrGeneral);
 	TRAP(r,aTime.FormatL(testString,_L("%:z")));
	test(r==KErrGeneral);
 	TRAP(r,aTime.FormatL(testString,_L("%/z")));
	test(r==KErrGeneral);
 	TRAP(r,aTime.FormatL(testString,_L("%: ")));
	test(r==KErrGeneral);
 	TRAP(r,aTime.FormatL(testString,_L("%/ ")));
	test(r==KErrGeneral);
 	TRAP(r,aTime.FormatL(testString,_L("%- ")));
	test(r==KErrGeneral);
 	TRAP(r,aTime.FormatL(testString,_L("%+ ")));
	test(r==KErrGeneral);

	// HA - 258
	aTime.Set(_L("19991231:000000.0000"));
	local.SetTimeFormat(ETime24);
	local.Set();
	aTime.FormatL(testString, _L("%*J%BX"));
	test(testString==_L("0X"));
	local.SetTimeFormat(ETime12);
	local.Set();
	aTime.FormatL(testString, _L("%*J%BX"));
	test(testString==_L("12 amX"));
	aTime.FormatL(testString, _L("%IX"));
	test(testString==_L("12X"));
	aTime.FormatL(testString, _L("%HX"));
	test(testString==_L("00X"));

	//Reset so it can be run twice
	local.SetDateFormat(EDateEuropean);
	local.SetTimeFormat(ETime12);
	local.Set();

	// Test for overload of TTime::FormatL(TDes& aDes,const TDesC& aFormat,const TLocale& aLocale);
	// Reset Time and dates
	aDate.Set(lyear,TMonth(lmonth),lday,lhour,lminute,lsecond,lmicroSecond);
	test.Next(_L("Different DateFormats with specified locale"));
	TTime aTimeLocale(aDate);

	local.SetDateFormat(EDateAmerican);
	aTimeLocale.FormatL(testString,(_L("%*D%X%N%Y%1 %2 '%*3")),local);
	if (testString.Compare(_L("July 4th '93")))
		test.Panic(_L("%%*D%%X%%N'%%*Y, American"));
	local.SetDateFormat(EDateJapanese);
	aTimeLocale.FormatL(testString,(_L("%*D%*N%4 %5")),local);
	if (testString.Compare(_L("Jul 4")))
		test.Panic(_L("%%*D%%*N, Japanese"));
	aTimeLocale.FormatL(testString,(_L("%F%Y %D%X %N")),local); 
	if (testString.Compare(_L("1993 04th July")))
		test.Panic(_L("%%F%%Y %%D%%X %%N"));

	test.Next(_L("Times with specified locale"));
	aTimeLocale.FormatL(testString,(_L("%*I%:1%T%A")),local);
	if (testString.Compare(_L("1:53 pm")))
		test.Panic(_L("%%*I%%:1%%T%%A"));
	local.SetAmPmSymbolPosition(ELocaleBefore);
	aTimeLocale.FormatL(testString,(_L("%*I%:1%T%A")),local);
	if (testString.Compare(_L("1:53pm ")))
		test.Panic(_L("%%*I%%:1%%T%%A Bef"));
	local.SetAmPmSpaceBetween(EFalse);
	aTimeLocale.FormatL(testString,(_L("%*I%:1%T%A")),local);
	if (testString.Compare(_L("1:53pm")))
		test.Panic(_L("%%*I%%:1%%T%%A Bef NoSp"));
	local.SetAmPmSymbolPosition(ELocaleAfter);
	aTimeLocale.FormatL(testString,(_L("%*I%:1%T%A")),local);
	if (testString.Compare(_L("1:53pm")))
		test.Panic(_L("%%*I%%:1%%T%%A NoSp"));
	aTimeLocale.FormatL(testString,(_L("%-A%*I%:1%T%+A")),local);
	if (testString.Compare(_L("1:53pm")))
		test.Panic(_L("%%-A%%*I%%:1%%T%%+A NoSp"));
	local.SetAmPmSymbolPosition(ELocaleBefore);
	aTimeLocale.FormatL(testString,(_L("%-A%*I%:1%T%+A")),local);
	if (testString.Compare(_L("pm1:53")))
		test.Panic(_L("%%-A%%*I%%:1%%T%%+A Bef NoSp"));
	local.SetAmPmSpaceBetween(ETrue);
	aTimeLocale.FormatL(testString,(_L("%-A%*I%:1%T%+A")),local);
	if (testString.Compare(_L("pm 1:53")))
		test.Panic(_L("%%-A%%*I%%:1%%T%%+A Bef"));
	local.SetAmPmSymbolPosition(ELocaleAfter);
	aTimeLocale.FormatL(testString,(_L("%-A%*I%:1%T%+A")),local);
	if (testString.Compare(_L("1:53 pm")))
		test.Panic(_L("%%-A%%*I%%:1%%T%%+A"));
	aTimeLocale.FormatL(testString,(_L("%:0%H%:1%T%:2%S%.%C%:3")),local); 
	if (testString.Compare(_L("13:53:20.012345")))
		test.Panic(_L("%%:0%%H%%:1%%T%%:2%%S%%.%%C%%:3 1"));
	local.SetDecimalSeparator(',');
	aTimeLocale.FormatL(testString,(_L("%:0%H%:1%T%:2%S%.%C%:3")),local); 
	if (testString.Compare(_L("13:53:20,012345")))
		test.Panic(_L("%%:0%%H%%:1%%T%%:2%%S%%.%%C%%:3 2"));
	local.SetDecimalSeparator('.');
	aTimeLocale.FormatL(testString,(_L("%T%:2%S%.%*C0")),local); 
	if (testString.Compare(_L("53:20.")))
		test.Panic(_L("%%T%%:2%%S.%%*C0"));
	aTimeLocale.FormatL(testString,(_L("%S%.%*C1")),local); 
	if (testString.Compare(_L("20.0")))
		test.Panic(_L("%%S.%%*C1"));
	aTimeLocale.FormatL(testString,(_L(".%*C3")),local); 
	if (testString.Compare(_L(".012")))
		test.Panic(_L(".%%*C3"));
	aTimeLocale.FormatL(testString,(_L("%*C6")),local); 
	if (testString.Compare(_L("012345")))
		test.Panic(_L("%%*C6"));
	aTimeLocale.FormatL(testString,(_L(".%*CZTest")),local); 
	if (testString.Compare(_L(".012345Test")))
		test.Panic(_L("%%*C6"));
	aTimeLocale.FormatL(testString,(_L("%J%:1%T%B")),local);
	if (testString.Compare(_L("1:53 pm")))
		test.Panic(_L("%%J%%:1%%T%%B"));
	aTimeLocale.FormatL(testString,(_L("%J%:1%T%*B")),local);
	if (testString.Compare(_L("1:53pm")))
		test.Panic(_L("%%J%%:1%%T%%*B"));
	local.SetTimeFormat(ETime24);
	aTimeLocale.FormatL(testString,(_L("%J%:1%T%B")),local);
	if (testString.Compare(_L("13:53")))
		test.Panic(_L("%%J%%:1%%T%%B, ETime24"));
	aTimeLocale.FormatL(testString,(_L("%J%:1%T%*B")),local);
	if (testString.Compare(_L("13:53")))
		test.Panic(_L("%%J%%:1%%T%%*B, ETime24"));

	test.Next(_L("Miscellaneous with specified locale"));
	aTimeLocale.FormatL(testString,(_L("%W")),local);
	if (testString.Compare(_L("26")))
		test.Panic(_L("%%W"));
	aTimeLocale.FormatL(testString,(_L("%*Z")),local);
	if (testString.Compare(_L("185")))
		test.Panic(_L("%%*Z"));

	test.Next(_L("Junk strings with specified locale"));
	aTimeLocale.FormatL(testString,(_L("%F %M%O%N%D%A%Y")),local);
	if (testString.Compare(_L(" 07OJuly04 pm1993")))
		test.Panic(_L(" MONDAY"));
	aTimeLocale.FormatL(testString,(_L("%*D%X %N '%*Y")),local);
	if (testString.Compare(_L("  '")))
		test.Panic(_L("  '"));
	aTimeLocale.FormatL(testString,(_L("%G%K%L%O%P%Q%R%U%V%%")),local);
	if (testString.Compare(_L("GKLOPQRUV%")))
		test.Panic(_L("GKLOPQRUV%%"));
	aDate.Set(1993,TMonth(6),3,0,0,0,0);
	aTimeLocale=aDate;
	aTimeLocale.FormatL(testString,(_L("%*I%:1%T%A")),local);
	if (testString.Compare(_L("12:00 am")))
		test.Panic(_L("testDate->time"));
	aTimeLocale.FormatL(testString,(_L("%*I%:1%T%*A")),local);
	if (testString.Compare(_L("12:00am")))
		test.Panic(_L("testDate->time 2"));
	aTimeLocale.FormatL(testString,(_L("unformatted string")),local);  // test added 25/08/95
	if (testString.Compare(_L("unformatted string")))		
		test.Panic(_L("unformatted string"));
	TRAP(r,aTimeLocale.FormatL(buf,_L("%F %M%O%N%D%A%Y"),local));
	test(r==KErrOverflow);
	TRAP(r,aTimeLocale.FormatL(buf,_L("qwertyuiop"),local));
	test(r==KErrOverflow);
	TRAP(r,aTimeLocale.FormatL(testString,_L("%:4"),local));
	test(r==KErrGeneral);
 	TRAP(r,aTimeLocale.FormatL(testString,_L("%/4"),local));
	test(r==KErrGeneral);
 	TRAP(r,aTimeLocale.FormatL(testString,_L("%:/"),local));
	test(r==KErrGeneral);
 	TRAP(r,aTimeLocale.FormatL(testString,_L("%//"),local));
	test(r==KErrGeneral);
 	TRAP(r,aTimeLocale.FormatL(testString,_L("%:z"),local));
	test(r==KErrGeneral);
 	TRAP(r,aTimeLocale.FormatL(testString,_L("%/z"),local));
	test(r==KErrGeneral);
 	TRAP(r,aTimeLocale.FormatL(testString,_L("%: "),local));
	test(r==KErrGeneral);
 	TRAP(r,aTimeLocale.FormatL(testString,_L("%/ "),local));
	test(r==KErrGeneral);
 	TRAP(r,aTimeLocale.FormatL(testString,_L("%- "),local));
	test(r==KErrGeneral);
 	TRAP(r,aTimeLocale.FormatL(testString,_L("%+ "),local));
	test(r==KErrGeneral);
	aTimeLocale.Set(_L("19991231:000000.0000"));
	local.SetTimeFormat(ETime24);
	aTimeLocale.FormatL(testString, _L("%*J%BX"),local);
	test(testString==_L("0X"));
	local.SetTimeFormat(ETime12);
	aTimeLocale.FormatL(testString, _L("%*J%BX"),local);
	test(testString==_L("12 amX"));
	aTimeLocale.FormatL(testString, _L("%IX"),local);
	test(testString==_L("12X"));
	aTimeLocale.FormatL(testString, _L("%HX"),local);
	test(testString==_L("00X"));	
	}


void TestTTime::Test10()
//
// The Time functions that aren't called/tested above.
//
	{
	test.Next(_L("IsLeapYear()"));
	TDateTime date;
	date.Set(1200,EFebruary,28,13,58,59,245322);
	test(date.SetYear(1980)==KErrNone);
	TBool isLeap = Time::IsLeapYear(1580);
	TInt noUpTo = Time::LeapYearsUpTo(1580);
	test(isLeap==1);
	test(noUpTo==395); 
	isLeap = Time::IsLeapYear(1750);
	noUpTo = Time::LeapYearsUpTo(1750);
	test(isLeap==0);
	test(noUpTo==437); 
	isLeap = Time::IsLeapYear(1980);
	noUpTo = Time::LeapYearsUpTo(1980);  
	test(isLeap==1);
	test(noUpTo==492); 
	isLeap = Time::IsLeapYear(2000);
	noUpTo = Time::LeapYearsUpTo(2000);  
	test(isLeap==1);
	test(noUpTo==497); 
	isLeap = Time::IsLeapYear(25000);
	noUpTo = Time::LeapYearsUpTo(25000);  
	test(isLeap==0);
	test(noUpTo==6075);

	test.Next(_L("TTime::RoundUpToNextMinute()"));
	TTime time;
	time.RoundUpToNextMinute();
	TDateTime dateTime=time.DateTime();
	test(dateTime.MicroSecond()==0);
	test(dateTime.Second()==0);

	TDateTime dateTime2(2004,EFebruary,28,12,48,59,999999);
	time=dateTime2;
	time.RoundUpToNextMinute();
	dateTime=time.DateTime();
	test(dateTime.MicroSecond()==0);
	test(dateTime.Second()==0);
	test(dateTime.Minute()==dateTime2.Minute()+1);
	test(dateTime.Hour()==dateTime2.Hour());
	test(dateTime.Day()==dateTime2.Day());
	test(dateTime.Month()==dateTime2.Month());
	test(dateTime.Year()==dateTime2.Year());

	dateTime2.Set(2004,EFebruary,28,12,48,0,0);
	time=dateTime2;
	time.RoundUpToNextMinute();
	dateTime=time.DateTime();
	test(dateTime.MicroSecond()==0);
	test(dateTime.Second()==0);
	test(dateTime.MicroSecond()==dateTime2.MicroSecond());
	test(dateTime.Second()==dateTime2.Second());
	test(dateTime.Minute()==dateTime2.Minute());
	test(dateTime.Hour()==dateTime2.Hour());
	test(dateTime.Day()==dateTime2.Day());
	test(dateTime.Month()==dateTime2.Month());
	test(dateTime.Year()==dateTime2.Year());

	dateTime2.Set(2004,EFebruary,28,12,48,0,1);
	time=dateTime2;
	time.RoundUpToNextMinute();
	dateTime=time.DateTime();
	test(dateTime.MicroSecond()==0);
	test(dateTime.Second()==0);
	test(dateTime.Minute()==dateTime2.Minute()+1);
	test(dateTime.Hour()==dateTime2.Hour());
	test(dateTime.Day()==dateTime2.Day());
	test(dateTime.Month()==dateTime2.Month());
	test(dateTime.Year()==dateTime2.Year());

	dateTime2.Set(2037,EDecember,30,23,59,1,45341);
	time=dateTime2;
	time.RoundUpToNextMinute();
	dateTime=time.DateTime();
	test(dateTime.MicroSecond()==0);
	test(dateTime.Second()==0);
	test(dateTime.Minute()==0);
	test(dateTime.Hour()==0);
	test(dateTime.Day()==0);
	test(dateTime.Month()==EJanuary);
	test(dateTime.Year()==dateTime2.Year()+1);

	test.Next(_L("HomeTime and UniversalTime"));
	time.HomeTime();
	dateTime=time.DateTime();
	test.Printf(_L("     Local Time is - %+02d/%+02d/%+04d %+02d:%+02d:%+02d.%+06d\n"),dateTime.Day()+1,dateTime.Month()+1,dateTime.Year(),dateTime.Hour(),dateTime.Minute(),dateTime.Second(),dateTime.MicroSecond());
	time.UniversalTime();
	dateTime=time.DateTime();
	test.Printf(_L(" Universal Time is - %+02d/%+02d/%+04d %+02d:%+02d:%+02d.%+06d\n"),dateTime.Day()+1,dateTime.Month()+1,dateTime.Year(),dateTime.Hour(),dateTime.Minute(),dateTime.Second(),dateTime.MicroSecond());

	test.Next(_L("SetUTCTime"));
	time.UniversalTime();
	time+=TTimeIntervalMinutes(30);
	TInt r=User::SetUTCTime(time);
	test(r==KErrNone);
	time.HomeTime();
	dateTime=time.DateTime();
	test.Printf(_L("     Local Time is - %+02d/%+02d/%+04d %+02d:%+02d:%+02d.%+06d\n"),dateTime.Day()+1,dateTime.Month()+1,dateTime.Year(),dateTime.Hour(),dateTime.Minute(),dateTime.Second(),dateTime.MicroSecond());
	time.UniversalTime();
	dateTime=time.DateTime();
	test.Printf(_L(" Universal Time is - %+02d/%+02d/%+04d %+02d:%+02d:%+02d.%+06d\n"),dateTime.Day()+1,dateTime.Month()+1,dateTime.Year(),dateTime.Hour(),dateTime.Minute(),dateTime.Second(),dateTime.MicroSecond());

	r=User::SetUTCTime(TTime(TDateTime(2090,EJanuary,0,0,0,0,0)));
//#if defined (__MARM__)
	test(r==KErrOverflow);
//#else
//	test(r==KErrNone);
//	time.HomeTime();
//	test(time.DateTime().Second()==0);
//	test(time.DateTime().Minute()==0);
//	test(time.DateTime().Day()==0);
//	test(time.DateTime().Month()==EJanuary);
//	test(time.DateTime().Year()==2090);
//	test(time.DateTime().Hour()==0);
//#endif

	time.UniversalTime();
	time-=TTimeIntervalMinutes(30);
	r=User::SetUTCTime(time);
	test(r==KErrNone);
	time.HomeTime();
	dateTime=time.DateTime();
	test.Printf(_L("     Local Time is - %+02d/%+02d/%+04d %+02d:%+02d:%+02d.%+06d\n"),dateTime.Day()+1,dateTime.Month()+1,dateTime.Year(),dateTime.Hour(),dateTime.Minute(),dateTime.Second(),dateTime.MicroSecond());
	time.UniversalTime();
	dateTime=time.DateTime();
	test.Printf(_L(" Universal Time is - %+02d/%+02d/%+04d %+02d:%+02d:%+02d.%+06d\n"),dateTime.Day()+1,dateTime.Month()+1,dateTime.Year(),dateTime.Hour(),dateTime.Minute(),dateTime.Second(),dateTime.MicroSecond());
	test.Printf(_L("does this come out"));
	}

void TestTTime::Test11()
//
//
//
	{
	TTime now;
	now.UniversalTime();
	TTimeIntervalSeconds offset;
	offset = User::UTCOffset();
	RTimer timer[5];
	TRequestStatus stat[5];

	test.Start(_L("Create timers"));
	TInt i;
	for (i=0; i<5; i++)
		test(timer[i].CreateLocal()==KErrNone);

	test.Next(_L("Change the time"));
	TInt r=User::SetUTCTime(now-TTimeIntervalMinutes(120));
	test_Equal(r, KErrNone);

	test.Next(_L("Start an absolute timer"));
	timer[0].AtUTC(stat[0], now+TTimeIntervalMinutes(60));
	test.Next(_L("Change the system time"));
	r=User::SetUTCTime(now-TTimeIntervalMinutes(120));
 	test(r==KErrNone);
	User::WaitForRequest(stat[0]);
	test.Next(_L("Test timer aborted"));
	test(stat[0]==KErrAbort);

	test.Next(_L("Set UTC offset to zero"));
	User::SetUTCOffset(0);
	test.Next(_L("Start an absolute timer"));
	timer[0].AtUTC(stat[0], now+TTimeIntervalMinutes(120));
	test.Next(_L("Change the UTC offset to +1 hour"));
	User::SetUTCOffset(3600);
	User::WaitForRequest(stat[0]);
	test.Next(_L("Test timer aborted"));
	test(stat[0]==KErrAbort);
	test.Next(_L("Start another absolute timer"));
	timer[0].AtUTC(stat[0], now+TTimeIntervalMinutes(120));
	test.Next(_L("Re-set the UTC offset to +1 hour"));
	User::SetUTCOffset(3600);
	test.Next(_L("Test timer NOT aborted (no actual time change)"));
	test(stat[0]==KRequestPending);
	test.Next(_L("Cancel timer"));
	timer[0].Cancel();
	User::WaitForRequest(stat[0]);
	test(stat[0]==KErrCancel);

/*
//	This code fails intermitantly
	FOREVER
		{
		timer[0].AtUTC(stat[0], now+TTimeIntervalMinutes(60));
		test(stat[0]==KRequestPending);
		timer[1].AtUTC(stat[1], now+TTimeIntervalMinutes(30));
		test(stat[1]==KRequestPending);
		test.Next(_L("ABCDEFGHIJKLMNOPQRS D FD FDDFGDF ABCDEFGHIJ ABCDEFGHIJKGL"));
		timer[1].Cancel();
		timer[0].Cancel();
		User::WaitForRequest(stat[0]);
		User::WaitForRequest(stat[1]);
		test.Next(_L("ABCDEFGH"));
		test(stat[0]==KErrCancel);
		test(stat[1]==KErrCancel);
		}
*/

	test.Next(_L("Start 3 absolute timers and a relative timer"));
	timer[0].AtUTC(stat[0], now+TTimeIntervalMinutes(60));
	test_Equal(KRequestPending, stat[0].Int());
	timer[1].AtUTC(stat[1], now+TTimeIntervalMinutes(30));
	test_Equal(KRequestPending, stat[1].Int());
	timer[2].After(stat[2], 9000000);
	test_Equal(KRequestPending, stat[2].Int());
	timer[3].AtUTC(stat[3], now+TTimeIntervalMinutes(10));
	test_Equal(KRequestPending, stat[3].Int());
	TInt s=stat[2].Int();
	test.Next(_L("Change system time"));
	r=User::SetUTCTime(now-TTimeIntervalMinutes(100));
	test(r==KErrNone);
	User::WaitForRequest(stat[0]);
	User::WaitForRequest(stat[1]);
	User::WaitForRequest(stat[3]);
	test.Next(_L("Test absolute timers aborted"));
	test(stat[0]==KErrAbort);
	test(stat[1]==KErrAbort);
	test(stat[3]==KErrAbort);
	test(stat[2]==s);
	test.Next(_L("Cancel relative timer"));
	timer[2].Cancel();
	User::WaitForRequest(stat[2]);
	test(stat[2]==KErrCancel);

	test.Next(_L("Start 3 relative timers and 1 absolute timer"));
	timer[0].After(stat[0], 10000);
	timer[1].After(stat[1], 20000);
	timer[2].After(stat[2], 20100);
	timer[3].AtUTC(stat[3], now+TTimeIntervalMinutes(10));
	test.Next(_L("Wait for 1 relative timer to complete"));
	User::WaitForRequest(stat[0]);
	test(stat[0]==KErrNone);
	test.Next(_L("Change the time"));
	r=User::SetUTCTime(now-TTimeIntervalMinutes(100));
	test(r==KErrNone);
	User::WaitForRequest(stat[3]);
	test(stat[3]==KErrAbort);
	stat[3]=-999;
	test.Next(_L("Change the time again"));
	r=User::SetUTCTime(now-TTimeIntervalMinutes(110));
	test(r==KErrNone);
	test.Next(_L("Wait for other relative timers to complete"));
	User::WaitForRequest(stat[1]);
	User::WaitForRequest(stat[2]);
	test(stat[1]==KErrNone);
	test(stat[2]==KErrNone);
	test(stat[3]==-999);

	test.Next(_L("Start 2 absolute timers"));
	timer[0].AtUTC(stat[0], now+TTimeIntervalMinutes(60));
	timer[1].AtUTC(stat[1], now+TTimeIntervalMinutes(30));
	test.Next(_L("Cancel one"));
	timer[0].Cancel();
	User::WaitForRequest(stat[0]);
	test(stat[0]==KErrCancel);
	test.Next(_L("Change the time"));
	r=User::SetUTCTime(now-TTimeIntervalMinutes(110));
	test(r==KErrNone);
	User::WaitForRequest(stat[1]);
	test(stat[1]==KErrAbort);
	test(stat[0]==KErrCancel);

	// The platform may or may not support SecureTime, and even if it does,
	// it may not have a secure time set. So we start this test by making
	// sure that the NonSecureOffset is set (which may fail, if it's not
	// supported OR if it's already set); then read and write and reread
	// secure time, to make sure that it's supported and we have permission.
	//
	test.Next(_L("Test absolute timers with secure time change"));
	User::SetUTCTime(now);
	HAL::Set(HAL::ETimeNonSecureOffset, 0);
	TTime securetime;
	if ((r = securetime.UniversalTimeSecure()) != KErrNone)
		securetime = now;
	if ((r = User::SetUTCTimeSecure(securetime)) == KErrNone)
		r = securetime.UniversalTimeSecure();
	if (r != KErrNone)
		{
		RDebug::Printf("WARNING: Secure clock change test skipped because secure time could not be changed!");
		}
	else
		{
		timer[0].AtUTC(stat[0], now+TTimeIntervalSeconds(5));
		r = User::SetUTCTimeSecure(securetime+TTimeIntervalSeconds(30));
		test_Equal(KErrNone, r);
		r = User::SetUTCTimeSecure(securetime-TTimeIntervalSeconds(30));
		test_Equal(KErrNone, r);
		// The absolute timer should not have been aborted by the secure time change,
		test_Equal(KRequestPending, stat[0].Int());

		// The outstanding absolute timer should complete before this new relative timer
		timer[1].After(stat[1], 20000000);
		User::WaitForRequest(stat[0], stat[1]);
		timer[1].Cancel();
		test_Equal(KErrNone, stat[0].Int());
		test_Equal(KErrCancel, stat[1].Int());
		User::SetUTCTimeSecure(securetime+TTimeIntervalSeconds(5));
		}

	test.Next(_L("Close the timers"));
	for (i=0; i<5; i++)
		timer[i].Close();

	r=User::SetUTCTimeAndOffset(now,offset);
	test(r==KErrNone);
	test.End();
	}

void TestTTime::Test12()
    {

    TInt err;
    TDateTime dateTime;
	test.Start(_L("Setting date using YYYYMMDD:HHMMSS.MMMMMM"));
    TTime now(_L("19960201:122341.1234"));
    dateTime=now.DateTime();
	test(dateTime.MicroSecond()==1234);
	test(dateTime.Second()==41);
	test(dateTime.Minute()==23);
	test(dateTime.Hour()==12);
	test(dateTime.Day()==1);
	test(dateTime.Month()==2);
	test(dateTime.Year()==1996);
	test.Next(_L("Setting date using YYYYMMDD:"));
    err=now.Set(_L("19901129:")); // Just set the date
    dateTime=now.DateTime();
    test(err==KErrNone);
	test(dateTime.MicroSecond()==0);
	test(dateTime.Second()==0);
	test(dateTime.Minute()==0);
	test(dateTime.Hour()==0);
	test(dateTime.Day()==29);
	test(dateTime.Month()==11);
	test(dateTime.Year()==1990);
	test.Next(_L("Setting date using :HHMMSS."));
    err=now.Set(_L(":105614.")); // Just the time
    dateTime=now.DateTime();
    test(err==KErrNone);
	test(dateTime.MicroSecond()==0);
	test(dateTime.Second()==14);
	test(dateTime.Minute()==56);
	test(dateTime.Hour()==10);
	test(dateTime.Day()==0);
	test(dateTime.Month()==0);
	test(dateTime.Year()==0);
	test.Next(_L("Setting date using .MMMMMM"));
    err=now.Set(_L(".999999")); // Just the microseconds
    dateTime=now.DateTime();
    test(err==KErrNone);
	test(dateTime.MicroSecond()==999999);
	test(dateTime.Second()==0);
	test(dateTime.Minute()==0);
	test(dateTime.Hour()==0);
	test(dateTime.Day()==0);
	test(dateTime.Month()==0);
	test(dateTime.Year()==0);
	test.Next(_L("Setting date using HHMMSS should fail"));
    err=now.Set(_L("104520")); // Invalid - no separator
    dateTime=now.DateTime();
    test(err==KErrGeneral);
	test(dateTime.MicroSecond()==999999);
	test(dateTime.Second()==0);
	test(dateTime.Minute()==0);
	test(dateTime.Hour()==0);
	test(dateTime.Day()==0);
	test(dateTime.Month()==0);
	test(dateTime.Year()==0);
	test.Next(_L("Setting date using :HHMMSS"));
    err=now.Set(_L(":054531")); // Set time with no dot
    dateTime=now.DateTime();
    test(err==KErrNone);
	test(dateTime.MicroSecond()==0);
	test(dateTime.Second()==31);
	test(dateTime.Minute()==45);
	test(dateTime.Hour()==5);
	test(dateTime.Day()==0);
	test(dateTime.Month()==0);
	test(dateTime.Year()==0);
	test.Next(_L("Setting invalid date using YYYYMMSS:HHMMSS.MMMM"));
    err=now.Set(_L("19910130:023210.1234")); // invalid date
    dateTime=now.DateTime();
    test(err==KErrGeneral);
	test(dateTime.MicroSecond()==0);
	test(dateTime.Second()==31);
	test(dateTime.Minute()==45);
	test(dateTime.Hour()==5);
	test(dateTime.Day()==0);
	test(dateTime.Month()==0);
	test(dateTime.Year()==0);
	test.Next(_L("Setting date using YYYYMMDD:.MMMM"));
    err=now.Set(_L("19960730:.123456")); // Set date and microseconds
    dateTime=now.DateTime();
    test(err==KErrNone);
	test(dateTime.MicroSecond()==123456);
	test(dateTime.Second()==0);
	test(dateTime.Minute()==0);
	test(dateTime.Hour()==0);
	test(dateTime.Day()==30);
	test(dateTime.Month()==7);
	test(dateTime.Year()==1996);
	test.Next(_L("Setting date using ."));
    err=now.Set(_L("."));
    dateTime=now.DateTime();
    test(err==KErrNone);
	test(dateTime.MicroSecond()==0);
	test(dateTime.Second()==0);
	test(dateTime.Minute()==0);
	test(dateTime.Hour()==0);
	test(dateTime.Day()==0);
	test(dateTime.Month()==0);
	test(dateTime.Year()==0);
	test.Next(_L("Setting date using :."));
    err=now.Set(_L(":."));
    dateTime=now.DateTime();
    test(err==KErrNone);
	test(dateTime.MicroSecond()==0);
	test(dateTime.Second()==0);
	test(dateTime.Minute()==0);
	test(dateTime.Hour()==0);
	test(dateTime.Day()==0);
	test(dateTime.Month()==0);
	test(dateTime.Year()==0);
	test.Next(_L("Setting date using :"));
    err=now.Set(_L(":"));
    dateTime=now.DateTime();
    test(err==KErrNone);
	test(dateTime.MicroSecond()==0);
	test(dateTime.Second()==0);
	test(dateTime.Minute()==0);
	test(dateTime.Hour()==0);
	test(dateTime.Day()==0);
	test(dateTime.Month()==0);
	test(dateTime.Year()==0);
	test.Next(_L("Setting date using YYYYMMDD.HHMMSS:MMMM should fail"));
    err=now.Set(_L("19900101.105630:1234")); // Wrong way round
    dateTime=now.DateTime();
    test(err==KErrGeneral);
	test(dateTime.MicroSecond()==0);
	test(dateTime.Second()==0);
	test(dateTime.Minute()==0);
	test(dateTime.Hour()==0);
	test(dateTime.Day()==0);
	test(dateTime.Month()==0);
	test(dateTime.Year()==0);
	test.Next(_L("Setting date using YYYYMMDD:HHMMSS.MMMMMMM should fail"));
    err=now.Set(_L("19900101:105630.1234567")); // Microseconds too long
    dateTime=now.DateTime();
    test(err==KErrGeneral);
	test(dateTime.MicroSecond()==0);
	test(dateTime.Second()==0);
	test(dateTime.Minute()==0);
	test(dateTime.Hour()==0);
	test(dateTime.Day()==0);
	test(dateTime.Month()==0);
	test(dateTime.Year()==0);
    test.End();
    }

struct TestInfo
	{
	TestInfo (TTime aTime,TInt aMicroSec,TInt aSec,TInt aMin,TInt aHour,TInt aDay,TInt aMonth,TInt aYear,TText* aDayString,TTime aNextMin)
		{
		iTime=aTime;
		iMicroSec=aMicroSec;
		iSec=aSec;
		iMin=aMin;
		iHour=aHour;
		iDay=aDay;
		iMonth=aMonth;
		iYear=aYear;
		iDayString=aDayString;
		iNextMin=aNextMin;
		}
	TTime iTime;
	TInt iMicroSec;
	TInt iSec;
	TInt iMin;
	TInt iHour;
	TInt iDay;
	TInt iMonth;
	TInt iYear;
	TText* iDayString;
	TTime iNextMin;
	};

const TestInfo KTestArray[]=
	{
	TestInfo(TTime(KDaysToMicroSeconds*31+1),1,0,0,0,0,EFebruary,0,(TText*)_S("!Thu!am!00!02!"),TTime(KDaysToMicroSeconds*31+60000000)),
	TestInfo(TTime(KDaysToMicroSeconds*31),0,0,0,0,0,EFebruary,0,(TText*)_S("!Thu!am!00!02!"),TTime(KDaysToMicroSeconds*31)),
	TestInfo(TTime(KDaysToMicroSeconds*31-1),999999,59,59,23,30,EJanuary,0,(TText*)_S("!Wed!pm!59!01!"),TTime(KDaysToMicroSeconds*31)),
	TestInfo(TTime(60000001),1,0,1,0,0,EJanuary,0,(TText*)_S("!Mon!am!01!01!"),TTime(120000000)),
	TestInfo(TTime(60000000),0,0,1,0,0,EJanuary,0,(TText*)_S("!Mon!am!01!01!"),TTime(60000000)),
	TestInfo(TTime(59999999),999999,59,0,0,0,EJanuary,0,(TText*)_S("!Mon!am!00!01!"),TTime(60000000)),
	TestInfo(TTime(1000001),1,1,0,0,0,EJanuary,0,(TText*)_S("!Mon!am!00!01!"),TTime(60000000)),
	TestInfo(TTime(1000000),0,1,0,0,0,EJanuary,0,(TText*)_S("!Mon!am!00!01!"),TTime(60000000)),
	TestInfo(TTime(999999),999999,0,0,0,0,EJanuary,0,(TText*)_S("!Mon!am!00!01!"),TTime(60000000)),
	TestInfo(TTime(1),1,0,0,0,0,EJanuary,0,(TText*)_S("!Mon!am!00!01!"),TTime(60000000)),
	TestInfo(TTime(0),0,0,0,0,0,EJanuary,0,(TText*)_S("!Mon!am!00!01!"),TTime(0)),
	TestInfo(TTime(-1),999999,59,59,23,30,EDecember,-1,(TText*)_S("!Sun!pm!59!12!"),TTime(0)),
	TestInfo(TTime(-1000000),0,59,59,23,30,EDecember,-1,(TText*)_S("!Sun!pm!59!12!"),TTime(0)),
	TestInfo(TTime(-999999),1,59,59,23,30,EDecember,-1,(TText*)_S("!Sun!pm!59!12!"),TTime(0)),
	TestInfo(TTime(-1000001),999999,58,59,23,30,EDecember,-1,(TText*)_S("!Sun!pm!59!12!"),TTime(0)),
	TestInfo(TTime(-60000000),0,0,59,23,30,EDecember,-1,(TText*)_S("!Sun!pm!59!12!"),TTime(-60000000)),
	TestInfo(TTime(-59999999),1,0,59,23,30,EDecember,-1,(TText*)_S("!Sun!pm!59!12!"),TTime(0)),
	TestInfo(TTime(-60000001),999999,59,58,23,30,EDecember,-1,(TText*)_S("!Sun!pm!58!12!"),TTime(-60000000)),
	TestInfo(TTime(-180000000),0,0,57,23,30,EDecember,-1,(TText*)_S("!Sun!pm!57!12!"),TTime(-180000000)),
	TestInfo(TTime(-179999999),1,0,57,23,30,EDecember,-1,(TText*)_S("!Sun!pm!57!12!"),TTime(-120000000)),
	TestInfo(TTime(-180000001),999999,59,56,23,30,EDecember,-1,(TText*)_S("!Sun!pm!56!12!"),TTime(-180000000)),
	TestInfo(TTime(-KDaysToMicroSeconds+1),1,0,0,0,30,EDecember,-1,(TText*)_S("!Sun!am!00!12!"),TTime(-KDaysToMicroSeconds+60000000)),
	TestInfo(TTime(-KDaysToMicroSeconds),0,0,0,0,30,EDecember,-1,(TText*)_S("!Sun!am!00!12!"),TTime(-KDaysToMicroSeconds)),
	TestInfo(TTime(-KDaysToMicroSeconds-1),999999,59,59,23,29,EDecember,-1,(TText*)_S("!Sat!pm!59!12!"),TTime(-KDaysToMicroSeconds)),
	TestInfo(TTime(-KDaysToMicroSeconds*7),0,0,0,0,24,EDecember,-1,(TText*)_S("!Mon!am!00!12!"),TTime(-KDaysToMicroSeconds*7)),
	TestInfo(TTime(-KDaysToMicroSeconds*14),0,0,0,0,17,EDecember,-1,(TText*)_S("!Mon!am!00!12!"),TTime(-KDaysToMicroSeconds*14)),
	TestInfo(TTime(-KDaysToMicroSeconds*14+1),1,0,0,0,17,EDecember,-1,(TText*)_S("!Mon!am!00!12!"),TTime(-KDaysToMicroSeconds*14+60000000)),
	TestInfo(TTime(-KDaysToMicroSeconds*14-1),999999,59,59,23,16,EDecember,-1,(TText*)_S("!Sun!pm!59!12!"),TTime(-KDaysToMicroSeconds*14)),
	TestInfo(TTime(-KDaysToMicroSeconds*92),0,0,0,0,0,EOctober,-1,(TText*)_S("!Sun!am!00!10!"),TTime(-KDaysToMicroSeconds*92)),
	TestInfo(TTime(-KDaysToMicroSeconds*92+1),1,0,0,0,0,EOctober,-1,(TText*)_S("!Sun!am!00!10!"),TTime(-KDaysToMicroSeconds*92+60000000)),
	TestInfo(TTime(-KDaysToMicroSeconds*92-1),999999,59,59,23,29,ESeptember,-1,(TText*)_S("!Sat!pm!59!09!"),TTime(-KDaysToMicroSeconds*92)),
	TestInfo(Time::NullTTime(),224192,5,59,19,21,EDecember,-292272,(TText*)_S("!Thu!pm!59!12!"),TTime(Time::NullTTime().Int64()-Time::NullTTime().Int64()%60000000))
	};

void TestTTime::Test13()
	{
	TBuf<0x80> testString;
	TInt i=0;
	for (;i<(TInt)(sizeof(KTestArray)/sizeof(TestInfo))-1;i++)
		{
		TTime time=KTestArray[i].iTime;
 		TInt r=time.DateTime().MicroSecond();
 		test(r==KTestArray[i].iMicroSec);
		r=time.DateTime().Second();
 		test(r==KTestArray[i].iSec);
		r=time.DateTime().Minute();
 		test(r==KTestArray[i].iMin);
		r=time.DateTime().Hour();
 		test(r==KTestArray[i].iHour);
		r=time.DateTime().Day();
 		test(r==KTestArray[i].iDay);
		r=time.DateTime().Month();
 		test(r==KTestArray[i].iMonth);
		r=time.DateTime().Year();
 		test(r==KTestArray[i].iYear);
		TRAP(r,time.FormatL(testString,_L("!%*E!%*A!%T!%F%M!")));
		test(r==KErrNone);
		test(testString==TPtrC(KTestArray[i].iDayString));
        TTimeIntervalMicroSeconds usFrom;
		usFrom=time.MicroSecondsFrom(TTime(0));
		test(usFrom==time.Int64());
		usFrom=TTime(0).MicroSecondsFrom(time);
		test(usFrom==-time.Int64());
		usFrom=time.MicroSecondsFrom(TTime(-1));
		test(usFrom==time.Int64()+1);
		usFrom=TTime(-1).MicroSecondsFrom(time);
		test(usFrom==-time.Int64()-1);
		usFrom=time.MicroSecondsFrom(TTime(1));
		test(usFrom==time.Int64()-1);
		usFrom=TTime(1).MicroSecondsFrom(time);
		test(usFrom==-time.Int64()+1);
		TTime time2=time+TTimeIntervalYears(0);
		test(time2==time);
		time2=time+TTimeIntervalYears(1);
		r=time2.DateTime().Year();
 		test(r==KTestArray[i].iYear+1);
		time2=time-TTimeIntervalYears(1);
		r=time2.DateTime().Year();
 		test(r==KTestArray[i].iYear-1);
		time2=time+TTimeIntervalMonths(0);
		test(time2==time);
		time2=time+TTimeIntervalMonths(1);
		r=time2.DateTime().Month();
 		test(r==(KTestArray[i].iMonth+1)%12);
		time2=time-TTimeIntervalMonths(1);
		r=time2.DateTime().Month();
 		test(r==(KTestArray[i].iMonth+11)%12);
		time2=time+TTimeIntervalDays(0);
		test(time2==time);
		time2=time+TTimeIntervalHours(0);
		test(time2==time);
		time2=time+TTimeIntervalMinutes(0);
		test(time2==time);
		time2=time+TTimeIntervalSeconds(0);
		test(time2==time);
		time2=time+TTimeIntervalMicroSeconds(0);
		test(time2==time);
		time.RoundUpToNextMinute();
		test(time==TTime(KTestArray[i].iNextMin));
		}

	TTime time=KTestArray[i].iTime;
	test(time==Time::NullTTime());
 	TInt r=time.DateTime().MicroSecond();
 	test(r==KTestArray[i].iMicroSec);
	r=time.DateTime().Second();
 	test(r==KTestArray[i].iSec);
	r=time.DateTime().Minute();
 	test(r==KTestArray[i].iMin);
	r=time.DateTime().Hour();
 	test(r==KTestArray[i].iHour);
	r=time.DateTime().Day();
 	test(r==KTestArray[i].iDay);
	r=time.DateTime().Month();
 	test(r==KTestArray[i].iMonth);
	r=time.DateTime().Year();
 	test(r==KTestArray[i].iYear);
	TRAP(r,time.FormatL(testString,_L("!%*E!%*A!%T!%F%M!")));
	test(r==KErrNone);
	test(testString==TPtrC(KTestArray[i].iDayString));
	TTimeIntervalMicroSeconds usFrom;
	usFrom=time.MicroSecondsFrom(TTime(0));
	test(usFrom==time.Int64());
	usFrom=TTime(0).MicroSecondsFrom(time);
	test(usFrom==-time.Int64());
	usFrom=time.MicroSecondsFrom(TTime(-1));
	test(usFrom==time.Int64()+1);
	usFrom=TTime(-1).MicroSecondsFrom(time);
	test(usFrom==-time.Int64()-1);
	usFrom=time.MicroSecondsFrom(TTime(1));
	test(usFrom==time.Int64()-1);
	usFrom=TTime(1).MicroSecondsFrom(time);
	test(usFrom==-time.Int64()+1);
	TTime time2=time+TTimeIntervalYears(0);
	test(time2==time);
	time2=time+TTimeIntervalYears(1);
	r=time2.DateTime().Year();
 	test(r==KTestArray[i].iYear+1);
	time2=time+TTimeIntervalMonths(0);
	test(time2==time);
	time2=time+TTimeIntervalMonths(1);
	r=time2.DateTime().Month();
 	test(r==(KTestArray[i].iMonth+1)%12);
	time2=time+TTimeIntervalDays(0);
	test(time2==time);
	time2=time+TTimeIntervalHours(0);
	test(time2==time);
	time2=time+TTimeIntervalMinutes(0);
	test(time2==time);
	time2=time+TTimeIntervalSeconds(0);
	test(time2==time);
	time2=time+TTimeIntervalMicroSeconds(0);
	test(time2==time);
	time.RoundUpToNextMinute();
	test(time==TTime(KTestArray[i].iNextMin));

	}


void TestTTime::TestSecureClock()
{
	// See if secure clock present and early exit if its not enabled
	TInt nso = 0;
	TInt r = HAL::Get(HAL::ETimeNonSecureOffset,nso);
	if (r != KErrNone) {
		RDebug::Printf("WARNING: Secure clock test skipped because offset HAL attribute not present!");
		return;
	}

	// Get the secure and nonsecure times
	TTime securetime, now, march2001;
	r = securetime.HomeTimeSecure();
	if (r==KErrNoSecureTime) {
		TDateTime randomdate;
		randomdate.Set(2005, ESeptember, 13, 0,0,0,0);
		r = User::SetHomeTimeSecure(randomdate);
		test_Equal(KErrNone, r);
		r = securetime.HomeTimeSecure();
	}
	test_Equal(KErrNone, r);
	now.HomeTime();
	PrintTime("hometime=", now);
	PrintTime("securetime=", securetime);

	// Set nonsecure time to March 2001
	TDateTime bday;
	bday.Set(2001, EMarch, 6, 0,0,0,0);
	march2001 = bday;
	r = User::SetHomeTime(march2001);
	test(r==KErrNone);

	// Check the nonsecure system time really updated to March 2001
	TTime now2, securetime2;
	TTimeIntervalSeconds seconds_diff(100);
	now2.HomeTime();
	r=now2.SecondsFrom(march2001, seconds_diff);
	test(r==0);
	test(seconds_diff == TTimeIntervalSeconds(0)); 

	// Check the secure system time did not change as a result of changing nonsecure time
	r = securetime2.HomeTimeSecure();
	test(r==KErrNone);
	seconds_diff = TTimeIntervalSeconds(100); 
	r=securetime2.SecondsFrom(securetime, seconds_diff);
	test(r==0);
	test(seconds_diff == TTimeIntervalSeconds(0));

	// Set secure time to March 2001 (this would fail without DRM rights)
	// *** NB: Setting H4's rtc to any time before 1/1/2000 ***
	// *** will not work but no error will be reported!     ***
	securetime2 = march2001;
	r = User::SetHomeTimeSecure(securetime2);
	test(r==KErrNone);

	// Check both secure & nonsecure system times are March 2001
	TTime now3, securetime3;
	now3.HomeTime();
	r = securetime3.HomeTimeSecure();
	test(r==KErrNone);
	r = securetime3.SecondsFrom(march2001, seconds_diff);
	test(seconds_diff == TTimeIntervalSeconds(0));
	r = now3.SecondsFrom(march2001, seconds_diff);
	test(seconds_diff == TTimeIntervalSeconds(0));

	// Verify the offset changes by the right amount when the nonsecure time is changed
	TTime time;
	r = HAL::Get(HAL::ETimeNonSecureOffset,nso);
	test_Equal(KErrNone, r);
	time.UniversalTime();
    time+=TTimeIntervalMinutes(30);
	TInt nso_expected = nso + 30*60;
	r=User::SetUTCTime(time);
	test_Equal(KErrNone, r);
	r = HAL::Get(HAL::ETimeNonSecureOffset,nso);
	test_Equal(KErrNone, r);
	test_Equal(nso_expected, nso);

	// Restore secure clock and system time to what they were at the top of this function
	r = User::SetHomeTimeSecure(securetime);
	test_Equal(KErrNone, r);
	r = User::SetHomeTime(now);
	test_Equal(KErrNone, r);

}

GLDEF_C TInt E32Main()	
	{

	test.Title();
	test.Start(_L("Testing TDateTime classes"));
	TestTTime T;

	TLocale savedLocale;

	TLocale b;
	b.SetDateSeparator('\0',0);
	b.SetDateSeparator('/',1);
	b.SetDateSeparator('/',2);
	b.SetDateSeparator('\0',3);
	b.SetDateFormat(EDateEuropean);
	b.SetTimeFormat(ETime12);
	b.SetTimeSeparator('\0',0);
	b.SetTimeSeparator(':',1);
	b.SetTimeSeparator(':',2);
	b.SetTimeSeparator('\0',3);
	b.SetAmPmSpaceBetween(ETrue);
	b.SetAmPmSymbolPosition(ELocaleAfter);
	b.SetWorkDays(0x1F);
	b.SetStartOfWeek(EMonday);
	b.Set();

	TTimeIntervalSeconds savedOffset = User::UTCOffset();
	User::SetUTCOffset(0);

    test.Next(_L("Testing TDateTime class"));
	T.Test1();
	test.Next(_L("Testing TTimeIntervalMicroSeconds"));
	T.Test2();
    test.Next(_L("Testing TTimeIntervalSeconds"));
	T.Test3();
	test.Next(_L("Testing other time intervals"));
	T.Test4();
	test.Next(_L("Testing TDateTime To TTime conversions"));	
	T.Test5();
	test.Next(_L("Testing adding TTimeIntervals and Subtracting TTimes"));
    T.Test6();
    test.Next(_L("Day numbers in week and year"));
    T.Test7();
	test.Next(_L("week numbers in year"));
	T.Test8();
	test.Next(_L("String parsing"));
	T.Test9();
	T.Test9();
	test.Next(_L("Remaining Time functions"));
	//T.Test10();
	test.Next(_L("Test time change"));
	T.Test11();
	test.Next(_L("Test TTime::Set(TDesC aString)"));
	T.Test12();
	test.Next(_L("Test negative times"));
	T.Test13();
	test.Next(_L("Test secure clock"));
	T.TestSecureClock();
    test.Next(_L("The year 2000"));
    TTime year2000(TDateTime(2000,EJanuary,0,0,0,0,0));
    test.Printf(_L("\tYear 2000 = %016lx\n"),year2000.Int64());
	savedLocale.Set();	//restore locale info
	User::SetUTCOffset(savedOffset);
	test.End();
	return(0);
    }
