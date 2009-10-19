// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32/include/d32usbcshared.h
// User side class definitions for USB Device support.
// 
//

/**
 @file d32usbcshared.h
 @publishedPartner
 @released
*/

#ifndef __D32USBCSHARED_H__
#define __D32USBCSHARED_H__

// USB driver error codes

/** USB driver specific error codes start from here
*/
const TInt KErrUsbDriverBase = -6701;

/**	Attempt at data transfer, or something interface related,
	when neither an Interface has been set up nor Device Control is owned by
	the channel
*/
const TInt KErrUsbInterfaceNotReady = -6702;

/**	Attempt at data transfer on an endpoint that does not belong to the active interface
*/
const TInt KErrUsbEpNotInInterface = -6703;

/**	Attempt at data transfer in a direction not supported by the endpoint
*/
const TInt KErrUsbEpBadDirection = -6704;

/**	The data transfer size specified exceeds that of the source or
	destination buffer descriptor
*/
const TInt KErrUsbTransferSize = -6705;

/**	This has multiple uses:
	1) User request completed because device is no longer in configured state
	2) Something endpoint related, stall, unstall, status enquiry etc,
	   that requires the device to be configured
*/
const TInt KErrUsbDeviceNotConfigured = -6706;

/**	Requested endpoint properties inconsistent during Interface setup
*/
const TInt KErrUsbBadEndpoint = -6707;

/**	User data request completed because channel is closing (channel destructor called)
*/
const TInt KErrUsbDeviceClosing = -6708;

/**	User data request completed because current endpoint set is being
	replaced since alternate setting is changing
*/
const TInt KErrUsbInterfaceChange = -6709;

/**	User data request completed because cable has been detached (or equivalent)
*/
const TInt KErrUsbCableDetached = -6710;

/**	User data request completed because cable has been detached (or equivalent)
*/
const TInt KErrUsbDeviceBusReset = -6711;

/**	This means that read data is still available when a write request is made.
	Relates to bidirectional eps only (ep0).
	A bidirectional ep must consume all of its read data before attempting to write.
*/
const TInt KErrUsbEpNotReady = -6712;

/** These are states that are described in the USB standard.

	@see RDevUsbcClient::DeviceStatus()
	@see RDevUsbcClient::AlternateDeviceStatusNotify()
*/
enum TUsbcDeviceState
	{
	EUsbcDeviceStateUndefined,								// 0
	EUsbcDeviceStateAttached,								// 1
	EUsbcDeviceStatePowered,								// 2
	EUsbcDeviceStateDefault,								// 3
	EUsbcDeviceStateAddress,								// 4
	EUsbcDeviceStateConfigured,								// 5
	EUsbcDeviceStateSuspended,								// 6
 	EUsbcNoState = 0xff										// 255 (used as a place holder)
	};

/** The endpoint states.

	@see RDevUsbcClient::EndpointStatus()
	@see RDevUsbcClient::EndpointStatusNotify()
*/
enum TEndpointState
	{
	EEndpointStateNotStalled,
	EEndpointStateStalled,
	EEndpointStateUnknown
	};

/** Endpoint resources/behaviours.

	@see AllocateEndpointResource()
	@see DeAllocateEndpointResource()
	@see QueryEndpointResourceUse()
*/
enum TUsbcEndpointResource
	{
	/** Requests the use of DMA. */
	EUsbcEndpointResourceDMA = 0,
	/** Requests the use of double FIFO buffering. */
	EUsbcEndpointResourceDoubleBuffering = 1
	};


/** The USB client device capability class.
*/
class TCapsDevUsbc
	{
public:
	/** The device version. */
	TVersion version;
	};


/** The maximum number of endpoints supported by the device, excluding ep0.
*/
const TInt KUsbcMaxEndpoints = 30;

/** The maximum number of endpoints per interface, excluding ep0.
*/
const TInt KMaxEndpointsPerClient = 5;

/** @internalComponent
*/
const TInt KInvalidEndpointNumber = 31;

/** The alternate setting flag; when this bit is set the state change notified by
	RDevUsbcClient::AlternateDeviceStatusNotify() is an alternate setting number.
*/
const TUint KUsbAlternateSetting = 0x80000000;

/** The USB cable detection feature flag; used by TUsbDeviceCapsV01::iFeatureWord1.
	When this bit is set then the USB controller hardware (UDC) supports detection
	of a plugged-in USB cable even when not powered.

	@see TUsbDeviceCapsV01
*/
const TUint KUsbDevCapsFeatureWord1_CableDetectWithoutPower = 0x00000001;

