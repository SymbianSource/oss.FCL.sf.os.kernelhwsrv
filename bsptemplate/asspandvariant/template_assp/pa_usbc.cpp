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
// template\template_assp\pa_usbc.cpp
// Platform-dependent USB client controller layer (USB PSL).
// 
//


#include <template_assp.h>									// /assp/template_assp/
#include <template_assp_priv.h>								// /assp/template_assp/

#include <drivers/usbc.h>

#include "pa_usbc.h"										// .

// Debug support
#ifdef _DEBUG
static const char KUsbPanicCat[] = "USB PSL";
#endif


// Define USB_SUPPORTS_PREMATURE_STATUS_IN to enable proper handling of a premature STATUS_IN stage, i.e. a
// situation where the host sends less data than first announced and instead of more data (OUT) will send an
// IN token to start the status stage. What we do in order to implement this here is to prime the TX fifo with
// a ZLP immediately when we find out that we're dealing with a DATA_OUT request. This way, as soon as the
// premature IN token is received, we complete the transaction by sending off the ZLP. If we don't prime the
// TX fifo then there is no way for us to recognise a premature status because the IN token itself doesn't
// raise an interrupt. We would simply wait forever for more data, or rather we would time out and the host
// would move on and send the next Setup packet.
// The reason why we would not want to implement the proper behaviour is this: After having primed the TX fifo
// with a ZLP, it is impossible for a user to reject such a (class/vendor specific) Setup request, basically
// because the successful status stage happens automatically. At the time the user has received and decoded
// the Setup request there's for her no way to stall Ep0 in order to show to the host that this Setup packet
// is invalid or inappropriate or whatever, because she cannot prevent the status stage from happening.
// (All this is strictly true only if the amount of data in the data stage is less than or equal to Ep0's max
//	packet size. However this is almost always the case.)
//#define USB_SUPPORTS_PREMATURE_STATUS_IN


static const TUsbcEndpointCaps DeviceEndpoints[KUsbTotalEndpoints] =
	{
	//                                                      Hardware #    iEndpoints index
	{KEp0MaxPktSzMask,	(KUsbEpTypeControl	   | KUsbEpDirOut)}, //	 0 -  0
	{KEp0MaxPktSzMask,	(KUsbEpTypeControl	   | KUsbEpDirIn )}, //	 0 -  1
	{KUsbEpNotAvailable, KUsbEpNotAvailable},				// --- Not present
	{KBlkMaxPktSzMask,	(KUsbEpTypeBulk		   | KUsbEpDirIn )}, //	 1 -  3
	{KBlkMaxPktSzMask,	(KUsbEpTypeBulk		   | KUsbEpDirOut)}, //	 2 -  4
	{KUsbEpNotAvailable, KUsbEpNotAvailable},				// --- Not present
	{KUsbEpNotAvailable, KUsbEpNotAvailable},				// --- Not present
	{KIsoMaxPktSzMask,	(KUsbEpTypeIsochronous | KUsbEpDirIn )}, //	 3 -  7
	{KIsoMaxPktSzMask,	(KUsbEpTypeIsochronous | KUsbEpDirOut)}, //	 4 -  8
	{KUsbEpNotAvailable, KUsbEpNotAvailable},				// --- Not present
	{KUsbEpNotAvailable, KUsbEpNotAvailable},				// --- Not present
	{KIntMaxPktSzMask,	(KUsbEpTypeInterrupt   | KUsbEpDirIn )}, //	 5 - 11
	};


// --- TEndpoint --------------------------------------------------------------

TEndpoint::TEndpoint()
//
// Constructor
//
	: iRxBuf(NULL), iReceived(0), iLength(0), iZlpReqd(EFalse), iNoBuffer(EFalse), iDisabled(EFalse),
	  iPackets(0), iLastError(KErrNone), iRequest(NULL), iRxTimer(RxTimerCallback, this),
	  iRxTimerSet(EFalse), iRxMoreDataRcvd(EFalse), iPacketIndex(NULL), iPacketSize(NULL)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TEndpoint::TEndpoint"));
	}


void TEndpoint::RxTimerCallback(TAny* aPtr)
//
// (This function is static.)
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TEndpoint::RxTimerCallback"));

	TEndpoint* const ep = static_cast<TEndpoint*>(aPtr);
	if (!ep)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: !ep"));
		}
	else if (!ep->iRxTimerSet)
		{
		// Timer 'stop' substitute (instead of stopping it,
		// we just let it expire after clearing iRxTimerSet)
		__KTRACE_OPT(KUSB, Kern::Printf("!ep->iRxTimerSet - returning"));
		}
	else if (!ep->iRxBuf)
		{
		// Request already completed
		__KTRACE_OPT(KUSB, Kern::Printf("!ep->iRxBuf - returning"));
		}
	else if (ep->iRxMoreDataRcvd)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(" > rx timer cb: not yet completing..."));
		ep->iRxMoreDataRcvd = EFalse;
		ep->iRxTimer.Again(KRxTimerTimeout);
		}
	else
		{
		__KTRACE_OPT(KUSB, Kern::Printf(" > rx timer cb: completing now..."));
		*ep->iPacketSize = ep->iReceived;
		ep->iController->RxComplete(ep);
		}
	}


// --- TTemplateAsspUsbcc public ---------------------------------------------------

TTemplateAsspUsbcc::TTemplateAsspUsbcc()
//
// Constructor.
//
	: iCableConnected(ETrue), iBusIsPowered(EFalse),
	  iInitialized(EFalse), iUsbClientConnectorCallback(UsbClientConnectorCallback),
	  iEp0Configured(EFalse)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::TTemplateAsspUsbcc"));

	iAssp = static_cast<TemplateAssp*>(Arch::TheAsic());

	iSoftwareConnectable = iAssp->UsbSoftwareConnectable();

	iCableDetectable = iAssp->UsbClientConnectorDetectable();

	if (iCableDetectable)
		{
		// Register our callback for detecting USB cable insertion/removal.
		// We ignore the error code: if the registration fails, we just won't get any events.
		// (Which of course is bad enough...)
		(void) iAssp->RegisterUsbClientConnectorCallback(iUsbClientConnectorCallback, this);
		// Call the callback straight away so we get the proper PIL state from the beginning.
		(void) UsbClientConnectorCallback(this);
		}

	for (TInt i = 0; i < KUsbTotalEndpoints; i++)
		{
		iEndpoints[i].iController = this;
		}
	}


