// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\usbcdesc.h
// USB descriptors and their management.
// 
//

/**
 @file
 @internalTechnology
*/

#if !defined(__USBCDESC_H__)
#define __USBCDESC_H__

#include "kerndefs.h"
#include <d32usbc.h>

class TUsbcDescriptorBase
	{
public:
	virtual ~TUsbcDescriptorBase();
	void SetByte(TUint aPosition, TUint8 aValue);
	void SetWord(TUint aPosition, TUint16 aValue);
	TUint8 Byte(TUint aPosition) const;
	TUint16 Word(TUint aPosition) const;
	void GetDescriptorData(TDes8& aBuffer) const;
	TInt GetDescriptorData(TUint8* aBuffer) const;
	TInt GetDescriptorData(TUint8* aBuffer, TInt aMaxSize) const;
	const TDes8& DescriptorData() const;
	TDes8& DescriptorData();
	TInt Size() const;
	TUint8 Type() const;
protected:
	TUsbcDescriptorBase();
	void SetBufferPointer(const TDesC8& aDes);
private:
#ifdef USB_SUPPORTS_SET_DESCRIPTOR_REQUEST
	TUint8 iIndex;											// only needed for SET_DESCRIPTOR
#endif
	TPtr8 iBufPtr;
	};


class TUsbcDeviceDescriptor : public TUsbcDescriptorBase
	{
public:
	static TUsbcDeviceDescriptor* New(TUint8 aDeviceClass, TUint8 aDeviceSubClass,
									  TUint8 aDeviceProtocol, TUint8 aMaxPacketSize0,
									  TUint16 aVendorId, TUint16 aProductId,
									  TUint16 aDeviceRelease, TUint8 aNumConfigurations);
private:
	TUsbcDeviceDescriptor();
	TInt Construct(TUint8 aDeviceClass, TUint8 aDeviceSubClass, TUint8 aDeviceProtocol,
				   TUint8 aMaxPacketSize0, TUint16 aVendorId, TUint16 aProductId,
				   TUint16 aDeviceRelease, TUint8 aNumConfigurations);
	TBuf8<KUsbDescSize_Device> iBuf;
	};


class TUsbcConfigDescriptor : public TUsbcDescriptorBase
	{
public:
	static TUsbcConfigDescriptor* New(TUint8 aConfigurationValue, TBool aSelfPowered, TBool aRemoteWakeup,
									  TUint8 aMaxPower);	// give MaxPower in milliamps!
private:
	TUsbcConfigDescriptor();
	TInt Construct(TUint8 aConfigurationValue, TBool aSelfPowered, TBool aRemoteWakeup, TUint8 aMaxPower);
	TBuf8<KUsbDescSize_Config> iBuf;
	};


class TUsbcInterfaceDescriptor : public TUsbcDescriptorBase
	{
public:
	static TUsbcInterfaceDescriptor* New(TUint8 aInterfaceNumber, TUint8 aAlternateSetting, TInt NumEndpoints,
										 const TUsbcClassInfo& aClassInfo);
private:
	TUsbcInterfaceDescriptor();
	TInt Construct(TUint8 aInterfaceNumber, TUint8 aAlternateSetting, TInt aNumEndpoints,
				   const TUsbcClassInfo& aClassInfo);
	TBuf8<KUsbDescSize_Interface> iBuf;
	};


class TUsbcEndpointDescriptor : public TUsbcDescriptorBase
	{
public:
	static TUsbcEndpointDescriptor* New(TUint8 aEndpointAddress, const TUsbcEndpointInfo& aEpInfo);
private:
	TUsbcEndpointDescriptor();
	TInt Construct(TUint8 aEndpointAddress, const TUsbcEndpointInfo& aEpInfo);
	TBuf8<KUsbDescSize_Endpoint> iBuf;
	};


