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
// e32test/mmu/t_shbuf_perfclient.cpp
//
//

#include <e32std.h>
#include <e32cmn.h>
#include <e32debug.h>
#include <e32msgqueue.h>
#include <e32shbuf.h>

#include "t_shbuf_perfserver.h"
#include "t_shbuf_perfclient.h"


/**
 *  @file
 * 
 *  Client side APIs for a test server used for Performance Testing of shared buffers.
 */


/**
 *  Test API version.
 */
const TInt KRShBufTestServerMajorVersion = 1;
const TInt KRShBufTestServerMinorVersion = 0;
const TInt KRShBufTestServerBuildVersion = 1;


/**
 *  Start the server process which lives in its own executable and rendezvous with it.
 *
 *  @return  KErrNone if successful, or an error code if not.
 */
static TInt StartTestServer()
	{
#ifdef CAN_TRANSFER_SHBUF_TO_ANOTHER_PROCESS
	//
	// Create a new server process. Simultaneous launching of two such
	// processes should be detected when the second one attempts to
	// create the server object, failing with KErrAlreadyExists.
	//		
	_LIT(KTestServerExeImg, "T_SHBUF_PERFSERVER.EXE");
	RProcess server;
	
	TInt  ret = server.Create(KTestServerExeImg, KNullDesC);
	if (ret != KErrNone)
		{
		return ret;
		}
#else
	//
	// Start a thread with the server in it.
	//		
	RThread  server;
	
	TInt  ret = server.Create(_L("RShBufTestServerThread"), RShBufTestServerThread,
							  KDefaultStackSize, 0x10000, 0x10000, NULL);
	if (ret != KErrNone)
		{
		return ret;
		}
#endif
	
	//
	// Rendezvous with the server or abort startup...
	//
	TRequestStatus  status;

	server.Rendezvous(status);
	if (status != KRequestPending)
		{
		server.Kill(0);
		}
	else
		{
		server.Resume();
		}
	User::WaitForRequest(status);

	//
	// We can't use the 'exit reason' if the server panicked as this
	// is the panic 'reason' and may be '0' which cannot be distinguished
	// from KErrNone.
	//
	if (server.ExitType() == EExitPanic)
		{
		ret = KErrGeneral;
		}
	else if (status.Int() != KErrAlreadyExists)
		{
		ret = status.Int();
		}

	server.Close();
	
	return ret;
	} // StartTestServer


/**
 *  Standard constructor.
 */
EXPORT_C RShBufTestServerSession::RShBufTestServerSession()
	{
	// NOP
	} // RShBufTestServerSession::RShBufTestServerSession


/**
 *  Connects the client to the test server. 
 *
 *  @return  KErrNone if successful, a system-wide error code if not. 
 *
 *  @capability None
 */
EXPORT_C TInt RShBufTestServerSession::Connect()
	{
	//
	// Create a session with the server, but if it doesn't exist then start it and
	// then create a session.
	//
	TInt  result = CreateSession(KRShBufTestServerName,
								 TVersion(KRShBufTestServerMajorVersion,
	                					  KRShBufTestServerMinorVersion,
	                					  KRShBufTestServerBuildVersion));
	if (result == KErrNotFound  ||  result == KErrServerTerminated)
		{
		result = StartTestServer();
		
		if(result == KErrNone)
			{
			result = CreateSession(KRShBufTestServerName,
								   TVersion(KRShBufTestServerMajorVersion,
	                					    KRShBufTestServerMinorVersion,
	                					    KRShBufTestServerBuildVersion));
			}
		}	
		
	//
	// If the creation of the session fails clean up session data...
	//
	if (result != KErrNone)
		{
		Close();
		}

	return result;
	} // RShBufTestServerSession::Connect


/**
 *  Closes the client's session with the RShBuf Test Server. 
 *
 *  @capability None
 */
EXPORT_C void RShBufTestServerSession::Close()
	{
	RSessionBase::Close();
	} // RShBufTestServerSession::Close


/**
 *  Returns the current version of the RShBuf Test Server.
 *
 *  @return The version of the RShBuf Test Server. 
 *
 *  @capability None
 */
EXPORT_C TVersion RShBufTestServerSession::Version() const
	{
	return(TVersion(KRShBufTestServerMajorVersion,
	                KRShBufTestServerMinorVersion,
	                KRShBufTestServerBuildVersion));
	} // RShBufTestServerSession::Version


