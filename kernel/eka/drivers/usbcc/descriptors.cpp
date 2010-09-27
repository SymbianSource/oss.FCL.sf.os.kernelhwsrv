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
// e32/drivers/usbcc/descriptors.cpp
// Platform independent layer (PIL) of the USB Device controller driver:
// USB descriptor handling and management.
// 
//

/**
 @file descriptors.cpp
 @internalTechnology
*/

#include <kernel/kern_priv.h>
#include <drivers/usbc.h>
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "descriptorsTraces.h"
#endif



// Debug Support
static const char KUsbPanicCat[] = "USB PIL";


// --- TUsbcDescriptorBase

TUsbcDescriptorBase::TUsbcDescriptorBase()
	:
#ifdef USB_SUPPORTS_SET_DESCRIPTOR_REQUEST
	iIndex(0),
#endif
	iBufPtr(NULL, 0)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORBASE_TUSBCDESCRIPTORBASE_CONS,
	        "TUsbcDescriptorBase::TUsbcDescriptorBase()" );
	}


TUsbcDescriptorBase::~TUsbcDescriptorBase()
	{
    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORBASE_TUSBCDESCRIPTORBASE_DES,
	        "TUsbcDescriptorBase::~TUsbcDescriptorBase()" );
	}


void TUsbcDescriptorBase::SetByte(TInt aPosition, TUint8 aValue)
	{
	iBufPtr[aPosition] = aValue;
	}


void TUsbcDescriptorBase::SetWord(TInt aPosition, TUint16 aValue)
	{
	*reinterpret_cast<TUint16*>(&iBufPtr[aPosition]) = SWAP_BYTES_16(aValue);
	}


TUint8 TUsbcDescriptorBase::Byte(TInt aPosition) const
	{
	return iBufPtr[aPosition];
	}


TUint16 TUsbcDescriptorBase::Word(TInt aPosition) const
	{
	return SWAP_BYTES_16(*reinterpret_cast<const TUint16*>(&iBufPtr[aPosition]));
	}


void TUsbcDescriptorBase::GetDescriptorData(TDes8& aBuffer) const
	{
	aBuffer = iBufPtr;
	}


TInt TUsbcDescriptorBase::GetDescriptorData(TUint8* aBuffer) const
	{
	memcpy(aBuffer, iBufPtr.Ptr(), Size());
	return Size();
	}


TInt TUsbcDescriptorBase::GetDescriptorData(TUint8* aBuffer, TUint aMaxSize) const
	{
	if (aMaxSize < Size())
		{
		// No use to copy only half a descriptor
		return 0;
		}
	return GetDescriptorData(aBuffer);
	}


const TDes8& TUsbcDescriptorBase::DescriptorData() const
	{
	return iBufPtr;
	}


TDes8& TUsbcDescriptorBase::DescriptorData()
	{
	return iBufPtr;
	}


TUint TUsbcDescriptorBase::Size() const
	{
	return iBufPtr.Size();
	}


TUint8 TUsbcDescriptorBase::Type() const
	{
	return iBufPtr[1];
	}


void TUsbcDescriptorBase::UpdateFs()
	{
	// virtual function can be overridden in derived classes.
	return;
	}


void TUsbcDescriptorBase::UpdateHs()
	{
	// virtual function can be overridden in derived classes.
	return;
	}


void TUsbcDescriptorBase::SetBufferPointer(const TDesC8& aDes)
	{
	iBufPtr.Set(const_cast<TUint8*>(aDes.Ptr()), aDes.Size(), aDes.Size());
	}


// --- TUsbcDeviceDescriptor

TUsbcDeviceDescriptor::TUsbcDeviceDescriptor()
	: iBuf()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDEVICEDESCRIPTOR_TUSBCDEVICEDESCRIPTOR_CONS, 
	        "TUsbcDeviceDescriptor::TUsbcDeviceDescriptor()" );
	}


TUsbcDeviceDescriptor* TUsbcDeviceDescriptor::New(TUint8 aDeviceClass, TUint8 aDeviceSubClass,
												  TUint8 aDeviceProtocol, TUint8 aMaxPacketSize0,
												  TUint16 aVendorId, TUint16 aProductId,
												  TUint16 aDeviceRelease, TUint8 aNumConfigurations)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDEVICEDESCRIPTOR_NEW, 
	        "TUsbcDeviceDescriptor::New()" );
	TUsbcDeviceDescriptor* self = new TUsbcDeviceDescriptor();
	if (self)
		{
		if (self->Construct(aDeviceClass, aDeviceSubClass, aDeviceProtocol, aMaxPacketSize0, aVendorId,
							aProductId, aDeviceRelease, aNumConfigurations) != KErrNone)
			{
			delete self;
			return NULL;
			}
		}
	return self;
	}


TInt TUsbcDeviceDescriptor::Construct(TUint8 aDeviceClass, TUint8 aDeviceSubClass, TUint8 aDeviceProtocol,
									  TUint8 aMaxPacketSize0, TUint16 aVendorId, TUint16 aProductId,
									  TUint16 aDeviceRelease, TUint8 aNumConfigurations)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDEVICEDESCRIPTOR_CONSTRUCT,
	        "TUsbcDeviceDescriptor::Construct()" );
	iBuf.SetMax();
	SetBufferPointer(iBuf);
	iBuf[0] = iBuf.Size();									// bLength
	iBuf[1] = KUsbDescType_Device;							// bDescriptorType
	SetWord(2, KUsbcUsbVersion);							// bcdUSB
	iBuf[4] = aDeviceClass;									// bDeviceClass
	iBuf[5] = aDeviceSubClass;								// bDeviceSubClass
	iBuf[6] = aDeviceProtocol;								// bDeviceProtocol
	iBuf[7] = aMaxPacketSize0;								// bMaxPacketSize0
	SetWord(8, aVendorId);									// idVendor
	SetWord(10, aProductId);								// idProduct
	SetWord(12, aDeviceRelease);							// bcdDevice
	iBuf[14] = 0;											// iManufacturer
	iBuf[15] = 0;											// iProduct
	iBuf[16] = 0;											// iSerialNumber
	iBuf[17] = aNumConfigurations;							// bNumConfigurations
	iEp0Size_Fs = aMaxPacketSize0;
	return KErrNone;
	}


void TUsbcDeviceDescriptor::UpdateFs()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDEVICEDESCRIPTOR_UPDATEFS, 
	        "TUsbcDeviceDescriptor::UpdateFs()" );
	SetByte(7, iEp0Size_Fs);								// bMaxPacketSize0
	}


void TUsbcDeviceDescriptor::UpdateHs()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDEVICEDESCRIPTOR_UPDATEHS,
	        "TUsbcDeviceDescriptor::UpdateHs()" );
	SetByte(7, 64);											// bMaxPacketSize0
	}


// --- TUsbcDeviceQualifierDescriptor

TUsbcDeviceQualifierDescriptor::TUsbcDeviceQualifierDescriptor()
	: iBuf()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDEVICEQUALIFIERDESCRIPTOR_TUSBCDEVICEQUALIFIERDESCRIPTOR_CONS,
	        "TUsbcDeviceQualifierDescriptor::TUsbcDeviceQualifierDescriptor()" );
	}


TUsbcDeviceQualifierDescriptor* TUsbcDeviceQualifierDescriptor::New(TUint8 aDeviceClass,
																	TUint8 aDeviceSubClass,
																	TUint8 aDeviceProtocol,
																	TUint8 aMaxPacketSize0,
																	TUint8 aNumConfigurations,
																	TUint8 aReserved)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDEVICEQUALIFIERDESCRIPTOR_NEW, 
	        "TUsbcDeviceQualifierDescriptor::New()" );
	TUsbcDeviceQualifierDescriptor* self = new TUsbcDeviceQualifierDescriptor();
	if (self)
		{
		if (self->Construct(aDeviceClass, aDeviceSubClass, aDeviceProtocol, aMaxPacketSize0,
							aNumConfigurations, aReserved) != KErrNone)
			{
			delete self;
			return NULL;
			}
		}
	return self;
	}


TInt TUsbcDeviceQualifierDescriptor::Construct(TUint8 aDeviceClass, TUint8 aDeviceSubClass,
											   TUint8 aDeviceProtocol, TUint8 aMaxPacketSize0,
											   TUint8 aNumConfigurations, TUint8 aReserved)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDEVICEQUALIFIERDESCRIPTOR_CONSTRUCT,
	        "TUsbcDeviceQualifierDescriptor::Construct()" );
	iBuf.SetMax();
	SetBufferPointer(iBuf);
	iBuf[0] = iBuf.Size();									// bLength
	iBuf[1] = KUsbDescType_DeviceQualifier;					// bDescriptorType
	SetWord(2, KUsbcUsbVersion);							// bcdUSB
	iBuf[4] = aDeviceClass;									// bDeviceClass
	iBuf[5] = aDeviceSubClass;								// bDeviceSubClass
	iBuf[6] = aDeviceProtocol;								// bDeviceProtocol
	iBuf[7] = aMaxPacketSize0;								// bMaxPacketSize0
	iBuf[8] = aNumConfigurations;							// bNumConfigurations
	if (aReserved) aReserved = 0;
	iBuf[9] = aReserved;									// Reserved for future use, must be zero
	iEp0Size_Fs = aMaxPacketSize0;
	return KErrNone;
	}


void TUsbcDeviceQualifierDescriptor::UpdateFs()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDEVICEQUALIFIERDESCRIPTOR_UPDATEFS,
	        "TUsbcDeviceQualifierDescriptor::UpdateFs()" );
	// Here we do exactly the opposite of what's done in the Device descriptor (as this one's
	// documenting the 'other than the current speed').
	SetByte(7, 64);											// bMaxPacketSize0
	}


void TUsbcDeviceQualifierDescriptor::UpdateHs()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDEVICEQUALIFIERDESCRIPTOR_UPDATEHS,
	        "TUsbcDeviceQualifierDescriptor::UpdateHs()" );
	// Here we do exactly the opposite of what's done in the Device descriptor (as this one's
	// documenting the 'other than the current speed').
	SetByte(7, iEp0Size_Fs);								// bMaxPacketSize0
	}


// --- TUsbcConfigDescriptor

TUsbcConfigDescriptor::TUsbcConfigDescriptor()
	: iBuf()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCCONFIGDESCRIPTOR_TUSBCCONFIGDESCRIPTOR_CONS,
	        "TUsbcConfigDescriptor::TUsbcConfigDescriptor()" );
	}


TUsbcConfigDescriptor* TUsbcConfigDescriptor::New(TUint8 aConfigurationValue, TBool aSelfPowered,
												  TBool aRemoteWakeup, TUint16 aMaxPower)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCCONFIGDESCRIPTOR_NEW,
	        "TUsbcConfigDescriptor::New()" );
	TUsbcConfigDescriptor* self = new TUsbcConfigDescriptor();
	if (self)
		{
		if (self->Construct(aConfigurationValue, aSelfPowered, aRemoteWakeup, aMaxPower) != KErrNone)
			{
			delete self;
			return NULL;
			}
		}
	return self;
	}


TInt TUsbcConfigDescriptor::Construct(TUint8 aConfigurationValue, TBool aSelfPowered,
									   TBool aRemoteWakeup, TUint16 aMaxPower)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCCONFIGDESCRIPTOR_CONSTRUCT,
	        "TUsbcConfigDescriptor::Construct()" );
	iBuf.SetMax();
	SetBufferPointer(iBuf);
	iBuf[0] = iBuf.Size();									// bLength
	iBuf[1] = KUsbDescType_Config;							// bDescriptorType
	SetWord(2, KUsbDescSize_Config);						// wTotalLength
	iBuf[4] = 0;											// bNumInterfaces
	iBuf[5] = aConfigurationValue;							// bConfigurationValue
	iBuf[6] = 0;											// iConfiguration
	iBuf[7] = 0x80 |
		(aSelfPowered ? KUsbDevAttr_SelfPowered : 0) |
		(aRemoteWakeup ? KUsbDevAttr_RemoteWakeup : 0);		// bmAttributes (bit 7 always 1)
	if (aMaxPower > 510)
		OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, DUP1_TUSBCCONFIGDESCRIPTOR_CONSTRUCT_DUP1,
		        "  Error: Invalid value for bMaxPower: %d", aMaxPower );
	iBuf[8] = aMaxPower / 2;								// bMaxPower (2mA units!)
	return KErrNone;
	}


// --- TUsbcInterfaceDescriptor

TUsbcInterfaceDescriptor::TUsbcInterfaceDescriptor()
	: iBuf()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCINTERFACEDESCRIPTOR_TUSBCINTERFACEDESCRIPTOR_CONS,
	        "TUsbcInterfaceDescriptor::TUsbcInterfaceDescriptor()" );
	}


