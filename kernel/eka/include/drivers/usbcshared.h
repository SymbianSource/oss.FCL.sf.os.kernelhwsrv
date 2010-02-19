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
// e32\include\drivers\usbcshared.h
// Kernel side definitions for the USB Device driver stack (PIL + LDD).
// 
//

/**
 @file usbcshared.h
 @internalTechnology
*/

#ifndef __USBCSHARED_H__
#define __USBCSHARED_H__

#include <drivers/usbcque.h>

// Define here what options are required:
// (USB_SUPPORTS_CONTROLENDPOINTS and USB_SUPPORTS_SET_DESCRIPTOR_REQUEST
//  have never been tested though...)
//#define USB_SUPPORTS_CONTROLENDPOINTS
//#define USB_SUPPORTS_SET_DESCRIPTOR_REQUEST

#include <drivers/usbcdesc.h>

// Debug Support

// Use for debugging purposes only (commented out for normal operation):
//#define USBC_LDD_BUFFER_TRACE

static const char KUsbPILPanicCat[] = "USB PIL FAULT"; // kernel fault category
_LIT(KUsbPILKillCat, "USB PIL KILL");					// thread kill category
_LIT(KUsbLDDKillCat, "USB LDD KILL");					// thread kill category

/** Error code for stalled endpoint.
*/
const TInt KErrEndpointStall = KErrLocked;

/** Error code for Ep0 write prematurely ended by a host OUT token.
*/
const TInt KErrPrematureEnd = KErrDiskFull;

/** The following constants control the buffer arrangement for OUT transfers (IN transfers have only 1
	buffer). The total size of buffering for an OUT endpoint will be number of buffers * buffersize,
	so that, for example, a Bulk OUT endpoint will have KUsbcDmaBufNumBulk * KUsbcDmaBufSzBulk bytes of
	buffering.
	These buffers will be physically contiguous, so that DMA may be used.
	The number of buffers MUST be >=2 - otherwise the buffering scheme won't work.
	The buffer sizes should be an exact fraction of 4kB and the number of buffers such that the
	buffersize * number of buffers is an exact multiple of 4kB, otherwise memory will be wasted.
*/
/** Size of a Control ep buffer.
*/
const TInt KUsbcDmaBufSzControl = 1024;

/** Size of a Bulk ep buffer.
*/
const TInt KUsbcDmaBufSzBulk = 4096;

/** Size of an Interrupt ep buffer.
*/
const TInt KUsbcDmaBufSzInterrupt = 4096;

/** Size of an Isochronous ep buffer.
*/
const TInt KUsbcDmaBufSzIsochronous = 4096;

/** Number of buffers for Control OUT endpoints.
*/
const TInt KUsbcDmaBufNumControl = 2;

/** Number of buffers for Isochronous OUT endpoints.
*/
const TInt KUsbcDmaBufNumIsochronous = 2;

/** Number of buffers for Bulk OUT endpoints.
*/
const TInt KUsbcDmaBufNumBulk = 2;

/** Number of buffers for Interrupt OUT endpoints.
*/
const TInt KUsbcDmaBufNumInterrupt = 2;

/** Maximum buffer number.
*/
const TInt KUsbcDmaBufNumMax = MAX4(KUsbcDmaBufNumControl, KUsbcDmaBufNumIsochronous,
									KUsbcDmaBufNumBulk, KUsbcDmaBufNumInterrupt);

/** Maximum number of recorded packets possible.
*/
const TUint KUsbcDmaBufMaxPkts = 2;

/** Number of arrays.
*/
const TInt KUsbcDmaBufNumArrays = 2;

/** Max size that Ep0 packets might have.
*/
const TInt KUsbcBufSzControl = 64;

/** The Ep0 RX data collection buffer area.
	(Arbitrary size, judged to be sufficient for SET_DESCRIPTOR requests)
*/
const TInt KUsbcBufSz_Ep0Rx = 1024;

/** The Ep0 TX buffer area.
	(Size sufficient to hold as much data as can be requested via GET_DESCRIPTOR)
*/
const TInt KUsbcBufSz_Ep0Tx = 1024 * 64; 


/** The USB version the stack is compliant with: 2.0 (BCD).
*/
const TUint16 KUsbcUsbVersion = 0x0200;

/** Maximum number of endpoints an interface (i.e. LDD) may have.
*/
const TInt KUsbcMaxEpNumber = 5;

/** Status FIFO depth; enough for 2 complete configs.
*/
const TInt KUsbDeviceStatusQueueDepth = 15;

/** = 'no status info'.
*/
const TUint32 KUsbDeviceStatusNull = 0xffffffffu;

/** = 'no buffer available'.
*/
const TInt KUsbcInvalidBufferIndex = -1;

/** = 'no packet available'.
*/
const TUint KUsbcInvalidPacketIndex = (TUint)(-1);

/** = 'no drainable buffers'.
*/
const TInt KUsbcInvalidDrainQueueIndex = -1;

/** Number of possible bandwidth priorities.
*/
const TInt KUsbcDmaBufMaxPriorities = 4;

// The following buffer sizes are used within the LDD for the different
// user-selectable endpoint bandwidth priorities
// (EUsbcBandwidthOUTDefault/Plus1/Plus2/Maximum + the same for 'IN').
// These values, in particular those for the Maximum setting, were obtained
// empirically.

/** Bulk IN buffer sizes for different priorities (4K, 16K, 64K, 512K).
*/
const TInt KUsbcDmaBufSizesBulkIN[KUsbcDmaBufMaxPriorities] =
	{KUsbcDmaBufSzBulk, 0x4000, 0x10000, 0x80000};

/** Bulk OUT buffer sizes for different priorities (4K, 16K, 64K, 512K).
*/
const TInt KUsbcDmaBufSizesBulkOUT[KUsbcDmaBufMaxPriorities] =
	{KUsbcDmaBufSzBulk, 0x4000, 0x10000, 0x80000};

/** Number of UDCs supported in the system.
	(Support for more than one UDC is preliminary.)
*/
const TInt KUsbcMaxUdcs = 2;

/** Number of endpoints a USB device can have.
	(30 regular endpoints + 2 x Ep0)
*/
const TInt KUsbcEpArraySize = KUsbcMaxEndpoints + 2;

/** Number of notification requests of the same kind that can be registered at
	a time. As normally not more than one request per kind per LDD is
	permitted, this number is roughly equivalent to the maximum number of LDDs
	that can be operating at the same time.
	This constant is used by the PIL while maintaining its request lists
	(iClientCallbacks, iStatusCallbacks, iEpStatusCallbacks, iOtgCallbacks) to
	ensure that the lists are of a finite length and thus the list traverse
	time is bounded.
	This value is chosen with the maximum number of USB interfaces (not
	settings) allowed by the spec for a single device in mind.
*/
const TInt KUsbcMaxListLength = 256;

/** Used by the LDD.
*/
typedef TUint32 TUsbcPacketArray;


/** Used as a return value from DUsbClientController::EnquireEp0NextState(),
	the purpose of which is to enable the PSL to find out what the next stage
	will be for a newly received Setup packet.

	The enum values are self-explanatory.

	@publishedPartner
	@released
*/
enum TUsbcEp0State
	{
	EEp0StateDataIn,
	EEp0StateDataOut,
	EEp0StateStatusIn
	};


/** Used to show the direction of a transfer request to the Controller.

	@see TUsbcRequestCallback
*/
enum TTransferDirection {EControllerNone, EControllerRead, EControllerWrite};


/** These event codes are used by the PSL to tell the PIL what has happened.

	@publishedPartner
	@released
*/
enum TUsbcDeviceEvent
	{
	/** The USB Suspend bus state has been detected. */
	EUsbEventSuspend,
	/** USB Resume signalling has been detected. */
	EUsbEventResume,
	/** A USB Reset condition has been detected. */
	EUsbEventReset,
	/** Physical removal of the USB cable has been detected. */
	EUsbEventCableRemoved,
	/** Physical insertion of the USB cable has been detected. */
	EUsbEventCableInserted
	};


/** USB LDD client callback.
*/
class TUsbcClientCallback
    {
public:
	inline TUsbcClientCallback(DBase* aOwner, TDfcFn aCallback, TInt aPriority);
	inline DBase* Owner() const;
	inline TInt DoCallback();
	inline void Cancel();
	inline void SetDfcQ(TDfcQue* aDfcQ);
public:
	/** Used by the PIL to queue callback objects into a TSglQue. */
	TSglQueLink iLink;
private:
	DBase* iOwner;
	TDfc iDfc;
    };


