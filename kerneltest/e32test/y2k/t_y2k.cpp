// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\y2k\t_y2k.cpp
// Overview:
// Year 2000 compliance tests for E32
// API Information:
// TTime, TDateTime, RTimer
// Details:
// - Test the T_Time & TDateTime classes in accordance with the test spec for Y2K
// Section 4.1.1.1. Set a variety of date and time values, calculate using different
// TTimeInterval methods, verify results are as expected.
// - Test the input of valid and invalid dates to TDateTime and TTime. Verify results.
// - Test operators <=, >=, >, <, +, - on TDateTime and TTime objects. Verify results.
// - Test days of the week, verify results are as expected.
// - Test various timer alarms, verify results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

// Use this to test all the elements, and still give useful line numbers
#define dttest(q,x,y,z,a,b,c,d) test(q.Year()==x), test(q.Month()==(y)-1), \
 test(q.Day()==(z)-1), test(q.Hour()==a), test(q.Minute()==b), test(q.Second()==c),\
 test(q.MicroSecond()==d)

LOCAL_D RTest test(_L("T_Y2K"));

#define __TRACE_LINE	test.Printf(_L("line %d\n"),__LINE__)

// test fields in TDateTime, using sensible dates (ie 1/1/1999 not 0/0/1999)

class TestY2K
	{
public:
	void Test1();  // test TTime for Y2K compliance
	void Test2();
	void Test3();
	void Test4();
	void Test5();
	};

void PrintTime(const TDesC& aName, const TTime& aTime)
	{
	TDateTime dt(aTime.DateTime());
	test.Printf(_L("%S = %02d:%02d:%02d:%06d\n"),&aName,dt.Hour(),dt.Minute(),dt.Second(),dt.MicroSecond());
	}

