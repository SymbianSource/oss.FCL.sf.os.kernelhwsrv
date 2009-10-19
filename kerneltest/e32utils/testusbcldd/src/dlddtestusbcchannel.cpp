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
// f32test\testusbcldd\src\dlddtestusbcchannel.cpp
// 
//

#include "usbcdesc.h"
#include "dtestusblogdev.h"
#include "testusbc.h"

extern TDynamicDfcQue* gDfcQ;

const TUsbcEndpointData DLddTestUsbcChannel::iEndpointData[] =
	{
		{{KUsbEpSize64,	(KUsbEpTypeControl | KUsbEpDirBidirect)}, EFalse},
		{{KUsbEpSize64,	(KUsbEpTypeBulk	   | KUsbEpDirOut)}, EFalse},
		{{KUsbEpSize64,	(KUsbEpTypeBulk	   | KUsbEpDirIn )}, EFalse},
		{{KUsbEpSize64,	(KUsbEpTypeBulk	   | KUsbEpDirOut)}, EFalse},
		{{KUsbEpSize64,	(KUsbEpTypeBulk	   | KUsbEpDirIn )}, EFalse},
		{{KUsbEpNotAvailable, KUsbEpNotAvailable}, EFalse}
	};

// The EKA1 base class needs a DLogicalDevice* for its constructor
DLddTestUsbcChannel::DLddTestUsbcChannel(RPointerArray<DTestUsbcEndpoint>& aEndpoints) :
	iDescriptors(NULL),
	iIfcSet(this, 0),
	iEndpoints(aEndpoints),
	iDeviceState(EUsbcDeviceStateConfigured)
	{
	iClient = &Kern::CurrentThread();
	iClient->Open();
	}

DLddTestUsbcChannel::~DLddTestUsbcChannel()
	{
	}
	
TInt DLddTestUsbcChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
    {
	TInt err = KErrNone;
    // Setup LDD for receiving client messages
    SetDfcQ(gDfcQ);
    iMsgQ.Receive();

	_LIT(KEmptyString, "");
	err = iDescriptors.Init(
							TUsbcDeviceDescriptor::New(0, 0, 0, 0, 0, 0, 0, 0),
							TUsbcConfigDescriptor::New(0, ETrue, ETrue, 0),
							TUsbcLangIdDescriptor::New(0),
							TUsbcStringDescriptor::New(KEmptyString),
							TUsbcStringDescriptor::New(KEmptyString),
							TUsbcStringDescriptor::New(KEmptyString),
							TUsbcStringDescriptor::New(KEmptyString)
							);
    
    return err;
    }

void DLddTestUsbcChannel::HandleMsg(TMessageBase* aMsg)
	{
	TThreadMessage& m = *(TThreadMessage*)aMsg;
	TInt id = m.iValue;
	if (id == (TInt)ECloseMsg)
		{
		m.Complete(KErrNone, EFalse);
		return;
		}
	else if (id == KMaxTInt)
		{
		// Cancel request.
		TInt mask = m.Int0();
		DTestUsbcEndpoint* pEndpoint = NULL;
		switch (mask)
			{
			case RDevUsbcClient::ERequestEp0Cancel:
				pEndpoint = iEndpoints[0];
				pEndpoint->DoCancel();
				break;
			case RDevUsbcClient::ERequestEp1Cancel:
				pEndpoint = iEndpoints[FindRealEndpoint(1)];
				pEndpoint->DoCancel();
				break;
			case RDevUsbcClient::ERequestEp2Cancel:
				pEndpoint = iEndpoints[FindRealEndpoint(2)];
				pEndpoint->DoCancel();
				break;
			case RDevUsbcClient::ERequestEp3Cancel:
				pEndpoint = iEndpoints[FindRealEndpoint(3)];
				pEndpoint->DoCancel();
				break;
			case RDevUsbcClient::ERequestEp4Cancel:
				pEndpoint = iEndpoints[FindRealEndpoint(4)];
				pEndpoint->DoCancel();
				break;
			case RDevUsbcClient::ERequestEp5Cancel:
				pEndpoint = iEndpoints[FindRealEndpoint(5)];
				pEndpoint->DoCancel();
				break;
			case RDevUsbcClient::ERequestAlternateDeviceStatusNotifyCancel:
				CancelAlternateDeviceStatusNotify();
				break;
			default:
				m.Complete(KErrNotSupported, ETrue);
				return;
			}
		m.Complete(KErrNone, ETrue);
		return;
		}

	if (id < 0)
		{
		// DoRequest
		TRequestStatus* pS = (TRequestStatus*)m.Ptr0();
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

TInt DLddTestUsbcChannel::DoCancel(TInt /*aReqNo*/)
	{
	return KErrNone;
	}

void DLddTestUsbcChannel::DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("DoRequest 0x%08x"),aReqNo));
		TInt r = KErrNone;
		//If request is ep number then do a transfer request.
		if(aReqNo > KUsbcMaxEpNumber)
			{
			if (aReqNo == RDevTestUsbcClient::ETestRequestNotifyEndpointStatus)
				{
				__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("ETestRequestNotifyEndpointStatus")));
				r = HostEndpointStatusNotify((TInt)a1, aStatus);
				}
			else if (aReqNo == RDevUsbcClient::ERequestAlternateDeviceStatusNotify)
				{
				__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("ERequestAlternateDeviceStatusNotify")));
				r = SetAlternateDeviceStatusNotify(aStatus, (TUint*)a1);
				}
			else if (aReqNo == RDevUsbcClient::ERequestReEnumerate)
				{
				__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("ERequestReEnumerate")));
				r = ReEnumerate((TRequestStatus*)a1);
				}
			else
				{
				Kern::RequestComplete(iClient, aStatus, KErrNotSupported);
				}
			}
		else
			{
			r = DoTransferAsyncReq(aReqNo, a1, a2, *aStatus);
			}
			
	if (r != KErrNone)
		{
		Kern::RequestComplete(iClient, aStatus, r);
		}			
	}

