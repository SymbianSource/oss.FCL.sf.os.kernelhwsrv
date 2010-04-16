// Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32/drivers/usbcc/chapter9.cpp
// Platform independent layer (PIL) of the USB Device controller driver:
// Processing of USB spec chapter 9 standard requests.
// 
//

/**
 @file chapter9.cpp
 @internalTechnology
*/

#include <drivers/usbc.h>


//#define ENABLE_EXCESSIVE_DEBUG_OUTPUT

//
// The way functions are called after an request has been completed by the PSL:
//
//                                         Ep0RequestComplete
//                                                 |
//                                        ------------------------------------------------
//                                       |                                                |
//                              ProcessEp0ReceiveDone                            ProcessEp0TransmitDone
//                                       |                                                |
//                   ---------------------------------------                              |
//                  |                                       |                             |
//        ProcessEp0SetupReceived                 ProcessEp0DataReceived        ProcessDataTransferDone
//                  |                                       |
//         ---------------------                      ---------------
//        |                     |                    |               |
//   ProcessXXX       ProcessDataTransferDone   ProceedXXX  ProcessDataTransferDone
//
//   XXX = Specific_Request
//

//
// === USB Controller member function implementation - PSL API (protected) ========================
//

/** Used to synchronize the Ep0 state machine between the PSL and PIL.
	Accepts a SETUP packet and returns the next Ep0 state.

	@param aSetupBuf The SETUP packet just received by the PSL.
	@return The next Ep0 state.

	@publishedPartner @released
*/
TUsbcEp0State DUsbClientController::EnquireEp0NextState(const TUint8* aSetupBuf) const
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::EnquireEp0NextState()"));

	// This function may be called by the PSL from within an ISR -- so we have
	// to take care what we do here (and also in all functions that get called
	// from here).

	if (SWAP_BYTES_16((reinterpret_cast<const TUint16*>(aSetupBuf)[3])) == 0) // iLength
		{
		__KTRACE_OPT(KUSB, Kern::Printf("  --> EEp0StateStatusIn"));
		return EEp0StateStatusIn;							// No-data Control => Status_IN
		}
	else if ((aSetupBuf[0] & KUsbRequestType_DirMask) == KUsbRequestType_DirToDev)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("  --> EEp0StateDataOut"));
		return EEp0StateDataOut;							// Control Write => Data_OUT
		}
	else
		{
		__KTRACE_OPT(KUSB, Kern::Printf("  --> EEp0StateDataIn"));
		return EEp0StateDataIn;								// Control Read => Data_IN
		}
	}


TInt DUsbClientController::ProcessEp0ReceiveDone(TInt aCount)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProcessEp0ReceiveDone()"));
	TInt r;
	if (iEp0DataReceiving == EFalse)
		{
		// It's obviously a Setup packet, so...
		r = ProcessEp0SetupReceived(aCount);
		}
	else
		{
		// If it isn't a Setup, it must be data...
		// (This is actually not quite true, as it could also be - in theory - a new Setup packet
		//  when the host has abandoned, for whatever reason, the previous one which was still
		//  in progress. However no such case is known to have occurred with this driver, or at
		//  least it didn't lead to problems.
		//  Some UDCs have a dedicated interrupt for Setup packets, but so far this driver hasn't
		//  made use of such a feature (as it would require a PSL/PIL API change).)
		r = ProcessEp0DataReceived(aCount);
		}
	return r;
	}


TInt DUsbClientController::ProcessEp0TransmitDone(TInt aCount, TInt aError)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProcessEp0TransmitDone()"));
	// In any case: there's now no longer a write pending
	iEp0WritePending = EFalse;
	// If it was a client who set up this transmission, we report to that client
	if (iEp0ClientDataTransmitting)
		{
		iEp0ClientDataTransmitting = EFalse;
		TUsbcRequestCallback* const p = iRequestCallbacks[KEp0_Tx];
		if (p)
			{
			__ASSERT_DEBUG((p->iTransferDir == EControllerWrite), Kern::Fault(KUsbPILPanicCat, __LINE__));
			p->iError = aError;
			p->iTxBytes = aCount;
			ProcessDataTransferDone(*p);
			return KErrNone;
			}
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: DUsbClientController::ProcessEpTransmitDone: Stalling Ep0"));
		StallEndpoint(KEp0_In);								// request not found
		return KErrNotFound;
		}
	// If _we_ sent the data, we simply do nothing here...
	return KErrNone;
	}


