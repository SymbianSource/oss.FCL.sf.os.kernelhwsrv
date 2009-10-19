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
// e32/include/drivers/usbcdesc.h
// USB descriptors and their management.
// 
//

/**
 @file usbcdesc.h
 @internalTechnology
*/

#ifndef __USBCDESC_H__
#define __USBCDESC_H__

#include <kernel/kernel.h>


// Hard-wired positions of some descriptors in iDescriptors array (whether present or not):
static const TInt KDescPosition_Device           = 0;
static const TInt KDescPosition_DeviceQualifier  = 1;
static const TInt KDescPosition_OtherSpeedConfig = 2;
static const TInt KDescPosition_Config           = 3;
static const TInt KDescPosition_Otg              = 4;
static const TInt KDescPosition_FirstAvailable   = 5;

// Hard-wired positions of string descriptors in iStrings array (whether present or not):
static const TInt KStringPosition_Langid           = 0;
static const TInt KStringPosition_Manufact         = 1;
static const TInt KStringPosition_Product          = 2;
static const TInt KStringPosition_Serial           = 3;
static const TInt KStringPosition_Config           = 4;
static const TInt KStringPosition_OtherSpeedConfig = 5;
static const TInt KStringPosition_FirstAvailable   = 6;


class TUsbcDescriptorBase
	{
public:
	virtual ~TUsbcDescriptorBase();
	void SetByte(TInt aPosition, TUint8 aValue);
	void SetWord(TInt aPosition, TUint16 aValue);
	TUint8 Byte(TInt aPosition) const;
	TUint16 Word(TInt aPosition) const;
	void GetDescriptorData(TDes8& aBuffer) const;
	TInt GetDescriptorData(TUint8* aBuffer) const;
	TInt GetDescriptorData(TUint8* aBuffer, TUint aMaxSize) const;
	const TDes8& DescriptorData() const;
	TDes8& DescriptorData();
	TUint Size() const;
	TUint8 Type() const;
	virtual void UpdateFs();
	virtual void UpdateHs();
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
	/** aMaxPacketSize0 should be the Ep0 max packet size for FS operation (as the HS size
		is fixed and known).
	*/
	static TUsbcDeviceDescriptor* New(TUint8 aDeviceClass, TUint8 aDeviceSubClass,
									  TUint8 aDeviceProtocol, TUint8 aMaxPacketSize0,
									  TUint16 aVendorId, TUint16 aProductId,
									  TUint16 aDeviceRelease, TUint8 aNumConfigurations);
	virtual void UpdateFs();
	virtual void UpdateHs();
private:
	TUsbcDeviceDescriptor();
	TInt Construct(TUint8 aDeviceClass, TUint8 aDeviceSubClass, TUint8 aDeviceProtocol,
				   TUint8 aMaxPacketSize0, TUint16 aVendorId, TUint16 aProductId,
				   TUint16 aDeviceRelease, TUint8 aNumConfigurations);
	TBuf8<KUsbDescSize_Device> iBuf;
	TUint8 iEp0Size_Fs;										// holds Ep0 size for FS (could be < 64)
	};


class TUsbcDeviceQualifierDescriptor : public TUsbcDescriptorBase
	{
public:
	/** aMaxPacketSize0 should be the Ep0 max packet size for FS operation (as the HS size
		is fixed and known).
	*/
	static TUsbcDeviceQualifierDescriptor* New(TUint8 aDeviceClass, TUint8 aDeviceSubClass,
											   TUint8 aDeviceProtocol, TUint8 aMaxPacketSize0,
											   TUint8 aNumConfigurations, TUint8 aReserved=0);
	virtual void UpdateFs();
	virtual void UpdateHs();
private:
	TUsbcDeviceQualifierDescriptor();
	TInt Construct(TUint8 aDeviceClass, TUint8 aDeviceSubClass, TUint8 aDeviceProtocol,
				   TUint8 aMaxPacketSize0, TUint8 aNumConfigurations, TUint8 aReserved);
	TBuf8<KUsbDescSize_DeviceQualifier> iBuf;
	TUint8 iEp0Size_Fs;										// holds Ep0 size for FS (could be < 64)
	};


class TUsbcConfigDescriptor : public TUsbcDescriptorBase
	{
public:
	/** aMaxPower should be given here in milliamps (not mA/2). */
	static TUsbcConfigDescriptor* New(TUint8 aConfigurationValue, TBool aSelfPowered, TBool aRemoteWakeup,
									  TUint16 aMaxPower);
private:
	TUsbcConfigDescriptor();
	TInt Construct(TUint8 aConfigurationValue, TBool aSelfPowered, TBool aRemoteWakeup, TUint16 aMaxPower);
	TBuf8<KUsbDescSize_Config> iBuf;
	};


