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
// e32test\buffer\tarraysp.cpp
// 
//

#include <e32test.h>
#include <e32math.h>

GLREF_D RTest test;
GLREF_C TInt Random();

LOCAL_D TInt Count=0;

GLREF_D TLinearOrder<TInt64> Int64Order;

volatile TReal Absorber;
volatile TInt DummyFlag = 1;

#undef FOREVER
#define FOREVER	while (DummyFlag)


LOCAL_C TInt VerySlowInt64Order(const TInt64& a, const TInt64& b)
	{
	TReal x;
	Math::Ln(x, 2.0);
	Absorber = x;
	if (a < b)
		return -1;
	if (a == b)
		return 0;
	return 1;
	}


LOCAL_C TInt SpeedTest1(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	pS->Signal();
	FOREVER
		{
		TInt i;
		RArray<TInt> a;
		for (i=0; i<1000; i++)
			a.Append(Count++);
		a.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt SpeedTest2(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	CArrayFixFlat<TInt>* pA=new CArrayFixFlat<TInt>(8);
	pS->Signal();
	FOREVER
		{
		TInt i;
		for (i=0; i<1000; i++)
			{
			pA->AppendL(Count++);
			}
		pA->Reset();
		}
	delete pA;
	return KErrNone;
	}

TInt Total;
LOCAL_C TInt SpeedTest3(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	TInt i;
	RArray<TInt> a;
	for (i=0; i<1000; i++)
		a.Append(i);
	pS->Signal();
	FOREVER
		{
		TInt total=0;
		for (i=0; i<1000; i++)
			{
			total+=a[i];
			Count++;
			}
		Total=total;
		}
	return KErrNone;
	}

LOCAL_C TInt SpeedTest4(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	CArrayFixFlat<TInt>* pA=new CArrayFixFlat<TInt>(8);
	TInt i;
	for (i=0; i<1000; i++)
		{
		pA->AppendL(i);
		}
	pS->Signal();
	FOREVER
		{
		TInt total=0;
		for (i=0; i<1000; i++)
			{
			total+=(*pA)[i];
			Count++;
			}
		Total=total;
		}
	delete pA;
	return KErrNone;
	}

LOCAL_C TInt SpeedTest5(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	pS->Signal();
	FOREVER
		{
		TInt i;
		RArray<TInt> a;
		for (i=999; i>=0; i--, Count++)
			a.InsertInOrder(i);
		a.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt SpeedTest5a(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	pS->Signal();
	FOREVER
		{
		TInt i;
		RArray<TInt> a;
		for (i=999; i>=0; i--, Count++)
			a.InsertInOrderAllowRepeats(0);
		a.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt SpeedTest6(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	CArrayFixFlat<TInt>* pA=new CArrayFixFlat<TInt>(8);
	TKeyArrayFix key(0,ECmpTInt);
	pS->Signal();
	FOREVER
		{
		TInt i;
		for (i=999; i>=0; i--, Count++)
			{
			pA->InsertIsqL(i,key);
			}
		pA->Reset();
		}
	delete pA;
	return KErrNone;
	}

LOCAL_C TInt SpeedTest7(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	TInt i;
	RArray<TInt> a(1024);
	for (i=0; i<1000; i++)
		a.Append(Random());
	pS->Signal();
	FOREVER
		{
		a.Sort();
		for (i=0; i<1000; i++)
			a[i]=Random();
		Count++;
		}
	return KErrNone;
	}

LOCAL_C TInt SpeedTest7b(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	TInt i;
	RArray<TInt64> a(1024);
	for (i=0; i<1000; i++)
		a.Append(MAKE_TINT64(Random(),Random()));
	pS->Signal();
	FOREVER
		{
		a.Sort(Int64Order);
		for (i=0; i<1000; i++)
			a[i]=MAKE_TINT64(Random(),Random());
		Count++;
		}
	return KErrNone;
	}

LOCAL_C TInt SpeedTest8(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	CArrayFixFlat<TInt>* pA=new CArrayFixFlat<TInt>(1024);
	TKeyArrayFix key(0,ECmpTInt);
	TInt i;
	for (i=0; i<1000; i++)
		pA->AppendL(Random());
	pS->Signal();
	FOREVER
		{
		pA->Sort(key);
		for (i=0; i<1000; i++)
			(*pA)[i]=Random();
		Count++;
		}
	delete pA;
	return KErrNone;
	}

LOCAL_C TInt SpeedTest8a(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	CArrayFixFlat<TInt64>* pA=new CArrayFixFlat<TInt64>(1024);
	TKeyArrayFix key(0,ECmpTInt64);
	TInt i;
	for (i=0; i<1000; i++)
		pA->AppendL(MAKE_TINT64(Random(),Random()));
	pS->Signal();
	FOREVER
		{
		pA->Sort(key);
		for (i=0; i<1000; i++)
			(*pA)[i]=MAKE_TINT64(Random(),Random());
		Count++;
		}
	delete pA;
	return KErrNone;
	}

LOCAL_C TInt SpeedTest9(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	pS->Signal();
	FOREVER
		{
		TInt i;
		RArray<TInt> a;
		for (i=0; i<1000; i++, Count++)
			a.InsertInOrder(Random());
		a.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt SpeedTest9b(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	pS->Signal();
	FOREVER
		{
		TInt i;
		RArray<TInt64> a;
		for (i=0; i<1000; i++, Count++)
			a.InsertInOrder(MAKE_TINT64(Random(),Random()),Int64Order);
		a.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt SpeedTest9q(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	pS->Signal();
	FOREVER
		{
		TInt i;
		RArray<TInt64> a;
		for (i=0; i<1000; i++, Count++)
			a.InsertInOrderAllowRepeats(MAKE_TINT64(0,i), &VerySlowInt64Order);
		a.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt SpeedTest9r(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	pS->Signal();
	FOREVER
		{
		TInt i;
		RArray<TInt64> a;
		for (i=0; i<1000; i++, Count++)
			a.InsertInOrderAllowRepeats(MAKE_TINT64(0,0), &VerySlowInt64Order);
		a.Close();
		}
	return KErrNone;
	}

LOCAL_C TInt SpeedTest10(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	CArrayFixFlat<TInt>* pA=new CArrayFixFlat<TInt>(8);
	TKeyArrayFix key(0,ECmpTInt);
	pS->Signal();
	FOREVER
		{
		TInt i;
		for (i=0; i<1000; i++, Count++)
			{
			pA->InsertIsqAllowDuplicatesL(Random(),key);
			}
		pA->Reset();
		}
	delete pA;
	return KErrNone;
	}

LOCAL_C TInt SpeedTest10a(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	CArrayFixFlat<TInt64>* pA=new CArrayFixFlat<TInt64>(8);
	TKeyArrayFix key(0,ECmpTInt64);
	pS->Signal();
	FOREVER
		{
		TInt i;
		for (i=0; i<1000; i++, Count++)
			{
			pA->InsertIsqAllowDuplicatesL(MAKE_TINT64(Random(),Random()),key);
			}
		pA->Reset();
		}
	delete pA;
	return KErrNone;
	}

LOCAL_C TInt SpeedTest11(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	TInt i;
	RArray<TInt> a(1024);
	for (i=0; i<1024; i++)
		a.Append(i);
	pS->Signal();
	FOREVER
		{
		for (i=0; i<1024; i++)
			{
			a.FindInOrder(i^0x2b9);
			Count++;
			}
		}
	return KErrNone;
	}

LOCAL_C TInt SpeedTest11b(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	TInt i;
	RArray<TInt64> a(1024);
	for (i=0; i<1024; i++)
		a.Append(MAKE_TINT64(i>>6,i));
	pS->Signal();
	FOREVER
		{
		for (i=0; i<1024; i++)
			{
			TInt j=i^0x2b9;
			TInt64 x = MAKE_TINT64(j>>6,j);
			a.FindInOrder(x,Int64Order);
			Count++;
			}
		}
	return KErrNone;
	}

LOCAL_C TInt SpeedTest12(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	CArrayFixFlat<TInt>* pA=new CArrayFixFlat<TInt>(8);
	TKeyArrayFix key(0,ECmpTInt);
	TInt i;
	for (i=0; i<1024; i++)
		{
		pA->AppendL(i);
		}
	pS->Signal();
	FOREVER
		{
		for (i=0; i<1024; i++)
			{
			TInt j;
			TInt k=i^0x2b9;
			pA->FindIsq(k,key,j);
			Count++;
			}
		}
	delete pA;
	return KErrNone;
	}

LOCAL_C TInt SpeedTest12a(TAny* aSem)
	{
	RSemaphore *pS=(RSemaphore*)aSem;
	CArrayFixFlat<TInt64>* pA=new CArrayFixFlat<TInt64>(8);
	TKeyArrayFix key(0,ECmpTInt64);
	TInt i;
	for (i=0; i<1024; i++)
		{
		pA->AppendL(MAKE_TINT64(i>>6,i));
		}
	pS->Signal();
	FOREVER
		{
		for (i=0; i<1024; i++)
			{
			TInt j;
			TInt k=i^0x2b9;
			TInt64 x = MAKE_TINT64(k>>6,k);
			pA->FindIsq(x,key,j);
			Count++;
			}
		}
	delete pA;
	return KErrNone;
	}

const TInt KHeapSize=16384;
LOCAL_C TInt DoSpeedTest(TThreadFunction f)
	{
	RSemaphore sem;
	sem.CreateLocal(0);
	RThread t;
	t.Create(_L("Speedy"),f,KDefaultStackSize,KHeapSize,KHeapSize,&sem);
	t.SetPriority(EPriorityLess);
	Count=0;
	TRequestStatus s;
	t.Logon(s);
	t.Resume();
	sem.Wait();
	User::After(2000000);
	TInt num=Count/2;
	t.Kill(0);
	User::WaitForRequest(s);
	if (t.ExitType()!=EExitKill)
		{
		TExitCategoryName aExitCategory = t.ExitCategory();
 		test.Printf(_L("Exit %d %S %d\n"),t.ExitType(),&aExitCategory,t.ExitReason());
		}
	CLOSE_AND_WAIT(t);
	sem.Close();
	return num;
	}

GLDEF_C void DoSpeedTests()
	{
	TInt r;
	test.Next(_L("Speed Tests"));
	r=DoSpeedTest(SpeedTest1);
	test.Printf(_L("RArray<TInt> append, %d in 1 second\n"),r);
	r=DoSpeedTest(SpeedTest2);
	test.Printf(_L("CArrayFixFlat<TInt> append, %d in 1 second\n"),r);
	r=DoSpeedTest(SpeedTest3);
	test.Printf(_L("RArray<TInt> access, %d in 1 second\n"),r);
	test(Total==999*1000/2);
	r=DoSpeedTest(SpeedTest4);
	test.Printf(_L("CArrayFixFlat<TInt> access, %d in 1 second\n"),r);
	test(Total==999*1000/2);
	r=DoSpeedTest(SpeedTest5);
	test.Printf(_L("RArray<TInt> InsertInOrder, %d in 1 second\n"),r);
	r=DoSpeedTest(SpeedTest5a);
	test.Printf(_L("RArray<TInt> InsertInOrder repeats, %d in 1 second\n"),r);
	r=DoSpeedTest(SpeedTest6);
	test.Printf(_L("CArrayFixFlat<TInt> InsertIsqL, %d in 1 second\n"),r);
	r=DoSpeedTest(SpeedTest7);
	test.Printf(_L("RArray<TInt> Sort 1000, %d in 1 second\n"),r);
	r=DoSpeedTest(SpeedTest7b);
	test.Printf(_L("RArray<TInt64> Sort 1000, %d in 1 second\n"),r);
	r=DoSpeedTest(SpeedTest8);
	test.Printf(_L("CArrayFixFlat<TInt> Sort 1000, %d in 1 second\n"),r);
	r=DoSpeedTest(SpeedTest8a);
	test.Printf(_L("CArrayFixFlat<TInt64> Sort 1000, %d in 1 second\n"),r);
	r=DoSpeedTest(SpeedTest9);
	test.Printf(_L("RArray<TInt> InsertInOrder random, %d in 1 second\n"),r);
	r=DoSpeedTest(SpeedTest9b);
	test.Printf(_L("RArray<TInt64> InsertInOrder random, %d in 1 second\n"),r);
	r=DoSpeedTest(SpeedTest9q);
	test.Printf(_L("RArray<TInt64> InsertInOrder repeat control, %d in 1 second\n"),r);
	r=DoSpeedTest(SpeedTest9r);
	test.Printf(_L("RArray<TInt64> InsertInOrder repeats, %d in 1 second\n"),r);
	r=DoSpeedTest(SpeedTest10);
	test.Printf(_L("CArrayFixFlat<TInt> InsertIsqL random, %d in 1 second\n"),r);
	r=DoSpeedTest(SpeedTest10a);
	test.Printf(_L("CArrayFixFlat<TInt64> InsertIsqL random, %d in 1 second\n"),r);
	r=DoSpeedTest(SpeedTest11);
	test.Printf(_L("RArray<TInt> FindInOrder, %d in 1 second\n"),r);
	r=DoSpeedTest(SpeedTest11b);
	test.Printf(_L("RArray<TInt64> FindInOrder, %d in 1 second\n"),r);
	r=DoSpeedTest(SpeedTest12);
	test.Printf(_L("CArrayFixFlat<TInt> FindIsqL, %d in 1 second\n"),r);
	r=DoSpeedTest(SpeedTest12a);
	test.Printf(_L("CArrayFixFlat<TInt64> FindIsqL, %d in 1 second\n"),r);
	}