TInt TTemplateAsspUsbcc::Construct()
//
// Construct.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::Construct"));

	TUsbcDeviceDescriptor* DeviceDesc = TUsbcDeviceDescriptor::New(
		0x00,												// aDeviceClass
		0x00,												// aDeviceSubClass
		0x00,												// aDeviceProtocol
		KEp0MaxPktSz,										// aMaxPacketSize0
		KUsbVendorId,										// aVendorId
		KUsbProductId,										// aProductId
		KUsbDevRelease,										// aDeviceRelease
		1);													// aNumConfigurations
	if (!DeviceDesc)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Memory allocation for dev desc failed."));
		return KErrGeneral;
		}

	TUsbcConfigDescriptor* ConfigDesc = TUsbcConfigDescriptor::New(
		1,													// aConfigurationValue
		ETrue,												// aSelfPowered (see 12.4.2 "Bus-Powered Devices")
		ETrue,												// aRemoteWakeup
		0);													// aMaxPower (mA)
	if (!ConfigDesc)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Memory allocation for config desc failed."));
		return KErrGeneral;
		}

	TUsbcLangIdDescriptor* StringDescLang = TUsbcLangIdDescriptor::New(KUsbLangId);
	if (!StringDescLang)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Memory allocation for lang id $ desc failed."));
		return KErrGeneral;
		}

	// ('sizeof(x) - 2' because 'wchar_t KStringXyz' created a wide string that ends in '\0\0'.)

	TUsbcStringDescriptor* StringDescManu =
		TUsbcStringDescriptor::New(TPtr8(
									   const_cast<TUint8*>(reinterpret_cast<const TUint8*>(KStringManufacturer)),
									   sizeof(KStringManufacturer) - 2, sizeof(KStringManufacturer) - 2));
	if (!StringDescManu)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Memory allocation for manufacturer $ desc failed."));
		return KErrGeneral;
		}

	TUsbcStringDescriptor* StringDescProd =
		TUsbcStringDescriptor::New(TPtr8(
									   const_cast<TUint8*>(reinterpret_cast<const TUint8*>(KStringProduct)),
									   sizeof(KStringProduct) - 2, sizeof(KStringProduct) - 2));
	if (!StringDescProd)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Memory allocation for product $ desc failed."));
		return KErrGeneral;
		}

	TUsbcStringDescriptor* StringDescSer =
		TUsbcStringDescriptor::New(TPtr8(
									   const_cast<TUint8*>(reinterpret_cast<const TUint8*>(KStringSerialNo)),
									   sizeof(KStringSerialNo) - 2, sizeof(KStringSerialNo) - 2));
	if (!StringDescSer)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Memory allocation for serial no $ desc failed."));
		return KErrGeneral;
		}

	TUsbcStringDescriptor* StringDescConf =
		TUsbcStringDescriptor::New(TPtr8(
									   const_cast<TUint8*>(reinterpret_cast<const TUint8*>(KStringConfig)),
									   sizeof(KStringConfig) - 2, sizeof(KStringConfig) - 2));
	if (!StringDescConf)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Memory allocation for config $ desc failed."));
		return KErrGeneral;
		}

	const TBool r =	InitialiseBaseClass(DeviceDesc,
										ConfigDesc,
										StringDescLang,
										StringDescManu,
										StringDescProd,
										StringDescSer,
										StringDescConf);
	if (!r)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: UsbClientController::InitialiseBaseClass failed."));
		return KErrGeneral;
		}

	return KErrNone;
	}


TTemplateAsspUsbcc::~TTemplateAsspUsbcc()
//
// Destructor.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::~TTemplateAsspUsbcc"));

	// Unregister our callback for detecting USB cable insertion/removal
	if (iCableDetectable)
		{
		iAssp->UnregisterUsbClientConnectorCallback();
		}
	if (iInitialized)
		{
		// (The explicit scope operator is used against Lint warning #1506.)
		TTemplateAsspUsbcc::StopUdc();
		}
	}


TBool TTemplateAsspUsbcc::DeviceStateChangeCaps() const
//
// Returns capability of hardware to accurately track the device state (Chapter 9 state).
//
	{
	// TO DO: Return EFalse or ETrue here, depending on whether the UDC supports exact device state tracking
	// (most don't).
	return EFalse;
	}


TInt TTemplateAsspUsbcc::SignalRemoteWakeup()
//
// Forces the UDC into a non-idle state to perform a remote wakeup operation.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::SignalRemoteWakeup"));

	// TO DO: Do here whatever is necessary for the UDC to signal remote wakeup.
	
	return KErrNone;
	}


void TTemplateAsspUsbcc::DumpRegisters()
//
// Dumps the contents of a number of UDC registers to the screen (using Kern::Printf()).
// Rarely used, but might prove helpful when needed.
//
	{
	Kern::Printf("TCotullaUsbcc::DumpRegisters:");

	// TO DO: Print the contents of some (or all) UDC registers here.
	}


TDfcQue* TTemplateAsspUsbcc::DfcQ(TInt /* aUnit */)
//
// Returns a pointer to the kernel DFC queue to be used buy the USB LDD.
//
	{
	return Kern::DfcQue0();
	}


// --- TTemplateAsspUsbcc private virtual ------------------------------------------

TInt TTemplateAsspUsbcc::SetDeviceAddress(TInt aAddress)
//
// Sets the PIL-provided device address manually (if possible - otherwise do nothing).
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::SetDeviceAddress: %d", aAddress));

	// TO DO (optional): Set device address here.

	if (aAddress)
		{
		// Address can be zero.
		MoveToAddressState();
		}

	return KErrNone;
	}


TInt TTemplateAsspUsbcc::ConfigureEndpoint(TInt aRealEndpoint, const TUsbcEndpointInfo& aEndpointInfo)
//
// Prepares (enables) an endpoint (incl. Ep0) for data transmission or reception.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::ConfigureEndpoint(%d)", aRealEndpoint));

	const TInt n = ArrayIdx2TemplateEp(aRealEndpoint);
	if (n < 0)
		return KErrArgument;

	TEndpoint* const ep = &iEndpoints[aRealEndpoint];
	if (ep->iDisabled == EFalse)
		{
		EnableEndpointInterrupt(n);
		}
	ep->iNoBuffer = EFalse;
	if (n == 0)
		iEp0Configured = ETrue;

	return KErrNone;
	}


TInt TTemplateAsspUsbcc::DeConfigureEndpoint(TInt aRealEndpoint)
//
// Disables an endpoint (incl. Ep0).
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::DeConfigureEndpoint(%d)", aRealEndpoint));

	const TInt n = ArrayIdx2TemplateEp(aRealEndpoint);
	if (n < 0)
		return KErrArgument;

	DisableEndpointInterrupt(n);
	if (n == 0)
		iEp0Configured = EFalse;

	return KErrNone;
	}


TInt TTemplateAsspUsbcc::AllocateEndpointResource(TInt aRealEndpoint, TUsbcEndpointResource aResource)
//
// Puts the requested endpoint resource to use, if possible.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::AllocateEndpointResource(%d): %d",
									aRealEndpoint, aResource));

	// TO DO: Allocate endpoint resource here.

	return KErrNone;
	}


TInt TTemplateAsspUsbcc::DeAllocateEndpointResource(TInt aRealEndpoint, TUsbcEndpointResource aResource)
//
// Stops the use of the indicated endpoint resource, if beneficial.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::DeAllocateEndpointResource(%d): %d",
									aRealEndpoint, aResource));

	// TO DO: Deallocate endpoint resource here.

	return KErrNone;
	}


TBool TTemplateAsspUsbcc::QueryEndpointResource(TInt aRealEndpoint, TUsbcEndpointResource aResource) const
//
// Returns the status of the indicated resource and endpoint.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::QueryEndpointResource(%d): %d",
									aRealEndpoint, aResource));

	// TO DO: Query endpoint resource here. The return value should reflect the actual state.
	return ETrue;
	}


TInt TTemplateAsspUsbcc::OpenDmaChannel(TInt aRealEndpoint)
//
// Opens a DMA channel for this endpoint. This function is always called during the creation of an endpoint
// in the PIL. If DMA channels are a scarce resource, it's possible to do nothing here and wait for an
// AllocateEndpointResource call instead.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::OpenDmaChannel(%d)", aRealEndpoint));

	// TO DO (optional): Open DMA channel here.

	// An error should only  be returned in case of an actual DMA problem.
	return KErrNone;
	}


