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
// e32test\locale\t_names.cpp
// Overview:
// Test Date, Time and Currency Symbol locale settings.
// API Information:
// TDayName,TDayNameAbb,TMonthName, TMonthNameAbb, TDateSuffix, TAmPmName,
// TCurrencySymbol.
// Details:
// - Construct and set the full, abbreviated text name for a day of the week,
// month and check it is as specified. 
// - Construct and set date suffix text for a specific day in the month and 
// test that constructor, TDateSuffix::Set panics when invalid parameters 
// are passed.
// - Construct and assign current locale's text identifying time before noon,
// after noon and check it is as expected.
// - Assign the current locale's currency symbol with different text and check 
// it is as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

#ifdef __VC32__
    // Solve compilation problem caused by non-English locale
    #pragma setlocale("english")
#endif

LOCAL_D RTest test(_L("T_NAMES"));

void TestDayName()
	{
	test.Start(_L("TDayName"));
	TDayName name1;
	test(name1.Compare(_L("Monday"))==KErrNone);
	TDayName name2(ESunday);
	test(name2.Compare(_L("Sunday"))==KErrNone);
	name1.Set(EWednesday);
	test(name1.Compare(_L("Wednesday"))==KErrNone);
	}

void TestDayNameAbb()
	{
	test.Next(_L("TDayNameAbb"));
	TDayNameAbb name1;
	test(name1.Compare(_L("Mon"))==KErrNone);
	TDayNameAbb name2(ETuesday);
	test(name2.Compare(_L("Tue"))==KErrNone);
	name1.Set(ESaturday);
	test(name1.Compare(_L("Sat"))==KErrNone);
	}

void TestMonthName()
	{
	test.Next(_L("TMonthName"));
	TMonthName name1;
	test(name1.Compare(_L("January"))==KErrNone);
	TMonthName name2(EDecember);
	test(name2.Compare(_L("December"))==KErrNone);
	name1.Set(EMarch);
	test(name1.Compare(_L("March"))==KErrNone);
	}

void TestMonthNameAbb()
	{
	test.Next(_L("TMonthNameAbb"));
	TMonthNameAbb name1;
	test(name1.Compare(_L("Jan"))==KErrNone);
	TMonthNameAbb name2(EFebruary);
	test(name2.Compare(_L("Feb"))==KErrNone);
	name1.Set(ENovember);
	test(name1.Compare(_L("Nov"))==KErrNone);
	}

TInt TestThread1(TAny* Ptr)
//
// Used in TestDateSuffix() to test the constructor panics when silly parameters are passed
//
	{
	TDateSuffix ds((TInt) Ptr);
	(void)ds;
	return(KErrNone);
	}

TInt TestThread2(TAny* Ptr)
//
// Used in TestDateSuffix() to test TDateSuffix::Set panics when silly parameters are passed
//
	{
	TDateSuffix suff(0);
	suff.Set((TInt) Ptr);
	return(KErrNone);
	}

void TestDateSuffix()
	{
	test.Next(_L("TDateSuffix"));
	
	test.Start(_L("Simple creation and assignment"));
	TDateSuffix suff1;
	test(suff1.Compare(_L("st"))==KErrNone);
	TDateSuffix suff2(1);
	test(suff2.Compare(_L("nd"))==KErrNone);
	suff1.Set(2);
	test(suff1.Compare(_L("rd"))==KErrNone);
	
	test.Next(_L("Constructor in a thread"));
	RThread thread;
	TRequestStatus stat;
	thread.Create(_L("Test Thread"),TestThread1,KDefaultStackSize,0x100,0x200,(TAny*)0);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitType()==EExitKill);
	CLOSE_AND_WAIT(thread);
	
	test.Next(_L("Constructor panics for -1"));
	thread.Create(_L("Test Thread"),TestThread1,KDefaultStackSize,0x100,0x200,(TAny*)-1);
	thread.Logon(stat);
	// don't want just in time debugging as we trap panics
	TBool justInTime=User::JustInTime(); 
	User::SetJustInTime(EFalse); 
	thread.Resume();
	User::WaitForRequest(stat);
	User::SetJustInTime(justInTime); 
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Constructor panics for KMaxSuffices"));
	thread.Create(_L("Test Thread"),TestThread1,KDefaultStackSize,0x100,0x200,(TAny*)KMaxSuffixes);
	thread.Logon(stat);
	User::SetJustInTime(EFalse); 
	thread.Resume();
	User::WaitForRequest(stat);
	User::SetJustInTime(justInTime); 
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
	
	test.Next(_L("Set in a thread"));
	thread.Create(_L("Test Thread"),TestThread2,KDefaultStackSize,0x100,0x200,(TAny*)0);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitType()==EExitKill);
	CLOSE_AND_WAIT(thread);
	
	test.Next(_L("Set panics for -1"));
	thread.Create(_L("Test Thread"),TestThread2,KDefaultStackSize,0x100,0x200,(TAny*)-1);
	thread.Logon(stat);
	User::SetJustInTime(EFalse); 
	thread.Resume();
	User::WaitForRequest(stat);
	User::SetJustInTime(justInTime); 
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Set panics for KMaxSuffices"));
	thread.Create(_L("Test Thread"),TestThread2,KDefaultStackSize,0x100,0x200,(TAny*)KMaxSuffixes);
	thread.Logon(stat);						  
	User::SetJustInTime(EFalse); 
	thread.Resume();
	User::WaitForRequest(stat);
	User::SetJustInTime(justInTime); 
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);

	test.End();
	}

void TestAmPmName()
	{
	test.Next(_L("TAmPmName"));
	TAmPmName name1;
	test(name1.Compare(_L("am"))==KErrNone);
	TAmPmName name2(EPm);
	test(name2.Compare(_L("pm"))==KErrNone);
	name1.Set(EPm);
	test(name1.Compare(_L("pm"))==KErrNone);
	}

void TestCurrencySymbol()
	{
	test.Next(_L("TCurrencySymbol"));
	TCurrencySymbol name;
	test(name.Compare(_L("\xA3"))==KErrNone);
	name[0]=0;
	name.SetLength(0);
	test(name.Compare(_L("\xA3"))!=KErrNone);
	name.Set();
	test(name.Compare(_L("\xA3"))==KErrNone);
	name.Copy(_L("Syphilis"));
	test(name.Compare(_L("\xA3"))!=KErrNone);
	test(name.Compare(_L("Syphilis"))==KErrNone);
	User::SetCurrencySymbol(name);
	TCurrencySymbol name2;
	test(name2.Compare(_L("Syphilis"))==KErrNone);
	name2.Copy(_L("\xA3"));
	User::SetCurrencySymbol(name2);
	name.Set();
	test(name.Compare(_L("\xA3"))==KErrNone);
	}

GLDEF_C TInt E32Main()
	{

	test.Title();
	TestDayName();
	TestDayNameAbb();
	TestMonthName();
	TestMonthNameAbb();
	TestDateSuffix();
	TestAmPmName();
	TestCurrencySymbol();
	test.End();

	return(KErrNone);
	}
