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
// f32test\testusbcldd\src\descriptors.cpp
// Platform independent USB client controller layer (PIL):
// USB descriptor handling and management.
// 
//

#include "usbcdesc.h"
#include "dtestusblogdev.h"

// --- TUsbcDescriptorBase

TUsbcDescriptorBase::TUsbcDescriptorBase()
	:
#ifdef USB_SUPPORTS_SET_DESCRIPTOR_REQUEST
	iIndex(0),
#endif
	iBufPtr(NULL, 0)
  	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcDescriptorBase::TUsbcDescriptorBase()")));
	}


TUsbcDescriptorBase::~TUsbcDescriptorBase()
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcDescriptorBase::~TUsbcDescriptorBase()")));
	}


void TUsbcDescriptorBase::SetByte(TUint aPosition, TUint8 aValue)
	{
	iBufPtr[aPosition] = aValue;
	}


void TUsbcDescriptorBase::SetWord(TUint aPosition, TUint16 aValue)
	{
	*reinterpret_cast<TUint16*>(&iBufPtr[aPosition]) = SWAP_BYTES_16(aValue);
	}


TUint8 TUsbcDescriptorBase::Byte(TUint aPosition) const
	{
	return iBufPtr[aPosition];
	}


TUint16 TUsbcDescriptorBase::Word(TUint aPosition) const
	{
	return SWAP_BYTES_16(*reinterpret_cast<const TUint16*>(&iBufPtr[aPosition]));
	}


void TUsbcDescriptorBase::GetDescriptorData(TDes8& aBuffer) const
	{
	aBuffer = iBufPtr;
	}


TInt TUsbcDescriptorBase::GetDescriptorData(TUint8* aBuffer) const
	{
	__MEMCPY(aBuffer, iBufPtr.Ptr(), Size());
	return Size();
	}


TInt TUsbcDescriptorBase::GetDescriptorData(TUint8* aBuffer, TInt aMaxSize) const
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


TInt TUsbcDescriptorBase::Size() const
	{
	return iBufPtr.Size();
	}


TUint8 TUsbcDescriptorBase::Type() const
	{
	return iBufPtr[1];
	}


void TUsbcDescriptorBase::SetBufferPointer(const TDesC8& aDes)
	{
	iBufPtr.Set(const_cast<TUint8*>(aDes.Ptr()), aDes.Size(), aDes.Size());
	}


// --- TUsbcDeviceDescriptor

TUsbcDeviceDescriptor::TUsbcDeviceDescriptor()
	: iBuf()
  	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcDeviceDescriptor::TUsbcDeviceDescriptor()")));
	}


TUsbcDeviceDescriptor* TUsbcDeviceDescriptor::New(TUint8 aDeviceClass, TUint8 aDeviceSubClass,
												  TUint8 aDeviceProtocol, TUint8 aMaxPacketSize0,
												  TUint16 aVendorId, TUint16 aProductId,
												  TUint16 aDeviceRelease, TUint8 aNumConfigurations)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcDeviceDescriptor::New()")));
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
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcDeviceDescriptor::Construct()")));
	iBuf.SetMax();
	SetBufferPointer(iBuf);
	iBuf[0] = (TUint8)iBuf.Size();									// bLength
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
	return KErrNone;
	}

// --- TUsbcConfigDescriptor

TUsbcConfigDescriptor::TUsbcConfigDescriptor()
	: iBuf()
  	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcConfigDescriptor::TUsbcConfigDescriptor()")));
	}


TUsbcConfigDescriptor* TUsbcConfigDescriptor::New(TUint8 aConfigurationValue, TBool aSelfPowered,
												  TBool aRemoteWakeup, TUint8 aMaxPower)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcConfigDescriptor::New()")));
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
									   TBool aRemoteWakeup, TUint8 aMaxPower)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcConfigDescriptor::Construct()")));
	iBuf.SetMax();
	SetBufferPointer(iBuf);
	iBuf[0] = (TUint8)iBuf.Size();									// bLength
	iBuf[1] = KUsbDescType_Config;							// bDescriptorType
	SetWord(2, KUsbDescSize_Config);						// wTotalLength
	iBuf[4] = 0;											// bNumInterfaces
	iBuf[5] = aConfigurationValue;							// bConfigurationValue
	iBuf[6] = 0;											// iConfiguration
	iBuf[7] = (TUint8)(0x80 | (aSelfPowered ? 0x40 : 0) | (aRemoteWakeup ? 0x20 : 0)); // bmAttributes (bit 7 always 1)
	iBuf[8] = (TUint8)(aMaxPower / 2);								// MaxPower (2mA units!)
	return KErrNone;
	}


// --- TUsbcInterfaceDescriptor

TUsbcInterfaceDescriptor::TUsbcInterfaceDescriptor()
	: iBuf()
  	{
 	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcInterfaceDescriptor::TUsbcInterfaceDescriptor()")));
	}


TUsbcInterfaceDescriptor* TUsbcInterfaceDescriptor::New(TUint8 aInterfaceNumber, TUint8 aAlternateSetting,
														TInt aNumEndpoints, const TUsbcClassInfo& aClassInfo)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcInterfaceDescriptor::New()")));
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
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcInterfaceDescriptor::Construct()")));
	iBuf.SetMax();
	SetBufferPointer(iBuf);
	iBuf[0] = (TUint8)iBuf.Size();									// bLength
	iBuf[1] = KUsbDescType_Interface;						// bDescriptorType
	iBuf[2] = aInterfaceNumber;								// bInterfaceNumber
	iBuf[3] = aAlternateSetting;							// bAlternateSetting
	iBuf[4] = (TUint8)aNumEndpoints;								// bNumEndpoints
	iBuf[5] = (TUint8)aClassInfo.iClassNum;							// bInterfaceClass
	iBuf[6] = (TUint8)aClassInfo.iSubClassNum;						// bInterfaceSubClass
	iBuf[7] = (TUint8)aClassInfo.iProtocolNum;						// bInterfaceProtocol
	iBuf[8] = 0;											// iInterface
	return KErrNone;
	}


