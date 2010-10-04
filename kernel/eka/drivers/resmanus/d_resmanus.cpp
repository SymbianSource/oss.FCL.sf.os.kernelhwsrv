// Copyright (c) 1995-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\resmanus\d_resmanus.cpp
// 
//

// LDD for Resource Manager user side API
#include "resmanus.h"
#include <kernel/kern_priv.h>
#include <kernel/kernel.h>
#include <e32hal.h>
#include <e32uid.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include <e32def_private.h>
#include <drivers/resource_extend.h>

#ifdef RESOURCE_MANAGER_SIMULATED_PSL
#include "rescontrol_psl.h"
#endif

#ifdef PRM_US_INSTRUMENTATION_MACRO
#include <drivers/resmanus_trace.h>
#endif

#ifdef PRM_ENABLE_EXTENDED_VERSION2
_LIT(KResManUsThreadName,"ResManUsExtendedCoreLddThread");
#elif defined (PRM_ENABLE_EXTENDED_VERSION)
_LIT(KResManUsThreadName,"ResManUsExtendedLddThread");
#else
_LIT(KResManUsThreadName,"ResManUsLddThread");
#endif

#define RESMANUS_FAULT()	Kern::Fault("RESMANUS",__LINE__)

const TInt KResManUsThreadPriority = 24;
const TInt KResManUsRegistrationPriority = 5; // Arbitrary! Can be 0-7, 7 is highest
const TInt KResManCallBackPriority = 5; // Arbitrary! Can be 0-7, 7 is highest

//Macro to return appropriate request type.
#define GET_USER_REQUEST(request, buffer, type)						\
	{																\
	if(type == EGetState)											\
		request = ((TTrackGetStateBuf*)buffer)->iRequest;			\
	else if(type == ESetState)										\
		request = ((TTrackSetStateBuf*)buffer)->iRequest;			\
	else															\
		request = ((TTrackNotifyBuf*)buffer)->iRequest;				\
	}

//Macro to call the correct destructor for the tracking buffer deletion
#define DELETE_TRACKING_BUFFER(tracker,buf)							\
	{																\
	switch(tracker->iType)											\
		{															\
		case EGetState:												\
			delete (TTrackGetStateBuf *)(buf);						\
			break;													\
		case ESetState:												\
			delete (TTrackSetStateBuf *)(buf);						\
			break;													\
		case ENotify:												\
			delete (TTrackNotifyBuf *)(buf);						\
			break;													\
		default:													\
			__ASSERT_ALWAYS(0,RESMANUS_FAULT());					\
		}															\
	}
/***************************************************************************************
	class TTrackGetStateBuf
 ***************************************************************************************/
TTrackGetStateBuf::TTrackGetStateBuf(TPowerResourceCbFn aFn, TAny* aPtr,
						       TDfcQue* aQue, TInt aPriority)
							   :	iCtrlBlock(aFn, aPtr, aQue, aPriority)
	{
	iRequest = NULL;
	}

/***************************************************************************************
	class TTrackSetStateBuf
 ***************************************************************************************/
TTrackSetStateBuf::TTrackSetStateBuf(TPowerResourceCbFn aFn, TAny* aPtr,
						       TDfcQue* aQue, TInt aPriority)
							   :	iCtrlBlock(aFn, aPtr, aQue, aPriority)
	{
	iRequest = NULL;
	}

/***************************************************************************************
	class TTrackNotifyBuf
 ***************************************************************************************/
TTrackNotifyBuf::TTrackNotifyBuf(TPowerResourceCbFn aFn, TAny* aPtr,
								 TDfcQue* aQue, TInt aPriority)
							   :	iNotifyBlock(aFn, aPtr, aQue, aPriority)
	{
	iRequest = NULL;
	}

TTrackNotifyBuf::~TTrackNotifyBuf()
	{
	if(iRequest)
		Kern::DestroyClientRequest(iRequest);
	}

TTrackSetStateBuf::~TTrackSetStateBuf()
	{
	if(iRequest)
		Kern::DestroyClientRequest(iRequest);
	}

TTrackGetStateBuf::~TTrackGetStateBuf()
	{
	if(iRequest)
		Kern::DestroyClientRequest(iRequest);
	}
	
/***************************************************************************************
	class DDeviceResManUs
 ***************************************************************************************/
DDeviceResManUs::DDeviceResManUs()
// Constructor
    {
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("> DDeviceResManUs::DDeviceResManUs()"));
    iParseMask=KDeviceAllowAll&~KDeviceAllowUnit; // Allow info and pdd, but not units
    iUnitsMask=0;
    iVersion=TVersion(KResManUsMajorVersionNumber,
		      KResManUsMinorVersionNumber,
		      KResManUsBuildVersionNumber);
    }

DDeviceResManUs::~DDeviceResManUs()
// Destructor
    {
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("> DDeviceResManUs::~DDeviceResManUs()"));
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
	iSharedDfcQue->Destroy();
#else
	delete iSharedDfcQue;
#endif
	}

TInt DDeviceResManUs::Install()
// Install the device driver.
    {
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("> DDeviceResManUs::Install()"));
	// Create the message queue and initialise the DFC queue pointer
#ifndef RESOURCE_MANAGER_SIMULATED_PSL
	TInt r=Kern::DfcQInit(iSharedDfcQue,KResManUsThreadPriority,&KResManUsThreadName);
#else
	TInt r = Kern::DynamicDfcQCreate(iSharedDfcQue,KResManUsThreadPriority,KResManUsThreadName);
#endif
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("> DfcQCreate r  = %d", r));
	if(r!=KErrNone)
		return r;

#ifdef CPU_AFFINITY_ANY
        NKern::ThreadSetCpuAffinity((NThread*)(iSharedDfcQue->iThread), KCpuAffinityAny);
#endif
	r = SetName(&KLddRootName);
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("> SetName, r  = %d", r));
	return r;
    }


void DDeviceResManUs::GetCaps(TDes8& aDes) const
// Return the ResManUs capabilities.
    {
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("> DDeviceResManUs::GetCaps(TDes8& aDes) const"));
    TPckgBuf<TCapsDevResManUs> b;
    b().version=TVersion(KResManUsMajorVersionNumber,
			 KResManUsMinorVersionNumber,
			 KResManUsBuildVersionNumber);
    Kern::InfoCopy(aDes,b);
    }


TInt DDeviceResManUs::Create(DLogicalChannelBase*& aChannel)
// Create a channel on the device.
    {
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("> DDeviceResManUs::Create(DLogicalChannelBase*& aChannel)"));
	if(iOpenChannels>=KMaxNumChannels)
		return KErrOverflow;
    aChannel=new DChannelResManUs;
    return aChannel?KErrNone:KErrNoMemory;
    }


/***************************************************************************************
	class DChannelResManUs
 ***************************************************************************************/
DChannelResManUs::DChannelResManUs() 
// Constructor
    {
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("> DChannelResManUs::DChannelResManUs()"));
    iClient=&Kern::CurrentThread();
	// Increase the DThread's ref count so that it does not close without us
	iClient->Open();
    }


DChannelResManUs::~DChannelResManUs()
// Destructor
    {
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("> DChannelResManUs::~DChannelResManUs()"));

	// Cancel any outstanding requests
	//
	// For each tracker (Get, Set and notify)
	// 
	if(iGetStateTracker != NULL)
		{
		CancelTrackerRequests(iGetStateTracker,EFalse,0,NULL); // EFalse,0, to ignore resource IDs
		RemoveTrackingControl(iGetStateTracker);
		}
	if(iSetStateTracker != NULL)
		{
		CancelTrackerRequests(iSetStateTracker,EFalse,0,NULL); // EFalse,0, to ignore resource IDs
		RemoveTrackingControl(iSetStateTracker);
		}
	if(iListenableTracker != NULL)
		{
		CancelTrackerRequests(iListenableTracker,EFalse,0,NULL); // EFalse,0, to ignore resource IDs
		RemoveTrackingControl(iListenableTracker);
		}

	delete iUserNameUsed;
	delete iResourceDependencyIds;
	delete iClientNamesResCtrl;
	delete iResourceInfoResCtrl;

	// decrement the DThread's reference count
	Kern::SafeClose((DObject*&)iClient, NULL);
    }


static void AsyncCallBackFn(TUint aClient, TUint aResourceId, TInt aLevel, TInt aLevelOwnerId, TInt aResult, TAny* aTrackingBuffer)
	{
// Callback function for asynchronous requests
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("> AsyncCallBackFn aClient=0x%x aResourceId=0x%x, aLevel=0x%x, aResult=0x%x, aTrackingBuffer=0x%x\n",aClient, aResourceId, aLevel, aResult, aTrackingBuffer));
	TTrackingBuffer* buffer = ((TTrackingBuffer*)aTrackingBuffer);
	TTrackingControl* tracker = buffer->GetTrackingControl();
	__ASSERT_ALWAYS((tracker!=NULL),RESMANUS_FAULT());
	DChannelResManUs* channel = tracker->iOwningChannel;

#ifdef PRM_US_INSTRUMENTATION_MACRO
	if(tracker->iType==EGetState)
		{
		PRM_US_GET_RESOURCE_STATE_END_TRACE;
		}
	else if(tracker->iType==ESetState)
		{
		PRM_US_SET_RESOURCE_STATE_END_TRACE;
		}
#endif
	if(tracker->iType == EGetState)
		{
		TTrackGetStateBuf* stateBuf = (TTrackGetStateBuf*)aTrackingBuffer;
		if(aResult==KErrNone)
			{
			// Write the state value to the user-supplied variable
			stateBuf->iRequest->Data1() = aLevel;
			stateBuf->iRequest->Data2() = aLevelOwnerId;
			}
		Kern::QueueRequestComplete(channel->iClient, ((TTrackGetStateBuf*)buffer)->iRequest, aResult);
		}
	else if(tracker->iType == ESetState)
		{
		Kern::QueueRequestComplete(channel->iClient, ((TTrackSetStateBuf*)buffer)->iRequest, aResult);
		}
	// Once notified of a change in a resource state, must cancel the notification
	// request in the Resource Controller to give the client the appearance of a 
	// 'one'shot' type of operation.
	else if(tracker->iType==ENotify)
		{
#ifdef PRM_ENABLE_EXTENDED_VERSION
		if(((TInt)aClient==KDynamicResourceDeRegistering)&&(aResourceId&KIdMaskDynamic))  
			{
			// Resource has de-registered from Resource Controller, so can't expect any more notifications
			// of this type. Cancellation of notifications (i.e. invoke Resource Controller) and transfer of
			// buffers to free queue (for both conditional and unconditional notifications) is already done.
			// To distinguish removal of a dynamic resource, hijack aResult (the value used when completing
			// the user-side TRequestStatus object) and set it to KErrDisconnected.
			aResult = KErrDisconnected;
			}

#endif
		TInt r = (channel->iPddPtr)->CancelNotification(channel->ClientHandle(),aResourceId,
										((TTrackNotifyBuf*)buffer)->iNotifyBlock);
		__ASSERT_ALWAYS((r == KErrCancel),RESMANUS_FAULT());
		Kern::QueueRequestComplete(channel->iClient, ((TTrackNotifyBuf*)buffer)->iRequest, aResult);
		}

	// Return the tracking buffer to the free queue
	channel->FreeTrackingBuffer(buffer);
	}

TInt DChannelResManUs::GetValidName(const TDesC8* aInfo)
	{
// Extract a usable name from that supplied by the client
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DChannelResManUs::GetValidName"));
	TInt err=KErrNone;
	if(aInfo)
		{
		DThread* thread = &Kern::CurrentThread();
		TInt nameLen = Kern::ThreadGetDesLength(thread, aInfo);
		if(nameLen<0)
			return nameLen; // return error code
		iNameProvidedLength = nameLen;
		if(nameLen > MAX_CLIENT_NAME_LENGTH)
			err=KErrBadName;
		else
			{
			nameLen = (nameLen<=MAX_NAME_LENGTH_IN_RESMAN) ? nameLen : MAX_NAME_LENGTH_IN_RESMAN;
			if((iUserNameUsed = HBuf8::New(nameLen))==NULL)
				return KErrNoMemory;
			err = Kern::ThreadDesRead(thread,aInfo,*iUserNameUsed,0);
			if(err!=KErrNone)
				return err;
			}
		}
	else
		err=KErrBadName;
	return err;
	}