TUsbcInterfaceDescriptor* TUsbcInterfaceDescriptor::New(TUint8 aInterfaceNumber, TUint8 aAlternateSetting,
														TInt aNumEndpoints, const TUsbcClassInfo& aClassInfo)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCINTERFACEDESCRIPTOR_NEW,
	        "TUsbcInterfaceDescriptor::New()" );
	TUsbcInterfaceDescriptor* self = new TUsbcInterfaceDescriptor();
	if (self)
		{
		if (self->Construct(aInterfaceNumber, aAlternateSetting, aNumEndpoints, aClassInfo) != KErrNone)
			{
			delete self;
			return NULL;
			}
		}
	return self;
	}


TInt TUsbcInterfaceDescriptor::Construct(TUint8 aInterfaceNumber, TUint8 aAlternateSetting,
										 TInt aNumEndpoints, const TUsbcClassInfo& aClassInfo)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCINTERFACEDESCRIPTOR_CONSTRUCT,
	        "TUsbcInterfaceDescriptor::Construct()" );
	iBuf.SetMax();
	SetBufferPointer(iBuf);
	iBuf[0] = iBuf.Size();									// bLength
	iBuf[1] = KUsbDescType_Interface;						// bDescriptorType
	iBuf[2] = aInterfaceNumber;								// bInterfaceNumber
	iBuf[3] = aAlternateSetting;							// bAlternateSetting
	iBuf[4] = aNumEndpoints;								// bNumEndpoints
	iBuf[5] = aClassInfo.iClassNum;							// bInterfaceClass
	iBuf[6] = aClassInfo.iSubClassNum;						// bInterfaceSubClass
	iBuf[7] = aClassInfo.iProtocolNum;						// bInterfaceProtocol
	iBuf[8] = 0;											// iInterface
	return KErrNone;
	}


// --- TUsbcEndpointDescriptorBase

TUsbcEndpointDescriptorBase::TUsbcEndpointDescriptorBase()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCENDPOINTDESCRIPTORBASE_TUSBCENDPOINTDESCRIPTORBASE_CONS,
	        "TUsbcEndpointDescriptorBase::TUsbcEndpointDescriptorBase()" );
	}


TInt TUsbcEndpointDescriptorBase::Construct(const TUsbcEndpointInfo& aEpInfo)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCENDPOINTDESCRIPTORBASE_CONSTRUCT,
	        "TUsbcEndpointDescriptorBase::Construct()" );
	//  Adjust FS/HS endpoint sizes
	if (aEpInfo.AdjustEpSizes(iEpSize_Fs, iEpSize_Hs) != KErrNone)
		{
		OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCENDPOINTDESCRIPTORBASE_CONSTRUCT_DUP1,
		        "  Error: Unknown endpoint type: %d", aEpInfo.iType );
		}
	OstTraceDefExt3( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINTDESCRIPTORBASE_CONSTRUCT_DUP2,
	        "  Now set: iEpSize_Fs=%d iEpSize_Hs=%d (aEpInfo.iSize=%d)",
	        iEpSize_Fs, iEpSize_Hs, aEpInfo.iSize );

	//  Adjust HS endpoint size for additional transactions
	if ((aEpInfo.iType == KUsbEpTypeIsochronous) || (aEpInfo.iType == KUsbEpTypeInterrupt))
		{
		if ((aEpInfo.iTransactions > 0) && (aEpInfo.iTransactions < 3))
			{
			// Bits 12..11 specify the number of additional transactions per microframe
			iEpSize_Hs |= (aEpInfo.iTransactions << 12);
			 OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCENDPOINTDESCRIPTORBASE_CONSTRUCT_DUP3,
			         "  Adjusted for add. transact.: iEpSize_Hs=0x%02x (aEpInfo.iTransactions=%d)",
			         iEpSize_Hs, aEpInfo.iTransactions );

			}
		else if (aEpInfo.iTransactions != 0)
			{
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCENDPOINTDESCRIPTORBASE_CONSTRUCT_DUP4,
                    "  Warning: Invalid iTransactions value: %d (ignored)", aEpInfo.iTransactions );

			}
		}

	//  Adjust HS polling interval
	TUsbcEndpointInfo info(aEpInfo);						// create local writeable copy
	if (info.AdjustPollInterval() != KErrNone)
		{
        OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCENDPOINTDESCRIPTORBASE_CONSTRUCT_DUP5,
                "  Error: Unknown ep type (%d) or invalid interval value (%d)", info.iType, info.iInterval );
		}
	iInterval_Fs = info.iInterval;
	iInterval_Hs = info.iInterval_Hs;
    OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCENDPOINTDESCRIPTORBASE_CONSTRUCT_DUP6,
            "  Now set: iInterval_Fs=%d iInterval_Hs=%d", iInterval_Fs, iInterval_Hs );

	return KErrNone;
	}


void TUsbcEndpointDescriptorBase::UpdateFs()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCENDPOINTDESCRIPTORBASE_UPDATEFS,
	        "TUsbcEndpointDescriptorBase::UpdateFs" );
	// (TUsbcEndpointDescriptorBase's FS/HS endpoint sizes and interval values got
	//  adjusted in its Construct() method.)
	SetWord(4, iEpSize_Fs);									// wMaxPacketSize
	SetByte(6, iInterval_Fs);								// bInterval
	}


void TUsbcEndpointDescriptorBase::UpdateHs()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCENDPOINTDESCRIPTORBASE_UPDATEHS,
	        "TUsbcEndpointDescriptorBase::UpdateHs()" );
	// (TUsbcEndpointDescriptorBase's FS/HS endpoint sizes and interval values get
	//  adjusted in its Construct() method.)
	SetWord(4, iEpSize_Hs);									// wMaxPacketSize
	SetByte(6, iInterval_Hs);								// bInterval
	}


// --- TUsbcEndpointDescriptor

TUsbcEndpointDescriptor::TUsbcEndpointDescriptor()
	: iBuf()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCENDPOINTDESCRIPTOR_TUSBCENDPOINTDESCRIPTOR_CONS,
	        "TUsbcEndpointDescriptor::TUsbcEndpointDescriptor()" );
	}


TUsbcEndpointDescriptor* TUsbcEndpointDescriptor::New(TUint8 aEndpointAddress,
													  const TUsbcEndpointInfo& aEpInfo)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCENDPOINTDESCRIPTOR_NEW,
	        "TUsbcEndpointDescriptor::New()" );
	TUsbcEndpointDescriptor* self = new TUsbcEndpointDescriptor();
	if (self)
		{
		if (self->Construct(aEndpointAddress, aEpInfo) != KErrNone)
			{
			delete self;
			return NULL;
			}
		}
	return self;
	}


TInt TUsbcEndpointDescriptor::Construct(TUint8 aEndpointAddress, const TUsbcEndpointInfo& aEpInfo)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCENDPOINTDESCRIPTOR_CONSTRUCT,
	        "TUsbcEndpointDescriptor::Construct()" );
	(void) TUsbcEndpointDescriptorBase::Construct(aEpInfo);	// Init Base class
	iBuf.SetMax();
	SetBufferPointer(iBuf);
	iBuf[0] = iBuf.Size();									// bLength
	iBuf[1] = KUsbDescType_Endpoint;						// bDescriptorType
	iBuf[2] = aEndpointAddress;								// bEndpointAddress
	iBuf[3] = EpTypeMask2Value(aEpInfo.iType);				// bmAttributes
	SetWord(4, iEpSize_Fs);									// wMaxPacketSize (default is FS)
	iBuf[6] = iInterval_Fs;									// bInterval (default is FS)
	return KErrNone;
	}


// --- TUsbcAudioEndpointDescriptor

TUsbcAudioEndpointDescriptor::TUsbcAudioEndpointDescriptor()
	: iBuf()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCAUDIOENDPOINTDESCRIPTOR_TUSBCAUDIOENDPOINTDESCRIPTOR_CONS,
	        "TUsbcAudioEndpointDescriptor::TUsbcAudioEndpointDescriptor()" );
	}


TUsbcAudioEndpointDescriptor* TUsbcAudioEndpointDescriptor::New(TUint8 aEndpointAddress,
																const TUsbcEndpointInfo& aEpInfo)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCAUDIOENDPOINTDESCRIPTOR_NEW,
	        "TUsbcAudioEndpointDescriptor::New()" );
	TUsbcAudioEndpointDescriptor* self = new TUsbcAudioEndpointDescriptor();
	if (self)
		{
		if (self->Construct(aEndpointAddress, aEpInfo) != KErrNone)
			{
			delete self;
			return NULL;
			}
		}
	return self;
	}


TInt TUsbcAudioEndpointDescriptor::Construct(TUint8 aEndpointAddress, const TUsbcEndpointInfo& aEpInfo)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCAUDIOENDPOINTDESCRIPTOR_CONSTRUCT,
	        "TUsbcAudioEndpointDescriptor::Construct()" );
	(void) TUsbcEndpointDescriptorBase::Construct(aEpInfo);	// Init Base class
	iBuf.SetMax();
	SetBufferPointer(iBuf);
	iBuf[0] = iBuf.Size();									// bLength
	iBuf[1] = KUsbDescType_Endpoint;						// bDescriptorType
	iBuf[2] = aEndpointAddress;								// bEndpointAddress
	iBuf[3] = EpTypeMask2Value(aEpInfo.iType);				// bmAttributes
	SetWord(4, iEpSize_Fs);									// wMaxPacketSize (default is FS)
	iBuf[6] = iInterval_Fs;									// bInterval (default is FS)
	iBuf[7] = 0;
	iBuf[8] = 0;
	return KErrNone;
	}


// --- TUsbcOtgDescriptor

TUsbcOtgDescriptor* TUsbcOtgDescriptor::New(TBool aHnpSupport, TBool aSrpSupport)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCOTGDESCRIPTOR_NEW,
	        "TUsbcOtgDescriptor::New()" );
	TUsbcOtgDescriptor* self = new TUsbcOtgDescriptor();
	if (self && (self->Construct(aHnpSupport, aSrpSupport) != KErrNone))
		{
		delete self;
		return NULL;
		}
	return self;
	}


TUsbcOtgDescriptor::TUsbcOtgDescriptor()
	: iBuf()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCOTGDESCRIPTOR_TUSBCOTGDESCRIPTOR_CONS, 
	        "TUsbcOtgDescriptor::TUsbcOtgDescriptor()" );
	}


TInt TUsbcOtgDescriptor::Construct(TBool aHnpSupport, TBool aSrpSupport)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCOTGDESCRIPTOR_CONSTRUCT,
	        "TUsbcOtgDescriptor::Construct()" );
	iBuf.SetMax();
	SetBufferPointer(iBuf);
	iBuf[0] = iBuf.Size();									// bLength
	iBuf[1] = KUsbDescType_Otg;								// bDescriptorType
  // B HNP not supported which is temporarily hard coded here. 
	iBuf[2] = ( aHnpSupport ? 0 : 0 ) | ( aSrpSupport ? KUsbOtgAttr_SrpSupp : 0 );			// bmAttributes			
	iBuf[3] = KUsbOtgDesc_bcdOTG & 0x00ff;
  iBuf[4] = ( KUsbOtgDesc_bcdOTG >> 8 ) & 0x00ff;
		
	return KErrNone;
    }


// --- TUsbcClassSpecificDescriptor

TUsbcClassSpecificDescriptor::TUsbcClassSpecificDescriptor()
	: iBuf(NULL)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCCLASSSPECIFICDESCRIPTOR_TUSBCCLASSSPECIFICDESCRIPTOR_CONS,
	        "TUsbcClassSpecificDescriptor::TUsbcClassSpecificDescriptor()" );
	}


TUsbcClassSpecificDescriptor::~TUsbcClassSpecificDescriptor()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCCLASSSPECIFICDESCRIPTOR_TUSBCCLASSSPECIFICDESCRIPTOR_DES,
	        "TUsbcClassSpecificDescriptor::~TUsbcClassSpecificDescriptor()" );
	delete iBuf;
	}


TUsbcClassSpecificDescriptor* TUsbcClassSpecificDescriptor::New(TUint8 aType, TInt aSize)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCCLASSSPECIFICDESCRIPTOR_NEW,
	        "TUsbcClassSpecificDescriptor::New()" );
	TUsbcClassSpecificDescriptor* self = new TUsbcClassSpecificDescriptor();
	if (self)
		{
		if (self->Construct(aType, aSize) != KErrNone)
			{
			delete self;
			return NULL;
			}
		}
	return self;
	}


