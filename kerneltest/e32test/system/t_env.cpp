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
// e32test\system\t_env.cpp
// Overview:
// Test RProcess parameters
// API Information:
// RProcess
// Details:
// - Create a thread that causes a variety of panics. Verify that the exit
// reason is as expected.
// - Create a process that causes a variety of panics. Verify that the results
// are as expected.
// - Test passing 16 bit and 8 bit descriptors to another process. Verify the 
// text and other results are as expected.
// - Verify that an invalid read of a descriptor by a process causes a panic.
// - Test passing zero length data to a separate process works as expected.
// - Test that passing each of a mutex, semaphore, file handle and chunk to a 
// separate process works as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32test.h>
#include <e32panic.h>
#include <e32msgqueue.h>
#include <d32comm.h>
#include <f32file.h>

#if defined (__WINS__)
#define PDD_NAME _L("ECDRV.PDD")
#define LDD_NAME _L("ECOMM.LDD")
#else
#define PDD_NAME _L("EUART")
#define LDD_NAME _L("ECOMM")
#endif


LOCAL_D RTest test(_L("T_ENV"));


_LIT(KProcName, "t_env_child.exe");
_LIT(KThreadName, "t_env_panic_thread");
_LIT(KQueueName, "testqueue");
_LIT(KMutexName, "testmutex");
_LIT(KSemName, "testsemaphore");
_LIT(KChunkName, "testchunk");
_LIT(KFileName, "c:\\testfile");

const TInt KHeapSize=0x2000;



class TData 
	{
public:
	TData(TInt aTest, RProcess& aProcess);
	TInt iTest;
	RProcess& iProcess;
	};

TData::TData(TInt aTest, RProcess&  aProcess) :	iTest(aTest), iProcess(aProcess)
	{
	//empty
	};


class Handles
	{
	public:
	void SetupParameters(RProcess& aNewProcess);
	Handles();
	~Handles();
	void Command(TInt aCommand);

	public:
	RMsgQueue<TInt> iCommandQueue;
	RMsgQueue<TInt> iIntQueue;
	RMutex iMutex;
	RSemaphore iSem;
	RChunk iChunk;
	RFile iFile;
	RFs iSession;
	};

void Handles::Command(TInt aC)
	{
	iCommandQueue.SendBlocking(aC);
	}

void Handles::SetupParameters(RProcess& aNewProcess)
	{
	aNewProcess.SetParameter(1, iCommandQueue);
	aNewProcess.SetParameter(2, iIntQueue);
	aNewProcess.SetParameter(3, iMutex);
	aNewProcess.SetParameter(4, iSem);
	aNewProcess.SetParameter(5, iChunk);
	aNewProcess.SetParameter(7, iSession);
	aNewProcess.SetParameter(8, iFile);
	}



_LIT8(KTestData,"test data");

Handles::Handles()
	{
	TInt ret = iCommandQueue.CreateGlobal(KNullDesC, 10, EOwnerProcess);
	test(ret == KErrNone);

	ret = iIntQueue.CreateGlobal(KQueueName,1);
	test(ret == KErrNone);
	
	ret = iMutex.CreateGlobal(KMutexName);
	test(ret == KErrNone);

	ret = iSem.CreateGlobal(KSemName,0);
	test(ret == KErrNone);

	ret = iChunk.CreateGlobal(KChunkName, 1024, 2048);
	test(ret == KErrNone);

	ret = iSession.Connect();
	test(ret == KErrNone);

	ret = iSession.ShareProtected();
	test(ret == KErrNone);

	ret = iFile.Open(iSession, KFileName, EFileStreamText|EFileWrite|EFileShareAny); 
	if (ret == KErrNotFound) // file does not exist - create it 
		ret = iFile.Create(iSession, KFileName, EFileStreamText|EFileWrite|EFileShareAny);
	test(ret == KErrNone);
	ret = iFile.Write(0, KTestData);
	test(ret == KErrNone);
	}

