/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/


/**
Overview:
	Test DLL Writeable Static Data support

API Information:


Details:
	- Each process has independent DLL WSD
	- Whether DLL linked directly or indirectly
	- Whether DLL loaded dynamically or statically
	- DLL WSD is consistent under heavy usage by multiple processes
	- IPC works to/from DLL WSD descriptors & TRequestStatus
	This source file builds in 4 configurations, with each of
	direct and indirect linking either used or not used.
	These configurations are set by 4 MM files, t_dllwsd[d][i].mmp
	Any of the exe created from the MMP files can be started 
	to run the tests, it does not matter which is used. 
	All exe configurations will be used during the tests.

Platforms/Drives/Compatibility:
	All.

Assumptions/Requirement/Pre-requisites:
	

Failures and causes:
	

Base Port information:

*/

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32svr.h>
#include <f32dbg.h>
#include <u32std.h>
#include "t_dllwsd_dll.h"
#include "t_dllwsd_dlli.h"


LOCAL_D RTest test(_L("T_DLLWSD"));

enum TTestFunc
	{
	ETestFuncTestCons=1,
	ETestFuncThrash1,
	ETestFuncIpcTest,
	ETestFuncIpcGet,
	ETestFuncIpcReverse,
	ETestFuncIpcSet,
	ETestFuncPanic,
	};

// Test session for IPC use of WSD, talks to the same server as RDllWsd
class RIpcTestSession : public RSessionBase
	{
	public:
	TInt Connect()
		{
		return CreateSession(_L("IpcTestServer"), TVersion());
		}
	void Get(TBuf<60000>& buf, TRequestStatus& req)
		{
		SendReceive(ETestFuncIpcGet, TIpcArgs(&buf), req);
		}
	void Reverse(TRequestStatus& req)
		{
		SendReceive(ETestFuncIpcReverse, req);
		}
	void Set(const TBuf<60000>& buf, TRequestStatus& req)
		{
		SendReceive(ETestFuncIpcSet, TIpcArgs(&buf), req);
		}
	};

#ifdef T_DLLWSD_DIRECT
void FillBuf(TInt start, TInt inc)
	{
	for (int ii=0; ii<WsdBuf().Length(); ii++)
		{
		WsdBuf()[ii] = (unsigned short)start;
		start += inc;
		}
	}

TInt CheckBuf(TInt start, TInt inc)
	{
	for (int ii=0; ii<WsdBuf().Length(); ii++)
		{
		if (WsdBuf()[ii] != start)
			return KErrGeneral;
		start += inc;
		}
	return KErrNone;
	}
#endif

class CDllWsdServer : public CServer2
	{
public:
	CDllWsdServer() : CServer2(EPriorityStandard)
		{
		}
	CSession2* NewSessionL(const TVersion& /*aVersion*/,const RMessage2& /*aMessage*/) const;
	mutable TInt iCount;
	};