TInt DChannelResManUs::RequestUserHandle(DThread* aThread, TOwnerType aType)
// Called when a user thread requests a handle to this channel
    {
    // Make sure that only our client can get a handle
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DChannelResManUs::RequestUserHandle"));
    if (aType!=EOwnerThread || aThread!=iClient)
        return KErrAccessDenied;
    return KErrNone;
    }

void DChannelResManUs::RegistrationDfcFunc(TAny* aChannel)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DChannelResManUs::RegistrationDfcFunc"));
	// DFC function invoked for registration with Resource Controller
	DChannelResManUs* channel = (DChannelResManUs*)aChannel;
	// RegisterProxyClient(TUint& aProxyId, const TDesC& aName);
	TUint uintVal=0;
	TInt r = KErrNone;
	__ASSERT_ALWAYS((r==KErrNone),RESMANUS_FAULT());

	r=(channel->iPddPtr)->RegisterProxyClient(uintVal,*((TDesC8*)(channel->iUserNameUsed)));
	if(r!=KErrNone)
		{
		// Registration failed
		// Ensure that the client-side flag is cleared in uintVal
		// so the failure can be detected in DoCreate
		uintVal &= ~USER_SIDE_CLIENT_BIT_MASK; // Copied from rescontrol_export
		}
	channel->SetClientHandle((TInt)uintVal);
	NKern::FSSignal(channel->iFastSem);
	}


TInt DChannelResManUs::RegisterWithResCtrlr()
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DChannelResManUs::RegisterWithResCtrlr"));
	TInt r = KErrNone;
	// Initialise the channel's fast semaphore
	iFastSem = new NFastSemaphore();
	if(iFastSem == NULL)
		r = KErrNoMemory;
	else
		{
		iFastSem->iOwningThread = (NThreadBase*)NKern::CurrentThread();

		// Attempt to perform registration with the Resource Controller on behalf of the client.
		SetDfcQ(((DDeviceResManUs*)(iDevice))->iSharedDfcQue);
		TDfc tempDfc(RegistrationDfcFunc, this, iDfcQ, KResManUsRegistrationPriority);

		// Block this thread until the DFC has executed
		tempDfc.Enque();
		NKern::FSWait(iFastSem);
		// Have finished with iFastSem
		delete iFastSem;

		// Registration complete - check success
		if(!(USER_SIDE_CLIENT_BIT_MASK & ClientHandle()))
			{
			// Registration failed
			r = KErrCouldNotConnect;	
			}
		// Start receiving messages ...
		iMsgQ.Receive();
		}
	return r;
	}

TInt DChannelResManUs::DoCreate(TInt /*aUnit*/,
                                const TDesC8* aInfo, 
                                const TVersion &aVer)
// Create the channel from the passed info.
    {
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("> DChannelResManUs::DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion &aVer)"));

	TInt r = KErrNone;
	iPddPtr = ((DUserSideProxyInterface*)iPdd)->iController;
	// Check client has appropriate capabilities
	if(!Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt,__PLATSEC_DIAGNOSTIC_STRING("Checked by DDevicePowerRsrc::Create")))
		return KErrPermissionDenied;

	// Check software version
	if (!Kern::QueryVersionSupported(TVersion(KResManUsMajorVersionNumber,
			 KResManUsMinorVersionNumber,
			 KResManUsBuildVersionNumber),
				     aVer))
		return KErrNotSupported;

	// Implementation note: if this method fails, the destructor will be invoked
	// as part of which all successfully-allocated memory will be freed. Therefore,
	// no memory will be explicitly freed in the event of failure in the code which follows.
	
	// Allocate the arrays used for acquiring client, resource and dependency information
	if((iClientNamesResCtrl = HBuf8::New(KNumClientNamesResCtrl * sizeof(TPowerClientInfoV01)))==NULL)
		return KErrNoMemory;
	if((iResourceInfoResCtrl = HBuf8::New(KNumResourceInfoResCtrl * sizeof(TPowerResourceInfoV01)))==NULL)
		return KErrNoMemory;
	if((iResourceDependencyIds = HBuf8::New(KNumResourceDependencies * sizeof(SResourceDependencyInfo)))==NULL)
		return KErrNoMemory;
	// Obtain the channel name to use
	if((r=GetValidName(aInfo))!=KErrNone)
		return r;
#ifdef PRM_ENABLE_EXTENDED_VERSION
	iResDepsValid = 0;
#endif

#ifdef PRM_US_INSTRUMENTATION_MACRO
	PRM_US_OPEN_CHANNEL_START_TRACE;	 
#endif

	// Set up the request tracking support
	iGetStateTracker = new TTrackingControl();
	iSetStateTracker = new TTrackingControl();;
	iListenableTracker = new TTrackingControl();
	if((iGetStateTracker==NULL) || (iSetStateTracker==NULL) || (iListenableTracker==NULL))
		return KErrNoMemory;

	// Register with the Resource Controller
	r = RegisterWithResCtrlr();

#ifdef PRM_US_INSTRUMENTATION_MACRO
	PRM_US_OPEN_CHANNEL_END_TRACE;
#endif

    return r;
    }

//Override sendMsg to allow data copy in the context of client thread for WDP.
TInt DChannelResManUs::SendMsg(TMessageBase* aMsg)
	{
	TThreadMessage& m = *(TThreadMessage*)aMsg;
	TInt id = m.iValue;
	TInt r = KErrNone;
	if (id != (TInt)ECloseMsg && id != KMaxTInt)
		{
		if (id<0)
			{
			TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
			r = SendRequest(aMsg);
			if (r != KErrNone)
				Kern::RequestComplete(pS,r);
			}
		else
			r = SendControl(aMsg);
		}
	else
		r = DLogicalChannel::SendMsg(aMsg);
	return r;
	}

TInt DChannelResManUs::SendRequest(TMessageBase* aMsg)
	{
	TThreadMessage& m = *(TThreadMessage*)aMsg;
	TInt id = ~m.iValue;
	TRequestStatus* pS = (TRequestStatus*)m.Ptr0();
	TInt r = KErrNone;
	TTrackingBuffer *trackBuf = NULL;
	TUint parms[4];
	TPowerResourceCb *callBack = NULL;
	DPowerResourceNotification *prn;

	switch(id)
		{
		case RBusDevResManUs::EChangeResourceState:
			{
#ifdef _DUMP_TRACKERS
			DumpTracker(iSetStateTracker);
#endif
			r = GetAndInitTrackingBuffer(iSetStateTracker, trackBuf, (TUint)m.Ptr1(), pS);
			if( r != KErrNone)
				return r;
			callBack = &(((TTrackSetStateBuf*)trackBuf)->iCtrlBlock);
			new (callBack) TPowerResourceCb(&AsyncCallBackFn, (TAny*)trackBuf, iDfcQ, KResManCallBackPriority);
			parms[0] = (TUint)m.Ptr2();
			parms[1] = (TUint)callBack;
			m.iArg[2] = &(parms[0]);
			break;
			}
		case RBusDevResManUs::EGetResourceState:
			{
			__ASSERT_ALWAYS(m.Ptr2() != NULL, RESMANUS_FAULT());
			umemget32(&(parms[0]), m.Ptr2(), 3*sizeof(TInt));
#ifdef _DUMP_TRACKERS
			DumpTracker(iGetStateTracker);
#endif
			r = GetStateBuffer(iGetStateTracker, trackBuf, (TUint)m.Ptr1(), (TInt*)parms[1], (TInt*)parms[2], callBack, pS);
			if(r != KErrNone)
				return r;
			parms[3] = (TUint)callBack;
			m.iArg[2] = &(parms[0]);
			break;
			}
		case RBusDevResManUs::ERequestChangeNotification:
			{
			__ASSERT_ALWAYS(m.Ptr1() != NULL, RESMANUS_FAULT());
			r = GetAndInitTrackingBuffer(iListenableTracker, trackBuf, (TUint)m.Ptr1(), pS);
			if(r != KErrNone)
				return r;
			prn = &(((TTrackNotifyBuf*)trackBuf)->iNotifyBlock);
			new (prn) DPowerResourceNotification(&AsyncCallBackFn, (TAny*)trackBuf, iDfcQ, KResManCallBackPriority);
			m.iArg[2] = (TAny*)prn;
			break;
			}
		case RBusDevResManUs::ERequestQualifiedChangeNotification:
			{
			__ASSERT_ALWAYS(m.Ptr1() != NULL, RESMANUS_FAULT());
			__ASSERT_ALWAYS(m.Ptr2() != NULL, RESMANUS_FAULT());
			umemget32(&(parms[0]), m.Ptr2(), 2*sizeof(TUint));
			m.iArg[2] = &parms[0];
			r = GetAndInitTrackingBuffer(iListenableTracker, trackBuf, (TUint)parms[0], pS);
			if(r != KErrNone)
				return r;
			prn = &(((TTrackNotifyBuf*)trackBuf)->iNotifyBlock);
			new (prn) DPowerResourceNotification(&AsyncCallBackFn, (TAny*)trackBuf, iDfcQ, KResManCallBackPriority);
			parms[2] = (TUint)prn;
			break;
			}
		default:
			{
			return KErrNotSupported;
			}
		}

	if(r == KErrNone)
		r = DLogicalChannel::SendMsg(aMsg);
	if(r != KErrNone)
		FreeTrackingBuffer(trackBuf);
	return r;
	}


