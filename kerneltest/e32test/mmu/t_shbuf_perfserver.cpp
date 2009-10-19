// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// t_shbuf_perfserver.cpp
//
//

/**
 *  @file
 * 
 *  Test server used for Performance Testing of shared buffers.
 */

#include <e32std.h>
#include <e32debug.h>
#include <e32msgqueue.h>
#include <e32shbuf.h>

#include "t_shbuf_perfserver.h"


/**
 *  Second phase constructor for sessions. Called by the CServer2 framework
 *  when a session is created (e.g. a connection is made to the server).
 */
void CShBufTestServerSession::CreateL()
	{
	Server().AddSessionL(this);
	} // CShBufTestServerSession::CreateL()


/**
 *  Destructor for session classes. When this is called it indicates that
 *  a session is closing its connection with the server.
 */
CShBufTestServerSession::~CShBufTestServerSession()
	{
	Server().DropSession(this);
	} // CShBufTestServerSession::~CShBufTestServerSession()


/**
 *  Handle message requests for this session. Leaving is handled by
 *  CShBufTestServer::RunError() which reports the error code to the client.
 *
 *  @param aMessage  RMessage2 reference which encapsulates a client request.
 */
void CShBufTestServerSession::ServiceL(const RMessage2& aMessage)

	{
	switch (aMessage.Function())
		{
		case EShBufServerShutdownServer:
			{
			ShutdownServerL(aMessage);
			}
			break;

		case EShBufServerFromTPtr8ProcessAndReturn:
			{
			FromTPtr8ProcessAndReturnL(aMessage);
			}
			break;

		case EShBufServerFromTPtr8ProcessAndRelease:
			{
			FromTPtr8ProcessAndReleaseL(aMessage);
			}
			break;

		case EShBufServerOpenRShBufPool:
			{
			OpenRShBufPoolL(aMessage);
			}
			break;

		case EShBufServerCloseRShBufPool:
			{
			CloseRShBufPoolL(aMessage);
			}
			break;

		case EShBufServerFromRShBufProcessAndReturn:
			{
			FromRShBufProcessAndReturnL(aMessage);
			}
			break;

		case EShBufServerFromRShBufProcessAndRelease:
			{
			FromRShBufProcessAndReleaseL(aMessage);
			}
			break;

		case EShBufServerDbgMarkHeap:
			{
			DbgMarkHeapL(aMessage);
			}
			break;

		case EShBufServerDbgCheckHeap:
			{
			DbgCheckHeapL(aMessage);
			}
			break;

		case EShBufServerDbgMarkEnd:
			{
			DbgMarkEndL(aMessage);
			}
			break;

		case EShBufServerDbgFailNext:
			{
			DbgFailNextL(aMessage);
			}
			break;

		default:
			{
			aMessage.Panic(KRShBufTestServerName, 999);
			}
			break;
		}
	} // CShBufTestServerSession::ServiceL()


/**
 *  Completes a client request. This function provides a single point of
 *  message completion which benefits debugging and maintenance.
 *
 *  @param aMessage  The RMessage2 client request.
 *  @param aResult   Result of the request.
 */
void CShBufTestServerSession::CompleteRequest(const RMessage2& aMessage,
										      TInt aResult) const
	{
	if (aMessage.IsNull() == EFalse)
		{
	    aMessage.Complete(aResult);
		}
    } // CShBufTestServerSession::CompleteRequest()


/**
 *  Takes a buffer from the client, sends to the driver and back to the client.
 *
 *  @param aMessage  RMessage2 client request.
 */
void CShBufTestServerSession::FromTPtr8ProcessAndReturnL(const RMessage2& aMessage)
	{
	//
	// Read the client buffer...
	//
	TPtr8  bufPtr(iSessionTempBuffer, sizeof(iSessionTempBuffer));
	
	aMessage.ReadL(0, bufPtr); 

	TUint bufSize;

	bufSize = aMessage.Int1();

	//
	// Pass to the server to pass to the driver and back...
	//
	TInt  result;
	
	result = Server().FromTPtr8ProcessAndReturn(bufPtr, bufSize);

	//
	// Write the client buffer back...
	//
	aMessage.WriteL(0, bufPtr); 

	//
	// Complete the request...
	//
	CompleteRequest(aMessage, result);
	} // CShBufTestServerSession::FromTPtr8ProcessAndReturnL