void TTemplateAsspUsbcc::CloseDmaChannel(TInt aRealEndpoint)
//
// Closes a DMA channel for this endpoint. This function is always called during the destruction of an
// endpoint in the PIL.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::CloseDmaChannel(%d)", aRealEndpoint));

	// TO DO (optional): Close DMA channel here (only if it was opened via OpenDmaChannel).
	}


TInt TTemplateAsspUsbcc::SetupEndpointRead(TInt aRealEndpoint, TUsbcRequestCallback& aCallback)
//
// Sets up a read request for an endpoint on behalf of the LDD.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::SetupEndpointRead(%d)", aRealEndpoint));

	if (!IS_OUT_ENDPOINT(aRealEndpoint))
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: !IS_OUT_ENDPOINT(%d)", aRealEndpoint));
		return KErrArgument;
		}
	TEndpoint* const ep = &iEndpoints[aRealEndpoint];
	if (ep->iRxBuf != NULL)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(" > WARNING: iEndpoints[%d].iRxBuf != NULL", aRealEndpoint));
		return KErrGeneral;
		}
	ep->iRxBuf = aCallback.iBufferStart;
	ep->iReceived = 0;
	ep->iLength = aCallback.iLength;
	// For Bulk reads we start out with the assumption of 1 packet (see BulkReceive for why):
	ep->iPackets = IS_BULK_OUT_ENDPOINT(aRealEndpoint) ? 1 : 0;
	ep->iRequest = &aCallback;
	ep->iPacketIndex = aCallback.iPacketIndex;
	if (IS_BULK_OUT_ENDPOINT(aRealEndpoint))
		*ep->iPacketIndex = 0;								// a one-off optimization
	ep->iPacketSize = aCallback.iPacketSize;

	const TInt n = ArrayIdx2TemplateEp(aRealEndpoint);
	if (ep->iDisabled)
		{
		ep->iDisabled = EFalse;
		EnableEndpointInterrupt(n);
		}
	else if (ep->iNoBuffer)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(" > There had been no Rx buffer available: reading Rx FIFO now"));
		ep->iNoBuffer = EFalse;
		if (IS_BULK_OUT_ENDPOINT(aRealEndpoint))
			{
			BulkReadRxFifo(n);
			}
		else if (IS_ISO_OUT_ENDPOINT(aRealEndpoint))
			{
			IsoReadRxFifo(n);
			}
		else
			{
			__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Endpoint not found"));
			}
		}

	return KErrNone;
	}


TInt TTemplateAsspUsbcc::SetupEndpointWrite(TInt aRealEndpoint, TUsbcRequestCallback& aCallback)
//
// Sets up a write request for an endpoint on behalf of the LDD.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::SetupEndpointWrite(%d)", aRealEndpoint));

	if (!IS_IN_ENDPOINT(aRealEndpoint))
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: !IS_IN_ENDPOINT(%d)", aRealEndpoint));
		return KErrArgument;
		}
	TEndpoint* const ep = &iEndpoints[aRealEndpoint];
	if (ep->iTxBuf != NULL)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: iEndpoints[%d].iTxBuf != NULL", aRealEndpoint));
		return KErrGeneral;
		}
	ep->iTxBuf = aCallback.iBufferStart;
	ep->iTransmitted = 0;
	ep->iLength = aCallback.iLength;
	ep->iPackets = 0;
	ep->iZlpReqd = aCallback.iZlpReqd;
	ep->iRequest = &aCallback;

	const TInt n = ArrayIdx2TemplateEp(aRealEndpoint);
	if (IS_BULK_IN_ENDPOINT(aRealEndpoint))
		{
		if (ep->iDisabled)
			{
			ep->iDisabled = EFalse;
			EnableEndpointInterrupt(n);
			}
		BulkTransmit(n);
		}
	else if (IS_ISO_IN_ENDPOINT(aRealEndpoint))
		{
		IsoTransmit(n);
		}
	else if (IS_INT_IN_ENDPOINT(aRealEndpoint))
		{
		IntTransmit(n);
		}
	else
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Endpoint not found"));
		}

	return KErrNone;
	}


TInt TTemplateAsspUsbcc::CancelEndpointRead(TInt aRealEndpoint)
//
// Cancels a read request for an endpoint on behalf of the LDD.
// No completion to the PIL occurs.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::CancelEndpointRead(%d)", aRealEndpoint));

	if (!IS_OUT_ENDPOINT(aRealEndpoint))
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: !IS_OUT_ENDPOINT(%d)", aRealEndpoint));
		return KErrArgument;
		}
	TEndpoint* const ep = &iEndpoints[aRealEndpoint];
	if (ep->iRxBuf == NULL)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(" > WARNING: iEndpoints[%d].iRxBuf == NULL", aRealEndpoint));
		return KErrNone;
		}
	ep->iRxBuf = NULL;
	ep->iReceived = 0;
	ep->iNoBuffer = EFalse;

	return KErrNone;
	}


TInt TTemplateAsspUsbcc::CancelEndpointWrite(TInt aRealEndpoint)
//
// Cancels a write request for an endpoint on behalf of the LDD.
// No completion to the PIL occurs.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::CancelEndpointWrite(%d)", aRealEndpoint));

	if (!IS_IN_ENDPOINT(aRealEndpoint))
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: !IS_IN_ENDPOINT(%d)", aRealEndpoint));
		return KErrArgument;
		}
	TEndpoint* const ep = &iEndpoints[aRealEndpoint];
	if (ep->iTxBuf == NULL)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(" > WARNING: iEndpoints[%d].iTxBuf == NULL", aRealEndpoint));
		return KErrNone;
		}

	// TO DO (optional): Flush the Ep's Tx FIFO here, if possible.

	ep->iTxBuf = NULL;
	ep->iTransmitted = 0;
	ep->iNoBuffer = EFalse;

	return KErrNone;
	}


TInt TTemplateAsspUsbcc::SetupEndpointZeroRead()
//
// Sets up an Ep0 read request (own function due to Ep0's special status).
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::SetupEndpointZeroRead"));

	TEndpoint* const ep = &iEndpoints[KEp0_Out];
	if (ep->iRxBuf != NULL)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(" > WARNING: iEndpoints[%d].iRxBuf != NULL", KEp0_Out));
		return KErrGeneral;
		}
	ep->iRxBuf = iEp0_RxBuf;
	ep->iReceived = 0;

	return KErrNone;
	}


TInt TTemplateAsspUsbcc::SetupEndpointZeroWrite(const TUint8* aBuffer, TInt aLength, TBool aZlpReqd)
//
// Sets up an Ep0 write request (own function due to Ep0's special status).
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::SetupEndpointZeroWrite"));

	TEndpoint* const ep = &iEndpoints[KEp0_In];
	if (ep->iTxBuf != NULL)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: iEndpoints[%d].iTxBuf != NULL", KEp0_In));
		return KErrGeneral;
		}
	ep->iTxBuf = aBuffer;
	ep->iTransmitted = 0;
	ep->iLength = aLength;
	ep->iZlpReqd = aZlpReqd;
	ep->iRequest = NULL;
	Ep0Transmit();

	return KErrNone;
	}


TInt TTemplateAsspUsbcc::SendEp0ZeroByteStatusPacket()
//
// Sets up an Ep0 write request for zero bytes.
// This is a separate function because no data transfer is involved here.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::SendEp0ZeroByteStatusPacket"));

	// This is possibly a bit tricky. When this function is called it just means that the higher layer wants a
	// ZLP to be sent. Whether we actually send one manually here depends on a number of factors, as the
	// current Ep0 state (i.e. the stage of the Ep0 Control transfer), and, in case the hardware handles some
	// ZLPs itself, whether it might already handle this one.

	// Here is an example of what the checking of the conditions might look like:

