// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\d32usbcsc.h
// User side class definitions for USB Device support.
// 
//

/**
 @file d32usbcsc.h
 @publishedPartner
 @released
*/

#ifndef __D32USBCSC_H__
#define __D32USBCSC_H__

#include <e32ver.h>
#include <usb.h>
#include <d32usbcshared.h>


/** This means that SetInterface was called after RealizeInterface had been called.
    RealizeInterface is meant to signal that all interfaces have been set.
*/
const TInt KErrUsbAlreadyRealized = -6813; //Changed from -6713 for future compatibility with the d32usbcshared.h file

const TInt KErrAlternateSettingChanged = -6814;


const TUint KChunkCellSize = 1; //1 32 bit integer
const TInt KEpDescPacketSizeOffset = 4;

/** The user side enpoint number of Endpoint Zero. */
const TInt KEp0Number = 0;

/* Used in TUsbcScHdrEndpointRecord to describe the endpoint type
*/

const TUint8 KUsbScHdrEpDirectionIn=1;
const TUint8 KUsbScHdrEpDirectionOut=2;
const TUint8 KUsbScHdrEpDirectionBiDir=3;


const TUint8 KUsbScHdrEpTypeControl = 0;
const TUint8 KUsbScHdrEpTypeIsochronous =1;
const TUint8 KUsbScHdrEpTypeBulk = 2;
const TUint8 KUsbScHdrEpTypeInterrupt = 3;
const TUint8 KUsbScHdrEpTypeUnknown = ~0;
/** Used in the the Shared Chunk Header, to represent an endpoint.
*/
class TUsbcScHdrEndpointRecord
	{
public:
	inline TUsbcScHdrEndpointRecord(TInt aBufferNo, TUint8 aType);
	inline TUint Direction() const;
	inline TUint Type() const;
public:
	TUint8 iBufferNo;
	TUint8 iType;
	TUint16 iReserved;
	};


// This is used to hold the geometry of the shared buffers
class TUsbcScBufferRecord
	{
	friend class DLddUsbcScChannel;

public:
	inline TUint Offset() const;
	inline TUint Size() const;
private:
	inline void Set(TUint aOffset, TUint aEndOffset);
	TUint iOffset;
	TUint iSize;
	};

struct SUsbcScBufferHeader
	{
	TInt iHead;		// Where the LDD will next write a transfer
	TInt iTail;		// Indicates the fist packet that user side is processing (anything prior is disposable)
	TInt iBilTail;  // This is not used by LDD at all, but just for the BIL's benifit.
	};


struct SUsbcScAlternateSetting
	{
	TUint16 iSetting;
	TUint16 iSequence;
	};

/** Endpoint pair information.  This can be used to control pairing of endpoint
for classes that require explicit pairing of endpoint, such as ADC.

This is currently not used.
*/

class TEndpointPairInfo
{
public:
	TEndpointPairInfo(TUint8 aType=0, TUint16 aPair=0, TUint8 aSpare=0);
public:
	TUint8 iType;
	TUint8 iSpare;
	TUint16 iPair;
}; 

/**
This is the buffer number reserved for use with endpoint zero with the  ReadDataNotify and WriteData methods.
*/
const TInt KUsbcScEndpointZero = -1;

/**
This flag is reserved in TUsbcscEndpointInfo::iFlags, to indicate that the client driver's client wishes reads to operate in a coupled fashion.
This behaviour is not currently supported.
*/
const TUint KUsbScCoupledRead = 0x1;

/**
This flag, used in parameter aFlag of WriteData, is used to indicate that a ZLP should be sent.
*/
const TUint KUsbcScWriteFlagsZlp = 0x0001;


// Bit fields used in iFlags of TUsbcScTransferHeader

const TUint KUsbcScShortPacket = 0x0001;
const TUint KUsbcScStateChange = 0x0004;

class TUsbcScTransferHeader
{
public:
#ifdef _DEBUG
	TUint iHashId;
	TUint iSequence;
#endif
	TUint iBytes;
	TUint iFlags;
	TUint iNext;
	TUint16 iAltSettingSeq;
	TInt16 iAltSetting;
	union
	{
		TUint  i[1]; // Extends
		TUint8 b[4]; // Extends
	} iData;
};

/** The desired endpoint capabilities used in RDevUsbcScClient::SetInterface().

This derived class has additional fields used in the shared chunk USB driver.
*/
class TUsbcScEndpointInfo: public TUsbcEndpointInfo
	{
public:
	TUsbcScEndpointInfo(TUint aType=KUsbEpTypeBulk, TUint aDir=KUsbEpDirOut, TInt aInterval=0, TInt aExtra=0,
												TUint aBufferSize=0, TUint aReadSize=0);


public:
	/** This indicates the requested size of the endpoint buffer and must be at
		least as large as iReadSize.
	*/
	TUint iBufferSize;

	/** This is the amount of data that the LDD reads from the bus before it sets
	    up another read and is normally intended to read many packets.
	*/
	TUint iReadSize;

	/** TEndpointPairInfo represents pairing information for isochronous endpoints.
		Should be zero in other cases.  If this specifies paired endpoints then
		iExtra (in the base class TUsbcEndpointInfo) also needs to be set to the
		class specific size for this endpoint descriptor (This is here only for future use).
	*/
	TEndpointPairInfo iPairing;
	
	/** The necessary alignment, in bytes, that needs to be applied to the transfer start points
	    of the data.  This is only observed on OUT endpoints.  Zero can be specified to indicate
		there are no class side requirements for alignment.   The alignment for OUT endpoints is
		which ever is greater of this field or the USB hardware requirements.
	*/
	TUint iAlignment;

	/** Flags to indicate how the endpoint should function. None currently defined,
	    but could be used to allow support for direct read.
	*/
	TUint iFlags;
	
	/** Reserved for future use. */
	TUint32 iReserved2[2];
	};

/** This must be filled in prior to a call to RDevUsbcClient::SetInterface().
*/
class TUsbcScInterfaceInfo
	{
public:
	TUsbcScInterfaceInfo(TInt aClass=0, TInt aSubClass=0, TInt aProtocol=0,
					   TDesC16* aString=NULL, TUint aTotalEndpoints=0);
public:
	/** The class, subclass and protocol that this interface supports. */
	TUsbcClassInfo iClass;
	/** The description string for the interface. Used to construct the interface string descriptor. */
	const TDesC16* iString;
	/** Total number of endpoints being requested (0-5). Endpoint 0 should not be included in this number. */
	TUint iTotalEndpointsUsed;
	/** Desired properties of the endpoints requested.
		Do NOT include endpoint 0.
		APIs use endpoint numbers that are offsets into this array.
	*/
	TUsbcScEndpointInfo iEndpointData[KMaxEndpointsPerClient];
	/** 32 flag bits used for specifying miscellaneous Interface features.
		Currently defined are:
		- KUsbcInterfaceInfo_NoEp0RequestsPlease = 0x00000001
	*/
	TUint32 iFeatureWord;
	};

/** Package buffer for a TUsbcInterfaceInfo object.

	@see TUsbcInterfaceInfo
*/
typedef TPckgBuf<TUsbcScInterfaceInfo> TUsbcScInterfaceInfoBuf;

struct TUsbcScChunkHdrOffs
	{
	TUint iBuffers;
	TUint iAltSettings;
	};


class TUsbcScChunkBuffersHeader  // Not instantiable
	{
	friend class DLddUsbcScChannel;
	friend class TRealizeInfo;

public:
	inline TUsbcScBufferRecord* Ep0Out() const;
	inline TUsbcScBufferRecord* Ep0In() const;
	inline TUsbcScBufferRecord* Buffers(TInt aBuffer) const;
	inline TInt NumberOfBuffers() const;

private:
	TUsbcScChunkBuffersHeader();
private:
	TInt iRecordSize;
	TInt  iNumOfBufs;
	TUint8 iBufferOffset[8]; // Extends
	};