#define USB_PROCESS_REQUEST(request) \
	if (Process ## request(packet) != KErrNone) \
		{ \
		__KTRACE_OPT(KUSB, \
					 Kern::Printf("  ProcessEp0SetupReceived: Stalling Ep0")); \
		StallEndpoint(KEp0_In); \
		}

TInt DUsbClientController::ProcessEp0SetupReceived(TInt aCount)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProcessEp0SetupReceived()"));

	if (aCount > iEp0MaxPacketSize)
		{
		// Fatal error: too much data!
		aCount = iEp0MaxPacketSize;
		}

	// first we split the data into meaningful units:
	TUsbcSetup packet;
	Buffer2Setup(iEp0_RxBuf, packet);

#if defined(_DEBUG) && defined(ENABLE_EXCESSIVE_DEBUG_OUTPUT)
	// let's see what we've got:
	__KTRACE_OPT(KUSB, Kern::Printf("  bmRequestType = 0x%02x", packet.iRequestType));
	if ((packet.iRequestType & KUsbRequestType_TypeMask) == KUsbRequestType_TypeStd)
		{
		switch (packet.iRequest)
			{
		case KUsbRequest_GetStatus:
			__KTRACE_OPT(KUSB, Kern::Printf("  bRequest      = 0x%02x (GET_STATUS)",
											KUsbRequest_GetStatus));
			break;
		case KUsbRequest_ClearFeature:
			__KTRACE_OPT(KUSB, Kern::Printf("  bRequest      = 0x%02x (CLEAR_FEATURE)",
											KUsbRequest_ClearFeature));
			break;
		case KUsbRequest_SetFeature:
			__KTRACE_OPT(KUSB, Kern::Printf("  bRequest      = 0x%02x (SET_FEATURE)",
											KUsbRequest_SetFeature));
			break;
		case KUsbRequest_SetAddress:
			__KTRACE_OPT(KUSB, Kern::Printf("  bRequest      = 0x%02x (SET_ADDRESS)",
											KUsbRequest_SetAddress));
			break;
		case KUsbRequest_GetDescriptor:
			__KTRACE_OPT(KUSB, Kern::Printf("  bRequest      = 0x%02x (GET_DESCRIPTOR)",
											KUsbRequest_GetDescriptor));
			break;
		case KUsbRequest_SetDescriptor:
			__KTRACE_OPT(KUSB, Kern::Printf("  bRequest      = 0x%02x (SET_DESCRIPTOR)",
											KUsbRequest_SetDescriptor));
			break;
		case KUsbRequest_GetConfig:
			__KTRACE_OPT(KUSB, Kern::Printf("  bRequest      = 0x%02x (GET_CONFIGURATION)",
											KUsbRequest_GetConfig));
			break;
		case KUsbRequest_SetConfig:
			__KTRACE_OPT(KUSB, Kern::Printf("  bRequest      = 0x%02x (SET_CONFIGURATION)",
											KUsbRequest_SetConfig));
			break;
		case KUsbRequest_GetInterface:
			__KTRACE_OPT(KUSB, Kern::Printf("  bRequest      = 0x%02x (GET_INTERFACE)",
											KUsbRequest_GetInterface));
			break;
		case KUsbRequest_SetInterface:
			__KTRACE_OPT(KUSB, Kern::Printf("  bRequest      = 0x%02x (SET_INTERFACE)",
											KUsbRequest_SetInterface));
			break;
		case KUsbRequest_SynchFrame:
			__KTRACE_OPT(KUSB, Kern::Printf("  bRequest      = 0x%02x (SYNCH_FRAME)",
											KUsbRequest_SynchFrame));
			break;
		default:
			__KTRACE_OPT(KPANIC, Kern::Printf("  Error: bRequest = 0x%02x (UNKNWON STANDARD REQUEST)",
											  packet.iRequest));
			break;
			}
		}
	else
		{
		__KTRACE_OPT(KUSB, Kern::Printf("  bRequest      = 0x%02x (NON-STANDARD REQUEST)",
										packet.iRequest));
		}
	__KTRACE_OPT(KUSB, Kern::Printf("  wValue        = 0x%04x", packet.iValue));
	__KTRACE_OPT(KUSB, Kern::Printf("  wIndex        = 0x%04x", packet.iIndex));
	__KTRACE_OPT(KUSB, Kern::Printf("  wLength       = 0x%04x", packet.iLength));
#endif // defined(_DEBUG) && defined(ENABLE_EXCESSIVE_DEBUG_OUTPUT)

	// now the actual analysis
	if ((packet.iRequestType & KUsbRequestType_TypeMask) == KUsbRequestType_TypeStd)
		{
		iEp0ReceivedNonStdRequest = EFalse;
		switch (packet.iRequest)
			{
		case KUsbRequest_GetStatus:
			switch (packet.iRequestType & KUsbRequestType_DestMask)
				{ // Recipient
			case KUsbRequestType_DestDevice:
				USB_PROCESS_REQUEST(GetDeviceStatus);
				break;
			case KUsbRequestType_DestIfc:
				USB_PROCESS_REQUEST(GetInterfaceStatus);
				break;
			case KUsbRequestType_DestEp:
				USB_PROCESS_REQUEST(GetEndpointStatus);
				break;
			default:
				__KTRACE_OPT(KPANIC, Kern::Printf("  Error: GET STATUS - Other or Unknown recipient"));
				__KTRACE_OPT(KPANIC, Kern::Printf("  -> DUsbClientController::ProcessEp0SetupReceived: "
												  "Stalling Ep0"));
				StallEndpoint(KEp0_In);
				break;
				}
			break;
		case KUsbRequest_ClearFeature:
		case KUsbRequest_SetFeature:
			switch (packet.iRequestType & KUsbRequestType_DestMask)
				{ // Recipient
			case KUsbRequestType_DestDevice:
				USB_PROCESS_REQUEST(SetClearDevFeature);
				break;
			case KUsbRequestType_DestIfc:
				USB_PROCESS_REQUEST(SetClearIfcFeature);
				break;
			case KUsbRequestType_DestEp:
				USB_PROCESS_REQUEST(SetClearEpFeature);
				break;
			default:
				__KTRACE_OPT(KPANIC, Kern::Printf("  Error: SET/CLEAR FEATURE - "
												  "Other or Unknown recipient"));
				__KTRACE_OPT(KPANIC, Kern::Printf("  -> Stalling Ep0"));
				StallEndpoint(KEp0_In);
				break;
				}
			break;
		case KUsbRequest_SetAddress:
			USB_PROCESS_REQUEST(SetAddress);
			break;
		case KUsbRequest_GetDescriptor:
			USB_PROCESS_REQUEST(GetDescriptor);
			break;
		case KUsbRequest_SetDescriptor:
			USB_PROCESS_REQUEST(SetDescriptor);
			break;
		case KUsbRequest_GetConfig:
			USB_PROCESS_REQUEST(GetConfiguration);
			break;
		case KUsbRequest_SetConfig:
			USB_PROCESS_REQUEST(SetConfiguration);
			break;
		case KUsbRequest_GetInterface:
			USB_PROCESS_REQUEST(GetInterface);
			break;
		case KUsbRequest_SetInterface:
			USB_PROCESS_REQUEST(SetInterface);
			break;
		case KUsbRequest_SynchFrame:
			USB_PROCESS_REQUEST(SynchFrame);
			break;
		default:
			__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Unknown/unsupported Std Setup Request"));
			__KTRACE_OPT(KPANIC, Kern::Printf("  -> Stalling Ep0"));
			StallEndpoint(KEp0_In);
			break;
			}
		}
	else
		{
		// Type mask != KUsbRequestType_TypeStd => class- or vendor-specific request
        iEp0ReceivedNonStdRequest = ETrue;
		const DBase* client = NULL;
		switch (packet.iRequestType & KUsbRequestType_DestMask)
			{ // Recipient
		case KUsbRequestType_DestDevice:
			client = iEp0DeviceControl;
			break;
		case KUsbRequestType_DestIfc:
		    //Add this mutex to protect the interface set data structure
		    if (NKern::CurrentContext() == EThread)
		        {
		        NKern::FMWait(&iMutex);
		        }
			if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateConfigured)
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Invalid device state"));
				}
			else
				{
				const TUsbcInterfaceSet* const ifcset_ptr =
					InterfaceNumber2InterfacePointer(packet.iIndex);
				//In some rare case, ifcset_ptr is not NULL but the ifcset_ptr->iInterfaces.Count() is 0,
				//so panic will happen when excute the following line. so I add the conditon
				//0 != ifcset_ptr->iInterfaces.Count() here.
				if (ifcset_ptr && 0 != ifcset_ptr->iInterfaces.Count())
					{
					if (ifcset_ptr->CurrentInterface()->iNoEp0Requests)
						{
						__KTRACE_OPT(KUSB, Kern::Printf("  Recipient says: NoEp0RequestsPlease"));
						}
					else
						{
						client = ifcset_ptr->iClientId;
						}
					}
				else
					{
					__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Interface 0x%02x does not exist",
													  packet.iIndex));
					}
				}
	        if (NKern::CurrentContext() == EThread)
	            {
                NKern::FMSignal(&iMutex);
	            }
			break;
		case KUsbRequestType_DestEp:
		    //Add this mutex to protect the interface set data structure
	        if (NKern::CurrentContext() == EThread)
	            {
                NKern::FMWait(&iMutex);
	            }
			if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateConfigured)
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Invalid device state"));
				}
			else if (EndpointExists(packet.iIndex) == EFalse)
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Endpoint 0x%02x does not exist",
												  packet.iIndex));
				}
			else
				{
				const TInt idx = EpAddr2Idx(packet.iIndex);
				const TUsbcInterfaceSet* const ifcset_ptr =
					iRealEndpoints[idx].iLEndpoint->iInterface->iInterfaceSet;
				if (ifcset_ptr->CurrentInterface()->iNoEp0Requests)
					{
					__KTRACE_OPT(KUSB, Kern::Printf("  Recipient says: NoEp0RequestsPlease"));
					}
				else
					{
					client = ifcset_ptr->iClientId;
					}
				}
            if (NKern::CurrentContext() == EThread)
                {
                NKern::FMSignal(&iMutex);
                }
			break;
		default:
			__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Other or Unknown recipient"));
			break;
			}
		if (client != NULL)
			{
			// Try to relay packet to the appropriate recipient
			TSglQueIter<TUsbcRequestCallback> iter(iEp0ReadRequestCallbacks);
			TUsbcRequestCallback* p;
			while ((p = iter++) != NULL)
				{
				if (p->Owner() == client)
					{
					__ASSERT_DEBUG((p->iEndpointNum == 0), Kern::Fault(KUsbPILPanicCat, __LINE__));
					__ASSERT_DEBUG((p->iTransferDir == EControllerRead), Kern::Fault(KUsbPILPanicCat, __LINE__));
					__KTRACE_OPT(KUSB, Kern::Printf("  Found Ep0 read request"));
					if (packet.iLength != 0)
						{
						if ((packet.iRequestType & KUsbRequestType_DirMask) == KUsbRequestType_DirToDev)
							{
							// Data transfer & direction OUT => there'll be a DATA_OUT stage
							__KTRACE_OPT(KUSB, Kern::Printf("  Next is DATA_OUT: setting up DataOutVars"));
							SetEp0DataOutVars(packet, client);
							}
						else if ((packet.iRequestType & KUsbRequestType_DirMask) == KUsbRequestType_DirToHost)
							{
							// For possible later use (ZLP).
							iEp0_TxNonStdCount = packet.iLength;
							}
						}
					memcpy(p->iBufferStart, iEp0_RxBuf, aCount);
					p->iError = KErrNone;					// if it wasn't 'KErrNone' we wouldn't be here
					*(p->iPacketSize) = aCount;
					p->iRxPackets = 1;
					*(p->iPacketIndex) = 0;
					ProcessDataTransferDone(*p);
					return KErrNone;
					}
				}
			__KTRACE_OPT(KUSB, Kern::Printf("  Ep0 read request not found: setting RxExtra vars (Setup)"));
			iEp0_RxExtraCount = aCount;
			iEp0_RxExtraData = ETrue;
			return KErrNotFound;
			}
		else // if (client == NULL)
			{
			__KTRACE_OPT(KPANIC, Kern::Printf("  Ep0 request error: Stalling Ep0"));
			StallEndpoint(KEp0_In);
			return KErrGeneral;
			}
		}
	return KErrNone;
	}