TInt TUsbcClassSpecificDescriptor::Construct(TUint8 aType, TInt aSize)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCCLASSSPECIFICDESCRIPTOR_CONSTRUCT, 
	        "TUsbcClassSpecificDescriptor::Construct()" );
	iBuf = HBuf8::New(aSize);
	if (!iBuf)
		{
		OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCCLASSSPECIFICDESCRIPTOR_CONSTRUCT_DUP1,
		        "  Error: Allocation of CS desc buffer failed" );
		return KErrNoMemory;
		}
	iBuf->SetMax();
	SetBufferPointer(*iBuf);
	SetByte(1, aType);										// bDescriptorType
	return KErrNone;
	}


// --- TUsbcStringDescriptorBase

TUsbcStringDescriptorBase::TUsbcStringDescriptorBase()
	: /*iIndex(0),*/ iSBuf(0), iBufPtr(NULL, 0)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCSTRINGDESCRIPTORBASE_TUSBCSTRINGDESCRIPTORBASE_CONS,
	        "TUsbcStringDescriptorBase::TUsbcStringDescriptorBase()" );
	}


TUsbcStringDescriptorBase::~TUsbcStringDescriptorBase()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCSTRINGDESCRIPTORBASE_TUSBCSTRINGDESCRIPTORBASE_DES, 
	        "TUsbcStringDescriptorBase::~TUsbcStringDescriptorBase()" );
	}


TUint16 TUsbcStringDescriptorBase::Word(TInt aPosition) const
	{
	if (aPosition <= 1)
		{
		OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCSTRINGDESCRIPTORBASE_WORD,
		        "  Error: Word(%d) in string descriptor (TUsbcStringDescriptorBase::Word)", aPosition );
		return 0;
		}
	else
		{
		// since iBufPtr[0] is actually string descriptor byte index 2,
		// we have to subtract 2 from the absolute position.
		return SWAP_BYTES_16(*reinterpret_cast<const TUint16*>(&iBufPtr[aPosition - 2]));
		}
	}


void TUsbcStringDescriptorBase::SetWord(TInt aPosition, TUint16 aValue)
	{
	if (aPosition <= 1)
		{
		OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCSTRINGDESCRIPTORBASE_SETWORD,
		        "  Error: SetWord(%d) in string descriptor (TUsbcStringDescriptorBase::SetWord)", aPosition );
		return;
		}
	else
		{
		// since iBufPtr[0] is actually string descriptor byte index 2,
		// we have to subtract 2 from the absolute position.
		*reinterpret_cast<TUint16*>(&iBufPtr[aPosition - 2]) = SWAP_BYTES_16(aValue);
		}
	}


TInt TUsbcStringDescriptorBase::GetDescriptorData(TUint8* aBuffer) const
	{
	aBuffer[0] = iSBuf[0];
	aBuffer[1] = iSBuf[1];
	memcpy(&aBuffer[2], iBufPtr.Ptr(), iBufPtr.Size());
	return Size();
	}


TInt TUsbcStringDescriptorBase::GetDescriptorData(TUint8* aBuffer, TUint aMaxSize) const
	{
	if (aMaxSize < Size())
		{
		// No use to copy only half a string
		return 0;
		}
	return GetDescriptorData(aBuffer);
	}


const TDes8& TUsbcStringDescriptorBase::StringData() const
	{
	return iBufPtr;
	}


TDes8& TUsbcStringDescriptorBase::StringData()
	{
	return iBufPtr;
	}


TUint TUsbcStringDescriptorBase::Size() const
	{
	return iSBuf[0];
	}


void TUsbcStringDescriptorBase::SetBufferPointer(const TDesC8& aDes)
	{
	iBufPtr.Set(const_cast<TUint8*>(aDes.Ptr()), aDes.Size(), aDes.Size());
	}


// --- TUsbcStringDescriptor

TUsbcStringDescriptor::TUsbcStringDescriptor()
	: iBuf(NULL)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCSTRINGDESCRIPTOR_TUSBCSTRINGDESCRIPTOR_CONS, 
	        "TUsbcStringDescriptor::TUsbcStringDescriptor()" );
	}


TUsbcStringDescriptor::~TUsbcStringDescriptor()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCSTRINGDESCRIPTOR_TUSBCSTRINGDESCRIPTOR_DES,
	        "TUsbcStringDescriptor::~TUsbcStringDescriptor()" );
	delete iBuf;
	}


TUsbcStringDescriptor* TUsbcStringDescriptor::New(const TDesC8& aString)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCSTRINGDESCRIPTOR_NEW, 
	        "TUsbcStringDescriptor::New()" );
	TUsbcStringDescriptor* self = new TUsbcStringDescriptor();
	if (self)
		{
		if (self->Construct(aString) != KErrNone)
			{
			delete self;
			return NULL;
			}
		}
	return self;
	}


TInt TUsbcStringDescriptor::Construct(const TDesC8& aString)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCSTRINGDESCRIPTOR_CONSTRUCT,
	        "TUsbcStringDescriptor::Construct()" );
	iBuf = HBuf8::New(aString.Size());						// bytes, not UNICODE chars
	if (!iBuf)
		{
		OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCSTRINGDESCRIPTOR_CONSTRUCT_DUP1,
		        "  Error: Allocation of string buffer failed" );
		return KErrNoMemory;
		}
	iBuf->SetMax();
	SetBufferPointer(*iBuf);
	iBufPtr.Copy(aString);
	iSBuf.SetMax();
	iSBuf[0] = iBuf->Size() + 2;							// Bytes
	iSBuf[1] = KUsbDescType_String;
	return KErrNone;
	}


// --- TUsbcLangIdDescriptor

TUsbcLangIdDescriptor::TUsbcLangIdDescriptor()
	: iBuf(NULL)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCLANGIDDESCRIPTOR_TUSBCLANGIDDESCRIPTOR_CONS, 
	        "TUsbcLangIdDescriptor::TUsbcLangIdDescriptor()" );
	}


TUsbcLangIdDescriptor::~TUsbcLangIdDescriptor()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCLANGIDDESCRIPTOR_TUSBCLANGIDDESCRIPTOR_DES,
	        "TUsbcLangIdDescriptor::~TUsbcLangIdDescriptor()" );
	}


TUsbcLangIdDescriptor* TUsbcLangIdDescriptor::New(TUint16 aLangId)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCLANGIDDESCRIPTOR_NEW, 
	        "TUsbcLangIdDescriptor::New()" );
	TUsbcLangIdDescriptor* self = new TUsbcLangIdDescriptor();
	if (self)
		{
		if (self->Construct(aLangId) != KErrNone)
			{
			delete self;
			return NULL;
			}
		}
	return self;
	}


TInt TUsbcLangIdDescriptor::Construct(TUint16 aLangId)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCLANGIDDESCRIPTOR_CONSTRUCT,
	        "TUsbcLangIdDescriptor::Construct()" );
	iBuf.SetMax();
	SetBufferPointer(iBuf);
	iBufPtr[0] = LowByte(SWAP_BYTES_16(aLangId));			// Language ID value
	iBufPtr[1] = HighByte(SWAP_BYTES_16(aLangId));
	iSBuf.SetMax();
	iSBuf[0] = iBuf.Size() + 2;								// Bytes
	iSBuf[1] = KUsbDescType_String;
	return KErrNone;
	}


// --- TUsbcDescriptorPool

TUsbcDescriptorPool::TUsbcDescriptorPool(TUint8* aEp0_TxBuf)
//
//	The constructor for this class.
//
	: iDescriptors(), iStrings(), iIfcIdx(0), iEp0_TxBuf(aEp0_TxBuf), iHighSpeed(EFalse)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_TUSBCDESCRIPTORPOOL_CONS,
	        "TUsbcDescriptorPool::TUsbcDescriptorPool()" );
	}


TUsbcDescriptorPool::~TUsbcDescriptorPool()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_TUSBCDESCRIPTORPOOL_DES, 
	        "TUsbcDescriptorPool::~TUsbcDescriptorPool()" );
	// The destructor of each <class T> object is called before the objects themselves are destroyed.
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_TUSBCDESCRIPTORPOOL_DES_DUP1,
	        "  iDescriptors.Count(): %d", iDescriptors.Count() );
	iDescriptors.ResetAndDestroy();
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_TUSBCDESCRIPTORPOOL_DES_DUP2,
            "  iStrings.Count(): %d", iStrings.Count() );

	iStrings.ResetAndDestroy();
	}


TInt TUsbcDescriptorPool::Init(TUsbcDeviceDescriptor* aDeviceDesc, TUsbcConfigDescriptor* aConfigDesc,
							   TUsbcLangIdDescriptor* aLangId, TUsbcStringDescriptor* aManufacturer,
							   TUsbcStringDescriptor* aProduct, TUsbcStringDescriptor* aSerialNum,
							   TUsbcStringDescriptor* aConfig, TUsbcOtgDescriptor* aOtgDesc)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_INIT, 
	        "TUsbcDescriptorPool::Init()" );
	if (!aDeviceDesc || !aConfigDesc)
		{
		OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_INIT_DUP1, 
		        "  Error: No Device or Config descriptor specified" );
		return KErrArgument;
		}
	for (TInt n = 0; n < KDescPosition_FirstAvailable; n++)
		{
		iDescriptors.Append(NULL);
		}
#ifdef _DEBUG
	if (iDescriptors.Count() != KDescPosition_FirstAvailable)
	    {
        OstTraceDefExt2( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_INIT_DUP2,
                "  Error: iDescriptors.Count() (%d) != KDescPosition_FirstAvailable (%d)",
                iDescriptors.Count(), KDescPosition_FirstAvailable );

	    }
#endif
	iDescriptors[KDescPosition_Device] = aDeviceDesc;
	iDescriptors[KDescPosition_Config] = aConfigDesc;
	if (aOtgDesc)
		{
		iDescriptors[KDescPosition_Otg] = aOtgDesc;
		// Update the config descriptor's wTotalLength field
		UpdateConfigDescriptorLength(KUsbDescSize_Otg);
		}
	if (!aLangId)
		{
		// USB spec 9.6.7 says: "String index zero for all languages returns a string descriptor
		// that contains an array of two-byte LANGID codes supported by the device. ...
		// USB devices that omit all string descriptors must not return an array of LANGID codes."
		// So if we have at least one string descriptor, we must also have a LANGID descriptor.
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_INIT_DUP3, 
                "  Error: No LANGID string descriptor specified" );

		return KErrArgument;
		}
	iStrings.Insert(aLangId, KStringPosition_Langid);
	iStrings.Insert(aManufacturer, KStringPosition_Manufact);
	iStrings.Insert(aProduct, KStringPosition_Product);
	iStrings.Insert(aSerialNum, KStringPosition_Serial);
	iStrings.Insert(aConfig, KStringPosition_Config);
#ifdef _DEBUG
	if (iStrings.Count() != 5)
	    {
        OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_INIT_DUP4,
                "  Error: iStrings.Count() != 5 (%d)", iStrings.Count() );
	    }
#endif
#ifdef _DEBUG
	for (TInt i = KStringPosition_Langid; i <= KStringPosition_Config; i++)
		{
        OstTraceDefExt2( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_INIT_DUP5,
                "TUsbcDescriptorPool.iStrings[%d] = 0x%x", i, reinterpret_cast<TUint>(iStrings[i]) );
		}
#endif
	// Set string indices
	if (aManufacturer)
		iDescriptors[KDescPosition_Device]->SetByte(KUsbDescStringIndex_Manufact,
													KStringPosition_Manufact);
	if (aProduct)
		iDescriptors[KDescPosition_Device]->SetByte(KUsbDescStringIndex_Product,
													KStringPosition_Product);
	if (aSerialNum)
		iDescriptors[KDescPosition_Device]->SetByte(KUsbDescStringIndex_Serial,
													KStringPosition_Serial);
	if (aConfig)
		iDescriptors[KDescPosition_Config]->SetByte(KUsbDescStringIndex_Config,
													KStringPosition_Config);
	return KErrNone;
	}


TInt TUsbcDescriptorPool::InitHs()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_INITHS, 
	        "TUsbcDescriptorPool::InitHs()" );
#ifdef _DEBUG
	if (iDescriptors.Count() < KDescPosition_FirstAvailable)
	    {
        OstTraceDef0( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_INITHS_DUP1, 
                "  Error: Call Init() first)" );

	    }
