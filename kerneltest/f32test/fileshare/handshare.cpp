// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\loader\handshare.cpp
// 
//

#include <e32svr.h>
#define __E32TEST_EXTENSION__
#include <e32test.h>
#include "handshare.h"

#ifdef __VC32__
#pragma warning(disable:4706)
#endif



GLDEF_D RTest test(_L("HANDSHARE_SVR"));

#define PANIC()		FHSvrPanic(__LINE__)
#define FHS_ASSERT(c)	((void)((c)||(PANIC(),0)))

const TTimeIntervalMicroSeconds32 KHalfSecond(500000);

const TInt KHeapSize=0x2000;

void FHSvrPanic(TInt aLine)
	{
	User::Panic(_L("FHServer"),aLine);
	}

LOCAL_D TInt gTestDrive;

/******************************************************************************
 * Class Definitions
 ******************************************************************************/


class CFHServer : public CServer2
	{
public:
	static CFHServer* NewL();
	void ConstructL();
	virtual ~CFHServer();
	virtual CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const;
	virtual TInt RunError(TInt aError);
private:
	CFHServer();
	};

class CFHSession : public CSession2
	{
public:
	virtual ~CFHSession();
	virtual void CreateL();
	virtual void ServiceL(const RMessage2& aMessage);
public:
	
	void GetFileHandle(const RMessage2& aMsg);
	void GetFileHandle2(const RMessage2& aMsg);
	void PassFileHandle(const RMessage2& aMsg);
	void PassFileHandleProcess(const RMessage2& aMsg);
	void PassInvalidFileHandle(const RMessage2& aMsg);
	};

// a second server so we can test passing file handles from 
// client to server to server2
class CFHServer2 : public CServer2
	{
public:
	static CFHServer2* NewL();
	void ConstructL();
	virtual ~CFHServer2();
	virtual CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const;
	virtual TInt RunError(TInt aError);
private:
	CFHServer2();
public:
	};


class CFHSession2 : public CSession2
	{
public:
	virtual ~CFHSession2();
	virtual void CreateL();
	virtual void ServiceL(const RMessage2& aMessage);
public:
	void PassFileHandle(const RMessage2& aMsg);
	void GetFileHandle(const RMessage2& aMsg);
	};


/******************************************************************************
 * Class CFHSession/CFHServer
 ******************************************************************************/
void ExceptionHandler(TExcType)
	{
	User::Leave(KErrGeneral);
	}



CFHSession::~CFHSession()
	{
	}

void CFHSession::CreateL()
	{

	}

void CFHSession::ServiceL(const RMessage2& aMessage)
	{
	__UHEAP_MARK;
	TInt mid=aMessage.Function();
	switch(mid)
		{
		case RFileHandleSharer::EMsgGetFileHandle:
			GetFileHandle(aMessage);
			break;

		case RFileHandleSharer::EMsgGetFileHandle2:
			GetFileHandle2(aMessage);
			break;

		case RFileHandleSharer::EMsgPassFileHandle:
			PassFileHandle(aMessage);
			break;
		
		case RFileHandleSharer::EMsgPassFileHandleProcess:
			PassFileHandleProcess(aMessage);
			break;

		case RFileHandleSharer::EMsgPassInvalidFileHandle:
			PassInvalidFileHandle(aMessage);
			break;

		case RFileHandleSharer::EMsgExit:
			{
			// stop server2
			RFileHandleSharer2 handsvr2;
			TInt r=handsvr2.Connect();
			test_KErrNone(r);
			r = handsvr2.Exit();
			test_Value(r, r ==KErrNone || r == KErrServerTerminated);
			handsvr2.Close();

			aMessage.Complete(KErrNone);	

			CActiveScheduler::Stop();
			}
			break;

		case RFileHandleSharer::EMsgSync:
			aMessage.Complete(KErrNone);	
			break;

		case RFileHandleSharer::EMsgDrive:
			gTestDrive=aMessage.Int0();
			aMessage.Complete(KErrNone);	
			break;
		default:
			break;
		}
	__UHEAP_MARKEND;
	}


//
// Returns session and relevant file handle to Client in read mode
// This is to allow the client to test the deprcated function RFile::Adopt()
void CFHSession::GetFileHandle(const RMessage2& aMsg)
	{
	test.Printf(_L("Get file handle"));

	// get the requested file mode
	TInt fileMode = aMsg.Int1();

	RFs fs;
	TInt r=fs.Connect();
	r=fs.CreatePrivatePath(gTestDrive);
	test_KErrNone(r);
	r=fs.SetSessionToPrivate(gTestDrive);
	test_KErrNone(r);
	r=fs.ShareProtected();
	test_KErrNone(r);
	RFile file1;
	r=file1.Create(fs,KSvrFileName,EFileWrite);
	test_Value(r, r ==KErrNone || r==KErrAlreadyExists);
	if (r==KErrAlreadyExists)
		{
		r=file1.Open(fs,KSvrFileName, EFileWrite);
		test_KErrNone(r);
		}
	r=file1.Write(KTestData1());
	test_KErrNone(r);
	file1.Close();

	r=file1.Open(fs,KSvrFileName, fileMode);
	test_KErrNone(r);

	TInt fssh=file1.SubSessionHandle();
	r=aMsg.Write(0, TPckgC<TInt>(fssh));
	test_KErrNone(r);
	aMsg.Complete(fs);
	fs.Close();
	}

