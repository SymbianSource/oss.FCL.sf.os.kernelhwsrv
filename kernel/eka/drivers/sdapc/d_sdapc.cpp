// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL " http://www.eclipse.org/legal/epl-v10.html ".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:


#include <kernel/kernel.h>
#include <drivers/mmc.h>
#include <drivers/sdcard.h>
#include <drivers/sdio/sdio.h>
#include <drivers/d_sdapc.h>

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "locmedia_ost.h"
#ifdef __VC32__
#pragma warning(disable: 4127) // disabling warning "conditional expression is constant"
#endif
#include "d_sdapcTraces.h"
#endif

_LIT(KLddName,"D_SDAPC");

const TInt KMajorVersionNumber=1;
const TInt KMinorVersionNumber=0;
const TInt KBuildVersionNumber=1;

const TInt KSocketNumber = 0;
const TInt KStackNumber  = 0;
const TInt KCardNumber   = 0;

// global Dfc Que
TDynamicDfcQue* gDfcQ;

class DSDAuxiliaryPowerControlFactory : public DLogicalDevice
//
// LDD factory
//
	{
public:
	DSDAuxiliaryPowerControlFactory();
	~DSDAuxiliaryPowerControlFactory();
	virtual TInt Install(); 					//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel); 	//overriding pure virtual
	};


class DSDAuxiliaryPowerControl : public DLogicalChannel
//
// Logical channel
//
	{
public:
	DSDAuxiliaryPowerControl();
	virtual ~DSDAuxiliaryPowerControl();

protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual void HandleMsg(class TMessageBase *);

private:
	static void BusCallBack(TAny* aPtr, TInt aReason, TAny* a1, TAny* a2);
	TInt PowerUpStack();

private:
	DMMCSocket* iSocketP;
	DMMCStack*  iStackP;
	TSDCard*  iCardP;
    
	DThread* iClient;
	
	TPBusCallBack iBusCallBack;		
	DSemaphore* iPowerUpSemaphore;
	TBool iInitialised;
	};


DECLARE_STANDARD_LDD()
	{
	return new DSDAuxiliaryPowerControlFactory;
	}

DSDAuxiliaryPowerControlFactory::DSDAuxiliaryPowerControlFactory()
//
// Constructor
//
	{
    OstTrace0( TRACE_FLOW, DSDAUXILIARYPOWERCONTROLFACTORY_DSDAUXILIARYPOWERCONTROLFACTORY, "DSDAuxiliaryPowerControlFactory::DSDAuxiliaryPowerControlFactory");
	__KTRACE_OPT(KPBUS1, Kern::Printf(">DSDAuxiliaryPowerControlFactory::DSDAuxiliaryPowerControlFactory"));
    iParseMask=KDeviceAllowUnit;
	iUnitsMask=0xffffffff;
	iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	}

TInt DSDAuxiliaryPowerControlFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create a new DSDAuxiliaryPowerControl on this logical device
//
	{
    OstTrace0( TRACE_FLOW, DSDAUXILIARYPOWERCONTROLFACTORY_CREATE, "DSDAuxiliaryPowerControlFactory::Create");
	__KTRACE_OPT(KPBUS1, Kern::Printf(">DSDAuxiliaryPowerControlFactory::Create"));
	aChannel=new DSDAuxiliaryPowerControl;
	return aChannel ? KErrNone : KErrNoMemory;
	}

const TInt KDSDAuxiliaryPowerControlApiThreadPriority = 27;
_LIT(KDSDAuxiliaryPowerControlApiThread,"DSDAuxiliaryPowerControlApiThread");

TInt DSDAuxiliaryPowerControlFactory::Install()
//
// Install the LDD - overriding pure virtual
//
	{
	// Allocate a kernel thread to run the DFC 
	TInt r = Kern::DynamicDfcQCreate(gDfcQ, KDSDAuxiliaryPowerControlApiThreadPriority, KDSDAuxiliaryPowerControlApiThread);

	if (r != KErrNone)
		return r; 	

	return SetName(&KLddName);
	}