/**
 *  Requests the shutdown of the server when the last client disconnects.
 *  There is no support for immediate shutdown functionality. This API call
 *  can only be executed if the server is compiled as a debug release.
 *  
 *  @return  KErrNone if successful, a system-wide error code if not. 
 */
EXPORT_C TInt RShBufTestServerSession::ShutdownServer()
	{	
	return SendReceive(EShBufServerShutdownServer, TIpcArgs());
	} // RShBufTestServerSession::ShutdownServer


EXPORT_C TInt RShBufTestServerSession::FromTPtr8ProcessAndReturn(TDes8& aBuf, TUint aBufSize)
	{
	TIpcArgs  args(&aBuf, aBufSize);
	
	return SendReceive(EShBufServerFromTPtr8ProcessAndReturn, args);
	} // RShBufTestServerSession::FromTPtr8ProcessAndReturn


EXPORT_C TInt RShBufTestServerSession::FromTPtr8ProcessAndRelease(const TDesC8& aBuf)
	{
	TIpcArgs  args(&aBuf);
	
	return SendReceive(EShBufServerFromTPtr8ProcessAndRelease, args);
	} // RShBufTestServerSession::FromTPtr8ProcessAndRelease


EXPORT_C TInt RShBufTestServerSession::FromRShBufProcessAndReturn(RShBuf& aShBuf, TUint aBufSize)
	{
	TInt r;
	TInt handle;
	TPckg<TInt> handlePckg(handle);

	TIpcArgs  args(&handlePckg, aBufSize);

	r = SendReceive(EShBufServerFromRShBufProcessAndReturn, args);

	if (r == KErrNone)
	    aShBuf.SetReturnedHandle(handle);

	return r;
	} // RShBufTestServerSession::FromRShBufProcessAndReturn


EXPORT_C TInt RShBufTestServerSession::OpenRShBufPool(TInt aHandle, const TShPoolInfo& aShPoolInfo)
	{
	TPckg<TShPoolInfo>  shPoolInfoPckg(aShPoolInfo);
	TIpcArgs  args(aHandle, &shPoolInfoPckg);
	
	return SendReceive(EShBufServerOpenRShBufPool, args);
	} // RShBufTestServerSession::OpenRShBufPool


EXPORT_C TInt RShBufTestServerSession::CloseRShBufPool(TInt aHandle)
	{
	TIpcArgs  args(aHandle);
	
	return SendReceive(EShBufServerCloseRShBufPool, args);
	} // RShBufTestServerSession::CloseRShBufPool


EXPORT_C TInt RShBufTestServerSession::FromRShBufProcessAndRelease(RShBuf& aShBuf)
	{
	TIpcArgs  args(aShBuf.Handle());

	return SendReceive(EShBufServerFromRShBufProcessAndRelease, args);
	} // RShBufTestServerSession::FromRShBufProcessAndRelease


/**
 *  Set a heap mark in the RShBuf Test Server thread.
 *
 *  @capability None
 */
EXPORT_C TInt RShBufTestServerSession::__DbgMarkHeap()
	{
	TIpcArgs args(TIpcArgs::ENothing);

	return SendReceive(EShBufServerDbgMarkHeap, args);
	} // RShBufTestServerSession::__DbgMarkHeap


/**
 *  Performs a heap mark check in the RShBuf Test Server thread.
 *
 *  @param aCount  The number of heap cells expected to be allocated at
 *                 the current nest level.
 *
 *  @capability None
 */
EXPORT_C TInt RShBufTestServerSession::__DbgCheckHeap(TInt aCount)
	{
	TIpcArgs args(aCount);

	return SendReceive(EShBufServerDbgCheckHeap, args);
	} // RShBufTestServerSession::__DbgCheckHeap


/**
 *  Perfom a heap mark end check in the RShBuf Test Server thread.
 *
 *  @param aCount  The number of heap cells expected to remain allocated
 *                 at the current nest level.
 *
 *  @capability None
 */
EXPORT_C TInt RShBufTestServerSession::__DbgMarkEnd(TInt aCount)
	{
	TIpcArgs args(aCount);

	return SendReceive(EShBufServerDbgMarkEnd, args);
	} // RShBufTestServerSession::__DbgMarkEnd


/**
 *  Set a heap fail next condition in the RShBuf Test Server thread.
 *
 *  @param aCount  Determines when the allocation will fail.
 *
 *  @capability None
 */
EXPORT_C TInt RShBufTestServerSession::__DbgFailNext(TInt aCount)
	{
	TIpcArgs args(aCount);

	return SendReceive(EShBufServerDbgFailNext, args);
	} // RShBufTestServerSession::__DbgFailNext


