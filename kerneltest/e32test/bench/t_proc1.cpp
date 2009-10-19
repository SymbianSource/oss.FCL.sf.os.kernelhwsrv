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
// e32test\bench\t_proc1.cpp
// One half of the process relative type test stuff
// Overview:
// Tests the RProcess class, including tests on the heap, process naming, 
// process resumption, process creation and shared chunks. 
// API Information:
// RProcess
// Details:
// - Open a nonexistent process by a process Id and checks for the failure 
// of finding this process.
// - Open a process with invalid name and verify failure results are as expected.
// - Test the closing of processes by calling Kill, Terminate, and Panic methods.
// Verify results are as expected.
// - Create a process and verify the full path name of the loaded executable on 
// which this process is based.
// - Open a process by name, rename the process in a variety of ways and verify 
// the results are as expected.
// - Open a process, assign high, low, and bad priorities, verify the results
// are as expected.
// - Open a process, kill it and verify the results are as expected.
// - Open a process by name based on the file name, verify the results are as 
// expected.
// - Retrieve the process Id and open a handle on it. Create a duplicate process.
// Verify the results are as expected.
// - Find a process using TFindProcess() and open a handle on it. Verify the
// results are as expected.
// - Check chunk sharing between threads and verify results are as expected.
// - Perform a "speed" test where a new thread is created and the thread increments
// a count until stopped. Calculate and display counts per second. Verify results
// are as expected.
// - Verify the ExitReason, ExitType and ExitCatagory when the thread dies.
// - Verify that stopping the process completes existing pending requests with 
// KErrServerTerminated.
// - Verify that the heap was not corrupted by the tests.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include "../mmu/mmudetect.h"
#include "t_proc.h"

LOCAL_D RTest test(_L("T_PROC1"));

const TBufC<67> tooLong=_L("This should return KErrBadName and not crash.......0123456789ABCDEF");
const TBufC<66> notQuiteTooLong=_L("This should return KErrNone and be completely OK..0123456789ABCDEF");
const TBufC<26> bond=_L("Bond, James Bond[00000000]");
const TBufC<16> jamesBond=_L("Bond, James Bond");

RProcess proc;
RProcess proc2;
RProcess proc3;
RProcess proc4;

TName command;
TRequestStatus stat,notStat;

const TInt KKillReason=2563453;
const TInt KTerminateReason=8034255;
const TInt KPanicReason=39365235;

LOCAL_D RSemaphore client;
LOCAL_D TInt speedCount;

class RDisplay : public RSessionBase
	{
public:
	TInt Open();
	TInt Display(const TDesC& aMessage);
	TInt Read();
	TInt Write();
	TInt Stop();
	TInt Test();
	TVersion Version();
	};

TInt RDisplay::Open()
//
// Open the server.
//
	{

	return(CreateSession(_L("Display"),Version(),1));
	}

TInt RDisplay::Display(const TDesC& aMessage)
//
// Display a message.
//
	{

	TBuf<0x10> b(aMessage);
	return(SendReceive(CMyServer::EDisplay,TIpcArgs(&b)));
	}

TInt RDisplay::Read()
//
// Get session to test CSession2::ReadL.
//
	{

	TBuf<0x10> b(_L("Testing read"));
	return(SendReceive(CMyServer::ERead,TIpcArgs(&b)));
	}

TInt RDisplay::Write()
//
// Get session to test CSession2::WriteL.
//
	{

	TBuf<0x10> b;
    TBufC<0x10> c; // Bad descriptor - read only
	TInt r=SendReceive(CMyServer::EWrite,TIpcArgs(&b,&c));
	if (r==KErrNone && b!=_L("It worked!"))
		r=KErrGeneral;
	return r;
	}

TInt RDisplay::Test()
//
// Send a message and wait for completion.
//
	{

	TInt i[4];
	return(SendReceive(CMyServer::ETest,TIpcArgs(&i[0])));
	}

TInt RDisplay::Stop()
//
// Stop the server.
//
	{

	TInt i[4];
	return(SendReceive(CMyServer::EStop,TIpcArgs(&i[0])));
	}