#ifndef USB_SUPPORTS_SET_DESCRIPTOR_REQUEST
	if ((!iEp0ReceivedNonStdRequest && iEp0State == EP0_IN_DATA_PHASE) ||
#else
	if ((!iEp0ReceivedNonStdRequest && iEp0State != EP0_IDLE) ||
#endif
#ifdef USB_SUPPORTS_PREMATURE_STATUS_IN
		(iEp0ReceivedNonStdRequest && iEp0State != EP0_OUT_DATA_PHASE))
#else
		(iEp0ReceivedNonStdRequest))
#endif
		{
		// TO DO: Arrange for the sending of a ZLP here.
		}

	return KErrNone;
	}


TInt TTemplateAsspUsbcc::StallEndpoint(TInt aRealEndpoint)
//
// Stalls an endpoint.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::StallEndpoint(%d)", aRealEndpoint));

	if (IS_ISO_ENDPOINT(aRealEndpoint))
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Iso endpoint cannot be stalled"));
		return KErrArgument;
		}

	// TO DO: Stall the endpoint here.

	return KErrNone;
	}


TInt TTemplateAsspUsbcc::ClearStallEndpoint(TInt aRealEndpoint)
//
// Clears the stall condition of an endpoint.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::ClearStallEndpoint(%d)", aRealEndpoint));

	if (IS_ISO_ENDPOINT(aRealEndpoint))
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Iso endpoint cannot be unstalled"));
		return KErrArgument;
		}

	// TO DO: De-stall the endpoint here.

	return KErrNone;
	}


TInt TTemplateAsspUsbcc::EndpointStallStatus(TInt aRealEndpoint) const
//
// Reports the stall status of an endpoint.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::EndpointStallStatus(%d)", aRealEndpoint));

	if (IS_ISO_ENDPOINT(aRealEndpoint))
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Iso endpoint has no stall status"));
		return KErrArgument;
		}

	// TO DO: Query endpoint stall status here. The return value should reflect the actual state.
	return ETrue;
	}


TInt TTemplateAsspUsbcc::EndpointErrorStatus(TInt aRealEndpoint) const
//
// Reports the error status of an endpoint.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::EndpointErrorStatus(%d)", aRealEndpoint));

	if (!IS_VALID_ENDPOINT(aRealEndpoint))
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: !IS_VALID_ENDPOINT(%d)", aRealEndpoint));
		return KErrArgument;
		}

	// TO DO: Query endpoint error status here. The return value should reflect the actual state.
	// With some UDCs there is no way of inquiring the endpoint error status; say 'ETrue' in that case.
	return KErrNone;
	}


TInt TTemplateAsspUsbcc::ResetDataToggle(TInt aRealEndpoint)
//
// Resets to zero the data toggle bit of an endpoint.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::ResetDataToggle(%d)", aRealEndpoint));

	// TO DO: Reset the endpoint's data toggle bit here.
	// With some UDCs there is no way to individually reset the endpoint's toggle bits; just return KErrNone
	// in that case.

	return KErrNone;
	}


TInt TTemplateAsspUsbcc::SynchFrameNumber() const
//
// For use with isochronous endpoints only. Causes the SOF frame number to be returned.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::SynchFrameNumber"));

	// TO DO: Query and return the SOF frame number here.
	return 0;
	}


void TTemplateAsspUsbcc::SetSynchFrameNumber(TInt aFrameNumber)
//
// For use with isochronous endpoints only. Causes the SOF frame number to be stored.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::SetSynchFrameNumber(%d)", aFrameNumber));

	// We should actually store this number somewhere. But the PIL always sends '0x00'
	// in response to a SYNCH_FRAME request...
	// TO DO: Store the frame number. Alternatively (until SYNCH_FRAME request specification changes): Do
	// nothing.
	}


TInt TTemplateAsspUsbcc::StartUdc()
//
// Called to initialize the device controller hardware before any operation can be performed.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::StartUdc"));

	if (iInitialized)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: UDC already initialised"));
		return KErrNone;
		}

	// Disable UDC (might also reset the entire design):
	UdcDisable();

	// Enable UDC's clock:
	// TO DO: Enable UDC's clock here.

	// Even if only one USB feature has been enabled, we later need to undo it:
	iInitialized = ETrue;

	// Bind & enable the UDC interrupt
	if (SetupUdcInterrupt() != KErrNone)
		{
		return KErrGeneral;
		}

	// Write meaningful values to some registers:
	InitialiseUdcRegisters();

	// Finally, turn on the UDC:
	UdcEnable();

	return KErrNone;
	}


TInt TTemplateAsspUsbcc::StopUdc()
//
// Basically, makes undone what happened in StartUdc.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::StopUdc"));

	if (!iInitialized)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: UDC not initialized"));
		return KErrNone;
		}

	// Disable UDC:
	UdcDisable();

	// Mask (disable) Reset interrupt:
	// TO DO: Mask (disable) the USB Reset interrupt here.

	// Disable & unbind the UDC interrupt:
	ReleaseUdcInterrupt();

	// Finally turn off UDC's clock:
	// TO DO: Disable UDC's clock here.

	// Only when all USB features have been disabled we'll call it a day:
	iInitialized = EFalse;

	return KErrNone;
	}


TInt TTemplateAsspUsbcc::UdcConnect()
//
// Connects the UDC to the bus under software control. How this is achieved depends on the UDC; the
// functionality might also be part of the Variant component (instead of the ASSP).
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::UdcConnect"));

	// Here: A call into the Variant-provided function.
	return iAssp->UsbConnect();
	}


TInt TTemplateAsspUsbcc::UdcDisconnect()
//
// Disconnects the UDC from the bus under software control. How this is achieved depends on the UDC; the
// functionality might also be part of the Variant component (instead of the ASSP).
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::UdcDisconnect"));

	// Here: A call into the Variant-provided function.
	return iAssp->UsbDisconnect();
	}


TBool TTemplateAsspUsbcc::UsbConnectionStatus() const
//
// Returns a value showing the USB cable connection status of the device.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::UsbConnectionStatus"));

	return iCableConnected;
	}


TBool TTemplateAsspUsbcc::UsbPowerStatus() const
//
// Returns a truth value showing whether VBUS is currently powered or not.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::UsbPowerStatus"));

	return iBusIsPowered;
	}


TBool TTemplateAsspUsbcc::DeviceSelfPowered() const
//
// Returns a truth value showing whether the device is currently self-powered or not.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::DeviceSelfPowered"));

	// TO DO: Query and return self powered status here. The return value should reflect the actual state.
	// (This can be always 'ETrue' if the UDC does not support bus-powered devices.)
	return ETrue;
	}


const TUsbcEndpointCaps* TTemplateAsspUsbcc::DeviceEndpointCaps() const
//
// Returns a pointer to an array of elements, each of which describes the capabilities of one endpoint.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::DeviceEndpointCaps"));
	__KTRACE_OPT(KUSB, Kern::Printf(" > Ep: Sizes Mask, Types Mask"));
	__KTRACE_OPT(KUSB, Kern::Printf(" > --------------------------"));
	for (TInt i = 0; i < KUsbTotalEndpoints; ++i)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(" > %02d: 0x%08x, 0x%08x",
										i, DeviceEndpoints[i].iSizes, DeviceEndpoints[i].iTypesAndDir));
		}
	return DeviceEndpoints;
	}


