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
// f32test\server\t_fsy2k.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include "t_server.h"
#include <e32hal.h>
#include <e32math.h>
#include <f32dbg.h>

GLDEF_D RTest test(_L("T_FSY2K"));
GLDEF_D TBuf<128> gDateBuf;

const TInt KMaxValidDateTimes=25;
const TInt KMaxInvalidDateTimes=7;

static void testRFsSetEntry(TDateTime* aDateTime, TTime* aTime, TBool validDate)
//
// Test RFs::SetEntry() and RFs::Entry() functions on both a file and a directory
//
	{	
	MakeFile(_L("Y2KTEST.tst"));
	
	TInt r=TheFs.SetEntry(_L("Y2KTEST.tst"),*aTime,KEntryAttHidden,KEntryAttArchive);
	test_KErrNone(r);
	
	TEntry entry;
	r=TheFs.Entry(_L("Y2KTEST.tst"),entry);
	test_KErrNone(r);

	TDateTime checkDateTime=(entry.iModified).DateTime();
	test(checkDateTime.Year()==aDateTime->Year());
	if (validDate)
		{
		test(checkDateTime.Month()==aDateTime->Month());
		test(checkDateTime.Day()==aDateTime->Day());
		}
	else
		{
		test(checkDateTime.Month()==aDateTime->Month()+1);
		test(checkDateTime.Day()==0);
		}
	
	(entry.iModified).FormatL(gDateBuf,_L("%*D%X%N%Y %1 %2 %3"));
	test.Printf(_L("Valid date: %S\n"),&gDateBuf);
		
	r=TheFs.Delete(_L("Y2KTEST.tst"));
	test_KErrNone(r);

	MakeDir(_L("\\Y2KTEST\\"));
	r=TheFs.SetEntry(_L("\\Y2KTEST\\"),*aTime,KEntryAttHidden,KEntryAttArchive);
	test_KErrNone(r);

	r=TheFs.Entry(_L("\\Y2KTEST\\"),entry);
	test_KErrNone(r);
	
	checkDateTime=(entry.iModified).DateTime();
	test(checkDateTime.Year()==aDateTime->Year());
	if (validDate)
		{
		test(checkDateTime.Month()==aDateTime->Month());
		test(checkDateTime.Day()==aDateTime->Day());
		}
	else
		{
		test(checkDateTime.Month()==aDateTime->Month()+1);
		test(checkDateTime.Day()==0);
		}
	
	(entry.iModified).FormatL(gDateBuf,_L("%*D%X%N%Y %1 %2 %3"));
	test.Printf(_L("Valid date: %S\n"),&gDateBuf);

	r=TheFs.RmDir(_L("\\Y2KTEST\\"));
	test_KErrNone(r);
	
	}


static void testRFsSetModified(TDateTime* aDateTime, TTime* aTime, TBool validDate)
//
// Test RFs::SetModified() and RFs::Modified() functions on both a file and a directory
//
	{
	MakeFile(_L("Y2KTEST.tst"));
	
	TInt r=TheFs.SetModified(_L("Y2KTEST.tst"),*aTime);
	test_KErrNone(r);
	
	TTime check;
	r=TheFs.Modified(_L("Y2KTEST.tst"),check);
	test_KErrNone(r);
		
	TDateTime checkDateTime=check.DateTime();
	
	test(checkDateTime.Year()==aDateTime->Year());
	if (validDate)
		{
		test(checkDateTime.Month()==aDateTime->Month());
		test(checkDateTime.Day()==aDateTime->Day());
		}
	else
		{
		test(checkDateTime.Month()==aDateTime->Month()+1);
		test(checkDateTime.Day()==0);
		}
	
	
	check.FormatL(gDateBuf,_L("%*D%X%N%Y %1 %2 %3"));
	test.Printf(_L("Valid date: %S\n"),&gDateBuf);
		
	r=TheFs.Delete(_L("Y2KTEST.tst"));
	test_KErrNone(r);

	MakeDir(_L("\\Y2KTEST\\"));
	r=TheFs.SetModified(_L("\\Y2KTEST\\"),*aTime);
	test_KErrNone(r);

	r=TheFs.Modified(_L("\\Y2KTEST\\"),check);
	test_KErrNone(r);
	
	checkDateTime=check.DateTime();
	test(checkDateTime.Year()==aDateTime->Year());
	if (validDate)	
		{
		test(checkDateTime.Month()==aDateTime->Month());
		test(checkDateTime.Day()==aDateTime->Day());
		}
	else
		{
		test(checkDateTime.Month()==aDateTime->Month()+1);
		test(checkDateTime.Day()==0);
		}

	check.FormatL(gDateBuf,_L("%*D%X%N%Y %1 %2 %3"));
	test.Printf(_L("Valid date: %S\n"),&gDateBuf);

	r=TheFs.RmDir(_L("\\Y2KTEST\\"));
	test_KErrNone(r);
	}

	
