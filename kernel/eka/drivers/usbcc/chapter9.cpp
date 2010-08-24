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
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "chapter9Traces.h"
#endif



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
    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_ENQUIREEP0NEXTSTATE,
            "DUsbClientController::EnquireEp0NextState()" );

	// This function may be called by the PSL from within an ISR -- so we have
	// to take care what we do here (and also in all functions that get called
	// from here).

	if (SWAP_BYTES_16((reinterpret_cast<const TUint16*>(aSetupBuf)[3])) == 0) // iLength
		{
	    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_ENQUIREEP0NEXTSTATE_DUP1,
	            "  --> EEp0StateStatusIn" );

		return EEp0StateStatusIn;							// No-data Control => Status_IN
		}
	else if ((aSetupBuf[0] & KUsbRequestType_DirMask) == KUsbRequestType_DirToDev)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_ENQUIREEP0NEXTSTATE_DUP2,
		        "  --> EEp0StateDataOut" );
		return EEp0StateDataOut;							// Control Write => Data_OUT
		}
	else
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_ENQUIREEP0NEXTSTATE_DUP3,
		        "  --> EEp0StateDataIn" );
		return EEp0StateDataIn;								// Control Read => Data_IN
		}
	}


TInt DUsbClientController::ProcessEp0ReceiveDone(TInt aCount)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0RECEIVEDONE,
	        "DUsbClientController::ProcessEp0ReceiveDone()" );
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
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0TRANSMITDONE,
	        "DUsbClientController::ProcessEp0TransmitDone()" );
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
	    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSEP0TRANSMITDONE_DUP1,
		        "  Error: DUsbClientController::ProcessEpTransmitDone: Stalling Ep0" );
		StallEndpoint(KEp0_In);								// request not found
		return KErrNotFound;
		}
	// If _we_ sent the data, we simply do nothing here...
	return KErrNone;
	}

