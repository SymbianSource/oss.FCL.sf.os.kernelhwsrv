// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file
 @internalTechnology
*/

#include <e32base.h>
#include <e32base_private.h>
#include <e32property.h>
#include "msctypes.h"
#include "mscutils.h"
#include "shared.h"
#include "msgservice.h"
#include "cusbhostmslogicalunit.h"
#include "cusbhostmsdevice.h"
#include "cusbhostmsserver.h"
#include "usbmshostpanic.h"
#include "cusbhostmsdevicethread.h"
#include "cusbhostmssession.h"
#include "msdebug.h"
#include "debug.h"

/** Construct a Symbian OS session object.

	param	aServer		Service the session will be a member of
	param	aMessage	The message from the client.
	return	A new CUsbHostMsSession object
 */
CUsbHostMsSession* CUsbHostMsSession::NewL(CUsbHostMsServer& aServer)
	{
    __MSFNSLOG
	CUsbHostMsSession* r = new (ELeave) CUsbHostMsSession(aServer);
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}

/**
 Constructor.

	param	aServer	Service the session will be a member of
 */
CUsbHostMsSession::CUsbHostMsSession(CUsbHostMsServer& aServer)
	: iUsbHostMsServer(aServer)
	{
    __MSFNLOG
	iMsgCount = 0;
	}


/**
 2nd Phase Construction.
 */
void CUsbHostMsSession::ConstructL()
	{
    __MSFNLOG
	iUsbHostMsServer.IncrementSessionCount();
    __HOSTPRINT1(_L("\tiSessionCount: %d\n"), iUsbHostMsServer.SessionCount());
	}


/**
 Destructor.
 */
CUsbHostMsSession::~CUsbHostMsSession()
	{
    __MSFNLOG

	iUsbHostMsServer.DecrementSessionCount();
	iThread.Close();
    __HOSTPRINT1(_L("\tClosed a session -> iSessionCount: %d\n"), iUsbHostMsServer.SessionCount());
	}

/**
 Called when a message is received from the client.

	param	aMessage	Message received from the client
 */
void CUsbHostMsSession::ServiceL(const RMessage2& aMessage)
	{
    __MSFNLOG
	DispatchMessageL(aMessage);
	}


/**
 Handles the request (in the form of a message) received from the client

	param	aMessage	The received message
 */
void CUsbHostMsSession::DispatchMessageL(const RMessage2& aMessage)
	{
    __MSFNLOG
	TInt r = KErrNone;
	switch (aMessage.Function())
		{
	case EUsbHostMsRegisterInterface:
		TRAP(r, CreateDeviceThreadL(aMessage));
		if(r != KErrNone)
			{
			aMessage.Complete(r);
			return;
			}
		break;
	/* If it is a cleanup then we need to delete the iDeviceThread */
	case EUsbHostMsFinalCleanup:
		if(iDeviceThread->IsActive())
			{
			TRequestStatus* s=&iDeviceThread->iStatus;
			iThread.RequestComplete(s, KErrSessionClosed);
			}
		iDeviceThread->Cancel();
		delete iDeviceThread;
		iThread.Kill(KErrNone);
		aMessage.Complete(KErrNone);
		return;
	default:
		break;
		}

	__HOSTPRINT1(_L("Queuing %d message"), ++iMsgCount);
	__ASSERT_DEBUG(iDeviceThread != NULL, User::Panic(KUsbMsHostPanicCat, EDeviceThreadDoesNotExist));

	r = iDeviceThread->QueueMsg(aMessage);
	if(r != KErrNone)
		{
		aMessage.Complete(r);
		return;
		}

	if(iDeviceThread->IsActive())
		{
		iDeviceThread->Lock();
		if(iDeviceThread->iIsSignalled)
			{
			iDeviceThread->Unlock();
			return;
			}
		iDeviceThread->iIsSignalled = ETrue;
		iDeviceThread->Unlock();
		__HOSTPRINT(_L("Signaling device thread to handle message"));
		TRequestStatus* s=&iDeviceThread->iStatus;
		iThread.RequestComplete(s, KErrNone);
		}
	}


void CUsbHostMsSession::CreateDeviceThreadL(const RMessage2& aMessage)
	{
	THostMassStorageConfig msDeviceConfig;
	TPtr8 ptr((TUint8*)&msDeviceConfig,sizeof(THostMassStorageConfig));

	aMessage.ReadL(0, ptr);
	__HOSTPRINT1(_L("EUsbHostMsRegisterInterface Token=%d "), msDeviceConfig.iInterfaceToken);

	TInt r = KErrNone;
    TName nameBuf;
	TRequestStatus aStatus;

	nameBuf.Format(_L("Host Ms Thread%d"), msDeviceConfig.iInterfaceToken);
	iDeviceThread = CUsbHostMsDeviceThread::NewL(msDeviceConfig.iInterfaceToken);

	RHeap* h = (RHeap*)&User::Allocator();
	TInt maxsize = h->MaxLength();	// loader heap max size = file server heap max size
	const TUint KHeapMinSize = 2048;

	r = iThread.Create(nameBuf, CUsbHostMsDeviceThread::Entry, KDefaultStackSize, KHeapMinSize, maxsize, iDeviceThread);
	if(r != KErrNone)
		{
		delete iDeviceThread;
        iDeviceThread = NULL;
		User::Leave(r);
		}
	iThread.SetPriority(EPriorityAbsoluteBackgroundNormal);
	iThread.Rendezvous(aStatus);
	iThread.Resume();
	User::WaitForRequest(aStatus);
	if(aStatus != KErrNone)
		{
		if(iDeviceThread->IsActive())
			{
			TRequestStatus* s=&iDeviceThread->iStatus;
			iThread.RequestComplete(s, KErrSessionClosed);
			}
		iDeviceThread->Cancel();
		delete iDeviceThread;
        iDeviceThread = NULL;
		iThread.Kill(KErrNone);
		User::Leave(aStatus.Int());
		}
	}