TInt DChannelResManUs::SendControl(TMessageBase* aMsg)
	{
	TThreadMessage& m = *(TThreadMessage*)aMsg;
	TInt id = m.iValue;
	TInt param1 = 0;
	TUint parms[4];
	TAny* a1 = m.Ptr0();
	TAny* a2 = m.Ptr1();
	TAny* ptr1 = NULL;
	switch(id)
		{
		case RBusDevResManUs::EInitialise:
			{
			__ASSERT_ALWAYS(a1 != NULL, RESMANUS_FAULT());
			TUint8 stateRes[3];
			umemget(&(stateRes[0]), a1, 3*sizeof(TUint8));
			m.iArg[0] = &(stateRes[0]);
			break;
			}
		case RBusDevResManUs::EGetNoOfResources:
		case RBusDevResManUs::EGetResourceControllerVersion:
			{
			__ASSERT_ALWAYS(a1 != NULL, RESMANUS_FAULT());
			m.iArg[0] = &param1;
			break;
			}
		case RBusDevResManUs::EGetNoOfClients:
		case RBusDevResManUs::EGetNumClientsUsingResource:
			{
			__ASSERT_ALWAYS(a1 != NULL, RESMANUS_FAULT());
			__ASSERT_ALWAYS(a2 != NULL, RESMANUS_FAULT());
			umemget32(&(parms[0]), a2, 3*sizeof(TUint));
			m.iArg[1]  = &(parms[0]);
			m.iArg[0] = &param1;
			break;
			}
		case RBusDevResManUs::EGetNumResourcesInUseByClient:
			{
			__ASSERT_ALWAYS(a1 != NULL, RESMANUS_FAULT());
			__ASSERT_ALWAYS(a2 != NULL, RESMANUS_FAULT());
			TBuf8 <MAX_NAME_LENGTH_IN_RESMAN> clientName;
			Kern::KUDesGet(clientName, *(TDesC8*)m.Ptr0());
			m.iArg[0] = (TAny*)&clientName;
			umemget32(&(parms[0]), m.Ptr1(), 2*sizeof(TUint));
			param1 = parms[1];
			m.iArg[1] = &param1;
			break;
			}
		case RBusDevResManUs::EGetResourceIdByName:
			{
			__ASSERT_ALWAYS(a1 != NULL, RESMANUS_FAULT());
			__ASSERT_ALWAYS(a2 != NULL, RESMANUS_FAULT());
			TBuf8 <MAX_NAME_LENGTH_IN_RESMAN> resourceName;
			Kern::KUDesGet(resourceName, *(TDesC8*)m.Ptr0());
			m.iArg[0] = (TAny*)&resourceName;
			m.iArg[1] = &param1;
			break;
			}
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
		case RBusDevResManUs::EGetNumCandidateAsyncResources:
		case RBusDevResManUs::EGetNumCandidateSharedResources:
			{
			__ASSERT_ALWAYS(a1 != NULL, RESMANUS_FAULT());
			m.iArg[0] = &param1;
			break;
			}
		case RBusDevResManUs::EGetCandidateAsyncResourceId:
		case RBusDevResManUs::EGetCandidateSharedResourceId:
			{
			__ASSERT_ALWAYS(a2 != NULL, RESMANUS_FAULT());
			m.iArg[1] = &param1;
			break;
			}
#endif
		case RBusDevResManUs::EGetNumDependentsForResource:
			{
			__ASSERT_ALWAYS(a1 != NULL, RESMANUS_FAULT());
			__ASSERT_ALWAYS(a2 != NULL, RESMANUS_FAULT());
			umemget32(&(parms[0]), m.Ptr1(), 2*sizeof(TUint));
			m.iArg[1] = &(parms[0]);
			m.iArg[0] = &param1;
			break;
			}
		case RBusDevResManUs::EGetDependentsIdForResource:
			{
			__ASSERT_ALWAYS(a1 != NULL, RESMANUS_FAULT());
			__ASSERT_ALWAYS(a2 != NULL, RESMANUS_FAULT());
			umemget32(&(parms[0]), m.Ptr1(), 3*sizeof(TUint));
			TInt len, maxLen;
			ptr1 = (TAny*)parms[1];
			Kern::KUDesInfo(*(const TDesC8*)parms[1], len, maxLen);
			umemget32(&param1, m.Ptr0(), sizeof(TUint));
			if((maxLen - len) < (TInt)(param1 * sizeof(SResourceDependencyInfo)))
				{
				return KErrArgument;
				}
			m.iArg[0] = &param1;
			m.iArg[1] = &(parms[0]);
			break;
			}
		case RBusDevResManUs::EGetResourceInfo:
			{
			__ASSERT_ALWAYS(a1 != NULL, RESMANUS_FAULT());
			__ASSERT_ALWAYS(a2 != NULL, RESMANUS_FAULT());
			TResourceInfoBuf buf;
			m.iArg[1] = &buf;
			break;
			}
		case RBusDevResManUs::EGetAllResourcesInfo:
			{
			__ASSERT_ALWAYS(a1 != NULL, RESMANUS_FAULT());
			__ASSERT_ALWAYS(a2 != NULL, RESMANUS_FAULT());
			umemget32(&(parms[0]), m.Ptr1(), 2*sizeof(TUint));
			ptr1 = (TAny*)parms[0];
			umemget32(&param1, (TAny*)parms[0], sizeof(TUint));
			parms[0]  =(TUint)&param1;
			RSimplePointerArray<TResourceInfoBuf> infoPtrs;
			umemget(&infoPtrs, m.Ptr0(), sizeof(RSimplePointerArray<TResourceInfoBuf>));
			if((infoPtrs.Count() < 0) || (infoPtrs.Count() < param1))
				return KErrArgument;
			m.iArg[1] = &(parms[0]);
			break;
			}
		case RBusDevResManUs::EGetInfoOnClientsUsingResource:
			{
			__ASSERT_ALWAYS(a1 != NULL, RESMANUS_FAULT());
			__ASSERT_ALWAYS(a2 != NULL, RESMANUS_FAULT());
			umemget32(&parms[0], m.Ptr1(), 4*sizeof(TUint));
			ptr1 = (TAny*)parms[0];
			umemget32(&param1, (TAny*)parms[0], sizeof(TUint));
			parms[0] = (TUint)&param1;
			RSimplePointerArray<TClientInfoBuf>infoPtrs;
			umemget(&infoPtrs, m.Ptr0(), sizeof(RSimplePointerArray<TClientInfoBuf>));
			if((infoPtrs.Count() < 0) || (infoPtrs.Count() < param1))
				return KErrArgument;
			m.iArg[1] = &(parms[0]);
			break;
			}
		case RBusDevResManUs::EGetNamesAllClients:
			{
			__ASSERT_ALWAYS(a1 != NULL, RESMANUS_FAULT());
			__ASSERT_ALWAYS(a2 != NULL, RESMANUS_FAULT());
			umemget32(&parms[0], m.Ptr1(), 4*sizeof(TUint));
			ptr1 = (TAny*)parms[0];
			umemget32(&param1, (TAny*)parms[0], sizeof(TUint));
			parms[0] = (TUint)&param1;
			RSimplePointerArray<TClientName> infoPtrs;
			umemget(&infoPtrs, m.Ptr0(), sizeof(RSimplePointerArray<TClientName>));
			if((infoPtrs.Count() < 0) || (infoPtrs.Count() < param1))
				return KErrArgument;
			m.iArg[1] = &(parms[0]);
			break;
			}
		case RBusDevResManUs::EGetInfoOnResourcesInUseByClient:
			{
			__ASSERT_ALWAYS(a1 != NULL, RESMANUS_FAULT());
			__ASSERT_ALWAYS(a2 != NULL, RESMANUS_FAULT());
			TBuf8 <MAX_NAME_LENGTH_IN_RESMAN> clientName;
			Kern::KUDesGet(clientName, *(TDesC8*)m.Ptr0());
			m.iArg[0] = (TAny*)&clientName;
			umemget32(&parms[0], m.Ptr1(), 3*sizeof(TUint));
			ptr1 = (TAny*)parms[0];
			umemget32(&param1, (TAny*)parms[0], sizeof(TUint));
			parms[0] = (TUint)&param1;
			RSimplePointerArray<TResourceInfoBuf> infoPtrs;
			umemget(&infoPtrs, (TAny*)parms[1], sizeof(RSimplePointerArray<TResourceInfoBuf>));
			if((infoPtrs.Count() < 0) || (infoPtrs.Count() < param1))
				return KErrArgument;
			m.iArg[1] = &(parms[0]);
			break;
			}
		}

	TInt r = DLogicalChannel::SendMsg(aMsg);
	if(r != KErrNone)
		return r;

	switch(id)
		{
		case RBusDevResManUs::EGetNoOfResources:
		case RBusDevResManUs::EGetNoOfClients:
		case RBusDevResManUs::EGetNumClientsUsingResource:
		case RBusDevResManUs::EGetResourceControllerVersion:
		case RBusDevResManUs::EGetNumDependentsForResource:
			{
			umemput32(a1, (TAny*)&param1, sizeof(TUint));
			break;
			}
		case RBusDevResManUs::EGetNumResourcesInUseByClient:
			{
			umemput32((TAny*)parms[0], (TAny*)&param1, sizeof(TUint));
			break;
			}
		case RBusDevResManUs::EGetResourceIdByName:
			{
			umemput32(a2, (TAny*)&param1, sizeof(TUint));
			break;
			}
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
		case RBusDevResManUs::EGetNumCandidateAsyncResources:
		case RBusDevResManUs::EGetNumCandidateSharedResources:
			{
			umemput32(a1, (TAny*)&param1, sizeof(TUint));
			break;
			}
		case RBusDevResManUs::EGetCandidateAsyncResourceId:
		case RBusDevResManUs::EGetCandidateSharedResourceId:
			{
			umemput32(a2, (TAny*)&param1, sizeof(TUint));
			break;
			}
#endif
		case RBusDevResManUs::EGetDependentsIdForResource:
			{
			r = Kern::ThreadDesWrite(iClient,(TAny*)ptr1, (const TDesC8&)*(SResourceDependencyInfo*)parms[1], 0);
			if(r == KErrOverflow) //This is done to retain the error as per API spec
				r = KErrArgument;
			break;
			}
		case RBusDevResManUs::EGetResourceInfo:
			{
			Kern::KUDesPut(*(TDes8*)a2, (const TDesC8&)*(TResourceInfoBuf*)m.Ptr1());
			break;
			}
		case RBusDevResManUs::EGetAllResourcesInfo:
			{
			TUint numToCopy;
			RSimplePointerArray<TResourceInfoBuf> infoPtrs;
			umemget(&infoPtrs, a1, sizeof(RSimplePointerArray<TResourceInfoBuf>));
			numToCopy = (infoPtrs.Count() < param1) ? infoPtrs.Count() : param1;
			umemput32(ptr1, (TAny*)&param1, sizeof(TUint));
			TResourceInfoBuf** entriesAddr = infoPtrs.Entries();
			TInt* entryPtr = (TInt*)entriesAddr;
			TPowerResourceInfoV01 *currRes = (TPowerResourceInfoV01*)iResourceInfoResCtrl->Ptr();
			TResourceInfoBuf* clientAddr;
			TResourceInfoBuf tempInfo;
			for(TUint index = 0; index < numToCopy; index++)
				{
				umemget32(&clientAddr, entryPtr, sizeof(TResourceInfoBuf*));
				entryPtr++;
				r = ExtractResourceInfo(currRes, tempInfo);
				if(r != KErrNone)
					return r;
				umemput((TAny*)clientAddr, (TAny*)&(tempInfo), tempInfo.Length());
				currRes++;
				}
			break;
			}
		case RBusDevResManUs::EGetInfoOnClientsUsingResource:
			{
			TUint numToCopy;
			RSimplePointerArray<TClientInfoBuf> infoPtrs;
			umemget(&infoPtrs, a1, sizeof(RSimplePointerArray<TClientName>));
			numToCopy = infoPtrs.Count();
			TClientInfoBuf** entriesAddr = infoPtrs.Entries();
			TInt* entryPtr = (TInt*)entriesAddr;
			TPowerClientInfoV01* rcDataPtr = (TPowerClientInfoV01*)iClientNamesResCtrl->Ptr();
			TClientInfoBuf* clientAddr;
			TUint userSideClients = 0;
			TClientInfoBuf tempInfo;
			for(TInt index = 0; index < param1; index++)
				{
				if((!parms[1]) && !(rcDataPtr->iClientId & USER_SIDE_CLIENT_BIT_MASK))
					{
					rcDataPtr++;
					continue;
					}
				if(numToCopy == 0)
					{
					userSideClients++;
					continue;
					}
				umemget32(&clientAddr, entryPtr, sizeof(TClientName*));
				entryPtr++;
				tempInfo().iId = rcDataPtr->iClientId;
				tempInfo().iName = *rcDataPtr->iClientName;
				Kern::InfoCopy(*clientAddr, tempInfo);
				rcDataPtr++;
				numToCopy--;
				userSideClients++;
				}
			if(parms[1])
				umemput32(ptr1, (TAny*)&param1, sizeof(TUint));
			else
				umemput32(ptr1, (TAny*)&userSideClients, sizeof(TUint));
			break;
			}
		case RBusDevResManUs::EGetNamesAllClients:
			{
			TUint numToCopy;
			RSimplePointerArray<TClientName> infoPtrs;
			umemget(&infoPtrs, a1, sizeof(RSimplePointerArray<TClientName>));
			numToCopy = infoPtrs.Count();
			TClientName** entriesAddr = infoPtrs.Entries();
			TInt* entryPtr = (TInt*)entriesAddr;
			TPowerClientInfoV01* rcDataPtr = (TPowerClientInfoV01*)iClientNamesResCtrl->Ptr();
			TClientName* clientAddr;
			TUint userSideClients = 0;
			for(TInt index = 0; index < param1; index++)
				{
				if((!parms[1]) && !(rcDataPtr->iClientId & USER_SIDE_CLIENT_BIT_MASK))
					{
					rcDataPtr++;
					continue;
					}
				if(numToCopy == 0)
					{
					userSideClients++;
					continue;
					}
				umemget32(&clientAddr, entryPtr, sizeof(TClientName*));
				entryPtr++;
				Kern::KUDesPut(*((TDes8*)clientAddr), *(const TDesC8*)rcDataPtr->iClientName);
				rcDataPtr++;
				numToCopy--;
				userSideClients++;
				}
			if(parms[1])
				umemput32(ptr1, (TAny*)&param1, sizeof(TUint));
			else
				umemput32(ptr1, (TAny*)&userSideClients, sizeof(TUint));
			break;
			}
		case RBusDevResManUs::EGetInfoOnResourcesInUseByClient:
			{
			TUint numToCopy;
			RSimplePointerArray<TResourceInfoBuf> infoPtrs;
			umemget(&infoPtrs, (TAny*)parms[1], sizeof(RSimplePointerArray<TResourceInfoBuf>));
			numToCopy = (infoPtrs.Count() < param1) ? infoPtrs.Count() : param1;
			umemput32(ptr1, (TAny*)&param1, sizeof(TUint));
			TResourceInfoBuf** entriesAddr = infoPtrs.Entries();
			TInt* entryPtr = (TInt*)entriesAddr;
			TPowerResourceInfoV01* currRes = (TPowerResourceInfoV01*)iResourceInfoResCtrl->Ptr();
			TResourceInfoBuf* clientAddr;
			TResourceInfoBuf tempInfo;
			for(TUint index = 0; index < numToCopy; index++)
				{
				umemget32(&clientAddr, entryPtr, sizeof(TResourceInfoBuf*));
				entryPtr++;
				r = ExtractResourceInfo(currRes, tempInfo);
				if(r != KErrNone)
					return r;
				umemput((TAny*)clientAddr, (TAny*)&(tempInfo), tempInfo.Length());
				currRes++;
				}
			break;
			}
		}
	return r;
	}