struct TUsbcScChunkAltSettingHeader // Not instantiable
	{
	TInt iEpRecordSize;
	TInt iNumOfAltSettings;
	TUint iAltTableOffset[1]; // Extends
	};

class TEndpointBuffer;


/** The user side handle to the USB client driver.
*/
class RDevUsbcScClient : public RBusLogicalChannel
	{
public:
	/** @internalComponent */
	enum TVer
		{
		EMajorVersionNumber = 0,
		EMinorVersionNumber = 1,
		EBuildVersionNumber = KE32BuildVersionNumber
		};

// Bit pattern. s = Request/Control.  c = Cancel,  m = mode bits, B = Buffer number, R = request number.
//	scmm mmmm |  mmmm mmmm | mmBB BBBB |RRRR RRRR  

	enum TRequest
		{
		ERequestWriteData = 1,
		ERequestReadDataNotify = 2, 	
		ERequestAlternateDeviceStatusNotify = 3,
		ERequestReEnumerate = 4,
		ERequestEndpointStatusNotify = 5,
 		ERequestOtgFeaturesNotify = 6,
		ERequestMaxRequests, // 7

		ERequestCancel = 0x40000000,

		ERequestWriteDataCancel						= ERequestWriteData                   | ERequestCancel,
		ERequestReadDataNotifyCancel				= ERequestReadDataNotify              | ERequestCancel,
		ERequestAlternateDeviceStatusNotifyCancel 	= ERequestAlternateDeviceStatusNotify | ERequestCancel,
		ERequestReEnumerateCancel 					= ERequestReEnumerate                 | ERequestCancel,
		ERequestEndpointStatusNotifyCancel 			= ERequestEndpointStatusNotify        | ERequestCancel,
        ERequestOtgFeaturesNotifyCancel 			= ERequestOtgFeaturesNotify           | ERequestCancel
		};

	enum TControl
		{
		// Changing the order of these enums will break BC.
		EControlEndpointZeroRequestError,					// 00
		EControlEndpointCaps,
		EControlDeviceCaps,
		EControlGetAlternateSetting,
		EControlDeviceStatus,
		EControlEndpointStatus,
		EControlSetInterface,
		EControlReleaseInterface,
		EControlSendEp0StatusPacket,
		EControlHaltEndpoint,								// 09
		//
		EControlClearHaltEndpoint,							// 10
		EControlSetDeviceControl,
		EControlReleaseDeviceControl,
		EControlEndpointZeroMaxPacketSizes,
		EControlSetEndpointZeroMaxPacketSize,
		EControlGetDeviceDescriptor,
		EControlSetDeviceDescriptor,
		EControlGetDeviceDescriptorSize,
		EControlGetConfigurationDescriptor,
		EControlSetConfigurationDescriptor,					// 19
		//
		EControlGetConfigurationDescriptorSize,				// 20
		EControlGetInterfaceDescriptor,
		EControlSetInterfaceDescriptor,
		EControlGetInterfaceDescriptorSize,
		EControlGetEndpointDescriptor,
		EControlSetEndpointDescriptor,
		EControlGetEndpointDescriptorSize,
		EControlGetCSInterfaceDescriptor,
		EControlSetCSInterfaceDescriptor,
		EControlGetCSInterfaceDescriptorSize,				// 29
		//
		EControlGetCSEndpointDescriptor,					// 30
		EControlSetCSEndpointDescriptor,
		EControlGetCSEndpointDescriptorSize,
		EControlSignalRemoteWakeup,
		EControlGetStringDescriptorLangId,
		EControlSetStringDescriptorLangId,
		EControlGetManufacturerStringDescriptor,
		EControlSetManufacturerStringDescriptor,
		EControlRemoveManufacturerStringDescriptor,
		EControlGetProductStringDescriptor,					// 39
		//
		EControlSetProductStringDescriptor,					// 40
		EControlRemoveProductStringDescriptor,
		EControlGetSerialNumberStringDescriptor,
		EControlSetSerialNumberStringDescriptor,
		EControlRemoveSerialNumberStringDescriptor,
		EControlGetConfigurationStringDescriptor,
		EControlSetConfigurationStringDescriptor,
		EControlRemoveConfigurationStringDescriptor,
		EControlDeviceDisconnectFromHost,
		EControlDeviceConnectToHost,						// 49
		//
		EControlDevicePowerUpUdc,							// 50
		EControlDumpRegisters,
		EControlAllocateEndpointResource,
		EControlDeAllocateEndpointResource,
		EControlQueryEndpointResourceUse,
		EControlGetEndpointZeroMaxPacketSize,
		EControlGetDeviceQualifierDescriptor,
		EControlSetDeviceQualifierDescriptor,
		EControlGetOtherSpeedConfigurationDescriptor,
		EControlSetOtherSpeedConfigurationDescriptor,		// 59
		//
		EControlCurrentlyUsingHighSpeed,					// 60
		EControlSetStringDescriptor,
		EControlGetStringDescriptor,
		EControlRemoveStringDescriptor,
        EControlSetOtgDescriptor,
        EControlGetOtgDescriptor,
        EControlGetOtgFeatures, 
		EControlRealizeInterface,
		EControlStartNextInAlternateSetting
		};


 // const static TUint KFieldIdPos     = 0;
	const static TUint KFieldIdMask    = 0xFF;
	const static TUint KFieldBuffPos   = 8;
	const static TUint KFieldBuffMask  = 0x3F;
	const static TUint KFieldFlagsPos  = 14;
	const static TUint KFieldFlagsMask = 0xFFFF;


public:

#ifndef __KERNEL_MODE__


	/** Opens a channel.

		@param aUnit This should be 0 (zero).
		@param aShare if this channel can be used in another process.

		@return KErrNone if successful.
	*/
	inline TInt Open(TInt aUnit, TBool aShare=ETrue);

	/** Opens a channel which has created.

		@param aMsg client-server message contain the handle of this channel.
		@param aPos index of message slot that contain handle.
		@param aType ownership type of the handle.

		@return KErrNone if successful.
	*/

	inline TInt Open(RMessagePtr2 aMsg, TInt aIndex, TOwnerType aType=EOwnerProcess);

	inline TVersion VersionRequired() const;

	/** Stalls ep0 to signal command fault to the host.

		@return KErrNone if successful.
	*/
	inline TInt EndpointZeroRequestError();

	/** Retrieves the capabilities of all the endpoints on the device.

		Suggested use is as follows:

		@code
		TUsbcEndpointData data[KUsbcMaxEndpoints];
		TPtr8 dataPtr(reinterpret_cast<TUint8*>(data), sizeof(data), sizeof(data));
		ret = usbPort.EndpointCaps(dataPtr);
		@endcode

		@param aEpCapsBuf A descriptor encapsulating an array of TUsbcEndpointData.

		@return KErrNone if successful.
	*/
	inline TInt EndpointCaps(TDes8& aEpCapsBuf);

	/** Retrieves the capabilities of the USB device.

		@see TUsbDeviceCaps

		@param aDevCapsBuf A TUsbDeviceCaps object.

		@return KErrNone if successful.
	*/
	inline TInt DeviceCaps(TUsbDeviceCaps& aDevCapsBuf);

	/** Copies the current alternate setting for this interface into aInterfaceNumber
		For USB Interfaces whose main interface is active, this will be 0 (zero).

		@param aInterfaceNumber Current alternate setting for this interface is copied into this.

		@return KErrNone if successful.
	*/
	inline TInt GetAlternateSetting(TInt& aInterfaceNumber);

	/** Copies the current device status into aDeviceStatus.

		@param aDeviceStatus Current device status is copied into this.

		@return KErrNone if successful
	*/
	inline TInt DeviceStatus(TUsbcDeviceState& aDeviceStatus);

	/** Copies the current endpoint status into aEndpointStatus.

		@param aEndpoint Endpoint number valid for the current alternate setting.
		@param aEndpointStatus The current endpoint status, might be stalled, not stalled or unknown.

		@return KErrNone if successful.
	*/
	inline TInt EndpointStatus(TInt aEndpoint, TEndpointState& aEndpointStatus);


	/** Requests that a zero length status packet be sent to the host in response
		to a class or vendor specific ep0 SETUP packet.

		@return KErrNone if successful.
	*/
	inline TInt SendEp0StatusPacket();

	/** Stalls endpoint aEndpoint, usually to indicate an error condition with a previous command.
		The host will normally send a SET_FEATURE command on ep0 to acknowledge and clear the stall.

		@return KErrNone if successful.
	*/
	inline TInt HaltEndpoint(TInt aEndpoint);

	/** Clears the stall condition on endpoint aEndpoint. This is inluded for symmetry and test purposes.

		@return KErrNone if successful.
	*/
	inline TInt ClearHaltEndpoint(TInt aEndpoint);

	/** Requests that device control be allocated to this channel.

		@return KErrNone if successful.
	*/
	inline TInt SetDeviceControl();

	/** Relinquishes device control previously allocated to this channel.

		@return KErrNone if successful.
	*/
	inline TInt ReleaseDeviceControl();

	/** Returns a bitmap of available ep0 maximum packet sizes.

		@return bitmap of available ep0 maximum packet sizes.
	*/
	inline TUint EndpointZeroMaxPacketSizes();

	/** Requests that a maximum packet size of aMaxPacketSize be set on ep0.

		@param aMaxPacketSize The maximum packet size.

		@return KErrNone if successful.
	*/
	inline TInt SetEndpointZeroMaxPacketSize(TInt aMaxPacketSize);

	/** Queries the current maximum packet size on ep0.

		@return The currently set maximum packet size on ep0.
	*/
	inline TInt GetEndpointZeroMaxPacketSize();

	/** Copies the current device descriptor into aDeviceDescriptor.

		@param aDeviceDescriptor Receives the current device descriptor.

		@return KErrNone if successful.
	*/
	inline TInt GetDeviceDescriptor(TDes8& aDeviceDescriptor);

	/** Sets the contents of aDeviceDescriptor to be the current device descriptor.

		@param aDeviceDescriptor Contains the device descriptor.

		@return KErrNone if successful.
	*/
	inline TInt SetDeviceDescriptor(const TDesC8& aDeviceDescriptor);

	/** Gets the size of the current device descriptor. This is unlikely to be anything other than 9.

		@param aSize Receives the size of the current device descriptor.

		@return KErrNone if successful.
	*/
	inline TInt GetDeviceDescriptorSize(TInt& aSize);

	/** Copies the current configuration descriptor into aConfigurationDescriptor.

		@param aConfigurationDescriptor Receives the current configuration descriptor.

		@return KErrNone if successful.
	*/
	inline TInt GetConfigurationDescriptor(TDes8& aConfigurationDescriptor);

	/** Sets the contents of aConfigurationDescriptor to be the current configuration descriptor.

		@param aConfigurationDescriptor Contains the configuration descriptor.

		@return KErrNone if successful.
	*/
	inline TInt SetConfigurationDescriptor(const TDesC8& aConfigurationDescriptor);

	/** Gets the size of the current configuration descriptor.

		@param aSize Receives the size of the current configuration descriptor.

		@return KErrNone if successful.
	*/
	inline TInt GetConfigurationDescriptorSize(TInt& aSize);

	/** Copies the interface descriptor into aInterfaceDescriptor for the interface with alternate
		setting aSettingNumber, 0 for the main interface.

		@param aSettingNumber Alternate setting number on the interface, 0 for the main interface.
		@param aInterfaceDescriptor Receives the interface descriptor.

		@return KErrNone if successful.
	*/
	inline TInt GetInterfaceDescriptor(TInt aSettingNumber, TDes8& aInterfaceDescriptor);

	/** Sets the interface descriptor contained in aInterfaceDescriptor for the interface with
		alternate setting aSettingNumber, 0 for the main interface, for transmission to the host
		during enumeration.

		@param aSettingNumber Alternate setting number on the interface, 0 for the main interface.
		@param aInterfaceDescriptor Contains the interface descriptor to be set.

		@return KErrNone if successful.
	*/
	inline TInt SetInterfaceDescriptor(TInt aSettingNumber, const TDesC8& aInterfaceDescriptor);

	/** Copies the size of the interface descriptor for the interface with alternate
		setting aSettingNumber, 0 for the main interface, into aSize.

		@param aSettingNumber The alternate setting.
		@param aSize receives the size of the interface descriptor.

		@return KErrNone if successful.
	*/
	inline TInt GetInterfaceDescriptorSize(TInt aSettingNumber, TInt& aSize);

	/** Copies the endpoint descriptor for logical endpoint number aEndpointNumber into aEndpointDescriptor
		for the interface with alternate setting aSettingNumber, 0 for the main interface.

		@param aSettingNumber Alternate setting number on the interface, 0 for the main interface.
		@param aEndpointNumber The endpoint number of the endpoint.
		@param aEndpointDescriptor Receives the endpoint descriptor.

		@return KErrNone if successful.
	*/
	inline TInt GetEndpointDescriptor(TInt aSettingNumber, TInt aEndpointNumber, TDes8& aEndpointDescriptor);

	/** Sets the endpoint descriptor for logical endpoint number aEndpointNumber contained in
		aEndpointDescriptor for the interface with alternate setting aSettingNumber, 0 for the main interface,
		for transmission to the host during enumeration.

		@param aSettingNumber Alternate setting number on the interface, 0 for the main interface.
		@param aEndpointNumber Valid endpoint number on this interface.
		@param aEndpointDescriptor Contains the endpoint descriptor to be set.

		@return KErrNone if successful.
	*/
	inline TInt SetEndpointDescriptor(TInt aSettingNumber, TInt aEndpointNumber,
									  const TDesC8& aEndpointDescriptor);

	/** Copies the size of the endpoint descriptor for logical endpoint number aEndpointNumber for the
		interface with alternate setting aSettingNumber, 0 for the main interface, into aSize.

		@param aSettingNumber Alternate setting number on the interface, 0 for the main interface.
		@param aEndpointNumber Valid endpoint number on this interface.
		@param aSize Receives the size of the endpoint descriptor.

		@return KErrNone if successful.
	*/
	inline TInt GetEndpointDescriptorSize(TInt aSettingNumber, TInt aEndpointNumber, TInt& aSize);

    /** Get OTG descriptor size

		@param aSize TInt Reference which contains OTG descriptor size on return.
    */
    inline void GetOtgDescriptorSize(TInt& aSize);

    /** Get OTG descriptor of USB on-the-go feature.

		@param aOtgDesc User-side buffer to store copy of descriptor.

		@return KErrNone if successful.
    */
    inline TInt GetOtgDescriptor(TDes8& aOtgDesc);

    /** Set OTG descriptor by user to enable/disable USB on-the-go feature.

		@param aOtgDesc Descriptor buffer containing OTG features.

		@return KErrNone if successful.
    */
    inline TInt SetOtgDescriptor(const TDesC8& aOtgDesc);

	/** Copies the current device_qualifier descriptor into aDescriptor.

		@param aDescriptor Receives the current device_qualifier descriptor.

		@return KErrNone if successful.
	*/
	inline TInt GetDeviceQualifierDescriptor(TDes8& aDescriptor);

	/** Sets the device_qualifier descriptor to the contents of aDescriptor.

		@param aDescriptor Contains the new device_qualifier descriptor.

		@return KErrNone if successful.
	*/
	inline TInt SetDeviceQualifierDescriptor(const TDesC8& aDescriptor);

	/** Copies the current other_speed_configuration descriptor into aDescriptor.

		@param aDescriptor Receives the current other_speed_configuration descriptor.

		@return KErrNone if successful.
	*/
	inline TInt GetOtherSpeedConfigurationDescriptor(TDes8& aDescriptor);

	/** Sets the other_speed_configuration descriptor to the contents of aDescriptor.

		@param aDescriptor Contains the new other_speed_configuration descriptor.

		@return KErrNone if successful.
	*/
	inline TInt SetOtherSpeedConfigurationDescriptor(const TDesC8& aDescriptor);

	/** Copies the class specific interface descriptor block into aInterfaceDescriptor for the interface
		with alternate setting aSettingNumber, 0 for the main interface.

		@param aSettingNumber Alternate setting number on the interface, 0 for the main interface.
		@param aInterfaceDescriptor Contains the interface descriptor to be set.

		@return KErrNone if successful.
	*/
	inline TInt GetCSInterfaceDescriptorBlock(TInt aSettingNumber, TDes8& aInterfaceDescriptor);

	/** aSettingNumber is the alternate interface setting, 0 for the main interface, that the descriptor block
		aDes should be attached to. aDes is a block of data containing at least one class specific descriptor
		for transmission during enumeration after the class interface descriptor (or alternate interface
		descriptor) has been sent, but before the endpoint descriptors belonging to this interface are sent.
		aDes may contain as many descriptors as are necessary or only one. SetCSInterfaceDescriptorBlock()
		should be called at any time after SetInterface() has been called to establish a main interface or an
		alternate interface. More than one call may be made - the data blocks will be concatenated prior to
		sending. No checking or validation of the contents of aDes will be made and it is the caller's
		responsibility to ensure that the data supplied is correct and appropriate to the interface identified
		by aSettingNumber.

		@param aSettingNumber Alternate setting number on the interface, 0 for the main interface.
		@param aInterfaceDescriptor Contains the interface descriptor to be set.

		@return KErrNone if successful.
	*/
	inline TInt SetCSInterfaceDescriptorBlock(TInt aSettingNumber, const TDesC8& aInterfaceDescriptor);

	/** Copies the size of the class specific interface descriptor block for the interface with alternate
		setting aSettingNumber, 0 for the main interface, into aSize.

		@param aSettingNumber The alternate setting number.
		@param aSize receives the size of the interface descriptor.

		@return KErrNone if successful.
	*/
	inline TInt GetCSInterfaceDescriptorBlockSize(TInt aSettingNumber, TInt& aSize);

	/** Copies the class specific endpoint descriptor for logical endpoint number aEndpointNumber
		into aEndpointDescriptor for the interface with alternate setting aSettingNumber, 0 for the main
		interface.

		@param aSettingNumber Alternate setting number on the interface, 0 for the main interface.
		@param aEndpointNumber Valid endpoint number on this interface.
		@param aEndpointDescriptor Receives the endpoint descriptor.

		@return KErrNone if successful.
	*/
	inline TInt GetCSEndpointDescriptorBlock(TInt aSettingNumber, TInt aEndpointNumber,
											 TDes8& aEndpointDescriptor);

	/** Sets the class specific endpoint descriptor for logical endpoint number aEndpointNumber contained in
		aEndpointDescriptor for the interface with alternate setting aSettingNumber, 0 for the main interface,
		for transmission to the host during enumeration.

		@param aSettingNumber Alternate setting number on the interface, 0 for the main interface.
		@param aEndpointNumber Valid endpoint number on this interface.
		@param aEndpointDescriptor Contains the endpoint descriptor to be set.

		@return KErrNone if successful.
	*/
	inline TInt SetCSEndpointDescriptorBlock(TInt aSettingNumber, TInt aEndpointNumber,
											 const TDesC8& aEndpointDescriptor);

	/** Copies the size of the class specific endpoint descriptor block for logical endpoint number
		aEndpointNumber for the interface with alternate setting aSettingNumber, 0 for the main interface,
		into aSize.

		@param aSettingNumber Alternate setting number on the interface, 0 for the main interface.
		@param aEndpointNumber Valid endpoint number on this interface.
		@param aSize On return, contains the size of the class specific endpoint descriptor block.

		@return KErrNone if successful.
	*/
	inline TInt GetCSEndpointDescriptorBlockSize(TInt aSettingNumber, TInt aEndpointNumber, TInt& aSize);

	/** Generates a Remote Wakeup bus condition.
		The capability of the device to generate Remote Wakeup signalling is enquired in
		RDevUsbcClient::DeviceCaps.

		@return KErrNone if this signalling is possible and the signal has been generated.
	*/
	inline TInt SignalRemoteWakeup();

	/** Simulates a physical removal of the USB cable by disabling the D+/- pull-ups.The iConnect member of
		TUsbDeviceCapsV01, returned by RDevUsbcClient::DeviceCaps(), indicates whether this functionality is
		supported.

		@return KErrNone if successful.
	*/
	inline TInt DeviceDisconnectFromHost();

	/** Simulates a physical insertion of the USB cable by enabling the D+/- pull-ups.The iConnect member
		of TUsbDeviceCapsV01, returned by RDevUsbcClient::DeviceCaps(),  indicates whether this functionality
		is supported.

		@return KErrNone if successful.
	*/
	inline TInt DeviceConnectToHost();

	/** Powers up the UDC and connects it to the bus if one or more interfaces exist.

		@return KErrNone if UDC successfully powered up, KErrNotReady if no
		interfaces have been registered yet, KErrHardwareNotAvailable if UDC
		couldn't be activated.
	*/
	inline TInt PowerUpUdc();

	/** Enquires about the current operating speed of the UDC.

		@return ETrue if the UDC is currently operating at High speed, EFalse otherwise.
	*/
	inline TBool CurrentlyUsingHighSpeed();

	/** Allocates the use of aResource to aEndpoint. It will be used from when the current bus transfer	has been
		completed.

		@param aResource is typically some rationed hardware resource or possibly specifies a type of endpoint
		behaviour. aResource is not a bitmap and TEndpointResource values should not be combined.
		@param aEndpoint The endpoint number to which the resource is to be allocated.

		@return KErrNone if successful, KErrInUse if the resource is already consumed and cannot be allocated,
		KErrNotSupported if the endpoint does not support the resource requested.

		@publishedPartner @deprecated
	*/
	inline TInt AllocateEndpointResource(TInt aEndpoint, TUsbcEndpointResource aResource);

	/** Deallocates the use of aResource aEndpoint or ends a specified endpoint behaviour.

		@param aResource is typically some rationed hardware resource or possibly specifies a type of endpoint
		behaviour. aResource is not a bitmap and TEndpointResource values should not be combined.
		@param aEndpoint The endpoint number from which the resource is to be removed.

		@return KErrNone if the resource has been successfully deallocated, KErrNotSupported if the endpoint
		does not support the resource requested.

		@publishedPartner @deprecated
	*/
	inline TInt DeAllocateEndpointResource(TInt aEndpoint, TUsbcEndpointResource aResource);

	/** Queries endpoint resource use.

		@param aResource is typically some rationed hardware resource or possibly specifies a type of endpoint
		behaviour. aResource is not a bitmap and TEndpointResource values should not be combined.
		@param aEndpoint The endpoint number at which the resource is to be queried.

		@return ETrue is the specified resource is in use at the endpoint and EFalse if not.
	*/
	inline TBool QueryEndpointResourceUse(TInt aEndpoint, TUsbcEndpointResource aResource);

	/** Request (i.e. claim for this channel) up to five endpoints and set the class type for this USB
		interface. 'aInterfaceData' is a package buffer which describes the interface and all the endpoints
		being requested by the driver for this interface.

		@param aInterfaceNumber Distinguishes between alternate interfaces. If these are not be used then this
		should always be zero. If this parameter is used, then its value must be one more than that of the
		proceeding alternate interface.
		@param aInterfaceData A package buffer which describes the interface and all the endpoints being
		requested by the driver for this interface.


		@return KErrInUse if any of the endpoints being requested have already been claimed by another channel.
		KErrNotSupported if an endpoint with all of the specified properties is not supported on this
		platform. KErrNoMemory if insufficient memory is available to complete the operation.
	*/
	inline TInt SetInterface(TInt aInterfaceNumber, TUsbcScInterfaceInfoBuf& aInterfaceData);


	/**
		This method should be called after SetInterface has been called for all possible alternative settings.
		Calling this invalidates further calls to SetInterface. On success, a chunk handle is created and
		passed back though aChunk.   This is needed for the user side to access the shared chunk where the
		data is stored.  Note that if you are using the BIL (described later), then FinalizeInterface (...)
		should be used instead, which will call this method.
		
		@return KErrNone on successful completion, or one of the system wide error codes.
	*/
	inline TInt RealizeInterface(RChunk& aChunk);


	/** Release an interface previously claimed by this channel. Alternate interfaces need to be released
		in strict descending order, starting with the last (i.e. highest numbered) one.
		It is not necessary to release an interface that wasn't successfully requested.

		@param aInterfaceNumber Specifies the alternate setting number 'aInterfaceNum' of the interface to be
		released.

		@return KErrNone if successful. KErrArgument if the alternate setting doesn't exist or is released out
		of order.
	*/
	inline TInt ReleaseInterface(TInt aInterfaceNumber);

	/** Copies the current string descriptor language ID (LANGID) code into the aLangId argument. Even though
		the USB spec allows for the existence of a whole array of LANGID codes, we only support one.

		@param aLangId Receives the LANGID code.

		@return KErrNone if successful, KErrArgument if problem with argument (memory cannot be written to, etc.).
	*/
	inline TInt GetStringDescriptorLangId(TUint16& aLangId);

	/** Sets the string descriptor language ID (LANGID). Even though the USB spec allows for the existence of
		a whole array of LANGID codes, we only support one.

		@param aLangId The LANGID code to be set.

		@return KErrNone if successful.
	*/
	inline TInt SetStringDescriptorLangId(TUint16 aLangId);

	/** Copies the string descriptor identified by the iManufacturer index field of the Standard Device
		Descriptor into the aString argument.

		@param aString Receives manufacturer string.

		@return KErrNone if successful, KErrArgument if MaxLength of aString is too small to hold the entire
		descriptor, KErrNotFound if the string descriptor couldn't be found.
	*/
	inline TInt GetManufacturerStringDescriptor(TDes16& aString);

	/** Sets the string descriptor identified by the iManufacturer index field of the Standard Device
		Descriptor to the aString argument.

		@param aString Contains the new manufacturer string descriptor.

		@return KErrNone if successful, KErrNoMemory if no memory is available to store the new string from
		aString (in which case the old string descriptor will be preserved).
	*/
	inline TInt SetManufacturerStringDescriptor(const TDesC16& aString);

	/** Removes (deletes) the string descriptor identified by the iManufacturer index field of the Standard
		Device Descriptor and sets that field to zero.

		@return KErrNone if successful, KErrNotFound if the string descriptor couldn't be found.
	*/
	inline TInt RemoveManufacturerStringDescriptor();

	/** Retrieves the string descriptor identified by the iProduct index field of the Standard Device
		Descriptor into the aString argument.

		@param aString Receives product string.

		@return KErrNone if successful, KErrArgument if MaxLength of aString is too small to hold the entire
		descriptor, KErrNotFound if the string descriptor couldn't be found.
	*/
	inline TInt GetProductStringDescriptor(TDes16& aString);

	/** Sets the string descriptor identified by the iProduct index field of the Standard Device Descriptor to
		the aString argument.

		@param aString Contains the new product string descriptor.

		@return KErrNone if successful, KErrNoMemory if no memory is available to store the new string from
		aString (in which case the old string descriptor will be preserved).
	*/
	inline TInt SetProductStringDescriptor(const TDesC16& aString);

	/** Removes (deletes) the string descriptor identified by the iProduct index field of the Standard Device
		Descriptor and sets that field to zero.

		@return KErrNone if successful, KErrNotFound if the string descriptor couldn't be found.
	*/
	inline TInt RemoveProductStringDescriptor();

	/** Retrieves the string descriptor identified by the iSerialNumber index field of the Standard Device
		Descriptor into the aString argument.

		@param aString Receives product string.

		@return KErrNone if successful, KErrArgument if MaxLength of aString is too small to hold the entire
		descriptor, KErrNotFound if the string descriptor couldn't be found.
	*/
	inline TInt GetSerialNumberStringDescriptor(TDes16& aString);

	/** Sets the string descriptor identified by the iSerialNumber index field of the Standard Device
		Descriptor to the aString argument.

		@param aString Contains the new serial number string descriptor.

		@return KErrNone if successful, KErrNoMemory if no memory is available to store the new string from
		aString (in which case the old string descriptor will be preserved).
	*/
	inline TInt SetSerialNumberStringDescriptor(const TDesC16& aString);

	/** Removes (deletes) the string descriptor identified by the iSerialNumber index field of the Standard
		Device Descriptor and sets that field to zero.

		@return KErrNone if successful, KErrNotFound if the string descriptor couldn't be found.
	*/
	inline TInt RemoveSerialNumberStringDescriptor();

	/** Retrieves the string descriptor identified by the iConfiguration index field of the (first) Standard
		Configuration Descriptor into the aString argument.

		@param aString Receives configuration string.

		@return KErrNone if successful, KErrArgument if MaxLength of aString is too small to hold the entire
		descriptor, KErrNotFound if the string descriptor couldn't be found.
	*/
	inline TInt GetConfigurationStringDescriptor(TDes16& aString);

	/** Sets the string descriptor identified by the iConfiguration index field of the Standard Configuration
		Descriptor to the aString argument.

		@param aString Contains the new serial number string descriptor.

		@return KErrNone if successful, KErrNoMemory if no memory is available to store the new string from
		aString (in which case the old string descriptor will be preserved).
	*/
	inline TInt SetConfigurationStringDescriptor(const TDesC16& aString);

	/** Removes (deletes) the string descriptor identified by the iConfiguration index field of the Standard
		Configuration Descriptor and sets that field to zero.

		@return KErrNone if successful, KErrNotFound if the string descriptor couldn't be found.
	*/
	inline TInt RemoveConfigurationStringDescriptor();

	/** Copies the string of the USB string descriptor at the specified index in the string descriptor array
		into the aString argument.

		Although this function can also be used for it, for querying most standard string descriptors
		there exists a set of dedicated access functions.

		@see RDevUsbcClient::GetStringDescriptorLangId
		@see RDevUsbcClient::GetManufacturerStringDescriptor
		@see RDevUsbcClient::GetProductStringDescriptor
		@see RDevUsbcClient::GetSerialNumberStringDescriptor
		@see RDevUsbcClient::GetConfigurationStringDescriptor

		@param aIndex The position of the string descriptor in the string descriptor array.
		@param aString The target location for the string descriptor copy.

		@return KErrNone if successful, KErrNotFound if no string descriptor exists at the specified index,
		KErrArgument if MaxLength() of aString is too small to hold the entire descriptor.
	*/
	inline TInt GetStringDescriptor(TUint8 aIndex, TDes16& aString);

	/** Sets the aString argument to be the string of a USB string descriptor at the specified index in the
		string descriptor array. If a string descriptor already exists at that position then its string will
		be replaced.

		Care should be taken, when choosing aIndex, not to inadvertently overwrite one of the standard
		string descriptors.	For their manipulation there exists a set of dedicated access functions.

		@see RDevUsbcClient::SetStringDescriptorLangId
		@see RDevUsbcClient::SetManufacturerStringDescriptor
		@see RDevUsbcClient::SetProductStringDescriptor
		@see RDevUsbcClient::SetSerialNumberStringDescriptor
		@see RDevUsbcClient::SetConfigurationStringDescriptor

		@param aIndex The position of the string descriptor in the string descriptor array.
		@param aString Contains the string descriptor to be set.

		@return KErrNone if successful, KErrArgument if aIndex is invalid, KErrNoMemory if no memory
		is available to store the new string (an existing descriptor at that index will be preserved).
	*/
	inline TInt SetStringDescriptor(TUint8 aIndex, const TDesC16& aString);

	/** Removes (deletes) the USB string descriptor at the specified index in the string descriptor array.
		The position in the array of other string descriptors is not affected.

		Care should be taken, when choosing aIndex, not to inadvertently delete a standard string descriptor
		(also because index references from non-string descriptors would be invalidated). For the deletion
		of most standard string descriptors there exists a set of dedicated functions.

		@see RDevUsbcClient::RemoveManufacturerStringDescriptor
		@see RDevUsbcClient::RemoveProductStringDescriptor
		@see RDevUsbcClient::RemoveSerialNumberStringDescriptor
		@see RDevUsbcClient::RemoveConfigurationStringDescriptor

		@param aIndex The position of the string descriptor in the string descriptor array.

		@return KErrNone if successful, KErrNotFound if no string descriptor exists at the specified index.
	*/
	inline TInt RemoveStringDescriptor(TUint8 aIndex);



	/**  Requests notification for when there is data available on the buffer indicated.  If the buffer
	     already has data in it, it will return immediately, otherwise it will block until there is.

	If the BIL methods are being used (recommended), then this method should not be called directly,
	using TEndpointBuffer::GetBuffer instead.

	@param aBufferNumber Indicates the buffer for which the caller wishes to know about data 
	additions.  The buffer needed of any given endpoint can be found by inspecting the alternative
	setting table, in the chunk header.  The location of the buffer can be found by looking at the
	buffer offset table, also in the chunk header. 

	@param aStatus The request status where notification of completion is directed. KErrCancel is
	returned if the asynchronous operation was cancelled.

	@param aLength A preference for the quantity of data to be read.  This value is only a 
	suggestion and my be ignored.  The default value of 0 indicates no preference.

	@return KErrNone on success, or KErrArgument if the buffer number is invalid.    
	*/
	inline TInt ReadDataNotify(TInt aBufferNumber, TRequestStatus& aStatus, TInt aLength=0);


	/**  Requests the LDD to write the contents of the given buffer to the USB hardware.  Notification is
	given when this is complete.  More then one write request can be queued on any one endpoint, to allow
	for less Hardware idling between buffers.

	If the BIL methods are being used (recommended), then this method should not be called directly,
	using TEndpointBuffer::WriteBuffer instead.

	@param aBufferNumber represents the buffer number of the buffer of which the caller has placed the
	data. As described with ReadDataNotify(...), details of the buffers can be found in the chunk header.

	@param aStart Represents the start offset of the data within the chunk.  This start location must be
	aligned to a multiple of the maximum packet size for the endpoint, so that the data can be DMAed
	straight out of the buffer.

	@param aLength Represents the amount of data to be sent to the host in bytes.

	@param aFlags Is a bitfield, where bit 0 should be set if a ZLP is to be sent to the host after the
	current transaction.  All other bits are reserved for future use.
*/
	inline void WriteData(TInt aBufferNumber, TUint aStart, TUint aLength, TUint aFlags, TRequestStatus& aStatus);



	/** Cancels an outstanding read request on endpoint buffer aBufferNumber.

		@param aBufferNumber The endpoint buffer number whose read is to be cancelled.
	*/
	inline void ReadCancel(TInt aBufferNumber);


	/** Cancels an outstanding write request on endpoint buffer aBufferNumber.

		@param aBufferNumber The endpoint buffer number whose write is to be cancelled.
	*/
	inline void WriteCancel(TInt aBufferNumber);

	/** Cancels any transfer on any endpoint buffer specified in aBufferMask.

		@code
		// Cancel transfer requests on buffers 1, 2, 3 & 4
		usbPort.EndpointTransferCancel(1 | 2 | 4 | 8);
		@endcode

		@param aBufferMask bitmap of the endpoint buffer numbers.
	*/
	inline void EndpointTransferCancel(TUint aBufferMask);

	/**	Register for notification when a change of the Interface alternate setting or the USB Controller's
		current state occurs. When the alternate setting or the Controller state changes, then the
		asynchronous function completes and the current alternate setting number or Controller state is
		written back to aValue. If the KUsbAlternateSetting bit is set then the remaining bits are the
		alternate setting number. Otherwise aValue is interpreted as a TUsbcDeviceState.

		@see TUsbcDeviceState

		@param aStatus The request status.
		@param aValue Receives the alternate setting number or Controller state.
	*/
	inline void AlternateDeviceStatusNotify(TRequestStatus& aStatus, TUint& aValue);

	/** Completes an AlternateDeviceStatusNotify request. If a request has previously been made then the
		status variable is updated with the current device state.
	*/
	inline void AlternateDeviceStatusNotifyCancel();

	/** If the channel has changed the grouping of endpoints between interfaces or changed the interface class
		type from the defaults then it is necessary to force a re-enumeration. This will typically involve the
		Symbian OS device initiating a disconnection and re-connection. This is an asynchronous operation
		which will complete when the Controller is successfully configured by the host, i.e. has achieved
		EUsbcDeviceStateConfigured.  Since it is not known if the operation has failed, at the same time that
		a ReEnumerate request is made, a timer should be set up to complete after approximately 5 seconds. It
		can be assumed that if the operation has not completed after this time interval then it will not
		complete.

		@param aStatus The request status.
	*/
	inline void ReEnumerate(TRequestStatus& aStatus);

	/** Cancels an outstanding ReEnumerate() request.
	*/
	inline void ReEnumerateCancel();

	/**	Register for notification when a change in stall status of any of the interface's endpoints occurs,
		but not ep0. When a change in stall status occurs, then the asynchronous function completes and the
		current stall state is written back to 'aEndpointStatus' as a bit map: Only stall state changes caused
		by SET_FEATURE and CLEAR_FEATURE standard commands on ep0 will be notified when this function
		completes. After this request completes the request should be re-issued to obtain future
		notifications.

		@param aStatus The request status.
		@param aEndpointMask A bitmap of the endpoints' stall status. This is filled in when the call completes.
		bit 1 represents the interface's virtual endpoint 1, (KUsbcEndpoint1Bit)
		bit 2 represents the interface's virtual endpoint 2, (KUsbcEndpoint2Bit) etc.
		bit value 0 - not stalled,
		bit value 1 - stalled.
	*/
	inline void EndpointStatusNotify(TRequestStatus& aStatus, TUint& aEndpointMask);

	/** Completes an endpoint status notify request.
	*/
 	inline void EndpointStatusNotifyCancel();

    /** Get current on-the-go features relating to the ability of device/host pair to
        perform OTG role swap.

        @param aFeatures On return it contains features the device currently has.
                bit 2 represents b_hnp_enable,       (KUsbOtgAttr_B_HnpEnable)
                bit 3 represents a_hnp_support,      (KUsbOtgAttr_A_HnpSupport)
                bit 4 represents a_alt_hnp_support,  (KUsbOtgAttr_A_AltHnpSupport)
        @return KErrNone if successful, KErrNotSupported if OTG is not supported by
                this device, otherwise system-wide error returns.
    */
    inline TInt GetOtgFeatures(TUint8& aFeatures);

    /** Register for notification on USB on-the-go features' change. If any OTG feature
        is changed, request completes and current feature value is filled in aValue.

        @param aStatus Request status object.
        @param aValue On request completion, contains current OTG feature value.
    */
    inline void OtgFeaturesNotify(TRequestStatus& aStatus, TUint8& aValue);

    /** Cancel pending OTG feature request.
    */
    inline void OtgFeaturesNotifyCancel();

	/**	This function retrieves the alternate setting that the WriteData function can
		write to.  After a host sets the alternate setting, writes to the IN endpoint
		are not permitted by the LDD until this method has been called.
		This function is not asynchronous nor blocking, and should not be used to
		detect that an alternate setting has happened.

		If the BIL methods are being used (recommended), then this method should not be called directly. 

		@return The alternative setting number or KErrInUse if the current alternative
	 	setting is already in use, that is to say that the alternative setting has not changed.
	*/
	inline TInt StartNextInAlternateSetting();


	/*******************************\
	*  Buffer Interface Layer (BIL) *
	\*******************************/

	// This following functions, as well as the ones in TEndpointBuffer (below), 
	// can be considered the BIL.


	/**
	Finalize the interface, creating a chunk for use with reading/writing to endpoints.
	FinalizeInterface should be called after all alternate interfaces have been set up with SetInteface.
	Any attempt to call SetInterface after this stage will fail.

	@return		KErrNone if operation is successfull
				System wide error codes if chunk creation failed
	*/
	IMPORT_C TInt FinalizeInterface();

	/**
	Finalize the interface, creating a chunk for use with reading/writing to endpoints. This 
	version of the method provides a handle to the chunk, which is needed if the
	buffer is to be passed and used by other processes. 
	FinalizeInterface should be called after all alternate interfaces have been set up with SetInteface.
	Any attempt to call SetInterface after this stage will fail.

	@param	aChunk	On success aChunk points to the created chunk.
	@return			KErrNone if operation is successfull
					System wide error codes if chunk creation failed
	*/
	IMPORT_C TInt FinalizeInterface(RChunk*& aChunk);

	/**
	Opens an endpoint, an endpoint should be opened before any operations are attempted on it.

	@param	aEpB	On success aEpB will be filled with the relevant details for that endpoint	
	@param	aEpI	endpoint number to be opened
	@return			KErrNone if operation is successfull
					KErrNotFound if endpoint number is not valid for current alternate setting
					KErrInUse if endpoint is already opened
					KErrArgument if endpoint buffer argument passed is already in existence and being used
	*/
	IMPORT_C TInt OpenEndpoint(TEndpointBuffer & aEpB, TInt aEpI);

	/**
	Switches to processing from one Alternate setting to the next. All open endpoints (except EP0) must
	be close before this can be called.

	@param	aFlush	If ETrue, the method will purge the buffers of any data unread for the old setting.
					If each endpoint was not read up until KErrEof was reached, then this should be set.
					 
	@return		the alternate Setting if operation is successful
				KErrInUse if any endpoints in present alternate setting is still open (except Ep0)
				KErrNotReady if there is no change in alternate setting
				KErrInUse if StartNextInAlternateSetting detects no change in alternate setting.
				KErrCorrupt if the buffer structure becomes corrupt.
	*/
	IMPORT_C TInt StartNextOutAlternateSetting(TBool aFlush);

	/**
	Sets aChunk to RChunk currently in use by BIL.

	@param	aChunk	aChunk will point to RChunk currently in use by BIL
	@return KErrNone on success otherwise a system wide error code, if an error has occurred.
	*/
	IMPORT_C TInt GetDataTransferChunk(RChunk*& aChunk);
	/**
	Call this function to reset alternate setting related data to initial state,
	this API should be called when device state goes to undefined.
	*/
	IMPORT_C void ResetAltSetting();

	

private:
 	/** @internalTechnology */
	TInt Empty(TUint aBufferOffset);
	/** @internalTechnology */
	TInt Drain(TUint aBuffer);
	/** @internalTechnology */ 
	TInt Peek(TUint aBuffer);
	/** @internalTechnology */ 
	TInt FindNextAlternateSetting();

private:
	TUint8 iEndpointStatus;	/** @internalTechnology Each bit corresponds to each endpoint's open/close status. */
	RChunk iSharedChunk; 	/** @internalTechnology The shared chunk in use. */
	TInt iAltSettingSeq;	/** @internalTechnology Used to track alternate setting changes. */
	TInt iAlternateSetting; /** @internalTechnology The alternate setting used by OUT endpoints, which may lag that of IN endpoints. */
	TInt iNewAltSetting; 	/** @internalTechnology Used to track the next alternate setting change on OUT endpoints,
								during transition from one to the next. */ 
	TInt iInAltSetting; 	/** @internalTechnology The alternate setting used by IN endpoints, which may be ahead of OUT endpoints. */


	friend class TEndpointBuffer;	
#endif // #ifndef __KERNEL_MODE__
	};

