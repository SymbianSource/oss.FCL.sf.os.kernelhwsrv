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
// e32test\mmu\t_chunk4.cpp
// Overview:
// Test disconnected chunks
// API Information:
// RChunk, CBase
// Details:
// - Check Allocate/Commit/Decommit methods on local disconnected chunk and write/read 
// access to both committed and uncommitted regions.
// - Check IPC that involves local disconnected chunk and verify results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include "u32std.h"
#include "mmudetect.h"
#include "../misc/prbs.h"
#include "d_memorytest.h"
#include "freeram.h"

RTest test(_L("T_CHUNK4"));

RMemoryTestLdd TestLdd;

TUint RndSeed[2];

class CChunk : public CBase
	{
public:
	static CChunk* New(TInt aMaxSize);
public:
	virtual ~CChunk();
	TInt Verify();
	TInt Commit(TInt anOffset, TInt aSize);
	TInt Allocate(TInt aSize);
	TInt Decommit(TInt anOffset, TInt aSize);
	void CheckL(volatile TUint* aPtr);
	TInt AddPages(TInt anOffset, TInt aSize);
	TInt RemovePages(TInt anOffset, TInt aSize);
public:
	RChunk iChunk;
	TUint8* iPageInfo;
	TInt iPageSize;
	TInt iMaxSize;
	TInt iNumPages;
	};

CChunk* CChunk::New(TInt aMaxSize)
	{
	CChunk* pC=new CChunk;
	if (pC)
		{
		TInt p;
		UserHal::PageSizeInBytes(p);
		pC->iPageSize=p;
		pC->iMaxSize=aMaxSize;
		TInt n=aMaxSize/p;
		pC->iNumPages=n;
		TInt r=pC->iChunk.CreateDisconnectedLocal(0,0,aMaxSize);
		if (r==KErrNone)
			{
			TUint8* pI=(TUint8*)User::Alloc(n);
			if (pI)
				{
				pC->iPageInfo=pI;
				Mem::FillZ(pI,n);
				}
			else
				r=KErrNoMemory;
			}
		if (r!=KErrNone)
			{
			delete pC;
			pC=NULL;
			}
		}
	return pC;
	}

CChunk::~CChunk()
	{
	delete iPageInfo;
	iChunk.Close();
	}

void CChunk::CheckL(volatile TUint* aPtr)
	{
	TUint x=*aPtr;
	*aPtr=x;
	}

TInt CChunk::Verify()
	{
//	test.Getch();
	TInt i;
	TUint8* base=iChunk.Base();
	for (i=0; i<iNumPages; i++)
		{
		volatile TUint* pX=(volatile TUint*)base;
		TInt r=TestLdd.ReadWriteMemory((TAny*)pX);
		TUint8 info=iPageInfo[i];
		if (info==0 && r==KErrNone)
			return KErrGeneral;
		if (info!=0 && r!=KErrNone)
			return KErrAccessDenied;
		if (info!=0)
			{
			TUint seed[2];
			seed[0]=info<<8;
			seed[1]=0;
			TInt j;
			for (j=0; j<iPageSize; j+=4)
				{
				if (*pX++!=Random(seed))
					return KErrArgument;
				}
			}
		base+=iPageSize;
		}
	return KErrNone;
	}

TInt CChunk::AddPages(TInt anOffset, TInt aSize)
	{
	TInt i=anOffset/iPageSize;
	TInt n=aSize/iPageSize;
	TInt e=i+n;
	TUint* p=(TUint*)(iChunk.Base()+anOffset);
	for (; i<e; i++)
		{
		TUint8 s=(TUint8)Random(RndSeed);
		if (s==0)
			s=1;
		iPageInfo[i]=s;
		TUint seed[2];
		seed[0]=s<<8;
		seed[1]=0;
		TInt j;
		for (j=0; j<iPageSize; j+=4)
			{
			*p++=Random(seed);
			}
		}
	return KErrNone;
	}

TInt CChunk::RemovePages(TInt anOffset, TInt aSize)
	{
	TInt i=anOffset/iPageSize;
	TInt n=aSize/iPageSize;
	TInt e=i+n;
	for (; i<e; i++)
		iPageInfo[i]=0;
	return KErrNone;
	}