#undef USB_PROCESS_REQUEST


TInt DUsbClientController::ProcessEp0DataReceived(TInt aCount)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProcessEp0DataReceived()"));

	__KTRACE_OPT(KUSB, Kern::Printf("  : %d bytes", aCount));

	if (aCount > iEp0MaxPacketSize)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Too much data"));
		aCount = iEp0MaxPacketSize;
		}
	iEp0DataReceived += aCount;
	if (iEp0ClientId == NULL)
		{
		// it is us (not an app), who owns this transaction
		switch (iSetup.iRequest)
			{
#ifdef USB_SUPPORTS_SET_DESCRIPTOR_REQUEST
		case KUsbRequest_SetDescriptor:
			memcpy(iEp0_RxCollectionBuf + iEp0DataReceived, iEp0_RxBuf, aCount);
			ProceedSetDescriptor();
			break;
#endif
		default:
			__KTRACE_OPT(KPANIC, Kern::Printf("  Error: invalid request in iSetup"));
			__KTRACE_OPT(KPANIC, Kern::Printf("  -> DUsbClientController::ProcessEp0DataReceived: Stalling Ep0"));
			StallEndpoint(KEp0_In);
			ResetEp0DataOutVars();
			break;
			}
		}
	else
		{
		// pass the data on to a client
		TSglQueIter<TUsbcRequestCallback> iter(iEp0ReadRequestCallbacks);
		TUsbcRequestCallback* p;
		while ((p = iter++) != NULL)
			{
			if (p->Owner() == iEp0ClientId)
				{
				__ASSERT_DEBUG((p->iEndpointNum == 0), Kern::Fault(KUsbPILPanicCat, __LINE__));
				__ASSERT_DEBUG((p->iTransferDir == EControllerRead), Kern::Fault(KUsbPILPanicCat, __LINE__));
				__KTRACE_OPT(KUSB, Kern::Printf("  Found Ep0 read request"));
				memcpy(p->iBufferStart, iEp0_RxBuf, aCount);
				p->iError = KErrNone;						// if it wasn't 'KErrNone' we wouldn't be here
				*(p->iPacketSize) = aCount;
				p->iRxPackets = 1;
				*(p->iPacketIndex) = 0;
				ProcessDataTransferDone(*p);
				goto found;
				}
			}
		__KTRACE_OPT(KUSB, Kern::Printf("  Ep0 read request not found: setting RxExtra vars (Data)"));
		iEp0_RxExtraCount = aCount;
		iEp0_RxExtraData = ETrue;
		iEp0DataReceived -= aCount;
		return KErrNotFound;
		}
 found:
	if (iEp0DataReceived >= iSetup.iLength)
		{
		// all data seems now to be here
		ResetEp0DataOutVars();
		}
	return KErrNone;
	}


