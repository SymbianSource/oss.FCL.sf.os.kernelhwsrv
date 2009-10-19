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
// e32test\dll\t_tdlla.cpp
// Overview:
// Test static data in DLLs
// API Information:
// RLibrary
// Details:
// - Test statically linked DLLs with static data and verify results.
// - Load a dynamically loadable DLL, check DLL data, verify results.
// - Load a dynamically loadable DLL in another thread, check DLL data, 
// verify results.
// - Load a DLL that is statically linked to the test process, check 
// DLL data, verify results.
// - Load in a different thread a DLL that is statically linked to the 
// test process, check DLL data, verify results.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32svr.h>

#include "../mmu/mmudetect.h"

LOCAL_D RTest test(_L("T_TDLLA"));
TInt InitialisedData=0x7c99103b;
TInt ZeroInitialisedData[32];

IMPORT_C TInt Function3();
IMPORT_C TInt Function4();
IMPORT_C void SetDll3Data(TInt);
IMPORT_C TInt GetDll3Data();

_LIT(KExeCommandLine,"2");
_LIT(KDll1Name,"T_DLLA1");
_LIT(KDll2Name,"T_DLLA2");
_LIT(KDll3Name,"T_DLLA3");

typedef TFullName& (*PFRFN)();
TFullName& DllA1ProcessName(RLibrary lib)
	{

	PFRFN f=(PFRFN)lib.Lookup(3);
	return (*f)();
	}

TFullName& DllA1ThreadName(RLibrary lib)
	{

	PFRFN f=(PFRFN)lib.Lookup(4);
	return (*f)();
	}

void Kick(RThread aThread)
	{
	TRequestStatus s;
	TRequestStatus* pS=&s;
	aThread.RequestComplete(pS,0);
	}

RSemaphore gThreadSem;
TInt Thread1(TAny* a)
	{
	TInt r=KErrNone;
	const TDesC& name=*(const TDesC*)a;
	RLibrary lib;
	r=lib.Load(name);
	if (r!=KErrNone)
		return r;
	gThreadSem.Signal();
	User::WaitForAnyRequest();
	lib.Close();
	return KErrNone;
	}

TInt Thread2(TAny* a)
	{
	TInt r=KErrNone;
	const TDesC& name=*(const TDesC*)a;
	RLibrary lib;
	r=lib.Load(name);
	if (r!=KErrNone)
		return r;
	gThreadSem.Signal();
	User::WaitForAnyRequest();
	return KErrNone;
	}

LOCAL_D TInt ProcessEntryCount=0;
LOCAL_D TInt ExportCallCount=0;
EXPORT_C void ExportedFunction()
	{
	++ExportCallCount;
	}

void SpawnExe()
	{
#ifdef __EPOC32__
	test.Printf(_L("SpawnExe()\n"));
	if (User::CommandLineLength()!=0)
		{
		test.Printf(_L("This is second EXE\n"));
		return;
		}
	RProcess p;
	TInt r=p.Create(RProcess().FileName(), KExeCommandLine);
	test(r==KErrNone);
	TFullName aFullName = p.FullName();
	test.Printf(_L("Second EXE: %S\n"),&aFullName);
	TRequestStatus s;
	p.Logon(s);
	p.Resume();
	User::WaitForRequest(s);
	TExitCategoryName aExitCategory = p.ExitCategory();
	test.Printf(_L("Second EXE: %d,%d,%S\n"),p.ExitType(),p.ExitReason(),&aExitCategory);
	CLOSE_AND_WAIT(p);
#endif
	}