TInt DUsbClientController::ProcessEp0SetupReceived(TInt aCount)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED, 
	        "DUsbClientController::ProcessEp0SetupReceived()" );

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
	OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP1, 
	        "  bmRequestType = 0x%02x", packet.iRequestType );
	if ((packet.iRequestType & KUsbRequestType_TypeMask) == KUsbRequestType_TypeStd)
		{
		switch (packet.iRequest)
			{
		case KUsbRequest_GetStatus:
		    OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP2, 
		            "  bRequest      = 0x%02x (GET_STATUS)", KUsbRequest_GetStatus );
			break;
		case KUsbRequest_ClearFeature:
		    OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP3, 
		            "  bRequest      = 0x%02x (CLEAR_FEATURE)", KUsbRequest_ClearFeature );
			break;
		case KUsbRequest_SetFeature:
		    OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP4, 
		            "  bRequest      = 0x%02x (SET_FEATURE)", KUsbRequest_SetFeature );
			break;
		case KUsbRequest_SetAddress:
		    OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP5, 
		            "  bRequest      = 0x%02x (SET_ADDRESS)", KUsbRequest_SetAddress );
			break;
		case KUsbRequest_GetDescriptor:
		    OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP6, 
		            "  bRequest      = 0x%02x (GET_DESCRIPTOR)", KUsbRequest_GetDescriptor );
			break;
		case KUsbRequest_SetDescriptor:
		    OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP7, 
		            "  bRequest      = 0x%02x (SET_DESCRIPTOR)", KUsbRequest_SetDescriptor );
			break;
		case KUsbRequest_GetConfig:
		    OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP8, 
		            "  bRequest      = 0x%02x (GET_CONFIGURATION)", KUsbRequest_GetConfig );
			break;
		case KUsbRequest_SetConfig:
		    OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP9, 
		            "  bRequest      = 0x%02x (SET_CONFIGURATION)", KUsbRequest_SetConfig );
			break;
		case KUsbRequest_GetInterface:
		    OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP10, 
		            "  bRequest      = 0x%02x (GET_INTERFACE)", KUsbRequest_GetInterface );
			break;
		case KUsbRequest_SetInterface:
		    OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP11, 
		            "  bRequest      = 0x%02x (SET_INTERFACE)", KUsbRequest_SetInterface );
			break;
		case KUsbRequest_SynchFrame:
		    OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP12, 
		            "  bRequest      = 0x%02x (SYNCH_FRAME)", KUsbRequest_SynchFrame );
			break;
		default:
		    OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP13, 
		            "  Error: bRequest = 0x%02x (UNKNWON STANDARD REQUEST)", packet.iRequest );
			break;
			}
		}
	else
		{
        OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP14, 
                "  bRequest      = 0x%02x (NON-STANDARD REQUEST)", packet.iRequest );
		}
	OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP15, 
	        "  wValue        = 0x%04x", packet.iValue );
	OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP16, 
	        "  wIndex        = 0x%04x", packet.iIndex );
	OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP17, 
	        "  wLength       = 0x%04x", packet.iLength );
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
			    if (ProcessGetDeviceStatus(packet) != KErrNone)
			        {
                    OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP18, 
                            "ProcessEp0SetupReceived: Stalling Ep0" );
                    StallEndpoint(KEp0_In);
                    }
				break;
			case KUsbRequestType_DestIfc:
                if (ProcessGetInterfaceStatus(packet) != KErrNone)
                    {
                    OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP19, 
                            "ProcessEp0SetupReceived: Stalling Ep0" );
                    StallEndpoint(KEp0_In);
                    }
				break;
			case KUsbRequestType_DestEp:
                if (ProcessGetEndpointStatus(packet) != KErrNone)
                    {
                    OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP20, 
                            "ProcessEp0SetupReceived: Stalling Ep0" );
                    StallEndpoint(KEp0_In);
                    }
				break;
			default:
			    OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP21, 
			            "  Error: GET STATUS - Other or Unknown recipient" );
			    OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP22, 
			            "  -> DUsbClientController::ProcessEp0SetupReceived: Stalling Ep0" );
				StallEndpoint(KEp0_In);
				break;
				}
			break;
		case KUsbRequest_ClearFeature:
		case KUsbRequest_SetFeature:
			switch (packet.iRequestType & KUsbRequestType_DestMask)
				{ // Recipient
			case KUsbRequestType_DestDevice:
                if (ProcessSetClearDevFeature(packet) != KErrNone)
                    {
                    OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP23, 
                            "ProcessEp0SetupReceived: Stalling Ep0" );
                    StallEndpoint(KEp0_In);
                    }
				break;
			case KUsbRequestType_DestIfc:
                if (ProcessSetClearIfcFeature(packet) != KErrNone)
                    {
                    OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP24, 
                            "ProcessEp0SetupReceived: Stalling Ep0" );
                    StallEndpoint(KEp0_In);
                    }
				break;
			case KUsbRequestType_DestEp:
                if (ProcessSetClearEpFeature(packet) != KErrNone)
                    {
                    OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP25, 
                            "ProcessEp0SetupReceived: Stalling Ep0" );
                    StallEndpoint(KEp0_In);
                    }
				break;
			default:
			    OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP26, 
			            "  Error: SET/CLEAR FEATURE - Other or Unknown recipient" );
			    OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP27, 
			            "  -> Stalling Ep0" );
				StallEndpoint(KEp0_In);
				break;
				}
			break;
		case KUsbRequest_SetAddress:
            if (ProcessSetAddress(packet) != KErrNone)
                {
                OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP28, 
                        "ProcessEp0SetupReceived: Stalling Ep0" );
                StallEndpoint(KEp0_In);
                }
			break;
		case KUsbRequest_GetDescriptor:
            if (ProcessGetDescriptor(packet) != KErrNone)
                {
                OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP29, 
                        "ProcessEp0SetupReceived: Stalling Ep0" );
                StallEndpoint(KEp0_In);
                }
			break;
		case KUsbRequest_SetDescriptor:
            if (ProcessSetDescriptor(packet) != KErrNone)
                {
                OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP30, 
                        "ProcessEp0SetupReceived: Stalling Ep0" );
                StallEndpoint(KEp0_In);
                }
			break;
		case KUsbRequest_GetConfig:
            if (ProcessGetConfiguration(packet) != KErrNone)
                {
                OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP31, 
                        "ProcessEp0SetupReceived: Stalling Ep0" );
                StallEndpoint(KEp0_In);
                }
			break;
		case KUsbRequest_SetConfig:
            if (ProcessSetConfiguration(packet) != KErrNone)
                {
                OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP32, 
                        "ProcessEp0SetupReceived: Stalling Ep0" );
                StallEndpoint(KEp0_In);
                }
			break;
		case KUsbRequest_GetInterface:
            if (ProcessGetInterface(packet) != KErrNone)
                {
                OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP33, 
                        "ProcessEp0SetupReceived: Stalling Ep0" );
                StallEndpoint(KEp0_In);
                }
			break;
		case KUsbRequest_SetInterface:
            if (ProcessSetInterface(packet) != KErrNone)
                {
                OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP34, 
                        "ProcessEp0SetupReceived: Stalling Ep0" );
                StallEndpoint(KEp0_In);
                }
			break;
		case KUsbRequest_SynchFrame:
            if (ProcessSynchFrame(packet) != KErrNone)
                {
                OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP35, 
                        "ProcessEp0SetupReceived: Stalling Ep0" );
                StallEndpoint(KEp0_In);
                }
			break;
		default:
		    OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP36, 
		            "  Error: Unknown/unsupported Std Setup Request" );
		    OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP37, 
		            "  -> Stalling Ep0" );
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
                OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP38, 
                        "  Error: Invalid device state" );
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
                        OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP39, 
                                "  Recipient says: NoEp0RequestsPlease" );
						}
					else
						{
						client = ifcset_ptr->iClientId;
						}
					}
				else
					{
                    OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP40, 
                            "  Error: Interface 0x%02x does not exist", packet.iIndex );
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
                OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP41, 
                        "  Error: Invalid device state" );
				}
			else if (EndpointExists(packet.iIndex) == EFalse)
				{
                OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP42, 
                        "  Error: Endpoint 0x%02x does not exist", packet.iIndex );
				}
			else
				{
				const TInt idx = EpAddr2Idx(packet.iIndex);
				const TUsbcInterfaceSet* const ifcset_ptr =
					iRealEndpoints[idx].iLEndpoint->iInterface->iInterfaceSet;
				if (ifcset_ptr->CurrentInterface()->iNoEp0Requests)
					{
                    OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP43, 
                            "  Recipient says: NoEp0RequestsPlease" );
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
		    OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP44, 
		            "  Error: Other or Unknown recipient" );
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
					OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP45, 
					        "  Found Ep0 read request" );
					if (packet.iLength != 0)
						{
						if ((packet.iRequestType & KUsbRequestType_DirMask) == KUsbRequestType_DirToDev)
							{
							// Data transfer & direction OUT => there'll be a DATA_OUT stage
                            OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP46, 
                                    "  Next is DATA_OUT: setting up DataOutVars" );
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
			OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP47, 
			        "  Ep0 read request not found: setting RxExtra vars (Setup)" );
			iEp0_RxExtraCount = aCount;
			iEp0_RxExtraData = ETrue;
			return KErrNotFound;
			}
		else // if (client == NULL)
			{
            OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSEP0SETUPRECEIVED_DUP48, 
                    "  Ep0 request error: Stalling Ep0" );
			StallEndpoint(KEp0_In);
			return KErrGeneral;
			}
		}
	return KErrNone;
	}