// --- The USB Spec Chapter 9 Standard Endpoint Zero Device Requests ---

TInt DUsbClientController::ProcessGetDeviceStatus(const TUsbcSetup& aPacket)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProcessGetDeviceStatus()"));
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateAddress)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Invalid device state"));
		return KErrGeneral;
		}
	const TUint16 status = ((DeviceSelfPowered() ? KUsbDevStat_SelfPowered : 0) |
					  (iRmWakeupStatus_Enabled ? KUsbDevStat_RemoteWakeup : 0));
	__KTRACE_OPT(KUSB, Kern::Printf("  Reporting device status: 0x%02x", status));
	*reinterpret_cast<TUint16*>(iEp0_TxBuf) = SWAP_BYTES_16(status);
	if (SetupEndpointZeroWrite(iEp0_TxBuf, sizeof(status)) == KErrNone)
		{
		iEp0WritePending = ETrue;
		}
	return KErrNone;
	}


TInt DUsbClientController::ProcessGetInterfaceStatus(const TUsbcSetup& aPacket)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProcessGetInterfaceStatus()"));
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateConfigured)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Invalid device state"));
		return KErrGeneral;
		}
	if (InterfaceExists(aPacket.iIndex) == EFalse)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Interface does not exist"));
		return KErrGeneral;
		}
	const TUint16 status = 0x0000;							// as of USB Spec 2.0
	__KTRACE_OPT(KUSB, Kern::Printf("  Reporting interface status: 0x%02x", status));
	*reinterpret_cast<TUint16*>(iEp0_TxBuf) = SWAP_BYTES_16(status);
	if (SetupEndpointZeroWrite(iEp0_TxBuf, sizeof(status)) == KErrNone)
		{
		iEp0WritePending = ETrue;
		}
	return KErrNone;
	}


TInt DUsbClientController::ProcessGetEndpointStatus(const TUsbcSetup& aPacket)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProcessGetEndpointStatus()"));
	if (iTrackDeviceState &&
		((iDeviceState < EUsbcDeviceStateAddress) ||
		 (iDeviceState == EUsbcDeviceStateAddress && (aPacket.iIndex & KUsbEpAddress_Portmask) != 0)))
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Invalid device state"));
		return KErrGeneral;
		}
	if (EndpointExists(aPacket.iIndex) == EFalse)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Endpoint does not exist"));
		return KErrGeneral;
		}
	const TInt ep = EpAddr2Idx(aPacket.iIndex);
	const TUint16 status = (iRealEndpoints[ep].iHalt) ?	 KUsbEpStat_Halt : 0;
	__KTRACE_OPT(KUSB, Kern::Printf("  Reporting endpoint status 0x%02x for real endpoint %d",
									status, ep));
	*reinterpret_cast<TUint16*>(iEp0_TxBuf) = SWAP_BYTES_16(status);
	if (SetupEndpointZeroWrite(iEp0_TxBuf, 2) == KErrNone)
		{
		iEp0WritePending = ETrue;
		}
	return KErrNone;
	}


