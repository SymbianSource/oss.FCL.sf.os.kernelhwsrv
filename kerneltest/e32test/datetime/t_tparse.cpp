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
// e32test\datetime\t_tparse.cpp
// Overview:
// Date & time tests
// API Information:
// TTime, TDateTime
// Details:
// - Set various locale settings to known values.
// - Test parsing a variety of simple time formats. Verify results are as expected.
// - Test parsing a variety of simple date formats. Verify results are as expected.
// - Test parsing a variety of date and time formats. Verify results are as expected.
// - Attempt to parse a variety of bad date and time descriptors. Verify the error 
// results are as expected.
// - Test a variety of date and time locale changes. Verify results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

RTest test(_L("T_TPARSE"));

LOCAL_C TInt DateTimeParse(TDateTime& aDateTime,const TDesC& aDes,TInt aCenturyOffset=0)
	{

	TTime time;
	TInt r=time.Parse(aDes,aCenturyOffset);
	if (r>=0)
		aDateTime=time.DateTime();
	return r;
	}

LOCAL_D void SimpleTimeFormats()
	{

	TInt error;
	TDateTime dateTime;
	error=DateTimeParse(dateTime,_L("23:34:45:56"));
	test(error==EParseTimePresent);
	test(dateTime.Hour()==23);
	test(dateTime.Minute()==34);
	test(dateTime.Second()==45);
	test(dateTime.MicroSecond()==56);

	error=DateTimeParse(dateTime,_L("23:34:45.56"));
	test(error==EParseTimePresent);
	test(dateTime.Hour()==23);
	test(dateTime.Minute()==34);
	test(dateTime.Second()==45);
	test(dateTime.MicroSecond()==56);

	error=DateTimeParse(dateTime,_L("23:34:45.1234567"));
	test(error==KErrGeneral);

	error=DateTimeParse(dateTime,_L("23:34:45"));
	test(error==EParseTimePresent);
	test(dateTime.Hour()==23);
	test(dateTime.Minute()==34);
	test(dateTime.Second()==45);
	test(dateTime.MicroSecond()==0);

	error=DateTimeParse(dateTime,_L("00023:00034"));
	test(error==EParseTimePresent);
	test(dateTime.Hour()==23);
	test(dateTime.Minute()==34);
	test(dateTime.Second()==00);
	test(dateTime.MicroSecond()==0);

	error=DateTimeParse(dateTime,_L("23: 34"));
	test(error==EParseTimePresent);
	test(dateTime.Hour()==23);
	test(dateTime.Minute()==34);
	test(dateTime.Second()==0);
	test(dateTime.MicroSecond()==0);

	error=DateTimeParse(dateTime,_L("23: 34am"));
	test(error==EParseTimePresent);
	test(dateTime.Hour()==23);
	test(dateTime.Minute()==34);
	test(dateTime.Second()==0);
	test(dateTime.MicroSecond()==0);

	error=DateTimeParse(dateTime,_L("23: 34AM"));
	test(error==EParseTimePresent);
	test(dateTime.Hour()==23);
	test(dateTime.Minute()==34);
	test(dateTime.Second()==0);
	test(dateTime.MicroSecond()==0);

	error=DateTimeParse(dateTime,_L("     23  : 34   "));
	test(error==EParseTimePresent);
	test(dateTime.Hour()==23);
	test(dateTime.Minute()==34);
	test(dateTime.Second()==0);
	test(dateTime.MicroSecond()==0);

	DateTimeParse(dateTime,_L("12    .    34"));
	test(dateTime.Hour()==12);
	test(dateTime.Minute()==34);

	error=DateTimeParse(dateTime,_L("23:34:"));
	test(error==KErrArgument);

	error=DateTimeParse(dateTime,_L("   0012    : 00034  .    056"));
	test(error==EParseTimePresent);
	test(dateTime.Hour()==12);
	test(dateTime.Minute()==34);
	test(dateTime.Second()==56);
	test(dateTime.MicroSecond()==0);

	error=DateTimeParse(dateTime,_L("24    :    56"));
	test(error==KErrGeneral);// TDateTime class does not allow 24:56

	error=DateTimeParse(dateTime,_L("10"));
	test(error==KErrArgument);

	error=DateTimeParse(dateTime,_L("10a"));
	test(error==EParseTimePresent);
	test(dateTime.Hour()==10);

	error=DateTimeParse(dateTime,_L("10p"));
	test(error==EParseTimePresent);
	test(dateTime.Hour()==22);

	error=DateTimeParse(dateTime,_L("10 p"));
	test(error==EParseTimePresent);
	test(dateTime.Hour()==22);

	error=DateTimeParse(dateTime,_L("10pm "));
	test(error==EParseTimePresent);
	test(dateTime.Hour()==22);

	error=DateTimeParse(dateTime,_L("10 pm"));
	test(error==EParseTimePresent);
	test(dateTime.Hour()==22);

	error=DateTimeParse(dateTime,_L("10 AM"));
	test(error==EParseTimePresent);
	test(dateTime.Hour()==10);

	error=DateTimeParse(dateTime,_L("10 PM"));
	test(error==EParseTimePresent);
	test(dateTime.Hour()==22);

	error=DateTimeParse(dateTime,_L("10pmERROR"));
	test(error==KErrArgument);

	DateTimeParse(dateTime,_L("12 : 56 pm"));
	test(dateTime.Hour()==12);
	test(dateTime.Minute()==56);

	DateTimeParse(dateTime,_L("12 : 56 am "));
	test(dateTime.Hour()==0);
	test(dateTime.Minute()==56);

	DateTimeParse(dateTime,_L("12.34.56am"));
	test(dateTime.Hour()==0);
	test(dateTime.Minute()==34);
	test(dateTime.Second()==56);

	DateTimeParse(dateTime,_L("12.34.56pm "));
	test(dateTime.Hour()==12);
	test(dateTime.Minute()==34);
	test(dateTime.Second()==56);
	test(dateTime.MicroSecond()==0);

	error=DateTimeParse(dateTime,_L("12:34:56am  ERROR"));
	test(error==KErrArgument);

	error=DateTimeParse(dateTime,_L("12 : 56 am 05/jan/1996 "));
	test(error==(EParseTimePresent|EParseDatePresent));
	test(dateTime.Hour()==0);
	test(dateTime.Minute()==56);
	}