TInt TTemplateAsspUsbcc::DeviceTotalEndpoints() const
//
// Returns the element number of the endpoints array a pointer to which is returned by DeviceEndpointCaps.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::DeviceTotalEndpoints"));

	return KUsbTotalEndpoints;
	}


TBool TTemplateAsspUsbcc::SoftConnectCaps() const
//
// Returns a truth value showing whether or not there is the capability to disconnect and re-connect the D+
// line under software control.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::SoftConnectCaps"));

	return iSoftwareConnectable;
	}


void TTemplateAsspUsbcc::Suspend()
//
// Called by the PIL after a Suspend event has been reported (by us).
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::Suspend"));

	// TO DO (optional): Implement here anything the device might require after bus SUSPEND signalling.
	}


void TTemplateAsspUsbcc::Resume()
//
// Called by the PIL after a Resume event has been reported (by us).
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::Resume"));

	// TO DO (optional): Implement here anything the device might require after bus RESUME signalling.
	}


void TTemplateAsspUsbcc::Reset()
//
// Called by the PIL after a Reset event has been reported (by us).
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::Reset"));

	// This does not really belong here, but has to do with the way the PIL sets
	// up Ep0 reads and writes.
	TEndpoint* ep = &iEndpoints[0];
	ep->iRxBuf = NULL;
	++ep;
	ep->iTxBuf = NULL;
	// Idle
	Ep0NextState(EP0_IDLE);

	// TO DO (optional): Implement here anything the device might require after bus RESET signalling.

	// Write meaningful values to some registers
	InitialiseUdcRegisters();
	UdcEnable();
	if (iEp0Configured)
		EnableEndpointInterrupt(0);
	}


// --- TTemplateAsspUsbcc private --------------------------------------------------

void TTemplateAsspUsbcc::InitialiseUdcRegisters()
//
// Called after every USB Reset etc.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::InitialiseUdcRegisters"));

	// Unmask Suspend interrupt
	// TO DO: Unmask Suspend interrupt here.

	// Unmask Resume interrupt
	// TO DO: Unmask Resume interrupt here.

	// Unmask Start-of-Frame (SOF) interrupt
	// TO DO (optional): Unmask SOF interrupt here.

	// Disable interrupt requests for all endpoints
	// TO DO: Disable interrupt requests for all endpoints here.
	}


void TTemplateAsspUsbcc::UdcEnable()
//
// Enables the UDC for USB transmission or reception.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::UdcEnable"));

	// TO DO: Do whatever is necessary to enable the UDC here. This might include enabling (unmasking)
	// the USB Reset interrupt, setting a UDC enable bit, etc.
	}


void TTemplateAsspUsbcc::UdcDisable()
//
// Disables the UDC.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::UdcDisable"));

	// TO DO: Do whatever is necessary to disable the UDC here. This might include disabling (masking)
	// the USB Reset interrupt, clearing a UDC enable bit, etc.
	}


void TTemplateAsspUsbcc::EnableEndpointInterrupt(TInt aEndpoint)
//
// Enables interrupt requests for an endpoint.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::EnableEndpointInterrupt(%d)", aEndpoint));

	// Enable (unmask) interrupt requests for this endpoint:
	// TO DO: Enable interrupt requests for aEndpoint here.
	}


void TTemplateAsspUsbcc::DisableEndpointInterrupt(TInt aEndpoint)
//
// Disables interrupt requests for an endpoint.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::DisableEndpointInterrupt(%d)", aEndpoint));

	// Disable (mask) interrupt requests for this endpoint:
	// TO DO: Disable interrupt requests for aEndpoint here.
	}


void TTemplateAsspUsbcc::ClearEndpointInterrupt(TInt aEndpoint)
//
// Clears a pending interrupt request for an endpoint.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::ClearEndpointInterrupt(%d)", aEndpoint));

	// Clear (reset) pending interrupt request for this endpoint:
	// TO DO: Clear interrupt request for aEndpoint here.
	}


void TTemplateAsspUsbcc::Ep0IntService()
//
// ISR for endpoint zero interrupt.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::Ep0IntService"));

	// TO DO: Enquire about Ep0 status & the interrupt cause here. Depending on the event and the Ep0 state,
	// one or more of the following functions might then be called:
	Ep0Cancel();
	Ep0ReadSetupPkt();
	Ep0EndXfer();
	Ep0PrematureStatusOut();
	Ep0Transmit();
	Ep0StatusIn();
	Ep0Receive();
	ClearStallEndpoint(0);

	ClearEndpointInterrupt(0);
	return;
	}


void TTemplateAsspUsbcc::Ep0ReadSetupPkt()
//
// Called from the Ep0 ISR when a new Setup packet has been received.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::Ep0ReadSetupPkt"));

	TEndpoint* const ep = &iEndpoints[KEp0_Out];
	TUint8* buf = ep->iRxBuf;
	if (!buf)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: No Ep0 Rx buffer available (1)"));
		StallEndpoint(KEp0_Out);
		return;
		}

	// TO DO: Read Setup packet data from Rx FIFO into 'buf' here.
	// (In this function we don't need to use "ep->iReceived" since Setup packets
	// are always 8 bytes long.)

	// Upcall into PIL to determine next Ep0 state:
	TUsbcEp0State state = EnquireEp0NextState(ep->iRxBuf);

	if (state == EEp0StateStatusIn)
		{
		Ep0NextState(EP0_IDLE);								// Ep0 No Data
		}
	else if (state == EEp0StateDataIn)
		{
		Ep0NextState(EP0_IN_DATA_PHASE);					// Ep0 Control Read
		}
	else
		{
		Ep0NextState(EP0_OUT_DATA_PHASE);					// Ep0 Control Write
		}

	ep->iRxBuf = NULL;
	const TInt r = Ep0RequestComplete(KEp0_Out, 8, KErrNone);

	// Don't finish (proceed) if request completion returned 'KErrNotFound'!
	if (!(r == KErrNone || r == KErrGeneral))
		{
		DisableEndpointInterrupt(0);
		}

	// TO DO (optional): Clear Ep0 Setup condition flags here.

#ifdef USB_SUPPORTS_PREMATURE_STATUS_IN
	if (iEp0State == EP0_OUT_DATA_PHASE)
		{
		// Allow for a premature STATUS IN
		// TO DO: Arrange for the sending of a ZLP here.
		}
#endif
	}


void TTemplateAsspUsbcc::Ep0ReadSetupPktProceed()
//
// Called by the PIL to signal that it has finished processing a received Setup packet and that the PSL can
// now prepare itself for the next Ep0 reception (for instance by re-enabling the Ep0 interrupt).
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::Ep0ReadSetupPktProceed"));

	EnableEndpointInterrupt(0);
	}


void TTemplateAsspUsbcc::Ep0Receive()
//
// Called from the Ep0 ISR when a data packet has been received.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::Ep0Receive"));

	TEndpoint* const ep = &iEndpoints[KEp0_Out];
	TUint8* buf = ep->iRxBuf;
	if (!buf)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: No Ep0 Rx buffer available (2)"));
		StallEndpoint(KEp0_Out);
		return;
		}

	TInt n = 0;
	// TO DO: Read packet data from Rx FIFO into 'buf' and update 'n' (# of received bytes) here.

	ep->iReceived = n;
	ep->iRxBuf = NULL;
	const TInt r = Ep0RequestComplete(KEp0_Out, n, KErrNone);

	// Don't finish (proceed) if request was 'KErrNotFound'!
	if (!(r == KErrNone || r == KErrGeneral))
		{
		DisableEndpointInterrupt(0);
		}

	// TO DO (optional): Clear Ep0 Rx condition flags here.

