// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32/include/d32usbc.h
// User side class definitions for USB Device support.
// 
//

/**
 @file d32usbc.h
 @publishedPartner
 @released
*/

#ifndef __D32USBC_H__
#define __D32USBC_H__

#include <e32ver.h>
#include <usb.h>
#include <d32usbcshared.h>



/** @internalComponent
*/
enum TTransferType
	{
	ETransferTypeReadData,
	ETransferTypeReadPacket,
	ETransferTypeWrite,
	ETransferTypeNone,
	ETransferTypeReadOneOrMore,
	ETransferTypeReadUntilShort
	};


/** Available endpoints. At most only these are available per interface.
*/
enum TEndpointNumber
	{
	EEndpoint0 = 0,
	EEndpoint1 = 1,
	EEndpoint2 = 2,
	EEndpoint3 = 3,
	EEndpoint4 = 4,
	EEndpoint5 = 5
	};




/** Bandwidth indicators for an Interface

	@see RDevUsbcClient::SetInterface()
*/
enum TUsbcBandwidthPriority
	{
	/** Economical OUT buffering. */
	EUsbcBandwidthOUTDefault = 0x00,
	/** More memory than Default for OUT buffering. */
	EUsbcBandwidthOUTPlus1   = 0x01,
	/** More memory than Plus1 for OUT buffering. */
	EUsbcBandwidthOUTPlus2   = 0x02,
	/** Maximum memory for OUT buffering.
		Use this value for high volume USB High-speed
		data transfers only, otherwise memory will be wasted.
	*/
	EUsbcBandwidthOUTMaximum = 0x03,
	//
	/** Economical IN buffering */
	EUsbcBandwidthINDefault  = 0x00,
	/** More memory than Default for IN buffering */
	EUsbcBandwidthINPlus1    = 0x10,
	/** More memory than Plus1 for IN buffering */
	EUsbcBandwidthINPlus2    = 0x20,
	/** Maximum memory for IN buffering.
		Use this value for high volume USB High-speed
		data transfers only, otherwise memory will be wasted.
	*/
	EUsbcBandwidthINMaximum  = 0x30
	};







/** Bit positions of endpoints.

	@see RDevUsbcClient::EndpointStatusNotify()
	@see RDevUsbcClient::EndpointTransferCancel()

	Bit position of endpoint0.
*/
const TUint KUsbcEndpoint0Bit = 1<<EEndpoint0;
/** Bit position of endpoint1.
*/
const TUint KUsbcEndpoint1Bit = 1<<EEndpoint1;
/** Bit position of endpoint2.
*/
const TUint KUsbcEndpoint2Bit = 1<<EEndpoint2;
/** Bit position of endpoint3.
*/
const TUint KUsbcEndpoint3Bit = 1<<EEndpoint3;
/** Bit position of endpoint4.
*/
const TUint KUsbcEndpoint4Bit = 1<<EEndpoint4;
/** Bit position of endpoint5.
*/
const TUint KUsbcEndpoint5Bit = 1<<EEndpoint5;