/** If this flag is set then the driver supports the new endpoint resource
	allocation scheme for DMA and Double-buffering via
	TUsbcEndpointInfo::iFeatureWord1.

	@see TUsbDeviceCapsV01
*/
const TUint KUsbDevCapsFeatureWord1_EndpointResourceAllocV2 = 0x00000002;


/** Device USB capabilities.
*/
class TUsbDeviceCapsV01
	{
public:
	/** The total number of endpoints on the device. */
	TInt iTotalEndpoints;
	/** Indicates whether the device supports software connect/disconnect. */
	TBool iConnect;
	/** Indicates whether the device is self powered. */
	TBool iSelfPowered;
	/** Indicates whether the device can send Remote Wakeup. */
	TBool iRemoteWakeup;
	/** Indicates whether the device supports High-speed mode. */
	TBool iHighSpeed;
	/** 32 flag bits indicating miscellaneous UDC/device features.
		Currently defined are:
		- KUsbDevCapsFeatureWord1_CableDetectWithoutPower = 0x00000001
		- KUsbDevCapsFeatureWord1_EndpointResourceAllocV2 = 0x00000002
	*/
	TUint32 iFeatureWord1;
	/** Reserved for future use. */
	TUint32 iReserved;
	};

/** Package buffer for a TUsbDeviceCapsV01 object.

	@see TUsbDeviceCapsV01
*/
typedef TPckgBuf<TUsbDeviceCapsV01> TUsbDeviceCaps;

/** Bitmaps for TUsbcEndpointCaps.iSizes.

	This endpoint is not available (= no size).
*/
const TUint KUsbEpNotAvailable = 0x00000000;
/**	Max packet size is continuously variable up to some size specified.
	(Interrupt and Isochronous endpoints only.)
*/
const TUint KUsbEpSizeCont     = 0x00000001;
/** Max packet size 8 bytes is supported
*/
const TUint KUsbEpSize8        = 0x00000008;
/** Max packet size 16 bytes is supported
*/
const TUint KUsbEpSize16       = 0x00000010;
/** Max packet size 32 bytes is supported
*/
const TUint KUsbEpSize32       = 0x00000020;
/** Max packet size 64 bytes is supported
*/
const TUint KUsbEpSize64       = 0x00000040;
/** Max packet size 128 bytes is supported
*/
const TUint KUsbEpSize128      = 0x00000080;
/** Max packet size 256 bytes is supported
*/
const TUint KUsbEpSize256      = 0x00000100;
/** Max packet size 512 bytes is supported
*/
const TUint KUsbEpSize512      = 0x00000200;
/** Max packet size 1023 bytes is supported
*/
const TUint KUsbEpSize1023     = 0x00000400;
/** Max packet size 1024 bytes is supported
*/
const TUint KUsbEpSize1024     = 0x00000800;


/** Bitmaps for TUsbcEndpointCaps.iSupportedTypesAndDir.

	Endpoint supports Control transfer type.
*/
const TUint KUsbEpTypeControl     = 0x00000001;
/** Endpoint supports Isochronous transfer type.
*/
const TUint KUsbEpTypeIsochronous = 0x00000002;
/** Endpoint supports Bulk transfer type.
*/
const TUint KUsbEpTypeBulk        = 0x00000004;
/** Endpoint supports Interrupt transfer type.
*/
const TUint KUsbEpTypeInterrupt   = 0x00000008;
/** Endpoint supports IN transfers.
*/
const TUint KUsbEpDirIn           = 0x80000000;
/** Endpoint supports OUT transfers.
*/
const TUint KUsbEpDirOut          = 0x40000000;
/** Endpoint supports bidirectional (Control) transfers only.
*/
const TUint KUsbEpDirBidirect     = 0x20000000;


/** Converts an absolute size value into a KUsbEpSize... mask.
*/
static inline TUint PacketSize2Mask(TInt aSize);

/** Converts an endpoint type mask KUsbEpType...  into an endpoint attribute
	value KUsbEpAttr_....
*/
static inline TUint EpTypeMask2Value(TInt aType);