#endif

	TUsbcDeviceQualifierDescriptor* const dq_desc = TUsbcDeviceQualifierDescriptor::New(
		iDescriptors[KDescPosition_Device]->Byte(4),		// aDeviceClass
		iDescriptors[KDescPosition_Device]->Byte(5),		// aDeviceSubClass
		iDescriptors[KDescPosition_Device]->Byte(6),		// aDeviceProtocol
		iDescriptors[KDescPosition_Device]->Byte(7),		// aMaxPacketSize0
		iDescriptors[KDescPosition_Device]->Byte(17));		// aNumConfigurations
	if (!dq_desc)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_INITHS_DUP2,
                "  Error: Memory allocation for dev qualif desc failed." );
		return KErrGeneral;
		}
	iDescriptors[KDescPosition_DeviceQualifier] = dq_desc;

	TUsbcOtherSpeedConfigDescriptor* const osc_desc = TUsbcOtherSpeedConfigDescriptor::New(
		iDescriptors[KDescPosition_Config]->Byte(5),		// aConfigurationValue
		iDescriptors[KDescPosition_Config]->Byte(7) & KUsbDevAttr_SelfPowered, // aSelfPowered
		iDescriptors[KDescPosition_Config]->Byte(7) & KUsbDevAttr_RemoteWakeup,	// aRemoteWakeup
		iDescriptors[KDescPosition_Config]->Byte(8) * 2);	// aMaxPower (mA)
	if (!osc_desc)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_INITHS_DUP3,
                "  Error: Memory allocation for other speed conf desc failed." );
		return KErrGeneral;
		}

	// We need to set the bDescriptorType field manually, as that's the only one
	// that differs from a Configuration descriptor.
	osc_desc->SetByte(1, KUsbDescType_OtherSpeedConfig);

	// Also, initially we set the iConfiguration string index to the same value as
	// in the Configuration descriptor.
	osc_desc->SetByte(KUsbDescStringIndex_Config,
					  iDescriptors[KDescPosition_Config]->Byte(KUsbDescStringIndex_Config));

	iDescriptors[KDescPosition_OtherSpeedConfig] = osc_desc;

	return KErrNone;
	}


TInt TUsbcDescriptorPool::UpdateDescriptorsFs()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_UPDATEDESCRIPTORSFS,
	        "TUsbcDescriptorPool::UpdateDescriptorsFs()" );
	const TInt count = iDescriptors.Count();
	for (TInt i = KDescPosition_FirstAvailable; i < count; i++)
		{
		TUsbcDescriptorBase* const ptr = iDescriptors[i];
		ptr->UpdateFs();
		}
	iHighSpeed = EFalse;
	return KErrNone;
	}


TInt TUsbcDescriptorPool::UpdateDescriptorsHs()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_UPDATEDESCRIPTORSHS,
	        "TUsbcDescriptorPool::UpdateDescriptorsHs()" );
	const TInt count = iDescriptors.Count();
	for (TInt i = KDescPosition_FirstAvailable; i < count; i++)
		{
		TUsbcDescriptorBase* const ptr = iDescriptors[i];
		ptr->UpdateHs();
		}
	iHighSpeed = ETrue;
	return KErrNone;
	}


//
// An error can be indicated by either a return value != KErrNone or by a descriptor size == 0.
//
TInt TUsbcDescriptorPool::FindDescriptor(TUint8 aType, TUint8 aIndex, TUint16 aLangid, TInt& aSize) const
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_FINDDESCRIPTOR, 
	        "TUsbcDescriptorPool::FindDescriptor()" );
	TInt result = KErrGeneral;
	switch (aType)
		{
	case KUsbDescType_Device:
		if (aLangid != 0)
			{
		 OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_FINDDESCRIPTOR_DUP1,
		         "  Error: bad langid: 0x%04x", aLangid );
			}
		else if (aIndex > 0)
			{
	       OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_FINDDESCRIPTOR_DUP2,
	               "  Error: bad device index: %d", aIndex );
			}
		else
			{
			aSize = GetDeviceDescriptor(KDescPosition_Device);
			result = KErrNone;
			}
		break;
	case KUsbDescType_Config:
		if (aLangid != 0)
			{
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_FINDDESCRIPTOR_DUP3,
                    "  Error: bad langid: 0x%04x", aLangid );
			}
		else if (aIndex > 0)
			{
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_FINDDESCRIPTOR_DUP4,
                    "  Error: bad config index: %d", aIndex );
			}
		else
			{
			aSize = GetConfigurationDescriptor(KDescPosition_Config);
			result = KErrNone;
			}
		break;
	case KUsbDescType_DeviceQualifier:
		if (aLangid != 0)
			{
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_FINDDESCRIPTOR_DUP5,
                    "  Error: bad langid: 0x%04x", aLangid );
			}
		else if (aIndex > 0)
			{
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_FINDDESCRIPTOR_DUP6,
                    "  Error: bad device index: %d", aIndex );
			}
		else
			{
			aSize = GetDeviceDescriptor(KDescPosition_DeviceQualifier);
			result = KErrNone;
			}
		break;
	case KUsbDescType_OtherSpeedConfig:
		if (aLangid != 0)
			{
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_FINDDESCRIPTOR_DUP7,
                    "  Error: bad langid: 0x%04x", aLangid );
			}
		else if (aIndex > 0)
			{
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_FINDDESCRIPTOR_DUP8,
                    "  Error: bad config index: %d", aIndex );
			}
		else
			{
			aSize = GetConfigurationDescriptor(KDescPosition_OtherSpeedConfig);
			result = KErrNone;
			}
		break;
	case KUsbDescType_Otg:
		aSize = GetOtgDescriptor();
		result = KErrNone;
		break;
	case KUsbDescType_String:
		if (aIndex == 0)									// 0 addresses the LangId array
			{
			if (AnyStringDescriptors())
				{
				aSize = GetStringDescriptor(aIndex);
				result = KErrNone;
				}
			else
				{
                OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_FINDDESCRIPTOR_DUP9,
                        "  No string descriptors: not returning LANGID array" );
				}
			}
		else
			{
   			if (!aLangid)
   				{
                OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_FINDDESCRIPTOR_DUP10,
                        "  Strange: LANGID=0 for a $ descriptor (ignoring LANGID)" );
				// The USB spec doesn't really say what to do in this case, but as there are host apps
				// that fail if we return an error here, we choose to ignore the issue.
   				}
			else if (aLangid != iStrings[KStringPosition_Langid]->Word(2))
				{
				// We have only one (this) language
                OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_FINDDESCRIPTOR_DUP11,
                        "  Bad LANGID: 0x%04X requested, 0x%04X supported (ignoring LANGID)",
                        aLangid, iStrings[KStringPosition_Langid]->Word(2) );
				// We could return an error here, but rather choose to ignore the discrepancy
				// (the USB spec is not very clear what to do in such a case anyway).
				}
			aSize = GetStringDescriptor(aIndex);
			result = KErrNone;
			}
		break;
	case KUsbDescType_CS_Interface:
		/* fall through */
	case KUsbDescType_CS_Endpoint:
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_FINDDESCRIPTOR_DUP12,
                "  Warning: finding of class specific descriptors not supported" );
		break;
	default:
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_FINDDESCRIPTOR_DUP13,
                "  Error: unknown descriptor type requested: %d", aType );
		break;
		}
	return result;
	}


void TUsbcDescriptorPool::InsertDescriptor(TUsbcDescriptorBase* aDesc)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_INSERTDESCRIPTOR, 
	        "TUsbcDescriptorPool::InsertDescriptor()" );
	switch (aDesc->Type())
		{
	case KUsbDescType_Interface:
		InsertIfcDesc(aDesc);
		break;
	case KUsbDescType_Endpoint:
		InsertEpDesc(aDesc);
		break;
	default:
	    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_INSERTDESCRIPTOR_DUP1,
	            "  Error: unsupported descriptor type" );
		}
	}


void TUsbcDescriptorPool::SetIfcStringDescriptor(TUsbcStringDescriptor* aDesc, TInt aNumber, TInt aSetting)
	{
	OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_SETIFCSTRINGDESCRIPTOR,
	        "TUsbcDescriptorPool::SetIfcDescriptor(%d, %d)", aNumber, aSetting );
	const TInt i = FindIfcDescriptor(aNumber, aSetting);
	if (i < 0)
		{
	    OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETIFCSTRINGDESCRIPTOR_DUP1,
	           "  Error: Ifc descriptor not found (%d, %d)", aNumber, aSetting );
		return;
		}
	// Try to find available NULL postition
	TInt str_idx = FindAvailableStringPos();
	if (str_idx >= 0)
		{
		// Insert string descriptor for specified interface
		ExchangeStringDescriptor(str_idx, aDesc);
		}
	else
		{
		// No NULL found - expand array
		str_idx = iStrings.Count();
		if (str_idx > 0xff)
			{
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETIFCSTRINGDESCRIPTOR_DUP2,
                    "  Error: $ descriptor array full (idx=%d)", str_idx );
			return;
			}
		while (str_idx < KStringPosition_FirstAvailable)
			{
			iStrings.Append(NULL);
			str_idx = iStrings.Count();
			}
		// Append string descriptor for specified interface
		iStrings.Append(aDesc);
		}
	// Update this ifc descriptor's string index field
	iDescriptors[i]->SetByte(8, str_idx);
	OstTraceDefExt4( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_SETIFCSTRINGDESCRIPTOR_DUP3,
	        "  String for ifc %d/%d (@ pos %d): \"%s\"", aNumber, aSetting, str_idx,
	        iStrings[str_idx]->StringData() );
	}


void TUsbcDescriptorPool::DeleteIfcDescriptor(TInt aNumber, TInt aSetting)
	{
	OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_DELETEIFCDESCRIPTOR,
	        "TUsbcDescriptorPool::DeleteIfcDescriptor(%d, %d)", aNumber, aSetting );	        
	const TInt i = FindIfcDescriptor(aNumber, aSetting);
	if (i < 0)
		{
        OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_DELETEIFCDESCRIPTOR_DUP1,
                "  Error: DeleteIfcDescriptor - descriptor not found (%d, %d)", aNumber, aSetting );
		return;
		}
	// Delete (if necessary) specified interface's string descriptor
	const TInt si = iDescriptors[i]->Byte(8);
	if (si != 0)
		{
		ExchangeStringDescriptor(si, NULL);
		}
	// Delete specified ifc setting + all its cs descriptors + all its endpoints + all their cs descriptors:
	// find position of the next interface descriptor: we need to delete everything in between
	const TInt count = iDescriptors.Count();
	TInt j = i, n = 1;
	while (++j < count && iDescriptors[j]->Type() != KUsbDescType_Interface)
		++n;
	DeleteDescriptors(i, n);
	// Update all the following interfaces' bInterfaceNumber field if required
	// (because those descriptors might have moved down by one position)
	UpdateIfcNumbers(aNumber);
	iIfcIdx = 0;											// ifc index no longer valid
	}


// The TC in many of the following functions stands for 'ThreadCopy',
// because that's what's happening there.

TInt TUsbcDescriptorPool::GetDeviceDescriptorTC(DThread* aThread, TDes8& aBuffer) const
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_GETDEVICEDESCRIPTORTC, 
	        "TUsbcDescriptorPool::GetDeviceDescriptorTC()" );
	return Kern::ThreadDesWrite(aThread, &aBuffer, iDescriptors[KDescPosition_Device]->DescriptorData(), 0);
	}


TInt TUsbcDescriptorPool::SetDeviceDescriptorTC(DThread* aThread, const TDes8& aBuffer)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_SETDEVICEDESCRIPTORTC, 
	        "TUsbcDescriptorPool::SetDeviceDescriptorTC()" );
	TBuf8<KUsbDescSize_Device> device;
	const TInt r = Kern::ThreadDesRead(aThread, &aBuffer, device, 0);
	if (r != KErrNone)
		{
		return r;
		}
	iDescriptors[KDescPosition_Device]->SetByte(2, device[2]); // bcdUSB
	iDescriptors[KDescPosition_Device]->SetByte(3, device[3]); // bcdUSB (part II)
	iDescriptors[KDescPosition_Device]->SetByte(4, device[4]); // bDeviceClass
	iDescriptors[KDescPosition_Device]->SetByte(5, device[5]); // bDeviceSubClass
	iDescriptors[KDescPosition_Device]->SetByte(6, device[6]); // bDeviceProtocol
	iDescriptors[KDescPosition_Device]->SetByte(8, device[8]); // idVendor
	iDescriptors[KDescPosition_Device]->SetByte(9, device[9]); // idVendor (part II)
	iDescriptors[KDescPosition_Device]->SetByte(10, device[10]); // idProduct
	iDescriptors[KDescPosition_Device]->SetByte(11, device[11]); // idProduct (part II)
	iDescriptors[KDescPosition_Device]->SetByte(12, device[12]); // bcdDevice
	iDescriptors[KDescPosition_Device]->SetByte(13, device[13]); // bcdDevice (part II)
	return KErrNone;
	}


TInt TUsbcDescriptorPool::GetConfigurationDescriptorTC(DThread* aThread, TDes8& aBuffer) const
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_GETCONFIGURATIONDESCRIPTORTC, 
	        "TUsbcDescriptorPool::GetConfigurationDescriptorTC()" );
	return Kern::ThreadDesWrite(aThread, &aBuffer, iDescriptors[KDescPosition_Config]->DescriptorData(), 0);
	}