TInt DUsbClientController::ProcessSetClearDevFeature(const TUsbcSetup& aPacket)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProcessSetClearDevFeature()"));
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateDefault)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Invalid device state"));
		return KErrGeneral;
		}

	TUint test_sel = 0;

	if (aPacket.iRequest == KUsbRequest_SetFeature)
		{
		switch (aPacket.iValue)
			{
		case KUsbFeature_RemoteWakeup:
			if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateAddress)
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Invalid device state"));
				return KErrGeneral;
				}
			iRmWakeupStatus_Enabled = ETrue;
			break;
		case KUsbFeature_TestMode:
			if (!iHighSpeed)
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Request only supported in High-Speed mode"));
				return KErrGeneral;
				}
			if (LowByte(aPacket.iIndex) != 0)
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Lower byte of wIndex must be zero"));
				return KErrGeneral;
				}
			test_sel = HighByte(aPacket.iIndex);
			if ((test_sel < KUsbTestSelector_Test_J) || (test_sel > KUsbTestSelector_Test_Force_Enable))
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Invalid test selector: %d", test_sel));
				return KErrGeneral;
				}
			break;
		case KUsbFeature_B_HnpEnable:
			if (!iOtgSupport)
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Request only supported on a OTG device"));
				return KErrGeneral;
				}
			if (!(iOtgFuncMap & KUsbOtgAttr_HnpSupp))
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Request only valid if OTG device supports HNP"));
				return KErrGeneral;
				}
			iOtgFuncMap |= KUsbOtgAttr_B_HnpEnable;
			OtgFeaturesNotify();
			break;
		case KUsbFeature_A_HnpSupport:
			if (!iOtgSupport)
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Request only supported on a OTG device"));
				return KErrGeneral;
				}
			if (!(iOtgFuncMap & KUsbOtgAttr_HnpSupp))
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Request only valid if OTG device supports HNP"));
				return KErrGeneral;
				}
			iOtgFuncMap |= KUsbOtgAttr_A_HnpSupport;
			OtgFeaturesNotify();
			break;
		case KUsbFeature_A_AltHnpSupport:
			if (!iOtgSupport)
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Request only supported on a OTG device"));
				return KErrGeneral;
				}
			if (!(iOtgFuncMap & KUsbOtgAttr_HnpSupp))
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Request only valid if OTG device supports HNP"));
				return KErrGeneral;
				}
			iOtgFuncMap |= KUsbOtgAttr_A_AltHnpSupport;
			OtgFeaturesNotify();
			break;
		default:
			__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Unknown feature requested"));
			return KErrGeneral;
			}
		}
	else // KUsbRequest_ClearFeature
		{
		switch (aPacket.iValue)
			{
		case KUsbFeature_RemoteWakeup:
			if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateAddress)
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Invalid device state"));
				return KErrGeneral;
				}
			iRmWakeupStatus_Enabled = EFalse;
			break;
		default:
			__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Unknown feature requested"));
			return KErrGeneral;
			}
		}

	SendEp0ZeroByteStatusPacket();							// success: zero bytes data during status stage

	// 9.4.9: "The transition to test mode of an upstream facing port must not happen until
	// after the status stage of the request."
	if (test_sel)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Entering HS Test Mode %d", test_sel));
		EnterTestMode(test_sel);
		}

	return KErrNone;
	}


TInt DUsbClientController::ProcessSetClearIfcFeature(const TUsbcSetup& aPacket)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProcessSetClearIfcFeature()"));
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateConfigured)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Invalid device state"));
		return KErrGeneral;
		}
	// No interface features defined in USB spec, thus
	return KErrGeneral;
	}


TInt DUsbClientController::ProcessSetClearEpFeature(const TUsbcSetup& aPacket)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProcessSetClearEpFeature()"));
	if (iTrackDeviceState &&
		((iDeviceState < EUsbcDeviceStateAddress) ||
		 (iDeviceState == EUsbcDeviceStateAddress && (aPacket.iIndex & KUsbEpAddress_Portmask) != 0)))
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Invalid device state"));
		return KErrGeneral;
		}
	if (aPacket.iValue != KUsbFeature_EndpointHalt)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Unknown feature requested"));
		return KErrGeneral;
		}
	if (EndpointExists(aPacket.iIndex) == EFalse)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Endpoint does not exist"));
		return KErrGeneral;
		}
	const TInt ep = EpAddr2Idx(aPacket.iIndex);
	if (iRealEndpoints[ep].iLEndpoint->iInfo.iType == KUsbEpTypeControl ||
		iRealEndpoints[ep].iLEndpoint->iInfo.iType == KUsbEpTypeIsochronous)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Endpoint is Control or Isochronous"));
		return KErrGeneral;
		}
	SetClearHaltFeature(ep, aPacket.iRequest);
	SendEp0ZeroByteStatusPacket();							// success: zero bytes data during status stage
	return KErrNone;
	}


TInt DUsbClientController::ProcessSetAddress(const TUsbcSetup& aPacket)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProcessSetAddress()"));
	if (iTrackDeviceState && iDeviceState > EUsbcDeviceStateAddress)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Invalid device state"));
		return KErrGeneral;
		}
	const TUint16 addr = aPacket.iValue;
	if (addr > 127)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Bad address value: %d (>127)", addr));
		return KErrGeneral;
		}
	if (addr == 0)
		{
		// Enter Default state (from Default or Address)
		NextDeviceState(EUsbcDeviceStateDefault);
		}
	__KTRACE_OPT(KUSB, Kern::Printf("  USB address: %d", addr));
	// The spec says, under section 9.4.6:
	// "Stages after the initial Setup packet assume the same device address as the Setup packet. The USB
	// device does not change its device address until after the Status stage of this request is completed
	// successfully. Note that this is a difference between this request and all other requests. For all other
	// requests, the operation indicated must be completed before the Status stage."
	// Therefore, here we first send the status packet and only then actually execute the request.
	SendEp0ZeroByteStatusPacket();
	SetDeviceAddress(addr);
	return KErrNone;
	}


TInt DUsbClientController::ProcessGetDescriptor(const TUsbcSetup& aPacket)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProcessGetDescriptor()"));
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateDefault)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Invalid device state"));
		return KErrGeneral;
		}

	// Make sure we assume the correct speed
	__ASSERT_DEBUG((iHighSpeed == CurrentlyUsingHighSpeed()), Kern::Fault(KUsbPILPanicCat, __LINE__));

	TInt size = 0;
	const TInt result = iDescriptors.FindDescriptor(HighByte(aPacket.iValue), // Type
													LowByte(aPacket.iValue), // Index
													aPacket.iIndex, // Language ID
													size);

	if ((result != KErrNone) || (size == 0))
		{
		// This doesn't have to be an error - protocol-wise it's OK.
		__KTRACE_OPT(KUSB, Kern::Printf("  Couldn't retrieve descriptor"));
		return KErrGeneral;
		}

	__KTRACE_OPT(KUSB, Kern::Printf("  Descriptor found, size: %d (requested: %d)",
									size, aPacket.iLength));
	if (size > KUsbcBufSz_Ep0Tx)
		{
		// This should actually not be possible (i.e. we should never get here).
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Ep0_Tx buffer too small"));
		}
	if (size > aPacket.iLength)
		{
		// Send only as much data as requested by the host
		size = aPacket.iLength;
		}

#ifdef ENABLE_EXCESSIVE_DEBUG_OUTPUT
	__KTRACE_OPT(KUSB,
				 Kern::Printf("  Data: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ...",
							  iEp0_TxBuf[0], iEp0_TxBuf[1], iEp0_TxBuf[2], iEp0_TxBuf[3],
							  iEp0_TxBuf[4], iEp0_TxBuf[5], iEp0_TxBuf[6], iEp0_TxBuf[7]));
