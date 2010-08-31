// Copyright (c) 2000-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32/drivers/usbc/d_usbc.cpp
// LDD for USB Device driver stack:
// The channel object.
// 
//

/**
 @file d_usbc.cpp
 @internalTechnology
*/

#include <drivers/usbc.h>
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "d_usbcTraces.h"
#endif



_LIT(KUsbLddName, "Usbc");

static const TInt KUsbRequestCallbackPriority = 2;


// Quick sanity check on endpoint properties
static TBool ValidateEndpoint(const TUsbcEndpointInfo* aEndpointInfo)
	{
	const TUint dir = aEndpointInfo->iDir;
	const TInt size = aEndpointInfo->iSize;
	if (size <= 0)
		return EFalse;

	switch (aEndpointInfo->iType)
		{
	case KUsbEpTypeControl:
		if (dir != KUsbEpDirBidirect || size > 64)
			return EFalse;
		break;
	case KUsbEpTypeIsochronous:
		if ((dir != KUsbEpDirIn && dir != KUsbEpDirOut) || size > 1024)
			return EFalse;
		break;
	case KUsbEpTypeBulk:
		if ((dir != KUsbEpDirIn && dir != KUsbEpDirOut) || size > 512)
			return EFalse;
		break;
	case KUsbEpTypeInterrupt:
		if ((dir != KUsbEpDirIn && dir != KUsbEpDirOut) || size > 1024)
			return EFalse;
		break;
	default:
		return EFalse;
		}
	return ETrue;
	}


/** Real entry point from the Kernel: return a new driver.
 */
DECLARE_STANDARD_LDD()
	{
	return new DUsbcLogDevice;
	}


/** Create a channel on the device.

	@internalComponent
*/
TInt DUsbcLogDevice::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel = new DLddUsbcChannel;
	return aChannel ? KErrNone : KErrNoMemory;
	}


DUsbcLogDevice::DUsbcLogDevice()
      {
	  iParseMask = KDeviceAllowUnit;
	  iUnitsMask = 0xffffffff;								// Leave units decision to the Controller
      iVersion = TVersion(KUsbcMajorVersion, KUsbcMinorVersion, KUsbcBuildVersion);
      }


TInt DUsbcLogDevice::Install()
	{
	// Only proceed if we have the Controller underneath us
	if (!DUsbClientController::UsbcControllerPointer())
		{
		OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLOGDEVICE_INSTALL,
		        "LDD Install: USB Controller Not Present" );
		
		return KErrGeneral;
		}
	return SetName(&KUsbLddName);
	}


//
// Return the USB controller capabilities.
//
void DUsbcLogDevice::GetCaps(TDes8& aDes) const
	{
	TPckgBuf<TCapsDevUsbc> b;
	b().version = iVersion;
	Kern::InfoCopy(aDes, b);
	}


//
// Constructor
//
DLddUsbcChannel::DLddUsbcChannel()
	: iValidInterface(EFalse),
	  iAlternateSettingList(NULL),
	  iCompleteAllCallbackInfo(this, DLddUsbcChannel::EmergencyCompleteDfc, KUsbRequestCallbackPriority),
	  iStatusChangePtr(NULL),
	  iStatusCallbackInfo(this, DLddUsbcChannel::StatusChangeCallback, KUsbRequestCallbackPriority),
	  iEndpointStatusChangePtr(NULL),
	  iEndpointStatusCallbackInfo(this, DLddUsbcChannel::EndpointStatusChangeCallback,
								  KUsbRequestCallbackPriority),
      iOtgFeatureChangePtr(NULL),
      iOtgFeatureCallbackInfo(this, DLddUsbcChannel::OtgFeatureChangeCallback, KUsbRequestCallbackPriority),
	  iNumberOfEndpoints(0),
	  iDeviceState(EUsbcDeviceStateUndefined),
	  iOwnsDeviceControl(EFalse),
	  iAlternateSetting(0),
	  iDeviceStatusNeeded(EFalse),
	  iChannelClosing(EFalse)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DLDDUSBCCHANNEL_DLDDUSBCCHANNEL_CONS, 
	        "*** DLddUsbcChannel::DLddUsbcChannel CTOR" );
	
	iClient = &Kern::CurrentThread();
	iClient->Open();
	for (TInt i = 1; i <= KMaxEndpointsPerClient; i++)
		{
		iEndpoint[i] = NULL;
		}
	for (TInt i = 1; i < KUsbcMaxRequests; i++)
		{
		iRequestStatus[i] = NULL;
		}
	}


DLddUsbcChannel::~DLddUsbcChannel()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DLDDUSBCCHANNEL_DLDDUSBCCHANNEL_DES, 
	        "DLddUsbcChannel::~DLddUsbcChannel()" );
	
	if (iController)
		{
		iController->DeRegisterClient(this);
		iStatusCallbackInfo.Cancel();
		iEndpointStatusCallbackInfo.Cancel();
        iOtgFeatureCallbackInfo.Cancel();
        iCompleteAllCallbackInfo.Cancel();
		AbortInterface();
		DestroyAllInterfaces();
		if (iOwnsDeviceControl)
			{
			iController->ReleaseDeviceControl(this);
			iOwnsDeviceControl = EFalse;
			}
		DestroyEp0();
		delete iStatusFifo;
		Kern::DestroyClientRequest(iStatusChangeReq);
		Kern::DestroyClientRequest(iEndpointStatusChangeReq);
		Kern::DestroyClientRequest(iOtgFeatureChangeReq);

		Kern::DestroyVirtualPinObject(iPinObj1);
		Kern::DestroyVirtualPinObject(iPinObj2);
		Kern::DestroyVirtualPinObject(iPinObj3);

		for (TInt i = 0; i < KUsbcMaxRequests; i++)
			{
			Kern::DestroyClientBufferRequest(iClientAsynchNotify[i]->iBufferRequest);
			delete iClientAsynchNotify[i];
			}
		}
	Kern::SafeClose((DObject*&)iClient, NULL);
	}


inline TBool DLddUsbcChannel::ValidEndpoint(TInt aEndpoint)
	{
	return (aEndpoint <= iNumberOfEndpoints && aEndpoint >= 0);
	}


//
// Create channel
//
TInt DLddUsbcChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& aVer)
	{
	OstTraceDefExt3( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCREATE, 
	        "LDD DoCreateL 1 Ver = %02d %02d %02d", aVer.iMajor, aVer.iMinor, aVer.iBuild );
	
	if (!Kern::CurrentThreadHasCapability(ECapabilityCommDD,
										  __PLATSEC_DIAGNOSTIC_STRING("Checked by USBC.LDD (USB Driver)")))
		{
		return KErrPermissionDenied;
		}

	iController = DUsbClientController::UsbcControllerPointer();

	if (!iController)
		{
		return KErrGeneral;
		}

	iStatusFifo = new TUsbcDeviceStatusQueue;
	if (iStatusFifo == NULL)
		{
		return KErrNoMemory;
		}

  	if (!Kern::QueryVersionSupported(TVersion(KUsbcMajorVersion, KUsbcMinorVersion, KUsbcBuildVersion), aVer))
		{
		return KErrNotSupported;
		}

	// set up the correct DFC queue
	SetDfcQ(iController->DfcQ(0));							// sets the channel's dfc queue
	#ifdef DFC_REALTIME_STATE
		iDfcQ.SetRealtimeState(ERealtimeStateOn);
	#endif
    iCompleteAllCallbackInfo.SetDfcQ(iDfcQ);
	iStatusCallbackInfo.SetDfcQ(iDfcQ);						// use the channel's dfcq for this dfc
	iEndpointStatusCallbackInfo.SetDfcQ(iDfcQ);				// use the channel's dfcq for this dfc
    iOtgFeatureCallbackInfo.SetDfcQ(iDfcQ);
	iMsgQ.Receive();										//start up the message q
	TInt r = iController->RegisterClientCallback(iCompleteAllCallbackInfo);
	if (r != KErrNone)
		return r;
	r = iController->RegisterForStatusChange(iStatusCallbackInfo);
	if (r != KErrNone)
		return r;
	r = iController->RegisterForEndpointStatusChange(iEndpointStatusCallbackInfo);
	if (r != KErrNone)
		return r;
	r = iController->RegisterForOtgFeatureChange(iOtgFeatureCallbackInfo);
	if (r != KErrNone)
		return r;

	r = Kern::CreateClientDataRequest(iStatusChangeReq);
	if (r != KErrNone)
		return r;
	r = Kern::CreateClientDataRequest(iEndpointStatusChangeReq);
	if (r != KErrNone)
		return r;
	r = Kern::CreateClientDataRequest(iOtgFeatureChangeReq);
	if (r != KErrNone)
		return r;
	
	Kern::CreateVirtualPinObject(iPinObj1);
	Kern::CreateVirtualPinObject(iPinObj2);
	Kern::CreateVirtualPinObject(iPinObj3);

	for (TInt i = 0; i < KUsbcMaxRequests; i++)
		{
			iClientAsynchNotify[i] = new TClientAsynchNotify;
			if(iClientAsynchNotify[i] == NULL)
				return KErrNoMemory;
			r = Kern::CreateClientBufferRequest(iClientAsynchNotify[i]->iBufferRequest,1,TClientBufferRequest::EPinVirtual);
			if (r != KErrNone)
				{
				delete iClientAsynchNotify[i];
				iClientAsynchNotify[i]=NULL;
				return r;
				}
		}
	
	return r;
	}



void DLddUsbcChannel::CompleteBufferRequest(DThread* aThread, TInt aReqNo, TInt aReason)
{
	iRequestStatus[aReqNo]=NULL;
	Kern::QueueBufferRequestComplete(aThread, iClientAsynchNotify[aReqNo]->iBufferRequest, aReason);
}


TClientBuffer * DLddUsbcChannel::GetClientBuffer(TInt aEndpoint)
{
	return iClientAsynchNotify[aEndpoint]->iClientBuffer;
}

//Runs in client thread
TInt DLddUsbcChannel::SendMsg(TMessageBase * aMsg)
{
	TThreadMessage& m=* (TThreadMessage*)aMsg;
	TInt id = m.iValue;
	
	TInt r = KErrNone;
	//Cancel Request
	if (id == KMaxTInt)
		{
			r = DLogicalChannel::SendMsg(aMsg);
			return r;
		}
	if (id < 0)
		{
		// DoRequest
		TRequestStatus* pS = (TRequestStatus*) m.Ptr0();
		r = PreSendRequest(aMsg,~id, pS, m.Ptr1(), m.Ptr2());
		if (r == KErrNone)
			{
			r = DLogicalChannel::SendMsg(aMsg);
			}
		}
	else
		{
		//SendControl
		r = SendControl(aMsg);
		}
	return r;
}


TInt DLddUsbcChannel::PreSendRequest(TMessageBase * aMsg,TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2)
{
	TInt r = KErrNone;
	if (aReqNo >= KUsbcMaxRequests)
	{
		Kern::RequestComplete(aStatus, KErrNotSupported);
		return KErrNotSupported;
	}
	if (aReqNo > KUsbcMaxEpNumber)//DoOtherAsyncReq
	{
		switch (aReqNo)
		{
			case RDevUsbcClient::ERequestEndpointStatusNotify:
				iEndpointStatusChangeReq->Reset();
				iEndpointStatusChangeReq->SetStatus(aStatus);
				iEndpointStatusChangeReq->SetDestPtr(a1);
			break;
			case RDevUsbcClient::ERequestOtgFeaturesNotify:
				iOtgFeatureChangeReq->Reset();
				iOtgFeatureChangeReq->SetStatus(aStatus);
				iOtgFeatureChangeReq->SetDestPtr(a1);
			break;
			case RDevUsbcClient::ERequestAlternateDeviceStatusNotify:
				iStatusChangeReq->Reset();
				iStatusChangeReq->SetStatus(aStatus);
				iStatusChangeReq->SetDestPtr(a1);
			break;
			case RDevUsbcClient::ERequestReEnumerate://WE use bufferrequest to complete even tho we dont add any buffers
				iClientAsynchNotify[aReqNo]->Reset();
				r=iClientAsynchNotify[aReqNo]->iBufferRequest->StartSetup(aStatus);
				if (r != KErrNone)
					return r;
				iClientAsynchNotify[aReqNo]->iBufferRequest->EndSetup();
			break;
		}
	}
	else //DoTransferAsyncReq
	{
			if(a1 == NULL)
				return KErrArgument;
			iClientAsynchNotify[aReqNo]->Reset();
			r=iClientAsynchNotify[aReqNo]->iBufferRequest->StartSetup(aStatus);
			if (r != KErrNone)
				return r;
			kumemget(&iTfrInfo,a1,sizeof(TEndpointTransferInfo));
			r=iClientAsynchNotify[aReqNo]->iBufferRequest->AddBuffer(iClientAsynchNotify[aReqNo]->iClientBuffer, iTfrInfo.iDes);
			if (r != KErrNone)
				return r;
			iClientAsynchNotify[aReqNo]->iBufferRequest->EndSetup();
			TThreadMessage& m=*(TThreadMessage*)aMsg;
			m.iArg[1] = (TAny*)&iTfrInfo; //Use Channel owned TransfereInfo structure 
	}
	return KErrNone;
}


void DLddUsbcChannel::HandleMsg(TMessageBase* aMsg)
	{
	TThreadMessage& m = *(TThreadMessage*)aMsg;
	TInt id = m.iValue;
	if (id == (TInt) ECloseMsg)
		{
		iChannelClosing = ETrue;
		m.Complete(KErrNone, EFalse);
		return;
		}
	else if (id == KMaxTInt)
		{
		// Cancel request
		TInt mask = m.Int0();
		TInt b = 1;
		for(TInt reqNo = 0; reqNo < KUsbcMaxRequests; reqNo++)
			{
			TRequestStatus* pS = iRequestStatus[reqNo];
			if ((mask & b) && (pS != NULL))
				{
				DoCancel(reqNo);
				}
			b <<= 1;
			}
		m.Complete(KErrNone, ETrue);
		return;
		}

	if (id < 0)
		{
		// DoRequest
		TRequestStatus* pS = (TRequestStatus*) m.Ptr0();
		DoRequest(~id, pS, m.Ptr1(), m.Ptr2());
		m.Complete(KErrNone, ETrue);
		}
	else
		{
		// DoControl
		TInt r = DoControl(id, m.Ptr0(), m.Ptr1());
		m.Complete(r, ETrue);
		}
	}


//
// Overriding DObject virtual
//
TInt DLddUsbcChannel::RequestUserHandle(DThread* aThread, TOwnerType /*aType*/)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_REQUESTUSERHANDLE,
	        "DLddUsbcChannel::RequestUserHandle" );
	// The USB client LDD is not designed for a channel to be shared between
	// threads. It saves a pointer to the current thread when it is opened, and
	// uses this to complete any asynchronous requests.
	// It is therefore not acceptable for the handle to be duplicated and used
	// by another thread:
	if (aThread == iClient)
		{
		return KErrNone;
		}
	else
		{
		return KErrAccessDenied;
		}
	}