TInt CChunk::Commit(TInt anOffset, TInt aSize)
	{
	TInt r=iChunk.Commit(anOffset,aSize);
	if (r==KErrNone)
		{
		AddPages(anOffset,aSize);
		}
	return r;
	}

TInt CChunk::Allocate(TInt aSize)
	{
	TInt r=iChunk.Allocate(aSize);
	if (r>=0)
		{
		AddPages(r,aSize);
		}
	return r;
	}

TInt CChunk::Decommit(TInt anOffset, TInt aSize)
	{
	TInt r=iChunk.Decommit(anOffset,aSize);
	if (r==KErrNone)
		{
		RemovePages(anOffset,aSize);
		}
	return r;
	}

// Stuff to test remote writes
_LIT(KServerName, "Chunk4Test");

class CTestSession : public CSession2
	{
public:
	CTestSession();
	virtual void ServiceL(const RMessage2& aMessage);
	};

class CTestServer : public CServer2
	{
public:
	CTestServer();
	virtual CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const;
	};

class RTestSession : public RSessionBase
	{
public:
	enum {ETestIpc, ETestStop};
	TInt Connect();
	void Stop();
	TInt TestRemoteWrite(TInt aLength, TInt anOffset1, TInt anOffset2, TInt anOffset3, TInt anOffset4);
	TInt IpcWrite(TDes8* aRemoteDest, const TAny* aLocalSrc, TInt aOffset);
	};

CTestSession::CTestSession()
	{
	}

void CTestSession::ServiceL(const RMessage2& aMessage)
	{
	switch (aMessage.Function())
		{
		case RTestSession::ETestIpc:
			{
			const TDesC8& localSrc = *(const TDesC8*)aMessage.Ptr1();
			TInt offset = aMessage.Int2();
			TInt r = aMessage.Write(0, localSrc, offset);
			aMessage.Complete(r);
			break;
			}
		case RTestSession::ETestStop:
			CActiveScheduler::Stop();
			break;
		default:
			User::Invariant();
		}
	}

CTestServer::CTestServer()
	: CServer2(0)
	{
	}

CSession2* CTestServer::NewSessionL(const TVersion& /*aVersion*/, const RMessage2& /*aMessage*/) const
	{
	return new (ELeave) CTestSession;
	}

TInt ServerThread(TAny*)
	{
	CActiveScheduler* pA = new CActiveScheduler;
	CTestServer* pS = new CTestServer;
	if (!pA || !pS)
		return KErrNoMemory;
	CActiveScheduler::Install(pA);
	TInt r = pS->Start(KServerName);
	if (r!=KErrNone)
		return r;
	RThread::Rendezvous(KErrNone);
	CActiveScheduler::Start();
	return KErrNone;
	}

TInt RTestSession::Connect()
	{
	RThread t;
	TInt r = t.Create(KServerName, ServerThread, 0x1000, 0x1000, 0x10000, NULL);
	test(r==KErrNone);
	t.SetPriority(EPriorityMore);
	TRequestStatus s;
	t.Rendezvous(s);
	test(s==KRequestPending);
	t.Resume();
	User::WaitForRequest(s);
	test(t.ExitType()==EExitPending);
	test(s==KErrNone);
	t.Close();
	r = CreateSession(KServerName, TVersion());
	return r;
	}

void RTestSession::Stop()
	{
	TInt r = SendReceive(ETestStop);
	test(r==KErrServerTerminated);
	Close();
	}

TInt RTestSession::IpcWrite(TDes8* aRemoteDest, const TAny* aLocalSrc, TInt aOffset)
	{
	return SendReceive(ETestIpc, TIpcArgs(aRemoteDest, aLocalSrc, aOffset));
	}