void TestY2K::Test1()
	{
	// Test the T_Time & TDateTime classes in accordance with the test spec for Y2K
	// Section 4.1.1.1
	
	// 31/12/1998 and 1/1/1999
	test.Next(_L("Testing existing functionality"));
	TTime t1;
	TTime t2;
	TDateTime dt;

	test(t1.Set(_L("19981130:235959.999999"))==KErrNone);
	test(t2.Set(_L("19990000:000000."))==KErrNone);

	dt=t1.DateTime();

	test(dt.Year()==1998);
	test(dt.Month()==EDecember);
	test(dt.Day()==30);
	
	t1+=TTimeIntervalMicroSeconds(1);
	dt=t1.DateTime();

	dttest(dt,1999,1,1,0,0,0,0);
	test(t1==t2);
	
	test(t1.Set(_L("19981130:235900.000000"))==KErrNone);
	test(t2.Set(_L("19990000:000001.000000"))==KErrNone);
	
	
	t1+=TTimeIntervalSeconds(61);
	dt=t1.DateTime();

	dttest(dt,1999,1,1,0,0,1,0);
	test(t1==t2);

	t1.Set(_L("19981130:234500.000100"));
	t2.Set(_L("19990000:000100.000100"));
	t1+=TTimeIntervalMinutes(16);
	dt=t1.DateTime();

	dttest(dt,1999,1,1,0,1,0,100);
	test(t1==t2);

	t1.Set(_L("19981130:230000.000001"));
	t2.Set(_L("19990000:000000.000001"));
	t1+=TTimeIntervalHours(1);
	dt=t1.DateTime();

	dttest(dt,1999,1,1,0,0,0,1);
	test(t1==t2);

	t1.Set(_L("19990000:164523.101000"));
	t2.Set(_L("19981130:164523.101000"));
	t1-=TTimeIntervalDays(1);
	dt=t1.DateTime();

	dttest(dt,1998,12,31,16,45,23,101000);
	test(t1==t2);

	t1.Set(_L("19990030:164523.999999"));
	t2.Set(_L("19981130:164523.999999"));
	t1-=TTimeIntervalMonths(1);
	dt=t1.DateTime();

	dttest(dt,1998,12,31,16,45,23,999999);
	test(t1==t2);

	t1.Set(_L("19990000:235959.999999"));
	t2.Set(_L("19980000:235959.999999"));
	t1-=TTimeIntervalYears(1);
	dt=t1.DateTime();

	dttest(dt,1998,1,1,23,59,59,999999);
	test(t1==t2);

	// Direct setting
	t1.Set(_L("19981130:235959.999999"));
	test(dt.SetYear(1998)==KErrNone);
	test(dt.SetMonth(EDecember)==KErrNone);
	test(dt.SetDay(30)==KErrNone);
	test(dt.SetHour(23)==KErrNone);
	test(dt.SetMinute(59)==KErrNone);
	test(dt.SetSecond(59)==KErrNone);
	test(dt.SetMicroSecond(999999)==KErrNone);
	test(t1==dt);

	t1.Set(_L("19990000:101010.101000"));
	test(dt.SetYear(1999)==KErrNone);
	test(dt.SetMonth(EJanuary)==KErrNone);
	test(dt.SetDay(0)==KErrNone);
	test(dt.SetHour(10)==KErrNone);
	test(dt.SetMinute(10)==KErrNone);
	test(dt.SetSecond(10)==KErrNone);
	test(dt.SetMicroSecond(101000)==KErrNone);
	test(t1==dt);

	
	// 27/2/1998 & 28/2/1998

	t1.Set(_L("19980126:144559.101000"));
	t2.Set(_L("19980127:144559.101000"));

	dt=t1.DateTime();
	dttest(dt,1998,2,27,14,45,59,101000);
		
	t1+=TTimeIntervalDays(1);
	test(t1==t2);
	dt=t2.DateTime();

	dttest(dt,1998,2,28,14,45,59,101000);

	// 28/2/1998 & 1/3/1998

	t1.Set(_L("19980127:"));
	t2.Set(_L("19980200:"));
	dt=t1.DateTime();
	
	dttest(dt,1998,2,28,0,0,0,0);

	t1+=TTimeIntervalDays(1);
	test(t1==t2);
	dt=t2.DateTime();

	dttest(dt,1998,3,1,0,0,0,0);

	// 31/8/1999 & 1/9/1999

	t1.Set(_L("19990730:235959.121200"));
	t2.Set(_L("19990800:000000.121200"));
	dt=t1.DateTime();

	dttest(dt,1999,8,31,23,59,59,121200);
	
	t1+=TTimeIntervalSeconds(1);
	test(t1==t2);
	dt=t2.DateTime();

	dttest(dt,1999,9,1,0,0,0,121200);

	// 8/9/1999 & 9/9/1999

	t1.Set(_L("19990807:233434."));
	t2.Set(_L("19990808:003434."));
	dt=t1.DateTime();

	dttest(dt,1999,9,8,23,34,34,0);

	t1+=TTimeIntervalHours(1);
	test(t1==t2);
	dt=t2.DateTime();

	dttest(dt,1999,9,9,0,34,34,0);

	// 9/9/1999 & 10/9/1999

	t1.Set(_L("19990808:235934."));
	t2.Set(_L("19990809:000034."));
   	dt=t1.DateTime();
	
	dttest(dt,1999,9,9,23,59,34,0);
	
	t1+=TTimeIntervalMinutes(1);
	test(t1==t2);
	dt=t2.DateTime();

	dttest(dt,1999,9,10,0,0,34,0);
 
	// 31/12/1999 & 1/1/2000

	test(t1.Set(_L("19991130:"))==KErrNone);
	test(t2.Set(_L("20000000:"))==KErrNone);
	dt=t1.DateTime();

	dttest(dt,1999,12,31,0,0,0,0);
	
	t1+=TTimeIntervalDays(1);
	test(t1==t2);
	dt=t2.DateTime();

	dttest(dt,2000,1,1,0,0,0,0);

	test(dt.SetYear(2000)==KErrNone);
	test(dt.SetMonth(EJanuary)==KErrNone);
	test(dt.SetDay(0)==KErrNone);
	test(dt.SetHour(0)==KErrNone);
	test(dt.SetMinute(0)==KErrNone);
	test(dt.SetSecond(0)==KErrNone);
	test(dt.SetMicroSecond(0)==KErrNone);

	test(t1==dt);
	test(t1.DayNoInYear()==1);
	test(t1.WeekNoInYear(EFirstWeek)==1);
	test(t2.DaysInMonth()==31);

	// 27/2/2000 to 28/2/2000

	test(t1.Set(_L("20000126:235958.999999"))==KErrNone);
	test(t2.Set(_L("20000127:"))==KErrNone);
	dt=t1.DateTime();

	dttest(dt,2000,2,27,23,59,58,999999);

	t1+=TTimeIntervalSeconds(1);
	t1+=TTimeIntervalMicroSeconds(1);
	test(t1==t2);
	test(t1.DaysInMonth()==29);
	dt=t2.DateTime();

	dttest(dt,2000,2,28,0,0,0,0);

	// 28/2/2000 & 29/2/2000

    test(t2.Set(_L("20000128:"))==KErrNone);
	test(dt.Set(2000,EFebruary,28,0,0,0,0)==KErrNone);

	t1+=TTimeIntervalDays(1);
	test(t1==t2);
	test(t1==dt);
	test(Time::IsLeapYear(t2.DateTime().Year()));
	test(t1.DayNoInMonth()==28);
	test(t1.DaysFrom(t2)==TTimeIntervalDays(0));
	t2.Set(_L("20000200:"));
	test(t1.DaysFrom(t2)==TTimeIntervalDays(-1));
	test(t2.DaysFrom(t1)==TTimeIntervalDays(1));
	
	// 29/2/2000 to 1/3/2000

	t1.Set(_L("20000128:"));
	t2.Set(_L("20000200:"));

	dt=t1.DateTime();
	dttest(dt,2000,2,29,0,0,0,0);
	t1+=TTimeIntervalHours(24);
	test(t1==t2);
	dt=t1.DateTime();
	dttest(dt,2000,3,1,0,0,0,0);

	// 31/12/2000 & 1/1/2001

	t1.Set(_L("20001130:"));
	t2.Set(_L("20010000:235945."));
	dt=t1.DateTime();
	dttest(dt,2000,12,31,0,0,0,0);
	test(t1!=t2);
	t1+=TTimeIntervalHours(24);
	t1+=TTimeIntervalMinutes(60*23);
	t1+=TTimeIntervalSeconds(59*60+45);
	test(t1==t2);
	
	dt=t1.DateTime();
	dttest(dt,2001,1,1,23,59,45,0);

	// 28/2/2001 & 1/3/2001
	test(t1.Set(_L("20010127:"))==KErrNone);
	test(t2.Set(_L("20010200:"))==KErrNone);
	test(t2.DaysFrom(t1)==TTimeIntervalDays(1));
	test(t2.MonthsFrom(t1)==TTimeIntervalMonths(0));
	dt=t1.DateTime();
	dttest(dt,2001,2,28,0,0,0,0);
	t1+=TTimeIntervalDays(1);
	test(t1==t2);

	dt=t1.DateTime();
	dttest(dt,2001,3,1,0,0,0,0);

	// 28/2/2004 29/2/2004
	t1.Set(_L("20040127:"));
	test(t2.Set(_L("20040128:"))==KErrNone);
	test(Time::IsLeapYear(t1.DateTime().Year()));
	test(t2.DaysFrom(t1)==TTimeIntervalDays(1));
	dt=t1.DateTime();
	dttest(dt,2004,2,28,0,0,0,0);
	t1+=TTimeIntervalMinutes(60*24);
	test(t1==t2);
	dt=t2.DateTime();
	dttest(dt,2004,2,29,0,0,0,0);

	// 29/2/2004 & 1/3/2004
	test(t1.Set(_L("20040128:"))==KErrNone);
	test(t2.Set(_L("20040200:"))==KErrNone);
	test(Time::IsLeapYear(t2.DateTime().Year()));
	dt=t1.DateTime();
	dttest(dt,2004,2,29,0,0,0,0);
	t1+=TTimeIntervalSeconds(60*60*24);
	test(t1==t2);
	dt=t1.DateTime();
	dttest(dt,2004,3,1,0,0,0,0);
	
	}