//
// Asynchronous requests - overriding pure virtual
//
void DLddUsbcChannel::DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2)
	{
	// Check on request status
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOREQUEST, 
	        "DoRequest 0x%08x", aReqNo );
		TInt r = KErrNone;
		if (iRequestStatus[aReqNo] != NULL)
			{
			DestroyAllInterfaces();
			PanicClientThread(ERequestAlreadyPending);
			}
		else
			{
			TBool needsCompletion;
			iRequestStatus[aReqNo] = aStatus;

			if (aReqNo > KUsbcMaxEpNumber)
				{
				r = DoOtherAsyncReq(aReqNo, a1, a2, needsCompletion);
				if (needsCompletion)
					{
					switch (aReqNo)
					{
						case RDevUsbcClient::ERequestEndpointStatusNotify:
							iRequestStatus[aReqNo]=NULL;
							Kern::QueueRequestComplete(iClient,iEndpointStatusChangeReq,r);
						break;
						case RDevUsbcClient::ERequestOtgFeaturesNotify:
							iRequestStatus[aReqNo]=NULL;
							Kern::QueueRequestComplete(iClient,iOtgFeatureChangeReq,r);
						break;
						case RDevUsbcClient::ERequestAlternateDeviceStatusNotify:
							iRequestStatus[aReqNo]=NULL;
							Kern::QueueRequestComplete(iClient,iStatusChangeReq,r);
						break;
						case RDevUsbcClient::ERequestReEnumerate:
							iRequestStatus[aReqNo]=NULL;
							Kern::QueueBufferRequestComplete(iClient, iClientAsynchNotify[aReqNo]->iBufferRequest, r);
						break;
					}
				  }
				}
			else
				{
				r = DoTransferAsyncReq(aReqNo, a1, a2, needsCompletion);
				if (needsCompletion)
					{
					//Kern::RequestComplete(iClient, iRequestStatus[aReqNo], r);
					CompleteBufferRequest(iClient, aReqNo, r);
					}
				}
			}
	}



TInt DLddUsbcChannel::DoOtherAsyncReq(TInt aReqNo, TAny* a1, TAny* a2, TBool& aNeedsCompletion)
	{
	// The general assumption is that none of these will complete now.
	// However, those that make this function return something other than
	// KErrNone will get completed by the calling function.
	// So, 1) If you are returning KErrNone but really need to complete because
	//        completion criteria can be met (for example, sufficient data is
	//        available in the buffer) and then set aNeedsCompletion = ETrue.
	//     2) Do NOT complete here AT ALL.
	//
	aNeedsCompletion = EFalse;
	TInt r = KErrNone;

	switch (aReqNo)
		{
	case RDevUsbcClient::ERequestAlternateDeviceStatusNotify:
		{
		OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOOTHERASYNCREQ, 
		        "EControlReqDeviceStatusNotify" );
		if (a1 != NULL)
			{
			iDeviceStatusNeeded = ETrue;
			iStatusChangePtr = a1;
			aNeedsCompletion = AlternateDeviceStateTestComplete();
			}
		else
			r = KErrArgument;
		break;
		}
	case RDevUsbcClient::ERequestReEnumerate:
		{
	    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOOTHERASYNCREQ_DUP1, 
	            "ERequestReEnumerate" );
		// If successful, this will complete via the status notification.
		r = iController->ReEnumerate();
		break;
		}
	case RDevUsbcClient::ERequestEndpointStatusNotify:
		{
	    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOOTHERASYNCREQ_DUP2, 
	            "ERequestEndpointStatusNotify" );
		if (a1 != NULL)
			{
			iEndpointStatusChangePtr = a1;
			}
		else
			r = KErrArgument;
		break;
			}
	case RDevUsbcClient::ERequestOtgFeaturesNotify:
		{
		OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOOTHERASYNCREQ_DUP3, 
		        "ERequestOtgFeaturesNotify" );
		if (a1 != NULL)
			{
            iOtgFeatureChangePtr = a1;
            }
		else
			r = KErrArgument;
        break;
        }
    default:
		r = KErrNotSupported;
		}

	aNeedsCompletion = aNeedsCompletion || (r != KErrNone);

	return r;
	}


TInt DLddUsbcChannel::DoTransferAsyncReq(TInt aEndpointNum, TAny* a1, TAny* a2, TBool& aNeedsCompletion)
	{
	// The general assumption is that none of these will complete now.
	// however, those that are returning something other than KErrNone will get completed
	// by the calling function.
	// So,	1) if you are returning KErrNone but really need to complete because completion criteria can be met
	//			(for example, sufficient data is available in the buffer) and then set aNeedsCompletion=ETrue..
	//		2) Do NOT complete here AT ALL.
	//
	aNeedsCompletion = EFalse;
	TInt r = KErrNone;
	TUsbcEndpoint* pEndpoint = NULL;
	TUsbcEndpointInfo* pEndpointInfo = NULL;
	TEndpointTransferInfo* pTfr = NULL;

	if (aEndpointNum == 0)
		{
		// ep0 requests
		if (!(iValidInterface || iOwnsDeviceControl))
			{
			OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ,
			        "DoRequest rejected: not configured (Ep0)" );
			r = KErrUsbInterfaceNotReady;
			goto exit;
			}
		}
	else
		{
		// other eps
		if (!(iValidInterface && (iDeviceState == EUsbcDeviceStateConfigured ||
		                          iDeviceState == EUsbcDeviceStateSuspended))
		   )
			{
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP1,
			        "DoRequest rejected not configured (Ep %d)", aEndpointNum );
			r = KErrUsbInterfaceNotReady;
			goto exit;
			}
		}

	if (!ValidEndpoint(aEndpointNum))
		{
		OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP2,
		        "  Error: DoRequest Read: in error complete" );
		r = KErrUsbEpNotInInterface;
		goto exit;
 		}

	if (a1 == NULL)
		{
		r = KErrArgument;
		goto exit;
		}
	pTfr = (TEndpointTransferInfo *)a1;

	if (pTfr->iTransferSize < 0)
		{
		r = KErrArgument;
		goto exit;
		}
	pEndpoint = iEndpoint[aEndpointNum];
	if (!pEndpoint)
		{
		OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP3,
		        "  Error: DoRequest Read: in error complete" );
		r = KErrUsbEpNotInInterface;
		goto exit;
		}

	pEndpointInfo = pEndpoint->EndpointInfo();
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP4,
	        "DoRequest %d", aEndpointNum );

	switch (pTfr->iTransferType)
		{

	case ETransferTypeReadData:
	case ETransferTypeReadPacket:
	case ETransferTypeReadUntilShort:
	case ETransferTypeReadOneOrMore:
		{
		OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP5,
		        "DoRequest Read" );
		if (pEndpoint->iDmaBuffers->RxIsActive())
			{
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP6,
			        "**** ReadReq ep%d RxActive", aEndpointNum );
			}
		else
			{
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP7,
			        "**** ReadReq ep%d RxInActive", aEndpointNum );
			}

		if (pEndpointInfo->iDir != KUsbEpDirOut &&
			pEndpointInfo->iDir != KUsbEpDirBidirect)
			{
			// Trying to do the wrong thing
			OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP8,
			        "  Error: DoRequest Read: in error complete" );
			r = KErrUsbEpBadDirection;
			break;
			}
		// Set the length of data to zero now to catch all cases
		TPtrC8 pZeroDesc(NULL, 0);
		r=Kern::ThreadBufWrite(iClient,iClientAsynchNotify[aEndpointNum]->iClientBuffer, pZeroDesc, 0, 0,iClient);
		if (r != KErrNone)
			PanicClientThread(r);
		pEndpoint->SetTransferInfo(pTfr);
		if (pEndpoint->iDmaBuffers->IsReaderEmpty())
			{
			pEndpoint->SetClientReadPending(ETrue);
			}
		else
			{
			if (pTfr->iTransferType == ETransferTypeReadPacket)
				{
				OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP9,
				        "DoRequest Read packet: data available complete" );
				r = pEndpoint->CopyToClient(iClient,iClientAsynchNotify[aEndpointNum]->iClientBuffer);
				aNeedsCompletion = ETrue;
				break;
				}
			else if (pTfr->iTransferType == ETransferTypeReadData)
				{
				if (pTfr->iTransferSize <= pEndpoint->RxBytesAvailable())
					{
					OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP10,
					        "DoRequest Read data: data available complete");
					r = pEndpoint->CopyToClient(iClient,iClientAsynchNotify[aEndpointNum]->iClientBuffer);
					aNeedsCompletion = ETrue;
					break;
					}
				else
					{
					pEndpoint->SetClientReadPending(ETrue);
					}
				}
			else if (pTfr->iTransferType == ETransferTypeReadOneOrMore)
				{
				if (pEndpoint->RxBytesAvailable() > 0)
					{
					OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP11,
					        "DoRequest Read data: data available complete" );
					r = pEndpoint->CopyToClient(iClient,iClientAsynchNotify[aEndpointNum]->iClientBuffer);
					aNeedsCompletion = ETrue;
					break;
					}
				else
					{
					pEndpoint->SetClientReadPending(ETrue);
					}
				}
			else if (pTfr->iTransferType == ETransferTypeReadUntilShort)
				{
				TInt nRx = pEndpoint->RxBytesAvailable();
				TInt maxPacketSize = pEndpoint->EndpointInfo()->iSize;
				if( (pTfr->iTransferSize <= nRx) ||
					(nRx < maxPacketSize) ||
					pEndpoint->iDmaBuffers->ShortPacketExists())
					{
                    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP12,
                            "DoRequest Read data: data available complete" );

					r = pEndpoint->CopyToClient(iClient,iClientAsynchNotify[aEndpointNum]->iClientBuffer);
					aNeedsCompletion = ETrue;
					}
				else
					{
					pEndpoint->SetClientReadPending(ETrue);
					}
				}
			}
		r = pEndpoint->TryToStartRead(EFalse);
		if (r != KErrNone)
			{
            OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP13,
                    "DoRequest Read: couldn't start read" );

			r = KErrNone;									// Reader full isn't a userside error;
			}
		break;
		}

	case ETransferTypeWrite:
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP14,
                "DoRequest Write 1" );
		if (pEndpointInfo->iDir != KUsbEpDirIn &&
			pEndpointInfo->iDir != KUsbEpDirBidirect)
			{
			OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP15,
			        "  Error: DoRequest Write: wrong direction complete" );
			r = KErrUsbEpBadDirection;
			break;
			}
		OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP16,
		        "DoRequest Write 2" );


		TInt desLength=iClientAsynchNotify[aEndpointNum]->iClientBuffer->Length();
		
		if (desLength < pTfr->iTransferSize)
			{
			OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP17,
			        "  Error: DoRequest Write: user buffer too short" );
			r = KErrUsbTransferSize;
			break;
			}

		OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP18,
		        "DoRequest Write 3 length=%d maxlength=%d", pTfr->iTransferSize, desLength);
		// Zero length writes are acceptable
		pEndpoint->SetClientWritePending(ETrue);
		r = pEndpoint->TryToStartWrite(pTfr);
		if (r != KErrNone)
			{
			OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP19,
			        "  Error: DoRequest Write: couldn't start write" );
			pEndpoint->SetClientWritePending(EFalse);
			}
		break;
		}

	default:
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DLDDUSBCCHANNEL_DOTRANSFERASYNCREQ_DUP20,
                "  Error: DoTransferAsyncReq: pTfr->iTransferType = %d not supported", pTfr->iTransferType);

		r = KErrNotSupported;
		break;
		}
 exit:
	aNeedsCompletion = aNeedsCompletion || (r != KErrNone);
	return r;
	}


//
// Cancel an outstanding request - overriding pure virtual
//
TInt DLddUsbcChannel::DoCancel(TInt aReqNo)
	{
	TInt r = KErrNone;
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCANCEL, 
	        "DoCancel: 0x%x", aReqNo );
	if (aReqNo <= iNumberOfEndpoints)
		{
		OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCANCEL_DUP1, 
		        "DoCancel endpoint: 0x%x", aReqNo );

		iEndpoint[aReqNo]->CancelTransfer(iClient,iClientAsynchNotify[aReqNo]->iClientBuffer);
		}
	else if (aReqNo == RDevUsbcClient::ERequestAlternateDeviceStatusNotify)
		{
		OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCANCEL_DUP2, 
		        "DoCancel: ERequestAlternateDeviceStatusNotify 0x%x", aReqNo );

		iDeviceStatusNeeded = EFalse;
		iStatusFifo->FlushQueue();
		if (iStatusChangePtr)
			{
			iStatusChangeReq->Data()=iController->GetDeviceStatus();
			iStatusChangePtr = NULL;

			if (iStatusChangeReq->IsReady())
				{
				iRequestStatus[aReqNo] = NULL;
				Kern::QueueRequestComplete(iClient, iStatusChangeReq, KErrCancel);
				}
				return KErrNone;
			}
		}
	else if (aReqNo == RDevUsbcClient::ERequestReEnumerate)
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCANCEL_DUP3, 
                "DoCancel ERequestReEnumerate: 0x%x", aReqNo );

		}
	else if (aReqNo == RDevUsbcClient::ERequestEndpointStatusNotify)
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCANCEL_DUP4,
                "DoCancel ERequestEndpointStatusNotify: 0x%x", aReqNo );

		CancelNotifyEndpointStatus();
		if (iEndpointStatusChangeReq->IsReady())
			{
			iRequestStatus[aReqNo] = NULL;
			Kern::QueueRequestComplete(iClient, iEndpointStatusChangeReq, KErrCancel);
			}
		return KErrNone;
		}
	else if (aReqNo == RDevUsbcClient::ERequestOtgFeaturesNotify)
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCANCEL_DUP5,
                "DoCancel ERequestOtgFeaturesNotify: 0x%x", aReqNo );

		CancelNotifyOtgFeatures();
		if (iOtgFeatureChangeReq->IsReady())
			{
			iRequestStatus[aReqNo] = NULL;
			Kern::QueueRequestComplete(iClient, iOtgFeatureChangeReq, KErrCancel);
			}
		}
	else
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCANCEL_DUP6,
                "DoCancel Unknown! 0x%x", aReqNo );

		}

		if (r == KErrNone)
			r = KErrCancel;

		CompleteBufferRequest(iClient, aReqNo, r);
	return r;
	}


void DLddUsbcChannel::CancelNotifyEndpointStatus()
	{
	if (iEndpointStatusChangePtr)
		{
		TUint epBitmap = 0;
		for (TInt i = 0; i <= iNumberOfEndpoints; i++)
			{
			TInt v = iController->GetEndpointStatus(this, iEndpoint[i]->RealEpNumber());
			TUint b;
			(v == EEndpointStateStalled) ? b = 1 : b = 0;
			epBitmap |= b << i;
			}
		iEndpointStatusChangeReq->Data()=epBitmap;
		iEndpointStatusChangePtr = NULL;
		}
	}


void DLddUsbcChannel::CancelNotifyOtgFeatures()
	{
    if (iOtgFeatureChangePtr)
        {
        TUint8 features;
        iController->GetCurrentOtgFeatures(features);
	 iOtgFeatureChangeReq->Data()=features;
        iOtgFeatureChangePtr = NULL;
        }
    }