#ifdef USB_SUPPORTS_PREMATURE_STATUS_IN
	// Allow for a premature STATUS IN
	// TO DO: Arrange for the sending of a ZLP here.
#endif
	}


void TTemplateAsspUsbcc::Ep0ReceiveProceed()
//
// Called by the PIL to signal that it has finished processing a received Ep0 data packet and that the PSL can
// now prepare itself for the next Ep0 reception (for instance by re-enabling the Ep0 interrupt).
//
	{
	Ep0ReadSetupPktProceed();
	}


void TTemplateAsspUsbcc::Ep0Transmit()
//
// Called from either the Ep0 ISR or the PIL when a data packet has been or is to be transmitted.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::Ep0Transmit"));

	if (iEp0State != EP0_IN_DATA_PHASE)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(" > WARNING: Invalid Ep0 state when trying to handle EP0 IN"));
		// TO DO (optional): Do something about this warning.
		}

	TEndpoint* const ep = &iEndpoints[KEp0_In];
	const TUint8* buf = ep->iTxBuf;
	if (!buf)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(" > No Tx buffer available: returning"));
		return;
		}
	const TInt t = ep->iTransmitted;						// already transmitted
	buf += t;
	TInt n = 0;												// now transmitted

	// TO DO: Write packet data (if any) into Tx FIFO from 'buf' and update 'n' (# of tx'ed bytes) here.

	ep->iTransmitted += n;
	
	// coverity[dead_error_condition]
	// The next line should be reachable when this template file is edited for use
	if (n == KEp0MaxPktSz)
		{
		if (ep->iTransmitted == ep->iLength && !(ep->iZlpReqd))
			Ep0NextState(EP0_END_XFER);
		}
	else if (n && n != KEp0MaxPktSz)
		{
		// Send off the data
		__ASSERT_DEBUG((ep->iTransmitted == ep->iLength),
					   Kern::Printf(" > ERROR: Short packet in mid-transfer"));
		Ep0NextState(EP0_END_XFER);
		// TO DO: Send off the data here.
		}
	else // if (n == 0)
		{
		__ASSERT_DEBUG((ep->iTransmitted == ep->iLength),
					   Kern::Printf(" > ERROR: Nothing transmitted but still not finished"));
		if (ep->iZlpReqd)
			{
			// Send a zero length packet
			ep->iZlpReqd = EFalse;
			Ep0NextState(EP0_END_XFER);
			// TO DO: Arrange for the sending of a ZLP here.
			}
		else
			{
			__KTRACE_OPT(KPANIC, Kern::Printf("  Error: nothing transmitted & no ZLP req'd"));
			}
		}
	}


void TTemplateAsspUsbcc::Ep0EndXfer()
//
// Called at the end of a Ep0 Control transfer.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::Ep0EndXfer"));

	// TO DO (optional): Clear Ep0 Rx condition flags here.

	Ep0NextState(EP0_IDLE);
	TEndpoint* const ep = &iEndpoints[KEp0_In];
	ep->iTxBuf = NULL;
	(void) Ep0RequestComplete(KEp0_In, ep->iTransmitted, KErrNone);
	}


void TTemplateAsspUsbcc::Ep0Cancel()
//
// Called when an ongoing Ep0 Control transfer has to be aborted prematurely (for instance when receiving a
// new Setup packet before the processing of the old one has completed).
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::Ep0Cancel"));

	Ep0NextState(EP0_IDLE);
	TEndpoint* const ep = &iEndpoints[KEp0_In];
	if (ep->iTxBuf)
		{
		ep->iTxBuf = NULL;
		const TInt err = (ep->iTransmitted == ep->iLength) ? KErrNone : KErrCancel;
		(void) Ep0RequestComplete(KEp0_In, ep->iTransmitted, err);
		}
	}


void TTemplateAsspUsbcc::Ep0PrematureStatusOut()
//
// Called when an ongoing Ep0 Control transfer encounters a premature Status OUT condition.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::Ep0PrematureStatusOut"));

	// TO DO (optional): Clear Ep0 Rx condition flags here.

	Ep0NextState(EP0_IDLE);

	// TO DO (optional): Flush the Ep0 Tx FIFO here, if possible.

	TEndpoint* const ep = &iEndpoints[KEp0_In];
	if (ep->iTxBuf)
		{
		ep->iTxBuf = NULL;
		(void) Ep0RequestComplete(KEp0_In, ep->iTransmitted, KErrPrematureEnd);
		}
	}


void TTemplateAsspUsbcc::Ep0StatusIn()
//
// Called when an ongoing Ep0 Control transfer moves to a Status IN stage.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::Ep0StatusIn"));

	Ep0NextState(EP0_IDLE);
	}


void TTemplateAsspUsbcc::BulkTransmit(TInt aEndpoint)
//
// Endpoint 1 (BULK IN).
// Called from either the Ep ISR or the PIL when a data packet has been or is to be transmitted.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::BulkTransmit(%d)", aEndpoint));

	// TO DO: Enquire about Ep status here.

	const TInt idx = 3;										// only in our special case of course!
	TEndpoint* const ep = &iEndpoints[idx];
	const TUint8* buf = ep->iTxBuf;
	if (!buf)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: No Tx buffer has been set up"));
		DisableEndpointInterrupt(aEndpoint);
		ep->iDisabled = ETrue;
		ClearEndpointInterrupt(aEndpoint);
		return;
		}
	const TInt t = ep->iTransmitted;						// already transmitted
	const TInt len = ep->iLength;							// to be sent in total
	// (len || ep->iPackets): Don't complete for a zero bytes request straight away.
	if (t >= len && (len || ep->iPackets))
		{
		if (ep->iZlpReqd)
			{
			__KTRACE_OPT(KUSB, Kern::Printf(" > 'Transmit Short Packet' explicitly"));
			// TO DO: Arrange for the sending of a ZLP here.
			ep->iZlpReqd = EFalse;
			}
		else
			{
			__KTRACE_OPT(KUSB, Kern::Printf(" > All data sent: %d --> completing", len));
			ep->iTxBuf = NULL;
			ep->iRequest->iTxBytes = ep->iTransmitted;
			ep->iRequest->iError = KErrNone;
			EndpointRequestComplete(ep->iRequest);
			ep->iRequest = NULL;
			}
		}
	else
		{
		buf += t;
		TInt left = len - t;								// left in total
		TInt n = (left >= KBlkMaxPktSz) ? KBlkMaxPktSz : left; // now to be transmitted
		__KTRACE_OPT(KUSB, Kern::Printf(" > About to send %d bytes (%d bytes left in total)", n, left));

		// TO DO: Write data into Tx FIFO from 'buf' here.

		ep->iTransmitted += n;
		ep->iPackets++;										// only used for (len == 0) case
		left -= n;											// (still) left in total
		if (n < KBlkMaxPktSz)
			{
			__KTRACE_OPT(KUSB, Kern::Printf(" > 'Transmit Short Packet' implicitly"));
			// TO DO: Arrange for the sending of a ZLP here.
			ep->iZlpReqd = EFalse;
			}
		// If double-buffering is available, it might be possible to stick a second packet
		// into the FIFO here.

		// TO DO (optional): Send another packet if possible (& available) here.
		}

	ClearEndpointInterrupt(aEndpoint);
	}