TVersion RDisplay::Version()
//
// Return the current version.
//
	{

	TVersion v(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	return(v);
	}

LOCAL_C TInt RunPanicThread(RThread& aThread)
	{
	TRequestStatus s;
	aThread.Logon(s);
	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);
	aThread.Resume();
	User::WaitForRequest(s);
	User::SetJustInTime(jit);
	return s.Int();
	}

TInt KillProtectedEntry(TAny*)
	{
	proc.Kill(KErrGeneral);
	return KErrGeneral;
	}

TInt createProc2(RProcess& aProcess)
	{
	TFileName filename(RProcess().FileName());
	TInt pos=filename.LocateReverse(TChar('\\'));
	filename.SetLength(pos+1);
	filename+=_L("T_PROC2.EXE");
	TInt r=aProcess.Create(filename, command);
	if (r==KErrNone)
		{
		TFullName fn(aProcess.FullName());
		test.Printf(_L("Created %S\n"),&fn);
		}
	return r;
	}

void simpleTests1()
	{
	test.Next(_L("Open by name"));
	RProcess me;
	TInt r=proc2.Open(me.Name());
	test(r==KErrNone);
	test.Next(_L("Rename"));
	TName initName(me.Name());
	r=User::RenameProcess(jamesBond);
	test(r==KErrNone);
	test(me.Name().Left(26)==bond);
	test(proc2.Name().Left(26)==bond);
	r=User::RenameProcess(tooLong);
	test(r==KErrBadName);
	test(me.Name().Left(26)==bond);
	test(proc2.Name().Left(26)==bond);
	TName* work=new TName(notQuiteTooLong);
	r=User::RenameProcess(*work);
	test(r==KErrNone);
	work->Append(_L("[00000000]"));
	test(me.Name().Length()==KMaxKernelName);
	test(me.Name().Left(KMaxKernelName-4)==*work);
	test(proc2.Name().Length()==KMaxKernelName);
	test(proc2.Name().Left(KMaxKernelName-4)==*work);
	delete work;
	r=User::RenameProcess(_L("T_PROC1"));
	test(r==KErrNone);
	TFullName fn(_L("T_PROC1["));
	TUidType uidType(me.Type());
	TUint32 uid3 = uidType[2].iUid;
	fn.AppendNumFixedWidth(uid3,EHex,8);
	fn.Append(']');
	test(proc2.Name().Left(17)==fn);
	test(me.Name().Left(17)==fn);
	TInt l = initName.Locate('[');
	r=User::RenameProcess(initName.Left(l));
	test(r==KErrNone);
	test(proc2.Name()==initName);
	proc2.Close();
	}

TInt BadPriority(TAny* proc2)
	{
	((RProcess*)proc2)->SetPriority(EPriorityWindowServer);
	return KErrNone; 
	}

void simpleTests2()
	{
	TInt r=proc2.Open(proc.Name());
	test.Next(_L("Mess with Priority"));
	proc.SetPriority(EPriorityHigh);
	test.Printf(_L("%d %d\n"),proc.Priority(),proc2.Priority());
	test(proc.Priority()==EPriorityHigh);
	test(proc2.Priority()==EPriorityHigh);
	proc2.SetPriority(EPriorityLow);
	test(proc.Priority()==EPriorityLow);
	test(proc2.Priority()==EPriorityLow);

	RThread thread;
	r=thread.Create(_L("Bad Priority"),BadPriority,KDefaultStackSize,NULL,&proc2);
	test(r==KErrNone);
	r=RunPanicThread(thread);
	test(r==EBadPriority);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitReason()==EBadPriority);
	test(thread.ExitCategory()==_L("KERN-EXEC"));
	CLOSE_AND_WAIT(thread);
	test(proc.Priority()==EPriorityLow);
	test(proc2.Priority()==EPriorityLow);
	proc2.Close();
	}