TInt DLddTestUsbcChannel::DoControl(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r=KErrNone;
	TDes8& a1Buf = *((TDes8*)a1);
	TDes8& a2Buf = *((TDes8*)a2);
	TPtrC8 pZeroDesc(NULL,0);


	switch (aFunction)
		{
	case RDevUsbcClient::EControlEndpointZeroRequestError:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlEndpointZeroRequestError")));
		r = KErrNone;
		break;
		}
	
	case RDevUsbcClient::EControlEndpointCaps:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlEndpointCaps")));
		r = __THREADWRITE(iClient, a1, pZeroDesc);
		if(r == KErrNone)
			{
			TBuf8<KMaxTUint8> aBuf;
			memclr(&aBuf, sizeof(aBuf));
        	TPtr8 endpointCfg((TUint8*)&aBuf,0,sizeof(aBuf));
		
			r = Kern::ThreadDesRead(iClient,a1,endpointCfg,0,0);
			if(r == KErrNone)
				{
				endpointCfg.Copy(reinterpret_cast<const TUint8*>(iEndpointData), 
								 Min(endpointCfg.MaxLength(), 
								 sizeof(TUsbcEndpointData[5])));
				r = Kern::ThreadDesWrite(iClient,a1,endpointCfg,0,KTruncateToMaxLength,iClient);
				}
			}
			
		if(r != KErrNone)
			{
			__THREADPANIC(iClient, r);
			}

		break;
		}
	
	case RDevUsbcClient::EControlDeviceCaps:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlDeviceCaps")));
		r = __THREADWRITE(iClient, a1, pZeroDesc);
		if(r!=KErrNone) 
			{
			__THREADPANIC(iClient, r);
			}
		TUsbDeviceCaps caps;
		caps().iTotalEndpoints = KMaxEndpointsPerClient;
		caps().iConnect = ETrue;
		caps().iSelfPowered = ETrue;
		caps().iRemoteWakeup = ETrue;
		TBuf8<KMaxTUint8> aBuf;
        memclr(&aBuf, sizeof(aBuf));
 		TPtr8 cfg((TUint8*)&aBuf,0,sizeof(aBuf));
 		r=Kern::ThreadDesRead(iClient,a1,cfg,0,0);
		cfg = caps.Left(Min(caps.Length(), cfg.MaxLength()));
		r=Kern::ThreadDesWrite(iClient,a1,cfg,0,KTruncateToMaxLength,iClient);
		break;
		}
	
	case RDevUsbcClient::EControlDeviceStatus:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlDeviceStatus")));
		r = __THREADRAWWRITE(iClient, a1, (TUint8*)&iDeviceState, (TInt)sizeof(iDeviceState));
		if(r != KErrNone)
			{
			__THREADPANIC(iClient, r);
			}
		break;
		}
		
	case RDevUsbcClient::EControlHaltEndpoint:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlHaltEndpoint")));
		r = HaltClearEndpoint(ETrue, (TInt)a1);
		break;
		}
	
	case RDevUsbcClient::EControlGetDeviceDescriptor:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlGetDeviceDescriptor")));
		r = __THREADWRITE(iClient, a1, pZeroDesc);
		if(r != KErrNone)
			{
			__THREADPANIC(iClient, r);
			}
		r = iDescriptors.GetDeviceDescriptorTC(iClient, a1Buf);
		break;
		}
	
	case RDevUsbcClient::EControlSetDeviceDescriptor:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlSetDeviceDescriptor")));
		r = iDescriptors.SetDeviceDescriptorTC(iClient, a1Buf);
		break;
		}
		
	case RDevUsbcClient::EControlGetDeviceDescriptorSize:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlGetDeviceDescriptorSize")));
		if(a1 != NULL)
			{
			const TPtrC8 size(reinterpret_cast<const TUint8*>(&KUsbDescSize_Device), sizeof(KUsbDescSize_Device));
			r = __THREADWRITE(iClient, a1, size);
			}
		else
			{
			r = KErrArgument;
			}
		break;
		}
	
	case RDevUsbcClient::EControlGetConfigurationDescriptor:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlGetConfigurationDescriptor")));
		r = __THREADWRITE(iClient, a1, pZeroDesc);		// set client descriptor length to zero
		if(r != KErrNone)
			{
			__THREADPANIC(iClient, r);
			}
		r = iDescriptors.GetConfigurationDescriptorTC(iClient, a1Buf);
		break;
		}
	
	case RDevUsbcClient::EControlSetConfigurationDescriptor:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlSetConfigurationDescriptor")));
		r = iDescriptors.SetConfigurationDescriptorTC(iClient, a1Buf);
		break;
		}
	
	case RDevUsbcClient::EControlGetConfigurationDescriptorSize:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlGetConfigurationDescriptorSize")));
		if(a1 != NULL)
			{
			const TPtrC8 size(reinterpret_cast<const TUint8*>(&KUsbDescSize_Config), sizeof(KUsbDescSize_Config));
			r = __THREADWRITE(iClient, a1, size);
			}
		else
			{
			r=KErrArgument;
			}
		break;
		}
	
	case RDevUsbcClient::EControlGetInterfaceDescriptor:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlGetInterfaceDescriptor")));
		r = iDescriptors.GetInterfaceDescriptorTC(iClient, a2Buf, 0, (TInt)a1);
		break;
		}
		
	case RDevUsbcClient::EControlSetInterfaceDescriptor:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlSetInterfaceDescriptor")));
		TBuf8<KUsbDescSize_Interface> new_ifc;
		r = __THREADWRITE(iClient, a2, new_ifc);
		if (r != KErrNone)
			{
			break;
			}
		r = iDescriptors.SetInterfaceDescriptor(new_ifc, 0, (TInt)a1);
		break;
		}
		
	case RDevUsbcClient::EControlGetInterfaceDescriptorSize:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlGetInterfaceDescriptorSize")));
		if (a2 != NULL)
			{
			const TPtrC8 size(reinterpret_cast<const TUint8*>(&KUsbDescSize_Interface), sizeof(KUsbDescSize_Interface));
			r = __THREADWRITE(iClient, a2, size);
			}
		else
			{
			r = KErrArgument;
			}
		break;
		}
		
	case RDevUsbcClient::EControlGetEndpointDescriptor:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlGetEndpointDescriptor")));
		TEndpointDescriptorInfo info;
		r = __THREADRAWREAD(iClient, a1,(TUint8*)&info, (TInt)sizeof(info));
		if(r != KErrNone)
			{
			__THREADPANIC(iClient, r);
			}
		r = iDescriptors.GetEndpointDescriptorTC(iClient, *((TDes8*)info.iArg), 0, info.iSetting, (TUint8)info.iEndpoint);
		}
	
	case RDevUsbcClient::EControlSetEndpointDescriptor:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlSetEndpointDescriptor")));
		TEndpointDescriptorInfo info;
		r = __THREADRAWREAD(iClient, a1, (TUint8*)&info, (TInt)sizeof(TEndpointDescriptorInfo));
		if(r != KErrNone)
			__THREADPANIC(iClient, r);
		r = iDescriptors.SetEndpointDescriptorTC(iClient, *((TDes8*)info.iArg), 0, info.iSetting, (TUint8)info.iEndpoint);
		break;
		}
	
	case RDevUsbcClient::EControlGetEndpointDescriptorSize:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlGetEndpointDescriptorSize")));
		TEndpointDescriptorInfo info;
		r = __THREADRAWREAD(iClient, a1, (TUint8*)&info, (TInt)sizeof(TEndpointDescriptorInfo));
		if(r != KErrNone)
			__THREADPANIC(iClient, r);
		TInt s;
		TInt r = iDescriptors.GetEndpointDescriptorSize(0, info.iSetting, (TUint8)info.iEndpoint, s);
		if (r == KErrNone)
			{
			TPtrC8 size(reinterpret_cast<const TUint8*>(&s), sizeof(s));
			r = __THREADWRITE(iClient, &(info.iArg), size);
			}
		break;
		}
		
	case RDevUsbcClient::EControlSetCSInterfaceDescriptor:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlSetCSInterfaceDescriptor")));
		TCSDescriptorInfo info;
		r = __THREADRAWREAD(iClient, a1, (TUint8*)&info, (TInt)sizeof(TCSDescriptorInfo));
		if(r != KErrNone)
			__THREADPANIC(iClient, r);
		r = iDescriptors.SetCSInterfaceDescriptorTC(iClient, *((TDes8*)info.iArg), 0, info.iSetting, info.iSize);
		}
		break;

	case RDevUsbcClient::EControlDeviceDisconnectFromHost:
		{
		r = KErrNone;
		break;
		}
		
	case RDevUsbcClient::EControlDeviceConnectToHost:
		{
		r = KErrNone;
		break;
		}
		
	case RDevUsbcClient::EControlDevicePowerUpUdc:
		{
		r = KErrNone;
		break;
		}
	
	case RDevUsbcClient::EControlSetInterface:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlSetInterface")));
		TUsbcIfcInfo info;
		r = __THREADRAWREAD(iClient, a2, (TUint8*)&info, (TInt)sizeof(TUsbcIfcInfo));
		if(r != KErrNone)
			__THREADPANIC(iClient, r);

		TUsbcInterfaceInfoBuf* interfaceData = info.iInterfaceData;
		TPtr8* ifcString = info.iString;
		r = SetInterface((TInt)a1, interfaceData, ifcString);
		}
		break;
		
	case RDevUsbcClient::EControlReleaseInterface:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlReleaseInterface")));
		r = ReleaseInterface((TInt)a1);
		break;
		}
	
	case RDevUsbcClient::EControlGetManufacturerStringDescriptor:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlGetManufacturerStringDescriptor")));
		r = iDescriptors.GetManufacturerStringDescriptorTC(iClient, a1Buf);
		break;
		}
		
	case RDevUsbcClient::EControlSetManufacturerStringDescriptor:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlSetManufacturerStringDescriptor")));
		r = iDescriptors.SetManufacturerStringDescriptorTC(iClient, a1Buf);
		break;
		}
		
	case RDevUsbcClient::EControlGetProductStringDescriptor:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlGetManufacturerStringDescriptor")));
		r = iDescriptors.GetProductStringDescriptorTC(iClient, a1Buf);
		break;
		}

	case RDevUsbcClient::EControlSetProductStringDescriptor:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlSetProductStringDescriptor")));
		r = iDescriptors.SetProductStringDescriptorTC(iClient, a1Buf);
		break;
		}

	case RDevUsbcClient::EControlGetSerialNumberStringDescriptor:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlGetManufacturerStringDescriptor")));
		r = iDescriptors.GetSerialNumberStringDescriptorTC(iClient, a1Buf);
		break;
		}

	case RDevUsbcClient::EControlSetSerialNumberStringDescriptor:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlSetSerialNumberStringDescriptor")));
		r = iDescriptors.SetSerialNumberStringDescriptorTC(iClient, a1Buf);
		break;
		}
		
	case RDevUsbcClient::EControlGetConfigurationStringDescriptor:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlGetManufacturerStringDescriptor")));
		r = iDescriptors.GetConfigurationStringDescriptorTC(iClient, a1Buf);
		break;
		}

	case RDevUsbcClient::EControlSetConfigurationStringDescriptor:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("EControlSetConfigurationStringDescriptor")));
		r = iDescriptors.SetConfigurationStringDescriptorTC(iClient, a1Buf);
		break;
		}
	case RDevUsbcClient::EControlEndpointStatus:
		{
		TInt ep = (TInt)a1;
		DTestUsbcEndpoint* pEndpoint = iEndpoints[FindRealEndpoint(ep)];
		TEndpointState state = EEndpointStateUnknown;
		if (pEndpoint->IsHalted())
			{
			state = EEndpointStateStalled;
			}
		else
			{
			state = EEndpointStateNotStalled;
			}
		__THREADRAWWRITE(iClient, a2, (TUint8*)&state, (TInt)sizeof(state));		
		break;
		}
		
	case RDevTestUsbcClient::ETestControlReqEndpointStatusNotify:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("ETestControlReqEndpointStatusNotify")));
		r = HostEndpointStatusNotify((TInt)a2, (TRequestStatus*)a1);
		break;
		}
	
	case RDevTestUsbcClient::ETestControlClearEndpoint:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("ETestControlClearEndpoint")));
		r = ClearEndpoint((TInt)a1);
		break;
		}
		
	default:
		r = KErrNotSupported;
		}
		
	return r;
	}