// --- TUsbcEndpointDescriptor

TUsbcEndpointDescriptor::TUsbcEndpointDescriptor()
	: iBuf()
  	{
 	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcEndpointDescriptor::TUsbcEndpointDescriptor()")));
	}


TUsbcEndpointDescriptor* TUsbcEndpointDescriptor::New(TUint8 aEndpointAddress,
													  const TUsbcEndpointInfo& aEpInfo)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcEndpointDescriptor::New()")));
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
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcEndpointDescriptor::Construct()")));
	iBuf.SetMax();
	SetBufferPointer(iBuf);
	iBuf[0] = (TUint8)iBuf.Size();									// bLength
	iBuf[1] = KUsbDescType_Endpoint;						// bDescriptorType
 	iBuf[2] = aEndpointAddress;								// bEndpointAddress
	iBuf[3] = (TUint8)EpTypeMask2Value(aEpInfo.iType);				// bmAttributes
	SetWord(4, (TUint8)aEpInfo.iSize);								// wMaxPacketSize
	iBuf[6] = (TUint8)aEpInfo.iInterval;							// bInterval
	return KErrNone;
	}


// --- TUsbcAudioEndpointDescriptor

TUsbcAudioEndpointDescriptor::TUsbcAudioEndpointDescriptor()
	: iBuf()
  	{
 	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcAudioEndpointDescriptor::TUsbcAudioEndpointDescriptor()")));
	}


TUsbcAudioEndpointDescriptor* TUsbcAudioEndpointDescriptor::New(TUint8 aEndpointAddress,
																const TUsbcEndpointInfo& aEpInfo)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcAudioEndpointDescriptor::New()")));
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
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcAudioEndpointDescriptor::Construct()")));
	iBuf.SetMax();
	SetBufferPointer(iBuf);
	iBuf[0] = (TUint8)iBuf.Size();									// bLength
	iBuf[1] = KUsbDescType_Endpoint;						// bDescriptorType
 	iBuf[2] = aEndpointAddress;								// bEndpointAddress
	iBuf[3] = (TUint8)EpTypeMask2Value(aEpInfo.iType);				// bmAttributes
	SetWord(4, (TUint8)aEpInfo.iSize);								// wMaxPacketSize
	iBuf[6] = (TUint8)aEpInfo.iInterval;							// bInterval
	iBuf[7] = 0;
	iBuf[8] = 0;
	return KErrNone;
	}


// --- TUsbcClassSpecificDescriptor

TUsbcClassSpecificDescriptor::TUsbcClassSpecificDescriptor()
	: iBuf(NULL)
  	{
 	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcClassSpecificDescriptor::TUsbcClassSpecificDescriptor()")));
	}


TUsbcClassSpecificDescriptor::~TUsbcClassSpecificDescriptor()
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcClassSpecificDescriptor::~TUsbcClassSpecificDescriptor()")));
	delete iBuf;
	}


TUsbcClassSpecificDescriptor* TUsbcClassSpecificDescriptor::New(TUint8 aType, TInt aSize)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcClassSpecificDescriptor::New()")));
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
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcClassSpecificDescriptor::Construct()")));
	__NEWPLATBUF(iBuf, aSize);
	if (!iBuf)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: Allocation of CS desc buffer failed")));
		return KErrNoMemory;
		}
	SetBufferPointer(*iBuf);
	SetByte(1, aType);										// bDescriptorType
	return KErrNone;
	}


// --- TUsbcStringDescriptorBase

TUsbcStringDescriptorBase::TUsbcStringDescriptorBase()
	: /*iIndex(0),*/ iSBuf(0), iBufPtr(NULL, 0)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcStringDescriptorBase::TUsbcStringDescriptorBase()")));
	}


TUsbcStringDescriptorBase::~TUsbcStringDescriptorBase()
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcStringDescriptorBase::~TUsbcStringDescriptorBase()")));
	}


TUint16 TUsbcStringDescriptorBase::Word(TUint aPosition) const
	{
	if (aPosition <= 1)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("TUsbcStringDescriptorBase::Word: Error: Word(%d) in string descriptor!"), aPosition));
		return 0;
		}
	else
		{
		// since iBufPtr[0] is actually string descriptor byte index 2,
		// we have to subtract 2 from the absolute position.
		return SWAP_BYTES_16(*reinterpret_cast<const TUint16*>(&iBufPtr[aPosition - 2]));
		}
	}


TInt TUsbcStringDescriptorBase::GetDescriptorData(TUint8* aBuffer) const
	{
	aBuffer[0] = iSBuf[0];
	aBuffer[1] = iSBuf[1];
	__MEMCPY(&aBuffer[2], iBufPtr.Ptr(), iBufPtr.Size());
	return Size();
	}


TInt TUsbcStringDescriptorBase::GetDescriptorData(TUint8* aBuffer, TInt aMaxSize) const
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


TInt TUsbcStringDescriptorBase::Size() const
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
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcStringDescriptor::TUsbcStringDescriptor()")));
	}


TUsbcStringDescriptor::~TUsbcStringDescriptor()
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcStringDescriptor::~TUsbcStringDescriptor()")));
	delete iBuf;
	}