#endif
	// If we're about to send less bytes than expected by the host AND our number is a
	// multiple of the packet size, in order to indicate the end of the control transfer,
	// we must finally send a zero length data packet (ZLP):
	const TBool zlp = ((size < aPacket.iLength) && (size % iEp0MaxPacketSize == 0));
	if (SetupEndpointZeroWrite(iEp0_TxBuf, size, zlp) == KErrNone)
		{
		iEp0WritePending = ETrue;
		}

	return KErrNone;
	}


TInt DUsbClientController::ProcessSetDescriptor(const TUsbcSetup& aPacket)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProcessSetDescriptor()"));
#ifndef USB_SUPPORTS_SET_DESCRIPTOR_REQUEST
	return KErrGeneral;
#else
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateAddress)
		{
		// Error: Invalid device state!
		return KErrGeneral;
		}
	if (aPacket.iLength > KUsbcBufSz_Ep0Rx)
		{
		// Error: Our Rx buffer is too small! (Raise a defect to make it larger)
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Ep0_Rx buffer too small"));
		return KErrGeneral;
		}
	SetEp0DataOutVars(aPacket);
	SetupEndpointZeroRead();
	return KErrNone;
#endif
	}


TInt DUsbClientController::ProcessGetConfiguration(const TUsbcSetup& aPacket)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProcessGetConfiguration()"));
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateAddress)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Invalid device state"));
		return KErrGeneral;
		}
	if (iTrackDeviceState && iDeviceState == EUsbcDeviceStateAddress && iCurrentConfig != 0)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: DeviceState Address && Config != 0"));
		return KErrGeneral;
		}
	if (iTrackDeviceState && iDeviceState == EUsbcDeviceStateConfigured && iCurrentConfig == 0)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: DeviceState Configured && Config == 0"));
		return KErrGeneral;
		}
	if (aPacket.iLength != 1)								// "unspecified behavior"
		{
		__KTRACE_OPT(KUSB, Kern::Printf("  Warning: wLength != 1 (= %d)", aPacket.iLength));
		}
	__KTRACE_OPT(KUSB, Kern::Printf("  Reporting configuration value %d", iCurrentConfig));
	if (SetupEndpointZeroWrite(&iCurrentConfig, sizeof(iCurrentConfig)) == KErrNone)
		{
		iEp0WritePending = ETrue;
		}
	return KErrNone;
	}


/** Changes the device's configuration value, including interface setup and/or
	teardown and state change notification of higher-layer clients.
	May also be called by the PSL in special cases - therefore publishedPartner.

	@param aPacket The received Ep0 SET_CONFIGURATION setup request packet.
	@return KErrGeneral in case of a protocol error, KErrNone otherwise.

	@publishedPartner @released
*/
TInt DUsbClientController::ProcessSetConfiguration(const TUsbcSetup& aPacket)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProcessSetConfiguration()"));

	// This function may be called by the PSL from within an ISR -- so we have
	// to take care what we do here (and also in all functions that get called
	// from here).

	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateAddress)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Invalid device state"));
		return KErrGeneral;
		}
	const TUint16 value = aPacket.iValue;
	if (value > 1)											// we support only one configuration
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Configuration value too large: %d", value));
		return KErrGeneral;
		}

	__KTRACE_OPT(KUSB, Kern::Printf("  Configuration value: %d", value));
	ChangeConfiguration(value);

	// In 9.4.5 under GET_STATUS we read, that after SET_CONFIGURATION the HALT feature
	// for all endpoints is reset to zero.
	TInt num = 0;
	(TAny) DoForEveryEndpointInUse(&DUsbClientController::ClearHaltFeature, num);
	__KTRACE_OPT(KUSB, Kern::Printf("  Called ClearHaltFeature() for %d endpoints", num));
	SendEp0ZeroByteStatusPacket();							// success: zero bytes data during status stage
	return KErrNone;
	}


TInt DUsbClientController::ProcessGetInterface(const TUsbcSetup& aPacket)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProcessGetInterface()"));
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateConfigured)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Invalid device state"));
		return KErrGeneral;
		}
	if (iCurrentConfig == 0)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Device not configured"));
		return KErrGeneral;
		}
	const TInt number = aPacket.iIndex;
	if (!InterfaceExists(number))
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Bad interface index: %d", number));
		return KErrGeneral;
		}
	// Send alternate setting code of iCurrentInterface of Interface(set) <number> of the current
	// config (iCurrentConfig).
	const TUint8 setting = InterfaceNumber2InterfacePointer(number)->iCurrentInterface;
	__KTRACE_OPT(KUSB, Kern::Printf("  Reporting interface setting %d", setting));
	if (SetupEndpointZeroWrite(&setting, 1) == KErrNone)
		{
		iEp0WritePending = ETrue;
		}
	return KErrNone;
	}


TInt DUsbClientController::ProcessSetInterface(const TUsbcSetup& aPacket)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProcessSetInterface()"));
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateConfigured)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Invalid device state"));
		return KErrGeneral;
		}
	if (iCurrentConfig == 0)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Device not configured"));
		return KErrGeneral;
		}
	const TInt number = aPacket.iIndex;
	if (!InterfaceExists(number))
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Bad interface index: %d", number));
		return KErrGeneral;
		}
	const TInt setting = aPacket.iValue;
	TUsbcInterfaceSet* const ifcset_ptr = InterfaceNumber2InterfacePointer(number);
	RPointerArray<TUsbcInterface>& ifcs = ifcset_ptr->iInterfaces;
	if (setting >= ifcs.Count())
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Alt Setting >= bNumAltSettings: %d", setting));
		return KErrGeneral;
		}
	__KTRACE_OPT(KUSB, Kern::Printf("  Interface setting:: %d", setting));
	// Set iCurrentInterface of Interface(set) <number> of the current config
	// (iCurrentConfig) to alternate setting <setting>.
	ChangeInterface(ifcs[setting]);
	// In 9.4.5 under GET_STATUS we read, that after SET_INTERFACE the HALT feature
	// for all endpoints (of the now current interface setting) is reset to zero.
	RPointerArray<TUsbcLogicalEndpoint>& eps = ifcset_ptr->CurrentInterface()->iEndpoints;
	const TInt num_eps = eps.Count();
	for (TInt i = 0; i < num_eps; i++)
		{
		const TInt ep_num = EpAddr2Idx(eps[i]->iPEndpoint->iEndpointAddr);
		(TAny) ClearHaltFeature(ep_num);
		}
	SendEp0ZeroByteStatusPacket();							// success: zero bytes data during status stage
	return KErrNone;
	}