TInt DLddUsbcChannel::PinMemory(TDesC8 *aDes, TVirtualPinObject *aPinObj)
	{
	TInt r = KErrNone;
	TInt  len,mlen;
	
	const TUint8*p = Kern::KUDesInfo(*aDes, len,mlen);
	r=Kern::PinVirtualMemory(aPinObj, (TLinAddr) p, len);
	return r;
	}

//Called in Client thread context
TInt DLddUsbcChannel::SendControl(TMessageBase* aMsg)
	{
	TThreadMessage& m=*(TThreadMessage*)aMsg;
	const TInt fn=m.iValue;
	TAny *const a1=m.Ptr0();
	TAny *const a2=m.Ptr1();
	TInt  kern_param;
	TEndpointDescriptorInfo epi;
	TUsbcIfcInfo ifc;
	TCSDescriptorInfo desInfo;
	TInt r = KErrNone;

	switch (fn)
		{
		
	case RDevUsbcClient::EControlDeviceStatus:
	case RDevUsbcClient::EControlGetAlternateSetting:
		m.iArg[0] = &kern_param;  			// update message to point to kernel-side buffer
		break;

	case RDevUsbcClient::EControlQueryReceiveBuffer:
	case RDevUsbcClient::EControlEndpointStatus:
		m.iArg[1] = &kern_param;  			// update message to point to kernel-side buffer
		break;

	case RDevUsbcClient::EControlEndpointCaps:
	case RDevUsbcClient::EControlDeviceCaps:
	case RDevUsbcClient::EControlGetDeviceDescriptor:
	case RDevUsbcClient::EControlSetDeviceDescriptor:
	case RDevUsbcClient::EControlGetDeviceDescriptorSize:
	case RDevUsbcClient::EControlGetConfigurationDescriptor:
	case RDevUsbcClient::EControlGetConfigurationDescriptorSize:
	case RDevUsbcClient::EControlGetDeviceQualifierDescriptor:
	case RDevUsbcClient::EControlSetDeviceQualifierDescriptor:
	case RDevUsbcClient::EControlGetOtherSpeedConfigurationDescriptor:
	case RDevUsbcClient::EControlSetOtherSpeedConfigurationDescriptor:
	case RDevUsbcClient::EControlGetStringDescriptorLangId:
	case RDevUsbcClient::EControlGetManufacturerStringDescriptor:
	case RDevUsbcClient::EControlSetManufacturerStringDescriptor:
	case RDevUsbcClient::EControlGetProductStringDescriptor:
	case RDevUsbcClient::EControlSetProductStringDescriptor:
	case RDevUsbcClient::EControlGetSerialNumberStringDescriptor:	
	case RDevUsbcClient::EControlSetSerialNumberStringDescriptor:
	case RDevUsbcClient::EControlGetConfigurationStringDescriptor:
	case RDevUsbcClient::EControlSetConfigurationStringDescriptor:
	case RDevUsbcClient::EControlSetOtgDescriptor:
	case RDevUsbcClient::EControlGetOtgDescriptor:
	case RDevUsbcClient::EControlGetOtgFeatures:
		r=PinMemory((TDesC8 *) a1, iPinObj1);
		if(r!=KErrNone)
			{
			PanicClientThread(r);
			return r;
			}
		break;

	case RDevUsbcClient::EControlGetInterfaceDescriptor:
	case RDevUsbcClient::EControlGetInterfaceDescriptorSize:
	case RDevUsbcClient::EControlSetInterfaceDescriptor:
	case RDevUsbcClient::EControlGetCSInterfaceDescriptor:
	case RDevUsbcClient::EControlGetCSInterfaceDescriptorSize:
	case RDevUsbcClient::EControlGetStringDescriptor:
	case RDevUsbcClient::EControlSetStringDescriptor:
		r=PinMemory((TDesC8 *) a2, iPinObj1);
		if(r!=KErrNone)
			{
			PanicClientThread(r);
			return r;
			}
		break;

	case RDevUsbcClient::EControlGetEndpointDescriptor:
	case RDevUsbcClient::EControlGetEndpointDescriptorSize:
	case RDevUsbcClient::EControlSetEndpointDescriptor:
	case RDevUsbcClient::EControlGetCSEndpointDescriptor:
	case RDevUsbcClient::EControlGetCSEndpointDescriptorSize:
		if(a1!=NULL)
			{
			r=Kern::PinVirtualMemory(iPinObj1, (TLinAddr)a1, sizeof(epi));
			if(r!=KErrNone)
				{
				PanicClientThread(r);
				return r;
				}
			kumemget(&epi, a1, sizeof(epi));
			r=PinMemory((TDesC8 *) epi.iArg, iPinObj2);
			if(r!=KErrNone)
				{
				Kern::UnpinVirtualMemory(iPinObj1);
				PanicClientThread(r);
				return r;
				}
			}
		break;

	case RDevUsbcClient::EControlSetInterface:
		if(a2!=NULL)
			{
			r=Kern::PinVirtualMemory(iPinObj1, (TLinAddr)a2, sizeof(ifc));
			if(r!=KErrNone)
				{
				PanicClientThread(r);
				return r;
				}	
			kumemget(&ifc, a2, sizeof(ifc));				
			r=PinMemory((TDesC8 *) ifc.iInterfaceData, iPinObj2);
			if(r!=KErrNone)
				{
				Kern::UnpinVirtualMemory(iPinObj1);
				PanicClientThread(r);
				return r;
				}
			}
		break;

	case RDevUsbcClient::EControlSetCSInterfaceDescriptor:
	case RDevUsbcClient::EControlSetCSEndpointDescriptor:
		if(a1!=NULL)
			{
			r=Kern::PinVirtualMemory(iPinObj1, (TLinAddr)a1, sizeof(desInfo));
			if(r!=KErrNone)
				{
				PanicClientThread(r);
				return r;
				}
			kumemget(&desInfo, a1, sizeof(desInfo));
			r=PinMemory((TDesC8 *) desInfo.iArg, iPinObj2);
			if(r!=KErrNone)
				{
				Kern::UnpinVirtualMemory(iPinObj1);
				PanicClientThread(r);
				return r;
				}
			}
		break;
	}


	//Send Message and wait for synchronous complete	
	r = DLogicalChannel::SendMsg(aMsg);
	
	
	
	switch (fn)
		{
	case RDevUsbcClient::EControlDeviceStatus:
	case RDevUsbcClient::EControlGetAlternateSetting:
		umemput32(a1, &kern_param, sizeof(kern_param));
		break;

	case RDevUsbcClient::EControlQueryReceiveBuffer:
	case RDevUsbcClient::EControlEndpointStatus:
		umemput32(a2, &kern_param, sizeof(kern_param));
		break;

	case RDevUsbcClient::EControlDeviceCaps:
	case RDevUsbcClient::EControlEndpointCaps:
	case RDevUsbcClient::EControlGetDeviceDescriptor:
	case RDevUsbcClient::EControlSetDeviceDescriptor:
	case RDevUsbcClient::EControlGetDeviceDescriptorSize:
	case RDevUsbcClient::EControlGetConfigurationDescriptor:
	case RDevUsbcClient::EControlGetConfigurationDescriptorSize:
	case RDevUsbcClient::EControlGetDeviceQualifierDescriptor:
	case RDevUsbcClient::EControlSetDeviceQualifierDescriptor:
	case RDevUsbcClient::EControlGetOtherSpeedConfigurationDescriptor:
	case RDevUsbcClient::EControlSetOtherSpeedConfigurationDescriptor:
	case RDevUsbcClient::EControlGetStringDescriptorLangId:
	case RDevUsbcClient::EControlGetManufacturerStringDescriptor:
	case RDevUsbcClient::EControlSetManufacturerStringDescriptor:
	case RDevUsbcClient::EControlGetProductStringDescriptor:
	case RDevUsbcClient::EControlSetProductStringDescriptor:
	case RDevUsbcClient::EControlGetSerialNumberStringDescriptor:	
	case RDevUsbcClient::EControlSetSerialNumberStringDescriptor:
	case RDevUsbcClient::EControlGetConfigurationStringDescriptor:
	case RDevUsbcClient::EControlSetConfigurationStringDescriptor:
	case RDevUsbcClient::EControlSetOtgDescriptor:
	case RDevUsbcClient::EControlGetOtgDescriptor:
	case RDevUsbcClient::EControlGetOtgFeatures:
		if(a1!=NULL)
			{
			Kern::UnpinVirtualMemory(iPinObj1);
			}
		break;

	case RDevUsbcClient::EControlGetInterfaceDescriptor:
	case RDevUsbcClient::EControlGetInterfaceDescriptorSize:
	case RDevUsbcClient::EControlSetInterfaceDescriptor:
	case RDevUsbcClient::EControlGetCSInterfaceDescriptor:
	case RDevUsbcClient::EControlGetCSInterfaceDescriptorSize:
	case RDevUsbcClient::EControlGetStringDescriptor:
	case RDevUsbcClient::EControlSetStringDescriptor:
		if(a2!=NULL)
			{
			Kern::UnpinVirtualMemory(iPinObj1);
			}
		break;
	
	case RDevUsbcClient::EControlGetEndpointDescriptor:
	case RDevUsbcClient::EControlGetEndpointDescriptorSize:
	case RDevUsbcClient::EControlSetEndpointDescriptor:
	case RDevUsbcClient::EControlGetCSEndpointDescriptor:
	case RDevUsbcClient::EControlGetCSEndpointDescriptorSize:
	case RDevUsbcClient::EControlSetCSInterfaceDescriptor:
	case RDevUsbcClient::EControlSetCSEndpointDescriptor:
		if(a1!=NULL)
			{
			Kern::UnpinVirtualMemory(iPinObj1);
			Kern::UnpinVirtualMemory(iPinObj2);
			}
		break;

	case RDevUsbcClient::EControlSetInterface:
		if(a2!=NULL)
			{
			Kern::UnpinVirtualMemory(iPinObj1);
			Kern::UnpinVirtualMemory(iPinObj2);
			}
		break;
		}

	return r;
    }