TInt TUsbcDescriptorPool::SetConfigurationDescriptorTC(DThread* aThread, const TDes8& aBuffer)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_SETCONFIGURATIONDESCRIPTORTC, 
	        "TUsbcDescriptorPool::SetConfigurationDescriptorTC()" );
	TBuf8<KUsbDescSize_Config> config;
	const TInt r = Kern::ThreadDesRead(aThread, &aBuffer, config, 0);
	if (r != KErrNone)
		{
		return r;
		}
	iDescriptors[KDescPosition_Config]->SetByte(7, config[7]); // bmAttributes
	iDescriptors[KDescPosition_Config]->SetByte(8, config[8]); // bMaxPower
	return KErrNone;
	}


TInt TUsbcDescriptorPool::GetOtgDescriptorTC(DThread* aThread, TDes8& aBuffer) const
	{
	return Kern::ThreadDesWrite(aThread, &aBuffer, iDescriptors[KDescPosition_Otg]->DescriptorData(), 0);
	}


TInt TUsbcDescriptorPool::SetOtgDescriptor(const TDesC8& aBuffer)
	{
	iDescriptors[KDescPosition_Otg]->SetByte(2, aBuffer[2]); // bmAttributes
	return KErrNone;
	}


TInt TUsbcDescriptorPool::GetInterfaceDescriptorTC(DThread* aThread, TDes8& aBuffer,
												   TInt aInterface, TInt aSetting) const
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_GETINTERFACEDESCRIPTORTC,
	        "TUsbcDescriptorPool::GetInterfaceDescriptorTC()" );
	const TInt i = FindIfcDescriptor(aInterface, aSetting);
	if (i < 0)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_GETINTERFACEDESCRIPTORTC_DUP1,
                "  Error: no such interface" );
		return KErrNotFound;
		}
	return Kern::ThreadDesWrite(aThread, &aBuffer, iDescriptors[i]->DescriptorData(), 0);
	}


TInt TUsbcDescriptorPool::SetInterfaceDescriptor(const TDes8& aBuffer, TInt aInterface, TInt aSetting)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_SETINTERFACEDESCRIPTOR, 
	        "TUsbcDescriptorPool::SetInterfaceDescriptor()" );
	const TInt i = FindIfcDescriptor(aInterface, aSetting);
	if (i < 0)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETINTERFACEDESCRIPTOR_DUP1,
                "  Error: no such interface" );
		return KErrNotFound;
		}
	iDescriptors[i]->SetByte(2, aBuffer[2]);				// bInterfaceNumber
	iDescriptors[i]->SetByte(5, aBuffer[5]);				// bInterfaceClass
	iDescriptors[i]->SetByte(6, aBuffer[6]);				// bInterfaceSubClass
	iDescriptors[i]->SetByte(7, aBuffer[7]);				// bInterfaceProtocol
	return KErrNone;
	}


TInt TUsbcDescriptorPool::GetEndpointDescriptorTC(DThread* aThread, TDes8& aBuffer,
												  TInt aInterface, TInt aSetting, TUint8 aEndpointAddress) const
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_GETENDPOINTDESCRIPTORTC,
	        "TUsbcDescriptorPool::GetEndpointDescriptorTC()" );
	const TInt i = FindEpDescriptor(aInterface, aSetting, aEndpointAddress);
	if (i < 0)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_GETENDPOINTDESCRIPTORTC_DUP1,
                "  Error: no such endpoint" );
		return KErrNotFound;
		}
	return Kern::ThreadDesWrite(aThread, &aBuffer, iDescriptors[i]->DescriptorData(), 0);
	}


TInt TUsbcDescriptorPool::SetEndpointDescriptorTC(DThread* aThread, const TDes8& aBuffer,
												  TInt aInterface, TInt aSetting, TUint8 aEndpointAddress)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_SETENDPOINTDESCRIPTORTC, 
	        "TUsbcDescriptorPool::SetEndpointDescriptorTC()" );
	const TInt i = FindEpDescriptor(aInterface, aSetting, aEndpointAddress);
	if (i < 0)
		{
	    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETENDPOINTDESCRIPTORTC_DUP1, 
	            "  Error: no such endpoint" );
		return KErrNotFound;
		}
	TBuf8<KUsbDescSize_AudioEndpoint> ep;					// it could be an audio endpoint
	const TInt r = Kern::ThreadDesRead(aThread, &aBuffer, ep, 0);
	if (r != KErrNone)
		{
		return r;
		}
	iDescriptors[i]->SetByte(3, ep[3]);						// bmAttributes
	iDescriptors[i]->SetByte(6, ep[6]);						// bInterval
	if (iDescriptors[i]->Size() == KUsbDescSize_AudioEndpoint)
		{
		iDescriptors[i]->SetByte(7, ep[7]);					// bRefresh
		iDescriptors[i]->SetByte(8, ep[8]);					// bSynchAddress
		}
	return KErrNone;
	}


TInt TUsbcDescriptorPool::GetEndpointDescriptorSize(TInt aInterface, TInt aSetting, TUint8 aEndpointAddress,
													TInt& aSize) const
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_GETENDPOINTDESCRIPTORSIZE,
	        "TUsbcDescriptorPool::GetEndpointDescriptorSize()" );
	const TInt i = FindEpDescriptor(aInterface, aSetting, aEndpointAddress);
	if (i < 0)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_GETENDPOINTDESCRIPTORSIZE_DUP1,
                "  Error: no such endpoint" );
		return KErrNotFound;
		}
	aSize = iDescriptors[i]->Size();
	return KErrNone;
	}


TInt TUsbcDescriptorPool::GetDeviceQualifierDescriptorTC(DThread* aThread, TDes8& aBuffer) const
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_GETDEVICEQUALIFIERDESCRIPTORTC, 
	        "TUsbcDescriptorPool::GetDeviceQualifierDescriptorTC()" );
	if (iDescriptors[KDescPosition_DeviceQualifier] == NULL)
		{
	    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_GETDEVICEQUALIFIERDESCRIPTORTC_DUP1, 
	            "  Warning: Device_Qualifier descriptor not supported" );
		return KErrNotSupported;
		}
	return Kern::ThreadDesWrite(aThread, &aBuffer,
								iDescriptors[KDescPosition_DeviceQualifier]->DescriptorData(), 0);
	}


TInt TUsbcDescriptorPool::SetDeviceQualifierDescriptorTC(DThread* aThread, const TDes8& aBuffer)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_SETDEVICEQUALIFIERDESCRIPTORTC,
	        "TUsbcDescriptorPool::SetDeviceQualifierDescriptorTC()" );
	if (iDescriptors[KDescPosition_DeviceQualifier] == NULL)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETDEVICEQUALIFIERDESCRIPTORTC_DUP1,
                "  Warning: Device_Qualifier descriptor not supported" );
		return KErrNotSupported;
		}
	TBuf8<KUsbDescSize_DeviceQualifier> device;
	const TInt r = Kern::ThreadDesRead(aThread, &aBuffer, device, 0);
	if (r != KErrNone)
		{
		return r;
		}
	iDescriptors[KDescPosition_DeviceQualifier]->SetByte(2, device[2]); // bcdUSB
	iDescriptors[KDescPosition_DeviceQualifier]->SetByte(3, device[3]); // bcdUSB (part II)
	iDescriptors[KDescPosition_DeviceQualifier]->SetByte(4, device[4]); // bDeviceClass
	iDescriptors[KDescPosition_DeviceQualifier]->SetByte(5, device[5]); // bDeviceSubClass
	iDescriptors[KDescPosition_DeviceQualifier]->SetByte(6, device[6]); // bDeviceProtocol
	return KErrNone;
	}


TInt TUsbcDescriptorPool::GetOtherSpeedConfigurationDescriptorTC(DThread* aThread, TDes8& aBuffer) const
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_GETOTHERSPEEDCONFIGURATIONDESCRIPTORTC, 
	        "TUsbcDescriptorPool::GetOtherSpeedConfigurationDescriptorTC()" );
	if (iDescriptors[KDescPosition_OtherSpeedConfig] == NULL)
		{
	    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_GETOTHERSPEEDCONFIGURATIONDESCRIPTORTC_DUP1, 
	            "  Warning: Other_Speed_Configuration descriptor not supported" );
		return KErrNotSupported;
		}
	return Kern::ThreadDesWrite(aThread, &aBuffer,
								iDescriptors[KDescPosition_OtherSpeedConfig]->DescriptorData(), 0);
	}


TInt TUsbcDescriptorPool::SetOtherSpeedConfigurationDescriptorTC(DThread* aThread, const TDes8& aBuffer)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_SETOTHERSPEEDCONFIGURATIONDESCRIPTORTC, 
	        "TUsbcDescriptorPool::SetOtherSpeedConfigurationDescriptorTC()" );
	if (iDescriptors[KDescPosition_OtherSpeedConfig] == NULL)
		{
	    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETOTHERSPEEDCONFIGURATIONDESCRIPTORTC_DUP1, 
	            "  Warning: Other_Speed_Configuration descriptor not supported" );
		return KErrNotSupported;
		}
	TBuf8<KUsbDescSize_OtherSpeedConfig> config;
	const TInt r = Kern::ThreadDesRead(aThread, &aBuffer, config, 0);
	if (r != KErrNone)
		{
		return r;
		}
	iDescriptors[KDescPosition_OtherSpeedConfig]->SetByte(7, config[7]); // bmAttributes
	iDescriptors[KDescPosition_OtherSpeedConfig]->SetByte(8, config[8]); // bMaxPower
	return KErrNone;
	}


TInt TUsbcDescriptorPool::GetCSInterfaceDescriptorTC(DThread* aThread, TDes8& aBuffer,
													 TInt aInterface, TInt aSetting) const
	{
	// first find the interface
	TInt i = FindIfcDescriptor(aInterface, aSetting);
	if (i < 0)
		{
		OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_GETCSINTERFACEDESCRIPTORTC,
		        "  Error: no such interface" );
		return KErrNotFound;
		}
	TInt r = KErrNotFound;
	TInt offset = 0;
	const TInt count = iDescriptors.Count();
	while (++i < count && iDescriptors[i]->Type() == KUsbDescType_CS_Interface)
		{
		r = Kern::ThreadDesWrite(aThread, &aBuffer,
								 iDescriptors[i]->DescriptorData(), offset);
		if (r != KErrNone)
			break;
		offset += iDescriptors[i]->Size();
		}
	return r;
	}


TInt TUsbcDescriptorPool::SetCSInterfaceDescriptorTC(DThread* aThread, const TDes8& aBuffer,
													 TInt aInterface, TInt aSetting, TInt aSize)
	{
	// First find the interface
	TInt i = FindIfcDescriptor(aInterface, aSetting);
	if (i < 0)
		{
		OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETCSINTERFACEDESCRIPTORTC, 
		        "  Error: no such interface" );
		return KErrNotFound;
		}
	// Find a position where to insert the new class specific interface descriptor(s)
	const TInt count = iDescriptors.Count();
	while (++i < count && iDescriptors[i]->Type() == KUsbDescType_CS_Interface)
		;
	// Create a new cs descriptor
	TUsbcClassSpecificDescriptor* desc = TUsbcClassSpecificDescriptor::New(KUsbDescType_CS_Interface, aSize);
	if (!desc)
		{
		return KErrNoMemory;
		}
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_SETCSINTERFACEDESCRIPTORTC_DUP1,
            "  inserting descriptor at position %d", i );
	iDescriptors.Insert(desc, i);

	// Update the config descriptor's wTotalLength field
	UpdateConfigDescriptorLength(aSize);

	// Copy contents from the user side
	return Kern::ThreadDesRead(aThread, &aBuffer, iDescriptors[i]->DescriptorData(), 0);
	}


TInt TUsbcDescriptorPool::GetCSInterfaceDescriptorSize(TInt aInterface, TInt aSetting, TInt& aSize) const
	{
	// first find the interface
	TInt i = FindIfcDescriptor(aInterface, aSetting);
	if (i < 0)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_GETCSINTERFACEDESCRIPTORSIZE, 
                "  Error: no such interface" );
		return KErrNotFound;
		}
	TInt r = KErrNotFound;
	TInt size = 0;
	const TInt count = iDescriptors.Count();
	while (++i < count && iDescriptors[i]->Type() == KUsbDescType_CS_Interface)
		{
		size += iDescriptors[i]->Size();
		r = KErrNone;
		}
	if (r == KErrNone)
		aSize = size;
	return r;
	}