TInt DLddTestUsbcChannel::SetInterface(TInt aInterfaceNumber,
									   TUsbcInterfaceInfoBuf *aUserInterfaceInfoBuf,
									   TPtr8* aInterfaceString)
	{
// The Interface Descriptor string is no interest to us
// so leave that as is, the controller will have to take a local copy
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("SetInterface Entry Interface#=%d Endpoints =%d"),aInterfaceNumber,(*aUserInterfaceInfoBuf)().iTotalEndpointsUsed));
	TInt r = KErrNone;
	TUsbcEndpointInfo* pEndpointData=NULL;
	TInt numberOfEndpoints=0;
	TUsbcInterfaceInfoBuf interfaceBuff;
	TInt bufLen=interfaceBuff.Length();
	TInt srcLen=__THREADDESLEN(iClient,aUserInterfaceInfoBuf);
	if(srcLen<bufLen)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("SetInterface can't copy")));
		__THREADPANIC(iClient,EDesOverflow);
		}
	r = __THREADREAD(iClient, aUserInterfaceInfoBuf, interfaceBuff);
	if(r!=KErrNone)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("SetInterface Copy failed reason=%d"),r));
		__THREADPANIC(iClient,r);
		}
	pEndpointData=interfaceBuff().iEndpointData;

// If an alternate interface is being asked for then do nothing
// just pass it down to the Controller
	numberOfEndpoints=interfaceBuff().iTotalEndpointsUsed;
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("SetInterface 10 numberOfEndpoints=%d"),numberOfEndpoints));