//
//	Returns a file handle from server2
//
void CFHSession::GetFileHandle2(const RMessage2& aMsg)
	{
	test.Next(_L("RFile::AdoptFromServer()"));

	// pass the request on to FHServer2 - this will create a file
	// which we can then adopt before returning it to the client

	// get the requested file mode
	TInt fileMode = aMsg.Int1();

	RFileHandleSharer2 handsvr2;
	TInt r = handsvr2.Connect();
	test_KErrNone(r);

	TInt ssh;
	TInt fsh = handsvr2.GetFileHandle(ssh, TFileMode(fileMode));
	test_Value(fsh, fsh >= 0);

	// adopt the file from FHServer2
	RFile file;
	r=file.AdoptFromServer(fsh, ssh);
	test_KErrNone(r);

	test.Next(_L("RFile::TransferToClient()"));

	// transfer the file to the client
	r = file.TransferToClient(aMsg, 0);
	test_KErrNone(r);

	// test we can still use the file
	TInt pos = 0;
	r = file.Seek(ESeekStart, pos);
	test_KErrNone(r);
	TBuf8<100> rbuf;
	r=file.Read(0,rbuf);
	test_KErrNone(r);
	r=rbuf.CompareF(KTestData1());
	test_KErrNone(r);

	handsvr2.Close();

	file.Close();

	RDebug::Print(_L("completed"));	
	}

void CFHSession::PassFileHandle(const RMessage2& aMsg)
//
// Adopts file from test program and tests what it can and can't do
// Uses new AdoptFromClient() API
//	
	{
	test.Next(_L("RFile::AdoptFromClient()"));

	// connect to FHServer2
	RFileHandleSharer2 handsvr2;
	TInt r = handsvr2.Connect();
	test_KErrNone(r);

	RFile file;

	// Message slot 0 is a RFs handle
	// Message slot 1 is a RFile Subsession handle (RFile::SubSessionHandle())
	r = file.AdoptFromClient(aMsg, 0, 1);
	test_KErrNone(r);



	TBuf8<100> rbuf;
	r=file.Read(0,rbuf);
	test_KErrNone(r);
	r=rbuf.CompareF(KTestData());
	test_KErrNone(r);
	r=file.Write(KTestData1());
	test_Value(r, r ==KErrAccessDenied);
	r=file.ChangeMode(EFileWrite);
	test_Value(r, r ==KErrArgument);
	r=file.Rename(_L("\\newname.txt"));
	test_Value(r, r ==KErrPermissionDenied || r==KErrAccessDenied);
//	should try a delete

	// pass the file handle to FHServer2
	test.Next(_L("RFile::TransferToServer()"));

	TIpcArgs ipcArgs;
	file.TransferToServer(ipcArgs, 0, 1);
	r = handsvr2.PassFileHandle(ipcArgs);
	test_KErrNone(r);

	TInt pos = 0;
	r = file.Seek(ESeekStart, pos);
	test_KErrNone(r);
	r=file.Read(0,rbuf);
	test_KErrNone(r);
	r=rbuf.CompareF(KTestData());
	test_KErrNone(r);
	
	file.Close();

	handsvr2.Close();

	aMsg.Complete(KErrNone);
	}

//
// Adopts file from test program and tests what it can and can't do
// Uses new AdoptFromCreator() API
//	
void CFHSession::PassFileHandleProcess(const RMessage2& aMsg)
	{
	test.Next(_L("RFile::AdoptFromCreator()"));

	RFile file;
	TInt r = file.AdoptFromCreator(1, 2);
	test_KErrNone(r);

	TBuf8<100> rbuf;
	r=file.Read(0,rbuf);
	test_KErrNone(r);
	r=rbuf.CompareF(KTestData());
	test_KErrNone(r);

	test.Next(_L("RFile::Rename()"));

	// define a filename in our private path
	RFs fs;
	r=fs.Connect();
	test_KErrNone(r);

	TFileName sessionp;
	fs.SessionPath(sessionp);
	r = fs.MkDirAll(sessionp);
	test_Value(r, r ==KErrNone || r==KErrAlreadyExists);

	r=fs.ShareProtected();
	test_KErrNone(r);

	r=fs.CreatePrivatePath(gTestDrive);
	test_KErrNone(r);
	r=fs.SetSessionToPrivate(gTestDrive);
	test_KErrNone(r);

	TPath newPath;
	fs.PrivatePath(newPath);
	TFileName newFileName;
	newFileName = newPath;
	newFileName.Append(_L("newname.txt"));
	
	// delete the file before we try to rename anything to it
	r = fs.Delete(newFileName);
	test_Value(r, r  == KErrNone || r == KErrNotFound);

	r=file.Rename(newFileName);
	test_KErrNone(r);

	file.Close();

	// Next verify that we can delete the file (which should now 
	// have been moved to our private directory)
	test.Next(_L("RFs::Delete()"));
	r = fs.Delete(newFileName);
	test_KErrNone(r);

	fs.Close();

	
	aMsg.Complete(KErrNone);
	}