void DSDAuxiliaryPowerControlFactory::GetCaps(TDes8& aDes) const
//
// Get capabilities - overriding pure virtual
//
	{
	TCapsTestV01 b;
	b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
	}

/**
  Destructor
*/
DSDAuxiliaryPowerControlFactory::~DSDAuxiliaryPowerControlFactory()
	{
    OstTrace0( TRACE_FLOW, DSDAUXILIARYPOWERCONTROLFACTORY_DSDAUXILIARYPOWERCONTROLFACTORY_DTOR, "DSDAuxiliaryPowerControlFactory::~DSDAuxiliaryPowerControlFactory");
	__KTRACE_OPT(KPBUS1, Kern::Printf(">DSDAuxiliaryPowerControlFactory::~DSDAuxiliaryPowerControlFactory"));
	if (gDfcQ)
		gDfcQ->Destroy();
	}

void DSDAuxiliaryPowerControl::BusCallBack(TAny* aPtr, TInt aReason, TAny* a1, TAny* a2)
	{
	DSDAuxiliaryPowerControl* pTest = (DSDAuxiliaryPowerControl*)aPtr;
	TPBusState busState = (TPBusState) (TInt) a1;
	switch (aReason)
		{
		case TPBusCallBack::EPBusStateChange:
			if (busState != EPBusPoweringUp)
				Kern::SemaphoreSignal(*(pTest->iPowerUpSemaphore));
			break;
		}
	}

TInt DSDAuxiliaryPowerControl::PowerUpStack()
	{
	iBusCallBack.iFunction = BusCallBack;
	iBusCallBack.iPtr=this;
	iBusCallBack.SetSocket(iSocketP->iSocketNumber);
	iBusCallBack.Add();
	TInt r = Kern::SemaphoreCreate(iPowerUpSemaphore, _L("SDPowerUpSem"), 0);
	if (r == KErrNone)
		{
		r = iSocketP->PowerUp();
		if (r == KErrNone)
			Kern::SemaphoreWait(*iPowerUpSemaphore);
		}
	return r;
	}