void DChannelResManUs::HandleMsg(TMessageBase* aMsg)
    {
    TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt id=m.iValue;
    
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(" >ldd: DChannelResManUs::HandleMsg(TMessageBase* aMsg) id=%d\n", id));
	
	if (id==(TInt)ECloseMsg)
		{
		// Deregister here to ensure the correct thread ID is read
		if(ClientHandle() != 0)
			{
			// Must de-register from Resource Controller before closing down
			// Not checking return value - still need to delete allocated buffers
#ifdef PRM_US_INSTRUMENTATION_MACRO
	PRM_US_DEREGISTER_CLIENT_START_TRACE;
#endif
			((DPowerResourceController*)iPddPtr)->DeregisterProxyClient(ClientHandle());

#ifdef PRM_US_INSTRUMENTATION_MACRO
	PRM_US_DEREGISTER_CLIENT_END_TRACE;
#endif
			SetClientHandle(0);
			}
	    iMsgQ.iMessage->Complete(KErrNone,EFalse);
		return;
		}
    else if (id==KMaxTInt)
		{
		// DoCancel
		DoCancel(m.Int0());
		m.Complete(KErrNone,ETrue);
		return;
		}

    if (id<0)
		{
		// DoRequest
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		TInt r=DoRequest(~id, pS, m.Ptr1(), m.Ptr2());
		m.Complete(r,ETrue);
		}
    else
		{
		// DoControl
		__KTRACE_OPT(KRESMANAGER, Kern::Printf(" >ldd: do control id=%d...\n", id));
		TInt r=DoControl(id,m.Ptr0(),m.Ptr1());
		m.Complete(r,ETrue);
		}
	}

TInt DChannelResManUs::CancelTrackerRequests(TTrackingControl* aTracker, TBool aSingleRsrc, TUint aResourceId, TRequestStatus* aStatus)
	{
	// Cancel all outstanding requests from this client for a specified operation on 
	// a specified resource

	// Loop all entries in the iBusyQue of requests to locate a match for the 
	// operation type and resource ID
	//
	// For each match, remove the buffer from the busy queue and return to the free queue
	// If the request is already being processed, and so the callback function will be called
	// later, then the callback will exit gracefully.
	//
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(" > DChannelResManUs::CancelTrackerRequests"));
	TInt returnVal = KErrNone;
	TBool statusMatched=EFalse;
	TTrackingBuffer* firstLink = NULL;
	TTrackingBuffer* lastLink = NULL;
	TInt type = aTracker->iType;

#ifdef PRM_US_INSTRUMENTATION_MACRO
	if(type==EGetState)
		{
		PRM_US_CANCEL_GET_RESOURCE_STATE_START_TRACE;
		}
	else if(type==ESetState)
		{
		PRM_US_CANCEL_SET_RESOURCE_STATE_START_TRACE;
		}
#endif

	if(aTracker->iBusyQue != NULL)
		{
		firstLink = (TTrackingBuffer*)(aTracker->iBusyQue->iA.iNext);
		lastLink = (TTrackingBuffer*)(&(aTracker->iBusyQue->iA));
		}
	while(( firstLink!=lastLink )&&(!statusMatched))
		{
		TTrackingBuffer* buffer = firstLink;
		TUint resourceId = buffer->GetResourceId();
		if(aSingleRsrc)
			if(resourceId != aResourceId)	// Required resource?
				{
				firstLink=(TTrackingBuffer*)(firstLink->iNext);
				continue;
				}
		if(aStatus!=NULL)
			{
			TClientRequest *request;
			GET_USER_REQUEST(request, buffer, type)
			if(request->StatusPtr() == aStatus)
				{
				statusMatched = ETrue;
				}
			else
				{
				firstLink=(TTrackingBuffer*)(firstLink->iNext);
				continue;
				}
			}
		TInt r = KErrNone;
		if(type==EGetState)
			{
			TTrackGetStateBuf* stateBuf = (TTrackGetStateBuf*)firstLink;
			r=((DPowerResourceController*)iPddPtr)->CancelAsyncRequestCallBack(ClientHandle(),
															resourceId, (stateBuf->iCtrlBlock));
			}
		else if(type==ESetState)
			{
			TTrackSetStateBuf* stateBuf = (TTrackSetStateBuf*)firstLink;
			r = ((DPowerResourceController*)iPddPtr)->CancelAsyncRequestCallBack(ClientHandle(), 
															resourceId, (stateBuf->iCtrlBlock));
			}
		else if(type==ENotify)
			{
			TTrackNotifyBuf* notifyBuf = (TTrackNotifyBuf*)firstLink;
			r=((DPowerResourceController*)iPddPtr)->CancelNotification(ClientHandle(), resourceId,
															notifyBuf->iNotifyBlock);
			}

		// Process the accumulated return value
		if((r==KErrCompletion)&&((returnVal==KErrNone)||(returnVal==KErrCancel)))
			{
			returnVal=KErrCompletion;	
			}
		else if((r==KErrInUse)&&
			((returnVal==KErrNone)||(returnVal==KErrCompletion)||(returnVal==KErrCancel)))
			{
			returnVal=KErrInUse;
			}
		else if(r!=KErrCancel)
			{
			returnVal=r;
			}

		// Return the tracking buffer to the free queue
		TTrackingBuffer* tempLink = (TTrackingBuffer*)(firstLink->iNext);
		FreeTrackingBuffer(firstLink);
		firstLink = tempLink;

#ifdef PRM_US_INSTRUMENTATION_MACRO
	if(type==EGetState)
		{
		PRM_US_CANCEL_GET_RESOURCE_STATE_END_TRACE;
		}
	else if(type==ESetState)
		{
		PRM_US_CANCEL_SET_RESOURCE_STATE_END_TRACE;
		}
#endif
		// Complete the TRequestStatus object
		if((r!=KErrCompletion)&&(r!=KErrInUse))
			{
			TClientRequest* request;
			GET_USER_REQUEST(request, buffer, type)
			Kern::QueueRequestComplete(iClient, request, r);
			}

		} //  while
	return returnVal;
	}


TTrackingControl* DChannelResManUs::MapRequestToTracker(TInt aRequestType)
// Utility function to map identifiers for cancel commands to request types.
	{
	TTrackingControl *tracker=NULL;
	switch(aRequestType)
		{
		case RBusDevResManUs::ECancelChangeResourceStateRequests:
		case RBusDevResManUs::ECancelChangeResourceState:
			{
			tracker=iSetStateTracker;
			break;
			}
		case RBusDevResManUs::ECancelGetResourceStateRequests:
		case RBusDevResManUs::ECancelGetResourceState:
			{
			tracker=iGetStateTracker;
			break;
			}
		case RBusDevResManUs::ECancelChangeNotificationRequests:
		case RBusDevResManUs::ECancelRequestChangeNotification:
			{
			tracker=iListenableTracker;
			break;
			}
		default:
			{
			__ASSERT_ALWAYS(0,RESMANUS_FAULT());
			}
		}
	return tracker;
	}


TInt DChannelResManUs::CancelRequestsOfType(TInt aRequestType, TRequestStatus* aStatus)
// Cancel a particular request. This may be qualified by the type of operation
    {
	__ASSERT_ALWAYS(((aRequestType==RBusDevResManUs::ECancelChangeResourceState)||
					(aRequestType==RBusDevResManUs::ECancelGetResourceState)||
					(aRequestType==RBusDevResManUs::ECancelRequestChangeNotification)||
					(KMaxTInt)),
					RESMANUS_FAULT());
	// For the KMaxTInt case, the type of the request is not known and so all trackers
	// must be considered before the request is found.
	// For all other cases, only the relevant tracker is searched.
	TInt r=KErrNone;
	if(aRequestType!=KMaxTInt)
		{
		TTrackingControl*tracker=MapRequestToTracker(aRequestType);
		r=CancelTrackerRequests(tracker, EFalse, 0, aStatus);
		}
	else
		{
		TTrackingControl* tracker[3] = {iGetStateTracker, iSetStateTracker, iListenableTracker};
		TUint8 index=0;
		while((index<3) && (r==KErrNone))
			{
			r=CancelTrackerRequests(tracker[index], EFalse, 0, aStatus);
			++index;
			}
		}
	if(r==KErrCancel) 
		r=KErrNone;	// All cancellations were successful

	return r;
	}


void DChannelResManUs::DoCancel(TInt aMask)
// Cancel an outstanding request.
    {
	TRequestStatus* status = (TRequestStatus*)aMask;
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoCancel, TRequestStatus addr = 0x%x",(TInt)status));

	CancelRequestsOfType(KMaxTInt, status); // Ignore return value
	return;
	}

TInt DChannelResManUs::DoRequest(TInt aReqNo, TRequestStatus* /*aStatus*/, TAny* a1, TAny* a2)
// Asynchronous requests.
    {
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2)"));

    TInt r=KErrNone;
    switch (aReqNo)
		{
		case RBusDevResManUs::EChangeResourceState:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoRequest case EChangeResourceState"));
			// a1 specifies the identifier of the required resource
			// a2 specifies the required state for the resource
			//
			TUint *param = (TUint*)a2;
			TUint resourceId = (TUint)a1;
			TInt newState = (TInt)param[0];

#ifdef PRM_US_INSTRUMENTATION_MACRO
	PRM_US_SET_RESOURCE_STATE_START_TRACE;
#endif
				// Invoke the API
				r=((DPowerResourceController*)iPddPtr)->ChangeResourceState(ClientHandle(),
														resourceId, newState, (TPowerResourceCb*)param[1]);
			break;
			}

		case RBusDevResManUs::EGetResourceState:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoRequest case EGetResourceState"));
			// a1 specifies the resource ID
			// a2 specifies the container stating if a cached value is required, the address of the variable
			// to be update with the state value and the address of the level owner ID
			//
			TUint resourceId = (TUint)a1;
			TUint *parms = (TUint*)a2;
			TBool cached = (TBool)(parms[0]);

#ifdef PRM_US_INSTRUMENTATION_MACRO
	PRM_US_GET_RESOURCE_STATE_START_TRACE;
#endif
				// Always invoke the asynchronous version of the API
				r=((DPowerResourceController*)iPddPtr)->GetResourceState(ClientHandle(),
																		resourceId, cached, *((TPowerResourceCb*)parms[3]));
			break;
			}


		case RBusDevResManUs::ERequestChangeNotification:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoRequest case ERequestChangeNotification"));
			// a1 specifies the resource ID
			r=((DPowerResourceController*)iPddPtr)->RequestNotification(ClientHandle(),
														(TUint)a1, *((DPowerResourceNotification*)a2));
			break;
			}

		case RBusDevResManUs::ERequestQualifiedChangeNotification:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoRequest case ERequestQualifiedChangeNotification"));
			// a1 specifies the threshold value that the state is to change by
			// a2 specifies the address of the container holding the resourceID and the required direction
			TInt threshold = (TInt)a1;
			TUint *parms = (TUint*)a2;
			TUint resourceId = parms[0];
			TBool direction = (TBool)(parms[1]);			
			r=((DPowerResourceController*)iPddPtr)->RequestNotification(ClientHandle(),
														resourceId, *((DPowerResourceNotification*)parms[2]), threshold, direction);
			break;
			}

		default:
	    	return KErrNotSupported;
		}
	    return r;
    }