/** The endpoint halt/clear_halt status.
*/
class TUsbcEndpointStatusCallback
	{
public:
	inline TUsbcEndpointStatusCallback(DBase* aOwner, TDfcFn aCallback, TInt aPriority);
	inline void SetState(TUint aState);
	inline TUint State() const;
	inline DBase* Owner() const;
	inline TInt DoCallback();
	inline void Cancel();
	inline void SetDfcQ(TDfcQue* aDfcQ);
public:
	/** Used by the PIL to queue callback objects into a TSglQue. */
	TSglQueLink iLink;
private:
	DBase* iOwner;
	TDfc iDfc;
	TUint iState;
	};


/** Maximum number of device status requests that can be queued at a time.
	The value chosen is thought to be sufficient in all situations.
*/
const TInt KUsbcDeviceStateRequests = 8;


/** The USB device status.
*/
class TUsbcStatusCallback
	{
public:
	inline TUsbcStatusCallback(DBase* aOwner, TDfcFn aCallback, TInt aPriority);
	inline void SetState(TUsbcDeviceState aState);
	inline TUsbcDeviceState State(TInt aIndex) const;
	inline void ResetState();
	inline DBase* Owner() const;
	inline TInt DoCallback();
	inline void Cancel();
	inline void SetDfcQ(TDfcQue* aDfcQ);
public:
	/** Used by the PIL to queue callback objects into a TSglQue. */
	TSglQueLink iLink;
private:
	DBase* iOwner;
	TDfc iDfc;
	TUsbcDeviceState iState[KUsbcDeviceStateRequests];
	};


/** A USB transfer request.

	@publishedPartner
	@released
*/
class TUsbcRequestCallback
	{
public:
	/** @internalTechnology */
	inline TUsbcRequestCallback(const DBase* aOwner, TInt aEndpointNum, TDfcFn aDfcFunc,
						 TAny* aEndpoint, TDfcQue* aDfcQ, TInt aPriority);
	/** @internalTechnology	*/
	inline ~TUsbcRequestCallback();
	/** @internalTechnology	*/
	inline void SetRxBufferInfo(TUint8* aBufferStart, TPhysAddr aBufferAddr,
								TUsbcPacketArray* aPacketIndex, TUsbcPacketArray* aPacketSize,
								TInt aLength);
	/** @internalTechnology	*/
	inline void SetTxBufferInfo(TUint8* aBufferStart, TPhysAddr aBufferAddr, TInt aLength);
	/** @internalTechnology	*/
	inline void SetTransferDirection(TTransferDirection aTransferDir);
	/** @internalTechnology	*/
	inline const DBase* Owner() const;
	/** @internalTechnology	*/
	inline TInt DoCallback();
	/** @internalTechnology	*/
	inline void Cancel();
public:
	/** Used by the PIL to queue callback objects into a TSglQue.
		@internalTechnology
	*/
	TSglQueLink iLink;
public:
	/** The endpoint number. */
	const TInt iEndpointNum;
	/** The 'real' endpoint number, as used by the PDD. */
	TInt iRealEpNum;
	/** Indicates the LDD client for this transfer. */
	const DBase* const iOwner;
	/** DFC, used by PIL to call back the LDD when transfer completes to the LDD. */
	TDfc iDfc;
	/** Direction of transfer request. */
	TTransferDirection iTransferDir;
	/** Start address of this buffer. */
	TUint8* iBufferStart;
	/** Physical address of buffer start (used for DMA). */
	TPhysAddr iBufferAddr;
	/** Array of pointers into iBufferStart (actually TUsbcPacketArray (*)[]). */
	TUsbcPacketArray* iPacketIndex;
	/** Array of packet sizes (actually TUsbcPacketArray (*)[]). */
	TUsbcPacketArray* iPacketSize;
	/** Length in bytes of buffer (iBufferStart). */
	TInt iLength;
	/** For IN transfers, if a zlp might be required at the end of this transfer. */
	TBool iZlpReqd;
	/** Number of bytes transmitted; changed by the PSL. */
	TUint iTxBytes;
	/** Number of packets received (if it is a read); changed by the PSL. */
	TUint iRxPackets;
	/** The error code upon completion of the request; changed by the PSL. */
	TInt iError;
	};

/** USB On-The-Go feature change callback.
*/
class TUsbcOtgFeatureCallback
    {
public:
	inline TUsbcOtgFeatureCallback(DBase* aOwner, TDfcFn aCallback, TInt aPriority);
	inline void SetFeatures(TUint8 aFeatures);
	inline TUint8 Features() const;
	inline DBase* Owner() const;
	inline TInt DoCallback();
	inline void Cancel();
	inline void SetDfcQ(TDfcQue* aDfcQ);
public:
	/** Used by the PIL to queue callback objects into a TSglQue. */
	TSglQueLink iLink;
private:
	DBase* iOwner;
	TDfc iDfc;
	TUint8 iValue;
    };

//
//########################### Physical Device Driver (PIL + PSL) ######################
//

class TUsbcLogicalEndpoint;

/** This models a physical (real) endpoint of the UDC.
*/
class TUsbcPhysicalEndpoint
	{
public:
	TUsbcPhysicalEndpoint();
	~TUsbcPhysicalEndpoint();
	TBool EndpointSuitable(const TUsbcEndpointInfo* aEpInfo, TInt aIfcNumber) const; // Check Todo, SC will pass pointer to derived class
	TInt TypeAvailable(TUint aType) const;
	TInt DirAvailable(TUint aDir) const;
public:
	/** This endpoint's capabilities. */
	TUsbcEndpointCaps iCaps;
	/** USB address: 0x00, 0x80, 0x01, 0x81, etc. */
	TUint8 iEndpointAddr;
	/** Pointer to interface # this endpoint has been assigned to. */
	const TUint8* iIfcNumber;
	/** Pointer to corresponding logical endpoint or NULL. */
	const TUsbcLogicalEndpoint* iLEndpoint;
	/** Only used when searching for available endpoints. */
	TBool iSettingReserve;
	/** True if endpoint is halted (i.e. issues STALL handshakes), false otherwise. */
	TBool iHalt;
	};


class DUsbClientController;
class TUsbcInterface;

/** This is a 'logical' endpoint, as used by our device configuration model.
*/
class TUsbcLogicalEndpoint
	{
public:
	TUsbcLogicalEndpoint(DUsbClientController* aController, TUint aEndpointNum,
						 const TUsbcEndpointInfo& aEpInfo, TUsbcInterface* aInterface,
						 TUsbcPhysicalEndpoint* aPEndpoint);		// Check Todo, SC will pass pointer to derived class
	~TUsbcLogicalEndpoint();
public:
	/** Pointer to controller object. */
	DUsbClientController* iController;
	/** The virtual (logical) endpoint number. */
	const TInt iLEndpointNum;
	/** This endpoint's info structure. */
	TUsbcEndpointInfo iInfo;										// Check Todo, SC will pass pointer to derived class
	/** Stores the endpoint size to be used for FS. */
	TInt iEpSize_Fs;
	/** Stores the endpoint size to be used for HS. */
	TInt iEpSize_Hs;
	/** 'Back' pointer. */
	const TUsbcInterface* iInterface;
	/** Pointer to corresponding physical endpoint, never NULL. */
	TUsbcPhysicalEndpoint* const iPEndpoint;
	};


class TUsbcInterfaceSet;

/** This is an 'alternate setting' of an interface.
*/
class TUsbcInterface
	{
public:
	TUsbcInterface(TUsbcInterfaceSet* aIfcSet, TUint8 aSetting, TBool aNoEp0Requests);
	~TUsbcInterface();
public:
	/** Array of endpoints making up (belonging to) this setting. */
	RPointerArray<TUsbcLogicalEndpoint> iEndpoints;
	/** 'Back' pointer. */
	TUsbcInterfaceSet* const iInterfaceSet;
	/** bAlternateSetting (zero-based). */
	const TUint8 iSettingCode;
	/** KUsbcInterfaceInfo_NoEp0RequestsPlease: stall non-std Setup requests. */
	const TBool iNoEp0Requests;
	};


/** This is an 'interface' (owning 1 or more alternate settings).

	@see TUsbcInterface
*/
class TUsbcInterfaceSet
	{
public:
	TUsbcInterfaceSet(const DBase* aClientId, TUint8 aIfcNum);
	~TUsbcInterfaceSet();
	inline const TUsbcInterface* CurrentInterface() const;
	inline TUsbcInterface* CurrentInterface();
public:
	/** Array of alternate settings provided by (belonging to) this interface. */
	RPointerArray<TUsbcInterface> iInterfaces;
	/** Pointer to the LDD which created and owns this interface. */
	const DBase* const iClientId;
	/** bInterfaceNumber (zero-based). */
	TUint8 iInterfaceNumber;
	/** bAlternateSetting (zero-based). */
	TUint8 iCurrentInterface;
	};