static void testRFileSet(TDateTime* aDateTime, TTime* aTime, TBool validDate)
//
// Test RFile::Set() and RFile::Modified()
//
	{
	RFile file;
	TInt r=file.Replace(TheFs,_L("Y2KTEST.tst"),0);
	test_Value(r, r == KErrNone || r==KErrPathNotFound);	
		
	r=file.Set(*aTime,KEntryAttHidden,KEntryAttNormal);
	test_KErrNone(r);
	file.Close();
	
	TTime check;
	file.Open(TheFs,_L("Y2KTEST.tst"),EFileWrite);
	r=file.Modified(check);
	test_KErrNone(r);
	file.Close();
		
	test.Printf(_L("Date set to "));
	test.Printf(_L("Day %d "),aDateTime->Day());
	test.Printf(_L("Month %d "),aDateTime->Month());
	test.Printf(_L("Year %d \n"),aDateTime->Year());
	
	TDateTime checkDateTime=check.DateTime();
	if (checkDateTime.Year()!=aDateTime->Year())
		{
	//	Failure occurs for F32 releases before 111 - due to a problem
	//	in SFILE/SF_FILE DoFsFileSet() in which the TTime parameter
	//	was being incorrectly obtained.  Not a year 2000 issue.	
		
		test.Printf(_L("ERROR!\n"));
		test.Printf(_L("Actually set to "));
		test.Printf(_L("Day %d "),checkDateTime.Day());
		test.Printf(_L("Month %d "),checkDateTime.Month());
		test.Printf(_L("Year %d \n"),checkDateTime.Year());
	//	test.Printf(_L("Press any key to continue \n"));
	//	test.Getch();
		r=TheFs.Delete(_L("Y2KTEST.tst"));
		return;
		}
	else
		test(checkDateTime.Year()==aDateTime->Year());
	if (validDate)
		{
		test(checkDateTime.Month()==aDateTime->Month());
		test(checkDateTime.Day()==aDateTime->Day());
		}
	else
		{
		test(checkDateTime.Month()==aDateTime->Month()+1);
		test(checkDateTime.Day()==0);
		}

	
	check.FormatL(gDateBuf,_L("%*D%X%N%Y %1 %2 %3"));
	test.Printf(_L("Valid date: %S\n"),&gDateBuf);

	r=TheFs.Delete(_L("Y2KTEST.tst"));
	
	}	


static void testRFileSetModified(TDateTime* aDateTime, TTime* aTime, TBool validDate)
//
// Test RFile::SetModified() and RFile::Modified()
//
	{
	RFile file;
	TInt r=file.Replace(TheFs,_L("Y2KTEST.tst"),0);
	test_Value(r, r == KErrNone || r==KErrPathNotFound);	

	r=file.SetModified(*aTime);
	test_KErrNone(r);
	file.Close();
	
	TTime check;
	file.Open(TheFs,_L("Y2KTEST.tst"),EFileWrite);
	r=file.Modified(check);
	test_KErrNone(r);
	file.Close();
		
	TDateTime checkDateTime=check.DateTime();
	
	test(checkDateTime.Year()==aDateTime->Year());
	if (validDate)
		{
		test(checkDateTime.Month()==aDateTime->Month());
		test(checkDateTime.Day()==aDateTime->Day());
		}
	else
		{
		test(checkDateTime.Month()==aDateTime->Month()+1);
		test(checkDateTime.Day()==0);
		}

	check.FormatL(gDateBuf,_L("%*D%X%N%Y %1 %2 %3"));
	test.Printf(_L("Valid date: %S\n"),&gDateBuf);

	r=TheFs.Delete(_L("Y2KTEST.tst"));
	
	}	