TInt TUsbcDescriptorPool::GetCSEndpointDescriptorTC(DThread* aThread, TDes8& aBuffer, TInt aInterface,
													TInt aSetting, TUint8 aEndpointAddress) const
	{
	// first find the endpoint
	TInt i = FindEpDescriptor(aInterface, aSetting, aEndpointAddress);
	if (i < 0)
		{
		OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_GETCSENDPOINTDESCRIPTORTC,
		        "  Error: no such endpoint" );
		return KErrNotFound;
		}
	TInt r = KErrNotFound;
	TInt offset = 0;
	const TInt count = iDescriptors.Count();
	while (++i < count && iDescriptors[i]->Type() == KUsbDescType_CS_Endpoint)
		{
		r = Kern::ThreadDesWrite(aThread, &aBuffer,
								 iDescriptors[i]->DescriptorData(), offset);
		if (r != KErrNone)
			break;
		offset += iDescriptors[i]->Size();
		}
	return r;
	}


TInt TUsbcDescriptorPool::SetCSEndpointDescriptorTC(DThread* aThread, const TDes8& aBuffer, TInt aInterface,
													TInt aSetting, TUint8 aEndpointAddress, TInt aSize)
	{
	// first find the endpoint
	TInt i = FindEpDescriptor(aInterface, aSetting, aEndpointAddress);
	if (i < 0)
		{
		OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETCSENDPOINTDESCRIPTORTC,
		        "  Error: no such endpoint" );
		return KErrNotFound;
		}
	// find a position where to insert the new class specific endpoint descriptor(s)
	const TInt count = iDescriptors.Count();
	while (++i < count && iDescriptors[i]->Type() == KUsbDescType_CS_Endpoint)
		;
	// create a new cs descriptor
	TUsbcClassSpecificDescriptor* desc = TUsbcClassSpecificDescriptor::New(KUsbDescType_CS_Endpoint, aSize);
	if (!desc)
		{
		return KErrNoMemory;
		}
	iDescriptors.Insert(desc, i);
	// update the config descriptor's wTotalLength field
	UpdateConfigDescriptorLength(aSize);
	// copy contents from user side
	return Kern::ThreadDesRead(aThread, &aBuffer, iDescriptors[i]->DescriptorData(), 0);
	}


TInt TUsbcDescriptorPool::GetCSEndpointDescriptorSize(TInt aInterface, TInt aSetting,
													  TUint8 aEndpointAddress, TInt& aSize) const
	{
	// first find the endpoint
	TInt i = FindEpDescriptor(aInterface, aSetting, aEndpointAddress);
	if (i < 0)
		{
		OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_GETCSENDPOINTDESCRIPTORSIZE, 
		        "  Error: no such endpoint" );
		return KErrNotFound;
		}
	TInt r = KErrNotFound;
	TInt size = 0;
	const TInt count = iDescriptors.Count();
	while (++i < count && iDescriptors[i]->Type() == KUsbDescType_CS_Endpoint)
		{
		size += iDescriptors[i]->Size();
		r = KErrNone;
		}
	if (r == KErrNone)
		aSize = size;
	return r;
	}


TInt TUsbcDescriptorPool::GetStringDescriptorLangIdTC(DThread* aThread, TDes8& aLangId) const
	{
	const TUint16 id = iStrings[KStringPosition_Langid]->Word(2);
	const TPtrC8 id_des(reinterpret_cast<const TUint8*>(&id), sizeof(id));
	return Kern::ThreadDesWrite(aThread, &aLangId, id_des, 0);
	}


TInt TUsbcDescriptorPool::SetStringDescriptorLangId(TUint16 aLangId)
	{
	iStrings[KStringPosition_Langid]->SetWord(2, aLangId);
	return KErrNone;
	}


TInt TUsbcDescriptorPool::GetManufacturerStringDescriptorTC(DThread* aThread, TDes8& aString) const
	{
	return GetDeviceStringDescriptorTC(aThread, aString, KUsbDescStringIndex_Manufact,
									   KStringPosition_Manufact);
	}


TInt TUsbcDescriptorPool::SetManufacturerStringDescriptorTC(DThread* aThread, const TDes8& aString)
	{
	return SetDeviceStringDescriptorTC(aThread, aString, KUsbDescStringIndex_Manufact,
									   KStringPosition_Manufact);
	}


TInt TUsbcDescriptorPool::RemoveManufacturerStringDescriptor()
	{
	return RemoveDeviceStringDescriptor(KUsbDescStringIndex_Manufact, KStringPosition_Manufact);
	}


TInt TUsbcDescriptorPool::GetProductStringDescriptorTC(DThread* aThread, TDes8& aString) const
	{
	return GetDeviceStringDescriptorTC(aThread, aString, KUsbDescStringIndex_Product,
									   KStringPosition_Product);
	}


TInt TUsbcDescriptorPool::SetProductStringDescriptorTC(DThread* aThread, const TDes8& aString)
	{
	return SetDeviceStringDescriptorTC(aThread, aString, KUsbDescStringIndex_Product,
									   KStringPosition_Product);
	}


TInt TUsbcDescriptorPool::RemoveProductStringDescriptor()
	{
	return RemoveDeviceStringDescriptor(KUsbDescStringIndex_Product, KStringPosition_Product);
	}


TInt TUsbcDescriptorPool::GetSerialNumberStringDescriptorTC(DThread* aThread, TDes8& aString) const
	{
	return GetDeviceStringDescriptorTC(aThread, aString, KUsbDescStringIndex_Serial,
									   KStringPosition_Serial);
	}


TInt TUsbcDescriptorPool::SetSerialNumberStringDescriptorTC(DThread* aThread, const TDes8& aString)
	{
	return SetDeviceStringDescriptorTC(aThread, aString, KUsbDescStringIndex_Serial,
									   KStringPosition_Serial);
	}


TInt TUsbcDescriptorPool::RemoveSerialNumberStringDescriptor()
	{
	return RemoveDeviceStringDescriptor(KUsbDescStringIndex_Serial, KStringPosition_Serial);
	}


TInt TUsbcDescriptorPool::GetConfigurationStringDescriptorTC(DThread* aThread, TDes8& aString) const
	{
	const TInt str_idx = iDescriptors[KDescPosition_Config]->Byte(KUsbDescStringIndex_Config);
	if (str_idx)
		{
		__ASSERT_ALWAYS((str_idx == KStringPosition_Config), Kern::Fault(KUsbPanicCat, __LINE__));
		OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_GETCONFIGURATIONSTRINGDESCRIPTORTC, 
		        "  String @ pos %d (conf $): \"%s\"", str_idx, iStrings[str_idx]->StringData() );
		return Kern::ThreadDesWrite(aThread, &aString,
									iStrings[str_idx]->StringData(), 0);
		}
	else
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_GETCONFIGURATIONSTRINGDESCRIPTORTC_DUP1,
                "  No config string descriptor @ pos %d", str_idx );

		return KErrNotFound;
		}
	}


TInt TUsbcDescriptorPool::SetConfigurationStringDescriptorTC(DThread* aThread, const TDes8& aString)
	{
	// we don't know the length of the string, so we have to allocate memory dynamically
	TUint strlen = Kern::ThreadGetDesLength(aThread, &aString);
	if (strlen > KUsbStringDescStringMaxSize)
		{
		OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETCONFIGURATIONSTRINGDESCRIPTORTC,
		        "  Warning: config $ descriptor too long - will be truncated" );
		strlen = KUsbStringDescStringMaxSize;
		}
	HBuf8* const strbuf = HBuf8::New(strlen);
	if (!strbuf)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETCONFIGURATIONSTRINGDESCRIPTORTC_DUP1,
                "  Error: Memory allocation for config $ desc string failed (1)" );
		return KErrNoMemory;
		}
	strbuf->SetMax();
	// the aString points to data that lives in user memory, so we have to copy it:
	const TInt r = Kern::ThreadDesRead(aThread, &aString, *strbuf, 0);
	if (r != KErrNone)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETCONFIGURATIONSTRINGDESCRIPTORTC_DUP2,
                "  Error: Thread read error" );
		delete strbuf;
		return r;
		}
	TUsbcStringDescriptor* sd = TUsbcStringDescriptor::New(*strbuf);
	if (!sd)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETCONFIGURATIONSTRINGDESCRIPTORTC_DUP3,
                "  Error: Memory allocation for config $ desc failed (2)" );
		delete strbuf;
		return KErrNoMemory;
		}
	// Delete old string, put in new one
	ExchangeStringDescriptor(KStringPosition_Config, sd);
	// Update Config descriptor string index field
	iDescriptors[KDescPosition_Config]->SetByte(KUsbDescStringIndex_Config, KStringPosition_Config);
	// Update Other_Speed_Config descriptor string index field as well, if applicable
	if (iDescriptors[KDescPosition_OtherSpeedConfig])
		iDescriptors[KDescPosition_OtherSpeedConfig]->SetByte(KUsbDescStringIndex_Config,
															  KStringPosition_Config);
	delete strbuf;
	return KErrNone;
	}


TInt TUsbcDescriptorPool::RemoveConfigurationStringDescriptor()
	{
	if (iDescriptors[KDescPosition_Config]->Byte(KUsbDescStringIndex_Config) == 0)
		{
		OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_REMOVECONFIGURATIONSTRINGDESCRIPTOR,
		        "  RemoveConfigurationStringDescriptor: no $ desc @ index %d", KUsbDescStringIndex_Config );
		return KErrNotFound;
		}
	// Delete old string, put in NULL pointer
	ExchangeStringDescriptor(KStringPosition_Config, NULL);
	// Update Config descriptor string index field
	iDescriptors[KDescPosition_Config]->SetByte(KUsbDescStringIndex_Config, 0);
	// Update Other_Speed_Config descriptor string index field as well, if applicable
	if (iDescriptors[KDescPosition_OtherSpeedConfig])
		iDescriptors[KDescPosition_OtherSpeedConfig]->SetByte(KUsbDescStringIndex_Config, 0);
	return KErrNone;
	}


TInt TUsbcDescriptorPool::GetStringDescriptorTC(DThread* aThread, TInt aIndex, TDes8& aString) const
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_GETSTRINGDESCRIPTORTC,
	        "TUsbcDescriptorPool::GetStringDescriptorTC()" );
	if (!StringDescriptorExists(aIndex))
		{
		return KErrNotFound;
		}
	OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_GETSTRINGDESCRIPTORTC_DUP1,
	        "  String @ pos %d: \"%s\"", aIndex, iStrings[aIndex]->StringData() );
	return Kern::ThreadDesWrite(aThread, &aString, iStrings[aIndex]->StringData(), 0);
	}


TInt TUsbcDescriptorPool::SetStringDescriptorTC(DThread* aThread, TInt aIndex, const TDes8& aString)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_SETSTRINGDESCRIPTORTC,
	        "TUsbcDescriptorPool::SetStringDescriptorTC()" );
	// we don't know the length of the string, so we have to allocate memory dynamically
	TUint strlen = Kern::ThreadGetDesLength(aThread, &aString);
	if (strlen > KUsbStringDescStringMaxSize)
		{
	   OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETSTRINGDESCRIPTORTC_DUP1,
	           "  Warning: $ descriptor too long - will be truncated" );
		strlen = KUsbStringDescStringMaxSize;
		}
	HBuf8* strbuf = HBuf8::New(strlen);
	if (!strbuf)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETSTRINGDESCRIPTORTC_DUP2,
                "  Error: Mem alloc for $ desc string failed (1)" );
		return KErrNoMemory;
		}
	strbuf->SetMax();
	// the aString points to data that lives in user memory, so we have to copy it over:
	const TInt r = Kern::ThreadDesRead(aThread, &aString, *strbuf, 0);
	if (r != KErrNone)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETSTRINGDESCRIPTORTC_DUP3,
                "  Error: Thread read error" );
		delete strbuf;
		return r;
		}
	TUsbcStringDescriptor* const sd = TUsbcStringDescriptor::New(*strbuf);
	if (!sd)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETSTRINGDESCRIPTORTC_DUP4,
                "  Error: Mem alloc for $ desc failed (2)" );
		delete strbuf;
		return KErrNoMemory;
		}
	if (aIndex < iStrings.Count())
		{
		ExchangeStringDescriptor(aIndex, sd);
		}
	else // if (aIndex >= iStrings.Count())
		{
		while (aIndex > iStrings.Count())
			{
			iStrings.Append(NULL);
			}
		iStrings.Append(sd);
		}
	delete strbuf;
	return KErrNone;
	}