void procTests1()
	{
	test.Next(_L("Test functions"));
    TFileName fileName(proc.FileName());
    test.Printf(fileName);
#ifndef __WINS__
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		test(fileName.Mid(1).CompareF(_L(":\\Sys\\Bin\\T_PROC2.EXE"))==0);
	else
		test(fileName.Mid(1).CompareF(_L(":\\System\\Bin\\T_PROC2.EXE"))==0);
#else
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		test(fileName.CompareF(_L("Z:\\Sys\\Bin\\T_PROC2.EXE"))==0);
	else
		test(fileName.CompareF(_L("Z:\\System\\Bin\\T_PROC2.EXE"))==0);
#endif
	test(proc.Name().Left(21).CompareF(_L("T_PROC2.EXE[00000000]"))==0);
	test(proc.Priority()==EPriorityForeground);
	test(proc.ExitType()==EExitPending);
	test(proc.ExitReason()==0);
	test(proc.ExitCategory()==KNullDesC);
	}

void procTests2()
	{
	test.Next(_L("Kill and recreate"));
	RProcess proc2;
	TInt r=proc2.Open(proc.Id());
	test(r==KErrNone);
	test(proc2.Handle()!=0);
	proc.Logon(stat);
	proc.Logon(notStat);
	r=proc.LogonCancel(notStat);
	test(r==KErrNone);
	test(notStat==KErrNone);
	proc.Kill(KKillReason);
	User::WaitForRequest(stat);
	test(stat==KKillReason);
	test(proc.ExitType()==EExitKill);
	test(proc.ExitReason()==KKillReason);
	test(proc.ExitCategory()==_L("Kill"));
	proc.Close();
	test(proc.Handle()==0);
	test(proc2.ExitType()==EExitKill);
	test(proc2.ExitReason()==KKillReason);
	test(proc2.ExitCategory()==_L("Kill"));
	CLOSE_AND_WAIT(proc2);
	test(proc2.Handle()==0);
	}

void procTests3()
	{
	TFileName filename(RProcess().FileName());
	TInt pos=filename.LocateReverse(TChar('\\'));
	filename.SetLength(pos+1);
	filename+=_L("T_PROC2.EXE");
	TInt r=proc.Create(filename, command);
	test(r==KErrNone);
	TFullName fn(proc.FullName());
	test.Printf(_L("Created %S\n"),&fn);
	test(proc.FileName().CompareF(filename)==0);
	test(proc.Name().Left(21).CompareF(_L("T_PROC2.EXE[00000000]"))==0);
	test(proc.Priority()==EPriorityForeground);
	test(proc.ExitType()==EExitPending);
	test(proc.ExitReason()==0);
	test(proc.ExitCategory()==KNullDesC);
	}

void procTests4()
	{
	test.Next(_L("Get and open by Id"));
	TProcessId id=proc.Id();
	TProcessId id2=proc.Id();
	test(id==id2);
	TInt r=proc2.Open(proc.Name());
	test(r==KErrNone);
	id2=proc2.Id();
	test(id==id2);
	r=proc3.Open(id);
	test(r==KErrNone);
	id2=proc3.Id();
	test(id==id2);
	proc3.Close();
	if (HaveVirtMem())
		{
		test.Next(_L("Create duplicate"));
		r=createProc2(proc3);
		test(r==KErrNone);
		id2=proc3.Id();
		test(id!=id2);
		test(*(TUint*)&id<*(TUint*)&id2);
		}
	}

void procTests5()
	{
	test.Next(_L("Try to find processes"));
	TFindProcess* findProc=new TFindProcess(_L("T_PROC2*"));
	test(findProc!=NULL);
	TFullName* result=new TFullName;
	test(result!=NULL);
	TInt r=findProc->Next(*result);
	test(r==KErrNone);
	TFullName temp = proc.FullName();
	test(result->CompareF(temp)==0);
	r=findProc->Next(*result);
	test(r==KErrNone);
	test(result->CompareF(proc3.FullName())==0);
	r=findProc->Next(*result);
	test(r==KErrNotFound);
	findProc->Find(_L("T?PROC2*]*"));
	r=findProc->Next(*result);
	test(r==KErrNone);
	test(result->CompareF(temp)==0);
	r=findProc->Next(*result);
	test(r==KErrNone);
	test(result->CompareF(proc3.FullName())==0);
	delete result;
	test.Next(_L("Open by find handle"));
	r=proc4.Open(*findProc);
	test(r==KErrNone);
	TProcessId id=proc3.Id();
	TProcessId id2=proc4.Id();
	test(id==id2);
	delete findProc;
	}