void TTemplateAsspUsbcc::BulkReceive(TInt aEndpoint)
//
// Endpoint 2 (BULK OUT) (This one is called in an ISR.)
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::BulkReceive(%d)", aEndpoint));

	// TO DO: Enquire about Ep status here.
	const TUint32 status = *(TUint32*)0xdefaced;			// bogus

	const TInt idx = 4;										// only in our special case of course!
	TEndpoint* const ep = &iEndpoints[idx];
	TUint8* buf = ep->iRxBuf;
	if (!buf)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(" > No Rx buffer available: setting iNoBuffer"));
		ep->iNoBuffer = ETrue;
		DisableEndpointInterrupt(aEndpoint);
		ep->iDisabled = ETrue;
		ClearEndpointInterrupt(aEndpoint);
		return;
		}
	TInt bytes = 0;
	const TInt r = ep->iReceived;							// already received
	// TO DO: Check whether a ZLP was received here:
	if (status & 1)											// some condition
		{
		__KTRACE_OPT(KUSB, Kern::Printf(" > received zero-length packet"));
		}
	else if (status & 2)									// some other condition
		{
		// TO DO: Get number of bytes received here.
		bytes = *(TUint32*)0xdadadada;						// bogus
		__KTRACE_OPT(KUSB, Kern::Printf(" > Bulk received: %d bytes", bytes));
		if (r + bytes > ep->iLength)
			{
			__KTRACE_OPT(KUSB, Kern::Printf(" > not enough space in rx buffer: setting iNoBuffer"));
			ep->iNoBuffer = ETrue;
			StopRxTimer(ep);
			*ep->iPacketSize = ep->iReceived;
			RxComplete(ep);

			// TO DO (optional): Clear Ep Rx condition flags here.

			ClearEndpointInterrupt(aEndpoint);
			return;
			}
		buf += r;											// set buffer pointer

		// TO DO: Read 'bytes' bytes from Rx FIFO into 'buf' here.

		ep->iReceived += bytes;
		}
	else
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Inconsistent Ep%d state", aEndpoint));

		// TO DO (optional): Clear Ep Rx condition flags here.

		ClearEndpointInterrupt(aEndpoint);
		return;
		}

	if (bytes == 0)
		{
		// ZLPs must be recorded separately
		const TInt i = ep->iReceived ? 1 : 0;
		ep->iPacketIndex[i] = r;
		ep->iPacketSize[i] = 0;
		// If there were data packets before: total packets reported 1 -> 2
		ep->iPackets += i;
		}

	if ((bytes < KBlkMaxPktSz) ||
		(ep->iReceived == ep->iLength))
		{
		StopRxTimer(ep);
		*ep->iPacketSize = ep->iReceived;
		RxComplete(ep);
		// since we have no buffer any longer we disable interrupts:
		DisableEndpointInterrupt(aEndpoint);
		ep->iDisabled = ETrue;
		}
	else
		{
		if (!ep->iRxTimerSet)
			{
			__KTRACE_OPT(KUSB, Kern::Printf(" > setting rx timer"));
			ep->iRxTimerSet = ETrue;
			ep->iRxTimer.OneShot(KRxTimerTimeout);
			}
		else
			{
			ep->iRxMoreDataRcvd = ETrue;
			}
		}

	// TO DO (optional): Clear Ep Rx condition flags here.

	ClearEndpointInterrupt(aEndpoint);
	}


void TTemplateAsspUsbcc::BulkReadRxFifo(TInt aEndpoint)
//
// Endpoint 2 (BULK OUT) (This one is called w/o interrupt to be served.)
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::BulkReadRxFifo(%d)", aEndpoint));

	// TO DO: Enquire about Ep status here.
	const TUint32 status = *(TUint32*)0xdefaced;			// bogus

	const TInt idx = 4;										// only in our special case of course!
	TEndpoint* const ep = &iEndpoints[idx];
	TUint8* buf = ep->iRxBuf;
	if (!buf)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: No Rx buffer has been set up"));
		return;
		}
	TInt bytes = 0;
	const TInt r = ep->iReceived;							// already received
	// TO DO: Check whether a ZLP was received here:
	if (status & 1)											// some condition
		{
		__KTRACE_OPT(KUSB, Kern::Printf(" > received zero-length packet"));
		}
	else if (status & 2)									// some other condition
		{
		// TO DO: Get number of bytes received here.
		bytes = *(TUint32*)0xdadadada;						// bogus
		__KTRACE_OPT(KUSB, Kern::Printf(" > Bulk received: %d bytes", bytes));
		if (r + bytes > ep->iLength)
			{
			__KTRACE_OPT(KUSB, Kern::Printf(" > not enough space in rx buffer: setting iNoBuffer"));
			ep->iNoBuffer = ETrue;
			*ep->iPacketSize = ep->iReceived;
			RxComplete(ep);
			return;
			}
		buf += r;											// set buffer pointer

		// TO DO: Read 'bytes' bytes from Rx FIFO into 'buf' here.

		ep->iReceived += bytes;
		}
	else
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Inconsistent Ep%d state", aEndpoint));
		return;
		}

	if (bytes == 0)
		{
		// ZLPs must be recorded separately
		const TInt i = ep->iReceived ? 1 : 0;
		ep->iPacketIndex[i] = r;
		ep->iPacketSize[i] = 0;
		// If there were data packets before: total packets reported 1 -> 2
		ep->iPackets += i;
		}

	if ((bytes < KBlkMaxPktSz) ||
		(ep->iReceived == ep->iLength))
		{
		*ep->iPacketSize = ep->iReceived;
		RxComplete(ep);
		}
	else
		{
		if (!ep->iRxTimerSet)
			{
			__KTRACE_OPT(KUSB, Kern::Printf(" > setting rx timer"));
			ep->iRxTimerSet = ETrue;
			ep->iRxTimer.OneShot(KRxTimerTimeout);
			}
		else
			{
			ep->iRxMoreDataRcvd = ETrue;
			}
		}

	// TO DO (optional): Clear Ep Rx condition flags here.

	}


void TTemplateAsspUsbcc::IsoTransmit(TInt aEndpoint)
//
// Endpoint 3 (ISOCHRONOUS IN).
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::IsoTransmit(%d)", aEndpoint));

	// TO DO: Write data to endpoint FIFO. Might be similar to BulkTransmit.

	}


void TTemplateAsspUsbcc::IsoReceive(TInt aEndpoint)
//
// Endpoint 4 (ISOCHRONOUS OUT) (This one is called in an ISR.)
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::IsoReceive(%d)", aEndpoint));

	// TO DO: Read data from endpoint FIFO. Might be similar to BulkReceive.
	}


void TTemplateAsspUsbcc::IsoReadRxFifo(TInt aEndpoint)
//
// Endpoint 4 (ISOCHRONOUS OUT) (This one is called w/o interrupt to be served.)
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::IsoReadRxFifo(%d)", aEndpoint));

	// TO DO: Read data from endpoint FIFO. Might be similar to BulkReadRxFifo.
	}


void TTemplateAsspUsbcc::IntTransmit(TInt aEndpoint)
//
// Endpoint 5 (INTERRUPT IN).
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::IntTransmit(%d)", aEndpoint));

	// TO DO: Write data to endpoint FIFO. Might be similar to BulkTransmit.
	}


