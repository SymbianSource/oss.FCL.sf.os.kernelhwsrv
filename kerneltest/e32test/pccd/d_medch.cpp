// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\pccd\d_medch.cpp
// This LDD allow simulation of media change on a peripheral bus controller.
// 
//

#include <kernel/kernel.h>
#include <drivers/pbus.h>
#include "d_medch.h"

const TInt KMajorVersionNumber=1;
const TInt KMinorVersionNumber=0;
const TInt KBuildVersionNumber=1;

_LIT(KDFCThreadName,"D_MEDCH_DFC_THREAD");
const TInt KMedChThreadPriority = 27;

class DLddFactoryMedCh : public DLogicalDevice
	{
public:
	DLddFactoryMedCh();
	virtual TInt Install();
	virtual void GetCaps(TDes8 &aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DLddMedCh : public DLogicalChannel
	{
public:
	 DLddMedCh();
	~DLddMedCh();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual void HandleMsg(class TMessageBase *);
private:
	TInt DoRequest(TInt aReqNo,TAny *a1,TAny *a2);
	TInt DoControl(TInt aFunction,TAny *a1,TAny *a2);
private:
	static void MsCBFunc(TAny* aPtr);
private:	
	DPBusSocket* iSocketP;
	DThread* iClient;
	TRequestStatus* iReqStat;
	TDynamicDfcQue* iDfcQ;
	
	NTimer iMsCallBack;
	TInt iMsInterval;

	DPBusSocket::TPBusSimulateMediaState iDelayedOperation;
	};

DECLARE_STANDARD_LDD()
	{
	return new DLddFactoryMedCh;
	}

DLddFactoryMedCh::DLddFactoryMedCh()
/**
 * Constructor
 */
	{

    iParseMask=KDeviceAllowUnit;
	iUnitsMask=0xffffffff;
	iVersion = TVersion(KMajorVersionNumber, KMinorVersionNumber, KBuildVersionNumber);
	}

TInt DLddFactoryMedCh::Install()
/**
 * Install the device driver.
 */
	{

    TPtrC name = _L("MedCh");
	return(SetName(&name));
	}

void DLddFactoryMedCh::GetCaps(TDes8 &aDes) const
/**
 * Return the media change LDD capabilities.
 */
	{

	TCapsMediaChangeV01 caps;
	caps.version = TVersion(KMajorVersionNumber, KMinorVersionNumber, KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&caps,sizeof(caps));
	}

TInt DLddFactoryMedCh::Create(DLogicalChannelBase*& aChannel)
/**
 * Create a channel on the device.
 */
	{

	aChannel = new DLddMedCh;
	return aChannel ? KErrNone : KErrNoMemory;
	}

DLddMedCh::DLddMedCh()
/**
 * Constructor
 */
	: iMsCallBack(MsCBFunc, this)
	{

	iClient = &Kern::CurrentThread();
	((DObject*)iClient)->Open();
	}

DLddMedCh::~DLddMedCh()
/**
 * Destructor
 */
	{ 
	if(iSocketP)
		(void)iSocketP->ControlIO(DPBusSocket::EControlMediaState, (TAny*)DPBusSocket::EPeriphBusMediaNormal, NULL);

	Kern::SafeClose((DObject*&)iClient, NULL);

	if (iDfcQ)
		iDfcQ->Destroy();
	}

TInt DLddMedCh::DoCreate(TInt aUnit, const TDesC8* /*aInfo*/, const TVersion& aVer)
/**
 * Create channel.
 */
	{

	if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber, KMinorVersionNumber, KBuildVersionNumber), aVer))
		return(KErrNotSupported);

	//
	// Obtain the requested socket (as specified by the opened logical unit)
	//
	iSocketP = DPBusSocket::SocketFromId(aUnit);
	if(iSocketP == NULL)
		return(KErrNoMemory);

	if (!iDfcQ)
 			{
 			TInt r = Kern::DynamicDfcQCreate(iDfcQ, KMedChThreadPriority, KDFCThreadName);
			if (r != KErrNone)
 				return r;
#ifdef CPU_AFFINITY_ANY
			NKern::ThreadSetCpuAffinity((NThread*)(iDfcQ->iThread), KCpuAffinityAny);			
#endif

			SetDfcQ(iDfcQ);
 			}	

	iMsgQ.Receive();
	
    return KErrNone;
	}