TInt DChannelResManUs::DoControl(TInt aFunction, TAny* a1, TAny* a2)
// Synchronous requests.
    {
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("> DChannelResManUs::DoControl(TInt aFunction, TAny* a1, TAny* a2)") );

    TInt r=KErrNone;
    switch (aFunction)
		{
		case RBusDevResManUs::EInitialise:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoControl case EInitialise"));
			// a1 specifies the array describing the number of 'gettable' and 'settable' state resources
			// and the number of 'listenable' resources
			//
			TUint8 *stateRes = (TUint8*)a1;
#ifdef PRM_US_INSTRUMENTATION_MACRO
	PRM_US_REGISTER_CLIENT_START_TRACE;
#endif
			// The call to the Resource Controller's AllocReserve method requires two parameters:
			// the number of client level objects and the number of request message objects
			// Each 'settable' state resource requires a client level object and a request message object
			// Each 'gettable' state resource requires a request message object, only.
			// Call Resource Control to make allocations
			r=((DPowerResourceController*)iPddPtr)->AllocReserve(ClientHandle(),
															stateRes[1],							// Number of settable
															(TUint8)(stateRes[1] + stateRes[0]));	// Number of (settable + gettable)
#ifdef PRM_US_INSTRUMENTATION_MACRO
	PRM_US_REGISTER_CLIENT_END_TRACE;
#endif
			if(r==KErrNone)
				{
				// Require 1 TPowerResourceCb object per gettable resource state
				// Require 1 TPowerResourceCb object per settable resource state
				// Require 1 DPowerResourceNotification object per listenable resource
				//
				if(stateRes[0]>0)
					r=InitTrackingControl(iGetStateTracker,EGetState,stateRes[0]);
				if((r==KErrNone) && (stateRes[1]>0))
					r=InitTrackingControl(iSetStateTracker,ESetState,stateRes[1]);
				if((r==KErrNone) && (stateRes[2]>0))
					r=InitTrackingControl(iListenableTracker,ENotify,stateRes[2]);
#ifdef _DUMP_TRACKERS
				DumpTracker(iGetStateTracker);
				DumpTracker(iSetStateTracker);
#endif
				}
			break;
			}

		case RBusDevResManUs::EGetNoOfResources:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoControl case EGetNoOfResources"));
			TUint numResources;
			r=((DPowerResourceController*)iPddPtr)->GetNumResourcesInUseByClient(ClientHandle(),0,numResources);
			iResInfoValid = 0;			// New numResources invalidates the iResInfoXXXX information
			iResInfoStoredClientId = 0;
			iResInfoStoredNum = 0;
			if(r!=KErrNone)
				return r;
			// a2 specifies whether the resource information should be loaded
			if((r==KErrNone)&&(a2!=NULL))
				{
				TUint prevNumRes = 0;
				while((numResources != prevNumRes)&&(r==KErrNone))
					{
					// if the number of resources is greater than can be accommodated by the array,
					// re-size it
					if((r=EnsureSizeIsSufficient(iResourceInfoResCtrl, (TInt)(numResources*sizeof(TPowerResourceInfoV01))))!=KErrNone)
						break;
					prevNumRes = numResources;
					// Get the resource info from the Resource Controller
					// Specify 'aTargetClientId' as zero to access all resources
					iResourceInfoResCtrl->SetLength(0);
					r=((DPowerResourceController*)iPddPtr)->GetInfoOnResourcesInUseByClient(
															ClientHandle(),0,numResources,iResourceInfoResCtrl);
					}
				if(r==KErrNone)
					{
					iResInfoValid = 1;
					iResInfoStoredClientId = KAllResInfoStored;
					iResInfoStoredNum = numResources;
					}
				}
			if(r==KErrNone)
				*(TUint*)a1 = numResources;
			break;
			}

		case RBusDevResManUs::EGetAllResourcesInfo:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoControl case EGetAllResourcesInfo"));
			// Parameters are passed in TUint* parms[2]
			// The address of the number of resources is at element 0
			// The flag to indicate if the resource info stored is to be refreshed is at element 1
			TUint* parms = (TUint*)a2;
			TUint numResources = *(TUint*)parms[0];
			TBool refresh=(TBool)(parms[1]);
			
			// The results are to be written to an RSimplePointerArray, the address is in a1
			// Check that the array has enough elements
			if(refresh)
				{
				// For the refresh option, invoke Resource Controller API once, only (do not recurse)
				// If the number of requested resources is greater than can be accommodated by the array,
				// re-size it
				if((r=EnsureSizeIsSufficient(iResourceInfoResCtrl, (TInt)(numResources*sizeof(TPowerResourceInfoV01))))!=KErrNone)
					break;
				// Get the resource info from the Resource Controller
				// Specify 'aTargetClientId' as zero to access all resources
				iResourceInfoResCtrl->SetLength(0);
				r=((DPowerResourceController*)iPddPtr)->GetInfoOnResourcesInUseByClient(
														ClientHandle(),0,numResources,iResourceInfoResCtrl);
				if(numResources != iResInfoStoredNum)
					{
					iResInfoValid = 0;		// Assume cohesion is now lost 
					iResInfoStoredClientId = 0;
					iResInfoStoredNum = 0;
					}
				}
			else
				{
				// If the information stored is not valid or is not for all resources return KErrNotReady 
				if((iResInfoValid != 1)||(iResInfoStoredClientId != KAllResInfoStored))
					{
					r=KErrNotReady;
					break;
					}
				// The number of resources for which information is available in this case is iResInfoStoredNum
				numResources = iResInfoStoredNum;
				}
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
			TPowerResourceInfoV01* currRes = (TPowerResourceInfoV01*)iResourceInfoResCtrl->Ptr();
			for(TUint index = 0; index < numResources; index++)
				{
				CheckForCandidateAsyncResource(currRes);
				CheckForCandidateSharedResource(currRes);
				currRes++;
				}