TInt DLddUsbcChannel::DoControl(TInt aFunction, TAny* a1, TAny* a2)
	{
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL, 
            "DoControl: %d", aFunction );

	TInt r = KErrNone;
	TInt ep;
	TUsbcEndpoint* pEndpoint;
	TPtrC8 pZeroDesc(NULL, 0);
	TEndpointDescriptorInfo epInfo;
	TUsbcIfcInfo ifcInfo;
	TCSDescriptorInfo desInfo;
	TUsbcEndpointResource epRes;
	TInt bandwidthPriority;

	switch (aFunction)
		{
	case RDevUsbcClient::EControlEndpointZeroRequestError:
	    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP1, 
	            "EControlEndpointZeroRequestError" );

		r = KErrNone;
		if (iOwnsDeviceControl || (iValidInterface && iDeviceState == EUsbcDeviceStateConfigured))
			{
			iController->Ep0Stall(this);
			}
		else
			{
			if (iDeviceState != EUsbcDeviceStateConfigured)
				r = KErrUsbDeviceNotConfigured;
			else
				r = KErrUsbInterfaceNotReady;
			}
		break;

	case RDevUsbcClient::EControlGetAlternateSetting:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP2,
                "EControlGetAlternateSetting" );

		if (iValidInterface && iDeviceState == EUsbcDeviceStateConfigured)
			{
			r = iController->GetInterfaceNumber(this, *(TInt*)a1);
			}
		else
			{
			if (iDeviceState != EUsbcDeviceStateConfigured)
				r = KErrUsbDeviceNotConfigured;
			else
				r = KErrUsbInterfaceNotReady;
			}
		break;

	case RDevUsbcClient::EControlDeviceStatus:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP3,
                "EControlDeviceStatus" );

		*(TInt*)a1 = iController->GetDeviceStatus();
		break;

	case RDevUsbcClient::EControlEndpointStatus:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP4,
                "EControlEndpointStatus" );

		if (iValidInterface && ValidEndpoint((TInt) a1))
			{
			pEndpoint = iEndpoint[(TInt)a1];
			if (pEndpoint == NULL)
				r = KErrNotSupported;
			else
				{
				*(TInt*)a2 = iController->GetEndpointStatus(this, iEndpoint[(TInt)a1]->RealEpNumber());
				}
			}
		else
			{
			if (iDeviceState != EUsbcDeviceStateConfigured)
				r = KErrUsbDeviceNotConfigured;
			else
				r = KErrUsbInterfaceNotReady;
			}
		break;

	case RDevUsbcClient::EControlQueryReceiveBuffer:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP5,
                "EControlQueryReceiveBuffer" );

		if (iValidInterface && ValidEndpoint((TInt) a1))
			{
			pEndpoint=iEndpoint[(TInt) a1];
			if (pEndpoint == NULL)
				r = KErrNotSupported;
			else if (pEndpoint->EndpointInfo()->iDir != KUsbEpDirIn)
				{
		        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP6,
		                "  bytes = %d", pEndpoint->RxBytesAvailable());
				*(TInt*)a2 = pEndpoint->RxBytesAvailable();
				}
			}
		else
			{
			if (iDeviceState != EUsbcDeviceStateConfigured)
				r = KErrUsbDeviceNotConfigured;
			else
				r = KErrUsbInterfaceNotReady;
			}
		break;

	case RDevUsbcClient::EControlEndpointCaps:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP7,
                "EControlEndpointCaps" );
		r = Kern::ThreadDesWrite(iClient, a1, pZeroDesc, 0, 0, iClient);
		if (r != KErrNone)
			PanicClientThread(r);
		iController->EndpointCaps(this, *((TDes8*) a1));
		break;

	case RDevUsbcClient::EControlDeviceCaps:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP8,
                "EControlDeviceCaps" );
		r = Kern::ThreadDesWrite(iClient, a1, pZeroDesc, 0, 0, iClient);
		if (r != KErrNone)
			PanicClientThread(r);
		iController->DeviceCaps(this, *((TDes8*) a1));
		break;

	case RDevUsbcClient::EControlSendEp0StatusPacket:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP9,
                "EControlSendEp0StatusPacket" );
		iController->SendEp0StatusPacket(this);
		break;

	case RDevUsbcClient::EControlHaltEndpoint:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP10,
                "EControlHaltEndpoint" );
		if (iValidInterface && ValidEndpoint((TInt) a1))
			{
			r = iController->HaltEndpoint(this, iEndpoint[(TInt)a1]->RealEpNumber());
			}
		else
			{
			if (iDeviceState != EUsbcDeviceStateConfigured)
				r = KErrUsbDeviceNotConfigured;
			else
				r = KErrUsbInterfaceNotReady;
			}
		break;

	case RDevUsbcClient::EControlClearHaltEndpoint:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP11,
                "EControlClearHaltEndpoint" );
		if (iValidInterface && ValidEndpoint((TInt) a1))
			{
			r = iController->ClearHaltEndpoint(this, iEndpoint[(TInt)a1]->RealEpNumber());
			}
		else
			{
			if (iDeviceState != EUsbcDeviceStateConfigured)
				r = KErrUsbDeviceNotConfigured;
			else
				r = KErrUsbInterfaceNotReady;
			}
		break;

	case RDevUsbcClient::EControlDumpRegisters:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP12,
                "EControlDumpRegisters" );
		iController->DumpRegisters();
		break;

	case RDevUsbcClient::EControlReleaseDeviceControl:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP13,
                "EControlReleaseDeviceControl" );
		iController->ReleaseDeviceControl(this);
		iOwnsDeviceControl = EFalse;
		break;

	case RDevUsbcClient::EControlEndpointZeroMaxPacketSizes:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP14,
                "EControlEndpointZeroMaxPacketSizes" );
		r = iController->EndpointZeroMaxPacketSizes();
		break;

	case RDevUsbcClient::EControlSetEndpointZeroMaxPacketSize:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP15,
                "EControlSetEndpointZeroMaxPacketSize" );
		r = iController->SetEndpointZeroMaxPacketSize(reinterpret_cast<TInt>(a1));
		break;

	case RDevUsbcClient::EControlGetEndpointZeroMaxPacketSize:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP16,
                "EControlGetEndpointZeroMaxPacketSize" );
		r = iController->Ep0PacketSize();
		break;

	case RDevUsbcClient::EControlGetDeviceDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP17,
                "EControlGetDeviceDescriptor" );
		r = Kern::ThreadDesWrite(iClient, a1, pZeroDesc, 0, 0, iClient);
		if (r != KErrNone)
			PanicClientThread(r);
		r = iController->GetDeviceDescriptor(iClient, *((TDes8*) a1));
		break;

	case RDevUsbcClient::EControlSetDeviceDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP18,
                "EControlSetDeviceDescriptor" );
		if (a1 != NULL)
			r = iController->SetDeviceDescriptor(iClient, *((TDes8*) a1));
		else
			r = KErrArgument;
		break;

	case RDevUsbcClient::EControlGetDeviceDescriptorSize:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP19,
                "EControlGetDeviceDescriptorSize" );
		if (a1 != NULL)
			r = iController->GetDeviceDescriptorSize(iClient, *((TDes8*) a1));
		else
			r = KErrArgument;
		break;

	case RDevUsbcClient::EControlGetConfigurationDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP20,
                "EControlGetConfigurationDescriptor" );
		r = Kern::ThreadDesWrite(iClient, a1, pZeroDesc, 0 , 0, iClient);
		if (r != KErrNone)
			PanicClientThread(r);
		r = iController->GetConfigurationDescriptor(iClient, *((TDes8*) a1));
		break;

	case RDevUsbcClient::EControlGetConfigurationDescriptorSize:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP21,
                "EControlGetConfigurationDescriptorSize" );
		if (a1 != NULL)
			{
			r = iController->GetConfigurationDescriptorSize(iClient, *((TDes8*) a1));
			}
		else
			r = KErrArgument;
		break;

	case RDevUsbcClient::EControlSetConfigurationDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP22,
                "EControlSetConfigurationDescriptor" );
		r = iController->SetConfigurationDescriptor(iClient, *((TDes8*) a1));
		break;

	case RDevUsbcClient::EControlGetInterfaceDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP23,
                "EControlGetInterfaceDescriptor" );
		r = iController->GetInterfaceDescriptor(iClient, this, (TInt) a1, *((TDes8*) a2));
		break;

	case RDevUsbcClient::EControlGetInterfaceDescriptorSize:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP24,
                "EControlGetInterfaceDescriptorSize" );
		r = iController->GetInterfaceDescriptorSize(iClient, this, (TInt) a1, *(TDes8*) a2);
		break;

	case RDevUsbcClient::EControlSetInterfaceDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP25,
                "EControlSetInterfaceDescriptor" );
		r = iController->SetInterfaceDescriptor(iClient, this, (TInt) a1, *((TDes8*) a2));
		break;

	case RDevUsbcClient::EControlGetEndpointDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP26,
                "EControlGetEndpointDescriptor" );
		r = Kern::ThreadRawRead(iClient, a1, &epInfo, sizeof(epInfo));
		if (r != KErrNone)
			PanicClientThread(r);
		ep = EpFromAlternateSetting(epInfo.iSetting, epInfo.iEndpoint);
		r = iController->GetEndpointDescriptor(iClient, this, epInfo.iSetting,
											   ep, *(TDes8*) epInfo.iArg);
		break;

	case RDevUsbcClient::EControlGetEndpointDescriptorSize:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP27,
                "EControlGetEndpointDescriptorSize" );
		r = Kern::ThreadRawRead(iClient, a1, &epInfo, sizeof(epInfo));
		if (r != KErrNone)
			PanicClientThread(r);
		ep = EpFromAlternateSetting(epInfo.iSetting, epInfo.iEndpoint);
		r = iController->GetEndpointDescriptorSize(iClient, this, epInfo.iSetting,
												   ep, *(TDes8*) epInfo.iArg);
		break;

	case RDevUsbcClient::EControlSetEndpointDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP28,
                "EControlSetEndpointDescriptor" );
		r = Kern::ThreadRawRead(iClient, a1, &epInfo, sizeof(epInfo));
		if (r != KErrNone)
			PanicClientThread(r);
		ep = EpFromAlternateSetting(epInfo.iSetting, epInfo.iEndpoint);
		r = iController->SetEndpointDescriptor(iClient, this, epInfo.iSetting,
											   ep, *(TDes8*)epInfo.iArg);
		break;

	case RDevUsbcClient::EControlGetDeviceQualifierDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP29,
                "EControlGetDeviceQualifierDescriptor" );
		r = Kern::ThreadDesWrite(iClient, a1, pZeroDesc, 0, 0, iClient);
		if (r != KErrNone)
			PanicClientThread(r);
		r = iController->GetDeviceQualifierDescriptor(iClient, *((TDes8*) a1));
		break;

	case RDevUsbcClient::EControlSetDeviceQualifierDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP30,
                "EControlSetDeviceQualifierDescriptor" );
		if (a1 != NULL)
			r = iController->SetDeviceQualifierDescriptor(iClient, *((TDes8*) a1));
		else
			r = KErrArgument;
		break;

	case RDevUsbcClient::EControlGetOtherSpeedConfigurationDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP31,
                "EControlGetOtherSpeedConfigurationDescriptor" );
		r = Kern::ThreadDesWrite(iClient, a1, pZeroDesc, 0 , 0, iClient);
		if (r != KErrNone)
			PanicClientThread(r);
		r = iController->GetOtherSpeedConfigurationDescriptor(iClient, *((TDes8*) a1));
		break;

	case RDevUsbcClient::EControlSetOtherSpeedConfigurationDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP32,
                "EControlSetOtherSpeedConfigurationDescriptor" );
		r = iController->SetOtherSpeedConfigurationDescriptor(iClient, *((TDes8*) a1));
		break;


	case RDevUsbcClient::EControlGetCSInterfaceDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP33,
                "EControlGetCSInterfaceDescriptor" );
		r = iController->GetCSInterfaceDescriptorBlock(iClient, this, (TInt) a1, *((TDes8*) a2));
		break;

	case RDevUsbcClient::EControlGetCSInterfaceDescriptorSize:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP34,
                "EControlGetCSInterfaceDescriptorSize" );
		r = iController->GetCSInterfaceDescriptorBlockSize(iClient, this, (TInt) a1, *(TDes8*) a2);
		break;

	case RDevUsbcClient::EControlGetCSEndpointDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP35,
                "EControlGetCSEndpointDescriptor" );
		r = Kern::ThreadRawRead(iClient, a1, &epInfo, sizeof(epInfo));
		if (r != KErrNone)
			PanicClientThread(r);
		ep = EpFromAlternateSetting(epInfo.iSetting, epInfo.iEndpoint);
		r = iController->GetCSEndpointDescriptorBlock(iClient, this, epInfo.iSetting,
													  ep, *(TDes8*) epInfo.iArg);
		break;

	case RDevUsbcClient::EControlGetCSEndpointDescriptorSize:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP36,
                "EControlGetCSEndpointDescriptorSize" );
		r = Kern::ThreadRawRead(iClient, a1, &epInfo, sizeof(epInfo));
		if (r != KErrNone)
			PanicClientThread(r);
		ep = EpFromAlternateSetting(epInfo.iSetting, epInfo.iEndpoint);
		r = iController->GetCSEndpointDescriptorBlockSize(iClient, this, epInfo.iSetting,
														  ep, *(TDes8*) epInfo.iArg);
		break;

	case RDevUsbcClient::EControlSignalRemoteWakeup:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP37,
                "EControlSignalRemoteWakeup" );
		r = iController->SignalRemoteWakeup();
		break;

	case RDevUsbcClient::EControlDeviceDisconnectFromHost:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP38,
                "EControlDeviceDisconnectFromHost" );
		r = iController->UsbDisconnect();
		break;

	case RDevUsbcClient::EControlDeviceConnectToHost:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP39,
                "EControlDeviceConnectToHost" );
		r = iController->UsbConnect();
		break;

	case RDevUsbcClient::EControlDevicePowerUpUdc:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP40,
                "EControlDevicePowerUpUdc" );
		r = iController->PowerUpUdc();
		break;

	case RDevUsbcClient::EControlSetDeviceControl:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP41,
                "EControlSetDeviceControl" );
		r = iController->SetDeviceControl(this);
		if (r == KErrNone)
			{
			iOwnsDeviceControl = ETrue;
			if (iEndpoint[0] == NULL)
				{
		        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP42,
		                "EControlSetDeviceControl 11" );
				r = SetupEp0();
				if (r != KErrNone)
					{
	               OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DLDDUSBCCHANNEL_DOCONTROL_DUP43,
	                       "  Error: SetupEp0() failed" );

					iController->ReleaseDeviceControl(this);
					DestroyEp0();
					iOwnsDeviceControl = EFalse;
					}
				iEndpoint[0]->TryToStartRead(EFalse);
				}
			}
		else
			r = KErrInUse;
		break;

	case RDevUsbcClient::EControlCurrentlyUsingHighSpeed:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP44,
                "EControlCurrentlyUsingHighSpeed" );
		r = iController->CurrentlyUsingHighSpeed();
		break;

	case RDevUsbcClient::EControlSetInterface:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP45,
                "EControlSetInterface" );
		r = Kern::ThreadRawRead(iClient, a2, &ifcInfo, sizeof(ifcInfo));
		if (r != KErrNone)
			PanicClientThread(r);
		if (iValidInterface && (iDeviceState == EUsbcDeviceStateConfigured))
			{
			r = KErrGeneral;
			}
		else
			{
			bandwidthPriority = ifcInfo.iBandwidthPriority;
			if ((bandwidthPriority & 0xffffff00) ||
				((bandwidthPriority & 0x0f) >= KUsbcDmaBufMaxPriorities) ||
				(((bandwidthPriority >> 4) & 0x0f) >= KUsbcDmaBufMaxPriorities))
				{
				r = KErrArgument;
				}
			else
				{
				r = SetInterface((TInt) a1, &ifcInfo);
				}
			}
			
		break;

	case RDevUsbcClient::EControlReleaseInterface:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP46,
                "EControlReleaseInterface" );
		r = iController->ReleaseInterface(this, (TInt) a1);
		if (r == KErrNone)
			{
			DestroyInterface((TUint) a1);
			}
		else
			{
	        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DLDDUSBCCHANNEL_DOCONTROL_DUP47,
	                "  Error in PIL: LDD interface won't be released." );
			}
		break;

	case RDevUsbcClient::EControlSetCSInterfaceDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP48,
                "EControlSetCSInterfaceDescriptor" );
		r = Kern::ThreadRawRead(iClient, a1, &desInfo, sizeof(desInfo));
		if (r != KErrNone)
			PanicClientThread(r);
		r = iController->SetCSInterfaceDescriptorBlock(iClient, this, desInfo.iSetting,
													   *reinterpret_cast<const TDes8*>(desInfo.iArg),
													   desInfo.iSize);
		break;

	case RDevUsbcClient::EControlSetCSEndpointDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP49,
                "EControlSetCSEndpointDescriptor" );
		r = Kern::ThreadRawRead(iClient, a1, &desInfo, sizeof(desInfo));
		if (r != KErrNone)
			PanicClientThread(r);
		ep = EpFromAlternateSetting(desInfo.iSetting, desInfo.iEndpoint);
		r = iController->SetCSEndpointDescriptorBlock(iClient, this, desInfo.iSetting, ep,
													  *reinterpret_cast<const TDes8*>(desInfo.iArg),
													  desInfo.iSize);
		break;

	case RDevUsbcClient::EControlGetStringDescriptorLangId:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP50,
                "EControlGetStringDescriptorLangId" );
		r = iController->GetStringDescriptorLangId(iClient, *((TDes8*) a1));
		break;

	case RDevUsbcClient::EControlSetStringDescriptorLangId:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP51,
                "EControlSetStringDescriptorLangId" );
		r = iController->SetStringDescriptorLangId(reinterpret_cast<TUint>(a1));
		break;

	case RDevUsbcClient::EControlGetManufacturerStringDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP52,
                "EControlGetManufacturerStringDescriptor" );
		r = iController->GetManufacturerStringDescriptor(iClient, *((TPtr8*) a1));
		break;

	case RDevUsbcClient::EControlSetManufacturerStringDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP53,
                "EControlSetManufacturerStringDescriptor" );
		r = iController->SetManufacturerStringDescriptor(iClient, *((TPtr8*) a1));
		break;

	case RDevUsbcClient::EControlRemoveManufacturerStringDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP54,
                "EControlRemoveManufacturerStringDescriptor" );
		r = iController->RemoveManufacturerStringDescriptor();
		break;

	case RDevUsbcClient::EControlGetProductStringDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP55,
                "EControlGetProductStringDescriptor" );
		r = iController->GetProductStringDescriptor(iClient, *((TPtr8*) a1));
		break;

	case RDevUsbcClient::EControlSetProductStringDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP56,
                "EControlSetProductStringDescriptor" );
		r = iController->SetProductStringDescriptor(iClient, *((TPtr8*) a1));
		break;

	case RDevUsbcClient::EControlRemoveProductStringDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP57,
                "EControlRemoveProductStringDescriptor" );
		r = iController->RemoveProductStringDescriptor();
		break;

	case RDevUsbcClient::EControlGetSerialNumberStringDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP58,
                "EControlGetSerialNumberStringDescriptor" );
		r = iController->GetSerialNumberStringDescriptor(iClient, *((TPtr8*) a1));
		break;

	case RDevUsbcClient::EControlSetSerialNumberStringDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP59,
                "EControlSetSerialNumberStringDescriptor" );
		r = iController->SetSerialNumberStringDescriptor(iClient, *((TPtr8*) a1));
		break;

	case RDevUsbcClient::EControlRemoveSerialNumberStringDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP60,
                "EControlRemoveSerialNumberStringDescriptor" );
		r = iController->RemoveSerialNumberStringDescriptor();
		break;

	case RDevUsbcClient::EControlGetConfigurationStringDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP61,
                "EControlGetConfigurationStringDescriptor" );
		r = iController->GetConfigurationStringDescriptor(iClient, *((TPtr8*) a1));
		break;

	case RDevUsbcClient::EControlSetConfigurationStringDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP62,
                "EControlSetConfigurationStringDescriptor" );
		r = iController->SetConfigurationStringDescriptor(iClient, *((TPtr8*) a1));
		break;

	case RDevUsbcClient::EControlRemoveConfigurationStringDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP63,
                "EControlRemoveConfigurationStringDescriptor" );
		r = iController->RemoveConfigurationStringDescriptor();
		break;

	case RDevUsbcClient::EControlGetStringDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP64,
                "EControlGetStringDescriptor" );
		r = iController->GetStringDescriptor(iClient, (TUint8) (TInt) a1, *((TPtr8*) a2));
		break;

	case RDevUsbcClient::EControlSetStringDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP65,
                "EControlSetStringDescriptor" );
		r = iController->SetStringDescriptor(iClient, (TUint8) (TInt) a1, *((TPtr8*) a2));
		break;

	case RDevUsbcClient::EControlRemoveStringDescriptor:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP66,
                "EControlRemoveStringDescriptor" );
		r = iController->RemoveStringDescriptor((TUint8) (TInt) a1);
		break;

	case RDevUsbcClient::EControlAllocateEndpointResource:
		epRes = (TUsbcEndpointResource)((TInt) a2);
		if (!ValidEndpoint((TInt)a1))
			{
			r = KErrUsbEpNotInInterface;
			}
		else
			{
			r = iController->AllocateEndpointResource(this, iEndpoint[(TInt)a1]->RealEpNumber(), epRes);
			}
		break;

	case RDevUsbcClient::EControlDeAllocateEndpointResource:
		epRes = (TUsbcEndpointResource)((TInt) a2);
		if (!ValidEndpoint((TInt)a1))
			{
			r = KErrUsbEpNotInInterface;
			}
		else
			{
			r = iController->DeAllocateEndpointResource(this, iEndpoint[(TInt)a1]->RealEpNumber(), epRes);
			}
		break;

	case RDevUsbcClient::EControlQueryEndpointResourceUse:
		epRes = (TUsbcEndpointResource)((TInt) a2);
		if (!ValidEndpoint((TInt)a1))
			{
			r = KErrUsbEpNotInInterface;
			}
		else
			{
			r = iController->QueryEndpointResource(this, iEndpoint[(TInt)a1]->RealEpNumber(), epRes);
			}
		break;

	case RDevUsbcClient::EControlSetOtgDescriptor:
		{
		r = iController->SetOtgDescriptor(iClient, *((const TDesC8*)a1));
		}
		break;

	case RDevUsbcClient::EControlGetOtgDescriptor:
		{
		r = iController->GetOtgDescriptor(iClient, *((TDes8*)a1));
		}
		break;

	case RDevUsbcClient::EControlGetOtgFeatures:
		{
		r = iController->GetOtgFeatures(iClient, *((TDes8*)a1));
		}
		break;

    default:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOCONTROL_DUP67,
                "Function code not supported" );
		r = KErrNotSupported;
		}

	return r;
	}