Handles::~Handles()
	{
	iCommandQueue.Close();
	iIntQueue.Close();
	iMutex.Close();
	iSem.Close();
	iChunk.Close();
	iSession.Close();
	}


LOCAL_C TInt testSetParameterPanics(TAny* aData)
	{
	const TData* data = (const TData*)aData;
	switch (data->iTest)
		{
		case 0:	//try and pass a non local handle
			{
			RMsgQueue<TInt> localMsgQueue;
			localMsgQueue.CreateLocal(1);
			data->iProcess.SetParameter(1, localMsgQueue);	//should panic with plat security panic
			break;
			}
		
		case 1:	//out of range slot
			{
			RMsgQueue<TInt> globalMsgQueue;
			globalMsgQueue.CreateGlobal(KNullDesC, 1);
			data->iProcess.SetParameter(-1, globalMsgQueue);	//should panic with range error
			break;
			}

		case 2:
			{
			RMsgQueue<TInt> globalMsgQueue;
			globalMsgQueue.CreateGlobal(KNullDesC, 1);
			data->iProcess.SetParameter(1234, globalMsgQueue);	//should panic with range error
			break;
			}
		
		case 3:	//in use 
			{
			RMsgQueue<TInt> globalMsgQueue;
			globalMsgQueue.CreateGlobal(KNullDesC, 1);
			data->iProcess.SetParameter(1, globalMsgQueue);
			data->iProcess.SetParameter(1, globalMsgQueue);	//panic, in use
			break;
			}

		case 4:
			{
			TPtrC8 bad((const TUint8*)0xfeed,4);
			data->iProcess.SetParameter(1, bad);
			break;
			}

		case 5:	//slot 0 is for the command line
			{
			RMsgQueue<TInt> globalMsgQueue;
			globalMsgQueue.CreateGlobal(KNullDesC, 1);
			data->iProcess.SetParameter(0, globalMsgQueue);	//panic, in use
			break;
			}

		}
	return KErrNone;
	}