TInt RTestSession::TestRemoteWrite(TInt aLength, TInt anOffset1, TInt anOffset2, TInt anOffset3, TInt anOffset4)
	{
	test.Printf(_L("%x %x %x %x %x\n"),aLength,anOffset1,anOffset2,anOffset3,anOffset4);

	TBool ptr=(anOffset1>=0);
	TDes8* pDes;
	TUint8* pData;
	RChunk c;
	TInt r=c.CreateDisconnectedLocal(0,0,0x800000);
	test(r==KErrNone);
	TUint8* base=c.Base();
	if (ptr)
		{
		r=c.Commit(anOffset1,(TInt)sizeof(TPtr8));
		test(r==KErrNone);
		pDes=(TDes8*)(base+anOffset1);
		Mem::FillZ(pDes,(TInt)sizeof(TPtr8));
		r=c.Commit(anOffset2,aLength);
		test(r==KErrNone);
		pData=base+anOffset2;
		Mem::FillZ(pData,aLength);
		new(pDes) TPtr8(pData,0,aLength);
		test(pDes->Length()==0);
		test(pDes->MaxLength()==aLength);
		test(pDes->Ptr()==pData);
		}
	else
		{
		TInt len=(TInt)sizeof(TDes8)+aLength;
		r=c.Commit(anOffset2,len);
		test(r==KErrNone);
		pDes=(TDes8*)(base+anOffset2);
		Mem::FillZ(pDes,len);
		pData=base+anOffset2+(TInt)sizeof(TDes8);
		new(pDes) TBuf8<1>;
		((TInt*)pDes)[1]=aLength;
		test(pDes->Length()==0);
		test(pDes->MaxLength()==aLength);
		test(pDes->Ptr()==pData);
		}
	TInt slen=aLength-anOffset3;
	TUint8* pS=(TUint8*)User::Alloc(aLength);
	test(pS!=NULL);
	Mem::FillZ(pS,aLength);
	TPtrC8 src(pS+anOffset3,slen);
	TInt i;
	for (i=anOffset3; i<aLength; i++)
		pS[i]=(TUint8)Random(RndSeed);
	if (anOffset4>=0)
		c.Decommit(anOffset4,0x1000);
	r = IpcWrite(pDes, &src, anOffset3);
	if (r==KErrNone)
		{
		TPtrC8 tsrc(pS,aLength);
		if (*pDes!=tsrc)
			r=KErrCorrupt;
		}
	User::Free(pS);
	c.Close();
	test.Printf(_L("Return value %d\n"),r);
	return r;
	}