LOCAL_D void SimpleDateFormats()
	{
	TDateTime before2000DateTime(1999,EJanuary,0,0,0,0,0);
	TDateTime after2000DateTime(2001,EJanuary,0,0,0,0,0);
	TTime before2000(before2000DateTime);
	TTime after2000(after2000DateTime);
	
	for (TInt ii=0;ii<=1;ii++)
		{
		TBool currentTimeIsBeforeYear2000=ETrue;
		if (ii==0)
			{
			User::SetUTCTime(before2000);
			currentTimeIsBeforeYear2000=ETrue;
			}
		else if (ii==1)
			{
			User::SetUTCTime(after2000);
			currentTimeIsBeforeYear2000=EFalse;
			}
		TInt error;
		TDateTime dateTime;
		DateTimeParse(dateTime,_L("5/6/1996"));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==1996);

		DateTimeParse(dateTime,_L("5-6,1996"));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==1996);

		DateTimeParse(dateTime,_L("5 6 1996"));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==1996);

		DateTimeParse(dateTime,_L("5  ;  6     1996"));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==1996);

		
		DateTimeParse(dateTime,_L("5 / 6, 96 "));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1996);
		else
			test(dateTime.Year()==2096);

		DateTimeParse(dateTime,_L("5/6/19 "),20);
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==2019);

		DateTimeParse(dateTime,_L("5/6/20 "),20);
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==1920);

		DateTimeParse(dateTime,_L("5/6/00"),20);
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==2000);

		error=DateTimeParse(dateTime,_L("5/6/00  "),00);
		test(error==EParseDatePresent);
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1900);
		else
			test(dateTime.Year()==2000);

		error=DateTimeParse(dateTime,_L("june 5 /00  "),00);
		test(error==EParseDatePresent);
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1900);
		else
			test(dateTime.Year()==2000);

		error=DateTimeParse(dateTime,_L("5 june/00  "),00);
		test(error==EParseDatePresent);
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1900);
		else
			test(dateTime.Year()==2000);

		error=DateTimeParse(dateTime,_L("5june 96  "),00);
		test(error==KErrArgument);

		// two field dates

		error=DateTimeParse(dateTime,_L("5/6"));
		test(error==EParseDatePresent);
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==0);

		error=DateTimeParse(dateTime,_L("5-6,"));
		test(error==KErrArgument);

		error=DateTimeParse(dateTime,_L("5/6/ "),20);
		test(error==KErrArgument);

		error=DateTimeParse(dateTime,_L("july 11"),20);
		test(error==EParseDatePresent);
		test(dateTime.Day()==10);
		test(dateTime.Month()==EJuly);
		test(dateTime.Year()==0);

		error=DateTimeParse(dateTime,_L("11 july"),20);
		test(error==EParseDatePresent);
		test(dateTime.Day()==10);
		test(dateTime.Month()==EJuly);
		test(dateTime.Year()==0);

		error=DateTimeParse(dateTime,_L("june 5  "),00);
		test(error==EParseDatePresent);
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==0);

		error=DateTimeParse(dateTime,_L("5 june/  "),00);
		test(error==KErrArgument);

		error=DateTimeParse(dateTime,_L("5june  "),00);
		test(error==KErrArgument);

		DateTimeParse(dateTime,_L("5/6/1"));
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1901);
		else
			test(dateTime.Year()==2001);
		DateTimeParse(dateTime,_L("5/6/01"));
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1901);
		else
			test(dateTime.Year()==2001);
		DateTimeParse(dateTime,_L("5/6/001"));
		test(dateTime.Year()==0001);		
		DateTimeParse(dateTime,_L("5/6/0001"));
		test(dateTime.Year()==0001);		
		}
	}