TUsbcStringDescriptor* TUsbcStringDescriptor::New(const TDesC8& aString)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcStringDescriptor::New")));
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
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcStringDescriptor::Construct")));
	__NEWPLATBUF(iBuf, aString.Size());
	if (!iBuf)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: Allocation of string buffer failed")));
		return KErrNoMemory;
		}
	SetBufferPointer(*iBuf);
	iBufPtr.Copy(aString);
	iSBuf.SetMax();
	iSBuf[0] = (TUint8)(iBuf->Size() + 2); // Bytes
	iSBuf[1] = KUsbDescType_String;
	return KErrNone;
	}


// --- TUsbcLangIdDescriptor

TUsbcLangIdDescriptor::TUsbcLangIdDescriptor()
	: iBuf(NULL)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcLangIdDescriptor::TUsbcLangIdDescriptor()")));
	}


TUsbcLangIdDescriptor::~TUsbcLangIdDescriptor()
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcLangIdDescriptor::~TUsbcLangIdDescriptor()")));
	}


TUsbcLangIdDescriptor* TUsbcLangIdDescriptor::New(TUint16 aLangId)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcLangIdDescriptor::New")));
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
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcLangIdDescriptor::Construct")));
	iBuf.SetMax();
	SetBufferPointer(iBuf);
	iBufPtr[0] = LowByte(SWAP_BYTES_16(aLangId));			// Language ID value
	iBufPtr[1] = HighByte(SWAP_BYTES_16(aLangId));
	iSBuf.SetMax();
	iSBuf[0] = (TUint8)(iBuf.Size() + 2);								// Bytes
	iSBuf[1] = KUsbDescType_String;
	return KErrNone;
	}


// --- TUsbcDescriptorPool

TUsbcDescriptorPool::TUsbcDescriptorPool(TUint8* aEp0_TxBuf)
//
//  The constructor for this class.
//
	: iDescriptors(4), iStrings(4),	iIfcIdx(0), iEp0_TxBuf(aEp0_TxBuf) // 4 = granularity
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcDescriptorPool::TUsbcDescriptorPool()")));
	}


TUsbcDescriptorPool::~TUsbcDescriptorPool()
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcDescriptorPool::~TUsbcDescriptorPool()")));
	// The destructor of each <class T> object is called before the objects themselves are destroyed.
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("  iDescriptors.Count(): %d"), iDescriptors.Count()));
	iDescriptors.ResetAndDestroy();
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("  iStrings.Count(): %d"), iStrings.Count()));
	iStrings.ResetAndDestroy();
	}


TInt TUsbcDescriptorPool::Init(TUsbcDeviceDescriptor* aDeviceDesc, TUsbcConfigDescriptor* aConfigDesc,
							   TUsbcLangIdDescriptor* aLangId, TUsbcStringDescriptor* aManufacturer,
							   TUsbcStringDescriptor* aProduct, TUsbcStringDescriptor* aSerialNum,
							   TUsbcStringDescriptor* aConfig)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcDescriptorPool::Init()")));
	iDescriptors.Insert(aDeviceDesc, 0);
	iDescriptors.Insert(aConfigDesc, 1);
	if (!aLangId || !aManufacturer || !aProduct || !aSerialNum || !aConfig)
		{
		// USB spec p. 202 says: "A USB device may omit all string descriptors."
		// So, either ALL string descriptors are supplied or none at all.
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: No string descriptor(s)")));
		return KErrArgument;
		}
	iStrings.Insert(aLangId, 0);
	iStrings.Insert(aManufacturer, 1);
	iStrings.Insert(aProduct, 2);
	iStrings.Insert(aSerialNum, 3);
	iStrings.Insert(aConfig, 4);
	// set string indices
	iDescriptors[0]->SetByte(14, 1);						// Device.iManufacturer
	iDescriptors[0]->SetByte(15, 2);						// Device.iProduct
	iDescriptors[0]->SetByte(16, 3);						// Device.iSerialNumber
	iDescriptors[1]->SetByte( 6, 4);						// Config.iConfiguration
	return KErrNone;
	}


TInt TUsbcDescriptorPool::FindDescriptor(TUint8 aType, TUint8 aIndex, TUint16 aLangid, TInt& aSize) const
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcDescriptorPool::FindDescriptor()")));
	TInt result = KErrGeneral;

	switch(aType)
		{
	case KUsbDescType_Device:
		if (aLangid != 0)
			{
			__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: bad langid: 0x%04x"), aLangid));
			result = KErrGeneral;							// bad langid
			}
		else if (aIndex > 0)
			{
			__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: bad device index: %d"), aIndex));
			result = KErrGeneral;							// we have only one device
			}
		else
			{
			aSize = GetDeviceDescriptor();
			result = KErrNone;
			}
		break;
	case KUsbDescType_Config:
		if (aLangid != 0)
			{
			__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: bad langid: 0x%04x"), aLangid));
			result = KErrGeneral;							// bad langid
			}
		else if (aIndex > 0)
			{
			__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: bad config index: %d"), aIndex));
			result = KErrGeneral;							// we have only one configuration
			}
		else
			{
			aSize = GetConfigDescriptor();
			result = KErrNone;
			}
		break;
	case KUsbDescType_String:
		if ((aLangid != 0) &&								// 0 addresses the LangId array
			(aLangid != iStrings[0]->Word(2)))				// we have just one (this) language
			{
			__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: bad langid (0x%04x requested, 0x%04x supported)"),
											  aLangid, iStrings[0]->Word(2)));
			result = KErrGeneral;							// bad langid
			}
		else if (aIndex >= iStrings.Count())
			{
			__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: bad string index: %d"), aIndex));
			result = KErrGeneral;
			}
		else
			{
			aSize = GetStringDescriptor(aIndex);
			result = KErrNone;
			}
		break;
	case KUsbDescType_CS_Interface:
		/* fall through */
	case KUsbDescType_CS_Endpoint:
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Warning: finding of class specific descriptors not supported")));
		break;
	default:
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: unknown descriptor type requested: %d"), aType));
		result = KErrGeneral;
		break;
		}

	return result;
	}