// other endpoints
	for(TInt i=1;i<=numberOfEndpoints;i++,pEndpointData++)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("SetInterface for ep=%d"),i));
		if(!ValidateEndpoint(pEndpointData))
			{
			r=KErrUsbBadEndpoint;
			goto KillAll;
			}
		}

	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("SetInterface Calling controller")));
	r=SetInterface(aInterfaceNumber,
				   interfaceBuff().iClass,
				   aInterfaceString,
				   interfaceBuff().iTotalEndpointsUsed,
				   interfaceBuff().iEndpointData);
	
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("SetInterface controller returned %d"),r));
	if(r!=KErrNone)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("SetInterface failed reason=%d"),r));
		goto KillAll;
		}
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("SetInterface ready to exit")));

	return KErrNone;

KillAll:
	return r;
	}
	
TInt DLddTestUsbcChannel::SetInterface(TInt aInterfaceNumber, TUsbcClassInfo& aClass,
									   TDesC8* aString, TInt aTotalEndpointsUsed,
									   const TUsbcEndpointInfo aEndpointData[])
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("DLddTestUsbcChannel::SetInterface()")));
	for (TInt i = 0; i < aTotalEndpointsUsed; ++i)
		{
		if (aEndpointData[i].iType == KUsbEpTypeControl)
			{
			__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  control endpoints not supported")));
			return KErrNotSupported;
			}
		}
	// create & setup new interface
	TUsbcInterface* ifc = CreateInterface(aInterfaceNumber);
	if (ifc == NULL)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error (ifc == NULL)")));
		return KErrGeneral;
		}
	// Create logical endpoints
	if (CreateEndpoints(ifc, aTotalEndpointsUsed, aEndpointData) != KErrNone)
		{
 		DeleteInterface(aInterfaceNumber);
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error (CreateEndpoints() != KErrNone)")));
		return KErrGeneral;
		}
	// create & setup interface, string, and endpoint descriptors
	const TInt r = SetupIfcDescriptor(ifc, aClass, aString, aEndpointData);
	if (r != KErrNone)
		{
		return r;
		}
	return KErrNone;
	}
	