TInt DUsbClientController::ProcessEp0DataReceived(TInt aCount)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_PROCESSEP0DATARECEIVED, 
	        "DUsbClientController::ProcessEp0DataReceived()" );
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0DATARECEIVED_DUP1, 
	        "  : %d bytes", aCount );
	if (aCount > iEp0MaxPacketSize)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSEP0DATARECEIVED_DUP2, 
                "  Error: Too much data" );
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
            OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSEP0DATARECEIVED_DUP3, 
                    "  Error: invalid request in iSetup" );
            OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSEP0DATARECEIVED_DUP4, 
                    "  -> DUsbClientController::ProcessEp0DataReceived: Stalling Ep0" );
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
	            OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0DATARECEIVED_DUP5,
	                    "  Found Ep0 read request" );
				memcpy(p->iBufferStart, iEp0_RxBuf, aCount);
				p->iError = KErrNone;						// if it wasn't 'KErrNone' we wouldn't be here
				*(p->iPacketSize) = aCount;
				p->iRxPackets = 1;
				*(p->iPacketIndex) = 0;
				ProcessDataTransferDone(*p);
				goto found;
				}
			}
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSEP0DATARECEIVED_DUP6,
                "  Ep0 read request not found: setting RxExtra vars (Data)" );
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
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_PROCESSGETDEVICESTATUS, 
	        "DUsbClientController::ProcessGetDeviceStatus()" );
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateAddress)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSGETDEVICESTATUS_DUP1, 
                "  Error: Invalid device state" );
		return KErrGeneral;
		}
	const TUint16 status = ((DeviceSelfPowered() ? KUsbDevStat_SelfPowered : 0) |
					  (iRmWakeupStatus_Enabled ? KUsbDevStat_RemoteWakeup : 0));
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSGETDEVICESTATUS_DUP2, 
	        "  Reporting device status: 0x%02x", status );
	*reinterpret_cast<TUint16*>(iEp0_TxBuf) = SWAP_BYTES_16(status);
	if (SetupEndpointZeroWrite(iEp0_TxBuf, sizeof(status)) == KErrNone)
		{
		iEp0WritePending = ETrue;
		}
	return KErrNone;
	}


TInt DUsbClientController::ProcessGetInterfaceStatus(const TUsbcSetup& aPacket)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_PROCESSGETINTERFACESTATUS, 
	        "DUsbClientController::ProcessGetInterfaceStatus()" );
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateConfigured)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSGETINTERFACESTATUS_DUP1, 
                "  Error: Invalid device state" );
		return KErrGeneral;
		}
	if (InterfaceExists(aPacket.iIndex) == EFalse)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSGETINTERFACESTATUS_DUP2, 
                "  Error: Interface does not exist" );
		return KErrGeneral;
		}
	const TUint16 status = 0x0000;							// as of USB Spec 2.0
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSGETINTERFACESTATUS_DUP3,
            "  Reporting interface status: 0x%02x", status );
	*reinterpret_cast<TUint16*>(iEp0_TxBuf) = SWAP_BYTES_16(status);
	if (SetupEndpointZeroWrite(iEp0_TxBuf, sizeof(status)) == KErrNone)
		{
		iEp0WritePending = ETrue;
		}
	return KErrNone;
	}