TInt DLddUsbcChannel::SetInterface(TInt aInterfaceNumber, TUsbcIfcInfo* aInfoBuf)
	{
	TUsbcInterfaceInfoBuf ifc_info_buf;
	TUsbcInterfaceInfoBuf* const ifc_info_buf_ptr = aInfoBuf->iInterfaceData;
	const TInt srcLen = Kern::ThreadGetDesLength(iClient, ifc_info_buf_ptr);
	if (srcLen < ifc_info_buf.Length())
		{
		OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETINTERFACE, 
		        "SetInterface can't copy" );
		PanicClientThread(EDesOverflow);
		}

	TInt r = Kern::ThreadDesRead(iClient, ifc_info_buf_ptr, ifc_info_buf, 0, KChunkShiftBy0);
	if (r != KErrNone)
		{
	    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETINTERFACE_DUP1,
	            "SetInterface Copy failed reason=%d", r );
		PanicClientThread(r);
		}

	TUsbcEndpointInfo* pEndpointData = ifc_info_buf().iEndpointData;

	// If an alternate interface is being asked for then do nothing,
	// just pass it down to the Controller.
	const TInt num_endpoints = ifc_info_buf().iTotalEndpointsUsed;
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETINTERFACE_DUP2,
            "SetInterface num_endpoints=%d", num_endpoints );

	// [The next 4 variables have to be initialized here because of the goto's that follow.]
	// Both IN and OUT buffers will be fully cached:
	const TUint32 cacheAttribs = EMapAttrSupRw | EMapAttrCachedMax;
	const TUint32 bandwidthPriority = aInfoBuf->iBandwidthPriority;

	// Supports ep0+5 endpoints
	TInt real_ep_numbers[6] = {-1, -1, -1, -1, -1, -1};

    // See if PIL will accept this interface
    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETINTERFACE_DUP3,
            "SetInterface Calling controller" );
	r = iController->SetInterface(this,
								  iClient,
								  aInterfaceNumber,
								  ifc_info_buf().iClass,
								  aInfoBuf->iString,
								  ifc_info_buf().iTotalEndpointsUsed,
								  ifc_info_buf().iEndpointData,
								  &real_ep_numbers,
								  ifc_info_buf().iFeatureWord);

    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETINTERFACE_DUP4,
            "SetInterface controller returned %d", r );
	if (r != KErrNone)
		{
	    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETINTERFACE_DUP5, 
	            "SetInterface failed reason=%d", r );
		return r;
		}

	// [The next variable has to be initialized here because of the goto's that follow.]
	TUsbcAlternateSettingList* alternateSettingListRec;

	// ep0
	if (iEndpoint[0] == NULL)
		{
	    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETINTERFACE_DUP6,
	            "SetInterface 11" );
		r = SetupEp0();
		if (r != KErrNone)
			{
		    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DLDDUSBCCHANNEL_SETINTERFACE_DUP7,
		            "  Error: SetupEp0() failed" );
			DestroyEp0();
			goto F1;
			}
		}

	alternateSettingListRec = new TUsbcAlternateSettingList;
	if (!alternateSettingListRec)
		{
		r = KErrNoMemory;
		goto F1;
		}

    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETINTERFACE_DUP8,
            "DLddUsbcChannel::SetInterface num_endpoints=%d", num_endpoints );

	// other endpoints
	// calculate the total buffer size
	for (TInt i = 1; i <= num_endpoints; i++, pEndpointData++)
		{
	    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETINTERFACE_DUP9,
	            "SetInterface for ep=%d", i );
		if (!ValidateEndpoint(pEndpointData))
			{
			r = KErrUsbBadEndpoint;
			goto F2;
			}

		TUsbcEndpoint* ep = new TUsbcEndpoint(this, iController, pEndpointData, i, bandwidthPriority);
		alternateSettingListRec->iEndpoint[i] = ep;
		if (!ep)
			{
			r = KErrNoMemory;
			goto F2;
			}
		if (ep->Construct() != KErrNone)
			{
			r = KErrNoMemory;
			goto F2;
			}

	    OstTraceDefExt3( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETINTERFACE_DUP10,
	            "SetInterface for ep=%d rec=0x%08x ep==0x%08x", i, 
	            reinterpret_cast<TUint>(alternateSettingListRec), reinterpret_cast<TUint>(ep) );
		}

	// buf size of each endpoint
	TInt bufSizes[KMaxEndpointsPerClient + 1];
	TInt epNum[KMaxEndpointsPerClient + 1];

    // init
    for( TInt i=0;i<KMaxEndpointsPerClient+1;i++ )
        {
        bufSizes[i] = -1;
        epNum[i] = i;
        }

	// Record the actual buf size of each endpoint
	for( TInt i=1;i<=num_endpoints;i++ )
	    {
	    bufSizes[i] = alternateSettingListRec->iEndpoint[i]->BufferSize();
	    }

    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETINTERFACE_DUP11,
            "Sort the endpoints:" );

    // sort the endpoint number by the bufsize decreasely
	for( TInt i=1;i<num_endpoints;i++ )
	    {
	    TInt epMaxBuf = i;
	    for(TInt k=i+1;k<=num_endpoints;k++ )
	        {
	        if( bufSizes[epMaxBuf]<bufSizes[k])
	            {
	            epMaxBuf = k;
	            }
	        }
	    TInt temp = bufSizes[i];
	    bufSizes[i] = bufSizes[epMaxBuf];
	    bufSizes[epMaxBuf] = temp;

	    temp = epNum[i];
        epNum[i] = epNum[epMaxBuf];
        epNum[epMaxBuf] = temp;

	    alternateSettingListRec->iEpNumDeOrderedByBufSize[i] = epNum[i];

	    OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETINTERFACE_DUP12,
	            " %d:%d", epNum[i], bufSizes[i] );
	    }
    alternateSettingListRec->iEpNumDeOrderedByBufSize[num_endpoints] = epNum[num_endpoints];
    OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETINTERFACE_DUP13,
            " %d:%d", epNum[num_endpoints], bufSizes[num_endpoints] );
    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETINTERFACE_DUP14, "\n" );

	// chain in this alternate setting
	alternateSettingListRec->iNext = iAlternateSettingList;
	iAlternateSettingList = alternateSettingListRec;
	alternateSettingListRec->iSetting = aInterfaceNumber;
	alternateSettingListRec->iNumberOfEndpoints = num_endpoints;

	// Record the 'real' endpoint number used by the PDD in both the Ep and
	// the Req callback:
	for (TInt i = 1; i <= num_endpoints; i++)
		{
		alternateSettingListRec->iEndpoint[i]->SetRealEpNumber(real_ep_numbers[i]);
		}

	r = SetupInterfaceMemory(iHwChunks, cacheAttribs );
	if( r==KErrNone )
	    {
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETINTERFACE_DUP15,
                "SetInterface ready to exit" );
    
        if (aInterfaceNumber == 0)
            {
            // make sure we're ready to go with the main interface
            iValidInterface = ETrue;
            OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETINTERFACE_DUP16,
                    "SetInterface SelectAlternateSetting" );
            SelectAlternateSetting(0);
            }
        return KErrNone;
	    }
	else
	    {
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETINTERFACE_DUP17,
                "Destroying all interfaces" );
        DestroyAllInterfaces();
        DestroyEp0();
        return r;
	    }

 F2:
	delete alternateSettingListRec;
	//Fall through
 
 F1:
#if _DEBUG
#ifdef OST_TRACE_COMPILER_IN_USE
	TInt r1 = iController->ReleaseInterface(this, aInterfaceNumber);
#else
	(void)  iController->ReleaseInterface(this, aInterfaceNumber);
#endif
    OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, DLDDUSBCCHANNEL_SETINTERFACE_DUP18,
            "Release Interface controller returned %d", r1 );
#else
	(void)	iController->ReleaseInterface(this, aInterfaceNumber);
#endif
	return r;
	}

// realloc the memory, and set the previous interfaces 
TInt DLddUsbcChannel::SetupInterfaceMemory(RArray<DPlatChunkHw*> &aHwChunks, 
        TUint32 aCacheAttribs )
    {
    TUsbcAlternateSettingList* asRec = iAlternateSettingList;

    // if buffers has been changed
    TBool chunkChanged = EFalse;
    TInt numOfEp = asRec->iNumberOfEndpoints;
 
    // 1, collect all bufs' sizes for the current interface
    //    to realloc all the chunks
    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETUPINTERFACEMEMORY, 
            "Collect all buffer sizes:" );
    RArray<TInt> bufSizes;
    for(TInt i=1;i<=numOfEp;i++)
        {
        TInt nextEp = asRec->iEpNumDeOrderedByBufSize[i];
        TInt epBufCount = asRec->iEndpoint[nextEp]->BufferNumber();
        OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETUPINTERFACEMEMORY_DUP1,
                " ep %d, buf count %d", nextEp, epBufCount );
        for(TInt k=0;k<epBufCount;k++)
            {
            TInt epBufSize = asRec->iEndpoint[nextEp]->BufferSize();
            TInt r = bufSizes.Append(epBufSize);
            if(r!=KErrNone)
                {
                iController->DeRegisterClient(this);
                bufSizes.Close();
                return r;
                }
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETUPINTERFACEMEMORY_DUP2,
                    " %d", epBufSize );
            }
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETUPINTERFACEMEMORY_DUP3, "\n" );

        }
   
    // 2, alloc the buffer decreasely, biggest-->smallest
    //   2.1 check the existing chunks
    TInt bufCount = bufSizes.Count();
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETUPINTERFACEMEMORY_DUP4,
            " ep buf number needed %d", bufCount );
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETUPINTERFACEMEMORY_DUP5,
            " chunks available %d", aHwChunks.Count() );

    TInt chunkInd = 0;
    while( (chunkInd<aHwChunks.Count())&& (chunkInd<bufCount))
        {
        TUint8* oldAddr = NULL;
        oldAddr = reinterpret_cast<TUint8*>(aHwChunks[chunkInd]->LinearAddress());

        DPlatChunkHw* chunk = ReAllocate(bufSizes[chunkInd], aHwChunks[chunkInd], aCacheAttribs);
        if (chunk == NULL)
            {
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETUPINTERFACEMEMORY_DUP6,
                    "Failed to alloc chunks size %d!", bufSizes[chunkInd] );
            // lost all interfaces:
            // Tell Controller to release Interface and h/w resources associated with this
            iController->DeRegisterClient(this);
            bufSizes.Close();
            return KErrNoMemory;
            }
        else
            {
            // Parcel out the memory between endpoints
            TUint8* newAddr = reinterpret_cast<TUint8*>(chunk->LinearAddress());
            OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETUPINTERFACEMEMORY_DUP7,
                    "SetupInterfaceMemory alloc new chunk=0x%x, size=%d", 
                    reinterpret_cast<TUint>(newAddr), bufSizes[chunkInd] );
            // The check is important to avoid chunkChanged to be corrupted.
            // This code change is to fix the problem that one chunk is used by multiple interfaces.
            if(!chunkChanged)
            	{
            	chunkChanged = (newAddr != oldAddr);
            	}            
            aHwChunks[chunkInd] = chunk;
            }
        chunkInd++;
        }
    
    //   2.2 in case available chunks are not enough
    while( chunkInd<bufCount)
        {
        DPlatChunkHw* chunk = NULL;
        chunk = Allocate( bufSizes[chunkInd], aCacheAttribs);
        if (chunk == NULL)
            {
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETUPINTERFACEMEMORY_DUP8,
                    "Failed to alloc chunk, size %d!", bufSizes[chunkInd] );
            // lost all interfaces:
            // Tell Controller to release Interface and h/w resources associated with this
            iController->DeRegisterClient(this);
            bufSizes.Close();
            return KErrNoMemory;
            }
        else
            {
            // Parcel out the memory between endpoints
            OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETUPINTERFACEMEMORY_DUP9,
                    "SetupInterfaceMemory alloc new chunk=0x%x, size=%d",
                    static_cast<TUint>(chunk->LinearAddress()), bufSizes[chunkInd]);

            TInt r = aHwChunks.Append(chunk);
            if(r!=KErrNone)
                {
                ClosePhysicalChunk(chunk);
                iController->DeRegisterClient(this);
                bufSizes.Close();
                return r;
                }
            }
        chunkInd++;
        }

    // 3, Set the the bufs of the interfaces
    
    ReSetInterfaceMemory(asRec, aHwChunks);

    if(chunkChanged)
        {
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETUPINTERFACEMEMORY_DUP10,
                "SetupInterfaceMemory readdressing." );
        asRec = asRec->iNext;
        while (asRec)
            {
            // Interfaces are not concurrent so they can all start at the same logical address
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETUPINTERFACEMEMORY_DUP11,
                    "SetupInterfaceMemory readdressing setting=%d", asRec->iSetting );
            ReSetInterfaceMemory(asRec, aHwChunks);
            asRec = asRec->iNext;
            }
        }
    return KErrNone;
    }