#endif
			*(TUint*)(parms[0]) = numResources;

			break;
			}

		case RBusDevResManUs::EGetNoOfClients:
		case RBusDevResManUs::EGetNumClientsUsingResource:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoControl case EGetNoOfClients"));
			// Parameters are passed in TUint parms[3]
			// The flag to indicate if kernel-side clients are to be included is at element 0
			// The ID of the resource of interest (0 is expected for EGetNoOfClients)
			// The flag to indicate if the client info is to be read now is at element 1
			TUint *parms = (TUint*)a2;
			TUint includeKern = parms[0];
			TUint resourceId = parms[1];
			TUint infoRead = parms[2];
			TUint requiredId = resourceId;
			if(aFunction == RBusDevResManUs::EGetNoOfClients)
				{
				__ASSERT_ALWAYS(resourceId==0,RESMANUS_FAULT());
				requiredId = KAllClientInfoStored;
				}
			TUint numClients = 0;
			if(includeKern==1)
				{
				// Client must exhibit PlatSec capability ReadDeviceData
				if(!iClient->HasCapability(ECapabilityReadDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by Resource Manager user-side API function EGetNoOfClients")))
					{
					r =  KErrPermissionDenied;
					break;
					}
				if(r==KErrNone)
					r=((DPowerResourceController*)iPddPtr)->GetNumClientsUsingResource(ClientHandle(),resourceId,numClients);
				}
			else
				numClients = (TUint)(iDevice->iOpenChannels);

			// New numClients invalidates the iClientInfoXXXX information
			iClientInfoValid = 0;
			iClientInfoStoredResId = 0;
			iClientInfoStoredNum= 0;

			if((r==KErrNone)&&(infoRead==1))
				{
				// Capability check already performed, so no need to repeat ...
				TUint prevNumClients = 0;
				while((numClients != prevNumClients)&&(r == KErrNone))
					{
					// Ensure buffer is large enough to store the information
					if((r=EnsureSizeIsSufficient(iClientNamesResCtrl, (TInt)(numClients*sizeof(TPowerClientInfoV01))))!=KErrNone)
						break;
					prevNumClients = numClients;
					// Invoke the API
					r=((DPowerResourceController*)iPddPtr)->GetInfoOnClientsUsingResource(ClientHandle(),
																					resourceId,numClients,iClientNamesResCtrl);
					};

				if(r==KErrNone)
					{
					iClientInfoValid = 1;
					iClientInfoStoredResId = requiredId;
					iClientInfoStoredNum = numClients;
					if(includeKern!=1)
						{
						TUint numAllClients = numClients;
						numClients = 0;
						TPowerClientInfoV01* rcDataPtr = (TPowerClientInfoV01*)(iClientNamesResCtrl->Ptr());
						for(TUint i=0; i<numAllClients; i++)
							{
							if( rcDataPtr->iClientId & USER_SIDE_CLIENT_BIT_MASK)
								++numClients;
							++rcDataPtr;
							}
						}
					}
				}
			if(r==KErrNone)
				*(TUint*)a1 = numClients;
			break;
			}

		case RBusDevResManUs::EGetNamesAllClients:
		case RBusDevResManUs::EGetInfoOnClientsUsingResource:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoControl case EGetNamesAllClients-EGetInfoOnClientsUsingResource"));
			// Parameters are passed in TUint* parms[4]
			// The address of the number of clients is at element 0
			// The flag to indicate if kernel-side info is requested is at element 1
			// The resource ID is at element 2
			// The flag to indicate if the client information stored is to be refreshed is at element 3
			TUint* parms = (TUint*)a2;
			TUint numClients = *(TUint*)parms[0];
			TBool includeKern=(TBool)(parms[1]);
			TUint resourceId=(TUint)(parms[2]);
			TBool refresh=(TBool)(parms[3]);
		
			TUint numClientsAvailable = 0; 
			iClientNamesResCtrl->SetLength(0);
			
			if(includeKern)
				{
				// Client must exhibit PlatSec capability ReadDeviceData
				if(!iClient->HasCapability(ECapabilityReadDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by Resource Manager user-side API function EGetNamesAllClients-EGetInfoOnClientsUsingResource")))
					{
					r = KErrPermissionDenied;
					break;  // Early exit in event of error
					}
				TUint requiredId = (resourceId==0)?(TUint)KAllClientInfoStored:resourceId;
				if(refresh)
					{
					// For the refresh option, invoke Resource Controller API once, only (do not recurse)
					// If the number of clients is greater than can be accommodated by the array,
					// re-size it
					if((r=EnsureSizeIsSufficient(iClientNamesResCtrl, (TInt)(numClients*sizeof(TPowerClientInfoV01))))!=KErrNone)
						break;
					// Invoke the API
					numClientsAvailable = numClients; // Arbitrary initialisation (to silence compiler warning)
					r=((DPowerResourceController*)iPddPtr)->GetInfoOnClientsUsingResource(ClientHandle(),
																					resourceId,numClientsAvailable,iClientNamesResCtrl);
					if((r!=KErrNone)||(numClientsAvailable != iClientInfoStoredNum)||(iClientInfoStoredResId != requiredId))
						{
						iClientInfoValid = 0;	// Assume cohesion is now lost	
						iClientInfoStoredResId = 0;
						iClientInfoStoredNum = 0;
						}
					}
				else
					{
					// If the information stored is not valid, is not for the required resources return KErrNotReady 
					if((iClientInfoValid != 1)||(iClientInfoStoredResId != requiredId))
						r=KErrNotReady;
					// The number of clients for which information is available in this case is iClientInfoStoredNum
					numClientsAvailable = iClientInfoStoredNum;
					}
				}
			else
				{
				// Resource Controller will return information for the number of clients requested,
				// taken in order from its internal storage - but this will be regardless of whether
				// they are kernel-side or user-side; the USER_SIDE_CLIENT_BIT_MASK bit must be 
				// interrogated to determine this.
				//
				// Therefore, need to read all the clients - but to do this, must find out how many 
				// clients there are first.
				TUint numAllClients;
				r=((DPowerResourceController*)iPddPtr)->GetNumClientsUsingResource(ClientHandle(),resourceId,numAllClients);
				if(r!=KErrNone)
					break;  // Early exit in event of error
				if(numAllClients > 0)
					{
					if(refresh)
						{
						// For the refresh option, invoke Resource Controller API once, only (do not recurse)
						// If the number of clients is greater than can be accommodated by the array,
						// re-size it
						if((r=EnsureSizeIsSufficient(iClientNamesResCtrl, (TInt)(numAllClients*sizeof(TPowerClientInfoV01))))!=KErrNone)
							break;
						// Invoke the API
						r=((DPowerResourceController*)iPddPtr)->GetInfoOnClientsUsingResource(ClientHandle(),
																						resourceId,numAllClients,iClientNamesResCtrl);
						TUint requiredId = (resourceId==0)?(TUint)KAllClientInfoStored:resourceId;
						if((r!=KErrNone)||(numClientsAvailable != iClientInfoStoredNum)||(iClientInfoStoredResId != requiredId))
							{
							iClientInfoValid = 0;	// Assume cohesion is now lost	
							iClientInfoStoredResId = 0;
							iClientInfoStoredNum = 0;
							break;
							}
						else
							{
							iClientInfoValid = 1;
							iClientInfoStoredResId = requiredId;
							iClientInfoStoredNum = numAllClients;
							}
						}
					else
						{
						// If the information stored is not valid, is not for the required resources return KErrNotReady 
						TUint requiredId = (resourceId==0)?(TUint)KAllClientInfoStored:resourceId;
						if((iClientInfoValid != 1)||(iClientInfoStoredResId != requiredId))
							{
							r=KErrNotReady;
							break;
							}
						// The number of clients for which information is available in this case is iClientInfoStoredNum
						numAllClients = iClientInfoStoredNum;
						}
					numClientsAvailable = numAllClients;
					} // if(numAllClients > 0)
				}
			// Write the total number of user side cients available
			*(TUint*)parms[0] = numClientsAvailable;
			break;
			}
		case RBusDevResManUs::EGetNumResourcesInUseByClient:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoControl case EGetNumResourcesInUseByClient"));
			// a1 specifies the container holding the client name
			//
			
			// If client doesn't exist, return KErrNotFound
			// If client has appropriate capabilities, or if the client for which the information is sought
			// is user-side, invoke the Resource Controller API directly
			// Otherwise, return KErrPermissionDenied
			TUint clientId=0;
			r=((DPowerResourceController*)iPddPtr)->GetClientId(ClientHandle(),
															*(TDesC8*)a1,clientId);
			if(r!=KErrNone)
				return KErrNotFound;
			// Perform capability check
			if(!iClient->HasCapability(ECapabilityReadDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by Resource Manager user-side API function EGetNoOfClients")))
				{
				if(!(clientId & USER_SIDE_CLIENT_BIT_MASK))
					return KErrPermissionDenied;
				}
			TUint numResources=0;
			if(r==KErrNone)
				r=((DPowerResourceController*)iPddPtr)->GetNumResourcesInUseByClient(ClientHandle(),
																			clientId,numResources);
			// New numResources invalidates the iResXXXX information
			iResInfoValid = 0;
			iResInfoStoredClientId = 0;
			iResInfoStoredNum= 0;

			// parms[1] specifies whether the resource information should be loaded
			if((r==KErrNone)&&(*(TUint*)a2 != NULL))
				{
				TUint prevNumRes = 0;
				while((numResources != prevNumRes)&&(r==KErrNone))
					{
					// if the number of resources is greater than can be accommodated by the array,
					// re-size it
					if((r=EnsureSizeIsSufficient(iResourceInfoResCtrl, (TInt)(numResources*sizeof(TPowerResourceInfoV01))))!=KErrNone)
						break;
					prevNumRes = numResources;
					// Get the resource info from the Resource Controller
					// Specify 'aTargetClientId' as zero to access all resources
					iResourceInfoResCtrl->SetLength(0);
					r=((DPowerResourceController*)iPddPtr)->GetInfoOnResourcesInUseByClient(
															ClientHandle(),clientId,numResources,iResourceInfoResCtrl);
					}
				if(r==KErrNone)
					{
					iResInfoValid = 1;
					iResInfoStoredClientId = clientId;
					iResInfoStoredNum = numResources;
					}
				}
			if(r==KErrNone)
				*(TUint*)a2 = numResources;
			break;
			}

		case RBusDevResManUs::EGetInfoOnResourcesInUseByClient:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoControl case EGetInfoOnResourcesInUseByClient"));
			// a1 specifies the container holding the client name
			// a2 specifies an array TUint* parms[3] which contains:
			//   - the address of the variable to write the number of reasources to
			//   - a pointer to the container to hold the resources' information
			//   - the flag to indicate whether the resource info should be (re-)read here

			TUint clientId=0;
			TUint *parms = (TUint*)a2;
			TUint numResources = *(TUint*)parms[0];
			// The results are to be written to an RSimplePointerArray, the address is in parms[1]
			// Check that the array has enough elements
			// If client doesn't exist, return KErrNotFound
			// If client has appropriate capabilities, or if the client for which the information is sought
			// is user-side, invoke the Resource Controller API directly
			// Otherwise, return KErrPermissionDenied
			r=((DPowerResourceController*)iPddPtr)->GetClientId(ClientHandle(),
															*(TDesC8*)a1,clientId);
			if(r!=KErrNone)
				return KErrNotFound;
			// Perform capability check
			if(!iClient->HasCapability(ECapabilityReadDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by Resource Manager user-side API function EGetNoOfClients")))
				{
				if(!(clientId & USER_SIDE_CLIENT_BIT_MASK))
					return KErrPermissionDenied;
				}

			TUint updatedNumResources = numResources;
			r=((DPowerResourceController*)iPddPtr)->GetNumResourcesInUseByClient(ClientHandle(),clientId,updatedNumResources);
			if(r!=KErrNone)
				break;

			if(updatedNumResources>0)
				{
				if((TUint)(parms[2] != 0))
					{
					// For the refresh option, invoke Resource Controller API once, only (do not recurse)
					// If the number of requested resources is greater than can be accommodated by the array,
					// re-size it
					if((r=EnsureSizeIsSufficient(iResourceInfoResCtrl, (TInt)(numResources*sizeof(TPowerResourceInfoV01))))!=KErrNone)
						break;
					// Get the resource info from the Resource Controller
					// Specify 'aTargetClientId' as zero to access all resources
					iResourceInfoResCtrl->SetLength(0);
					r=((DPowerResourceController*)iPddPtr)->GetInfoOnResourcesInUseByClient(
															ClientHandle(),clientId,numResources,iResourceInfoResCtrl);
					if((numResources != iResInfoStoredNum)||(iResInfoStoredClientId != clientId))
						{
						iResInfoValid = 0;		// Assume cohesion is now lost 
						iResInfoStoredClientId = 0;
						iResInfoStoredNum = 0;
						}
					}
				else
					{
					// If the information stored is not valid or is not for the required clientId return KErrNotReady 
					if((iResInfoValid != 1)||(iResInfoStoredClientId != clientId))
						r=KErrNotReady;
					// The number of resources for which information is available in this case is iResInfoStoredNum
					numResources = iResInfoStoredNum;
					}
				}
			if(r==KErrNone)
				*(TUint*)parms[0] = updatedNumResources;

			break;
			}

		case RBusDevResManUs::EGetResourceIdByName:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoControl case EGetResourceIdByName"));
			// a1 specifies the container holding the resource name
			// a2 specifies the variable to be update with the ID
			TUint resourceId;
			r=((DPowerResourceController*)iPddPtr)->GetResourceId(ClientHandle(), *(TDesC8*)a1, resourceId);
			if(r==KErrNone)
				*(TUint *)a2 = resourceId;
			break;
			}

		case RBusDevResManUs::EGetResourceInfo:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoControl case EGetResourceInfo"));
			// a1 specifies the container holding the resource ID
			// a2 specifies the address of the container to be written to

			TUint resourceId= (TUint)a1;
			TPowerResourceInfoBuf01 resCtrlInfo;
			resCtrlInfo.SetLength(0);
			TResourceInfoBuf tempInfo;
			r=((DPowerResourceController*)iPddPtr)->GetResourceInfo(ClientHandle(),resourceId,&resCtrlInfo);
			if(r==KErrNone)
				{
				// Copy the client buffer to tempInfo so that its size can be determined
				// by ExtractResourceInfo
				r=ExtractResourceInfo(&(resCtrlInfo()), tempInfo);
				}			
			if(r==KErrNone)
				{
				// Write the resources' info to the client thread
				*(TResourceInfoBuf*)a2 = tempInfo;
				}
			break;
			}


		case RBusDevResManUs::ECancelChangeResourceStateRequests:
		case RBusDevResManUs::ECancelGetResourceStateRequests:
		case RBusDevResManUs::ECancelChangeNotificationRequests:
			{
			TUint resourceId = (TUint)a1;
			TTrackingControl*tracker=MapRequestToTracker(aFunction);
			r=CancelTrackerRequests(tracker, ETrue, resourceId, NULL);
			if(r==KErrCancel)
				r=KErrNone;	// All cancellations were successful
			break;
			}

		case RBusDevResManUs::ECancelChangeResourceState:
		case RBusDevResManUs::ECancelGetResourceState:
		case RBusDevResManUs::ECancelRequestChangeNotification:
			{
			TRequestStatus* status = (TRequestStatus*)a1;
			r=CancelRequestsOfType(aFunction, status);
			break;
			}


#ifdef RESOURCE_MANAGER_SIMULATED_PSL
		case RBusDevResManUs::EGetNumCandidateAsyncResources:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoControl case EGetNumCandidateAsyncResources"));
			TUint numResources;
			GetNumCandidateAsyncResources(numResources);
			// Write the result to the client thread
			*(TUint*)a1 = numResources;
			break;
			}
		case RBusDevResManUs::EGetCandidateAsyncResourceId:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoControl case EGetCandidateAsyncResourceId"));
			// Get the index to use
			TUint index = (TUint)a1;
			TUint resourceId = 0;
			r=GetCandidateAsyncResourceId(index, resourceId);
			if(r==KErrNone)				// Write the result to the client thread
				*(TUint*)a2 = resourceId;
			break;
			}

		case RBusDevResManUs::EGetNumCandidateSharedResources:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoControl case EGetNumCandidateSharedResources"));
			TUint numResources;
			GetNumCandidateSharedResources(numResources);
			// Write the result to the client thread
			*(TUint*)a1 = numResources;
			break;
			}
		case RBusDevResManUs::EGetCandidateSharedResourceId:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoControl case EGetCandidateSharedResourceId"));
			// Get the index to use
			TUint index = (TUint)a1;
			TUint resourceId = 0;
			r=GetCandidateSharedResourceId(index, resourceId);
			if(r==KErrNone)				// Write the result to the client thread
				*(TUint*)a2 = resourceId;
			break;
			}