/**
 *  Takes a buffer from the client and sends to the driver.
 *
 *  @param aMessage  RMessage2 client request.
 */
void CShBufTestServerSession::FromTPtr8ProcessAndReleaseL(const RMessage2& aMessage)
	{
	//
	// Read the client buffer...
	//
	TPtr8  bufPtr(iSessionTempBuffer, sizeof(iSessionTempBuffer));
	
	aMessage.ReadL(0, bufPtr); 

	//
	// Pass to the server to pass to the driver and back...
	//
	TInt  result;
	
	result = Server().FromTPtr8ProcessAndRelease(bufPtr);

	//
	// Complete the request...
	//
	CompleteRequest(aMessage, result);
	} // CShBufTestServerSession::FromTPtr8ProcessAndReleaseL


/**
 *  Allows the client to ask the test server to open a buffer pool.
 *
 *  @param aMessage  RMessage2 client request.
 */
void CShBufTestServerSession::OpenRShBufPoolL(const RMessage2& aMessage)
	{
	//
	// Read the handle...
	//
	TInt  poolHandle = aMessage.Int0();
	
	//
	// Read the pool info...
	//
	TShPoolInfo  shPoolInfo;
	TPckg<TShPoolInfo>  shPoolInfoPckg(shPoolInfo);
	
	aMessage.ReadL(1, shPoolInfoPckg); 

	//
	// Pass to the server to open the pool...
	//
	TInt  result = Server().OpenRShBufPool(poolHandle, shPoolInfo);

	//
	// Complete the request...
	//
	CompleteRequest(aMessage, result);
	} // CShBufTestServerSession::OpenRShBufPoolL


/**
 *  Allows the client to ask the test server to close a buffer pool.
 *
 *  @param aMessage  RMessage2 client request.
 */
void CShBufTestServerSession::CloseRShBufPoolL(const RMessage2& aMessage)
	{
	//
	// Pass to the server to close the pool...
	//
	TInt  result = Server().CloseRShBufPool();

	//
	// Complete the request...
	//
	CompleteRequest(aMessage, result);
	} // CShBufTestServerSession::CloseRShBufPoolL


/**
 *  Takes a buffer from the client, sends to the driver and back to the client.
 *
 *  @param aMessage  RMessage2 client request.
 */
void CShBufTestServerSession::FromRShBufProcessAndReturnL(const RMessage2& aMessage)
	{
	//
	// Read the client handle buffer...
	//
	RShBuf shBuf;
	TUint bufSize;

	bufSize = aMessage.Int1();

	//
	// Pass to the server to pass to the driver and back...
	//
	TInt  result;

	result = Server().FromRShBufProcessAndReturn(shBuf, bufSize);

	//
	// Write the client buffer handle back...
	//
#ifdef CAN_TRANSFER_SHBUF_TO_ANOTHER_PROCESS
	// TDBD aMessage.Complete(shbuf->Handle());
#else
	TPckg<TInt> handlePckg(shBuf.Handle());
	aMessage.WriteL(0, handlePckg);
#endif

	//
	// Complete the request...
	//
	CompleteRequest(aMessage, result);
	} // CShBufTestServerSession::FromRShBufProcessAndReturnL


/**
 *  Takes a buffer from the client and sends to the driver.
 *
 *  @param aMessage  RMessage2 client request.
 */
void CShBufTestServerSession::FromRShBufProcessAndReleaseL(const RMessage2& aMessage)
	{
	//
	// Read the client buffer handle...
	//

	RShBuf  shBuf;

#ifdef CAN_TRANSFER_SHBUF_TO_ANOTHER_PROCESS
	// TBD RShBuf.Open(aMessage, 0);
#else
	shBuf.SetReturnedHandle(aMessage.Int0());
#endif

	//
	// Pass to the server to pass to the driver and back...
	//
	TInt  result;
	
	result = Server().FromRShBufProcessAndRelease(shBuf);

	//
	// Complete the request...
	//
	CompleteRequest(aMessage, result);
	} // CShBufTestServerSession::FromRShBufProcessAndReleaseL


/**
 *  Requests the server to mark the start of checking the server's heap.
 *  This function only works in debug releases and is a synchronous request
 *  which will be completed when the procedure returns.
 *
 *  @param aMessage  RMessage2 client request.
 */