LOCAL_D void DateTimeFormats()
	{
	TDateTime before2000DateTime(1999,EJanuary,0,0,0,0,0);
	TDateTime after2000DateTime(2001,EJanuary,0,0,0,0,0);
	TTime before2000(before2000DateTime);
	TTime after2000(after2000DateTime);
	
	for (TInt ii=0;ii<=1;ii++)
		{
		TBool currentTimeIsBeforeYear2000=ETrue;
		if (ii==0)
			{
			User::SetUTCTime(before2000);
			currentTimeIsBeforeYear2000=ETrue;
			}
		else if (ii==1)
			{
			User::SetUTCTime(after2000);
			currentTimeIsBeforeYear2000=EFalse;
			}
		TInt error;
		TDateTime dateTime;
		error=DateTimeParse(dateTime,_L("5/6/1996 10am"));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==1996);
		test(dateTime.Hour()==10);

		error=DateTimeParse(dateTime,_L("5/6/1996 10 pm "));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==1996);
		test(dateTime.Hour()==22);

		error=DateTimeParse(dateTime,_L("5/6/1996 10 : 30  "));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==1996);
		test(dateTime.Hour()==10);
		test(dateTime.Minute()==30);

		error=DateTimeParse(dateTime,_L("5/6/1996 10 : 40 pm  "));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==1996);
		test(dateTime.Hour()==22);
		test(dateTime.Minute()==40);

		error=DateTimeParse(dateTime,_L("5/6/1996 10 : 40 . 01 "));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==1996);
		test(dateTime.Hour()==10);
		test(dateTime.Minute()==40);
		test(dateTime.Second()==01);
		test(dateTime.MicroSecond()==0);

		error=DateTimeParse(dateTime,_L("5-6 ,1996    10 : 40 . 01 pm "));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==1996);
		test(dateTime.Hour()==22);
		test(dateTime.Minute()==40);
		test(dateTime.Second()==01);
		test(dateTime.MicroSecond()==0);

		error=DateTimeParse(dateTime,_L("20- feb /96    12 : 40 . 01 am"));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Day()==19);
		test(dateTime.Month()==EFebruary);
		test(dateTime.Hour()==0);
		test(dateTime.Minute()==40);
		test(dateTime.Second()==01);
		test(dateTime.MicroSecond()==0);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1996);
		else
			test(dateTime.Year()==2096);

		error=DateTimeParse(dateTime,_L("5/6/1996 10 : 40 . 01 . 02 "));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==1996);
		test(dateTime.Hour()==10);
		test(dateTime.Minute()==40);
		test(dateTime.Second()==01);
		test(dateTime.MicroSecond()==2);

		error=DateTimeParse(dateTime,_L("5-6 ,1996    10 : 40 . 01 . 03 pm "));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==1996);
		test(dateTime.Hour()==22);
		test(dateTime.Minute()==40);
		test(dateTime.Second()==01);
		test(dateTime.MicroSecond()==3);

		error=DateTimeParse(dateTime,_L("20- feb /96    12 : 40 . 01 . 04 am"));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Day()==19);
		test(dateTime.Month()==EFebruary);
		test(dateTime.Hour()==0);
		test(dateTime.Minute()==40);
		test(dateTime.Second()==01);
		test(dateTime.MicroSecond()==4);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1996);
		else
			test(dateTime.Year()==2096);

		error=DateTimeParse(dateTime,_L("20- feb /9612 : 40.01am"));
		test(error==KErrGeneral);

		// two field dates

		error=DateTimeParse(dateTime,_L("5/6 10am"));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==0);
		test(dateTime.Hour()==10);

		error=DateTimeParse(dateTime,_L("5/6/ 10 pm "));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==0);
		test(dateTime.Hour()==22);

		error=DateTimeParse(dateTime,_L("5/6 10 : 40 . 01 "));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==0);
		test(dateTime.Hour()==10);
		test(dateTime.Minute()==40);
		test(dateTime.Second()==01);
		test(dateTime.MicroSecond()==0);

		error=DateTimeParse(dateTime,_L("5-6 ,    10 : 40 . 01 pm "));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==0);
		test(dateTime.Hour()==22);
		test(dateTime.Minute()==40);
		test(dateTime.Second()==01);
		test(dateTime.MicroSecond()==0);

		error=DateTimeParse(dateTime,_L("5/6 10 : 40 . 01 . 02"));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==0);
		test(dateTime.Hour()==10);
		test(dateTime.Minute()==40);
		test(dateTime.Second()==01);
		test(dateTime.MicroSecond()==2);

		error=DateTimeParse(dateTime,_L("5-6 ,    10 : 40 . 01 . 05pm "));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		test(dateTime.Year()==0);
		test(dateTime.Hour()==22);
		test(dateTime.Minute()==40);
		test(dateTime.Second()==01);
		test(dateTime.MicroSecond()==5);

		}
	}