TInt DLddUsbcChannel::SetupEp0()
	{
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETUPEP0, 
	        "SetupEp0 entry %x", this );
	TInt ep0Size = iController->Ep0PacketSize();
	TUsbcEndpointInfo ep0Info = TUsbcEndpointInfo(KUsbEpTypeControl, KUsbEpDirBidirect, ep0Size);
	TUsbcEndpoint* ep0 = new TUsbcEndpoint(this, iController, &ep0Info, 0, 0);
	if (ep0 == NULL)
		{
		return KErrNoMemory;
		}
	// In case we have to return early:
	iEndpoint[0] = ep0;
	TInt r = ep0->Construct();
	if (r != KErrNone)
		{
		return KErrNoMemory;
		}

    TInt bufferNum = ep0->BufferNumber();
    TInt bufferSize = ep0->BufferSize();
    TUint32 cacheAttribs = EMapAttrSupRw | EMapAttrCachedMax;

    for(TInt i=0;i<bufferNum;i++)
        {
        DPlatChunkHw* chunk = Allocate(bufferSize, cacheAttribs );
        if(chunk==NULL)
            {
            return KErrNoMemory;
            }
        TInt r = iHwChunksEp0.Append(chunk);
        if(r!=KErrNone)
            {
            ClosePhysicalChunk(chunk);
            return r;
            }
        TUint8 * buf;
        buf = (TUint8*) chunk->LinearAddress();
        ep0->SetBufferAddr( i, buf);
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETUPEP0_DUP1,
                "SetupEp0 60 buffer number %d", i );
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_SETUPEP0_DUP2,
                "SetupEp0 60 buffer size %d", bufferSize );
        }

    ep0->SetRealEpNumber(0);
	return KErrNone;
	}

// Set buffer address of the interface
// Precondition: Enough chunks available.
void DLddUsbcChannel::ReSetInterfaceMemory(TUsbcAlternateSettingList* aAlternateSettingListRec,
        RArray<DPlatChunkHw*> &aHwChunks)
    {
    TUsbcAlternateSettingList* asRec = aAlternateSettingListRec;

    // set all the interfaces
    TInt chunkInd = 0;
    TInt numOfEp = asRec->iNumberOfEndpoints;

    for (TInt i = 1; i <= numOfEp; i++)
        {
        TInt nextEp = asRec->iEpNumDeOrderedByBufSize[i];
        TInt epBufCount = asRec->iEndpoint[nextEp]->BufferNumber();
        for(TInt k=0;k<epBufCount;k++)
            {
            TUsbcEndpoint* ep = asRec->iEndpoint[nextEp];
            if (ep != NULL )
                {
                TUint8* pBuf = NULL;
                pBuf = reinterpret_cast<TUint8*>(aHwChunks[chunkInd]->LinearAddress());
                ep->SetBufferAddr( k, pBuf);
                OstTraceDefExt3( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_RESETINTERFACEMEMORY,
                        "  ep %d, buf %d, addr 0x%x", nextEp, k, reinterpret_cast<TUint>(pBuf) );
                chunkInd++;
#ifdef _DEBUG
                if (chunkInd > aHwChunks.Count())
                    {
                    OstTraceDefExt3(OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, DLDDUSBCCHANNEL_RESETINTERFACEMEMORY_DUP1,
                            "  Error: available chunks %d, run out at epInd%d, bufInd%d",
                            aHwChunks.Count(), i, k );
                    }
#endif
                __ASSERT_DEBUG(chunkInd<=aHwChunks.Count(),
                                   Kern::Fault("usbc.ldd", __LINE__));
                }
            }
        }

    }

void DLddUsbcChannel::DestroyAllInterfaces()
	{
	// Removes all interfaces
	TUsbcAlternateSettingList* alternateSettingListRec = iAlternateSettingList;
	while (alternateSettingListRec)
		{
		iController->ReleaseInterface(this, alternateSettingListRec->iSetting);
		TUsbcAlternateSettingList* alternateSettingListRecNext = alternateSettingListRec->iNext;
		delete alternateSettingListRec;
		alternateSettingListRec = alternateSettingListRecNext;
		}
	iNumberOfEndpoints = 0;
	iAlternateSettingList = NULL;

    for(TInt i=0;i<iHwChunks.Count();i++)
        {
        ClosePhysicalChunk( iHwChunks[i]);
        }
	iHwChunks.Close();

	iValidInterface = EFalse;
	}


void DLddUsbcChannel::DestroyInterface(TUint aInterfaceNumber)
	{
	if (iAlternateSetting == aInterfaceNumber)
		{
		ResetInterface(KErrUsbInterfaceNotReady);
		iValidInterface = EFalse;
		iNumberOfEndpoints = 0;
		for (TInt i = 1; i <= KMaxEndpointsPerClient; i++)
			{
			iEndpoint[i] = NULL;
			}
		}
	TUsbcAlternateSettingList* alternateSettingListRec = iAlternateSettingList;
	TUsbcAlternateSettingList* alternateSettingListRecOld = NULL;
	while (alternateSettingListRec)
		{
		TUsbcAlternateSettingList* alternateSettingListRecNext = alternateSettingListRec->iNext;
		if (alternateSettingListRec->iSetting == aInterfaceNumber)
			{
			// This record is to be deleted
			if (alternateSettingListRecOld == NULL)
				{
				// The record to be deleted is at the list head
				iAlternateSettingList = alternateSettingListRecNext;
				}
			else
				{
				// The record to be deleted is NOT at the list head
				alternateSettingListRecOld->iNext = alternateSettingListRecNext;
				}
			delete alternateSettingListRec;
			break;
			}
		alternateSettingListRecOld = alternateSettingListRec;
		alternateSettingListRec = alternateSettingListRecNext;
		}

	if (iAlternateSettingList == NULL)
		{
		// if no interfaces left destroy non-ep0 buffering
		for(TInt i=0;i<iHwChunks.Count();i++)
	        {
	        ClosePhysicalChunk( iHwChunks[i]);
	        }
	    iHwChunks.Close();
		}
	}


void DLddUsbcChannel::DestroyEp0()
	{
	delete iEndpoint[0];
	iEndpoint[0] = NULL;
	for(TInt i=0;i<iHwChunksEp0.Count();i++)
	    {
	    ClosePhysicalChunk( iHwChunksEp0[i] );
	    }
	iHwChunksEp0.Close();
	}


void DLddUsbcChannel::EndpointStatusChangeCallback(TAny* aDLddUsbcChannel)
    {
    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_ENDPOINTSTATUSCHANGECALLBACK,
            "EndpointStatusChangeCallback" );
    DLddUsbcChannel* dUsbc = (DLddUsbcChannel*) aDLddUsbcChannel;
	if (dUsbc->iChannelClosing)
		return;
	TUint endpointState = dUsbc->iEndpointStatusCallbackInfo.State();
	const TInt reqNo = (TInt) RDevUsbcClient::ERequestEndpointStatusNotify;
	if (dUsbc->iRequestStatus[reqNo])
		{
	    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_ENDPOINTSTATUSCHANGECALLBACK_DUP1,
	            "EndpointStatusChangeCallback Notify status" );
		DThread* client = dUsbc->iClient;
		
		dUsbc->iEndpointStatusChangeReq->Data() = endpointState;
		dUsbc->iRequestStatus[reqNo] = NULL;
		Kern::QueueRequestComplete(client,dUsbc->iEndpointStatusChangeReq,KErrNone);
		dUsbc->iEndpointStatusChangePtr = NULL;
		}
	}


void DLddUsbcChannel::StatusChangeCallback(TAny* aDLddUsbcChannel)
	{
    DLddUsbcChannel* dUsbc = (DLddUsbcChannel*) aDLddUsbcChannel;
	if (dUsbc->iChannelClosing)
		return;

    TUsbcDeviceState deviceState;
    TInt i;
 	for (i = 0;
 		 (i < KUsbcDeviceStateRequests) && ((deviceState = dUsbc->iStatusCallbackInfo.State(i)) != EUsbcNoState);
 		 ++i)
		{
		OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_STATUSCHANGECALLBACK, 
		        "StatusChangeCallBack status=%d", static_cast<TInt>(deviceState) );
		if (deviceState & KUsbAlternateSetting)
			{
			dUsbc->ProcessAlternateSetting(deviceState);
			}
		else
			{
			dUsbc->ProcessDeviceState(deviceState);
			}
		// Only queue if userside is interested
		if (dUsbc->iDeviceStatusNeeded)
			{
			dUsbc->iStatusFifo->AddStatusToQueue(deviceState);
			const TInt reqNo = (TInt) RDevUsbcClient::ERequestAlternateDeviceStatusNotify;
			if (dUsbc->AlternateDeviceStateTestComplete())
				{
					dUsbc->iRequestStatus[reqNo]=NULL;
					Kern::QueueRequestComplete(dUsbc->iClient,dUsbc->iStatusChangeReq,KErrNone);
				}
			}
		}
 	// We don't want to be interrupted in the middle of this:
	const TInt irqs = NKern::DisableInterrupts(2);
 	dUsbc->iStatusCallbackInfo.ResetState();
	NKern::RestoreInterrupts(irqs);
	}


void DLddUsbcChannel::OtgFeatureChangeCallback(TAny* aDLddUsbcChannel)
    {
    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_OTGFEATURECHANGECALLBACK,
            "OtgFeatureChangeCallback" );
    DLddUsbcChannel* dUsbc = (DLddUsbcChannel*) aDLddUsbcChannel;
	if (dUsbc->iChannelClosing)
		return;

    TUint8 features;
    // No return value check. Assume OTG always supported here
    dUsbc->iController->GetCurrentOtgFeatures(features);

    const TInt reqNo = (TInt) RDevUsbcClient::ERequestOtgFeaturesNotify;
	if (dUsbc->iRequestStatus[reqNo])
		{
	    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_OTGFEATURECHANGECALLBACK_DUP1,
	            "OtgFeatureChangeCallback Notify status" );
		dUsbc->iOtgFeatureChangeReq->Data()=features;
		dUsbc->iRequestStatus[reqNo] = NULL;
		Kern::QueueRequestComplete(dUsbc->iClient,dUsbc->iOtgFeatureChangeReq,KErrNone);
		dUsbc->iOtgFeatureChangePtr = NULL;
		}
    }


TInt DLddUsbcChannel::SelectAlternateSetting(TUint aAlternateSetting)
	{
	TInt r = KErrGeneral;									// error code doesn't go userside
	TUsbcAlternateSettingList* alternateSettingListRec = iAlternateSettingList;
	while (alternateSettingListRec)
		{
		if (alternateSettingListRec->iSetting == aAlternateSetting)
			{
			// found the correct interface, now latch in new endpoint set
			for (TInt i = 1; i <= KMaxEndpointsPerClient; i++)
				{
				iEndpoint[i] = NULL;
				}
			iNumberOfEndpoints = alternateSettingListRec->iNumberOfEndpoints;
			r = KErrNone;
			for (TInt i = 1; i <= KMaxEndpointsPerClient; i++)
				{
				iEndpoint[i] = alternateSettingListRec->iEndpoint[i];
				}
			// Only after correct alternate setting has been chosen.
			UpdateEndpointSizes();
			}
		alternateSettingListRec = alternateSettingListRec->iNext;
		}
	return r;
	}


TInt DLddUsbcChannel::EpFromAlternateSetting(TUint aAlternateSetting, TInt aEndpoint)
	{
	TUsbcAlternateSettingList* alternateSettingListRec = iAlternateSettingList;
	while (alternateSettingListRec)
		{
		if (alternateSettingListRec->iSetting == aAlternateSetting)
			{
			if ((aEndpoint <= alternateSettingListRec->iNumberOfEndpoints) &&
				(aEndpoint >= 0))
				{
				return alternateSettingListRec->iEndpoint[aEndpoint]->RealEpNumber();
				}
			else
				{
				OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DLDDUSBCCHANNEL_EPFROMALTERNATESETTING,
				        "  Error: aEndpoint %d wrong for aAlternateSetting %d", aEndpoint, aAlternateSetting );
				return -1;
				}
			}
		alternateSettingListRec = alternateSettingListRec->iNext;
		}
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DLDDUSBCCHANNEL_EPFROMALTERNATESETTING_DUP1,
            "  Error: no aAlternateSetting %d found", aAlternateSetting );
	return -1;
	}


TInt DLddUsbcChannel::ProcessAlternateSetting(TUint aAlternateSetting)
	{
	ResetInterface(KErrUsbInterfaceChange);					// kill any outstanding transfers
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_PROCESSALTERNATESETTING,
	        "ProcessAlternateSetting 0x%08x", aAlternateSetting );
	TUint newSetting = aAlternateSetting&(~KUsbAlternateSetting);
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_PROCESSALTERNATESETTING_DUP1,
            "ProcessAlternateSetting selecting alternate setting 0x%08x", newSetting );

	TInt r = SelectAlternateSetting(newSetting);
	if (r != KErrNone)
		return r;
	StartEpReads();
	iAlternateSetting = newSetting;
    return KErrNone;
	}


TInt DLddUsbcChannel::ProcessDeviceState(TUsbcDeviceState aDeviceState)
	{
	OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_PROCESSDEVICESTATE,
	        "ProcessDeviceState(%d -> %d)", iDeviceState, aDeviceState );
	if (iDeviceState == aDeviceState)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_PROCESSDEVICESTATE_DUP1,
                "  No state change => nothing to be done." );
		return KErrNone;
		}
	if (iDeviceState == EUsbcDeviceStateSuspended)
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_PROCESSDEVICESTATE_DUP2,
                "  Coming out of Suspend: old state = %d", iOldDeviceState );
		iDeviceState = iOldDeviceState;
		if (iDeviceState == aDeviceState)
			{
	        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_PROCESSDEVICESTATE_DUP3,
	                "  New state same as before Suspend => nothing to be done." );
			return KErrNone;
			}
		}
	TBool renumerateState = (aDeviceState == EUsbcDeviceStateConfigured);
	TBool deconfigured = EFalse;
	TInt cancellationCode = KErrNone;
	if (aDeviceState == EUsbcDeviceStateSuspended)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_PROCESSDEVICESTATE_DUP4,
                "  Suspending..." );
		iOldDeviceState = iDeviceState;
		// Put PSL into low power mode here
		}
	else
		{
		deconfigured = (iDeviceState == EUsbcDeviceStateConfigured &&
						aDeviceState != EUsbcDeviceStateConfigured);
		if (iDeviceState == EUsbcDeviceStateConfigured)
			{
			if (aDeviceState == EUsbcDeviceStateUndefined)
				cancellationCode = KErrUsbCableDetached;
			else if (aDeviceState == EUsbcDeviceStateAddress)
				cancellationCode = KErrUsbDeviceNotConfigured;
			else if (aDeviceState == EUsbcDeviceStateDefault)
				cancellationCode = KErrUsbDeviceBusReset;
			else
				cancellationCode = KErrUsbDeviceNotConfigured;
			}
		}
    OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_PROCESSDEVICESTATE_DUP5,
            "  %d --> %d", iDeviceState, aDeviceState );
	iDeviceState = aDeviceState;
	if (iValidInterface || iOwnsDeviceControl)
		{
		// This LDD may not own an interface. It could be some manager reenumerating
		// after its subordinate LDDs have setup their interfaces.
		if (deconfigured)
			{
		    DeConfigure(cancellationCode);
			}
		else if (renumerateState)
			{
			// Update size of Ep0.
			iEndpoint[0]->SetMaxPacketSize(iController->Ep0PacketSize());
			// First cancel transfers on all endpoints
			ResetInterface(KErrUsbInterfaceChange);
			// Select main interface & latch in new endpoint set
			SelectAlternateSetting(0);
			// Here we go
			StartEpReads();
			}
		}

	const TInt reqNo = (TInt) RDevUsbcClient::ERequestReEnumerate;
	if (renumerateState && iRequestStatus[reqNo])
		{
		// This lot must be done if we are reenumerated
		CompleteBufferRequest(iClient, reqNo, KErrNone);
		}

    return KErrNone;
    }