class TUsbcAudioEndpointDescriptor : public TUsbcDescriptorBase
	{
public:
	static TUsbcAudioEndpointDescriptor* New(TUint8 aEndpointAddress, const TUsbcEndpointInfo& aEpInfo);
private:
	TUsbcAudioEndpointDescriptor();
	TInt Construct(TUint8 aEndpointAddress, const TUsbcEndpointInfo& aEpInfo);
	TBuf8<KUsbDescSize_AudioEndpoint> iBuf;
	};


class TUsbcClassSpecificDescriptor : public TUsbcDescriptorBase
	{
public:
	virtual ~TUsbcClassSpecificDescriptor();
	static TUsbcClassSpecificDescriptor* New(TUint8 aType, TInt aSize);
private:
	TUsbcClassSpecificDescriptor();
	TInt Construct(TUint8 aType, TInt aSize);
	HBuf8Plat* iBuf;
	};


class TUsbcStringDescriptorBase
	{
public:
	virtual ~TUsbcStringDescriptorBase();
	TUint16 Word(TUint aPosition) const;
	TInt GetDescriptorData(TUint8* aBuffer) const;
	TInt GetDescriptorData(TUint8* aBuffer, TInt aMaxSize) const;
	const TDes8& StringData() const;
	TDes8& StringData();
	TInt Size() const;
	void SetBufferPointer(const TDesC8& aDes);
protected:
	TUsbcStringDescriptorBase();
	TBuf8<2> iSBuf;
	TPtr8 iBufPtr;
private:
//	TUint8 iIndex;											// not needed in DescriptorPool: position == index
	};


class TUsbcStringDescriptor : public TUsbcStringDescriptorBase
	{
public:
	virtual ~TUsbcStringDescriptor();
	static TUsbcStringDescriptor* New(const TDesC8& aString);
private:
	TUsbcStringDescriptor();								// use static New
	TInt Construct(const TDesC8& aString);
	HBuf8Plat* iBuf;
	};


// Currently we support only one language, and thus there's no need to provide
// a LangId string descriptor with more than one array element.
class TUsbcLangIdDescriptor : public TUsbcStringDescriptorBase
	{
public:
	virtual ~TUsbcLangIdDescriptor();
	static TUsbcLangIdDescriptor* New(TUint16 aLangId);
private:
	TUsbcLangIdDescriptor();								// use static New
	TInt Construct(TUint16 aLangId);
	TBuf8<2> iBuf;
	};