void TUsbcDescriptorPool::InsertDescriptor(TUsbcDescriptorBase* aDesc)
	{
	switch (aDesc->Type())
		{
	case KUsbDescType_Device:
		InsertDevDesc(aDesc);
		break;
	case KUsbDescType_Config:
		InsertConfigDesc(aDesc);
		break;
	case KUsbDescType_Interface:
		InsertIfcDesc(aDesc);
		break;
	case KUsbDescType_Endpoint:
		InsertEpDesc(aDesc);
		break;
	case KUsbDescType_CS_Interface:
		/* fall through */
	case KUsbDescType_CS_Endpoint:
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Warning: inserting class specific descriptors not supported")));
		break;
	default:
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcDescriptorPool::InsertDescriptor: Error: invalid type")));
		break;
		}
	}


void TUsbcDescriptorPool::SetIfcStringDescriptor(TUsbcStringDescriptor* aDesc, TInt aNumber, TInt aSetting)
	{
	TInt i = FindIfcDescriptor(aNumber, aSetting);
	if (i < 0)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcDescriptorPool::SetIfcStringDescriptor: error")));
		return;
		}
	// Set (append) string descriptor for specified interface
	iStrings.Append(aDesc);
	// Update this ifc descriptors' string index field
	const TInt str_idx = iStrings.Count() - 1;
	iDescriptors[i]->SetByte(8, (TUint8)str_idx);
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("  String for ifc %d/%d (@ pos %d): \"%S\""), aNumber, aSetting, str_idx,
									&iStrings[str_idx]->StringData()));
	}


void TUsbcDescriptorPool::DeleteIfcDescriptor(TInt aNumber, TInt aSetting)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcDescriptorPool::DeleteIfcDescriptor(%d, %d)"), aNumber, aSetting));
	TInt i = FindIfcDescriptor(aNumber, aSetting);
	if (i < 0)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING(" > Error: descriptor not found")));
		return;
		}
	// Delete (if necessary) specified interface's string descriptor
	TInt si = iDescriptors[i]->Byte(8);
	if (si != 0)
		{
		DeleteString(si);
		}
	// Delete specified ifc setting + all its cs descriptors + all its endpoints + all their cs descriptors:
	// find position of the next interface descriptor: we need to delete everything in between
	const TInt count = iDescriptors.Count();
	TInt j = i, n = 1;
	while (++j < count && iDescriptors[j]->Type() != KUsbDescType_Interface)
		++n;
	DeleteDescriptors(i, n);
	// Update all the following interfaces' bInterfaceNumber field if required
	// (because these descriptors might have moved down by one position)
	UpdateIfcNumbers(aNumber);
	// Update (if necessary) all interfaces' string index field
	if (si != 0)
		{
		UpdateIfcStringIndexes(si);
		}
	iIfcIdx = 0;											// ifc index no longer valid
	}


// The TC in many of the following functions stands for 'ThreadCopy',
// because that's what's happening there.

TInt TUsbcDescriptorPool::GetDeviceDescriptorTC(DThread* aThread, TDes8& aBuffer) const
	{
	return __THREADWRITE(aThread, &aBuffer, iDescriptors[0]->DescriptorData());
	}


TInt TUsbcDescriptorPool::SetDeviceDescriptorTC(DThread* aThread, const TDes8& aBuffer)
	{
	TBuf8<KUsbDescSize_Device> device;
	TInt r = __THREADREAD(aThread, &aBuffer, device);
	if (r != KErrNone)
		{
		return r;
		}
	iDescriptors[0]->SetByte(2, device[2]);					// bcdUSB
	iDescriptors[0]->SetByte(3, device[3]);					// bcdUSB (part II)
	iDescriptors[0]->SetByte(4, device[4]);					// bDeviceClass
	iDescriptors[0]->SetByte(5, device[5]);					// bDeviceSubClass
	iDescriptors[0]->SetByte(6, device[6]);					// bDeviceProtocol
	iDescriptors[0]->SetByte(8, device[8]);					// idVendor
	iDescriptors[0]->SetByte(9, device[9]);					// idVendor (part II)
	iDescriptors[0]->SetByte(10, device[10]);				// idProduct
	iDescriptors[0]->SetByte(11, device[11]);				// idProduct (part II)
	iDescriptors[0]->SetByte(12, device[12]);				// bcdDevice
	iDescriptors[0]->SetByte(13, device[13]);				// bcdDevice (part II)
	return KErrNone;
	}


TInt TUsbcDescriptorPool::GetConfigurationDescriptorTC(DThread* aThread, TDes8& aBuffer) const
	{
	return __THREADWRITE(aThread, &aBuffer, iDescriptors[1]->DescriptorData());
	}


TInt TUsbcDescriptorPool::SetConfigurationDescriptorTC(DThread* aThread, const TDes8& aBuffer)
	{
	TBuf8<KUsbDescSize_Config> config;
	TInt r = __THREADREAD(aThread, &aBuffer, config);
	if (r != KErrNone)
		{
		return r;
		}
	iDescriptors[1]->SetByte(7, config[7]);					// bmAttributes
	iDescriptors[1]->SetByte(8, config[8]);					// bMaxPower
	return KErrNone;
	}


TInt TUsbcDescriptorPool::GetInterfaceDescriptorTC(DThread* aThread, TDes8& aBuffer,
												   TInt aInterface, TInt aSetting) const
	{
	TInt i = FindIfcDescriptor(aInterface, aSetting);
	if (i < 0)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: no such interface")));
		return KErrNotFound;
		}
	return __THREADWRITE(aThread, &aBuffer, iDescriptors[i]->DescriptorData());
	}


