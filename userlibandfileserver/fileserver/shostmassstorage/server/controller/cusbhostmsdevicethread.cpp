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
// Implements a Session of a Symbian OS server for the RUsbMassStorage API
//
//

/**
 @file
 @internalTechnology
*/

#include <e32base.h>
#include <e32base_private.h>
#include "msctypes.h"
#include "mscutils.h"
#include "shared.h"
#include "msgservice.h"
#include "cusbhostmslogicalunit.h"
#include "cusbhostmsdevice.h"
#include "cusbhostmsserver.h"
#include "msdebug.h"
#include "cusbhostmsdevicethread.h"
#include "cusbhostmssession.h"
#include "debug.h"

/**
Constructor
*/
TDeviceHandler::TDeviceHandler(CUsbHostMsDevice& aDevice)
:   iDevice(aDevice)
    {
    __MSFNLOG
    }


/**
   Services messages directed to a device. Messages associated with a single
   Logical Unit contained by the device are forwarded to the Logical Unit
   Handler.

   @param aMessage
 */
void TDeviceHandler::HandleMessageL(const RMessage2& aMessage)
    {
    __MSFNLOG
    TLun lun = iDevice.GetAndSetLunL(aMessage);

	switch (aMessage.Function())
		{
	case EUsbHostMsSuspendLun:
        iDevice.SuspendLunL(lun);
		break;
    case EUsbHostMsUnRegisterLun:
        iDevice.RemoveLunL(lun);
        break;
    default:
        // Try Logical Unit Handler
        CUsbHostMsLogicalUnit& lu = iDevice.GetLuL(lun);
        TLogicalUnitHandler luHandler(lu);
        luHandler.HandleMessageL(aMessage);
		break;
		}
    }


/**
   Constructor

   @param aLu Reference to the logical unit object
 */
TLogicalUnitHandler::TLogicalUnitHandler(CUsbHostMsLogicalUnit& aLu)
:   iLu(aLu)
    {
    __MSFNLOG
    }


/**
   Services messages directed to a specific Logical Unit. Unrecognized messages
   will cause a PANIC.

   @param aMessage
 */
void TLogicalUnitHandler::HandleMessageL(const RMessage2& aMessage)
	{
    __MSFNLOG
	switch (aMessage.Function())
		{
	case EUsbHostMsNotifyChange:
        iLu.NotifyChange(aMessage);
		break;
	case EUsbHostMsCancelChangeNotifier:
        iLu.CancelChangeNotifierL();
		break;
	case EUsbHostMsForceRemount:
        iLu.ForceCompleteNotifyChangeL();
		break;
	case EUsbHostMsRead:
        iLu.ReadL(aMessage);
		break;
	case EUsbHostMsWrite:
        iLu.WriteL(aMessage);
		break;
	case EUsbHostMsErase:
        iLu.EraseL(aMessage);
		break;
	case EUsbHostMsCapacity:
        iLu.CapsL(aMessage);
		break;
	default:
		aMessage.Panic(KUsbHostMsSrvPncCat, EUsbMsPanicIllegalIPC);
		break;
		}
	}


void CUsbHostMsDeviceThread::DoStartServerL(TAny* aPtr)
    {
    __MSFNSLOG
    CActiveScheduler* s = new(ELeave) CActiveScheduler;
    CActiveScheduler::Install(s);

	CUsbHostMsDeviceThread* iThread = (CUsbHostMsDeviceThread*)aPtr;
	CActiveScheduler::Add(iThread);

	iThread->iStatus = KRequestPending;
	iThread->SetActive();

	RThread::Rendezvous(KErrNone);

	//
    // Ready to run
    CActiveScheduler::Start();

    //
    // Cleanup the scheduler
	delete s;
    }