TInt DUsbClientController::ProcessGetEndpointStatus(const TUsbcSetup& aPacket)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_PROCESSGETENDPOINTSTATUS, 
	        "DUsbClientController::ProcessGetEndpointStatus()" );
	if (iTrackDeviceState &&
		((iDeviceState < EUsbcDeviceStateAddress) ||
		 (iDeviceState == EUsbcDeviceStateAddress && (aPacket.iIndex & KUsbEpAddress_Portmask) != 0)))
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSGETENDPOINTSTATUS_DUP1, 
                "  Error: Invalid device state" );
		return KErrGeneral;
		}
	if (EndpointExists(aPacket.iIndex) == EFalse)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSGETENDPOINTSTATUS_DUP2, 
                "  Error: Endpoint does not exist" );
		return KErrGeneral;
		}
	const TInt ep = EpAddr2Idx(aPacket.iIndex);
	const TUint16 status = (iRealEndpoints[ep].iHalt) ?	 KUsbEpStat_Halt : 0;
	OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSGETENDPOINTSTATUS_DUP3, 
	        "  Reporting endpoint status 0x%02x for real endpoint %d", status, ep );
	*reinterpret_cast<TUint16*>(iEp0_TxBuf) = SWAP_BYTES_16(status);
	if (SetupEndpointZeroWrite(iEp0_TxBuf, 2) == KErrNone)
		{
		iEp0WritePending = ETrue;
		}
	return KErrNone;
	}


TInt DUsbClientController::ProcessSetClearDevFeature(const TUsbcSetup& aPacket)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_PROCESSSETCLEARDEVFEATURE,
	        "DUsbClientController::ProcessSetClearDevFeature()" );
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateDefault)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEARDEVFEATURE_DUP1,
                "  Error: Invalid device state" );
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
                OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEARDEVFEATURE_DUP2,
                        "  Error: Invalid device state" );
				return KErrGeneral;
				}
			iRmWakeupStatus_Enabled = ETrue;
			break;
		case KUsbFeature_TestMode:
			if (!iHighSpeed)
				{
                OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEARDEVFEATURE_DUP3,
                        "  Error: Request only supported in High-Speed mode" );
				return KErrGeneral;
				}
			if (LowByte(aPacket.iIndex) != 0)
				{
                OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEARDEVFEATURE_DUP4,
                        "  Error: Lower byte of wIndex must be zero" );
				return KErrGeneral;
				}
			test_sel = HighByte(aPacket.iIndex);
			if ((test_sel < KUsbTestSelector_Test_J) || (test_sel > KUsbTestSelector_Test_Force_Enable))
				{
                OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEARDEVFEATURE_DUP5,
                        "  Error: Invalid test selector: %d", test_sel );
				return KErrGeneral;
				}
			break;
		case KUsbFeature_B_HnpEnable:
			if (!iOtgSupport)
				{
                OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEARDEVFEATURE_DUP6,
                        "  Error: Request only supported on a OTG device" );
				return KErrGeneral;
				}
			if (!(iOtgFuncMap & KUsbOtgAttr_HnpSupp))
				{
                OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEARDEVFEATURE_DUP7,
                        "  Error: Request only valid if OTG device supports HNP" );
				return KErrGeneral;
				}
			iOtgFuncMap |= KUsbOtgAttr_B_HnpEnable;
			OtgFeaturesNotify();
			break;
		case KUsbFeature_A_HnpSupport:
			if (!iOtgSupport)
				{
                OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEARDEVFEATURE_DUP8,
                        "  Error: Request only supported on a OTG device" );
				return KErrGeneral;
				}
			if (!(iOtgFuncMap & KUsbOtgAttr_HnpSupp))
				{
                OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEARDEVFEATURE_DUP9,
                        "  Error: Request only valid if OTG device supports HNP" );
				return KErrGeneral;
				}
			iOtgFuncMap |= KUsbOtgAttr_A_HnpSupport;
			OtgFeaturesNotify();
			break;
		case KUsbFeature_A_AltHnpSupport:
			if (!iOtgSupport)
				{
                OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEARDEVFEATURE_DUP10,
                        "  Error: Request only supported on a OTG device" );
				return KErrGeneral;
				}
			if (!(iOtgFuncMap & KUsbOtgAttr_HnpSupp))
				{
                OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEARDEVFEATURE_DUP11,
                        "  Error: Request only valid if OTG device supports HNP" );
				return KErrGeneral;
				}
			iOtgFuncMap |= KUsbOtgAttr_A_AltHnpSupport;
			OtgFeaturesNotify();
			break;
		default:
		    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEARDEVFEATURE_DUP12,
		            "  Error: Unknown feature requested" );
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
                OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEARDEVFEATURE_DUP13,
                        "  Error: Invalid device state" );
				return KErrGeneral;
				}
			iRmWakeupStatus_Enabled = EFalse;
			break;
		default:
		    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEARDEVFEATURE_DUP14,
		            "  Error: Unknown feature requested" );
			return KErrGeneral;
			}
		}

	SendEp0ZeroByteStatusPacket();							// success: zero bytes data during status stage

	// 9.4.9: "The transition to test mode of an upstream facing port must not happen until
	// after the status stage of the request."
	if (test_sel)
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEARDEVFEATURE_DUP15,
                "  Entering HS Test Mode %d", test_sel );
		EnterTestMode(test_sel);
		}

	return KErrNone;
	}