void CShBufTestServerSession::DbgMarkHeapL(const RMessage2& aMessage)
	{
	TInt  result;
	
	result = Server().DbgMarkHeap();

	//
	// Complete the request...
	//
	CompleteRequest(aMessage, result);
	} // CShBufTestServerSession::DbgMarkHeapL()


/**
 *  Requests the server to check that the number of allocated cells at the
 *  current nested level on the server's heap is the same as the specified value.
 *  This function only works for debug builds and is a synchronous request
 *  which will be completed when the procedure returns.
 *
 *  @param aMessage  RMessage2 client request.
 */
void CShBufTestServerSession::DbgCheckHeapL(const RMessage2& aMessage)
	{
	TInt  count = aMessage.Int0();
	TInt  result;
	
	result = Server().DbgCheckHeap(count);

	//
	// Complete the request...
	//
	CompleteRequest(aMessage, result);
	} // CShBufTestServerSession::DbgCheckHeapL()


/**
 *  Requests the server to mark the end of checking the server's heap. This
 *  function must match an earlier call to DbgMarkHeap() and only functions
 *  on debug releases. This is a synchronous request which will be completed
 *  when the procedure returns.
 *
 *  @param aMessage  RMessage2 client request.
 */
void CShBufTestServerSession::DbgMarkEndL(const RMessage2& aMessage)
	{
	TInt  count = aMessage.Int0();
	TInt  result;
	
	result = Server().DbgMarkEnd(count);

	//
	// Complete the request...
	//
	CompleteRequest(aMessage, result);
	} // CShBufTestServerSession::DbgMarkEndL()


/**
 *  Simulates heap allocation failure for the sever. The failure occurs on
 *  the next call to new or any of the functions which allocate memory from
 *  the heap. This is defined only for debug builds and is a synchronous
 *  request which will be completed when the procedure returns.
 *
 *  @param aMessage  RMessage2 client request.
 */
void CShBufTestServerSession::DbgFailNextL(const RMessage2& aMessage)
	{
	TInt  count = aMessage.Int0();
	TInt  result;
	
	result = Server().DbgFailNext(count);

	//
	// Complete the request...
	//
	CompleteRequest(aMessage, result);
	} // CShBufTestServerSession::DbgFailNextL()


/**
 *  Requests the server to shut down when it no longer has any connected
 *  sessions. This procedure is only premitted in debug builds for security
 *  reasons (e.g. to prevent a denial of service attack) and is provided
 *  for testing purposes. This is a synchronous request which will be
 *  completed when the procedure returns. The server will shutdown when the
 *  last session disconnects.
 *
 *  @param aMessage  RMessage2 client request.
 */
void CShBufTestServerSession::ShutdownServerL(const RMessage2& aMessage)
	{
	TInt  result = Server().ShutdownServer();
	
	//
	// Complete the request...
	//
	CompleteRequest(aMessage, result);
	} // CShBufTestServerSession::ShutdownServerL()


/**
 *  Static factory method used to create a CShBufTestServer object.
 *
 *  @return  Pointer to the created CShBufTestServer object, or NULL.
 */
CShBufTestServer* CShBufTestServer::NewL()
	{
	CShBufTestServer* self = new (ELeave) CShBufTestServer();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);

	return self;
	} // CShBufTestServer::NewL


/**
 *  Standard constructor.
 */
CShBufTestServer::CShBufTestServer()
	: CServer2(EPriorityNormal),
	iShouldShutdownServer(EFalse)
	{
	__DECLARE_NAME(_L("CShBufTestServer"));
	} // CShBufTestServer::CShBufTestServer


/**
 *  Second phase constructor. Ensures the server is created and ready to run.
 */
void CShBufTestServer::ConstructL()
	{
	//
	// Open the driver...
	//
	TInt  ret;
	
	ret = User::LoadLogicalDevice(_L("D_SHBUF_CLIENT.LDD"));
	if (ret != KErrAlreadyExists)
		{
		User::LeaveIfError(ret);
		}
	
	ret = User::LoadLogicalDevice(_L("D_SHBUF_OWN.LDD"));
	if (ret != KErrAlreadyExists)
		{
		User::LeaveIfError(ret);
		}
	
	User::LeaveIfError(iShBufLdd.Open(RShBufTestChannel::EClientThread));
	
	StartL(KRShBufTestServerName);
	} // CShBufTestServer::ConstructL


/**
 *  Destructor.
 */
CShBufTestServer::~CShBufTestServer()
	{
	iSessionArray.Reset();
	iShBufLdd.Close();
	} // CShBufTestServer::~CShBufTestServer