static void testCFileManAttribsL(TDateTime* aDateTime, TTime* aTime, TBool validDate)
//
// Test CFileMan::Attribs()
//
	{
	MakeFile(_L("Y2KTEST.tst"));
	
	CFileMan* fileMan=CFileMan::NewL(TheFs);
		
	TInt r=fileMan->Attribs(_L("Y2KTEST.tst"),KEntryAttHidden,KEntryAttNormal,*aTime);
	test_KErrNone(r);
		
	TEntry entry;
	r=TheFs.Entry(_L("Y2KTEST.tst"),entry);
	test_KErrNone(r);
	
	TTime check=entry.iModified;	
	TDateTime checkDateTime=check.DateTime();
	
	test(checkDateTime.Year()==aDateTime->Year());
	if (validDate)
		{
		test(checkDateTime.Month()==aDateTime->Month());
		test(checkDateTime.Day()==aDateTime->Day());
		}
	else
		{
		test(checkDateTime.Month()==aDateTime->Month()+1);
		test(checkDateTime.Day()==0);
		}

	
	check.FormatL(gDateBuf,_L("%*D%X%N%Y %1 %2 %3"));
	test.Printf(_L("Valid date: %S\n"),&gDateBuf);

	r=TheFs.Delete(_L("Y2KTEST.tst"));
	test_KErrNone(r);

	delete fileMan;
	}

//	Valid dates to be tested for year 2000 compliance
//	Times are always set at 11.11.00

LOCAL_D TInt testYearValid[KMaxValidDateTimes] =
	{
	1998,	//	December	31	1998
	1999,	//	January		01	1999
	1999,	//	February	27	1999
	1999,	//	February	28	1999
	1999,	//	March		01	1999
	1999,	//	August		31	1999
	1999,	//	September	01	1999
	1999,	//	September	08	1999
	1999,	//	September	09	1999
	1999,	//	September	09	1999
	1999,	//	December	31	1999
	2000,	//	January		01	2000
	2000,	//	February	27	2000
	2000,	//	February	28	2000
	2000,	//	February	29	2000
	2000,	//	March		01	2000
	2000,	//	December	31	2000
	2001,	//	January		01	2001
	2001,	//	February	28	2001
	2001,	//	March		01	2001
	2004,	//	February	28	2004
	2004,	//	February	29	2004
	2004,	//	March		01	2004
	2098,	//	January		01	2098
	2099	//	January		01	2099
	};

LOCAL_D TMonth testMonthValid[KMaxValidDateTimes] =
	{
	EDecember,	//	December	31	1998
	EJanuary,	//	January		01	1999
	EFebruary,	//	February	27	1999
	EFebruary,	//	February	28	1999
	EMarch,		//	March		01	1999
	EAugust,	//	August		31	1999
	ESeptember,	//	September	01	1999
	ESeptember,	//	September	08	1999
	ESeptember,	//	September	09	1999
	ESeptember,	//	September	10	1999
	EDecember,	//	December	31	1999
	EJanuary,	//	January		01	2000
	EFebruary,	//	February	27	2000
	EFebruary,	//	February	28	2000
	EFebruary,	//	February	29	2000
	EMarch,		//	March		01	2000
	EDecember,	//	December	31	2000
	EJanuary,	//	January		01	2001
	EFebruary,	//	February	28	2001
	EMarch,		//	March		01	2001
	EFebruary,	//	February	28	2004
	EFebruary,	//	February	29	2004
	EMarch,		//	March		01	2004
	EJanuary,	//	January		01	2098
	EJanuary	//	January		01	2099
	};

LOCAL_D TInt testDayValid[KMaxValidDateTimes] =
	{
	30,	//	December	31	1998
	0,	//	January		01	1999
	26,	//	February	27	1999
	27,	//	February	28	1999
	0,	//	March		01	1999
	30,	//	August		31	1999
	0,	//	September	01	1999
	7,	//	September	08	1999
	8,	//	September	09	1999
	8,	//	September	09	1999
	30,	//	December	31	1999
	0,	//	January		01	2000
	26,	//	February	27	2000
	27,	//	February	28	2000
	28,	//	February	29	2000
	0,	//	March		01	2000
	30,	//	December	31	2000
	0,	//	January		01	2001
	27,	//	February	28	2001
	0,	//	March		01	2001
	27,	//	February	28	2004
	28,	//	February	29	2004
	0,	//	March		01	2004
	0,	//	January		01	2098
	0	//	January		01	2099
	};


//	Invalid dates to be tested for year 2000 compliance
//	Times are always set at 11.11.00