/** This is a 'configuration' of the USB device.
	Currently we support only one configuration.
*/
class TUsbcConfiguration
	{
public:
	TUsbcConfiguration(TUint8 aConfigVal);
	~TUsbcConfiguration();
public:
	/** Array of interfaces making up (belonging to) this configuration. */
	RPointerArray<TUsbcInterfaceSet> iInterfaceSets;
	/** bConfigurationValue (one-based). */
	const TUint8 iConfigValue;
	};


/** A USB Setup packet.

	Used mainly internally by the PIL but also by
	DUsbClientController::ProcessSetConfiguration(const TUsbcSetup&),
	which is classified as publishedPartner.

	@publishedPartner @released
*/
struct TUsbcSetup
	{
	/** bmRequestType */
	TUint8 iRequestType;
	/** bRequest */
	TUint8 iRequest;
	/** wValue */
	TUint16 iValue;
	/** wIndex */
	TUint16 iIndex;
	/** wLength */
	TUint16 iLength;
	};


/** The USB controller's power handler class.
*/
class DUsbcPowerHandler : public DPowerHandler
	{
public:
	void PowerUp();
	void PowerDown(TPowerState);
public:
	DUsbcPowerHandler(DUsbClientController* aController);
private:
	DUsbClientController* iController;
	};


/*
This is the EndpointInfo class used by the usb shared chunk client driver. 
*/

class TUsbcScEndpointInfo;


/**
Used to represent an array of (or inheriting from) TUsbcEndpointInfo objects.

@see DUsbClientController::SetInterface
*/

class TUsbcEndpointInfoArray
	{
public:
	typedef enum {EUsbcEndpointInfo, EUsbcScEndpointInfo} TArrayType;

	TUsbcEndpointInfoArray(const TUsbcEndpointInfo* aData, TInt aDataSize=0);
	TUsbcEndpointInfoArray(const TUsbcScEndpointInfo* aData, TInt aDataSize=0);
	inline TUsbcEndpointInfo& operator[](TInt aIndex) const; 

	TArrayType iType;
private:
	TUint8* iData;
	TInt iDataSize;
	};

class TUsbcRequestCallback; // todo?? required only for class below