TUsbcInterface* DLddTestUsbcChannel::CreateInterface(TInt aIfc)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("DLddTestUsbcChannel::CreateInterface(x, aIfc=%d)"), aIfc));
	if (aIfc != iIfcSet.iInterfaces.Count())
		{
		// 9.2.3: "Alternate settings range from zero to one less than the number of alternate
		// settings for a specific interface." (Thus we can here only append a setting.)
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  invalid interface setting number (2): %d"), aIfc));
		return NULL;
		}
	TUsbcInterface* const ifc_ptr = new TUsbcInterface(&iIfcSet, (TUint8)aIfc);
	if (!ifc_ptr)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: new TUsbcInterface(ifcset, aIfc) failed")));
		return NULL;
		}
	iIfcSet.iInterfaces.Append(ifc_ptr);
	return ifc_ptr;
	}
	
void DLddTestUsbcChannel::DeleteInterface(TInt aIfc)
	{
	if (iIfcSet.iInterfaces.Count() <= aIfc)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: invalid interface setting: %d"), aIfc));
		return;
		}
	TUsbcInterface* const ifc_ptr = iIfcSet.iInterfaces[aIfc];
	iIfcSet.iInterfaces.Remove(aIfc);
	delete ifc_ptr;
	if (aIfc == iIfcSet.iCurrentInterface)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING(" > Warning: deleting current interface setting")));
		iIfcSet.iCurrentInterface = 0;
		}
	}
	
TInt DLddTestUsbcChannel::CreateEndpoints(TUsbcInterface* aIfc, TInt aEndpointsUsed,
										  const TUsbcEndpointInfo aEndpointData[])
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("DLddTestUsbcChannel::CreateEndpoints()")));
	
	for (TInt i = 0; i < aEndpointsUsed; ++i)
		{
		for (TInt j = 1; j <= KMaxEndpointsPerClient; ++j)
			{
			if (iEndpoints[j]->EndpointSuitable(aEndpointData[i]))
				{
				TUsbcLogicalEndpoint* const ep = new TUsbcLogicalEndpoint(j, aEndpointData[i], aIfc);
				if (!ep)
					{
					__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: new TUsbcLogicalEndpoint() failed")));
					aIfc->iEndpoints.ResetAndDestroy();
					for (TInt k = 1; k <= KMaxEndpointsPerClient; ++k)
						{
						iEndpoints[k]->iReserve = EFalse;
						}
					return KErrNoMemory;
					}
				iEndpoints[j]->iReserve = ETrue;
				iEndpoints[j]->SetClearCallback(this);
				aIfc->iEndpoints.Append(ep);
				break;
				}
			}
		}
	return KErrNone;
	}

TBool DLddTestUsbcChannel::ValidateEndpoint(TUsbcEndpointInfo* aEndpointInfo)
	{ // Quick sanity check on endpoint properties
	TUint dir=aEndpointInfo->iDir;
	TInt size=aEndpointInfo->iSize;
	if(size <=0)
		return EFalse;
	switch (aEndpointInfo->iType)
		{
		case KUsbEpTypeControl:
			if(dir != KUsbEpDirBidirect || size > 64)
				return EFalse;
			break;
		case KUsbEpTypeIsochronous:
			if((dir != KUsbEpDirIn && dir != KUsbEpDirOut) || size > 1023)
				return EFalse;
			break;
		case KUsbEpTypeBulk:
			if((dir != KUsbEpDirIn && dir != KUsbEpDirOut) || size > 64)
				return EFalse;
			break;
		case KUsbEpTypeInterrupt:
			if((dir != KUsbEpDirIn && dir != KUsbEpDirOut) || size > 64)
				return EFalse;
			break;
		default:
			return EFalse;
		}
	return ETrue;
	}