void DLddUsbcChannel::UpdateEndpointSizes()
	{
	// The regular ones.
	TInt i = 0;
	while ((++i <= KMaxEndpointsPerClient) && iEndpoint[i])
		{
		const TInt size = iController->EndpointPacketSize(this, iEndpoint[i]->RealEpNumber());
		if (size < 0)
			{
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DLDDUSBCCHANNEL_UPDATEENDPOINTSIZES,
			        "  Error: Packet size < 0 for ep %d", i );
			continue;
			}
		iEndpoint[i]->SetMaxPacketSize(size);
		}
#ifdef _DEBUG
	if (i != iNumberOfEndpoints + 1)
	    {
        OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_FATAL, DLDDUSBCCHANNEL_UPDATEENDPOINTSIZES_DUP1,
                "  Error: iNumberOfEndpoints wrong (%d)", iNumberOfEndpoints );

	    }
#endif
	}


DPlatChunkHw* DLddUsbcChannel::ReAllocate(TInt aBuffersize, DPlatChunkHw* aHwChunk,TUint32 aCacheAttribs)
	{
	DPlatChunkHw* chunk = aHwChunk;
	if ((!chunk) || (chunk->iSize < aBuffersize))
		{
		if (chunk)
			{
			ClosePhysicalChunk(chunk);
			}
		OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_REALLOCATE, 
		        "ReAllocate need to get new chunk" );
		chunk = Allocate(aBuffersize, aCacheAttribs);
		}
	return chunk;
	}


DPlatChunkHw* DLddUsbcChannel::Allocate(TInt aBuffersize, TUint32 aCacheAttribs)
	{
	TUint32 physAddr = 0;
	TUint32 size = Kern::RoundToPageSize(aBuffersize);

	if (Epoc::AllocPhysicalRam(size, physAddr) != KErrNone)
		return NULL;

	DPlatChunkHw* HwChunk;
	if (DPlatChunkHw::New(HwChunk, physAddr, aBuffersize, aCacheAttribs) != KErrNone)
		{
		Epoc::FreePhysicalRam(physAddr, size);
		return NULL;
		}

	return HwChunk;
	}


TInt DLddUsbcChannel::DoRxComplete(TUsbcEndpoint* aTUsbcEndpoint, TInt aEndpoint, TBool aReEntrant)
	{
	TBool completeNow;
	TInt err = aTUsbcEndpoint->CopyToClient(iClient, completeNow,iClientAsynchNotify[aEndpoint]->iClientBuffer);
	if (completeNow)
		{
		aTUsbcEndpoint->SetClientReadPending(EFalse);
		CompleteBufferRequest(iClient, aEndpoint, err);
		}
	aTUsbcEndpoint->TryToStartRead(aReEntrant);
	return err;
	}


void DLddUsbcChannel::DoRxCompleteNow(TUsbcEndpoint* aTUsbcEndpoint, TInt aEndpoint)
	{
	aTUsbcEndpoint->SetClientReadPending(EFalse);
	CompleteBufferRequest(iClient, aEndpoint, KErrCancel);
	}


void DLddUsbcChannel::DoTxComplete(TUsbcEndpoint* aTUsbcEndpoint, TInt aEndpoint, TInt aError)
	{
	aTUsbcEndpoint->SetClientWritePending(EFalse);
	CompleteBufferRequest(iClient, aEndpoint, aError);
	}


TBool DLddUsbcChannel::AlternateDeviceStateTestComplete()
	{
	TBool completeNow = EFalse;
	const TInt reqNo = (TInt) RDevUsbcClient::ERequestAlternateDeviceStatusNotify;
	if (iRequestStatus[reqNo])
		{
		// User req is outstanding
		TUint32 deviceState;
		if (iStatusFifo->GetDeviceQueuedStatus(deviceState) == KErrNone)
			{
			// Device state waiting to be sent userside
			completeNow = ETrue;
			OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_ALTERNATEDEVICESTATETESTCOMPLETE,
			        "StatusChangeCallback Notify status" );
			iStatusChangeReq->Data()=deviceState;
			iStatusChangePtr = NULL;
			}
		}
	return completeNow;
	}


void DLddUsbcChannel::EmergencyCompleteDfc(TAny* aDLddUsbcChannel)
	{
	((DLddUsbcChannel*) aDLddUsbcChannel)->DoEmergencyComplete();
	}


void DLddUsbcChannel::DeConfigure(TInt aErrorCode)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DECONFIGURE,
	        "DLddUsbcChannel::DeConfigure()" );
	// Called after deconfiguration. Cancels transfers on all endpoints.
	ResetInterface(aErrorCode);
	// Cancel the endpoint status notify request if it is outstanding.
	const TInt KEpNotReq = RDevUsbcClient::ERequestEndpointStatusNotify;
	if (iRequestStatus[KEpNotReq])
		{
		CancelNotifyEndpointStatus();
		iRequestStatus[KEpNotReq]=NULL;
		Kern::QueueRequestComplete(iClient,iEndpointStatusChangeReq,aErrorCode);
		}
	// We have to reset the alternate setting number when the config goes away.
 	SelectAlternateSetting(0);
	iAlternateSetting = 0;
	}


void DLddUsbcChannel::StartEpReads()
	{
	// Queued after enumeration. Starts reads on all endpoints.
	// The endpoint itself decides if it can do a read
	TInt i;
	for (i = 0; i <= iNumberOfEndpoints; i++)
		{
		// The endpoint itself will decide if it can read
		iEndpoint[i]->TryToStartRead(EFalse);
		}
	}


void DLddUsbcChannel::ResetInterface(TInt aErrorCode)
	{
	// Called after change in alternate setting.  Cancels transfers on all endpoints
	if (iValidInterface || iOwnsDeviceControl)
		{
		// Reset each endpoint except ep0
		for (TInt i = 1; i <= iNumberOfEndpoints; i++)
			{
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_RESETINTERFACE,
			        "Cancelling transfer ep=%d", i );
			iEndpoint[i]->CancelTransfer(iClient,iClientAsynchNotify[i]->iClientBuffer);			// Copies data userside
			iEndpoint[i]->AbortTransfer();					// kills any ldd->pil outstanding transfers
			iEndpoint[i]->iDmaBuffers->Flush();
			if (iRequestStatus[i] != NULL)
				CompleteBufferRequest(iClient, i, aErrorCode);
			iEndpoint[i]->SetClientWritePending(EFalse);
			iEndpoint[i]->SetClientReadPending(EFalse);
			}
		}
	}


void DLddUsbcChannel::AbortInterface()
	{
	// Called after when channel is closing
	if (iValidInterface || iOwnsDeviceControl)
		{
		for (TInt i = 0; i <= iNumberOfEndpoints; i++)
			{
			if (iEndpoint[i])
				{
				// kills any LDD->PDD outstanding transfers
				iEndpoint[i]->AbortTransfer();
				}
			}
		}
	}


void DLddUsbcChannel::ClosePhysicalChunk(DPlatChunkHw*& aHwChunk)
	{
	if (aHwChunk)
		{
 		const TPhysAddr addr = aHwChunk->PhysicalAddress();
 		const TInt size = aHwChunk->iSize;
		aHwChunk->Close(NULL);
 		Epoc::FreePhysicalRam(addr, size);
		}
	aHwChunk = NULL;
	}


TInt DLddUsbcChannel::DoEmergencyComplete()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOEMERGENCYCOMPLETE,
	        "TUsbcEndpoint::DoEmergencyComplete" );
	// cancel any pending DFCs
	// complete all client requests
    for (TInt i = 0; i < KUsbcMaxRequests; i++)
        {
        if (iRequestStatus[i])
            {
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DLDDUSBCCHANNEL_DOEMERGENCYCOMPLETE_DUP1,
                    "Complete request 0x%x", iRequestStatus[i] );

            if (i == RDevUsbcClient::ERequestAlternateDeviceStatusNotify)
                {

                iDeviceStatusNeeded = EFalse;
                iStatusFifo->FlushQueue();

                if (iStatusChangePtr)
                    {
                    iStatusChangeReq->Data() = iController->GetDeviceStatus();
                    iStatusChangePtr = NULL;

                    if (iStatusChangeReq->IsReady())
                        {
                        iRequestStatus[i] = NULL;
                        Kern::QueueRequestComplete(iClient, iStatusChangeReq,
                                KErrDisconnected);
                        }
                    }

                }
            else if (i == RDevUsbcClient::ERequestEndpointStatusNotify)
                {
                	
               	if (iEndpointStatusChangePtr)
					{
	                TUint epBitmap = 0;
					for (TInt i = 0; i <= iNumberOfEndpoints; i++)
						{
						TInt v = iController->GetEndpointStatus(this, iEndpoint[i]->RealEpNumber());
						TUint b;
						(v == EEndpointStateStalled) ? b = 1 : b = 0;
						epBitmap |= b << i;
						}	

					iEndpointStatusChangeReq->Data() = epBitmap;
					iEndpointStatusChangePtr = NULL;
					}

                if (iEndpointStatusChangeReq->IsReady())
                    {
					iRequestStatus[i] = NULL;
					Kern::QueueRequestComplete(iClient,iEndpointStatusChangeReq,KErrDisconnected);
					}

                }
            else if (i == RDevUsbcClient::ERequestOtgFeaturesNotify)
                {
                	
                if (iOtgFeatureChangePtr)
			        {
			        TUint8 features;
			        iController->GetCurrentOtgFeatures(features);
					iOtgFeatureChangeReq->Data()=features;
			        iOtgFeatureChangePtr = NULL;
			        }
                	
                if (iOtgFeatureChangeReq->IsReady())
                    {
                    iRequestStatus[i] = NULL;
                    Kern::QueueRequestComplete(iClient, iOtgFeatureChangeReq,
                            KErrDisconnected);
                    }

                }
            else
            	{
				CompleteBufferRequest(iClient, i, KErrDisconnected);
				}

            }
        }

    iStatusCallbackInfo.Cancel();
    iEndpointStatusCallbackInfo.Cancel();
    iOtgFeatureCallbackInfo.Cancel();
	return KErrNone;
	}


void DLddUsbcChannel::PanicClientThread(TInt aReason)
	{
	Kern::ThreadKill(iClient, EExitPanic, aReason, KUsbLDDKillCat);
	}


// ===============Endpoint====================

// Constructor
TUsbcEndpoint::TUsbcEndpoint(DLddUsbcChannel* aLDD, DUsbClientController* aController,
							 const TUsbcEndpointInfo* aEndpointInfo, TInt aEndpointNum,
							 TInt aBandwidthPriority)
	: iController(aController),
	  iEndpointInfo(aEndpointInfo->iType, aEndpointInfo->iDir, aEndpointInfo->iSize),
	  iClientReadPending(EFalse),
	  iClientWritePending(EFalse),
	  iEndpointNumber(aEndpointNum),
	  iRealEpNumber(-1),
	  iLdd(aLDD),
	  iError(KErrNone),
	  iRequestCallbackInfo(NULL),
	  iBytesTransferred(0),
	  iBandwidthPriority(aBandwidthPriority)
	{
 	ResetTransferInfo();
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCENDPOINT_TUSBCENDPOINT_CONS, 
	        "TUsbcEndpoint::TUsbcEndpoint 2" );
	}


TInt TUsbcEndpoint::Construct()
	{
	iDmaBuffers = new TDmaBuf(&iEndpointInfo, iBandwidthPriority);
	if (iDmaBuffers == NULL)
		{
		return KErrNoMemory;
		}
	const TInt r = iDmaBuffers->Construct(&iEndpointInfo);
	if (r != KErrNone)
		{
		return r;
		}
	iRequestCallbackInfo = new TUsbcRequestCallback(iLdd,
													iEndpointNumber,
													TUsbcEndpoint::RequestCallback,
													this,
													iLdd->iDfcQ,
													KUsbRequestCallbackPriority);
	if (iRequestCallbackInfo == NULL)
		{
		return KErrNoMemory;
		}
	return KErrNone;
	}


TUsbcEndpoint::~TUsbcEndpoint()
	{
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCENDPOINT_TUSBCENDPOINT_DES, 
	        "TUsbcEndpoint::~TUsbcEndpoint(%d)", iEndpointNumber );
	AbortTransfer();
	delete iRequestCallbackInfo;
	delete iDmaBuffers;
	}


void TUsbcEndpoint::RequestCallback(TAny* aTUsbcEndpoint)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_REQUESTCALLBACK, 
	        "TUsbcEndpoint::RequestCallback" );
	((TUsbcEndpoint*) aTUsbcEndpoint)->EndpointComplete();
	}


void TUsbcEndpoint::SetMaxPacketSize(TInt aSize)
	{
	iEndpointInfo.iSize = aSize;
	iDmaBuffers->SetMaxPacketSize(aSize);
	}