class TUsbcDescriptorPool
	{
public:
	TUsbcDescriptorPool(TUint8* aEp0_TxBuf);
	~TUsbcDescriptorPool();
	TInt Init(TUsbcDeviceDescriptor* aDeviceDesc, TUsbcConfigDescriptor* aConfigDesc,
			  TUsbcLangIdDescriptor* aLangId, TUsbcStringDescriptor* aManufacturer,
			  TUsbcStringDescriptor* aProduct, TUsbcStringDescriptor* aSerialNum,
			  TUsbcStringDescriptor* aConfig);
	// Descriptors
	TInt FindDescriptor(TUint8 aType, TUint8 aIndex, TUint16 aLangid, TInt& aSize) const;
	void InsertDescriptor(TUsbcDescriptorBase* aDesc);
	void DeleteIfcDescriptor(TInt aNumber, TInt aSetting = 0);

	// The TC in many of the following functions stands for 'ThreadCopy' because that's what happens there.
	TInt GetDeviceDescriptorTC(DThread* aThread, TDes8& aBuffer) const;
	TInt SetDeviceDescriptorTC(DThread* aThread, const TDes8& aBuffer);
	TInt GetConfigurationDescriptorTC(DThread* aThread, TDes8& aBuffer) const;
	TInt SetConfigurationDescriptorTC(DThread* aThread, const TDes8& aBuffer);
	TInt GetInterfaceDescriptorTC(DThread* aThread, TDes8& aBuffer, TInt aInterface, TInt aSetting) const;
	TInt SetInterfaceDescriptor(const TDes8& aBuffer, TInt aInterface, TInt aSetting);
	TInt GetEndpointDescriptorTC(DThread* aThread, TDes8& aBuffer, TInt aInterface, TInt aSetting,
								 TUint8 aEndpointAddress) const;
	TInt SetEndpointDescriptorTC(DThread* aThread, const TDes8& aBuffer, TInt aInterface, TInt aSetting,
								 TUint8 aEndpointAddress);
	TInt GetEndpointDescriptorSize(TInt aInterface, TInt aSetting, TUint8 aEndpointAddress, TInt& aSize) const;
	TInt GetCSInterfaceDescriptorTC(DThread* aThread, TDes8& aBuffer, TInt aInterface, TInt aSetting) const;
	TInt SetCSInterfaceDescriptorTC(DThread* aThread, const TDes8& aBuffer, TInt aInterface, TInt aSetting,
									TInt aSize);
	TInt GetCSInterfaceDescriptorSize(TInt aInterface, TInt aSetting, TInt& aSize) const;
	TInt GetCSEndpointDescriptorTC(DThread* aThread, TDes8& aBuffer, TInt aInterface, TInt aSetting,
								   TUint8 aEndpointAddress) const;
	TInt SetCSEndpointDescriptorTC(DThread* aThread, const TDes8& aBuffer, TInt aInterface, TInt aSetting,
								   TUint8 aEndpointAddress, TInt aSize);
	TInt GetCSEndpointDescriptorSize(TInt aInterface, TInt aSetting, TUint8 aEndpointAddress, TInt& aSize) const;

	// String descriptors
	void SetIfcStringDescriptor(TUsbcStringDescriptor* aDesc, TInt aNumber, TInt aSetting = 0);
	TInt GetManufacturerStringDescriptorTC(DThread* aThread, TDes8& aString) const;
	TInt SetManufacturerStringDescriptorTC(DThread* aThread, const TDes8& aString);
	TInt GetProductStringDescriptorTC(DThread* aThread, TDes8& aString) const;
	TInt SetProductStringDescriptorTC(DThread* aThread, const TDes8& aString);
	TInt GetSerialNumberStringDescriptorTC(DThread* aThread, TDes8& aString) const;
	TInt SetSerialNumberStringDescriptorTC(DThread* aThread, const TDes8& aString);
	TInt GetConfigurationStringDescriptorTC(DThread* aThread, TDes8& aString) const;
	TInt SetConfigurationStringDescriptorTC(DThread* aThread, const TDes8& aString);
private:
	void InsertDevDesc(TUsbcDescriptorBase* aDesc);
	void InsertConfigDesc(TUsbcDescriptorBase* aDesc);
	void InsertIfcDesc(TUsbcDescriptorBase* aDesc);
	void InsertEpDesc(TUsbcDescriptorBase* aDesc);
	TInt FindIfcDescriptor(TInt aIfcNumber, TInt aIfcSetting) const;
	TInt FindEpDescriptor(TInt aIfcNumber, TInt aIfcSetting, TUint8 aEpAddress) const;
	void DeleteDescriptors(TInt aIndex, TInt aCount = 1);
	void DeleteString(TInt aIndex);
	void UpdateIfcNumbers(TInt aNumber);
	void UpdateIfcStringIndexes(TInt aStringIndex);
	TInt GetDeviceDescriptor() const;
	TInt GetConfigDescriptor() const;
	TInt GetStringDescriptor(TInt aIndex) const;
	TInt GetDeviceStringDescriptorTC(DThread* aThread, TDes8& aString, TInt aIndex) const;
	TInt SetDeviceStringDescriptorTC(DThread* aThread, const TDes8& aString, TInt aIndex);
	TInt ExchangeStringDescriptor(TInt aIndex, const TUsbcStringDescriptor* aDesc);
private:
	// Data members
	RPointerArray<TUsbcDescriptorBase> iDescriptors;
	RPointerArray<TUsbcStringDescriptorBase> iStrings;
	TInt iIfcIdx;
	TUint8* const iEp0_TxBuf;								// points to the controller's Ep0 Tx buffer
	};


#endif	// __USBCDESC_H__