void procTests6()
	{
	test.Next(_L("Kill duplicate"));
	proc3.Logon(stat);
	test(stat==KRequestPending);
	proc4.Kill(12345);
	User::WaitForRequest(stat);
	test(stat==12345);
	}

void createProcess()
//
// Create T_PROC2 and, on the way, do lots of basic tests
//
	{
	test.Start(_L("Create"));
	TInt r=globSem1.CreateGlobal(_L("GlobSem1"), 0);
	test(r==KErrNone);
	r=globSem2.CreateGlobal(_L("GlobSem2"), 0);
	test(r==KErrNone);

	r=createProc2(proc);
	test(r==KErrNone);

	procTests1();

	simpleTests1();
	simpleTests2();

	procTests2();
	procTests3();
	procTests4();
	if (HaveVirtMem())
		{
		procTests5();
		procTests6();
		}

	proc2.Close();
	proc3.Close();
	if (proc4.Handle())
		CLOSE_AND_WAIT(proc4);

	test.Next(_L("Resume"));
	proc.Logon(stat); // logon to process
	proc.Logon(notStat);
	r=proc.LogonCancel(notStat);
	test(r==KErrNone);
	test(notStat==KErrNone);
	test(proc.ExitType()==EExitPending);
	proc.Resume();
	globSem1.Wait(); // wait for T_PROC2 to get started
	test.End();
	}

void murderProcess()
	{
	test.Start(_L("Kill"));
	RProcess process;
	TInt r=createProc2(process);
	test(r==KErrNone);
	TProcessId id=process.Id();
	TProcessId id2=process.Id();
	test(id==id2);
 	TRequestStatus stat;
	process.Logon(stat);
	test(process.ExitType()==EExitPending);
	process.Kill(KKillReason);
	User::WaitForRequest(stat);
	test(stat==KKillReason);
	test(process.ExitType()==EExitKill);
	test(process.ExitReason()==KKillReason);
	test(process.ExitCategory()==_L("Kill"));
	CLOSE_AND_WAIT(process);

	test.Next(_L("Terminate"));
	r=createProc2(process);
	test(r==KErrNone);
	id2=process.Id();
	test(*(TUint*)&id+2==*(TUint*)&id2);	// use 2 ID's each time, one for process, one for thread
	process.Logon(stat);
	test(process.ExitType()==EExitPending);
	process.Terminate(KTerminateReason);
	User::WaitForRequest(stat);
	test(stat==KTerminateReason);
	test(process.ExitType()==EExitTerminate);
	test(process.ExitReason()==KTerminateReason);
	test(process.ExitCategory()==_L("Terminate"));
	CLOSE_AND_WAIT(process);

	test.Next(_L("Panic"));
	r=createProc2(process);
	test(r==KErrNone);
	id2=process.Id();
	test(*(TUint*)&id+4==*(TUint*)&id2);
 	test(process.ExitType()==EExitPending);
	process.Logon(stat);
	process.SetJustInTime(EFalse);	// prevent the process panic from starting the debugger
	process.Panic(_L("BOO!"),KPanicReason);
	User::WaitForRequest(stat);
	test(stat==KPanicReason);
	test(process.ExitType()==EExitPanic);
	test(process.ExitReason()==KPanicReason);
	test(process.ExitCategory()==_L("BOO!"));
	CLOSE_AND_WAIT(process);
	test.End();
	}

void sharedChunks()
	{
	test.Start(_L("Test chunk sharing between threads"));

	test.Next(_L("Create chunk Marmalade"));
	TInt r=0;
	RChunk chunk;
	TInt size=0x1000;
	TInt maxSize=0x5000;
	r=chunk.CreateGlobal(_L("Marmalade"),size,maxSize);
	test(r==KErrNone);
	test.Next(_L("Write 0-9 to it"));
	TUint8* base=chunk.Base();
	for (TInt8 j=0;j<10;j++)
		*base++=j; // write 0 - 9 to the chunk
  	globSem2.Signal(); // T_PROC2 can check the chunk now
	globSem1.Wait();
	chunk.Close(); // now it's ok to kill the chunk

	test.End();
	}