TInt CUsbHostMsDeviceThread::Entry(TAny* aPtr)
	{
    __MSFNSLOG
	CTrapCleanup* cleanup = CTrapCleanup::New();
    if (!cleanup)
        {
        return KErrNoMemory;
        }

    TRAPD(error, DoStartServerL(aPtr));
    delete cleanup;
    return error;
	}


void  CUsbHostMsDeviceThread::RunL()
	{
    __MSFNLOG
	Lock();
	if (!iUsbHostMsDevice || iUsbHostMsDevice->IsActive())
        {
        // Note: In the case of suspended/resuming state we do not want to get
        // woken by the session msg handler repeatedly
        iIsSignalled = EFalse;
        }

	Unlock();

	RMessage2	msg;
	TBool handleMsg = EFalse;

	for(;;)
		{
		Lock();
		if ((iQueueIndex != iDequeueIndex) || iQueueFull)
			{
			if (iUsbHostMsDevice && iUsbHostMsDevice->IsSuspended())
				{
				Unlock();
				SetActive();
				iUsbHostMsDevice->ResumeL(iStatus);
				return;
				}

			msg = iRMessage2[iDequeueIndex];
			handleMsg = ETrue;
			iDequeueIndex++;

			if(iDequeueIndex >= KMaxNumMessage)
				{
				iDequeueIndex = 0;
				}
			if(iQueueFull)
				{
				iQueueFull = EFalse;
				}
			}
		Unlock();
		if (handleMsg)
			{
			HandleMessage(msg);
			handleMsg = EFalse;
			}
		else
			{
			break;
			}
		}
	iStatus = KRequestPending;
	SetActive();
	}


TInt CUsbHostMsDeviceThread::QueueMsg(const RMessage2& aMsg)
	{
    __MSFNLOG
	if (iQueueFull)
		{
		return KErrOverflow;
		}

    Lock();
	iRMessage2[iQueueIndex] = aMsg;
	iQueueIndex++;

	if (iQueueIndex >= KMaxNumMessage)
		{
		iQueueIndex = 0;
		}

	if (iQueueIndex == iDequeueIndex)
		{
		iQueueFull = ETrue;
		}
	Unlock();
	return KErrNone;
	}


void CUsbHostMsDeviceThread::Lock()
	{
    __MSFNLOG
	iMutex.Wait();
	}


void CUsbHostMsDeviceThread::Unlock()
	{
    __MSFNLOG
	iMutex.Signal();
	}


CUsbHostMsDeviceThread::CUsbHostMsDeviceThread(TUint token)
:	CActive(EPriorityStandard),
	iIsSignalled(EFalse),
    iQueueFull(EFalse)
	{
    TName nameBuf;
    nameBuf.Format(_L("Host Ms ThreadMutex%d"), token);
	iMutex.CreateGlobal(nameBuf,EOwnerProcess);
	}

CUsbHostMsDeviceThread::~CUsbHostMsDeviceThread()
	{
	iMutex.Close();
	}

CUsbHostMsDeviceThread* CUsbHostMsDeviceThread::NewL(TUint aToken)
	{
	CUsbHostMsDeviceThread* r = new (ELeave) CUsbHostMsDeviceThread(aToken);
	CleanupStack::PushL(r);
	CleanupStack::Pop();
	return r;
	}


/**
 Handles the request (in the form of a message) received from the client
@param	aMessage	The received message
 */