/** This must be filled in prior to a call to RDevUsbcClient::SetInterface().
*/
class TUsbcInterfaceInfo
	{
public:
	TUsbcInterfaceInfo(TInt aClass=0, TInt aSubClass=0, TInt aProtocol=0,
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
	TUsbcEndpointInfo iEndpointData[KMaxEndpointsPerClient];
	/** 32 flag bits used for specifying miscellaneous Interface features.
		Currently defined are:
		- KUsbcInterfaceInfo_NoEp0RequestsPlease = 0x00000001
	*/
	TUint32 iFeatureWord;
	};


/** Package buffer for a TUsbcInterfaceInfo object.

	@see TUsbcInterfaceInfo
*/
typedef TPckgBuf<TUsbcInterfaceInfo> TUsbcInterfaceInfoBuf;


/** The user side handle to the USB client driver.
*/
class RDevUsbcClient : public RBusLogicalChannel
	{
public:
	/** @internalComponent */
	enum TVer
		{
		EMajorVersionNumber = 0,
		EMinorVersionNumber = 1,
		EBuildVersionNumber = KE32BuildVersionNumber
		};

	enum TRequest
		{
		// Positive requests.
		ERequestEp0 = 0x0,
		ERequestEp1 = EEndpoint1,
		ERequestEp2 = EEndpoint2,
		ERequestEp3 = EEndpoint3,
		ERequestEp4 = EEndpoint4,
		ERequestEp5 = EEndpoint5,
		ERequestUnused = 6,
		ERequestAlternateDeviceStatusNotify = 7,
		ERequestReEnumerate = 8,
		ERequestEndpointStatusNotify = 9,
		// The cancel TRequest's are interpreted as bitmaps. As they're not mixed
		// with the previous ones, it doesn't matter if they have the same absolute
		// value as those.
		ERequestEp0Cancel = 1<<ERequestEp0,
		ERequestEp1Cancel = 1<<ERequestEp1,
		ERequestEp2Cancel = 1<<ERequestEp2,
		ERequestEp3Cancel = 1<<ERequestEp3,
		ERequestEp4Cancel = 1<<ERequestEp4,
		ERequestEp5Cancel = 1<<ERequestEp5,
		ERequestUnusedCancel = 1<<ERequestUnused,
        ERequestAllCancel = (ERequestEp0Cancel | ERequestEp1Cancel |
							 ERequestEp2Cancel | ERequestEp3Cancel |
							 ERequestEp4Cancel | ERequestEp5Cancel |
							 ERequestUnusedCancel),
		ERequestAlternateDeviceStatusNotifyCancel = 1<<ERequestAlternateDeviceStatusNotify,
		ERequestReEnumerateCancel = 1<<ERequestReEnumerate,
		ERequestEndpointStatusNotifyCancel = 1<<ERequestEndpointStatusNotify,
        ERequestOtgFeaturesNotify = 10,
        ERequestOtgFeaturesNotifyCancel = 1<<ERequestOtgFeaturesNotify,
		};

	enum TControl
		{
		// Changing the order of these enums will break BC.
		EControlEndpointZeroRequestError,					// 0
		EControlEndpointCaps,
		EControlDeviceCaps,
		EControlGetAlternateSetting,
		EControlDeviceStatus,
		EControlEndpointStatus,
		EControlSetInterface,
		EControlReleaseInterface,
		EControlQueryReceiveBuffer,
		EControlSendEp0StatusPacket,						// 9
		//
		EControlHaltEndpoint,								// 10
		EControlClearHaltEndpoint,
		EControlSetDeviceControl,
		EControlReleaseDeviceControl,
		EControlEndpointZeroMaxPacketSizes,
		EControlSetEndpointZeroMaxPacketSize,
		EControlGetDeviceDescriptor,
		EControlSetDeviceDescriptor,
		EControlGetDeviceDescriptorSize,
		EControlGetConfigurationDescriptor,					// 19
		//
		EControlSetConfigurationDescriptor,					// 20
		EControlGetConfigurationDescriptorSize,
		EControlGetInterfaceDescriptor,
		EControlSetInterfaceDescriptor,
		EControlGetInterfaceDescriptorSize,
		EControlGetEndpointDescriptor,
		EControlSetEndpointDescriptor,
		EControlGetEndpointDescriptorSize,
		EControlGetCSInterfaceDescriptor,
		EControlSetCSInterfaceDescriptor,					// 29
		//
		EControlGetCSInterfaceDescriptorSize,				// 30
		EControlGetCSEndpointDescriptor,
		EControlSetCSEndpointDescriptor,
		EControlGetCSEndpointDescriptorSize,
		EControlSignalRemoteWakeup,
		EControlGetStringDescriptorLangId,
		EControlSetStringDescriptorLangId,
		EControlGetManufacturerStringDescriptor,
		EControlSetManufacturerStringDescriptor,
		EControlRemoveManufacturerStringDescriptor,			// 39
		//
		EControlGetProductStringDescriptor,					// 40
		EControlSetProductStringDescriptor,
		EControlRemoveProductStringDescriptor,
		EControlGetSerialNumberStringDescriptor,
		EControlSetSerialNumberStringDescriptor,
		EControlRemoveSerialNumberStringDescriptor,
		EControlGetConfigurationStringDescriptor,
		EControlSetConfigurationStringDescriptor,
		EControlRemoveConfigurationStringDescriptor,
		EControlDeviceDisconnectFromHost,					// 49
		//
		EControlDeviceConnectToHost,						// 50
		EControlDevicePowerUpUdc,
		EControlDumpRegisters,
		EControlSetInterface1,								// (not used)
		EControlAllocateEndpointResource,
		EControlDeAllocateEndpointResource,
		EControlQueryEndpointResourceUse,
		EControlGetEndpointZeroMaxPacketSize,
		EControlGetDeviceQualifierDescriptor,
		EControlSetDeviceQualifierDescriptor,				// 59
		//
		EControlGetOtherSpeedConfigurationDescriptor,		// 60
		EControlSetOtherSpeedConfigurationDescriptor,
		EControlCurrentlyUsingHighSpeed,
		EControlSetStringDescriptor,
		EControlGetStringDescriptor,
		EControlRemoveStringDescriptor,
        EControlSetOtgDescriptor,
        EControlGetOtgDescriptor,
        EControlGetOtgFeatures
		};

public:

#ifndef __KERNEL_MODE__

	/** Opens a channel.

		@param aUnit This should be 0 (zero).

		@return KErrNone if successful.
	*/
	inline TInt Open(TInt aUnit);

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

		@param aEpCapsBuf a descriptor encapsulating an array of TUsbcEndpointData.

		@return KErrNone if successful.
	*/
	inline TInt EndpointCaps(TDes8& aEpCapsBuf);

	/** Retrieves the capabilities of the USB device.

		@see TUsbDeviceCaps

		@param aDevCapsBuf a TUsbDeviceCaps object.

		@return KErrNone if successful.
	*/
	inline TInt DeviceCaps(TUsbDeviceCaps& aDevCapsBuf);

	/** Copies the current alternate setting for this interface into aInterfaceNumber
		For USB Interfaces whose main interface is active, this will be 0 (zero).

		@param aInterfaceNumber current alternate setting for this interface is copied into this.

		@return KErrNone if successful.
	*/
	inline TInt GetAlternateSetting(TInt& aInterfaceNumber);

	/** Copies the current device status into aDeviceStatus.

		@param aDeviceStatus current device status is copied into this.

		@return KErrNone if successful
	*/
	inline TInt DeviceStatus(TUsbcDeviceState& aDeviceStatus);

	/** Copies the current endpoint status into aEndpointStatus.

		@param aEndpoint endpoint number valid for the current alternate setting.
		@param aEndpointStatus the current endpoint status, might be stalled, not stalled or unknown.

		@return KErrNone if successful.
	*/
	inline TInt EndpointStatus(TEndpointNumber aEndpoint, TEndpointState& aEndpointStatus);

	/** Copies the number of bytes available in the aEndpoint read buffer into aNumberOfBytes.

		@param aEndpoint endpoint number valid for the current alternate setting.
		@param aNumberOfBytes number of bytes available in the aEndpoint read buffer.

		@return KErrNone if successful.
	*/
	inline TInt QueryReceiveBuffer(TEndpointNumber aEndpoint, TInt& aNumberOfBytes);

	/** Requests that a zero length status packet be sent to the host in response
		to a class or vendor specific ep0 SETUP packet.

		@return KErrNone if successful.
	*/
	inline TInt SendEp0StatusPacket();

	/** Stalls endpoint aEndpoint, usually to indicate an error condition with a previous command.
		The host will normally send a SET_FEATURE command on ep0 to acknowledge and clear the stall.

		@return KErrNone if successful.
	*/
	inline TInt HaltEndpoint(TEndpointNumber aEndpoint);

	/** Clears the stall condition on endpoint aEndpoint. This is inluded for symmetry and test purposes.

		@return KErrNone if successful.
	*/
	inline TInt ClearHaltEndpoint(TEndpointNumber aEndpoint);

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

		@param aDeviceDescriptor contains the device descriptor.

		@return KErrNone if successful.
	*/
	inline TInt SetDeviceDescriptor(const TDesC8& aDeviceDescriptor);

	/** Gets the size of the current device descriptor. This is unlikely to be anything other than 9.

		@param aSize receives the size of the current device descriptor.

		@return KErrNone if successful.
	*/
	inline TInt GetDeviceDescriptorSize(TInt& aSize);

	/** Copies the current configuration descriptor into aConfigurationDescriptor.

		@param aConfigurationDescriptor Receives the current configuration descriptor.

		@return KErrNone if successful.
	*/
	inline TInt GetConfigurationDescriptor(TDes8& aConfigurationDescriptor);

	/** Sets the contents of aConfigurationDescriptor to be the current configuration descriptor.

		@param aConfigurationDescriptor contains the configuration descriptor.

		@return KErrNone if successful.
	*/
	inline TInt SetConfigurationDescriptor(const TDesC8& aConfigurationDescriptor);

	/** Gets the size of the current configuration descriptor.

		@param aSize receives the size of the current configuration descriptor.

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
		@param aEndpointNumber.
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
		@param aSize receives the size of the endpoint descriptor.

		@return KErrNone if successful.
	*/
	inline TInt GetEndpointDescriptorSize(TInt aSettingNumber, TInt aEndpointNumber, TInt& aSize);

    /** Get OTG descriptor size

		@param aSize TInt reference which contains OTG descriptor size on return
    */
    inline void GetOtgDescriptorSize(TInt& aSize);

    /** Get OTG descriptor of USB on-the-go feature

		@param aOtgDesc User-side buffer to store copy of descriptor

		@return KErrNone if successful
    */
    inline TInt GetOtgDescriptor(TDes8& aOtgDesc);

    /** Set OTG descriptor by user to enable/disable USB on-the-go feature

		@param aOtgDesc Descriptor buffer containing OTG features

		@return KErrNone if successful
    */
    inline TInt SetOtgDescriptor(const TDesC8& aOtgDesc);

	/** Copies the current device_qualifier descriptor into aDescriptor.

		@param aDescriptor Receives the current device_qualifier descriptor.

		@return KErrNone if successful.
	*/
	inline TInt GetDeviceQualifierDescriptor(TDes8& aDescriptor);

	/** Sets the device_qualifier descriptor to the contents of aDescriptor.

		@param aDescriptor contains the new device_qualifier descriptor.

		@return KErrNone if successful.
	*/
	inline TInt SetDeviceQualifierDescriptor(const TDesC8& aDescriptor);

	/** Copies the current other_speed_configuration descriptor into aDescriptor.

		@param aDescriptor Receives the current other_speed_configuration descriptor.

		@return KErrNone if successful.
	*/
	inline TInt GetOtherSpeedConfigurationDescriptor(TDes8& aDescriptor);

	/** Sets the other_speed_configuration descriptor to the contents of aDescriptor.

		@param aDescriptor contains the new other_speed_configuration descriptor.

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
		aDes may contain as many descriptors as are necessary or only 1. SetCSInterfaceDescriptorBlock()
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

	/** Generates a Remote Wakeup bus condition
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

	/** Allocates the use of aResource to aEndpoint. it will be used from when the current bus transfer	has been
		completed.

		@param aResource is typically some rationed hardware resource or possibly specifies a type of endpoint
		behaviour. aResource is not a bitmap and TEndpointResource values should not be combined.
		@param aEndpoint The endpoint number to which the resource is to be allocated.

		@return KErrNone if successful, KErrInUse if the resource is already consumed and cannot be allocated,
		KErrNotSupported if the endpoint does not support the resource requested.

		@publishedPartner @deprecated

		@see TUsbcEndpointInfo
	*/
	inline TInt AllocateEndpointResource(TInt aEndpoint, TUsbcEndpointResource aResource);

	/** Deallocates the use of aResource aEndpoint or ends a specified endpoint behaviour.

		@param aResource is typically some rationed hardware resource or possibly specifies a type of endpoint
		behaviour. aResource is not a bitmap and TEndpointResource values should not be combined.
		@param aEndpoint The endpoint number from which the resource is to be removed.

		@return KErrNone if the resource has been successfully deallocated, KErrNotSupported if the endpoint
		does not support the resource requested.

		@publishedPartner @deprecated

		@see TUsbcEndpointInfo
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
		@param aInterfaceData a package buffer which describes the interface and all the endpoints being
		requested by the driver for this interface.
		@param aBandwidthPriority is a bitmap combining the required IN and OUT priorities. Values are in the
		range [0,3] from the lowest priority bandwidth, 0, to the highest 3 and are separately specified for
		IN and OUT endpoints. Interfaces requiring higher bandwidth are allocated significantly more buffering
		than low bandwidth interfaces. Interfaces should not be given a higher bandwidth priority than they
		require.

		@return KErrInUse if any of the endpoints being requested have already been claimed by another channel
		KErrNotSupported if an endpoint with all of the specified properties is not supported on this
		platform. KErrNoMemory if insufficient memory is available to complete the operation.
	*/
	inline TInt SetInterface(TInt aInterfaceNumber, TUsbcInterfaceInfoBuf& aInterfaceData,
							 TUint32 aBandwidthPriority =
							 (EUsbcBandwidthOUTDefault | EUsbcBandwidthINDefault));

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

		@param aLangId receives the LANGID code.

		@return KErrNone if successful, KErrArgument if problem with argument (memory cannot be written to, etc.).
	*/
	inline TInt GetStringDescriptorLangId(TUint16& aLangId);

	/** Sets the string descriptor language ID (LANGID). Even though the USB spec allows for the existence of
		a whole array of LANGID codes, we only support one.

		@param aLangId the LANGID code to be set.

		@return KErrNone if successful.
	*/
	inline TInt SetStringDescriptorLangId(TUint16 aLangId);

	/** Copies the string descriptor identified by the iManufacturer index field of the Standard Device
		Descriptor into the aString argument.

		@param aString receives manufacturer string.

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

		@param aString receives product string.

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

		@param aString receives product string.

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

		@param aString receives configuration string.

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

	/** Asynchronously read data from endpoint 'aEndpoint' into the descriptor 'aDes'.
		Request completes when the specified number of bytes is received, length taken from max. length of
		descriptor.

		@param aStatus The request status.
		@param aEndpoint The endpoint number to read from.
		@param aDes	Descriptor to receive the data.
	*/
	inline void Read(TRequestStatus& aStatus, TEndpointNumber aEndpoint, TDes8& aDes);

	/** Asynchronously read data from endpoint 'aEndpoint' into the descriptor 'aDes'.
		Request completes when the specified number of bytes is received.

		@param aStatus The request status.
		@param aEndpoint The endpoint number to read from.
		@param aDes	Descriptor to receive the data.
		@param aLen The number of bytes to read.
	*/
	inline void Read(TRequestStatus& aStatus, TEndpointNumber aEndpoint, TDes8& aDes, TInt aLen);

	/** Asynchronously read an entire packet of data from endpoint 'aEndpoint' into the descriptor 'aDes'.
		If a packet has previously been partly read. then only the remainder of the packet will be returned.
		The request should be for the maximum packet size of the endpoint. If less data is requested, then
		after this read completes the remainder of the data in the packet will be discarded and the next read
		will start from the next available packet.
		Request completes when either a complete packet is received or the length of the packet currently
		being received exceeds 'aMaxLen'.

		@param aStatus The request status.
		@param aEndpoint The endpoint number to read from.
		@param aDes	Descriptor to receive the data.
		@param aMaxLen .
	*/
	inline void ReadPacket(TRequestStatus& aStatus, TEndpointNumber aEndpoint, TDes8& aDes, TInt aMaxLen);

	/** Asynchronously read data from endpoint 'aEndpoint' into the descriptor 'aDes'.
		Request completes when the specified number of bytes is received (in first version,
		length taken from max. length of descriptor).

		@param aStatus The request status.
		@param aEndpoint The endpoint number to read from.
		@param aDes	Descriptor to receive the data.
	*/
	inline void ReadUntilShort(TRequestStatus& aStatus, TEndpointNumber aEndpoint, TDes8& aDes);

	/** Asynchronously read data from endpoint 'aEndpoint' into the descriptor 'aDes'.
		Request completes when the specified number of bytes is received (in first version,
		length taken from max. length of descriptor).

		@param aStatus The request status.
		@param aEndpoint The endpoint number to read from.
		@param aDes	Descriptor to receive the data.
		@param aLen The number of bytes to receive.
	*/
	inline void ReadUntilShort(TRequestStatus& aStatus, TEndpointNumber aEndpoint, TDes8& aDes, TInt aLen);

	/** Asynchronously read data from endpoint 'aEndpoint' into the descriptor 'aDes'. The request completes
		when the specified number of bytes is received, length taken from max. length of descriptor or a
		packet whose size is smaller than the endpoint's maximum packet size is received.

		@param aStatus The request status.
		@param aEndpoint The endpoint number to read from.
		@param aDes	Descriptor to receive the data.
	*/
	inline void ReadOneOrMore(TRequestStatus& aStatus, TEndpointNumber aEndpoint, TDes8& aDes);

	/** Asynchronously read data from endpoint 'aEndpoint' into the descriptor 'aDes'. The request completes
		when the specified number of bytes is received, or a packet whose size is smaller than the endpoint's
		maximum packet size is received.

		@param aStatus The request status.
		@param aEndpoint The endpoint number to read from.
		@param aDes	Descriptor to receive the data.
		@param aLen The number of bytes to receive.
	*/
	inline void ReadOneOrMore(TRequestStatus& aStatus, TEndpointNumber aEndpoint, TDes8& aDes, TInt aLen);

	/** Cancels an outstanding read request. The request will complete with whatever data is available.
	*/
	inline void ReadCancel(TEndpointNumber aEndpoint);

	/** Asynchronously write 'aLen' bytes of data to endpoint 'aEndpoint' from descriptor 'aDes'. 'aZlpRequired'
		(optional) signals that ZLP termination may be required.

		@param aStatus The request status.
		@param aEndpoint The endpoint number to write to.
		@param aDes	Descriptor to provide the data.
		@param aLen The number of bytes of data to be written.
		@param aZlpRequired True, if ZLP termination is required; false, otherwise.
	*/
	inline void Write(TRequestStatus& aStatus, TEndpointNumber aEndpoint, const TDesC8& aDes, TInt aLen,
					  TBool aZlpRequired=EFalse);

	/** Cancels an outstanding write request on endpoint aEndpoint.

		@param aEndpoint The endpoint number whose write is to be cancelled.
	*/
	inline void WriteCancel(TEndpointNumber aEndpoint);

	/** Cancels any transfer on any endpoint specified in aEndpointMask.

		@code
		// Cancel transfer requests on endpoints 1, 2 & 3
		usbPort.EndpointTransferCancel(KUsbcEndpoint1Bit | KUsbcEndpoint2Bit | KUsbcEndpoint3Bit);
		@endcode

		@param aEndpointMask bitmap of the endpoints.
	*/
	inline void EndpointTransferCancel(TUint aEndpointMask);

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
		@param aEndpointMask a bitmap of the endpoints stall status. This is filled in when the call completes
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

        @param aFeatures On return it contanis features the device currently has
                bit 2 represents b_hnp_enable,       (KUsbOtgAttr_B_HnpEnable)
                bit 3 represents a_hnp_support,      (KUsbOtgAttr_A_HnpSupport)
                bit 4 represents a_alt_hnp_support,  (KUsbOtgAttr_A_AltHnpSupport)
        @return KErrNone if successful, KErrNotSupported if OTG is not supported by
                this device, otherwise system-wide error returns
    */
    inline TInt GetOtgFeatures(TUint8& aFeatures);

    /** Register for notification on USB on-the-go features' change. If any OTG feature
        is changed, request completes and current feature value is filled in aValue.

        @param aStatus Request status object
        @param aValue On request completion, it contains current OTG feature value
    */
    inline void OtgFeaturesNotify(TRequestStatus& aStatus, TUint8& aValue);

    /** Cancel pending OTG feature request.
    */
    inline void OtgFeaturesNotifyCancel();

#endif // #ifndef __KERNEL_MODE__
	};


#include <d32usbc.inl>


#endif // __D32USBC_H__
