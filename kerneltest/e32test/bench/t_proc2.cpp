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
// e32test\bench\t_proc2.cpp
// The other half of the process relative type test stuff
// 
//
										
#include <e32test.h>
#include "t_proc.h"

RTest test(_L("T_PROC2"));
RTest testSvr(_L("Server"));

void CMySession::DisplayName(const RMessage2& aMessage, const TDesC& aText)
//
// Display the client's name.
//
	{
	RThread client;
	TInt r = aMessage.Client(client);
	test(r == KErrNone);
	TFullName name = client.FullName();
	client.Close();
	testSvr.Printf(_L("Session %S\n%S\n"), &name, &aText);
	}

void CMySession::ServiceL(const RMessage2& aMessage)
//
// Handle messages for this server.
//
	{

	TInt r=KErrNone;
	TBuf<0x100> b;
	switch (aMessage.Function())
		{
	case CMyServer::EDisplay:
		TRAP(r,aMessage.ReadL(0,b));
		DisplayName(aMessage, b);
		break;
	case CMyServer::ERead:
		TRAP(r,aMessage.ReadL(0,b));
		if (r==KErrNone && b!=_L("Testing read"))
			r=KErrGeneral;
		if (r==KErrNone)
			{
			TRAP(r,aMessage.ReadL(0,b,-1));
			if (r==KErrNone)
				r=KErrGeneral;
			if (r==KErrArgument)
				r=KErrNone;
			}
		if (r==KErrNone)
			{
			TInt i = TInt(0xfefefefe);
			TRAP(r,aMessage.ReadL(i,b));
			if (r==KErrNone)
				r=KErrGeneral;
			if (r==KErrBadDescriptor)
				r=KErrNone;
			}
		break;
	case CMyServer::EWrite:
		TRAP(r,aMessage.WriteL(0,_L("It worked!")));
		if (r==KErrNone)
			{
			TRAP(r,aMessage.WriteL(0,b,-1));
			if (r==KErrNone)
				r=KErrGeneral;
			if (r==KErrArgument)
				r=KErrNone;
			}
		if (r==KErrNone)
			{
			TRAP(r,aMessage.WriteL(1,b));
			if (r==KErrNone)
				r=KErrGeneral;
			if (r==KErrBadDescriptor)
				r=KErrNone;
			}
		break;
	case CMyServer::ETest:
		break;
	case CMyServer::EStop:
		CActiveScheduler::Stop();
		break;
	default:
		r=KErrNotSupported;
		}
	aMessage.Complete(r);
	}

CMyServer* CMyServer::New(TInt aPriority)
//
// Create a new CMyServer.
//
	{

	return new CMyServer(aPriority);
	}

CMyServer::CMyServer(TInt aPriority)
//
// Constructor.
//
	: CServer2(aPriority)
	{}

CSession2* CMyServer::NewSessionL(const TVersion& aVersion, const RMessage2&) const
//
// Create a new client for this server.
//
	{

	TVersion v(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	if (!User::QueryVersionSupported(v,aVersion))
		User::Leave(KErrNotSupported);
	return(new(ELeave) CMySession());
	}

void CMyActiveScheduler::Error(TInt anError) const
//
// Called if any Run() method leaves.
//
	{

	testSvr.Panic(anError,_L("CMyActiveScheduler::Error"));
	}

LOCAL_C TInt serverThreadEntryPoint(TAny*)
//
// The entry point for the producer thread.
//
	{

//	testSvr.Title();
//	testSvr.Start(_L("Create CActiveScheduler"));
	CMyActiveScheduler* pR=new CMyActiveScheduler;
//	testSvr(pR!=NULL);
	CActiveScheduler::Install(pR);
//
//	testSvr.Next(_L("Create CMyServer"));
	CMyServer* pS=CMyServer::New(0);
//	testSvr(pS!=NULL);
//
//	testSvr.Next(_L("Start CMyServer"));

	pS->Start(KServerName);

//	TInt r=pS->Start();
//	testSvr(r==KErrNone);
//
//	testSvr.Next(_L("Signal to client that we have started"));
//	client.Signal();
//
//	testSvr.Next(_L("Start CActiveScheduler"));
	CActiveScheduler::Start();
//
//	testSvr.Next(_L("Exit server"));
//	testSvr.Close();

	return(KErrNone);
	}


void init()
	{
	// create server thread
	RThread server;
	TInt r=server.Create(_L("Server"),serverThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	server.SetPriority(EPriorityMore);
	server.Resume();

	r=globSem1.OpenGlobal(_L("GlobSem1"));
	test(r==KErrNone);
	r=globSem2.OpenGlobal(_L("GlobSem2"));
	test(r==KErrNone);
	globSem1.Signal(); // finished init
	}

void sharedChunks()
	{

	globSem2.Wait(); // wait for chunk test to be set up

	RChunk c;
	TInt r=c.OpenGlobal(_L("Marmalade"),EFalse);
	test(r==KErrNone);
    test.Printf(_L("Chunk opened\r\n"));
	TUint8* base=c.Base();
    test.Printf(_L("Chunk address %x\r\n"),base);
	for (TInt8 i=0;i<10;i++)
		test(*base++==i); // check the chunk has 0-9
    test.Printf(_L("Chunk contents tested\r\n"));
  	c.Close();
	globSem1.Signal(); // say we've done it
	}

TInt sharedChunks2(TAny* /*aDummy*/)
	{

    RTest test(_L("Shared Chunks 2"));

	globSem2.Wait(); // wait for chunk test to be set up

	RChunk c;
	TInt r=c.OpenGlobal(_L("Marmalade"),EFalse);
	test(r==KErrNone);
    test.Printf(_L("Chunk opened\r\n"));
	TUint8* base=c.Base();
    test.Printf(_L("Chunk address %x\r\n"),base);
	for (TInt8 i=0;i<10;i++)
		test(*base++==i); // check the chunk has 0-9
    test.Printf(_L("Chunk contents tested\r\n"));
  	c.Close();
	globSem1.Signal(); // say we've done it
    return(KErrNone);
	}

TInt E32Main()
	{	
 
	test.Title();

	test.Start(_L("Testing process stuff 2"));

	test.Next(_L("Check that T_PROC1 is running"));
	TFindProcess fProcess(_L("*T_PROC1*"));
	TFullName n;
	TInt r=fProcess.Next(n);
	if (r!=KErrNone)
		{
		test.Printf(_L("This test should not be run from the command line.\n"));
		test.Printf(_L("Run T_PROC1, which loads this test.\n"));
		test.End();
		test.Close();
		return (KErrNone);
		}

	test.Next(_L("Initialize"));
	init();

	test.Next(_L("Shared chunks from primary thread"));
	sharedChunks();
	
	test.Next(_L("Shared chunks from secondary thread"));
	RThread t;
	r=t.Create(_L("Shared chunks 2"),sharedChunks2,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
    test(r==KErrNone);
	TRequestStatus s;
	t.Logon(s);
	t.Resume();
	User::WaitForRequest(s);
	test(s==KErrNone);
	CLOSE_AND_WAIT(t);

	test.Close();
	globSem1.Signal(); // tell proc1 I'm ready

//	test.Next(_L("Wait for first process to do speed tests"));
	globSem2.Wait();
	
	return(KErrNone);
	}