/** The USB Device software controller class.

	Implements the platform-independent layer (PIL), and defines the interface to the
	platform-specific layer PSL).

	The implementation of the platform-specific layer interfaces with the hardware.
*/
class DUsbClientController : public DBase
	{
friend class DUsbcPowerHandler;
friend TUsbcLogicalEndpoint::~TUsbcLogicalEndpoint();
	//
	// --- Platform Independent Layer (PIL) ---
	//

public:

	//
	// --- The following functions constitute the PIL interface to the LDD ---
	//

	virtual ~DUsbClientController();
	IMPORT_C void DisableClientStack();
	IMPORT_C void EnableClientStack();
	IMPORT_C TBool IsActive();
	IMPORT_C TInt RegisterClientCallback(TUsbcClientCallback& aCallback);
	IMPORT_C static DUsbClientController* UsbcControllerPointer(TInt aUdc=0);
	IMPORT_C void EndpointCaps(const DBase* aClientId, TDes8 &aCapsBuf) const;
	IMPORT_C void DeviceCaps(const DBase* aClientId, TDes8 &aCapsBuf) const;
	IMPORT_C TInt SetInterface(const DBase* aClientId, DThread* aThread, TInt aInterfaceNum,
							   TUsbcClassInfo& aClass, TDesC8* aString, TInt aTotalEndpointsUsed,
							   const TUsbcEndpointInfo aEndpointData[], TInt (*aRealEpNumbers)[6],
							   TUint32 aFeatureWord);
	IMPORT_C TInt SetInterface(const DBase* aClientId, DThread* aThread,
												 TInt aInterfaceNum, TUsbcClassInfo& aClass,
												 TDesC8* aString, TInt aTotalEndpointsUsed,
												 const TUsbcEndpointInfoArray aEndpointData,
												 TInt aRealEpNumbers[], TUint32 aFeatureWord);
	IMPORT_C TInt ReleaseInterface(const DBase* aClientId, TInt aInterfaceNum);
	IMPORT_C TInt ReEnumerate();
	IMPORT_C TInt PowerUpUdc();
	IMPORT_C TInt UsbConnect();
	IMPORT_C TInt UsbDisconnect();
	IMPORT_C TInt RegisterForStatusChange(TUsbcStatusCallback& aCallback);
	IMPORT_C TInt DeRegisterForStatusChange(const DBase* aClientId);
	IMPORT_C TInt RegisterForEndpointStatusChange(TUsbcEndpointStatusCallback& aCallback);
	IMPORT_C TInt DeRegisterForEndpointStatusChange(const DBase* aClientId);
	IMPORT_C TInt GetInterfaceNumber(const DBase* aClientId, TInt& aInterfaceNum) const;
	IMPORT_C TInt DeRegisterClient(const DBase* aClientId);
	IMPORT_C TInt Ep0PacketSize() const;
	IMPORT_C TInt Ep0Stall(const DBase* aClientId);
	IMPORT_C void SendEp0StatusPacket(const DBase* aClientId);
	IMPORT_C TUsbcDeviceState GetDeviceStatus() const;
	IMPORT_C TEndpointState GetEndpointStatus(const DBase* aClientId, TInt aEndpointNum) const;
	IMPORT_C TInt SetupReadBuffer(TUsbcRequestCallback& aCallback);
	IMPORT_C TInt SetupWriteBuffer(TUsbcRequestCallback& aCallback);
	IMPORT_C void CancelReadBuffer(const DBase* aClientId, TInt aRealEndpoint);
	IMPORT_C void CancelWriteBuffer(const DBase* aClientId, TInt aRealEndpoint);
	IMPORT_C TInt HaltEndpoint(const DBase* aClientId, TInt aEndpointNum);
	IMPORT_C TInt ClearHaltEndpoint(const DBase* aClientId, TInt aEndpointNum);
	IMPORT_C TInt SetDeviceControl(const DBase* aClientId);
	IMPORT_C TInt ReleaseDeviceControl(const DBase* aClientId);
	IMPORT_C TUint EndpointZeroMaxPacketSizes() const;
	IMPORT_C TInt SetEndpointZeroMaxPacketSize(TInt aMaxPacketSize);
	IMPORT_C TInt GetDeviceDescriptor(DThread* aThread, TDes8& aDeviceDescriptor);
	IMPORT_C TInt SetDeviceDescriptor(DThread* aThread, const TDes8& aDeviceDescriptor);
	IMPORT_C TInt GetDeviceDescriptorSize(DThread* aThread, TDes8& aSize);
	IMPORT_C TInt GetConfigurationDescriptor(DThread* aThread, TDes8& aConfigurationDescriptor);
	IMPORT_C TInt SetConfigurationDescriptor(DThread* aThread, const TDes8& aConfigurationDescriptor);
	IMPORT_C TInt GetConfigurationDescriptorSize(DThread* aThread, TDes8& aSize);
	IMPORT_C TInt SetOtgDescriptor(DThread* aThread, const TDesC8& aOtgDesc);
	IMPORT_C TInt GetOtgDescriptor(DThread* aThread, TDes8& aOtgDesc) const;
	IMPORT_C TInt GetOtgFeatures(DThread* aThread, TDes8& aFeatures) const;
	IMPORT_C TInt GetCurrentOtgFeatures(TUint8& aFeatures) const;
	IMPORT_C TInt RegisterForOtgFeatureChange(TUsbcOtgFeatureCallback& aCallback);
	IMPORT_C TInt DeRegisterForOtgFeatureChange(const DBase* aClientId);
	IMPORT_C TInt GetInterfaceDescriptor(DThread* aThread, const DBase* aClientId, TInt aSettingNum,
										 TDes8& aInterfaceDescriptor);
	IMPORT_C TInt SetInterfaceDescriptor(DThread* aThread, const DBase* aClientId, TInt aSettingNum,
										 const TDes8& aInterfaceDescriptor);
	IMPORT_C TInt GetInterfaceDescriptorSize(DThread* aThread, const DBase* aClientId, TInt aSettingNum,
											 TDes8& aSize);
	IMPORT_C TInt GetEndpointDescriptor(DThread* aThread, const DBase* aClientId, TInt aSettingNum,
										TInt aEndpointNum, TDes8& aEndpointDescriptor);
	IMPORT_C TInt SetEndpointDescriptor(DThread* aThread, const DBase* aClientId, TInt aSettingNum,
										TInt aEndpointNum, const TDes8& aEndpointDescriptor);
	IMPORT_C TInt GetEndpointDescriptorSize(DThread* aThread, const DBase* aClientId, TInt aSettingNum,
											TInt aEndpointNum, TDes8& aSize);
	IMPORT_C TInt GetDeviceQualifierDescriptor(DThread* aThread, TDes8& aDeviceQualifierDescriptor);
	IMPORT_C TInt SetDeviceQualifierDescriptor(DThread* aThread, const TDes8& aDeviceQualifierDescriptor);
	IMPORT_C TInt GetOtherSpeedConfigurationDescriptor(DThread* aThread, TDes8& aConfigurationDescriptor);
	IMPORT_C TInt SetOtherSpeedConfigurationDescriptor(DThread* aThread, const TDes8& aConfigurationDescriptor);
	IMPORT_C TInt GetCSInterfaceDescriptorBlock(DThread* aThread, const DBase* aClientId, TInt aSettingNum,
												TDes8& aInterfaceDescriptor);
	IMPORT_C TInt SetCSInterfaceDescriptorBlock(DThread* aThread, const DBase* aClientId, TInt aSettingNum,
												const TDes8& aInterfaceDescriptor, TInt aSize);
	IMPORT_C TInt GetCSInterfaceDescriptorBlockSize(DThread* aThread, const DBase* aClientId,
													TInt aSettingNum, TDes8& aSize);
	IMPORT_C TInt GetCSEndpointDescriptorBlock(DThread* aThread, const DBase* aClientId, TInt aSettingNum,
											   TInt aEndpointNum, TDes8& aEndpointDescriptor);
	IMPORT_C TInt SetCSEndpointDescriptorBlock(DThread* aThread, const DBase* aClientId, TInt aSettingNum,
											   TInt aEndpointNum, const TDes8& aEndpointDescriptor,
											   TInt aSize);
	IMPORT_C TInt GetCSEndpointDescriptorBlockSize(DThread* aThread, const DBase* aClientId, TInt aSettingNum,
												   TInt aEndpointNum, TDes8& aSize);
	IMPORT_C TInt GetStringDescriptorLangId(DThread* aThread, TDes8& aLangId);
	IMPORT_C TInt SetStringDescriptorLangId(TUint16 aLangId);
	IMPORT_C TInt GetManufacturerStringDescriptor(DThread* aThread, TDes8& aString);
	IMPORT_C TInt SetManufacturerStringDescriptor(DThread* aThread, const TDes8& aString);
	IMPORT_C TInt RemoveManufacturerStringDescriptor();
	IMPORT_C TInt GetProductStringDescriptor(DThread* aThread, TDes8& aString);
	IMPORT_C TInt SetProductStringDescriptor(DThread* aThread, const TDes8& aString);
	IMPORT_C TInt RemoveProductStringDescriptor();
	IMPORT_C TInt GetSerialNumberStringDescriptor(DThread* aThread, TDes8& aString);
	IMPORT_C TInt SetSerialNumberStringDescriptor(DThread* aThread, const TDes8& aString);
	IMPORT_C TInt RemoveSerialNumberStringDescriptor();
	IMPORT_C TInt GetConfigurationStringDescriptor(DThread* aThread, TDes8& aString);
	IMPORT_C TInt SetConfigurationStringDescriptor(DThread* aThread, const TDes8& aString);
	IMPORT_C TInt RemoveConfigurationStringDescriptor();
	IMPORT_C TInt GetStringDescriptor(DThread* aThread, TUint8 aIndex, TDes8& aString);
	IMPORT_C TInt SetStringDescriptor(DThread* aThread, TUint8 aIndex, const TDes8& aString);
	IMPORT_C TInt RemoveStringDescriptor(TUint8 aIndex);
	IMPORT_C TInt AllocateEndpointResource(const DBase* aClientId, TInt aEndpointNum,
										   TUsbcEndpointResource aResource);
	IMPORT_C TInt DeAllocateEndpointResource(const DBase* aClientId, TInt aEndpointNum,
											 TUsbcEndpointResource aResource);
	IMPORT_C TBool QueryEndpointResource(const DBase* aClientId, TInt aEndpointNum,
										 TUsbcEndpointResource aResource);
	IMPORT_C TInt EndpointPacketSize(const DBase* aClientId, TInt aEndpointNum);

	//
	// --- Public (pure) virtual (implemented by PSL, used by LDD) ---
	//

	/** Forces the UDC into a non-idle state to perform a USB remote wakeup operation.

		The PSL should first check the current value of iRmWakeupStatus_Enabled
		to determine whether or not to actually send a Remote Wakeup.

		@see iRmWakeupStatus_Enabled

		@return KErrGeneral if Remote Wakeup feature is not enabled or an error is encountered,
		KErrNone otherwise.

		@publishedPartner @released
	*/
	IMPORT_C virtual TInt SignalRemoteWakeup() =0;

	/** Dumps the contents of (all or part of) the UDC registers to the serial console.
		This is for debugging purposes only.

		@publishedPartner @released
	*/
	IMPORT_C virtual void DumpRegisters() =0;

	/** Returns a pointer to the DFC queue that should be used by the USB LDD.

		@return A pointer to the DFC queue that should be used by the USB LDD.

		@publishedPartner @released
	*/
	IMPORT_C virtual TDfcQue* DfcQ(TInt aUnit) =0;

	/** Returns information about the current operating speed of the UDC.

		(Function is not 'pure virtual' for backwards compatibility with existing USB PSLs.
		 The default implementation in the PIL returns EFalse.)

		@return ETrue if the UDC is currently operating at High speed, EFalse
		otherwise (i.e. controller is operating at Full speed).

		@publishedPartner @released
	*/
	IMPORT_C virtual TBool CurrentlyUsingHighSpeed();

	//
	// --- Public PIL functions ---
	//

	DUsbClientController* RegisterUdc(TInt aUdc);

protected:

	//
	// --- Functions and data members provided by PIL, called by PSL ---
	//

	TBool InitialiseBaseClass(TUsbcDeviceDescriptor* aDeviceDesc,
							  TUsbcConfigDescriptor* aConfigDesc,
							  TUsbcLangIdDescriptor* aLangId,
							  TUsbcStringDescriptor* aManufacturer =0,
							  TUsbcStringDescriptor* aProduct =0,
							  TUsbcStringDescriptor* aSerialNum =0,
							  TUsbcStringDescriptor* aConfig =0,
							  TUsbcOtgDescriptor* aOtgDesc =0);
	DUsbClientController();
	TInt DeviceEventNotification(TUsbcDeviceEvent aEvent);
	void EndpointRequestComplete(TUsbcRequestCallback* aCallback);
	TInt Ep0RequestComplete(TInt aRealEndpoint, TInt aCount, TInt aError);
	void MoveToAddressState();
	void SetCurrent(TInt aCurrent);
	TUsbcEp0State EnquireEp0NextState(const TUint8* aSetupBuf) const;
	TInt ProcessSetConfiguration(const TUsbcSetup& aPacket);
	void HandleHnpRequest(TInt aHnpState);

	/** This info can be used by the PSL before sending ZLPs.

		@publishedPartner @released
	*/
	TBool iEp0ReceivedNonStdRequest;

	/** True if RMW is currently enabled (set by either PIL or PSL).

		@publishedPartner @released
	*/
	TBool iRmWakeupStatus_Enabled;

	/** Ep0 incoming (rx) data is placed here (one packet).

		@publishedPartner @released
	*/
	TUint8 iEp0_RxBuf[KUsbcBufSzControl];

private:

	//
	// --- Platform Specific Layer (PSL) ---
	//

	/** This function will be called by the PIL upon decoding a SET_ADDRESS request.

		UDCs which require a manual setting of the USB device address should do that in this function.

		@param aAddress A valid USB device address that was received with the SET_ADDRESS request.

		@return KErrNone if address was set successfully or if this UDC's address cannot be set manually,
		KErrGeneral otherwise.

		@publishedPartner @released
	*/
	virtual TInt SetDeviceAddress(TInt aAddress) =0;

	/** Configures (enables) an endpoint (incl. Ep0) for data transmission or reception.

		@param aRealEndpoint The number of the endpoint to be enabled.
		@param aEndpointInfo A reference to a properly filled-in endpoint info structure.

		@return KErrArgument if endpoint number or endpoint info invalid, KErrNone if endpoint
		successfully configured, KErrGeneral otherwise.

		@publishedPartner @released
	*/
	virtual TInt ConfigureEndpoint(TInt aRealEndpoint, const TUsbcEndpointInfo& aEndpointInfo) =0;

	/** De-configures (disables) an endpoint (incl. Ep0).

		@param aRealEndpoint The number of the endpoint to be disabled.

		@return KErrArgument if endpoint number invalid, KErrNone if endpoint successfully de-configured,
		KErrGeneral otherwise.

		@publishedPartner @released
	*/
	virtual TInt DeConfigureEndpoint(TInt aRealEndpoint) =0;

	/** Allocates an endpoint resource.

		If the resource gets successfully allocated, it will be used from when the current bus transfer
		has been completed.

		@param aRealEndpoint The number of the endpoint.
		@param aResource The endpoint resource to be allocated.

		@return KErrNone if the resource has been successfully allocated, KErrNotSupported if the endpoint
		does not support the resource requested, and KErrInUse if the resource is already consumed and
		cannot be allocated.

		@publishedPartner @deprecated
	*/
	virtual TInt AllocateEndpointResource(TInt aRealEndpoint, TUsbcEndpointResource aResource) =0;

	/** Deallocates (frees) an endpoint resource.

		The resource will be removed from when the current bus transfer has been completed.

		@param aRealEndpoint The number of the endpoint.
		@param aResource The endpoint resource to be deallocated.

		@return KErrNone if the resource has been successfully deallocated, KErrNotSupported if the endpoint
		does not support the resource requested.

		@publishedPartner @deprecated
	*/
	virtual TInt DeAllocateEndpointResource(TInt aRealEndpoint, TUsbcEndpointResource aResource) =0;

	/** Queries the use of and endpoint resource.

		@param aRealEndpoint The number of the endpoint.
		@param aResource The endpoint resource to be queried.

		@return ETrue if the specified resource is in use at the endpoint, EFalse if not.

		@publishedPartner @released
	*/
	virtual TBool QueryEndpointResource(TInt aRealEndpoint, TUsbcEndpointResource aResource) const =0;

	/** Opens a DMA channel (if possible).

		@param aRealEndpoint The number of the endpoint for which to open the DMA channel.

		@return KErrArgument if endpoint number invalid, KErrNone if channel successfully opened or if
		endpoint not DMA-capable, KErrGeneral otherwise.

		@publishedPartner @released
	*/
	virtual TInt OpenDmaChannel(TInt aRealEndpoint);

	/** Closes a DMA channel (if possible).

		@param aRealEndpoint The number of the endpoint for which to close the DMA channel.

		@publishedPartner @released
	*/
	virtual void CloseDmaChannel(TInt aRealEndpoint);

	/** Sets up a read request on an endpoint (excl. Ep0) for data reception.

		For endpoint 0 read requests, SetupEndpointZeroRead() is used instead.

		@param aRealEndpoint The number of the endpoint to be used.
		@param aCallback A reference to a properly filled-in request callback structure.

		@return KErrArgument if endpoint number invalid, KErrNone if read successfully set up,
		KErrGeneral otherwise.

		@publishedPartner @released
	*/
	virtual TInt SetupEndpointRead(TInt aRealEndpoint, TUsbcRequestCallback& aCallback) =0;

	/** Sets up a write request on an endpoint (excl. Ep0) for data transmission.

		For endpoint 0 write requests, SetupEndpointZeroWrite() is used instead.

		@param aRealEndpoint The number of the endpoint to be used.
		@param aCallback A reference to a properly filled-in request callback structure.

		@return KErrArgument if endpoint number invalid, KErrNone if write successfully set up,
		KErrGeneral otherwise.

		@publishedPartner @released
	*/
	virtual TInt SetupEndpointWrite(TInt aRealEndpoint, TUsbcRequestCallback& aCallback) =0;

	/** Cancels a read request on an endpoint (excl. Ep0).

		Note that endpoint 0 read requests are never cancelled by the PIL, so
		there is also no CancelEndpointZeroRead() function.

		@param aRealEndpoint The number of the endpoint to be used.

		@return KErrArgument if endpoint number invalid, KErrNone if read successfully cancelled,
		KErrGeneral otherwise.

		@publishedPartner @released
	*/
	virtual TInt CancelEndpointRead(TInt aRealEndpoint) =0;

	/** Cancels a write request on an endpoint (incl. Ep0).

		The PIL calls this function also to cancel endpoint zero write requests.

		@param aRealEndpoint The number of the endpoint to be used.

		@return KErrArgument if endpoint number invalid, KErrNone if write successfully cancelled,
		KErrGeneral otherwise.

		@publishedPartner @released
	*/
	virtual TInt CancelEndpointWrite(TInt aRealEndpoint) =0;

	/** Same as SetupEndpointRead(), but for endpoint zero only.

		No callback is used here as this function is only used internally by the PIL and no user side request
		exists for it. The data buffer to be used (filled) is iEp0_RxBuf.

		@return KErrGeneral if (&iEndpoints[KEp0_Out]->iRxBuf != NULL) or some other error occurs,
		KErrNone if read successfully set up.

		@publishedPartner @released
	*/
	virtual TInt SetupEndpointZeroRead() =0;

	/** Same as SetupEndpointWrite(), but for endpoint zero only.

		No callback is used here as this function is only used internally by the PIL and no user side request
		exists for it.

		@param aBuffer This points to the beginning of the data to be sent.
		@param aLength The number of bytes to be sent.
		@param aZlpReqd ETrue if a zero-length packet (ZLP) is to be sent after the data.

		@return KErrGeneral if (&iEndpoints[KEp0_In]->iTxBuf != NULL) or some other error occurs,
		KErrNone if write successfully set up.

		@publishedPartner @released
	*/
	virtual TInt SetupEndpointZeroWrite(const TUint8* aBuffer, TInt aLength, TBool aZlpReqd=EFalse) =0;

	/** Sets up on Ep0 the transmission of a single zero-length packet (ZLP).

		@return KErrNone if ZLP successfully set up, KErrGeneral otherwise..

		@publishedPartner @released
	*/
	virtual TInt SendEp0ZeroByteStatusPacket() =0;

	/** Stalls an endpoint (incl. Ep0).

		Isochronous endpoints cannot be stalled.

		@param aRealEndpoint The number of the endpoint to be stalled.

		@return KErrArgument if endpoint number invalid, KErrNone if endpoint successfully stalled,
		KErrGeneral otherwise.

		@publishedPartner @released
	*/
	virtual TInt StallEndpoint(TInt aRealEndpoint) =0;

	/** Clears the stall condition on an endpoint (incl. Ep0).

		Isochronous endpoints cannot be stalled.

		@param aRealEndpoint The number of the endpoint to be stalled.

		@return KErrArgument if endpoint number invalid, KErrNone if endpoint successfully de-stalled,
		KErrGeneral otherwise.

		@publishedPartner @released
	*/
	virtual TInt ClearStallEndpoint(TInt aRealEndpoint) =0;

	/** Returns the stall status of an endpoint (incl. Ep0).

		Isochronous endpoints cannot be stalled.

		@param aRealEndpoint The number of the endpoint to be used.

		@return KErrArgument if endpoint number invalid, 1 if endpoint is currently stalled, 0 if not.

		@publishedPartner @released
	*/
	virtual TInt EndpointStallStatus(TInt aRealEndpoint) const =0;

	/** Returns the error status of an endpoint (incl. Ep0).

		@param aRealEndpoint The number of the endpoint to be used.

		@return KErrArgument if endpoint number invalid, KErrNone if no error at this endpoint,
		KErrGeneral if there is an error.

		@publishedPartner @released
	*/
	virtual TInt EndpointErrorStatus(TInt aRealEndpoint) const =0;

	/** Resets the data toggle bit for an endpoint (incl. Ep0).

		Isochronous endpoints don't use data toggles.

		@param aRealEndpoint The number of the endpoint to be used.

		@return KErrArgument if endpoint number invalid, KErrNone if data toggle successfully reset,
		KErrGeneral otherwise.

		@publishedPartner @released
	*/
	virtual TInt ResetDataToggle(TInt aRealEndpoint) =0;

	/** Returns the (11-bit) frame number contained in the last received SOF packet.

		This information is used for isochronous transfers.

		@return The (11-bit) frame number contained in the last received SOF packet.

		@publishedPartner @released
	*/
	virtual TInt SynchFrameNumber() const =0;

	/** Stores the (11-bit) frame number that should be sent in response to the next SYNCH_FRAME request(s).

		@publishedPartner @released
	 */
	virtual void SetSynchFrameNumber(TInt aFrameNumber) =0;

	/** Starts the UDC.

		This initializes the device controller hardware before any other operation can be
		performed. Tasks to be carried out here might include
		- resetting the whole UDC design
		- enabling the UDC's clock
		- binding & enabling the UDC (primary) interrupt
		- write meaningful values to some general UDC registers
		- enabling the USB Reset interrupt
		- enabling the UDC proper (for instance by setting an Enable bit)

		@return KErrNone if UDC successfully started, KErrGeneral if there was an error.

		@publishedPartner @released
	*/
	virtual TInt StartUdc() =0;

	/** Stops the UDC.

		This basically makes undone what happened in StartUdc(). Tasks to be carried out
		here might include:
		- disabling the UDC proper (for instance by clearing an Enable bit)
		- disabling the USB Reset interrupt
		- disabling & unbinding the UDC (primary) interrupt
		- disabling the UDC's clock

		@return KErrNone if UDC successfully stopped, KErrGeneral if there was an error.

		@publishedPartner @released
	*/
	virtual TInt StopUdc() =0;

	/** Connects the UDC  - and thus the USB device - physically to the bus.

		This might involve a call into the Variant DLL, as the mechanism to achieve the connection can be
		specific to the platform (rather than UDC specific). Since this functionality is not part of the USB
		specification it has to be explicitly supported, either by the UDC itself or by the hardware
		platform. If it is supported, then the member function SoftConnectCaps should be implemented to return
		ETrue.

		@see SoftConnectCaps()

		@return KErrNone if UDC successfully connected, KErrGeneral if there was an error.

		@publishedPartner @released
	*/
	virtual TInt UdcConnect() =0;

	/** Disconnects the UDC	 - and thus the USB device - physically from the bus.

		This might involve a call into the Variant DLL, as the mechanism to achieve the disconnection can be
		specific to the platform (rather than UDC specific). Since this functionality is not part of the USB
		specification it has to be explicitly supported, either by the UDC itself or by the hardware
		platform. If it is supported, then the member function SoftConnectCaps should be implemented to return
		ETrue.

		@see SoftConnectCaps()

		@return KErrNone if UDC successfully disconnected, KErrGeneral if there was an error.

		@publishedPartner @released
	*/
	virtual TInt UdcDisconnect() =0;

	/** Returns the USB cable connection status.

		@return ETrue if the device is connected (via the USB cable) to a USB host, EFalse if not.

		@publishedPartner @released
	*/
	virtual TBool UsbConnectionStatus() const  =0;

	/** Returns a truth value showing if the VBUS line is powered or not.

		Lack of power may indicate an unpowered host or upstream hub, or a disconnected cable.

		@return ETrue if VBUS is powered, EFalse otherwise.

		@publishedPartner @released
	*/
	virtual TBool UsbPowerStatus() const =0;

	/** Returns the current power status of the USB device.

		@return ETrue if the device is currently self-powered, EFalse if not.

		@publishedPartner @released
	*/
	virtual TBool DeviceSelfPowered() const =0;

	/** Returns a pointer to an array of TUsbcEndpointCaps structures, which describe the endpoint
		capabilities of this UDC.

		The dimension of the array can be obtained by calling the member function DeviceTotalEndpoints().
		Note that there might be gaps in the array, as the endpoints are numbered using the 'real endpoint'
		numbering scheme. Here is how an array could look like:

		@code
		static const TInt KUsbTotalEndpoints = 5;
		static const TUsbcEndpointCaps DeviceEndpoints[KUsbTotalEndpoints] =
		{
		//															 UDC #	  iEndpoints index
		{KEp0MaxPktSzMask,	(KUsbEpTypeControl	   | KUsbEpDirOut)}, //	 0 -  0
		{KEp0MaxPktSzMask,	(KUsbEpTypeControl	   | KUsbEpDirIn )}, //	 0 -  1
		{KUsbEpNotAvailable, KUsbEpNotAvailable},					 // --- Not present
		{KBlkMaxPktSzMask,	(KUsbEpTypeBulk		   | KUsbEpDirIn )}, //	 1 -  3
		{KBlkMaxPktSzMask,	(KUsbEpTypeBulk		   | KUsbEpDirOut)}	 //	 2 -  4
		};
		@endcode

		For the endpoint max packet sizes on a USB 2.0 High-speed UDC, the PSL should provide
		the overlaid values for both FS and HS, as the PIL can deduce the appropriate values
		for either speed.

		@see TUsbcEndpointCaps
		@see DeviceTotalEndpoints()

		@return A pointer to an array of TUsbcEndpointCaps structures, which describe the endpoint
		capabilities of this UDC.

		@publishedPartner @released
	*/
	virtual const TUsbcEndpointCaps* DeviceEndpointCaps() const =0;

	/** Returns the number of elements in the array pointed to by the return value of DeviceEndpointCaps().

		Note that this is not necessarily equal to the number of usable endpoints. In the example to the
		DeviceEndpointCaps() function, this value would be 5 even though there are only 4 endpoints.

		@see DeviceEndpointCaps()

		@return The number of elements in the array pointed to by the return value of DeviceEndpointCaps().

		@publishedPartner @released
	*/
	virtual TInt DeviceTotalEndpoints() const =0;

	/** Returns a truth value indicating whether or not this UDC or platform has the capability to disconnect
		and re-connect the USB D+ line under software control.

		@see UdcConnect()
		@see UdcDisconnect()

		@return ETrue if software connect/disconnect is supported, EFalse otherwise.

		@publishedPartner @released
	*/
	virtual TBool SoftConnectCaps() const =0;

	/** Returns a truth value indicating whether or not this UDC allows the accurate tracking of the USB
		device state.

		This capability affects how device state change notifications to the LDD/user are being handled.

		@return ETrue if this UDC allows the tracking of the USB device state, EFalse otherwise.

		@publishedPartner @released
	*/
	virtual TBool DeviceStateChangeCaps() const =0;

	/** Returns a truth value indicating whether the USB device controller (UDC) hardware supports
		detection of a plugged-in USB cable even when not powered.

		This capability affects the power management strategy used by the USB Manager.

		(Function is not 'pure virtual' for backwards compatibility with existing USB PSLs.
		 The default implementation in the PIL returns EFalse.)

		@return ETrue if this UDC supports USB cable detection when not powered, EFalse otherwise.

		@publishedPartner @released
	*/
	virtual TBool CableDetectWithoutPowerCaps() const;

	/** Returns a truth value indicating whether the USB device controller (UDC) hardware supports
		USB High-speed operation.

		This capability affects driver functionality and behaviour throughout the implementation.

		(Function is not 'pure virtual' for backwards compatibility with existing USB PSLs.
		 The default implementation in the PIL returns EFalse.)

		@return ETrue if this UDC supports USB High-speed operation, EFalse otherwise.

		@publishedPartner @released
	*/
	virtual TBool DeviceHighSpeedCaps() const;

	/** Returns a truth value indicating whether this PSL supports the new
		('V2') endpoint resource request scheme.

		This capability can be queried from the user-side and may determine the
		way the USB application issues resource allocation requests.

		(Function is not 'pure virtual' for backwards compatibility with existing USB PSLs.
		 The default implementation in the PIL returns EFalse.)

		@return ETrue if PSL supports the new endpoint resource request scheme,
		EFalse otherwise.

		@publishedPartner @released
	*/
	virtual TBool DeviceResourceAllocV2Caps() const;

	/** Returns a truth value indicating whether this UDC handles OTG HNP bus
		connects/disconnects automatically in hardware.

		This capability will be queried by the PIL and determines the way the
		PIL calls the functions behind the
		iEnablePullUpOnDPlus / iDisablePullUpOnDPlus pointers.

		If HNP is handled by hardware (TBool = ETrue) then the PIL will (in
		that order)

		1) make calls to those function pointers dependent only on the
		readiness or otherwise of user-side USB Client support (i.e. the Client
		LDD calls DeviceConnectToHost() / DeviceDisconnectFromHost()), as
		opposed to also evaluating Client PDD EnableClientStack() /
		DisableClientStack() calls from the OTG driver.

		2) delay its USB Reset processing incl. the notification of upper
		layers (LDD + user-side), plus the initial setting up of an Ep0 read
		until user-side USB Client support readiness has been signalled
		(i.e. until after a call to DeviceConnectToHost()).

		(Function is not 'pure virtual' for backwards compatibility with
		 existing USB PSLs. The default implementation in the PIL returns EFalse.)

		@return ETrue if UDC/PSL handles HNP connects/disconnects in hardware,
		EFalse otherwise.

		@publishedPartner @released
	*/
	virtual TBool DeviceHnpHandledByHardwareCaps() const;

	/** Implements anything the UDC (PSL) might require following bus Suspend signalling.

		This function gets called by the PIL after it has been notified (by the PSL) about the Suspend
		condition.

		@publishedPartner @released
	*/
	virtual void Suspend() =0;

	/** Implements anything the UDC (PSL) might require following bus Resume signalling.

		This function gets called by the PIL after it has been notified (by the PSL) about the Resume event.

		@publishedPartner @released
	*/
	virtual void Resume() =0;

	/** Implements anything the UDC (PSL) might require following bus Reset signalling.

		This function gets called by the PIL after it has been notified (by the PSL) about the Reset event.

		@publishedPartner @released
	*/
	virtual void Reset() =0;

	/** Called by the PIL to signal to the PSL that it has finished processing a received Setup packet (on
		Ep0) and that the PSL can now prepare itself for the next Ep0 reception (for instance by re-enabling
		the Ep0 interrupt).

		The reason for having this function is the situation where no Ep0 read has been set up by the user and
		thus a received Setup packet cannot immediately be delivered to the user. Once the user however sets
		up an Ep0 read, the PIL completes the request and eventually calls this function. This way we can
		implement some sort of flow-control.

		@publishedPartner @released
	*/
	virtual void Ep0ReadSetupPktProceed() =0;

	/** Called by the PIL to signal to the PSL that it has finished processing a received Ep0 data packet and
		that the PSL can now prepare itself for the next Ep0 reception (for instance by re-enabling the Ep0
		interrupt).

		The reason for having this function is the situation where no Ep0 read has been set up by the user and
		thus a received packet cannot immediately be delivered to the user. Once the user however sets up an
		Ep0 read, the PIL completes the request and eventually calls this function. This way we can implement
		some sort of flow-control.

		@publishedPartner @released
	*/
	virtual void Ep0ReceiveProceed() =0;

	/** Returns a truth value indicating whether the USB controller hardware (UDC) supports being powered
		down while (a configuration is) active.

		This capability affects the power management strategy used by the USB Manager.

		(Function is not 'pure virtual' for backwards compatibility with existing USB PSLs. The default
		implementation in the PIL - to be on the safe side - returns EFalse.)

		@return ETrue if this UDC supports power-down while active, EFalse otherwise.

		@publishedPartner @released
	*/
	virtual TBool PowerDownWhenActive() const;

	/** Powers the UDC down, i.e. puts it into a (hardware-dependent) power-saving mode. Note that this
		function is not the same as StopUdc(). The difference is that while StopUdc() effectively turns the
		UDC off and so may invalidate all its settings, after a call to PowerDown() the UDC is expected to
		return to its previous state when PowerUp() is called. This function is also not the same as
		Suspend() which gets called by the PIL in response to a Suspend event on the bus, and only then
		(but apart from that the two functions are very similar).

		This function won't be called by the PIL once the UDC is active if PowerDownWhenActive() returns
		EFalse (which it by default does).

		(Function is not 'pure virtual' for backwards compatibility with existing USB PSLs. The default
		implementation in the PIL does nothing.)

		@see PowerUp()
		@see PowerDownWhenActive()

		@return KErrNone if UDC was successfully powered down, KErrGeneral otherwise.

		@publishedPartner @released
	*/
	virtual TInt PowerDown();

	/** Powers the UDC up by exiting a (hardware-dependent) power-saving mode. Note that this function is not
		the same as StartUdc(). The difference is that while StartUdc() starts the UDC from zero and merely
		leads to the default state (i.e. not an active configuration), after a call to PowerUp() the UDC is
		expected to have returned to the state it was in before PowerDown() was called.

		(Function is not 'pure virtual' for backwards compatibility with existing USB PSLs. The default
		implementation in the PIL does nothing.)

		@see PowerDown()

		@return KErrNone if UDC was successfully powered up, KErrGeneral otherwise.

		@publishedPartner @released
	*/
	virtual TInt PowerUp();

	/** Puts the controller into a specific test mode (during HS operation only).

		9.4.9 Set Feature: "The transition to test mode must be complete no later than 3 ms after the
		completion of the status stage of the request." (The status stage will have been completed
		immediately before this function gets called.)

		(Function is not 'pure virtual' for backwards compatibility with existing USB PSLs.
		 The default implementation in the PIL returns KErrNotSupported.)

		@param aTestSelector The specific test mode selector (@see usb.h).

		@return KErrNone if the specified test mode was entered successfully,
		KErrGeneral otherwise.

		@publishedPartner @released
	*/
	virtual TInt EnterTestMode(TInt aTestSelector);

	/** Turn on USB client functionality in an OTG/Host setup.

		This PSL function is called by the PIL when the OTG stack calls the PIL
		function EnableClientStack(). Immediately afterwards the PIL may
		connect the B-device to the bus (via the OTG stack). OtgEnableUdc() is
		called always after StartUdc().

		There is no equivalent to this function in the non-OTG version of the
		USB PDD.

		@return KErrNone if UDC successfully enabled, KErrGeneral otherwise.

		@publishedPartner @released
	*/
	virtual TInt OtgEnableUdc();

	/** Turn off USB client functionality in an OTG/Host setup.

		This function is called by the PIL when the OTG stack calls the PIL
		function DisableClientStack(); the PIL will do this immediately after
		it has disconnected the B-device from the bus (via the OTG stack) and
		before calling StopUdc().

		There is no equivalent to this function in the non-OTG version of the
		USB PDD.

		@return KErrNone if UDC successfully disabled, KErrGeneral otherwise.

		@publishedPartner @released
	*/
	virtual TInt OtgDisableUdc();

private:

	//
	// --- Private member functions (used by PIL) ---
	//

	TInt DeRegisterClientCallback(const DBase* aClientId);
	TBool CheckEpAvailability(TInt aEndpointsUsed, const TUsbcEndpointInfoArray& aEndpointData, TInt aIfcNumber)
		const;
	TUsbcInterface* CreateInterface(const DBase* aClientId, TInt aIfc, TUint32 aFeatureWord);
	TInt CreateEndpoints(TUsbcInterface* aIfc, TInt aEndpointsUsed, const TUsbcEndpointInfoArray& aEndpointData,
						 TInt *aRealEpNumbers);
	TInt SetupIfcDescriptor(TUsbcInterface* aIfc, TUsbcClassInfo& aClass, DThread* aThread, TDesC8* aString,
							const TUsbcEndpointInfoArray& aEndpointData);
	TInt ClientId2InterfaceNumber(const DBase* aClientId) const;
	TUsbcInterfaceSet* ClientId2InterfacePointer(const DBase* aClientId) const;
	const DBase* InterfaceNumber2ClientId(TInt aIfcSet) const;
	TUsbcInterfaceSet* InterfaceNumber2InterfacePointer(TInt aIfcSet) const;
	inline const DBase* PEndpoint2ClientId(TInt aRealEndpoint) const;
	inline TInt PEndpoint2LEndpoint(TInt aRealEndpoint) const;
	TInt ActivateHardwareController();
	void DeActivateHardwareController();
	void DeleteInterfaceSet(TInt aIfcSet);
	void DeleteInterface(TInt aIfcSet, TInt aIfc);
	void CancelTransferRequests(TInt aRealEndpoint);
	void DeleteRequestCallback(const DBase* aClientId, TInt aEndpointNum, TTransferDirection aTransferDir);
	void DeleteRequestCallbacks(const DBase* aClientId);
	void StatusNotify(TUsbcDeviceState aState, const DBase* aClientId=NULL);
	void EpStatusNotify(TInt aRealEndpoint);
	void OtgFeaturesNotify();
	void RunClientCallbacks();
	void ProcessDataTransferDone(TUsbcRequestCallback& aRcb);
	void NextDeviceState(TUsbcDeviceState aNextState);
	TInt ProcessSuspendEvent();
	TInt ProcessSuspendEventProceed();
	TInt ProcessResumeEvent();
	TInt ProcessResetEvent(TBool aPslUpcall=ETrue);
	TInt ProcessCableInsertEvent();
	TInt ProcessCableRemoveEvent();
	TInt ProcessEp0ReceiveDone(TInt aCount);
	TInt ProcessEp0TransmitDone(TInt aCount, TInt aError);
	TInt ProcessEp0SetupReceived(TInt aCount);
	TInt ProcessEp0DataReceived(TInt aCount);
	TInt ProcessGetDeviceStatus(const TUsbcSetup& aPacket);
	TInt ProcessGetInterfaceStatus(const TUsbcSetup& aPacket);
	TInt ProcessGetEndpointStatus(const TUsbcSetup& aPacket);
	TInt ProcessSetClearDevFeature(const TUsbcSetup& aPacket);
	TInt ProcessSetClearIfcFeature(const TUsbcSetup& aPacket);
	TInt ProcessSetClearEpFeature(const TUsbcSetup& aPacket);
	TInt ProcessSetAddress(const TUsbcSetup& aPacket);
	TInt ProcessGetDescriptor(const TUsbcSetup& aPacket);
	TInt ProcessSetDescriptor(const TUsbcSetup& aPacket);
	TInt ProcessGetConfiguration(const TUsbcSetup& aPacket);
	TInt ProcessGetInterface(const TUsbcSetup& aPacket);
	TInt ProcessSetInterface(const TUsbcSetup& aPacket);
	TInt ProcessSynchFrame(const TUsbcSetup& aPacket);
	void ProceedSetDescriptor();
	void SetClearHaltFeature(TInt aRealEndpoint, TUint8 aRequest);
	TInt ClearHaltFeature(TInt aRealEndpoint);
	void ChangeConfiguration(TUint16 aValue);
	void InterfaceSetup(TUsbcInterface* aIfc);
	void InterfaceSetTeardown(TUsbcInterfaceSet* aIfc);
	void ChangeInterface(TUsbcInterface* aIfc);
	TInt DoForEveryEndpointInUse(TInt (DUsbClientController::*aFunction)(TInt), TInt& aCount);
	void EnterFullSpeed();
	void EnterHighSpeed();
	TInt EvaluateOtgConnectFlags();
	inline const TUsbcConfiguration* CurrentConfig() const;
	inline TUsbcConfiguration* CurrentConfig();
	inline TBool InterfaceExists(TInt aNumber) const;
	inline TBool EndpointExists(TUint aAddress) const;
	inline void Buffer2Setup(const TAny* aBuf, TUsbcSetup& aSetup) const;
	inline TUint EpIdx2Addr(TUint aRealEndpoint) const;
	inline TUint EpAddr2Idx(TUint aAddress) const;
	inline void SetEp0DataOutVars(const TUsbcSetup& aPacket, const DBase* aClientId = NULL);
	inline void ResetEp0DataOutVars();
	inline TBool IsInTheStatusList(const TUsbcStatusCallback& aCallback);
	inline TBool IsInTheEpStatusList(const TUsbcEndpointStatusCallback& aCallback);
	inline TBool IsInTheOtgFeatureList(const TUsbcOtgFeatureCallback& aCallback);
	inline TBool IsInTheRequestList(const TUsbcRequestCallback& aCallback);
	static void ReconnectTimerCallback(TAny* aPtr);
	static void CableStatusTimerCallback(TAny* aPtr);
	static void PowerUpDfc(TAny* aPtr);
	static void PowerDownDfc(TAny* aPtr);

private:

	//
	// --- Private data members ---
	//

	static DUsbClientController* UsbClientController[KUsbcMaxUdcs];

	TInt iDeviceTotalEndpoints;								// number of endpoints reported by PSL
	TInt iDeviceUsableEndpoints;							// number of endpoints reported to LDD
	TUsbcDeviceState iDeviceState;							// states as of USB spec chapter 9.1
	TUsbcDeviceState iDeviceStateB4Suspend;					// state before entering suspend state
	TBool iSelfPowered;										// true if device is capable of beeing self-powered
	TBool iRemoteWakeup;									// true if device is capable of signalling rmwakeup
	TBool iTrackDeviceState;								// true if we should track the device state in s/w
	TBool iHardwareActivated;								// true if controller silicon is in operating state
	TBool iOtgSupport;										// true if OTG is supported by this device
	TBool iOtgHnpHandledByHw;								// true if HNP dis/connect is handled by hardware
	TUint8 iOtgFuncMap;										// bitmap indicating OTG extension features
	TBool iHighSpeed;										// true if currently operating at high-speed
	TUsbcSetup iSetup;										// storage for a setup packet during its DATA_OUT
	TInt iEp0MaxPacketSize;									// currently configured max packet size for Ep0
	const DBase* iEp0ClientId;								// see comment at the begin of ps_usbc.cpp
	TUint16 iEp0DataReceived;								// indicates how many bytes have already been received
	TBool iEp0DataReceiving;								// true if ep0's in DATA_OUT stage
	TBool iEp0WritePending;									// true if a write on ep0 has been set up
	TBool iEp0ClientDataTransmitting;						// true if ep0's in DATA_IN on behalf of a client
	const DBase* iEp0DeviceControl;							// Device Ep0 requests are delivered to this LDD
	TUsbcDescriptorPool iDescriptors;						// the descriptors as of USB spec chapter 9.5
	TUint8 iCurrentConfig;									// bConfigurationValue of current Config (1-based!)
	RPointerArray<TUsbcConfiguration> iConfigs;				// the root of the modelled USB device
	TUsbcPhysicalEndpoint iRealEndpoints[KUsbcEpArraySize]; // array will be filled once at startup
	TUint8 iEp0_TxBuf[KUsbcBufSz_Ep0Tx];					// ep0 outgoing (tx) data is placed here
#ifdef USB_SUPPORTS_SET_DESCRIPTOR_REQUEST
	TUint8 iEp0_RxCollectionBuf[KUsbcBufSz_Ep0Rx];			// used for (optional) SET_DESCRIPTOR request
#endif
	TInt iEp0_RxExtraCount;									// number of bytes received but not yet delivered
	TBool iEp0_RxExtraData;									// true if iEp0_RxExtraCount is valid
	TInt iEp0_TxNonStdCount;								// number of bytes requested by non-std Ep0 request
	TUsbcRequestCallback* iRequestCallbacks[KUsbcEpArraySize]; // xfer requests; indexed by real ep number
	TSglQue<TUsbcRequestCallback> iEp0ReadRequestCallbacks;	// list of ep0 read requests
	TSglQue<TUsbcClientCallback> iClientCallbacks;          // registered LDD clients and their callback functions
	TSglQue<TUsbcStatusCallback> iStatusCallbacks;			// list of device state notification requests
	TSglQue<TUsbcEndpointStatusCallback> iEpStatusCallbacks; // list of endpoint state notification requests
	TSglQue<TUsbcOtgFeatureCallback> iOtgCallbacks;		    // list of OTG feature change requests
	NTimer iReconnectTimer;									// implements USB re-enumeration delay
	NTimer iCableStatusTimer;								// implements USB cable status detection delay
	DUsbcPowerHandler* iPowerHandler;						// pointer to USB power handler object
	TSpinLock iUsbLock;                                     // implement SMP for USB PDD and LDD

protected:
	TDfc iPowerUpDfc;										// queued by power handler upon power-up
	TDfc iPowerDownDfc;										// queued by power handler upon power-down

private:
	TBool iStandby;											// toggled by power handler as appropriate
	TBool iStackIsActive;									// client stack's function is usable
	TBool iOtgClientConnect;								// OTG stack wishes to connect to the host
	TBool iClientSupportReady;								// user-side USB Client support is loaded & active
	TBool iDPlusEnabled;									// set if both sides agree and DPLUS is asserted
	TBool iUsbResetDeferred;								// set when user-side wasn't ready yet

public:
	TInt (*iEnablePullUpOnDPlus)(TAny* aOtgContext);		// these are to be filled in by the Host component
	TInt (*iDisablePullUpOnDPlus)(TAny* aOtgContext);		// in an OTG setup (otherwise unused)
	TAny* iOtgContext;										// to be passed into the above 2 functions
	};


/** Simple queue of status changes to be recorded.
	Items are fetched by userside when able.
*/
class TUsbcDeviceStatusQueue
	{
public:
	TUsbcDeviceStatusQueue();
	void AddStatusToQueue(TUint32 aDeviceStatus);
	TInt GetDeviceQueuedStatus(TUint32& aDeviceStatus);
	void FlushQueue();

private:
	TUint32 iDeviceStatusQueue[KUsbDeviceStatusQueueDepth];
	TInt iStatusQueueHead;
	};

#include <drivers/usbcshared.inl>

#endif	// __USBCSHARED_H__


















