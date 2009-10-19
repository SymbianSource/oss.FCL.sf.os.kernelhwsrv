// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\debyg\LogToFile.cpp
// t_logtofile.exe tests (alongside with d_logtofile.ldd) trace handler hook (TTraceHandler).
// Usage:
// 1. start t_logtofile -p=Pattern
// - Starts logging into memory.(RDebug::Pring, NKern::Print & PletSec log are all considered).
// - Only logs that start with "Pattern" will be logged (case sensitive).Leave '-p=' empty to log them all.
// - All debug loggings to serial port (or epocwnd.out on emulator) are suppressed.
// - There are 64KB memory available for the log. Once the memory is full, the logging stops.
// 2. start t_logtofile stop
// - Stops the logging (unless already stopped due to full memory).
// - Transfers collected logs into c:\logtofile.dat
// The format of the file is as follows:
// The pattern:		Pattern
// Buffer Size is:		65536
// Fast counter freq:	3579545
// 93559955	U	MsgSent
// 108774945	K	Thread t_suser.EXE::Main created @ 0x973fe8 - Win32 Thread ID 0xbbc
// 108810756	U	RTEST TITLE: T_SUSER 2.00(1013)
// The first column is the current value of the fast counter.
// The second column indicates U - user side, K - kernel or P-PlatSec logging.
// 
//

#include <e32debug.h>
#include <e32cons.h>
#include <f32file.h>
#include <e32base.h>
#include <e32base_private.h>
#include "d_logtofile.h"


// The name of the output file use to save the sample data
_LIT(KFileName,"C:\\LogToFile.DAT");
_LIT(KAppName,"Logtofile");
const TInt KHeapSize=0x2000;

#define KPatternMaxSize 10
TBuf8<KPatternMaxSize> Pattern;

/**Server*/
class CLogToFileServer : public CServer2
	{
public:
	static CLogToFileServer* New(TInt aPriority) {return new CLogToFileServer(aPriority);}
private:
	CLogToFileServer(TInt aPriority) : CServer2(aPriority){}
	CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMessage) const;
public:
static RLogToFileDevice iDevice;
static TChunkCreateStr iChunkStr;
static RChunk Chunk;

	};

/**Session*/
class CLogToFileSession : public CSession2
	{
public:
	enum TState {EStart, EStop};
private:
	void ServiceL(const RMessage2& aMessage);
	TInt Stop();
	TInt Start();
	};

/*Client-side session*/
class RLogToFileSession : private RSessionBase
	{
public:
public:
	static inline TInt Start(){return Control(CLogToFileSession::EStart);}
	static inline TInt Stop() {return Control(CLogToFileSession::EStop);}
private:
	static inline TInt Control(CLogToFileSession::TState aRequest);
	};

//------------globals---------------------
RLogToFileDevice CLogToFileServer::iDevice;
TChunkCreateStr  CLogToFileServer::iChunkStr;
RChunk           CLogToFileServer::Chunk;


/**Creates a new client for this server.*/
CSession2* CLogToFileServer::NewSessionL(const TVersion&, const RMessage2&) const
	{
	return new(ELeave) CLogToFileSession();
	}

/**Entry point of the session request*/
void CLogToFileSession::ServiceL(const RMessage2& aMessage)
	{
	TInt r=KErrNone;
	switch (aMessage.Function())
		{
	case EStart:
		{
		r = Start();
		break;
		}
	case EStop:
		{
		r = Stop();
		CActiveScheduler::Stop();//This will stop the server thread.
		break;
		}
	default:
		r=KErrNotSupported;
		}
	aMessage.Complete(r);
	}

/**
This will:
 - Load t_logtofile.ldd
 - Tell ldd to create the chunk
 - Tell ldd to start logging
*/
TInt CLogToFileSession::Start()
	{
	TInt r = User::LoadLogicalDevice(KLogToFileName);
	if(r !=KErrNone && r!=KErrAlreadyExists)
		return r;
	if((r = CLogToFileServer::iDevice.Open())!=KErrNone)	
		{
		User::FreeLogicalDevice(KLogToFileName);
		return r;
		}
	CLogToFileServer::iChunkStr.iSize = 0x10000; //64K chunk size - hard coded
	if((r=CLogToFileServer::iDevice.CreateChunk(&CLogToFileServer::iChunkStr))<0)
	{
		User::FreeLogicalDevice(KLogToFileName);
		return r;
	}
	CLogToFileServer::Chunk.SetHandle(r);
	CLogToFileServer::iDevice.Start();	
	return KErrNone;
}