TInt DSDAuxiliaryPowerControl::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& aVer)
//
// Create channel
//
	{
    OstTrace0( TRACE_FLOW, DSDAUXILIARYPOWERCONTROL_DOCREATE_1, "DSDAuxiliaryPowerControl::DoCreate()");
	__KTRACE_OPT(KPBUS1, Kern::Printf(">DSDAuxiliaryPowerControl::DoCreate()"));

	if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
		return KErrNotSupported;

	//
	// Obtain the appropriate card from the socket/stack
	//
	iSocketP = static_cast<DMMCSocket*>(DPBusSocket::SocketFromId(KSocketNumber));
	if(iSocketP == NULL)
		{
        OstTrace0( TRACE_FLOW, DSDAUXILIARYPOWERCONTROL_DOCREATE_2, "DSDAuxiliaryPowerControl::DoCreate() : Didn't obtain socket");
		__KTRACE_OPT(KPBUS1, Kern::Printf("DSDAuxiliaryPowerControl::DoCreate() : Didn't obtain socket"));
		return KErrNoMemory;
		}

	iStackP = static_cast<DSDStack*>(iSocketP->Stack(KStackNumber));
	if(iStackP == NULL)
		{
        OstTrace0( TRACE_FLOW, DSDAUXILIARYPOWERCONTROL_DOCREATE_3, "DSDAuxiliaryPowerControl::DoCreate() : Didn't obtain stack");
		__KTRACE_OPT(KPBUS1, Kern::Printf("DSDAuxiliaryPowerControl::DoCreate() : Didn't obtain stack"));
		return KErrNoMemory;
		}

	iCardP = static_cast<TSDCard*>(iStackP->CardP(KCardNumber));
	if(iCardP == NULL)
		{
        OstTrace0( TRACE_FLOW, DSDAUXILIARYPOWERCONTROL_DOCREATE_4, "DSDAuxiliaryPowerControl::DoCreate() : Didn't obtain card");
		__KTRACE_OPT(KPBUS1, Kern::Printf("DSDAuxiliaryPowerControl::DoCreate() : Didn't obtain card"));
		return KErrNoMemory;
		}

	SetDfcQ(gDfcQ);
	iMsgQ.Receive();

	// Make sure stack is powered up
	TInt r = PowerUpStack();
	if (r != KErrNone && r != KErrCompletion)
		{
        OstTrace1( TRACE_FLOW, DSDAUXILIARYPOWERCONTROL_DOCREATE_5, "DSDAuxiliaryPowerControl::DoCreate() : Failed To Power up stack, r = %d", r);
		__KTRACE_OPT(KPBUS1, Kern::Printf("DSDAuxiliaryPowerControl::DoCreate() : Failed To Power up stack, r = %d", r));
		return r;
		}
		
	if(!iCardP->IsReady())
		{
        OstTrace0( TRACE_FLOW, DSDAUXILIARYPOWERCONTROL_DOCREATE_6, "DSDAuxiliaryPowerControl::DoCreate() : Card not ready");
		__KTRACE_OPT(KPBUS1, Kern::Printf("DSDAuxiliaryPowerControl::DoCreate() : Card not ready"));
		return KErrNotReady;
		}
    	
	if(!iCardP->ClientsRegistered())
		{		
		iCardP->RegisterClient();
		((DSDIOPsu*)(iSocketP->iVcc))->Lock();
		TBool locked = ((DSDIOPsu*)(iSocketP->iVcc))->IsLocked();
		OstTrace1( TRACE_FLOW, DSDAUXILIARYPOWERCONTROL_DOCREATE_7, "DSDAuxiliaryPowerControl::DoCreate() : PSU IsLocked(), locked = %d", locked);
		__KTRACE_OPT(KPBUS1, Kern::Printf("DSDAuxiliaryPowerControl::DoCreate() : PSU IsLocked(), locked = %d", locked));
		if(!locked)
			return KErrNotReady;
		}
		
	return KErrNone;
	}

DSDAuxiliaryPowerControl::DSDAuxiliaryPowerControl()
//
// Constructor
//
	{
	iClient=&Kern::CurrentThread();
	((DObject*)iClient)->Open();
	}


DSDAuxiliaryPowerControl::~DSDAuxiliaryPowerControl()
//
// Destructor
//
	{
    OstTrace0( TRACE_FLOW, DSDAUXILIARYPOWERCONTROL_DSDAUXILIARYPOWERCONTROL_DTOR, "DSDAuxiliaryPowerControl::~DSDAuxiliaryPowerControl");
	__KTRACE_OPT(KPBUS1, Kern::Printf("DSDAuxiliaryPowerControl::~DSDAuxiliaryPowerControl"));
	iBusCallBack.Remove();

	if (iSocketP)
		iSocketP->ControlIO(DPBusSocket::EControlMediaState, (TAny*)DPBusSocket::EPeriphBusMediaNormal, NULL);

	
	if(iCardP->ClientsRegistered())
		{		
		iCardP->DeregisterClient();
		((DSDIOPsu*)(iSocketP->iVcc))->Unlock();
		}

	iPowerUpSemaphore->Close(NULL);

	Kern::SafeClose((DObject*&)iClient,NULL);
	}

void DSDAuxiliaryPowerControl::HandleMsg(TMessageBase* aMsg)
    {
    TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt id=m.iValue;
    
	if (id==(TInt)ECloseMsg)
		{
		if (iSocketP)
			iSocketP->ControlIO(DPBusSocket::EControlMediaState, (TAny*)DPBusSocket::EPeriphBusMediaNormal, NULL);

		m.Complete(KErrNone, EFalse);
		return;
		}
    else if (id==KMaxTInt)
		{
		// DoCancel
		m.Complete(KErrNone,ETrue);
		return;
		}
	}