void TestY2K::Test2()
	{
	// Test the input of valid and invalid dates to TDateTime and TTime
	// Section 4.2

	TTime t1,t2;
	TDateTime dt;
	// 31/12/1998
	test(dt.Set(1998,EDecember,30,0,0,0,0)==KErrNone);
	test(t1.Set(_L("19981130:"))==KErrNone);
	dttest(dt,1998,12,31,0,0,0,0);

	// 1/3/1999
	test(dt.Set(1999,EMarch,0,0,0,0,0)==KErrNone);
	test(t1.Set(_L("19990200:"))==KErrNone);
	dttest(dt,1999,3,1,0,0,0,0);

	// 27/2/2000
	test(dt.Set(2000,EFebruary,26,0,0,0,0)==KErrNone);
	test(t1.Set(_L("20000126:"))==KErrNone);
	dttest(dt,2000,2,27,0,0,0,0);

	// 31/12/2000
	test(dt.Set(2000,EDecember,30,0,0,0,0)==KErrNone);
	test(t1.Set(_L("20001130:225645."))==KErrNone);
	dttest(dt,2000,12,31,0,0,0,0);

	// 28/2/2004
	test(dt.Set(2004,EFebruary,27,1,2,3,4)==KErrNone);
	test(t1.Set(_L("20040127:"))==KErrNone);
	dttest(dt,2004,2,28,1,2,3,4);

	// 1/1/1999
	test(dt.Set(1999,EJanuary,0,0,0,0,0)==KErrNone);
	test(t1.Set(_L("19990000:"))==KErrNone);
	dttest(dt,1999,1,1,0,0,0,0);

	// 9/9/1999
	test(dt.Set(1999,ESeptember,8,0,0,0,0)==KErrNone);
	test(t1.Set(_L("19990808:"))==KErrNone);
	dttest(dt,1999,9,9,0,0,0,0);

	// 28/2/2000
	test(dt.Set(2000,EFebruary,27,0,0,0,0)==KErrNone);
	test(t1.Set(_L("20000127:"))==KErrNone);
	dttest(dt,2000,2,28,0,0,0,0);

	// 1/1/2001
	test(dt.Set(2001,EJanuary,0,0,0,0,0)==KErrNone);
	test(t1.Set(_L("20010000:"))==KErrNone);
	dttest(dt,2001,1,1,0,0,0,0);

	// 29/2/2004
	test(dt.Set(2004,EFebruary,28,2,3,4,5)==KErrNone);
	test(t1.Set(_L("20040128:"))==KErrNone);
	dttest(dt,2004,2,29,2,3,4,5);

	// 27/2/1999
	test(dt.Set(1999,EFebruary,26,0,0,0,0)==KErrNone);
	test(t1.Set(_L("19990126:"))==KErrNone);
	dttest(dt,1999,2,27,0,0,0,0);

	// 31/12/1999
	test(dt.Set(1999,EDecember,30,0,0,0,0)==KErrNone);
	test(t1.Set(_L("19991130:"))==KErrNone);
	dttest(dt,1999,12,31,0,0,0,0);

	// 29/2/2000
	test(dt.Set(2000,EFebruary,28,0,0,0,0)==KErrNone);
	test(t1.Set(_L("20000128:"))==KErrNone);
	dttest(dt,2000,2,29,0,0,0,0);

	// 28/2/2001
	test(dt.Set(2001,EFebruary,27,0,0,0,0)==KErrNone);
	test(t1.Set(_L("20010127:"))==KErrNone);
	dttest(dt,2001,2,28,0,0,0,0);

	// 1/3/2004
	test(dt.Set(2004,EMarch,0,0,0,0,0)==KErrNone);
	test(t1.Set(_L("20040200:"))==KErrNone);
	dttest(dt,2004,3,1,0,0,0,0);

	// 28/2/1999
	test(dt.Set(1999,EFebruary,27,0,0,0,0)==KErrNone);
	test(t1.Set(_L("19990127:"))==KErrNone);
	dttest(dt,1999,2,28,0,0,0,0);

	// 1/1/2000
	test(dt.Set(2000,EJanuary,0,0,0,0,0)==KErrNone);
	test(t1.Set(_L("20000000:"))==KErrNone);
	dttest(dt,2000,1,1,0,0,0,0);

	// 1/3/2000
	test(dt.Set(2000,EMarch,0,0,0,0,0)==KErrNone);
	test(t1.Set(_L("20000200:"))==KErrNone);
	dttest(dt,2000,3,1,0,0,0,0);

	// 1/3/2001
	test(dt.Set(2001,EMarch,0,0,0,0,0)==KErrNone);
	test(t1.Set(_L("20010200:"))==KErrNone);
	dttest(dt,2001,3,1,0,0,0,0);

	// Invalid dates
	// 31/4/1998
	test(dt.Set(1998,EApril,30,0,0,0,0)!=KErrNone);
	test(t1.Set(_L("19980330:"))!=KErrNone);
	dt=t1.DateTime();
	dttest(dt,2001,3,1,0,0,0,0);

	// 30/2/2000
	test(dt.Set(2000,EFebruary,29,0,0,0,0)!=KErrNone);
	test(t1.Set(_L("20000129:"))!=KErrNone);
	dt=t1.DateTime();
	dttest(dt,2001,3,1,0,0,0,0);

	// 29/2/2001
	test(dt.Set(2001,EFebruary,28,0,0,0,0)!=KErrNone);
	test(t1.Set(_L("20010128:"))!=KErrNone);
	dt=t1.DateTime();
	dttest(dt,2001,3,1,0,0,0,0);

	// 29/2/1999
	test(dt.Set(1999,EFebruary,28,0,0,0,0)!=KErrNone);
	test(t1.Set(_L("19990128:"))!=KErrNone);
	dt=t1.DateTime();
	dttest(dt,2001,3,1,0,0,0,0);

	// 30/2/2004
	test(dt.Set(2004,EFebruary,29,0,0,0,0)!=KErrNone);
	test(t1.Set(_L("20040129:"))!=KErrNone);
	dt=t1.DateTime();
	dttest(dt,2001,3,1,0,0,0,0);

	// test of TTime::Parse
	// Set UniversalTime to C19 first
	TTime oldtime;
	oldtime.UniversalTime();
	t1.Set(_L("19990303:"));
	test(User::SetUTCTime(t1)==KErrNone);
	
	test(t1.Parse(_L("1/1/00"))>=0);
	dt=t1.DateTime();
	dttest(dt,1900,1,1,0,0,0,0);
	test(t1.Parse(_L("1/1/00"),1)>=0);
	dt=t1.DateTime();
	dttest(dt,2000,1,1,0,0,0,0);

	test(t1.Parse(_L("2/3/2000"),20)>=0);
	dt=t1.DateTime();
	dttest(dt,2000,3,2,0,0,0,0);

	test(t1.Parse(_L("31/12/99"),50)>=0);
	dt=t1.DateTime();
	dttest(dt,1999,12,31,0,0,0,0);

	test(t1.Parse(_L("1/1/99"))>=0);
	dt=t1.DateTime();
	dttest(dt,1999,1,1,0,0,0,0);

	test(t1.Parse(_L("1/1/99"),98)>=0);
	dt=t1.DateTime();
	TBuf<100> str;
	t1.FormatL(str,_L("%F %D %M %Y %H:%T:%S.%C"));
	test.Printf(str);
	test.Printf(_L("\n"));
	dttest(dt,1999,1,1,0,0,0,0);

	// Reset UniversalTime
	test(User::SetUTCTime(oldtime) == KErrNone);

	// Year lengths
	t1.Set(_L("20000000:"));
	t2.Set(_L("20001130:"));
	test(t2.DaysFrom(t1)==TTimeIntervalDays(365));
	test(Time::IsLeapYear(t1.DateTime().Year()));
	test(Time::IsLeapYear(t2.DateTime().Year()));
	t1.Set(_L("20000100:"));
	test(t1.DaysInMonth()==29);

	t1.Set(_L("19990000:"));
	t2.Set(_L("19991130:"));
	test(t2.DaysFrom(t1)==TTimeIntervalDays(364));
	test(Time::IsLeapYear(t1.DateTime().Year())==EFalse);
	test(Time::IsLeapYear(t2.DateTime().Year())==EFalse);
	t1.Set(_L("19990100:"));
	test(t1.DaysInMonth()==28);

	t1.Set(_L("20040000:"));
	t2.Set(_L("20041130:"));
	test(t2.DaysFrom(t1)==TTimeIntervalDays(365));
	test(Time::IsLeapYear(t2.DateTime().Year()));
	test(Time::IsLeapYear(t1.DateTime().Year()));
	t1.Set(_L("20040100:"));
	test(t1.DaysInMonth()==29);
		   
	}