TInt TUsbcDescriptorPool::SetInterfaceDescriptor(const TDes8& aBuffer, TInt aInterface, TInt aSetting)
	{
	TInt i = FindIfcDescriptor(aInterface, aSetting);
	if (i < 0)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: no such interface")));
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
	TInt i = FindEpDescriptor(aInterface, aSetting, aEndpointAddress);
	if (i < 0)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: no such endpoint")));
		return KErrNotFound;
		}
	return __THREADWRITE(aThread, &aBuffer, iDescriptors[i]->DescriptorData());
	}


TInt TUsbcDescriptorPool::SetEndpointDescriptorTC(DThread* aThread, const TDes8& aBuffer,
												  TInt aInterface, TInt aSetting, TUint8 aEndpointAddress)
	{
	TInt i = FindEpDescriptor(aInterface, aSetting, aEndpointAddress);
	if (i < 0)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: no such endpoint")));
		return KErrNotFound;
		}
	TBuf8<KUsbDescSize_AudioEndpoint> ep;					// it could be an audio endpoint
	TInt r = __THREADREAD(aThread, &aBuffer, ep);
	if (r != KErrNone)
		{
		return r;
		}
	iDescriptors[i]->SetByte(3, ep[3]);						// bmAttributes
	iDescriptors[i]->SetByte(6, ep[6]);						// bInterval
	if (static_cast<TUint>(iDescriptors[i]->Size()) == KUsbDescSize_AudioEndpoint)
		{
		iDescriptors[i]->SetByte(7, ep[7]);					// bRefresh
		iDescriptors[i]->SetByte(8, ep[8]);					// bSynchAddress
		}
	return KErrNone;
	}


TInt TUsbcDescriptorPool::GetEndpointDescriptorSize(TInt aInterface, TInt aSetting, TUint8 aEndpointAddress,
													TInt& aSize) const
	{
	TInt i = FindEpDescriptor(aInterface, aSetting, aEndpointAddress);
	if (i < 0)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: no such endpoint")));
		return KErrNotFound;
		}
	aSize = iDescriptors[i]->Size();
	return KErrNone;
	}


TInt TUsbcDescriptorPool::GetCSInterfaceDescriptorTC(DThread* aThread, TDes8& aBuffer,
													 TInt aInterface, TInt aSetting) const
	{
	// first find the interface
	TInt i = FindIfcDescriptor(aInterface, aSetting);
	if (i < 0)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: no such interface")));
		return KErrNotFound;
		}
	TInt r = KErrNotFound;
	TInt offset = 0;
	const TInt count = iDescriptors.Count();
	while (++i < count && iDescriptors[i]->Type() == KUsbDescType_CS_Interface)
		{
		r = __THREADWRITEOFFSET(aThread, &aBuffer, iDescriptors[i]->DescriptorData(), offset);
		if (r != KErrNone)
			break;
		offset += iDescriptors[i]->Size();
		}
	return r;
	}


TInt TUsbcDescriptorPool::SetCSInterfaceDescriptorTC(DThread* aThread, const TDes8& aBuffer,
													 TInt aInterface, TInt aSetting, TInt aSize)
	{
	// first find the interface
	TInt i = FindIfcDescriptor(aInterface, aSetting);
	if (i < 0)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: no such interface")));
		return KErrNotFound;
		}
	// find a position where to insert the new class specific interface descriptor(s)
	const TInt count = iDescriptors.Count();
	while (++i < count && iDescriptors[i]->Type() == KUsbDescType_CS_Interface)
		;
	// create a new cs descriptor
	TUsbcClassSpecificDescriptor* desc = TUsbcClassSpecificDescriptor::New(KUsbDescType_CS_Interface, aSize);
	if (!desc)
		{
		return KErrNoMemory;
		}
	iDescriptors.Insert(desc, i);
	if (iDescriptors[1])
		{
		// if there's a config descriptor (and not a NULL pointer), we update its wTotalLength field
		iDescriptors[1]->SetWord(2, (TUint8)(iDescriptors[1]->Word(2) + aSize));
		}
	// copy contents from user side
	return __THREADREAD(aThread, &aBuffer, iDescriptors[i]->DescriptorData());
	}


TInt TUsbcDescriptorPool::GetCSInterfaceDescriptorSize(TInt aInterface, TInt aSetting, TInt& aSize) const
	{
	// first find the interface
	TInt i = FindIfcDescriptor(aInterface, aSetting);
	if (i < 0)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: no such interface")));
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
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: no such endpoint")));
		return KErrNotFound;
		}
	TInt r = KErrNotFound;
	TInt offset = 0;
	const TInt count = iDescriptors.Count();
	while (++i < count && iDescriptors[i]->Type() == KUsbDescType_CS_Endpoint)
		{
		__THREADWRITEOFFSET(aThread, &aBuffer, iDescriptors[i]->DescriptorData(), offset);
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
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: no such endpoint")));
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
	if (iDescriptors[1])
		{
		// if there's a config descriptor (and not a NULL pointer), we update its wTotalLength field
		iDescriptors[1]->SetWord(2, (TUint8)(iDescriptors[1]->Word(2) + aSize));
		}
	// copy contents from user side
	return __THREADREAD(aThread, &aBuffer, iDescriptors[i]->DescriptorData());
	}