TInt TUsbcDescriptorPool::RemoveStringDescriptor(TInt aIndex)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_REMOVESTRINGDESCRIPTOR, 
	        "TUsbcDescriptorPool::RemoveStringDescriptor()" );
	if (!StringDescriptorExists(aIndex))
		{
		return KErrNotFound;
		}
   OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_REMOVESTRINGDESCRIPTOR_DUP1, 
           "  Removing string @ pos %d: \"%s\"", aIndex, iStrings[aIndex]->StringData() );
	ExchangeStringDescriptor(aIndex, NULL);

	// Make sure there's no $ after aIndex.
	const TInt n = iStrings.Count();
	for (TInt i = aIndex; i < n; i++)
		{
		if (iStrings[i] != NULL)
			{
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_REMOVESTRINGDESCRIPTOR_DUP2, 
                    "  Found $ @ idx %d - not compressing", i );
			return KErrNone;
			}
		}

    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_REMOVESTRINGDESCRIPTOR_DUP3,
            "  No $ found after idx %d - compressing array", aIndex );
	// Move aIndex back just before the first !NULL element.
	while (iStrings[--aIndex] == NULL)
		;
	// Let aIndex point to first NULL.
	aIndex++;
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_REMOVESTRINGDESCRIPTOR_DUP4,
            "  Starting at index %d", aIndex );
	// Now remove NULL pointers until (Count() == aIndex).
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_REMOVESTRINGDESCRIPTOR_DUP5,
            "  iStrings.Count() before: %d", iStrings.Count() );
	do
		{
		iStrings.Remove(aIndex);
	    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_REMOVESTRINGDESCRIPTOR_DUP6,
	            "  Removing $" );
		}
	while (iStrings.Count() > aIndex);
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_REMOVESTRINGDESCRIPTOR_DUP7,
            "  iStrings.Count() after: %d", iStrings.Count() );
	// Regain some memory.
	iStrings.Compress();

	return KErrNone;
	}


// ===================================================================
// --- private ---
// ===================================================================

//
// Insert an Interface descriptor into the descriptor array at the appropriate index.
//
void TUsbcDescriptorPool::InsertIfcDesc(TUsbcDescriptorBase* aDesc)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_INSERTIFCDESC, 
	        "TUsbcDescriptorPool::InsertIfcDesc()" );

	const TInt count = iDescriptors.Count();
	TBool ifc_exists = EFalse;								// set to 'true' if we're adding an alternate
															// setting to an already existing interface
	TInt i = KDescPosition_FirstAvailable;
	while (i < count)
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_INSERTIFCDESC_DUP1, 
                "  already descriptors there (%d)...", count );
		if (iDescriptors[i]->Type() == KUsbDescType_Interface)
			{
			if (iDescriptors[i]->Byte(2) > aDesc->Byte(2))
				{
				// our interface number is less than the one's just found => insert before it (= here)
				break;
				}
			else if (iDescriptors[i]->Byte(2) == aDesc->Byte(2))
				{
				ifc_exists = ETrue;
				// same interface number => look at settings number
				if (iDescriptors[i]->Byte(3) > aDesc->Byte(3))
					{
					// our setting number is less than the one's found => insert before (= here)
					break;
					}
				else if (iDescriptors[i]->Byte(3) == aDesc->Byte(3))
					{
                    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_INSERTIFCDESC_DUP2, 
                            "  Error: first delete old desc (TUsbcDescriptorPool::InsertIfcDesc)" );
					return;
					}
				}
			}
		++i;
		}
	// In any case: put the new descriptor at position i.
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_INSERTIFCDESC_DUP3,
            "  inserting descriptor at position %d", i );
	iDescriptors.Insert(aDesc, i);

	// Update the config descriptor's wTotalLength field.
	UpdateConfigDescriptorLength(KUsbDescSize_Interface);

	if (!ifc_exists)
		{
		// If this is the first setting for the interface, increment bNumInterfaces.
		UpdateConfigDescriptorNumIfcs(1);
		}

	iIfcIdx = i;
	}


//
// Insert an Endpoint descriptor into the descriptor array at the appropriate index.
//
void TUsbcDescriptorPool::InsertEpDesc(TUsbcDescriptorBase* aDesc)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_INSERTEPDESC, 
	        "TUsbcDescriptorPool::InsertEpDesc()" );
	if (iIfcIdx == 0)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_INSERTEPDESC_DUP1, 
                "  Error: only after interface (TUsbcDescriptorPool::InsertEpDesc)" );
		return;
		}
	const TInt count = iDescriptors.Count();
	TInt i = iIfcIdx + 1;
	while (i < count)
		{
		if (iDescriptors[i]->Type() != KUsbDescType_Endpoint)
			break;
		++i;
		}
	// put the new descriptor at position i
	iDescriptors.Insert(aDesc, i);
	// update the config descriptor's wTotalLength field
	UpdateConfigDescriptorLength(aDesc->Size());
	}


//
// Find the index of the Interface descriptor for a given interface setting.
//
TInt TUsbcDescriptorPool::FindIfcDescriptor(TInt aIfcNumber, TInt aIfcSetting) const
	{
	OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_FINDIFCDESCRIPTOR,
	        "TUsbcDescriptorPool::FindIfcDescriptor(%d, %d)", aIfcNumber, aIfcSetting );
	const TInt count = iDescriptors.Count();
	for (TInt i = KDescPosition_FirstAvailable; i < count; i++)
		{
		if ((iDescriptors[i]->Type() == KUsbDescType_Interface) &&
			(iDescriptors[i]->Byte(2) == aIfcNumber) &&
			(iDescriptors[i]->Byte(3) == aIfcSetting))
			{
			return i;
			}
		}
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_FINDIFCDESCRIPTOR_DUP1,
	        "  Error: no such interface" );
	return -1;
	}


//
// Find the index of the Endpoint descriptor for a given endpoint on a given interface setting.
//
TInt TUsbcDescriptorPool::FindEpDescriptor(TInt aIfcNumber, TInt aIfcSetting, TUint8 aEpAddress) const
	{
	OstTraceDefExt3( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_FINDEPDESCRIPTOR,
	        "TUsbcDescriptorPool::FindEpDescriptor(%d, %d, 0x%02x)", aIfcNumber, aIfcSetting, aEpAddress );
	// first find the interface
	const TInt ifc = FindIfcDescriptor(aIfcNumber, aIfcSetting);
	if (ifc < 0)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_FINDEPDESCRIPTOR_DUP1,
                "  Error: no such interface" );
		return ifc;
		}
	const TInt count = iDescriptors.Count();
	// then, before the next interface, try to locate the endpoint
	for (TInt i = ifc + 1; i < count; i++)
		{
		if (iDescriptors[i]->Type() == KUsbDescType_Interface)
			{
            OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_FINDEPDESCRIPTOR_DUP2,
                    "  Error: no such endpoint before next interface" );
			return -1;
			}
		else if ((iDescriptors[i]->Type() == KUsbDescType_Endpoint) &&
				 (iDescriptors[i]->Byte(2) == aEpAddress))
			{
			// found
			return i;
			}
		}
    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_FINDEPDESCRIPTOR_DUP3,
            "  Error: no such endpoint" );
	return -1;
	}


//
// Delete n descriptors starting from aIndex and remove their pointers from the array.
//
void TUsbcDescriptorPool::DeleteDescriptors(TInt aIndex, TInt aCount)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_DELETEDESCRIPTORS, 
	        "TUsbcDescriptorPool::DeleteDescriptors()" );
	if (aIndex < KDescPosition_FirstAvailable)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_DELETEDESCRIPTORS_DUP1, 
                "  Error: aIndex < KDescPosition_FirstAvailable" );
		return;
		}
	if (aCount <= 0)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_DELETEDESCRIPTORS_DUP2, 
                "  Error: aCount <= 0" );
		return;
		}
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_DELETEDESCRIPTORS_DUP3, 
            "  Removing descriptors at index %d:", aIndex );
	// Try to update wTotalLength field in Config descriptor
	while (aCount--)
		{
		// In this loop we don't decrement aIndex, because after deleting an element
		// aIndex is already indexing the next one.
		TUsbcDescriptorBase* const ptr = iDescriptors[aIndex];
		switch (ptr->Type())
			{
		case KUsbDescType_Interface:
		    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_DELETEDESCRIPTORS_DUP4,
		            "  - an interface descriptor" );
			UpdateConfigDescriptorLength(-KUsbDescSize_Interface);
			break;
		case KUsbDescType_Endpoint:
			OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_DELETEDESCRIPTORS_DUP5,
			        "  - an endpoint descriptor" );
			UpdateConfigDescriptorLength(-ptr->Size());
			break;
		case KUsbDescType_CS_Interface:
			/* fall through */
		case KUsbDescType_CS_Endpoint:
		    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_DELETEDESCRIPTORS_DUP6,
		            "  - a class specific descriptor" );
			UpdateConfigDescriptorLength(-ptr->Size());
			break;
		default:
		    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_DELETEDESCRIPTORS_DUP7,
		            "  - an unknown descriptor" );
		    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_DELETEDESCRIPTORS_DUP8,
		            "  Error: unknown descriptor type" );
			}
		iDescriptors.Remove(aIndex);
		delete ptr;
		}
	}


//
// Update the wTotalLength field in the Configuration descriptor (aLength can be negative).
//
void TUsbcDescriptorPool::UpdateConfigDescriptorLength(TInt aLength)
	{
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_UPDATECONFIGDESCRIPTORLENGTH,
	        "TUsbcDescriptorPool::UpdateConfigDescriptorLength(%d)", aLength );
	TUsbcDescriptorBase* const cnf = iDescriptors[KDescPosition_Config];
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_UPDATECONFIGDESCRIPTORLENGTH_DUP1,
	        "  wTotalLength old: %d", cnf->Word(2) );
	// Update Config descriptor
	cnf->SetWord(2, cnf->Word(2) + aLength);
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_UPDATECONFIGDESCRIPTORLENGTH_DUP2,
	        "  wTotalLength new: %d", cnf->Word(2) );
	// Update Other_Speed_Config descriptor as well, if applicable
	if (iDescriptors[KDescPosition_OtherSpeedConfig])
		iDescriptors[KDescPosition_OtherSpeedConfig]->SetWord(2, cnf->Word(2));
	}


//
// Update the bNumInterfaces field in the Configuration descriptor (aNumber can be negative).
//
void TUsbcDescriptorPool::UpdateConfigDescriptorNumIfcs(TInt aNumber)
	{
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_UPDATECONFIGDESCRIPTORNUMIFCS,
	        "TUsbcDescriptorPool::UpdateConfigDescriptorNumIfcs(%d)", aNumber );
	TUsbcDescriptorBase* const cnf = iDescriptors[KDescPosition_Config];
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_UPDATECONFIGDESCRIPTORNUMIFCS_DUP1,
	        "  bNumInterfaces old: %d", cnf->Byte(4) );
	const TInt n = cnf->Byte(4) + aNumber;
	if (n < 0)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_UPDATECONFIGDESCRIPTORNUMIFCS_DUP2,
                "  Error: bNumInterfaces + aNumber < 0" );
		return;
		}
	// Update Config descriptor
	cnf->SetByte(4, n);
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_UPDATECONFIGDESCRIPTORNUMIFCS_DUP3,
            "  bNumInterfaces new: %d", cnf->Byte(4) );
	// Update Other_Speed_Config descriptor as well, if applicable
	if (iDescriptors[KDescPosition_OtherSpeedConfig])
		iDescriptors[KDescPosition_OtherSpeedConfig]->SetByte(4, n);
	}


//
// Update the bNumInterfaces field in the Configuration descriptor if necessary.
//
void TUsbcDescriptorPool::UpdateIfcNumbers(TInt aNumber)
	{
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_UPDATEIFCNUMBERS,
	        "TUsbcDescriptorPool::UpdateIfcNumbers(%d)", aNumber );
	const TInt count = iDescriptors.Count();
	for (TInt i = KDescPosition_FirstAvailable; i < count; i++)
		{
		if ((iDescriptors[i]->Type() == KUsbDescType_Interface) &&
			(iDescriptors[i]->Byte(2) == aNumber))
			{
			// there's still an interface with 'number' so we don't need to update anything
			return;
			}
		}
	// if we haven't returned yet, we decrement bNumInterfaces
	UpdateConfigDescriptorNumIfcs(-1);
	}


//
// Put the current Device or Device_Qualifier descriptor in the Ep0 Tx buffer.
// Only used for Ep0 standard requests, so target buffer can be hard-wired.
//
TInt TUsbcDescriptorPool::GetDeviceDescriptor(TInt aIndex) const
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_GETDEVICEDESCRIPTOR, 
	        "TUsbcDescriptorPool::GetDeviceDescriptor()" );
#ifdef _DEBUG
	if (!((aIndex == KDescPosition_Device) || (aIndex == KDescPosition_DeviceQualifier)))
	    {
        OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_GETDEVICEDESCRIPTOR_DUP1, 
                "  Error: invalid descriptor index: %d", aIndex );
	    }