void CFHSession::PassInvalidFileHandle(const RMessage2& aMsg)
//
// Attempts to adopt an invalid file handle from test program 
// and tests that KErrBadHandle is returned by AdoptFromClient()
//	
	{
	test.Next(_L("PassInvalidFileHandle - RFile::AdoptFromClient()"));

	RFile file;

	// Message slot 0 is a RFs handle
	// Message slot 1 is a RFile Subsession handle (RFile::SubSessionHandle())
	TInt r = file.AdoptFromClient(aMsg, 0, 1);
	test_Value(r, r ==KErrBadHandle);


	aMsg.Complete(r);
	}

CFHServer* CFHServer::NewL()
	{
	CFHServer* server = new (ELeave) CFHServer;
	CleanupStack::PushL(server);
	server->ConstructL();
	CleanupStack::Pop(server);
	return server;
	}

void CFHServer::ConstructL()
	{
	}

CFHServer::CFHServer()
	: CServer2(0,ESharableSessions)
	{
	}

CFHServer::~CFHServer()
	{
	}

CSession2* CFHServer::NewSessionL(const TVersion& aVersion, const RMessage2&) const
//
//	 Create New Session
//
	{
	(void)aVersion;
	CFHSession* s = new (ELeave) CFHSession;
	return s;
	}

_LIT(KErr,"FHSERVER_ERR");
CFHServer2* CFHServer2::NewL()
	{
	CFHServer2* server = new (ELeave) CFHServer2;
	CleanupStack::PushL(server);
	server->ConstructL();
	CleanupStack::Pop(server);
	return server;
	}

void CFHServer2::ConstructL()
	{
	}

TInt CFHServer::RunError(TInt aError)
	{
	User::Panic(KErr,aError);
	return 0;
	}

// File handle server #2
CFHServer2::CFHServer2()
	: CServer2(0,ESharableSessions)
	{
	}

CFHServer2::~CFHServer2()
	{
	}

CSession2* CFHServer2::NewSessionL(const TVersion& aVersion, const RMessage2&) const
//
//	 Create New Session
//
	{
	(void)aVersion;
	CFHSession2* s = new (ELeave) CFHSession2;
	return s;
	}

_LIT(KErr2,"FHSERVER2_ERR");
TInt CFHServer2::RunError(TInt aError)
	{
	User::Panic(KErr2,aError);
	return 0;
	}

CFHSession2::~CFHSession2()
	{
	}

void CFHSession2::CreateL()
	{

	}

void CFHSession2::ServiceL(const RMessage2& aMessage)
	{
	__UHEAP_MARK;
	TInt mid=aMessage.Function();
	switch(mid)
		{
		case RFileHandleSharer::EMsgPassFileHandle:
			PassFileHandle(aMessage);
			break;

		case RFileHandleSharer::EMsgGetFileHandle:
			GetFileHandle(aMessage);
			break;

		case RFileHandleSharer::EMsgExit:
			aMessage.Complete(KErrNone);	
			CActiveScheduler::Stop();
			break;

		default:
			break;
		}
	__UHEAP_MARKEND;
	}


//
// Adopts file from server 1
//	
void CFHSession2::PassFileHandle(const RMessage2& aMsg)
	{
	RFile file;

	// Message slot 0 is a RFs handle
	// Message slot 1 is a RFile Subsession handle (RFile::SubSessionHandle())
	TInt r = file.AdoptFromClient(aMsg, 0, 1);
	if (r != KErrNone)
		{
		aMsg.Complete(r);
		return;
		}


	TBuf8<100> rbuf;
	
	if (r == KErrNone)
		r=file.Read(0,rbuf);

	if (r == KErrNone)
		r = rbuf.CompareF(KTestData());

	if (r == KErrNone)
		{
		r = file.Write(KTestData1());
		if (r == KErrAccessDenied)
			r = KErrNone;
		}

	if (r == KErrNone)
		{
		r = file.ChangeMode(EFileWrite);
		if (r == KErrArgument)
			r = KErrNone;
		}

	if (r == KErrNone)
		{
		r = file.Rename(_L("\\newname.txt"));
		if (r == KErrPermissionDenied || r == KErrAccessDenied)
			r = KErrNone;
		}

	file.Close();

	aMsg.Complete(r);
	}