class CDllWsdSession : public CSession2
	{
public:
	CDllWsdSession()
		{
		ResetConsistencyCheck();
		}
		
	~CDllWsdSession()
		{
		// transient server immediate shutdown when last client disconnects
		if (--((CDllWsdServer*)Server())->iCount == 0)
			CActiveScheduler::Stop();
		}
		
	void ResetConsistencyCheck()
		{
		iX=42;
		iY=0;
		}

	void OptResetConsistencyCheck()
		{
#if !defined(T_DLLWSD_DIRECT) && !defined(T_DLLWSD_INDIRECT)
		// if DLL has been unloaded (dynamic library closed, no static link)
		// WSD will be reset
		ResetConsistencyCheck();
#endif
		}
		
	TInt TestConsistency()
		{
#ifdef T_DLLWSD_DIRECT
		// static direct
		if (WsdFuncX() != iX++)
			return KErrGeneral;
		if (WsdFuncY() != iY++)
			return KErrGeneral;
#endif

#ifdef T_DLLWSD_INDIRECT
		// static indirect
		if (IndWsdFuncX() != iX++)
			return KErrGeneral;
		if (IndWsdFuncY() != iY++)
			return KErrGeneral;
#endif

		// dynamic direct
		OptResetConsistencyCheck();
		RLibrary lib;
		TInt err = lib.Load(_L("t_dllwsd_dll"));
		if (err) return err;
		if ((*lib.Lookup(1))/*WsdFuncX*/() != iX++)
			return KErrGeneral;
		if ((*lib.Lookup(2))/*WsdFuncX*/() != iY++)
			return KErrGeneral;
		lib.Close();

		// dynamic indirect
		OptResetConsistencyCheck();
		err = lib.Load(_L("t_dllwsd_dlli"));
		if (err) return err;
		if ((*lib.Lookup(1))/*IndWsdFuncX*/() != iX++)
			return KErrGeneral;
		if ((*lib.Lookup(2))/*IndWsdFuncX*/() != iY++)
			return KErrGeneral;
		lib.Close();
		
		return KErrNone;
		}
		
	TInt Thrash1()
		{
		TTime start;
		start.HomeTime();
		TInt count = 0;
		const TTimeIntervalMicroSeconds limit(10000000); // 10 seconds
		for (;; count++)
			{
			TInt err = TestConsistency();
			if (err) return err;
			TTime now;
			now.HomeTime();
			if (now.MicroSecondsFrom(start) > limit)
				break;
			}
		return count > 0 ? count : -count;
		}
		
	TInt IpcTest()
		{
#ifdef T_DLLWSD_DIRECT
    	RIpcTestSession s;
    	TInt err = s.Connect();
    	if (!err) return err;
        WsdBuf().SetLength(WsdBuf().MaxLength());
    	for (int i=0; i<10; i++)
    		{
    		// 0..n -> buf
	        FillBuf(0,1);
	        err = CheckBuf(0,1);
	    	if (!err) return err;
	        
	        // buf -> server
	        s.Set(WsdBuf(), WsdReq());
	        err = CheckBuf(0,1);
	    	if (!err) return err;
	    	
	    	// use TReqestStatus in WSD
	        User::WaitForRequest(WsdReq());
	        if (!WsdReq().Int()) return WsdReq().Int();
	        
	        // 0..0 -> buf
	        FillBuf(0,0);
	        err = CheckBuf(0,0);
	    	if (!err) return err;
	        WsdReq() = KRequestPending;

			// reverse buf on server
	        s.Reverse(WsdReq());
	        
			// local buf is still 0..0
	        err = CheckBuf(0,0);
	    	if (!err) return err;

	    	// use TReqestStatus in WSD
	        User::WaitForRequest(WsdReq());
	        if (!WsdReq().Int()) return WsdReq().Int();

			// local buf is still 0..0
	        err = CheckBuf(0,0);
	    	if (!err) return err;

	        // get buf from server
	        s.Get(WsdBuf(), WsdReq());
	        User::WaitForRequest(WsdReq());
	        
	        // buf is n..0
	        err = CheckBuf(59999,-1);
	    	if (!err) return err;
    		}
    	s.Close();
		return KErrNone;
#else
		return KErrNotSupported;
#endif
		}
	
	void ServiceL(const RMessage2& aMessage)
		{
#ifdef T_DLLWSD_DIRECT
		TInt ii=0;
#endif
		switch (aMessage.Function())
			{
			case ETestFuncTestCons:
				aMessage.Complete(TestConsistency());
				break;
			case ETestFuncThrash1:
				aMessage.Complete(Thrash1());
				break;
			case ETestFuncIpcTest:
				aMessage.Complete(IpcTest());
				break;
			case ETestFuncIpcGet:
#ifdef T_DLLWSD_DIRECT
				aMessage.WriteL(0, WsdBuf());
				aMessage.Complete(KErrNone);
#else
				aMessage.Complete(KErrNotSupported);
#endif
				break;
			case ETestFuncIpcReverse:
#ifdef T_DLLWSD_DIRECT
				for (ii=0; ii<WsdBuf().Length()/2; ii++)
					{
					TInt o = WsdBuf().Length() - 1 - ii;
					TInt t = WsdBuf()[ii];
					WsdBuf()[ii] = WsdBuf()[o];
					WsdBuf()[o] = (unsigned short)t;
					}
				aMessage.Complete(KErrNone);
#else
				aMessage.Complete(KErrNotSupported);
#endif
				break;
			case ETestFuncIpcSet:
#ifdef T_DLLWSD_DIRECT
				aMessage.ReadL(0, WsdBuf());
				aMessage.Complete(KErrNone);
#else
				aMessage.Complete(KErrNotSupported);
#endif
				break;
			case ETestFuncPanic:
				User::Panic(_L("As requested..."), 0);
				break;
			default:
				aMessage.Panic(_L("Unrecognised"), aMessage.Function());
				break;
			}
		}
	
	int iX;
	int iY;
	};
	
CSession2* CDllWsdServer::NewSessionL(const TVersion& /*aVersion*/,const RMessage2& /*aMessage*/) const
	{
	iCount++;
	return new(ELeave) CDllWsdSession;
	}
	
TInt SlaveMain()
	{
	TName name;
	User::CommandLine(name);
	
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if (!cleanup)
		return KErrNoMemory;
	
	TRAPD(err, 
		{
		CActiveScheduler* sched=new(ELeave) CActiveScheduler;
		CActiveScheduler::Install(sched);

		CDllWsdServer* server = new(ELeave) CDllWsdServer;
		server->StartL(name);

		RProcess::Rendezvous(KErrNone);
		CActiveScheduler::Start();
		
		delete server;
		delete sched;
		});
	delete cleanup;

	return err;
	}

//
// Master test controller
//