TInt DUsbClientController::ProcessSynchFrame(const TUsbcSetup& aPacket)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProcessSynchFrame()"));
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateConfigured)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Invalid device state"));
		return KErrGeneral;
		}
	const TInt ep = aPacket.iIndex;
	if (EndpointExists(ep) == EFalse)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Endpoint does not exist"));
		return KErrGeneral;
		}
	if (iRealEndpoints[EpAddr2Idx(ep)].iLEndpoint->iInfo.iType != KUsbEpTypeIsochronous)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Endpoint is not isochronous"));
		return KErrGeneral;
		}
	// We always send 0:
	*reinterpret_cast<TUint16*>(iEp0_TxBuf) = 0x00;
	if (SetupEndpointZeroWrite(iEp0_TxBuf, 2) == KErrNone)
		{
		iEp0WritePending = ETrue;
		}
	return KErrNone;
	}


#ifdef USB_SUPPORTS_SET_DESCRIPTOR_REQUEST
void DUsbClientController::ProceedSetDescriptor()
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ProceedSetDescriptor()"));
	// iEp0DataReceived already reflects the current buffer state
	if (iEp0DataReceived < iSetup.iLength)
		{
		// Not yet all data received => proceed
		return;
		}
	if (iEp0DataReceived > iSetup.iLength)
		{
		// Error: more data received than expected
		// but we don't care...
		}
	// at this point: iEp0DataReceived == iSetup.iLength
	const TUint8 type = HighByte(iSetup.iValue);
	if (type == KUsbDescType_String)
		{
		// set/add new string descriptor
		}
	else
		{
		// set/add new ordinary descriptor
		}
	TUint8 index = LowByte(iSetup.iValue);
	TUint16 langid = iSetup.iIndex;
	TUint16 length_total = iSetup.iLength;
	}
#endif


// --- Secondary (Helper) Functions

void DUsbClientController::SetClearHaltFeature(TInt aRealEndpoint, TUint8 aRequest)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::SetClearHaltFeature()"));
	if (aRequest == KUsbRequest_SetFeature)
		{
		if (iRealEndpoints[aRealEndpoint].iHalt)
			{
			// (This condition is not really an error)
			__KTRACE_OPT(KUSB, Kern::Printf("  Warning: HALT feature already set"));
			return;
			}
		__KTRACE_OPT(KUSB, Kern::Printf("  setting HALT feature for real endpoint %d",
										aRealEndpoint));
		StallEndpoint(aRealEndpoint);
		iRealEndpoints[aRealEndpoint].iHalt = ETrue;
		}
	else													// KUsbRequest_ClearFeature
		{
		if (iRealEndpoints[aRealEndpoint].iHalt == EFalse)
			{
			// In this case, before we return, the data toggles are reset to DATA0.
			__KTRACE_OPT(KUSB, Kern::Printf("  Warning: HALT feature already cleared"));
			ResetDataToggle(aRealEndpoint);
			return;
			}
		__KTRACE_OPT(KUSB, Kern::Printf("  clearing HALT feature for real endpoint %d",
										aRealEndpoint));
		ResetDataToggle(aRealEndpoint);
		ClearStallEndpoint(aRealEndpoint);
		iRealEndpoints[aRealEndpoint].iHalt = EFalse;
		}
	EpStatusNotify(aRealEndpoint);							// only called if actually something changed
	}


TInt DUsbClientController::ClearHaltFeature(TInt aRealEndpoint)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ClearHaltFeature()"));
	if (iRealEndpoints[aRealEndpoint].iHalt != EFalse)
		{
		ClearStallEndpoint(aRealEndpoint);
		iRealEndpoints[aRealEndpoint].iHalt = EFalse;
		}
	return KErrNone;
	}


void DUsbClientController::ChangeConfiguration(TUint16 aValue)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ChangeConfiguration()"));
	// New configuration is the same as the old one: 0
	if (iCurrentConfig == 0 && aValue == 0)
		{
		// no-op
		__KTRACE_OPT(KUSB, Kern::Printf("  Configuration: New == Old == 0 --> exiting"));
		return;
		}
	// New configuration is the same as the old one (but not 0)
	if (iCurrentConfig == aValue)
		{
		// no-op
		__KTRACE_OPT(KUSB, Kern::Printf("  Configuration: New == Old == %d --> exiting", aValue));
		return;
		}
	// Device is already configured
	if (iCurrentConfig != 0)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("  Device was configured: %d", iCurrentConfig));
		// Tear down all interface(set)s of the old configuration
		RPointerArray<TUsbcInterfaceSet>& ifcsets = CurrentConfig()->iInterfaceSets;
		for (TInt i = 0; i < ifcsets.Count(); ++i)
			{
			__KTRACE_OPT(KUSB, Kern::Printf("  Tearing down InterfaceSet %d", i));
			InterfaceSetTeardown(ifcsets[i]);
			}
		iCurrentConfig = 0;
		// Enter Address state (from Configured)
		if (iDeviceState == EUsbcDeviceStateConfigured)
			NextDeviceState(EUsbcDeviceStateAddress);
		}
	// Device gets a new configuration
	if (aValue != 0)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("  Device gets new configuration..."));
		// Setup all alternate settings 0 of all interfaces
		// (Don't separate the next two lines of code.)
		iCurrentConfig = aValue;
		RPointerArray<TUsbcInterfaceSet>& ifcsets = CurrentConfig()->iInterfaceSets;
		const TInt n = ifcsets.Count();
		for (TInt i = 0; i < n; ++i)
			{
			__KTRACE_OPT(KUSB, Kern::Printf("  Setting up InterfaceSet %d", i));
			InterfaceSetup(ifcsets[i]->iInterfaces[0]);
			}
		// Enter Configured state (from Address or Configured)
		NextDeviceState(EUsbcDeviceStateConfigured);
		}
	__KTRACE_OPT(KUSB, Kern::Printf("  New configuration: %d", iCurrentConfig));
	return;
	}


