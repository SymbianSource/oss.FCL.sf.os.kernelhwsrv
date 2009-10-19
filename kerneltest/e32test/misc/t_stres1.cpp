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
// e32test\misc\t_stres1.cpp
// 
//

#include <e32test.h>
#include "u32std.h"
#include <e32atomics.h>
#include "../misc/prbs.h"

RTest test(_L("T_STRES1"));

_LIT(KLibraryFileName,"STRES1");

LOCAL_D RMutex Mutex;
LOCAL_D RSemaphore Semaphore;
LOCAL_D RChunk Chunk;
LOCAL_D RLibrary Library;

LOCAL_D TBool CheckerHasRun=EFalse;
LOCAL_D TInt TotalChecks=0;
LOCAL_D TInt CheckerBumped=0;
LOCAL_D TInt HeapUserIterations=0;
LOCAL_D TInt CreatorIterations=0;
LOCAL_D RSemaphore KillerSem;

LOCAL_C TInt HeapChecker(TAny*)
	{
	TInt n=0;
	FOREVER
		{
		User::Allocator().Check();
		TotalChecks++;
		CheckerHasRun=ETrue;
		if (++n==4)
			{
			n=0;
			User::CompressAllHeaps();
			}
		RThread().SetPriority(EPriorityMuchLess);
		}
	}

LOCAL_C TAny* HeapAlloc(TUint* aSeed)
	{
	TInt size=(Random(aSeed)&0x3ffc)+8;
	TUint32* p=(TUint32*)User::Alloc(size);
	if (p)
		{
		size=User::Allocator().AllocLen(p);
		p[0]=aSeed[0];
		p[1]=aSeed[1];
		TInt i;
		for (i=2; i<(size>>2); i++)
			p[i]=Random(aSeed);
		}
	return p;
	}

LOCAL_C void HeapFree(TAny* aPtr)
	{
	if (!aPtr)
		return;
	TInt size=User::Allocator().AllocLen(aPtr);
	TUint32* p=(TUint32*)aPtr;
	TUint seed[2];
	seed[0]=p[0];
	seed[1]=p[1];
	TInt i;
	for (i=2; i<(size>>2); i++)
		{
		if (p[i]!=Random(seed))
			User::Panic(_L("VERIFY"),i);
		}
	User::Free(p);
	}

LOCAL_C TInt HeapUser(TAny* aSeed)
	{
	TUint seed[2];
	seed[0]=(TUint)aSeed;
	seed[1]=0;
	FOREVER
		{
		TAny* p0=HeapAlloc(seed);
		TAny* p1=HeapAlloc(seed);
		TAny* p2=HeapAlloc(seed);
		HeapFree(p1);
		TAny* p3=HeapAlloc(seed);
		TAny* p4=HeapAlloc(seed);
		HeapFree(p4);
		HeapFree(p0);
		TAny* p5=HeapAlloc(seed);
		HeapFree(p2);
		TAny* p6=HeapAlloc(seed);
		TAny* p7=HeapAlloc(seed);
		HeapFree(p3);
		HeapFree(p7);
		HeapFree(p5);
		HeapFree(p6);
		__e32_atomic_add_ord32(&HeapUserIterations, 1);
		TInt ms=Random(seed)&63;
		TTimeIntervalMicroSeconds32 wait(1000*ms);
		User::AfterHighRes(wait);
		}
	}

LOCAL_C TInt KernelObjectCreator(TAny* aSeed)
	{
	TUint seed[2];
	seed[0]=(TUint)aSeed;
	seed[1]=0;
	FOREVER
		{
		TInt rm=Mutex.CreateLocal(EOwnerThread);
		TInt rs=Semaphore.CreateLocal(0,EOwnerThread);
		TInt rc=Chunk.CreateLocal(0x10000,0x100000,EOwnerThread);
		TInt rl=Library.Load(KLibraryFileName);
		if (rm==KErrNone)
			{
			Mutex.Close();
			}
		if (rs==KErrNone)
			{
			Semaphore.Close();
			}
		if (rc==KErrNone)
			{
			Chunk.Close();
			}
		if (rl==KErrNone)
			{
			Library.Close();
			}
		CreatorIterations++;
		TInt ms=Random(seed)&63;
		TTimeIntervalMicroSeconds32 wait(1000*ms);
		User::AfterHighRes(wait);
		}
	}