#endif

		case RBusDevResManUs::EGetResourceControllerVersion:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoControl case EGetResourceControllerVersion"));
			// a1 specifies the address of the TVersion variable to be written to
			// a2 is not used
			TUint version;
			if((r=((DPowerResourceController*)iPddPtr)->GetInterface(ClientHandle(), 
																	KResManControlIoGetVersion, 
																	(TAny*)&version,
																	NULL, 
																	NULL))!=KErrNone)
				return r;
			// Write the required information
			*(TUint*)a1 = version;
			break;
			}
		
		case RBusDevResManUs::EGetNumDependentsForResource:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoControl case EGetNumDependentsForResource"));
			// a1 specifies a pointer to the variable to be written to
			// a2 specifies an array TUint parms[2] which contains:
			//   - the resource ID
			//   - flag to indicate if dependency information is to be loaded as part of this call
			TUint *parms = (TUint*)a2;
			TUint numDependents = 0;
			r=((DPowerResourceController*)iPddPtr)->GetInterface(ClientHandle(), 
																	KResManControlIoGetNumDependents,
																	(TAny*)(parms[0]),	// Resource ID
																	&numDependents,
																	NULL);
			iResDepsValid=EFalse; // The number of dependents may differ from the dependency information stored
			if(r!=KErrNone)
				return r;

			// Load the dependency information, if required.
			if(parms[1])
				{
				// The dependency information may be updated subsequent to the request for the number of dependents. In order
				// to provide a coherent number and array of dependents, the requests for dependency information will be
				// re-issued if the (new) number of dependents differs from that previously read.
				TUint prevNumDeps = 0;
				TUint newNumDeps = numDependents;
				while((newNumDeps != prevNumDeps)&&(r == KErrNone))
					{
					if((r=EnsureSizeIsSufficient(iResourceDependencyIds, (TInt)(newNumDeps*sizeof(SResourceDependencyInfo))))!=KErrNone)
						return r;
					prevNumDeps = newNumDeps;
					if((r=((DPowerResourceController*)iPddPtr)->GetInterface(ClientHandle(), 
																			KResManControlIoGetDependentsId,
																			(TAny*)(parms[0]),	// Resource ID
																			(TAny*)(iResourceDependencyIds),
																			(TAny*)&newNumDeps))!=KErrNone)
						return r;
					};
				// Dependency information now in synch with number reported
				numDependents = newNumDeps;
				iNumResDepsStored = newNumDeps;
				iResDepsValid = ETrue;
				}
			// Write the number of dependents to the client thread
			*(TUint*)a1 = numDependents;
			break;
			}


		case RBusDevResManUs::EGetDependentsIdForResource:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoControl case EGetDependentsIdForResource"));
			// a1 specifies a pointer to the variable to hold the number of dependencies
			// a2 specifies an array TUint parms[4] which contains:
			//   - the resource ID
			//   - the address of the array to write the required IDs to
			//   - flag to indicate if dependency information is to be (re-)loaded as part of this call
			TUint *parms = (TUint*)a2;
			TUint numDependents = 0;

			// (Re-)Load the dependency information, if required.
			if(parms[2])
				{
				if((r=((DPowerResourceController*)iPddPtr)->GetInterface(ClientHandle(), 
															KResManControlIoGetNumDependents, 
															(TAny*)(parms[0]), 
															(TAny*)&numDependents, 
															NULL))!=KErrNone)
					return r;

				iResDepsValid=EFalse; // The number of dependents may differ from the dependency information stored
				// In order to provide a coherent number and array of dependents, the requests for dependency information
				// will be re-issued if the (new) number of dependents differs from that previously read.
				TUint prevNumDeps = 0;
				TUint newNumDeps = numDependents;
				while(newNumDeps != prevNumDeps)
					{
					if((r=EnsureSizeIsSufficient(iResourceDependencyIds, (TInt)(newNumDeps*sizeof(SResourceDependencyInfo))))!=KErrNone)
						return r;
					prevNumDeps = newNumDeps;
					if((r=((DPowerResourceController*)iPddPtr)->GetInterface(ClientHandle(), 
																			KResManControlIoGetDependentsId,
																			(TAny*)(parms[0]),	// Resource ID
																			(TAny*)(iResourceDependencyIds),
																			(TAny*)&newNumDeps))!=KErrNone)
						return r;
					};

				// Dependency information now in synch with number reported
				numDependents = newNumDeps;
				iNumResDepsStored = newNumDeps;
				iResDepsValid = ETrue;
				}

			// If iResDepsValid equals zero, the results are invalid - so return KErrNotReady.
			if(iResDepsValid==0)
				return KErrNotReady;

			// Write the number of dependencies available to the client
			*(TUint*)a1 = iNumResDepsStored;
			// Write the dependencies to the client array if it is of sufficient size
			// Copy the required dependency information to the user-supplied container.
			parms[1] = (TUint)iResourceDependencyIds;
			break;
			}
		
		default:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::DoControl default 0x%x", aFunction));
			r=KErrNotSupported;
			}
		}
	    return(r);
    }


TInt DChannelResManUs::EnsureSizeIsSufficient(HBuf*& aBuffer, TInt aMinSize)
	{
// Utility function to ensure a buffer is of at least the minimum required size
// If the buffer is to small, an attempt is made to increase its size.
// If the re-sizing fails, KErrNoMemory is returned; otherwise KErrNone.
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("> DChannelResManUs::EnsureSizeIsSufficient"));

	if(aBuffer->MaxLength() < aMinSize)
		{
		aBuffer = aBuffer->ReAlloc(aMinSize);
		if(aBuffer->MaxLength() < aMinSize)
			return KErrNoMemory; // ReAlloc failed - aBuffer is unchanged
		}
	aBuffer->SetLength(0);
	return KErrNone;
	}

void DChannelResManUs::FreeTrackingBuffer(TTrackingBuffer*& aBuffer)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DChannelResManUs::FreeTrackingBuffer"));
	// Function invoked for to free tracking buffers from the busy to free queue of a tracking control
	__ASSERT_ALWAYS((aBuffer!=NULL),RESMANUS_FAULT());
	NKern::FMWait(&iBufferFastMutex);
	TTrackingControl* tracker = aBuffer->GetTrackingControl();
	SDblQue* bufQue = aBuffer->GetQue();

	__ASSERT_ALWAYS(((tracker!=NULL)&&(bufQue!=NULL)),RESMANUS_FAULT());

	// Check that the buffer is still in the busy queue of the tracker - exit if not
	if(bufQue == tracker->iBusyQue)
		{
		aBuffer->Deque();
		tracker->iFreeQue->Add(aBuffer);
		aBuffer->SetQue(tracker->iFreeQue);
		}
	NKern::FMSignal(&iBufferFastMutex);	
	}


TInt DChannelResManUs::GetAndInitTrackingBuffer(TTrackingControl*& aTracker, TTrackingBuffer*& aBuffer, TUint aResourceId, TRequestStatus* aStatus)
	{
// Utility function - perform the necessary processing to get a buffer to support
// asynchronous requests to change the state of a resource
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("> DChannelResManUs::GetAndInitTrackingBuffer"));
	TInt r=KErrNone;
	NKern::FMWait(&iBufferFastMutex);
	if(aTracker->iFreeQue->IsEmpty())
		r = KErrUnderflow;
	else
		{
		// Need intermediate cast from SDblQueLink* to TAny* before TTrackingBuffer*
		TAny* ptr = (TAny*)(aTracker->iFreeQue->GetFirst());
		aBuffer = (TTrackingBuffer*)ptr;
		aTracker->iBusyQue->Add((SDblQueLink*)ptr);
		aBuffer->SetQue(aTracker->iBusyQue);
		aBuffer->SetResourceId(aResourceId);
		TClientRequest* request;
		TTrackingControl* tracker = aBuffer->GetTrackingControl();
		GET_USER_REQUEST(request, aBuffer, tracker->iType);
		request->Reset();
		request->SetStatus(aStatus);
		}
	NKern::FMSignal(&iBufferFastMutex);	
	return r;
	}

TInt DChannelResManUs::GetStateBuffer(TTrackingControl*& aTracker, TTrackingBuffer*& aBuffer, TUint aResourceId, TInt* aState, TInt* aLevelOwnerPtr, TPowerResourceCb*& aCb, TRequestStatus* aStatus)
	{
// Utility function - perform the necessary processing to get a buffer and control block
// to support asynchronous requests to change the state of a resource
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("> DChannelResManUs::GetStateBuffer"));

	TInt r=GetAndInitTrackingBuffer(aTracker, aBuffer, aResourceId, aStatus);
	if(r==KErrNone)
		{
		TTrackGetStateBuf* stateBuf = (TTrackGetStateBuf*)aBuffer;
		stateBuf->iRequest->SetDestPtr1(aState);
		stateBuf->iRequest->SetDestPtr2(aLevelOwnerPtr);
		// Use placement new to update the content of the TPowerResourceCb
		aCb = &(stateBuf->iCtrlBlock);
		new (aCb) TPowerResourceCb(&AsyncCallBackFn,(TAny*)aBuffer,iDfcQ,KResManCallBackPriority);
		}
	return r;
	}

#ifdef _DUMP_TRACKERS
void DChannelResManUs::DumpTracker(TTrackingControl* aTracker)
	{
	Kern::Printf("\nDChannelResManUs::DumpTracker");
	Kern::Printf("Tracker at 0x%x\n",aTracker);
	if(!aTracker)
		{
		Kern::Printf("Nothing to dump..");
		return;
		}
	Kern::Printf("iType=%d",aTracker->iType);
	switch(aTracker->iType)
		{
		case 0:
			Kern::Printf("= GetState tracker\n");
		break;
		case 1:
			Kern::Printf("= SetState tracker\n");
		break;
		case 2:
			Kern::Printf("= Notify tracker\n");
		break;
		}
	Kern::Printf("iOwningChannel at 0x%x\n",aTracker->iOwningChannel);
	Kern::Printf("iFreeQue at 0x%x\n",aTracker->iFreeQue);
	SDblQueLink* buf;
	if(aTracker->iFreeQue!=NULL)
		{
		buf=aTracker->iFreeQue->First();
		while(buf!=aTracker->iFreeQue->Last())
			{
			Kern::Printf("iFreeQue first buffer at 0x%x\n",buf);
			TAny* intermediatePtr = (TAny*)buf;
			if((aTracker->iType == EGetState)||(aTracker->iType == ESetState))
				{
				TTrackSetStateBuf* tempBuf =(TTrackSetStateBuf*)intermediatePtr;
				Kern::Printf("buffer control block at 0x%x\n",(TInt)&tempBuf->iCtrlBlock);
				}
			buf = buf->iNext;
			};
		}
	Kern::Printf("iBusyQue at 0x%x\n",aTracker->iBusyQue);
	if(aTracker->iBusyQue!=NULL)
		{
		buf=aTracker->iBusyQue->First();
		while(buf!=aTracker->iBusyQue->Last())
			{
			Kern::Printf("iBusyQue buffer at 0x%x\n",buf);
			TAny* intermediatePtr = (TAny*)buf;
			if((aTracker->iType == EGetState)||(aTracker->iType == ESetState))
				{
				TTrackSetStateBuf* tempBuf =(TTrackSetStateBuf*)intermediatePtr;
				Kern::Printf("buffer control block at 0x%x\n", (TInt)&tempBuf->iCtrlBlock);
				}
			buf= buf->iNext;
			};
		}
	}
#endif

TInt DChannelResManUs::InitTrackingControl(TTrackingControl*& aTracker, TUint8 aType, TUint8 aNumBuffers)
	{
// Set the tracker type, create the tracking queues and required tracking buffers.
// Assign all the tracking buffers to the free queue.
//
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::InitTrackingControl()"));

	TInt r = KErrNone;
	aTracker->iType = (TAsyncOpType)aType;
	aTracker->iOwningChannel = this;
	aTracker->iFreeQue = new SDblQue();
	__ASSERT_DEBUG(aTracker->iFreeQue != NULL, RESMANUS_FAULT());
	if(aTracker->iFreeQue == NULL)
		r = KErrNoMemory;
	if(r==KErrNone)
		{
		aTracker->iBusyQue = new SDblQue();
		__ASSERT_DEBUG(aTracker->iBusyQue != NULL, RESMANUS_FAULT());
		if(aTracker->iBusyQue == NULL)
			{
			delete aTracker->iFreeQue;
			r = KErrNoMemory;
			}
		}
	if(r==KErrNone)
		{
		for(TUint8 i=0; (i<aNumBuffers) && (r==KErrNone) ;i++)
			{
			TAny* buf = NULL;
			TAny* ptr=NULL; // To be assigned to non-NULL value later
			switch(aTracker->iType)
				{
				case EGetState:
					{
					buf = (TAny*)(new TTrackGetStateBuf(&AsyncCallBackFn,ptr,iDfcQ,KResManCallBackPriority));
					r = Kern::CreateClientDataRequest2(((TTrackGetStateBuf*)buf)->iRequest);
					break;
					}
				case ESetState:
					{
					buf = (TAny*)(new TTrackSetStateBuf(&AsyncCallBackFn, ptr, iDfcQ, KResManCallBackPriority));
					r = Kern::CreateClientRequest(((TTrackSetStateBuf*)buf)->iRequest);
					break;
					}
				case ENotify:
					{
					buf = (TAny*)(new TTrackNotifyBuf(&AsyncCallBackFn,ptr,iDfcQ,KResManCallBackPriority));
					r = Kern::CreateClientRequest(((TTrackNotifyBuf*)buf)->iRequest);
					break;
					}
				default:
					__ASSERT_ALWAYS(0,RESMANUS_FAULT());
				}
			__ASSERT_DEBUG(buf!=NULL, RESMANUS_FAULT());
			if((buf == NULL) || (r == KErrNoMemory))
				{
				r = KErrNoMemory;
				break;
				}
			else
				{
				((TTrackingBuffer*)buf)->SetTrackingControl(aTracker);
				(aTracker->iFreeQue)->Add((SDblQueLink*)buf);
				((TTrackingBuffer*)buf)->SetQue(aTracker->iFreeQue);
				}
			}
		// If buffer allocation failed, need to remove all previously-allocated buffers and the queues
		if(r!=KErrNone)
			{
			SDblQueLink* ptr = (aTracker->iFreeQue)->First();
			do
				{
				SDblQueLink* next = NULL;
				if(ptr !=NULL)
					next = ptr->iNext;
				delete ptr;
				ptr = next;
				} while ((ptr!=NULL)&&(ptr!=(aTracker->iFreeQue)->Last()));
			delete aTracker->iFreeQue;
			delete aTracker->iBusyQue;
			}
		}
	return r;
	}


