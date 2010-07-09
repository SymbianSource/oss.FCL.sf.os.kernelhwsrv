// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\resourceman\d_rescontrolclisync.cpp
// 
//

#include <kernel/kern_priv.h>
#include <drivers/resource_extend.h>
#include <drivers/resourceman.h>
#include "d_rescontrolclisync.h"

#include "resourceman_psl/rescontrol_psl.h"

_LIT(KTestPowerRCName, "D_RESCONTROLCLISYNC.LDD");


const TInt KTestResManLddThreadPriority  = 0x5;
const TUint KResourceId = 16; // DMLSHLGLSPResource
const TInt KResourceMax = 65;
const TInt KResourceMin = 10;

_LIT(KTestResManLddThread, "TestResManLddThread");
_LIT(KTestResManLddHelperThread, "TestResManLddHelperThread");
_LIT(KTestResManLddCallbackThread, "TestResManLddCallbackThread");
_LIT(KResClientName1, "ResTestClient1");
_LIT(KResClientName2, "ResTestClient2");

class DTestResManLdd;
/**
The logical device (factory class) for the resource manager client side test.
*/
class DTestResManLddFactory : public DLogicalDevice
	{
public:
    enum {
        ESignallerCallback,
        EWaiterCallback
    };
	DTestResManLddFactory();
	~DTestResManLddFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8 &aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	
    NFastSemaphore iSemaphore1; // fast semaphore for helper queue
    NFastSemaphore iSemaphore2; // fast semaphore for call back thread

	TInt iResourceClientRegisterCount;
	TDynamicDfcQue* iLddQue;   // dfc que for logical channel (also running the primary ChangeResourceState
	TDynamicDfcQue* iLddHelperQue; // helper dfc que to execute the secondary ChangeResourceState
    TDynamicDfcQue* iCallbackQue;  // dfc que for call back 
 
    DTestResManLdd* iChannel1;
    DTestResManLdd* iChannel2;
    TInt iCallbackState;
    TPowerResourceCb* iAsyncResourceCallback; 
	};