/**
This will:
 - Tell ldd to stop logging
 - Put the content of the chunk into c:\logtofile.dat
 - Tell ldd to close the chunk
 - Unload t_logtofile.ldd
*/
TInt CLogToFileSession::Stop()
{
	TInt bytes = CLogToFileServer::iDevice.Stop();	

	
	RFs fs;
	RFile file;
	TInt r;
	TRequestStatus status;

	if(KErrNone != (r = fs.Connect()))
		return r;
	if(KErrNone != (r = file.Replace(fs, KFileName,EFileWrite)))
		{
		fs.Close();
		return r;
		}

	TPtrC8 log(CLogToFileServer::Chunk.Base(),bytes);
	file.Write(log,status);
	User::WaitForRequest(status);
	r = status.Int();
	file.Close();
	fs.Close();
	CLogToFileServer::Chunk.Close();

	CLogToFileServer::iDevice.RemoveChunk();	
	User::FreeLogicalDevice(KLogToFileName);
	return r;
}

/**Sends request to the server*/
TInt RLogToFileSession::Control(CLogToFileSession::TState aRequest)
	{
	RLogToFileSession p;
	TInt r = p.CreateSession(KAppName, TVersion(), 0);
	if (r == KErrNone)
		p.SendReceive(aRequest);
	return r;
	}

/**The entry point for the server thread.*/
LOCAL_C TInt serverThreadEntryPoint(TAny*)
	{
	TInt r=0;
	CLogToFileServer* pS=0;
	
	CActiveScheduler *pR=new CActiveScheduler;
	if (!pR) User::Panic(_L("Create ActSchdlr error"), KErrNoMemory);
	CActiveScheduler::Install(pR);

	pS=CLogToFileServer::New(0);
	if (!pS)
	{
		delete pR;
		User::Panic(_L("Create svr error"), KErrNoMemory);
	}

	r=pS->Start(KAppName);
	if(r)
		{
		delete pS;
		delete pR;
		User::Panic(_L("Start svr error"), r);
		}

	RThread::Rendezvous(KErrNone);
	CActiveScheduler::Start();
	delete pS;
	delete pR;
	return(KErrNone);
	}

/**Reads the command line and set the matching pattern to be - what is after '-p='*/
void SetPattern(void)
	{
	_LIT8(KPattern,"-p=");
	TBuf<64> c;
	User::CommandLine(c);
	#if defined(_UNICODE)
	TPtr8 ptr = c.Collapse();
	#else
	TPtr8 ptr(c.Ptr(),c.Length(),c.MaxLEngth());
	#endif

	TInt patternStart = ptr.FindF(KPattern);
	if (patternStart < 0)
		{
		Pattern.SetLength(0);
		return;
		}
	patternStart+=3;
	
	TPtrC8 pattern (ptr.Ptr()+patternStart,Min(ptr.Length()-patternStart, KPatternMaxSize) );
	CLogToFileServer::iChunkStr.iPattern.Copy(pattern);
	}

/**The main program if we have to start logging
   It creates the server and sends start-logging request.*/
void MainL()
	{
	RThread server;
	TRequestStatus status;
	TInt r = 0;

	SetPattern();
	r=server.Create(_L("LogToFileServer"),serverThreadEntryPoint,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	User::LeaveIfError(r);
	server.Resume();
	server.Rendezvous(status);
	User::WaitForRequest(status);
	User::LeaveIfError(status.Int());

	User::LeaveIfError(RLogToFileSession::Start());

	server.Logon(status);
	User::WaitForRequest(status);
	}

/**Returns true if 'stop' is found in the command line*/
TBool GetStop()
	{
	_LIT(KStop,"stop");
	TBuf<64> c;
	User::CommandLine(c);
	if (c.FindF(KStop) >= 0)
		return ETrue;
	return EFalse;
	}

/**LogToFile.exe entry point*/
TInt E32Main()
	{
	if (GetStop())
		return RLogToFileSession::Stop();

	CTrapCleanup::New();
	TRAPD(r,MainL());
	return r;
	}