TInt TUsbcDescriptorPool::GetCSEndpointDescriptorSize(TInt aInterface, TInt aSetting,
													  TUint8 aEndpointAddress, TInt& aSize) const
	{
	// first find the endpoint
	TInt i = FindEpDescriptor(aInterface, aSetting, aEndpointAddress);
	if (i < 0)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: no such endpoint")));
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


TInt TUsbcDescriptorPool::GetManufacturerStringDescriptorTC(DThread* aThread, TDes8& aString) const
	{
	return GetDeviceStringDescriptorTC(aThread, aString, KUsbDescStringIndex_Manufact);
	}


TInt TUsbcDescriptorPool::SetManufacturerStringDescriptorTC(DThread* aThread, const TDes8& aString)
	{
	return SetDeviceStringDescriptorTC(aThread, aString, KUsbDescStringIndex_Manufact);
	}


TInt TUsbcDescriptorPool::GetProductStringDescriptorTC(DThread* aThread, TDes8& aString) const
	{
	return GetDeviceStringDescriptorTC(aThread, aString, KUsbDescStringIndex_Product);
	}


TInt TUsbcDescriptorPool::SetProductStringDescriptorTC(DThread* aThread, const TDes8& aString)
	{
	return SetDeviceStringDescriptorTC(aThread, aString, KUsbDescStringIndex_Product);
	}


TInt TUsbcDescriptorPool::GetSerialNumberStringDescriptorTC(DThread* aThread, TDes8& aString) const
	{
	return GetDeviceStringDescriptorTC(aThread, aString, KUsbDescStringIndex_Serial);
	}


TInt TUsbcDescriptorPool::SetSerialNumberStringDescriptorTC(DThread* aThread, const TDes8& aString)
	{
	return SetDeviceStringDescriptorTC(aThread, aString, KUsbDescStringIndex_Serial);
	}


TInt TUsbcDescriptorPool::GetConfigurationStringDescriptorTC(DThread* aThread, TDes8& aString) const
	{
	TBuf8<KUsbDescSize_Config> config_desc;
	iDescriptors[1]->GetDescriptorData(config_desc);
	const TInt str_idx = config_desc[KUsbDescStringIndex_Config];
	if ((str_idx > 0) && iStrings[str_idx])
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("  String @ pos %d (conf $): \"%S\""),
										str_idx, &iStrings[str_idx]->StringData()));
		return __THREADWRITE(aThread, &aString, iStrings[str_idx]->StringData());
		}
	else
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: no string descriptor @ pos %d!"), str_idx));
		return KErrNotFound;
		}
	}


TInt TUsbcDescriptorPool::SetConfigurationStringDescriptorTC(DThread* aThread, const TDes8& aString)
	{
	// we don't know the length of the string, so we have to allocate memory dynamically
	TUint strlen = __THREADDESLEN(aThread, &aString);
	if (strlen > KUsbStringDescStringMaxSize)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Warning: $ descriptor too long - string will be truncated")));
		strlen = KUsbStringDescStringMaxSize;
		}

	HBuf8Plat* strbuf = NULL;
	__NEWPLATBUF(strbuf, strlen);
	if (!strbuf)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: Memory allocation for config $ desc string failed (1)")));
		return KErrNoMemory;
		}

	TInt r;	
	__THREADREADPLATBUF(aThread, &aString, strbuf, r);
	if (r != KErrNone)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: Thread read error")));
		delete strbuf;
		return r;
		}
	TUsbcStringDescriptor* sd = TUsbcStringDescriptor::New(*strbuf);
	if (!sd)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: Memory allocation for config $ desc failed (2)")));
		delete strbuf;
		return KErrNoMemory;
		}
	TBuf8<KUsbDescSize_Config> config_desc;
	config_desc.FillZ(config_desc.MaxLength());
	iDescriptors[1]->GetDescriptorData(config_desc);
	ExchangeStringDescriptor(config_desc[KUsbDescStringIndex_Config], sd);
	delete strbuf;
	return r;
	}


// --- private ---

void TUsbcDescriptorPool::InsertDevDesc(TUsbcDescriptorBase* aDesc)
	{
	TInt count = iDescriptors.Count();
	if (count > 0)
		{
		DeleteDescriptors(0);								// if there's already something at pos. 0, delete it
		}
	iDescriptors.Insert(aDesc, 0);							// in any case: put the new descriptor at position 0
	}


void TUsbcDescriptorPool::InsertConfigDesc(TUsbcDescriptorBase* aDesc)
	{
	TInt count = iDescriptors.Count();
	if (count == 0)
		{
		TUsbcDescriptorBase* const iNullDesc = NULL;
		iDescriptors.Append(iNullDesc);						// if array's empty, put a dummy in position 0
		}
	else if (count > 1)
		{
		DeleteDescriptors(1);								// if there's already something at pos. 1, delete it
		}
	iDescriptors.Insert(aDesc, 1);							// in any case: put the new descriptor at position 1
	// Currently this code assumes, that the config descriptor is inserted _before_ any interface
	// or endpoint descriptors!
	if (iDescriptors.Count() != 2)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("TUsbcDescriptorPool::InsertConfigDesc: config descriptor will be invalid!")));
		}
	}


void TUsbcDescriptorPool::InsertIfcDesc(TUsbcDescriptorBase* aDesc)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcDescriptorPool::InsertIfcDesc()")));
	TInt count = iDescriptors.Count();
	if (count < 2)
		{
		TUsbcDescriptorBase* const iNullDesc = NULL;
		iDescriptors.Append(iNullDesc);						// if array's too small, put some dummies in
		iDescriptors.Append(iNullDesc);
		}
	TBool interface_exists = EFalse;						// set to 'true' if we're adding an alternate
															// setting to an already existing interface
	TInt i = 2;
	while (i < count)
		{
		if (iDescriptors[i]->Type() == KUsbDescType_Interface)
			{
			if (iDescriptors[i]->Byte(2) > aDesc->Byte(2))
				{
				// our interface number is less than the one's just found => insert before it (= here)
				break;
				}
			else if (iDescriptors[i]->Byte(2) == aDesc->Byte(2))
				{
				interface_exists = ETrue;
				// same interface number => look at settings number
				if (iDescriptors[i]->Byte(3) > aDesc->Byte(3))
					{
					// our setting number is less than the one's found => insert before (= here)
					break;
					}
				else if (iDescriptors[i]->Byte(3) == aDesc->Byte(3))
					{
					__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("TUsbcDescriptorPool::InsertIfcDesc: error: first delete old one!")));
					return;
					}
				}
			}
		++i;
		}
	iDescriptors.Insert(aDesc, i);							// in any case: put the new descriptor at position i
	if (iDescriptors[1])
		{
		// if there's a config descriptor (and not a NULL pointer), update its wTotalLength field...
		iDescriptors[1]->SetWord(2, (TUint8)(iDescriptors[1]->Word(2) + KUsbDescSize_Interface));
		//  and increment bNumInterfaces if this is the first setting for the interface
		if (!interface_exists)
			iDescriptors[1]->SetByte(4, (TUint8)(iDescriptors[1]->Byte(4) + 1));
		}
	iIfcIdx = i;
	}