/** Logical channel class for Resource manager test LDD */
class DTestResManLdd : public DLogicalChannel
	{
public:
	DTestResManLdd();
	virtual ~DTestResManLdd();
	// Inherited from DLogicalChannel
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual void HandleMsg(TMessageBase* aMsg);

    DThread* iClientThreadPtr;	
    TRequestStatus* iStatus;
    TUint iResClientId;
    TDfc* iWaitAndChangeResourceDfc;
    
private:
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	TInt DoRequest(TInt aFunction, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	TInt DoCancel(TUint aMask);
	};

void AsyncResourceCallbackFn(TUint /*aClientId*/, TUint /*aResourceId*/, TInt /*aLevel*/, TInt /*aLevelOwnerId*/, TInt aResult, TAny* aParam)
    {
    // call back function, always run on call back dfc queue
    Kern::Printf(">AsyncResourceCallbackFn 0x%x", &(Kern::CurrentThread().iNThread));
    DTestResManLddFactory* pLddFactory = (DTestResManLddFactory*)aParam;
    DTestResManLdd* pLdd = NULL;

    if(pLddFactory->iCallbackState == DTestResManLddFactory::ESignallerCallback)
        {
        pLdd = pLddFactory->iChannel1;
        Kern::Printf("AsyncResourceCallbackFn cond 1 signal [#3.1]"); 
        NKern::FSSignal(&(pLddFactory->iSemaphore1));
        
        Kern::Printf("AsyncResourceCallbackFn cond 1 wait [#3.2]");
        NKern::FSWait(&(pLddFactory->iSemaphore2));
        
        Kern::Printf("AsyncResourceCallbackFn cond 1 wake up [#3.3]");
        }
    else if(pLddFactory->iCallbackState == DTestResManLddFactory::EWaiterCallback)
        {
        pLdd = pLddFactory->iChannel2;
        // aResult should be equal to KErrNone (not KErrCompletion)
        Kern::Printf("AsyncResourceCallbackFn cond 2 r = %d [#2.5]", aResult);
        }
    else
        {
        Kern::Fault("AsyncResourceCallbackFn", __LINE__);
        }
    
    Kern::RequestComplete(pLdd->iClientThreadPtr, pLdd->iStatus, aResult);
    pLdd->iStatus = NULL;    
    Kern::Printf("<AsyncResourceCallbackFn");
    }

void WaitAndChangeResourceDfcFn(TAny* aLdd)
    {
    // helper function to call ChangeResourceState, always run on ldd helper dfc queue
    Kern::Printf(">WaitAndChangeResourceDfcFn 0x%x [#2.1]", &(Kern::CurrentThread().iNThread));
    DTestResManLdd* pLdd = (DTestResManLdd*)aLdd;
    DTestResManLddFactory* pLddFactory = (DTestResManLddFactory*)(pLdd->iDevice);
    
    Kern::Printf(" WaitAndChangeResourceDfcFn - Wait for Semaphore [#2.2]");
    NKern::FSWait(&(pLddFactory->iSemaphore1));
    
    Kern::Printf(" WaitAndChangeResourceDfcFn - ChangeResourceState [#2.3]");
    pLddFactory->iCallbackState = DTestResManLddFactory::EWaiterCallback;
    PowerResourceManager::ChangeResourceState(pLdd->iResClientId, KResourceId, KResourceMax, pLddFactory->iAsyncResourceCallback);
    
    Kern::Printf(" WaitAndChangeResourceDfcFn - signal [#2.4]");
    NKern::FSSignal(&(pLddFactory->iSemaphore2));
    
    delete pLdd->iWaitAndChangeResourceDfc;
    Kern::Printf("<WaitAndChangeResourceDfcFn");
    }

DTestResManLddFactory::DTestResManLddFactory()
	{
	iParseMask=0; // Allow info and pdd, but not units
	iUnitsMask=0;
	// Set version number for this device
	iVersion=RTestResMan::VersionRequired();
	}

DTestResManLddFactory::~DTestResManLddFactory()
	{
    if(iLddQue)
        iLddQue->Destroy();       
    if(iLddHelperQue)
      iLddHelperQue->Destroy(); 
    if(iCallbackQue)
      iCallbackQue->Destroy(); 
    
    if(iAsyncResourceCallback)
      delete iAsyncResourceCallback;
	}

/** Entry point for this driver */
DECLARE_STANDARD_LDD()
	{
	DTestResManLddFactory* p = new DTestResManLddFactory;
	if(!p)
		return NULL;

	TInt r = KErrNone;
	
    r = Kern::DynamicDfcQCreate(p->iLddQue, KTestResManLddThreadPriority, KTestResManLddThread);
    if(r != KErrNone)
        {
        return NULL;
        }
    Kern::Printf("iLddQue 0x%x", p->iLddQue->iThread);
    
    r = Kern::DynamicDfcQCreate(p->iLddHelperQue, KTestResManLddThreadPriority, KTestResManLddHelperThread);
    if(r != KErrNone)
        {
        p->iLddQue->Destroy();  
        return NULL;
        }
    p->iSemaphore1.iOwningThread = (NThreadBase*)(p->iLddHelperQue->iThread);
    Kern::Printf("iSemaphore1 owning thread 0x%x", p->iSemaphore1.iOwningThread);

    r = Kern::DynamicDfcQCreate(p->iCallbackQue, KTestResManLddThreadPriority, KTestResManLddCallbackThread);
    if(r != KErrNone)
        {
        p->iLddQue->Destroy();  
        p->iLddHelperQue->Destroy();          
        return NULL;
        }
    p->iSemaphore2.iOwningThread = (NThreadBase*)(p->iCallbackQue->iThread);
    Kern::Printf("iSemaphore2 owning thread 0x%x", p->iSemaphore2.iOwningThread);

    p->iAsyncResourceCallback = new TPowerResourceCb(AsyncResourceCallbackFn, p, 
            p->iCallbackQue, 5);    
    
#ifdef __SMP__      
    NKern::ThreadSetCpuAffinity((NThread*)(p->iLddQue->iThread), 0);
    NKern::ThreadSetCpuAffinity((NThread*)(p->iLddHelperQue->iThread), 0);
    NKern::ThreadSetCpuAffinity((NThread*)(p->iCallbackQue->iThread), 1);
#endif      
    
	r = DSimulatedPowerResourceController::CompleteResourceControllerInitialisation();
	if(r != KErrNone)
		Kern::Fault("PRM INIT FAILED", __LINE__);
	return p;
	}

/** Second stage constuctor */
TInt DTestResManLddFactory::Install()
	{
   	return(SetName(&KTestPowerRCName));
	}

/** Device capabilities */
void DTestResManLddFactory::GetCaps(TDes8& aDes)const
	{
	// Create a capabilities object
	RTestResMan::TCaps caps;
	caps.iVersion = iVersion;
	// Write it back to user memory
	Kern::InfoCopy(aDes,(TUint8*)&caps,sizeof(caps));
	}


TInt DTestResManLddFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel = new DTestResManLdd();
	if(!aChannel)
		return KErrNoMemory;
	if(!iChannel1)
	    iChannel1 = (DTestResManLdd*)aChannel;
	else if(!iChannel2)
	    iChannel2 = (DTestResManLdd*)aChannel;
	else
	    {
        delete aChannel;
	    return KErrInUse;
	    }
	return KErrNone;
	}

/** Constructor */
DTestResManLdd::DTestResManLdd()
	{
	iClientThreadPtr=&Kern::CurrentThread();
	iResClientId = 0;

	// Increase the DThread's ref count so that it does not close without us
	((DObject*)iClientThreadPtr)->Open();
	}

/** Destructor */
DTestResManLdd::~DTestResManLdd()
	{
	Kern::SafeClose((DObject*&)iClientThreadPtr,NULL);
	}