TInt DUsbClientController::ProcessSetClearIfcFeature(const TUsbcSetup& aPacket)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_PROCESSSETCLEARIFCFEATURE,
	        "DUsbClientController::ProcessSetClearIfcFeature()" );
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateConfigured)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEARIFCFEATURE_DUP1,
                "  Error: Invalid device state" );
		return KErrGeneral;
		}
	// No interface features defined in USB spec, thus
	return KErrGeneral;
	}


TInt DUsbClientController::ProcessSetClearEpFeature(const TUsbcSetup& aPacket)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_PROCESSSETCLEAREPFEATURE, 
	        "DUsbClientController::ProcessSetClearEpFeature()" );
	if (iTrackDeviceState &&
		((iDeviceState < EUsbcDeviceStateAddress) ||
		 (iDeviceState == EUsbcDeviceStateAddress && (aPacket.iIndex & KUsbEpAddress_Portmask) != 0)))
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEAREPFEATURE_DUP1, 
                "  Error: Invalid device state" );
		return KErrGeneral;
		}
	if (aPacket.iValue != KUsbFeature_EndpointHalt)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEAREPFEATURE_DUP2, 
                "  Error: Unknown feature requested" );
		return KErrGeneral;
		}
	if (EndpointExists(aPacket.iIndex) == EFalse)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEAREPFEATURE_DUP3, 
                "  Error: Endpoint does not exist" );
		return KErrGeneral;
		}
	const TInt ep = EpAddr2Idx(aPacket.iIndex);
	if (iRealEndpoints[ep].iLEndpoint->iInfo.iType == KUsbEpTypeControl ||
		iRealEndpoints[ep].iLEndpoint->iInfo.iType == KUsbEpTypeIsochronous)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCLEAREPFEATURE_DUP4, 
                "  Error: Endpoint is Control or Isochronous" );
		return KErrGeneral;
		}
	SetClearHaltFeature(ep, aPacket.iRequest);
	SendEp0ZeroByteStatusPacket();							// success: zero bytes data during status stage
	return KErrNone;
	}


TInt DUsbClientController::ProcessSetAddress(const TUsbcSetup& aPacket)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_PROCESSSETADDRESS,
	        "DUsbClientController::ProcessSetAddress()" );
	if (iTrackDeviceState && iDeviceState > EUsbcDeviceStateAddress)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETADDRESS_DUP1,
                "  Error: Invalid device state" );
		return KErrGeneral;
		}
	const TUint16 addr = aPacket.iValue;
	if (addr > 127)
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETADDRESS_DUP2,
                "  Error: Bad address value: %d (>127)", addr );
		return KErrGeneral;
		}
	if (addr == 0)
		{
		// Enter Default state (from Default or Address)
		NextDeviceState(EUsbcDeviceStateDefault);
		}
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSSETADDRESS_DUP3,
	        "  USB address: %d", addr );
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
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_PROCESSGETDESCRIPTOR, 
	        "DUsbClientController::ProcessGetDescriptor()" );
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateDefault)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSGETDESCRIPTOR_DUP1, 
                "  Error: Invalid device state" );
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
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSGETDESCRIPTOR_DUP2, 
                "  Couldn't retrieve descriptor" );
		return KErrGeneral;
		}

    OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSGETDESCRIPTOR_DUP3,
            "  Descriptor found, size: %d (requested: %d)", size, aPacket.iLength );
	if (size > KUsbcBufSz_Ep0Tx)
		{
		// This should actually not be possible (i.e. we should never get here).
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSGETDESCRIPTOR_DUP4,
                "  Error: Ep0_Tx buffer too small" );
		}
	if (size > aPacket.iLength)
		{
		// Send only as much data as requested by the host
		size = aPacket.iLength;
		}

#ifdef ENABLE_EXCESSIVE_DEBUG_OUTPUT
    OstTraceDefExt1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSGETDESCRIPTOR_DUP5,
            "  Data: %{uint8[]}", TOstArray<TUint8>(iEp0_TxBuf, 8) );
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
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_PROCESSSETDESCRIPTOR, 
	        "DUsbClientController::ProcessSetDescriptor()" );
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
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETDESCRIPTOR_DUP1,
	            "  Error: Ep0_Rx buffer too small" );
		return KErrGeneral;
		}
	SetEp0DataOutVars(aPacket);
	SetupEndpointZeroRead();
	return KErrNone;
#endif
	}