#ifndef __KERNEL_MODE__


/**
 This class forms part of the Buffer Interface Layer (BIL), which forms the 
 user side component of the USB Shared Chunk Client.  Objects of this type
 represent the shared chunk buffer, for a given endpoint.
 The method RDevUsbcScClient::OpenEndpoint() should be used to initialise
 objects of this type.
*/
class TEndpointBuffer
	{
public:
 
	/**
	This return value used by GetBuffer indicates that the item pointed to by 
	aBuffer/aOffset represents a state change, instead of endpoint data.
	*/
	const static TInt KStateChange = 0x01;

public:
	IMPORT_C TEndpointBuffer();

	/**
	Read the next block of data from the Shared chunk buffer. This method should be used if the user wishes to process one block of data at a time. 
	This method also expires the previously read block, meaning that the memory used by the block of data may be re-used by the system, overwriting it
	with new data.
	@param	aBuffer	aBuffer will point to data location in shared chunk	
	@param	aSize	aSize will hold the number of valid bytes that can be read
	@param	aZLP	aZLP will indicate whether its a short packet or not
	@param	aStatus	In case of no data available to be read, aStatus will be passed on to LDD, and user side should wait for 
					asynchronous call ReadDataNotify to return
	@param	aLength	Not used at the moment
	@return			KErrCompletion if operation is successfull and data is available in the buffer
					KErrNone if no data is available to be read
					KErrEof if alternate setting has changed
					KErrAccessDenied if endpoint is not opened
					KErrNotSupported if its an IN endpoint
					TEndpointBuffer::KStateChange if the returned data represents a state change (Ep0 only)
	*/
	IMPORT_C TInt GetBuffer(TAny*& aBuffer,TUint& aSize,TBool& aZLP,TRequestStatus& aStatus,TUint aLength=0);

	/**
	Read the next block of data from the Shared chunk buffer. This method should be used if the user wishes to process one block of data at a time. 
	This method also expires the previously read block, meaning that the memory used by the block of data may be re-used by the system, overwriting it
	with new data. 
	@param	aOffset	aOffset will point to data offset in shared chunk	
	@param	aSize	aSize will hold the number of valid bytes that can be read
	@param	aZLP	aZLP will indicate whether its a short packet or not
	@param	aStatus	In case of no data available to be read, aStatus will be passed on to LDD, and user side should wait for 
			asynchronous call ReadDataNotify to return
	@param	aLength	Not used at the moment
	@return	KErrCompletion if operation is successfull and data is available in the buffer
			KErrNone if no data is available to be read
			KErrEof if alternate setting has changed
			KErrAccessDenied if endpoint is not opened
			KErrNotSupported if its an IN endpoint
			TEndpointBuffer::KStateChange if the returned data represents a state change (Ep0 only)
	*/
	inline   TInt GetBuffer(TUint& aOffset,TUint& aSize,TBool& aZLP,TRequestStatus& aStatus,TUint aLength=0);

	/**
	Read the next block of data from the Shared chunk buffer. This method should be used if the user wishes to process more than one block of data
	simultaneously. The user must call one of the Expire() methods to free the memory used by the block of data, and make it available for new data.
	@param	aBuffer	aBuffer will point to data location in shared chunk	
	@param	aSize	aSize will hold the number of valid bytes that can be read
	@param	aZLP	aZLP will indicate whether its a short packet or not
	@param	aStatus	In case of no data available to be read, aStatus will be passed on to LDD, and user side should wait for 
					asynchronous call ReadDataNotify to return
	@param	aLength	Not used at the moment
	@return			KErrCompletion if operation is successfull and data is available in the buffer
					KErrNone if no data is available to be read
					KErrEof if alternate setting has changed
					KErrAccessDenied if endpoint is not opened
					KErrNotSupported if its an IN endpoint
					TEndpointBuffer::KStateChange if the returned data represents a state change (Ep0 only)
	*/
	IMPORT_C TInt TakeBuffer(TAny*& aBuffer,TUint& aSize,TBool& aZLP,TRequestStatus& aStatus,TUint aLength=0);

	/**
	Used in conjunction with TakeBuffer method. This will make the 'oldest' block of data previously read out using the TakeBuffer method, but not
	already	expired, to be released back to the system. This block can then be overwritten with new data, when it becomes available.
	@return 		KErrNotSupported if its an IN endpoint
					KErrNone if iTail is successfully bumped to the next transfer to be read
	*/

	IMPORT_C TInt Expire();

	/**
	Used in conjunction with TakeBuffer method. This function allows blocks to be expired in a different order from which the user read the data out
	of the buffer. Note that the system will only reuse blocks up to the point of the oldest non-expired block read. This means that the user must
	ensure to expire all blocks in a timely manner to prevent the system from running out of usable memory.
	@param	aAddress aAddress is the start address of the block of data previously read by the user which can be overwritten.	
	@return			KErrNotSupported if its an IN endpoint
					KErrNone if iTail  is successfully bumped to the next transfer to be read
					KErrNotFound if a 'transfer' with start address of the data block is aAddress is not found
	*/

	IMPORT_C TInt Expire(TAny* aAddress);

	/**
	Initiates write operation.
	@param	aBuffer	aBuffer will point to data in shared chunk to be written out. aBuffer should be aligned	
	@param	aSize	aSize will hold the number of valid bytes to be written out
	@param	aZLP	aZLP will indicate whether a ZLP should be transmitted after writing the data out
	@param	aStatus	This is an asynchronous function and user side should wait on status to know the end of write operation
	@return			KErrNone if a write is successfully queued
					KErrEof if an alternate setting change has occurred, ending this endpoint.
					KErrNotSupported if its an OUT endpoint
					KErrAccessDenied if endpoint is not opened, or if buffer is out of range
	*/
	IMPORT_C TInt WriteBuffer(TAny* aBuffer,TUint aSize,TBool aZLP,TRequestStatus& aStatus);

	/**
	Initiates write operation.
	@param	aOffset	aOffset will point to offset of data in shared chunk to be written out. 	
	@param	aSize	aSize will hold the number of valid bytes to be written out
	@param	aZLP	aZLP will indicate whether a ZLP should be transmitted after writing the data out
	@param	aStatus	This is an asynchronous function and user side should wait on status to know the end of write operation
	@return			KErrNone if a write is successfully queued
					KErrEof if an alternate setting change has occurred, ending this endpoint.
					KErrNotSupported if its an OUT endpoint
					KErrAccessDenied if endpoint is not opened, or if buffer is out of range
	*/
	IMPORT_C TInt WriteBuffer(TUint aOffset,TUint aSize,TBool aZLP,TRequestStatus& aStatus);
	/**
	For IN endpoints, this method retrieves the geometry for the buffer, for which the
	caller should stay within, when using the WriteBuffer method.

	@param aStart A pointer, which is set to point to the start of the buffer.
	@param aSize An TUint for which the size (in bytes) of buffer, is written into.

	@returns KErrNotSupported if the object is on an open IN endpoint, 
			otherwise it KErrNone is returned on success.
	*/
	IMPORT_C TInt GetInBufferRange(TAny*& aStart, TUint& aSize);

	/**
	For IN endpoints, this method retrieves the geometry for the buffer, for which the
	caller should stay within, when using the WriteBuffer method.

	@param aStart A TUint for which the buffer's start offset from the start of the chunk,
					in written into.
	@param aSize An TUint for which the size (in bytes) of buffer, is written into.

	@returns KErrNotSupported if the object is on an open IN endpoint, 
			otherwise it KErrNone is returned on success.
	*/
	IMPORT_C TInt GetInBufferRange(TUint& aStart, TUint& aSize);

	/**
	This method closes the endpoint, after it was opened with 
	RDevUsbcScClient::OpenEndpoint(...).
	No method of this object can be used after this	call, until
	RDevUsbcScClient::OpenEndpoint(...) is called on it again.

	@return	KErrNone on success, otherwise KErrNotFound, if the current object is not open.
	*/
	IMPORT_C TInt Close();

	IMPORT_C void Dump();

	/**
	Used to retrieve the endpoint number for which this object was open on.

	@returns the endpoint number opened by this object.
	*/
	inline TInt GetEndpointNumber();

	inline TInt BufferNumber();

private:
	/** @internalTechnology */
	void Construct(RDevUsbcScClient* aClient, TUint8* aBaseAddr, const TUsbcScHdrEndpointRecord* aEpType,
		 				TInt aEndpointNumber, SUsbcScBufferHeader* aEndpointHdr=NULL);

private:
	enum TDir {EValid = KErrNone, ENotValid = KErrNotSupported, EEOF = KErrEof};
	TDir iInState;						/** @internalTechnology describes state of endpoint, KErrNone if IN endpoint and ready to use, KErrNotSupportd if not an IN endpoint, KErrEof on alternate setting change */
	TDir iOutState;						/** @internalTechnology describes state of endpoint, KErrNone if OUT endpoint and ready to use, KErrNotSupportd if not an OUT endpoint, KErrEoF on alternate setting change */
	TInt iEndpointNumber;				/** @internalTechnology associated endpoint number */
	TInt iBufferNum;					/** @internalTechnology buffer number within shared chunk */
	RDevUsbcScClient *iClient;			/** @internalTechnology Parent RDevUsbcScClient object */
	TUint iBaseAddr;					/** @internalTechnology The address of the beginning of the Ldd's chunk */

	SUsbcScBufferHeader* iEndpointHdr;  /** @internalTechnology Pointer to the buffer Header for OUT/BI endpoints */
	TUint8* iBufferStartAddr; 			/** @internalTechnology IN/BI endpoint buffer start address within shared chunk */
	TUint iSize;						/** @internalTechnology IN/BI endpoint buffer size within shared chunk */
	friend class RDevUsbcScClient;
	};