TInt TUsbcEndpoint::EndpointComplete()
	{
    OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_ENDPOINTCOMPLETE, 
            "TUsbcEndpoint::EndpointComplete ep=%d %d", iEndpointNumber, iRequestCallbackInfo->iEndpointNum );

	if (iLdd->ChannelClosing())
		{
	    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_ENDPOINTCOMPLETE_DUP1,
	            "We're going home -> completions no longer accepted" );
		return KErrNone;
		}

	TTransferDirection transferDir = iRequestCallbackInfo->iTransferDir;
	TInt error = iRequestCallbackInfo->iError;

	switch (transferDir)
		{

	case EControllerWrite:
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_ENDPOINTCOMPLETE_DUP2,
                "TUsbcEndpoint::EndpointComplete Write 2" );
		if (!iDmaBuffers->TxIsActive())
			{
	        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_ENDPOINTCOMPLETE_DUP3,
	                "  TX completion but !iDmaBuffers->TxIsActive()" );
			break;
			}

		iDmaBuffers->TxSetInActive();
		TBool completeNow = EFalse;
		iBytesTransferred += iRequestCallbackInfo->iTxBytes;
		if (iClientWritePending)
			{
			//Complete Outstanding Write if necessary
			iError = error;
			if (iError != KErrNone)
				{
				completeNow = ETrue;
				if (iError == KErrPrematureEnd)				// Previous write could not be completed
					iError = KErrNone;
				}
			else
				{
				if (iBytesTransferred == (TUint32) iTransferInfo.iTransferSize)
					{
					completeNow = ETrue;
					}
				else
					{
					iError = ContinueWrite();
					if (iError != KErrNone)
						completeNow = ETrue;
					}
				}
			if (completeNow)
				{
				TxComplete();
				ResetTransferInfo();
				if (iEndpointNumber == 0)
					{
					iDmaBuffers->Flush();
					TryToStartRead(EFalse);
					}
				}
			}
		break;
		}

	case EControllerRead:
		{
		// The first packet always contains the total #of bytes
		const TInt byteCount = iRequestCallbackInfo->iPacketSize[0];
		const TInt packetCount = iRequestCallbackInfo->iRxPackets;
		iDmaBuffers->ReadXferComplete(byteCount, packetCount, error);

		// We queue the dfc if we can complete the read, i.e. if we are reading a packet,
		// or if we have enough data to satisfy a read data request.
		if (iClientReadPending)
			{
			//Complete outstanding read
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_ENDPOINTCOMPLETE_DUP4,
                    "TUsbcEndpoint::EndpointComplete Read 3 (bytes available=%d)", 
                    iDmaBuffers->RxBytesAvailable());

			TInt bytesReqd = iTransferInfo.iTransferSize - iBytesTransferred;
			TBool completeNow = EFalse;

			if (iTransferInfo.iTransferType == ETransferTypeReadPacket ||
				iTransferInfo.iTransferType == ETransferTypeReadOneOrMore)
				{
				// Always complete on a packet read
				completeNow = ETrue;
				}
			else if (iTransferInfo.iTransferType == ETransferTypeReadData)
				{
				// Complete only if enough data is present
				if (iDmaBuffers->RxBytesAvailable() >= bytesReqd)
					completeNow = ETrue;
				}
			else if (iTransferInfo.iTransferType == ETransferTypeReadUntilShort)
				{
				// Complete if enough data is present or if a short packet has been delivered
				const TInt maxPacketSize = iEndpointInfo.iSize;
				const TInt lastPacketSize = iRequestCallbackInfo->iPacketSize[packetCount - 1];
				if (lastPacketSize < maxPacketSize)
					completeNow = ETrue;
				else if (iDmaBuffers->RxBytesAvailable() >= bytesReqd)
					completeNow = ETrue;
				else
					{
					const TUint type = iEndpointInfo.iType;
					if ((type == KUsbEpTypeBulk) && (lastPacketSize & (maxPacketSize - 1)))
						{
						completeNow = ETrue;
						}
					else if ((type != KUsbEpTypeBulk) &&
							 (lastPacketSize > maxPacketSize) &&
							 (lastPacketSize % maxPacketSize))
						{
						completeNow = ETrue;
						}
					}
				}
			if (completeNow)
				{
				iError = error;
				RxComplete(EFalse);
				iClientReadPending = EFalse;
				}
			}
		iDmaBuffers->RxSetInActive();
		if (error != KErrNone)
			{
			return error;
			}
		if (TryToStartRead(EFalse) != KErrNone)
			{
//			if (iEndpointNumber != 0)
//				Kern::Printf("EndpointComplete couldn't start read on ep=%d", iEndpointNumber);
			}
		break;
		}

	default:
		// shouldn't get here
		break;
		}

	return KErrNone;
	}


void TUsbcEndpoint::TxComplete()
	{
	iLdd->DoTxComplete(this, iEndpointNumber, iError);
	}


TInt TUsbcEndpoint::RxComplete(TBool aReEntrant)
	{
	return iLdd->DoRxComplete(this, iEndpointNumber, aReEntrant);
	}


void TUsbcEndpoint::RxCompleteNow()
	{
	iLdd->DoRxCompleteNow(this, iEndpointNumber);
	}


TInt TUsbcEndpoint::CopyToClient(DThread* aClient, TClientBuffer *aTcb)
	{
	TBool completeNow;
	return CopyToClient(aClient, completeNow,aTcb);
	}


TInt TUsbcEndpoint::CopyToClient(DThread* aClient, TBool& aCompleteNow, TClientBuffer *aTcb)
	{
	TInt err;
	const TInt length = iTransferInfo.iTransferSize;
	const TBool KReadData = EFalse;
	const TBool KReadUntilShort = ETrue;

    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_COPYTOCLIENT, 
            "CopyToClient: length = %d", length );

	if (iTransferInfo.iTransferType == ETransferTypeReadPacket)
		{
		err = iDmaBuffers->RxCopyPacketToClient(aClient, aTcb, length);
		aCompleteNow = ETrue;
		}
	else if (iTransferInfo.iTransferType == ETransferTypeReadOneOrMore)
		{
		err = iDmaBuffers->RxCopyDataToClient(aClient, aTcb, length, iBytesTransferred,
											  KReadData, aCompleteNow);
		aCompleteNow = ETrue;
		}
	else if (iTransferInfo.iTransferType == ETransferTypeReadUntilShort)
		{
		err = iDmaBuffers->RxCopyDataToClient(aClient, aTcb, length, iBytesTransferred,
											  KReadUntilShort, aCompleteNow);
		}
	else
		{
		err = iDmaBuffers->RxCopyDataToClient(aClient, aTcb, length, iBytesTransferred,
											  KReadData, aCompleteNow);
		}

	if (aCompleteNow)
		{
		ResetTransferInfo();
		SetClientReadPending(EFalse);
		}

	return err;
	}


TInt TUsbcEndpoint::TryToStartRead(TBool aReEntrant)
	{
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_TRYTOSTARTREAD,
	        "TryToStartRead 1 ep=%d", iEndpointNumber );
	TInt r = KErrNone;
	if (iEndpointInfo.iDir != KUsbEpDirOut &&
		iEndpointInfo.iDir != KUsbEpDirBidirect)
		{
		// Verify ep direction
		  OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_TRYTOSTARTREAD_DUP1,
		          "TryToStartRead wrong direction ep=%d", iEndpointNumber );
		return KErrUsbEpBadDirection;
		}

	if (iEndpointNumber == 0)
		{
		// Can't issue an Ep0 read if reader or writer is active
		if (iDmaBuffers->TxIsActive())
			{
            OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_TRYTOSTARTREAD_DUP2,
                    "TryToStartRead ep0 Tx already active FATAL" );

			return KErrUsbEpNotReady;
			}
		if (iDmaBuffers->RxIsActive())
			{
            OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_TRYTOSTARTREAD_DUP3,
                    "TryToStartRead ep0 Rx already active non-FATAL" );
			}
		}

	if (!(iDmaBuffers->RxIsActive()))
		{
		TUint8* bufferAddr;
		TPhysAddr physAddr;
		TUsbcPacketArray* indexArray;
		TUsbcPacketArray* sizeArray;
		TInt length;
		r = iDmaBuffers->RxGetNextXfer(bufferAddr, indexArray, sizeArray, length, physAddr);
		if (r == KErrNone)
			{
			iDmaBuffers->RxSetActive();
			iRequestCallbackInfo->SetRxBufferInfo(bufferAddr, physAddr, indexArray, sizeArray, length);
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_TRYTOSTARTREAD_DUP4,
                    "TryToStartRead 2 bufferAddr=0x%08x", bufferAddr );

			r = iController->SetupReadBuffer(*iRequestCallbackInfo);
			if (r != KErrNone)
				{
				iDmaBuffers->RxSetInActive();
	            OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_TRYTOSTARTREAD_DUP5,
	                    "  Error: TryToStartRead controller rejects read" );
				}
			}
		else
			{
			if (iClientReadPending)
				{
				// Deadlock, try to resolve it by draining buffer into descriptor
				if (!aReEntrant)
					{
					RxComplete(ETrue);
					}
				else
					{
					// we are stuck, better complete userside otherwise the userside request will hang
					RxCompleteNow();
					}
				}
			}
		}
	return r;
	}


TInt TUsbcEndpoint::TryToStartWrite(TEndpointTransferInfo* pTfr)
	{
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_TRYTOSTARTWRITE,
	        "TryToStartWrite 1 ep=%d", iEndpointNumber );
	if (iEndpointInfo.iDir != KUsbEpDirIn &&
		iEndpointInfo.iDir != KUsbEpDirBidirect)
		{
		// Verify ep direction
		return KErrUsbEpBadDirection;
		}
	if (iEndpointNumber == 0)
		{
		// Can't issue an Ep0 write if unread data is available or writer is active
		if (iDmaBuffers->TxIsActive() || !iDmaBuffers->IsReaderEmpty())
			{
			return KErrUsbEpNotReady;
			}
		if (iDmaBuffers->RxIsActive())
			{
			// if a reader is active then cancel the read
			iDmaBuffers->RxSetInActive();
			iController->CancelReadBuffer(iLdd, iRealEpNumber);
			}
		}
	SetTransferInfo(pTfr);
	ContinueWrite();
	return KErrNone;
	}


TInt TUsbcEndpoint::ContinueWrite()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_CONTINUEWRITE, "ContinueWrite 2" );
	TUint8* bufferAddr;
	TPhysAddr physAddr;
	TInt bufferLength;
	TInt r = iDmaBuffers->TxGetNextXfer(bufferAddr, bufferLength, physAddr);
	if (r != KErrNone)											// probably already active
		return r;
    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_CONTINUEWRITE_DUP1,
            "ContinueWrite 3" );

	iDmaBuffers->TxSetActive();
	TBool zlpReqd = EFalse;
	TUint32 transferSize = iTransferInfo.iTransferSize;
	TInt length = Min(transferSize - iBytesTransferred, (TUint32) bufferLength);
	if (iBytesTransferred+length>=transferSize)
		{
		// only send a zlp if this is the last buffer of the transfer
		zlpReqd = iTransferInfo.iZlpReqd;
		}
	r = iDmaBuffers->TxStoreData(iLdd->Client(), iLdd->GetClientBuffer(iEndpointNumber), length, iBytesTransferred);
	if (r != KErrNone)
		return r;
	iDmaBuffers->TxSetActive();
	iRequestCallbackInfo->SetTxBufferInfo(bufferAddr, physAddr, length);
	iRequestCallbackInfo->iZlpReqd = zlpReqd;
#if 0
	for (TInt i = 0; i < iRequestCallbackInfo->iLength; i++)
		{
	    OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_CONTINUEWRITE_DUP2,
	            "Buffer[%d] = 0x%02x", i, iRequestCallbackInfo->iBufferStart[i] );

		}
#endif
	r = iController->SetupWriteBuffer(*iRequestCallbackInfo);
	return r;
	}


void TUsbcEndpoint::CancelTransfer(DThread* aThread, TClientBuffer *aTcb)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_CANCELTRANSFER, "CancelTransfer" );
	if (iDmaBuffers != NULL)
		{
		if (iClientWritePending)
			{
            OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_CANCELTRANSFER_DUP1,
                    "  (iClientWritePending)" );
			iClientWritePending = EFalse;
			iController->CancelWriteBuffer(iLdd, iRealEpNumber);
			iDmaBuffers->TxSetInActive();
			}
		if (iClientReadPending)
			{
            OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_CANCELTRANSFER_DUP2,
                    "  (iClientReadPending)" );
			iClientReadPending = EFalse;
			CopyToClient(aThread,aTcb);
			}
		}
	}


void TUsbcEndpoint::AbortTransfer()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_ABORTTRANSFER, "Abort Transfer" );
	if (iDmaBuffers != NULL)
		{
		if (iDmaBuffers->TxIsActive())
			{
            OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_ABORTTRANSFER_DUP1,
                    "  (iClientWritePending)" );
			iController->CancelWriteBuffer(iLdd, iRealEpNumber);
			iDmaBuffers->TxSetInActive();
			}
		if (iDmaBuffers->RxIsActive())
			{
            OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINT_ABORTTRANSFER_DUP2,
                    "  (iClientReadPending)" );
			iController->CancelReadBuffer(iLdd, iRealEpNumber);
			iDmaBuffers->RxSetInActive();
			}
		iRequestCallbackInfo->iDfc.Cancel();
		}
	}


TUsbcAlternateSettingList::TUsbcAlternateSettingList()
	: iNext(NULL),
	  iNumberOfEndpoints(0),
	  iSetting(0)
	{
	for (TInt i = 0; i <= KMaxEndpointsPerClient; i++)
		{
		iEpNumDeOrderedByBufSize[i] = -1;
		iEndpoint[i] = NULL;
		}
	}


TUsbcAlternateSettingList::~TUsbcAlternateSettingList()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCALTERNATESETTINGLIST_TUSBCALTERNATESETTINGLIST_DES,
	        "TUsbcAlternateSettingList::~TUsbcAlternateSettingList()" );
	for (TInt i = 0; i <= KMaxEndpointsPerClient; i++)
		{
		delete iEndpoint[i];
		}
	}


TUsbcDeviceStatusQueue::TUsbcDeviceStatusQueue()
	{
	FlushQueue();
	}


void TUsbcDeviceStatusQueue::FlushQueue()
	{
	for (TInt i = 0; i < KUsbDeviceStatusQueueDepth; i++)
		{
		iDeviceStatusQueue[i] = KUsbDeviceStatusNull;
		}
	iStatusQueueHead = 0;
	}


void TUsbcDeviceStatusQueue::AddStatusToQueue(TUint32 aDeviceStatus)
	{
	// Only add a new status if it is not a duplicate of the one at the head of the queue
	if (!(iStatusQueueHead != 0 &&
		  iDeviceStatusQueue[iStatusQueueHead - 1] == aDeviceStatus))
		{
		if (iStatusQueueHead == KUsbDeviceStatusQueueDepth)
			{
			// Discard item at tail of queue
			TUint32 status;
			GetDeviceQueuedStatus(status);
			}
		iDeviceStatusQueue[iStatusQueueHead] = aDeviceStatus;
		iStatusQueueHead++;
		}
	}


TInt TUsbcDeviceStatusQueue::GetDeviceQueuedStatus(TUint32& aDeviceStatus)
	{
	TInt r = KErrNone;
	if (iStatusQueueHead <= 0)
		{
		r = KErrGeneral;
		aDeviceStatus = KUsbDeviceStatusNull;
		}
	else
		{
		aDeviceStatus = iDeviceStatusQueue[0];
		for(TInt i = 1; i < KUsbDeviceStatusQueueDepth; i++)
			{
			TUint32 s = iDeviceStatusQueue[i];
			iDeviceStatusQueue[i - 1] = s;
			}
		iStatusQueueHead--;
		iDeviceStatusQueue[KUsbDeviceStatusQueueDepth - 1] = KUsbDeviceStatusNull;
		}
	return r;
	}

void TClientAsynchNotify::Reset()
{
	iBufferRequest->Reset();
	iClientBuffer=NULL;
}

//---