void TUsbcDescriptorPool::InsertEpDesc(TUsbcDescriptorBase* aDesc)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcDescriptorPool::InsertEpDesc()")));
	if (iIfcIdx == 0)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("TUsbcDescriptorPool::InsertEpDesc: error: only after interface")));
		return;
		}
	TInt count = iDescriptors.Count();
	TInt i = iIfcIdx + 1;
	while (i < count)
		{
		if (iDescriptors[i]->Type() != KUsbDescType_Endpoint)
			break;
		++i;
		}
	iDescriptors.Insert(aDesc, i);							// put the new descriptor at position i
	if (iDescriptors[1])
		{
		// if there's a config descriptor (and not a NULL pointer), update its wTotalLength field
		iDescriptors[1]->SetWord(2, (TUint8)(iDescriptors[1]->Word(2) + aDesc->Size()));
		}
	}


TInt TUsbcDescriptorPool::FindIfcDescriptor(TInt aIfcNumber, TInt aIfcSetting) const
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcDescriptorPool::FindIfcDescriptor(%d, %d)"),
									aIfcNumber, aIfcSetting));
	TInt count = iDescriptors.Count();
	for (TInt i = 2; i < count; ++i)
		{
		if ((iDescriptors[i]->Type() == KUsbDescType_Interface) &&
			(iDescriptors[i]->Byte(2) == aIfcNumber) &&
			(iDescriptors[i]->Byte(3) == aIfcSetting))
			{
			return i;
			}
		}
	__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: no such interface")));
	return -1;
	}


TInt TUsbcDescriptorPool::FindEpDescriptor(TInt aIfcNumber, TInt aIfcSetting, TUint8 aEpAddress) const
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcDescriptorPool::FindEpDescriptor(%d, %d, 0x%02x)"),
									aIfcNumber, aIfcSetting, aEpAddress));
	// first find the interface
	TInt ifc = FindIfcDescriptor(aIfcNumber, aIfcSetting);
	if (ifc < 0)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: no such interface")));
		return ifc;
		}
	TInt count = iDescriptors.Count();
	// then, before the next interface, try to locate the endpoint
	for (TInt i = ifc + 1; i < count; ++i)
		{
		if (iDescriptors[i]->Type() == KUsbDescType_Interface)
			{
			__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: no such endpoint before next interface")));
			return -1;
			}
		else if ((iDescriptors[i]->Type() == KUsbDescType_Endpoint) &&
				 (iDescriptors[i]->Byte(2) == aEpAddress))
			{
			return i;										// found
			}
		}
	__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: no such endpoint")));
	return -1;
	}


void TUsbcDescriptorPool::DeleteDescriptors(TInt aIndex, TInt aCount)
	{
	if (aCount <= 0)
		{
		return;
		}
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("  Removing descriptors at index %d:"), aIndex));
	while (aCount--)
		{
		// in this loop we don't decrement aIndex, because after deleting an element
		// aIndex is already indexing the next one!
		TUsbcDescriptorBase* ptr = iDescriptors[aIndex];
		if (iDescriptors[1])
			{
			// if there's a config descriptor (and not a NULL pointer),
			if (ptr->Type() == KUsbDescType_Interface)
				{
				__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("  - an interface descriptor")));
				// if it's an interface descriptor:
				// we update its wTotalLength field...
				iDescriptors[1]->SetWord(2, (TUint8)(iDescriptors[1]->Word(2) - KUsbDescSize_Interface));
				}
			else if (ptr->Type() == KUsbDescType_Endpoint)
				{
				__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("  - an endpoint descriptor")));
				// if it's an endpoint descriptor:
				// we only update its wTotalLength field
				iDescriptors[1]->SetWord(2, (TUint8)(iDescriptors[1]->Word(2) - ptr->Size()));
				}
			else if (ptr->Type() == KUsbDescType_CS_Interface || ptr->Type() == KUsbDescType_CS_Endpoint)
				{
				__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("  - a class specific descriptor")));
				// if it's an class specific descriptor:
				// we only update its wTotalLength field
				iDescriptors[1]->SetWord(2, (TUint8)(iDescriptors[1]->Word(2) - ptr->Size()));
				}
			}
		iDescriptors.Remove(aIndex);
		delete ptr;
		}
	}


void TUsbcDescriptorPool::DeleteString(TInt aIndex)
	{
	TUsbcStringDescriptorBase* ptr = iStrings[aIndex];
	iStrings.Remove(aIndex);
	delete ptr;
	}


void TUsbcDescriptorPool::UpdateIfcNumbers(TInt aNumber)
	{
	const TInt count = iDescriptors.Count();
	for (TInt i = 2; i < count; ++i)
		{
		if ((iDescriptors[i]->Type() == KUsbDescType_Interface) &&
			(iDescriptors[i]->Byte(2) == aNumber))
			{
			// there's still an interface with 'number' so we don't need to update anything
			return;
			}
		}
	// if we haven't returned yet, we decrement bNumInterfaces
	iDescriptors[1]->SetByte(4, (TUint8)(iDescriptors[1]->Byte(4) - 1));
	}