// The Other_Speed_Configuration descriptor has same size and layout as the
// standard Configuration descriptor, therefore we don't need a new definition.
typedef TUsbcConfigDescriptor TUsbcOtherSpeedConfigDescriptor;


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


class TUsbcEndpointDescriptorBase : public TUsbcDescriptorBase
	{
public:
	virtual void UpdateFs();
	virtual void UpdateHs();
protected:
	TInt Construct(const TUsbcEndpointInfo& aEpInfo);
	TUsbcEndpointDescriptorBase();
protected:
	/** Stores the endpoint size to be used for FS. */
	TInt iEpSize_Fs;
	/** Stores the endpoint size to be used for HS. */
	TInt iEpSize_Hs;
	/** Stores the endpoint polling interval to be used for FS. */
	TInt iInterval_Fs;
	/** Stores the endpoint polling interval to be used for HS. */
	TInt iInterval_Hs;
	};


class TUsbcEndpointDescriptor : public TUsbcEndpointDescriptorBase
	{
public:
	static TUsbcEndpointDescriptor* New(TUint8 aEndpointAddress, const TUsbcEndpointInfo& aEpInfo);
private:
	TUsbcEndpointDescriptor();
	TInt Construct(TUint8 aEndpointAddress, const TUsbcEndpointInfo& aEpInfo);
	TBuf8<KUsbDescSize_Endpoint> iBuf;
	};


class TUsbcAudioEndpointDescriptor : public TUsbcEndpointDescriptorBase
	{
public:
	static TUsbcAudioEndpointDescriptor* New(TUint8 aEndpointAddress, const TUsbcEndpointInfo& aEpInfo);
private:
	TUsbcAudioEndpointDescriptor();
	TInt Construct(TUint8 aEndpointAddress, const TUsbcEndpointInfo& aEpInfo);
	TBuf8<KUsbDescSize_AudioEndpoint> iBuf;
	};


class TUsbcOtgDescriptor : public TUsbcDescriptorBase
	{
public:
	static TUsbcOtgDescriptor* New(TBool aHnpSupport, TBool aSrpSupport);
private:
	TUsbcOtgDescriptor();
	TInt Construct(TBool aHnpSupport, TBool aSrpSupport);
	TBuf8<KUsbDescSize_Otg> iBuf;
	};


class TUsbcClassSpecificDescriptor : public TUsbcDescriptorBase
	{
public:
	virtual ~TUsbcClassSpecificDescriptor();
	static TUsbcClassSpecificDescriptor* New(TUint8 aType, TInt aSize);
private:
	TUsbcClassSpecificDescriptor();
	TInt Construct(TUint8 aType, TInt aSize);
	HBuf8* iBuf;
	};


class TUsbcStringDescriptorBase
	{
public:
	virtual ~TUsbcStringDescriptorBase();
	TUint16 Word(TInt aPosition) const;
	void SetWord(TInt aPosition, TUint16 aValue);
	TInt GetDescriptorData(TUint8* aBuffer) const;
	TInt GetDescriptorData(TUint8* aBuffer, TUint aMaxSize) const;
	const TDes8& StringData() const;
	TDes8& StringData();
	TUint Size() const;
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
	TUsbcStringDescriptor();
	TInt Construct(const TDesC8& aString);
	HBuf8* iBuf;
	};


