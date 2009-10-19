// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implements a Session of a Symbian OS server for the RUsbMassStorage API
// 
//

/**
 @file
 @internalTechnology
*/

#include "cusbmassstoragesession.h"
#include "cusbmassstoragecontroller.h"
#include "cusbmassstorageserver.h"
#include "usbmsshared.h"

/**
 Construct a Symbian OS session object.
 
 @param	aServer		Service the session will be a member of
 @param	aMessage	The message from the client.
 @return	A new CUsbMassStorageSession object
 */
CUsbMassStorageSession* CUsbMassStorageSession::NewL(CUsbMassStorageServer& aServer)
	{
	CUsbMassStorageSession* r = new (ELeave) CUsbMassStorageSession(aServer);
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}

/**
 Constructor.
 
 @param	aServer	Service the session will be a member of
 */
CUsbMassStorageSession::CUsbMassStorageSession(CUsbMassStorageServer& aServer)
	: iUsbMsServer(aServer)
	{
    __PRINT(_L("CUsbMassStorageSession::CUsbMassStorageSession\n"));
	}


/**
 2nd Phase Construction.
 */
void CUsbMassStorageSession::ConstructL()
	{
	iUsbMsServer.IncrementSessionCount();
    if (iUsbMsServer.SessionCount() > 1)
        {
        __PRINT1(_L("\tiSessionCount: %d\n"), iUsbMsServer.SessionCount());
        // Only one session is allowed
        User::Leave(KErrInUse);
        }        
 	}


/**
 Destructor.
 */
CUsbMassStorageSession::~CUsbMassStorageSession()
	{
	iUsbMsServer.DecrementSessionCount();
	}

/**
 Called when a message is received from the client.
 
 @param	aMessage	Message received from the client
 */
void CUsbMassStorageSession::ServiceL(const RMessage2& aMessage)
	{
	DispatchMessageL(aMessage);
	}

/**
 Handles the request (in the form of a the message) received from the client
 
 @internalTechnology
 @param	aMessage	The received message
 */
void CUsbMassStorageSession::DispatchMessageL(const RMessage2& aMessage)
	{
	TInt ret = KErrNone;
	
	switch (aMessage.Function())
		{
	case EUsbMsStart:
		ret = Start(aMessage);
		break;
	case EUsbMsStop:
		ret = Stop();
		break;
	case EUsbMsShutdown:
		ret = Shutdown();
		break;

	default:
		aMessage.Panic(KUsbMsCliPncCat, EUsbMsPanicIllegalIPC);
		break;
		}

	aMessage.Complete(ret);
	}

/**
 Client request to start the device.
 
 @return	Any error that occurred or KErrNone
 */
TInt CUsbMassStorageSession::Start(const RMessage2& aMessage)
	{
	__PRINT(_L("CUsbMassStorageSession::Start\n"));

	TMassStorageConfig msConfig;
	TRAPD(err, GetMsConfigL(aMessage, msConfig));
	if (err != KErrNone)
		{
		__PRINT(_L("Failed to get mass storage configuration data\n"));
		return err;
		}
		
	return iUsbMsServer.Controller().Start(msConfig);
	}

/**
 Client request to stop the device.
 
 @return	Any error that occurred or KErrNone
 */
TInt CUsbMassStorageSession::Stop()
    {
    __PRINT(_L("CUsbMassStorageSession::Stop\n"));
    TInt err = iUsbMsServer.Controller().Stop();
	return err;
	}

/**
 Client request to shut down the server
 
 @return KErrNone
 */
TInt CUsbMassStorageSession::Shutdown()
    {
    __PRINT(_L("CUsbMassStorageSession::Shutdown\n"));
    CActiveScheduler::Stop();
	return KErrNone;
	}

 /**
  Get mass storage configuration data from the received message
  */
 void CUsbMassStorageSession::GetMsConfigL(const RMessage2& aMessage, TMassStorageConfig& aMsStorage)
 	{
 	aMessage.ReadL(0,aMsStorage.iVendorId);
 	aMessage.ReadL(1,aMsStorage.iProductId);
 	aMessage.ReadL(2,aMsStorage.iProductRev);
 	}