void TestY2K::Test3()
	{
	TTime t1,t2;
	TDateTime dt;

	// Test operator <=, >=, >, <, +, -

	// mid 1999 to 31/12/1999
	t1.Set(_L("19990620:145459."));
	t2.Set(_L("19991130:"));
	test(!(t1>t2));
	test(t1<t2);
	test(t1<=t2);
	test(!(t1>=t2));
	test(!(t1==t2));

	t1-=TTimeIntervalSeconds(59);
	t1-=TTimeIntervalMinutes(54);
	t1-=TTimeIntervalHours(14);
	t1=t1+TTimeIntervalDays(10);
	t1+=TTimeIntervalMonths(5);
	test(t1==t2);
	t2.Set(_L("19990730:"));
	t1-=TTimeIntervalMonths(4);
	test(t1==t2);

	// Mid 1999 to 1/1/2000
	t1.Set(_L("19990500:"));
	t2.Set(_L("20000000:"));
	test(t2>t1);
	test(t2>=t1);
	test(!(t2<t1));
	test(!(t2<=t1));
	t1+=TTimeIntervalMonths(7);
	test(t1==t2);
	t2.Set(_L("19990707:020202.2"));
	t1+=TTimeIntervalMicroSeconds(2);
	t1=t1+TTimeIntervalSeconds(2)+TTimeIntervalMinutes(2)+TTimeIntervalHours(2);
	t1-=TTimeIntervalDays(24);
	t1-=TTimeIntervalMonths(4);
	dt=t1.DateTime();
	test(t1==t2);

	// Mid 1999 to 20/2/2000
	t1.Set(_L("19990819:"));
	t2.Set(_L("20000119:"));
	test(t1<t2);
	test(t1<=t2);
	test(!(t1>t2));
	test(!(t1>=t2));
	test(!(t1==t2));
	t1+=TTimeIntervalMonths(5);
	test(t1==t2);
	t2.Set(_L("19990600:"));
	t1-=TTimeIntervalMonths(7);
	t1-=TTimeIntervalDays(19);
	test(t1==t2);

	// Mid 1999 to 1/3/2000
	t1.Set(_L("19990700:122345."));
	t2.Set(_L("20000200:122345."));
	test(t1<t2);
	test(t1<=t2);
	test(!(t1>t2));
	test(!(t1>=t2));
	test(!(t1==t2));
	t1+=TTimeIntervalMonths(7);
	test(t1==t2);
	t2.Set(_L("19990800:"));
	t1-=TTimeIntervalMonths(6);
	t1=t1-TTimeIntervalHours(12)-TTimeIntervalMinutes(23)-TTimeIntervalSeconds(45);
	test(t1==t2);

	// mid 1999 to 1/4/2000
	t1.Set(_L("19990500:"));
	t2.Set(_L("20000300:"));
	test(t1<t2);
	test(t1<=t2);
	test(!(t1>t2));
	test(!(t1>=t2));
	test(!(t1==t2));
	t1+=TTimeIntervalMonths(10);
	test(t1==t2);
	t2.Set(_L("19990600:"));
	t1-=TTimeIntervalMonths(9);
	test(t1==t2);

	// Mid 1999 to Mid 2001
	t1.Set(_L("19990620:"));
	t2.Set(_L("20010620:"));
	test(t1<t2);
	test(t1<=t2);
	test(!(t1>t2));
	test(!(t1>=t2));
	test(!(t1==t2));
	t1+=TTimeIntervalMonths(24);
	test(t1==t2);
	t2.Set(_L("19990720:"));
	t1-=TTimeIntervalMonths(23);
	test(t1==t2);

	// Mid 2000 to Mid 2001
	t1.Set(_L("20000620:"));
	t2.Set(_L("20010620:"));
	test(t1<t2);
	test(t1<=t2);
	test(!(t1>t2));
	test(!(t1>=t2));
	test(!(t1==t2));
	t1+=TTimeIntervalMonths(12);
	test(t1==t2);
	t2.Set(_L("20000720:"));
	t1-=TTimeIntervalMonths(11);
	test(t1==t2);
		
	}
	
	