void DLddMedCh::HandleMsg(TMessageBase* aMsg)
/**
 * Message Handler
 */
    {

    TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt id=m.iValue;
    
	if (id == (TInt)ECloseMsg)
		{
		iMsCallBack.Cancel();
		(void)iSocketP->ControlIO(DPBusSocket::EControlMediaState, (TAny*)DPBusSocket::EPeriphBusMediaNormal, NULL);
		m.Complete(KErrNone, EFalse);
		return;
		}
    else if (id == KMaxTInt)
		{
		// DoCancel
		m.Complete(KErrNone, ETrue);
		return;
		}

    if (id < 0)
		{
		// DoRequest
		TRequestStatus* pS = (TRequestStatus*)m.Ptr0();
		iReqStat = pS;
		TInt r = DoRequest(~id, m.Ptr1(), m.Ptr2());
		if (r != KErrNone)
	    	Kern::RequestComplete(iClient, pS, r);
		m.Complete(KErrNone, ETrue);
		}
    else
		{
		// DoControl
		TInt r = DoControl(id, m.Ptr0(), m.Ptr1());
		if(r != KErrCompletion)
			{
			m.Complete(r, ETrue);
			}
		}
	}

TInt DLddMedCh::DoRequest(TInt aFunction, TAny* a1, TAny* a2)
/**
 * Asynchronous requests
 */
	{

	TInt err = KErrNotSupported;
	
	switch (aFunction)
		{
		case RMedCh::EDelayedDoorOpen:
			{
			const TInt KMsInterval = (TInt)a1;
			iDelayedOperation = DPBusSocket::EPeriphBusDoorOpen;
			err = iMsCallBack.OneShot(NKern::TimerTicks(KMsInterval), ETrue);
			break;
			}
		
		case RMedCh::EDelayedDoorClose:
			{
			const TInt KMsInterval = (TInt)a1;
			const TBool KMediaPresent = (TBool)a2;
			iDelayedOperation = KMediaPresent ? DPBusSocket::EPeriphBusMediaPresent : DPBusSocket::EPeriphBusMediaRemoved;
			err = iMsCallBack.OneShot(NKern::TimerTicks(KMsInterval), ETrue);
			break;
			}
				
		default:
			{
			err = KErrNotSupported;
			break;
			}
		}
	
	return err;
	}

TInt DLddMedCh::DoControl(TInt aFunction,TAny* a1, TAny* /*a2*/)
/**
 * Synchronous requests
 */
	{

	TInt err = KErrNotSupported;
	
	switch (aFunction)
		{
		case RMedCh::EDoorOpen:
			{
			err = iSocketP->ControlIO(DPBusSocket::EControlMediaState, (TAny*)DPBusSocket::EPeriphBusDoorOpen, NULL);
			break;
			}

		case RMedCh::EDoorClose:
			{
			const TBool KMediaPresent = (TBool)a1;

			err = iSocketP->ControlIO(DPBusSocket::EControlMediaState, 
									  (TAny*)(KMediaPresent ? DPBusSocket::EPeriphBusMediaPresent : DPBusSocket::EPeriphBusMediaRemoved),
									  NULL);
			break;
			}

		case RMedCh::EDoorNormal:
			{
			err = iSocketP->ControlIO(DPBusSocket::EControlMediaState, (TAny*)DPBusSocket::EPeriphBusMediaNormal, NULL);
			break;
			}

		case RMedCh::EDoubleDoorOpen:
			{
			err = iSocketP->ControlIO(DPBusSocket::EControlMediaState, (TAny*)DPBusSocket::EPeriphBusMediaDoubleDoorOpen, NULL);
			break;
			}

		default:
			{
			err = KErrNotSupported;
			break;
			}
		}
	
	return err;
	}

void DLddMedCh::MsCBFunc(TAny* aPtr)
/**
 * Delayed Open/Close timer callback
 */
	{
	DLddMedCh& mcldd=*(DLddMedCh*)aPtr;
	TInt err = mcldd.iSocketP->ControlIO(DPBusSocket::EControlMediaState, (TAny*)mcldd.iDelayedOperation, NULL);
   	Kern::RequestComplete(mcldd.iClient, mcldd.iReqStat, err);
	}