void DUsbClientController::InterfaceSetup(TUsbcInterface* aIfc)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::InterfaceSetup()"));
	const TInt num_eps = aIfc->iEndpoints.Count();
	for (TInt i = 0; i < num_eps; i++)
		{
		// Prepare this endpoint for I/O
		TUsbcLogicalEndpoint* const ep = aIfc->iEndpoints[i];
		// (TUsbcLogicalEndpoint's FS/HS endpoint sizes and interval values got
		//  adjusted in its constructor.)
		if (iHighSpeed)
			{
			__KTRACE_OPT(KUSB, Kern::Printf("  Setting Ep info size to %d (HS)", ep->iEpSize_Hs));
			ep->iInfo.iSize = ep->iEpSize_Hs;
			}
		else
			{
			__KTRACE_OPT(KUSB, Kern::Printf("  Setting Ep info size to %d (FS)", ep->iEpSize_Fs));
			ep->iInfo.iSize = ep->iEpSize_Fs;
			}
		const TInt idx = EpAddr2Idx(ep->iPEndpoint->iEndpointAddr);
		if (ConfigureEndpoint(idx, ep->iInfo) != KErrNone)
			{
			__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Endpoint %d configuration failed", idx));
			continue;
			}
		// Should there be a problem with it then we could try resetting the ep
		// data toggle at this point (or before the Configure) as well.
		__KTRACE_OPT(KUSB, Kern::Printf("  Connecting real ep addr 0x%02x & logical ep #%d",
										ep->iPEndpoint->iEndpointAddr, ep->iLEndpointNum));
		ep->iPEndpoint->iLEndpoint = ep;
		}
	aIfc->iInterfaceSet->iCurrentInterface = aIfc->iSettingCode;
	return;
	}


void DUsbClientController::InterfaceSetTeardown(TUsbcInterfaceSet* aIfcSet)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::InterfaceSetTeardown()"));
	if (aIfcSet->iInterfaces.Count() == 0)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("  No interfaces exist - returning"));
		return;
		}
	RPointerArray<TUsbcLogicalEndpoint>& eps = aIfcSet->CurrentInterface()->iEndpoints;
	const TInt num_eps = eps.Count();
	for (TInt i = 0; i < num_eps; i++)
		{
		TUsbcLogicalEndpoint* const ep = eps[i];
		const TInt idx = EpAddr2Idx(ep->iPEndpoint->iEndpointAddr);

		CancelTransferRequests(idx);

		if (!ep->iPEndpoint->iLEndpoint)
			{
			__KTRACE_OPT(KUSB, Kern::Printf("  real ep %d not configured: skipping", idx));
			continue;
			}
		if (ResetDataToggle(idx) != KErrNone)
			{
			__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Endpoint %d data toggle reset failed", idx));
			}
		if (DeConfigureEndpoint(idx) != KErrNone)
			{
			__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Endpoint %d de-configuration failed", idx));
			}

		__KTRACE_OPT(KUSB, Kern::Printf("  disconnecting real ep & logical ep"));
		ep->iPEndpoint->iLEndpoint = NULL;
		}
	if (aIfcSet->CurrentInterface() != 0)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("  Resetting alternate interface setting to 0"));
		//Add this mutex to protect the interface set data structure
		if (NKern::CurrentContext() == EThread)
		    {
            NKern::FMWait(&iMutex);
		    }
        
		aIfcSet->iCurrentInterface = 0;
	    if (NKern::CurrentContext() == EThread)
	        {
            NKern::FMSignal(&iMutex);
	        }		
		}
	return;
	}


void DUsbClientController::ChangeInterface(TUsbcInterface* aIfc)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ChangeInterface()"));
	TUsbcInterfaceSet* ifcset = aIfc->iInterfaceSet;
	const TUint8 setting = aIfc->iSettingCode;
	if (ifcset->iCurrentInterface == setting)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("  New Ifc == old Ifc: nothing to do"));
		return;
		}
	__KTRACE_OPT(KUSB, Kern::Printf("  Setting new interface setting #%d", setting));
	InterfaceSetTeardown(ifcset);
	InterfaceSetup(aIfc);
	StatusNotify(static_cast<TUsbcDeviceState>(KUsbAlternateSetting | setting), ifcset->iClientId);
	}


// aFunction gets called, successively, with the endpoint index of every ep in-use as its argument.
// (BTW: The declaration "type (class::*name)(params)" makes <name> a "pointer to element function".)
//
TInt DUsbClientController::DoForEveryEndpointInUse(TInt (DUsbClientController::*aFunction)(TInt), TInt& aCount)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::DoForEveryEndpointInUse()"));
	aCount = 0;
	TUsbcConfiguration* const config = CurrentConfig();
	if (!config)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("  Device is not configured - returning"));
		return KErrNone;
		}
	RPointerArray<TUsbcInterfaceSet>& ifcsets = config->iInterfaceSets;
	const TInt num_ifcsets = ifcsets.Count();
	for (TInt i = 0; i < num_ifcsets; i++)
		{
		RPointerArray<TUsbcLogicalEndpoint>& eps = ifcsets[i]->CurrentInterface()->iEndpoints;
		const TInt num_eps = eps.Count();
		for (TInt j = 0; j < num_eps; j++)
			{
			const TInt ep_num = EpAddr2Idx(eps[j]->iPEndpoint->iEndpointAddr);
			const TInt result = (this->*aFunction)(ep_num);
			++aCount;
			if (result != KErrNone)
				{
				return result;
				}
			}
		}
	return KErrNone;
	}


// -eof-