TInt sharedChunks2(TAny* /*aDummy*/)
	{
    RTest test(_L("Shared Chunks 2"));

	test.Title();	
	test.Start(_L("Test chunk sharing between threads"));

	test.Next(_L("Create chunk Marmalade"));
	TInt r=0;
	RChunk chunk;
	TInt size=0x1000;
	TInt maxSize=0x5000;
	r=chunk.CreateGlobal(_L("Marmalade"),size,maxSize);
	test(r==KErrNone);
	test.Next(_L("Write 0-9 to it"));
	TUint8* base=chunk.Base();
	for (TInt8 j=0;j<10;j++)
		*base++=j; // write 0 - 9 to the chunk
  	globSem2.Signal(); // T_PROC2 can check the chunk now
	globSem1.Wait();
	chunk.Close(); // now it's ok to kill the chunk

	test.End();
    return(KErrNone);
	}

TInt speedyThreadEntryPoint(TAny*)
//
// The entry point for the speed test thread.
//
	{
	RDisplay t;
	TInt r=t.Open();
	test(r==KErrNone);
	speedCount=0;
	client.Signal();
	while ((r=t.Test())==KErrNone)
		speedCount++;
	t.Close();
	return r;
	}

TInt BadName(TAny*)
	{
	proc.Open(_L("*"));
	return KErrNone;
	}

TInt sharedHeap(TAny*)
	{
	RTest test2(_L("sharedHeap"));
	test2.Title();
	test2.Start(_L("Shared heap tests"));

	RAllocator* allocator = &User::Allocator();
	test2.Printf(_L("sharedHeap's heap is at %08x\n"), allocator);
	
	TInt size;
	allocator->AllocSize(size);
	test2.Printf(_L("sharedHeap's heap allocsize is %08x\n"),size);

// Press a key only if running the test in manual mode. We will be
// able to ascertain this when RTest has been enhanced.
//	test.Next(_L("Press a key to continue"));
//	test2.Getch();

	test2.End();
	return(KErrNone);
	}

_LIT(KTestProcessNewName,"T_PROC1_NEW.EXE");

TInt DupRenameProcTest(TInt aCall)
	{
	test.Printf(_L("DupRenameProcTest: call %d\n"),aCall);

	TInt r;

	switch(aCall)
		{
	case 1:
		{
		r = User::RenameProcess(KTestProcessNewName);
		test(r==KErrNone);
		TFullName fn(RProcess().FullName());
		test.Printf(_L("Renamed to %S\n"),&fn);
		TInt li = fn.Locate('[');
		TInt ri = fn.Locate(']');
		test(fn.Left(li)==KTestProcessNewName);
		test(fn.Mid(ri+1)==_L("0001"));
		}

	case 0:
		{
		TFileName filename(RProcess().FileName());
		TInt pos=filename.LocateReverse(TChar('\\'));
		filename.SetLength(pos+1);
		filename+=_L("T_PROC1.EXE");
		RProcess pr;
		TBuf16<10> call;
		call.Num(aCall+1);
		r = pr.Create(filename, call);
		TFullName fn(pr.FullName());
		test.Printf(_L("Created %S\n"),&fn);
		TRequestStatus st;
		pr.Logon(st);
		pr.Resume();
		User::WaitForRequest(st);
		CLOSE_AND_WAIT(pr);
		}
		return KErrNone;

	case 2:
		{
		r = User::RenameProcess(KTestProcessNewName);
		test(r==KErrNone);
		TFullName fn(RProcess().FullName());
		test.Printf(_L("Renamed to %S\n"),&fn);
		TInt li = fn.Locate('[');
		TInt ri = fn.Locate(']');
		test(fn.Left(li)==KTestProcessNewName);
		test(fn.Mid(ri+1)==_L("0002"));
		}
		return KErrNone;

	default:
		return KErrArgument;
		}
	}

_LIT(KTestProcessName,"TestName");

void TestProcessRename()
	{
	// Rename the current process with test name
    TInt r = User::RenameProcess(KTestProcessName);
    test(r==KErrNone);
    TName name1 = RProcess().Name();

    // Check new name is correct
    TName name2 = name1;
    name2.SetLength(KTestProcessName().Length());
    test(name2.CompareF(KTestProcessName)==0);
    
    // Rename the process with same test name
    r = User::RenameProcess(KTestProcessName);
    test(r==KErrNone);
    name2 = RProcess().Name();
    test(name1.Compare(name2)==0);  // name should be unchanged
	}