void TUsbcDescriptorPool::UpdateIfcStringIndexes(TInt aStringIndex)
	{
	// aStringIndex is the index value of the string descriptor that has just been removed.
	// We update all ifc descriptors with a string index value that is greater than aStringIndex,
	// because those strings moved all down by one position.
	//
	TInt count = iDescriptors.Count();
	for (TInt i = 2; i < count; ++i)
		{
		if ((iDescriptors[i]->Type() == KUsbDescType_Interface) &&
			(iDescriptors[i]->Byte(8) > aStringIndex))
			{
			iDescriptors[i]->SetByte(8, (TUint8)(iDescriptors[i]->Byte(8) - 1));
			}
		}
	}


//
// Only used for Ep0 standard requests, so target buffer could be hard-wired.
//
TInt TUsbcDescriptorPool::GetDeviceDescriptor() const
	{
	return iDescriptors[0]->GetDescriptorData(iEp0_TxBuf, KUsbcBufSz_Ep0Tx);
	}


//
// Only used for Ep0 standard requests, so target buffer could be hard-wired.
//
TInt TUsbcDescriptorPool::GetConfigDescriptor() const
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcDescriptorPool::GetConfigDescriptor()")));
	TInt copied = 0;
	TInt count = iDescriptors.Count();
	TUint8* buf = iEp0_TxBuf;
	for (TInt i = 1; i < count; ++i)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING(" > desc[%02d]: type = 0x%02x size = %d "),
										i, iDescriptors[i]->Type(), iDescriptors[i]->Size()));
		const TInt size = iDescriptors[i]->GetDescriptorData(buf, KUsbcBufSz_Ep0Tx - copied);
		if (size == 0)
			{
			// There was no buffer space to copy the descriptor -> no use to proceed
			break;
			}
		copied += size;
		if (copied >= KUsbcBufSz_Ep0Tx)
			{
			// There's no buffer space left -> we need to stop copying here
			break;
			}
		buf += size;
		}
	return copied;
	}


//
// Only used for Ep0 standard requests, so target buffer could be hard-wired.
//
TInt TUsbcDescriptorPool::GetStringDescriptor(TInt aIndex) const
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcDescriptorPool::GetStringDescriptor()")));
	// I really would have liked to display here the descriptor contents, but without trailing zero
	// we got a problem: how could we tell printf where the string ends? We would have to
	// dynamically allocate memory (since we don't know the size in advance), copy the descriptor
	// contents there, append a zero, and give this to printf. That's a bit too much effort...
	if (iStrings[aIndex])
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("  String @ pos %d"), aIndex));
		TInt size = iStrings[aIndex]->GetDescriptorData(iEp0_TxBuf, KUsbcBufSz_Ep0Tx);
		return size;
		}
	else
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: no descriptor @ pos %d!"), aIndex));
		return 0;
		}
	}


TInt TUsbcDescriptorPool::GetDeviceStringDescriptorTC(DThread* aThread, TDes8& aString, TInt aIndex) const
	{
	TBuf8<KUsbDescSize_Device> dev_desc;
	iDescriptors[0]->GetDescriptorData(dev_desc);
	const TInt str_idx = dev_desc[aIndex];
	if ((str_idx > 0) && iStrings[str_idx])
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("  String @ pos %d (device $): \"%S\""),
										str_idx, &iStrings[str_idx]->StringData()));
		return __THREADWRITE(aThread, &aString, iStrings[str_idx]->StringData());
		}
	else
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: no string descriptor @ pos %d!"), str_idx));
		return KErrNotFound;
		}
	}


TInt TUsbcDescriptorPool::SetDeviceStringDescriptorTC(DThread* aThread, const TDes8& aString, TInt aIndex)
	{
	// we don't know the length of the string, so we have to allocate memory dynamically
	TUint strlen = __THREADDESLEN(aThread, &aString);
	if (strlen > KUsbStringDescStringMaxSize)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Warning: $ descriptor too long - string will be truncated")));
		strlen = KUsbStringDescStringMaxSize;
		}
	
	HBuf8Plat* strbuf = NULL;
	__NEWPLATBUF(strbuf, strlen);
	if (!strbuf)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: Memory allocation for dev $ desc string failed (1)")));
		return KErrNoMemory;
		}

	TInt r;	
	__THREADREADPLATBUF(aThread, &aString, strbuf, r);
	if (r != KErrNone)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: Thread read error")));
		delete strbuf;
		return r;
		}
	TUsbcStringDescriptor* sd = TUsbcStringDescriptor::New(*strbuf);
	if (!sd)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: Memory allocation for dev $ desc failed (2)")));
		delete strbuf;
		return KErrNoMemory;
		}
	TBuf8<KUsbDescSize_Device> dev_desc;
	dev_desc.FillZ(dev_desc.MaxLength());
	iDescriptors[0]->GetDescriptorData(dev_desc);
	ExchangeStringDescriptor(dev_desc[aIndex], sd);
	delete strbuf;
	return r;
	}


TInt TUsbcDescriptorPool::ExchangeStringDescriptor(TInt aIndex, const TUsbcStringDescriptor* aDesc)
	{
	if (aIndex <= 0)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf(__KSTRING("  Error: invalid string descriptor index: %d!"), aIndex));
		return KErrArgument;
		}
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("  Exchanging string descriptor @ index %d"), aIndex));
	DeleteString(aIndex);
	iStrings.Insert(aDesc, aIndex);
	return KErrNone;
	}


// -eof-
