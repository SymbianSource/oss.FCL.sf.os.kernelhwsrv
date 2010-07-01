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
// File Name:		f32test/fileshare/t_handshare64bit.cpp
// 64 bit FileHandle Server. Used by t_file64bit for testing
// RFile64::AdoptFromServer() and RFile64::TransferToServer()
// functionality.
// 
//

#define __E32TEST_EXTENSION__
#include <e32svr.h>
#include <e32test.h>
#include "handshare64bit.h"

#ifdef __VC32__
#pragma warning(disable:4706)
#endif



GLDEF_D RTest test(_L("HANDSHARE_SVR"));
const TInt64 KGB  = 1<<30;
const TInt64 K4GB  = 4 * KGB;


#define PANIC()		FHSvrPanic(__LINE__)
#define FHS_ASSERT(c)	((void)((c)||(PANIC(),0)))

const TTimeIntervalMicroSeconds32 KHalfSecond(500000);


void FHSvrPanic(TInt aLine)
	{
	User::Panic(_L("FHServer"),aLine);
	}

LOCAL_D TInt gTestDrive;

/******************************************************************************
 * Class Definitions
 ******************************************************************************/


class CFHServer64Bit : public CServer2
	{
public:
	static CFHServer64Bit* NewL();
	void ConstructL();
	virtual ~CFHServer64Bit();
	virtual CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const;
	virtual TInt RunError(TInt aError);
private:
	CFHServer64Bit();
	};

class CFHSession64Bit : public CSession2
	{
public:
	virtual ~CFHSession64Bit();
	virtual void CreateL();
	virtual void ServiceL(const RMessage2& aMessage);
public:
	
	void GetFileHandleLargeFile2(const RMessage2& aMsg);
	void PassFileHandleLargeFile(const RMessage2& aMsg);
	void PassFileHandleProcessLargeFile(const RMessage2& aMsg);
	};

/******************************************************************************
 * Class CFHSession/CFHServer
 ******************************************************************************/
void ExceptionHandler(TExcType)
	{
	User::Leave(KErrGeneral);
	}



CFHSession64Bit::~CFHSession64Bit()
	{
	}

void CFHSession64Bit::CreateL()
	{

	}

void CFHSession64Bit::ServiceL(const RMessage2& aMessage)
	{
	__UHEAP_MARK;
	TInt mid=aMessage.Function();
	switch(mid)
		{
		case RFileHandleSharer64Bit::EMsgGetFileHandleLargeFile:
			GetFileHandleLargeFile2(aMessage);
			break;

		case RFileHandleSharer64Bit::EMsgPassFileHandleProcessLargeFileClient:
			PassFileHandleLargeFile(aMessage);
			break;

		case RFileHandleSharer64Bit::EMsgPassFileHandleProcessLargeFileCreator:
			PassFileHandleProcessLargeFile(aMessage);
			break;
		
		case RFileHandleSharer64Bit::EMsgExit:
			{
			aMessage.Complete(KErrNone);	

			CActiveScheduler::Stop();
			}
			break;

		case RFileHandleSharer64Bit::EMsgSync:
			aMessage.Complete(KErrNone);	
			break;

		case RFileHandleSharer64Bit::EMsgDrive:
			gTestDrive=aMessage.Int0();
			aMessage.Complete(KErrNone);	
			break;
		default:
			break;
		}
	__UHEAP_MARKEND;
	}

//
//	Returns a file handle from server
//
void CFHSession64Bit::GetFileHandleLargeFile2(const RMessage2& aMsg)
	{
	test.Next(_L("RFile64::AdoptFromServer()"));
	// get the requested file mode
	TFileMode fileMode = TFileMode(aMsg.Int1());

	RFs fs;
	TInt r = fs.Connect();

	if (r == KErrNone)
	r = fs.CreatePrivatePath(gTestDrive);

	if (r == KErrNone)
	r = fs.SetSessionToPrivate(gTestDrive);

	if (r == KErrNone)
	r = fs.ShareProtected();

	// make sure file exists & has valid data in it
	RFile64 file1;
	if (r == KErrNone)
	r = file1.Replace(fs,KServerFileName,EFileWrite);
	r=file1.SetSize(K4GB-1);
	test_KErrNone(r);
	r = file1.Write(K4GB-10,KTestData4());
		
	file1.Close();


	// re-open the file with the mode the client has requested & pass it to the client
	
	if (r == KErrNone)
	r = file1.Open(fs,KServerFileName, fileMode);
	if (r == KErrNone)
	
	test.Next(_L("RFile::TransferToClient()"));

	// transfer the file to the client
	r = file1.TransferToClient(aMsg, 0);
	test_KErrNone(r);

	// test we can still use the file
	TInt64 pos = 0;
	r = file1.Seek(ESeekStart, pos);
	test_KErrNone(r);
	TBuf8<9> rbuf;
	r=file1.Read(K4GB-10,rbuf);
	test_KErrNone(r);
	r=rbuf.CompareF(KTestData4());
	test_KErrNone(r);

	file1.Close();
	fs.Close();
	RDebug::Print(_L("completed"));	
	}