TInt E32Main()
	{
	__KHEAP_MARK;

	// Turn off lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	TBuf16<512> cmd;
	User::CommandLine(cmd);
	if(cmd.Length() && TChar(cmd[0]).IsDigit())
		{
		TInt r = DupRenameProcTest(TUint(TChar(cmd[0])) - '0');
		test(r==KErrNone);
		return 0;
		}

	test.Title();

	test.Start(_L("Testing process stuff 1"));
	TInt r;
	TRequestStatus s;
	test.Next(_L("Creating semaphore"));
	r=client.CreateLocal(0);
	test(r==KErrNone);

	test.Next(_L("Try to open nonexistant process by ID"));
	r=proc.Open(*(TProcessId*)&KMaxTUint);
	test(r==KErrNotFound);

	test.Next(_L("Try to open process with invalid name"));
	RThread thread;
	r=thread.Create(_L("Bad Name"),BadName,KDefaultStackSize,NULL,NULL);
	test(r==KErrNone);
	TRequestStatus threadStat;
	thread.Logon(threadStat);
	TBool justInTime=User::JustInTime();
	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(threadStat);
	User::SetJustInTime(justInTime);
	test(threadStat==EBadName);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitReason()==EBadName);
	test(thread.ExitCategory()==_L("KERN-EXEC"));
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Murder processes in different ways"));
	murderProcess();

	test.Next(_L("Create second process"));
	createProcess();

	test.Next(_L("Shared Chunks from main thread"));
	sharedChunks();

	test.Next(_L("Shared chunks from secondary thread"));
	RThread t;
	r=t.Create(_L("Shared chunks 2"),sharedChunks2,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
    test(r==KErrNone);
	t.Logon(s);
	t.Resume();
	User::WaitForRequest(s);
	test(s==KErrNone);
	CLOSE_AND_WAIT(t);

	test.Next(_L("Starting speedy client"));
	RThread speedy;
	r=speedy.Create(_L("Speedy"),speedyThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);

	RThread().SetPriority(EPriorityMuchMore);
    speedy.SetPriority(EPriorityNormal);
	TRequestStatus speedyStatus;
	speedy.Logon(speedyStatus);
	speedy.Resume();

	test.Next(_L("Wait for speedy to start"));
	client.Wait();

	globSem1.Wait(); // wait for proc2 to be nice & quiet
	test.Printf(_L("Starting speed test...\n"));
    User::After(300000);
    TInt b=speedCount;
    User::After(3000000);
    TInt n=speedCount;
    test.Printf(_L("Count = %d in 1 second\n"),(n-b)/3);

	test.Next(_L("Tell second process speed tests are done"));
	globSem2.Signal();

	test.Next(_L("Process Logon"));
	User::WaitForRequest(stat);
	const TDesC& cat=proc.ExitCategory();
	test.Printf(_L("Exit category = %S\n"),&cat);
	test.Printf(_L("Exit reason = %x\n"),proc.ExitReason());
	test.Printf(_L("Exit type = %x\n"),proc.ExitType());

	test(stat==KErrNone);
	test(proc.ExitCategory()==_L("Kill"));
	test(proc.ExitReason()==KErrNone);
	test(proc.ExitType()==EExitKill);
	test(notStat==KErrNone);
	test.Next(_L("Test LogonCancel to dead process is ok"));
	r=proc.LogonCancel(stat);
	test(r==KErrGeneral);
	globSem1.Close();
	globSem2.Close();
	client.Close();

	User::WaitForRequest(speedyStatus);
	test(speedyStatus==KErrServerTerminated);
	test(speedy.ExitReason()==KErrServerTerminated);
	test(speedy.ExitType()==EExitKill);
	CLOSE_AND_WAIT(speedy);
	CLOSE_AND_WAIT(proc);

	User::After(5000000);	// wait for MMC session to disappear

	test.Next(_L("Test rename of the processes with duplicate names"));
	r = DupRenameProcTest(0);
	test(r==KErrNone);

    TestProcessRename();

	test.Next(_L("Check for kernel alloc heaven"));
	__KHEAP_MARKEND; 

	test.End();

	return(KErrNone);
	}