/**
 *  Create a new client session.
 */
CSession2* CShBufTestServer::NewSessionL(const TVersion&, const RMessage2& /*aMessage*/) const
	{
	return new(ELeave) CShBufTestServerSession();
	} // CShBufTestServer::NewSessionL


/**
 *  Called by the session class when it is being created.
 *
 *  @param aSession  Server side session.
 */
void CShBufTestServer::AddSessionL(CShBufTestServerSession* aSession)
	{
	//
	// Store this session in the list of sessions...
	//
	iSessionArray.Append(aSession);
	} // CShBufTestServer::AddSessionL


/**
 *  Called by the session class when it is being destroyed.
 *
 *  @param aSession  Server side session.
 */
void CShBufTestServer::DropSession(CShBufTestServerSession* aSession)
	{
	//
	// Remove this session from the session array list...
	//
	TInt  position;
	
	position = iSessionArray.Find(aSession);
 	if (position != KErrNotFound) 
		{
 		iSessionArray.Remove(position);
		}

	//
	// If we are shuting down then unconfigure and stop...
	//
	if (iSessionArray.Count() == 0  &&  iShouldShutdownServer)
		{
		CActiveScheduler::Stop();
		}
	} // CShBufTestServer::DropSession


TInt CShBufTestServer::FromTPtr8ProcessAndReturn(TDes8& aBuf, TUint aBufSize)
	{
	// clear cache
	memset(iClearCache, 0xFF, sizeof(iClearCache));

	return iShBufLdd.FromTPtr8ProcessAndReturn(aBuf, aBufSize);
	} // CShBufTestServer::FromTPtr8ProcessAndReturn


TInt CShBufTestServer::FromTPtr8ProcessAndRelease(TDes8& aBuf)
	{
	return iShBufLdd.FromTPtr8ProcessAndRelease(aBuf);
	} // CShBufTestServer::FromTPtr8ProcessAndRelease


TInt CShBufTestServer::OpenRShBufPool(TInt aHandle, const TShPoolInfo& aPoolInfo)
	{
	return iShBufLdd.OpenUserPool(aHandle, aPoolInfo);
	} // CShBufTestServer::OpenRShBufPool

	
TInt CShBufTestServer::CloseRShBufPool()
	{
	return iShBufLdd.CloseUserPool();
	} // CShBufTestServer::CloseRShBufPool

	
TInt CShBufTestServer::FromRShBufProcessAndReturn(RShBuf& aShBuf, TUint aBufSize)
	{
	TInt ret;

	// clear cache
	memset(iClearCache, 0xFF, sizeof(iClearCache));

	ret = iShBufLdd.FromRShBufProcessAndReturn(aBufSize);

	if (ret > 0)
		{
		aShBuf.SetReturnedHandle(ret);
		return KErrNone;
		}

	return ret;
	} // CShBufTestServer::FromRShBufProcessAndReturn


TInt CShBufTestServer::FromRShBufProcessAndRelease(RShBuf& aShBuf)
	{
	return iShBufLdd.FromRShBufProcessAndRelease(aShBuf.Handle());
	} // CShBufTestServer::FromRShBufProcessAndRelease 


/**
 *  Marks the start of checking the server's heap. This function only works
 *  in debug releases.
 *
 *  Calls to this function can be nested but each call must be matched by
 *  corresponding DbgMarkEnd().
 *
 *  @return  KErrNone.
 */
TInt CShBufTestServer::DbgMarkHeap() const
	{
#ifdef _DEBUG
	__UHEAP_MARK;
#endif

	return(KErrNone);
	} // CShBufTestServer::DbgMarkHeap


/**
 *  Checks that the number of allocated cells at the current nested level on
 *  the server's heap is the same as the specified value. This function only
 *  works for debug builds.
 *
 *  @param aCount  The number of heap cells expected to be allocated at
 *                 the current nest level.
 *
 *  @return  KErrNone.
 */
TInt CShBufTestServer::DbgCheckHeap(TInt aCount) const
	{
#ifdef _DEBUG 
	__UHEAP_CHECK(aCount);
#else
	(void) aCount;
#endif

	return(KErrNone);
	} // CShBufTestServer::DbgCheckHeap