TInt DUsbClientController::ProcessGetConfiguration(const TUsbcSetup& aPacket)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_PROCESSGETCONFIGURATION, 
	        "DUsbClientController::ProcessGetConfiguration()" );
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateAddress)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSGETCONFIGURATION_DUP1, 
                "  Error: Invalid device state" );
		return KErrGeneral;
		}
	if (iTrackDeviceState && iDeviceState == EUsbcDeviceStateAddress && iCurrentConfig != 0)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSGETCONFIGURATION_DUP2, 
                "  Error: DeviceState Address && Config != 0" );
		return KErrGeneral;
		}
	if (iTrackDeviceState && iDeviceState == EUsbcDeviceStateConfigured && iCurrentConfig == 0)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSGETCONFIGURATION_DUP3, 
                "  Error: DeviceState Configured && Config == 0" );
		return KErrGeneral;
		}
	if (aPacket.iLength != 1)								// "unspecified behavior"
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSGETCONFIGURATION_DUP4, 
                "  Warning: wLength != 1 (= %d)", aPacket.iLength );
		}
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSGETCONFIGURATION_DUP5, 
	        "  Reporting configuration value %d", iCurrentConfig );
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
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_PROCESSSETCONFIGURATION, 
	        "DUsbClientController::ProcessSetConfiguration()" );
	// This function may be called by the PSL from within an ISR -- so we have
	// to take care what we do here (and also in all functions that get called
	// from here).

	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateAddress)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCONFIGURATION_DUP1, 
                "  Error: Invalid device state" );
		return KErrGeneral;
		}
	const TUint16 value = aPacket.iValue;
	if (value > 1)											// we support only one configuration
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETCONFIGURATION_DUP2, 
                "  Error: Configuration value too large: %d", value );
		return KErrGeneral;
		}

    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSSETCONFIGURATION_DUP3,
            "  Configuration value: %d", value );
	ChangeConfiguration(value);

	// In 9.4.5 under GET_STATUS we read, that after SET_CONFIGURATION the HALT feature
	// for all endpoints is reset to zero.
	TInt num = 0;
	(TAny) DoForEveryEndpointInUse(&DUsbClientController::ClearHaltFeature, num);
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSSETCONFIGURATION_DUP4,
	        "  Called ClearHaltFeature() for %d endpoints", num );
	SendEp0ZeroByteStatusPacket();							// success: zero bytes data during status stage
	return KErrNone;
	}


TInt DUsbClientController::ProcessGetInterface(const TUsbcSetup& aPacket)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_PROCESSGETINTERFACE, 
	        "DUsbClientController::ProcessGetInterface()" );
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateConfigured)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSGETINTERFACE_DUP1, 
                "  Error: Invalid device state" );
		return KErrGeneral;
		}
	if (iCurrentConfig == 0)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSGETINTERFACE_DUP2, 
                "  Error: Device not configured" );
		return KErrGeneral;
		}
	const TInt number = aPacket.iIndex;
	if (!InterfaceExists(number))
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSGETINTERFACE_DUP3, 
                "  Error: Bad interface index: %d", number );
		return KErrGeneral;
		}
	// Send alternate setting code of iCurrentInterface of Interface(set) <number> of the current
	// config (iCurrentConfig).
	const TUint8 setting = InterfaceNumber2InterfacePointer(number)->iCurrentInterface;
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSGETINTERFACE_DUP4, 
	        "  Reporting interface setting %d", setting );
	if (SetupEndpointZeroWrite(&setting, 1) == KErrNone)
		{
		iEp0WritePending = ETrue;
		}
	return KErrNone;
	}