void TestY2K::Test4()
	{
	TTime t1;
	TLocale loc;

	loc.SetStartOfWeek(EMonday);
	
	// 1/1/1900 was Mon
	t1.Set(_L("19000000:"));
	test(t1.DayNoInWeek()==EMonday);

	// 28/2/1900 was Wed
	t1.Set(_L("19000127:"));
	test(t1.DayNoInWeek()==EWednesday);

	// 1/3/1900 was Thur
	t1.Set(_L("19000200:"));
	test(t1.DayNoInWeek()==EThursday);

	// 28/2/1999 is sun
	t1.Set(_L("19990127:"));
	test(t1.DayNoInWeek()==ESunday);

	// 1/3/1999 is mon
	t1.Set(_L("19990200:"));
	test(t1.DayNoInWeek()==EMonday);

	// 31/12/1999
	t1.Set(_L("19991130:"));
	test(t1.DayNoInWeek()==EFriday);

	// 1/1/2000
	t1.Set(_L("20000000:"));
	test(t1.DayNoInWeek()==ESaturday);

	// 28/2/2000
	t1.Set(_L("20000127:"));
	test(t1.DayNoInWeek()==EMonday);

	// 29/2/2000
	t1.Set(_L("20000128:"));
	test(t1.DayNoInWeek()==ETuesday);

	// 1/3/2000
	t1.Set(_L("20000200:"));
	test(t1.DayNoInWeek()==EWednesday);

	// 1/1/2001
	t1.Set(_L("20010000:"));
	test(t1.DayNoInWeek()==EMonday);

	// 28/2/2004
	t1.Set(_L("20040127:"));
	test(t1.DayNoInWeek()==ESaturday);

	// 29/2/2004
	t1.Set(_L("20040128:"));
	test(t1.DayNoInWeek()==ESunday);

	// 1/3/2004
	t1.Set(_L("20040200:"));
	test(t1.DayNoInWeek()==EMonday);
		
	}

