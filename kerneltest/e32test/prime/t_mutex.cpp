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
// e32test\prime\t_mutex.cpp
// Test mutexes
// 
//

#include <e32test.h>
#include "u32std.h"
#include "../misc/prbs.h"

RTest test(_L("T_MUTEX"));

TInt Count1=0;
TInt Count2=0;
TInt TId=-1;
RMutex Mutex;

void BusyWait(TInt aMicroseconds)
	{
	TTime begin;
	begin.HomeTime();
	FOREVER
		{
		TTime now;
		now.HomeTime();
		TTimeIntervalMicroSeconds iv=now.MicroSecondsFrom(begin);
		if (iv.Int64()>=TInt64(aMicroseconds))
			return;
		}
	}

TInt ThreadFunction(TAny* aPtr)
	{
	TInt id=(TInt)aPtr;
	FOREVER
		{
		Mutex.Wait();
		TId=id;
		if (Count1!=Count2)
			{
			RProcess me;
			me.Panic(_L("FAIL!"),0);
			}
		++Count1;
		BusyWait(50000);
		++Count2;
		TId=-1;
		Mutex.Signal();
		}
	}

void CreateThread(RThread& t, TInt aId)
	{
	TInt r=t.Create(KNullDesC,ThreadFunction,0x2000,NULL,(TAny*)aId);
	test(r==KErrNone);
	t.Resume();
	}


GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Create mutex"));
	TInt r=Mutex.CreateLocal();
	test(r==KErrNone);

	test.Next(_L("Create threads"));
	RThread().SetPriority(EPriorityMuchMore);
	RThread t[4];
	TInt i;
	for (i=0; i<4; ++i)
		CreateThread(t[i],i);

	TUint seed[2];
	seed[0]=0xb17217f8;
	seed[1]=0;

	FOREVER
		{
		TUint ms=Random(seed);
		TUint action=(ms>>30);
		i=(ms>>28)&3;
		ms&=63;
		ms+=15;
		User::AfterHighRes(ms*1000);
		switch(action)
			{
			case 0:
				{
				t[i].Suspend();
				break;
				}
			case 1:
				{
				t[i].Resume();
				break;
				}
			case 2:
				{
				TRequestStatus s;
				t[i].Logon(s);
				t[i].Kill(0);
				if (Count1!=Count2)
					{
					if (Count1-Count2!=1)
						{
						RProcess me;
						me.Panic(_L("FAIL!"),1);
						}
					else if (TId==i)
						++Count2;
					}
				User::AfterHighRes(50000);
				User::WaitForRequest(s);
				t[i].Close();
				CreateThread(t[i],i);
				break;
				}
			case 3:
				{
				break;
				}
			}
		}

//	test.End();
	}