void CFHSession2::GetFileHandle(const RMessage2& aMsg)
//
//	Returns a file handle in write mode
//
	{
	RFs fs;
	TInt r = fs.Connect();

	if (r == KErrNone)
		r = fs.CreatePrivatePath(gTestDrive);

	if (r == KErrNone)
		r = fs.SetSessionToPrivate(gTestDrive);

	if (r == KErrNone)
		r = fs.ShareProtected();

	// make sure file exists & has valid data in it
	RFile file1;
	if (r == KErrNone)
		r = file1.Replace(fs,KSvrFileName,EFileWrite);

	if (r == KErrNone)
		r = file1.Write(KTestData1());
		
	file1.Close();


	// re-open the file with the mode the client has requested & pass it to the client
	TFileMode fileMode = TFileMode(aMsg.Int1());
	if (r == KErrNone)
		r = file1.Open(fs,KSvrFileName, fileMode);
	if (r == KErrNone)
		r = file1.TransferToClient(aMsg, 0);
	file1.Close();
	
	fs.Close();

	if (r != KErrNone)
		aMsg.Complete(r);
	}

LOCAL_C TInt FHServer2(TAny * /*anArg*/)
	{	
	RTest test(_L("FHServer2"));

	// Remember the number of open handles. Just for a sanity check ....
	TInt start_thc, start_phc;
	RThread().HandleCount(start_phc, start_thc);


	CTrapCleanup* cleanup;
	cleanup=CTrapCleanup::New();

	CActiveScheduler* sched=new CActiveScheduler;
	FHS_ASSERT(sched);
	CActiveScheduler::Install(sched);

	CFHServer2* svr2 = NULL;
	TRAP_IGNORE(svr2 = CFHServer2::NewL());
	FHS_ASSERT(svr2);
	FHS_ASSERT(svr2->Start(_L("FHServer2"))==KErrNone);

	CActiveScheduler::Start();
	
	delete svr2;
	delete sched;

	delete cleanup;

	// Sanity check for open handles
	TInt end_thc, end_phc;
	RThread().HandleCount(end_phc, end_thc);
	test_Value(start_thc, start_thc == end_thc);
//	test(start_phc == end_phc);
	// and also for pending requests ...
	test_Value(RThread().RequestCount(), RThread().RequestCount() == 0);

	
	return KErrNone;
	}



GLDEF_C TInt E32Main()
//
// Test Server for file handle sharing
//
	{
	test.Title();
	test.Start(_L("Starting FHServer..."));

	// Remember the number of open handles. Just for a sanity check ....
	TInt start_thc, start_phc;
	RThread().HandleCount(start_phc, start_thc);

	CTrapCleanup* cleanup=CTrapCleanup::New();

	FHS_ASSERT(cleanup);
	CActiveScheduler* sched=new CActiveScheduler;
	FHS_ASSERT(sched);
	CActiveScheduler::Install(sched);

	// start server1
	CFHServer* svr = NULL;
	TRAP_IGNORE(svr = CFHServer::NewL());
	FHS_ASSERT(svr);
	FHS_ASSERT(svr->Start(_L("FHServer"))==KErrNone);

	test.Title();
	test.Start(_L("Starting tests..."));

	// start server2 in a seperate thread
	RThread server2Thread;
	TInt r = server2Thread.Create(_L("FHServer2"), FHServer2, KDefaultStackSize, KHeapSize, KHeapSize, NULL);	
	test_KErrNone(r);	
	TRequestStatus statq;
	server2Thread.Logon(statq);
	server2Thread.Resume();

	CActiveScheduler::Start();


	// wait for server2's thread to end gracefully
	User::WaitForRequest(statq);
	test_KErrNone(statq.Int());

	server2Thread.Close();

	RFs cleanupfs;
	r = cleanupfs.Connect();
	test_KErrNone(r);
	r=cleanupfs.SetSessionToPrivate(gTestDrive);
	test_KErrNone(r);
	r=cleanupfs.Delete(KSvrFileName);
	test_Value(r, r ==KErrNone || r==KErrNotFound);
	cleanupfs.Close();


	test.End();

	delete svr;
	delete sched;
	delete cleanup;

	// Sanity check for open handles and pending requests
	TInt end_thc, end_phc;
	RThread().HandleCount(end_phc, end_thc);
	test_Value(start_thc, start_thc == end_thc);
	test_Value(start_phc, start_phc == end_phc);
	test_Value(RThread().RequestCount(), RThread().RequestCount() == 0);
	
	return 0;
	}