void TestY2K::Test5()
	{
	// Test various timer alarms -
	// User::At, User::After, RTimer::At, RTimer::After
	TTime t1,t2, oldtime;
	TRequestStatus stat;
	RTimer timer;
	timer.CreateLocal();
	const TInt sec=1000000;
	oldtime.UniversalTime();

	// Check that timers across the boundary work
	// 31/12/1998 to 1/1/1999
	t1.Set(_L("19981130:235959."));
	t2.Set(_L("19990000:000001."));
	test(User::SetUTCTime(t1)==KErrNone);
	timer.At(stat,t2);
	__TRACE_LINE;
	User::WaitForRequest(stat);
	t1.UniversalTime();
	PrintTime(_L("t1"),t1);
	PrintTime(_L("t2"),t2);
	test(t1 >= t2);
	test(stat==KErrNone);
	timer.At(stat,t1);
	__TRACE_LINE;
	User::WaitForRequest(stat);
	test(stat==KErrUnderflow);
	t1.Set(_L("19981130:235959."));
	t2.Set(_L("19990000:000001."));
	test(User::SetUTCTime(t1)==KErrNone);
	test(User::At(t2)==KErrNone);
	t1.UniversalTime();
	test(t1 >= t2);
	test(User::At(t1) == KErrUnderflow);

	// 31/12/1999 1/1/2000
	t1.Set(_L("19991130:235959."));
	t2.Set(_L("20000000:000000.0"));
	test(User::SetUTCTime(t1)==KErrNone);
	timer.At(stat,t2);
	__TRACE_LINE;
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	t1.UniversalTime();
	TBuf<100> str;
	t1.FormatL(str,_L("%F %D %M %Y %H:%T:%S.%C"));
	test.Printf(str);
	test.Printf(_L("\n"));
	test(t1>=t2);
	timer.At(stat,t1);
	__TRACE_LINE;
	User::WaitForRequest(stat);
	test(stat==KErrUnderflow);
	t1.Set(_L("19991130:235959."));
	t2.Set(_L("20000000:000000.00000"));
	test(User::SetUTCTime(t1)==KErrNone);
	test(User::At(t2)==KErrNone);
	t1.UniversalTime();
	test(t1>=t2);

	// 31/12/2000 to 1/1/2001
	t1.Set(_L("20001130:235958."));
	t2.Set(_L("20010000:"));
	test(User::SetUTCTime(t1)==KErrNone);
	timer.At(stat,t2);
	__TRACE_LINE;
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	t1.UniversalTime();
	test(t1>=t2);
	timer.At(stat,t1);
	__TRACE_LINE;
	User::WaitForRequest(stat);
	test(stat==KErrUnderflow);
	t1.Set(_L("20001130:235958."));
	t2.Set(_L("20010000:"));
	test(User::SetUTCTime(t1)==KErrNone);
	test(User::At(t2)==KErrNone);
	t1.UniversalTime();
	test(t1>=t2);
	
	// Check that tick timers across the C work

	// 31/12/1998 & 1/1/1999
	t1.Set(_L("19981130:235959."));
	t2.Set(_L("19990000:"));
	test(User::SetUTCTime(t1)==KErrNone);
	timer.After(stat,sec*2);
	__TRACE_LINE;
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	t1.UniversalTime();
	test(t1>=t2);
	t1.Set(_L("19981130:235959."));
	t2.Set(_L("19990000:"));
	test(User::SetUTCTime(t1)==KErrNone);
	User::After(sec*2);
	t1.UniversalTime();
	test(t1>=t2);
	t1.Set(_L("19981130:235959."));
	t2.Set(_L("19990000:"));
	test(User::SetUTCTime(t1)==KErrNone);
	test(User::At(t2)==KErrNone);
	t1.UniversalTime();
	test(t1>=t2);

	// 31/12/1999 & 1/1/2000
	t1.Set(_L("19991130:235959."));
	t2.Set(_L("20000000:"));
	test(User::SetUTCTime(t1)==KErrNone);
	timer.After(stat,sec*2);
	__TRACE_LINE;
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	t1.UniversalTime();
	test(t1>=t2);
	t1.Set(_L("19991130:235959."));
	t2.Set(_L("20000000:"));
	test(User::SetUTCTime(t1)==KErrNone);
	User::After(sec*2);
	t1.UniversalTime();
	test(t1>=t2);
	t1.Set(_L("19991130:235959."));
	t2.Set(_L("20000000:"));
	test(User::SetUTCTime(t1)==KErrNone);
	test(User::At(t2)==KErrNone);
	t1.UniversalTime();
	test(t1>=t2);

	//31/12/2000 and 1/1/2001
	t1.Set(_L("20001130:235959."));
	t2.Set(_L("20010000:"));
	test(User::SetUTCTime(t1)==KErrNone);
	timer.After(stat, sec*5);
	__TRACE_LINE;
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	t1.UniversalTime();
	test(t1>=t2);
	t1.Set(_L("20001130:235959."));
	t2.Set(_L("20010000:"));
	test(User::SetUTCTime(t1)==KErrNone);
	User::After(sec*5);
	t1.UniversalTime();
	test(t1>=t2);
	t1.Set(_L("20001130:235959."));
	t2.Set(_L("20010000:"));
	test(User::SetUTCTime(t1)==KErrNone);
	test(User::At(t2)==KErrNone);
	t1.UniversalTime();
	test(t1>=t2);

	// check that timers abort across the C, with timer b4 and after
	// date changed to.
	TTime times[4];
	RTimer timers[3];
	TRequestStatus stats[3];
	TInt i;
	
	for(i=0;i<3;i++)
		timers[i].CreateLocal();
	
	// 31/12/1998 & 1/1/1999
	times[0].Set(_L("19981129:"));
	times[1].Set(_L("19981130:235959.999999"));
	times[2].Set(_L("19990203:234500."));
	times[3].Set(_L("19990506:"));
	
	test(User::SetUTCTime(times[0])==KErrNone);
	for(i=1;i<4;i++)
		timers[i-1].At(stats[i-1], times[i]);
	User::SetUTCTime(times[2]);
	// Should all abort
	for(i=0;i<3;i++)
		{
		__TRACE_LINE;
		User::WaitForRequest(stats[i]);
		test(stats[1]==KErrAbort);
		}

	// 31/12/1999 & 1/1/2000
	times[0].Set(_L("19991029:"));
	times[1].Set(_L("19991130:235959.999999"));
	times[2].Set(_L("20000203:234500."));
	times[3].Set(_L("20000506:"));
	
	test(User::SetUTCTime(times[0])==KErrNone);
	for(i=1;i<4;i++)
		timers[i-1].At(stats[i-1], times[i]);
	User::SetUTCTime(times[2]);
	// Should all abort
	for(i=0;i<3;i++)
		{
		__TRACE_LINE;
		User::WaitForRequest(stats[i]);
		test(stats[1]==KErrAbort);
		}

	// 31/12/2000 and 1/1/2001
	times[0].Set(_L("20001129:"));
	times[1].Set(_L("20001130:235959.999999"));
	times[2].Set(_L("20010203:234500."));
	times[3].Set(_L("20010506:"));
	
	test(User::SetUTCTime(times[0])==KErrNone);
	for(i=1;i<4;i++)
		timers[i-1].At(stats[i-1], times[i]);
	User::SetUTCTime(times[2]);
	// Should all abort
	for(i=0;i<3;i++)
		{
		__TRACE_LINE;
		User::WaitForRequest(stats[i]);
		test(stats[1]==KErrAbort);
		}

	// Check that overflow works
	t1.Set(_L("99991130:"));
	test(User::At(t1)==KErrOverflow);
	test(User::SetUTCTime(oldtime)==KErrNone);
	}