void CFHSession64Bit::PassFileHandleLargeFile(const RMessage2& aMsg)
//
// Adopts file from test program and tests what it can and can't do
// Uses RFile64::AdoptFromClient() API
//	
	{
	test.Next(_L("RFile64::AdoptFromClient()"));
	
	RFile64 file;

	// Message slot 0 is a RFs handle
	// Message slot 1 is a RFile Subsession handle (RFile::SubSessionHandle())
	TInt r = file.AdoptFromClient(aMsg, 0, 1);
	test_KErrNone(r);

	TBuf8<9> rbuf;
	r=file.Read(K4GB-10,rbuf);
	test_KErrNone(r);
	r=rbuf.CompareF(KTestData3());
	test_KErrNone(r);
	r=file.Write(KTestData1());
	test_Value(r, r==KErrAccessDenied);
	r=file.ChangeMode(EFileWrite);
	test_Value(r, r==KErrArgument);
	r=file.Rename(_L("\\newname.txt"));
	test_Value(r, r==KErrPermissionDenied || r==KErrAccessDenied);
	file.Close();

	aMsg.Complete(KErrNone);
	}
	
//
// Adopts file from test program and tests what it can and can't do
// Uses RFile64::AdoptFromCreator() API
//	
void CFHSession64Bit::PassFileHandleProcessLargeFile(const RMessage2& aMsg)
	{
	test.Next(_L("RFile64::AdoptFromCreator()"));

	RFile64 file;
	TInt r = file.AdoptFromCreator(1, 2);
	test_KErrNone(r);

	TBuf8<3> rbuf;
	r=file.Read(K4GB-10,rbuf,3);
	test_KErrNone(r);
	r=rbuf.CompareF(KTestData2());
	test_KErrNone(r);

	test.Next(_L("RFile::Rename()"));

	// define a filename in our private path
	RFs fs;
	r=fs.Connect();
	test_KErrNone(r);

	TFileName sessionp;
	fs.SessionPath(sessionp);
	r = fs.MkDirAll(sessionp);
	test_Value(r, r==KErrNone || r==KErrAlreadyExists);

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
	test_Value(r, r == KErrNone || r == KErrNotFound);

	TFileName fileName;
	r = file.FullName(fileName);
	test_KErrNone(r);
	
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




CFHServer64Bit* CFHServer64Bit::NewL()
	{
	CFHServer64Bit* server = new (ELeave) CFHServer64Bit;
	CleanupStack::PushL(server);
	server->ConstructL();
	CleanupStack::Pop(server);
	return server;
	}

void CFHServer64Bit::ConstructL()
	{
	}

CFHServer64Bit::CFHServer64Bit()
	: CServer2(0,ESharableSessions)
	{
	}

CFHServer64Bit::~CFHServer64Bit()
	{
	}

CSession2* CFHServer64Bit::NewSessionL(const TVersion& aVersion, const RMessage2&) const
//
//	 Create New Session
//
	{
	(void)aVersion;
	CFHSession64Bit* s = new (ELeave) CFHSession64Bit;
	return s;
	}

_LIT(KErr,"FHSERVER64BIT_ERR");


TInt CFHServer64Bit::RunError(TInt aError)
	{
	User::Panic(KErr,aError);
	return 0;
	}




TInt E32Main()
//
// Test Server for file handle sharing
//
	{
	test.Title();
	test.Start(_L("Starting FHServer64bit..."));

	// Remember the number of open handles. Just for a sanity check ....
	TInt start_thc, start_phc;
	RThread().HandleCount(start_phc, start_thc);

	CTrapCleanup* cleanup=CTrapCleanup::New();

	FHS_ASSERT(cleanup);
	CActiveScheduler* sched=new CActiveScheduler;
	FHS_ASSERT(sched);
	CActiveScheduler::Install(sched);

	// start server1
	CFHServer64Bit* svr = NULL;
	TRAP_IGNORE(svr = CFHServer64Bit::NewL());
	FHS_ASSERT(svr);
	FHS_ASSERT(svr->Start(_L("FHServer64bit"))== KErrNone||KErrAlreadyExists);

	test.Title();
	test.Start(_L("Starting tests..."));


	CActiveScheduler::Start();

	RFs cleanupfs;
	TInt r = cleanupfs.Connect();
	test_KErrNone(r);
	r=cleanupfs.SetSessionToPrivate(gTestDrive);
	test_KErrNone(r);
	r=cleanupfs.Delete(KSvrFileName);
	test_Value(r, r==KErrNone || r==KErrNotFound);
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