TInt DLddTestUsbcChannel::SetupIfcDescriptor(TUsbcInterface* aIfc, TUsbcClassInfo& aClass,
											 TDesC8* aString, const TUsbcEndpointInfo aEndpointData[])
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("DLddTestUsbcChannel::SetupIfcDescriptor()")));
	
	// interface descriptor
	TUsbcDescriptorBase* d = TUsbcInterfaceDescriptor::New(aIfc->iInterfaceSet->iInterfaceNumber,
														   aIfc->iSettingCode,
														   aIfc->iEndpoints.Count(),
														   aClass);
	if (!d)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING(" > Error: Memory allocation for ifc desc failed.")));
		return KErrNoMemory;
		}
	iDescriptors.InsertDescriptor(d);

	// interface string descriptor
	if (aString)
		{
		// we don't know the length of the string, so we have to allocate memory dynamically
		TUint strlen = __THREADDESLEN(iClient, aString);
		if (strlen > KUsbStringDescStringMaxSize)
			{
			__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Warning: $ descriptor too long - string will be truncated")));
			strlen = KUsbStringDescStringMaxSize;
			}
		
		HBuf8Plat* stringbuf = NULL;
		__NEWPLATBUF(stringbuf, strlen);
		if (!stringbuf)
			{
			__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING(" > Error: Memory allocation for ifc $ desc string failed.")));
			iDescriptors.DeleteIfcDescriptor(aIfc->iInterfaceSet->iInterfaceNumber, aIfc->iSettingCode);
			return KErrNoMemory;
			}
		
		TInt r;	
		__THREADREADPLATBUF(iClient, aString, stringbuf, r);
		if (r != KErrNone)
			{
			__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  > Error: Thread read error!")));
			iDescriptors.DeleteIfcDescriptor(aIfc->iInterfaceSet->iInterfaceNumber,
											 aIfc->iSettingCode);
			delete stringbuf;
			return r;
			}
		TUsbcStringDescriptor* const sd = TUsbcStringDescriptor::New(*stringbuf);
		if (!sd)
			{
			__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING(" > Error: Memory allocation for ifc $ desc failed.")));
			iDescriptors.DeleteIfcDescriptor(aIfc->iInterfaceSet->iInterfaceNumber, aIfc->iSettingCode);
			delete stringbuf;
			return KErrNoMemory;
			}
		iDescriptors.SetIfcStringDescriptor(sd, aIfc->iInterfaceSet->iInterfaceNumber, aIfc->iSettingCode);
		delete stringbuf;									// the (EPOC) descriptor was copied by New()
		}

	// endpoint descriptors
	for (TInt i = 0; i < aIfc->iEndpoints.Count(); ++i)
		{
		// The reason for using another function argument for Endpoint Info - and not possibly (similar to the
		// Endpoint Address) "aIfc->iEndpoints[i]->iPEndpoint->iLEndpoint->iInfo" - is that at this time
		// there are no logical endpoints associated with our real endpoints, i.e. iLEndpoint is NULL!
		if (aEndpointData[i].iExtra)
			{
			// if non-standard endpoint descriptor requested...
			if (aEndpointData[i].iExtra != 2)
				{
				// ...then it must be a Audio Class endpoint descriptor. Else...
				__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING(" > We only support EP desc extension of 2 bytes (not %d)"), aEndpointData[i].iExtra));
				iDescriptors.DeleteIfcDescriptor(aIfc->iInterfaceSet->iInterfaceNumber,
												 aIfc->iSettingCode);
				return KErrArgument;
				}
			d = TUsbcAudioEndpointDescriptor::New((TUint8)i, aEndpointData[i]);
			}
		else
			{
			d = TUsbcEndpointDescriptor::New((TUint8)i, aEndpointData[i]);
			}
		if (!d)
			{
			__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING(" > Memory allocation for ep desc #%d failed."), i));
			iDescriptors.DeleteIfcDescriptor(aIfc->iInterfaceSet->iInterfaceNumber,
											 aIfc->iSettingCode);
			return KErrNoMemory;
			}
		iDescriptors.InsertDescriptor(d);
		}

	return KErrNone;
	}