// Currently we support only one language, and thus there's no need to provide
// a LangId string descriptor with more than one array element.
class TUsbcLangIdDescriptor : public TUsbcStringDescriptorBase
	{
public:
	virtual ~TUsbcLangIdDescriptor();
	static TUsbcLangIdDescriptor* New(TUint16 aLangId);
private:
	TUsbcLangIdDescriptor();
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
			  TUsbcStringDescriptor* aConfig, TUsbcOtgDescriptor* aOtgDesc);
	TInt InitHs();
	TInt UpdateDescriptorsFs();
	TInt UpdateDescriptorsHs();

	// Descriptors
	TInt FindDescriptor(TUint8 aType, TUint8 aIndex, TUint16 aLangid, TInt& aSize) const;
	void InsertDescriptor(TUsbcDescriptorBase* aDesc);
	void DeleteIfcDescriptor(TInt aNumber, TInt aSetting=0);
	// The TC in many of the following functions stands for 'ThreadCopy' because that's what happens there.
	TInt GetDeviceDescriptorTC(DThread* aThread, TDes8& aBuffer) const;
	TInt SetDeviceDescriptorTC(DThread* aThread, const TDes8& aBuffer);
	TInt GetConfigurationDescriptorTC(DThread* aThread, TDes8& aBuffer) const;
	TInt SetConfigurationDescriptorTC(DThread* aThread, const TDes8& aBuffer);
    TInt GetOtgDescriptorTC(DThread* aThread, TDes8& aBuffer) const;
	TInt SetOtgDescriptor(const TDesC8& aBuffer);
	TInt GetInterfaceDescriptorTC(DThread* aThread, TDes8& aBuffer, TInt aInterface, TInt aSetting) const;
	TInt SetInterfaceDescriptor(const TDes8& aBuffer, TInt aInterface, TInt aSetting);
	TInt GetEndpointDescriptorTC(DThread* aThread, TDes8& aBuffer, TInt aInterface, TInt aSetting,
								 TUint8 aEndpointAddress) const;
	TInt SetEndpointDescriptorTC(DThread* aThread, const TDes8& aBuffer, TInt aInterface, TInt aSetting,
								 TUint8 aEndpointAddress);
	TInt GetEndpointDescriptorSize(TInt aInterface, TInt aSetting, TUint8 aEndpointAddress, TInt& aSize) const;
	TInt GetDeviceQualifierDescriptorTC(DThread* aThread, TDes8& aBuffer) const;
	TInt SetDeviceQualifierDescriptorTC(DThread* aThread, const TDes8& aBuffer);
	TInt GetOtherSpeedConfigurationDescriptorTC(DThread* aThread, TDes8& aBuffer) const;
	TInt SetOtherSpeedConfigurationDescriptorTC(DThread* aThread, const TDes8& aBuffer);
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
	void SetIfcStringDescriptor(TUsbcStringDescriptor* aDesc, TInt aNumber, TInt aSetting=0);
	TInt GetStringDescriptorLangIdTC(DThread* aThread, TDes8& aLangId) const;
	TInt SetStringDescriptorLangId(TUint16 aLangId);
	TInt GetManufacturerStringDescriptorTC(DThread* aThread, TDes8& aString) const;
	TInt SetManufacturerStringDescriptorTC(DThread* aThread, const TDes8& aString);
	TInt RemoveManufacturerStringDescriptor();
	TInt GetProductStringDescriptorTC(DThread* aThread, TDes8& aString) const;
	TInt SetProductStringDescriptorTC(DThread* aThread, const TDes8& aString);
	TInt RemoveProductStringDescriptor();
	TInt GetSerialNumberStringDescriptorTC(DThread* aThread, TDes8& aString) const;
	TInt SetSerialNumberStringDescriptorTC(DThread* aThread, const TDes8& aString);
	TInt RemoveSerialNumberStringDescriptor();
	TInt GetConfigurationStringDescriptorTC(DThread* aThread, TDes8& aString) const;
	TInt SetConfigurationStringDescriptorTC(DThread* aThread, const TDes8& aString);
	TInt RemoveConfigurationStringDescriptor();
	TInt GetStringDescriptorTC(DThread* aThread, TInt aIndex, TDes8& aString) const;
	TInt SetStringDescriptorTC(DThread* aThread, TInt aIndex, const TDes8& aString);
	TInt RemoveStringDescriptor(TInt aIndex);

private:
	// Descriptors
	void InsertIfcDesc(TUsbcDescriptorBase* aDesc);
	void InsertEpDesc(TUsbcDescriptorBase* aDesc);
	TInt FindIfcDescriptor(TInt aIfcNumber, TInt aIfcSetting) const;
	TInt FindEpDescriptor(TInt aIfcNumber, TInt aIfcSetting, TUint8 aEpAddress) const;
	void DeleteDescriptors(TInt aIndex, TInt aCount = 1);
	void UpdateConfigDescriptorLength(TInt aLength);
	void UpdateConfigDescriptorNumIfcs(TInt aNumber);
	void UpdateIfcNumbers(TInt aNumber);
	TInt GetDeviceDescriptor(TInt aIndex) const;
	TInt GetConfigurationDescriptor(TInt aIndex) const;
	TInt GetOtgDescriptor() const;

	// String descriptors
	TInt GetStringDescriptor(TInt aIndex) const;
	TInt GetDeviceStringDescriptorTC(DThread* aThread, TDes8& aString, TInt aIndex, TInt aPosition) const;
	TInt SetDeviceStringDescriptorTC(DThread* aThread, const TDes8& aString, TInt aIndex, TInt aPosition);
	TInt RemoveDeviceStringDescriptor(TInt aIndex, TInt aPosition);
	void ExchangeStringDescriptor(TInt aIndex, const TUsbcStringDescriptor* aDesc);
	TBool AnyStringDescriptors() const;
	TBool StringDescriptorExists(TInt aIndex) const;
	TInt FindAvailableStringPos() const;

private:
	// Data members
	RPointerArray<TUsbcDescriptorBase> iDescriptors;
	RPointerArray<TUsbcStringDescriptorBase> iStrings;
	TInt iIfcIdx;
	TUint8* const iEp0_TxBuf;								// points to the controller's ep0 TX buffer
	TBool iHighSpeed;										// true if currently operating at high-speed
	};


#endif	// __USBCDESC_H__
