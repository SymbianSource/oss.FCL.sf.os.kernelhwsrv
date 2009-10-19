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
// e32test\system\t_prot2.cpp
// 
//

#include <e32test.h>
#include "u32std.h"

const TInt KHeapSize=0x1000;

_LIT(KSecondProcessName,"T_PROT2A");
_LIT(KSecondProcessDataChunkName,"T_PROT2A*$DAT");

RTest test(_L("T_PROT2"));

LOCAL_C TInt WatcherThread(TAny* aSemaphoreHandle)
	{
	RTest wtest(_L("Watcher Thread"));
	wtest.Title();
	RThread().SetPriority(EPriorityMuchMore);
	wtest.Start(_L("Create undertaker"));
	RUndertaker u;
	TInt r=u.Create();
	if (r!=KErrNone)
		{
		RProcess me;
		me.Panic(_L("UNDERTAKER"),r);
		}
	RSemaphore sem;
	sem.SetHandle((TInt)aSemaphoreHandle);
	sem.Signal();
	FOREVER
		{
		TRequestStatus s;
		TInt h;
		u.Logon(s,h);
		User::WaitForRequest(s);
		RThread t;
		t.SetHandle(h);
		const TDesC& fn=t.FullName();
		wtest.Printf(_L("Thread %S exited\n"),&fn);
		const TDesC& cat=t.ExitCategory();
		wtest.Printf(_L("Exit type %d,%d,%S\n"),t.ExitType(),t.ExitReason(),&cat);
		t.Close();
		}
	}

LOCAL_C void StartWatcherThread()
	{
	test.Next(_L("Start watcher thread"));
	RSemaphore s;
	TInt r=s.CreateLocal(0);
	test(r==KErrNone);
	RThread t;
	r=t.Create(_L("WatcherThread"),WatcherThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)s.Handle());
	test(r==KErrNone);
	t.Resume();
	s.Wait();
	s.Close();
	}

void ExceptionHandler(TExcType)
	{
	User::Leave(KErrAbort);
	}

LOCAL_C TInt RogueThread1(TAny*)
	{
	TInt n;
	TInt m=0;
	TUint* p=NULL;
	RChunk c;
	User::SetExceptionHandler(ExceptionHandler,KExceptionFault);
	for (n=0; n<100; n++)
		{
		User::AfterHighRes(1000);	// wait 1ms
		if (m==0 && !p)
			{
			TFindChunk fc(KSecondProcessDataChunkName);
			TFullName fn;
			TInt r=fc.Next(fn);
			if (r!=KErrNone)
				continue;
			r=c.Open(fc);
			if (r!=KErrNone)
				continue;
			p=(TUint*)c.Base();	// second process is fixed
			continue;
			}
		if (m==0 && p)
			{
			if (c.Size()==0)
				continue;
			m++;
			c.Close();
			}
		TRAPD(r,Mem::Fill(p,0x1000,0xc9));
		if (r==KErrNone)
			return KErrNone;
		}
	User::Panic(_L("NOTFOUND"),m);
	return KErrNone;
	}

LOCAL_C void TestLoaderProtection()
	{
	test.Next(_L("Create rogue thread"));
	RThread t;
	TInt r=t.Create(_L("RogueThread1"),RogueThread1,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	t.SetPriority(EPriorityRealTime);	// this should be above the loader
	TRequestStatus s;
	t.Logon(s);
	test(s==KRequestPending);
	test.Next(_L("Resume rogue thread"));
	t.Resume();

	test.Next(_L("Create second process"));
	RProcess p;
	r=p.Create(KSecondProcessName,KNullDesC);
	test(r==KErrNone);
	TRequestStatus s2;
	p.Logon(s2);
	test(s2==KRequestPending);
	test.Next(_L("Resume second process"));
	p.Resume();
	
	test.Next(_L("Wait for second process to exit"));
	User::WaitForRequest(s2);
	const TDesC& pcat=p.ExitCategory();
	test.Printf(_L("Exit type %d,%d,%S\n"),p.ExitType(),p.ExitReason(),&pcat);
	test.Getch();

	test.Next(_L("Wait for rogue thread to exit"));
	User::WaitForRequest(s);
	const TDesC& tcat=t.ExitCategory();
	test.Printf(_L("Exit type %d,%d,%S\n"),t.ExitType(),t.ExitReason(),&tcat);
	test.Getch();
	}

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Testing protection against errant user threads"));
	RProcess().SetPriority(EPriorityHigh);

	StartWatcherThread();
	TestLoaderProtection();

	test.End();
	return KErrNone;
	}