LOCAL_D void TimeDateFormats()
	{
	TDateTime before2000DateTime(1999,EJanuary,0,0,0,0,0);
	TDateTime after2000DateTime(2001,EJanuary,0,0,0,0,0);
	TTime before2000(before2000DateTime);
	TTime after2000(after2000DateTime);
	
	for (TInt ii=0;ii<=1;ii++)
		{
		TBool currentTimeIsBeforeYear2000=ETrue;
		if (ii==0)
			{
			User::SetUTCTime(before2000);
			currentTimeIsBeforeYear2000=ETrue;
			}
		else if (ii==1)
			{
			User::SetUTCTime(after2000);
			currentTimeIsBeforeYear2000=EFalse;
			}
		TInt error;
		TDateTime dateTime;
		error=DateTimeParse(dateTime,_L("10pm 5/6/96"));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Hour()==22);
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1996);
		else
			test(dateTime.Year()==2096);


		error=DateTimeParse(dateTime,_L("12 am  5 -feb,03 "));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Hour()==00);
		test(dateTime.Day()==4);
		test(dateTime.Month()==EFebruary);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1903);
		else
			test(dateTime.Year()==2003);

		error=DateTimeParse(dateTime,_L("12 .56am  5 -feb,03 "));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Hour()==00);
		test(dateTime.Minute()==56);
		test(dateTime.Day()==4);
		test(dateTime.Month()==EFebruary);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1903);
		else
			test(dateTime.Year()==2003);

		error=DateTimeParse(dateTime,_L("12 .56:01.03pm  5 -SEPTEMBER,03 "));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Hour()==12);
		test(dateTime.Minute()==56);
		test(dateTime.Second()==01);
		test(dateTime.MicroSecond()==03);
		test(dateTime.Day()==4);
		test(dateTime.Month()==ESeptember);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1903);
		else
			test(dateTime.Year()==2003);


		error=DateTimeParse(dateTime,_L("12 .56:01pm  SEPTEMBER 5,03 "));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Hour()==12);
		test(dateTime.Minute()==56);
		test(dateTime.Second()==01);
		test(dateTime.Day()==4);
		test(dateTime.Month()==ESeptember);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1903);
		else
			test(dateTime.Year()==2003);


		error=DateTimeParse(dateTime,_L("12 .0056:01pm  5 -SEPTEMBERX,03 "));
		test(error==KErrArgument);

		error=DateTimeParse(dateTime,_L("12 .56:015 -SEPTEMBER,03 "));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Hour()==12);
		test(dateTime.Minute()==56);
		test(dateTime.Second()==15);
		test(dateTime.Day()==2);
		test(dateTime.Month()==ESeptember);
		test(dateTime.Year()==0);
		
		// max descriptor length 27 tokens// 13 tokens after spaces are striped
		error=DateTimeParse(dateTime,_L(" 10 : 00 : 00 . 123456 pm 5 / 6 / 96 "));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.MicroSecond()==123456);
		test(dateTime.Second()==0);
		test(dateTime.Minute()==0);
		test(dateTime.Hour()==22);
		test(dateTime.Day()==4);
		test(dateTime.Month()==EJune);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1996);
		else
			test(dateTime.Year()==2096);

		
		// the formaula array will overflow with too many tokens
		error=DateTimeParse(dateTime,_L(" 10 : 00 : 00 pm 5 / 6 / 96 / / / / / / / / / / / / / / / "));
		test(error==KErrArgument);
		}
	}