void TTemplateAsspUsbcc::RxComplete(TEndpoint* aEndpoint)
//
// Called at the end of an Rx (OUT) transfer to complete to the PIL.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::RxComplete"));

	TUsbcRequestCallback* const req = aEndpoint->iRequest;

	__ASSERT_DEBUG((req != NULL), Kern::Fault(KUsbPanicCat, __LINE__));

	aEndpoint->iRxBuf = NULL;
	aEndpoint->iRxTimerSet = EFalse;
	aEndpoint->iRxMoreDataRcvd = EFalse;
	req->iRxPackets = aEndpoint->iPackets;
	req->iError = aEndpoint->iLastError;
	EndpointRequestComplete(req);
	aEndpoint->iRequest = NULL;
	}


void TTemplateAsspUsbcc::StopRxTimer(TEndpoint* aEndpoint)
//
// Stops (cancels) the Rx timer for an endpoint.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::StopRxTimer"));

	if (aEndpoint->iRxTimerSet)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(" > stopping rx timer"));
		aEndpoint->iRxTimer.Cancel();
		aEndpoint->iRxTimerSet = EFalse;
		}
	}


void TTemplateAsspUsbcc::EndpointIntService(TInt aEndpoint)
//
// ISR for endpoint interrupts.
// Note: the aEndpoint here is a "hardware endpoint", not a aRealEndpoint.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::EndpointIntService(%d)", aEndpoint));

	switch (aEndpoint)
		{
	case 0:
		Ep0IntService();
		break;
	case 1:
		BulkTransmit(aEndpoint);
		break;
	case 2:
		BulkReceive(aEndpoint);
		break;
	case 3:
		IsoTransmit(aEndpoint);
		break;
	case 4:
		IsoReceive(aEndpoint);
		break;
	case 5:
		IntTransmit(aEndpoint);
		break;
	default:
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Endpoint not found"));
		break;
		}
	}


TInt TTemplateAsspUsbcc::ResetIntService()
//
// ISR for a USB Reset event interrupt.
// This function returns a value which can be used on the calling end to decide how to proceed.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::ResetIntService"));

	// Clear an interrupt:
	// TO DO: Clear reset interrupt flag here.

	// TO DO (optional): Enquire about special conditions and possibly return here.

	DeviceEventNotification(EUsbEventReset);

	return KErrNone;
	}


void TTemplateAsspUsbcc::SuspendIntService()
//
// ISR for a USB Suspend event interrupt.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::SuspendIntService"));

	// Clear an interrupt:
	// TO DO: Clear suspend interrupt flag here.

	DeviceEventNotification(EUsbEventSuspend);
	}


void TTemplateAsspUsbcc::ResumeIntService()
//
// ISR for a USB Resume event interrupt.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::ResumeIntService"));

	// Clear an interrupt:
	// TO DO: Clear resume interrupt flag here.

	DeviceEventNotification(EUsbEventResume);
	}


void TTemplateAsspUsbcc::SofIntService()
//
// ISR for a USB Start-of-Frame event interrupt.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::SofIntService"));

	// Clear an interrupt:
	// TO DO: Clear SOF interrupt flag here.

	// TO DO (optional): Do something about the SOF condition.
	}


void TTemplateAsspUsbcc::UdcInterruptService()
//
// Main UDC ISR - determines the cause of the interrupt, clears the condition, dispatches further for service.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::InterruptService"));

	// TO DO: Find the cause of the interrupt (possibly querying a number of status registers) here.

	// Determine the type of UDC interrupt & then serve it:
	// (The following operations are of course EXAMPLES only.)
	volatile const TUint32* const status_reg = (TUint32*) 0xdefaced;
	const TUint32 status = *status_reg;
	enum {reset_interrupt, suspend_interrupt, resume_interrupt, sof_interrupt, ep_interrupt};

	// Reset interrupt
	if (status & reset_interrupt)
		{
		ResetIntService();
		}

	// Suspend interrupt
	if (status & suspend_interrupt)
		{
		SuspendIntService();
		}

	// Resume interrupt
	if (status & resume_interrupt)
		{
		ResumeIntService();
		}

	// Start-of-Frame interrupt
	if (status & sof_interrupt)
		{
		SofIntService();
		}

	// Endpoint interrupt
	if (status & ep_interrupt)
		{
		const TInt ep = status & 0xffff0000;
			{
			EndpointIntService(ep);
			}
		}
	}


void TTemplateAsspUsbcc::Ep0NextState(TEp0State aNextState)
//
// Moves the Ep0 state to aNextState.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::Ep0NextState"));

	iEp0State = aNextState;
	}


void TTemplateAsspUsbcc::UdcIsr(TAny* aPtr)
//
// This is the static ASSP first-level UDC interrupt service routine. It dispatches the call to the
// actual controller's ISR.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::UdcIsr"));

	static_cast<TTemplateAsspUsbcc*>(aPtr)->UdcInterruptService();
	}


TInt TTemplateAsspUsbcc::UsbClientConnectorCallback(TAny* aPtr)
//
// This function is called in ISR context by the Variant's UsbClientConnectorInterruptService.
// (This function is static.)
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::UsbClientConnectorCallback"));

	TTemplateAsspUsbcc* const ptr = static_cast<TTemplateAsspUsbcc*>(aPtr);
	ptr->iCableConnected = ptr->iAssp->UsbClientConnectorInserted();
#ifdef _DEBUG
	_LIT(KIns, "inserted");
	_LIT(KRem, "removed");
	__KTRACE_OPT(KUSB, Kern::Printf(" > USB cable now %S", ptr->iCableConnected ? &KIns : &KRem));
#endif
	if (ptr->iCableConnected)
		{
		ptr->DeviceEventNotification(EUsbEventCableInserted);
		}
	else
		{
		ptr->DeviceEventNotification(EUsbEventCableRemoved);
		}

	return KErrNone;
	}


TInt TTemplateAsspUsbcc::SetupUdcInterrupt()
//
// Registers and enables the UDC interrupt (ASSP first level interrupt).
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::SetupUdcInterrupt"));

	// Register UDC interrupt:
	const TInt error = Interrupt::Bind(EAsspIntIdUsb, UdcIsr, this);
	if (error != KErrNone)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Binding UDC interrupt failed"));
		return error;
		}

	// Enable UDC interrupt:
	Interrupt::Enable(EAsspIntIdUsb);

	return KErrNone;
	}


void TTemplateAsspUsbcc::ReleaseUdcInterrupt()
//
// Disables and unbinds the UDC interrupt.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TTemplateAsspUsbcc::ReleaseUdcInterrupt"));

	// Disable UDC interrupt:
	Interrupt::Disable(EAsspIntIdUsb);

	// Unregister UDC interrupt:
	Interrupt::Unbind(EAsspIntIdUsb);
	}


//
// --- DLL Exported Function --------------------------------------------------
//

DECLARE_STANDARD_EXTENSION()
//
// Creates and initializes a new USB client controller object on the kernel heap.
//
	{
	__KTRACE_OPT(KUSB, Kern::Printf(" > Initializing USB client support (Udcc)..."));

	TTemplateAsspUsbcc* const usbcc = new TTemplateAsspUsbcc();
	if (!usbcc)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Memory allocation for TTemplateAsspUsbcc failed"));
		return KErrNoMemory;
		}

	TInt r;
	if ((r = usbcc->Construct()) != KErrNone)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: Construction of TTemplateAsspUsbcc failed (%d)", r));
		delete usbcc;
		return r;
		}

	if (usbcc->RegisterUdc(0) == NULL)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: PIL registration of PSL failed"));
		delete usbcc;
		return KErrGeneral;
		}

	__KTRACE_OPT(KUSB, Kern::Printf(" > Initializing USB client support: Done"));

	return KErrNone;
	}


// --- EOF --------------------------------------------------------------------