void DChannelResManUs::RemoveTrackingControl(TTrackingControl*& aTracker)
    {
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("DChannelResManUs::RemoveTrackingControl()"));

	// Free the resource-tracking links and their respective queues
	if(aTracker->iFreeQue!=NULL)
		{
		while(!aTracker->iFreeQue->IsEmpty())
			{
			TTrackingBuffer *buf = (TTrackingBuffer *)aTracker->iFreeQue->GetFirst(); //Dequeues the element
			DELETE_TRACKING_BUFFER(aTracker,buf)
			}
		delete aTracker->iFreeQue;
		}

	if(aTracker->iBusyQue!=NULL)
		{
		while(!aTracker->iBusyQue->IsEmpty())
			{
			TTrackingBuffer *buf = (TTrackingBuffer *)aTracker->iBusyQue->GetFirst(); //Dequeues the element
			DELETE_TRACKING_BUFFER(aTracker,buf)
			}
		delete aTracker->iBusyQue;
		}
	delete aTracker;
    }


#ifdef RESOURCE_MANAGER_SIMULATED_PSL
void DChannelResManUs::CheckForCandidateAsyncResource(TPowerResourceInfoV01* aResource)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("> DChannelResManUs::CheckForCandidateAsyncResource"));
	// Proceed only if we already have less that the maximum number of candidate resources
	if(iNoCandidateAsyncRes >= MAX_NUM_CANDIDATE_RESOURCES)
		return;
	// For the purposes of asynchronous testing, we need a long latency resource
	if(((TInt)(aResource->iLatencyGet)==(TInt)(EResLongLatency)) && 
		((TInt)(aResource->iLatencySet)==(TInt)(EResLongLatency)))
		{
		// An additional requirement is that the level of the resource can be 
		// updated a sufficient amount of times to support the required testing.
		if(((aResource->iMaxLevel - aResource->iMinLevel) > LEVEL_GAP_REQUIRED_FOR_ASYNC_TESTING) &&
			((TInt)(aResource->iSense) == (TInt)(EResPositive)) )
			{
			TInt r=((DPowerResourceController*)iPddPtr)->GetResourceId(ClientHandle(), *(aResource->iResourceName), iCandidateAsyncResIds[iNoCandidateAsyncRes]);
			if(r!=KErrNone)
				{
				__KTRACE_OPT(KRESMANAGER, Kern::Printf("Failed to identify long latency resource\n"));
				}
			else
				{
				__KTRACE_OPT(KRESMANAGER, Kern::Printf("Potential async resource ID = %d\n",iCandidateAsyncResIds[iNoCandidateAsyncRes]));
				iHaveLongLatencyResource = ETrue;
				++iNoCandidateAsyncRes;
				}
			}
		}
	}


void DChannelResManUs::GetNumCandidateAsyncResources(TUint& aNumResources)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("> DChannelResManUs::GetNumCandidateAsyncResources"));

	aNumResources = iNoCandidateAsyncRes;
	}

TInt DChannelResManUs::GetCandidateAsyncResourceId(TUint aIndex, TUint& aResourceId)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("> DChannelResManUs::GetCandidateAsyncResourceId"));
	TInt r = KErrNone;
	if(aIndex>=iNoCandidateAsyncRes)
		r = KErrNotFound;
	else
		aResourceId = iCandidateAsyncResIds[aIndex];
	return r;
	}

void DChannelResManUs::CheckForCandidateSharedResource(TPowerResourceInfoV01* aResource)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("> DChannelResManUs::CheckForCandidateSharedResource"));

	// Proceed only if we already have less that the maximum number of candidate resources
	if(iNoCandidateSharedRes >= MAX_NUM_CANDIDATE_RESOURCES)
		return;

	// For the purposes of testing shared usgae of resources, we need a shareable resource
	if((TInt)(aResource->iUsage)==(TInt)(EResShared))
		{
		// An additional requirement is that the level of the resource can be 
		// updated a sufficient amount of times to support the required testing.
		if(((aResource->iMaxLevel - aResource->iMinLevel) > LEVEL_GAP_REQUIRED_FOR_SHARED_TESTING) &&
			((TInt)(aResource->iSense) == (TInt)(EResPositive)) )
			{
			TInt r=((DPowerResourceController*)iPddPtr)->GetResourceId(ClientHandle(), *(aResource->iResourceName), iCandidateSharedResIds[iNoCandidateSharedRes]);
			if(r!=KErrNone)
				{
				__KTRACE_OPT(KRESMANAGER, Kern::Printf("Failed to identify shared resource\n"));
				}
			else
				{
				__KTRACE_OPT(KRESMANAGER, Kern::Printf("Potential shared resource ID = %d\n",iCandidateSharedResIds[iNoCandidateAsyncRes]));
				iHaveLongLatencyResource = ETrue;
				++iNoCandidateSharedRes;
				}
			}
		}
	}

void DChannelResManUs::GetNumCandidateSharedResources(TUint& aNumResources)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("> DChannelResManUs::GetNumCandidateSharedResources"));

	aNumResources = iNoCandidateSharedRes;
	}

TInt DChannelResManUs::GetCandidateSharedResourceId(TUint aIndex, TUint& aResourceId)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("> DChannelResManUs::GetCandidateSharedResourceId"));
	TInt r = KErrNone;
	if(aIndex>=iNoCandidateSharedRes)
		r = KErrNotFound;
	else
		aResourceId = iCandidateSharedResIds[aIndex];
	return r;
	}

#endif

TInt DChannelResManUs::ExtractResourceInfo(const TPowerResourceInfoV01* aPwrResInfo, TResourceInfoBuf& aInfo)
	{
// Extract data from a TPowerResourceInfoV01 object to a TResourceInfo instance
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("> DChannelResManUs::ExtractResourceInfo"));

	TInt r=KErrNone;
	TInt copyLength=(((aInfo().iName).MaxLength())<((aPwrResInfo->iResourceName)->Length()))?
					(aInfo().iName).MaxLength():
					(aPwrResInfo->iResourceName)->Length();
	(aInfo().iName).Copy((aPwrResInfo->iResourceName)->Ptr(),copyLength);
	aInfo().iId = aPwrResInfo->iResourceId;
	aInfo().iClass	= (TResourceClass)aPwrResInfo->iClass;
	aInfo().iType	= (TResourceType)aPwrResInfo->iType;
	aInfo().iUsage	= (TResourceUsage)aPwrResInfo->iUsage;
	aInfo().iSense	= (TResourceSense)aPwrResInfo->iSense;
	aInfo().iMinLevel = aPwrResInfo->iMinLevel;
	aInfo().iMaxLevel = aPwrResInfo->iMaxLevel;

#ifdef _DUMP_TRACKERS
	r=DumpResource(aPwrResInfo);
#endif
	return r;
	}

#ifdef _DUMP_TRACKERS
TInt DChannelResManUs::DumpResource(const TPowerResourceInfoV01* aResource)
	{
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("> DChannelResManUs::DumpResource"));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("Resource name = %S \n",aResource->iResourceName));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("Resource ID = 0x%d \n",aResource->iResourceId));
	switch(aResource->iClass)
		{
		case DStaticPowerResource::EPhysical:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("class = EPhysical\n"));
			break;
			}
		case DStaticPowerResource::ELogical:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("class = ELogical\n"));
			break;
			}
		default:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("class = % is UNKNOWN!\n"));
			return KErrGeneral;
			}
		}
	switch(aResource->iType)
		{
		case DStaticPowerResource::EBinary:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("type = EBinary\n"));
			break;
			}
		case DStaticPowerResource::EMultilevel:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("type = EMultilevel\n"));
			break;
			}
		case DStaticPowerResource::EMultiProperty:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("type = EMultiProperty\n"));
			break;
			}
		default:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("type = % is UNKNOWN!\n"));
			return KErrGeneral;
			}
		}
	switch(aResource->iUsage)
		{
		case DStaticPowerResource::ESingleUse:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("usage = ESingleUse\n"));
			break;
			}
		case DStaticPowerResource::EShared:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("usage = EShared\n"));
			break;
			}
		default:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("usage = % is UNKNOWN!\n"));
			return KErrGeneral;
			}
		}
	switch(aResource->iSense)
		{
		case DStaticPowerResource::EPositive:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("sense = EPositive\n"));
			break;
			}
		case DStaticPowerResource::ENegative:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("sense = ENegative\n"));
			break;
			}
		case DStaticPowerResource::ECustom:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("sense = ECustom\n"));
			break;
			}
		default:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("sense = % is UNKNOWN!\n"));
			return KErrGeneral;
			}
		}
	switch(aResource->iLatencyGet)
		{
		case DStaticPowerResource::EInstantaneous:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("latency get = EInstantaneous\n"));
			break;
			}
		case DStaticPowerResource::ENegative:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("latency get = ELongLatency\n"));
			break;
			}
		default:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("latency get = % is UNKNOWN!\n"));
			return KErrGeneral;
			}
		}
	switch(aResource->iLatencySet)
		{
		case DStaticPowerResource::EInstantaneous:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("latency set = EInstantaneous\n"));
			break;
			}
		case DStaticPowerResource::ENegative:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("latency set = ELongLatency\n"));
			break;
			}
		default:
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("latency set = % is UNKNOWN!\n"));
			return KErrGeneral;
			}
		}
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("DefaultLevel = %d\n",aResource->iDefaultLevel));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("MinLevel = %d\n",aResource->iMinLevel));
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("MaxLevel = %d\n",aResource->iMaxLevel));

	return KErrNone;
	}
#endif

#ifndef RESOURCE_MANAGER_SIMULATED_PSL
DECLARE_EXTENSION_LDD()
	{
	return new DDeviceResManUs;
	}


DECLARE_STANDARD_EXTENSION()
	{
	DDeviceResManUs* device = new DDeviceResManUs;
	__KTRACE_OPT(KBOOT, Kern::Printf("DECLARE_STANDARD_EXTENSION, device = 0x%x\n",device));

	if(device == NULL)
		return KErrNoMemory;
	else
		{
		device->iSharedDfcQue = new TDfcQue();
		if(device->iSharedDfcQue==NULL)
			return KErrNoMemory;

		return (Kern::InstallLogicalDevice(device));
		}
	}
#else
DECLARE_STANDARD_LDD()
	{
	TInt r = DSimulatedPowerResourceController::CompleteResourceControllerInitialisation();
	if(r != KErrNone)
		{
		// Unconditionally print message 
		__KTRACE_OPT(KRESMANAGER, Kern::Printf("DECLARE_STANDARD_LDD: initialise Resource Controller failed with %d\n",r));
		return NULL;
		}
	DDeviceResManUs* device = new DDeviceResManUs;
	return device;
	}
#endif