GLDEF_C TInt E32Main()
    {

	test.Title();
	test.Start(_L("Process Parameters"));

	test.Next(_L("Test Panics on a set"));

	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);
	TInt i;
	RProcess p;
	TInt ret = p.Create(KProcName, KProcName);
	test(ret == KErrNone);
	for (i = 0; i < 6; i++)
		{
		test.Printf(_L("panic test number %d\n"), i);
		TData data(i, p);
		RThread thread;
		ret = thread.Create(KThreadName, testSetParameterPanics, KDefaultStackSize, KHeapSize, KHeapSize, &data);
		test(KErrNone == ret);
		TRequestStatus stat;
		thread.Logon(stat);
		thread.Resume();
		User::WaitForRequest(stat);
		test.Printf(_L("exit type is %d, stat is %d"), thread.ExitType(), stat.Int());
		test (thread.ExitType() == EExitPanic);
		switch (i)
			{
			case 0:
			test (thread.ExitReason() == EPlatformSecurityTrap);
			break;

			case 1:
			case 2:
			test (thread.ExitReason() == EParameterSlotRange);
			break;

			case 3:
			case 5:
			test (thread.ExitReason() == EParameterSlotInUse);
			break;

			case 4:
			test (thread.ExitReason() == ECausedException);
			break;
			}
		CLOSE_AND_WAIT(thread);
		}
	p.Kill(0);	
	CLOSE_AND_WAIT(p);
	
	test.Next(_L("launch panicing process"));

	
	Handles h;	
	
	TRequestStatus stat;
	for (i = 0; i < 1; i++)
		{
		h.Command(i);
		ret = p.Create(KProcName, KNullDesC);
		test(ret == KErrNone);
		p.Logon(stat);
		h.SetupParameters(p);
		p.Resume();
		User::WaitForRequest(stat);
		test(p.ExitType()==EExitPanic);
		test(p.ExitReason()==EParameterSlotRange);
		CLOSE_AND_WAIT(p);
		}
	User::SetJustInTime(jit);

	test.Next(_L("test 16 bit descriptor"));
	h.Command(8);
	ret = p.Create(KProcName, KNullDesC);
	test(ret == KErrNone);
	p.Logon(stat);
	h.SetupParameters(p);
	p.SetParameter(15, _L("16 bit text"));
	p.Resume();
	User::WaitForRequest(stat);
	test(p.ExitType()==EExitKill);
	test(p.ExitReason()==KErrNone);
	CLOSE_AND_WAIT(p);

	test.Next(_L("test 8 bit descriptor"));
	h.Command(9);
	ret = p.Create(KProcName, KNullDesC);
	test(ret == KErrNone);
	p.Logon(stat);
	h.SetupParameters(p);
	p.SetParameter(15, _L8("8 bit text"));
	p.Resume();
	User::WaitForRequest(stat);
	test(p.ExitType()==EExitKill);
	test(p.ExitReason()==KErrNone);
	CLOSE_AND_WAIT(p);


	test.Next(_L("test bad read of descriptor"));
	h.Command(10);
	ret = p.Create(KProcName, KNullDesC);
	test(ret == KErrNone);
	p.Logon(stat);
	h.SetupParameters(p);
	p.SetParameter(15, _L8("aa"));
	p.Resume();
	User::WaitForRequest(stat);
	test(p.ExitType()==EExitPanic);
	test(p.ExitReason()==ECausedException);
	CLOSE_AND_WAIT(p);

	test.Next(_L("test zero length data"));
	h.Command(11);
	ret = p.Create(KProcName, KNullDesC);
	test(ret == KErrNone);
	p.Logon(stat);
	h.SetupParameters(p);
	p.SetParameter(15, KNullDesC);
	p.Resume();
	User::WaitForRequest(stat);
	test(p.ExitType()==EExitKill);
	test(p.ExitReason()==KErrNone);
	CLOSE_AND_WAIT(p);

	test.Next(_L("test reserved command line"));
	h.Command(12);
	ret = p.Create(KProcName, KNullDesC);
	test(ret == KErrNone);
	p.Logon(stat);
	h.SetupParameters(p);
	p.Resume();
	User::WaitForRequest(stat);
	test(p.ExitType()==EExitKill);
	test(p.ExitReason()==KErrNone);
	CLOSE_AND_WAIT(p);


	test.Next(_L("test mutex"));
	h.Command(4);
	ret = p.Create(KProcName, KNullDesC);
	test(ret == KErrNone);
	p.Logon(stat);
	h.SetupParameters(p);
	p.Resume();
	User::WaitForRequest(stat);
	test(p.ExitType()==EExitKill);
	test(p.ExitReason()==KErrNone);
	CLOSE_AND_WAIT(p);

	
	test.Next(_L("test semaphore"));
	h.Command(5);
	ret = p.Create(KProcName, KNullDesC);
	test(ret == KErrNone);
	p.Logon(stat);
	h.SetupParameters(p);
	p.Resume();
	User::WaitForRequest(stat);
	test(p.ExitType()==EExitKill);
	test(p.ExitReason()==KErrNone);
	CLOSE_AND_WAIT(p);

	test.Next(_L("test file handle"));
	h.Command(6);
	ret = p.Create(KProcName, KNullDesC);
	test(ret == KErrNone);
	p.Logon(stat);
	h.SetupParameters(p);
	p.Resume();
	User::WaitForRequest(stat);
	test(p.ExitType()==EExitKill);
	test(p.ExitReason()==KErrNone);
	CLOSE_AND_WAIT(p);

	test.Next(_L("test chunk"));
	h.Command(7);
	ret = p.Create(KProcName, KNullDesC);
	test(ret == KErrNone);
	p.Logon(stat);
	h.SetupParameters(p);
	p.Resume();
	User::WaitForRequest(stat);
	test(p.ExitType()==EExitKill);
	test(p.ExitReason()==KErrNone);
	CLOSE_AND_WAIT(p);

	test.End();
	return 0;
    }