GLDEF_C TInt E32Main()	
	{

	TTime oldtime;
	oldtime.UniversalTime();
	test.Title();
	test.Start(_L("Testing Y2K behaviour"));
	TestY2K T;
	TLocale currentLocale;

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

	TTimeIntervalSeconds oldOffset = User::UTCOffset();
	User::SetUTCOffset(0);

	test.Next(_L("Testing TTime & TDateTime class (section 4.1.1.1)"));
	T.Test1();
	test.Next(_L("Testing TDateTime & TTime with valid and invalid dates (Section 4.2)"));
	T.Test2();
	test.Next(_L("Testing TDateTime & TTime with sorts/calculations (Section 4.2.5)"));
	T.Test3();
	test.Next(_L("Testing week days (Section 4.2.6)"));
	T.Test4();
	test.Next(_L("Testing timers"));
	T.Test5();
	test.Next(_L("Checking we can set to 31/12/1999 and 29/2/2000"));
	TTime t1;
	t1.Set(_L("19991130:"));
	User::SetUTCTime(t1);
	t1.UniversalTime();
	TBuf<100> str;
	t1.FormatL(str,_L("%F %D %M %Y %H:%T:%S.%C"));
	test.Printf(str);
	test.Printf(_L("\n"));
	t1.Set(_L("20000128:"));
	User::SetUTCTime(t1);
	t1.UniversalTime();
	t1.FormatL(str,_L("%F %D %M %Y %H:%T:%S.%C"));
	test.Printf(str);
	test.Printf(_L("\n"));
	//test.Printf(_L("Press a key"));
	//test.Getch();
	User::SetUTCTime(oldtime);
	currentLocale.Set();
	User::SetUTCOffset(oldOffset);
	test.End();
	return (0);
	}