/**
@internalTechnology

   Releases an existing USB interface (one setting), complete with endpoints, descriptors, etc.,
   and removes it from the internal device configuration tree.
   
   @param aClientId A pointer to the LDD owning the interface.
   @param aInterfaceNumber The setting number of the interface setting to be deleted. This must be
   the highest numbered (or 'last') setting for this interface.

   @return KErrNotFound if interface (not setting) for some reason cannot be found, KErrArgument if an
   invalid interface setting number is specified (not existing or existing but too small), KErrNone if
   interface successfully released or if this client doesn't own any interface.
*/
TInt DLddTestUsbcChannel::ReleaseInterface(TInt aInterfaceNumber)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("DLddTestUsbcChannel::ReleaseInterface(..., %d)"), aInterfaceNumber));
	const TInt setting_count = iIfcSet.iInterfaces.Count();
	if ((aInterfaceNumber != 0) && ((setting_count - 1) != aInterfaceNumber))
		{
		__KTRACE_OPT(KUSB,
					 Kern::Printf(__KSTRING(" > Error: interface settings must be released in descending order:\n\r%d settings exist, #%d was requested to be released: release %d first)"),
								  setting_count, aInterfaceNumber, setting_count - 1));
		return KErrArgument;
		}
	// Reset reserved status of the endpoints
	for (TInt i = 0; i < KMaxEndpointsPerClient+1; i++)
			{
			iEndpoints[i]->iReserve = EFalse;
			}
 	if (aInterfaceNumber == 0)
 		{
 		TInt m = iIfcSet.iInterfaces.Count();
 		while (m > 0)
 			{
 			m--;
 			// Delete the setting itself + its ifc & ep descriptors
 			DeleteInterface(m);
 			iDescriptors.DeleteIfcDescriptor(0, m);
 			}
 		}
 	else
		{		
 		// Delete the setting itself + its ifc & ep descriptors
 		DeleteInterface(aInterfaceNumber);
 		iDescriptors.DeleteIfcDescriptor(0, aInterfaceNumber);
		}
	// Delete the whole interface if all settings are gone
	if (iIfcSet.iInterfaces.Count() == 0)
		{
		DeleteInterfaceSet();
		}
	return KErrNone;
	}
	
void DLddTestUsbcChannel::DeleteInterfaceSet()
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("DLddTestUsbcChannel::DeleteInterfaceSet")));
	iIfcSet.iInterfaceNumber = 0;
	iIfcSet.iCurrentInterface = 0;
	iIfcSet.iInterfaces.ResetAndDestroy();
	}

TInt DLddTestUsbcChannel::DoTransferAsyncReq(TInt aEndpointNumber, TAny* a1, TAny* a2, TRequestStatus& aStatus)
	{
	TInt r = KErrNone;
	DTestUsbcEndpoint* pEndpoint = NULL;
	TEndpointTransferInfo *pTfr = NULL;


	if(!ValidEndpoint(aEndpointNumber))
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("DoRequest Read: in error complete")));
		return KErrUsbEpNotInInterface;
		}

	if(a1 == NULL)
		{
		return KErrArgument;
		}
	
	TBool hostTransfer = EFalse;
	if(a2 != NULL)
		{
		hostTransfer = ETrue;
		}

	TEndpointTransferInfo transferInfo;
	pTfr = (TEndpointTransferInfo*)&transferInfo;
	r = __THREADRAWREAD(iClient, a1, (TUint8*)&transferInfo, sizeof(TEndpointTransferInfo));
	if(r != KErrNone)
		{
		__THREADPANIC(iClient, r);
		}
	if (aEndpointNumber != 0)
		{
		if (hostTransfer)
			{
			pEndpoint = iEndpoints[aEndpointNumber];
			}
		else
			{
			pEndpoint = iEndpoints[FindRealEndpoint(aEndpointNumber)];
			}
		}
	else
		{
		pEndpoint = iEndpoints[0];
		}
	if(!pEndpoint)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("DoRequest Read: in error complete")));
		return KErrUsbEpNotInInterface;
		}

	switch(pTfr->iTransferType)
		{
    case ETransferTypeReadUntilShort:
	case ETransferTypeReadData:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("DoRequest Read")));

		if(!hostTransfer && !pEndpoint->SupportsDir(KUsbEpDirOut) && !pEndpoint->SupportsDir(KUsbEpDirBidirect))
			{ // Trying to make the wrong thing
			__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("DoRequest Read: in error complete")));
			r = KErrUsbEpBadDirection;
			break;
			}

		// Set the length of data to zero now to catch all cases
		TPtrC8 pZeroDesc(NULL, 0);
		r = __THREADWRITE(iClient, pTfr->iDes, pZeroDesc);	 // set client descriptor length to zero
		if(r != KErrNone)
			__THREADPANIC(iClient, r);
		if (hostTransfer)
			r = pEndpoint->NewHostRequest(iClient, &aStatus, *pTfr, pTfr->iTransferType);
		else
			r = pEndpoint->NewRequest(iClient, &aStatus, *pTfr, pTfr->iTransferType);
		break;
		}

	case ETransferTypeWrite:
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("DoRequest Write 1")));
		if(!hostTransfer && !pEndpoint->SupportsDir(KUsbEpDirIn) && !pEndpoint->SupportsDir(KUsbEpDirBidirect))
			{
			__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("DoRequest Write: wrong direction complete")));
			r = KErrUsbEpBadDirection;
			break;
			}
		if (hostTransfer)
			r = pEndpoint->NewHostRequest(iClient, &aStatus, *pTfr, ETransferTypeWrite);
		else
			r = pEndpoint->NewRequest(iClient, &aStatus, *pTfr, ETransferTypeWrite);
		break;
		}
	default:
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("DoRequest Not supported complete")));
		r = KErrNotSupported;
		break;
		}
	return r;
	}
	