/**
 *  Marks the end of checking the current server's heap. 
 *
 *  The function expects aCount heap cells to remain allocated at the
 *  current nest level. This function must match an earlier call to
 *  DbgMarkHeap() and only functions on debug releases.
 *
 *  @param aCount  The number of heap cells expected to remain allocated
 *                 at the current nest level.
 *
 *  @return  KErrNone.
 */
TInt CShBufTestServer::DbgMarkEnd(TInt aCount) const
	{
#ifdef _DEBUG
	__UHEAP_MARKENDC(aCount);
#else
	(void) aCount;
#endif

	return(KErrNone);
	} // CShBufTestServer::DbgMarkEnd


/**
 *  Simulates heap allocation failure for the server.
 *
 *  The failure occurs on the next call to new or any of the functions which 
 *  allocate memory from the heap. This is defined only for debug builds.
 *
 *  @param aCount  Determines when the allocation will fail.
 *
 *  @return  KErrNone.
 */
TInt CShBufTestServer::DbgFailNext(TInt aCount) const
	{
#ifdef _DEBUG
	if (aCount == 0)
		{
		__UHEAP_RESET;
		}
	else
		{
		__UHEAP_FAILNEXT(aCount);
		}
#else
	(void) aCount;
#endif

	return(KErrNone);
	} // CShBufTestServer::DbgFailNext


/**
 *  Requests the server to shut down when it no longer has any connected
 *  sessions. This procedure is only premitted in debug builds and is provided
 *  for testing purposes.
 *
 *  The server will shutdown when the last session disconnects.
 *
 *  @return KErrNone if the shutdown request was accepted, otherwise returns
 *          an error.
 */
TInt CShBufTestServer::ShutdownServer()
	{
	iShouldShutdownServer = ETrue;

	return(KErrNone);
	} // CShBufTestServer::ShutdownServer



/**
 *  Standard Active Object RunError() method, called when the RunL() method
 *  leaves, which will be when the CShBufTestServerSession::ServiceL() leaves.
 *
 *  Find the current message and complete it before restarting the server.
 *
 *  @param aError  Leave code from CShBufTestServerSession::ServiceL().
 *
 *  @return KErrNone
 */
TInt CShBufTestServer::RunError(TInt aError)
	{
	//
	// Complete the request with the available error code.
	//
	if (Message().IsNull() == EFalse)
		{
		Message().Complete(aError);
		}

	//
	// The leave will result in an early return from CServer::RunL(), skipping
	// the call to request another message. So do that now in order to keep the
	// server running.
	//
	ReStart();

	return KErrNone;
	} // CShBufTestServer::RunError


/**
 *  Perform all server initialisation, in particular creation of the
 *  scheduler and server and then run the scheduler.
 */
static void RunServerL()
	{
	//
	// Naming the server thread after the server helps to debug panics.
	//
	User::LeaveIfError(User::RenameThread(KRShBufTestServerName));

	//
	// Increase priority so that requests are handled promptly...
	//
	RThread().SetPriority(EPriorityMuchMore);

	//	
	// Create a new Active Scheduler...
	//
	CActiveScheduler*  scheduler = new CActiveScheduler();
	CleanupStack::PushL(scheduler);	
	CActiveScheduler::Install(scheduler);
	
	//
	// Create a new PhoneBookServer...
	//
	CShBufTestServer*  server = CShBufTestServer::NewL();
	CleanupStack::PushL(server);
	
	//
	// Initialisation complete, now signal the client thread...
	//
#ifdef CAN_TRANSFER_SHBUF_TO_ANOTHER_PROCESS
	RProcess::Rendezvous(KErrNone);
#else
	RThread::Rendezvous(KErrNone);
#endif

	//
	// Run the server...
	//
	CActiveScheduler::Start();
	
	CleanupStack::PopAndDestroy(2, scheduler);
	} // RunServerL


/**
 *  Server process entry-point.
 *
 *  @return  KErrNone or a standard Symbian error code.
 */
#ifdef CAN_TRANSFER_SHBUF_TO_ANOTHER_PROCESS
TInt E32Main()
#else
TInt RShBufTestServerThread(TAny* /*aPtr*/)
#endif
	{
	__UHEAP_MARK;

	CTrapCleanup*  cleanup = CTrapCleanup::New();
	TInt  ret(KErrNoMemory);

	if (cleanup)
		{
		TRAP(ret, RunServerL());
		delete cleanup;
		}

	__UHEAP_MARKEND;

	return ret;
	} // E32Main