void CUsbHostMsDeviceThread::HandleMessage(const RMessage2& aMessage)
	{
    __MSFNLOG
	TInt ret = KErrNotReady;
    __HOSTPRINT2(_L(">> HOST DispatchMessageL Function=%d %d"), aMessage.Function(), aMessage.Int3());
	switch (aMessage.Function())
		{
	case EUsbHostMsRegisterInterface:
		TRAP(ret, RegisterInterfaceL(aMessage));
		break;
    case EUsbHostMsInitialiseInterface:
		TRAP(ret, InitialiseInterfaceL(aMessage));
        // CUsbInterfaceHandler::GetMaxLun() completes asynchronously
        if (ret)
            {
            // Error condition needs to be completed
            break;
            }
		return;
	case EUsbHostMsUnRegisterInterface:
		TRAP(ret, UnRegisterInterfaceL(aMessage));
		break;
	case EUsbHostMsRegisterLun:
		TRAP(ret, RegisterLogicalUnitL(aMessage));
		break;
	case EUsbHostMsGetNumLun:
		TRAP(ret, GetNumLunL(aMessage));
		break;
	case EUsbHostMsShutdown:
		ret = Shutdown();
		break;
    default:
        // Try Device Handler and Logical Unit Handler
        TDeviceHandler deviceHandler(*iUsbHostMsDevice);
        TRAP(ret, deviceHandler.HandleMessageL(aMessage));
		break;
		}
    __HOSTPRINT1(_L(">> HOST returning %d"), ret);
    if (aMessage.Function() != EUsbHostMsNotifyChange)
        {
        aMessage.Complete(ret);
        }
	}


/**
Client request to shut down the server

return KErrNone
*/
TInt CUsbHostMsDeviceThread::Shutdown()
    {
    __MSFNLOG
    CActiveScheduler::Stop();
	return KErrNone;
	}

void CUsbHostMsDeviceThread::GetNumLunL(const RMessage2& aMessage)
	{
	__MSFNLOG
    if (!iUsbHostMsDevice)
        {
        User::Leave(KErrNotReady);
        }

	TUint32 maxLun = iUsbHostMsDevice->GetMaxLun() + 1;
	TPtrC8 pLun((TUint8*)&maxLun,sizeof(TUint32));
	aMessage.WriteL(0,pLun);
	}


void CUsbHostMsDeviceThread::RegisterInterfaceL(const RMessage2& aMessage)
	{
	__MSFNLOG

	THostMassStorageConfig msDeviceConfig;
	TPtr8 ptr((TUint8*)&msDeviceConfig,sizeof(THostMassStorageConfig));
	aMessage.ReadL(0, ptr);

    __HOSTPRINT1(_L("RegisterInterfaceL Token=%d "), msDeviceConfig.iInterfaceToken);

	iUsbHostMsDevice = CUsbHostMsDevice::NewL(msDeviceConfig);
	}


void CUsbHostMsDeviceThread::InitialiseInterfaceL(const RMessage2& aMessage)
	{
	__MSFNLOG
    if (!iUsbHostMsDevice)
        {
        User::Leave(KErrNotReady);
        }

	TRAPD(err, iUsbHostMsDevice->InitialiseL(aMessage));
    if (err != KErrNone)
        {
        delete iUsbHostMsDevice;
		iUsbHostMsDevice = NULL;
        User::Leave(err);
        }
	}


void CUsbHostMsDeviceThread::UnRegisterInterfaceL(const RMessage2& aMessage)
    {
    __MSFNLOG
    if (!iUsbHostMsDevice)
        {
        User::Leave(KErrNotReady);
        }

	TRAPD(err, iUsbHostMsDevice->UnInitialiseL());
	delete iUsbHostMsDevice;
	iUsbHostMsDevice = NULL;
    User::LeaveIfError(err);
    }


void CUsbHostMsDeviceThread::RegisterLogicalUnitL(const RMessage2& aMessage)
	{
    if (!iUsbHostMsDevice)
        {
        User::Leave(KErrNotReady);
        }

	TUint32 iLunId = aMessage.Int0() + 1; // Subssessions need a positive value to store in the handles. We represent Luns as LunId+1
	TPtrC8 pLun((TUint8*)&iLunId, sizeof(TUint32));
	aMessage.WriteL(3, pLun);
	iLunId -= 1;	// We represent LunId in MSC from 0 to MaxLun-1 as represented in BOT
	iUsbHostMsDevice->AddLunL(iLunId);
	iUsbHostMsDevice->InitLunL(iLunId);
	}