LOCAL_D TInt testYearInvalid[KMaxInvalidDateTimes] =
	{
	1998,	//	April		31	1998
	1999,	//	February	29	1999
	2000,	//	February	30	2000
	2001,	//	February	29	2001
	2002,	//	February	29	2002
	2003,	//	February	29	2003
	2004,	//	February	30	2004
	};

LOCAL_D TMonth testMonthInvalid[KMaxValidDateTimes] =
	{
	EApril,		//	April		31	1998
	EFebruary,	//	February	29	1999
	EFebruary,	//	February	30	2000
	EFebruary,	//	February	29	2001
	EFebruary,	//	February	29	2002
	EFebruary,	//	February	29	2003
	EFebruary,	//	February	30	2004
	};

LOCAL_D TInt testDayInvalid[KMaxValidDateTimes] =
	{
	30,	//	April		31	1998
	28,	//	February	29	1999
	29,	//	February	30	2000
	28,	//	February	29	2001
	28,	//	February	29	2002
	28,	//	February	29	2003
	29,	//	February	30	2004
	};

LOCAL_D TPtrC invalidDates[KMaxValidDateTimes] =
	{
	_L("31st April 1998"),
	_L("29th February 1999"),
	_L("30th February 2000"),
	_L("29th February 2001"),
	_L("29th February 2002"),
	_L("29th February 2003"),
	_L("30th February 2004"),
	};


static void TestValidDates(TDateTime* aDateTime, TTime* aTime)
//
//	Test an array of valid dates for year 2000 compliance
//
	{
	test.Next(_L("Test RFs::SetEntry() and RFs::Entry()"));
	
	TDateTime* tempDateTime=aDateTime;
	TTime* tempTime=aTime;
	TInt i=0;

	for (;i<KMaxValidDateTimes;i++)
		{
		testRFsSetEntry(tempDateTime, tempTime, ETrue);
		tempDateTime++;
		tempTime++;
		}

	test.Next(_L("Test RFs::SetModified() and RFs::Modified()"));
	
	tempDateTime=aDateTime;
	tempTime=aTime;
	for (i=0;i<KMaxValidDateTimes;i++)
		{
		testRFsSetModified(tempDateTime, tempTime, ETrue);	
		tempDateTime++;
		tempTime++;
		}	
	
	test.Next(_L("Test RFile::Set() and RFile::Modified()"));
	
	tempDateTime=aDateTime;
	tempTime=aTime;
	for (i=0;i<KMaxValidDateTimes;i++)
		{
		testRFileSet(tempDateTime, tempTime, ETrue);	
		tempDateTime++;
		tempTime++;
		}

	test.Next(_L("Test RFile::SetModified() and RFile::Modified()"));
	
	tempDateTime=aDateTime;
	tempTime=aTime;
	for (i=0;i<KMaxValidDateTimes;i++)
		{
		testRFileSetModified(tempDateTime, tempTime, ETrue);	
		tempDateTime++;
		tempTime++;
		}

	test.Next(_L("Test CFileMan::Attribs()"));
	tempDateTime=aDateTime;
	tempTime=aTime;
	for (i=0;i<KMaxValidDateTimes;i++)
		{
		TRAPD(error,testCFileManAttribsL(tempDateTime, tempTime, ETrue));
		test_KErrNone(error);
		tempDateTime++;
		tempTime++;
		}
	}