GLDEF_C TInt E32Main()
	{
	RndSeed[0]=0xddb3d743;
	RndSeed[1]=0;

	test.Title();
	if (!HaveVirtMem())
		{
		test.Printf(_L("This test requires an MMU\n"));
		return KErrNone;
		}
	test.Start(_L("Testing Disconnected Chunks"));

	test.Printf(_L("Load test LDD\n"));
	test(TestLdd.Open()==KErrNone);

	CChunk* pC=CChunk::New(0x800000);
	test(pC!=NULL);
	TInt free=FreeRam();
	test.Printf(_L("Free RAM %08x\n"),free);
	test(pC->Verify()==KErrNone);
	test.Printf(_L("Commit 0+0x1000\n"));
	test(pC->Commit(0,0x1000)==KErrNone);
	test(pC->Verify()==KErrNone);
	test.Printf(_L("Commit 0+0x1000 (again)\n"));
	test(pC->Commit(0,0x1000)==KErrAlreadyExists);
	test(pC->Verify()==KErrNone);
	test.Printf(_L("Commit 0x3000+0x1000\n"));
	test(pC->Commit(0x3000,0x1000)==KErrNone);
	test(pC->Verify()==KErrNone);
	test.Printf(_L("Commit 0x2000+0x3000 (overlaps previous)\n"));
	test(pC->Commit(0x2000,0x3000)==KErrAlreadyExists);
	test(pC->Verify()==KErrNone);
	test.Printf(_L("Allocate 0x5000\n"));
	test(pC->Allocate(0x5000)==0x4000);
	test(pC->Verify()==KErrNone);
	test.Printf(_L("Allocate 0x1000\n"));
	test(pC->Allocate(0x1000)==0x1000);
	test(pC->Verify()==KErrNone);
	test.Printf(_L("Decommit 0+0x4000\n"));
	test(pC->Decommit(0,0x4000)==KErrNone);
	test(pC->Verify()==KErrNone);
	test.Printf(_L("Decommit 0+0x4000 (again)\n"));
	test(pC->Decommit(0,0x4000)==KErrNone);
	test(pC->Verify()==KErrNone);
	test.Printf(_L("Allocate 0x4000\n"));
	test(pC->Allocate(0x4000)==0x0000);
	test(pC->Verify()==KErrNone);
	test.Printf(_L("Decommit 0+0x10000\n"));
	test(pC->Decommit(0,0x10000)==KErrNone);
	test(pC->Verify()==KErrNone);
	test.Printf(_L("Check Free RAM\n"));
	test(FreeRam()==free);

	test.Printf(_L("Commit 0x700000+0x10000\n"));
	test(pC->Commit(0x700000,0x10000)==KErrNone);
	test(pC->Verify()==KErrNone);
	test.Printf(_L("Allocate 0x4000\n"));
	test(pC->Allocate(0x4000)==0x0000);
	test(pC->Verify()==KErrNone);
	test.Printf(_L("Decommit 0+0x800000\n"));
	test(pC->Decommit(0,0x800000)==KErrNone);
	test(pC->Verify()==KErrNone);
	test.Printf(_L("Check Free RAM\n"));
	test(FreeRam()==free);

	delete pC;

	// Test decommiting from a chunk which has no memory commited
	// in the first megabyte. On the moving memory model, such
	// chunks have a non-zero iHomeRegionOffset value and has been
	// the cause of defects (DEF121857)
	test.Printf(_L("Create new chunk\n"));
	pC=CChunk::New(0x800000);
	test(pC!=NULL);
	test.Printf(_L("Commit 0x100000+0x3000\n"));
	test(pC->Commit(0x100000,0x3000)==KErrNone);
	test(pC->Verify()==KErrNone);
	test.Printf(_L("Decommit 0+0x101000\n"));
	test(pC->Decommit(0,0x101000)==KErrNone);
	test(pC->Verify()==KErrNone);
	test.Printf(_L("Decommit 0x101000+0x1000\n"));
	test(pC->Decommit(0x101000,0x1000)==KErrNone);
	test(pC->Verify()==KErrNone);
	test.Printf(_L("Decommit 0x100000+0x100000\n"));
	test(pC->Decommit(0x100000,0x100000)==KErrNone);
	test(pC->Verify()==KErrNone);
	delete pC;

	test.Next(_L("Testing RThread::Write to disconnected chunks"));
	RTestSession ts;
	TInt r = ts.Connect();
	test(r==KErrNone);
	test(ts.TestRemoteWrite(64,0,0x3000,0,-1)==KErrNone);
	test(ts.TestRemoteWrite(64,0xffc,0x8000,0,-1)==KErrNone);
	test(ts.TestRemoteWrite(256,0xffc,0x7f80,0,-1)==KErrNone);
	test(ts.TestRemoteWrite(256,0xffc,0x7f80,128,-1)==KErrNone);
	test(ts.TestRemoteWrite(0x10000,0xffc,0x707f80,0,-1)==KErrNone);
	test(ts.TestRemoteWrite(0x10000,0xffc,0x707f80,0x2000,-1)==KErrNone);
	test(ts.TestRemoteWrite(64,-1,0x3000,0,-1)==KErrNone);
	test(ts.TestRemoteWrite(0x10000,-1,0xfff00,0x2000,-1)==KErrNone);
	test(ts.TestRemoteWrite(256,0xffc,0x7f80,16,0)==KErrBadDescriptor);
	test(ts.TestRemoteWrite(256,0xffc,0x7f80,16,0x1000)==KErrBadDescriptor);
	test(ts.TestRemoteWrite(256,0xffc,0x7f80,16,0x7000)==KErrBadDescriptor);
	test(ts.TestRemoteWrite(256,0xffc,0x7f80,16,0x8000)==KErrBadDescriptor);
	test(ts.TestRemoteWrite(0x10000,0xffc,0x707f80,0x2000,0x710000)==KErrBadDescriptor);
	test(ts.TestRemoteWrite(0x10000,-1,0xfff00,0x1000,0x100000)==KErrBadDescriptor);
	ts.Stop();

	test.End();
	return 0;
	}