GLDEF_C TInt E32Main()
//
// Test static data in dlls
//
	{
				
	// Turn off evil lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	test(InitialisedData==0x7c99103b);
	++InitialisedData;
	TInt i;
	for (i=0; i<31; ++i)
		test(ZeroInitialisedData[i]==0);
	for (i=0; i<31; ++i)
		ZeroInitialisedData[i]=(i+487)*(i+487);
	if (++ProcessEntryCount!=1)
		User::Panic(_L("PROC_REENT"),ProcessEntryCount);

	test.Title();
	test.Start(_L("Test statically linked dlls with static data"));

	test(Function3()==3);
	test(Function4()==4);
	SetDll3Data(101);
	test(GetDll3Data()==101);
	SetDll3Data(105);
	test(GetDll3Data()==105);

	test.Next(_L("Load T_DLLA1"));

	RLibrary lib;
	TInt r=lib.Load(KDll1Name);
	test(r==KErrNone);
	TLibraryFunction f=lib.Lookup(1);
	test((*f)()==KErrNone);
	f=lib.Lookup(2);
	test((*f)()==1);

	test.Printf(_L("DLLA1 Process Name %S\n"),&DllA1ProcessName(lib));
	test.Printf(_L("DLLA1 Thread  Name %S\n"),&DllA1ThreadName(lib));
	test(DllA1ProcessName(lib)==RProcess().FullName());
	test(DllA1ThreadName(lib)==RThread().FullName());

	test.Next(_L("Unload"));
	lib.Close();

	test.Next(_L("Load again"));
	r=lib.Load(KDll1Name);
	test(r==KErrNone);
	test.Next(_L("Check Dll data"));
	f=lib.Lookup(1);
	test((*f)()==KErrNone);
	f=lib.Lookup(2);
	test((*f)()==1);
	test((*f)()==2);
	test((*f)()==3);
	test.Next(_L("Close"));
	lib.Close();

	test.Next(_L("Loading T_DLLA1 again in another thread"));
	r=gThreadSem.CreateLocal(0);
	RThread t;
	r=t.Create(_L("Thread1"), Thread1, KDefaultStackSize, NULL, (TAny*)&KDll1Name);
	test(r==KErrNone);
	t.SetPriority(EPriorityMore);
	TRequestStatus stat;
	t.Logon(stat);
	t.Resume();
	gThreadSem.Wait();

	test.Next(_L("Load again"));
	r=lib.Load(KDll1Name);
	test(r==KErrNone);
	test.Printf(_L("DLLA1 Process Name %S\n"),&DllA1ProcessName(lib));
	test.Printf(_L("DLLA1 Thread  Name %S\n"),&DllA1ThreadName(lib));
	test(DllA1ProcessName(lib)==RProcess().FullName());
	test(DllA1ThreadName(lib)==t.FullName());
	test.Next(_L("Check Dll data"));
	f=lib.Lookup(1);
	test((*f)()==KErrNone);
	f=lib.Lookup(2);
	test((*f)()==1);
	test((*f)()==2);
	test((*f)()==3);

	Kick(t);
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	test(t.ExitType()==EExitKill);

	SpawnExe();

	test.Printf(_L("DLLA1 Process Name %S\n"),&DllA1ProcessName(lib));
	test.Printf(_L("DLLA1 Thread  Name %S\n"),&DllA1ThreadName(lib));
	test(DllA1ProcessName(lib)==RProcess().FullName());
	test(DllA1ThreadName(lib)==t.FullName());
	test((*f)()==4);
	test((*f)()==5);
	test((*f)()==6);

	CLOSE_AND_WAIT(t);
	test.Next(_L("Close"));
	lib.Close();

	test.Next(_L("Load a dll that this process is statically linked to"));
	r=lib.Load(KDll3Name);
	test.Printf(_L("Returns %d\n"),r);
	test(r==KErrNone);
	test(GetDll3Data()==105);

	test.Next(_L("Close it"));
	lib.Close();
	test(GetDll3Data()==105);

	test.Next(_L("Load a dll that is statically linked to this process"));
	test(ExportCallCount==0);
	r=lib.Load(KDll2Name);
	test(r==KErrNone);
	test(InitialisedData==0x7c99103c);
	for (i=0; i<31; ++i)
		test(ZeroInitialisedData[i]==(i+487)*(i+487));
	test(ExportCallCount==1);
	test.Next(_L("Close it"));
	lib.Close();
	test(ExportCallCount==2);

	test.Next(_L("Load it in a different thread"));
	r=t.Create(_L("Thread1"), Thread1, KDefaultStackSize, NULL, (TAny*)&KDll2Name);
	test(r==KErrNone);
	t.SetPriority(EPriorityMore);
	t.Logon(stat);
	t.Resume();
	gThreadSem.Wait();
	test(ExportCallCount==3);
	Kick(t);
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	test(t.ExitType()==EExitKill);
	test(ExportCallCount==4);
	CLOSE_AND_WAIT(t);

	r=t.Create(_L("Thread2"), Thread2, KDefaultStackSize, NULL, (TAny*)&KDll2Name);
	test(r==KErrNone);
	t.SetPriority(EPriorityMore);
	t.Logon(stat);
	t.Resume();
	gThreadSem.Wait();
	test(ExportCallCount==5);
	SpawnExe();
	Kick(t);
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	test(t.ExitType()==EExitKill);
	test(ExportCallCount==6);
	CLOSE_AND_WAIT(t);

	test.Next(_L("Test loading twice"));
	RDebug::Print(_L("Loading T_DLLA1.DLL"));
	r=lib.Load(KDll1Name);
	test(r==KErrNone);
	f=lib.Lookup(1);
	test((*f)()==KErrNone);
	f=lib.Lookup(2);
	test((*f)()==1);
	test(DllA1ProcessName(lib)==RProcess().FullName());
	test(DllA1ThreadName(lib)==RThread().FullName());
	RLibrary lib2;
	RDebug::Print(_L("Loading T_DLLA1.DLL again"));
	r=lib2.Load(KDll1Name);
	test(r==KErrNone);
	f=lib2.Lookup(2);
	test((*f)()==2);
	test(DllA1ProcessName(lib2)==RProcess().FullName());
	test(DllA1ThreadName(lib2)==RThread().FullName());

	test.Next(_L("Close One"));
	RDebug::Print(_L("Closing T_DLLA1"));
	lib.Close();
	f=lib2.Lookup(2);
	test((*f)()==3);
	test(DllA1ProcessName(lib2)==RProcess().FullName());
	test(DllA1ThreadName(lib2)==RThread().FullName());

	test.Next(_L("Close Two"));
	RDebug::Print(_L("Closing T_DLLA1 again"));
	lib2.Close();
	test(GetDll3Data()==105);

	test.End();
	return(KErrNone);
	}