TInt DUsbClientController::ProcessSetInterface(const TUsbcSetup& aPacket)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_PROCESSSETINTERFACE,
	        "DUsbClientController::ProcessSetInterface()" );
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateConfigured)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETINTERFACE_DUP1,
                "  Error: Invalid device state" );
		return KErrGeneral;
		}
	if (iCurrentConfig == 0)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETINTERFACE_DUP2,
                "  Error: Device not configured" );
		return KErrGeneral;
		}
	const TInt number = aPacket.iIndex;
	if (!InterfaceExists(number))
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETINTERFACE_DUP3,
                "  Error: Bad interface index: %d", number );
		return KErrGeneral;
		}
	const TInt setting = aPacket.iValue;
	TUsbcInterfaceSet* const ifcset_ptr = InterfaceNumber2InterfacePointer(number);
	RPointerArray<TUsbcInterface>& ifcs = ifcset_ptr->iInterfaces;
	if (setting >= ifcs.Count())
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSETINTERFACE_DUP4,
                "  Error: Alt Setting >= bNumAltSettings: %d", setting );
		return KErrGeneral;
		}
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_PROCESSSETINTERFACE_DUP5,
            "  Interface setting:: %d", setting );
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
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_PROCESSSYNCHFRAME, 
	        "DUsbClientController::ProcessSynchFrame()" );
	if (iTrackDeviceState && iDeviceState < EUsbcDeviceStateConfigured)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSYNCHFRAME_DUP1, 
                "  Error: Invalid device state" );
		return KErrGeneral;
		}
	const TInt ep = aPacket.iIndex;
	if (EndpointExists(ep) == EFalse)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSYNCHFRAME_DUP2, 
                "  Error: Endpoint does not exist" );
		return KErrGeneral;
		}
	if (iRealEndpoints[EpAddr2Idx(ep)].iLEndpoint->iInfo.iType != KUsbEpTypeIsochronous)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_PROCESSSYNCHFRAME_DUP3, 
                "  Error: Endpoint is not isochronous" );
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
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_PROCEEDSETDESCRIPTOR,
	        "DUsbClientController::ProceedSetDescriptor()" );
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
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_SETCLEARHALTFEATURE, 
	        "DUsbClientController::SetClearHaltFeature()" );
	if (aRequest == KUsbRequest_SetFeature)
		{
		if (iRealEndpoints[aRealEndpoint].iHalt)
			{
			// (This condition is not really an error)
            OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_SETCLEARHALTFEATURE_DUP1, 
                    "  Warning: HALT feature already set" );
			return;
			}
		OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_SETCLEARHALTFEATURE_DUP2, 
		        "  setting HALT feature for real endpoint %d", aRealEndpoint );
		StallEndpoint(aRealEndpoint);
		iRealEndpoints[aRealEndpoint].iHalt = ETrue;
		}
	else													// KUsbRequest_ClearFeature
		{
		if (iRealEndpoints[aRealEndpoint].iHalt == EFalse)
			{
			// In this case, before we return, the data toggles are reset to DATA0.
            OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_SETCLEARHALTFEATURE_DUP3, 
                    "  Warning: HALT feature already cleared" );
			ResetDataToggle(aRealEndpoint);
			return;
			}
		OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_SETCLEARHALTFEATURE_DUP4, 
		        "  clearing HALT feature for real endpoint %d", aRealEndpoint );
		ResetDataToggle(aRealEndpoint);
		ClearStallEndpoint(aRealEndpoint);
		iRealEndpoints[aRealEndpoint].iHalt = EFalse;
		}
	EpStatusNotify(aRealEndpoint);							// only called if actually something changed
	}


TInt DUsbClientController::ClearHaltFeature(TInt aRealEndpoint)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_CLEARHALTFEATURE, 
	        "DUsbClientController::ClearHaltFeature()" );
	if (iRealEndpoints[aRealEndpoint].iHalt != EFalse)
		{
		ClearStallEndpoint(aRealEndpoint);
		iRealEndpoints[aRealEndpoint].iHalt = EFalse;
		}
	return KErrNone;
	}


void DUsbClientController::ChangeConfiguration(TUint16 aValue)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_CHANGECONFIGURATION, 
	        "DUsbClientController::ChangeConfiguration()" );
	// New configuration is the same as the old one: 0
	if (iCurrentConfig == 0 && aValue == 0)
		{
		// no-op
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_CHANGECONFIGURATION_DUP1, 
                "  Configuration: New == Old == 0 --> exiting" );
		return;
		}
	// New configuration is the same as the old one (but not 0)
	if (iCurrentConfig == aValue)
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_CHANGECONFIGURATION_DUP2, 
                "  Configuration: New == Old == %d --> exiting", aValue );
		// From the spec 9.1.1.5, Data toggle is reset to zero here when 
		// setconfiguration(x->x)(x!=0) received, although we only support
		// single configuration currently.
		TInt num = 0;
		TInt ret = DoForEveryEndpointInUse(&DUsbClientController::ResetDataToggle, num);
		if(ret != KErrNone)
			{
            OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_CHANGECONFIGURATION_DUP3, 
                    "  Error: Endpoint data toggle reset failed" );
			}
		OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_CHANGECONFIGURATION_DUP4, 
		        "  Called ResetDataToggle()for %d endpoints", num );	
		return;
		}
	// Device is already configured
	if (iCurrentConfig != 0)
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_CHANGECONFIGURATION_DUP5, 
                "  Device was configured: %d", iCurrentConfig );
		// Tear down all interface(set)s of the old configuration
		RPointerArray<TUsbcInterfaceSet>& ifcsets = CurrentConfig()->iInterfaceSets;
		for (TInt i = 0; i < ifcsets.Count(); ++i)
			{
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_CHANGECONFIGURATION_DUP6, 
                    "  Tearing down InterfaceSet %d", i );
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
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_CHANGECONFIGURATION_DUP7, 
                "  Device gets new configuration..." );
		// Setup all alternate settings 0 of all interfaces
		// (Don't separate the next two lines of code.)
		iCurrentConfig = aValue;
		RPointerArray<TUsbcInterfaceSet>& ifcsets = CurrentConfig()->iInterfaceSets;
		const TInt n = ifcsets.Count();
		for (TInt i = 0; i < n; ++i)
			{
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_CHANGECONFIGURATION_DUP8, 
                    "  Setting up InterfaceSet %d", i );
			InterfaceSetup(ifcsets[i]->iInterfaces[0]);
			}
		// Enter Configured state (from Address or Configured)
		NextDeviceState(EUsbcDeviceStateConfigured);
		}
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_CHANGECONFIGURATION_DUP9, 
            "  New configuration: %d", iCurrentConfig );
	return;
	}