LOCAL_D void BadDescriptors()
	{

	TInt error;
	TDateTime dateTime;
	error=DateTimeParse(dateTime,_L("10pmpm"));
	test(error==KErrArgument);

	error=DateTimeParse(dateTime,_L("10pmX"));
	test(error==KErrArgument);

	error=DateTimeParse(dateTime,_L("1111"));
	test(error==KErrArgument);

	error=DateTimeParse(dateTime,_L("10::10"));
	test(error==KErrGeneral);

	error=DateTimeParse(dateTime,_L("10-,10"));
	test(error==KErrArgument);

	error=DateTimeParse(dateTime,_L("   -  10:10"));
	test(error==KErrArgument);

	error=DateTimeParse(dateTime,_L(" 10  10:10"));
	test(error==KErrArgument);

	error=DateTimeParse(dateTime,_L(" 5 june 10000"));
	test(error==KErrArgument);

	error=DateTimeParse(dateTime,_L("24:01"));
	test(error==KErrGeneral);

	error=DateTimeParse(dateTime,_L("may 2, 9623:34:45"));
	test(error==KErrGeneral);

	error=DateTimeParse(dateTime,_L(" 9/10/- "));
	test(error==KErrArgument);
	}

LOCAL_D void TestLocaleChanges()
	{
	TDateTime before2000DateTime(1999,EJanuary,0,0,0,0,0);
	TDateTime after2000DateTime(2001,EJanuary,0,0,0,0,0);
	TTime before2000(before2000DateTime);
	TTime after2000(after2000DateTime);
	
	for (TInt ii=0;ii<=1;ii++)
		{
		TBool currentTimeIsBeforeYear2000=ETrue;
		if (ii==0)
			{
			User::SetUTCTime(before2000);
			currentTimeIsBeforeYear2000=ETrue;
			}
		else if (ii==1)
			{
			User::SetUTCTime(after2000);
			currentTimeIsBeforeYear2000=EFalse;
			}
		TLocale locale;
		locale.Refresh();
		TLocale savedLocale;
		TInt error;
		TDateTime dateTime;

		//Set decimal separator in locale to another value:
		locale.SetDecimalSeparator('!');
		locale.Set();
		error=DateTimeParse(dateTime,_L("3/6 10:10:10!111111"));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.MicroSecond()==111111);
		test(dateTime.Second()==10);
		test(dateTime.Minute()==10);
		test(dateTime.Hour()==10);
		test(dateTime.Day()==2);
		test(dateTime.Month()==EJune);
				
		error=DateTimeParse(dateTime,_L("1/6/12"));
		test(error==EParseDatePresent);
		test(dateTime.Second()==0);
		test(dateTime.Minute()==0);
		test(dateTime.Hour()==0);
		test(dateTime.Day()==0);
		test(dateTime.Month()==EJune);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1912);
		else
			test(dateTime.Year()==2012);

		error=DateTimeParse(dateTime,_L(" dec  3 12"),10);
		test(error==EParseDatePresent);
		test(dateTime.Second()==0);
		test(dateTime.Minute()==0);
		test(dateTime.Hour()==0);
		test(dateTime.Day()==2);
		test(dateTime.Month()==EDecember);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1912);
		else
			test(dateTime.Year()==1912);

		error=DateTimeParse(dateTime,_L(" 3 dec 12 "),12);
		test(error==EParseDatePresent);
		test(dateTime.Second()==0);
		test(dateTime.Minute()==0);
		test(dateTime.Hour()==0);
		test(dateTime.Day()==2);
		test(dateTime.Month()==EDecember);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1912);
		else
			test(dateTime.Year()==1912);

		TDateFormat dateFormat=EDateAmerican;
		locale.SetDateFormat(dateFormat);
		locale.Set();

		error=DateTimeParse(dateTime,_L("1/6/12"));
		test(error==EParseDatePresent);
		test(dateTime.Second()==0);
		test(dateTime.Minute()==0);
		test(dateTime.Hour()==0);
		test(dateTime.Day()==5);
		test(dateTime.Month()==EJanuary);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1912);
		else
			test(dateTime.Year()==2012);

		error=DateTimeParse(dateTime,_L(" dec  3 12"),13);
		test(error==EParseDatePresent);
		test(dateTime.Second()==0);
		test(dateTime.Minute()==0);
		test(dateTime.Hour()==0);
		test(dateTime.Day()==2);
		test(dateTime.Month()==EDecember);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==2012);
		else
			test(dateTime.Year()==2012);

		error=DateTimeParse(dateTime,_L(" 3 dec 12 "));
		test(error==EParseDatePresent);
		test(dateTime.Second()==0);
		test(dateTime.Minute()==0);
		test(dateTime.Hour()==0);
		test(dateTime.Day()==2);
		test(dateTime.Month()==EDecember);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1912);
		else
			test(dateTime.Year()==2012);

		dateFormat=EDateJapanese;
		locale.SetDateFormat(dateFormat);
		locale.Set();

		error=DateTimeParse(dateTime,_L("3/17"));
		test(error==EParseDatePresent);
		test(dateTime.Second()==0);
		test(dateTime.Minute()==0);
		test(dateTime.Hour()==0);
		test(dateTime.Day()==16);
		test(dateTime.Month()==EMarch);
		test(dateTime.Year()==0);

		error=DateTimeParse(dateTime,_L("1/6/12"));
		test(error==EParseDatePresent);
		test(dateTime.Second()==0);
		test(dateTime.Minute()==0);
		test(dateTime.Hour()==0);
		test(dateTime.Day()==11);
		test(dateTime.Month()==EJune);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1901);
		else
			test(dateTime.Year()==2001);

		error=DateTimeParse(dateTime,_L("1*6*12"));
		test(error==KErrArgument);

		error=DateTimeParse(dateTime,_L("1+6+12"));
		test(error==KErrArgument);

		locale.SetDateSeparator('*',1);
		locale.SetDateSeparator('+',2);
		locale.Set();

		error=DateTimeParse(dateTime,_L("1*6*12"));
		test(error==EParseDatePresent);
		test(dateTime.Second()==0);
		test(dateTime.Minute()==0);
		test(dateTime.Hour()==0);
		test(dateTime.Day()==11);
		test(dateTime.Month()==EJune);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1901);
		else
			test(dateTime.Year()==2001);

		error=DateTimeParse(dateTime,_L("1+6+12"));
		test(error==EParseDatePresent);
		test(dateTime.Second()==0);
		test(dateTime.Minute()==0);
		test(dateTime.Hour()==0);
		test(dateTime.Day()==11);
		test(dateTime.Month()==EJune);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1901);
		else
			test(dateTime.Year()==2001);

		error=DateTimeParse(dateTime,_L("1+6+12 14:32.54am"));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Second()==54);
		test(dateTime.Minute()==32);
		test(dateTime.Hour()==14);
		test(dateTime.Day()==11);
		test(dateTime.Month()==EJune);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1901);
		else
			test(dateTime.Year()==2001);

		error=DateTimeParse(dateTime,_L("10p 1+6+12"));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Second()==0);
		test(dateTime.Minute()==0);
		test(dateTime.Hour()==22);
		test(dateTime.Day()==11);
		test(dateTime.Month()==EJune);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1901);
		else
			test(dateTime.Year()==2001);

		// European
		locale.SetDateFormat(EDateEuropean);
		locale.SetDateSeparator('.',2);
		locale.Set();

		error=DateTimeParse(dateTime,_L("1+6+12"));
		test(error==KErrArgument);

		error=DateTimeParse(dateTime,_L("3.6.86"));
		test(error==EParseDatePresent);
		test(dateTime.Second()==0);
		test(dateTime.Minute()==0);
		test(dateTime.Hour()==0);
		test(dateTime.Day()==2);
		test(dateTime.Month()==EJune);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1986);
		else
			test(dateTime.Year()==2086);

		error=DateTimeParse(dateTime,_L("3.6.86 10:10:10"));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Second()==10);
		test(dateTime.Minute()==10);
		test(dateTime.Hour()==10);
		test(dateTime.Day()==2);
		test(dateTime.Month()==EJune);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1986);
		else
			test(dateTime.Year()==2086);

		locale.SetDateSeparator(':',1);
		locale.Set();

		error=DateTimeParse(dateTime,_L("3.6.86 10:10:10"));
		test(error==KErrArgument);