/** Second stage constructor. */
TInt DTestResManLdd::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& aVer)
	{
   	// Check version
	if (!Kern::QueryVersionSupported(RTestResMan::VersionRequired(),aVer))
		return KErrNotSupported;

	SetDfcQ(((DTestResManLddFactory*)iDevice)->iLddQue);
 	iMsgQ.Receive();
	return KErrNone;
	}

/** Process a message for this logical channel */
void DTestResManLdd::HandleMsg(TMessageBase* aMsg)
	{
	TThreadMessage& m=*(TThreadMessage*)aMsg;
	TInt id=m.iValue;

	if (id==(TInt)ECloseMsg)
		{
		// Channel close.
		m.Complete(KErrNone,EFalse);
		return;
		}
	else if (id==KMaxTInt)
		{
		// DoCancel
		m.Complete(KErrNone,ETrue);
		return;
		}
	else if (id<0)
		{
		// DoRequest
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		TInt r=DoRequest(~id,pS,m.Ptr1(),m.Ptr2());
		if (r!=KErrNone)
			Kern::RequestComplete(iClientThreadPtr,pS,r);
		m.Complete(KErrNone,ETrue);
		}
	else
		{
		// DoControl
		TInt r=DoControl(id,m.Ptr0(),m.Ptr1());
		m.Complete(r,ETrue);
		}
	}

/**
  Process synchronous 'control' requests
*/
TInt DTestResManLdd::DoControl(TInt aFunction, TAny* /*a1*/, TAny* /*a2*/)
	{
	TInt r = KErrNone;

	switch(aFunction)
		{
		case RTestResMan::ERegisterClient:
		    {
            Kern::Printf("RTestResMan::ERegisterClient");		    
			if(iResClientId!=0)
				{
				r = KErrInUse;
				break;
				}
			if(((DTestResManLddFactory*)iDevice)->iResourceClientRegisterCount==0)
			    {
			    r = PowerResourceManager::RegisterClient(iResClientId, KResClientName1);
			    }
			else if(((DTestResManLddFactory*)iDevice)->iResourceClientRegisterCount==1)
			    {
                r = PowerResourceManager::RegisterClient(iResClientId, KResClientName2);
                }                
			else
			    r = KErrInUse;
			
			(((DTestResManLddFactory*)iDevice)->iResourceClientRegisterCount)++;
			
			break;
		    }
	    case RTestResMan::EDeRegisterClient:
	        {
            Kern::Printf("RTestResMan::EDeRegisterClient");          
            if(iResClientId==0)
                {
                r = KErrArgument;
                break;
                }
            r = PowerResourceManager::DeRegisterClient(iResClientId);
            break;
	        }    
        case RTestResMan::EPrintResourceInfo:
            {
            Kern::Printf("RTestResMan::EPrintResourceInfo");          
            TPowerResourceInfoBuf01 info;
            info.SetLength(0);
            Kern::Printf("EPrintResourceInfo");
            r = PowerResourceManager::GetResourceInfo(iResClientId,KResourceId,&info);
            Kern::Printf("EPrintResourceInfo:%S", info().iResourceName);
            break;
            }
		default:
		    {
			r = KErrNotSupported;
			break;
		    }
		}
	return r;

	}


TInt DTestResManLdd::DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* /*a1*/, TAny* /*a2*/)
	{
	TInt r = KErrNone;

	if(r != KErrNone)
		Kern::RequestComplete(iClientThreadPtr, aStatus, r);
	switch(aReqNo)
		{
        case RTestResMan::EWaitAndChangeResource:
            // Queue a dfc which wait for the semaphore and then call ChangeResourceState
            Kern::Printf("RTestResMan::EWaitAndChangeResource 0x%x [#1.1]", &(Kern::CurrentThread().iNThread));            
            if(iStatus)
                {
                r = KErrInUse;
                break;
                }
            iStatus = aStatus;

            iWaitAndChangeResourceDfc = new TDfc(WaitAndChangeResourceDfcFn, this, 
                    ((DTestResManLddFactory*)iDevice)->iLddHelperQue, 5);
            iWaitAndChangeResourceDfc->Enque();

            break;
        case RTestResMan::EChangeResourceAndSignal:	
            // call ChangeResourceState and signal the semaphore
            Kern::Printf("RTestResMan::EChangeResourceAndSignal 0x%x [#1.2]", &(Kern::CurrentThread().iNThread));             
            if(iStatus)
                {
                r = KErrInUse;
                break;
                }
            iStatus = aStatus;
            PowerResourceManager::ChangeResourceState(iResClientId, KResourceId, KResourceMin, 
                    ((DTestResManLddFactory*)iDevice)->iAsyncResourceCallback);
            ((DTestResManLddFactory*)iDevice)->iCallbackState = DTestResManLddFactory::ESignallerCallback;

            break;            
	    default:
	        break;
		}
	return r;
	}
