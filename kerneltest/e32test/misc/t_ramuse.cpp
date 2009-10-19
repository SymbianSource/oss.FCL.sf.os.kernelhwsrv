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
// e32test\misc\t_ramuse.cpp
// 
//

#include <e32test.h>

struct SThread
	{
	TUint iId;
	HBufC* iFullName;
	TInt iHandles;
	};

struct SProcess
	{
	TUint iId;
	HBufC* iFullName;
	TInt iHandles;
	};

LOCAL_D RTest test(_L("T_RAMUSE"));
LOCAL_D RArray<SThread> Threads;
LOCAL_D RArray<SProcess> Processes;

LOCAL_C void GetProcessInfo()
	{
	TFindProcess fp(_L("*"));
	TFullName fn;
	while (fp.Next(fn)==KErrNone)
		{
		RProcess p;
		TInt r=p.Open(fp);
		if (r!=KErrNone)
			continue;
		SProcess pInfo;
		TProcessId pid=p.Id();
		pInfo.iId=*((TUint*)&pid);
		pInfo.iFullName=fn.Alloc();
		pInfo.iHandles=-1;
		p.Close();
		Processes.InsertInUnsignedKeyOrder(pInfo);
		}
	}

LOCAL_C void GetThreadInfo()
	{
	TFindThread ft(_L("*"));
	TFullName fn;
	while (ft.Next(fn)==KErrNone)
		{
		RThread t;
		TInt r=t.Open(ft);
		if (r!=KErrNone)
			continue;
		SThread tInfo;
		TThreadId tid=t.Id();
		tInfo.iId=*((TUint*)&tid);
		tInfo.iFullName=fn.Alloc();
		TInt tHandles;
		TInt pHandles;
		t.HandleCount(pHandles,tHandles);
		tInfo.iHandles=tHandles;
		RProcess p;
		r=t.Process(p);
		if (r==KErrNone)
			{
			TProcessId pid=p.Id();
			SProcess pInfo;
			pInfo.iId=*((TUint*)&pid);
			TInt i;
			r=Processes.FindInUnsignedKeyOrder(pInfo,i);
			if (r==KErrNone)
				{
				if (Processes[i].iHandles<0)
					Processes[i].iHandles=pHandles;
				}
			p.Close();
			}
		t.Close();
		Threads.InsertInUnsignedKeyOrder(tInfo);
		}
	}

LOCAL_C void DisplayProcessInfo()
	{
	TInt n=Processes.Count();
	TInt i;
	test.Printf(_L("%d Processes:\n"),n);
	for (i=0; i<n; i++)
		{
		test.Printf(_L("%S %d handles\n"),Processes[i].iFullName,Processes[i].iHandles);
		test.Getch();
		}
	}

LOCAL_C void DisplayThreadInfo()
	{
	TInt n=Threads.Count();
	TInt i;
	test.Printf(_L("%d Threads:\n"),n);
	for (i=0; i<n; i++)
		{
		test.Printf(_L("%S %d handles\n"),Threads[i].iFullName,Threads[i].iHandles);
		test.Getch();
		}
	}

#ifdef __EPOC32__
extern TUint32 KernelHeapUsed();
#else
TUint32 KernelHeapUsed()
	{
	return 0;
	}
#endif

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Checking kernel RAM use"));

	test.Printf(_L("Total kernel heap used = %d bytes\n"),KernelHeapUsed());

	GetProcessInfo();
	GetThreadInfo();
	DisplayProcessInfo();
	DisplayThreadInfo();

	test.End();
	return 0;
	}