/** Endpoint capabilities as reported by the driver.
*/
class TUsbcEndpointCaps
	{
public:
	/** Returns the greatest available packet size for this endpoint. */
	TInt MaxPacketSize() const;
	/** Returns the smallest available packet size for this endpoint. */
	TInt MinPacketSize() const;
public:
	/** The supported maximum packet sizes. */
	TUint iSizes;
	/** The supported endpoint types and directions. */
	TUint iTypesAndDir;
	/** This is a 'high-speed, high bandwidth' endpoint. */
	TBool iHighBandwidth;
	/** Reserved for future use. */
	TUint32 iReserved[2];
	};


/** Endpoint capabilities as returned by RDevUsbcClient::EndpointCaps().
*/
class TUsbcEndpointData
	{
public:
	/** Detail of endpoint capabilities. */
	TUsbcEndpointCaps iCaps;
	/** Indicates whether this endpoint is already claimed. */
	TBool iInUse;
	};

/** The endpoint resource allocation flags;
	used by TUsbcEndpointInfo::iFeatureWord1.

	@see TUsbcEndpointInfo
*/
const TUint KUsbcEndpointInfoFeatureWord1_DMA             = 0x00000001;
const TUint KUsbcEndpointInfoFeatureWord1_DoubleBuffering = 0x00000002;

/** The desired endpoint capabilities used in RDevUsbcClient::SetInterface().
*/
class TUsbcEndpointInfo
	{
public:
	TUsbcEndpointInfo(TUint aType=KUsbEpTypeBulk, TUint aDir=KUsbEpDirOut,
					  TInt aSize=0, TInt aInterval=0, TInt aExtra=0);
	/** @internalComponent */
	TInt AdjustEpSizes(TInt& aEpSize_Fs, TInt& aEpSize_Hs) const;
	/** @internalComponent */
	TInt AdjustPollInterval();
public:
	/** Endpoint type (mask: KUsbEpTypeControl, etc., but used as value). */
	TUint iType;
	/** Direction (mask: KUsbEpDirIn, etc., but used as value). */
	TUint iDir;
	/** Maximum packet size (literal, no mask). */
	TInt iSize;
	/** Interval for polling full-speed interrupt and isochronous endpoints.
		Expressed either directly in milliseconds with a valid range 1..255
		(interrupt), or for use as 'value' in the expression interval=2^(value-1)
		with a valid range 1..16 (isochronous).
	*/
	TInt iInterval;
	/** Interval for polling high-speed interrupt and isochronous endpoints,
		or to specify the NAK rate for high-speed control and bulk OUT endpoints.
		Expressed either for use as 'value' in the expression interval=2^(value-1)
		with a valid range 1..16 (interrupt and isochronous), or directly as the
		maximum NAK rate with a valid range 0..255 (control and bulk).
	*/
	TInt iInterval_Hs;
	/** The number of additional transactions per uframe to be scheduled (0..2)
		(A value greater than zero is only valid for high-speed high bandwidth
		 interrupt and isochronous endpoints. Also note that there are endpoint size
		 restrictions associated with additional transactions - see 9.6.6.)
	*/
	TInt iTransactions;
	/** The number of extra bytes that the standard endpoint descriptor should be extended by.
		In almost all cases, this should be 0 (zero).
	*/
	TInt iExtra;
	/** 32 flag bits indicating miscellaneous endpoint features.
		Currently defined are:
		- KUsbcEndpointInfoFeatureWord1_DMA             = 0x00000001
		- KUsbcEndpointInfoFeatureWord1_DoubleBuffering = 0x00000002
	*/
	TUint32 iFeatureWord1;
	/** Reserved for future use. */
	TUint32 iReserved;
	};

/** USB Class information used in RDevUsbcClient::SetInterface().
*/
class TUsbcClassInfo
	{
public:
	TUsbcClassInfo(TInt aClass=0, TInt aSubClass=0, TInt aProtocol=0);
public:
	/** The class type number. */
	TInt iClassNum;
	/** The sub-class type number. */
	TInt iSubClassNum;
	/** The protocol number. */
	TInt iProtocolNum;
	/** Reserved for future use. */
	TUint32 iReserved;
	};


/** The Ep0 Setup request 'unsubscribe' flag; used by
	TUsbcInterfaceInfo::iFeatureWord. When this bit is set then valid vendor-
	or class-specific Ep0 requests addressed to this interface or any of its
	endpoints will be stalled by the USB PDD PIL.

	@see TUsbcInterfaceInfo
*/
const TUint KUsbcInterfaceInfo_NoEp0RequestsPlease = 0x00000001;

#include <d32usbcshared.inl>

#endif 