// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include <e32test.h>
#include "t_property.h"

_LIT(KTestName,"t_stress_property");

RTest test(KTestName);

const TInt32 KUidPropTestCategoryValue = 0x101f75b8;
const TUid KPropTestCategory = { KUidPropTestCategoryValue };

#define TEST_TIME 36000			//10 hours

#define TEST_ERROR(rl,rr) { if((TInt)rl!=(TInt)rr) { ExitThread(rl, rr, __LINE__); return KErrGeneral; } }

TBool volatile StopAndExit = EFalse;
TTime startTime;

LOCAL_D void ExitThread(TInt rl, TInt rr, TInt aLine)
	{
	test.Printf(_L("Test '%S' failed at line %d; Expected value=%d Actual value=%d; \n"), &KTestName, aLine, rr, rl);
	StopAndExit = ETrue;
	//delete if it's not deleted, to wake up subscribing threads waiting for events on this property
	RProperty::Delete(KPropTestCategory,0);
	}

LOCAL_D TInt LowPriorityThread1(TAny* /*aParameter*/)
	{
	RProperty prop;
	TBuf8<512> buffer;
	TInt length = 2;

	TInt r=prop.Attach(KPropTestCategory,0);
	TEST_ERROR(r,KErrNone);

	while(!StopAndExit)
		{
		buffer.SetLength(length);
		buffer[0]=(TUint8)(length%256);
		buffer[length-1]=(TUint8)((length-1)%256);
		++length;
		if(length>512)
			length=2;
		r=prop.Set(buffer);
		if(r!=KErrArgument && r!=KErrNotFound)
			{
			//if it's not of type EInt and defined
			TEST_ERROR(r,KErrNone);
			}
		User::AfterHighRes(0);
		}

	return KErrNone;
	}

LOCAL_D TInt LowPriorityThread2(TAny* /*aParameter*/)
	{
	RProperty prop;
	TBuf8<512> buffer;

	TInt r=prop.Attach(KPropTestCategory,0);
	TEST_ERROR(r,KErrNone);

	while(!StopAndExit)
		{
		r=prop.Get(buffer);
		if(r!=KErrArgument && r!=KErrNotFound)
			{
			//if it's not of type EInt and defined
			TEST_ERROR(r,KErrNone);
			TInt length=buffer.Length();
			if(length>0)
				{
				TEST_ERROR(buffer[0],length%256);
				TEST_ERROR(buffer[length-1],(length-1)%256);
				}
			}
		}
	return KErrNone;
	}

LOCAL_D TInt MediumPriorityThread(TAny* /*aParameter*/)
	{
	RProperty prop;
	TBuf8<512> buffer;

	TInt r=prop.Attach(KPropTestCategory,0);
	TEST_ERROR(r,KErrNone);

	TRequestStatus status;
	
	while(!StopAndExit)
		{
		prop.Subscribe(status);

		User::WaitForRequest(status);
		if(StopAndExit)
			break;
		if(status.Int() != KErrNotFound)
			{
			//property is defined
			TEST_ERROR(status.Int(),KErrNone);

			r=prop.Get(buffer);
			if(r!=KErrArgument)
				{
				TEST_ERROR(r,KErrNone);
				TInt length=buffer.Length();
				if(length>0)
					{
					TEST_ERROR(buffer[0],length%256);
					TEST_ERROR(buffer[length-1],(length-1)%256);
					}
				}
			}
		}

	return KErrNone;
	}

LOCAL_D TInt HighPriorityThread(TAny* /*aParameter*/)
	{

	TInt type=RProperty::EInt;
	TInt iteration=0;
	TInt r;

	while(!StopAndExit)
		{
		User::AfterHighRes(1000); //wait for 1ms
		
//		test.Printf(_L("Deleting property\r\n"));
		r=RProperty::Delete(KPropTestCategory,0);
		TEST_ERROR(r,KErrNone);

//		test.Printf(_L("Defining property\r\n"));
		r=RProperty::Define(KPropTestCategory,0,type, KPassPolicy, KPassPolicy);
		TEST_ERROR(r,KErrNone);

		type=(type+1)%RProperty::ETypeLimit;

		if(1000 == ++iteration)
			{
			//check if we should exit
			TTimeIntervalSeconds timeTaken;
			TTime time;
			time.HomeTime();
			TInt r = time.SecondsFrom(startTime, timeTaken);
			TEST_ERROR(r,KErrNone);

			if(timeTaken.Int() >= TEST_TIME)
				{
				//we should exit

				StopAndExit=ETrue;
				//delete if it's not deleted, to wake up subscribing threads waiting for events on this property
				RProperty::Delete(KPropTestCategory,0);
				break;
				}
			iteration=0;
			}
		}
	return KErrNone;
	}



GLDEF_C TInt E32Main()
	{

	test.Start(_L("Stress test using multiple threads accessing the same property"));

    startTime.HomeTime();
	TInt r=RProperty::Define(KPropTestCategory,0,RProperty::EInt, KPassPolicy, KPassPolicy);
	test(r==KErrNone);

	TRequestStatus status1;
	TRequestStatus status2;
	TRequestStatus status3;
	TRequestStatus status4;
	RThread t1;
	RThread t2;
	RThread t3;
	RThread t4;

	r = t1.Create(KNullDesC, LowPriorityThread1, 0x2000, NULL, 0);
	test(r == KErrNone);
	t1.SetPriority(EPriorityLess);
	t1.Logon(status1);

	r = t2.Create(KNullDesC, LowPriorityThread2, 0x2000, NULL, 0);
	test(r == KErrNone);
	t2.SetPriority(EPriorityLess);
	t2.Logon(status2);

	r = t3.Create(KNullDesC, MediumPriorityThread, 0x2000, NULL, 0);
	test(r == KErrNone);
	t3.SetPriority(EPriorityNormal);
	t3.Logon(status3);
	
	r = t4.Create(KNullDesC, HighPriorityThread, 0x2000, NULL, 0);
	test(r == KErrNone);
	t4.SetPriority(EPriorityMore);
	t4.Logon(status4);
	
	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);

	t1.Resume();
	t2.Resume();
	t3.Resume();
	t4.Resume();

	User::WaitForRequest(status1);
	User::WaitForRequest(status2);
	User::WaitForRequest(status3);
	User::WaitForRequest(status4);

	User::SetJustInTime(jit);

	test(status1 == KErrNone);
	test(status2 == KErrNone);
	test(status3 == KErrNone);
	test(status4 == KErrNone);

	TTimeIntervalSeconds timeTaken;
	TTime time;
	time.HomeTime();
	r = time.SecondsFrom(startTime, timeTaken);
	test(r==KErrNone);
	TInt totalTime = timeTaken.Int();
	
	TInt seconds = totalTime % 60;
    TInt minutes = (totalTime / 60) % 60;
    TInt hours   = totalTime / 3600;

    test.Printf(_L("Time taken since test started: %d:%d:%d\r\n"), 
                   hours, minutes, seconds);

	CLOSE_AND_WAIT(t1);
	CLOSE_AND_WAIT(t2);
	CLOSE_AND_WAIT(t3);
	CLOSE_AND_WAIT(t4);

	test.End();

	return KErrNone;
	}