class RDllWsd : public RSessionBase
	{
public:
	RDllWsd(const TDesC& aServerName, const TDesC& aExeName = _L("t_dllwsddi"))
		{
		test.Start(_L("RDllWsd create"));
		RProcess proc;
		test_KErrNone(proc.Create(aExeName, aServerName));
		TRequestStatus req;
		proc.Rendezvous(req);
		proc.Resume();
		User::WaitForRequest(req);
		test_KErrNone(req.Int());
		test_KErrNone(CreateSession(aServerName, TVersion()));
		proc.Close();
		test.End();
		}
	TInt ConsistencyTest()
		{
		return SendReceive(ETestFuncTestCons);
		}
	void ThrashTest1(TRequestStatus& aStatus)
		{
		SendReceive(ETestFuncThrash1, aStatus);
		}
	TInt IpcTest()
		{
		return SendReceive(ETestFuncIpcTest);
		}
	TInt Panic()
		{
		return SendReceive(ETestFuncPanic);
		}
	};

void BasicTest()
	{
	test.Start(_L("BasicConsistency"));

	// create a test server/process for each link variant
	RDllWsd slaves[] =
		{
		RDllWsd(_L("slave1"), _L("t_dllwsd")),
		RDllWsd(_L("slave2"), _L("t_dllwsdd")),
		RDllWsd(_L("slave3"), _L("t_dllwsdi")),
		RDllWsd(_L("slave4"), _L("t_dllwsddi")),
		RDllWsd(_L("slave5"), _L("t_dllwsd")),
		RDllWsd(_L("slave6"), _L("t_dllwsdd")),
		RDllWsd(_L("slave7"), _L("t_dllwsdi")),
		RDllWsd(_L("slave8"), _L("t_dllwsddi"))
		};
	TInt nSlaves = sizeof(slaves)/sizeof(slaves[0]);
	TInt ii;
	// do this a few times
	for (TInt jj=0; jj<10; jj++)
		{
		// all four test variants
		for (ii=0; ii<nSlaves; ii++)
			{
			// repeat the test different numbers of times, to ensure WSD values diverge
			for (TInt kk=0; kk<ii+2; kk++)
				{
				// change order in which processes run the tests
				int idx = (ii + jj) % nSlaves;
				test_KErrNone(slaves[idx].ConsistencyTest());
				}
			}
		// start and stop an extra process
		RDllWsd extra(_L("slave9"), _L("t_dllwsddi"));
		test_KErrNone(extra.ConsistencyTest());
		extra.Close();
		}

	for (ii=nSlaves-1; ii>=0; ii--)
		slaves[ii].Close();

	test.End();
	}

void ThrashTest1()
	{
	test.Start(_L("ThrashTest1"));

	// create a test server/process for each link variant
	RDllWsd slaves[4] =
		{
		RDllWsd(_L("slaveA"), _L("t_dllwsd")),
		RDllWsd(_L("slaveB"), _L("t_dllwsdd")),
		RDllWsd(_L("slaveC"), _L("t_dllwsdi")),
		RDllWsd(_L("slaveD"), _L("t_dllwsddi"))
		};
	
	TRequestStatus req[4];
	TInt ii;
	// start the thrash tests
	for (ii=0; ii<4; ii++)
		{
		slaves[ii].ThrashTest1(req[ii]);
		test.Printf(_L("slave %d thrash started\n"), ii);
		}

	// show some progress to indicate that things are running		
	for (ii=0; ii<8; ii++)
		{
		test.Printf(_L("Waiting %d\n"), ii);
		User::After(1000000);
		}
	// demonstrate that test processes are still doing their stuff
	test.Printf(_L("Still a couple of seconds to wait...\n"));

	// wait till the test process are done
	for (ii=0; ii<4; ii++)
		{
		User::WaitForRequest(req[ii]);
		// show how much each process did
		test.Printf(_L("Slave %d count = %d\n"), ii, req[ii].Int());
		test_NotNegative(req[ii].Int());
		}
		
	for (ii=3; ii>=0; ii--)
		slaves[ii].Close();

	test.End();
	}

void PanicTest()
	{
	test.Start(_L("PanicTest1"));

	// create a test server/process for each link variant
	RDllWsd slaves[4] =
		{
		RDllWsd(_L("slaveP1"), _L("t_dllwsd")),
		RDllWsd(_L("slaveP2"), _L("t_dllwsdd")),
		RDllWsd(_L("slaveP3"), _L("t_dllwsdi")),
		RDllWsd(_L("slaveP4"), _L("t_dllwsddi"))
		};
	TInt ii;
	for (ii=0; ii<4; ii++)
		slaves[ii].Panic();
		
	for (ii=0; ii<4; ii++)
		slaves[ii].Close();
	}

void IpcTest()
	{
	test.Start(_L("IPC test"));
	// these two processes will use t_dllwsddi, static link variant
	RDllWsd server(_L("IpcTestServer"));
	RDllWsd client(_L("IpcTestClient"));
	// client will talk to IpcTestServer, ie the server
	test_KErrNone(client.IpcTest());
	client.Close();
	server.Close();
	test.End();
	}
	
TInt MasterMain()
	{
	test.Title();
	test.Start(_L("Test"));

	BasicTest();
	ThrashTest1();
	IpcTest();
//	PanicTest();

	test.End();
	return KErrNone;
	}

TInt E32Main()
	{
	if (User::CommandLineLength() > 0)	// command line contains server name
		return SlaveMain();
	else
		return MasterMain();
	}