#endif
	if (iDescriptors[aIndex] == NULL)
		{
		// This doesn't have to be an error - we might get asked here for the Device_Qualifier descriptor
		// on a FS-only device.
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_GETDEVICEDESCRIPTOR_DUP2, 
                "  Descriptor #%d requested but not available", aIndex );
		return 0;
		}
	return iDescriptors[aIndex]->GetDescriptorData(iEp0_TxBuf, KUsbcBufSz_Ep0Tx);
	}
	
	
//
// Put the current Configuration or Other_Speed_Configuration descriptor + all the following
// descriptors in the Ep0 Tx buffer.
// Only used for Ep0 standard requests, so target buffer can be hard-wired.
//
TInt TUsbcDescriptorPool::GetConfigurationDescriptor(TInt aIndex) const
	{
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_GETCONFIGURATIONDESCRIPTOR,
	        "TUsbcDescriptorPool::GetConfigDescriptor(%d)", aIndex );
#ifdef _DEBUG
	if (!((aIndex == KDescPosition_Config) || (aIndex == KDescPosition_OtherSpeedConfig)))
	    {
        OstTraceDef1( OST_TRACE_CATEGORY_DEBUG, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_GETCONFIGURATIONDESCRIPTOR_DUP1,
                "  Error: invalid descriptor index: %d", aIndex );
	    }
#endif
	if (iDescriptors[aIndex] == NULL)
		{
		// This is always an error: We should always have a Configuration descriptor and we should never
		// get asked for the Other_Speed_Configuration descriptor if we don't have one (9.6.2).
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_GETCONFIGURATIONDESCRIPTOR_DUP2,
                "  Warning: Descriptor %d requested but not available", aIndex );
		return 0;
		}
		
	const TInt count = iDescriptors.Count();	
	TInt copied = 0;
	TUint8* buf = iEp0_TxBuf;
	// pos == count is used so that otg descriptor is added to the end of returned descriptors
	for (TInt pos = aIndex; pos < count + 1; pos++)
		{
			
		if ((aIndex == KDescPosition_OtherSpeedConfig) && (pos == KDescPosition_Config))
			{
			// Skip Config descriptor when returning Other_Speed_Config
			continue;
			}
			
		if (pos == KDescPosition_Otg) 
			{
			// Skip otg descriptor just now. add it when pos is count, so that OTG are added at the end
			continue;
			}
						
		TUsbcDescriptorBase* ptr = NULL;	
		if (pos == count)
			{
				if (iDescriptors[KDescPosition_Otg] == NULL)
					{
					// Skip since there is no otg descriptor
                    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_GETCONFIGURATIONDESCRIPTOR_DUP3,
                            "  no otg descriptor" );
					continue;				
					}
				else
					{
					// add otg to the end of returned descriptors
					ptr = iDescriptors[KDescPosition_Otg];
					}
			}
		else
			{
			ptr = iDescriptors[pos];
			}

		// We need to edit endpoint descriptors on the fly because we have only one copy
		// of each and that copy has to contain different information, depending on the
		// current speed and the type of descriptor requested.
		if (ptr->Type() == KUsbDescType_Endpoint)
			{
			if ((iHighSpeed && (aIndex == KDescPosition_Config)) ||
				(!iHighSpeed && (aIndex == KDescPosition_OtherSpeedConfig)))
				{
				ptr->UpdateHs();
				}
			else
				{
				ptr->UpdateFs();
				}
			}

        OstTraceDefExt3( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_GETCONFIGURATIONDESCRIPTOR_DUP4,
                "  desc[%02d]: type = 0x%02x size = %d ", pos, ptr->Type(), ptr->Size() );			
		const TInt size = ptr->GetDescriptorData(buf, KUsbcBufSz_Ep0Tx - copied);
		if (size == 0)
			{
			
            OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_GETCONFIGURATIONDESCRIPTOR_DUP5,
                    "  Error: No Tx buffer space to copy this descriptor -> exiting" );
			break;
			}
		copied += size;
		if (copied >= KUsbcBufSz_Ep0Tx)
			{
            OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_GETCONFIGURATIONDESCRIPTOR_DUP6,
                    "  Error: No Tx buffer space left -> stopping here" );
			break;
			}
		buf += size;
		}
		
		
    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_GETCONFIGURATIONDESCRIPTOR_DUP7,
            "  copied %d bytes", copied );
	return copied;
	}


//
// Put the current OTG descriptor in the Ep0 Tx buffer.
// Only used for Ep0 standard requests, so target buffer can be hard-wired.
//
TInt TUsbcDescriptorPool::GetOtgDescriptor() const
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_GETOTGDESCRIPTOR,
	        "TUsbcDescriptorPool::GetOtgDescriptor()" );
	if (iDescriptors[KDescPosition_Otg] == NULL)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_GETOTGDESCRIPTOR_DUP1,
                "  OTG Descriptor not set" );
		return 0;
		}
	return iDescriptors[KDescPosition_Otg]->GetDescriptorData(iEp0_TxBuf, KUsbcBufSz_Ep0Tx);
	}


//
// Put a specific String descriptor in the Ep0 Tx buffer.
// Only used for Ep0 standard requests, so target buffer can be hard-wired.
//
TInt TUsbcDescriptorPool::GetStringDescriptor(TInt aIndex) const
	{
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_GETSTRINGDESCRIPTOR,
	        "TUsbcDescriptorPool::GetStringDescriptor(%d)", aIndex );
	// I really would have liked to display the descriptor contents here, but without trailing zero
	// we got a problem: how can we tell printf where the string ends? We would have to
	// dynamically allocate memory (since we don't know the size in advance), copy the descriptor
	// contents there, append a zero, and give this to printf. That's a bit too much effort...
	if (!StringDescriptorExists(aIndex))
		{
		return 0;
		}
	return iStrings[aIndex]->GetDescriptorData(iEp0_TxBuf, KUsbcBufSz_Ep0Tx);
	}


//
// Write a String descriptor pointed to by the Device descriptor to the user side
// (one of Manufacturer, Product, SerialNumber).
//
TInt TUsbcDescriptorPool::GetDeviceStringDescriptorTC(DThread* aThread, TDes8& aString,
													  TInt aIndex, TInt aPosition) const
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_GETDEVICESTRINGDESCRIPTORTC, 
	        "TUsbcDescriptorPool::GetDeviceStringDescriptorTC()" );
	const TInt str_idx = iDescriptors[KDescPosition_Device]->Byte(aIndex);
	if (str_idx)
		{
		__ASSERT_ALWAYS((str_idx == aPosition), Kern::Fault(KUsbPanicCat, __LINE__));
        OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_GETDEVICESTRINGDESCRIPTORTC_DUP1, 
                "  String @ pos %d (device $): \"%s\"", str_idx, iStrings[str_idx]->StringData() );
		return Kern::ThreadDesWrite(aThread, &aString,
									iStrings[str_idx]->StringData(), 0);
		}
	else
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_GETDEVICESTRINGDESCRIPTORTC_DUP2, 
                "  No string descriptor @ pos %d", aIndex );
		return KErrNotFound;
		}
	}


//
// Read a Device String descriptor from the user side and put in the descriptor arrays
// (one of Manufacturer, Product, SerialNumber).
//
TInt TUsbcDescriptorPool::SetDeviceStringDescriptorTC(DThread* aThread, const TDes8& aString,
													  TInt aIndex, TInt aPosition)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_SETDEVICESTRINGDESCRIPTORTC,
	        "TUsbcDescriptorPool::SetDeviceStringDescriptorTC()" );
	// we don't know the length of the string, so we have to allocate memory dynamically
	TUint strlen = Kern::ThreadGetDesLength(aThread, &aString);
	if (strlen > KUsbStringDescStringMaxSize)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETDEVICESTRINGDESCRIPTORTC_DUP1,
                "  Warning: $ descriptor too long - will be truncated" );
		strlen = KUsbStringDescStringMaxSize;
		}
	HBuf8* const strbuf = HBuf8::New(strlen);
	if (!strbuf)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETDEVICESTRINGDESCRIPTORTC_DUP2,
                "  Error: Memory allocation for dev $ desc string failed (1)" );
		return KErrNoMemory;
		}
	strbuf->SetMax();
	// the aString points to data that lives in user memory, so we have to copy it:
	const TInt r = Kern::ThreadDesRead(aThread, &aString, *strbuf, 0);
	if (r != KErrNone)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETDEVICESTRINGDESCRIPTORTC_DUP3,
                "  Error: Thread read error" );
		delete strbuf;
		return r;
		}
	TUsbcStringDescriptor* const sd = TUsbcStringDescriptor::New(*strbuf);
	if (!sd)
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_SETDEVICESTRINGDESCRIPTORTC_DUP4,
                "  Error: Memory allocation for dev $ desc failed (2)" );
		delete strbuf;
		return KErrNoMemory;
		}
	ExchangeStringDescriptor(aPosition, sd);
	iDescriptors[KDescPosition_Device]->SetByte(aIndex, aPosition);
	delete strbuf;
	return r;
	}


//
// Remove a Device String descriptor from the descriptor arrays
// (one of Manufacturer, Product, SerialNumber).
//
TInt TUsbcDescriptorPool::RemoveDeviceStringDescriptor(TInt aIndex, TInt aPosition)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_REMOVEDEVICESTRINGDESCRIPTOR, 
	        "TUsbcDescriptorPool::RemoveDeviceStringDescriptor()" );
	if (iDescriptors[KDescPosition_Device]->Byte(aIndex) == 0)
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_REMOVEDEVICESTRINGDESCRIPTOR_DUP1, 
                "  RemoveDeviceStringDescriptor: no $ desc @ index %d", aIndex );
		return KErrNotFound;
		}
	ExchangeStringDescriptor(aPosition, NULL);
	iDescriptors[KDescPosition_Device]->SetByte(aIndex, 0);
	return KErrNone;
	}


//
// Puts aDesc at postion aIndex in the string descriptor array, after deleting what was (possibly) there.
//
void TUsbcDescriptorPool::ExchangeStringDescriptor(TInt aIndex, const TUsbcStringDescriptor* aDesc)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_EXCHANGESTRINGDESCRIPTOR,
	        "TUsbcDescriptorPool::ExchangeStringDescriptor()" );
	TUsbcStringDescriptorBase* const ptr = iStrings[aIndex];
    OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_EXCHANGESTRINGDESCRIPTOR_DUP1,
            "  Deleting string descriptor at index %d: 0x%x", aIndex, reinterpret_cast<TUint>(ptr) );
	iStrings.Remove(aIndex);
	delete ptr;
    OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_EXCHANGESTRINGDESCRIPTOR_DUP2,
            "  Inserting string descriptor at index %d: 0x%x", aIndex, reinterpret_cast<TUint>(aDesc) );
	iStrings.Insert(aDesc, aIndex);
	}


//
// Checks whether there are any string descriptors in the array (apart from LangID).
//
TBool TUsbcDescriptorPool::AnyStringDescriptors() const
	{
	const TInt n = iStrings.Count();
	for (TInt i = 1; i < n; i++)
		{
		if (iStrings[i] != NULL)
			return ETrue;
		}
	return EFalse;
	}


//
// Returns true if aIndex exists and what is at that positition is not a NULL pointer.
//
TBool TUsbcDescriptorPool::StringDescriptorExists(TInt aIndex) const
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_STRINGDESCRIPTOREXISTS, 
	        "TUsbcDescriptorPool::StringDescriptorExists" );
	if (aIndex >= iStrings.Count())
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_STRINGDESCRIPTOREXISTS_DUP1, 
                "  Error: Bad string index: %d", aIndex );
		return EFalse;
		}
	else if (iStrings[aIndex] == NULL)
		{
        OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TUSBCDESCRIPTORPOOL_STRINGDESCRIPTOREXISTS_DUP2,
                "  Error: No $ descriptor @ pos %d", aIndex );
		return EFalse;
		}
	return ETrue;
	}


//
//
//
TInt TUsbcDescriptorPool::FindAvailableStringPos() const
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TUSBCDESCRIPTORPOOL_FINDAVAILABLESTRINGPOS,
	        "TUsbcDescriptorPool::FindAvailableStringPos()" );
	const TInt n = iStrings.Count();
	// We don't start from 0 because the first few locations are 'reserved'.
	for (TInt i = KStringPosition_FirstAvailable; i < n; i++)
		{
		if (iStrings[i] == NULL)
			{
            OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TUSBCDESCRIPTORPOOL_FINDAVAILABLESTRINGPOS_DUP1,
                    " Found available NULL position: %d", i );
			return i;
			}
		}
	return -1;
	}


// -eof-