LOCAL_C TInt KillerThread(TAny* aSeed)
	{
	TUint seed[2];
	seed[0]=(TUint)aSeed;
	seed[1]=0;
	FOREVER
		{
		KillerSem.Wait();
		TInt ms=Random(seed)&127;
		ms+=32;
		TTimeIntervalMicroSeconds32 wait(1000*ms);
		User::AfterHighRes(wait);
		RThread t;
		TFindThread ft(_L("*Creator"));
		TFullName fn;
		TInt r=ft.Next(fn);
		if (r!=KErrNone)
			User::Panic(_L("FindErr"),r);
		r=t.Open(ft);
		if (r!=KErrNone)
			User::Panic(_L("OpenErr"),r);
		t.Kill(1);
		t.Close();
		}
	}

LOCAL_C TInt StartThread(const TDesC& aName, TThreadFunction aFunction, TAny* aPtr, TThreadPriority aPriority, RThread* aThread)
	{
	RThread t;
	TInt r=t.Create(aName,aFunction,0x1000,NULL,aPtr);
	if (r!=KErrNone)
		return r;
	t.SetPriority(aPriority);
	t.Resume();
	if (aThread)
		aThread->SetHandle(t.Handle());
	else
		t.Close();
	return r;
	}

LOCAL_C void Initialise(RThread& aChecker)
	{
	TInt r=StartThread(_L("HeapChecker"),HeapChecker,NULL,EPriorityMuchLess,&aChecker);
	test(r==KErrNone);
	r=StartThread(_L("HeapUser1"),HeapUser,(TAny*)0xb504f333,EPriorityNormal,NULL);
	test(r==KErrNone);
	r=StartThread(_L("HeapUser2"),HeapUser,(TAny*)0xddb3d743,EPriorityNormal,NULL);
	test(r==KErrNone);
	r=StartThread(_L("Creator"),KernelObjectCreator,(TAny*)0xadf85458,EPriorityNormal,NULL);
	test(r==KErrNone);
	r=KillerSem.CreateLocal(1);
	test(r==KErrNone);
	r=StartThread(_L("Killer"),KillerThread,(TAny*)0xb17217f8,EPriorityMore,NULL);
	test(r==KErrNone);
	}

LOCAL_C void Restart(const TDesC&)
	{
	FOREVER
		{
		TInt r=StartThread(_L("Creator"),KernelObjectCreator,(TAny*)0xadf85458,EPriorityNormal,NULL);
		if (r==KErrNone)
			break;
		User::After(15000);
		}
	KillerSem.Signal();
	}

GLDEF_C TInt E32Main()
	{
	test.Title();

	RThread().SetPriority(EPriorityMuchMore);
	RThread checker;

	test.Start(_L("Initialise"));
	Initialise(checker);

	test.Next(_L("Create undertaker"));
	RUndertaker u;
	TInt r=u.Create();
	test(r==KErrNone);
	test.Next(_L("Create timer"));
	RTimer timer;
	r=timer.CreateLocal();
	test(r==KErrNone);
	TInt tick=0;
	TRequestStatus su;
	TRequestStatus st;
	TInt h=0;
	test.Next(_L("Logon to undertaker"));
	u.Logon(su,h);
	test.Next(_L("Start timer"));
	timer.After(st,500000);
	FOREVER
		{
		User::WaitForRequest(su,st);
		if (su!=KRequestPending)
			{
			RThread t;
			t.SetHandle(h);
			TName n=t.Name();
			TExitType exitType=t.ExitType();
			TInt exitReason=t.ExitReason();
			TName exitCategory=t.ExitCategory();
			t.Close();
			if (exitType==EExitPanic)
				{
				test.Printf(_L("Thread %S Panic %S %d\n"),&n,&exitCategory,exitReason);
				test(0);
				}
			if (exitType==EExitKill)
				{
				Restart(n);
				}
			h=0;
			u.Logon(su,h);
			}
		if (st!=KRequestPending)
			{
			if (!CheckerHasRun)
				{
				checker.SetPriority(EPriorityMuchMore);
				CheckerBumped++;
				}
			CheckerHasRun=EFalse;
			if (++tick==4)
				{
				tick=0;
				test.Printf(_L("Heap user iterations %d Creator iterations %d\n"),HeapUserIterations,CreatorIterations);
				test.Printf(_L("Checks %d Bumped %d\n"),TotalChecks,CheckerBumped);
				}
			timer.After(st,500000);
			}
		}
	}