static void TestInvalidDates(TDateTime* aDateTime, TTime* aTime)
//
//	Test an array of invalid dates for year 2000 compliance
//	aDateTime is set as one day less than the desired invalid
//	date, because TDateTime correctly prevents us setting invalid dates
//	aTime is set to TDateTime + 1 day, which should be the next
//	correct date after TDateTime, NOT the desired invalid date 
//
	{
	test.Next(_L("Test RFs::SetEntry() and RFs::Entry()"));
	
	TDateTime* tempDateTime=aDateTime;
	TTime* tempTime=aTime;
	TInt i=0;

	for (;i<KMaxInvalidDateTimes;i++)
		{
		test.Printf(_L("Invalid date: %S\n"),&invalidDates[i]);
		testRFsSetEntry(tempDateTime, tempTime, EFalse);
		tempDateTime++;
		tempTime++;
		}

	test.Next(_L("Test RFs::SetModified() and RFs::Modified()"));
	
	tempDateTime=aDateTime;
	tempTime=aTime;
	for (i=0;i<KMaxInvalidDateTimes;i++)
		{
		test.Printf(_L("Invalid date: %S\n"),&invalidDates[i]);
		testRFsSetModified(tempDateTime, tempTime, EFalse);	
		tempDateTime++;
		tempTime++;
		}	
	
	test.Next(_L("Test RFile::Set() and RFile::Modified()"));
	
	tempDateTime=aDateTime;
	tempTime=aTime;
	for (i=0;i<KMaxInvalidDateTimes;i++)
		{
		test.Printf(_L("Invalid date: %S\n"),&invalidDates[i]);
		testRFileSet(tempDateTime, tempTime, EFalse);	
		tempDateTime++;
		tempTime++;
		}

	test.Next(_L("Test RFile::SetModified() and RFile::Modified()"));
	
	tempDateTime=aDateTime;
	tempTime=aTime;
	for (i=0;i<KMaxInvalidDateTimes;i++)
		{
		test.Printf(_L("Invalid date: %S\n"),&invalidDates[i]);
		testRFileSetModified(tempDateTime, tempTime, EFalse);	
		tempDateTime++;
		tempTime++;
		}
	
	test.Next(_L("Test CFileMan::Attribs()"));
	tempDateTime=aDateTime;
	tempTime=aTime;
	for (i=0;i<KMaxInvalidDateTimes;i++)
		{
		test.Printf(_L("Invalid date: %S\n"),&invalidDates[i]);
		TRAPD(error,testCFileManAttribsL(tempDateTime, tempTime, EFalse));	
		test_KErrNone(error);
		tempDateTime++;
		tempTime++;
		}
	}


static void CallTests()
//
// Do tests relative to session path
//
	{
	
//	Set up an array of valid TDateTimes and TTimes to be tested for Y2K compliance

	TDateTime validDateTime[KMaxValidDateTimes];
	TTime validTime[KMaxValidDateTimes];
	TInt r;
	TInt i=0;

	for (;i<KMaxValidDateTimes;i++)
		{
	//	Dummy time is used to initialise validDateTime[i] before calling SetX()
		r=validDateTime[i].Set(1998,EJune,23,11,11,11,0);
		test_KErrNone(r);
		r=validDateTime[i].SetYear(testYearValid[i]);
		test_KErrNone(r);
		r=validDateTime[i].SetMonth(testMonthValid[i]);
		test_KErrNone(r);
		r=validDateTime[i].SetDay(testDayValid[i]);
		test_KErrNone(r);
		validTime[i]=validDateTime[i];
		}

//	Set up an array of invalid TDateTimes and TTimes to be tested for Y2K compliance

	TDateTime invalidDateTime[KMaxInvalidDateTimes];
	TTime invalidTime[KMaxInvalidDateTimes];
	TTimeIntervalDays extraDay(1);

	for (i=0;i<KMaxInvalidDateTimes;i++)
		{
	//	Dummy time is used to initialise validDateTime[i] before calling SetX()
		r=invalidDateTime[i].Set(1998,EJune,22,11,11,11,0);
		test_KErrNone(r);
		r=invalidDateTime[i].SetYear(testYearInvalid[i]);
		test_KErrNone(r);
		r=invalidDateTime[i].SetMonth(testMonthInvalid[i]);
		test_KErrNone(r);
		r=invalidDateTime[i].SetDay(testDayInvalid[i]);
		test_Value(r, r == KErrGeneral);		//	This will fail because it is an invalid date
		r=invalidDateTime[i].SetDay(testDayInvalid[i]-1);
		test_KErrNone(r);			//	Set it one day less 
		invalidTime[i]=invalidDateTime[i];
		invalidTime[i]+=extraDay;	//	Add on an extra day.  This should bump the
		}							//	date onto the next month, NOT set the day
									//	to the invalid date in invalidDateTime[i]
										
	TestValidDates(&validDateTime[0],&validTime[0]);
	TestInvalidDates(&invalidDateTime[0],&invalidTime[0]);		
	}

GLDEF_C void CallTestsL(void)
//
// Do testing on aDrive
//
	{

	TInt r=TheFs.MkDirAll(_L("\\F32-TST\\YEAR 2000 TESTS\\"));
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	TRAP(r,CallTests());
	if (r==KErrNone)
		TheFs.ResourceCountMarkEnd();
	else
		{
		test.Printf(_L("Error: Leave %d\n"),r);
		test(0);
		}
	r=TheFs.RmDir(_L("\\F32-TST\\YEAR 2000 TESTS\\"));
	test_KErrNone(r);
	}