//		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Second()==10);
		test(dateTime.Minute()==10);
		test(dateTime.Hour()==10);
		test(dateTime.Day()==2);
		test(dateTime.Month()==EJune);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1986);
		else
			test(dateTime.Year()==2086);

		error=DateTimeParse(dateTime,_L("3:6:86 10.10.10"));
		test(error==KErrArgument);
		test(dateTime.Second()==10);
		test(dateTime.Minute()==10);
		test(dateTime.Hour()==10);
		test(dateTime.Day()==2);
		test(dateTime.Month()==EJune);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1986);
		else
			test(dateTime.Year()==2086);

		error=DateTimeParse(dateTime,_L("10.10"));
		test(error==EParseDatePresent);
		test(dateTime.Second()==0);
		test(dateTime.Minute()==0);
		test(dateTime.Hour()==0);
		test(dateTime.Day()==9);
		test(dateTime.Month()==EOctober);
		test(dateTime.Year()==0);

		// If the date sep has been set to '.' then 10th October
		error=DateTimeParse(dateTime,_L("10.10pm"));
		test(error==KErrArgument);
		test(dateTime.Second()==0);
		test(dateTime.Minute()==0);
		test(dateTime.Hour()==0);
		test(dateTime.Day()==9);
		test(dateTime.Month()==EOctober);
		test(dateTime.Year()==0);

		error=DateTimeParse(dateTime,_L("10p 3:6:86"));
		test(error==(EParseDatePresent|EParseTimePresent));
		test(dateTime.Second()==0);
		test(dateTime.Minute()==0);
		test(dateTime.Hour()==22);
		test(dateTime.Day()==2);
		test(dateTime.Month()==EJune);
		if (currentTimeIsBeforeYear2000) 
			test(dateTime.Year()==1986);
		else
			test(dateTime.Year()==2086);

		error=DateTimeParse(dateTime,_L("1:6:12 3.05.06"));
		test(error==KErrArgument);

		error=DateTimeParse(dateTime,_L(" 3.05.06  1:6:12"));
		test(error==KErrArgument);

		locale.Refresh();
		savedLocale.Set();
		}
	}

TInt E32Main()
	{
	
	test.Title();
	test.Start(_L("Begin tests"));
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

	test.Console()->Printf(_L("SimpleTimeFormats\n"));
	SimpleTimeFormats();
	test.Console()->Printf(_L("SimpleDateFormats\n"));
	SimpleDateFormats();
	test.Console()->Printf(_L("DateTimeFormats\n"));
	DateTimeFormats();
	test.Console()->Printf(_L("TimeDateFormats\n"));
	TimeDateFormats();
	test.Console()->Printf(_L("BadDescriptors\n"));
	BadDescriptors();
	test.Console()->Printf(_L("TestLocaleChanges\n"));
	TestLocaleChanges();
	test.Console()->Printf(_L("End of tests\n"));
	currentLocale.Set();
	User::SetUTCOffset(oldOffset);
	test.End();
	return(KErrNone);
	}


