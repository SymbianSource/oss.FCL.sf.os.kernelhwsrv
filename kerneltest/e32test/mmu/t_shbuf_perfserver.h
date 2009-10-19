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
// e32test/mmu/t_shbuf_perfserver.h
//
//

#ifndef _T_SHBUF_PERFSERVER_H_
#define _T_SHBUF_PERFSERVER_H_

/**
 *  @file
 * 
 *  Test server used for Performance Testing of shared buffers.
 */

#include <e32base.h>
#include <e32shbuf.h>

#include "d_shbuf.h"

class RShBuf;

/**
 *  Name of the server. The '!' means it is a protected server.
 */
_LIT(KRShBufTestServerName, "!RShBufServer");


/**
 *  Server IPC requests.
 */
enum TShBufServerRequest
	{
	EShBufServerSendBuffer,
	EShBufServerReceiveBuffer,
	EShBufServerSendAndReceiveBuffer,
	EShBufServerSendSharedBuffer,
	EShBufServerReceiveSharedBuffer,
	EShBufServerSendAndReceiveSharedBuffer,
	EShBufServerShutdownServer,
	EShBufServerFromTPtr8ProcessAndReturn,
	EShBufServerFromTPtr8ProcessAndRelease,
	EShBufServerOpenRShBufPool,
	EShBufServerCloseRShBufPool,
	EShBufServerFromRShBufProcessAndReturn,
	EShBufServerFromRShBufProcessAndRelease,
	EShBufServerDbgMarkHeap,
	EShBufServerDbgCheckHeap,
	EShBufServerDbgMarkEnd,
	EShBufServerDbgFailNext
	};


class CShBufTestServerSession;


/**
 *  The RShBuf test server class.
 *
 *  The class provides all the services required by class CShBufTestServerSession.
 */
class CShBufTestServer : public CServer2
	{
public:
	static CShBufTestServer* NewL();

	void AddSessionL(CShBufTestServerSession* aSession);
	void DropSession(CShBufTestServerSession* aSession);

	TInt FromTPtr8ProcessAndReturn(TDes8& aBuf, TUint aBufSize);
	TInt FromTPtr8ProcessAndRelease(TDes8& aBuf);

	TInt OpenRShBufPool(TInt aHandle, const TShPoolInfo& aPoolInfo);
	TInt CloseRShBufPool();
	TInt FromRShBufProcessAndReturn(RShBuf& aShBuf, TUint aBufSize);
	TInt FromRShBufProcessAndRelease(RShBuf& aShBuf);

	TInt DbgMarkHeap() const;
	TInt DbgCheckHeap(TInt aCount) const;
	TInt DbgMarkEnd(TInt aCount) const;
	TInt DbgFailNext(TInt aCount) const;

	TInt ShutdownServer();

private:
	CShBufTestServer();
	~CShBufTestServer();

	void ConstructL();

	CSession2* NewSessionL(const TVersion& aVersion,const RMessage2&/*aMessage*/) const;
	TInt RunError(TInt aError);

	RPointerArray<CShBufTestServerSession>  iSessionArray;

	//
	// Variables to control shutdown of the server...
	//
	TBool  iShouldShutdownServer;

	//
	// Handle to the driver...
	//
	RShBufTestChannel  iShBufLdd;
	TUint8  iClearCache[32768];
	};


/**
 *  This is the Phonebook Sync Server side session class and is responsible
 *  for handling the client (RPhoneBookSession) requests, encoding/decoding
 *  the parameters and Contacts Item phonebook data across the API.
 *  Once the parameters are decoded the request is sent to the server where
 *  it will either be handled directly or forwarded to the Background Sync
 *  Engine. Once the request is completed, any return parameters are written
 *  back to the client if neccessary.
 */
class CShBufTestServerSession : public CSession2
	{
public:
	void CreateL();
	void CompleteRequest(const RMessage2& aMessage, TInt aResult) const;

	inline CShBufTestServer& Server();

private:
	~CShBufTestServerSession();

	void ServiceL(const RMessage2& aMessage);

	void ShutdownServerL(const RMessage2& aMessage);

	void FromTPtr8ProcessAndReturnL(const RMessage2& aMessage);
	void FromTPtr8ProcessAndReleaseL(const RMessage2& aMessage);

	void OpenRShBufPoolL(const RMessage2& aMessage);
	void CloseRShBufPoolL(const RMessage2& aMessage);
	void FromRShBufProcessAndReturnL(const RMessage2& aMessage);
	void FromRShBufProcessAndReleaseL(const RMessage2& aMessage);

	void DbgMarkHeapL(const RMessage2& aMessage);
	void DbgCheckHeapL(const RMessage2& aMessage);
	void DbgMarkEndL(const RMessage2& aMessage);
	void DbgFailNextL(const RMessage2& aMessage);

private:
	TUint8  iSessionTempBuffer[8192];
	};


/**
 *  Returns a reference to the CPhoneBookServer class.
 */
inline CShBufTestServer& CShBufTestServerSession::Server()
	{
	return *static_cast<CShBufTestServer*>(const_cast<CServer2*>(CSession2::Server()));
	} // CShBufTestServerSession::Server


#ifndef CAN_TRANSFER_SHBUF_TO_ANOTHER_PROCESS
TInt RShBufTestServerThread(TAny* aPtr);
#endif


#endif // _T_SHBUF_PERFSERVER_H_