TInt DLddTestUsbcChannel::HaltClearEndpoint(TBool aHalt, TInt aEndpointNumber)
	{
	DTestUsbcEndpoint* pEndpoint = NULL;
	if (aEndpointNumber != 0)
		{
		pEndpoint = iEndpoints[FindRealEndpoint(aEndpointNumber)];
		}
	else
		{
		pEndpoint = iEndpoints[0];
		}
	TInt err;
	if (aHalt)
		{
		err = pEndpoint->Halt();
		}
	else
		{
		err = pEndpoint->Clear();
		}
	return err;
	}
	
TInt DLddTestUsbcChannel::HostEndpointStatusNotify(TInt aEndpointNumber, TRequestStatus* aStatus)
	{
	DTestUsbcEndpoint* pEndpoint = iEndpoints[aEndpointNumber];
	return pEndpoint->HostStatusNotify(iClient, aStatus);
	}
	
TInt DLddTestUsbcChannel::ClearEndpoint(TInt aEndpointNumber)
	{
	DTestUsbcEndpoint* pEndpoint = iEndpoints[aEndpointNumber];
	return pEndpoint->Clear();
	}

TInt DLddTestUsbcChannel::EndpointStatusNotify(TUint* aEndpointMask, TRequestStatus* aStatus)
	{
	iEndpointStatusMask = aEndpointMask;
	iEndpointStatusNotifyRequest = aStatus;
	return KErrNone;
	}

void DLddTestUsbcChannel::EndpointStatusNotifyCallback()
	{
	if (iEndpointStatusNotifyRequest == NULL || iEndpointStatusMask == NULL)
		{
		return;
		}
	
	//Get status for interface's endpoints.
	//NOTE: currently we only support one interface.
	TUsbcInterface* interface = iIfcSet.iInterfaces[0];
	TUint bitmask = 0;
	for (TInt i = interface->iEndpoints.Count() - 1; i >= 0; i--)
		{
		TUsbcLogicalEndpoint* logep = interface->iEndpoints[i];
		DTestUsbcEndpoint* pEndpoint = iEndpoints[FindRealEndpoint(logep->iLEndpointNum)];
		if (pEndpoint->IsHalted())
			{
			bitmask |= 1;
			bitmask = bitmask << 1;
			}
		}
	
	//Write bitmask back to client space.
	TInt r = __THREADRAWWRITE(iClient, (void*)iEndpointStatusMask, (TUint8*)&bitmask, (TInt)sizeof(bitmask));
	
	//Complete client request.
	Kern::RequestComplete(iClient, iEndpointStatusNotifyRequest, r);
		
	iEndpointStatusMask = NULL;
	iEndpointStatusNotifyRequest = NULL;
	}
	
TBool DLddTestUsbcChannel::ValidEndpoint(TInt aEndpointNumber)
	{
	return (aEndpointNumber <= 5 && aEndpointNumber >= 0);
	}

TInt DLddTestUsbcChannel::FindRealEndpoint(TInt aEndpointNumber)
	{
	TUsbcInterface* pIfc = iIfcSet.CurrentInterface();
	return pIfc->iEndpoints[aEndpointNumber - 1]->iLEndpointNum;
	}

void DLddTestUsbcChannel::AlternateDeviceStatusNotify()
	{
	if (iAlternateDeviceStatusNotifyRequest != NULL)
		{
		TInt r = __THREADRAWWRITE(iClient, (void*)iAlternateDeviceStatusNotifyValue, (TUint8*)&iDeviceState, (TInt)sizeof(iDeviceState));
		Kern::RequestComplete(iClient, iAlternateDeviceStatusNotifyRequest, r);
		iAlternateDeviceStatusNotifyRequest = NULL;
		}
	}

TInt DLddTestUsbcChannel::SetAlternateDeviceStatusNotify(TRequestStatus* aStatus, TUint* aValue)
	{
	if (iAlternateDeviceStatusNotifyRequest != NULL)
		{
		return KErrInUse;
		}

	TRequestStatus s;
	s = KRequestPending;

	__THREADRAWWRITE(iClient, (void*)aStatus, (TUint8*)&s, (TInt)sizeof(s));
	iAlternateDeviceStatusNotifyRequest = aStatus;
	iAlternateDeviceStatusNotifyValue = aValue;
	return KErrNone;
	}

void DLddTestUsbcChannel::CancelAlternateDeviceStatusNotify()
	{
	if (iAlternateDeviceStatusNotifyRequest != NULL)
		{
		__THREADRAWWRITE(iClient, (void*)iAlternateDeviceStatusNotifyValue, (TUint8*)&iDeviceState, (TInt)sizeof(iDeviceState));
		Kern::RequestComplete(iClient, iAlternateDeviceStatusNotifyRequest, KErrCancel);
		iAlternateDeviceStatusNotifyRequest = NULL;
		}
	}

TInt DLddTestUsbcChannel::ReEnumerate(TRequestStatus* aStatus)
	{
	SetDeviceState(EUsbcDeviceStateConfigured);
	Kern::RequestComplete(iClient, aStatus, KErrNone);
	return KErrNone;
	}
	
void DLddTestUsbcChannel::SetDeviceState(TUsbcDeviceState aState)
	{
	iDeviceState = aState;
	AlternateDeviceStatusNotify();
	}
	