/**
This class can be used to retrieve the geometry of the structures 
within a shared chunk, as used by RDevUsbcScClient.

@see RDevUsbcScClient
*/

class TUsbcScChunkHeader
	{
public:
/**
The constructor for the TUsbcScChunkHeader class takes a RChunk object, 
containing USBcSc data structures, and initialises itself, so that
GetNumberOfEndpoints & GetBuffer can be used to interpret the chunk structures.

@param	An RChunk object, which represents an shared chunk, containing the
		USBcSc data structures to be retrieved.
*/
	IMPORT_C TUsbcScChunkHeader(RChunk aChunk);
/**
Retrieves the available information in the chunk, about the given endpoint, 
on the given alternate setting.  The returned TUsbcScBufferRecord, 
represents the buffer geometry, used for for the endpoint, while
the filled in TUsbcScHdrEndpointRecord represents additional endpoint
information.

@param aAltSetting The alternate setting, for which the provided endpoint number, is a member of.
@param aEndpoint	The endpoint, who's buffer geometry is required.
@param aEndpointInf	The provided record is filled in with details of the endpoint, who's number was given.
*/
	IMPORT_C TUsbcScBufferRecord* GetBuffer(TInt aAltSetting, TInt aEndpoint, TUsbcScHdrEndpointRecord*& aEndpointInf);
/**
Retrieves the number of endpoints found in a given alternate setting.
@param aAltSetting The alternate setting number, for which the number of endpoints contained within, is needed.
*/
	IMPORT_C TInt GetNumberOfEndpoints(TInt aAltSetting);

public:
	TUsbcScChunkBuffersHeader*    iBuffers;		/** A pointer to the TUsbcScChunkBuffersHeader object, within the chunk header */
	TUsbcScChunkAltSettingHeader* iAltSettings;	/** A pointer to the TUsbcScChunkAltSettingHeader object, within the chunk header */
private:
	RChunk iChunk;
	};

#endif

#include <d32usbcsc.inl>

#endif // __D32USBCSC_H__
