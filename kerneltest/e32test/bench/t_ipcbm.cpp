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
// e32test\bench\t_ipcbm.cpp
// Overview:
// Test and benchmark the data copy in inter-process communication. 
// API Information:
// RServer2, RSessionBase
// Details:
// - Create a server thread and write data blocks of varying sizes from the 
// server to client, increment a count after each write.
// - Create a client session, connect to server and send data block size 
// to the server.
// - Print the block size, total number of bytes written along with usec 
// per block.
// - Verify that the heap was not corrupted by any of the tests.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32def.h>
#include <e32def_private.h>

LOCAL_D RTest test(_L("T_IPCBM"));

const TInt KBufferSize = 65536;

LOCAL_D TUint32* Src;
LOCAL_D TUint32* Dest;
LOCAL_D TInt Count=0;
LOCAL_D RServer2 Server;
LOCAL_D RSemaphore ServerSem;


class RMySession : public RSessionBase
	{
public:
	TInt Connect(RServer2 aSrv,TRequestStatus& aStat)
		{return CreateSession(aSrv,TVersion(),1,EIpcSession_Unsharable,0,&aStat);}
	void Test(TDes8& aDes)
		{Send(0,TIpcArgs(&aDes));}
	};

LOCAL_C TInt TestThread(TAny* aSize)
	{
	TInt r = Server.CreateGlobal(KNullDesC);
	ServerSem.Signal();
	if (r != KErrNone)
		return r;

	RMessage2 m;
	Server.Receive(m);
	m.Complete(KErrNone);	// connect message

	Server.Receive(m);
	TInt size=(TInt)aSize;
	TPtrC8 src((const TUint8*)Src,size);
	FOREVER
		{
		m.Write(0,src);
		Count++;
		}
	}

const TInt KTestRunSeconds = 10;

LOCAL_C void RunTest(TInt aSize)
	{
	const TInt KTestRunUs = KTestRunSeconds * 1000000;

	RThread t;
	TInt r=t.Create(KNullDesC,TestThread,0x1000,NULL,(TAny*)aSize);
	test(r==KErrNone);
	t.SetPriority(EPriorityLess);
	TRequestStatus s;
	t.Logon(s);
	t.Resume();
	ServerSem.Wait();
	test(Server.Handle() != KNullHandle);

	RMySession sess;
	TRequestStatus stat;
	test(sess.Connect(Server,stat) == KErrNone);
	User::WaitForRequest(stat);	// connected

	Count=0;
	TPtr8 des((TUint8*)Dest, 0, aSize);
	sess.Test(des);
	User::After(KTestRunUs);
	t.Kill(0);
	User::WaitForRequest(s);
	sess.Close();
	Server.Close();
	CLOSE_AND_WAIT(t);

	TInt us=10*KTestRunUs/Count;
	test.Printf(_L("%5d byte writes: %8d/%ds %4d.%01dus\n"),aSize,Count,KTestRunSeconds,us/10,us%10);
	}

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Benchmark IPC copy"));

	__KHEAP_MARK;

	TAny* buffer = User::Alloc(2 * KBufferSize + 32);
	test(buffer != 0);

	Src = (TUint32*)((((TInt)buffer) & ~0x1f) + 0x20);	
	Dest = (TUint32*)(((TInt)Src) + KBufferSize);
	
	test(ServerSem.CreateLocal(0) == KErrNone);

	static TInt KMaxCounts[] = { 16, 256, 512, 2048, 2052, 4096, 32768, 65536 };
	for (TUint i=0; i<sizeof KMaxCounts/sizeof KMaxCounts[0]; ++i)
		{
		RunTest(KMaxCounts[i]);
		}

	ServerSem.Close();
	User::Free(buffer);

	__KHEAP_MARKEND;

	test.End();
	return KErrNone;
	}