void DUsbClientController::InterfaceSetup(TUsbcInterface* aIfc)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_INTERFACESETUP, 
	        "DUsbClientController::InterfaceSetup()" );
	const TInt num_eps = aIfc->iEndpoints.Count();
	for (TInt i = 0; i < num_eps; i++)
		{
		// Prepare this endpoint for I/O
		TUsbcLogicalEndpoint* const ep = aIfc->iEndpoints[i];
		// (TUsbcLogicalEndpoint's FS/HS endpoint sizes and interval values got
		//  adjusted in its constructor.)
		if (iHighSpeed)
			{
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_INTERFACESETUP_DUP1, 
                    "  Setting Ep info size to %d (HS)", ep->iEpSize_Hs );
			ep->iInfo.iSize = ep->iEpSize_Hs;
			}
		else
			{
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_INTERFACESETUP_DUP2, 
                    "  Setting Ep info size to %d (FS)", ep->iEpSize_Fs );
			ep->iInfo.iSize = ep->iEpSize_Fs;
			}
		const TInt idx = EpAddr2Idx(ep->iPEndpoint->iEndpointAddr);
		if (ConfigureEndpoint(idx, ep->iInfo) != KErrNone)
			{
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_INTERFACESETUP_DUP3, 
                    "  Error: Endpoint %d configuration failed", idx );
			continue;
			}
		// Should there be a problem with it then we could try resetting the ep
		// data toggle at this point (or before the Configure) as well.
        OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_INTERFACESETUP_DUP4, 
                "  Connecting real ep addr 0x%02x & logical ep #%d", ep->iPEndpoint->iEndpointAddr, ep->iLEndpointNum );
		ep->iPEndpoint->iLEndpoint = ep;
		}
	aIfc->iInterfaceSet->iCurrentInterface = aIfc->iSettingCode;
	return;
	}


void DUsbClientController::InterfaceSetTeardown(TUsbcInterfaceSet* aIfcSet)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_INTERFACESETTEARDOWN,
	        "DUsbClientController::InterfaceSetTeardown()" );
	if (aIfcSet->iInterfaces.Count() == 0)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_INTERFACESETTEARDOWN_DUP1,
                "  No interfaces exist - returning" );
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
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_INTERFACESETTEARDOWN_DUP2,
                    "  real ep %d not configured: skipping", idx );
			continue;
			}
		if (ResetDataToggle(idx) != KErrNone)
			{
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_INTERFACESETTEARDOWN_DUP3,
                    "  Error: Endpoint %d data toggle reset failed", idx );
			}
		if (DeConfigureEndpoint(idx) != KErrNone)
			{
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUSBCLIENTCONTROLLER_INTERFACESETTEARDOWN_DUP4,
                    "  Error: Endpoint %d de-configuration failed", idx );
			}

        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_INTERFACESETTEARDOWN_DUP5,
                "  disconnecting real ep & logical ep" );
		ep->iPEndpoint->iLEndpoint = NULL;
		}
	if (aIfcSet->CurrentInterface() != 0)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_INTERFACESETTEARDOWN_DUP6,
                "  Resetting alternate interface setting to 0" );
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
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_CHANGEINTERFACE, 
	        "DUsbClientController::ChangeInterface()" );
	TUsbcInterfaceSet* ifcset = aIfc->iInterfaceSet;
	const TUint8 setting = aIfc->iSettingCode;
	if (ifcset->iCurrentInterface == setting)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_CHANGEINTERFACE_DUP1, 
                "  New Ifc == old Ifc: nothing to do" );
		return;
		}
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_CHANGEINTERFACE_DUP2,
            "  Setting new interface setting #%d", setting );
	InterfaceSetTeardown(ifcset);
	InterfaceSetup(aIfc);
	StatusNotify(static_cast<TUsbcDeviceState>(KUsbAlternateSetting | setting), ifcset->iClientId);
	}


// aFunction gets called, successively, with the endpoint index of every ep in-use as its argument.
// (BTW: The declaration "type (class::*name)(params)" makes <name> a "pointer to element function".)
//
TInt DUsbClientController::DoForEveryEndpointInUse(TInt (DUsbClientController::*aFunction)(TInt), TInt& aCount)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, DUSBCLIENTCONTROLLER_DOFOREVERYENDPOINTINUSE, 
	        "DUsbClientController::DoForEveryEndpointInUse()" );
	aCount = 0;
	TUsbcConfiguration* const config = CurrentConfig();
	if (!config)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, DUSBCLIENTCONTROLLER_DOFOREVERYENDPOINTINUSE_DUP1, 
                "  Device is not configured - returning" );
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
